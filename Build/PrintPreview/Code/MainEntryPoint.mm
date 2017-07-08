/*!	\file MainEntryPoint.mm
	\brief Front end to the Print Preview application.
*/
/*###############################################################

	Print Preview
		© 2005-2017 by Kevin Grant.
	
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

#import "UniversalDefines.h"

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaExtensions.objc++.h>



#pragma mark Types

/*!
Implements Print Preview.  See "TerminalPrintDialogCocoa.xib".
*/
@interface MainEntryPoint_TerminalPrintDialogWC : NSWindowController //{
{
@private
	IBOutlet NSTextView*	previewPane;
	NSString*				previewTitle;
	NSString*				previewText;
	NSFont*					previewFont;
	NSPrintInfo*			pageSetup;
	NSString*				paperInfo;
	NSString*				fontSize;
}

// initializers
	- (instancetype)
	initWithString:(NSString*)_
	font:(NSFont*)_
	title:(NSString*)_
	landscape:(BOOL)_;

// accessors
	- (NSString*)
	fontSize;
	- (void)
	setFontSize:(NSString*)_; // binding
	- (NSString*)
	maximumSensibleFontSize; // binding
	- (NSString*)
	minimumSensibleFontSize; // binding
	- (NSString*)
	paperInfo;
	- (void)
	setPaperInfo:(NSString*)_; // binding

// actions
	- (IBAction)
	cancel:(id)_;
	- (IBAction)
	help:(id)_;
	- (IBAction)
	pageSetup:(id)_;
	- (IBAction)
	print:(id)_;

@end //}

/*!
The delegate for NSApplication.
*/
@interface PrintPreviewAppDelegate : NSObject //{
{
@private
	MainEntryPoint_TerminalPrintDialogWC*	_panelWC;
	NSMutableArray*							_registeredObservers;
}

// accessors
	@property (strong) MainEntryPoint_TerminalPrintDialogWC*
	panelWC;
	@property (strong) NSRunningApplication*
	parentRunningApp;

// actions
	- (IBAction)
	orderFrontNextWindow:(id)_;
	- (IBAction)
	orderFrontPreviousWindow:(id)_;

// NSApplicationDelegate
	- (void)
	applicationDidFinishLaunching:(NSNotification*)_;

@end //}



#pragma mark Public Methods

/*!
Main entry point.
*/
int
main	(int			argc,
		 const char*	argv[])
{
	return NSApplicationMain(argc, argv);
}


#pragma mark Internal Methods

@implementation PrintPreviewAppDelegate //{


#pragma mark Initializers


/*!
Destructor.

(2016.09)
*/
- (void)
dealloc
{
	[self removeObserversSpecifiedInArray:_registeredObservers];
	[_registeredObservers release];
	[super dealloc];
}// dealloc


#pragma mark Actions


/*!
Rotates forward through windows; since this is an
internal application that creates the illusion of
a seamless interface with the parent, the “next
window” could actually be in a different application.

(2016.09)
*/
- (IBAction)
orderFrontNextWindow:(id)	sender
{
#pragma unused(sender)
	[self.parentRunningApp activateWithOptions:0L];
}// orderFrontNextWindow:


/*!
Rotates backward through windows; since this is an
internal application that creates the illusion of
a seamless interface with the parent, the “previous
window” could actually be in a different application.

(2016.09)
*/
- (IBAction)
orderFrontPreviousWindow:(id)	sender
{
#pragma unused(sender)
	[self.parentRunningApp activateWithOptions:0L];
}// orderFrontPreviousWindow:


#pragma mark NSApplicationDelegate

