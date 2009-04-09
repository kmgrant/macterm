/*###############################################################

	TerminalView.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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

#define BLINK_MEANS_BLINK	1

#include "UniversalDefines.h"

// standard-C includes
#include <algorithm>
#include <cctype>
#include <set>
#include <vector>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CGContextSaveRestore.h>
#include <ColorUtilities.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <Cursors.h>
#include <FileSelectionDialogs.h>
#include <HIViewWrap.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// resource includes
#include "GeneralResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Clipboard.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "ContextualMenuBuilder.h"
#include "DialogUtilities.h"
#include "DragAndDrop.h"
#include "EventLoop.h"
#include "FileUtilities.h"
#include "GenericThreads.h"
#include "NetEvents.h"
#include "Preferences.h"
#include "SessionFactory.h"
#include "Terminal.h"
#include "TerminalBackground.h"
#include "TerminalView.h"
#include "TextTranslation.h"
#include "UIStrings.h"
#include "URL.h"
#include "VectorInterpreter.h"



#pragma mark Constants

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
This delta value is used to shift the alpha used for
cursor rendering, and for simplicity it is defined in
terms of the existing blink timer.
*/
Float32 const		kTerminalView_BlinkAlphaDelta = 1.0 / STATIC_CAST(kMy_BlinkingColorCount, Float32);

/*!
Indices into the "customColors" array of the main structure.
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

#pragma mark Types

struct MyPreferenceProxies
{
	Boolean		cursorBlinks;
	Boolean		dontDimTerminals;
	Boolean		invertSelections;
	Boolean		notifyOfBeeps;
};

typedef std::vector< TerminalView_CellRange >	My_CellRangeList;
typedef std::vector< CGDeviceColor >			My_CGColorList;
typedef std::map< UInt16, CGDeviceColor >		My_CGColorByIndex; // a map is necessary because "vector" cannot handle 256 sequential color structures
typedef std::vector< EventTime >				My_TimeIntervalList;

/*!
Calculates mappings between XTerm encoded color values and
the roughly 256 equivalent RGB triplets or gray scales.
*/
class My_XTerm256Table
{
public:
	typedef std::vector< SInt16 >			RGBLevels;
	typedef std::map< UInt8, RGBLevels >	RGBLevelsByIndex;
	typedef std::map< UInt8, SInt16 >		GrayLevelByIndex;
	
	My_XTerm256Table ();
	
	void
	makeRGBColor	(UInt8, UInt8, UInt8, RGBColor&);
	
	GrayLevelByIndex	grayLevels;
	RGBLevelsByIndex	colorLevels;
};

// TEMPORARY: This structure is transitioning to C++, and so initialization
// and maintenance of it is downright ugly for the time being.  It *will*
// be simplified and become more object-oriented in the future.
struct TerminalView
{
	TerminalView	(HIViewRef);
	~TerminalView	();
	
	void
	initialize		(TerminalScreenRef, Preferences_ContextRef);
	
	Preferences_ContextRef	encodingConfig;		// various settings from an external source; not kept up to date, see TerminalView_ReturnTranslationConfiguration()
	Preferences_ContextRef	formatConfig;		// various settings from an external source; not kept up to date, see TerminalView_ReturnFormatConfiguration()
	std::set< Preferences_Tag >		configFilter;	// settings that this view ignores when they are changed globally by the user
	ListenerModel_Ref	changeListenerModel;	// listeners for various types of changes to this data
	TerminalView_DisplayMode	displayMode;	// how the content fills the display area
	Boolean				isActive;				// true if the HIView is in an active state, false otherwise; kept in sync
												// using Carbon Events, stored here to avoid extra system calls in cases
												// where this would be slow (e.g. drawing)
	
	CFRetainRelease		accessibilityObject;	// AXUIElementRef; makes terminal views compatible with accessibility technologies
	HIViewRef			encompassingHIView;		// contains all other HIViews but is otherwise invisible
	HIViewRef			backgroundHIView;		// view that renders the background of the terminal screen (border included)
	HIViewRef			focusAndPaddingHIView;	// view that renders the focus ring and is outset by the padding amount from the content view
	HIViewRef			contentHIView;			// view that renders the text of the terminal screen
	
	HIViewPartCode		currentContentFocus;	// used in the content view focus handler to determine where (if anywhere) a ring goes
	
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
			Float32					blinkAlphaDelta;	// +/-kTerminalView_BlinkAlphaDelta, current direction of change
		} cursor;
	} animation;
	
	My_CGColorByIndex	coreColors;		// the (up to) 256 standard colors used, e.g. in color XTerms; base 16 are ANSI colors
	My_CGColorList		customColors;	// the colors currently used to render based on purpose, e.g. normal, bold, blinking, matte
	My_CGColorList		blinkColors;	// an automatically generated set of intermediate colors for blink animation
	
	struct
	{
		TerminalScreenRef			ref;					// where the data for this terminal view comes from
		Boolean						areANSIColorsEnabled;	// are ANSI-colored text and graphics allowed in this window?
		Boolean						focusRingEnabled;		// is the matte and content area focus ring displayed?
		Boolean						isReverseVideo;			// are foreground and background colors temporarily swapped?
		
		PicHandle					backgroundPicture;		// custom terminal screen background (if any)
	#if 0
		ListenerModel_ListenerRef	backgroundDragHandler;	// listener for clicks in screens from inactive windows
	#endif
		ListenerModel_ListenerRef	contentMonitor;			// listener for changes to the contents of the screen buffer
		ListenerModel_ListenerRef	cursorMonitor;			// listener for changes to the terminal cursor position or visible state
		ListenerModel_ListenerRef	preferenceMonitor;		// listener for changes to preferences that affect a particular view
		ListenerModel_ListenerRef	bellHandler;			// listener for bell signals from the terminal screen
		ListenerModel_ListenerRef	videoModeMonitor;		// listener for changes to the reverse-video setting
		
		struct
		{
			Rect				bounds;			// the rectangle of the cursor’s visible region, relative to the content pane!!!
			Rect				ghostBounds;	// the exact view-relative rectangle of a dragged cursor’s outline region
			MyCursorState		currentState;	// whether the cursor is visible
			MyCursorState		ghostState;		// whether the cursor ghost is visible
			Boolean				idleFlash;		// is timer currently animating anything?
		} cursor;
		
		SInt16			topEdgeInPixels;			// the most negative possible value for the top visible edge in pixels
		SInt16			topVisibleEdgeInPixels;		// 0 if scrolled to the main screen, negative if scrollback; do not change this
													//   value directly, use offsetTopVisibleEdge()
		SInt16			topVisibleEdgeInRows;		// 0 if scrolled to the main screen, negative if scrollback; do not change this
													//   value directly, use offsetTopVisibleEdge()
		UInt16			leftVisibleEdgeInPixels;	// 0 if leftmost column is visible, positive if content is scrolled to the left;
													//   do not change this value directly, use offsetLeftVisibleEdge()
		UInt16			leftVisibleEdgeInColumns;	// 0 if leftmost column is visible, positive if content is scrolled to the left;
													//   do not change this value directly, use offsetLeftVisibleEdge()
		SInt16			currentRenderedLine;		// only defined while drawing; the row that is currently being drawn
		Boolean			currentRenderBlinking;		// only defined while drawing; if true, at least one section is blinking
		Boolean			currentRenderDragColors;	// only defined while drawing; if true, drag highlight text colors are used
		Boolean			currentRenderNoBackground;	// only defined while drawing; if true, text is using the ordinary background color
		CGContextRef	currentRenderContext;		// only defined while drawing; if not nullptr, the context from the view draw event
		SInt16			viewWidthInPixels;			// size of window view (window could be smaller than the screen size);
		SInt16			viewHeightInPixels;			//   always identical to the current dimensions of the content view
		SInt16			maxViewWidthInPixels;		// size of bottommost screenful (regardless of window size);
		SInt16			maxViewHeightInPixels;		//   always identical to the maximum dimensions of the content view
		SInt16			maxWidthInPixels;			// the largest size the terminal view content area can possibly have, which
		SInt16			maxHeightInPixels;			//   would display every last line of the visible screen *and* scrollback!
		Float32			paddingLeftEmScale;			// left padding between text and focus ring; multiplies against normal (undoubled) character width
		Float32			paddingRightEmScale;		// right padding between text and focus ring; multiplies against normal (undoubled) character width
		Float32			paddingTopEmScale;			// top padding between text and focus ring; multiplies against normal (undoubled) character height
		Float32			paddingBottomEmScale;		// bottom padding between text and focus ring; multiplies against normal (undoubled) character height
		Float32			marginLeftEmScale;			// left margin between focus ring and container edge; multiplies against normal (undoubled) character width
		Float32			marginRightEmScale;			// right margin between focus ring and container edge; multiplies against normal (undoubled) character width
		Float32			marginTopEmScale;			// top margin between focus ring and container edge; multiplies against normal (undoubled) character height
		Float32			marginBottomEmScale;		// bottom margin between focus ring and container edge; multiplies against normal (undoubled) character height
	} screen;
	
	struct
	{
		TerminalTextAttributes		attributes;		// current text attribute flags, affecting color of terminal text, etc.
		
		struct
		{
			Boolean				isMonospaced;	// whether every character in the font is the same width (expected to be true)
			Str255				familyName;		// font name (as might appear in a Font menu)
			CFStringEncoding	encoding;		// encoding actually used by font, which may be different than what the terminal uses!
			struct Metrics
			{
				SInt16		ascent;			// number of pixels highest character extends above the base line
				SInt16		size;			// point size of text written in the font indicated by "familyName"
			} normalMetrics,	// metrics for text meant to fit in a single cell (normal)
			  doubleMetrics;	// metrics for text meant to fit in 4 cells, not 1 cell (double-width/height)
			Float32			scaleWidthPerCharacter;	// a multiplier (normally 1.0) to force characters from the font to fit in a different width
			SInt16			widthPerCharacter;	// number of pixels wide each character is (multiply by 2 on double-width lines);
												// generally, you should call getRowCharacterWidth() instead of referencing this!
			SInt16			heightPerCharacter;	// number of pixels high each character is (multiply by 2 if double-height text)
		} font;
		
		RGBColor	colors[kMyBasicColorCount];	// indices are "kMyBasicColorIndexNormalText", etc.
		
		struct
		{
			TerminalView_CellRange		range;			// region of text selection
			My_SelectionMode			keyboardMode;	// used for keyboard navigation; determines what is changed by keyboard-select actions
			Boolean						exists;			// is any text highlighted anywhere in the window?
			Boolean						isRectangular;	// is the text selection unattached from the left and right screen edges?
		} selection;
		
		My_CellRangeList			searchResults;			// regions matching the most recent Find results
		My_CellRangeList::iterator	toCurrentSearchResult;	// most recently focused match; MUST change if "searchResults" changes
	} text;
	
	TerminalViewRef		selfRef;				// redundant opaque reference that would resolve to point to this structure
};
typedef TerminalView*			TerminalViewPtr;
typedef TerminalView const*		TerminalViewConstPtr;

typedef MemoryBlockPtrLocker< TerminalViewRef, TerminalView >	TerminalViewPtrLocker;
typedef LockAcquireRelease< TerminalViewRef, TerminalView >		TerminalViewAutoLocker;

#pragma mark Internal Method Prototypes

static pascal void		animateBlinkingItems			(EventLoopTimerRef, void*);
static void				audioEvent						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static EventTime		calculateAnimationStageDelay	(TerminalViewPtr, My_TimeIntervalList::size_type);
static void				calculateDoubleSize				(TerminalViewPtr, SInt16&, SInt16&);
static UInt16			copyColorPreferences			(TerminalViewPtr, Preferences_ContextRef, Boolean);
static UInt16			copyFontPreferences				(TerminalViewPtr, Preferences_ContextRef, Boolean);
static void				copySelectedTextIfUserPreference(TerminalViewPtr);
static void				copyTranslationPreferences		(TerminalViewPtr, Preferences_ContextRef);
static OSStatus			createWindowColorPalette		(TerminalViewPtr, Preferences_ContextRef, Boolean = true);
static Boolean			cursorBlinks					(TerminalViewPtr);
static OSStatus			dragTextSelection				(TerminalViewPtr, RgnHandle, EventRecord*, Boolean*);
static Boolean			drawSection						(TerminalViewPtr, CGContextRef, UInt16, UInt16, UInt16, UInt16);
static void				drawTerminalScreenRunOp			(TerminalScreenRef, UniChar const*, UInt16, Terminal_LineRef,
														 UInt16, TerminalTextAttributes, void*);
static void				drawTerminalText				(TerminalViewPtr, CGContextRef, CGRect const&, Rect const&, UniChar const*, CFIndex,
														 TerminalTextAttributes);
static void				drawVTGraphicsGlyph				(TerminalViewPtr, CGContextRef, Rect const*, UniChar, char, Boolean);
static void				eraseSection					(TerminalViewPtr, CGContextRef, SInt16, SInt16, CGRect&);
static void				eventNotifyForView				(TerminalViewConstPtr, TerminalView_Event, void*);
static Terminal_LineRef	findRowIterator					(TerminalViewPtr, UInt16);
static Boolean			findVirtualCellFromLocalPoint	(TerminalViewPtr, Point, TerminalView_Cell&, SInt16&, SInt16&);
static void				getBlinkAnimationColor			(TerminalViewPtr, UInt16, RGBColor*);
static void				getRowBounds					(TerminalViewPtr, UInt16, Rect*);
static SInt16			getRowCharacterWidth			(TerminalViewPtr, UInt16);
static void				getRowSectionBounds				(TerminalViewPtr, UInt16, UInt16, SInt16, Rect*);
static void				getScreenBaseColor				(TerminalViewPtr, TerminalView_ColorIndex, RGBColor*);
static void				getScreenColorsForAttributes	(TerminalViewPtr, TerminalTextAttributes, RGBColor*, RGBColor*, Boolean*);
static Boolean			getScreenCoreColor				(TerminalViewPtr, UInt16, RGBColor*);
static void				getScreenCustomColor			(TerminalViewPtr, TerminalView_ColorIndex, RGBColor*);
static void				getScreenOrigin					(TerminalViewPtr, SInt16*, SInt16*);
static void				getScreenOriginFloat			(TerminalViewPtr, Float32&, Float32&);
static Handle			getSelectedTextAsNewHandle		(TerminalViewPtr, UInt16, TerminalView_TextFlags);
static HIShapeRef		getSelectedTextAsNewHIShape		(TerminalViewPtr, Float32 = 0.0);
static RgnHandle		getSelectedTextAsNewRegion		(TerminalViewPtr);
static RgnHandle	getSelectedTextAsNewRegionOnScreen	(TerminalViewPtr);
static size_t			getSelectedTextSize				(TerminalViewPtr);
static HIShapeRef		getVirtualRangeAsNewHIShape		(TerminalViewPtr, TerminalView_Cell const&, TerminalView_Cell const&, Float32, Boolean);
static RgnHandle		getVirtualRangeAsNewRegion		(TerminalViewPtr, TerminalView_Cell const&, TerminalView_Cell const&, Boolean);
static RgnHandle	getVirtualRangeAsNewRegionOnScreen	(TerminalViewPtr, TerminalView_Cell const&, TerminalView_Cell const&, Boolean);
static void				getVirtualVisibleRegion			(TerminalViewPtr, SInt16*, SInt16*, SInt16*, SInt16*);
static void				handleMultiClick				(TerminalViewPtr, UInt16);
static void				handleNewViewContainerBounds	(HIViewRef, Float32, Float32, void*);
static void				highlightCurrentSelection		(TerminalViewPtr, Boolean, Boolean);
static void				highlightVirtualRange			(TerminalViewPtr, TerminalView_CellRange const&, TerminalTextAttributes, Boolean, Boolean);
static void				invalidateRowSection			(TerminalViewPtr, UInt16, UInt16, UInt16);
static Boolean			isMonospacedFont				(FMFontFamily);
static void				localToScreen					(TerminalViewPtr, SInt16*, SInt16*);
static Boolean			mainEventLoopEvent				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static pascal void		navigationFileCaptureDialogEvent(NavEventCallbackMessage, NavCBRecPtr, void*);
static void				offsetLeftVisibleEdge			(TerminalViewPtr, SInt16);
static void				offsetTopVisibleEdge			(TerminalViewPtr, SInt16);
static void				preferenceChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				preferenceChangedForView		(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				recalculateCachedDimensions		(TerminalViewPtr				inTerminalViewPtr);
static pascal OSStatus  receiveTerminalHIObjectEvents	(EventHandlerCallRef, EventRef, void*);
static OSStatus			receiveTerminalViewActiveStateChange	(EventHandlerCallRef, EventRef, TerminalViewRef);
static pascal OSStatus	receiveTerminalViewContextualMenuSelect	(EventHandlerCallRef, EventRef, void*);
static OSStatus			receiveTerminalViewDraw			(EventHandlerCallRef, EventRef, TerminalViewRef);
static OSStatus			receiveTerminalViewFocus		(EventHandlerCallRef, EventRef, TerminalViewRef);
static OSStatus			receiveTerminalViewGetData		(EventHandlerCallRef, EventRef, TerminalViewRef);
static OSStatus			receiveTerminalViewHitTest		(EventHandlerCallRef, EventRef, TerminalViewRef);
static pascal OSStatus	receiveTerminalViewRawKeyDown	(EventHandlerCallRef, EventRef, void*);
static OSStatus			receiveTerminalViewRegionRequest(EventHandlerCallRef, EventRef, TerminalViewRef);
static OSStatus			receiveTerminalViewTrack		(EventHandlerCallRef, EventRef, TerminalViewRef);
static void				receiveVideoModeChange			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				redrawScreensTerminalWindowOp	(TerminalWindowRef, void*, SInt32, void*);
static void				releaseRowIterator				(TerminalViewPtr, Terminal_LineRef*);
static SInt32			returnNumberOfCharacters		(TerminalViewPtr);
static CFStringRef		returnSelectedTextAsNewUnicode	(TerminalViewPtr, UInt16, TerminalView_TextFlags);
static void				screenBufferChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				screenCursorChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				screenToLocal					(TerminalViewPtr, SInt16*, SInt16*);
static void				screenToLocalRect				(TerminalViewPtr, Rect*);
static void				setBlinkAnimationColor			(TerminalViewPtr, UInt16, RGBColor const*);
static void				setBlinkingTimerActive			(TerminalViewPtr, Boolean);
static void				setCursorGhostVisibility		(TerminalViewPtr, Boolean);
static void				setCursorVisibility				(TerminalViewPtr, Boolean);
static void				setFontAndSize					(TerminalViewPtr, ConstStringPtr, UInt16, Float32 = 0, Boolean = true);
static SInt16			setPortScreenPort				(TerminalViewPtr);
static void				setScreenBaseColor				(TerminalViewPtr, TerminalView_ColorIndex, RGBColor const*);
static void				setScreenCoreColor				(TerminalViewPtr, UInt16, RGBColor const*);
static void				setScreenCustomColor			(TerminalViewPtr, TerminalView_ColorIndex, RGBColor const*);
static void				setUpCursorBounds				(TerminalViewPtr, SInt16, SInt16, Rect*,
														 TerminalView_CursorType =
															kTerminalView_CursorTypeCurrentPreferenceValue);
static void				setUpCursorGhost				(TerminalViewPtr, Point);
static void				setUpScreenFontMetrics			(TerminalViewPtr);
static void				sortAnchors						(TerminalView_Cell&, TerminalView_Cell&, Boolean);
static void				trackTextSelection				(TerminalViewPtr, Point, EventModifiers, Point*, UInt32*);
static void				useTerminalTextAttributes		(TerminalViewPtr, CGContextRef, TerminalTextAttributes);
static void				useTerminalTextColors			(TerminalViewPtr, CGContextRef, TerminalTextAttributes, Float32 = 1.0);
static void				visualBell						(TerminalViewRef);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	HIObjectClassRef			gTerminalTextViewHIObjectClassRef = nullptr;
	EventHandlerUPP				gMyTextViewConstructorUPP = nullptr;
	ListenerModel_ListenerRef	gMainEventLoopEventListener = nullptr;
	ListenerModel_ListenerRef	gPreferenceChangeEventListener = nullptr;
	struct MyPreferenceProxies	gPreferenceProxies;
	Boolean						gApplicationIsSuspended = false;
	Boolean						gTerminalViewInitialized = false;
	TerminalViewPtrLocker&		gTerminalViewPtrLocks ()				{ static TerminalViewPtrLocker x; return x; }
	RgnHandle					gInvalidationScratchRegion ()			{ static RgnHandle x = Memory_NewRegion(); assert(nullptr != x); return x; }
	My_XTerm256Table&			gColorGrid ()							{ static My_XTerm256Table x; return x; }
}


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
									{ kEventClassControl, kEventControlSetFocusPart },
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
		Preferences_Result		error = kPreferences_ResultOK;
		
		
		error = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagCursorBlinks,
											true/* call immediately to get initial value */);
		error = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagDontDimBackgroundScreens,
											true/* call immediately to get initial value */);
		error = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagNotifyOfBeeps,
											true/* call immediately to get initial value */);
		error = Preferences_StartMonitoring(gPreferenceChangeEventListener, kPreferences_TagPureInverse,
											true/* call immediately to get initial value */);
		// TMP - should check for errors here!
		//if (error != kPreferences_ResultOK) ...
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
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagDontDimBackgroundScreens);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagNotifyOfBeeps);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagPureInverse);
	ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
}// Done


/*!
Creates a new HIView hierarchy for a terminal view,
complete with all the callbacks and data necessary to
drive it.

The font, colors, etc. are based on the default preferences
context, where any settings in "inFormatOrNull" will
override the defaults.  You can access this configuration
later with TerminalView_ReturnFormatConfiguration().

Since this is entirely associated with an HIObject, the
view automatically goes away whenever the HIView from
TerminalView_ReturnContainerHIView() is destroyed.  (For
example, if you have this view in a window and then
dispose of the window.)

If any problems occur, nullptr is returned; otherwise,
a reference to the data associated with the new view is
returned.  (Use TerminalView_ReturnContainerHIView() or
TerminalView_ReturnUserFocusHIView() to get at parts of
the actual HIView hierarchy.)

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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	Terminal_DeleteAllSavedLines(viewPtr->screen.ref);
    eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
}// DeleteScrollback


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
		TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
		OSStatus					error = noErr;
		
		
		error = NavGetDefaultDialogCreationOptions(&dialogOptions);
		if (noErr == error)
		{
			// this call sets most of the options up front (parent window, etc.)
			dialogOptions.optionFlags |= kNavDontAddTranslateItems;
			Localization_GetCurrentApplicationNameAsCFString(&dialogOptions.clientName);
			dialogOptions.preferenceKey = kPreferences_NavPrefKeyGenericSaveFile;
			dialogOptions.parentWindow = HIViewGetWindow(viewPtr->contentHIView);
			
			// now set things specific to this instance
			(UIStrings_Result)UIStrings_Copy(kUIStrings_FileDefaultCaptureFile, dialogOptions.saveFileName);
			(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptCaptureToFile, dialogOptions.message);
			dialogOptions.modality = kWindowModalityWindowModal;
		}
		error = NavCreatePutFileDialog(&dialogOptions, 'TEXT'/* type */, 'ttxt'/* creator (TextEdit) */,
										NewNavEventUPP(navigationFileCaptureDialogEvent), inView/* client data */,
										&navigationServicesDialog);
		Alert_ReportOSStatus(error);
		if (noErr == error)
		{
			// display the dialog; it is a sheet, so this will return immediately
			// and the dialog will close whenever the user is actually done with it
			error = NavDialogRun(navigationServicesDialog);
		}
	}
}// DisplaySaveSelectedTextUI


