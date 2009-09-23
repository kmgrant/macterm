/*!	\file DebugInterface.mm
	\brief Cocoa implementation of the Debugging panel.
*/
/*###############################################################

	MacTelnet
		© 1998-2009 by Kevin Grant.
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

// MacTelnet includes
#import "DebugInterface.h"



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
		gDebugInterface_PanelController = [[DebugInterface_PanelController allocWithZone:NULL] init];
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

@end // DebugInterface_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
