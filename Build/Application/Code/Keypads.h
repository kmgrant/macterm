/*!	\file Keypads.h
	\brief Handles the floating keypad windoids for control
	keys, VT220 keys, etc.
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

#ifndef __KEYPADS__
#define __KEYPADS__

// Mac includes
#include <Carbon/Carbon.h>
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif

// MacTelnet includes
#include "Preferences.h"



#pragma mark Constants

typedef SInt16	Keypads_WindowType;
enum
{
	kKeypads_WindowTypeControlKeys		= 0,	//!< control keys (^A, ^B, etc.)
	kKeypads_WindowTypeFunctionKeys		= 1,	//!< VT220 function keys (F6 - F20)
	kKeypads_WindowTypeVT220Keys		= 2,	//!< VT220 keypad (PF1, PF2, etc.)
	kKeypads_WindowTypeFullScreen		= 3,	//!< controls for Full Screen mode
	kKeypads_WindowTypeArrangeWindow	= 4		//!< for specifying the location of a window
};

#pragma mark Types

#ifdef __OBJC__

@interface Keypads_PanelController : NSWindowController
- (void)sendCharacter:(UInt8)inCharacter;
- (void)sendKey:(UInt8)inKey;
@end

/*!
Implements the Control Keys palette.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_ControlKeysPanelController : Keypads_PanelController
+ (id)sharedControlKeysPanelController;
// the following MUST match what is in "KeypadControlKeys.nib"
- (IBAction)typeNull:(id)sender;
- (IBAction)typeControlA:(id)sender;
- (IBAction)typeControlB:(id)sender;
- (IBAction)typeControlC:(id)sender;
- (IBAction)typeControlD:(id)sender;
- (IBAction)typeControlE:(id)sender;
- (IBAction)typeControlF:(id)sender;
- (IBAction)typeControlG:(id)sender;
- (IBAction)typeControlH:(id)sender;
- (IBAction)typeControlI:(id)sender;
- (IBAction)typeControlJ:(id)sender;
- (IBAction)typeControlK:(id)sender;
- (IBAction)typeControlL:(id)sender;
- (IBAction)typeControlM:(id)sender;
- (IBAction)typeControlN:(id)sender;
- (IBAction)typeControlO:(id)sender;
- (IBAction)typeControlP:(id)sender;
- (IBAction)typeControlQ:(id)sender;
- (IBAction)typeControlR:(id)sender;
- (IBAction)typeControlS:(id)sender;
- (IBAction)typeControlT:(id)sender;
- (IBAction)typeControlU:(id)sender;
- (IBAction)typeControlV:(id)sender;
- (IBAction)typeControlW:(id)sender;
- (IBAction)typeControlX:(id)sender;
- (IBAction)typeControlY:(id)sender;
- (IBAction)typeControlZ:(id)sender;
- (IBAction)typeControlLeftSquareBracket:(id)sender;
- (IBAction)typeControlBackslash:(id)sender;
- (IBAction)typeControlRightSquareBracket:(id)sender;
- (IBAction)typeControlTilde:(id)sender;
- (IBAction)typeControlQuestionMark:(id)sender;
@end

/*!
Implements the VT220 Function Keys palette.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_FunctionKeysPanelController : Keypads_PanelController
+ (id)sharedFunctionKeysPanelController;
// the following MUST match what is in "KeypadFunctionKeys.nib"
- (IBAction)typeF6:(id)sender;
- (IBAction)typeF7:(id)sender;
- (IBAction)typeF8:(id)sender;
- (IBAction)typeF9:(id)sender;
- (IBAction)typeF10:(id)sender;
- (IBAction)typeF11:(id)sender;
- (IBAction)typeF12:(id)sender;
- (IBAction)typeF13:(id)sender;
- (IBAction)typeF14:(id)sender;
- (IBAction)typeF15:(id)sender;
- (IBAction)typeF16:(id)sender;
- (IBAction)typeF17:(id)sender;
- (IBAction)typeF18:(id)sender;
- (IBAction)typeF19:(id)sender;
- (IBAction)typeF20:(id)sender;
@end

/*!
Implements the VT220 Keypad palette.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_VT220KeysPanelController : Keypads_PanelController
+ (id)sharedVT220KeysPanelController;
// the following MUST match what is in "KeypadVT220Keys.nib"
- (IBAction)type0:(id)sender;
- (IBAction)type1:(id)sender;
- (IBAction)type2:(id)sender;
- (IBAction)type3:(id)sender;
- (IBAction)type4:(id)sender;
- (IBAction)type5:(id)sender;
- (IBAction)type6:(id)sender;
- (IBAction)type7:(id)sender;
- (IBAction)type8:(id)sender;
- (IBAction)type9:(id)sender;
- (IBAction)typeArrowDown:(id)sender;
- (IBAction)typeArrowLeft:(id)sender;
- (IBAction)typeArrowRight:(id)sender;
- (IBAction)typeArrowUp:(id)sender;
- (IBAction)typeComma:(id)sender;
- (IBAction)typeDecimalPoint:(id)sender;
- (IBAction)typeDelete:(id)sender;
- (IBAction)typeEnter:(id)sender;
- (IBAction)typeFind:(id)sender;
- (IBAction)typeHyphen:(id)sender;
- (IBAction)typeInsert:(id)sender;
- (IBAction)typePageDown:(id)sender;
- (IBAction)typePageUp:(id)sender;
- (IBAction)typePF1:(id)sender;
- (IBAction)typePF2:(id)sender;
- (IBAction)typePF3:(id)sender;
- (IBAction)typePF4:(id)sender;
- (IBAction)typeSelect:(id)sender;
@end

/*!
Implements the Full Screen control window, which is
currently only used to disable Full Screen mode.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_FullScreenPanelController : NSWindowController
+ (id)sharedFullScreenPanelController;
// the following MUST match what is in "KeypadFullScreen.nib"
- (IBAction)disableFullScreen:(id)sender;
@end

/*!
Implements the Arrange Window panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_ArrangeWindowPanelController : NSWindowController
+ (id)sharedArrangeWindowPanelController;
// the following MUST match what is in "KeypadArrangeWindow.nib"
- (IBAction)doneArranging:(id)sender;
- (void)setOriginToX:(int)x andY:(int)y;
@end

#endif



#pragma mark Public Methods

void
	Keypads_SetArrangeWindowPanelBinding		(Preferences_Tag			inWindowBindingOrZero,
												 FourCharCode				inDataTypeForWindowBinding,
												 Preferences_Tag			inScreenBindingOrZero = 0,
												 FourCharCode				inDataTypeForScreenBinding = 0,
												 Preferences_ContextRef		inContextOrNull = nullptr);

Boolean
	Keypads_IsVisible							(Keypads_WindowType			inKeypad);

void
	Keypads_SetEventTarget						(Keypads_WindowType			inKeypad,
												 EventTargetRef				inTarget);

void
	Keypads_SetVisible							(Keypads_WindowType			inKeypad,
												 Boolean					inIsVisible);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
