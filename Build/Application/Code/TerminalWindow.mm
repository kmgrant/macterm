/*!	\file TerminalWindow.mm
	\brief The most common type of window, used to hold
	terminal views and scroll bars for a session.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#import "TerminalWindow.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <map>
#import <random>
#import <vector>

// UNIX includes
extern "C"
{
#	include <pthread.h>
#	include <strings.h>
}

// Mac includes
#import <Carbon/Carbon.h> // for old APIs (TEMPORARY; deprecated)
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <CFRetainRelease.h>
#import <CGContextSaveRestore.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <ContextSensitiveMenu.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceTracker.template.h>
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <Registrar.template.h>
#import <SoundSystem.h>
#import <Undoables.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "EventLoop.h"
#import "FindDialog.h"
#import "GenericDialog.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "Preferences.h"
#import "PrefPanelFormats.h"
#import "PrefPanelTerminals.h"
#import "PrefPanelTranslations.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalToolbar.objc++.h"
#import "TerminalView.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

/*!
Named flags, for clarity in the methods that use them.
*/
enum My_FullScreenState : UInt8
{
	kMy_FullScreenStateCompleted	= true,
	kMy_FullScreenStateInProgress	= false
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

} // anonymous namespace

#pragma mark Types

/*!
Private properties.
*/
@interface TerminalWindow_Controller () //{

// accessors
	@property (assign) NSRect
	preFullScreenFrame;
	@property (assign) TerminalToolbar_Delegate*
	toolbarDelegate;

@end //}


/*!
The private class interface.
*/
@interface TerminalWindow_Controller (TerminalWindow_ControllerInternal) //{

// notifications
	- (void)
	toolbarDidChangeVisibility:(NSNotification*)_;

@end //}


/*!
Private properties.
*/
@interface TerminalWindow_InfoBubble () //{

// accessors
	@property (assign) NSPoint
	idealWindowRelativeAnchorPoint;
	@property (strong) Popover_Window*
	infoWindow;
	@property (strong) NSView*
	paddingView;
	@property (assign) PopoverManager_Ref
	popoverMgr;
	@property (strong) NSTextField*
	textView;

@end //}


/*!
The private class interface.
*/
@interface TerminalWindow_InfoBubble (TerminalWindow_InfoBubbleInternal) //{

@end //}


/*!
Private properties.
*/
@interface TerminalWindow_RootView () //{

// accessors
	@property (strong) NSMutableArray*
	scrollableTerminalRootViews;

@end //}


/*!
Private properties.
*/
@interface TerminalWindow_RootVC () //{

// accessors
	@property (strong) NSMutableArray*
	terminalScrollControllers;

@end //}


/*!
The private class interface.
*/
@interface TerminalWindow_RootVC (TerminalWindow_RootVCInternal) //{

@end //}


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
	
	bool
	isCocoa () const;
	
	My_RefRegistrar				refValidator;				// ensures this reference is recognized as a valid one
	TerminalWindowRef			selfRef;					// redundant reference to self, for convenience
	
	ListenerModel_Ref			changeListenerModel;		// who to notify for various kinds of changes to this terminal data
	
	TerminalWindow_Controller*	windowController;			// controller for the main NSWindow (responds to events, etc.)
	NSWindow*					window;						// the Cocoa window reference for the terminal window
	Float32						tabOffsetInPixels;			// used to position the tab drawer, if any
	Float32						tabSizeInPixels;			// used to position and size a tab drawer, if any
	TerminalView_DisplayMode	preResizeViewDisplayMode;	// stored in case user invokes option key variation on resize
	
	struct
	{
		Boolean						isOn;		// temporary flag to track full-screen mode (under Cocoa this will be easier to determine from the window)
		Boolean						isUsingOS;	// temporary flag; tracks windows that are currently Full Screen in system style so they can transition back in the same style
		CGFloat						oldFontSize;		// font size prior to full-screen
		Rect						oldContentBounds;	// old window boundaries, content area
		TerminalView_DisplayMode	oldMode;			// previous terminal resize effect
		TerminalView_DisplayMode	newMode;			// full-screen terminal resize effect
	} fullScreen;
	
	Boolean						isObscured;				// is the window hidden, via a command in the Window menu?
	Boolean						isDead;					// is the window title flagged to indicate a disconnected session?
	Boolean						isLEDOn[4];				// true only if this terminal light is lit
	Boolean						viewSizeIndependent;	// true only temporarily, to handle transitional cases such as full-screen mode
	Preferences_ContextWrap		recentSheetContext;		// defined temporarily while a Preferences-dependent sheet (such as screen size) is up
	My_SheetType				sheetType;				// if a sheet is active, this is a hint as to what settings can be put in the context
	FindDialog_Ref				searchDialog;			// retains the user interface for finding text in the terminal buffer
	FindDialog_Options			recentSearchOptions;	// the options used during the last search in the dialog
	CFRetainRelease				recentSearchStrings;	// CFMutableArrayRef; the CFStrings used in searches since this window was opened
	CFRetainRelease				baseTitleString;		// user-provided title string; may be adorned prior to becoming the window title
	CFRetainRelease				preResizeTitleString;	// used to save the old title during resizes, where the title is overwritten
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
	CGFloat					fontSize;			//!< the old font size (ignored if "undoFontSize" is false)
	CFRetainRelease			fontName;			//!< the old font (ignored if "undoFont" is false)
};
typedef UndoDataFontSizeChanges*		UndoDataFontSizeChangesPtr;

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

#pragma mark Internal Method Prototypes
namespace {

void					calculateIndexedWindowPosition	(My_TerminalWindowPtr, UInt16, UInt16, CGPoint*/* top-left origin */);
void					calculateWindowFrameCocoa		(My_TerminalWindowPtr, UInt16*, UInt16*, NSRect*);
void					changeNotifyForTerminalWindow	(My_TerminalWindowPtr, TerminalWindow_Change, void*);
TerminalScreenRef		getActiveScreen					(My_TerminalWindowPtr);
TerminalViewRef			getActiveView					(My_TerminalWindowPtr);
void					getWindowSizeFromViewSize		(My_TerminalWindowPtr, SInt16, SInt16, SInt16*, SInt16*);
void					handleFindDialogClose			(FindDialog_Ref);
void					installUndoFontSizeChanges		(TerminalWindowRef, Boolean, Boolean);
void					installUndoScreenDimensionChanges	(TerminalWindowRef);
bool					lessThanIfGreaterAreaCocoa		(NSWindow*, NSWindow*);
UInt16					returnScrollBarWidth			(My_TerminalWindowPtr);
UInt16					returnStatusBarHeight			(My_TerminalWindowPtr);
UInt16					returnToolbarHeight				(My_TerminalWindowPtr);
void					reverseFontChanges				(Undoables_ActionInstruction, Undoables_ActionRef, void*);
void					reverseScreenDimensionChanges	(Undoables_ActionInstruction, Undoables_ActionRef, void*);
void					sessionStateChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void					setCocoaWindowFullScreenIcon	(NSWindow*, Boolean);
void					setScreenPreferences			(My_TerminalWindowPtr, Preferences_ContextRef, Boolean = false);
void					setStandardState				(My_TerminalWindowPtr, UInt16, UInt16, Boolean = false);
void					setUpForFullScreenModal			(My_TerminalWindowPtr, Boolean, Boolean, My_FullScreenState);
void					setViewFormatPreferences		(My_TerminalWindowPtr, Preferences_ContextRef);
void					setViewSizeIndependentFromWindow(My_TerminalWindowPtr, Boolean);
void					setViewTranslationPreferences	(My_TerminalWindowPtr, Preferences_ContextRef);
void					setWarningOnWindowClose			(My_TerminalWindowPtr, Boolean);
void					setWindowAndTabTitle			(My_TerminalWindowPtr, CFStringRef);
void					setWindowToIdealSizeForDimensions	(My_TerminalWindowPtr, UInt16, UInt16, Boolean = false);
void					setWindowToIdealSizeForFont		(My_TerminalWindowPtr);
void					sheetClosed						(GenericDialog_Ref, Boolean);
Preferences_ContextRef	sheetContextBegin				(My_TerminalWindowPtr, Quills::Prefs::Class, My_SheetType);
void					sheetContextEnd					(My_TerminalWindowPtr);
void					terminalStateChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void					terminalViewStateChanged		(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void					updateScrollBars				(My_TerminalWindowPtr);

} // anonymous namespace

#pragma mark Variables
namespace {

My_TerminalWindowByNSWindow&	gTerminalWindowRefsByNSWindow ()		{ static My_TerminalWindowByNSWindow x; return x; }
My_RefTracker&					gTerminalWindowValidRefs ()		{ static My_RefTracker x; return x; }
My_TerminalWindowPtrLocker&		gTerminalWindowPtrLocks ()		{ static My_TerminalWindowPtrLocker x; return x; }

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

(2016.03)
*/
TerminalWindowRef
TerminalWindow_New		(Preferences_ContextRef		inTerminalInfoOrNull,
						 Preferences_ContextRef		inFontInfoOrNull,
						 Preferences_ContextRef		inTranslationOrNull,
						 Boolean					inNoStagger)
{
	TerminalWindowRef	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_TerminalWindow(inTerminalInfoOrNull, inFontInfoOrNull, inTranslationOrNull,
														inNoStagger),
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
	else if (false == TerminalWindow_IsValid(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValueAddress, "attempt to dispose of invalid terminal window", *inoutRefPtr);
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
Returns the base name of the specified terminal window,
without any special adornments added by the Terminal Window
module.  (To copy the complete title, just ask the OS.)

(4.0)
*/
void
TerminalWindow_CopyWindowTitle	(TerminalWindowRef	inRef,
								 CFStringRef&		outName)
{
	outName = nullptr;
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		if (ptr->baseTitleString.exists())
		{
			outName = CFStringCreateCopy(kCFAllocatorDefault, ptr->baseTitleString.returnCFStringRef());
		}
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to copy title of invalid terminal window", inRef);
	}
}// CopyWindowTitle


/*!
Displays an interface for the user to customize formatting,
such as the font and color settings.

This is called by event handlers (in response to menu
commands, etc.) and should not need to be called directly.

(2018.03)
*/
void
TerminalWindow_DisplayCustomFormatUI	(TerminalWindowRef		inRef)
{
	if (TerminalWindow_IsValid(inRef))
	{
		// display a format customization dialog
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		Preferences_ContextRef			temporaryContext = sheetContextBegin(ptr, Quills::Prefs::FORMAT,
																				kMy_SheetTypeFormat);
		
		
		if (nullptr == temporaryContext)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
		}
		else
		{
			GenericDialog_Wrap				dialog;
			PrefPanelFormats_ViewManager*	embeddedPanel = [[PrefPanelFormats_ViewManager alloc] init];
			CFRetainRelease					addToPrefsString(UIStrings_ReturnCopy(kUIStrings_PreferencesWindowAddToFavoritesButton),
																CFRetainRelease::kAlreadyRetained);
			CFRetainRelease					cancelString(UIStrings_ReturnCopy(kUIStrings_ButtonCancel),
															CFRetainRelease::kAlreadyRetained);
			CFRetainRelease					okString(UIStrings_ReturnCopy(kUIStrings_ButtonOK),
														CFRetainRelease::kAlreadyRetained);
			
			
			// display the sheet
			dialog = GenericDialog_Wrap(GenericDialog_New(TerminalWindow_ReturnNSWindow(inRef).contentView,
															embeddedPanel, temporaryContext),
										GenericDialog_Wrap::kAlreadyRetained);
			[embeddedPanel release], embeddedPanel = nil; // panel is retained by the call above
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton1, okString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton1,
												^{ sheetClosed(dialog.returnRef(), true/* is OK */); });
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton2, cancelString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton2,
												^{ sheetClosed(dialog.returnRef(), false/* is OK */); });
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton3, addToPrefsString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton3,
												^{
													Preferences_TagSetRef	tagSet = PrefPanelFormats_NewTagSet();
													
													
													PrefsWindow_AddCollection(temporaryContext, tagSet,
																				kCommandDisplayPrefPanelFormats);
													Preferences_ReleaseTagSet(&tagSet);
												});
			GenericDialog_SetImplementation(dialog.returnRef(), inRef);
			// TEMPORARY; maybe TerminalWindow_Retain/Release concept needs
			// to be implemented and called here; for now, assume that the
			// terminal window will remain valid as long as the dialog exists
			// (that ought to be the case)
			GenericDialog_Display(dialog.returnRef(), true/* animated */, ^{}); // retains dialog until it is dismissed
		}
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to display custom format UI on invalid terminal window", inRef);
	}
}// DisplayCustomFormatUI


/*!
Displays an interface for the user to customize the terminal
screen dimensions and scrollback settings.

This is called by event handlers (in response to menu
commands, etc.) and should not need to be called directly.

(2018.03)
*/
void
TerminalWindow_DisplayCustomScreenSizeUI		(TerminalWindowRef		inRef)
{
	if (TerminalWindow_IsValid(inRef))
	{
		// display a screen size customization dialog
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		Preferences_ContextRef			temporaryContext = sheetContextBegin(ptr, Quills::Prefs::TERMINAL,
																				kMy_SheetTypeScreenSize);
		
		
		if (nullptr == temporaryContext)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
		}
		else
		{
			GenericDialog_Wrap						dialog;
			PrefPanelTerminals_ScreenViewManager*	embeddedPanel = [[PrefPanelTerminals_ScreenViewManager alloc] init];
			CFRetainRelease							cancelString(UIStrings_ReturnCopy(kUIStrings_ButtonCancel),
																	CFRetainRelease::kAlreadyRetained);
			CFRetainRelease							okString(UIStrings_ReturnCopy(kUIStrings_ButtonOK),
																CFRetainRelease::kAlreadyRetained);
			
			
			// display the sheet
			dialog = GenericDialog_Wrap(GenericDialog_New(TerminalWindow_ReturnNSWindow(inRef).contentView,
															embeddedPanel, temporaryContext),
										GenericDialog_Wrap::kAlreadyRetained);
			[embeddedPanel release], embeddedPanel = nil; // panel is retained by the call above
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton1, okString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton1,
												^{ sheetClosed(dialog.returnRef(), true/* is OK */); });
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton2, cancelString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton2,
												^{ sheetClosed(dialog.returnRef(), false/* is OK */); });
			GenericDialog_SetImplementation(dialog.returnRef(), inRef);
			// TEMPORARY; maybe TerminalWindow_Retain/Release concept needs
			// to be implemented and called here; for now, assume that the
			// terminal window will remain valid as long as the dialog exists
			// (that ought to be the case)
			GenericDialog_Display(dialog.returnRef(), true/* animated */, ^{}); // retains dialog until it is dismissed
		}
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to display custom screen size UI on invalid terminal window", inRef);
	}
}// DisplayCustomScreenSizeUI


