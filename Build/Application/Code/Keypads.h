/*!	\file Keypads.h
	\brief Handles the floating keypad windoids for control
	keys, VT220 keys, etc.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#ifndef __KEYPADS__
#define __KEYPADS__

// Mac includes
#include <Carbon/Carbon.h>
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
	kKeypads_WindowTypeFullScreen		= 3,	//!< controls for Full Screen mode
	kKeypads_WindowTypeArrangeWindow	= 4		//!< for specifying the location of a window
};

#pragma mark Types

#ifdef __OBJC__

/*!
Base class for palettes that interact with sessions.
This is useful to encapsulate the complexity of
locating a target session.
*/
@interface Keypads_PanelController : NSWindowController //{
{
	float	baseFontSize;
}

// initializers
	- (instancetype)
	initWithWindowNibName:(NSString*)_;

// new methods
	- (void)
	sendCharacter:(UInt8)_;
	- (void)
	sendKey:(UInt8)_;

@end //}


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


/*!
Implements the Control Keys palette.  See
"KeypadControlKeysCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_ControlKeysPanelController : Keypads_PanelController //{

// class methods
	+ (id)
	sharedControlKeysPanelController;

// actions
	- (IBAction)
	typeNull:(id)_;
	- (IBAction)
	typeControlA:(id)_;
	- (IBAction)
	typeControlB:(id)_;
	- (IBAction)
	typeControlC:(id)_;
	- (IBAction)
	typeControlD:(id)_;
	- (IBAction)
	typeControlE:(id)_;
	- (IBAction)
	typeControlF:(id)_;
	- (IBAction)
	typeControlG:(id)_;
	- (IBAction)
	typeControlH:(id)_;
	- (IBAction)
	typeControlI:(id)_;
	- (IBAction)
	typeControlJ:(id)_;
	- (IBAction)
	typeControlK:(id)_;
	- (IBAction)
	typeControlL:(id)_;
	- (IBAction)
	typeControlM:(id)_;
	- (IBAction)
	typeControlN:(id)_;
	- (IBAction)
	typeControlO:(id)_;
	- (IBAction)
	typeControlP:(id)_;
	- (IBAction)
	typeControlQ:(id)_;
	- (IBAction)
	typeControlR:(id)_;
	- (IBAction)
	typeControlS:(id)_;
	- (IBAction)
	typeControlT:(id)_;
	- (IBAction)
	typeControlU:(id)_;
	- (IBAction)
	typeControlV:(id)_;
	- (IBAction)
	typeControlW:(id)_;
	- (IBAction)
	typeControlX:(id)_;
	- (IBAction)
	typeControlY:(id)_;
	- (IBAction)
	typeControlZ:(id)_;
	- (IBAction)
	typeControlLeftSquareBracket:(id)_;
	- (IBAction)
	typeControlBackslash:(id)_;
	- (IBAction)
	typeControlRightSquareBracket:(id)_;
	- (IBAction)
	typeControlCaret:(id)_;
	- (IBAction)
	typeControlUnderscore:(id)_;

@end //}


/*!
Implements the multi-layout Function Keys palette.  See
"KeypadFunctionKeysCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_FunctionKeysPanelController : Keypads_PanelController //{
{
	IBOutlet NSPopUpButton*		layoutMenu;
	IBOutlet NSButton*			functionKeyF1;		// may become “PF1” (VT100)
	IBOutlet NSButton*			functionKeyF2;		// may become “PF2” (VT100)
	IBOutlet NSButton*			functionKeyF3;		// may become “PF3” (VT100)
	IBOutlet NSButton*			functionKeyF4;		// may become “PF4” (VT100)
	IBOutlet NSButton*			functionKeyF15;		// may become “?” (help)
	IBOutlet NSButton*			functionKeyF16;		// may become “do”
	NSWindow*					menuChildWindow;
}

// class methods
	+ (id)
	sharedFunctionKeysPanelController;

// accessors
	- (Session_FunctionKeyLayout)
	currentFunctionKeyLayout;

// actions
	- (IBAction)
	performSetFunctionKeyLayoutRxvt:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutVT220:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermX11:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermXFree86:(id)_;
	- (IBAction)
	typeF1:(id)_;
	- (IBAction)
	typeF2:(id)_;
	- (IBAction)
	typeF3:(id)_;
	- (IBAction)
	typeF4:(id)_;
	- (IBAction)
	typeF5:(id)_;
	- (IBAction)
	typeF6:(id)_;
	- (IBAction)
	typeF7:(id)_;
	- (IBAction)
	typeF8:(id)_;
	- (IBAction)
	typeF9:(id)_;
	- (IBAction)
	typeF10:(id)_;
	- (IBAction)
	typeF11:(id)_;
	- (IBAction)
	typeF12:(id)_;
	- (IBAction)
	typeF13:(id)_;
	- (IBAction)
	typeF14:(id)_;
	- (IBAction)
	typeF15:(id)_;
	- (IBAction)
	typeF16:(id)_;
	- (IBAction)
	typeF17:(id)_;
	- (IBAction)
	typeF18:(id)_;
	- (IBAction)
	typeF19:(id)_;
	- (IBAction)
	typeF20:(id)_;
	- (IBAction)
	typeF21:(id)_;
	- (IBAction)
	typeF22:(id)_;
	- (IBAction)
	typeF23:(id)_;
	- (IBAction)
	typeF24:(id)_;
	- (IBAction)
	typeF25:(id)_;
	- (IBAction)
	typeF26:(id)_;
	- (IBAction)
	typeF27:(id)_;
	- (IBAction)
	typeF28:(id)_;
	- (IBAction)
	typeF29:(id)_;
	- (IBAction)
	typeF30:(id)_;
	- (IBAction)
	typeF31:(id)_;
	- (IBAction)
	typeF32:(id)_;
	- (IBAction)
	typeF33:(id)_;
	- (IBAction)
	typeF34:(id)_;
	- (IBAction)
	typeF35:(id)_;
	- (IBAction)
	typeF36:(id)_;
	- (IBAction)
	typeF37:(id)_;
	- (IBAction)
	typeF38:(id)_;
	- (IBAction)
	typeF39:(id)_;
	- (IBAction)
	typeF40:(id)_;
	- (IBAction)
	typeF41:(id)_;
	- (IBAction)
	typeF42:(id)_;
	- (IBAction)
	typeF43:(id)_;
	- (IBAction)
	typeF44:(id)_;
	- (IBAction)
	typeF45:(id)_;
	- (IBAction)
	typeF46:(id)_;
	- (IBAction)
	typeF47:(id)_;
	- (IBAction)
	typeF48:(id)_;

@end //}


/*!
Implements the VT220 Keypad palette.  See
"KeypadVT220KeysCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_VT220KeysPanelController : Keypads_PanelController //{

// class methods
	+ (id)
	sharedVT220KeysPanelController;

// actions
	- (IBAction)
	type0:(id)_;
	- (IBAction)
	type1:(id)_;
	- (IBAction)
	type2:(id)_;
	- (IBAction)
	type3:(id)_;
	- (IBAction)
	type4:(id)_;
	- (IBAction)
	type5:(id)_;
	- (IBAction)
	type6:(id)_;
	- (IBAction)
	type7:(id)_;
	- (IBAction)
	type8:(id)_;
	- (IBAction)
	type9:(id)_;
	- (IBAction)
	typeArrowDown:(id)_;
	- (IBAction)
	typeArrowLeft:(id)_;
	- (IBAction)
	typeArrowRight:(id)_;
	- (IBAction)
	typeArrowUp:(id)_;
	- (IBAction)
	typeComma:(id)_;
	- (IBAction)
	typeDecimalPoint:(id)_;
	- (IBAction)
	typeDelete:(id)_;
	- (IBAction)
	typeEnter:(id)_;
	- (IBAction)
	typeFind:(id)_;
	- (IBAction)
	typeHyphen:(id)_;
	- (IBAction)
	typeInsert:(id)_;
	- (IBAction)
	typePageDown:(id)_;
	- (IBAction)
	typePageUp:(id)_;
	- (IBAction)
	typePF1:(id)_;
	- (IBAction)
	typePF2:(id)_;
	- (IBAction)
	typePF3:(id)_;
	- (IBAction)
	typePF4:(id)_;
	- (IBAction)
	typeSelect:(id)_;

@end //}


/*!
Implements the Full Screen control window, which is
currently only used to disable Full Screen mode.  See
"KeypadFullScreenCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_FullScreenPanelController : NSWindowController //{

// class methods
	+ (id)
	sharedFullScreenPanelController;

// actions
	- (IBAction)
	disableFullScreen:(id)_;

@end //}


/*!
Implements the Arrange Window panel.  See
"KeypadArrangeWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Keypads_ArrangeWindowPanelController : NSWindowController //{

// class methods
	+ (id)
	sharedArrangeWindowPanelController;

// actions
	- (IBAction)
	doneArranging:(id)_;

// new methods
	- (void)
	setOriginToX:(int)_
	andY:(int)_;

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
	Keypads_IsVisible							(Keypads_WindowType			inKeypad);

void
	Keypads_RemoveEventTarget					(Keypads_WindowType			inKeypad,
												 EventTargetRef				inTarget);

void
	Keypads_RemoveResponder						(Keypads_WindowType			inKeypad,
												 NSObject*					inTarget); // NSObject< Keypads_ControlKeyResponder >*

void
	Keypads_SetEventTarget						(Keypads_WindowType			inKeypad,
												 EventTargetRef				inTarget);

void
	Keypads_SetResponder						(Keypads_WindowType			inKeypad,
												 NSObject*					inTarget); // NSObject< Keypads_ControlKeyResponder >*

void
	Keypads_SetVisible							(Keypads_WindowType			inKeypad,
												 Boolean					inIsVisible);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
