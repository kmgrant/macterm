/*!	\file TerminalView.mm
	\brief The renderer for terminal screen buffers.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <QuickTime/QuickTime.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CGContextSaveRestore.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <ColorUtilities.h>
#import <CommonEventHandlers.h>
#import <ContextSensitiveMenu.h>
#import <Console.h>
#import <FileSelectionDialogs.h>
#import <HIViewWrap.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Clipboard.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "DragAndDrop.h"
#import "EventLoop.h"
#import "MacroManager.h"
#import "NetEvents.h"
#import "Preferences.h"
#import "QuillsTerminal.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalBackground.h"
#import "TerminalGlyphDrawing.objc++.h"
#import "TerminalWindow.h"
#import "TextTranslation.h"
#import "UIStrings.h"
#import "URL.h"



#pragma mark Constants
namespace {

UInt16 const	kMy_LargeIBeamMinimumFontSize = 16;						//!< mouse I-beam cursor is 32x32 only if the font is at least this size
SInt16 const	kArbitraryVTGraphicsPseudoFontID = 74;					//!< should not match any real font; used to flag switch to VT graphics
SInt16 const	kArbitraryDoubleWidthDoubleHeightPseudoFontSize = 1;	//!< should not match any real size; used to flag the *lower* half
																		//!  of double-width, double-height text (upper half is marked by
																		//!  double the normal font size)

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

enum
{
	/*!
	Mac OS HIView part codes for Terminal Views (used ONLY when dealing
	with the HIView directly!!!).  Every visible line is assigned a part
	code number; if the terminal has more than 120 lines visible, the
	behavior is undefined.
	*/
	kTerminalView_ContentPartVoid			= kControlNoPart,	//!< nowhere in the content area
	kTerminalView_ContentPartFirstLine		= 1,				//!< draw topmost visible line
	//kTerminalView_ContentPartLineN			= <value in range>,	//!< draw line with this number
	kTerminalView_ContentPartLastLine		= 120,				//!< the largest line number that can be explicitly drawn
	kTerminalView_ContentPartText			= 121,				//!< draw a focus ring around terminal text area
	kTerminalView_ContentPartCursor			= 122,				//!< draw cursor (this part can’t be hit or focused)
	kTerminalView_ContentPartCursorGhost	= 123,				//!< draw cursor outline (also can’t be hit or focused)
};

enum MyCursorState
{
	kMyCursorStateInvisible = 0,
	kMyCursorStateVisible = 1
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
	TerminalView_CursorType		cursorType;
	Boolean						cursorBlinks;
	Boolean						dontDimTerminals;
	Boolean						invertSelections;
	Boolean						notifyOfBeeps;
	UInt16						renderMarginAtColumn; // the value 0 means “no rendering”; column 1 is first column, etc.
};

typedef std::vector< CGDeviceColor >			My_CGColorList;
typedef std::map< UInt16, CGDeviceColor >		My_CGColorByIndex; // a map is necessary because "vector" cannot handle 256 sequential color structures
typedef std::vector< EventTime >				My_TimeIntervalList;

/*!
A wrapper that calls HIViewConvertRegion() at construction
time, and optionally at destruction time to undo the effects
of its conversion.
*/
struct My_RegionConverter
{
	My_RegionConverter	(RgnHandle, HIViewRef, HIViewRef, Boolean = false);
	~My_RegionConverter	();
	
	RgnHandle	region;			//!< the region, retained only by reference, that is converted
	HIViewRef	source;			//!< source view for HIViewConvertRegion()
	HIViewRef	destination;	//!< source view for HIViewConvertRegion()
	Boolean		convertBack;	//!< if true, at destruction time HIViewConvertRegion() is called with inverted arguments;
								//!  otherwise, it is not called at all, and the region is considered permanently changed
};

class My_XTerm256Table;

// TEMPORARY: This structure is transitioning to C++, and so initialization
// and maintenance of it is downright ugly for the time being.  It *will*
// be simplified and become more object-oriented in the future.
struct My_TerminalView
{
	My_TerminalView		(HIViewRef);
	My_TerminalView		(TerminalView_ContentView*, TerminalView_BackgroundView*, TerminalView_BackgroundView*);
	~My_TerminalView	();
	
	void
	initialize		(TerminalScreenRef, Preferences_ContextRef);
	
	Preferences_ContextWrap		encodingConfig;	// various settings from an external source; not kept up to date, see TerminalView_ReturnTranslationConfiguration()
	Preferences_ContextWrap		formatConfig;	// various settings from an external source; not kept up to date, see TerminalView_ReturnFormatConfiguration()
	std::set< Preferences_Tag >		configFilter;	// settings that this view ignores when they are changed globally by the user
	ListenerModel_Ref	changeListenerModel;	// listeners for various types of changes to this data
	TerminalView_DisplayMode	displayMode;	// how the content fills the display area
	Boolean				isActive;				// true if the HIView is in an active state, false otherwise; kept in sync
												// using Carbon Events, stored here to avoid extra system calls in cases
												// where this would be slow (e.g. drawing)
	Boolean				isCocoa;				// true if using Cocoa view; this helps code to change without breaking the Carbon view in the meantime
	
	CFRetainRelease		accessibilityObject;	// AXUIElementRef; makes terminal views compatible with accessibility technologies
	NSView*				encompassingNSView;		// contains all other HIViews but is otherwise invisible
	HIViewRef			encompassingHIView;		// Carbon version; will go away when Cocoa transition is complete
	TerminalView_BackgroundView*	backgroundNSView;	// view that renders the background of the terminal screen (border included)
	HIViewRef			backgroundHIView;		// Carbon version; will go away when Cocoa transition is complete
	NSView*				focusNSView;			// view whose current focus part determines the visibility and boundaries of the focus ring
	HIViewRef			focusHIView;			// Carbon version; will go away when Cocoa transition is complete
	TerminalView_BackgroundView*	paddingNSView;	// view that is outset by the padding amount from the content view, if constructed as a Cocoa view
	HIViewRef			paddingHIView;			// Carbon version; will go away when Cocoa transition is complete
	TerminalView_ContentView*	contentNSView;	// view that renders the text of the terminal screen, if constructed as a Cocoa view
	HIViewRef			contentHIView;			// Carbon version; will go away when Cocoa transition is complete
	
	CommonEventHandlers_HIViewResizer	containerResizeHandler;		// responds to changes in the terminal view container boundaries
	CarbonEventHandlerWrap		contextualMenuHandler;		// responds to right-clicks
	CarbonEventHandlerWrap		rawKeyDownHandler;			// responds to keystrokes that change the text selection
	
	struct
	{
		struct
		{
			Boolean					isActive;	// true only if the timer is running
			EventLoopTimerUPP		upp;		// procedure that animates blinking text palette entries regularly
			EventLoopTimerRef		ref;		// timer to invoke animation procedure periodically
		} timer;
		
		struct
		{
			My_TimeIntervalList		delays;		// duration to wait after each animation stage
			SInt16					stage;		// which color and delay is currently being used
			SInt16					stageDelta;	// +1 or -1, current direction of change
			RgnHandle				region;		// used to optimize redraws during animation
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
		
	#if 0
		ListenerModel_ListenerWrap	backgroundDragHandler;	// listener for clicks in screens from inactive windows
	#endif
		ListenerModel_ListenerWrap	contentMonitor;			// listener for changes to the contents of the screen buffer
		ListenerModel_ListenerWrap	cursorMonitor;			// listener for changes to the terminal cursor position or visible state
		ListenerModel_ListenerWrap	preferenceMonitor;		// listener for changes to preferences that affect a particular view
		ListenerModel_ListenerWrap	bellHandler;			// listener for bell signals from the terminal screen
		ListenerModel_ListenerWrap	videoModeMonitor;		// listener for changes to the reverse-video setting
		RgnHandle					refreshRegion;			// used to optimize redraws
		EventLoopTimerUPP			refreshTimerUPP;		// UPP for the equivalent timer ref
		EventLoopTimerRef			refreshTimerRef;		// timer to invoke HIViewSetNeedsDisplayInRegion() at controlled intervals
		
		struct
		{
			RgnHandle			boundsAsRegion;	// update region for cursor (may or may not correspond to "bounds")
			Rect				bounds;			// the rectangle of the cursor’s visible region, relative to the content pane!!!
			Rect				ghostBounds;	// the exact view-relative rectangle of a dragged cursor’s outline region
			MyCursorState		currentState;	// whether the cursor is visible
			MyCursorState		ghostState;		// whether the cursor ghost is visible
			Boolean				inhibited;		// if true, the cursor can never be displayed regardless of its state
			Boolean				isCustomColor;	// if true, the cursor color is set by the user instead of being inherited from the screen
		} cursor;
		
TerminalView_RowIndex	topVisibleEdgeInRows;	// 0 if scrolled to the main screen, negative if scrollback; do not change this
													//   value directly, use offsetTopVisibleEdge()
		UInt16			leftVisibleEdgeInColumns;	// 0 if leftmost column is visible, positive if content is scrolled to the left;
													//   do not change this value directly, use offsetLeftVisibleEdge()
TerminalView_RowIndex	currentRenderedLine;	// only defined while drawing; the row that is currently being drawn
		Boolean			currentRenderBlinking;		// only defined while drawing; if true, at least one section is blinking
		Boolean			currentRenderDragColors;	// only defined while drawing; if true, drag highlight text colors are used
		Boolean			currentRenderNoBackground;	// only defined while drawing; if true, text is using the ordinary background color
		CGContextRef	currentRenderContext;		// only defined while drawing; if not nullptr, the context from the view draw event
		Float32			paddingLeftEmScale;			// left padding between text and focus ring; multiplies against normal (undoubled) character width
		Float32			paddingRightEmScale;		// right padding between text and focus ring; multiplies against normal (undoubled) character width
		Float32			paddingTopEmScale;			// top padding between text and focus ring; multiplies against normal (undoubled) character height
		Float32			paddingBottomEmScale;		// bottom padding between text and focus ring; multiplies against normal (undoubled) character height
		Float32			marginLeftEmScale;			// left margin between focus ring and container edge; multiplies against normal (undoubled) character width
		Float32			marginRightEmScale;			// right margin between focus ring and container edge; multiplies against normal (undoubled) character width
		Float32			marginTopEmScale;			// top margin between focus ring and container edge; multiplies against normal (undoubled) character height
		Float32			marginBottomEmScale;		// bottom margin between focus ring and container edge; multiplies against normal (undoubled) character height
		
		struct
		{
			// these settings should only ever be modified by recalculateCachedDimensions(),
			// and that routine should be called when any dependent factor, such as font,
			// is changed; see that routine’s documentation for more information
			UInt16			viewWidthInPixels;		// size of window view (window could be smaller than the screen size);
TerminalView_PixelHeight	viewHeightInPixels;		//   always identical to the current dimensions of the content view
		} cache;
	} screen;
	
	struct
	{
		TextAttributes_Object		attributes;		// current text attribute flags, affecting color of terminal text, etc.
		NSMutableDictionary*		attributeDict;	// most recent equivalent attributed-string attributes (e.g. fonts, colors)
		
		struct
		{
			NSFont*				normalFont;		// font for most text; also represents current family, size and metrics (Cocoa terminals)
			NSFont*				boldFont;		// alternate font for bold-weighted text (might match "normalFont" if no special font is found)
			Boolean				isMonospaced;	// whether every character in the font is the same width (expected to be true)
			Str255				familyName;		// font name (as might appear in a Font menu)
			struct Metrics
			{
				SInt16		ascent;			// number of pixels highest character extends above the base line
				SInt16		size;			// point size of text written in the font indicated by "familyName"
			} normalMetrics,	// metrics for text meant to fit in a single cell (normal)
			  doubleMetrics;	// metrics for text meant to fit in 4 cells, not 1 cell (double-width/height)
			Float32			scaleWidthPerCharacter;	// a multiplier (normally 1.0) to force characters from the font to fit in a different width
			SInt16			baseWidthPerCharacter;	// value of "widthPerCharacter" without any scaling applied
			SInt16			widthPerCharacter;	// number of pixels wide each character is (multiply by 2 on double-width lines);
												// generally, you should call getRowCharacterWidth() instead of referencing this!
			SInt16			heightPerCharacter;	// number of pixels high each character is (multiply by 2 if double-height text)
			Float32			thicknessHorizontalLines;	// thickness of non-bold horizontal lines in manually-drawn glyphs; diagonal lines may use based on angle
			Float32			thicknessHorizontalBold;	// thickness of bold horizontal lines in manually-drawn glyphs; diagonal lines may use based on angle
			Float32			thicknessVerticalLines;		// thickness of non-bold vertical lines in manually-drawn glyphs; diagonal lines may use based on angle
			Float32			thicknessVerticalBold;		// thickness of bold vertical lines in manually-drawn glyphs; diagonal lines may use based on angle
		} font;
		
		CGDeviceColor	colors[kMyBasicColorCount];	// indices are "kMyBasicColorIndexNormalText", etc.
		
		struct
		{
			TerminalView_CellRange		range;			// region of text selection
			My_SelectionMode			keyboardMode;	// used for keyboard navigation; determines what is changed by keyboard-select actions
			Boolean						exists;			// is any text highlighted anywhere in the window?
			Boolean						isRectangular;	// is the text selection unattached from the left and right screen edges?
			Boolean						readOnly;		// does the view respond to clicks and keystrokes that affect text selections?
			Boolean						inhibited;		// does the view refuse to highlight or manage selections even when API calls are made?
		} selection;
		
		TerminalView_CellRangeList				searchResults;			// regions matching the most recent Find results
		TerminalView_CellRangeList::iterator	toCurrentSearchResult;	// most recently focused match; MUST change if "searchResults" changes
	} text;
	
	TerminalViewRef		selfRef;				// redundant opaque reference that would resolve to point to this structure
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
	makeCGDeviceColor	(UInt8, UInt8, UInt8, CGDeviceColor&);
	
	void
	setColor	(UInt8, UInt8, UInt8, UInt8);
	
	GrayLevelByIndex	grayLevels;
	RGBLevelsByIndex	colorLevels;
};

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean				addDataSource						(My_TerminalViewPtr, TerminalScreenRef);
void				animateBlinkingItems				(EventLoopTimerRef, void*);
void				audioEvent							(ListenerModel_Ref, ListenerModel_Event, void*, void*);
EventTime			calculateAnimationStageDelay		(My_TerminalViewPtr, My_TimeIntervalList::size_type);
void				calculateDoubleSize					(My_TerminalViewPtr, SInt16&, SInt16&);
UInt16				copyColorPreferences				(My_TerminalViewPtr, Preferences_ContextRef, Boolean);
UInt16				copyFontPreferences					(My_TerminalViewPtr, Preferences_ContextRef, Boolean);
void				copySelectedTextIfUserPreference	(My_TerminalViewPtr);
void				copyTranslationPreferences			(My_TerminalViewPtr, Preferences_ContextRef);
OSStatus			createWindowColorPalette			(My_TerminalViewPtr, Preferences_ContextRef, Boolean = true);
Boolean				cursorBlinks						(My_TerminalViewPtr);
TerminalView_CursorType	cursorType						(My_TerminalViewPtr);
NSCursor*			customCursorCrosshairs				();
NSCursor*			customCursorIBeam					(Boolean = false);
NSCursor*			customCursorMoveTerminalCursor		(Boolean = false);
void				delayMinimumTicks					(UInt16 = 8);
OSStatus			dragTextSelection					(My_TerminalViewPtr, RgnHandle, EventRecord*, Boolean*);
void				drawSingleColorImage				(CGContextRef, CGColorRef, CGRect, id);
void				drawSingleColorPattern				(CGContextRef, CGColorRef, CGRect, id);
Boolean				drawSection							(My_TerminalViewPtr, CGContextRef, UInt16, TerminalView_RowIndex,
														 UInt16, TerminalView_RowIndex);
void				drawSymbolFontLetter				(My_TerminalViewPtr, CGContextRef, CGRect const&, UniChar, char);
void				drawTerminalScreenRunOp				(TerminalScreenRef, UInt16, CFStringRef, Terminal_LineRef, UInt16,
														 TextAttributes_Object, void*);
void				drawTerminalText					(My_TerminalViewPtr, CGContextRef, CGRect const&, Rect const&, CFIndex,
														 CFStringRef, TextAttributes_Object);
void				drawVTGraphicsGlyph					(My_TerminalViewPtr, CGContextRef, CGRect const&, UniChar, char,
														 CGFloat, TextAttributes_Object);
void				eraseSection						(My_TerminalViewPtr, CGContextRef, SInt16, SInt16, CGRect&);
void				eventNotifyForView					(My_TerminalViewConstPtr, TerminalView_Event, void*);
Terminal_LineRef	findRowIterator						(My_TerminalViewPtr, TerminalView_RowIndex, Terminal_LineStackStorage*);
Terminal_LineRef	findRowIteratorRelativeTo			(My_TerminalViewPtr, TerminalView_RowIndex, TerminalView_RowIndex,
														 Terminal_LineStackStorage*);
Boolean				findVirtualCellFromLocalPoint		(My_TerminalViewPtr, Point, TerminalView_Cell&, SInt16&, SInt16&);
Boolean				findVirtualCellFromScreenPoint		(My_TerminalViewPtr, HIPoint, TerminalView_Cell&, Float32&, Float32&);
void				getBlinkAnimationColor				(My_TerminalViewPtr, UInt16, CGDeviceColor*);
void				getRowBounds						(My_TerminalViewPtr, TerminalView_RowIndex, Rect*);
SInt16				getRowCharacterWidth				(My_TerminalViewPtr, TerminalView_RowIndex);
void				getRowSectionBounds					(My_TerminalViewPtr, TerminalView_RowIndex, UInt16, SInt16, Rect*);
void				getScreenBaseColor					(My_TerminalViewPtr, TerminalView_ColorIndex, CGDeviceColor*);
void				getScreenColorsForAttributes		(My_TerminalViewPtr, TextAttributes_Object, CGDeviceColor*, CGDeviceColor*, Boolean*);
Boolean				getScreenCoreColor					(My_TerminalViewPtr, UInt16, CGDeviceColor*);
void				getScreenCustomColor				(My_TerminalViewPtr, TerminalView_ColorIndex, CGDeviceColor*);
void				getScreenOrigin						(My_TerminalViewPtr, SInt16*, SInt16*);
void				getScreenOriginFloat				(My_TerminalViewPtr, Float32&, Float32&);
Handle				getSelectedTextAsNewHandle			(My_TerminalViewPtr, UInt16, TerminalView_TextFlags);
HIShapeRef			getSelectedTextAsNewHIShape			(My_TerminalViewPtr, Float32 = 0.0);
RgnHandle			getSelectedTextAsNewRegion			(My_TerminalViewPtr);
RgnHandle			getSelectedTextAsNewRegionOnScreen	(My_TerminalViewPtr);
size_t				getSelectedTextSize					(My_TerminalViewPtr);
HIShapeRef			getVirtualRangeAsNewHIShape			(My_TerminalViewPtr, TerminalView_Cell const&, TerminalView_Cell const&,
														 Float32, Boolean);
RgnHandle			getVirtualRangeAsNewRegion			(My_TerminalViewPtr, TerminalView_Cell const&, TerminalView_Cell const&, Boolean);
RgnHandle			getVirtualRangeAsNewRegionOnScreen	(My_TerminalViewPtr, TerminalView_Cell const&, TerminalView_Cell const&, Boolean);
void				getVirtualVisibleRegion				(My_TerminalViewPtr, UInt16*, TerminalView_RowIndex*, UInt16*, TerminalView_RowIndex*);
void				handleMultiClick					(My_TerminalViewPtr, UInt16);
void				handleNewViewContainerBounds		(HIViewRef, Float32, Float32, void*);
void				handlePendingUpdates				();
void				highlightCurrentSelection			(My_TerminalViewPtr, Boolean, Boolean);
void				highlightVirtualRange				(My_TerminalViewPtr, TerminalView_CellRange const&, TextAttributes_Object,
														 Boolean, Boolean);
void				invalidateRowSection				(My_TerminalViewPtr, TerminalView_RowIndex, UInt16, UInt16);
Boolean				isMonospacedFont					(FMFontFamily);
Boolean				isSmallIBeam						(My_TerminalViewPtr);
void				localToScreen						(My_TerminalViewPtr, SInt16*, SInt16*);
Boolean				mainEventLoopEvent					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				navigationFileCaptureDialogEvent	(NavEventCallbackMessage, NavCBRecPtr, void*);
void				offsetLeftVisibleEdge				(My_TerminalViewPtr, SInt16);
void				offsetTopVisibleEdge				(My_TerminalViewPtr, SInt32);
void				populateContextualMenu				(My_TerminalViewPtr, NSMenu*);
void				preferenceChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				preferenceChangedForView			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				recalculateCachedDimensions			(My_TerminalViewPtr);
OSStatus			receiveTerminalHIObjectEvents		(EventHandlerCallRef, EventRef, void*);
OSStatus			receiveTerminalViewActiveStateChange(EventHandlerCallRef, EventRef, TerminalViewRef);
OSStatus			receiveTerminalViewContextualMenuSelect	(EventHandlerCallRef, EventRef, void*);
OSStatus			receiveTerminalViewDraw				(EventHandlerCallRef, EventRef, TerminalViewRef);
OSStatus			receiveTerminalViewGetData			(EventHandlerCallRef, EventRef, TerminalViewRef);
OSStatus			receiveTerminalViewHitTest			(EventHandlerCallRef, EventRef, TerminalViewRef);
OSStatus			receiveTerminalViewRawKeyDown		(EventHandlerCallRef, EventRef, void*);
OSStatus			receiveTerminalViewRegionRequest	(EventHandlerCallRef, EventRef, TerminalViewRef);
OSStatus			receiveTerminalViewTrack			(EventHandlerCallRef, EventRef, TerminalViewRef);
void				receiveVideoModeChange				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				releaseRowIterator					(My_TerminalViewPtr, Terminal_LineRef*);
Boolean				removeDataSource					(My_TerminalViewPtr, TerminalScreenRef);
SInt64				returnNumberOfCharacters			(My_TerminalViewPtr);
CFStringRef			returnSelectedTextCopyAsUnicode		(My_TerminalViewPtr, UInt16, TerminalView_TextFlags);
void				screenBufferChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				screenCursorChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				screenToLocal						(My_TerminalViewPtr, SInt16*, SInt16*);
void				screenToLocalRect					(My_TerminalViewPtr, Rect*);
void				setAlphaTerminalWindowOp			(TerminalWindowRef, void*, SInt32, void*);
void				setBlinkAnimationColor				(My_TerminalViewPtr, UInt16, CGDeviceColor const*);
void				setBlinkingTimerActive				(My_TerminalViewPtr, Boolean);
void				setCursorVisibility					(My_TerminalViewPtr, Boolean);
void				setFontAndSize						(My_TerminalViewPtr, CFStringRef, UInt16, Float32 = 0, Boolean = true);
SInt16				setPortScreenPort					(My_TerminalViewPtr);
void				setScreenBaseColor					(My_TerminalViewPtr, TerminalView_ColorIndex, CGDeviceColor const*);
void				setScreenCoreColor					(My_TerminalViewPtr, UInt16, CGDeviceColor const*);
void				setScreenCustomColor				(My_TerminalViewPtr, TerminalView_ColorIndex, CGDeviceColor const*);
void				setTextAttributesDictionary			(My_TerminalViewPtr, NSMutableDictionary*, TextAttributes_Object,
														 Float32 = 1.0);
void				setUpCursorBounds					(My_TerminalViewPtr, SInt16, SInt16, Rect*, RgnHandle,
														 TerminalView_CursorType = kTerminalView_CursorTypeCurrentPreferenceValue);
void				setUpScreenFontMetrics				(My_TerminalViewPtr);
void				sortAnchors							(TerminalView_Cell&, TerminalView_Cell&, Boolean);
Boolean				startMonitoringDataSource			(My_TerminalViewPtr, TerminalScreenRef);
Boolean				stopMonitoringDataSource			(My_TerminalViewPtr, TerminalScreenRef);
void				trackTextSelection					(My_TerminalViewPtr, Point, EventModifiers, Point*, UInt32*);
void				updateDisplay						(My_TerminalViewPtr);
void				updateDisplayInRegion				(My_TerminalViewPtr, RgnHandle);
void				updateDisplayTimer					(EventLoopTimerRef, void*);
void				useTerminalTextAttributes			(My_TerminalViewPtr, CGContextRef, TextAttributes_Object);
void				useTerminalTextColors				(My_TerminalViewPtr, CGContextRef, TextAttributes_Object, Boolean, Float32 = 1.0);
void				visualBell							(TerminalViewRef);

} // anonymous namespace

/*!
Private properties.
*/
@interface TerminalView_BackgroundView () //{

// accessors
	@property (assign) size_t
	colorIndex; // a "kTerminalView_ColorIndex..." constant
	@property (assign) My_TerminalViewPtr
	internalViewPtr; // weak

@end //}

/*!
The private class interface.
*/
@interface TerminalView_BackgroundView (TerminalView_BackgroundViewInternal) //{

// (contains NSView overrides)

@end //}

/*!
Private properties.
*/
@interface TerminalView_ContentView () //{

// accessors
	@property (assign) My_TerminalViewPtr
	internalViewPtr; // weak
	@property (assign) NSUInteger
	modifierFlagsForCursor;
	@property (assign) BOOL
	showDragHighlight;

@end //}

/*!
The private class interface.
*/
@interface TerminalView_ContentView (TerminalView_ContentViewInternal) //{

// (contains NSControl overrides)

// (contains NSView overrides)

@end //}

#pragma mark Variables
namespace {

HIObjectClassRef			gTerminalTextViewHIObjectClassRef = nullptr;
EventHandlerUPP				gMyTextViewConstructorUPP = nullptr;
ListenerModel_ListenerRef	gMainEventLoopEventListener = nullptr;
ListenerModel_ListenerRef	gPreferenceChangeEventListener = nullptr;
struct My_PreferenceProxies	gPreferenceProxies;
Boolean						gApplicationIsSuspended = false;
Boolean						gTerminalViewInitialized = false;
My_TerminalViewPtrLocker&	gTerminalViewPtrLocks ()				{ static My_TerminalViewPtrLocker x; return x; }
RgnHandle					gInvalidationScratchRegion ()			{ static RgnHandle x = NewRgn(); assert(nullptr != x); return x; }
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
	// register a Human Interface Object class with Mac OS X
	// so that terminal view text areas can be easily created
	{
		EventTypeSpec const		whenHIObjectEventOccurs[] =
								{
									{ kEventClassHIObject, kEventHIObjectConstruct },
									{ kEventClassHIObject, kEventHIObjectInitialize },
									{ kEventClassHIObject, kEventHIObjectDestruct },
									{ kEventClassControl, kEventControlInitialize },
									{ kEventClassControl, kEventControlActivate },
									{ kEventClassControl, kEventControlDeactivate },
									{ kEventClassControl, kEventControlDraw },
									{ kEventClassControl, kEventControlGetData },
									{ kEventClassControl, kEventControlGetPartRegion },
									{ kEventClassControl, kEventControlHitTest },
									{ kEventClassControl, kEventControlSetCursor },
									{ kEventClassControl, kEventControlTrack },
									{ kEventClassAccessibility, kEventAccessibleGetAllAttributeNames },
									{ kEventClassAccessibility, kEventAccessibleGetNamedAttribute },
									{ kEventClassAccessibility, kEventAccessibleIsNamedAttributeSettable }
								};
		OSStatus				error = noErr;
		
		
		gMyTextViewConstructorUPP = NewEventHandlerUPP(receiveTerminalHIObjectEvents);
		assert(nullptr != gMyTextViewConstructorUPP);
		error = HIObjectRegisterSubclass(kConstantsRegistry_HIObjectClassIDTerminalTextView/* derived class */,
											kHIViewClassID/* base class */, 0/* options */, gMyTextViewConstructorUPP,
											GetEventTypeCount(whenHIObjectEventOccurs), whenHIObjectEventOccurs,
											nullptr/* constructor data */, &gTerminalTextViewHIObjectClassRef);
		assert_noerr(error);
	}
	
	// set up a callback to receive interesting events
	gMainEventLoopEventListener = ListenerModel_NewBooleanListener(mainEventLoopEvent);
	EventLoop_StartMonitoring(kEventLoop_GlobalEventSuspendResume, gMainEventLoopEventListener);
	gApplicationIsSuspended = FlagManager_Test(kFlagSuspended); // set initial value (above callback updates the value later on)
	
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
	
	// on older Mac OS X systems, custom cursors do not seem
	// to take effect in Carbon windows until a Cocoa window
	// has been displayed; to work around this odd behavior,
	// a useless Cocoa window is “displayed” at startup time
	{
		NSWindow*	invisibleWindow = [[NSWindow alloc] initWithContentRect:NSZeroRect styleMask:NSBorderlessWindowMask
																			backing:NSBackingStoreRetained
																			defer:NO];
		
		
		[invisibleWindow orderBack:NSApp];
		[invisibleWindow orderOut:NSApp];
		[invisibleWindow release];
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
	
	HIObjectUnregisterClass(gTerminalTextViewHIObjectClassRef), gTerminalTextViewHIObjectClassRef = nullptr;
	DisposeEventHandlerUPP(gMyTextViewConstructorUPP), gMyTextViewConstructorUPP = nullptr;
	
	EventLoop_StopMonitoring(kEventLoop_GlobalEventSuspendResume, gMainEventLoopEventListener);
	ListenerModel_ReleaseListener(&gMainEventLoopEventListener);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagCursorBlinks);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagNotifyOfBeeps);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagPureInverse);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagTerminalCursorType);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagTerminalShowMarginAtColumn);
	ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
}// Done


/*!
Creates a new HIView hierarchy for a terminal view, complete
with all the callbacks and data necessary to drive it.

DEPRECATED.  Use TerminalView_NewNSViewBased() instead.
However, until the Carbon window is retired completely, that
window will need this version of the view.

The font, colors, etc. are based on the default preferences
context, where any settings in "inFormatOrNull" will override
the defaults.  You can access this configuration later with
TerminalView_ReturnFormatConfiguration().

Since this is entirely associated with an HIObject, the view
automatically goes away whenever the HIView from
TerminalView_ReturnContainerHIView() is destroyed.  (For
example, if you have this view in a window and then dispose
of the window.)

If any problems occur, nullptr is returned; otherwise, a
reference to the data associated with the new view is
returned.  (Use TerminalView_ReturnContainerHIView() or
TerminalView_ReturnUserFocusHIView() to get at parts of the
actual HIView hierarchy.)

NOTE:	Since this is now based on an HIObject, you can
		technically create the view without this API.  But
		you will generally find this API to be simplest.

IMPORTANT:	As with all APIs in this module, you must have
			called TerminalView_Init() first.  In this case,
			that will have registered the appropriate
			HIObject class with the Mac OS X Toolbox.

(3.1)
*/
TerminalViewRef
TerminalView_NewHIViewBased		(TerminalScreenRef			inScreenDataSource,
								 Preferences_ContextRef		inFormatOrNull)
{
	TerminalViewRef		result = nullptr;
	HIViewRef			contentHIView = nullptr;
	EventRef			initializationEvent = nullptr;
	OSStatus			error = noErr;
	
	
	assert(gTerminalViewInitialized);
	
	error = CreateEvent(kCFAllocatorDefault, kEventClassHIObject, kEventHIObjectInitialize,
						GetCurrentEventTime(), kEventAttributeNone, &initializationEvent);
	if (noErr == error)
	{
		// specify the first source of data for this terminal (there must be at least one)
		error = SetEventParameter(initializationEvent, kEventParamNetEvents_TerminalDataSource,
									typeNetEvents_TerminalScreenRef, sizeof(inScreenDataSource), &inScreenDataSource);
		if (noErr == error)
		{
			Boolean		keepView = false; // used to tell when everything succeeds
			
			
			// optional - set format
			if (nullptr != inFormatOrNull)
			{
				error = SetEventParameter(initializationEvent, kEventParamNetEvents_TerminalFormatPreferences,
											typeNetEvents_PreferencesContextRef, sizeof(inFormatOrNull), &inFormatOrNull);
				if (noErr != error) Console_Warning(Console_WriteValue, "failed to use given format with new view, error", error);
			}
			
			// now construct!
			error = HIObjectCreate(kConstantsRegistry_HIObjectClassIDTerminalTextView, initializationEvent,
									REINTERPRET_CAST(&contentHIView, HIObjectRef*));
			if (noErr == error)
			{
				UInt32		actualSize = 0;
				
				
				// give the content view an ID, so that the background (parent) can find it when forwarding clicks
				{
					ControlID		newID = TerminalView_ReturnContainerHIViewID();
					
					
					error = SetControlID(contentHIView, &newID);
					assert_noerr(error);
				}
				
				// the event handlers for this class of HIObject will attach a custom
				// property to the new view, containing the TerminalViewRef
				error = GetControlProperty(contentHIView, AppResources_ReturnCreatorCode(),
											kConstantsRegistry_ControlPropertyTypeTerminalViewRef,
											sizeof(result), &actualSize, &result);
				if (noErr == error)
				{
					// success!
					keepView = true;
				}
			}
			
			// any errors?
			unless (keepView)
			{
				result = nullptr;
			}
		}
		
		ReleaseEvent(initializationEvent);
	}
	return result;
}// NewHIViewBased


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
TerminalView_NewNSViewBased		(TerminalView_ContentView*		inBaseView,
								 TerminalView_BackgroundView*	inPaddingView,
								 TerminalView_BackgroundView*	inBackgroundView,
								 TerminalScreenRef				inScreenDataSource,
								 Preferences_ContextRef			inFormatOrNull)
{
	TerminalViewRef		result = nullptr;
	
	
	assert(nullptr != inScreenDataSource);
	assert(gTerminalViewInitialized);
	
	try
	{
		My_TerminalViewPtr	viewPtr = new My_TerminalView(inBaseView, inPaddingView, inBackgroundView);
		
		
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
		if (false == viewPtr->text.selection.exists)
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
						NSMutableSet*	setOfCompletions = [[[NSMutableSet alloc] init] autorelease];
						
						
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
						
						// if the resulting set has exactly one string that
						// matches the original string, ignore it; otherwise,
						// pop up a menu with completion options
						if ((setOfCompletions.count > 1) ||
							(NO == [[setOfCompletions anyObject]
									isEqualToString:BRIDGE_CAST(searchQueryCFString.returnCFStringRef(), NSString*)]))
						{
							NSPoint				globalLocation = NSMakePoint(300, 500); // arbitrary default (TEMPORARY)
							NSMutableArray*		sortedCompletions = [[[NSMutableArray alloc] initWithCapacity:setOfCompletions.count]
																		autorelease];
							NSMenu*				completionsMenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
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
								[newItem release], newItem = nil;
								++completionIndex;
							}
							
							// specify that menu should pop up at cursor location
							{
								Rect		screenRect;
								Rect		cursorRect;
								HIRect		cursorHIRect;
								
								
								RegionUtilities_GetWindowDeviceGrayRect(HIViewGetWindow(viewPtr->contentHIView), &screenRect);
								
								cursorRect = viewPtr->screen.cursor.bounds;
								
								cursorHIRect.origin.x = cursorRect.left;
								cursorHIRect.origin.y = cursorRect.top;
								cursorHIRect.size.width = cursorRect.right - cursorRect.left;
								cursorHIRect.size.height = cursorRect.bottom - cursorRect.top;
								
								HIRectConvert(&cursorHIRect, kHICoordSpaceView, viewPtr->contentHIView,
												kHICoordSpaceScreenPixel, nullptr/* target object */);
								
								// translate the selection area into Cocoa coordinates that are
								// relative to the content view of the window
								globalLocation = NSMakePoint(cursorHIRect.origin.x,
																screenRect.bottom - screenRect.top - cursorHIRect.origin.y);
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
file, and then writes all selected text from the specified
view to that file.  If no text is selected, no user interface
appears.

This call returns immediately.  The save will only occur when
the user eventually commits the sheet that appears.  The user
can also cancel the operation.

A future version of this function could allow flags to be
specified to affect the capture “rules” (e.g. whether or not
to substitute tabs for consecutive spaces).

(3.1)
*/
void
TerminalView_DisplaySaveSelectedTextUI	(TerminalViewRef	inView)
{
	if (TerminalView_TextSelectionExists(inView))
	{
		NavDialogCreationOptions	dialogOptions;
		NavDialogRef				navigationServicesDialog = nullptr;
		My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
		OSStatus					error = noErr;
		
		
		error = NavGetDefaultDialogCreationOptions(&dialogOptions);
		if (noErr != error)
		{
			bzero(&dialogOptions, sizeof(dialogOptions));
		}
		// this call sets most of the options up front (parent window, etc.)
		dialogOptions.optionFlags |= kNavDontAddTranslateItems;
		Localization_GetCurrentApplicationNameAsCFString(&dialogOptions.clientName);
		dialogOptions.preferenceKey = kPreferences_NavPrefKeyGenericSaveFile;
		dialogOptions.parentWindow = HIViewGetWindow(viewPtr->contentHIView);
		
		// now set things specific to this instance
		(UIStrings_Result)UIStrings_Copy(kUIStrings_FileDefaultCaptureFile, dialogOptions.saveFileName);
		(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptCaptureToFile, dialogOptions.message);
		dialogOptions.modality = kWindowModalityWindowModal;
		
		error = NavCreatePutFileDialog(&dialogOptions, 'TEXT'/* type */, 'ttxt'/* creator (TextEdit) */,
										NewNavEventUPP(navigationFileCaptureDialogEvent), inView/* client data */,
										&navigationServicesDialog);
		Alert_ReportOSStatus(error);
		if (noErr == error)
		{
			// display the dialog; it is a sheet, so this will return immediately
			// and the dialog will close whenever the user is actually done with it
			error = NavDialogRun(navigationServicesDialog);
			Alert_ReportOSStatus(error);
		}
	}
}// DisplaySaveSelectedTextUI


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
	
	
	if (viewPtr->text.selection.exists)
	{
		SInt16		i = 0;
		UInt32		finalTick = 0L;
		CGrafPtr	currentPort = nullptr;
		GDHandle	currentDevice = nullptr;
		
		
		// for better performance on Mac OS X, lock the bits of a port
		// before performing a series of QuickDraw operations in it
		GetGWorld(&currentPort, &currentDevice);
		UNUSED_RETURN(OSStatus)LockPortBits(currentPort);
		
		for (i = 0; i < 2; ++i)
		{
			Delay(5/* arbitrary; measured in 60ths of a second */, &finalTick);
			highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
			
			// hand-hold Mac OS X to get the damned changes to show up!
			updateDisplay(viewPtr);
			
			Delay(5, &finalTick);
			highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
			
			// hand-hold Mac OS X to get the damned changes to show up!
			updateDisplay(viewPtr);
		}
		
		// undo the lock done earlier in this block
		UNUSED_RETURN(OSStatus)UnlockPortBits(currentPort);
	}
}// FlashSelection


/*!
Changes the focus of the specified view’s owning
window to be the given view’s container.

This is ONLY AN ADORNMENT; a Terminal View has no
ability to process keystrokes on its own.  However, a
controlling module such as Session might compare a
window’s currently-focused view with the result of
TerminalView_ReturnUserFocusHIView() to see whether
keyboard input should go to the specified view.

(3.0)
*/
void
TerminalView_FocusForUser	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	NSView*						cocoaView = TerminalView_ReturnUserFocusNSView(inView);
	
	
	if (nil != cocoaView)
	{
		[[cocoaView window] makeFirstResponder:TerminalView_ReturnUserFocusNSView(inView)];
	}
	else
	{
		UNUSED_RETURN(OSStatus)DialogUtilities_SetKeyboardFocus(TerminalView_ReturnUserFocusHIView(inView));
	}
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
						 CGDeviceColor*				outColorPtr)
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
Returns the current terminal cursor rectangle in
coordinates global to the display containing most
of the view.

Note that the cursor can have different shapes...
for instance, the returned rectangle may be very
flat (for an underline), narrow (for an insertion
point), or block shaped.

(3.1)
*/
void
TerminalView_GetCursorGlobalBounds	(TerminalViewRef	inView,
									 HIRect&			outGlobalBounds)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	outGlobalBounds.origin = CGPointMake(0, 0);
	outGlobalBounds.size = CGSizeMake(0, 0);
	if (viewPtr != nullptr)
	{
		Rect	globalCursorBounds;
		
		
		globalCursorBounds = viewPtr->screen.cursor.bounds;
		screenToLocalRect(viewPtr, &globalCursorBounds);
		QDLocalToGlobalRect(GetWindowPort(HIViewGetWindow(viewPtr->contentHIView)), &globalCursorBounds);
		
		outGlobalBounds = CGRectMake(globalCursorBounds.left, globalCursorBounds.top,
										globalCursorBounds.right - globalCursorBounds.left,
										globalCursorBounds.bottom - globalCursorBounds.top);
	}
}// GetCursorGlobalBounds


