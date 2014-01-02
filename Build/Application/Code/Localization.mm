/*!	\file Localization.mm
	\brief Helps to make this program easier to translate into
	other languages and cultures.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2014 by Kevin Grant
	
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

#import <Localization.h>
#import <UniversalDefines.h>

// standard-C includes
#import <cctype>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AutoPool.objc++.h>
#import <CFRetainRelease.h>
#import <MemoryBlocks.h>
#import <Releases.h>

// application includes
#import "AppResources.h"



#pragma mark Constants
namespace {

// some (bad) “magic” numbers that are really “pretty good” estimates
enum
{
	kDialogTextMaxCharWidth = 7		// Width in pixels of typical-to-widest character in standard system font.
									// You can *guarantee* that *any* string you throw at this routine will not
									// get clipped if you set this to the width of the widest possible character
									// in the system font.  However, this will also make the dialog box much too
									// big in the average case.  Instead, I choose a “good” value that really is
									// sufficient for any string you’ll ever use.
};

} // anonymous namespace

#pragma mark Variables
namespace {

CFRetainRelease		gApplicationNameCFString;
Boolean				gLeftToRight = true;		// text reads left-to-right?

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSStatus	getThemeFontInfo	(ThemeFontID, ConstStringPtr, Str255, SInt16*, Style*, UInt16*, UInt16*);
OSStatus	setControlFontInfo	(ControlRef, ConstStringPtr, SInt16, Style);

} // anonymous namespace



#pragma mark Public Methods

/*!
This routine should be called once, before any
other Localization Utilities routine.  It allows
you to set flags that affect operation of all of
the module’s functions, and (optionally) to set
the file descriptor of a localization resource
file.  You only need to set the second parameter
if you plan on using Localization_UseResourceFile().

(1.0)
*/
void
Localization_Init	(UInt32		inFlags)
{
	gApplicationNameCFString = CFBundleGetValueForInfoDictionaryKey
								(AppResources_ReturnBundleForInfo(), CFSTR("CFBundleName"));
	gLeftToRight = !(inFlags & kLocalization_InitFlagReadTextRightToLeft);
}// Init


/*!
Auto-arranges a window’s help button to occupy the
lower-left (for left-to-right localization) or lower-
right (for right-to-left localization) corner of a
window.

(1.0)
*/
void
Localization_AdjustHelpButtonControl	(ControlRef		inControl)
{
	Rect	windowRect;
	Rect	controlRect;
	
	
	bzero(&windowRect, sizeof(windowRect));
	UNUSED_RETURN(OSStatus)GetWindowBounds(GetControlOwner(inControl), kWindowContentRgn, &windowRect);
	GetControlBounds(inControl, &controlRect);
	
	// now move the button
	{
		UInt16		positionH = 0,
					positionV = 0,
					buttonWidth = (controlRect.right - controlRect.left),
					buttonHeight = (controlRect.bottom - controlRect.top);
		
		
		positionV = windowRect.bottom - windowRect.top - VSP_BUTTON_AND_DIALOG - buttonHeight;
		positionH = (Localization_IsLeftToRight())
						? (HSP_BUTTON_AND_DIALOG)
						: (windowRect.right - windowRect.left - HSP_BUTTON_AND_DIALOG - buttonWidth);
		MoveControl(inControl, positionH, positionV);
	}
}// AdjustHelpButtonControl


