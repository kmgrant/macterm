/*!	\file TerminalWindow.mm
	\brief The most common type of window, used to hold
	terminal views and scroll bars for a session.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2011 by Kevin Grant.
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

#import "UniversalDefines.h"

// standard-C includes
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <map>
#import <vector>

// GNU extension includes
#import <ext/numeric>

// UNIX includes
extern "C"
{
#	include <pthread.h>
#	include <strings.h>
}

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <AutoPool.objc++.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CGContextSaveRestore.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaFuture.objc++.h>
#import <ColorUtilities.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <ContextSensitiveMenu.h>
#import <Cursors.h>
#import <Embedding.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceTracker.template.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RandomWrap.h>
#import <RegionUtilities.h>
#import <Registrar.template.h>
#import <SoundSystem.h>
#import <Undoables.h>
#import <WindowInfo.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "Console.h"
#import "ContextualMenuBuilder.h"
#import "DialogUtilities.h"
#import "EventLoop.h"
#import "FindDialog.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "Preferences.h"
#import "PrefPanelFormats.h"
#import "PrefPanelTerminals.h"
#import "PrefPanelTranslations.h"
#import "PrefsContextDialog.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalWindow.h"
#import "TerminalView.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

// NOTE: do not ever change these, that would only break user preferences
NSString*	kMy_ToolbarItemIDLED1		= @"com.mactelnet.MacTelnet.toolbaritem.led1";
NSString*	kMy_ToolbarItemIDLED2		= @"com.mactelnet.MacTelnet.toolbaritem.led2";
NSString*	kMy_ToolbarItemIDLED3		= @"com.mactelnet.MacTelnet.toolbaritem.led3";
NSString*	kMy_ToolbarItemIDLED4		= @"com.mactelnet.MacTelnet.toolbaritem.led4";
NSString*	kMy_ToolbarItemIDPrint		= @"com.mactelnet.MacTelnet.toolbaritem.print";
// WARNING: The Customize item ID is currently redundantly specified in the Info Window module; this is TEMPORARY, but both should agree.
NSString*	kMy_ToolbarItemIDCustomize	= @"com.mactelnet.MacTelnet.toolbaritem.customize";
NSString*	kMy_ToolbarItemIDTabs		= @"net.macterm.MacTerm.toolbaritem.tabs";

SInt16 const	kMy_MaximumNumberOfArrangedWindows = 20; // TEMPORARY RESTRICTION

/*!
These are hacks.  But they make up for the fact that theme
APIs do not really work very well at all, and it is
necessary in a few places to figure out how much space is
occupied by certain parts of a scroll bar.
*/
float const		kMy_ScrollBarThumbEndCapSize = 16.0; // pixels
float const		kMy_ScrollBarThumbMinimumSize = kMy_ScrollBarThumbEndCapSize + 32.0 + kMy_ScrollBarThumbEndCapSize; // pixels
float const		kMy_ScrollBarArrowHeight = 16.0; // pixels

/*!
Use with getScrollBarKind() for an unknown scroll bar.
*/
enum My_ScrollBarKind
{
	kMy_InvalidScrollBarKind	= 0,
	kMy_ScrollBarKindVertical	= 1,
	kMy_ScrollBarKindHorizontal = 2
};

/*!
Specifies the type of sheet (if any) that is currently
displayed.  This is used by the preferences context
monitor, so that it knows what settings were changed.
*/
enum My_SheetType
{
	kMy_SheetTypeNone			= 0,
	kMy_SheetTypeFormat			= 1,
	kMy_SheetTypeScreenSize		= 2,
	kMy_SheetTypeTranslation	= 3
};

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"DimensionsFloater" and "DimensionsSheet" NIB from the
package "TerminalWindow.nib".
*/
HIViewID const	idMyTextScreenDimensions	= { 'Dims', 0/* ID */ };

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Tab" NIB from the package "TerminalWindow.nib".
*/
HIViewID const	idMyLabelTabTitle			= { 'TTit', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

typedef std::map< TerminalViewRef, TerminalScreenRef >			My_ScreenByTerminalView;
typedef std::map< NSWindow*, TerminalWindowRef >				My_TerminalWindowByNSWindow;
typedef std::vector< TerminalScreenRef >						My_TerminalScreenList;
typedef std::vector< TerminalViewRef >							My_TerminalViewList;
typedef std::vector< Undoables_ActionRef >						My_UndoableActionList;
typedef std::multimap< TerminalScreenRef, TerminalViewRef >		My_ViewsByScreen;

typedef MemoryBlockReferenceTracker< TerminalWindowRef >	My_RefTracker;
typedef Registrar< TerminalWindowRef, My_RefTracker >		My_RefRegistrar;

struct My_TerminalWindow
{
	My_TerminalWindow  (Preferences_ContextRef, Preferences_ContextRef, Preferences_ContextRef, Boolean);
	~My_TerminalWindow ();
	
	My_RefRegistrar				refValidator;				// ensures this reference is recognized as a valid one
	TerminalWindowRef			selfRef;					// redundant reference to self, for convenience
	
	ListenerModel_Ref			changeListenerModel;		// who to notify for various kinds of changes to this terminal data
	
	NSWindow*					window;						// the Cocoa window reference for the terminal window (wrapping Carbon)
	CFRetainRelease				tab;						// the Mac OS window reference (if any) for the sister window acting as a tab
	CarbonEventHandlerWrap*		tabContextualMenuHandlerPtr;// used to track contextual menu clicks in tabs
	CarbonEventHandlerWrap*		tabDragHandlerPtr;			// used to track drags that enter tabs
	WindowGroupRef				tabAndWindowGroup;			// WindowGroupRef; forces the window and its tab to move together
	Float32						tabOffsetInPixels;			// used to position the tab drawer, if any
	Float32						tabSizeInPixels;			// used to position and size a tab drawer, if any
	HIToolbarRef				toolbar;					// customizable toolbar of icons at the top
	CFRetainRelease				toolbarItemBell;			// if present, enable/disable bell item
	CFRetainRelease				toolbarItemLED1;			// if present, LED #1 status item
	CFRetainRelease				toolbarItemLED2;			// if present, LED #2 status item
	CFRetainRelease				toolbarItemLED3;			// if present, LED #3 status item
	CFRetainRelease				toolbarItemLED4;			// if present, LED #4 status item
	CFRetainRelease				toolbarItemScrollLock;		// if present, scroll lock status item
	HIWindowRef					resizeFloater;				// temporary window that appears during resizes
	TerminalView_DisplayMode	preResizeViewDisplayMode;	// stored in case user invokes option key variation on resize
	WindowInfo_Ref				windowInfo;					// window information object for the terminal window
	
	struct
	{
		HIViewRef		scrollBarH;				// scroll bar used to specify which range of columns is visible
		HIViewRef		scrollBarV;				// scroll bar used to specify which range of rows is visible
		HIViewRef		textScreenDimensions;   // defined only in the floater window that appears during resizes
	} controls;
	
	SInt16						staggerPositionIndex;	// index into array of window stagger positions (after enough
														// windows are open, stagger positions are re-used, so there
														// can be more than one window with the same index)
	Boolean						isObscured;				// is the window hidden, via a command in the Window menu?
	Boolean						isDead;					// is the window title flagged to indicate a disconnected session?
	Boolean						isLEDOn[4];				// true only if this terminal light is lit
	Boolean						viewSizeIndependent;	// true only temporarily, to handle transitional cases such as full-screen mode
	Preferences_ContextWrap		recentSheetContext;		// defined temporarily while a Preferences-dependent sheet (such as screen size) is up
	My_SheetType				sheetType;				// if a sheet is active, this is a hint as to what settings can be put in the context
	FindDialog_Options			recentSearchOptions;	// the options used during the last search in the dialog
	CFRetainRelease				recentSearchStrings;	// CFMutableArrayRef; the CFStrings used in searches since this window was opened
	CFRetainRelease				baseTitleString;		// user-provided title string; may be adorned prior to becoming the window title
	CFRetainRelease				preResizeTitleString;	// used to save the old title during resizes, where the title is overwritten
	ControlActionUPP			scrollProcUPP;							// handles scrolling activity
	CommonEventHandlers_WindowResizer	windowResizeHandler;			// responds to changes in the window size
	CommonEventHandlers_WindowResizer	tabDrawerWindowResizeHandler;	// responds to changes in the tab drawer size
	CarbonEventHandlerWrap		mouseWheelHandler;						// responds to scroll wheel events
	CarbonEventHandlerWrap		scrollTickHandler;						// responds to drawing events in the scroll bar
	EventHandlerUPP				commandUPP;								// wrapper for command callback
	EventHandlerRef				commandHandler;							// invoked whenever a terminal window command is executed
	EventHandlerUPP				windowClickActivationUPP;				// wrapper for window background clicks callback
	EventHandlerRef				windowClickActivationHandler;			// invoked whenever a terminal window is hit while inactive
	EventHandlerUPP				windowCursorChangeUPP;					// wrapper for window cursor change callback
	EventHandlerRef				windowCursorChangeHandler;				// invoked whenever the mouse cursor might change in a terminal window
	EventHandlerUPP				windowDragCompletedUPP;					// wrapper for window move completion callback
	EventHandlerRef				windowDragCompletedHandler;				// invoked whenever a terminal window has finished being moved by the user
	EventHandlerUPP				windowResizeEmbellishUPP;				// wrapper for window resize callback
	EventHandlerRef				windowResizeEmbellishHandler;			// invoked whenever a terminal window is resized
	EventHandlerUPP				growBoxClickUPP;						// wrapper for grow box click callback
	EventHandlerRef				growBoxClickHandler;					// invoked whenever a terminal window’s grow box is clicked
	EventHandlerUPP				toolbarEventUPP;						// wrapper for toolbar callback
	EventHandlerRef				toolbarEventHandler;					// invoked whenever a toolbar needs an item created, etc.
	ListenerModel_ListenerWrap	sessionStateChangeEventListener;		// responds to changes in a session
	ListenerModel_ListenerWrap	terminalStateChangeEventListener;		// responds to changes in a terminal
	ListenerModel_ListenerWrap	terminalViewEventListener;				// responds to changes in a terminal view
	ListenerModel_ListenerWrap	toolbarStateChangeEventListener;		// responds to changes in a toolbar
	
	My_ViewsByScreen				screensToViews;			// map of a screen buffer to one or more views
	My_ScreenByTerminalView			viewsToScreens;			// map of views to screen buffers
	My_TerminalScreenList			allScreens;				// all screen buffers represented in the two maps above
	My_TerminalViewList				allViews;				// all views represented in the two maps above
	
	My_UndoableActionList			installedActions;		// undoable things installed on behalf of this window
};
typedef My_TerminalWindow*			My_TerminalWindowPtr;
typedef My_TerminalWindow const*	My_TerminalWindowConstPtr;

/*!
Context data for the context ID "kUndoableContextIdentifierTerminalFontSizeChanges".
*/
struct UndoDataFontSizeChanges
{
	Undoables_ActionRef		action;				//!< used to manage the Undo command
	TerminalWindowRef		terminalWindow;		//!< which window was reformatted
	Boolean					undoFontSize;		//!< is this Undo action going to reverse the font size changes?
	Boolean					undoFont;			//!< is this Undo action going to reverse the font changes?
	UInt16					fontSize;			//!< the old font size (ignored if "undoFontSize" is false)
	Str255					fontName;			//!< the old font (ignored if "undoFont" is false)
};
typedef UndoDataFontSizeChanges*		UndoDataFontSizeChangesPtr;

/*!
Context data for the context ID "kUndoableContextIdentifierTerminalFullScreenChanges".
*/
struct UndoDataFullScreenChanges
{
	Undoables_ActionRef			action;				//!< used to manage the Undo command
	TerminalWindowRef			terminalWindow;		//!< which window was reformatted
	TerminalView_DisplayMode	oldMode;			//!< previous terminal resize effect
	TerminalView_DisplayMode	newMode;			//!< full-screen terminal resize effect
	UInt16						oldFontSize;		//!< font size prior to full-screen
	Rect						oldContentBounds;	//!< old window boundaries, content area
};
typedef UndoDataFullScreenChanges*		UndoDataFullScreenChangesPtr;

/*!
Context data for the context ID "kUndoableContextIdentifierTerminalDimensionChanges".
*/
struct UndoDataScreenDimensionChanges
{
	Undoables_ActionRef		action;				//!< used to manage the Undo command
	TerminalWindowRef		terminalWindow;		//!< which window was resized
	UInt16					columns;			//!< the old screen width
	UInt16					rows;				//!< the old screen height
};
typedef UndoDataScreenDimensionChanges*		UndoDataScreenDimensionChangesPtr;

typedef MemoryBlockPtrLocker< TerminalWindowRef, My_TerminalWindow >	My_TerminalWindowPtrLocker;
typedef LockAcquireRelease< TerminalWindowRef, My_TerminalWindow >		My_TerminalWindowAutoLocker;

} // anonymous namespace

/*!
Toolbar item “Customize”.
*/
@interface TerminalWindow_ToolbarItemCustomize : NSToolbarItem
{
}
@end

/*!
Toolbar item “L1”.
*/
@interface TerminalWindow_ToolbarItemLED1 : NSToolbarItem
{
}
@end

/*!
Toolbar item “L2”.
*/
@interface TerminalWindow_ToolbarItemLED2 : NSToolbarItem
{
}
@end

/*!
Toolbar item “L3”.
*/
@interface TerminalWindow_ToolbarItemLED3 : NSToolbarItem
{
}
@end

/*!
Toolbar item “L4”.
*/
@interface TerminalWindow_ToolbarItemLED4 : NSToolbarItem
{
}
@end

/*!
Toolbar item “Print”.
*/
@interface TerminalWindow_ToolbarItemPrint : NSToolbarItem
{
}
@end

/*!
Toolbar item “Tabs”.
*/
@interface TerminalWindow_ToolbarItemTabs : NSToolbarItem
{
	NSSegmentedControl*		segmentedControl;
	NSArray*				targets;
	SEL						action;
}

- (void)
setTabTargets:(NSArray*)_
andAction:(SEL)_;

@end

/*!
A sample object type that can be used to represent a tab in
the object array of a TerminalWindow_ToolbarItemTabs instance.
*/
@interface TerminalWindow_TabSource : NSObject
{
	NSAttributedString*		description;
}

- (id)
initWithDescription:(NSAttributedString*)_;

- (NSAttributedString*)
attributedDescription;

- (void)
performAction:(id)_;

@end

#pragma mark Internal Method Prototypes
namespace {

void					addContextualMenuItemsForTab	(MenuRef, HIObjectRef, AEDesc&);
void					calculateWindowPosition			(My_TerminalWindowPtr, Rect*);
void					calculateIndexedWindowPosition	(My_TerminalWindowPtr, SInt16, Point*);
void					changeNotifyForTerminalWindow	(My_TerminalWindowPtr, TerminalWindow_Change, void*);
IconRef					createBellOffIcon				();
IconRef					createBellOnIcon				();
IconRef					createCustomizeToolbarIcon		();
IconRef					createFullScreenIcon			();
IconRef					createHideWindowIcon			();
IconRef					createScrollLockOffIcon			();
IconRef					createScrollLockOnIcon			();
IconRef					createLEDOffIcon				();
IconRef					createLEDOnIcon					();
IconRef					createPrintIcon					();
void					createViews						(My_TerminalWindowPtr);
Boolean					createTabWindow					(My_TerminalWindowPtr);
NSWindow*				createWindow					();
void					ensureTopLeftCornersExists		();
TerminalScreenRef		getActiveScreen					(My_TerminalWindowPtr);
TerminalViewRef			getActiveView					(My_TerminalWindowPtr);
My_ScrollBarKind		getScrollBarKind				(My_TerminalWindowPtr, HIViewRef);
TerminalScreenRef		getScrollBarScreen				(My_TerminalWindowPtr, HIViewRef);
TerminalViewRef			getScrollBarView				(My_TerminalWindowPtr, HIViewRef);
void					getViewSizeFromWindowSize		(My_TerminalWindowPtr, SInt16, SInt16, SInt16*, SInt16*);
void					getWindowSizeFromViewSize		(My_TerminalWindowPtr, SInt16, SInt16, SInt16*, SInt16*);
void					handleFindDialogClose			(FindDialog_Ref);
void					handleNewDrawerWindowSize		(WindowRef, Float32, Float32, void*);
void					handleNewSize					(WindowRef, Float32, Float32, void*);
void					installTickHandler				(My_TerminalWindowPtr);
void					installUndoFontSizeChanges		(TerminalWindowRef, Boolean, Boolean);
void					installUndoFullScreenChanges	(TerminalWindowRef, TerminalView_DisplayMode, TerminalView_DisplayMode);
void					installUndoScreenDimensionChanges	(TerminalWindowRef);
OSStatus				receiveHICommand				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveMouseWheelEvent			(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveScrollBarDraw			(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveTabContextualMenuClick	(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveTabDragDrop				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveToolbarEvent				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowCursorChange		(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowDragCompleted		(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowGetClickActivation	(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowResize				(EventHandlerCallRef, EventRef, void*);
HIWindowRef				returnCarbonWindow				(My_TerminalWindowPtr);
UInt16					returnGrowBoxHeight				(My_TerminalWindowPtr);
UInt16					returnGrowBoxWidth				(My_TerminalWindowPtr);
UInt16					returnScrollBarHeight			(My_TerminalWindowPtr);
UInt16					returnScrollBarWidth			(My_TerminalWindowPtr);
UInt16					returnStatusBarHeight			(My_TerminalWindowPtr);
UInt16					returnToolbarHeight				(My_TerminalWindowPtr);
void					reverseFontChanges				(Undoables_ActionInstruction, Undoables_ActionRef, void*);
void					reverseFullScreenChanges		(Undoables_ActionInstruction, Undoables_ActionRef, void*);
void					reverseScreenDimensionChanges	(Undoables_ActionInstruction, Undoables_ActionRef, void*);
void					scrollProc						(HIViewRef, HIViewPartCode);
void					sessionStateChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
OSStatus				setCursorInWindow				(WindowRef, Point, UInt32);
void					setScreenPreferences			(My_TerminalWindowPtr, Preferences_ContextRef);
void					setStandardState				(My_TerminalWindowPtr, UInt16, UInt16, Boolean);
void					setViewFormatPreferences		(My_TerminalWindowPtr, Preferences_ContextRef);
void					setViewSizeIndependentFromWindow(My_TerminalWindowPtr, Boolean);
void					setViewTranslationPreferences	(My_TerminalWindowPtr, Preferences_ContextRef);
void					setWarningOnWindowClose			(My_TerminalWindowPtr, Boolean);
void					setWindowAndTabTitle			(My_TerminalWindowPtr, CFStringRef);
void					setWindowToIdealSizeForDimensions	(My_TerminalWindowPtr, UInt16, UInt16);
void					setWindowToIdealSizeForFont		(My_TerminalWindowPtr);
void					sheetClosed						(GenericDialog_Ref, Boolean);
Preferences_ContextRef	sheetContextBegin				(My_TerminalWindowPtr, Quills::Prefs::Class, My_SheetType);
void					sheetContextEnd					(My_TerminalWindowPtr);
void					stackWindowTerminalWindowOp		(TerminalWindowRef, void*, SInt32, void*);
void					terminalStateChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void					terminalViewStateChanged		(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void					updateScrollBars				(My_TerminalWindowPtr);

} // anonymous namespace

#pragma mark Variables
namespace {

My_TerminalWindowByNSWindow&	gTerminalNSWindows ()			{ static My_TerminalWindowByNSWindow x; return x; }
My_RefTracker&					gTerminalWindowValidRefs ()		{ static My_RefTracker x; return x; }
My_TerminalWindowPtrLocker&		gTerminalWindowPtrLocks ()		{ static My_TerminalWindowPtrLocker x; return x; }
SInt16**					gTopLeftCorners = nullptr;
SInt16						gNumberOfTransitioningWindows = 0;	// used only by TerminalWindow_StackWindows()
IconRef&					gBellOffIcon ()					{ static IconRef x = createBellOffIcon(); return x; }
IconRef&					gBellOnIcon ()					{ static IconRef x = createBellOnIcon(); return x; }
IconRef&					gCustomizeToolbarIcon ()		{ static IconRef x = createCustomizeToolbarIcon(); return x; }
IconRef&					gFullScreenIcon ()				{ static IconRef x = createFullScreenIcon(); return x; }
IconRef&					gHideWindowIcon ()				{ static IconRef x = createHideWindowIcon(); return x; }
IconRef&					gLEDOffIcon ()					{ static IconRef x = createLEDOffIcon(); return x; }
IconRef&					gLEDOnIcon ()					{ static IconRef x = createLEDOnIcon(); return x; }
IconRef&					gPrintIcon ()					{ static IconRef x = createPrintIcon(); return x; }
IconRef&					gScrollLockOffIcon ()			{ static IconRef x = createScrollLockOffIcon(); return x; }
IconRef&					gScrollLockOnIcon ()			{ static IconRef x = createScrollLockOnIcon(); return x; }
Float32						gDefaultTabWidth = 0.0;		// set later
Float32						gDefaultTabHeight = 0.0;	// set later

} // anonymous namespace


#pragma mark Public Methods

/*!
Creates a new terminal window that is configured in the given
ways.  If any problems occur, nullptr is returned; otherwise,
a reference to the new terminal window is returned.

Any of the contexts can be "nullptr" if you want to rely on
defaults.  These contexts only determine initial settings;
future changes to the contexts will not affect the window.

The "inNoStagger" argument should normally be set to false; it
is used for the special case of a new window that duplicates
an existing window (so that it can be animated into its final
position).

IMPORTANT:	In general, you should NOT create terminal windows
			this way; use the Session Factory module.

(3.0)
*/
TerminalWindowRef
TerminalWindow_New  (Preferences_ContextRef		inTerminalInfoOrNull,
					 Preferences_ContextRef		inFontInfoOrNull,
					 Preferences_ContextRef		inTranslationOrNull,
					 Boolean					inNoStagger)
{
	TerminalWindowRef	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_TerminalWindow(inTerminalInfoOrNull, inFontInfoOrNull, inTranslationOrNull, inNoStagger),
									TerminalWindowRef);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
This method cleans up a terminal window by destroying all
of the data associated with it.  On output, your copy of
the given reference will be set to nullptr.

(3.0)
*/
void
TerminalWindow_Dispose   (TerminalWindowRef*	inoutRefPtr)
{
	if (gTerminalWindowPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteLine, "attempt to dispose of locked terminal window");
	}
	else
	{
		// clean up
		{
			My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), *inoutRefPtr);
			
			
			delete ptr;
		}
		*inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Displays the Find dialog for the given terminal window,
handling searches automatically.  On Mac OS X, the dialog
is a sheet, so this routine may return immediately without
the user having finished searching.

(3.0)
*/
void
TerminalWindow_DisplayTextSearchDialog	(TerminalWindowRef		inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	FindDialog_Ref					findDialog = FindDialog_New(inRef, handleFindDialogClose,
																ptr->recentSearchStrings.returnCFMutableArrayRef(),
																ptr->recentSearchOptions);
	
	
	// display a text search dialog (automatically disposed when the user clicks a button)
	FindDialog_Display(findDialog);
}// DisplayTextSearchDialog


/*!
Determines if the specified mouse (or drag) event is hitting
any part of the given terminal window.

(3.1)
*/
Boolean
TerminalWindow_EventInside	(TerminalWindowRef	inRef,
							 EventRef			inMouseEvent)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	HIViewRef						hitView = nullptr;
	OSStatus						error = noErr;
	Boolean							result = false;
	
	
	error = HIViewGetViewForMouseEvent(HIViewGetRoot(returnCarbonWindow(ptr)), inMouseEvent, &hitView);
	if (noErr == error)
	{
		result = true;
	}
	
	return result;
}// EventInside


/*!
Determines if a Mac OS window has a terminal window
reference.

(3.0)
*/
Boolean
TerminalWindow_ExistsFor	(WindowRef	inWindow)
{
	Boolean		result = false;
	
	
#if 0
	if (inWindow != nullptr)
	{
		SInt16  kind = GetWindowKind(inWindow);
		
		
		result = ((kind == WIN_CONSOLE) || (kind == WIN_CNXN) || (kind == WIN_SHELL));
		if (result) result = (ScreenFactory_GetWindowActiveScreen(inWindow) != nullptr);
	}
#else
	{
		WindowInfo_Ref		windowInfo = WindowInfo_ReturnFromWindow(inWindow);
		
		
		result = ((nullptr != windowInfo) &&
					(kConstantsRegistry_WindowDescriptorAnyTerminal == WindowInfo_ReturnWindowDescriptor(windowInfo)));
	}
#endif
	
	return result;
}// ExistsFor


/*!
Makes a terminal window the target of keyboard input, but does
not force it to be in front.

See also TerminalWindow_IsFocused().

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.  All calls to the Carbon SelectWindow(),
that had been using TerminalWindow_ReturnWindow(), should
DEFINITELY change to call this routine, instead (which
manipulates the Cocoa window internally).

(4.0)
*/
void
TerminalWindow_Focus	(TerminalWindowRef	inRef)
{
	AutoPool						_;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	[ptr->window makeKeyWindow];
}// Focus


/*!
Returns the font and/or size used by the terminal
screens in the specified window.  If you are not
interested in one of the values, simply pass nullptr
as its input.

IMPORTANT:	This API is under evaluation.  It does
			not allow for the possibility of more
			than one terminal view per window, in
			the sense that each view theoretically
			can have its own font and size.

(3.0)
*/
void
TerminalWindow_GetFontAndSize	(TerminalWindowRef	inRef,
								 StringPtr			outFontFamilyNameOrNull,
								 UInt16*			outFontSizeOrNull)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	TerminalView_GetFontAndSize(getActiveView(ptr)/* TEMPORARY */, outFontFamilyNameOrNull, outFontSizeOrNull);
}// GetFontAndSize


/*!
Returns references to all virtual terminal screen buffers
that can be seen in the given terminal window.  In order
for the result to be greater than 1, there must be at
least two distinct source buffers (and not just two
distinct split-pane views) used by the window.

Use TerminalWindow_ReturnScreenCount() to determine an
appropriate size for your array, then allocate an array
and pass the count as "inArrayLength".  Note that this
is the number of elements, not necessarily the number
of bytes!

Currently, MacTerm only has one screen per window, so
only one screen will be returned.  However, if your code
*could* vary depending on the number of screens in a
window, you should use this API to iterate properly now,
to ensure correct behavior in the future.

(3.0)
*/
void
TerminalWindow_GetScreens	(TerminalWindowRef		inRef,
							 UInt16					inArrayLength,
							 TerminalScreenRef*		outScreenArray,
							 UInt16*				outActualCountOrNull)
{
	if (outScreenArray != nullptr)
	{
		My_TerminalWindowAutoLocker				ptr(gTerminalWindowPtrLocks(), inRef);
		My_TerminalScreenList::const_iterator	screenIterator;
		My_TerminalScreenList::const_iterator	maxIterator = ptr->allScreens.begin();
		
		
		// based on the available space given by the caller,
		// find where the list “past-the-end” is
		std::advance(maxIterator, INTEGER_MINIMUM(inArrayLength, ptr->allScreens.size()));
		
		// copy all possible screen buffer references
		for (screenIterator = ptr->allScreens.begin(); screenIterator != maxIterator; ++screenIterator)
		{
			*outScreenArray++ = *screenIterator;
		}
		if (outActualCountOrNull != nullptr) *outActualCountOrNull = ptr->allScreens.size();
	}
}// GetScreens


/*!
Returns the number of columns and/or the number of
rows visible in the specified terminal window.  If
there are multiple split-panes (multiple screens),
the result is the sum of the views shown by all.

(3.0)
*/
void
TerminalWindow_GetScreenDimensions	(TerminalWindowRef	inRef,
									 UInt16*			outColumnCountPtrOrNull,
									 UInt16*			outRowCountPtrOrNull)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (outColumnCountPtrOrNull != nullptr) *outColumnCountPtrOrNull = Terminal_ReturnColumnCount(getActiveScreen(ptr)/* TEMPORARY */);
	if (outRowCountPtrOrNull != nullptr) *outRowCountPtrOrNull = Terminal_ReturnRowCount(getActiveScreen(ptr)/* TEMPORARY */);
}// GetScreenDimensions


/*!
Returns the tab width, in pixels, of the specified terminal
window (or height, for left/right tabs).  If the window has never
been explicitly sized, some default size will be returned.
Otherwise, the size most recently set with
TerminalWindow_SetTabWidth() will be returned.

See also TerminalWindow_GetTabWidthAvailable().

\retval kTerminalWindow_ResultOK
if there are no errors

\retval kTerminalWindow_ResultInvalidReference
if the specified terminal window is unrecognized

\retval kTerminalWindow_ResultGenericFailure
if no window has ever had a tab; "outWidthHeightInPixels" will be 0

(3.1)
*/
TerminalWindow_Result
TerminalWindow_GetTabWidth	(TerminalWindowRef	inRef,
							 Float32&			outWidthHeightInPixels)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result			result = kTerminalWindow_ResultOK;
	
	
	if (gTerminalWindowValidRefs().end() == gTerminalWindowValidRefs().find(inRef))
	{
		result = kTerminalWindow_ResultInvalidReference;
		Console_Warning(Console_WriteValueAddress, "attempt to TerminalWindow_GetTabWidth() with invalid reference", inRef);
		outWidthHeightInPixels = 0;
	}
	else
	{
		Float32 const	kMinimumWidth = 32.0; // arbitrary
		
		
		if (ptr->tabSizeInPixels < kMinimumWidth)
		{
			result = kTerminalWindow_ResultGenericFailure;
			outWidthHeightInPixels = kMinimumWidth;
		}
		else
		{
			outWidthHeightInPixels = ptr->tabSizeInPixels;
		}
	}
	return result;
}// GetTabWidth


/*!
Returns the space for tabs, in pixels, along the tab edge of the
specified terminal window, at the current window size.  This is
automatically a vertical measurement if the user preference is
for left-edge or right-edge tabs.

Note that it is not generally a good idea to rely on the width
available to one window.  Every window in a workspace should be
consulted, and the smallest range available to any of the windows
should be used as the available range for all of them.  This way,
a tab cannot be positioned in such a way that the parent window
is forced to be widened.

See also TerminalWindow_GetTabWidth().

\retval kTerminalWindow_ResultOK
if there are no errors

\retval kTerminalWindow_ResultInvalidReference
if the specified terminal window is unrecognized

\retval kTerminalWindow_ResultGenericFailure
if no window has ever had a tab; "outWidthHeightInPixels" will be 0

(3.1)
*/
TerminalWindow_Result
TerminalWindow_GetTabWidthAvailable		(TerminalWindowRef	inRef,
										 Float32&			outMaxWidthHeightInPixels)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result			result = kTerminalWindow_ResultOK;
	
	
	if (gTerminalWindowValidRefs().end() == gTerminalWindowValidRefs().find(inRef))
	{
		result = kTerminalWindow_ResultInvalidReference;
		Console_Warning(Console_WriteValueAddress, "attempt to TerminalWindow_GetTabWidthAvailable() with invalid reference", inRef);
		outMaxWidthHeightInPixels = 500.0; // arbitrary!
	}
	else
	{
		Rect					windowRect;
		OSStatus				error = noErr;
		OptionBits				preferredEdge = kWindowEdgeTop;
		Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagWindowTabPreferredEdge,
																	sizeof(preferredEdge), &preferredEdge);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			preferredEdge = kWindowEdgeTop;
		}
		
