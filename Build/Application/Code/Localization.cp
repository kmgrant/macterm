/*###############################################################

	Localization.cp
	
	Use the powerful methods in this module to perform all kinds
	of useful operations on dialog box controls, often a must
	when doing localization work.
	
	Interface Library 1.2
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
#include <cctype>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <Releases.h>

// MacTelnet includes
#include "AppResources.h"

// resource includes
#include "SpacingConstants.r"



#pragma mark Constants

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

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Str255				gApplicationName;
	CFRetainRelease		gApplicationNameCFString;
	Boolean				gLeftToRight = true;		// text reads left-to-right?
	Boolean				gTopToBottom = true;		// text reads top-to-bottom?
}

#pragma mark Internal Method Prototypes

static OSStatus		getControlFontInfo	(ControlFontStyleRec const*, ConstStringPtr, Str255, SInt16*, Style*,
											UInt16*, UInt16*);
static OSStatus		getThemeFontInfo	(ThemeFontID, ConstStringPtr, Str255, SInt16*, Style*, UInt16*, UInt16*);
static OSStatus		setControlFontInfo	(ControlRef, ConstStringPtr, SInt16, Style);



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
	Boolean		gotIt = false;
	
	
	gApplicationNameCFString = CFBundleGetValueForInfoDictionaryKey
								(AppResources_ReturnBundleForInfo(), CFSTR("CFBundleName"));
	gotIt = gApplicationNameCFString.exists();
	if (gotIt)
	{
		ByteCount	actualSize = 0;
		
		
		gApplicationName[0] = 255;
		if (GetTextAndEncodingFromCFString(gApplicationNameCFString.returnCFStringRef(),
											gApplicationName + 1, gApplicationName[0], &actualSize,
											nullptr/* text encoding */) == noErr)
		{
			gotIt = (actualSize <= sizeof(gApplicationName));
		}
	}
	
	// if process information fails, default to low-memory value
	// (not ideal, as on Mac OS X this will not be the “display name”
	// for the application, but that of its buried executable file)
	unless (gotIt) PLstrcpy(gApplicationName, LMGetCurApName());
	
	gLeftToRight = !(inFlags & kLocalization_InitFlagReadTextRightToLeft);
	gTopToBottom = !(inFlags & kLocalization_InitFlagReadTextBottomToTop);
}// Init


/*!
Like Localization_ArrangeButtonArray(), only for
dialog items.

DEPRECATED, use Localization_ArrangeButtonArray()
instead.

(1.0)
*/
UInt16
Localization_AdjustDialogButtonArray	(DialogRef					inDialog,
										 DialogItemIndex const*		inDialogItemIndices,
										 UInt16						inDialogItemCount)
{
	UInt16				result = BUTTON_WD_MINIMUM;
	register SInt16		i = 0;
	SInt16				positionH = 0,
						positionV = 0;
	Rect				windowRect;
	
	
	GetPortBounds(GetWindowPort(GetDialogWindow(inDialog)), &windowRect);
	
	// determine the width of the widest button
	for (i = 0; i < inDialogItemCount; i++)
	{
		result = Localization_AutoSizeButtonItem(inDialog, inDialogItemIndices[i], result);
	}
	
	// resize all of the buttons to that width
	for (i = 0; i < inDialogItemCount; i++)
	{
		(UInt16)Localization_AutoSizeButtonItem(inDialog, inDialogItemIndices[i], result);
	}
	
	// now move the buttons
	{
		SInt16		deltaPositionH = ((Localization_IsLeftToRight()) ? -1 : 1) * (HSP_BUTTONS + result);
		
		
		positionV = windowRect.bottom - VSP_BUTTON_AND_DIALOG - BUTTON_HT;
		positionH = (Localization_IsLeftToRight())
						? (windowRect.right - HSP_BUTTON_AND_DIALOG - result)
						: (HSP_BUTTON_AND_DIALOG);
		for (i = 0; i < inDialogItemCount; i++, positionH += deltaPositionH)
		{
			MoveDialogItem(inDialog, inDialogItemIndices[i], positionH, positionV);
		}
	}
	
	return result;
}// AdjustDialogButtonArray


/*!
Automatically moves and resizes a dialog box’s
default and/or cancel buttons so that they are
the same size and just large enough to fit their
button titles.  Dialog buttons are assumed to be
positioned on the same line horizontally, and
located at the bottom-right corner of a dialog
box (or the bottom-left corner, if the localization
is right-to-left).  Thus, buttons are moved away
from their assumed corner, and spaced exactly 12
pixels apart (as specified in "SpacingConstants.r").

This routine operates on generic dialogs; alerts
automatically invoke it.  For convenience, the
"common size" chosen for both buttons is returned,
as a number in units of horizontal pixels.  The
buttons will not acquire a width smaller than the
standard minimum button width.

Invoking this method can cause buttons to move
and resize, causing onscreen flickering.  You
should invoke this method while drawing to an
offscreen graphics world or while the specified
dialog is invisible.

(1.0)
*/
UInt16
Localization_AdjustDialogButtons	(DialogRef		inDialog,
									 Boolean		inAdjustDefaultButton,
									 Boolean		inAdjustCancelButton)
{
	UInt16				result = 0;
	SInt16				itemCount = 0;
	DialogItemIndex		indices[2];
	
	
	if (inAdjustDefaultButton) indices[itemCount++] = GetDialogDefaultItem(inDialog);
	if (inAdjustCancelButton) indices[itemCount++] = GetDialogCancelItem(inDialog);
	
	result = Localization_AdjustDialogButtonArray(inDialog, indices, itemCount);
	
	return result;
}// AdjustDialogButtons


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
	Rect		windowRect,
				controlRect;
	
	
	GetPortBounds(GetWindowPort(GetControlOwner(inControl)), &windowRect);
	GetControlBounds(inControl, &controlRect);
	
	// now move the button
	{
		UInt16		positionH = 0,
					positionV = 0,
					buttonWidth = (controlRect.right - controlRect.left),
					buttonHeight = (controlRect.bottom - controlRect.top);
		
		
		positionV = windowRect.bottom - VSP_BUTTON_AND_DIALOG - buttonHeight;
		positionH = (Localization_IsLeftToRight())
						? (HSP_BUTTON_AND_DIALOG)
						: (windowRect.right - HSP_BUTTON_AND_DIALOG - buttonWidth);
		MoveControl(inControl, positionH, positionV);
	}
}// AdjustHelpButtonControl


