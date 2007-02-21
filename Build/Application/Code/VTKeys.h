/*!	\file VTKeys.h
	\brief This file contains constants representing
	special keys on the keyboards of real VT terminals.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#ifndef __VTKEYS__
#define __VTKEYS__



#pragma mark Constants

/*!
Internal codes for identifying special keys.  They appear to be
arbitrary, however their values should be above the normal ASCII
range.

IMPORTANT:	The order of these values (and the skipped values)
			is depended upon in code in Session.cp that performs
			key mappings for Session_UserInputKey().
*/
enum
{
	VSF10			= 96,  //!< function key 10; F5  on Mac keyboard
	VSF11			= 97,  //!< function key 11; F6  on Mac keyboard
	VSF12			= 98,  //!< function key 12; F7  on Mac keyboard
	VSF8			= 99,  //!< function key  8; F3  on Mac keyboard
	VSF13			= 100, //!< function key 13; F8  on Mac keyboard
	VSF14			= 101, //!< function key 14; F9  on Mac keyboard
	//				// 102 is unused
	VSF16			= 103, //!< function key 16; F11 on Mac keyboardz
	//				// 104 is unused
	VSF18			= 105, //!< function key 18; F13 on Mac keyboard
	//				// 106 is unused
	VSF19			= 107, //!< function key 19; F14 on Mac keyboard
	//				// 108 is unused
	VSF15			= 109, //!< function key 15; F10 on Mac keyboard
	//				// 110 is unused
	VSF17			= 111, //!< function key 17; F12 on Mac keyboard
	//				// 112 is unused
	VSF20			= 113, //!< function key 20; F15 on Mac keyboard
	VSHELP			= 114, //!< help key
	VSHOME			= 115, //!< home key
	VSPGUP			= 116, //!< page up key
	VSDEL			= 117, //!< forward delete key
	VSF9			= 118, //!< function key  9; F4  on Mac keyboard
	VSEND			= 119, //!< end key
	VSF7			= 120, //!< function key  7; F2  on Mac keyboard
	VSPGDN			= 121, //!< page down key
	VSF6			= 122  //!< function key  6; F1  on Mac keyboard
};

/*!
VT220 Cursor Control keys.
*/
enum
{
	VSUP			= 129, //!< up arrow
	VSDN			= 130, //!< down arrow
	VSRT			= 131, //!< right arrow
	VSLT			= 132  //!< left arrow
};

/*!
VT220 Auxiliary Keypad keys.
*/
enum
{
	VSK0			= 133, //!< keypad 0
	VSK1			= 134, //!< keypad 1
	VSK2			= 135, //!< keypad 2
	VSK3			= 136, //!< keypad 3
	VSK4			= 137, //!< keypad 4
	VSK5			= 138, //!< keypad 5
	VSK6			= 139, //!< keypad 6
	VSK7			= 140, //!< keypad 7
	VSK8			= 141, //!< keypad 8
	VSK9			= 142, //!< keypad 9
	VSKC			= 143, //!< keypad ,
	VSKM			= 144, //!< keypad -
	VSKP			= 145, //!< keypad .
	VSKE			= 146, //!< enter
	VSF1			= 147, //!< PF1 (programmable function key); clear on Mac keypad
	VSF2			= 148, //!< PF2 (programmable function key); = on Mac keypad
	VSF3			= 149, //!< PF3 (programmable function key); / on Mac keypad
	VSF4			= 150  //!< PF4 (programmable function key); * on Mac keypad
};

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
