/*###############################################################

	TerminalWindow.cp
	
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

// standard-C includes
#include <cstring>

// standard-C++ includes
#include <algorithm>
#include <map>
#include <vector>

// GNU extension includes
#include <ext/numeric>

// UNIX includes
extern "C"
{
#	include <pthread.h>
#	include <strings.h>
}

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <ColorUtilities.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <Cursors.h>
#include <Embedding.h>
#include <HIViewWrap.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlockReferenceTracker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <Undoables.h>
#include <WindowInfo.h>

// resource includes
#include "DialogResources.h"
#include "GeneralResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "Console.h"
#include "DialogUtilities.h"
#include "EventInfoWindowScope.h"
#include "EventLoop.h"
#include "FindDialog.h"
#include "FormatDialog.h"
#include "GenericThreads.h"
#include "HelpSystem.h"
#include "Preferences.h"
#include "SessionFactory.h"
#include "SizeDialog.h"
#include "Terminal.h"
#include "TerminalWindow.h"
#include "TerminalView.h"
#include "UIStrings.h"



#pragma mark Constants

SInt16 const		kMaximumNumberOfArrangedWindows = 20; // TEMPORARY RESTRICTION


/*!
Use with getScrollBarKind() for an unknown scroll bar.
*/
enum TerminalWindowScrollBarKind
{
	kInvalidTerminalWindowScrollBarKind = 0,
	kTerminalWindowScrollBarKindVertical = 1,
	kTerminalWindowScrollBarKindHorizontal = 2
};

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"DimensionsFloater" and "DimensionsSheet" NIB from the
package "TerminalWindow.nib".
*/
static HIViewID const	idMyTextScreenDimensions	= { 'Dims', 0/* ID */ };

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Tab" NIB from the package "TerminalWindow.nib".
*/
static HIViewID const	idMyButtonTabTitle			= { 'TTit', 0/* ID */ };

#pragma mark Types

typedef std::vector< TerminalScreenRef >						TerminalScreenList;
typedef std::multimap< TerminalScreenRef, TerminalViewRef >		TerminalScreenToViewMultiMap;
typedef std::vector< TerminalViewRef >							TerminalViewList;
typedef std::map< TerminalViewRef, TerminalScreenRef >			TerminalViewToScreenMap;
typedef std::vector< Undoables_ActionRef >						UndoableActionList;

typedef MemoryBlockReferenceTracker< TerminalWindowRef >			TerminalWindowRefTracker;
typedef Registrar< TerminalWindowRef, TerminalWindowRefTracker >	TerminalWindowRefRegistrar;

struct TerminalWindow
{
	TerminalWindow  (Preferences_ContextRef, Preferences_ContextRef);
	~TerminalWindow ();
	
	TerminalWindowRefRegistrar	refValidator;				// ensures this reference is recognized as a valid one
	TerminalWindowRef			selfRef;					// redundant reference to self, for convenience
	
	ListenerModel_Ref			changeListenerModel;		// who to notify for various kinds of changes to this terminal data
	
	HIWindowRef					window;						// the Mac OS window reference for the terminal window
	CFRetainRelease				tab;						// the Mac OS window reference (if any) for the sister window acting as a tab
	CarbonEventHandlerWrap*		tabDragHandlerPtr;			// used to track drags that enter tabs
	WindowGroupRef				tabAndWindowGroup;			// WindowGroupRef; forces the window and its tab to move together
	Float32						tabOffsetInPixels;			// used to position the tab drawer, if any
	Float32						tabWidthInPixels;			// used to position and size the tab drawer, if any
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
	FindDialog_Ref				searchSheet;			// search dialog, if any (retained so that history is kept each time, etc.)
	FindDialog_Options			recentSearchOptions;	// used to implement Find Again baesd on the most recent Find
	CFRetainRelease				recentSearchString;		// CFStringRef; used to implement Find Again based on the most recent Find
	CFRetainRelease				baseTitleString;		// user-provided title string; may be adorned prior to becoming the window title
	CFRetainRelease				preResizeTitleString;	// used to save the old title during resizes, where the title is overwritten
	ControlActionUPP			scrollProcUPP;							// handles scrolling activity
	CommonEventHandlers_WindowResizer	windowResizeHandler;			// responds to changes in the window size
	CommonEventHandlers_WindowResizer	tabDrawerWindowResizeHandler;	// responds to changes in the tab drawer size
	CarbonEventHandlerWrap		mouseWheelHandler;						// responds to scroll wheel events
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
	ListenerModel_ListenerRef	sessionStateChangeEventListener;		// responds to changes in a session
	ListenerModel_ListenerRef	terminalStateChangeEventListener;		// responds to changes in a terminal
	ListenerModel_ListenerRef	terminalViewEventListener;				// responds to changes in a terminal view
	ListenerModel_ListenerRef	toolbarStateChangeEventListener;		// responds to changes in a toolbar
	
	TerminalScreenToViewMultiMap	screensToViews;			// map of a screen buffer to one or more views
	TerminalViewToScreenMap			viewsToScreens;			// map of views to screen buffers
	TerminalScreenList				allScreens;				// all screen buffers represented in the two maps above
	TerminalViewList				allViews;				// all views represented in the two maps above
	
	UndoableActionList			installedActions;			// undoable things installed on behalf of this window
};
typedef TerminalWindow*			TerminalWindowPtr;
typedef TerminalWindow const*	TerminalWindowConstPtr;

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
	Undoables_ActionRef		action;				//!< used to manage the Undo command
	TerminalWindowRef		terminalWindow;		//!< which window was reformatted
	UInt16					fontSize;			//!< the old font size (ignored if "undoFontSize" is false)
	Float32					oldContentLeft;		//!< old location in pixels, left edge of content area
	Float32					oldContentTop;		//!< old location in pixels, top edge of content area
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

typedef MemoryBlockPtrLocker< TerminalWindowRef, TerminalWindow >	TerminalWindowPtrLocker;
typedef LockAcquireRelease< TerminalWindowRef, TerminalWindow >		TerminalWindowAutoLocker;

#pragma mark Internal Method Prototypes

static void					calculateWindowPosition			(TerminalWindowPtr, Rect*);
static void					calculateIndexedWindowPosition	(TerminalWindowPtr, SInt16, Point*);
static void					changeNotifyForTerminalWindow	(TerminalWindowPtr, TerminalWindow_Change, void*);
static IconRef				createBellOffIcon				();
static IconRef				createBellOnIcon				();
static IconRef				createFullScreenIcon			();
static IconRef				createHideWindowIcon			();
static HIWindowRef			createKioskOffSwitchWindow		();
static IconRef				createScrollLockOffIcon			();
static IconRef				createScrollLockOnIcon			();
static IconRef				createLEDOffIcon				();
static IconRef				createLEDOnIcon					();
static void					createViews						(TerminalWindowPtr);
static Boolean				createTabWindow					(TerminalWindowPtr);
static HIWindowRef			createWindow					();
static void					ensureTopLeftCornersExists		();
static TerminalScreenRef	getActiveScreen					(TerminalWindowPtr);
static TerminalViewRef		getActiveView					(TerminalWindowPtr);
static UInt16				getGrowBoxHeight				();
static UInt16				getGrowBoxWidth					();
static TerminalWindowScrollBarKind	getScrollBarKind		(TerminalWindowPtr, HIViewRef);
static TerminalScreenRef	getScrollBarScreen				(TerminalWindowPtr, HIViewRef);
static TerminalViewRef		getScrollBarView				(TerminalWindowPtr, HIViewRef);
static UInt16				getStatusBarHeight				(TerminalWindowPtr);
static UInt16				getToolbarHeight				(TerminalWindowPtr);
static void					getViewSizeFromWindowSize		(TerminalWindowPtr, SInt16, SInt16, SInt16*, SInt16*);
static void					getWindowSizeFromViewSize		(TerminalWindowPtr, SInt16, SInt16, SInt16*, SInt16*);
static void					handleFindDialogClose			(FindDialog_Ref);
static void					handleNewDrawerWindowSize		(WindowRef, Float32, Float32, void*);
static void					handleNewSize					(WindowRef, Float32, Float32, void*);
static void					handlePendingUpdates			();
static void					installUndoFontSizeChanges		(TerminalWindowRef, Boolean, Boolean);
static void					installUndoFullScreenChanges	(TerminalWindowRef);
static void					installUndoScreenDimensionChanges	(TerminalWindowRef);
static pascal OSStatus		receiveGrowBoxClick				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveMouseWheelEvent			(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveTabDragDrop				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveToolbarEvent				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowCursorChange		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowDragCompleted		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowGetClickActivation	(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowResize				(EventHandlerCallRef, EventRef, void*);
static void					reverseFontChanges				(Undoables_ActionInstruction, Undoables_ActionRef, void*);
static void					reverseFullScreenChanges		(Undoables_ActionInstruction, Undoables_ActionRef, void*);
static void					reverseScreenDimensionChanges	(Undoables_ActionInstruction, Undoables_ActionRef, void*);
static pascal void			scrollProc						(HIViewRef, HIViewPartCode);
static void					sessionStateChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static OSStatus				setCursorInWindow				(WindowRef, Point, UInt32);
static void					setStandardState				(TerminalWindowPtr, UInt16, UInt16, Boolean);
static void					setWarningOnWindowClose			(TerminalWindowPtr, Boolean);
static void					stackWindowTerminalWindowOp		(TerminalWindowRef, void*, SInt32, void*);
static void					terminalStateChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void					terminalViewStateChanged		(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void		updateScreenSizeDialogCloseNotifyProc		(SizeDialog_Ref, Boolean);
static void					updateScrollBars				(TerminalWindowPtr);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	TerminalWindowRefTracker&	gTerminalWindowValidRefs ()		{ static TerminalWindowRefTracker x; return x; }
	TerminalWindowPtrLocker&	gTerminalWindowPtrLocks ()		{ static TerminalWindowPtrLocker x; return x; }
	SInt16**					gTopLeftCorners = nullptr;
	SInt16						gNumberOfTransitioningWindows = 0;	// used only by TerminalWindow_StackWindows()
	HIWindowRef					gKioskOffSwitchWindow ()		{ static HIWindowRef x = createKioskOffSwitchWindow(); return x; }
	TerminalWindowRef&			gKioskTerminalWindow ()			{ static TerminalWindowRef x = nullptr; return x; }
	IconRef&					gBellOffIcon ()					{ static IconRef x = createBellOffIcon(); return x; }
	IconRef&					gBellOnIcon ()					{ static IconRef x = createBellOnIcon(); return x; }
	IconRef&					gFullScreenIcon ()				{ static IconRef x = createFullScreenIcon(); return x; }
	IconRef&					gHideWindowIcon ()				{ static IconRef x = createHideWindowIcon(); return x; }
	IconRef&					gLEDOffIcon ()					{ static IconRef x = createLEDOffIcon(); return x; }
	IconRef&					gLEDOnIcon ()					{ static IconRef x = createLEDOnIcon(); return x; }
	IconRef&					gScrollLockOffIcon ()			{ static IconRef x = createScrollLockOffIcon(); return x; }
	IconRef&					gScrollLockOnIcon ()			{ static IconRef x = createScrollLockOnIcon(); return x; }
	Float32						gDefaultTabWidth = 0.0;		// set later
}


#pragma mark Public Methods

/*!
Creates a new terminal window according to the given
specifications.  If any problems occur, nullptr is
returned; otherwise, a reference to the new terminal
window is returned.

Either context can be "nullptr" if you want to rely
on defaults.  These contexts only determine initial
settings; future changes to the preferences will not
affect the terminal window.

In general, you should NOT create terminal windows this
way; use the Session Factory module.

(3.0)
*/
TerminalWindowRef
TerminalWindow_New  (Preferences_ContextRef		inTerminalInfoOrNull,
					 Preferences_ContextRef		inFontInfoOrNull)
{
	TerminalWindowRef	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new TerminalWindow(inTerminalInfoOrNull, inFontInfoOrNull), TerminalWindowRef);
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
		Console_WriteLine("warning, attempt to dispose of locked terminal window");
	}
	else
	{
		// clean up
		{
			TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), *inoutRefPtr);
			
			
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (ptr->searchSheet == nullptr)
	{
		ptr->searchSheet = FindDialog_New(inRef, handleFindDialogClose, ptr->recentSearchOptions);
	}
	
	// display a text search dialog (automatically disposed when the user clicks a button)
	FindDialog_Display(ptr->searchSheet);
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	HIViewRef					hitView = nullptr;
	OSStatus					error = noErr;
	Boolean						result = false;
	
	
	error = HIViewGetViewForMouseEvent(HIViewGetRoot(ptr->window), inMouseEvent, &hitView);
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
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

Currently, MacTelnet only has one screen per window, so
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
		TerminalWindowAutoLocker			ptr(gTerminalWindowPtrLocks(), inRef);
		TerminalScreenList::const_iterator	screenIterator;
		TerminalScreenList::const_iterator	maxIterator = ptr->allScreens.begin();
		
		
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (outColumnCountPtrOrNull != nullptr) *outColumnCountPtrOrNull = Terminal_ReturnColumnCount(getActiveScreen(ptr)/* TEMPORARY */);
	if (outRowCountPtrOrNull != nullptr) *outRowCountPtrOrNull = Terminal_ReturnRowCount(getActiveScreen(ptr)/* TEMPORARY */);
}// GetScreenDimensions