/*!
Responds to application startup by keeping track of
the parent process, gathering configuration data from
the environment and then displaying a Print Preview
window with the appropriate terminal text (in the
specified font, etc.).

(2016.09)
*/
- (void)
applicationDidFinishLaunching:(NSNotification*)		aNotification
{
#pragma unused(aNotification)

	self->_registeredObservers = [[NSMutableArray alloc] init];
	
	// detect when parent process is terminated
	self->_parentRunningApp = nil;
	{
		char const*		pathSelf = [[[[NSBundle mainBundle] bundleURL] absoluteURL] fileSystemRepresentation];
		
		
		//NSLog(@"helper application: %s", pathSelf); // debug
		for (NSRunningApplication* runningApp : [NSRunningApplication runningApplicationsWithBundleIdentifier:@"net.macterm.MacTerm"])
		{
			// when searching for the parent application, consider not only
			// the bundle ID but the location (in case the user has somehow
			// started more than one, use the one that physically contains
			// this internal application bundle)
			char const*		appPath = [[runningApp.bundleURL absoluteURL] fileSystemRepresentation];
			
			
			if (0 == strncmp(appPath, pathSelf, strlen(appPath)))
			{
				// absolute root directories match; this is the right parent
				self.parentRunningApp = runningApp;
				//NSLog(@"found parent application: %s", appPath); // debug
				break;
			}
		}
		
		if (nil == self.parentRunningApp)
		{
			NSLog(@"terminating because parent application cannot be found");
			[NSApp terminate:nil];
		}
		
		[self->_registeredObservers addObject:[[self newObserverFromKeyPath:@"terminated" ofObject:self.parentRunningApp
																			options:(NSKeyValueChangeSetting)
																			context:nil] autorelease]];
	}
	
	// key configuration data MUST be passed through the environment;
	// if any expected variable is not available, it is an error (the
	// main application code that spawns Print Preview must use these
	// same environment variable names)
	char*			jobTitleRaw = getenv("MACTERM_PRINT_PREVIEW_JOB_TITLE");
	char*			pasteboardNameRaw = getenv("MACTERM_PRINT_PREVIEW_PASTEBOARD_NAME");
	char*			fontNameRaw = getenv("MACTERM_PRINT_PREVIEW_FONT_NAME");
	char*			fontSizeRaw = getenv("MACTERM_PRINT_PREVIEW_FONT_SIZE");
	char*			isLandscapeRaw = getenv("MACTERM_PRINT_PREVIEW_IS_LANDSCAPE");
	NSString*		jobTitleString = [NSString stringWithUTF8String:jobTitleRaw];
	NSString*		pasteboardNameString = [NSString stringWithUTF8String:pasteboardNameRaw];
	NSString*		fontNameString = [NSString stringWithUTF8String:fontNameRaw];
	NSString*		fontSizeString = [NSString stringWithUTF8String:fontSizeRaw];
	BOOL			isLandscape = ((nullptr == isLandscapeRaw)
									? NO
									: (0 == strcmp(isLandscapeRaw, "1")));
	NSPasteboard*	textToPrintPasteboard = nil; // see below
	NSString*		previewText = @"";
	float			fontSize = 12.0; // see below
	
	
	if ((nil == jobTitleString) || (nil == pasteboardNameString) ||
		(nil == fontNameString) || (nil == fontSizeString))
	{
		NSLog(@"terminating because one or more required environment variables are not set");
		[NSApp terminate:nil];
	}
	
	// convert string value of font size environment variable into a number
	{
		NSScanner*	fontSizeScanner = [NSScanner scannerWithString:fontSizeString];
		
		
		if (NO == [fontSizeScanner scanFloat:&fontSize])
		{
			NSLog(@"warning, failed to parse “%@” into a valid font size; ignoring", fontSizeString);
			fontSize = 12.0; // arbitrary work-around
		}
	}
	
	// find the unique pasteboard that was given
	{
		NSArray*	dataClasses = @[NSString.class];
		
		
		textToPrintPasteboard = [NSPasteboard pasteboardWithName:pasteboardNameString];
		if (NO == [textToPrintPasteboard canReadObjectForClasses:dataClasses options:nil])
		{
			NSLog(@"launch environment for print preview has not specified any preview text");
		}
		else
		{
			NSArray*	dataArray = [textToPrintPasteboard readObjectsForClasses:dataClasses options:nil];
			
			
			if (nil == dataArray)
			{
				NSLog(@"failed to retrieve text for print preview");
			}
			else if (0 == dataArray.count)
			{
				NSLog(@"no data found for print preview");
			}
			else
			{
				id		dataObject = [dataArray objectAtIndex:0];
				
				
				if (NO == [dataObject isKindOfClass:NSString.class])
				{
					NSLog(@"no suitable data found for print preview");
				}
				else
				{
					previewText = STATIC_CAST(dataObject, NSString*);
				}
			}
		}
	}
	
#if 0
	NSLog(@"RECEIVED JOB TITLE: “%@”", jobTitleString); // debug
	NSLog(@"RECEIVED PASTEBOARD NAME: “%@”", pasteboardNameString); // debug
	NSLog(@"RESOLVED TO PASTEBOARD: “%@”", textToPrintPasteboard); // debug
	NSLog(@"PASTEBOARD TEXT: “%@”", previewText); // debug
	NSLog(@"RECEIVED FONT: “%@”", fontNameString); // debug
	NSLog(@"RECEIVED FONT SIZE (converted): “%f”", fontSize); // debug
	NSLog(@"RECEIVED LANDSCAPE MODE: %d", (int)isLandscape); // debug
#endif
	
	// will no longer need this pasteboard anywhere
	[textToPrintPasteboard releaseGlobally];
	
	// bring this process to the front
	[NSApp activateIgnoringOtherApps:YES];
	
	// create a preview sheet; this is retained by the controller
	self.panelWC = [[[MainEntryPoint_TerminalPrintDialogWC alloc]
					initWithString:previewText
					font:[NSFont fontWithName:fontNameString size:fontSize]
					title:jobTitleString landscape:isLandscape] autorelease];
	[self.panelWC.window orderFront:nil];
	
	//	FIXME: also, can window rotation include the sub-application window, and can sub rotate to parent?
}// applicationDidFinishLaunching:


