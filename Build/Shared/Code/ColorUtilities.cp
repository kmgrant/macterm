/*!	\file ColorUtilities.cp
	\brief Various routines commonly needed for graphics.
*/
/*###############################################################

	Interface Library
	© 1998-2015 by Kevin Grant
	
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

#include <ColorUtilities.h>
#include <UniversalDefines.h>

// standard-C includes
#include <climits>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

/*!
Returns a CGDeviceColor equivalent to the given
QuickDraw RGBColor values.

DEPRECATED.  (Don’t rely on QuickDraw types.)

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
Returns “darker” versions of the current foreground
and background colors of the current graphics port -
except if the background color is very close to black,
then a brighter version of the color is returned.
You can pass nullptr in place of either parameter if
you are not interested in one of the colors.

You might use this to show that a range of colored
text is highlighted.

DEPRECATED.  See "colorCloserToBlack:" (NSColor
extension in Cocoa Extensions module).

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
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &black, outDarkerForegroundColorOrNull);
		black = *outDarkerForegroundColorOrNull;
		GetForeColor(outDarkerForegroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &black, outDarkerForegroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &black, outDarkerForegroundColorOrNull);
	}
	
	if (outDarkerBackgroundColorOrNull != nullptr)
	{
		// set up background color
		black.red = black.green = black.blue = 0;
		GetBackColor(outDarkerBackgroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &black, outDarkerBackgroundColorOrNull);
		black = *outDarkerBackgroundColorOrNull;
		GetBackColor(outDarkerBackgroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &black, outDarkerBackgroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &black, outDarkerBackgroundColorOrNull);
	}
}// GetDarkerColors


/*!
Returns “lighter” versions of the current foreground
and background colors of the current graphics port.
You can pass nullptr in place of either parameter if
you are not interested in one of the colors.

You might use this to represent the inactive state of
something.

DEPRECATED.  See "colorCloserToWhite:" (NSColor
extension in Cocoa Extensions module).

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
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &white, outLighterForegroundColorOrNull);
		white = *outLighterForegroundColorOrNull;
		GetForeColor(outLighterForegroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &white, outLighterForegroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &white, outLighterForegroundColorOrNull);
	}
	
	if (outLighterBackgroundColorOrNull != nullptr)
	{
		// set up background color
		white.red = white.green = white.blue = RGBCOLOR_INTENSITY_MAX;
		GetBackColor(outLighterBackgroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &white, outLighterBackgroundColorOrNull);
		white = *outLighterBackgroundColorOrNull;
		GetBackColor(outLighterBackgroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &white, outLighterBackgroundColorOrNull);
		UNUSED_RETURN(Boolean)GetGray(GetMainDevice(), &white, outLighterBackgroundColorOrNull);
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

DEPRECATED.  See "selectionColorsForForeground:background:"
(NSColor extension in Cocoa Extensions module).

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
Determines if the specified device uses Color QuickDraw.

DEPRECATED.  (Don’t rely on QuickDraw types.)

(1.0)
*/
Boolean
ColorUtilities_IsColorDevice	(GDHandle	inDevice)
{
	Boolean		result = TestDeviceAttribute(inDevice, gdDevType);
	
	
	return result;
}// IsColorDevice


/*!
Creates a QuickDraw color by converting the red, green and blue
intensity values from the given device color.  The resulting
color will obviously be limited by the number of color values
supported by the RGBColor structure.

IMPORTANT:	For help with porting only!!!

DEPRECATED.  (Don’t rely on QuickDraw types.)

(4.0)
*/
RGBColor
ColorUtilities_QuickDrawColorMake	(CGDeviceColor const&	inDeviceColor)
{
	Float32		fullIntensityFraction = 0.0;
	RGBColor	result;
	
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= inDeviceColor.red;
	result.red = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= inDeviceColor.green;
	result.green = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= inDeviceColor.blue;
	result.blue = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	return result;
}// QuickDrawColorMake


/*!
Determines the color depth of the specified port.

DEPRECATED.  (Don’t rely on QuickDraw types.)

(1.0)
*/
SInt16
ColorUtilities_ReturnCurrentDepth	(CGrafPtr	inPort)
{
	SInt16		result = 1;
	
	
	//result = (**GetPortPixMap(inPort)).pixelSize;
	result = GetPixDepth(GetPortPixMap(inPort));
	
	return result;
}// ReturnCurrentDepth


/*!
Calls PenPat() with an all-black pattern.

DEPRECATED.  (Don’t use QuickDraw.)

(1.0)
*/
void
ColorUtilities_SetBlackPenPattern ()
{
	Pattern		blackPat = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
	
	
	PenPat(&blackPat);
}// SetBlackPenPattern


/*!
Calls PenPat() with a checkerboard pattern.

DEPRECATED.  (Don’t use QuickDraw.)

(1.0)
*/
void
ColorUtilities_SetGrayPenPattern ()
{
	Pattern		grayPat = { { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA } };
	
	
	PenPat(&grayPat);
}// SetGrayPenPattern


/*!
Uses GetLighterColors() to determine the “dimmed”
versions of the current foreground and background
colors of the current graphics port, and makes the
inactive versions the current foreground and
background colors.

This routine might be used inside a drawing procedure
for a custom control, to represent an inactive state.

DEPRECATED.  (Don’t use QuickDraw.)

(1.3)
*/
void
UseInactiveColors ()
{
	RGBColor	foreground;
	RGBColor	background;
	
	
	GetLighterColors(&foreground, &background);
	RGBForeColor(&foreground);
	RGBBackColor(&background);
}// UseInactiveColors


/*!
Reverses the current foreground and background
colors of the current graphics port.

This routine might be used inside a drawing
procedure for a custom control, to represent
a selected state.

DEPRECATED.  Don’t use QuickDraw.  Manually swap
foreground and background colors.

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

DEPRECATED.  See "colorCloserToWhite:" (NSColor
extension in Cocoa Extensions module).

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

DEPRECATED.  See "selectionColorsForForeground:background:"
(NSColor extension in Cocoa Extensions module).

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