/*!
Returns the font and size of the text in the specified
terminal screen.  Either result can be nullptr if you
are not interested in that value.

(3.0)
*/
void
TerminalView_GetFontAndSize		(TerminalViewRef	inView,
								 CFStringRef*		outFontFamilyNameOrNull,
								 UInt16*			outFontSizeOrNull)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr != viewPtr)
	{
		if (nullptr != outFontFamilyNameOrNull)
		{
			CFStringRef		newString = CFStringCreateWithPascalString(kCFAllocatorDefault,
																		viewPtr->text.font.familyName,
																		kCFStringEncodingMacRoman);
			
			
			*outFontFamilyNameOrNull = newString;
			[BRIDGE_CAST(newString, NSString*) autorelease];
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
							 UInt16&					outWidthInPixels,
							 TerminalView_PixelHeight&	outHeightInPixels)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						result = false;
	
	
	if (nullptr != viewPtr)
	{
		Float32		highPrecision = 0;
		
		
		highPrecision = viewPtr->screen.marginLeftEmScale + viewPtr->screen.paddingLeftEmScale
						+ viewPtr->screen.paddingRightEmScale + viewPtr->screen.marginRightEmScale;
		highPrecision *= viewPtr->text.font.widthPerCharacter;
		highPrecision += viewPtr->screen.cache.viewWidthInPixels;
		outWidthInPixels = STATIC_CAST(highPrecision, UInt16);
		
		highPrecision = viewPtr->screen.marginTopEmScale + viewPtr->screen.paddingTopEmScale
						+ viewPtr->screen.paddingBottomEmScale + viewPtr->screen.marginBottomEmScale;
		highPrecision *= viewPtr->text.font.widthPerCharacter; // yes, width, because this is an “em” scale factor
		highPrecision += viewPtr->screen.cache.viewHeightInPixels;
		outHeightInPixels = STATIC_CAST(highPrecision, TerminalView_PixelHeight);
		
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
									 SInt32&			outStartOfView,
									 SInt32&			outPastEndOfView,
									 SInt32&			outStartOfMaximumRange,
									 SInt32&			outPastEndOfMaximumRange)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		SInt32 const	kScrollbackRows = Terminal_ReturnInvisibleRowCount(viewPtr->screen.ref);
		SInt16 const	kRows = Terminal_ReturnRowCount(viewPtr->screen.ref);
		
		
		// WARNING: this must use the same logic as TerminalView_ScrollToIndicatorPosition()
		outStartOfMaximumRange = 0;
		outPastEndOfMaximumRange = outStartOfMaximumRange + kScrollbackRows + kRows;
		outStartOfView = outPastEndOfMaximumRange - kRows + viewPtr->screen.topVisibleEdgeInRows;
		outPastEndOfView = outStartOfView + (viewPtr->screen.cache.viewHeightInPixels / viewPtr->text.font.heightPerCharacter);
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
												 UInt16						inWidthInPixels,
												 TerminalView_PixelHeight	inHeightInPixels,
												 UInt16*					outColumnCount,
												 TerminalView_RowIndex*		outRowCount)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Float32						highPrecision = 0.0;
	
	
	// Remove padding, border and margins before scaling remainder to find number of characters.
	// IMPORTANT: Synchronize this with TerminalView_GetTheoreticalViewSize() and TerminalView_GetIdealSize().
	highPrecision = STATIC_CAST(inWidthInPixels, Float32) / STATIC_CAST(viewPtr->text.font.widthPerCharacter, Float32);
	highPrecision -= (viewPtr->screen.marginLeftEmScale + viewPtr->screen.paddingLeftEmScale
						+ viewPtr->screen.paddingRightEmScale + viewPtr->screen.marginRightEmScale);
	*outColumnCount = STATIC_CAST(highPrecision, UInt16);
	if (*outColumnCount < 1) *outColumnCount = 1;
	highPrecision = inHeightInPixels
					- STATIC_CAST(viewPtr->screen.marginTopEmScale + viewPtr->screen.paddingTopEmScale
									+ viewPtr->screen.paddingBottomEmScale + viewPtr->screen.marginBottomEmScale, Float32)
						* STATIC_CAST(viewPtr->text.font.widthPerCharacter/* yes, width, because this is an “em” scale factor */, Float32);
	highPrecision /= STATIC_CAST(viewPtr->text.font.heightPerCharacter, Float32);
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
										 UInt16*					outWidthInPixels,
										 TerminalView_PixelHeight*	outHeightInPixels)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Float32						highPrecision = 0.0;
	
	
	// Incorporate the rectangle required for this amount of text, plus padding, border and margins.
	// IMPORTANT: Synchronize this with TerminalView_GetTheoreticalScreenDimensions() and TerminalView_GetIdealSize().
	highPrecision = inColumnCount + viewPtr->screen.marginLeftEmScale + viewPtr->screen.paddingLeftEmScale
					+ viewPtr->screen.paddingRightEmScale + viewPtr->screen.marginRightEmScale;
	highPrecision *= viewPtr->text.font.widthPerCharacter;
	*outWidthInPixels = STATIC_CAST(highPrecision, SInt16);
	highPrecision = STATIC_CAST(inRowCount, Float32) * STATIC_CAST(viewPtr->text.font.heightPerCharacter, Float32);
	highPrecision += STATIC_CAST(viewPtr->screen.marginTopEmScale + viewPtr->screen.paddingTopEmScale
									+ viewPtr->screen.paddingBottomEmScale + viewPtr->screen.marginBottomEmScale, Float32)
						* viewPtr->text.font.widthPerCharacter; // yes, width, because this is an “em” scale factor
	*outHeightInPixels = STATIC_CAST(highPrecision, SInt16);
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
										 Point				inLocalMouse)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Cell			newColumnRow;
	SInt16						unusedDeltaColumn = 0;
	SInt16						unusedDeltaRow = 0;
	Boolean						moveOK = false;
	
	
	// IMPORTANT: The deltas returned here refer to mouse locations that
	// are outside the view, which are not needed.
	if (findVirtualCellFromLocalPoint(viewPtr, inLocalMouse, newColumnRow, unusedDeltaColumn, unusedDeltaRow))
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
Determines whether a point in the coordinate system of a
window’s graphics port is in the boundaries of the
highlighted text in that window (if any).

Updated in version 3.0 to account for a selection area
that is rectangular.

(2.6)
*/
Boolean
TerminalView_PtInSelection	(TerminalViewRef	inView,
							 Point				inLocalPoint)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean						result = false;
	
	
	if (viewPtr != nullptr)
	{
		TerminalView_Cell	newColumnRow;
		SInt16				deltaColumn = 0;
		SInt16				deltaRow = 0;
		
		
		UNUSED_RETURN(Boolean)findVirtualCellFromLocalPoint(viewPtr, inLocalPoint, newColumnRow, deltaColumn, deltaRow);
		
		// test row range, which must be satisfied regardless of the selection style
		result = ((newColumnRow.second >= viewPtr->text.selection.range.first.second) &&
					(newColumnRow.second < viewPtr->text.selection.range.second.second));
		if (result)
		{
			if ((viewPtr->text.selection.isRectangular) ||
				(1 == (viewPtr->text.selection.range.second.second - viewPtr->text.selection.range.first.second)))
			{
				// purely rectangular selection range; verify column range
				result = ((newColumnRow.first >= viewPtr->text.selection.range.first.first) &&
							(newColumnRow.first < viewPtr->text.selection.range.second.first));
			}
			else
			{
				// Macintosh-like selection range
				if (newColumnRow.second == viewPtr->text.selection.range.first.second)
				{
					// first row; range from the column (inclusive) to the end is valid
					result = ((newColumnRow.first >= viewPtr->text.selection.range.first.first) &&
								(newColumnRow.first < Terminal_ReturnColumnCount(viewPtr->screen.ref)));
				}
				else if (newColumnRow.second == (viewPtr->text.selection.range.second.second - 1))
				{
					// last row; range from the start to the column (exclusive) is valid
					result = (newColumnRow.first < viewPtr->text.selection.range.second.first);
				}
				else
				{
					// anywhere in between is fine
					result = true;
				}
			}
		}
	}
	return result;
}// PtInSelection


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
Returns the Mac OS HIView that is the root of the specified
screen.  With the container, you can safely position a screen
anywhere in a window and the contents of the screen (embedded
in the container) will move with it.

To resize a screen’s view area, use a routine like
HIViewSetFrame() - Carbon Event handlers are installed such
that embedded views are properly resized when you do that.

DEPRECATED; use TerminalView_ReturnContainerNSView() instead.
However, until the Carbon window is retired completely, that
window will need this version of the view.

(3.0)
*/
HIViewRef
TerminalView_ReturnContainerHIView		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIViewRef					result = nullptr;
	
	
	result = viewPtr->encompassingHIView;
	return result;
}// ReturnContainerHIView


/*!
Returns the ID that can be used to find the terminal view’s
content view in a view hierarchy.  This is currently required to
allow click forwarding from the parent background view.

(4.0)
*/
HIViewID
TerminalView_ReturnContainerHIViewID ()
{
	HIViewID	result = { 'Cnvs', 0/* ID */ };
	
	
	return result;
}// ReturnContainerHIViewID


/*!
Returns the Cocoa NSView* that is the root of the specified
screen.  With the container, you can safely position a screen
anywhere in a window and the contents of the screen (embedded
in the container) will move with it.

To resize a screen’s view area, use the view "frame" property.

(4.0)
*/
NSView*
TerminalView_ReturnContainerNSView		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	NSView*						result = nil;
	
	
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
Returns the Mac OS HIView that can render a drag highlight
(via TerminalView_SetDragHighlight()) and should have drag
handlers installed on it.

This might be, but is NOT guaranteed to be, the same as the
user focus view.  See also TerminalView_ReturnUserFocusHIView().

DEPRECATED; use TerminalView_ReturnDragFocusHIView() instead.
However, until the Carbon window is retired completely, that
window will need this version of the view.

(3.1)
*/
HIViewRef
TerminalView_ReturnDragFocusHIView	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIViewRef					result = nullptr;
	
	
	// should match whichever view has drag handlers installed on it
	result = viewPtr->contentHIView;
	return result;
}// ReturnDragFocusHIView


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
	result = viewPtr->contentNSView;
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
		SInt16		fontSize = viewPtr->text.font.normalMetrics.size;
		
		
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
	
	
	result = [viewPtr->contentNSView window];
	return result;
}// ReturnNSWindow


/*!
DEPRECATED.
Use TerminalView_ReturnSelectedTextCopyAsUnicode()
instead.

Returns the contents of the current selection for
the specified screen view, allocating a new handle.
If there is no selection, an empty handle is returned.
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

You must dispose of the returned handle yourself,
using Memory_DisposeHandle().

IMPORTANT:	Use this kind of routine very judiciously.
			Copying data is very inefficient and is
			almost never necessary, particularly for
			scrollback rows which are immutable.  Use
			other APIs to access text ranges in ways
			that do not replicate bytes needlessly.

(3.0)
*/
Handle
TerminalView_ReturnSelectedTextAsNewHandle	(TerminalViewRef			inView,
											 UInt16						inMaxSpacesToReplaceWithTabOrZero,
											 TerminalView_TextFlags		inFlags)
{
    My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Handle						result = nullptr;
	
	
	if (nullptr != viewPtr)
	{
		result = getSelectedTextAsNewHandle(viewPtr, inMaxSpacesToReplaceWithTabOrZero, inFlags);
	}
	return result;
}// ReturnSelectedTextAsNewHandle


/*!
Returns a region in coordinates LOCAL to the given
view’s window, describing the highlighted text.
You must destroy this region yourself!

(2.6)
*/
RgnHandle
TerminalView_ReturnSelectedTextAsNewRegion		(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	RgnHandle					result = nullptr;
	
	
	result = getSelectedTextAsNewRegion(viewPtr);
	return result;
}// ReturnSelectedTextAsNewRegion


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
Returns the Mac OS HIView that can be focused for keyboard
input.  You can use this compared with the current keyboard
focus for a window to determine if the specified view is
focused.

DEPRECATED; use TerminalView_ReturnUserFocusHIView() instead.
However, until the Carbon window is retired completely, that
window will need this version of the view.

(3.0)
*/
HIViewRef
TerminalView_ReturnUserFocusHIView	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIViewRef					result = nullptr;
	
	
	// should match whichever view has the set-focus-part event handler installed on it
	result = viewPtr->focusHIView;
	return result;
}// ReturnUserFocusHIView


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
	result = viewPtr->focusNSView;
	return result;
}// ReturnUserFocusNSView


/*!
If the user focus event target appears to belong to ANY
terminal view, it is returned from this routine; otherwise,
nullptr is returned.

WARNING:	Since the user is in control, the focus can
			change over time; do not retain this value
			indefinitely if there is a chance the event
			loop will run.

(3.1)
*/
TerminalViewRef
TerminalView_ReturnUserFocusTerminalView ()
{
	TerminalViewRef		result = nullptr;
	WindowRef			focusWindow = GetUserFocusWindow();
	
	
	if (nullptr != focusWindow)
	{
		HIViewRef	focusView = nullptr;
		
		
		if (noErr == GetKeyboardFocus(focusWindow, &focusView))
		{
			UInt32		actualSize = 0;
			OSStatus	error = noErr;
			
			
			// if this control really is a Terminal View, it should have
			// the view reference attached as a control property
			error = GetControlProperty(focusView, AppResources_ReturnCreatorCode(),
										kConstantsRegistry_ControlPropertyTypeTerminalViewRef,
										sizeof(result), &actualSize, &result);
			if (noErr != error) result = nullptr;
		}
	}
	return result;
}// ReturnUserFocusTerminalView


/*!
Returns the Mac OS window reference for the given
Terminal View.

(3.0)
*/
HIWindowRef
TerminalView_ReturnWindow	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	WindowRef					result = nullptr;
	
	
	result = HIViewGetWindow(viewPtr->contentHIView);
	assert(IsValidWindowRef(result));
	return result;
}// ReturnWindow


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
		CGDeviceColor	oldTextColor;
		CGDeviceColor	oldBackgroundColor;
		
		
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
		result = TerminalView_ScrollColumnsTowardRightEdge(inView, inColumnCountDelta);
	}
	
	if (inRowCountDelta > 0)
	{
		result = TerminalView_ScrollRowsTowardTopEdge(inView, inRowCountDelta);
	}
	else if (inRowCountDelta < 0)
	{
		result = TerminalView_ScrollRowsTowardBottomEdge(inView, inRowCountDelta);
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
		
		
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, INTEGER_DIV_4(kVisibleRowCount));
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, INTEGER_DIV_4(kVisibleRowCount));
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, INTEGER_DIV_4(kVisibleRowCount));
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(inView, kVisibleRowCount - 3 * INTEGER_DIV_4(kVisibleRowCount));
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
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardLeftEdge(inView, kVisibleColumnCountBy4);
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardLeftEdge(inView, kVisibleColumnCountBy4);
		handlePendingUpdates();
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
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardRightEdge(inView, kVisibleColumnCountBy4);
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollColumnsTowardRightEdge(inView, kVisibleColumnCountBy4);
		handlePendingUpdates();
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
		
		
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, INTEGER_DIV_4(kVisibleRowCount));
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, INTEGER_DIV_4(kVisibleRowCount));
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, INTEGER_DIV_4(kVisibleRowCount));
		handlePendingUpdates();
		delayMinimumTicks(kMy_PageScrollDelayTicks);
		UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(inView, kVisibleRowCount - 3 * INTEGER_DIV_4(kVisibleRowCount));
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
TerminalView_ScrollToCell	(TerminalViewRef				inView,
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
TerminalView_ScrollToIndicatorPosition	(TerminalViewRef		inView,
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
				viewPtr->text.selection.exists = true;
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
			viewPtr->text.selection.exists = true;
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
			TerminalView_SelectNothing(inView);
			
			viewPtr->text.selection.range.first = std::make_pair(0, cursorY);
			viewPtr->text.selection.range.second = std::make_pair(Terminal_ReturnColumnCount(viewPtr->screen.ref) + 1, cursorY + 1);
			
			highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
			viewPtr->text.selection.exists = true;
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
		TerminalView_SelectNothing(inView);
		
		// NOTE that the screen is anchored with a (0, 0) origin at the top of the bottommost
		// windowful, so a negative offset is required to capture the entire scrollback buffer
		viewPtr->text.selection.range.first = std::make_pair(0, -Terminal_ReturnInvisibleRowCount(viewPtr->screen.ref));
		viewPtr->text.selection.range.second = std::make_pair(Terminal_ReturnColumnCount(viewPtr->screen.ref),
																Terminal_ReturnRowCount(viewPtr->screen.ref));
		
		highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.exists = true;
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
		TerminalView_SelectNothing(inView);
		
		viewPtr->text.selection.range.first = std::make_pair(0, 0);
		viewPtr->text.selection.range.second = std::make_pair(Terminal_ReturnColumnCount(viewPtr->screen.ref),
																Terminal_ReturnRowCount(viewPtr->screen.ref));
		
		highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.exists = true;
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
		if (viewPtr->text.selection.exists)
		{
			highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
			viewPtr->text.selection.exists = false;
		}
		viewPtr->text.selection.range.first = std::make_pair(0, 0);
		viewPtr->text.selection.range.second = std::make_pair(0, 0);
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
		TerminalView_SelectNothing(inView);
		
		viewPtr->text.selection.range = inSelection;
		
		highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.exists = true;
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
						 CGDeviceColor const*		inColorPtr)
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
Arranges for a drag highlight to be shown or hidden for
the given view, which should be the drag focus view of a
terminal view.  Also updates the cursor, because this is
presumably happening during a drag.

The highlight is only rendered when the view is drawn.
However, this call will invalidate the appropriate area.

See TerminalView_ReturnDragFocusHIView().

(3.1)
*/
void
TerminalView_SetDragHighlight	(HIViewRef		inView,
								 DragRef		UNUSED_ARGUMENT(inDrag),
								 Boolean		inIsHighlighted)
{
	Boolean		showHighlight = inIsHighlighted;
	
	
	// set a property so that the view drawing routine understands
	// when it should include a drag highlight
	// TEMPORARY: this error is ignored, perhaps this function should return an error
	UNUSED_RETURN(OSStatus)SetControlProperty(inView, AppResources_ReturnCreatorCode(),
												kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
												sizeof(showHighlight), &showHighlight);
	
	// change the cursor, for additional visual feedback
	if (showHighlight)
	{
		[[NSCursor dragCopyCursor] set];
	}
	else
	{
		[[NSCursor arrowCursor] set];
	}
	
	// redraw the view
	UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplay(inView, true);
}// SetDragHighlight


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
	
	
	UNUSED_RETURN(OSStatus)HIViewSetDrawingEnabled(viewPtr->contentHIView, inIsDrawingEnabled);
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
								 UInt16				inFontSizeOrZero)
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
		OSStatus	error = noErr;
		
		
		// add a listener to the specified target’s listener model for the given event
		error = ListenerModel_AddListenerForEvent(viewPtr->changeListenerModel, inForWhatEvent, inListener);
		if (error != noErr) result = kTerminalView_ResultParameterError;
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
	
	
	if (viewPtr != nullptr) result = viewPtr->text.selection.exists;
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
		// the selection region is currently defined in the window’s local (content view) coordinates
		HIShapeRef	selectionShape = getSelectedTextAsNewHIShape(viewPtr, 0);
		
		
		if (nullptr != selectionShape)
		{
			HIWindowRef			screenWindow = TerminalView_ReturnWindow(inView);
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(screenWindow);
			HIRect				selectionViewBounds = CGRectZero;
			CGRect				selectionCGRect = CGRectZero;
			
			
			// since the region is currently defined in content-local coordinates,
			// the “height” of the “view” containing the selection is actually
			// going to be the entire content view and not just the screen part
			UNUSED_RETURN(OSStatus)HIViewGetBounds(HIViewWrap(kHIViewWindowContentID, screenWindow), &selectionViewBounds);
			
			// translate the selection area into Cocoa coordinates that are
			// relative to the content view of the window
			UNUSED_RETURN(CGRect*)HIShapeGetBounds(selectionShape, &selectionCGRect);
			selectionCGRect.origin.y = selectionViewBounds.size.height - (selectionCGRect.origin.y + selectionCGRect.size.height);
			
			// animate!
			TerminalView_FlashSelection(inView);
			if (nullptr != terminalWindow)
			{
				NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(terminalWindow);
				
				
				CocoaAnimation_TransitionWindowSectionForOpen(cocoaWindow, selectionCGRect);
			}
			
			CFRelease(selectionShape), selectionShape = nullptr;
		}
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
		HIWindowRef			screenWindow = TerminalView_ReturnWindow(inView);
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(screenWindow);
		HIRect				cursorViewBounds = CGRectZero;
		CGRect				cursorCGRect = CGRectZero;
		Rect				cursorRect;
		
		
		cursorRect = viewPtr->screen.cursor.bounds;
		
		screenToLocalRect(viewPtr, &cursorRect);
		
		// since the region is currently defined in content-local coordinates,
		// the “height” of the “view” containing the selection is actually
		// going to be the entire content view and not just the screen part
		UNUSED_RETURN(OSStatus)HIViewGetBounds(HIViewWrap(kHIViewWindowContentID, screenWindow), &cursorViewBounds);
		
		// translate the selection area into Cocoa coordinates that are
		// relative to the content view of the window
		cursorCGRect.origin.x = cursorRect.left;
		cursorCGRect.origin.y = cursorViewBounds.size.height - cursorRect.bottom;
		cursorCGRect.size.width = cursorRect.right - cursorRect.left;
		cursorCGRect.size.height = cursorRect.bottom - cursorRect.top;
		cursorCGRect = CGRectInset(cursorCGRect, -30, -30); // arbitrary
		
		// animate!
		if (nullptr != terminalWindow)
		{
			NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(terminalWindow);
			
			
			// this is a bit of a hack, but the “search result” animation is a
			// reasonable way to draw attention to the cursor’s rectangle
			CocoaAnimation_TransitionWindowSectionForSearchResult(cocoaWindow, cursorCGRect);
		}
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
		
		// the selection region is currently defined in the window’s local (content view) coordinates
		HIShapeRef	selectionShape = getVirtualRangeAsNewHIShape(viewPtr, viewPtr->text.toCurrentSearchResult->first,
																	viewPtr->text.toCurrentSearchResult->second,
																	0/* insets */, false/* is rectangular */);
		
		
		if (nullptr != selectionShape)
		{
			HIWindowRef			screenWindow = TerminalView_ReturnWindow(inView);
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(screenWindow);
			HIRect				selectionViewBounds = CGRectZero;
			CGRect				selectionCGRect = CGRectZero;
			
			
			// since the region is currently defined in content-local coordinates,
			// the “height” of the “view” containing the selection is actually
			// going to be the entire content view and not just the screen part
			UNUSED_RETURN(OSStatus)HIViewGetBounds(HIViewWrap(kHIViewWindowContentID, screenWindow), &selectionViewBounds);
			
			// translate the selection area into Cocoa coordinates that are
			// relative to the content view of the window
			UNUSED_RETURN(CGRect*)HIShapeGetBounds(selectionShape, &selectionCGRect);
			selectionCGRect.origin.y = selectionViewBounds.size.height - (selectionCGRect.origin.y + selectionCGRect.size.height);
			
			// animate!
			if (nullptr != terminalWindow)
			{
				NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(terminalWindow);
				
				
				CocoaAnimation_TransitionWindowSectionForSearchResult(cocoaWindow, selectionCGRect);
			}
			
			CFRelease(selectionShape), selectionShape = nullptr;
		}
	}
}// ZoomToSearchResults


/*!
Displays an animation that helps the user locate
the currently-selected text.

(3.0)
*/
void
TerminalView_ZoomToSelection	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((viewPtr != nullptr) && (viewPtr->text.selection.exists))
	{
		RgnHandle	selectionRegion = getSelectedTextAsNewRegion(viewPtr);
		
		
		if (selectionRegion != nullptr)
		{
			Rect	selectionBounds;
			HIRect	screenContentFloatBounds;
			Rect	screenContentBounds;
			
			
			// find global rectangle of selection
			QDLocalToGlobalRegion(GetWindowPort(HIViewGetWindow(viewPtr->contentHIView)), selectionRegion);
			GetRegionBounds(selectionRegion, &selectionBounds);
			
			// find global rectangle of the screen area
			UNUSED_RETURN(OSStatus)HIViewGetBounds(viewPtr->contentHIView, &screenContentFloatBounds);
			RegionUtilities_SetRect(&screenContentBounds, 0, 0, STATIC_CAST(screenContentFloatBounds.size.width, SInt16),
									STATIC_CAST(screenContentFloatBounds.size.height, SInt16));
			screenToLocalRect(viewPtr, &screenContentBounds);
			QDLocalToGlobalRect(GetWindowPort(HIViewGetWindow(viewPtr->contentHIView)), &screenContentBounds);
			
			// animate!
			UNUSED_RETURN(OSStatus)ZoomRects(&screenContentBounds, &selectionBounds, 20/* steps, arbitrary */, kZoomDecelerate);
			
			DisposeRgn(selectionRegion), selectionRegion = nullptr;
		}
	}
}// ZoomToSelection


#pragma mark Internal Methods
namespace {

/*!
Calls HIViewConvertRegion() to convert the specified region
into a new coordinate system.

(4.0)
*/
My_RegionConverter::
My_RegionConverter	(RgnHandle		inoutRegion,
					 HIViewRef		inSource,
					 HIViewRef		inDestination,
					 Boolean		inConvertBack)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
region			(inoutRegion),
source			(inSource),
destination		(inDestination),
convertBack		(inConvertBack)
{
	OSStatus	error = HIViewConvertRegion(region, source, destination);
	
	
	assert_noerr(error);
}// My_RegionConverter constructor


/*!
For instances configured to convert back, once again calls
HIViewConvertRegion() to restore the region to its previous
coordinate system.

(4.0)
*/
My_RegionConverter::
~My_RegionConverter ()
{
	if (convertBack)
	{
		OSStatus	error = HIViewConvertRegion(region, destination, source);
		
		
		assert_noerr(error);
	}
}// My_RegionConverter destructor


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
Fills in a CGDeviceColor structure with appropriate values
based on the given 8-bit components.

(4.0)
*/
void
My_XTerm256Table::
makeCGDeviceColor	(UInt8				inRed,
					 UInt8				inGreen,
					 UInt8				inBlue,
					 CGDeviceColor&		outColor)
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
}// My_XTerm256Table::makeCGDeviceColor


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
Constructor.  See receiveTerminalHIObjectEvents().

WARNING:	This constructor leaves the class uninitialized!
			You MUST invoke the initialize() method before
			doing anything with it.  This unsafe constructor
			exists to support the HIObject model.

(3.1)
*/
My_TerminalView::
My_TerminalView		(HIViewRef		inSuperclassViewInstance)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
encodingConfig(), // set later
formatConfig(), // set later
configFilter(),
changeListenerModel(nullptr), // set later
displayMode(kTerminalView_DisplayModeNormal), // set later
isActive(true),
isCocoa(false),
accessibilityObject(AXUIElementCreateWithHIObjectAndIdentifier
					(REINTERPRET_CAST(inSuperclassViewInstance, HIObjectRef), 0/* identifier */), CFRetainRelease::kAlreadyRetained),
encompassingNSView(nil),
encompassingHIView(nullptr), // set later
backgroundNSView(nil),
backgroundHIView(nullptr), // set later
focusNSView(nil),
focusHIView(nullptr), // set later
paddingNSView(nil),
paddingHIView(nullptr), // set later
contentNSView(nil),
contentHIView(inSuperclassViewInstance),
containerResizeHandler(), // set later
contextualMenuHandler(),
rawKeyDownHandler(),
selfRef(REINTERPRET_CAST(this, TerminalViewRef))
{
}// My_TerminalView 1-argument constructor (HIViewRef)


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
My_TerminalView		(TerminalView_ContentView*		inSuperclassViewInstance,
					 TerminalView_BackgroundView*	inPaddingView,
					 TerminalView_BackgroundView*	inBackgroundView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
encodingConfig(), // set later
formatConfig(), // set later
configFilter(),
changeListenerModel(nullptr), // set later
displayMode(kTerminalView_DisplayModeNormal), // set later
isActive(true),
isCocoa(true),
accessibilityObject(), // obsolete
encompassingNSView(nil), // set later
encompassingHIView(nullptr),
backgroundNSView([inBackgroundView retain]),
backgroundHIView(nullptr),
focusNSView(nil), // set later
focusHIView(nullptr),
paddingNSView([inPaddingView retain]),
paddingHIView(nullptr),
contentNSView([inSuperclassViewInstance retain]),
contentHIView(nullptr),
containerResizeHandler(), // set later
contextualMenuHandler(),
rawKeyDownHandler(),
selfRef(REINTERPRET_CAST(this, TerminalViewRef))
{
}// My_TerminalView 1-argument constructor (NSView*)


/*!
Initializer.  See the constructor as well as
receiveTerminalHIObjectEvents().

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
	this->screen.refreshRegion = NewRgn();
	
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
	this->text.selection.exists = false;
	this->text.selection.isRectangular = false;
	this->text.selection.readOnly = false;
	this->text.selection.inhibited = false;
	this->screen.leftVisibleEdgeInColumns = 0;
	this->screen.topVisibleEdgeInRows = 0;
	this->screen.cache.viewWidthInPixels = 0; // set later...
	this->screen.cache.viewHeightInPixels = 0; // set later...
	this->screen.sizeNotMatchedWithView = false;
	this->screen.focusRingEnabled = true;
	this->screen.isReverseVideo = 0;
	this->screen.cursor.currentState = kMyCursorStateVisible;
	this->screen.cursor.ghostState = kMyCursorStateInvisible;
	this->screen.cursor.boundsAsRegion = NewRgn();
	this->screen.cursor.inhibited = false;
	this->screen.cursor.isCustomColor = false;
	this->screen.currentRenderContext = nullptr;
	this->text.toCurrentSearchResult = this->text.searchResults.end();
	
	// read user preferences for the spacing around the edges
	{
		Preferences_Result	preferencesResult = kPreferences_ResultOK;
		
		
		// margins
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginLeft,
														sizeof(this->screen.marginLeftEmScale), &this->screen.marginLeftEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginRight,
														sizeof(this->screen.marginRightEmScale), &this->screen.marginRightEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginTop,
														sizeof(this->screen.marginTopEmScale), &this->screen.marginTopEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalMarginBottom,
														sizeof(this->screen.marginBottomEmScale), &this->screen.marginBottomEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		
		// paddings
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingLeft,
														sizeof(this->screen.paddingLeftEmScale), &this->screen.paddingLeftEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingRight,
														sizeof(this->screen.paddingRightEmScale), &this->screen.paddingRightEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingTop,
														sizeof(this->screen.paddingTopEmScale), &this->screen.paddingTopEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalPaddingBottom,
														sizeof(this->screen.paddingBottomEmScale), &this->screen.paddingBottomEmScale,
														true/* search defaults too */);
		assert(kPreferences_ResultOK == preferencesResult);
	}
	
	// copy font defaults
	{
		UInt16		preferenceCount = copyFontPreferences(this, inFormat, true/* search defaults too */);
		
		
		assert(preferenceCount > 0);
	}
	
	// create views
	if (this->isCocoa)
	{
		// read optional image URL
		// UNIMPLEMENTED
		
		// NOTE: in the Cocoa implementation, all views are passed in to the routine that
		// constructs the Terminal View, so views do not need to be created here; but,
		// it is necessary to associate the Terminal View object with them
		this->contentNSView.internalViewPtr = this;
		this->paddingNSView.internalViewPtr = this;
		this->backgroundNSView.internalViewPtr = this;
		
		// NOTE: initializing the padding and background view colors is not really necessary
		// (by the time the views are displayed, they will have been updated in the same way
		// that they are whenever the user changes preferences later); however, it is
		// important to tell the views which color index to use
		this->paddingNSView.colorIndex = kMyBasicColorIndexNormalBackground;
		this->backgroundNSView.colorIndex = kMyBasicColorIndexMatteBackground;
		
		// specify the view to use for focus and basic input
		this->focusNSView = this->backgroundNSView;
		
		// since no extra rendering, etc. is required, make this a mere ALIAS
		// for the background view; this is so that code intending to operate
		// on the “entire” view can clearly indicate this by referring to the
		// encompassing pane instead of some specific pane that might change
		this->encompassingNSView = this->backgroundNSView;
	}
	else if (nullptr != this->contentHIView)
	{
		Preferences_Result	preferencesResult = kPreferences_ResultOK;
		CFStringRef			imageURLCFString = nullptr;
		OSStatus			error = noErr;
		
		
		// read optional image URL
		preferencesResult = Preferences_ContextGetData(inFormat, kPreferences_TagTerminalImageNormalBackground,
														sizeof(imageURLCFString), &imageURLCFString, true/* search defaults too */);
		if (kPreferences_ResultOK != preferencesResult)
		{
			imageURLCFString = nullptr;
		}
		
		// see the HIObject callbacks, this will already exist
		error = HIViewSetVisible(this->contentHIView, true);
		assert_noerr(error);
		
		assert_noerr(TerminalBackground_CreateHIView(this->paddingHIView, false/* is matte */, imageURLCFString));
		error = HIViewSetVisible(this->paddingHIView, true);
		assert_noerr(error);
		// IMPORTANT: Set a property with the TerminalViewRef, so that
		// TerminalView_ReturnUserFocusHIView() works properly
		error = SetControlProperty(this->paddingHIView,
									AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeTerminalViewRef,
									sizeof(this->selfRef), &this->selfRef);
		assert_noerr(error);
		
		assert_noerr(TerminalBackground_CreateHIView(this->backgroundHIView, true/* is matte */));
		error = HIViewSetVisible(this->backgroundHIView, true);
		assert_noerr(error);
		
		// specify the view to use for focus and basic input
		//this->focusHIView = this->paddingHIView;
		this->focusHIView = this->backgroundHIView;
		
		// initialize matte color
		{
			CGDeviceColor* const	kColorPtr = &this->text.colors[kMyBasicColorIndexMatteBackground];
			
			
			error = SetControlProperty(this->backgroundHIView, AppResources_ReturnCreatorCode(),
										kConstantsRegistry_ControlPropertyTypeBackgroundColor,
										sizeof(*kColorPtr), kColorPtr);
			assert_noerr(error);
		}
		
		// since no extra rendering, etc. is required, make this a mere ALIAS
		// for the background view; this is so that code intending to operate
		// on the “entire” view can clearly indicate this by referring to the
		// encompassing pane instead of some specific pane that might change
		this->encompassingHIView = this->backgroundHIView;
		
		// set up embedding hierarchy
		error = HIViewAddSubview(this->backgroundHIView, this->paddingHIView);
		assert_noerr(error);
		error = HIViewAddSubview(this->paddingHIView, this->contentHIView);
		assert_noerr(error);
	}
	
	// store the colors this view will be using
	assert_noerr(createWindowColorPalette(this, inFormat));
	
	// initialize blink animation
	{
		this->animation.rendering.delays.resize(kMy_BlinkingColorCount);
		for (My_TimeIntervalList::size_type i = 0; i < this->animation.rendering.delays.size(); ++i)
		{
			this->animation.rendering.delays[i] = calculateAnimationStageDelay(this, i);
		}
		this->animation.rendering.stage = 0;
		this->animation.rendering.stageDelta = +1;
		this->animation.rendering.region = NewRgn();
		this->animation.cursor.blinkAlpha = 1.0;
	}
	
	if (false == this->isCocoa)
	{
		// install a monitor on the container that finds out about
		// resizes (for example, because HIViewSetFrame() was called
		// on it) and updates subviews to match
		this->containerResizeHandler.install(this->encompassingHIView, kCommonEventHandlers_ChangedBoundsAnyEdge,
												handleNewViewContainerBounds, this->selfRef/* context */);
		assert(this->containerResizeHandler.isInstalled());
		
		// install a handler for contextual menu clicks
		this->contextualMenuHandler.install(HIViewGetEventTarget(this->contentHIView),
											receiveTerminalViewContextualMenuSelect,
											CarbonEventSetInClass(CarbonEventClass(kEventClassControl),
																	kEventControlContextualMenuClick),
											REINTERPRET_CAST(this, TerminalViewRef)/* user data */);
		assert(this->contextualMenuHandler.isInstalled());
		
		// set up keyboard text selection on the focus view
		this->rawKeyDownHandler.install(HIViewGetEventTarget(this->focusHIView),
											receiveTerminalViewRawKeyDown,
											CarbonEventSetInClass(CarbonEventClass(kEventClassKeyboard),
																	kEventRawKeyDown, kEventRawKeyRepeat),
											REINTERPRET_CAST(this, TerminalViewRef)/* user data */);
		assert(this->rawKeyDownHandler.isInstalled());
	}
	
	if (false == this->isCocoa)
	{
		// show all HIViews
		HIViewSetVisible(this->encompassingHIView, true/* visible */);
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
	
	// refreshes are requested on a fixed schedule because the API call
	// may be expensive, and the system coalesces updates to within
	// 60 frames (once per tick) anyway
	{
		OSStatus	error = noErr;
		
		
		this->screen.refreshTimerUPP = NewEventLoopTimerUPP(updateDisplayTimer);
		error = InstallEventLoopTimer(GetCurrentEventLoop(),
										kEventDurationNoWait + 0.01/* time before first fire; must be nonzero for Mac OS X 10.3 */,
										TicksToEventTime(1)/* time between fires */,
										this->screen.refreshTimerUPP,
										this->selfRef/* user data */,
										&this->screen.refreshTimerRef);
		assert_noerr(error);
	}
	
	// now that everything is initialized, start watching for data changes
	assert(startMonitoringDataSource(this, inScreenDataSource));
}// My_TerminalView::initialize