/*!
Removes all highlighted search results ranges, and clears the
current search result focus.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

(3.1)
*/
TerminalView_Result
TerminalView_FindNothing	(TerminalViewRef	inView)
{
	TerminalView_Result		result = kTerminalView_ResultOK;
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		for (My_CellRangeList::const_iterator toRange = viewPtr->text.searchResults.begin();
				toRange != viewPtr->text.searchResults.end(); ++toRange)
		{
			highlightVirtualRange(viewPtr, *toRange, kTerminalTextAttributeSearchResult,
									false/* is highlighted */, true/* redraw */);
		}
		viewPtr->text.searchResults.clear();
		viewPtr->text.toCurrentSearchResult = viewPtr->text.searchResults.end();
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
	TerminalView_Result		result = kTerminalView_ResultOK;
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		viewPtr->text.searchResults.push_back(inSelection);
		assert(false == viewPtr->text.searchResults.empty());
		if (viewPtr->text.searchResults.end() == viewPtr->text.toCurrentSearchResult)
		{
			viewPtr->text.toCurrentSearchResult = viewPtr->text.searchResults.begin();
		}
		highlightVirtualRange(viewPtr, viewPtr->text.searchResults.back(),
								kTerminalTextAttributeSearchResult,
								true/* is highlighted */, true/* redraw */);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr->text.selection.exists)
	{
		register SInt16		i = 0;
		UInt32				finalTick = 0L;
		CGrafPtr			currentPort = nullptr;
		GDHandle			currentDevice = nullptr;
		
		
		// for better performance on Mac OS X, lock the bits of a port
		// before performing a series of QuickDraw operations in it
		GetGWorld(&currentPort, &currentDevice);
		(OSStatus)LockPortBits(currentPort);
		
		for (i = 0; i < 2; ++i)
		{
			Delay(5/* arbitrary; measured in 60ths of a second */, &finalTick);
			highlightCurrentSelection(viewPtr, false/* is highlighted */, true/* redraw */);
			
			// hand-hold Mac OS X to get the damned changes to show up!
			(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
			
			Delay(5, &finalTick);
			highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
			
			// hand-hold Mac OS X to get the damned changes to show up!
			(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
		}
		
		// undo the lock done earlier in this block
		(OSStatus)UnlockPortBits(currentPort);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	(OSStatus)DialogUtilities_SetKeyboardFocus(TerminalView_ReturnUserFocusHIView(inView));
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
						 RGBColor*					outColorPtr)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean					result = false;
	
	
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
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
								 StringPtr			outFontFamilyNameOrNull,
								 UInt16*			outFontSizeOrNull)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
	{
		if (outFontFamilyNameOrNull != nullptr) PLstrcpy(outFontFamilyNameOrNull, viewPtr->text.font.familyName);
		if (outFontSizeOrNull != nullptr) *outFontSizeOrNull = viewPtr->text.font.normalMetrics.size;
	}
}// GetFontAndSize


/*!
Retrieves the view’s best dimensions in pixels, given
the current font metrics and screen dimensions.
Returns true only if successful.

(3.1)
*/
Boolean
TerminalView_GetIdealSize	(TerminalViewRef	inView,
							 SInt16&			outWidthInPixels,
							 SInt16&			outHeightInPixels)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean					result = false;
	
	
	if (nullptr != viewPtr)
	{
		Float32		highPrecision = 0;
		
		
		highPrecision = viewPtr->screen.marginLeftEmScale + viewPtr->screen.paddingLeftEmScale
						+ viewPtr->screen.paddingRightEmScale + viewPtr->screen.marginRightEmScale;
		highPrecision *= viewPtr->text.font.widthPerCharacter;
		highPrecision += viewPtr->screen.viewWidthInPixels;
		outWidthInPixels = STATIC_CAST(highPrecision, SInt16);
		
		highPrecision = viewPtr->screen.marginTopEmScale + viewPtr->screen.paddingTopEmScale
						+ viewPtr->screen.paddingBottomEmScale + viewPtr->screen.marginBottomEmScale;
		highPrecision *= viewPtr->text.font.widthPerCharacter; // yes, width, because this is an “em” scale factor
		highPrecision += viewPtr->screen.viewHeightInPixels;
		outHeightInPixels = STATIC_CAST(highPrecision, SInt16);
		
		result = true;
	}
	return result;
}// GetIdealSize


/*!
Returns values for the specified range along one axis.
See the documentation on "TerminalView_RangeCode" for
more information.

See also TerminalView_ScrollPixelsTo().

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

\retval kTerminalView_ResultParameterError
if the range code is not valid

(3.1)
*/
TerminalView_Result
TerminalView_GetRange	(TerminalViewRef			inView,
						 TerminalView_RangeCode		inRangeCode,
						 UInt32&					outStartOfRange,
						 UInt32&					outPastEndOfRange)
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		switch (inRangeCode)
		{
		case kTerminalView_RangeCodeScrollRegionV:
			// IMPORTANT: This routine should derive range values in the same way as
			// TerminalView_ScrollPixelsTo().
			//
			// Although the top visible edge is stored internally as a negative value
			// (where zero means the main screen is fully visible and anything less
			// indicates scrollback), the returned range is defined with positive
			// integers.  In the returned range, the smallest possible value is zero
			// and the largest value depends on how many lines are in the scrollback
			// and main screen, the size (double or not) of those lines, and the font.
			//
			// Since the maximum height includes the bottommost screenful, the height
			// (in pixels) of the screen is subtracted from that height to find the
			// scrollback height.  The scrollback is then simply added to the internal
			// negative offset to find the overall pixel position relative to the
			// oldest row.
			//
			// This range definition is chosen to be convenient for scrollbars.
			outStartOfRange = viewPtr->screen.topVisibleEdgeInPixels/* negative for scrollback */ +
								viewPtr->screen.maxHeightInPixels - viewPtr->screen.viewHeightInPixels;
			outPastEndOfRange = outStartOfRange + viewPtr->screen.viewHeightInPixels;
			break;
		
		case kTerminalView_RangeCodeScrollRegionVMaximum:
			outStartOfRange = 0;
			outPastEndOfRange = viewPtr->screen.maxHeightInPixels;
			break;
		
		default:
			// ???
			result = kTerminalView_ResultParameterError;
			break;
		}
	}
	
	return result;
}// GetRange


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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
	{
		Handle		textHandle = getSelectedTextAsNewHandle(viewPtr, 0/* space info */, kTerminalView_TextFlagInline);
		
		
		if (textHandle != nullptr)
		{
			TerminalSpeaker_Ref		speaker = Terminal_ReturnSpeaker(viewPtr->screen.ref);
			
			
			if (nullptr != speaker)
			{
				TerminalSpeaker_Result		speakerResult = kTerminalSpeaker_ResultOK;
				
				
				speakerResult = TerminalSpeaker_SynthesizeSpeechFromBuffer(speaker, *textHandle, GetHandleSize(textHandle));
			}
			Memory_DisposeHandle(&textHandle);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
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
TerminalView_GetTheoreticalScreenDimensions		(TerminalViewRef	inView,
												 SInt16				inWidthInPixels,
												 SInt16				inHeightInPixels,
												 UInt16*			outColumnCount,
												 UInt16*			outRowCount)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Float32					highPrecision = 0.0;
	
	
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
TerminalView_GetTheoreticalViewSize		(TerminalViewRef	inView,
										 UInt16				inColumnCount,
										 UInt16				inRowCount,
										 SInt16*			outWidthInPixels,
										 SInt16*			outHeightInPixels)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Float32					highPrecision = 0.0;
	
	
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result		result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		viewPtr->configFilter.insert(inWhichSetting);
	}
	return result;
}// IgnoreChangesToPreference


/*!
MacTelnet 3.0 supports two modes of text selection:
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

IMPORTANT:	All of MacTelnet’s functions, including
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Cell		newColumnRow;
	SInt16					unusedDeltaColumn = 0;
	SInt16					unusedDeltaRow = 0;
	Boolean					moveOK = false;
	
	
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
			SInt16 const	kDeltaY = newColumnRow.second - STATIC_CAST(cursorY, SInt16);
			
			
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
Determines whether a point in the coordinate
system of a window’s graphics port is in the
boundaries of the highlighted text in that
window (if any).

Updated in MacTelnet 3.0 to account for a
selection area that is rectangular.

(2.6)
*/
Boolean
TerminalView_PtInSelection	(TerminalViewRef	inView,
							 Point				inLocalPoint)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean					result = false;
	
	
	if (viewPtr != nullptr)
	{
		TerminalView_Cell	newColumnRow;
		SInt16				deltaColumn = 0;
		SInt16				deltaRow = 0;
		
		
		(Boolean)findVirtualCellFromLocalPoint(viewPtr, inLocalPoint, newColumnRow, deltaColumn, deltaRow);
		
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
Returns the Mac OS HIView that is the root of the
specified screen.  With the container, you can safely
position a screen anywhere in a window and the
contents of the screen (embedded in the container)
will move with it.

To resize a screen’s view area, use a routine like
HIViewSetFrame() - Carbon Event handlers are installed
such that embedded views are properly resized when you
do that.

(3.0)
*/
HIViewRef
TerminalView_ReturnContainerHIView		(TerminalViewRef	inView)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIViewRef				result = nullptr;
	
	
	result = viewPtr->encompassingHIView;
	return result;
}// ReturnContainerHIView


/*!
Returns the current display mode, which is normal by
default but could have been changed using the
TerminalView_SetDisplayMode() routine.

(3.1)
*/
TerminalView_DisplayMode
TerminalView_ReturnDisplayMode	(TerminalViewRef	inView)
{
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
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

(3.1)
*/
HIViewRef
TerminalView_ReturnDragFocusHIView	(TerminalViewRef	inView)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIViewRef				result = nullptr;
	
	
	// should match whichever view has drag handlers installed on it
	result = viewPtr->contentHIView;
	return result;
}// ReturnDragFocusHIView


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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef		result = viewPtr->formatConfig;
	
	
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
	
	return result;
}// ReturnFormatConfiguration


/*!
DEPRECATED.  Use TerminalView_ReturnSelectedTextAsNewUnicode()
instead.

Returns the contents of the current selection for
the specified screen view, allocating a new handle.
If there is no selection, an empty handle is returned.
If any problems occur, nullptr is returned.

Use the parameters to this routine to affect how the
text is returned; for example, you can have the text
returned inline, or delimited by carriage returns.
The "inNumberOfSpacesToReplaceWithOneTabOrZero" value
can be 0 to perform no substitution, or a positive
integer to indicate how many consecutive spaces are
to be replaced with a single tab before returning the
text.

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
											 UInt16						inNumberOfSpacesToReplaceWithOneTabOrZero,
											 TerminalView_TextFlags		inFlags)
{
    TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Handle					result = nullptr;
	
	
	if (nullptr != viewPtr)
	{
		result = getSelectedTextAsNewHandle(viewPtr, inNumberOfSpacesToReplaceWithOneTabOrZero, inFlags);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	RgnHandle				result = nullptr;
	
	
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
The "inNumberOfSpacesToReplaceWithOneTabOrZero" value
can be 0 to perform no substitution, or a positive
integer to indicate how many consecutive spaces are
to be replaced with a single tab before returning the
text.

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
TerminalView_ReturnSelectedTextAsNewUnicode	(TerminalViewRef			inView,
											 UInt16						inNumberOfSpacesToReplaceWithOneTabOrZero,
											 TerminalView_TextFlags		inFlags)
{
    TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	CFStringRef				result = nullptr;
	
	
	if (nullptr != viewPtr)
	{
		result = returnSelectedTextAsNewUnicode(viewPtr, inNumberOfSpacesToReplaceWithOneTabOrZero, inFlags);
	}
	return result;
}// ReturnSelectedTextAsNewUnicode


/*!
Returns the size in bytes of the current selection for
the specified screen view, or zero if there is none.

(3.1)
*/
size_t
TerminalView_ReturnSelectedTextSize		(TerminalViewRef	inView)
{
    TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	size_t					result = 0;
	
	
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	Preferences_ContextRef		result = viewPtr->encodingConfig;
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
Returns the Mac OS HIView that can be focused
for keyboard input.  You can use this compared
with the current keyboard focus for a window to
determine if the specified view is focused.

(3.0)
*/
HIViewRef
TerminalView_ReturnUserFocusHIView	(TerminalViewRef	inView)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIViewRef				result = nullptr;
	
	
	// should match whichever view has the set-focus-part event handler installed on it
	result = viewPtr->focusAndPaddingHIView;
	return result;
}// ReturnUserFocusHIView


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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	WindowRef				result = nullptr;
	
	
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (inReverseVideo != viewPtr->screen.isReverseVideo)
	{
		RGBColor	oldTextColor;
		RGBColor	oldBackgroundColor;
		
		
		viewPtr->screen.isReverseVideo = !viewPtr->screen.isReverseVideo;
		
		// flip the background and foreground color positions
		getScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalText, &oldTextColor);
		getScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalBackground, &oldBackgroundColor);
		setScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalText, &oldBackgroundColor);
		setScreenCustomColor(viewPtr, kTerminalView_ColorIndexNormalBackground, &oldTextColor);
		
		// update the screen
		(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if ((viewPtr != nullptr) && (inNumberOfColumnsToScroll != 0))
	{
		// just redraw everything
		(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
		
		// update anchor
		offsetLeftVisibleEdge(viewPtr, +(inNumberOfColumnsToScroll * viewPtr->text.font.widthPerCharacter));
		
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr != nullptr)
	{
		// just redraw everything
		(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
		
		// update anchor
		offsetLeftVisibleEdge(viewPtr, -(inNumberOfColumnsToScroll * viewPtr->text.font.widthPerCharacter));
		
		eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
	}
	return result;
}// ScrollColumnsTowardRightEdge


/*!
Scrolls the contents of the terminal screen by a specific
number of pixels, as if the user dragged a scroll bar
indicator.  This routine reserves the right to snap the
amount to some less precise value.

Currently, the horizontal value is ignored.

\retval kTerminalView_ResultOK
if no error occurred

\retval kTerminalView_ResultInvalidID
if the view reference is unrecognized

\retval kTerminalView_ResultParameterError
if the range code is not valid

(3.1)
*/
TerminalView_Result
TerminalView_ScrollPixelsTo		(TerminalViewRef	inView,
								 UInt32				inStartOfRangeV,
								 UInt32				UNUSED_ARGUMENT(inStartOfRangeH))
{
	TerminalView_Result			result = kTerminalView_ResultOK;
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		// IMPORTANT: This routine should derive range values in the same way
		// as TerminalView_GetRange().
		SInt16 const	kNewValue = inStartOfRangeV - viewPtr->screen.maxHeightInPixels +
									viewPtr->screen.viewHeightInPixels;
		
		
		// just redraw everything
		(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
		
		// update anchor
		offsetTopVisibleEdge(viewPtr, kNewValue - viewPtr->screen.topVisibleEdgeInPixels);
		
		eventNotifyForView(viewPtr, kTerminalView_EventScrolling, inView/* context */);
	}
	
	return result;
}// ScrollPixelsTo


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
											 UInt16 			inNumberOfRowsToScroll)
{
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		// just redraw everything
		(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
		
		// update anchor
		// TEMPORARY: really, something should traverse the row attributes between
		// the current location and the final location, figure out how many of them
		// use double-sized text and double the height of each appropriate row
		offsetTopVisibleEdge(viewPtr, -(inNumberOfRowsToScroll * viewPtr->text.font.heightPerCharacter));
		
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
										 UInt16				inNumberOfRowsToScroll)
{
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result			result = kTerminalView_ResultOK;
	
	
	if (viewPtr == nullptr) result = kTerminalView_ResultInvalidID;
	else
	{
		// just redraw everything
		(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
		
		// update anchor
		// TEMPORARY: really, something should traverse the row attributes between
		// the current location and the final location, figure out how many of them
		// use double-sized text and double the height of each appropriate row
		offsetTopVisibleEdge(viewPtr, +(inNumberOfRowsToScroll * viewPtr->text.font.heightPerCharacter));
		
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
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
Returns true only if one or more ranges were specified as
search results.  (TerminalView_FindNothing() clears this.)

(3.1)
*/
Boolean
TerminalView_SearchResultsExist		(TerminalViewRef	inView)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean					result = false;
	
	
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
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
Highlights the specified range of text, clearing any
other selection outside the range.

In MacTelnet 3.0, the exact characters selected depends
on the current selection mode: in normal mode, the
specified anchors mark the first and last characters of
a Macintosh-style selection range.  However, if the new
“rectangular text selection” mode is being used, the
width of each line of the selected region is the same,
equal to the difference between the horizontal coordinates
of the given points.

(2.6)
*/
void
TerminalView_SelectVirtualRange		(TerminalViewRef				inView,
									 TerminalView_CellRange const&	inSelection)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
	{
		TerminalView_SelectNothing(inView);
		
		viewPtr->text.selection.range = inSelection;
		
		highlightCurrentSelection(viewPtr, true/* is highlighted */, true/* redraw */);
		viewPtr->text.selection.exists = true;
	}
}// SelectVirtualRange


/*!
Specifies whether ANSI-colored text and graphics
are allowed to be rendered in the specified screen.
This setting cannot currently be changed on the
fly unless it was enabled to begin with, because
various color resources required to make it work
are not always allocated.

(2.6)
*/
void
TerminalView_SetANSIColorsEnabled	(TerminalViewRef	inView,
									 Boolean			inUseANSIColorSequences)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	viewPtr->screen.areANSIColorsEnabled = inUseANSIColorSequences;
}// SetANSIColorsEnabled


/*!
Sets a new value for a specific color in a
terminal window.  Returns "true" only if the
color could be set.

(3.0)
*/
Boolean
TerminalView_SetColor	(TerminalViewRef			inView,
						 TerminalView_ColorIndex	inColorEntryNumber,
						 RGBColor const*			inColorPtr)
{
	Boolean		result = false;
	
	
	if ((inColorEntryNumber <= kTerminalView_ColorIndexLastValid) &&
		(inColorEntryNumber >= kTerminalView_ColorIndexFirstValid))
	{
		TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
		
		
		setScreenBaseColor(viewPtr, inColorEntryNumber, inColorPtr);
		result = true;
	}
	
	return result;
}// SetColor


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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
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
	(OSStatus)SetControlProperty(inView, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
									sizeof(showHighlight), &showHighlight);
	
	// change the cursor, for additional visual feedback
	if (showHighlight)
	{
		Cursors_UseArrowCopy();
	}
	else
	{
		Cursors_UseArrow();
	}
	
	// redraw the view
	(OSStatus)HIViewSetNeedsDisplay(inView, true);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	(OSStatus)HIViewSetDrawingEnabled(viewPtr->contentHIView, inIsDrawingEnabled);
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
	
	
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
customary in MacTelnet to perform these kinds of actions
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
								 ConstStringPtr		inFontFamilyNameOrNull,
								 UInt16				inFontSizeOrZero)
{
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), inView);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean					result = false;
	
	
	if (viewPtr != nullptr) result = viewPtr->text.selection.exists;
	return result;
}// TextSelectionExists


/*!
Returns "true" only if text selections in the specified
view are currently forced to be rectangular.

MacTelnet 3.0 supports two modes of text selection:
standard, like most Macintosh applications, and
rectangular, useful for capturing columns of output (for
example, data from a UNIX command like "top").

(3.0)
*/
Boolean
TerminalView_TextSelectionIsRectangular		(TerminalViewRef	inView)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	Boolean					result = 0;
	
	
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	TerminalView_Result		result = kTerminalView_ResultOK;
	
	
	if (nullptr == viewPtr) result = kTerminalView_ResultInvalidID;
	else
	{
		outRange.first = TerminalView_Cell(inRange.firstColumn, inRange.firstRow);
		outRange.second = TerminalView_Cell(inRange.firstColumn + inRange.columnCount,
											inRange.firstRow + inRange.rowCount);
	}
	return result;
}// TranslateTerminalScreenRange