		error = GetWindowBounds(returnCarbonWindow(ptr), kWindowStructureRgn, &windowRect);
		assert_noerr(error);
		
		if ((kWindowEdgeLeft == preferredEdge) || (kWindowEdgeRight == preferredEdge))
		{
			outMaxWidthHeightInPixels = windowRect.bottom - windowRect.top;
		}
		else
		{
			outMaxWidthHeightInPixels = windowRect.right - windowRect.left;
		}
	}
	return result;
}// GetTabWidthAvailable


/*!
Returns references to all terminal views in the given
terminal window.  In order for any views to be
returned, there must be at least two terminal screen
controls in use by the window.

Use TerminalWindow_GetViewCount() to determine an
appropriate size for your array, then allocate an array
and pass the count as "inArrayLength".  Note that this
is the number of elements, not necessarily the number
of bytes!

Currently, MacTerm only has one view per window, so
only one view will be returned.  However, if your code
*could* vary depending on the number of views in a
window, you should use this API to iterate properly now,
to ensure correct behavior in the future.

IMPORTANT:	The TerminalWindow_GetViewsInGroup() API is
			more specific and recommended.  This API
			should now be avoided.

(3.0)
*/
void
TerminalWindow_GetViews		(TerminalWindowRef	inRef,
							 UInt16				inArrayLength,
							 TerminalViewRef*	outViewArray,
							 UInt16*			outActualCountOrNull)
{
	if (outViewArray != nullptr)
	{
		My_TerminalWindowAutoLocker				ptr(gTerminalWindowPtrLocks(), inRef);
		My_TerminalViewList::const_iterator		viewIterator;
		My_TerminalViewList::const_iterator		maxIterator = ptr->allViews.begin();
		
		
		// based on the available space given by the caller,
		// find where the list “past-the-end” is
		std::advance(maxIterator, INTEGER_MINIMUM(inArrayLength, ptr->allViews.size()));
		
		// copy all possible view references
		for (viewIterator = ptr->allViews.begin(); viewIterator != maxIterator; ++viewIterator)
		{
			*outViewArray++ = *viewIterator;
		}
		if (outActualCountOrNull != nullptr) *outActualCountOrNull = ptr->allViews.size();
	}
}// GetViews


/*!
Returns references to all terminal views in the given
terminal window that belong to the specified group.

By specifying a group filter, you can automatically
retrieve an ordered list of views pertinent to your
purpose.

Use TerminalWindow_GetViewCountInGroup() to determine
an appropriate size for your array, then allocate an
array and pass the count as "inArrayLength" (and be
sure to pass the same group constant, too!).  Note
that this is the number of *elements*, not necessarily
the number of bytes!

Currently, MacTerm only has one view per window, so
only one view will be returned.  However, if your code
*could* vary depending on the number of views in a
window, you should use this API to iterate properly now,
to ensure correct behavior in the future.

(3.0)
*/
TerminalWindow_Result
TerminalWindow_GetViewsInGroup	(TerminalWindowRef			inRef,
								 TerminalWindow_ViewGroup	inViewGroup,
								 UInt16						inArrayLength,
								 TerminalViewRef*			outViewArray,
								 UInt16*					outActualCountOrNull)
{
	TerminalWindow_Result	result = kTerminalWindow_ResultGenericFailure;
	
	
	switch (inViewGroup)
	{
	case kTerminalWindow_ViewGroupEverything:
	case kTerminalWindow_ViewGroupActive:
		if (outViewArray != nullptr)
		{
			My_TerminalWindowAutoLocker				ptr(gTerminalWindowPtrLocks(), inRef);
			My_TerminalViewList::const_iterator		viewIterator;
			My_TerminalViewList::const_iterator		maxIterator = ptr->allViews.begin();
			
			
			// based on the available space given by the caller,
			// find where the list “past-the-end” is
			std::advance(maxIterator, INTEGER_MINIMUM(inArrayLength, ptr->allViews.size()));
			
			// copy all possible view references
			for (viewIterator = ptr->allViews.begin(); viewIterator != maxIterator; ++viewIterator)
			{
				*outViewArray++ = *viewIterator;
			}
			if (outActualCountOrNull != nullptr) *outActualCountOrNull = ptr->allViews.size();
			result = kTerminalWindow_ResultOK;
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// GetViewsInGroup


/*!
Returns "true" only if the specified window currently has the
keyboard focus.

See also TerminalWindow_Focus().

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.  Calls to the Carbon IsWindowFocused(),
that had been using TerminalWindow_ReturnWindow(), should
DEFINITELY change to call this routine, instead (which
manipulates the Cocoa window internally).

(4.0)
*/
Boolean
TerminalWindow_IsFocused	(TerminalWindowRef	inRef)
{
	AutoPool						_;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	Boolean							result = false;
	
	
	result = (YES == [ptr->window isKeyWindow]);
	return result;
}// IsFocused


/*!
Returns "true" only if the specified window is obscured,
meaning it is invisible to the user but technically
considered a visible window.  This is the state used by
the “Hide Front Window” command.

(3.0)
*/
Boolean
TerminalWindow_IsObscured	(TerminalWindowRef	inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	Boolean							result = false;
	
	
	result = ptr->isObscured;
	return result;
}// IsObscured


/*!
Returns "true" only if the specified window has a tab
appearance, as set with TerminalWindow_SetTabAppearance().

(4.0)
*/
Boolean
TerminalWindow_IsTab	(TerminalWindowRef	inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	Boolean							result = false;
	
	
	result = ptr->tab.exists();
	return result;
}// IsTab


/*!
Changes the settings of every view in the specified group,
to include the recognized settings of the given context.  You
might use this, for example, to do a batch-mode change of all
the fonts and colors of a terminal window’s views.

A preferences class can be provided as a hint to indicate what
should be changed.  For example, Quills::Prefs::FORMAT will set
fonts and colors on views, but Quills::Prefs::TERMINAL will set
internal screen buffer preferences.

Currently, the only supported group is the active view,
"kTerminalWindow_ViewGroupActive".

Returns true only if successful.

WARNING:	The Quills::Prefs::TRANSLATION class can be set up
			with this API, but only as a helper for Session APIs!
			If you actually want to change encodings, be sure to
			use Session_ReturnTranslationConfiguration() and
			copy changes there, so that the Session can see them.
			A Session-level change will, in turn, call this
			routine to update the views.

(4.0)
*/
Boolean
TerminalWindow_ReconfigureViewsInGroup	(TerminalWindowRef			inRef,
										 TerminalWindow_ViewGroup	inViewGroup,
										 Preferences_ContextRef		inContext,
										 Quills::Prefs::Class		inPrefsClass)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	Boolean							result = false;
	
	
	if (kTerminalWindow_ViewGroupActive == inViewGroup)
	{
		switch (inPrefsClass)
		{
		case Quills::Prefs::FORMAT:
			setViewFormatPreferences(ptr, inContext);
			result = true;
			break;
		
		case Quills::Prefs::TERMINAL:
			setScreenPreferences(ptr, inContext);
			result = true;
			break;
		
		case Quills::Prefs::TRANSLATION:
			setViewTranslationPreferences(ptr, inContext);
			result = true;
			break;
		
		default:
			// ???
			break;
		}
	}
	return result;
}// ReconfigureViewsInGroup


/*!
Returns the Terminal Window associated with the most recently
active non-floating Cocoa or Carbon window, or nullptr if there
is none.

Use this in cases where you want to interact with the terminal
window even if something else is focused, e.g. if a floating
window is currently the target of keyboard input.

(4.0)
*/
TerminalWindowRef
TerminalWindow_ReturnFromMainWindow ()
{
	NSWindow*			activeWindow = [NSApp mainWindow];
	TerminalWindowRef	result = [activeWindow terminalWindowRef];
	
	
	if (nullptr == result)
	{
		// old method; temporary, for Carbon
		result = TerminalWindow_ReturnFromWindow(ActiveNonFloatingWindow());
	}
	return result;
}// ReturnFromMainWindow


/*!
Returns the Terminal Window associated with the Cocoa or Carbon
window that has keyboard focus, or nullptr if there is none.  In
particular, if a floating window is focused, this will always
return nullptr.

Use this in cases where the target of keyboard input absolutely
must be a terminal, and cannot be a floating non-terminal window.

(4.0)
*/
TerminalWindowRef
TerminalWindow_ReturnFromKeyWindow ()
{
	NSWindow*			activeWindow = [NSApp keyWindow];
	TerminalWindowRef	result = [activeWindow terminalWindowRef];
	
	
	if (nullptr == result)
	{
		// old method; temporary, for Carbon
		result = TerminalWindow_ReturnFromWindow(GetUserFocusWindow());
	}
	return result;
}// ReturnFromKeyWindow


/*!
Returns the Terminal Window associated with the specified
window, if any.  A window that is not a terminal window
will cause a nullptr return value.

See also TerminalWindow_ReturnFromActiveWindow().

(3.0)
*/
TerminalWindowRef
TerminalWindow_ReturnFromWindow		(WindowRef	inWindow)
{
	TerminalWindowRef	result = nullptr;
	WindowInfo_Ref		windowInfo = WindowInfo_ReturnFromWindow(inWindow);
	
	
	if ((nullptr != windowInfo) &&
		(kConstantsRegistry_WindowDescriptorAnyTerminal == WindowInfo_ReturnWindowDescriptor(windowInfo)))
	{
		result = REINTERPRET_CAST(WindowInfo_ReturnAuxiliaryDataPtr(windowInfo), TerminalWindowRef);
	}
	
	return result;
}// ReturnFromWindow


/*!
Returns the Cocoa window for the specified terminal window.

IMPORTANT:	If an API exists to manipulate a terminal window,
			use the Terminal Window API; only use the Cocoa
			window when absolutely necessary.

(4.0)
*/
NSWindow*
TerminalWindow_ReturnNSWindow	(TerminalWindowRef	inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	NSWindow*						result = nil;
	
	
	result = ptr->window;
	return result;
}// ReturnNSWindow


/*!
Returns the number of distinct virtual terminal screen
buffers in the given terminal window.  For example,
even if the window contains 3 split-pane views of the
same screen buffer, the result will still be 1.

Currently, MacTerm only has one screen per window,
so the return value will always be 1.  However, if your
code *could* vary depending on the number of screens in
a window, you should use this API along with
TerminalWindow_GetScreens() to iterate properly now, to
ensure correct behavior in the future.

(3.0)
*/
UInt16
TerminalWindow_ReturnScreenCount	(TerminalWindowRef		inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	UInt16							result = 0;
	
	
	result = ptr->allScreens.size();
	return result;
}// ReturnScreenCount


/*!
Returns a reference to the virtual terminal that has most
recently had keyboard focus in the given terminal window.
Thus, a valid reference is returned even if no terminal
screen control has the keyboard focus.

WARNING:	MacTerm is going to change in the future to
			support multiple screens per window.  Be sure
			to use TerminalWindow_GetScreens() instead of
			this routine if it is appropriate to iterate
			over all screens in a window.

(3.0)
*/
TerminalScreenRef
TerminalWindow_ReturnScreenWithFocus	(TerminalWindowRef	inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalScreenRef				result = nullptr;
	
	
	result = getActiveScreen(ptr);
	return result;
}// ReturnScreenWithFocus


/*!
Returns the Mac OS window reference for the tab drawer that
is sometimes attached to a terminal window.

IMPORTANT:	This is not for general use.  It is an accessor
			temporarily required to enable alpha-channel
			changes, and will probably go way.

(4.0)
*/
HIWindowRef
TerminalWindow_ReturnTabWindow		(TerminalWindowRef	inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	HIWindowRef						result = (ptr->tab.exists())
												? REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef)
												: nullptr;
	
	
	return result;
}// ReturnTabWindow


/*!
Returns the number of distinct terminal views in the
given terminal window.  For example, if a window has a
single split, the result will be 2.

Currently, MacTerm only has one view per window, so
the return value will always be 1.  However, if your
code *could* vary depending on the number of views in
a window, you should use this API along with
TerminalWindow_GetViewsInGroup() to iterate properly
now, to ensure correct behavior in the future.

By definition, this function is equivalent to calling
TerminalWindow_ReturnViewCountInGroup() with a group
of "kTerminalWindow_ViewGroupEverything".

(3.0)
*/
UInt16
TerminalWindow_ReturnViewCount		(TerminalWindowRef		inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	UInt16							result = 0;
	
	
	result = ptr->allViews.size();
	return result;
}// ReturnViewCount


/*!
Returns the number of distinct terminal views in the
given group of the given terminal window.  Use this
to determine the length of the array you need to pass
into TerminalWindow_GetViewsInGroup().

(3.0)
*/
UInt16
TerminalWindow_ReturnViewCountInGroup	(TerminalWindowRef			inRef,
										 TerminalWindow_ViewGroup	inGroup)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	UInt16							result = 0;
	
	
	switch (inGroup)
	{
	case kTerminalWindow_ViewGroupEverything:
		result = ptr->allViews.size();
		assert(result == TerminalWindow_ReturnViewCount(inRef));
		break;
	
	case kTerminalWindow_ViewGroupActive:
		// currently, only one tab per window so the result is the same
		result = ptr->allViews.size();
		assert(result == TerminalWindow_ReturnViewCount(inRef));
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// ReturnViewCountInGroup


/*!
Returns a reference to the screen view that has most
recently had keyboard focus in the given terminal window.
Thus, a valid reference is returned even if no terminal
screen control has the keyboard focus.

WARNING:	MacTerm is going to change in the future to
			support multiple views per window.  Be sure
			to use TerminalWindow_GetViews() instead of
			this routine if it is appropriate to iterate
			over all views in a window.

(3.0)
*/
TerminalViewRef
TerminalWindow_ReturnViewWithFocus		(TerminalWindowRef	inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalViewRef					result = nullptr;
	
	
	result = getActiveView(ptr);
	return result;
}// ReturnViewWithFocus


/*!
Returns the Mac OS window reference for the specified
terminal window.

DEPRECATED.  You should generally manipulate the Cocoa window,
if anything (which can also be used to find the Carbon window).
See TerminalWindow_ReturnNSWindow().

IMPORTANT:	If an API exists to manipulate a terminal
			window, use the Terminal Window API; only
			use the Mac OS window reference when
			absolutely necessary.

(3.0)
*/
WindowRef
TerminalWindow_ReturnWindow		(TerminalWindowRef	inRef)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	WindowRef						result = nullptr;
	
	
	result = returnCarbonWindow(ptr);
	return result;
}// ReturnWindow


/*!
Puts a terminal window in front of other windows.  For
convenience, if "inFocus" is true, TerminalWindow_Focus() is
also called (which is commonly required at the same time).

See also TerminalWindow_Focus() and TerminalWindow_IsFocused().

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.  All calls to the Carbon SelectWindow(),
that had been using TerminalWindow_ReturnWindow(), should
DEFINITELY change to call this routine, instead (which
manipulates the Cocoa window internally).

(4.0)
*/
void
TerminalWindow_Select	(TerminalWindowRef	inRef,
						 Boolean			inFocus)
{
	AutoPool						_;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	[ptr->window orderFront:nil];
	if (inFocus)
	{
		TerminalWindow_Focus(inRef);
	}
}// Select


/*!
Changes the font and/or size used by all screens in
the specified terminal window.  If the font name is
nullptr, the font is not changed.  If the size is 0,
the size is not changed.

The font and size are currently tied to the window
dimensions, so adjusting these parameters will force
the window to resize to use the new space.  In the
future, it may make more sense to leave the user’s
chosen size intact (at least, when the new view size
will fit within the current window).

IMPORTANT:	This API is under evaluation.  It does
			not allow for the possibility of more
			than one terminal view per window, in
			the sense that each view theoretically
			can have its own font and size.

See also setViewFormatPreferences().

(3.0)
*/
void
TerminalWindow_SetFontAndSize	(TerminalWindowRef		inRef,
								 ConstStringPtr			inFontFamilyNameOrNull,
								 UInt16					inFontSizeOrZero)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalViewRef					activeView = getActiveView(ptr);
	TerminalView_DisplayMode		oldMode = kTerminalView_DisplayModeNormal;
	TerminalView_Result				viewResult = kTerminalView_ResultOK;
	
	
	// update terminal screen font attributes; temporarily change the
	// view mode to allow this, since the view might automatically
	// be controlling its font size
	oldMode = TerminalView_ReturnDisplayMode(activeView);
	viewResult = TerminalView_SetDisplayMode(activeView, kTerminalView_DisplayModeNormal);
	assert(kTerminalView_ResultOK == viewResult);
	viewResult = TerminalView_SetFontAndSize(activeView, inFontFamilyNameOrNull, inFontSizeOrZero);
	assert(kTerminalView_ResultOK == viewResult);
	viewResult = TerminalView_SetDisplayMode(activeView, oldMode);
	assert(kTerminalView_ResultOK == viewResult);
	
	// IMPORTANT: this window adjustment should match setViewFormatPreferences()
	unless (ptr->viewSizeIndependent)
	{
		setWindowToIdealSizeForFont(ptr);
	}
}// SetFontAndSize


/*!
Set to "true" if you want to hide the specified window
(as in the “Hide Front Window” command).  An obscured
window is invisible to the user but technically
considered a visible window.

If "inIsHidden" is false, the window is automatically
restored in every way (e.g. unminimized from the Dock).

IMPORTANT:	Currently, this function ought to be the
			preferred way to show a terminal window,
			otherwise there are corner cases where a
			window could become visible and usable but
			still be “marked” as hidden.  This should
			be reimplemented to use window event
			handlers so that the obscured state is
			corrected whenever a window is redisplayed
			(in any way).

(3.0)
*/
void
TerminalWindow_SetObscured	(TerminalWindowRef	inRef,
							 Boolean			inIsHidden)
{
	AutoPool						_;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (ptr->isObscured != inIsHidden)
	{
		ptr->isObscured = inIsHidden;
		if (inIsHidden)
		{
			// hide the window and notify listeners of the event (that ought to trigger
			// actions such as a zoom rectangle effect, updating Window menu items, etc.)
			[ptr->window orderOut:nil];
			
			// notify interested listeners about the change in state
			changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeObscuredState, ptr->selfRef/* context */);
		}
		else
		{
			// show the window and notify listeners of the event (that ought to trigger
			// actions such as updating Window menu items, etc.)
			[ptr->window makeKeyAndOrderFront:nil];
			
			// also restore the window if it was collapsed/minimized
			if ([ptr->window isMiniaturized]) [ptr->window deminiaturize:nil];
			
			// notify interested listeners about the change in state
			changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeObscuredState, ptr->selfRef/* context */);
		}
	}
}// SetObscured


/*!
Changes the dimensions of the visible screen area in the
given terminal window.  The window is resized accordingly.

See also setScreenPreferences().

(3.0)
*/
void
TerminalWindow_SetScreenDimensions	(TerminalWindowRef	inRef,
									 UInt16				inNewColumnCount,
									 UInt16				inNewRowCount,
									 Boolean			UNUSED_ARGUMENT(inSendToRecordingScripts))
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalScreenRef				activeScreen = getActiveScreen(ptr);
	
	
	Terminal_SetVisibleScreenDimensions(activeScreen, inNewColumnCount, inNewRowCount);
	
	// IMPORTANT: this window adjustment should match setScreenPreferences()
	unless (ptr->viewSizeIndependent)
	{
		setWindowToIdealSizeForDimensions(ptr, inNewColumnCount, inNewRowCount);
	}
}// SetScreenDimensions


/*!
Renames a terminal window’s minimized Dock tile, notifying
listeners that the window title has changed.

(3.0)
*/
void
TerminalWindow_SetIconTitle		(TerminalWindowRef	inRef,
								 CFStringRef		inName)
{
	AutoPool						_;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	[ptr->window setMiniwindowTitle:(NSString*)inName];
	changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeIconTitle, ptr->selfRef/* context */);
}// SetIconTitle


/*!
Creates a sister window that appears to be attached to the
specified terminal window, acting as its tab.  This is a
visual adornment only; you typically use this for more than
one terminal window and then place them into a window group
that ensures only one is visible at a time.

IMPORTANT:	You should only call this routine on visible
			terminal windows, otherwise the tab may not be
			displayed properly.  The result will only be
			"noErr" if the tab is properly displayed.

Note that since this affects only a single window, this is
not the proper API for general tab manipulation; it is a
low-level routine.  See the Workspace module.

(3.1)
*/
OSStatus
TerminalWindow_SetTabAppearance		(TerminalWindowRef		inRef,
									 Boolean				inIsTab)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	OSStatus						result = noErr;
	
	
	if (inIsTab)
	{
		HIWindowRef		tabWindow = nullptr;
		Boolean			isNew = false;
		
		
		// create a sister tab window and group it with the terminal window
		if (false == ptr->tab.exists())
		{
			Boolean		createOK = createTabWindow(ptr);
			
			
			assert(createOK);
			assert(ptr->tab.exists());
			isNew = true;
		}
		
		tabWindow = REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef);
		
		if (isNew)
		{
			// update the tab display to match the window title
			TerminalWindow_SetWindowTitle(inRef, ptr->baseTitleString.returnCFStringRef());
			
			// attach the tab to the top edge of the window
			result = SetDrawerParent(tabWindow, TerminalWindow_ReturnWindow(inRef));
			if (noErr == result)
			{
				OptionBits				preferredEdge = kWindowEdgeTop;
				Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagWindowTabPreferredEdge,
																			sizeof(preferredEdge), &preferredEdge);
				
				
				if (kPreferences_ResultOK != prefsResult)
				{
					preferredEdge = kWindowEdgeTop;
				}
				result = SetDrawerPreferredEdge(tabWindow, preferredEdge);
			}
		}
		else
		{
			// note that the drawer is NOT opened when it is first created,
			// above, because this would have the undesirable visual side effect
			// of an oversized tab sliding out in the wrong position; instead,
			// this function is called again, later; at that point, the tab is
			// no longer new, and is opened here at the correct size
			if (kWindowDrawerOpen != GetDrawerState(tabWindow))
			{
				// IMPORTANT: This will return paramErr if the window is invisible.
				result = OpenDrawer(tabWindow, kWindowEdgeDefault, false/* asynchronously */);
			}
		}
	}
	else
	{
		// remove window from group and destroy tab
		HIWindowRef		tabWindow = REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef);
		
		
		if (ptr->tab.exists())
		{
			// IMPORTANT: This will return paramErr if the window is invisible.
			result = CloseDrawer(tabWindow, false/* asynchronously */);
		}
	}
	return result;
}// SetTabAppearance


/*!
Specifies the position of the tab (if any) for this window, and
optionally its width; set the width to FLT_MAX to auto-size.

This is a visual adornment only; you typically use this when
windows are grouped and you want all tabs to be visible at
the same time.

WARNING:	Prior to Snow Leopard, the Mac OS X window manager
			will not allow a drawer to be cut off, and it solves
			this problem by resizing the *parent* (terminal)
			window to make room for the tab.  If you do not want
			this behavior, you need to check in advance how
			large the window is, and what a reasonable tab
			placement would be.

Note that since this affects only a single window, this is not
the proper API for general tab manipulation; it is a low-level
routine.  See the Workspace module.

\retval kTerminalWindow_ResultOK
if there are no errors

\retval kTerminalWindow_ResultInvalidReference
if the specified terminal window is unrecognized

\retval kTerminalWindow_ResultGenericFailure
if the specified terminal window has no tab (however,
the proper offset is still remembered)

(3.1)
*/
TerminalWindow_Result
TerminalWindow_SetTabPosition	(TerminalWindowRef	inRef,
								 Float32			inOffsetFromStartingPointInPixels,
								 Float32			inWidthInPixelsOrFltMax)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result			result = kTerminalWindow_ResultOK;
	
	
	if (gTerminalWindowValidRefs().end() == gTerminalWindowValidRefs().find(inRef))
	{
		result = kTerminalWindow_ResultInvalidReference;
		Console_Warning(Console_WriteValueAddress, "attempt to TerminalWindow_SetTabPosition() with invalid reference", inRef);
	}
	else
	{
		Float32 const	kWidth = (FLT_MAX == inWidthInPixelsOrFltMax) ? ptr->tabSizeInPixels : inWidthInPixelsOrFltMax;
		
		
		ptr->tabOffsetInPixels = inOffsetFromStartingPointInPixels;
		
		// “setting the width” has the side effect of putting the tab in the right place
		result = TerminalWindow_SetTabWidth(inRef, kWidth);
	}
	return result;
}// SetTabPosition


/*!
Specifies the size of the tab (if any) for this window,
including any frame it has.  This is a visual adornment
only; see also TerminalWindow_SetTabPosition().

Currently, for tabs attached to the left or right edges of
a window, the specified width may be ignored (not even
used as a height); these tabs tend to have uniform size.

You can pass "kTerminalWindow_DefaultMetaTabWidth" to
indicate that the tab should be resized to its ordinary
(default) width or height.

Note that since this affects only a single window, this is
not the proper API for general tab manipulation; it is a
low-level routine.  See the Workspace module.

\retval kTerminalWindow_ResultOK
if there are no errors

\retval kTerminalWindow_ResultInvalidReference
if the specified terminal window is unrecognized

\retval kTerminalWindow_ResultGenericFailure
if the specified terminal window has no tab (however,
the proper width is still remembered)

(3.1)
*/
TerminalWindow_Result
TerminalWindow_SetTabWidth	(TerminalWindowRef	inRef,
							 Float32			inWidthInPixels)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result			result = kTerminalWindow_ResultOK;
	
	
	if (gTerminalWindowValidRefs().end() == gTerminalWindowValidRefs().find(inRef))
	{
		result = kTerminalWindow_ResultInvalidReference;
		Console_Warning(Console_WriteValueAddress, "attempt to TerminalWindow_SetTabWidth() with invalid reference", inRef);
	}
	else
	{
		if (false == ptr->tab.exists())
		{
			result = kTerminalWindow_ResultGenericFailure;
		}
		else
		{
			// drawers are managed in terms of start and end offsets as opposed to
			// a “width”, so some roundabout calculations are done to find offsets
			HIWindowRef				tabWindow = REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef);
			HIWindowRef				parentWindow = GetDrawerParent(tabWindow);
			Rect					currentParentBounds;
			OSStatus				error = noErr;
			float					leadingOffset = kWindowOffsetUnchanged;
			OptionBits				preferredEdge = kWindowEdgeTop;
			Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagWindowTabPreferredEdge,
																		sizeof(preferredEdge), &preferredEdge);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				preferredEdge = kWindowEdgeTop;
			}
			
			// the tab width must refer to the structure region of the tab window,
			// however the total space available to tabs is limited to the width
			// of the parent window’s content region, not its structure region
			error = GetWindowBounds(parentWindow, kWindowContentRgn, &currentParentBounds);
			assert_noerr(error);
			leadingOffset = STATIC_CAST(ptr->tabOffsetInPixels, float);
			if ((kWindowEdgeLeft == preferredEdge) || (kWindowEdgeRight == preferredEdge))
			{
				// currently, vertically-stacked tabs are all the same size
				ptr->tabSizeInPixels = gDefaultTabHeight;
			}
			else
			{
				ptr->tabSizeInPixels = inWidthInPixels;
			}
			
			// ensure that the drawer stays at its assigned size; but note that the
			// given tab width refers to the entire structure, whereas the constrained
			// dimensions are only for the interior (content region)
			{
				Float32		width = 0.0;
				Float32		height = 0.0;
				Rect		borderWidths;
				
				
				error = GetWindowStructureWidths(tabWindow, &borderWidths);
				assert_noerr(error);
				error = ptr->tabDrawerWindowResizeHandler.getWindowMaximumSize(width, height);
				assert_noerr(error);
				if ((kWindowEdgeTop == preferredEdge) || (kWindowEdgeBottom == preferredEdge))
				{
					ptr->tabDrawerWindowResizeHandler.setWindowMinimumSize
														(ptr->tabSizeInPixels - borderWidths.right - borderWidths.left, height);
					ptr->tabDrawerWindowResizeHandler.setWindowMaximumSize
														(ptr->tabSizeInPixels - borderWidths.right - borderWidths.left, height);
				}
				else
				{
					ptr->tabDrawerWindowResizeHandler.setWindowMinimumSize
														(width, ptr->tabSizeInPixels - borderWidths.bottom - borderWidths.top);
					ptr->tabDrawerWindowResizeHandler.setWindowMaximumSize
														(width, ptr->tabSizeInPixels - borderWidths.bottom - borderWidths.top);
				}
			}
			
			// resize the drawer; setting the trailing offset would be
			// counterproductive, because it would force the parent window
			// to remain relatively wide (or high, for left/right tabs)
			error = SetDrawerOffsets(tabWindow, leadingOffset, kWindowOffsetUnchanged);
			
			// force a “resize” to cause the tab position to update immediately
			// (TEMPORARY: is there a better way to do this?)
			if ((kWindowEdgeTop == preferredEdge) || (kWindowEdgeBottom == preferredEdge))
			{
				++currentParentBounds.right;
				SetWindowBounds(parentWindow, kWindowContentRgn, &currentParentBounds);
				--currentParentBounds.right;
				SetWindowBounds(parentWindow, kWindowContentRgn, &currentParentBounds);
			}
			else
			{
				++currentParentBounds.bottom;
				SetWindowBounds(parentWindow, kWindowContentRgn, &currentParentBounds);
				--currentParentBounds.bottom;
				SetWindowBounds(parentWindow, kWindowContentRgn, &currentParentBounds);
			}
		}
	}
	return result;
}// SetTabWidth