/*!
Auto-sizes and auto-arranges a series of action
buttons for a modal window.  Buttons are assumed
to be positioned on the same line horizontally,
and located at the bottom-right corner of a modal
window (or the bottom-left corner, if the locale
is right-to-left).  Thus, buttons are moved away
from their assumed corner, and spaced exactly 12
pixels apart (as specified in "Localization.h").
The first button in the given array of item
indices is expected to be the default button (if
any), and buttons are positioned in the order
they appear in the array.

Invoking this method can make buttons move and
resize, causing onscreen flickering.  You should
invoke this method while drawing to an offscreen
graphics world or while the owning window is
invisible.

(1.1)
*/
void
Localization_ArrangeButtonArray		(ControlRef const*	inButtons,
									 UInt16				inButtonCount)
{
	register SInt16		i = 0;
	UInt16				firstButtonWidth = 0;
	SInt16				positionH = 0;
	SInt16				positionV = 0;
	Rect				windowRect;
	
	
	bzero(&windowRect, sizeof(windowRect));
	if (inButtonCount > 0)
	{
		UNUSED_RETURN(OSStatus)GetWindowBounds(GetControlOwner(inButtons[0]), kWindowContentRgn, &windowRect);
	}
	
	// resize every button
	for (i = 0; i < inButtonCount; ++i)
	{
		UInt16		buttonWidth = Localization_AutoSizeButtonControl(inButtons[i]);
		
		
		if (0 == i)
		{
			firstButtonWidth = buttonWidth;
		}
	}
	
	// now move the buttons
	if (inButtonCount > 0)
	{
		Boolean		isSmall = false;
		SInt16		deltaPositionH = 0;
		
		
		// use existing button height as a clue
		{
			HIRect		floatBounds;
			
			
			UNUSED_RETURN(OSStatus)HIViewGetBounds(inButtons[0], &floatBounds);
			if (floatBounds.size.height < BUTTON_HT)
			{
				isSmall = true;
			}
		}
		
		positionV = windowRect.bottom - windowRect.top -
					((isSmall) ? VSP_BUTTON_SMALL_AND_DIALOG : VSP_BUTTON_AND_DIALOG) -
					((isSmall) ? BUTTON_HT_SMALL : BUTTON_HT);
		positionH = (Localization_IsLeftToRight())
						? (windowRect.right - windowRect.left - HSP_BUTTON_AND_DIALOG - firstButtonWidth)
						: (HSP_BUTTON_AND_DIALOG);
		for (i = 0; i < inButtonCount; positionH += deltaPositionH)
		{
			Rect	buttonBounds;
			
			
			MoveControl(inButtons[i], positionH, positionV);
			++i;
			if (i < inButtonCount)
			{
				GetControlBounds(inButtons[i], &buttonBounds);
				deltaPositionH = ((Localization_IsLeftToRight()) ? -1 : 1) * (HSP_BUTTONS + (buttonBounds.right - buttonBounds.left));
			}
		}
	}
}// ArrangeButtonArray


/*!
Automatically sets the width of a button
control so it is wide enough to comfortably
fit its current title, but not less wide
than the specified minimum.  The chosen width
for the button is returned.

(1.0)
*/
UInt16
Localization_AutoSizeButtonControl	(ControlRef		inControl,
									 UInt16			inMinimumWidth)
{
	CFStringRef		buttonCFString = nullptr;
	UInt16			result = inMinimumWidth;
	
	
	if (inControl != nullptr)
	{
		if (noErr == CopyControlTitleAsCFString(inControl, &buttonCFString))
		{
			CFIndex const	kStringLength = CFStringGetLength(buttonCFString);
			
			
			if (kStringLength > 0)
			{
				// The quick, dirty, and usually adequate way - this saves a LOT
				// of fooling around with fonts (but maybe you would like to write
				// a better algorithm that resets the graphics port, extracts the
				// theme font information, does a little dance and calculates π to
				// 26 digits before returning the ideal width of the button?).
				result = INTEGER_DOUBLED(MINIMUM_BUTTON_TITLE_CUSHION) +
							kStringLength * kDialogTextMaxCharWidth;
			}
			CFRelease(buttonCFString), buttonCFString = nullptr;
		}
		if (result < inMinimumWidth)
		{
			result = inMinimumWidth;
		}
		SizeControl(inControl, result, BUTTON_HT);
	}
	return result;
}// AutoSizeButtonControl


