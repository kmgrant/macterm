/*!	\file ConnectionData.h
	\brief A structure collecting all data used by sessions.
	
	Direct access is STRONGLY deprecated; the Session API
	abstraction is being set up to avoid direct access.
	Eventually, this structure will go away.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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

#include "UniversalDefines.h"

#ifndef __CONNECTIONDATA__
#define __CONNECTIONDATA__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <ListenerModel.h>

// MacTelnet includes
#include "Local.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalWindow.h"



#pragma mark Constants

enum
{
	MAXFTP = 128,
	MAXKB = 256
};

#define MHOPTS_BASE		37	// Base option for {my,his}opts (Authenticate)
							//  {my,his} opts should only be used for telnet options
							//  in the range starting at MHOPTS_BASE and limited
							//  by MHOPTS_SIZE. This saves memory.
#define MHOPTS_SIZE		2	// Number of options supported in {my,his}opts

#define LINE_MODE_SLC_MAX		30			// must be identical to SLC_MAX in "Parse.h"

#pragma mark Types

struct ConnectionData
{
	explicit ConnectionData	();
	
	TerminalScreenRef	vs;						// virtual screen number (TelnetScreenID); DEPRECATED
												// (instead, use Session_ReturnActiveTerminalWindow() and then
												// TerminalWindow_GetScreenWithFocus() or
												// TerminalWindow_GetScreens())
	WindowRef			window;					// DIRECT ACCESS PROHIBITED; use Session_ReturnActiveWindow()
	CFRetainRelease		alternateTitle;			// DIRECT ACCESS PROHIBITED; use Session_GetWindowUserDefinedTitle()
	
	SInt16				enabled;				// DIRECT ACCESS PROHIBITED; use:
												//		Session_NetworkIsSuspended()
												//		Session_SetNetworkSuspended()
	
	SInt16				bsdel,					// backspace or delete is default 
						eightbit,				// eight bit font displayed (false is seven bit display
						national,				// LU/MP: translation table to use for this connection 
						arrowmap,				// MAT: should we allow the arrow keys to be mapped?? 
						showErrors,				// show ALL errors if this is set 
						keypadmap,				// CCP 2.7: should we have numeric keypad operators work like regular operators? 
						metaKey,				// JMB/SMB:	should option key work as EMACS meta key? 
						Xterm;					// JMB/WNR:	should Xterm sequences be recognized? 
	
	Boolean				pgupdwn;				// DIRECT ACCESS PROHIBITED; use:
												//		Session_PageKeysControlTerminalView()
	
	SInt16				crmap;					// DIRECT ACCESS PROHIBITED; use:
												//		Session_SendNewline()
												//		Session_SetNewlineMode()
	
	SInt16				echo,					// DIRECT ACCESS PROHIBITED; use:
						halfdup;				//		Session_LocalEchoIsEnabled()
												//		Session_LocalEchoIsFullDuplex()
												//		Session_LocalEchoIsHalfDuplex()
												//		Session_SetLocalEchoEnabled()
												//		Session_SetLocalEchoFullDuplex()
												//		Session_SetLocalEchoHalfDuplex()
	
	SInt16				kblen;					// offset into buffer of character to use
	char				kbbuf[MAXKB];			// The keyboard buffer (echo mode)
	
	UInt8				parsedat[450];			// DIRECT ACCESS PROHIBITED; used only in parser (changing)
	SInt16				parseIndex;				// DIRECT ACCESS PROHIBITED; used only in parser (changing)
	
	struct ControlKeys
	{
		ControlKeys (): suspend('\0'), resume('\0'), interrupt('\0'), pad('\0') {}
		
		char				suspend,			// character for scrolling to stop
							resume,				// character for scrolling to go
							interrupt,			// character for “interrupt process”
							pad;				// unused
	} controlKey;
};
typedef ConnectionData*		ConnectionDataPtr;

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
