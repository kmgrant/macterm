/*!	\file TerminalView.h
	\brief Renders the contents of a terminal screen buffer and
	handles interaction with the user (such as selection of
	text).
	
	This is the Terminal View module, which defines the visual
	component of a terminal screen and tends to refer to screen
	coordinates in pixels.  Compare this to the Terminal Screen
	module (Terminal.h), which works with terminal screens in
	terms of the data in them, and tends to refer to screen
	coordinates in rows and columns.
	
	Generally, you only use Terminal View APIs to manipulate
	things that are unique to user interaction with a terminal,
	such as the text selection.  Anything that is data-centric
	should be manipulated from the Terminal Screen standpoint
	(see Terminal.h), because data changes will eventually be
	propagated to the view for rendering.  So, expect only the
	Terminal Screen module to use most of these APIs.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

#ifndef __TERMINALVIEW__
#define __TERMINALVIEW__

// standard-C++ includes
#include <utility>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include "ListenerModel.h"

// MacTelnet includes
#include "Preferences.h"
#include "TerminalRangeDescription.typedef.h"
#include "TerminalScreenRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from certain APIs in this module.
*/
enum TerminalView_Result
{
	kTerminalView_ResultOK = 0,					//!< no error
	kTerminalView_ResultInvalidID = -1,			//!< a given "TerminalViewRef" does not correspond to any known view
	kTerminalView_ResultParameterError = -2,	//!< invalid input (e.g. a null pointer)
	kTerminalView_ResultNotEnoughMemory = -3,	//!< there is not enough memory to allocate required data structures
	kTerminalView_ResultIllegalOperation = -4   //!< attempt to change a setting that is currently automatically-controlled
};

/*!
Identifiers for the ÒcustomÓ colors of a terminal view.
*/
typedef SInt16 TerminalView_ColorIndex;
enum
{
	kTerminalView_ColorIndexNormalText				= 0,
	kTerminalView_ColorIndexNormalBackground		= 1,
	kTerminalView_ColorIndexBlinkingText			= 2,
	kTerminalView_ColorIndexBlinkingBackground		= 3,
	kTerminalView_ColorIndexBoldText				= 4,
	kTerminalView_ColorIndexBoldBackground			= 5,
	kTerminalView_ColorIndexMatteBackground			= 6,
	
	// useful constants
	kTerminalView_ColorIndexFirstValid				= kTerminalView_ColorIndexNormalText,
	kTerminalView_ColorIndexLastValid				= kTerminalView_ColorIndexMatteBackground
};

/*!
Determines the shape of the cursor, when rendered.
*/
enum TerminalView_CursorType
{
	kTerminalView_CursorTypeBlock					= 0,	//!< solid, filled rectangle
	kTerminalView_CursorTypeUnderscore				= 1,	//!< 1-pixel-high underline
	kTerminalView_CursorTypeVerticalLine			= 2,	//!< standard Mac insertion point appearance
	kTerminalView_CursorTypeThickUnderscore			= 3,	//!< 2-pixel-high underscore, makes cursor easier to see
	kTerminalView_CursorTypeThickVerticalLine		= 4,	//!< 2-pixel-wide vertical line, makes cursor easier to see
	kTerminalView_CursorTypeCurrentPreferenceValue	= 200	//!< meta-value only used as a parameter in some routines
};

/*!
Determines how the pixel area of the container control is filled
with terminal content.  Normally, the font is a specific size, and
the content either fits or doesnÕt fit the pixel area.  However, in
zoom mode, the content is forced to fit in the pixel area; the font
size changes to whatever size makes the content best fit the area.
*/
enum TerminalView_DisplayMode
{
	kTerminalView_DisplayModeNormal		= 'Norm',   //!< underlying terminal screenÕs dimensions are altered to
													//!  best suit the pixel dimensions of the view, when resized;
													//!  the terminal view font size is unchanged
	kTerminalView_DisplayModeZoom		= 'Zoom'	//!< font size of text in view is altered to make the current
													//!  rows and columns best fill the screen area, when resized;
													//!  the underlying terminal screenÕs dimensions are unchanged
};

