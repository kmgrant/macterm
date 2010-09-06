/*###############################################################

	ColorUtilities.cp
	
	Color Pen State portions of this file are from the
	OS Technologies group at Apple Computer Inc. (source: SDK
	for Appearance 1.0), and are written by Ed Voas.  Copyright
	© 1997 by Apple Computer, Inc., all rights reserved.
	
	Interface Library 2.0
	© 1998-2006 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
    This program is distributed in the hope that it will be
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

// standard-C includes
#include <climits>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <ColorUtilities.h>
#include <Releases.h>



#pragma mark Public Methods

/*!
Returns a CGDeviceColor equivalent to the given
QuickDraw RGBColor values.

(3.1)
*/
CGDeviceColor
ColorUtilities_CGDeviceColorMake	(RGBColor const&	inColor)
{
	CGDeviceColor	result;
	Float32			fullIntensityFraction = 0.0;
	
	
	// a Core Graphics color uses fractions of the total intensity,
	// instead of absolute values, to represent each element of color
	fullIntensityFraction = inColor.red;
	fullIntensityFraction /= STATIC_CAST(RGBCOLOR_INTENSITY_MAX, Float32);
	result.red = fullIntensityFraction;
	fullIntensityFraction = inColor.green;
	fullIntensityFraction /= STATIC_CAST(RGBCOLOR_INTENSITY_MAX, Float32);
	result.green = fullIntensityFraction;
	fullIntensityFraction = inColor.blue;
	fullIntensityFraction /= STATIC_CAST(RGBCOLOR_INTENSITY_MAX, Float32);
	result.blue = fullIntensityFraction;
	
	return result;
}// CGDeviceColorMake


/*!
This universal routine allows you to use the most
powerful version of the Mac OS color picker that
is available.  There is no support, however, for
special callback routines when using this method.

The dialog assumes the application is not ColorSync™
aware, detachs the dialog body from the choices,
uses a system dialog, and is centered on the main
screen.

Since the dialog prefers to be movable, you should
(but do not have to) provide a routine that can
respond to update events for your application windows,
should the user cause a region of a background window
to require updating.  If you do not provide an update
routine, the dialog is not made movable.

The frontmost window is automatically deactivated (if
possible, that is, if it has an Appearance-based
embedding hierarchy) before the dialog appears, and
is restored to its normal state before this routine
returns.

If the user chooses a new color, true is returned;
otherwise, false is returned.

(1.0)
*/
Boolean
ColorUtilities_ColorChooserDialogDisplay	(CFStringRef			inPrompt,
											 const RGBColor*		inColor,
											 RGBColor*				outColor,
											 Boolean				inIsModal,
											 UserEventUPP			inUserEventProc,
											 PickerMenuItemInfo*	inEditMenuInfo)
{
	Boolean		result = false;
	UInt8		majorRev = 0,
				minorRev = 0;
	Point		dialogLocation;
	OSStatus	error = noErr;
	UInt32		kColorUtilitiesColorPickerFlags =	((inUserEventProc != nullptr) ? kColorPickerDialogIsMoveable : 0) |
													((inIsModal) ? kColorPickerDialogIsModal : 0) |
													kColorPickerCanModifyPalette |
													kColorPickerCanAnimatePalette;
	Str255		promptPascalString;
	
	
	// convert CFString to Pascal string, due to limitations of older APIs
	if (true != CFStringGetPascalString(inPrompt, promptPascalString, sizeof(promptPascalString),
										kCFStringEncodingMacRoman))
	{
		// failed; so, do not use a prompt
		PLstrcpy(promptPascalString, "\p");
	}
	
	// figure out the highest version of the Color Picker API that is available
	{
		long		version = 0L;
		
		
		error = Gestalt(gestaltColorPickerVersion, &version);
		majorRev = Releases_ReturnMajorRevisionForVersion(version);
		minorRev = Releases_ReturnMinorRevisionForVersion(version);
	}
	
	// set the dialog location to 0, 0 so the Color Picker is centered on the main screen
	SetPt(&dialogLocation, 0, 0);
	
	// the Color Picker 2.1 dialog is the best: use it if it's available
	if (((majorRev == 0x02) && (minorRev >= 0x01)) || (majorRev > 0x02))
	{
		// Color Picker 2.1 or later
		NColorPickerInfo	info;
		
		
		info.theColor.profile = nullptr;
		info.theColor.color.rgb.red = inColor->red;
		info.theColor.color.rgb.green = inColor->green;
		info.theColor.color.rgb.blue = inColor->blue;
		info.dstProfile = nullptr;
		info.flags = kColorUtilitiesColorPickerFlags;
		info.placeWhere = kCenterOnMainScreen;
		info.dialogOrigin = dialogLocation;
		info.pickerType = 0L;
		info.eventProc = inUserEventProc;
		info.colorProc = nullptr;
		info.colorProcData = 0L;
		PLstrcpy(info.prompt, promptPascalString);
		info.mInfo = *inEditMenuInfo;
		info.newColorChosen = false;
		info.reserved = 0;
		
		error = NPickColor(&info);
		result = ((error == noErr) ? info.newColorChosen : false);
		if (result)
		{
			// return the color that the user selected
			outColor->red = info.theColor.color.rgb.red;
			outColor->green = info.theColor.color.rgb.green;
			outColor->blue = info.theColor.color.rgb.blue;
		}
	}
	else if (majorRev == 0x02)
	{
		// Color Picker 2.0 or earlier
		ColorPickerInfo		info;
		
		
		info.theColor.profile = nullptr;
		info.theColor.color.rgb.red = inColor->red;
		info.theColor.color.rgb.green = inColor->green;
		info.theColor.color.rgb.blue = inColor->blue;
		info.dstProfile = nullptr;
		info.flags = kColorUtilitiesColorPickerFlags;
		info.placeWhere = kCenterOnMainScreen;
		info.dialogOrigin = dialogLocation;
		info.pickerType = 0L;
		info.eventProc = inUserEventProc;
		info.colorProc = nullptr;
		info.colorProcData = 0L;
		PLstrcpy(info.prompt, promptPascalString);
		info.mInfo = *inEditMenuInfo;
		info.newColorChosen = false;
		info.filler = 0;
		
		error = PickColor(&info);
		result = ((error == noErr) ? info.newColorChosen : false);
		if (result)
		{
			// return the color that the user selected
			outColor->red = info.theColor.color.rgb.red;
			outColor->green = info.theColor.color.rgb.green;
			outColor->blue = info.theColor.color.rgb.blue;
		}
	}
	
	if ((majorRev < 0x02) || (error != noErr))
	{
		// no Color Picker extension, or there was a problem using the cool, new stuff?
		// Well, okay, use the GetColor() call, which works even without the Color Picker extension
		result = GetColor(dialogLocation, promptPascalString, inColor, outColor);
	}
	
	return result;
}// ColorChooserDialogDisplay


