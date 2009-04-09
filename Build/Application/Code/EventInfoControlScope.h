/*!	\file EventInfoControlScope.h
	\brief Data structures used as contexts for main event loop
	events specific to one user interface element.
	
	Defined in a separate file to better-organize the data and
	to reduce the number of files depending on one header.
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

#include "UniversalDefines.h"

#ifndef __EVENTINFOCONTROLSCOPE__
#define __EVENTINFOCONTROLSCOPE__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

/*!
Data structure providing the context for event handlers
notified about "kApplicationControlEventOccurrence_KeyPress".
*/
struct EventInfoControlScope_KeyPress
{
	ControlRef			control;			//!< which control is focused
	SInt16				characterCode;		//!< code uniquifying the character corresponding to the key pressed
	SInt16				characterCode2;		//!< if nonzero, the key press represents a sequence of two characters to send
	SInt16				virtualKeyCode;		//!< code uniquifying the key pressed
	Boolean				commandDown;		//!< the state of the Command modifier key
	Boolean				controlDown;		//!< the state of the Control modifier key
	Boolean				optionDown;			//!< the state of the Option modifier key
	Boolean				shiftDown;			//!< the state of the Shift modifier key
};
typedef EventInfoControlScope_KeyPress*		EventInfoControlScope_KeyPressPtr;

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