/*!
Renames a terminal window, notifying listeners that the
window title has changed.

The value of "inName" can be nullptr if you want the current
base title to be unchanged, but you want adornments to be
evaluated again (an updated session status, for instance).

(3.0)
*/
void
TerminalWindow_SetWindowTitle	(TerminalWindowRef	inRef,
								 CFStringRef		inName)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (nullptr != inName)
	{
		ptr->baseTitleString.setCFTypeRef(inName);
	}
	
	if (nil != ptr->window)
	{
		if (ptr->isDead)
		{
			// add a visual indicator to the window title of disconnected windows
			CFRetainRelease		adornedCFString(CFStringCreateWithFormat
												(kCFAllocatorDefault, nullptr/* format options */,
													CFSTR("~ %@ ~")/* LOCALIZE THIS? */,
													ptr->baseTitleString.returnCFStringRef()),
												true/* is retained */);
			
			
			if (adornedCFString.exists())
			{
				setWindowAndTabTitle(ptr, adornedCFString.returnCFStringRef());
			}
		}
		else if (nullptr != inName)
		{
			setWindowAndTabTitle(ptr, ptr->baseTitleString.returnCFStringRef());
		}
	}
	changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeWindowTitle, ptr->selfRef/* context */);
}// SetWindowTitle


/*!
Set to "true" to show a terminal window, and "false" to hide it.

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.  All calls to the Carbon ShowWindow() or
HideWindow(), that had been using TerminalWindow_ReturnWindow(),
should DEFINITELY change to call this routine, instead (which
manipulates the Cocoa window internally).

(4.0)
*/
void
TerminalWindow_SetVisible	(TerminalWindowRef	inRef,
							 Boolean			inIsVisible)
{
	AutoPool						_;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (inIsVisible)
	{
		[ptr->window orderFront:nil];
	}
	else
	{
		[ptr->window orderOut:nil];
	}
}// SetVisible


/*!
Rearranges all terminal windows so that their top-left
corners form an invisible, diagonal line.  On Mac OS X,
the effect is also animated, showing each window sliding
into its new position.

(3.0)
*/
void
TerminalWindow_StackWindows ()
{
	gNumberOfTransitioningWindows = 0;
	SessionFactory_ForEveryTerminalWindowDo(stackWindowTerminalWindowOp, 0L/* data 1 - unused */, 0L/* data 2 - unused */,
											nullptr/* pointer to result - unused */);
}// StackWindows


/*!
Arranges for a callback to be invoked whenever a setting
changes for a terminal window (such as its screen size).

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "TerminalWindow.h" for
			comments on what the context means for each
			type of change.

(3.0)
*/
void
TerminalWindow_StartMonitoring	(TerminalWindowRef			inRef,
								 TerminalWindow_Change		inForWhatChange,
								 ListenerModel_ListenerRef	inListener)
{
	OSStatus						error = noErr;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	// add a listener to the specified target’s listener model for the given setting change
	error = ListenerModel_AddListenerForEvent(ptr->changeListenerModel, inForWhatChange, inListener);
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked whenever
a setting changes for a terminal window (such as its
screen size).

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "TerminalWindow.h" for
			comments on what the context means for each
			type of change.

(3.0)
*/
void
TerminalWindow_StopMonitoring	(TerminalWindowRef			inRef,
								 TerminalWindow_Change		inForWhatChange,
								 ListenerModel_ListenerRef	inListener)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	// add a listener to the specified target’s listener model for the given setting change
	ListenerModel_RemoveListenerForEvent(ptr->changeListenerModel, inForWhatChange, inListener);
}// StopMonitoring


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See TerminalWindow_New().

(3.0)
*/
My_TerminalWindow::
My_TerminalWindow	(Preferences_ContextRef		inTerminalInfoOrNull,
					 Preferences_ContextRef		inFontInfoOrNull,
					 Preferences_ContextRef		inTranslationInfoOrNull,
					 Boolean					inNoStagger)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
refValidator(REINTERPRET_CAST(this, TerminalWindowRef), gTerminalWindowValidRefs()),
selfRef(REINTERPRET_CAST(this, TerminalWindowRef)),
changeListenerModel(ListenerModel_New(kListenerModel_StyleStandard,
										kConstantsRegistry_ListenerModelDescriptorTerminalWindowChanges)),
window(createWindow()),
tab(),
tabContextualMenuHandlerPtr(nullptr),
tabDragHandlerPtr(nullptr),
tabAndWindowGroup(nullptr),
tabOffsetInPixels(0.0),
tabSizeInPixels(0.0),
toolbar(nullptr),
toolbarItemBell(),
toolbarItemLED1(),
toolbarItemLED2(),
toolbarItemLED3(),
toolbarItemLED4(),
toolbarItemScrollLock(),
resizeFloater(nullptr),
preResizeViewDisplayMode(kTerminalView_DisplayModeNormal/* corrected below */),
windowInfo(WindowInfo_New()),
// controls initialized below
// toolbar initialized below
staggerPositionIndex(0),
isObscured(false),
isDead(false),
viewSizeIndependent(false),
recentSheetContext(nullptr),
sheetType(kMy_SheetTypeNone),
recentSearchOptions(kFindDialog_OptionsDefault),
recentSearchStrings(CFArrayCreateMutable(kCFAllocatorDefault, 0/* limit; 0 = no size limit */, &kCFTypeArrayCallBacks),
					true/* is retained */),
baseTitleString(),
scrollProcUPP(nullptr), // reset below
windowResizeHandler(),
tabDrawerWindowResizeHandler(),
mouseWheelHandler(GetApplicationEventTarget(), receiveMouseWheelEvent,
					CarbonEventSetInClass(CarbonEventClass(kEventClassMouse), kEventMouseWheelMoved),
					this->selfRef/* user data */),