/*!
Destructor.  See the handler for HIObject events
that deals with setup and teardown.

(3.0)
*/
My_TerminalView::
~My_TerminalView ()
{
	// NOTE: encompassingNSView and focusNSView are aliases only, and are never retained
	[this->contentNSView release];
	[this->paddingNSView release];
	[this->backgroundNSView release];
	[this->text.attributeDict release];
	if (nil != this->text.font.normalFont)
	{
		[this->text.font.normalFont release], this->text.font.normalFont = nil;
	}
	if (nil != this->text.font.boldFont)
	{
		[this->text.font.boldFont release], this->text.font.boldFont = nil;
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
	RemoveEventLoopTimer(this->animation.timer.ref), this->animation.timer.ref = nullptr;
	DisposeEventLoopTimerUPP(this->animation.timer.upp), this->animation.timer.upp = nullptr;
	if (nullptr != this->screen.refreshTimerRef)
	{
		RemoveEventLoopTimer(this->screen.refreshTimerRef), this->screen.refreshTimerRef = nullptr;
		DisposeEventLoopTimerUPP(this->screen.refreshTimerUPP), this->screen.refreshTimerUPP = nullptr;
	}
	
	DisposeRgn(this->animation.rendering.region), this->animation.rendering.region = nullptr;
	DisposeRgn(this->screen.cursor.boundsAsRegion), this->screen.cursor.boundsAsRegion = nullptr;
	DisposeRgn(this->screen.refreshRegion), this->screen.refreshRegion = nullptr;
	ListenerModel_Dispose(&this->changeListenerModel);
	
	// release strong references to data sources
	assert(removeDataSource(this, nullptr/* specific screen or nullptr for all screens */));
}// My_TerminalView destructor


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
animateBlinkingItems	(EventLoopTimerRef		inTimer,
						 void*					inTerminalViewRef)
{
	TerminalViewRef				ref = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	My_TerminalViewAutoLocker	ptr(gTerminalViewPtrLocks(), ref);
	
	
	if (ptr != nullptr)
	{
		CGDeviceColor	currentColor;
		
		
		// for simplicity, keep the cursor and text blinks in sync
		UNUSED_RETURN(OSStatus)SetEventLoopTimerNextFireTime(inTimer, ptr->animation.rendering.delays[ptr->animation.rendering.stage]);
		
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
		updateDisplayInRegion(ptr, ptr->animation.rendering.region);
		
		//
		// cursor
		//
		
		// adjust the alpha setting to be used for cursor-drawing
		ptr->animation.cursor.blinkAlpha = kAlphaByPhase[ptr->animation.rendering.stage];
		
		// invalidate the cursor
		updateDisplayInRegion(ptr, ptr->screen.cursor.boundsAsRegion);
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
			
			
			visualBell(inView);
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
EventTime
calculateAnimationStageDelay	(My_TerminalViewPtr					inTerminalViewPtr,
								 My_TimeIntervalList::size_type		inZeroBasedStage)
{
	My_TimeIntervalList::size_type const	kNumStages = inTerminalViewPtr->animation.rendering.delays.size();
	assert(inZeroBasedStage < kNumStages);
	
	EventTime		result = 0;
	
	
	//result = 200.0 * kEventDurationMillisecond; // linear
	result = (inZeroBasedStage * inZeroBasedStage + inZeroBasedStage) * 2.0 * kEventDurationMillisecond; // quadratic
	return result;
}// calculateAnimationStageDelay


/*!
Determines the ideal font size for the font of the given
terminal screen that would fit 4 screen cells.  Normally
text is sized to fit one cell, but double-width and double-
height text fills 4 cells.

Returns the double font size.  The “official” double size
of the specified terminal is NOT changed.

(3.0)
*/
void
calculateDoubleSize		(My_TerminalViewPtr		inTerminalViewPtr,
						 SInt16&				outPointSize,
						 SInt16&				outAscent)
{
	HIWindowRef		window = HIViewGetWindow(inTerminalViewPtr->contentHIView);
	CGrafPtr		oldPort = nullptr;
	CGrafPtr		windowPort = nullptr;
	GDHandle		oldDevice = nullptr;
	GDHandle		windowDevice = nullptr;
	SInt16			preservedFontID = 0;
	SInt16			preservedFontSize = 0;
	Style			preservedFontStyle = 0;
	FontInfo		info;
	
	
	outPointSize = inTerminalViewPtr->text.font.normalMetrics.size;
	outAscent = inTerminalViewPtr->text.font.normalMetrics.ascent;
	
	// preserve state
	GetGWorld(&oldPort, &oldDevice);
	SetPortWindowPort(window);
	GetGWorld(&windowPort, &windowDevice);
	preservedFontID = GetPortTextFont(windowPort);
	preservedFontSize = GetPortTextSize(windowPort);
	preservedFontStyle = GetPortTextFace(windowPort);
	
	// choose an appropriate font size to best fill 4 cells; favor maximum
	// width over height, but reduce the font size if the characters overrun
	// the bottom of the area
	TextFontByName(inTerminalViewPtr->text.font.familyName);
	TextSize(outPointSize);
	while ((STATIC_CAST(CharWidth('A'), Float32) * inTerminalViewPtr->text.font.scaleWidthPerCharacter) <
			INTEGER_TIMES_2(inTerminalViewPtr->text.font.widthPerCharacter))
	{
		TextSize(++outPointSize);
	}
	GetFontInfo(&info);
	while (STATIC_CAST(info.ascent + info.descent + info.leading, unsigned int) >=
			INTEGER_TIMES_2(inTerminalViewPtr->text.font.heightPerCharacter))
	{
		TextSize(--outPointSize);
		GetFontInfo(&info);
	}
	outAscent = info.ascent;
	
	// restore previous state
	TextFont(preservedFontID);
	TextSize(preservedFontSize);
	TextFace(preservedFontStyle);
	SetGWorld(oldPort, oldDevice);
}// calculateDoubleSize


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
	CGDeviceColor				colorValue;
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
	SInt16			fontSize = 0;
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
	if (inTerminalViewPtr->text.selection.exists)
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

Returns "noErr" only if the palette is created successfully.

(3.1)
*/
OSStatus
createWindowColorPalette	(My_TerminalViewPtr			inTerminalViewPtr,
							 Preferences_ContextRef		inFormat,
							 Boolean					inSearchForDefaults)
{
	OSStatus	result = noErr;
	
	
	// create palettes for future use
	inTerminalViewPtr->customColors.resize(kTerminalView_ColorIndexLastValid - kTerminalView_ColorIndexFirstValid + 1);
	inTerminalViewPtr->blinkColors.resize(kMy_BlinkingColorCount);
	inTerminalViewPtr->extendedColorsPtr = &gColorGrid(); // globally shared unless customized
	
	// set up window’s colors; note that this will set the non-ANSI colors,
	// the blinking colors, and the ANSI colors
	UNUSED_RETURN(UInt16)copyColorPreferences(inTerminalViewPtr, inFormat, inSearchForDefaults);
	
	// install a timer to modify blinking text color entries periodically
	assert(nullptr != inTerminalViewPtr->selfRef);
	inTerminalViewPtr->animation.timer.upp = NewEventLoopTimerUPP(animateBlinkingItems);
	UNUSED_RETURN(OSStatus)InstallEventLoopTimer(GetCurrentEventLoop(), kEventDurationForever/* time before first fire */,
													kEventDurationForever/* time between fires - set later */,
													inTerminalViewPtr->animation.timer.upp,
													inTerminalViewPtr->selfRef/* user data */,
													&inTerminalViewPtr->animation.timer.ref);
	inTerminalViewPtr->animation.timer.isActive = false;
	
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
TerminalView_CursorType
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
customCursorCrosshairs ()
{
	static NSCursor*	gCustomCrosshairsCursor = nil;
	NSCursor*			result = nil;
	
	
	if (nil == gCustomCrosshairsCursor)
	{
		// IMPORTANT: specified hot-spot should be synchronized with the image data
		gCustomCrosshairsCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorCrosshairs"]
																	hotSpot:NSMakePoint(15, 15)];
	}
	result = gCustomCrosshairsCursor;
	
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
customCursorIBeam	(Boolean	inSmall)
{
	static NSCursor*	gCustomIBeamCursor = nil;
	static NSCursor*	gCustomIBeamCursorSmall = nil;
	NSCursor*			result = nil;
	
	
	if (inSmall)
	{
		if (nil == gCustomIBeamCursorSmall)
		{
			// IMPORTANT: specified hot-spot should be synchronized with the image data
			gCustomIBeamCursorSmall = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorIBeamSmall"]
																		hotSpot:NSMakePoint(12, 11)];
		}
		result = gCustomIBeamCursorSmall;
	}
	else
	{
		if (nil == gCustomIBeamCursor)
		{
			// IMPORTANT: specified hot-spot should be synchronized with the image data
			gCustomIBeamCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorIBeam"]
																	hotSpot:NSMakePoint(16, 14)];
		}
		result = gCustomIBeamCursor;
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
customCursorMoveTerminalCursor	(Boolean	inSmall)
{
	static NSCursor*	gCustomMoveCursor = nil;
	static NSCursor*	gCustomMoveCursorSmall = nil;
	NSCursor*			result = nil;
	
	
	if (inSmall)
	{
		if (nil == gCustomMoveCursorSmall)
		{
			// IMPORTANT: specified hot-spot should be synchronized with the image data
			gCustomMoveCursorSmall = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorMoveCursorSmall"]
																		hotSpot:NSMakePoint(12, 12)];
		}
		result = gCustomMoveCursorSmall;
	}
	else
	{
		if (nil == gCustomMoveCursor)
		{
			// IMPORTANT: specified hot-spot should be synchronized with the image data
			gCustomMoveCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorMoveCursor"]
																	hotSpot:NSMakePoint(16, 16)];
		}
		result = gCustomMoveCursor;
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
	UInt32		dummy = 0L;
	
	
	Delay(inTickCount, &dummy);
}// delayMinimumTicks


/*!
This method handles dragging of a text selection.
If the text is dropped (i.e. not cancelled), then
"true" is returned in "outDragWasDropped".

(3.0)
*/
OSStatus
dragTextSelection	(My_TerminalViewPtr		inTerminalViewPtr,
					 RgnHandle				inoutGlobalDragOutlineRegion,
					 EventRecord*			inoutEventPtr,
					 Boolean*				outDragWasDropped)
{
	OSStatus		result = noErr;
	PasteboardRef	dragPasteboard = nullptr;
	DragRef			dragRef = nullptr;
	
	
	*outDragWasDropped = true;
	
	result = PasteboardCreate(kPasteboardUniqueName, &dragPasteboard);
	if (noErr == result)
	{
		Clipboard_TextToScrap(inTerminalViewPtr->selfRef, kClipboard_CopyMethodStandard, dragPasteboard);
		result = NewDragWithPasteboard(dragPasteboard, &dragRef);
		if (noErr == result)
		{
			NSCursor*	oldCursor = [NSCursor currentCursor];
			
			
			[[NSCursor arrowCursor] set];
			result = TrackDrag(dragRef, inoutEventPtr, inoutGlobalDragOutlineRegion);
			[oldCursor set];
		}
	}
	DisposeDrag(dragRef), dragRef = nullptr;
	
	[[NSCursor arrowCursor] set];
	
	return result;
}// dragTextSelection


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
		CGrafPtr			currentPort = nullptr;
		GDHandle			currentDevice = nullptr;
		
		
		if (false == inTerminalViewPtr->isCocoa)
		{
			// for better performance on Mac OS X, lock the bits of a port
			// before performing a series of QuickDraw operations in it,
			// and make the intended drawing area part of the dirty region
			GetGWorld(&currentPort, &currentDevice);
			UNUSED_RETURN(OSStatus)LockPortBits(currentPort);
		}
		
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
				Terminal_Result			terminalError = kTerminal_ResultOK;
				
				
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
					if (nullptr == lineIterator) break;
					
					iteratorResult = Terminal_ForEachLikeAttributeRunDo
										(inTerminalViewPtr->screen.ref, lineIterator/* starting row */,
											drawTerminalScreenRunOp, inTerminalViewPtr/* context, passed to callback */);
					if (iteratorResult != kTerminal_ResultOK)
					{
						// did not draw successfully...?
					}
					else
					{
						result = true;
					}
					
					// since double-width text is a row-wide attribute, it must be applied
					// to the entire row instead of per-style-run; do that here
					terminalError = Terminal_GetLineGlobalAttributes(inTerminalViewPtr->screen.ref, lineIterator,
																		&lineGlobalAttributes);
					if (kTerminal_ResultOK != terminalError)
					{
						lineGlobalAttributes.clear();
					}
					if (lineGlobalAttributes.hasDoubleWidth())
					{
						Rect	rowBounds;
						
						
						getRowBounds(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderedLine, &rowBounds);
						
						{
							PixMapHandle	pixels = nullptr;
							
							
							GetGWorld(&currentPort, &currentDevice);
							pixels = GetPortPixMap(currentPort);
							if (LockPixels(pixels))
							{
								ImageDescriptionHandle  image = nullptr;
								Rect					sourceRect = rowBounds;
								Rect					destRect = rowBounds;
								OSStatus				error = noErr;
								
								
								// halve the source width, as the boundaries refer to the rendering width
								sourceRect.right -= INTEGER_DIV_2(sourceRect.right - sourceRect.left);
								
								error = MakeImageDescriptionForPixMap(pixels, &image);
								if (error == noErr)
								{
									// IMPORTANT:		The source rectangle is relative to the origin of the
									//				pixel map, whereas the destination is relative to the
									//				graphics port.  So, although the source and destination
									//				come from the “same” place, the source is offset by the
									//				port origin.
									Rect	pixelMapBounds;
									
									
									GetPixBounds(pixels, &pixelMapBounds);
									RegionUtilities_OffsetRect(&sourceRect, -pixelMapBounds.left, -pixelMapBounds.top);
									error = DecompressImage(GetPixBaseAddr(pixels), image, pixels,
															&sourceRect/* source rectangle */,
															&destRect/* destination rectangle */,
															srcCopy/* drawing mode */, nullptr/* mask */);
									if (noErr != error)
									{
										Console_Warning(Console_WriteValue, "failed to stretch double-wide text image, error", error);
									}
									DisposeHandle(REINTERPRET_CAST(image, Handle)), image = nullptr;
								}
								
								UnlockPixels(pixels);
							}
						}
					}
					
					releaseRowIterator(inTerminalViewPtr, &lineIterator);
				}
			}
		}
		
		// reset these, they shouldn’t have significance outside the above loop
		inTerminalViewPtr->screen.currentRenderedLine = -1;
		inTerminalViewPtr->screen.currentRenderContext = nullptr;
		
		if (false == inTerminalViewPtr->isCocoa)
		{
			// undo the lock done earlier in this block
			UNUSED_RETURN(OSStatus)UnlockPortBits(currentPort);
		}
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
This is only intended to be used by drawVTGraphicsGlyph(),
to handle glyphs that happen to be Greek letters or other
Mac Roman characters that the Symbol font can render.

It is a TEMPORARY solution for the fact that the terminal
renderer cannot handle complete Unicode, because of
QuickDraw legacy.