/*!
Events in a Terminal View that other modules can register to
receive notification of.
*/
enum TerminalView_Event
{
	kTerminalView_EventFontSizeChanged	= 'FSiz',	//!< the font size used for drawing text has been altered
													//!  (context: TerminalViewRef)
	kTerminalView_EventScrolling		= 'Scrl'	//!< the visible part of the terminal view has changed
													//!  (context: TerminalViewRef)
};

/*!
Special ranges of a terminal view.  Ranges always start at
0 (the ÒoldestÓ pixel, for the vertical axis), and end at
one past the actual value (useful algorithmically).  So for
instance, the first 10 pixels of the view would be represented
as the range (0, 10), where 10 is one past the end (it
follows that the pixel count in the range is the difference
between the start and end points).
*/
enum TerminalView_RangeCode
{
	kTerminalView_RangeCodeScrollRegionV			= 0,	//!< the *scroll* region of the screen background, in pixels;
															//!  this can be compared with the maximum scroll region to see
															//!  both where in the maximum space the screen is scrolled, and
															//!  how much of the maximum screen is showing
	kTerminalView_RangeCodeScrollRegionVMaximum		= 1		//!< the maximum *virtual* region of the screen background, in
															//!  pixels; useful for comparisons against the range returned
															//!  for "kTerminalView_RangeCodeScrollRegionV"
};

#include "TerminalTextAttributes.typedef.h"

typedef UInt16 TerminalView_TextFlags;
enum
{
	kTerminalView_TextFlagInline = (1 << 0)		//!< strip end-of-line markers?
};

#pragma mark Types

#include "TerminalViewRef.typedef.h"

typedef std::pair< UInt16, SInt32 >							TerminalView_Cell;		//!< order: column, row

typedef std::pair< TerminalView_Cell, TerminalView_Cell >	TerminalView_CellRange;	//!< order: inclusive start, exclusive end



#pragma mark Public Methods

//!\name Initialization
//@{

void
	TerminalView_Init				();

void
	TerminalView_Done				();

//@}

//!\name Creating and Destroying Terminal Views
//@{

// AUTOMATICALLY DESTROYED WHEN THE VIEW FROM TerminalView_ReturnContainerHIView() GOES AWAY
TerminalViewRef
	TerminalView_NewHIViewBased		(TerminalScreenRef			inScreenDataSource,
									 Preferences_ContextRef		inFormatOrNull = nullptr);

//@}

//!\name Event Notification
//@{

TerminalView_Result
	TerminalView_IgnoreChangesToPreference	(TerminalViewRef			inView,
											 Preferences_Tag			inWhichSetting);

TerminalView_Result
	TerminalView_StartMonitoring			(TerminalViewRef			inRef,
											 TerminalView_Event			inForWhatEvent,
											 ListenerModel_ListenerRef	inListener);

TerminalView_Result
	TerminalView_StopMonitoring				(TerminalViewRef			inRef,
											 TerminalView_Event			inForWhatEvent,
											 ListenerModel_ListenerRef	inListener);

//@}

//!\name Hit Testing
//@{

Boolean
	TerminalView_PtInSelection		(TerminalViewRef	inView,
									 Point				inLocalPoint);

//@}

//!\name Drag and Drop
//@{

void
	TerminalView_SetDragHighlight	(HIViewRef		inView,
									 DragRef		inDrag,
									 Boolean		inIsHighlighted);

//@}

//!\name Managing the Text Selection
//@{