/*!
Returns the tab width, in pixels, of the specified terminal
window.  If the window has never been explicitly sized, some
default width will be returned.  Otherwise, the size most
recently set with TerminalWindow_SetTabWidth() will be returned.

\retval kTerminalWindow_ResultOK
if there are no errors

\retval kTerminalWindow_ResultInvalidReference
if the specified terminal window is unrecognized

\retval kTerminalWindow_ResultGenericFailure
if no window has ever had a tab; "outWidthInPixels" will be 0

(3.1)
*/
TerminalWindow_Result
TerminalWindow_GetTabWidth	(TerminalWindowRef	inRef,
							 Float32&			outWidthInPixels)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result		result = kTerminalWindow_ResultOK;
	
	
	if (gTerminalWindowValidRefs().end() == gTerminalWindowValidRefs().find(inRef))
	{
		result = kTerminalWindow_ResultInvalidReference;
		Console_WriteValueAddress("warning, attempt to TerminalWindow_GetTabWidth() with invalid reference", inRef);
		outWidthInPixels = 0;
	}
	else
	{
		if (0.0 == ptr->tabWidthInPixels) result = kTerminalWindow_ResultGenericFailure;
		outWidthInPixels = ptr->tabWidthInPixels;
	}
	
	return result;
}// GetTabWidth


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

Currently, MacTelnet only has one view per window, so
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
		TerminalWindowAutoLocker			ptr(gTerminalWindowPtrLocks(), inRef);
		TerminalViewList::const_iterator	viewIterator;
		TerminalViewList::const_iterator	maxIterator = ptr->allViews.begin();
		
		
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
purpose.  For example, if you are implementing a tab
switcher, passing "kTerminalWindow_ViewGroup3" or some
other more specific group lets you access the views
that should appear on a particular tab.  Basically,
since you are communicating a certain amount of context
to the Terminal Window module, you can receive a set of
views that makes sense for what you are doing - and in
the order you would need them to be in.

Use TerminalWindow_GetViewCountInGroup() to determine
an appropriate size for your array, then allocate an
array and pass the count as "inArrayLength" (and be
sure to pass the same group constant, too!).  Note
that this is the number of *elements*, not necessarily
the number of bytes!

Currently, MacTelnet only has one view per window, so
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
	case kTerminalWindow_ViewGroup1:
	case kTerminalWindow_ViewGroup2:
	case kTerminalWindow_ViewGroup3:
	case kTerminalWindow_ViewGroup4:
	case kTerminalWindow_ViewGroup5:
		if (outViewArray != nullptr)
		{
			TerminalWindowAutoLocker			ptr(gTerminalWindowPtrLocks(), inRef);
			TerminalViewList::const_iterator	viewIterator;
			TerminalViewList::const_iterator	maxIterator = ptr->allViews.begin();
			
			
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
Returns "true" only if the specified window is obscured,
meaning it is invisible to the user but technically
considered a visible window.  This is the state used by
the “Hide Front Window” command.

(3.0)
*/
Boolean
TerminalWindow_IsObscured	(TerminalWindowRef	inRef)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	Boolean						result = false;
	
	
	result = ptr->isObscured;
	return result;
}// IsObscured


/*!
Returns the Terminal Window associated with the specified
window, if any.  A window that is not a terminal window
will cause a nullptr return value.

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
Returns the number of distinct virtual terminal screen
buffers in the given terminal window.  For example,
even if the window contains 3 split-pane views of the
same screen buffer, the result will still be 1.

Currently, MacTelnet only has one screen per window,
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	UInt16						result = 0;
	
	
	result = ptr->allScreens.size();
	return result;
}// ReturnScreenCount


/*!
Returns a reference to the virtual terminal that has most
recently had keyboard focus in the given terminal window.
Thus, a valid reference is returned even if no terminal
screen control has the keyboard focus.

WARNING:	MacTelnet could change in the future to
			support multiple screens per window.  Be sure
			to use TerminalWindow_GetScreens() instead of
			this routine if it is appropriate to iterate
			over all screens in a window.

(3.0)
*/
TerminalScreenRef
TerminalWindow_ReturnScreenWithFocus	(TerminalWindowRef	inRef)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalScreenRef			result = nullptr;
	
	
	result = getActiveScreen(ptr);
	return result;
}// ReturnScreenWithFocus


/*!
Returns the height in pixels of a scroll bar in any terminal
window.  In Carbon, this routine automatically calls an
Appearance API to determine the current system-wide scroll bar
size.

(3.0)
*/
UInt16
TerminalWindow_ReturnScrollBarHeight ()
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
}// ReturnScrollBarHeight


/*!
Returns the width in pixels of a scroll bar in any terminal
window.  In Carbon, this routine automatically calls an
Appearance API to determine the current system-wide scroll bar
size.

(3.0)
*/
UInt16
TerminalWindow_ReturnScrollBarWidth ()
{
	UInt16		result = 0;
	SInt32		data = 0L;
	OSStatus	error = noErr;
	
	
	error = GetThemeMetric(kThemeMetricScrollBarWidth, &data);
	if (error != noErr) Console_WriteValue("unexpected error using GetThemeMetric()", error);
	result = data;
	return result;
}// ReturnScrollBarWidth


/*!
Returns the number of distinct terminal views in the
given terminal window.  For example, if a window has a
single split, the result will be 2.

Currently, MacTelnet only has one view per window, so
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	UInt16						result = 0;
	
	
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	UInt16						result = 0;
	
	
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
	
	case kTerminalWindow_ViewGroup1:
	case kTerminalWindow_ViewGroup2:
	case kTerminalWindow_ViewGroup3:
	case kTerminalWindow_ViewGroup4:
	case kTerminalWindow_ViewGroup5:
		// not currently used
		result = 0;
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

WARNING:	MacTelnet could change in the future to
			support multiple views per window.  Be sure
			to use TerminalWindow_GetViews() instead of
			this routine if it is appropriate to iterate
			over all views in a window.

(3.0)
*/
TerminalViewRef
TerminalWindow_ReturnViewWithFocus		(TerminalWindowRef	inRef)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalViewRef				result = nullptr;
	
	
	result = getActiveView(ptr);
	return result;
}// ReturnViewWithFocus


/*!
Returns the Mac OS window reference for the specified
terminal window.

IMPORTANT:	If an API exists to manipulate a terminal
			window, use the Terminal Window API; only
			use the Mac OS window reference when
			absolutely necessary.

(3.0)
*/
WindowRef
TerminalWindow_ReturnWindow		(TerminalWindowRef	inRef)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	WindowRef					result = nullptr;
	
	
	result = ptr->window;
	return result;
}// ReturnWindow


/*!
Changes the font and/or size used by all screens in
the specified terminal window.  The font size also
affects any other text elements (such as the status
text in the toolbar).  If the font name is nullptr,
the font is not changed.  If the size is 0, the
size is not changed.

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

(3.0)
*/
void
TerminalWindow_SetFontAndSize	(TerminalWindowRef		inRef,
								 ConstStringPtr			inFontFamilyNameOrNull,
								 UInt16					inFontSizeOrZero)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalViewRef				activeView = getActiveView(ptr);
	TerminalView_DisplayMode	oldMode = kTerminalView_DisplayModeNormal;
	TerminalView_Result			viewResult = kTerminalView_ResultOK;
	SInt16						screenWidth = 0;
	SInt16						screenHeight = 0;
	
	
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
	
	// set the standard state to be large enough for the current font and size;
	// and, set window dimensions to this new standard size
	TerminalView_GetIdealSize(activeView/* TEMPORARY - must consider a list of views */,
								screenWidth, screenHeight);
	setStandardState(ptr, screenWidth, screenHeight, true/* resize window */);
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (ptr->isObscured != inIsHidden)
	{
		ptr->isObscured = inIsHidden;
		if (inIsHidden)
		{
			// hide the window and notify listeners of the event (that ought to trigger
			// actions such as a zoom rectangle effect, updating Window menu items, etc.)
			HiliteWindow(ptr->window, false); // doing this might avoid a bug on re-display?
			HideWindow(ptr->window);
			
			// notify interested listeners about the change in state
			changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeObscuredState, ptr->selfRef/* context */);
		}
		else
		{
			// show the window and notify listeners of the event (that ought to trigger
			// actions such as updating Window menu items, etc.)
			ShowWindow(ptr->window);
			
			// also restore the window if it was collapsed/minimized
			if (IsWindowCollapsed(ptr->window)) CollapseWindow(ptr->window, false);
			
			// notify interested listeners about the change in state
			changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeObscuredState, ptr->selfRef/* context */);
		}
	}
}// SetObscured


