/*!	\file ColorUtilities.h
	\brief Use this module to maximize the color capabilites of
	your Mac application.
	
	You will find useful methods for accessing information
	required to use Mac OS Appearance theme-brush-based routines.
	There is also a simple way to “completely” save the current
	graphics port’s state, in a way sufficient for any “damage”
	that a theme brush can cause when drawing.  Finally, there’s
	a cool routine that lets you automatically display the most
	powerful Color Picker dialog available.  A few useful methods
	have been thrown in that will allow you to use “standard”
	patterns without actually accessing QuickDraw global
	variables, very useful in preparation for the opaque
	QuickDraw structures of Mac OS X.
	
	Color Pen State portions of this file are from the OS
	Technologies group at Apple Computer Inc. (source: SDK for
	Appearance 1.0), and are written by Ed Voas with some
	modifications by Kevin Grant.  © 1997-1999 by Apple
	Computer, Inc., all rights reserved.
*/
/*###############################################################

	Interface Library 2.2
	© 1998-2010 by Kevin Grant
	
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

#include "UniversalDefines.h"

#ifndef __COLORUTILITIES__
#define __COLORUTILITIES__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Types

struct ColorPenState
{
#if TARGET_API_MAC_OS8
	// under Classic, parameters are explicitly saved and restored
	Boolean				isColorPort;
	RGBColor			foreColor;
	RGBColor			backColor;
	PenState			pen;
	SInt16				textMode;
	PixPatHandle		pnPixPat;
	PixPatHandle		bkPixPat;
	Pattern				bkPat;
	UInt32				fgColor;
	UInt32				bkColor;
#else
	// under Carbon, the OS does this automatically
	ThemeDrawingState	parameters;
#endif
};



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

Boolean
	ColorUtilities_IsColorGrafPort				(GrafPtr				inPort);

SInt16
	ColorUtilities_ReturnCurrentDepth			(CGrafPtr				inPort);

//@}

//!\name Saving, Restoring and Normalizing QuickDraw Port States
//@{

void
	ColorUtilities_NormalizeColorAndPen			();

void
	ColorUtilities_PreserveColorAndPenState		(ColorPenState*			outStatePtr);

void
	ColorUtilities_RestoreColorAndPenState		(ColorPenState*			inoutStatePtr);

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