/*!
Displays an interface for the user to customize the text
encoding.

This is called by event handlers (in response to menu
commands, etc.) and should not need to be called directly.

(2018.03)
*/
void
TerminalWindow_DisplayCustomTranslationUI	(TerminalWindowRef		inRef)
{
	if (TerminalWindow_IsValid(inRef))
	{
		// display a translation customization dialog
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		Preferences_ContextRef			temporaryContext = sheetContextBegin(ptr, Quills::Prefs::TRANSLATION,
																				kMy_SheetTypeTranslation);
		
		
		if (nullptr == temporaryContext)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
		}
		else
		{
			GenericDialog_Wrap					dialog;
			PrefPanelTranslations_ViewManager*	embeddedPanel = [[PrefPanelTranslations_ViewManager alloc] init];
			CFRetainRelease						addToPrefsString(UIStrings_ReturnCopy(kUIStrings_PreferencesWindowAddToFavoritesButton),
																	CFRetainRelease::kAlreadyRetained);
			CFRetainRelease						cancelString(UIStrings_ReturnCopy(kUIStrings_ButtonCancel),
																CFRetainRelease::kAlreadyRetained);
			CFRetainRelease						okString(UIStrings_ReturnCopy(kUIStrings_ButtonOK),
															CFRetainRelease::kAlreadyRetained);
			
			
			// display the sheet
			dialog = GenericDialog_Wrap(GenericDialog_New(TerminalWindow_ReturnNSWindow(inRef).contentView,
															embeddedPanel, temporaryContext),
										GenericDialog_Wrap::kAlreadyRetained);
			[embeddedPanel release], embeddedPanel = nil; // panel is retained by the call above
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton1, okString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton1,
												^{ sheetClosed(dialog.returnRef(), true/* is OK */); });
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton2, cancelString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton2,
												^{ sheetClosed(dialog.returnRef(), false/* is OK */); });
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton3, addToPrefsString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton3,
												^{
													Preferences_TagSetRef	tagSet = PrefPanelTranslations_NewTagSet();
													
													
													PrefsWindow_AddCollection(temporaryContext, tagSet,
																				kCommandDisplayPrefPanelTranslations);
													Preferences_ReleaseTagSet(&tagSet);
												});
			GenericDialog_SetImplementation(dialog.returnRef(), inRef);
			// TEMPORARY; maybe TerminalWindow_Retain/Release concept needs
			// to be implemented and called here; for now, assume that the
			// terminal window will remain valid as long as the dialog exists
			// (that ought to be the case)
			GenericDialog_Display(dialog.returnRef(), true/* animated */, ^{}); // retains dialog until it is dismissed
		}
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to display custom translation UI on invalid terminal window", inRef);
	}
}// DisplayCustomTranslationUI


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
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		if (nullptr == ptr->searchDialog)
		{
			ptr->searchDialog = FindDialog_New(inRef, handleFindDialogClose,
												ptr->recentSearchStrings.returnCFMutableArrayRef(),
												ptr->recentSearchOptions);
		}
		
		// display a text search dialog (automatically closed when the user clicks a button)
		FindDialog_Display(ptr->searchDialog);
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to display text search dialog on invalid terminal window", inRef);
	}
}// DisplayTextSearchDialog


/*!
Makes a terminal window the target of keyboard input, but does
not force it to be in front.

See also TerminalWindow_IsFocused().

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.

(4.0)
*/
void
TerminalWindow_Focus	(TerminalWindowRef	inRef)
{
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		[ptr->window makeKeyWindow];
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to focus invalid terminal window", inRef);
	}
}// Focus


/*!
Performs the specified operation on every terminal view
in the window.  The list must NOT change during iteration.

The iteration terminates early if the block sets its
stop-flag parameter.

\retval kTerminalWindow_ResultOK
if there are no errors

\retval kTerminalWindow_ResultInvalidReference
if the specified workspace is unrecognized

(2018.12)
*/
TerminalWindow_Result
TerminalWindow_ForEachTerminalView	(TerminalWindowRef					inRef,
									 TerminalWindow_TerminalViewBlock	inBlock)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	TerminalWindow_Result			result = kTerminalWindow_ResultOK;
	Boolean							stopFlag = false;
	
	
	if (nullptr == ptr)
	{
		result = kTerminalWindow_ResultInvalidReference;
	}
	else
	{
		// traverse the list
		for (auto terminalViewRef : ptr->allViews)
		{
			inBlock(terminalViewRef, stopFlag);
			if (stopFlag)
			{
				break;
			}
		}
	}
	
	return result;
}// ForEachTerminalView


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
								 CFStringRef*		outFontFamilyNameOrNull,
								 CGFloat*			outFontSizeOrNull)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	TerminalView_GetFontAndSize(getActiveView(ptr)/* TEMPORARY */, outFontFamilyNameOrNull, outFontSizeOrNull);
}// GetFontAndSize


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
Returns "true" only if the specified window currently has the
keyboard focus.

See also TerminalWindow_Focus().

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.

(4.0)
*/
Boolean
TerminalWindow_IsFocused	(TerminalWindowRef	inRef)
{
	Boolean		result = false;
	
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		result = (YES == [ptr->window isKeyWindow]);
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to check “is focused” property of invalid terminal window", inRef);
	}
	return result;
}// IsFocused


/*!
Returns "true" only if the specified terminal window is currently
taking over its entire display in a special mode.  If true, this
does not guarantee that there are no other windows in a full-screen
mode.

(4.1)
*/
Boolean
TerminalWindow_IsFullScreen		(TerminalWindowRef	inRef)
{
	Boolean		result = false;
	
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		result = ptr->fullScreen.isOn;
	}
	return result;
}// IsFullScreen


/*!
Returns "true" if any terminal window is full-screen.  If the
user has enabled the OS full-screen mechanism, some windows
could be full-screen without preventing access to the rest of
the application; thus, “mode” in this context does not
necessarily mean the application can do nothing else.

See also TerminalWindow_IsFullScreen(), which checks to see if
a particular window is full-screen.

(4.1)
*/
Boolean
TerminalWindow_IsFullScreenMode ()
{
	__block Boolean		result = false;
	
	
	// NOTE: although this could be implemented with a simple
	// counter, that is vulnerable to unexpected problems (e.g.
	// a window somehow closing where the close notification
	// is missed and the counter is out of date); this is a
	// small-N linear search that can be changed later if
	// necessary
	SessionFactory_ForEachTerminalWindow
	(^(TerminalWindowRef	inTerminalWindow,
	   Boolean&				outStop)
	{
		if (TerminalWindow_IsFullScreen(inTerminalWindow))
		{
			result = true;
			outStop = YES;
		}
	});
	
	return result;
}// IsFullScreenMode


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
	Boolean		result = false;
	
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		result = ptr->isObscured;
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to check “obscured” property of invalid terminal window", inRef);
	}
	return result;
}// IsObscured


/*!
Returns "true" only if the specified terminal’s window is
currently displayed as a tab or allowing new tabbed windows.

(4.0)
*/
Boolean
TerminalWindow_IsTab	(TerminalWindowRef	inRef)
{
	Boolean		result = false;
	
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		if (@available(macOS 10.12, *))
		{
			if ((ptr->window.tabbedWindows.count > 1) ||
				(NSWindowTabbingModeDisallowed != ptr->window.tabbingMode))
			{
				result = true;
			}
		}
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to check “is tab” property of invalid terminal window", inRef);
	}
	return result;
}// IsTab


/*!
Returns "true" only if the specified terminal window has
not been destroyed with TerminalWindow_Dispose(), and is
not in the process of being destroyed.

Most of the time, checking for a null reference is enough,
and efficient; this check may be slower, but is important
if you are handling something indirectly or asynchronously
(where a terminal window could have been destroyed at any
time).

(4.1)
*/
Boolean
TerminalWindow_IsValid	(TerminalWindowRef	inRef)
{
	Boolean		result = ((nullptr != inRef) && (gTerminalWindowValidRefs().find(inRef) != gTerminalWindowValidRefs().end()));
	
	
	return result;
}// IsValid


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
active non-floating window, or nullptr if there is none.

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
	
	
	return result;
}// ReturnFromMainWindow


/*!
Returns the Terminal Window associated with the window that has
keyboard focus, or nullptr if there is none.  In particular, if a
floating window is focused, this will always return nullptr.

Use this in cases where the target of keyboard input absolutely
must be a terminal, and cannot be a floating non-terminal window.

(4.0)
*/
TerminalWindowRef
TerminalWindow_ReturnFromKeyWindow ()
{
	NSWindow*			activeWindow = [NSApp keyWindow];
	TerminalWindowRef	result = [activeWindow terminalWindowRef];
	
	
	return result;
}// ReturnFromKeyWindow


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
	NSWindow*	result = nil;
	
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		result = ptr->window;
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to find Cocoa window of invalid terminal window", inRef);
	}
	return result;
}// ReturnNSWindow


/*!
Returns a reference to the virtual terminal that has most
recently had keyboard focus in the given terminal window.
Thus, a valid reference is returned even if no terminal
screen control has the keyboard focus.

WARNING:	MacTerm is going to change in the future to
			support multiple screens per window.  Be sure
			to use TerminalWindow_ForEachTerminalView()
			instead of this routine if it is appropriate
			to iterate over all screens in a window.

(3.0)
*/
TerminalScreenRef
TerminalWindow_ReturnScreenWithFocus	(TerminalWindowRef	inRef)
{
	TerminalScreenRef	result = nullptr;
	
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		result = getActiveScreen(ptr);
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to find focused screen of invalid terminal window", inRef);
	}
	return result;
}// ReturnScreenWithFocus


/*!
Returns a reference to the screen view that has most
recently had keyboard focus in the given terminal window.
Thus, a valid reference is returned even if no terminal
screen control has the keyboard focus.

WARNING:	MacTerm is going to change in the future to
			support multiple views per window.  Be sure
			to use TerminalWindow_ForEachTerminalView()
			instead of this routine if it is appropriate
			to iterate over all views in a window.

(3.0)
*/
TerminalViewRef
TerminalWindow_ReturnViewWithFocus		(TerminalWindowRef	inRef)
{
	TerminalViewRef		result = nullptr;
	
	
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		result = getActiveView(ptr);
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to find focused view of invalid terminal window", inRef);
	}
	return result;
}// ReturnViewWithFocus


/*!
Puts a terminal window in front of other windows.  For
convenience, if "inFocus" is true, TerminalWindow_Focus() is
also called (which is commonly required at the same time).

See also TerminalWindow_Focus() and TerminalWindow_IsFocused().

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.

(4.0)
*/
void
TerminalWindow_Select	(TerminalWindowRef	inRef,
						 Boolean			inFocus)
{
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		[ptr->window orderFront:nil];
		if (inFocus)
		{
			TerminalWindow_Focus(inRef);
		}
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to select invalid terminal window", inRef);
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
								 CFStringRef			inFontFamilyNameOrNull,
								 CGFloat				inFontSizeOrZero)
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
Makes the text slightly bigger or smaller than it is
currently.  Note that in this initial implementation,
floating-point values are converted to integers (later
they will be able to offer more fine-grained control).

If "inAbsoluteLimit" is nonzero, it is used to prevent
the given adjustment if it goes too far.  Use a small
number when "inDeltaFontSize" is negative and a larger
number when "inDeltaFontSize" is positive.

Returns true only if the change occurred.  If the
change occurs and "inAllowUndo" is true, an action is
added so that the user can Undo the size change later.

(2018.03)
*/
Boolean
TerminalWindow_SetFontRelativeSize	(TerminalWindowRef		inRef,
									 Float32				inDeltaFontSize,
									 Float32				inAbsoluteLimitOrZero,
									 Boolean				inAllowUndo)
{
	Float32		proposedSize = 0;
	CGFloat		fontSize = 0;
	Boolean		preventChange = false;
	Boolean		result = false;
	
	
	TerminalWindow_GetFontAndSize(inRef, nullptr/* font */, &fontSize);
	proposedSize = (fontSize + inDeltaFontSize);
	if (0 != inAbsoluteLimitOrZero)
	{
		if (inDeltaFontSize > 0)
		{
			preventChange = (fontSize >= inAbsoluteLimitOrZero);
		}
		else
		{
			preventChange = (fontSize <= inAbsoluteLimitOrZero);
		}
	}
	
	if (false == preventChange)
	{
		result = true;
		
		if (inAllowUndo)
		{
			installUndoFontSizeChanges(inRef, false/* undo font */, true/* undo font size */);
		}
		
		// set the window size to fit the new font size optimally
		TerminalWindow_SetFontAndSize(inRef, nullptr/* font */, STATIC_CAST(proposedSize, UInt16));
	}
	
	return result;
}// SetFontRelativeSize


/*!
Temporary, controlled by the Session in response to changes
in user preferences.  Updates all windows appropriately to
let the user enter or exit Full Screen with the standard
mechanism.  This should not be called except as a side effect
of preferences changes.

(4.1)
*/
void
TerminalWindow_SetFullScreenIconsEnabled	(Boolean	inAllTerminalWindowsHaveFullScreenIcons)
{
	My_TerminalWindowByNSWindow::const_iterator		toPair;
	My_TerminalWindowByNSWindow::const_iterator		endPairs(gTerminalWindowRefsByNSWindow().end());
	
	
	for (toPair = gTerminalWindowRefsByNSWindow().begin(); toPair != endPairs; ++toPair)
	{
		setCocoaWindowFullScreenIcon(toPair->first, inAllTerminalWindowsHaveFullScreenIcons);
	}
}// SetFullScreenIconsEnabled


/*!
Renames a terminal window’s minimized Dock tile, notifying
listeners that the window title has changed.

(3.0)
*/
void
TerminalWindow_SetIconTitle		(TerminalWindowRef	inRef,
								 CFStringRef		inName)
{
@autoreleasepool {
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	[ptr->window setMiniwindowTitle:(NSString*)inName];
	changeNotifyForTerminalWindow(ptr, kTerminalWindow_ChangeIconTitle, ptr->selfRef/* context */);
}// @autoreleasepool
}// SetIconTitle


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
@autoreleasepool {
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
}// @autoreleasepool
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
									 UInt16				inNewRowCount)
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
Set to "true" to show a terminal window, and "false" to hide it.

This is a TEMPORARY API that should be used in any code that
cannot use TerminalWindow_ReturnNSWindow() to manipulate the
Cocoa window directly.

(4.0)
*/
void
TerminalWindow_SetVisible	(TerminalWindowRef	inRef,
							 Boolean			inIsVisible)
{
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		if (inIsVisible)
		{
			[ptr->window orderFront:nil];
		}
		else
		{
			[ptr->window orderOut:nil];
		}
	}
	else
	{
		Console_Warning(Console_WriteValueAddress, "attempt to display invalid terminal window", inRef);
	}
}// SetVisible


