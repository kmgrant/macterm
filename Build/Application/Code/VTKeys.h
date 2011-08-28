/*!	\file VTKeys.h
	\brief This file contains constants representing
	special keys on the keyboards of real VT terminals.
*/
/*###############################################################

	MacTerm
		© 1998-2011 by Kevin Grant.
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
New key codes for function keys.  They are identified by the
layouts that primarily use them, but note that many layouts
will use a combination of keys (e.g. the F5 code is used by
all layouts despite being credited to XTerm on X11).
*/
enum VTKeys_FKey
{
	// VT100
	kVTKeys_FKeyVT100PF1	= 1,		//!< ^[OP
	kVTKeys_FKeyVT100PF2	= 2,		//!< ^[OQ
	kVTKeys_FKeyVT100PF3	= 3,		//!< ^[OR
	kVTKeys_FKeyVT100PF4	= 4,		//!< ^[OS
	// VT220
	kVTKeys_FKeyVT220F6		= 6,		//!< ^[[17~
	kVTKeys_FKeyVT220F7		= 7,		//!< ^[[18~
	kVTKeys_FKeyVT220F8		= 8,		//!< ^[[19~
	kVTKeys_FKeyVT220F9		= 9,		//!< ^[[20~
	kVTKeys_FKeyVT220F10	= 10,		//!< ^[[21~
	kVTKeys_FKeyVT220F11	= 11,		//!< ^[[23~
	kVTKeys_FKeyVT220F12	= 12,		//!< ^[[24~
	kVTKeys_FKeyVT220F13	= 13,		//!< ^[[25~
	kVTKeys_FKeyVT220F14	= 14,		//!< ^[[26~
	kVTKeys_FKeyVT220F15	= 15,		//!< ^[[28~
	kVTKeys_FKeyVT220F16	= 16,		//!< ^[[29~
	kVTKeys_FKeyVT220F17	= 17,		//!< ^[[31~
	kVTKeys_FKeyVT220F18	= 18,		//!< ^[[32~
	kVTKeys_FKeyVT220F19	= 19,		//!< ^[[33~
	kVTKeys_FKeyVT220F20	= 20,		//!< ^[[34~
	// XTerm (X11)
	kVTKeys_FKeyXTermX11F1	= 21,		//!< ^[[11~
	kVTKeys_FKeyXTermX11F2	= 22,		//!< ^[[12~
	kVTKeys_FKeyXTermX11F3	= 23,		//!< ^[[13~
	kVTKeys_FKeyXTermX11F4	= 24,		//!< ^[[14~
	kVTKeys_FKeyXTermX11F5	= 25,		//!< ^[[15~
	kVTKeys_FKeyXTermX11F13	= 33,		//!< ^[[11;2~
	kVTKeys_FKeyXTermX11F14	= 34,		//!< ^[[12;2~
	kVTKeys_FKeyXTermX11F15	= 35,		//!< ^[[13;2~
	kVTKeys_FKeyXTermX11F16	= 36,		//!< ^[[14;2~
	kVTKeys_FKeyXTermX11F17	= 37,		//!< ^[[15;2~
	kVTKeys_FKeyXTermX11F18	= 38,		//!< ^[[17;2~
	kVTKeys_FKeyXTermX11F19	= 39,		//!< ^[[18;2~
	kVTKeys_FKeyXTermX11F20	= 40,		//!< ^[[19;2~
	kVTKeys_FKeyXTermX11F21	= 41,		//!< ^[[20;2~
	kVTKeys_FKeyXTermX11F22	= 42,		//!< ^[[21;2~
	kVTKeys_FKeyXTermX11F23	= 43,		//!< ^[[23;2~
	kVTKeys_FKeyXTermX11F24	= 44,		//!< ^[[24;2~
	kVTKeys_FKeyXTermX11F25	= 45,		//!< ^[[11;5~
	kVTKeys_FKeyXTermX11F26	= 46,		//!< ^[[12;5~
	kVTKeys_FKeyXTermX11F27	= 47,		//!< ^[[13;5~
	kVTKeys_FKeyXTermX11F28	= 48,		//!< ^[[14;5~
	kVTKeys_FKeyXTermX11F29	= 49,		//!< ^[[15;5~
	kVTKeys_FKeyXTermX11F30	= 50,		//!< ^[[17;5~
	kVTKeys_FKeyXTermX11F31	= 51,		//!< ^[[18;5~
	kVTKeys_FKeyXTermX11F32	= 52,		//!< ^[[19;5~
	kVTKeys_FKeyXTermX11F33	= 53,		//!< ^[[20;5~
	kVTKeys_FKeyXTermX11F34	= 54,		//!< ^[[21;5~
	kVTKeys_FKeyXTermX11F35	= 55,		//!< ^[[23;5~
	kVTKeys_FKeyXTermX11F36	= 56,		//!< ^[[24;5~
	kVTKeys_FKeyXTermX11F37	= 57,		//!< ^[[11;6~
	kVTKeys_FKeyXTermX11F38	= 58,		//!< ^[[12;6~
	kVTKeys_FKeyXTermX11F39	= 59,		//!< ^[[13;6~
	kVTKeys_FKeyXTermX11F40	= 60,		//!< ^[[14;6~
	kVTKeys_FKeyXTermX11F41	= 61,		//!< ^[[15;6~
	kVTKeys_FKeyXTermX11F42	= 62,		//!< ^[[17;6~
	kVTKeys_FKeyXTermX11F43	= 63,		//!< ^[[18;6~
	kVTKeys_FKeyXTermX11F44	= 64,		//!< ^[[19;6~
	kVTKeys_FKeyXTermX11F45	= 65,		//!< ^[[20;6~
	kVTKeys_FKeyXTermX11F46	= 66,		//!< ^[[21;6~
	kVTKeys_FKeyXTermX11F47	= 67,		//!< ^[[23;6~
	kVTKeys_FKeyXTermX11F48	= 68,		//!< ^[[24;6~
	// XTerm (XFree86)
	kVTKeys_FKeyXFree86F13	= 73,		//!< ^[O2P
	kVTKeys_FKeyXFree86F14	= 74,		//!< ^[O2Q
	kVTKeys_FKeyXFree86F15	= 75,		//!< ^[O2R
	kVTKeys_FKeyXFree86F16	= 76,		//!< ^[O2S
	kVTKeys_FKeyXFree86F25	= 85,		//!< ^[O5P
	kVTKeys_FKeyXFree86F26	= 86,		//!< ^[O5Q
	kVTKeys_FKeyXFree86F27	= 87,		//!< ^[O5R
	kVTKeys_FKeyXFree86F28	= 88,		//!< ^[O5S
	kVTKeys_FKeyXFree86F37	= 97,		//!< ^[O6P
	kVTKeys_FKeyXFree86F38	= 98,		//!< ^[O6Q
	kVTKeys_FKeyXFree86F39	= 99,		//!< ^[O6R
	kVTKeys_FKeyXFree86F40	= 100,		//!< ^[O6S
	// rxvt
	kVTKeys_FKeyRxvtF21		= 101,		//!< ^[[23$
	kVTKeys_FKeyRxvtF22		= 102,		//!< ^[[24$
	kVTKeys_FKeyRxvtF23		= 103,		//!< ^[[11^
	kVTKeys_FKeyRxvtF24		= 104,		//!< ^[[12^
	kVTKeys_FKeyRxvtF25		= 105,		//!< ^[[13^
	kVTKeys_FKeyRxvtF26		= 106,		//!< ^[[14^
	kVTKeys_FKeyRxvtF27		= 107,		//!< ^[[15^
	kVTKeys_FKeyRxvtF28		= 108,		//!< ^[[17^
	kVTKeys_FKeyRxvtF29		= 109,		//!< ^[[18^
	kVTKeys_FKeyRxvtF30		= 110,		//!< ^[[19^
	kVTKeys_FKeyRxvtF31		= 111,		//!< ^[[20^
	kVTKeys_FKeyRxvtF32		= 112,		//!< ^[[21^
	kVTKeys_FKeyRxvtF33		= 113,		//!< ^[[23^
	kVTKeys_FKeyRxvtF34		= 114,		//!< ^[[24^
	kVTKeys_FKeyRxvtF35		= 115,		//!< ^[[25^
	kVTKeys_FKeyRxvtF36		= 116,		//!< ^[[26^
	kVTKeys_FKeyRxvtF37		= 117,		//!< ^[[28^
	kVTKeys_FKeyRxvtF38		= 118,		//!< ^[[29^
	kVTKeys_FKeyRxvtF39		= 119,		//!< ^[[31^
	kVTKeys_FKeyRxvtF40		= 120,		//!< ^[[32^
	kVTKeys_FKeyRxvtF41		= 121,		//!< ^[[33^
	kVTKeys_FKeyRxvtF42		= 122,		//!< ^[[34^
	kVTKeys_FKeyRxvtF43		= 123,		//!< ^[[23@
	kVTKeys_FKeyRxvtF44		= 124		//!< ^[[24@
};

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
	VSF16_220DO		= 103, //!< function key 16; F11 on Mac keyboard (“do” key on VT220)
	//				// 104 is unused
	VSF18			= 105, //!< function key 18; F13 on Mac keyboard
	//				// 106 is unused
	VSF19			= 107, //!< function key 19; F14 on Mac keyboard
	//				// 108 is unused
	VSF15_220HELP	= 109, //!< function key 15; F10 on Mac keyboard (“help” key on VT220)
	//				// 110 is unused
	VSF17			= 111, //!< function key 17; F12 on Mac keyboard
	//				// 112 is unused
	VSF20			= 113, //!< function key 20; F15 on Mac keyboard
	VSHELP_220FIND	= 114, //!< help key (“find” key on VT220)
	VSHOME_220INS	= 115, //!< home key (“insert” key on VT220)
	VSPGUP_220DEL	= 116, //!< page up key (“delete” key on VT220)
	VSDEL_220SEL	= 117, //!< forward delete key (“select” key on VT220)
	VSF9			= 118, //!< function key  9; F4  on Mac keyboard
	VSEND_220PGUP	= 119, //!< end key (“page up” key on VT220)
	VSF7			= 120, //!< function key  7; F2  on Mac keyboard
	VSPGDN_220PGDN	= 121, //!< page down key (even on VT220)
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
