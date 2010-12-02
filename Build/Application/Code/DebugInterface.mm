/*!	\file DebugInterface.mm
	\brief Cocoa implementation of the Debugging panel.
*/
/*###############################################################

	MacTelnet
		© 1998-2010 by Kevin Grant.
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <AutoPool.objc++.h>
#import <Console.h>
#import <SoundSystem.h>

// MacTelnet includes
#import "DebugInterface.h"
#import "Session.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "TerminalWindow.h"



#pragma mark Variables
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
	(OSStatus)SetUserFocusWindow(oldActiveWindow);
}// Display


#pragma mark Internal Methods

@implementation DebugInterface_PanelController

static DebugInterface_PanelController*	gDebugInterface_PanelController = nil;
+ (id)
sharedDebugInterfacePanelController
{
	if (nil == gDebugInterface_PanelController)
	{
		gDebugInterface_PanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gDebugInterface_PanelController;
}

- (id)
init
{
	self = [super initWithWindowNibName:@"DebugInterfaceCocoa"];
	return self;
}


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
			
			
			activeSession = SessionFactory_ReturnUserFocusSession();
			
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
Accessor.

(4.0)
*/
- (BOOL)
logsTerminalInputChar
{
	return gDebugInterface_LogsTerminalInputChar;
}
- (void)
setLogsTerminalInputChar:(BOOL)		flag
{
	if (flag != gDebugInterface_LogsTerminalInputChar)
	{
		if (flag) Console_WriteLine("started logging of terminal input characters");
		else Console_WriteLine("stopped logging of terminal input characters");
		
		gDebugInterface_LogsTerminalInputChar = flag;
	}
}// setLogsTerminalInputChar:


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
setLogsTerminalState:(BOOL)		flag
{
	if (flag != gDebugInterface_LogsTerminalState)
	{
		if (flag) Console_WriteLine("started logging of terminal state");
		else Console_WriteLine("stopped logging of terminal state");
		
		gDebugInterface_LogsTerminalState = flag;
	}
}// setLogsTerminalState:


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
	[self->testTerminalWindow makeKeyAndOrderFront:sender];
}// showTestTerminalWindow:


#pragma mark NSWindowController


- (void)
windowDidLoad
{
	assert(nil != testTerminalContentView);
	assert(nil != testTerminalPaddingView);
	assert(nil != testTerminalBackgroundView);
	assert(nil != testTerminalWindow);
	
	Preferences_ContextRef	terminalConfig = Preferences_NewContext(Quills::Prefs::TERMINAL);
	Preferences_ContextRef	translationConfig = Preferences_NewContext(Quills::Prefs::TRANSLATION);
	
	
	// could customize the new contexts above to initialize settings;
	// currently, this is not done
	{
		TerminalScreenRef		buffer = nullptr;
		Terminal_Result			bufferResult = Terminal_NewScreen(terminalConfig, translationConfig, &buffer);
		
		
		if (kTerminal_ResultOK != bufferResult)
		{
			Console_WriteValue("error creating test terminal screen buffer", bufferResult);
		}
		else
		{
			TerminalViewRef		view = TerminalView_NewNSViewBased(testTerminalContentView, testTerminalPaddingView,
																	testTerminalBackgroundView, buffer, nullptr/* format */);
			
			
			if (nullptr == view)
			{
				Console_WriteLine("error creating test terminal view!");
			}
			else
			{
				// write some text in various styles to the screen (happens to be a
				// copy of what the sample view does); this will help with testing
				// the new Cocoa-based renderer as it is implemented
				Terminal_EmulatorProcessCString(buffer,
												"\033[2J\033[H"); // clear screen, home cursor (assumes VT100)
				Terminal_EmulatorProcessCString(buffer,
												"sel norm \033[1mbold\033[0m \033[5mblink\033[0m \033[3mital\033[0m \033[7minv\033[0m \033[4munder\033[0m");
				// the range selected here should be as long as the length of the word “sel” above
				TerminalView_SelectVirtualRange(view, std::make_pair(std::make_pair(0, 0), std::make_pair(3, 1)/* exclusive end */));
			}
		}
	}
	
	Preferences_ReleaseContext(&terminalConfig);
	Preferences_ReleaseContext(&translationConfig);
}// windowDidLoad

@end // DebugInterface_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