(4.0)
*/
void
drawSymbolFontLetter	(My_TerminalViewPtr		UNUSED_ARGUMENT(inTerminalViewPtr),
						 CGContextRef			UNUSED_ARGUMENT(inDrawingContext),
						 CGRect const&			UNUSED_ARGUMENT(inBoundaries),
						 UniChar				UNUSED_ARGUMENT(inUnicode),
						 char					inMacRomanForQuickDraw) // DEPRECATED
{
	// Greek character; “cheat” and try to use the Symbol font for this
	// (changing the font on the fly is probably insanely inefficient,
	// but this is all a temporary hack anyway to make up for the lack
	// of full Unicode rendering support, thanks to QuickDraw legacy)
	if (FMGetFontFamilyFromName("\pSymbol") != kInvalidFontFamily)
	{
		// this is in Mac Roman encoding
		UInt8	text[] = { '\0' };
		SInt16	oldFontID = 0;
		SInt16	oldFontSize = 0;
		
		
		text[0] = inMacRomanForQuickDraw;
		
		{
			CGrafPtr	currentPort = nullptr;
			GDHandle	currentDevice = nullptr;
			
			
			GetGWorld(&currentPort, &currentDevice);
			oldFontID = GetPortTextFont(currentPort);
			oldFontSize = GetPortTextSize(currentPort);
		}
		TextFontByName("\pSymbol");
		TextSize(oldFontSize - oldFontSize / 6); // arbitrary heuristic; assume Symbol might be too big, shrink it proportionately
		DrawText(text, 0/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
		TextFont(oldFontID);
		TextSize(oldFontSize);
	}
	else
	{
		// this is in Mac Roman encoding
		UInt8	text[] = { '?' };
		
		
		DrawText(text, 0/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
	}
}// drawSymbolFontLetter


/*!
Draws the specified chunk of text in the given view
(line number 1 is the oldest line in the scrollback).

This routine expects to be called while the current
graphics port origin is equal to the screen origin.

For efficiency, this routine does not preserve or
restore state; do that on your own before invoking
Terminal_ForEachLikeAttributeRunDo().

(3.0)
*/
void
drawTerminalScreenRunOp		(TerminalScreenRef			UNUSED_ARGUMENT(inScreen),
							 UInt16						inLineTextBufferLength,
							 CFStringRef				inLineTextBufferAsCFStringOrNull,
							 Terminal_LineRef			UNUSED_ARGUMENT(inRow),
							 UInt16						inZeroBasedStartColumnNumber,
							 TextAttributes_Object		inAttributes,
							 void*						inTerminalViewPtr)
{
	My_TerminalViewPtr	viewPtr = REINTERPRET_CAST(inTerminalViewPtr, My_TerminalViewPtr);
	CGRect				sectionBounds;
	
	
	// set up context foreground and background colors appropriately
	// for the specified terminal attributes; this takes into account
	// things like bold and highlighted text, etc.
	useTerminalTextColors(viewPtr, viewPtr->screen.currentRenderContext, inAttributes, false/* is cursor */, 1.0/* alpha */);
	
	// erase and redraw the current rendering line, but only the
	// specified range (starting column and character count)
	eraseSection(viewPtr, viewPtr->screen.currentRenderContext,
					inZeroBasedStartColumnNumber, inZeroBasedStartColumnNumber + inLineTextBufferLength,
					sectionBounds);
	
	// draw the text or graphics
	if ((nullptr != inLineTextBufferAsCFStringOrNull) && (0 != inLineTextBufferLength))
	{
		Rect	intBounds;
		
		
		// TEMPORARY - for QuickDraw use only
		RegionUtilities_SetRect(&intBounds, STATIC_CAST(sectionBounds.origin.x, short), STATIC_CAST(sectionBounds.origin.y, short),
								STATIC_CAST(sectionBounds.origin.x + sectionBounds.size.width, short),
								STATIC_CAST(sectionBounds.origin.y + sectionBounds.size.height, short));
		
		// TEMPORARY...these kinds of offsets make no sense whatsoever
		// and yet Core Graphics drawings seem to be utterly misaligned
		// with QuickDraw backgrounds without them (NOTE: adjusting
		// width is not reasonable because it refers to the whole range,
		// which might encompass several characters)
		//sectionBounds.origin.x += 1;
		sectionBounds.size.height -= 3;
		drawTerminalText(viewPtr, viewPtr->screen.currentRenderContext, sectionBounds, intBounds,
							inLineTextBufferLength, inLineTextBufferAsCFStringOrNull, inAttributes);
		
		// since blinking forces frequent redraws, do not do it more
		// than necessary; keep track of any blink attributes, and
		// elsewhere this flag can be used to disable the animation
		// timer when it is not needed
		if (inAttributes.hasBlink())
		{
			RectRgn(gInvalidationScratchRegion(), &intBounds);
			UnionRgn(gInvalidationScratchRegion(), viewPtr->animation.rendering.region,
						viewPtr->animation.rendering.region);
			
			viewPtr->screen.currentRenderBlinking = true;
		}
	}
	
#if 0
	// DEBUGGING ONLY: try to see what column is drawn where, by rendering a number
	{
		char x[255];
		Str255 pstr;
		(int)std::snprintf(x, sizeof(x), "%d", inZeroBasedStartColumnNumber);
		StringUtilities_CToP(x, pstr);
		DrawString(pstr);
	}
#endif
#if 0
	// DEBUGGING ONLY: try to see what line is drawn where, by rendering a number
	{
		char x[255];
		Str255 pstr;
		(int)std::snprintf(x, sizeof(x), "%d", viewPtr->screen.currentRenderedLine);
		StringUtilities_CToP(x, pstr);
		DrawString(pstr);
	}
#endif
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

NOTE:	Despite the Unicode input, this routine is currently
		transitioning from QuickDraw and does not render all
		characters properly.

IMPORTANT:	The "inOldQuickDrawBoundaries" parameter should be
			equivalent to "inBoundaries", and will be removed
			in the future.  It is only for convenience when
			supporting old QuickDraw calls.  Also, QuickDraw
			code needs to correctly outset the bottom-right
			corner when making certain calls (e.g. filling
			rectangles, but not framing rectangles).

IMPORTANT:	The "inTextBufferAsCFString" parameter should be
			equivalent to the pair "inTextBufferPtr" and
			"inCharacterCount", and the CFStringRef be used
			wherever possible.  The latter two are only for
			legacy QuickDraw code and they will be removed.

(3.0)
*/
void
drawTerminalText	(My_TerminalViewPtr			inTerminalViewPtr,
					 CGContextRef				inDrawingContext,
					 CGRect const&				inBoundaries,
					 Rect const&				inOldQuickDrawBoundaries,
					 CFIndex					inCharacterCount,
					 CFStringRef				inTextBufferAsCFString,
					 TextAttributes_Object		inAttributes)
{
	if (inTerminalViewPtr->isCocoa)
	{
		// store new text attributes, for anything that refers to them
		inTerminalViewPtr->text.attributes = inAttributes;
		
		// font attributes are set directly on the (attributed) string
		// of the text storage, not in the graphics context
		setTextAttributesDictionary(inTerminalViewPtr, inTerminalViewPtr->text.attributeDict,
									inAttributes, 1.0/* alpha */);
		
		// draw the text with the correct attributes: font, etc.
		{
			NSAttributedString*		attributedString = [[[NSAttributedString alloc]
														initWithString:BRIDGE_CAST(inTextBufferAsCFString, NSString*)
																		attributes:inTerminalViewPtr->text.attributeDict]
																		autorelease];
			CFRetainRelease			lineObject(CTLineCreateWithAttributedString
												(BRIDGE_CAST(attributedString, CFAttributedStringRef)),
												CFRetainRelease::kAlreadyRetained);
			CTLineRef				asLineRef = REINTERPRET_CAST(lineObject.returnCFTypeRef(), CTLineRef);
			NSPoint					drawingLocation = NSZeroPoint;
			CGFloat					ascentMeasurement = 0;
			CGFloat					descentMeasurement = 0;
			CGFloat					leadingMeasurement = 0;
			Float64					lineWidth = 0;
			
			
			// measure the text’s layout so that it can be centered in the
			// background region that was chosen, regardless of how much
			// vertical space the text would otherwise require
			lineWidth = CTLineGetTypographicBounds(asLineRef, &ascentMeasurement, &descentMeasurement, &leadingMeasurement);
			drawingLocation = NSMakePoint(inBoundaries.origin.x,
											NSHeight([inTerminalViewPtr->contentNSView frame]) - inBoundaries.origin.y - ascentMeasurement - (leadingMeasurement / 2.0f));
			
			{
				CGContextSaveRestore	_(inDrawingContext);
				
				
				CGContextTranslateCTM(inDrawingContext, 0, NSHeight([inTerminalViewPtr->contentNSView frame]));
				CGContextScaleCTM(inDrawingContext, 1.0, -1.0);
				
				CGContextSetTextPosition(inDrawingContext, drawingLocation.x, drawingLocation.y);
				CTLineDraw(asLineRef, inDrawingContext);
				
				if (inAttributes.hasBold() &&
					(inTerminalViewPtr->text.font.boldFont == inTerminalViewPtr->text.font.normalFont))
				{
					// COMPLETE AND UTTER HACK: occasionally a font will have no bold version
					// in the same family and Cocoa does not seem as capable as QuickDraw in
					// terms of inventing a bold rendering for such fonts; as a work-around
					// text is drawn TWICE (the second at a slight offset from the original)
					drawingLocation.x += (1 + (inTerminalViewPtr->text.font.widthPerCharacter / 30)); // arbitrary
					
					CGContextSetTextPosition(inDrawingContext, drawingLocation.x, drawingLocation.y);
					CTLineDraw(asLineRef, inDrawingContext);
				}
			}
		}
	}
	else
	{
		// TEMPORARY: Unicode imaging is not supported yet, so the data
		// must first be converted into Mac Roman so QuickDraw can use it
		char const*		oldMacRomanBufferForQuickDraw = CFStringGetCStringPtr(inTextBufferAsCFString, kCFStringEncodingMacRoman);
		UInt8*			deletedBufferPtr = nullptr;
		
		
		if (nullptr == oldMacRomanBufferForQuickDraw)
		{
			// TEMPORARY (convert renderer to Unicode!)
			// not ideal, but if the internal buffer is not a byte array,
			// it must be copied before it can be interpreted that way
			deletedBufferPtr = new UInt8[inCharacterCount];
			
			CFIndex		bytesUsed = 0;
			CFIndex		conversionResult = CFStringGetBytes(inTextBufferAsCFString, CFRangeMake(0, inCharacterCount),
															kCFStringEncodingMacRoman, '?'/* loss byte */,
															false/* is external representation */,
															deletedBufferPtr, inCharacterCount, &bytesUsed);
			if (conversionResult > 0)
			{
				oldMacRomanBufferForQuickDraw = REINTERPRET_CAST(deletedBufferPtr, char*);
			}
		}
		
		if (nullptr == oldMacRomanBufferForQuickDraw)
		{
			Console_Warning(Console_WriteLine, "failed to render entire range because Mac Roman buffer was not found");
		}
		else
		{
			// draw all of the text, but scan for sub-sections that could be ANSI graphics
			SInt16		terminalFontID = 0;
			SInt16		terminalFontSize = 0;
			Style		terminalFontStyle = normal;
			
			
			// for better performance on Mac OS X, make the intended drawing area
			// part of the QuickDraw port’s dirty region
			if (inOldQuickDrawBoundaries.bottom >= 0)
			{
				CGrafPtr	currentPort = nullptr;
				GDHandle	currentDevice = nullptr;
				
				
				GetGWorld(&currentPort, &currentDevice);
				UNUSED_RETURN(OSStatus)QDAddRectToDirtyRegion(currentPort, &inOldQuickDrawBoundaries);
			}
			
			// set up the specified graphics context (and current QuickDraw port, for
			// legacy calls); the colors should already be set by the caller, but
			// this will add proper settings for font, style, etc. and set internal
			// flags that determine whether or not to use double-width and graphics
			useTerminalTextAttributes(inTerminalViewPtr, inDrawingContext, inAttributes);
			
			// get current font information (used to determine what the text should
			// look like - including “pseudo-font” and “pseudo-font-size” values
			// indicating VT graphics or double-width text, etc.
			{
				CGrafPtr	currentPort = nullptr;
				GDHandle	currentDevice = nullptr;
				
				
				GetGWorld(&currentPort, &currentDevice);
				terminalFontID = GetPortTextFont(currentPort);
				terminalFontSize = GetPortTextSize(currentPort);
				terminalFontStyle = GetPortTextFace(currentPort);
			}
			
			// position pen at start of text, on font baseline
			MoveTo(inOldQuickDrawBoundaries.left,
					inOldQuickDrawBoundaries.top +
						(inAttributes.hasAttributes(kTextAttributes_DoubleHeightBottom)
													? inTerminalViewPtr->text.font.doubleMetrics.ascent
													: inTerminalViewPtr->text.font.normalMetrics.ascent));
			
			// if bold or large text or graphics are being drawn, do it one character
			// at a time; bold fonts typically increase the font spacing, and double-
			// sized text needs to occupy twice the normal font spacing, so a regular
			// DrawText() won’t work; instead, each character must be drawn
			// individually, allowing MacTerm to correct the pen location after
			// each character *as if* a normal character were just rendered (this
			// ensures that everything remains aligned with text in the same column)
			if (terminalFontSize == kArbitraryDoubleWidthDoubleHeightPseudoFontSize)
			{
				// top half of double-sized text; this is not rendered, but the pen should move double the distance anyway
				Move(STATIC_CAST(inCharacterCount * INTEGER_TIMES_2(inTerminalViewPtr->text.font.widthPerCharacter), SInt16), 0);
			}
			else if (terminalFontSize == inTerminalViewPtr->text.font.doubleMetrics.size)
			{
				// bottom half of double-sized text; force the text to use
				// twice the normal font metrics
				CGFloat const	kHOffsetPerGlyph = INTEGER_TIMES_2(inTerminalViewPtr->text.font.widthPerCharacter);
				SInt16			i = 0;
				Point			oldPen;
				CGRect			glyphBounds = CGRectMake(inBoundaries.origin.x - 2, inBoundaries.origin.y - 2,
															kHOffsetPerGlyph + 4,
															inBoundaries.size.height/*INTEGER_TIMES_2(inTerminalViewPtr->text.font.heightPerCharacter)*/ + 4);
				
				
				for (i = 0; i < inCharacterCount; ++i)
				{
					GetPen(&oldPen);
					if (terminalFontID == kArbitraryVTGraphicsPseudoFontID)
					{
						// draw a graphics character
						drawVTGraphicsGlyph(inTerminalViewPtr, inDrawingContext, glyphBounds,
											CFStringGetCharacterAtIndex(inTextBufferAsCFString, i),
											oldMacRomanBufferForQuickDraw[i], oldPen.v - glyphBounds.origin.y, inAttributes);
					}
					else
					{
						// draw a normal character
						DrawText(oldMacRomanBufferForQuickDraw, i/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
					}
					
					glyphBounds.origin.x += kHOffsetPerGlyph;
					MoveTo(oldPen.h + STATIC_CAST(kHOffsetPerGlyph, SInt16), oldPen.v);
				}
			}
			else if ((terminalFontStyle & bold) || (terminalFontID == kArbitraryVTGraphicsPseudoFontID) ||
						(false == inTerminalViewPtr->text.font.isMonospaced) ||
						(1.0 != inTerminalViewPtr->text.font.scaleWidthPerCharacter))
			{
				// proportional font, or bold, or otherwise non-standard width; force the text
				// to draw one character at a time so that the character offset can be corrected
				CGFloat const	kHOffsetPerGlyph = inTerminalViewPtr->text.font.widthPerCharacter;
				SInt16			i = 0;
				char			previousChar = '\0'; // aids heuristic algorithm; certain letter combinations may demand different offsets
				Point			oldPen;
				CGRect			glyphBounds = CGRectMake(inBoundaries.origin.x - 2, inBoundaries.origin.y - 2,
															kHOffsetPerGlyph + 4,
															inBoundaries.size.height/*inTerminalViewPtr->text.font.heightPerCharacter*/ + 4);
				
				
				for (i = 0; i < inCharacterCount; ++i)
				{
					char const	thisChar = *(oldMacRomanBufferForQuickDraw + i);
					SInt16		offset = 0;
					
					
					if (false == inTerminalViewPtr->text.font.isMonospaced)
					{
						// the following is completely arbitrary, a heuristic; in an attempt to make
						// font display nicer when proportional fonts are shoehorned into monospace
						// layout, ASSUME that certain characters will TEND to be an unusual size,
						// and nudge them a bit to center them in the fixed-size cell that they use
						// (note: these are pixel-based instead of being proportional to the width,
						// so at small or very large font sizes they won’t really work as intended)
						switch (thisChar)
						{
						case ':':
						case ';':
						case '.':
						case ',':
						case '-':
						case '"':
						case '\'':
						case '/':
						case '\\':
							++offset;
							break;
						case 'c':
						case 'e':
						case 'r':
						case 's':
						case 't':
							++offset;
							if (std::isupper(previousChar))
							{
								++offset;
							}
							break;
						case 'i':
						case 'l':
						case '[':
						case '{':
						case '(':
							offset += 2;
							break;
						case 'I':
							offset += 3;
							break;
						case 'C':
						case 'O':
						case 'T':
							if (false == std::isupper(previousChar))
							{
								--offset;
							}
							break;
						case 'Q':
						case 'U':
							if (false == std::isupper(previousChar))
							{
								offset -= 2;
							}
							break;
						case 'w':
							offset -= 2;
							break;
						case 'm':
						case 'M':
						case 'W':
							offset -= 3;
							break;
						default:
							break;
						}
					}
					
					GetPen(&oldPen);
					if (offset != 0)
					{
						MoveTo(oldPen.h + offset, oldPen.v);
					}
					if (terminalFontID == kArbitraryVTGraphicsPseudoFontID)
					{
						// draw a graphics character
						drawVTGraphicsGlyph(inTerminalViewPtr, inDrawingContext, glyphBounds,
											CFStringGetCharacterAtIndex(inTextBufferAsCFString, i),
											oldMacRomanBufferForQuickDraw[i], oldPen.v - glyphBounds.origin.y, inAttributes);
					}
					else
					{
						// draw a normal character
						DrawText(oldMacRomanBufferForQuickDraw, i/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
					}
					glyphBounds.origin.x += kHOffsetPerGlyph;
					MoveTo(oldPen.h + STATIC_CAST(kHOffsetPerGlyph, SInt16), oldPen.v);
					
					previousChar = thisChar;
				}
			}
			else
			{
				// *completely* normal text (no special style, size, or character set); this is probably
				// fastest if rendered all at once using a single QuickDraw call, and since there are no
				// forced font metrics with normal text, this can be a lot simpler (this is also almost
				// certainly the common case, so it’s good if this is as efficient as possible)
				DrawText(oldMacRomanBufferForQuickDraw, 0/* offset */, STATIC_CAST(inCharacterCount, short)); // draw text using current font, size, color, etc.
			}
		}
		
		if (nullptr != deletedBufferPtr)
		{
			delete [] deletedBufferPtr, deletedBufferPtr = nullptr;
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

The specified Unicode character is redundantly specified as
a pre-translated Mac Roman character (or some loss byte if
none is appropriate), for the TEMPORARY use of old drawing
code that must use QuickDraw.

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
						 UniChar					inUnicode,
						 char						inMacRomanForQuickDraw, // DEPRECATED
						 CGFloat					inBaselineHint,
						 TextAttributes_Object		inAttributes)
{
	CGFloat const					kMaxWidthInPixelsForSmallSizeGlyphs = 10; // arbitrary; when to switch to small-size renderings
	TerminalGlyphDrawing_Options	drawingOptions = 0;
	// float parameters are used for improved renderings with Core Graphics
	CGContextSaveRestore			_(inDrawingContext);
	TerminalGlyphDrawing_Cache*		sourceLayerCache = nil; // may be set below
	CGRect							floatBounds = inBoundaries;
	CGFloat							floatCellTop = inBoundaries.origin.y;
	CGFloat							floatCellLeft = inBoundaries.origin.x;
	CGFloat							floatCellRight = (inBoundaries.origin.x + inBoundaries.size.width);
	CGFloat							floatCellBottom = (inBoundaries.origin.y + inBoundaries.size.height);
	CGColorRef						foregroundColor = nullptr;
	NSColor*						foregroundNSColor = nil;
	// legacy metrics for QuickDraw code (will be removed eventually)
	SInt16							preservedFontID = 0;
	Boolean							noDefaultRender = false;
	Rect							cellRect; // set later...
	Point							cellCenter; // used for line drawing glyphs
	SInt16							cellTop = STATIC_CAST(floatCellTop, SInt16);
	SInt16							cellLeft = STATIC_CAST(floatCellLeft, SInt16);
	SInt16							cellRight = STATIC_CAST(floatCellRight, SInt16);
	SInt16							cellBottom = STATIC_CAST(floatCellBottom, SInt16);
	SInt16							lineWidth = 1; // changed later...
	SInt16							lineHeight = 1; // changed later...
	
	
	// TEMPORARY; since original bounds are based in QuickDraw
	// coordinates, must adjust to produce equivalent results
	// in Core Graphics (for example, when the background is
	// filled for text selection, it must appear to be in
	// exactly the same pixels that Core Graphics would use to
	// draw vector graphics); definitely a hack...
	floatBounds.origin.x += 1.0;
	//floatBounds.origin.y += 1.0;
	floatBounds.size.width -= 3.0;
	//floatBounds.size.height -= 1.0;
	
	if (inAttributes.hasBold())
	{
		drawingOptions |= kTerminalGlyphDrawing_OptionBold;
	}
	
	if (floatBounds.size.width < kMaxWidthInPixelsForSmallSizeGlyphs)
	{
		drawingOptions |= kTerminalGlyphDrawing_OptionSmallSize;
	}
	
	// TEMPORARY; set now to ensure correct values (later, when
	// removing Carbon support entirely, this can probably be
	// done earlier and persist for all other calls implicitly)
	setTextAttributesDictionary(inTerminalViewPtr, inTerminalViewPtr->text.attributeDict,
								inAttributes, 1.0/* alpha */);
	foregroundNSColor = STATIC_CAST([inTerminalViewPtr->text.attributeDict objectForKey:NSForegroundColorAttributeName],
									NSColor*);
	{
		NSColor*	asRGB = [foregroundNSColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
		
		
		assert(nil != asRGB);
		foregroundColor = CGColorCreateGenericRGB(asRGB.redComponent, asRGB.greenComponent, asRGB.blueComponent,
													asRGB.alphaComponent);
	}
	
	// preserve font
	{
		CGrafPtr	currentPort = nullptr;
		GDHandle	currentDevice = nullptr;
		
		
		GetGWorld(&currentPort, &currentDevice);
		preservedFontID = GetPortTextFont(currentPort);
	}
	
	// Since a “pseudo-font” is used to indicate VT graphics mode,
	// the current port’s font will NOT be correct for normal text.
	// Many VT graphics glyphs and ALL non-graphics characters must
	// be drawn as regular text, so here the port font is temporarily
	// restored in case DrawText() must be called.  However, since
	// the pseudo-font-ID is a terminal text MODE indicator, it is
	// restored before this routine returns!
	TextFontByName(inTerminalViewPtr->text.font.familyName);
	
	// set metrics
	{
		PenState	penState;
		
		
		GetPenState(&penState);
		lineWidth = penState.pnSize.h;
		lineHeight = penState.pnSize.v;
	}
	RegionUtilities_SetRect(&cellRect, cellLeft, cellTop, cellRight, cellBottom);
	RegionUtilities_SetPoint(&cellCenter, cellLeft + STATIC_CAST(INTEGER_DIV_2(cellRight - cellLeft), SInt16),
								cellTop + STATIC_CAST(INTEGER_DIV_2(cellBottom - cellTop), SInt16));
	
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
	
	// pre-check certain ranges
	if ((inUnicode >= 0x2800) && (inUnicode <= 0x28FF))
	{
		// handle below (after switch)
		noDefaultRender = true;
	}
	
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
	
	case 0x0192: // small 'f' with hook
	#if 0
		MoveTo(cellCenter.h - lineWidth * 3/* arbitrary */, cellBottom - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellBottom - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellTop + lineHeight);
		LineTo(cellCenter.h + lineWidth * 3/* arbitrary */, cellTop + lineHeight);
		MoveTo(cellLeft + lineWidth, cellCenter.v - lineHeight);
		LineTo(cellCenter.h + lineWidth * 2/* arbitrary */, cellCenter.v - lineHeight);
	#else
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0xA6);
	#endif
		break;
	
	case 0x0391: // capital alpha
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x41);
		break;
	
	case 0x0392: // capital beta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x42);
		break;
	
	case 0x0393: // capital gamma
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x47);
		break;
	
	case 0x0394: // capital delta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x44);
		break;
	
	case 0x0395: // capital epsilon
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x45);
		break;
	
	case 0x0396: // capital zeta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x5A);
		break;
	
	case 0x0397: // capital eta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x48);
		break;
	
	case 0x0398: // capital theta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x51);
		break;
	
	case 0x0399: // capital iota
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x49);
		break;
	
	case 0x039A: // capital kappa
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x4B);
		break;
	
	case 0x039B: // capital lambda
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x4C);
		break;
	
	case 0x039C: // capital mu
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x4D);
		break;
	
	case 0x039D: // capital nu
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x4E);
		break;
	
	case 0x039E: // capital xi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x58);
		break;
	
	case 0x039F: // capital omicron
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x4F);
		break;
	
	case 0x03A0: // capital pi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x50);
		break;
	
	case 0x03A1: // capital rho
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x52);
		break;
	
	case 0x03A3: // capital sigma
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x53);
		break;
	
	case 0x03A4: // capital tau
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x54);
		break;
	
	case 0x03A5: // capital upsilon
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x55);
		break;
	
	case 0x03A6: // capital phi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x46);
		break;
	
	case 0x03A7: // capital chi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x43);
		break;
	
	case 0x03A8: // capital psi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x59);
		break;
	
	case 0x03A9: // capital omega
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x57);
		break;
	
	case 0x03B1: // alpha
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x61);
		break;
	
	case 0x03B2: // beta
	case 0x00DF: // small sharp "S" (TEMPORARY; render as a beta due to similarity)
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x62);
		break;
	
	case 0x03B3: // gamma
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x67);
		break;
	
	case 0x03B4: // delta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x64);
		break;
	
	case 0x03B5: // epsilon
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x65);
		break;
	
	case 0x03B6: // zeta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x7A);
		break;
	
	case 0x03B7: // eta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x68);
		break;
	
	case 0x03B8: // theta
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
						 	 0x71);
		break;
	
	case 0x03B9: // iota
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x69);
		break;
	
	case 0x03BA: // kappa
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x6B);
		break;
	
	case 0x03BB: // lambda
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x6C);
		break;
	
	case 0x00B5: // mu
	case 0x03BC: // mu
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x6D);
		break;
	
	case 0x03BD: // nu
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x6E);
		break;
	
	case 0x03BE: // xi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x78);
		break;
	
	case 0x03BF: // omicron
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x6F);
		break;
	
	case 0x03C0: // pi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x70);
		break;
	
	case 0x03C1: // rho
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x72);
		break;
	
	case 0x03C2: // final sigma
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x56);
		break;
	
	case 0x03C3: // sigma
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x73);
		break;
	
	case 0x03C4: // tau
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x74);
		break;
	
	case 0x03C5: // upsilon
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x75);
		break;
	
	case 0x03C6: // phi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x66);
		break;
	
	case 0x03C7: // chi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x63);
		break;
	
	case 0x03C8: // psi
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x79);
		break;
	
	case 0x03C9: // omega
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x77);
		break;
	
	case 0x03D1: // theta (symbol)
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x4A);
		break;
	
	case 0x03D5: // phi (symbol)
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x6A);
		break;
	
	case 0x03D6: // pi (symbol)
		drawSymbolFontLetter(inTerminalViewPtr, inDrawingContext, floatBounds, inUnicode,
							 0x76);
		break;
	
	default:
		// non-graphics character
		unless (noDefaultRender)
		{
			// this is in Mac Roman encoding
			UInt8	text[] = { '\0' };
			
			
			text[0] = inMacRomanForQuickDraw;
			
			DrawText(text, 0/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
		}
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
	
	// restore font
	TextFont(preservedFontID);
	
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
	Rect	intBounds;
	
	
	getRowSectionBounds(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderedLine,
						inLeftmostColumnToErase, inPastRightmostColumnToErase - inLeftmostColumnToErase,
						&intBounds);
	outRowSectionBounds = CGRectMake(intBounds.left, intBounds.top, intBounds.right - intBounds.left,
										intBounds.bottom - intBounds.top);
	if (CGRectIsEmpty(outRowSectionBounds))
	{
		Console_Warning(Console_WriteValueFloat4, "attempt to erase empty row section",
						outRowSectionBounds.origin.x, outRowSectionBounds.origin.y,
						outRowSectionBounds.size.width, outRowSectionBounds.size.height);
	}
	
	if (false == inTerminalViewPtr->screen.currentRenderNoBackground)
	{
		if (inTerminalViewPtr->isCocoa)
		{
			CGContextFillRect(inDrawingContext, CGRectMake(outRowSectionBounds.origin.x - 0.5f, outRowSectionBounds.origin.y - 0.5f,
															outRowSectionBounds.size.width + 1, outRowSectionBounds.size.height + 1));
		}
		else
		{
			EraseRect(&intBounds);
		}
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
Finds the cell position in the visible screen area of
the indicated window that is closest to the given point.

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

(3.1)
*/
Boolean
findVirtualCellFromLocalPoint	(My_TerminalViewPtr		inTerminalViewPtr,
								 Point					inLocalPixelPosition,
								 TerminalView_Cell&		outCell,
								 SInt16&				outDeltaColumn,
								 SInt16&				outDeltaRow)
{
	Boolean		result = true;
	SInt16		offsetH = inLocalPixelPosition.h;
	SInt16		offsetV = inLocalPixelPosition.v;
	
	
	// adjust point to fit in local screen area
	localToScreen(inTerminalViewPtr, &offsetH, &offsetV);
	{
		HIPoint		screenPixelPosition = CGPointMake(offsetH, offsetV);
		Float32		deltaColumn = 0.0;
		Float32		deltaRow = 0.0;
		
		
		result = findVirtualCellFromScreenPoint(inTerminalViewPtr, screenPixelPosition, outCell,
												deltaColumn, deltaRow);
		outDeltaColumn = STATIC_CAST(deltaColumn, SInt16); // TEMPORARY; lost precision here
		outDeltaRow = STATIC_CAST(deltaRow, SInt16);
	}
	
#if 0
	// TEMPORARY - for debugging purposes, could display the coordinates
	{
	Str31 x, y;
	MoveTo(inLocalPixelPosition.h, inLocalPixelPosition.v);
	NumToString(outCell.first, x);
	NumToString(outCell.second, y);
	DrawString(x);
	DrawString("\p,");
	DrawString(y);
	}
#endif
	
	return result;
}// findVirtualCellFromLocalPoint


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
								 Float32&				outDeltaColumn,
								 Float32&				outDeltaRow)
{
	Boolean		result = true;
	SInt32		columnCalculation = 0;
	SInt32		rowCalculation = 0;
	
	
	outDeltaColumn = 0;
	outDeltaRow = 0;
	
	// adjust point to fit in local screen area
	{
		// NOTE: This code starts in units of pixels for convenience,
		// but must convert to units of columns and rows upon return.
		Float32		offsetH = inScreenLocalPixelPosition.x;
		Float32		offsetV = inScreenLocalPixelPosition.y;
		
		
		if (offsetH < 0)
		{
			result = false;
			outDeltaColumn = offsetH;
			columnCalculation = 0;
		}
		else
		{
			columnCalculation = STATIC_CAST(offsetH, SInt16);
		}
		
		if (offsetV < 0)
		{
			result = false;
			outDeltaRow = offsetV;
			rowCalculation = 0;
		}
		else
		{
			rowCalculation = STATIC_CAST(offsetV, SInt32);
		}
	}
	
	if (rowCalculation > STATIC_CAST(inTerminalViewPtr->screen.cache.viewHeightInPixels, SInt32))
	{
		result = false;
		outDeltaRow = rowCalculation - inTerminalViewPtr->screen.cache.viewHeightInPixels;
		rowCalculation = inTerminalViewPtr->screen.cache.viewHeightInPixels;
	}
	
	// TEMPORARY: this needs to take into account double-height text
	rowCalculation /= inTerminalViewPtr->text.font.heightPerCharacter;
	outDeltaRow /= inTerminalViewPtr->text.font.heightPerCharacter;
	
	if (columnCalculation > inTerminalViewPtr->screen.cache.viewWidthInPixels)
	{
		result = false;
		outDeltaColumn = columnCalculation - inTerminalViewPtr->screen.cache.viewWidthInPixels;
		columnCalculation = inTerminalViewPtr->screen.cache.viewWidthInPixels;
	}
	
	{
		SInt16 const	kWidthPerCharacter = getRowCharacterWidth(inTerminalViewPtr, rowCalculation);
		
		
		columnCalculation /= kWidthPerCharacter;
		outDeltaColumn /= kWidthPerCharacter;
	}
	
	columnCalculation += inTerminalViewPtr->screen.leftVisibleEdgeInColumns;
	rowCalculation += inTerminalViewPtr->screen.topVisibleEdgeInRows;
	
	outCell.first = STATIC_CAST(columnCalculation, SInt16);
	outCell.second = rowCalculation;
	
	return result;
}// findVirtualCellFromScreenPoint


/*!
Given a stage of blink animation, returns its rendering color.

(4.0)
*/
inline void
getBlinkAnimationColor	(My_TerminalViewPtr		inTerminalViewPtr,
						 UInt16					inAnimationStage,
						 CGDeviceColor*			outColorPtr)
{
	*outColorPtr = inTerminalViewPtr->blinkColors[inAnimationStage];
}// getBlinkAnimationColor


/*!
Calculates the boundaries of the given row in pixels
relative to the SCREEN, so if you passed row 0, the
top-left corner of the rectangle would be (0, 0), but
the other boundaries depend on the font and font size.

This routine respects the top-half-double-height and
bottom-half-double-height row global attributes, such
that a double-size line is twice as big as a normal
line’s boundaries.

To convert this to coordinates local to the window,
use screenToLocalRect().

(3.0)
*/
void
getRowBounds	(My_TerminalViewPtr		inTerminalViewPtr,
				 TerminalView_RowIndex	inZeroBasedRowIndex,
				 Rect*					outBoundsPtr)
{
	Terminal_LineStackStorage	rowIteratorData;
	Terminal_LineRef			rowIterator = nullptr;
	SInt16						sectionTopEdge = STATIC_CAST(inZeroBasedRowIndex * inTerminalViewPtr->text.font.heightPerCharacter, SInt16);
	TerminalView_RowIndex		topRow = 0;
	
	
	// start with the interior bounds, as this defines two of the edges
	if (inTerminalViewPtr->isCocoa)
	{
		NSRect		contentFrame = [inTerminalViewPtr->contentNSView frame];
		
		
		outBoundsPtr->right = STATIC_CAST(contentFrame.size.width, SInt16);
		outBoundsPtr->left = 0;
	}
	else
	{
		HIRect		contentFrame;
		OSStatus	error = HIViewGetBounds(inTerminalViewPtr->contentHIView, &contentFrame);
		
		
		assert_noerr(error);
		outBoundsPtr->right = STATIC_CAST(contentFrame.size.width, SInt16);
		outBoundsPtr->left = 0;
	}
	
	// now set the top and bottom edges based on the requested row
	outBoundsPtr->top = sectionTopEdge;
	outBoundsPtr->bottom = sectionTopEdge + inTerminalViewPtr->text.font.heightPerCharacter;
	
	// account for double-height rows
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
				outBoundsPtr->bottom = outBoundsPtr->top + STATIC_CAST(INTEGER_TIMES_2(outBoundsPtr->bottom - outBoundsPtr->top), SInt16);
			}
			else if (globalAttributes.hasDoubleHeightBottom())
			{
				// if this is the bottom half, the total boundaries extend upwards by one normal line height
				outBoundsPtr->top = outBoundsPtr->bottom - STATIC_CAST(INTEGER_TIMES_2(outBoundsPtr->bottom - outBoundsPtr->top), SInt16);
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
SInt16
getRowCharacterWidth	(My_TerminalViewPtr		inTerminalViewPtr,
						 TerminalView_RowIndex	inLineNumber)
{
	TextAttributes_Object		globalAttributes;
	Terminal_LineStackStorage	rowIteratorData;
	Terminal_LineRef			rowIterator = nullptr;
	SInt16						result = inTerminalViewPtr->text.font.widthPerCharacter;
	
	
	rowIterator = findRowIterator(inTerminalViewPtr, inLineNumber, &rowIteratorData);
	if (nullptr != rowIterator)
	{
		UNUSED_RETURN(Terminal_Result)Terminal_GetLineGlobalAttributes(inTerminalViewPtr->screen.ref, rowIterator, &globalAttributes);
		if (globalAttributes.hasDoubleAny())
		{
			result = STATIC_CAST(INTEGER_TIMES_2(result), SInt16);
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

To convert this to coordinates local to the window,
use screenToLocalRect().

(3.0)
*/
void
getRowSectionBounds		(My_TerminalViewPtr		inTerminalViewPtr,
						 TerminalView_RowIndex	inZeroBasedRowIndex,
						 UInt16					inZeroBasedStartingColumnNumber,
						 SInt16					inCharacterCount,
						 Rect*					outBoundsPtr)
{
	SInt16		widthPerCharacter = getRowCharacterWidth(inTerminalViewPtr, inZeroBasedRowIndex);
	
	
	// first find the row bounds, as this defines two of the resulting edges
	getRowBounds(inTerminalViewPtr, inZeroBasedRowIndex, outBoundsPtr);
	
	// now set the rectangle’s left and right edges based on the requested column range
	outBoundsPtr->left = inZeroBasedStartingColumnNumber * widthPerCharacter;
	outBoundsPtr->right = (inZeroBasedStartingColumnNumber + inCharacterCount) * widthPerCharacter;
}// getRowSectionBounds


/*!
Returns a color stored internally in the view data structure.

(3.0)
*/
inline void
getScreenBaseColor	(My_TerminalViewPtr			inTerminalViewPtr,
					 TerminalView_ColorIndex	inColorEntryNumber,
					 CGDeviceColor*				outColorPtr)
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
								 CGDeviceColor*				outForeColorPtr,
								 CGDeviceColor*				outBackColorPtr,
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
		if (inAttributes.hasAttributes(kTextAttributes_ColorIndexIsTrueColorID))
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
		std::swap< CGDeviceColor >(*outForeColorPtr, *outBackColorPtr);
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
					 CGDeviceColor*			outColorPtr)
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
		My_XTerm256Table::makeCGDeviceColor(sourceGrid.colorLevels[kColorLevelsKey][0],
											sourceGrid.colorLevels[kColorLevelsKey][1],
											sourceGrid.colorLevels[kColorLevelsKey][2], *outColorPtr);
		result = true;
	}
	else if (sourceGrid.grayLevels.end() !=
				sourceGrid.grayLevels.find(kGrayLevelsKey))
	{
		// one of the standard grays
		//Console_WriteValue("gray", sourceGrid.grayLevels[kGrayLevelsKey]);
		My_XTerm256Table::makeCGDeviceColor(sourceGrid.grayLevels[kGrayLevelsKey],
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
						 CGDeviceColor*				outColorPtr)
{
	*outColorPtr = inTerminalViewPtr->customColors[inColorEntryNumber];
}// getScreenCustomColor


/*!
Returns the origin, in window content view coordinates
(which are the same as QuickDraw local coordinates), of
the screen interior (i.e. taking into account any insets).
Since the user has control over collapsing the window
header, etc. you need to call this routine to get the
current origin.

Deprecated.  Use getScreenOriginFloat() for more precision
and less casting.

(3.0)
*/
void
getScreenOrigin		(My_TerminalViewPtr		inTerminalViewPtr,
					 SInt16*				outScreenPositionX,
					 SInt16*				outScreenPositionY)
{
	Float32		x = 0;
	Float32		y = 0;
	
	
	getScreenOriginFloat(inTerminalViewPtr, x, y);
	*outScreenPositionX = STATIC_CAST(x, SInt16);
	*outScreenPositionY = STATIC_CAST(y, SInt16);
}// getScreenOrigin


/*!
Returns the origin, in window content view coordinates
(which are the same as QuickDraw local coordinates), of
the screen interior (i.e. taking into account any insets).
Since the user has control over collapsing the window
header, etc. you need to call this routine to get the
current origin.

Unlike getScreenOrigin(), this returns floating-point
values, suitable for a CGPoint or HIPoint.

(3.1)
*/
void
getScreenOriginFloat	(My_TerminalViewPtr		inTerminalViewPtr,
						 Float32&				outScreenPositionX,
						 Float32&				outScreenPositionY)
{
	if (inTerminalViewPtr->isCocoa)
	{
		
	}
	else
	{
		HIViewWrap	windowContentView(kHIViewWindowContentID, HIViewGetWindow(inTerminalViewPtr->contentHIView));
		HIRect		contentFrame;
		OSStatus	error = noErr;
		
		
		assert(windowContentView.exists());
		error = HIViewGetFrame(inTerminalViewPtr->contentHIView, &contentFrame);
		assert_noerr(error);
		error = HIViewConvertRect(&contentFrame, HIViewGetSuperview(inTerminalViewPtr->contentHIView), windowContentView);
		assert_noerr(error);
		outScreenPositionX = contentFrame.origin.x;
		outScreenPositionY = contentFrame.origin.y;
	}
}// getScreenOriginFloat


/*!
Internal version of TerminalView_ReturnSelectedTextAsNewHandle().

DEPRECATED.  Use returnSelectedTextCopyAsUnicode() instead.

(3.0)
*/
Handle
getSelectedTextAsNewHandle	(My_TerminalViewPtr			inTerminalViewPtr,
							 UInt16						inMaxSpacesToReplaceWithTabOrZero,
							 TerminalView_TextFlags		inFlags)
{
    Handle		result = nullptr;
	
	
	if (inTerminalViewPtr->text.selection.exists)
	{
		TerminalView_Cell const&	kSelectionStart = inTerminalViewPtr->text.selection.range.first;
		TerminalView_Cell const&	kSelectionPastEnd = inTerminalViewPtr->text.selection.range.second;
		size_t const				kByteCount = getSelectedTextSize(inTerminalViewPtr);
		
		
		result = Memory_NewHandle(kByteCount);
		if (nullptr != result)
		{
			Terminal_Result				copyResult = kTerminal_ResultOK;
			Terminal_LineStackStorage	lineIteratorData;
			Terminal_LineRef			lineIterator = findRowIterator(inTerminalViewPtr, kSelectionStart.second,
																		&lineIteratorData);
			Terminal_TextCopyFlags		flags = 0L;
			char*						characters = nullptr;
			SInt32						actualLength = 0L;
			
			
			HLock(result);
			characters = *result;
			
			if (inTerminalViewPtr->text.selection.isRectangular) flags |= kTerminal_TextCopyFlagsRectangular;
			
			copyResult = Terminal_CopyRange(inTerminalViewPtr->screen.ref, lineIterator,
											kSelectionPastEnd.second - kSelectionStart.second,
											kSelectionStart.first, kSelectionPastEnd.first - 1/* make inclusive range */,
											characters, kByteCount, &actualLength,
											(inFlags & kTerminalView_TextFlagInline) ? "" : "\015",
											inMaxSpacesToReplaceWithTabOrZero, flags);
			releaseRowIterator(inTerminalViewPtr, &lineIterator);
			HUnlock(result);
			
			// signal fatal errors with a nullptr handle; a lack of
			// space is not considered fatal, instead the truncated
			// string is returned to the caller
			if ((copyResult == kTerminal_ResultOK) ||
				(copyResult == kTerminal_ResultNotEnoughMemory))
			{
				UNUSED_RETURN(OSStatus)Memory_SetHandleSize(result, actualLength * sizeof(char));
			}
			else
			{
				Memory_DisposeHandle(&result);
			}
		}
	}
	
	return result;
}// getSelectedTextAsNewHandle


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
Internal version of TerminalView_ReturnSelectedTextAsNewRegion().

DEPRECATED.  Use getSelectedTextAsNewRegionOnScreen() instead.

(2.6)
*/
RgnHandle
getSelectedTextAsNewRegion		(My_TerminalViewPtr		inTerminalViewPtr)
{
	RgnHandle	result = getVirtualRangeAsNewRegion(inTerminalViewPtr, inTerminalViewPtr->text.selection.range.first,
													inTerminalViewPtr->text.selection.range.second,
													inTerminalViewPtr->text.selection.isRectangular);
	
	
	return result;
}// getSelectedTextAsNewRegion


/*!
Like getVirtualRangeAsNewRegionOnScreen(), except specifically
for the current text selection.  Automatically takes into
account rectangular shape, if applicable.

(3.1)
*/
RgnHandle
getSelectedTextAsNewRegionOnScreen		(My_TerminalViewPtr		inTerminalViewPtr)
{
	RgnHandle	result = getVirtualRangeAsNewRegionOnScreen(inTerminalViewPtr, inTerminalViewPtr->text.selection.range.first,
															inTerminalViewPtr->text.selection.range.second,
															inTerminalViewPtr->text.selection.isRectangular);
	
	
	return result;
}// getSelectedTextAsNewRegionOnScreen


/*!
Returns the size in bytes of the current selection for
the specified window, or zero.

(3.1)
*/
size_t
getSelectedTextSize		(My_TerminalViewPtr		inTerminalViewPtr)
{
    size_t		result = 0L;
	
	
	if (inTerminalViewPtr->text.selection.exists)
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
	HIRect				screenBounds;
	TerminalView_Cell	selectionStart;
	TerminalView_Cell	selectionPastEnd;
	OSStatus			error = noErr;
	
	
	// find clipping region
	error = HIViewGetBounds(inTerminalViewPtr->contentHIView, &screenBounds);
	assert_noerr(error);
	
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
		// then the area to be highlighted is a rectangle; this simplifies things...
		CGRect		clippedRect;
		CGRect		selectionBounds;
		
		
		// make the points the “right way around”, in case the first point is
		// technically to the right of or below the second point
		sortAnchors(selectionStart, selectionPastEnd, true/* is a rectangular selection */);
		
		// set up rectangle bounding area to be highlighted
		selectionBounds = CGRectMake(selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter,
										selectionStart.second * inTerminalViewPtr->text.font.heightPerCharacter,
										(selectionPastEnd.first - selectionStart.first) * inTerminalViewPtr->text.font.widthPerCharacter,
										(selectionPastEnd.second - selectionStart.second) * inTerminalViewPtr->text.font.heightPerCharacter);
		
		// the final selection region is the portion of the full rectangle
		// that fits within the current screen boundaries
		clippedRect = CGRectIntegral(CGRectIntersection(selectionBounds, screenBounds));
		if (0.0 != inInsets)
		{
			clippedRect = CGRectInset(clippedRect, inInsets, inInsets);
		}
		result = HIShapeCreateWithRect(&clippedRect);
	}
	else
	{
		// then the area to be highlighted is irregularly shaped; this is more complex...
		HIMutableShapeRef	mutableResult = nullptr;
		HIShapeRef			rectShape = nullptr;
		CGRect				clippedRect;
		CGRect				partialSelectionBounds;
		
		
		mutableResult = HIShapeCreateMutable();
		
		// make the points the “right way around”, in case the first point is
		// technically to the right of or below the second point
		sortAnchors(selectionStart, selectionPastEnd, false/* is a rectangular selection */);
		
		// bounds of first (possibly partial) line to be highlighted
		{
			// NOTE: vertical insets are applied to end caps as extensions since the middle piece vertically shrinks
			partialSelectionBounds = CGRectMake(selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter + inInsets,
												selectionStart.second * inTerminalViewPtr->text.font.heightPerCharacter + inInsets,
												inTerminalViewPtr->screen.cache.viewWidthInPixels -
													selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter -
													2.0f * inInsets,
												inTerminalViewPtr->text.font.heightPerCharacter/* no insets here, due to shrunk mid-section */);
			clippedRect = CGRectIntegral(CGRectIntersection(partialSelectionBounds, screenBounds)); // clip to constraint rectangle
			rectShape = HIShapeCreateWithRect(&clippedRect);
			if (nullptr != rectShape)
			{
				HIShapeUnion(mutableResult, rectShape, mutableResult);
				CFRelease(rectShape), rectShape = nullptr;
			}
		}
		
		// bounds of last (possibly partial) line to be highlighted
		{
			// NOTE: vertical insets are applied to end caps as extensions since the middle piece vertically shrinks
			partialSelectionBounds = CGRectMake(inInsets,
												(selectionPastEnd.second - 1) * inTerminalViewPtr->text.font.heightPerCharacter - inInsets,
												selectionPastEnd.first * inTerminalViewPtr->text.font.widthPerCharacter - 2.0f * inInsets,
												inTerminalViewPtr->text.font.heightPerCharacter/* no insets here, due to shrunk mid-section */);
			clippedRect = CGRectIntegral(CGRectIntersection(partialSelectionBounds, screenBounds)); // clip to constraint rectangle
			rectShape = HIShapeCreateWithRect(&clippedRect);
			if (nullptr != rectShape)
			{
				HIShapeUnion(mutableResult, rectShape, mutableResult);
				CFRelease(rectShape), rectShape = nullptr;
			}
		}
		
		if ((selectionPastEnd.second - selectionStart.second) > 2)
		{
			// highlight extends across more than two lines - fill in the space in between
			partialSelectionBounds = CGRectMake(inInsets,
												(selectionStart.second + 1) * inTerminalViewPtr->text.font.heightPerCharacter + inInsets,
												inTerminalViewPtr->screen.cache.viewWidthInPixels - 2.0f * inInsets,
												(selectionPastEnd.second - selectionStart.second - 2/* skip first and last lines */) *
													inTerminalViewPtr->text.font.heightPerCharacter - 2.0f * inInsets);
			clippedRect = CGRectIntegral(CGRectIntersection(partialSelectionBounds, screenBounds)); // clip to constraint rectangle
			rectShape = HIShapeCreateWithRect(&clippedRect);
			if (nullptr != rectShape)
			{
				HIShapeUnion(mutableResult, rectShape, mutableResult);
				CFRelease(rectShape), rectShape = nullptr;
			}
		}
		
		result = mutableResult;
	}
	
	return result;
}// getVirtualRangeAsNewHIShape


/*!
Returns a new region locating the specified area of the
terminal view, LOCAL to its window port.  You must dispose of
the region yourself.

DEPRECATED.  Use getVirtualRangeAsNewRegionOnScreen() instead.

(3.1)
*/
RgnHandle
getVirtualRangeAsNewRegion		(My_TerminalViewPtr			inTerminalViewPtr,
								 TerminalView_Cell const&	inSelectionStart,
								 TerminalView_Cell const&	inSelectionPastEnd,
								 Boolean					inIsRectangular)
{
	RgnHandle	result = getVirtualRangeAsNewRegionOnScreen(inTerminalViewPtr, inSelectionStart,
															inSelectionPastEnd, inIsRectangular);
	
	
	if (nullptr != result)
	{
		// now convert the region to be in QuickDraw local coordinates,
		// which are the same as those of the window content view
		HIPoint		offsetAmount = CGPointMake(0, 0);
		OSStatus	error = noErr;
		
		
		error = HIViewConvertPoint(&offsetAmount, inTerminalViewPtr->contentHIView,
									HIViewWrap(kHIViewWindowContentID, HIViewGetWindow(inTerminalViewPtr->contentHIView)));
		assert_noerr(error);
		OffsetRgn(result, STATIC_CAST(offsetAmount.x, SInt16), STATIC_CAST(offsetAmount.y, SInt16));
	}
	return result;
}// getVirtualRangeAsNewRegion


/*!
Returns a new region locating the specified area of the
terminal view, relative to itself: for example, the first
character in the top-left corner has origin (0, 0) in
pixels.  You must dispose of the region yourself.

(3.1)
*/
RgnHandle
getVirtualRangeAsNewRegionOnScreen	(My_TerminalViewPtr			inTerminalViewPtr,
									 TerminalView_Cell const&	inSelectionStart,
									 TerminalView_Cell const&	inSelectionPastEnd,
									 Boolean					inIsRectangular)
{
	RgnHandle	result = NewRgn();
	
	
	if (nullptr != result)
	{
		HIRect				floatBounds;
		Rect				screenBounds;
		TerminalView_Cell	selectionStart;
		TerminalView_Cell	selectionPastEnd;
		OSStatus			error = noErr;
		
		
		// find clipping region
		error = HIViewGetBounds(inTerminalViewPtr->contentHIView, &floatBounds);
		assert_noerr(error);
		RegionUtilities_SetRect(&screenBounds, 0, 0, STATIC_CAST(floatBounds.size.width, SInt16), STATIC_CAST(floatBounds.size.height, SInt16));
		
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
			// then the area to be highlighted is a rectangle; this simplifies things...
			Rect	clippedRect;
			Rect	selectionBounds;
			
			
			// make the points the “right way around”, in case the first point is
			// technically to the right of or below the second point
			sortAnchors(selectionStart, selectionPastEnd, true/* is a rectangular selection */);
			
			// set up rectangle bounding area to be highlighted
			RegionUtilities_SetRect(&selectionBounds,
									selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter,
									STATIC_CAST(selectionStart.second, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter,
									selectionPastEnd.first * inTerminalViewPtr->text.font.widthPerCharacter,
									STATIC_CAST(selectionPastEnd.second, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter);
			
			// the final selection region is the portion of the full rectangle
			// that fits within the current screen boundaries
			SectRect(&selectionBounds, &screenBounds, &clippedRect);
			RectRgn(result, &clippedRect);
		}
		else
		{
			// then the area to be highlighted is irregularly shaped; this is more complex...
			RgnHandle	clippedRegion = NewRgn();
			
			
			if (nullptr != clippedRegion)
			{
				Rect	clippedRect;
				Rect	partialSelectionBounds;
				
				
				// make the points the “right way around”, in case the first point is
				// technically to the right of or below the second point
				sortAnchors(selectionStart, selectionPastEnd, false/* is a rectangular selection */);
				
				// bounds of first (possibly partial) line to be highlighted
				RegionUtilities_SetRect(&partialSelectionBounds,
										selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter,
										STATIC_CAST(selectionStart.second, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter,
										inTerminalViewPtr->screen.cache.viewWidthInPixels,
										STATIC_CAST(selectionStart.second + 1, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter);
				SectRect(&partialSelectionBounds, &screenBounds, &clippedRect); // clip to constraint rectangle
				RectRgn(result, &clippedRect);
				
				// bounds of last (possibly partial) line to be highlighted
				RegionUtilities_SetRect(&partialSelectionBounds,
										0,
										STATIC_CAST(selectionPastEnd.second - 1, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter,
										selectionPastEnd.first * inTerminalViewPtr->text.font.widthPerCharacter,
										STATIC_CAST(selectionPastEnd.second, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter);
				SectRect(&partialSelectionBounds, &screenBounds, &clippedRect); // clip to constraint rectangle
				RectRgn(clippedRegion, &clippedRect);
				UnionRgn(clippedRegion, result, result);
				
				if ((selectionPastEnd.second - selectionStart.second) > 2)
				{
					// highlight extends across more than two lines - fill in the space in between
					RegionUtilities_SetRect(&partialSelectionBounds,
											0,
											STATIC_CAST(selectionStart.second + 1, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter,
											inTerminalViewPtr->screen.cache.viewWidthInPixels,
											STATIC_CAST(selectionPastEnd.second - 1, SInt16) * inTerminalViewPtr->text.font.heightPerCharacter);
					SectRect(&partialSelectionBounds, &screenBounds, &clippedRect); // clip to constraint rectangle
					RectRgn(clippedRegion, &clippedRect);
					UnionRgn(clippedRegion, result, result);
				}
				
				DisposeRgn(clippedRegion), clippedRegion = nullptr;
			}
		}
	}
	return result;
}// getVirtualRangeAsNewRegionOnScreen


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
			UniChar const*				textStart = nullptr;
			UniChar const*				textPastEnd = nullptr;
			Terminal_TextCopyFlags		flags = 0L;
			
			
			// configure terminal routine
			if (inTerminalViewPtr->text.selection.isRectangular) flags |= kTerminal_TextCopyFlagsRectangular;
			
			// double-click - select a word; or, do intelligent double-click
			// based on the character underneath the cursor
			// TEMPORARY; only one line is examined, but this is probably
			// more useful if it joins at least the preceding line, and
			// possible the following line as well
			if (kTerminal_ResultOK ==
				Terminal_GetLine(inTerminalViewPtr->screen.ref, lineIterator, textStart, textPastEnd, flags))
			{
				// NOTE: while currently a short-cut is taken to avoid copying the string,
				// this may have to become a true copy if this is expanded to look at text
				// across more than one line
				CFRetainRelease			asCFString(CFStringCreateWithCharactersNoCopy
													(kCFAllocatorDefault, textStart, textPastEnd - textStart, kCFAllocatorNull/* deallocator */),
													CFRetainRelease::kAlreadyRetained);
				std::string				asUTF8;
				std::pair< long, long >	wordInfo; // offset (zero-based), and count
				
				
				StringUtilities_CFToUTF8(asCFString.returnCFStringRef(), asUTF8);
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
		inTerminalViewPtr->text.selection.exists = (inTerminalViewPtr->text.selection.range.first !=
													inTerminalViewPtr->text.selection.range.second);
		copySelectedTextIfUserPreference(inTerminalViewPtr);
	}
}// handleMultiClick


/*!
Responds to terminal view size or position changes by
updating sub-views.

(3.0)
*/
void
handleNewViewContainerBounds	(HIViewRef		inHIView,
								 Float32		UNUSED_ARGUMENT(inDeltaX),
								 Float32		UNUSED_ARGUMENT(inDeltaY),
								 void*			inTerminalViewRef)
{
	TerminalViewRef				view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
	HIRect						terminalViewBounds;
	
	
	// get view’s boundaries, synchronize background picture with that size
	HIViewGetFrame(inHIView, &terminalViewBounds);
	HIViewSetFrame(viewPtr->backgroundHIView, &terminalViewBounds);
	
	// from here on, use this only for the bounds, not the origin
	HIViewGetBounds(inHIView, &terminalViewBounds);
	
	// determine the view size within its parent
	{
		Float32 const	kMaximumViewWidth = terminalViewBounds.size.width;
		Float32 const	kMaximumViewHeight = terminalViewBounds.size.height; 
		
		
		// if the display mode is zooming, choose a font size to fill the new boundaries
		if (viewPtr->displayMode == kTerminalView_DisplayModeZoom)
		{
			UInt16				visibleColumns = Terminal_ReturnColumnCount(viewPtr->screen.ref);
			UInt16				visibleRows = Terminal_ReturnRowCount(viewPtr->screen.ref);
			CGrafPtr			oldPort = nullptr;
			GDHandle			oldDevice = nullptr;
			GrafPortFontState   fontState;
			SInt16				fontSize = 4; // set an initial font size that is then scaled up appropriately
			
			
			GetGWorld(&oldPort, &oldDevice);
			SetPortWindowPort(HIViewGetWindow(viewPtr->contentHIView));
			Localization_PreservePortFontState(&fontState);
			
			TextFontByName(viewPtr->text.font.familyName);
			TextSize(fontSize);
			
			// choose an appropriate font size to best fill the area; favor maximum
			// width over height, but reduce the font size if the characters overrun
			if (viewPtr->screen.cache.viewWidthInPixels > 0)
			{
				UInt16		loopGuard = 0;
				
				
				while ((STATIC_CAST(CharWidth('A'), Float32) * viewPtr->text.font.scaleWidthPerCharacter *
						STATIC_CAST(visibleColumns, Float32)) < kMaximumViewWidth)
				{
					TextSize(++fontSize);
					if (++loopGuard > 100/* arbitrary */) break;
				}
				
				// since the final size will have gone over the edge, back up one font size
				TextSize(--fontSize);
			}
			else
			{
				TextSize(viewPtr->text.font.normalMetrics.size);
			}
			if (viewPtr->screen.cache.viewHeightInPixels > 0)
			{
				FontInfo	info;
				UInt16		loopGuard = 0;
				
				
				GetFontInfo(&info);
				while (((info.ascent + info.descent + info.leading) * visibleRows) >= kMaximumViewHeight)
				{
					TextSize(--fontSize);
					GetFontInfo(&info);
					if (++loopGuard > 100/* arbitrary */) break;
				}
			}
			
			// updating the font size should also recalculate cached dimensions
			// which are used later to center the view rectangle; but do not
			// notify listeners, since this routine is itself a response to a
			// change in another property (view size)
			setFontAndSize(viewPtr, nullptr/* font */, fontSize, 0/* scale for character width, or zero */, false/* notify listeners */);
			
			Localization_RestorePortFontState(&fontState);
			SetGWorld(oldPort, oldDevice);
		}
		else if (false == viewPtr->screen.sizeNotMatchedWithView)
		{
			UInt16					columns = 0;
			TerminalView_RowIndex	rows = 0;
			
			
			// normal mode; resize the underlying terminal screen
			TerminalView_GetTheoreticalScreenDimensions(view, STATIC_CAST(kMaximumViewWidth, UInt16),
														STATIC_CAST(kMaximumViewHeight, UInt16),
														&columns, &rows);
			Terminal_SetVisibleScreenDimensions(viewPtr->screen.ref, columns, STATIC_CAST(rows, UInt16));
			recalculateCachedDimensions(viewPtr);
		}
	}
	
	// keep the text area centered inside the background region;
	// keep the focus/padding background the right padding distance
	// away from the text
	{
		Float32 const	kPadLeft = (viewPtr->screen.paddingLeftEmScale *
									STATIC_CAST(viewPtr->text.font.widthPerCharacter, Float32));
		Float32 const	kPadRight = (viewPtr->screen.paddingRightEmScale *
										STATIC_CAST(viewPtr->text.font.widthPerCharacter, Float32));
		Float32 const	kPadTop = (viewPtr->screen.paddingTopEmScale *
									STATIC_CAST(viewPtr->text.font.widthPerCharacter/* yes, width, this is an “em” scale */, Float32));
		Float32 const	kPadBottom = (viewPtr->screen.paddingBottomEmScale *
										STATIC_CAST(viewPtr->text.font.widthPerCharacter/* yes, width, this is an “em” scale */, Float32));
		HIRect			terminalFocusFrame = CGRectMake(0, 0, kPadLeft + viewPtr->screen.cache.viewWidthInPixels + kPadRight,
														kPadTop + viewPtr->screen.cache.viewHeightInPixels + kPadBottom);
		HIRect			terminalInteriorFrame = CGRectMake(kPadLeft, kPadTop, viewPtr->screen.cache.viewWidthInPixels,
															viewPtr->screen.cache.viewHeightInPixels);
		
		
		// TEMPORARY: this should in fact respect margin values too
		RegionUtilities_CenterHIRectIn(terminalFocusFrame, terminalViewBounds);
		HIViewSetFrame(viewPtr->paddingHIView, &terminalFocusFrame);
		HIViewSetFrame(viewPtr->contentHIView, &terminalInteriorFrame);
	}
	
	// recalculate cursor boundaries for the specified view
	// (should the cursor boundaries be stored screen-relative
	// so that this kind of maintenance can be avoided?)
	{
		Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		getCursorLocationError = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
		if (kTerminal_ResultOK != getCursorLocationError)
		{
			Console_Warning(Console_WriteValue, "failed to update cursor after view resize; internal error", getCursorLocationError);
		}
		else
		{
			setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds, viewPtr->screen.cursor.boundsAsRegion);
		}
	}
}// handleNewViewContainerBounds


/*!
On Mac OS X, displays all unrendered changes to visible graphics
ports.  Useful in unusual circumstances, namely any time drawing
must occur so soon that an event loop iteration is not guaranteed
to run first.  (An event loop iteration will automatically handle
pending updates.)

(3.1)
*/
void
handlePendingUpdates ()
{
	EventRecord		updateEvent;
	
	
	// simply *checking* for events triggers approprate flushing to the
	// display; so would WaitNextEvent(), but this is nice because it
	// does not pull any events from the queue (after all, this routine
	// couldn’t handle the events if they were pulled)
	UNUSED_RETURN(Boolean)EventAvail(updateMask, &updateEvent);
}// handlePendingUpdates


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
		#if 1
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
  	Rect	textBounds;
	
	
	// mark the specified area; it is already in “screen coordinates”,
	// which match the view coordinates used by updateDisplayInRegion()
	getRowSectionBounds(inTerminalViewPtr, inLineNumber, inStartingColumnNumber, inCharacterCount, &textBounds);
	
	RectRgn(gInvalidationScratchRegion(), &textBounds);
	updateDisplayInRegion(inTerminalViewPtr, gInvalidationScratchRegion());
}// invalidateRowSection


/*!
Determines if a font is monospaced.

(3.0)
*/
Boolean
isMonospacedFont	(FMFontFamily	inFontID)
{
	Boolean		result = false;
	Boolean		doRomanTest = false;
	SInt32		numberOfScriptsEnabled = GetScriptManagerVariable(smEnabled);	// this returns the number of scripts enabled
	
	
	if (numberOfScriptsEnabled > 1)
	{
		ScriptCode	scriptNumber = smRoman;
		
		
		scriptNumber = FontToScript(inFontID);
		if (scriptNumber != smRoman)
		{
			SInt32		thisScriptEnabled = GetScriptVariable(scriptNumber, smScriptEnabled);
			
			
			if (thisScriptEnabled)
			{
				// check if this font is the preferred monospaced font for its script
				SInt32		theSizeAndFontFamily = 0L;
				SInt16		thePreferredFontFamily = 0;
				
				
				theSizeAndFontFamily = GetScriptVariable(scriptNumber, smScriptMonoFondSize);
				thePreferredFontFamily = theSizeAndFontFamily >> 16; // high word is font family 
				result = (thePreferredFontFamily == inFontID);
			}
			else result = false; // this font’s script isn’t enabled
		}
		else doRomanTest = true;
	}
	else doRomanTest = true;
	
	if (doRomanTest)
	{
		GDHandle	currentDevice = nullptr;
		CGrafPtr	currentPort = nullptr;
		SInt16		oldFont = inFontID;
		
		
		GetGWorld(&currentPort, &currentDevice);
		oldFont = GetPortTextFont(currentPort);
		TextFont(inFontID);
		result = (CharWidth('W') == CharWidth('.'));
		TextFont(oldFont);
	}
	
	return result;
}// isMonospacedFont


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
Translates pixels in the coordinate system of a
window so that they are relative to the origin of
the screen.  In version 2.6, the program always
assumed that the screen origin was the top-left
corner of the window, which made it extremely
hard to change.  Now, this routine is invoked
everywhere to ensure that, before port pixel
offsets are used to refer to the screen, they are
translated correctly.

If you are not interested in translating a point,
but rather in shifting a dimension horizontally or
vertically, you can pass nullptr for the dimension
that you do not use.

See also screenToLocal().

(3.0)
*/
void
localToScreen	(My_TerminalViewPtr		inTerminalViewPtr,
				 SInt16*				inoutHorizontalPixelOffsetFromPortOrigin,
				 SInt16*				inoutVerticalPixelOffsetFromPortOrigin)
{
	Point	origin;
	
	
	getScreenOrigin(inTerminalViewPtr, &origin.h, &origin.v);
	if (inoutHorizontalPixelOffsetFromPortOrigin != nullptr)
	{
		(*inoutHorizontalPixelOffsetFromPortOrigin) -= origin.h;
	}
	if (inoutVerticalPixelOffsetFromPortOrigin != nullptr)
	{
		(*inoutVerticalPixelOffsetFromPortOrigin) -= origin.v;
	}
}// localToScreen


/*!
Invoked whenever a monitored event from the main event loop
occurs (see TerminalView_Init() to see which events are
monitored).  This routine responds appropriately to each
event (e.g. updating internal variables).

The result is "true" only if the event is to be absorbed
(preventing anything else from seeing it).

(3.0)
*/
Boolean
mainEventLoopEvent	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inEventThatOccurred,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Boolean		result = false;
	
	
	switch (inEventThatOccurred)
	{
	case kEventLoop_GlobalEventSuspendResume:
		{
			Boolean		fadeWindows = false;
			Float32		alpha = 1.0;
			
			
			// update the internal variable to reflect the current suspended state of the application
			gApplicationIsSuspended = FlagManager_Test(kFlagSuspended);
			
			unless (kPreferences_ResultOK ==
					Preferences_GetData(kPreferences_TagFadeBackgroundWindows,
					sizeof(fadeWindows), &fadeWindows))
			{
				fadeWindows = false; // assume a value, if preference can’t be found
			}
			
			// modify windows
			if (fadeWindows)
			{
				Float32		fadeAlpha = 1.0;
				
				
				// update the internal variable to reflect the current suspended state of the application
				gApplicationIsSuspended = FlagManager_Test(kFlagSuspended);
				
				unless (kPreferences_ResultOK ==
						Preferences_GetData(kPreferences_TagFadeAlpha,
						sizeof(fadeAlpha), &fadeAlpha))
				{
					fadeAlpha = 1.0; // assume a value, if preference can’t be found
				}
				
				if (gApplicationIsSuspended)
				{
					alpha = fadeAlpha;
					SessionFactory_ForEveryTerminalWindowDo(setAlphaTerminalWindowOp, &alpha/* data 1 */, 0/* data 2 */, nullptr/* result */);
				}
				else
				{
					SessionFactory_ForEveryTerminalWindowDo(setAlphaTerminalWindowOp, &alpha/* data 1 */, 0/* data 2 */, nullptr/* result */);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// mainEventLoopEvent


/*!
Invoked by the Mac OS whenever something interesting happens
in a Navigation Services file-capture-save-dialog attached to
a terminal view’s window.

(3.1)
*/
void
navigationFileCaptureDialogEvent	(NavEventCallbackMessage	inMessage,
								 	 NavCBRecPtr				inParameters,
								 	 void*						inTerminalViewRef)
{
	TerminalViewRef		view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	
	
	switch (inMessage)
	{
	case kNavCBUserAction:
		if (kNavUserActionSaveAs == inParameters->userAction)
		{
			NavReplyRecord		reply;
			OSStatus			error = noErr;
			
			
			// save file
			error = NavDialogGetReply(inParameters->context/* dialog */, &reply);
			if ((noErr == error) && (reply.validRecord))
			{
				FSRef	saveFile;
				FSRef	temporaryFile;
				OSType	captureFileCreator = 'ttxt';
				
				
				// create a temporary file for “safe” saving, in addition to the user’s requested file
				error = FileSelectionDialogs_CreateOrFindUserSaveFile
						(reply, captureFileCreator, 'TEXT', saveFile, temporaryFile);
				if (error == noErr)
				{
					Handle		textHandle = nullptr;
					
					
					textHandle = TerminalView_ReturnSelectedTextAsNewHandle(view, 0/* spaces equal to one tab, or zero for no substitution */,
																			0/* flags */);
					if (nullptr != textHandle)
					{
						SInt16		fileRefNum = -1;
						
						
						// open file for overwrite
						error = FSOpenFork(&temporaryFile, 0/* name length */, nullptr/* name (implied from structure) */,
											fsWrPerm, &fileRefNum);
						if (noErr == error)
						{
							SInt32		total = GetHandleSize(textHandle);
							ByteCount	byteCountToWrite = 0;
							ByteCount	byteCountLeft = 0;
							
							
							// overwrite existing files
							error = FSSetForkSize(fileRefNum, fsFromStart, 0);
							assert_noerr(error);
							
							// write text to the file; as long as bytes remain and no error
							// has occurred, data remains to be written and should be written
							byteCountToWrite = total;
							while ((total > 0) && (noErr == error))
							{
								error = FSWriteFork(fileRefNum, fsFromMark, 0/* offset */,
													byteCountToWrite, *textHandle, &byteCountLeft);
								total -= byteCountLeft;
								byteCountToWrite = total; // prepare for next loop
							}
							
							UNUSED_RETURN(OSStatus)FSCloseFork(fileRefNum), fileRefNum = -1;
							
							// finally, “swap” the new file into the right place on disk
							error = FSExchangeObjects(&temporaryFile, &saveFile);
							if (noErr == error)
							{
								UNUSED_RETURN(OSStatus)FSDeleteObject(&temporaryFile);
							}
						}
					}
				}
			}
			Alert_ReportOSStatus(error);
			error = FileSelectionDialogs_CompleteSave(&reply);
			if (noErr != error)
			{
				Console_Warning(Console_WriteValue, "failed to complete save for file capture, error", error);
			}
		}
		break;
	
	case kNavCBTerminate:
		// clean up
		NavDialogDispose(inParameters->context);
		break;
	
	default:
		// not handled
		break;
	}
}// navigationFileCaptureDialogEvent


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
	
	
	newDiscreteValue = STATIC_CAST(INTEGER_MAXIMUM(kMinimum, newDiscreteValue), SInt16);
	newDiscreteValue = STATIC_CAST(INTEGER_MINIMUM(kMaximum, newDiscreteValue), SInt16);
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
						 SInt32					inDeltaInRows)
{
	SInt32 const	kMinimum = -Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref);
	SInt32 const	kMaximum = 0;
	SInt32 const	kOldDiscreteValue = inTerminalViewPtr->screen.topVisibleEdgeInRows;
	SInt32			newDiscreteValue = kOldDiscreteValue + inDeltaInRows;
	
	
	newDiscreteValue = INTEGER_MAXIMUM(kMinimum, newDiscreteValue);
	newDiscreteValue = INTEGER_MINIMUM(kMaximum, newDiscreteValue);
	inTerminalViewPtr->screen.topVisibleEdgeInRows = newDiscreteValue;
	
#if 0
	inTerminalViewPtr->text.selection.range.first.second += (newDiscreteValue - kOldDiscreteValue);
	inTerminalViewPtr->text.selection.range.second.second += (newDiscreteValue - kOldDiscreteValue);
#endif
	
	// hide the cursor while the main screen is not showing
	setCursorVisibility(inTerminalViewPtr, (inTerminalViewPtr->screen.topVisibleEdgeInRows >= 0));
}// offsetTopVisibleEdge


/*!
Adds menu items to the specified menu based on the
current state of the given terminal view.

To keep menus short, this routine builds two types of
menus: one for text selections, and one if nothing is
selected.  If text is selected, this routine assumes
that the user will most likely not want any of the
more-general items.  Otherwise, the general items are
made available whenever they are applicable.

(4.1)
*/
void
populateContextualMenu	(My_TerminalViewPtr		inTerminalViewPtr,
						 NSMenu*				inoutMenu)
{
	NSMenuItem*		newItem = nil;
	CFStringRef		commandText = nullptr;
	UInt32			targetCommandID = 0;
	
	
	ContextSensitiveMenu_Init();
	
	if (TerminalView_TextSelectionExists(inTerminalViewPtr->selfRef))
	{
		// URL commands
		ContextSensitiveMenu_NewItemGroup();
		
		targetCommandID = kCommandHandleURL;
		if (UIStrings_Copy(kUIStrings_ContextualMenuOpenThisResource, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// clipboard-related text selection commands
		ContextSensitiveMenu_NewItemGroup();
		
		targetCommandID = kCommandCopy;
		if (UIStrings_Copy(kUIStrings_ContextualMenuCopyToClipboard, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandCopyTable;
		if (UIStrings_Copy(kUIStrings_ContextualMenuCopyUsingTabsForSpaces, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandSaveText;
		if (UIStrings_Copy(kUIStrings_ContextualMenuSaveSelectedText, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandShowCompletions;
		if (UIStrings_Copy(kUIStrings_ContextualMenuShowCompletions, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// user macros
		MacroManager_AddContextualMenuGroup(inoutMenu); // implicitly calls ContextSensitiveMenu_NewItemGroup() again
		
		// other text-selection-related commands
		ContextSensitiveMenu_NewItemGroup();
		
		targetCommandID = kCommandPrint;
		if (UIStrings_Copy(kUIStrings_ContextualMenuPrintSelectedText, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandSpeakSelectedText;
		if (UIStrings_Copy(kUIStrings_ContextualMenuSpeakSelectedText, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandStopSpeaking;
		if (UIStrings_Copy(kUIStrings_ContextualMenuStopSpeaking, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
	}
	else
	{
		// text-editing-related menu items
		ContextSensitiveMenu_NewItemGroup();
		
		targetCommandID = kCommandPaste;
		if (UIStrings_Copy(kUIStrings_ContextualMenuPasteText, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandFind;
		if (UIStrings_Copy(kUIStrings_ContextualMenuFindInThisWindow, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// window arrangement menu items
		ContextSensitiveMenu_NewItemGroup();
		
		{
			HIWindowRef			screenWindow = TerminalView_ReturnWindow(inTerminalViewPtr->selfRef);
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(screenWindow);
			
			
			if (TerminalWindow_IsFullScreen(terminalWindow))
			{
				targetCommandID = kCommandFullScreenToggle;
				if (UIStrings_Copy(kUIStrings_ContextualMenuFullScreenExit, commandText).ok())
				{
					newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
					if (nil != newItem)
					{
						ContextSensitiveMenu_AddItem(inoutMenu, newItem);
						[newItem release], newItem = nil;
					}
					CFRelease(commandText), commandText = nullptr;
				}
			}
		}
		
		targetCommandID = kCommandHideFrontWindow;
		if (UIStrings_Copy(kUIStrings_ContextualMenuHideThisWindow, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandStackWindows;
		if (UIStrings_Copy(kUIStrings_ContextualMenuArrangeAllInFront, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandChangeWindowTitle;
		if (UIStrings_Copy(kUIStrings_ContextualMenuRenameThisWindow, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// terminal-related menu items
		ContextSensitiveMenu_NewItemGroup();
		
		targetCommandID = kCommandPrintScreen;
		if (UIStrings_Copy(kUIStrings_ContextualMenuPrintScreen, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandSetScreenSize;
		if (UIStrings_Copy(kUIStrings_ContextualMenuCustomScreenDimensions, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandFormat;
		if (UIStrings_Copy(kUIStrings_ContextualMenuCustomFormat, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		targetCommandID = kCommandSetKeys;
		if (UIStrings_Copy(kUIStrings_ContextualMenuSpecialKeySequences, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
			}
			CFRelease(commandText), commandText = nullptr;
		}
		
		// speech-related items
		ContextSensitiveMenu_NewItemGroup();
		
		// NOTE: this item is available whether or not there is a text selection,
		// as long as speech is in progress
		targetCommandID = kCommandStopSpeaking;
		if (UIStrings_Copy(kUIStrings_ContextualMenuStopSpeaking, commandText).ok())
		{
			newItem = Commands_NewMenuItemForCommand(targetCommandID, commandText, true/* must be enabled */);
			if (nil != newItem)
			{
				ContextSensitiveMenu_AddItem(inoutMenu, newItem);
				[newItem release], newItem = nil;
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
			gPreferenceProxies.cursorType = kTerminalView_CursorTypeBlock; // assume a value, if preference can’t be found
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
			if (IsValidControlHandle(viewPtr->contentHIView))
			{
				Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
				UInt16				cursorX = 0;
				UInt16				cursorY = 0;
				
				
				// invalidate the entire old cursor region (in case it is bigger than the new one)
				updateDisplayInRegion(viewPtr, viewPtr->screen.cursor.boundsAsRegion);
				
				// find the new cursor region
				getCursorLocationError = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
				if (kTerminal_ResultOK != getCursorLocationError)
				{
					Console_Warning(Console_WriteValue, "failed to update cursor after preference change; internal error", getCursorLocationError);
				}
				else
				{
					setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds, viewPtr->screen.cursor.boundsAsRegion);
					
					// invalidate the new cursor region (in case it is bigger than the old one)
					updateDisplayInRegion(viewPtr, viewPtr->screen.cursor.boundsAsRegion);
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
	UInt16 const	kVisibleWidth = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	UInt16 const	kVisibleLines = Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
	
	
	inTerminalViewPtr->screen.cache.viewWidthInPixels = kVisibleWidth * inTerminalViewPtr->text.font.widthPerCharacter;
	inTerminalViewPtr->screen.cache.viewHeightInPixels = STATIC_CAST(kVisibleLines, SInt32) *
															STATIC_CAST(inTerminalViewPtr->text.font.heightPerCharacter, SInt32);
}// recalculateCachedDimensions


/*!
Handles standard events for the HIObject of a terminal
view’s text area.

IMPORTANT:	You cannot simply add cases here to handle new
			events...see TerminalView_Init() for the registry
			of events that will invoke this routine.

Invoked by Mac OS X.

(3.1)
*/
OSStatus
receiveTerminalHIObjectEvents	(EventHandlerCallRef	inHandlerCallRef,
								 EventRef				inEvent,
								 void*					inTerminalViewRef)
{
	OSStatus			result = eventNotHandledErr;
	// IMPORTANT: This data is NOT valid during the kEventClassHIObject/kEventHIObjectConstruct
	// event: that is, in fact, when its value is defined.
	TerminalViewRef		view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	if (kEventClass == kEventClassHIObject)
	{
		switch (kEventKind)
		{
		case kEventHIObjectConstruct:
			///
			///!!! REMEMBER, THIS IS CALLED DIRECTLY BY THE TOOLBOX - NO CallNextEventHandler() ALLOWED!!!
			///
			//Console_WriteLine("HI OBJECT construct terminal view");
			{
				HIObjectRef		superclassHIObject = nullptr;
				
				
				// get the superclass view that has already been constructed (but not initialized)
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamHIObjectInstance,
																typeHIObjectRef, superclassHIObject);
				if (noErr == result)
				{
					// allocate a view without initializing it
					HIViewRef		superclassHIObjectAsHIView = REINTERPRET_CAST(superclassHIObject, HIViewRef);
					
					
					try
					{
						My_TerminalViewPtr	viewPtr = new My_TerminalView(superclassHIObjectAsHIView);
						TerminalViewRef		ref = nullptr;
						
						
						assert(nullptr != viewPtr);
						ref = REINTERPRET_CAST(viewPtr, TerminalViewRef);
						
						// IMPORTANT: The setting of this parameter also ensures the context parameter
						// "inTerminalViewRef" is equal to this value when all other events enter
						// this function.
						result = SetEventParameter(inEvent, kEventParamHIObjectInstance,
													typeVoidPtr, sizeof(ref), &ref);
						if (noErr == result)
						{
							// IMPORTANT: Set a property with the TerminalViewRef, so that code
							// invoking HIObjectCreate() has a reasonable way to get this value.
							result = SetControlProperty(superclassHIObjectAsHIView,
														AppResources_ReturnCreatorCode(),
														kConstantsRegistry_ControlPropertyTypeTerminalViewRef,
														sizeof(ref), &ref);
						}
					}
					catch (std::exception)
					{
						result = memFullErr;
					}
				}
			}
			break;
		
		case kEventHIObjectInitialize:
			//Console_WriteLine("HI OBJECT initialize terminal view");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
				TerminalScreenRef			initialDataSource = nullptr;
				
				
				// get the terminal data source
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_TerminalDataSource,
																typeNetEvents_TerminalScreenRef, initialDataSource);
				if (noErr == result)
				{
					Preferences_ContextRef		viewFormat = nullptr;
					
					
					// get the terminal format; if not found, use the default
					result = CarbonEventUtilities_GetEventParameter
								(inEvent, kEventParamNetEvents_TerminalFormatPreferences,
									typeNetEvents_PreferencesContextRef, viewFormat);
					if ((noErr != result) || (nullptr == viewFormat))
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
							viewFormat = Preferences_NewCloneContext(defaultContext, true/* force detach */);
						}
						result = noErr; // ignore
					}
					
					// finally, initialize the view properly
					try
					{
						assert(nullptr != initialDataSource);
						viewPtr->initialize(initialDataSource, viewFormat);
						result = noErr;
					}
					catch (std::exception)
					{
						result = eventNotHandledErr;
					}
				}
			}
			break;
		
		case kEventHIObjectDestruct:
			///
			///!!! REMEMBER, THIS IS CALLED DIRECTLY BY THE TOOLBOX - NO CallNextEventHandler() ALLOWED!!!
			///
			//Console_WriteLine("HI OBJECT destruct terminal view");
			if (gTerminalViewPtrLocks().isLocked(view))
			{
				Console_Warning(Console_WriteValue, "attempt to dispose of locked terminal view; outstanding locks",
								gTerminalViewPtrLocks().returnLockCount(view));
			}
			else
			{
				//Console_WriteValueAddress("request to destroy HIObject implementation", view);
				delete REINTERPRET_CAST(view, My_TerminalViewPtr);
				result = noErr;
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	else if (kEventClass == kEventClassAccessibility)
	{
		assert(kEventClass == kEventClassAccessibility);
		switch (kEventKind)
		{
		case kEventAccessibleGetAllAttributeNames:
			{
				CFMutableArrayRef	listOfNames = nullptr;
				
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamAccessibleAttributeNames,
																typeCFMutableArrayRef, listOfNames);
				if (noErr == result)
				{
					// each attribute mentioned here should be handled below
					CFArrayAppendValue(listOfNames, kAXDescriptionAttribute);
					CFArrayAppendValue(listOfNames, kAXRoleAttribute);
					CFArrayAppendValue(listOfNames, kAXRoleDescriptionAttribute);
					CFArrayAppendValue(listOfNames, kAXNumberOfCharactersAttribute);
					CFArrayAppendValue(listOfNames, kAXTopLevelUIElementAttribute);
					CFArrayAppendValue(listOfNames, kAXWindowAttribute);
					CFArrayAppendValue(listOfNames, kAXParentAttribute);
					CFArrayAppendValue(listOfNames, kAXEnabledAttribute);
					CFArrayAppendValue(listOfNames, kAXPositionAttribute);
					CFArrayAppendValue(listOfNames, kAXSizeAttribute);
				}
			}
			break;
		
		case kEventAccessibleGetNamedAttribute:
		case kEventAccessibleIsNamedAttributeSettable:
			{
				CFStringRef		requestedAttribute = nullptr;
				
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamAccessibleAttributeName,
																typeCFStringRef, requestedAttribute);
				if (noErr == result)
				{
					// for the purposes of accessibility, identify a Terminal View as having
					// the same role as a standard text area
					CFStringRef		roleCFString = kAXTextAreaRole;
					Boolean			isSettable = false;
					
					
					// IMPORTANT: The cases handled here should match the list returned
					// by "kEventAccessibleGetAllAttributeNames", above.
					if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXRoleAttribute, kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFStringRef,
														sizeof(roleCFString), &roleCFString);
						}
					}
					else if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXRoleDescriptionAttribute,
																	kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							CFStringRef		roleDescCFString = HICopyAccessibilityRoleDescription
																(roleCFString, nullptr/* sub-role */);
							
							
							if (nullptr != roleDescCFString)
							{
								result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFStringRef,
															sizeof(roleDescCFString), &roleDescCFString);
								CFRelease(roleDescCFString), roleDescCFString = nullptr;
							}
						}
					}
					else if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXNumberOfCharactersAttribute,
																	kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
							SInt64						numChars = returnNumberOfCharacters(viewPtr);
							CFNumberRef					numCharsCFNumber = CFNumberCreate(kCFAllocatorDefault,
																							kCFNumberSInt64Type, &numChars);
							
							
							if (nullptr != numCharsCFNumber)
							{
								result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFTypeRef,
															sizeof(numCharsCFNumber), &numCharsCFNumber);
								CFRelease(numCharsCFNumber), numCharsCFNumber = nullptr;
							}
						}
					}
					else if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXDescriptionAttribute, kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							UIStrings_Result	stringResult = kUIStrings_ResultOK;
							CFStringRef			descriptionCFString = nullptr;
							
							
							stringResult = UIStrings_Copy(kUIStrings_TerminalAccessibilityDescription,
															descriptionCFString);
							if (false == stringResult.ok())
							{
								result = resNotFound;
							}
							else
							{
								result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFStringRef,
															sizeof(descriptionCFString), &descriptionCFString);
							}
						}
					}
					else
					{
						// Many attributes are already supported by the default handler:
						//	kAXTopLevelUIElementAttribute
						//	kAXWindowAttribute
						//	kAXParentAttribute
						//	kAXEnabledAttribute
						//	kAXSizeAttribute
						//	kAXPositionAttribute
						result = eventNotHandledErr;
					}
					
					// return the read-only flag when requested, if the attribute was used above
					if ((noErr == result) &&
						(kEventAccessibleIsNamedAttributeSettable == kEventKind))
					{
						result = SetEventParameter(inEvent, kEventParamAccessibleAttributeSettable, typeBoolean,
													sizeof(isSettable), &isSettable);
					}
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	else
	{
		assert(kEventClass == kEventClassControl);
		switch (kEventKind)
		{
		case kEventControlInitialize:
			//Console_WriteLine("HI OBJECT initialize control for terminal view");
			{
				UInt32		controlFeatures = //kControlSupportsFocus |
												//kControlHandlesTracking |
												//kControlGetsFocusOnClick |
												//kControlSupportsGetRegion |
												kControlSupportsDragAndDrop |
												kControlSupportsSetCursor;
				
				
				// Return the features of this control; note that the drag-and-drop feature
				// in particular will NOT take effect unless this information is in the event
				// BEFORE the call to CallNextEventHandler().
				//
				// The drag-and-drop bit is set here, however views themselves do NOT install
				// drag handlers currently.  They are considered “dumb”; their intelligence
				// is added by the Session module, which attaches them to (for instance) a
				// running Unix process.  The Session module installs events on views that
				// handle keyboard input and drag-and-drop, however the drag and focus features
				// must be added here at control initialization time.
				result = SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32,
											sizeof(controlFeatures), &controlFeatures);
				if (noErr == result)
				{
					result = CallNextEventHandler(inHandlerCallRef, inEvent);
				}
			}
			break;
		
		case kEventControlActivate:
		case kEventControlDeactivate:
			//Console_WriteLine("HI OBJECT control activate or deactivate for terminal view");
			result = receiveTerminalViewActiveStateChange(inHandlerCallRef, inEvent, view);
			break;
		
		case kEventControlDraw:
			//Console_WriteLine("HI OBJECT control draw for terminal view");
			result = receiveTerminalViewDraw(inHandlerCallRef, inEvent, view);
			break;
		
		case kEventControlGetData:
			//Console_WriteLine("HI OBJECT control get data for terminal view");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				result = receiveTerminalViewGetData(inHandlerCallRef, inEvent, view);
			}
			break;
		
		case kEventControlGetPartRegion:
			//Console_WriteLine("HI OBJECT control get part region for terminal view");
			result = receiveTerminalViewRegionRequest(inHandlerCallRef, inEvent, view);
			break;
		
		case kEventControlHitTest:
			//Console_WriteLine("HI OBJECT control hit test for terminal view");
			result = receiveTerminalViewHitTest(inHandlerCallRef, inEvent, view);
			break;
		
		case kEventControlSetCursor:
			//Console_WriteLine("HI OBJECT control set cursor for terminal view");
			{
				UInt32		eventModifiers = 0;
				Boolean		isInSelection = false;
				
				
				// attempt to read key modifiers, so as to make the cursor change more accurate
				if (noErr != CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers,
																	typeUInt32, eventModifiers))
				{
					eventModifiers = EventLoop_ReturnCurrentModifiers();
				}
				
				// when there is a text selection, see if the mouse is in it
				if (TerminalView_TextSelectionExists(view))
				{
					// figure out where the mouse is
					Point	mouseLocation;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation,
																	typeQDPoint, mouseLocation);
					if (noErr == result)
					{
						// if the mouse is in the selection, indicate it is draggable
						isInSelection = TerminalView_PtInSelection(view, mouseLocation);
					}
				}
				
				// the event is successfully handled
				result = noErr;
				
				// change the cursor appropriately...the cursor for dragging
				// terminal text selections appears in background windows
				// because “direct drag” is implemented for selections
				if (isInSelection)
				{
					// cursor applicable to regions of text that are selected
					if (eventModifiers & cmdKey)
					{
						// if clicked, a highlighted URL would be opened
						[[NSCursor pointingHandCursor] set];
					}
					else
					{
						// text is draggable
						[[NSCursor openHandCursor] set];
					}
				}
				else
				{
					// these cursors should not appear in background windows
					if (GetUserFocusWindow() == TerminalView_ReturnWindow(view))
					{
						// cursor applicable to regions of text that are not selected
						if (eventModifiers & controlKey)
						{
							// if clicked, there would be a contextual menu
							[[NSCursor contextualMenuCursor] set];
						}
						else if ((eventModifiers & optionKey) && (eventModifiers & cmdKey))
						{
							// if clicked, the terminal cursor will move to the mouse location
							My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
							NSCursor*					cursorMove = customCursorMoveTerminalCursor(isSmallIBeam(viewPtr));
							
							
							if (nil == cursorMove)
							{
								// fall back to some standard system cursor
								[[NSCursor arrowCursor] set];
							}
							else
							{
								[cursorMove set];
							}
						}
						else if (eventModifiers & optionKey)
						{
							// if clicked, the text selection would be rectangular
							NSCursor*	cursorCrosshairs = customCursorCrosshairs();
							
							
							if (nil == cursorCrosshairs)
							{
								// fall back to standard system cursor
								[[NSCursor crosshairCursor] set];
							}
							else
							{
								[cursorCrosshairs set];
							}
						}
						else
						{
							// normal - text selection cursor
							My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
							NSCursor*					cursorIBeam = customCursorIBeam(isSmallIBeam(viewPtr));
							
							
							if (nil == cursorIBeam)
							{
								// fall back to standard system cursor
								[[NSCursor IBeamCursor] set];
							}
							else
							{
								[cursorIBeam set];
							}
						}
					}
					else
					{
						[[NSCursor arrowCursor] set];
					}
				}
			}
			break;
		
		case kEventControlTrack:
			//Console_WriteLine("HI OBJECT control track for terminal view");
			result = receiveTerminalViewTrack(inHandlerCallRef, inEvent, view);
			break;
		
		default:
			// ???
			break;
		}
	}
	return result;
}// receiveTerminalHIObjectEvents


/*!
Handles "kEventControlActivate" and "kEventControlDeactivate"
of "kEventClassControl".

Invoked by Mac OS X whenever a terminal view is activated or
dimmed.

(3.1)
*/
OSStatus
receiveTerminalViewActiveStateChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
										 EventRef				inEvent,
										 TerminalViewRef		inTerminalViewRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert((kEventKind == kEventControlActivate) || (kEventKind == kEventControlDeactivate));
	{
		My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
		
		
		// update state flag
		viewPtr->isActive = (kEventKind == kEventControlActivate);
		
		// since inactive and active terminal views have different appearances, force redraws
		// when they are activated or deactivated
		unless (gPreferenceProxies.dontDimTerminals)
		{
			HIViewRef	viewBeingActivatedOrDeactivated = nullptr;
			
			
			// get the target view
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef,
															viewBeingActivatedOrDeactivated);
			
			// if the view was found, continue
			if (noErr == result)
			{
				result = HIViewSetNeedsDisplay(viewBeingActivatedOrDeactivated, true);
			}
		}
	}
	return result;
}// receiveTerminalViewActiveStateChange


/*!
Handles "kEventControlContextualMenuClick" of "kEventClassControl"
for terminal views.

(3.0)
*/
OSStatus
receiveTerminalViewContextualMenuSelect	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
										 EventRef				inEvent,
										 void*					inTerminalViewRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalViewRef		terminalView = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlContextualMenuClick);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the view in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, proceed
		if (noErr == result)
		{
			My_TerminalViewAutoLocker	ptr(gTerminalViewPtrLocks(), terminalView);
			
			
			if (view == ptr->contentHIView)
			{
				// display a contextual menu
				NSMenu*		contextualMenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
				NSPoint		globalLocation = [NSEvent mouseLocation];
				
				
				// set up the contextual menu
				//[contextualMenu setAllowsContextMenuPlugIns:NO];
				populateContextualMenu(ptr, contextualMenu);
				
				// display the menu; note that this mechanism does not require
				// either a positioning item or a view, effectively making the
				// menu appear at the given global location
				UNUSED_RETURN(BOOL)[contextualMenu popUpMenuPositioningItem:nil atLocation:globalLocation inView:nil];
				
				result = noErr; // event is completely handled
			}
			else
			{
				// ???
				result = eventNotHandledErr;
			}
		}
	}
	
	return result;
}// receiveTerminalViewContextualMenuSelect


/*!
Handles "kEventControlDraw" of "kEventClassControl".

Invoked by Mac OS X whenever the foreground of a terminal
view needs to be rendered.

(3.0)
*/
OSStatus
receiveTerminalViewDraw		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 TerminalViewRef		inTerminalViewRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (noErr == result)
		{
			HIViewPartCode		partCode = 0;
			CGrafPtr			drawingPort = nullptr;
			CGContextRef		drawingContext = nullptr;
			CGrafPtr			oldPort = nullptr;
			GDHandle			oldDevice = nullptr;
			
			
			// find out the current port
			GetGWorld(&oldPort, &oldDevice);
			
			// determine which part (if any) to draw; if none, draw everything
			UNUSED_RETURN(OSStatus)CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, drawingPort);
			if (noErr != result)
			{
				// use current port
				drawingPort = oldPort;
			}
			
			// determine the context to draw in with Core Graphics
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef,
															drawingContext);
			assert_noerr(result);
			
			// if all information can be found, proceed with drawing
			if (noErr == result)
			{
				My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				
				
				// draw text
				if (nullptr != viewPtr)
				{
					Rect		clipBounds;
					HIRect		floatBounds;
					CGRect		floatClipBounds;
					HIShapeRef	optionalTargetShape = nullptr;
					
					
					SetPort(drawingPort);
					
					// determine boundaries of the content view being drawn;
					// ensure view-local coordinates
					HIViewGetBounds(view, &floatBounds);
					
					// maybe a focus region has been provided
					if (noErr == CarbonEventUtilities_GetEventParameter(inEvent, kEventParamShape, typeHIShapeRef,
																		optionalTargetShape))
					{
						UNUSED_RETURN(CGRect*)HIShapeGetBounds(optionalTargetShape, &floatClipBounds);
						RegionUtilities_SetRect(&clipBounds,
												STATIC_CAST(floatClipBounds.origin.x, SInt16),
												STATIC_CAST(floatClipBounds.origin.y, SInt16),
												STATIC_CAST((floatClipBounds.origin.x + floatClipBounds.size.width), SInt16),
												STATIC_CAST((floatClipBounds.origin.y + floatClipBounds.size.height), SInt16));
					}
					else
					{
						SetRectRgn(gInvalidationScratchRegion(), 0, 0, STATIC_CAST(floatBounds.size.width, SInt16),
									STATIC_CAST(floatBounds.size.height, SInt16));
						GetRegionBounds(gInvalidationScratchRegion(), &clipBounds);
					}
					
					if ((partCode == kTerminalView_ContentPartText) ||
						(partCode == kControlEntireControl) ||
						((partCode >= kTerminalView_ContentPartFirstLine) &&
							(partCode <= kTerminalView_ContentPartLastLine)))
					{
						// perform any necessary rendering for drags
						{
							Boolean		dragHighlight = false;
							UInt32		actualSize = 0;
							
							
							if (noErr != GetControlProperty(view, AppResources_ReturnCreatorCode(),
															kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
															sizeof(dragHighlight), &actualSize,
															&dragHighlight))
							{
								dragHighlight = false;
							}
							
							if (dragHighlight)
							{
								DragAndDrop_ShowHighlightBackground(drawingContext, floatBounds);
								// frame is drawn at the end, after any content
							}
							else
							{
								// hide is not necessary because the HIView model ensures the
								// backdrop behind this view is painted as needed
							}
							
							// tell text routines to draw in black if there is a drag highlight
							viewPtr->screen.currentRenderDragColors = dragHighlight;
						}
						
						// draw text and graphics
					#if 0
						{
							UInt16					startColumn = 0;
							UInt16					pastTheEndColumn = 0;
							TerminalView_RowIndex	startRow = 0;
							TerminalView_RowIndex	pastTheEndRow = 0;
							
							
							// draw EVERYTHING
							getVirtualVisibleRegion(viewPtr, &startColumn, &startRow, &pastTheEndColumn, &pastTheEndRow);
							if (pastTheEndColumn > startColumn)
							{
								//DEBUG//Console_WriteValueFloat4("virt vis", startColumn, pastTheEndColumn, startRow, pastTheEndRow);
								if ((partCode >= kTerminalView_ContentPartFirstLine) &&
									(partCode <= kTerminalView_ContentPartLastLine))
								{
									// restrict drawing to one row
									startRow = partCode - 1;
									pastTheEndRow = startRow + 1;
									Console_WriteValuePair("view part code indicates past-the-end range", startRow, pastTheEndRow);
								}
								
								// draw the window text
								(Boolean)drawSection(viewPtr, drawingContext, startColumn - viewPtr->screen.leftVisibleEdgeInColumns,
														startRow - viewPtr->screen.topVisibleEdgeInRows,
														pastTheEndColumn - viewPtr->screen.leftVisibleEdgeInColumns,
														pastTheEndRow - viewPtr->screen.topVisibleEdgeInRows);
							}
						}
					#else
						{
							// draw only the requested area; convert from pixels to screen cells
							HIPoint const		kTopLeftAnchor = CGPointMake(clipBounds.left, clipBounds.top);
							HIPoint const		kBottomRightAnchor = CGPointMake(clipBounds.right, clipBounds.bottom);
							TerminalView_Cell	leftTopCell;
							TerminalView_Cell	rightBottomCell;
							Float32				deltaColumn = 0;
							Float32				deltaRow = 0;
							
							
							// figure out what cells to draw
							UNUSED_RETURN(Boolean)findVirtualCellFromScreenPoint(viewPtr, kTopLeftAnchor, leftTopCell, deltaColumn, deltaRow);
							UNUSED_RETURN(Boolean)findVirtualCellFromScreenPoint(viewPtr, kBottomRightAnchor, rightBottomCell, deltaColumn, deltaRow);
							
							// draw the text in the clipped area
							UNUSED_RETURN(Boolean)drawSection(viewPtr, drawingContext, leftTopCell.first - viewPtr->screen.leftVisibleEdgeInColumns,
																leftTopCell.second - viewPtr->screen.topVisibleEdgeInRows,
																rightBottomCell.first + 1/* past-the-end */ - viewPtr->screen.leftVisibleEdgeInColumns,
																rightBottomCell.second + 1/* past-the-end */ - viewPtr->screen.topVisibleEdgeInRows);
						}
					#endif
						viewPtr->text.attributes = kTextAttributes_Invalid; // forces attributes to reset themselves properly
						
						// if, after drawing all text, no text is actually blinking,
						// then disable the animation timer (so unnecessary refreshes
						// are not done); otherwise, install it
						setBlinkingTimerActive(viewPtr, (viewPtr->isActive) && (cursorBlinks(viewPtr) ||
																				(viewPtr->screen.currentRenderBlinking)));
						
						// if inactive, render the text selection as an outline
						// (if active, the call above to draw the text will also
						// have drawn the active selection)
						if ((false == viewPtr->isActive) && viewPtr->text.selection.exists)
						{
							HIShapeRef		selectionShape = getSelectedTextAsNewHIShape(viewPtr, 1.0/* inset */);
							
							
							if (nullptr != selectionShape)
							{
								RGBColor		highlightColorRGB;
								CGDeviceColor	highlightColorDevice;
								OSStatus		error = noErr;
								
								
								// TEMPORARY - account for QuickDraw/Quartz differences until conversion is complete
								{
									HIMutableShapeRef		offsetCopy = HIShapeCreateMutableCopy(selectionShape);
									
									
									if (nullptr != offsetCopy)
									{
										HIShapeOffset(offsetCopy, -1.0, -1.0);
										
										CFRelease(selectionShape), selectionShape = nullptr;
										selectionShape = offsetCopy;
									}
								}
								
								// draw outline
								CGContextSetLineWidth(drawingContext, 2.0); // make thick outline frame on Mac OS X
								CGContextSetLineCap(drawingContext, kCGLineCapRound);
								LMGetHiliteRGB(&highlightColorRGB);
								highlightColorDevice = ColorUtilities_CGDeviceColorMake(highlightColorRGB);
								CGContextSetRGBStrokeColor(drawingContext, highlightColorDevice.red, highlightColorDevice.green,
															highlightColorDevice.blue, 1.0/* alpha */);
								error = HIShapeReplacePathInCGContext(selectionShape, drawingContext);
								assert_noerr(error);
								CGContextStrokePath(drawingContext);
								
								// free allocated memory
								CFRelease(selectionShape), selectionShape = nullptr;
							}
						}
						
						// perform any necessary rendering for drags
						{
							Boolean		dragHighlight = false;
							UInt32		actualSize = 0;
							
							
							if (noErr != GetControlProperty(view, AppResources_ReturnCreatorCode(),
															kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
															sizeof(dragHighlight), &actualSize,
															&dragHighlight))
							{
								dragHighlight = false;
							}
							
							if (dragHighlight)
							{
								DragAndDrop_ShowHighlightFrame(drawingContext, floatBounds);
							}
						}
					}
					
					// render margin line, if requested
					if (gPreferenceProxies.renderMarginAtColumn > 0)
					{
						Rect			dummyBounds;
						RGBColor		highlightColorRGB;
						CGDeviceColor	highlightColorDevice;
						
						
						getRowSectionBounds(viewPtr, 0/* row; does not matter which */,
											gPreferenceProxies.renderMarginAtColumn/* zero-based column number, but following column is desired */,
											1/* character count - not important */, &dummyBounds);
						CGContextSetLineWidth(drawingContext, 2.0);
						CGContextSetLineCap(drawingContext, kCGLineCapButt);
						LMGetHiliteRGB(&highlightColorRGB);
						highlightColorDevice = ColorUtilities_CGDeviceColorMake(highlightColorRGB);
						CGContextSetRGBStrokeColor(drawingContext, highlightColorDevice.red, highlightColorDevice.green,
													highlightColorDevice.blue, 1.0/* alpha */);
						CGContextBeginPath(drawingContext);
						CGContextMoveToPoint(drawingContext, dummyBounds.left, floatBounds.origin.y);
						CGContextAddLineToPoint(drawingContext, dummyBounds.left, floatBounds.origin.y + floatBounds.size.height);
						CGContextStrokePath(drawingContext);
					}
					
					// kTerminalView_ContentPartCursor
					if (kMyCursorStateVisible == viewPtr->screen.cursor.currentState)
					{
						CGContextSaveRestore	_(drawingContext);
						TextAttributes_Object	cursorAttributes = Terminal_CursorReturnAttributes(viewPtr->screen.ref);
						CGRect					cursorFloatBounds = CGRectMake(viewPtr->screen.cursor.bounds.left - 1,
																				viewPtr->screen.cursor.bounds.top - 1,
																				viewPtr->screen.cursor.bounds.right -
																					viewPtr->screen.cursor.bounds.left
																					+ 1/* TEMPORARY Quartz/QD conversion */,
																				viewPtr->screen.cursor.bounds.bottom -
																					viewPtr->screen.cursor.bounds.top
																					+ 1/* TEMPORARY Quartz/QD conversion */);
						
						
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
						CGContextFillRect(drawingContext, cursorFloatBounds);
						
						// if the terminal is currently in password mode, annotate the cursor
						if (Terminal_IsInPasswordMode(viewPtr->screen.ref))
						{
							CGFloat const	newHeight = viewPtr->text.font.heightPerCharacter; // TEMPORARY; does not handle double-height lines (probably does not need to)
							CGFloat			dotDimensions = STATIC_CAST(ceil(viewPtr->text.font.heightPerCharacter * 0.33/* arbitrary */), CGFloat);
							CGRect			fullRectangleBounds = cursorFloatBounds;
							CGRect			dotBounds = CGRectMake(0, 0, dotDimensions, dotDimensions); // arbitrary
							
							
							// the cursor may not be block-size so convert to a block rectangle
							// before determining the placement of the oval inset
							fullRectangleBounds.origin.y = fullRectangleBounds.origin.y + fullRectangleBounds.size.height - newHeight;
							fullRectangleBounds.size.width = viewPtr->text.font.widthPerCharacter;
							fullRectangleBounds.size.height = newHeight;
							RegionUtilities_CenterHIRectIn(dotBounds, fullRectangleBounds);
							++dotBounds.origin.x; // TEMPORARY: figure out why this correction seems necessary
							
							// draw the dot in the middle of the cell that the cursor occupies
							if (kTerminalView_CursorTypeBlock == gPreferenceProxies.cursorType)
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
					
					// kTerminalView_ContentPartCursorGhost
					if (kMyCursorStateVisible == viewPtr->screen.cursor.ghostState)
					{
						// draw the cursor at its ghost location (with ghost appearance)
						// UNIMPLEMENTED
					}
				}
			}
			
			// restore port
			SetGWorld(oldPort, oldDevice);
		}
	}
	return result;
}// receiveTerminalViewDraw


/*!
Handles "kEventControlGetData" of "kEventClassControl".

Sets the control kind.

(3.1)
*/
OSStatus
receiveTerminalViewGetData	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef					inEvent,
							 TerminalViewRef			UNUSED_ARGUMENT(inTerminalViewRef))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlGetData);
	{
		// determine the type of data being retrieved
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (result == noErr)
		{
			HIViewPartCode		partNeedingDataGet = kControlNoPart;
			
			
			// determine the part for which data is being set
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode,
															partNeedingDataGet);
			if (result == noErr)
			{
				FourCharCode	dataType = '----';
				
				
				// determine the setting that is being changed
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlDataTag,
																typeEnumeration, dataType);
				if (noErr == result)
				{
					switch (dataType)
					{
					case kControlKindTag:
						{
							SInt32		dataSize = 0;
							
							
							result = CarbonEventUtilities_GetEventParameter
										(inEvent, kEventParamControlDataBufferSize,
											typeLongInteger, dataSize);
							if (noErr == result)
							{
								ControlKind*	kindPtr = nullptr;
								
								
								assert(dataSize == sizeof(ControlKind));
								result = CarbonEventUtilities_GetEventParameter
											(inEvent, kEventParamControlDataBuffer,
												typePtr, kindPtr);
								if (noErr == result)
								{
									kindPtr->signature = AppResources_ReturnCreatorCode();
									kindPtr->kind = kConstantsRegistry_ControlKindTerminalView;
								}
							}
						}
						result = noErr;
						break;
					
					default:
						// ???
						break;
					}
				}
			}
		}
	}
	return result;
}// receiveTerminalViewGetData


/*!
Handles "kEventControlHitTest" of "kEventClassControl".

Invoked by Mac OS X whenever a point needs to be mapped
to a view part code (presumably due to a mouse click).

(3.0)
*/
OSStatus
receiveTerminalViewHitTest	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 TerminalViewRef		inTerminalViewRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	// for simplicity, the tracking handler may invoke this directly with one of its events,
	// so the event kind will be kEventControlTrack in those cases
	assert((kEventKind == kEventControlHitTest) || (kEventKind == kEventControlTrack));
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (result == noErr)
		{
			HIPoint   localMouse;
			
			
			// determine where the mouse is
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, localMouse);
			if (result == noErr)
			{
				My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				HIViewPartCode				hitPart = kTerminalView_ContentPartVoid;
				HIRect						viewBounds;
				
				
				// find the part the mouse is in
				HIViewGetBounds(view, &viewBounds);
				if (CGRectContainsPoint(viewBounds, localMouse))
				{
					// TEMPORARY - a hit anywhere in the view is in text, otherwise nowhere; ignore other parts
					// (this will be updated to detect clicks on specific lines, clicks in the cursor, etc.)
					hitPart = kTerminalView_ContentPartText;
				}
				
				// update the part code parameter with the part under the mouse
				result = SetEventParameter(inEvent, kEventParamControlPart,
											typeControlPartCode, sizeof(hitPart), &hitPart);
			}
		}
	}
	return result;
}// receiveTerminalViewHitTest


/*!
Handles "kEventRawKeyDown" of "kEventClassKeyboard".
Responds by modifying currently selected text (if any)
based on keyboard shortcuts that alter selections.

Invoked by Mac OS X whenever the user hits a key.

(3.1)
*/
OSStatus
receiveTerminalViewRawKeyDown	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inTerminalViewRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalViewRef		view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassKeyboard);
	assert((kEventKind == kEventRawKeyDown) || (kEventKind == kEventRawKeyRepeat));
	{
		UInt32		virtualKeyCode = 0;
		
		
		// get the key
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, virtualKeyCode);
		
		// if the key was found, continue
		if (noErr == result)
		{
			UInt32		modifiers = 0;
			
			
			// get modifier key states
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
			if (noErr == result)
			{
				My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
				TerminalView_CellRange		oldSelectionRange = viewPtr->text.selection.range;
				Boolean						selectionChanged = false;
				
				
				// take the opportunity to reset the cursor state, so it is fully visible as
				// keys are being pressed; also reset the stage so that the timing is right
				viewPtr->animation.cursor.blinkAlpha = 1.0;
				viewPtr->animation.rendering.stage = 0;
				
				// ignore Caps Lock and extra keys for the sake of the comparisons below...
				modifiers &= (cmdKey | shiftKey | optionKey | controlKey);
				
				result = eventNotHandledErr; // initially...
				switch (virtualKeyCode)
				{
				case 0x3B:
				case 0x7B:
					// left arrow
					if (false == viewPtr->text.selection.readOnly)
					{
						if (modifiers == shiftKey)
						{
							if ((kMy_SelectionModeUnset == viewPtr->text.selection.keyboardMode) ||
								(false == viewPtr->text.selection.exists))
							{
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeBeginning;
							}
							
							if (false == viewPtr->text.selection.exists)
							{
								TerminalView_SelectBeforeCursorCharacter(view);
							}
							else
							{
								// shift-left-arrow
								TerminalView_Cell&		anchorToChange = (kMy_SelectionModeChangeEnd == viewPtr->text.selection.keyboardMode)
																			// deselect the character to the right of the bottom selection anchor
																			? viewPtr->text.selection.range.second
																			// extend top selection anchor one character backward
																			: viewPtr->text.selection.range.first;
								
								
								// this wraps to the next line, but the wrap column depends on
								// the style (rectangular or not)
								if (anchorToChange.first > 0)
								{
									// back up one character, same line
									--anchorToChange.first;
								}
								else
								{
									// move to previous line, end
									if (false == viewPtr->text.selection.isRectangular)
									{
										anchorToChange.first = Terminal_ReturnColumnCount(viewPtr->screen.ref);
									}
									--anchorToChange.second;
								}
								selectionChanged = true;
							}
							
							// event is handled
							result = noErr;
						}
						else if (modifiers == (shiftKey | cmdKey))
						{
							if ((kMy_SelectionModeUnset == viewPtr->text.selection.keyboardMode) ||
								(false == viewPtr->text.selection.exists))
							{
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeBeginning;
							}
							
							if (false == viewPtr->text.selection.exists)
							{
								TerminalView_SelectBeforeCursorCharacter(view);
							}
							
							// shift-command-left-arrow
							if (kMy_SelectionModeChangeEnd == viewPtr->text.selection.keyboardMode)
							{
								// deselect all characters on this line
								viewPtr->text.selection.range.second.first = 0;
							}
							else
							{
								// extend selection to beginning of line
								viewPtr->text.selection.range.first.first = 0;
							}
							selectionChanged = true;
							
							// event is handled
							result = noErr;
						}
					}
					break;
				
				case 0x3C:
				case 0x7C:
					// right arrow
					if (false == viewPtr->text.selection.readOnly)
					{
						if (modifiers == shiftKey)
						{
							if ((kMy_SelectionModeUnset == viewPtr->text.selection.keyboardMode) ||
								(false == viewPtr->text.selection.exists))
							{
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeEnd;
							}
							
							if (false == viewPtr->text.selection.exists)
							{
								TerminalView_SelectCursorCharacter(view);
							}
							else
							{
								// shift-right-arrow
								TerminalView_Cell&		anchorToChange = (kMy_SelectionModeChangeEnd == viewPtr->text.selection.keyboardMode)
																			// extend bottom selection anchor one character forward
																			? viewPtr->text.selection.range.second
																			// deselect the character to the left of the top selection anchor
																			: viewPtr->text.selection.range.first;
								
								
								// this wraps to the next line, but the wrap column depends on
								// the style (rectangular or not)
								if (anchorToChange.first < Terminal_ReturnColumnCount(viewPtr->screen.ref))
								{
									// go forward one character, same line
									++anchorToChange.first;
								}
								else
								{
									// move to next line, beginning
									if (false == viewPtr->text.selection.isRectangular)
									{
										anchorToChange.first = 0;
									}
									++anchorToChange.second;
								}
								selectionChanged = true;
							}
							
							// event is handled
							result = noErr;
						}
						else if (modifiers == (shiftKey | cmdKey))
						{
							if ((kMy_SelectionModeUnset == viewPtr->text.selection.keyboardMode) ||
								(false == viewPtr->text.selection.exists))
							{
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeEnd;
							}
							
							if (false == viewPtr->text.selection.exists)
							{
								TerminalView_SelectCursorCharacter(view);
							}
							
							// shift-command-right-arrow
							if (kMy_SelectionModeChangeEnd == viewPtr->text.selection.keyboardMode)
							{
								// extend selection to end of line
								viewPtr->text.selection.range.second.first = Terminal_ReturnColumnCount(viewPtr->screen.ref);
							}
							else
							{
								// deselect all characters on this line
								viewPtr->text.selection.range.first.first = Terminal_ReturnColumnCount(viewPtr->screen.ref);
							}
							selectionChanged = true;
							
							// event is handled
							result = noErr;
						}
					}
					break;
				
				case 0x3E:
				case 0x7E:
					// up arrow
					if (false == viewPtr->text.selection.readOnly)
					{
						if (modifiers == shiftKey)
						{
							if ((kMy_SelectionModeUnset == viewPtr->text.selection.keyboardMode) ||
								(false == viewPtr->text.selection.exists))
							{
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeBeginning;
							}
							
							if (false == viewPtr->text.selection.exists)
							{
								TerminalView_SelectCursorLine(view);
							}
							else
							{
								// shift-up-arrow
								TerminalView_Cell&		anchorToChange = (kMy_SelectionModeChangeEnd == viewPtr->text.selection.keyboardMode)
																			// reduce selection by one line off the bottom
																			? viewPtr->text.selection.range.second
																			// extend selection one line backward, same column
																			: viewPtr->text.selection.range.first;
								
								
								--anchorToChange.second;
								selectionChanged = true;
							}
							
							// event is handled
							result = noErr;
						}
					}
					break;
				
				case 0x3D:
				case 0x7D:
					// down arrow
					if (false == viewPtr->text.selection.readOnly)
					{
						if (modifiers == shiftKey)
						{
							if ((kMy_SelectionModeUnset == viewPtr->text.selection.keyboardMode) ||
								(false == viewPtr->text.selection.exists))
							{
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeChangeEnd;
							}
							
							if (false == viewPtr->text.selection.exists)
							{
								TerminalView_SelectCursorLine(view);
							}
							else
							{
								// shift-down-arrow
								TerminalView_Cell&		anchorToChange = (kMy_SelectionModeChangeEnd == viewPtr->text.selection.keyboardMode)
																			// extend selection one line forward, same column
																			? viewPtr->text.selection.range.second
																			// reduce selection by one line off the top
																			: viewPtr->text.selection.range.first;
								
								
								++anchorToChange.second;
								selectionChanged = true;
							}
							
							// event is handled
							result = noErr;
						}
					}
					break;
				
				default:
					// do not handle
					break;
				}
				
				if (selectionChanged)
				{
					// TEMPORARY - could adjust this to only invalidate the part that was
					// actually added/removed
					highlightVirtualRange(viewPtr, oldSelectionRange, kTextAttributes_Selected,
											false/* highlighted */, true/* draw */);
					highlightCurrentSelection(viewPtr, true/* highlighted */, true/* draw */);
					copySelectedTextIfUserPreference(viewPtr);
				}
			}
			else
			{
				result = eventNotHandledErr;
			}
		}
		else
		{
			result = eventNotHandledErr;
		}
	}
	return result;
}// receiveTerminalViewRawKeyDown


/*!
Handles "kEventControlGetPartRegion" of "kEventClassControl".

Invoked by Mac OS X whenever the boundaries of a particular
part of a terminal view must be determined.

(3.1)
*/
OSStatus
receiveTerminalViewRegionRequest	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef				inEvent,
									 TerminalViewRef		inTerminalViewRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlGetPartRegion);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (noErr == result)
		{
			HIViewPartCode		partNeedingRegion = kControlNoPart;
			
			
			// determine the focus part
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode,
															partNeedingRegion);
			if (noErr == result)
			{
				My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				Rect						partBounds;
				
				
				// IMPORTANT: All regions are currently rectangles, so this code is simplified.
				// If any irregular regions are added in the future, this has to be restructured.
				switch (partNeedingRegion)
				{
				case kControlStructureMetaPart:
				case kControlContentMetaPart:
				case kTerminalView_ContentPartText:
					GetControlBounds(viewPtr->contentHIView, &partBounds);
					RegionUtilities_OffsetRect(&partBounds, -partBounds.left, -partBounds.top); // make view-relative coordinates
					break;
				
				case kControlOpaqueMetaPart:
					// the text area is designed to draw on top of a background widget,
					// so in general it is not really considered opaque anywhere (this
					// could be changed for certain cases, however)
					bzero(&partBounds, sizeof(partBounds));
					break;
				
				case kTerminalView_ContentPartCursor:
					partBounds = viewPtr->screen.cursor.bounds;
					break;
				
				case kTerminalView_ContentPartCursorGhost:
					partBounds = viewPtr->screen.cursor.ghostBounds;
					break;
				
				case kTerminalView_ContentPartVoid:
				default:
					bzero(&partBounds, sizeof(partBounds));
					break;
				}
				
				// the line-specific regions are special part codes not
				// easily switched, so scan for that range
				if ((partNeedingRegion >= kTerminalView_ContentPartFirstLine) &&
					(partNeedingRegion <= kTerminalView_ContentPartLastLine))
				{
					// UNIMPLEMENTED
					result = eventNotHandledErr;
				}
				
				//Console_WriteValue("request was for region code", partNeedingRegion);
				//Console_WriteValueFloat4("returned terminal view region bounds",
				//							STATIC_CAST(partBounds.left, Float32),
				//							STATIC_CAST(partBounds.top, Float32),
				//							STATIC_CAST(partBounds.right, Float32),
				//							STATIC_CAST(partBounds.bottom, Float32));
				
				// modify the given region, which effectively returns the boundaries to the caller
				if (noErr == result)
				{
					RgnHandle	regionToSet = nullptr;
					OSStatus	error = noErr;
					
					
					error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlRegion, typeQDRgnHandle,
																	regionToSet);
					if (noErr != error) result = eventNotHandledErr;
					else
					{
						RectRgn(regionToSet, &partBounds);
						result = noErr;
					}
				}
			}
		}
	}
	return result;
}// receiveTerminalViewRegionRequest


/*!
Handles "kEventControlTrack" of "kEventClassControl".

Invoked by Mac OS X whenever the user is dragging the
mouse in a terminal view.  This handles text selection,
etc.

(3.0)
*/
OSStatus
receiveTerminalViewTrack	(EventHandlerCallRef	inHandlerCallRef,
							 EventRef				inEvent,
							 TerminalViewRef		inTerminalViewRef)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
	OSStatus					result = eventNotHandledErr;
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlTrack);
	if (false == viewPtr->text.selection.readOnly)
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (result == noErr)
		{
			Point   originalLocalMouse;
			
			
			// NOTE: the following is a TOTAL HACK to help Carbon and Cocoa play nicely together;
			// in certain situations, if a Cocoa window had focus and the user clicks in the
			// Carbon-based terminal window, the Cocoa window won’t lose focus (which is very
			// unexpected behavior); this FORCES the window belonging to this view to become the
			// focus from Cocoa’s point of view, so that it will behave better
			{
				HIWindowRef		viewWindow = HIViewGetWindow(view);
				
				
				SetUserFocusWindow(viewWindow);
				CocoaBasic_MakeFrontWindowCarbonUserFocusWindow();
			}
			
			// determine where the mouse is (view-relative!)
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, originalLocalMouse);
			if (result == noErr)
			{
				UInt32		currentModifiers = 0;
				
				
				// translate the mouse coordinates to QuickDraw (content-relative)
				screenToLocal(viewPtr, &originalLocalMouse.h, &originalLocalMouse.v);
				
				// get current modifiers
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, currentModifiers);
				if (result != noErr)
				{
					// guess
					currentModifiers = EventLoop_ReturnCurrentModifiers();
					result = noErr;
				}
				
				// track the mouse location
				if (viewPtr != nullptr)
				{
					MouseTrackingResult		trackingResult = kMouseTrackingMouseDown;
					Point					localMouse = originalLocalMouse;
					CGrafPtr				oldPort = nullptr;
					GDHandle				oldDevice = nullptr;
					Boolean					movedCursor = false; // used to avoid conflict with command-click URL handling
					Boolean					cannotBeDoubleClick = false; // this is set if certain actions occur (such as URL handling)
					Boolean					dragged = false; // whether text selection was dragged
					Boolean					mouseInSelection = false; // whether the mouse was clicked inside an existing text selection
					
					
					GetGWorld(&oldPort, &oldDevice);
					SetPortWindowPort(GetControlOwner(view));
					
					do
					{
						if ((currentModifiers & optionKey) && (currentModifiers & cmdKey))
						{
							// send host the appropriate sequences to move the cursor to the specified
							// position; this is done during the mouse-up-wait loop so that the user
							// can hold down these modifier keys and drag the mouse around, causing
							// the terminal cursor to follow the mouse continuously
							TerminalView_MoveCursorWithArrowKeys(viewPtr->selfRef, localMouse);
							movedCursor = true;
							cannotBeDoubleClick = true;
						}
						else
						{
							// regular mouse-down; either drag text or adjust the text selection;
							// first determine whether the current selection has been clicked;
							// if drag-and-drop is possible, then maybe this will result in a drag;
							// otherwise, it will cause the selection to be cancelled
							if (viewPtr->text.selection.exists)
							{
								RgnHandle	dragRgn = nullptr;
								
								
								dragRgn = getSelectedTextAsNewRegion(viewPtr);
								if (dragRgn != nullptr)
								{
									mouseInSelection = PtInRgn(localMouse, dragRgn);
									if ((mouseInSelection) && WaitMouseMoved(localMouse))
									{
										EventRecord		event;
										
										
										// the user has attempted to drag text; track the drag
										SetPortWindowPort(GetControlOwner(view));
										LocalToGlobalRegion(dragRgn);
										OutlineRegion(dragRgn);
										event.where = localMouse;
										SetPortWindowPort(GetControlOwner(view));
										LocalToGlobal(&event.where);
										event.modifiers = STATIC_CAST(currentModifiers, EventModifiers);
										UNUSED_RETURN(OSStatus)dragTextSelection(viewPtr, dragRgn, &event, &dragged);
										trackingResult = kMouseTrackingMouseUp; // terminate loop
										cannotBeDoubleClick = true;
									}
									else if (0 == (currentModifiers & shiftKey))
									{
										// no drag, but unshifted click; cancel the selection
										TerminalView_SelectNothing(viewPtr->selfRef);
									}
									DisposeRgn(dragRgn), dragRgn = nullptr;
								}
							}
							
							// if the user clicks outside the current selection, then extend or
							// replace the selection
							unless (mouseInSelection)
							{
								Point const		kOldLocalMouse = localMouse;
								
								
								// drag until the user releases the mouse, highlighting as the mouse moves
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
								trackTextSelection(viewPtr, localMouse, STATIC_CAST(currentModifiers, EventModifiers), &localMouse, &currentModifiers);
								
								// since trackTextSelection() loops on mouse-up, assume the mouse is now up
								trackingResult = kMouseTrackingMouseUp;
								
								// if the user moved the mouse a ways or cancelled the selection, it cannot be the start of a double-click
								unless (RegionUtilities_NearPoints(localMouse, kOldLocalMouse) && !(viewPtr->text.selection.exists))
								{
									cannotBeDoubleClick = true;
								}
							}
						}
						
						// find next mouse location
						if (trackingResult != kMouseTrackingMouseUp)
						{
							OSStatus	error = noErr;
							
							
							error = TrackMouseLocationWithOptions(nullptr/* port, or nullptr for current port */, 0/* options */,
																	kEventDurationForever/* timeout */, &localMouse,
																	&currentModifiers, &trackingResult);
							if (error != noErr) break;
						}
					}
					while (trackingResult != kMouseTrackingMouseUp);
					
					// a single click has occurred; check for single-click actions (like URL handling);
					// a command-click means the selection is to be interpreted as a URL, or (if there
					// is no selection) the text nearest the cursor is to be searched for something
					// that looks like a URL, at which time the text is handled like a URL
					if ((!movedCursor) && (currentModifiers & cmdKey))
					{
						cannotBeDoubleClick = true;
						SetPortWindowPort(GetControlOwner(view));
						if ((false == viewPtr->text.selection.exists) ||
							(false == TerminalView_PtInSelection(viewPtr->selfRef, localMouse)))
						{
							// find the URL around the click location, if possible
							SInt16		deltaColumn = 0;
							SInt16		deltaRow = 0;
							
							
							// cancel any previous selection
							TerminalView_SelectNothing(viewPtr->selfRef);
							
							// select an entire word
							UNUSED_RETURN(Boolean)findVirtualCellFromLocalPoint(viewPtr, localMouse,
																				viewPtr->text.selection.range.first,
																				deltaColumn, deltaRow);
							viewPtr->text.selection.range.second = viewPtr->text.selection.range.first;
							handleMultiClick(viewPtr, 2/* click count */);
						}
						
						// open the selection (apparently a URL)
						URL_HandleForScreenView(viewPtr->screen.ref, viewPtr->selfRef);
					}
					
					// see if a double- or triple-click occurs
					unless (cannotBeDoubleClick)
					{
						Boolean		foundAnotherClick = false;
						
						
						// see if a double-click occurs
						foundAnotherClick = EventLoop_IsNextDoubleClick(HIViewGetWindow(view), localMouse/* mouse in global coordinates */);
						if (foundAnotherClick)
						{
							SetPortWindowPort(GetControlOwner(view));
							GlobalToLocal(&localMouse);
							foundAnotherClick = RegionUtilities_NearPoints(originalLocalMouse, localMouse);
						}
						
						if (foundAnotherClick)
						{
							SInt16		deltaColumn = 0;
							SInt16		deltaRow = 0;
							
							
							// cancel any previous selection
							TerminalView_SelectNothing(viewPtr->selfRef);
							
							// select an entire word
							UNUSED_RETURN(Boolean)findVirtualCellFromLocalPoint(viewPtr, localMouse,
																				viewPtr->text.selection.range.first,
																				deltaColumn, deltaRow);
							viewPtr->text.selection.range.second = viewPtr->text.selection.range.first;
							handleMultiClick(viewPtr, 2/* click count */);
						}
						
						// if the mouse did come back up fairly soon (that is, within double-click
						// time), then do essentially the same thing once more, to see if another
						// quick click was made (which would make 3 clicks total)
						if (foundAnotherClick)
						{
							// look for a triple-click
							foundAnotherClick = EventLoop_IsNextDoubleClick(HIViewGetWindow(view), localMouse/* mouse in global coordinates */);
							if (foundAnotherClick)
							{
								SetPortWindowPort(GetControlOwner(view));
								GlobalToLocal(&localMouse);
								foundAnotherClick = RegionUtilities_NearPoints(originalLocalMouse, localMouse);
							}
							
							if (foundAnotherClick)
							{
								SInt16		deltaColumn = 0;
								SInt16		deltaRow = 0;
								
								
								// cancel any previous selection
								TerminalView_SelectNothing(viewPtr->selfRef);
								
								// select an entire line
								UNUSED_RETURN(Boolean)findVirtualCellFromLocalPoint(viewPtr, localMouse,
																					viewPtr->text.selection.range.first,
																					deltaColumn, deltaRow);
								viewPtr->text.selection.range.second = viewPtr->text.selection.range.first;
								handleMultiClick(viewPtr, 3/* click count */);
							}
						}
					}
					
					SetGWorld(oldPort, oldDevice);
					
					// update the key modifiers parameter with the latest key modifier states
					currentModifiers = EventLoop_ReturnCurrentModifiers();
					UNUSED_RETURN(OSStatus)SetEventParameter(inEvent, kEventParamKeyModifiers,
																typeUInt32, sizeof(currentModifiers), &currentModifiers);
					
					// find the part the mouse is in, and update the event to include this part
					// (it so happens the hit test handler is compatible with these requirements)
					result = receiveTerminalViewHitTest(inHandlerCallRef, inEvent, inTerminalViewRef);
				}
			}
		}
	}
	return result;
}// receiveTerminalViewTrack


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
Returns an approximation of how many characters are
represented by this terminal view’s text area.

