/*!	\file PrintTerminal.mm
	\brief Handles printing, by using a preview panel.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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
#import <cstring>
#import <utility>

// library includes
#import <CocoaBasic.h>
#import <CFRetainRelease.h>

// Mac includes
@import ApplicationServices;
@import CoreServices;

// library includes
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "ChildProcessWC.objc++.h"
#import "Console.h"
#import "ConstantsRegistry.h"
#import "Terminal.h"
#import "TerminalView.h"



#pragma mark Types

/*!
Private properties.
*/
@interface PrintTerminal_Job () //{

// accessors
	//! If YES, page setup defaults to landscape mode.
	@property (assign) BOOL
	isLandscapeMode;
	//! The title of the print job and preview window.
	@property (strong) NSString*
	jobTitle;
	//! The lines of plain text to print.
	@property (strong) NSString*
	printedText;
	//! The font and size to use.
	@property (strong) NSFont*
	textFont;

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

The returned reference is ARC-compliant so use a strong
reference if necessary (otherwise release is implicit).

(4.0)
*/
PrintTerminal_JobRef
PrintTerminal_NewJobFromFile	(CFURLRef			inFile,
								 TerminalViewRef	inView,
								 CFStringRef		inJobName,
								 Boolean			inDefaultToLandscape)
{
@autoreleasepool {
	CFRetainRelease			selectedText(TerminalView_ReturnSelectedTextCopyAsUnicode
											(inView, 0/* space/tab conversion, or zero */, 0/* flags */),
											CFRetainRelease::kAlreadyRetained);
	NSError*				error = nil;
	NSString*				printFileString = [NSString stringWithContentsOfURL:BRIDGE_CAST(inFile, NSURL*)
																				encoding:NSUTF8StringEncoding
																				error:&error];
	PrintTerminal_JobRef	result = nullptr;
	
	
	if (nil != error)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteValueCFString, "failed to read string from temporary printing file, error",
						BRIDGE_CAST([error localizedDescription], CFStringRef));
	}
	else
	{
		result = [[PrintTerminal_Job alloc] initWithString:printFileString
															font:returnNSFontForTerminalView(inView)
															title:BRIDGE_CAST(inJobName, NSString*)
															landscape:((inDefaultToLandscape) ? YES : NO)];
	}
	
	return result;
}// @autoreleasepool
}// NewJobFromFile


/*!
Creates a new object to manage printing of the text currently
on the main screen of a terminal view, using the font and size
of that view.  Returns "nullptr" if any problem occurs.

The returned reference is ARC-compliant so use a strong
reference if necessary (otherwise release is implicit).

(4.0)
*/
PrintTerminal_JobRef
PrintTerminal_NewJobFromSelectedText	(TerminalViewRef	inView,
										 CFStringRef		inJobName,
										 Boolean			inDefaultToLandscape)
{
@autoreleasepool {
	CFRetainRelease			selectedText(TerminalView_ReturnSelectedTextCopyAsUnicode
											(inView, 0/* space/tab conversion, or zero */, 0/* flags */),
											CFRetainRelease::kAlreadyRetained);
	PrintTerminal_JobRef	result = nullptr;
	
	
	result = [[PrintTerminal_Job alloc] initWithString:BRIDGE_CAST(selectedText.returnCFStringRef(), NSString*)
														font:returnNSFontForTerminalView(inView)
														title:BRIDGE_CAST(inJobName, NSString*)
														landscape:((inDefaultToLandscape) ? YES : NO)];
	return result;
}// @autoreleasepool
}// NewJobFromSelectedText


