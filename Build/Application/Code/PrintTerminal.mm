/*!	\file PrintTerminal.mm
	\brief Handles printing, by using a preview panel.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#import "PrintTerminal.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstdio>
#import <cstring>

// library includes
#import <AutoPool.objc++.h>
#import <CocoaBasic.h>
#import <CFRetainRelease.h>
#import <MemoryBlockPtrLocker.template.h>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <FlagManager.h>

// application includes
#import "Console.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Terminal.h"
#import "TerminalView.h"



#pragma mark Types

/*!
Helper class for printing from a terminal view.
*/
@interface PrintTerminal_Job : NSObject //{
{
@public
	NSString*	jobTitle;			//!< the title of the print job and preview window
	NSString*	printedText;		//!< the lines of plain text to print
	NSFont*		textFont;			//!< the font and size to use
	BOOL		isLandscapeMode;	//!< if YES, page setup defaults to landscape mode
}

// class methods
	+ (PrintTerminal_Job*)
	jobFromRef:(PrintTerminal_JobRef)_;

// new methods
	- (void)
	beginPreviewSheetModalForWindow:(NSWindow*)_;

// initializers
	- (instancetype)
	initWithString:(NSString*)_
	andFont:(NSFont*)_
	andTitle:(NSString*)_
	andLandscape:(BOOL)_;

@end //}