/*!
Returns “darker” versions of the current foreground
and background colors of the current graphics port -
except if the background color is very close to black,
then a brighter version of the color is returned.
You can pass nullptr in place of either parameter if
you are not interested in one of the colors.

You might use this to show that a range of colored
text is highlighted.

(1.3)
*/
void
GetDarkerColors		(RGBColor*		outDarkerForegroundColorOrNull,
					 RGBColor*		outDarkerBackgroundColorOrNull)
{
	RGBColor	black;
	
	
	if (outDarkerForegroundColorOrNull != nullptr)
	{
		// set up foreground color
		black.red = black.green = black.blue = 0;
		GetForeColor(outDarkerForegroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &black, outDarkerForegroundColorOrNull);
		black = *outDarkerForegroundColorOrNull;
		GetForeColor(outDarkerForegroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &black, outDarkerForegroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &black, outDarkerForegroundColorOrNull);
	}
	
	if (outDarkerBackgroundColorOrNull != nullptr)
	{
		// set up background color
		black.red = black.green = black.blue = 0;
		GetBackColor(outDarkerBackgroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &black, outDarkerBackgroundColorOrNull);
		black = *outDarkerBackgroundColorOrNull;
		GetBackColor(outDarkerBackgroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &black, outDarkerBackgroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &black, outDarkerBackgroundColorOrNull);
	}
}// GetDarkerColors