/*!
Renames a terminal window, notifying listeners that the
window title has changed.

The value of "inName" can be nullptr if you want the current
base title to be unchanged, but you want adornments to be
evaluated again (an updated session status, for instance).

See also TerminalWindow_CopyWindowTitle().

(3.0)
*/
void
TerminalWindow_SetWindowTitle	(TerminalWindowRef	inRef,
								 CFStringRef		inName)
{
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
	
	
	if (nullptr != inName)
	{
		ptr->baseTitleString.setWithRetain(inName);
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
												CFRetainRelease::kAlreadyRetained);
			
			
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
Rearranges all terminal windows so that their top-left
corners form an invisible, diagonal line.  The effect
is also animated, showing each window sliding into its
new position.

If there are several Spaces defined, this routine only
operates on windows in the current Space.  If windows
span multiple displays however, they will be arranged
separately on each display.

(3.0)
*/
void
TerminalWindow_StackWindows ()
{
	typedef std::vector< NSWindow* >					My_CocoaWindowList;
	typedef std::map< NSScreen*, My_CocoaWindowList >	My_CocoaWindowsByDisplay; // which windows are on which devices?
	
	NSArray*					currentSpaceWindowNumbers = [NSWindow windowNumbersWithOptions:0];
	NSWindow*					activeWindow = [NSApp mainWindow];
	My_CocoaWindowsByDisplay	cocoaWindowsByDisplay;
	
	
	// first determine which windows are on each display
	for (NSNumber* currentWindowNumber in currentSpaceWindowNumbers)
	{
		NSInteger			windowNumber = [currentWindowNumber integerValue];
		NSWindow*			window = [NSApp windowWithWindowNumber:windowNumber];
		TerminalWindowRef	terminalWindow = [window terminalWindowRef];
		
		
		if (nil != terminalWindow)
		{
			NSWindow* const			kWindow = TerminalWindow_ReturnNSWindow(terminalWindow);
			My_CocoaWindowList&		windowsOnThisDisplay = cocoaWindowsByDisplay[window.screen];
			
			
			if (windowsOnThisDisplay.empty())
			{
				// the first time a display’s window list is seen, allocate
				// an approximately-correct vector size up front (this value
				// will only be exactly right on single-display systems)
				windowsOnThisDisplay.reserve([currentSpaceWindowNumbers count]);
			}
			windowsOnThisDisplay.push_back(kWindow);
		}
		else
		{
			// not a terminal; ignore
		}
	}
	
	// sort windows by largest area arbitrarily to minimize the chance
	// that a window will be completely hidden by the stacking of other
	// windows on its display
	for (auto& displayWindowPair : cocoaWindowsByDisplay)
	{
		My_CocoaWindowList&		windowsOnThisDisplay = displayWindowPair.second;
		
		
		std::sort(windowsOnThisDisplay.begin(), windowsOnThisDisplay.end(), lessThanIfGreaterAreaCocoa);
	}
	
	// for each display, stack windows separately
	for (auto displayWindowPair : cocoaWindowsByDisplay)
	{
		My_CocoaWindowList const&		windowsOnThisDisplay = displayWindowPair.second;
		auto							toWindow = windowsOnThisDisplay.begin();
		auto							endWindows = windowsOnThisDisplay.end();
		UInt16							staggerListIndexHint = 0;
		UInt16							localWindowIndexHint = 0;
		UInt16							transitioningWindowCount = 0;
		
		
		for (; toWindow != endWindows; ++toWindow, ++localWindowIndexHint)
		{
			// arbitrary limit: only animate a few windows (asynchronously)
			// per display, moving the rest into position immediately
			NSWindow* const					kNSWindow = *toWindow;
			TerminalWindowRef const			kTerminalWindow = [kNSWindow terminalWindowRef];
			Boolean const					kTooManyWindows = (transitioningWindowCount > 3/* arbitrary */);
			My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), kTerminalWindow);
			NSRect							originalFrame = kNSWindow.frame;
			NSRect							windowFrame = kNSWindow.frame;
			NSRect							screenFrame = kNSWindow.screen.frame;
			
			
			++transitioningWindowCount;
			
			// NOTE: on return, "windowFrame" is updated again
			calculateWindowFrameCocoa(ptr, &staggerListIndexHint, &localWindowIndexHint, &windowFrame);
			
			// if a window’s current position places it entirely on-screen
			// and its “stacked” location would put it partially off-screen,
			// do not relocate the window (presumably it was better before!)
			if ((false == CGRectContainsRect(NSRectToCGRect(screenFrame), NSRectToCGRect(originalFrame))) ||
				CGRectContainsRect(NSRectToCGRect(screenFrame), NSRectToCGRect(windowFrame)))
			{
				// select each window as it is stacked so that keyboard cycling
				// will be in sync with the new order of windows
				[kNSWindow orderFront:nil];
				
				if (kTooManyWindows)
				{
					// move the window immediately without animation
					[kNSWindow setFrame:windowFrame display:NO];
				}
				else
				{
					// do a cool slide animation to move the window into place
					[kNSWindow setFrame:windowFrame display:NO animate:YES];
				}
			}
		}
	}
	
	// restore original Z-order of active window
	[activeWindow makeKeyAndOrderFront:nil];
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
	if (TerminalWindow_IsValid(inRef))
	{
		OSStatus						error = noErr;
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		// add a listener to the specified target’s listener model for the given setting change
		error = ListenerModel_AddListenerForEvent(ptr->changeListenerModel, inForWhatChange, inListener);
		if (noErr != error)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to start monitoring terminal window change", inForWhatChange);
			Console_Warning(Console_WriteValue, "monitor installation error", error);
		}
	}
	else
	{
		Console_Warning(Console_WriteValueFourChars, "attempt to start monitoring invalid terminal window, event", inForWhatChange);
	}
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
	if (TerminalWindow_IsValid(inRef))
	{
		My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), inRef);
		
		
		// add a listener to the specified target’s listener model for the given setting change
		ListenerModel_RemoveListenerForEvent(ptr->changeListenerModel, inForWhatChange, inListener);
	}
	else
	{
		Console_Warning(Console_WriteValueFourChars, "attempt to stop monitoring invalid terminal window, event", inForWhatChange);
	}
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
windowController([[TerminalWindow_Controller alloc]
					initWithTerminalVC:[[[TerminalView_Controller alloc] init] autorelease]
										owner:REINTERPRET_CAST(this, TerminalWindowRef)]),
window(windowController.window),
tabOffsetInPixels(0.0),
tabSizeInPixels(0.0),
preResizeViewDisplayMode(kTerminalView_DisplayModeNormal/* corrected below */),
// controls initialized below
// toolbar initialized below
isObscured(false),
isDead(false),
viewSizeIndependent(false),
recentSheetContext(),
sheetType(kMy_SheetTypeNone),
searchDialog(nullptr),
recentSearchOptions(kFindDialog_OptionsDefault),
recentSearchStrings(CFArrayCreateMutable(kCFAllocatorDefault, 0/* limit; 0 = no size limit */, &kCFTypeArrayCallBacks),
					CFRetainRelease::kAlreadyRetained),
baseTitleString(),
sessionStateChangeEventListener(),
terminalStateChangeEventListener(),
terminalViewEventListener(),
toolbarStateChangeEventListener(),
screensToViews(),
viewsToScreens(),
allScreens(),
allViews(),
installedActions()
{
@autoreleasepool {
	TerminalScreenRef			newScreen = nullptr;
	__block TerminalViewRef		newView = nullptr;
	Preferences_Result			preferencesResult = kPreferences_ResultOK;
	
	
	// for completeness, zero-out this structure (though it is set up
	// each time full-screen mode changes)
	bzero(&this->fullScreen, sizeof(this->fullScreen));
	
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
		
		
		UNUSED_RETURN(Preferences_Result)Preferences_GetData(kPreferences_TagRandomTerminalFormats, sizeof(chooseRandom), &chooseRandom);
		if (chooseRandom)
		{
			std::vector< Preferences_ContextRef >	contextList;
			
			
			if (Preferences_GetContextsInClass(Quills::Prefs::FORMAT, contextList))
			{
				std::vector< UInt16 >	numberList(contextList.size());
				std::random_device		randomDevice;
				std::mt19937			generator(randomDevice());
				UInt16					counter = 0;
				
				
				for (auto toNumber = numberList.begin(); toNumber != numberList.end(); ++toNumber, ++counter)
				{
					*toNumber = counter;
				}
				std::shuffle(numberList.begin(), numberList.end(), generator);
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
	
	gTerminalWindowRefsByNSWindow()[this->window] = this->selfRef;
	
	// create controls
	{
		Terminal_Result		terminalError = kTerminal_ResultOK;
		
		
		terminalError = Terminal_NewScreen(inTerminalInfoOrNull, inTranslationInfoOrNull, &newScreen);
		if (terminalError != kTerminal_ResultOK)
		{
			newScreen = nullptr;
		}
	}
	if (nullptr != newScreen)
	{
		// already set up by window controller
		[this->windowController enumerateTerminalViewControllersUsingBlock:
		^(TerminalView_Controller* aVC, BOOL* UNUSED_ARGUMENT(outStopFlagPtr))
		{
			newView = [aVC.terminalView.terminalContentView terminalViewRef];
			if (nullptr == newView)
			{
				Console_Warning(Console_WriteLine, "failed to construct Cocoa TerminalViewRef!");
			}
			else
			{
				TerminalView_Result		viewResult = kTerminalView_ResultOK;
				
				
				viewResult = TerminalView_RemoveDataSource(newView, nullptr/* specific screen or "nullptr" for all screens */);
				if (kTerminalView_ResultOK != viewResult)
				{
					Console_Warning(Console_WriteValue, "failed to remove Cocoa terminal view’s previous data source, error", viewResult);
				}
				viewResult = TerminalView_AddDataSource(newView, newScreen);
				if (kTerminalView_ResultOK != viewResult)
				{
					Console_Warning(Console_WriteValue, "failed to set new data source for Cocoa terminal view, error", viewResult);
				}
			}
		}];
		
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
	
	// set the standard state to be large enough for the specified number of columns and rows;
	// and, use the standard size, initially; then, perform a maximize/restore to correct the
	// initial-zoom quirk that would otherwise occur
	if (nullptr != newScreen)
	{
		TerminalView_PixelWidth		screenWidth;
		TerminalView_PixelHeight	screenHeight;
		
		
		TerminalView_GetTheoreticalViewSize(getActiveView(this)/* TEMPORARY - must consider a list of views */,
											Terminal_ReturnColumnCount(newScreen), Terminal_ReturnRowCount(newScreen),
											screenWidth, screenHeight);
		setStandardState(this, screenWidth.integralPixels(), screenHeight.integralPixels());
	}
	
	// stagger the window (this is effective for newly-created windows
	// that are alone; if a workspace is spawning them then the actual
	// window location will be overridden by the workspace configuration)
	unless (inNoStagger)
	{
		NSWindow*	frontWindow = [NSApp mainWindow];
		NSRect		thisFrame = this->window.frame;
		UInt16		staggerListIndex = 0;
		UInt16		localWindowIndex = 0;
		
		
		calculateWindowFrameCocoa(this, &staggerListIndex, &localWindowIndex, &thisFrame);
		
		// if the frontmost window already occupies the location for
		// the new window, offset it slightly
		if (nil != frontWindow)
		{
			if (NSEqualPoints(frontWindow.frame.origin, thisFrame.origin))
			{
				thisFrame.origin.x += 20; // per Aqua Human Interface Guidelines
				thisFrame.origin.y -= 20; // per Aqua Human Interface Guidelines
			}
		}
		
		[this->window setFrameOrigin:thisFrame.origin];
	}
	
	// set up callbacks to receive various state change notifications
	this->sessionStateChangeEventListener.setWithNoRetain(ListenerModel_NewStandardListener
															(sessionStateChanged, this->selfRef/* context */));
	SessionFactory_StartMonitoringSessions(kSession_ChangeSelected, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StartMonitoringSessions(kSession_ChangeStateAttributes, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowInvalid, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowTitle, this->sessionStateChangeEventListener.returnRef());
	this->terminalStateChangeEventListener.setWithNoRetain(ListenerModel_NewStandardListener
															(terminalStateChanged, this->selfRef/* context */));
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeExcessiveErrors, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeScrollActivity, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowFrameTitle, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowIconTitle, this->terminalStateChangeEventListener.returnRef());
	Terminal_StartMonitoring(newScreen, kTerminal_ChangeWindowMinimization, this->terminalStateChangeEventListener.returnRef());
	this->terminalViewEventListener.setWithNoRetain(ListenerModel_NewStandardListener
													(terminalViewStateChanged, this->selfRef/* context */));
	TerminalView_StartMonitoring(newView, kTerminalView_EventScrolling, this->terminalViewEventListener.returnRef());
	TerminalView_StartMonitoring(newView, kTerminalView_EventSearchResultsExistence, this->terminalViewEventListener.returnRef());
	
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
	
	// override this default; technically terminal windows
	// are immediately closeable for the first 15 seconds
	setWarningOnWindowClose(this, false);
}// @autoreleasepool
}// My_TerminalWindow constructor


/*!
Destructor.  See TerminalWindow_Dispose().

(3.0)
*/
My_TerminalWindow::
~My_TerminalWindow ()
{
@autoreleasepool {
	sheetContextEnd(this);
	
	if (nullptr != this->searchDialog)
	{
		FindDialog_Dispose(&this->searchDialog);
	}
	
	// now that the window is going away, destroy any Undo commands
	// that could be applied to this window
	for (auto actionRef : this->installedActions)
	{
		Undoables_RemoveAction(actionRef);
	}
	
	// show a hidden window just before it is destroyed (most importantly, notifying callbacks)
	TerminalWindow_SetObscured(this->selfRef, false);
	
	// hide window
	if (nil != this->window)
	{
		Boolean		noAnimations = false;
		
		
		// remove from tracking maps
		gTerminalWindowRefsByNSWindow().erase(this->window);
		
		// determine if animation should occur
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNoAnimations,
									sizeof(noAnimations), &noAnimations))
		{
			noAnimations = false; // assume a value, if preference can’t be found
		}
		
		// hide the window
		if (noAnimations)
		{
			[this->window orderOut:nil];
		}
		else
		{
			// this will hide the window immediately and replace it with a window
			// that looks exactly the same; that way, it is perfectly safe for
			// the rest of the destructor to run (cleaning up other state) even
			// if the animation finishes after the original window is destroyed
			CocoaAnimation_TransitionWindowForRemove(this->window);
		}
	}
	
	// unregister session callbacks
	SessionFactory_StopMonitoringSessions(kSession_ChangeSelected, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StopMonitoringSessions(kSession_ChangeState, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StopMonitoringSessions(kSession_ChangeStateAttributes, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowInvalid, this->sessionStateChangeEventListener.returnRef());
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowTitle, this->sessionStateChangeEventListener.returnRef());
	
	// unregister screen buffer callbacks and destroy all buffers
	// (NOTE: perhaps this should be revisited, as a future feature
	// may be to allow multiple windows to use the same buffer; if
	// that were the case, killing one window should not necessarily
	// throw out its buffer)
	for (auto screenRef : this->allScreens)
	{
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeAudioState, this->terminalStateChangeEventListener.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeExcessiveErrors, this->terminalStateChangeEventListener.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeNewLEDState, this->terminalStateChangeEventListener.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeScrollActivity, this->terminalStateChangeEventListener.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeWindowFrameTitle, this->terminalStateChangeEventListener.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeWindowIconTitle, this->terminalStateChangeEventListener.returnRef());
		Terminal_StopMonitoring(screenRef, kTerminal_ChangeWindowMinimization, this->terminalStateChangeEventListener.returnRef());
	}
	
	// destroy all terminal views
	for (auto viewRef : this->allViews)
	{
		TerminalView_StopMonitoring(viewRef, kTerminalView_EventScrolling, this->terminalViewEventListener.returnRef());
		TerminalView_StopMonitoring(viewRef, kTerminalView_EventSearchResultsExistence, this->terminalViewEventListener.returnRef());
	}
	
	// throw away information about terminal window change listeners
	ListenerModel_Dispose(&this->changeListenerModel);
	
	// finally, dispose of the window
	if (nil != this->window)
	{
		[this->window close], this->window = nil;
	}
	
	// destroy all buffers (NOTE: perhaps this should be revisited, as
	// a future feature may be to allow multiple windows to use the same
	// buffer; if that were the case, killing one window should not
	// necessarily throw out its buffer)
	for (auto screenRef : this->allScreens)
	{
		Terminal_ReleaseScreen(&screenRef);
	}
}// @autoreleasepool
}// My_TerminalWindow destructor


/*!
Returns true.  Left over porting from legacy code.

(2018.02)
*/
bool
My_TerminalWindow::
isCocoa ()
const
{
	return true;
}// My_TerminalWindow::isCocoa


/*!
Provides the global coordinates that the top-left corner
of the specified window’s frame (structure) would occupy
if it were in the requested stagger position.

The “stagger list index” is initially zero.  The very
first window (at “local window index 0”) in list index 0
will sit precisely at the user’s requested window
stacking origin, if it’s on the window’s display.  As
windows are stacked, adjust the “local window index” so
that the next window is placed diagonally down and to
the right of the previous one.  Once a window touches
the bottom of the screen, increment “stagger list index”
by one so that the next window will once again sit near
the user’s preferred stacking origin.  Each stagger list
is offset slightly to the right of the preceding one so
that windows can never completely overlap.

(4.1)
*/
void
calculateIndexedWindowPosition	(My_TerminalWindowPtr	inPtr,
								 UInt16					inStaggerListIndex,
								 UInt16					inLocalWindowIndex,
								 CGPoint*				outTopLeftPositionPtr)
{
	if ((nullptr != inPtr) && (nullptr != outTopLeftPositionPtr))
	{
		// calculate the stagger offset
		CGPoint		stackingOrigin;
		CGPoint		stagger;
		CGRect		screenRect;
		
		
		// determine the user’s preferred stacking origin
		// (TEMPORARY; ideally this preference can be per-display)
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagWindowStackingOrigin, sizeof(stackingOrigin),
									&stackingOrigin))
		{
			stackingOrigin = CGPointMake(40, 40); // assume a default, if preference can’t be found
		}
		
		if (inStaggerListIndex > 0)
		{
			// when previous stagger stacks hit the bottom of the
			// screen, arbitrarily shift new stacks over slightly
			stackingOrigin.x += (inStaggerListIndex * 60); // arbitrary
		}
		
		// the stagger amount on Mac OS X is set by the Aqua Human Interface Guidelines
		stagger = CGPointMake(20, 20);
		
		// convert to floating-point rectangle
		{
			NSScreen*	windowScreen = [inPtr->window screen];
			
			
			if (nil == windowScreen)
			{
				windowScreen = [NSScreen mainScreen];
			}
			screenRect = NSRectToCGRect([windowScreen visibleFrame]);
		}
		
		if (CGRectContainsPoint(screenRect, stackingOrigin))
		{
			// window is on the display where the user set an origin preference
			outTopLeftPositionPtr->x = stackingOrigin.x + (stagger.x * inLocalWindowIndex);
			outTopLeftPositionPtr->y = stackingOrigin.y + (stagger.y * inLocalWindowIndex);
		}
		else
		{
			// window is on a different display; use the origin magnitude to
			// determine a reasonable offset on the window’s actual display
			// (TEMPORARY; ideally the preference itself is per-display)
			stackingOrigin.x += screenRect.origin.x;
			stackingOrigin.y += screenRect.origin.y;
			outTopLeftPositionPtr->x = stackingOrigin.x + (stagger.x * inLocalWindowIndex);
			outTopLeftPositionPtr->y = stackingOrigin.y + (stagger.y * inLocalWindowIndex);
		}
	}
}// calculateIndexedWindowPosition