/*!
Returns YES to cause this internal application to
quit as soon as the preview is dismissed for any
reason.

(2016.09)
*/
- (BOOL)
applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)		anApp
{
#pragma unused(anApp)
	return YES;
}// applicationShouldTerminateAfterLastWindowClosed:


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to observed key values.

(2016.09)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
#pragma unused(anObject, aContext)
	//if (NO == self.disableObservers)
	{
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			if ([aKeyPath isEqualToString:@"terminated"])
			{
				// it is not necessary to kill the print preview application
				// simply because the parent exited; in fact, it could be good
				// not to, as an abnormal exit indicates a crash (the user can
				// continue to print!); the exception is if the preview window
				// has already closed, in which case the process is useless
				if (self.panelWC.window.isVisible)
				{
					NSLog(@"detected parent exit; ignoring because preview is still active");
				}
				else
				{
					// theoretically this case should not happen because the
					// application is configured to quit as soon as the last
					// window closes; still, being explicit doesn’t hurt
					NSLog(@"detected parent exit and Print Preview window now closed; terminating");
					[NSApp terminate:nil];
				}
			}
		}
	}
}// observeValueForKeyPath:ofObject:change:context:


@end //} PrintPreviewAppDelegate


#pragma mark -
@implementation MainEntryPoint_TerminalPrintDialogWC //{


/*!
Designated initializer.

(2016.09)
*/
- (instancetype)
initWithString:(NSString*)	aString
font:(NSFont*)				aFont
title:(NSString*)			aTitle
landscape:(BOOL)			landscapeMode
{
	self = [super initWithWindowNibName:@"TerminalPrintDialogCocoa"];
	if (nil != self)
	{
		previewTitle = [aTitle retain];
		previewText = [aString retain];
		previewFont = [aFont retain];
		pageSetup = [NSPrintInfo sharedPrintInfo];
		paperInfo = [[NSString string] retain];
		[self setFontSize:[NSString stringWithFormat:@"%.2f", STATIC_CAST([previewFont pointSize], float)]];
		
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
}// initWithString:font:title:landscape:


/*!
Destructor.

(2016.09)
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
Closes the sheet without doing anything.

(2016.09)
*/
- (IBAction)
cancel:(id)		sender
{
	[self.window orderOut:sender];
}// cancel:


/*!
Displays relevant help information for the user.

(2016.09)
*/
- (IBAction)
help:(id)	sender
{
#pragma unused(sender)
	// TEMPORARY - add contextual help
	NSString*	bookName = [[NSBundle bundleWithIdentifier:@"net.macterm.MacTerm"]/* TEMPORARY; externalize dependency? */
							objectForInfoDictionaryKey:@"CFBundleHelpBookName"];
	
	
	[[NSHelpManager sharedHelpManager] openHelpAnchor:@"#" inBook:bookName];
}// help:


/*!
Displays the Page Setup sheet for this print job.

(2016.09)
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

(2016.09)
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

(2016.09)
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

(2016.09)
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
		float const		kAsFloat = [aString floatValue];
		NSString*		formattedString = [NSString stringWithFormat:@"%.2f", kAsFloat];
		
		
		[fontSize release];
		fontSize = [formattedString retain];
		
		if (nil != self->previewFont)
		{
			self->previewFont = [NSFont fontWithName:[self->previewFont fontName] size:kAsFloat];
			[self->previewPane setFont:self->previewFont];
		}
	}
}// setFontSize:


/*!
Accessor.

(2016.09)
*/
- (NSString*)
maximumSensibleFontSize
{
	return [NSString stringWithFormat:@"%d", 48]; // arbitrary
}// maximumSensibleFontSize


/*!
Accessor.

(2016.09)
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

(2016.09)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	
	// update the text view with the requested string
	self.window.title = self->previewTitle;
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

(2016.09)
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

(2016.09)
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

(2016.09)
*/
- (void)
printOperationDidRun:(NSPrintOperation*)	printOperation
success:(BOOL)								success
contextInfo:(void*)							contextInfo
{
#pragma unused(printOperation, contextInfo)
	if (success)
	{
		[self.window orderOut:nil];
	}
}// printOperationDidRun:success:contextInfo:


@end //} MainEntryPoint_TerminalPrintDialogWC

// BELOW IS REQUIRED NEWLINE TO END FILE
