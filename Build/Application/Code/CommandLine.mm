/*!	\file CommandLine.mm
	\brief Cocoa implementation of the floating command line.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#import "CommandLine.h"
#include <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <Console.h>
#import <SoundSystem.h>

// application includes
#import "HelpSystem.h"
#import "Preferences.h"
#import "Session.h"
#import "SessionFactory.h"



#pragma mark Internal Method Prototypes
namespace {

void	sendInputSessionOp		(SessionRef, void*, SInt32, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

CommandLine_PanelController*	gCommandLine_PanelController = nil;

} // anonymous namespace



#pragma mark Public Methods

/*!
If the command line window was visible the last time the
application quit, then this will initialize and display it;
otherwise, nothing is done (deferring initialization until
the window is actually requested).

(4.0)
*/
void
CommandLine_Init ()
{
	Boolean		windowIsVisible = false;
	
	
	unless (kPreferences_ResultOK ==
			Preferences_GetData(kPreferences_TagWasCommandLineShowing,
								sizeof(windowIsVisible), &windowIsVisible))
	{
		windowIsVisible = false; // assume invisible if the preference can’t be found
	}
	
	if (windowIsVisible)
	{
		CommandLine_Display();
	}
}// Init


/*!
Saves the visibility of the command line window.  Nothing is
saved if the window was never used.

(4.0)
*/
void
CommandLine_Done ()
{
	// do not initialize the window here if it was never constructed!
	if (nil != gCommandLine_PanelController)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		Boolean				windowIsVisible = false;
		
		
		// save current visibility
		windowIsVisible = [[[CommandLine_PanelController sharedCommandLinePanelController] window] isVisible];
		prefsResult = Preferences_SetData(kPreferences_TagWasCommandLineShowing,
											sizeof(windowIsVisible), &windowIsVisible);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to store visibility of command line, error", prefsResult);
		}
		
		if (NO == [[NSUserDefaults standardUserDefaults] synchronize])
		{
			Console_Warning(Console_WriteLine, "failed to save command line visibility setting");
		}
	}
}// Done


/*!
Shows the command line floating window, and focuses it.

(3.1)
*/
void
CommandLine_Display ()
{
@autoreleasepool {
	[[CommandLine_PanelController sharedCommandLinePanelController] showWindow:NSApp];
}// @autoreleasepool
}// Display


#pragma mark Internal Methods
namespace {

/*!
This method, of standard "SessionFactory_SessionOpProcPtr"
form, is used for the “multi-terminal input” mode to send
the same input to all sessions.

(4.1)
*/
void
sendInputSessionOp	(SessionRef		inSession,
					 void*			inInputCFStringRef,
					 SInt32			UNUSED_ARGUMENT(inData2),
					 void*			UNUSED_ARGUMENT(inoutResultPtr))
{
	Session_UserInputCFString(inSession, BRIDGE_CAST(inInputCFStringRef, CFStringRef));
	Session_SendNewline(inSession, kSession_EchoCurrentSessionValue);
}// sendInputSessionOp

} // anonymous namespace


#pragma mark -
@implementation CommandLine_HistoryDataSource


/*!
Designated initializer.

(3.1)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		// arrays grow automatically, this is just an initial size
		_commandHistoryArray = [[NSMutableArray alloc] initWithCapacity:15/* initial capacity; arbitrary */];
	}
	return self;
}// init


/*!
Returns the array of strings for recent command lines.

(3.1)
*/
- (NSMutableArray*)
historyArray
{
	return _commandHistoryArray;
}// historyArray


#pragma mark NSComboBoxDataSource


/*!
For auto-completion support; has no effect.

(3.1)
*/
- (NSString*)
comboBox:(NSComboBox*)			aComboBox
completedString:(NSString*)		string
{
#pragma unused(aComboBox, string)
	// this combo box does not support completion
	return nil;
}// comboBox:completedString:


/*!
Returns the index into the underlying array for the first
item that matches the given string value.

(3.1)
*/
- (unsigned int)
comboBox:(NSComboBox*)					aComboBox
indexOfItemWithStringValue:(NSString*)	string
{
#pragma unused(aComboBox)
	return [_commandHistoryArray indexOfObject:string];
}// comboBox:indexOfItemWithStringValue:


/*!
Returns the exact object (string) in the underlying array
at the given index.

(3.1)
*/
- (id)
comboBox:(NSComboBox*)				aComboBox
objectValueForItemAtIndex:(int)		index
{
#pragma unused(aComboBox)
	return [_commandHistoryArray objectAtIndex:index];
}// comboBox:objectValueForItemAtIndex:


/*!
Returns the size of the underlying array.

(3.1)
*/
- (int)
numberOfItemsInComboBox:(NSComboBox*)	aComboBox
{
#pragma unused(aComboBox)
	return [_commandHistoryArray count];
}// numberOfItemsInComboBox:


@end // CommandLine_HistoryDataSource


#pragma mark -
@implementation CommandLine_TerminalLikeComboBox