/*!
Returns “lighter” versions of the current foreground
and background colors of the current graphics port.
You can pass nullptr in place of either parameter if
you are not interested in one of the colors.

You might use this to represent the inactive state of
something.

(1.3)
*/
void
GetLighterColors	(RGBColor*		outLighterForegroundColorOrNull,
					 RGBColor*		outLighterBackgroundColorOrNull)
{
	RGBColor	white;
	
	
	if (outLighterForegroundColorOrNull != nullptr)
	{
		// set up foreground color
		white.red = white.green = white.blue = RGBCOLOR_INTENSITY_MAX;
		GetForeColor(outLighterForegroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &white, outLighterForegroundColorOrNull);
		white = *outLighterForegroundColorOrNull;
		GetForeColor(outLighterForegroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &white, outLighterForegroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &white, outLighterForegroundColorOrNull);
	}
	
	if (outLighterBackgroundColorOrNull != nullptr)
	{
		// set up background color
		white.red = white.green = white.blue = RGBCOLOR_INTENSITY_MAX;
		GetBackColor(outLighterBackgroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &white, outLighterBackgroundColorOrNull);
		white = *outLighterBackgroundColorOrNull;
		GetBackColor(outLighterBackgroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &white, outLighterBackgroundColorOrNull);
		(Boolean)GetGray(GetMainDevice(), &white, outLighterBackgroundColorOrNull);
	}
}// GetLighterColors


/*!
Returns “selected” versions of the current foreground
and background colors of the current graphics port.
You can pass nullptr in place of either parameter if
you are not interested in one of the colors.

Since traditional selection schemes assume that white
is the background color, this routine examines the
current background color and will only use the system-
wide highlight color if the background is approximately
white.  Otherwise, the actual colors are used to find
an appropriate selection color.

(3.0)
*/
void
GetSelectionColors	(RGBColor*		outLighterForegroundColorOrNull,
					 RGBColor*		outLighterBackgroundColorOrNull)
{
	RGBColor		foreground;
	RGBColor		background;
	SInt16 const	kTolerance = 2048; // color intensities can vary by this much and still be considered black or white
	
	
	GetForeColor(&foreground);
	GetBackColor(&background);
	if ((foreground.red < kTolerance) && (foreground.green < kTolerance) && (foreground.blue < kTolerance) &&
			(background.red > (RGBCOLOR_INTENSITY_MAX - kTolerance)) &&
			(background.green > (RGBCOLOR_INTENSITY_MAX - kTolerance)) &&
			(background.blue > (RGBCOLOR_INTENSITY_MAX - kTolerance)))
	{
		// approximately monochromatic; this is what the highlight color
		// is “traditionally” intended for, so it should look okay; use it
		if (outLighterForegroundColorOrNull != nullptr) *outLighterForegroundColorOrNull = foreground;
		if (outLighterForegroundColorOrNull != nullptr) LMGetHiliteRGB(outLighterBackgroundColorOrNull);
	}
	else if ((background.red < kTolerance) && (background.green < kTolerance) && (background.blue < kTolerance))
	{
		// very dark background; therefore, darkening the background will
		// not make it clear where the selection is; brighten it instead
		GetLighterColors(outLighterForegroundColorOrNull, outLighterBackgroundColorOrNull);
	}
	else
	{
		// typical case for a terminal window; some combination of colors,
		// just find darker colors to show highlighted text in this case
		GetDarkerColors(outLighterForegroundColorOrNull, outLighterBackgroundColorOrNull);
	}
}// GetSelectionColors


/*!
Tells QuickDraw to use the highlighting
color instead of inversion for the next
operation.

(1.0)
*/
void
ColorUtilities_HiliteMode ()
{
	LMSetHiliteMode(LMGetHiliteMode() & 0x7F);
}// HiliteMode


/*!
Determines if the specified device uses Color QuickDraw.

(1.0)
*/
Boolean
ColorUtilities_IsColorDevice	(GDHandle	inDevice)
{
	Boolean		result = TestDeviceAttribute(inDevice, gdDevType);
	
	
	return result;
}// IsColorDevice


/*!
Determines if the specified port uses Color QuickDraw.
Once Mac OS 8 code is abandoned completely, this check
will become obsolete.

(1.0)
*/
Boolean
ColorUtilities_IsColorGrafPort	(GrafPtr	UNUSED_ARGUMENT(inPort))
{
	Boolean		result = true;
	
	
#if TARGET_API_MAC_OS8
	result = (((inPort)->portBits.rowBytes & 0xC000) == 0xC000);
#endif
	return result;
}// IsColorGrafPort


