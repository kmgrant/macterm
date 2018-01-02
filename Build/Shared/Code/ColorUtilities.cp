/*!	\file ColorUtilities.cp
	\brief Various routines commonly needed for graphics.
*/
/*###############################################################

	Interface Library
	© 1998-2018 by Kevin Grant
	
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

// BELOW IS REQUIRED NEWLINE TO END FILE