/*!
Calculates the stagger position of Cocoa windows.

The index hints can be used to strongly suggest where
a window should end up on the screen.  (Use this if
the given window is part of an iteration over several
windows, where its order in the list is important.)
If no hints are provided, a window position is
determined in some other way.

On input, the rectangle must be the structure/frame
of the window in its NSScreen coordinates.

On output, a new frame rectangle is provided in its
NSScreen coordinates (lower-left origin).

(2018.03)
*/
void
calculateWindowFrameCocoa	(My_TerminalWindowPtr	inPtr,
							 UInt16*				inoutStaggerListIndexHintPtr,
							 UInt16*				inoutLocalWindowIndexHintPtr,
							 NSRect*				inoutArrangement)
{
	NSRect const	kStructureRegionBounds = *inoutArrangement;
	Float32 const	kStructureWidth = kStructureRegionBounds.size.width;
	Float32 const	kStructureHeight = kStructureRegionBounds.size.height;
	NSScreen*		windowScreen = [inPtr->window screen];
	NSRect			screenRect;
	CGPoint			structureTopLeftScrap = CGPointMake(0, 0);
	Boolean			doneCalculation = false;
	Boolean			tooFarRight = false;
	Boolean			tooFarDown = false;
	
	
	if (nil == windowScreen)
	{
		windowScreen = [NSScreen mainScreen];
	}
	screenRect = [windowScreen visibleFrame];
	
	while ((false == doneCalculation) && (false == tooFarRight))
	{
		calculateIndexedWindowPosition(inPtr, *inoutStaggerListIndexHintPtr, *inoutLocalWindowIndexHintPtr,
										&structureTopLeftScrap);
		inoutArrangement->origin.x = structureTopLeftScrap.x;
		inoutArrangement->origin.y = (windowScreen.frame.origin.y + NSHeight(windowScreen.frame) - structureTopLeftScrap.y - kStructureHeight);
		inoutArrangement->size.width = kStructureWidth;
		inoutArrangement->size.height = kStructureHeight;
		
		// see if the window position would start to nudge it off
		// the bottom or right edge of its display
		tooFarRight = ((inoutArrangement->origin.x + inoutArrangement->size.width) > (screenRect.origin.x + screenRect.size.width));
		tooFarDown = (inoutArrangement->origin.y < screenRect.origin.y);
		
		if (tooFarDown)
		{
			if (0 == *inoutLocalWindowIndexHintPtr)
			{
				// the window is already offscreen despite being at the
				// stacking origin so there is nowhere else to go
				doneCalculation = true;
			}
			else
			{
				// try shifting the top-left-corner origin over to see
				// if there is still room for a new window stack
				++(*inoutStaggerListIndexHintPtr);
				*inoutLocalWindowIndexHintPtr = 0;
			}
		}
		else
		{
			doneCalculation = true;
		}
	}
}// calculateWindowFrameCocoa


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
Returns the width and height of the content region
of a terminal window whose screen interior has the
specified dimensions.

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
											returnToolbarHeight(inPtr);
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
	}
}// handleFindDialogClose


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
	UndoDataFontSizeChangesPtr	dataPtr = new UndoDataFontSizeChanges;	// must be allocated by "new" because it contains C++ classes;
																		// disposed in the action method
	
	
	if (dataPtr == nullptr) error = memFullErr;
	else
	{
		// initialize context structure
		CFStringRef		fontName = nullptr;
		
		
		dataPtr->terminalWindow = inTerminalWindow;
		dataPtr->undoFontSize = inUndoFontSize;
		dataPtr->undoFont = inUndoFont;
		TerminalWindow_GetFontAndSize(inTerminalWindow, &fontName, &dataPtr->fontSize);
		dataPtr->fontName.setWithRetain(fontName);
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
Installs an Undo procedure that will revert the
dimensions of the of the specified screen to its
current dimensions when the user chooses Undo.

(3.1)
*/
void
installUndoScreenDimensionChanges	(TerminalWindowRef		inTerminalWindow)
{
	OSStatus							error = noErr;
	UndoDataScreenDimensionChangesPtr	dataPtr = new UndoDataScreenDimensionChanges;	// disposed in the action method
	
	
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
A comparison routine that is compatible with std::sort();
returns true if the first window is strictly less-than
the second based on whether or not "inWindow1" covers a
strictly larger pixel area.

(2018.03)
*/
bool
lessThanIfGreaterAreaCocoa		(NSWindow*		inWindow1,
								 NSWindow*		inWindow2)
{
	bool			result = false;
	NSRect			frame1 = inWindow1.frame;
	NSRect			frame2 = inWindow2.frame;
	CGFloat const	kArea1 = (NSWidth(frame1) * NSHeight(frame1));
	CGFloat const	kArea2 = (NSWidth(frame2) * NSHeight(frame2));
	
	
	result = (kArea1 > kArea2);
	if (kArea1 == kArea2)
	{
		// enforce strict weak ordering to make ties consistent
		result = (inWindow1 < inWindow2);
	}
	
	return result;
}// lessThanIfGreaterAreaCocoa


/*!
Returns the width in pixels of a scroll bar in the given
terminal window.  If the scroll bar is invisible, the width
is set to 0.

(3.0)
*/
UInt16
returnScrollBarWidth	(My_TerminalWindowPtr	inPtr)
{
	UInt16		result = 0;
	Boolean		showScrollBar = true;
	
	
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskShowsScrollBar, sizeof(showScrollBar),
							&showScrollBar))
	{
		showScrollBar = true; // assume a value if the preference cannot be found
	}
	
	if ((false == TerminalWindow_IsFullScreen(inPtr->selfRef)) || (showScrollBar))
	{
		// TEMPORARY; update this value
		result = 16;
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
			if (nullptr != dataPtr)
			{
				delete dataPtr, dataPtr = nullptr;
			}
			break;
		
		case kUndoables_ActionInstructionRedo:
		case kUndoables_ActionInstructionUndo:
		default:
			{
				CGFloat			oldFontSize = 0;
				CFStringRef		oldFontName = nullptr;
				
				
				// make this reversible by preserving the font information
				TerminalWindow_GetFontAndSize(dataPtr->terminalWindow, &oldFontName, &oldFontSize);
				
				// change the font and/or size of the window
				TerminalWindow_SetFontAndSize(dataPtr->terminalWindow,
												(dataPtr->undoFont) ? dataPtr->fontName.returnCFStringRef() : nullptr,
												(dataPtr->undoFontSize) ? dataPtr->fontSize : 0);
				
				// save the font and size
				dataPtr->fontSize = oldFontSize;
				dataPtr->fontName.setWithRetain(oldFontName);
			}
			break;
		}
	}
}// reverseFontChanges


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
			if (nullptr != dataPtr)
			{
				delete dataPtr, dataPtr = nullptr;
			}
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
				TerminalWindow_SetScreenDimensions(dataPtr->terminalWindow, dataPtr->columns, dataPtr->rows);
				
				// save the dimensions
				dataPtr->columns = oldColumns;
				dataPtr->rows = oldRows;
			}
			break;
		}
	}
}// reverseScreenDimensionChanges


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
@autoreleasepool {
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
					
					// set the session used to determine the state of toolbar items
					if (ptr->isCocoa())
					{
						ptr->windowController.toolbarDelegate.session = session;
					}
				}
				if (Session_StateIsActiveUnstable(session) || Session_StateIsActiveStable(session))
				{
					// the cursor should be allowed to render once again, if it was inhibited
					for (auto viewRef : ptr->allViews)
					{
						UNUSED_RETURN(TerminalView_Result)TerminalView_SetCursorRenderingEnabled(viewRef, true);
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
					
					// the cursor should not be displayed for inactive sessions
					for (auto viewRef : ptr->allViews)
					{
						UNUSED_RETURN(TerminalView_Result)TerminalView_SetCursorRenderingEnabled(viewRef, false);
					}
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
				CFStringRef		titleCFString = nullptr;
				
				
				if (kSession_ResultOK == Session_GetWindowUserDefinedTitle(session, titleCFString))
				{
					TerminalWindow_SetWindowTitle(terminalWindow, titleCFString);
				}
			}
		}
		break;
	
	case kSession_ChangeWindowInvalid:
		// if a window is still Full Screen, kick it into normal mode
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
			// this handler is invoked for changes to ANY session,
			// but the response is specific to one, so check first
			if (Session_ReturnActiveTerminalWindow(session) == terminalWindow)
			{
				if (TerminalWindow_IsFullScreen(terminalWindow))
				{
					[TerminalWindow_ReturnNSWindow(terminalWindow) toggleFullScreen:nil];
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
}// @autoreleasepool
}// sessionStateChanged