/*!
Standardizes a graphics port for use with theme
brushes under Appearance 1.0 (that is, slightly
more than what PenNormal() does).

© 1997-1999 by Apple Computer, Inc.

(1.0)
*/
void
ColorUtilities_NormalizeColorAndPen	()
{
	RGBColor	black,
				white;
	
	
	black.red = black.green = black.blue = 0x0000;
	white.red = white.green = white.blue = RGBCOLOR_INTENSITY_MAX;
	
	RGBForeColor(&black);
	RGBBackColor(&white);
	PenNormal();
	ColorUtilities_SetWhiteBackgroundPattern();
	TextMode(srcOr);
}// NormalizeColorAndPen


/*!
To save all pen state information that might
get affected by using a theme brush under
Appearance 1.0, invoke this method.

© 1997-1999 by Apple Computer, Inc.

(1.0)
*/
void
ColorUtilities_PreserveColorAndPenState		(ColorPenState*		outStatePtr)
{
#if TARGET_API_MAC_CARBON
	// Carbon has an API for this
	(OSStatus)GetThemeDrawingState(&outStatePtr->parameters);
#else
	// under Classic, Apple has specified the parameters that need saving
	CGrafPtr	currentPort = nullptr;
	GDHandle	currentDevice = nullptr;
	
	
	GetGWorld(&currentPort, &currentDevice);
	
	outStatePtr->pnPixPat = nullptr;
	outStatePtr->bkPixPat = nullptr;
	
	// these are non-color graphics port features
	outStatePtr->bkPat = currentPort->bkPat;
	outStatePtr->bkColor = currentPort->bkColor;
	outStatePtr->fgColor = currentPort->fgColor;
	outStatePtr->isColorPort = ColorUtilities_IsColorGrafPort(currentPort);
	
	if (outStatePtr->isColorPort)
	{
		GetForeColor(&outStatePtr->foreColor);
		GetBackColor(&outStatePtr->backColor);
		
		// If the pen pattern is not an old-style pattern,
		// copy the handle.  If it is an old style pattern,
		// GetPenState(), below, will save the right thing.
		if ((**currentPort->pnPixPat).patType != 0)
		{
			outStatePtr->pnPixPat = currentPort->pnPixPat;
		}
		
		// If the pen pattern is not an old style pattern,
		// copy the handle.  Otherwise, get the old pattern
		// into "bkPat", for restoring that way.
		if ((**currentPort->bkPixPat).patType != 0)
		{
			outStatePtr->bkPixPat = currentPort->bkPixPat;
		}
		else
		{
			outStatePtr->bkPat = *(PatPtr)(*(**(currentPort)->bkPixPat).patData);
		}
	}
	
	GetPenState(&outStatePtr->pen);
	outStatePtr->textMode = GetPortTextMode(currentPort);
#endif
}// PreserveColorAndPenState


/*!
To restore all pen state information that was previously
saved with ColorUtilities_GetColorAndPenState(), invoke
this method.

© 1997-1999 by Apple Computer, Inc.

(1.0)
*/
void
ColorUtilities_RestoreColorAndPenState	(ColorPenState*		inoutStatePtr)
{
#if TARGET_API_MAC_CARBON
	// Carbon has an API for this
	(OSStatus)SetThemeDrawingState(inoutStatePtr->parameters, true/* dispose now */);
#else
	// under Classic, Apple has specified the parameters that need restoring
	SetPenState(&inoutStatePtr->pen);
	if (inoutStatePtr->isColorPort)
	{
		RGBForeColor(&inoutStatePtr->foreColor);
		RGBBackColor(&inoutStatePtr->backColor);
		if (inoutStatePtr->pnPixPat != nullptr)
		{
			PenPixPat(inoutStatePtr->pnPixPat);
		}
		if (inoutStatePtr->bkPixPat != nullptr)
		{
			BackPixPat(inoutStatePtr->bkPixPat);
		}
	}
	else
	{
		BackPat(&inoutStatePtr->bkPat);
		ForeColor(inoutStatePtr->fgColor);
		BackColor(inoutStatePtr->bkColor);
	}
	
	TextMode(inoutStatePtr->textMode);
#endif
}// RestoreColorAndPenState


/*!
Determines the color depth of the specified port.

(1.0)
*/
SInt16
ColorUtilities_ReturnCurrentDepth	(CGrafPtr	inPort)
{
	SInt16		result = 1;
	
	
#if TARGET_API_MAC_OS8
	result = (ColorUtilities_IsColorGrafPort((GrafPtr)inPort) ? (**(inPort)->portPixMap).pixelSize : 1);
#else
	//result = (**GetPortPixMap(inPort)).pixelSize;
	result = GetPixDepth(GetPortPixMap(inPort));
#endif
	return result;
}// ReturnCurrentDepth


