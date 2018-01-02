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

	MacTerm
		© 1998-2018 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// standard-C++ includes
#include <utility>
#include <vector>

// Mac includes
#ifdef __OBJC__
#include <AppKit/AppKit.h>
#else
class NSView;
class NSWindow;
#endif
#include <Carbon/Carbon.h>

// library includes
#include "ListenerModel.h"

// application includes
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
Identifiers for the “custom” colors of a terminal view.
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
	kTerminalView_ColorIndexCursorBackground		= 6,
	kTerminalView_ColorIndexMatteBackground			= 7,
	
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
the content either fits or doesn’t fit the pixel area.  However, in
zoom mode, the content is forced to fit in the pixel area; the font
size changes to whatever size makes the content best fit the area.
*/
enum TerminalView_DisplayMode
{
	kTerminalView_DisplayModeNormal		= 'Norm',   //!< underlying terminal screen’s dimensions are altered to
													//!  best suit the pixel dimensions of the view, when resized;
													//!  the terminal view font size is unchanged
	kTerminalView_DisplayModeZoom		= 'Zoom'	//!< font size of text in view is altered to make the current
													//!  rows and columns best fill the screen area, when resized;
													//!  the underlying terminal screen’s dimensions are unchanged
};

/*!
Events in a Terminal View that other modules can register to
receive notification of.
*/
enum TerminalView_Event
{
	kTerminalView_EventFontSizeChanged			= 'FSiz',	//!< the font size used for drawing text has been altered
															//!  (context: TerminalViewRef)
	kTerminalView_EventScrolling				= 'Scrl',	//!< the visible part of the terminal view has changed
															//!  (context: TerminalViewRef)
	kTerminalView_EventSearchResultsExistence	= 'Srch'	//!< the result of TerminalView_SearchResultsExist() is now
															//!  different; this is NOT called if the number of search
															//!  results simply changes (context: TerminalViewRef)
};

typedef UInt16 TerminalView_TextFlags;
enum
{
	kTerminalView_TextFlagInline					= (1 << 0),		//!< strip end-of-line markers?
	kTerminalView_TextFlagLineSeparatorLF 		= (1 << 1),		//!< use LF as line ending (default is CR)
	kTerminalView_TextFlagLastLineHasSeparator 	= (1 << 2)		//!< also add end-of-line to end of text? (default is no)
};

#pragma mark Types

#include "TerminalViewRef.typedef.h"

/*!
This object wraps pixel values to guard against accidental
conversions or other misuse (such as a value in units other
than pixels).  It also stores both the precise and pixel-grid
version of a pixel value, allowing Core Graphics renderings
to retain exact calculation results that cannot be preserved
in legacy QuickDraw views.  QuickDraw support will eventually
be removed.

The storage sizes are also template parameters so that this
can use less space if the pixel range is not expected to be
big (for example, terminal display width versus the entire
pixel range of the terminal scrollback region).
*/
template < typename discrete_type, typename precise_type >
struct TerminalView_PixelValue
{
public:
    explicit TerminalView_PixelValue ()
    : pixels_(0.0)
    {
    }
	
    void
	setIntegralPixels	(discrete_type		inIntegralPixelCount)
    {
		pixels_ = inIntegralPixelCount;
    }
	
    void
	setPrecisePixels	(precise_type		inExactPixelRange)
    {
		pixels_ = inExactPixelRange;
    }
	
	discrete_type
	integralPixels () const
	{
		return STATIC_CAST(pixels_, discrete_type);
	}
	
	precise_type
	precisePixels () const
	{
		return pixels_;
	}

private:
    precise_type	pixels_; // Core Graphics high-precision value
};

typedef TerminalView_PixelValue< SInt16, Float32 >	TerminalView_PixelWidth;
typedef TerminalView_PixelValue< SInt32, CGFloat >	TerminalView_PixelHeight;

/*!
Since a terminal view can have a potentially huge scrollback
buffer, it is important to use the data type below (and not
just some integer) to represent an index for a row.  Note
that a row index is signed because negative values indicate
scrollback rows and positive values indicate screen rows.
*/
typedef SInt32												TerminalView_RowIndex;

typedef std::pair< UInt16, TerminalView_RowIndex >			TerminalView_Cell;		//!< order: column, row

typedef std::pair< TerminalView_Cell, TerminalView_Cell >	TerminalView_CellRange;	//!< order: inclusive start, exclusive end

typedef std::vector< TerminalView_CellRange >				TerminalView_CellRangeList;

#ifdef __OBJC__