void
	TerminalView_DisplaySaveSelectedTextUI		(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_FindNothing					(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_FindVirtualRange				(TerminalViewRef				inView,
												 TerminalView_CellRange const&	inSelection);

void
	TerminalView_FlashSelection					(TerminalViewRef				inView);

void
	TerminalView_GetSelectedTextAsAudio			(TerminalViewRef				inView);

void
	TerminalView_GetSelectedTextAsVirtualRange	(TerminalViewRef				inView,
												 TerminalView_CellRange&		outSelection);

void
	TerminalView_MakeSelectionsRectangular		(TerminalViewRef				inView,
												 Boolean						inAreSelectionsNotAttachedToScreenEdges);

// INEFFICIENT, USE WITH CARE; LOOK FOR OTHER APIS THAT CAN READ THE SELECTION WITHOUT COPYING IT
Handle
	TerminalView_ReturnSelectedTextAsNewHandle	(TerminalViewRef				inView,
												 UInt16							inNumberOfSpacesToReplaceWithOneTabOrZero,
												 TerminalView_TextFlags			inFlags);

CFStringRef
	TerminalView_ReturnSelectedTextAsNewUnicode	(TerminalViewRef				inView,
												 UInt16							inNumberOfSpacesToReplaceWithOneTabOrZero,
												 TerminalView_TextFlags			inFlags);

RgnHandle
	TerminalView_ReturnSelectedTextAsNewRegion	(TerminalViewRef				inView);

size_t
	TerminalView_ReturnSelectedTextSize			(TerminalViewRef				inView);

void
	TerminalView_SelectBeforeCursorCharacter	(TerminalViewRef				inView);

void
	TerminalView_SelectCursorCharacter			(TerminalViewRef				inView);

void
	TerminalView_SelectCursorLine				(TerminalViewRef				inView);

void
	TerminalView_SelectEntireBuffer				(TerminalViewRef				inView);

void
	TerminalView_SelectMainScreen				(TerminalViewRef				inView);

void
	TerminalView_SelectNothing					(TerminalViewRef				inView);

void
	TerminalView_SelectVirtualRange				(TerminalViewRef				inView,
												 TerminalView_CellRange const&	inSelection);

Boolean
	TerminalView_TextSelectionExists			(TerminalViewRef				inView);

Boolean
	TerminalView_TextSelectionIsRectangular		(TerminalViewRef				inView);

//@}

//!\name Window Management
//@{

HIWindowRef
	TerminalView_ReturnWindow					(TerminalViewRef			inView);

//@}

//!\name Visible Area
//@{

TerminalView_DisplayMode
	TerminalView_ReturnDisplayMode				(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollAround					(TerminalViewRef			inView,
												 SInt16						inColumnCountDelta,
												 SInt16						inRowCountDelta);

TerminalView_Result
	TerminalView_ScrollColumnsTowardLeftEdge	(TerminalViewRef			inView,
												 UInt16						inNumberOfColumnsToScroll);

TerminalView_Result
	TerminalView_ScrollColumnsTowardRightEdge	(TerminalViewRef			inView,
												 UInt16						inNumberOfColumnsToScroll);

TerminalView_Result
	TerminalView_ScrollPixelsTo					(TerminalViewRef			inView,
												 UInt32						inStartOfVerticalRange,
												 UInt32						inStartOfHorizontalRange = 0);

TerminalView_Result
	TerminalView_ScrollRowsTowardBottomEdge		(TerminalViewRef			inView,
												 UInt16						inNumberOfRowsToScroll);

TerminalView_Result
	TerminalView_ScrollRowsTowardTopEdge		(TerminalViewRef			inView,
												 UInt16						inNumberOfRowsToScroll);

TerminalView_Result
	TerminalView_ScrollToBeginning				(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollToEnd					(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_SetDisplayMode					(TerminalViewRef			inView,
												 TerminalView_DisplayMode   inNewMode);

TerminalView_Result
	TerminalView_SetFocusRingDisplayed			(TerminalViewRef			inView,
												 Boolean					inShowFocusRingAndMatte);

//@}

//!\name Cursor Management
//@{

void
	TerminalView_GetCursorGlobalBounds			(TerminalViewRef			inView,
												 HIRect&					outGlobalBounds);

void
	TerminalView_MoveCursorWithArrowKeys		(TerminalViewRef			inView,
												 Point						inLocalMouse);

//@}

//!\name Metrics
//@{

Boolean
	TerminalView_GetIdealSize					(TerminalViewRef			inView,
												 SInt16&					outWidthInPixels,
												 SInt16&					outHeightInPixels);

TerminalView_Result
	TerminalView_GetRange						(TerminalViewRef			inView,
												 TerminalView_RangeCode		inRangeCode,
												 UInt32&					outStartOfRange,
												 UInt32&					outPastEndOfRange);

void
	TerminalView_GetTheoreticalScreenDimensions	(TerminalViewRef			inView,
												 SInt16						inWidthInPixels,
												 SInt16						inHeightInPixels,
												 UInt16*					outColumnCount,
												 UInt16*					outRowCount);

void
	TerminalView_GetTheoreticalViewSize			(TerminalViewRef			inView,
												 UInt16						inColumnCount,
												 UInt16						inRowCount,
												 SInt16*					outWidthInPixels,
												 SInt16*					outHeightInPixels);

 //@}

//!\name Mac OS HIView Management
//@{

void
	TerminalView_FocusForUser					(TerminalViewRef			inView);

HIViewRef
	TerminalView_ReturnContainerHIView			(TerminalViewRef			inView);

HIViewRef
	TerminalView_ReturnDragFocusHIView			(TerminalViewRef			inView);

HIViewRef
	TerminalView_ReturnUserFocusHIView			(TerminalViewRef			inView);

TerminalViewRef
	TerminalView_ReturnUserFocusTerminalView	();

//@}

//!\name Appearance
//@{

Boolean
	TerminalView_GetColor						(TerminalViewRef			inView,
												 TerminalView_ColorIndex	inColorEntryNumber,
												 RGBColor*					outColorPtr);

void
	TerminalView_GetFontAndSize					(TerminalViewRef			inView,
												 StringPtr					outFontFamilyNameOrNull,
												 UInt16*					outFontSizeOrNull);

Preferences_ContextRef
	TerminalView_ReturnConfiguration			(TerminalViewRef			inView);

void
	TerminalView_ReverseVideo					(TerminalViewRef			inView,
												 Boolean					inReverseVideo);

Boolean
	TerminalView_SetColor						(TerminalViewRef			inView,
												 TerminalView_ColorIndex	inColorEntryNumber,
												 RGBColor const*			inColorPtr);

TerminalView_Result
	TerminalView_SetFontAndSize					(TerminalViewRef			inView,
												 ConstStringPtr				inFontFamilyNameOrNull,
												 UInt16						inFontSizeOrZero);

//@}

//!\name State Management
//@{

// THIS ALSO SETS THE STATE OF ANSI GRAPHICS MODE
void
	TerminalView_SetANSIColorsEnabled			(TerminalViewRef			inView,
												 Boolean					inUseANSIColorSequences);

void
	TerminalView_SetDrawingEnabled				(TerminalViewRef			inView,
												 Boolean					inIsDrawingEnabled);

//@}

//!\name Miscellaneous
//@{

void
	TerminalView_DeleteScrollback				(TerminalViewRef					inView);

void
	TerminalView_RotateSearchResultHighlight	(TerminalViewRef					inView,
												 SInt16								inHowFarWhichWay);

Boolean
	TerminalView_SearchResultsExist				(TerminalViewRef					inView);

TerminalView_Result
	TerminalView_TranslateTerminalScreenRange	(TerminalViewRef					inView,
												 Terminal_RangeDescription const&	inRange,
												 TerminalView_CellRange&			outRange);

void
	TerminalView_ZoomToCursor					(TerminalViewRef					inView,
												 Boolean							inQuick = false);

void
	TerminalView_ZoomToSearchResults			(TerminalViewRef					inView);

void
	TerminalView_ZoomToSelection				(TerminalViewRef					inView);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