/*!
Determines the width of the specified button that would
comfortably fit its current title, or else the specified
minimum width.  The chosen width for the button is returned.
If "inResize" is set to true, the button is resized to this
ideal width (the height and location are not changed).

Although NSControl has "sizeToFit", in practice this does not
work very well for buttons as the button appears to be too
constricted.

Note that this routine currently ignores other attributes of
a button that might affect its layout, such as the presence
of an image.

(2.5)
*/
UInt16
Localization_AutoSizeNSButton	(NSButton*		inButton,
								 UInt16			inMinimumWidth,
								 Boolean		inResize)
{
	AutoPool	_;
	UInt16		result = inMinimumWidth;
	
	
	if (nil != inButton)
	{
		NSString*	stringValue = [inButton title];
		NSFont*		font = [inButton font];
		
		
		// This is obviously a simplistic approach, but it is enough for now.
		// LOCALIZE THIS
		result = [font widthOfString:stringValue] + INTEGER_DOUBLED(MINIMUM_BUTTON_TITLE_CUSHION);
		if (result < inMinimumWidth)
		{
			result = inMinimumWidth;
		}
		
		if (inResize)
		{
			NSRect		frame = [inButton frame];
			
			
			frame.size.width = result;
			[inButton setFrame:frame];
		}
	}
	return result;
}// AutoSizeNSButton


/*!
Returns a copy of the name of this process.
Unreliable unless Localization_Init() has
been called.

(3.0)
*/
void
Localization_GetCurrentApplicationNameAsCFString	(CFStringRef*		outProcessDisplayNamePtr)
{
	*outProcessDisplayNamePtr = gApplicationNameCFString.returnCFStringRef();
}// GetCurrentApplicationName


/*!
Conditionally rearranges two controls horizontally
so that the left and right edges of the smallest
rectangle bounding both controls remains the same
after they trade places.

This method assumes that you have laid out controls
in the left-to-right arrangement by default, so IF
YOU ARE in left-to-right localization, calling this
routine has no effect.  The first control is
assumed to be the leftmost one, and the second is
assumed to be the rightmost one of the two.

(1.0)
*/
void
Localization_HorizontallyPlaceControls	(ControlRef		inControl1,
										 ControlRef		inControl2)
{
	unless (Localization_IsLeftToRight())
	{
		Rect	controlRect,
				combinedRect;
		
		
		// find a rectangle whose horizontal expanse is exactly large enough to touch the edges of both controls
		GetControlBounds(inControl1, &combinedRect);
		GetControlBounds(inControl2, &controlRect);
		if (combinedRect.right < controlRect.right) combinedRect.right = controlRect.right;
		if (combinedRect.left > controlRect.left) combinedRect.left = controlRect.left;
		
		// switch the controls inside the smallest space that they both occupy
		GetControlBounds(inControl1, &controlRect);
		MoveControl(inControl1, (combinedRect.right - (controlRect.right - controlRect.left)), controlRect.top);
		GetControlBounds(inControl2, &controlRect);
		MoveControl(inControl2, combinedRect.left, controlRect.top);
	}
}// HorizontallyPlaceControls


/*!
Determines if the Localization module was
initialized such that text reads left to
right (such as in North America).

(1.0)
*/
Boolean
Localization_IsLeftToRight ()
{
	Boolean		result = gLeftToRight;
	
	
	return result;
}// IsLeftToRight


/*!
Saves the current font, font size, font style,
and text mode of the current graphics port.

(1.0)
*/
void
Localization_PreservePortFontState	(GrafPortFontState*		outState)
{
	if (outState != nullptr)
	{
		GrafPtr		port = nullptr;
		
		
		GetPort(&port);
		outState->fontID = GetPortTextFont(port);
		outState->fontSize = GetPortTextSize(port);
		outState->fontStyle = GetPortTextFace(port);
		outState->textMode = GetPortTextMode(port);
	}
}// PreservePortFontState