/*!
Auto-arranges a dialog box’s help button to occupy
the lower-left (for left-to-right localization) or
lower-right (for right-to-left localization) corner
of a dialog.

This routine operates on generic dialogs; alerts
automatically invoke it.

(1.0)
*/
void
Localization_AdjustHelpButtonItem	(DialogRef			inDialog,
									 DialogItemIndex	inHelpButtonIndex)
{
	ControlRef		control = nullptr;
	Rect			windowRect;
	Rect			controlRect;
	
	
	(OSStatus)GetDialogItemAsControl(inDialog, inHelpButtonIndex, &control);
	GetPortBounds(GetWindowPort(GetControlOwner(control)), &windowRect);
	GetControlBounds(control, &controlRect);
	
	// now move the button
	{
		UInt16		positionH = 0,
					positionV = 0,
					buttonWidth = (controlRect.right - controlRect.left),
					buttonHeight = (controlRect.bottom - controlRect.top);
		
		
		positionV = windowRect.bottom - VSP_BUTTON_AND_DIALOG - buttonHeight;
		positionH = (Localization_IsLeftToRight())
						? (HSP_BUTTON_AND_DIALOG)
						: (windowRect.right - HSP_BUTTON_AND_DIALOG - buttonWidth);
		MoveDialogItem(inDialog, inHelpButtonIndex, positionH, positionV);
	}
}// AdjustHelpButtonItem


/*!
Auto-sizes and auto-arranges a series of action
buttons for a modal window.  Buttons are assumed
to be positioned on the same line horizontally,
and located at the bottom-right corner of a modal
window (or the bottom-left corner, if the locale
is right-to-left).  Thus, buttons are moved away
from their assumed corner, and spaced exactly 12
pixels apart (as specified in "SpacingConstants.r").
The first button in the given array of item
indices is expected to be the default button (if
any), and buttons are positioned in the order
they appear in the array.

For convenience, the "common size" chosen for
the buttons is returned, as a number in units of
horizontal pixels.  The buttons will not acquire
a width smaller than the standard minimum button
width (as specified in "SpacingConstants.r").

Invoking this method can make buttons move and
resize, causing onscreen flickering.  You should
invoke this method while drawing to an offscreen
graphics world or while the owning window is
invisible.

(1.1)
*/
UInt16
Localization_ArrangeButtonArray		(ControlRef const*	inButtons,
									 UInt16				inButtonCount)
{
	UInt16				result = BUTTON_WD_MINIMUM;
	register SInt16		i = 0;
	SInt16				positionH = 0,
						positionV = 0;
	Rect				windowRect;
	
	
	GetPortBounds(GetWindowPort(GetControlOwner(inButtons[0])), &windowRect);
	
	// determine the width of the widest button
	for (i = 0; i < inButtonCount; ++i)
	{
		result = Localization_AutoSizeButtonControl(inButtons[i], result);
	}
	
	// resize all of the buttons to that width
	for (i = 0; i < inButtonCount; ++i)
	{
		(UInt16)Localization_AutoSizeButtonControl(inButtons[i], result);
	}
	
	// now move the buttons
	{
		SInt16		deltaPositionH = ((Localization_IsLeftToRight()) ? -1 : 1) * (HSP_BUTTONS + result);
		
		
		positionV = windowRect.bottom - VSP_BUTTON_AND_DIALOG - BUTTON_HT;
		positionH = (Localization_IsLeftToRight())
						? (windowRect.right - HSP_BUTTON_AND_DIALOG - result)
						: (HSP_BUTTON_AND_DIALOG);
		for (i = 0; i < inButtonCount; ++i, positionH += deltaPositionH)
		{
			MoveControl(inButtons[i], positionH, positionV);
		}
	}
	
	return result;
}// ArrangeButtonArray