/*!
Sets the color of the cursor to match the text, which is useful
when the user’s default terminal background is dark.

(3.1)
*/
- (void)
textDidBeginEditing:(NSNotification*)	notification
{
#pragma unused(notification)
	NSText*		fieldEditor = [self currentEditor];
	
	
	if ([fieldEditor isKindOfClass:[NSTextView class]])
	{
		// the field editor is actually an NSTextView*
		NSTextView*		fieldEditorAsView = (NSTextView*)fieldEditor;
		
		
		// Since the text color and background are modified to match user
		// preferences, the insertion point should also be a color that
		// looks reasonable against the custom background.
		[fieldEditorAsView setInsertionPointColor:
							[[CommandLine_PanelController sharedCommandLinePanelController] textColor]];
	}
}// textDidBeginEditing:


@end // CommandLine_TerminalLikeComboBox


#pragma mark -
@implementation CommandLine_PanelController


/*!
Specifies whether or not the input from the Command Line
is sent to all terminal windows.
*/
@synthesize multiTerminalInput = _multiTerminalInput;


/*!
Returns the singleton.

(3.1)
*/
+ (id)
sharedCommandLinePanelController
{
	if (nil == gCommandLine_PanelController)
	{
		gCommandLine_PanelController = [[self.class allocWithZone:NULL] init];
	}
	return gCommandLine_PanelController;
}// sharedCommandLinePanelController


/*!
Designated initializer.

(3.1)
*/
- (instancetype)
init
{
	self = [super initWithWindowNibName:@"CommandLineCocoa"];
	if (nil != self)
	{
		commandLineText = [[NSMutableString alloc] init];
	}
	return self;
}// init


/*!
Destructor.

(3.1)
*/
- (void)
dealloc
{
	[commandLineText release];
	[super dealloc];
}// dealloc


/*!
Responds to a click in the help button.

(3.1)
*/
- (IBAction)
displayHelp:(id)	sender
{
#pragma unused(sender)
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseCommandLine);
}// displayHelp:


/*!
“Types” the command line from this window into the active
session’s terminal window, followed by an appropriate new-line
sequence.

(3.1)
*/
- (IBAction)
sendText:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = SessionFactory_ReturnUserRecentSession();
	
	
	if ((nullptr == session) || (nil == commandLineText))
	{
		// apparently nothing to send or nowhere to send it
		Sound_StandardAlert();
	}
	else
	{
		NSMutableArray*		historyArray = [STATIC_CAST([commandLineField dataSource], CommandLine_HistoryDataSource*) historyArray];
		
		
		[historyArray insertObject:[NSString stringWithString:commandLineText] atIndex:0];
		if (self.multiTerminalInput)
		{
			SessionFactory_ForEachSessionDo(kSessionFactory_SessionFilterFlagAllSessions,
											sendInputSessionOp,
											STATIC_CAST(commandLineText, void*)/* data 1: menu info */,
											0L/* data 2: undefined */, nullptr/* result: unused */);
		}
		else
		{
			Session_UserInputCFString(session, BRIDGE_CAST(commandLineText, CFStringRef));
			Session_SendNewline(session, kSession_EchoCurrentSessionValue);
		}
	}
}// sendText:


/*!
Returns the user’s default terminal text color.

(3.1)
*/
- (NSColor*)
textColor
{
	return [commandLineField textColor];
}// textColor


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(3.1)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	
	
#if 0
	// set the font; though this can look kind of odd
	{
		CFStringRef		fontName = nullptr;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagFontName, sizeof(fontName), &fontName);
		if (kPreferences_ResultOK == prefsResult)
		{
			[commandLineField setFont:[NSFont fontWithName:BRIDGE_CAST(fontName, NSString*)
															size:[[commandLineField font] pointSize]]];
		}
	}
#endif
	// set colors
	{
		RGBColor	colorInfo;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagTerminalColorNormalBackground, sizeof(colorInfo), &colorInfo);
		if (kPreferences_ResultOK == prefsResult)
		{
			NSColor*	colorObject = [NSColor colorWithDeviceRed:((Float32)colorInfo.red / (Float32)RGBCOLOR_INTENSITY_MAX)
										green:((Float32)colorInfo.green / (Float32)RGBCOLOR_INTENSITY_MAX)
										blue:((Float32)colorInfo.blue / (Float32)RGBCOLOR_INTENSITY_MAX)
										alpha:1.0];
			
			
			[commandLineField setDrawsBackground:YES];
			[commandLineField setBackgroundColor:colorObject];
			
			// refuse to set the text color if the background cannot be found, just in case the
			// text color is white or some other color that would suck with the default background
			prefsResult = Preferences_GetData(kPreferences_TagTerminalColorNormalForeground, sizeof(colorInfo), &colorInfo);
			if (kPreferences_ResultOK == prefsResult)
			{
				colorObject = [NSColor colorWithDeviceRed:((Float32)colorInfo.red / (Float32)RGBCOLOR_INTENSITY_MAX)
								green:((Float32)colorInfo.green / (Float32)RGBCOLOR_INTENSITY_MAX)
								blue:((Float32)colorInfo.blue / (Float32)RGBCOLOR_INTENSITY_MAX)
								alpha:1.0];
				[commandLineField setTextColor:colorObject];
			}
		}
	}
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
	return @"CommandLine";
}// windowFrameAutosaveName


@end // CommandLine_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
