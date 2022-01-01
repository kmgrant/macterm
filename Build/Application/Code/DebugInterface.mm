/*!	\file DebugInterface.mm
	\brief Cocoa implementation of the Debugging panel.
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

#import "DebugInterface.h"

// Mac includes
@import Cocoa;

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
@interface DebugInterface_ActionHandler : NSObject <UIDebugInterface_ActionHandling> //{

@end //}

#pragma mark Variables

DebugInterface_ActionHandler*	gDebugUIRunner = [[DebugInterface_ActionHandler alloc] init];
UIDebugInterface_Model*			gDebugData = [[UIDebugInterface_Model alloc] initWithRunner:gDebugUIRunner];
Boolean							gDebugInterface_LogsSixelDecoderErrors = false;
Boolean							gDebugInterface_LogsSixelDecoderState = false;
Boolean							gDebugInterface_LogsSixelDecoderSummary = false;
Boolean							gDebugInterface_LogsSixelInput = false;
Boolean							gDebugInterface_LogsTerminalInputChar = false;
Boolean							gDebugInterface_LogsTeletypewriterState = false;
Boolean							gDebugInterface_LogsTerminalEcho = false;
Boolean							gDebugInterface_LogsTerminalState = false;



#pragma mark Public Methods

/*!
Shows the Debugging panel.

(4.0)
*/
void
DebugInterface_Display ()
{
@autoreleasepool {
	static NSWindowController*	windowController = nil;
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		NSViewController*	viewController = [UIDebugInterface_ObjC makeViewController:gDebugData];
		NSPanel*			panelObject = [NSPanel windowWithContentViewController:viewController];
		
		
		panelObject.styleMask = (NSWindowStyleMaskTitled |
									NSWindowStyleMaskClosable |
									NSWindowStyleMaskUtilityWindow |
									NSWindowStyleMaskHUDWindow);
		panelObject.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
		panelObject.title = @"Debug Interface";
		
		windowController = [[NSWindowController alloc] initWithWindow:panelObject];
		windowController.windowFrameAutosaveName = @"Debugging"; // (for backward compatibility, never change this)
	});
	
	[gDebugUIRunner updateSettingCache];
	[windowController showWindow:NSApp];
}// @autoreleasepool
}// Display


#pragma mark Internal Methods

#pragma mark -
@implementation DebugInterface_ActionHandler //{


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
																styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskUtilityWindow | NSWindowStyleMaskClosable |
																			NSWindowStyleMaskMiniaturizable)
																backing:NSBackingStoreBuffered
																defer:YES];
	}
	if (nil == gDebugInterface_ToolbarWindowController)
	{
		gDebugInterface_ToolbarWindowController = [[NSWindowController alloc] initWithWindow:gDebugInterface_ToolbarWindow];
	}
	[gDebugInterface_ToolbarWindow makeKeyAndOrderFront:nil];
}// showTestTerminalToolbar


/*!
Synchronizes cache variables with the UI state so that
queries for debug modes can be as fast as possible
(see header file, where accesses are inlined away).

(2020.11)
*/
- (void)
updateSettingCache
{
	gDebugInterface_LogsSixelDecoderErrors = gDebugData.logSixelGraphicsDecoderErrors; // note: currently the same as decoder state
	gDebugInterface_LogsSixelDecoderState = gDebugData.logSixelGraphicsDecoderState;
	gDebugInterface_LogsSixelDecoderSummary = gDebugData.logSixelGraphicsSummary; // note: currently the same as decoder state
	gDebugInterface_LogsSixelInput = gDebugData.logSixelGraphicsDecoderState; // note: currently the same as decoder state
	gDebugInterface_LogsTerminalInputChar = gDebugData.logTerminalInputCharacters;
	gDebugInterface_LogsTeletypewriterState = gDebugData.logPseudoTerminalDeviceSettings;
	gDebugInterface_LogsTerminalEcho = gDebugData.logTerminalEchoState;
	gDebugInterface_LogsTerminalState = gDebugData.logTerminalState;
}// updateSettingCache


@end //} DebugInterface_ActionHandler

// BELOW IS REQUIRED NEWLINE TO END FILE