/*!
Moves a set of controls so that they fit inside
the given rectangle in a grid layout, each ideally
separated by the given amount of space in pixels.
On output, the actual bounding rectangle of the
grid is returned (almost always, the actual
boundaries will be smaller).  Returns "true" as
long as the specified set of controls fits into
the boundaries according to the given spacings.

The spacings are signed because you may want them
to be negative (implies scrunched controls).  In
certain cases, especially if you MUST collect a
large number of controls in a space that is not
strictly large enough, this routine can still be
used to lay out controls that are overlapping.

Items are arranged using a “cursor” mechanism;
that is, a cursor remains on a row as long as the
row’s total width (for controls, spacings) is not
beyond the constraint rectangle.  The height of a
row is the height of the “tallest” control in that
row.  When the cursor is asked to move horizontally
beyond the boundaries, it moves to a new grid row
(downward in top-to-bottom locales, upward in
bottom-to-top locales).

For best-looking results, provide controls that all
have the same height in pixels.  Currently, there
is no way to say that an unusually-shaped control
should span multiple grid rows or columns.

(1.0)
*/
Boolean
Localization_ArrangeControlsInRows		(Rect*							inoutBoundsPtr,
										 ControlRef*					inControlList,
										 UInt16							inControlListLength,
										 SInt16							inSpacingH,
										 SInt16							inSpacingV,
										 LocalizationRowLayoutFlags		inFlags)
{
	Boolean const	kArrangeControlsLeftToRight = (inFlags & kLocalizationRowLayoutFlagsReverseSystemDirectionH)
													? !Localization_IsLeftToRight()
													: Localization_IsLeftToRight();
	Boolean const	kArrangeControlsTopToBottom = (inFlags & kLocalizationRowLayoutFlagsReverseSystemDirectionV)
													? !Localization_IsTopToBottom()
													: Localization_IsTopToBottom();
	UInt16 const	kLayoutWidth = (inoutBoundsPtr->right - inoutBoundsPtr->left);
	UInt16 const	kCursorHReset = (kArrangeControlsLeftToRight) ? inoutBoundsPtr->left : inoutBoundsPtr->right;
	UInt16			cursorH = kCursorHReset;
	UInt16			cursorV = (kArrangeControlsTopToBottom) ? inoutBoundsPtr->top : inoutBoundsPtr->bottom;
	UInt16			currentRowWidthInPixels = 0,
					currentControlWidthInPixels = 0;
	UInt16			currentRowHeight = 0;
	Rect			controlBounds;
	Boolean			result = true;
	
	
	// arrange all controls
	for (UInt16 i = 0; i < inControlListLength; ++i)
	{
		// find width of current control
		GetControlBounds(inControlList[i], &controlBounds);
		currentControlWidthInPixels = (controlBounds.right - controlBounds.left);
		
		// if the control width is actually larger than the boundaries,
		// this is the one case where the layout may horizontally expand
		if (currentControlWidthInPixels > kLayoutWidth)
		{
			// set final horizontal boundary, depending on which
			// direction the layout would likely need to expand
			if (kArrangeControlsLeftToRight)
			{
				inoutBoundsPtr->right = inoutBoundsPtr->left + currentControlWidthInPixels + inSpacingH;
			}
			else
			{
				inoutBoundsPtr->left = inoutBoundsPtr->right - currentControlWidthInPixels - inSpacingH;
			}
		}
		
		// set up other variables for the current control
		currentRowWidthInPixels += currentControlWidthInPixels;
		if (currentRowWidthInPixels > kLayoutWidth)
		{
			// no room, make a new row (resetting variables)
			cursorH = kCursorHReset;
			if (kArrangeControlsTopToBottom) cursorV += (currentRowHeight + inSpacingV);
			else cursorV -= (currentRowHeight + inSpacingV);
			currentRowWidthInPixels = (currentControlWidthInPixels + inSpacingH);
			currentRowHeight = 0;
		}
		else
		{
			// still room, add in control spacing value
			currentRowWidthInPixels += inSpacingH;
		}
		currentRowHeight = INTEGER_MAXIMUM(currentRowHeight, controlBounds.bottom - controlBounds.top);
		
		// now arrange the control at the cursor location, but note
		// that the location is always the top-left corner, which
		// will not always be the cursor location (locale-dependent)
		if (kArrangeControlsLeftToRight)
		{
			if (kArrangeControlsTopToBottom) MoveControl(inControlList[i], cursorH, cursorV);
			else MoveControl(inControlList[i], cursorH, cursorV - currentRowHeight);
		}
		else
		{
			if (kArrangeControlsTopToBottom) MoveControl(inControlList[i], cursorH - currentControlWidthInPixels, cursorV);
			else MoveControl(inControlList[i], cursorH - currentControlWidthInPixels,
								cursorV - currentRowHeight);
		}
		
		// if requested, automatically hide controls outside the boundary and show all others
		if (inFlags & kLocalizationRowLayoutFlagsSetVisibilityOnOverflow)
		{
			Rect	unionRect;
			
			
			GetControlBounds(inControlList[i], &controlBounds);
			UnionRect(&controlBounds, inoutBoundsPtr, &unionRect);
			if (EqualRect(inoutBoundsPtr, &unionRect))
			{
				SetControlVisibility(inControlList[i], true/* visible */, true/* draw */);
			}
			else
			{
				SetControlVisibility(inControlList[i], false/* visible */, false/* draw */);
			}
		}
		
		// move the cursor
		if (kArrangeControlsLeftToRight) cursorH += (currentControlWidthInPixels + inSpacingH);
		else cursorH -= (currentControlWidthInPixels + inSpacingH);
	}
	
	// set final vertical boundary, depending on which edge the
	// cursor may actually cross
	if (kArrangeControlsTopToBottom) inoutBoundsPtr->bottom = cursorV;
	else inoutBoundsPtr->top = cursorV;
	
	return result;
}// ArrangeControlsInRows


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
	Str255		buttonString;
	UInt16		result = inMinimumWidth;
	
	
	if (inControl != nullptr)
	{
		GetControlTitle(inControl, buttonString);
		if (PLstrlen(buttonString) > 0)
		{
			// The quick, dirty, and usually adequate way - this saves a LOT
			// of fooling around with fonts (but maybe you would like to write
			// a better algorithm that resets the graphics port, extracts the
			// theme font information, does a little dance and calculates π to
			// 26 digits before returning the ideal width of the button?).
			result = INTEGER_DOUBLED(MINIMUM_BUTTON_TITLE_CUSHION) +
						PLstrlen(buttonString) * kDialogTextMaxCharWidth;
		}
		if (result < inMinimumWidth) result = inMinimumWidth;
		SizeControl(inControl, result, BUTTON_HT);
	}
	return result;
}// AutoSizeButtonControl


