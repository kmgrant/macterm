/*!	\file DebugInterface.h
	\brief Implements a utility window for enabling certain
	debugging features.
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

#include <UniversalDefines.h>

#ifndef __DEBUGINTERFACE__
#define __DEBUGINTERFACE__

#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif



#pragma mark Types

#ifdef __OBJC__

/*!
Implements the Debugging panel.  This class must be in sync with
references in "DebugInterfaceCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface DebugInterface_PanelController : NSWindowController
{
}

+ (id)
sharedDebugInterfacePanelController;

- (IBAction)
dumpStateOfActiveTerminal:(id)_;

- (IBAction)
showTestTerminalToolbar:(id)_;

- (IBAction)
showTestTerminalWindow:(id)_;

// accessors

- (BOOL)
logsTerminalInputChar;
- (void)
setLogsTerminalInputChar:(BOOL)_; // binding

- (BOOL)
logsTeletypewriterState;
- (void)
setLogsTeletypewriterState:(BOOL)_; // binding

- (BOOL)
logsTerminalState;
- (void)
setLogsTerminalState:(BOOL)_; // binding

@end

#endif // __OBJC__

#pragma mark Variables

// These are exposed for maximum efficiency.
extern Boolean		gDebugInterface_LogsDeviceState;
extern Boolean		gDebugInterface_LogsTerminalInputChar;
extern Boolean		gDebugInterface_LogsTerminalState;



#pragma mark Public Methods

void
	DebugInterface_Display					();

inline Boolean
	DebugInterface_LogsTerminalInputChar	()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsTerminalInputChar;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsTeletypewriterState	()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsDeviceState;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsTerminalState		()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsTerminalState;
	#else
		return false;
	#endif
	}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