(3.1)
*/
SInt64
returnNumberOfCharacters	(My_TerminalViewPtr		inTerminalViewPtr)
{
	UInt16 const	kNumVisibleRows = Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
	UInt32 const	kNumScrollbackRows = Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref);
	UInt16 const	kNumColumns = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	SInt64			result = (kNumVisibleRows + kNumScrollbackRows) * kNumColumns;
	
	
	return result;
}// returnNumberOfCharacters


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
	
	
	if (inTerminalViewPtr->text.selection.exists)
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
			
			
			// read every line of Unicode characters within this range;
			// if appropriate, ignore some characters on each line
			for (CFIndex i = kSelectionStart.second; i < kSelectionPastEnd.second; ++i)
			{
				UniChar const*		textBegin = nullptr;
				UniChar const*		textPastEnd = nullptr;
				
				
				if ((inTerminalViewPtr->text.selection.isRectangular) ||
					(1 == (kSelectionPastEnd.second - kSelectionStart.second)))
				{
					// for rectangular or one-line selections, copy a specific column range
					textGrabResult = Terminal_GetLineRange(inTerminalViewPtr->screen.ref, lineIterator,
															kSelectionStart.first, kSelectionPastEnd.first,
															textBegin, textPastEnd);
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
																textBegin, textPastEnd,
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
																textBegin, textPastEnd, kTerminal_TextFilterFlagsNoEndWhitespace);
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
															textBegin, textPastEnd, kTerminal_TextFilterFlagsNoEndWhitespace);
						if (kTerminal_ResultOK != textGrabResult)
						{
							Console_Warning(Console_WriteValue, "middle-spanning-line text copy failed, terminal error", textGrabResult);
							break;
						}
					}
				}
				
				// add the characters for the line...
				{
					ptrdiff_t	copyLength = (textPastEnd - textBegin);
					
					
					if (copyLength < 0)
					{
						Console_Warning(Console_WriteValue, "internal error, copy range ended up negative; proposed size", copyLength);
						copyLength = 0;
					}
					
					if (copyLength > 0)
					{
						ptrdiff_t const		kSanityCheckCopyLimit = 256; // arbitrary, but matches maximum line buffer size
						
						
						if (copyLength > kSanityCheckCopyLimit)
						{
							Console_Warning(Console_WriteValue, "internal error, unexpectedly large line copy (truncating); proposed size", copyLength);
							copyLength = kSanityCheckCopyLimit;
						}
						CFStringAppendCharacters(resultMutable, textBegin, copyLength/* number of characters */);
						
						// perform spaces-to-tabs substitution; this could be done while
						// appending text in the first place but instead it is done as a
						// lazy post-processing step (also, no effort is made to perform
						// particularly well...multiple linear searches are done to
						// iteratively replace ranges of spaces)
						if (inMaxSpacesToReplaceWithTabOrZero > 0)
						{
							CFRetainRelease		spacesString(CFStringCreateMutable
																(kCFAllocatorDefault,
																	inMaxSpacesToReplaceWithTabOrZero),
																	CFRetainRelease::kAlreadyRetained);
							CFStringRef			singleSpaceString = CFSTR(" "); // LOCALIZE THIS?
							CFStringRef			tabString = CFSTR("\011"); // LOCALIZE THIS?
							
							
							// create a string to represent the required whitespace range
							// (TEMPORARY: there is no reason to do this every time, it
							// could be cached at the time the user preference changes;
							// for now however the lazy approach is being taken)
							for (UInt16 spaceIndex = 0; spaceIndex < inMaxSpacesToReplaceWithTabOrZero; ++spaceIndex)
							{
								CFStringAppend(spacesString.returnCFMutableStringRef(), singleSpaceString);
							}
							
							// first replace all “long” series of spaces with single tabs, as per user preference
							UNUSED_RETURN(CFIndex)CFStringFindAndReplace(resultMutable, spacesString.returnCFStringRef(), tabString,
																			CFRangeMake(0, CFStringGetLength(resultMutable)),
																			0/* comparison options */);
							
							// replace all smaller ranges of spaces with one tab as well
							for (UInt16 maxSpaces = (inMaxSpacesToReplaceWithTabOrZero - 1); maxSpaces > 0; --maxSpaces)
							{
								// create a string representing the next-largest range of spaces...
								CFStringReplaceAll(spacesString.returnCFMutableStringRef(), CFSTR(""));
								for (UInt16 spaceIndex = 0; spaceIndex < maxSpaces; ++spaceIndex)
								{
									CFStringAppend(spacesString.returnCFMutableStringRef(), singleSpaceString);
								}
								
								// ...and turn those spaces into a tab
								UNUSED_RETURN(CFIndex)CFStringFindAndReplace(resultMutable, spacesString.returnCFStringRef(), tabString,
																				CFRangeMake(0, CFStringGetLength(resultMutable)),
																				0/* comparison options */);
							}
						}
					}
				}
				
				// if requested, add a new-line
				if (0 == (inFlags & kTerminalView_TextFlagInline))
				{
					// do not terminate last line
					if (i < (kSelectionPastEnd.second - 1))
					{
						CFStringAppendCString(resultMutable, "\015", kCFStringEncodingASCII);
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
	case kTerminal_ChangeTextEdited:
		if (IsValidWindowRef(HIViewGetWindow(viewPtr->contentHIView)))
		{
			Terminal_RangeDescriptionConstPtr	rangeInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																				Terminal_RangeDescriptionConstPtr);
			SInt32								i = 0;
			
			
			// debug
			//Console_WriteValuePair("first changed row, number of changed rows",
			//						rangeInfoPtr->firstRow, rangeInfoPtr->rowCount);
			for (i = rangeInfoPtr->firstRow - viewPtr->screen.topVisibleEdgeInRows;
					i < (rangeInfoPtr->firstRow - viewPtr->screen.topVisibleEdgeInRows + STATIC_CAST(rangeInfoPtr->rowCount, SInt32)); ++i)
			{
				invalidateRowSection(viewPtr, i, rangeInfoPtr->firstColumn, rangeInfoPtr->columnCount);
			}
		}
		break;
	
	case kTerminal_ChangeScrollActivity:
		{
			Terminal_ScrollDescriptionConstPtr	rangeInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																				Terminal_ScrollDescriptionConstPtr);
			
			
			if (viewPtr->animation.timer.isActive)
			{
				SetEmptyRgn(viewPtr->animation.rendering.region);
			}
			recalculateCachedDimensions(viewPtr);
			
			highlightCurrentSelection(viewPtr, false/* highlight */, true/* draw */);
			viewPtr->text.selection.range.first.second += rangeInfoPtr->rowDelta;
			viewPtr->text.selection.range.second.second += rangeInfoPtr->rowDelta;
			highlightCurrentSelection(viewPtr, true/* highlight */, true/* draw */);
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
		// when moving or hiding/showing the cursor, invalidate its original rectangle
		updateDisplayInRegion(viewPtr, viewPtr->screen.cursor.boundsAsRegion);
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
				setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds, viewPtr->screen.cursor.boundsAsRegion);
				updateDisplayInRegion(viewPtr, viewPtr->screen.cursor.boundsAsRegion);
			}
		}
	}
}// screenCursorChanged