/*!
Implements the background rendering part of the
Terminal View.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalView_BackgroundView : NSView //{
{
@private
	size_t	_colorIndex;
	void*	_internalViewPtr;
}
@end //}

/*!
Implements the main rendering part of the Terminal View.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalView_ContentView : NSControl //{
{
@private
	BOOL		_showDragHighlight;
	NSUInteger	_modifierFlagsForCursor;
	void*		_internalViewPtr;
}

// new methods
	- (TerminalViewRef)
	terminalViewRef;

@end //}


/*!
Implements a view controller for the Cocoa version of
the terminal view.  See "TerminalWindowCocoa.xib".

A TerminalViewRef owns this controller, created as a
side effect of TerminalView_NewNSViewBased().

Window elements are handled by TerminalWindow_Controller.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalView_Controller : NSViewController //{
{
	IBOutlet TerminalView_ContentView*		_terminalContentView;
	IBOutlet TerminalView_BackgroundView*	_terminalPaddingView; // should embed the content view
	IBOutlet TerminalView_BackgroundView*	_terminalBackgroundView; // should embed the padding view
}

// accessors
	@property (strong) TerminalView_BackgroundView*
	terminalBackgroundView;
	@property (strong) TerminalView_BackgroundView*
	terminalPaddingView;
	@property (strong) TerminalView_ContentView*
	terminalContentView;

@end //}

#else

class TerminalView_BackgroundView;
class TerminalView_ContentView;

#endif // __OBJC__



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

TerminalViewRef
	TerminalView_NewNSViewBased		(TerminalView_ContentView*		inBaseView,
									 TerminalView_BackgroundView*	inPaddingView,
									 TerminalView_BackgroundView*	inBackgroundView,
									 TerminalScreenRef				inScreenDataSource,
									 Preferences_ContextRef			inFormatOrNull = nullptr);

// AUTOMATICALLY DESTROYED WHEN THE VIEW FROM TerminalView_ReturnContainerHIView() GOES AWAY
TerminalViewRef
	TerminalView_NewHIViewBased		(TerminalScreenRef				inScreenDataSource,
									 Preferences_ContextRef			inFormatOrNull = nullptr);

//@}

//!\name Modifying Terminal View Data
//@{

TerminalView_Result
	TerminalView_AddDataSource				(TerminalViewRef			inView,
											 TerminalScreenRef			inScreenDataSource);

TerminalView_Result
	TerminalView_RemoveDataSource			(TerminalViewRef			inView,
											 TerminalScreenRef			inScreenDataSourceOrNull);

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
	TerminalView_DisplayCompletionsUI			(TerminalViewRef				inView);

void
	TerminalView_DisplaySaveSelectionUI			(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_FindNothing					(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_FindVirtualRange				(TerminalViewRef				inView,
												 TerminalView_CellRange const&	inSelection);

void
	TerminalView_FlashSelection					(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_GetSearchResults				(TerminalViewRef				inView,
												 TerminalView_CellRangeList&	outResults);

void
	TerminalView_GetSelectedTextAsAudio			(TerminalViewRef				inView);

void
	TerminalView_GetSelectedTextAsVirtualRange	(TerminalViewRef				inView,
												 TerminalView_CellRange&		outSelection);

void
	TerminalView_MakeSelectionsRectangular		(TerminalViewRef				inView,
												 Boolean						inAreSelectionsNotAttachedToScreenEdges);

CFStringRef
	TerminalView_ReturnCursorWordCopyAsUnicode	(TerminalViewRef				inView);

CFArrayRef
	TerminalView_ReturnSelectedImageArrayCopy	(TerminalViewRef				inView);

// INEFFICIENT, USE WITH CARE; LOOK FOR OTHER APIS THAT CAN READ THE SELECTION WITHOUT COPYING IT
CFStringRef
	TerminalView_ReturnSelectedTextCopyAsUnicode	(TerminalViewRef				inView,
													 UInt16							inNumberOfSpacesToReplaceWithOneTabOrZero,
													 TerminalView_TextFlags			inFlags);

size_t
	TerminalView_ReturnSelectedTextSize			(TerminalViewRef				inView);

Boolean
	TerminalView_SearchResultsExist				(TerminalViewRef				inView);

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

TerminalView_Result
	TerminalView_SetTextSelectionRenderingEnabled	(TerminalViewRef			inView,
												 Boolean						inIsSelectionEnabled);

Boolean
	TerminalView_TextSelectionExists			(TerminalViewRef				inView);

Boolean
	TerminalView_TextSelectionIsRectangular		(TerminalViewRef				inView);

//@}

//!\name Window Management
//@{

NSWindow*
	TerminalView_ReturnNSWindow					(TerminalViewRef			inView);

HIWindowRef
	TerminalView_ReturnWindow					(TerminalViewRef			inView);

//@}

//!\name Visible Area
//@{

TerminalView_Result
	TerminalView_GetScrollVerticalInfo			(TerminalViewRef			inView,
												 SInt32&					outStartOfRange,
												 SInt32&					outPastEndOfRange,
												 SInt32&					outStartOfMaximum,
												 SInt32&					outPastEndOfMaximum);

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
	TerminalView_ScrollPageTowardBottomEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollPageTowardLeftEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollPageTowardRightEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollPageTowardTopEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollRowsTowardBottomEdge		(TerminalViewRef			inView,
												 UInt32						inNumberOfRowsToScroll);

TerminalView_Result
	TerminalView_ScrollRowsTowardTopEdge		(TerminalViewRef			inView,
												 UInt32						inNumberOfRowsToScroll);

TerminalView_Result
	TerminalView_ScrollToBeginning				(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollToCell					(TerminalViewRef				inView,
												 TerminalView_Cell const&	inCell);

TerminalView_Result
	TerminalView_ScrollToEnd					(TerminalViewRef			inView);

// USE TerminalView_GetScrollVerticalInfo() TO DETERMINE APPROPRIATE VALUES FOR THESE INTEGER RANGES
TerminalView_Result
	TerminalView_ScrollToIndicatorPosition		(TerminalViewRef			inView,
												 SInt32						inStartOfVerticalRange,
												 SInt32						inStartOfHorizontalRange = 0);

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
												 TerminalView_PixelWidth&	outWidthInPixels,
												 TerminalView_PixelHeight&	outHeightInPixels);

void
	TerminalView_GetTheoreticalScreenDimensions	(TerminalViewRef			inView,
												 TerminalView_PixelWidth	inWidthInPixels,
												 TerminalView_PixelHeight	inHeightInPixels,
												 UInt16*					outColumnCount,
												 TerminalView_RowIndex*		outRowCount);

void
	TerminalView_GetTheoreticalViewSize			(TerminalViewRef			inView,
												 UInt16						inColumnCount,
												 TerminalView_RowIndex		inRowCount,
												 TerminalView_PixelWidth&	outWidthInPixels,
												 TerminalView_PixelHeight&	outHeightInPixels);

//@}

//!\name Cocoa NSView and Mac OS HIView Management
//@{

void
	TerminalView_FocusForUser					(TerminalViewRef			inView);

HIViewRef
	TerminalView_ReturnContainerHIView			(TerminalViewRef			inView);

HIViewID
	TerminalView_ReturnContainerHIViewID		();

NSView*
	TerminalView_ReturnContainerNSView			(TerminalViewRef			inView);

HIViewRef
	TerminalView_ReturnDragFocusHIView			(TerminalViewRef			inView);

NSView*
	TerminalView_ReturnDragFocusNSView			(TerminalViewRef			inView);

HIViewRef
	TerminalView_ReturnUserFocusHIView			(TerminalViewRef			inView);

NSView*
	TerminalView_ReturnUserFocusNSView			(TerminalViewRef			inView);

TerminalViewRef
	TerminalView_ReturnUserFocusTerminalView	();

//@}

//!\name Appearance
//@{

Boolean
	TerminalView_GetColor						(TerminalViewRef			inView,
												 TerminalView_ColorIndex	inColorEntryNumber,
												 CGDeviceColor*				outColorPtr);

void
	TerminalView_GetFontAndSize					(TerminalViewRef			inView,
												 CFStringRef*				outFontFamilyNameOrNull,
												 UInt16*					outFontSizeOrNull);

Preferences_ContextRef
	TerminalView_ReturnFormatConfiguration		(TerminalViewRef			inView);

Preferences_ContextRef
	TerminalView_ReturnTranslationConfiguration	(TerminalViewRef			inView);

void
	TerminalView_ReverseVideo					(TerminalViewRef			inView,
												 Boolean					inReverseVideo);

Boolean
	TerminalView_SetColor						(TerminalViewRef			inView,
												 TerminalView_ColorIndex	inColorEntryNumber,
												 CGDeviceColor const*		inColorPtr);

TerminalView_Result
	TerminalView_SetFontAndSize					(TerminalViewRef			inView,
												 CFStringRef				inFontFamilyNameOrNull,
												 Float32					inFontSizeOrZero);

//@}

//!\name State Management
//@{

TerminalView_Result
	TerminalView_SetCursorRenderingEnabled		(TerminalViewRef			inView,
												 Boolean					inIsCursorVisible);

void
	TerminalView_SetDrawingEnabled				(TerminalViewRef			inView,
												 Boolean					inIsDrawingEnabled);

TerminalView_Result
	TerminalView_SetResizeScreenBufferWithView	(TerminalViewRef			inView,
												 Boolean					inScreenDimensionsAutoSync);

TerminalView_Result
	TerminalView_SetUserInteractionEnabled		(TerminalViewRef			inView,
												 Boolean					inIsInteractionEnabled);

//@}

//!\name Miscellaneous
//@{

void
	TerminalView_DeleteScrollback				(TerminalViewRef					inView);

void
	TerminalView_RotateSearchResultHighlight	(TerminalViewRef					inView,
												 SInt16								inHowFarWhichWay);

TerminalView_Result
	TerminalView_TranslateTerminalScreenRange	(TerminalViewRef					inView,
												 Terminal_RangeDescription const&	inRange,
												 TerminalView_CellRange&			outRange);

void
	TerminalView_ZoomOpenFromSelection			(TerminalViewRef					inView);

void
	TerminalView_ZoomToCursor					(TerminalViewRef					inView);

void
	TerminalView_ZoomToSearchResults			(TerminalViewRef					inView);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