scrollTickHandler(), // see createViews()
commandUPP(nullptr),
commandHandler(nullptr),
windowClickActivationUPP(nullptr),
windowClickActivationHandler(nullptr),
windowCursorChangeUPP(nullptr),
windowCursorChangeHandler(nullptr),
windowResizeEmbellishUPP(nullptr),
windowResizeEmbellishHandler(nullptr),
growBoxClickUPP(nullptr),
growBoxClickHandler(nullptr),
toolbarEventUPP(nullptr),
toolbarEventHandler(nullptr),
sessionStateChangeEventListener(nullptr),
terminalStateChangeEventListener(nullptr),
terminalViewEventListener(nullptr),
toolbarStateChangeEventListener(nullptr),
screensToViews(),
viewsToScreens(),
allScreens(),
allViews(),
installedActions()
{
	AutoPool				_;
	TerminalScreenRef		newScreen = nullptr;
	TerminalViewRef			newView = nullptr;
	Preferences_Result		preferencesResult = kPreferences_ResultOK;
	
	
	// get defaults if no contexts provided; if these cannot be found
	// for some reason, that’s fine because defaults are set in case
	// of errors later on
	if (nullptr == inTerminalInfoOrNull)
	{
		preferencesResult = Preferences_GetDefaultContext(&inTerminalInfoOrNull, Quills::Prefs::TERMINAL);
		assert(kPreferences_ResultOK == preferencesResult);
		assert(nullptr != inTerminalInfoOrNull);
	}
	if (nullptr == inTranslationInfoOrNull)
	{
		preferencesResult = Preferences_GetDefaultContext(&inTranslationInfoOrNull, Quills::Prefs::TRANSLATION);
		assert(kPreferences_ResultOK == preferencesResult);
		assert(nullptr != inTranslationInfoOrNull);
	}
	if (nullptr == inFontInfoOrNull)
	{
		Boolean		chooseRandom = false;
		
		
		(Preferences_Result)Preferences_GetData(kPreferences_TagRandomTerminalFormats, sizeof(chooseRandom), &chooseRandom);
		if (chooseRandom)
		{
			std::vector< Preferences_ContextRef >	contextList;
			
			
			if (Preferences_GetContextsInClass(Quills::Prefs::FORMAT, contextList))
			{
				std::vector< UInt16 >	numberList(contextList.size());
				RandomWrap				generator;
				
				
				__gnu_cxx::iota(numberList.begin(), numberList.end(), 0/* starting value */);
				std::random_shuffle(numberList.begin(), numberList.end(), generator);
				inFontInfoOrNull = contextList[numberList[0]];
			}
			
			if (nullptr == inFontInfoOrNull) chooseRandom = false; // error...
		}
		
		if (false == chooseRandom)
		{
			preferencesResult = Preferences_GetDefaultContext(&inFontInfoOrNull, Quills::Prefs::FORMAT);
			assert(kPreferences_ResultOK == preferencesResult);
			assert(nullptr != inFontInfoOrNull);
		}
	}
	
	// set up Window Info; it is important to do this right away
	// because this is relied upon by other code to find the
	// terminal window data attached to the Mac OS window
	assert(this->window != nil);
	gTerminalNSWindows()[this->window] = this->selfRef;
	WindowInfo_SetWindowDescriptor(this->windowInfo, kConstantsRegistry_WindowDescriptorAnyTerminal);
	WindowInfo_SetAuxiliaryDataPtr(this->windowInfo, REINTERPRET_CAST(this, TerminalWindowRef)); // the auxiliary data is the "TerminalWindowRef"
	WindowInfo_SetForWindow(returnCarbonWindow(this), this->windowInfo);
	
	// set up the Help System
	HelpSystem_SetWindowKeyPhrase(returnCarbonWindow(this), kHelpSystem_KeyPhraseTerminals);
	
#if 0
	// on 10.4, use the special unified toolbar appearance
	if (FlagManager_Test(kFlagOS10_4API))
	{
		(OSStatus)ChangeWindowAttributes
					(this->window,
						FUTURE_SYMBOL(1 << 7, kWindowUnifiedTitleAndToolbarAttribute)/* attributes to set */,
						0/* attributes to clear */);
	}
#endif
	
	// install a callback that responds as a window is resized
	this->windowResizeHandler.install(returnCarbonWindow(this), handleNewSize, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
										250/* arbitrary minimum width */,
										200/* arbitrary minimum height */,
										SHRT_MAX/* maximum width */,
										SHRT_MAX/* maximum height */);
	assert(this->windowResizeHandler.isInstalled());
	
	// create controls
	{
		Terminal_Result		terminalError = kTerminal_ResultOK;
		
		
		terminalError = Terminal_NewScreen(inTerminalInfoOrNull, inTranslationInfoOrNull, &newScreen);
		if (terminalError == kTerminal_ResultOK)
		{
			newView = TerminalView_NewHIViewBased(newScreen, inFontInfoOrNull);
			if (newView != nullptr)
			{
				HIViewWrap		contentView(kHIViewWindowContentID, returnCarbonWindow(this));
				HIViewRef		terminalHIView = TerminalView_ReturnContainerHIView(newView);
				OSStatus		error = noErr;
				
				
				assert(contentView.exists());
				error = HIViewAddSubview(contentView, terminalHIView);
				assert_noerr(error);
				
				error = HIViewSetVisible(terminalHIView, true);
				assert_noerr(error);
				
				// remember the initial screen-to-view and view-to-screen mapping;
				// later, additional views or screens may be added
				this->screensToViews.insert(std::pair< TerminalScreenRef, TerminalViewRef >(newScreen, newView));
				this->viewsToScreens.insert(std::pair< TerminalViewRef, TerminalScreenRef >(newView, newScreen));
				this->allScreens.push_back(newScreen);
				this->allViews.push_back(newView);
				assert(this->screensToViews.find(newScreen) != this->screensToViews.end());
				assert(this->viewsToScreens.find(newView) != this->viewsToScreens.end());
				assert(!this->allScreens.empty());
				assert(this->allScreens.back() == newScreen);
				assert(!this->allViews.empty());
				assert(this->allViews.back() == newView);
			}
		}
	}
	createViews(this);
	
	// create toolbar icons
	if (noErr == HIToolbarCreate(kConstantsRegistry_HIToolbarIDTerminal,
									kHIToolbarAutoSavesConfig | kHIToolbarIsConfigurable, &this->toolbar))
	{
		// IMPORTANT: Do not invoke toolbar manipulation APIs at this stage,
		// until the event handlers below are installed.  A saved toolbar may
		// contain references to items that only the handlers below can create;
		// manipulation APIs often trigger creation of the entire toolbar, so
		// that means some saved items would fail to be inserted properly.
		
		// install a callback that can create any items needed for this
		// toolbar (used in the toolbar and in the customization sheet, etc.);
		// and, a callback that specifies which items are in the toolbar by
		// default, and which items are available in the customization sheet
		{
			EventTypeSpec const		whenToolbarEventOccurs[] =
									{
										{ kEventClassToolbar, kEventToolbarCreateItemWithIdentifier },
										{ kEventClassToolbar, kEventToolbarGetAllowedIdentifiers },
										{ kEventClassToolbar, kEventToolbarGetDefaultIdentifiers },
										{ kEventClassToolbar, kEventToolbarItemRemoved }
									};
			OSStatus				error = noErr;
			
			
			this->toolbarEventUPP = NewEventHandlerUPP(receiveToolbarEvent);
			error = InstallEventHandler(HIObjectGetEventTarget(this->toolbar), this->toolbarEventUPP,
										GetEventTypeCount(whenToolbarEventOccurs), whenToolbarEventOccurs,
										REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
										&this->toolbarEventHandler/* event handler reference */);
			assert_noerr(error);
		}
		
		// Check preferences for a stored toolbar; if one exists, leave the
		// toolbar display mode and size untouched, as the user will have
		// specified one; otherwise, initialize it to the desired look.
		//
		// IMPORTANT: This is a bit of a hack, as it relies on the key name
		// that Mac OS X happens to use for toolbar preferences as of 10.4.
		// If that ever changes, this code will be pointless.
		CFPropertyListRef	toolbarConfigPref = CFPreferencesCopyAppValue
												(CFSTR("HIToolbar Config com.mactelnet.toolbar.terminal"),
													kCFPreferencesCurrentApplication);
		Boolean				usingSavedToolbar = false;
		if (nullptr != toolbarConfigPref)
		{
			usingSavedToolbar = true;
			CFRelease(toolbarConfigPref), toolbarConfigPref = nullptr;
		}
		unless (usingSavedToolbar)
		{
			(OSStatus)HIToolbarSetDisplayMode(this->toolbar, kHIToolbarDisplayModeIconAndLabel);
			(OSStatus)HIToolbarSetDisplaySize(this->toolbar, kHIToolbarDisplaySizeSmall);
		}
		
		// the toolbar is NOT used yet and NOT released yet, until it is installed (below)
	}
	
	// set the standard state to be large enough for the specified number of columns and rows;
	// and, use the standard size, initially; then, perform a maximize/restore to correct the
	// initial-zoom quirk that would otherwise occur
	assert(nullptr != newScreen);
	if (nullptr != newScreen)
	{
		SInt16		screenWidth = 0;
		SInt16		screenHeight = 0;
		
		
		TerminalView_GetTheoreticalViewSize(getActiveView(this)/* TEMPORARY - must consider a list of views */,
											Terminal_ReturnColumnCount(newScreen), Terminal_ReturnRowCount(newScreen),
											&screenWidth, &screenHeight);
		setStandardState(this, screenWidth, screenHeight, true/* resize window */);
	}
	
	// stagger the window
	unless (inNoStagger)
	{
		Rect		windowRect;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(returnCarbonWindow(this), kWindowContentRgn, &windowRect);
		assert_noerr(error);
		calculateWindowPosition(this, &windowRect);
		error = SetWindowBounds(returnCarbonWindow(this), kWindowContentRgn, &windowRect);
		assert_noerr(error);
	}
	
	// set up callbacks to receive various state change notifications
	this->sessionStateChangeEventListener.setRef(ListenerModel_NewStandardListener(sessionStateChanged, this->selfRef/* context */),
													true/* is retained */);
	SessionFactory_StartMonitoringSessions(kSession_ChangeSelected, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StartMonitoringSessions(kSession_ChangeStateAttributes, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowTitle, this->sessionStateChangeEventListener.returnRef());
	this->terminalStateChangeEventListener.setRef(ListenerModel_NewStandardListener
													(terminalStateChanged, REINTERPRET_CAST(this, TerminalWindowRef)/* context */),
													true/* is retained */);
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeAudioState, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeExcessiveErrors, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeNewLEDState, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeScrollActivity, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowFrameTitle, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowIconTitle, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowMinimization, this->terminalStateChangeEventListener.returnRef());
	this->terminalViewEventListener.setRef(ListenerModel_NewStandardListener
											(terminalViewStateChanged, REINTERPRET_CAST(this, TerminalWindowRef)/* context */),
											true/* is retained */);
	TerminalView_StartMonitoring(newView, kTerminalView_EventScrolling, this->terminalViewEventListener.returnRef());
	TerminalView_StartMonitoring(newView, kTerminalView_EventSearchResultsExistence, this->terminalViewEventListener.returnRef());
	
	// install a callback that handles commands relevant to terminal windows
	{
		EventTypeSpec const		whenCommandExecuted[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		OSStatus				error = noErr;
		
		
		this->commandUPP = NewEventHandlerUPP(receiveHICommand);
		error = InstallWindowEventHandler(returnCarbonWindow(this), this->commandUPP, GetEventTypeCount(whenCommandExecuted),
											whenCommandExecuted, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->commandHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that tells the Window Manager the proper behavior for clicks in background terminal windows
#if 0
	// this behavior is disabled temporarily under the hybrid environment with a Cocoa
	// runtime; it will be restored when terminals become full-fledged Cocoa windows
	{
		EventTypeSpec const		whenWindowClickActivationRequired[] =
								{
									{ kEventClassWindow, kEventWindowGetClickActivation }
								};
		OSStatus				error = noErr;
		
		
		this->windowClickActivationUPP = NewEventHandlerUPP(receiveWindowGetClickActivation);
		error = InstallWindowEventHandler(returnCarbonWindow(this), this->windowClickActivationUPP, GetEventTypeCount(whenWindowClickActivationRequired),
											whenWindowClickActivationRequired, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->windowClickActivationHandler/* event handler reference */);
		assert_noerr(error);
	}
#endif
	
	// install a callback that attempts to fix tab locations after a window is moved far enough below the menu bar
	{
		EventTypeSpec const		whenWindowDragCompleted[] =
								{
									{ kEventClassWindow, kEventWindowDragCompleted }
								};
		OSStatus				error = noErr;
		
		
		this->windowDragCompletedUPP = NewEventHandlerUPP(receiveWindowDragCompleted);
		error = InstallWindowEventHandler(returnCarbonWindow(this), this->windowDragCompletedUPP, GetEventTypeCount(whenWindowDragCompleted),
											whenWindowDragCompleted, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->windowDragCompletedHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that changes the mouse cursor appropriately
	{
		EventTypeSpec const		whenCursorChangeRequired[] =
								{
									{ kEventClassWindow, kEventWindowCursorChange },
									{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
								};
		OSStatus				error = noErr;
		
		
		this->windowCursorChangeUPP = NewEventHandlerUPP(receiveWindowCursorChange);
		error = InstallWindowEventHandler(returnCarbonWindow(this), this->windowCursorChangeUPP, GetEventTypeCount(whenCursorChangeRequired),
											whenCursorChangeRequired, this->window/* user data */,
											&this->windowCursorChangeHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that uses the title bar to display terminal dimensions during resize
	{
		EventTypeSpec const		whenWindowResizeStartsContinuesOrStops[] =
								{
									{ kEventClassWindow, kEventWindowResizeStarted },
									{ kEventClassWindow, kEventWindowBoundsChanging },
									{ kEventClassWindow, kEventWindowResizeCompleted }
								};
		OSStatus				error = noErr;
		
		
		this->windowResizeEmbellishUPP = NewEventHandlerUPP(receiveWindowResize);
		error = InstallWindowEventHandler(returnCarbonWindow(this), this->windowResizeEmbellishUPP, GetEventTypeCount(whenWindowResizeStartsContinuesOrStops),
											whenWindowResizeStartsContinuesOrStops, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->windowResizeEmbellishHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// put the toolbar in the window
	{
		OSStatus	error = noErr;
		size_t		actualSize = 0L;
		Boolean		headersCollapsed = false;
		
		
		error = SetWindowToolbar(returnCarbonWindow(this), this->toolbar);
		assert_noerr(error);
		
		// also show the toolbar, unless the user preference to collapse is set
		unless (Preferences_GetData(kPreferences_TagHeadersCollapsed, sizeof(headersCollapsed),
									&headersCollapsed, &actualSize) ==
				kPreferences_ResultOK)
		{
			headersCollapsed = false; // assume headers aren’t collapsed, if preference can’t be found
		}
		unless (headersCollapsed)
		{
			error = ShowHideWindowToolbar(returnCarbonWindow(this), true/* show */, false/* animate */);
			assert_noerr(error);
		}
	}
	CFRelease(this->toolbar); // once set in the window, the toolbar is retained, so release the creation lock
	
	// enable drag tracking so that certain default toolbar behaviors work
	(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(returnCarbonWindow(this), true/* enabled */);
	
	// finish by applying any desired attributes to the screen
	{
		Boolean		flag = false;
		
		
		preferencesResult = Preferences_ContextGetData(inTerminalInfoOrNull, kPreferences_TagTerminalLineWrap,
														sizeof(flag), &flag);
		if (preferencesResult != kPreferences_ResultOK) flag = false;
		if (flag)
		{
			Terminal_EmulatorProcessCString(newScreen, "\033[?7h"); // turn on autowrap
		}
	}
}// My_TerminalWindow 2-argument constructor


/*!
Destructor.  See TerminalWindow_Dispose().

(3.0)
*/
My_TerminalWindow::
~My_TerminalWindow ()
{
	AutoPool	_;
	
	
	sheetContextEnd(this);
	
	// now that the window is going away, destroy any Undo commands
	// that could be applied to this window
	{
		My_UndoableActionList::const_iterator	actionIter;
		
		
		for (actionIter = this->installedActions.begin();
				actionIter != this->installedActions.end(); ++actionIter)
		{
			Undoables_RemoveAction(*actionIter);
		}
	}
	
	// show a hidden window just before it is destroyed (most importantly, notifying callbacks)
	TerminalWindow_SetObscured(REINTERPRET_CAST(this, TerminalWindowRef), false);
	
	// remove any tab contextual menu handler
	if (nullptr != tabContextualMenuHandlerPtr) delete tabContextualMenuHandlerPtr, tabContextualMenuHandlerPtr = nullptr;
	
	// remove any tab drag handler
	if (nullptr != tabDragHandlerPtr) delete tabDragHandlerPtr, tabDragHandlerPtr = nullptr;
	
	// disable window command callback
	RemoveEventHandler(this->commandHandler), this->commandHandler = nullptr;
	DisposeEventHandlerUPP(this->commandUPP), this->commandUPP = nullptr;
	
	// disable window click activation callback
	RemoveEventHandler(this->windowClickActivationHandler), this->windowClickActivationHandler = nullptr;
	DisposeEventHandlerUPP(this->windowClickActivationUPP), this->windowClickActivationUPP = nullptr;
	
	// disable window cursor change callback
	RemoveEventHandler(this->windowCursorChangeHandler), this->windowCursorChangeHandler = nullptr;
	DisposeEventHandlerUPP(this->windowCursorChangeUPP), this->windowCursorChangeUPP = nullptr;
	
	// disable window move completion callback
	RemoveEventHandler(this->windowDragCompletedHandler), this->windowDragCompletedHandler = nullptr;
	DisposeEventHandlerUPP(this->windowDragCompletedUPP), this->windowDragCompletedUPP = nullptr;
	
	// disable window resize callback
	RemoveEventHandler(this->windowResizeEmbellishHandler), this->windowResizeEmbellishHandler = nullptr;
	DisposeEventHandlerUPP(this->windowResizeEmbellishUPP), this->windowResizeEmbellishUPP = nullptr;
	
	// disable size box click callback
	RemoveEventHandler(this->growBoxClickHandler), this->growBoxClickHandler = nullptr;
	DisposeEventHandlerUPP(this->growBoxClickUPP), this->growBoxClickUPP = nullptr;
	
	// disable toolbar callback
	RemoveEventHandler(this->toolbarEventHandler), this->toolbarEventHandler = nullptr;
	DisposeEventHandlerUPP(this->toolbarEventUPP), this->toolbarEventUPP = nullptr;
	
	// hide window and kill its controls to disable callbacks
	if (nil != this->window)
	{
		gTerminalNSWindows().erase(this->window);
		
		// this will hide the window immediately and replace it with a window
		// that looks exactly the same; that way, it is perfectly safe for
		// the rest of the destructor to run (cleaning up other state) even
		// if the animation finishes after the original window is destroyed
		CocoaAnimation_TransitionWindowForRemove(this->window);
		
		KillControls(returnCarbonWindow(this));
	}
	
	// perform other clean-up
	if (gTopLeftCorners != nullptr) --((*gTopLeftCorners)[this->staggerPositionIndex]); // one less window at this position
	
	DisposeControlActionUPP(this->scrollProcUPP), this->scrollProcUPP = nullptr;
	
	// unregister session callbacks
	SessionFactory_StopMonitoringSessions(kSession_ChangeSelected, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StopMonitoringSessions(kSession_ChangeState, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StopMonitoringSessions(kSession_ChangeStateAttributes, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowTitle, this->sessionStateChangeEventListener.returnRef());
	
	// unregister screen buffer callbacks and destroy all buffers
	// (NOTE: perhaps this should be revisited, as a future feature
	// may be to allow multiple windows to use the same buffer; if
	// that were the case, killing one window should not necessarily
	// throw out its buffer)
	{
		My_TerminalScreenList::const_iterator	screenIterator;
		
		
		for (screenIterator = this->allScreens.begin(); screenIterator != this->allScreens.end(); ++screenIterator)
		{
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeAudioState, this->terminalStateChangeEventListener.returnRef());
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeExcessiveErrors, this->terminalStateChangeEventListener.returnRef());
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeNewLEDState, this->terminalStateChangeEventListener.returnRef());
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeScrollActivity, this->terminalStateChangeEventListener.returnRef());
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeWindowFrameTitle, this->terminalStateChangeEventListener.returnRef());
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeWindowIconTitle, this->terminalStateChangeEventListener.returnRef());
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeWindowMinimization, this->terminalStateChangeEventListener.returnRef());
		}
	}
	
	// destroy all terminal views
	{
		My_TerminalViewList::const_iterator		viewIterator;
		TerminalViewRef							view = nullptr;
		
		
		for (viewIterator = this->allViews.begin(); viewIterator != this->allViews.end(); ++viewIterator)
		{
			view = *viewIterator;
			TerminalView_StopMonitoring(view, kTerminalView_EventScrolling, this->terminalViewEventListener.returnRef());
			TerminalView_StopMonitoring(view, kTerminalView_EventSearchResultsExistence, this->terminalViewEventListener.returnRef());
		}
	}
	
	// throw away information about terminal window change listeners
	ListenerModel_Dispose(&this->changeListenerModel);
	
	// finally, dispose of the window
	if (nil != this->window)
	{
		HelpSystem_SetWindowKeyPhrase(returnCarbonWindow(this), kHelpSystem_KeyPhraseDefault); // clean up
		DisposeWindow(returnCarbonWindow(this));
		[this->window close], this->window = nil;
	}
	if (nullptr == this->tabAndWindowGroup)
	{
		ReleaseWindowGroup(this->tabAndWindowGroup), this->tabAndWindowGroup = nullptr;
	}
	
	// destroy all buffers (NOTE: perhaps this should be revisited, as
	// a future feature may be to allow multiple windows to use the same
	// buffer; if that were the case, killing one window should not
	// necessarily throw out its buffer)
	{
		My_TerminalScreenList::const_iterator	screenIterator;
		std::set< TerminalScreenRef >			visitedScreens; // NOTE: not needed if Terminal module adopts retain/release
		
		
		for (screenIterator = this->allScreens.begin(); screenIterator != this->allScreens.end(); ++screenIterator)
		{
			if (visitedScreens.end() == visitedScreens.find(*screenIterator))
			{
				Terminal_DisposeScreen(*screenIterator);
				visitedScreens.insert(*screenIterator);
			}
		}
	}
}// My_TerminalWindow destructor


/*!
Adds items to the specified menu that are appropriate
for the given terminal window tab’s main view, and
fills in the given description of the content (for use
by system contextual items).

(4.0)
*/
void
addContextualMenuItemsForTab	(MenuRef		inMenu,
								 HIObjectRef	UNUSED_ARGUMENT(inView),
								 AEDesc&		UNUSED_ARGUMENT(inoutContentsDesc))
{
	ContextSensitiveMenu_Item	itemInfo;
	
	
	// set up states for all items used below
	// UNIMPLEMENTED
	
	// window-related menu items
	ContextSensitiveMenu_NewItemGroup(inMenu);
	
	ContextSensitiveMenu_InitItem(&itemInfo);
	itemInfo.commandID = kCommandTerminalNewWorkspace;
	if (Commands_IsCommandEnabled(itemInfo.commandID))
	{
		if (UIStrings_Copy(kUIStrings_ContextualMenuMoveToNewWorkspace, itemInfo.commandText).ok())
		{
			(OSStatus)ContextSensitiveMenu_AddItem(inMenu, &itemInfo); // add “Move to New Workspace”
			CFRelease(itemInfo.commandText), itemInfo.commandText = nullptr;
		}
	}
}// addContextualMenuItemsForTab


/*!
When a window is staggered, it is given an onscreen
index indicating what stagger position (relative to
the first window’s location) it occupies.  To determine
the global coordinates that the top-left corner of the
specified window would occupy if it were in the
specified position, use this method.

This routine now supports multiple monitors, although
not as well as it should; ideally, there would be a
list of stagger positions for each device, instead of
one list that crosses all devices.  I believe the
current implementation will cause positions to be
skipped when adjacent windows in the list occupy
different devices.

(3.0)
*/
void
calculateIndexedWindowPosition	(My_TerminalWindowPtr	inPtr,
								 SInt16					inStaggerIndex,
								 Point*					outPositionPtr)
{
	if ((inPtr != nullptr) && (outPositionPtr != nullptr))
	{
		// calculate the stagger offset{
		Point		stackingOrigin;
		Point		stagger;
		Rect		screenRect;
		size_t		actualSize = 0;
		
		
		// determine the user’s preferred stacking origin
		unless (Preferences_GetData(kPreferences_TagWindowStackingOrigin, sizeof(stackingOrigin),
									&stackingOrigin, &actualSize) == kPreferences_ResultOK)
		{
			SetPt(&stackingOrigin, 40, 40); // assume a default, if preference can’t be found
		}
		
		// the stagger amount on Mac OS X is set by the Aqua Human Interface Guidelines
		SetPt(&stagger, 20, 20);
		
		RegionUtilities_GetPositioningBounds(returnCarbonWindow(inPtr), &screenRect);
		outPositionPtr->v = stackingOrigin.v + stagger.v * (1 + inStaggerIndex);
		outPositionPtr->h = stackingOrigin.h + stagger.h * (1 + inStaggerIndex);
	}
}// calculateIndexedWindowPosition


/*!
Calculates the stagger position of windows.

(2.6)
*/
void
calculateWindowPosition		(My_TerminalWindowPtr	inPtr,
							 Rect*					outArrangement)
{
	Rect			contentRegionBounds;
	SInt16			currentCount = 0;
	SInt16			wideCount = 0;
	Boolean			done = false;
	Boolean			tooFarRight = false;
	Boolean			tooFarDown = false;
	Boolean			tooBig = false;
	OSStatus		error = noErr;
	
	
	ensureTopLeftCornersExists();
	inPtr->staggerPositionIndex = 0;
	error = GetWindowBounds(returnCarbonWindow(inPtr), kWindowContentRgn, &contentRegionBounds);
	assert_noerr(error);
	while ((!done) && (!tooBig))
	{
		while (((*gTopLeftCorners)[inPtr->staggerPositionIndex] > currentCount) && // find an empty spot
					(inPtr->staggerPositionIndex < kMy_MaximumNumberOfArrangedWindows - 1))
		{
			++inPtr->staggerPositionIndex;
		}
		
		{
			Point		topLeft;
			
			
			calculateIndexedWindowPosition(inPtr, inPtr->staggerPositionIndex, &topLeft);
			outArrangement->top = topLeft.v;
			outArrangement->left = topLeft.h;
		}
		if (tooFarRight) wideCount += currentCount - 1;
		outArrangement->bottom = outArrangement->top + (contentRegionBounds.bottom - contentRegionBounds.top);
		outArrangement->right = outArrangement->left + (contentRegionBounds.right - contentRegionBounds.left);
		{
			Rect	screenRect;
			
			
			RegionUtilities_GetPositioningBounds(returnCarbonWindow(inPtr), &screenRect);
			tooFarRight = ((outArrangement->left + contentRegionBounds.right - contentRegionBounds.left) > screenRect.right);
			tooFarDown = ((outArrangement->top + contentRegionBounds.bottom - contentRegionBounds.top) > screenRect.bottom);
		}
		if (tooFarRight || tooFarDown)
		{
			// then the position is off the screen
			if (inPtr->staggerPositionIndex == 0)
			{
				tooBig = true; // the window is bigger than the screen size - quit now
			}
			else
			{
				++currentCount; // go through again, pick spot with the least number already at it
				inPtr->staggerPositionIndex = 0;
			}
		}
		else done = true;
	}	
	unless (tooBig)
	{
		++((*gTopLeftCorners)[inPtr->staggerPositionIndex]); // add the window to the number at this spot
	}
}// calculateWindowPosition


/*!
Notifies all listeners for the specified Terminal
Window state change, passing the given context to
the listener.

IMPORTANT:	The context must make sense for the
			type of change; see "TerminalWindow.h"
			for the type of context associated with
			each terminal window change.

(3.0)
*/
void
changeNotifyForTerminalWindow	(My_TerminalWindowPtr	inPtr,
								 TerminalWindow_Change	inWhatChanged,
								 void*					inContextPtr)
{
	// invoke listener callback routines appropriately, from the specified terminal window’s listener model
	ListenerModel_NotifyListenersOfEvent(inPtr->changeListenerModel, inWhatChanged, inContextPtr);
}// changeNotifyForTerminalWindow


/*!
Registers the “bell off” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
IconRef
createBellOffIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnBellOffIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemBellOff,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createBellOffIcon


/*!
Registers the “bell on” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
IconRef
createBellOnIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnBellOnIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemBellOn,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createBellOnIcon


/*!
Registers the “customize toolbar” icon reference with the system,
and returns a reference to the new icon.

NOTE:	This is only being created for short-term Carbon use; it
		will not be necessary to allocate icons at all in Cocoa
		windows.

(4.0)
*/
IconRef
createCustomizeToolbarIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnCustomizeToolbarIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemCustomize,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createCustomizeToolbarIcon


/*!
Registers the “full screen” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
IconRef
createFullScreenIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnFullScreenIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemFullScreen,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createFullScreenIcon


/*!
Registers the “hide window” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
IconRef
createHideWindowIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnHideWindowIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemHideWindow,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createHideWindowIcon


/*!
Registers the “LED off” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
IconRef
createLEDOffIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnLEDOffIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemLEDOff,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createLEDOffIcon


/*!
Registers the “LED on” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
IconRef
createLEDOnIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnLEDOnIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemLEDOn,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createLEDOnIcon


/*!
Registers the “print” icon reference with the system, and returns
a reference to the new icon.

NOTE:	This is only being created for short-term Carbon use; it
		will not be necessary to allocate icons at all in Cocoa
		windows.

(4.0)
*/
IconRef
createPrintIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnPrintIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemPrint,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createPrintIcon


/*!
Registers the “scroll lock off” icon reference with the
system, and returns a reference to the new icon.

(3.1)
*/
IconRef
createScrollLockOffIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnScrollLockOffIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemScrollLockOff,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createScrollLockOffIcon


/*!
Registers the “scroll lock on” icon reference with the
system, and returns a reference to the new icon.

(3.1)
*/
IconRef
createScrollLockOnIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnScrollLockOnIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconToolbarItemScrollLockOn,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createScrollLockOnIcon


/*!
Creates a floating window that looks like a tab, used to
“attach” to an existing terminal window in tab view.

Also installs a resize handler to ensure the drawer is
no bigger than its default size (otherwise, the Toolbox
will make it as wide as the window).

(3.1)
*/
Boolean
createTabWindow		(My_TerminalWindowPtr	inPtr)
{
	HIWindowRef		tabWindow = nullptr;
	Boolean			result = false;
	
	
	// load the NIB containing this floater (automatically finds the right localization)
	tabWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
							CFSTR("TerminalWindow"), CFSTR("Tab")) << NIBLoader_AssertWindowExists;
	if (nullptr != tabWindow)
	{
		OSStatus	error = noErr;
		Rect		currentBounds;
		
		
		// install a callback that responds as the drawer window is resized; this is used
		// primarily to enforce a maximum drawer width, not to allow a resizable drawer;
		// these are also only initial values, they are updated later if anything resizes
		error = GetWindowBounds(tabWindow, kWindowContentRgn, &currentBounds);
		assert_noerr(error);
		inPtr->tabDrawerWindowResizeHandler.install(tabWindow, handleNewDrawerWindowSize, inPtr->selfRef/* user data */,
													currentBounds.right - currentBounds.left/* minimum width */,
													currentBounds.bottom - currentBounds.top/* minimum height */,
													currentBounds.right - currentBounds.left/* maximum width */,
													currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(inPtr->tabDrawerWindowResizeHandler.isInstalled());
		
		// if the global default width has not yet been initialized, set it;
		// initialize this window’s tab size field to the global default
		if (0.0 == gDefaultTabWidth)
		{
			error = GetWindowBounds(tabWindow, kWindowStructureRgn, &currentBounds);
			assert_noerr(error);
			gDefaultTabWidth = STATIC_CAST(currentBounds.right - currentBounds.left, Float32);
			gDefaultTabHeight = STATIC_CAST(currentBounds.bottom - currentBounds.top, Float32);
		}
		{
			OptionBits				preferredEdge = kWindowEdgeTop;
			Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagWindowTabPreferredEdge,
																		sizeof(preferredEdge), &preferredEdge);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				preferredEdge = kWindowEdgeTop;
			}
			if ((kWindowEdgeLeft == preferredEdge) || (kWindowEdgeRight == preferredEdge))
			{
				inPtr->tabSizeInPixels = gDefaultTabHeight;
			}
			else
			{
				inPtr->tabSizeInPixels = gDefaultTabWidth;
			}
		}
		
		// enable drag tracking so that tabs can auto-activate during drags
		error = SetAutomaticControlDragTrackingEnabledForWindow(tabWindow, true/* enabled */);
		assert_noerr(error);
		
		// install a drag handler so that tabs switch automatically as
		// items hover over them
		{
			HIViewRef	contentPane = nullptr;
			
			
			error = HIViewFindByID(HIViewGetRoot(tabWindow), kHIViewWindowContentID, &contentPane);
			assert_noerr(error);
			inPtr->tabDragHandlerPtr = new CarbonEventHandlerWrap(CarbonEventUtilities_ReturnViewTarget(contentPane),
																	receiveTabDragDrop,
																	CarbonEventSetInClass
																	(CarbonEventClass(kEventClassControl),
																		kEventControlDragEnter),
																	inPtr->selfRef/* handler data */);
			assert(nullptr != inPtr->tabDragHandlerPtr);
			assert(inPtr->tabDragHandlerPtr->isInstalled());
			error = SetControlDragTrackingEnabled(contentPane, true/* is drag enabled */);
			assert_noerr(error);
		}
		
		// install a contextual menu handler
		{
			HIViewRef	contentPane = nullptr;
			
			
			error = HIViewFindByID(HIViewGetRoot(tabWindow), kHIViewWindowContentID, &contentPane);
			assert_noerr(error);
			inPtr->tabContextualMenuHandlerPtr = new CarbonEventHandlerWrap(CarbonEventUtilities_ReturnViewTarget(contentPane),
																			receiveTabContextualMenuClick,
																			CarbonEventSetInClass
																			(CarbonEventClass(kEventClassControl),
																				kEventControlContextualMenuClick),
																			inPtr->selfRef/* handler data */);
			assert(nullptr != inPtr->tabContextualMenuHandlerPtr);
			assert(inPtr->tabContextualMenuHandlerPtr->isInstalled());
			assert_noerr(error);
		}
	}
	
	inPtr->tab = tabWindow;
	result = (nullptr != tabWindow);
	
	return result;
}// createTabWindow


/*!
Creates the content controls (except the terminal screen
itself) in the specified terminal window, for which a
Mac OS window must already exist.  The controls include
the scroll bars and the toolbar.

(3.0)
*/
void
createViews		(My_TerminalWindowPtr	inPtr)
{
	HIViewWrap	contentView(kHIViewWindowContentID, returnCarbonWindow(inPtr));
	Rect		rect;
	OSStatus	error = noErr;
	
	
	assert(contentView.exists());
	
	// create routine to handle scroll activity
	inPtr->scrollProcUPP = NewControlActionUPP(scrollProc); // this is disposed via TerminalWindow_Dispose()
	
	// create a vertical scroll bar; the resize event handler initializes its size correctly
	SetRect(&rect, 0, 0, 0, 0);
	error = CreateScrollBarControl(returnCarbonWindow(inPtr), &rect, 0/* value */, 0/* minimum */, 0/* maximum */, 0/* view size */,
									true/* live tracking */, inPtr->scrollProcUPP, &inPtr->controls.scrollBarV);
	assert_noerr(error);
	error = SetControlProperty(inPtr->controls.scrollBarV, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeTerminalWindowRef,
								sizeof(inPtr->selfRef), &inPtr->selfRef); // used in scrollProc
	assert_noerr(error);
	error = HIViewAddSubview(contentView, inPtr->controls.scrollBarV);
	assert_noerr(error);
	
	// create a horizontal scroll bar; the resize event handler initializes its size correctly
	SetRect(&rect, 0, 0, 0, 0);
	error = CreateScrollBarControl(returnCarbonWindow(inPtr), &rect, 0/* value */, 0/* minimum */, 0/* maximum */, 0/* view size */,
									true/* live tracking */, inPtr->scrollProcUPP, &inPtr->controls.scrollBarH);
	assert_noerr(error);
	error = SetControlProperty(inPtr->controls.scrollBarH, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeTerminalWindowRef,
								sizeof(inPtr->selfRef), &inPtr->selfRef); // used in scrollProc
	assert_noerr(error);
	error = HIViewAddSubview(contentView, inPtr->controls.scrollBarH);
	assert_noerr(error);
	// horizontal scrolling is not supported for now...
	(OSStatus)HIViewSetVisible(inPtr->controls.scrollBarH, false);
}// createViews


/*!
Creates a Cocoa window for the specified terminal window,
based on a Carbon window (for now), and constructs a root
view for subsequent embedding.

Returns nullptr if the window was not created successfully.

(4.0)
*/
NSWindow*
createWindow ()
{
	AutoPool		_;
	NSWindow*		result = nil;
	HIWindowRef		window = nullptr;
	
	
	// load the NIB containing this window (automatically finds the right localization)
	window = NIBWindow(AppResources_ReturnBundleForNIBs(),
						CFSTR("TerminalWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	if (nullptr != window)
	{
		result = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(window);
		
		// override this default; technically terminal windows
		// are immediately closeable for the first 15 seconds
		(OSStatus)SetWindowModified(window, false);
	}
	return result;
}// createWindow


/*!
Allocates the "gTopLeftCorners" array if it does
not already exist.

(3.0)
*/
void
ensureTopLeftCornersExists ()
{
	if (gTopLeftCorners == nullptr)
	{
		gTopLeftCorners = REINTERPRET_CAST(Memory_NewHandleInProperZone(kMy_MaximumNumberOfArrangedWindows * sizeof(SInt16),
																		kMemoryBlockLifetimeLong), SInt16**);
	}
}// ensureTopLeftCornersExists


/*!
Returns the screen buffer used by the view most recently
focused by the user.  Therefore, even if no view is
currently focused, some valid screen buffer will be
returned as long as SOME screen is used by the window
(which should always be true!).

IMPORTANT:	This API is under evaluation.  Perhaps there
			will be value in allowing more than one screen
			buffer per view, in which case returning just
			one would be too limiting.

(3.0)
*/
TerminalScreenRef
getActiveScreen		(My_TerminalWindowPtr	inPtr)
{
	assert(!inPtr->allScreens.empty());
	return inPtr->allScreens.front(); // TEMPORARY; should instead use focus-change events from terminal views
}// getActiveScreen


/*!
Returns the view most recently focused by the user.
Therefore, even if no view is currently focused, some
valid view will be returned as long as SOME view exists
in the window (which should always be true!).

(3.0)
*/
TerminalViewRef
getActiveView	(My_TerminalWindowPtr	inPtr)
{
	assert(!inPtr->allViews.empty());
	return inPtr->allViews.front(); // TEMPORARY; should instead use focus-change events from terminal views
}// getActiveView


/*!
Returns a constant describing the type of scroll bar that is
given.  If the specified control does not belong to the given
terminal window, "kMy_InvalidScrollBarKind" is returned;
otherwise, a constant is returned indicating whether the
control is horizontal or vertical.

(3.0)
*/
My_ScrollBarKind
getScrollBarKind	(My_TerminalWindowPtr	inPtr,
					 HIViewRef				inScrollBarControl)
{
	My_ScrollBarKind	result = kMy_InvalidScrollBarKind;
	
	
	if (inScrollBarControl == inPtr->controls.scrollBarH) result = kMy_ScrollBarKindHorizontal;
	if (inScrollBarControl == inPtr->controls.scrollBarV) result = kMy_ScrollBarKindVertical;
	return result;
}// getScrollBarKind


/*!
Returns the screen buffer that the given scroll bar
controls, or nullptr if none.

(3.0)
*/
TerminalScreenRef
getScrollBarScreen	(My_TerminalWindowPtr	inPtr,
					 HIViewRef				UNUSED_ARGUMENT(inScrollBarControl))
{
	assert(!inPtr->allScreens.empty());
	return inPtr->allScreens.front(); // one day, if more than one view per window exists, this logic will be more complex
}// getScrollBarScreen


/*!
Returns the view that the given scroll bar controls,
or nullptr if none.

(3.0)
*/
TerminalViewRef
getScrollBarView	(My_TerminalWindowPtr	inPtr,
					 HIViewRef				UNUSED_ARGUMENT(inScrollBarControl))
{
	assert(!inPtr->allViews.empty());
	return inPtr->allViews.front(); // one day, if more than one view per window exists, this logic will be more complex
}// getScrollBarView


/*!
Returns the width and height of the screen interior
(i.e. not including insets) of a terminal window
whose content region has the specified dimensions.
The resultant dimensions subtract out the size of any
window header (unless it’s collapsed), the scroll
bars, and the terminal screen insets (padding).

See also getWindowSizeFromViewSize(), which does the
reverse.

IMPORTANT:	Any changes to this routine should be
			reflected inversely in the code for
			getWindowSizeFromViewSize().

(3.0)
*/
void
getViewSizeFromWindowSize	(My_TerminalWindowPtr	inPtr,
							 SInt16					inWindowContentWidthInPixels,
							 SInt16					inWindowContentHeightInPixels,
							 SInt16*				outScreenInteriorWidthInPixels,
							 SInt16*				outScreenInteriorHeightInPixels)
{
	if (nullptr != outScreenInteriorWidthInPixels)
	{
		*outScreenInteriorWidthInPixels = inWindowContentWidthInPixels - returnScrollBarWidth(inPtr);
	}
	if (nullptr != outScreenInteriorHeightInPixels)
	{
		*outScreenInteriorHeightInPixels = inWindowContentHeightInPixels - returnStatusBarHeight(inPtr) -
											returnToolbarHeight(inPtr) - returnScrollBarHeight(inPtr);
	}
}// getViewSizeFromWindowSize


/*!
Returns the width and height of the content region
of a terminal window whose screen interior has the
specified dimensions.

See also getViewSizeFromWindowSize(), which does
the reverse.

IMPORTANT:	Any changes to this routine should be
			reflected inversely in the code for
			getViewSizeFromWindowSize().

(3.0)
*/
void
getWindowSizeFromViewSize	(My_TerminalWindowPtr	inPtr,
							 SInt16					inScreenInteriorWidthInPixels,
							 SInt16					inScreenInteriorHeightInPixels,
							 SInt16*				outWindowContentWidthInPixels,
							 SInt16*				outWindowContentHeightInPixels)
{
	if (nullptr != outWindowContentWidthInPixels)
	{
		*outWindowContentWidthInPixels = inScreenInteriorWidthInPixels + returnScrollBarWidth(inPtr);
	}
	if (nullptr != outWindowContentHeightInPixels)
	{
		*outWindowContentHeightInPixels = inScreenInteriorHeightInPixels + returnStatusBarHeight(inPtr) +
											returnToolbarHeight(inPtr) + returnScrollBarHeight(inPtr);
	}
}// getWindowSizeFromViewSize


/*!
Responds to a close of the Find dialog sheet in a terminal
window.  Currently, this just retains the keyword string
so that Find Again can be used, and remembers the user’s
most recent checkbox settings.

(3.0)
*/
void
handleFindDialogClose	(FindDialog_Ref		inDialogThatClosed)
{
	TerminalWindowRef		terminalWindow = FindDialog_ReturnTerminalWindow(inDialogThatClosed);
	
	
	if (terminalWindow != nullptr)
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
		
		
		// save things the user entered in the dialog
		// (history array is implicitly saved because
		// the mutable array given at construction is
		// retained by reference)
		ptr->recentSearchOptions = FindDialog_ReturnOptions(inDialogThatClosed);
		
		FindDialog_Dispose(&inDialogThatClosed);
	}
}// handleFindDialogClose


/*!
This routine is called whenever the tab is resized, and it
should resize and relocate views as appropriate.

Even if this implementation does nothing, it must exist so the
drawer height (or width, for vertical tabs) is constrained.

(3.1)
*/
void
handleNewDrawerWindowSize	(WindowRef		inWindowRef,
							 Float32		inDeltaX,
							 Float32		UNUSED_ARGUMENT(inDeltaY),
							 void*			UNUSED_ARGUMENT(inContext))
{
	HIViewWrap		viewWrap;
	
	
	// resize the title view, and move the pop-out button
	if (Localization_IsLeftToRight())
	{
		viewWrap = HIViewWrap(idMyLabelTabTitle, inWindowRef);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0);
	}
	else
	{
		viewWrap = HIViewWrap(idMyLabelTabTitle, inWindowRef);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0);
	}
}// handleNewDrawerWindowSize


/*!
This method moves and resizes the contents of a terminal
window in response to a resize.

(3.0)
*/
void
handleNewSize	(WindowRef	inWindow,
				 Float32	UNUSED_ARGUMENT(inDeltaX),
				 Float32	UNUSED_ARGUMENT(inDeltaY),
				 void*		inTerminalWindowRef)
{
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	HIRect				contentBounds;
	OSStatus			error = noErr;
	
	
	// get window boundaries in local coordinates
	error = HIViewGetBounds(HIViewWrap(kHIViewWindowContentID, inWindow), &contentBounds);
	assert_noerr(error);
	
	if (terminalWindow != nullptr)
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
		HIRect							viewBounds;
		
		
		// glue the vertical scroll bar to the new right side of the window and to the
		// bottom edge of the status bar, and ensure it is glued to the size box in the
		// corner (so vertically resize it)
		viewBounds.origin.x = contentBounds.size.width - returnScrollBarWidth(ptr);
		viewBounds.origin.y = -1; // frame thickness
		viewBounds.size.width = returnScrollBarWidth(ptr);
		viewBounds.size.height = contentBounds.size.height - returnGrowBoxHeight(ptr);
		error = HIViewSetFrame(ptr->controls.scrollBarV, &viewBounds);
		assert_noerr(error);
		
		// glue the horizontal scroll bar to the new bottom edge of the window; it must
		// also move because its left edge is glued to the window edge, and it must resize
		// because its right edge is glued to the size box in the corner
		viewBounds.origin.x = -1; // frame thickness
		viewBounds.origin.y = contentBounds.size.height - returnScrollBarHeight(ptr);
		viewBounds.size.width = contentBounds.size.width - returnGrowBoxWidth(ptr);
		viewBounds.size.height = returnScrollBarHeight(ptr);
		error = HIViewSetFrame(ptr->controls.scrollBarH, &viewBounds);
		//assert_noerr(error); // ignore this error since the scroll bar is not used
		
		// change the screen sizes to match the user’s window size as well as possible,
		// notifying listeners of the change (to trigger actions such as sending messages
		// to the Unix process in the window, etc.); the number of columns in each screen
		// will be changed to closely match the overall width, but only the last screen’s
		// line count will be changed; in the event that there are tabs, only views
		// belonging to the same group will be scaled together during the resizing
		{
			TerminalWindow_ViewGroup const	kViewGroupArray[] =
											{
												kTerminalWindow_ViewGroupEverything
											};
			UInt16 const					kNumberOfViews = TerminalWindow_ReturnViewCount(terminalWindow);
			TerminalViewRef*				viewArray = new TerminalViewRef[kNumberOfViews];
			register SInt16					groupIndex = 0;
			
			
			for (groupIndex = 0; groupIndex < STATIC_CAST(sizeof(kViewGroupArray) / sizeof(TerminalWindow_ViewGroup), SInt16); ++groupIndex)
			{
				register SInt16		i = 0;
				UInt16				actualNumberOfViews = 0;
				
				
				// find all the views belonging to this tab and apply the resize
				// algorithm only while considering views belonging to the same tab
				TerminalWindow_GetViewsInGroup(terminalWindow, kViewGroupArray[groupIndex], kNumberOfViews, viewArray,
												&actualNumberOfViews);
				if (actualNumberOfViews > 0)
				{
					HIRect		terminalScreenBounds;
					
					
					for (i = 0; i < actualNumberOfViews; ++i)
					{
						// figure out how big the screen is becoming;
						// TEMPORARY: sets each view size to the whole area, since there is only one right now
						//getViewSizeFromWindowSize(ptr, contentBounds.right - contentBounds.left,
						//							contentBounds.bottom - contentBounds.top,
						//							&terminalScreenWidth, &terminalScreenHeight);
						
						// make the view stick to the scroll bars, effectively adding perhaps a few pixels of padding
						{
							HIRect		scrollBarBounds;
							
							
							error = HIViewGetFrame(TerminalView_ReturnContainerHIView(viewArray[i]), &terminalScreenBounds);
							assert_noerr(error);
							error = HIViewGetFrame(ptr->controls.scrollBarV, &scrollBarBounds);
							assert_noerr(error);
							terminalScreenBounds.origin.x = -1/* frame thickness */;
							terminalScreenBounds.origin.y = -1/* frame thickness */;
							terminalScreenBounds.size.width = scrollBarBounds.origin.x - terminalScreenBounds.origin.x;
							error = HIViewGetFrame(ptr->controls.scrollBarH, &scrollBarBounds);
							assert_noerr(error);
							terminalScreenBounds.size.height = scrollBarBounds.origin.y - terminalScreenBounds.origin.y;
						}
						error = HIViewSetFrame(TerminalView_ReturnContainerHIView(viewArray[i]), &terminalScreenBounds);
						assert_noerr(error);
					}
				}
			}
			delete [] viewArray, viewArray = nullptr;
		}
		
		// when the window size changes, the screen dimensions are likely to change
		// TEMPORARY: analyze this further, see if this behavior is really desirable
		changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeScreenDimensions, terminalWindow/* context */);
		
		// update the scroll bars’ values to reflect the new screen size
		updateScrollBars(ptr);
	}
}// handleNewSize


/*!
Installs a handler to draw tick marks on top of the
standard scroll bar (for showing the location of search
results).

This handler is not always installed because it is only
needed while there are search results defined, and there
is a cost to allowing scroll bar draws to enter the
application’s memory space.

To remove, call inPtr->scrollTickHandler.remove().

(4.0)
*/
void
installTickHandler	(My_TerminalWindowPtr	inPtr)
{
	inPtr->scrollTickHandler.remove();
	assert(false == inPtr->scrollTickHandler.isInstalled());
	inPtr->scrollTickHandler.install(GetControlEventTarget(inPtr->controls.scrollBarV), receiveScrollBarDraw,
										CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlDraw),
										inPtr->selfRef/* user data */);
	assert(inPtr->scrollTickHandler.isInstalled());
}// installTickHandler


/*!
Installs an Undo procedure that will revert
the font and/or font size of the specified
screen to its current font and/or font size
when the user chooses Undo.

(3.0)
*/
void
installUndoFontSizeChanges	(TerminalWindowRef	inTerminalWindow,
							 Boolean			inUndoFont,
							 Boolean			inUndoFontSize)
{
	OSStatus					error = noErr;
	UndoDataFontSizeChangesPtr	dataPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(UndoDataFontSizeChanges)),
															UndoDataFontSizeChangesPtr); // disposed in the action method
	
	
	if (dataPtr == nullptr) error = memFullErr;
	else
	{
		// initialize context structure
		dataPtr->terminalWindow = inTerminalWindow;
		dataPtr->undoFontSize = inUndoFontSize;
		dataPtr->undoFont = inUndoFont;
		TerminalWindow_GetFontAndSize(inTerminalWindow, dataPtr->fontName, &dataPtr->fontSize);
	}
	{
		CFStringRef		undoNameCFString = nullptr;
		CFStringRef		redoNameCFString = nullptr;
		
		
		assert(UIStrings_Copy(kUIStrings_UndoFormatChanges, undoNameCFString).ok());
		assert(UIStrings_Copy(kUIStrings_RedoFormatChanges, redoNameCFString).ok());
		dataPtr->action = Undoables_NewAction(undoNameCFString, redoNameCFString, reverseFontChanges,
												kUndoableContextIdentifierTerminalFontSizeChanges, dataPtr);
		if (nullptr != undoNameCFString) CFRelease(undoNameCFString), undoNameCFString = nullptr;
		if (nullptr != redoNameCFString) CFRelease(redoNameCFString), redoNameCFString = nullptr;
	}
	Undoables_AddAction(dataPtr->action);
	if (error != noErr) Console_WriteValue("Warning: Could not make font and/or size change undoable, error", error);
	else
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
		
		
		ptr->installedActions.push_back(dataPtr->action);
	}
}// installUndoFontSizeChanges


/*!
Installs an Undo procedure that will revert
the font size and window location of the
specified screen to its current font size
and window location when the user chooses Undo.

(3.1)
*/
void
installUndoFullScreenChanges	(TerminalWindowRef			inTerminalWindow,
								 TerminalView_DisplayMode	inPreFullScreenMode,
								 TerminalView_DisplayMode	inFullScreenMode)
{
	OSStatus						error = noErr;
	UndoDataFullScreenChangesPtr	dataPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(UndoDataFullScreenChanges)),
																UndoDataFullScreenChangesPtr); // disposed in the action method
	CFStringRef						undoCFString = nullptr;
	UIStrings_Result				stringResult = UIStrings_Copy(kUIStrings_UndoFullScreen, undoCFString);
	
	
	if (nullptr == dataPtr) error = memFullErr;
	else
	{
		// initialize context structure
		Rect	windowBounds;
		
		
		dataPtr->terminalWindow = inTerminalWindow;
		dataPtr->oldMode = inPreFullScreenMode;
		dataPtr->newMode = inFullScreenMode;
		TerminalWindow_GetFontAndSize(inTerminalWindow, nullptr/* font name */, &dataPtr->oldFontSize);
		GetWindowBounds(TerminalWindow_ReturnWindow(inTerminalWindow), kWindowContentRgn, &windowBounds);
		dataPtr->oldContentBounds = windowBounds;
	}
	dataPtr->action = Undoables_NewAction(undoCFString/* undo name */, nullptr/* redo name; this is not redoable */,
											reverseFullScreenChanges, kUndoableContextIdentifierTerminalFullScreenChanges,
											dataPtr);
	Undoables_AddAction(dataPtr->action);
	if (noErr != error) Console_Warning(Console_WriteValue, "could not make font size and/or location change undoable, error", error);
	else
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
		
		
		ptr->installedActions.push_back(dataPtr->action);
	}
	
	if (nullptr != undoCFString)
	{
		CFRelease(undoCFString), undoCFString = nullptr;
	}
}// installUndoFullScreenChanges


/*!
Installs an Undo procedure that will revert the
dimensions of the of the specified screen to its
current dimensions when the user chooses Undo.

(3.1)
*/
void
installUndoScreenDimensionChanges	(TerminalWindowRef		inTerminalWindow)
{
	OSStatus							error = noErr;
	UndoDataScreenDimensionChangesPtr	dataPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(UndoDataFontSizeChanges)),
																	UndoDataScreenDimensionChangesPtr); // disposed in the action method
	
	
	if (dataPtr == nullptr) error = memFullErr;
	else
	{
		// initialize context structure
		dataPtr->terminalWindow = inTerminalWindow;
		TerminalWindow_GetScreenDimensions(inTerminalWindow, &dataPtr->columns, &dataPtr->rows);
	}
	{
		CFStringRef		undoNameCFString = nullptr;
		CFStringRef		redoNameCFString = nullptr;
		
		
		assert(UIStrings_Copy(kUIStrings_UndoDimensionChanges, undoNameCFString).ok());
		assert(UIStrings_Copy(kUIStrings_RedoDimensionChanges, redoNameCFString).ok());
		dataPtr->action = Undoables_NewAction(undoNameCFString, redoNameCFString, reverseScreenDimensionChanges,
												kUndoableContextIdentifierTerminalDimensionChanges, dataPtr);
		if (nullptr != undoNameCFString) CFRelease(undoNameCFString), undoNameCFString = nullptr;
		if (nullptr != redoNameCFString) CFRelease(redoNameCFString), redoNameCFString = nullptr;
	}
	Undoables_AddAction(dataPtr->action);
	if (error != noErr) Console_WriteValue("Warning: Could not make dimension change undoable, error", error);
	else
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
		
		
		ptr->installedActions.push_back(dataPtr->action);
	}
}// installUndoScreenDimensionChanges