/*!
Translates pixels in the coordinate system of a
screen so that they are relative to the origin of
the window itself.  In version 2.6, the program
always assumed that the screen origin was the
top-left corner of the window, which made it
extremely hard to change.  Now, this routine is
invoked everywhere to ensure that, before pixel
offsets are used to refer to the screen, they are
translated correctly.

If you are not interested in translating a point,
but rather in shifting a dimension horizontally or
vertically, you can pass nullptr for the dimension
that you do not use.

See also localToScreen().

(3.0)
*/
void
screenToLocal	(My_TerminalViewPtr		inTerminalViewPtr,
				 SInt16*				inoutHorizontalPixelOffsetFromScreenOrigin,
				 SInt16*				inoutVerticalPixelOffsetFromScreenOrigin)
{
	Point	origin;
	
	
	getScreenOrigin(inTerminalViewPtr, &origin.h, &origin.v);
	if (inoutHorizontalPixelOffsetFromScreenOrigin != nullptr)
	{
		(*inoutHorizontalPixelOffsetFromScreenOrigin) += origin.h;
	}
	if (inoutVerticalPixelOffsetFromScreenOrigin != nullptr)
	{
		(*inoutVerticalPixelOffsetFromScreenOrigin) += origin.v;
	}
}// screenToLocal


