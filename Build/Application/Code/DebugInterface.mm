/*!	\file DebugInterface.mm
	\brief Cocoa implementation of the Debugging panel.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#import "DebugInterface.h"

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <AutoPool.objc++.h>
#import <Console.h>
#import <SoundSystem.h>

// application includes
#import "PrefsWindow.h"
#import "Session.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalToolbar.objc++.h"
#import "TerminalWindow.h"



#pragma mark Variables
Boolean		gDebugInterface_LogsDeviceState = false;
Boolean		gDebugInterface_LogsTerminalInputChar = false;
Boolean		gDebugInterface_LogsTerminalState = false;



#pragma mark Public Methods

/*!
Shows the Debugging panel.

(4.0)
*/
void
DebugInterface_Display ()
{
	AutoPool		_;
	HIWindowRef		oldActiveWindow = GetUserFocusWindow();
	
	
	[[DebugInterface_PanelController sharedDebugInterfacePanelController] showWindow:NSApp];
	UNUSED_RETURN(OSStatus)SetUserFocusWindow(oldActiveWindow);
}// Display


/*!
Shows the experimental new preferences window (Cocoa based).

(4.1)
*/
void
DebugInterface_DisplayTestPrefsWindow ()
{
	[[[PrefsWindow_Controller sharedPrefsWindowController] window] makeKeyAndOrderFront:nil];
}// DisplayTestPrefsWindow


/*!
Shows the experimental new terminal window (Cocoa based).

(4.0)
*/
void
DebugInterface_DisplayTestTerminal ()
{
	[[[TerminalWindow_Controller sharedTerminalWindowController] window] makeKeyAndOrderFront:nil];
}// DisplayTestTerminal


#pragma mark Internal Methods

@implementation DebugInterface_PanelController


static DebugInterface_PanelController*	gDebugInterface_PanelController = nil;
static TerminalToolbar_Window*			gDebugInterface_ToolbarWindow = nil;
static NSWindowController*				gDebugInterface_ToolbarWindowController = nil;


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedDebugInterfacePanelController
{
	if (nil == gDebugInterface_PanelController)
	{
		gDebugInterface_PanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gDebugInterface_PanelController;
}// sharedDebugInterfacePanelController


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"DebugInterfaceCocoa"];
	return self;
}// init


/*!
Prints information to the console on the current state of
the active terminal; for example, whether it is in insert
or replace mode.

(4.0)
*/
- (IBAction)
dumpStateOfActiveTerminal:(id)	sender
{
#pragma unused(sender)
	SessionRef			activeSession = nullptr;
	TerminalWindowRef	activeTerminalWindow = nullptr;
	TerminalScreenRef	activeScreen = nullptr;
	
	
	Console_WriteLine("");
	Console_WriteHorizontalRule();
	Console_WriteLine("Report on active window:");
	{
		Console_BlockIndent		_;
		
		
		Console_WriteLine("Session");
		{
			Console_BlockIndent		_2;
			
			
			activeSession = SessionFactory_ReturnUserRecentSession();
			
			if (nullptr == activeSession)
			{
				Sound_StandardAlert();
				Console_WriteLine("There is no active session.");
			}
			else
			{
				Console_WriteLine("No debugging information has been defined.");
			}
		}
		Console_WriteLine("Terminal Window");
		{
			Console_BlockIndent		_2;
			
			
			if (nullptr != activeSession)
			{
				activeTerminalWindow = Session_ReturnActiveTerminalWindow(activeSession);
			}
			
			if (nullptr == activeTerminalWindow)
			{
				Sound_StandardAlert();
				Console_WriteLine("The active session has no terminal window.");
			}
			else
			{
				Console_WriteValue("Is hidden (obscured)", TerminalWindow_IsObscured(activeTerminalWindow));
				Console_WriteValue("Is tabbed", TerminalWindow_IsTab(activeTerminalWindow));
			}
		}
		Console_WriteLine("Terminal Screen Buffer");
		{
			Console_BlockIndent		_2;
			
			
			if (nullptr != activeTerminalWindow)
			{
				activeScreen = TerminalWindow_ReturnScreenWithFocus(activeTerminalWindow);
			}
			
			if (nullptr == activeScreen)
			{
				Sound_StandardAlert();
				Console_WriteLine("The active session has no focused terminal screen.");
			}
			else
			{
				Terminal_DebugDumpDetailedSnapshot(activeScreen);
			}
		}
	}
	Console_WriteLine("End of active terminal report.");
	Console_WriteHorizontalRule();
}// dumpStateOfActiveTerminal:


/*!
Displays the Cocoa-based preferences window that is constructed
secretly at startup time.  It is incomplete, so it is only used
for testing incremental additions to the Cocoa-based views.

(4.1)
*/
- (IBAction)
showTestPreferencesWindow:(id)	sender
{
#pragma unused(sender)
	DebugInterface_DisplayTestPrefsWindow();
}// showTestPreferencesWindow:


/*!
Displays a Cocoa-based terminal toolbar window.

(4.0)
*/
- (IBAction)
showTestTerminalToolbar:(id)	sender
{
	if (nil == gDebugInterface_ToolbarWindow)
	{
		NSRect		newRect = NSMakeRect(0, 0, [[NSScreen mainScreen] visibleFrame].size.width, 0);
		NSScreen*	targetScreen = [NSScreen mainScreen];
		
		
		gDebugInterface_ToolbarWindow = [[TerminalToolbar_Window alloc] initWithContentRect:newRect
																							screen:targetScreen];
	}
	if (nil == gDebugInterface_ToolbarWindowController)
	{
		gDebugInterface_ToolbarWindowController = [[NSWindowController alloc] initWithWindow:gDebugInterface_ToolbarWindow];
	}
	[gDebugInterface_ToolbarWindow makeKeyAndOrderFront:sender];
}// showTestTerminalToolbar:


/*!
Displays the Cocoa-based terminal window that is constructed
secretly at startup time.  It is incomplete, so it is only
used for testing incremental additions to the Cocoa-based
terminal view.

(4.0)
*/
- (IBAction)
showTestTerminalWindow:(id)		sender
{
#pragma unused(sender)
	DebugInterface_DisplayTestTerminal();
}// showTestTerminalWindow:


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (BOOL)
logsTerminalInputChar
{
	return gDebugInterface_LogsTerminalInputChar;
}
- (void)
setLogsTerminalInputChar:(BOOL)		aFlag
{
	if (aFlag != gDebugInterface_LogsTerminalInputChar)
	{
		if (aFlag)
		{
			Console_WriteLine("started logging of terminal input characters");
		}
		else
		{
			Console_WriteLine("stopped logging of terminal input characters");
		}
		
		gDebugInterface_LogsTerminalInputChar = aFlag;
	}
}// setLogsTerminalInputChar:


/*!
Accessor.

(4.0)
*/
- (BOOL)
logsTeletypewriterState
{
	return gDebugInterface_LogsDeviceState;
}
- (void)
setLogsTeletypewriterState:(BOOL)	aFlag
{
	if (aFlag != gDebugInterface_LogsDeviceState)
	{
		if (aFlag)
		{
			Console_WriteLine("started logging of pseudo-terminal device configurations");
		}
		else
		{
			Console_WriteLine("stopped logging of pseudo-terminal device configurations");
		}
		
		gDebugInterface_LogsDeviceState = aFlag;
	}
}// setLogsTeletypewriterState:


/*!
Accessor.

(4.0)
*/
- (BOOL)
logsTerminalState
{
	return gDebugInterface_LogsTerminalState;
}
- (void)
setLogsTerminalState:(BOOL)		aFlag
{
	if (aFlag != gDebugInterface_LogsTerminalState)
	{
		if (aFlag)
		{
			Console_WriteLine("started logging of terminal state");
		}
		else
		{
			Console_WriteLine("stopped logging of terminal state");
		}
		
		gDebugInterface_LogsTerminalState = aFlag;
	}
}// setLogsTerminalState:


#pragma mark NSWindowController


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
	return @"Debugging";
}// windowFrameAutosaveName


@end // DebugInterface_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
