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
	© 1998-2013 by Kevin Grant
	
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
	kLocalization_InitFlagReadTextRightToLeft	= (1 << 0)		//!< is text read right to left, or left to right?
};

/*!
The following constants are used as a rough approximation
of the interface guidelines for cases where controls are
arranged programmatically.
*/
enum
{
	// standard heights in pixels for buttons of various kinds
	BUTTON_HT						= 20,
	BUTTON_HT_SMALL					= 17,
	CHECKBOX_HT						= 21,
	RADIOBUTTON_HT					= CHECKBOX_HT,
	TAB_HT_BIG						= 22,
	
	// amount of space required between a button’s title and its sides
	MINIMUM_BUTTON_TITLE_CUSHION	= 20,
	
	// standard distances in pixels between buttons and their surroundings
	HSP_BUTTONS						= 12,
	HSP_BUTTON_AND_DIALOG			= 24,
	VSP_BUTTON_AND_DIALOG			= 20,
	VSP_BUTTON_SMALL_AND_DIALOG		= 14,
	
	// standard distances in pixels between a tab’s edges and its surroundings
	HSP_TAB_AND_DIALOG				= 20,
	VSP_TAB_AND_DIALOG				= 17,
	
	// standard distances in pixels between a tab’s inner pane and its edges
	HSP_TABPANE_AND_TAB				= 20,
	VSP_TABPANE_AND_TAB				= 10,
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

//@}

//!\name Miscellaneous
//@{

void
	Localization_GetCurrentApplicationNameAsCFString	(CFStringRef*		outProcessDisplayNamePtr);

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

UInt16
	Localization_AutoSizeButtonControl			(ControlRef					inControl,
												 UInt16						inMinimumWidth = 86);

UInt16
	Localization_AutoSizeNSButton				(NSButton*					inButton,
												 UInt16						inMinimumWidth = 86,
												 Boolean					inResize = true);

void
	Localization_HorizontallyPlaceControls		(ControlRef					inControl1,
												 ControlRef					inControl2);

OSStatus
	Localization_SetControlThemeFontInfo		(ControlRef					inControl,
												 ThemeFontID				inThemeFontToUse);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