/*!
Creates a new object to manage printing of the text currently
on the main screen of a terminal view, using the font and size
of that view.  Returns "nullptr" if any problem occurs.

The returned reference is ARC-compliant so use a strong
reference if necessary (otherwise release is implicit).

(4.0)
*/
PrintTerminal_JobRef
PrintTerminal_NewJobFromVisibleScreen	(TerminalViewRef	inView,
										 TerminalScreenRef	inViewBuffer,
										 CFStringRef		inJobName)
{
@autoreleasepool {
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
		NSView*		focusView = TerminalView_ReturnUserFocusNSView(inView);
		
		
		if (nil != focusView)
		{
			isLandscape = (NSWidth(focusView.frame) > NSHeight(focusView.frame));
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
}// @autoreleasepool
}// NewJobFromVisibleScreen


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
								 NSWindow*				inParentWindowOrNil)
{
@autoreleasepool {
	PrintTerminal_Result	result = kPrintTerminal_ResultOK;
	
	
	if (nullptr == inRef)
	{
		result = kPrintTerminal_ResultInvalidID;
	}
	else
	{
		NSPasteboard*			textToPrintPasteboard = [NSPasteboard pasteboardWithUniqueName];
		NSString*				pasteboardName = textToPrintPasteboard.name;
		NSString*				titleString = ((nil != inRef.jobTitle)
												? inRef.jobTitle
												: @"");
		NSMutableDictionary*	appEnvDict = [[NSMutableDictionary alloc] initWithCapacity:5/* arbitrary */];
		CFErrorRef				errorRef = nullptr;
		
		
		// MUST correspond to environment variables expected
		// by the Print Preview internal application (also,
		// every value must be an NSString* type)
		[appEnvDict setValue:titleString forKey:@"MACTERM_PRINT_PREVIEW_JOB_TITLE"];
		[appEnvDict setValue:inRef.textFont.fontName forKey:@"MACTERM_PRINT_PREVIEW_FONT_NAME"];
		[appEnvDict setValue:[NSString stringWithFormat:@"%f", STATIC_CAST(inRef.textFont.pointSize, float)]
								forKey:@"MACTERM_PRINT_PREVIEW_FONT_SIZE"];
		[appEnvDict setValue:[NSString stringWithFormat:@"%f", STATIC_CAST(inParentWindowOrNil.contentView.frame.size.width, float)]
								forKey:@"MACTERM_PRINT_PREVIEW_PIXEL_WIDTH_HINT"];
		[appEnvDict setValue:pasteboardName forKey:@"MACTERM_PRINT_PREVIEW_PASTEBOARD_NAME"];
		[appEnvDict setValue:((inRef.isLandscapeMode) ? @"1" : @"0") forKey:@"MACTERM_PRINT_PREVIEW_IS_LANDSCAPE"];
		
		// share the terminal text with the print-preview application
		[textToPrintPasteboard clearContents];
		if (nil == inRef.printedText)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "no terminal text available to print!");
		}
		else if (NO == [textToPrintPasteboard writeObjects:@[inRef.printedText]])
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "unable to write terminal text to new pasteboard");
		}
		else
		{
			NSRunningApplication*	runHandle = AppResources_LaunchPrintPreview
												(BRIDGE_CAST(appEnvDict, CFDictionaryRef), &errorRef);
			
			
			if (nullptr != errorRef)
			{
				[NSApp presentError:BRIDGE_CAST(errorRef, NSError*)];
			}
			else
			{
				// success; sub-process will display print preview interface asynchronously;
				// create a local proxy window so that it is possible to give the illusion
				// of a window that is present in a single application’s window rotation
				// (despite the autoreleased return value, the window is immediately “shown”
				// offscreen and the controller is internally retained until the window closes)
				ChildProcessWC_AtExitBlockType	atExit =
												^{
													// nothing needed
													//NSLog(@"Print Preview at-exit called"); // debug
												};
				ChildProcessWC_Object*			proxyWindowController = nil;
				
				
				proxyWindowController = [ChildProcessWC_Object
											childProcessWCWithRunningApp:runHandle atExit:atExit];
				[proxyWindowController.window orderBack:nil]; // NOTE: not needed, except to “use” the variable
			}
		}
	}
	return result;
}// @autoreleasepool
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
	CGFloat			fontSize = 0;
	
	
	// find font information from the Terminal View; yes, this
	// is a painfully old way to specify fonts!
	TerminalView_GetFontAndSize(inView, &fontName, &fontSize);
	result = [NSFont fontWithName:BRIDGE_CAST(fontName, NSString*) size:fontSize];
	
	return result;
}// returnNSFontForTerminalView

} // anonymous namespace


#pragma mark -
@implementation PrintTerminal_Job //{


#pragma mark Initializers


/*!
Initialize with the (perhaps multi-line) string of text that
is to be printed.

Designated initializer.

(4.0)
*/
- (instancetype)
initWithString:(NSString*)	aString
font:(NSFont*)				aFont
title:(NSString*)			aTitle
landscape:(BOOL)			landscapeMode
{
	self = [super init];
	if (nil != self)
	{
		_isLandscapeMode = landscapeMode;
		_jobTitle = aTitle;
		_printedText = aString;
		_textFont = aFont;
	}
	return self;
}// initWithString:font:title:landscape:


#pragma mark Initializers Disabled From Superclass


/*!
This is not a valid way to initialize this class.

(2021.01)
*/
- (instancetype)
init
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithString:@"" font:nil title:nil landscape:NO];
}// init


@end //} PrintTerminal_Job

// BELOW IS REQUIRED NEWLINE TO END FILE