/*!
Automatically sets the width of a dialog
box button control so it is wide enough to
comfortably fit its current title, but not
less wide than the specified minimum.  The
chosen width for the button is returned.

(1.0)
*/
UInt16
Localization_AutoSizeButtonItem		(DialogRef			inDialog,
									 DialogItemIndex	inItemIndex,
									 UInt16				inMinimumWidth)
{
	ControlRef		control = nullptr;
	UInt16			result = inMinimumWidth;
	
	
	if (GetDialogItemAsControl(inDialog, inItemIndex, &control) == noErr)
	{
		result = Localization_AutoSizeButtonControl(control, inMinimumWidth);
		SizeDialogItem(inDialog, inItemIndex, result, BUTTON_HT); // update Dialog Manager view of control
	}
	return result;
}// AutoSizeButtonItem


/*!
Compares two character buffers to see whether
the first buffer is “less than” the second.
0 is returned only if the texts are considered
equal.

You can apply case-sensitive and diacritics-
sensitive comparison, which will return -1 if
the first string is “less than” the second,
or 1 if the second string is “less than” the
first.

If you don’t apply case-sensitive sorting, the
result is 1 if the strings are not equal.

(3.0)
*/
SInt16
Localization_CompareTextSystemScript	(Boolean		inCaseSensitive,
										 void const*	inBuffer1,
										 void const*	inBuffer2,
										 Size			inBuffer1Size,
										 Size			inBuffer2Size)
{
	Handle		itl2Table = nullptr;
	long		offset = 0L;
	long		length = 0L;
	short		script = 0;
	SInt16		result = 0;
	
	
	script = FontToScript(GetSysFont());
	GetIntlResourceTable(script, smWordSelectTable, &itl2Table, &offset, &length);
	if (itl2Table != nullptr)
	{
		if (inCaseSensitive) result = CompareText(inBuffer1, inBuffer2, inBuffer1Size, inBuffer2Size, itl2Table);
		else result = IdenticalText(inBuffer1, inBuffer2, inBuffer1Size, inBuffer2Size, itl2Table) ? 1 : 0;
	}
	return result;
}// CompareTextSystemScript


/*!
Returns a copy of the name of this process.
Unreliable unless Localization_Init() has
been called.

(3.0)
*/
void
Localization_GetCurrentApplicationName		(Str255		outProcessDisplayName)
{
	PLstrcpy(outProcessDisplayName, gApplicationName);
}// GetCurrentApplicationName


#if TARGET_API_MAC_CARBON
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
#endif


/*!
Returns the text encoding base for the specified
font.  This can be used to create a new text
encoding converter.

(3.0)
*/
OSStatus
Localization_GetFontTextEncoding	(ConstStringPtr		inFontName,
									 TextEncoding*		outTextEncoding)
{
	OSStatus	result = noErr;
	
	
	if (outTextEncoding == nullptr) result = memPCErr;
	else if (API_AVAILABLE(FontToScript) && API_AVAILABLE(UpgradeScriptInfoToTextEncoding))
	{
		//LangCode		textLanguageID = GetScriptVariable(smSystemScript, smScriptLang);
		SInt16			fontID = 0;
		ScriptCode		scriptCode = smRoman;
		TextEncoding	encoding = kTextEncodingMacRoman;
		
		
		GetFNum(inFontName, &fontID);
		scriptCode = FontToScript(fontID);
		
		// The Mac OS uses a giant map to determine the proper encoding for
		// “special cases” (such as Icelandic) that use the Roman script;
		// in these cases, the script code alone isn’t enough to choose the
		// right encoding, so the system localization itself is used!  To
		// get this behavior, the language and region must be “don’t care”.
		result = UpgradeScriptInfoToTextEncoding(scriptCode, kTextLanguageDontCare, kTextRegionDontCare,
													inFontName, &encoding);
	}
	else
	{
		*outTextEncoding = kTextEncodingMacRoman; // assume this when an error occurs
		result = cfragNoSymbolErr;
	}
	
	return result;
}// GetFontTextEncoding


/*!
Arranges the specified control so that its bisector
is aligned with the bisector of its parent, resulting
in the left half of the control being to the left of
the bisector (and the right half to the right of it).

(1.0)
*/
void
Localization_HorizontallyCenterControlWithinContainer	(ControlRef		inControlToCenterHorizontally,
														 ControlRef		inContainerOrNullToUseHierarchyParent)
{
	ControlRef	container = inContainerOrNullToUseHierarchyParent;
	Rect		controlBounds;
	Rect		containerBounds;
	
	
	if (container == nullptr)
	{
		if (GetSuperControl(inControlToCenterHorizontally, &inContainerOrNullToUseHierarchyParent) != noErr)
		{
			container = nullptr;
		}
	}
	
	if (container != nullptr)
	{
		GetControlBounds(inControlToCenterHorizontally, &controlBounds);
		GetControlBounds(container, &containerBounds);
		MoveControl(inControlToCenterHorizontally,
					containerBounds.left + INTEGER_HALVED((containerBounds.right - containerBounds.left) -
															(controlBounds.right - controlBounds.left)),
					controlBounds.top);
	}
}// HorizontallyCenterControlWithinContainer


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
Conditionally rearranges two dialog items horizontally
so that the left and right edges of the smallest
rectangle bounding both controls remains the same
after they trade places.

This method assumes that you have laid out controls in
the left-to-right arrangement by default, so IF YOU
ARE in left-to-right localization, calling this
routine has no effect.