/*!
Handles "kEventCommandProcess" of "kEventClassCommand" for
terminal window commands.

(3.0)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inTerminalWindowRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// Execute a command selected from a menu (or sent from a control, etc.).
				//
				// IMPORTANT: This could imply ANY form of menu selection, whether from
				//            the menu bar, from a contextual menu, or from a pop-up menu!
				switch (received.commandID)
				{
				case kCommandFind:
					// enter search mode
					TerminalWindow_DisplayTextSearchDialog(terminalWindow);
					result = noErr;
					break;
				
				case kCommandFindAgain:
				case kCommandFindPrevious:
					// rotate to next or previous match; since ALL matches are highlighted at
					// once, this is simply a focusing mechanism and does not conduct a search
					{
						TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
						
						
						TerminalView_RotateSearchResultHighlight(view, (kCommandFindPrevious == received.commandID) ? -1 : +1);
						TerminalView_ZoomToSearchResults(view);
					}
					result = noErr;
					break;
				
				case kCommandFindCursor:
					// draw attention to terminal cursor location
					TerminalView_ZoomToCursor(TerminalWindow_ReturnViewWithFocus(terminalWindow));
					result = noErr;
					break;
				
				case kCommandBiggerText:
				case kCommandSmallerText:
					{
						UInt16		fontSize = 0;
						
						
						// determine the new font size
						TerminalWindow_GetFontAndSize(terminalWindow, nullptr/* font */, &fontSize);
						if (kCommandBiggerText == received.commandID) ++fontSize;
						else if (kCommandSmallerText == received.commandID)
						{
							if (fontSize > 4/* arbitrary */) --fontSize;
						}
						
						// set the window size to fit the new font size optimally
						installUndoFontSizeChanges(terminalWindow, false/* undo font */, true/* undo font size */);
						TerminalWindow_SetFontAndSize(terminalWindow, nullptr/* font */, fontSize);
						
						result = noErr;
					}
					break;
				
				case kCommandFullScreen:
				case kCommandFullScreenModal:
				case kCommandKioskModeDisable:
					{
						static UInt16		gNumberOfDisplaysUsed = 0;
						Boolean				turnOnFullScreen = (kCommandKioskModeDisable != received.commandID) &&
																(false == FlagManager_Test(kFlagKioskMode));
						Boolean				modalFullScreen = (kCommandFullScreenModal == received.commandID);
						
						
						// enable kiosk mode only if it is not enabled already
						if (turnOnFullScreen)
						{
							HIWindowRef			activeWindow = EventLoop_ReturnRealFrontWindow();
							HIWindowRef			alternateDisplayWindow = nullptr;
							Boolean				allowForceQuit = true;
							Boolean				showMenuBar = true;
							Boolean				showOffSwitch = true;
							Boolean				showScrollBar = true;
							Boolean				showWindowFrame = true;
							Boolean				useEffects = true;
							SystemUIOptions		optionsForFullScreen = 0;
							
							
							// read relevant user preferences
							{
								size_t		actualSize = 0;
								
								
								if (kPreferences_ResultOK !=
									Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
														&allowForceQuit, &actualSize))
								{
									allowForceQuit = true; // assume a value if the preference cannot be found
								}
								unless (allowForceQuit) optionsForFullScreen |= kUIOptionDisableForceQuit;
								
								if (kPreferences_ResultOK !=
									Preferences_GetData(kPreferences_TagKioskShowsMenuBar, sizeof(showMenuBar),
														&showMenuBar, &actualSize))
								{
									showMenuBar = false; // assume a value if the preference cannot be found
								}
								if (showMenuBar) optionsForFullScreen |= kUIOptionAutoShowMenuBar;
								
								if (kPreferences_ResultOK !=
									Preferences_GetData(kPreferences_TagKioskShowsOffSwitch, sizeof(showOffSwitch),
														&showOffSwitch, &actualSize))
								{
									showOffSwitch = true; // assume a value if the preference cannot be found
								}
								
								if (kPreferences_ResultOK !=
									Preferences_GetData(kPreferences_TagKioskShowsScrollBar, sizeof(showScrollBar),
														&showScrollBar, &actualSize))
								{
									showScrollBar = true; // assume a value if the preference cannot be found
								}
								
								if (kPreferences_ResultOK !=
									Preferences_GetData(kPreferences_TagKioskShowsWindowFrame, sizeof(showWindowFrame),
														&showWindowFrame, &actualSize))
								{
									showWindowFrame = true; // assume a value if the preference cannot be found
								}
								
								if (kPreferences_ResultOK !=
									Preferences_GetData(kPreferences_TagKioskUsesSuperfluousEffects, sizeof(useEffects),
														&useEffects, &actualSize))
								{
									useEffects = true; // assume a value if the preference cannot be found
								}
							}
							
							if (modalFullScreen)
							{
								CGDirectDisplayID	activeWindowDisplay = kCGNullDirectDisplay;
								
								
								alternateDisplayWindow = activeWindow; // initially...
								if (RegionUtilities_GetWindowDirectDisplayID(activeWindow, activeWindowDisplay))
								{
									TerminalWindowRef	alternateTerminalWindow = nullptr;
									CGDirectDisplayID	alternateWindowDisplay = kCGNullDirectDisplay;
									Boolean				validAlternative = false;
									
									
									// in full screen mode with multiple displays, try to detect a
									// good window to maximize on each display
									do
									{
										validAlternative = false; // initially...
										
										// the most recently activated window seems like a good place to start
										alternateDisplayWindow = GetNextWindow(alternateDisplayWindow);
										alternateTerminalWindow = TerminalWindow_ReturnFromWindow(alternateDisplayWindow);
										if (nullptr != alternateTerminalWindow)
										{
											// find the monitor that contains most of the window, and also make
											// sure that it isn’t “the same” desktop (mirrored) as the one that
											// already contains the active window
											if (RegionUtilities_GetWindowDirectDisplayID(alternateDisplayWindow,
																							alternateWindowDisplay))
											{
												CGDirectDisplayID	mirrorSource = CGDisplayMirrorsDisplay(alternateWindowDisplay);
												
												
												if ((kCGNullDirectDisplay == mirrorSource) &&
													(activeWindowDisplay != alternateWindowDisplay))
												{
													// good; this window is primarily covering a different display,
													// so it is a candidate for zooming to its display
													validAlternative = true;
												}
											}
										}
									} while ((false == validAlternative) &&
												(nullptr != alternateDisplayWindow) &&
												(activeWindow != alternateDisplayWindow));
									
									if (false == validAlternative)
									{
										alternateDisplayWindow = nullptr;
									}
								}
								
								// lock down the system (hide menu bar, etc.) for kiosk;
								// do this early, so that below when the window positioning
								// boundaries are sought, they reflect the proper hidden
								// state of the Dock, etc. if normal content should be hidden
								if (noErr != SetSystemUIMode(kUIModeAllHidden, optionsForFullScreen/* options */))
								{
									// apparently failed...
									Sound_StandardAlert();
								}
								
								// show “off switch” if user has requested it
								if (showOffSwitch)
								{
									Keypads_SetVisible(kKeypads_WindowTypeFullScreen, true);
									SelectWindow(activeWindow); // return focus to the terminal window
									CocoaBasic_MakeFrontWindowCarbonUserFocusWindow();
								}
								
								// finally, set a global flag indicating the mode is in effect
								FlagManager_Set(kFlagKioskMode, true);
							}
							
							// terminal views are mostly capable of handling text zoom or row/column resize
							// on their own, as a side effect of changing dimensions in the right mode; so
							// it is just a matter of setting the window size, and then setting up an
							// appropriate Undo command (which is used to turn off Full Screen later)
							gNumberOfDisplaysUsed = 0;
							for (HIWindowRef targetWindow = activeWindow; targetWindow != nullptr; ++gNumberOfDisplaysUsed)
							{
								TerminalWindowRef				targetTerminalWindow = TerminalWindow_ReturnFromWindow(targetWindow);
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), targetTerminalWindow);
								TerminalView_DisplayMode const	kOldMode = TerminalView_ReturnDisplayMode(ptr->allViews.front());
								Rect							maxBounds;
								
								
								// resize the window and fill its monitor
								if (modalFullScreen)
								{
									// NOTE: while it would be more consistent to require the Control key,
									// this isn't really possible (it would cause problems when trying to
									// Control-click the Full Screen toolbar icon, and it would cause the
									// default Control-command-F key equivalent to always trigger a swap);
									// so a Full Screen preference swap requires Option, even though the
									// equivalent behavior during a window resize requires the Control key
									Boolean const					kSwapModes = EventLoop_IsOptionKeyDown();
									TerminalView_DisplayMode const	kNewMode = (false == kSwapModes)
																				? kOldMode
																				: (kTerminalView_DisplayModeZoom == kOldMode)
																					? kTerminalView_DisplayModeNormal
																					: kTerminalView_DisplayModeZoom;
									
									
									installUndoFullScreenChanges(targetTerminalWindow, kOldMode, kNewMode);
									if (kSwapModes)
									{
										TerminalView_SetDisplayMode(ptr->allViews.front(), kNewMode);
									}
									
									// hide the scroll bar, if requested
									unless (showScrollBar)
									{
										(OSStatus)HIViewSetVisible(ptr->controls.scrollBarV, false);
									}
									
									// always hide the size box
									(OSStatus)ChangeWindowAttributes(returnCarbonWindow(ptr), 0/* attributes to set */,
																		kWindowCollapseBoxAttribute | kWindowFullZoomAttribute |
																			kWindowResizableAttribute/* attributes to clear */);
								}
								else
								{
									installUndoFontSizeChanges(targetTerminalWindow, false/* undo font */, true/* undo font size */);
									TerminalView_SetDisplayMode(ptr->allViews.front(), kTerminalView_DisplayModeZoom);
								}
								
								if ((showWindowFrame) || (kCommandFullScreen == received.commandID))
								{
									RegionUtilities_GetWindowMaximumBounds(targetWindow, &maxBounds,
																			nullptr/* previous bounds */, true/* no insets */);
								}
								else
								{
									// entire screen is available, so use it
									RegionUtilities_GetWindowDeviceGrayRect(targetWindow, &maxBounds);
								}
								
								setViewSizeIndependentFromWindow(ptr, true);
								(OSStatus)SetWindowBounds(targetWindow, kWindowContentRgn, &maxBounds);
								setViewSizeIndependentFromWindow(ptr, false);
								
								TerminalView_SetDisplayMode(ptr->allViews.front(), kOldMode);
								
								// switch to next display; TEMPORARY, only supports up to 2 displays
								if (activeWindow == targetWindow) targetWindow = alternateDisplayWindow;
								else targetWindow = nullptr;
							}
						}
						else
						{
							// end Kiosk Mode
							for (UInt16 i = 0; i < gNumberOfDisplaysUsed; ++i)
							{
								Commands_ExecuteByID(kCommandUndo);
							}
							result = noErr;
						}
						
						result = noErr;
					}
					break;
				
				case kCommandFormatDefault:
					{
						// reformat frontmost window using the Default preferences
						My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
						Preferences_ContextRef			defaultSettings = nullptr;
						
						
						if (kPreferences_ResultOK == Preferences_GetDefaultContext(&defaultSettings, Quills::Prefs::FORMAT))
						{
							setViewFormatPreferences(ptr, defaultSettings);
						}
						
						result = noErr;
					}
					break;
				
				case kCommandFormatByFavoriteName:
					// IMPORTANT: This implementation is for Carbon compatibility only, as the
					// Session Preferences panel is still Carbon-based and has a menu that
					// relies on this command handler.  The equivalent menu bar command does
					// not use this, it has an associated Objective-C action method.
					{
						// reformat frontmost window using the specified preferences
						if (received.attributes & kHICommandFromMenu)
						{
							My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
							CFStringRef						collectionName = nullptr;
							
							
							if ((noErr == CopyMenuItemTextAsCFString(received.menu.menuRef, received.menu.menuItemIndex, &collectionName)) &&
								Preferences_IsContextNameInUse(Quills::Prefs::FORMAT, collectionName))
							{
								Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
																			(Quills::Prefs::FORMAT, collectionName),
																			true/* is retained */);
								
								
								if (namedSettings.exists())
								{
									setViewFormatPreferences(ptr, namedSettings.returnRef());
								}
								CFRelease(collectionName), collectionName = nullptr;
							}
						}
						
						result = noErr;
					}
					break;
				
				case kCommandFormat:
					{
						// display a format customization dialog
						My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
						Preferences_ContextRef			temporaryContext = sheetContextBegin(ptr, Quills::Prefs::FORMAT,
																								kMy_SheetTypeFormat);
						
						
						if (nullptr == temporaryContext)
						{
							Sound_StandardAlert();
							Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
						}
						else
						{
							PrefsContextDialog_Ref		dialog = nullptr;
							Panel_Ref					prefsPanel = PrefPanelFormats_New();
							
							
							// display the sheet
							dialog = PrefsContextDialog_New(GetUserFocusWindow(), prefsPanel, temporaryContext,
															kPrefsContextDialog_DisplayOptionsDefault, sheetClosed);
							PrefsContextDialog_Display(dialog); // automatically disposed when the user clicks a button
						}
						
						result = noErr;
					}
					break;
				
				case kCommandHideFrontWindow:
					// hide the frontmost terminal window from view
					TerminalWindow_SetObscured(terminalWindow, true);
					result = noErr;
					break;
				
				case kCommandHideOtherWindows:
					// hide all except the frontmost terminal window from view
					if (TerminalWindow_ReturnWindow(terminalWindow) != GetUserFocusWindow())
					{
						TerminalWindow_SetObscured(terminalWindow, true);
						
						// since every terminal window installs a handler for this command,
						// pretending this event was not handled allows the next window to
						// be notified; in effect, returning this value causes all windows
						// to be hidden automatically as a side effect of notifying listeners!
						result = eventNotHandledErr;
					}
					break;
				
				case kCommandWiderScreen:
				case kCommandNarrowerScreen:
				case kCommandTallerScreen:
				case kCommandShorterScreen:
					{
						TerminalScreenRef	activeScreen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
						UInt16				columns = Terminal_ReturnColumnCount(activeScreen);
						UInt16				rows = Terminal_ReturnRowCount(activeScreen);
						
						
						if (received.commandID == kCommandNarrowerScreen)
						{
							columns -= 4; // arbitrary delta
						}
						else if (received.commandID == kCommandTallerScreen)
						{
							rows += 2; // arbitrary delta
						}
						else if (received.commandID == kCommandShorterScreen)
						{
							rows -= 2; // arbitrary delta
						}
						else
						{
							columns += 4; // arbitrary delta
						}
						
						// arbitrarily restrict the minimum size
						if (columns < 10) columns = 10;
						if (rows < 10) rows = 10;
						
						// resize the screen and the window
						TerminalWindow_SetScreenDimensions(terminalWindow, columns, rows, true/* recordable */);
						
						result = noErr;
					}
					break;
				
				case kCommandLargeScreen:
				case kCommandSmallScreen:
				case kCommandTallScreen:
					{
						UInt16		columns = 0;
						UInt16		rows = 0;
						
						
						// NOTE: Currently these are arbitrary, primarily because
						// certain terminals like VT100 *must* have these quick
						// switch commands available at specific dimensions of
						// 132 and 80 column widths.  However, this could be
						// expanded in the future to allow user-customized sets
						// of dimensions as well.
						if (received.commandID == kCommandLargeScreen)
						{
							columns = 132;
							rows = 24;
						}
						else if (received.commandID == kCommandTallScreen)
						{
							columns = 80;
							rows = 40;
						}
						else
						{
							columns = 80;
							rows = 24;
						}
						
						// resize the screen and the window
						installUndoScreenDimensionChanges(terminalWindow);
						TerminalWindow_SetScreenDimensions(terminalWindow, columns, rows, true/* recordable */);
						
						result = noErr;
					}
					break;
				
				case kCommandSetScreenSize:
					{
						// display a screen size customization dialog
						My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
						Preferences_ContextRef			temporaryContext = sheetContextBegin(ptr, Quills::Prefs::TERMINAL,
																								kMy_SheetTypeScreenSize);
						
						
						if (nullptr == temporaryContext)
						{
							Sound_StandardAlert();
							Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
						}
						else
						{
							PrefsContextDialog_Ref		dialog = nullptr;
							Panel_Ref					prefsPanel = PrefPanelTerminals_NewScreenPane();
							
							
							// display the sheet
							dialog = PrefsContextDialog_New(GetUserFocusWindow(), prefsPanel, temporaryContext,
															kPrefsContextDialog_DisplayOptionNoAddToPrefsButton, sheetClosed);
							PrefsContextDialog_Display(dialog); // automatically disposed when the user clicks a button
						}
						
						result = noErr;
					}
					break;
				
				case kCommandTranslationTableDefault:
					{
						// change character set of frontmost window according to Default preferences
						My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
						SessionRef						session = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
						
						
						if (nullptr != session)
						{
							Preferences_ContextRef		sessionSettings = Session_ReturnTranslationConfiguration(session);
							
							
							if (nullptr != sessionSettings)
							{
								Preferences_TagSetRef		translationTags = PrefPanelTranslations_NewTagSet();
								
								
								if (nullptr != translationTags)
								{
									Preferences_ContextRef		defaultSettings = nullptr;
									Preferences_Result			prefsResult = Preferences_GetDefaultContext
																				(&defaultSettings, Quills::Prefs::TRANSLATION);
									
									
									if (kPreferences_ResultOK != prefsResult)
									{
										Console_Warning(Console_WriteValue, "failed to locate default translation settings, error", prefsResult);
									}
									else
									{
										prefsResult = Preferences_ContextCopy(defaultSettings, sessionSettings, translationTags);
										if (kPreferences_ResultOK != prefsResult)
										{
											Console_Warning(Console_WriteValue, "failed to apply named translation settings to session, error", prefsResult);
										}
									}
									Preferences_ReleaseTagSet(&translationTags);
								}
							}
						}
						result = noErr;
					}
					break;
				
				case kCommandTranslationTableByFavoriteName:
					// IMPORTANT: This implementation is for Carbon compatibility only, as the
					// Session Preferences panel is still Carbon-based and has a menu that
					// relies on this command handler.  The equivalent menu bar command does
					// not use this, it has an associated Objective-C action method.
					{
						// change character set of frontmost window according to the specified preferences
						if (received.attributes & kHICommandFromMenu)
						{
							My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
							SessionRef						session = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
							CFStringRef						collectionName = nullptr;
							
							
							if ((nullptr != session) &&
								(noErr == CopyMenuItemTextAsCFString(received.menu.menuRef, received.menu.menuItemIndex, &collectionName)) &&
								Preferences_IsContextNameInUse(Quills::Prefs::TRANSLATION, collectionName))
							{
								Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
																			(Quills::Prefs::TRANSLATION, collectionName),
																			true/* is retained */);
								Preferences_ContextRef		sessionSettings = Session_ReturnTranslationConfiguration(session);
								
								
								if (namedSettings.exists() && (nullptr != sessionSettings))
								{
									Preferences_TagSetRef		translationTags = PrefPanelTranslations_NewTagSet();
									
									
									if (nullptr != translationTags)
									{
										// change character set of frontmost window according to the specified preferences
										Preferences_Result		prefsResult = Preferences_ContextCopy
																				(namedSettings.returnRef(), sessionSettings, translationTags);
										
										
										if (kPreferences_ResultOK != prefsResult)
										{
											Console_Warning(Console_WriteLine, "failed to apply named translation settings to session");
										}
										Preferences_ReleaseTagSet(&translationTags);
									}
								}
								CFRelease(collectionName), collectionName = nullptr;
							}
						}
						
						result = noErr;
					}
					break;
				
				case kCommandSetTranslationTable:
					{
						// display a translation customization dialog
						My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
						Preferences_ContextRef			temporaryContext = sheetContextBegin(ptr, Quills::Prefs::TRANSLATION,
																								kMy_SheetTypeTranslation);
						
						
						if (nullptr == temporaryContext)
						{
							Sound_StandardAlert();
							Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
						}
						else
						{
							PrefsContextDialog_Ref		dialog = nullptr;
							Panel_Ref					prefsPanel = PrefPanelTranslations_New();
							
							
							// display the sheet
							dialog = PrefsContextDialog_New(GetUserFocusWindow(), prefsPanel, temporaryContext,
															kPrefsContextDialog_DisplayOptionsDefault, sheetClosed);
							PrefsContextDialog_Display(dialog); // automatically disposed when the user clicks a button
						}
						
						result = noErr;
					}
					break;
				
				case kCommandTerminalNewWorkspace:
					// note that this event often originates from the tab drawer, but
					// due to event hierarchy it will eventually be sent to the handler
					// installed on the parent terminal window (how convenient!)
					SessionFactory_MoveTerminalWindowToNewWorkspace(terminalWindow);
					result = noErr;
					break;
				
				case kCommandToggleTerminalLED1:
					{
						TerminalScreenRef	activeScreen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
						
						
						if (nullptr == activeScreen)
						{
							// error...
							Sound_StandardAlert();
						}
						else
						{
							Terminal_LEDSetState(activeScreen, 1/* LED number */, false == Terminal_LEDIsOn(activeScreen, 1));
						}
						result = noErr;
					}
					break;
				
				case kCommandToggleTerminalLED2:
					{
						TerminalScreenRef	activeScreen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
						
						
						if (nullptr == activeScreen)
						{
							// error...
							Sound_StandardAlert();
						}
						else
						{
							Terminal_LEDSetState(activeScreen, 2/* LED number */, false == Terminal_LEDIsOn(activeScreen, 2));
						}
						result = noErr;
					}
					break;
				
				case kCommandToggleTerminalLED3:
					{
						TerminalScreenRef	activeScreen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
						
						
						if (nullptr == activeScreen)
						{
							// error...
							Sound_StandardAlert();
						}
						else
						{
							Terminal_LEDSetState(activeScreen, 3/* LED number */, false == Terminal_LEDIsOn(activeScreen, 3));
						}
						result = noErr;
					}
					break;
				
				case kCommandToggleTerminalLED4:
					{
						TerminalScreenRef	activeScreen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
						
						
						if (nullptr == activeScreen)
						{
							// error...
							Sound_StandardAlert();
						}
						else
						{
							Terminal_LEDSetState(activeScreen, 4/* LED number */, false == Terminal_LEDIsOn(activeScreen, 4));
						}
						result = noErr;
					}
					break;
				
				default:
					// ???
					break;
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	return result;
}// receiveHICommand


/*!
Handles "kEventMouseWheelMoved" of "kEventClassMouse".

Invoked by Mac OS X whenever a mouse with a scrolling
function is used on the frontmost window.

(3.1)
*/
OSStatus
receiveMouseWheelEvent	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassMouse);
	assert(kEventKind == kEventMouseWheelMoved);
	{
		EventMouseWheelAxis		axis = kEventMouseWheelAxisY;
		
		
		// find out which way the mouse wheel moved
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, axis);
		
		// if the axis information was found, continue
		if (noErr == result)
		{
			SInt32		delta = 0;
			UInt32		modifiers = 0;
			
			
			// determine modifier keys pressed during scroll
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
			result = noErr; // ignore modifier key parameter if absent
			
			// determine how far the mouse wheel was scrolled
			// and in which direction; negative means up/left,
			// positive means down/right
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeLongInteger, delta);
			
			// if all information can be found, proceed with scrolling
			if (noErr == result)
			{
				HIWindowRef		targetWindow = nullptr;
				
				
				if (noErr != CarbonEventUtilities_GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, targetWindow))
				{
					// cannot find information (implies Mac OS X 10.0.x) - fine, assume frontmost window
					targetWindow = EventLoop_ReturnRealFrontWindow();
				}
				
				if (TerminalWindow_ExistsFor(targetWindow))
				{
					TerminalWindowRef		terminalWindow = TerminalWindow_ReturnFromWindow(targetWindow);
					
					
					if (nullptr == terminalWindow) result = eventNotHandledErr;
					else if (modifiers & controlKey)
					{
						// like Firefox, use control-scroll-wheel to affect font size
						if (false == FlagManager_Test(kFlagKioskMode))
						{
							Commands_ExecuteByIDUsingEvent((delta > 0) ? kCommandBiggerText : kCommandSmallerText);
						}
						result = noErr;
					}
					else if (modifiers & optionKey)
					{
						// adjust screen width or height
						if (kEventMouseWheelAxisX == axis)
						{
							// adjust screen width
							if (false == FlagManager_Test(kFlagKioskMode))
							{
								Commands_ExecuteByIDUsingEvent((delta > 0) ? kCommandNarrowerScreen : kCommandWiderScreen);
							}
							result = noErr;
						}
						else
						{
							if (modifiers & cmdKey)
							{
								// adjust screen width
								if (false == FlagManager_Test(kFlagKioskMode))
								{
									Commands_ExecuteByIDUsingEvent((delta > 0) ? kCommandWiderScreen : kCommandNarrowerScreen);
								}
								result = noErr;
							}
							else
							{
								if (false == FlagManager_Test(kFlagKioskMode))
								{
									Commands_ExecuteByIDUsingEvent((delta > 0) ? kCommandTallerScreen : kCommandShorterScreen);
								}
								result = noErr;
							}
						}
					}
					else
					{
						// ordinary scrolling; when in Full Screen mode, scrolling is allowed
						// as long as the user preference to show a scroll bar is set;
						// otherwise, any form of scrolling (via mouse or not) is disabled
						Boolean		allowScrolling = true;
						size_t		actualSize = 0;
						
						
						if (FlagManager_Test(kFlagKioskMode))
						{
							if (kPreferences_ResultOK !=
								Preferences_GetData(kPreferences_TagKioskShowsScrollBar, sizeof(allowScrolling),
													&allowScrolling, &actualSize))
							{
								allowScrolling = true; // assume a value if the preference cannot be found
							}
						}
						
						if (allowScrolling)
						{
							My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
							HIViewRef						scrollBar = ptr->controls.scrollBarV;
							HIViewPartCode					hitPart = (delta > 0)
																		? (modifiers & optionKey)
																			? kControlPageUpPart
																			: kControlUpButtonPart
																		: (modifiers & optionKey)
																			? kControlPageDownPart
																			: kControlDownButtonPart;
							
							
							// vertically scroll the terminal, but 3 lines at a time (scroll wheel)
							InvokeControlActionUPP(scrollBar, hitPart, GetControlAction(scrollBar));
							InvokeControlActionUPP(scrollBar, hitPart, GetControlAction(scrollBar));
							InvokeControlActionUPP(scrollBar, hitPart, GetControlAction(scrollBar));
						}
						result = noErr;
					}
				}
				else
				{
					result = eventNotHandledErr;
				}
			}
		}
	}
	return result;
}// receiveMouseWheelEvent


/*!
Embellishes "kEventControlDraw" of "kEventClassControl"
for scroll bars.

Invoked by Mac OS X whenever a scroll bar needs to be
rendered; calls through to the default renderer, and
then adds “on top” tick marks for any active searches.

(4.0)
*/
OSStatus
receiveScrollBarDraw	(EventHandlerCallRef	inHandlerCallRef,
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	OSStatus		result = eventNotHandledErr;
	HIViewRef		view = nullptr;
	
	
	// first use the system implementation to draw the scroll bar,
	// because this drawing should appear “on top”
	(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// get the target view
	result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
	
	// if the view was found, continue
	if (noErr == result)
	{
		TerminalWindowRef	terminalWindow = nullptr;
		OSStatus			error = noErr;
		UInt32				actualSize = 0L;
		CGContextRef		drawingContext = nullptr;
		
		
		// retrieve TerminalWindowRef from the scroll bar
		error = GetControlProperty(view, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeTerminalWindowRef,
									sizeof(terminalWindow), &actualSize, &terminalWindow);
		assert_noerr(error);
		assert(actualSize == sizeof(terminalWindow));
		
		// determine the context to draw in with Core Graphics
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef,
														drawingContext);
		assert_noerr(result);
		
		// if all information can be found, proceed with drawing
		if ((noErr == error) && (noErr == result) && (nullptr != terminalWindow))
		{
			TerminalScreenRef	activeScreen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
			TerminalViewRef		activeView = TerminalWindow_ReturnViewWithFocus(terminalWindow);
			
			
			// draw line markers
			if (nullptr != activeView)
			{
				HIRect		floatBounds;
				
				
				// determine boundaries of the content view being drawn;
				// ensure view-local coordinates
				HIViewGetBounds(view, &floatBounds);
				
				// overlay tick marks on the region, assuming a vertical scroll bar
				// and using the scale of the underlying terminal screen buffer
				if (TerminalView_SearchResultsExist(activeView))
				{
					TerminalView_CellRangeList	searchResults;
					SInt32						kNumberOfScrollbackLines = Terminal_ReturnInvisibleRowCount(activeScreen);
					SInt32						kNumberOfLines = Terminal_ReturnRowCount(activeScreen) + kNumberOfScrollbackLines;
					TerminalView_Result			viewResult = kTerminalView_ResultOK;
					
					
					viewResult = TerminalView_GetSearchResults(activeView, searchResults);
					if (kTerminalView_ResultOK == viewResult)
					{
						HIRect						trackBounds = floatBounds;
						ThemeScrollBarThumbStyle	arrowLocations = kThemeScrollBarArrowsSingle;
						
						
						// It would be nice to use HITheme APIs here, but after a few trials
						// they seem to be largely broken, returning at best a subset of the
						// required information (e.g. scroll bar boundaries without taking
						// into account the location of arrows).  Therefore, a series of hacks
						// is used instead, to approximate where the arrows will be, in order
						// to avoid drawing on top of any arrows.
						(OSStatus)GetThemeScrollBarThumbStyle(&arrowLocations);
						trackBounds.size.height -= 2 * kMy_ScrollBarThumbEndCapSize;
						trackBounds.origin.y += kMy_ScrollBarThumbEndCapSize;
						if (kThemeScrollBarArrowsSingle == arrowLocations)
						{
							trackBounds.size.height -= 2 * kMy_ScrollBarArrowHeight;
							trackBounds.origin.y += kMy_ScrollBarArrowHeight;
						}
						else
						{
							// make space for two arrows at each end, regardless
							// (since that is a hidden style that many people use)
							trackBounds.size.height -= 4 * kMy_ScrollBarArrowHeight;
							trackBounds.origin.y += 2 * kMy_ScrollBarArrowHeight;
						}
						
						// now draw the tick marks
						{
							float const				kXPad = 4; // in pixels, arbitrary; reduces line size
							float const				kYPad = 0; // in pixels, arbitrary
							float const				kX1 = trackBounds.origin.x + kXPad;
							float const				kX2 = trackBounds.origin.x + trackBounds.size.width - kXPad - kXPad;
							float const				kY1 = trackBounds.origin.y + kYPad;
							float const				kHeight = trackBounds.size.height - kYPad - kYPad;
							CGContextSaveRestore	_(drawingContext);
							float					y = 0;
							SInt32					topRelativeRow = 0;
							
							
							// arbitrary color
							// TEMPORARY - this could be a preference, even if it is just a low-level setting
							CGContextSetRGBStrokeColor(drawingContext, 1.0/* red */, 0/* green */, 0/* blue */, 1.0/* alpha */);
							
							// draw a line in the scroll bar for each thumb
							// TEMPORARY - this might be very inefficient to calculate per draw;
							// it is probably better to detect changes in the search results,
							// cache the line locations, and then render as often as required
							CGContextBeginPath(drawingContext);
							for (TerminalView_CellRangeList::const_iterator toRange = searchResults.begin();
									toRange != searchResults.end(); ++toRange)
							{
								// negative means “in scrollback” and positive means “main screen”, so
								// translate into a single space
								topRelativeRow = toRange->first.second + kNumberOfScrollbackLines;
								
								y = kY1 + topRelativeRow * (kHeight / STATIC_CAST(kNumberOfLines, Float32));
								CGContextMoveToPoint(drawingContext, kX1, y);
								CGContextAddLineToPoint(drawingContext, kX2, y);
							}
							CGContextStrokePath(drawingContext);
						}
					}
				}
			}
		}
	}
	return result;
}// receiveScrollBarDraw