#pragma mark Internal Methods
namespace {

NSFont*		returnNSFontForTerminalView		(TerminalViewRef);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new object to manage printing of the text in the
given file, using the font and size of the given view.
Returns "nullptr" if any problem occurs.

(4.0)
*/
PrintTerminal_JobRef
PrintTerminal_NewJobFromFile	(CFURLRef			inFile,
								 TerminalViewRef	inView,
								 CFStringRef		inJobName,
								 Boolean			inDefaultToLandscape)
{
	AutoPool				_;
	CFRetainRelease			selectedText(TerminalView_ReturnSelectedTextCopyAsUnicode
											(inView, 0/* space/tab conversion, or zero */, 0/* flags */),
											true/* is retained */);
	PrintTerminal_JobRef	result = nullptr;
	
	
	result = REINTERPRET_CAST([[PrintTerminal_Job alloc] initWithString:[NSString stringWithContentsOfURL:BRIDGE_CAST(inFile, NSURL*)]
																		andFont:returnNSFontForTerminalView(inView)
																		andTitle:BRIDGE_CAST(inJobName, NSString*)
																		andLandscape:((inDefaultToLandscape) ? YES : NO)],
								PrintTerminal_JobRef);
	return result;
}// NewJobFromFile


/*!
Creates a new object to manage printing of the text currently
on the main screen of a terminal view, using the font and size
of that view.  Returns "nullptr" if any problem occurs.

(4.0)
*/
PrintTerminal_JobRef
PrintTerminal_NewJobFromSelectedText	(TerminalViewRef	inView,
										 CFStringRef		inJobName,
										 Boolean			inDefaultToLandscape)
{
	AutoPool				_;
	CFRetainRelease			selectedText(TerminalView_ReturnSelectedTextCopyAsUnicode
											(inView, 0/* space/tab conversion, or zero */, 0/* flags */),
											true/* is retained */);
	PrintTerminal_JobRef	result = nullptr;
	
	
	result = REINTERPRET_CAST([[PrintTerminal_Job alloc] initWithString:BRIDGE_CAST(selectedText.returnCFStringRef(), NSString*)
																		andFont:returnNSFontForTerminalView(inView)
																		andTitle:BRIDGE_CAST(inJobName, NSString*)
																		andLandscape:((inDefaultToLandscape) ? YES : NO)],
								PrintTerminal_JobRef);
	return result;
}// NewJobFromSelectedText


/*!
Creates a new object to manage printing of the text currently
on the main screen of a terminal view, using the font and size
of that view.  Returns "nullptr" if any problem occurs.

(4.0)
*/
PrintTerminal_JobRef
PrintTerminal_NewJobFromVisibleScreen	(TerminalViewRef	inView,
										 TerminalScreenRef	inViewBuffer,
										 CFStringRef		inJobName)
{
	AutoPool				_;
	TerminalView_CellRange	startEnd;
	TerminalView_CellRange	oldStartEnd;
	Boolean					isRectangular = false;
	Boolean					isSelection = TerminalView_TextSelectionExists(inView);
	Boolean					isLandscape = true;
	PrintTerminal_JobRef	result = nullptr;
	
	
	// preserve any existing selection
	if (isSelection)
	{
		TerminalView_GetSelectedTextAsVirtualRange(inView, oldStartEnd);
		isRectangular = TerminalView_TextSelectionIsRectangular(inView);
	}
	else
	{
		oldStartEnd = std::make_pair(std::make_pair(0, 0), std::make_pair(0, 0));
	}
	
	// select the entire visible screen and print it
	startEnd.first = std::make_pair(0, 0);
	startEnd.second = std::make_pair(Terminal_ReturnColumnCount(inViewBuffer),
										Terminal_ReturnRowCount(inViewBuffer));
	TerminalView_SelectVirtualRange(inView, startEnd);
	{
		HIViewRef	focusView = TerminalView_ReturnUserFocusHIView(inView);
		
		
		if (nullptr != focusView)
		{
			HIRect		bounds;
			
			
			if (noErr == HIViewGetBounds(focusView, &bounds))
			{
				isLandscape = (bounds.size.width > bounds.size.height);
			}
		}
	}
	result = PrintTerminal_NewJobFromSelectedText(inView, inJobName, isLandscape);
	
	// clear the full screen selection, restoring any previous selection
	TerminalView_MakeSelectionsRectangular(inView, isRectangular);
	if (isSelection)
	{
		TerminalView_SelectVirtualRange(inView, oldStartEnd);
	}
	else
	{
		TerminalView_SelectNothing(inView);
	}
	
	return result;
}// NewJobFromVisibleScreen


/*!
Decreases the retain count of the specified print job object
and sets your copy of the reference to "nullptr".  If the
object is no longer in use, it is destroyed.

(4.0)
*/
void
PrintTerminal_ReleaseJob	(PrintTerminal_JobRef*		inoutRefPtr)
{
	AutoPool				_;
	PrintTerminal_Job*		ptr = [PrintTerminal_Job jobFromRef:*inoutRefPtr];
	
	
	[ptr release];
	*inoutRefPtr = nullptr;
}// ReleaseJob


/*!
Returns true only if printing is supported.  This should be
used to hide print-related commands in user interfaces, if
printing is not possible.

Now that a Cocoa runtime is used, there are no known issues
with printing (even on older Mac OS X versions).  So, this
routine always returns "true".

(4.0)
*/
Boolean
PrintTerminal_IsPrintingSupported ()
{
	Boolean		result = true;
	
	
	return result;
}// IsPrintingSupported


/*!
Allows the user to send the specified job to the printer.
A preview interface is first displayed.

The job is retained internally, so it is safe to release it
immediately with PrintTerminal_ReleaseJob().

\retval kPrintTerminal_ResultOK
if no error occurred

\retval kPrintTerminal_ResultInvalidID
if "inRef" is not recognized

(4.0)
*/
PrintTerminal_Result
PrintTerminal_JobSendToPrinter	(PrintTerminal_JobRef	inRef,
								 HIWindowRef			inParentWindowOrNull)
{
	AutoPool				_;
	PrintTerminal_Job*		ptr = [PrintTerminal_Job jobFromRef:inRef];
	PrintTerminal_Result	result = kPrintTerminal_ResultOK;
	
	
	if (nullptr == ptr) result = kPrintTerminal_ResultInvalidID;
	else
	{
		NSWindow*	window = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(inParentWindowOrNull);
		
		
		// run the sheet, which will also retain the job object
		[ptr beginPreviewSheetModalForWindow:window];
	}
	return result;
}// JobSendToPrinter


#pragma mark Internal Methods
namespace {

/*!
Returns a system-managed NSFont object that represents
the font and size currently in use by the specified
terminal view.

(4.0)
*/
NSFont*
returnNSFontForTerminalView		(TerminalViewRef	inView)
{
	NSFont*			result = nil;
	CFStringRef		fontName = nullptr;
	UInt16			fontSize = 0;
	
	
	// find font information from the Terminal View; yes, this
	// is a painfully old way to specify fonts!
	TerminalView_GetFontAndSize(inView, &fontName, &fontSize);
	result = [NSFont fontWithName:BRIDGE_CAST(fontName, NSString*) size:fontSize];
	
	return result;
}// returnNSFontForTerminalView

} // anonymous namespace


#pragma mark -
@implementation PrintTerminal_Job


/*!
Initialize with the (perhaps multi-line) string of text that
is to be printed.

Designated initializer.

(4.0)
*/
- (instancetype)
initWithString:(NSString*)		aString
andFont:(NSFont*)				aFont
andTitle:(NSString*)			aTitle
andLandscape:(BOOL)				landscapeMode
{
	self = [super init];
	if (nil != self)
	{
		jobTitle = [aTitle retain];
		printedText = [aString retain];
		textFont = [aFont retain];
		isLandscapeMode = landscapeMode;
	}
	return self;
}// initWithString:andFont:andTitle:andLandscape:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[jobTitle release];
	[printedText release];
	[textFont release];
	[super dealloc];
}// dealloc