(1.0)
*/
void
Localization_HorizontallyPlaceItems		(DialogRef			inDialog,
										 DialogItemIndex	inItemIndex1,
										 DialogItemIndex	inItemIndex2)
{
	unless (Localization_IsLeftToRight())
	{
		ControlRef		control1 = nullptr;
		ControlRef		control2 = nullptr;
		Rect			controlRect;
		Rect			combinedRect;
		
		
		(OSStatus)GetDialogItemAsControl(inDialog, inItemIndex1, &control1);
		(OSStatus)GetDialogItemAsControl(inDialog, inItemIndex2, &control2);
		
		// find a rectangle whose horizontal expanse is exactly large enough to touch the edges of both controls
		GetControlBounds(control1, &combinedRect);
		GetControlBounds(control2, &controlRect);
		if (combinedRect.right < controlRect.right) combinedRect.right = controlRect.right;
		if (combinedRect.left > controlRect.left) combinedRect.left = controlRect.left;
		
		// switch the controls inside the smallest space that they both occupy
		GetControlBounds(control1, &controlRect);
		MoveDialogItem(inDialog, inItemIndex1,
						(combinedRect.right - (controlRect.right - controlRect.left)), controlRect.top);
		GetControlBounds(control2, &controlRect);
		MoveDialogItem(inDialog, inItemIndex2, combinedRect.left, controlRect.top);
	}
}// HorizontallyPlaceItems


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
Determines if the Localization module was
initialized such that text reads top to
bottom (such as in North America).

(1.0)
*/
Boolean
Localization_IsTopToBottom ()
{
	Boolean		result = gTopToBottom;
	
	
	return result;
}// IsTopToBottom


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
Determines the height required to fit a single
line of text in the specified theme font.  If
no API exists (i.e. earlier than Appearance 1.1)
to calculate the font exactly, the font constant
is used to determine the correct value for a
“reasonable font” (if you ask for the system
font, you get a 12-point system font height, for
most others you get 10-point Geneva, etc.).

(1.0)
*/
UInt16
Localization_ReturnSingleLineTextHeight		(ThemeFontID	inThemeFontToUse)
{
	UInt16		result = 0;
	SInt16		fontSize = 0;
	Style		fontStyle = normal;
	Str255		fontName;
	
	
	if (getThemeFontInfo(inThemeFontToUse, nullptr/* string to calculate width of - unused */,
							fontName, &fontSize, &fontStyle, nullptr/* string width - unused */,
							&result/* font height */) != noErr) result = 0;
	return result;
}// ReturnSingleLineTextHeight


/*!
Determines the width required to fit the given
line of text in the specified theme font.  If
no API exists (i.e. earlier than Appearance 1.1)
to calculate the font exactly, the font constant
is used to determine the correct value for a
“reasonable font” (if you ask for the system
font, you get a 12-point system font height, for
most others you get 10-point Geneva, etc.).

(1.0)
*/
UInt16
Localization_ReturnSingleLineTextWidth	(ConstStringPtr		inString,
										 ThemeFontID		inThemeFontToUse)
{
	UInt16		result = 0;
	SInt16		fontSize = 0;
	Style		fontStyle = normal;
	Str255		fontName;
	
	
	if (getThemeFontInfo(inThemeFontToUse, inString/* string to calculate width of */,
							fontName, &fontSize, &fontStyle, &result/* string width */,
							nullptr/* font height - unused */) != noErr) result = 0;
	return result;
}// ReturnSingleLineTextWidth


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
Works like Localization_SetUpSingleLineTextControlMax(),
except there is no limit on the width the resultant
control can have (it remains one line, as wide as the
text happens to be).

The chosen width for the control, in pixels, is
returned.

(1.0)
*/
UInt16
Localization_SetUpSingleLineTextControl		(ControlRef			inControl,
											 ConstStringPtr		inTextContents,
											 Boolean			inMakeRoomForCheckBoxOrRadioButtonGlyph)
{
	return Localization_SetUpSingleLineTextControlMax(inControl, inTextContents,
														inMakeRoomForCheckBoxOrRadioButtonGlyph,
														0/* means “no limits” */,
														kStringUtilities_TruncateAtEnd);
}// SetUpSingleLineTextControl