/*!
Handles "kEventControlContextualMenuClick" for a terminal
window tab.

Invoked by Mac OS X whenever the tab is right-clicked.

(4.0)
*/
OSStatus
receiveTabContextualMenuClick	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inTerminalWindowRef)
{
	AutoPool			_;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	OSStatus			result = eventNotHandledErr;
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlContextualMenuClick);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the view in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		if (noErr == result)
		{
			My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
			
			
			// make this the current focus, so that menu commands are sent to it!
			SetUserFocusWindow(HIViewGetWindow(view));
			(OSStatus)DialogUtilities_SetKeyboardFocus(view);
			
			// display a contextual menu
			(OSStatus)ContextualMenuBuilder_DisplayMenuForView(view, inEvent, addContextualMenuItemsForTab);
			result = noErr; // event is completely handled
		}
	}
	return result;
}// receiveTabContextualMenuClick


/*!
Handles "kEventControlDragEnter" for a terminal window tab.

Invoked by Mac OS X whenever the tab is involved in a
drag-and-drop operation.

(3.1)
*/
OSStatus
receiveTabDragDrop	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inTerminalWindowRef)
{
	AutoPool			_;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	OSStatus			result = eventNotHandledErr;
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDragEnter);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target control
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the control was found, continue
		if (noErr == result)
		{
			DragRef		dragRef = nullptr;
			
			
			// determine the drag taking place
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, dragRef);
			if (noErr == result)
			{
				switch (kEventKind)
				{
				case kEventControlDragEnter:
					// indicate whether or not this drag is interesting
					{
						My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
						Boolean							acceptDrag = true;
						
						
						result = SetEventParameter(inEvent, kEventParamControlWouldAcceptDrop,
													typeBoolean, sizeof(acceptDrag), &acceptDrag);
						[ptr->window orderFront:nil];
					}
					break;
				
				default:
					// ???
					result = eventNotHandledErr;
					break;
				}
			}
		}
	}
	return result;
}// receiveTabDragDrop


/*!
Handles "kEventToolbarGetAllowedIdentifiers" and
"kEventToolbarGetDefaultIdentifiers" from "kEventClassToolbar"
for the floating general terminal toolbar.  Responds by
updating the given lists of identifiers.

(3.1)
*/
OSStatus
receiveToolbarEvent		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inTerminalWindowRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassToolbar);
	{
		HIToolbarRef	toolbarRef = nullptr;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbar, typeHIToolbarRef, toolbarRef);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventToolbarGetAllowedIdentifiers:
				{
					CFMutableArrayRef	allowedIdentifiers = nullptr;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMutableArray,
																	typeCFMutableArrayRef, allowedIdentifiers);
					if (noErr == result)
					{
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED1);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED2);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED3);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED4);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDScrollLock);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDHideWindow);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDFullScreen);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalBell);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarSpaceIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarFlexibleSpaceIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDPrint);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDCustomize);
					}
				}
				break;
			
			case kEventToolbarGetDefaultIdentifiers:
				{
					CFMutableArrayRef	defaultIdentifiers = nullptr;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMutableArray,
																	typeCFMutableArrayRef, defaultIdentifiers);
					if (noErr == result)
					{
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSpaceIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSpaceIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDHideWindow);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDScrollLock);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalBell);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarFlexibleSpaceIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED1);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED2);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED3);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED4);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarFlexibleSpaceIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDPrint);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDFullScreen);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDCustomize);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSpaceIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSpaceIdentifier);
					}
				}
				break;
			
			case kEventToolbarCreateItemWithIdentifier:
				{
					CFStringRef		identifierCFString = nullptr;
					HIToolbarRef	targetToolbar = nullptr;
					HIToolbarRef	terminalToolbar = nullptr;
					Boolean			isPermanentItem = false;
					
					
					// see if this item is for a toolbar; if not, it may be used for something
					// else (like a customization sheet); this is used to determine whether or
					// not to save the toolbar item reference for later use (e.g. icon updates)
					if (noErr != CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbar,
																		typeHIToolbarRef, targetToolbar))
					{
						targetToolbar = nullptr;
					}
					isPermanentItem = (nullptr != targetToolbar);
					if (noErr == GetWindowToolbar(TerminalWindow_ReturnWindow(terminalWindow), &terminalToolbar))
					{
						isPermanentItem = ((isPermanentItem) && (terminalToolbar == targetToolbar));
					}
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbarItemIdentifier,
																	typeCFStringRef, identifierCFString);
					if (noErr == result)
					{
						CFTypeRef	itemData = nullptr;
						
						
						// NOTE: configuration data is not always present
						result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbarItemConfigData,
																		typeCFTypeRef, itemData);
						if (noErr != result)
						{
							itemData = nullptr;
							result = noErr;
						}
						
						// create the specified item, if its identifier is recognized
						{
							HIToolbarItemRef	itemRef = nullptr;
							Boolean const		kIs1 = (kCFCompareEqualTo == CFStringCompare
																				(kConstantsRegistry_HIToolbarItemIDTerminalLED1,
																					identifierCFString,
																					kCFCompareBackwards));
							Boolean const		kIs2 = (kCFCompareEqualTo == CFStringCompare
																				(kConstantsRegistry_HIToolbarItemIDTerminalLED2,
																					identifierCFString,
																					kCFCompareBackwards));
							Boolean const		kIs3 = (kCFCompareEqualTo == CFStringCompare
																				(kConstantsRegistry_HIToolbarItemIDTerminalLED3,
																					identifierCFString,
																					kCFCompareBackwards));
							Boolean const		kIs4 = (kCFCompareEqualTo == CFStringCompare
																				(kConstantsRegistry_HIToolbarItemIDTerminalLED4,
																					identifierCFString,
																					kCFCompareBackwards));
							
							
							// all LED items are very similar in appearance, so check all at once
							if ((kIs1) || (kIs2) || (kIs3) || (kIs4))
							{
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
								if (noErr == HIToolbarItemCreate(identifierCFString,
																	kHIToolbarItemNoAttributes, &itemRef))
								{
									CFStringRef		nameCFString = nullptr;
									
									
									// set the label based on which LED is being created
									if (kIs1)
									{
										UInt32 const	kMyCommandID = kCommandToggleTerminalLED1;
										
										
										// then this is the LED 1 item; remember it so it can be updated later
										if (isPermanentItem)
										{
											ptr->toolbarItemLED1.setCFTypeRef(itemRef);
										}
										
										if (UIStrings_Copy(kUIStrings_ToolbarItemTerminalLED1, nameCFString).ok())
										{
											result = HIToolbarItemSetLabel(itemRef, nameCFString);
											assert_noerr(result);
											result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																				nullptr/* long text */);
											assert_noerr(result);
											result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
											assert_noerr(result);
											CFRelease(nameCFString), nameCFString = nullptr;
										}
									}
									else if (kIs2)
									{
										UInt32 const	kMyCommandID = kCommandToggleTerminalLED2;
										
										
										// then this is the LED 2 item; remember it so it can be updated later
										if (isPermanentItem)
										{
											ptr->toolbarItemLED2.setCFTypeRef(itemRef);
										}
										
										if (UIStrings_Copy(kUIStrings_ToolbarItemTerminalLED2, nameCFString).ok())
										{
											result = HIToolbarItemSetLabel(itemRef, nameCFString);
											assert_noerr(result);
											result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																				nullptr/* long text */);
											assert_noerr(result);
											result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
											assert_noerr(result);
											CFRelease(nameCFString), nameCFString = nullptr;
										}
									}
									else if (kIs3)
									{
										UInt32 const	kMyCommandID = kCommandToggleTerminalLED3;
										
										
										// then this is the LED 3 item; remember it so it can be updated later
										if (isPermanentItem)
										{
											ptr->toolbarItemLED3.setCFTypeRef(itemRef);
										}
										
										if (UIStrings_Copy(kUIStrings_ToolbarItemTerminalLED3, nameCFString).ok())
										{
											result = HIToolbarItemSetLabel(itemRef, nameCFString);
											assert_noerr(result);
											result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																				nullptr/* long text */);
											assert_noerr(result);
											result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
											assert_noerr(result);
											CFRelease(nameCFString), nameCFString = nullptr;
										}
									}
									else if (kIs4)
									{
										UInt32 const	kMyCommandID = kCommandToggleTerminalLED4;
										
										
										// then this is the LED 4 item; remember it so it can be updated later
										if (isPermanentItem)
										{
											ptr->toolbarItemLED4.setCFTypeRef(itemRef);
										}
										
										if (UIStrings_Copy(kUIStrings_ToolbarItemTerminalLED4, nameCFString).ok())
										{
											result = HIToolbarItemSetLabel(itemRef, nameCFString);
											assert_noerr(result);
											result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																				nullptr/* long text */);
											assert_noerr(result);
											result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
											assert_noerr(result);
											CFRelease(nameCFString), nameCFString = nullptr;
										}
									}
									
									// set icon; currently, all LEDs have the same icon, but perhaps
									// some day they will have different colors, etc.
									result = HIToolbarItemSetIconRef(itemRef, gLEDOffIcon());
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDScrollLock,
																			identifierCFString, kCFCompareBackwards))
							{
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandSuspendNetwork;
									CFStringRef		nameCFString = nullptr;
									
									
									// remember this item so its icon can be kept in sync with scroll lock state
									if (isPermanentItem)
									{
										ptr->toolbarItemScrollLock.setCFTypeRef(itemRef);
									}
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = HIToolbarItemSetIconRef(itemRef, gScrollLockOnIcon());
									assert_noerr(result);
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDHideWindow,
																			identifierCFString, kCFCompareBackwards))
							{
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandHideFrontWindow;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = HIToolbarItemSetIconRef(itemRef, gHideWindowIcon());
									assert_noerr(result);
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDFullScreen,
																			identifierCFString, kCFCompareBackwards))
							{
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandFullScreenModal;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = HIToolbarItemSetIconRef(itemRef, gFullScreenIcon());
									assert_noerr(result);
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDCustomize,
																			identifierCFString, kCFCompareBackwards))
							{
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kHICommandCustomizeToolbar;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = HIToolbarItemSetIconRef(itemRef, gCustomizeToolbarIcon());
									assert_noerr(result);
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDPrint,
																			identifierCFString, kCFCompareBackwards))
							{
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandPrint;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = HIToolbarItemSetIconRef(itemRef, gPrintIcon());
									assert_noerr(result);
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDTerminalBell,
																			identifierCFString, kCFCompareBackwards))
							{
								My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandBellEnabled;
									CFStringRef		nameCFString = nullptr;
									
									
									// then this is the bell item; remember it so it can be updated later
									if (isPermanentItem)
									{
										ptr->toolbarItemBell.setCFTypeRef(itemRef);
									}
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = HIToolbarItemSetIconRef(itemRef, gBellOffIcon());
									assert_noerr(result);
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							
							if (nullptr == itemRef)
							{
								result = eventNotHandledErr;
							}
							else
							{
								result = SetEventParameter(inEvent, kEventParamToolbarItem, typeHIToolbarItemRef,
															sizeof(itemRef), &itemRef);
							}
						}
					}
				}
				break;
			
			case kEventToolbarItemRemoved:
				// if the removed item was a known status item, forget its reference
				{
					HIToolbarItemRef	removedItem = nullptr;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbarItem,
																	typeHIToolbarItemRef, removedItem);
					if (noErr == result)
					{
						CFStringRef		identifierCFString = nullptr;
						
						
						result = HIToolbarItemCopyIdentifier(removedItem, &identifierCFString);
						if (noErr == result)
						{
							My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
							
							
							// forget any stale references to important items being removed
							if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDScrollLock,
																		identifierCFString, kCFCompareBackwards))
							{
								ptr->toolbarItemScrollLock.clear();
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDTerminalBell,
																		identifierCFString, kCFCompareBackwards))
							{
								ptr->toolbarItemBell.clear();
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDTerminalLED1,
																		identifierCFString, kCFCompareBackwards))
							{
								ptr->toolbarItemLED1.clear();
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDTerminalLED2,
																			identifierCFString, kCFCompareBackwards))
							{
								ptr->toolbarItemLED2.clear();
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDTerminalLED3,
																			identifierCFString, kCFCompareBackwards))
							{
								ptr->toolbarItemLED3.clear();
							}
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDTerminalLED4,
																			identifierCFString, kCFCompareBackwards))
							{
								ptr->toolbarItemLED4.clear();
							}
							
							CFRelease(identifierCFString), identifierCFString = nullptr;
						}
					}
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// receiveToolbarEvent


/*!
Handles "kEventWindowCursorChange" of "kEventClassWindow",
or "kEventRawKeyModifiersChanged" of "kEventClassKeyboard",
for a terminal window.

IMPORTANT:	This is completely generic.  It should move
			into some other module, so it can be used as
			a utility for other windows.

(3.1)
*/
OSStatus
receiveWindowCursorChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inWindowRef)
{
	OSStatus		result = eventNotHandledErr;
	WindowRef		targetWindow = REINTERPRET_CAST(inWindowRef, WindowRef);
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(((kEventClass == kEventClassWindow) && (kEventKind == kEventWindowCursorChange)) ||
			((kEventClass == kEventClassKeyboard) && (kEventKind == kEventRawKeyModifiersChanged)));
	
	// do not change the cursor if this window is not active
	if (GetUserFocusWindow() == targetWindow)
	{
		if (kEventClass == kEventClassWindow)
		{
			WindowRef	window = nullptr;
			
			
			// determine the window in question
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
			if (noErr == result)
			{
				Point	globalMouse;
				
				
				assert(window == targetWindow);
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, globalMouse);
				if (noErr == result)
				{
					UInt32		modifiers = 0;
					
					
					// try to vary the cursor according to key modifiers, but it’s no
					// catastrophe if this information isn’t available
					(OSStatus)CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
					
					// finally, set the cursor
					result = setCursorInWindow(window, globalMouse, modifiers);
				}
			}
		}
		else
		{
			// when the key modifiers change, it is still nice to have the
			// cursor automatically change when necessary; however, there
			// is no mouse information available, so it must be determined
			UInt32		modifiers = 0;
			
			
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
			if (noErr == result)
			{
				Point	globalMouse;
				
				
				GetMouse(&globalMouse);
				result = setCursorInWindow(targetWindow, globalMouse, modifiers);
			}
		}
	}
	
	return result;
}// receiveWindowCursorChange


/*!
Handles "kEventWindowDragCompleted" of "kEventClassWindow"
for a terminal window.

(3.1)
*/
OSStatus
receiveWindowDragCompleted	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inTerminalWindowRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowDragCompleted);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
			
			
			// check the tab location and fix if necessary
			if (ptr->tab.exists())
			{
				HIWindowRef		tabWindow = REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef);
				
				
				if (GetDrawerPreferredEdge(tabWindow) != GetDrawerCurrentEdge(tabWindow))
				{
					OSStatus	error = noErr;
					
					
					// toggle twice; the first should close the drawer at its
					// “wrong” location, the 2nd should open it on the right edge
					error = CloseDrawer(tabWindow, false/* asynchronously */);
					if (noErr == error)
					{
						error = OpenDrawer(tabWindow, kWindowEdgeDefault, true/* asynchronously */);
					}
				}
			}
		}
	}
	
	return result;
}// receiveWindowDragCompleted


/*!
Handles "kEventWindowGetClickActivation" of "kEventClassWindow"
for a terminal window.

(3.1)
*/
OSStatus
receiveWindowGetClickActivation		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef				inEvent,
									 void*					inTerminalWindowRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowGetClickActivation);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
			HIViewRef						control = nullptr;
			UInt32							actualSize = 0L;
			EventParamType					actualType = typeNull;
			
			
			// only clicks in controls matter for this
			result = eventNotHandledErr;
			if (noErr == GetEventParameter(inEvent, kEventParamControlRef, typeControlRef, &actualType,
											sizeof(control), &actualSize, &control))
			{
				// find clicks in terminal regions
				My_TerminalViewList::const_iterator		viewIterator;
				
				
				for (viewIterator = ptr->allViews.begin(); viewIterator != ptr->allViews.end(); ++viewIterator)
				{
					if (TerminalView_ReturnDragFocusHIView(*viewIterator) == control)
					{
						// clicks in terminals may work from the background
						ClickActivationResult	clickInfo = kActivateAndIgnoreClick;
						Point					eventMouse;
						
						
						if (noErr == GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, &actualType,
														sizeof(eventMouse), &actualSize, &eventMouse))
						{
							// preferably only allow click-through when a background selection is touched
							if (WaitMouseMoved(eventMouse))
							{
								// allow pending drags only
								CGrafPtr	oldPort = nullptr;
								GDHandle	oldDevice = nullptr;
								
								
								GetGWorld(&oldPort, &oldDevice);
								SetPortWindowPort(TerminalView_ReturnWindow(*viewIterator));
								GlobalToLocal(&eventMouse);
								if (TerminalView_PtInSelection(*viewIterator, eventMouse))
								{
									clickInfo = kActivateAndHandleClick;
								}
								SetGWorld(oldPort, oldDevice);
							}
						}
						result = SetEventParameter(inEvent, kEventParamClickActivation,
													typeClickActivationResult, sizeof(clickInfo), &clickInfo);
						break;
					}
				}
			}
		}
	}
	
	return result;
}// receiveWindowGetClickActivation


/*!
Embellishes "kEventWindowResizeStarted", "kEventWindowBoundsChanging",
and "kEventWindowResizeCompleted" of "kEventClassWindow" for a
terminal window.

(3.0)
*/
OSStatus
receiveWindowResize		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inTerminalWindowRef)
{
	AutoPool			_;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	OSStatus			result = eventNotHandledErr;
	
	
	assert(kEventClass == kEventClassWindow);
	assert((kEventKind == kEventWindowResizeStarted) ||
			(kEventKind == kEventWindowBoundsChanging) ||
			(kEventKind == kEventWindowResizeCompleted));
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			Boolean							useSheet = false;
			Boolean							showWindow = FlagManager_Test(kFlagOS10_3API);
			My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
			
			
			if (kEventKind == kEventWindowResizeStarted)
			{
				TerminalViewRef		focusedView = TerminalWindow_ReturnViewWithFocus(terminalWindow);
				
				
				// on Mac OS X 10.7 and beyond, the system can initiate resizes in ways that do
				// not allow the detection of modifier keys; so unfortunately it is necessary
				// to check the raw key state at this point (e.g. responding to clicks in a
				// size box will no longer work)
				if (nullptr != focusedView)
				{
					// remember the previous view mode, so that it can be restored later
					ptr->preResizeViewDisplayMode = TerminalView_ReturnDisplayMode(focusedView);
					
					// the Control key must be used because Mac OS X 10.7 assigns special
					// meaning to the Shift and Option keys during a window resize
					if (EventLoop_IsControlKeyDown())
					{
						TerminalView_SetDisplayMode(focusedView,
													(kTerminalView_DisplayModeNormal ==
														TerminalView_ReturnDisplayMode(focusedView))
														? kTerminalView_DisplayModeZoom
														: kTerminalView_DisplayModeNormal);
					}
				}	
				
				// load the NIB containing this dialog (automatically finds the right localization)
				ptr->resizeFloater = NIBWindow(AppResources_ReturnBundleForNIBs(), CFSTR("TerminalWindow"),
												useSheet ? CFSTR("DimensionsSheet") : CFSTR("DimensionsFloater"))
										<< NIBLoader_AssertWindowExists;
				
				if (nullptr != ptr->resizeFloater)
				{
					OSStatus	error = noErr;
					
					
					error = GetControlByID(ptr->resizeFloater, &idMyTextScreenDimensions, &ptr->controls.textScreenDimensions);
					if (error != noErr) ptr->controls.textScreenDimensions = nullptr;
					
					// change font to something a little clearer
					(OSStatus)Localization_SetControlThemeFontInfo(ptr->controls.textScreenDimensions,
																	kThemeAlertHeaderFont);
					
					if (showWindow)
					{
						if (useSheet)
						{
							ShowSheetWindow(ptr->resizeFloater, returnCarbonWindow(ptr));
						}
						else
						{
							ShowWindow(ptr->resizeFloater);
						}
					}
				}
				
				// remember the old window title
				{
					CFStringRef		nameCFString = (CFStringRef)[ptr->window title];
					
					
					ptr->preResizeTitleString.setCFTypeRef(nameCFString);
				}
			}
			else if ((kEventKind == kEventWindowBoundsChanging) && (ptr->preResizeTitleString.exists()))
			{
				// for bounds-changing, ensure a resize is in progress (denoted
				// by a non-nullptr preserved title string), make sure the window
				// bounds are changing because of a user interaction, and make
				// sure the dimensions themselves are changing
				UInt32		attributes = 0L;
				
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamAttributes, typeUInt32, attributes);
				if ((result == noErr) && (attributes & kWindowBoundsChangeUserResize) &&
					(attributes & kWindowBoundsChangeSizeChanged))
				{
					// update display; the contents of the display depend on the view mode,
					// either dimension changing (the default) or font size changing
					CFStringRef			newTitle = nullptr;
					TerminalViewRef		focusedView = TerminalWindow_ReturnViewWithFocus(terminalWindow);
					Boolean				isFontSizeDisplay = false;
					
					
					if (nullptr != focusedView)
					{
						isFontSizeDisplay = (kTerminalView_DisplayModeZoom == TerminalView_ReturnDisplayMode(focusedView));
					}
					
					if (isFontSizeDisplay)
					{
						// font size display
						UInt16		fontSize = 0;
						
						
						TerminalWindow_GetFontAndSize(terminalWindow, nullptr/* font */, &fontSize);
						newTitle = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */,
															CFSTR("%d pt")/* LOCALIZE THIS */, fontSize);
					}
					else
					{
						// columns and rows display
						UInt16		columns = 0;
						UInt16		rows = 0;
						
						
						TerminalWindow_GetScreenDimensions(terminalWindow, &columns, &rows);
						newTitle = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */,
															CFSTR("%dx%d")/* LOCALIZE THIS */, columns, rows);
					}
					
					if (nullptr != newTitle)
					{
						unless (useSheet)
						{
							// here the title is set directly, instead of via the
							// TerminalWindow_SetWindowTitle() routine, because
							// it is a “secret” and temporary change to the title
							// that will be undone when resizing completes
							[ptr->window setTitle:(NSString*)newTitle];
						}
						
						// update the floater
						SetControlTextWithCFString(ptr->controls.textScreenDimensions, newTitle);
						
						CFRelease(newTitle), newTitle = nullptr;
					}
				}
			}
			else if (kEventKind == kEventWindowResizeCompleted)
			{
				if (useSheet)
				{
					HideSheetWindow(ptr->resizeFloater);
				}
				else
				{
					HideWindow(ptr->resizeFloater);
				}
				
				// in case the reverse resize mode was enabled by the resize click, restore the original resize mode
				{
					TerminalViewRef		focusedView = TerminalWindow_ReturnViewWithFocus(terminalWindow);
					
					
					if (nullptr != focusedView)
					{
						TerminalView_SetDisplayMode(focusedView, ptr->preResizeViewDisplayMode);
					}
				}
				
				// dispose of the floater
				DisposeWindow(ptr->resizeFloater), ptr->resizeFloater = nullptr;
				ptr->controls.textScreenDimensions = nullptr; // implicitly destroyed with the window
				
				// restore the window title
				if (ptr->preResizeTitleString.exists())
				{
					[ptr->window setTitle:(NSString*)ptr->preResizeTitleString.returnCFStringRef()];
					ptr->preResizeTitleString.clear();
				}
			}
		}
	}
	
	return result;
}// receiveWindowResize


/*!
Implementation of TerminalWindow_ReturnWindow().

(4.0)
*/
HIWindowRef
returnCarbonWindow		(My_TerminalWindowPtr	inPtr)
{
	AutoPool		_;
	HIWindowRef		result = nullptr;
	
	
	result = (HIWindowRef)[inPtr->window windowRef];
	return result;
}// returnCarbonWindow


/*!
Returns the height in pixels of the grow box in a terminal
window.  Currently this is identical to the height of a
horizontal scroll bar, but this function exists so that
code can explicitly identify this metric when no horizontal
scroll bar may be present or when the size box is missing
(Full Screen mode).

(3.0)
*/
UInt16
returnGrowBoxHeight		(My_TerminalWindowPtr	inPtr)
{
	UInt16				result = 0;
	Boolean				hasSizeBox = false;
	WindowAttributes	attributes = kWindowNoAttributes;
	OSStatus			error = noErr;
	
	
	error = GetWindowAttributes(returnCarbonWindow(inPtr), &attributes);
	if (noErr != error)
	{
		// not sure if the window has a size box; assume it does
		hasSizeBox = true;
	}
	else
	{
		if (attributes & kWindowResizableAttribute)
		{
			hasSizeBox = true;
		}
	}
	
	if (hasSizeBox)
	{
		SInt32		data = 0;
		
		
		error = GetThemeMetric(kThemeMetricScrollBarWidth, &data);
		if (noErr != error)
		{
			Console_WriteValue("unexpected error using GetThemeMetric()", error);
			result = 16; // arbitrary
		}
		else
		{
			result = data;
		}
	}
	return result;
}// returnGrowBoxHeight


/*!
Returns the width in pixels of the grow box in a terminal
window.  Currently this is identical to the width of a
vertical scroll bar, but this function exists so that code
can explicitly identify this metric when no vertical scroll
bar may be present.

(3.0)
*/
UInt16
returnGrowBoxWidth		(My_TerminalWindowPtr	inPtr)
{
	return returnScrollBarWidth(inPtr);
}// returnGrowBoxWidth


/*!
Returns the height in pixels of a scroll bar in the given
terminal window.  If the scroll bar is invisible, the height
is set to 0.

(3.0)
*/
UInt16
returnScrollBarHeight	(My_TerminalWindowPtr	UNUSED_ARGUMENT(inPtr))
{
	UInt16		result = 0;
	//SInt32		data = 0L;
	//OSStatus	error = noErr;
	
	
	// temporarily disable horizontal scroll bars; one option
	// is to have this routine dynamically return a nonzero
	// height if the terminal actually needs horizontal
	// scrolling (extremely rare), and to otherwise use 0 to
	// effectively hide the scroll bar and save some space
#if 0
	error = GetThemeMetric(kThemeMetricScrollBarWidth, &data);
	if (error != noErr) Console_WriteValue("unexpected error using GetThemeMetric()", error);
	result = data;
#endif
	return result;
}// returnScrollBarHeight


/*!
Returns the width in pixels of a scroll bar in the given
terminal window.  If the scroll bar is invisible, the width
is set to 0.

(3.0)
*/
UInt16
returnScrollBarWidth	(My_TerminalWindowPtr	UNUSED_ARGUMENT(inPtr))
{
	UInt16		result = 0;
	Boolean		showScrollBar = true;
	size_t		actualSize = 0;
	
	
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskShowsScrollBar, sizeof(showScrollBar),
							&showScrollBar, &actualSize))
	{
		showScrollBar = true; // assume a value if the preference cannot be found
	}
	
	if ((false == FlagManager_Test(kFlagKioskMode)) || (showScrollBar))
	{
		SInt32		data = 0L;
		OSStatus	error = noErr;
		
		
		error = GetThemeMetric(kThemeMetricScrollBarWidth, &data);
		if (error != noErr) Console_WriteValue("unexpected error using GetThemeMetric()", error);
		result = data;
	}
	return result;
}// returnScrollBarWidth


/*!
Returns the height in pixels of the status bar.  The status bar
height is defined as the number of pixels between the toolbar
and the top edge of the terminal screen; thus, an invisible
status bar has zero height.

(3.0)
*/
UInt16
returnStatusBarHeight	(My_TerminalWindowPtr	UNUSED_ARGUMENT(inPtr))
{
	return 0;
}// returnStatusBarHeight


/*!
Returns the height in pixels of the toolbar.  The toolbar
height is defined as the number of pixels between the top
edge of the window and the top edge of the status bar; thus,
an invisible toolbar has zero height.

(3.0)
*/
UInt16
returnToolbarHeight		(My_TerminalWindowPtr	UNUSED_ARGUMENT(inPtr))
{
	return 0;
}// returnToolbarHeight