/*!
Translates a boundary in the coordinate system of a
screen so that it is relative to the origin of the
window itself.  In version 2.6, the program always
assumed that the screen origin was the top-left
corner of the window, which made it extremely hard
to change.  Now, this routine is invoked everywhere
to ensure that, before pixel offsets are used to
refer to the screen, they are translated correctly.

(3.0)
*/
void
screenToLocalRect	(My_TerminalViewPtr		inTerminalViewPtr,
					 Rect*					inoutScreenOriginBoundsPtr)
{
	if (inoutScreenOriginBoundsPtr != nullptr)
	{
		screenToLocal(inTerminalViewPtr, &inoutScreenOriginBoundsPtr->left, &inoutScreenOriginBoundsPtr->top);
		screenToLocal(inTerminalViewPtr, &inoutScreenOriginBoundsPtr->right, &inoutScreenOriginBoundsPtr->bottom);
	}
}// screenToLocalRect


/*!
This routine, of "SessionFactory_TerminalWindowOpProcPtr"
form, changes the opacity of the specified terminal window.
The "inAlphaFloatPtr" must be a pointer to a "float".

(4.0)
*/
void
setAlphaTerminalWindowOp	(TerminalWindowRef	inTerminalWindow,
							 void*				inAlphaFloatPtr,
							 SInt32				UNUSED_ARGUMENT(inData2),
							 void*				UNUSED_ARGUMENT(inoutResultPtr))
{
	float const*		kAlphaPtr = REINTERPRET_CAST(inAlphaFloatPtr, float const*);
	HIWindowRef const	kWindow  = (nullptr != inTerminalWindow)
									? TerminalWindow_ReturnWindow(inTerminalWindow)
									: nullptr;
	HIWindowRef const	kTabWindow  = (nullptr != inTerminalWindow)
										? TerminalWindow_ReturnTabWindow(inTerminalWindow)
										: nullptr;
	
	
	if (nullptr != kTabWindow)
	{
		UNUSED_RETURN(OSStatus)SetWindowAlpha(kTabWindow, *kAlphaPtr);
	}
	if (nullptr != kWindow)
	{
		UNUSED_RETURN(OSStatus)SetWindowAlpha(kWindow, *kAlphaPtr);
	}
}// setAlphaTerminalWindowOp


/*!
Copies a color into the blink animation palette for the
specified (zero-based) animation stage.  The animation timer
will use this palette to find colors as needed.

(4.0)
*/
inline void
setBlinkAnimationColor	(My_TerminalViewPtr		inTerminalViewPtr,
						 UInt16					inAnimationStage,
						 CGDeviceColor const*	inColorPtr)
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
	if (inTerminalViewPtr->animation.timer.isActive != inIsActive)
	{
		if (inIsActive)
		{
			UNUSED_RETURN(OSStatus)SetEventLoopTimerNextFireTime(inTerminalViewPtr->animation.timer.ref, kEventDurationNoWait);
		}
		else
		{
			UNUSED_RETURN(OSStatus)SetEventLoopTimerNextFireTime(inTerminalViewPtr->animation.timer.ref, kEventDurationForever);
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
		
		
		// cursor flashing is done within a thread; after a window closes,
		// the flashing ought to stop, but to make sure of that the window
		// must be valid (otherwise drawing occurs in the desktop!)
		renderCursor = (renderCursor && IsValidWindowRef(HIViewGetWindow(inTerminalViewPtr->contentHIView)));
		
		// change state
		inTerminalViewPtr->screen.cursor.currentState = newCursorState;
		
		// redraw the cursor if necessary
		if (renderCursor)
		{
			updateDisplayInRegion(inTerminalViewPtr, inTerminalViewPtr->screen.cursor.boundsAsRegion);
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
					 UInt16					inFontSizeOrZero,
					 Float32				inCharacterWidthScalingOrZero,
					 Boolean				inNotifyListeners)
{
	if (inTerminalViewPtr->isCocoa)
	{
		NSFontManager*		fontManager = [NSFontManager sharedFontManager];
		
		
		// release any previous fonts
		if (nil != inTerminalViewPtr->text.font.normalFont)
		{
			[inTerminalViewPtr->text.font.normalFont release], inTerminalViewPtr->text.font.normalFont = nil;
		}
		if (nil != inTerminalViewPtr->text.font.boldFont)
		{
			[inTerminalViewPtr->text.font.boldFont release], inTerminalViewPtr->text.font.boldFont = nil;
		}
		
		// find a font for most text
		inTerminalViewPtr->text.font.normalFont =
			[NSFont fontWithName:BRIDGE_CAST(inFontFamilyNameOrNull, NSString*) size:inFontSizeOrZero];
		[inTerminalViewPtr->text.font.normalFont retain];
		
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
		
		[inTerminalViewPtr->text.font.boldFont retain];
	}
	
	if (inFontFamilyNameOrNull != nullptr)
	{
		// remember font selection
		Boolean		getOK = CFStringGetPascalString(inFontFamilyNameOrNull,
													inTerminalViewPtr->text.font.familyName,
													sizeof(inTerminalViewPtr->text.font.familyName),
													kCFStringEncodingMacRoman);
		
		
		if (false == getOK)
		{
			Console_Warning(Console_WriteValueCFString, "failed to find Pascal string for font name", inFontFamilyNameOrNull);
		}
	}
	
	if (inFontSizeOrZero > 0)
	{
		inTerminalViewPtr->text.font.normalMetrics.size = inFontSizeOrZero;
	}
	
	if (inCharacterWidthScalingOrZero > 0)
	{
		inTerminalViewPtr->text.font.scaleWidthPerCharacter = inCharacterWidthScalingOrZero;
	}
	
	// set the font metrics (including double size)
	{
		CGrafPtr			oldPort = nullptr;
		GDHandle			oldDevice = nullptr;
		GrafPortFontState	fontState;
		
		
		GetGWorld(&oldPort, &oldDevice);
		setPortScreenPort(inTerminalViewPtr);
		Localization_PreservePortFontState(&fontState);
		TextFontByName(inTerminalViewPtr->text.font.familyName);
		TextSize(inTerminalViewPtr->text.font.normalMetrics.size);
		
		setUpScreenFontMetrics(inTerminalViewPtr);
		
		Localization_RestorePortFontState(&fontState);
		SetGWorld(oldPort, oldDevice);
	}
	
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
								inTerminalViewPtr->screen.cursor.boundsAsRegion);
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
Sets the current port to the port of the
indicated terminal window.

Obsolete.

(2.6)
*/
SInt16
setPortScreenPort	(My_TerminalViewPtr		inTerminalViewPtr)
{
	SInt16		result = 0;
	
	
	if (nullptr == inTerminalViewPtr) result = -3;
	else
	{
		static My_TerminalViewPtr	oldViewPtr = nullptr;
		HIWindowRef					window = HIViewGetWindow(inTerminalViewPtr->contentHIView);
		
		
		if (nullptr == window) result = -4;
		else
		{
			if (oldViewPtr != inTerminalViewPtr) // is last-used window different?
			{
				oldViewPtr = inTerminalViewPtr;
				inTerminalViewPtr->text.attributes = kTextAttributes_Invalid; // attributes will need setting
				result = 1;
			}
			SetPortWindowPort(window);
		}
	}
	
	return result;
}// setPortScreenPort


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
					 CGDeviceColor const*		inColorPtr)
{
	switch (inColorEntryNumber)
	{
	case kTerminalView_ColorIndexNormalBackground:
		{
			inTerminalViewPtr->text.colors[kMyBasicColorIndexNormalBackground] = *inColorPtr;
			if (inTerminalViewPtr->isCocoa)
			{
				// the view reads its color from the associated data structure automatically, so just redraw
				[inTerminalViewPtr->paddingNSView setNeedsDisplay:YES];
			}
			else if (nullptr != inTerminalViewPtr->paddingHIView)
			{
				OSStatus	error = noErr;
				
				
				error = SetControlProperty(inTerminalViewPtr->paddingHIView, AppResources_ReturnCreatorCode(),
											kConstantsRegistry_ControlPropertyTypeBackgroundColor,
											sizeof(*inColorPtr), inColorPtr);
				assert_noerr(error);
			}
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
			if (inTerminalViewPtr->isCocoa)
			{
				// the view reads its color from the associated data structure automatically, so just redraw
				[inTerminalViewPtr->backgroundNSView setNeedsDisplay:YES];
			}
			else if (nullptr != inTerminalViewPtr->backgroundHIView)
			{
				OSStatus	error = noErr;
				
				
				error = SetControlProperty(inTerminalViewPtr->backgroundHIView, AppResources_ReturnCreatorCode(),
											kConstantsRegistry_ControlPropertyTypeBackgroundColor,
											sizeof(*inColorPtr), inColorPtr);
				assert_noerr(error);
			}
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
		CGDeviceColor		colorValue;
		
		
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
setScreenCoreColor	(My_TerminalViewPtr		inTerminalViewPtr,
					 UInt16					inColorEntryNumber,
					 CGDeviceColor const*	inColorPtr)
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
						 CGDeviceColor const*		inColorPtr)
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

(4.0)
*/
void
setTextAttributesDictionary		(My_TerminalViewPtr			inTerminalViewPtr,
								 NSMutableDictionary*		inoutDictionary,
								 TextAttributes_Object		inAttributes,
								 Float32					UNUSED_ARGUMENT(inAlpha))
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
		
		if (1.0 != inTerminalViewPtr->text.font.scaleWidthPerCharacter)
		{
			[inoutDictionary setObject:[NSNumber numberWithFloat:((inTerminalViewPtr->text.font.baseWidthPerCharacter * inTerminalViewPtr->text.font.scaleWidthPerCharacter) -
																	inTerminalViewPtr->text.font.baseWidthPerCharacter)]
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
			CGDeviceColor	backgroundDeviceColor;
			CGDeviceColor	foregroundDeviceColor;
			
			
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
				NSColor*	searchResultTextColor = [[foregroundNSColor copy] autorelease];
				NSColor*	searchResultBackgroundColor = [[backgroundNSColor copy] autorelease];
				
				
				if (NO == [NSColor searchResultColorsForForeground:&searchResultTextColor
																	background:&searchResultBackgroundColor])
				{
					foregroundNSColor = [[foregroundNSColor copy] autorelease];
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
					NSColor*	selectionTextColor = [[foregroundNSColor copy] autorelease];
					NSColor*	selectionBackgroundColor = [[backgroundNSColor copy] autorelease];
					
					
					if (NO == [NSColor selectionColorsForForeground:&selectionTextColor
																	background:&selectionBackgroundColor])
					{
						// bail; force the default color even if it won’t look as good
						selectionTextColor = [NSColor blackColor];
					}
					foregroundNSColor = [selectionTextColor
											colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
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
region for the rectangle; if so, pass a valid region for
"inoutBoundsRegionOrNull".		

(3.0)
*/
void
setUpCursorBounds	(My_TerminalViewPtr			inTerminalViewPtr,
					 SInt16						inX,
					 SInt16						inY,
					 Rect*						outBoundsPtr,
					 RgnHandle					inoutBoundsRegionOrNull,
					 TerminalView_CursorType	inTerminalCursorType)
{
	enum
	{
		// cursor dimensions, in pixels
		// (NOTE: these currently hack in Quartz/QuickDraw conversion factors
		// and will eventually change)
		kTerminalCursorUnderscoreHeight = 4,
		kTerminalCursorThickUnderscoreHeight = 6,
		kTerminalCursorVerticalLineWidth = 4,
		kTerminalCursorThickVerticalLineWidth = 6
	};
	
	
	Point						characterSizeInPixels; // based on font metrics
	Rect						rowBounds;
	Rect						maximumCursorBounds;
	UInt16						thickness = 0; // used for non-block-shaped cursors
	TerminalView_CursorType		terminalCursorType = inTerminalCursorType;
	
	
	// if requested, use whatever cursor shape the user wants;
	// otherwise, calculate the bounds for the given shape
	if (inTerminalCursorType == kTerminalView_CursorTypeCurrentPreferenceValue)
	{
		terminalCursorType = cursorType(inTerminalViewPtr);
	}
	
	getRowBounds(inTerminalViewPtr, inY, &rowBounds);
	
	RegionUtilities_SetPoint(&characterSizeInPixels, getRowCharacterWidth(inTerminalViewPtr, inY), rowBounds.bottom - rowBounds.top);
	
	outBoundsPtr->left = inX * characterSizeInPixels.h;
	outBoundsPtr->top = rowBounds.top;
	
	maximumCursorBounds.left = outBoundsPtr->left;
	maximumCursorBounds.top = rowBounds.top;
	maximumCursorBounds.bottom = maximumCursorBounds.top + characterSizeInPixels.v;
	maximumCursorBounds.right = maximumCursorBounds.left + characterSizeInPixels.h;
	
	switch (terminalCursorType)
	{
	case kTerminalView_CursorTypeUnderscore:
	case kTerminalView_CursorTypeThickUnderscore:
		thickness = (terminalCursorType == kTerminalView_CursorTypeUnderscore)
						? kTerminalCursorUnderscoreHeight
						: kTerminalCursorThickUnderscoreHeight;
		outBoundsPtr->top += (characterSizeInPixels.v - thickness);
		outBoundsPtr->right = outBoundsPtr->left + characterSizeInPixels.h;	
		outBoundsPtr->bottom = outBoundsPtr->top + thickness;
		break;
	
	case kTerminalView_CursorTypeVerticalLine:
	case kTerminalView_CursorTypeThickVerticalLine:
		thickness = (terminalCursorType == kTerminalView_CursorTypeVerticalLine)
						? kTerminalCursorVerticalLineWidth
						: kTerminalCursorThickVerticalLineWidth;
		outBoundsPtr->right = outBoundsPtr->left + thickness;
		outBoundsPtr->bottom = outBoundsPtr->top + characterSizeInPixels.v;
		break;
	
	case kTerminalView_CursorTypeBlock:
	default:
		outBoundsPtr->right = outBoundsPtr->left + characterSizeInPixels.h;
		outBoundsPtr->bottom = outBoundsPtr->top + characterSizeInPixels.v;
		break;
	}
	
	if (nullptr != inoutBoundsRegionOrNull)
	{
		// TEMPORARY; not clear why correctional factors are needed now
		// but it seems that QuickDraw update regions interact badly
		// with Core Graphics drawings and trigger anti-aliasing effects
		// unless they are forced to overlap all filled-rectangle edges
		maximumCursorBounds.top -= 1;
		maximumCursorBounds.left -= 1;
		maximumCursorBounds.right += 2;
		maximumCursorBounds.bottom += 3;
		RectRgn(inoutBoundsRegionOrNull, &maximumCursorBounds);
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
	if (inTerminalViewPtr->isCocoa)
	{
		NSFont* const	sourceFont = [inTerminalViewPtr->text.font.normalFont screenFont];
		
		
		// TEMPORARY; eventually store floating-point values instead of casting
		inTerminalViewPtr->text.font.normalMetrics.ascent = STATIC_CAST([sourceFont ascender], SInt16);
		inTerminalViewPtr->text.font.heightPerCharacter = STATIC_CAST([sourceFont defaultLineHeightForFont], SInt16);
		inTerminalViewPtr->text.font.isMonospaced = (YES == [sourceFont isFixedPitch]);
		
		// Set the width per character.
		if (inTerminalViewPtr->text.font.isMonospaced)
		{
			inTerminalViewPtr->text.font.baseWidthPerCharacter = STATIC_CAST([sourceFont maximumAdvancement].width, SInt16);
		}
		else
		{
			inTerminalViewPtr->text.font.baseWidthPerCharacter = STATIC_CAST([sourceFont advancementForGlyph:[sourceFont glyphWithName:@"A"]].width,
																				SInt16);
		}
	}
	else
	{
		FontInfo	fontInfo;
		
		
		GetFontInfo(&fontInfo);
		inTerminalViewPtr->text.font.normalMetrics.ascent = fontInfo.ascent;
		inTerminalViewPtr->text.font.heightPerCharacter = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
		inTerminalViewPtr->text.font.isMonospaced = isMonospacedFont
													(FMGetFontFamilyFromName(inTerminalViewPtr->text.font.familyName));
		
		// Set the width per character; for monospaced fonts one can simply
		// use the width of any character in the set, but for proportional
		// fonts the question is more difficult to answer.  For one thing,
		// should the widest character in the font be used?  Technically yes
		// to ensure enough room to display any text, but in practice this
		// makes everything look really spaced out.  Another consideration
		// is the script system: not all fonts are Roman, so one cannot just
		// pick an arbitrary character because it could map to an unexpected
		// glyph in the font and be a different width than expected.
		//
		// For reasonable speed and decent spacing, proportional fonts use
		// the width of the widest character in the font, minus an arbitrary
		// amount proportional to the font size.
		if (inTerminalViewPtr->text.font.isMonospaced)
		{
			inTerminalViewPtr->text.font.baseWidthPerCharacter = CharWidth('A');
		}
		else
		{
			inTerminalViewPtr->text.font.baseWidthPerCharacter = fontInfo.widMax;
		}
	}
	
	// scale the font width according to user preferences
	{
		Float32		reduction = inTerminalViewPtr->text.font.baseWidthPerCharacter;
		
		
		reduction *= inTerminalViewPtr->text.font.scaleWidthPerCharacter;
		inTerminalViewPtr->text.font.widthPerCharacter = STATIC_CAST(reduction, SInt16);
	}
	
	// now use the font metrics to determine how big double-width text should be
	calculateDoubleSize(inTerminalViewPtr, inTerminalViewPtr->text.font.doubleMetrics.size,
						inTerminalViewPtr->text.font.doubleMetrics.ascent);
	
	// the thickness of lines in certain glyphs is also scaled with the font size
	inTerminalViewPtr->text.font.thicknessHorizontalLines = std::max(1.0f, inTerminalViewPtr->text.font.widthPerCharacter / 5.0f); // arbitrary
	inTerminalViewPtr->text.font.thicknessHorizontalBold = 2.0f * inTerminalViewPtr->text.font.thicknessHorizontalLines; // arbitrary
	inTerminalViewPtr->text.font.thicknessVerticalLines = std::max(1.0f, inTerminalViewPtr->text.font.heightPerCharacter / 7.0f); // arbitrary
	inTerminalViewPtr->text.font.thicknessVerticalBold = 2.0f * inTerminalViewPtr->text.font.thicknessVerticalLines; // arbitrary
}// setUpScreenFontMetrics
	

/*!
Ensures that the first point given is “sooner” in a
left-to-right localization than the 2nd point.  In
other words, the two points are swapped if necessary
to meet this condition.

If "inRectangular" is true, then the points may be
*redefined* so that the first point is the top-left
corner of the rectangle that the two points form,
and so the second point is the bottom-right corner
of the rectangle.

Specifically, if the two points are in the same row,
the first must be to the left of the second; but if
they are on different rows, then the first must be
on an earlier row than the second and then the left-
to-right positioning is immaterial.

LOCALIZE THIS

(3.0)
*/
void
sortAnchors		(TerminalView_Cell&		inoutPoint1,
				 TerminalView_Cell&		inoutPoint2,
				 Boolean				inRectangular)
{
	if (inRectangular)
	{
		// create a standardized rectangle (effectively sorts anchors)
		CGRect		myRect = CGRectStandardize
								(CGRectMake(inoutPoint1.first, inoutPoint1.second,
											inoutPoint2.first - inoutPoint1.first,
											inoutPoint2.second - inoutPoint1.second));
		
		
		// fill in new points; since the end point is exclusive to
		// the range, this simply requires adding the size parts
		inoutPoint1.first = STATIC_CAST(myRect.origin.x, UInt16);
		inoutPoint1.second = STATIC_CAST(myRect.origin.y, SInt32);
		inoutPoint2.first = STATIC_CAST(myRect.origin.x + myRect.size.width, UInt16);
		inoutPoint2.second = STATIC_CAST(myRect.origin.y + myRect.size.height, SInt32);
	}
	else
	{
		if ((inoutPoint2.second - inoutPoint1.second) <= 1)
		{
			// one-line range, or empty range; 1st must be to the left of the 2nd
			if (inoutPoint1.first > inoutPoint2.first)
			{
				std::swap(inoutPoint1, inoutPoint2);
				assert(inoutPoint2.first > inoutPoint1.first);
			}
		}
		else
		{
			// different lines; 1st must be on earlier row than the 2nd
			if (inoutPoint1.second > inoutPoint2.second)
			{
				std::swap(inoutPoint1, inoutPoint2);
				assert(inoutPoint2.second > inoutPoint1.second);
			}
		}
	}
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
Responds to a mouse-down event in a session window by
changing the cursor to an I-beam and creating or
extending the text selection of the specified window
based on the user’s use of the mouse and modifier keys.
The window also scrolls if the user moves the mouse out
of the boundaries of the window’s content area.  Make
sure that the “is text selected” flag has a value of 1
when using this routine -- otherwise, autoscrolling
screws up drawing of the text selection.

Loops until the user releases the mouse button.  On
output, the new mouse location and modifier key states
are returned.

(3.0)
*/
void
trackTextSelection	(My_TerminalViewPtr		inTerminalViewPtr,
					 Point					inLocalMouse,
					 EventModifiers			inModifiers,
					 Point*					outNewLocalMousePtr,
					 UInt32*				outNewModifiersPtr)
{
	if ((false == inTerminalViewPtr->text.selection.inhibited) &&
		(false == inTerminalViewPtr->text.selection.readOnly))
	{
		TerminalView_Cell		originalCellUnderMouse;
		TerminalView_Cell		cellUnderMouse;
		Point					previousLocalMouse;
		SInt16					deltaColumn = 0;
		SInt16					deltaRow = 0;
		CGrafPtr				oldPort = nullptr;
		GDHandle				oldDevice = nullptr;
		MouseTrackingResult		trackingResult = kMouseTrackingMouseDown;
		Boolean					extendSelection = false;
		
		
		GetGWorld(&oldPort, &oldDevice);
		
		// determine if the old selection should go away first
		extendSelection = (0 != (inModifiers & shiftKey));
		if (extendSelection)
		{
			originalCellUnderMouse = inTerminalViewPtr->text.selection.range.first;
		}
		else
		{
			inTerminalViewPtr->text.selection.exists = false;
			highlightCurrentSelection(inTerminalViewPtr, false/* is highlighted */, true/* redraw */);
		}
		
		// this must be set after unhighlighting (above), since for example
		// the user might have had a regular selection that is being replaced
		// by a rectangular one, and the right type of region should be erased
		inTerminalViewPtr->text.selection.isRectangular = (0 != (inModifiers & optionKey));
		
		if (inTerminalViewPtr->text.selection.isRectangular)
		{
			NSCursor*	cursorCrosshairs = customCursorCrosshairs();
			
			
			if (nil == cursorCrosshairs)
			{
				// fall back to standard system cursor
				[[NSCursor crosshairCursor] set];
			}
			else
			{
				[cursorCrosshairs set];
			}
		}
		else
		{
			NSCursor*	cursorIBeam = customCursorIBeam(isSmallIBeam(inTerminalViewPtr));
			
			
			if (nil == cursorIBeam)
			{
				// fall back to standard system cursor
				[[NSCursor IBeamCursor] set];
			}
			else
			{
				[cursorIBeam set];
			}
		}
		
		SetPortWindowPort(HIViewGetWindow(inTerminalViewPtr->contentHIView));
		
		// continue tracking until the mouse is released
		*outNewLocalMousePtr = inLocalMouse;
		*outNewModifiersPtr = inModifiers;
		previousLocalMouse = inLocalMouse;
		do
		{
			// find new mouse location, scroll if necessary
			UNUSED_RETURN(Boolean)findVirtualCellFromLocalPoint(inTerminalViewPtr, *outNewLocalMousePtr, cellUnderMouse, deltaColumn, deltaRow);
			UNUSED_RETURN(TerminalView_Result)TerminalView_ScrollAround(inTerminalViewPtr->selfRef, deltaColumn, deltaRow);
			
			// if the mouse moves (or the shift key is down), update the selection
			unless (RegionUtilities_NearPoints(*outNewLocalMousePtr, previousLocalMouse) && (false == extendSelection))
			{
				// toggle the highlight state of text between the current and last mouse positions
				highlightCurrentSelection(inTerminalViewPtr, false/* is highlighted */, true/* redraw */);
				
				// if this is the first move (that is, the mouse was not simply clicked to delete
				// the previous selection), start a new selection range consisting of the cell
				// underneath the mouse
				if (false == inTerminalViewPtr->text.selection.exists)
				{
					inTerminalViewPtr->text.selection.exists = true;
					originalCellUnderMouse = cellUnderMouse;
					inTerminalViewPtr->text.selection.range.first = cellUnderMouse;
					inTerminalViewPtr->text.selection.range.second = cellUnderMouse;
				}
				else
				{
					// LOCALIZE THIS; implies left-to-right locale
					if (inTerminalViewPtr->text.selection.isRectangular)
					{
						// rectangular selection
						CGRect		r1;
						
						
						if (extendSelection)
						{
							// when extending, remember the section previously highlighted...
							r1 = CGRectMake(inTerminalViewPtr->text.selection.range.first.first,
											inTerminalViewPtr->text.selection.range.first.second,
											inTerminalViewPtr->text.selection.range.second.first -
												inTerminalViewPtr->text.selection.range.first.first,
											inTerminalViewPtr->text.selection.range.second.second -
												inTerminalViewPtr->text.selection.range.first.second);
						}
						inTerminalViewPtr->text.selection.range.first = originalCellUnderMouse;
						inTerminalViewPtr->text.selection.range.second = cellUnderMouse;
						// in rectangular mode, the anchor points might not exactly match
						// one of the mouse anchors (they could be the other two corners),
						// so a rectangular-sort is necessary before adjustments are made
						sortAnchors(inTerminalViewPtr->text.selection.range.first,
									inTerminalViewPtr->text.selection.range.second,
									true/* is rectangular */);
						// end point’s top-left would be past-the-end for the character
						// above and to the left, so offset in both directions to
						// encompass the character underneath the mouse
						++(inTerminalViewPtr->text.selection.range.second.first);
						++(inTerminalViewPtr->text.selection.range.second.second);
						if (extendSelection)
						{
							// the largest encompassing rectangle must be used when extending;
							// this composes both the original selection and the new range,
							// and ensures the overall rectangle is completely highlighted
							CGRect		r2 = CGRectMake(inTerminalViewPtr->text.selection.range.first.first,
														inTerminalViewPtr->text.selection.range.first.second,
														inTerminalViewPtr->text.selection.range.second.first -
															inTerminalViewPtr->text.selection.range.first.first,
														inTerminalViewPtr->text.selection.range.second.second -
															inTerminalViewPtr->text.selection.range.first.second);
							CGRect		r3 = CGRectUnion(r1, r2);
							
							
							inTerminalViewPtr->text.selection.range.first = std::make_pair(r3.origin.x, r3.origin.y);
							inTerminalViewPtr->text.selection.range.second = std::make_pair(r3.origin.x + r3.size.width,
																							r3.origin.y + r3.size.height);
							sortAnchors(inTerminalViewPtr->text.selection.range.first,
										inTerminalViewPtr->text.selection.range.second,
										true/* is rectangular */);
						}
						else
						{
							if (inTerminalViewPtr->text.selection.range.first.first < originalCellUnderMouse.first)
							{
								// selection is moving before the initial anchor point (exclusive)...
								--(inTerminalViewPtr->text.selection.range.second.first);
							}
						}
					}
					else
					{
						// Mac-like continuous selection anchors
						if ((cellUnderMouse.second < originalCellUnderMouse.second) ||
							((cellUnderMouse.second == originalCellUnderMouse.second) &&
								(cellUnderMouse.first < originalCellUnderMouse.first)))
						{
							// selection is moving before the initial anchor point (exclusive)...
							inTerminalViewPtr->text.selection.range.first = cellUnderMouse;
							if (false == extendSelection)
							{
								inTerminalViewPtr->text.selection.range.second = originalCellUnderMouse;
								// ...start point’s top-left would be past-the-end for the line above,
								// so descend by one
								++(inTerminalViewPtr->text.selection.range.second.second);
							}
						}
						else
						{
							// selection is moving after the initial anchor point (inclusive)...
							if (false == extendSelection)
							{
								inTerminalViewPtr->text.selection.range.first = originalCellUnderMouse;
								++(inTerminalViewPtr->text.selection.range.second.first);
							}
							inTerminalViewPtr->text.selection.range.second = cellUnderMouse;
							// end point’s top-left would be past-the-end for the character
							// above and to the left, so offset in both directions to
							// encompass the character underneath the mouse
							++(inTerminalViewPtr->text.selection.range.second.second);
						}
					}
				}
				
				highlightCurrentSelection(inTerminalViewPtr, true/* is highlighted */, true/* redraw */);
				previousLocalMouse = *outNewLocalMousePtr;
			}
			
			// find next mouse location
			{
				OSStatus	error = noErr;
				
				
				error = TrackMouseLocationWithOptions(nullptr/* port, or nullptr for current port */, 0/* options */,
														kEventDurationForever/* timeout */, outNewLocalMousePtr, outNewModifiersPtr,
														&trackingResult);
				if (noErr != error) break;
			}
		}
		while (kMouseTrackingMouseUp != trackingResult);
		
		SetGWorld(oldPort, oldDevice);
		
		inTerminalViewPtr->text.selection.exists = (inTerminalViewPtr->text.selection.range.first !=
													inTerminalViewPtr->text.selection.range.second);
		if (inTerminalViewPtr->text.selection.exists)
		{
			sortAnchors(inTerminalViewPtr->text.selection.range.first, inTerminalViewPtr->text.selection.range.second,
						inTerminalViewPtr->text.selection.isRectangular);
		}
		copySelectedTextIfUserPreference(inTerminalViewPtr);
	}
}// trackTextSelection


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
	if (inTerminalViewPtr->isCocoa)
	{
		[inTerminalViewPtr->backgroundNSView setNeedsDisplay:YES];
		[inTerminalViewPtr->paddingNSView setNeedsDisplay:YES];
		[inTerminalViewPtr->contentNSView setNeedsDisplay:YES];
	}
	else
	{
		UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplay(inTerminalViewPtr->backgroundHIView, true);
		UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplay(inTerminalViewPtr->paddingHIView, true);
		UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplay(inTerminalViewPtr->contentHIView, true);
	}
}// updateDisplay


/*!
Arranges for the specified portion of the terminal screen to be
redrawn at the next opportunity.  The region should be relative
to the main content view.

(4.0)
*/
void
updateDisplayInRegion	(My_TerminalViewPtr		inTerminalViewPtr,
						 RgnHandle				inoutRegion)
{
	// it is potentially slow to call HIViewSetNeedsDisplay() here, so instead
	// an internal region is maintained and refreshed regularly via a timer
	UnionRgn(inTerminalViewPtr->screen.refreshRegion, inoutRegion, inTerminalViewPtr->screen.refreshRegion);
}// updateDisplayInRegion


/*!
A timer that is functionally equivalent to updateDisplay() and
similar routines, except that it passes the internal region to
the system (whereas, all other similar methods merely add to
the region).  The region is then cleared.

This is implemented as a timer because it keeps the potentially
expensive and useless invalidation calls from occurring more
frequently than the system will handle them (which is, at a
refresh rate of once per tick).

(4.0)
*/
void
updateDisplayTimer	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
					 void*					inTerminalViewRef)
{
	TerminalViewRef				ref = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	My_TerminalViewAutoLocker	ptr(gTerminalViewPtrLocks(), ref);
	
	
	if (false == EmptyRgn(ptr->screen.refreshRegion))
	{
		if (ptr->isCocoa)
		{
			Rect	regionBounds;
			NSRect	floatBounds;
			
			
			// invalidate the same screen region in all views
			// (requires translation into each view’s space)
			GetRegionBounds(ptr->screen.refreshRegion, &regionBounds);
			floatBounds = NSMakeRect(regionBounds.left, regionBounds.top,
										regionBounds.right - regionBounds.left,
										regionBounds.bottom - regionBounds.top);
			[ptr->contentNSView setNeedsDisplayInRect:floatBounds];
			
			floatBounds = [ptr->paddingNSView convertRect:floatBounds fromView:ptr->contentNSView];
			[ptr->paddingNSView setNeedsDisplayInRect:floatBounds];
			
			floatBounds = [ptr->backgroundNSView convertRect:floatBounds fromView:ptr->paddingNSView];
			[ptr->backgroundNSView setNeedsDisplayInRect:floatBounds];
		}
		else
		{
			HIViewRef	currentView = ptr->contentHIView;
			
			
			if (IsValidControlHandle(currentView))
			{
				// no need to convert for first view, input region is assumed
				// to already be in its coordinate system
				UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplayInRegion(currentView, ptr->screen.refreshRegion, true);
				{
					My_RegionConverter	contentToPadding(ptr->screen.refreshRegion, currentView, HIViewGetSuperview(currentView), true/* translate back */);
					
					
					UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplayInRegion(contentToPadding.destination, ptr->screen.refreshRegion, true);
					currentView = HIViewGetSuperview(currentView);
					{
						My_RegionConverter	paddingToBackground(ptr->screen.refreshRegion, currentView, HIViewGetSuperview(currentView), true/* translate back */);
						
						
						UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplayInRegion(paddingToBackground.destination, ptr->screen.refreshRegion, true);
					}
				}
			}
		}
		SetEmptyRgn(ptr->screen.refreshRegion);
	}
}// updateDisplayTimer


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
	if (inTerminalViewPtr->isCocoa)
	{
		// Cocoa and Quartz setup
		CGDeviceColor	backgroundDeviceColor;
		CGDeviceColor	foregroundDeviceColor; // not used except for reference when inverting
		
		
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
						NSColor*	selectionTextColor = [[foregroundNSColor copy] autorelease];
						NSColor*	selectionBackgroundColor = [[backgroundNSColor copy] autorelease];
						
						
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
					((false == inTerminalViewPtr->text.selection.inhibited) && inAttributes.hasSelection() && !gApplicationIsSuspended))
			{
				inTerminalViewPtr->screen.currentRenderNoBackground = false;
				
				// dim screen
				foregroundNSColor = [foregroundNSColor colorCloserToWhite];
				backgroundNSColor = [backgroundNSColor colorCloserToWhite];
				
				// for text that is not selected, do an “iPhotoesque” selection where
				// the selection appears darker than its surrounding text; this also
				// causes the selection to look dark (appropriate because it is draggable)
				unless ((!inTerminalViewPtr->text.selection.exists) || (gPreferenceProxies.invertSelections))
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
				NSColor*	searchResultTextColor = [[foregroundNSColor copy] autorelease];
				NSColor*	searchResultBackgroundColor = [[backgroundNSColor copy] autorelease];
				
				
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
												colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
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
				CGDeviceColor	floatRGB;
				
				
				if (inTerminalViewPtr->screen.cursor.isCustomColor)
				{
					// read the user’s preferred cursor color
					getScreenCustomColor(inTerminalViewPtr, kTerminalView_ColorIndexCursorBackground, &floatRGB);
					CGContextSetRGBFillColor(inDrawingContext, floatRGB.red, floatRGB.green, floatRGB.blue, inDesiredAlpha);
				}
			}
		}
	}
	else
	{
		// legacy Carbon and QuickDraw setup (will be removed after Cocoa transition)
		RGBColor		colorRGB;
		CGDeviceColor	backgroundDeviceColor;
		CGDeviceColor	deviceColor;
		Boolean			usingDragHighlightColors = (inTerminalViewPtr->screen.currentRenderDragColors);
		
		
		// IMPORTANT: Drawing code is currently transitioning to Core Graphics;
		// so, not all QuickDraw settings are completely replaced yet, and some
		// are even made redundantly in both contexts.
		
		// find the correct colors in the color table
		getScreenColorsForAttributes(inTerminalViewPtr, inAttributes, &deviceColor, &backgroundDeviceColor,
										&inTerminalViewPtr->screen.currentRenderNoBackground);
		colorRGB = ColorUtilities_QuickDrawColorMake(deviceColor);
		
		// set up foreground color
		if (usingDragHighlightColors)
		{
			// when rendering a drag highlight, use black text
			colorRGB.red = 0;
			colorRGB.green = 0;
			colorRGB.blue = 0;
			RGBForeColor(&colorRGB);
			CGContextSetRGBStrokeColor(inDrawingContext, 0/* red */, 0/* green */, 0/* blue */, 1.0/* alpha */);
			
			// ...and allow background (which will be the drag highlight) to show through
			inTerminalViewPtr->screen.currentRenderNoBackground = true;
		}
		else
		{
			RGBForeColor(&colorRGB);
			// set Core Graphics color below...
		}
		
		// set up background color; note that in drag highlighting mode,
		// the background color is preset by the highlight renderer
		if (false == usingDragHighlightColors)
		{
			RGBColor	backgroundRGB = ColorUtilities_QuickDrawColorMake(backgroundDeviceColor);
			
			
			RGBBackColor(&backgroundRGB);
			
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
						RGBForeColor(&backgroundRGB);
						RGBBackColor(&colorRGB);
					}
					else
					{
						// TEMPORARY: Carbon legacy will go away but for now transition this to
						// use the new Cocoa methods for finding the appropriate colors.
						NSColor*	selectionTextColor = [NSColor colorWithCalibratedRed:deviceColor.red
																							green:deviceColor.green
																							blue:deviceColor.blue
																							alpha:1.0];
						NSColor*	selectionBackgroundColor = [NSColor colorWithCalibratedRed:backgroundDeviceColor.red
																								green:backgroundDeviceColor.green
																								blue:backgroundDeviceColor.blue
																								alpha:1.0];
						
						
						if (NO == [NSColor selectionColorsForForeground:&selectionTextColor
																		background:&selectionBackgroundColor])
						{
							// bail; force the default color even if it won’t look as good
							selectionBackgroundColor = [NSColor selectedTextBackgroundColor];
						}
						
						// convert to RGB and then update the context
						[selectionTextColor setAsForegroundInQDCurrentPort];
						[selectionBackgroundColor setAsBackgroundInQDCurrentPort];
					}
				}
			}
			
			// when the screen is inactive, it is dimmed; in addition, any text that is
			// not selected is FURTHER dimmed so as to emphasize the selection and show
			// that the selection is in fact a click-through region
			unless ((inTerminalViewPtr->isActive) || (gPreferenceProxies.dontDimTerminals) ||
					((false == inTerminalViewPtr->text.selection.inhibited) && inAttributes.hasSelection() && !gApplicationIsSuspended))
			{
				inTerminalViewPtr->screen.currentRenderNoBackground = false;
				
				// dim screen
				UseInactiveColors();
				
				// for text that is not selected, do an “iPhotoesque” selection where
				// the selection appears darker than its surrounding text; this also
				// causes the selection to look dark (appropriate because it is draggable)
				unless ((!inTerminalViewPtr->text.selection.exists) || (gPreferenceProxies.invertSelections))
				{
					UseLighterColors();
				}
			}
			
			// give search results a special appearance
			if (inAttributes.hasSearchHighlight())
			{
				// TEMPORARY: Carbon legacy will go away but for now transition this to
				// use the new Cocoa methods for finding the appropriate colors.
				NSColor*	searchResultTextColor = [NSColor colorWithCalibratedRed:deviceColor.red
																					green:deviceColor.green
																					blue:deviceColor.blue
																					alpha:1.0];
				NSColor*	searchResultBackgroundColor = [NSColor colorWithCalibratedRed:backgroundDeviceColor.red
																							green:backgroundDeviceColor.green
																							blue:backgroundDeviceColor.blue
																							alpha:1.0];
				
				
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
				
				// convert to RGB and then update the context
				[searchResultTextColor setAsForegroundInQDCurrentPort];
				[searchResultBackgroundColor setAsBackgroundInQDCurrentPort];
				
				inTerminalViewPtr->screen.currentRenderNoBackground = false;
			}
			
			// finally, check the foreground and background colors; do not allow
			// them to be identical unless “concealed” is the style (e.g. perhaps
			// text is ANSI white and the background is white; that's invisible!)
			unless (inAttributes.hasConceal())
			{
				RGBColor	comparedColor;
				
				
				GetForeColor(&colorRGB);
				GetBackColor(&comparedColor);
				if ((colorRGB.red == comparedColor.red) &&
					(colorRGB.green == comparedColor.green) &&
					(colorRGB.blue == comparedColor.blue))
				{
					// arbitrarily make the text black to resolve the contrast issue,
					// unless the color *is* black, in which case choose white
					// TEMPORARY: this could be some kind of user preference...
					if ((comparedColor.red != 0) ||
						(comparedColor.green != 0) ||
						(comparedColor.blue != 0))
					{
						colorRGB.red = 0;
						colorRGB.green = 0;
						colorRGB.blue = 0;
					}
					else
					{
						colorRGB.red = RGBCOLOR_INTENSITY_MAX;
						colorRGB.green = RGBCOLOR_INTENSITY_MAX;
						colorRGB.blue = RGBCOLOR_INTENSITY_MAX;
					}
					RGBForeColor(&colorRGB);
				}
			}
			
			// take advantage of the color blending done in the QuickDraw
			// port for now, and simply “steal” the colors for the CG context
			{
				CGDeviceColor	floatRGB;
				Boolean			useScreenBackColor = true;
				
				
				GetForeColor(&colorRGB);
				floatRGB = ColorUtilities_CGDeviceColorMake(colorRGB);
				CGContextSetRGBStrokeColor(inDrawingContext, floatRGB.red, floatRGB.green, floatRGB.blue, inDesiredAlpha);
				
				if (inIsCursor)
				{
					if (inTerminalViewPtr->screen.cursor.isCustomColor)
					{
						// read the user’s preferred cursor color
						getScreenCustomColor(inTerminalViewPtr, kTerminalView_ColorIndexCursorBackground, &floatRGB);
						useScreenBackColor = false;
					}
					else
					{
						// base the cursor color on the screen (the cursor-drawing
						// code uses inverse video so the current background is
						// actually a foreground color)
						useScreenBackColor = true;
					}
				}
				
				if (useScreenBackColor)
				{
					GetBackColor(&colorRGB);
					floatRGB = ColorUtilities_CGDeviceColorMake(colorRGB);
				}
				
				CGContextSetRGBFillColor(inDrawingContext, floatRGB.red, floatRGB.green, floatRGB.blue, inDesiredAlpha);
			}
		}
	}
}// useTerminalTextColors


/*!
LEGACY.  Used for Carbon views only.  See also the routine
setTextAttributesDictionary(), which returns keys and values
suitable for use in attributed strings.

Sets the screen variable for the current text attributes to be
the specified attributes, and sets QuickDraw and Core Graphics
settings appropriately for the requested attributes (text font,
size, style, etc.).  Also updates internal flags for the current
render to indicate whether or not to use double-size text and
VT graphics.

This does NOT set colors; see useTerminalTextColors().

IMPORTANT:	This is ONLY for use by drawTerminalText(), in
			order to set up the drawing port properly.  Note
			that certain modes (such as VT graphics and
			double-size text) are communicated to
			drawTerminalText() through special font sizes;
			so calling this routine does NOT necessarily leave
			the drawing port in a state where normal calls
			could render terminal text as expected.  Use only
			the drawTerminalText() method for rendering text
			and graphics, that’s what it’s there for!

IMPORTANT:	Core Graphics support is INCOMPLETE.  This routine
			may not completely update the context with all
			necessary parameters.

(3.1)
*/
void
useTerminalTextAttributes	(My_TerminalViewPtr			inTerminalViewPtr,
							 CGContextRef				inDrawingContext,
							 TextAttributes_Object		inAttributes)
{
	if ((inTerminalViewPtr->text.attributes == kTextAttributes_Invalid) ||
		(inTerminalViewPtr->text.attributes != inAttributes))
	{
		Float32		fontSize = 0.0;
		
		
		// set text size, examining “double size” bits
		// NOTE: there’s no real reason this should have to be checked
		//       here, EVERY time text is rendered; it could eventually
		//       be set somewhere else, e.g. whenever the attribute bits
		//       or font size change, a rendering size could be updated
		if (inAttributes.hasDoubleHeightTop())
		{
			// top half - do not render
			fontSize = kArbitraryDoubleWidthDoubleHeightPseudoFontSize;
		}
		else if (inAttributes.hasDoubleHeightBottom())
		{
			// bottom half - double size
			fontSize = inTerminalViewPtr->text.font.doubleMetrics.size;
		}
		else
		{
			// normal size
			fontSize = inTerminalViewPtr->text.font.normalMetrics.size;
		}
		
		inTerminalViewPtr->text.attributes = inAttributes;
		
		if (inTerminalViewPtr->isCocoa)
		{
			// Cocoa and Quartz setup; note that the font and size
			// are retrieved from the stored NSFont object and not
			// set on the drawing context or any other temporary object
			
			// set font style - UNIMPLEMENTED
			
			// set pen - UNIMPLEMENTED
			
			// for a sufficiently large font, allow boldface - UNIMPLEMENTED
			
			// for a sufficiently large font, allow italicizing - UNIMPLEMENTED
			
			// for a sufficiently large font, allow underlining - UNIMPLEMENTED
			
			// set text mode and pen mode - UNIMPLEMENTED
		}
		else
		{
			// legacy Carbon and QuickDraw setup (will be removed after Cocoa transition)
			CGrafPtr	currentPort = nullptr;
			GDHandle	currentDevice = nullptr;
			
			
			GetGWorld(&currentPort, &currentDevice);
			
			// When special glyphs should be rendered, set the font to
			// a specific ID that tells the renderer to do graphics.
			// The font is not actually used, it is just a marker so
			// that the renderer knows it is in graphics mode.  In most
			// cases, the terminal’s regular font is used instead.
			if (inAttributes.hasAttributes(kTextAttributes_VTGraphics))
			{
				TextFont(kArbitraryVTGraphicsPseudoFontID);
			}
			else
			{
				TextFontByName(inTerminalViewPtr->text.font.familyName);
			}
			
			// set text size...
			TextSize(STATIC_CAST(fontSize, SInt16));
			
			// set font style...
			TextFace(normal);
			
			// set pen...
			PenNormal();
			// if the text is really big, drawn lines should be bigger as well, so
			// multiply 1 pixel by a scaling factor based on the size of the text
			PenSize(STATIC_CAST(inTerminalViewPtr->text.font.thicknessVerticalLines, SInt16),
					STATIC_CAST(inTerminalViewPtr->text.font.thicknessHorizontalLines, SInt16));
			
			// 3.0 - for a sufficiently large font, allow boldface
			if (inAttributes.hasBold() || inAttributes.hasSearchHighlight())
			{
				Style	fontFace = GetPortTextFace(currentPort);
				
				
				if (fontSize > 8) // arbitrary (should this be a user preference?)
				{
					TextFace(fontFace | bold);
					
					// VT graphics use the pen size when rendering, so that should be “bolded” as well
					PenSize(STATIC_CAST(inTerminalViewPtr->text.font.thicknessVerticalBold, SInt16),
							STATIC_CAST(inTerminalViewPtr->text.font.thicknessHorizontalBold, SInt16));
				}
			}
			
			// 3.0 - for a sufficiently large font, allow italicizing
			if (inAttributes.hasItalic())
			{
				Style	fontFace = GetPortTextFace(currentPort);
				
				
				if (fontSize > 12/* arbitrary (should this be a user preference?) */)
				{
					TextFace(fontFace | italic);
				}
				else if (fontSize > 8)
				{
					// render as underline at small font sizes
					TextFace(fontFace | underline);
				}
			}
			
			// 3.0 - for a sufficiently large font, allow underlining
			if (inAttributes.hasUnderline() || inAttributes.hasSearchHighlight())
			{
				Style	fontFace = GetPortTextFace(currentPort);
				
				
				if (fontSize > 8/* arbitrary (should this be a user preference?) */)
				{
					TextFace(fontFace | underline);
				}
			}
			
			// set text mode and pen mode
			TextMode(srcOr); // do not render background color
			PenMode(patCopy);
		}
	}
}// useTerminalTextAttributes


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
visualBell	(TerminalViewRef	inView)
{
	My_TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIWindowRef const			kViewWindow = HIViewGetWindow(viewPtr->contentHIView);
	Boolean const				kWasReverseVideo = viewPtr->screen.isReverseVideo;
	Boolean						visual = false;				// is visual used?
	Boolean						visualPreference = false;	// is ONLY visual used?
	
	
	// If the user turned off audible bells, always use a visual;
	// otherwise, use a visual if the beep is in a background window.
	unless (kPreferences_ResultOK ==
			Preferences_GetData(kPreferences_TagVisualBell, sizeof(visualPreference),
								&visualPreference))
	{
		visualPreference = false; // assume audible bell, if preference can’t be found
	}
	visual = (visualPreference || (!IsWindowHilited(kViewWindow)));
	
	if (visual)
	{
		TerminalView_ReverseVideo(inView, !kWasReverseVideo); // also invalidates view
		HIWindowFlush(kViewWindow);
	}
	
	// Mac OS 8 asynchronous sounds mean that a sound generates
	// very little delay, therefore a standard visual delay of 8
	// ticks should be enforced even if a beep was emitted.
	{
		UInt32		dummy = 0L;
		
		
		Delay(8/* ticks */, &dummy);
	}
	
	// if previously inverted, invert again to restore to normal
	if (visual)
	{
		TerminalView_ReverseVideo(inView, kWasReverseVideo); // also invalidates view
		HIWindowFlush(kViewWindow);
	}
	
	if (gPreferenceProxies.notifyOfBeeps) Alert_BackgroundNotification();
}// visualBell

} // anonymous namespace