/*!
Automatically sets the contents, width and height
of a text control (editable or static) to accommodate
its theme font.  The width of the control is made
sufficient to hold the specified text, within the
specified width limit (pass 0 to have no width
limits).  If the control text is too wide, it is
automatically changed to include a trailing ellipsis
(...) and truncated to fit.

The chosen width for the control, in pixels, is
returned.

(1.0)
*/
UInt16
Localization_SetUpSingleLineTextControlMax		(ControlRef							inControl,
												 ConstStringPtr						inTextContents,
												 Boolean							inIsCheckBoxOrRadioButton,
												 UInt16								inMaximumAllowedWidth,
												 StringUtilitiesTruncationMethod	inTruncationMethod,
												 Boolean							inSetControlFontInfo)
{
	enum
	{
		kStringPaddingH = 8		// arbitrary horizontal extra space, in pixels, “just to be safe”
	};
	UInt16		result = 0;
	SInt16		fontSize = 0;
	Style		fontStyle = normal;
	Str255		fontName;
	UInt16		fontHeight = 0;
	GrafPtr		oldPort = nullptr;
	Str255		buffer;
	
	
	PLstrcpy(buffer, inTextContents);
	
	GetPort(&oldPort);
	SetPortWindowPort(GetControlOwner(inControl));
	
	// obtain a wealth of information about the requested theme font
	{
		ControlFontStyleRec		controlFontInfo;
		OSStatus				error = noErr;
		Size					actualSize = 0;
		
		
		error = GetControlData(inControl, kControlNoPart, kControlFontStyleTag, sizeof(controlFontInfo),
								&controlFontInfo, &actualSize);
		if (error == noErr)
		{
			(OSStatus)getControlFontInfo(&controlFontInfo, buffer/* string to calculate width of */,
											fontName, &fontSize, &fontStyle, &result/* string width */,
											&fontHeight);
		}
	}
	
	if (inSetControlFontInfo)
	{
		// set the font characteristics to be the font derived above
		(OSStatus)setControlFontInfo(inControl, fontName, fontSize, fontStyle);
	}
	
	// size the control to be just wide enough for the text
	result += kStringPaddingH; // add some (arbitrary) additional space to be sure
	if (inMaximumAllowedWidth > 0)
	{
		// a maximum was given; be sure the text isn’t any longer
		(Boolean)StringUtilities_PTruncate(buffer, inMaximumAllowedWidth, inTruncationMethod);
	}
	
	// set the control size
	{
		SInt16		newControlWidth = result,
					newControlHeight = fontHeight;
		
		
		if (inIsCheckBoxOrRadioButton)
		{
			newControlWidth += 25/* arbitrary */;
			newControlHeight = CHECKBOX_HT;
		}
		SizeControl(inControl, newControlWidth, newControlHeight);
	}
	
	// set the control text to be the specified text
	if (inIsCheckBoxOrRadioButton)
	{
		SetControlTitle(inControl, buffer);
	}
	else
	{
		(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlStaticTextTextTag,
									PLstrlen(buffer) * sizeof(char), (Ptr)(buffer + 1));
	}
	
	SetPort(oldPort);
	
	return result;
}// SetUpSingleLineTextControlMax


/*!
Automatically sets the contents and height of a
text control (editable or static) to accommodate
its font, style and size info.  The width of the
control is not changed.

The chosen height for the control, in pixels, is
returned.

(1.0)
*/
UInt16
Localization_SetUpMultiLineTextControl	(ControlRef			inControl,
										 ConstStringPtr		inTextContents)
{
	UInt16			result = 0;
	UInt16			textExpanseH = 0;
	UInt16 const	textContentsLength = PLstrlen(inTextContents);
	SInt16			fontSize = 0;
	Style			fontStyle = normal;
	Str255			fontName;
	UInt16			fontHeight = 0;
	UInt16			charWidth = 0;
	GrafPtr			oldPort = nullptr;
	
	
	GetPort(&oldPort);
	SetPortWindowPort(GetControlOwner(inControl));
	
	// obtain a wealth of information about the requested theme font
	// (in the “quick and dirty” calculation, approximate character
	// widths are used with a small margin for error; if no such
	// short-cut is taken, the error is less but the calculation is
	// significantly slower)
	{
		ControlFontStyleRec		controlFontInfo;
		OSStatus				error = noErr;
		Size					actualSize = 0;
		
		
		error = GetControlData(inControl, kControlNoPart, kControlFontStyleTag, sizeof(controlFontInfo),
								&controlFontInfo, &actualSize);
		if (error == noErr)
		{
			(OSStatus)getControlFontInfo(&controlFontInfo, "\pA"/* string to calculate width of */,
											fontName, &fontSize, &fontStyle, &charWidth/* string width */,
											&fontHeight);
		}
	}
	
	// size the control to be just high enough for the text
	{
		GrafPortFontState	fontState;
		Rect				controlRect;
		SInt16				fontID = 0;
		
		
		// save port settings
		Localization_PreservePortFontState(&fontState);
		
		// set port font so CharWidth() can be used
		fontID = FMGetFontFamilyFromName(fontName);
		TextFont(fontID);
		TextSize(fontSize);
		TextFace(fontStyle);
		
		GetControlBounds(inControl, &controlRect);
		textExpanseH = controlRect.right - controlRect.left;
		#define CHEAP_TEXT_HEIGHT_CALCULATION 0
		#if CHEAP_TEXT_HEIGHT_CALCULATION
		// this is “pretty good” for most cases, but there is a chance that
		// exceptional text will make this height calculation inadequate
		result = (textContentsLength * charWidth / textExpanseH + 1) * fontHeight;
		#else
		// this will cover more exceptional cases, but for the average case
		// it might be overkill because it traverses the entire string and
		// usually yields the same result as the short-cut calculation above
		{
			UInt8 const*	ptr = inTextContents + 1; // skip length byte
			SInt16			length = 0, // count of characters
							lineWidth = 0;  // pixel width of characters so far on one line
			Boolean			lastCharacter = false,
							newLine = false;
			
			
			lastCharacter = false;
			for (result = 0; (!lastCharacter); result += fontHeight)
			{
				lineWidth = 0;
				newLine = false;
				while ((!lastCharacter) && (!newLine))
				{
					UInt8	c = '\0';
					
					
					c = ptr[0];
					ptr++;
					length++;
					if (c == '\n')
					{
						newLine = true;
					}
					else
					{
						lineWidth += CharWidth(c);
						if (lineWidth >= textExpanseH)
						{
							// end-of-line reached implicitly (i.e. no '\n'); check to see
							// if it’s the end of the word, too; if not, back up, because
							// the Mac OS will wrap the remaining characters to the next line
							if (length < textContentsLength)
							{
								while (CPP_STD::isalnum((char)ptr[0]) && (length > 0))
								{
									ptr--;
									length--;
								}
							}
							newLine = true;
						}
					}
					if (length >= textContentsLength) lastCharacter = true;
				}
			}
		}
		#endif
		
		// make room for blank lines, if the text contains new-line characters
		{
			UInt16				extraBlanks = 0;
			register UInt16		i = 0;
			
			
			for (i = 1; i <= textContentsLength; i++)
			{
				if (inTextContents[i] == '\n') extraBlanks++;
			}
			result += (extraBlanks * fontHeight);
		}
		
		// set the control’s height to match...
		SizeControl(inControl, textExpanseH, result);
		
		// restore port settings
		Localization_RestorePortFontState(&fontState);
	}
	
	// set the control text to be the specified text
	(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlStaticTextTextTag,
								textContentsLength * sizeof(char), (Ptr)(inTextContents + 1));
	
	SetPort(oldPort);
	
	return result;
}// SetUpMultiLineTextControl