/*!
Changes the dimensions of the visible screen area
in the given terminal window.  If split-panes are
active, the total view size is shared among the
panes, proportional to their current sizes.

The screen size is currently tied to the window
dimensions, so adjusting these parameters will
force the window to resize to use the new space.
In the future, it may make more sense to leave
the user’s chosen size intact (at least, when the
new view size will fit within the current window).

As a convenience, you may choose to send the resize
event back to MacTelnet, for recording into scripts.

(3.0)
*/
void
TerminalWindow_SetScreenDimensions	(TerminalWindowRef	inRef,
									 UInt16				inNewColumnCount,
									 UInt16				inNewRowCount,
									 Boolean			inSendToRecordingScripts)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	// set the standard state to be large enough for the specified number of columns and rows;
	// and, set window dimensions to this new standard size
	{
		TerminalViewRef				activeView = getActiveView(ptr);
		TerminalView_DisplayMode	oldMode = kTerminalView_DisplayModeNormal;
		TerminalView_Result			viewResult = kTerminalView_ResultOK;
		SInt16						screenWidth = 0;
		SInt16						screenHeight = 0;
		
		
		// changing the window size will force the view to match;
		// temporarily change the view mode to allow this, since
		// the view might automatically be controlling its font size
		oldMode = TerminalView_ReturnDisplayMode(activeView);
		viewResult = TerminalView_SetDisplayMode(activeView, kTerminalView_DisplayModeNormal);
		assert(kTerminalView_ResultOK == viewResult);
		TerminalView_GetTheoreticalViewSize(activeView/* TEMPORARY - must consider a list of views */,
											inNewColumnCount, inNewRowCount,
											&screenWidth, &screenHeight);
		setStandardState(ptr, screenWidth, screenHeight, true/* resize window */);
		viewResult = TerminalView_SetDisplayMode(activeView, oldMode);
		assert(kTerminalView_ResultOK == viewResult);
	}
	
	changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeScreenDimensions, inRef/* context */);
	
	if (inSendToRecordingScripts) SizeDialog_SendRecordableDimensionChangeEvents(inNewColumnCount, inNewRowCount);
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (ptr->window != nullptr) SetWindowAlternateTitle(ptr->window, inName);
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	OSStatus					result = noErr;
	
	
	if (inIsTab)
	{
		HIWindowRef		tabWindow = nullptr;
		Boolean			isNew = false;
		
		
		// create a sister tab window and group it with the terminal window
		if (false == ptr->tab.exists())
		{
			(Boolean)createTabWindow(ptr);
			assert(ptr->tab.exists());
			isNew = true;
		}
		
		tabWindow = REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef);
		
		if (isNew)
		{
			// update the tab display to match the window title
			TerminalWindow_SetWindowTitle(inRef, ptr->baseTitleString.returnCFStringRef());
			
			// attach the tab to the top edge of the window
			result = SetDrawerParent(tabWindow, ptr->window);
			if (noErr == result)
			{
				result = SetDrawerPreferredEdge(tabWindow, kWindowEdgeTop);
			}
		}
		
		if (noErr == result)
		{
			// IMPORTANT: This will return paramErr if the window is invisible.
			result = OpenDrawer(tabWindow, kWindowEdgeDefault, false/* asynchronously */);
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
Specifies the position of the tab (if any) for this window.
This is a visual adornment only; you typically use this when
windows are grouped and you want all tabs to be visible at
the same time.

WARNING:	The Mac OS X window manager will not allow a
			drawer to be cut off, and it solves this problem
			by resizing the *parent* (terminal) window to
			make room for the tab.  If you do not want this
			behavior, you need to check in advance how large
			the window is, and what a reasonable tab placement
			would be.

Note that since this affects only a single window, this is
not the proper API for general tab manipulation; it is a
low-level routine.  See the Workspace module.

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
								 Float32			inOffsetFromStartingPointInPixels)
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result		result = kTerminalWindow_ResultOK;
	
	
	if (gTerminalWindowValidRefs().end() == gTerminalWindowValidRefs().find(inRef))
	{
		result = kTerminalWindow_ResultInvalidReference;
		Console_WriteValueAddress("warning, attempt to TerminalWindow_SetTabPosition() with invalid reference", inRef);
	}
	else
	{
		ptr->tabOffsetInPixels = inOffsetFromStartingPointInPixels;
		if (false == ptr->tab.exists())
		{
			result = kTerminalWindow_ResultGenericFailure;
		}
		else
		{
			// drawers are managed in terms of start and end offsets as opposed to
			// a “width”, so some roundabout calculations are done to find offsets
			HIWindowRef		tabWindow = REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef);
			HIWindowRef		parentWindow = ptr->window;
			Rect			currentTabBounds;
			Rect			currentParentBounds;
			OSStatus		error = noErr;
			float			leadingOffset = kWindowOffsetUnchanged;
			float			trailingOffset = kWindowOffsetUnchanged;
			
			
			error = GetWindowBounds(tabWindow, kWindowStructureRgn, &currentTabBounds);
			assert_noerr(error);
			error = GetWindowBounds(parentWindow, kWindowStructureRgn, &currentParentBounds);
			assert_noerr(error);
			leadingOffset = (float)ptr->tabOffsetInPixels;
			// it does not appear necessary to set the trailing offset...
			//trailingOffset = (float)(currentParentBounds.right - currentParentBounds.left -
			//							(currentTabBounds.right - currentTabBounds.left) - leadingOffset);
			
			// force a “resize” to cause the tab position to update immediately
			// (TEMPORARY: is there a better way to do this?)
			++currentParentBounds.right;
			SetWindowBounds(parentWindow, kWindowStructureRgn, &currentParentBounds);
			error = SetDrawerOffsets(tabWindow, leadingOffset, trailingOffset);
			--currentParentBounds.right;
			SetWindowBounds(parentWindow, kWindowStructureRgn, &currentParentBounds);
		}
	}
	
	return result;
}// SetTabPosition


/*!
Specifies the size of the tab (if any) for this window,
including any frame it has.  This is a visual adornment
only; see also TerminalWindow_SetTabPosition().

You can pass "kTerminalWindow_DefaultMetaTabWidth" to
indicate that the tab should be resized to its ordinary
(default) width.

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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result		result = kTerminalWindow_ResultOK;
	
	
	if (gTerminalWindowValidRefs().end() == gTerminalWindowValidRefs().find(inRef))
	{
		result = kTerminalWindow_ResultInvalidReference;
		Console_WriteValueAddress("warning, attempt to TerminalWindow_SetTabWidth() with invalid reference", inRef);
	}
	else
	{
		ptr->tabWidthInPixels = inWidthInPixels;
		if (false == ptr->tab.exists())
		{
			result = kTerminalWindow_ResultGenericFailure;
		}
		else
		{
			// drawers are managed in terms of start and end offsets as opposed to
			// a “width”, so some roundabout calculations are done to find offsets
			HIWindowRef		tabWindow = REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef);
			HIWindowRef		parentWindow = GetDrawerParent(tabWindow);
			Rect			currentParentBounds;
			OSStatus		error = noErr;
			float			leadingOffset = kWindowOffsetUnchanged;
			float			trailingOffset = kWindowOffsetUnchanged;
			
			
			error = GetWindowBounds(parentWindow, kWindowStructureRgn, &currentParentBounds);
			assert_noerr(error);
			leadingOffset = (float)ptr->tabOffsetInPixels;
			trailingOffset = (float)(currentParentBounds.right - currentParentBounds.left -
										inWidthInPixels - leadingOffset);
			
			// force a “resize” to cause the tab position to update immediately
			// (TEMPORARY: is there a better way to do this?)
			++currentParentBounds.right;
			SetWindowBounds(parentWindow, kWindowStructureRgn, &currentParentBounds);
			error = SetDrawerOffsets(tabWindow, leadingOffset, trailingOffset);
			--currentParentBounds.right;
			SetWindowBounds(parentWindow, kWindowStructureRgn, &currentParentBounds);
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (nullptr != inName)
	{
		ptr->baseTitleString.setCFTypeRef(inName);
	}
	
	if (nullptr != ptr->window)
	{
		if (ptr->isDead)
		{
			// add a visual indicator to the window title of disconnected windows
			CFStringRef		adornedCFString = CFStringCreateWithFormat
												(kCFAllocatorDefault, nullptr/* format options */,
													CFSTR("~ %@ ~")/* LOCALIZE THIS? */,
													ptr->baseTitleString.returnCFStringRef());
			
			
			if (nullptr != adornedCFString)
			{
				SetWindowTitleWithCFString(ptr->window, adornedCFString);
				if (ptr->tab.exists())
				{
					HIViewWrap		titleWrap(idMyButtonTabTitle, REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef));
					
					
					SetControlTitleWithCFString(titleWrap, adornedCFString);
				}
				CFRelease(adornedCFString), adornedCFString = nullptr;
			}
		}
		else if (nullptr != inName)
		{
			SetWindowTitleWithCFString(ptr->window, ptr->baseTitleString.returnCFStringRef());
			if (ptr->tab.exists())
			{
				HIViewWrap		titleWrap(idMyButtonTabTitle, REINTERPRET_CAST(ptr->tab.returnHIObjectRef(), HIWindowRef));
				
				
				SetControlTitleWithCFString(titleWrap, ptr->baseTitleString.returnCFStringRef());
			}
		}
	}
	changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeWindowTitle, ptr->selfRef/* context */);
}// SetWindowTitle


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
	OSStatus					error = noErr;
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
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
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	// add a listener to the specified target’s listener model for the given setting change
	ListenerModel_RemoveListenerForEvent(ptr->changeListenerModel, inForWhatChange, inListener);
}// StopMonitoring


#pragma mark Internal Methods

/*!
Constructor.  See TerminalWindow_New().

(3.0)
*/
TerminalWindow::
TerminalWindow  (Preferences_ContextRef		inTerminalInfoOrNull,
				 Preferences_ContextRef		inFontInfoOrNull)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
refValidator(REINTERPRET_CAST(this, TerminalWindowRef), gTerminalWindowValidRefs()),
selfRef(REINTERPRET_CAST(this, TerminalWindowRef)),
changeListenerModel(ListenerModel_New(kListenerModel_StyleStandard,
										kConstantsRegistry_ListenerModelDescriptorTerminalWindowChanges)),
