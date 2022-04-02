/*!	\file TerminalView.mm
	\brief The renderer for terminal screen buffers.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#import "TerminalView.h"
#import <UniversalDefines.h>

#define BLINK_MEANS_BLINK	1

// standard-C includes
#import <algorithm>
#import <cctype>
#import <set>
#import <vector>

// Mac includes
#import <Carbon/Carbon.h> // TEMPORARY; some legacy types used below (like EventTime)
#import <CoreImage/CIFilterBuiltins.h>
@import ApplicationServices;
@import Cocoa;
@import CoreServices;

// library includes
#import <AlertMessages.h>
#import <CFRetainRelease.h>
#import <CGContextSaveRestore.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <ContextSensitiveMenu.h>
#import <Console.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>
#import <MenuUtilities.objc++.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// application includes
#import "AppResources.h"
#import "Clipboard.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DragAndDrop.h"
#import "EventLoop.h"
#import "Keypads.h"
#import "MacroManager.h"
#import "Preferences.h"
#import "PrefPanelTranslations.h"
#import "PrintTerminal.h"
#import "QuillsTerminal.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalGlyphDrawing.objc++.h"
#import "TerminalWindow.h"
#import "TextTranslation.h"
#import "UIStrings.h"
#import "URL.h"



#pragma mark Constants
namespace {

CGFloat const	kMy_LargeIBeamMinimumFontSize = 16.0;					//!< mouse I-beam cursor is 32x32 only if the font is at least this size

/*!
Indices into the "colors" array of the main structure.
*/
enum
{
	// consecutive and zero-based
	kMyBasicColorIndexNormalText			= 0,
	kMyBasicColorIndexNormalBackground		= 1,
	kMyBasicColorIndexBoldText				= 2,
	kMyBasicColorIndexBoldBackground		= 3,
	kMyBasicColorIndexBlinkingText			= 4,
	kMyBasicColorIndexBlinkingBackground	= 5,
	kMyBasicColorIndexMatteBackground		= 6,
	kMyBasicColorCount						= 7		//!< set to the number of indices in this list
};

/*!
The number of blink colors is the number of animation stages
used to blink.
*/
UInt16 const		kMy_BlinkingColorCount		= 10;

/*!
Used to break a single-page scroll into animated parts, where
each stage takes approximately this much time (in 60ths of a
second) to appear.
*/
UInt16 const		kMy_PageScrollDelayTicks	= 2;

/*!
Indices into the "coreColors" array of the main structure.
Valid indices range from 0 to 256, and depending on the terminal
configuration the upper limit may CHANGE (typically to 16 colors,
or none at all!).  Check the array size before using it!!!
*/
enum
{
	// the ORDER of the first 16 colors must be the same as the ANSI
	// standard (and XTerm 256-color) values: 0 = black, 1 = red,
	// 2 = green, 3 = yellow, 4 = blue, 5 = magenta, 6 = cyan, 7 = white,
	// and repeated for the emphasized versions
	kTerminalView_ColorIndexNormalANSIBlack			= 0,
	kTerminalView_ColorIndexNormalANSIRed			= 1,
	kTerminalView_ColorIndexNormalANSIGreen			= 2,
	kTerminalView_ColorIndexNormalANSIYellow		= 3,
	kTerminalView_ColorIndexNormalANSIBlue			= 4,
	kTerminalView_ColorIndexNormalANSIMagenta		= 5,
	kTerminalView_ColorIndexNormalANSICyan			= 6,
	kTerminalView_ColorIndexNormalANSIWhite			= 7,
	kTerminalView_ColorIndexEmphasizedANSIBlack		= 8,
	kTerminalView_ColorIndexEmphasizedANSIRed		= 9,
	kTerminalView_ColorIndexEmphasizedANSIGreen		= 10,
	kTerminalView_ColorIndexEmphasizedANSIYellow	= 11,
	kTerminalView_ColorIndexEmphasizedANSIBlue		= 12,
	kTerminalView_ColorIndexEmphasizedANSIMagenta	= 13,
	kTerminalView_ColorIndexEmphasizedANSICyan		= 14,
	kTerminalView_ColorIndexEmphasizedANSIWhite		= 15,
	kMy_CoreColorCount								= 256
};

enum MyCursorState
{
	kMyCursorStateInvisible = 0,
	kMyCursorStateVisible = 1
};

enum My_SelectionExtensionType
{
	kMy_SelectionExtensionTypeDelta			= 0,	//!< extend only by specified changes in columns and/or rows
	kMy_SelectionExtensionTypeLineStart		= 1,	//!< extend to beginning of cursor line
	kMy_SelectionExtensionTypeLineEnd		= 2,	//!< extend to end of cursor line
};

enum My_SelectionMode
{
	kMy_SelectionModeUnset				= 0,	//!< set to this mode when a text selection is first created, no keyboard action yet
	kMy_SelectionModeChangeBeginning	= 1,	//!< keyboard actions morph the beginning anchor only
	kMy_SelectionModeChangeEnd			= 2		//!< keyboard actions morph the end anchor only
};

// used for smooth animations of text and the cursor
Float32 const	kAlphaByPhase[kMy_BlinkingColorCount] =
				{
					// changing alpha values, used for cursor rendering;
					// arbitrary (progression is roughly quadratic to
					// make one end of the animation loop more gradual)
					0.11f, 0.11f, 0.11f, 0.11f, 0.39f,
					0.4725f, 0.56f, 0.6525f, 0.75f, 0.8525f
				};
Float32 const	kBlendingByPhase[kMy_BlinkingColorCount] =
				{
					// percentage of foreground blended with background;
					// arbitrary (progression is roughly quadratic to
					// make one end of the animation loop more gradual)
					0.11f, 0.1725f, 0.24f, 0.3125f, 0.39f,
					0.4725f, 0.56f, 0.6525f, 0.75f, 0.8525f
				};

} // anonymous namespace

#pragma mark Types
namespace {

struct My_PreferenceProxies
{
	Terminal_CursorType		cursorType;
	Boolean					cursorBlinks;
	Boolean					dontDimTerminals;
	Boolean					invertSelections;
	Boolean					notifyOfBeeps;
	UInt16					renderMarginAtColumn; // the value 0 means “no rendering”; column 1 is first column, etc.
};

typedef std::vector< CGFloatRGBColor >			My_CGColorList;
typedef std::map< UInt16, CGFloatRGBColor >		My_CGColorByIndex; // a map is necessary because "vector" cannot handle 256 sequential color structures
typedef std::vector< NSTimeInterval >			My_TimeIntervalList;

class My_XTerm256Table;

// TEMPORARY: This structure is transitioning to C++, and so initialization
// and maintenance of it is downright ugly for the time being.  It *will*
// be simplified and become more object-oriented in the future.
struct My_TerminalView
{
	My_TerminalView		(TerminalView_Object*);
	~My_TerminalView	();
	
	void
	initialize		(TerminalScreenRef, Preferences_ContextRef);
	
	bool
	isCocoa () const;
	
	TerminalViewRef		selfRef;				// redundant reference to self, for convenience
	
	Preferences_ContextWrap		encodingConfig;	// various settings from an external source; not kept up to date, see TerminalView_ReturnTranslationConfiguration()
	Preferences_ContextWrap		formatConfig;	// various settings from an external source; not kept up to date, see TerminalView_ReturnFormatConfiguration()
	std::set< Preferences_Tag >		configFilter;	// settings that this view ignores when they are changed globally by the user
	ListenerModel_Ref	changeListenerModel;	// listeners for various types of changes to this data
	TerminalView_DisplayMode	displayMode;	// how the content fills the display area
	Boolean				isActive;				// true if the view is in an active state, false otherwise; kept in sync
												// using Carbon Events, stored here to avoid extra system calls in cases
												// where this would be slow (e.g. drawing)
	
	TerminalView_Object* __strong	encompassingNSView;		// contains the terminal view hierarchy
	
	struct
	{
		struct
		{
			Boolean					isActive;	// true only if the timer is running
			NSTimer* __strong		objectRef;	// timer to invoke animation procedure periodically; retain in order to invalidate at destruction time
		} timer;
		
		struct
		{
			My_TimeIntervalList		delays;		// duration to wait after each animation stage
			SInt16					stage;		// which color and delay is currently being used
			SInt16					stageDelta;	// +1 or -1, current direction of change
			HIMutableShapeRef		region;		// used to optimize redraws during animation
		} rendering;
		
		struct
		{
			Float32					blinkAlpha;	// between 1.0 (opaque) and 0.0 (transparent), cursor flashing stage
		} cursor;
	} animation;
	
	My_CGColorByIndex	coreColors;			// the (up to) 256 standard colors used, e.g. in color XTerms; base 16 are ANSI colors
	My_CGColorList		customColors;		// the colors currently used to render based on purpose, e.g. normal, bold, blinking, matte
	My_CGColorList		blinkColors;		// an automatically generated set of intermediate colors for blink animation
	My_XTerm256Table*	extendedColorsPtr;	// for storing 256 XTerm colors; globally shared unless a customization command is received
	
	struct
	{
		TerminalScreenRef			ref;					// where the data for this terminal view comes from
		Boolean						sizeNotMatchedWithView;	// if true, screen dimensions of screen buffer do not change when the view is resized
		Boolean						focusRingEnabled;		// is the matte and content area focus ring displayed?
		Boolean						isReverseVideo;			// are foreground and background colors temporarily swapped?
		
		ListenerModel_ListenerWrap	contentMonitor;			// listener for changes to the contents of the screen buffer
		ListenerModel_ListenerWrap	cursorMonitor;			// listener for changes to the terminal cursor position or visible state
		ListenerModel_ListenerWrap	preferenceMonitor;		// listener for changes to preferences that affect a particular view
		ListenerModel_ListenerWrap	bellHandler;			// listener for bell signals from the terminal screen
		ListenerModel_ListenerWrap	videoModeMonitor;		// listener for changes to the reverse-video setting
		
		struct
		{
			HIMutableShapeRef	updatedShape;	// update region for cursor (may or may not correspond to "bounds")
			CGRect				bounds;			// the rectangle of the cursor’s visible region, relative to the content pane!!!
			MyCursorState		currentState;	// whether the cursor is visible
			MyCursorState		ghostState;		// whether the cursor ghost is visible
			Boolean				inhibited;		// if true, the cursor can never be displayed regardless of its state
			Boolean				isCustomColor;	// if true, the cursor color is set by the user instead of being inherited from the screen
		} cursor;
		
		struct
		{
			TerminalView_MousePointerColor	pointerColor;	// customize appearance of mouse cursors (I-beam, etc.)
		} mouse;
		
TerminalView_RowIndex	topVisibleEdgeInRows;	// 0 if scrolled to the main screen, negative if scrollback; do not change this
													//   value directly, use offsetTopVisibleEdge()
		UInt16			leftVisibleEdgeInColumns;	// 0 if leftmost column is visible, positive if content is scrolled to the left;
													//   do not change this value directly, use offsetLeftVisibleEdge()
TerminalView_RowIndex	currentRenderedLine;	// only defined while drawing; the row that is currently being drawn
		Boolean			currentRenderBlinking;		// only defined while drawing; if true, at least one section is blinking
		Boolean			currentRenderDragColors;	// only defined while drawing; if true, drag highlight text colors are used
		Boolean			currentRenderNoBackground;	// only defined while drawing; if true, text is using the ordinary background color
		CGContextRef	currentRenderContext;		// only defined while drawing; if not nullptr, the context from the view draw event
		CGFloat			paddingLeftEmScale;			// left padding between text and focus ring; multiplies against normal (undoubled) character width
		CGFloat			paddingRightEmScale;		// right padding between text and focus ring; multiplies against normal (undoubled) character width
		CGFloat			paddingTopEmScale;			// top padding between text and focus ring; multiplies against normal (undoubled) character height
		CGFloat			paddingBottomEmScale;		// bottom padding between text and focus ring; multiplies against normal (undoubled) character height
		CGFloat			marginLeftEmScale;			// left margin between focus ring and container edge; multiplies against normal (undoubled) character width
		CGFloat			marginRightEmScale;			// right margin between focus ring and container edge; multiplies against normal (undoubled) character width
		CGFloat			marginTopEmScale;			// top margin between focus ring and container edge; multiplies against normal (undoubled) character height
		CGFloat			marginBottomEmScale;		// bottom margin between focus ring and container edge; multiplies against normal (undoubled) character height
		
		struct
		{
			// these settings should only ever be modified by recalculateCachedDimensions(),
			// and that routine should be called when any dependent factor, such as font,
			// is changed; see that routine’s documentation for more information
			TerminalView_PixelWidth		viewWidthInPixels;		// size of window view (window could be smaller than the screen size);
			TerminalView_PixelHeight	viewHeightInPixels;		//   always identical to the current dimensions of the content view
			UInt16						columnCountAtLastNotify;	// captures last screen column count that triggered notification, to minimize traffic
			UInt16						rowCountAtLastNotify;	// captures last screen row count that triggered notification, to minimize traffic
		} cache;
	} screen;
	
	struct
	{
		TextAttributes_Object			attributes;		// current text attribute flags, affecting color of terminal text, etc.
		NSMutableDictionary* __strong	attributeDict;	// most recent equivalent attributed-string attributes (e.g. fonts, colors)
		
		struct
		{
			NSFont* __strong	normalFont;		// font for most text; also represents current family, size and metrics (Cocoa terminals)
			NSFont* __strong	boldFont;		// alternate font for bold-weighted text (might match "normalFont" if no special font is found)
			CFRetainRelease		familyName;		// CFStringRef; font name (as might appear in a Font menu)
			struct Metrics
			{
				CGFloat		baseLine;		// starting point for ascenders, relative to the bottom of the cell
				CGFloat		size;			// point size of text written in the font indicated by "familyName"
			} normalMetrics;	// metrics for text meant to fit in a single cell (normal)
			CGFloat			scaleWidthPerCell;	// a multiplier (normally 1.0) to force characters from the font to fit in a different width
TerminalView_PixelWidth		baseWidthPerCell;	// value of "widthPerCell" without any scaling applied
TerminalView_PixelWidth		widthPerCell;	// number of pixels wide each character is (multiply by 2 on double-width lines);
												// generally, you should call getRowCharacterWidth() instead of referencing this!
TerminalView_PixelHeight	heightPerCell;	// number of pixels high each character is (multiply by 2 if double-height text)
			CGFloat			thicknessHorizontalLines;	// thickness of non-bold horizontal lines in manually-drawn glyphs; diagonal lines may use based on angle
			CGFloat			thicknessHorizontalBold;	// thickness of bold horizontal lines in manually-drawn glyphs; diagonal lines may use based on angle
			CGFloat			thicknessVerticalLines;		// thickness of non-bold vertical lines in manually-drawn glyphs; diagonal lines may use based on angle
			CGFloat			thicknessVerticalBold;		// thickness of bold vertical lines in manually-drawn glyphs; diagonal lines may use based on angle
		} font;
		
		CGFloatRGBColor		colors[kMyBasicColorCount];	// indices are "kMyBasicColorIndexNormalText", etc.
		
		struct
		{
			TerminalView_CellRange		range;			// region of text selection
			TerminalView_Cell			clickCell;		// for extending selections with the mouse
			My_SelectionMode			keyboardMode;	// used for keyboard navigation; determines what is changed by keyboard-select actions
			Boolean						isRectangular;	// is the text selection unattached from the left and right screen edges?
			Boolean						readOnly;		// does the view respond to clicks and keystrokes that affect text selections?
			Boolean						inhibited;		// does the view refuse to highlight or manage selections even when API calls are made?
		} selection;
		
		TerminalView_CellRangeList				searchResults;			// regions matching the most recent Find results
		TerminalView_CellRangeList::iterator	toCurrentSearchResult;	// most recently focused match; MUST change if "searchResults" changes
	} text;
};
typedef My_TerminalView*		My_TerminalViewPtr;
typedef My_TerminalView const*	My_TerminalViewConstPtr;

typedef MemoryBlockPtrLocker< TerminalViewRef, My_TerminalView >	My_TerminalViewPtrLocker;
typedef LockAcquireRelease< TerminalViewRef, My_TerminalView >		My_TerminalViewAutoLocker;

/*!
Calculates mappings between XTerm encoded color values and
the roughly 256 equivalent RGB triplets or gray scales.
*/
class My_XTerm256Table
{
public:
	typedef std::vector< UInt8 >			RGBLevels;
	typedef std::map< UInt8, RGBLevels >	RGBLevelsByIndex;
	typedef std::map< UInt8, UInt8 >		GrayLevelByIndex;
	
	My_XTerm256Table ();
	
	static void
	makeCGFloatRGBColor		(UInt8, UInt8, UInt8, CGFloatRGBColor&);
	
	void
	setColor	(UInt8, UInt8, UInt8, UInt8);
	
	GrayLevelByIndex	grayLevels;
	RGBLevelsByIndex	colorLevels;
};

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean				addDataSource						(My_TerminalViewPtr, TerminalScreenRef);
void				animateBlinkingItems				(NSTimer*, TerminalViewRef);
void				audioEvent							(ListenerModel_Ref, ListenerModel_Event, void*, void*);
NSTimeInterval		calculateAnimationStageDelay		(My_TerminalViewPtr, My_TimeIntervalList::size_type);
UInt16				copyColorPreferences				(My_TerminalViewPtr, Preferences_ContextRef, Boolean);
UInt16				copyFontPreferences					(My_TerminalViewPtr, Preferences_ContextRef, Boolean);
void				copySelectedTextIfUserPreference	(My_TerminalViewPtr);
void				copyTranslationPreferences			(My_TerminalViewPtr, Preferences_ContextRef);
Boolean				createWindowColorPalette			(My_TerminalViewPtr, Preferences_ContextRef, Boolean = true);
Boolean				cursorBlinks						(My_TerminalViewPtr);
Terminal_CursorType	cursorType							(My_TerminalViewPtr);
NSCursor*			customCursorCrosshairs				(My_TerminalViewPtr);
NSCursor*			customCursorIBeam					(My_TerminalViewPtr, Boolean = false);
NSCursor*			customCursorMoveTerminalCursor		(My_TerminalViewPtr, Boolean = false);
void				delayMinimumTicks					(UInt16 = 8);
void				drawSingleColorImage				(CGContextRef, CGColorRef, CGRect, id);
void				drawSingleColorPattern				(CGContextRef, CGColorRef, CGRect, id);
Boolean				drawSection							(My_TerminalViewPtr, CGContextRef, UInt16, TerminalView_RowIndex,
														 UInt16, TerminalView_RowIndex);
void				drawTerminalScreenRunOp				(My_TerminalViewPtr, UInt16, CFStringRef, UInt16, TextAttributes_Object);
void				drawTerminalText					(My_TerminalViewPtr, CGContextRef, CGRect const&, CFIndex,
														 CFStringRef, TextAttributes_Object);
void				drawVTGraphicsGlyph					(My_TerminalViewPtr, CGContextRef, CGRect const&, UnicodeScalarValue,
														 CGFloat, TextAttributes_Object);
void				eraseSection						(My_TerminalViewPtr, CGContextRef, SInt16, SInt16, CGRect&);
void				eventNotifyForView					(My_TerminalViewConstPtr, TerminalView_Event, void*);
Terminal_LineRef	findRowIterator						(My_TerminalViewPtr, TerminalView_RowIndex, Terminal_LineStackStorage*);
Terminal_LineRef	findRowIteratorRelativeTo			(My_TerminalViewPtr, TerminalView_RowIndex, TerminalView_RowIndex,
														 Terminal_LineStackStorage*);
Boolean				findVirtualCellFromScreenPoint		(My_TerminalViewPtr, HIPoint, TerminalView_Cell&, SInt16* = nullptr, SInt16* = nullptr);
void				getBlinkAnimationColor				(My_TerminalViewPtr, UInt16, CGFloatRGBColor*);
void				getImagesInVirtualRange				(My_TerminalViewPtr, TerminalView_CellRange const&, NSMutableArray*);
void				getRowBounds						(My_TerminalViewPtr, TerminalView_RowIndex, CGRect&);
TerminalView_PixelWidth		getRowCharacterWidth		(My_TerminalViewPtr, TerminalView_RowIndex);
void				getRowSectionBounds					(My_TerminalViewPtr, TerminalView_RowIndex, UInt16, SInt16, CGRect&);
void				getScreenBaseColor					(My_TerminalViewPtr, TerminalView_ColorIndex, CGFloatRGBColor*);
void				getScreenColorsForAttributes		(My_TerminalViewPtr, TextAttributes_Object, CGFloatRGBColor*, CGFloatRGBColor*, Boolean*);
Boolean				getScreenCoreColor					(My_TerminalViewPtr, UInt16, CGFloatRGBColor*);
void				getScreenCustomColor				(My_TerminalViewPtr, TerminalView_ColorIndex, CGFloatRGBColor*);
HIShapeRef			getSelectedTextAsNewHIShape			(My_TerminalViewPtr, Float32 = 0.0);
size_t				getSelectedTextSize					(My_TerminalViewPtr);
HIShapeRef			getVirtualRangeAsNewHIShape			(My_TerminalViewPtr, TerminalView_Cell const&, TerminalView_Cell const&,
														 Float32, Boolean);
void				getVirtualVisibleRegion				(My_TerminalViewPtr, UInt16*, TerminalView_RowIndex*, UInt16*, TerminalView_RowIndex*);
void				handleMultiClick					(My_TerminalViewPtr, UInt16);
void				highlightCurrentSelection			(My_TerminalViewPtr, Boolean, Boolean);
void				highlightVirtualRange				(My_TerminalViewPtr, TerminalView_CellRange const&, TextAttributes_Object,
														 Boolean, Boolean);
void				invalidateRowSection				(My_TerminalViewPtr, TerminalView_RowIndex, UInt16, UInt16);
Boolean				isSmallIBeam						(My_TerminalViewPtr);
TerminalView_MousePointerColor	mousePointerColor		(My_TerminalViewPtr);
void				offsetLeftVisibleEdge				(My_TerminalViewPtr, SInt16);
void				offsetTopVisibleEdge				(My_TerminalViewPtr, SInt64);
Boolean				pointInSelection					(My_TerminalViewPtr, TerminalView_Cell const&);
void				populateContextualMenu				(My_TerminalViewPtr, NSMenu*);
void				preferenceChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				preferenceChangedForView			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				recalculateCachedDimensions			(My_TerminalViewPtr);
void				receiveVideoModeChange				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				releaseRowIterator					(My_TerminalViewPtr, Terminal_LineRef*);
Boolean				removeDataSource					(My_TerminalViewPtr, TerminalScreenRef);
CFStringRef			returnSelectedTextCopyAsUnicode		(My_TerminalViewPtr, UInt16, TerminalView_TextFlags);
void				screenBufferChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				screenCursorChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
Boolean				selectionExists						(My_TerminalViewPtr);
void				selectionModify						(My_TerminalViewPtr, SInt16, SInt16, My_SelectionExtensionType, Boolean = true);
void				setBlinkAnimationColor				(My_TerminalViewPtr, UInt16, CGFloatRGBColor const*);
void				setBlinkingTimerActive				(My_TerminalViewPtr, Boolean);
void				setCursorVisibility					(My_TerminalViewPtr, Boolean);
void				setFontAndSize						(My_TerminalViewPtr, CFStringRef, CGFloat, Float32 = 0, Boolean = true);
void				setScreenBaseColor					(My_TerminalViewPtr, TerminalView_ColorIndex, CGFloatRGBColor const*);
void				setScreenCoreColor					(My_TerminalViewPtr, UInt16, CGFloatRGBColor const*);
void				setScreenCustomColor				(My_TerminalViewPtr, TerminalView_ColorIndex, CGFloatRGBColor const*);
void				setTextAttributesDictionary			(My_TerminalViewPtr, NSMutableDictionary*, TextAttributes_Object, Boolean = false);
void				setUpCursorBounds					(My_TerminalViewPtr, SInt16, SInt16, CGRect*, HIMutableShapeRef);
void				setUpScreenFontMetrics				(My_TerminalViewPtr);
void				sortAnchors							(TerminalView_Cell&, TerminalView_Cell&, Boolean);
Boolean				startMonitoringDataSource			(My_TerminalViewPtr, TerminalScreenRef);
Boolean				stopMonitoringDataSource			(My_TerminalViewPtr, TerminalScreenRef);
void				updateDisplay						(My_TerminalViewPtr);
void				updateDisplayInShape				(My_TerminalViewPtr, HIShapeRef);
OSStatus			updateDisplayInShapeSubRect			(int, HIShapeRef, CGRect const*, void*);
void				useTerminalTextColors				(My_TerminalViewPtr, CGContextRef, TextAttributes_Object, Boolean, Float32 = 1.0);
void				visualBell							(My_TerminalViewPtr);

} // anonymous namespace


/*!
Private properties.
*/
@interface TerminalView_BackgroundView () //{

// accessors
	//! Zero-based index into the "text.colors" array of a
	//! Terminal View structure, specifying the matte color
	//! to use for rendering.
	@property (assign) size_t
	colorIndex; // a "kTerminalView_ColorIndex..." constant
	//! Internal version of associated TerminalViewRef.
	@property (assign) My_TerminalViewPtr
	internalViewPtr; // weak
	//! Stores information on key-value observers.
	@property (strong) NSMutableArray*
	registeredObservers;

@end //}


/*!
The private class interface.
*/
@interface TerminalView_BackgroundView (TerminalView_BackgroundViewInternal) //{

@end //}


/*!
Private properties.
*/
@interface TerminalView_ContentView () //{

// accessors
	//! Set only during drags, to keep track of window activation
	//! (background window can auto-activate from hovering drag).
	@property (assign) BOOL
	didDragActivateWindow;
	//! Set only during drags, to keep track of time elapsed when
	//! drag is hovering over background window.  If a drag has
	//! hovered for long enough, the window auto-activates.
	@property (assign) CFAbsoluteTime
	dragEnterTime;
	//! Tracks the mouse even in background windows so that the
	//! window can automatically become “key” if the user preference
	//! for “focus follows mouse” is set.
	@property (strong) NSTrackingArea*
	focusFollowsMouseTrackingArea;
	//! Auto-updated when the corresponding user preference changes.
	@property (assign) BOOL
	focusShouldFollowMouse;
	//! Internal version of associated TerminalViewRef.
	@property (assign) My_TerminalViewPtr
	internalViewPtr; // weak
	//! Set only when menu tracking ends, to keep track of time
	//! elapsed when later handling focus-follows-mouse.  If a
	//! menu item was just handled, focus-follows-mouse is
	//! delayed to prevent a selected command from inadvertently
	//! applying to the unintended window.
	@property (assign) CFAbsoluteTime
	menuEndTrackingTime;
	//! Current state of modifier keys, used to set an appropriate
	//! cursor (that should be consistent with whatever action
	//! would be performed by clicking or dragging with the same
	//! modifier keys pressed).
	@property (assign) NSUInteger
	modifierFlagsForCursor;
	//! Responds when underlying preference storage is changed.
	@property (strong) ListenerModel_StandardListener*
	preferenceChangeListener;
	//! Stores information on key-value observers.
	@property (strong) NSMutableArray*
	registeredObservers;
	//! If set to YES, the background and text rendering is changed
	//! to show that a drag-drop is pending.
	@property (assign) BOOL
	showDragHighlight;
	//! If set to YES, the background and text rendering is changed
	//! to show that a bell sound was triggered.  (Typically this
	//! is immediately turned off afterward.)
	@property (assign) BOOL
	showVisualBell;

@end //}


/*!
The private class interface.
*/
@interface TerminalView_ContentView (TerminalView_ContentViewInternal) //{

// new methods
	- (BOOL)
	assessIsDragForSingleButtonMouseDownEvent:(NSEvent*)_;
	- (SessionRef)
	boundSession;
	- (void)
	extendSelectionForKeyBindingSelector:(SEL)_
	sender:(id)_;
	- (void)
	model:(ListenerModel_Ref)_
	preferenceChange:(ListenerModel_Event)_
	context:(void*)_;
	- (void)
	moveCursorForKeyBindingSelector:(SEL)_
	sender:(id)_;
	- (void)
	printForSelector:(SEL)_
	sender:(id)_;
	- (void)
	setFocusFollowsMouseTrackingAreasEnabled:(BOOL)_;

@end //}


/*!
Private properties.
*/
@interface TerminalView_Controller () //{

@end //}


/*!
Private properties.
*/
@interface TerminalView_Object () //{

// accessors
	//! Internal version of associated TerminalViewRef.
	@property (assign) My_TerminalViewPtr
	internalViewPtr; // weak

@end //}


/*!
The private class interface.
*/
@interface TerminalView_Object (TerminalView_ObjectInternal) //{

// new methods
	- (void)
	createSubviews;

@end //}


/*!
Private properties.
*/
@interface TerminalView_ScrollBar () //{

// accessors
	//! Internal version of associated TerminalViewRef.
	@property (assign) My_TerminalViewPtr
	internalViewPtr; // weak

@end //}


/*!
The private class interface.
*/
@interface TerminalView_ScrollBar (TerminalView_ScrollBarInternal) //{

@end //}


/*!
Private properties.
*/
@interface TerminalView_ScrollableRootView () //{

// accessors
	//! Internal version of associated TerminalViewRef.
	@property (assign) My_TerminalViewPtr
	internalViewPtr; // weak

@end //}


/*!
The private class interface.
*/
@interface TerminalView_ScrollableRootView (TerminalView_ScrollableRootViewInternal) //{

// new methods
	- (void)
	createSubviews;

@end //}


/*!
Private properties.
*/
@interface TerminalView_ScrollableRootVC () //{

// accessors
	//! The terminal views that are operated by this view controller’s
	//! scroll views.  Also determines the source of annotations in
	//! scroll bars (for example, tick bars during a text search).
	@property (strong) NSMutableArray*
	terminalViewControllers;

@end //}


/*!
The private class interface.
*/
@interface TerminalView_ScrollableRootVC (TerminalView_ScrollableRootVCInternal) //{

@end //}

#pragma mark Variables
namespace {

ListenerModel_ListenerRef	gPreferenceChangeEventListener = nullptr;
struct My_PreferenceProxies	gPreferenceProxies;
Boolean						gTerminalViewInitialized = false;
My_TerminalViewPtrLocker&	gTerminalViewPtrLocks ()				{ static My_TerminalViewPtrLocker x; return x; }
HIMutableShapeRef			gInvalidationScratchRegion ()			{ static HIMutableShapeRef x = HIShapeCreateMutable(); assert(nullptr != x); return x; }
My_XTerm256Table&			gColorGrid ()							{ static My_XTerm256Table x; return x; }

} // anonymous namespace


#pragma mark Public Methods

/*!
Call this routine once, before any other method
from this module.

(2.6)
*/
void
TerminalView_Init ()
{
	// set up a callback to receive preference change notifications
	gPreferenceChangeEventListener = ListenerModel_NewStandardListener(preferenceChanged);
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagCursorBlinks,
													true/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up global monitor for cursor-blink setting, error", prefsResult);
		}
		prefsResult = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagNotifyOfBeeps,
													true/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up global monitor for notify-on-bell setting, error", prefsResult);
		}
		prefsResult = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagPureInverse,
													true/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up global monitor for invert-selected-text setting, error", prefsResult);
		}
		prefsResult = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagTerminalCursorType,
													true/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up global monitor for cursor-shape setting, error", prefsResult);
		}
		prefsResult = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagTerminalShowMarginAtColumn,
													true/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up global monitor for show-margin-line setting, error", prefsResult);
		}
	}
	
	gTerminalViewInitialized = true;
}// Init


/*!
Call this routine when you are finished using
the Terminal View module.

(2.6)
*/
void
TerminalView_Done ()
{
	gTerminalViewInitialized = false;
	
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagCursorBlinks);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagNotifyOfBeeps);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagPureInverse);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagTerminalCursorType);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagTerminalShowMarginAtColumn);
	ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
}// Done


/*!
Creates a new NSView* hierarchy for a terminal view, complete
with all the methods and data necessary to drive it.  The
specified base view is used as the root.

The font, colors, etc. are based on the default preferences
context, where any settings in "inFormatOrNull" will override
the defaults.  You can access this configuration later with
TerminalView_ReturnFormatConfiguration().

If any problems occur, nullptr is returned; otherwise, a
reference to the data associated with the new view is returned.
(Use TerminalView_ReturnContainerNSView() or
TerminalView_ReturnUserFocusNSView() to get at parts of the
actual NSView* hierarchy.)

(4.0)
*/
TerminalViewRef
TerminalView_NewNSViewBased		(TerminalView_Object*		inRootView,
								 TerminalScreenRef			inScreenDataSource,
								 Preferences_ContextRef		inFormatOrNull)
{
	TerminalViewRef		result = nullptr;
	
	
	assert(nullptr != inScreenDataSource);
	assert(gTerminalViewInitialized);
	
	try
	{
		My_TerminalViewPtr		viewPtr = new My_TerminalView(inRootView);
		
		
		assert(nullptr != viewPtr);
		result = REINTERPRET_CAST(viewPtr, TerminalViewRef);
	}
	catch (std::exception const& e)
	{
		Console_Warning(Console_WriteValueCString, "exception caught while creating a terminal view structure", e.what());
	}
	
	// set up the associated terminal view structure
	if (nullptr != result)
	{
		My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), result);
		Preferences_ContextWrap		viewFormat(inFormatOrNull, Preferences_ContextWrap::kNotYetRetained);
		
		
		// get the terminal format; if not found, use the default
		if (false == viewFormat.exists())
		{
			// set default
			Preferences_Result		prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef	defaultContext = nullptr;
			
			
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::FORMAT);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "failed to find default context for new view");
			}
			else
			{
				viewFormat.setWithNoRetain(Preferences_NewCloneContext(defaultContext, true/* force detach */));
			}
		}
		
		// finally, initialize the view properly
		viewPtr->initialize(inScreenDataSource, viewFormat.returnRef());
	}
	
	return result;
}// NewNSViewBased


/*!
Specifies a terminal buffer whose data will be displayed
by the terminal view, and starts monitoring it for changes
that would affect the display.

Currently only one data source can be set at a time; to
remove a previous one, call TerminalView_RemoveDataSource().

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

\retval kTerminalView_ResultParameterError
if the data source cannot be changed

(4.1)
*/
TerminalView_Result
TerminalView_AddDataSource	(TerminalViewRef		inView,
							 TerminalScreenRef		inScreenDataSource)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		if (false == addDataSource(viewPtr, inScreenDataSource))
		{
			result = kTerminalView_ResultParameterError;
		}
		else
		{
			if (false == startMonitoringDataSource(viewPtr, inScreenDataSource))
			{
				result = kTerminalView_ResultParameterError;
			}
			else
			{
				// success!
				result = kTerminalView_ResultOK;
			}
		}
	}
	
	return result;
}// AddDataSource


/*!
Erases all the scrollback lines in the underlying
terminal buffer, and updates the display appropriately.
This triggers a "kTerminalView_EventScrolling" event
so that listeners can respond (for instance, to clear
a scroll bar that represents the view region).

(3.1)
*/
void
TerminalView_DeleteScrollback	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	Terminal_DeleteAllSavedLines(viewPtr->screen.ref);
    eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
}// DeleteScrollback


/*!
Displays a pop-up showing possible completions for the
word under the cursor, or informs the user that there are
no completions available.  If the user selects a completion,
it is “typed” into the terminal’s listening session.

(4.1)
*/
void
TerminalView_DisplayCompletionsUI	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						areCompletionsAvailable = false;
	
	
	if (nullptr != viewPtr)
	{
		TerminalView_CellRange const	kOldSelectionRange = viewPtr->text.selection.range;
		CFRetainRelease					searchQueryCFString;
		
		
		// if text is selected, assume it is the text to complete;
		// otherwise, automatically find the word around the cursor
		// (using the same rules as double-clicking)
		if (false == selectionExists(viewPtr))
		{
			searchQueryCFString.setWithNoRetain(TerminalView_ReturnCursorWordCopyAsUnicode(inView));
		}
		else
		{
			// use current selection as a base
			searchQueryCFString.setWithNoRetain(TerminalView_ReturnSelectedTextCopyAsUnicode
												(inView, 0/* spaces to replace with tab */, 0/* flags */));
		}
		
		if (false == searchQueryCFString.exists())
		{
			Console_Warning(Console_WriteLine, "unable to find a suitable query word for auto-completion");
		}
		else
		{
			// find all matches for the selected word
			std::vector< Terminal_RangeDescription >	searchResults;
			Terminal_SearchFlags						flags = 0;
			Terminal_Result								searchStatus = kTerminal_ResultOK;
			
			
			// make a mutable copy and strip any end whitespace
			searchQueryCFString = CFRetainRelease(CFStringCreateMutableCopy(kCFAllocatorDefault, 0/* maximum length, or zero */,
																			searchQueryCFString.returnCFStringRef()),
													CFRetainRelease::kAlreadyRetained);
			CFStringTrimWhitespace(searchQueryCFString.returnCFMutableStringRef());
			
			// initiate search for base term; then, at every search
			// result location, perform a word search to determine
			// the possible completions
			if (0 == CFStringGetLength(searchQueryCFString.returnCFStringRef()))
			{
				Console_Warning(Console_WriteLine, "query word for auto-completion is effectively empty");
			}
			else
			{
				Boolean		searchOK = false;
				
				
				// configure search
				//flags |= kTerminal_SearchFlagsCaseSensitive; // for completions, be case-insensitive
				
				// initiate synchronous (should it be asynchronous?) search
				searchStatus = Terminal_Search(viewPtr->screen.ref, searchQueryCFString.returnCFStringRef(), flags, searchResults);
				if (kTerminal_ResultOK != searchStatus)
				{
					Console_Warning(Console_WriteValue, "search for completions failed, error", searchStatus);
					searchOK = false;
				}
				else
				{
					if (false == searchResults.empty())
					{
						NSMutableSet*	setOfCompletions = [[NSMutableSet alloc] init];
						CGRect			screenRelativeCursorBounds; // set below
						
						
						// find all the “words” at the locations where the
						// starting word fragment were found in the terminal
						// (scrollback or screen), discarding duplicates
						for (auto resultRange: searchResults)
						{
							CFRetainRelease		completionCFString;
							
							
							// “select” this result
							viewPtr->text.selection.range.first.first = resultRange.firstColumn;
							viewPtr->text.selection.range.first.second = resultRange.firstRow;
							viewPtr->text.selection.range.second.first = resultRange.firstColumn + resultRange.columnCount;
							viewPtr->text.selection.range.second.second = resultRange.firstRow + 1;
							
							// “double-click” this result to find a word (TEMPORARY; should
							// probably factor out the code that searches for a word so that
							// the text selection does not have to change; although, this is
							// a very convenient way to produce exactly the right behavior)
							handleMultiClick(viewPtr, 2);
							completionCFString = CFRetainRelease(TerminalView_ReturnSelectedTextCopyAsUnicode
																	(inView, 0/* spaces to replace with tab */, 0/* flags */),
																	CFRetainRelease::kAlreadyRetained);
							TerminalView_SelectNothing(inView); // fix any highlighting changes caused by the “selection” above
							if (false == completionCFString.exists())
							{
								Console_Warning(Console_WriteValuePair, "failed to find a string for completion location",
												resultRange.firstColumn, resultRange.firstRow);
							}
							else
							{
								NSString*	asNSString = BRIDGE_CAST(completionCFString.returnCFStringRef(), NSString*);
								
								
								[setOfCompletions addObject:asNSString];
							}
						}
						
						// find global cursor location so that the completions list can appear near the cursor
						{
							TerminalView_Result		viewResult = kTerminalView_ResultOK;
							CGRect					windowRelativeCursorBounds;
							
							
							viewResult = TerminalView_GetCursorBoundsWindowRelative(inView, windowRelativeCursorBounds);
							if (kTerminalView_ResultOK != viewResult)
							{
								Console_Warning(Console_WriteLine, "completions list display: unable to find window-relative terminal cursor coordinates");
								windowRelativeCursorBounds = NSMakeRect(32, 10, 0, 0); // arbitrary fallback
							}
							
							screenRelativeCursorBounds = [TerminalView_ReturnNSWindow(inView) convertRectToScreen:windowRelativeCursorBounds];
						}
						
						// if the resulting set has exactly one string that
						// matches the original string, ignore it; otherwise,
						// pop up a menu with completion options
						if ((setOfCompletions.count > 1) ||
							(NO == [[setOfCompletions anyObject]
									isEqualToString:BRIDGE_CAST(searchQueryCFString.returnCFStringRef(), NSString*)]))
						{
							NSPoint				globalLocation = NSMakePoint(screenRelativeCursorBounds.origin.x, screenRelativeCursorBounds.origin.y);
							NSMutableArray*		sortedCompletions = [[NSMutableArray alloc] initWithCapacity:setOfCompletions.count];
							NSMenu*				completionsMenu = [[NSMenu alloc] initWithTitle:@""];
							NSUInteger			completionIndex = 0;
							
							
							// sort the (unique) list of strings
							for (NSString* completionOption in setOfCompletions)
							{
								[sortedCompletions addObject:completionOption];
							}
							[sortedCompletions sortUsingSelector:@selector(caseInsensitiveCompare:)];
							
							// quick debug
							//NSLog(@"completions for “%@”: %@", BRIDGE_CAST(searchQueryCFString.returnCFStringRef(), NSString*),
							//		sortedCompletions);
							
							// now create menu items showing completions; the first
							// few options will be given convenient key equivalents
							for (NSString* completionOption in sortedCompletions)
							{
								NSString*		keyEquivalentString = (completionIndex < 10)
																		? [[NSNumber numberWithUnsignedInteger:completionIndex]
																			stringValue]
																		: @"";
								NSMenuItem*		newItem = [[NSMenuItem alloc]
															initWithTitle:completionOption
																			action:@selector(performSendMenuItemText:)
																			keyEquivalent:keyEquivalentString];
								
								
								[completionsMenu addItem:newItem];
								++completionIndex;
							}
							
							// display the menu; note that this mechanism does not require
							// either a positioning item or a view, effectively making the
							// menu appear at the given global location
							UNUSED_RETURN(BOOL)[completionsMenu popUpMenuPositioningItem:nil atLocation:globalLocation inView:nil];
							
							// success!
							areCompletionsAvailable = true;
						}
					}
				}
			}
		}
		
		// restore old selection
		TerminalView_SelectVirtualRange(inView, kOldSelectionRange);
	}
	
	unless (areCompletionsAvailable)
	{
		// failed to find any auto-completions
		// TEMPORARY: could still pop up a menu with a simple
		// disabled-item message like “No completions available.”
		Sound_StandardAlert();
	}
}// DisplayCompletionsUI


/*!
Displays a dialog allowing the user to choose a destination
file, and then writes the selected data to that file (either
a range of text or bitmap images that cross the selected
region).  If nothing is selected, no user interface appears.

This call returns immediately.  The save will only occur when
the user eventually commits the sheet that appears.  The user
can also cancel the operation.

A future version of this function could allow flags to be
specified to affect the capture “rules” (e.g. whether or not
to substitute tabs for consecutive spaces).

(3.1)
*/
void
TerminalView_DisplaySaveSelectionUI		(TerminalViewRef	inView)
{
	if (TerminalView_TextSelectionExists(inView))
	{
		My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
		NSMutableArray*				selectedImages = [[NSMutableArray alloc] init];
		NSSavePanel*				savePanel = [NSSavePanel savePanel];
		
		
		getImagesInVirtualRange(viewPtr, viewPtr->text.selection.range, selectedImages);
		if (selectedImages.count > 0)
		{
			// currently, only a single image can be saved
			if (1 == selectedImages.count)
			{
				NSImage*			imageObject = [selectedImages objectAtIndex:0];
				assert([imageObject isKindOfClass:NSImage.class]);
				CFRetainRelease		saveFileCFString(UIStrings_ReturnCopy(kUIStrings_FileDefaultImageFile),
														CFRetainRelease::kAlreadyRetained);
				CFRetainRelease		promptCFString(UIStrings_ReturnCopy(kUIStrings_SystemDialogPromptSaveSelectedImage),
													CFRetainRelease::kAlreadyRetained);
				
				
				savePanel.message = BRIDGE_CAST(promptCFString.returnCFStringRef(), NSString*);
				savePanel.directoryURL = nil;
				savePanel.nameFieldStringValue = BRIDGE_CAST(saveFileCFString.returnCFStringRef(), NSString*);
				savePanel.allowedFileTypes = @[@"png", @"tiff", @"bmp", @"gif", @"jpg", @"jpeg"];
				[savePanel beginSheetModalForWindow:TerminalView_ReturnNSWindow(inView)
							completionHandler:^(NSInteger aReturnCode)
							{
								if (NSModalResponseOK == aReturnCode)
								{
									// determine a bitmap representation that is consistent with the file name
									NSData*					imageData = [imageObject TIFFRepresentation];
									NSBitmapImageRep*		imageRep = [NSBitmapImageRep imageRepWithData:imageData];
									NSDictionary*			propertyDict = @{
																				NSImageCompressionFactor: @(1.0)
																			};
									NSBitmapImageFileType	imageFileType = NSBitmapImageFileTypePNG;
									NSError*				error = nil;
									
									
									// TEMPORARY; there is probably a better way to do this...
									if ([savePanel.URL.path hasSuffix:@"tiff"])
									{
										imageFileType = NSBitmapImageFileTypeTIFF;
									}
									else if ([savePanel.URL.path hasSuffix:@"bmp"])
									{
										imageFileType = NSBitmapImageFileTypeBMP;
									}
									else if ([savePanel.URL.path hasSuffix:@"gif"])
									{
										imageFileType = NSBitmapImageFileTypeGIF;
									}
									else if ([savePanel.URL.path hasSuffix:@"jpg"] || [savePanel.URL.path hasSuffix:@"jpeg"])
									{
										imageFileType = NSBitmapImageFileTypeJPEG;
									}
									
									imageData = [imageRep representationUsingType:imageFileType properties:propertyDict];
									if (NO == [imageData writeToURL:savePanel.URL options:0L error:&error])
									{
										Sound_StandardAlert();
										Console_Warning(Console_WriteValueCFString,
														"failed to save selected text to file, error",
														BRIDGE_CAST([error localizedDescription], CFStringRef));
									}
								}
							}];
			}
		}
		else
		{
			// save selected text
			CFRetainRelease		saveFileCFString(UIStrings_ReturnCopy(kUIStrings_FileDefaultCaptureFile),
													CFRetainRelease::kAlreadyRetained);
			CFRetainRelease		promptCFString(UIStrings_ReturnCopy(kUIStrings_SystemDialogPromptSaveSelectedText),
												CFRetainRelease::kAlreadyRetained);
			
			
			savePanel.message = BRIDGE_CAST(promptCFString.returnCFStringRef(), NSString*);
			savePanel.directoryURL = nil;
			savePanel.nameFieldStringValue = BRIDGE_CAST(saveFileCFString.returnCFStringRef(), NSString*);
			[savePanel beginSheetModalForWindow:TerminalView_ReturnNSWindow(inView)
						completionHandler:^(NSInteger aReturnCode)
						{
							if (NSModalResponseOK == aReturnCode)
							{
								CFRetainRelease		textSelection(TerminalView_ReturnSelectedTextCopyAsUnicode
																	(inView, 0/* spaces equal to one tab, or zero for no substitution */,
																		kTerminalView_TextFlagLineSeparatorLF |
																		kTerminalView_TextFlagLastLineHasSeparator),
																	CFRetainRelease::kAlreadyRetained);
								
								
								if (textSelection.exists())
								{
									NSString*	asNSString = BRIDGE_CAST
																(textSelection.returnCFStringRef(), NSString*);
									NSError*	error = nil;
									
									
									if (NO == [asNSString writeToURL:savePanel.URL atomically:YES
															encoding:NSUTF8StringEncoding error:&error])
									{
										Sound_StandardAlert();
										Console_Warning(Console_WriteValueCFString,
														"failed to save selected text to file, error",
														BRIDGE_CAST([error localizedDescription], CFStringRef));
									}
								}
							}
						}];
		}
	}
}// DisplaySaveSelectionUI


/*!
Removes all highlighted search results ranges, and clears the
current search result focus.

A "kTerminalView_EventSearchResultsExistence" event is sent if
the previous search results were not already cleared; this
allows a view to stop customizing its scroll bar display.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(3.1)
*/
TerminalView_Result
TerminalView_FindNothing	(TerminalViewRef	inView)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		Boolean const	kWasCleared = viewPtr->text.searchResults.empty();
		
		
		for (auto cellRange : viewPtr->text.searchResults)
		{
			highlightVirtualRange(viewPtr, cellRange, kTextAttributes_SearchHighlight,
									false/* is highlighted */, true/* redraw */);
		}
		viewPtr->text.searchResults.clear();
		viewPtr->text.toCurrentSearchResult = viewPtr->text.searchResults.end();
		
		if (false == kWasCleared)
		{
			eventNotifyForView(viewPtr, kTerminalView_EventSearchResultsExistence, inView/* context */);
		}
	}
	return result;
}// FindNothing


/*!
Highlights the specified range of text as if it were a matching
word in a set of search results.  The current search result
focus is unchanged unless nothing was focused, in which case
this first search range becomes the focused one.

Unlike TerminalView_SelectVirtualRange(), this does not replace
any current selection: it adds the specified range to a group of
ranges representing all the search results for the view.  To
remove ranges, use TerminalView_FindNothing().

This triggers a "kTerminalView_EventScrolling" event so that
listeners can respond (for instance, to update a scroll bar that
is showing the positions of the search results).

A "kTerminalView_EventSearchResultsExistence" event is also sent,
if the previous search results were empty; this allows a view to
start customizing its scroll bar display.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(3.1)
*/
TerminalView_Result
TerminalView_FindVirtualRange	(TerminalViewRef				inView,
								 TerminalView_CellRange const&	inSelection)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		Boolean const	kWasCleared = viewPtr->text.searchResults.empty();
		
		
		viewPtr->text.searchResults.push_back(inSelection);
		assert(false == viewPtr->text.searchResults.empty());
		viewPtr->text.toCurrentSearchResult = viewPtr->text.searchResults.begin();
		highlightVirtualRange(viewPtr, viewPtr->text.searchResults.back(),
								kTextAttributes_SearchHighlight,
								true/* is highlighted */, true/* redraw */);
		
		// TEMPORARY - efficiency may demand a unique type of event for this
		eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
		
		if (kWasCleared != viewPtr->text.searchResults.empty())
		{
			eventNotifyForView(viewPtr, kTerminalView_EventSearchResultsExistence, inView/* context */);
		}
	}
	return result;
}// FindVirtualRange
  
  
/*!
Flashes the selected text, to indicate that it has
been accepted for a 'GURL' event.

(2.6)
*/
void
TerminalView_FlashSelection		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (selectionExists(viewPtr))
	{
		Console_Warning(Console_WriteLine, "TerminalView_FlashSelection() not implemented for Cocoa");
	}
}// FlashSelection


/*!
Changes the focus of the specified view’s owning
window to be the given view’s container.

(3.0)
*/
void
TerminalView_FocusForUser	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	NSView*						cocoaView = TerminalView_ReturnUserFocusNSView(inView);
	
	
	[[cocoaView window] makeFirstResponder:TerminalView_ReturnUserFocusNSView(inView)];
}// FocusForUser


/*!
Provides the color with the specified index from
the given screen.  Returns "true" only if the
color could be acquired.

NOTE:	This routine returns a base color, which
		is suitable for display in dialog boxes,
		etc.; it does NOT return the color that
		is currently being used to render text.
		For example, blinking text is rendered in
		different colors depending on the instance
		in time, but this routine always returns
		the same colors for blinking no matter
		what the current rendering state is.

(3.0)
*/
Boolean
TerminalView_GetColor	(TerminalViewRef			inView,
						 TerminalView_ColorIndex	inColorEntryNumber,
						 CGFloatRGBColor*			outColorPtr)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						result = false;
	
	
	if (outColorPtr != nullptr)
	{
		outColorPtr->red = outColorPtr->green = outColorPtr->blue = 0;
		if ((inColorEntryNumber <= kTerminalView_ColorIndexLastValid) &&
			(inColorEntryNumber >= kTerminalView_ColorIndexFirstValid))
		{
			getScreenBaseColor(viewPtr, inColorEntryNumber, outColorPtr);
			result = true;
		}
	}
	return result;
}// GetColor


/*!
Provides the current terminal cursor rectangle with
respect to the origin of the terminal’s NSWindow
(not the terminal view).

Note that the cursor can have different shapes...
for instance, the returned rectangle may be very
flat (for an underline), narrow (for an insertion
point), or block shaped.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(2020.05)
*/
TerminalView_Result
TerminalView_GetCursorBoundsWindowRelative	(TerminalViewRef	inView,
											 CGRect&			outCursorBoundsRelativeToWindowFrameOrigin)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	outCursorBoundsRelativeToWindowFrameOrigin = CGRectZero;
	
	if (nullptr == viewPtr)
	{
		result = kTerminalView_ResultInvalidID;
	}
	else
	{
		outCursorBoundsRelativeToWindowFrameOrigin = viewPtr->screen.cursor.bounds; // initially...
		outCursorBoundsRelativeToWindowFrameOrigin = [viewPtr->encompassingNSView.terminalContentView convertRect:outCursorBoundsRelativeToWindowFrameOrigin toView:nil/* use window */];
	}
	
	return result;
}// GetCursorBoundsWindowRelative


/*!
Returns the font and size of the text in the specified
terminal screen.  Either result can be nullptr if you
are not interested in that value.

(3.0)
*/
void
TerminalView_GetFontAndSize		(TerminalViewRef	inView,
								 CFStringRef*		outFontFamilyNameOrNull,
								 CGFloat*			outFontSizeOrNull)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr != viewPtr)
	{
		if (nullptr != outFontFamilyNameOrNull)
		{
			CFStringRef		fontNameCFString = viewPtr->text.font.familyName.returnCFStringRef();
			
			
			*outFontFamilyNameOrNull = fontNameCFString;
		}
		
		if (nullptr != outFontSizeOrNull)
		{
			*outFontSizeOrNull = viewPtr->text.font.normalMetrics.size;
		}
	}
}// GetFontAndSize


/*!
Retrieves the view’s best dimensions in pixels, given
the current font metrics and screen dimensions.
Returns true only if successful.

(3.1)
*/
Boolean
TerminalView_GetIdealSize	(TerminalViewRef			inView,
							 TerminalView_PixelWidth&	outWidthInPixels,
							 TerminalView_PixelHeight&	outHeightInPixels)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						result = false;
	
	
	// IMPORTANT: the assumptions made here also apply to the layout
	// of subviews performed by TerminalView_Controller
	if (nullptr != viewPtr)
	{
		CGFloat		highPrecision = 0;
		
		
		highPrecision = viewPtr->screen.marginLeftEmScale + viewPtr->screen.paddingLeftEmScale
						+ viewPtr->screen.paddingRightEmScale + viewPtr->screen.marginRightEmScale;
		highPrecision *= viewPtr->text.font.widthPerCell.precisePixels();
		highPrecision += viewPtr->screen.cache.viewWidthInPixels.precisePixels();
		outWidthInPixels.setPrecisePixels(highPrecision);
		
		highPrecision = viewPtr->screen.marginTopEmScale + viewPtr->screen.paddingTopEmScale
						+ viewPtr->screen.paddingBottomEmScale + viewPtr->screen.marginBottomEmScale;
		highPrecision *= viewPtr->text.font.widthPerCell.precisePixels(); // yes, width, because this is an “em” scale factor
		highPrecision += viewPtr->screen.cache.viewHeightInPixels.precisePixels();
		outHeightInPixels.setPrecisePixels(highPrecision);
		
		result = true;
	}
	return result;
}// GetIdealSize


/*!
Returns information on the currently visible rows and the
maximum theoretical row view size, scaled to the limits of
a 32-bit integer (for use in user interface elements).

The maximum range is not necessarily appropriate as the
maximum value of a control; in particular, you would want
to subtract one page (the view range size) to reflect the
fact that the current value is always the top of the range.

See also TerminalView_ScrollToIndicatorPosition(), which is
useful for handling live tracking of user interface elements,
and TerminalView_ScrollToCell() for other kinds of activity.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(4.0)
*/
TerminalView_Result
TerminalView_GetScrollVerticalInfo	(TerminalViewRef	inView,
									 SInt64&			outStartOfView,
									 SInt64&			outPastEndOfView,
									 SInt64&			outStartOfMaximumRange,
									 SInt64&			outPastEndOfMaximumRange)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		SInt64 const	kScrollbackRows = Terminal_ReturnInvisibleRowCount(viewPtr->screen.ref);
		SInt64 const	kRows = Terminal_ReturnRowCount(viewPtr->screen.ref);
		
		
		// WARNING: this must use the same logic as TerminalView_ScrollToIndicatorPosition()
		outStartOfMaximumRange = 0;
		outPastEndOfMaximumRange = outStartOfMaximumRange + kScrollbackRows + kRows;
		outStartOfView = outPastEndOfMaximumRange - kRows + viewPtr->screen.topVisibleEdgeInRows;
		outPastEndOfView = outStartOfView + (viewPtr->screen.cache.viewHeightInPixels.precisePixels() /
												viewPtr->text.font.heightPerCell.precisePixels());
	}
	return result;
}// GetScrollVerticalInfo


/*!
Returns all of the ranges highlighted as search results, as
determined by repeated calls to TerminalView_FindVirtualRange()
(and cleared by TerminalView_FindNothing()).

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(4.0)
*/
TerminalView_Result
TerminalView_GetSearchResults	(TerminalViewRef				inView,
								 TerminalView_CellRangeList&	outResults)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		outResults = viewPtr->text.searchResults;
	}
	return result;
}// GetSearchResults


/*!
Causes the computer to speak the specified terminal’s
text selection, using the speech channel of that
same terminal.  You can therefore interrupt this
speech using the normal mechanisms for interrupting
other terminal speech.

IMPORTANT:	This API is under evaluation.  Audio should
			probably be handled separately using a
			reference to the text selection data.

(3.0)
*/
void
TerminalView_GetSelectedTextAsAudio		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
	{
		CFRetainRelease		spokenText(returnSelectedTextCopyAsUnicode(viewPtr, 0/* space info */, kTerminalView_TextFlagInline),
										CFRetainRelease::kAlreadyRetained);
		
		
		if (spokenText.exists())
		{
			TerminalSpeaker_Ref		speaker = Terminal_ReturnSpeaker(viewPtr->screen.ref);
			
			
			if (nullptr != speaker)
			{
				TerminalSpeaker_Result		speakerResult = kTerminalSpeaker_ResultOK;
				
				
				speakerResult = TerminalSpeaker_SynthesizeSpeechFromCFString(speaker, spokenText.returnCFStringRef());
				if (kTerminalSpeaker_ResultOK != speakerResult)
				{
					Console_Warning(Console_WriteValue, "failed to synthesize speech, error", speakerResult);
				}
			}
		}
	}
}// GetSelectedTextAsAudio


/*!
Returns the start and end anchor points (which
are zero-based row and column numbers).  You can
pass these into TerminalView_SelectVirtualRange().

(3.0)
*/
void
TerminalView_GetSelectedTextAsVirtualRange	(TerminalViewRef			inView,
											 TerminalView_CellRange&	outSelection)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	outSelection = viewPtr->text.selection.range;
}// GetSelectedTextAsVirtualRange


/*!
Calculates the number of rows and columns that the
specified screen, using its current font metrics,
would require in order to best fit the specified
width and height in pixels.

The inverse of this routine is
TerminalView_GetTheoreticalViewSize().

(3.0)
*/
void
TerminalView_GetTheoreticalScreenDimensions		(TerminalViewRef			inView,
												 TerminalView_PixelWidth	inWidthInPixels,
												 TerminalView_PixelHeight	inHeightInPixels,
												 UInt16*					outColumnCount,
												 TerminalView_RowIndex*		outRowCount)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	CGFloat						highPrecision = 0.0;
	
	
	// Remove padding, border and margins before scaling remainder to find number of characters.
	// IMPORTANT: Synchronize this with TerminalView_GetTheoreticalViewSize() and TerminalView_GetIdealSize().
	highPrecision = inWidthInPixels.precisePixels() / viewPtr->text.font.widthPerCell.precisePixels();
	highPrecision -= (viewPtr->screen.marginLeftEmScale + viewPtr->screen.paddingLeftEmScale
						+ viewPtr->screen.paddingRightEmScale + viewPtr->screen.marginRightEmScale);
	*outColumnCount = STATIC_CAST(highPrecision, UInt16);
	if (*outColumnCount < 1) *outColumnCount = 1;
	highPrecision = inHeightInPixels.precisePixels()
					- (STATIC_CAST(viewPtr->screen.marginTopEmScale + viewPtr->screen.paddingTopEmScale
									+ viewPtr->screen.paddingBottomEmScale + viewPtr->screen.marginBottomEmScale, CGFloat)
						* viewPtr->text.font.widthPerCell.precisePixels()); // yes, width, because this is an “em” scale factor
	highPrecision /= viewPtr->text.font.heightPerCell.precisePixels();
	*outRowCount = STATIC_CAST(highPrecision, UInt16);
	if (*outRowCount < 1) *outRowCount = 1;
}// GetTheoreticalScreenDimensions


/*!
Calculates the width and height in pixels that the
specified screen would have if, using its current
font metrics, its dimensions were the specified
number of rows and columns.

The inverse of this routine is
TerminalView_GetTheoreticalScreenDimensions().

(3.0)
*/
void
TerminalView_GetTheoreticalViewSize		(TerminalViewRef			inView,
										 UInt16						inColumnCount,
										 TerminalView_RowIndex		inRowCount,
										 TerminalView_PixelWidth&	outWidthInPixels,
										 TerminalView_PixelHeight&	outHeightInPixels)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	CGFloat						highPrecision = 0.0;
	
	
	// Incorporate the rectangle required for this amount of text, plus padding, border and margins.
	// IMPORTANT: Synchronize this with TerminalView_GetTheoreticalScreenDimensions() and TerminalView_GetIdealSize()
	// and ensure that TerminalView_Controller uses the same relative values for subview layout.
	highPrecision = STATIC_CAST(inColumnCount, CGFloat) + viewPtr->screen.marginLeftEmScale + viewPtr->screen.paddingLeftEmScale
					+ viewPtr->screen.paddingRightEmScale + viewPtr->screen.marginRightEmScale;
	highPrecision *= viewPtr->text.font.widthPerCell.precisePixels();
	outWidthInPixels.setPrecisePixels(highPrecision);
	highPrecision = STATIC_CAST(inRowCount, CGFloat) * viewPtr->text.font.heightPerCell.precisePixels();
	highPrecision += (STATIC_CAST(viewPtr->screen.marginTopEmScale + viewPtr->screen.paddingTopEmScale
									+ viewPtr->screen.paddingBottomEmScale + viewPtr->screen.marginBottomEmScale, CGFloat)
						* viewPtr->text.font.widthPerCell.precisePixels()); // yes, width, because this is an “em” scale factor
	outHeightInPixels.setPrecisePixels(highPrecision);
}// GetTheoreticalViewSize


/*!
Normally, when the user changes a preference that can
affect a view, a notification is sent and the view is
automatically synchronized with the change; but this
routine allows you to ignore certain changes.

Currently, this does not work for settings that are
completely global, such as cursor blinking.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(3.1)
*/
TerminalView_Result
TerminalView_IgnoreChangesToPreference	(TerminalViewRef	inView,
										 Preferences_Tag	inWhichSetting)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		viewPtr->configFilter.insert(inWhichSetting);
	}
	return result;
}// IgnoreChangesToPreference


/*!
Version 3.0 supports two modes of text selection:
standard, like most Macintosh applications, and
rectangular, which allows only rectangular regions,
anywhere in a terminal window, to be highlighted.
This second mode is very useful for capturing specific
output from terminal applications (for examples,
columns of output from a UNIX command line result,
like "ls -l").

To make text selections rectangular in a particular
window, use this method and pass a value of "true"
for the second parameter.  To reset text selections
to normal, pass "false".

IMPORTANT:	All of MacTerm’s functions, including
			updating the terminal text selection of a
			window, rely on this flag.  Thus, you
			should ensure a text selection is *empty*
			before toggling this value (to prevent
			any selected text from appearing to cover
			a different region than it really does).

(3.0)
*/
void
TerminalView_MakeSelectionsRectangular	(TerminalViewRef	inView,
										 Boolean			inAreSelectionsNotAttachedToScreenEdges)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	viewPtr->text.selection.isRectangular = inAreSelectionsNotAttachedToScreenEdges;
}// MakeSelectionsRectangular


/*!
Positions the cursor at the screen row and column underneath
the specified point in coordinates local to the screen window.
This is done interactively, by sending the appropriate cursor
control key sequences to the server.  Invoking this function
to move the cursor ensures that remote applications see that
the cursor has moved, and where it has moved to.

(3.0)
*/
void
TerminalView_MoveCursorWithArrowKeys	(TerminalViewRef	inView,
										 CGPoint			inNSViewLocalMouse)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Cell			newColumnRow;
	Boolean						moveOK = false;
	
	
	// IMPORTANT: The deltas returned here refer to mouse locations that
	// are outside the view, which are not needed.
	if (findVirtualCellFromScreenPoint(viewPtr, inNSViewLocalMouse, newColumnRow))
	{
		Terminal_Result		terminalResult = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		// find the cursor position
		terminalResult = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
		if (kTerminal_ResultOK == terminalResult)
		{
			SInt16 const	kDeltaX = newColumnRow.first - STATIC_CAST(cursorX, SInt16);
			SInt16 const	kDeltaY = STATIC_CAST(newColumnRow.second, SInt16) - STATIC_CAST(cursorY, SInt16);
			
			
			terminalResult = Terminal_UserInputOffsetCursor(viewPtr->screen.ref, kDeltaX, kDeltaY);
			if (kTerminal_ResultOK == terminalResult)
			{
				// success!
				moveOK = true;
			}
		}
	}
	
	if (false == moveOK)
	{
		Sound_StandardAlert();
	}
}// MoveCursorWithArrowKeys


/*!
Removes a specific data source that was previously set by
TerminalView_AddDataSource(), or removes all data sources
if "nullptr" is given.  The data from the removed source
is no longer reflected in the view.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

\retval kTerminalView_ResultParameterError
if at least one requested data source cannot be removed

(4.1)
*/
TerminalView_Result
TerminalView_RemoveDataSource	(TerminalViewRef		inView,
								 TerminalScreenRef		inScreenDataSourceOrNull)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		if (false == stopMonitoringDataSource(viewPtr, inScreenDataSourceOrNull))
		{
			result = kTerminalView_ResultParameterError;
		}
		else
		{
			if (false == removeDataSource(viewPtr, inScreenDataSourceOrNull))
			{
				result = kTerminalView_ResultParameterError;
			}
			else
			{
				// success!
				result = kTerminalView_ResultOK;
			}
		}
	}
	
	return result;
}// RemoveDataSource


/*!
Returns the Cocoa NSView* that is the root of the specified
screen.  With the container, you can safely position a screen
anywhere in a window and the contents of the screen (embedded
in the container) will move with it.

To resize a screen’s view area, use the view "frame" property.

(4.0)
*/
TerminalView_Object*
TerminalView_ReturnContainerNSView		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Object*		result = nil;
	
	
	result = viewPtr->encompassingNSView;
	return result;
}// ReturnContainerNSView


/*!
Returns the string nearest the current terminal cursor
position, or nullptr if there is no selection.  If there
is no character immediately under the cursor, the character
immediately preceding the cursor is also checked.  The word
itself may begin and end anywhere on the cursor line but
some character of the string will be at the cursor position.

NOTE:	This is a function of the Terminal View and not the
		screen buffer because only the view has the concept
		of a “word” (normally used for double-clicking).

(4.1)
*/
CFStringRef
TerminalView_ReturnCursorWordCopyAsUnicode	(TerminalViewRef	inView)
{
    My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	CFStringRef					result = nullptr;
	
	
	if (nullptr != viewPtr)
	{
		TerminalView_CellRange const	kOldSelectionRange = viewPtr->text.selection.range;
		
		
		TerminalView_SelectBeforeCursorCharacter(inView);
		handleMultiClick(viewPtr, 2);
		result = TerminalView_ReturnSelectedTextCopyAsUnicode
					(inView, 0/* spaces to replace with tab */, 0/* flags */);
		TerminalView_SelectNothing(inView); // fix any highlighting changes caused by the “selection” above
		TerminalView_SelectVirtualRange(inView, kOldSelectionRange);
	}
	
	return result;
}// ReturnCursorWordCopyAsUnicode


/*!
Returns the current display mode, which is normal by
default but could have been changed using the
TerminalView_SetDisplayMode() routine.

(3.1)
*/
TerminalView_DisplayMode
TerminalView_ReturnDisplayMode	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_DisplayMode	result = kTerminalView_DisplayModeNormal;
	
	
	result = viewPtr->displayMode;
	return result;
}// ReturnDisplayMode


/*!
Returns the Cocoa NSView* that can render a drag highlight
(via TerminalView_SetDragHighlight()).  It should have drag
handlers.

This might be, but is NOT guaranteed to be, the same as the
user focus view.  See also TerminalView_ReturnUserFocusNSView().

(4.0)
*/
NSView*
TerminalView_ReturnDragFocusNSView	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	NSView*						result = nil;
	
	
	// should match whichever view has drag handlers
	result = viewPtr->encompassingNSView.terminalContentView;
	return result;
}// ReturnDragFocusNSView


/*!
Returns a variety of font/color preferences unique to this view.

You can make changes to this context ONLY if you do it in “batch
mode” with Preferences_ContextCopy().  In other words, even to
make a single change, you must first add the change to a new
temporary context, then use Preferences_ContextCopy() to read
the temporary settings into the context returned by this routine.
Batch mode changes are detected by the Terminal View and used to
automatically update the display and internal caches.

Note that you cannot expect all possible tags to be present;
be prepared to not find what you look for.  In addition, tags
that are present in one view may be absent in another.

You cannot change the font size if the display mode is
currently setting the size automatically.  Use the
TerminalView_ReturnDisplayMode() routine to determine
what the display mode is.

(3.1)
*/
Preferences_ContextRef
TerminalView_ReturnFormatConfiguration		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef		result = viewPtr->formatConfig.returnRef();
	
	
	// since many settings are represented internally, this context
	// will not contain the latest information; update the context
	// based on current settings
	
	// IMPORTANT: There is a trick here...because NO internal
	// routines in this module mess with fonts or colors outside
	// of initialization and user changes, it is NOT necessary to
	// resync those preferences here: they will already be accurate.
	// HOWEVER, if an internal routine were added that (say) messed
	// with internally-stored colors for some reason, then it WOULD
	// be necessary to call Preferences_ContextSetData() here to
	// ensure the latest cached values are in the context.
	
	// font size is messed with programmatically (e.g. Make Text Bigger)
	// so its current value is resynced with the context regardless
	{
		Float64		fontSize = viewPtr->text.font.normalMetrics.size;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagFontSize,
													sizeof(fontSize), &fontSize);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	// cursor auto-color flag is also set programmatically
	{
		Boolean		isAutoSet = (false == viewPtr->screen.cursor.isCustomColor);
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagAutoSetCursorColor,
													sizeof(isAutoSet), &isAutoSet);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	// mouse pointer color is also set programmatically
	{
		TerminalView_MousePointerColor		pointerColor = viewPtr->screen.mouse.pointerColor;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagTerminalMousePointerColor,
													sizeof(pointerColor), &pointerColor);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	return result;
}// ReturnFormatConfiguration


/*!
Returns the Cocoa window reference for the given
Terminal View.  This is currently only defined if
the view was created as an NSView.

(4.0)
*/
NSWindow*
TerminalView_ReturnNSWindow		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	NSWindow*					result = nil;
	
	
	result = [viewPtr->encompassingNSView window];
	return result;
}// ReturnNSWindow


/*!
Returns a new array of NSImage* values that refer to
“complete” images within the text selection range.
(In other words, an image is returned if it even
slightly in the highlighted region.)

Use CFRelease() to dispose of the array when finished.

(2017.12)
*/
CFArrayRef
TerminalView_ReturnSelectedImageArrayCopy	(TerminalViewRef	inView)
{
    My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	NSMutableArray*				mutableArray = [[NSMutableArray alloc] init];
	CFArrayRef					result = BRIDGE_CAST_CFRETAIN(mutableArray, CFArrayRef);
	
	
	getImagesInVirtualRange(viewPtr, viewPtr->text.selection.range, mutableArray);
	
	return result;
}// ReturnSelectedImageArrayCopy


/*!
Returns the contents of the current selection for the
specified screen view as a Core Foundation string.  If
there is no selection, an empty string is returned.
If any problems occur, nullptr is returned.

Use the parameters to this routine to affect how the
text is returned; for example, you can have the text
returned inline, or delimited by carriage returns.
The "inMaxSpacesToReplaceWithTabOrZero" value can be 0
to perform no substitution, or a positive integer to
indicate the longest range of consecutive spaces that
should be replaced with a single tab before returning
the text (all smaller ranges are also replaced by one
tab).

Use CFRelease() to dispose of the data when finished.

IMPORTANT:	Use this kind of routine very judiciously.
			Copying data is very inefficient and is
			almost never necessary, particularly for
			scrollback rows which are immutable.  Use
			other APIs to access text ranges in ways
			that do not replicate bytes needlessly.

(3.1)
*/
CFStringRef
TerminalView_ReturnSelectedTextCopyAsUnicode	(TerminalViewRef			inView,
												 UInt16						inMaxSpacesToReplaceWithTabOrZero,
												 TerminalView_TextFlags		inFlags)
{
    My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	CFStringRef					result = nullptr;
	
	
	if (nullptr != viewPtr)
	{
		result = returnSelectedTextCopyAsUnicode(viewPtr, inMaxSpacesToReplaceWithTabOrZero, inFlags);
	}
	return result;
}// ReturnSelectedTextCopyAsUnicode


/*!
Returns the size in bytes of the current selection for
the specified screen view, or zero if there is none.

(3.1)
*/
size_t
TerminalView_ReturnSelectedTextSize		(TerminalViewRef	inView)
{
    My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	size_t						result = 0;
	
	
	if (nullptr != viewPtr)
	{
		result = getSelectedTextSize(viewPtr);
	}
	return result;
}// ReturnSelectedTextSize


/*!
Returns text encoding preferences unique to this view.

You can make changes to this context ONLY if you do it in “batch
mode” with Preferences_ContextCopy().  In other words, even to
make a single change, you must first add the change to a new
temporary context, then use Preferences_ContextCopy() to read
the temporary settings into the context returned by this routine.
Batch mode changes are detected by the Terminal View and used to
automatically update the display and internal caches.

Note that you cannot expect all possible tags to be present;
be prepared to not find what you look for.  In addition, tags
that are present in one view may be absent in another.

(4.0)
*/
Preferences_ContextRef
TerminalView_ReturnTranslationConfiguration		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Preferences_ContextRef		result = viewPtr->encodingConfig.returnRef();
	Boolean						setOK = false;
	
	
	// since many settings are represented internally, this context
	// will not contain the latest information; update the context
	// based on current settings
	
	// set encoding name and ID
	setOK = TextTranslation_ContextSetEncoding(result, Terminal_ReturnTextEncoding(viewPtr->screen.ref));
	if (false == setOK) Console_Warning(Console_WriteLine, "unable to set the text encoding in the terminal view translation configuration");
	
	return result;
}// ReturnTranslationConfiguration


/*!
Returns the Cocoa NSView* that can be focused for keyboard
input.  You can use this compared with the current keyboard
focus for a window to determine if the specified view is
focused.

(4.0)
*/
NSView*
TerminalView_ReturnUserFocusNSView	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	NSView*						result = nil;
	
	
	// should match whichever view renders a focus ring
	result = viewPtr->encompassingNSView.terminalContentView;
	return result;
}// ReturnUserFocusNSView


/*!
Reverses the foreground and background colors
of a window.

(2.6)
*/
void
TerminalView_ReverseVideo	(TerminalViewRef	inView,
							 Boolean			inReverseVideo)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (inReverseVideo != viewPtr->screen.isReverseVideo)
	{
		CGFloatRGBColor		oldTextColor;
		CGFloatRGBColor		oldBackgroundColor;
		
		
		viewPtr->screen.isReverseVideo = !viewPtr->screen.isReverseVideo;
		
		// flip the background and foreground color positions
		getScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalText, &oldTextColor);
		getScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalBackground, &oldBackgroundColor);
		setScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalText, &oldBackgroundColor);
		setScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalBackground, &oldTextColor);
		
		updateDisplay(viewPtr);
	}
}// ReverseVideo


/*!
Changes the search results index for highlighting.  This is
reset by a call to TerminalView_FindNothing().

IMPORTANT:	Currently, the rotation amount should be +1 or -1,
			to move respectively to the next or previous range.

To update the display, use TerminalView_ZoomToSearchResults().

(3.1)
*/
void
TerminalView_RotateSearchResultHighlight	(TerminalViewRef	inView,
											 SInt16				inHowFarWhichWay)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((viewPtr != nullptr) && (false == viewPtr->text.searchResults.empty()))
	{
		if (inHowFarWhichWay == -1)
		{
			if (viewPtr->text.searchResults.begin() == viewPtr->text.toCurrentSearchResult)
			{
				viewPtr->text.toCurrentSearchResult = viewPtr->text.searchResults.end();
			}
			--viewPtr->text.toCurrentSearchResult;
		}
		else
		{
			++viewPtr->text.toCurrentSearchResult;
			if (viewPtr->text.searchResults.end() == viewPtr->text.toCurrentSearchResult)
			{
				viewPtr->text.toCurrentSearchResult = viewPtr->text.searchResults.begin();
			}
		}
	}
}// RotateSearchResultHighlight


/*!
Scrolls the contents of the terminal screen both
horizontally and vertically.  If a delta is negative,
the *contents* of the screen move down or to the right;
otherwise, the contents move up or to the left.

(3.1)
*/
TerminalView_Result
TerminalView_ScrollAround	(TerminalViewRef	inView,
							 SInt16				inColumnCountDelta,
							 SInt16				inRowCountDelta)
{
	TerminalView_Result		result = kTerminalView_ResultOK;
	
	
	if (inColumnCountDelta > 0)
	{
		result = TerminalView_ScrollColumnsTowardLeftEdge(inView, inColumnCountDelta);
	}
	else if (inColumnCountDelta < 0)
	{
		result = TerminalView_ScrollColumnsTowardRightEdge(inView, -inColumnCountDelta);
	}
	
	if (inRowCountDelta > 0)
	{
		result = TerminalView_ScrollRowsTowardTopEdge(inView, inRowCountDelta);
	}
	else if (inRowCountDelta < 0)
	{
		result = TerminalView_ScrollRowsTowardBottomEdge(inView, -inRowCountDelta);
	}
	
	return result;
}// ScrollAround


/*!
Scrolls the contents of the terminal screen as if
the user clicked the right scroll arrow.  You must
specify the number of columns to scroll.

(2.6)
*/
TerminalView_Result
TerminalView_ScrollColumnsTowardLeftEdge	(TerminalViewRef	inView,
											 UInt16				inNumberOfColumnsToScroll)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if ((viewPtr != nullptr) && (inNumberOfColumnsToScroll != 0))
	{
		// just redraw everything
		updateDisplay(viewPtr);
		
		// update anchor
		offsetLeftVisibleEdge(viewPtr, +inNumberOfColumnsToScroll);
		
		eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
	}
	return result;
}// ScrollColumnsTowardLeftEdge


/*!
Scrolls the contents of the terminal screen as if
the user clicked the left scroll arrow.  You must
specify the number of columns to scroll.

(2.6)
*/
TerminalView_Result
TerminalView_ScrollColumnsTowardRightEdge	(TerminalViewRef	inView,
											 UInt16				inNumberOfColumnsToScroll)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr != nullptr)
	{
		// just redraw everything
		updateDisplay(viewPtr);
		
		// update anchor
		offsetLeftVisibleEdge(viewPtr, -inNumberOfColumnsToScroll);
		
		eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
	}
	return result;
}// ScrollColumnsTowardRightEdge


/*!
Scrolls the contents of the terminal screen as if the user
clicked the page-up region.  Currently, this is done in a
synchronously-animated fashion that cannot be disabled.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(4.0)
*/
TerminalView_Result
TerminalView_ScrollPageTowardBottomEdge		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		UInt16 const	kVisibleRowCount = Terminal_ReturnRowCount(viewPtr->screen.ref);
		
		
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, kVisibleRowCount / 4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, kVisibleRowCount / 4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, kVisibleRowCount / 4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, kVisibleRowCount - 3 * (kVisibleRowCount / 4));
	}
	return result;
}// ScrollPageTowardBottomEdge


/*!
Scrolls the contents of the terminal screen as if the user
clicked the page-right region.  Currently, this is done in a
synchronously-animated fashion that cannot be disabled.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(4.0)
*/
TerminalView_Result
TerminalView_ScrollPageTowardLeftEdge		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		UInt16 const	kVisibleColumnCount = Terminal_ReturnColumnCount(viewPtr->screen.ref);
		UInt16 const	kVisibleColumnCountBy4 = STATIC_CAST(INTEGER_DIV_4(kVisibleColumnCount), UInt16);
		
		
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardLeftEdge(inView, kVisibleColumnCountBy4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardLeftEdge(inView, kVisibleColumnCountBy4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardLeftEdge(inView, kVisibleColumnCountBy4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardLeftEdge(inView, kVisibleColumnCount - 3 * kVisibleColumnCountBy4);
	}
	return result;
}// ScrollPageTowardLeftEdge


/*!
Scrolls the contents of the terminal screen as if the user
clicked the page-left region.  Currently, this is done in a
synchronously-animated fashion that cannot be disabled.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(4.0)
*/
TerminalView_Result
TerminalView_ScrollPageTowardRightEdge		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		UInt16 const	kVisibleColumnCount = Terminal_ReturnColumnCount(viewPtr->screen.ref);
		UInt16 const	kVisibleColumnCountBy4 = STATIC_CAST(INTEGER_DIV_4(kVisibleColumnCount), UInt16);
		
		
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardRightEdge(inView, kVisibleColumnCountBy4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardRightEdge(inView, kVisibleColumnCountBy4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardRightEdge(inView, kVisibleColumnCountBy4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardRightEdge(inView, kVisibleColumnCount - 3 * kVisibleColumnCountBy4);
	}
	return result;
}// ScrollPageTowardRightEdge


/*!
Scrolls the contents of the terminal screen as if the user
clicked the page-down region.  Currently, this is done in a
synchronously-animated fashion that cannot be disabled.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(4.0)
*/
TerminalView_Result
TerminalView_ScrollPageTowardTopEdge		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		UInt16 const	kVisibleRowCount = Terminal_ReturnRowCount(viewPtr->screen.ref);
		
		
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, kVisibleRowCount / 4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, kVisibleRowCount / 4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, kVisibleRowCount / 4);
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, kVisibleRowCount - 3 * (kVisibleRowCount / 4));
	}
	return result;
}// ScrollPageTowardTopEdge


/*!
Scrolls the contents of the terminal screen as if
the user clicked the up scroll arrow.  You must
specify the number of rows to scroll.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the screen reference is unrecognized

(2.6)
*/
TerminalView_Result
TerminalView_ScrollRowsTowardBottomEdge		(TerminalViewRef	inView,
											 UInt32 			inNumberOfRowsToScroll)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		// just redraw everything
		updateDisplay(viewPtr);
		
		// update anchor
		offsetTopVisibleEdge(viewPtr, -inNumberOfRowsToScroll);
		
		eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
	}
	return result;
}// ScrollRowsTowardBottomEdge


/*!
Scrolls the contents of the terminal screen as if
the user clicked the down scroll arrow.  You must
specify the number of rows to scroll.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the screen reference is unrecognized

(2.6)
*/
TerminalView_Result
TerminalView_ScrollRowsTowardTopEdge	(TerminalViewRef	inView,
										 UInt32				inNumberOfRowsToScroll)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		// just redraw everything
		updateDisplay(viewPtr);
		
		// update anchor
		offsetTopVisibleEdge(viewPtr, +inNumberOfRowsToScroll);
		
		eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
	}
	return result;
}// ScrollRowsTowardTopEdge


/*!
Scrolls the contents of the terminal view as if the user
clicked the “home” key on an extended keyboard - conveniently
scrolling rows downward to show the very topmost row in the
screen area.

Returns anything that TerminalView_ScrollRowsTowardBottomEdge()
might return.

(3.0)
*/
TerminalView_Result
TerminalView_ScrollToBeginning	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		// scroll as far as possible
		result = TerminalView_ScrollRowsTowardBottomEdge(inView, Terminal_ReturnRowCount(viewPtr->screen.ref) +
																	Terminal_ReturnInvisibleRowCount(viewPtr->screen.ref));
	}
	return result;
}// ScrollToBeginning


/*!
Scrolls the contents of the terminal screen to focus on a
certain row and column location.

Currently, the horizontal value is ignored.

See also TerminalView_ScrollToIndicatorPosition().

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

\retval kTerminalView_ResultParameterError
if a range is invalid

(2016.10)
*/
TerminalView_Result
TerminalView_ScrollToCell	(TerminalViewRef			inView,
							 TerminalView_Cell const&	inCell)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		if (viewPtr->screen.topVisibleEdgeInRows != inCell.second)
		{
			offsetTopVisibleEdge(viewPtr, inCell.second - viewPtr->screen.topVisibleEdgeInRows);
			updateDisplay(viewPtr);
			eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
		}
	}
	return result;
}// ScrollToCell


/*!
Scrolls the contents of the terminal view as if the user
clicked the “end” key on an extended keyboard - conveniently
scrolling rows upward to show the very bottommost row in the
screen area.

Returns anything that TerminalView_ScrollRowsTowardTopEdge()
might return.

(3.0)
*/
TerminalView_Result
TerminalView_ScrollToEnd	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		// scroll as far as possible
		result = TerminalView_ScrollRowsTowardTopEdge(inView, Terminal_ReturnRowCount(viewPtr->screen.ref) +
														Terminal_ReturnInvisibleRowCount(viewPtr->screen.ref));
	}
	return result;
}// ScrollToEnd


/*!
Scrolls the contents of the terminal screen to a particular
location, as if the user dragged a scroll bar indicator.

The given integer ranges do not necessarily correspond to any
real value (such as row counts, or pixels); they could be
scaled.  Always use TerminalView_GetScrollVerticalInfo() to
set up the maximum range and value of a user interface element,
at which point it is safe to pass the current value of that
element to this routine.

Currently, the horizontal value is ignored.

See also TerminalView_ScrollToCell(), and other convenience
functions for scrolling.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

\retval kTerminalView_ResultParameterError
if a range is invalid

(4.0)
*/
TerminalView_Result
TerminalView_ScrollToIndicatorPosition	(TerminalViewRef	inView,
										 SInt32				inStartOfRangeV,
										 SInt32				UNUSED_ARGUMENT(inStartOfRangeH))
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		SInt32 const	kScrollbackRows = Terminal_ReturnInvisibleRowCount(viewPtr->screen.ref);
		
		
		// WARNING: this must use the same logic as TerminalView_GetScrollVerticalInfo()
		result = TerminalView_ScrollToCell(inView,
											TerminalView_Cell(0/* currently not doing horizontal scrolling */,
																(inStartOfRangeV - kScrollbackRows)));
	}
	return result;
}// ScrollToIndicatorPosition


/*!
Returns true only if one or more ranges were specified as
search results.  (TerminalView_FindNothing() clears this.)

(3.1)
*/
Boolean
TerminalView_SearchResultsExist		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						result = false;
	
	
	if ((viewPtr != nullptr) && (false == viewPtr->text.searchResults.empty()))
	{
		result = true;
	}
	return result;
}// SearchResultsExist


/*!
Sets the selection to be zero or one character, the cell just
preceding the current cursor location on the current line.
If the cursor is at the left margin, nothing is selected.

This is useful for beginning a completely-keyboard-driven
text selection.

See also SelectCursorCharacter().

(3.1)
*/
void
TerminalView_SelectBeforeCursorCharacter	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.inhibited))
	{
		Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		// find the new cursor region
		getCursorLocationError = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
		if (kTerminal_ResultOK == getCursorLocationError)
		{
			TerminalView_SelectNothing(inView);
			
			if (0 != cursorX)
			{
				viewPtr->text.selection.range.first = std::make_pair(cursorX - 1, cursorY);
				viewPtr->text.selection.range.second = std::make_pair(cursorX, cursorY + 1);
				
				highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
				viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeBeginning;
			}
		}
	}
}// SelectBeforeCursorCharacter


/*!
Sets the selection to be exactly one character, the cell
of the current cursor location.

This is useful for beginning a completely-keyboard-driven
text selection.

See also SelectBeforeCursorCharacter().

(3.1)
*/
void
TerminalView_SelectCursorCharacter		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.inhibited))
	{
		Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		// find the new cursor region
		getCursorLocationError = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
		if (kTerminal_ResultOK == getCursorLocationError)
		{
			TerminalView_SelectNothing(inView);
			
			viewPtr->text.selection.range.first = std::make_pair(cursorX, cursorY);
			viewPtr->text.selection.range.second = std::make_pair(cursorX + 1, cursorY + 1);
			
			highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
			viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeEnd;
		}
	}
}// SelectCursorCharacter


/*!
Sets the selection to the line of the current cursor location.

This is useful for beginning a completely-keyboard-driven
text selection.

(3.1)
*/
void
TerminalView_SelectCursorLine	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.inhibited))
	{
		Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		// find the new cursor region
		getCursorLocationError = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
		if (kTerminal_ResultOK == getCursorLocationError)
		{
			highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
			viewPtr->text.selection.range.first = std::make_pair(0, cursorY);
			viewPtr->text.selection.range.second = std::make_pair(Terminal_ReturnColumnCount(viewPtr->screen.ref) + 1, cursorY + 1);
			highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
			viewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
		}
	}
}// SelectCursorLine


/*!
Highlights the entire screen buffer, which
includes the entire scrollback as well as the
visible screen area.

(2.6)
*/
void
TerminalView_SelectEntireBuffer		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.inhibited))
	{
		highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
		// NOTE that the screen is anchored with a (0, 0) origin at the top of the bottommost
		// windowful, so a negative offset is required to capture the entire scrollback buffer
		viewPtr->text.selection.range.first = std::make_pair(0, -Terminal_ReturnInvisibleRowCount(viewPtr->screen.ref));
		viewPtr->text.selection.range.second = std::make_pair(Terminal_ReturnColumnCount(viewPtr->screen.ref),
																Terminal_ReturnRowCount(viewPtr->screen.ref));
		highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
	}
}// SelectEntireBuffer


/*!
Highlights the primary screen buffer, which
does not include the scrollback but does
include the rest of the screen area.

(2.6)
*/
void
TerminalView_SelectMainScreen	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.inhibited))
	{
		highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.range.first = std::make_pair(0, 0);
		viewPtr->text.selection.range.second = std::make_pair(Terminal_ReturnColumnCount(viewPtr->screen.ref),
																Terminal_ReturnRowCount(viewPtr->screen.ref));
		highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
	}
}// SelectMainScreen


/*!
Clears the text selection for the specified screen.

(2.6)
*/
void
TerminalView_SelectNothing	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
	{
		highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.range.first = std::make_pair(0, 0);
		viewPtr->text.selection.range.second = std::make_pair(0, 0);
		viewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
	}
}// SelectNothing


/*!
Highlights the specified range of text, clearing any other
selection outside the range.

In version 3.0, the exact characters selected depends on the
current selection mode: in normal mode, the specified anchors
mark the first and last characters of a Macintosh-style
selection range.  However, if the new “rectangular text
selection” mode is being used, the width of each line of the
selected region is the same, equal to the difference between
the horizontal coordinates of the given points.

(2.6)
*/
void
TerminalView_SelectVirtualRange		(TerminalViewRef				inView,
									 TerminalView_CellRange const&	inSelection)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.inhibited))
	{
		highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.range = inSelection;
		highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
	}
}// SelectVirtualRange


/*!
Sets a new value for a specific color in a
terminal window.  Returns "true" only if the
color could be set.

(3.0)
*/
Boolean
TerminalView_SetColor	(TerminalViewRef			inView,
						 TerminalView_ColorIndex	inColorEntryNumber,
						 CGFloatRGBColor const*		inColorPtr)
{
	Boolean		result = false;
	
	
	if ((inColorEntryNumber <= kTerminalView_ColorIndexLastValid) &&
		(inColorEntryNumber >= kTerminalView_ColorIndexFirstValid))
	{
		My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
		
		
		setScreenBaseColor(viewPtr, inColorEntryNumber, inColorPtr);
		result = true;
	}
	
	return result;
}// SetColor


/*!
Specifies whether or not the given view is capable of displaying
the cursor at all!  Changing this flag will automatically update
the cursor to reflect the new permanent state.

It is typical to hide the cursor of a view in a split-view
situation, where only one view is expected to be the target at
any one time.  Displaying multiple flashing cursors to the user
for the same terminal buffer can be confusing.

See also TerminalView_SetUserInteractionEnabled().

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

(4.0)
*/
TerminalView_Result
TerminalView_SetCursorRenderingEnabled	(TerminalViewRef	inView,
										 Boolean			inIsEnabled)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		setCursorVisibility(viewPtr, inIsEnabled);
		viewPtr->screen.cursor.inhibited = (false == inIsEnabled);
	}
	
	return result;
}// SetCursorRenderingEnabled


/*!
Changes the current display mode, which is normal by default.
Query it later with TerminalView_ReturnDisplayMode().

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the screen reference is unrecognized

(3.1)
*/
TerminalView_Result
TerminalView_SetDisplayMode		(TerminalViewRef			inView,
								 TerminalView_DisplayMode   inNewMode)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		viewPtr->displayMode = inNewMode;
	}
	return result;
}// SetDisplayMode


/*!
Specifies whether screen updates should be serviced
for a particular terminal screen.  If drawing is
disabled, events that would normally trigger screen
updates will not cause updates.

(2.6)
*/
void
TerminalView_SetDrawingEnabled	(TerminalViewRef	inView,
								 Boolean			inIsDrawingEnabled)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	Console_Warning(Console_WriteLine, "set-drawing-enabled not implemented for Cocoa");
}// SetDrawingEnabled


/*!
Specifies whether or not to make room around the edges
of the content area for a border, showing the focus ring
around the content area when it is accepting keyboard
input.

Disabling focus ring display is not recommended, because
it is possible for a terminal to lose focus (when shifted
to a toolbar item or floating window, for example) and
the user should see when this occurs.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

(3.1)
*/
TerminalView_Result
TerminalView_SetFocusRingDisplayed	(TerminalViewRef	inView,
									 Boolean			inShowFocusRingAndMatte)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else viewPtr->screen.focusRingEnabled = inShowFocusRingAndMatte;
	
	return result;
}// SetFocusRingDisplayed


/*!
Sets the font and size for the specified screen’s text.
Pass nullptr if the font name should be ignored (and
therefore not changed).  Pass 0 if the font size should
not be changed.

You cannot change the font size if the display mode is
currently setting the size automatically.  Use the
TerminalView_ReturnDisplayMode() routine to determine
what the display mode is.

Remember, this only affects a single screen!  It is now
customary in MacTerm to perform these kinds of actions
at the highest level possible; therefore, if you are
responding to a user command, look for a similar API at,
say, the Terminal Window level.

You must redraw the screen yourself.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultIllegalOperation
if the display mode currently sets the size automatically

(3.0)
*/
TerminalView_Result
TerminalView_SetFontAndSize		(TerminalViewRef	inView,
								 CFStringRef		inFontFamilyNameOrNull,
								 CGFloat			inFontSizeOrZero)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr != nullptr)
	{
		if ((inFontSizeOrZero > 0) && (inFontSizeOrZero != viewPtr->text.font.normalMetrics.size) &&
			(viewPtr->displayMode == kTerminalView_DisplayModeZoom))
		{
			// the font size may not be changed explicitly if it is currently being controlled automatically
			result = kTerminalView_ResultIllegalOperation;
		}
		else
		{
			setFontAndSize(viewPtr, inFontFamilyNameOrNull, inFontSizeOrZero);
		}
	}
	
	return result;
}// SetFontAndSize


/*!
Specifies whether or not a resize of the view will cause the
underlying terminal buffer’s screen dimensions to change (to
the size that best fills the space).  This should usually be
enabled, but for split-panes it can make sense to view only a
portion of the screen area.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

(4.0)
*/
TerminalView_Result
TerminalView_SetResizeScreenBufferWithView		(TerminalViewRef	inView,
												 Boolean			inScreenDimensionsAutoSync)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		viewPtr->screen.sizeNotMatchedWithView = (false == inScreenDimensionsAutoSync);
	}
	
	return result;
}// SetResizeScreenBufferWithView


/*!
Specifies whether or not the given view is capable of displaying
text selections at all!  Clearing this flag will automatically
remove any highlighting currently shown in the view.

If you disable rendering, you should generally also disable user
interaction with TerminalView_SetUserInteractionEnabled().

IMPORTANT:	Since the highlighted state of text is tied to its
			underlying attributes, it is crucial to use this API
			when multiple views share the same storage (such as
			in a split view).  At most one of the views sharing
			the same buffer should allow selections at any time.
			You may however provide some mechanism for switching
			which view currently displays the selection.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

(4.0)
*/
TerminalView_Result
TerminalView_SetTextSelectionRenderingEnabled	(TerminalViewRef	inView,
												 Boolean			inIsEnabled)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		if (false == inIsEnabled)
		{
			TerminalView_SelectNothing(inView);
		}
		
		viewPtr->text.selection.inhibited = (false == inIsEnabled);
	}
	
	return result;
}// SetTextSelectionRenderingEnabled


/*!
Specifies whether or not the given view responds to mouse clicks
or keystrokes, or any other user actions that can affect the
view.  This does not prevent API calls from making changes, and
it does NOT prevent other monitors from accepting user input
(such as an attached Session, that typically routes keystrokes
to a program and ends up causing text to appear in the view).

You typically use this to “disable” a view that isn’t meant to
be interacted with, such as a sample display.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

(4.0)
*/
TerminalView_Result
TerminalView_SetUserInteractionEnabled	(TerminalViewRef	inView,
										 Boolean			inIsEnabled)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		viewPtr->text.selection.readOnly = (false == inIsEnabled);
	}
	
	return result;
}// SetUserInteractionEnabled


/*!
Arranges for a callback to be invoked whenever an event
occurs on a view (such as scrolling).

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to an event.  See "TerminalView.h" for comments
			on what the context means for each event type.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

\retval kTerminalView_ResultParameterError
if the specified event could not be monitored successfully

(3.0)
*/
TerminalView_Result
TerminalView_StartMonitoring	(TerminalViewRef			inView,
								 TerminalView_Event			inForWhatEvent,
								 ListenerModel_ListenerRef	inListener)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		Boolean		addOK = false;
		
		
		// add a listener to the specified target’s listener model for the given event
		addOK = ListenerModel_AddListenerForEvent(viewPtr->changeListenerModel, inForWhatEvent, inListener);
		if (false == addOK)
		{
			result = kTerminalView_ResultParameterError;
		}
	}
	return result;
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked whenever an
event occurs for a view (such as scrolling).

IMPORTANT:	This routine cancels the effects of a previous
			call to TerminalView_StartMonitoring() - your
			parameters must match the previous start-call,
			or the stop will fail.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

(3.0)
*/
TerminalView_Result
TerminalView_StopMonitoring		(TerminalViewRef			inView,
								 TerminalView_Event			inForWhatEvent,
								 ListenerModel_ListenerRef	inListener)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		// remove a listener from the specified target’s listener model for the given event
		ListenerModel_RemoveListenerForEvent(viewPtr->changeListenerModel, inForWhatEvent, inListener);
	}
	return result;
}// StopMonitoring


/*!
Determines whether there is currently any text
selected in the specified screen.

(2.6)
*/
Boolean
TerminalView_TextSelectionExists	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						result = false;
	
	
	if (nullptr != viewPtr)
	{
		result = selectionExists(viewPtr);
	}
	return result;
}// TextSelectionExists


/*!
Returns "true" only if text selections in the specified
view are currently forced to be rectangular.

Version 3.0 supports two modes of text selection: standard,
like most Macintosh applications, and rectangular, useful
for capturing columns of output (for example, data from a
UNIX command like "top").

(3.0)
*/
Boolean
TerminalView_TextSelectionIsRectangular		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						result = 0;
	
	
	result = viewPtr->text.selection.isRectangular;
	return result;
}// TextSelectionIsRectangular


/*!
A utility for converting from the range format used by a
terminal screen buffer, into the range format used by the
view for rendering.

Changes to the view will not invalidate the results, but
any change to the screen buffer (such as scrolling text
off the end of the scrollback, or clearing it) will
invalidate these results.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the specified view reference is not valid

(3.1)
*/
TerminalView_Result
TerminalView_TranslateTerminalScreenRange	(TerminalViewRef					inView,
											 Terminal_RangeDescription const&	inRange,
											 TerminalView_CellRange&			outRange)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		outRange.first = TerminalView_Cell(inRange.firstColumn, inRange.firstRow);
		outRange.second = TerminalView_Cell(outRange.first.first + inRange.columnCount,
											outRange.first.second + inRange.rowCount);
	}
	return result;
}// TranslateTerminalScreenRange


/*!
Displays an “opening” animation from the current text selection.
This is currently used for opening URLs.

(4.0)
*/
void
TerminalView_ZoomOpenFromSelection		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr != viewPtr)
	{
		Console_Warning(Console_WriteLine, "zoom-open-from-selection not implemented for Cocoa");
	}
}// ZoomOpenFromSelection


/*!
Displays an animation that helps the user locate
the terminal cursor.

(3.0)
*/
void
TerminalView_ZoomToCursor	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
	{
		Console_Warning(Console_WriteLine, "zoom-to-cursor not implemented for Cocoa");
	}
}// ZoomToCursor


/*!
Displays an animation that helps the user locate
search results.  The animation focuses on whichever
match is considered the current match.

(3.1)
*/
void
TerminalView_ZoomToSearchResults	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.searchResults.empty()))
	{
		// try to scroll to the right part of the terminal view
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollToCell(inView, viewPtr->text.toCurrentSearchResult->first);
		
		Console_Warning(Console_WriteLine, "zoom-to-search-results not implemented for Cocoa");
	}
}// ZoomToSearchResults


#pragma mark Internal Methods
namespace {

/*!
Initializes all tables, after which they can be used to
conveniently translate received parameter values in XTerm
256-color syntax, into RGB or gray values.

(4.0)
*/
My_XTerm256Table::
My_XTerm256Table ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
grayLevels(),
colorLevels()
{
	// most colors are defined as a 6x6x6 cube; note that the
	// following is completely arbitrary, and terminal sequences
	// (eventually) may trigger setColor() to change any of these
	// at the request of a program running in the terminal
	UInt8 const		kCubeBase = 16; // first index
	for (UInt8 red = 0; red < 6; ++red)
	{
		for (UInt8 green = 0; green < 6; ++green)
		{
			for (UInt8 blue = 0; blue < 6; ++blue)
			{
				UInt8 const		kIndex = STATIC_CAST(kCubeBase + (36 * red) + (6 * green) + blue, UInt8);
				UInt8 const	kRedLevel = (red) ? (55 + 40 * red) : 0;
				UInt8 const	kGreenLevel = (green) ? (55 + 40 * green) : 0;
				UInt8 const	kBlueLevel = (blue) ? (55 + 40 * blue) : 0;
				RGBLevels		levels;
				
				
				levels.reserve(3);
				levels.push_back(kRedLevel);
				levels.push_back(kGreenLevel);
				levels.push_back(kBlueLevel);
				assert(3 == levels.size());
				this->colorLevels[kIndex] = levels;
			}
		}
	}
	
	// some colors are simply levels of gray
	UInt8 const		kGrayBase = 232; // first index
	for (UInt8 gray = 0; gray < 24; ++gray)
	{
		UInt8 const		kIndex = (kGrayBase + gray);
		UInt8 const		kGrayLevel = 8 + 10 * gray;
		
		
		this->grayLevels[kIndex] = kGrayLevel;
	}
}// My_XTerm256Table


/*!
Fills in a CGFloatRGBColor structure with appropriate
values based on the given 8-bit components.

(4.0)
*/
void
My_XTerm256Table::
makeCGFloatRGBColor		(UInt8				inRed,
						 UInt8				inGreen,
						 UInt8				inBlue,
						 CGFloatRGBColor&	outColor)
{
	Float32		fullIntensityFraction = 0.0;
	
	
	fullIntensityFraction = inRed;
	fullIntensityFraction /= 255;
	outColor.red = fullIntensityFraction;
	
	fullIntensityFraction = inGreen;
	fullIntensityFraction /= 255;
	outColor.green = fullIntensityFraction;
	
	fullIntensityFraction = inBlue;
	fullIntensityFraction /= 255;
	outColor.blue = fullIntensityFraction;
}// My_XTerm256Table::makeCGFloatRGBColor


/*!
Sets a color in the table to a new value.  Only the normal
color range can be customized; the base 16 colors and the
grayscale range will always come from a different source
(even if they are set here).

(4.0)
*/
void
My_XTerm256Table::
setColor	(UInt8		inIndex,
			 UInt8		inRed,
			 UInt8		inGreen,
			 UInt8		inBlue)
{
	My_XTerm256Table::RGBLevelsByIndex::iterator	toColors = colorLevels.find(inIndex);
	
	
	if (colorLevels.end() == colorLevels.find(inIndex))
	{
		Console_Warning(Console_WriteValue, "warning, attempt to set color in reserved range (base-16 or gray scale range)", inIndex);
	}
	else
	{
		assert(3 == toColors->second.size());
		toColors->second[0] = inRed;
		toColors->second[1] = inGreen;
		toColors->second[2] = inBlue;
	}
}// My_XTerm256Table::setColor


/*!
Constructor for Cocoa windows.

WARNING:	This constructor leaves the class uninitialized!
			You MUST invoke the initialize() method before
			doing anything with it.  The code is currently
			structured this way for historical reasons, due to
			roots as a Carbon HIObject.

The Cocoa transition is not complete, so this is not going to
create a functional Cocoa view, yet.  It is scaffolding to
allow some other changes.

(4.0)
*/
My_TerminalView::
My_TerminalView		(TerminalView_Object*	inRootView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef(REINTERPRET_CAST(this, TerminalViewRef)),
encodingConfig(), // set later
formatConfig(), // set later
configFilter(),
changeListenerModel(nullptr), // set later
displayMode(kTerminalView_DisplayModeNormal), // set later
isActive(true),
encompassingNSView(inRootView)
{
}// My_TerminalView 3-argument constructor (NSView*)


/*!
Initializer.

IMPORTANT:	Settings that are read from "inFormat" here
			and cached in the class, need to also be updated
			in TerminalView_ReturnFormatConfiguration()
			before a context is returned from that API!
			This ensures that external editors are seeing
			accurate settings.  Put another way, the calls
			to Preferences_ContextGetData() here should be
			balanced by Preferences_ContextSetData() calls
			in TerminalView_ReturnFormatConfiguration().

(3.1)
*/
void
My_TerminalView::
initialize		(TerminalScreenRef			inScreenDataSource,
				 Preferences_ContextRef		inFormat)
{
	this->selfRef = REINTERPRET_CAST(this, TerminalViewRef);
	
	this->encodingConfig.setWithNoRetain(Preferences_NewContext(Quills::Prefs::TRANSLATION));
	assert(this->encodingConfig.exists());
	{
		Boolean		setOK = TextTranslation_ContextSetEncoding(this->encodingConfig.returnRef(), kCFStringEncodingUTF8);
		
		
		assert(setOK);
	}
	
	this->formatConfig.setWithNoRetain(Preferences_NewCloneContext(inFormat, true/* detach */));
	assert(this->formatConfig.exists());
	
	this->changeListenerModel = ListenerModel_New(kListenerModel_StyleStandard,
													kConstantsRegistry_ListenerModelDescriptorTerminalViewChanges);
	
	// automatically read the user preference for window resize behavior
	// and initialize appropriately
	{
		Boolean		affectsFontSize = false;
		
		
		if (kPreferences_ResultOK !=
			Preferences_GetData(kPreferences_TagTerminalResizeAffectsFontSize, sizeof(affectsFontSize), &affectsFontSize))
		{
			// assume a default, if the value cannot be found...
			affectsFontSize = false;
		}
		this->displayMode = (affectsFontSize) ? kTerminalView_DisplayModeZoom : kTerminalView_DisplayModeNormal;
	}
	
	// retain the screen reference
	this->screen.ref = nullptr; // initially (asserted by addDataSource())
	assert(addDataSource(this, inScreenDataSource));
	
	// miscellaneous settings
	this->text.attributes = TextAttributes_Object();
	this->text.attributeDict = [[NSMutableDictionary alloc] initWithCapacity:4/* arbitrary initial size */];
	this->text.font.normalFont = nil; // set later
	this->text.font.boldFont = nil; // set later
	this->text.selection.range.first.first = 0;
	this->text.selection.range.first.second = 0;
	this->text.selection.range.second.first = 0;
	this->text.selection.range.second.second = 0;
	this->text.selection.keyboardMode = kMy_SelectionModeUnset;
	this->text.selection.isRectangular = false;
	this->text.selection.readOnly = false;
	this->text.selection.inhibited = false;
	this->screen.leftVisibleEdgeInColumns = 0;
	this->screen.topVisibleEdgeInRows = 0;
	this->screen.cache.viewWidthInPixels.setIntegralPixels(0); // set later...
	this->screen.cache.viewHeightInPixels.setIntegralPixels(0); // set later...
	this->screen.cache.columnCountAtLastNotify = 0; // updated when triggered
	this->screen.cache.rowCountAtLastNotify = 0; // updated when triggered
	this->screen.sizeNotMatchedWithView = false;
	this->screen.focusRingEnabled = true;
	this->screen.isReverseVideo = 0;
	this->screen.cursor.currentState = kMyCursorStateVisible;
	this->screen.cursor.ghostState = kMyCursorStateInvisible;
	this->screen.cursor.updatedShape = HIShapeCreateMutable();
	this->screen.cursor.inhibited = false;
	this->screen.cursor.isCustomColor = false;
	this->screen.mouse.pointerColor = kTerminalView_MousePointerColorRed; // set later
	this->screen.currentRenderContext = nullptr;
	this->text.toCurrentSearchResult = this->text.searchResults.end();
	
	// read user preferences for the spacing around the edges
	{
		Preferences_Result	preferencesResult = kPreferences_ResultOK;
		Float32				preferenceFloatValue = 0.0;
		
		
		// margins
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginLeft,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.marginLeftEmScale = preferenceFloatValue;
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginRight,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.marginRightEmScale = preferenceFloatValue;
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginTop,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.marginTopEmScale = preferenceFloatValue;
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginBottom,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.marginBottomEmScale = preferenceFloatValue;
		
		// paddings
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingLeft,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.paddingLeftEmScale = preferenceFloatValue;
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingRight,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.paddingRightEmScale = preferenceFloatValue;
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingTop,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.paddingTopEmScale = preferenceFloatValue;
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingBottom,
														sizeof(preferenceFloatValue), &preferenceFloatValue,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		this->screen.paddingBottomEmScale = preferenceFloatValue;
	}
	
	// store the colors this view will be using (also initializes mouse pointer color)
	UNUSED_RETURN(Boolean)createWindowColorPalette(this, inFormat);
	
	// copy font defaults
	{
		UInt16		preferenceCount = copyFontPreferences(this, inFormat, true/* search defaults too */);
		
		
		assert(preferenceCount > 0);
	}
	
	// create views
	if (this->isCocoa())
	{
		// read optional image URL
		// UNIMPLEMENTED
		
		// NOTE: in the Cocoa implementation, all views are passed in to the routine that
		// constructs the Terminal View, so views do not need to be created here; but,
		// it is necessary to associate the Terminal View object with them
		this->encompassingNSView.internalViewPtr = this;
		this->encompassingNSView.terminalContentView.internalViewPtr = this;
		this->encompassingNSView.terminalMarginViewBottom.internalViewPtr = this;
		this->encompassingNSView.terminalMarginViewLeft.internalViewPtr = this;
		this->encompassingNSView.terminalMarginViewRight.internalViewPtr = this;
		this->encompassingNSView.terminalMarginViewTop.internalViewPtr = this;
		this->encompassingNSView.terminalPaddingViewBottom.internalViewPtr = this;
		this->encompassingNSView.terminalPaddingViewLeft.internalViewPtr = this;
		this->encompassingNSView.terminalPaddingViewRight.internalViewPtr = this;
		this->encompassingNSView.terminalPaddingViewTop.internalViewPtr = this;
		
		// NOTE: initializing the padding and background view colors is not really necessary
		// (by the time the views are displayed, they will have been updated in the same way
		// that they are whenever the user changes preferences later); however, it is
		// important to tell the views which color index to use
		this->encompassingNSView.terminalMarginViewBottom.colorIndex = kMyBasicColorIndexMatteBackground;
		this->encompassingNSView.terminalMarginViewLeft.colorIndex = kMyBasicColorIndexMatteBackground;
		this->encompassingNSView.terminalMarginViewRight.colorIndex = kMyBasicColorIndexMatteBackground;
		this->encompassingNSView.terminalMarginViewTop.colorIndex = kMyBasicColorIndexMatteBackground;
		this->encompassingNSView.terminalPaddingViewBottom.colorIndex = kMyBasicColorIndexNormalBackground;
		this->encompassingNSView.terminalPaddingViewLeft.colorIndex = kMyBasicColorIndexNormalBackground;
		this->encompassingNSView.terminalPaddingViewRight.colorIndex = kMyBasicColorIndexNormalBackground;
		this->encompassingNSView.terminalPaddingViewTop.colorIndex = kMyBasicColorIndexNormalBackground;
	}
	
	// initialize blink animation
	{
		this->animation.rendering.delays.resize(kMy_BlinkingColorCount);
		for (My_TimeIntervalList::size_type i = 0; i < this->animation.rendering.delays.size(); ++i)
		{
			this->animation.rendering.delays[i] = calculateAnimationStageDelay(this, i);
		}
		this->animation.rendering.stage = 0;
		this->animation.rendering.stageDelta = +1;
		this->animation.rendering.region = HIShapeCreateMutable();
		this->animation.cursor.blinkAlpha = 1.0;
	}
	
	// set up a callback to receive preference change notifications
	this->screen.preferenceMonitor.setWithNoRetain(ListenerModel_NewStandardListener
													(preferenceChangedForView, this->selfRef/* context */));
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring(this->screen.preferenceMonitor.returnRef(), kPreferences_TagDontDimBackgroundScreens,
													true/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up monitor for no-terminal-dim setting, error", prefsResult);
		}
		prefsResult = Preferences_StartMonitoring(this->screen.preferenceMonitor.returnRef(), kPreferences_TagTerminalCursorType,
													false/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up monitor for cursor-shape setting, error", prefsResult);
		}
		prefsResult = Preferences_StartMonitoring(this->screen.preferenceMonitor.returnRef(), kPreferences_TagTerminalResizeAffectsFontSize,
													false/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up monitor for resize-effect setting, error", prefsResult);
		}
		prefsResult = Preferences_ContextStartMonitoring(this->encodingConfig.returnRef(), this->screen.preferenceMonitor.returnRef(),
															kPreferences_ChangeContextBatchMode);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up monitor for batch-mode changes to translation preferences of terminal view, error", prefsResult);
		}
		prefsResult = Preferences_ContextStartMonitoring(this->formatConfig.returnRef(), this->screen.preferenceMonitor.returnRef(),
															kPreferences_ChangeContextBatchMode);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set up monitor for batch-mode changes to format preferences of terminal view, error", prefsResult);
		}
	}
	
	// now that everything is initialized, start watching for data changes
	assert(startMonitoringDataSource(this, inScreenDataSource));
}// My_TerminalView::initialize


/*!
Destructor.

(3.0)
*/
My_TerminalView::
~My_TerminalView ()
{
	if (nil != this->text.font.normalFont)
	{
		this->text.font.normalFont = nil;
	}
	if (nil != this->text.font.boldFont)
	{
		this->text.font.boldFont = nil;
	}
	
	// if the table is not the global one, a copy was allocated
	if (&gColorGrid() != this->extendedColorsPtr)
	{
		delete this->extendedColorsPtr;
	}
	
	// stop watching for data changes
	assert(stopMonitoringDataSource(this, nullptr/* specific screen or nullptr for all screens */));
	
	// stop receiving preference change notifications
	Preferences_StopMonitoring(this->screen.preferenceMonitor.returnRef(), kPreferences_TagDontDimBackgroundScreens);
	Preferences_StopMonitoring(this->screen.preferenceMonitor.returnRef(), kPreferences_TagTerminalCursorType);
	Preferences_StopMonitoring(this->screen.preferenceMonitor.returnRef(), kPreferences_TagTerminalResizeAffectsFontSize);
	UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(this->encodingConfig.returnRef(), this->screen.preferenceMonitor.returnRef(),
																		kPreferences_ChangeContextBatchMode);
	UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(this->formatConfig.returnRef(), this->screen.preferenceMonitor.returnRef(),
																		kPreferences_ChangeContextBatchMode);
	
	// remove timers
	if (nil != this->animation.timer.objectRef)
	{
		[this->animation.timer.objectRef invalidate];
		this->animation.timer.objectRef = nil;
	}
	
	CFRelease(this->animation.rendering.region); this->animation.rendering.region = nullptr;
	CFRelease(this->screen.cursor.updatedShape); this->screen.cursor.updatedShape = nullptr;
	ListenerModel_Dispose(&this->changeListenerModel);
	
	// release strong references to data sources
	assert(removeDataSource(this, nullptr/* specific screen or nullptr for all screens */));
}// My_TerminalView destructor


/*!
Returns true only if this structure was initialized as a
modern (Cocoa and Core Graphics) user interface instead of
a legacy (Carbon and QuickDraw) interface.

(2018.02)
*/
bool
My_TerminalView::
isCocoa ()
const
{
	return true;
}// My_TerminalView::isCocoa


/*!
Specifies a terminal buffer whose data will be displayed
by the terminal view and returns true only if successful.

See also startMonitoringDataSource().

IMPORTANT: Currently the implementation is limited to a
single data source so this routine will fail if any
other data source is already set.  A previous source can
be removed with removeDataSource().

(4.1)
*/
Boolean
addDataSource	(My_TerminalViewPtr		inTerminalViewPtr,
				 TerminalScreenRef		inScreenDataSource)
{
	Boolean		result = false;
	
	
	// currently only one source is possible so the implementation
	// is more straightforward (IMPORTANT: the steps below should
	// be the inverse of the code in removeDataSource())
	if (nullptr == inTerminalViewPtr->screen.ref)
	{
		TerminalScreenRef	screenRef = inScreenDataSource;
		
		
		inTerminalViewPtr->screen.ref = screenRef;
		Terminal_RetainScreen(screenRef);
		
		result = true;
	}
	
	return result;
}// addDataSource


/*!
Changes the blinking text colors of a terminal view,
and modifies the cursor-flashing alpha channel.

This very efficient and simple animation scheme
allows text to “really blink”, because this routine
is called regularly by a timer.

Timers that draw must save and restore the current
graphics port.

(3.0)
*/
void
animateBlinkingItems	(NSTimer*			inTimer,
						 TerminalViewRef	inTerminalViewRef)
{
	My_TerminalViewAutoLocker	ptr(gTerminalViewPtrLocks(), inTerminalViewRef);
	
	
	if ((ptr != nullptr) && (inTimer.valid))
	{
		CGFloatRGBColor		currentColor;
		
		
		// for simplicity, keep the cursor and text blinks in sync
		
		inTimer.fireDate = [NSDate dateWithTimeIntervalSinceNow:ptr->animation.rendering.delays[ptr->animation.rendering.stage]];
		
		//
		// blinking text
		//
		
		// update the rendered text color of the screen
		getBlinkAnimationColor(ptr, ptr->animation.rendering.stage, &currentColor);
		setScreenCustomColor(ptr, kTerminalView_ColorIndexBlinkingText, &currentColor);
		
		// figure out which color is next; the color cycling goes up and
		// down the list continuously, thus creating a pulsing effect
		ptr->animation.rendering.stage += ptr->animation.rendering.stageDelta;
		if (ptr->animation.rendering.stage < 0)
		{
			ptr->animation.rendering.stageDelta = +1;
			ptr->animation.rendering.stage = 0;
		}
		else if (ptr->animation.rendering.stage >= kMy_BlinkingColorCount)
		{
			ptr->animation.rendering.stageDelta = -1;
			ptr->animation.rendering.stage = kMy_BlinkingColorCount - 1;
		}
		
		// invalidate only the appropriate (blinking) parts of the screen
		updateDisplayInShape(ptr, ptr->animation.rendering.region);
		
		//
		// cursor
		//
		
		// adjust the alpha setting to be used for cursor-drawing
		ptr->animation.cursor.blinkAlpha = kAlphaByPhase[ptr->animation.rendering.stage];
		
		// invalidate the cursor
		updateDisplayInShape(ptr, ptr->screen.cursor.updatedShape);
	}
}// animateBlinkingItems


/*!
Responds to terminal bells visually, if visual bells
are enabled.

Also updates the bell toolbar item when sound is
enabled or disabled for a terminal.

(3.0)
*/
void
audioEvent	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
			 ListenerModel_Event	inEventThatOccurred,
			 void*					UNUSED_ARGUMENT(inEventContextPtr),
			 void*					inTerminalViewRef)
{
	TerminalViewRef		inView = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	
	
	switch (inEventThatOccurred)
	{
	case kTerminal_ChangeAudioEvent:
		{
			//TerminalScreenRef	inScreen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
			
			
			if (nullptr != viewPtr)
			{
				visualBell(viewPtr);
				
				if (gPreferenceProxies.notifyOfBeeps)
				{
					[NSApp requestUserAttention:NSInformationalRequest];
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// audioEvent


/*!
For the specified stage of blink animation, calculates
the delay that should come after that stage is rendered.

For linear animation, all delays are the same; but for
more interesting effects, all delays may be different.

(3.1)
*/
NSTimeInterval
calculateAnimationStageDelay	(My_TerminalViewPtr					inTerminalViewPtr,
								 My_TimeIntervalList::size_type		inZeroBasedStage)
{
	My_TimeIntervalList::size_type const	kNumStages = inTerminalViewPtr->animation.rendering.delays.size();
	assert(inZeroBasedStage < kNumStages);
	
	NSTimeInterval		result = 0;
	
	
	//result = 200.0 * 0.001/* convert from milliseconds to seconds */; // linear
	result = (inZeroBasedStage * inZeroBasedStage + inZeroBasedStage) * 2.0 * 0.001/* convert from milliseconds to seconds */; // quadratic
	return result;
}// calculateAnimationStageDelay


/*!
Attempts to read all supported color tags from the given
preference context, and any colors that exist will be
used to update the specified view.

Returns the number of colors that were changed.

(3.1)
*/
UInt16
copyColorPreferences	(My_TerminalViewPtr			inTerminalViewPtr,
						 Preferences_ContextRef		inSource,
						 Boolean					inSearchForDefaults)
{
	TerminalView_ColorIndex		currentColorID = 0;
	UInt16						currentIndex = 0;
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	Preferences_Tag				currentPrefsTag = '----';
	CGFloatRGBColor				colorValue;
	UInt16						result = 0;
	
	
	//
	// basic colors
	//
	
	currentColorID = kTerminalView_ColorIndexNormalText;
	currentPrefsTag = kPreferences_TagTerminalColorNormalForeground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	currentColorID = kTerminalView_ColorIndexNormalBackground;
	currentPrefsTag = kPreferences_TagTerminalColorNormalBackground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	currentColorID = kTerminalView_ColorIndexBlinkingText;
	currentPrefsTag = kPreferences_TagTerminalColorBlinkingForeground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	currentColorID = kTerminalView_ColorIndexBlinkingBackground;
	currentPrefsTag = kPreferences_TagTerminalColorBlinkingBackground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	currentColorID = kTerminalView_ColorIndexBoldText;
	currentPrefsTag = kPreferences_TagTerminalColorBoldForeground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	currentColorID = kTerminalView_ColorIndexBoldBackground;
	currentPrefsTag = kPreferences_TagTerminalColorBoldBackground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	currentColorID = kTerminalView_ColorIndexCursorBackground;
	currentPrefsTag = kPreferences_TagTerminalColorCursorBackground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	// when reading the cursor color, also copy in the “auto-set color” flag
	{
		Boolean		isAutoSet = false;
		
		
		prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagAutoSetCursorColor,
													sizeof(isAutoSet), &isAutoSet, inSearchForDefaults);
		if (kPreferences_ResultOK == prefsResult)
		{
			inTerminalViewPtr->screen.cursor.isCustomColor = (false == isAutoSet);
		}
		else
		{
			inTerminalViewPtr->screen.cursor.isCustomColor = false;
		}
	}
	
	currentColorID = kTerminalView_ColorIndexMatteBackground;
	currentPrefsTag = kPreferences_TagTerminalColorMatteBackground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenBaseColor(inTerminalViewPtr, currentColorID, &colorValue);
		++result;
	}
	
	// also update preferred mouse pointer color
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, kPreferences_TagTerminalMousePointerColor,
															sizeof(inTerminalViewPtr->screen.mouse.pointerColor),
															&inTerminalViewPtr->screen.mouse.pointerColor,
															inSearchForDefaults))
	{
		++result;
		
		// immediately invalidate so the cursor is updated by the OS
		[[inTerminalViewPtr->encompassingNSView window] invalidateCursorRectsForView:inTerminalViewPtr->encompassingNSView.terminalContentView];
	}
	
	//
	// ANSI colors
	//
	
	currentIndex = kTerminalView_ColorIndexNormalANSIBlack;
	currentPrefsTag = kPreferences_TagTerminalColorANSIBlack;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexNormalANSIRed;
	currentPrefsTag = kPreferences_TagTerminalColorANSIRed;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexNormalANSIGreen;
	currentPrefsTag = kPreferences_TagTerminalColorANSIGreen;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexNormalANSIYellow;
	currentPrefsTag = kPreferences_TagTerminalColorANSIYellow;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexNormalANSIBlue;
	currentPrefsTag = kPreferences_TagTerminalColorANSIBlue;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexNormalANSIMagenta;
	currentPrefsTag = kPreferences_TagTerminalColorANSIMagenta;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexNormalANSICyan;
	currentPrefsTag = kPreferences_TagTerminalColorANSICyan;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexNormalANSIWhite;
	currentPrefsTag = kPreferences_TagTerminalColorANSIWhite;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSIBlack;
	currentPrefsTag = kPreferences_TagTerminalColorANSIBlackBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSIRed;
	currentPrefsTag = kPreferences_TagTerminalColorANSIRedBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSIGreen;
	currentPrefsTag = kPreferences_TagTerminalColorANSIGreenBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSIYellow;
	currentPrefsTag = kPreferences_TagTerminalColorANSIYellowBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSIBlue;
	currentPrefsTag = kPreferences_TagTerminalColorANSIBlueBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSIMagenta;
	currentPrefsTag = kPreferences_TagTerminalColorANSIMagentaBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSICyan;
	currentPrefsTag = kPreferences_TagTerminalColorANSICyanBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	currentIndex = kTerminalView_ColorIndexEmphasizedANSIWhite;
	currentPrefsTag = kPreferences_TagTerminalColorANSIWhiteBold;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setScreenCoreColor(inTerminalViewPtr, currentIndex, &colorValue);
		++result;
	}
	
	return result;
}// copyColorPreferences


/*!
Attempts to read all supported font tags from the given
preference context, and any font information that exists
will be used to update the specified view.

Note that the font size cannot be changed while in zoom
mode, because the view size determines the font size.

Returns the number of font settings that were changed.

(3.1)
*/
UInt16
copyFontPreferences		(My_TerminalViewPtr			inTerminalViewPtr,
						 Preferences_ContextRef		inSource,
						 Boolean					inSearchDefaults)
{
	UInt16			result = 0;
	CFStringRef		fontName = nullptr;
	Float64			fontSize = 0;
	Float32			charWidthScale = 1.0;
	
	
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, kPreferences_TagFontName,
															sizeof(fontName), &fontName, inSearchDefaults))
	{
		++result;
	}
	
	if (inTerminalViewPtr->displayMode != kTerminalView_DisplayModeZoom)
	{
		if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, kPreferences_TagFontSize,
																sizeof(fontSize), &fontSize, inSearchDefaults))
		{
			++result;
		}
	}
	
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, kPreferences_TagFontCharacterWidthMultiplier,
															sizeof(charWidthScale), &charWidthScale, inSearchDefaults))
	{
		++result;
	}
	else
	{
		charWidthScale = 0; // do not change
	}
	
	// set font size to automatically fill in initial font metrics, etc.
	setFontAndSize(inTerminalViewPtr, fontName, fontSize, charWidthScale);
	
	return result;
}// copyFontPreferences


/*!
Copies all of the selected text to the clipboard
under the condition that the user has set the
preference to automatically copy new selections.

Use this when the user changes the text selection.

(3.1)
*/
void
copySelectedTextIfUserPreference	(My_TerminalViewPtr		inTerminalViewPtr)
{
	if (selectionExists(inTerminalViewPtr))
	{
		Boolean		copySelectedText = false;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCopySelectedText, sizeof(copySelectedText),
									&copySelectedText))
		{
			copySelectedText = false; // assume text isn’t automatically copied, if preference can’t be found
		}
		
		if (copySelectedText) Clipboard_TextToScrap(inTerminalViewPtr->selfRef, kClipboard_CopyMethodStandard);
	}
}// copySelectedTextIfUserPreference


/*!
Attempts to read all supported text translation tags from
the given preference context.

(4.0)
*/
void
copyTranslationPreferences	(My_TerminalViewPtr			inTerminalViewPtr,
							 Preferences_ContextRef		inSource)
{
	CFStringEncoding	newInputEncoding = TextTranslation_ContextReturnEncoding
											(inSource, kCFStringEncodingUTF8/* default */);
	
	
	Terminal_SetTextEncoding(inTerminalViewPtr->screen.ref, newInputEncoding);
	
	//Console_WriteValue("terminal input text encoding changed to", newInputEncoding);
#if 0
	{
		CFStringRef		nameCFString = CFStringConvertEncodingToIANACharSetName(newInputEncoding);
		
		
		Console_WriteValueCFString("terminal input text encoding changed to (name)", nameCFString);
	}
#endif
}// copyTranslationPreferences


/*!
Creates a new color palette and initializes its colors using
terminal preferences.

In order to succeed, the specified view’s self-reference must
be valid.

Returns true only if the palette is created successfully.

(3.1)
*/
Boolean
createWindowColorPalette	(My_TerminalViewPtr			inTerminalViewPtr,
							 Preferences_ContextRef		inFormat,
							 Boolean					inSearchForDefaults)
{
	Boolean		result = false;
	
	
	// create palettes for future use
	inTerminalViewPtr->customColors.resize(kTerminalView_ColorIndexLastValid - kTerminalView_ColorIndexFirstValid + 1);
	inTerminalViewPtr->blinkColors.resize(kMy_BlinkingColorCount);
	inTerminalViewPtr->extendedColorsPtr = &gColorGrid(); // globally shared unless customized
	
	// set up window’s colors; note that this will set the non-ANSI colors,
	// the blinking colors, and the ANSI colors
	UNUSED_RETURN(UInt16)copyColorPreferences(inTerminalViewPtr, inFormat, inSearchForDefaults);
	
	// initialize timer to modify blinking text color (see setBlinkingTimerActive())
	inTerminalViewPtr->animation.timer.isActive = false;
	
	result = true;
	
	return result;
}// createWindowColorPalette


/*!
Returns "true" only if the specified screen’s cursor
should be blinking.  Currently, this preference is
global, so either all screen cursors blink or all
don’t; however, for future flexibility, this routine
takes a specific screen as a parameter.

(3.0)
*/
Boolean
cursorBlinks	(My_TerminalViewPtr		UNUSED_ARGUMENT(inTerminalViewPtr))
{
	return gPreferenceProxies.cursorBlinks;
}// cursorBlinks


/*!
Returns the user’s preferred cursor shape.  Currently,
this preference is global, so either all screen cursors
blink or all don’t; however, for future flexibility,
this routine takes a specific screen as a parameter.

(4.0)
*/
Terminal_CursorType
cursorType		(My_TerminalViewPtr		UNUSED_ARGUMENT(inTerminalViewPtr))
{
	return gPreferenceProxies.cursorType;
}// cursorType


/*!
Returns a custom version of the crosshairs for use in
terminal views.  The intent is to use a cursor that
has a larger size and better contrast.

(4.1)
*/
NSCursor*
customCursorCrosshairs	(My_TerminalViewPtr		inTerminalViewPtr)
{
	NSCursor*		result = nil;
	
	
	switch (mousePointerColor(inTerminalViewPtr))
	{
	case kTerminalView_MousePointerColorBlack:
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorBlackCrosshairs"]
																hotSpot:NSMakePoint(15, 15)];
			}
			result = gCachedCursor;
		}
		break;
	
	case kTerminalView_MousePointerColorWhite:
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorWhiteCrosshairs"]
																hotSpot:NSMakePoint(15, 15)];
			}
			result = gCachedCursor;
		}
		break;
	
	case kTerminalView_MousePointerColorRed:
	default:
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorCrosshairs"]
																hotSpot:NSMakePoint(15, 15)];
			}
			result = gCachedCursor;
		}
		break;
	}
	
	return result;
}// customCursorCrosshairs


/*!
Returns a custom version of the I-beam for use in
terminal views.  The intent is to use a cursor that
has a larger size and better contrast.

If the font size is very small, setting "inSmall"
to true is recommended so that a normal-size cursor
can be returned.

(4.1)
*/
NSCursor*
customCursorIBeam		(My_TerminalViewPtr		inTerminalViewPtr,
						 Boolean				inSmall)
{
	NSCursor*		result = nil;
	
	
	switch (mousePointerColor(inTerminalViewPtr))
	{
	case kTerminalView_MousePointerColorBlack:
		if (inSmall)
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorBlackIBeamSmall"]
																hotSpot:NSMakePoint(12, 11)];
			}
			result = gCachedCursor;
		}
		else
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorBlackIBeam"]
																hotSpot:NSMakePoint(16, 14)];
			}
			result = gCachedCursor;
		}
		break;
	
	case kTerminalView_MousePointerColorWhite:
		if (inSmall)
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorWhiteIBeamSmall"]
																hotSpot:NSMakePoint(12, 11)];
			}
			result = gCachedCursor;
		}
		else
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorWhiteIBeam"]
																hotSpot:NSMakePoint(16, 14)];
			}
			result = gCachedCursor;
		}
		break;
	
	case kTerminalView_MousePointerColorRed:
	default:
		if (inSmall)
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorIBeamSmall"]
																hotSpot:NSMakePoint(12, 11)];
			}
			result = gCachedCursor;
		}
		else
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorIBeam"]
																hotSpot:NSMakePoint(16, 14)];
			}
			result = gCachedCursor;
		}
		break;
	}
	
	return result;
}// customCursorIBeam


/*!
Returns a custom cursor to indicate that the terminal
cursor will move to the clicked location.

If the font size is very small, setting "inSmall" to
true is recommended so that a normal-size cursor can
be returned.

(4.1)
*/
NSCursor*
customCursorMoveTerminalCursor	(My_TerminalViewPtr		inTerminalViewPtr,
								 Boolean				inSmall)
{
	NSCursor*		result = nil;
	
	
	switch (mousePointerColor(inTerminalViewPtr))
	{
	case kTerminalView_MousePointerColorBlack:
		if (inSmall)
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorBlackMoveCursorSmall"]
																hotSpot:NSMakePoint(12, 12)];
			}
			result = gCachedCursor;
		}
		else
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorBlackMoveCursor"]
																hotSpot:NSMakePoint(16, 16)];
			}
			result = gCachedCursor;
		}
		break;
	
	case kTerminalView_MousePointerColorWhite:
		if (inSmall)
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorWhiteMoveCursorSmall"]
																hotSpot:NSMakePoint(12, 12)];
			}
			result = gCachedCursor;
		}
		else
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorWhiteMoveCursor"]
																hotSpot:NSMakePoint(16, 16)];
			}
			result = gCachedCursor;
		}
		break;
	
	case kTerminalView_MousePointerColorRed:
	default:
		if (inSmall)
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorMoveCursorSmall"]
																hotSpot:NSMakePoint(12, 12)];
			}
			result = gCachedCursor;
		}
		else
		{
			static NSCursor*	gCachedCursor = nil;
			
			
			if (nil == gCachedCursor)
			{
				// IMPORTANT: specified hot-spot should be synchronized with the image data
				gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorMoveCursor"]
																hotSpot:NSMakePoint(16, 16)];
			}
			result = gCachedCursor;
		}
		break;
	}
	
	return result;
}// customCursorMoveTerminalCursor


/*!
Delays the active thread by the specified amount
(in 60ths of a second).

(4.0)
*/
void
delayMinimumTicks	(UInt16		inTickCount)
{
	// UNIMPLEMENTED
}// delayMinimumTicks


/*!
Redraws the specified part of the given view.  Returns
"true" only if the text was drawn successfully.

This routine expects to be called while the current
graphics port origin is equal to the screen origin.

WARNING:	For efficiency, this does no range checking.

The given range refers to actual visible screen cells,
NOT their corresponding locations in the terminal
buffer.  So for example, row 0 means you want the first
visible line redrawn; the text that is actually drawn
will depend on where the user has scrolled the content.
It follows that you cannot specify values greater than
or equal to the number of visible rows or columns in
the view, whichever applies to the parameter.

IMPORTANT:	The QuickDraw port state is not saved or
			restored, but could change.

(3.0)
*/
Boolean
drawSection		(My_TerminalViewPtr		inTerminalViewPtr,
				 CGContextRef			inDrawingContext,
				 UInt16					UNUSED_ARGUMENT(inZeroBasedLeftmostColumnToDraw),
				 TerminalView_RowIndex	inZeroBasedTopmostRowToDraw,
				 UInt16					UNUSED_ARGUMENT(inZeroBasedPastTheRightmostColumnToDraw),
				 TerminalView_RowIndex	inZeroBasedPastTheBottommostRowToDraw)
{
	Boolean		result = false;
	
	
	// debug
	//Console_WriteValuePair("draw lines in past-the-end range", inZeroBasedTopmostRowToDraw, inZeroBasedPastTheBottommostRowToDraw);
	
	if (nullptr != inTerminalViewPtr)
	{
		Terminal_Result		iteratorResult = kTerminal_ResultOK;
		
		
		// find contiguous blocks of text on each line in the given
		// range that have the same attributes, and draw those blocks
		// all at once (which is more efficient than doing them
		// individually); a field inside the structure is used to
		// track the current line, so that drawTerminalScreenRunOp()
		// can determine the correct drawing rectangle for the line
		{
			Terminal_LineStackStorage	lineIteratorData;
			Terminal_LineRef			lineIterator = findRowIterator
														(inTerminalViewPtr, inZeroBasedTopmostRowToDraw,
															&lineIteratorData);
			
			
			if (nullptr != lineIterator)
			{
				TextAttributes_Object	lineGlobalAttributes;
				
				
				// unfortunately rendering requires knowledge of the physical location of
				// a row in the view area, and this is something that a terminal screen
				// buffer does not know; the work-around is to set a numeric field in the
				// view structure, and change it as lines are iterated over (the rendering
				// routine consults this field to figure out where to draw terminal text)
				inTerminalViewPtr->screen.currentRenderContext = inDrawingContext;
				for (inTerminalViewPtr->screen.currentRenderedLine = inZeroBasedTopmostRowToDraw;
						inTerminalViewPtr->screen.currentRenderedLine < inZeroBasedPastTheBottommostRowToDraw;
						++(inTerminalViewPtr->screen.currentRenderedLine))
				{
					// TEMPORARY: extremely inefficient, but necessary for
					// correct scrollback behavior at the moment
					lineIterator = findRowIterator(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderedLine,
													&lineIteratorData);
					if (nullptr == lineIterator)
					{
						break;
					}
					
					iteratorResult = Terminal_ForEachLikeAttributeRun
										(inTerminalViewPtr->screen.ref, lineIterator,
											^(UInt16					inLineTextBufferLength,
											  CFStringRef				inLineTextBufferAsCFStringOrNull,
											  Terminal_LineRef			UNUSED_ARGUMENT(inRow),
											  UInt16					inZeroBasedStartColumnNumber,
											  TextAttributes_Object		inAttributes)
											{
												drawTerminalScreenRunOp(inTerminalViewPtr, inLineTextBufferLength, inLineTextBufferAsCFStringOrNull,
																		inZeroBasedStartColumnNumber, inAttributes);
											});
					if (iteratorResult != kTerminal_ResultOK)
					{
						// did not draw successfully...?
					}
					else
					{
						result = true;
					}
					
					releaseRowIterator(inTerminalViewPtr, &lineIterator);
				}
			}
		}
		
		// reset these, they shouldn’t have significance outside the above loop
		inTerminalViewPtr->screen.currentRenderedLine = -1;
		inTerminalViewPtr->screen.currentRenderContext = nullptr;
	}
	return result;
}// drawSection


/*!
Instead of drawing an image as-is, draws the image as
if it were entirely in one color (except for effects
from anti-aliasing).  For example, this can be used to
draw an image using a text color as if it were a member
of a font.

The layer image object must be something that is valid
for a CALayer (currently: CGImageRef or NSImage*).

Note, this is generic and it should move elsewhere.

(4.1)
*/
void
drawSingleColorImage	(CGContextRef	inDrawingContext,
						 CGColorRef		inColor,
						 CGRect			inFrame,
						 id				inLayerImage)
{
	CALayer*	maskLayer = [CALayer layer];
	CALayer*	renderingLayer = [CALayer layer];
	
	
	assert(nil != maskLayer);
	assert(nil != renderingLayer);
	maskLayer.contents = inLayerImage;
	maskLayer.frame = CGRectMake(0, 0, inFrame.size.width, inFrame.size.height);
	renderingLayer.backgroundColor = inColor;
	renderingLayer.frame = inFrame;
	renderingLayer.mask = maskLayer;
	
	// place the drawing in the right part of the window
	CGContextTranslateCTM(inDrawingContext, renderingLayer.frame.origin.x,
							renderingLayer.frame.origin.y);
	
	[renderingLayer renderInContext:inDrawingContext];
	
	// revert the above adjustment to the transformation
	// matrix to avoid permanently changing graphics state
	CGContextTranslateCTM(inDrawingContext, -renderingLayer.frame.origin.x,
							-renderingLayer.frame.origin.y);
}// drawSingleColorImage


/*!
Invokes drawSingleColorImage() one or more times, to
ensure that all renderings of the image will appear to
align with other calls for the same image.  (All the
warnings and side effects of that method apply.)

NOTE:	Patterns are generally created with images for
		performance reasons.  Attempts to use other
		approaches (like direct Core Graphics APIs)
		have had relatively slow rendering times.

(4.1)
*/
void
drawSingleColorPattern	(CGContextRef	inDrawingContext,
						 CGColorRef		inColor,
						 CGRect			inFrame,
						 id				inLayerImage)
{
	CGFloat const	kOriginalX = inFrame.origin.x;
	CGFloat const	kOriginalY = inFrame.origin.y;
	CGFloat const	kWidth = CGRectGetWidth(inFrame);
	CGFloat const	kHeight = CGRectGetHeight(inFrame);
	SInt32 const	kOffsetToAlignX = -(STATIC_CAST(kOriginalX, SInt32) % STATIC_CAST(kWidth, SInt32));
	SInt32 const	kOffsetToAlignY = -(STATIC_CAST(kOriginalY, SInt32) % STATIC_CAST(kHeight, SInt32));
	
	
	// disable antialiasing to make pattern edges join together better
	// (the image itself will still blur in order to scale)
	CGContextSetAllowsAntialiasing(inDrawingContext, false);
	
	if ((0 == kOffsetToAlignX) && (0 == kOffsetToAlignY))
	{
		// proposed location happens to be aligned; nothing special
		// is required so take the faster path
		drawSingleColorImage(inDrawingContext, inColor, inFrame, inLayerImage);
	}
	else
	{
		CGFloat const	kX1 = (kOriginalX + kOffsetToAlignX);
		CGFloat const	kY1 = (kOriginalY + kOffsetToAlignY);
		CGFloat const	kX2 = (kX1 + kWidth);
		CGFloat const	kY2 = (kY1 + kHeight);
		CGRect const	kAlignedX1Y1Rect = CGRectMake(kX1, kY1, kWidth, kHeight);
		CGRect const	kAlignedX1Y2Rect = CGRectMake(kX1, kY2, kWidth, kHeight);
		CGRect const	kAlignedX2Y1Rect = CGRectMake(kX2, kY1, kWidth, kHeight);
		CGRect const	kAlignedX2Y2Rect = CGRectMake(kX2, kY2, kWidth, kHeight);
		
		
		// when alignment is required there will always be a base image
		// rendered at the closest aligned location
		drawSingleColorImage(inDrawingContext, inColor, kAlignedX1Y1Rect, inLayerImage);
		
		// draw up to 3 more images (which will be clipped out) to show
		// up to 4 sections of a fully-aligned pattern
		if (0 == kOffsetToAlignX)
		{
			// only Y direction requires an extra image to fill
			drawSingleColorImage(inDrawingContext, inColor, kAlignedX1Y2Rect, inLayerImage);
		}
		else if (0 == kOffsetToAlignY)
		{
			// only X direction requires an extra image to fill
			drawSingleColorImage(inDrawingContext, inColor, kAlignedX2Y1Rect, inLayerImage);
		}
		else
		{
			// alignment is shifted in both directions; multiple
			// images are required to complete the pattern
			drawSingleColorImage(inDrawingContext, inColor, kAlignedX1Y2Rect, inLayerImage);
			drawSingleColorImage(inDrawingContext, inColor, kAlignedX2Y1Rect, inLayerImage);
			drawSingleColorImage(inDrawingContext, inColor, kAlignedX2Y2Rect, inLayerImage);
		}
	}
	
	CGContextSetAllowsAntialiasing(inDrawingContext, true);
}// drawSingleColorPattern


/*!
Draws the specified chunk of text in the given view
(line number 1 is the oldest line in the scrollback).

This routine expects to be called while the current
graphics port origin is equal to the screen origin.

For efficiency, this routine does not preserve or
restore state; do that on your own before invoking
Terminal_ForEachLikeAttributeRun().

(2017.12)
*/
void
drawTerminalScreenRunOp		(My_TerminalViewPtr			inTerminalViewPtr,
							 UInt16						inLineTextBufferLength,
							 CFStringRef				inLineTextBufferAsCFStringOrNull,
							 UInt16						inZeroBasedStartColumnNumber,
							 TextAttributes_Object		inAttributes)
{
	CGRect		sectionBounds;
	
	
	// set up context foreground and background colors appropriately
	// for the specified terminal attributes; this takes into account
	// things like bold and highlighted text, etc.
	useTerminalTextColors(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderContext, inAttributes, false/* is cursor */, 1.0/* alpha */);
	
	// erase and redraw the current rendering line, but only the
	// specified range (starting column and character count)
	eraseSection(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderContext,
					inZeroBasedStartColumnNumber, inZeroBasedStartColumnNumber + inLineTextBufferLength,
					sectionBounds);
	
	// draw the text or graphics
	if (inAttributes.hasAttributes(kTextAttributes_ColorIndexIsBitmapID))
	{
		// bitmap has been defined
		Terminal_Result				terminalResult = kTerminal_ResultOK;
		TextAttributes_BitmapID		bitmapID = inAttributes.bitmapID();
		CGRect						imageSubRect = CGRectZero;
		NSImage* __autoreleasing	completeImage = nil;
		
		
		terminalResult = Terminal_BitmapGetFromID(inTerminalViewPtr->screen.ref, bitmapID, imageSubRect, completeImage);
		if ((kTerminal_ResultOK != terminalResult) || (nil == completeImage))
		{
			// do not log image errors because there could potentially be
			// many of them (such as, one per cell of the image); instead,
			// add a special rendering to the terminal cell
			CFStringRef		errorString = CFSTR("!");
			
			
			drawTerminalText(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderContext, sectionBounds,
								CFStringGetLength(errorString), errorString, kTextAttributes_StyleInverse);
		}
		else
		{
			NSRect				targetNSRect = NSMakeRect(sectionBounds.origin.x, sectionBounds.origin.y,
															sectionBounds.size.width, sectionBounds.size.height);
			NSRect				sourceNSRect = NSMakeRect(imageSubRect.origin.x, imageSubRect.origin.y,
															imageSubRect.size.width, imageSubRect.size.height);
			NSDictionary*		hintDict = @{}; // from NSString* to id
			NSGraphicsContext*	graphicsContext = [NSGraphicsContext
													graphicsContextWithCGContext:inTerminalViewPtr->screen.currentRenderContext
																					flipped:YES];
			auto				oldInterpolation = [graphicsContext imageInterpolation];
			
			
			[NSGraphicsContext saveGraphicsState];
			[NSGraphicsContext setCurrentContext:graphicsContext];
			[graphicsContext setImageInterpolation:NSImageInterpolationNone]; // TEMPORARY; may want this to be an option
			CGContextSetAllowsAntialiasing(inTerminalViewPtr->screen.currentRenderContext, false);
			[completeImage drawInRect:targetNSRect fromRect:sourceNSRect
										operation:(inAttributes.hasSelection()
													? NSCompositingOperationPlusDarker
													: NSCompositingOperationSourceOver)
										fraction:1.0
										respectFlipped:YES hints:hintDict];
			CGContextSetAllowsAntialiasing(inTerminalViewPtr->screen.currentRenderContext, true);
			[graphicsContext setImageInterpolation:oldInterpolation];
			[NSGraphicsContext restoreGraphicsState];
		}
	}
	else if ((nullptr != inLineTextBufferAsCFStringOrNull) && (0 != inLineTextBufferLength))
	{
		drawTerminalText(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderContext, sectionBounds,
							inLineTextBufferLength, inLineTextBufferAsCFStringOrNull, inAttributes);
		
		// since blinking forces frequent redraws, do not do it more
		// than necessary; keep track of any blink attributes, and
		// elsewhere this flag can be used to disable the animation
		// timer when it is not needed
		if (inAttributes.hasBlink())
		{
			UNUSED_RETURN(OSStatus)HIShapeUnionWithRect(inTerminalViewPtr->animation.rendering.region, &sectionBounds);
			
			inTerminalViewPtr->screen.currentRenderBlinking = true;
		}
	}
}// drawTerminalScreenRunOp


/*!
Draws the specified text using the given attributes, at a pen
location appropriate for fitting within the given boundaries.
If the cursor falls anywhere in the given area and is in a
visible state, it is drawn automatically.

WARNING:	For Cocoa-based drawing this routine CANNOT be called
		outside of the content view’s normal layer drawing code.

Although this function will set up text attributes such as
font, color, graphics line width, etc., it will NOT set colors.
The context must already be set up to use the desired stroke
and fill colors.

For optimal performance the graphics context state may not be
saved or restored; it could be returned in any state.

(3.0)
*/
void
drawTerminalText	(My_TerminalViewPtr			inTerminalViewPtr,
					 CGContextRef				inDrawingContext,
					 CGRect const&				inBoundaries,
					 CFIndex					inCharacterCount,
					 CFStringRef				inTextBufferAsCFString,
					 TextAttributes_Object		inAttributes)
{
	// store new text attributes, for anything that refers to them
	inTerminalViewPtr->text.attributes = inAttributes;
	
	// font attributes are set directly on the (attributed) string
	// of the text storage, not in the graphics context
	setTextAttributesDictionary(inTerminalViewPtr, inTerminalViewPtr->text.attributeDict, inAttributes);
	
	if (inAttributes.hasDoubleHeightTop())
	{
		// ignore; top half is not drawn (bottom is drawn twice as large, below)
	}
	else if (inAttributes.hasAttributes(kTextAttributes_VTGraphics))
	{
		CGFloat				baselineHint = 0;
		CGFloat				cellCount = inCharacterCount; // TEMPORARY; this is not really true and must be deduced from the text
		__block CGRect		glyphBounds = CGRectMake(inBoundaries.origin.x, inBoundaries.origin.y,
														inBoundaries.size.width / cellCount,
														inBoundaries.size.height);
		
		
		StringUtilities_ForEachComposedCharacterSequenceInRange
		(inTextBufferAsCFString, CFRangeMake(0, inCharacterCount),
		^(CFStringRef	aSubstring,
		  CFRange		UNUSED_ARGUMENT(aRange),
		  Boolean&		UNUSED_ARGUMENT(outStopFlag))
		{
			UnicodeScalarValue		glyphType = StringUtilities_ReturnUnicodeSymbol(aSubstring);
			
			
			// NOTE: double-sized text is handled implicitly here because the
			// glyph renderers use the given rectangle (which has been sized
			// ahead of time to be correct if the line is double-width and/or
			// double-height)
			drawVTGraphicsGlyph(inTerminalViewPtr, inDrawingContext, glyphBounds, glyphType, baselineHint, inAttributes);
			glyphBounds.origin.x += glyphBounds.size.width;
		});
	}
	else
	{
		// draw the text with the correct attributes: font, etc.
		CGFloat const			viewHeight = inTerminalViewPtr->screen.cache.viewHeightInPixels.precisePixels();
		CGFloat const			cellHeight = inTerminalViewPtr->text.font.heightPerCell.precisePixels();
		NSAttributedString*		attributedString = [[NSAttributedString alloc]
													initWithString:BRIDGE_CAST(inTextBufferAsCFString, NSString*)
																	attributes:inTerminalViewPtr->text.attributeDict];
		CFRetainRelease			lineObject(CTLineCreateWithAttributedString
											(BRIDGE_CAST(attributedString, CFAttributedStringRef)),
											CFRetainRelease::kAlreadyRetained);
		CTLineRef				asLineRef = REINTERPRET_CAST(lineObject.returnCFTypeRef(), CTLineRef);
		// text is laid out using Core Text metrics previously cached by setUpScreenFontMetrics()
		// (for efficiency and also consistency, as cells are the same size regardless of attributes,
		// except for double-size text which is scaled below)
		NSPoint					drawingLocation = NSMakePoint(inBoundaries.origin.x,
																viewHeight - inBoundaries.origin.y - cellHeight + inTerminalViewPtr->text.font.normalMetrics.baseLine);
		CGContextSaveRestore	_(inDrawingContext);
		
		
		CGContextTranslateCTM(inDrawingContext, 0, viewHeight);
		if (inAttributes.hasDoubleWidth())
		{
			CGContextScaleCTM(inDrawingContext, 2.0, -1.0);
			drawingLocation.x *= 0.5; // convert point to 2x scale
		}
		else if (inAttributes.hasDoubleHeightBottom())
		{
			// note: hasDoubleHeightTop() is handled earlier so it
			// is not considered in this if-else clause
			CGContextScaleCTM(inDrawingContext, 2.0, -2.0);
			drawingLocation.x *= 0.5; // convert point to 2x scale
			drawingLocation.y -= (cellHeight - inTerminalViewPtr->text.font.normalMetrics.baseLine); // default drawing location assumes single height; offset by additional single height
			drawingLocation.y *= 0.5; // convert point to 2x scale
		}
		else
		{
			CGContextScaleCTM(inDrawingContext, 1.0, -1.0);
		}
		
		CGContextSetTextPosition(inDrawingContext, drawingLocation.x, drawingLocation.y);
		CTLineDraw(asLineRef, inDrawingContext);
		
		if (inAttributes.hasBold() &&
			(inTerminalViewPtr->text.font.boldFont == inTerminalViewPtr->text.font.normalFont))
		{
			// COMPLETE AND UTTER HACK: occasionally a font will have no bold version
			// in the same family and Cocoa does not seem as capable as QuickDraw in
			// terms of inventing a bold rendering for such fonts; as a work-around
			// text is drawn TWICE (the second at a slight offset from the original)
			if (inAttributes.hasDoubleAny())
			{
				drawingLocation.x += (1 + (inTerminalViewPtr->text.font.widthPerCell.precisePixels() / 60)); // arbitrary
			}
			else
			{
				drawingLocation.x += (1 + (inTerminalViewPtr->text.font.widthPerCell.precisePixels() / 30)); // arbitrary
			}
			
			CGContextSetTextPosition(inDrawingContext, drawingLocation.x, drawingLocation.y);
			CTLineDraw(asLineRef, inDrawingContext);
		}
	}
}// drawTerminalText


/*!
Renders a special graphics character within the specified
boundaries, assuming the pen is at the baseline of where a
font character would be inserted.  Glyphs that are drawn by
fonts will be inserted at the pen’s location.  Glyphs that
are specially rendered can use the bounding rectangle
directly and may ignore the pen location.

All other text-related aspects of the specified port are
used to affect graphics (such as the presence of bold face).

The setTextAttributesDictionary() function should be called
before calling this.

Due to the difficulty of creating vector-based fonts that
line graphics up properly, and the lousy look of scaled-up
bitmapped fonts, MacTerm renders most VT font characters on
its own, using line drawing.  In addition, MacTerm uses
international symbols for glyphs such as CR, HT, etc.
instead of the Roman-based ones prescribed by the standard
VT font.

(3.0)
*/
void
drawVTGraphicsGlyph		(My_TerminalViewPtr			inTerminalViewPtr,
						 CGContextRef				inDrawingContext,
						 CGRect const&				inBoundaries,
						 UnicodeScalarValue			inUnicode,
						 CGFloat					inBaselineHint,
						 TextAttributes_Object		inAttributes)
{
	CGFloat const					kMaxWidthInPixelsForSmallSizeGlyphs = 10; // arbitrary; when to switch to small-size renderings
	TerminalGlyphDrawing_Options	drawingOptions = 0;
	// float parameters are used for improved renderings with Core Graphics
	CGContextSaveRestore			_(inDrawingContext);
	TerminalGlyphDrawing_Cache*		sourceLayerCache = nil; // may be set below
	CGRect							floatBounds = inBoundaries;
	CGColorRef						foregroundColor = nullptr;
	NSColor*						foregroundNSColor = nil;
	
	
	if (inAttributes.hasBold())
	{
		drawingOptions |= kTerminalGlyphDrawing_OptionBold;
	}
	
	if (floatBounds.size.width < kMaxWidthInPixelsForSmallSizeGlyphs)
	{
		drawingOptions |= kTerminalGlyphDrawing_OptionSmallSize;
	}
	
	foregroundNSColor = STATIC_CAST([inTerminalViewPtr->text.attributeDict objectForKey:NSForegroundColorAttributeName],
									NSColor*);
	{
		NSColor*	asRGB = [foregroundNSColor colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
		
		
		assert(nil != asRGB);
		foregroundColor = CGColorCreateGenericRGB(asRGB.redComponent, asRGB.greenComponent, asRGB.blueComponent,
													asRGB.alphaComponent);
	}
	
#if 0
	{
		// debug: show the clipping rectangle
		CGContextSaveRestore	_(inDrawingContext);
		
		
		[[NSColor redColor] setAsForegroundInCGContext:inDrawingContext];
		CGContextStrokeRect(inDrawingContext, floatBounds);
	}
#endif
#if 1 // disable to debug out-of-bounds errors
	// clip drawing to the boundaries (this is restored upon return);
	// note that this cannot offset by as much as one full pixel
	// because this would start to create gaps between graphics glyphs
	CGContextClipToRect(inDrawingContext, CGRectMake(floatBounds.origin.x + 0.5f, floatBounds.origin.y + 0.5f,
														floatBounds.size.width - 1, floatBounds.size.height - 1));
	//CGContextClipToRect(inDrawingContext, floatBounds);
#endif
	
	// The set of characters supported here should also be used in the
	// translateCharacter() internal method of the Terminal module.
	// That way, this renderer will always be called for these characters,
	// whether or not they were originally tagged as being graphical.
	switch (inUnicode)
	{
	case 0x2501: // middle line, bold version
	case 0x2503: // vertical line, bold version
	case 0x250F: // hook mid-right to mid-bottom, bold version
	case 0x2513: // hook mid-left to mid-bottom, bold version
	case 0x2517: // hook mid-top to mid-right, bold version
	case 0x251B: // hook mid-top to mid-left, bold version
	case 0x2523: // cross minus the left piece, bold version
	case 0x252B: // cross minus the right piece, bold version
	case 0x2533: // cross minus the top piece, bold version
	case 0x253B: // cross minus the bottom piece, bold version
	case 0x254B: // cross, bold version
	case 0x2578: // cross, left segment only, bold version
	case 0x2579: // cross, top segment only, bold version
	case 0x257A: // cross, right segment only, bold version
	case 0x257B: // cross, bottom segment only, bold version
		{
			// glyph can be generated by Terminal Glyphs module
			// but it is also the bold version (note: most likely
			// the option was already set by attributes but this
			// is for completeness)
			drawingOptions |= kTerminalGlyphDrawing_OptionBold;
		}
		//break; // INTENTIONAL FALL-THROUGH
	// INTENTIONAL FALL-THROUGH
	case 0x221A: // square root left edge
	case 0x23B7: // square root bottom, centered
	case 0x23B8: // left vertical box line
	case 0x23B9: // right vertical box line
	case 0x23BA: // top line
	case 0x23BB: // line between top and middle regions
	case 0x23BC: // line between middle and bottom regions
	case 0x23BD: // bottom line
	case 0x23D0: // vertical line extension
	case 0x2500: // middle line
	case 0x2502: // vertical line
	case 0x250C: // hook mid-right to mid-bottom
	case 0x250D: // hook mid-right to mid-bottom, bold right
	case 0x250E: // hook mid-right to mid-bottom, bold bottom
	case 0x2510: // hook mid-left to mid-bottom
	case 0x2511: // hook mid-left to mid-bottom, bold left
	case 0x2512: // hook mid-left to mid-bottom, bold bottom
	case 0x2514: // hook mid-top to mid-right
	case 0x2515: // hook mid-top to mid-right, bold right
	case 0x2516: // hook mid-top to mid-right, bold top
	case 0x2518: // hook mid-top to mid-left
	case 0x2519: // hook mid-top to mid-left, bold left
	case 0x251A: // hook mid-top to mid-left, bold top
	case 0x251C: // cross minus the left piece
	case 0x251D: // cross minus the left piece, bold right
	case 0x251E: // cross minus the left piece, bold top
	case 0x251F: // cross minus the left piece, bold bottom
	case 0x2520: // cross minus the left piece, bold vertical
	case 0x2521: // cross minus the left piece, bold hook mid-top to mid-right
	case 0x2522: // cross minus the left piece, bold hook mid-bottom to mid-right
	//case 0x2523: // cross minus the left piece, bold version
	case 0x2524: // cross minus the right piece
	case 0x2525: // cross minus the right piece, bold left
	case 0x2526: // cross minus the right piece, bold top
	case 0x2527: // cross minus the right piece, bold bottom
	case 0x2528: // cross minus the right piece, bold vertical
	case 0x2529: // cross minus the right piece, bold hook mid-top to mid-left
	case 0x252A: // cross minus the right piece, bold hook mid-bottom to mid-left
	//case 0x252B: // cross minus the right piece, bold version
	case 0x252C: // cross minus the top piece
	case 0x252D: // cross minus the top piece, bold left
	case 0x252E: // cross minus the top piece, bold right
	case 0x252F: // cross minus the top piece, bold horizontal
	case 0x2530: // cross minus the top piece, bold bottom
	case 0x2531: // cross minus the top piece, bold hook mid-bottom to mid-left
	case 0x2532: // cross minus the top piece, bold hook mid-bottom to mid-right
	//case 0x2533: // cross minus the top piece, bold version
	case 0x2534: // cross minus the bottom piece
	case 0x2535: // cross minus the bottom piece, bold left
	case 0x2536: // cross minus the bottom piece, bold right
	case 0x2537: // cross minus the bottom piece, bold horizontal
	case 0x2538: // cross minus the bottom piece, bold top
	case 0x2539: // cross minus the bottom piece, bold hook mid-top to mid-left
	case 0x253A: // cross minus the bottom piece, bold hook mid-top to mid-right
	//case 0x253B: // cross minus the bottom piece, bold version
	case 0x253C: // cross
	case 0x253D: // cross, bold left
	case 0x253E: // cross, bold right
	case 0x253F: // cross, bold horizontal
	case 0x2540: // cross, bold top
	case 0x2541: // cross, bold bottom
	case 0x2542: // cross, bold vertical
	case 0x2543: // cross, bold hook mid-top to mid-left
	case 0x2544: // cross, bold hook mid-top to mid-right
	case 0x2545: // cross, bold hook mid-bottom to mid-left
	case 0x2546: // cross, bold hook mid-bottom to mid-right
	case 0x2547: // cross, bold T-up
	case 0x2548: // cross, bold T-down
	case 0x2549: // cross, bold T-left
	case 0x254A: // cross, bold T-right
	//case 0x254B: // cross, bold version
	case 0x2550: // middle line, double-line version
	case 0x2551: // vertical line, double-line version
	case 0x2552: // hook mid-right to mid-bottom, double-horizontal-only version
	case 0x2553: // hook mid-right to mid-bottom, double-vertical-only version
	case 0x2554: // hook mid-right to mid-bottom, double-line version
	case 0x2555: // hook mid-left to mid-bottom, double-horizontal-only version
	case 0x2556: // hook mid-left to mid-bottom, double-vertical-only version
	case 0x2557: // hook mid-left to mid-bottom, double-line version
	case 0x2558: // hook mid-top to mid-right, double-horizontal-only version
	case 0x2559: // hook mid-top to mid-right, double-vertical-only version
	case 0x255A: // hook mid-top to mid-right, double-line version
	case 0x255B: // hook mid-top to mid-left, double-horizontal-only version
	case 0x255C: // hook mid-top to mid-left, double-vertical-only version
	case 0x255D: // hook mid-top to mid-left, double-line version
	case 0x255E: // cross minus the left piece, double-horizontal-only version
	case 0x255F: // cross minus the left piece, double-vertical-only version
	case 0x2560: // cross minus the left piece, double-line version
	case 0x2561: // cross minus the right piece, double-horizontal-only version
	case 0x2562: // cross minus the right piece, double-vertical-only version
	case 0x2563: // cross minus the right piece, double-line version
	case 0x2564: // cross minus the top piece, double-horizontal-only version
	case 0x2565: // cross minus the top piece, double-vertical-only version
	case 0x2566: // cross minus the top piece, double-line version
	case 0x2567: // cross minus the bottom piece, double-horizontal-only version
	case 0x2568: // cross minus the bottom piece, double-vertical-only version
	case 0x2569: // cross minus the bottom piece, double-line version
	case 0x256A: // cross, double-horizontal-only
	case 0x256B: // cross, double-vertical-only
	case 0x256C: // cross, double-line version
	case 0x2574: // cross, left segment only
	case 0x2575: // cross, top segment only
	case 0x2576: // cross, right segment only
	case 0x2577: // cross, bottom segment only
	case 0x257C: // horizontal line, bold right half
	case 0x257D: // vertical line, bold bottom half
	case 0x257E: // horizontal line, bold left half
	case 0x257F: // vertical line, bold top half
	case 0x2580: // upper-half block
	case 0x2581: // 1/8 bottom block
	case 0x2582: // 2/8 (1/4) bottom block
	case 0x2583: // 3/8 bottom block
	case 0x2584: // lower-half block
	case 0x2585: // 5/8 bottom block
	case 0x2586: // 6/8 (3/4) bottom block
	case 0x2587: // 7/8 bottom block
	case 0x2588: // solid block
	case 0x2589: // 7/8 left block
	case 0x258A: // 6/8 (3/4) left block
	case 0x258B: // 5/8 left block
	case 0x258C: // left-half block
	case 0x258D: // 3/8 left block
	case 0x258E: // 2/8 (1/4) left block
	case 0x258F: // 1/8 left block
	case 0x2590: // right-half block
	case 0x2594: // 1/8 top block
	case 0x2595: // 1/8 right block
	case 0x2596: // quadrant lower-left
	case 0x2597: // quadrant lower-right
	case 0x2598: // quadrant upper-left
	case 0x2599: // block minus upper-right quadrant
	case 0x259A: // quadrants upper-left and lower-right
	case 0x259B: // block minus lower-right quadrant
	case 0x259C: // block minus lower-left quadrant
	case 0x259D: // quadrant upper-right
	case 0x259E: // quadrants upper-right and lower-left
	case 0x259F: // block minus upper-left quadrant
		{
			// glyph can be generated by Terminal Glyphs module
			sourceLayerCache = [TerminalGlyphDrawing_Cache cacheWithUnicodePoint:inUnicode];
			
			// most glyphs are negatively affected by anti-aliasing
			// because adjacent characters are not cleanly joined;
			// note the exceptions in the case below
			drawingOptions |= kTerminalGlyphDrawing_OptionAntialiasingDisabled;
		}
		break;
	
	case 0x2505: // horizontal triple-dashed line, bold version
	case 0x2507: // vertical triple-dashed line, bold version
	case 0x2509: // horizontal quadruple-dashed line, bold version
	case 0x250B: // vertical quadruple-dashed line, bold version
	case 0x254D: // horizontal double-dashed line, bold version
	case 0x254F: // vertical double-dashed line, bold version
	case 0x2714: // check mark, bold
	case 0x2718: // X mark
		{
			// antialiased and bold
			drawingOptions |= kTerminalGlyphDrawing_OptionBold;
		}
		//break; // INTENTIONAL FALL-THROUGH
	// INTENTIONAL FALL-THROUGH
	case '=': // equal to
	case 0x00B7: // middle dot
	case 0x2022: // bullet (technically bigger than middle dot and circular)
	case 0x2026: // ellipsis (three dots)
	case 0x2027: // centered dot (hyphenation point)
	case 0x2190: // leftwards arrow
	case 0x2191: // upwards arrow
	case 0x2192: // rightwards arrow
	case 0x2193: // downwards arrow
	case 0x21B5: // new line (international symbol is an arrow that hooks from mid-top to mid-left)
	case 0x21DF: // form feed (international symbol is an arrow pointing top to bottom with two horizontal lines through it)
	case 0x21E5: // horizontal tab (international symbol is a right-pointing arrow with a terminating line)
	case 0x2260: // not equal to
	case 0x2261: // equivalent to (three horizontal lines)
	case 0x2219: // bullet operator (smaller than bullet)
	case 0x22EF: // middle ellipsis (three dots, centered)
	case 0x2320: // integral sign (elongated S), top
	case 0x2321: // integral sign (elongated S), bottom
	case 0x2325: // option key
	case 0x2387: // alternate key
	case 0x239B: // left parenthesis, upper
	case 0x239C: // left parenthesis extension
	case 0x239D: // left parenthesis, lower
	case 0x239E: // right parenthesis, upper
	case 0x239F: // right parenthesis extension
	case 0x23A0: // right parenthesis, lower
	case 0x23A1: // left square bracket, upper
	case 0x23A2: // left square bracket extension
	case 0x23A3: // left square bracket, lower
	case 0x23A4: // right square bracket, upper
	case 0x23A5: // right square bracket extension
	case 0x23A6: // right square bracket, lower
	case 0x23A7: // left curly brace, upper
	case 0x23A8: // left curly brace, middle
	case 0x23A9: // left curly brace, lower
	case 0x23AA: // curly brace extension
	case 0x23AB: // right curly brace, upper
	case 0x23AC: // right curly brace, middle
	case 0x23AD: // right curly brace, lower
	case 0x23AE: // integral extension
	case 0x23B2: // large sigma (summation), top half
	case 0x23B3: // large sigma (summation), bottom half
	case 0x2504: // horizontal triple-dashed line
	case 0x2506: // vertical triple-dashed line
	case 0x2508: // horizontal quadruple-dashed line
	case 0x250A: // vertical quadruple-dashed line
	case 0x254C: // horizontal double-dashed line
	case 0x254E: // vertical double-dashed line
	case 0x256D: // curved mid-right to mid-bottom
	case 0x256E: // curved mid-left to mid-bottom
	case 0x256F: // curved mid-top to mid-left
	case 0x2570: // curved mid-top to mid-right
	case 0x2571: // diagonal line from top-right to bottom-left
	case 0x2572: // diagonal line from top-left to bottom-right
	case 0x2573: // diagonal lines from each corner crossing in the center
	case 0x25A0: // black square
	case 0x25C6: // black diamond
	case 0x25C7: // white diamond
	case 0x25CA: // lozenge (narrower white diamond)
	case 0x26A1: // online/offline lightning bolt
	case 0x2713: // check mark
	case 0x27A6: // curve-to-right arrow (used for "detached from head" in powerline)
	case 0x2913: // vertical tab (international symbol is a down-pointing arrow with a terminating line)
	case 0xFFFD: // replacement character
		{
			// glyphs in this case are the same as the case above
			// except that anti-aliasing is ALLOWED
			sourceLayerCache = [TerminalGlyphDrawing_Cache cacheWithUnicodePoint:inUnicode];
		}
		break;
	
	case 0xE0A0: // "powerline" version control branch
	case 0xE0A1: // "powerline" line (LN) marker
	case 0xE0A2: // "powerline" closed padlock
	case 0xE0B0: // "powerline" rightward triangle
	case 0xE0B1: // "powerline" rightward arrowhead
	case 0xE0B2: // "powerline" leftward triangle
	case 0xE0B3: // "powerline" leftward arrowhead
		{
			// handling of these is currently the same as above but
			// they are separated so that it will be easy to disable
			// them conditionally later (technically they implement
			// Unicode extensions and some application may not want
			// this interpretation)
			sourceLayerCache = [TerminalGlyphDrawing_Cache cacheWithUnicodePoint:inUnicode];
		}
		break;
	
	case 0x2591: // light gray pattern
		{
			NSString*	imageName = BRIDGE_CAST(AppResources_ReturnGlyphPatternLightGrayFilenameNoExtension(),
												NSString*);
			NSImage*	patternImage = [NSImage imageNamed:imageName];
			
			
			if (nil == patternImage)
			{
				Console_Warning(Console_WriteValueCFString, "unable to find image, name", BRIDGE_CAST(imageName, CFStringRef));
			}
			else
			{
				drawSingleColorPattern(inDrawingContext, foregroundColor, floatBounds, patternImage);
			}
		}
		break;
	
	case 0x2592: // medium gray pattern
		{
			NSString*	imageName = BRIDGE_CAST(AppResources_ReturnGlyphPatternMediumGrayFilenameNoExtension(),
												NSString*);
			NSImage*	patternImage = [NSImage imageNamed:imageName];
			
			
			if (nil == patternImage)
			{
				Console_Warning(Console_WriteValueCFString, "unable to find image, name", BRIDGE_CAST(imageName, CFStringRef));
			}
			else
			{
				drawSingleColorPattern(inDrawingContext, foregroundColor, floatBounds, patternImage);
			}
		}
		break;
	
	case 0x2593: // heavy gray pattern
		{
			NSString*	imageName = BRIDGE_CAST(AppResources_ReturnGlyphPatternDarkGrayFilenameNoExtension(),
												NSString*);
			NSImage*	patternImage = [NSImage imageNamed:imageName];
			
			
			if (nil == patternImage)
			{
				Console_Warning(Console_WriteValueCFString, "unable to find image, name", BRIDGE_CAST(imageName, CFStringRef));
			}
			else
			{
				drawSingleColorPattern(inDrawingContext, foregroundColor, floatBounds, patternImage);
			}
		}
		break;
	
#if 0
	// although this glyph is not rendered here for consistency with other
	// similar glyphs (instead, the Terminal Glyph Drawing module is used),
	// it is potentially useful to enable this when debugging because it
	// shows the given rectangle in the simplest possible way
	case 0x2588: // solid block
		{
			[foregroundNSColor setAsBackgroundInCGContext:inDrawingContext];
			CGContextFillRect(inDrawingContext, floatBounds);
		}
		break;
#endif
	
	case 0x2699: // gear
		{
			// use the contextual menu icon (same shape) but transform it
			// so that the image is drawn only in the current text color
			NSString*	imageName = BRIDGE_CAST(AppResources_ReturnContextMenuFilenameNoExtension(),
												NSString*);
			NSImage*	gearImage = [NSImage imageNamed:imageName];
			
			
			if (nil == gearImage)
			{
				Console_Warning(Console_WriteValueCFString, "unable to find image, name", BRIDGE_CAST(imageName, CFStringRef));
			}
			else
			{
				// create a square inset rectangle
				CGRect		imageFrame = CGRectMake(floatBounds.origin.x,
													floatBounds.origin.y + CGFLOAT_DIV_2(floatBounds.size.height - floatBounds.size.width),
													floatBounds.size.width, floatBounds.size.width/* make square */);
				
				
				drawSingleColorImage(inDrawingContext, foregroundColor, imageFrame, gearImage);
			}
		}
		break;
	
	default:
		// non-graphics character
		break;
	}
	
	// Braille range
	if ((inUnicode >= 0x2800) && (inUnicode <= 0x28FF))
	{
		// glyph can be generated by Terminal Glyphs module
		sourceLayerCache = [TerminalGlyphDrawing_Cache cacheWithUnicodePoint:inUnicode];
		
		// dots should remain legible at small sizes
		if (floatBounds.size.width < 8/* arbitrary */)
		{
			drawingOptions |= kTerminalGlyphDrawing_OptionAntialiasingDisabled;
		}
	}
	
	// if a glyph implementation above uses a layer, render it
	if (nil != sourceLayerCache)
	{
		TerminalGlyphDrawing_Layer*		renderingLayer = [sourceLayerCache layerWithOptions:drawingOptions
																							color:foregroundColor];
		
		
		assert(nil != renderingLayer);
		[renderingLayer renderInContext:inDrawingContext frame:floatBounds baselineHint:inBaselineHint];
	}
	
	CGColorRelease(foregroundColor), foregroundColor = nullptr;
}// drawVTGraphicsGlyph


/*!
Erases a rectangular portion of the current rendering
line of the screen display, unless the renderer is in
“no background” mode (which implies that the entire
background is handled by whatever is rendered behind
it).

Either way, the boundaries of the specified area are
returned for convenience, to avoid repeating that
calculation in the calling code.

You should call useTerminalTextColors() before using
this routine, or otherwise ensure that the fill color
of the specified context is configured appropriately.

(2.6)
*/
void
eraseSection	(My_TerminalViewPtr		inTerminalViewPtr,
				 CGContextRef			inDrawingContext,
				 SInt16					inLeftmostColumnToErase,
				 SInt16					inPastRightmostColumnToErase,
				 CGRect&				outRowSectionBounds)
{
	getRowSectionBounds(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderedLine,
						inLeftmostColumnToErase, inPastRightmostColumnToErase - inLeftmostColumnToErase,
						outRowSectionBounds);
#if 0
	// (this is now a valid case for the top of double-height lines)
	if (CGRectIsEmpty(outRowSectionBounds))
	{
		Console_Warning(Console_WriteValueFloat4, "attempt to erase empty row section",
						outRowSectionBounds.origin.x, outRowSectionBounds.origin.y,
						outRowSectionBounds.size.width, outRowSectionBounds.size.height);
	}
#endif
	
	if (false == inTerminalViewPtr->screen.currentRenderNoBackground)
	{
		CGContextSetAllowsAntialiasing(inDrawingContext, false);
		CGContextFillRect(inDrawingContext, CGRectMake(outRowSectionBounds.origin.x, outRowSectionBounds.origin.y,
														outRowSectionBounds.size.width, outRowSectionBounds.size.height));
		CGContextSetAllowsAntialiasing(inDrawingContext, true);
	}
}// eraseSection


/*!
Notifies all listeners for the specified Terminal View
event, passing the given context to the listener.

IMPORTANT:	The context must make sense for the type of
			event; see "TerminalView.h" for the context
			associated with each event.

(3.0)
*/
void
eventNotifyForView	(My_TerminalViewConstPtr	inPtr,
					 TerminalView_Event			inEvent,
					 void*						inContextPtr)
{
	// invoke listener callback routines appropriately, from the specified view’s listener model
	ListenerModel_NotifyListenersOfEvent(inPtr->changeListenerModel, inEvent, inContextPtr);
}// eventNotifyForView


/*!
Returns the terminal buffer iterator for the specified line,
which is relative to the currently visible portion of the
buffer - for example, if the 50th scrollback line is
currently topmost in the view, row index 0 refers to the
50th scrollback line.  Returns nullptr if the index is out
of range or there is any other problem creating the iterator.

NOTE:	To avoid heap allocation, "inoutStackStorageOrNull"
		can be used to pass the address of a stack variable.
		Iterators allocated in this way do not strictly
		speaking need to be released (as that operation
		becomes a no-op) but it is still OK to do so.

Calls findRowIteratorRelativeTo().

(3.0)
*/
Terminal_LineRef
findRowIterator		(My_TerminalViewPtr				inTerminalViewPtr,
					 TerminalView_RowIndex			inZeroBasedRowIndex,
					 Terminal_LineStackStorage*		inoutStackStorageOrNull)
{
	Terminal_LineRef	result = findRowIteratorRelativeTo(inTerminalViewPtr, inZeroBasedRowIndex,
															inTerminalViewPtr->screen.topVisibleEdgeInRows,
															inoutStackStorageOrNull);
	
	
	return result;
}// findRowIterator


/*!
Returns the terminal buffer iterator for the specified line,
which is relative to the given origin row (where 0 is the
topmost main screen row, and -1 is the first scrollback row).
Returns nullptr if the index is out of range or there is any
other problem creating the iterator.

It is most efficient to interact with the terminal backing
store in terms of iterator references; however, in the
physical world (view rendering), these are only useful for
finding text strings and style runs and other attributes.
This routine allows the Terminal View to usually refer to
rows by number, but get access to an iterator when necessary.

It is possible to implement this in many inefficient ways...
basically, anything that searches the buffer for an iterator
will be slow.  Ideally, one or more iterators are preserved
someplace for fast conversion from the physical (row index)
world.

NOTE:	To avoid heap allocation, "inoutStackStorageOrNull"
		can be used to pass the address of a stack variable.
		Iterators allocated in this way do not strictly
		speaking need to be released (as that operation
		becomes a no-op) but it is still OK to do so.

IMPORTANT:	Call releaseRowIterator() when finished with the
			given iterator, in case any resources were
			allocated to create it in the first place.

(4.0)
*/
Terminal_LineRef
findRowIteratorRelativeTo	(My_TerminalViewPtr				inTerminalViewPtr,
							 TerminalView_RowIndex			inZeroBasedRowIndex,
							 TerminalView_RowIndex			inOriginRow,
							 Terminal_LineStackStorage*		inoutStackStorageOrNull)
{
	Terminal_LineRef				result = nullptr;
	// normalize the requested row so that a scrollback line is negative,
	// and all main screen lines are numbered 0 or greater
	TerminalView_RowIndex const		kActualIndex = inOriginRow + inZeroBasedRowIndex;
	
	
	// TEMPORARY: this is a lazy implementation and inefficient, but it works
	if (kActualIndex < 0)
	{
		// this call needs to know which scrollback line (0 or greater) to use;
		// the normalized values are negative and use -1 to indicate the first
		// line; so, this is converted by negating the negative and subtracting
		// one so that a normalized -1 becomes 0, -2 becomes 1, etc.
		result = Terminal_NewScrollbackLineIterator(inTerminalViewPtr->screen.ref, -kActualIndex - 1,
													inoutStackStorageOrNull);
	}
	else
	{
		// main screen lines are already zero-based and need no conversion
		result = Terminal_NewMainScreenLineIterator(inTerminalViewPtr->screen.ref, STATIC_CAST(kActualIndex, UInt16),
													inoutStackStorageOrNull);
	}
	return result;
}// findRowIteratorRelativeTo


/*!
Finds the cell position in the visible screen area that is
closest to the given screen-relative pixel position (in the
coordinate system of cells).

If the point is inside the visible area, "true" is
returned, the provided row and column will be the point
directly under the specified pixel position, and the
deltas will be set to zero.

If the point is outside the visible area, "false" is
returned, the provided row and column will be the nearest
visible point, and "outDeltaColumn" and "outDeltaRow"
will be nonzero.  A negative delta means that the virtual
cell is above or to the left of the nearest visible point;
a positive delta means the cell is below or to the right
(you could use this information to scroll the proper
distance to display the point, for instance).

(4.0)
*/
Boolean
findVirtualCellFromScreenPoint	(My_TerminalViewPtr		inTerminalViewPtr,
								 HIPoint				inScreenLocalPixelPosition,
								 TerminalView_Cell&		outCell,
								 SInt16*				outDeltaColumnPtrOrNull,
								 SInt16*				outDeltaRowPtrOrNull)
{
	Boolean		result = true;
	CGFloat		columnCalculation = 0;
	CGFloat		rowCalculation = 0;
	CGFloat		deltaColumnCalculation = 0;
	CGFloat		deltaRowCalculation = 0;
	SInt16		minimumDeltaColumns = 0;
	SInt16		minimumDeltaRows = 0;
	
	
	// adjust point to fit in local screen area
	{
		CGFloat		offsetH = inScreenLocalPixelPosition.x;
		CGFloat		offsetV = inScreenLocalPixelPosition.y;
		
		
		// NOTE: This code starts in units of pixels for convenience,
		// but must convert to units of columns and rows upon return.
		if (offsetH < 0)
		{
			result = false;
			columnCalculation = 0;
			deltaColumnCalculation = offsetH;
			minimumDeltaColumns = -1;
		}
		else
		{
			CGFloat const		kScreenPrecisePixelWidth = inTerminalViewPtr->screen.cache.viewWidthInPixels.precisePixels();
			
			
			columnCalculation = offsetH;
			if (columnCalculation > kScreenPrecisePixelWidth)
			{
				result = false;
				deltaColumnCalculation = (offsetH - kScreenPrecisePixelWidth);
				columnCalculation = kScreenPrecisePixelWidth;
				minimumDeltaColumns = +1;
			}
		}
		
		// NOTE: This code starts in units of pixels for convenience,
		// but must convert to units of columns and rows upon return.
		if (offsetV < 0)
		{
			result = false;
			rowCalculation = 0;
			deltaRowCalculation = offsetV;
			minimumDeltaRows = -1;
		}
		else
		{
			CGFloat const		kScreenPrecisePixelHeight = inTerminalViewPtr->screen.cache.viewHeightInPixels.precisePixels();
			
			
			rowCalculation = offsetV;
			if (rowCalculation > kScreenPrecisePixelHeight)
			{
				result = false;
				deltaRowCalculation = (offsetV - kScreenPrecisePixelHeight);
				rowCalculation = kScreenPrecisePixelHeight;
				minimumDeltaRows = +1;
			}
		}
	}
	
	// TEMPORARY: this needs to take into account double-height text
	// (or, redefine normal-height rows to be independent here?)
	rowCalculation /= inTerminalViewPtr->text.font.heightPerCell.precisePixels();
	deltaRowCalculation /= inTerminalViewPtr->text.font.heightPerCell.precisePixels();
	
	{
		TerminalView_PixelWidth const	kWidthPerCell = getRowCharacterWidth(inTerminalViewPtr, rowCalculation);
		
		
		columnCalculation /= kWidthPerCell.precisePixels();
		deltaColumnCalculation /= kWidthPerCell.precisePixels();
	}
	
	columnCalculation += inTerminalViewPtr->screen.leftVisibleEdgeInColumns;
	rowCalculation += inTerminalViewPtr->screen.topVisibleEdgeInRows;
	
	outCell.first = STATIC_CAST(floor(columnCalculation), SInt16);
	outCell.second = STATIC_CAST(floor(rowCalculation), TerminalView_RowIndex);
	
	// the change value potentially has a minimum value (and not
	// necessarily zero) to ensure that positions outside the
	// screen will definitely shift by at least one unit
	if (nullptr != outDeltaColumnPtrOrNull)
	{
		*outDeltaColumnPtrOrNull = ((deltaColumnCalculation < 0)
									? std::min< SInt16 >(STATIC_CAST(floor(deltaColumnCalculation), SInt16), minimumDeltaColumns)
									: std::max< SInt16 >(STATIC_CAST(floor(deltaColumnCalculation), SInt16), minimumDeltaColumns));
	}
	if (nullptr != outDeltaRowPtrOrNull)
	{
		*outDeltaRowPtrOrNull = ((deltaRowCalculation < 0)
									? std::min< SInt16 >(STATIC_CAST(floor(deltaRowCalculation), SInt16), minimumDeltaRows)
									: std::max< SInt16 >(STATIC_CAST(floor(deltaRowCalculation), SInt16), minimumDeltaRows));
	}
	
	return result;
}// findVirtualCellFromScreenPoint


/*!
Given a stage of blink animation, returns its rendering color.

(4.0)
*/
inline void
getBlinkAnimationColor	(My_TerminalViewPtr		inTerminalViewPtr,
						 UInt16					inAnimationStage,
						 CGFloatRGBColor*		outColorPtr)
{
	*outColorPtr = inTerminalViewPtr->blinkColors[inAnimationStage];
}// getBlinkAnimationColor


/*!
Scans the text attributes of lines in the given range and
adds “complete” images to the given array for any cells
that render bitmaps.  Each image is added at most once, in
the order it is first visited when iterating over a sorted
range, and the entire image is added no matter how much of
the image’s rendering is in the given terminal cell region.

NOTE:	Currently only range rows are considered; a bitmap
		anywhere on the row is considered significant.

(2017.12)
*/
void
getImagesInVirtualRange		(My_TerminalViewPtr				inTerminalViewPtr,
							 TerminalView_CellRange const&	inRange,
							 NSMutableArray*				outNSImageArray)
{
	TerminalView_CellRange		orderedRange = inRange;
	Terminal_LineStackStorage	lineIteratorData;
	Terminal_LineRef			lineIterator = nullptr;
	NSMutableSet*				uniqueImages = [[NSMutableSet alloc] init];
	
	
	// require beginning point to be “earlier” than the end point; swap points if not
	sortAnchors(orderedRange.first, orderedRange.second, inTerminalViewPtr->text.selection.isRectangular);
	
	for (TerminalView_RowIndex i = orderedRange.first.second; i < orderedRange.second.second; ++i)
	{
		lineIterator = findRowIterator(inTerminalViewPtr, i, &lineIteratorData);
		Terminal_ForEachLikeAttributeRun(inTerminalViewPtr->screen.ref, lineIterator/* starting row */,
											^(UInt16					UNUSED_ARGUMENT(inLineTextBufferLength),
											  CFStringRef				UNUSED_ARGUMENT(inLineTextBufferAsCFStringOrNull),
											  Terminal_LineRef			UNUSED_ARGUMENT(inRow),
											  UInt16					UNUSED_ARGUMENT(inZeroBasedStartColumnNumber),
											  TextAttributes_Object		inAttributes)
											{
												if (inAttributes.hasBitmap())
												{
													TextAttributes_BitmapID		bitmapID = inAttributes.bitmapID();
													CGRect						subRect = CGRectZero;
													NSImage* __autoreleasing	completeImage = nil;
													Terminal_Result				terminalResult = Terminal_BitmapGetFromID(inTerminalViewPtr->screen.ref,
																															bitmapID,
																															subRect,
																															completeImage);
													
													
													// the tile image is used for rendering; only the complete image
													// is returned here (regardless of how many tiles refer to it)
													if ((kTerminal_ResultOK == terminalResult) && (nil != completeImage))
													{
														if (NO == [uniqueImages containsObject:completeImage])
														{
															[uniqueImages addObject:completeImage];
															[outNSImageArray addObject:completeImage];
														}
													}
												}
											});
		releaseRowIterator(inTerminalViewPtr, &lineIterator);
	}
}// getImagesInVirtualRange


/*!
Calculates the boundaries of the given row in pixels
relative to the SCREEN, so if you passed row 0, the
top-left corner of the rectangle would be (0, 0), but
the other boundaries depend on the font and font size.

This routine respects the top-half-double-height and
bottom-half-double-height row global attributes, such
that a double-size line is twice as big as a normal
line’s boundaries.

(3.0)
*/
void
getRowBounds	(My_TerminalViewPtr		inTerminalViewPtr,
				 TerminalView_RowIndex	inZeroBasedRowIndex,
				 CGRect&				outBounds)
{
	Terminal_LineStackStorage	rowIteratorData;
	Terminal_LineRef			rowIterator = nullptr;
	CGFloat						sectionTopEdge = (STATIC_CAST(inZeroBasedRowIndex, CGFloat) * inTerminalViewPtr->text.font.heightPerCell.precisePixels());
	TerminalView_RowIndex		topRow = 0;
	
	
	// start with the interior bounds, as this defines two of the edges
	{
		NSRect		contentFrame = [inTerminalViewPtr->encompassingNSView.terminalContentView frame];
		
		
		outBounds.size.width = contentFrame.size.width;
		outBounds.origin.x = 0;
	}
	
	// now set the top and bottom edges based on the requested row
	outBounds.origin.y = sectionTopEdge;
	outBounds.size.height = inTerminalViewPtr->text.font.heightPerCell.precisePixels();
	
	// account for double-height rows
	// TEMPORARY: this is more inefficient than it should be...
	getVirtualVisibleRegion(inTerminalViewPtr, nullptr/* left */, &topRow, nullptr/* right */, nullptr/* bottom */);
	rowIterator = findRowIterator(inTerminalViewPtr, topRow + inZeroBasedRowIndex, &rowIteratorData);
	if (nullptr != rowIterator)
	{
		TextAttributes_Object		globalAttributes;
		Terminal_Result				terminalError = kTerminal_ResultOK;
		
		
		terminalError = Terminal_GetLineGlobalAttributes(inTerminalViewPtr->screen.ref, rowIterator, &globalAttributes);
		if (kTerminal_ResultOK == terminalError)
		{
			if (globalAttributes.hasDoubleHeightTop())
			{
				// if this is the top half, the total boundaries extend downwards by one normal line height
				// but in order to have a “fat line” when selecting text, etc. the top half is considered
				// to have a ZERO height (this also works because the underlying text storage is identical
				// on both the top and bottom halves); also, the Y coordinate is offset due to the initial
				// value of "sectionTopEdge" above that assumed a normal cell height...
				outBounds.origin.y -= outBounds.size.height;
				//outBounds.size.height *= 2.0;
				outBounds.size.height = 0;
			}
			else if (globalAttributes.hasDoubleHeightBottom())
			{
				// if this is the bottom half, the total boundaries extend upwards by one normal line height
				outBounds.origin.y -= outBounds.size.height;
				outBounds.size.height *= 2.0;
			}
		}
		releaseRowIterator(inTerminalViewPtr, &rowIterator);
	}
}// getRowBounds


/*!
Returns the width in pixels of a character on the
specified row of the specified screen.  In particular
the result by vary based on whether the specified
line is currently set to double-width text.

The specified line number is relative to the rendered
display; so if 0 refers to the topmost visible row
and the user is currently displaying 10 lines of
scrollback, then the character width of line 15 will
depend on whether line 5 of the terminal buffer has
the double-width attribute set.

(3.0)
*/
TerminalView_PixelWidth
getRowCharacterWidth	(My_TerminalViewPtr		inTerminalViewPtr,
						 TerminalView_RowIndex	inLineNumber)
{
	TextAttributes_Object		globalAttributes;
	Terminal_LineStackStorage	rowIteratorData;
	Terminal_LineRef			rowIterator = nullptr;
	TerminalView_PixelWidth		result = inTerminalViewPtr->text.font.widthPerCell;
	
	
	rowIterator = findRowIterator(inTerminalViewPtr, inLineNumber, &rowIteratorData);
	if (nullptr != rowIterator)
	{
		UNUSED_RETURN(Terminal_Result)Terminal_GetLineGlobalAttributes(inTerminalViewPtr->screen.ref, rowIterator, &globalAttributes);
		if (globalAttributes.hasDoubleAny())
		{
			result.setPrecisePixels(result.precisePixels() * 2.0);
		}
		releaseRowIterator(inTerminalViewPtr, &rowIterator);
	}
	return result;
}// getRowCharacterWidth


/*!
Calculates the boundaries of the given section of
the given row in pixels relative to the SCREEN, so
if you passed row 0 and column 0, the top-left
corner of the rectangle would be (0, 0), but the
other boundaries depend on the font and font size.

This routine respects the top-half-double-height and
bottom-half-double-height row global attributes, such
that a double-size line is twice as big as a normal
line’s boundaries.

(3.0)
*/
void
getRowSectionBounds		(My_TerminalViewPtr		inTerminalViewPtr,
						 TerminalView_RowIndex	inZeroBasedRowIndex,
						 UInt16					inZeroBasedStartingColumnNumber,
						 SInt16					inCharacterCount,
						 CGRect&				outBounds)
{
	TerminalView_PixelWidth		widthPerCell = getRowCharacterWidth(inTerminalViewPtr, inZeroBasedRowIndex);
	
	
	// first find the row bounds, as this defines two of the resulting edges
	getRowBounds(inTerminalViewPtr, inZeroBasedRowIndex, outBounds);
	
	// now set the rectangle’s left and right edges based on the requested column range
	outBounds.origin.x = STATIC_CAST(inZeroBasedStartingColumnNumber, CGFloat) * widthPerCell.precisePixels();
	outBounds.size.width = STATIC_CAST(inCharacterCount, CGFloat) * widthPerCell.precisePixels();
}// getRowSectionBounds


/*!
Returns a color stored internally in the view data structure.

(3.0)
*/
inline void
getScreenBaseColor	(My_TerminalViewPtr			inTerminalViewPtr,
					 TerminalView_ColorIndex	inColorEntryNumber,
					 CGFloatRGBColor*			outColorPtr)
{
	switch (inColorEntryNumber)
	{
	case kTerminalView_ColorIndexNormalBackground:
		*outColorPtr = inTerminalViewPtr->text.colors[kMyBasicColorIndexNormalBackground];
		break;
	
	case kTerminalView_ColorIndexBoldText:
		*outColorPtr = inTerminalViewPtr->text.colors[kMyBasicColorIndexBoldText];
		break;
	
	case kTerminalView_ColorIndexBoldBackground:
		*outColorPtr = inTerminalViewPtr->text.colors[kMyBasicColorIndexBoldBackground];
		break;
	
	case kTerminalView_ColorIndexBlinkingText:
		*outColorPtr = inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingText];
		break;
	
	case kTerminalView_ColorIndexBlinkingBackground:
		*outColorPtr = inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingBackground];
		break;
	
	case kTerminalView_ColorIndexMatteBackground:
		*outColorPtr = inTerminalViewPtr->text.colors[kMyBasicColorIndexMatteBackground];
		break;
	
	case kTerminalView_ColorIndexNormalText:
	default:
		*outColorPtr = inTerminalViewPtr->text.colors[kMyBasicColorIndexNormalText];
		break;
	}
}// getScreenBaseColor


/*!
Returns the RGB foreground and background colors to use for
the specified text style attributes.

If "outNoBackgroundPtr" is set to true, it means that the
returned background color is normal, which is effectively
no color if the foreground is rendered over top of a
background view.

(4.0)
*/
void
getScreenColorsForAttributes	(My_TerminalViewPtr			inTerminalViewPtr,
								 TextAttributes_Object		inAttributes,
								 CGFloatRGBColor*			outForeColorPtr,
								 CGFloatRGBColor*			outBackColorPtr,
								 Boolean*					outNoBackgroundPtr)
{
	Boolean		isCustom = false;
	
	
	*outNoBackgroundPtr = false; // initially...
	
	//
	// choose foreground color
	//
	
	isCustom = false; // initially...
	if (inAttributes.hasAttributes(kTextAttributes_EnableForeground))
	{
		if (inAttributes.hasAttributes(kTextAttributes_ColorIndexIsBitmapID))
		{
			// color bits may be enabled but they have a different purpose in this case
			isCustom = false;
		}
		else if (inAttributes.hasAttributes(kTextAttributes_ColorIndexIsTrueColorID))
		{
			// a “true color” was chosen
			TextAttributes_TrueColorID	colorID = inAttributes.colorIDForeground();
			Terminal_Result				getIDResult = Terminal_TrueColorGetFromID(inTerminalViewPtr->screen.ref, colorID,
																					outForeColorPtr->red,
																					outForeColorPtr->green,
																					outForeColorPtr->blue);
			
			
			if (kTerminal_ResultOK != getIDResult)
			{
				// UNIMPLEMENTED: handle error
			}
			isCustom = true;
		}
		else
		{
			// one of the “core” 256 colors was chosen
			UInt16		fg = kTerminalView_ColorIndexNormalANSIBlack + inAttributes.colorIndexForeground();
			
			
			if (inAttributes.hasBold() && (fg <= kTerminalView_ColorIndexNormalANSIWhite))
			{
				// “magically” use the emphasized color for text that is actually bold
				fg += (kTerminalView_ColorIndexEmphasizedANSIBlack - kTerminalView_ColorIndexNormalANSIBlack);
			}
			isCustom = getScreenCoreColor(inTerminalViewPtr, fg, outForeColorPtr);
			if (false == isCustom)
			{
				// depending on terminal state, this might indicate an error...
				// TEMPORARY - need to add more checks here
			}
		}
	}
	
	if (false == isCustom)
	{
		// ordinary color, based on style
		TerminalView_ColorIndex		fg = kTerminalView_ColorIndexNormalText;
		
		
		// the blinking text color is favored because version 3.0 has
		// “real” bold text; therefore, if some text happens to be both
		// boldface and blinking, using the blinking text color ensures
		// the text will still be recognizeable as boldface
		if (inAttributes.hasBlink())
		{
			fg = kTerminalView_ColorIndexBlinkingText;
		}
		else if (inAttributes.hasBold())
		{
			fg = kTerminalView_ColorIndexBoldText;
		}
		else
		{
			fg = kTerminalView_ColorIndexNormalText;
		}
		
		getScreenCustomColor(inTerminalViewPtr, fg, outForeColorPtr);
	}
	
	//
	// choose background color
	//
	
	isCustom = false; // initially...
	if (inAttributes.hasAttributes(kTextAttributes_EnableBackground))
	{
		if (inAttributes.hasAttributes(kTextAttributes_ColorIndexIsTrueColorID))
		{
			// a “true color” was chosen
			TextAttributes_TrueColorID	colorID = inAttributes.colorIDBackground();
			Terminal_Result				getIDResult = Terminal_TrueColorGetFromID(inTerminalViewPtr->screen.ref, colorID,
																					outBackColorPtr->red,
																					outBackColorPtr->green,
																					outBackColorPtr->blue);
			
			
			if (kTerminal_ResultOK != getIDResult)
			{
				// UNIMPLEMENTED: handle error
			}
			isCustom = true;
		}
		else
		{
			// one of the “core” 256 colors was chosen
			UInt16		bg = kTerminalView_ColorIndexNormalANSIBlack + inAttributes.colorIndexBackground();
			
			
			if (inAttributes.hasBold() && (bg <= kTerminalView_ColorIndexNormalANSIWhite))
			{
				// “magically” use the emphasized color for text that is actually bold
				bg += (kTerminalView_ColorIndexEmphasizedANSIBlack - kTerminalView_ColorIndexNormalANSIBlack);
			}
			isCustom = getScreenCoreColor(inTerminalViewPtr, bg, outBackColorPtr);
			if (false == isCustom)
			{
				// depending on terminal state, this might indicate an error...
				// TEMPORARY - need to add more checks here
			}
		}
	}
	
	if (false == isCustom)
	{
		// ordinary color, based on style
		TerminalView_ColorIndex		bg = kTerminalView_ColorIndexNormalBackground;
		
		
		// the blinking text color is favored because version 3.0 has
		// “real” bold text; therefore, if some text happens to be both
		// boldface and blinking, using the blinking text color ensures
		// the text will still be recognizeable as boldface
		if (inAttributes.hasBlink())
		{
			bg = kTerminalView_ColorIndexBlinkingBackground;
		}
		else if (inAttributes.hasBold())
		{
			bg = kTerminalView_ColorIndexBoldBackground;
		}
		else
		{
			bg = kTerminalView_ColorIndexNormalBackground;
			*outNoBackgroundPtr = true;
		}
		getScreenCustomColor(inTerminalViewPtr, bg, outBackColorPtr);
	}
	
	// to invert, swap the colors and make sure the background is drawn
	if (inAttributes.hasAttributes(kTextAttributes_StyleInverse))
	{
		std::swap< CGFloatRGBColor >(*outForeColorPtr, *outBackColorPtr);
		*outNoBackgroundPtr = false;
	}
	
	// if the entire screen is in reverse video mode, it currently
	// cannot let “matching” background colors fall through (because
	// the background still renders the ordinary color)
	if (inTerminalViewPtr->screen.isReverseVideo)
	{
		*outNoBackgroundPtr = false;
	}
	
	if (inAttributes.hasConceal())
	{
		*outForeColorPtr = *outBackColorPtr; // make “invisible” by using same colors for everything
	}
}// getScreenColorsForAttributes


/*!
Given an index from 0 to 255, returns the appropriate color
for rendering.

NOTE that depending on the terminal configuration, not all
color indices may be valid; in fact, none of them may be.
Returns true only if the specified index is valid.

See also getScreenCustomColor(), which returns the colors
most often needed for rendering.

(4.0)
*/
inline Boolean
getScreenCoreColor	(My_TerminalViewPtr		inTerminalViewPtr,
					 UInt16					inColorEntryNumber,
					 CGFloatRGBColor*		outColorPtr)
{
	auto					kColorLevelsKey = STATIC_CAST(inColorEntryNumber, My_XTerm256Table::RGBLevelsByIndex::key_type);
	auto					kGrayLevelsKey = STATIC_CAST(inColorEntryNumber, My_XTerm256Table::GrayLevelByIndex::key_type);
	My_CGColorByIndex&		colors = inTerminalViewPtr->coreColors; // only non-const for convenience of "[]"
	My_XTerm256Table&		sourceGrid = *(inTerminalViewPtr->extendedColorsPtr); // only non-const for convenience of "[]"
	Boolean					result = false;
	
	
	if (colors.end() != colors.find(inColorEntryNumber))
	{
		// one of the basic 16 colors
		*outColorPtr = colors[inColorEntryNumber];
		result = true;
	}
	else if (sourceGrid.colorLevels.end() !=
				sourceGrid.colorLevels.find(kColorLevelsKey))
	{
		// one of the many other standard colors
		//Console_WriteValueFloat4("color", sourceGrid.colorLevels[kColorLevelsKey][0],
		//							sourceGrid.colorLevels[kColorLevelsKey][1],
		//							sourceGrid.colorLevels[kColorLevelsKey][2],
		//							0);
		My_XTerm256Table::makeCGFloatRGBColor(sourceGrid.colorLevels[kColorLevelsKey][0],
												sourceGrid.colorLevels[kColorLevelsKey][1],
												sourceGrid.colorLevels[kColorLevelsKey][2], *outColorPtr);
		result = true;
	}
	else if (sourceGrid.grayLevels.end() !=
				sourceGrid.grayLevels.find(kGrayLevelsKey))
	{
		// one of the standard grays
		//Console_WriteValue("gray", sourceGrid.grayLevels[kGrayLevelsKey]);
		My_XTerm256Table::makeCGFloatRGBColor(sourceGrid.grayLevels[kGrayLevelsKey],
												sourceGrid.grayLevels[kGrayLevelsKey],
												sourceGrid.grayLevels[kGrayLevelsKey], *outColorPtr);
		result = true;
	}
	return result;
}// getScreenCoreColor


/*!
Given a color identifier, returns its rendering color.

See also getScreenCoreColor().

(3.0)
*/
inline void
getScreenCustomColor	(My_TerminalViewPtr			inTerminalViewPtr,
						 TerminalView_ColorIndex	inColorEntryNumber,
						 CGFloatRGBColor*			outColorPtr)
{
	*outColorPtr = inTerminalViewPtr->customColors[inColorEntryNumber];
}// getScreenCustomColor


/*!
Like getVirtualRangeAsNewHIShape(), except specifically for the
current text selection.  Automatically takes into account
rectangular shape, if applicable.

(4.0)
*/
HIShapeRef
getSelectedTextAsNewHIShape		(My_TerminalViewPtr		inTerminalViewPtr,
								 Float32				inInsets)
{
	HIShapeRef		result = getVirtualRangeAsNewHIShape(inTerminalViewPtr, inTerminalViewPtr->text.selection.range.first,
															inTerminalViewPtr->text.selection.range.second, inInsets,
															inTerminalViewPtr->text.selection.isRectangular);
	
	
	return result;
}// getSelectedTextAsNewHIShape


/*!
Returns the size in bytes of the current selection for
the specified window, or zero.

(3.1)
*/
size_t
getSelectedTextSize		(My_TerminalViewPtr		inTerminalViewPtr)
{
    size_t		result = 0L;
	
	
	if (selectionExists(inTerminalViewPtr))
	{
		TerminalView_Cell const&	kSelectionStart = inTerminalViewPtr->text.selection.range.first;
		TerminalView_Cell const&	kSelectionPastEnd = inTerminalViewPtr->text.selection.range.second;
		// for rectangular selections, the size also includes space for one new-line per row
		UInt16 const				kRowWidth = (inTerminalViewPtr->text.selection.isRectangular)
												? STATIC_CAST(INTEGER_ABSOLUTE(kSelectionPastEnd.first - kSelectionStart.first + 1/* size of new-line */), UInt16)
												: Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
		
		
		result = kRowWidth * INTEGER_ABSOLUTE(kSelectionPastEnd.second - kSelectionStart.second);
	}
	
	return result;
}// getSelectedTextSize


/*!
Returns a new shape locating the specified area of the terminal
view, relative to its content view: for example, the first
character in the top-left corner has origin (0, 0) in pixels.
You must release the shape when finished with it.

For convenience, an insets parameter is allowed that will return
a shape that is larger or smaller by this amount in all
directions; pass 0.0 to not adjust the shape.

(4.0)
*/
HIShapeRef
getVirtualRangeAsNewHIShape		(My_TerminalViewPtr			inTerminalViewPtr,
								 TerminalView_Cell const&	inSelectionStart,
								 TerminalView_Cell const&	inSelectionPastEnd,
								 Float32					inInsets,
								 Boolean					inIsRectangular)
{
	HIShapeRef			result = nullptr;
	HIMutableShapeRef	mutableResult = HIShapeCreateMutable();
	CGRect				screenBounds;
	TerminalView_Cell	selectionStart;
	TerminalView_Cell	selectionPastEnd;
	Boolean				isRectangular = inIsRectangular;
	
	
	screenBounds = NSRectToCGRect([inTerminalViewPtr->encompassingNSView.terminalContentView bounds]);
	
	selectionStart = inSelectionStart;
	selectionPastEnd = inSelectionPastEnd;
	
	// normalize coordinates with respect to visible area of virtual screen
	{
		TerminalView_RowIndex	top = 0;
		UInt16					left = 0;
		
		
		getVirtualVisibleRegion(inTerminalViewPtr, &left, &top, nullptr/* right */, nullptr/* bottom */);
		selectionStart.second -= top;
		selectionStart.first -= left;
		selectionPastEnd.second -= top;
		selectionPastEnd.first -= left;
	}
	
	if ((INTEGER_ABSOLUTE(selectionPastEnd.second - selectionStart.second) <= 1) ||
		(inIsRectangular))
	{
		isRectangular = true;
	}
	
	// make the points the “right way around”, in case the first point is
	// technically to the right of or below the second point
	sortAnchors(selectionStart, selectionPastEnd, isRectangular);
	
	// iterate over rows; this is necessary because it is possible
	// for any row to be double-width, affecting the final shape
	for (CFIndex i = selectionStart.second; i < selectionPastEnd.second; ++i)
	{
		CGRect		clippedRect;
		CGRect		rowSectionBounds;
		
		
		// TEMPORARY; getRowSectionBounds() is an inefficient call that
		// probably needs a relative version when iterating through rows
		if (isRectangular)
		{
			// then the area to be highlighted is a rectangle; this simplifies things...
			getRowSectionBounds(inTerminalViewPtr, i, selectionStart.first, selectionPastEnd.first - selectionStart.first, rowSectionBounds);
		}
		else
		{
			// then the area to be highlighted is irregularly shaped; this is more complex...
			UInt16		columnCount = inTerminalViewPtr->screen.cache.columnCountAtLastNotify;
			
			
			if (i == selectionStart.second)
			{
				// bounds of first (possibly partial) line to be highlighted
				// NOTE: vertical insets are applied to end caps as extensions since the middle piece vertically shrinks
				getRowSectionBounds(inTerminalViewPtr, i, selectionStart.first, columnCount - selectionStart.first, rowSectionBounds);
			}
			else if (i == (selectionPastEnd.second - 1))
			{
				// bounds of last (possibly partial) line to be highlighted
				// NOTE: vertical insets are applied to end caps as extensions since the middle piece vertically shrinks
				getRowSectionBounds(inTerminalViewPtr, i, 0, selectionPastEnd.first, rowSectionBounds);
			}
			else
			{
				// highlight extends across more than two lines - fill in the space in between
				getRowSectionBounds(inTerminalViewPtr, i, 0, columnCount, rowSectionBounds);
			}
		}
		
		// the final selection region is the portion of the full rectangle
		// that fits within the current screen boundaries
		clippedRect = CGRectIntegral(CGRectIntersection(rowSectionBounds, screenBounds));
		UNUSED_RETURN(OSStatus)HIShapeUnionWithRect(mutableResult, &clippedRect);
	}
	
	if (0.0 != inInsets)
	{
		UNUSED_RETURN(OSStatus)HIShapeInset(mutableResult, inInsets, inInsets);
	}
	
	result = mutableResult;
	
	return result;
}// getVirtualRangeAsNewHIShape


/*!
Returns the character positions of the smallest boundary
that completely encompasses the visible region of the
specified screen.

A negative value for the row implies a cross-over into
the scrollback buffer; 0 is the topmost row of the
“main” screen.

(3.0)
*/
void
getVirtualVisibleRegion		(My_TerminalViewPtr			inTerminalViewPtr,
							 UInt16*					outLeftColumnOrNull,
							 TerminalView_RowIndex*		outTopRowOrNull,
							 UInt16*					outPastTheRightColumnOrNull,
							 TerminalView_RowIndex*		outPastTheBottomRowOrNull)
{
	if (outLeftColumnOrNull != nullptr)
	{
		*outLeftColumnOrNull = 0;
	}
	if (outTopRowOrNull != nullptr)
	{
		*outTopRowOrNull = inTerminalViewPtr->screen.topVisibleEdgeInRows;
	}
	if (outPastTheRightColumnOrNull != nullptr)
	{
		*outPastTheRightColumnOrNull = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	}
	if (outPastTheBottomRowOrNull != nullptr)
	{
		*outPastTheBottomRowOrNull = inTerminalViewPtr->screen.topVisibleEdgeInRows +
										Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
	}
}// getVirtualVisibleRegion


/*!
Creates a selection appropriate for the specified
click count.  Only call this method if at least a
double-click ("inClickCount" == 2) has occurred.

A double-click selects a word.  A triple-click
selects an entire line.

(3.0)
*/
void
handleMultiClick	(My_TerminalViewPtr		inTerminalViewPtr,
					 UInt16					inClickCount)													
{
	if (false == inTerminalViewPtr->text.selection.inhibited)
	{
		TerminalView_Cell	selectionStart;
		TerminalView_Cell	selectionPastEnd;
		SInt16 const		kColumnCount = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
		//SInt16 const		kRowCount = Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
		
		
		selectionStart = inTerminalViewPtr->text.selection.range.first;
		selectionPastEnd = inTerminalViewPtr->text.selection.range.second;
		
		// all multi-clicks result in a selection that is one line high,
		// and the range is exclusive so the row difference must be 1
		selectionPastEnd.second = selectionStart.second + 1;
		
		if (inClickCount == 2)
		{
			// double-click; invoke the registered Python word-finding callback
			// to determine which text should be selected
			Terminal_LineStackStorage	lineIteratorData;
			Terminal_LineRef			lineIterator = findRowIteratorRelativeTo(inTerminalViewPtr, selectionStart.second,
																					0/* origin row */, &lineIteratorData);
			CFStringRef					textCFString = nullptr;
			Terminal_Result				getTextError = Terminal_GetLineCFString(inTerminalViewPtr->screen.ref, lineIterator, textCFString);
			
			
			// double-click - select a word; or, do intelligent double-click
			// based on the character underneath the cursor
			// TEMPORARY; only one line is examined, but this is probably
			// more useful if it joins at least the preceding line, and
			// possible the following line as well
			if (kTerminal_ResultOK != getTextError)
			{
				Console_Warning(Console_WriteValue, "failed to obtain current line of terminal text", getTextError);
			}
			else
			{
				const char*				ptrUTF8 = [BRIDGE_CAST(textCFString, NSString*) UTF8String];
				std::string				asUTF8{ (ptrUTF8) ? ptrUTF8 : "" };
				std::pair< long, long >	wordInfo; // offset (zero-based), and count
				
				
				try
				{
					wordInfo = Quills::Terminal::word_of_char_in_string(asUTF8, selectionStart.first);
					if ((wordInfo.first >= 0) && (wordInfo.first < kColumnCount) &&
						(wordInfo.second >= 0) && ((wordInfo.first + wordInfo.second) <= kColumnCount))
					{
						selectionStart.first = STATIC_CAST(wordInfo.first, UInt16);
						selectionPastEnd.first = STATIC_CAST(wordInfo.first + wordInfo.second, UInt16);
					}
				}
				catch (std::exception const& e)
				{
					CFStringRef			titleCFString = CFSTR("Exception while trying to find double-clicked word"); // LOCALIZE THIS
					CFRetainRelease		messageCFString(CFStringCreateWithCString
														(kCFAllocatorDefault, e.what(), kCFStringEncodingUTF8),
														CFRetainRelease::kAlreadyRetained); // LOCALIZE THIS?
					
					
					Console_WriteScriptError(titleCFString, messageCFString.returnCFStringRef());
				}
			}
			releaseRowIterator(inTerminalViewPtr, &lineIterator);
		}
		else
		{
			// triple-click - select the whole line
			selectionStart.first = 0;
			selectionPastEnd.first = kColumnCount;
		}
		
		inTerminalViewPtr->text.selection.range.first = selectionStart;
		inTerminalViewPtr->text.selection.range.second = selectionPastEnd;
		highlightCurrentSelection(inTerminalViewPtr, true/* is highlighted */, true/* redraw */);
		inTerminalViewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
		copySelectedTextIfUserPreference(inTerminalViewPtr);
	}
}// handleMultiClick


/*!
Like highlightVirtualRange(), but automatically uses
the current text selection anchors for the specified
terminal view.  Has no effect if there is no selection.

(3.0)
*/
inline void
highlightCurrentSelection	(My_TerminalViewPtr		inTerminalViewPtr,
							 Boolean				inIsHighlighted,
							 Boolean				inRedraw)
{
	if (inTerminalViewPtr->text.selection.range.first != inTerminalViewPtr->text.selection.range.second)
	{
	#if 0
		Console_WriteValueFloat4("Selection range",
									inTerminalViewPtr->text.selection.range.first.first,
									inTerminalViewPtr->text.selection.range.first.second,
									inTerminalViewPtr->text.selection.range.second.first,
									inTerminalViewPtr->text.selection.range.second.second);
	#endif
		highlightVirtualRange(inTerminalViewPtr, inTerminalViewPtr->text.selection.range,
								kTextAttributes_Selected, inIsHighlighted, inRedraw);
	}
}// highlightCurrentSelection


/*!
This method highlights text in the specified terminal
window, from the starting point (inclusive) to the
ending point (exclusive).

The two anchors do not need to represent the start and
end per se; this routine automatically draws the region
between them correctly no matter which anchor is which.

IMPORTANT:	This changes ONLY the appearance of the text,
			it has no effect on important internal state
			that remembers what is selected, etc.  In
			general, you should invoke other routines to
			change the selection “properly” (like
			TerminalView_SelectVirtualRange()), and let
			those routines use this method as a helper
			to alter text rendering.

LOCALIZE THIS:	The highlighting scheme should be
				locale-sensitive; namely, the terminal
				buffer may be rendered right-to-left
				on certain systems and therefore the
				first line should flush left and the
				last line should flush right (reverse
				of North American English behavior).

(3.1)
*/
void
highlightVirtualRange	(My_TerminalViewPtr				inTerminalViewPtr,
						 TerminalView_CellRange const&	inRange,
						 TextAttributes_Object			inHighlightingStyle,
						 Boolean						inIsHighlighted,
						 Boolean						inRedraw)
{
	TerminalView_CellRange		orderedRange = inRange;
	
	
	// require beginning point to be “earlier” than the end point; swap points if not
	sortAnchors(orderedRange.first, orderedRange.second, inTerminalViewPtr->text.selection.isRectangular);
	
	// modify selection attributes
	{
		Terminal_LineStackStorage	lineIteratorData;
		Terminal_LineRef			lineIterator = findRowIteratorRelativeTo(inTerminalViewPtr, orderedRange.first.second,
																				0/* origin row */, &lineIteratorData);
		UInt16 const				kNumberOfRows = STATIC_CAST(orderedRange.second.second - orderedRange.first.second, UInt16);
		
		
		if (nullptr != lineIterator)
		{
			UNUSED_RETURN(Terminal_Result)Terminal_ChangeRangeAttributes
											(inTerminalViewPtr->screen.ref, lineIterator, kNumberOfRows,
												orderedRange.first.first, orderedRange.second.first,
												inTerminalViewPtr->text.selection.isRectangular,
												(inIsHighlighted)
												? inHighlightingStyle
												: TextAttributes_Object()/* attributes to set */,
												(inIsHighlighted)
												? TextAttributes_Object()
												: inHighlightingStyle/* attributes to clear */);
			releaseRowIterator(inTerminalViewPtr, &lineIterator);
		}
	}
	
	if (inRedraw)
	{
		// perform a boundary check, since drawSection() doesn’t do one
		{
			UInt16					startColumn = 0;
			UInt16					pastTheEndColumn = 0;
			TerminalView_RowIndex	startRow = 0;
			TerminalView_RowIndex	pastTheEndRow = 0;
			
			
			getVirtualVisibleRegion(inTerminalViewPtr, &startColumn, &startRow, &pastTheEndColumn, &pastTheEndRow);
			
			// note that these coordinates are local to the visible area,
			// and are independent of what part of the buffer is shown;
			// use the size of the visible region to determine these
			// local coordinates
			if (orderedRange.second.second >= pastTheEndRow)
			{
				orderedRange.second.second = pastTheEndRow;
			}
			if (orderedRange.second.first >= pastTheEndColumn)
			{
				orderedRange.second.first = pastTheEndColumn;
			}
		}
		
		// redraw affected area; for rectangular selections it is only
		// necessary to redraw the actual range, but for normal selections
		// (which primarily consist of full-width highlighting), the entire
		// width in the given row range is redrawn
		if (orderedRange.second.second - orderedRange.first.second > 20/* arbitrary; some large number of rows */)
		{
			// just redraw everything
			updateDisplay(inTerminalViewPtr);
		}
		else
		{
		#if 0
			// Core Graphics and QuickDraw tend to clash and introduce
			// antialiasing artifacts in edge cases; rather than try to
			// debug all of the places this could happen, a full screen
			// refresh is forced while tracking text selections
			updateDisplay(inTerminalViewPtr);
		#else
			UInt16 const	kFirstChar = (inTerminalViewPtr->text.selection.isRectangular)
											? orderedRange.first.first
											: 0;
			UInt16 const	kPastLastChar = (inTerminalViewPtr->text.selection.isRectangular)
											? orderedRange.second.first
											: Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
			
			
			for (UInt16 rowIndex = orderedRange.first.second - inTerminalViewPtr->screen.topVisibleEdgeInRows;
					rowIndex < orderedRange.second.second - inTerminalViewPtr->screen.topVisibleEdgeInRows;
					++rowIndex)
			{
				invalidateRowSection(inTerminalViewPtr, rowIndex,
										kFirstChar, kPastLastChar - kFirstChar/* count */);
			}
		#endif
		}
	}
}// highlightVirtualRange


/*!
Marks a portion of text from a single line as requiring
rendering.  If the specified terminal view currently has
drawing turned off, this routine has no effect.

Invalid areas are redrawn at the next opportunity, and
may not be drawn at all if the invalid area becomes
irrelevant before the next drawing occurs.

MacTerm maintains invalid regions in terms of logical
sections of the terminal, so that the right area is
always redrawn even after scrolling, font size changes,
etc.

To force an area to render immediately, use the routine
TerminalView_DrawRowSection().

(3.0)
*/
void
invalidateRowSection	(My_TerminalViewPtr		inTerminalViewPtr,
						 TerminalView_RowIndex	inLineNumber,
						 UInt16					inStartingColumnNumber,
						 UInt16					inCharacterCount)
{
  	CGRect		floatBounds;
	
	
	// mark the specified area; it is already in “screen coordinates”,
	// which match the view coordinates used by updateDisplayInShape()
	getRowSectionBounds(inTerminalViewPtr, inLineNumber, inStartingColumnNumber, inCharacterCount, floatBounds);
	floatBounds = CGRectIntegral(floatBounds);
	
	UNUSED_RETURN(OSStatus)HIShapeSetEmpty(gInvalidationScratchRegion());
	UNUSED_RETURN(OSStatus)HIShapeUnionWithRect(gInvalidationScratchRegion(), &floatBounds);
	updateDisplayInShape(inTerminalViewPtr, gInvalidationScratchRegion());
}// invalidateRowSection


/*!
Returns true only if the specified terminal view
should use a small-size I-beam mouse pointer.

This is currently only true if the font size is
below a certain value.

(4.1)
*/
Boolean
isSmallIBeam	(My_TerminalViewPtr		inTerminalViewPtr)
{
	Boolean		result = false;
	
	
	if (nullptr != inTerminalViewPtr)
	{
		result = (inTerminalViewPtr->text.font.normalMetrics.size < kMy_LargeIBeamMinimumFontSize);
	}
	
	return result;
}// isSmallIBeam


/*!
Returns the user’s preferred mouse pointer color for
the given terminal view.

(2020.09)
*/
TerminalView_MousePointerColor
mousePointerColor	(My_TerminalViewPtr		inTerminalViewPtr)
{
	TerminalView_MousePointerColor		result = kTerminalView_MousePointerColorRed;
	
	
	if (nullptr != inTerminalViewPtr)
	{
		result = inTerminalViewPtr->screen.mouse.pointerColor;
	}
	
	return result;
}// mousePointerColor


/*!
Changes the left visible edge of the terminal view by the
specified number of columns; if positive, the display would
move left, otherwise it would move right.  No redrawing is
done, however; the change will take effect the next time
the screen is redrawn.

Currently, this routine has no real effect, because there
is no horizontal scrolling implemented.

(3.0)
*/
void
offsetLeftVisibleEdge	(My_TerminalViewPtr		inTerminalViewPtr,
						 SInt16					inDeltaInColumns)
{
	SInt16 const	kMinimum = 0; // TEMPORARY - some day, horizontal scrolling may be supported
	SInt16 const	kMaximum = 0;
	SInt16 const	kOldDiscreteValue = inTerminalViewPtr->screen.leftVisibleEdgeInColumns;
	SInt16			newDiscreteValue = kOldDiscreteValue + inDeltaInColumns;
	
	
	newDiscreteValue = STATIC_CAST(std::max<SInt16>(kMinimum, newDiscreteValue), SInt16);
	newDiscreteValue = STATIC_CAST(std::min<SInt16>(kMaximum, newDiscreteValue), SInt16);
	inTerminalViewPtr->screen.leftVisibleEdgeInColumns = newDiscreteValue;
	
	inTerminalViewPtr->text.selection.range.first.first += (newDiscreteValue - kOldDiscreteValue);
	inTerminalViewPtr->text.selection.range.second.first += (newDiscreteValue - kOldDiscreteValue);
}// offsetLeftVisibleEdge


/*!
Changes the top visible edge of the terminal view by the
specified number of rows; if positive, the display would
move up, otherwise it would move down.  No redrawing is
done, however; the change will take effect the next time
the screen is redrawn.

(3.0)
*/
void
offsetTopVisibleEdge	(My_TerminalViewPtr		inTerminalViewPtr,
						 SInt64					inDeltaInRows)
{
	SInt64 const	kMinimum = -Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref);
	SInt64 const	kMaximum = 0;
	SInt64 const	kOldDiscreteValue = inTerminalViewPtr->screen.topVisibleEdgeInRows;
	SInt64			newDiscreteValue = kOldDiscreteValue + inDeltaInRows;
	
	
	newDiscreteValue = std::max<SInt64>(kMinimum, newDiscreteValue);
	newDiscreteValue = std::min<SInt64>(kMaximum, newDiscreteValue);
	inTerminalViewPtr->screen.topVisibleEdgeInRows = newDiscreteValue;
	
#if 0
	inTerminalViewPtr->text.selection.range.first.second += (newDiscreteValue - kOldDiscreteValue);
	inTerminalViewPtr->text.selection.range.second.second += (newDiscreteValue - kOldDiscreteValue);
#endif
	
	// hide the cursor while the main screen is not showing
	setCursorVisibility(inTerminalViewPtr, (inTerminalViewPtr->screen.topVisibleEdgeInRows >= 0));
}// offsetTopVisibleEdge


/*!
Determines whether a terminal screen cell is in the boundaries
of the highlighted text in that view (if any).  The rectangular
selection mode is also considered.

(2020.04)
*/
Boolean
pointInSelection	(My_TerminalViewPtr			inTerminalViewPtr,
					 TerminalView_Cell const&	inColumnRow)
{
	Boolean		result = false;
	
	
	// test row range, which must be satisfied regardless of the selection style
	result = ((inColumnRow.second >= inTerminalViewPtr->text.selection.range.first.second) &&
				(inColumnRow.second < inTerminalViewPtr->text.selection.range.second.second));
	if (result)
	{
		if ((inTerminalViewPtr->text.selection.isRectangular) ||
			(1 == (inTerminalViewPtr->text.selection.range.second.second - inTerminalViewPtr->text.selection.range.first.second)))
		{
			// purely rectangular selection range; verify column range
			result = ((inColumnRow.first >= inTerminalViewPtr->text.selection.range.first.first) &&
						(inColumnRow.first < inTerminalViewPtr->text.selection.range.second.first));
		}
		else
		{
			// Mac-like selection range
			if (inColumnRow.second == inTerminalViewPtr->text.selection.range.first.second)
			{
				// first row; range from the column (inclusive) to the end is valid
				result = ((inColumnRow.first >= inTerminalViewPtr->text.selection.range.first.first) &&
							(inColumnRow.first < Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref)));
			}
			else if (inColumnRow.second == (inTerminalViewPtr->text.selection.range.second.second - 1))
			{
				// last row; range from the start to the column (exclusive) is valid
				result = (inColumnRow.first < inTerminalViewPtr->text.selection.range.second.first);
			}
			else
			{
				// anywhere in between is fine
				result = true;
			}
		}
	}
	
	return result;
}// pointInSelection


/*!
Adds menu items to the specified menu based on the
current state of the given terminal view.

To keep menus short, this routine builds three types of
menus: one for text selections, one for image selections,
and one if nothing is selected.  If text is selected,
this routine assumes that the user will most likely not
want any of the more-general items.  Image selections
offer a subset of the text selection set.  Otherwise,
general items are available whenever they are applicable.

(4.1)
*/
void
populateContextualMenu	(My_TerminalViewPtr		inTerminalViewPtr,
						 NSMenu*				inoutMenu)
{
	NSMenuItem*		newItem = nil;
	CFStringRef		commandText = nullptr;
	
	
	ContextSensitiveMenu_Init();
	
	if (TerminalView_TextSelectionExists(inTerminalViewPtr->selfRef))
	{
		NSMutableArray*		selectedImages = [[NSMutableArray alloc] init];
		Boolean				isImageSelection = false;
		
		
		getImagesInVirtualRange(inTerminalViewPtr, inTerminalViewPtr->text.selection.range, selectedImages);
		isImageSelection = (selectedImages.count > 0);
		
		// URL commands
		ContextSensitiveMenu_NewItemGroup();
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuOpenThisResource, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performOpenURL:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// clipboard-related text and image selection commands
		ContextSensitiveMenu_NewItemGroup();
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuCopyToClipboard, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(copy:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		if (false == isImageSelection)
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuCopyUsingTabsForSpaces, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performCopyWithTabSubstitution:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				}
				CFRelease(commandText), commandText = nullptr;
			}
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuSaveSelectedText, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performSaveSelection:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		if (false == isImageSelection)
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuShowCompletions, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performShowCompletions:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				}
				CFRelease(commandText), commandText = nullptr;
			}
		}
		
		if (false == isImageSelection)
		{
			// user macros
			MacroManager_AddContextualMenuGroup(inoutMenu, nullptr/* current macro set */, true/* search defaults */); // implicitly calls ContextSensitiveMenu_NewItemGroup() again
		}
		
		// other text-selection-related commands
		ContextSensitiveMenu_NewItemGroup();
		
		if (false == isImageSelection)
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuPrintSelectedText, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performPrintSelection:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				}
				CFRelease(commandText), commandText = nullptr;
			}
		}
		
		if (false == isImageSelection)
		{
			if (UIStrings_Copy(kUIStrings_ContextualMenuSpeakSelectedText, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(startSpeaking:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				}
				CFRelease(commandText), commandText = nullptr;
			}
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuStopSpeaking, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(stopSpeaking:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
	}
	else
	{
		// text-editing-related menu items
		ContextSensitiveMenu_NewItemGroup();
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuPasteText, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(paste:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
			
			// add an informational (disabled) item for Paste mode, similar to
			// the one in the main menu; rely on existing item validation to
			// automatically set the title text appropriately for the item
			newItem = [[NSMenuItem alloc] initWithTitle:@"<Paste mode info>" action:@selector(performAssessBracketedPasteMode:) keyEquivalent:@""];
			newItem.indentationLevel = 2; // (match main menu)
			[inoutMenu addItem:newItem]; // add directly to prevent conditional hiding of the disabled item
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuFindInThisWindow, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performFind:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// some macros can now operate on Clipboard data; insert items as appropriate
		MacroManager_AddContextualMenuGroup(inoutMenu, nullptr/* current macro set */, true/* search defaults */); // implicitly calls ContextSensitiveMenu_NewItemGroup() again
		
		// window arrangement menu items
		ContextSensitiveMenu_NewItemGroup();
		
		{
			NSWindow*			screenWindow = TerminalView_ReturnNSWindow(inTerminalViewPtr->selfRef);
			TerminalWindowRef	terminalWindow = [screenWindow terminalWindowRef];
			
			
			if (TerminalWindow_IsFullScreen(terminalWindow))
			{
				if (UIStrings_Copy(kUIStrings_ContextualMenuFullScreenExit, commandText).ok())
				{
					newItem = Commands_NewMenuItemForAction(@selector(toggleFullScreen:), commandText, true/* must be enabled */);
					if (nil != newItem)
					{
						ContextSensitiveMenu_AddItem(inoutMenu, newItem);
					}
					CFRelease(commandText), commandText = nullptr;
				}
			}
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuHideThisWindow, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performHideWindow:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuArrangeAllInFront, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performArrangeInFront:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuRenameThisWindow, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performRename:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// terminal-related menu items
		ContextSensitiveMenu_NewItemGroup();
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuPrintScreen, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performPrintScreen:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuCustomScreenDimensions, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performScreenResizeCustom:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuCustomFormat, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performFormatCustom:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuSpecialKeySequences, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(performMappingCustom:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// speech-related items
		ContextSensitiveMenu_NewItemGroup();
		
		// NOTE: this item is available whether or not there is a text selection,
		// as long as speech is in progress
		if (UIStrings_Copy(kUIStrings_ContextualMenuStopSpeaking, commandText).ok())
		{
			newItem = Commands_NewMenuItemForAction(@selector(stopSpeaking:), commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
			}
			CFRelease(commandText), commandText = nullptr;
		}
	}
}// populateContextualMenu


/*!
Invoked whenever a monitored preference value is changed
(see TerminalView_Init() to see which preferences are
monitored).  This routine responds by ensuring that internal
variables are up to date for the changed preference.

(3.0)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagCursorBlinks:
		// update global variable with current preference value
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCursorBlinks, sizeof(gPreferenceProxies.cursorBlinks),
									&gPreferenceProxies.cursorBlinks))
		{
			gPreferenceProxies.cursorBlinks = false; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagNotifyOfBeeps:
		// update global variable with current preference value
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNotifyOfBeeps, sizeof(gPreferenceProxies.notifyOfBeeps),
									&gPreferenceProxies.notifyOfBeeps))
		{
			gPreferenceProxies.notifyOfBeeps = false; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagPureInverse:
		// update global variable with current preference value
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagPureInverse, sizeof(gPreferenceProxies.invertSelections),
									&gPreferenceProxies.invertSelections))
		{
			gPreferenceProxies.invertSelections = false; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagTerminalCursorType:
		// update global variable with current preference value
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagTerminalCursorType, sizeof(gPreferenceProxies.cursorType),
									&gPreferenceProxies.cursorType))
		{
			gPreferenceProxies.cursorType = kTerminal_CursorTypeBlock; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagTerminalShowMarginAtColumn:
		// update global variable with current preference value
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagTerminalShowMarginAtColumn, sizeof(gPreferenceProxies.renderMarginAtColumn),
									&gPreferenceProxies.renderMarginAtColumn))
		{
			gPreferenceProxies.renderMarginAtColumn = 0; // assume a value, if preference can’t be found
		}
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Invoked whenever a monitored preference value is changed for
a particular view (see TerminalView_New() to see which
preferences are monitored).  This routine responds by updating
any necessary data and display elements.

(3.0)
*/
void
preferenceChangedForView	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inPreferenceTagThatChanged,
							 void*					inPreferencesContext,
							 void*					inTerminalViewRef)
{
	// WARNING: The context is only defined for the preferences monitored in a
	// context-specific way through Preferences_ContextStartMonitoring() calls.
	// Otherwise, the data type of the input is "Preferences_ChangeContext*".
	Preferences_ContextRef		prefsContext = REINTERPRET_CAST(inPreferencesContext, Preferences_ContextRef);
	TerminalViewRef				view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
	
	
	// ignore changes to settings that are part of the view’s filter
	unless (viewPtr->configFilter.end() != viewPtr->configFilter.find(inPreferenceTagThatChanged))
	{
		switch (inPreferenceTagThatChanged)
		{
		case kPreferences_TagDontDimBackgroundScreens:
			{
				// update global variable with current preference value
				unless (kPreferences_ResultOK ==
						Preferences_GetData(kPreferences_TagDontDimBackgroundScreens, sizeof(gPreferenceProxies.dontDimTerminals),
											&gPreferenceProxies.dontDimTerminals))
				{
					gPreferenceProxies.dontDimTerminals = false; // assume a value, if preference can’t be found
				}
				
				updateDisplay(viewPtr);
			}
			break;
		
		case kPreferences_TagTerminalCursorType:
			// recalculate cursor boundaries for the specified view
			if (nil != viewPtr->encompassingNSView.terminalContentView)
			{
				Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
				UInt16				cursorX = 0;
				UInt16				cursorY = 0;
				
				
				// invalidate the entire old cursor region (in case it is bigger than the new one)
				updateDisplayInShape(viewPtr, viewPtr->screen.cursor.updatedShape);
				
				// find the new cursor region
				getCursorLocationError = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
				if (kTerminal_ResultOK != getCursorLocationError)
				{
					Console_Warning(Console_WriteValue, "failed to update cursor after preference change; internal error", getCursorLocationError);
				}
				else
				{
					setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds, viewPtr->screen.cursor.updatedShape);
					
					// invalidate the new cursor region (in case it is bigger than the old one)
					updateDisplayInShape(viewPtr, viewPtr->screen.cursor.updatedShape);
				}
			}
			break;
		
		case kPreferences_TagTerminalResizeAffectsFontSize:
			// change the display mode for the specified view
			{
				Boolean		resizeAffectsFont = false;
				
				
				unless (kPreferences_ResultOK ==
						Preferences_GetData(kPreferences_TagTerminalResizeAffectsFontSize, sizeof(resizeAffectsFont),
											&resizeAffectsFont))
				{
					resizeAffectsFont = false; // assume a value, if preference can’t be found
				}
				UNUSED_RETURN(TerminalView_Result)TerminalView_SetDisplayMode(view, (resizeAffectsFont)
																					? kTerminalView_DisplayModeZoom
																					: kTerminalView_DisplayModeNormal);
			}
			break;
		
		default:
			if (kPreferences_ChangeContextBatchMode == inPreferenceTagThatChanged)
			{
				// batch mode; multiple things have changed, so check for the new values
				// of everything that is understood by a terminal view
				if (Quills::Prefs::TRANSLATION == Preferences_ContextReturnClass(prefsContext))
				{
					copyTranslationPreferences(viewPtr, prefsContext);
				}
				else
				{
					UNUSED_RETURN(UInt16)copyColorPreferences(viewPtr, prefsContext, true/* search for defaults */);
					UNUSED_RETURN(UInt16)copyFontPreferences(viewPtr, prefsContext, true/* search for defaults */);
				}
				updateDisplay(viewPtr);
			}
			else
			{
				// ???
			}
			break;
		}
	}
}// preferenceChangedForView


/*!
Caches various measurements in pixels, based on the current
font dimensions and the current number of columns, rows and
scrollback buffer lines.

It only makes sense to call this after one of the dependent
factors, above, is changed (such as font size or the screen
buffer size).

For code clarity, EVERY metric defined by this routine should
be in the "cache" sub-structure, and ONLY HERE should those
cached settings ever be changed.

(3.0)
*/
void
recalculateCachedDimensions		(My_TerminalViewPtr		inTerminalViewPtr)
{
	//UInt32 const	kScrollbackLines = Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref);
	CGFloat const	kVisibleWidth = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	CGFloat const	kVisibleLines = Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
	
	
	inTerminalViewPtr->screen.cache.viewWidthInPixels.setPrecisePixels(kVisibleWidth * inTerminalViewPtr->text.font.widthPerCell.precisePixels());
	inTerminalViewPtr->screen.cache.viewHeightInPixels.setPrecisePixels(kVisibleLines * inTerminalViewPtr->text.font.heightPerCell.precisePixels());
}// recalculateCachedDimensions


/*!
Receives notification whenever a monitored terminal
screen buffer’s video mode changes, and responds by
reversing the video of the terminal view to match.

(3.0)
*/
void
receiveVideoModeChange	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inTerminalChange,
						 void*					inEventContextPtr,
						 void*					inTerminalViewRef)
{
	TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
	TerminalViewRef		view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	
	
	assert(inTerminalChange == kTerminal_ChangeVideoMode);
	TerminalView_ReverseVideo(view, Terminal_ReverseVideoIsEnabled(screen));
}// receiveVideoModeChange


/*!
Releases an iterator previously acquired with the
findRowIterator() routine.

Depending on the findRowIterator() implementation,
this call may free memory or merely note that
fewer references to the given iterator now exist.

(3.0)
*/
void
releaseRowIterator  (My_TerminalViewPtr		UNUSED_ARGUMENT(inTerminalViewPtr),
					 Terminal_LineRef*		inoutRefPtr)
{
	Terminal_DisposeLineIterator(inoutRefPtr);
}// releaseRowIterator


/*!
Removes a data source previously specified with addDataSource()
(or all data sources if "nullptr" is given) and returns true
only if all requested data source(s) were actually removed.

(4.1)
*/
Boolean
removeDataSource	(My_TerminalViewPtr		inTerminalViewPtr,
					 TerminalScreenRef		inScreenDataSourceOrNull)
{
	Boolean		result = false;
	
	
	// currently only one source is possible so the implementation
	// is more straightforward (IMPORTANT: the steps below should
	// be the inverse of the code in addDataSource())
	if (inScreenDataSourceOrNull == inTerminalViewPtr->screen.ref)
	{
		// remove specified screen buffer reference (currently can only be one)
		Terminal_ReleaseScreen(&(inTerminalViewPtr->screen.ref));
		assert(nullptr == inTerminalViewPtr->screen.ref);
		result = true;
	}
	else if (nullptr == inScreenDataSourceOrNull)
	{
		// remove all screen buffer references (currently can only be one)
		Terminal_ReleaseScreen(&(inTerminalViewPtr->screen.ref));
		assert(nullptr == inTerminalViewPtr->screen.ref);
		result = true;
	}
	
	return result;
}// removeDataSource


/*!
Internal version of TerminalView_ReturnSelectedTextCopyAsUnicode().

(3.1)
*/
CFStringRef
returnSelectedTextCopyAsUnicode		(My_TerminalViewPtr			inTerminalViewPtr,
									 UInt16						inMaxSpacesToReplaceWithTabOrZero,
									 TerminalView_TextFlags		inFlags)
{
    CFStringRef		result = nullptr;
	
	
	if (selectionExists(inTerminalViewPtr))
	{
		TerminalView_Cell const&	kSelectionStart = inTerminalViewPtr->text.selection.range.first;
		TerminalView_Cell const&	kSelectionPastEnd = inTerminalViewPtr->text.selection.range.second;
		// TEMPORARY: It is probably easy enough to determine what this size should be in advance,
		// which would save a lot of reallocations.
		CFMutableStringRef			resultMutable = CFStringCreateMutable(kCFAllocatorDefault, 0/* size limit */);
		
		
		if (nullptr != resultMutable)
		{
			Terminal_LineStackStorage	lineIteratorData;
			Terminal_LineRef			lineIterator = findRowIteratorRelativeTo(inTerminalViewPtr, 0,
																					kSelectionStart.second,
																					&lineIteratorData);
			Terminal_Result				iteratorAdvanceResult = kTerminal_ResultOK;
			Terminal_Result				textGrabResult = kTerminal_ResultOK;
			Terminal_Result				attributeGrabResult = kTerminal_ResultOK;
			
			
			// read every line of Unicode characters within this range;
			// if appropriate, ignore some characters on each line
			for (CFIndex i = kSelectionStart.second; i < kSelectionPastEnd.second; ++i)
			{
				CFStringRef				referenceCFString = nullptr;
				CFRange					stringRange = CFRangeMake(0, 0);
				TextAttributes_Object	lineGlobalAttributes;
				Boolean					skipLine = false;
				
				
				attributeGrabResult = Terminal_GetLineGlobalAttributes(inTerminalViewPtr->screen.ref, lineIterator, &lineGlobalAttributes);
				if (kTerminal_ResultOK == attributeGrabResult)
				{
					if (lineGlobalAttributes.hasDoubleHeightTop())
					{
						// double-height text is replicated on two lines and the top half
						// is not rendered at all; skip the top half (the bottom half
						// will have identical text and this will match the user’s
						// expectation of seeing the text only appear once)
						skipLine = true;
					}
				}
				
				if (false == skipLine)
				{
					if ((inTerminalViewPtr->text.selection.isRectangular) ||
						(1 == (kSelectionPastEnd.second - kSelectionStart.second)))
					{
						// for rectangular or one-line selections, copy a specific column range
						textGrabResult = Terminal_GetLineRange(inTerminalViewPtr->screen.ref, lineIterator,
																kSelectionStart.first, kSelectionPastEnd.first,
																referenceCFString, stringRange);
						if (kTerminal_ResultOK != textGrabResult)
						{
							Console_Warning(Console_WriteValue, "one-line text copy failed, terminal error", textGrabResult);
							break;
						}
					}
					else
					{
						// for standard selections, the first and last lines are different
						// TEMPORARY: whitespace exclusion flags are mostly a hack to work
						// around the fact that terminals do not currently know where a
						// line actually ends; they store whitespace for the full width,
						// and it is undesirable to pad copied lines with meaningless spaces;
						// heuristics are employed to arbitrarily strip this end space most
						// of the time, making an exception for short (~2 line) wraps that
						// are most likely part of the same, continuing line anyway
						if (i == kSelectionStart.second)
						{
							// first line is anchored at the end (LOCALIZE THIS)
							textGrabResult = Terminal_GetLineRange(inTerminalViewPtr->screen.ref, lineIterator,
																	kSelectionStart.first, -1/* end column */,
																	referenceCFString, stringRange,
																	(2/* arbitrary */ == (kSelectionPastEnd.second - kSelectionStart.second))
																		? 0/* flags */
																		: kTerminal_TextFilterFlagsNoEndWhitespace);
							if (kTerminal_ResultOK != textGrabResult)
							{
								Console_Warning(Console_WriteValue, "first-line-anchored-at-end text copy failed, terminal error", textGrabResult);
								break;
							}
						}
						else if (i == (kSelectionPastEnd.second - 1))
						{
							// last line is anchored at the beginning (LOCALIZE THIS)
							textGrabResult = Terminal_GetLineRange(inTerminalViewPtr->screen.ref, lineIterator,
																	0/* start column */, kSelectionPastEnd.first,
																	referenceCFString, stringRange, kTerminal_TextFilterFlagsNoEndWhitespace);
							if (kTerminal_ResultOK != textGrabResult)
							{
								Console_Warning(Console_WriteValue, "last-line-anchored-at-beginning text copy failed, terminal error", textGrabResult);
								break;
							}
						}
						else
						{
							// middle lines span the whole width
							textGrabResult = Terminal_GetLine(inTerminalViewPtr->screen.ref, lineIterator,
																referenceCFString, stringRange, kTerminal_TextFilterFlagsNoEndWhitespace);
							if (kTerminal_ResultOK != textGrabResult)
							{
								Console_Warning(Console_WriteValue, "middle-spanning-line text copy failed, terminal error", textGrabResult);
								break;
							}
						}
					}
					
					// add the characters for the line...
					{
						CFRetainRelease		substringObject(CFStringCreateWithSubstring(kCFAllocatorDefault, referenceCFString, stringRange),
															CFRetainRelease::kAlreadyRetained);
						
						
						if (substringObject.exists())
						{
							CFStringAppend(resultMutable, substringObject.returnCFStringRef());
							
							// perform spaces-to-tabs substitution; this could be done while
							// appending text in the first place but instead it is done as a
							// lazy post-processing step (also, no effort is made to perform
							// particularly well...multiple linear searches are done to
							// iteratively replace ranges of spaces)
							if (inMaxSpacesToReplaceWithTabOrZero > 0)
							{
								// create a string to represent the required whitespace range
								// (TEMPORARY: there is no reason to do this every time, it
								// could be cached at the time the user preference changes;
								// for now however the lazy approach is being taken)
								CFRetainRelease		spacesString(StringUtilities_ReturnBlankStringCopy(inMaxSpacesToReplaceWithTabOrZero),
																	CFRetainRelease::kAlreadyRetained);
								CFStringRef			tabString = CFSTR("\011"); // LOCALIZE THIS?
								
								
								// first replace all “long” series of spaces with single tabs, as per user preference
								UNUSED_RETURN(CFIndex)CFStringFindAndReplace(resultMutable, spacesString.returnCFStringRef(), tabString,
																				CFRangeMake(0, CFStringGetLength(resultMutable)),
																				0/* comparison options */);
								
								// replace all smaller ranges of spaces with one tab as well
								for (UInt16 maxSpaces = (inMaxSpacesToReplaceWithTabOrZero - 1); maxSpaces > 0; --maxSpaces)
								{
									// create a string representing the next-smallest range of spaces...
									CFRetainRelease		smallerSpacesString(StringUtilities_ReturnBlankStringCopy(maxSpaces),
																			CFRetainRelease::kAlreadyRetained);
									
									
									// ...and turn those spaces into a tab
									UNUSED_RETURN(CFIndex)CFStringFindAndReplace(resultMutable, smallerSpacesString.returnCFStringRef(), tabString,
																					CFRangeMake(0, CFStringGetLength(resultMutable)),
																					0/* comparison options */);
								}
							}
						}
					}
					
					// if requested, add a new-line
					if (0 == (inFlags & kTerminalView_TextFlagInline))
					{
						// do not terminate last line unless requested
						if ((i < (kSelectionPastEnd.second - 1)) ||
							(0 != (inFlags & kTerminalView_TextFlagLastLineHasSeparator)))
						{
							// TEMPORARY; should this also have the option of capturing
							// text in other ways, such as the session’s default line-endings?
							if (inFlags & kTerminalView_TextFlagLineSeparatorLF)
							{
								CFStringAppendCString(resultMutable, "\012", kCFStringEncodingASCII);
							}
							else
							{
								CFStringAppendCString(resultMutable, "\015", kCFStringEncodingASCII);
							}
						}
					}
				}
				
				iteratorAdvanceResult = Terminal_LineIteratorAdvance(inTerminalViewPtr->screen.ref, lineIterator, +1);
				if (kTerminal_ResultIteratorCannotAdvance == iteratorAdvanceResult)
				{
					// last line of the buffer has been reached
					break;
				}
			}
			releaseRowIterator(inTerminalViewPtr, &lineIterator);
			
			result = resultMutable;
		}
	}
	else
	{
		result = CFSTR("");
		CFRetain(result);
	}
	
	return result;
}// returnSelectedTextCopyAsUnicode


/*!
Receives notification whenever a monitored terminal
screen buffer’s text changes, and responds by
invalidating the appropriate part of the view IF
the changed area is actually visible.  Also responds
to scroll activity by recalculating the total size
of the screen area in pixels (taking into account
new rows added to the scrollback buffer).

(3.0)
*/
void
screenBufferChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inTerminalChange,
						 void*					inEventContextPtr,
						 void*					inTerminalViewRef)
{
	TerminalViewRef				view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
	
	
	switch (inTerminalChange)
	{
	case kTerminal_ChangeScreenSize:
		{
			TerminalScreenRef	screenRef = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			UInt16 const		newColumnCount = Terminal_ReturnColumnCount(screenRef);
			UInt16 const		newRowCount = Terminal_ReturnRowCount(screenRef);
			
			
			// this notification is only sent if the view change actually modified the screen dimensions
			if ((viewPtr->screen.cache.columnCountAtLastNotify != newColumnCount) ||
				(viewPtr->screen.cache.rowCountAtLastNotify != newRowCount))
			{
				TerminalView_ScreenInfo		infoStruct;
				
				
				bzero(&infoStruct, sizeof(infoStruct));
				infoStruct.viewRef = view;
				infoStruct.screenRef = screenRef;
				
				viewPtr->screen.cache.columnCountAtLastNotify = newColumnCount;
				viewPtr->screen.cache.rowCountAtLastNotify = newRowCount;
				
				eventNotifyForView(viewPtr, kTerminalView_EventScreenSizeChanged,
									&infoStruct/* context */);
			}
		}
		break;
	
	case kTerminal_ChangeTextEdited:
		{
			Terminal_RangeDescriptionConstPtr	rangeInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																				Terminal_RangeDescriptionConstPtr);
			SInt64								i = 0;
			
			
			// debug
			//Console_WriteValuePair("first changed row, number of changed rows",
			//						rangeInfoPtr->firstRow, rangeInfoPtr->rowCount);
			for (i = rangeInfoPtr->firstRow - viewPtr->screen.topVisibleEdgeInRows;
					i < (rangeInfoPtr->firstRow - viewPtr->screen.topVisibleEdgeInRows + STATIC_CAST(rangeInfoPtr->rowCount, SInt32)); ++i)
			{
				invalidateRowSection(viewPtr, i, rangeInfoPtr->firstColumn, rangeInfoPtr->columnCount);
			}
			
			// when multiple rows change, just redraw everything
			if (rangeInfoPtr->rowCount > 1)
			{
				updateDisplay(viewPtr);
			}
		}
		break;
	
	case kTerminal_ChangeScrollActivity:
		{
			Terminal_ScrollDescriptionConstPtr	rangeInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																				Terminal_ScrollDescriptionConstPtr);
			
			
			if (viewPtr->animation.timer.isActive)
			{
				UNUSED_RETURN(OSStatus)HIShapeSetEmpty(viewPtr->animation.rendering.region);
			}
			recalculateCachedDimensions(viewPtr);
			
			highlightCurrentSelection(viewPtr, false/* highlight */, true/* draw */);
			viewPtr->text.selection.range.first.second += rangeInfoPtr->rowDelta;
			viewPtr->text.selection.range.second.second += rangeInfoPtr->rowDelta;
			highlightCurrentSelection(viewPtr, true/* highlight */, true/* draw */);
			
			updateDisplay(viewPtr);
		}
		break;
	
	case kTerminal_ChangeXTermColor:
		{
			Terminal_XTermColorDescriptionConstPtr	colorInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																					Terminal_XTermColorDescriptionConstPtr);
			
			
			if (&gColorGrid() == viewPtr->extendedColorsPtr)
			{
				// if the global table is in use, a copy must be made before any
				// customizations can occur
				viewPtr->extendedColorsPtr = new My_XTerm256Table;
			}
			viewPtr->extendedColorsPtr->setColor(colorInfoPtr->index, colorInfoPtr->redComponent, colorInfoPtr->greenComponent,
													colorInfoPtr->blueComponent);
			updateDisplay(viewPtr);
		}
		break;
	
	default:
		// ???
		assert(false && "unexpected screen buffer event sent to terminal view handler");
		break;
	}
}// screenBufferChanged


/*!
Receives notification whenever a monitored terminal
screen’s cursor is moved to a new row or column or
changes visibility, and responds by invalidating the
current cursor rectangle.  If the cursor is moving,
its rectangle is then changed and invalidated again.

(3.0)
*/
void
screenCursorChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inTerminalChange,
						 void*					inEventContextPtr,
						 void*					inTerminalViewRef)
{
	TerminalScreenRef			screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
	TerminalViewRef				view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
	
	
	//Console_WriteLine("screen cursor change notification at view level");
	assert((inTerminalChange == kTerminal_ChangeCursorLocation) ||
			(inTerminalChange == kTerminal_ChangeCursorState));
	if (viewPtr->screen.cursor.currentState != kMyCursorStateInvisible)
	{
		//
		// IMPORTANT: An odd inefficiency crops up in Cocoa views if the two
		// cursor rectangles are simply invalidated: a request is generated
		// to refresh the entire region of text in between the two cursor
		// positions!  This is because Cocoa likes to refresh a few large
		// rectangles instead of lots of small ones; that might make sense
		// when the update region forms a strange shape but it’s overkill
		// for two cursor locations that really are tiny rectangles.  To
		// make updating more sensible, the cursor shapes are rendered
		// immediately without altering the update-region.
		//
		
		if (viewPtr->isCocoa())
		{
			CGRect		oldCursorRect = CGRectZero;
			
			
			UNUSED_RETURN(CGRect*)HIShapeGetBounds(viewPtr->screen.cursor.updatedShape, &oldCursorRect);
			//[viewPtr->encompassingNSView.terminalContentView displayRect:NSRectFromCGRect(oldCursorRect)];
		}
		else
		{
			// when moving or hiding/showing the cursor, invalidate its original rectangle
			updateDisplayInShape(viewPtr, viewPtr->screen.cursor.updatedShape);
		}
		
		//UNUSED_RETURN(OSStatus)HIShapeEnumerate(viewPtr->screen.cursor.updatedShape, kHIShapeParseFromTopLeft, Console_WriteShapeElement, nullptr/* ref. con. */); // debug
		if (inTerminalChange == kTerminal_ChangeCursorLocation)
		{
			// in addition, when moving, recalculate the new bounds and invalidate again
			Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
			UInt16				cursorX = 0;
			UInt16				cursorY = 0;
			
			
			getCursorLocationError = Terminal_CursorGetLocation(screen, &cursorX, &cursorY);
			if (kTerminal_ResultOK != getCursorLocationError)
			{
				Console_Warning(Console_WriteValue, "failed to update cursor after change in cursor location or state; internal error", getCursorLocationError);
			}
			else
			{
				//Console_WriteValuePair("notification passed new location", cursorX, cursorY);
				setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds, viewPtr->screen.cursor.updatedShape);
				//UNUSED_RETURN(OSStatus)HIShapeEnumerate(viewPtr->screen.cursor.updatedShape, kHIShapeParseFromTopLeft, Console_WriteShapeElement, nullptr/* ref. con. */); // debug
				
				if (viewPtr->isCocoa())
				{
					//[viewPtr->encompassingNSView.terminalContentView displayRect:NSRectFromCGRect(viewPtr->screen.cursor.bounds)];
				}
				else
				{
					updateDisplayInShape(viewPtr, viewPtr->screen.cursor.updatedShape);
				}
			}
		}
	}
}// screenCursorChanged


/*!
Returns true only if the text selection range is not empty.
(Currently this does not check image selections.)

(2019.04)
*/
Boolean
selectionExists		(My_TerminalViewPtr		inTerminalViewPtr)
{
	Boolean		result = false;
	SInt64		rowRange = (inTerminalViewPtr->text.selection.range.second.second - inTerminalViewPtr->text.selection.range.first.second);
	
	
	if (1 == rowRange)
	{
		// range refers to a single row; check for empty column range
		result = (inTerminalViewPtr->text.selection.range.first.first != inTerminalViewPtr->text.selection.range.second.first);
	}
	else if (0 == rowRange)
	{
		result = false;
	}
	else
	{
		result = true;
	}
	
	return result;
}// selectionExists


/*!
Modifies the current text selection, creating a new selection
at the cursor location if necessary.  This is generally only
called in response to a keyboard/mouse action.

If the number of columns decreases or increases beyond a
screen boundary, the selection moves to the adjacent line
even if "inDeltaRow" is equal to 0.  The "inAllowScrolling"
flag allows the view display to move if the selection moves
to a line that is not currently visible.

If a new selection must be created, the specified deltas are
used to determine initial anchoring behavior, causing any
subsequent calls to extend the start or end of the selection
in a consistent manner.  For example, -1 might mean “shrink
selection” if the selection is already growing to the right
but it might mean “extend selection” if the selection is
already growing to the left.  The My_TerminalView property
"text.selection.keyboardMode" stores the anchor information
across calls.

(2019.04)
*/
void
selectionModify		(My_TerminalViewPtr				inTerminalViewPtr,
					 SInt16							inDeltaColumn,
					 SInt16							inDeltaRow,
					 My_SelectionExtensionType		inExtensionType,
					 Boolean						inAllowScrolling)
{
	UInt16 const	kMinColumn = 0;
	UInt16 const	kMaxColumn = (Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref) - 1);
	Boolean			didChangeRows = false;
	
	
	// this step may not be required, as stored ranges should
	// already be sorted; for now, do it every time...
	sortAnchors(inTerminalViewPtr->text.selection.range.first, inTerminalViewPtr->text.selection.range.second,
				inTerminalViewPtr->text.selection.isRectangular);
	
	highlightCurrentSelection(inTerminalViewPtr, false/* is highlighted */, true/* redraw */);
	
	// if nothing is selected, specify initial anchor location and extension behavior
	if (false == selectionExists(inTerminalViewPtr))
	{
		Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		// find the cursor location, anchor the selection there
		getCursorLocationError = Terminal_CursorGetLocation(inTerminalViewPtr->screen.ref, &cursorX, &cursorY);
		if (kTerminal_ResultOK != getCursorLocationError)
		{
			Console_Warning(Console_WriteValue, "failed to locate terminal cursor, error", (int)getCursorLocationError);
		}
		else
		{
			// IMPORTANT: these are not valid selection ranges, they only
			// set the anchor location for +/- in the code that follows
			// (in particular, selection is empty if start and past-the-end
			// are the same; it needs a +/- to turn into a valid range)
			if ((cursorX > kMinColumn) && (cursorX <= kMaxColumn))
			{
				inTerminalViewPtr->text.selection.range.first = std::make_pair(cursorX, cursorY);
				inTerminalViewPtr->text.selection.range.second = std::make_pair(cursorX, cursorY + 1); // past-the-end row
			}
			else
			{
				inTerminalViewPtr->text.selection.range.first = std::make_pair(kMinColumn, cursorY);
				inTerminalViewPtr->text.selection.range.second = std::make_pair(kMinColumn, cursorY + 1); // past-the-end row
			}
			
			if ((inDeltaColumn < 0) || (inDeltaRow < 0))
			{
				inTerminalViewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeBeginning;
			}
			else
			{
				inTerminalViewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeEnd;
			}
		}
	}
	
	// change highlighting
	if (kMy_SelectionExtensionTypeLineStart == inExtensionType)
	{
		inTerminalViewPtr->text.selection.range.first.first = kMinColumn;
	}
	else if (kMy_SelectionExtensionTypeLineEnd == inExtensionType)
	{
		// TEMPORARY; this might actually become “last defined cell” (e.g. non-whitespace), not “last column”
		inTerminalViewPtr->text.selection.range.second.first = (1 + kMaxColumn);
	}
	else
	{
		SInt16		totalColumnDelta = inDeltaColumn;
		SInt16		totalRowDelta = inDeltaRow;
		
		
		if (kMy_SelectionModeChangeBeginning == inTerminalViewPtr->text.selection.keyboardMode)
		{
			// grow or shrink from the beginning of the range instead of the end
			// NOTE: the following currently does not check rectangular-mode selections,
			// it only constructs selection ranges that have regular anchor points
			auto&			changingCellRef = inTerminalViewPtr->text.selection.range.first; // inclusive, start of selection
			SInt16 const	kProposedColumn = (STATIC_CAST(changingCellRef.first, SInt16) + totalColumnDelta);
			
			
			if (totalColumnDelta < 0)
			{
				if (kProposedColumn < kMinColumn)
				{
					// move to previous line
					// TEMPORARY; this might actually become “last defined cell” (e.g. non-whitespace), not “last column”
					changingCellRef.first = kMaxColumn; // change column here; row is changed below
					--totalRowDelta;
				}
				else
				{
					changingCellRef.first = kProposedColumn;
				}
			}
			else if (totalColumnDelta > 0)
			{
				if (kProposedColumn > kMaxColumn)
				{
					// move to next line
					changingCellRef.first = kMinColumn; // change column here; row is changed below
					++totalRowDelta;
				}
				else
				{
					changingCellRef.first = kProposedColumn;
				}
			}
			
			if (totalRowDelta < 0)
			{
				SInt64 const	kMinRow = -STATIC_CAST(Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref), SInt64);
				SInt64 const	kProposedRow = (changingCellRef.second + totalRowDelta);
				
				
				if (kProposedRow >= kMinRow)
				{
					changingCellRef.second = kProposedRow;
					didChangeRows = true;
				}
				else
				{
					// cannot go up further but fill in entire line
					changingCellRef.first = kMinColumn;
				}
			}
			else if (totalRowDelta > 0)
			{
				UInt16 const	kMaxRow = (Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref) - 1);
				SInt64 const	kProposedRow = (changingCellRef.second + totalRowDelta);
				
				
				if (kProposedRow <= kMaxRow)
				{
					changingCellRef.second = kProposedRow;
					didChangeRows = true;
				}
				else
				{
					// cannot go down further
					Console_Warning(Console_WriteLine, "unable to extend start of selection past the bottom");
				}
			}
		}
		else
		{
			// grow or shrink from past-the-end of the range
			// NOTE: the following currently does not check rectangular-mode selections,
			// it only constructs selection ranges that have regular anchor points
			auto&			changingCellRef = inTerminalViewPtr->text.selection.range.second; // exclusive, past-the-end of selection
			SInt16 const	kProposedPastEndColumn = (STATIC_CAST(changingCellRef.first, SInt16) + totalColumnDelta);
			
			
			if (totalColumnDelta < 0)
			{
				if (kProposedPastEndColumn < kMinColumn)
				{
					// move to previous line
					// TEMPORARY; this might actually become “last defined cell” (e.g. non-whitespace), not “last column”
					changingCellRef.first = (1 + kMaxColumn); // change column here; row is changed below
					--totalRowDelta;
				}
				else
				{
					changingCellRef.first = kProposedPastEndColumn;
				}
			}
			else if (totalColumnDelta > 0)
			{
				if (kProposedPastEndColumn > (1 + kMaxColumn))
				{
					// move to next line
					changingCellRef.first = (1 + kMinColumn); // change column here; row is changed below
					++totalRowDelta;
				}
				else
				{
					changingCellRef.first = kProposedPastEndColumn;
				}
			}
			
			if (totalRowDelta < 0)
			{
				SInt64 const	kMinRow = -STATIC_CAST(Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref), SInt64);
				SInt64 const	kProposedPastEndRow = (changingCellRef.second + totalRowDelta);
				
				
				if (kProposedPastEndRow >= kMinRow)
				{
					changingCellRef.second = kProposedPastEndRow;
					didChangeRows = true;
				}
				else
				{
					// cannot go up further
					Console_Warning(Console_WriteLine, "unable to extend end of selection past the top");
				}
			}
			else if (totalRowDelta > 0)
			{
				UInt16 const	kMaxRow = (Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref) - 1);
				SInt64 const	kProposedPastEndRow = (changingCellRef.second + totalRowDelta);
				
				
				if (kProposedPastEndRow <= (1 + kMaxRow))
				{
					changingCellRef.second = kProposedPastEndRow;
					didChangeRows = true;
				}
				else
				{
					// cannot go down further but fill in entire line
					changingCellRef.first = (1 + kMaxColumn);
				}
			}
		}
	}
	
	// alterations above have a small chance of inversions; restore ordering
	sortAnchors(inTerminalViewPtr->text.selection.range.first, inTerminalViewPtr->text.selection.range.second,
				inTerminalViewPtr->text.selection.isRectangular);
	
	highlightCurrentSelection(inTerminalViewPtr, true/* is highlighted */, true/* redraw */);
	copySelectedTextIfUserPreference(inTerminalViewPtr);
	
	if ((didChangeRows) && (inAllowScrolling))
	{
		// UNIMPLEMENTED: "inAllowScrolling" should allow auto-scrolling
		// of the view if the new selection region is outside the view
	}
}// selectionModify


/*!
Copies a color into the blink animation palette for the
specified (zero-based) animation stage.  The animation timer
will use this palette to find colors as needed.

(4.0)
*/
inline void
setBlinkAnimationColor	(My_TerminalViewPtr			inTerminalViewPtr,
						 UInt16						inAnimationStage,
						 CGFloatRGBColor const*		inColorPtr)
{
	assert(inAnimationStage < STATIC_CAST(inTerminalViewPtr->blinkColors.size(), UInt16));
	inTerminalViewPtr->blinkColors[inAnimationStage] = *inColorPtr;
}// setBlinkAnimationColor


/*!
Starts or stops a timer that periodically refreshes
blinking text.  Since this is potentially expensive,
this routine exists so that the timer is only running
when it is actually needed (that is, when blinking
text actually exists in the terminal view).

(3.1)
*/
void
setBlinkingTimerActive	(My_TerminalViewPtr		inTerminalViewPtr,
						 Boolean				inIsActive)
{
#if BLINK_MEANS_BLINK
	TerminalViewRef		blockViewRef = inTerminalViewPtr->selfRef;
	
	
	if (inTerminalViewPtr->animation.timer.isActive != inIsActive)
	{
		if (inIsActive)
		{
			// note: timer interval is modified continuously by animateBlinkingItems()
			inTerminalViewPtr->animation.timer.objectRef = [NSTimer scheduledTimerWithTimeInterval:(1 / 1000.0/* milliseconds per second */)/* in seconds */
			repeats:YES
			block:^(NSTimer* timer)
			{
				// note: this block can be invoked at arbitrary times, the view
				// reference must be revalidated in animateBlinkingItems()
				animateBlinkingItems(timer, blockViewRef);
			}];
		}
		else
		{
			if (nil != inTerminalViewPtr->animation.timer.objectRef)
			{
				[inTerminalViewPtr->animation.timer.objectRef invalidate];
				inTerminalViewPtr->animation.timer.objectRef = nil;
			}
		}
		inTerminalViewPtr->animation.timer.isActive = inIsActive;
	}
#endif
}// setBlinkingTimerActive


/*!
Hides or shows the cursor of the specified screen.

This has no effect if TerminalView_SetCursorRenderingEnabled()
has turned off cursor rendering.

(3.0)
*/
void
setCursorVisibility		(My_TerminalViewPtr		inTerminalViewPtr,
						 Boolean				inIsVisible)
{
	if (false == inTerminalViewPtr->screen.cursor.inhibited)
	{
		MyCursorState	newCursorState = (inIsVisible) ? kMyCursorStateVisible : kMyCursorStateInvisible;
		Boolean			renderCursor = (inTerminalViewPtr->screen.cursor.currentState != newCursorState);
		
		
		// change state
		inTerminalViewPtr->screen.cursor.currentState = newCursorState;
		
		// redraw the cursor if necessary
		if (renderCursor)
		{
			updateDisplayInShape(inTerminalViewPtr, inTerminalViewPtr->screen.cursor.updatedShape);
		}
	}
}// setCursorVisibility


/*!
Sets the font and size for the specified screen’s text.
Pass nullptr if the font name should be ignored (and
therefore not changed).  Pass 0 if the font size should
not be changed.

This routine changes the font size regardless of the
display mode (it is probably used to implement the
automatic font sizing of the zoom display mode!).

The screen is not redrawn.

(3.0)
*/
void
setFontAndSize		(My_TerminalViewPtr		inTerminalViewPtr,
					 CFStringRef			inFontFamilyNameOrNull,
					 CGFloat				inFontSizeOrZero,
					 Float32				inCharacterWidthScalingOrZero,
					 Boolean				inNotifyListeners)
{
	if (inTerminalViewPtr->isCocoa())
	{
		NSFontManager*		fontManager = [NSFontManager sharedFontManager];
		NSString*			sourceFontName = ((nullptr == inFontFamilyNameOrNull)
												? inTerminalViewPtr->text.font.normalFont.fontName
												: BRIDGE_CAST(inFontFamilyNameOrNull, NSString*));
		
		
		// release any previous fonts
		if (nil != inTerminalViewPtr->text.font.normalFont)
		{
			inTerminalViewPtr->text.font.normalFont = nil;
		}
		if (nil != inTerminalViewPtr->text.font.boldFont)
		{
			inTerminalViewPtr->text.font.boldFont = nil;
		}
		
		// find a font for most text
		inTerminalViewPtr->text.font.normalFont = [NSFont fontWithName:sourceFontName size:inFontSizeOrZero];
		
		// find a font for boldface text
		inTerminalViewPtr->text.font.boldFont = [fontManager convertFont:inTerminalViewPtr->text.font.normalFont
																			toHaveTrait:NSBoldFontMask];
		if ((nil == inTerminalViewPtr->text.font.boldFont) ||
			(NSBoldFontMask != ([fontManager traitsOfFont:inTerminalViewPtr->text.font.boldFont] & NSBoldFontMask)))
		{
			// if no dedicated bold font is available, try a higher-weight version of the original
			inTerminalViewPtr->text.font.boldFont =
				[fontManager fontWithFamily:[inTerminalViewPtr->text.font.normalFont familyName]
											traits:0 weight:9/* as documented, for bold */
											size:inFontSizeOrZero];
		}
		
		// in case the font is somehow not resolved at all after
		// attempts to transform it, bail and use the original
		if (nil == inTerminalViewPtr->text.font.boldFont)
		{
			inTerminalViewPtr->text.font.boldFont = inTerminalViewPtr->text.font.normalFont;
		}
	}
	
	if (inFontFamilyNameOrNull != nullptr)
	{
		// remember font selection
		inTerminalViewPtr->text.font.familyName.setWithRetain(inFontFamilyNameOrNull);
	}
	
	if (inFontSizeOrZero > 0)
	{
		inTerminalViewPtr->text.font.normalMetrics.size = inFontSizeOrZero;
	}
	
	if (inCharacterWidthScalingOrZero > 0)
	{
		inTerminalViewPtr->text.font.scaleWidthPerCell = inCharacterWidthScalingOrZero;
	}
	
	// set the font metrics (including double size)
	setUpScreenFontMetrics(inTerminalViewPtr);
	
	// recalculate cursor boundaries for the specified view
	{
		Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		getCursorLocationError = Terminal_CursorGetLocation(inTerminalViewPtr->screen.ref, &cursorX, &cursorY);
		if (kTerminal_ResultOK != getCursorLocationError)
		{
			Console_Warning(Console_WriteValue, "failed to adjust cursor location for new font and size, error", getCursorLocationError);
		}
		else
		{
			setUpCursorBounds(inTerminalViewPtr, cursorX, cursorY, &inTerminalViewPtr->screen.cursor.bounds,
								inTerminalViewPtr->screen.cursor.updatedShape);
		}
	}
	
	// resize window to preserve its dimensions in character cell units
	recalculateCachedDimensions(inTerminalViewPtr);
	
	if (inNotifyListeners)
	{
		// notify listeners that the size has changed
		eventNotifyForView(inTerminalViewPtr, kTerminalView_EventFontSizeChanged,
							inTerminalViewPtr->selfRef/* context */);
	}
}// setFontAndSize


/*!
Changes a color the user perceives is in use; these are
stored internally in the view data structure.  Also
updates the palette automatically, so you don’t have to
call setScreenCustomColor() too.

(3.0)
*/
inline void
setScreenBaseColor	(My_TerminalViewPtr			inTerminalViewPtr,
					 TerminalView_ColorIndex	inColorEntryNumber,
					 CGFloatRGBColor const*		inColorPtr)
{
	switch (inColorEntryNumber)
	{
	case kTerminalView_ColorIndexNormalBackground:
		{
			inTerminalViewPtr->text.colors[kMyBasicColorIndexNormalBackground] = *inColorPtr;
			
			// the view reads its color from the associated data structure automatically, so just redraw
			inTerminalViewPtr->encompassingNSView.terminalPaddingViewBottom.needsDisplay = YES;
			inTerminalViewPtr->encompassingNSView.terminalPaddingViewLeft.needsDisplay = YES;
			inTerminalViewPtr->encompassingNSView.terminalPaddingViewRight.needsDisplay = YES;
			inTerminalViewPtr->encompassingNSView.terminalPaddingViewTop.needsDisplay = YES;
		}
		break;
	
	case kTerminalView_ColorIndexBoldText:
		inTerminalViewPtr->text.colors[kMyBasicColorIndexBoldText] = *inColorPtr;
		break;
	
	case kTerminalView_ColorIndexBoldBackground:
		inTerminalViewPtr->text.colors[kMyBasicColorIndexBoldBackground] = *inColorPtr;
		break;
	
	case kTerminalView_ColorIndexBlinkingText:
		inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingText] = *inColorPtr;
		break;
	
	case kTerminalView_ColorIndexBlinkingBackground:
		inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingBackground] = *inColorPtr;
		break;
	
	case kTerminalView_ColorIndexMatteBackground:
		{
			inTerminalViewPtr->text.colors[kMyBasicColorIndexMatteBackground] = *inColorPtr;
			
			// the view reads its color from the associated data structure automatically, so just redraw
			inTerminalViewPtr->encompassingNSView.terminalMarginViewBottom.needsDisplay = YES;
			inTerminalViewPtr->encompassingNSView.terminalMarginViewLeft.needsDisplay = YES;
			inTerminalViewPtr->encompassingNSView.terminalMarginViewRight.needsDisplay = YES;
			inTerminalViewPtr->encompassingNSView.terminalMarginViewTop.needsDisplay = YES;
		}
		break;
	
	case kTerminalView_ColorIndexNormalText:
	default:
		inTerminalViewPtr->text.colors[kMyBasicColorIndexNormalText] = *inColorPtr;
		break;
	}
	
	// recalculate the blinking colors if they are changing
	if ((inColorEntryNumber == kTerminalView_ColorIndexBlinkingText) ||
		(inColorEntryNumber == kTerminalView_ColorIndexBlinkingBackground))
	{
		CGFloatRGBColor		colorValue;
		
		
		// create enough intermediate colors to make a reasonably
		// smooth “pulsing” effect as text blinks; the last color
		// in the sequence matches the base foreground color, just
		// for simplicity in the animation code
		colorValue = inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingText];
		for (SInt16 i = kMy_BlinkingColorCount - 1; i >= 0; --i)
		{
			colorValue = CocoaBasic_GetGray(inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingBackground],
											inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingText],
											kBlendingByPhase[i]/* arbitrary; determines the animation style */);
			setBlinkAnimationColor(inTerminalViewPtr, i, &colorValue);
		}
	}
	
	setScreenCustomColor(inTerminalViewPtr, inColorEntryNumber, inColorPtr);
}// setScreenBaseColor


/*!
Copies a color into a screen window’s color palette.

Most core colors are not meant to be customized, but
the base 16 (ANSI) colors typically are.

(4.0)
*/
inline void
setScreenCoreColor	(My_TerminalViewPtr			inTerminalViewPtr,
					 UInt16						inColorEntryNumber,
					 CGFloatRGBColor const*		inColorPtr)
{
	inTerminalViewPtr->coreColors[inColorEntryNumber] = *inColorPtr;
}// setScreenCoreColor


/*!
Copies a color into a screen window’s palette, given
information on which color it should replace.

Do not use this routine to change the colors a user
perceives are in use; those are stored internally in
the view data structure.  This routine modifies the
window palette, which defines the absolutely current
rendering colors.

(3.0)
*/
inline void
setScreenCustomColor	(My_TerminalViewPtr			inTerminalViewPtr,
						 TerminalView_ColorIndex	inColorEntryNumber,
						 CGFloatRGBColor const*		inColorPtr)
{
	assert(inColorEntryNumber < STATIC_CAST(inTerminalViewPtr->customColors.size(), TerminalView_ColorIndex));
	inTerminalViewPtr->customColors[inColorEntryNumber] = *inColorPtr;
}// setScreenCustomColor


/*!
Updates a dictionary suitable for use in attributed strings
that contains the specified terminal text attributes.

Currently this can set the following attribute keys:
- NSFontAttributeName
- NSForegroundColorAttributeName
- NSUnderlineStyleAttributeName

If "inSuppressKerning" is set to false, the character width
is also adjusted by setting:
- NSKernAttributeName

(4.0)
*/
void
setTextAttributesDictionary		(My_TerminalViewPtr			inTerminalViewPtr,
								 NSMutableDictionary*		inoutDictionary,
								 TextAttributes_Object		inAttributes,
								 Boolean					inSuppressKerning)
{
	//
	// IMPORTANT: since the output might be a persistent dictionary,
	// set values for ALL keys whether or not they “need” values
	//
	
	// set font attributes
	{
		NSFontManager*	fontManager = [NSFontManager sharedFontManager];
		NSFont*			originalFont = inTerminalViewPtr->text.font.normalFont;
		NSFont*			sourceFont = originalFont;
		
		
		if ((inAttributes.hasBold() || inAttributes.hasSearchHighlight()) &&
			(nil != inTerminalViewPtr->text.font.boldFont))
		{
			sourceFont = inTerminalViewPtr->text.font.boldFont;
		}
		
		if (inAttributes.hasItalic())
		{
			sourceFont = [fontManager convertFont:sourceFont toHaveTrait:NSItalicFontMask];
			if ((nil == sourceFont) ||
				(NSItalicFontMask != ([fontManager traitsOfFont:sourceFont] & NSItalicFontMask)))
			{
				// if no dedicated italic font is available, apply a transform to make
				// the current font appear slanted (note: this is what WebKit does)
				NSAffineTransform*			fontTransform = [NSAffineTransform transform];
				NSAffineTransform*			italicTransform = [NSAffineTransform transform];
				NSAffineTransformStruct		slantTransformData;
				
				
				std::memset(&slantTransformData, 0, sizeof(slantTransformData));
				slantTransformData.m11 = 1.0;
				slantTransformData.m12 = 0.0;
				slantTransformData.m21 = -tanf(-14.0f/* rotation in degrees */ * acosf(0) / 90.0f);
				slantTransformData.m22 = 1.0;
				slantTransformData.tX  = 0.0;
				slantTransformData.tY  = 0.0;
				[italicTransform setTransformStruct:slantTransformData];
				
				[fontTransform scaleBy:[originalFont pointSize]];
				[fontTransform appendTransform:italicTransform];
				
				sourceFont = [NSFont fontWithDescriptor:[originalFont fontDescriptor] textTransform:fontTransform];
			}
		}
		
		// in case the font is somehow not resolved at all after
		// attempts to transform it, bail and use the original
		if (nil == sourceFont)
		{
			sourceFont = originalFont;
		}
		
		if (nil != sourceFont)
		{
			[inoutDictionary setObject:sourceFont forKey:NSFontAttributeName];
		}
		
		// kerning only applies when setting attributes for drawing; if this
		// function is being called in order to measure for layout, kerning
		// should be disabled because "baseWidthPerCell" will NOT be valid
		if ((false == inSuppressKerning) && (1.0 != inTerminalViewPtr->text.font.scaleWidthPerCell))
		{
			[inoutDictionary setObject:[NSNumber numberWithDouble:((inTerminalViewPtr->text.font.baseWidthPerCell.precisePixels() * inTerminalViewPtr->text.font.scaleWidthPerCell) -
																	inTerminalViewPtr->text.font.baseWidthPerCell.precisePixels())]
										forKey:NSKernAttributeName];
		}
		else
		{
			[inoutDictionary removeObjectForKey:NSKernAttributeName];
		}
		
		if (inAttributes.hasUnderline() || inAttributes.hasSearchHighlight())
		{
			[inoutDictionary setObject:@(1) forKey:NSUnderlineStyleAttributeName];
		}
		else
		{
			[inoutDictionary setObject:@(0) forKey:NSUnderlineStyleAttributeName];
		}
	}
	
	// set color attributes
	{
		NSColor*	foregroundNSColor = (inTerminalViewPtr->screen.currentRenderDragColors)
										? [NSColor blackColor] // default for drags is black
										: nil;
		
		
		if (nil == foregroundNSColor)
		{
			CGFloatRGBColor		backgroundDeviceColor;
			CGFloatRGBColor		foregroundDeviceColor;
			
			
			// find the correct colors in the color table
			getScreenColorsForAttributes(inTerminalViewPtr, inAttributes,
											&foregroundDeviceColor, &backgroundDeviceColor,
											&inTerminalViewPtr->screen.currentRenderNoBackground);
			foregroundNSColor = [NSColor colorWithCalibratedRed:foregroundDeviceColor.red
																green:foregroundDeviceColor.green
																blue:foregroundDeviceColor.blue
																alpha:1.0];
			NSColor*	backgroundNSColor = [NSColor colorWithCalibratedRed:backgroundDeviceColor.red
																			green:backgroundDeviceColor.green
																			blue:backgroundDeviceColor.blue
																			alpha:1.0];
			
			
			if (inAttributes.hasSearchHighlight())
			{
				// use selection colors
				NSColor*	searchResultTextColor = [foregroundNSColor copy];
				NSColor*	searchResultBackgroundColor = [backgroundNSColor copy];
				
				
				if (NO == [NSColor searchResultColorsForForeground:&searchResultTextColor
																	background:&searchResultBackgroundColor])
				{
					foregroundNSColor = [foregroundNSColor copy];
				}
				else
				{
					foregroundNSColor = searchResultTextColor;
				}
			}
			else if (inAttributes.hasSelection() && inTerminalViewPtr->isActive)
			{
				if (gPreferenceProxies.invertSelections)
				{
					// invert the text (foreground from background)
					foregroundNSColor = backgroundNSColor;
				}
				else
				{
					// alter the color (usually to make it look darker)
					NSColor*	selectionTextColor = [foregroundNSColor copy];
					NSColor*	selectionBackgroundColor = [backgroundNSColor copy];
					
					
					if (NO == [NSColor selectionColorsForForeground:&selectionTextColor
																	background:&selectionBackgroundColor])
					{
						// bail; force the default color even if it won’t look as good
						selectionTextColor = [NSColor blackColor];
					}
					foregroundNSColor = [selectionTextColor
											colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
				}
			}
			else
			{
				// use default value (set above)
			}
			
			//
			// modify the base text color based on other state
			//
			
			// adjust for inactive views (dimmer text)
			if (false == inTerminalViewPtr->isActive)
			{
				// make the text color lighter, unless it is selected
				if (false == inAttributes.hasSelection())
				{
					foregroundNSColor = [foregroundNSColor blendedColorWithFraction:0.5/* arbitrary */
																					ofColor:[NSColor whiteColor]];
				}
			}
		}
		
		// UNIMPLEMENTED: highlighted-text (possibly inverted) colors
		// UNIMPLEMENTED: dimmed-screen colors
		// UNIMPLEMENTED: “concealed” style colors
		
		[inoutDictionary setObject:foregroundNSColor forKey:NSForegroundColorAttributeName];
	}
}// setTextAttributesDictionary


/*!
Uses the current cursor shape preference and font metrics
of the given screen to determine the view-relative boundaries
of the terminal cursor if it were located at the specified
row and column in the terminal screen.

It is usually a good idea to also precalculate an equivalent
update shape for the rectangle; if so, pass a valid mutable
shape for "inoutUpdatedShapeOrNull".		

(3.0)
*/
void
setUpCursorBounds	(My_TerminalViewPtr		inTerminalViewPtr,
					 SInt16					inX,
					 SInt16					inY,
					 CGRect*				outBoundsPtr,
					 HIMutableShapeRef		inoutUpdatedShapeOrNull)
{
	enum
	{
		// cursor dimensions, in pixels
		// (NOTE: these currently hack in Quartz/QuickDraw conversion factors
		// and will eventually change)
		kTerminalCursorUnderscoreHeight = 1,
		kTerminalCursorThickUnderscoreHeight = 2,
		kTerminalCursorVerticalLineWidth = 1,
		kTerminalCursorThickVerticalLineWidth = 2
	};
	
	CGSize					characterSizeInPixels; // based on font metrics
	CGRect					rowBounds;
	CGFloat					thickness = 0; // used for non-block-shaped cursors
	Terminal_CursorType		terminalCursorType = cursorType(inTerminalViewPtr);
	
	
	getRowBounds(inTerminalViewPtr, inY, rowBounds);
	
	characterSizeInPixels.width = getRowCharacterWidth(inTerminalViewPtr, inY).precisePixels();
	characterSizeInPixels.height = rowBounds.size.height;
	
	outBoundsPtr->origin.x = STATIC_CAST(inX, CGFloat) * characterSizeInPixels.width;
	outBoundsPtr->origin.y = rowBounds.origin.y;
	
	switch (terminalCursorType)
	{
	case kTerminal_CursorTypeUnderscore:
	case kTerminal_CursorTypeThickUnderscore:
		thickness = (terminalCursorType == kTerminal_CursorTypeUnderscore)
						? kTerminalCursorUnderscoreHeight
						: kTerminalCursorThickUnderscoreHeight;
		outBoundsPtr->origin.y += (characterSizeInPixels.height - thickness);
		outBoundsPtr->size.width = characterSizeInPixels.width;	
		outBoundsPtr->size.height = thickness;
		break;
	
	case kTerminal_CursorTypeVerticalLine:
	case kTerminal_CursorTypeThickVerticalLine:
		thickness = (terminalCursorType == kTerminal_CursorTypeVerticalLine)
						? kTerminalCursorVerticalLineWidth
						: kTerminalCursorThickVerticalLineWidth;
		outBoundsPtr->size.width = thickness;
		outBoundsPtr->size.height = characterSizeInPixels.height;
		break;
	
	case kTerminal_CursorTypeBlock:
	default:
		outBoundsPtr->size.width = characterSizeInPixels.width;
		outBoundsPtr->size.height = characterSizeInPixels.height;
		break;
	}
	
	if (nullptr != inoutUpdatedShapeOrNull)
	{
		CGRect		updatedBounds;
		
		
		// specify the update region for the cursor (always full-size because
		// it may need to erase a previous cursor when changing cursor shape)
		updatedBounds.origin.x = (outBoundsPtr->origin.x);
		updatedBounds.origin.y = (rowBounds.origin.y);
		updatedBounds.size.width = (characterSizeInPixels.width);
		updatedBounds.size.height = (characterSizeInPixels.height);
		updatedBounds = CGRectIntegral(updatedBounds);
		UNUSED_RETURN(OSStatus)HIShapeSetEmpty(inoutUpdatedShapeOrNull);
		UNUSED_RETURN(OSStatus)HIShapeUnionWithRect(inoutUpdatedShapeOrNull, &updatedBounds);
	}
}// setUpCursorBounds


/*!
Sets the font ascent, height, width, and monospaced
attributes of the font information for the specified
screen.  The font information is determined by looking
at the current settings for the specified port.

This routine corrects a long-standing bug where MacTerm
would incorrectly recalculate the font width attribute
based on the current font of the specified port, not the
default font of the specified screen.  On a terminal
screen, every cell must be the same width, so all
rendered fonts must use the same width, regardless of
the width that they might otherwise require.  Thus, as
of version 3.0, font metrics ignore the current font
of the screen (e.g. regardless of whether boldface text
is currently being drawn) and use the default font.

(3.0)
*/
void
setUpScreenFontMetrics	(My_TerminalViewPtr		inTerminalViewPtr)
{
	NSFont* const	sourceFont = [inTerminalViewPtr->text.font.normalFont screenFont];
	
	
	// INCOMPLETE; could offer user preferences for influencing line height,
	// e.g. perhaps to ignore leading for lines that are closer together
#if 0
	// alternate version that trusts the font’s base settings (note that
	// the "descender" is negative so it is negated when finding range)
	inTerminalViewPtr->text.font.normalMetrics.baseLine = (-sourceFont.descender + [sourceFont leading] / 2.0f);
	inTerminalViewPtr->text.font.heightPerCell.setPrecisePixels(sourceFont.ascender + -sourceFont.descender);
#else
	// since Core Text is used for rendering most text, use the
	// same API to request rendering-based measurements
	NSMutableDictionary*	attributeDict = [[NSMutableDictionary alloc] init];
	setTextAttributesDictionary(inTerminalViewPtr, attributeDict, TextAttributes_Object(), true/* suppress kerning */);
	{
		NSAttributedString*		attributedString = [[NSAttributedString alloc]
													initWithString:@"Âgp"/* arbitrary; test string for measurement */
																	attributes:attributeDict];
		CFRetainRelease			lineObject(CTLineCreateWithAttributedString
											(BRIDGE_CAST(attributedString, CFAttributedStringRef)),
											CFRetainRelease::kAlreadyRetained);
		CTLineRef				asLineRef = REINTERPRET_CAST(lineObject.returnCFTypeRef(), CTLineRef);
		CGFloat					ascentMeasurement = 0;
		CGFloat					descentMeasurement = 0;
		CGFloat					leadingMeasurement = 0;
		Float64					lineWidth = 0;
		
		
		lineWidth = CTLineGetTypographicBounds(asLineRef, &ascentMeasurement, &descentMeasurement, &leadingMeasurement);
		inTerminalViewPtr->text.font.normalMetrics.baseLine = (descentMeasurement + (leadingMeasurement / 2.0f));
		inTerminalViewPtr->text.font.heightPerCell.setPrecisePixels(ascentMeasurement + descentMeasurement + leadingMeasurement);
	}
	attributeDict = nil;
#endif
	
#if 0
	// note: monospace is not reliable; the resulting advancement is slightly
	// different (or even a lot different) depending on the font, versus the
	// value obtained by consistently measuring a glyph (see below)
	if (YES == [sourceFont isFixedPitch])
	{
		inTerminalViewPtr->text.font.baseWidthPerCell.setPrecisePixels([sourceFont maximumAdvancement].width);
	}
	else
#else
	{
		// since NSFont "advancementForGlyph" is deprecated, find CGGlyph instead
		CFRetainRelease		fontObject(CTFontCreateWithName(BRIDGE_CAST(sourceFont.fontName, CFStringRef), sourceFont.pointSize, nullptr/* transform */),
										CFRetainRelease::kAlreadyRetained);
		CTFontRef			asFontRef = REINTERPRET_CAST(fontObject.returnCFTypeRef(), CTFontRef);
		CGGlyph				letterGlyph = CTFontGetGlyphWithName(asFontRef, CFSTR("A"));
		
		
		//inTerminalViewPtr->text.font.baseWidthPerCell.setPrecisePixels([sourceFont advancementForGlyph:[sourceFont glyphWithName:@"A"]].width);
		inTerminalViewPtr->text.font.baseWidthPerCell.setPrecisePixels(CTFontGetAdvancesForGlyphs(asFontRef, kCTFontOrientationHorizontal, &letterGlyph,
																									nullptr/* array of advances */, 1/* count */));
	}
#endif
	
	// scale the font width according to user preferences (this should be
	// consistent with kerning in setTextAttributesDictionary(), which
	// is suppressed in the call above to find a proper base width);
	// note that double-size text no longer has its own font metrics
	// because Core Graphics is really good at producing nice-looking
	// text simply by applying transforms (unlike QuickDraw previously)
	{
		CGFloat		reduction = inTerminalViewPtr->text.font.baseWidthPerCell.precisePixels();
		
		
		reduction *= inTerminalViewPtr->text.font.scaleWidthPerCell;
		inTerminalViewPtr->text.font.widthPerCell.setPrecisePixels(reduction);
	}
	
	// the thickness of lines in certain glyphs is also scaled with the font size
	inTerminalViewPtr->text.font.thicknessHorizontalLines = std::max< CGFloat >(1.0f, inTerminalViewPtr->text.font.widthPerCell.precisePixels() / 5.0f); // arbitrary
	inTerminalViewPtr->text.font.thicknessHorizontalBold = 2.0f * inTerminalViewPtr->text.font.thicknessHorizontalLines; // arbitrary
	inTerminalViewPtr->text.font.thicknessVerticalLines = std::max< CGFloat >(1.0f, inTerminalViewPtr->text.font.heightPerCell.precisePixels() / 7.0f); // arbitrary
	inTerminalViewPtr->text.font.thicknessVerticalBold = 2.0f * inTerminalViewPtr->text.font.thicknessVerticalLines; // arbitrary
}// setUpScreenFontMetrics
	

/*!
Ensures that the first point given is “sooner” in a
left-to-right localization than the 2nd point.  In
other words, the two points are swapped if necessary
to meet this condition.

Specifically, if the two points are in the same row,
the first must be to the left of the second; but if
they are on different rows, then the first must be
on an earlier row than the second and then the left-
to-right positioning is immaterial.

If "inRectangular" is true, then the points may be
*redefined* so that the first point is the top-left
corner of the rectangle that the two points form,
and so the second point is the bottom-right corner
of the rectangle.

LOCALIZE THIS

(3.0)
*/
void
sortAnchors		(TerminalView_Cell&		inoutPoint1,
				 TerminalView_Cell&		inoutPoint2,
				 Boolean				inRectangular)
{
	TerminalView_Cell		newStart = inoutPoint1;
	TerminalView_Cell		newPastEnd = inoutPoint2;
	
	
	if (INTEGER_ABSOLUTE(newPastEnd.second - newStart.second) <= 1)
	{
		// empty range or one-line range; require columns to be in order
		if (newStart.first > newPastEnd.first)
		{
			if (newPastEnd.first > 0)
			{
				--newPastEnd.first; // convert to inclusive range for swap
			}
			--newPastEnd.second; // convert to inclusive range for swap
			std::swap(newStart.first, newPastEnd.first);
			++newPastEnd.first; // convert to past-the-end range
			++newPastEnd.second; // convert to past-the-end range
		}
	}
	else
	{
		// normal range
		if (inRectangular)
		{
			// in rectangular case, anchors are not simply swapped; the two
			// coordinates of each corner may also change
			auto	lowestStart = std::min< UInt16 >(inoutPoint1.first, inoutPoint2.first);
			auto	lowestPastEnd = std::min< TerminalView_RowIndex >(inoutPoint1.second, inoutPoint2.second);
			
			
			newStart.first = lowestStart;
			newStart.second = lowestPastEnd;
			newPastEnd.first = lowestStart + INTEGER_ABSOLUTE(inoutPoint1.first - inoutPoint2.first);
			newPastEnd.second = lowestPastEnd + INTEGER_ABSOLUTE(inoutPoint1.second - inoutPoint2.second);
		}
		else
		{
			if (newStart.second > newPastEnd.second)
			{
				// require point on earlier line to come first
				if (newPastEnd.first > 0)
				{
					--newPastEnd.first; // convert to inclusive range for swap
				}
				--newPastEnd.second; // convert to inclusive range for swap
				std::swap(newStart, newPastEnd);
				++newPastEnd.first; // convert to past-the-end range
				++newPastEnd.second; // convert to past-the-end range
			}
		}
	}
	
	inoutPoint1 = newStart;
	inoutPoint2 = newPastEnd;
}// sortAnchors


/*!
Starts monitoring a screen buffer for key state changes
so that the view remains up-to-date.  Returns true only
if successful.  The screen buffer must have previously
been added to the view with addDataSource().

See also stopMonitoringDataSource().

(4.1)
*/
Boolean
startMonitoringDataSource	(My_TerminalViewPtr		inTerminalViewPtr,
							 TerminalScreenRef		inScreenDataSource)
{
	Boolean		result = false;
	
	
	// currently only one source is possible so the implementation
	// is more straightforward (IMPORTANT: the steps below should
	// be the inverse of the code in stopMonitoringDataSource())
	if (inScreenDataSource == inTerminalViewPtr->screen.ref)
	{
		TerminalScreenRef	screenRef = inScreenDataSource;
		
		
		// ask to be notified of terminal bells
		inTerminalViewPtr->screen.bellHandler.setWithNoRetain(ListenerModel_NewStandardListener
																(audioEvent, inTerminalViewPtr->selfRef/* context */));
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeAudioEvent, inTerminalViewPtr->screen.bellHandler.returnRef());
		
		// ask to be notified of video mode changes
		inTerminalViewPtr->screen.videoModeMonitor.setWithNoRetain(ListenerModel_NewStandardListener
																	(receiveVideoModeChange, inTerminalViewPtr->selfRef/* context */));
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeVideoMode, inTerminalViewPtr->screen.videoModeMonitor.returnRef());
		
		// ask to be notified of screen buffer content changes
		inTerminalViewPtr->screen.contentMonitor.setWithNoRetain(ListenerModel_NewStandardListener
																	(screenBufferChanged, inTerminalViewPtr->selfRef/* context */));
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeScreenSize, inTerminalViewPtr->screen.contentMonitor.returnRef());
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeTextEdited, inTerminalViewPtr->screen.contentMonitor.returnRef());
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeScrollActivity, inTerminalViewPtr->screen.contentMonitor.returnRef());
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeXTermColor, inTerminalViewPtr->screen.contentMonitor.returnRef());
		
		// ask to be notified of cursor changes
		inTerminalViewPtr->screen.cursorMonitor.setWithNoRetain(ListenerModel_NewStandardListener
																(screenCursorChanged, inTerminalViewPtr->selfRef/* context */));
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeCursorLocation, inTerminalViewPtr->screen.cursorMonitor.returnRef());
		Terminal_StartMonitoring(screenRef, kTerminal_ChangeCursorState, inTerminalViewPtr->screen.cursorMonitor.returnRef());
		
		result = true;
	}
	
	return result;
}// startMonitoringDataSource


/*!
Stops monitoring a screen buffer for key state changes
so the view will no longer reflect its content.  Returns
true only if successful.

See also startMonitoringDataSource().

(4.1)
*/
Boolean
stopMonitoringDataSource	(My_TerminalViewPtr		inTerminalViewPtr,
							 TerminalScreenRef		inScreenDataSourceOrNull)
{
	TerminalScreenRef	screenRef = inTerminalViewPtr->screen.ref;
	Boolean				result = false;
	
	
	if (nullptr != inScreenDataSourceOrNull)
	{
		if (inScreenDataSourceOrNull != screenRef)
		{
			screenRef = nullptr;
		}
	}
	
	// IMPORTANT: the steps below should be the inverse of the
	// code in startMonitoringDataSource()
	if (nullptr != screenRef)
	{
		// stop listening for terminal bells
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeAudioEvent, inTerminalViewPtr->screen.bellHandler.returnRef());
		
		// stop listening for video mode changes
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeVideoMode, inTerminalViewPtr->screen.videoModeMonitor.returnRef());
		
		// stop listening for screen buffer content changes
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeScreenSize, inTerminalViewPtr->screen.contentMonitor.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeTextEdited, inTerminalViewPtr->screen.contentMonitor.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeScrollActivity, inTerminalViewPtr->screen.contentMonitor.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeXTermColor, inTerminalViewPtr->screen.contentMonitor.returnRef());
		
		// stop listening for cursor changes
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeCursorLocation, inTerminalViewPtr->screen.cursorMonitor.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeCursorState, inTerminalViewPtr->screen.cursorMonitor.returnRef());
		
		result = true;
	}
	
	return result;
}// stopMonitoringDataSource


/*!
Arranges for the entire terminal screen to be redrawn at the
next opportunity.

Use of this utility routine is recommended because there are
nested views that combine to produce a terminal display, and
they must often be updated as a set.

(4.0)
*/
void
updateDisplay	(My_TerminalViewPtr		inTerminalViewPtr)
{
	inTerminalViewPtr->encompassingNSView.terminalContentView.needsDisplay = YES;
}// updateDisplay


/*!
Arranges for the specified portion of the terminal screen to be
redrawn at the next opportunity.  The region should be relative
to the main content view.

(2017.11)
*/
void
updateDisplayInShape	(My_TerminalViewPtr		inTerminalViewPtr,
						 HIShapeRef				inRegion)
{
	// if necessary for debugging, dump the shape of the region
	//UNUSED_RETURN(OSStatus)HIShapeEnumerate(inRegion, kHIShapeParseFromTopLeft, Console_WriteShapeElement, nullptr/* ref. con. */); // debug
	UNUSED_RETURN(OSStatus)HIShapeEnumerate(inRegion, kHIShapeParseFromTopLeft, updateDisplayInShapeSubRect, inTerminalViewPtr/* ref. con. */); // debug
}// updateDisplayInShape


/*!
A routine of standard "HIShapeEnumerateProcPtr" form, this
is a helper used by updateDisplayInShape() to add each
sub-rectangle to the view’s update region.

(2020.04)
*/
OSStatus
updateDisplayInShapeSubRect		(int			inMessage,
								 HIShapeRef		inShape,
								 CGRect const*	inRectPtr,
								 void*			inRefConTerminalViewPtr)
{
	My_TerminalViewPtr	terminalViewPtr = REINTERPRET_CAST(inRefConTerminalViewPtr, My_TerminalViewPtr);
	OSStatus			result = noErr;
	
	
	if (kHIShapeEnumerateInit == inMessage)
	{
		// nothing needed
	}
	else if (kHIShapeEnumerateTerminate == inMessage)
	{
		// nothing needed
	}
	else if (kHIShapeEnumerateRect == inMessage)
	{
		[terminalViewPtr->encompassingNSView.terminalContentView setNeedsDisplayInRect:NSRectFromCGRect(*inRectPtr)];
	}
	else
	{
		// ???
	}
	
	return result;
}// updateDisplayInShapeSubRect


/*!
Sets the background color and pattern settings (and ONLY those
settings) of the current QuickDraw port and the specified
Core Graphics port, based on the requested attributes and the
active state of the terminal’s container view.

You may pass a desired alpha value, that ONLY affects the
Core Graphics context, and automatically uses that value when
setting colors.

In general, only style and dimming affect color.

NOTE:	In Cocoa views, setTextAttributesDictionary()
		determines the foreground color.

IMPORTANT:	Core Graphics support is INCOMPLETE.  This routine
			may not completely update the context with all
			necessary parameters.

(3.1)
*/
void
useTerminalTextColors	(My_TerminalViewPtr			inTerminalViewPtr,
						 CGContextRef				inDrawingContext,
						 TextAttributes_Object		inAttributes,
						 Boolean					inIsCursor,
						 Float32					inDesiredAlpha)
{
	// Cocoa and Quartz setup
	CGFloatRGBColor		backgroundDeviceColor;
	CGFloatRGBColor		foregroundDeviceColor; // not used except for reference when inverting
	
	
	// find the correct colors in the color table
	getScreenColorsForAttributes(inTerminalViewPtr, inAttributes,
									&foregroundDeviceColor, &backgroundDeviceColor,
									&inTerminalViewPtr->screen.currentRenderNoBackground);
	
	// set up background color; note that in drag highlighting mode,
	// the background color is preset by the highlight renderer
	if (false == inTerminalViewPtr->screen.currentRenderDragColors)
	{
		NSColor*	foregroundNSColor = [NSColor colorWithCalibratedRed:foregroundDeviceColor.red
																		green:foregroundDeviceColor.green
																		blue:foregroundDeviceColor.blue
																		alpha:1.0];
		NSColor*	backgroundNSColor = [NSColor colorWithCalibratedRed:backgroundDeviceColor.red
																		green:backgroundDeviceColor.green
																		blue:backgroundDeviceColor.blue
																		alpha:1.0];
		
		
		[backgroundNSColor setAsBackgroundInCGContext:inDrawingContext];
		
		if (false == inTerminalViewPtr->text.selection.inhibited)
		{
			// “darken” the colors if text is selected, but only in the foreground;
			// in the background, the view renders an outline of the selection, so
			// selected text should NOT have any special appearance in that case
			if (inAttributes.hasSelection() && inTerminalViewPtr->isActive)
			{
				inTerminalViewPtr->screen.currentRenderNoBackground = false;
				
				if (gPreferenceProxies.invertSelections)
				{
					// use inverted colors (foreground is set elsewhere)
					[foregroundNSColor setAsBackgroundInCGContext:inDrawingContext];
				}
				else
				{
					// use selection colors
					NSColor*	selectionTextColor = [foregroundNSColor copy];
					NSColor*	selectionBackgroundColor = [backgroundNSColor copy];
					
					
					if (NO == [NSColor selectionColorsForForeground:&selectionTextColor
																	background:&selectionBackgroundColor])
					{
						// bail; force the default color even if it won’t look as good
						selectionBackgroundColor = [NSColor selectedTextBackgroundColor];
					}
					
					// convert to RGB and then update the context
					[selectionBackgroundColor setAsBackgroundInCGContext:inDrawingContext];
				}
			}
		}
		
		// when the screen is inactive, it is dimmed; in addition, any text that is
		// not selected is FURTHER dimmed so as to emphasize the selection and show
		// that the selection is in fact a click-through region
		unless ((inTerminalViewPtr->isActive) || (gPreferenceProxies.dontDimTerminals) ||
				((false == inTerminalViewPtr->text.selection.inhibited) && inAttributes.hasSelection() && [NSApp isActive]))
		{
			inTerminalViewPtr->screen.currentRenderNoBackground = false;
			
			// dim screen
			foregroundNSColor = [foregroundNSColor colorCloserToWhite];
			backgroundNSColor = [backgroundNSColor colorCloserToWhite];
			
			// for text that is not selected, do an “iPhotoesque” selection where
			// the selection appears darker than its surrounding text; this also
			// causes the selection to look dark (appropriate because it is draggable)
			unless ((false == selectionExists(inTerminalViewPtr)) || (gPreferenceProxies.invertSelections))
			{
				// use even lighter colors (more than the dimmed color above)
				foregroundNSColor = [foregroundNSColor colorCloserToWhite];
				backgroundNSColor = [backgroundNSColor colorCloserToWhite];
			}
		}
		
		// give search results a special appearance
		if (inAttributes.hasSearchHighlight())
		{
			// use selection colors
			NSColor*	searchResultTextColor = [foregroundNSColor copy];
			NSColor*	searchResultBackgroundColor = [backgroundNSColor copy];
			
			
			if (NO == [NSColor searchResultColorsForForeground:&searchResultTextColor
																background:&searchResultBackgroundColor])
			{
				// bail; force the default color even if it won’t look as good
				searchResultBackgroundColor = [[NSColor selectedTextBackgroundColor] colorWithShading];
			}
			
			if (false == inTerminalViewPtr->text.selection.inhibited)
			{
				// adjust the colors if text is selected
				if (inAttributes.hasSelection() && inTerminalViewPtr->isActive)
				{
					searchResultBackgroundColor = [searchResultBackgroundColor colorWithShading];
				}
			}
			
			searchResultBackgroundColor = [searchResultBackgroundColor
											colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
			[searchResultBackgroundColor setAsBackgroundInCGContext:inDrawingContext];
			
			inTerminalViewPtr->screen.currentRenderNoBackground = false;
		}
		
		// finally, check the foreground and background colors; do not allow
		// them to be identical unless “concealed” is the style (e.g. perhaps
		// text is ANSI white and the background is white; that's invisible!)
		unless (inAttributes.hasConceal())
		{
			// UNIMPLEMENTED
		}
		
		// set the cursor color if necessary
		if (inIsCursor)
		{
			CGFloatRGBColor		floatRGB;
			
			
			if (inTerminalViewPtr->screen.cursor.isCustomColor)
			{
				// read the user’s preferred cursor color
				getScreenCustomColor(inTerminalViewPtr, kTerminalView_ColorIndexCursorBackground, &floatRGB);
				CGContextSetRGBFillColor(inDrawingContext, floatRGB.red, floatRGB.green, floatRGB.blue, inDesiredAlpha);
			}
		}
	}
}// useTerminalTextColors


/*!
This method handles bell signals in specific terminal
windows.  If the user has global bell sounding turned
off (“visual bell”), no bell is heard, but the window
is flashed.  If the specified window is not in front
at the time this method is invoked, the window will
flash regardless of the “visual bell” setting.  If
MacTerm is suspended when the bell occurs, and the
user has notification preferences set, MacTerm will
post a notification event.

(3.0)
*/
void
visualBell	(My_TerminalViewPtr		inTerminalViewPtr)
{
	auto	contentView = inTerminalViewPtr->encompassingNSView.terminalContentView;
	
	
	if (NO == contentView.showVisualBell)
	{
		id		invertFilter = [CIFilter colorInvertFilter];
		
		
		contentView.showVisualBell = YES;
		contentView.needsDisplay = YES;
		if (nil != invertFilter)
		{
			[contentView animator].layer.filters = @[invertFilter];
		}
		CocoaExtensions_RunLater(0.2,
									^{
										contentView.showVisualBell = NO;
										contentView.needsDisplay = YES;
										[contentView animator].layer.filters = nil;
									});
	}
}// visualBell

} // anonymous namespace


#pragma mark -
@implementation TerminalView_BackgroundView //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		self->_clickDelegate = nil;
		self->_colorIndex = kMyBasicColorIndexNormalBackground;
		self->_exactColor = nil;
		self->_internalViewPtr = nullptr;
		
		self.wantsLayer = YES;
		
		self->_registeredObservers = [[NSMutableArray alloc] init];
		[self.registeredObservers addObject:[self newObserverFromSelector:@selector(effectiveAppearance) ofObject:NSApp options:0]];
	}
	return self;
}// initWithFrame:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self removeObserversSpecifiedInArray:self.registeredObservers];
}// dealloc


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to key values by updating dependent
states such as frame rectangles or the display.

(2019.03)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
	BOOL	handled = NO;
	
	
	if ([self observerArray:self.registeredObservers containsContext:aContext])
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			// IMPORTANT: these will only be defined if the call to add
			// the observer includes the appropriate options
			//id			oldValue = [aChangeDictionary objectForKey:NSKeyValueChangeOldKey];
			//id			newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
			
			
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(effectiveAppearance)))
			{
				// NSApplication has changed appearance, e.g. Dark or not; this
				// may cause the colors to be different so refresh the display
				self.needsDisplay = YES;
			}
			else
			{
				Console_Warning(Console_WriteValueCFString, "terminal background view: valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


#pragma mark NSResponder


/*!
Used to notify the delegate of this event.

(2018.06)
*/
- (void)
mouseDown:(NSEvent*)	anEvent
{
	[self.clickDelegate didReceiveMouseDownEvent:anEvent forView:self];
}// mouseDown:


/*!
Used to notify the delegate of this event.

(2018.06)
*/
- (void)
mouseDragged:(NSEvent*)	anEvent
{
	if ([self.clickDelegate respondsToSelector:@selector(didReceiveMouseDraggedEvent:forView:)])
	{
		[self.clickDelegate didReceiveMouseDraggedEvent:anEvent forView:self];
	}
}// mouseDragged:


/*!
Used to notify the delegate of this event.

(2018.06)
*/
- (void)
mouseUp:(NSEvent*)	anEvent
{
	if ([self.clickDelegate respondsToSelector:@selector(didReceiveMouseUpEvent:forView:)])
	{
		[self.clickDelegate didReceiveMouseUpEvent:anEvent forView:self];
	}
}// mouseUp:


#pragma mark NSView


/*!
Returns YES to allow background views to render virtually
at any time.

(4.1)
*/
- (BOOL)
canDrawConcurrently
{
	// NOTE: This "YES" is meaningless unless the containing NSWindow
	// returns "YES" from its "allowsConcurrentViewDrawing" method.
	return YES;
}// canDrawConcurrently


/*!
Render the specified part of the terminal background.

(4.0)
*/
- (void)
drawRect:(NSRect)	aRect
{
#pragma unused(aRect)
	// WARNING: Since "canDrawConcurrently" returns YES, this should
	// not do anything that requires execution on the main thread.
	My_TerminalViewPtr	viewPtr = self.internalViewPtr;
	CGContextRef		drawingContext = [[NSGraphicsContext currentContext] CGContext];
	CGRect				clipBounds = CGContextGetClipBoundingBox(drawingContext);
	
	
	if (nil != self.exactColor)
	{
		NSColor*	asRGB = [self.exactColor colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
		
		
		// draw background
		CGContextSetRGBFillColor(drawingContext, asRGB.redComponent, asRGB.greenComponent, asRGB.blueComponent, 1.0/* alpha */);
		CGContextFillRect(drawingContext, clipBounds);
	}
	else
	{
		// NOTE: For porting purposes the colored background is simply drawn.
		// Another option would be to update the layer’s background color
		// directly whenever the view color changes.
		if (nullptr != viewPtr)
		{
			CGFloatRGBColor const&		asFloats = viewPtr->text.colors[self.colorIndex];
			
			
			// draw background
			CGContextSetRGBFillColor(drawingContext, asFloats.red, asFloats.green, asFloats.blue, 1.0/* alpha */);
			CGContextFillRect(drawingContext, clipBounds);
		}
		else
		{
			// no associated view yet; draw a dummy background
			CGContextSetRGBFillColor(drawingContext, 1.0/* red */, 1.0/* green */, 1.0/* blue */, 1.0/* alpha */);
			CGContextFillRect(drawingContext, clipBounds);
		}
	}
}// drawRect:


/*!
Returns YES only if the view has no transparent parts.

(4.0)
*/
- (BOOL)
isOpaque
{
	return YES;
}// isOpaque


@end //} TerminalView_BackgroundView


#pragma mark -
@implementation TerminalView_BackgroundView (TerminalView_BackgroundViewInternal) //{


@end //} TerminalView_BackgroundView (TerminalView_BackgroundViewInternal)


#pragma mark -
@implementation TerminalView_ContentView //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		_focusFollowsMouseTrackingArea = nil; // see "updateTrackingAreas"
		_focusShouldFollowMouse = NO; // see "model:preferenceChange:context:"
		_internalViewPtr = nullptr;
		_modifierFlagsForCursor = 0;
		_registeredObservers = [[NSMutableArray alloc] init];
		_showDragHighlight = NO;
		_showVisualBell = NO;
		[self.registeredObservers addObject:[self newObserverFromSelector:@selector(effectiveAppearance) ofObject:NSApp options:0]];
		
		[self registerForDraggedTypes:@[
											// in order of preference; the list of accepted drag text types should
											// agree with what Clipboard_CreateCFStringFromPasteboard() supports
											BRIDGE_CAST(kUTTypeUTF16ExternalPlainText, NSString*),
											BRIDGE_CAST(kUTTypeUTF16PlainText, NSString*),
											BRIDGE_CAST(kUTTypeUTF8PlainText, NSString*),
											BRIDGE_CAST(kUTTypePlainText, NSString*),
											@"com.apple.traditional-mac-plain-text",
											BRIDGE_CAST(kUTTypeFileURL, NSString*),
											BRIDGE_CAST(kUTTypeDirectory, NSString*), // includes packages
											//BRIDGE_CAST(kUTTypeFolder, NSString*), // also a directory
										]];
		
		self.refusesFirstResponder = NO;
		self.wantsLayer = YES;
		
		// install a callback that finds out about changes to key preferences
		{
			Preferences_Result		error = kPreferences_ResultOK;
			
			
			self.preferenceChangeListener = [[ListenerModel_StandardListener alloc]
												initWithTarget:self
																eventFiredSelector:@selector(model:preferenceChange:context:)];
			
			error = Preferences_StartMonitoring([self.preferenceChangeListener listenerRef], kPreferences_TagFocusFollowsMouse,
												true/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
		}
		
		// watch for menu tracking to alter focus-follows-mouse behavior
		[self whenObject:NSApp.mainMenu postsNote:NSMenuDidEndTrackingNotification
							performSelector:@selector(menuBarDidEndTracking:)];
	}
	return self;
}// initWithFrame:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self ignoreWhenObject:NSApp.mainMenu postsNote:NSMenuDidEndTrackingNotification];
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring(self.preferenceChangeListener.listenerRef,
																kPreferences_TagFocusFollowsMouse);
	[self removeObserversSpecifiedInArray:self.registeredObservers];
}// dealloc


/*!
Returns any Terminal View that is associated with this NSView
subclass, or nullptr if there is none.

(4.1)
*/
- (TerminalViewRef)
terminalViewRef
{
	TerminalViewRef		result = nullptr;
	My_TerminalViewPtr	ptr = [self internalViewPtr];
	
	
	if (nullptr != ptr)
	{
		result = ptr->selfRef;
	}
	return result;
}// terminalViewRef


#pragma mark Actions: Commands_Printing


- (IBAction)
performPrintScreen:(id)		sender
{
	// see also "performPrintSelection:", which is similar...
	[self printForSelector:_cmd sender:sender];
}
- (id)
canPerformPrintScreen:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
performPrintSelection:(id)	sender
{
#pragma unused(sender)
	// see also "performPrintScreen:", which is similar...
	[self printForSelector:_cmd sender:sender];
}
- (id)
canPerformPrintSelection:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = TerminalView_TextSelectionExists([self terminalViewRef]);
	
	
	return ((result) ? @(YES) : @(NO));
}


#pragma mark Actions: Commands_SessionProcessControlling


- (IBAction)
performKill:(id)	sender
{
#pragma unused(sender)
	TerminalWindowRef	terminalWindow = [self.window terminalWindowRef];
	Boolean				allowForceQuit = true;
	
	
	// in Full Screen mode, this command might not always be allowed
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
							&allowForceQuit))
	{
		allowForceQuit = true; // assume a value if the preference cannot be found
	}
	
	if ((false == TerminalWindow_IsFullScreen(terminalWindow)) || (allowForceQuit))
	{
		Session_DisplayTerminationWarning([self boundSession], kSession_TerminationDialogOptionKeepWindow);
	}
	else
	{
		Sound_StandardAlert();
	}
}
- (id)
canPerformKill:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		listeningSession = [self boundSession];
	BOOL			result = NO;
	
	
	if (nullptr != listeningSession)
	{
		result = (false == Session_StateIsDead(listeningSession));
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performRestart:(id)		sender
{
#pragma unused(sender)
	TerminalWindowRef	terminalWindow = [self.window terminalWindowRef];
	Boolean				allowForceQuit = true;
	
	
	// in Full Screen mode, this command might not always be allowed
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
							&allowForceQuit))
	{
		allowForceQuit = true; // assume a value if the preference cannot be found
	}
	
	if ((false == TerminalWindow_IsFullScreen(terminalWindow)) || (allowForceQuit))
	{
		Session_DisplayTerminationWarning([self boundSession], kSession_TerminationDialogOptionKeepWindow |
																kSession_TerminationDialogOptionRestart);
	}
	else
	{
		Sound_StandardAlert();
	}
}
- (id)
canPerformRestart:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	
	
	// this is not exactly the same as the default validator;
	// in particular, this is permitted in Full Screen mode
	if (nullptr != session)
	{
		return @(YES);
	}
	return @(NO);
}


#pragma mark Actions: Commands_SessionThrottling


- (IBAction)
performInterruptProcess:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_UserInputInterruptProcess(session);
}


- (IBAction)
performJumpScrolling:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_FlushNetwork(session);
}


- (IBAction)
performSuspendToggle:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_SetNetworkSuspended(session, !Session_NetworkIsSuspended(session));
}
- (id)
canPerformSuspendToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		isChecked = Session_NetworkIsSuspended(session);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


#pragma mark Actions: Commands_StandardEditing


/*!
Copies selected text or images from the terminal view to
the clipboard.

(2018.12)
*/
- (IBAction)
copy:(id)	sender
{
#pragma unused(sender)
	// determine if the current selection is an image
	CFRetainRelease		selectedNSImageArray(TerminalView_ReturnSelectedImageArrayCopy([self terminalViewRef]),
												CFRetainRelease::kAlreadyRetained); 
	NSArray*			asArray = BRIDGE_CAST(selectedNSImageArray.returnCFArrayRef(), NSArray*);
	
	
	if (selectedNSImageArray.exists() && (asArray.count > 0))
	{
		Boolean		haveCleared = false;
		
		
		for (NSImage* asImage in asArray)
		{
			assert([asImage isKindOfClass:NSImage.class]);
			
			
			if (false == Clipboard_AddNSImageToPasteboard(asImage, nullptr/* target pasteboard */,
															(false == haveCleared)/* clear flag */))
			{
				Console_Warning(Console_WriteLine, "failed to Copy image");
			}
			else
			{
				// if more than one image is selected, add them all
				haveCleared = true;
			}
		}
	}
	else
	{
		Clipboard_TextToScrap([self terminalViewRef], kClipboard_CopyMethodStandard);
	}
}
- (id)
canCopy:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = TerminalView_TextSelectionExists([self terminalViewRef]);
	
	
	return ((result) ? @(YES) : @(NO));
}


/*!
Send text from the clipboard to the session, as if it had
been typed by the user.  This may first trigger a warning,
if the text spans multiple lines.

(2018.12)
*/
- (IBAction)
paste:(id)	sender
{
#pragma unused(sender)
	Session_Result		sessionResult = Session_UserInputPaste([self boundSession]); // paste if there is a window to paste into
	
	
	if (false == sessionResult.ok())
	{
		Console_Warning(Console_WriteValue, "failed to paste, error", sessionResult.code());
	}
}
- (id)
canPaste:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL			result = NO;
	CFArrayRef		dummyCFStringArray = nullptr;
	Boolean			clipboardContainsText = false;
	
	
	// TEMPORARY; this is a somewhat expensive way to check something
	// that can probably be stored as a simple state flag (or queried
	// from the system)
	if (Clipboard_CreateCFStringArrayFromPasteboard(dummyCFStringArray))
	{
		clipboardContainsText = true;
		CFRelease(dummyCFStringArray); dummyCFStringArray = nullptr; 
	}
	result = (clipboardContainsText);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
selectAll:(id)		sender
{
#pragma unused(sender)
	// select only the bottomost windowful of text
	TerminalView_SelectMainScreen([self terminalViewRef]);
}


- (IBAction)
selectNone:(id)		sender
{
#pragma unused(sender)
	// remove selection
	TerminalView_SelectNothing([self terminalViewRef]);
}
- (id)
canSelectNone:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = TerminalView_TextSelectionExists([self terminalViewRef]);
	
	
	return ((result) ? @(YES) : @(NO));
}


#pragma mark Actions: Commands_StandardSpeechHandling


- (IBAction)
performSpeechToggle:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	if (Session_SpeechIsEnabled(session))
	{
		Session_SpeechPause(session);
		Session_SetSpeechEnabled(session, false);
	}
	else
	{
		Session_SetSpeechEnabled(session, true);
		Session_SpeechResume(session);
	}
}
- (id)
canPerformSpeechToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		isChecked = Session_SpeechIsEnabled(session);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
startSpeaking:(id)		sender
{
#pragma unused(sender)
	// note: this method is named to match NSTextView selector
	TerminalView_GetSelectedTextAsAudio([self terminalViewRef]);
}
- (id)
canStartSpeaking:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = TerminalView_TextSelectionExists([self terminalViewRef]);
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
stopSpeaking:(id)	sender
{
#pragma unused(sender)
	// note: this method is named to match NSTextView selector
	CocoaBasic_StopSpeaking();
}
- (id)
canStopSpeaking:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return (CocoaBasic_SpeakingInProgress() ? @(YES) : @(NO));
}


#pragma mark Actions: Commands_TerminalEditing


/*!
Displays information next to the Paste command that tells the
user what will happen when it is used in the terminal (e.g.
in bracketed Paste mode, programs like shells and editors
may react differently to multi-line Paste).

(2021.12)
*/
- (IBAction)
performAssessBracketedPasteMode:(id)		sender
{
#pragma unused(sender)
	// not a real action; used for updating menu state
}
- (id)
canPerformAssessBracketedPasteMode:(id <NSValidatedUserInterfaceItem>)		anItem
{
	// use same appearance as other “title items” (small, bold and disabled),
	// which is accomplished by using an attributed string
	if ([(id)anItem isKindOfClass:NSMenuItem.class])
	{
		NSMenuItem*				asMenuItem = STATIC_CAST(anItem, NSMenuItem*);
		TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow([self boundSession]);
		TerminalScreenRef		screenBuffer = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
		Boolean					isBracketedPaste = ((nullptr != screenBuffer) && Terminal_PasteIsBracketed(screenBuffer));
		CFRetainRelease			infoString(((isBracketedPaste)
											? UIStrings_ReturnCopy(kUIStrings_ContextualMenuPasteBracketedModeOn)
											: UIStrings_ReturnCopy(kUIStrings_ContextualMenuPasteBracketedModeOff)),
											CFRetainRelease::kAlreadyRetained);
		NSDictionary*			fontDict = @{NSFontAttributeName: [NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]]};
		NSAttributedString*		attributedTitle = [[NSAttributedString alloc] initWithString:BRIDGE_CAST(infoString.returnCFStringRef(), NSString*) attributes:fontDict];
		
		
		[asMenuItem setAttributedTitle:attributedTitle];
	}
	
	return @(NO); // always disabled (displays state only)
}


/*!
Copies selected text from the terminal view to the clipboard
as a single joined line and immediately performs a Paste of
that text.

(2018.12)
*/
- (IBAction)
performCopyAndPaste:(id)	sender
{
#pragma unused(sender)
	Session_Result		sessionResult = kSession_ResultOK;
	
	
	Clipboard_TextToScrap([self terminalViewRef], kClipboard_CopyMethodStandard | kClipboard_CopyMethodInline);
	
	sessionResult = Session_UserInputPaste([self boundSession]);
	if (false == sessionResult.ok())
	{
		Console_Warning(Console_WriteValue, "failed to paste, error", sessionResult.code());
	}
}
- (id)
canPerformCopyAndPaste:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = TerminalView_TextSelectionExists([self terminalViewRef]);
	
	
	return ((result) ? @(YES) : @(NO));
}


/*!
Copies selected text from the terminal view to the clipboard
except consecutive spaces may be replaced by a single tab
(based on user preferences).

(2018.12)
*/
- (IBAction)
performCopyWithTabSubstitution:(id)		sender
{
#pragma unused(sender)
	Clipboard_TextToScrap([self terminalViewRef], kClipboard_CopyMethodTable);
}
- (id)
canPerformCopyWithTabSubstitution:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = TerminalView_TextSelectionExists([self terminalViewRef]);
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSelectEntireScrollbackBuffer:(id)	sender
{
#pragma unused(sender)
	TerminalView_SelectEntireBuffer([self terminalViewRef]);
}


#pragma mark Actions: Commands_TerminalEventHandling


- (IBAction)
performBellToggle:(id)	sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_SetBellEnabled(screenRef, !Terminal_BellIsEnabled(screenRef));
}
- (id)
canPerformBellToggle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	BOOL				result = YES;
	
	
	if (nullptr != screenRef)
	{
		isChecked = Terminal_BellIsEnabled(screenRef);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerNone:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_SetWatch(session, kSession_WatchNothing);
}
- (id)
canPerformSetActivityHandlerNone:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		isChecked = Session_WatchIsOff(session);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerNotifyOnIdle:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_SetWatch(session, kSession_WatchForInactivity);
}
- (id)
canPerformSetActivityHandlerNotifyOnIdle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		isChecked = Session_WatchIsForInactivity(session);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerNotifyOnNext:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_SetWatch(session, kSession_WatchForPassiveData);
}
- (id)
canPerformSetActivityHandlerNotifyOnNext:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		isChecked = Session_WatchIsForPassiveData(session);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerSendKeepAliveOnIdle:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_SetWatch(session, kSession_WatchForKeepAlive);
}
- (id)
canPerformSetActivityHandlerSendKeepAliveOnIdle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		isChecked = Session_WatchIsForKeepAlive(session);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


#pragma mark Actions: Commands_TerminalFileCapturing


/*!
Prompts the user to specify where to save a new file,
and then initiates a continuous capture of terminal text
to that file.

(2018.12)
*/
- (IBAction)
performCaptureBegin:(id)	sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_FileCaptureEnd(screenRef); // terminate any previous capture
	Session_DisplayFileCaptureSaveDialog([self boundSession]);
}


/*!
Stops the continuous capture of terminal text to a file.

(2018.12)
*/
- (IBAction)
performCaptureEnd:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_FileCaptureEnd(screenRef);
}
- (id)
canPerformCaptureEnd:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	BOOL				result = NO;
	
	
	result = (Terminal_FileCaptureInProgress(screenRef) ? YES : NO);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSaveSelection:(id)	sender
{
#pragma unused(sender)
	TerminalView_DisplaySaveSelectionUI([self terminalViewRef]);
}
- (id)
canPerformSaveSelection:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = TerminalView_TextSelectionExists([self terminalViewRef]);
	
	
	return ((result) ? @(YES) : @(NO));
}


#pragma mark Actions: Commands_TerminalKeyMapping


- (IBAction)
performDeleteMapToBackspace:(id)	sender
{
#pragma unused(sender)
	SessionRef			session = [self boundSession];
	Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
	
	
	keyMappings.deleteSendsBackspace = YES;
	Session_SetEventKeys(session, keyMappings);
}
- (id)
canPerformDeleteMapToBackspace:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
		
		
		isChecked = (keyMappings.deleteSendsBackspace) ? YES : NO;
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performDeleteMapToDelete:(id)	sender
{
#pragma unused(sender)
	SessionRef			session = [self boundSession];
	Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
	
	
	keyMappings.deleteSendsBackspace = NO;
	Session_SetEventKeys(session, keyMappings);
}
- (id)
canPerformDeleteMapToDelete:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
		
		
		isChecked = (false == keyMappings.deleteSendsBackspace) ? YES : NO;
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performEmacsCursorModeToggle:(id)	sender
{
#pragma unused(sender)
	SessionRef			session = [self boundSession];
	Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
	
	
	keyMappings.arrowsRemappedForEmacs = !(keyMappings.arrowsRemappedForEmacs);
	Session_SetEventKeys(session, keyMappings);
}
- (id)
canPerformEmacsCursorModeToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
		
		
		isChecked = (keyMappings.arrowsRemappedForEmacs) ? YES : NO;
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performLocalPageKeysToggle:(id)	sender
{
#pragma unused(sender)
	SessionRef			session = [self boundSession];
	Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
	
	
	keyMappings.pageKeysLocalControl = !(keyMappings.pageKeysLocalControl);
	Session_SetEventKeys(session, keyMappings);
}
- (id)
canPerformLocalPageKeysToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(session);
		
		
		isChecked = (keyMappings.pageKeysLocalControl) ? YES : NO;
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMappingCustom:(id)	sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_DisplaySpecialKeySequencesDialog(session);
}
- (id)
canPerformMappingCustom:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			result = (nullptr != session);
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetFunctionKeyLayoutRxvt:(id)	sender
{
#pragma unused(sender)
	Keypads_SetFunctionKeyLayout(kSession_FunctionKeyLayoutRxvt);
}
- (id)
canPerformSetFunctionKeyLayoutRxvt:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = Keypads_IsFunctionKeyLayoutEqualTo(kSession_FunctionKeyLayoutRxvt);
	
	
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


- (IBAction)
performSetFunctionKeyLayoutVT220:(id)	sender
{
#pragma unused(sender)
	Keypads_SetFunctionKeyLayout(kSession_FunctionKeyLayoutVT220);
}
- (id)
canPerformSetFunctionKeyLayoutVT220:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = Keypads_IsFunctionKeyLayoutEqualTo(kSession_FunctionKeyLayoutVT220);
	
	
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


- (IBAction)
performSetFunctionKeyLayoutXTermX11:(id)	sender
{
#pragma unused(sender)
	Keypads_SetFunctionKeyLayout(kSession_FunctionKeyLayoutXTerm);
}
- (id)
canPerformSetFunctionKeyLayoutXTermX11:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = Keypads_IsFunctionKeyLayoutEqualTo(kSession_FunctionKeyLayoutXTerm);
	
	
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


- (IBAction)
performSetFunctionKeyLayoutXTermXFree86:(id)	sender
{
#pragma unused(sender)
	Keypads_SetFunctionKeyLayout(kSession_FunctionKeyLayoutXTermXFree86);
}
- (id)
canPerformSetFunctionKeyLayoutXTermXFree86:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = Keypads_IsFunctionKeyLayoutEqualTo(kSession_FunctionKeyLayoutXTermXFree86);
	
	
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


#pragma mark Actions: Commands_TerminalModeSwitching


- (IBAction)
performLineWrapToggle:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_SetLineWrapEnabled(screenRef, !Terminal_LineWrapIsEnabled(screenRef));
}
- (id)
canPerformLineWrapToggle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = [self.window terminalWindowRef];
	TerminalScreenRef	currentScreen = (nullptr == terminalWindow)
										? nullptr
										: TerminalWindow_ReturnScreenWithFocus(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentScreen)
	{
		isChecked = Terminal_LineWrapIsEnabled(currentScreen);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performLocalEchoToggle:(id)		sender
{
#pragma unused(sender)
	SessionRef		session = [self boundSession];
	
	
	Session_SetLocalEchoEnabled(session, !Session_LocalEchoIsEnabled(session));
}
- (id)
canPerformLocalEchoToggle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	SessionRef		session = [self boundSession];
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != session);
	
	
	if (nullptr != session)
	{
		isChecked = Session_LocalEchoIsEnabled(session);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performReset:(id)	sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_Reset(screenRef);
}


- (IBAction)
performSaveOnClearToggle:(id)	sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_SetSaveLinesOnClear(screenRef, !Terminal_SaveLinesOnClearIsEnabled(screenRef));
}
- (id)
canPerformSaveOnClearToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = [self.window terminalWindowRef];
	TerminalScreenRef	currentScreen = (nullptr == terminalWindow)
										? nullptr
										: TerminalWindow_ReturnScreenWithFocus(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentScreen)
	{
		isChecked = Terminal_SaveLinesOnClearIsEnabled(currentScreen);
	}
	MenuUtilities_SetItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performScrollbackClear:(id)		sender
{
#pragma unused(sender)
	TerminalViewRef		view = [self terminalViewRef];
	
	
	TerminalView_DeleteScrollback(view);
}


/*!
Turns the specified terminal LED on or off.  (LEDs can
also be toggled by terminal sequences.)

(2018.12)
*/
- (IBAction)
performTerminalLED1Toggle:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_LEDSetState(screenRef, 1, false == Terminal_LEDIsOn(screenRef, 1));
}


/*!
Turns the specified terminal LED on or off.  (LEDs can
also be toggled by terminal sequences.)

(2018.12)
*/
- (IBAction)
performTerminalLED2Toggle:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_LEDSetState(screenRef, 2, false == Terminal_LEDIsOn(screenRef, 2));
}


/*!
Turns the specified terminal LED on or off.  (LEDs can
also be toggled by terminal sequences.)

(2018.12)
*/
- (IBAction)
performTerminalLED3Toggle:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_LEDSetState(screenRef, 3, false == Terminal_LEDIsOn(screenRef, 3));
}


/*!
Turns the specified terminal LED on or off.  (LEDs can
also be toggled by terminal sequences.)

(2018.12)
*/
- (IBAction)
performTerminalLED4Toggle:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screenRef = self.internalViewPtr->screen.ref;
	
	
	Terminal_LEDSetState(screenRef, 4, false == Terminal_LEDIsOn(screenRef, 4));
}


#pragma mark Actions: Commands_TerminalScreenPaging


/*!
Scrolls the lines of the terminal view toward the top edge by
one full “screenful”, consistent with hitting the Page Down key.

(2020.02)
*/
- (IBAction)
performTerminalViewPageDown:(id)	sender
{
#pragma unused(sender)
	TerminalScreenRef	screen = self.internalViewPtr->screen.ref;
	TerminalViewRef		view = [self terminalViewRef];
	
	
	TerminalView_ScrollRowsTowardTopEdge(view, Terminal_ReturnRowCount(screen));
}


/*!
Scrolls the lines of the terminal view as far up as possible,
consistent with hitting the End key.

(2020.02)
*/
- (IBAction)
performTerminalViewPageEnd:(id)		sender
{
#pragma unused(sender)
	TerminalViewRef		view = [self terminalViewRef];
	
	
	TerminalView_ScrollToEnd(view);
}


/*!
Scrolls the lines of the terminal view as far down as possible,
consistent with hitting the Home key.

(2020.02)
*/
- (IBAction)
performTerminalViewPageHome:(id)	sender
{
#pragma unused(sender)
	TerminalViewRef		view = [self terminalViewRef];
	
	
	TerminalView_ScrollToBeginning(view);
}


/*!
Scrolls the lines of the terminal view toward the bottom edge by
one full “screenful”, consistent with hitting the Page Up key.

(2020.02)
*/
- (IBAction)
performTerminalViewPageUp:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screen = self.internalViewPtr->screen.ref;
	TerminalViewRef		view = [self terminalViewRef];
	
	
	TerminalView_ScrollRowsTowardBottomEdge(view, Terminal_ReturnRowCount(screen));
}


#pragma mark Actions: Commands_TextFormatting


/*!
Uses the name of the given sender (expected to be a menu item) to
find a Format set of Preferences from which to copy new font and
color information.

(4.0)
*/
- (IBAction)
performFormatByFavoriteName:(id)	sender
{
	TerminalViewRef		currentView = self.internalViewPtr->selfRef;
	BOOL				isError = YES;
	
	
	if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
	{
		// use the specified preferences
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
		
		
		if ((nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::FORMAT, collectionName))
		{
			Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
														(Quills::Prefs::FORMAT, collectionName),
														Preferences_ContextWrap::kAlreadyRetained);
			
			
			if (namedSettings.exists())
			{
				// change font and/or colors of frontmost window according to the specified preferences
				Preferences_ContextRef		currentSettings = TerminalView_ReturnFormatConfiguration(currentView);
				Preferences_Result			prefsResult = kPreferences_ResultOK;
				
				
				prefsResult = Preferences_ContextCopy(namedSettings.returnRef(), currentSettings);
				isError = (kPreferences_ResultOK != prefsResult);
			}
		}
	}
	
	if (isError)
	{
		// failed...
		Sound_StandardAlert();
	}
}
- (id)
canPerformFormatByFavoriteName:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


/*!
Copies new font and color information from the Default set.

(4.0)
*/
- (IBAction)
performFormatDefault:(id)	sender
{
#pragma unused(sender)
	TerminalViewRef			currentView = self.internalViewPtr->selfRef;
	Preferences_ContextRef	currentSettings = TerminalView_ReturnFormatConfiguration(currentView);
	Preferences_ContextRef	defaultSettings = nullptr;
	BOOL					isError = YES;
	
	
	// reformat frontmost window using the Default preferences
	if (kPreferences_ResultOK == Preferences_GetDefaultContext(&defaultSettings, Quills::Prefs::FORMAT))
	{
		isError = (kPreferences_ResultOK != Preferences_ContextCopy(defaultSettings, currentSettings));
	}
	
	if (isError)
	{
		// failed...
		Sound_StandardAlert();
	}
}
- (id)
canPerformFormatDefault:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
performTranslationSwitchByFavoriteName:(id)		sender
{
	SessionRef		session = [self boundSession];
	BOOL			isError = YES;
	
	
	if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
	{
		// use the specified preferences
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
		
		
		if ((nullptr != session) && (nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::TRANSLATION, collectionName))
		{
			Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
														(Quills::Prefs::TRANSLATION, collectionName),
														Preferences_ContextWrap::kAlreadyRetained);
			Preferences_ContextRef		sessionSettings = Session_ReturnTranslationConfiguration(session);
			
			
			if (namedSettings.exists() && (nullptr != sessionSettings))
			{
				Preferences_TagSetRef		translationTags = PrefPanelTranslations_NewTagSet();
				
				
				if (nullptr != translationTags)
				{
					// change character set of frontmost window according to the specified preferences
					Preferences_Result		prefsResult = Preferences_ContextCopy
															(namedSettings.returnRef(), sessionSettings, translationTags);
					
					
					isError = (kPreferences_ResultOK != prefsResult);
					
					Preferences_ReleaseTagSet(&translationTags);
				}
			}
		}
	}
	
	if (isError)
	{
		// failed...
		Console_Warning(Console_WriteLine, "failed to apply named translation settings to session");
		Sound_StandardAlert();
	}
}


- (IBAction)
performTranslationSwitchDefault:(id)	sender
{
#pragma unused(sender)
	SessionRef				session = [self boundSession];
	Preferences_ContextRef	sessionSettings = Session_ReturnTranslationConfiguration(session);
	Preferences_ContextRef	defaultSettings = nullptr;
	BOOL					isError = YES;
	
	
	// reformat frontmost window using the Default preferences
	if (kPreferences_ResultOK == Preferences_GetDefaultContext(&defaultSettings, Quills::Prefs::TRANSLATION))
	{
		isError = (kPreferences_ResultOK != Preferences_ContextCopy(defaultSettings, sessionSettings));
	}
	
	if (isError)
	{
		// failed...
		Sound_StandardAlert();
	}
}
- (id)
canPerformTranslationSwitchDefault:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return @(YES);
}


#pragma mark Actions: Commands_URLSelectionHandling


- (IBAction)
performOpenURL:(id)		sender
{
#pragma unused(sender)
	TerminalScreenRef	screen = self.internalViewPtr->screen.ref;
	TerminalViewRef		view = [self terminalViewRef];
	
	
	// open the appropriate helper application for the URL in the selected
	// text (which may be MacTerm itself), and send a “handle URL” event
	URL_HandleForScreenView(screen, view);
}
- (id)
canPerformOpenURL:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == EventLoop_IsMainWindowFullScreen()) && TerminalView_TextSelectionExists([self terminalViewRef]));
	
	
	if (result)
	{
		TerminalViewRef		view = [self terminalViewRef];
		CFRetainRelease		selectedText(TerminalView_ReturnSelectedTextCopyAsUnicode
											(view, 0/* Copy with Tab Substitution info */,
												kTerminalView_TextFlagInline),
											CFRetainRelease::kAlreadyRetained);
		
		
		if (false == selectedText.exists())
		{
			result = NO;
		}
		else
		{
			URL_Type	urlKind = URL_ReturnTypeFromCFString(selectedText.returnCFStringRef());
			
			
			if (kURL_TypeInvalid == urlKind)
			{
				result = NO; // disable command for non-URL text selections
			}
		}
	}
	
	return ((result) ? @(YES) : @(NO));
}


#pragma mark NSDraggingDestination


/*!
Invoked when the user has dragged data into the content
area of the terminal window.

(2018.08)
*/
- (NSDragOperation)
draggingEntered:(id <NSDraggingInfo>)	sender
{
	NSDragOperation			result = NSDragOperationNone;
	My_TerminalViewPtr		viewPtr = [self internalViewPtr];
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.readOnly) &&
		([sender draggingSourceOperationMask] & NSDragOperationCopy))
	{
		result = NSDragOperationCopy;
		
		// show a highlight area by setting the appropriate view property
		// and requesting a redraw
		self.showDragHighlight = YES;
		self.needsDisplay = YES;
		
		self.didDragActivateWindow = NO;
		self.dragEnterTime = CFAbsoluteTimeGetCurrent();
	}
	return result;
}// draggingEntered:


/*!
Returns YES so that the window can auto-activate after a drag
has lingered for awhile.

(2020.04)
*/
- (BOOL)
wantsPeriodicDraggingUpdates
{
	return YES;
}// wantsPeriodicDraggingUpdates


/*!
Invoked periodically while drag is still inside.  Responds
by activating the window.

(2020.04)
*/
- (NSDragOperation)
draggingUpdated:(id <NSDraggingInfo>)	sender
{
#pragma unused(sender)
	NSDragOperation			result = NSDragOperationCopy;
	My_TerminalViewPtr		viewPtr = [self internalViewPtr];
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.readOnly))
	{
		// if the user has been hovering the drag over the window for
		// awhile, auto-activate the window
		if ((NO == self.didDragActivateWindow) &&
			((CFAbsoluteTimeGetCurrent() - self.dragEnterTime) > 1.0/* arbitrary */))
		{
			TerminalWindowRef	terminalWindow = self.window.terminalWindowRef;
			
			
			self.didDragActivateWindow = YES;
			TerminalWindow_Select(terminalWindow);
		}
	}
	
	return result;
}// draggingUpdated:


/*!
Invoked when the user has dragged data out of the content
area of the terminal window.

(2018.08)
*/
- (void)
draggingExited:(id <NSDraggingInfo>)	sender
{
#pragma unused(sender)
	My_TerminalViewPtr		viewPtr = [self internalViewPtr];
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.readOnly))
	{
		// remove the drag highlight
		self.showDragHighlight = NO;
		self.needsDisplay = YES;
	}
	
	self.didDragActivateWindow = NO;
	self.dragEnterTime = 0;
}// draggingExited:


/*!
Invoked when the user has dragged data out of the content
area of the terminal window but dropped it somewhere else
(or cancelled the drag).

(2020.04)
*/
- (void)
draggingEnded:(id <NSDraggingInfo>)		sender
{
#pragma unused(sender)
	My_TerminalViewPtr		viewPtr = [self internalViewPtr];
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.readOnly))
	{
		// remove the drag highlight
		self.showDragHighlight = NO;
		self.needsDisplay = YES;
	}
	
	self.didDragActivateWindow = NO;
	self.dragEnterTime = 0;
}// draggingEnded:


/*!
Invoked when the user is about to drop data into the content
area of the terminal window; returns YES only if the data is
acceptable.

(2018.08)
*/
- (BOOL)
prepareForDragOperation:(id <NSDraggingInfo>)	sender
{
	BOOL				result = NO;
	My_TerminalViewPtr	viewPtr = [self internalViewPtr];
	
	
	if ((nullptr != viewPtr) && (false == viewPtr->text.selection.readOnly))
	{
		NSPoint				viewLocation = [self convertPoint:sender.draggingLocation fromView:nil];
		TerminalView_Cell	droppedOnColumnRow;
		Boolean				didFind = findVirtualCellFromScreenPoint(viewPtr, CGPointMake(viewLocation.x, viewLocation.y),
																		droppedOnColumnRow);
		
		
		// drops are rejected if they occur on top of selected text
		if ((false == didFind) || (false == pointInSelection(viewPtr, droppedOnColumnRow)))
		{
			// accept the drag
			result = YES;
		}
	}
	
	return result;
}// prepareForDragOperation:


/*!
Invoked when the user has dropped data into the content
area of the terminal window.

(2018.08)
*/
- (BOOL)
performDragOperation:(id <NSDraggingInfo>)		sender
{
	BOOL				result = NO;
	NSPasteboard*		dragPasteboard = [sender draggingPasteboard];
	SessionRef			listeningSession = [self boundSession];
	Session_Result		sessionResult = kSession_ResultOK;
	Boolean				cursorMovesPriorToDrops = false;
	
	
	//
	// the types sent to "registerForDraggedTypes" at initialization time
	// should all be checked here (in order of most specific, otherwise
	// the less-specific ones will match first)
	//
	
	// determine if terminal cursor first moves to drop location
	unless (kPreferences_ResultOK ==
			Preferences_GetData(kPreferences_TagCursorMovesPriorToDrops,
								sizeof(cursorMovesPriorToDrops), &cursorMovesPriorToDrops))
	{
		cursorMovesPriorToDrops = false; // assume a value, if a preference can’t be found
	}
	
	// if the user preference is set, first move the cursor to the drop location
	if (cursorMovesPriorToDrops)
	{
		// move cursor based on the local point
		NSPoint		viewLocation = [self convertPoint:sender.draggingLocation fromView:nil];
		
		
		TerminalView_MoveCursorWithArrowKeys([self terminalViewRef], CGPointMake(viewLocation.x, viewLocation.y));
	}
	
	// “type” the text; this could trigger the “multi-line paste” alert
	if ((nullptr != listeningSession) && (NO == Session_IsReadOnly(listeningSession)))
	{
		sessionResult = Session_UserInputPaste(listeningSession, dragPasteboard);
		if (sessionResult.ok())
		{
			result = YES;
		}
	}
	
	if (NO == result)
	{
		Console_Warning(Console_WriteValue, "failed to drop pasteboard item, error", sessionResult.code());
	}
	
	return result;
}// performDragOperation:


/*!
Removes the drag highlight when dragging is finished
for any reason.

(2020.04)
*/
- (void)
concludeDragOperation:(id <NSDraggingInfo>)		sender
{
#pragma unused(sender)
	self.showDragHighlight = NO;
	self.needsDisplay = YES;
}// concludeDragOperation:


#pragma mark NSDraggingSource


/*!
Tells the system what dragging operations can be initiated
from a terminal view.

(2020.04)
*/
- (NSDragOperation)
draggingSession:(NSDraggingSession*)						aSession
sourceOperationMaskForDraggingContext:(NSDraggingContext)	aContext
{
	NSDragOperation		result = NSDragOperationNone;
	
	
	switch (aContext)
	{
	case NSDraggingContextWithinApplication:
	case NSDraggingContextOutsideApplication:
	default:
		result = NSDragOperationCopy;
		break;
	}
	
	return result;
}// draggingSession:sourceOperationMaskForDraggingContext:


/*!
Tells the system whether or not to allow modifier keys (such
as Option) to alter the drag.

(2020.04)
*/
- (BOOL)
ignoreModifierKeysForDraggingSession:(NSDraggingSession*)	aSession
{
	BOOL	result = YES; // ignore keys because there is no move/copy option for terminals
	
	
	return result;
}// ignoreModifierKeysForDraggingSession:


/*!
Called when a drag has been initiated in the terminal view.

(2020.04)
*/
- (void)
draggingSession:(NSDraggingSession*)	aSession
willBeginAtPoint:(NSPoint)				aScreenPoint
{
	//NSLog(@"terminal view drag begin"); // debug
}// draggingSession:willBeginAtPoint:


/*!
Called when a drag initiated in the terminal view is still moving.

(2020.04)
*/
- (void)
draggingSession:(NSDraggingSession*)	aSession
movedToPoint:(NSPoint)					aScreenPoint
{
	//NSLog(@"terminal view drag move"); // debug
}// draggingSession:movedToPoint:


/*!
Called when a drag initiated in the terminal view has ended
(either by being dropped somewhere or cancelled).

(2020.04)
*/
- (void)
draggingSession:(NSDraggingSession*)	aSession
endedAtPoint:(NSPoint)					aScreenPoint
operation:(NSDragOperation)				anOperation
{
	//NSLog(@"terminal view drag end"); // debug
}// draggingSession:endedAtPoint:operation:


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to key values by updating dependent
states such as frame rectangles or the display.

(2019.03)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
	BOOL	handled = NO;
	
	
	if ([self observerArray:self.registeredObservers containsContext:aContext])
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			// IMPORTANT: these will only be defined if the call to add
			// the observer includes the appropriate options
			//id			oldValue = [aChangeDictionary objectForKey:NSKeyValueChangeOldKey];
			//id			newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
			
			
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(effectiveAppearance)))
			{
				// NSApplication has changed appearance, e.g. Dark or not; this
				// may cause the colors to be different so refresh the display
				// (INCOMPLETE; auto-switch to dark-mode Format could occur here)
				self.needsDisplay = YES;
			}
			else
			{
				Console_Warning(Console_WriteValueCFString, "terminal content view: valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


#pragma mark NSMenuDidEndTrackingNotification


/*!
Notified when menu bar tracking has ended.  This is
used to delay focus-follows-mouse slightly, otherwise
the selected menu command might inadvertently apply
to a newly-hovered-over window (e.g. the window that
happens to be under the menu item). 

(2022.04)
*/
- (void)
menuBarDidEndTracking:(NSNotification*)		aNotification
{
	NSMenu*		trackedMenuBar = REINTERPRET_CAST(aNotification.object, NSMenu*);
	
	
	if ([NSApp mainMenu] != trackedMenuBar)
	{
		// note: apparently AppKit will send this notification for ANY menu
		// (such as toolbars or pop-up menus); ignore if not the main menu bar
		//Console_Warning(Console_WriteLine, "'menuBarDidEndTracking:' received unexpected notification for different menu");
	}
	else
	{
		Console_WriteLine("did end menu tracking"); // debug
		self.menuEndTrackingTime = CFAbsoluteTimeGetCurrent();
	}
}// menuBarDidEndTracking:


#pragma mark NSResponder


/*!
It is necessary for terminal views to accept “first responder”
in order to ever receive actions such as menu commands!

This returns NO for read-only views however to prevent the
focus ring from appearing when the view is first clicked.

See also "canBecomeKeyView".

(4.0)
*/
- (BOOL)
acceptsFirstResponder
{
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	BOOL					result = YES;
	
	
	if (nullptr != viewPtr)
	{
		result = (false == viewPtr->text.selection.readOnly);
	}
	
	return result;
}// acceptsFirstResponder


/*!
Keeps track of the state of modifier keys so that an internal
cache can be updated (used to change the cursor shape).

(4.1)
*/
- (void)
flagsChanged:(NSEvent*)		anEvent
{
	[super flagsChanged:anEvent];
	self.modifierFlagsForCursor = [anEvent modifierFlags];
	[[self window] invalidateCursorRectsForView:self];
}// flagsChanged:


/*!
For any unrecognized keystrokes, translates the event into
an appropriate text command that may invoke another method
on this object from the NSTextInputClient protocol.

(2018.05)
*/
- (void)
keyDown:(NSEvent*)		anEvent
{
	if (nil != anEvent)
	{
		My_TerminalViewPtr		viewPtr = self.internalViewPtr;
		BOOL					normalText = YES;
		BOOL					metaDown = NO;
		
		
		if ((0 != (anEvent.modifierFlags & NSEventModifierFlagShift)) || (0 != (anEvent.modifierFlags & NSEventModifierFlagOption)))
		{
			SessionRef			listeningSession = [self boundSession];
			Session_EventKeys	keyMappings = Session_ReturnEventKeys(listeningSession);
			
			
			// determine if user-specified Emacs meta key sequence has been used
			if (keyMappings.meta == kSession_EmacsMetaKeyShiftOption)
			{
				metaDown = ((0 != (anEvent.modifierFlags & NSEventModifierFlagOption)) && (0 != (anEvent.modifierFlags & NSEventModifierFlagShift)) &&
							(0 == (anEvent.modifierFlags & NSEventModifierFlagCommand)) && (0 == (anEvent.modifierFlags & NSEventModifierFlagControl)));
			}
			else if (keyMappings.meta == kSession_EmacsMetaKeyOption)
			{
				metaDown = ((0 != (anEvent.modifierFlags & NSEventModifierFlagOption)) && (0 == (anEvent.modifierFlags & NSEventModifierFlagShift)) &&
							(0 == (anEvent.modifierFlags & NSEventModifierFlagCommand)) && (0 == (anEvent.modifierFlags & NSEventModifierFlagControl)));
			}
		}
		
		if ((metaDown) || (0 != (anEvent.modifierFlags & NSEventModifierFlagControl)))
		{
			// control keys should not be mapped to normal Mac text editing
			// (e.g. they do not locally move the cursor); they should be
			// interpreted by the Session, which has the option of still
			// handling certain events locally
			NSString*		characterString = [anEvent.charactersIgnoringModifiers lowercaseString];
			
			
			if (1 == characterString.length)
			{
				char	asChar = STATIC_CAST([characterString characterAtIndex:0], char);
				
				
				if ((asChar >= 64/* '@' */) && (asChar <= 127))
				{
					if (metaDown)
					{
						[self.textInputDelegate receivedMetaWithCharacter:asChar terminalView:viewPtr->selfRef];
					}
					else
					{
						[self.textInputDelegate receivedControlWithCharacter:asChar terminalView:viewPtr->selfRef];
					}
					normalText = NO;
				}
			}
		}
		
		if (0 != (anEvent.modifierFlags & NSEventModifierFlagFunction))
		{
			BOOL	didHandle = NO;
			
			
			[self.textInputDelegate receivedVirtualKeyPress:anEvent.keyCode terminalView:viewPtr->selfRef didHandle:&didHandle];
			if (didHandle)
			{
				normalText = NO;
			}
		}
		
		if (normalText)
		{
			// normally it should be possible to pass plain text to "interpretKeyEvents:" but
			// key repetition of ordinary keys does not work well in that case (most letters
			// appear once, seemingly because they would otherwise display an accent window
			// pop-up in a regular text view); since text editors such as "vim" and other
			// terminal commands benefit from proper repetition support, manually intervene
			// and recognize single-letter values directly (for anything else, see the method
			// "insertText:replacementRange:")
			if  ((1 == anEvent.characters.length) && isalnum(STATIC_CAST([anEvent.characters characterAtIndex:0], char)) &&
					((0 == (anEvent.modifierFlags & NSEventModifierFlagOption)) && (0 == (anEvent.modifierFlags & NSEventModifierFlagShift)) &&
						(0 == (anEvent.modifierFlags & NSEventModifierFlagCommand)) && (0 == (anEvent.modifierFlags & NSEventModifierFlagControl))))
			{
				[self.textInputDelegate receivedString:anEvent.characters terminalView:viewPtr->selfRef];
			}
			else
			{
				// "interpretKeyEvents:" uses "doCommandBySelector:" or "insertText:replacementRange:"
				[self interpretKeyEvents:@[anEvent]]; // translate for NSTextInputClient
			}
		}
	}
}// keyDown:


/*!
Responds to a mouse-down by potentially initiating a drag.

(2020.04)
*/
- (void)
mouseDown:(NSEvent*)	anEvent
{
	if (nil == anEvent)
	{
		return;
	}
	
	My_TerminalViewPtr	viewPtr = self.internalViewPtr;
	NSPoint				windowLocation = [anEvent locationInWindow];
	NSPoint				viewLocation = [self convertPoint:windowLocation fromView:nil];
	TerminalView_Cell	mouseDownCell;
	BOOL				singleClickInSelection = NO;
	BOOL				newSelection = NO;
	BOOL				extendSelection = NO;
	BOOL				beginDrag = NO; // initially...
	
	
	// constrain point to view (in case it was initiated from
	// a border view)
	if (viewLocation.x < 0)
	{
		viewLocation.x = 0;
	}
	if (viewLocation.y < 0)
	{
		viewLocation.y = 0;
	}
	if (viewLocation.x >= NSWidth(self.frame))
	{
		viewLocation.x = (NSWidth(self.frame) - 1);
	}
	if (viewLocation.y >= NSHeight(self.frame))
	{
		viewLocation.y = (NSHeight(self.frame) - 1);
	}
	
	// classify the event
	if (anEvent.clickCount > 2)
	{
		// triple-click action
		if (anEvent.modifierFlags & NSEventModifierFlagShift)
		{
			extendSelection = YES;
		}
		else
		{
			handleMultiClick(viewPtr, 3);
		}
	}
	else if (anEvent.clickCount > 1)
	{
		// double-click action
		if (anEvent.modifierFlags & NSEventModifierFlagShift)
		{
			extendSelection = YES;
		}
		else
		{
			handleMultiClick(viewPtr, 2);
		}
	}
	else
	{
		// single click
		UNUSED_RETURN(BOOL)findVirtualCellFromScreenPoint(viewPtr, CGPointMake(viewLocation.x, viewLocation.y),
															mouseDownCell);
		if (pointInSelection(viewPtr, mouseDownCell))
		{
			singleClickInSelection = YES;
		}
		else
		{
			if (anEvent.modifierFlags & NSEventModifierFlagShift)
			{
				extendSelection = YES;
			}
			else
			{
				newSelection = YES;
			}
		} 
	}
	
	if (singleClickInSelection)
	{
		// possible drag (if mouse goes up without moving much; ignore)
		//NSLog(@"possible drag"); // debug
		if (anEvent.modifierFlags & NSEventModifierFlagShift)
		{
			extendSelection = YES;
		}
		else
		{
			beginDrag = [self assessIsDragForSingleButtonMouseDownEvent:anEvent];
			if (NO == beginDrag)
			{
				//NSLog(@"no drag (deselect all)"); // debug
				TerminalView_SelectNothing(viewPtr->selfRef);
			}
		}
	}
	
	if (beginDrag)
	{
		//NSLog(@"begin drag"); // debug
		id< NSDraggingSource >				dragSource = self;
		NSMutableArray< NSDraggingItem* >*	draggedItems = [[NSMutableArray< NSDraggingItem* > alloc] init];
		HIShapeRef							selectedTextShape = getSelectedTextAsNewHIShape(viewPtr, 0/* inset */);
		CFRetainRelease						selectedTextCFString(returnSelectedTextCopyAsUnicode
																	(viewPtr, 0/* spaces to replace with tabs */, STATIC_CAST(0, TerminalView_TextFlags)),
																	CFRetainRelease::kAlreadyRetained);
		NSString*							asNSString = BRIDGE_CAST(selectedTextCFString.returnCFStringRef(), NSString*);
		NSDraggingItem*						selectedTextDragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:asNSString];
		
		
		if (nullptr == selectedTextShape)
		{
			Console_Warning(Console_WriteLine, "failed to determine shape of selection!");
			selectedTextDragItem.draggingFrame = CGRectMake(viewLocation.x, viewLocation.y, 200, 16); // arbitrary
		}
		else
		{
			Boolean const			isRectangular = viewPtr->text.selection.isRectangular;
			CGFloat const			cellWidth = viewPtr->text.font.widthPerCell.precisePixels();
			CGFloat const			cellHeight = viewPtr->text.font.heightPerCell.precisePixels();
			NSUInteger const		firstSelectedColumnIndex = viewPtr->text.selection.range.first.first;
			CGRect					shapeBounds = CGRectZero;
			NSImage*				dragImage = nil;
			HIMutableShapeRef		imageBlockShapeCopy = HIShapeCreateMutableCopy(selectedTextShape); // owned by block below (handler may draw image later)
			NSString*				selectedTextCopy = BRIDGE_CAST(selectedTextCFString.returnCFStringRef(), NSString*); // owned by block
			NSMutableDictionary*	attributeDict = [[NSMutableDictionary alloc] init]; // owned by block
			
			
			// the default selection shape is relative to the view;
			// anchor the offset at zero origin so that it can be
			// drawn relative to the drag image boundary
			UNUSED_RETURN(CGRect*)HIShapeGetBounds(selectedTextShape, &shapeBounds);
			CFRelease(selectedTextShape); selectedTextShape = nullptr;
			UNUSED_RETURN(OSStatus)HIShapeOffset(imageBlockShapeCopy, -shapeBounds.origin.x, -shapeBounds.origin.y);
			
			// determine appropriate font, size, etc.
			setTextAttributesDictionary(viewPtr, attributeDict, TextAttributes_Object());
			
			// create a drag image (outline of selected text area)
			dragImage = [NSImage imageWithSize:shapeBounds.size flipped:YES
			drawingHandler:^(NSRect UNUSED_ARGUMENT(aRect))
			{
				// IMPORTANT: "viewPtr" must be a valid object at the time this block is invoked;
				// since this is happening directly in response to a drag, the assumption is made
				// that the terminal view must be valid the entire time the image is rendered
				BOOL					result = YES; // note: documentation doesn’t say what this return value means
				__block BOOL			multiLine = NO;
				__block NSUInteger		minimumLineCount = 0; // block counts only far enough to set "multiLine"
				__block NSUInteger		lineIndex = 0;
				CGContextRef			drawingContext = [[NSGraphicsContext currentContext] CGContext];
				CGContextSaveRestore	_(drawingContext);
				
				
				// see if this is a multi-line block of text (this determines when to
				// indent the rendered text for the first line)
				[selectedTextCopy enumerateLinesUsingBlock:
				^(NSString* UNUSED_ARGUMENT(aLineString), BOOL* outStopFlagPtr) 
				{
					++minimumLineCount;
					if (minimumLineCount > 1)
					{
						multiLine = YES;
						*outStopFlagPtr = YES;
					}
				}];
				
				// draw background
				UNUSED_RETURN(OSStatus)HIShapeReplacePathInCGContext(imageBlockShapeCopy, drawingContext);
				CGContextSetRGBFillColor(drawingContext, 0.9, 0.9, 0.9, 0.93/* alpha */); // arbitrary; partially-translucent white center
				CGContextFillPath(drawingContext);
				
				// draw text
				[selectedTextCopy enumerateLinesUsingBlock:
				^(NSString* aLineString, BOOL* UNUSED_ARGUMENT(outStopFlagPtr)) 
				{
					// note: currently default attributes are used, which will cause plain text to render
					// (and cause graphics glyphs to use font rendering instead of MacTerm rendering);
					// this is generally OK because dragged text does not preserve attributes anyway but
					// note this could be improved by reading the corresponding attributes from the
					// terminal range that was used to create the original drag...
					TextAttributes_Object	textAttributes;
					CGRect					lineFrame = CGRectMake(0, (STATIC_CAST(lineIndex, CGFloat) * cellHeight),
																	aLineString.length * cellWidth, cellHeight);
					
					
					if ((0 == lineIndex) && (false == isRectangular) && (multiLine))
					{
						// indent first line based on actual start of selection range
						lineFrame.origin.x += (STATIC_CAST(firstSelectedColumnIndex, CGFloat) * cellWidth);
					}
					drawTerminalText(viewPtr, drawingContext, lineFrame, aLineString.length,
										BRIDGE_CAST(aLineString, CFStringRef), textAttributes);
					
					++lineIndex;
				}];
				
				// draw outline
				UNUSED_RETURN(OSStatus)HIShapeReplacePathInCGContext(imageBlockShapeCopy, drawingContext);
				CGContextSetLineWidth(drawingContext, 2.0);
				CGContextSetLineCap(drawingContext, kCGLineCapRound);
				CGContextSetRGBStrokeColor(drawingContext, 0.6, 0.6, 0.6, 1.0/* alpha */); // arbitrary; dark gray outline
				CGContextStrokePath(drawingContext);
				
				// free objects that were allocated outside the block (with ownership
				// transferred to the block; see above)
				CFRelease(imageBlockShapeCopy);
				
				return/* from block */ result;
			}];
			
			// configure the drag to use the image created above
			[selectedTextDragItem setDraggingFrame:shapeBounds contents:dragImage];
		}
		[draggedItems addObject:selectedTextDragItem]; // transfer ownership
		selectedTextDragItem = nil;
		
		if (0 == draggedItems.count)
		{
			Console_Warning(Console_WriteLine, "no items to drag!");
		}
		else
		{
			// initiate drag (asynchronous); see NSDraggingSource methods in this object
			[NSApp preventWindowOrdering];
			
			NSDraggingSession*		dragSession = [self beginDraggingSessionWithItems:draggedItems
																						event:anEvent
																						source:dragSource];
			
			
			dragSession.animatesToStartingPositionsOnCancelOrFail = YES;
			dragSession.draggingFormation = NSDraggingFormationDefault;
		}
	}
	
	if ((newSelection) || (extendSelection))
	{
		// begin or extend text selection
		//NSLog(@"begin/extend selection"); // debug
		__weak decltype(self)	weakSelf = self;
		
		
		//
		// note: modifier key treatment here should be consistent with Mac convention
		// and with any mouse cursors set in "resetCursorRects"
		//
		
		// if initial selection is rectangular or not, and different from the
		// selection type before the click began, erase the previous range first
		if (anEvent.modifierFlags & NSEventModifierFlagOption)
		{
			// rectangular selection shape
			if (true != viewPtr->text.selection.isRectangular)
			{
				highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
				viewPtr->text.selection.isRectangular = true;
			}
		}
		else
		{
			// normal selection shape
			if (false != viewPtr->text.selection.isRectangular)
			{
				highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
				viewPtr->text.selection.isRectangular = false;
			}
		}
		
		// this block (used in two places below) updates the text selection
		// based on the last click location, and the given new location
		auto	updateSelectionBlock =
		^(TerminalView_Cell const& newCell)
		{
			// note: selection ranges have an inclusive starting cell but an
			// exclusive (past-the-end) ending cell; special treatment is
			// required to both translate these markers, and to produce the
			// “standard” Mac selection behaviors...
			highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
			//TerminalView_FindVirtualRange(viewPtr->selfRef, std::make_pair(mouseOverCell, std::make_pair(mouseOverCell.first + 1, mouseOverCell.second + 1))); // debug
			TerminalView_Cell	inclusiveAnchor1 = viewPtr->text.selection.clickCell;
			TerminalView_Cell	inclusiveAnchor2 = newCell;
			if (viewPtr->text.selection.isRectangular)
			{
				BOOL	isLeftRange = (newCell.first < viewPtr->text.selection.clickCell.first);
				
				
				// rectangles behave a little differently; the markers can
				// move so that the first point is always top-left (no
				// matter where the mouse points are)
				auto	lowestStart = std::min< UInt16 >(inclusiveAnchor1.first, inclusiveAnchor2.first);
				auto	lowestPastEnd = std::min< TerminalView_RowIndex >(inclusiveAnchor1.second, inclusiveAnchor2.second);
				auto	charsWide = INTEGER_ABSOLUTE(inclusiveAnchor1.first - inclusiveAnchor2.first);
				auto	linesHigh = INTEGER_ABSOLUTE(inclusiveAnchor1.second - inclusiveAnchor2.second);
				
				
				if (isLeftRange)
				{
					--charsWide;
				}
				inclusiveAnchor1.first = lowestStart;
				inclusiveAnchor1.second = lowestPastEnd;
				inclusiveAnchor2.first = lowestStart + charsWide;
				inclusiveAnchor2.second = lowestPastEnd + linesHigh;
			}
			else
			{
				BOOL	isAboveAnchor = ((inclusiveAnchor2.second < inclusiveAnchor1.second) ||
										((inclusiveAnchor2.second == inclusiveAnchor1.second) && (inclusiveAnchor2.first < inclusiveAnchor1.first)));
				
				
				// normal selection style
				if (isAboveAnchor)
				{
					// place the earlier cell first, and shift end point so that
					// it is not included in the selection (standard behavior
					// while the mouse is “before” the starting point)
					std::swap(inclusiveAnchor1, inclusiveAnchor2);
					if (inclusiveAnchor2.first > 0)
					{
						--inclusiveAnchor2.first;
					}
				}
			}
			viewPtr->text.selection.range.first = inclusiveAnchor1;
			viewPtr->text.selection.range.second = inclusiveAnchor2;
			++viewPtr->text.selection.range.second.first; // convert to past-the-end
			++viewPtr->text.selection.range.second.second; // convert to past-the-end
			highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		};
		
		// determine if the selection is new or an extension of an existing range
		// (either way, highlight the initial range; note, this is why the
		// “rectangular” state above does not redraw the erased selection)
		if (extendSelection)
		{
			updateSelectionBlock(mouseDownCell);
		}
		else
		{
			// new selection
			// initialize text selection anchor to empty range (mouse must move
			// to produce highlighting)
			TerminalView_SelectNothing(viewPtr->selfRef); // initially...
			viewPtr->text.selection.clickCell = mouseDownCell;
			viewPtr->text.selection.range.first = viewPtr->text.selection.clickCell;
			viewPtr->text.selection.range.second = viewPtr->text.selection.clickCell;
			//highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		}
		
		// track mouse, extending selected range as appropriate
		[self.window trackEventsMatchingMask:(NSEventMaskLeftMouseDragged |
												NSEventMaskLeftMouseUp |
												NSEventMaskFlagsChanged)
												timeout:NSEventDurationForever
												mode:NSEventTrackingRunLoopMode
		handler:^(NSEvent* nextEvent, BOOL* outStopFlagPtr)
		{
			if (nil == nextEvent)
			{
				*outStopFlagPtr = YES;
				return/* from block */;
			}
			
			switch (nextEvent.type)
			{
			case NSEventTypeLeftMouseDragged:
				{
					NSPoint				newWindowLocation = [nextEvent locationInWindow];
					NSPoint				newViewLocation = [weakSelf convertPoint:newWindowLocation fromView:nil];
					TerminalView_Cell	mouseOverCell;
					
					
					// constrain range (not clear if this is needed)
					if (newViewLocation.x < 0)
					{
						newViewLocation.x = 0;
					}
					if (newViewLocation.y < 0)
					{
						newViewLocation.y = 0;
					}
					if (newViewLocation.x >= NSWidth(self.frame))
					{
						newViewLocation.x = (NSWidth(self.frame) - 1);
					}
					if (newViewLocation.y >= NSHeight(self.frame))
					{
						newViewLocation.y = (NSHeight(self.frame) - 1);
					}
					
					// find terminal cells, extend selection
					if (findVirtualCellFromScreenPoint(viewPtr, CGPointMake(newViewLocation.x, newViewLocation.y),
														mouseOverCell))
					{
						updateSelectionBlock(mouseOverCell);
					}
				}
				break;
			
			case NSEventTypeFlagsChanged:
				// modifier keys changed (e.g. change selection type to rectangular/not)
				if (nextEvent.modifierFlags & NSEventModifierFlagOption)
				{
					// rectangular selection shape
					if (true != viewPtr->text.selection.isRectangular)
					{
						highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
						viewPtr->text.selection.isRectangular = true;
						highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
					}
				}
				else
				{
					// normal selection shape
					if (false != viewPtr->text.selection.isRectangular)
					{
						highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
						viewPtr->text.selection.isRectangular = false;
						highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
					}
				}
				break;
			
			case NSEventTypeLeftMouseUp:
				*outStopFlagPtr = YES;
				copySelectedTextIfUserPreference(viewPtr);
				break;
			
			default:
				// ???
				Console_Warning(Console_WriteValue, "unexpected event received; ending terminal drag loop", nextEvent.type);
				*outStopFlagPtr = YES;
				break;
			}
		}];
	}
	
	//NSLog(@"terminal mouse down ended"); // debug
}// mouseDown:


/*!
Implements the “focus follows mouse” preference by making
the window the key window when the mouse moves inside it.

This is suppressed in certain situations that would lead
to a bad user experience (e.g. where losing focus may
cause a window to disappear).

See also "mouseExited:" and properties of type "NSTrackingArea".

(2021.08)
*/
- (void)
mouseEntered:(NSEvent*)		anEvent
{
	if (anEvent.trackingArea == self.focusFollowsMouseTrackingArea)
	{
		//NSLog(@"focus following mouse into: %@", self); // debug
		if ((NO == NSApp.keyWindow.isModalPanel) &&
			(self.window != NSApp.keyWindow.parentWindow)/* e.g. when a Find panel is open */ &&
			(self.window != NSApp.keyWindow.parentWindow.parentWindow)/* e.g. “Custom New Session” scenario when remote server popover is also visible */ &&
			((CFAbsoluteTimeGetCurrent() - self.menuEndTrackingTime) > 1.0/* arbitrary */))
		{
			[self.window makeKeyWindow];
			UNUSED_RETURN(BOOL)[self.window makeFirstResponder:self];
		}
	}
}// mouseEntered:


/*!
Currently this method does nothing because “focus follows mouse”
can be implemented entirely by "mouseEntered:".

(2021.08)
*/
- (void)
mouseExited:(NSEvent*)		anEvent
{
	if (anEvent.trackingArea == self.focusFollowsMouseTrackingArea)
	{
		//NSLog(@"focus following mouse away from: %@", self); // debug
	}
}// mouseExited:


/*!
Obtains data from a Services item.

Arbitrarily, it is assumed that text returned from a
Service should be sent to the active session (since
text in the terminal cannot be “replaced” like it
would be in a document).

NOTE: Both "writeSelectionToPasteboard:types:" and
"validRequestorForSendType:returnType:" should be
consistent with this.

(2020.09)
*/
- (BOOL)
readSelectionFromPasteboard:(NSPasteboard*)		aPasteboard
{
	BOOL		result = NO;
	NSArray*	availableTypes = [aPasteboard types];
	
	
	if ([availableTypes containsObject:NSPasteboardTypeString])
	{
		NSString*	availableText = [aPasteboard stringForType:NSPasteboardTypeString];
		
		
		if (nil == availableText)
		{
			// this should not happen...
			Console_Warning(Console_WriteLine, "failed to use Service; expected text to be available but string was not returned!");
		}
		else
		{
			// it is not technically possible to modify text in the terminal but
			// a close approximation is to send whatever string is provided...
		#if 1
			// friendlier: auto-Copy, then Paste (guarded for multi-line)
			Boolean		didCopy = Clipboard_AddCFStringToPasteboard
									(BRIDGE_CAST(availableText, CFStringRef), nil/* nil for main pasteboard */, true/* clear first */);
			
			
			if (false == didCopy)
			{
				Console_Warning(Console_WriteLine, "failed to use Service; unable to Copy");
			}
			else
			{
				Session_Result		pasteResult = Session_UserInputPaste([self boundSession]);
				
				
				if (kSession_ResultOK != pasteResult)
				{
					Console_Warning(Console_WriteValue, "failed to use Service; unable to Paste, error", pasteResult.code());
				}
			}
		#else
			// send text as-is; this is too abrupt...user cannot predict what is sent
			Session_UserInputCFString([self boundSession], BRIDGE_CAST(availableText, CFStringRef));
		#endif
			result = YES;
		}
	}
	
	return result;
}// readSelectionFromPasteboard:


/*!
Called as part of supporting the Services menu (system-wide);
communicates the currently-selected text to the system.

NOTE: Both "writeSelectionToPasteboard:types:" and
"readSelectionFromPasteboard:" should be consistent with this.

(2020.09)
*/
- (id)
validRequestorForSendType:(NSPasteboardType)	sendType
returnType:(NSPasteboardType)					returnType
{
	id		result = nil;
	
	
	if ([sendType isEqual:NSPasteboardTypeString] && [returnType isEqual:NSPasteboardTypeString])
	{
		Boolean const	isSelectedText = TerminalView_TextSelectionExists([self terminalViewRef]);
		
		
		if (isSelectedText)
		{
			result = self;
		}
	}
	else
	{
		result = [super validRequestorForSendType:sendType returnType:returnType];
	}
	
	return result;
}// validRequestorForSendType:returnType:


/*!
Provides data to a Services item.  Currently works for any text
selected in the terminal view.  (At a future time, this could
be extended to handle selected images as well.)

NOTE: Both "validRequestorForSendType:returnType:" and
"readSelectionFromPasteboard:" should be consistent with this.

(2020.09)
*/
- (BOOL)
writeSelectionToPasteboard:(NSPasteboard*)		aPasteboard
types:(NSArray*)								aTypeArray
{
	BOOL	result = NO;
	
	
	if (NO == [aTypeArray containsObject:NSPasteboardTypeString])
	{
		UInt16 const					spacesPerTab = 0;
		TerminalView_TextFlags const	optionFlags = 0;
		CFRetainRelease					selectedTextCFString(TerminalView_ReturnSelectedTextCopyAsUnicode
																([self terminalViewRef], spacesPerTab, optionFlags),
																CFRetainRelease::kAlreadyRetained);
		
		
		result = [aPasteboard setString:BRIDGE_CAST(selectedTextCFString.returnCFStringRef(), NSString*)
										forType:NSPasteboardTypeString];
	}
	
	return result;
}// writeSelectionToPasteboard:types:


#pragma mark NSStandardKeyBindingResponding


/*!
Sends an arrow sequence to the active session.

(2019.05)
*/
- (void)
moveDown:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveDown:


/*!
Locally extends the selection, creating a new selection at the
cursor location if necessary.  Typically this means the Shift
key has been held down in combination with an arrow.

(2019.04)
*/
- (void)
moveDownAndModifySelection:(id)		sender
{
	[self extendSelectionForKeyBindingSelector:_cmd sender:sender];
}// moveDownAndModifySelection:


/*!
Sends an arrow sequence to the active session.

(2019.05)
*/
- (void)
moveLeft:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveLeft:


/*!
Locally extends the selection, creating a new selection at the
cursor location if necessary.  Typically this means the Shift
key has been held down in combination with an arrow.

(2019.04)
*/
- (void)
moveLeftAndModifySelection:(id)		sender
{
	[self extendSelectionForKeyBindingSelector:_cmd sender:sender];
}// moveLeftAndModifySelection:


/*!
Sends an arrow sequence to the active session.

(2019.05)
*/
- (void)
moveRight:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveRight:


/*!
Sends the given selection-changing action to the attached
terminal session, if any.  Typically this means the Shift
key has been held down in combination with an arrow.

(2019.04)
*/
- (void)
moveRightAndModifySelection:(id)	sender
{
	[self extendSelectionForKeyBindingSelector:_cmd sender:sender];
}// moveRightAndModifySelection:


/*!
Sends a command-arrow sequence to the active session.

(2020.04)
*/
- (void)
moveToBeginningOfDocument:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveToBeginningOfDocument:


/*!
Sends a command-arrow sequence to the active session.

(2020.04)
*/
- (void)
moveToEndOfDocument:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveToEndOfDocument:


/*!
Sends a command-arrow sequence to the active session.

(2020.04)
*/
- (void)
moveToLeftEndOfLine:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveToLeftEndOfLine:


/*!
Locally extends the selection, creating a new selection at the
cursor location if necessary.  Typically this means the Shift
and Command keys have been held down in combination with an arrow.

(2019.04)
*/
- (void)
moveToLeftEndOfLineAndModifySelection:(id)		sender
{
	[self extendSelectionForKeyBindingSelector:_cmd sender:sender];
}// moveToLeftEndOfLineAndModifySelection:


/*!
Sends a command-arrow sequence to the active session.

(2020.04)
*/
- (void)
moveToRightEndOfLine:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveToRightEndOfLine:


/*!
Locally extends the selection, creating a new selection at the
cursor location if necessary.  Typically this means the Shift
and Command keys have been held down in combination with an arrow.

(2019.04)
*/
- (void)
moveToRightEndOfLineAndModifySelection:(id)		sender
{
	[self extendSelectionForKeyBindingSelector:_cmd sender:sender];
}// moveToRightEndOfLineAndModifySelection:


/*!
Locally extends the selection, creating a new selection at the
cursor location if necessary.  Typically this means the Shift
key has been held down in combination with an arrow.

(2019.04)
*/
- (void)
moveUpAndModifySelection:(id)	sender
{
	[self extendSelectionForKeyBindingSelector:_cmd sender:sender];
}// moveUpAndModifySelection:


/*!
Sends an arrow sequence to the active session.

(2019.05)
*/
- (void)
moveUp:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveUp:


/*!
Sends an option-arrow sequence to the active session.

(2020.04)
*/
- (void)
moveWordLeft:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveWordLeft:


/*!
Sends an option-arrow sequence to the active session.

(2020.04)
*/
- (void)
moveWordRight:(id)	sender
{
	[self moveCursorForKeyBindingSelector:_cmd sender:sender];
}// moveWordRight:


#pragma mark NSTextInputClient


/*!
Sends the given text to the attached terminal session, if any.

(2018.05)
*/
- (void)
insertText:(id)					aString
replacementRange:(NSRange)		replacementRange
{
#pragma unused(replacementRange)
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	// INCOMPLETE; it is not clear how to handle a random range request
	// from the caller (could attempt to determine the target location
	// relative to the terminal cursor and issue appropriate cursor move
	// commands beforehand but realistically a request for a random range
	// makes no sense in the terminal text entry model)
	
	//NSLog(@"term view %p / %@", viewPtr, NSStringFromSelector(_cmd)); // debug
	if (nullptr != viewPtr)
	{
		NSString*	asString = aString;
		
		
		if ([asString isKindOfClass:NSAttributedString.class])
		{
			asString = STATIC_CAST(aString, NSAttributedString*).string;
		}
		[self.textInputDelegate receivedString:asString terminalView:viewPtr->selfRef];
	}
	else
	{
		Sound_StandardAlert();
	}
}// insertText:replacementRange:


/*!
Performs the specified text-related action.  For example, this is
how newlines are detected as separate events from ordinary strings
(where the latter would be sent to "insertText:replacementRange:"
instead).

Bare Tab key presses are sent as text because they are used by
terminal applications such as shells.  Keyboard focus can still be
changed, going backward, by using Shift-Tab.

(2018.05)
*/
- (void)
doCommandBySelector:(SEL)	aSelector
{
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	//NSLog(@"term view '%@' asked to '%@'", NSStringFromSelector(_cmd), NSStringFromSelector(aSelector)); // debug
	if (@selector(cancel:) == aSelector)
	{
		// ignore redundant command
	}
	else if (@selector(cancelOperation:) == aSelector)
	{
		// Escape key; should be sent to session
		if (nil != self.textInputDelegate)
		{
			if (nullptr != viewPtr)
			{
				//[self.textInputDelegate receivedString:@"\033" terminalView:viewPtr->selfRef];
				[self.textInputDelegate receivedControlWithCharacter:'[' terminalView:viewPtr->selfRef];
			}
			else
			{
				Sound_StandardAlert();
			}
		}
		else
		{
			Sound_StandardAlert();
		}
	}
	else if (@selector(deleteBackward:) == aSelector)
	{
		// send appropriate delete sequence to session (varies)
		if (nullptr != viewPtr)
		{
			[self.textInputDelegate receivedDeleteBackwardInTerminalView:viewPtr->selfRef];
		}
		else
		{
			Sound_StandardAlert();
		}
	}
	else if (@selector(deleteWordBackward:) == aSelector)
	{
		// send appropriate delete sequence to session (varies)
		if (nullptr != viewPtr)
		{
			[self.textInputDelegate receivedDeleteWordBackwardInTerminalView:viewPtr->selfRef];
		}
		else
		{
			Sound_StandardAlert();
		}
	}
	else if (@selector(insertNewline:) == aSelector)
	{
		// send appropriate newline sequence to session (varies)
		if (nullptr != viewPtr)
		{
			[self.textInputDelegate receivedNewlineInTerminalView:viewPtr->selfRef];
		}
		else
		{
			Sound_StandardAlert();
		}
	}
	else if (@selector(insertTab:) == aSelector)
	{
		// send tab to session
		if (nil != self.textInputDelegate)
		{
			if (nullptr != viewPtr)
			{
				[self.textInputDelegate receivedString:@"\t" terminalView:viewPtr->selfRef];
			}
			else
			{
				Sound_StandardAlert();
			}
		}
		else
		{
			// treat as a keyboard focus change
			if ([self.window.firstResponder isKindOfClass:NSView.class])
			{
				NSView*		keyView = STATIC_CAST(self.window.firstResponder, NSView*);
				
				
				[self.window selectKeyViewFollowingView:keyView];
			}
		}
	}
	else if (@selector(insertBacktab:) == aSelector)
	{
		// treat as a keyboard focus change
		if ([self.window.firstResponder isKindOfClass:NSView.class])
		{
			NSView*		keyView = STATIC_CAST(self.window.firstResponder, NSView*);
			
			
			[self.window selectKeyViewPrecedingView:keyView];
		}
	}
	else
	{
		// NOTE: documentation specifies that "super" should not be called
		// as a fallback here but non-text keys like arrows do not work
		// unless "super" is called...
		//NSLog(@"using default implementation for key command %@", NSStringFromSelector(aSelector)); // debug
		[super doCommandBySelector:aSelector];
	}
}// doCommandBySelector:


/*!
NOTE: Marking is not currently implemented.

(2018.05)
*/
- (void)
setMarkedText:(id)			aString
selectedRange:(NSRange)		selectedRange
replacementRange:(NSRange)	replacementRange
{
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
}// setMarkedText:selectedRange:replacementRange:


/*!
NOTE: Marking is not currently implemented.

(2018.05)
*/
- (void)
unmarkText
{
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
}// unmarkText


/*!
NOTE: This method of determining the selected range is not
implemented.  Legacy code handles text selections through
different functions.

(2018.05)
*/
- (NSRange)
selectedRange
{
	NSRange		result = NSMakeRange(NSNotFound, 0);
	
	
	// UNIMPLEMENTED
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
	
	return result;
}// selectedRange


/*!
NOTE: Marking is not currently implemented.

(2018.05)
*/
- (NSRange)
markedRange
{
	NSRange		result = NSMakeRange(NSNotFound, 0);
	
	
	// UNIMPLEMENTED
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
	
	return result;
}// markedRange


/*!
Equivalent to calling "markedRange" and checking for a
nonzero length.

(2018.05)
*/
- (BOOL)
hasMarkedText
{
	BOOL	result = ([self markedRange].length > 0);
	
	
	return result;
}// hasMarkedText


/*!
NOTE: Not implemented.

(2018.05)
*/
- (NSAttributedString*)
attributedSubstringForProposedRange:(NSRange)	aRange
actualRange:(NSRangePointer)					actualRange
{
	NSAttributedString*		result = nil;
	
	
	// UNIMPLEMENTED
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
	
	return result;
}// attributedSubstringForProposedRange:actualRange:


/*!
NOTE: Marking is not currently implemented.

(2018.05)
*/
- (NSArray*)
validAttributesForMarkedText
{
	NSArray*	result = nil;
	
	
	// UNIMPLEMENTED
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
	
	return result;
}// validAttributesForMarkedText


/*!
NOTE: Rectangle computation is not supported through this
interface.

(2018.05)
*/
- (NSRect)
firstRectForCharacterRange:(NSRange)	aRange
actualRange:(NSRangePointer)			actualRange
{
	NSRect		result = NSZeroRect;
	
	
	// UNIMPLEMENTED
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
	
	return result;
}// firstRectForCharacterRange:actualRange:


/*!
NOTE: Character indexing is not supported through this
interface.

(2018.05)
*/
- (NSUInteger)
characterIndexForPoint:(NSPoint)	aPoint
{
	NSUInteger		result = 0;
	
	
	// UNIMPLEMENTED
	//NSLog(@"term view %@", NSStringFromSelector(_cmd)); // debug
	
	return result;
}// characterIndexForPoint:


#pragma mark NSUserInterfaceValidations


/*!
The standard Cocoa interface for validating things like menu
commands.  This method is present here because it must be in
the same class as the methods that perform actions; if the
action methods move, their validation code must move as well.

Returns true only if the specified item should be available
to the user.

(4.0)
*/
- (BOOL)
validateUserInterfaceItem:(id <NSObject, NSValidatedUserInterfaceItem>)		anItem
{
	// the call below enables the magic that allows validation to be automatic
	// whenever a "canPerform..." method for a "perform..." action also exists
	BOOL	result = [[Commands_Executor sharedExecutor] validateAction:anItem.action sender:self sourceItem:anItem];
	
	
	return result;
}// validateUserInterfaceItem:


#pragma mark NSView


/*!
Supports initiation of text drags while a terminal is inactive
by returning YES to accept mouse clicks while the window is
not active.

See also "shouldDelayWindowOrderingForEvent:" and "mouseDown:".

(2020.04)
*/
- (BOOL)
acceptsFirstMouse:(NSEvent*)	anEvent
{
	BOOL	result = NO;
	
	
	if ((NSEventTypeLeftMouseDown == anEvent.type) && (1 == anEvent.clickCount))
	{
		My_TerminalViewPtr	viewPtr = self.internalViewPtr;
		NSPoint				windowLocation = [anEvent locationInWindow];
		NSPoint				viewLocation = [self convertPoint:windowLocation fromView:nil];
		TerminalView_Cell	mouseDownCell;
		
		
		UNUSED_RETURN(BOOL)findVirtualCellFromScreenPoint(viewPtr, CGPointMake(viewLocation.x, viewLocation.y),
															mouseDownCell);
		if (pointInSelection(viewPtr, mouseDownCell))
		{
			result = YES;
		}
	}
	
	return result;
}// acceptsFirstMouse:


/*!
Prevents rendering of a focus ring for read-only terminals.

See also "acceptsFirstResponder".

(2018.12)
*/
- (BOOL)
canBecomeKeyView
{
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	BOOL					result = YES;
	
	
	if (nullptr != viewPtr)
	{
		result = (false == viewPtr->text.selection.readOnly);
	}
	
	return result;
}// canBecomeKeyView


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked by the system only on OS 10.7 or later, and
		less often than one might think.

(2018.12)
*/
- (void)
drawFocusRingMask
{
	// IMPORTANT: make consistent with "focusRingMaskBounds";
	// fill the exact shape of the focus ring however (not just
	// the boundaries of it); the settings below are meant to
	// match the current system appearance/layout (as such, in
	// the future they may require adjustment)
	NSRect		focusRingDrawingBounds = [self focusRingMaskBounds];
	
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithRoundedRect:focusRingDrawingBounds xRadius:3 yRadius:3] fill];
}// drawFocusRingMask


/*!
Render the specified part of the terminal screen.

(4.0)
*/
- (void)
drawRect:(NSRect)	aRect
{
#pragma unused(aRect)
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	if (nullptr != viewPtr)
	{
		CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
		CGRect			entireRect = NSRectToCGRect([self bounds]);
		CGRect			clipBounds = CGContextGetClipBoundingBox(drawingContext);
		CGRect			contentBounds = CGRectMake(CGRectGetMinX(entireRect), CGRectGetMinY(entireRect),
													CGRectGetWidth(entireRect),
													CGRectGetHeight(entireRect));
		
		
		// INCOMPLETE!
		
		// when removing a highlight, this flag should be set earlier (to ensure
		// the normal background is rendered)
		if (NO == self.showDragHighlight)
		{
			viewPtr->screen.currentRenderDragColors = false;
		}
		
		// draw default background
		useTerminalTextColors(viewPtr, drawingContext, viewPtr->text.attributes,
								false/* is cursor */, 1.0/* alpha */);
		//CGContextSetAllowsAntialiasing(drawingContext, false);
		CGContextFillRect(drawingContext, clipBounds);
		//CGContextSetAllowsAntialiasing(drawingContext, true);
		
		// perform any necessary rendering for drags
		if (self.showDragHighlight)
		{
			DragAndDrop_ShowHighlightBackground(drawingContext, contentBounds);
			
			// when drawing a new highlight, this flag should not be set above (to ensure
			// normal background is drawn behind) but set before text is overlaid below
			// (to ensure the text becomes black, on top)
			viewPtr->screen.currentRenderDragColors = true;
			
			// (frame is drawn at the end, after any content)
		}
		
		// draw text and graphics
		{
			// draw only the requested area; convert from pixels to screen cells
			HIPoint const&		kTopLeftAnchor = clipBounds.origin;
			HIPoint const		kBottomRightAnchor = CGPointMake(clipBounds.origin.x + clipBounds.size.width,
																	clipBounds.origin.y + clipBounds.size.height);
			TerminalView_Cell	leftTopCell;
			TerminalView_Cell	rightBottomCell;
			
			
			// figure out what cells to draw
			UNUSED_RETURN(Boolean)findVirtualCellFromScreenPoint(viewPtr, kTopLeftAnchor, leftTopCell);
			UNUSED_RETURN(Boolean)findVirtualCellFromScreenPoint(viewPtr, kBottomRightAnchor, rightBottomCell);
			
			// draw the text in the clipped area
			UNUSED_RETURN(Boolean)drawSection(viewPtr, drawingContext, leftTopCell.first - viewPtr->screen.leftVisibleEdgeInColumns,
												leftTopCell.second - viewPtr->screen.topVisibleEdgeInRows,
												rightBottomCell.first + 1/* past-the-end */ - viewPtr->screen.leftVisibleEdgeInColumns,
												rightBottomCell.second + 1/* past-the-end */ - viewPtr->screen.topVisibleEdgeInRows);
		}
		viewPtr->text.attributes = kTextAttributes_Invalid; // forces attributes to reset themselves properly
		
		// if, after drawing all text, no text is actually blinking,
		// then disable the animation timer (so unnecessary refreshes
		// are not done); otherwise, install it
		setBlinkingTimerActive(viewPtr, (viewPtr->isActive) && (cursorBlinks(viewPtr) ||
																(viewPtr->screen.currentRenderBlinking)));
		
		// if inactive, render the text selection as an outline
		// (if active, the call above to draw the text will also
		// have drawn the active selection)
		if ((false == viewPtr->isActive) && selectionExists(viewPtr))
		{
			HIShapeRef		selectionShape = getSelectedTextAsNewHIShape(viewPtr, 1.5/* inset */);
			
			
			if (nullptr != selectionShape)
			{
				// use selection colors
				NSColor*			highlightColorRGB = [[NSColor selectedTextBackgroundColor]
															colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
				CGFloatRGBColor		highlightColorDevice;
				
				
				// draw outline
				CGContextSetLineWidth(drawingContext, 2.0); // make thick outline frame on Mac OS X
				CGContextSetLineCap(drawingContext, kCGLineCapRound);
				highlightColorDevice.red = [highlightColorRGB redComponent];
				highlightColorDevice.green = [highlightColorRGB greenComponent];
				highlightColorDevice.blue = [highlightColorRGB blueComponent];
				CGContextSetRGBStrokeColor(drawingContext, highlightColorDevice.red, highlightColorDevice.green,
											highlightColorDevice.blue, 1.0/* alpha */);
				UNUSED_RETURN(OSStatus)HIShapeReplacePathInCGContext(selectionShape, drawingContext);
				CGContextStrokePath(drawingContext);
				
				// free allocated memory
				CFRelease(selectionShape), selectionShape = nullptr;
			}
		}
		
		// perform any necessary rendering for drags
		if (self.showDragHighlight)
		{
			DragAndDrop_ShowHighlightFrame(drawingContext, contentBounds);
		}
		
		// render margin line, if requested
		if (gPreferenceProxies.renderMarginAtColumn > 0)
		{
			CGRect				dummyBounds;
			NSColor*			highlightColorRGB = [[NSColor selectedTextBackgroundColor]
														colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
			CGFloatRGBColor		highlightColorDevice;
			
			
			getRowSectionBounds(viewPtr, 0/* row; does not matter which */,
								gPreferenceProxies.renderMarginAtColumn/* zero-based column number, but following column is desired */,
								1/* character count - not important */, dummyBounds);
			CGContextSetLineWidth(drawingContext, 2.0);
			CGContextSetLineCap(drawingContext, kCGLineCapButt);
			highlightColorDevice.red = [highlightColorRGB redComponent];
			highlightColorDevice.green = [highlightColorRGB greenComponent];
			highlightColorDevice.blue = [highlightColorRGB blueComponent];
			CGContextSetRGBStrokeColor(drawingContext, highlightColorDevice.red, highlightColorDevice.green,
										highlightColorDevice.blue, 1.0/* alpha */);
			CGContextBeginPath(drawingContext);
			CGContextMoveToPoint(drawingContext, dummyBounds.origin.x, contentBounds.origin.y);
			CGContextAddLineToPoint(drawingContext, dummyBounds.origin.x, contentBounds.origin.y + contentBounds.size.height);
			CGContextStrokePath(drawingContext);
		}
		
		// draw cursor
		if (kMyCursorStateVisible == viewPtr->screen.cursor.currentState)
		{
			CGContextSaveRestore		_(drawingContext);
			TextAttributes_Object		cursorAttributes = Terminal_CursorReturnAttributes(viewPtr->screen.ref);
			CGRect						cursorFloatBounds = viewPtr->screen.cursor.bounds;
			
			
			// flip colors and paint at the current blink alpha value
			if (cursorAttributes.hasAttributes(kTextAttributes_StyleInverse))
			{
				cursorAttributes.removeAttributes(kTextAttributes_StyleInverse);
			}
			else
			{
				cursorAttributes.addAttributes(kTextAttributes_StyleInverse);
			}
			useTerminalTextColors(viewPtr, drawingContext, cursorAttributes,
									true/* is cursor */,
									cursorBlinks(viewPtr)
									? viewPtr->animation.cursor.blinkAlpha
									: 0.8f/* arbitrary, but it should be possible to see characters underneath a block shape */);
			CGContextFillRect(drawingContext, CGRectIntegral(cursorFloatBounds));
			
			// if the terminal is currently in password mode, annotate the cursor
			if (Terminal_IsInPasswordMode(viewPtr->screen.ref))
			{
				CGFloat const	newHeight = viewPtr->text.font.heightPerCell.precisePixels(); // TEMPORARY; does not handle double-height lines (probably does not need to)
				CGFloat			dotDimensions = STATIC_CAST(ceil(viewPtr->text.font.heightPerCell.precisePixels() * 0.33/* arbitrary */), CGFloat);
				CGRect			fullRectangleBounds = cursorFloatBounds;
				CGRect			dotBounds = CGRectMake(0, 0, dotDimensions, dotDimensions); // arbitrary
				
				
				// the cursor may not be block-size so convert to a block rectangle
				// before determining the placement of the oval inset
				fullRectangleBounds.origin.y = fullRectangleBounds.origin.y + fullRectangleBounds.size.height - newHeight;
				fullRectangleBounds.size.width = viewPtr->text.font.widthPerCell.precisePixels();
				fullRectangleBounds.size.height = newHeight;
				RegionUtilities_CenterCGRectIn(dotBounds, fullRectangleBounds);
				if (cursorAttributes.hasDoubleWidth())
				{
					dotBounds.origin.x += (viewPtr->text.font.heightPerCell.precisePixels() * 0.3/* arbitrary */);
				}
				
				// draw the dot in the middle of the cell that the cursor occupies
				if (kTerminal_CursorTypeBlock == gPreferenceProxies.cursorType)
				{
					// since the disk will not be visible with a full block shape, first
					// restore original attributes (that way the disk appears in the
					// color of the background, not the color of the cursor)
					cursorAttributes = Terminal_CursorReturnAttributes(viewPtr->screen.ref);
					useTerminalTextColors(viewPtr, drawingContext, cursorAttributes,
											true/* is cursor */,
											cursorBlinks(viewPtr)
											? viewPtr->animation.cursor.blinkAlpha
											: 0.8f/* arbitrary, but it should be possible to see characters underneath a block shape */);
				}
				CGContextFillEllipseInRect(drawingContext, dotBounds);
			}
		}
		
		if (kMyCursorStateVisible == viewPtr->screen.cursor.ghostState)
		{
			// render cursor ghost
			// UNIMPLEMENTED
		}
		
		// debug: show the update region as a solid color
	#if 0
		{
			static UInt16		colorCounter = 0;
			CGFloatRGBColor		debugColor = { 0.0, 0.0, 0.0 };
			
			
			// rotate through a few different colors so that
			// adjacent updates in different areas are more likely
			// to be seen as distinct
			++colorCounter;
			if (1 == colorCounter)
			{
				debugColor = { 1.0, 0.0, 0.0 };
			}
			else if (2 == colorCounter)
			{
				debugColor = { 0.0, 1.0, 0.0 };
			}
			else if (3 == colorCounter)
			{
				debugColor = { 1.0, 1.0, 0.0 };
			}
			else if (4 == colorCounter)
			{
				debugColor = { 0.0, 0.0, 1.0 };
			}
			else if (5 == colorCounter)
			{
				debugColor = { 0.5, 0.5, 0.5 };
			}
			else
			{
				colorCounter = 0;
			}
			
			if (0 != colorCounter)
			{
				CGContextSetRGBFillColor(drawingContext, debugColor.red, debugColor.green,
											debugColor.blue, 1.0/* alpha */);
				CGContextSetAllowsAntialiasing(drawingContext, false);
				CGContextFillRect(drawingContext, clipBounds);
				CGContextSetAllowsAntialiasing(drawingContext, true);
			}
		}
	#endif
	}
}// drawRect:


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked only on OS 10.7 or later, and less often
		than one might think.

(2018.12)
*/
- (NSRect)
focusRingMaskBounds
{
	// IMPORTANT: make consistent with "drawFocusRingMask"
	// (TEMPORARY; this border layout does not currently read the actual
	// offsets around the terminal interior so it could become out of sync)
	// NOTE: the system seems to ignore boundary outsets if they are large
	NSRect		result = NSInsetRect(self.bounds, -5, -4);
	
	
	return result;
}// focusRingMaskBounds


/*!
Returns YES only if the view’s coordinate system uses
a top-left origin.

(4.0)
*/
- (BOOL)
isFlipped
{
	// since drawing code is originally from Carbon, keep the view
	// flipped for the time being
	return YES;
}// isFlipped


/*!
Returns YES only if the view has no transparent parts.

(4.0)
*/
- (BOOL)
isOpaque
{
	return NO;
}// isOpaque


/*!
Returns a contextual menu for the terminal view.

(2018.02)
*/
- (NSMenu*)
menuForEvent:(NSEvent*)		anEvent
{
#pragma unused(anEvent)
	NSMenu*					result = nil;
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	if (nullptr != viewPtr)
	{
		// display a contextual menu
		result = [[NSMenu alloc] initWithTitle:@""];
		
		// set up the contextual menu
		//[result setAllowsContextMenuPlugIns:NO];
		populateContextualMenu(viewPtr, result);
	}
	
	return result;
}// menuForEvent:


/*!
In addition to "acceptsFirstResponder" returning YES (a method
from NSResponder), this method (from NSView) requires that the
owning window be the key window before keyboard input is
accepted.

(2018.05)
*/
- (BOOL)
needsPanelToBecomeKey
{
	return YES;
}// needsPanelToBecomeKey


/*!
Invoked by NSView whenever it’s necessary to define the regions
that change the mouse pointer’s shape.

(4.1)
*/
- (void)
resetCursorRects
{
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	// note: now that mouse pointer color is customizable, the pointer is
	// displayed on all terminals (even read-only/sample ones) so the
	// user can see what they look like...
#if 0
	if ((nullptr != viewPtr) && (viewPtr->text.selection.readOnly))
	{
		// the user cannot interact with the terminal view so it is
		// inappropriate to display any special cursor shapes over it
		[self addCursorRect:[self bounds] cursor:[NSCursor arrowCursor]];
	}
	else
#endif
	{
		// the cursor varies based on the state of modifier keys
		if (self.modifierFlagsForCursor & NSEventModifierFlagControl)
		{
			// modifier key for contextual menu
			[self addCursorRect:[self bounds] cursor:[NSCursor contextualMenuCursor]];
		}
		else if ((self.modifierFlagsForCursor & NSEventModifierFlagCommand) &&
					(self.modifierFlagsForCursor & NSEventModifierFlagOption))
		{
			// modifier key for moving the terminal cursor to the click location
			[self addCursorRect:[self bounds] cursor:customCursorMoveTerminalCursor(viewPtr, isSmallIBeam(viewPtr))];
		}
		else if (self.modifierFlagsForCursor & NSEventModifierFlagCommand)
		{
			// modifier key for clicking a URL selection
			[self addCursorRect:[self bounds] cursor:[NSCursor pointingHandCursor]];
		}
		else if (self.modifierFlagsForCursor & NSEventModifierFlagOption)
		{
			// modifier key for rectangular text selections
			[self addCursorRect:[self bounds] cursor:customCursorCrosshairs(viewPtr)];
		}
		else
		{
			// normal cursor
			//[self addCursorRect:[self bounds] cursor:[NSCursor IBeamCursor]];
			[self addCursorRect:[self bounds] cursor:customCursorIBeam(viewPtr, isSmallIBeam(viewPtr))];
		}
		
		// INCOMPLETE; add support for any current text selection region
	}
}// resetCursorRects


/*!
Supports initiation of text drags while a terminal is inactive.

See also "acceptsFirstMouse:" and the "mouseDown:" handler.

(2020.04)
*/
- (BOOL)
shouldDelayWindowOrderingForEvent:(NSEvent*)	anEvent
{
	My_TerminalViewPtr	viewPtr = self.internalViewPtr;
	BOOL				result = NO;
	
	
	if ((NSEventTypeLeftMouseDown == anEvent.type) && (1 == anEvent.clickCount))
	{
		// note: "mouseDown:" should call "[NSApp preventWindowOrdering]" before drag
		if (NO == viewPtr->text.selection.readOnly)
		{
			result = YES;
		}
	}
	
	return result;
}// shouldDelayWindowOrderingForEvent:


/*!
Implements the “focus follows mouse” preference by maintaining
mouse-tracking areas that help to determine when the focused
window should be changed.

This is generally called when the window is opened or resized.

(2021.08)
*/
- (void)
updateTrackingAreas
{
	[super updateTrackingAreas];
	
	[self setFocusFollowsMouseTrackingAreasEnabled:self.focusShouldFollowMouse];
}// updateTrackingAreas


@end //} TerminalView_ContentView


#pragma mark -
@implementation TerminalView_ContentView (TerminalView_ContentViewInternal) //{


#pragma mark New Methods


/*!
Call this when given a single-button mouse-down event, to
briefly track the mouse and determine if it is a drag.

If YES is returned, it is “still” a mouse-down and a drag
should be initiated.  If NO, events have tracked a mouse-up
and this should be considered a single click.

(2020.04)
*/
- (BOOL)
assessIsDragForSingleButtonMouseDownEvent:(NSEvent*)	anEvent
{
	// possible drag (if mouse goes up without moving much; ignore)
	__weak decltype(self)	weakSelf = self;
	NSPoint					windowLocation = [anEvent locationInWindow];
	NSPoint					viewLocation = [self convertPoint:windowLocation fromView:nil];
	__block BOOL			result = NO;
	
	
	// briefly track the mouse to see if it moves far enough
	// to reasonably initiate a drag (if not, mouse-up will
	// cause text to be deselected)
	[self.window trackEventsMatchingMask:(NSEventMaskLeftMouseDragged |
											NSEventMaskLeftMouseUp)
											timeout:NSEventDurationForever
											mode:NSEventTrackingRunLoopMode
	handler:^(NSEvent* nextEvent, BOOL* outStopFlagPtr)
	{
		if (nil != nextEvent)
		{
			switch (nextEvent.type)
			{
			case NSEventTypeLeftMouseDragged:
				{
					NSPoint		newWindowLocation = [nextEvent locationInWindow];
					NSPoint		newViewLocation = [weakSelf convertPoint:newWindowLocation fromView:nil];
					
					
					// if the mouse moves “enough”, initiate a drag (otherwise, on next
					// mouse-up, it is treated as a click in the selection)
					if ((abs(newViewLocation.x - viewLocation.x) > 4.0/* arbitrary */) ||
						(abs(newViewLocation.y - viewLocation.y) > 4.0/* arbitrary */))
					{
						result = YES;
						*outStopFlagPtr = YES;
					}
				}
				break;
			
			case NSEventTypeLeftMouseUp:
				*outStopFlagPtr = YES;
				break;
			
			default:
				// ???
				Console_Warning(Console_WriteValue, "unexpected event received; ending terminal drag loop", nextEvent.type);
				*outStopFlagPtr = YES;
				break;
			}
		}
	}];
	
	return result;
}// assessIsDragForSingleButtonMouseDownEvent:


/*!
Return any listening session for this view, or nil.

(2018.10)
*/
- (SessionRef)
boundSession
{
	TerminalWindowRef	terminalWindow = [self.window terminalWindowRef];
	SessionRef			result = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	
	
	return result;
}// boundSession


/*!
Standard handler for "moveLeftAndModifySelection:",
"moveRightAndModifySelection:" and similar methods.
If no selection exists yet, it is started at the
terminal cursor location for right-extension or the
position before the cursor for left-extension.  The
initial direction away from the cursor position is
noted in order to control future modification of
the same selection (i.e. starting “left” means a
future “right” will shrink the left edge of the
selection, whereas starting “right” means a future
“left” will shrink the right edge of the selection).

See implementations of NSStandardKeyBindingResponding
methods above.

(2019.04)
*/
- (void)
extendSelectionForKeyBindingSelector:(SEL)	aSelector
sender:(id)									sender
{
#pragma unused(sender)
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	if (nullptr != viewPtr)
	{
		if (@selector(moveLeftAndModifySelection:) == aSelector)
		{
			selectionModify(viewPtr, -1, 0, kMy_SelectionExtensionTypeDelta, true/* allow scroll */);
		}
		else if (@selector(moveToLeftEndOfLineAndModifySelection:) == aSelector)
		{
			selectionModify(viewPtr, 0, 0, kMy_SelectionExtensionTypeLineStart, true/* allow scroll */);
		}
		else if (@selector(moveUpAndModifySelection:) == aSelector)
		{
			selectionModify(viewPtr, 0, -1, kMy_SelectionExtensionTypeDelta, true/* allow scroll */);
		}
		else if (@selector(moveDownAndModifySelection:) == aSelector)
		{
			selectionModify(viewPtr, 0, +1, kMy_SelectionExtensionTypeDelta, true/* allow scroll */);
		}
		else if (@selector(moveToRightEndOfLineAndModifySelection:) == aSelector)
		{
			selectionModify(viewPtr, 0, 0, kMy_SelectionExtensionTypeLineEnd, true/* allow scroll */);
		}
		else// if (@selector(moveRightAndModifySelection:) == aSelector)
		{
			selectionModify(viewPtr, +1, 0, kMy_SelectionExtensionTypeDelta, true/* allow scroll */);
		}
	}
}// extendSelectionForKeyBindingSelector:sender:


/*!
Called when a monitored preference is changed.  See the
initializer for the set of events that is monitored.

(2021.08)
*/
- (void)
model:(ListenerModel_Ref)				aModel
preferenceChange:(ListenerModel_Event)	anEvent
context:(void*)							aContext
{
#pragma unused(aModel, aContext)
	switch (anEvent)
	{
	case kPreferences_TagFocusFollowsMouse:
		{
			BOOL	newValue = false;
			
			
			unless (kPreferences_ResultOK == Preferences_GetData(kPreferences_TagFocusFollowsMouse, sizeof(newValue), &newValue))
			{
				newValue = false; // assume a value, if preference can’t be found
			}
			self.focusShouldFollowMouse = newValue;
			[self setFocusFollowsMouseTrackingAreasEnabled:newValue];
		}
		break;
	
	default:
		// ???
		break;
	}
}// model:preferenceChange:context:


/*!
Standard handler for "moveLeft:", "moveRight:" and
similar methods.

See implementations of NSStandardKeyBindingResponding
methods above.

(2019.05)
*/
- (void)
moveCursorForKeyBindingSelector:(SEL)	aSelector
sender:(id)								sender
{
#pragma unused(sender)
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	if (nullptr != viewPtr)
	{
		if (@selector(moveLeft:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSLT, 0/* modifier keys */);
		}
		else if (@selector(moveToLeftEndOfLine:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSLT, NSEventModifierFlagCommand);
		}
		else if (@selector(moveWordLeft:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSLT, NSEventModifierFlagOption);
		}
		else if (@selector(moveUp:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSUP, 0/* modifier keys */);
		}
		else if (@selector(moveDown:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSDN, 0/* modifier keys */);
		}
		else if (@selector(moveToBeginningOfDocument:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSUP, NSEventModifierFlagCommand);
		}
		else if (@selector(moveToEndOfDocument:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSDN, NSEventModifierFlagCommand);
		}
		else if (@selector(moveToRightEndOfLine:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSRT, NSEventModifierFlagCommand);
		}
		else if (@selector(moveWordRight:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSRT, NSEventModifierFlagOption);
		}
		else// if (@selector(moveRight:) == aSelector)
		{
			Session_UserInputKey(self.boundSession, VSRT, 0/* modifier keys */);
		}
	}
}// moveCursorForKeyBindingSelector:sender:


/*!
Implements both “print screen” and “print selection”.

(2020.01)
*/
- (void)
printForSelector:(SEL)	aSelector
sender:(id)				sender
{
#pragma unused(sender)
	TerminalScreenRef	screen = self.internalViewPtr->screen.ref;
	TerminalViewRef		view = [self terminalViewRef];
	BOOL				isPrintScreen = ((@selector(performPrintScreen:) == aSelector) ||
											(false == TerminalView_TextSelectionExists(view)));
	CFRetainRelease		jobTitle(((isPrintScreen)
									? UIStrings_ReturnCopy(kUIStrings_TerminalPrintScreenJobTitle)
									: UIStrings_ReturnCopy(kUIStrings_TerminalPrintSelectionJobTitle)),
									CFRetainRelease::kAlreadyRetained);
	
	
	if (jobTitle.exists())
	{
		PrintTerminal_JobRef	printJob = ((isPrintScreen)
											? PrintTerminal_NewJobFromVisibleScreen
												(view, screen, jobTitle.returnCFStringRef())
											: PrintTerminal_NewJobFromSelectedText
												(view, jobTitle.returnCFStringRef()));
		
		
		if (nullptr != printJob)
		{
			UNUSED_RETURN(PrintTerminal_Result)PrintTerminal_JobSendToPrinter
												(printJob, self.window);
		}
	}
}// printForSelector:sender:


/*!
Adds or removes mouse-tracking for the purposes of implementing
the “focus follows mouse” feature.

This is a helper method that should not be called arbitrarily.
See "updateTrackingAreas" and code that responds to changes in
user preferences ("model:preferenceChange:context:").

(2021.08)
*/
- (void)
setFocusFollowsMouseTrackingAreasEnabled:(BOOL)		aFlag
{
	if (nil != self.focusFollowsMouseTrackingArea)
	{
		[self removeTrackingArea:self.focusFollowsMouseTrackingArea];
		self.focusFollowsMouseTrackingArea = nil;
	}
	
	if (aFlag)
	{
		self.focusFollowsMouseTrackingArea = [[NSTrackingArea alloc]
												initWithRect:NSZeroRect
																options:(NSTrackingMouseEnteredAndExited | NSTrackingActiveInActiveApp | NSTrackingInVisibleRect)
																owner:self
																userInfo:nil];
		[self addTrackingArea:self.focusFollowsMouseTrackingArea];
	}
}// 


@end //} TerminalView_ContentView (TerminalView_ContentViewInternal)


#pragma mark -
@implementation TerminalView_Controller //{


#pragma mark Initializers


/*!
Designated initializer.

(2016.03)
*/
- (instancetype)
init
{
	self = [super initWithNibName:nil bundle:nil];
	if (nil != self)
	{
		Preferences_ContextWrap		terminalConfig(Preferences_NewContext(Quills::Prefs::TERMINAL),
													Preferences_ContextWrap::kAlreadyRetained);
		Preferences_ContextWrap		translationConfig(Preferences_NewContext(Quills::Prefs::TRANSLATION),
														Preferences_ContextWrap::kAlreadyRetained);
		auto						terminalView = [[TerminalView_Object alloc] initWithFrame:NSZeroRect];
		
		
		terminalView.layoutDelegate = self;
		
		@try
		{
			// could customize the new contexts above to initialize settings;
			// currently, this is not done
			{
				TerminalScreenRef		buffer = nullptr;
				Terminal_Result			bufferResult = Terminal_NewScreen(terminalConfig.returnRef(),
																			translationConfig.returnRef(), &buffer);
				
				
				if (kTerminal_ResultOK != bufferResult)
				{
					Console_WriteValue("error creating test terminal screen buffer", bufferResult);
				}
				else
				{
					TerminalViewRef		viewRef = TerminalView_NewNSViewBased(terminalView, buffer, nullptr/* format */);
					
					
					if (nullptr == viewRef)
					{
						Console_WriteLine("error creating test terminal view!");
					}
					else
					{
						// write some text in various styles to the screen (happens to be a
						// copy of what the sample view does); this will help with testing
						// the new Cocoa-based renderer as it is implemented
						Terminal_EmulatorProcessCString(buffer,
														"\033[2J\033[H"); // clear screen, home cursor (assumes VT100)
						Terminal_EmulatorProcessCString(buffer,
														"sel find norm \033[1mbold\033[0m \033[5mblink\033[0m \033[3mital\033[0m \033[7minv\033[0m \033[4munder\033[0m");
						// the range selected here should be as long as the length of the word “sel” above
						TerminalView_SelectVirtualRange(viewRef, std::make_pair(std::make_pair(0, 0), std::make_pair(3, 1)/* exclusive end */));
						// the range selected here should be as long as the length of the word “find” above
						TerminalView_FindVirtualRange(viewRef, std::make_pair(std::make_pair(4, 0), std::make_pair(8, 1)/* exclusive end */));
					}
				}
			}
		}
		@finally
		{
			terminalConfig.clear();
			translationConfig.clear();
		}
		
		// when no NIB is present, "view" must be assigned
		self.view = terminalView;
		assert(nil != self.terminalView);
	}
	return self;
}// init


#pragma mark Accessors


/*!
Returns the terminal view managed by this controller.
This is a convenience method for interpreting the
controller’s "view" property as a more precise class.

(2018.04)
*/
- (TerminalView_Object*)
terminalView
{
	TerminalView_Object*	result = nil;
	
	
	if (NO == [self.view isKindOfClass:TerminalView_Object.class])
	{
		Console_Warning(Console_WriteLine, "TerminalView_Controller: 'terminalView': 'self.view' is not of the expected class!");
	}
	else
	{
		result = STATIC_CAST(self.view, TerminalView_Object*);
	}
	
	return result;
}// terminalView


#pragma mark CoreUI_ViewLayoutDelegate


/*!
This is responsible for the layout of subviews.

(2018.04)
*/
- (void)
layoutDelegateForView:(NSView*)		aView
resizeSubviewsWithOldSize:(NSSize)	anOldSize
{
#pragma unused(aView, anOldSize)
	TerminalView_Object*			rootView = self.terminalView;
	My_TerminalViewPtr				viewPtr = rootView.internalViewPtr;
	TerminalView_BackgroundView*	marginViewB = rootView.terminalMarginViewBottom;
	TerminalView_BackgroundView*	marginViewL = rootView.terminalMarginViewLeft;
	TerminalView_BackgroundView*	marginViewR = rootView.terminalMarginViewRight;
	TerminalView_BackgroundView*	marginViewT = rootView.terminalMarginViewTop;
	TerminalView_BackgroundView*	padViewB = rootView.terminalPaddingViewBottom;
	TerminalView_BackgroundView*	padViewL = rootView.terminalPaddingViewLeft;
	TerminalView_BackgroundView*	padViewR = rootView.terminalPaddingViewRight;
	TerminalView_BackgroundView*	padViewT = rootView.terminalPaddingViewTop;
	CGFloat							widthPerCell = viewPtr->text.font.widthPerCell.precisePixels(); // “em” scale always multiplies against width
	CGFloat							marginH = (rootView.internalViewPtr->screen.marginLeftEmScale * widthPerCell);
	CGFloat							marginV = (rootView.internalViewPtr->screen.marginTopEmScale * widthPerCell);
	CGFloat							paddingH = (rootView.internalViewPtr->screen.paddingLeftEmScale * widthPerCell);
	CGFloat							paddingV = (rootView.internalViewPtr->screen.paddingTopEmScale * widthPerCell);
	
	
	//
	// NOTE: The layout performed below should also be reflected in
	// functions like TerminalView_GetIdealSize() that rely on the
	// same relative layout assumptions.
	//
	// The goal here is to avoid overlapping views by default and only
	// pay potential performance penalties when certain features are
	// enabled.  Also, separation of views makes it straightforward to
	// produce alternate layouts, e.g. hiding margins on one side so
	// that adjacent views can appear right next to one another.  The
	// top and bottom versions of a border extend horizontally so that
	// the layout looks like this:
	//         ._____________________________.
	//         |_____________________________|
	//         | |_________________________| |  
	//         | | |                     | | |
	//         | | |                     | | |
	//         | |_|_____________________|_| |
	//         |_|_________________________|_|
	//         |_____________________________|
	//
	
	// WARNING: these assignments are in order, depending on previous values
	marginViewB.frame = NSMakeRect(0, 0, NSWidth(rootView.frame), marginV);
	marginViewT.frame = NSMakeRect(0, NSHeight(rootView.frame) - NSHeight(marginViewB.frame),
									NSWidth(marginViewB.frame), NSHeight(marginViewB.frame));
	marginViewL.frame = NSMakeRect(0, NSHeight(marginViewB.frame),
									marginH,
									NSHeight(rootView.frame) - NSHeight(marginViewB.frame) - NSHeight(marginViewT.frame));
	marginViewR.frame = NSMakeRect(NSWidth(rootView.frame) - NSWidth(marginViewL.frame), marginViewL.frame.origin.y,
									NSWidth(marginViewL.frame), NSHeight(marginViewL.frame));
	padViewB.frame = NSMakeRect(marginViewL.frame.origin.x + NSWidth(marginViewL.frame),
								marginViewB.frame.origin.y + NSHeight(marginViewB.frame),
								NSWidth(marginViewB.frame) - (2.0 * NSWidth(marginViewL.frame)),
								paddingV);
	padViewT.frame = NSMakeRect(padViewB.frame.origin.x,
								NSHeight(rootView.frame) - NSHeight(marginViewT.frame) - NSHeight(padViewB.frame),
								NSWidth(padViewB.frame), NSHeight(padViewB.frame));
	padViewL.frame = NSMakeRect(padViewB.frame.origin.x, padViewB.frame.origin.y + NSHeight(padViewB.frame),
								paddingH,
								NSHeight(marginViewL.frame) - NSHeight(padViewB.frame) - NSHeight(padViewT.frame));
	padViewR.frame = NSMakeRect(marginViewR.frame.origin.x - NSWidth(padViewL.frame), padViewL.frame.origin.y,
								NSWidth(padViewL.frame), NSHeight(padViewL.frame));
	rootView.terminalContentView.frame = NSMakeRect(padViewL.frame.origin.x +
														NSWidth(padViewL.frame),
													padViewB.frame.origin.y +
														NSHeight(padViewB.frame),
													NSWidth(rootView.frame) -
														NSWidth(marginViewR.frame) -
														NSWidth(padViewR.frame) -
														NSWidth(padViewL.frame) -
														NSWidth(marginViewL.frame),
													NSHeight(rootView.frame) -
														NSHeight(marginViewB.frame) -
														NSHeight(padViewB.frame) -
														NSHeight(padViewT.frame) -
														NSHeight(marginViewT.frame));
}// layoutDelegateForView:resizeSubviewsWithOldSize:


#pragma mark NSViewController


/*!
As the view is added to a window, this installs handlers for
window notifications so that active appearance can be tracked.

(2020.04)
*/
- (void)
viewDidAppear
{
	[self whenObject:self.view.window postsNote:NSWindowDidBecomeKeyNotification
						performSelector:@selector(windowDidBecomeKey:)];
	[self whenObject:self.view.window postsNote:NSWindowDidResignKeyNotification
						performSelector:@selector(windowDidResignKey:)];
}// viewDidAppear


/*!
As the view is removed from a window, this removes handlers
installed by "viewDidAppear".

(2020.04)
*/
- (void)
viewWillDisappear
{
	[self ignoreWhenObject:self.view.window postsNote:NSWindowDidBecomeKeyNotification];
	[self ignoreWhenObject:self.view.window postsNote:NSWindowDidResignKeyNotification];
}// viewWillDisappear


#pragma mark NSWindowNotifications


/*!
Responds to window activation by restoring the frame colors
and installing cursor-tracking areas in resizable windows.

(2020.04)
*/
- (void)
windowDidBecomeKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.terminalView.internalViewPtr->isActive = YES;
	self.terminalView.terminalContentView.needsDisplay = YES;
}// windowDidBecomeKey:


/*!
Responds to window deactivation by graying the frame colors.

(2020.04)
*/
- (void)
windowDidResignKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.terminalView.internalViewPtr->isActive = NO;
	self.terminalView.terminalContentView.needsDisplay = YES;
}// windowDidResignKey:


@end //} TerminalView_Controller


#pragma mark -
@implementation TerminalView_Object //{


#pragma mark Initializers


/*!
Initializer for object constructed from a NIB.

(2018.04)
*/
- (instancetype)
initWithCoder:(NSCoder*)	aCoder
{
	self = [super initWithCoder:aCoder];
	if (nil != self)
	{
		[self createSubviews];
		_internalViewPtr = nullptr; // set later
	}
	return self;
}// initWithCoder:


/*!
Designated initializer.

(2018.04)
*/
- (instancetype)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		[self createSubviews];
		_internalViewPtr = nullptr; // set later
	}
	return self;
}// initWithFrame:


#pragma mark TerminalView_ClickDelegate


/*!
Invoked when a mouse-down event occurs in a background
view (such as the matte).  This responds by translating
the coordinates and then pretending that the event
occurred at the translated location in the canvas view.

This is crucial for usability, as it allows the user to
be “sloppy” when clicking near the edges of the canvas
while still having those clicks take effect.

(2018.06)
*/
- (void)
didReceiveMouseDownEvent:(NSEvent*)		anEvent
forView:(NSView*)						aView
{
#pragma unused(aView)
	// WARNING: the target view must directly handle this event and
	// not forward it to the first responder, otherwise the system
	// would probably enter a recursive loop
	[self.terminalContentView mouseDown:anEvent];
}// didReceiveMouseDownEvent:forView:


/*!
Invoked when a mouse-dragged event occurs in a background
view (such as the matte).  This responds by translating
the coordinates and then pretending that the event
occurred at the translated location in the canvas view.

This is crucial for usability, as it allows the user to
be “sloppy” when clicking near the edges of the canvas
while still having those clicks take effect.

(2018.06)
*/
- (void)
didReceiveMouseDraggedEvent:(NSEvent*)	anEvent
forView:(NSView*)						aView
{
#pragma unused(aView)
	// WARNING: the target view must directly handle this event and
	// not forward it to the first responder, otherwise the system
	// would probably enter a recursive loop
	[self.terminalContentView mouseDragged:anEvent];
}// didReceiveMouseDraggedEvent:forView:


/*!
Invoked when a mouse-up event occurs in a background
view (such as the matte).  This responds by translating
the coordinates and then pretending that the event
occurred at the translated location in the canvas view.

This is crucial for usability, as it allows the user to
be “sloppy” when clicking near the edges of the canvas
while still having those clicks take effect.

(2018.06)
*/
- (void)
didReceiveMouseUpEvent:(NSEvent*)	anEvent
forView:(NSView*)					aView
{
#pragma unused(aView)
	// WARNING: the target view must directly handle this event and
	// not forward it to the first responder, otherwise the system
	// would probably enter a recursive loop
	[self.terminalContentView mouseUp:anEvent];
}// didReceiveMouseUpEvent:forView:


#pragma mark NSView


/*!
Responds to a view entering a new superview (which includes
"nil") by updating notifications.

See also "viewWillMoveToWindow:".

(2020.12)
*/
- (void)
viewDidMoveToWindow
{
	if (nil != self.window)
	{
		[self whenObject:self postsNote:NSViewFrameDidChangeNotification
							performSelector:@selector(viewFrameDidChange:)];
	}
}// viewDidMoveToWindow


/*!
Responds to a view leaving a window (if "nil") by updating
view notifications.

See also "viewDidMoveToWindow".

(2020.12)
*/
- (void)
viewWillMoveToWindow:(NSWindow*)	aWindow
{
	if (nil == aWindow)
	{
		[self ignoreWhenObject:self postsNote:NSViewFrameDidChangeNotification];
	}
}// viewWillMoveToWindow:


/*!
Responds to the start of a live resize by displaying the
resize window (which will have either screen dimensions
or font size, depending on current settings).

See also "viewDidEndLiveResize".

(2020.12)
*/
- (void)
viewWillStartLiveResize
{
	[[TerminalWindow_InfoBubble sharedInfoBubble] reset];
	[TerminalWindow_InfoBubble sharedInfoBubble].delayBeforeRemoval = 0; // disable auto-remove
	[TerminalWindow_InfoBubble sharedInfoBubble].stringValue = @"—"; // see "viewFrameDidChange:"
	[[TerminalWindow_InfoBubble sharedInfoBubble] moveToCenterScreen:self.window.screen];
	[[TerminalWindow_InfoBubble sharedInfoBubble] display];
}// viewWillStartLiveResize


/*!
Responds to the end of a live resize by performing the
opposite of actions in "viewWillStartLiveResize".

(2020.12)
*/
- (void)
viewDidEndLiveResize
{
	[[TerminalWindow_InfoBubble sharedInfoBubble] removeWithAnimation];
}// viewDidEndLiveResize


#pragma mark NSViewNotifications


/*!
Responds to view frame changes by synchronizing the font size
or the underlying terminal screen dimensions (depending on
configuration).

See also TerminalView_SetResizeScreenBufferWithView() and
TerminalView_SetDisplayMode(), and the NSView property
"postsFrameChangedNotifications".

(2020.12)
*/
- (void)
viewFrameDidChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	//NSLog(@"%p: terminal view frame change", self); // debug
	if ((nil != self.internalViewPtr) && (false == self.internalViewPtr->screen.sizeNotMatchedWithView))
	{
		TerminalViewRef				terminalView = self.internalViewPtr->selfRef;
		TerminalView_DisplayMode	oldDisplayMode = TerminalView_ReturnDisplayMode(terminalView);
		TerminalView_DisplayMode	liveResizeDisplayMode = oldDisplayMode;
		// see Preferences window, General pane, “Special” tab; choice of modifier key should be consistent
		Boolean						invertMode = ((self.window.inLiveResize) &&
													(0 != ([NSEvent modifierFlags] & NSEventModifierFlagControl)));
		
		
		if (invertMode)
		{
			// in Preferences window, General pane, Special tab, it is noted
			// that the user can temporarily invert this behavior via keyboard
			liveResizeDisplayMode = ((kTerminalView_DisplayModeZoom == oldDisplayMode) ? kTerminalView_DisplayModeNormal : kTerminalView_DisplayModeZoom);
		}
		
		switch (liveResizeDisplayMode)
		{
		case kTerminalView_DisplayModeZoom:
			// change font size to fit new view frame at current screen dimensions
			{
				UInt16						columnCount = Terminal_ReturnColumnCount(self.internalViewPtr->screen.ref);
				UInt16						rowCount = Terminal_ReturnRowCount(self.internalViewPtr->screen.ref);
				TerminalView_PixelWidth		previousPixelWidth;
				TerminalView_PixelHeight	previousPixelHeight;
				Boolean						shrinkFontSize = false;
				CGFloat						newFontSize = 14; // arbitrary (should be overwritten by query below)
				SInt16						loopGuard = 10; // arbitrary; avoid accidental infinite loops
				
				
				// this notification occurs after the view frame has already changed;
				// determine how this initial frame compares to the ideal size (at
				// the current screen dimensions) and adjust the font up or down
				//
				// INCOMPLETE: if the new frame size has a wildly different aspect
				// ratio than the original, it might be useful to have a way to also
				// adjust the terminal screen dimensions, even if the font size is
				// also changing (e.g. if only one dimension changes, blow up the
				// font but also fill in the gap with extra terminal cells...)
				TerminalView_GetTheoreticalViewSize(terminalView, columnCount, rowCount, previousPixelWidth, previousPixelHeight);
				shrinkFontSize = ((self.frame.size.width < previousPixelWidth.precisePixels()) ||
									(self.frame.size.height < previousPixelHeight.precisePixels())); // if window appears to be shrinking, try smaller fonts; otherwise, try bigger ones
				TerminalView_GetFontAndSize(terminalView, nullptr/* font family name */, &newFontSize);
				UNUSED_RETURN(TerminalView_Result)TerminalView_SetDisplayMode(terminalView, kTerminalView_DisplayModeNormal); // prevent interference with explicit resize
				if (shrinkFontSize)
				{
					// find the smallest font size that will create the largest view within this area
					// (without allowing the view to exceed the boundaries)
					while ((loopGuard > 0) && (newFontSize > 4.0/* arbitrary */))
					{
						TerminalView_PixelWidth		newPixelWidth;
						TerminalView_PixelHeight	newPixelHeight;
						CGFloat						fontDelta = ((newFontSize < 18) ? -1.0 : -2.0); // arbitrary change per loop
						
						
						--loopGuard;
						newFontSize += fontDelta;
						UNUSED_RETURN(TerminalView_Result)setFontAndSize(self.internalViewPtr, nullptr/* font family name */, newFontSize, 0/* scale */, false/* notify listeners */);
						TerminalView_GetTheoreticalViewSize(terminalView, columnCount, rowCount, newPixelWidth, newPixelHeight);
						if ((self.frame.size.width > newPixelWidth.precisePixels()) &&
							(self.frame.size.height > newPixelHeight.precisePixels()))
						{
							// new ideal font size found but undo most recent font change (final view will otherwise be too large)
							newFontSize -= fontDelta;
							UNUSED_RETURN(TerminalView_Result)setFontAndSize(self.internalViewPtr, nullptr/* font family name */, newFontSize, 0/* scale */, true/* notify listeners */);
							break;
						}
					}
				}
				else
				{
					// find the largest font size that will create the largest view within this area
					// (without allowing the view to exceed the boundaries)
					while (loopGuard > 0)
					{
						TerminalView_PixelWidth		newPixelWidth;
						TerminalView_PixelHeight	newPixelHeight;
						CGFloat						fontDelta = ((newFontSize < 18) ? +1.0 : +2.0); // arbitrary change per loop
						
						
						--loopGuard;
						newFontSize += fontDelta;
						UNUSED_RETURN(TerminalView_Result)setFontAndSize(self.internalViewPtr, nullptr/* font family name */, newFontSize, 0/* scale */, false/* notify listeners */);
						TerminalView_GetTheoreticalViewSize(terminalView, columnCount, rowCount, newPixelWidth, newPixelHeight);
						if ((newPixelWidth.precisePixels() > self.frame.size.width) ||
							(newPixelHeight.precisePixels() > self.frame.size.height))
						{
							// new ideal font size found but undo most recent font change (final view will otherwise be too large)
							newFontSize -= fontDelta;
							UNUSED_RETURN(TerminalView_Result)setFontAndSize(self.internalViewPtr, nullptr/* font family name */, newFontSize, 0/* scale */, true/* notify listeners */);
							break;
						}
					}
				}
				// restore original mode (was temporarily disabled above during view size calculation)
				UNUSED_RETURN(TerminalView_Result)TerminalView_SetDisplayMode(terminalView, oldDisplayMode);
				if (self.window.inLiveResize)
				{
					CFRetainRelease		fontSizeFormatCFString(UIStrings_ReturnCopy(kUIStrings_TerminalDynamicResizeFontSize),
																CFRetainRelease::kAlreadyRetained);
					CFRetainRelease		fontSizeCFString(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */, fontSizeFormatCFString.returnCFStringRef(),
																					STATIC_CAST(newFontSize, unsigned long)),
															CFRetainRelease::kAlreadyRetained);
					
					
					// note: this bubble is displayed in "viewWillStartLiveResize"
					[TerminalWindow_InfoBubble sharedInfoBubble].stringValue = BRIDGE_CAST(fontSizeCFString.returnCFStringRef(), NSString*);
				}
			}
			break;
		
		case kTerminalView_DisplayModeNormal:
		default:
			// change screen dimensions to fit new view frame at current font size
			{
				TerminalView_PixelWidth		pixelWidth;
				TerminalView_PixelHeight	pixelHeight;
				UInt16						newColumnCount = 0;
				TerminalView_RowIndex		newRowCount = 0;
				
				
				// note: “theoretical dimensions” includes margin measurements so
				// this works correctly for a frame monitor on the container view
				// (namely, it should not be on the interior “content” view)
				pixelWidth.setPrecisePixels(self.frame.size.width);
				pixelHeight.setPrecisePixels(self.frame.size.height);
				TerminalView_GetTheoreticalScreenDimensions(terminalView, pixelWidth, pixelHeight,
															&newColumnCount, &newRowCount);
				if ((newColumnCount) && (newRowCount))
				{
					//NSLog(@"apparent ideal dimensions: %d x %d", (int)newColumnCount, (int)newRowCount); // debug
					UNUSED_RETURN(Terminal_Result)Terminal_SetVisibleScreenDimensions(self.internalViewPtr->screen.ref, newColumnCount, newRowCount);
					if (self.window.inLiveResize)
					{
						CFRetainRelease		screenDimensionsFormatCFString(UIStrings_ReturnCopy(kUIStrings_TerminalDynamicResizeWidthHeight),
																			CFRetainRelease::kAlreadyRetained);
						CFRetainRelease		screenDimensionsCFString(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */, screenDimensionsFormatCFString.returnCFStringRef(),
																								STATIC_CAST(newColumnCount, unsigned long), STATIC_CAST(newRowCount, unsigned long)),
																		CFRetainRelease::kAlreadyRetained);
						
						
						// note: this bubble is displayed in "viewWillStartLiveResize"
						[TerminalWindow_InfoBubble sharedInfoBubble].stringValue = BRIDGE_CAST(screenDimensionsCFString.returnCFStringRef(), NSString*);
					}
				}
			}
			break;
		}
	}
}// viewFrameDidChange:


@end //} TerminalView_Object


#pragma mark -
@implementation TerminalView_Object (TerminalView_ObjectInternal) //{


#pragma mark New Methods


/*!
Constructs all subviews such as the content region and
views that render padding and margin areas.

(2018.04)
*/
- (void)
createSubviews
{
	//
	// note that layout methods will adjust all views so their
	// initial frames are all empty
	//
	
	_terminalMarginViewBottom = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	[self addSubview:_terminalMarginViewBottom];
	
	_terminalMarginViewLeft = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	[self addSubview:_terminalMarginViewLeft];
	
	_terminalMarginViewRight = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	[self addSubview:_terminalMarginViewRight];
	
	_terminalMarginViewTop = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	[self addSubview:_terminalMarginViewTop];
	
	_terminalPaddingViewBottom = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	_terminalPaddingViewBottom.clickDelegate = self;
	[self addSubview:_terminalPaddingViewBottom];
	
	_terminalPaddingViewLeft = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	_terminalPaddingViewLeft.clickDelegate = self;
	[self addSubview:_terminalPaddingViewLeft];
	
	_terminalPaddingViewRight = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	_terminalPaddingViewRight.clickDelegate = self;
	[self addSubview:_terminalPaddingViewRight];
	
	_terminalPaddingViewTop = [[TerminalView_BackgroundView alloc] initWithFrame:NSZeroRect];
	_terminalPaddingViewTop.clickDelegate = self;
	[self addSubview:_terminalPaddingViewTop];
	
	_terminalContentView = [[TerminalView_ContentView alloc] initWithFrame:NSZeroRect];
	[self addSubview:_terminalContentView];
}// createSubviews


@end //} TerminalView_Object (TerminalView_ObjectInternal)


#pragma mark -
@implementation TerminalView_ScrollBar //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.04)
*/
- (instancetype)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		_internalViewPtr = nullptr;
	}
	return self;
}// initWithFrame:


#pragma mark NSView


/*!
Render the scroll bar, and any adornments on top of it (such
as the locations of terminal search results in the buffer).

(2018.04)
*/
- (void)
drawRect:(NSRect)	aRect
{
	[super drawRect:aRect];
	
	CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
	
	
#if 0
	// TEMPORARY: draw background
	{
		CGRect		clipBounds = CGContextGetClipBoundingBox(drawingContext);
		
		
		CGContextSetRGBFillColor(drawingContext, 1.0, 1.0, 0.0, 1.0/* alpha */);
		CGContextFillRect(drawingContext, clipBounds);
	}
#endif
	
	// check for search results
	if (nullptr != self.internalViewPtr)
	{
		TerminalView_CellRangeList		searchResultRanges;
		TerminalView_Result				viewResult = TerminalView_GetSearchResults(self.internalViewPtr->selfRef, searchResultRanges);
		
		
		if ((kTerminalView_ResultOK == viewResult) && (false == searchResultRanges.empty()))
		{
			// render red tick-marks for search results, showing their approximate
			// location within the screen/scrollback buffer
			UInt32			scrollbackRowCount = Terminal_ReturnInvisibleRowCount(self.internalViewPtr->screen.ref);
			UInt32			screenRowCount = Terminal_ReturnRowCount(self.internalViewPtr->screen.ref);
			UInt32 const	totalRowCount = (scrollbackRowCount + screenRowCount);
			CGFloat const	verticalPad = 10; // arbitrary; do not show ticks right at edge of frame
			CGFloat const	tickRange = (self.frame.size.height - verticalPad * 2);
			CGFloat const	offsetPerTick = (tickRange / totalRowCount);
			
			
			CGContextSetRGBFillColor(drawingContext, 1.0, 0.0, 0.0, 1.0/* alpha */);
			for (auto cellRange : searchResultRanges)
			{
				TerminalView_RowIndex	firstRow = std::get<1>(std::get<0>(cellRange));
				
				
				// convert from search result range to normalized range starting from top edge
				if (firstRow < 0)
				{
					// scrollback value (negative, measured from first scrollback line)
					firstRow += scrollbackRowCount;
				}
				else
				{
					// value is relative to top of main screen
					firstRow = (scrollbackRowCount + firstRow);
				}
				
				// draw the tick mark
				{
					CGRect		tickBounds = CGRectMake(0, verticalPad + offsetPerTick * firstRow, 15, 1);
					
					
					CGContextFillRect(drawingContext, tickBounds);
				}
			}
		}
	}
}// drawRect:


@end //} TerminalView_ScrollBar


#pragma mark -
@implementation TerminalView_ScrollBar (TerminalView_ScrollBarInternal) //{


@end //} TerminalView_ScrollBar (TerminalView_ScrollBarInternal)


#pragma mark -
@implementation TerminalView_ScrollableRootView //{


#pragma mark Initializers


/*!
Initializer for object constructed from a NIB.

(2018.04)
*/
- (instancetype)
initWithCoder:(NSCoder*)	aCoder
{
	self = [super initWithCoder:aCoder];
	if (nil != self)
	{
		[self createSubviews];
		_internalViewPtr = nullptr; // set later
	}
	return self;
}// initWithCoder:


/*!
Designated initializer.

(2018.04)
*/
- (instancetype)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		[self createSubviews];
		self->_internalViewPtr = nullptr; // set later
	}
	return self;
}// initWithFrame:


#pragma mark NSView


#if 0
/*!
For debug only; show the boundaries.

(2018.04)
*/
- (void)
drawRect:(NSRect)	aRect
{
#pragma unused(aRect)
	CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
	
	
	// TEMPORARY: draw background
	CGContextSetRGBStrokeColor(drawingContext, 0.0, 0.0, 0.0, 1.0/* alpha */);
	CGContextStrokeRect(drawingContext, CGRectMake(0, 0, NSWidth(self.bounds), NSHeight(self.bounds)));
}// drawRect:
#endif


@end //} TerminalView_ScrollableRootView


@implementation TerminalView_ScrollableRootView (TerminalView_ScrollableRootViewInternal) //{


#pragma mark New Methods


/*!
Constructs all subviews such as the content region and
views that render padding and margin areas.

(2018.04)
*/
- (void)
createSubviews
{
	//
	// note that layout methods will adjust all views so their
	// initial frames are all empty
	//
	
	self.scrollBarV = [[TerminalView_ScrollBar alloc] initWithFrame:NSZeroRect];
	[self addSubview:_scrollBarV];
}// createSubviews


@end //} TerminalView_ScrollableRootView (TerminalView_ScrollableRootViewInternal)


#pragma mark -
@implementation TerminalView_ScrollableRootVC //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.04)
*/
- (instancetype)
init
{
	self = [super initWithNibName:nil bundle:nil];
	if (nil != self)
	{
		auto	scrollableRootView = [[TerminalView_ScrollableRootView alloc] initWithFrame:NSZeroRect];
		
		
		_terminalViewControllers = [[NSMutableArray alloc] init];
		
		// when no NIB is present, "view" must be assigned
		scrollableRootView.layoutDelegate = self;
		self.view = scrollableRootView;
		assert(nil != self.scrollableRootView);
	}
	return self;
}// init


#pragma mark Accessors


/*!
Returns the scrollable view managed by this controller.
This is a convenience method for interpreting the
controller’s "view" property as a more precise class.

(2018.04)
*/
- (TerminalView_ScrollableRootView*)
scrollableRootView
{
	TerminalView_ScrollableRootView*	result = nil;
	
	
	if (NO == [self.view isKindOfClass:TerminalView_ScrollableRootView.class])
	{
		Console_Warning(Console_WriteLine, "TerminalView_ScrollableRootVC: 'scrollableRootView': 'self.view' is not of the expected class!");
	}
	else
	{
		result = STATIC_CAST(self.view, TerminalView_ScrollableRootView*);
	}
	
	return result;
}// scrollableRootView


#pragma mark New Methods


/*!
Starts managing the specified controller’s terminal view.
Currently, this means that user scroll activity can cause
the visible lines of the terminal view to change, and
search activity can cause tick marks to appear.

See also "enumerateTerminalViewControllersUsingBlock:" and
"removeTerminalViewController:".

(2018.04)
*/
- (void)
addTerminalViewController:(TerminalView_Controller*)	aVC
{
	[self.terminalViewControllers addObject:aVC];
	[self.view addSubview:aVC.view];
	self.scrollableRootView.scrollBarV.internalViewPtr = aVC.terminalView.internalViewPtr;
	[self.view forceResize];
}// addTerminalViewController:


/*!
Invokes the specified block on every terminal view controller
or stops when the block says to stop.

(2018.04)
*/
- (void)
enumerateTerminalViewControllersUsingBlock:(TerminalView_ControllerBlock)	aBlock
{
	__block BOOL	stopFlag = NO;
	
	
	for (TerminalView_Controller* aVC in self.terminalViewControllers)
	{
		aBlock(aVC, &stopFlag);
		if (stopFlag)
		{
			break;
		}
	}
}// enumerateTerminalViewControllersUsingBlock:


/*!
Stops managing the specified controller’s terminal view
(the reverse of "addTerminalViewController:").

(2018.04)
*/
- (void)
removeTerminalViewController:(TerminalView_Controller*)		aVC
{
	[aVC.view removeFromSuperview];
	self.scrollableRootView.scrollBarV.internalViewPtr = nil;
	[self.terminalViewControllers removeObject:aVC];
	[self.view forceResize];
}// removeTerminalViewController:


#pragma mark CoreUI_ViewLayoutDelegate


/*!
This is responsible for the layout of subviews.

(2018.04)
*/
- (void)
layoutDelegateForView:(NSView*)		aView
resizeSubviewsWithOldSize:(NSSize)	anOldSize
{
#pragma unused(aView, anOldSize)
	CGFloat		scrollBarWidth = [NSScroller scrollerWidthForControlSize:NSControlSizeRegular scrollerStyle:NSScrollerStyleLegacy];
	
	
	// if view is ridiculously narrow, give up on the scroll bar
	if (NSWidth(self.view.frame) < scrollBarWidth)
	{
		scrollBarWidth = 0;
	}
	
	[self enumerateTerminalViewControllersUsingBlock:
	^(TerminalView_Controller* aVC, BOOL* UNUSED_ARGUMENT(outStopFlagPtr))
	{
		aVC.view.frame = NSMakeRect(0, 0, NSWidth(self.view.frame) - scrollBarWidth,
									NSHeight(self.view.frame));
	}];
	self.scrollableRootView.scrollBarV.frame = NSMakeRect(NSWidth(self.view.frame) - scrollBarWidth,
															0, scrollBarWidth, NSHeight(self.view.frame));
}// layoutDelegateForView:resizeSubviewsWithOldSize:


@end //} TerminalView_ScrollableRootVC


#pragma mark -
@implementation TerminalView_ScrollableRootVC (TerminalView_ScrollableRootVCInternal) //{


@end //} TerminalView_ScrollableRootVC (TerminalView_ScrollableRootVCInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