/*!
Determines which of the two given controls is highest,
and changes the height of the other control to match.
The controls’ widths and positions are unchanged.

(1.1)
*/
void
Localization_SizeControlsMaximumHeight	(ControlRef		inControl1,
										 ControlRef		inControl2)
{
	Rect	control1Rect,
			control2Rect;
	
	
	GetControlBounds(inControl1, &control1Rect);
	GetControlBounds(inControl2, &control2Rect);
	SizeControl(inControl1, control1Rect.right - control1Rect.left,
				INTEGER_MAXIMUM(control1Rect.bottom - control1Rect.top,
								control2Rect.bottom - control2Rect.top));
	SizeControl(inControl2, control2Rect.right - control2Rect.left,
				INTEGER_MAXIMUM(control1Rect.bottom - control1Rect.top,
								control2Rect.bottom - control2Rect.top));
}// SizeControlsMaximumHeight


/*!
Determines which of the two given controls is widest,
and changes the width of the other control to match.
The controls’ heights and positions are unchanged.

(1.1)
*/
void
Localization_SizeControlsMaximumWidth	(ControlRef		inControl1,
										 ControlRef		inControl2)
{
	Rect	control1Rect,
			control2Rect;
	
	
	GetControlBounds(inControl1, &control1Rect);
	GetControlBounds(inControl2, &control2Rect);
	SizeControl(inControl1, INTEGER_MAXIMUM(control1Rect.right - control1Rect.left,
											control2Rect.right - control2Rect.left),
				control1Rect.bottom - control1Rect.top);
	SizeControl(inControl2, INTEGER_MAXIMUM(control1Rect.right - control1Rect.left,
											control2Rect.right - control2Rect.left),
				control2Rect.bottom - control2Rect.top);
}// SizeControlsMaximumWidth


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
	
	error = noErr;
	//unless (FlagManager_Test(kFlagOS10_2API))
	{
		if (inThemeFontToUse == kThemeToolbarFont)
		{
			// the toolbar font is not available on Mac OS X 10.1 and earlier,
			// but it can be approximated by resizing the small system font
			error = GetThemeFont(kThemeSmallSystemFont, GetScriptManagerVariable(smSysScript)/* script code */,
									outFontName, outFontSizePtr, outFontStylePtr);
			error = noErr;
		}
	}
	
	// without the appropriate Appearance Manager APIs, use the Script Manager
	if ((!haveAppearance1_1) || (error != noErr))
	{
		SInt16	fontID = GetScriptVariable(GetScriptManagerVariable(smSysScript),
											((inThemeFontToUse == kThemeSystemFont) || (inThemeFontToUse == USHRT_MAX))
											? smScriptSysFond
											: smScriptAppFond);
		
		
		(OSStatus)FMGetFontFamilyName(fontID, outFontName);
		*outFontSizePtr = (inThemeFontToUse == kThemeSystemFont) ? 12 : 10;
		*outFontStylePtr = (inThemeFontToUse == kThemeSmallEmphasizedSystemFont) ? bold : normal;
	}
	
	// special case for Aqua-like dialogs with “huge” title text
	if (inThemeFontToUse == USHRT_MAX)
	{
		*outFontSizePtr = 16;
		*outFontStylePtr = bold;
	}
	
	// change the font settings of the current graphics port to match the results
	{
		SInt16		fontID = 0;
		
		
		fontID = FMGetFontFamilyFromName(outFontName);
		TextFont(fontID);
	}
	TextSize(*outFontSizePtr);
	TextFace(*outFontStylePtr);
}// UseThemeFont


/*!
Arranges the specified control so that its bisector
is aligned with the bisector of its parent, resulting
in the top half of the control being above the bisector
(and the bottom half below it).

(1.0)
*/
void
Localization_VerticallyCenterControlWithinContainer		(ControlRef		inControlToCenterVertically,
														 ControlRef		inContainerOrNullToUseHierarchyParent)
{
	ControlRef	container = inContainerOrNullToUseHierarchyParent;
	Rect		controlBounds,
				containerBounds;
	
	
	if (container == nullptr)
	{
		if (GetSuperControl(inControlToCenterVertically, &inContainerOrNullToUseHierarchyParent) != noErr)
		{
			container = nullptr;
		}
	}
	
	if (container != nullptr)
	{
		GetControlBounds(inControlToCenterVertically, &controlBounds);
		GetControlBounds(container, &containerBounds);
		MoveControl(inControlToCenterVertically, controlBounds.left,
					containerBounds.top + INTEGER_HALVED((containerBounds.bottom - containerBounds.top) -
														(controlBounds.bottom - controlBounds.top)));
	}
}// VerticallyCenterControlWithinContainer


