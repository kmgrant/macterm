/*!	\file Keypads.h
	\brief Handles the floating keypad windoids for control
	keys, VT220 keys, etc.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSObject;
#endif

// application includes
#include "Preferences.h"
#include "Session.h"



#pragma mark Constants

typedef SInt16	Keypads_WindowType;
enum
{
	kKeypads_WindowTypeControlKeys		= 0,	//!< control keys (^A, ^B, etc.)
	kKeypads_WindowTypeFunctionKeys		= 1,	//!< VT220 function keys (F6 - F20)
	kKeypads_WindowTypeVT220Keys		= 2,	//!< VT220 keypad (PF1, PF2, etc.)
	kKeypads_WindowTypeArrangeWindow	= 3		//!< for specifying the location of a window
};

#pragma mark Types

#ifdef __OBJC__

/*!
Classes conform to this protocol in order to receive
notifications about control keys clicked in the palette.
*/
@protocol Keypads_ControlKeyResponder //{

@optional

	// notification that control keypad has been removed
	- (void)
	controlKeypadHidden;

	// notification that the specified ASCII code has been requested
	- (void)
	controlKeypadSentCharacterCode:(NSNumber*)_;

@end //}

#endif



#pragma mark Public Methods

void
	Keypads_SetArrangeWindowPanelBinding		(Preferences_Tag			inWindowBindingOrZero,
												 FourCharCode				inDataTypeForWindowBinding,
												 Preferences_Tag			inScreenBindingOrZero = 0,
												 FourCharCode				inDataTypeForScreenBinding = 0,
												 Preferences_ContextRef		inContextOrNull = nullptr);

#ifdef __OBJC__
void
	Keypads_SetArrangeWindowPanelBinding		(id							inDidEndTarget,
												 SEL						inDidEndSelector,
												 Preferences_Tag			inWindowBindingOrZero,
												 FourCharCode				inDataTypeForWindowBinding,
												 Preferences_Tag			inScreenBindingOrZero = 0,
												 FourCharCode				inDataTypeForScreenBinding = 0,
												 Preferences_ContextRef		inContextOrNull = nullptr);
#endif

Boolean
	Keypads_IsFunctionKeyLayoutEqualTo			(Session_FunctionKeyLayout	inLayout);

Boolean
	Keypads_IsVisible							(Keypads_WindowType			inKeypad);

void
	Keypads_RemoveResponder						(Keypads_WindowType			inKeypad,
												 NSObject*					inTarget); // NSObject< Keypads_ControlKeyResponder >*

void
	Keypads_SetFunctionKeyLayout				(Session_FunctionKeyLayout	inLayout);

void
	Keypads_SetResponder						(Keypads_WindowType			inKeypad,
												 NSObject*					inTarget); // NSObject< Keypads_ControlKeyResponder >*

void
	Keypads_SetVisible							(Keypads_WindowType			inKeypad,
												 Boolean					inIsVisible);

// BELOW IS REQUIRED NEWLINE TO END FILE