/*!
Adds or removes a Full Screen icon from the specified window.
Not for normal use, called as a side effect of changes to
user preferences.

(4.1)
*/
void
setCocoaWindowFullScreenIcon	(NSWindow*	inWindow,
								 Boolean	inHasFullScreenIcon)
{
	if (inHasFullScreenIcon)
	{
		[inWindow setCollectionBehavior:([inWindow collectionBehavior] | FUTURE_SYMBOL(1 << 7, NSWindowCollectionBehaviorFullScreenPrimary))];
	}
	else
	{
		[inWindow setCollectionBehavior:([inWindow collectionBehavior] & ~(FUTURE_SYMBOL(1 << 7, NSWindowCollectionBehaviorFullScreenPrimary)))];
	}
}// setCocoaWindowFullScreenIcon


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
						 Preferences_ContextRef		inContext,
						 Boolean					inAnimateWindowChanges)
{
	TerminalScreenRef		activeScreen = getActiveScreen(inPtr);
	
	
	if (nullptr != activeScreen)
	{
		Preferences_TagSetRef	tagSet = PrefPanelTerminals_NewScreenPaneTagSet();
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextCopy(inContext, Terminal_ReturnConfiguration(activeScreen), tagSet);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to overwrite terminal screen configuration, error", prefsResult);
		}
		
		Preferences_ReleaseTagSet(&tagSet);
		
		// IMPORTANT: this window adjustment should match TerminalWindow_SetScreenDimensions()
		unless (inPtr->viewSizeIndependent)
		{
			UInt16		columns = 0;
			UInt16		rows = 0;
			
			
			prefsResult = Preferences_ContextGetData(inContext, kPreferences_TagTerminalScreenColumns,
														sizeof(columns), &columns, true/* search defaults */);
			if (kPreferences_ResultOK == prefsResult)
			{
				prefsResult = Preferences_ContextGetData(inContext, kPreferences_TagTerminalScreenRows,
															sizeof(rows), &rows, true/* search defaults */);
				if (kPreferences_ResultOK == prefsResult)
				{
					Terminal_SetVisibleScreenDimensions(activeScreen, columns, rows);
					setWindowToIdealSizeForDimensions(inPtr, columns, rows, inAnimateWindowChanges);
				}
			}
		}
	}
}// setScreenPreferences


/*!
Sets the standard state (for zooming) of the given
terminal window to match the size required to fit the
specified width and height in pixels.

Once this is done, you can make the window this size
by zooming “out”.

(3.0)
*/
void
setStandardState	(My_TerminalWindowPtr	inPtr,
					 UInt16					inScreenWidthInPixels,
					 UInt16					inScreenHeightInPixels,
					 Boolean				inAnimatedResize)
{
	SInt16		windowWidth = 0;
	SInt16		windowHeight = 0;
	SInt16		originalHeight = NSHeight(inPtr->window.frame);
	
	
	getWindowSizeFromViewSize(inPtr, inScreenWidthInPixels, inScreenHeightInPixels, &windowWidth, &windowHeight);
	
	{
		NSRect		newContentRect = [inPtr->window contentRectForFrameRect:inPtr->window.frame];
		NSRect		newFrameRect;
		
		
		newContentRect.size.width = windowWidth;
		newContentRect.size.height = windowHeight;
		newFrameRect = [inPtr->window frameRectForContentRect:newContentRect];
		newFrameRect.origin.y += (originalHeight - NSHeight(newFrameRect));
		[inPtr->window setFrame:newFrameRect display:YES animate:inAnimatedResize];
	}
}// setStandardState


/*!
This routine handles any side effects that should occur when a
window is entering or exiting full-screen mode.

Currently this handles things like hiding or showing the scroll
bar when the user preference for “no scroll bar” is set.

This must be a separate function from any code that makes a
window actually enter or exit full-screen (such as when handling
a command) because the system-provided Full Screen mode can be
triggered indirectly.

(4.1)
*/
void
setUpForFullScreenModal		(My_TerminalWindowPtr	inPtr,
							 Boolean				inWillBeFullScreen,
							 Boolean				inSwapViewMode,
							 My_FullScreenState		inState)
{
	SystemUIOptions		optionsForFullScreen = 0;
	Boolean				useCustomFullScreenMode = true;
	Boolean				showOffSwitch = true;
	Boolean				showScrollBar = true;
	Boolean				allowForceQuit = true;
	Boolean				showMenuBar = true;
	Boolean				showWindowFrame = true;
	
	
	if (false == inWillBeFullScreen)
	{
		// if the window is already Full Screen, it must be returned
		// in the way that it started (the user may have changed the
		// preference in the meantime)
		useCustomFullScreenMode = (false == inPtr->fullScreen.isUsingOS);
	}
	else
	{
		if (kPreferences_ResultOK !=
			Preferences_GetData(kPreferences_TagKioskNoSystemFullScreenMode, sizeof(useCustomFullScreenMode),
								&useCustomFullScreenMode))
		{
			useCustomFullScreenMode = false; // assume a value if the preference cannot be found
		}
	}
	
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskShowsOffSwitch, sizeof(showOffSwitch),
							&showOffSwitch))
	{
		showOffSwitch = true; // assume a value if the preference cannot be found
	}
	
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskShowsScrollBar, sizeof(showScrollBar),
							&showScrollBar))
	{
		showScrollBar = true; // assume a value if the preference cannot be found
	}
	
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
							&allowForceQuit))
	{
		allowForceQuit = true; // assume a value if the preference cannot be found
	}
	unless (allowForceQuit) optionsForFullScreen |= kUIOptionDisableForceQuit;
	
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskShowsMenuBar, sizeof(showMenuBar),
							&showMenuBar))
	{
		showMenuBar = false; // assume a value if the preference cannot be found
	}
	if (showMenuBar) optionsForFullScreen |= kUIOptionAutoShowMenuBar;
	
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagKioskShowsWindowFrame, sizeof(showWindowFrame),
							&showWindowFrame))
	{
		showWindowFrame = true; // assume a value if the preference cannot be found
	}
	
	// if the system’s own Full Screen method is in use, fix
	// certain settings (they have no effect anyway)
	if (false == useCustomFullScreenMode)
	{
		allowForceQuit = true;
		showMenuBar = true;
		showWindowFrame = false;
	}
	
	if (inWillBeFullScreen)
	{
		// prepare to turn on Full Screen; “completed” means “everything
		// that happens AFTER the window frame is made full-screen”, whereas
		// the pre-completed setup all happens BEFORE the frame is changed;
		// everything here should be the opposite of the off-state code below
		if (kMy_FullScreenStateCompleted == inState)
		{
			if (useCustomFullScreenMode)
			{
				// remove any shadow so that “neighboring” full-screen windows
				// on other displays do not appear to have shadows over them
				[inPtr->window setHasShadow:NO];
			}
			
			// show “off switch” if user has requested it
			if (showOffSwitch)
			{
				Keypads_SetVisible(kKeypads_WindowTypeFullScreen, true);
				[inPtr->window makeKeyAndOrderFront:NSApp];
			}
			
			// set flags last because the window is not in a complete
			// full-screen state until every change is in effect
			inPtr->fullScreen.isOn = true;
			inPtr->fullScreen.isUsingOS = (false == useCustomFullScreenMode);
		}
		else
		{
			TerminalView_DisplayMode const	kOldMode = TerminalView_ReturnDisplayMode(inPtr->allViews.front());
			
			
			// initialize the structure to a known state
			bzero(&inPtr->fullScreen, sizeof(inPtr->fullScreen));
			
			if (useCustomFullScreenMode)
			{
				if (false == TerminalWindow_IsFullScreenMode())
				{
					// no windows are full-screen yet so turn on the system-wide
					// mode (hiding the menu bar and Dock, etc.); do this early
					// so that the usable screen space is up-to-date when the
					// window tries to figure out how much space it can use
					assert_noerr(SetSystemUIMode(kUIModeAllHidden, optionsForFullScreen));
				}
			}
			
			unless (showScrollBar)
			{
				Console_Warning(Console_WriteLine, "hide-scroll-bar not implemented for Cocoa window");
			}
			
			// configure the view to allow a font size change instead of
			// a dimension change, when appropriate; also store enough
			// information to reverse all changes later (IMPORTANT: this
			// changes the user’s preferred behavior for the window and
			// it must be undone when Full Screen ends)
			TerminalWindow_GetFontAndSize(inPtr->selfRef, nullptr/* font name */, &inPtr->fullScreen.oldFontSize);
			Console_Warning(Console_WriteLine, "save-window-frame not implemented for Cocoa window");
			inPtr->fullScreen.oldMode = kOldMode;
			inPtr->fullScreen.newMode = (false == inSwapViewMode)
										? kOldMode
										: (kTerminalView_DisplayModeZoom == kOldMode)
											? kTerminalView_DisplayModeNormal
											: kTerminalView_DisplayModeZoom;
			if (inPtr->fullScreen.newMode != inPtr->fullScreen.oldMode)
			{
				TerminalView_SetDisplayMode(inPtr->allViews.front(), inPtr->fullScreen.newMode);
				setViewSizeIndependentFromWindow(inPtr, true);
			}
		}
	}
	else
	{
		// prepare to turn off Full Screen; “completed” means “everything
		// that happens AFTER the window frame is changed back”, whereas
		// the pre-completed setup all happens BEFORE the frame is changed;
		// everything here should be the opposite of the code above
		if (kMy_FullScreenStateCompleted == inState)
		{
			if (inPtr->fullScreen.newMode != inPtr->fullScreen.oldMode)
			{
				// window is set to normally zoom text but was temporarily
				// changing dimensions; wait until now (after frame has
				// been restored) to reverse the display mode setting so
				// that the text zoom is not affected by the frame change
				if (kTerminalView_DisplayModeZoom == inPtr->fullScreen.oldMode)
				{
					TerminalView_SetDisplayMode(inPtr->allViews.front(), inPtr->fullScreen.oldMode);
					setViewSizeIndependentFromWindow(inPtr, false);
				}
			}
		}
		else
		{
			// clear flags immediately because the window is not in a
			// complete full-screen state once it has started to change back
			inPtr->fullScreen.isOn = false;
			inPtr->fullScreen.isUsingOS = false;
			
			if (useCustomFullScreenMode)
			{
				if (false == TerminalWindow_IsFullScreenMode())
				{
					// no windows remain that are full-screen; turn off the
					// system-wide mode (restoring the menu bar and Dock, etc.)
					assert_noerr(SetSystemUIMode(kUIModeNormal, 0/* options */));
				}
			}
			
			// restore the shadow
			[inPtr->window setHasShadow:YES];
			
			// now explicitly disable settings that apply to Full Screen
			// mode as a whole (and not to each window)
			Keypads_SetVisible(kKeypads_WindowTypeFullScreen, false);
			
			if (inPtr->fullScreen.newMode != inPtr->fullScreen.oldMode)
			{
				// window is set to normally change dimensions but was temporarily
				// zooming its font; reverse the display mode setting and font
				// before the frame changes back so that a normal window resize
				// will automatically set the correct original dimensions
				if (kTerminalView_DisplayModeNormal == inPtr->fullScreen.oldMode)
				{
					// normal mode (window size reflects screen dimensions)
					TerminalView_SetDisplayMode(inPtr->allViews.front(), inPtr->fullScreen.oldMode);
					TerminalWindow_SetFontAndSize(inPtr->selfRef, nullptr/* font name */, inPtr->fullScreen.oldFontSize);
					setViewSizeIndependentFromWindow(inPtr, false);
				}
			}
		}
	}
}// setUpForFullScreenModal


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
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to copy terminal screen Format configuration, error", prefsResult);
		}
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
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to copy terminal screen Translation configuration, error", prefsResult);
		}
		
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
		inPtr->window.documentEdited = ((inCloseBoxHasDot) ? YES : NO);
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
@autoreleasepool {
	[inPtr->window setTitle:(NSString*)inNewTitle];
	
	// technically the default behavior for Cocoa windows with tabs is
	// to keep the titles of both in sync, though new APIs were added
	// in 10.13 (NSWindow "tab" property, NSWindowTab class) that will
	// allow the two strings to diverge in the future if this is
	// desired...
}// @autoreleasepool
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
									 UInt16					inRows,
									 Boolean				inAnimateWindowChanges)
{
	if (false == inPtr->allViews.empty())
	{
		TerminalViewRef				activeView = getActiveView(inPtr);
		TerminalView_PixelWidth		screenWidth;
		TerminalView_PixelHeight	screenHeight;
		
		
		TerminalView_GetTheoreticalViewSize(activeView/* TEMPORARY - must consider a list of views */,
											inColumns, inRows, screenWidth, screenHeight);
		setStandardState(inPtr, screenWidth.integralPixels(), screenHeight.integralPixels(), inAnimateWindowChanges);
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
		TerminalViewRef				activeView = getActiveView(inPtr);
		TerminalView_PixelWidth		screenWidth;
		TerminalView_PixelHeight	screenHeight;
		
		
		TerminalView_GetIdealSize(activeView/* TEMPORARY - must consider a list of views */,
									screenWidth, screenHeight);
		setStandardState(inPtr, screenWidth.integralPixels(), screenHeight.integralPixels());
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
	TerminalWindowRef				ref = REINTERPRET_CAST(GenericDialog_ReturnImplementation(inDialogThatClosed), TerminalWindowRef);
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
				installUndoScreenDimensionChanges(ref);
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
	Preferences_ContextWrap		newContext(Preferences_NewContext(inClass),
											Preferences_ContextWrap::kAlreadyRetained);
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
			inPtr->recentSheetContext.setWithRetain(newContext.returnRef());
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
	case kTerminal_ChangeExcessiveErrors:
		// the terminal has finally had enough, having seen a ridiculous
		// number of data errors; report this to the user
		{
			//TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				AlertMessages_BoxWrap			box(Alert_NewWindowModal(TerminalWindow_ReturnNSWindow(terminalWindow)),
													AlertMessages_BoxWrap::kAlreadyRetained);
				CFRetainRelease					dialogTextCFString(UIStrings_ReturnCopy(kUIStrings_AlertWindowExcessiveErrorsPrimaryText),
																	CFRetainRelease::kAlreadyRetained);
				CFRetainRelease					helpTextCFString(UIStrings_ReturnCopy(kUIStrings_AlertWindowExcessiveErrorsHelpText),
																	CFRetainRelease::kAlreadyRetained);
				
				
				Alert_SetParamsFor(box.returnRef(), kAlert_StyleOK);
				Alert_SetIcon(box.returnRef(), kAlert_IconIDNote);
				
				assert(dialogTextCFString.exists());
				assert(helpTextCFString.exists());
				Alert_SetTextCFStrings(box.returnRef(), dialogTextCFString.returnCFStringRef(),
										helpTextCFString.returnCFStringRef());
				
				// show the message
				Alert_Display(box.returnRef()); // retains alert until it is dismissed
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
		@autoreleasepool {
			TerminalScreenRef	screen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inListenerContextPtr, TerminalWindowRef);
			
			
			if (nullptr != terminalWindow)
			{
				My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
				CFStringRef						titleCFString = nullptr;
				
				
				Terminal_CopyTitleForIcon(screen, titleCFString);
				if (nullptr != titleCFString)
				{
					[ptr->window setMiniwindowTitle:BRIDGE_CAST(titleCFString, NSString*)];
					
					CFRelease(titleCFString), titleCFString = nullptr;
				}
			}
		}// @autoreleasepool
		}
		break;
	
	case kTerminal_ChangeWindowMinimization:
		// minimize or restore window based on requested minimization
		{
		@autoreleasepool {
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
		}// @autoreleasepool
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
		}
		break;
	
	case kTerminalView_EventSearchResultsExistence:
		// search results either appeared or disappeared; ensure the scroll bar
		// rendering is only installed when it is needed
		{
			TerminalViewRef		view = REINTERPRET_CAST(inEventContextPtr, TerminalViewRef);
			
			
			// INCOMPLETE; when search results are found, add tick marks to Cocoa scroll bar
			//installTickHandler(ptr, TerminalView_SearchResultsExist(view));
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
	if (inPtr->allViews.empty())
	{
		// nothing needed
	}
	else
	{
		Console_Warning(Console_WriteLine, "update-scroll-bars not implemented for Cocoa window");
	}
}// updateScrollBars

} // anonymous namespace