/*!
Creates (if necessary) and displays the print preview sheet,
with the specified text.  The given text will be printed if
the user chooses to proceed.

(4.0)
*/
- (void)
beginPreviewSheetModalForWindow:(NSWindow*)		aWindow
{
	// create a preview sheet; this is released after the sheet closes
	PrintTerminal_PreviewPanelController*	preview = [[PrintTerminal_PreviewPanelController alloc]
														initWithString:self->printedText andFont:self->textFont
														andTitle:self->jobTitle andLandscape:self->isLandscapeMode];
	
	
	// run the sheet
	[preview beginPreviewSheetModalForWindow:aWindow];
}// beginPreviewSheetModalForWindow:


/*!
Returns the internal object that is equivalent to the
given external reference.

(4.0)
*/
+ (PrintTerminal_Job*)
jobFromRef:(PrintTerminal_JobRef)	ref
{
	return (PrintTerminal_Job*)ref;
}// jobFromRef:


@end // PrintTerminal_Job


#pragma mark -
@implementation PrintTerminal_PreviewPanelController


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithString:(NSString*)		aString
andFont:(NSFont*)				aFont
andTitle:(NSString*)			aTitle
andLandscape:(BOOL)				landscapeMode
{
	self = [super initWithWindowNibName:@"PrintPreviewCocoa"];
	if (nil != self)
	{
		previewTitle = [aTitle retain];
		previewText = [aString retain];
		previewFont = [aFont retain];
		pageSetup = [NSPrintInfo sharedPrintInfo];
		paperInfo = [[NSString string] retain];
		[self setFontSize:[[NSNumber numberWithFloat:[previewFont pointSize]] stringValue]];
		
		// initialize the page setup to some values that are saner
		// for printing terminal text
		if (landscapeMode)
		{
			[pageSetup setOrientation:NSLandscapeOrientation];
		}
		else
		{
			[pageSetup setOrientation:NSPortraitOrientation];
		}
		[pageSetup setHorizontalPagination:NSFitPagination];
		[pageSetup setHorizontallyCentered:NO];
		[pageSetup setVerticallyCentered:NO];
	}
	[self setShouldCascadeWindows:NO];
	return self;
}// initWithString:andFont:andTitle:andLandscape:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[previewTitle release];
	[previewText release];
	[previewFont release];
	[paperInfo release];
	[fontSize release];
	[super dealloc];
} // dealloc


/*!
Creates (if necessary) and displays the print preview sheet.
Text will be printed if the user chooses to proceed.

IMPORTANT:	Despite its name, this is not yet implemented as a
			sheet, because Cocoa-on-Carbon sheets are too flaky.
			Once the application becomes all-Cocoa, this will
			work as expected.

(4.0)
*/
- (void)
beginPreviewSheetModalForWindow:(NSWindow*)		aWindow
{
#pragma unused(aWindow)
	NSWindow*	sheet = [self window];
	
	
	// sheet is UNIMPLEMENTED - use a modal dialog
	if (nil != self->previewTitle)
	{
		[sheet setTitle:self->previewTitle];
	}
	[NSApp beginSheet:sheet modalForWindow:nil
							modalDelegate:nil didEndSelector:nil
							contextInfo:nil];
	UNUSED_RETURN(long)[NSApp runModalForWindow:[self window]];
	[NSApp endSheet:sheet];
	[sheet orderOut:self];
}// beginPreviewSheetModalForWindow:


/*!
Closes the sheet without doing anything.

(4.0)
*/
- (IBAction)
cancel:(id)		sender
{
#pragma unused(sender)
	[NSApp stopModal];
}// cancel:


/*!
Displays relevant help information for the user.

(4.0)
*/
- (IBAction)
help:(id)	sender
{
#pragma unused(sender)
	// TEMPORARY - add contextual help
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpWithoutContext();
}// help:


/*!
Displays the Page Setup sheet for this print job.

(4.0)
*/
- (IBAction)
pageSetup:(id)	sender
{
#pragma unused(sender)
	NSPrintInfo*	info = self->pageSetup;
	NSPageLayout*	layout = [NSPageLayout pageLayout];
	
	
	[layout beginSheetWithPrintInfo:info modalForWindow:[self window]
											delegate:self
											didEndSelector:@selector(pageLayoutDidEnd:returnCode:contextInfo:)
											contextInfo:nil];
}// pageSetup:


/*!
Asks the preview text view to print itself, and closes
the sheet.

(4.0)
*/
- (IBAction)
print:(id)	sender
{
#pragma unused(sender)
	NSPrintOperation*	op = [NSPrintOperation printOperationWithView:self->previewPane printInfo:self->pageSetup];
	
	
	[op runOperationModalForWindow:[self window] delegate:self
													didRunSelector:@selector(printOperationDidRun:success:contextInfo:)
													contextInfo:nil];
}// print:


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (NSString*)
paperInfo
{
	return paperInfo;
}
- (void)
setPaperInfo:(NSString*)	aString
{
	if (aString != paperInfo)
	{
		[paperInfo release];
		paperInfo = [aString retain];
	}
}// setPaperInfo:


/*!
Accessor.

(4.0)
*/
- (NSString*)
fontSize
{
	return fontSize;
}
- (void)
setFontSize:(NSString*)		aString
{
	if (aString != fontSize)
	{
		[fontSize release];
		fontSize = [aString retain];
		
		if (nil != self->previewFont)
		{
			self->previewFont = [NSFont fontWithName:[self->previewFont fontName] size:[aString floatValue]];
			[self->previewPane setFont:self->previewFont];
		}
	}
}// setFontSize:


/*!
Accessor.

(4.0)
*/
- (NSString*)
maximumSensibleFontSize
{
	return [NSString stringWithFormat:@"%d", 48]; // arbitrary
}// maximumSensibleFontSize


/*!
Accessor.

(4.0)
*/
- (NSString*)
minimumSensibleFontSize
{
	return [NSString stringWithFormat:@"%d", 8]; // arbitrary
}// minimumSensibleFontSize


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init...".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	
	// update the text view with the requested string
	[[[self->previewPane textStorage] mutableString] setString:self->previewText];
	if (nil != self->previewFont)
	{
		[self->previewPane setFont:self->previewFont];
	}
	[self setPaperInfo:[self->pageSetup localizedPaperName]];
}// windowDidLoad


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"PrintPreview";
}// windowFrameAutosaveName


#pragma mark Internal Methods


/*!
Responds to the closing of a page layout configuration sheet.

(4.0)
*/
- (void)
pageLayoutDidEnd:(NSPageLayout*)	pageLayout
returnCode:(int)					returnCode
contextInfo:(void*)					contextInfo
{
#pragma unused(pageLayout, returnCode, contextInfo)
	[self setPaperInfo:[self->pageSetup localizedPaperName]];
}// pageLayoutDidEnd:returnCode:contextInfo:


/*!
Responds to the closing of a printing configuration sheet
(or other completion of printing) by finally closing the
preview dialog.

(4.0)
*/
- (void)
printOperationDidRun:(NSPrintOperation*)	printOperation
success:(BOOL)								success
contextInfo:(void*)							contextInfo
{
#pragma unused(printOperation, contextInfo)
	if (success)
	{
		[NSApp stopModal];
	}
}// printOperationDidRun:success:contextInfo:


@end // PrintTerminal_PreviewPanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
