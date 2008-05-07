/*!	\file Keypads.h
	\brief Handles the floating keypad windoids for control
	keys, VT220 keys, etc.
*/
/*###############################################################

	MacTelnet
		© 1998-2007 by Kevin Grant.
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

#ifndef __KEYPADS__
#define __KEYPADS__



#pragma mark Constants

typedef SInt16	Keypads_WindowType;
enum
{
	kKeypads_WindowTypeControlKeys		= 0,	//!< control keys (^A, ^B, etc.)
	kKeypads_WindowTypeFunctionKeys		= 1,	//!< VT220 function keys (F6 - F20)
	kKeypads_WindowTypeVT220Keys		= 2		//!< VT220 keypad (PF1, PF2, etc.)
};



#pragma mark Public Methods

void
	Keypads_Init					();

void
	Keypads_Done					();

WindowRef
	Keypads_ReturnWindow			(Keypads_WindowType		inFromKeypad);

void
	Keypads_SetKeypadEventTarget	(Keypads_WindowType		inKeypad,
									 EventTargetRef			inTarget);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