window(createWindow()),
tab(),
tabDragHandlerPtr(nullptr),
tabAndWindowGroup(nullptr),
tabOffsetInPixels(0.0),
tabWidthInPixels(0.0),
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
searchSheet(nullptr),
recentSearchOptions(kFindDialog_OptionsDefault),
recentSearchString(),
baseTitleString(),
scrollProcUPP(nullptr), // reset below
windowResizeHandler(),
tabDrawerWindowResizeHandler(),
mouseWheelHandler(GetApplicationEventTarget(), receiveMouseWheelEvent,
					CarbonEventSetInClass(CarbonEventClass(kEventClassMouse), kEventMouseWheelMoved),
					this->selfRef/* user data */),
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
	TerminalScreenRef		newScreen = nullptr;
	TerminalViewRef			newView = nullptr;
	Preferences_Result		preferencesResult = kPreferences_ResultOK;
	
	
	// get defaults if no contexts provided; if these cannot be found
	// for some reason, that’s fine because defaults are set in case
	// of errors later on
	if (nullptr == inTerminalInfoOrNull)
	{
		preferencesResult = Preferences_GetDefaultContext(&inTerminalInfoOrNull, kPreferences_ClassTerminal);
		assert(kPreferences_ResultOK == preferencesResult);
		assert(nullptr != inTerminalInfoOrNull);
	}
	if (nullptr == inFontInfoOrNull)
	{
		Boolean		chooseRandom = false;
		
		
		(Preferences_Result)Preferences_GetData(kPreferences_TagRandomTerminalFormats, sizeof(chooseRandom), &chooseRandom);
		if (chooseRandom)
		{
			std::vector< Preferences_ContextRef >	contextList;
			
			
			if (Preferences_GetContextsInClass(kPreferences_ClassFormat, contextList))
			{
				std::vector< UInt16 >	numberList(contextList.size());
				
				
				__gnu_cxx::iota(numberList.begin(), numberList.end(), 0/* starting value */);
				std::random_shuffle(numberList.begin(), numberList.end());
				inFontInfoOrNull = contextList[numberList[0]];
			}
			
			if (nullptr == inFontInfoOrNull) chooseRandom = false; // error...
		}
		
		if (false == chooseRandom)
		{
			preferencesResult = Preferences_GetDefaultContext(&inFontInfoOrNull, kPreferences_ClassFormat);
			assert(kPreferences_ResultOK == preferencesResult);
			assert(nullptr != inFontInfoOrNull);
		}
	}
	
	// set up Window Info; it is important to do this right away
	// because this is relied upon by other code to find the
	// terminal window data attached to the Mac OS window
	assert(this->window != nullptr);
	WindowInfo_SetWindowDescriptor(this->windowInfo, kConstantsRegistry_WindowDescriptorAnyTerminal);
	WindowInfo_SetWindowPotentialDropTarget(this->windowInfo, true/* can receive data via drag-and-drop */);
	WindowInfo_SetAuxiliaryDataPtr(this->windowInfo, REINTERPRET_CAST(this, TerminalWindowRef)); // the auxiliary data is the "TerminalWindowRef"
	WindowInfo_SetForWindow(this->window, this->windowInfo);
	
	// set up the Help System
	HelpSystem_SetWindowKeyPhrase(this->window, kHelpSystem_KeyPhraseTerminals);
	
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
	this->windowResizeHandler.install(this->window, handleNewSize, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
										250/* arbitrary minimum width */,
										200/* arbitrary minimum height */,
										SHRT_MAX/* maximum width */,
										SHRT_MAX/* maximum height */);
	assert(this->windowResizeHandler.isInstalled());
	
	// create controls
	{
		Terminal_Result		terminalError = kTerminal_ResultOK;
		
		
		terminalError = Terminal_NewScreen(inTerminalInfoOrNull, &newScreen);
		if (terminalError == kTerminal_ResultOK)
		{
			newView = TerminalView_NewHIViewBased(newScreen, inFontInfoOrNull);
			if (newView != nullptr)
			{
				HIViewWrap		contentView(kHIViewWindowContentID, this->window);
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
	{
		Rect		windowRect;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(this->window, kWindowContentRgn, &windowRect);
		assert_noerr(error);
		calculateWindowPosition(this, &windowRect);
		error = SetWindowBounds(this->window, kWindowContentRgn, &windowRect);
		assert_noerr(error);
	}
	
	// set up callbacks to receive various state change notifications
	this->sessionStateChangeEventListener = ListenerModel_NewStandardListener(sessionStateChanged, this->selfRef/* context */);
	SessionFactory_StartMonitoringSessions(kSession_ChangeSelected, this->sessionStateChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, this->sessionStateChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeStateAttributes, this->sessionStateChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowTitle, this->sessionStateChangeEventListener);
	this->terminalStateChangeEventListener = ListenerModel_NewStandardListener(terminalStateChanged, REINTERPRET_CAST(this, TerminalWindowRef)/* context */);
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeAudioState, this->terminalStateChangeEventListener);
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeNewLEDState, this->terminalStateChangeEventListener);
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeScrollActivity, this->terminalStateChangeEventListener);
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowFrameTitle, this->terminalStateChangeEventListener);
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowIconTitle, this->terminalStateChangeEventListener);
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowMinimization, this->terminalStateChangeEventListener);
	this->terminalViewEventListener = ListenerModel_NewStandardListener(terminalViewStateChanged, REINTERPRET_CAST(this, TerminalWindowRef)/* context */);
	TerminalView_StartMonitoring(newView, kTerminalView_EventFontSizeChanged, this->terminalViewEventListener);
	TerminalView_StartMonitoring(newView, kTerminalView_EventScrolling, this->terminalViewEventListener);
	
	// install a callback that handles commands relevant to terminal windows
	{
		EventTypeSpec const		whenCommandExecuted[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		OSStatus				error = noErr;
		
		
		this->commandUPP = NewEventHandlerUPP(receiveHICommand);
		error = InstallWindowEventHandler(this->window, this->commandUPP, GetEventTypeCount(whenCommandExecuted),
											whenCommandExecuted, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->commandHandler/* event handler reference */);
		assert_noerr(error);
		
		// this technically is only needed once; but the attempt is made for each new
		// terminal window, so ignore the errors in installing it multiple times
		(OSStatus)InstallWindowEventHandler(gKioskOffSwitchWindow(), this->commandUPP, GetEventTypeCount(whenCommandExecuted),
											whenCommandExecuted, nullptr/* user data */,
											nullptr/* event handler reference */);
	}
	
	// install a callback that tells the Window Manager the proper behavior for clicks in background terminal windows
	{
		EventTypeSpec const		whenWindowClickActivationRequired[] =
								{
									{ kEventClassWindow, kEventWindowGetClickActivation }
								};
		OSStatus				error = noErr;
		
		
		this->windowClickActivationUPP = NewEventHandlerUPP(receiveWindowGetClickActivation);
		error = InstallWindowEventHandler(this->window, this->windowClickActivationUPP, GetEventTypeCount(whenWindowClickActivationRequired),
											whenWindowClickActivationRequired, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->windowClickActivationHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that attempts to fix tab locations after a window is moved far enough below the menu bar
	{
		EventTypeSpec const		whenWindowDragCompleted[] =
								{
									{ kEventClassWindow, kEventWindowDragCompleted }
								};
		OSStatus				error = noErr;
		
		
		this->windowDragCompletedUPP = NewEventHandlerUPP(receiveWindowDragCompleted);
		error = InstallWindowEventHandler(this->window, this->windowDragCompletedUPP, GetEventTypeCount(whenWindowDragCompleted),
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
		error = InstallWindowEventHandler(this->window, this->windowCursorChangeUPP, GetEventTypeCount(whenCursorChangeRequired),
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
		error = InstallWindowEventHandler(this->window, this->windowResizeEmbellishUPP, GetEventTypeCount(whenWindowResizeStartsContinuesOrStops),
											whenWindowResizeStartsContinuesOrStops, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->windowResizeEmbellishHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that detects key modifiers when the size box is clicked
	{
		EventTypeSpec const		whenGrowBoxClicked[] =
								{
									{ kEventClassControl, kEventControlClick }
								};
		HIViewRef				growBoxView = nullptr;
		OSStatus				error = noErr;
		
		
		this->growBoxClickUPP = NewEventHandlerUPP(receiveGrowBoxClick);
		error = HIViewFindByID(HIViewGetRoot(this->window), kHIViewWindowGrowBoxID, &growBoxView);
		assert_noerr(error);
		error = HIViewInstallEventHandler(growBoxView, this->growBoxClickUPP, GetEventTypeCount(whenGrowBoxClicked),
											whenGrowBoxClicked, REINTERPRET_CAST(this, TerminalWindowRef)/* user data */,
											&this->growBoxClickHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// put the toolbar in the window
	{
		OSStatus	error = noErr;
		size_t		actualSize = 0L;
		Boolean		headersCollapsed = false;
		
		
		error = SetWindowToolbar(this->window, this->toolbar);
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
			error = ShowHideWindowToolbar(this->window, true/* show */, false/* animate */);
			assert_noerr(error);
		}
	}
	CFRelease(this->toolbar); // once set in the window, the toolbar is retained, so release the creation lock
	
	// enable drag tracking so that certain default toolbar behaviors work
	(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(this->window, true/* enabled */);
	
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
}// TerminalWindow 2-argument constructor


/*!
Destructor.  See TerminalWindow_Dispose().

(3.0)
*/
TerminalWindow::
~TerminalWindow ()
{
	// now that the window is going away, destroy any Undo commands
	// that could be applied to this window
	{
		UndoableActionList::const_iterator	actionIter;
		
		
		for (actionIter = this->installedActions.begin();
				actionIter != this->installedActions.end(); ++actionIter)
		{
			Undoables_RemoveAction(*actionIter);
		}
	}
	
	// destroy any open Find dialog
	if (this->searchSheet != nullptr)
	{
		FindDialog_Dispose(&this->searchSheet);
	}
	
	// show a hidden window just before it is destroyed (most importantly, notifying callbacks)
	TerminalWindow_SetObscured(REINTERPRET_CAST(this, TerminalWindowRef), false);
	
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
	if (nullptr != this->window)
	{
		if (IsWindowVisible(this->window))
		{
			// 3.0 - use a zoom effect to close windows
			Rect	lowerRight;
			Rect	screenRect;
			
			
			RegionUtilities_GetWindowDeviceGrayRect(this->window, &screenRect);
			SetRect(&lowerRight, screenRect.right, screenRect.bottom, screenRect.right, screenRect.bottom);
			if (noErr != TransitionWindow(this->window, kWindowZoomTransitionEffect,
											kWindowHideTransitionAction, &lowerRight))
			{
				HideWindow(this->window);
			}
		}
		KillControls(this->window);
	}
	
	// perform other clean-up
	if (gTopLeftCorners != nullptr) --((*gTopLeftCorners)[this->staggerPositionIndex]); // one less window at this position
	
	DisposeControlActionUPP(this->scrollProcUPP), this->scrollProcUPP = nullptr;
	
	// unregister session callbacks
	SessionFactory_StopMonitoringSessions(kSession_ChangeSelected, this->sessionStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeState, this->sessionStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeStateAttributes, this->sessionStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowTitle, this->sessionStateChangeEventListener);
	ListenerModel_ReleaseListener(&this->sessionStateChangeEventListener);
	
	// unregister screen buffer callbacks and destroy all buffers
	// (NOTE: perhaps this should be revisited, as a future feature
	// may be to allow multiple windows to use the same buffer; if
	// that were the case, killing one window should not necessarily
	// throw out its buffer)
	{
		TerminalScreenList::const_iterator	screenIterator;
		
		
		for (screenIterator = this->allScreens.begin(); screenIterator != this->allScreens.end(); ++screenIterator)
		{
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeAudioState, this->terminalStateChangeEventListener);
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeNewLEDState, this->terminalStateChangeEventListener);
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeScrollActivity, this->terminalStateChangeEventListener);
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeWindowFrameTitle, this->terminalStateChangeEventListener);
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeWindowIconTitle, this->terminalStateChangeEventListener);
			Terminal_StopMonitoring(*screenIterator, kTerminal_ChangeWindowMinimization, this->terminalStateChangeEventListener);
		}
	}
	ListenerModel_ReleaseListener(&this->terminalStateChangeEventListener);
	
	// destroy all terminal views
	{
		TerminalViewList::const_iterator	viewIterator;
		TerminalViewRef						view = nullptr;
		
		
		for (viewIterator = this->allViews.begin(); viewIterator != this->allViews.end(); ++viewIterator)
		{
			view = *viewIterator;
			TerminalView_StopMonitoring(view, kTerminalView_EventFontSizeChanged, this->terminalViewEventListener);
			TerminalView_StopMonitoring(view, kTerminalView_EventScrolling, this->terminalViewEventListener);
		}
	}
	ListenerModel_ReleaseListener(&this->terminalViewEventListener);
	
	// destroy all buffers (NOTE: perhaps this should be revisited, as
	// a future feature may be to allow multiple windows to use the same
	// buffer; if that were the case, killing one window should not
	// necessarily throw out its buffer)
	{
		TerminalScreenList::const_iterator	screenIterator;
		
		
		for (screenIterator = this->allScreens.begin(); screenIterator != this->allScreens.end(); ++screenIterator)
		{
			Terminal_DisposeScreen(*screenIterator);
		}
	}
	
	// throw away information about terminal window change listeners
	ListenerModel_Dispose(&this->changeListenerModel);
	
	// finally, dispose of the window
	if (this->window != nullptr)
	{
		HelpSystem_SetWindowKeyPhrase(this->window, kHelpSystem_KeyPhraseDefault); // clean up
		DisposeWindow(this->window), this->window = nullptr;
	}
	if (nullptr == this->tabAndWindowGroup)
	{
		ReleaseWindowGroup(this->tabAndWindowGroup), this->tabAndWindowGroup = nullptr;
	}
}// TerminalWindow destructor


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
static void
calculateIndexedWindowPosition	(TerminalWindowPtr	inPtr,
								 SInt16				inStaggerIndex,
								 Point*				outPositionPtr)
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
		
		RegionUtilities_GetPositioningBounds(inPtr->window, &screenRect);
		outPositionPtr->v = stackingOrigin.v + stagger.v * (1 + inStaggerIndex);
		outPositionPtr->h = stackingOrigin.h + stagger.h * (1 + inStaggerIndex);
	}
}// calculateIndexedWindowPosition


/*!
Calculates the position of windows.  In MacTelnet 3.0,
there is no user preference for staggering windows:
windows are ALWAYS staggered upon creation, and the
amount to stagger by changes depending on the current
Appearance theme.

(2.6)
*/
static void
calculateWindowPosition		(TerminalWindowPtr	inPtr,
							 Rect*				outArrangement)
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
	error = GetWindowBounds(inPtr->window, kWindowContentRgn, &contentRegionBounds);
	assert_noerr(error);
	while ((!done) && (!tooBig))
	{
		while (((*gTopLeftCorners)[inPtr->staggerPositionIndex] > currentCount) && // find an empty spot
					(inPtr->staggerPositionIndex < kMaximumNumberOfArrangedWindows - 1))
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
			
			
			RegionUtilities_GetPositioningBounds(inPtr->window, &screenRect);
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
static void
changeNotifyForTerminalWindow	(TerminalWindowPtr		inPtr,
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
static IconRef
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
static IconRef
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
Registers the “full screen” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
static IconRef
createFullScreenIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnPrefPanelKioskIconFilenameNoExtension(),
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
static IconRef
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
Creates a floating window containing an “off switch” for
Kiosk Mode.

Returns nullptr if the window was not created successfully.

(3.0)
*/
static HIWindowRef
createKioskOffSwitchWindow ()
{
	HIWindowRef		result = nullptr;
	
	
	// load the NIB containing this floater (automatically finds the right localization)
	result = NIBWindow(AppResources_ReturnBundleForNIBs(),
						CFSTR("KioskDisableFloater"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	
	return result;
}// createKioskOffSwitchWindow


/*!
Registers the “LED off” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
static IconRef
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
static IconRef
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
Registers the “scroll lock off” icon reference with the
system, and returns a reference to the new icon.

(3.1)
*/
static IconRef
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
static IconRef
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
static Boolean
createTabWindow		(TerminalWindowPtr		inPtr)
{
	HIWindowRef		tabWindow = nullptr;
	Boolean			result = false;
	
	
	// load the NIB containing this floater (automatically finds the right localization)
	tabWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
							CFSTR("TerminalWindow"), CFSTR("Tab")) << NIBLoader_AssertWindowExists;
	if (nullptr != tabWindow)
	{
		// install a callback that responds as the drawer window is resized; this is used
		// primarily to enforce a maximum drawer width, not to allow a resizable drawer
		{
			Rect		currentBounds;
			OSStatus	error = noErr;
			
			
			error = GetWindowBounds(tabWindow, kWindowContentRgn, &currentBounds);
			assert_noerr(error);
			inPtr->tabDrawerWindowResizeHandler.install(tabWindow, handleNewDrawerWindowSize, inPtr->selfRef/* user data */,
														currentBounds.right - currentBounds.left/* minimum width */,
														currentBounds.bottom - currentBounds.top/* minimum height */,
														currentBounds.right - currentBounds.left/* maximum width */,
														currentBounds.bottom - currentBounds.top/* maximum height */);
			assert(inPtr->tabDrawerWindowResizeHandler.isInstalled());
			
			// if the global default width has not yet been initialized, set it;
			// initialize this window’s tab width field to the global default
			if (0.0 == gDefaultTabWidth)
			{
				error = GetWindowBounds(tabWindow, kWindowStructureRgn, &currentBounds);
				assert_noerr(error);
				gDefaultTabWidth = STATIC_CAST(currentBounds.right - currentBounds.left, Float32);
			}
			inPtr->tabWidthInPixels = gDefaultTabWidth;
			
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
static void
createViews		(TerminalWindowPtr	inPtr)
{
	HIViewWrap	contentView(kHIViewWindowContentID, inPtr->window);
	Rect		rect;
	OSStatus	error = noErr;
	
	
	assert(contentView.exists());
	
	// create routine to handle scroll activity
	inPtr->scrollProcUPP = NewControlActionUPP(scrollProc); // this is disposed via TerminalWindow_Dispose()
	
	// create a vertical scroll bar; the resize event handler initializes its size correctly
	SetRect(&rect, 0, 0, 0, 0);
	error = CreateScrollBarControl(inPtr->window, &rect, 0/* value */, 0/* minimum */, 0/* maximum */, 0/* view size */,
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
	error = CreateScrollBarControl(inPtr->window, &rect, 0/* value */, 0/* minimum */, 0/* maximum */, 0/* view size */,
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
Creates a Mac OS window for the specified terminal window
and constructs a root control for subsequent control
embedding.

Returns nullptr if the window was not created successfully.

(3.0)
*/
static HIWindowRef
createWindow ()
{
	HIWindowRef		result = nullptr;
	
	
	// load the NIB containing this window (automatically finds the right localization)
	result = NIBWindow(AppResources_ReturnBundleForNIBs(),
						CFSTR("TerminalWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	if (nullptr != result)
	{
		// override this default; technically terminal windows
		// are immediately closeable for the first 15 seconds
		SetWindowModified(result, false);
	}
	
	return result;
}// createWindow


/*!
Allocates the "gTopLeftCorners" array if it does
not already exist.

(3.0)
*/
static void
ensureTopLeftCornersExists ()
{
	if (gTopLeftCorners == nullptr)
	{
		gTopLeftCorners = REINTERPRET_CAST(Memory_NewHandleInProperZone(kMaximumNumberOfArrangedWindows * sizeof(SInt16),
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
static TerminalScreenRef
getActiveScreen		(TerminalWindowPtr	inPtr)
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
static TerminalViewRef
getActiveView	(TerminalWindowPtr	inPtr)
{
	assert(!inPtr->allViews.empty());
	return inPtr->allViews.front(); // TEMPORARY; should instead use focus-change events from terminal views
}// getActiveView


/*!
Returns the height in pixels of the grow box in a terminal
window.  Currently this is identical to the height of a
horizontal scroll bar, but this function exists so that
code can explicitly identify this metric when no horizontal
scroll bar may be present.

(3.0)
*/
static UInt16
getGrowBoxHeight ()
{
	UInt16		result = 0;
	SInt32		data = 0L;
	OSStatus	error = noErr;
	
	
	error = GetThemeMetric(kThemeMetricScrollBarWidth, &data);
	if (error != noErr) Console_WriteValue("unexpected error using GetThemeMetric()", error);
	result = data;
	return result;
}// getGrowBoxHeight


/*!
Returns the width in pixels of the grow box in a terminal
window.  Currently this is identical to the width of a
vertical scroll bar, but this function exists so that code
can explicitly identify this metric when no vertical scroll
bar may be present.

(3.0)
*/
static UInt16
getGrowBoxWidth ()
{
	return TerminalWindow_ReturnScrollBarWidth();
}// getGrowBoxWidth


/*!
Returns a constant describing the type of scroll bar that
is given.  If the specified control does not belong to the
given terminal window, "kInvalidTerminalWindowScrollBarKind"
is returned; otherwise, a constant is returned indicating
whether the control is horizontal or vertical.

(3.0)
*/
static TerminalWindowScrollBarKind
getScrollBarKind	(TerminalWindowPtr	inPtr,
					 HIViewRef			inScrollBarControl)
{
	TerminalWindowScrollBarKind		result = kInvalidTerminalWindowScrollBarKind;
	
	
	if (inScrollBarControl == inPtr->controls.scrollBarH) result = kTerminalWindowScrollBarKindHorizontal;
	if (inScrollBarControl == inPtr->controls.scrollBarV) result = kTerminalWindowScrollBarKindVertical;
	return result;
}// getScrollBarKind


/*!
Returns the screen buffer that the given scroll bar
controls, or nullptr if none.

(3.0)
*/
static TerminalScreenRef
getScrollBarScreen	(TerminalWindowPtr	inPtr,
					 HIViewRef			UNUSED_ARGUMENT(inScrollBarControl))
{
	assert(!inPtr->allScreens.empty());
	return inPtr->allScreens.front(); // one day, if more than one view per window exists, this logic will be more complex
}// getScrollBarScreen


/*!
Returns the view that the given scroll bar controls,
or nullptr if none.

(3.0)
*/
static TerminalViewRef
getScrollBarView	(TerminalWindowPtr	inPtr,
					 HIViewRef			UNUSED_ARGUMENT(inScrollBarControl))
{
	assert(!inPtr->allViews.empty());
	return inPtr->allViews.front(); // one day, if more than one view per window exists, this logic will be more complex
}// getScrollBarView


/*!
Returns the height in pixels of the status bar.  The status bar
height is defined as the number of pixels between the toolbar
and the top edge of the terminal screen; thus, an invisible
status bar has zero height.

(3.0)
*/
static UInt16
getStatusBarHeight	(TerminalWindowPtr	UNUSED_ARGUMENT(inPtr))
{
	return 0;
}// getStatusBarHeight


/*!
Returns the height in pixels of the toolbar.  The toolbar
height is defined as the number of pixels between the top
edge of the window and the top edge of the status bar; thus,
an invisible toolbar has zero height.

(3.0)
*/
static UInt16
getToolbarHeight	(TerminalWindowPtr	UNUSED_ARGUMENT(inPtr))
{
	return 0;
}// getToolbarHeight


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
static void
getViewSizeFromWindowSize	(TerminalWindowPtr	inPtr,
							 SInt16				inWindowContentWidthInPixels,
							 SInt16				inWindowContentHeightInPixels,
							 SInt16*			outScreenInteriorWidthInPixels,
							 SInt16*			outScreenInteriorHeightInPixels)
{
	if (nullptr != outScreenInteriorWidthInPixels)
	{
		*outScreenInteriorWidthInPixels = inWindowContentWidthInPixels -
											TerminalWindow_ReturnScrollBarWidth();
	}
	if (nullptr != outScreenInteriorHeightInPixels)
	{
		*outScreenInteriorHeightInPixels = inWindowContentHeightInPixels - getStatusBarHeight(inPtr) - getToolbarHeight(inPtr) -
											TerminalWindow_ReturnScrollBarHeight();
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
static void
getWindowSizeFromViewSize	(TerminalWindowPtr	inPtr,
							 SInt16				inScreenInteriorWidthInPixels,
							 SInt16				inScreenInteriorHeightInPixels,
							 SInt16*			outWindowContentWidthInPixels,
							 SInt16*			outWindowContentHeightInPixels)
{
	if (nullptr != outWindowContentWidthInPixels)
	{
		*outWindowContentWidthInPixels = inScreenInteriorWidthInPixels +
											TerminalWindow_ReturnScrollBarWidth();
	}
	if (nullptr != outWindowContentHeightInPixels)
	{
		*outWindowContentHeightInPixels = inScreenInteriorHeightInPixels +
											getStatusBarHeight(inPtr) + getToolbarHeight(inPtr) +
											TerminalWindow_ReturnScrollBarHeight();
	}
}// getWindowSizeFromViewSize


/*!
Responds to a close of the Find dialog sheet in a terminal
window.  Currently, this just retains the keyword string
so that Find Again can be used, and remembers the user’s
most recent checkbox settings.

(3.0)
*/
static void
handleFindDialogClose	(FindDialog_Ref		inDialogThatClosed)
{
	TerminalWindowRef		terminalWindow = FindDialog_ReturnTerminalWindow(inDialogThatClosed);
	
	
	if (terminalWindow != nullptr)
	{
		TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
		CFStringRef					searchCFString = nullptr;
		
		
		// save things the user entered in the dialog
		ptr->recentSearchOptions = FindDialog_ReturnOptions(inDialogThatClosed);
		FindDialog_GetSearchString(inDialogThatClosed, searchCFString);
		ptr->recentSearchString.setCFTypeRef(searchCFString); // automatically retains
		
		// DO NOT destroy the dialog here; it is potentially reopened later
		// (and is destroyed when the Terminal Window is destroyed)
	}
}// handleFindDialogClose


/*!
This routine is called whenever the tab is resized,
to make sure the drawer does not become too large.

(3.1)
*/
static void
handleNewDrawerWindowSize	(WindowRef		UNUSED_ARGUMENT(inWindowRef),
							 Float32		UNUSED_ARGUMENT(inDeltaX),
							 Float32		UNUSED_ARGUMENT(inDeltaY),
							 void*			UNUSED_ARGUMENT(inContext))
{
	// nothing really needs to be done here; the routine exists only
	// to ensure the initial constraints (at routine install time)
	// are always enforced, otherwise the drawer size cannot shrink
}// handleNewDrawerWindowSize


/*!
This method moves and resizes the contents of a terminal
window in response to a resize.

(3.0)
*/
static void
handleNewSize	(WindowRef	inWindow,
				 Float32	UNUSED_ARGUMENT(inDeltaX),
				 Float32	UNUSED_ARGUMENT(inDeltaY),
				 void*		inTerminalWindowRef)
{
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	Rect				contentBounds;
	
	
	// get window boundaries in local coordinates
	GetPortBounds(GetWindowPort(inWindow), &contentBounds);
	
	if (terminalWindow != nullptr)
	{
		TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
		Rect						controlBounds;
		HIRect						floatBounds;
		
		
		// glue the vertical scroll bar to the new right side of the window and to the
		// bottom edge of the status bar, and ensure it is glued to the size box in the
		// corner (so vertically resize it)
		GetControlBounds(ptr->controls.scrollBarV, &controlBounds);
		controlBounds.left = contentBounds.right - TerminalWindow_ReturnScrollBarWidth();
		controlBounds.top = -1; // frame thickness
		controlBounds.right = contentBounds.right;
		controlBounds.bottom = contentBounds.bottom - getGrowBoxHeight();
		floatBounds = CGRectMake(controlBounds.left, controlBounds.top, controlBounds.right - controlBounds.left,
									controlBounds.bottom - controlBounds.top);
		//SetControlBounds(ptr->controls.scrollBarV, &controlBounds);
		HIViewSetFrame(ptr->controls.scrollBarV, &floatBounds);
		
		// glue the horizontal scroll bar to the new bottom edge of the window; it must
		// also move because its left edge is glued to the window edge, and it must resize
		// because its right edge is glued to the size box in the corner
		GetControlBounds(ptr->controls.scrollBarH, &controlBounds);
		controlBounds.left = -1; // frame thickness
		controlBounds.top = contentBounds.bottom - TerminalWindow_ReturnScrollBarHeight();
		controlBounds.right = contentBounds.right - getGrowBoxWidth();
		floatBounds = CGRectMake(controlBounds.left, controlBounds.top, controlBounds.right - controlBounds.left,
									controlBounds.bottom - controlBounds.top);
		//SetControlBounds(ptr->controls.scrollBarH, &controlBounds);
		HIViewSetFrame(ptr->controls.scrollBarH, &floatBounds);
		
		// change the screen sizes to match the user’s window size as well as possible,
		// notifying listeners of the change (to trigger actions such as sending messages
		// to the Unix process in the window, etc.); the number of columns in each screen
		// will be changed to closely match the overall width, but only the last screen’s
		// line count will be changed; in the event that there are tabs, only views
		// belonging to the same group will be scaled together during the resizing
		{
			TerminalWindow_ViewGroup const	kViewGroupArray[] =
											{
												kTerminalWindow_ViewGroup1,
												kTerminalWindow_ViewGroup2,
												kTerminalWindow_ViewGroup3,
												kTerminalWindow_ViewGroup4,
												kTerminalWindow_ViewGroup5
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
					OSStatus	error = noErr;
					
					
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
						//SetControlBounds(TerminalView_ReturnContainerHIView(viewArray[i]), &terminalScreenBounds);
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
		
		// redraw controls
		DrawControls(TerminalWindow_ReturnWindow(terminalWindow));
	}
}// handleNewSize


/*!
On Mac OS X, displays all unrendered changes to
visible graphics ports.  Useful in unusual
circumstances, namely any time drawing must occur
so soon that an event loop iteration is not
guaranteed to run first.  (An event loop iteration
will automatically handle pending updates.)

This has no real use on Mac OS 8 or 9, so it is not
available for Classic builds.

(3.1)
*/
static void
handlePendingUpdates ()
{
	EventRecord		updateEvent;
	Boolean			isEvent = false;
	
	
	// simply *checking* for events triggers approprate
	// flushing to the display; so would WaitNextEvent(),
	// but this is nice because it does not pull any
	// events from the queue (after all, this routine
	// couldn’t handle the events if they were pulled)
	isEvent = EventAvail(updateMask, &updateEvent);
}// handlePendingUpdates


/*!
Installs an Undo procedure that will revert
the font and/or font size of the specified
screen to its current font and/or font size
when the user chooses Undo.

(3.0)
*/
static void
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
		TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
		
		
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
static void
installUndoFullScreenChanges	(TerminalWindowRef	inTerminalWindow)
{
	OSStatus						error = noErr;
	UndoDataFullScreenChangesPtr	dataPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(UndoDataFullScreenChanges)),
																UndoDataFullScreenChangesPtr); // disposed in the action method
	
	
	if (dataPtr == nullptr) error = memFullErr;
	else
	{
		// initialize context structure
		Rect	windowBounds;
		
		
		dataPtr->terminalWindow = inTerminalWindow;
		TerminalWindow_GetFontAndSize(inTerminalWindow, nullptr/* font name */, &dataPtr->fontSize);
		GetWindowBounds(TerminalWindow_ReturnWindow(inTerminalWindow), kWindowContentRgn, &windowBounds);
		dataPtr->oldContentLeft = windowBounds.left;
		dataPtr->oldContentTop = windowBounds.top;
	}
	dataPtr->action = Undoables_NewAction(CFSTR("")/* undo name - not applicable (not exposed to users) */,
											nullptr /* redo name; this is not redoable */, reverseFullScreenChanges,
											kUndoableContextIdentifierTerminalFullScreenChanges, dataPtr);
	Undoables_AddAction(dataPtr->action);
	if (error != noErr) Console_WriteValue("Warning: Could not make font size and/or location change undoable, error", error);
	else
	{
		TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
		
		
		ptr->installedActions.push_back(dataPtr->action);
	}
}// installUndoFullScreenChanges


/*!
Installs an Undo procedure that will revert the
dimensions of the of the specified screen to its
current dimensions when the user chooses Undo.

(3.1)
*/
static void
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
		TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
		
		
		ptr->installedActions.push_back(dataPtr->action);
	}
}// installUndoScreenDimensionChanges


/*!
Handles "kEventControlClick" of "kEventClassControl"
for a terminal window’s grow box.

(3.1)
*/
static pascal OSStatus
receiveGrowBoxClick		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inTerminalWindowRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlClick);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the view in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the grow box was found, proceed
		if (noErr == result)
		{
			TerminalViewRef		focusedView = TerminalWindow_ReturnViewWithFocus(terminalWindow);
			UInt32				eventModifiers = 0L;
			OSStatus			getModifiersError = noErr;
			
			
			// the event is never completely handled; it is sent to the
			// parent so that a resize will still occur automatically!
			result = eventNotHandledErr;
			
			// remember the previous view mode, so that it can be restored later
			if (nullptr != focusedView)
			{
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				
				
				ptr->preResizeViewDisplayMode = TerminalView_ReturnDisplayMode(focusedView);
			}
			
			getModifiersError = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, eventModifiers);
			if (noErr == getModifiersError)
			{
				if (eventModifiers & optionKey)
				{
					if (nullptr != focusedView)
					{
						// when the option key is held down, the resize behavior is inverted;
						// this MUST BE UNDONE, but it is undone within the “resize ended”
						// event handler; see receiveWindowResize()
						TerminalView_SetDisplayMode(focusedView,
													(kTerminalView_DisplayModeNormal ==
														TerminalView_ReturnDisplayMode(focusedView))
														? kTerminalView_DisplayModeZoom
														: kTerminalView_DisplayModeNormal);
					}
				}
			}
		}
	}
	
	return result;
}// receiveGrowBoxClick


/*!
Handles "kEventCommandProcess" of "kEventClassCommand" for
terminal window commands.

(3.0)
*/
static pascal OSStatus
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
					// search without a dialog, using the most recent search keyword string
					// UNIMPLEMENTED - but could use terminal window’s ->recentSearchString field
					Sound_StandardAlert();
					result = eventNotHandledErr;
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
						TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
						Boolean						turnOnFullScreen = (kCommandKioskModeDisable != received.commandID) &&
																		(false == FlagManager_Test(kFlagKioskMode));
						Boolean						modalFullScreen = (kCommandFullScreenModal == received.commandID);
						
						
						// enable kiosk mode only if it is not enabled already
						if (turnOnFullScreen)
						{
							HIWindowRef			window = EventLoop_ReturnRealFrontWindow();
							Rect				deviceBounds;
							Boolean				allowForceQuit = true;
							Boolean				showMenuBar = true;
							Boolean				showOffSwitch = true;
							Boolean				showScrollBar = true;
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
									Preferences_GetData(kPreferences_TagKioskUsesSuperfluousEffects, sizeof(useEffects),
														&useEffects, &actualSize))
								{
									useEffects = true; // assume a value if the preference cannot be found
								}
							}
							
							if (modalFullScreen)
							{
								// hide the scroll bar, if requested
								unless (showScrollBar)
								{
									(OSStatus)HIViewSetVisible(ptr->controls.scrollBarV, false);
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
								
								// entire screen is available, so use it
								RegionUtilities_GetWindowDeviceGrayRect(window, &deviceBounds);
								
								// show “off switch” if user has requested it
								if ((showOffSwitch) && (nullptr != gKioskOffSwitchWindow()))
								{
									ShowWindow(gKioskOffSwitchWindow());
								}
								
								// set a global with the terminal window that is full screen,
								// so that its scroll bar can be displayed later (a bit of a hack...)
								gKioskTerminalWindow() = terminalWindow;
								
								// finally, set a global flag indicating the mode is in effect
								FlagManager_Set(kFlagKioskMode, true);
							}
							else
							{
								// menu bar and Dock will remain visible, so maximize within that region
								RegionUtilities_GetPositioningBounds(window, &deviceBounds);
							}
							
							// retrieve initial values so they can be restored later
							{
								CGrafPtr	windowPort = nullptr;
								CGrafPtr	oldPort = nullptr;
								GDHandle	device = nullptr;
								FontInfo	info;
								Str255		fontName;		// current terminal font
								UInt16		fontSize = 0;	// current font size (will change)
								SInt16		preservedFontID = 0;
								SInt16		preservedFontSize = 0;
								Style		preservedFontStyle = 0;
								UInt16		visibleColumns = 0;
								UInt16		visibleRows = 0;
								SInt16		screenInteriorWidth = 0;
								SInt16		screenInteriorHeight = 0;
								
								
								TerminalWindow_GetFontAndSize(terminalWindow, fontName, &fontSize);
								TerminalWindow_GetScreenDimensions(terminalWindow, &visibleColumns, &visibleRows);
								
								GetGWorld(&oldPort, &device);
								SetPortWindowPort(window);
								GetGWorld(&windowPort, &device);
								preservedFontID = GetPortTextFont(windowPort);
								preservedFontSize = GetPortTextSize(windowPort);
								preservedFontStyle = GetPortTextFace(windowPort);
								
								// determine the rectangle that *will* be the optimal new terminal screen area
								getViewSizeFromWindowSize(ptr, deviceBounds.right - deviceBounds.left,
															deviceBounds.bottom - deviceBounds.top,
															&screenInteriorWidth, &screenInteriorHeight);
								
								// choose an appropriate font size to best fill the screen; favor maximum
								// width over height, but reduce the font size if the characters overrun
								// the bottom of the screen
								TextFontByName(fontName);
								TextSize(fontSize);
								while ((CharWidth('A') * visibleColumns) < screenInteriorWidth)
								{
									TextSize(++fontSize);
								}
								do
								{
									TextSize(--fontSize);
									GetFontInfo(&info);
								} while (((info.ascent + info.descent + info.leading) * visibleRows) >= screenInteriorHeight);
								
								// undo the damage...
								TextFont(preservedFontID);
								TextSize(preservedFontSize);
								TextFace(preservedFontStyle);
								
								// resize the window and fill its monitor
								if (modalFullScreen)
								{
									installUndoFullScreenChanges(terminalWindow);
								}
								else
								{
									installUndoFontSizeChanges(terminalWindow, false/* undo font */, true/* undo font size */);
								}
								TerminalWindow_SetFontAndSize(terminalWindow, fontName, fontSize);
								{
									Rect	maxBounds;
									
									
									RegionUtilities_GetWindowMaximumBounds(window, &maxBounds, nullptr/* previous bounds */);
									(OSStatus)SetWindowBounds(window, kWindowContentRgn, &maxBounds);
								}
								SetPort(oldPort);
							}
						}
						else
						{
							// end Kiosk Mode
							FlagManager_Set(kFlagKioskMode, false);
							HideWindow(gKioskOffSwitchWindow());
							if (nullptr != ptr) (OSStatus)HIViewSetVisible(ptr->controls.scrollBarV, true);
							assert_noerr(SetSystemUIMode(kUIModeNormal, 0/* options */));
							
							// undo font size changes, which is supposed to move the user’s
							// window back to where it was, at the original font size
							Commands_ExecuteByID(kCommandUndo);
							
							gKioskTerminalWindow() = nullptr;
							
							result = noErr;
						}
						
						result = noErr;
					}
					break;
				
				case kCommandFormatDefault:
					{
						// reformat frontmost window using the Default preferences
						Preferences_ContextRef		currentSettings = TerminalView_ReturnConfiguration
																		(TerminalWindow_ReturnViewWithFocus(terminalWindow));
						Preferences_ContextRef		defaultSettings = nullptr;
						Boolean						isError = true;
						
						
						if (kPreferences_ResultOK == Preferences_GetDefaultContext(&defaultSettings, kPreferences_ClassFormat))
						{
							isError = (kPreferences_ResultOK != Preferences_ContextCopy(defaultSettings, currentSettings));
						}
						
						if (isError)
						{
							// failed...
							Sound_StandardAlert();
						}
					}
					break;
				
				case kCommandFormatByFavoriteName:
					{
						// reformat frontmost window using the specified preferences
						Preferences_ContextRef		currentSettings = TerminalView_ReturnConfiguration
																		(TerminalWindow_ReturnViewWithFocus(terminalWindow));
						Boolean						isError = true;
						
						
						if (received.attributes & kHICommandFromMenu)
						{
							CFStringRef		collectionName = nullptr;
							
							
							if (noErr == CopyMenuItemTextAsCFString(received.menu.menuRef, received.menu.menuItemIndex, &collectionName))
							{
								Preferences_ContextRef		namedSettings = Preferences_NewContextFromFavorites
																			(kPreferences_ClassFormat, collectionName);
								
								
								if (nullptr != namedSettings)
								{
									isError = (kPreferences_ResultOK != Preferences_ContextCopy(namedSettings, currentSettings));
								}
								CFRelease(collectionName), collectionName = nullptr;
							}
						}
						
						if (isError)
						{
							// failed...
							Sound_StandardAlert();
						}
					}
					break;
				
				case kCommandFormat:
					{
						// display a format customization dialog
						FormatDialog_Ref	dialog = nullptr;
						
						
						// display the sheet
						dialog = FormatDialog_New(GetUserFocusWindow(),
													TerminalView_ReturnConfiguration(TerminalWindow_ReturnViewWithFocus(terminalWindow)));
						FormatDialog_Display(dialog); // automatically disposed when the user clicks a button
						
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
							rows = 48;
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
						SizeDialog_Ref		dialog = nullptr;
						
						
						// display the sheet
						dialog = SizeDialog_New(terminalWindow, updateScreenSizeDialogCloseNotifyProc);
						SizeDialog_Display(dialog); // automatically disposed when the user clicks a button
						
						result = noErr;
					}
					break;
				
				case kCommandTerminalNewWorkspace:
					// note that this event often originates from the tab drawer, but
					// due to event hierarchy it will eventually be sent to the handler
					// installed on the parent terminal window (how convenient!)
					SessionFactory_MoveTerminalWindowToNewWorkspace(terminalWindow);
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
static pascal OSStatus
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
			if (kEventMouseWheelAxisY != axis) result = eventNotHandledErr;
			else
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
						else
						{
							TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
							HIViewRef					scrollBar = ptr->controls.scrollBarV;
							HIViewPartCode				hitPart = (delta > 0)
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
	}
	return result;
}// receiveMouseWheelEvent


/*!
Handles "kEventControlDragEnter" for a terminal window tab.

Invoked by Mac OS X whenever the tab is involved in a
drag-and-drop operation.

(3.1)
*/
static pascal OSStatus
receiveTabDragDrop	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inTerminalWindowRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
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
						TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
						Boolean						acceptDrag = true;
						
						
						result = SetEventParameter(inEvent, kEventParamControlWouldAcceptDrop,
													typeBoolean, sizeof(acceptDrag), &acceptDrag);
						SelectWindow(ptr->window);
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
static pascal OSStatus
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
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarSeparatorIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarSpaceIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarFlexibleSpaceIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarPrintItemIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarCustomizeIdentifier);
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
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDScrollLock);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDHideWindow);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSeparatorIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED1);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED2);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED3);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDTerminalLED4);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSeparatorIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDFullScreen);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarCustomizeIdentifier);
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
								TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
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
								TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
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
								TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
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
								TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
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
							else if (kCFCompareEqualTo == CFStringCompare(kConstantsRegistry_HIToolbarItemIDTerminalBell,
																			identifierCFString, kCFCompareBackwards))
							{
								TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
								
								
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
							TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
							
							
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
static pascal OSStatus
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
static pascal OSStatus
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
			TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
			
			
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
static pascal OSStatus
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
			TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
			HIViewRef					control = nullptr;
			UInt32						actualSize = 0L;
			EventParamType				actualType = typeNull;
			
			
			// only clicks in controls matter for this
			result = eventNotHandledErr;
			if (noErr == GetEventParameter(inEvent, kEventParamControlRef, typeControlRef, &actualType,
											sizeof(control), &actualSize, &control))
			{
				// find clicks in terminal regions
				TerminalViewList::const_iterator	viewIterator;
				
				
				for (viewIterator = ptr->allViews.begin(); viewIterator != ptr->allViews.end(); ++viewIterator)
				{
					if (TerminalView_ReturnUserFocusHIView(*viewIterator) == control)
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
static pascal OSStatus
receiveWindowResize		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inTerminalWindowRef)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inTerminalWindowRef, TerminalWindowRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
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
			// a dimensions window is displayed during resize; but this does
			// not work prior to 10.3 for an as-yet-undetermined reason, so
			// only show it on 10.3 and beyond
			//Boolean					useSheet = FlagManager_Test(kFlagOS10_3API);
			Boolean						useSheet = false;
			Boolean						showWindow = FlagManager_Test(kFlagOS10_3API);
			TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
			
			
			if (kEventKind == kEventWindowResizeStarted)
			{
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
							ShowSheetWindow(ptr->resizeFloater, ptr->window);
						}
						else
						{
							ShowWindow(ptr->resizeFloater);
						}
					}
				}
				
				// remember the old window title
				{
					CFStringRef		nameCFString = nullptr;
					
					
					if (noErr == CopyWindowTitleAsCFString(ptr->window, &nameCFString))
					{
						ptr->preResizeTitleString.setCFTypeRef(nameCFString);
						CFRelease(nameCFString), nameCFString = nullptr;
					}
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
							SetWindowTitleWithCFString(ptr->window, newTitle);
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
					OSStatus	error = noErr;
					
					
					error = SetWindowTitleWithCFString(ptr->window, ptr->preResizeTitleString.returnCFStringRef());
					assert_noerr(error);
					ptr->preResizeTitleString.clear();
				}
			}
		}
	}
	
	return result;
}// receiveWindowResize


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
static void
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
				TerminalWindow_SetFontAndSize(dataPtr->terminalWindow, dataPtr->fontName, dataPtr->fontSize);
				
				// save the preserved dimensions
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
static void
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
		
		case kUndoables_ActionInstructionRedo:
		case kUndoables_ActionInstructionUndo:
		default:
			{
				// change the location
				MoveWindow(TerminalWindow_ReturnWindow(dataPtr->terminalWindow),
							STATIC_CAST(dataPtr->oldContentLeft, SInt16),
							STATIC_CAST(dataPtr->oldContentTop, SInt16), false/* activate */);
				
				// change the font and/or size of the window
				TerminalWindow_SetFontAndSize(dataPtr->terminalWindow, nullptr/* font name */, dataPtr->fontSize);
			}
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
static void
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
				
				// save the preserved dimensions
				dataPtr->columns = oldColumns;
				dataPtr->rows = oldRows;
			}
			break;
		}
	}
}// reverseScreenDimensionChanges


