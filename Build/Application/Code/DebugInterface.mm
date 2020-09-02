/*!	\file DebugInterface.mm
	\brief Cocoa implementation of the Debugging panel.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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
#import <Console.h>
#import <SoundSystem.h>
#import <XPCCallPythonClient.objc++.h>

// application includes
#import "Session.h"
#import "SessionFactory.h"
#import "Terminal.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>



#pragma mark Types

/*!
Implements the button actions of the Debug Interface panel.

(Currently any changes to checkboxes do not trigger custom
actions, and UIDebugInterface_Model simply prints log
messages when they are toggled.  If this needs to change
in the future, "UIDebugInterface_ActionHandling" could be
extended to provide more hooks.)
*/
@interface DebugInterface_ViewManager : NSObject <UIDebugInterface_ActionHandling> //{

@end //}

#pragma mark Variables

UIDebugInterface_Model*		gDebugData = [[UIDebugInterface_Model alloc] init];



#pragma mark Public Methods

/*!
Shows the Debugging panel.

(4.0)
*/
void
DebugInterface_Display ()
{
@autoreleasepool {
	DebugInterface_ViewManager*		runner = [[DebugInterface_ViewManager alloc] init];
	NSViewController*				viewController = [UIDebugInterface_ObjC makeViewController:gDebugData runner:runner];
	NSPanel*						panelObject = [NSPanel windowWithContentViewController:viewController];
	NSWindowController*				windowController = nil;
	
	
	panelObject.styleMask = (NSWindowStyleMaskTitled |
								NSWindowStyleMaskClosable |
								NSWindowStyleMaskHUDWindow);
	panelObject.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
	panelObject.title = @"Debug Interface";
	
	windowController = [[NSWindowController alloc] initWithWindow:panelObject];
	windowController.windowFrameAutosaveName = @"Debugging"; // (for backward compatibility, never change this)
	[windowController showWindow:NSApp];
}// @autoreleasepool
}// Display


/*!
Return specified debug setting (can be changed from panel).

(2020.08)
*/
Boolean
DebugInterface_LogsSixelDecoderState ()
{
#ifndef NDEBUG
	return gDebugData.logSixelGraphicsDecoderState;
#else
	return false;
#endif
}// LogsSixelDecoderState


/*!
Return specified debug setting (can be changed from panel).

(2020.08)
*/
Boolean
DebugInterface_LogsSixelInput ()
{
	// for now, log SIXEL input string whenever state is logged
	return DebugInterface_LogsSixelDecoderState();
}// LogsSixelInput


/*!
Return specified debug setting (can be changed from panel).

(2020.08)
*/
Boolean
DebugInterface_LogsTerminalInputChar ()
{
#ifndef NDEBUG
	return gDebugData.logTerminalInputCharacters;
#else
	return false;
#endif
}// LogsTerminalInputChar


/*!
Return specified debug setting (can be changed from panel).

(2020.08)
*/
Boolean
DebugInterface_LogsTeletypewriterState ()
{
#ifndef NDEBUG
	return gDebugData.logPseudoTerminalDeviceSettings;
#else
	return false;
#endif
}// LogsTeletypewriterState


/*!
Return specified debug setting (can be changed from panel).

(2020.08)
*/
Boolean
DebugInterface_LogsTerminalEcho ()
{
#ifndef NDEBUG
	return gDebugData.logTerminalEchoState;
#else
	return false;
#endif
}// LogsTerminalEcho


/*!
Return specified debug setting (can be changed from panel).

(2020.08)
*/
Boolean
DebugInterface_LogsTerminalState ()
{
#ifndef NDEBUG
	return gDebugData.logTerminalState;
#else
	return false;
#endif
}// LogsTerminalState


#pragma mark Internal Methods

#pragma mark -
@implementation DebugInterface_ViewManager


static TerminalToolbar_Window*	gDebugInterface_ToolbarWindow = nil;
static NSWindowController*		gDebugInterface_ToolbarWindowController = nil;


/*!
Designated initializer.

(2020.08)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
	}
	return self;
}// init


#pragma mark UIDebugInterface_ActionHandling


/*!
Prints information to the console on the current state of
the active terminal; for example, whether it is in insert
or replace mode.

(2020.08)
*/
- (void)
dumpStateOfActiveTerminal
{
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
}// dumpStateOfActiveTerminal


/*!
Spawns a new instance of the subprocess that wraps calls to
external Python callbacks.

(2020.08)
*/
- (void)
launchNewCallPythonClient
{
	NSXPCConnection*	connectionObject = [[NSXPCConnection alloc] initWithServiceName:@"net.macterm.helpers.CallPythonClient"];
	NSXPCInterface*		interfaceObject = [NSXPCInterface interfaceWithProtocol:@protocol(XPCCallPythonClient_RemoteObjectInterface)];
	
	
	connectionObject.interruptionHandler = ^{ NSLog(@"call-Python client connection interrupted"); };
	connectionObject.invalidationHandler = ^{ NSLog(@"call-Python client connection invalidated"); };
	connectionObject.remoteObjectInterface = interfaceObject;
	[connectionObject resume];
	
	NSLog(@"created call-Python client object: %@", connectionObject);
	
	id		remoteProxy = [connectionObject remoteObjectProxyWithErrorHandler:^(NSError* error){
												NSLog(@"remote object proxy error: %@", [error localizedDescription]);
											}];
	
	
	if (nil != remoteProxy)
	{
		id< XPCCallPythonClient_RemoteObjectInterface >		asInterface = remoteProxy;
		
		
		[asInterface xpcServiceSendMessage:@"hello!" withReply:^(NSString* aReplyString){
			NSLog(@"MacTerm received response from Python client: %@", aReplyString);
		}];
	}
	// TEMPORARY (INCOMPLETE)
}// launchNewCallPythonClient


/*!
Displays a Cocoa-based terminal toolbar window.

(2020.08)
*/
- (void)
showTestTerminalToolbar
{
	if (nil == gDebugInterface_ToolbarWindow)
	{
		NSRect		newRect = NSMakeRect(0, 0, [[NSScreen mainScreen] visibleFrame].size.width, 0);
		
		
		gDebugInterface_ToolbarWindow = [[TerminalToolbar_Window alloc]
											initWithContentRect:newRect
																styleMask:(NSTitledWindowMask | NSUtilityWindowMask | NSClosableWindowMask |
																			NSMiniaturizableWindowMask)
																backing:NSBackingStoreBuffered
																defer:YES];
	}
	if (nil == gDebugInterface_ToolbarWindowController)
	{
		gDebugInterface_ToolbarWindowController = [[NSWindowController alloc] initWithWindow:gDebugInterface_ToolbarWindow];
	}
	[gDebugInterface_ToolbarWindow makeKeyAndOrderFront:nil];
}// showTestTerminalToolbar


@end // DebugInterface_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