#pragma mark -
@implementation TerminalView_BackgroundView


@synthesize colorIndex = _colorIndex;


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
		self->_colorIndex = kMyBasicColorIndexNormalBackground;
		self->_internalViewPtr = nullptr;
		
		self.wantsLayer = YES;
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
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (My_TerminalViewPtr)
internalViewPtr
{
	return REINTERPRET_CAST(_internalViewPtr, My_TerminalViewPtr);
}
- (void)
setInternalViewPtr:(My_TerminalViewPtr)		aViewPtr
{
	_internalViewPtr = aViewPtr;
}// setInternalViewPtr:


@end // TerminalView_BackgroundView


#pragma mark -
@implementation TerminalView_BackgroundView (TerminalView_BackgroundViewInternal)


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
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	NSGraphicsContext*		contextMgr = [NSGraphicsContext currentContext];
	CGContextRef			drawingContext = REINTERPRET_CAST([contextMgr graphicsPort], CGContextRef);
	CGRect					clipBounds = CGContextGetClipBoundingBox(drawingContext);
	
	
	// NOTE: For porting purposes the colored background is simply drawn.
	// Another option would be to update the layer’s background color
	// directly whenever the view color changes.
	if (nullptr != viewPtr)
	{
		CGDeviceColor const&	asFloats = viewPtr->text.colors[self.colorIndex];
		
		
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


@end // TerminalView_BackgroundView (TerminalView_BackgroundViewInternal)


#pragma mark -
@implementation TerminalView_ContentView


@synthesize modifierFlagsForCursor = _modifierFlagsForCursor;
@synthesize showDragHighlight = _showDragHighlight;


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
		self->_showDragHighlight = NO;
		self->_modifierFlagsForCursor = 0;
		self->_internalViewPtr = nullptr;
		
		self.wantsLayer = YES;
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
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (My_TerminalViewPtr)
internalViewPtr
{
	return REINTERPRET_CAST(_internalViewPtr, My_TerminalViewPtr);
}
- (void)
setInternalViewPtr:(My_TerminalViewPtr)		aViewPtr
{
	_internalViewPtr = aViewPtr;
}// setInternalViewPtr:


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


#pragma mark Actions


/*!
Uses the name of the given sender (expected to be a menu item) to
find a Format set of Preferences from which to copy new font and
color information.

WARNING:	For the transition period, this can only be called
			by Commands_Executor, and not by the responder chain.

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

WARNING:	For the transition period, this can only be called
			by Commands_Executor, and not by the responder chain.

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


#pragma mark NSResponder


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


@end // TerminalView_ContentView


#pragma mark -
@implementation TerminalView_ContentView (TerminalView_ContentViewInternal)


#pragma mark NSView


/*!
It is necessary for terminal views to accept “first responder”
in order to ever receive actions such as menu commands!

(4.0)
*/
- (BOOL)
acceptsFirstResponder
{
	return YES;
}// acceptsFirstResponder


/*!
Render the specified part of the terminal screen.

INCOMPLETE.  This is going to be the test bed for transitioning
away from Carbon.  And given that the Carbon view is heavily
dependent on older technologies, it will be awhile before the
Cocoa version is exposed to users.  (The likely debut will be in
something like a formatting sheet’s preview pane.)

(4.0)
*/
- (void)
drawRect:(NSRect)	aRect
{
#pragma unused(aRect)
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	if (nullptr != viewPtr)
	{
		NSGraphicsContext*	contextMgr = [NSGraphicsContext currentContext];
		CGContextRef		drawingContext = REINTERPRET_CAST([contextMgr graphicsPort], CGContextRef);
		CGRect				entireRect = NSRectToCGRect([self bounds]);
		CGRect				clipBounds = CGContextGetClipBoundingBox(drawingContext);
		CGRect				contentBounds = CGRectMake(CGRectGetMinX(entireRect), CGRectGetMinY(entireRect),
														CGRectGetWidth(entireRect),
														CGRectGetHeight(entireRect));
		
		
		// INCOMPLETE!
		
		// perform any necessary rendering for drags
		{
			if (self.showDragHighlight)
			{
				DragAndDrop_ShowHighlightBackground(drawingContext, contentBounds);
				// frame is drawn at the end, after any content
			}
			else
			{
				// hide is not necessary because the NSView model ensures the
				// backdrop behind this view is painted as needed
			}
			
			// tell text routines to draw in black if there is a drag highlight
			viewPtr->screen.currentRenderDragColors = (self.showDragHighlight) ? true : false;
		}
		
		// draw text and graphics
		{
			// draw only the requested area; convert from pixels to screen cells
			HIPoint const&		kTopLeftAnchor = clipBounds.origin;
			HIPoint const		kBottomRightAnchor = CGPointMake(clipBounds.origin.y + clipBounds.size.height,
																	clipBounds.origin.x + clipBounds.size.width);
			TerminalView_Cell	leftTopCell;
			TerminalView_Cell	rightBottomCell;
			Float32				deltaColumn = 0.0;
			Float32				deltaRow = 0.0;
			
			
			// figure out what cells to draw
			UNUSED_RETURN(Boolean)findVirtualCellFromScreenPoint(viewPtr, kTopLeftAnchor, leftTopCell, deltaColumn, deltaRow);
			UNUSED_RETURN(Boolean)findVirtualCellFromScreenPoint(viewPtr, kBottomRightAnchor, rightBottomCell, deltaColumn, deltaRow);
			
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
		if ((false == viewPtr->isActive) && viewPtr->text.selection.exists)
		{
			HIShapeRef		selectionShape = getSelectedTextAsNewHIShape(viewPtr, 1.0/* inset */);
			
			
			if (nullptr != selectionShape)
			{
				RGBColor		highlightColorRGB;
				CGDeviceColor	highlightColorDevice;
				OSStatus		error = noErr;
				
				
				// TEMPORARY - account for QuickDraw/Quartz differences until conversion is complete
				{
					HIMutableShapeRef		offsetCopy = HIShapeCreateMutableCopy(selectionShape);
					
					
					if (nullptr != offsetCopy)
					{
						HIShapeOffset(offsetCopy, -1.0, -1.0);
						
						CFRelease(selectionShape), selectionShape = nullptr;
						selectionShape = offsetCopy;
					}
				}
				
				// draw outline
				CGContextSetLineWidth(drawingContext, 2.0); // make thick outline frame on Mac OS X
				CGContextSetLineCap(drawingContext, kCGLineCapRound);
				LMGetHiliteRGB(&highlightColorRGB);
				highlightColorDevice = ColorUtilities_CGDeviceColorMake(highlightColorRGB);
				CGContextSetRGBStrokeColor(drawingContext, highlightColorDevice.red, highlightColorDevice.green,
											highlightColorDevice.blue, 1.0/* alpha */);
				error = HIShapeReplacePathInCGContext(selectionShape, drawingContext);
				assert_noerr(error);
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
			Rect			dummyBounds;
			RGBColor		highlightColorRGB;
			CGDeviceColor	highlightColorDevice;
			
			
			getRowSectionBounds(viewPtr, 0/* row; does not matter which */,
								gPreferenceProxies.renderMarginAtColumn/* zero-based column number, but following column is desired */,
								1/* character count - not important */, &dummyBounds);
			CGContextSetLineWidth(drawingContext, 2.0);
			CGContextSetLineCap(drawingContext, kCGLineCapButt);
			LMGetHiliteRGB(&highlightColorRGB);
			highlightColorDevice = ColorUtilities_CGDeviceColorMake(highlightColorRGB);
			CGContextSetRGBStrokeColor(drawingContext, highlightColorDevice.red, highlightColorDevice.green,
										highlightColorDevice.blue, 1.0/* alpha */);
			CGContextBeginPath(drawingContext);
			CGContextMoveToPoint(drawingContext, dummyBounds.left, contentBounds.origin.y);
			CGContextAddLineToPoint(drawingContext, dummyBounds.left, contentBounds.origin.y + contentBounds.size.height);
			CGContextStrokePath(drawingContext);
		}
		
		// draw cursor
		if (kMyCursorStateVisible == viewPtr->screen.cursor.currentState)
		{
			CGContextSaveRestore		_(drawingContext);
			TextAttributes_Object		cursorAttributes = Terminal_CursorReturnAttributes(viewPtr->screen.ref);
			CGRect						cursorFloatBounds = CGRectMake(viewPtr->screen.cursor.bounds.left,
																		viewPtr->screen.cursor.bounds.top,
																		viewPtr->screen.cursor.bounds.right -
																			viewPtr->screen.cursor.bounds.left
																			- 2/* TEMPORARY Quartz/QD conversion */,
																		viewPtr->screen.cursor.bounds.bottom -
																			viewPtr->screen.cursor.bounds.top
																			- 2/* TEMPORARY Quartz/QD conversion */);
			
			
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
			CGContextFillRect(drawingContext, cursorFloatBounds);
			
			// if the terminal is currently in password mode, annotate the cursor
			if (Terminal_IsInPasswordMode(viewPtr->screen.ref))
			{
				CGFloat const	newHeight = viewPtr->text.font.heightPerCharacter; // TEMPORARY; does not handle double-height lines (probably does not need to)
				CGFloat			dotDimensions = STATIC_CAST(ceil(viewPtr->text.font.heightPerCharacter * 0.33/* arbitrary */), CGFloat);
				CGRect			fullRectangleBounds = cursorFloatBounds;
				CGRect			dotBounds = CGRectMake(0, 0, dotDimensions, dotDimensions); // arbitrary
				
				
				// the cursor may not be block-size so convert to a block rectangle
				// before determining the placement of the oval inset
				fullRectangleBounds.origin.y = fullRectangleBounds.origin.y + fullRectangleBounds.size.height - newHeight;
				fullRectangleBounds.size.width = viewPtr->text.font.widthPerCharacter;
				fullRectangleBounds.size.height = newHeight;
				RegionUtilities_CenterHIRectIn(dotBounds, fullRectangleBounds);
				++dotBounds.origin.x; // TEMPORARY: figure out why this correction seems necessary
				
				// draw the dot in the middle of the cell that the cursor occupies
				if (kTerminalView_CursorTypeBlock == gPreferenceProxies.cursorType)
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
	}
}// drawRect:


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
Invoked by NSView whenever it’s necessary to define the regions
that change the mouse pointer’s shape.

(4.1)
*/
- (void)
resetCursorRects
{
	My_TerminalViewPtr		viewPtr = self.internalViewPtr;
	
	
	if ((nullptr != viewPtr) && (viewPtr->text.selection.readOnly))
	{
		// the user cannot interact with the terminal view so it is
		// inappropriate to display any special cursor shapes over it
		[self addCursorRect:[self bounds] cursor:[NSCursor arrowCursor]];
	}
	else
	{
		// the cursor varies based on the state of modifier keys
		if (self.modifierFlagsForCursor & NSControlKeyMask)
		{
			// modifier key for contextual menu
			[self addCursorRect:[self bounds] cursor:[NSCursor contextualMenuCursor]];
		}
		else if ((self.modifierFlagsForCursor & NSCommandKeyMask) &&
					(self.modifierFlagsForCursor & NSAlternateKeyMask))
		{
			// modifier key for moving the terminal cursor to the click location
			[self addCursorRect:[self bounds] cursor:customCursorMoveTerminalCursor(isSmallIBeam(viewPtr))];
		}
		else if (self.modifierFlagsForCursor & NSCommandKeyMask)
		{
			// modifier key for clicking a URL selection
			[self addCursorRect:[self bounds] cursor:[NSCursor pointingHandCursor]];
		}
		else if (self.modifierFlagsForCursor & NSAlternateKeyMask)
		{
			// modifier key for rectangular text selections
			[self addCursorRect:[self bounds] cursor:customCursorCrosshairs()];
		}
		else
		{
			// normal cursor
			//[self addCursorRect:[self bounds] cursor:[NSCursor IBeamCursor]];
			[self addCursorRect:[self bounds] cursor:customCursorIBeam(isSmallIBeam(viewPtr))];
		}
		
		// INCOMPLETE; add support for any current text selection region
	}
}// resetCursorRects


@end // TerminalView_ContentView (TerminalView_ContentViewInternal)


#pragma mark -
@implementation TerminalView_Controller //{


/*!
The background view sits behind everything else in the terminal
and renders the matte color.  (In the future, this might also
render patterns or other visual effects.)
*/
@synthesize terminalBackgroundView = _terminalBackgroundView;

/*!
The content view sits in front of everything else in the terminal
and renders text and graphics.
*/
@synthesize terminalContentView = _terminalContentView;

/*!
The padding view sits between the content view and the background
and renders the background color.  (In the future, this might also
render pictures or other visual effects.)
*/
@synthesize terminalPaddingView = _terminalPaddingView;


/*!
Designated initializer.

(2016.03)
*/
- (instancetype)
init
{
	self = [super initWithNibName:@"TerminalViewCocoa" bundle:nil];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Destructor.

(2016.03)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark NSViewController


/*!
Invoked by NSViewController once the "self.view" property is set,
after the NIB file is loaded.  This essentially guarantees that
all file-defined user interface elements are now instantiated and
other settings that depend on valid UI objects can now be made.

NOTE:	As future SDKs are adopted, it makes more sense to only
		implement "viewDidLoad" (which was only recently added
		to NSViewController and is not otherwise available).
		This implementation can essentially move to "viewDidLoad".

(2016.03)
*/
- (void)
loadView
{
	[super loadView];
	assert(nil != self.terminalContentView);
	assert(nil != self.terminalPaddingView);
	assert(nil != self.terminalBackgroundView);
	
	Preferences_ContextWrap		terminalConfig(Preferences_NewContext(Quills::Prefs::TERMINAL),
												Preferences_ContextWrap::kAlreadyRetained);
	Preferences_ContextWrap		translationConfig(Preferences_NewContext(Quills::Prefs::TRANSLATION),
													Preferences_ContextWrap::kAlreadyRetained);
	
	
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
				TerminalViewRef		viewRef = TerminalView_NewNSViewBased(self.terminalContentView, self.terminalPaddingView,
																			self.terminalBackgroundView, buffer, nullptr/* format */);
				
				
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
}// loadView


@end //} TerminalView_Controller

// BELOW IS REQUIRED NEWLINE TO END FILE