/*!
Conditionally rearranges two controls vertically
so that the top and bottom edges of the smallest
rectangle bounding both controls remains the
same after they trade places.

This method assumes that you have laid out controls
in the top-to-bottom arrangement by default, so IF
YOU ARE in top-to-bottom localization, calling this
routine has no effect.  The first control is
assumed to be the topmost one, and the second is
assumed to be the bottommost one of the two.

(1.0)
*/
void
Localization_VerticallyPlaceControls	(ControlRef		inControl1,
										 ControlRef		inControl2)
{
	unless (Localization_IsTopToBottom())
	{
		Rect	controlRect,
				combinedRect;
		
		
		// find a rectangle whose vertical expanse is exactly large enough to touch the edges of both controls
		GetControlBounds(inControl1, &combinedRect);
		GetControlBounds(inControl2, &controlRect);
		if (combinedRect.bottom < controlRect.bottom) combinedRect.bottom = controlRect.bottom;
		if (combinedRect.top > controlRect.top) combinedRect.top = controlRect.top;
		
		// switch the controls inside the smallest space that they both occupy
		GetControlBounds(inControl1, &controlRect);
		MoveControl(inControl1, controlRect.left, (combinedRect.bottom - (controlRect.bottom - controlRect.top)));
		GetControlBounds(inControl2, &controlRect);
		MoveControl(inControl2, controlRect.left, combinedRect.top);
	}
}// VerticallyPlaceControls


/*!
Conditionally rearranges two dialog items vertically
so that the top and bottom edges of the smallest
rectangle bounding both controls remains the same
after they trade places.

This method assumes that you have laid out controls
in the top-to-bottom arrangement by default, so IF
YOU ARE in top-to-bottom localization, calling this
routine has no effect.

(1.0)
*/
void
Localization_VerticallyPlaceItems	(DialogRef			inDialog,
									 DialogItemIndex	inItemIndex1,
									 DialogItemIndex	inItemIndex2)
{
	unless (Localization_IsTopToBottom())
	{
		ControlRef		control1 = nullptr;
		ControlRef		control2 = nullptr;
		Rect			controlRect;
		Rect			combinedRect;
		
		
		// find a rectangle whose vertical expanse is exactly large enough to touch the edges of both controls
		GetControlBounds(control1, &combinedRect);
		GetControlBounds(control2, &controlRect);
		if (combinedRect.bottom < controlRect.bottom) combinedRect.bottom = controlRect.bottom;
		if (combinedRect.top > controlRect.top) combinedRect.top = controlRect.top;
		
		// switch the controls inside the smallest space that they both occupy
		GetControlBounds(control1, &controlRect);
		MoveDialogItem(inDialog, inItemIndex1,
						controlRect.left, (combinedRect.bottom - (controlRect.bottom - controlRect.top)));
		GetControlBounds(control2, &controlRect);
		MoveDialogItem(inDialog, inItemIndex2, controlRect.left, combinedRect.top);
	}
}// VerticallyPlaceItems


#pragma mark Internal Methods

/*!
Provides a wealth of information about the
font in the given control font style record.

The font name, size, and style of the indicated
theme font are returned.  The string width
parameter is optional (if you provide a string
and string width storage, the width of that
string in pixels is returned).  The font height
parameter is also optional.

Since there is a "flags" field in the record
that allows entries to be omitted, this routine
checks the flags and substitutes values as best
it can if anything is undefined.  In general,
provide a complete record as input to get the
results you desire.

(1.0)
*/
static OSStatus
getControlFontInfo	(ControlFontStyleRec const*	inFontStyleRecPtr,
					 ConstStringPtr				inStringOrNull,
					 Str255						outFontName,
					 SInt16*					outFontSizePtr,
					 Style*						outFontStylePtr,
					 UInt16*					outStringWidthPtr,
					 UInt16*					outFontHeightPtr)
{
	GrafPortFontState	fontState;
	OSStatus			result = noErr;
	
	
	// save port settings
	Localization_PreservePortFontState(&fontState);
	
	// set the font, and get additional metrics information if possible
	if ((inFontStyleRecPtr->flags & kControlUseFontMask) &&
		(inFontStyleRecPtr->flags & kControlUseFaceMask) &&
		(inFontStyleRecPtr->flags & kControlUseSizeMask))
	{
		TextFont(inFontStyleRecPtr->font);
		TextFace(inFontStyleRecPtr->style);
		TextSize(inFontStyleRecPtr->size);
	}
	else
	{
		Localization_UseThemeFont(kThemeSystemFont, outFontName, outFontSizePtr, outFontStylePtr);
	}
	
	if ((outStringWidthPtr != nullptr) && (inStringOrNull != nullptr))
	{
		*outStringWidthPtr = StringWidth(inStringOrNull);
	}
	if (outFontHeightPtr != nullptr)
	{
		FontInfo		theInfo;
		
		
		GetFontInfo(&theInfo);
		*outFontHeightPtr = theInfo.ascent + theInfo.descent + INTEGER_DOUBLED(theInfo.leading);
	}
	
	// restore port settings
	Localization_RestorePortFontState(&fontState);
	
	return result;
}// getControlFontInfo


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
static OSStatus
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
static OSStatus
setControlFontInfo	(ControlRef			inControl,
					 ConstStringPtr		inFontName,
					 SInt16				inFontSize,
					 Style				inFontStyle)
{
	ControlFontStyleRec		styleRecord;
	SInt16					fontID = 0;
	Size					actualSize = 0L;
	OSStatus				result = noErr;
	
	
	fontID = FMGetFontFamilyFromName(inFontName);
	styleRecord.flags = 0;
	(OSStatus)GetControlData(inControl, kControlEditTextPart, kControlFontStyleTag,
								sizeof(styleRecord), &styleRecord, &actualSize);
	styleRecord.flags |= kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask;
	styleRecord.font = fontID;
	styleRecord.size = inFontSize;
	styleRecord.style = inFontStyle;
	result = SetControlFontStyle(inControl, &styleRecord);
	return result;
}// setControlFontInfo

// BELOW IS REQUIRED NEWLINE TO END FILE
