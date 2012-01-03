/*!	\file ColorUtilities.h
	\brief Various routines commonly needed for graphics.
*/
/*###############################################################

	Interface Library 2.4
	© 1998-2012 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <UniversalDefines.h>

#ifndef __COLORUTILITIES__
#define __COLORUTILITIES__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

//!\name Core Graphics Helpers
//@{

CGDeviceColor
	ColorUtilities_CGDeviceColorMake			(RGBColor const&		inColor);

Boolean
	ColorUtilities_CGDeviceGetGray				(CGDirectDisplayID		inDevice,
												 CGDeviceColor const*	inBackgroundPtr,
												 CGDeviceColor*			inoutForegroundNewColorPtr);

RGBColor
	ColorUtilities_QuickDrawColorMake			(CGDeviceColor const&	inColor);

//@}

//!\name Appearance 1.0 Helpers For Theme Brush System Calls
//@{

Boolean
	ColorUtilities_IsColorDevice				(GDHandle				inDevice);

SInt16
	ColorUtilities_ReturnCurrentDepth			(CGrafPtr				inPort);

//@}

//!\name Highlighting
//@{

void
	ColorUtilities_HiliteMode					();

//@}

//!\name Extremely Handy QuickDraw Port Color Manipulators
//@{

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	GetDarkerColors								(RGBColor*				outDarkerForegroundColorOrNull,
												 RGBColor*				outDarkerBackgroundColorOrNull);

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
#define GetInactiveColors	GetLighterColors

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	GetLighterColors							(RGBColor*				outLighterForegroundColorOrNull,
												 RGBColor*				outLighterBackgroundColorOrNull);

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	GetSelectionColors							(RGBColor*				outSelectionForegroundColorOrNull,
												 RGBColor*				outSelectionBackgroundColorOrNull);

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	UseDarkerColors								();

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	UseInactiveColors							();

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	UseInvertedColors							();

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	UseLighterColors							();

// RESULTS ARE BASED ON THE CURRENT FOREGROUND AND BACKGROUND COLORS
void
	UseSelectionColors							();

//@}

//!\name Changing the Pen or Background to a Standard Pattern
//@{

void
	ColorUtilities_SetBlackBackgroundPattern	();

void
	ColorUtilities_SetGrayBackgroundPattern		();

void
	ColorUtilities_SetWhiteBackgroundPattern	();

void
	ColorUtilities_SetBlackPenPattern			();

void
	ColorUtilities_SetGrayPenPattern			();

void
	ColorUtilities_SetWhitePenPattern			();

//@}

//!\name Displaying Standard Get-Color Dialog Boxes
//@{

Boolean
	ColorUtilities_ColorChooserDialogDisplay	(CFStringRef			inPrompt,
												 RGBColor const*		inColor,
												 RGBColor*				outColor,
												 Boolean				inIsModal,
												 UserEventUPP			inUserEventProc,
												 PickerMenuItemInfo*	inEditMenuInfo);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