#pragma mark -
@implementation TerminalWindow_Controller //{


#pragma mark Internally-Declared Properties

/*!
The value of the window "frame" property just prior to
entering Full Screen mode.  (Used to restore the original
window frame when Full Screen exits.)
*/
@synthesize preFullScreenFrame = _preFullScreenFrame;

/*!
An object that specifies the majority of the behavior for
the terminal toolbar.
*/
@synthesize toolbarDelegate = _toolbarDelegate;


#pragma mark Externally-Declared Properties

/*!
The Terminal Window object that owns this window controller.
*/
@synthesize terminalWindowRef = _terminalWindowRef;


#pragma mark Initializers


/*!
A temporary initializer for creating a terminal window frame
that wraps an experimental, Cocoa-based terminal view.

Eventually, this will be the basis for the default interface.

(2016.03)
*/
- (instancetype)
initWithTerminalVC:(TerminalView_Controller*)	aViewController
owner:(TerminalWindowRef)						aTerminalWindowRef
{
	self = [super initWithWindowNibName:@"TerminalWindowCocoa"];
	if (nil != self)
	{
		NSView*		contentView = REINTERPRET_CAST(self.window.contentView, NSView*);
		
		
		self.window.delegate = self;
		
		_rootVC = [[TerminalWindow_RootVC alloc] init];
		[self.rootViewController addScrollControllerBeyondEdge:NSMinYEdge];
		{
			id		defaultScrollVC = [[self.rootViewController enumerateScrollControllers] nextObject];
			
			
			if (NO == [defaultScrollVC isKindOfClass:TerminalView_ScrollableRootVC.class])
			{
				Console_Warning(Console_WriteLine, "assertion failure: root view controller has unexpected scroll controller");
			}
			else
			{
				TerminalView_ScrollableRootVC*		asScrollVC = STATIC_CAST(defaultScrollVC, TerminalView_ScrollableRootVC*);
				
				
				[asScrollVC addTerminalViewController:aViewController];
			}
		}
		
		_terminalWindowRef = aTerminalWindowRef;
		
		// add root view controller’s view
		self.rootViewController.view.frame = NSMakeRect(0, 0, NSWidth(contentView.frame), NSHeight(contentView.frame));
		[contentView addSubview:self.rootViewController.view];
		
		// create toolbar
		{
			NSString*					toolbarID = @"TerminalToolbar"; // do not ever change this; that would only break user preferences
			TerminalToolbar_Object*		windowToolbar = [[[TerminalToolbar_Object alloc] initWithIdentifier:toolbarID] autorelease];
			
			
			self->_toolbarDelegate = [[TerminalToolbar_Delegate alloc] initForToolbar:windowToolbar
																						experimentalItems:YES];
			[windowToolbar setAllowsUserCustomization:YES];
			[windowToolbar setAutosavesConfiguration:YES];
			[windowToolbar setDelegate:self.toolbarDelegate];
			[self.window setToolbar:windowToolbar];
			
			[self whenObject:windowToolbar postsNote:kTerminalToolbar_ObjectDidChangeVisibilityNotification
								performSelector:@selector(toolbarDidChangeVisibility:)];
		}
		
		// "canDrawConcurrently" is YES for terminal background views
		// so enable concurrent view drawing at the window level
		[self.window setAllowsConcurrentViewDrawing:YES];
		
		// terminal items in the Window menu are managed separately
		self.window.excludedFromWindowsMenu = YES;
		
		// when constructed, the toolbar may restore a previous state of
		// being invisible; therefore, this initialization is conditional
		// (and should be consistent with explicit show/hide behavior at
		// other times)
		if (self.window.toolbar.isVisible)
		{
			// since the toolbar provides more power over window buttons,
			// remove the standard buttons from the window (this creates
			// an ugly toolbar gap on the left side though, which is
			// removed by overriding "_toolbarLeadingSpace" in the window
			// subclass)
			[self setWindowButtonsHidden:YES];
			self.window.titleVisibility = NSWindowTitleHidden;
		}
	}
	return self;
}// initWithTerminalVC:


/*!
Destructor.

(2016.03)
*/
- (void)
dealloc
{
	[_rootVC release];
	[_toolbarDelegate release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(2018.04)
*/
- (TerminalWindow_RootVC*)
rootViewController
{
	return _rootVC;
}// rootViewController


#pragma mark New Methods


/*!
Performs an action on each terminal view controller.
The block argument includes a flag for stopping early.

(2018.04)
*/
- (void)
enumerateTerminalViewControllersUsingBlock:(TerminalView_ControllerBlock)	aBlock
{
	[self.rootViewController enumerateTerminalViewControllersUsingBlock:aBlock];
}// enumerateTerminalViewControllersUsingBlock:


/*!
Hides the normal window buttons for close/minimize/zoom,
expecting the buttons to be in the toolbar or unavailable
(due to user customization of the toolbar).

Note that this creates an ugly gap on the left side of
the toolbar by default.  This is removed in the custom
subclass of the terminal window by implementing the
private method "_toolbarLeadingSpace".

(2018.03)
*/
- (void)
setWindowButtonsHidden:(BOOL)	aHiddenFlag
{
	// removing the buttons entirely with "removeFromSuperview" seemed to work
	// initially but there are situations that will bring the buttons back (such
	// as opening a toolbar customization sheet, oddly enough); therefore,
	// instead, the original buttons are simply marked as hidden
	[self.window standardWindowButton:NSWindowCloseButton].hidden = aHiddenFlag;
	[self.window standardWindowButton:NSWindowMiniaturizeButton].hidden = aHiddenFlag;
	[self.window standardWindowButton:NSWindowZoomButton].hidden = aHiddenFlag;
	
	// the toolbar seems to reserve space for the window buttons even if they
	// are not present; attempt to remove this space
	// UNIMPLEMENTED
}// setWindowButtonsHidden:


#pragma mark Actions: Commands_StandardSearching


/*!
Displays the Find dialog for the given terminal window,
handling searches automatically.

(2018.03)
*/
- (IBAction)
performFind:(id)	sender
{
#pragma unused(sender)
	TerminalWindow_DisplayTextSearchDialog(self.terminalWindowRef);
}// performFind:


/*!
Companion for "performShowCompletions:"; the "sender" is an NSMenuItem
whose title should be interpreted as text to be sent as a completion
(as if the user had typed it).

(2020.01)
*/
- (IBAction)
performSendMenuItemText:(id)	sender
{
	if ([[sender class] isSubclassOfClass:NSMenuItem.class])
	{
		TerminalWindowRef	terminalWindow = self.terminalWindowRef;
		SessionRef			focusSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		
		
		if (nullptr == view)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "unable to send menu item text because a user focus session or view was not found");
		}
		else
		{
			NSMenuItem*			asMenuItem = (NSMenuItem*)sender;
			CFStringRef			asCFString = BRIDGE_CAST([asMenuItem title], CFStringRef);
			CFStringRef			completionCFString = asCFString;
			CFRetainRelease		cursorCFString(TerminalView_ReturnCursorWordCopyAsUnicode(view),
												CFRetainRelease::kAlreadyRetained);
			CFRetainRelease		substringCFString;
			
			
			// since this is meant to be a “completion”, the text
			// currently at the cursor position matters; characters
			// may be pruned from the beginning of the proposed
			// completion string if the cursor text already contains
			// some part of it (case-insensitive)
			if (cursorCFString.exists() && (CFStringGetLength(cursorCFString.returnCFStringRef()) > 0))
			{
				CFIndex const	kOriginalCompletionLength = CFStringGetLength(asCFString);
				CFRange			matchRange = CFStringFind(asCFString, cursorCFString.returnCFStringRef(),
															kCFCompareCaseInsensitive | kCFCompareAnchored);
				
				
				if (matchRange.length > 0)
				{
					substringCFString = CFRetainRelease(CFStringCreateWithSubstring(kCFAllocatorDefault, asCFString,
																					CFRangeMake(matchRange.location + matchRange.length,
																								kOriginalCompletionLength - matchRange.length)),
														CFRetainRelease::kAlreadyRetained);
					completionCFString = substringCFString.returnCFStringRef();
				}
			}
			
			// send appropriate text to the session
			Session_UserInputCFString(focusSession, completionCFString);
		}
	}
	else
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "unable to send menu item text because given object is not apparently a menu item");
	}
}


/*!
Uses the current cursor location in the focused terminal view to
perform a search for the enclosing word, popping up an interface
for choosing matching words that are already present in the view.

(2020.01)
*/
- (IBAction)
performShowCompletions:(id)		sender
{
#pragma unused(sender)
	TerminalWindowRef	terminalWindow = self.terminalWindowRef;
	TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
	
	
	TerminalView_DisplayCompletionsUI(view);
}


#pragma mark Actions: Commands_TerminalScreenResizing


/*!
Displays an interface for the user to customize the terminal
screen dimensions and scrollback settings.

(2018.03)
*/
- (IBAction)
performScreenResizeCustom:(id)	sender
{
#pragma unused(sender)
	TerminalWindow_DisplayCustomScreenSizeUI(self.terminalWindowRef);
}// performScreenResizeCustom:


/*!
Slightly decreases the number of columns in each terminal view.

(2020.01)
*/
- (IBAction)
performScreenResizeNarrower:(id)	sender
{
#pragma unused(sender)
	UInt16 const	kMinimumDimension = 10; // arbitrary
	// note: delta should be consistent with "performScreenResizeWider:"
	UInt16 const	kReduction = 10; // arbitrary; resize by somewhat coarse amounts
	UInt16			columnCount = 0;
	UInt16			rowCount = 0;
	
	
	//installUndoScreenDimensionChanges(self.terminalWindowRef);
	TerminalWindow_GetScreenDimensions(self.terminalWindowRef, &columnCount, &rowCount);
	if ((columnCount - kReduction) < kMinimumDimension)
	{
		Sound_StandardAlert();
		TerminalWindow_SetScreenDimensions(self.terminalWindowRef, kMinimumDimension, rowCount);
	}
	else
	{
		TerminalWindow_SetScreenDimensions(self.terminalWindowRef, columnCount - kReduction, rowCount);
	}
}


/*!
Slightly decreases the number of rows in each terminal view.

(2020.01)
*/
- (IBAction)
performScreenResizeShorter:(id)	sender
{
#pragma unused(sender)
	UInt16 const	kMinimumDimension = 1; // arbitrary
	// note: delta should be consistent with "performScreenResizeTaller:"
	UInt16 const	kReduction = 4; // arbitrary; resize by somewhat coarse amounts
	UInt16			columnCount = 0;
	UInt16			rowCount = 0;
	
	
	//installUndoScreenDimensionChanges(self.terminalWindowRef);
	TerminalWindow_GetScreenDimensions(self.terminalWindowRef, &columnCount, &rowCount);
	if ((rowCount - kReduction) < kMinimumDimension)
	{
		Sound_StandardAlert();
		TerminalWindow_SetScreenDimensions(self.terminalWindowRef, columnCount, kMinimumDimension);
	}
	else
	{
		TerminalWindow_SetScreenDimensions(self.terminalWindowRef, columnCount, rowCount - kReduction);
	}
}


/*!
Sets the size of each terminal view so that the total number of
rows and columns is the standard 80×24 size.

(2020.01)
*/
- (IBAction)
performScreenResizeStandard:(id)	sender
{
#pragma unused(sender)
	// IMPORTANT: dimensions chosen here should match what
	// is shown in the corresponding menu item’s title;
	// also, these dimensions are specified by VT100 modes
	// and should not be set arbitrarily
	TerminalWindow_SetScreenDimensions(self.terminalWindowRef, 80, 24);
}


/*!
Sets the size of each terminal view so that the total number of
rows and columns is a standard 80 columns wide with a large
number of rows.

(2020.01)
*/
- (IBAction)
performScreenResizeTall:(id)	sender
{
#pragma unused(sender)
	// IMPORTANT: dimensions chosen here should match what
	// is shown in the corresponding menu item’s title
	TerminalWindow_SetScreenDimensions(self.terminalWindowRef, 80, 40);
}


/*!
Slightly increases the number of rows in each terminal view.

(2020.01)
*/
- (IBAction)
performScreenResizeTaller:(id)	sender
{
#pragma unused(sender)
	// note: delta should be consistent with "performScreenResizeShorter:"
	UInt16 const	kAddition = 4; // arbitrary; resize by somewhat coarse amounts
	UInt16			columnCount = 0;
	UInt16			rowCount = 0;
	
	
	//installUndoScreenDimensionChanges(self.terminalWindowRef);
	TerminalWindow_GetScreenDimensions(self.terminalWindowRef, &columnCount, &rowCount);
	TerminalWindow_SetScreenDimensions(self.terminalWindowRef, columnCount, rowCount + kAddition);
}


/*!
Sets the size of each terminal view so that the total number of
rows and columns is the standard 132×24 size.

(2020.01)
*/
- (IBAction)
performScreenResizeWide:(id)	sender
{
#pragma unused(sender)
	// IMPORTANT: dimensions chosen here should match what
	// is shown in the corresponding menu item’s title;
	// also, these dimensions are specified by VT100 modes
	// and should not be set arbitrarily
	TerminalWindow_SetScreenDimensions(self.terminalWindowRef, 132, 24);
}


/*!
Slightly increases the number of columns in each terminal view.

(2020.01)
*/
- (IBAction)
performScreenResizeWider:(id)	sender
{
#pragma unused(sender)
	// note: delta should be consistent with "performScreenResizeNarrower:"
	UInt16 const	kAddition = 10; // arbitrary; resize by somewhat coarse amounts
	UInt16			columnCount = 0;
	UInt16			rowCount = 0;
	
	
	//installUndoScreenDimensionChanges(self.terminalWindowRef);
	TerminalWindow_GetScreenDimensions(self.terminalWindowRef, &columnCount, &rowCount);
	TerminalWindow_SetScreenDimensions(self.terminalWindowRef, columnCount + kAddition, rowCount);
}


#pragma mark Actions: Commands_TextFormatting


/*!
Displays an interface for the user to customize formatting,
such as the font and color settings.

(2018.03)
*/
- (IBAction)
performFormatCustom:(id)	sender
{
#pragma unused(sender)
	TerminalWindow_DisplayCustomFormatUI(self.terminalWindowRef);
}// performFormatCustom:


/*!
Increases the font size of the terminal views in the main window.

(2018.03)
*/
- (IBAction)
performFormatTextBigger:(id)	sender
{
#pragma unused(sender)
	UNUSED_RETURN(Boolean)TerminalWindow_SetFontRelativeSize(self.terminalWindowRef, +1, 0/* absolute limit */, true/* allow Undo */);
}// performFormatTextBigger:


/*!
Decreases the font size of the terminal views in the main window.

(2018.03)
*/
- (IBAction)
performFormatTextSmaller:(id)	sender
{
#pragma unused(sender)
	UNUSED_RETURN(Boolean)TerminalWindow_SetFontRelativeSize(self.terminalWindowRef, -1, 4/* absolute limit */, true/* allow Undo */);
}// performFormatTextSmaller:


/*!
Sets the font size to the largest value that will fit the
terminal’s current number of rows and columns on the display.

(2020.01)
*/
- (IBAction)
performFormatTextMaximum:(id)	sender
{
#pragma unused(sender)
	Console_Warning(Console_WriteLine, "unimplemented: set-maximum-size");
}


/*!
Displays an interface for the user to customize the text
encoding.

(2018.03)
*/
- (IBAction)
performTranslationSwitchCustom:(id)	sender
{
#pragma unused(sender)
	TerminalWindow_DisplayCustomTranslationUI(self.terminalWindowRef);
}// performTranslationSwitchCustom:


#pragma mark Actions: Commands_WindowRenaming


- (IBAction)
performRename:(id)	sender
{
#pragma unused(sender)
	// let the user change the title of certain windows
	// (application-level fallback; this method is also 
	// implemented by vector graphics windows, etc.)
	SessionRef		session = SessionFactory_ReturnTerminalWindowSession(self.terminalWindowRef);
	
	
	if (nullptr != session)
	{
		Session_DisplayWindowRenameUI(session);
	}
	else
	{
		// ???
		Sound_StandardAlert();
	}
}


#pragma mark Notifications


/*!
Responds when the toolbar visibility changes, by altering
the “title visible” property of the window.

(2018.03)
*/
- (void)
toolbarDidChangeVisibility:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	if (self.window.toolbar.isVisible)
	{
		self.window.titleVisibility = NSWindowTitleHidden;
		[self setWindowButtonsHidden:YES];
	}
	else
	{
		[self setWindowButtonsHidden:NO];
		self.window.titleVisibility = NSWindowTitleVisible;
	}
}// toolbarDidChangeVisibility


/*!
Performs additional tasks after terminal window becomes
Full Screen, such as showing an “off switch” window if
that user preference is set.

(2018.09)
*/
- (void)
windowDidEnterFullScreen:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	Boolean const					isBecomingFullScreen = true;
	Boolean const					shouldSwapViewMode = false;
	TerminalWindowRef				terminalWindow = [self.window terminalWindowRef];
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
	
	
	if (nullptr != ptr)
	{
		setUpForFullScreenModal(ptr, isBecomingFullScreen, shouldSwapViewMode, kMy_FullScreenStateCompleted);
	}
}// windowDidEnterFullScreen:


/*!
Performs additional tasks after terminal window leaves
Full Screen.

(2018.09)
*/
- (void)
windowDidExitFullScreen:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	Boolean const					isBecomingFullScreen = false;
	Boolean const					shouldSwapViewMode = false;
	TerminalWindowRef				terminalWindow = [self.window terminalWindowRef];
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
	
	
	if (nullptr != ptr)
	{
		setUpForFullScreenModal(ptr, isBecomingFullScreen, shouldSwapViewMode, kMy_FullScreenStateCompleted);
	}
}// windowDidExitFullScreen:


/*!
Performs additional tasks after user has requested to make
terminal window Full Screen.

(2018.09)
*/
- (void)
windowWillEnterFullScreen:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	Boolean const					isBecomingFullScreen = true;
	Boolean const					shouldSwapViewMode = false;
	TerminalWindowRef				terminalWindow = [self.window terminalWindowRef];
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
	
	
	if (nullptr != ptr)
	{
		setUpForFullScreenModal(ptr, isBecomingFullScreen, shouldSwapViewMode, kMy_FullScreenStateInProgress);
	}
}// windowWillEnterFullScreen:


/*!
Performs additional tasks after user has requested to stop
Full Screen mode for terminal window.

(2018.09)
*/
- (void)
windowWillExitFullScreen:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	Boolean const					isBecomingFullScreen = false;
	Boolean const					shouldSwapViewMode = false;
	TerminalWindowRef				terminalWindow = [self.window terminalWindowRef];
	My_TerminalWindowAutoLocker		ptr(gTerminalWindowPtrLocks(), terminalWindow);
	
	
	if (nullptr != ptr)
	{
		setUpForFullScreenModal(ptr, isBecomingFullScreen, shouldSwapViewMode, kMy_FullScreenStateInProgress);
	}
}// windowWillExitFullScreen:


#pragma mark NSWindowDelegate


/*!
Part of custom Full Screen animation (see other
delegate methods).

(2018.03)
*/
- (NSArray*)
DISABLED_customWindowsToEnterFullScreenForWindow:(NSWindow*)		aWindow
{
	NSArray*	result = ((nil != aWindow)
							? @[aWindow]
							: @[]);
	
	
	return result;
}// customWindowsToEnterFullScreenForWindow:


/*!
Part of custom Full Screen animation (see other
delegate methods).

(2018.03)
*/
- (NSArray*)
DISABLED_customWindowsToExitFullScreenForWindow:(NSWindow*)		aWindow
{
	NSArray*	result = ((nil != aWindow)
							? @[aWindow]
							: @[]);
	
	
	return result;
}// customWindowsToExitFullScreenForWindow:


/*!
This is a hack to force the system animation to be
far, FAR faster (i.e. this completely ignores
whatever duration is suggested and picks a smaller
duration; the effect is that Full Screen animation
is quicker overall).

Part of custom Full Screen animation (see other
delegate methods).

NOTE:	It appears that in “title bar invisible”
		mode (unified toolbar), ANY implementation
		of these animation methods causes the
		Full Screen version of the window to end
		up with a blank toolbar.  Sigh.  Therefore,
		for now, this is being DISABLED; the final
		window only appears correctly if there is
		no custom animation.

(2018.03)
*/
- (void)
DISABLED_window:(NSWindow*)											aWindow
startCustomAnimationToEnterFullScreenWithDuration:(NSTimeInterval)	aDuration
{
#pragma unused(aDuration)
	//assert([aWindow isKindOfClass:TerminalWindow_Object.class]);
	//TerminalWindow_Object*		asTerminalWindow = STATIC_CAST(aWindow, TerminalWindow_Object*);
	NSAnimationContext*			animationContext = [NSAnimationContext currentContext];
	
	
	self.preFullScreenFrame = aWindow.frame;
	
	[NSAnimationContext beginGrouping];
	animationContext.duration = 0.01; // restore sanity
	[aWindow.animator setFrame:aWindow.screen.frame display:YES];
	[NSAnimationContext endGrouping];
}// window:startCustomAnimationToEnterFullScreenWithDuration:


/*!
This is a hack to force the system animation to be
far, FAR faster (i.e. this completely ignores
whatever duration is suggested and picks a smaller
duration; the effect is that Full Screen animation
is quicker overall).

Part of custom Full Screen animation (see other
delegate methods).

(2018.03)
*/
- (void)
DISABLED_window:(NSWindow*)											aWindow
startCustomAnimationToExitFullScreenWithDuration:(NSTimeInterval)	aDuration
{
#pragma unused(aDuration)
	assert([aWindow isKindOfClass:TerminalWindow_Object.class]);
	TerminalWindow_Object*				asTerminalWindow = STATIC_CAST(aWindow, TerminalWindow_Object*);
	__weak TerminalWindow_Object*		weakTerminalWindow = asTerminalWindow;
	NSAnimationContext*					animationContext = [NSAnimationContext currentContext];
	
	
	// as suggested by Apple sample code, normal constraints could be
	// suspended during exit animation; this does not seem to have a
	// logical effect though (as windows can end up below the menu bar
	// or otherwise incorrectly positioned by doing this)
	//asTerminalWindow.constrainingToScreenSuspended = YES;
	
	[NSAnimationContext beginGrouping];
	animationContext.duration = 0.01; // restore sanity
	[aWindow.animator setFrame:self.preFullScreenFrame display:YES];
	[NSAnimationContext endGrouping];
	
	// TEMPORARY; in later SDK, animation API directly supports cleanup blocks
	CocoaExtensions_RunLater(aDuration + 0.5/* arbitrary */, ^{ weakTerminalWindow.constrainingToScreenSuspended = NO; });
}// window:startCustomAnimationToExitFullScreenWithDuration:


/*!
Returns information on how auxiliary OS features such as
the menu bar and Dock should be handled in Full Screen mode.

(2018.03)
*/
- (NSApplicationPresentationOptions)
window:(NSWindow*)															window
willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)		proposedOptions
{
#pragma unused(window)
	NSApplicationPresentationOptions	result = proposedOptions;
	
	
	// INCOMPLETE; read user preferences for things like disabling Force Quit
	// and apply them here
	
	// if desired, hide the toolbar as well (this seems to hide tab bars too,
	// along with anything that can show the window title; for now, don’t do
	// this but maybe later it can become a new user option)
	//result |= FUTURE_SYMBOL(1 << 11, NSApplicationPresentationAutoHideToolbar);
	
	return result;
}// window:willUseFullScreenPresentationOptions:


/*!
Responds when the user asks to close the window.

(2018.02)
*/
- (BOOL)
windowShouldClose:(id)	sender
{
#pragma unused(sender)
	BOOL			result = YES;
	SessionRef		session = SessionFactory_ReturnTerminalWindowSession(self.terminalWindowRef);
	
	
	if (nullptr != session)
	{
		Session_DisplayTerminationWarning(session);
		result = NO; // deferred close
	}
	
	return result;
}// windowShouldClose:


@end //} TerminalWindow_Controller


#pragma mark -
@implementation TerminalWindow_InfoBubble //{


TerminalWindow_InfoBubble*		gTerminalWindow_SharedInfoBubble = nil;


#pragma mark Internally-Declared Properties


/*!
Determines the behavior of the delegate implementation;
specifies a window-relative position for the window.
Note that the window is centered and below this point.
*/
@synthesize idealWindowRelativeAnchorPoint = _idealWindowRelativeAnchorPoint;


/*!
The window in which the views are displayed, meant to be
floating (not becoming key or main).  This is managed by
the "popoverMgr".
*/
@synthesize infoWindow = _infoWindow;


/*!
A view that helps to simplify layout of the text; this
maintains the padding around the text.
*/
@synthesize paddingView = _paddingView;


/*!
An object that manages various display and behavior
characteristics of the popover window, including its
color, frame appearance and animations.
*/
@synthesize popoverMgr = _popoverMgr;


/*!
The view that renders the string of text.
*/
@synthesize textView = _textView;


#pragma mark Externally-Declared Properties


/*!
The number of seconds to wait before automatically
removing the info bubble in the "display" call.  The
default delay is short, similar to that of a tooltip.
*/
@synthesize delayBeforeRemoval = _delayBeforeRemoval;


/*!
If set to NO, using "display" to show the info bubble
will not automatically release this object (allowing
reuse).  The default is YES.
*/
@synthesize releaseOnClose = _releaseOnClose;


#pragma mark Class Methods


/*!
Returns an object representing the centered, shared
information display.  Used for window resize.

(2018.03)
*/
+ (instancetype)
sharedInfoBubble
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gTerminalWindow_SharedInfoBubble = [[self.class allocWithZone:NULL] initWithStringValue:@"Âgggggggp"]; // initial string for layout only
		assert(nil != gTerminalWindow_SharedInfoBubble);
		gTerminalWindow_SharedInfoBubble.stringValue = @"";
	});
	return gTerminalWindow_SharedInfoBubble;
}// sharedInfoBubble


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithStringValue:(NSString*)		aStringValue
{
	self = [super init];
	if (nil != self)
	{
		_delayBeforeRemoval = 0.8; // arbitrary
		_releaseOnClose = YES;
		_idealWindowRelativeAnchorPoint = NSZeroPoint; // initially...
		_popoverMgr = nullptr; // see the methods that set the location of the window
		
		// create a view in which to display the string
		{
			_textView = [[NSTextField alloc] initWithFrame:NSZeroRect];
			self.textView.alignment = NSCenterTextAlignment; // NSControl setting
			self.textView.autoresizingMask = (NSViewMinXMargin | NSViewWidthSizable | NSViewMaxXMargin |
												NSViewMinYMargin /*| NSViewHeightSizable*/ | NSViewMaxYMargin);
			self.textView.bezeled = NO;
			self.textView.bordered = NO;
			self.textView.drawsBackground = NO;
			self.textView.editable = NO;
			self.textView.font = [NSFont boldSystemFontOfSize:14];
			self.textView.selectable = NO;
			self.textView.textColor = [NSColor whiteColor]; // TEMPORARY; may want a way for Popover_Window to suggest a color here
			self.textView.backgroundColor = [NSColor clearColor]; // defer to Popover_Window background
			self.textView.stringValue = aStringValue;
			[self.textView sizeToFit];
			self.textView.frame = NSInsetRect(self.textView.frame, -3, 0); // make a bit more room to discourage wrapping
		}
		
		// create a view for layout purposes (centering the string)
		{
			CGFloat const	kPaddingH = 16; // arbitrary
			CGFloat const	kPaddingV = 12; // arbitrary (though smaller values seem to cause the NSTextField to shift)
			
			
			_paddingView = [[NSView alloc] initWithFrame:NSZeroRect];
			self.paddingView.autoresizingMask = (NSViewMinXMargin | NSViewWidthSizable | NSViewMaxXMargin |
													NSViewMinYMargin | NSViewHeightSizable | NSViewMaxYMargin);
			self.paddingView.frame = NSMakeRect(0, 0, NSWidth(self.textView.frame) + kPaddingH, NSHeight(self.textView.frame) + kPaddingV);
			[self.paddingView addSubview:self.textView];
			
			// set text position within the layout view
			[self.textView setFrameOrigin:NSMakePoint((NSWidth(self.paddingView.frame) - NSWidth(self.textView.frame)) / 2.0,
														(NSHeight(self.paddingView.frame) - NSHeight(self.textView.frame)) / 2.0)];
		}
		
		// create a popover window to display the text
		_infoWindow = [[Popover_Window alloc]
						initWithView:self.paddingView
										windowStyle:kPopover_WindowStyleHelp
										arrowStyle:kPopover_ArrowStyleNone
										attachedToPoint:NSMakePoint(0, 0)
										inWindow:nil
										vibrancy:NO];
		// although the window should be removed automatically,
		// set this property to ensure it can be easily relocated
		// in the event of a problem
		[self.infoWindow setMovableByWindowBackground:YES];
	}
	return self;
}// initWithStringValue:


/*!
Destructor.

(2018.03)
*/
- (void)
dealloc
{
	Memory_EraseWeakReferences(self);
	[_textView release];
	[_paddingView release];
	if (nil != _popoverMgr)
	{
		PopoverManager_Dispose(&_popoverMgr);
	}
	[_infoWindow release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Specifies the text that appears in the info bubble.  This
should fit on a single line.

IMPORTANT:	Currently this window does not resize itself to
			accommodate different strings; it only sets an
			appropriate size for the string given to the
			initializer.  If this object is reused, its
			initial string value must be something that will
			produce the desired maximum frame size.

(2018.03)
*/
- (NSString*)
stringValue
{
	return self.textView.stringValue;
}
- (void)
setStringValue:(NSString*)		aStringValue
{
	self.textView.stringValue = aStringValue;
}// setStringValue:


#pragma mark New Methods


/*!
Displays the info bubble for a short period of time.

Currently, the display time and animation style cannot
be customized.

(2018.03)
*/
- (void)
display
{
	__weak decltype(self)		weakSelf = self;
	
	
	if (nullptr == self.popoverMgr)
	{
		// a popover manager is only created when the layout is specified
		Console_Warning(Console_WriteLine, "assertion failure; should call a method such as 'moveToCenterScreen:' on TerminalWindow_InfoBubble prior to using 'display'");
	}
	else
	{
		PopoverManager_DisplayPopover(self.popoverMgr);
		// hide the window eventually
		CocoaExtensions_RunLater(self.delayBeforeRemoval,
									^{
										PopoverManager_RemovePopover(weakSelf.popoverMgr, true/* “confirming” animation style */);
										if (weakSelf.releaseOnClose)
										{
											//[weakSelf release]; // TEMPORARY (convert to "weakSelf" in future)
										}
									});
	}
}// display


/*!
Specifies that the information display should appear near
the current cursor location of the specified terminal
window (not covering the text on that line).  This does
not continue to monitor cursor movements; you must call
this method again for the same terminal window to update
the position.

(2018.03)
*/
- (void)
moveBelowCursorInTerminalWindow:(TerminalWindowRef)		aTerminalWindow
{
	NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(aTerminalWindow);
	NSView*		parentView = STATIC_CAST([cocoaWindow contentView], NSView*);
	CGRect		globalCursorBounds;
	
	
	TerminalView_GetCursorGlobalBounds(TerminalWindow_ReturnViewWithFocus(aTerminalWindow), globalCursorBounds);
	self.idealWindowRelativeAnchorPoint = NSMakePoint(globalCursorBounds.origin.x - cocoaWindow.frame.origin.x,
														globalCursorBounds.origin.y - cocoaWindow.frame.origin.y);
	
	// arrange to display the label over this window
	if (nullptr != self.popoverMgr)
	{
		PopoverManager_Dispose(&_popoverMgr);
	}
	
	self.popoverMgr = PopoverManager_New(self.infoWindow, self.textView/* first responder */,
											self/* delegate */, kPopoverManager_AnimationTypeStandard,
											kPopoverManager_BehaviorTypeFloating,
											parentView);
}// moveBelowCursorInTerminalWindow:


/*!
Specifies that the information display should appear in
the center of the given screen (or the main screen, if
"aScreen" is set to nil).

(2018.03)
*/
- (void)
moveToCenterScreen:(NSScreen*)		aScreen
{
	NSScreen*	windowScreen = aScreen;
	
	
	if (nil == windowScreen)
	{
		windowScreen = [NSScreen mainScreen];
	}
	
	self.idealWindowRelativeAnchorPoint = NSMakePoint(windowScreen.frame.origin.x + (NSWidth(windowScreen.frame) / 2.0),
														windowScreen.frame.origin.y + (NSHeight(windowScreen.frame) / 2.0));
	
	if (nullptr != self.popoverMgr)
	{
		PopoverManager_Dispose(&_popoverMgr);
	}
	self.popoverMgr = PopoverManager_New(self.infoWindow, self.textView/* first responder */,
											self/* delegate */, kPopoverManager_AnimationTypeStandard,
											kPopoverManager_BehaviorTypeFloating,
											STATIC_CAST(nil, NSView*));
}// moveToCenterScreen:


#pragma mark Popover_Delegate


/*!
Assists the dynamic resize of a popover window by indicating
whether or not there are per-axis constraints on resizing.

(2018.03)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getHorizontalResizeAllowed:(BOOL*)		outHorizontalFlagPtr
getVerticalResizeAllowed:(BOOL*)		outVerticalFlagPtr
{
#pragma unused(aPopoverManager)
	*outHorizontalFlagPtr = NO;
	*outVerticalFlagPtr = NO;
}// popoverManager:getHorizontalResizeAllowed:getVerticalResizeAllowed:


/*!
Returns the initial view size for the popover.

(2018.03)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getIdealSize:(NSSize*)					outSizePtr
{
#pragma unused(aPopoverManager)
	*outSizePtr = self.paddingView.frame.size;
}// popoverManager:getIdealSize:


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForFrame:parentWindow:".

(2018.03)
*/
- (NSPoint)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealAnchorPointForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentWindow)
	NSPoint		result = NSMakePoint(parentFrame.origin.x + self.idealWindowRelativeAnchorPoint.x,
										parentFrame.origin.y + self.idealWindowRelativeAnchorPoint.y);
	
	
	return result;
}// popoverManager:idealAnchorPointForFrame:parentWindow:


/*!
Returns arrow placement information for the popover.

(2018.03)
*/
- (Popover_Properties)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentFrame, parentWindow)
	Popover_Properties	result = (kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow);
	
	
	return result;
}// idealArrowPositionForFrame:parentWindow:


@end //} TerminalWindow_InfoBubble


#pragma mark -
@implementation TerminalWindow_InfoBubble (TerminalWindow_InfoBubbleInternal) //{


@end //} TerminalWindow_InfoBubble (TerminalWindow_InfoBubbleInternal)


#pragma mark -
@implementation TerminalWindow_Object //{


// externally-declared
@synthesize constrainingToScreenSuspended = _constrainingToScreenSuspended;


#pragma mark NSWindow


/*!
Constrains the window’s layout based on the dimensions of
the given screen and the "constrainingToScreenSuspended"
property.

(2018.03)
*/
- (NSRect)
constrainFrameRect:(NSRect)		frameRect
toScreen:(NSScreen*)			screen
{
	NSRect		result = frameRect;
	
	
	if (NO == self.constrainingToScreenSuspended)
	{
		result = [super constrainFrameRect:frameRect toScreen:screen];
	}
	
	return result;
}// constrainFrameRect:toScreen:


/*!
Override of PRIVATE method in NSWindow that forces the
toolbar to NOT make room for standard window buttons.

Somehow, even when the NSWindowButton instances are
hidden, the toolbar leaves a wide gap between the left
edge of the window and the first toolbar item.  This
method override removes that gap so that the toolbar
can be completely customized (and so that window button
toolbar items can be flushed completely left if the
user so desires).

See the Terminal Toolbar module for further dependencies
on toolbar layout.

(2018.04)
*/
- (CGFloat)
_toolbarLeadingSpace
{
	CGFloat		result = 5; // arbitrary
	
	
	// it isn’t clear if the base class would even call this
	// method when the toolbar is invisible but the value
	// might as well reflect the behavior (which is, normal
	// window buttons reappear when the toolbar goes away)
	if (NO == self.toolbar.isVisible)
	{
		result = 80; // arbitrary; leave enough space for a normal button triplet
	}
	
	return result;
}// _toolbarLeadingSpace


@end //} TerminalWindow_Object


#pragma mark -
@implementation TerminalWindow_RootView //{


#pragma mark Internally-Declared Properties


/*!
An array of an arbitrary number of scrollable containers that
appear in this root view. 
*/
@synthesize scrollableTerminalRootViews = _scrollableTerminalRootViews;


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
		_scrollableTerminalRootViews = [[NSMutableArray alloc] init];
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
		_scrollableTerminalRootViews = [[NSMutableArray alloc] init];
	}
	return self;
}// initWithFrame:


/*!
Destructor.

(2018.04)
*/
- (void)
dealloc
{
	[_scrollableTerminalRootViews release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Adds another terminal to the root view.  The exact layout of the
view is up to the delegate.

See also "enumerateScrollableRootViews" and
"removeScrollableRootView:".

(2018.04)
*/
- (void)
addScrollableRootView:(TerminalView_ScrollableRootView*)	aView
{
	[self.scrollableTerminalRootViews addObject:aView];
	[self addSubview:aView];
	[self forceResize];
}// addScrollableRootViewBeyondEdge:


/*!
Returns an object that can be used to determine all the
scrollable root views that this view is using.  Works
with Objective-C "for ... in ..." syntax.

(2018.04)
*/
- (NSEnumerator*)
enumerateScrollableRootViews
{
	return [self.scrollableTerminalRootViews objectEnumerator];
}// enumerateScrollableRootViews


/*!
Stops managing the specified scrollable root view (the reverse
of "addScrollableRootViewBeyondEdge:").

(2018.04)
*/
- (void)
removeScrollableRootView:(TerminalView_ScrollableRootView*)		aView
{
	[aView removeFromSuperview];
	[self.scrollableTerminalRootViews removeObject:aView];
	[self forceResize];
}// removeScrollableRootView:


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
	CGContextSetRGBStrokeColor(drawingContext, 0.0, 1.0, 1.0, 1.0/* alpha */);
	CGContextStrokeRect(drawingContext, CGRectMake(0, 0, NSWidth(self.bounds), NSHeight(self.bounds)));
}// drawRect:
#endif


@end //}


#pragma mark -
@implementation TerminalWindow_RootVC //{


#pragma mark Internally-Declared Properties


/*!
The scroll view controllers that indirectly determine which
terminal view controllers are shown in this root view.  A
default window has one scroll controller containing one view.
For example, a vertical split-view could be implemented as
additional scroll controllers holding more terminal views.
*/
@synthesize terminalScrollControllers = _terminalScrollControllers;


#pragma mark Initializers


/*!
Designated initializer.

(2018.04)
*/
- (instancetype)
init
{
	self = [super initWithNibName:@"TerminalRootCocoa" bundle:nil];
	if (nil != self)
	{
		_terminalScrollControllers = [[NSMutableArray alloc] init];
	}
	return self;
}// init


/*!
Destructor.

(2018.04)
*/
- (void)
dealloc
{
	[_terminalScrollControllers release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Returns the view of this controller, as the more
specific type "TerminalWindow_RootView".

(2018.04)
*/
- (TerminalWindow_RootView*)
rootView
{
	TerminalWindow_RootView*	result = nil;
	
	
	if (NO == [self.view isKindOfClass:TerminalWindow_RootView.class])
	{
		Console_Warning(Console_WriteLine, "TerminalWindow_RootVC 'rootView' failed; view is not of the expected class!");
	}
	else
	{
		result = STATIC_CAST(self.view, TerminalWindow_RootView*);
	}
	
	return result;
}// rootView


#pragma mark New Methods


/*!
Constructs a new scroll controller that is arranged
beyond the specified edge (for example, “top edge”
means the new view is at the top of the window).

Currently the edge is not used and arbitrary edges
are not expected; use "NSMinYEdge".

The new scroll controller has no terminal views; use
"TerminalView_ScrollableRootVC" methods as needed.

See also "enumerateScrollControllers" and
"removeScrollController:".

(2018.04)
*/
- (void)
addScrollControllerBeyondEdge:(NSRectEdge)	anEdge
{
	TerminalView_ScrollableRootVC*		newScrollVC = [[[TerminalView_ScrollableRootVC alloc] init]
														autorelease];
	
	
	if (NSMinYEdge != anEdge)
	{
		Console_Warning(Console_WriteValue, "'addScrollControllerBeyondEdge:' does not support edge constant with value", (int)anEdge);
	}
	
	[self.terminalScrollControllers addObject:newScrollVC];
	[self.rootView addScrollableRootView:newScrollVC.scrollableRootView];
}// addScrollControllerBeyondEdge:


/*!
Returns an object that can be used to determine all the
scroll controllers that this root view is using.  Works
with Objective-C "for ... in ..." syntax.

(2018.04)
*/
- (NSEnumerator*)
enumerateScrollControllers
{
	return [self.terminalScrollControllers objectEnumerator];
}// enumerateScrollControllers


/*!
Performs an action on each terminal view controller (found
indirectly through each scroll controller).  The block
argument includes a flag for stopping early.

(2018.04)
*/
- (void)
enumerateTerminalViewControllersUsingBlock:(TerminalView_ControllerBlock)	aBlock
{
	// TEMPORARY; this should determine whether or not any
	// of the iterations had to stop early
	for (TerminalView_ScrollableRootVC* aScrollVC in self.terminalScrollControllers)
	{
		[aScrollVC enumerateTerminalViewControllersUsingBlock:aBlock];
	}
}// enumerateTerminalViewControllersUsingBlock:


/*!
Stops managing the specified scroll controller’s root
view (the reverse of "addScrollControllerBeyondEdge:").

(2018.04)
*/
- (void)
removeScrollController:(TerminalView_ScrollableRootVC*)		aVC
{
	[aVC.view removeFromSuperview];
	[self.terminalScrollControllers removeObject:aVC];
}// removeScrollController:


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
	for (id object in [self enumerateScrollControllers])
	{
		assert([object isKindOfClass:TerminalView_ScrollableRootVC.class]);
		TerminalView_ScrollableRootVC*		asVC = STATIC_CAST(object, TerminalView_ScrollableRootVC*);
		
		
		asVC.view.frame = NSMakeRect(0, 0, NSWidth(self.view.frame), NSHeight(self.view.frame));
	}
}// layoutDelegateForView:resizeSubviewsWithOldSize:


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
	assert(nil != self.rootView);
	self.rootView.layoutDelegate = self;
}// loadView


@end //} TerminalWindow_RootVC


#pragma mark -
@implementation TerminalWindow_RootVC (TerminalWindow_RootVCInternal) //{


@end //} TerminalWindow_RootVC (TerminalWindow_RootVCInternal)


#pragma mark -
@implementation NSWindow (TerminalWindow_NSWindowExtensions) //{


#pragma mark Accessors


/*!
Returns any Terminal Window that is associated with an NSWindow,
or nullptr if there is none.

(4.0)
*/
- (TerminalWindowRef)
terminalWindowRef
{
	auto				toPair = gTerminalWindowRefsByNSWindow().find(self);
	TerminalWindowRef	result = nullptr;
	
	
	if (gTerminalWindowRefsByNSWindow().end() != toPair)
	{
		result = toPair->second;
	}
	return result;
}// terminalWindowRef


@end //} NSWindow (TerminalWindow_NSWindowExtensions)

// BELOW IS REQUIRED NEWLINE TO END FILE