/*!
Displays an animation that helps the user locate
the terminal cursor.

(3.0)
*/
void
TerminalView_ZoomToCursor	(TerminalViewRef	inView,
							 Boolean			inQuick)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if (viewPtr != nullptr)
	{
		Rect	globalCursorBounds;
		HIRect	screenContentFloatBounds;
		Rect	screenContentBounds;
		
		
		// find global rectangle of the screen area
		(OSStatus)HIViewGetBounds(viewPtr->contentHIView, &screenContentFloatBounds);
		SetRect(&screenContentBounds, 0, 0, STATIC_CAST(screenContentFloatBounds.size.width, SInt16),
				STATIC_CAST(screenContentFloatBounds.size.height, SInt16));
		screenToLocalRect(viewPtr, &screenContentBounds);
		QDLocalToGlobalRect(GetWindowPort(HIViewGetWindow(viewPtr->contentHIView)), &screenContentBounds);
		
		// find global rectangle of the cursor
		globalCursorBounds = viewPtr->screen.cursor.bounds;
		screenToLocalRect(viewPtr, &globalCursorBounds);
		QDLocalToGlobalRect(GetWindowPort(HIViewGetWindow(viewPtr->contentHIView)), &globalCursorBounds);
		
		//Console_WriteValueFloat4("zoom cursor screen bounds", screenBounds.left, screenBounds.top, screenBounds.right, screenBounds.bottom);
		{
			Rect	outsetCursorBounds = globalCursorBounds;
			
			
			InsetRect(&outsetCursorBounds, -20/* arbitrary */, -20/* arbitrary */);
			if (false == inQuick)
			{
				// unless in “quick” mode, be a little more obvious
				// by zooming first from the screen edges to the cursor
			#if 0
				(OSStatus)ZoomRects(&outsetCursorBounds, &globalCursorBounds, 10/* steps, arbitrary */, kZoomAccelerate);
			#else
				(OSStatus)ZoomRects(&screenContentBounds, &globalCursorBounds, 10/* steps, arbitrary */, kZoomAccelerate);
			#endif
			}
			
			// end the animation by enlarging rectangles around the cursor position
			(OSStatus)ZoomRects(&globalCursorBounds, &outsetCursorBounds, 20/* steps, arbitrary */, kZoomDecelerate);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
	if ((viewPtr != nullptr) && (false == viewPtr->text.searchResults.empty()))
	{
		RgnHandle	selectionRegion = getVirtualRangeAsNewRegion(viewPtr, viewPtr->text.toCurrentSearchResult->first,
																	viewPtr->text.toCurrentSearchResult->second,
																	false/* is rectangular */);
		
		
		if (selectionRegion != nullptr)
		{
			Rect	selectionBounds;
			Rect	outsetSelectionBounds;
			
			
			// find global rectangle of selection, and its immediate surrounding area
			QDLocalToGlobalRegion(GetWindowPort(HIViewGetWindow(viewPtr->contentHIView)), selectionRegion);
			GetRegionBounds(selectionRegion, &selectionBounds);
			outsetSelectionBounds = selectionBounds;
			InsetRect(&outsetSelectionBounds, -20/* arbitrary */, -20/* arbitrary */);
			
			// animate!
			(OSStatus)ZoomRects(&outsetSelectionBounds, &selectionBounds, 8/* steps, arbitrary */, kZoomDecelerate);
			
			Memory_DisposeRegion(&selectionRegion);
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
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	
	
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
			(OSStatus)HIViewGetBounds(viewPtr->contentHIView, &screenContentFloatBounds);
			SetRect(&screenContentBounds, 0, 0, STATIC_CAST(screenContentFloatBounds.size.width, SInt16),
					STATIC_CAST(screenContentFloatBounds.size.height, SInt16));
			screenToLocalRect(viewPtr, &screenContentBounds);
			QDLocalToGlobalRect(GetWindowPort(HIViewGetWindow(viewPtr->contentHIView)), &screenContentBounds);
			
			// animate!
			(OSStatus)ZoomRects(&screenContentBounds, &selectionBounds, 20/* steps, arbitrary */, kZoomDecelerate);
			
			Memory_DisposeRegion(&selectionRegion);
		}
	}
}// ZoomToSelection


#pragma mark Internal Methods

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
	// most colors are defined as a 6x6x6 cube
	SInt16 const	kCubeBase = 16; // first index
	for (SInt16 red = 0; red < 6; ++red)
	{
		for (SInt16 green = 0; green < 6; ++green)
		{
			for (SInt16 blue = 0; blue < 6; ++blue)
			{
				UInt8 const		kIndex = kCubeBase + (36 * red) + (6 * green) + blue;
				SInt16 const	kRedLevel = (red) ? (55 + 40 * red) : 0;
				SInt16 const	kGreenLevel = (green) ? (55 + 40 * green) : 0;
				SInt16 const	kBlueLevel = (blue) ? (55 + 40 * blue) : 0;
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
	SInt16 const	kGrayBase = 232; // first index
	for (SInt16 gray = 0; gray < 24; ++gray)
	{
		UInt8 const		kIndex = kGrayBase + gray;
		SInt16 const	kGrayLevel = 8 + 10 * gray;
		
		
		this->grayLevels[kIndex] = kGrayLevel;
	}
}// My_XTerm256Table


/*!
Fills in a QuickDraw RGBColor structure with appropriate
values based on the given 8-bit components.

(4.0)
*/
void
My_XTerm256Table::
makeRGBColor	(UInt8		inRed,
				 UInt8		inGreen,
				 UInt8		inBlue,
				 RGBColor&	outColor)
{
	Float32		fullIntensityFraction = 0.0;
	
	
	fullIntensityFraction = inRed;
	fullIntensityFraction /= 255;
	fullIntensityFraction *= RGBCOLOR_INTENSITY_MAX;
	outColor.red = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = inGreen;
	fullIntensityFraction /= 255;
	fullIntensityFraction *= RGBCOLOR_INTENSITY_MAX;
	outColor.green = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = inBlue;
	fullIntensityFraction /= 255;
	fullIntensityFraction *= RGBCOLOR_INTENSITY_MAX;
	outColor.blue = STATIC_CAST(fullIntensityFraction, unsigned short);
}// My_XTerm256Table::makeRGBColor


/*!
Constructor.  See receiveTerminalHIObjectEvents().

WARNING:	This constructor leaves the class uninitialized!
			You MUST invoke the initialize() method before
			doing anything with it.  This unsafe constructor
			exists to support the HIObject model.

(3.1)
*/
TerminalView::
TerminalView	(HIViewRef		inSuperclassViewInstance)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
encodingConfig(nullptr), // set later
formatConfig(nullptr), // set later
configFilter(),
changeListenerModel(nullptr), // set later
displayMode(kTerminalView_DisplayModeNormal), // set later
isActive(true),
accessibilityObject(AXUIElementCreateWithHIObjectAndIdentifier
					(REINTERPRET_CAST(inSuperclassViewInstance, HIObjectRef), 0/* identifier */)),
encompassingHIView(nullptr), // set later
backgroundHIView(nullptr), // set later
focusAndPaddingHIView(nullptr), // set later
contentHIView(inSuperclassViewInstance),
currentContentFocus(kControlEntireControl), // set later
containerResizeHandler(), // set later
contextualMenuHandler(),
rawKeyDownHandler(),
selfRef(REINTERPRET_CAST(this, TerminalViewRef))
{
}// TerminalView 1-argument constructor


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
TerminalView::
initialize		(TerminalScreenRef			inScreenDataSource,
				 Preferences_ContextRef		inFormat)
{
	this->selfRef = REINTERPRET_CAST(this, TerminalViewRef);
	
	this->encodingConfig = Preferences_NewContext(kPreferences_ClassTranslation);
	assert(nullptr != this->encodingConfig);
	{
		Boolean		setOK = TextTranslation_ContextSetEncoding(this->encodingConfig, kCFStringEncodingUTF8);
		
		
		assert(setOK);
	}
	
	this->formatConfig = Preferences_NewCloneContext(inFormat, true/* detach */);
	assert(nullptr != this->formatConfig);
	
	this->changeListenerModel = ListenerModel_New(kListenerModel_StyleStandard,
													kConstantsRegistry_ListenerModelDescriptorTerminalViewChanges);
	
	// automatically read the user preference for window resize behavior
	// and initialize appropriately
	{
		Boolean		affectsFontSize = false;
		size_t		actualSize = 0;
		
		
		if (kPreferences_ResultOK !=
			Preferences_GetData(kPreferences_TagTerminalResizeAffectsFontSize, sizeof(affectsFontSize), &affectsFontSize,
								&actualSize))
		{
			// assume a default, if the value cannot be found...
			affectsFontSize = false;
		}
		this->displayMode = (affectsFontSize) ? kTerminalView_DisplayModeZoom : kTerminalView_DisplayModeNormal;
	}
	
	// retain the screen reference
	this->screen.ref = inScreenDataSource;
	
	// miscellaneous settings
	this->text.selection.range.first.first = 0;
	this->text.selection.range.first.second = 0;
	this->text.selection.range.second.first = 0;
	this->text.selection.range.second.second = 0;
	this->text.selection.keyboardMode = kMy_SelectionModeUnset;
	this->text.selection.exists = false;
	this->text.selection.isRectangular = false;
	this->screen.leftVisibleEdgeInPixels = 0;
	this->screen.leftVisibleEdgeInColumns = 0;
	this->screen.topVisibleEdgeInPixels = 0;
	this->screen.topVisibleEdgeInRows = 0;
	this->screen.topEdgeInPixels = 0; // set later...
	this->screen.viewWidthInPixels = 0; // set later...
	this->screen.viewHeightInPixels = 0; // set later...
	this->screen.maxViewWidthInPixels = 0; // set later...
	this->screen.maxViewHeightInPixels = 0; // set later...
	this->screen.maxWidthInPixels = 0; // set later...
	this->screen.maxHeightInPixels = 0; // set later...
	this->screen.backgroundPicture = nullptr;
	this->screen.focusRingEnabled = true;
	this->screen.isReverseVideo = 0;
	this->screen.cursor.currentState = kMyCursorStateVisible;
	this->screen.cursor.ghostState = kMyCursorStateInvisible;
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
	
	// initialize focus area
	this->currentContentFocus = kTerminalView_ContentPartVoid;
	
	// create HIViews
	if (this->contentHIView != nullptr)
	{
		OSStatus	error = noErr;
		
		
		// see the HIObject callbacks, this will already exist
		error = HIViewSetVisible(this->contentHIView, true);
		assert_noerr(error);
		
		assert_noerr(TerminalBackground_CreateHIView(this->focusAndPaddingHIView));
		error = HIViewSetVisible(this->focusAndPaddingHIView, true);
		assert_noerr(error);
		// IMPORTANT: Set a property with the TerminalViewRef, so that
		// TerminalView_ReturnUserFocusHIView() works properly
		error = SetControlProperty(this->focusAndPaddingHIView,
									AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeTerminalViewRef,
									sizeof(this->selfRef), &this->selfRef);
		assert_noerr(error);
		
		assert_noerr(TerminalBackground_CreateHIView(this->backgroundHIView));
		error = HIViewSetVisible(this->backgroundHIView, true);
		assert_noerr(error);
		
		// initialize matte color
		{
			RGBColor* const		kColorPtr = &this->text.colors[kMyBasicColorIndexMatteBackground];
			
			
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
		error = HIViewAddSubview(this->backgroundHIView, this->focusAndPaddingHIView);
		assert_noerr(error);
		error = HIViewAddSubview(this->focusAndPaddingHIView, this->contentHIView);
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
		this->animation.rendering.region = Memory_NewRegion();
		this->animation.cursor.blinkAlpha = 1.0;
		this->animation.cursor.blinkAlphaDelta = -kTerminalView_BlinkAlphaDelta;
	}
	
	// install a monitor on the container that finds out about
	// resizes (for example, because HIViewSetFrame() was called
	// on it) and updates subviews to match
	this->containerResizeHandler.install(this->encompassingHIView, kCommonEventHandlers_ChangedBoundsAnyEdge,
											handleNewViewContainerBounds, this->selfRef/* context */);
	assert(this->containerResizeHandler.isInstalled());
	
	// install a handler for contextual menu clicks
	this->contextualMenuHandler.install(GetControlEventTarget(this->contentHIView),
										receiveTerminalViewContextualMenuSelect,
										CarbonEventSetInClass(CarbonEventClass(kEventClassControl),
																kEventControlContextualMenuClick),
										REINTERPRET_CAST(this, TerminalViewRef)/* user data */);
	assert(this->contextualMenuHandler.isInstalled());
	
	// set up keyboard text selection on the focus view
	this->rawKeyDownHandler.install(GetControlEventTarget(this->focusAndPaddingHIView),
										receiveTerminalViewRawKeyDown,
										CarbonEventSetInClass(CarbonEventClass(kEventClassKeyboard),
																kEventRawKeyDown, kEventRawKeyRepeat),
										REINTERPRET_CAST(this, TerminalViewRef)/* user data */);
	assert(this->rawKeyDownHandler.isInstalled());
	
	// show all HIViews
	HIViewSetVisible(this->encompassingHIView, true/* visible */);
	
	// ask to be notified of terminal bells
	this->screen.bellHandler = ListenerModel_NewStandardListener(audioEvent, this->selfRef/* context */);
	Terminal_StartMonitoring(inScreenDataSource, kTerminal_ChangeAudioEvent, this->screen.bellHandler);
	
	// ask to be notified of video mode changes
	this->screen.videoModeMonitor = ListenerModel_NewStandardListener(receiveVideoModeChange, this->selfRef/* context */);
	Terminal_StartMonitoring(inScreenDataSource, kTerminal_ChangeVideoMode, this->screen.videoModeMonitor);
	
	// ask to be notified of screen buffer content changes
	this->screen.contentMonitor = ListenerModel_NewStandardListener(screenBufferChanged, this->selfRef/* context */);
	Terminal_StartMonitoring(inScreenDataSource, kTerminal_ChangeText, this->screen.contentMonitor);
	Terminal_StartMonitoring(inScreenDataSource, kTerminal_ChangeScrollActivity, this->screen.contentMonitor);
	
	// ask to be notified of cursor changes
	this->screen.cursorMonitor = ListenerModel_NewStandardListener(screenCursorChanged, this->selfRef/* context */);
	Terminal_StartMonitoring(inScreenDataSource, kTerminal_ChangeCursorLocation, this->screen.cursorMonitor);
	Terminal_StartMonitoring(inScreenDataSource, kTerminal_ChangeCursorState, this->screen.cursorMonitor);
	
	// set up a callback to receive preference change notifications
	this->screen.preferenceMonitor = ListenerModel_NewStandardListener(preferenceChangedForView, this->selfRef/* context */);
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring(this->screen.preferenceMonitor, kPreferences_TagTerminalCursorType,
													false/* call immediately to get initial value */);
		prefsResult = Preferences_StartMonitoring(this->screen.preferenceMonitor, kPreferences_TagTerminalResizeAffectsFontSize,
													false/* call immediately to get initial value */);
		prefsResult = Preferences_ContextStartMonitoring(this->encodingConfig, this->screen.preferenceMonitor,
															kPreferences_ChangeContextBatchMode);
		prefsResult = Preferences_ContextStartMonitoring(this->formatConfig, this->screen.preferenceMonitor,
															kPreferences_ChangeContextBatchMode);
	}
}// initialize


/*!
Destructor.  See the handler for HIObject events
that deals with setup and teardown.

(3.0)
*/
TerminalView::
~TerminalView ()
{
	// stop listening for terminal bells
	Terminal_StopMonitoring(this->screen.ref, kTerminal_ChangeAudioEvent, this->screen.bellHandler);
	ListenerModel_ReleaseListener(&this->screen.bellHandler);
	
	// stop listening for video mode changes
	Terminal_StopMonitoring(this->screen.ref, kTerminal_ChangeVideoMode, this->screen.videoModeMonitor);
	ListenerModel_ReleaseListener(&this->screen.videoModeMonitor);
	
	// stop listening for screen buffer content changes
	Terminal_StopMonitoring(this->screen.ref, kTerminal_ChangeText, this->screen.contentMonitor);
	Terminal_StopMonitoring(this->screen.ref, kTerminal_ChangeScrollActivity, this->screen.contentMonitor);
	ListenerModel_ReleaseListener(&this->screen.contentMonitor);
	
	// stop listening for cursor changes
	Terminal_StopMonitoring(this->screen.ref, kTerminal_ChangeCursorLocation, this->screen.cursorMonitor);
	Terminal_StopMonitoring(this->screen.ref, kTerminal_ChangeCursorState, this->screen.cursorMonitor);
	ListenerModel_ReleaseListener(&this->screen.cursorMonitor);
	
	// stop receiving preference change notifications
	Preferences_StopMonitoring(this->screen.preferenceMonitor, kPreferences_TagTerminalCursorType);
	Preferences_StopMonitoring(this->screen.preferenceMonitor, kPreferences_TagTerminalResizeAffectsFontSize);
	(Preferences_Result)Preferences_ContextStopMonitoring(this->encodingConfig, this->screen.preferenceMonitor,
															kPreferences_ChangeContextBatchMode);
	(Preferences_Result)Preferences_ContextStopMonitoring(this->formatConfig, this->screen.preferenceMonitor,
															kPreferences_ChangeContextBatchMode);
	ListenerModel_ReleaseListener(&this->screen.preferenceMonitor);
	
	// remove timer
	RemoveEventLoopTimer(this->animation.timer.ref), this->animation.timer.ref = nullptr;
	DisposeEventLoopTimerUPP(this->animation.timer.upp), this->animation.timer.upp = nullptr;
	
	// 3.0 - destroy any picture
	//if (nullptr != this->backgroundHIView)
	//{
	//}
	
	Memory_DisposeRegion(&this->animation.rendering.region);
	ListenerModel_Dispose(&this->changeListenerModel);
	Preferences_ReleaseContext(&this->encodingConfig);
	Preferences_ReleaseContext(&this->formatConfig);
}// TerminalView destructor


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
static pascal void
animateBlinkingItems	(EventLoopTimerRef		inTimer,
						 void*					inTerminalViewRef)
{
	TerminalViewRef			ref = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	TerminalViewAutoLocker	ptr(gTerminalViewPtrLocks(), ref);
	
	
	if (ptr != nullptr)
	{
		RGBColor	currentColor;
		
		
		// for simplicity, keep the cursor and text blinks in sync
		(OSStatus)SetEventLoopTimerNextFireTime(inTimer, ptr->animation.rendering.delays[ptr->animation.rendering.stage]);
		
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
		(OSStatus)HIViewSetNeedsDisplayInRegion(ptr->contentHIView, ptr->animation.rendering.region, true);
		
		//
		// cursor
		//
		
		// adjust the alpha setting to be used for cursor-drawing
		ptr->animation.cursor.blinkAlpha += ptr->animation.cursor.blinkAlphaDelta;
		if (ptr->animation.cursor.blinkAlpha < 0.0)
		{
			ptr->animation.cursor.blinkAlphaDelta = +kTerminalView_BlinkAlphaDelta;
			ptr->animation.cursor.blinkAlpha = 0.0;
		}
		else if (ptr->animation.cursor.blinkAlpha >= 1.0)
		{
			ptr->animation.cursor.blinkAlphaDelta = -kTerminalView_BlinkAlphaDelta;
			ptr->animation.cursor.blinkAlpha = 1.0;
		}
		
		// invalidate the cursor
		RectRgn(gInvalidationScratchRegion(), &ptr->screen.cursor.bounds);
		(OSStatus)HIViewSetNeedsDisplayInRegion(ptr->contentHIView, gInvalidationScratchRegion(), true);
	}
}// animateBlinkingItems


/*!
Responds to terminal bells visually, if visual bells
are enabled.

Also updates the bell toolbar item when sound is
enabled or disabled for a terminal.

(3.0)
*/
static void
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
static EventTime
calculateAnimationStageDelay	(TerminalViewPtr					inTerminalViewPtr,
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
static void
calculateDoubleSize		(TerminalViewPtr	inTerminalViewPtr,
						 SInt16&			outPointSize,
						 SInt16&			outAscent)
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
	while (STATIC_CAST(CharWidth('A'), unsigned int) < INTEGER_DOUBLED(inTerminalViewPtr->text.font.widthPerCharacter))
	{
		TextSize(++outPointSize);
	}
	GetFontInfo(&info);
	while (STATIC_CAST(info.ascent + info.descent + info.leading, unsigned int) >=
			INTEGER_DOUBLED(inTerminalViewPtr->text.font.heightPerCharacter))
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
static UInt16
copyColorPreferences	(TerminalViewPtr			inTerminalViewPtr,
						 Preferences_ContextRef		inSource,
						 Boolean					inSearchForDefaults)
{
	TerminalView_ColorIndex		currentColorID = 0;
	UInt16						currentIndex = 0;
	Preferences_Tag				currentPrefsTag = '----';
	RGBColor					colorValue;
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
static UInt16
copyFontPreferences		(TerminalViewPtr			inTerminalViewPtr,
						 Preferences_ContextRef		inSource,
						 Boolean					inSearchDefaults)
{
	UInt16		result = 0;
	Str255		fontName;
	StringPtr	fontNamePtr = nullptr;
	SInt16		fontSize = 0;
	Float32		charWidthScale = 1.0;
	
	
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, kPreferences_TagFontName,
															sizeof(fontName), fontName, inSearchDefaults))
	{
		fontNamePtr = fontName;
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
	setFontAndSize(inTerminalViewPtr, fontNamePtr, fontSize, charWidthScale);
	
	return result;
}// copyFontPreferences


/*!
Copies all of the selected text to the clipboard
under the condition that the user has set the
preference to automatically copy new selections.

Use this when the user changes the text selection.

(3.1)
*/
static void
copySelectedTextIfUserPreference	(TerminalViewPtr	inTerminalViewPtr)
{
	if (inTerminalViewPtr->text.selection.exists)
	{
		Boolean		copySelectedText = false;
		size_t		actualSize = 0L;
		
		
		unless (Preferences_GetData(kPreferences_TagCopySelectedText, sizeof(copySelectedText),
									&copySelectedText, &actualSize) == kPreferences_ResultOK)
		{
			copySelectedText = false; // assume text isn’t automatically copied, if preference can’t be found
		}
		
		if (copySelectedText) Clipboard_TextToScrap(inTerminalViewPtr->selfRef, kClipboard_CopyMethodTable);
	}
}// copySelectedTextIfUserPreference


/*!
Attempts to read all supported text translation tags from
the given preference context.

(4.0)
*/
static void
copyTranslationPreferences	(TerminalViewPtr			inTerminalViewPtr,
							 Preferences_ContextRef		inSource)
{
	CFStringEncoding	newInputEncoding = TextTranslation_ContextReturnEncoding
											(inSource, kCFStringEncodingUTF8/* default */);
	
	
	Terminal_SetTextEncoding(inTerminalViewPtr->screen.ref, newInputEncoding);
	
	Console_WriteValue("terminal input text encoding changed to", newInputEncoding);
	{
		CFStringRef		nameCFString = CFStringConvertEncodingToIANACharSetName(newInputEncoding);
		
		
		Console_WriteValueCFString("terminal input text encoding changed to (name)", nameCFString);
	}
}// copyTranslationPreferences


/*!
Creates a new color palette and initializes its colors using
terminal preferences.

In order to succeed, the specified view’s self-reference must
be valid.

Returns "noErr" only if the palette is created successfully.

(3.1)
*/
static OSStatus
createWindowColorPalette	(TerminalViewPtr			inTerminalViewPtr,
							 Preferences_ContextRef		inFormat,
							 Boolean					inSearchForDefaults)
{
	OSStatus	result = noErr;
	
	
	// create palettes for future use
	inTerminalViewPtr->customColors.resize(kTerminalView_ColorIndexLastValid - kTerminalView_ColorIndexFirstValid + 1);
	inTerminalViewPtr->blinkColors.resize(kMy_BlinkingColorCount);
	
	// set up window’s colors; note that this will set the non-ANSI colors,
	// the blinking colors, and the ANSI colors
	(UInt16)copyColorPreferences(inTerminalViewPtr, inFormat, inSearchForDefaults);
	
	// install a timer to modify blinking text color entries periodically
	assert(nullptr != inTerminalViewPtr->selfRef);
	inTerminalViewPtr->animation.timer.upp = NewEventLoopTimerUPP(animateBlinkingItems);
	(OSStatus)InstallEventLoopTimer(GetCurrentEventLoop(), kEventDurationForever/* time before first fire */,
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
static Boolean
cursorBlinks	(TerminalViewPtr	UNUSED_ARGUMENT(inTerminalViewPtr))
{
	return gPreferenceProxies.cursorBlinks;
}// cursorBlinks


/*!
This method handles dragging of a text selection.
If the text is dropped (i.e. not cancelled), then
"true" is returned in "outDragWasDropped".

(3.0)
*/
OSStatus
dragTextSelection	(TerminalViewPtr	inTerminalViewPtr,
					 RgnHandle			inoutGlobalDragOutlineRegion,
					 EventRecord*		inoutEventPtr,
					 Boolean*			outDragWasDropped)
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
			SInt16		oldCursor = Cursors_UseArrow();
			
			
			result = TrackDrag(dragRef, inoutEventPtr, inoutGlobalDragOutlineRegion);
			Cursors_Use(oldCursor);
		}
	}
	DisposeDrag(dragRef), dragRef = nullptr;
	
	Cursors_UseArrow();
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
static Boolean
drawSection		(TerminalViewPtr	inTerminalViewPtr,
				 CGContextRef		inDrawingContext,
				 UInt16				UNUSED_ARGUMENT(inZeroBasedLeftmostColumnToDraw),
				 UInt16				inZeroBasedTopmostRowToDraw,
				 UInt16				UNUSED_ARGUMENT(inZeroBasedPastTheRightmostColumnToDraw),
				 UInt16				inZeroBasedPastTheBottommostRowToDraw)
{
	Boolean		result = false;
	
	
	// debug
	//Console_WriteValuePair("draw lines in past-the-end range", inZeroBasedTopmostRowToDraw, inZeroBasedPastTheBottommostRowToDraw);
	
	if (nullptr != inTerminalViewPtr)
	{
		Terminal_Result		iteratorResult = kTerminal_ResultOK;
		CGrafPtr			currentPort = nullptr;
		GDHandle			currentDevice = nullptr;
		
		
		// for better performance on Mac OS X, lock the bits of a port
		// before performing a series of QuickDraw operations in it,
		// and make the intended drawing area part of the dirty region
		GetGWorld(&currentPort, &currentDevice);
		(OSStatus)LockPortBits(currentPort);
		
		// find contiguous blocks of text on each line in the given
		// range that have the same attributes, and draw those blocks
		// all at once (which is more efficient than doing them
		// individually); a field inside the structure is used to
		// track the current line, so that drawTerminalScreenRunOp()
		// can determine the correct drawing rectangle for the line
		{
			Terminal_LineRef	lineIterator = findRowIterator
												(inTerminalViewPtr, inZeroBasedTopmostRowToDraw);
			
			
			if (nullptr != lineIterator)
			{
				TerminalTextAttributes		lineGlobalAttributes = 0L;
				Terminal_Result				terminalError = kTerminal_ResultOK;
				
				
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
					lineIterator = findRowIterator(inTerminalViewPtr, inTerminalViewPtr->screen.currentRenderedLine);
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
						lineGlobalAttributes = 0;
					}
					if (STYLE_IS_DOUBLE_WIDTH_ONLY(lineGlobalAttributes))
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
								sourceRect.right -= INTEGER_HALVED(sourceRect.right - sourceRect.left);
								
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
									OffsetRect(&sourceRect, -pixelMapBounds.left, -pixelMapBounds.top);
									error = DecompressImage(GetPixBaseAddr(pixels), image, pixels,
															&sourceRect/* source rectangle */,
															&destRect/* destination rectangle */,
															srcCopy/* drawing mode */, nullptr/* mask */);
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
		
		// undo the lock done earlier in this block
		(OSStatus)UnlockPortBits(currentPort);
	}
	return result;
}// drawSection


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
static void
drawTerminalScreenRunOp		(TerminalScreenRef			UNUSED_ARGUMENT(inScreen),
							 UniChar const*				inLineTextBufferOrNull,
							 UInt16						inLineTextBufferLength,
							 Terminal_LineRef			UNUSED_ARGUMENT(inRow),
							 UInt16						inZeroBasedStartColumnNumber,
							 TerminalTextAttributes		inAttributes,
							 void*						inTerminalViewPtr)
{
	TerminalViewPtr		viewPtr = REINTERPRET_CAST(inTerminalViewPtr, TerminalViewPtr);
	CGRect				sectionBounds;
	
	
	// set up context foreground and background colors appropriately
	// for the specified terminal attributes; this takes into account
	// things like bold and highlighted text, etc.
	useTerminalTextColors(viewPtr, viewPtr->screen.currentRenderContext, inAttributes, 1.0/* alpha */);
	
	// erase and redraw the current rendering line, but only the
	// specified range (starting column and character count)
	eraseSection(viewPtr, viewPtr->screen.currentRenderContext,
					inZeroBasedStartColumnNumber, inZeroBasedStartColumnNumber + inLineTextBufferLength,
					sectionBounds);
	
	// draw the text or graphics
	if ((nullptr != inLineTextBufferOrNull) && (0 != inLineTextBufferLength))
	{
		Rect	intBounds;
		
		
		// TEMPORARY - for QuickDraw use only
		SetRect(&intBounds, STATIC_CAST(sectionBounds.origin.x, short), STATIC_CAST(sectionBounds.origin.y, short),
				STATIC_CAST(sectionBounds.origin.x + sectionBounds.size.width, short),
				STATIC_CAST(sectionBounds.origin.y + sectionBounds.size.height, short));
		
		drawTerminalText(viewPtr, viewPtr->screen.currentRenderContext, sectionBounds, intBounds,
							inLineTextBufferOrNull, inLineTextBufferLength, inAttributes);
		
		// since blinking forces frequent redraws, do not do it more
		// than necessary; keep track of any blink attributes, and
		// elsewhere this flag can be used to disable the animation
		// timer when it is not needed
		if (STYLE_BLINKING(inAttributes))
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
		std::snprintf(x, sizeof(x), "%d", inZeroBasedStartColumnNumber);
		StringUtilities_CToP(x, pstr);
		DrawString(pstr);
	}
#endif
#if 0
	// DEBUGGING ONLY: try to see what line is drawn where, by rendering a number
	{
		char x[255];
		Str255 pstr;
		std::snprintf(x, sizeof(x), "%d", viewPtr->screen.currentRenderedLine);
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

(3.0)
*/
static void
drawTerminalText	(TerminalViewPtr			inTerminalViewPtr,
					 CGContextRef				inDrawingContext,
					 CGRect const&				inBoundaries,
					 Rect const&				inOldQuickDrawBoundaries,
					 UniChar const*				inTextBufferPtr,
					 CFIndex					inCharacterCount,
					 TerminalTextAttributes		inAttributes)
{
	// TEMPORARY: Unicode imaging is not supported yet, so the data
	// must first be converted into Mac Roman so QuickDraw can use it
	CFRetainRelease		oldMacRomanBufferCFString(CFStringCreateWithCharacters
													(kCFAllocatorDefault, inTextBufferPtr, inCharacterCount),
													true/* is retained */);
	char const*			oldMacRomanBufferForQuickDraw = CFStringGetCStringPtr(oldMacRomanBufferCFString.returnCFStringRef(),
																				kCFStringEncodingMacRoman);
	UInt8*				deletedBufferPtr = nullptr;
	
	
	if (nullptr == oldMacRomanBufferForQuickDraw)
	{
		// TEMPORARY (convert renderer to Unicode!)
		// not ideal, but if the internal buffer is not a byte array,
		// it must be copied before it can be interpreted that way
		deletedBufferPtr = new UInt8[inCharacterCount];
		
		CFIndex		bytesUsed = 0;
		CFIndex		conversionResult = CFStringGetBytes(oldMacRomanBufferCFString.returnCFStringRef(), CFRangeMake(0, inCharacterCount),
														kCFStringEncodingMacRoman, '?'/* loss byte */, false/* is external representation */,
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
			(OSStatus)QDAddRectToDirtyRegion(currentPort, &inOldQuickDrawBoundaries);
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
					(STYLE_IS_DOUBLE_HEIGHT_BOTTOM(inAttributes)
													? inTerminalViewPtr->text.font.doubleMetrics.ascent
													: inTerminalViewPtr->text.font.normalMetrics.ascent));
		
		// if bold or large text or graphics are being drawn, do it one character
		// at a time; bold fonts typically increase the font spacing, and double-
		// sized text needs to occupy twice the normal font spacing, so a regular
		// DrawText() won’t work; instead, each character must be drawn
		// individually, allowing MacTelnet to correct the pen location after
		// each character *as if* a normal character were just rendered (this
		// ensures that everything remains aligned with text in the same column)
		if (terminalFontSize == kArbitraryDoubleWidthDoubleHeightPseudoFontSize)
		{
			// top half of double-sized text; this is not rendered, but the pen should move double the distance anyway
			Move(inCharacterCount * INTEGER_DOUBLED(inTerminalViewPtr->text.font.widthPerCharacter), 0);
		}
		else if (terminalFontSize == inTerminalViewPtr->text.font.doubleMetrics.size)
		{
			// bottom half of double-sized text; force the text to use
			// twice the normal font metrics
			register SInt16		i = 0;
			Point				oldPen;
			
			
			for (i = 0; i < inCharacterCount; ++i)
			{
				GetPen(&oldPen);
				if (terminalFontID == kArbitraryVTGraphicsPseudoFontID)
				{
					// draw a graphics character
					drawVTGraphicsGlyph(inTerminalViewPtr, inDrawingContext, &inOldQuickDrawBoundaries, inTextBufferPtr[i],
										oldMacRomanBufferForQuickDraw[i], true/* is double width */);
				}
				else
				{
					// draw a normal character
					DrawText(oldMacRomanBufferForQuickDraw, i/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
				}
				
				MoveTo(oldPen.h + INTEGER_DOUBLED(inTerminalViewPtr->text.font.widthPerCharacter), oldPen.v);
			}
		}
		else if ((terminalFontStyle & bold) || (terminalFontID == kArbitraryVTGraphicsPseudoFontID) ||
					(false == inTerminalViewPtr->text.font.isMonospaced) ||
					(1.0 != inTerminalViewPtr->text.font.scaleWidthPerCharacter))
		{
			// proportional font, or bold, or otherwise non-standard width; force the text
			// to draw one character at a time so that the character offset can be corrected
			register SInt16		i = 0;
			Point				oldPen;
			
			
			for (i = 0; i < inCharacterCount; ++i)
			{
				GetPen(&oldPen);
				if (terminalFontID == kArbitraryVTGraphicsPseudoFontID)
				{
					// draw a graphics character
					drawVTGraphicsGlyph(inTerminalViewPtr, inDrawingContext, &inOldQuickDrawBoundaries, inTextBufferPtr[i],
										oldMacRomanBufferForQuickDraw[i], false/* is double width */);
				}
				else
				{
					// draw a normal character
					DrawText(oldMacRomanBufferForQuickDraw, i/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
				}
				MoveTo(oldPen.h + inTerminalViewPtr->text.font.widthPerCharacter, oldPen.v);
			}
		}
		else
		{
			// *completely* normal text (no special style, size, or character set); this is probably
			// fastest if rendered all at once using a single QuickDraw call, and since there are no
			// forced font metrics with normal text, this can be a lot simpler (this is also almost
			// certainly the common case, so it’s good if this is as efficient as possible)
			DrawText(oldMacRomanBufferForQuickDraw, 0/* offset */, inCharacterCount); // draw text using current font, size, color, etc.
		}
		
		// draw the cursor if it is in the requested region
		{
			Rect	intersection;
			
			
			if (SectRect(&inOldQuickDrawBoundaries, &inTerminalViewPtr->screen.cursor.bounds, &intersection) &&
				(kMyCursorStateVisible == inTerminalViewPtr->screen.cursor.currentState))
			{
				RectRgn(gInvalidationScratchRegion(), &inTerminalViewPtr->screen.cursor.bounds);
				(OSStatus)HIViewSetNeedsDisplayInRegion(inTerminalViewPtr->contentHIView, gInvalidationScratchRegion(), true);
			}
		}
	}
	
	if (nullptr != deletedBufferPtr)
	{
		delete [] deletedBufferPtr, deletedBufferPtr = nullptr;
	}
}// drawTerminalText


/*!
Renders a special graphics character at the current
pen location, assuming the pen is at the baseline of
where a font character would be inserted.  All other
text-related aspects of the specified port are used to
affect graphics (such as the presence of a bold face).

The specified Unicode character is redundantly specified
as a pre-translated Mac Roman character (or some loss
byte if none is appropriate), for the TEMPORARY use of
old drawing code that must use QuickDraw.

Due to the difficulty of creating vector-based fonts
that line graphics up properly, and the lousy look
of scaled-up bitmapped fonts, MacTelnet renders most
VT font characters on its own, using line drawing.
In addition, MacTelnet uses international symbols
for glyphs such as CR, HT, etc. instead of the Roman-
based ones prescribed by the standard VT font.

(3.0)
*/
static void
drawVTGraphicsGlyph		(TerminalViewPtr	inTerminalViewPtr,
						 CGContextRef		inDrawingContext,
						 Rect const*		inBoundaries,
						 UniChar			inUnicode,
						 char				inMacRomanForQuickDraw, // DEPRECATED
						 Boolean			inIsDoubleWidth)
{
	Rect		cellRect;
	Point		cellCenter; // used for line drawing glyphs
	SInt16		cellTop = 0;
	SInt16		cellLeft = 0;
	SInt16		cellRight = 0;
	SInt16		cellBottom = 0;
	SInt16		lineWidth = 1; // changed later...
	SInt16		lineHeight = 1; // changed later...
	SInt16		preservedFontID = 0;
	
	
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
	{
		Point	penLocation;
		
		
		GetPen(&penLocation);
		cellTop = inBoundaries->top;
		cellLeft = penLocation.h;
		cellRight = cellLeft + inTerminalViewPtr->text.font.widthPerCharacter;
		if (inIsDoubleWidth) cellRight += inTerminalViewPtr->text.font.widthPerCharacter;
		cellBottom = inBoundaries->bottom;
		SetRect(&cellRect, cellLeft, cellTop, cellRight, cellBottom);
	}
	SetPt(&cellCenter, cellLeft + INTEGER_HALVED(cellRight - cellLeft),
			cellTop + INTEGER_HALVED(cellBottom - cellTop));
	
	// The set of characters supported here should also be used in the
	// translateCharacter() internal method of the Terminal module.
	// That way, this renderer will always be called for these characters,
	// whether or not they were originally tagged as being graphical.
	switch (inUnicode)
	{
	case 0x2593: // checkerboard
		{
			PenState	penState;
			Pattern		checkerPat = { { 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC } }; // “fat” checkboard pattern
			
			
			GetPenState(&penState);
			PenPat(&checkerPat);
			PaintRect(&cellRect);
			SetPenState(&penState);
		}
		break;
	
	case 0x21E5: // horizontal tab (international symbol is a right-pointing arrow with a terminating line)
		// draw horizontal line
		MoveTo(cellLeft + lineWidth/* break from adjacent characters */, cellCenter.v);
		LineTo(cellRight - lineWidth/* break from adjacent characters */, cellCenter.v);
		// draw top part of arrowhead
		LineTo(cellCenter.h + INTEGER_HALVED(cellRight - cellCenter.h),
				cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		// draw bottom part of arrowhead
		MoveTo(cellRight - lineWidth/* break from adjacent characters */, cellCenter.v);
		LineTo(cellCenter.h + INTEGER_HALVED(cellRight - cellCenter.h),
				cellBottom - INTEGER_HALVED(cellBottom - cellCenter.v));
		// draw end line
		MoveTo(cellRight - lineWidth/* break from adjacent characters */, cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		LineTo(cellRight - lineWidth/* break from adjacent characters */, cellBottom - INTEGER_HALVED(cellBottom - cellCenter.v));
		break;
	
	case 0x21DF: // form feed (international symbol is an arrow pointing top to bottom with two horizontal lines through it)
		// draw vertical line
		MoveTo(cellCenter.h, cellTop + lineHeight/* break from adjacent characters */);
		LineTo(cellCenter.h, cellBottom - lineHeight/* break from adjacent characters */);
		// draw top part of arrowhead
		LineTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft),
				cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		// draw bottom part of arrowhead
		MoveTo(cellCenter.h, cellBottom - lineHeight/* break from adjacent characters */);
		LineTo(cellRight - INTEGER_HALVED(cellRight - cellCenter.h),
				cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		// draw lines across arrow
		MoveTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft), cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		LineTo(cellRight - INTEGER_HALVED(cellRight - cellCenter.h), cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		MoveTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft), cellCenter.v);
		LineTo(cellRight - INTEGER_HALVED(cellRight - cellCenter.h), cellCenter.v);
		break;
	
	case 0x2190: // carriage return (international symbol is an arrow pointing right to left)
		// draw horizontal line
		MoveTo(cellRight - lineWidth/* break from adjacent characters */, cellCenter.v);
		LineTo(cellLeft + lineWidth/* break from adjacent characters */, cellCenter.v);
		// draw top part of arrowhead
		LineTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft),
				cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		// draw bottom part of arrowhead
		MoveTo(cellLeft + lineWidth/* break from adjacent characters */, cellCenter.v);
		LineTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft),
				cellBottom - INTEGER_HALVED(cellBottom - cellCenter.v));
		break;
	
	case 0x2193: // line feed (international symbol is an arrow pointing top to bottom)
		// draw vertical line
		MoveTo(cellCenter.h, cellTop + lineHeight/* break from adjacent characters */);
		LineTo(cellCenter.h, cellBottom - lineHeight/* break from adjacent characters */);
		// draw top part of arrowhead
		LineTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft),
				cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		// draw bottom part of arrowhead
		MoveTo(cellCenter.h, cellBottom - lineHeight/* break from adjacent characters */);
		LineTo(cellRight - INTEGER_HALVED(cellRight - cellCenter.h),
				cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		break;
	
	case 0x21B5: // new line (international symbol is an arrow that hooks from mid-top to mid-left)
		// draw vertical component
		MoveTo(cellRight - lineWidth/* break from adjacent characters */, cellTop);
		LineTo(cellRight - lineWidth/* break from adjacent characters */, cellCenter.v);
		// draw horizontal component
		LineTo(cellLeft + lineWidth/* break from adjacent characters */, cellCenter.v);
		// draw top part of arrowhead
		LineTo(cellCenter.h, cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		// draw bottom part of arrowhead
		MoveTo(cellLeft + lineWidth/* break from adjacent characters */, cellCenter.v);
		LineTo(cellCenter.h, cellBottom - INTEGER_HALVED(cellBottom - cellCenter.v));
		break;
	
	case 0x2913: // vertical tab (international symbol is a down-pointing arrow with a terminating line)
		// draw vertical line
		MoveTo(cellCenter.h, cellTop + lineHeight/* break from adjacent characters */);
		LineTo(cellCenter.h, cellBottom - lineHeight/* break from adjacent characters */);
		// draw top part of arrowhead
		LineTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft),
				cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		// draw bottom part of arrowhead
		MoveTo(cellCenter.h, cellBottom - lineHeight/* break from adjacent characters */);
		LineTo(cellRight - INTEGER_HALVED(cellRight - cellCenter.h),
				cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		// draw end line
		MoveTo(cellLeft + INTEGER_HALVED(cellCenter.h - cellLeft), cellBottom - lineHeight/* break from adjacent characters */);
		LineTo(cellRight - INTEGER_HALVED(cellRight - cellCenter.h),
				cellBottom - lineHeight/* break from adjacent characters */);
		break;
	
	case 0x2518: // hook mid-top to mid-left
	case 0x251B: // bold version
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellCenter.h, cellCenter.v);
		LineTo(cellCenter.h, cellTop);
		break;
	
	case 0x255D: // hook mid-top to mid-left, double-line version
		MoveTo(cellLeft, cellCenter.v - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellCenter.v - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellTop);
		MoveTo(cellLeft, cellCenter.v + lineHeight);
		LineTo(cellCenter.h + lineWidth, cellCenter.v + lineHeight);
		LineTo(cellCenter.h + lineWidth, cellTop);
		break;
	
	case 0x2510: // hook mid-left to mid-bottom
	case 0x2513: // bold version
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellCenter.h, cellCenter.v);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		break;
	
	case 0x2557: // hook mid-left to mid-bottom, double-line version
		MoveTo(cellLeft, cellCenter.v - lineHeight);
		LineTo(cellCenter.h + lineWidth, cellCenter.v - lineHeight);
		LineTo(cellCenter.h + lineWidth, cellBottom - lineHeight);
		MoveTo(cellLeft, cellCenter.v + lineHeight);
		LineTo(cellCenter.h - lineWidth, cellCenter.v + lineHeight);
		LineTo(cellCenter.h - lineWidth, cellBottom - lineHeight);
		break;
	
	case 0x250C: // hook mid-right to mid-bottom
	case 0x250F: // bold version
		MoveTo(cellRight - lineWidth, cellCenter.v);
		LineTo(cellCenter.h, cellCenter.v);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		break;
	
	case 0x2554: // hook mid-right to mid-bottom, double-line version
		MoveTo(cellRight - lineWidth, cellCenter.v - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellCenter.v - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellBottom - lineHeight);
		MoveTo(cellRight - lineWidth, cellCenter.v + lineHeight);
		LineTo(cellCenter.h + lineWidth, cellCenter.v + lineHeight);
		LineTo(cellCenter.h + lineWidth, cellBottom - lineHeight);
		break;
	
	case 0x2514: // hook mid-top to mid-right
	case 0x2517: // bold version
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		break;
	
	case 0x255A: // hook mid-top to mid-right, double-line version
		MoveTo(cellCenter.h + lineWidth, cellTop);
		LineTo(cellCenter.h + lineWidth, cellCenter.v - lineHeight);
		LineTo(cellRight - lineWidth, cellCenter.v - lineHeight);
		MoveTo(cellCenter.h - lineWidth, cellTop);
		LineTo(cellCenter.h - lineWidth, cellCenter.v + lineHeight);
		LineTo(cellRight - lineWidth, cellCenter.v + lineHeight);
		break;
	
	case 0x253C: // cross
	case 0x254B: // bold version
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		break;
	
	case 0x256C: // cross, double-line version
		MoveTo(cellLeft, cellCenter.v - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellCenter.v - lineHeight);
		LineTo(cellCenter.h - lineWidth, cellTop);
		MoveTo(cellCenter.h + lineWidth, cellTop);
		LineTo(cellCenter.h + lineWidth, cellCenter.v - lineHeight);
		LineTo(cellRight - lineWidth, cellCenter.v - lineHeight);
		MoveTo(cellLeft, cellCenter.v + lineHeight);
		LineTo(cellCenter.h - lineWidth, cellCenter.v + lineHeight);
		LineTo(cellCenter.h - lineWidth, cellBottom - lineHeight);
		MoveTo(cellRight - lineWidth, cellCenter.v + lineHeight);
		LineTo(cellCenter.h + lineWidth, cellCenter.v + lineHeight);
		LineTo(cellCenter.h + lineWidth, cellBottom - lineHeight);
		break;
	
	case 0x256A: // cross, double-horizontal-only
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		MoveTo(cellLeft, cellCenter.v - lineHeight);
		LineTo(cellRight - lineWidth, cellCenter.v - lineHeight);
		MoveTo(cellLeft, cellCenter.v + lineHeight);
		LineTo(cellRight - lineWidth, cellCenter.v + lineHeight);
		break;
	
	case 0x256B: // cross, double-vertical-only
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		MoveTo(cellCenter.h - lineWidth, cellTop);
		LineTo(cellCenter.h - lineWidth, cellBottom - lineHeight);
		MoveTo(cellCenter.h + lineWidth, cellTop);
		LineTo(cellCenter.h + lineWidth, cellBottom - lineHeight);
		break;
	
	case 0x23BA: // top line
		MoveTo(cellLeft, cellTop);
		LineTo(cellRight - lineWidth, cellTop);
		break;
	
	case 0x23BB: // line between top and middle regions
		MoveTo(cellLeft, cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		LineTo(cellRight - lineWidth, cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		break;
	
	case 0x2500: // middle line
	case 0x2501: // bold version
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		break;
	
	case 0x2550: // middle line, double-line version
		MoveTo(cellLeft, cellCenter.v - lineHeight);
		LineTo(cellRight - lineWidth, cellCenter.v - lineHeight);
		MoveTo(cellLeft, cellCenter.v + lineHeight);
		LineTo(cellRight - lineWidth, cellCenter.v + lineHeight);
		break;
	
	case 0x23BC: // line between middle and bottom regions
		MoveTo(cellLeft, cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		LineTo(cellRight - lineWidth, cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		break;
	
	case 0x2261: // equivalent to (three horizontal lines)
		MoveTo(cellLeft, cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		LineTo(cellRight - lineWidth, cellTop + INTEGER_HALVED(cellCenter.v - cellTop));
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		MoveTo(cellLeft, cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		LineTo(cellRight - lineWidth, cellCenter.v + INTEGER_HALVED(cellBottom - cellCenter.v));
		break;
	
	case 0x23BD: // bottom line
		MoveTo(cellLeft, cellBottom - lineHeight);
		LineTo(cellRight - lineWidth, cellBottom - lineHeight);
		break;
	
	case 0x251C: // cross minus the left piece
	case 0x2523: // bold version
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		MoveTo(cellCenter.h, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		break;
	
	case 0x2524: // cross minus the right piece
	case 0x252B: // bold version
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellCenter.h, cellCenter.v);
		break;
	
	case 0x2534: // cross minus the bottom piece
	case 0x253B: // bold version
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellCenter.v);
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		break;
	
	case 0x252C: // cross minus the top piece
	case 0x2533: // bold version
		MoveTo(cellCenter.h, cellCenter.v);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		MoveTo(cellLeft, cellCenter.v);
		LineTo(cellRight - lineWidth, cellCenter.v);
		break;
	
	case 0x2502: // vertical line
	case 0x2503: // bold version
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		break;
	
	case 0x2551: // vertical line, double-line version
		MoveTo(cellCenter.h - lineWidth, cellTop);
		LineTo(cellCenter.h - lineWidth, cellBottom - lineHeight);
		MoveTo(cellCenter.h + lineWidth, cellTop);
		LineTo(cellCenter.h + lineWidth, cellBottom - lineHeight);
		break;
	
	case 0x2027: // centered dot
	case 0x00B7: // centered dot (alternate?)
	case 0x2219: // bullet (technically bigger than middle dot)
	case 0x25A0: // big centered dot
		MoveTo(cellCenter.h, cellCenter.v);
		Line(0, 0);
		break;
	
	case 0x2320: // integral sign (elongated S), top
		MoveTo(cellCenter.h + lineWidth + lineWidth, cellTop + lineHeight + lineHeight);
		LineTo(cellCenter.h + lineWidth + lineWidth, cellTop);
		LineTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		break;
	
	case 0x2321: // integral sign (elongated S), bottom
		MoveTo(cellCenter.h, cellTop);
		LineTo(cellCenter.h, cellBottom - lineHeight);
		LineTo(cellCenter.h - lineWidth - lineWidth, cellBottom - lineHeight);
		LineTo(cellCenter.h - lineWidth - lineWidth, cellBottom - lineHeight - lineHeight - lineHeight);
		break;
	
	default:
		// non-graphics character
		{
			// this is in Mac Roman encoding
			UInt8	text[] = { inMacRomanForQuickDraw };
			
			
			DrawText(text, 0/* offset */, 1/* character count */); // draw text using current font, size, color, etc.
		}
		break;
	}
	
	// restore font
	TextFont(preservedFontID);
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
static void
eraseSection	(TerminalViewPtr	inTerminalViewPtr,
				 CGContextRef		inDrawingContext,
				 SInt16				inLeftmostColumnToErase,
				 SInt16				inPastRightmostColumnToErase,
				 CGRect&			outRowSectionBounds)
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
		EraseRect(&intBounds);
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
static void
eventNotifyForView	(TerminalViewConstPtr	inPtr,
					 TerminalView_Event		inEvent,
					 void*					inContextPtr)
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

IMPORTANT:  Call releaseRowIterator() when finished with the
			given iterator, in case any resources were
			allocated to create it in the first place. 

(3.0)
*/
static Terminal_LineRef
findRowIterator		(TerminalViewPtr	inTerminalViewPtr,
					 UInt16				inZeroBasedRowIndex)
{
	Terminal_LineRef	result = nullptr;
	// normalize the requested row so that a scrollback line is negative,
	// and all main screen lines are numbered 0 or greater
	SInt16 const		kActualIndex = inTerminalViewPtr->screen.topVisibleEdgeInRows + inZeroBasedRowIndex;
	
	
	// TEMPORARY: this is a lazy implementation and inefficient, but it works
	if (kActualIndex < 0)
	{
		// this call needs to know which scrollback line (0 or greater) to use;
		// the normalized values are negative and use -1 to indicate the first
		// line; so, this is converted by negating the negative and subtracting
		// one so that a normalized -1 becomes 0, -2 becomes 1, etc.
		result = Terminal_NewScrollbackLineIterator(inTerminalViewPtr->screen.ref, -kActualIndex - 1);
	}
	else
	{
		// main screen lines are already zero-based and need no conversion
		result = Terminal_NewMainScreenLineIterator(inTerminalViewPtr->screen.ref, kActualIndex);
	}
	return result;
}// findRowIterator


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
static Boolean
findVirtualCellFromLocalPoint	(TerminalViewPtr		inTerminalViewPtr,
								 Point					inLocalPixelPosition,
								 TerminalView_Cell&		outCell,
								 SInt16&				outDeltaColumn,
								 SInt16&				outDeltaRow)
{
	Boolean		result = true;
	
	
	// NOTE: This code starts in units of pixels for convenience,
	// but must convert to units of columns and rows upon return.
	outCell.first = inLocalPixelPosition.h;
	outCell.second = inLocalPixelPosition.v;
	
	// adjust point to fit in local screen area
	{
		SInt16		offsetH = inLocalPixelPosition.h;
		SInt16		offsetV = inLocalPixelPosition.v;
		
		
		localToScreen(inTerminalViewPtr, &offsetH, &offsetV);
		if (offsetH < 0)
		{
			result = false;
			outDeltaColumn = offsetH;
			outCell.first = 0;
		}
		else
		{
			outCell.first = STATIC_CAST(offsetH, UInt16);
		}
		
		if (offsetV < 0)
		{
			result = false;
			outDeltaRow = offsetV;
			outCell.second = 0;
		}
		else
		{
			outCell.second = STATIC_CAST(offsetV, UInt16);
		}
	}
	
	if (outCell.second > inTerminalViewPtr->screen.viewHeightInPixels)
	{
		result = false;
		outDeltaRow = outCell.second - inTerminalViewPtr->screen.viewHeightInPixels;
		outCell.second = inTerminalViewPtr->screen.viewHeightInPixels;
	}
	
	// TEMPORARY: this needs to take into account double-height text
	outCell.second /= inTerminalViewPtr->text.font.heightPerCharacter;
	outDeltaRow /= inTerminalViewPtr->text.font.heightPerCharacter;
	
	if (outCell.first > inTerminalViewPtr->screen.viewWidthInPixels)
	{
		result = false;
		outDeltaColumn = outCell.first - inTerminalViewPtr->screen.viewWidthInPixels;
		outCell.first = inTerminalViewPtr->screen.viewWidthInPixels;
	}
	
	{
		SInt16 const	kWidthPerCharacter = getRowCharacterWidth(inTerminalViewPtr, outCell.second);
		
		
		outCell.first /= kWidthPerCharacter;
		outDeltaColumn /= kWidthPerCharacter;
	}
	
	outCell.first += inTerminalViewPtr->screen.leftVisibleEdgeInColumns;
	outCell.second += inTerminalViewPtr->screen.topVisibleEdgeInRows;
	
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
Given a stage of blink animation, returns its rendering color.

(4.0)
*/
static inline void
getBlinkAnimationColor	(TerminalViewPtr	inTerminalViewPtr,
						 UInt16				inAnimationStage,
						 RGBColor*			outColorPtr)
{
	CGDeviceColor	deviceColor = inTerminalViewPtr->blinkColors[inAnimationStage];
	Float32			fullIntensityFraction = 0.0;
	
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= deviceColor.red;
	outColorPtr->red = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= deviceColor.green;
	outColorPtr->green = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= deviceColor.blue;
	outColorPtr->blue = STATIC_CAST(fullIntensityFraction, unsigned short);
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
static void
getRowBounds	(TerminalViewPtr	inTerminalViewPtr,
				 UInt16				inZeroBasedRowIndex,
				 Rect*				outBoundsPtr)
{
	SInt16				sectionTopEdge = inZeroBasedRowIndex * inTerminalViewPtr->text.font.heightPerCharacter;
	Terminal_LineRef	rowIterator = nullptr;
	OSStatus			error = noErr;
	HIRect				contentFrame;
	SInt16				topRow = 0;
	
	
	// start with the interior bounds, as this defines two of the edges
	error = HIViewGetBounds(inTerminalViewPtr->contentHIView, &contentFrame);
	assert_noerr(error);
	outBoundsPtr->right = STATIC_CAST(contentFrame.size.width, SInt16);
	outBoundsPtr->left = 0;
	
	// now set the top and bottom edges based on the requested row
	outBoundsPtr->top = sectionTopEdge;
	outBoundsPtr->bottom = sectionTopEdge + inTerminalViewPtr->text.font.heightPerCharacter;
	
	// account for double-height rows
	getVirtualVisibleRegion(inTerminalViewPtr, nullptr/* left */, &topRow, nullptr/* right */, nullptr/* bottom */);
	rowIterator = findRowIterator(inTerminalViewPtr, topRow + inZeroBasedRowIndex);
	if (nullptr != rowIterator)
	{
		TerminalTextAttributes		globalAttributes = 0L;
		Terminal_Result				terminalError = kTerminal_ResultOK;
		
		
		terminalError = Terminal_GetLineGlobalAttributes(inTerminalViewPtr->screen.ref, rowIterator, &globalAttributes);
		if (kTerminal_ResultOK == terminalError)
		{
			if (STYLE_IS_DOUBLE_HEIGHT_TOP(globalAttributes))
			{
				// if this is the top half, the total boundaries extend downwards by one normal line height
				outBoundsPtr->bottom = outBoundsPtr->top + INTEGER_DOUBLED(outBoundsPtr->bottom - outBoundsPtr->top);
			}
			else if (STYLE_IS_DOUBLE_HEIGHT_BOTTOM(globalAttributes))
			{
				// if this is the bottom half, the total boundaries extend upwards by one normal line height
				outBoundsPtr->top = outBoundsPtr->bottom - INTEGER_DOUBLED(outBoundsPtr->bottom - outBoundsPtr->top);
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
static SInt16
getRowCharacterWidth	(TerminalViewPtr	inTerminalViewPtr,
						 UInt16				inLineNumber)
{
	TerminalTextAttributes	globalAttributes = 0L;
	Terminal_LineRef		rowIterator = nullptr;
	SInt16					result = inTerminalViewPtr->text.font.widthPerCharacter;
	
	
	rowIterator = findRowIterator(inTerminalViewPtr, inLineNumber);
	if (rowIterator != nullptr)
	{
		(Terminal_Result)Terminal_GetLineGlobalAttributes(inTerminalViewPtr->screen.ref, rowIterator, &globalAttributes);
		if (STYLE_IS_DOUBLE_ANY(globalAttributes)) result = INTEGER_DOUBLED(result);
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
static void
getRowSectionBounds		(TerminalViewPtr	inTerminalViewPtr,
						 UInt16				inZeroBasedRowIndex,
						 UInt16				inZeroBasedStartingColumnNumber,
						 SInt16				inCharacterCount,
						 Rect*				outBoundsPtr)
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
static inline void
getScreenBaseColor	(TerminalViewPtr			inTerminalViewPtr,
					 TerminalView_ColorIndex	inColorEntryNumber,
					 RGBColor*					outColorPtr)
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
static void
getScreenColorsForAttributes	(TerminalViewPtr			inTerminalViewPtr,
								 TerminalTextAttributes		inAttributes,
								 RGBColor*					outForeColorPtr,
								 RGBColor*					outBackColorPtr,
								 Boolean*					outNoBackgroundPtr)
{
	Boolean		isCustom = false;
	
	
	*outNoBackgroundPtr = false; // initially...
	
	// choose foreground color
	isCustom = false; // initially...
	if (/*(inTerminalViewPtr->screen.areANSIColorsEnabled) && */STYLE_USE_FOREGROUND_INDEX(inAttributes))
	{
		// one of the “core” 256 colors was chosen
		UInt16		fg = kTerminalView_ColorIndexNormalANSIBlack + STYLE_FOREGROUND_INDEX(inAttributes);
		
		
		if (STYLE_BOLD(inAttributes) && (fg <= kTerminalView_ColorIndexNormalANSIWhite))
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
	if (false == isCustom)
	{
		// ordinary color, based on style
		TerminalView_ColorIndex		fg = kTerminalView_ColorIndexNormalText;
		
		
		// the blinking text color is favored because MacTelnet 3.0 has
		// “real” bold text; therefore, if some text happens to be both
		// boldface and blinking, using the blinking text color ensures
		// the text will still be recognizeable as boldface
		if (STYLE_BLINKING(inAttributes)) fg = kTerminalView_ColorIndexBlinkingText;
		else if (STYLE_BOLD(inAttributes)) fg = kTerminalView_ColorIndexBoldText;
		else fg = kTerminalView_ColorIndexNormalText;
		
		getScreenCustomColor(inTerminalViewPtr, fg, outForeColorPtr);
	}
	
	// choose background color
	isCustom = false; // initially...
	if (/*(inTerminalViewPtr->screen.areANSIColorsEnabled) && */STYLE_USE_BACKGROUND_INDEX(inAttributes))
	{
		// one of the “core” 256 colors was chosen
		UInt16		bg = kTerminalView_ColorIndexNormalANSIBlack + STYLE_BACKGROUND_INDEX(inAttributes);
		
		
		if (STYLE_BOLD(inAttributes) && (bg <= kTerminalView_ColorIndexNormalANSIWhite))
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
	if (false == isCustom)
	{
		// ordinary color, based on style
		TerminalView_ColorIndex		bg = kTerminalView_ColorIndexNormalBackground;
		
		
		// the blinking text color is favored because MacTelnet 3.0 has
		// “real” bold text; therefore, if some text happens to be both
		// boldface and blinking, using the blinking text color ensures
		// the text will still be recognizeable as boldface
		if (STYLE_BLINKING(inAttributes)) bg = kTerminalView_ColorIndexBlinkingBackground;
		else if (STYLE_BOLD(inAttributes)) bg = kTerminalView_ColorIndexBoldBackground;
		else
		{
			bg = kTerminalView_ColorIndexNormalBackground;
			*outNoBackgroundPtr = true;
		}
		getScreenCustomColor(inTerminalViewPtr, bg, outBackColorPtr);
	}
	
	// to invert, swap the colors and make sure the background is drawn
	if (STYLE_INVERSE_VIDEO(inAttributes))
	{
		std::swap< RGBColor >(*outForeColorPtr, *outBackColorPtr);
		*outNoBackgroundPtr = false;
	}
	
	// if the entire screen is in reverse video mode, it currently
	// cannot let “matching” background colors fall through (because
	// the background still renders the ordinary color)
	if (inTerminalViewPtr->screen.isReverseVideo)
	{
		*outNoBackgroundPtr = false;
	}
	
	if (STYLE_CONCEALED(inAttributes)) *outForeColorPtr = *outBackColorPtr; // make “invisible” by using same colors for everything
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
static inline Boolean
getScreenCoreColor	(TerminalViewPtr	inTerminalViewPtr,
					 UInt16				inColorEntryNumber,
					 RGBColor*			outColorPtr)
{
	My_CGColorByIndex&	colors = inTerminalViewPtr->coreColors;
	My_XTerm256Table&	sourceGrid = gColorGrid();
	Boolean				result = false;
	
	
	if (colors.end() != colors.find(inColorEntryNumber))
	{
		// one of the basic 16 colors
		CGDeviceColor	deviceColor = colors[inColorEntryNumber];
		Float32			fullIntensityFraction = 0.0;
		
		
		fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
		fullIntensityFraction *= deviceColor.red;
		outColorPtr->red = STATIC_CAST(fullIntensityFraction, unsigned short);
		
		fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
		fullIntensityFraction *= deviceColor.green;
		outColorPtr->green = STATIC_CAST(fullIntensityFraction, unsigned short);
		
		fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
		fullIntensityFraction *= deviceColor.blue;
		outColorPtr->blue = STATIC_CAST(fullIntensityFraction, unsigned short);
		
		result = true;
	}
	else if (sourceGrid.colorLevels.end() !=
				sourceGrid.colorLevels.find(inColorEntryNumber))
	{
		// one of the many other standard colors
		//Console_WriteValueFloat4("color", sourceGrid.colorLevels[inColorEntryNumber][0],
		//							sourceGrid.colorLevels[inColorEntryNumber][1],
		//							sourceGrid.colorLevels[inColorEntryNumber][2],
		//							0);
		sourceGrid.makeRGBColor(sourceGrid.colorLevels[inColorEntryNumber][0],
								sourceGrid.colorLevels[inColorEntryNumber][1],
								sourceGrid.colorLevels[inColorEntryNumber][2], *outColorPtr);
		result = true;
	}
	else if (sourceGrid.grayLevels.end() !=
				sourceGrid.grayLevels.find(inColorEntryNumber))
	{
		// one of the standard grays
		//Console_WriteValue("gray", sourceGrid.grayLevels[inColorEntryNumber]);
		sourceGrid.makeRGBColor(sourceGrid.grayLevels[inColorEntryNumber],
								sourceGrid.grayLevels[inColorEntryNumber],
								sourceGrid.grayLevels[inColorEntryNumber], *outColorPtr);
		result = true;
	}
	return result;
}// getScreenCoreColor


/*!
Given a color identifier, returns its rendering color.

See also getScreenCoreColor().

(3.0)
*/
static inline void
getScreenCustomColor	(TerminalViewPtr			inTerminalViewPtr,
						 TerminalView_ColorIndex	inColorEntryNumber,
						 RGBColor*					outColorPtr)
{
	CGDeviceColor	deviceColor = inTerminalViewPtr->customColors[inColorEntryNumber];
	Float32			fullIntensityFraction = 0.0;
	
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= deviceColor.red;
	outColorPtr->red = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= deviceColor.green;
	outColorPtr->green = STATIC_CAST(fullIntensityFraction, unsigned short);
	
	fullIntensityFraction = RGBCOLOR_INTENSITY_MAX;
	fullIntensityFraction *= deviceColor.blue;
	outColorPtr->blue = STATIC_CAST(fullIntensityFraction, unsigned short);
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
static void
getScreenOrigin		(TerminalViewPtr	inTerminalViewPtr,
					 SInt16*			outScreenPositionX,
					 SInt16*			outScreenPositionY)
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
static void
getScreenOriginFloat	(TerminalViewPtr	inTerminalViewPtr,
						 Float32&			outScreenPositionX,
						 Float32&			outScreenPositionY)
{
	HIViewWrap	windowContentView(kHIViewWindowContentID, HIViewGetWindow(inTerminalViewPtr->contentHIView));
	HIRect		contentFrame;
	OSStatus	error = noErr;
	
	
	assert(windowContentView.exists());
	error = HIViewGetFrame(inTerminalViewPtr->contentHIView, &contentFrame);
	assert_noerr(error);
	error = HIViewConvertRect(&contentFrame, HIViewGetSuperview(inTerminalViewPtr->contentHIView), windowContentView);
	outScreenPositionX = contentFrame.origin.x;
	outScreenPositionY = contentFrame.origin.y;
}// getScreenOriginFloat


/*!
Internal version of TerminalView_ReturnSelectedTextAsNewHandle().

DEPRECATED.  Use returnSelectedTextAsNewUnicode() instead.

(3.0)
*/
static Handle
getSelectedTextAsNewHandle	(TerminalViewPtr			inTerminalViewPtr,
							 UInt16						inNumberOfSpacesToReplaceWithOneTabOrZero,
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
			Terminal_Result			copyResult = kTerminal_ResultOK;
			Terminal_LineRef		lineIterator = findRowIterator(inTerminalViewPtr, kSelectionStart.second);
			Terminal_TextCopyFlags	flags = 0L;
			char*					characters = nullptr;
			SInt32					actualLength = 0L;
			
			
			HLock(result);
			characters = *result;
			
			if (inTerminalViewPtr->text.selection.isRectangular) flags |= kTerminal_TextCopyFlagsRectangular;
			
			copyResult = Terminal_CopyRange(inTerminalViewPtr->screen.ref, lineIterator,
											kSelectionPastEnd.second - kSelectionStart.second,
											kSelectionStart.first, kSelectionPastEnd.first - 1/* make inclusive range */,
											characters, kByteCount, &actualLength,
											(inFlags & kTerminalView_TextFlagInline) ? "" : "\015",
											inNumberOfSpacesToReplaceWithOneTabOrZero, flags);
			releaseRowIterator(inTerminalViewPtr, &lineIterator);
			HUnlock(result);
			
			// signal fatal errors with a nullptr handle; a lack of
			// space is not considered fatal, instead the truncated
			// string is returned to the caller
			if ((copyResult == kTerminal_ResultOK) ||
				(copyResult == kTerminal_ResultNotEnoughMemory))
			{
				(OSStatus)Memory_SetHandleSize(result, actualLength * sizeof(char));
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
static HIShapeRef
getSelectedTextAsNewHIShape		(TerminalViewPtr	inTerminalViewPtr,
								 Float32			inInsets)
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
static RgnHandle
getSelectedTextAsNewRegion		(TerminalViewPtr	inTerminalViewPtr)
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
static RgnHandle
getSelectedTextAsNewRegionOnScreen		(TerminalViewPtr	inTerminalViewPtr)
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
static size_t
getSelectedTextSize		(TerminalViewPtr	inTerminalViewPtr)
{
    size_t		result = 0L;
	
	
	if (inTerminalViewPtr->text.selection.exists)
	{
		TerminalView_Cell const&	kSelectionStart = inTerminalViewPtr->text.selection.range.first;
		TerminalView_Cell const&	kSelectionPastEnd = inTerminalViewPtr->text.selection.range.second;
		// for rectangular selections, the size also includes space for one new-line per row
		UInt16 const				kRowWidth = (inTerminalViewPtr->text.selection.isRectangular)
												? INTEGER_ABSOLUTE(kSelectionPastEnd.first - kSelectionStart.first + 1/* size of new-line */)
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
static HIShapeRef
getVirtualRangeAsNewHIShape		(TerminalViewPtr			inTerminalViewPtr,
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
		SInt16		top = 0;
		SInt16		left = 0;
		
		
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
		if (inInsets)
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
												inTerminalViewPtr->screen.viewWidthInPixels -
													selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter -
													2.0 * inInsets,
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
												selectionPastEnd.first * inTerminalViewPtr->text.font.widthPerCharacter - 2.0 * inInsets,
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
												inTerminalViewPtr->screen.viewWidthInPixels - 2.0 * inInsets,
												(selectionPastEnd.second - selectionStart.second - 2/* skip first and last lines */) *
													inTerminalViewPtr->text.font.heightPerCharacter - 2.0 * inInsets);
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
static RgnHandle
getVirtualRangeAsNewRegion		(TerminalViewPtr			inTerminalViewPtr,
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
static RgnHandle
getVirtualRangeAsNewRegionOnScreen	(TerminalViewPtr			inTerminalViewPtr,
									 TerminalView_Cell const&	inSelectionStart,
									 TerminalView_Cell const&	inSelectionPastEnd,
									 Boolean					inIsRectangular)
{
	RgnHandle	result = Memory_NewRegion();
	
	
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
		SetRect(&screenBounds, 0, 0, STATIC_CAST(floatBounds.size.width, SInt16), STATIC_CAST(floatBounds.size.height, SInt16));
		
		selectionStart = inSelectionStart;
		selectionPastEnd = inSelectionPastEnd;
		
		// normalize coordinates with respect to visible area of virtual screen
		{
			SInt16		top = 0;
			SInt16		left = 0;
			
			
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
			SetRect(&selectionBounds,
					selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter,
					selectionStart.second * inTerminalViewPtr->text.font.heightPerCharacter,
					selectionPastEnd.first * inTerminalViewPtr->text.font.widthPerCharacter,
					selectionPastEnd.second * inTerminalViewPtr->text.font.heightPerCharacter);
			
			// the final selection region is the portion of the full rectangle
			// that fits within the current screen boundaries
			SectRect(&selectionBounds, &screenBounds, &clippedRect);
			RectRgn(result, &clippedRect);
		}
		else
		{
			// then the area to be highlighted is irregularly shaped; this is more complex...
			RgnHandle	clippedRegion = Memory_NewRegion();
			
			
			if (nullptr != clippedRegion)
			{
				Rect	clippedRect;
				Rect	partialSelectionBounds;
				
				
				// make the points the “right way around”, in case the first point is
				// technically to the right of or below the second point
				sortAnchors(selectionStart, selectionPastEnd, false/* is a rectangular selection */);
				
				// bounds of first (possibly partial) line to be highlighted
				SetRect(&partialSelectionBounds,
						selectionStart.first * inTerminalViewPtr->text.font.widthPerCharacter,
						selectionStart.second * inTerminalViewPtr->text.font.heightPerCharacter,
						inTerminalViewPtr->screen.viewWidthInPixels,
						(selectionStart.second + 1) * inTerminalViewPtr->text.font.heightPerCharacter);
				SectRect(&partialSelectionBounds, &screenBounds, &clippedRect); // clip to constraint rectangle
				RectRgn(result, &clippedRect);
				
				// bounds of last (possibly partial) line to be highlighted
				SetRect(&partialSelectionBounds,
						0,
						(selectionPastEnd.second - 1) * inTerminalViewPtr->text.font.heightPerCharacter,
						selectionPastEnd.first * inTerminalViewPtr->text.font.widthPerCharacter,
						selectionPastEnd.second * inTerminalViewPtr->text.font.heightPerCharacter);
				SectRect(&partialSelectionBounds, &screenBounds, &clippedRect); // clip to constraint rectangle
				RectRgn(clippedRegion, &clippedRect);
				UnionRgn(clippedRegion, result, result);
				
				if ((selectionPastEnd.second - selectionStart.second) > 2)
				{
					// highlight extends across more than two lines - fill in the space in between
					SetRect(&partialSelectionBounds,
							0,
							(selectionStart.second + 1) * inTerminalViewPtr->text.font.heightPerCharacter,
							inTerminalViewPtr->screen.viewWidthInPixels,
							(selectionPastEnd.second - 1) * inTerminalViewPtr->text.font.heightPerCharacter);
					SectRect(&partialSelectionBounds, &screenBounds, &clippedRect); // clip to constraint rectangle
					RectRgn(clippedRegion, &clippedRect);
					UnionRgn(clippedRegion, result, result);
				}
				
				Memory_DisposeRegion(&clippedRegion);
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
static void
getVirtualVisibleRegion		(TerminalViewPtr	inTerminalViewPtr,
							 SInt16*			outLeftColumnOrNull,
							 SInt16*			outTopRowOrNull,
							 SInt16*			outPastTheRightColumnOrNull,
							 SInt16*			outPastTheBottomRowOrNull)
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
static void
handleMultiClick	(TerminalViewPtr	inTerminalViewPtr,
					 UInt16				inClickCount)													
{
	TerminalView_Cell	selectionStart;
	TerminalView_Cell	selectionPastEnd;
	SInt16 const		kColumnCount = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	SInt16 const		kRowCount = Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
	
	
	selectionStart = inTerminalViewPtr->text.selection.range.first;
	selectionPastEnd = inTerminalViewPtr->text.selection.range.second;
	
	// all multi-clicks result in a selection that is one line high,
	// and the range is exclusive so the row difference must be 1
	selectionPastEnd.second = selectionStart.second + 1;
	
	if (inClickCount == 2)
	{
		SInt16 const				kNumberOfSpacesPerTab = 0; // zero means “do not substitute tabs”
		Terminal_LineRef			lineIterator = findRowIterator(inTerminalViewPtr, selectionStart.second);
		Terminal_TextCopyFlags		flags = 0L;
		char						theChar[5];
		
		
		// configure range copying routine
		if (inTerminalViewPtr->text.selection.isRectangular) flags |= kTerminal_TextCopyFlagsRectangular;
		
		// double-click - select a word; or, do intelligent double-click
		// based on the character underneath the cursor
		if (kTerminal_ResultOK ==
			Terminal_CopyLineRange(inTerminalViewPtr->screen.ref, lineIterator,
									inTerminalViewPtr->text.selection.range.first.first,
									inTerminalViewPtr->text.selection.range.first.first,
									theChar, 1L/* maximum characters to return */, nullptr/* actual length */,
									kNumberOfSpacesPerTab))
		{
			SInt32		actualLength = 0L;
			Boolean		foundEnd = false;
			
			
			//
			// IMPORTANT: The current line iterator is cached in the "lineIterator"
			// variable, and should be reset if the selection line ever changes below.
			//
			
			// normal word selection mode; scan to the right and left
			// of the click location, constrained to a single line
			for (foundEnd = false; !foundEnd; )
			{
				if (selectionPastEnd.first >= kColumnCount)
				{
					// wrap to next line
					releaseRowIterator(inTerminalViewPtr, &lineIterator);
					++selectionPastEnd.second;
					foundEnd = (selectionPastEnd.second >= kRowCount);
					if (foundEnd)
					{
						selectionPastEnd.second = kRowCount;
					}
					else
					{
						selectionPastEnd.first = 0;
					}
					lineIterator = findRowIterator(inTerminalViewPtr, selectionPastEnd.second);
				}
				unless (foundEnd)
				{
					actualLength = 0L;
					
					// copy a single character and examine it
					if (kTerminal_ResultOK ==
						Terminal_CopyLineRange(inTerminalViewPtr->screen.ref, lineIterator,
												selectionPastEnd.first, selectionPastEnd.first,
												theChar, 1L/* maximum characters to return */, &actualLength,
												kNumberOfSpacesPerTab))
					{
						foundEnd = ((actualLength == 0) || CPP_STD::isspace(*theChar));
						unless (foundEnd)
						{
							++selectionPastEnd.first;
						}
					}
					else
					{
						// read error...break now
						foundEnd = true;
					}
				}
			}
			// note that because the start of the range is inclusive
			// and the end is exclusive, the left-scan loop below is
			// not quite similar to the right-scan loop above
			for (foundEnd = false; !foundEnd; )
			{
				actualLength = 0L;
				
				// copy a single character and examine it
				if (kTerminal_ResultOK ==
					Terminal_CopyLineRange(inTerminalViewPtr->screen.ref, lineIterator,
											selectionStart.first, selectionStart.first,
											theChar, 1L/* maximum characters to return */, &actualLength,
											kNumberOfSpacesPerTab))
				{
					foundEnd = ((actualLength == 0) || CPP_STD::isspace(*theChar));
				}
				else
				{
					// read error...break now
					foundEnd = true;
				}
				
				if (foundEnd)
				{
					// with an inclusive range, the discovered space should be ignored
					++selectionStart.first;
					if (selectionStart.first >= kColumnCount)
					{
						// also ignore the shift to the previous line, if it ended with a space
						releaseRowIterator(inTerminalViewPtr, &lineIterator);
						selectionStart.first = 0;
						++selectionStart.second;
						lineIterator = findRowIterator(inTerminalViewPtr, selectionStart.second);
					}
				}
				else
				{
					if (selectionStart.first == 0)
					{
						// wrap to previous line
						releaseRowIterator(inTerminalViewPtr, &lineIterator);
						--selectionStart.second;
						foundEnd = (selectionStart.second <= 0);
						if (foundEnd)
						{
							selectionStart.second = 0;
						}
						else
						{
							selectionStart.first = kColumnCount - 1;
						}
						lineIterator = findRowIterator(inTerminalViewPtr, selectionStart.second);
					}
					else
					{
						// move to previous character on same line
						--selectionStart.first;
					}
				}
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
}// handleMultiClick


/*!
Responds to terminal view size or position changes by
updating sub-views.

(3.0)
*/
static void
handleNewViewContainerBounds	(HIViewRef		inHIView,
								 Float32		UNUSED_ARGUMENT(inDeltaX),
								 Float32		UNUSED_ARGUMENT(inDeltaY),
								 void*			inTerminalViewRef)
{
	TerminalViewRef			view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
	HIRect					terminalViewBounds;
	
	
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
			if (viewPtr->screen.viewWidthInPixels > 0)
			{
				UInt16		loopGuard = 0;
				
				
				while ((CharWidth('A') * visibleColumns) < kMaximumViewWidth)
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
			if (viewPtr->screen.viewHeightInPixels > 0)
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
		else
		{
			UInt16		columns = 0;
			UInt16		rows = 0;
			
			
			// normal mode; resize the underlying terminal screen
			TerminalView_GetTheoreticalScreenDimensions(view, STATIC_CAST(kMaximumViewWidth, UInt16),
														STATIC_CAST(kMaximumViewHeight, UInt16),
														&columns, &rows);
			Terminal_SetVisibleColumnCount(viewPtr->screen.ref, columns);
			Terminal_SetVisibleRowCount(viewPtr->screen.ref, rows);
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
		HIRect			terminalFocusFrame = CGRectMake(0, 0, kPadLeft + viewPtr->screen.viewWidthInPixels + kPadRight,
														kPadTop + viewPtr->screen.viewHeightInPixels + kPadBottom);
		HIRect			terminalInteriorFrame = CGRectMake(kPadLeft, kPadTop, viewPtr->screen.viewWidthInPixels,
															viewPtr->screen.viewHeightInPixels);
		
		
		// TEMPORARY: this should in fact respect margin values too
		RegionUtilities_CenterHIRectIn(terminalFocusFrame, terminalViewBounds);
		HIViewSetFrame(viewPtr->focusAndPaddingHIView, &terminalFocusFrame);
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
		setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds);
	}
}// handleNewViewContainerBounds


/*!
Like highlightVirtualRange(), but automatically uses
the current text selection anchors for the specified
terminal view.

(3.0)
*/
static inline void
highlightCurrentSelection	(TerminalViewPtr	inTerminalViewPtr,
							 Boolean			inIsHighlighted,
							 Boolean			inRedraw)
{
#if 0
	Console_WriteValueFloat4("Selection range",
								inTerminalViewPtr->text.selection.range.first.first,
								inTerminalViewPtr->text.selection.range.first.second,
								inTerminalViewPtr->text.selection.range.second.first,
								inTerminalViewPtr->text.selection.range.second.second);
#endif
	highlightVirtualRange(inTerminalViewPtr, inTerminalViewPtr->text.selection.range,
							kTerminalTextAttributeSelected, inIsHighlighted, inRedraw);
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
static void
highlightVirtualRange	(TerminalViewPtr				inTerminalViewPtr,
						 TerminalView_CellRange const&	inRange,
						 TerminalTextAttributes			inHighlightingStyle,
						 Boolean						inIsHighlighted,
						 Boolean						inRedraw)
{
	TerminalView_CellRange		orderedRange = inRange;
	
	
	// require beginning point to be “earlier” than the end point; swap points if not
	sortAnchors(orderedRange.first, orderedRange.second, inTerminalViewPtr->text.selection.isRectangular);
	
	// modify selection attributes
	{
		Terminal_LineRef	lineIterator = findRowIterator
											(inTerminalViewPtr, orderedRange.first.second);
		UInt16 const		kNumberOfRows = orderedRange.second.second - orderedRange.first.second;
		
		
		if (nullptr != lineIterator)
		{
			(Terminal_Result)Terminal_ChangeRangeAttributes
								(inTerminalViewPtr->screen.ref, lineIterator, kNumberOfRows,
									orderedRange.first.first, orderedRange.second.first,
									inTerminalViewPtr->text.selection.isRectangular,
									(inIsHighlighted) ? inHighlightingStyle : kNoTerminalTextAttributes/* attributes to set */,
									(inIsHighlighted) ? kNoTerminalTextAttributes : inHighlightingStyle/* attributes to clear */);
			releaseRowIterator(inTerminalViewPtr, &lineIterator);
		}
	}
	
	if (inRedraw)
	{
		// perform a boundary check, since drawSection() doesn’t do one
		{
			SInt16		startColumn = 0;
			SInt16		pastTheEndColumn = 0;
			SInt16		startRow = 0;
			SInt16		pastTheEndRow = 0;
			
			
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
			if (orderedRange.first.second < 0)
			{
				orderedRange.first.second = 0;
			}
		}
		
		// redraw affected area; for rectangular selections it is only
		// necessary to redraw the actual range, but for normal selections
		// (which primarily consist of full-width highlighting), the entire
		// width in the given row range is redrawn
		{
			UInt16 const	kFirstChar = (inTerminalViewPtr->text.selection.isRectangular)
											? orderedRange.first.first
											: 0;
			UInt16 const	kPastLastChar = (inTerminalViewPtr->text.selection.isRectangular)
											? orderedRange.second.first
											: Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
			
			
			for (UInt16 rowIndex = orderedRange.first.second;
					rowIndex < orderedRange.second.second; ++rowIndex)
			{
				invalidateRowSection(inTerminalViewPtr, rowIndex,
										kFirstChar, kPastLastChar - kFirstChar/* count */);
			}
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

MacTelnet maintains invalid regions in terms of logical
sections of the terminal, so that the right area is
always redrawn even after scrolling, font size changes,
etc.

To force an area to render immediately, use the routine
TerminalView_DrawRowSection().

(3.0)
*/
static void
invalidateRowSection	(TerminalViewPtr	inTerminalViewPtr,
						 UInt16				inLineNumber,
						 UInt16				inStartingColumnNumber,
						 UInt16				inCharacterCount)
{
  	Rect	textBounds;
	
	
	// mark the specified area; it is already in “screen coordinates”,
	// which match the view coordinates used by HIViewSetNeedsDisplayInRegion()
	getRowSectionBounds(inTerminalViewPtr, inLineNumber, inStartingColumnNumber, inCharacterCount, &textBounds);
	RectRgn(gInvalidationScratchRegion(), &textBounds);
	(OSStatus)HIViewSetNeedsDisplayInRegion(inTerminalViewPtr->contentHIView, gInvalidationScratchRegion(), true);
}// invalidateRowSection


/*!
Determines if a font is monospaced.

(3.0)
*/
static Boolean
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
static void
localToScreen	(TerminalViewPtr	inTerminalViewPtr,
				 SInt16*			inoutHorizontalPixelOffsetFromPortOrigin,
				 SInt16*			inoutVerticalPixelOffsetFromPortOrigin)
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
static Boolean
mainEventLoopEvent	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inEventThatOccurred,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Boolean		result = false;
	
	
	switch (inEventThatOccurred)
	{
	case kEventLoop_GlobalEventSuspendResume:
		// update the internal variable to reflect the current suspended state of the application
		gApplicationIsSuspended = FlagManager_Test(kFlagSuspended);
		{
			// redraw every screen that has a text selection, because these can look different in the background
			SessionFactory_ForEveryTerminalWindowDo(redrawScreensTerminalWindowOp, nullptr/* data 1 - unused */,
													0L/* data 2 - unused */, nullptr/* result - unused */);
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
static pascal void
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
				size_t	actualSize = 0L;
				
				
				// get the user’s Capture File Creator preference, if possible
				unless (Preferences_GetData(kPreferences_TagCaptureFileCreator,
											sizeof(captureFileCreator), &captureFileCreator, &actualSize) ==
						kPreferences_ResultOK)
				{
					captureFileCreator = 'ttxt'; // default to SimpleText if a preference can’t be found
				}
				
				// create a temporary file for “safe” saving, in addition to the user’s requested file
				error = FileSelectionDialogs_CreateOrFindUserSaveFile
						(reply, captureFileCreator, 'TEXT', saveFile, temporaryFile);
				if (error == noErr)
				{
					Handle		textHandle = nullptr;
					Boolean		result = false;
					
					
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
							
							error = FSClose(fileRefNum), fileRefNum = -1;
							
							// finally, “swap” the new file into the right place on disk
							error = FSExchangeObjects(&temporaryFile, &saveFile);
							if (noErr == error)
							{
								(OSStatus)FSDeleteObject(&temporaryFile);
							}
							
							result = true;
						}
					}
				}
			}
			Alert_ReportOSStatus(error);
			error = FileSelectionDialogs_CompleteSave(&reply);
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
specified number of pixels; if positive, the display would
move left, otherwise it would move right.  No redrawing is
done, however; the change will take effect the next time
the screen is redrawn.

(3.0)
*/
static void
offsetLeftVisibleEdge	(TerminalViewPtr	inTerminalViewPtr,
						 SInt16				inDeltaInPixels)
{
	SInt16 const	kMinimum = 0;
	SInt16 const	kMaximum = 0/*Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref) - 1*/;
	SInt16 const	kOldDiscreteValue = inTerminalViewPtr->screen.leftVisibleEdgeInColumns;
	SInt16			pixelValue = inTerminalViewPtr->screen.leftVisibleEdgeInPixels;
	SInt16			discreteValue = kOldDiscreteValue;
	
	
	// TEMPORARY; theoretically these could take into account which rows have double-sized text
	
	pixelValue += inDeltaInPixels;
	pixelValue = INTEGER_MAXIMUM(kMinimum * inTerminalViewPtr->text.font.widthPerCharacter, pixelValue);
	pixelValue = INTEGER_MINIMUM(kMaximum * inTerminalViewPtr->text.font.widthPerCharacter, pixelValue);
	
	discreteValue = pixelValue / inTerminalViewPtr->text.font.widthPerCharacter;
	
	inTerminalViewPtr->screen.leftVisibleEdgeInPixels = pixelValue;
	inTerminalViewPtr->screen.leftVisibleEdgeInColumns = discreteValue;
	
	inTerminalViewPtr->text.selection.range.first.first += (discreteValue - kOldDiscreteValue);
	inTerminalViewPtr->text.selection.range.second.first += (discreteValue - kOldDiscreteValue);
}// offsetLeftVisibleEdge


/*!
Changes the top visible edge of the terminal view by the
specified number of pixels; if positive, the display would
move up, otherwise it would move down.  No redrawing is
done, however; the change will take effect the next time
the screen is redrawn.

(3.0)
*/
static void
offsetTopVisibleEdge	(TerminalViewPtr	inTerminalViewPtr,
						 SInt16				inDeltaInPixels)
{
	SInt16 const	kMinimum = -Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref);
	SInt16 const	kMaximum = 0/*Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref) - 1*/;
	SInt16 const	kOldDiscreteValue = inTerminalViewPtr->screen.topVisibleEdgeInRows;
	SInt16			pixelValue = inTerminalViewPtr->screen.topVisibleEdgeInPixels;
	SInt16			discreteValue = kOldDiscreteValue;
	
	
	// TEMPORARY; theoretically these could take into account which rows have double-sized text
	
	pixelValue += inDeltaInPixels;
	pixelValue = INTEGER_MAXIMUM(kMinimum * inTerminalViewPtr->text.font.heightPerCharacter, pixelValue);
	pixelValue = INTEGER_MINIMUM(kMaximum * inTerminalViewPtr->text.font.heightPerCharacter, pixelValue);
	
	discreteValue = pixelValue / inTerminalViewPtr->text.font.heightPerCharacter;
	
	inTerminalViewPtr->screen.topVisibleEdgeInPixels = pixelValue;
	inTerminalViewPtr->screen.topVisibleEdgeInRows = discreteValue;
	
	inTerminalViewPtr->text.selection.range.first.second += (discreteValue - kOldDiscreteValue);
	inTerminalViewPtr->text.selection.range.second.second += (discreteValue - kOldDiscreteValue);
	
	// hide the cursor while the main screen is not showing
	setCursorVisibility(inTerminalViewPtr, (inTerminalViewPtr->screen.topVisibleEdgeInRows >= 0));
}// offsetTopVisibleEdge


/*!
Invoked whenever a monitored preference value is changed
(see TerminalView_Init() to see which preferences are
monitored).  This routine responds by ensuring that internal
variables are up to date for the changed preference.

(3.0)
*/
static void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	size_t	actualSize = 0L;
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagCursorBlinks:
		// update global variable with current preference value
		unless (Preferences_GetData(kPreferences_TagCursorBlinks, sizeof(gPreferenceProxies.cursorBlinks),
									&gPreferenceProxies.cursorBlinks, &actualSize) ==
				kPreferences_ResultOK)
		{
			gPreferenceProxies.cursorBlinks = false; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagDontDimBackgroundScreens:
		// update global variable with current preference value
		unless (Preferences_GetData(kPreferences_TagDontDimBackgroundScreens, sizeof(gPreferenceProxies.dontDimTerminals),
									&gPreferenceProxies.dontDimTerminals, &actualSize) ==
				kPreferences_ResultOK)
		{
			gPreferenceProxies.dontDimTerminals = false; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagNotifyOfBeeps:
		// update global variable with current preference value
		unless (Preferences_GetData(kPreferences_TagNotifyOfBeeps, sizeof(gPreferenceProxies.notifyOfBeeps),
									&gPreferenceProxies.notifyOfBeeps, &actualSize) ==
				kPreferences_ResultOK)
		{
			gPreferenceProxies.notifyOfBeeps = false; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagPureInverse:
		// update global variable with current preference value
		unless (Preferences_GetData(kPreferences_TagPureInverse, sizeof(gPreferenceProxies.invertSelections),
									&gPreferenceProxies.invertSelections, &actualSize) ==
				kPreferences_ResultOK)
		{
			gPreferenceProxies.invertSelections = false; // assume a value, if preference can’t be found
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
static void
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
	TerminalViewAutoLocker		viewPtr(gTerminalViewPtrLocks(), view);
	
	
	// ignore changes to settings that are part of the view’s filter
	unless (viewPtr->configFilter.end() != viewPtr->configFilter.find(inPreferenceTagThatChanged))
	{
		switch (inPreferenceTagThatChanged)
		{
		case kPreferences_TagTerminalCursorType:
			// recalculate cursor boundaries for the specified view
			{
				Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
				UInt16				cursorX = 0;
				UInt16				cursorY = 0;
				
				
				// invalidate the entire old cursor region (in case it is bigger than the new one)
				RectRgn(gInvalidationScratchRegion(), &viewPtr->screen.cursor.bounds);
				if (IsValidControlHandle(viewPtr->contentHIView))
				{
					(OSStatus)HIViewSetNeedsDisplayInRegion(viewPtr->contentHIView, gInvalidationScratchRegion(), true);
				}
				
				// find the new cursor region
				getCursorLocationError = Terminal_CursorGetLocation(viewPtr->screen.ref, &cursorX, &cursorY);
				setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds);
				
				// invalidate the new cursor region (in case it is bigger than the old one)
				RectRgn(gInvalidationScratchRegion(), &viewPtr->screen.cursor.bounds);
				if (IsValidControlHandle(viewPtr->contentHIView))
				{
					(OSStatus)HIViewSetNeedsDisplayInRegion(viewPtr->contentHIView, gInvalidationScratchRegion(), true);
				}
			}
			break;
		
		case kPreferences_TagTerminalResizeAffectsFontSize:
			// change the display mode for the specified view
			{
				size_t		actualSize = 0;
				Boolean		resizeAffectsFont = false;
				
				
				unless (Preferences_GetData(kPreferences_TagTerminalResizeAffectsFontSize, sizeof(resizeAffectsFont),
											&resizeAffectsFont, &actualSize) ==
						kPreferences_ResultOK)
				{
					resizeAffectsFont = false; // assume a value, if preference can’t be found
				}
				(TerminalView_Result)TerminalView_SetDisplayMode(view, (resizeAffectsFont)
																		? kTerminalView_DisplayModeZoom
																		: kTerminalView_DisplayModeNormal);
			}
			break;
		
		default:
			if (kPreferences_ChangeContextBatchMode == inPreferenceTagThatChanged)
			{
				// batch mode; multiple things have changed, so check for the new values
				// of everything that is understood by a terminal view
				if (kPreferences_ClassTranslation == Preferences_ContextReturnClass(prefsContext))
				{
					copyTranslationPreferences(viewPtr, prefsContext);
				}
				else
				{
					(UInt16)copyColorPreferences(viewPtr, prefsContext, false/* search for defaults */);
					(UInt16)copyFontPreferences(viewPtr, prefsContext, false/* search for defaults */);
				}
				(OSStatus)HIViewSetNeedsDisplay(viewPtr->contentHIView, true);
				(OSStatus)HIViewSetNeedsDisplay(viewPtr->focusAndPaddingHIView, true);
				(OSStatus)HIViewSetNeedsDisplay(viewPtr->backgroundHIView, true);
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
Determines the current and maximum view width and
height, and maximum possible width and height in
pixels, based on the current font dimensions and
the current number of columns, rows and scrollback
buffer lines.

It only makes sense to call this after one of the
dependent factors, above, is changed (such as font
size or the screen buffer size).

(3.0)
*/
static void
recalculateCachedDimensions		(TerminalViewPtr	inTerminalViewPtr)
{
	SInt16 const	kWidth = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	SInt16 const	kScrollbackLines = Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref);
	SInt16 const	kVisibleWidth = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	SInt16 const	kVisibleLines = Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
	SInt16 const	kLines = kScrollbackLines + kVisibleLines;
	
	
	inTerminalViewPtr->screen.topEdgeInPixels = -(kScrollbackLines * inTerminalViewPtr->text.font.heightPerCharacter);
	inTerminalViewPtr->screen.maxViewWidthInPixels = kVisibleWidth * inTerminalViewPtr->text.font.widthPerCharacter;
	inTerminalViewPtr->screen.viewWidthInPixels    = kVisibleWidth * inTerminalViewPtr->text.font.widthPerCharacter;
	// TEMPORARY: since some visible lines may be in double-height mode, that should be taken into account here
	inTerminalViewPtr->screen.maxViewHeightInPixels = kVisibleLines * inTerminalViewPtr->text.font.heightPerCharacter;
	inTerminalViewPtr->screen.viewHeightInPixels    = kVisibleLines * inTerminalViewPtr->text.font.heightPerCharacter;
	if (inTerminalViewPtr->screen.maxViewWidthInPixels > (kWidth * inTerminalViewPtr->text.font.widthPerCharacter))
	{
		inTerminalViewPtr->screen.maxViewWidthInPixels = (kWidth * inTerminalViewPtr->text.font.widthPerCharacter);
	}
	if (inTerminalViewPtr->screen.maxViewHeightInPixels > (kLines * inTerminalViewPtr->text.font.heightPerCharacter))
	{
		inTerminalViewPtr->screen.maxViewHeightInPixels = (kLines * inTerminalViewPtr->text.font.heightPerCharacter);
	}
	inTerminalViewPtr->screen.maxWidthInPixels = kWidth * inTerminalViewPtr->text.font.widthPerCharacter;
	inTerminalViewPtr->screen.maxHeightInPixels = kLines * inTerminalViewPtr->text.font.heightPerCharacter;
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
static pascal OSStatus
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
						TerminalViewPtr		viewPtr = new TerminalView(superclassHIObjectAsHIView);
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
				TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
				TerminalScreenRef		initialDataSource = nullptr;
				
				
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
					if (noErr != result)
					{
						// set default
						Preferences_Result		prefsResult = kPreferences_ResultOK;
						Preferences_ContextRef	defaultContext = nullptr;
						
						
						prefsResult = Preferences_GetDefaultContext(&defaultContext, kPreferences_ClassFormat);
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
				Console_WriteValueAddress("request to destroy HIObject implementation", view);
				delete REINTERPRET_CAST(view, TerminalViewPtr);
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
				#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
					CFArrayAppendValue(listOfNames, kAXDescriptionAttribute);
				#endif
					CFArrayAppendValue(listOfNames, kAXRoleAttribute);
					CFArrayAppendValue(listOfNames, kAXRoleDescriptionAttribute);
					CFArrayAppendValue(listOfNames, kAXNumberOfCharactersAttribute);
				#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
					CFArrayAppendValue(listOfNames, kAXTopLevelUIElementAttribute);
				#endif
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
						#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
							if (FlagManager_Test(kFlagOS10_4API))
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
							else
						#endif
							{
								// no API available prior to 10.4 to find this value, so be lazy and return nothing
								result = eventNotHandledErr;
							}
						}
					}
					else if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXNumberOfCharactersAttribute,
																	kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
							SInt32					numChars = returnNumberOfCharacters(viewPtr);
							CFNumberRef				numCharsCFNumber = CFNumberCreate(kCFAllocatorDefault,
																						kCFNumberSInt32Type, &numChars);
							
							
							if (nullptr != numCharsCFNumber)
							{
								result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFTypeRef,
															sizeof(numCharsCFNumber), &numCharsCFNumber);
								CFRelease(numCharsCFNumber), numCharsCFNumber = nullptr;
							}
						}
					}
				#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
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
				#endif
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
				(OSStatus)CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers,
																	typeUInt32, eventModifiers);
				
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
				
				// change the cursor appropriately...
				if (isInSelection)
				{
					// cursor applicable to regions of text that are selected
					if (eventModifiers & cmdKey)
					{
						// if clicked, a highlighted URL would be opened
						(SInt16)Cursors_UseHandPoint();
					}
					else
					{
						// text is draggable
						(SInt16)Cursors_UseHandOpen();
					}
				}
				else
				{
					// cursor applicable to regions of text that are not selected
					if (eventModifiers & controlKey)
					{
						// if clicked, there would be a contextual menu
						(SInt16)Cursors_UseArrowContextualMenu();
					}
					else if ((eventModifiers & optionKey) && (eventModifiers & cmdKey))
					{
						// if clicked, the terminal cursor will move to the mouse location
						(SInt16)Cursors_UsePlus();
					}
					else if (eventModifiers & optionKey)
					{
						// if clicked, the text selection would be rectangular
						(SInt16)Cursors_UseCrosshairs();
					}
					else
					{
						// normal - text selection cursor
						(SInt16)Cursors_UseIBeam();
					}
				}
			}
			break;
		
		case kEventControlSetFocusPart:
			//Console_WriteLine("HI OBJECT control set focus part for terminal view");
			//TMP//result = receiveTerminalViewFocus(inHandlerCallRef, inEvent, view);
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
static OSStatus
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
		TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
		
		
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
static pascal OSStatus
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
			TerminalViewAutoLocker	ptr(gTerminalViewPtrLocks(), terminalView);
			
			
			if (view == ptr->contentHIView)
			{
				// display a contextual menu
				EventRecord		fakeEvent;
				UInt32			modifiers = 0;
				
				
				// attempt to get modifier states (ignore if not available)
				(OSStatus)CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
				
				fakeEvent.what = mouseDown;
				fakeEvent.message = 0;
				fakeEvent.when = 0;
				fakeEvent.modifiers = modifiers;
				GetGlobalMouse(&fakeEvent.where);
				(OSStatus)ContextualMenuBuilder_DisplayMenuForWindow(GetControlOwner(view), &fakeEvent, inContent);
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
static OSStatus
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
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			result = noErr; // ignore part code parameter if absent
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, drawingPort);
			if (noErr != result)
			{
				// use current port
				drawingPort = oldPort;
				result = noErr;
			}
			
			// determine the context to draw in with Core Graphics
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef,
															drawingContext);
			assert_noerr(result);
			
			// if all information can be found, proceed with drawing
			if (noErr == result)
			{
				TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				
				
				// draw text
				if (nullptr != viewPtr)
				{
					RgnHandle	optionalTargetRegion = nullptr;
					HIRect		floatBounds;
					HIRect		floatClipBounds;
					Rect		clipBounds;
					
					
					SetPort(drawingPort);
					
					// determine boundaries of the content view being drawn;
					// ensure view-local coordinates
					HIViewGetBounds(view, &floatBounds);
					
					// maybe a focus region has been provided
					if (noErr == CarbonEventUtilities_GetEventParameter(inEvent, kEventParamRgnHandle, typeQDRgnHandle,
																		optionalTargetRegion))
					{
						SetClip(optionalTargetRegion);
						GetRegionBounds(optionalTargetRegion, &clipBounds);
						floatClipBounds = CGRectMake(clipBounds.left, clipBounds.top, clipBounds.right - clipBounds.left,
														clipBounds.bottom - clipBounds.top);
					}
					else
					{
						SetRectRgn(gInvalidationScratchRegion(), 0, 0, STATIC_CAST(floatBounds.size.width, SInt16),
									STATIC_CAST(floatBounds.size.height, SInt16));
						SetClip(gInvalidationScratchRegion());
						GetRegionBounds(gInvalidationScratchRegion(), &clipBounds);
						floatClipBounds = floatBounds;
					}
					CGContextClipToRect(drawingContext, floatClipBounds);
					
					if ((partCode == kTerminalView_ContentPartText) ||
						(partCode == kControlEntireControl) ||
						((partCode >= kTerminalView_ContentPartFirstLine) &&
							(partCode <= kTerminalView_ContentPartLastLine)))
					{
						Boolean		isFocused = false;
						
						
						// determine if a focus ring is required
						isFocused = HIViewSubtreeContainsFocus(viewPtr->contentHIView);
						
						// draw or erase focus ring
						if (viewPtr->screen.focusRingEnabled)
						{
							//TMP//(OSStatus)HIThemeDrawFocusRect(&floatBounds, isFocused, drawingContext, kHIThemeOrientationNormal);
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
							SInt16		startColumn = 0;
							SInt16		pastTheEndColumn = 0;
							SInt16		startRow = 0;
							SInt16		pastTheEndRow = 0;
							
							
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
							Point const			kTopLeftAnchor = { clipBounds.top, clipBounds.left };
							Point const			kBottomRightAnchor = { clipBounds.bottom, clipBounds.right };
							TerminalView_Cell	leftTopCell;
							TerminalView_Cell	rightBottomCell;
							SInt16				deltaColumn = 0;
							SInt16				deltaRow = 0;
							
							
							// figure out what cells to draw
							(Boolean)findVirtualCellFromLocalPoint(viewPtr, kTopLeftAnchor, leftTopCell, deltaColumn, deltaRow);
							(Boolean)findVirtualCellFromLocalPoint(viewPtr, kBottomRightAnchor, rightBottomCell, deltaColumn, deltaRow);
							
							// draw the text in the clipped area
							(Boolean)drawSection(viewPtr, drawingContext, leftTopCell.first - viewPtr->screen.leftVisibleEdgeInColumns,
													leftTopCell.second - viewPtr->screen.topVisibleEdgeInRows,
													rightBottomCell.first + 1/* past-the-end */ - viewPtr->screen.leftVisibleEdgeInColumns,
													rightBottomCell.second + 1/* past-the-end */ - viewPtr->screen.topVisibleEdgeInRows);
						}
					#endif
						viewPtr->text.attributes = kInvalidTerminalTextAttributes; // forces attributes to reset themselves properly
						
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
					
					// kTerminalView_ContentPartCursor
					{
						CGContextSaveRestore	_(drawingContext);
						CGRect					cursorFloatBounds = CGRectMake(viewPtr->screen.cursor.bounds.left,
																				viewPtr->screen.cursor.bounds.top,
																				viewPtr->screen.cursor.bounds.right -
																					viewPtr->screen.cursor.bounds.left
																					- 2/* TEMPORARY Quartz/QD conversion */,
																				viewPtr->screen.cursor.bounds.bottom -
																					viewPtr->screen.cursor.bounds.top
																					- 2/* TEMPORARY Quartz/QD conversion */);
						
						
						if (kMyCursorStateVisible == viewPtr->screen.cursor.currentState)
						{
							// flip colors and paint at the current blink alpha value
							useTerminalTextColors(viewPtr, drawingContext, kTerminalTextAttributeInverseVideo,
													cursorBlinks(viewPtr)
													? viewPtr->animation.cursor.blinkAlpha
													: 0.8/* arbitrary, but it should be possible to see characters underneath a block shape */);
							CGContextFillRect(drawingContext, cursorFloatBounds);
						}
					}
					
					// kTerminalView_ContentPartCursorGhost
					if (kMyCursorStateVisible == viewPtr->screen.cursor.ghostState)
					{
						// draw the cursor at its ghost location (with ghost appearance)
						Rect	viewRelativeCursorBounds = viewPtr->screen.cursor.ghostBounds/* view-relative */;
						
						
						//InvertRect(&viewRelativeCursorBounds);
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
Handles "kEventControlSetFocusPart" of "kEventClassControl".

Invoked by Mac OS X whenever the currently-focused part of
a terminal view should be changed.

(3.0)
*/
static OSStatus
receiveTerminalViewFocus	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 TerminalViewRef		inTerminalViewRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlSetFocusPart);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (result == noErr)
		{
			HIViewPartCode		focusPart = kControlNoPart;
			
			
			// determine the focus part
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, focusPart);
			if (result == noErr)
			{
				TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				
				
				if (viewPtr != nullptr)
				{
					HIViewPartCode		newFocusPart = kTerminalView_ContentPartVoid;
					
					
					switch (focusPart)
					{
					case kControlFocusNextPart:
						// advance focus
						switch (viewPtr->currentContentFocus)
						{
						case kTerminalView_ContentPartVoid:
							// when the previous view is highlighted and focus advances, the
							// entire terminal view content area should be the next focus target
							newFocusPart = kTerminalView_ContentPartText;
							break;
						
						case kTerminalView_ContentPartText:
						default:
							// when the entire terminal view is highlighted and focus advances,
							// the next view should be the next focus target
							newFocusPart = kTerminalView_ContentPartVoid;
							break;
						}
						break;
					
					case kControlFocusPrevPart:
						// reverse focus
						switch (viewPtr->currentContentFocus)
						{
						case kTerminalView_ContentPartVoid:
							// when the next view is highlighted and focus backs up, the
							// entire terminal view content area should be the next focus target
							newFocusPart = kTerminalView_ContentPartText;
							break;
						
						case kTerminalView_ContentPartText:
						default:
							// when the entire terminal view is highlighted and focus backs up,
							// the previous view should be the next focus target
							newFocusPart = kTerminalView_ContentPartVoid;
							break;
						}
						break;
					
					default:
						// set focus to given part
						newFocusPart = focusPart;
						break;
					}
					
					if (viewPtr->currentContentFocus != newFocusPart)
					{
						// update focus flag
						viewPtr->currentContentFocus = newFocusPart;
						
						// notify the system that the structure region has changed
						(OSStatus)HIViewReshapeStructure(viewPtr->contentHIView);
					}
					
					// update the part code parameter with the new focus part
					result = SetEventParameter(inEvent, kEventParamControlPart,
												typeControlPartCode, sizeof(newFocusPart), &newFocusPart);
				}
			}
		}
	}
	return result;
}// receiveTerminalViewFocus


/*!
Handles "kEventControlGetData" of "kEventClassControl".

Sets the control kind.

(3.1)
*/
static OSStatus
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
static OSStatus
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
				TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				HIViewPartCode			hitPart = kTerminalView_ContentPartVoid;
				HIRect					viewBounds;
				
				
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
static pascal OSStatus
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
				TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
				TerminalView_CellRange	oldSelectionRange = viewPtr->text.selection.range;
				Boolean					selectionChanged = false;
				
				
				// take the opportunity to reset the cursor state, so it is fully visible as
				// keys are being pressed; also reset the stage so that the timing is right
				viewPtr->animation.cursor.blinkAlpha = 1.0;
				viewPtr->animation.cursor.blinkAlphaDelta = -kTerminalView_BlinkAlphaDelta;
				viewPtr->animation.rendering.stage = 0;
				
				// ignore Caps Lock and extra keys for the sake of the comparisons below...
				modifiers &= (cmdKey | shiftKey | optionKey | controlKey);
				
				result = eventNotHandledErr; // initially...
				switch (virtualKeyCode)
				{
				case 0x3B:
				case 0x7B:
					// left arrow
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
					break;
				
				case 0x3C:
				case 0x7C:
					// right arrow
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
					break;
				
				case 0x3E:
				case 0x7E:
					// up arrow
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
					break;
				
				case 0x3D:
				case 0x7D:
					// down arrow
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
					break;
				
				default:
					// do not handle
					break;
				}
				
				if (selectionChanged)
				{
					// TEMPORARY - could adjust this to only invalidate the part that was
					// actually added/removed
					highlightVirtualRange(viewPtr, oldSelectionRange, kTerminalTextAttributeSelected,
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
static OSStatus
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
				TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				Rect					partBounds;
				
				
				// IMPORTANT: All regions are currently rectangles, so this code is simplified.
				// If any irregular regions are added in the future, this has to be restructured.
				switch (partNeedingRegion)
				{
				case kControlStructureMetaPart:
				case kControlContentMetaPart:
				case kTerminalView_ContentPartText:
					GetControlBounds(viewPtr->contentHIView, &partBounds);
					OffsetRect(&partBounds, -partBounds.left, -partBounds.top); // make view-relative coordinates
					break;
				
				case kControlOpaqueMetaPart:
					// the text area is designed to draw on top of a background widget,
					// so in general it is not really considered opaque anywhere (this
					// could be changed for certain cases, however)
					SetRect(&partBounds, 0, 0, 0, 0);
					break;
				
				case kTerminalView_ContentPartCursor:
					partBounds = viewPtr->screen.cursor.bounds;
				Console_WriteValueFloat4("returning cursor bounds on demand", partBounds.left, partBounds.top, partBounds.right, partBounds.bottom);
					break;
				
				case kTerminalView_ContentPartCursorGhost:
					partBounds = viewPtr->screen.cursor.ghostBounds;
					break;
				
				case kTerminalView_ContentPartVoid:
				default:
					SetRect(&partBounds, 0, 0, 0, 0);
					break;
				}
				
				// outset the structure rectangle when focused
				if ((kControlNoPart != viewPtr->currentContentFocus) &&
					(kControlStructureMetaPart == partNeedingRegion))
				{
					InsetRect(&partBounds, -3, -3);
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
static OSStatus
receiveTerminalViewTrack	(EventHandlerCallRef	inHandlerCallRef,
							 EventRef				inEvent,
							 TerminalViewRef		inTerminalViewRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlTrack);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (result == noErr)
		{
			Point   originalLocalMouse;
			
			
			// determine where the mouse is (view-relative!)
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, originalLocalMouse);
			if (result == noErr)
			{
				TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inTerminalViewRef);
				UInt32					currentModifiers = 0;
				
				
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
					
					// this assumes the mouse button is already down
					assert(GetCurrentEventButtonState() != 0);
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
									if ((mouseInSelection) && DragAndDrop_Available() && WaitMouseMoved(localMouse))
									{
										EventRecord		event;
										
										
										// the user has attempted to drag text; track the drag
										SetPortWindowPort(GetControlOwner(view));
										LocalToGlobalRegion(dragRgn);
										OutlineRegion(dragRgn);
										event.where = localMouse;
										SetPortWindowPort(GetControlOwner(view));
										LocalToGlobal(&event.where);
										event.modifiers = currentModifiers;
										(OSStatus)dragTextSelection(viewPtr, dragRgn, &event, &dragged);
										cannotBeDoubleClick = true;
									}
									else
									{
										// no drag, but click inside selection; cancel the selection
										TerminalView_SelectNothing(viewPtr->selfRef);
									}
									Memory_DisposeRegion(&dragRgn);
								}
							}
							
							// if the user clicks outside the current selection, then extend or
							// replace the selection
							unless (mouseInSelection)
							{
								Point const		kOldLocalMouse = localMouse;
								
								
								// drag until the user releases the mouse, highlighting as the mouse moves
								viewPtr->text.selection.keyboardMode = kMy_SelectionModeUnset;
								trackTextSelection(viewPtr, localMouse, currentModifiers, &localMouse, &currentModifiers);
								
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
						if ((viewPtr->text.selection.exists) && (TerminalView_PtInSelection(viewPtr->selfRef, localMouse)))
						{
							URL_HandleForScreenView(viewPtr->screen.ref, viewPtr->selfRef);
						}
						else
						{
							TerminalView_Cell	newColumnRow;
							SInt16				deltaColumn = 0;
							SInt16				deltaRow = 0;
							
							
							(Boolean)findVirtualCellFromLocalPoint(viewPtr, localMouse, newColumnRow, deltaColumn, deltaRow);
							
							// find the URL around the click location, if possible
							if (URL_SetSelectionToProximalURL(newColumnRow, viewPtr->screen.ref, viewPtr->selfRef))
							{
								// open the selected resource
								URL_HandleForScreenView(viewPtr->screen.ref, viewPtr->selfRef);
							}
							else
							{
								// does not appear to be a valid URL; signal this to the user
								Sound_StandardAlert();
							}
						}
					}
					
					// see if a double- or triple-click occurs
					unless (cannotBeDoubleClick)
					{
						Boolean		foundAnotherClick = false;
						
						
						// see if a double-click occurs
						foundAnotherClick = EventLoop_IsNextDoubleClick(&localMouse/* mouse in global coordinates */);
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
							(Boolean)findVirtualCellFromLocalPoint(viewPtr, localMouse,
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
							foundAnotherClick = EventLoop_IsNextDoubleClick(&localMouse);
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
								(Boolean)findVirtualCellFromLocalPoint(viewPtr, localMouse,
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
					result = SetEventParameter(inEvent, kEventParamKeyModifiers,
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
static void
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
Marks the specified terminal window’s contents as
“dirty” so that they are redrawn the next time an
update event is handled.

(3.0)
*/
static void
redrawScreensTerminalWindowOp	(TerminalWindowRef		inTerminalWindow,
								 void*					UNUSED_ARGUMENT(inData1),
								 SInt32					UNUSED_ARGUMENT(inData2),
								 void*					UNUSED_ARGUMENT(inoutResultPtr))
{
	HIViewRef		root = HIViewGetRoot(TerminalWindow_ReturnWindow(inTerminalWindow));
	
	
	(OSStatus)HIViewSetNeedsDisplay(root, true);
}// redrawScreensTerminalWindowOp


/*!
Releases an iterator previously acquired with the
findRowIterator() routine.

Depending on the findRowIterator() implementation,
this call may free memory or merely note that
fewer references to the given iterator now exist.

(3.0)
*/
static void
releaseRowIterator  (TerminalViewPtr	UNUSED_ARGUMENT(inTerminalViewPtr),
					 Terminal_LineRef*	inoutRefPtr)
{
	Terminal_DisposeLineIterator(inoutRefPtr);
}// releaseRowIterator


/*!
Returns an approximation of how many characters are
represented by this terminal view’s text area.

(3.1)
*/
static SInt32
returnNumberOfCharacters	(TerminalViewPtr	inTerminalViewPtr)
{
	UInt16 const	kNumVisibleRows = Terminal_ReturnRowCount(inTerminalViewPtr->screen.ref);
	UInt16 const	kNumScrollbackRows = Terminal_ReturnInvisibleRowCount(inTerminalViewPtr->screen.ref);
	UInt16 const	kNumColumns = Terminal_ReturnColumnCount(inTerminalViewPtr->screen.ref);
	SInt32			result = (kNumVisibleRows + kNumScrollbackRows) * kNumColumns;
	
	
	return result;
}// returnNumberOfCharacters


/*!
Internal version of TerminalView_ReturnSelectedTextAsNewUnicode().

IMPORTANT:	Tab substitution is currently not supported.

(3.1)
*/
static CFStringRef
returnSelectedTextAsNewUnicode	(TerminalViewPtr			inTerminalViewPtr,
								 UInt16						inNumberOfSpacesToReplaceWithOneTabOrZero,
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
			Terminal_LineRef	lineIterator = findRowIterator(inTerminalViewPtr, kSelectionStart.second);
			Terminal_Result		textGrabResult = kTerminal_ResultOK;
			
			
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
					assert(kTerminal_ResultOK == textGrabResult);
				}
				else
				{
					// for standard selections, the first and last lines are different
					if (i == kSelectionStart.second)
					{
						// first line is anchored at the end (LOCALIZE THIS)
						textGrabResult = Terminal_GetLineRange(inTerminalViewPtr->screen.ref, lineIterator,
																kSelectionStart.first, -1/* end column */,
																textBegin, textPastEnd, kTerminal_TextFilterFlagsNoEndWhitespace);
						assert(kTerminal_ResultOK == textGrabResult);
						
					}
					else if (i == (kSelectionPastEnd.second - 1))
					{
						// last line is anchored at the beginning (LOCALIZE THIS)
						textGrabResult = Terminal_GetLineRange(inTerminalViewPtr->screen.ref, lineIterator,
																0/* start column */, kSelectionPastEnd.first,
																textBegin, textPastEnd, kTerminal_TextFilterFlagsNoEndWhitespace);
						assert(kTerminal_ResultOK == textGrabResult);
					}
					else
					{
						// middle lines span the whole width
						textGrabResult = Terminal_GetLine(inTerminalViewPtr->screen.ref, lineIterator,
															textBegin, textPastEnd, kTerminal_TextFilterFlagsNoEndWhitespace);
						assert(kTerminal_ResultOK == textGrabResult);
					}
				}
				
				// add the characters for the line...
				CFStringAppendCharacters(resultMutable, textBegin, textPastEnd - textBegin/* number of characters */);
				
				// if requested, add a new-line
				if (0 == (inFlags & kTerminalView_TextFlagInline))
				{
					// do not terminate last line
					if (i < (kSelectionPastEnd.second - 1))
					{
						CFStringAppendCString(resultMutable, "\015", kCFStringEncodingASCII);
					}
				}
				
				Terminal_LineIteratorAdvance(inTerminalViewPtr->screen.ref, lineIterator, +1);
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
}// returnSelectedTextAsNewUnicode


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
static void
screenBufferChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inTerminalChange,
						 void*					inEventContextPtr,
						 void*					inTerminalViewRef)
{
	TerminalViewRef			view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
	
	
	assert((inTerminalChange == kTerminal_ChangeText) || (inTerminalChange == kTerminal_ChangeScrollActivity));
	
	switch (inTerminalChange)
	{
	case kTerminal_ChangeText:
		if (IsValidWindowRef(HIViewGetWindow(viewPtr->contentHIView)))
		{
			Terminal_RangeDescriptionConstPtr	rangeInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																				Terminal_RangeDescriptionConstPtr);
			register SInt16						i = 0;
			
			
			// debug
			//Console_WriteValuePair("first changed row, number of changed rows",
			//						rangeInfoPtr->firstRow, rangeInfoPtr->rowCount);
			for (i = rangeInfoPtr->firstRow - viewPtr->screen.topVisibleEdgeInRows;
					i < (rangeInfoPtr->firstRow - viewPtr->screen.topVisibleEdgeInRows + rangeInfoPtr->rowCount); ++i)
			{
				invalidateRowSection(viewPtr, i, rangeInfoPtr->firstColumn, rangeInfoPtr->columnCount);
			}
		}
		break;
	
	case kTerminal_ChangeScrollActivity:
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			
			
			assert(screen == viewPtr->screen.ref);
			if (viewPtr->animation.timer.isActive)
			{
				SetEmptyRgn(viewPtr->animation.rendering.region);
			}
			recalculateCachedDimensions(viewPtr);
		}
		break;
	
	default:
		// ???
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
static void
screenCursorChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inTerminalChange,
						 void*					inEventContextPtr,
						 void*					inTerminalViewRef)
{
	TerminalScreenRef		screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
	TerminalViewRef			view = REINTERPRET_CAST(inTerminalViewRef, TerminalViewRef);
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), view);
	
	
	//Console_WriteLine("screen cursor change notification at view level");
	assert((inTerminalChange == kTerminal_ChangeCursorLocation) ||
			(inTerminalChange == kTerminal_ChangeCursorState));
	if (viewPtr->screen.cursor.currentState != kMyCursorStateInvisible)
	{
		// when moving or hiding/showing the cursor, invalidate its original rectangle
		RectRgn(gInvalidationScratchRegion(), &viewPtr->screen.cursor.bounds);
		(OSStatus)HIViewSetNeedsDisplayInRegion(viewPtr->contentHIView, gInvalidationScratchRegion(), true);
		if (inTerminalChange == kTerminal_ChangeCursorLocation)
		{
			// in addition, when moving, recalculate the new bounds and invalidate again
			Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
			UInt16				cursorX = 0;
			UInt16				cursorY = 0;
			
			
			getCursorLocationError = Terminal_CursorGetLocation(screen, &cursorX, &cursorY);
			//Console_WriteValuePair("notification passed new location", cursorX, cursorY);
			setUpCursorBounds(viewPtr, cursorX, cursorY, &viewPtr->screen.cursor.bounds);
			RectRgn(gInvalidationScratchRegion(), &viewPtr->screen.cursor.bounds);
			(OSStatus)HIViewSetNeedsDisplayInRegion(viewPtr->contentHIView, gInvalidationScratchRegion(), true);
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
static void
screenToLocal	(TerminalViewPtr	inTerminalViewPtr,
				 SInt16*			inoutHorizontalPixelOffsetFromScreenOrigin,
				 SInt16*			inoutVerticalPixelOffsetFromScreenOrigin)
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
static void
screenToLocalRect	(TerminalViewPtr	inTerminalViewPtr,
					 Rect*				inoutScreenOriginBoundsPtr)
{
	if (inoutScreenOriginBoundsPtr != nullptr)
	{
		screenToLocal(inTerminalViewPtr, &inoutScreenOriginBoundsPtr->left, &inoutScreenOriginBoundsPtr->top);
		screenToLocal(inTerminalViewPtr, &inoutScreenOriginBoundsPtr->right, &inoutScreenOriginBoundsPtr->bottom);
	}
}// screenToLocalRect


/*!
Copies a color into the blink animation palette for the
specified (zero-based) animation stage.  The animation timer
will use this palette to find colors as needed.

(4.0)
*/
static inline void
setBlinkAnimationColor	(TerminalViewPtr	inTerminalViewPtr,
						 UInt16				inAnimationStage,
						 RGBColor const*	inColorPtr)
{
	assert(inAnimationStage < STATIC_CAST(inTerminalViewPtr->blinkColors.size(), UInt16));
	inTerminalViewPtr->blinkColors[inAnimationStage] = ColorUtilities_CGDeviceColorMake(*inColorPtr);
}// setBlinkAnimationColor


/*!
Starts or stops a timer that periodically refreshes
blinking text.  Since this is potentially expensive,
this routine exists so that the timer is only running
when it is actually needed (that is, when blinking
text actually exists in the terminal view).

(3.1)
*/
static void
setBlinkingTimerActive	(TerminalViewPtr	inTerminalViewPtr,
						 Boolean			inIsActive)
{
#if BLINK_MEANS_BLINK
	if (inTerminalViewPtr->animation.timer.isActive != inIsActive)
	{
		if (inIsActive)
		{
			(OSStatus)SetEventLoopTimerNextFireTime(inTerminalViewPtr->animation.timer.ref, kEventDurationNoWait);
		}
		else
		{
			(OSStatus)SetEventLoopTimerNextFireTime(inTerminalViewPtr->animation.timer.ref, kEventDurationForever);
		}
		inTerminalViewPtr->animation.timer.isActive = inIsActive;
	}
#endif
}// setBlinkingTimerActive


/*!
Hides or shows the cursor “ghost” in the specified
screen.  The ghost cursor is stale (it doesn’t flash),
it is used for indicating the possible new location of
the actual terminal cursor during drags.

(3.0)
*/
static void
setCursorGhostVisibility	(TerminalViewPtr	inTerminalViewPtr,
							 Boolean			inIsVisible)
{
	MyCursorState	newGhostCursorState = (inIsVisible) ? kMyCursorStateVisible : kMyCursorStateInvisible;
	Boolean			renderCursor = (inTerminalViewPtr->screen.cursor.ghostState != newGhostCursorState);
	
	
	if (API_AVAILABLE(IsValidWindowRef))
	{
		// cursor flashing is done within a thread; after a window closes,
		// the flashing ought to stop, but to make sure of that the window
		// must be valid (otherwise drawing occurs in the desktop!)
		renderCursor = (renderCursor && IsValidWindowRef(HIViewGetWindow(inTerminalViewPtr->contentHIView)));
	}
	
	// change state
	inTerminalViewPtr->screen.cursor.ghostState = newGhostCursorState;
	
	// redraw the cursor if necessary
	if (renderCursor)
	{
		// TEMPORARY: highlighting one part does not seem to work right now, so
		// invalidate the WHOLE view (expensive?) to ensure the cursor redraws
		(OSStatus)HIViewSetNeedsDisplay(inTerminalViewPtr->contentHIView, true);
		//HiliteControl(inTerminalViewPtr->contentHIView, kTerminalView_ContentPartCursorGhost);
	}
}// setCursorGhostVisibility


/*!
Hides or shows the cursor in the specified screen.
It may be invoked repeatedly.

(3.0)
*/
static void
setCursorVisibility		(TerminalViewPtr	inTerminalViewPtr,
						 Boolean			inIsVisible)
{
	MyCursorState	newCursorState = (inIsVisible) ? kMyCursorStateVisible : kMyCursorStateInvisible;
	Boolean			renderCursor = (inTerminalViewPtr->screen.cursor.currentState != newCursorState);
	
	
	if (API_AVAILABLE(IsValidWindowRef))
	{
		// cursor flashing is done within a thread; after a window closes,
		// the flashing ought to stop, but to make sure of that the window
		// must be valid (otherwise drawing occurs in the desktop!)
		renderCursor = (renderCursor && IsValidWindowRef(HIViewGetWindow(inTerminalViewPtr->contentHIView)));
	}
	
	// change state
	inTerminalViewPtr->screen.cursor.currentState = newCursorState;
	
	// redraw the cursor if necessary
	if (renderCursor)
	{
		RectRgn(gInvalidationScratchRegion(), &inTerminalViewPtr->screen.cursor.bounds);
		(OSStatus)HIViewSetNeedsDisplayInRegion(inTerminalViewPtr->contentHIView, gInvalidationScratchRegion(), true);
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
static void
setFontAndSize		(TerminalViewPtr	inViewPtr,
					 ConstStringPtr		inFontFamilyNameOrNull,
					 UInt16				inFontSizeOrZero,
					 Float32			inCharacterWidthScalingOrZero,
					 Boolean			inNotifyListeners)
{
	CGrafPtr	oldPort = nullptr;
	GDHandle	oldDevice = nullptr;
	
	
	if (inFontFamilyNameOrNull != nullptr)
	{
		// remember font selection
		PLstrcpy(inViewPtr->text.font.familyName, inFontFamilyNameOrNull);
	}
	
	if (inFontSizeOrZero > 0)
	{
		inViewPtr->text.font.normalMetrics.size = inFontSizeOrZero;
	}
	
	if (inCharacterWidthScalingOrZero > 0)
	{
		inViewPtr->text.font.scaleWidthPerCharacter = inCharacterWidthScalingOrZero;
	}
	
	// determine the encoding supported by the font
	{
		TextEncoding	encoding = kTextEncodingMacRoman;
		OSStatus		error = Localization_GetFontTextEncoding(inViewPtr->text.font.familyName, &encoding);
		
		
		if (noErr != error)
		{
			Console_Warning(Console_WriteValuePString, "unable to determine encoding used by font",
							inViewPtr->text.font.familyName);
		}
		inViewPtr->text.font.encoding = encoding; // TextEncoding is compatible with CFStringEncoding
	}
	
	// set the font metrics (including double size)
	GetGWorld(&oldPort, &oldDevice);
	setPortScreenPort(inViewPtr);
	TextFontByName(inViewPtr->text.font.familyName);
	TextSize(inViewPtr->text.font.normalMetrics.size);
	setUpScreenFontMetrics(inViewPtr);
	
	// recalculate cursor boundaries for the specified view
	{
		Terminal_Result		getCursorLocationError = kTerminal_ResultOK;
		UInt16				cursorX = 0;
		UInt16				cursorY = 0;
		
		
		getCursorLocationError = Terminal_CursorGetLocation(inViewPtr->screen.ref, &cursorX, &cursorY);
		setUpCursorBounds(inViewPtr, cursorX, cursorY, &inViewPtr->screen.cursor.bounds);
	}
	
	// resize window to preserve its dimensions in character cell units
	recalculateCachedDimensions(inViewPtr);
	
	SetGWorld(oldPort, oldDevice);
	
	if (inNotifyListeners)
	{
		// notify listeners that the size has changed
		eventNotifyForView(inViewPtr, kTerminalView_EventFontSizeChanged, inViewPtr->selfRef/* context */);
	}
}// setFontAndSize


/*!
Sets the current port to the port of the
indicated terminal window.

Obsolete.

(2.6)
*/
static SInt16
setPortScreenPort	(TerminalViewPtr	inTerminalViewPtr)
{
	SInt16		result = 0;
	
	
	if (nullptr == inTerminalViewPtr) result = -3;
	else
	{
		static TerminalViewPtr	oldViewPtr = nullptr;
		HIWindowRef				window = HIViewGetWindow(inTerminalViewPtr->contentHIView);
		
		
		if (nullptr == window) result = -4;
		else
		{
			if (oldViewPtr != inTerminalViewPtr) // is last-used window different?
			{
				oldViewPtr = inTerminalViewPtr;
				inTerminalViewPtr->text.attributes = kInvalidTerminalTextAttributes; // attributes will need setting
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
static inline void
setScreenBaseColor	(TerminalViewPtr			inTerminalViewPtr,
					 TerminalView_ColorIndex	inColorEntryNumber,
					 RGBColor const*			inColorPtr)
{
	switch (inColorEntryNumber)
	{
	case kTerminalView_ColorIndexNormalBackground:
		inTerminalViewPtr->text.colors[kMyBasicColorIndexNormalBackground] = *inColorPtr;
		if (nullptr != inTerminalViewPtr->focusAndPaddingHIView)
		{
			OSStatus	error = noErr;
			
			
			error = SetControlProperty(inTerminalViewPtr->focusAndPaddingHIView, AppResources_ReturnCreatorCode(),
										kConstantsRegistry_ControlPropertyTypeBackgroundColor,
										sizeof(*inColorPtr), inColorPtr);
			assert_noerr(error);
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
			if (nullptr != inTerminalViewPtr->backgroundHIView)
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
		GDHandle	device = nullptr;
		RGBColor	colorValue;
		
		
		// figure out which display has most control over screen content
		// NOTE: This check should be done separately, probably whenever a
		//       terminal window is moved.
		if (GetWindowGreatestAreaDevice(HIViewGetWindow(inTerminalViewPtr->contentHIView), kWindowContentRgn,
										&device, nullptr/* rectangle */) != noErr)
		{
			// if this can’t be found, just assume the main monitor
			device = GetMainDevice();
		}
		
		// create enough intermediate colors to make a reasonably
		// smooth “pulsing” effect as text blinks; the last color
		// in the sequence matches the base foreground color, just
		// for simplicity in the animation code
		colorValue = inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingText];
		for (SInt16 i = kMy_BlinkingColorCount - 1; i >= 0; --i)
		{
			(Boolean)GetGray(device, &inTerminalViewPtr->text.colors[kMyBasicColorIndexBlinkingBackground],
								&colorValue/* both input and output */);
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
static inline void
setScreenCoreColor	(TerminalViewPtr	inTerminalViewPtr,
					 UInt16				inColorEntryNumber,
					 RGBColor const*	inColorPtr)
{
	inTerminalViewPtr->coreColors[inColorEntryNumber] = ColorUtilities_CGDeviceColorMake(*inColorPtr);
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
static inline void
setScreenCustomColor	(TerminalViewPtr			inTerminalViewPtr,
						 TerminalView_ColorIndex	inColorEntryNumber,
						 RGBColor const*			inColorPtr)
{
	assert(inColorEntryNumber < STATIC_CAST(inTerminalViewPtr->customColors.size(), TerminalView_ColorIndex));
	inTerminalViewPtr->customColors[inColorEntryNumber] = ColorUtilities_CGDeviceColorMake(*inColorPtr);
}// setScreenCustomColor


/*!
Uses the current cursor shape preference and font metrics
of the given screen to determine the view-relative boundaries
of the terminal cursor if it were located at the specified
row and column in the terminal screen.

(3.0)
*/
static void
setUpCursorBounds	(TerminalViewPtr			inTerminalViewPtr,
					 SInt16						inX,
					 SInt16						inY,
					 Rect*						outBoundsPtr,
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
	UInt16						thickness = 0; // used for non-block-shaped cursors
	TerminalView_CursorType		terminalCursorType = inTerminalCursorType;
	size_t						actualSize = 0L;
	
	
	// if requested, use whatever cursor shape the user wants;
	// otherwise, calculate the bounds for the given shape
	if (inTerminalCursorType == kTerminalView_CursorTypeCurrentPreferenceValue)
	{
		unless (Preferences_GetData(kPreferences_TagTerminalCursorType, sizeof(terminalCursorType),
									&terminalCursorType, &actualSize) == kPreferences_ResultOK)
		{
			terminalCursorType = kTerminalView_CursorTypeBlock; // assume a block-shaped cursor, if preference can’t be found
		}
	}
	
	SetPt(&characterSizeInPixels, getRowCharacterWidth(inTerminalViewPtr, inY), inTerminalViewPtr->text.font.heightPerCharacter);
	
	outBoundsPtr->left = inX * characterSizeInPixels.h;
	outBoundsPtr->top = inY * characterSizeInPixels.v;
	
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
}// setUpCursorBounds


/*!
Specifies the top-left corner of the ghost cursor in
coordinates local to the WINDOW.  The coordinates are
automatically converted to screen coordinates based on
wherever the terminal view happens to be.

The ghost cursor is used during drags to indicate where
the cursor would go, but is not indicative of the
cursor’s actual position.

NOTE:	This routine currently anchors the cursor at the
		edges of the screen, when in fact the better
		behavior is to hide the cursor if it is not in
		the screen area.

(3.0)
*/
static void
setUpCursorGhost	(TerminalViewPtr	inTerminalViewPtr,
					 Point				inLocalMouse)
{
	Rect	screenBounds;
	
	
	GetControlBounds(inTerminalViewPtr->contentHIView, &screenBounds);
	
	// do not show the ghost if the point is not inside the screen area
	if (PtInRect(inLocalMouse, &screenBounds))
	{
		TerminalView_Cell	newColumnRow;
		SInt16				deltaColumn = 0;
		SInt16				deltaRow = 0;
		
		
		(Boolean)findVirtualCellFromLocalPoint(inTerminalViewPtr, inLocalMouse, newColumnRow, deltaColumn, deltaRow);
		// cursor is visible - put it where it’s supposed to be
		setUpCursorBounds(inTerminalViewPtr, newColumnRow.first, newColumnRow.second, &inTerminalViewPtr->screen.cursor.ghostBounds);
	}
	else
	{
		// cursor is outside the screen - do not show any cursor in the visible screen area
		setUpCursorBounds(inTerminalViewPtr, -100/* arbitrary */, -100/* arbitrary */, &inTerminalViewPtr->screen.cursor.ghostBounds);
	}
}// setUpCursorGhost


/*!
Sets the font ascent, height, width, and monospaced
attributes of the font information for the specified
screen.  The font information is determined by looking
at the current settings for the specified port.

This routine corrects a long-standing bug where MacTelnet
would incorrectly recalculate the font width attribute
based on the current font of the specified port, not the
default font of the specified screen.  On a terminal
screen, every cell must be the same width, so all
rendered fonts must use the same width, regardless of
the width that they might otherwise require.  Thus, as
of MacTelnet 3.0, font metrics ignore the current font
of the screen (e.g. regardless of whether boldface text
is currently being drawn) and use the default font.

(3.0)
*/
static void
setUpScreenFontMetrics	(TerminalViewPtr	inTerminalViewPtr)
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
		inTerminalViewPtr->text.font.widthPerCharacter = CharWidth('A');
	}
	else
	{
		inTerminalViewPtr->text.font.widthPerCharacter = fontInfo.widMax;
	}
	
	// scale the font width according to user preferences
	{
		Float32		reduction = inTerminalViewPtr->text.font.widthPerCharacter;
		
		
		reduction *= inTerminalViewPtr->text.font.scaleWidthPerCharacter;
		inTerminalViewPtr->text.font.widthPerCharacter = STATIC_CAST(reduction, SInt16);
	}
	
	// now use the font metrics to determine how big double-width text should be
	calculateDoubleSize(inTerminalViewPtr, inTerminalViewPtr->text.font.doubleMetrics.size,
						inTerminalViewPtr->text.font.doubleMetrics.ascent);
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
static void
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
static void
trackTextSelection	(TerminalViewPtr	inTerminalViewPtr,
					 Point				inLocalMouse,
					 EventModifiers		inModifiers,
					 Point*				outNewLocalMousePtr,
					 UInt32*			outNewModifiersPtr)
{
	TerminalView_Cell		clickedColumnRow;
	TerminalView_Cell		previousClick;
	SInt16					deltaColumn = 0;
	SInt16					deltaRow = 0;
	CGrafPtr				oldPort = nullptr;
	GDHandle				oldDevice = nullptr;
	MouseTrackingResult		trackingResult = kMouseTrackingMouseDown;
	Point					localMouse = inLocalMouse;
	Boolean					startNewSelection = true;
	
	
	GetGWorld(&oldPort, &oldDevice);
	
	// start by locating the cell under the mouse, scrolling to reveal it as needed
	(Boolean)findVirtualCellFromLocalPoint(inTerminalViewPtr, inLocalMouse, clickedColumnRow, deltaColumn, deltaRow);
	(TerminalView_Result)TerminalView_ScrollAround(inTerminalViewPtr->selfRef, deltaColumn, deltaRow);
	
	if (inTerminalViewPtr->text.selection.exists)
	{
		// the default behavior is to start a new selection; however,
		// the shift key will instead cause the selection to be extended
		startNewSelection = true;
		if (inModifiers & shiftKey)
		{
			sortAnchors(inTerminalViewPtr->text.selection.range.first, inTerminalViewPtr->text.selection.range.second,
						inTerminalViewPtr->text.selection.isRectangular);
			if ((clickedColumnRow.second < inTerminalViewPtr->text.selection.range.first.second) ||
				((clickedColumnRow.second == inTerminalViewPtr->text.selection.range.first.second) &&
					(clickedColumnRow.first < inTerminalViewPtr->text.selection.range.first.first)))
			{
				sortAnchors(inTerminalViewPtr->text.selection.range.first, inTerminalViewPtr->text.selection.range.second,
							inTerminalViewPtr->text.selection.isRectangular);
			}
			startNewSelection = false;	// in this case, DON’T cancel the selection; shift-drag means “continue”
		}
	}
	
	if (startNewSelection)
	{
		// unhighlight the old selection
		highlightCurrentSelection(inTerminalViewPtr, false/* is highlighted */, true/* redraw */);
		inTerminalViewPtr->text.selection.range.first = std::make_pair(0, 0);
		inTerminalViewPtr->text.selection.range.second = std::make_pair(0, 0);
		inTerminalViewPtr->text.selection.exists = false;
		
		// use the modifier key states to decide on the new selection mode
		inTerminalViewPtr->text.selection.isRectangular = (0 != (inModifiers & optionKey));
	}
	
	Cursors_UseIBeam();
	SetPortWindowPort(HIViewGetWindow(inTerminalViewPtr->contentHIView));
	
	// continue tracking until the mouse is released
	previousClick = std::make_pair(-1, -1);
	do
	{
		// find new mouse location, scroll if necessary
		(Boolean)findVirtualCellFromLocalPoint(inTerminalViewPtr, localMouse, clickedColumnRow, deltaColumn, deltaRow);
		(TerminalView_Result)TerminalView_ScrollAround(inTerminalViewPtr->selfRef, deltaColumn, deltaRow);
		
		// if the mouse moves, update the selection
		{
			TerminalView_Cell const		kClickedCell = std::make_pair(clickedColumnRow.first, clickedColumnRow.second);
			
			
			unless (kClickedCell == previousClick)
			{
				// if this is the first move (that is, the mouse was not simply clicked to delete
				// the previous selection), start a new selection range consisting of the cell
				// underneath the mouse
				if (false == inTerminalViewPtr->text.selection.exists)
				{
					// the selection range is inclusive-start, exclusive-end, so add one to the ends...
					inTerminalViewPtr->text.selection.range.first = kClickedCell;
					inTerminalViewPtr->text.selection.range.second = std::make_pair
																		(kClickedCell.first + 1, kClickedCell.second + 1);
					inTerminalViewPtr->text.selection.exists = true;
				}
				
				// toggle the highlight state of text between the current and last mouse positions
				highlightCurrentSelection(inTerminalViewPtr, false/* is highlighted */, true/* redraw */);
				inTerminalViewPtr->text.selection.range.second = kClickedCell;
				highlightCurrentSelection(inTerminalViewPtr, true/* is highlighted */, true/* redraw */);
				
				previousClick = kClickedCell;
			}
		}
		
		// find next mouse location
		{
			OSStatus	error = noErr;
			
			
			error = TrackMouseLocationWithOptions(nullptr/* port, or nullptr for current port */, 0/* options */,
													kEventDurationForever/* timeout */, &localMouse, outNewModifiersPtr,
													&trackingResult);
			if (error != noErr) break;
		}
	}
	while (trackingResult != kMouseTrackingMouseUp);
	
	// update given parameters (output modifiers were set earlier)
	*outNewLocalMousePtr = localMouse;
	
	// the anchors might end up out of order after the loop
	sortAnchors(inTerminalViewPtr->text.selection.range.first, inTerminalViewPtr->text.selection.range.second,
				inTerminalViewPtr->text.selection.isRectangular);
	
	SetGWorld(oldPort, oldDevice);
	
	inTerminalViewPtr->text.selection.exists = (inTerminalViewPtr->text.selection.range.first !=
												inTerminalViewPtr->text.selection.range.second);
	copySelectedTextIfUserPreference(inTerminalViewPtr);
}// trackTextSelection


/*!
Sets the background color and pattern settings (and ONLY those
settings) of the current QuickDraw port and the specified
Core Graphics port, based on the requested attributes and the
active state of the terminal’s container view.

You may pass a desired alpha value, that ONLY affects the
Core Graphics context, and automatically uses that value when
setting colors.

In general, only style and dimming affect color.

IMPORTANT:	Core Graphics support is INCOMPLETE.  This routine
			may not completely update the context with all
			necessary parameters.

(3.1)
*/
static void
useTerminalTextColors	(TerminalViewPtr			inTerminalViewPtr,
						 CGContextRef				inDrawingContext,
						 TerminalTextAttributes		inAttributes,
						 Float32					inDesiredAlpha)
{
	RGBColor	backgroundRGB;
	RGBColor	colorRGB;
	Boolean		usingDragHighlightColors = (inTerminalViewPtr->screen.currentRenderDragColors);
	
	
	// IMPORTANT: Drawing code is currently transitioning to Core Graphics;
	// so, not all QuickDraw settings are completely replaced yet, and some
	// are even made redundantly in both contexts.
	
	// find the correct colors in the color table
	getScreenColorsForAttributes(inTerminalViewPtr, inAttributes, &colorRGB, &backgroundRGB,
									&inTerminalViewPtr->screen.currentRenderNoBackground);
	
	// set up foreground color
	if (usingDragHighlightColors)
	{
		// when rendering a drag highlight, use black text
		colorRGB.red = 0;
		colorRGB.green = 0;
		colorRGB.blue = 0;
		RGBForeColor(&colorRGB);
		CGContextSetRGBStrokeColor(inDrawingContext, 0/* red */, 0/* green */, 0/* blue */, inDesiredAlpha);
		
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
		RGBBackColor(&backgroundRGB);
		
		// “darken” the colors if text is selected, but only in the foreground;
		// in the background, the view renders an outline of the selection, so
		// selected text should NOT have any special appearance in that case
		if (STYLE_SELECTED(inAttributes) && inTerminalViewPtr->isActive)
		{
			inTerminalViewPtr->screen.currentRenderNoBackground = false;
			
			if (gPreferenceProxies.invertSelections) UseInvertedColors();
			else UseSelectionColors();
		}
		
		// when the screen is inactive, it is dimmed; in addition, any text that is
		// not selected is FURTHER dimmed so as to emphasize the selection and show
		// that the selection is in fact a click-through region
		unless ((inTerminalViewPtr->isActive) || (gPreferenceProxies.dontDimTerminals) ||
				(STYLE_SELECTED(inAttributes) && !gApplicationIsSuspended))
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
		if (STYLE_SEARCH_RESULT(inAttributes))
		{
			inTerminalViewPtr->screen.currentRenderNoBackground = false;
			
			UseSelectionColors();
			UseLighterColors();
		}
		
		// finally, check the foreground and background colors; do not allow
		// them to be identical unless “concealed” is the style (e.g. perhaps
		// text is ANSI white and the background is white; that's invisible!)
		unless (STYLE_CONCEALED(inAttributes))
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
			CGDeviceColor		floatRGB;
			
			
			GetForeColor(&colorRGB);
			floatRGB = ColorUtilities_CGDeviceColorMake(colorRGB);
			CGContextSetRGBStrokeColor(inDrawingContext, floatRGB.red, floatRGB.green, floatRGB.blue, inDesiredAlpha);
			
			GetBackColor(&colorRGB);
			floatRGB = ColorUtilities_CGDeviceColorMake(colorRGB);
			CGContextSetRGBFillColor(inDrawingContext, floatRGB.red, floatRGB.green, floatRGB.blue, inDesiredAlpha);
		}
	}
}// useTerminalTextColors


/*!
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
static void
useTerminalTextAttributes	(TerminalViewPtr			inTerminalViewPtr,
							 CGContextRef				inDrawingContext,
							 TerminalTextAttributes		inAttributes)
{
	//static CGrafPtr	lastPort = nullptr;
	CGrafPtr			currentPort = nullptr;
	GDHandle			currentDevice = nullptr;
	
	
	GetGWorld(&currentPort, &currentDevice);
	if ((inTerminalViewPtr->text.attributes == kInvalidTerminalTextAttributes) ||
		(inTerminalViewPtr->text.attributes != inAttributes)/* ||
		(currentPort != lastPort)*/)
	{
		//lastPort = currentPort;
		inTerminalViewPtr->text.attributes = inAttributes;
		
		// When special glyphs should be rendered, set the font to
		// a specific ID that tells the renderer to do graphics.
		// The font is not actually used, it is just a marker so
		// that the renderer knows it is in graphics mode.  In most
		// cases, the terminal’s regular font is used instead.
		if (STYLE_USE_VT_GRAPHICS(inAttributes)) TextFont(kArbitraryVTGraphicsPseudoFontID);
		else TextFontByName(inTerminalViewPtr->text.font.familyName);
		
		// set text size, examining “double size” bits
		// NOTE: there’s no real reason this should have to be checked
		//       here, EVERY time text is rendered; it could eventually
		//       be set somewhere else, e.g. whenever the attribute bits
		//       or font size change, a rendering size could be updated
		if (STYLE_IS_DOUBLE_HEIGHT_TOP(inAttributes))
		{
			// top half - do not render
			TextSize(kArbitraryDoubleWidthDoubleHeightPseudoFontSize);
		}
		else if (STYLE_IS_DOUBLE_HEIGHT_BOTTOM(inAttributes))
		{
			// bottom half - double size
			TextSize(inTerminalViewPtr->text.font.doubleMetrics.size);
		}
		else
		{
			// normal size
			TextSize(inTerminalViewPtr->text.font.normalMetrics.size);
		}
		
		// set font style...
		TextFace(normal);
		
		// set pen...
		PenNormal();
		{
			Float32		scaleH = 0.0;
			Float32		scaleV = 0.0;
			
			
			// if the text is really big, drawn lines should be bigger as well, so
			// multiply 1 pixel by a scaling factor based on the size of the text
			scaleH = inTerminalViewPtr->text.font.widthPerCharacter / 5.0;
			scaleV = inTerminalViewPtr->text.font.heightPerCharacter / 7.0;
			PenSize(INTEGER_MAXIMUM(1, STATIC_CAST(scaleH, SInt16)), INTEGER_MAXIMUM(1, STATIC_CAST(scaleV, SInt16)));
		}
		
		// 3.0 - for a sufficiently large font, allow boldface
		if (STYLE_BOLD(inAttributes) || STYLE_SEARCH_RESULT(inAttributes))
		{
			SInt16		fontSize = GetPortTextSize(currentPort);
			Style		fontFace = GetPortTextFace(currentPort);
			
			
			if (fontSize > 8) // arbitrary (should this be a user preference?)
			{
				TextFace(fontFace | bold);
				
				// VT graphics use the pen size when rendering, so that should be “bolded” as well
				{
					PenState	penState;
					
					
					GetPenState(&penState);
					PenSize(INTEGER_DOUBLED(penState.pnSize.h)/* width */, INTEGER_DOUBLED(penState.pnSize.v)/* height */);
				}
			}
		}
		
		// 3.0 - for a sufficiently large font, allow italicizing
		if (STYLE_ITALIC(inAttributes))
		{
			SInt16		fontSize = GetPortTextSize(currentPort);
			Style		fontFace = GetPortTextFace(currentPort);
			
			
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
		if (STYLE_UNDERLINE(inAttributes) || STYLE_SEARCH_RESULT(inAttributes))
		{
			SInt16		fontSize = GetPortTextSize(currentPort);
			Style		fontFace = GetPortTextFace(currentPort);
			
			
			if (fontSize > 8/* arbitrary (should this be a user preference?) */)
			{
				TextFace(fontFace | underline);
			}
		}
		
		// set text mode and pen mode
		TextMode(srcOr); // do not render background color
		PenMode(patCopy);
	}
}// useTerminalTextAttributes


/*!
This method handles bell signals in specific terminal
windows.  If the user has global bell sounding turned
off (“visual bell”), no bell is heard, but the window
is flashed.  If the specified window is not in front
at the time this method is invoked, the window will
flash regardless of the “visual bell” setting.  If
MacTelnet is suspended when the bell occurs, and the
user has notification preferences set, MacTelnet will
post a notification event.

(3.0)
*/
static void
visualBell	(TerminalViewRef	inView)
{
	TerminalViewAutoLocker	viewPtr(gTerminalViewPtrLocks(), inView);
	HIWindowRef const		kViewWindow = HIViewGetWindow(viewPtr->contentHIView);
	Boolean const			kWasReverseVideo = viewPtr->screen.isReverseVideo;
	Boolean					visual = false;				// is visual used?
	Boolean					visualPreference = false;	// is ONLY visual used?
	size_t					actualSize = 0L;
	
	
	// If the user turned off audible bells, always use a visual;
	// otherwise, use a visual if the beep is in a background window.
	unless (Preferences_GetData(kPreferences_TagVisualBell, sizeof(visualPreference),
								&visualPreference, &actualSize) ==
			kPreferences_ResultOK)
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
	GenericThreads_DelayMinimumTicks(8);
	
	// if previously inverted, invert again to restore to normal
	if (visual)
	{
		TerminalView_ReverseVideo(inView, kWasReverseVideo); // also invalidates view
		HIWindowFlush(kViewWindow);
	}
	
	if (gPreferenceProxies.notifyOfBeeps) Alert_BackgroundNotification();
}// visualBell

// BELOW IS REQUIRED NEWLINE TO END FILE