/*!
This routine, of standard UndoActionProcPtr form,
can undo or redo changes to the font and/or font
size of a terminal screen.

Note that it will really be nice to get all of
the AppleScript stuff working, so junk like this
does not have to be done using Copy-and-Paste
coding™, and can be made into a recordable event.

(3.0)
*/
void
reverseFontChanges	(Undoables_ActionInstruction	inDoWhat,
					 Undoables_ActionRef			inApplicableAction,
					 void*							inContextPtr)
{
	// this routine only recognizes one kind of context - be absolutely sure that’s what was given!
	assert(Undoables_ReturnActionID(inApplicableAction) == kUndoableContextIdentifierTerminalFontSizeChanges);
	
	{
		UndoDataFontSizeChangesPtr	dataPtr = REINTERPRET_CAST(inContextPtr, UndoDataFontSizeChangesPtr);
		
		
		switch (inDoWhat)
		{
		case kUndoables_ActionInstructionDispose:
			// release memory previously allocated when this action was installed
			Undoables_DisposeAction(&inApplicableAction);
			if (dataPtr != nullptr) Memory_DisposePtr(REINTERPRET_CAST(&dataPtr, Ptr*));
			break;
		
		case kUndoables_ActionInstructionRedo:
		case kUndoables_ActionInstructionUndo:
		default:
			{
				UInt16		oldFontSize = 0;
				Str255		oldFontName;
				
				
				// make this reversible by preserving the font information
				TerminalWindow_GetFontAndSize(dataPtr->terminalWindow, oldFontName, &oldFontSize);
				
				// change the font and/or size of the window
				TerminalWindow_SetFontAndSize(dataPtr->terminalWindow,
												(dataPtr->undoFont) ? dataPtr->fontName : nullptr,
												(dataPtr->undoFontSize) ? dataPtr->fontSize : 0);
				
				// save the font and size
				dataPtr->fontSize = oldFontSize;
				PLstrcpy(dataPtr->fontName, oldFontName);
			}
			break;
		}
	}
}// reverseFontChanges


/*!
This routine, of standard UndoActionProcPtr form,
can undo or redo changes to the font size and
location of a terminal screen (made by going into
Full Screen mode).

(3.0)
*/
void
reverseFullScreenChanges	(Undoables_ActionInstruction	inDoWhat,
							 Undoables_ActionRef			inApplicableAction,
							 void*							inContextPtr)
{
	// this routine only recognizes one kind of context - be absolutely sure that’s what was given!
	assert(Undoables_ReturnActionID(inApplicableAction) == kUndoableContextIdentifierTerminalFullScreenChanges);
	
	{
		UndoDataFullScreenChangesPtr	dataPtr = REINTERPRET_CAST(inContextPtr, UndoDataFullScreenChangesPtr);
		
		
		switch (inDoWhat)
		{
		case kUndoables_ActionInstructionDispose:
			// release memory previously allocated when this action was installed
			Undoables_DisposeAction(&inApplicableAction);
			if (dataPtr != nullptr) Memory_DisposePtr(REINTERPRET_CAST(&dataPtr, Ptr*));
			break;
		
		case kUndoables_ActionInstructionUndo:
			// exit Full Screen mode and restore the window
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), dataPtr->terminalWindow);
				
				
				// restore the size box
				(OSStatus)ChangeWindowAttributes(returnCarbonWindow(ptr),
													kWindowCollapseBoxAttribute | kWindowFullZoomAttribute |
														kWindowResizableAttribute/* attributes to set */,
													0/* attributes to clear */);
				
				FlagManager_Set(kFlagKioskMode, false);
				Keypads_SetVisible(kKeypads_WindowTypeFullScreen, false);
				if (nullptr != ptr) (OSStatus)HIViewSetVisible(ptr->controls.scrollBarV, true);
				assert_noerr(SetSystemUIMode(kUIModeNormal, 0/* options */));
				
				// some mode changes do not require font size restoration; and, the boundaries
				// may need to change before or after the display mode is restored, due to the
				// side effects of window resizes in each mode
				TerminalView_SetDisplayMode(ptr->allViews.front(), dataPtr->newMode);
				if ((dataPtr->oldMode != dataPtr->newMode) && (kTerminalView_DisplayModeNormal == dataPtr->oldMode))
				{
					TerminalWindow_SetFontAndSize(dataPtr->terminalWindow, nullptr/* font name */, dataPtr->oldFontSize);
				}
				if ((dataPtr->oldMode != dataPtr->newMode) && (kTerminalView_DisplayModeNormal != dataPtr->newMode))
				{
					TerminalView_SetDisplayMode(ptr->allViews.front(), dataPtr->oldMode);
				}
				setViewSizeIndependentFromWindow(ptr, true);
				SetWindowBounds(TerminalWindow_ReturnWindow(dataPtr->terminalWindow), kWindowContentRgn, &dataPtr->oldContentBounds);
				setViewSizeIndependentFromWindow(ptr, false);
				if ((dataPtr->oldMode != dataPtr->newMode) && (kTerminalView_DisplayModeNormal == dataPtr->newMode))
				{
					TerminalView_SetDisplayMode(ptr->allViews.front(), dataPtr->oldMode);
				}
			}
			break;
		
		case kUndoables_ActionInstructionRedo:
		default:
			// ???
			break;
		}
	}
}// reverseFullScreenChanges


/*!
This routine, of standard UndoActionProcPtr form,
can undo or redo changes to the dimensions of a
terminal screen.

(3.1)
*/
void
reverseScreenDimensionChanges	(Undoables_ActionInstruction	inDoWhat,
								 Undoables_ActionRef			inApplicableAction,
								 void*							inContextPtr)
{
	// this routine only recognizes one kind of context - be absolutely sure that’s what was given!
	assert(Undoables_ReturnActionID(inApplicableAction) == kUndoableContextIdentifierTerminalDimensionChanges);
	
	{
		UndoDataScreenDimensionChangesPtr	dataPtr = REINTERPRET_CAST(inContextPtr, UndoDataScreenDimensionChangesPtr);
		
		
		switch (inDoWhat)
		{
		case kUndoables_ActionInstructionDispose:
			// release memory previously allocated when this action was installed
			Undoables_DisposeAction(&inApplicableAction);
			if (dataPtr != nullptr) Memory_DisposePtr(REINTERPRET_CAST(&dataPtr, Ptr*));
			break;
		
		case kUndoables_ActionInstructionRedo:
		case kUndoables_ActionInstructionUndo:
		default:
			{
				UInt16		oldColumns = 0;
				UInt16		oldRows = 0;
				
				
				// make this reversible by preserving the dimensions
				TerminalWindow_GetScreenDimensions(dataPtr->terminalWindow, &oldColumns, &oldRows);
				
				// resize the window
				TerminalWindow_SetScreenDimensions(dataPtr->terminalWindow, dataPtr->columns, dataPtr->rows,
													true/* recordable */);
				
				// save the dimensions
				dataPtr->columns = oldColumns;
				dataPtr->rows = oldRows;
			}
			break;
		}
	}
}// reverseScreenDimensionChanges


/*!
This is a standard control action procedure that dynamically
scrolls a terminal window.

(2.6)
*/
void
scrollProc	(HIViewRef			inScrollBarClicked,
			 HIViewPartCode		inPartCode)
{
	TerminalWindowRef	terminalWindow = nullptr;
	OSStatus			error = noErr;
	UInt32				actualSize = 0L;
	
	
	// retrieve TerminalWindowRef from control
	error = GetControlProperty(inScrollBarClicked, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeTerminalWindowRef,
								sizeof(terminalWindow), &actualSize, &terminalWindow);
	assert_noerr(error);
	assert(actualSize == sizeof(terminalWindow));
	
	if (nullptr != terminalWindow)
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
		TerminalScreenRef				screen = nullptr;
		TerminalViewRef					view = nullptr;
		My_ScrollBarKind				kind = kMy_InvalidScrollBarKind;
		SInt16							visibleColumnCount = 0;
		SInt16							visibleRowCount = 0;
		
		
		screen = getScrollBarScreen(ptr, inScrollBarClicked); // 3.0
		view = getScrollBarView(ptr, inScrollBarClicked); // 3.0
		kind = getScrollBarKind(ptr, inScrollBarClicked); // 3.0
		
		visibleColumnCount = Terminal_ReturnColumnCount(screen);
		visibleRowCount = Terminal_ReturnRowCount(screen);
		
		if (kMy_ScrollBarKindHorizontal == kind)
		{
			switch (inPartCode)
			{
			case kControlUpButtonPart: // “up arrow” on a horizontal scroll bar means “left arrow”
				(TerminalView_Result)TerminalView_ScrollColumnsTowardRightEdge(view, 1/* number of columns to scroll */);
				break;
			
			case kControlDownButtonPart: // “down arrow” on a horizontal scroll bar means “right arrow”
				(TerminalView_Result)TerminalView_ScrollColumnsTowardLeftEdge(view, 1/* number of columns to scroll */);
				break;
			
			case kControlPageUpPart:
				(TerminalView_Result)TerminalView_ScrollPageTowardRightEdge(view);
				break;
			
			case kControlPageDownPart:
				(TerminalView_Result)TerminalView_ScrollPageTowardLeftEdge(view);
				break;
			
			case kControlIndicatorPart:
				(TerminalView_Result)TerminalView_ScrollTo(view, GetControl32BitValue(ptr->controls.scrollBarV),
															GetControl32BitValue(ptr->controls.scrollBarH));
				break;
			
			default:
				// ???
				break;
			}
		}
		else if (kMy_ScrollBarKindVertical == kind)
		{
			switch (inPartCode)
			{
			case kControlUpButtonPart:
				(TerminalView_Result)TerminalView_ScrollRowsTowardBottomEdge(view, 1/* number of rows to scroll */);
				break;
			
			case kControlDownButtonPart:
				(TerminalView_Result)TerminalView_ScrollRowsTowardTopEdge(view, 1/* number of rows to scroll */);
				break;
			
			case kControlPageUpPart:
				(TerminalView_Result)TerminalView_ScrollPageTowardBottomEdge(view);
				break;
			
			case kControlPageDownPart:
				(TerminalView_Result)TerminalView_ScrollPageTowardTopEdge(view);
				break;
			
			case kControlIndicatorPart:
				(TerminalView_Result)TerminalView_ScrollTo(view, GetControl32BitValue(ptr->controls.scrollBarV));
				break;
			
			default:
				// ???
				break;
			}
		}
	}
}// scrollProc


/*!
Invoked whenever a monitored session state is changed
(see TerminalWindow_New() to see which states are
monitored).  This routine responds by updating session
windows appropriately.

(3.0)
*/
void
sessionStateChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inSessionSettingThatChanged,
						 void*					inEventContextPtr,
						 void*					inTerminalWindowRef)
{
	AutoPool			_;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	
	
	switch (inSessionSettingThatChanged)
	{
	case kSession_ChangeSelected:
		// bring the window to the front, unhiding it if necessary
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
			// this handler is invoked for changes to ANY session,
			// but the response is specific to one, so check first
			if (Session_ReturnActiveTerminalWindow(session) == terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				
				
				TerminalWindow_SetObscured(terminalWindow, false);
				[ptr->window makeKeyAndOrderFront:nil];
			}
		}
		break;
	
	case kSession_ChangeState:
		// update various GUI elements to reflect the new session state
		{
			SessionRef						session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
			
			
			// this handler is invoked for changes to ANY session,
			// but the response is specific to one, so check first
			if (Session_ReturnActiveTerminalWindow(session) == terminalWindow)
			{
				// set the initial window title
				if (Session_StateIsActiveUnstable(session))
				{
					CFStringRef		titleCFString = nullptr;
					
					
					if (kSession_ResultOK == Session_GetWindowUserDefinedTitle(session, titleCFString))
					{
						TerminalWindow_SetWindowTitle(terminalWindow, titleCFString);
					}
				}
				// add or remove window adornments as appropriate; once a session has died
				// its window (if left open by the user) won’t display a warning, so the
				// adornment is removed in that case, although its title is then changed
				if (Session_StateIsActiveStable(session)) setWarningOnWindowClose(ptr, true);
				if (Session_StateIsDead(session))
				{
					ptr->isDead = true;
					TerminalWindow_SetWindowTitle(terminalWindow, nullptr/* keep title, evaluate state again */);
					setWarningOnWindowClose(ptr, false);
				}
				else
				{
					ptr->isDead = false;
				}
			}
		}
		break;
	
	case kSession_ChangeStateAttributes:
		// update various GUI elements to reflect the new session state
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
			// this handler is invoked for changes to ANY session,
			// but the response is specific to one, so check first
			if (Session_ReturnActiveTerminalWindow(session) == terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				Session_StateAttributes			currentAttributes = Session_ReturnStateAttributes(session);
				
				
				// the scroll lock toolbar item, if visible, should have its icon changed
				if (ptr->toolbarItemScrollLock.exists())
				{
					if (currentAttributes & kSession_StateAttributeSuspendNetwork)
					{
						(OSStatus)HIToolbarItemSetIconRef(ptr->toolbarItemScrollLock.returnHIObjectRef(), gScrollLockOffIcon());
					}
					else
					{
						(OSStatus)HIToolbarItemSetIconRef(ptr->toolbarItemScrollLock.returnHIObjectRef(), gScrollLockOnIcon());
					}
				}
			}
		}
		break;
	
	case kSession_ChangeWindowTitle:
		// update the window based on the new session title
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
			// this handler is invoked for changes to ANY session,
			// but the response is specific to one, so check first
			if (Session_ReturnActiveTerminalWindow(session) == terminalWindow)
			{
				CFStringRef		titleCFString = nullptr;
				
				
				if (kSession_ResultOK == Session_GetWindowUserDefinedTitle(session, titleCFString))
				{
					TerminalWindow_SetWindowTitle(terminalWindow, titleCFString);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// sessionStateChanged


/*!
Based on the specified global mouse location and
event modifiers, sets the cursor appropriately
for the given window.

(3.1)
*/
OSStatus
setCursorInWindow	(WindowRef		inWindow,
					 Point			inGlobalMouse,
					 UInt32			inModifiers)
{
	OSStatus	result = noErr;
	HIViewRef	contentView = nullptr;
	
	
	// find content view (needed to translate coordinates)
	result = HIViewFindByID(HIViewGetRoot(inWindow), kHIViewWindowContentID, &contentView);
	if (noErr == result)
	{
		CGrafPtr	oldPort = nullptr;
		GDHandle	oldDevice = nullptr;
		Point		localMouse;
		HIPoint		localMouseHIPoint;
		HIViewRef	viewUnderMouse = nullptr;
		
		
		GetGWorld(&oldPort, &oldDevice);
		SetPortWindowPort(inWindow);
		localMouse = inGlobalMouse;
		GlobalToLocal(&localMouse);
		localMouseHIPoint = CGPointMake(localMouse.h, localMouse.v);
		
		// figure out what view is under the specified point
		result = HIViewGetSubviewHit(contentView, &localMouseHIPoint, true/* deepest */, &viewUnderMouse);
		if ((noErr != result) || (nullptr == viewUnderMouse))
		{
			// nothing underneath the mouse, or some problem; restore the arrow and claim all is well
			(SInt16)Cursors_UseArrow();
			result = noErr;
		}
		else
		{
			static WindowRef	gPreviousWindow = nullptr;
			static HIViewRef	gPreviousHIView = nullptr;
			static UInt32		gPreviousModifiers = 0;
			ControlKind			controlKind;
			Boolean				doSet = false;
			Boolean				wasSet = false;
			
			
			result = GetControlKind(viewUnderMouse, &controlKind);
			if ((AppResources_ReturnCreatorCode() == controlKind.signature) &&
				(kConstantsRegistry_ControlKindTerminalView == controlKind.kind))
			{
				// for terminal views, it is possible to determine whether or not
				// there is text selected
				TerminalViewRef		view = nullptr;
				UInt32				actualSize = 0;
				
				
				result = GetControlProperty(viewUnderMouse, AppResources_ReturnCreatorCode(),
											kConstantsRegistry_ControlPropertyTypeTerminalViewRef,
											sizeof(view), &actualSize, &view);
				if (noErr == result)
				{
					if (TerminalView_TextSelectionExists(view))
					{
						// when text is selected, the cursor varies within the view;
						// take the slower approach and constantly check the cursor
						doSet = true;
					}
					else
					{
						// when no text is selected, the cursor will always have the
						// same shape within the view (varying only by modifiers);
						// take the efficient approach and check only when the
						// view under the mouse is changed
						doSet = ((gPreviousWindow != inWindow) || (gPreviousHIView != viewUnderMouse) ||
									(gPreviousModifiers != inModifiers));
					}
				}							
			}
			else
			{
				// unknown control type - set only as needed
				doSet = ((gPreviousWindow != inWindow) || (gPreviousHIView != viewUnderMouse) ||
							(gPreviousModifiers != inModifiers));
			}
			
			// to avoid wasting time, do not even tell the control about the event
			// unless (as determined above) this seems necessary
			if (doSet)
			{
				// set the cursor appropriately in whatever control is under the mouse
				result = HandleControlSetCursor(viewUnderMouse, localMouse, inModifiers, &wasSet);
				if (noErr != result)
				{
					// some problem; restore the arrow and claim all is well
					(SInt16)Cursors_UseArrow();
					result = noErr;
				}
				
				// update cache for next time
				gPreviousWindow = inWindow;
				gPreviousHIView = viewUnderMouse;
				gPreviousModifiers = inModifiers;
			}
			else
			{
				// no action; assume the arrow should be restored
				(SInt16)Cursors_UseArrow();
			}
		}
		SetGWorld(oldPort, oldDevice);
	}
	
	return result;
}// setCursorInWindow


/*!
Copies the screen size and scrollback settings from the given
context to the underlying terminal buffer.  The main view is
updated to the size that will show the new dimensions entirely
at the current font size.  The window size changes to fit the
new screen.

See also TerminalWindow_SetScreenDimensions().

(4.0)
*/
void
setScreenPreferences	(My_TerminalWindowPtr		inPtr,
						 Preferences_ContextRef		inContext)
{
	TerminalScreenRef		activeScreen = getActiveScreen(inPtr);
	
	
	if (nullptr != activeScreen)
	{
		Preferences_TagSetRef	tagSet = PrefPanelTerminals_NewScreenPaneTagSet();
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextCopy(inContext, Terminal_ReturnConfiguration(activeScreen), tagSet);
		Preferences_ReleaseTagSet(&tagSet);
		
		// IMPORTANT: this window adjustment should match TerminalWindow_SetScreenDimensions()
		unless (inPtr->viewSizeIndependent)
		{
			UInt16		columns = 0;
			UInt16		rows = 0;
			
			
			prefsResult = Preferences_ContextGetData(inContext, kPreferences_TagTerminalScreenColumns,
														sizeof(columns), &columns);
			if (kPreferences_ResultOK == prefsResult)
			{
				prefsResult = Preferences_ContextGetData(inContext, kPreferences_TagTerminalScreenRows,
															sizeof(rows), &rows);
				if (kPreferences_ResultOK == prefsResult)
				{
					Terminal_SetVisibleScreenDimensions(activeScreen, columns, rows);
					setWindowToIdealSizeForDimensions(inPtr, columns, rows);
				}
			}
		}
	}
}// setScreenPreferences


/*!
Sets the standard state (for zooming) of the given
terminal window to match the size required to fit
the specified width and height in pixels.

Once this is done, you can make the window this
size by zooming “out”, or by passing "true" for
"inResizeWindow".

(3.0)
*/
void
setStandardState	(My_TerminalWindowPtr	inPtr,
					 UInt16					inScreenWidthInPixels,
					 UInt16					inScreenHeightInPixels,
					 Boolean				inResizeWindow)
{
	SInt16		windowWidth = 0;
	SInt16		windowHeight = 0;
	
	
	getWindowSizeFromViewSize(inPtr, inScreenWidthInPixels, inScreenHeightInPixels, &windowWidth, &windowHeight);
	(OSStatus)inPtr->windowResizeHandler.setWindowIdealSize(windowWidth, windowHeight);
	{
		Rect		bounds;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(returnCarbonWindow(inPtr), kWindowContentRgn, &bounds);
		assert_noerr(error);
		// force the current size regardless (in reality, the event handlers
		// will be consulted so that the window size is constrained); but
		// resize at the same time, if that is applicable
		if (inResizeWindow)
		{
			bounds.right = bounds.left + windowWidth;
			bounds.bottom = bounds.top + windowHeight;
		}
		error = SetWindowBounds(returnCarbonWindow(inPtr), kWindowContentRgn, &bounds);
		assert_noerr(error);
	}
}// setStandardState


/*!
Copies the format settings (like font and colors) from the given
context to every view in the window.  The window size changes to
fit any new font.

See also TerminalWindow_SetFontAndSize().

(4.0)
*/
void
setViewFormatPreferences	(My_TerminalWindowPtr		inPtr,
							 Preferences_ContextRef		inContext)
{
	if (false == inPtr->allViews.empty())
	{
		TerminalViewRef				activeView = getActiveView(inPtr);
		TerminalView_DisplayMode	oldMode = kTerminalView_DisplayModeNormal;
		TerminalView_Result			viewResult = kTerminalView_ResultOK;
		Preferences_TagSetRef		tagSet = PrefPanelFormats_NewTagSet();
		Preferences_Result			prefsResult = kPreferences_ResultOK;
		
		
		// TEMPORARY; should iterate over other views if there is ever more than one
		oldMode = TerminalView_ReturnDisplayMode(activeView);
		viewResult = TerminalView_SetDisplayMode(activeView, kTerminalView_DisplayModeNormal);
		assert(kTerminalView_ResultOK == viewResult);
		prefsResult = Preferences_ContextCopy(inContext, TerminalView_ReturnFormatConfiguration(activeView), tagSet);
		viewResult = TerminalView_SetDisplayMode(activeView, oldMode);
		assert(kTerminalView_ResultOK == viewResult);
		
		Preferences_ReleaseTagSet(&tagSet);
		
		// IMPORTANT: this window adjustment should match TerminalWindow_SetFontAndSize()
		unless (inPtr->viewSizeIndependent)
		{
			setWindowToIdealSizeForFont(inPtr);
		}
	}
}// setViewFormatPreferences


/*!
This internal state changes in certain special situations
(such as during the transition to full-screen mode) to
temporarily prevent view dimension or font size changes
from triggering “ideal” window resizes to match those
changes.  Normally, the flag should be true.

This is useful for full-screen mode because the window
should be flush to an exact size (that of the display),
whereas in most cases it is better to make the window no
bigger than it needs to be.

(4.0)
*/
void
setViewSizeIndependentFromWindow	(My_TerminalWindowPtr	inPtr,
									 Boolean				inWindowResizesWhenViewSizeChanges)
{
	inPtr->viewSizeIndependent = inWindowResizesWhenViewSizeChanges;
}// setViewSizeIndependentFromWindow


/*!
Copies the translation settings (like the character set) from
the given context to every view in the window.

WARNING:	This is done internally to propagate settings to all
			the right places beneath a window, but this is not a
			good entry point for changing translation settings!
			Copy changes into a Session-level configuration, as
			returned by Session_ReturnTranslationConfiguration(),
			so that the Session is always aware of them.

(4.0)
*/
void
setViewTranslationPreferences	(My_TerminalWindowPtr		inPtr,
								 Preferences_ContextRef		inContext)
{
	if (false == inPtr->allViews.empty())
	{
		TerminalViewRef				activeView = getActiveView(inPtr);
		Preferences_TagSetRef		tagSet = PrefPanelTranslations_NewTagSet();
		Preferences_Result			prefsResult = kPreferences_ResultOK;
		
		
		// TEMPORARY; should iterate over other views if there is ever more than one
		prefsResult = Preferences_ContextCopy(inContext, TerminalView_ReturnTranslationConfiguration(activeView), tagSet);
		
		Preferences_ReleaseTagSet(&tagSet);
	}
}// setViewTranslationPreferences


/*!
Adorns or strips a window frame indicator showing
that a warning message will appear if the user
tries to close the window.  On Mac OS 8/9, this
disables any window proxy icon; on Mac OS X, a dot
also appears in the close box.

NOTE:	This does NOT force a warning message to
		appear, this call is nothing more than an
		adornment.  Making the window’s behavior
		consistent with the adornment is up to you.

(3.0)
*/
void
setWarningOnWindowClose		(My_TerminalWindowPtr	inPtr,
							 Boolean				inCloseBoxHasDot)
{
	if (nil != inPtr->window)
	{
		// attach or remove an adornment in the window that shows
		// that attempting to close it will display a warning;
		// on Mac OS X, a dot appears in the middle of the close box
		(OSStatus)SetWindowModified(returnCarbonWindow(inPtr), inCloseBoxHasDot);
	}
}// setWarningOnWindowClose


/*!
Changes the title of the terminal window, and any tab
associated with it, to the specified string.

See also TerminalWindow_SetWindowTitle().

(4.0)
*/
void
setWindowAndTabTitle	(My_TerminalWindowPtr	inPtr,
						 CFStringRef			inNewTitle)
{
	AutoPool	_;
	
	
	[inPtr->window setTitle:(NSString*)inNewTitle];
	if (inPtr->tab.exists())
	{
		HIViewWrap			titleWrap(idMyLabelTabTitle,
										REINTERPRET_CAST(inPtr->tab.returnHIObjectRef(), HIWindowRef));
		HMHelpContentRec	helpTag;
		
		
		// set text
		SetControlTextWithCFString(titleWrap, inNewTitle);
		
		// set help tag, to show full title on hover
		// in case it is too long to display
		bzero(&helpTag, sizeof(helpTag));
		helpTag.version = kMacHelpVersion;
		helpTag.tagSide = kHMOutsideBottomCenterAligned;
		helpTag.content[0].contentType = kHMCFStringContent;
		helpTag.content[0].u.tagCFString = inNewTitle;
		helpTag.content[1].contentType = kHMCFStringContent;
		helpTag.content[1].u.tagCFString = CFSTR("");
		OSStatus error = noErr;
		//(OSStatus)HMSetControlHelpContent(titleWrap, &helpTag);
		error = HMSetControlHelpContent(titleWrap, &helpTag);
		assert_noerr(error);
	}
}// setWindowAndTabTitle


/*!
Resizes the window so that its main view is large enough for
the specified number of columns and rows at the current font
size.  Split-pane views are removed.

(4.0)
*/
void
setWindowToIdealSizeForDimensions	(My_TerminalWindowPtr	inPtr,
									 UInt16					inColumns,
									 UInt16					inRows)
{
	if (false == inPtr->allViews.empty())
	{
		TerminalViewRef		activeView = getActiveView(inPtr);
		SInt16				screenWidth = 0;
		SInt16				screenHeight = 0;
		
		
		TerminalView_GetTheoreticalViewSize(activeView/* TEMPORARY - must consider a list of views */,
											inColumns, inRows, &screenWidth, &screenHeight);
		setStandardState(inPtr, screenWidth, screenHeight, true/* resize window */);
	}
}// setWindowToIdealSizeForDimensions


/*!
Resizes the window so that its main view is large enough to
render the current number of columns and rows using the font
and font size of the view.

(4.0)
*/
void
setWindowToIdealSizeForFont		(My_TerminalWindowPtr	inPtr)
{
	if (false == inPtr->allViews.empty())
	{
		TerminalViewRef		activeView = getActiveView(inPtr);
		SInt16				screenWidth = 0;
		SInt16				screenHeight = 0;
		
		
		TerminalView_GetIdealSize(activeView/* TEMPORARY - must consider a list of views */,
									screenWidth, screenHeight);
		setStandardState(inPtr, screenWidth, screenHeight, true/* resize window */);
	}
}// setWindowToIdealSizeForFont


/*!
Responds to a close of any sheet on a Terminal Window that
is updating a context constructed by sheetContextBegin().

This calls sheetContextEnd() to ensure that the context is
cleaned up.

(4.0)
*/
void
sheetClosed		(GenericDialog_Ref		inDialogThatClosed,
				 Boolean				inOKButtonPressed)
{
	TerminalWindowRef				ref = TerminalWindow_ReturnFromWindow
											(GenericDialog_ReturnParentWindow(inDialogThatClosed));
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), ref);
	
	
	if (nullptr == ptr)
	{
		Console_Warning(Console_WriteLine, "unexpected problem finding Terminal Window that corresponds to a closed sheet");
	}
	else
	{
		if (inOKButtonPressed)
		{
			switch (ptr->sheetType)
			{
			case kMy_SheetTypeFormat:
				setViewFormatPreferences(ptr, ptr->recentSheetContext.returnRef());
				break;
			
			case kMy_SheetTypeScreenSize:
				setScreenPreferences(ptr, ptr->recentSheetContext.returnRef());
				break;
			
			case kMy_SheetTypeTranslation:
				setViewTranslationPreferences(ptr, ptr->recentSheetContext.returnRef());
				break;
			
			default:
				Console_Warning(Console_WriteLine, "no active sheet but sheet context still exists and was changed");
				break;
			}
		}
		sheetContextEnd(ptr);
	}
}// sheetClosed


/*!
Constructs a new sheet context and starts monitoring it for
changes.  The given sheet type determines what the response
will be when settings are dumped into the target context.

The returned context is stored as "recentSheetContext" in the
specified window structure, and is nullptr if there was any
error.

(4.0)
*/
Preferences_ContextRef
sheetContextBegin	(My_TerminalWindowPtr	inPtr,
					 Quills::Prefs::Class	inClass,
					 My_SheetType			inSheetType)
{
	Preferences_ContextWrap		newContext(Preferences_NewContext(inClass), true/* is retained */);
	Preferences_ContextRef		result = nullptr;
	
	
	if (kMy_SheetTypeNone == inPtr->sheetType)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		Boolean					copyOK = false;
		
		
		// initialize settings so that the sheet has the right data
		// IMPORTANT: the contexts and tag sets chosen here should match those
		// used elsewhere in this file to update preferences later (that is,
		// in setScreenPreferences(), setViewFormatPreferences() and
		// setViewTranslationPreferences())
		switch (inSheetType)
		{
		case kMy_SheetTypeFormat:
			{
				Preferences_TagSetRef	tagSet = PrefPanelFormats_NewTagSet();
				
				
				prefsResult = Preferences_ContextCopy(TerminalView_ReturnFormatConfiguration(getActiveView(inPtr)),
														newContext.returnRef(), tagSet);
				if (kPreferences_ResultOK == prefsResult)
				{
					copyOK = true;
				}
				Preferences_ReleaseTagSet(&tagSet);
			}
			break;
		
		case kMy_SheetTypeScreenSize:
			{
				Preferences_TagSetRef	tagSet = PrefPanelTerminals_NewScreenPaneTagSet();
				
				
				prefsResult = Preferences_ContextCopy(Terminal_ReturnConfiguration(getActiveScreen(inPtr)),
														newContext.returnRef(), tagSet);
				if (kPreferences_ResultOK == prefsResult)
				{
					copyOK = true;
				}
				Preferences_ReleaseTagSet(&tagSet);
			}
			break;
		
		case kMy_SheetTypeTranslation:
			{
				Preferences_TagSetRef	tagSet = PrefPanelTranslations_NewTagSet();
				
				
				prefsResult = Preferences_ContextCopy(TerminalView_ReturnTranslationConfiguration(getActiveView(inPtr)),
														newContext.returnRef(), tagSet);
				if (kPreferences_ResultOK == prefsResult)
				{
					copyOK = true;
				}
				Preferences_ReleaseTagSet(&tagSet);
			}
			break;
		
		default:
			// ???
			break;
		}
		
		if (copyOK)
		{
			inPtr->sheetType = inSheetType;
			inPtr->recentSheetContext.setRef(newContext.returnRef()); // this also retains the new context
		}
		else
		{
			Console_Warning(Console_WriteLine, "failed to copy initial preferences into sheet context");
		}
	}
	
	result = inPtr->recentSheetContext.returnRef();
	
	return result;
}// sheetContextBegin