/*!
Calls BackPat() with an all-black pattern.

(1.0)
*/
void
ColorUtilities_SetBlackBackgroundPattern ()
{
	Pattern		blackPat = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
	
	
	BackPat(&blackPat);
}// SetBlackBackgroundPattern


/*!
Calls PenPat() with an all-black pattern.

(1.0)
*/
void
ColorUtilities_SetBlackPenPattern ()
{
	Pattern		blackPat = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
	
	
	PenPat(&blackPat);
}// SetBlackPenPattern


/*!
Calls BackPat() with a checkerboard pattern.

(1.0)
*/
void
ColorUtilities_SetGrayBackgroundPattern ()
{
	Pattern		grayPat = { { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA } };
	
	
	BackPat(&grayPat);
}// SetGrayBackgroundPattern


/*!
Calls PenPat() with a checkerboard pattern.

(1.0)
*/
void
ColorUtilities_SetGrayPenPattern ()
{
	Pattern		grayPat = { { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA } };
	
	
	PenPat(&grayPat);
}// SetGrayPenPattern


/*!
Calls BackPat() with an all-white pattern.

(1.0)
*/
void
ColorUtilities_SetWhiteBackgroundPattern ()
{
	Pattern		whitePat = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
	
	
	BackPat(&whitePat);
}// SetWhiteBackgroundPattern


/*!
Calls PenPat() with an all-white pattern.

(1.0)
*/
void
ColorUtilities_SetWhitePenPattern ()
{
	Pattern		whitePat = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
	
	
	PenPat(&whitePat);
}// SetWhitePenPattern


/*!
Uses GetDarkerColors() to determine the
“darkened” versions of the current foreground
and background colors of the current graphics
port, and makes the darkened versions the
current foreground and background colors.

Do not use this routine to represent a selected
state; for that, call UseSelectionColors().

(1.3)
*/
void
UseDarkerColors ()
{
	RGBColor	foreground;
	RGBColor	background;
	
	
	GetDarkerColors(&foreground, &background);
	RGBForeColor(&foreground);
	RGBBackColor(&background);
}// UseDarkerColors


/*!
Uses GetInactiveColors() to determine the “dimmed”
versions of the current foreground and background
colors of the current graphics port, and makes the
inactive versions the current foreground and
background colors.

This routine might be used inside a drawing procedure
for a custom control, to represent an inactive state.

(1.3)
*/
void
UseInactiveColors ()
{
	RGBColor	foreground;
	RGBColor	background;
	
	
	GetInactiveColors(&foreground, &background);
	RGBForeColor(&foreground);
	RGBBackColor(&background);
}// UseInactiveColors


/*!
Reverses the current foreground and background
colors of the current graphics port.

This routine might be used inside a drawing
procedure for a custom control, to represent
a selected state.

See ColorUtilities_UseInvertedColors().

(1.3)
*/
void
UseInvertedColors ()
{
	RGBColor	foreground;
	RGBColor	background;
	
	
	GetForeColor(&foreground);
	GetBackColor(&background);
	RGBForeColor(&background);
	RGBBackColor(&foreground);
}// UseInvertedColors


/*!
Uses GetLighterColors() to determine the “lighter”
versions of the current foreground and background
colors of the current graphics port, and makes the
lighter versions the current foreground and
background colors.

Do not use this routine to represent an inactive
state; for that, call UseInactiveColors().

(1.3)
*/
void
UseLighterColors ()
{
	RGBColor	foreground;
	RGBColor	background;
	
	
	GetLighterColors(&foreground, &background);
	RGBForeColor(&foreground);
	RGBBackColor(&background);
}// UseLighterColors


/*!
Uses GetSelectionColors() to determine the “selected”
versions of the current foreground and background
colors of the current graphics port, and makes the
selected versions the current foreground and background
colors.

(1.3)
*/
void
UseSelectionColors ()
{
	RGBColor	foreground;
	RGBColor	background;
	
	
	GetSelectionColors(&foreground, &background);
	RGBForeColor(&foreground);
	RGBBackColor(&background);
}// UseSelectionColors

// BELOW IS REQUIRED NEWLINE TO END FILE
