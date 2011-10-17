/*!	\file Localization.h
	\brief Helps to make this program easier to translate into
	other languages and cultures.
	
	All Localization module utility functions work automatically
	based on your specified text reading directions.  There are
	all kinds of APIs for conveniently arranging sets of controls
	according to certain constraints.
	
	NOTE:	This header is a bit of a dumping ground for some
			miscellaneous font utilities that, while typically
			used for rendering localized UIs, are not
			particularly appropriate in this module.  They will
			probably move one day.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2011 by Kevin Grant
	
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

#ifndef __LOCALIZATION__
#define __LOCALIZATION__

#ifndef REZ

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSButton;
#else
class NSButton;
#endif

// Data Access Library includes
#include <StringUtilities.h>



#pragma mark Constants

/*!
Flags to configure text reading direction initially.
*/
typedef UInt32 Localization_InitFlags;
enum
{
	kLocalization_InitFlagReadTextRightToLeft	= (1 << 0),		//!< is text read right to left, or left to right?
	kLocalization_InitFlagReadTextBottomToTop	= (1 << 1)		//!< is text read bottom to top, or top to bottom?
};

/*!
Flags that control Localization_ArrangeControlsInRows().
*/
typedef UInt32 LocalizationRowLayoutFlags;
enum
{
	kLocalizationRowLayoutFlagsReverseSystemDirectionH	= (1 << 0),	//!< produces a right-to-left layout on a left-to-right system,
																	//!  and vice-versa
	kLocalizationRowLayoutFlagsReverseSystemDirectionV	= (1 << 1),	//!< produces a bottom-to-top layout on a top-to-bottom system,
																	//!  and vice-versa
	kLocalizationRowLayoutFlagsCenterRowCursor			= (1 << 2),	//!< UNIMPLEMENTED --
																	//!  how to align the contents of a row; the default is to
																	//!  arrange starting at the left edge of the boundary in
																	//!  left-to-right locales, or the right edge in right-to-left
																	//!  locales, but if this flag is set then row items are always
																	//!  inserted from the center, such that the row’s contents are
																	//!  collectively centered horizontally
	kLocalizationRowLayoutFlagsCenterRowItemAlignment	= (1 << 3),	//!< UNIMPLEMENTED --
																	//!  how to vertically-align the controls in a row; the default
																	//!  is to align all row items’ top edges in top-to-bottom
																	//!  locales or bottom edges in bottom-to-top locales, but if
																	//!  this flag is set then items in the same row are arranged
																	//!  so their vertical bisectors are the same
	kLocalizationRowLayoutFlagsSetVisibilityOnOverflow	= (1 << 4)	//!< ensures that all controls in the given set that fall
																	//!  within the prescribed boundary are visible; all other
																	//!  given controls are made invisible (useful for toolbars)
};

#pragma mark Types

/*!
Preserves key elements of a QuickDraw graphics port.
*/
struct GrafPortFontState
{
	SInt16		fontID;
	SInt16		fontSize;
	Style		fontStyle;
	SInt16		textMode;
};



#pragma mark Public Methods

//!\name Initialization
//@{

void
	Localization_Init							(Localization_InitFlags		inFlags);

//@}

//!\name Flag Lookup
//@{

Boolean
	Localization_IsLeftToRight					();

Boolean
	Localization_IsTopToBottom					();

//@}

//!\name Miscellaneous
//@{

void
	Localization_GetCurrentApplicationNameAsCFString	(CFStringRef*		outProcessDisplayNamePtr);

OSStatus
	Localization_GetFontTextEncoding			(ConstStringPtr				inFontName,
												 TextEncoding*				outTextEncoding);

//@}

//!\name Manipulating Font States
//@{

void
	Localization_PreservePortFontState			(GrafPortFontState*			outState);

void
	Localization_RestorePortFontState			(GrafPortFontState const*	inState);

void
	Localization_UseThemeFont					(ThemeFontID				inThemeFontToUse,
												 StringPtr					outFontName,
												 SInt16*					outFontSizePtr,
												 Style*						outFontStylePtr);

//@}

//!\name Utilities for View Manipulation
//@{

void
	Localization_AdjustHelpButtonControl		(ControlRef					inControl);

void
	Localization_ArrangeButtonArray				(ControlRef const*			inButtons,
												 UInt16						inButtonCount);

Boolean
	Localization_ArrangeControlsInRows			(Rect*						inoutBoundsPtr,
												 ControlRef*				inControlList,
												 UInt16						inControlListLength,
												 SInt16						inSpacingH,
												 SInt16						inSpacingV,
												 LocalizationRowLayoutFlags	inFlags = 0);

UInt16
	Localization_AutoSizeButtonControl			(ControlRef					inControl,
												 UInt16						inMinimumWidth = 86);

UInt16
	Localization_AutoSizeNSButton				(NSButton*					inButton,
												 UInt16						inMinimumWidth = 86,
												 Boolean					inResize = true);

void
	Localization_HorizontallyCenterControlWithinContainer	(ControlRef		inControlToCenterHorizontally,
												 ControlRef					inContainerOrNullToUseHierarchyParent = nullptr);

void
	Localization_HorizontallyPlaceControls		(ControlRef					inControl1,
												 ControlRef					inControl2);

UInt16
	Localization_ReturnSingleLineTextHeight		(ThemeFontID				inThemeFontToUse);

UInt16
	Localization_ReturnSingleLineTextWidth		(ConstStringPtr				inString,
												 ThemeFontID				inThemeFontToUse);

OSStatus
	Localization_SetControlThemeFontInfo		(ControlRef					inControl,
												 ThemeFontID				inThemeFontToUse);

UInt16
	Localization_SetUpSingleLineTextControl		(ControlRef					inControl,
												 ConstStringPtr				inTextContents,
												 Boolean					inMakeRoomForCheckBoxOrRadioButtonGlyph = false);

UInt16
	Localization_SetUpSingleLineTextControlMax	(ControlRef					inControl,
												 ConstStringPtr				inTextContents,
												 Boolean					inMakeRoomForCheckBoxOrRadioButtonGlyph,
												 UInt16						inMaximumAllowedWidth,
												 StringUtilitiesTruncationMethod	inTruncationMethod = kStringUtilities_TruncateAtEnd,
												 Boolean					inSetControlFontInfo = true);

UInt16
	Localization_SetUpMultiLineTextControl		(ControlRef					inControl,
												 ConstStringPtr				inTextContents);

void
	Localization_SizeControlsMaximumHeight		(ControlRef					inControl1,
												 ControlRef					inControl2);

void
	Localization_SizeControlsMaximumWidth		(ControlRef					inControl1,
												 ControlRef					inControl2);

void
	Localization_VerticallyCenterControlWithinContainer		(ControlRef		inControlToCenterVertically,
												 ControlRef					inContainerOrNullToUseHierarchyParent = nullptr);

void
	Localization_VerticallyPlaceControls		(ControlRef					inControl1,
												 ControlRef					inControl2);

//@}

#endif /* ifndef REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