/*!
Restores the font, font size, font style, and
text mode of the current graphics port.

(1.0)
*/
void
Localization_RestorePortFontState	(GrafPortFontState const*	inState)
{
	if (inState != nullptr)
	{
		TextFont(inState->fontID);
		TextSize(inState->fontSize);
		TextFace(inState->fontStyle);
		TextMode(inState->textMode);
	}
}// RestorePortFontState


/*!
Sets the specified control’s font, font size,
and font style to match the indicated theme
font.  If no API is available to determine
the exact font, a “reasonable approximation”
is made, usually resulting in a choice of
either the 12-point system font or 10-point
Geneva.

(1.0)
*/
OSStatus
Localization_SetControlThemeFontInfo	(ControlRef		inControl,
										 ThemeFontID	inThemeFontToUse)
{
	SInt16		fontSize = 0;
	Style		fontStyle = normal;
	Str255		fontName;
	UInt16		fontHeight = 0;
	OSStatus	result = noErr;
	
	
	result = getThemeFontInfo(inThemeFontToUse, nullptr/* string to calculate width of - unused */,
								fontName, &fontSize, &fontStyle, nullptr/* string width - unused */,
								&fontHeight);
	if (result == noErr)
	{
		result = setControlFontInfo(inControl, fontName, fontSize, fontStyle);
	}
	return result;
}// SetControlThemeFontInfo


/*!
Sets the font, size, and style for text drawn
in the current graphics port to correspond to
the specified theme font (or the closest match,
in absence of the appropriate APIs).  Useful
font information about the new font is returned
(all pointer parameters MUST NOT be nullptr).

Pass the special font code USHRT_MAX to indicate that
you want to set up a control to use the Aqua-like
“huge” system font, i.e. the system font, size 14.

(1.0)
*/
void
Localization_UseThemeFont	(ThemeFontID	inThemeFontToUse,
							 StringPtr		outFontName,
							 SInt16*		outFontSizePtr,
							 Style*			outFontStylePtr)
{
	OSStatus		error = noErr;
	Boolean			haveAppearance1_1 = false;
	
	
	// Appearance 1.1 or later?
	{
		long		appv = 0L;
		
		
		error = Gestalt(gestaltAppearanceVersion, &appv);
		if (error == noErr)
		{
			UInt8		majorRev = Releases_ReturnMajorRevisionForVersion(appv);
			UInt8		minorRev = Releases_ReturnMinorRevisionForVersion(appv);
			
			
			haveAppearance1_1 = (((majorRev == 0x01) && (minorRev >= 0x01)) || (majorRev > 0x01));
		}
	}
	
	error = noErr;
	if (haveAppearance1_1)
	{
		error = GetThemeFont((inThemeFontToUse != USHRT_MAX) ? inThemeFontToUse : STATIC_CAST(kThemeSystemFont, ThemeFontID),
								GetScriptManagerVariable(smSysScript)/* script code */,
								outFontName, outFontSizePtr, outFontStylePtr);
	}
	
	// without the appropriate Appearance Manager APIs, use the Script Manager
	if ((!haveAppearance1_1) || (error != noErr))
	{
		FMFontFamily	fontID = GetScriptVariable(GetScriptManagerVariable(smSysScript),
													((inThemeFontToUse == kThemeSystemFont) || (inThemeFontToUse == USHRT_MAX))
													? smScriptSysFond
													: smScriptAppFond);
		
		
		UNUSED_RETURN(OSStatus)FMGetFontFamilyName(fontID, outFontName);
		*outFontSizePtr = (inThemeFontToUse == kThemeSystemFont) ? 12 : 10;
		*outFontStylePtr = (inThemeFontToUse == kThemeSmallEmphasizedSystemFont) ? bold : normal;
	}
	
	// special case for Aqua-like dialogs with “huge” title text
	if (inThemeFontToUse == USHRT_MAX)
	{
		Str255		specialFontName;
		
		
		PLstrcpy(specialFontName, "\pHelvetica");
		if (kInvalidFontFamily == FMGetFontFamilyFromName(specialFontName))
		{
			// the specified font does not exist; use the system font but make it bigger
			error = GetThemeFont(kThemeSystemFont, GetScriptManagerVariable(smSysScript)/* script code */,
									outFontName, outFontSizePtr, outFontStylePtr);
			error = noErr;
			*outFontSizePtr = 16;
			*outFontStylePtr = bold;
		}
		else
		{
			PLstrcpy(outFontName, specialFontName);
			*outFontSizePtr = 18;
			*outFontStylePtr = bold;
		}
	}
	
	// change the font settings of the current graphics port to match the results
	{
		FMFontFamily	fontID = 0;
		
		
		fontID = FMGetFontFamilyFromName(outFontName);
		if (kInvalidFontFamily != fontID)
		{
			TextFont(fontID);
		}
	}
	TextSize(*outFontSizePtr);
	TextFace(*outFontStylePtr);
}// UseThemeFont