/*!
This is a standard control action procedure which,
as of MacTelnet 3.0, dynamically scrolls a terminal
window.

(2.6)
*/
static pascal void
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
		enum
		{
			kPageScrollDelayTicks = 2
		};
		TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
		TerminalScreenRef				screen = nullptr;
		TerminalViewRef					view = nullptr;
		TerminalWindowScrollBarKind		kind = kInvalidTerminalWindowScrollBarKind;
		SInt16							visibleColumnCount = 0;
		SInt16							visibleRowCount = 0;
		
		
		screen = getScrollBarScreen(ptr, inScrollBarClicked); // 3.0
		view = getScrollBarView(ptr, inScrollBarClicked); // 3.0
		kind = getScrollBarKind(ptr, inScrollBarClicked); // 3.0
		
		visibleColumnCount = Terminal_ReturnColumnCount(screen);
		visibleRowCount = Terminal_ReturnRowCount(screen);
		
		if (kind == kTerminalWindowScrollBarKindHorizontal)
		{
			switch (inPartCode)
			{
			case kControlUpButtonPart: // “up arrow” on a horizontal scroll bar means “left arrow”
				(Terminal_Result)TerminalView_ScrollColumnsTowardRightEdge(view, 1/* number of columns to scroll */);
				break;
			
			case kControlDownButtonPart: // “down arrow” on a horizontal scroll bar means “right arrow”
				(Terminal_Result)TerminalView_ScrollColumnsTowardLeftEdge(view, 1/* number of columns to scroll */);
				break;
			
			case kControlPageUpPart:
				// 3.0 - animate page scrolling a bit (users can more easily see what is happening)
				(Terminal_Result)TerminalView_ScrollColumnsTowardRightEdge(view, INTEGER_QUARTERED(visibleColumnCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollColumnsTowardRightEdge(view, INTEGER_QUARTERED(visibleColumnCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollColumnsTowardRightEdge(view, INTEGER_QUARTERED(visibleColumnCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollColumnsTowardRightEdge(view, visibleColumnCount - 3 * INTEGER_QUARTERED(visibleColumnCount));
				break;
			
			case kControlPageDownPart:
				// 3.0 - animate page scrolling a bit (users can more easily see what is happening)
				(Terminal_Result)TerminalView_ScrollColumnsTowardLeftEdge(view, INTEGER_QUARTERED(visibleColumnCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollColumnsTowardLeftEdge(view, INTEGER_QUARTERED(visibleColumnCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollColumnsTowardLeftEdge(view, INTEGER_QUARTERED(visibleColumnCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollColumnsTowardLeftEdge(view, visibleColumnCount - 3 * INTEGER_QUARTERED(visibleColumnCount));
				handlePendingUpdates();
				break;
			
			case kControlIndicatorPart:
				// 3.0 - live scrolling
				TerminalView_ScrollPixelsTo(view, GetControl32BitValue(ptr->controls.scrollBarV),
											GetControl32BitValue(ptr->controls.scrollBarH));
				break;
			
			default:
				// ???
				break;
			}
		}
		else if (kind == kTerminalWindowScrollBarKindVertical)
		{
			switch (inPartCode)
			{
			case kControlUpButtonPart:
				(Terminal_Result)TerminalView_ScrollRowsTowardBottomEdge(view, 1/* number of rows to scroll */);
				break;
			
			case kControlDownButtonPart:
				(Terminal_Result)TerminalView_ScrollRowsTowardTopEdge(view, 1/* number of rows to scroll */);
				break;
			
			case kControlPageUpPart:
				// 3.0 - animate page scrolling a bit (users can more easily see what is happening)
				(Terminal_Result)TerminalView_ScrollRowsTowardBottomEdge(view, INTEGER_QUARTERED(visibleRowCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollRowsTowardBottomEdge(view, INTEGER_QUARTERED(visibleRowCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollRowsTowardBottomEdge(view, INTEGER_QUARTERED(visibleRowCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollRowsTowardBottomEdge(view, visibleRowCount - 3 * INTEGER_QUARTERED(visibleRowCount));
				break;
			
			case kControlPageDownPart:
				// 3.0 - animate page scrolling a bit (users can more easily see what is happening)
				(Terminal_Result)TerminalView_ScrollRowsTowardTopEdge(view, INTEGER_QUARTERED(visibleRowCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollRowsTowardTopEdge(view, INTEGER_QUARTERED(visibleRowCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollRowsTowardTopEdge(view, INTEGER_QUARTERED(visibleRowCount));
				handlePendingUpdates();
				GenericThreads_DelayMinimumTicks(kPageScrollDelayTicks);
				(Terminal_Result)TerminalView_ScrollRowsTowardTopEdge(view, visibleRowCount - 3 * INTEGER_QUARTERED(visibleRowCount));
				break;
			
			case kControlIndicatorPart:
				// 3.0 - live scrolling
				TerminalView_ScrollPixelsTo(view, GetControl32BitValue(ptr->controls.scrollBarV));
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
static void
sessionStateChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inSessionSettingThatChanged,
						 void*					inEventContextPtr,
						 void*					inTerminalWindowRef)
{
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
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				
				
				TerminalWindow_SetObscured(terminalWindow, false);
				EventLoop_SelectBehindDialogWindows(ptr->window);
			}
		}
		break;
	
	case kSession_ChangeState:
		// update various GUI elements to reflect the new session state
		{
			SessionRef					session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
			
			
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
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				Session_StateAttributes		currentAttributes = Session_ReturnStateAttributes(session);
				
				
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
static OSStatus
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
Sets the standard state (for zooming) of the given
terminal window to match the size required to fit
the specified width and height in pixels.

Once this is done, you can make the window this
size by zooming “out”, or by passing "true" for
"inResizeWindow".

(3.0)
*/
static void
setStandardState	(TerminalWindowPtr	inPtr,
					 UInt16				inScreenWidthInPixels,
					 UInt16				inScreenHeightInPixels,
					 Boolean			inResizeWindow)
{
	SInt16		windowWidth = 0;
	SInt16		windowHeight = 0;
	
	
	getWindowSizeFromViewSize(inPtr, inScreenWidthInPixels, inScreenHeightInPixels, &windowWidth, &windowHeight);
	(OSStatus)inPtr->windowResizeHandler.setWindowIdealSize(windowWidth, windowHeight);
	{
		Rect		bounds;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(inPtr->window, kWindowContentRgn, &bounds);
		assert_noerr(error);
		// force the current size regardless (in reality, the event handlers
		// will be consulted so that the window size is constrained); but
		// resize at the same time, if that is applicable
		if (inResizeWindow)
		{
			bounds.right = bounds.left + windowWidth;
			bounds.bottom = bounds.top + windowHeight;
		}
		error = SetWindowBounds(inPtr->window, kWindowContentRgn, &bounds);
		assert_noerr(error);
	}
}// setStandardState


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
static void
setWarningOnWindowClose		(TerminalWindowPtr	inPtr,
							 Boolean			inCloseBoxHasDot)
{
	if (nullptr != inPtr->window)
	{
		// attach or remove an adornment in the window that shows
		// that attempting to close it will display a warning;
		// on Mac OS 8.5 and beyond, this disables any proxy icon;
		// on Mac OS X, a dot appears in the middle of the close box
		if (API_AVAILABLE(SetWindowModified))
		{
			SetWindowModified(inPtr->window, inCloseBoxHasDot);
		}
	}
}// setWarningOnWindowClose


/*!
This routine, of "SessionFactory_TerminalWindowOpProcPtr"
form, arranges the specified window in one of the “free
slots” for staggered terminal windows.

(3.0)
*/
static void
stackWindowTerminalWindowOp		(TerminalWindowRef	inTerminalWindow,
								 void*				UNUSED_ARGUMENT(inData1),
								 SInt32				UNUSED_ARGUMENT(inData2),
								 void*				UNUSED_ARGUMENT(inoutResultPtr))
{
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), inTerminalWindow);
	
	
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
static void
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
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				HIToolbarItemRef			bellItem = nullptr;
				OSStatus					error = noErr;
				
				
				bellItem = REINTERPRET_CAST(ptr->toolbarItemBell.returnHIObjectRef(), HIToolbarItemRef);
				if (nullptr != bellItem)
				{
					error = HIToolbarItemSetIconRef(bellItem, (Terminal_BellIsEnabled(screen)) ? gBellOffIcon() : gBellOnIcon());
					assert_noerr(error);
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
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				HIToolbarItemRef			relevantItem = nullptr;
				UInt16						i = 0;
				OSStatus					error = noErr;
				
				
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
			//TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef); // not needed
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				
				
				updateScrollBars(ptr);
				(OSStatus)HIViewSetNeedsDisplay(ptr->controls.scrollBarH, true);
				(OSStatus)HIViewSetNeedsDisplay(ptr->controls.scrollBarV, true);
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
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				CFStringRef					titleCFString = nullptr;
				
				
				Terminal_CopyTitleForWindow(screen, titleCFString);
				if (nullptr != titleCFString)
				{
					SetWindowTitleWithCFString(ptr->window, titleCFString);
					CFRelease(titleCFString), titleCFString = nullptr;
				}
			}
		}
		break;
	
	case kTerminal_ChangeWindowIconTitle:
		// set window’s alternate (Dock icon) title to match
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				CFStringRef					titleCFString = nullptr;
				
				
				Terminal_CopyTitleForIcon(screen, titleCFString);
				if (nullptr != titleCFString)
				{
					SetWindowAlternateTitle(ptr->window, titleCFString);
					CFRelease(titleCFString), titleCFString = nullptr;
				}
			}
		}
		break;
	
	case kTerminal_ChangeWindowMinimization:
		// minimize or restore window based on requested minimization
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), terminalWindow);
				
				
				CollapseWindow(ptr->window, Terminal_WindowIsToBeMinimized(screen));
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
static void
terminalViewStateChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inTerminalViewEvent,
							 void*					inEventContextPtr,
							 void*					inListenerContextPtr)
{
	TerminalWindowRef			ref = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
	TerminalWindowAutoLocker	ptr(gTerminalWindowPtrLocks(), ref);
	
	
	// currently, only one type of event is expected
	assert((inTerminalViewEvent == kTerminalView_EventScrolling) ||
			(inTerminalViewEvent == kTerminalView_EventFontSizeChanged));
	
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
	
	case kTerminalView_EventFontSizeChanged:
		{
			TerminalViewRef		view = REINTERPRET_CAST(inEventContextPtr, TerminalViewRef);
			SInt16				screenWidth = 0;
			SInt16				screenHeight = 0;
			
			
			// set the standard state to be large enough for the current font and size;
			// and, set window dimensions to this new standard size
			TerminalView_GetIdealSize(view, screenWidth, screenHeight);
			setStandardState(ptr, screenWidth, screenHeight, true/* resize window */);
		}
		break;
	
	default:
		// ???
		break;
	}
}// terminalViewStateChanged


/*!
Responds to a close of the Screen Dimensions dialog
box by updating the terminal area of the parent
window to reflect changes to the column and row count.

(3.1)
*/
static void
updateScreenSizeDialogCloseNotifyProc	(SizeDialog_Ref		inDialogThatClosed,
										 Boolean				inOKButtonPressed)
{
	if (inOKButtonPressed)
	{
		TerminalWindowRef	forWhichTerminalWindow = SizeDialog_ReturnParentTerminalWindow(inDialogThatClosed);
		
		
		if (nullptr != forWhichTerminalWindow)
		{
			UInt16		columns = 0;
			UInt16		rows = 0;
			
			
			// save the changes
			installUndoScreenDimensionChanges(forWhichTerminalWindow);
			SizeDialog_GetDisplayedDimensions(inDialogThatClosed, columns, rows);
			TerminalWindow_SetScreenDimensions(forWhichTerminalWindow, columns, rows, true/* recordable */);
		}
	}
}// updateScreenSizeDialogCloseNotifyProc


/*!
Updates the values and view sizes of the scroll bars
to show the position and percentage of the total
screen area that is currently visible in the window.

(3.0)
*/
static void
updateScrollBars	(TerminalWindowPtr		inPtr)
{
	TerminalViewRef		view = getActiveView(inPtr);
	
	
	assert(nullptr != view);
	{
		// update the scroll bars to reflect the contents of the selected view
		HIViewRef				scrollBarView = nullptr;
		UInt32					scrollVStartView = 0;
		UInt32					scrollVPastEndView = 0;
		UInt32					scrollVRangeMinimum = 0;
		UInt32					scrollVRangePastMaximum = 0;
		TerminalView_Result		rangeResult = kTerminalView_ResultOK;
		
		
		// use the maximum possible screen size for the maximum resize limits
		rangeResult = TerminalView_GetRange(view, kTerminalView_RangeCodeScrollRegionV,
											scrollVStartView, scrollVPastEndView);
		assert(kTerminalView_ResultOK == rangeResult);
		rangeResult = TerminalView_GetRange(view, kTerminalView_RangeCodeScrollRegionVMaximum,
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
		SetControlViewSize(scrollBarView, scrollVPastEndView - scrollVStartView);
		
		// set the scroll bar view size to be the ratio
	#if 0
		{
			Fixed		fixedValueRange;
			Fixed		fixedVisibleToMaxRatio;
			SInt32		maximumValue = 0;
			SInt32		minimumValue = 0;
			
			
			scrollBarView = inPtr->controls.scrollBarH;
			maximumValue = GetControl32BitMaximum(scrollBarView);
			minimumValue = GetControl32BitMinimum(scrollBarView);
			fixedValueRange = Long2Fix(maximumValue - minimumValue);
			fixedVisibleToMaxRatio = FixRatio(visibleBounds.bottom - visibleBounds.top, maximumValue - minimumValue);
			SetControlViewSize(scrollBarView, STATIC_CAST(FixRound(FixMul(fixedValueRange, fixedVisibleToMaxRatio)), SInt32));
			
			scrollBarView = inPtr->controls.scrollBarV;
			maximumValue = GetControl32BitMaximum(scrollBarView);
			minimumValue = GetControl32BitMinimum(scrollBarView);
			fixedValueRange = Long2Fix(maximumValue - minimumValue);
			fixedVisibleToMaxRatio = FixRatio(scrollVRangePastMaximum - scrollVRangeMinimum, maximumValue - minimumValue);
			SetControlViewSize(scrollBarView, STATIC_CAST(FixRound(FixMul(fixedValueRange, fixedVisibleToMaxRatio)), SInt32));
		}
	#endif
	}
}// updateScrollBars

// BELOW IS REQUIRED NEWLINE TO END FILE