/*!
Destroys the temporary sheet preferences context, removing
the monitor on it, and clearing any flags that keep track of
active sheets.

(4.0)
*/
void
sheetContextEnd		(My_TerminalWindowPtr	inPtr)
{
	if (Preferences_ContextIsValid(inPtr->recentSheetContext.returnRef()))
	{
		inPtr->recentSheetContext.clear();
	}
	inPtr->sheetType = kMy_SheetTypeNone;
}// sheetContextEnd


/*!
This routine, of "SessionFactory_TerminalWindowOpProcPtr"
form, arranges the specified window in one of the “free
slots” for staggered terminal windows.

(3.0)
*/
void
stackWindowTerminalWindowOp		(TerminalWindowRef	inTerminalWindow,
								 void*				UNUSED_ARGUMENT(inData1),
								 SInt32				UNUSED_ARGUMENT(inData2),
								 void*				UNUSED_ARGUMENT(inoutResultPtr))
{
	AutoPool						_;
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
	
	
	if (nullptr != ptr)
	{
		GrafPtr		oldPort = nullptr;
		Point		windowLocation;
		WindowRef	window = nullptr;
		
		
		++gNumberOfTransitioningWindows; // this is reset elsewhere
		GetPort(&oldPort);
		window = TerminalWindow_ReturnWindow(inTerminalWindow);
		SetPortWindowPort(window);
		
		ensureTopLeftCornersExists();
		--((*gTopLeftCorners)[ptr->staggerPositionIndex]); // one less window at this position
		calculateIndexedWindowPosition(ptr, ptr->staggerPositionIndex, &windowLocation);
		
		// On Mac OS X, do a cool slide animation to move the window into place.
		{
			// arbitrary limit - do not animate many windows sequentially, that takes time
			Boolean		tooManyWindows = (gNumberOfTransitioningWindows > 6);
			
			
			if (FlagManager_Test(kFlagOS10_3API))
			{
				// on 10.3 and later, asynchronous transitions are used
				// so relax the restriction on number of animating windows
				// (in fact, the more that move, the cooler it looks!)
				tooManyWindows = (gNumberOfTransitioningWindows > 12/* arbitrary */);
			}
			
			if (tooManyWindows)
			{
				// move the window immediately without animation
				MoveWindow(window, windowLocation.h, windowLocation.v, false/* activate */);
			}
			else
			{
				// move the window with animation
				Rect		structureBounds;
				Rect		contentBounds;
				OSStatus	error = noErr;
				
				
				// TransitionWindow() requires the structure region, but the “location” is
				// that of the content region; so, it is necessary to figure out both what
				// the new structure origin will be, and the dimensions of that region.
				// This may fail if the window is not visible anymore.
				error = GetWindowBounds(window, kWindowContentRgn, &contentBounds);
				if (noErr == error)
				{
					error = GetWindowBounds(window, kWindowStructureRgn, &structureBounds);
					assert_noerr(error);
					SetRect(&structureBounds, windowLocation.h - (contentBounds.left - structureBounds.left),
							windowLocation.v - (contentBounds.top - structureBounds.top),
							windowLocation.h - (contentBounds.left - structureBounds.left) +
								(structureBounds.right - structureBounds.left),
							windowLocation.v - (contentBounds.top - structureBounds.top) +
								(structureBounds.bottom - structureBounds.top));
					if (FlagManager_Test(kFlagOS10_3API))
					{
						HIRect						floatBounds = CGRectMake(structureBounds.left, structureBounds.top,
																				structureBounds.right - structureBounds.left,
																				structureBounds.bottom - structureBounds.top);
						TransitionWindowOptions		transitionOptions;
						
						
						// transition asynchronously for minimum interruption to the user
						bzero(&transitionOptions, sizeof(transitionOptions));
						transitionOptions.version = 0;
						if (TransitionWindowWithOptions(window, kWindowSlideTransitionEffect, kWindowMoveTransitionAction,
														&floatBounds, true/* asynchronous */, &transitionOptions)
							!= noErr)
						{
							// on error, just move the window
							MoveWindow(window, windowLocation.h, windowLocation.v, false/* activate */);
						}
					}
					else
					{
						// on 10.2 and earlier, no special options are available
						if (TransitionWindow(window, kWindowSlideTransitionEffect, kWindowMoveTransitionAction, &structureBounds)
							!= noErr)
						{
							// on error, just move the window
							MoveWindow(window, windowLocation.h, windowLocation.v, false/* activate */);
						}
					}
				}
			}
		}
		SetPort(oldPort);
	}
}// stackWindowTerminalWindowOp


/*!
Invoked whenever a monitored terminal state is changed
(see TerminalWindow_New() to see which states are monitored).
This routine responds by updating terminal windows
appropriately.

(3.0)
*/
void
terminalStateChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inTerminalSettingThatChanged,
						 void*					inEventContextPtr,
						 void*					inListenerContextPtr)
{
	switch (inTerminalSettingThatChanged)
	{
	case kTerminal_ChangeAudioState:
		// update the bell toolbar item based on the bell being enabled or disabled
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				HIToolbarItemRef				bellItem = nullptr;
				OSStatus						error = noErr;
				
				
				bellItem = REINTERPRET_CAST(ptr->toolbarItemBell.returnHIObjectRef(), HIToolbarItemRef);
				if (nullptr != bellItem)
				{
					error = HIToolbarItemSetIconRef(bellItem, (Terminal_BellIsEnabled(screen)) ? gBellOffIcon() : gBellOnIcon());
					assert_noerr(error);
				}
			}
		}
		break;
	
	case kTerminal_ChangeExcessiveErrors:
		// the terminal has finally had enough, having seen a ridiculous
		// number of data errors; report this to the user
		{
			//TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				AlertMessages_BoxRef			warningBox = Alert_NewWindowModal(TerminalWindow_ReturnWindow(terminalWindow),
																					false/* is close warning */, Alert_StandardCloseNotifyProc,
																					nullptr/* context */);
				UIStrings_Result				stringResult = kUIStrings_ResultOK;
				CFStringRef						dialogTextCFString = nullptr;
				CFStringRef						helpTextCFString = nullptr;
				
				
				Alert_SetParamsFor(warningBox, kAlert_StyleOK);
				Alert_SetType(warningBox, kAlertNoteAlert);
				
				stringResult = UIStrings_Copy(kUIStrings_AlertWindowExcessiveErrorsPrimaryText, dialogTextCFString);
				assert(stringResult.ok());
				stringResult = UIStrings_Copy(kUIStrings_AlertWindowExcessiveErrorsHelpText, helpTextCFString);
				assert(stringResult.ok());
				Alert_SetTextCFStrings(warningBox, dialogTextCFString, helpTextCFString);
				
				// show the message; it is disposed asynchronously
				Alert_Display(warningBox);
				
				if (nullptr != dialogTextCFString)
				{
					CFRelease(dialogTextCFString), dialogTextCFString = nullptr;
				}
				if (nullptr != helpTextCFString)
				{
					CFRelease(helpTextCFString), helpTextCFString = nullptr;
				}
			}
		}
		break;
	
	case kTerminal_ChangeNewLEDState:
		// find the new LED state(s)
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if ((nullptr != screen) && (nullptr != terminalWindow))
			{
				// find the 4 terminal LED states; update internal state, and the toolbar
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				HIToolbarItemRef				relevantItem = nullptr;
				UInt16							i = 0;
				OSStatus						error = noErr;
				
				
				ptr->isLEDOn[i] = Terminal_LEDIsOn(screen, i + 1/* LED # */);
				relevantItem = REINTERPRET_CAST(ptr->toolbarItemLED1.returnHIObjectRef(), HIToolbarItemRef);
				if (nullptr != relevantItem)
				{
					error = HIToolbarItemSetIconRef(relevantItem, (ptr->isLEDOn[i]) ? gLEDOnIcon() : gLEDOffIcon());
					assert_noerr(error);
				}
				++i;
				
				ptr->isLEDOn[i] = Terminal_LEDIsOn(screen, i + 1/* LED # */);
				relevantItem = REINTERPRET_CAST(ptr->toolbarItemLED2.returnHIObjectRef(), HIToolbarItemRef);
				if (nullptr != relevantItem)
				{
					error = HIToolbarItemSetIconRef(relevantItem, (ptr->isLEDOn[i]) ? gLEDOnIcon() : gLEDOffIcon());
					assert_noerr(error);
				}
				++i;
				
				ptr->isLEDOn[i] = Terminal_LEDIsOn(screen, i + 1/* LED # */);
				relevantItem = REINTERPRET_CAST(ptr->toolbarItemLED3.returnHIObjectRef(), HIToolbarItemRef);
				if (nullptr != relevantItem)
				{
					error = HIToolbarItemSetIconRef(relevantItem, (ptr->isLEDOn[i]) ? gLEDOnIcon() : gLEDOffIcon());
					assert_noerr(error);
				}
				++i;
				
				ptr->isLEDOn[i] = Terminal_LEDIsOn(screen, i + 1/* LED # */);
				relevantItem = REINTERPRET_CAST(ptr->toolbarItemLED4.returnHIObjectRef(), HIToolbarItemRef);
				if (nullptr != relevantItem)
				{
					error = HIToolbarItemSetIconRef(relevantItem, (ptr->isLEDOn[i]) ? gLEDOnIcon() : gLEDOffIcon());
					assert_noerr(error);
				}
				++i;
			}
		}
		break;
	
	case kTerminal_ChangeScrollActivity:
		// recalculate appearance of the scroll bars to match current screen attributes, and redraw them
		{
			//Terminal_ScrollDescriptionConstPtr		scrollInfoPtr = REINTERPRET_CAST(inEventContextPtr, Terminal_ScrollDescriptionConstPtr); // not needed
			TerminalWindowRef					terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				
				
				updateScrollBars(ptr);
			}
		}
		break;
	
	case kTerminal_ChangeWindowFrameTitle:
		// set window’s title to match
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				CFStringRef						titleCFString = nullptr;
				
				
				Terminal_CopyTitleForWindow(screen, titleCFString);
				if (nullptr != titleCFString)
				{
					TerminalWindow_SetWindowTitle(ptr->selfRef, titleCFString);
					CFRelease(titleCFString), titleCFString = nullptr;
				}
			}
		}
		break;
	
	case kTerminal_ChangeWindowIconTitle:
		// set window’s alternate (Dock icon) title to match
		{
			AutoPool			_;
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				CFStringRef						titleCFString = nullptr;
				
				
				Terminal_CopyTitleForIcon(screen, titleCFString);
				if (nullptr != titleCFString)
				{
					// TEMPORARY - Cocoa wrapper window does not seem to recognize setMiniwindowTitle:,
					// so the Carbon call is also used in the meantime
					(OSStatus)SetWindowAlternateTitle(returnCarbonWindow(ptr), titleCFString);
					
					[ptr->window setMiniwindowTitle:(NSString*)titleCFString];
					
					CFRelease(titleCFString), titleCFString = nullptr;
				}
			}
		}
		break;
	
	case kTerminal_ChangeWindowMinimization:
		// minimize or restore window based on requested minimization
		{
			AutoPool			_;
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				
				
				if (Terminal_WindowIsToBeMinimized(screen))
				{
					[ptr->window miniaturize:nil];
				}
				else
				{
					[ptr->window deminiaturize:nil];
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// terminalStateChanged


/*!
Invoked whenever a monitored terminal view event occurs (see
TerminalWindow_New() to see which events are monitored).
This routine responds by updating terminal windows appropriately.

(3.1)
*/
void
terminalViewStateChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inTerminalViewEvent,
							 void*					inEventContextPtr,
							 void*					inListenerContextPtr)
{
	TerminalWindowRef				ref = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), ref);
	
	
	// currently, only one type of event is expected
	assert((inTerminalViewEvent == kTerminalView_EventScrolling) ||
			(inTerminalViewEvent == kTerminalView_EventSearchResultsExistence));
	
	switch (inTerminalViewEvent)
	{
	case kTerminalView_EventScrolling:
		// recalculate appearance of the scroll bars to match current screen attributes, and redraw them
		{
			//TerminalViewRef	view = REINTERPRET_CAST(inEventContextPtr, TerminalViewRef); // not needed
			
			
			updateScrollBars(ptr);
			(OSStatus)HIViewSetNeedsDisplay(ptr->controls.scrollBarH, true);
			(OSStatus)HIViewSetNeedsDisplay(ptr->controls.scrollBarV, true);
		}
		break;
	
	case kTerminalView_EventSearchResultsExistence:
		// search results either appeared or disappeared; ensure the scroll bar
		// rendering is only installed when it is needed
		{
			TerminalViewRef		view = REINTERPRET_CAST(inEventContextPtr, TerminalViewRef);
			
			
			if (TerminalView_SearchResultsExist(view))
			{
				installTickHandler(ptr);
			}
			else
			{
				ptr->scrollTickHandler.remove();
				assert(false == ptr->scrollTickHandler.isInstalled());
			}
			(OSStatus)HIViewSetNeedsDisplay(ptr->controls.scrollBarH, true);
			(OSStatus)HIViewSetNeedsDisplay(ptr->controls.scrollBarV, true);
		}
		break;
	
	default:
		// ???
		break;
	}
}// terminalViewStateChanged


/*!
Updates the values and view sizes of the scroll bars
to show the position and percentage of the total
screen area that is currently visible in the window.

(3.0)
*/
void
updateScrollBars	(My_TerminalWindowPtr	inPtr)
{
	if (false == inPtr->allViews.empty())
	{
		// update the scroll bars to reflect the contents of the selected view
		TerminalViewRef			view = getActiveView(inPtr);
		HIViewRef				scrollBarView = nullptr;
		SInt32					scrollVStartView = 0;
		SInt32					scrollVPastEndView = 0;
		SInt32					scrollVRangeMinimum = 0;
		SInt32					scrollVRangePastMaximum = 0;
		TerminalView_Result		rangeResult = kTerminalView_ResultOK;
		
		
		// use the maximum possible screen size for the maximum resize limits
		rangeResult = TerminalView_GetScrollVerticalInfo(view, scrollVStartView, scrollVPastEndView,
															scrollVRangeMinimum, scrollVRangePastMaximum);
		assert(kTerminalView_ResultOK == rangeResult);
		
		// update controls’ maximum and minimum values; the vertical scroll bar
		// is special, in that its maximum value is zero (this ensures the main
		// area is pinned in view even if new scrollback rows show up, etc.)
		scrollBarView = inPtr->controls.scrollBarH;
		SetControl32BitMinimum(scrollBarView, 0);
		SetControl32BitMaximum(scrollBarView, 0);
		SetControl32BitValue(scrollBarView, 0);
		SetControlViewSize(scrollBarView, 0);
		scrollBarView = inPtr->controls.scrollBarV;
		SetControl32BitMinimum(scrollBarView, scrollVRangeMinimum);
		SetControl32BitMaximum(scrollBarView, scrollVRangePastMaximum - (scrollVPastEndView - scrollVStartView)/* subtract last page */);
		SetControl32BitValue(scrollBarView, scrollVStartView);
		
		// set the size of the scroll thumb, but refuse to make it as ridiculously
		// tiny as Apple allows; artificially require it to be bigger
		{
			UInt32		proposedViewSize = scrollVPastEndView - scrollVStartView;
			HIRect		scrollBarBounds;
			Float64		viewDenominator = (STATIC_CAST(GetControl32BitMaximum(scrollBarView), Float32) -
											STATIC_CAST(GetControl32BitMinimum(scrollBarView), Float32));
			Float64		viewScale = STATIC_CAST(proposedViewSize, Float32) / viewDenominator;
			Float64		barScale = 0;
			
			
			(OSStatus)HIViewGetBounds(scrollBarView, &scrollBarBounds);
			
			// adjust the numerator to require a larger minimum size for the thumb
			barScale = kMy_ScrollBarThumbMinimumSize / (scrollBarBounds.size.height - 2 * kMy_ScrollBarArrowHeight);
			if (viewScale < barScale)
			{
				proposedViewSize = barScale * viewDenominator;
			}
			
			SetControlViewSize(scrollBarView, proposedViewSize);
		}
		
		(OSStatus)HIViewSetNeedsDisplay(inPtr->controls.scrollBarV, true);
		(OSStatus)HIViewSetNeedsDisplay(inPtr->controls.scrollBarH, true);
	}
}// updateScrollBars

} // anonymous namespace


@implementation TerminalWindow_Controller


static TerminalWindow_Controller*	gTerminalWindow_Controller = nil;


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedTerminalWindowController
{
	if (nil == gTerminalWindow_Controller)
	{
		gTerminalWindow_Controller = [[[self class] allocWithZone:NULL] init];
	}
	return gTerminalWindow_Controller;
}// sharedTerminalWindowController


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"TerminalWindowCocoa"];
	return self;
}// init


#pragma mark NSToolbarDelegate


/*!
Responds when the specified kind of toolbar item should be
constructed for the given toolbar.

(4.0)
*/
- (NSToolbarItem*)
toolbar:(NSToolbar*)				toolbar
itemForItemIdentifier:(NSString*)	itemIdentifier
willBeInsertedIntoToolbar:(BOOL)	flag
{
#pragma unused(toolbar, flag)
	NSToolbarItem*		result = nil;
	
	
	// NOTE: no need to handle standard items here
	// TEMPORARY - need to create all custom items
	if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDCustomize])
	{
		result = [[[TerminalWindow_ToolbarItemCustomize alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED1])
	{
		result = [[[TerminalWindow_ToolbarItemLED1 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED2])
	{
		result = [[[TerminalWindow_ToolbarItemLED2 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED3])
	{
		result = [[[TerminalWindow_ToolbarItemLED3 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED4])
	{
		result = [[[TerminalWindow_ToolbarItemLED4 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDPrint])
	{
		result = [[[TerminalWindow_ToolbarItemPrint alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDTabs])
	{
		result = [[[TerminalWindow_ToolbarItemTabs alloc] init] autorelease];
	}
	return result;
}// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:


/*!
Returns the identifiers for the kinds of items that can appear
in the given toolbar.

(4.0)
*/
- (NSArray*)
toolbarAllowedItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	return [NSArray arrayWithObjects:
						kMy_ToolbarItemIDTabs,
						NSToolbarSpaceItemIdentifier,
						NSToolbarFlexibleSpaceItemIdentifier,
						kMy_ToolbarItemIDCustomize,
						kMy_ToolbarItemIDLED1,
						kMy_ToolbarItemIDLED2,
						kMy_ToolbarItemIDLED3,
						kMy_ToolbarItemIDLED4,
						kMy_ToolbarItemIDPrint,
						nil];
}// toolbarAllowedItemIdentifiers


/*!
Returns the identifiers for the items that will appear in the
given toolbar whenever the user has not customized it.

(4.0)
*/
- (NSArray*)
toolbarDefaultItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	return [NSArray arrayWithObjects:
						kMy_ToolbarItemIDTabs,
						NSToolbarFlexibleSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						kMy_ToolbarItemIDLED1,
						kMy_ToolbarItemIDLED2,
						kMy_ToolbarItemIDLED3,
						kMy_ToolbarItemIDLED4,
						NSToolbarSpaceItemIdentifier,
						kMy_ToolbarItemIDPrint,
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						nil];
}// toolbarDefaultItemIdentifiers


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
windowDidLoad
{
	assert(nil != testTerminalContentView);
	assert(nil != testTerminalPaddingView);
	assert(nil != testTerminalBackgroundView);
	
	Preferences_ContextWrap		terminalConfig(Preferences_NewContext(Quills::Prefs::TERMINAL), true/* is retained */);
	Preferences_ContextWrap		translationConfig(Preferences_NewContext(Quills::Prefs::TRANSLATION), true/* is retained */);
	
	
	@try
	{
		// create toolbar; has to be done programmatically, because
		// IB only supports them in 10.5; which makes sense, you know,
		// since toolbars have only been in the OS since 10.0, and
		// hardly any applications would have found THOSE useful...
		{
			NSString*		toolbarID = @"TerminalToolbar"; // do not ever change this; that would only break user preferences
			NSToolbar*		windowToolbar = [[[NSToolbar alloc] initWithIdentifier:toolbarID] autorelease];
			
			
			[windowToolbar setAllowsUserCustomization:YES];
			[windowToolbar setAutosavesConfiguration:YES];
			[windowToolbar setDelegate:self];
			[[self window] setToolbar:windowToolbar];
		}
		
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
				TerminalViewRef		view = TerminalView_NewNSViewBased(testTerminalContentView, testTerminalPaddingView,
																		testTerminalBackgroundView, buffer, nullptr/* format */);
				
				
				if (nullptr == view)
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
													"sel norm \033[1mbold\033[0m \033[5mblink\033[0m \033[3mital\033[0m \033[7minv\033[0m \033[4munder\033[0m");
					// the range selected here should be as long as the length of the word “sel” above
					TerminalView_SelectVirtualRange(view, std::make_pair(std::make_pair(0, 0), std::make_pair(3, 1)/* exclusive end */));
				}
			}
		}
	}
	@finally
	{
		terminalConfig.clear();
		translationConfig.clear();
	}
}// windowDidLoad


@end // TerminalWindow_Controller


@implementation TerminalWindow_ToolbarItemCustomize


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	// WARNING: The Customize item is currently redundantly implemented in the Terminal Window module;
	// this is TEMPORARY, but both implementations should agree.
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDCustomize];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnCustomizeToolbarIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Customize", @"toolbar item name; for customizing the toolbar")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	// TEMPORARY; only doing it this way during Carbon/Cocoa transition
	[[Commands_Executor sharedExecutor] runToolbarCustomizationPaletteSetup:NSApp];
}// performToolbarItemAction:


@end // TerminalWindow_ToolbarItemCustomize


@implementation TerminalWindow_ToolbarItemLED1


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED1];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L1", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED1);
}// performToolbarItemAction:


@end // TerminalWindow_ToolbarItemLED1


@implementation TerminalWindow_ToolbarItemLED2


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED2];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L2", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED2);
}// performToolbarItemAction:


@end // TerminalWindow_ToolbarItemLED2


@implementation TerminalWindow_ToolbarItemLED3


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED3];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L3", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED3);
}// performToolbarItemAction:


@end // TerminalWindow_ToolbarItemLED3


@implementation TerminalWindow_ToolbarItemLED4


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED4];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L4", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED4);
}// performToolbarItemAction:


@end // TerminalWindow_ToolbarItemLED4


@implementation TerminalWindow_ToolbarItemPrint


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDPrint];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnPrintIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Print", @"toolbar item name; for printing")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandPrint);
}// performToolbarItemAction:


@end // TerminalWindow_ToolbarItemPrint


@implementation TerminalWindow_TabSource


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithDescription:(NSAttributedString*)	aDescription
{
	self = [super init];
	if (nil != self)
	{
		self->description = [aDescription copy];
	}
	return self;
}// initWithDescription:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[description release];
	[super dealloc];
}// dealloc


/*!
The attributed string describing the title of the tab; this
is used in the segmented control as well as any overflow menu.

(4.0)
*/
- (NSAttributedString*)
attributedDescription
{
	return description;
}// attributedDescription


/*!
The tooltip that is displayed when the mouse points to the
tab’s segment in the toolbar.

(4.0)
*/
- (NSString*)
toolTip
{
	return @"";
}// toolTip


/*!
Performs the action for this tab (namely, to bring something
to the front).  The sender will be an instance of the
"TerminalWindow_ToolbarItemTabs" class.

(4.0)
*/
- (void)
performAction:(id) sender
{
#pragma unused(sender)
}// performAction:


@end // TerminalWindow_TabSource


@implementation TerminalWindow_ToolbarItemTabs


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDTabs];
	if (nil != self)
	{
		self->segmentedControl = [[NSSegmentedControl alloc] initWithFrame:NSZeroRect];
		[self->segmentedControl setTarget:self];
		[self->segmentedControl setAction:@selector(performSegmentedControlAction:)];
		if (FlagManager_Test(kFlagOS10_5API))
		{
			if ([self->segmentedControl respondsToSelector:@selector(setSegmentStyle:)])
			{
				[self->segmentedControl setSegmentStyle:FUTURE_SYMBOL(4, NSSegmentStyleTexturedSquare)];
			}
		}
		
		//[self setAction:@selector(performToolbarItemAction:)];
		//[self setTarget:self];
		[self setEnabled:YES];
		[self setView:self->segmentedControl];
		[self setMinSize:NSMakeSize(120, 25)]; // arbitrary
		[self setMaxSize:NSMakeSize(1024, 25)]; // arbitrary
		[self setLabel:@""];
		[self setPaletteLabel:NSLocalizedString(@"Tabs", @"toolbar item name; for tabs")];
		
		[self setTabTargets:[NSArray arrayWithObjects:
										[[[TerminalWindow_TabSource alloc]
											initWithDescription:[[[NSAttributedString alloc]
																	initWithString:NSLocalizedString
																					(@"Tab 1", @"toolbar item tabs; default segment 0 name")]
																	autorelease]] autorelease],
										[[[TerminalWindow_TabSource alloc]
											initWithDescription:[[[NSAttributedString alloc]
																	initWithString:NSLocalizedString
																					(@"Tab 2", @"toolbar item tabs; default segment 1 name")]
																	autorelease]] autorelease],
										nil]
				andAction:nil];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self->segmentedControl release];
	[self->targets release];
	[super dealloc];
}// dealloc


/*!
Responds to an action in the menu of a toolbar item by finding
the corresponding object in the target array and making it
perform the action set by "setTabTargets:andAction:".

(4.0)
*/
- (void)
performMenuAction:(id)		sender
{
	if (nullptr != self->action)
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		NSMenu*			menu = [asMenuItem menu];
		
		
		if (nil != menu)
		{
			unsigned int const		kIndex = STATIC_CAST([menu indexOfItem:asMenuItem], unsigned int);
			
			
			if (kIndex < [self->targets count])
			{
				(id)[[self->targets objectAtIndex:kIndex] performSelector:self->action withObject:self];
			}
		}
	}
}// performMenuAction:


/*!
Responds to an action in a segmented control by finding the
corresponding object in the target array and making it perform
the action set by "setTabTargets:andAction:".

(4.0)
*/
- (void)
performSegmentedControlAction:(id)		sender
{
	if (nullptr != self->action)
	{
		if (self->segmentedControl == sender)
		{
			unsigned int const		kIndex = STATIC_CAST([self->segmentedControl selectedSegment], unsigned int);
			
			
			if (kIndex < [self->targets count])
			{
				(id)[[self->targets objectAtIndex:kIndex] performSelector:self->action withObject:self];
			}
		}
	}
}// performSegmentedControlAction:


/*!
Specifies the model on which the tab display is based.

When any tab is selected the specified selector is invoked
on one of the given objects, with this toolbar item as the
sender.  This is true no matter how the item is found: via
segmented control or overflow menu.

Each object:
- MUST respond to an "attributedDescription" selector that
  returns an "NSAttributedString*" for that tab’s label.
- MAY respond to a "toolTip" selector to return a string for
  that tab’s tooltip.

See the class "TerminalWindow_TabSource" which is an example
of a valid object for the array.

In the future, additional selectors may be prescribed for the
object (to set an icon, for instance).

(4.0)
*/
- (void)
setTabTargets:(NSArray*)	anObjectArray
andAction:(SEL)				aSelector
{
	if (self->targets != anObjectArray)
	{
		[self->targets release];
		self->targets = [anObjectArray copy];
	}
	self->action = aSelector;
	
	// update the user interface
	[self->segmentedControl setSegmentCount:[self->targets count]];
	[self->segmentedControl setSelectedSegment:0];
	{
		NSEnumerator*	toObject = [self->targets objectEnumerator];
		NSMenu*			menuRep = [[[NSMenu alloc] init] autorelease];
		NSMenuItem*		menuItem = [[[NSMenuItem alloc] initWithTitle:@"" action:self->action keyEquivalent:@""] autorelease];
		unsigned int	i = 0;
		
		
		// with "performMenuAction:" and "performSegmentedControlAction:",
		// actions can be handled consistently by the caller; those
		// methods reroute invocations by menu item or segmented control,
		// and present the same sender (this NSToolbarItem) instead
		while (id object = [toObject nextObject])
		{
			NSMenuItem*		newItem = [[[NSMenuItem alloc] initWithTitle:[[object attributedDescription] string]
																			action:@selector(performMenuAction:) keyEquivalent:@""]
										autorelease];
			
			
			[newItem setAttributedTitle:[object attributedDescription]];
			[newItem setTarget:self];
			[self->segmentedControl setLabel:[[object attributedDescription] string] forSegment:i];
			if ([object respondsToSelector:@selector(toolTip)])
			{
				[[self->segmentedControl cell] setToolTip:[object toolTip] forSegment:i];
			}
			[menuRep addItem:newItem];
			++i;
		}
		[menuItem setSubmenu:menuRep];
		[self setMenuFormRepresentation:menuItem];
	}
}// setTabTargets:andAction:


@end // TerminalWindow_ToolbarItemTabs


@implementation NSWindow (TerminalWindow_NSWindowExtensions)


/*!
Returns any Terminal Window that is associated with an NSWindow,
or nullptr if there is none.

(4.0)
*/
- (TerminalWindowRef)
terminalWindowRef
{
	My_TerminalWindowByNSWindow::const_iterator		toPair = gTerminalNSWindows().find(self);
	TerminalWindowRef								result = nullptr;
	
	
	if (gTerminalNSWindows().end() != toPair)
	{
		result = toPair->second;
	}
	return result;
}// terminalWindowRef


@end // NSWindow (TerminalWindow_NSWindowExtensions)

// BELOW IS REQUIRED NEWLINE TO END FILE
