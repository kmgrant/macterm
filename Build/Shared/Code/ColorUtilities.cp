/*!	\file ColorUtilities.cp
	\brief Various routines commonly needed for graphics.
*/
/*###############################################################

	Interface Library 2.4
	© 1998-2012 by Kevin Grant
	
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

// library includes
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
Does the equivalent of the old QuickDraw GetGray() routine, but
for Core Graphics device colors.  Returns true only if the
conversion is successful.

NOTE:	The device parameter is currently ignored, and the main
		display is used for calculations.

WARNING:	The initial implementation is for porting only, and
			will be less efficient because it converts data
			structures heavily.

(4.0)
*/
Boolean
ColorUtilities_CGDeviceGetGray	(CGDirectDisplayID		UNUSED_ARGUMENT(inDevice),
								 CGDeviceColor const*	inBackgroundPtr,
								 CGDeviceColor*			inoutForegroundNewColorPtr)
{
	// TEMPORARY: although GetGray() works nicely, it is probably deprecated along with
	// the rest of QuickDraw (and converting into RGBColor is a pain)...figure out what
	// else can be done here to get good results
	RGBColor	backgroundColor = ColorUtilities_QuickDrawColorMake(*inBackgroundPtr);
	RGBColor	foregroundColor = ColorUtilities_QuickDrawColorMake(*inoutForegroundNewColorPtr);
	Boolean		result = false;
	
	
	result = GetGray(GetMainDevice()/* TEMPORARY; use inDevice parameter someday */,
						&backgroundColor, &foregroundColor/* both input and output */);
	*inoutForegroundNewColorPtr = ColorUtilities_CGDeviceColorMake(foregroundColor);
	
	return result;
}// GetGray


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
Creates a QuickDraw color by converting the red, green and blue
intensity values from the given device color.  The resulting
color will obviously be limited by the number of color values
supported by the RGBColor structure.

IMPORTANT:	For help with porting only!!!

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