#pragma mark Internal Methods
namespace {

/*!
Provides a wealth of information about
the specified theme font (or about an
“appropriate” substitute font, if the
exact theme font cannot be determined
because Appearance 1.1 is not installed).

The font name, size, and style of the
indicated theme font are returned.  The
string width parameter is optional (if
you provide a string and string width
storage, the width of that string in
pixels is returned).  The font height
parameter is also optional.

(1.0)
*/
OSStatus
getThemeFontInfo	(ThemeFontID		inThemeFontToUse,
					 ConstStringPtr		inStringOrNull,
					 Str255				outFontName,
					 SInt16*			outFontSizePtr,
					 Style*				outFontStylePtr,
					 UInt16*			outStringWidthPtr,
					 UInt16*			outFontHeightPtrOrNull)
{
	GrafPortFontState	fontState;
	OSStatus			result = noErr;
	
	
	// save port settings
	Localization_PreservePortFontState(&fontState);
	
	// set the font, and get additional metrics information if possible
	Localization_UseThemeFont(inThemeFontToUse, outFontName, outFontSizePtr, outFontStylePtr);
	if ((outStringWidthPtr != nullptr) && (inStringOrNull != nullptr))
	{
		*outStringWidthPtr = StringWidth(inStringOrNull);
	}
	if (outFontHeightPtrOrNull != nullptr)
	{
		FontInfo		theInfo;
		
		
		GetFontInfo(&theInfo);
		*outFontHeightPtrOrNull = theInfo.ascent + theInfo.descent + INTEGER_DOUBLED(theInfo.leading);
	}
	
	// restore port settings
	Localization_RestorePortFontState(&fontState);
	
	return result;
}// getThemeFontInfo


/*!
Changes the text appearance of a control displaying
static or editable text.  Any other attributes (such
as justification) are preserved if they were set
previously.

(1.0)
*/
OSStatus
setControlFontInfo	(ControlRef			inControl,
					 ConstStringPtr		inFontName,
					 SInt16				inFontSize,
					 Style				inFontStyle)
{
	ControlFontStyleRec		styleRecord;
	FMFontFamily			fontID = 0;
	Size					actualSize = 0L;
	OSStatus				result = noErr;
	
	
	bzero(&styleRecord, sizeof(styleRecord));
	styleRecord.flags = 0;
	UNUSED_RETURN(OSStatus)GetControlData(inControl, kControlEditTextPart, kControlFontStyleTag,
											sizeof(styleRecord), &styleRecord, &actualSize);
	styleRecord.flags |= kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask;
	fontID = FMGetFontFamilyFromName(inFontName);
	if (kInvalidFontFamily != fontID)
	{
		styleRecord.font = fontID;
	}
	styleRecord.size = inFontSize;
	styleRecord.style = inFontStyle;
	result = SetControlFontStyle(inControl, &styleRecord);
	return result;
}// setControlFontInfo

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
