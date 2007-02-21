/*###############################################################

	MenuBar.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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
#include <algorithm>
#include <climits>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFUtilities.h>
#include <Console.h>
#include <Cursors.h>
#include <IconManager.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <SoundSystem.h>
#include <Undoables.h>
#include <WindowInfo.h>

// resource includes
#include "ApplicationVersion.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "MenuResources.h"
#include "StringResources.h"
#include "TelnetMenuItemNames.h"

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "AppResources.h"
#include "Clipboard.h"
#include "CommandLine.h"
#include "Commands.h"
#include "ConnectionData.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "FileUtilities.h"
#include "Folder.h"
#include "InfoWindow.h"
#include "Keypads.h"
#include "MacroSetupWindow.h"
#include "MenuBar.h"
#include "Preferences.h"
#include "ProgressDialog.h"
#include "RasterGraphicsScreen.h"
#include "Session.h"
#include "SessionFactory.h"
#include "TektronixRealGraphics.h"
#include "TektronixVirtualGraphics.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "ThreadEntryPoints.h"
#include "UIStrings.h"
#include "URL.h"



#pragma mark Constants

#define kInsertMenuAfterAllOthers	0	// for use with InsertMenu (a constant Menus.h REALLY SHOULD HAVE)

typedef SInt16 My_WindowMenuMarkType;
enum
{
	kMy_WindowMenuMarkTypeEnableIcon		= -3,	// a command, not a mark... (session reference must be defined)
	kMy_WindowMenuMarkTypeDisableIcon		= -2,	// a command, not a mark... (session reference must be defined)
	kMy_WindowMenuMarkTypeDispose			= -1,	// a command, not a mark... (global - session reference should be nullptr)
	kMy_WindowMenuMarkTypeSessionLive		= 0,	// (session reference must be defined)
	kMy_WindowMenuMarkTypeSessionOpening	= 1,	// (session reference must be defined)
	kMy_WindowMenuMarkTypeSessionDead		= 2		// (session reference must be defined)
};

#pragma mark Types

struct MyMenuItemInsertionInfo
{
	MenuRef			menu;				//!< the menu in which to create the item
	MenuItemIndex	afterItemIndex;		//!< the existing item after which the new item should appear
};
typedef MyMenuItemInsertionInfo const*		MyMenuItemInsertionInfoConstPtr;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	ListenerModel_ListenerRef	gPreferenceChangeEventListener = nullptr,
								gSessionStateChangeEventListener = nullptr,
								gSessionCountChangeEventListener = nullptr,
								gSessionWindowStateChangeEventListener = nullptr;
	MenuItemIndex				gNumberOfSessionMenuItemsAdded = 0,
								gNumberOfFormatMenuItemsAdded = 0,
								gNumberOfMacroSetMenuItemsAdded = 0,
								gNumberOfSpecialWindowMenuItemsAdded = 0,
								gNumberOfTranslationTableMenuItemsAdded = 0,
								gNumberOfWindowMenuItemsAdded = 0,
								gNumberOfSpecialScriptMenuItems = 0;
	EventModifiers				gCurrentAdjustMenuItemModifiers = 0;
	Boolean						gSimplifiedMenuBar = false;
	Boolean						gFontMenusAvailable = false;
	UInt32						gNewCommandShortcutEffect = kCommandNewSessionDefaultFavorite;
}

#pragma mark Internal Method Prototypes

static Boolean			addWindowMenuItemForSession			(SessionRef, MyMenuItemInsertionInfoConstPtr, CFStringRef);
static void				addWindowMenuItemSessionOp			(SessionRef, void*, SInt32, void*);
static void				adjustMenuItem						(MenuRef, MenuItemIndex, UInt32);
static void				adjustMenuItemByCommandID			(UInt32);
static void				adjustUsingModifiers				(EventModifiers);
static Boolean			areSessionRelatedItemsEnabled		();
static Boolean			areTEKRelatedItemsEnabled			();
static void 			buildMenuBar						();
static void				executeScriptByMenuEvent			(MenuRef, MenuItemIndex);
static MenuItemIndex	getMenuAndMenuItemIndexByCommandID	(UInt32, MenuRef*);
static void				getMenuItemAdjustmentProc			(MenuRef, MenuItemIndex, MenuCommandStateTrackerProcPtr*);
static void				getMenusAndMenuItemIndicesByCommandID	(UInt32, MenuRef*, MenuRef*, MenuItemIndex*,
																MenuItemIndex*);
static HIWindowRef		getWindowFromWindowMenuItemIndex	(MenuItemIndex);
static MenuItemIndex	getWindowMenuItemIndexForSession	(SessionRef);
static void				installMenuItemStateTrackers		();
static void				preferenceChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				removeMenuItemModifier				(MenuRef, MenuItemIndex);
static void				removeMenuItemModifiers				(MenuRef);
static MenuItemIndex	returnFirstWindowItemAnchor			(MenuRef);
static void				sessionCountChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				sessionStateChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				sessionWindowStateChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
#if 0
static void				setMenuEnabled						(MenuRef, Boolean);
#endif
static void				setMenuItemEnabled					(MenuRef, UInt16, Boolean);
static inline OSStatus	setMenuItemVisibility				(MenuRef, MenuItemIndex, Boolean);
static void				setMenusHaveKeyEquivalents			(Boolean, Boolean);
static void				setNewCommand						(UInt32);
static void				setUpDynamicMenus					();
static void				setUpFormatFavoritesMenu			(MenuRef);
static void				setUpMacroSetsMenu					(MenuRef);
static void				setUpScreenSizeFavoritesMenu		(MenuRef);
static void				setUpSessionFavoritesMenu			(MenuRef);
static void				setUpScriptsMenu					(MenuRef);
static void				setUpTranslationTablesMenu			(MenuRef);
static void				setUpWindowMenu						(MenuRef);
static void				setWindowMenuItemMark				(MenuRef, MenuItemIndex, My_WindowMenuMarkType);
static void				setWindowMenuItemMarkForSession		(SessionRef, MenuRef = nullptr, MenuItemIndex = 0);
static void				simplifyMenuBar						(Boolean);
static Boolean			stateTrackerCheckableItems			(UInt32, MenuRef, MenuItemIndex);
static Boolean			stateTrackerGenericSessionItems		(UInt32, MenuRef, MenuItemIndex);
static Boolean			stateTrackerNetSendItems			(UInt32, MenuRef, MenuItemIndex);
static Boolean			stateTrackerPrintingItems			(UInt32, MenuRef, MenuItemIndex);
static Boolean			stateTrackerShowHideItems			(UInt32, MenuRef, MenuItemIndex);
static Boolean			stateTrackerStandardEditItems		(UInt32, MenuRef, MenuItemIndex);
static Boolean			stateTrackerTEKItems				(UInt32, MenuRef, MenuItemIndex);
static Boolean			stateTrackerWindowMenuWindowItems	(UInt32, MenuRef, MenuItemIndex);



#pragma mark Public Methods

/*!
Call this routine once before performing any other
menu operations.

(3.0)
*/
void
MenuBar_Init ()
{
	// set up a callback to receive preference change notifications;
	// this ALSO has the effect of initializing menus, because the
	// callbacks are installed with a request to be invoked immediately
	// with the initial preference value (this has side effects like
	// creating the menu bar, possibly setting simplified mode, etc.)
	gPreferenceChangeEventListener = ListenerModel_NewStandardListener(preferenceChanged);
	{
		Preferences_ResultCode		prefsResult = kPreferences_ResultCodeSuccess;
		
		
		prefsResult = Preferences_ListenForChanges
						(gPreferenceChangeEventListener, kPreferences_TagMenuItemKeys,
							true/* notify of initial value */);
		assert(kPreferences_ResultCodeSuccess == prefsResult);
		prefsResult = Preferences_ListenForChanges
						(gPreferenceChangeEventListener, kPreferences_TagNewCommandShortcutEffect,
							true/* notify of initial value */);
		assert(kPreferences_ResultCodeSuccess == prefsResult);
		prefsResult = Preferences_ListenForChanges
						(gPreferenceChangeEventListener, kPreferences_TagSimplifiedUserInterface,
							true/* notify of initial value */);
		assert(kPreferences_ResultCodeSuccess == prefsResult);
	}
	
	// set up a callback to receive session count change notifications
	gSessionCountChangeEventListener = ListenerModel_NewStandardListener(sessionCountChanged);
	SessionFactory_StartMonitoring(kSessionFactory_ChangeNewSessionCount, gSessionCountChangeEventListener);
	
	// set up a callback to receive connection state change notifications
	gSessionStateChangeEventListener = ListenerModel_NewStandardListener(sessionStateChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, gSessionStateChangeEventListener);
	gSessionWindowStateChangeEventListener = ListenerModel_NewStandardListener(sessionWindowStateChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowObscured, gSessionWindowStateChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowTitle, gSessionWindowStateChangeEventListener);
	
	// explicitly request the Help menu, which will add that menu to the menu bar
	{
		MenuRef				helpMenu = nullptr;
		MenuItemIndex		itemIndex = 0;
		OSStatus			error = noErr;
		
		
		error = HMGetHelpMenu(&helpMenu, &itemIndex/* first item index, or nullptr */);
		if (noErr == error)
		{
			// add MacTelnet Help to the Help Center
			{
				CFBundleRef		myAppsBundle = AppResources_ReturnApplicationBundle();
				CFURLRef		myBundleURL = nullptr;
				FSRef			myBundleRef;
				
				
				if (nullptr != myAppsBundle)
				{
					myBundleURL = CFBundleCopyBundleURL(myAppsBundle);
					if (nullptr != myBundleURL)
					{
						if (CFURLGetFSRef(myBundleURL, &myBundleRef))
						{
							error = AHRegisterHelpBook(&myBundleRef);
							assert_noerr(error);
						}
					}
				}
				if (nullptr != myBundleURL) CFRelease(myBundleURL), myBundleURL = nullptr;
			}
			
			// add other items
			(OSStatus)InsertMenuItemTextWithCFString(helpMenu, CFSTR("")/* fixed later via callback */,
														itemIndex++, 0/* attributes */,
														kCommandContextSensitiveHelp);
			(OSStatus)InsertMenuItemTextWithCFString(helpMenu, CFSTR("")/* fixed later via callback */,
														itemIndex++, kMenuItemAttrSeparator,
														0/* command ID */);
			{
				UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
				CFStringRef				itemNameCFString = nullptr;
				
				
				stringResult = UIStrings_Copy(kUIStrings_HelpSystemShowTagsCommandName, itemNameCFString);
				if (kUIStrings_ResultCodeSuccess == stringResult)
				{
					(OSStatus)InsertMenuItemTextWithCFString(helpMenu, itemNameCFString,
																itemIndex++, 0/* attributes */,
																kCommandShowHelpTags);
					CFRelease(itemNameCFString), itemNameCFString = nullptr;
				}
				stringResult = UIStrings_Copy(kUIStrings_HelpSystemHideTagsCommandName, itemNameCFString);
				if (kUIStrings_ResultCodeSuccess == stringResult)
				{
					(OSStatus)InsertMenuItemTextWithCFString(helpMenu, itemNameCFString,
															itemIndex++, 0/* attributes */,
															kCommandHideHelpTags);
					CFRelease(itemNameCFString), itemNameCFString = nullptr;
				}
			}
		}
	}
	
	// since MacTelnet provides Show/Hide capability, make the display delay very short
	// and turn tags off initially
	(OSStatus)HMSetTagDelay(5L/* milliseconds */);
	(OSStatus)HMSetHelpTagsDisplayed(false);
	
	// synchronously build the font list...
	ThreadEntryPoints_InitializeFontMenu(GetMenuRef(kPopUpMenuIDFont));
	
	DrawMenuBar();
}// Init


/*!
Call this routine when you are completely done
with menus (at the end of the program).

(3.0)
*/
void
MenuBar_Done ()
{
	// disable preference change listener
	Preferences_StopListeningForChanges(gPreferenceChangeEventListener, kPreferences_TagMenuItemKeys);
	Preferences_StopListeningForChanges(gPreferenceChangeEventListener, kPreferences_TagNewCommandShortcutEffect);
	Preferences_StopListeningForChanges(gPreferenceChangeEventListener, kPreferences_TagSimplifiedUserInterface);
	ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
	
	// disable session counting listener
	SessionFactory_StopMonitoring(kSessionFactory_ChangeNewSessionCount, gSessionCountChangeEventListener);
	ListenerModel_ReleaseListener(&gSessionCountChangeEventListener);
	
	// disable session state listeners
	SessionFactory_StopMonitoringSessions(kSession_ChangeState, gSessionStateChangeEventListener);
	ListenerModel_ReleaseListener(&gSessionStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowObscured, gSessionWindowStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowTitle, gSessionWindowStateChangeEventListener);
	ListenerModel_ReleaseListener(&gSessionWindowStateChangeEventListener);
	
	// perform other cleanup
	setWindowMenuItemMark(nullptr/* menu is N/A */, 0/* item is N/A */, kMy_WindowMenuMarkTypeDispose);
}// Done


/*!
Determines the name of a menu item in a menu bar menu
knowing only its command ID.  If the command specified
cannot be found in any menu, "menuItemNotFoundErr" is
returned, this method does nothing and the returned text
is invalid.

IMPORTANT:  This is a low-level API.  Use of higher-level
			APIs such as Commands_CopyCommandName() is
			probably sufficient for your needs.

(3.1)
*/
OSStatus
MenuBar_CopyMenuItemTextByCommandID	(UInt32			inCommandID,
									 CFStringRef&	outText)
{
	MenuRef			menuHandle = nullptr;
	MenuItemIndex	itemIndex = getMenuAndMenuItemIndexByCommandID(inCommandID, &menuHandle);
	OSStatus		result = noErr;
	
	
	outText = nullptr;
	if ((nullptr != menuHandle) && (itemIndex > 0))
	{
		result = CopyMenuItemTextAsCFString(menuHandle, itemIndex, &outText);
	}
	else
	{
		result = menuItemNotFoundErr;
	}
	
	return result;
}// CopyMenuItemTextByCommandID


/*!
Determines the enabled or disabled (dimmed) state of a
menu item in a menu bar menu knowing only its command ID.
If the command specified cannot be found in any menu,
this method does nothing and returns "false".

IMPORTANT:	All commands must be associated with a
			MenuCommandStateTrackerProcPtr callback, or
			this routine will return "false" for any
			item that does not have one.  MacTelnet always
			determines item states dynamically, not
			statically, so this routine will not read
			the current enabled state of the item.
			Rather, it invokes the command’s callback to
			determine what the item state *should* be,
			which ultimately is what you’re interested
			in, right???

(3.0)
*/
Boolean
MenuBar_GetMenuItemEnabledByCommandID	(UInt32		inCommandID)
{
	MenuRef							menu1 = nullptr;
	MenuRef							menu2 = nullptr;
	MenuItemIndex					itemIndex1 = 0;
	MenuItemIndex					itemIndex2 = 0;
	MenuCommandStateTrackerProcPtr	proc = nullptr;
	Boolean							result = false;
	
	
	getMenusAndMenuItemIndicesByCommandID(inCommandID, &menu1, &menu2, &itemIndex1, &itemIndex2);
	if ((nullptr != menu1) && (itemIndex1 > 0)) // only look at the primary menu (both should be the same anyway)
	{
		getMenuItemAdjustmentProc(menu1, itemIndex1, &proc);
		if (nullptr != proc) result = MenuBar_InvokeCommandStateTracker(proc, inCommandID, menu1, itemIndex1);
	}
	
	return result;
}// GetMenuItemEnabledByCommandID


/*!
Returns the global rectangle approximating the
physical boundaries of the specified menu’s title
in the menu bar.  You might use this to show a
zoom rectangle animation to or from a menu.

If the specified menu has a global title rectangle,
"true" is returned; otherwise, "false" is returned.

(3.0)
*/
Boolean
MenuBar_GetMenuTitleRectangle	(MenuBar_Menu	inMenuBarMenuSpecifier,
								 Rect*			outMenuBarMenuTitleRect)
{
	Boolean		result = false;
	
	
	if (nullptr != outMenuBarMenuTitleRect)
	{
		enum
		{
			kWindowMenuTitleWidthApproximation = 72, // pixels
			kWindowMenuTitleLeftEdgeNormalApproximation = 400, // pixel offset
			kWindowMenuTitleLeftEdgeSimpleModeApproximation = 272 // pixel offset
		};
		
		
		switch (inMenuBarMenuSpecifier)
		{
		case kMenuBar_MenuWindow:
			// approximate the rectangle of the Window menu, which is different in Simplified User
			// Interface mode (there’s gotta be a beautiful way to get this just right, but I’m
			// not finding it now)
			*outMenuBarMenuTitleRect = (**(GetMainDevice())).gdRect;
			outMenuBarMenuTitleRect->bottom = GetMBarHeight();
			outMenuBarMenuTitleRect->left += (gSimplifiedMenuBar)
												? kWindowMenuTitleLeftEdgeSimpleModeApproximation
												: kWindowMenuTitleLeftEdgeNormalApproximation;
			outMenuBarMenuTitleRect->right = outMenuBarMenuTitleRect->left + kWindowMenuTitleWidthApproximation;
			result = true;
			break;
		
		default:
			// unsupported, or not a menu bar menu
			break;
		}
	}
	return result;
}// GetMenuTitleRectangle


/*!
To obtain a unique variation of the specified
item text, guaranteeing that it will not be
the identical text of any item in the specified
menu, use this method.

(3.0)
*/
void
MenuBar_GetUniqueMenuItemText	(MenuRef	inMenu,
								 Str255		inoutItemText)
{
	// only change the specified item text if it’s already “taken” by an item in the menu
	while (!MenuBar_IsMenuItemUnique(inMenu, inoutItemText))
	{
		if ((inoutItemText[inoutItemText[0]] > '9') ||
			(inoutItemText[inoutItemText[0]] < '0')) // add a number
		{
			inoutItemText[++inoutItemText[0]] = ' ';
			inoutItemText[++inoutItemText[0]] = '1';
		}
		else if (inoutItemText[inoutItemText[0]] == '9') // add another digit
		{
			inoutItemText[inoutItemText[0]] = '-';
			inoutItemText[++inoutItemText[0]] = '1';
		}
		else
		{
			++(inoutItemText[inoutItemText[0]]); // increment the number
		}
	}
}// GetUniqueMenuItemText


/*!
To obtain a unique variation of the specified
item text, guaranteeing that it will not be the
identical text of any item in the specified menu,
use this method.  Unlike the original, this
routine takes a CFStringRef as input.  You must
invoke CFRelease() on the returned string when
you are finished with it.

(3.0)
*/
void
MenuBar_GetUniqueMenuItemTextCFString	(MenuRef		inMenu,
										 CFStringRef	inItemText,
										 CFStringRef&   outUniqueItemText)
{
	CFMutableStringRef		duplicateString = CFStringCreateMutableCopy(kCFAllocatorDefault,
																		CFStringGetLength(inItemText),
																		inItemText/* base */);
	
	
	if (nullptr == duplicateString) outUniqueItemText = nullptr;
	else
	{
		// only change the specified item text if it’s already “taken” by an item in the menu
		while (!MenuBar_IsMenuItemUniqueCFString(inMenu, duplicateString))
		{
			CFIndex const   kStringLength = CFStringGetLength(duplicateString);
			UniChar			lastCharacter = CFStringGetCharacterAtIndex(duplicateString, kStringLength - 1);
			
			
			if ((lastCharacter > '9') || (lastCharacter < '0')) // add a number
			{
				CFStringAppend(duplicateString, CFSTR(" 1"));
			}
			else if (lastCharacter == '9') // add another digit
			{
				CFStringAppend(duplicateString, CFSTR("-1"));
			}
			else
			{
				CFStringRef		newNumberString = nullptr;
				
				
				++lastCharacter;
				newNumberString = CFStringCreateWithBytes(kCFAllocatorDefault,
															REINTERPRET_CAST(&lastCharacter, UInt8*),
															sizeof(lastCharacter),
															CFStringGetFastestEncoding(duplicateString),
															false/* is external representation */);
				if (nullptr != newNumberString)
				{
					CFStringReplace(duplicateString, CFRangeMake(kStringLength - 1, kStringLength - 1),
									newNumberString); // increment the number
					CFRelease(newNumberString), newNumberString = nullptr;
				}
			}
		}
		outUniqueItemText = duplicateString;
	}
}// GetUniqueMenuItemTextCFString


/*!
Handles the specified menu and menu item selection.
It is not always possible to handle commands by ID
(e.g. in dynamic menus such as the Window menu), so
this routine lets you handle them by menu and menu
item number, instead.

If you already have a menu item’s command ID, it is
more efficient to call MenuBar_HandleMenuCommandByID().

DEPRECATED.  Almost every menu command is now handled
with a Carbon Event.

(2.6)
*/
Boolean
MenuBar_HandleMenuCommand	(MenuRef			inMenu,
							 MenuItemIndex		inMenuItemIndex,
							 EventModifiers		inModifiers)
{
	UInt32		commandID = 0L;
	UInt16		menuID = GetMenuID(inMenu);
	OSStatus	error = noErr;
	Boolean		result = true;
	
	
	error = GetMenuItemCommandID(inMenu, inMenuItemIndex, &commandID);
	if (noErr == error)
	{
		(Boolean)Commands_ModifyID(&commandID, inModifiers);
	}
	
	switch (menuID)
	{
	case kMenuIDFile:
		// scan for Favorites in the menu...
		{
			CFStringRef		textCFString = nullptr;
			
			
			if (noErr != CopyMenuItemTextAsCFString(inMenu, inMenuItemIndex, &textCFString))
			{
				// error while copying string!
				Sound_StandardAlert();
			}
			else
			{
				Preferences_ContextRef		sessionContext = Preferences_NewContext
																(kPreferences_ClassSession, textCFString);
				
				
				if (nullptr == sessionContext)
				{
					// error in finding the Favorite
					Sound_StandardAlert();
				}
				else
				{
					if (false == SessionFactory_NewSessionUserFavorite
									(nullptr/* nullptr = create new terminal window */, sessionContext))
					{
						// error in attempting to use the Favorite!
						Sound_StandardAlert();
					}
					Preferences_ReleaseContext(&sessionContext);
				}
				CFRelease(textCFString), textCFString = nullptr;
			}
		}
		break;
	
	case kMenuIDView:
		{
			CFStringRef		textCFString = nullptr;
			
			
			if (noErr != CopyMenuItemTextAsCFString(inMenu, inMenuItemIndex, &textCFString))
			{
				// error while copying string!
				Sound_StandardAlert();
			}
			else
			{
				Preferences_ContextRef		formatContext = Preferences_NewContext
															(kPreferences_ClassFormat, textCFString);
				
				
				if (nullptr == formatContext)
				{
					// error in finding the Favorite
					Sound_StandardAlert();
				}
				else
				{
					if (1/* reformat window with this Favorite - unimplemented! */)
					{
						// error in attempting to use the Favorite!
						Sound_StandardAlert();
					}
					Preferences_ReleaseContext(&formatContext);
				}
				CFRelease(textCFString), textCFString = nullptr;
			}
		}
		break;
	
	case kMenuIDWindow:
		{
			// find the window corresponding to the selected menu item;
			// search the session list matching the menu item order
			// (currently, the order in which sessions are created)
			SessionRef		session = nullptr;
			
			
			error = SessionFactory_GetSessionWithZeroBasedIndex
					(inMenuItemIndex - returnFirstWindowItemAnchor(inMenu),
						kSessionFactory_ListInCreationOrder, &session);
			if (error == kSessionFactory_ResultCodeSuccess)
			{
				TerminalWindowRef	terminalWindow = nullptr;
				HIWindowRef			window = nullptr;
				
				
				// first make the window visible if it was obscured
				window = Session_ReturnActiveWindow(session);
				terminalWindow = Session_ReturnActiveTerminalWindow(session);
				if (nullptr != terminalWindow) TerminalWindow_SetObscured(terminalWindow, false);
				
				// now select the window
				EventLoop_SelectBehindDialogWindows(window);
			}
			else
			{
				// item is a regular command, not a window
				(Boolean)MenuBar_HandleMenuCommandByID(commandID);
			}
		}
		break;
	
	case kMenuIDScripts:
		// execute script using name of menu item
		executeScriptByMenuEvent(inMenu, inMenuItemIndex);
		break;
	
	case kMenuIDEdit:		// assume it’s a MacTelnet-handled editing command (to hell with
	case kMenuIDSimpleEdit:	// desk accessories, they’re not CARBON-supported anyway)
	case kMenuIDTerminal:
	case kMenuIDKeys:
	default:
		// In the vast majority of cases, PREFER to use
		// command IDs exclusively.  Hopefully, all other
		// cases will be eliminated in future releases.
		result = MenuBar_HandleMenuCommandByID(commandID);
		break;
	}
	
	// if a menu item wasn’t handled, make this an obvious bug by leaving the menu title highlighted
	if (result) HiliteMenu(0);
	
	return result;
}// HandleMenuCommand


/*!
This method handles menu commands using the new
command IDs assigned in MacTelnet 3.0 for OS 8.
The command ID to use can be found in the file
"MenuResources.h", and the modifiers come from a
standard event record.

(3.0)
*/
Boolean
MenuBar_HandleMenuCommandByID	(UInt32		inCommandID)
{
	Boolean		result = false;
	
	
	result = Commands_ExecuteByID(inCommandID);
	return result;
}// HandleMenuCommandByID


/*!
To determine if a menu contains no item with
text identical to the specified item text,
use this method.

(3.0)
*/
Boolean
MenuBar_IsMenuItemUnique	(MenuRef			inMenu,
							 ConstStringPtr		inItemText)
{
	Boolean		result = true;
	UInt16		itemCount = CountMenuItems(inMenu),
				i = 0;
	Str255		itemString;
	
	
	// look for an identical item
	for (i = 1; ((result) && (i <= itemCount)); ++i)
	{
		GetMenuItemText(inMenu, i, itemString);
		unless (PLstrcmp(itemString, inItemText)) result = false;
	}
	
	return result;
}// IsItemUnique


/*!
To determine if a menu contains no item with
text identical to the specified item text,
use this method.  Unlike the original routine,
this one takes a CFStringRef as input.

(3.0)
*/
Boolean
MenuBar_IsMenuItemUniqueCFString	(MenuRef		inMenu,
									 CFStringRef	inItemText)
{
	Boolean			result = true;
	UInt16			itemCount = CountMenuItems(inMenu);
	UInt16			i = 0;
	CFStringRef		itemCFString = nullptr;
	OSStatus		error = noErr;
	
	
	// look for an identical item
	for (i = 1; ((result) && (i <= itemCount)); ++i)
	{
		error = CopyMenuItemTextAsCFString(inMenu, i, &itemCFString);
		if (noErr == error)
		{
			if (kCFCompareEqualTo == CFStringCompare(itemCFString, inItemText, 0/* flags */))
			{
				result = false;
			}
			CFRelease(itemCFString), itemCFString = nullptr;
		}
	}
	
	return result;
}// IsItemUniqueCFString


/*!
Attempts to create a new menu with an ID that is
not used by any other menu in the application.
The menu is NOT inserted into any menu list or
menu bar, etc., nor are there any particular
default attributes.  So, you should always choose
to initialize the resultant menu with your desired
characteristics.

If a menu cannot be created, nullptr is returned.

(3.1)
*/
MenuRef
MenuBar_NewMenuWithArbitraryID ()
{
	MenuRef		result = nullptr;
	
	
	if (noErr != CreateNewMenu(MenuBar_ReturnUniqueMenuID(), 0/* attributes */, &result))
	{
		result = nullptr;
	}
	return result;
}// NewMenuWithArbitraryID


/*!
Returns a handle to the font menu, which will have
been stripped of all proportional font names and
have the font of each menu item set to the same
font it represents.

(3.0)
*/
MenuRef
MenuBar_ReturnFontMenu ()
{
	return GetMenuRef(kPopUpMenuIDFont);
}// ReturnFontMenu


/*!
Determines the item index of a menu item that has
the specified identical text.  If the item is not
in the menu, 0 is returned.

(3.0)
*/
MenuItemIndex
MenuBar_ReturnMenuItemIndexByItemText	(MenuRef			inMenu,
										 ConstStringPtr		inItemText)
{
	MenuItemIndex const		kItemCount = CountMenuItems(inMenu);
	Str255					itemString;
	MenuItemIndex			i = 0;
	MenuItemIndex			result = 0;
	
	
	// look for an identical item
	for (i = 1; i <= kItemCount; ++i)
	{
		GetMenuItemText(inMenu, i, itemString);
		if (PLstrlen(itemString) > 0) unless (PLstrcmp(itemString, inItemText))
		{
			result = i;
			break;
		}
	}
	
	return result;
}// ReturnMenuItemIndexByItemText


/*!
Returns a menu ID that is not used by any other menu
in the application.  Use this to create new menus
that will not clobber other ones.

(3.1)
*/
MenuID
MenuBar_ReturnUniqueMenuID ()
{
	// set this to an arbitrary value that MUST NOT MATCH any other
	// menus used by the application; also, this will be increased
	// by one for each new call to this API, so ideally this is at
	// the high end of the application range so it can never clobber
	// any other menu ID after an arbitrary number of API calls
	static MenuID	gMenuIDUniquifier = 450;
	
	
	return gMenuIDUniquifier++;
}// ReturnUniqueMenuID


/*!
To flag whether or not the font menus are
available, use this method.  This is only
necessary to keep the font list rebuilding
process reasonably “thread safe”, making
sure the user can’t access fonts until it
is done its work.

(3.0)
*/
void
MenuBar_SetFontMenuAvailable		(Boolean		inIsAvailable)
{
	gFontMenusAvailable = inIsAvailable;
}// SetFontMenuAvailable


/*!
To associate a menu item with a function specifically
designed to report what its proper state should be, use
this method.  The menu item’s reference constant is used
to store the information.

See also MenuBar_SetMenuItemStateTrackerProcByCommandID().

(3.0)
*/
void
MenuBar_SetMenuItemStateTrackerProc		(MenuRef							inMenu,
										 MenuItemIndex						inItemIndex,
										 MenuCommandStateTrackerProcPtr		inProc)
{
	if ((nullptr != inMenu) && (inItemIndex > 0)) SetMenuItemRefCon(inMenu, inItemIndex, REINTERPRET_CAST(inProc, UInt32));
}// SetMenuItemStateTrackerProc


/*!
To associate a menu command with a function specifically
designed to report what its proper state should be, use
this method.  The menu item’s reference constant is used
to store the information.

This is the best way to set or remove a callback from a
menu item in MacTelnet 3.0.  It is completely independent
of menu implementation, and will always modify the correct
item even if a menu command is relocated within a menu or
moved to a different menu altogether.  Do not defeat the
purpose of the new, centralized menu management system in
MacTelnet 3.0!

(3.0)
*/
void
MenuBar_SetMenuItemStateTrackerProcByCommandID	(UInt32								inCommandID,
												 MenuCommandStateTrackerProcPtr		inProc)
{
	MenuRef			menuHandle1 = nullptr;
	MenuRef			menuHandle2 = nullptr;
	MenuItemIndex	itemIndex1 = 0;
	MenuItemIndex	itemIndex2 = 0;
	
	
	getMenusAndMenuItemIndicesByCommandID(inCommandID, &menuHandle1, &menuHandle2, &itemIndex1, &itemIndex2);
	MenuBar_SetMenuItemStateTrackerProc(menuHandle1, itemIndex1, inProc);
	MenuBar_SetMenuItemStateTrackerProc(menuHandle2, itemIndex2, inProc);
}// SetMenuItemStateTrackerProcByCommandID


/*!
This method ensures that everything about the specified
menu command (such as item text, enabled state, checked
states and menu enabled state) is correct for the context
of the program at the time this method is invoked.

(3.0)
*/
void
MenuBar_SetUpMenuItemState	(UInt32				inCommandID,
							 EventModifiers		inModifiers)
{
	// set modifiers (takes effect for all subsequent adjustMenuItem...() calls;
	// however, for the purpose of sychronizing menu item glyphs, assume the command
	// key is down (otherwise it will not appear next to certain menu items)
	adjustUsingModifiers(inModifiers | cmdKey);
	adjustMenuItemByCommandID(inCommandID);
}// SetUpMenuItemState


/*!
Updates the state of all menu titles in the menu bar.

Obsolete.

(3.0)
*/
void
MenuBar_Service ()
{
}// Service


#pragma mark Internal Methods

/*!
Adds a session’s name to the Window menu, arranging so
that future selections of the new menu item will cause
the window to be selected, and so that the item’s state
is synchronized with that of the session and its window.

Returns true only if an item was added.

(3.0)
*/
static Boolean
addWindowMenuItemForSession		(SessionRef							inSession,
								 MyMenuItemInsertionInfoConstPtr	inMenuInfoPtr,
								 CFStringRef						inWindowName)
{
	OSStatus	error = noErr;
	Boolean		result = false;
	
	
	error = InsertMenuItemTextWithCFString(inMenuInfoPtr->menu, inWindowName/* item name */,
											inMenuInfoPtr->afterItemIndex, kMenuItemAttrIgnoreMeta/* attributes */,
											0/* command ID */);
	if (noErr == error)
	{
		// set icon appropriately for the state
		setWindowMenuItemMarkForSession(inSession, inMenuInfoPtr->menu, inMenuInfoPtr->afterItemIndex + 1);
		
		// attach meta-data to the new menu command; this allows it to have its
		// checkmark set automatically, etc.
		MenuBar_SetMenuItemStateTrackerProc(inMenuInfoPtr->menu, inMenuInfoPtr->afterItemIndex + 1,
											stateTrackerWindowMenuWindowItems);
		
		// all done adding this item!
		result = true;
	}
	
	return result;
}// addWindowMenuItemForSession


/*!
This method, of standard "SessionFactory_SessionOpProcPtr"
form, adds the specified session to the Window menu.

(3.0)
*/
static void
addWindowMenuItemSessionOp	(SessionRef		inSession,
							 void*			inWindowMenuInfoPtr,
							 SInt32			UNUSED_ARGUMENT(inData2),
							 void*			inoutResultPtr)
{
	CFStringRef							nameCFString = nullptr;
	MyMenuItemInsertionInfoConstPtr		menuInfoPtr = REINTERPRET_CAST
														(inWindowMenuInfoPtr, MyMenuItemInsertionInfoConstPtr);
	MenuItemIndex*						numberOfItemsAddedPtr = REINTERPRET_CAST(inoutResultPtr, MenuItemIndex*);
	
	
	// if the session has a window, use its title if possible
	if (false == Session_GetWindowUserDefinedTitle(inSession, nameCFString).ok())
	{
		// no window yet; find a descriptive string for this session
		// (resource location will be a remote URL or local Unix command)
		nameCFString = Session_GetResourceLocationCFString(inSession);
		if (nullptr == nameCFString)
		{
			nameCFString = CFSTR("<no name or URL found>"); // LOCALIZE THIS?
		}
	}
	
	if (addWindowMenuItemForSession(inSession, menuInfoPtr, nameCFString))
	{
		++(*numberOfItemsAddedPtr);
	}
}// addWindowMenuItemSessionOp


/*!
Automatically sets the enabled state and, as appropriate,
the mark, item text, modifiers, etc. for a specific item.

You have the option of specifying the command ID that should
be used for adjustment context; you might do this if it is
not obvious from the menu and menu item number alone (for
example, a menu item that toggles between multiple commands
based on the state of modifier keys).  If you pass 0 as the
command ID, an attempt to extract the command ID from the
menu item is made.

For this method to work, you must have previously associated
a state tracking procedure with the item using
MenuBar_SetMenuItemStateTrackerProcByCommandID().

This is called by MenuBar_SetUpMenuItemState(), and is probably
not needed any place else.

(3.0)
*/
static void
adjustMenuItem	(MenuRef		inMenu,
				 MenuItemIndex	inItemNumber,
				 UInt32			inOverridingCommandIDOrZero)
{
	MenuCommand						commandID = inOverridingCommandIDOrZero;
	OSStatus						error = noErr;
	MenuCommandStateTrackerProcPtr	proc = nullptr;
	
	
	if (commandID == 0) error = GetMenuItemCommandID(inMenu, inItemNumber, &commandID);
	if (error != noErr) commandID = 0;
	
	// change the command ID to allow for variants; note that this
	// really assumes that the state tracker handling the original
	// command is also handling all variants of that command!
	(Boolean)Commands_ModifyID(&commandID, gCurrentAdjustMenuItemModifiers);
	
	// invoke the state tracker, if it exists
	getMenuItemAdjustmentProc(inMenu, inItemNumber, &proc);
	if (nullptr == proc)
	{
		// error!
	}
	else
	{
		// invoking the state tracker will trigger all item state changes
		// except the enabled state, which is returned; so, that state is
		// explicitly set following the call
		Boolean		isEnabled = MenuBar_InvokeCommandStateTracker(proc, commandID, inMenu, inItemNumber);
		
		
		setMenuItemEnabled(inMenu, inItemNumber, isEnabled);
	}
}// adjustMenuItem


/*!
Automatically sets the enabled state and, as appropriate,
the mark, item text, etc. for a menu item using only its
command ID.  Both primary and secondary menus are updated,
which means that menus specific to simplified-user-interface
mode are automatically adjusted, too.

For this method to work, you must have previously associated
a state tracking procedure with the item using
MenuBar_SetMenuItemStateTrackerProcByCommandID().

This is called by MenuBar_SetUpMenuItemState(), and is probably
not needed any place else.

(3.0)
*/
static void
adjustMenuItemByCommandID	(UInt32		inCommandID)
{
	MenuRef			menu1 = nullptr;
	MenuRef			menu2 = nullptr;
	MenuItemIndex	itemIndex1 = 0;
	MenuItemIndex	itemIndex2 = 0;
	
	
	getMenusAndMenuItemIndicesByCommandID(inCommandID, &menu1, &menu2, &itemIndex1, &itemIndex2);
	adjustMenuItem(menu1, itemIndex1, inCommandID);
	
	// some commands also appear in Simplified User Interface mode, but in different places
	if ((itemIndex2 != itemIndex1) || (menu2 != menu1)) adjustMenuItem(menu2, itemIndex2, inCommandID);
}// adjustMenuItemByCommandID


/*!
Sets the global variable used by adjustMenuItem() to
determine how to modify a command ID.  This is done for
efficiency so that the modifier information does not have
to be passed in to EVERY call.

(3.0)
*/
static void
adjustUsingModifiers	(EventModifiers		inModifiers)
{
	gCurrentAdjustMenuItemModifiers = inModifiers;
}// adjustUsingModifiers


/*!
Many menu items are all jointly enabled or disabled
based on the same basic factor - whether any session
window is frontmost.  Use this method to determine if
session-related menu items should be enabled.

(3.0)
*/
static Boolean
areSessionRelatedItemsEnabled ()
{
	Boolean		result = false;
	WindowRef	frontWindow = EventLoop_GetRealFrontWindow();
	
	
	if (nullptr != frontWindow)
	{
		short		windowKind = GetWindowKind(EventLoop_GetRealFrontWindow());
		
		
		result = ((windowKind == WIN_CNXN) || (windowKind == WIN_SHELL));
	}
	return result;
}// areSessionRelatedItemsEnabled


/*!
Many menu items are all jointly enabled or disabled
based on the same basic factor - whether any graphics
window is frontmost.  Use this method to determine if
TEK-related menu items should be enabled.

(3.0)
*/
static Boolean
areTEKRelatedItemsEnabled ()
{
	Boolean		result = false;
	WindowRef	frontWindow = EventLoop_GetRealFrontWindow();
	
	
	if (nullptr != frontWindow)
	{
		short		windowKind = GetWindowKind(EventLoop_GetRealFrontWindow());
		
		
		result = ((windowKind == WIN_CNXN) || (windowKind == WIN_SHELL));
	}
	return result;
}// areTEKRelatedItemsEnabled
	
	
/*!
This method adds menus to the menu bar, and identifies
which menus are submenus of menu items.

(3.0)
*/
static void
buildMenuBar ()
{
	// Load the NIB containing each menu (automatically finds
	// the right localizations), and create the menu bar.
	//
	// Right now, MacTelnet 3.0 wastefully grabs simple menus
	// along with regular menus, even though the two will never
	// be used simultaneously.  Maybe this could be optimized.
	{
		NIBLoader	menuFile(AppResources_ReturnBundleForNIBs(), CFSTR("MainMenus"));
		assert(menuFile.isLoaded());
		OSStatus	error = noErr;
		
		
		error = SetMenuBarFromNib(menuFile.returnNIB(), CFSTR("MenuBar"));
		assert_noerr(error);
	}
	
	// apply the appropriate key equivalents to New commands
	setNewCommand(gNewCommandShortcutEffect);
	
	// set the scripts menu to an icon title
	{
		IconManagerIconRef	icon = IconManager_NewIcon();
		
		
		if (nullptr != icon)
		{
			OSStatus	error = noErr;
			
			
			error = IconManager_MakeIconRefFromBundleFile(icon, CFSTR("IconForScriptsMenu"),
															kConstantsRegistry_ApplicationCreatorSignature,
															kConstantsRegistry_IconServicesIconMenuTitleScripts);
			if (noErr == error)
			{
				error = IconManager_SetMenuTitleIcon(GetMenuRef(kMenuIDScripts), icon);
			}
		}
	}
	
	// add pop-up menus and submenus to the submenu list
	DeleteMenu(kPopUpMenuIDFont);
	InsertMenu(GetMenuRef(kPopUpMenuIDFont), kInsertHierarchicalMenu);
	
	// ensure handlers exist for all item states
	installMenuItemStateTrackers();
}// buildMenuBar


/*!
To run a script, whose file is located in MacTelnet’s
Scripts Menu Items folder with a name identical to that
of the given item of the given menu, use this method.

(3.0)
*/
static void
executeScriptByMenuEvent	(MenuRef		inMenu,
							 MenuItemIndex	inItemNumber)
{
	FSSpec		scriptFile;
	OSStatus	error = noErr;
	
	
	error = Folder_GetFSSpec(kFolder_RefScriptsMenuItems, &scriptFile);
	if (noErr == error)
	{
		GetMenuItemText(inMenu, inItemNumber, scriptFile.name);
		
		// restore the "!" prefix for special files (it was stripped prior to placement in the menu)
		if (inItemNumber <= gNumberOfSpecialScriptMenuItems) StringUtilities_PInsert(scriptFile.name, "\p!");
		
		error = FSMakeFSSpec(scriptFile.vRefNum, scriptFile.parID, scriptFile.name, &scriptFile);
		if (error == fnfErr)
		{
			// maybe the original file had a ".scpt" extension which was stripped prior to placement in the menu
			PLstrcat(scriptFile.name, "\p.scpt");
			error = FSMakeFSSpec(scriptFile.vRefNum, scriptFile.parID, scriptFile.name, &scriptFile);
		}
		if (noErr == error) error = AppleEventUtilities_ExecuteScriptFile(&scriptFile, true/* notify user */);
	}
}// executeScriptByMenuEvent


/*!
Returns the main menu and item index for an item based only on
its command ID.  If the specified command cannot be matched with
an item index, 0 is returned.  If the specified command cannot
be matched with a menu, nullptr is returned via the specified menu
handle pointer.

IMPORTANT:	There are several utility functions in this file that
			automatically use the power of this routine to get
			and set various attributes of menu items (such as
			item text, item enabled state and checkmarks).  If
			you think this method is useful, be sure to first
			check if there is not an even more useful version that
			directly performs a command-ID-based function that you
			need.

WARNING:	This method will only return the menu handle and item
			number for the command appropriate for the current
			“mode” (i.e. simplified-user-interface or otherwise).
			To ensure you change all versions of a command, you
			should use the getMenusAndMenuItemIndicesByCommandID()
			variation of this method.  If you use the convenience
			functions in this module, you avoid this potential
			problem.

(3.0)
*/
static MenuItemIndex
getMenuAndMenuItemIndexByCommandID	(UInt32		inCommandID,
									 MenuRef*	outMenu)
{
	MenuRef			simplifiedUIModeMenu = nullptr;
	MenuItemIndex	simplifiedUIModeMenuItemIndex = 0;
	MenuItemIndex	result = 0;
	
	
	getMenusAndMenuItemIndicesByCommandID(inCommandID, outMenu, &simplifiedUIModeMenu,
											&result, &simplifiedUIModeMenuItemIndex);
	
	// If the user is in simplified-user-interface mode, then it’s the
	// secondary menus that are potentially important.  If the secondary
	// menu is nullptr, however, then the primary menu is also the menu to
	// use while in simplified-user-interface mode.
	if (gSimplifiedMenuBar && (nullptr != simplifiedUIModeMenu))
	{
		*outMenu = simplifiedUIModeMenu;
		result = simplifiedUIModeMenuItemIndex;
	}
	return result;
}// getMenuAndMenuItemIndexByCommandID


/*!
To interpret the reference constant of a menu item as if
it were a command state tracking routine, use this method.
This allows you to directly get access to a routine that
can describe an item’s state, using only the item
information itself.

(3.0)
*/
static void
getMenuItemAdjustmentProc	(MenuRef							inMenu,
							 MenuItemIndex						inItemNumber,
							 MenuCommandStateTrackerProcPtr*	outProcPtrPtr)
{
	(OSStatus)GetMenuItemRefCon(inMenu, inItemNumber, (UInt32*)outProcPtrPtr);
}// getMenuItemAdjustmentProc


/*!
Returns the two menu bar menus and menu item indices for a command
based only on its command ID.  If the specified command cannot be
matched with an item index, 0 is returned in the item index pointers.
If the specified command cannot be matched with a menu, nullptr is
returned via the specified menu handle pointers.

The “primary” menu specifications refer to the menu in normal mode,
and the “secondary” menu specifications refer to the menu in
simplified-user-interface mode.  Some menus do not have versions
for the simplified-user-interface mode, so the primary and secondary
menus are identical in those cases.  Also, sometimes an item present
in a primary menu will not be present in a secondary menu.  0 is
returned for the item index in this case, and the Mac OS Menu Manager
should safely ignore any attempts to perform menu operations with
this index.

IMPORTANT:	This is the only place in the entire program where menu
			commands are physically tied to menus, with the exception
			of the 'xmnu' resource specifications.  DO NOT DEFEAT THE
			PURPOSE OF THIS by placing similar mappings ANYWHERE ELSE.
			
			There are several utility functions in this file that
			automatically use the power of this routine to get and
			set various attributes of menu items (such as item
			text, item enabled state and checkmarks) using only a
			command ID.  If you think this method is useful, be
			sure to first check if there is an even more useful
			version that directly performs a command-ID-based
			function that you need.  In particular, you avoid the
			hassle of working with the primary and secondary menus
			if you use one of the utility functions, because the
			utility functions automatically update both menus.

(3.0)
*/
static void
getMenusAndMenuItemIndicesByCommandID	(UInt32				inCommandID,
										 MenuRef*			outPrimaryMenu,
										 MenuRef*			outSecondaryMenu,
										 MenuItemIndex*		outPrimaryItemIndex,
										 MenuItemIndex*		outSecondaryItemIndex)
{
	OSStatus	error = noErr;
	
	
	// These searches start from the root menu; it may be possible to
	// write a faster scan (no speed test has been done) by making
	// the code more dependent on known commands IDs, but this
	// approach has definite maintainability advantages.
	if (nullptr != outPrimaryMenu)
	{
		error = GetIndMenuItemWithCommandID(nullptr/* starting point */, inCommandID,
											1/* which matching item to return */,
											outPrimaryMenu/* matching menu */,
											outPrimaryItemIndex/* matching index, or nullptr */);
	}
	if (nullptr != outSecondaryMenu)
	{
		error = GetIndMenuItemWithCommandID(nullptr/* starting point */, inCommandID,
											2/* which matching item to return */,
											outSecondaryMenu/* matching menu */,
											outSecondaryItemIndex/* matching index, or nullptr */);
		if ((noErr != error) && (nullptr != outPrimaryMenu)) *outSecondaryMenu = *outPrimaryMenu;
	}
}// getMenusAndMenuItemIndicesByCommandID


/*!
Returns the session corresponding to an item index
from the Window menu.

(3.0)
*/
static HIWindowRef
getWindowFromWindowMenuItemIndex	(MenuItemIndex		inIndex)
{
	SessionFactory_ResultCode	error = kSessionFactory_ResultCodeSuccess;
	SessionRef					session = nullptr;
	HIWindowRef					result = nullptr;
	
	
	error = SessionFactory_GetSessionWithZeroBasedIndex
			(inIndex - returnFirstWindowItemAnchor(GetMenuRef(kMenuIDWindow)),
				kSessionFactory_ListInCreationOrder, &session);
	if (error == kSessionFactory_ResultCodeSuccess) result = Session_ReturnActiveWindow(session);
	return result;
}// getWindowFromWindowMenuItemIndex


/*!
Returns the index of the item in the Window menu
corresponding to the specified session’s window.

(3.0)
*/
static MenuItemIndex
getWindowMenuItemIndexForSession	(SessionRef		inSession)
{
	SessionFactory_ResultCode	error = kSessionFactory_ResultCodeSuccess;
	UInt16						sessionIndex = 0;
	MenuItemIndex				result = 0;
	
	
	error = SessionFactory_GetZeroBasedIndexOfSession
				(inSession, kSessionFactory_ListInCreationOrder, &sessionIndex);
	if (kSessionFactory_ResultCodeSuccess == error)
	{
		result = returnFirstWindowItemAnchor(GetMenuRef(kMenuIDWindow)) + sessionIndex;
	}
	
	return result;
}// getWindowMenuItemIndexForSession


/*!
After the global set of menus has been created (probably
by getting menus from resources), use this method to
install item state trackers in each one.  If you ever
re-grab a menu “fresh” from a resource file, be sure to
call this method to make sure that the new menu is set up
properly, otherwise its items’ states will not be set by
a call to MenuBar_SetUpMenuItemState().

Any menu command whose state (enabled, checked, item text,
etc.) could ever change should have a state tracker.  Only
one state tracker is allowed per command.  The trackers are
invoked by MenuBar_SetUpMenuItemState().

For convenience, the procedures are set in the exact order
they appear in menus, and grouped by menu.  Any command for
which no state tracker is used is listed anyway, as a
comment, for completeness.

(3.0)
*/
static void
installMenuItemStateTrackers ()
{
	// Apple
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandAboutThisApplication, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandURLAuthorMail, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandURLHomePage, nullptr/* no state tracker necessary */);
	
	// File
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNewSessionDefaultFavorite, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNewSessionShell, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNewSessionLoginShell, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNewSessionDialog, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandOpenSession, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandCloseConnection, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandKillProcessesKeepWindow, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSaveSession, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSaveText, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandImportMacroSet, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandExportCurrentMacroSet, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNewDuplicateSession, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHandleURL, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandPrint, stateTrackerPrintingItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandPrintOne, stateTrackerPrintingItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandPrintScreen, stateTrackerPrintingItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandPageSetup, stateTrackerPrintingItems);
	
	// Edit
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandUndo, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandRedo, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandCut, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandCopy, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandCopyTable, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandCopyAndPaste, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandPaste, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandClear, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFind, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFindAgain, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFindPrevious, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFindCursor, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSelectAll, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSelectAllWithScrollback, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSelectNothing, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowClipboard, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideClipboard, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFixCharacterTranslation, stateTrackerStandardEditItems);
	
	// View
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandLargeScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSmallScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTallScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSetScreenSize, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandBiggerText, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSmallerText, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFullScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFormat, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFormatDefault, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFullScreenModal, stateTrackerGenericSessionItems);
	
	// Terminal
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTerminalEmulatorSetup, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandDisableBell, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandEcho, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandWrapMode, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandClearScreenSavesLines, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandJumpScrolling, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandCaptureToFile, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandEndCaptureToFile, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTEKPageCommand, stateTrackerTEKItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTEKPageClearsScreen, stateTrackerTEKItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSpeechEnabled, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSpeakSelectedText, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandClearEntireScrollback, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandResetGraphicsCharacters, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandResetTerminal, stateTrackerGenericSessionItems);
	
	// Keys
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandDeletePressSendsBackspace, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandDeletePressSendsDelete, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSetKeys, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandEMACSArrowMapping, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandLocalPageUpDown, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandMacroSetNone, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTranslationTableNone, stateTrackerGenericSessionItems);
	
	// Network
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowNetworkNumbers, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendInternetProtocolNumber, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendAreYouThere, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendAbortOutput, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendBreak, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendInterruptProcess, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendEraseCharacter, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendEraseLine, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendEndOfFile, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendSync, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandWatchForActivity, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSuspendNetwork, stateTrackerCheckableItems);
	
	// Window
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandMinimizeWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandZoomWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandMaximizeWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandChangeWindowTitle, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideFrontWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideOtherWindows, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowAllHiddenWindows, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandKioskModeDisable, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandStackWindows, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNextWindow, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNextWindowHideCurrent, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandPreviousWindow, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowConnectionStatus, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideConnectionStatus, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowMacros, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideMacros, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowCommandLine, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideCommandLine, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowKeypad, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideKeypad, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowFunction, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideFunction, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowControlKeys, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideControlKeys, stateTrackerShowHideItems);
	// state trackers for connection items are installed dynamically
}// installMenuItemStateTrackers


/*!
Invoked whenever a monitored preference value is changed
(see MenuBar_Init() to see which preferences are monitored).
This routine responds by ensuring that menu states are
up to date for the changed preference.

(3.0)
*/
static void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					inEventContextPtr,
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Preferences_ChangeContext*		contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	size_t							actualSize = 0L;
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagMenuItemKeys:
		{
			Boolean		flag = false;
			
			
			unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(flag), &flag, &actualSize) ==
					kPreferences_ResultCodeSuccess)
			{
				flag = true; // assume menu items have key equivalents, if preference can’t be found
			}
			setMenusHaveKeyEquivalents(flag, contextPtr->firstCall);
		}
		break;
	
	case kPreferences_TagNewCommandShortcutEffect:
		// update internal variable that matches the value of the preference
		// (easier than calling Preferences_GetData() every time!)
		unless (Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
									sizeof(gNewCommandShortcutEffect), &gNewCommandShortcutEffect,
									&actualSize) == kPreferences_ResultCodeSuccess)
		{
			gNewCommandShortcutEffect = kCommandNewSessionDefaultFavorite; // assume command, if preference can’t be found
		}
		setNewCommand(gNewCommandShortcutEffect);
		break;
	
	case kPreferences_TagSimplifiedUserInterface:
		// update internal variable that matches the value of the preference
		// (easier than calling Preferences_GetData() every time!)
		unless (Preferences_GetData(kPreferences_TagSimplifiedUserInterface,
									sizeof(gSimplifiedMenuBar), &gSimplifiedMenuBar,
									&actualSize) == kPreferences_ResultCodeSuccess)
		{
			gSimplifiedMenuBar = false; // assume normal mode, if preference can’t be found
		}
		simplifyMenuBar(gSimplifiedMenuBar);
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Removes all key equivalents from the specified
menu item.

(3.0)
*/
static inline void
removeMenuItemModifier	(MenuRef		inMenu,
						 MenuItemIndex	inItemIndex)
{
	(OSStatus)SetMenuItemCommandKey(inMenu, inItemIndex, false/* virtual key */, '\0'/* character */);
	(OSStatus)SetMenuItemModifiers(inMenu, inItemIndex, kMenuNoCommandModifier);
	(OSStatus)SetMenuItemKeyGlyph(inMenu, inItemIndex, kMenuNullGlyph);
}// removeMenuItemModifier


/*!
To remove the key equivalents from all menu items
in a menu, call this routine.  This routine will
automatically avoid potential disasters involving
specially-encoded command keys (0x1B, etc.) as
documented in Inside Macintosh: Macintosh Toolbox
Essentials, by not touching command key fields
for items with special command key fields.

(3.0)
*/
static void
removeMenuItemModifiers		(MenuRef	inMenu)
{
	register MenuItemIndex	i = 0;
	MenuItemIndex const		kItemCount = CountMenuItems(inMenu);
	
	
	for (i = 1; i <= kItemCount; ++i) removeMenuItemModifier(inMenu, i);
}// removeMenuItemModifiers


/*!
Returns the item number that the first window-specific
item in the Window menu WOULD have.  This assumes there
is a dividing line between the last known command in the
Window menu and the would-be first window item.

(3.1)
*/
static MenuItemIndex
returnFirstWindowItemAnchor		(MenuRef	inWindowMenu)
{
	MenuItemIndex	result = 0;
	MenuRef			menuContainingMatch = nullptr;
	OSStatus		error = GetIndMenuItemWithCommandID
							(inWindowMenu, kCommandHideKeypad/* this command should be last in the menu */,
								1/* which match to return */, &menuContainingMatch, &result);
	
	
	assert_noerr(error);
	assert(menuContainingMatch == inWindowMenu);
	
	// skip the dividing line item, then point to the next item (so +2)
	result += 2;
	
	return result;
}// returnFirstWindowItemAnchor


/*!
Invoked whenever the Session Factory changes (see
MenuBar_Init() to see which states are monitored).
This routine responds by ensuring that the Window
menu is up to date.

(3.0)
*/
static void
sessionCountChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inSessionFactorySettingThatChanged,
						 void*					UNUSED_ARGUMENT(inEventContextPtr),
						 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inSessionFactorySettingThatChanged)
	{
	case kSessionFactory_ChangeNewSessionCount:
		break;
	
	default:
		// ???
		break;
	}
}// sessionCountChanged


/*!
Invoked whenever a monitored connection state is changed
(see MenuBar_Init() to see which states are monitored).
This routine responds by ensuring that menu states are
up to date for the changed connection state.

(3.0)
*/
static void
sessionStateChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inConnectionSettingThatChanged,
						 void*					inEventContextPtr,
						 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inConnectionSettingThatChanged)
	{
	case kSession_ChangeState:
		// update menu item icon to reflect new connection state
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			MenuRef const	kWindowMenu = GetMenuRef(kMenuIDWindow);
			
			
			switch (Session_GetState(session))
			{
				case kSession_StateActiveUnstable:
				case kSession_StateImminentDisposal:
					// a session is appearing or disappearing; rebuild the session items in the Window menu
					setUpWindowMenu(kWindowMenu);
					break;
				
				case kSession_StateBrandNew:
				case kSession_StateInitialized:
				case kSession_StateDead:
				case kSession_StateActiveStable:
				default:
					// existing item update; just change the icon as needed
					setWindowMenuItemMarkForSession(session, kWindowMenu);
					break;
			}
			MenuBar_Service();
		}
		break;
	
	default:
		// ???
		break;
	}
}// sessionStateChanged


/*!
Invoked whenever a monitored session state is changed
(see MenuBar_Init() to see which states are monitored).
This routine responds by ensuring that menu states are
up to date for the changed session state.

(3.0)
*/
static void
sessionWindowStateChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inSessionSettingThatChanged,
							 void*					inEventContextPtr,
							 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inSessionSettingThatChanged)
	{
	case kSession_ChangeWindowObscured:
		// update menu item icon to reflect new hidden state
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
		#if 1
			if (TerminalWindow_IsObscured(Session_ReturnActiveTerminalWindow(session)))
			{
				HIWindowRef		hiddenWindow = Session_ReturnActiveWindow(session);
				
				
				if (nullptr != hiddenWindow)
				{
					Rect	windowMenuTitleBounds;
					
					
					if (MenuBar_GetMenuTitleRectangle(kMenuBar_MenuWindow, &windowMenuTitleBounds))
					{
						Rect	structureBounds;
						
						
						// make the window zoom into the Window menu’s title area, for visual feedback
						// (an error while doing this is unimportant)
						if (noErr == GetWindowBounds(hiddenWindow, kWindowStructureRgn, &structureBounds))
						{
							ZoomRects(&structureBounds, &windowMenuTitleBounds, 14/* steps, arbitrary */, kZoomDecelerate);
						}
					}
				}
			}
		#endif
			setWindowMenuItemMarkForSession(session);
		}
		break;
	
	case kSession_ChangeWindowTitle:
		// update menu item text to reflect new window name
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			CFStringRef		text = nullptr;
			
			
			if (Session_GetWindowUserDefinedTitle(session, text) == kSession_ResultCodeSuccess)
			{
				MenuItemIndex	itemIndex = getWindowMenuItemIndexForSession(session);
				
				
				if (itemIndex > 0) SetMenuItemTextWithCFString(GetMenuRef(kMenuIDWindow), itemIndex, text);
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// sessionWindowStateChanged


/*!
To set the enabled state of an entire menu,
use this method.  This routine automatically
uses the most advanced routine possible
(namely the EnableMenuItem() or
DisableMenuItem() commands under OS 8.5 and
beyond, if available).

(3.0)
*/
#if 0
static void
setMenuEnabled	(MenuRef	inMenu,
				 Boolean	inEnabled)
{
	setMenuItemEnabled(inMenu, 0, inEnabled);
}// setMenuEnabled
#endif


/*!
To set the enabled state of a menu item or an
entire menu, use this method.  This routine
automatically uses the most advanced routine
possible (namely the EnableMenuItem() or
DisableMenuItem() commands under OS 8.5 and
beyond, if available).

(3.0)
*/
static inline void
setMenuItemEnabled		(MenuRef		inMenu,
						 UInt16			inItemNumberOrZero,
						 Boolean		inEnabled)
{
	if (inEnabled) EnableMenuItem(inMenu, inItemNumberOrZero);
	else DisableMenuItem(inMenu, inItemNumberOrZero);
}// setMenuItemEnabled


/*!
Changes the visible state of a menu item.  It
is useful, for instance, to implement Show/Hide
items by inserting both in the menu and simply
hiding whichever of the two is not applicable.

(3.1)
*/
static inline OSStatus
setMenuItemVisibility	(MenuRef			inMenu,
						 MenuItemIndex		inItemNumber,
						 Boolean			inIsVisible)
{
	OSStatus	result = noErr;
	
	
	if (inIsVisible)
	{
		result = ChangeMenuItemAttributes(inMenu, inItemNumber,
											0L, kMenuItemAttrHidden/* attributes to clear */);
	}
	else
	{
		result = ChangeMenuItemAttributes(inMenu, inItemNumber,
											kMenuItemAttrHidden/* attributes to set */, 0L);
	}
	return result;
}// setMenuItemVisibility


/*!
To remove from or restore to menu items their key
equivalents, use this method.  At startup time,
this method behaves slightly differently - therefore,
the additional parameter is set to "true" only then.

(3.0)
*/
static void
setMenusHaveKeyEquivalents	(Boolean	inMenusHaveKeyEquivalents,
							 Boolean	inVeryFirstCall)
{
	static short	lastMenuSetHadCommandKeys = -1;
	
	
	if ((lastMenuSetHadCommandKeys != inMenusHaveKeyEquivalents) || (inVeryFirstCall))
	{
		lastMenuSetHadCommandKeys = inMenusHaveKeyEquivalents;
		
		if ((inMenusHaveKeyEquivalents) && (false == inVeryFirstCall))
		{
			DeleteMenu(kMenuIDFile);
			DeleteMenu((gSimplifiedMenuBar) ? kMenuIDSimpleEdit : kMenuIDEdit);
			DeleteMenu(kMenuIDView);
			DeleteMenu(kMenuIDTerminal);
			DeleteMenu(kMenuIDKeys);
			DeleteMenu(kMenuIDWindow);
		}
		
		// Obtain fresh copies of the killed menus.  Or, if this is
		// the first call, then this is the actual menu bar setup!
		if ((inMenusHaveKeyEquivalents) || (inVeryFirstCall))
		{
			buildMenuBar();
		}
		
		if (false == inMenusHaveKeyEquivalents)
		{
			// Remove key equivalents from all menus that have them!
			removeMenuItemModifiers(GetMenuRef(kMenuIDApplication));
			removeMenuItemModifiers(GetMenuRef(kMenuIDFile));
			removeMenuItemModifiers(GetMenuRef(kMenuIDEdit));
			removeMenuItemModifiers(GetMenuRef(kMenuIDSimpleEdit));
			removeMenuItemModifiers(GetMenuRef(kMenuIDView));
			removeMenuItemModifiers(GetMenuRef(kMenuIDTerminal));
			removeMenuItemModifiers(GetMenuRef(kMenuIDKeys));
			removeMenuItemModifiers(GetMenuRef(kMenuIDWindow));
			removeMenuItemModifiers(GetMenuRef(kMenuIDKeywords));
		}
	}
	
	// since there is a possibility menus were reloaded, it
	// is necessary for this routine to perform all post-load
	// corrections to menus
	unless (inVeryFirstCall)
	{
		simplifyMenuBar(true/* tmp */);
		simplifyMenuBar(gSimplifiedMenuBar); // need to reset this since menus were reloaded
	}
	DrawMenuBar();
	setUpDynamicMenus();
}// setMenusHaveKeyEquivalents


/*!
Sets the key equivalent of the specified command
to be command-N, automatically changing other
affected commands to use something else.

This has no effect if the user preference for
no menu command keys is set.

(3.0)
*/
static void
setNewCommand	(UInt32		inCommandNShortcutCommand)
{
	size_t		actualSize = 0;
	Boolean		menuCommandKeys = false;
	
	
	// determine if menus are supposed to display key equivalents at all...
	unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(menuCommandKeys), &menuCommandKeys,
			&actualSize) == kPreferences_ResultCodeSuccess)
	{
		menuCommandKeys = true; // assume menu items have key equivalents, if preference can’t be found
	}
	
	if (menuCommandKeys)
	{
		MenuRef			newShellSessionMenu = nullptr;
		MenuRef			newShellSessionMenuSimplifiedMode = nullptr;
		MenuRef			newSessionDialogMenu = nullptr;
		MenuRef			newSessionDialogMenuSimplifiedMode = nullptr;
		MenuRef			newDefaultSessionMenu = nullptr;
		MenuRef			newDefaultSessionMenuSimplifiedMode = nullptr;
		MenuItemIndex	itemIndexNewShellSession = 0;
		MenuItemIndex	itemIndexNewShellSessionSimplifiedMode = 0;
		MenuItemIndex	itemIndexNewSessionDialog = 0;
		MenuItemIndex	itemIndexNewSessionDialogSimplifiedMode = 0;
		MenuItemIndex	itemIndexNewDefaultSession = 0;
		MenuItemIndex	itemIndexNewDefaultSessionSimplifiedMode = 0;
		
		
		// first clear the non-command-key modifiers of certain “New” commands
		getMenusAndMenuItemIndicesByCommandID(kCommandNewSessionDefaultFavorite, &newDefaultSessionMenu,
												&newDefaultSessionMenuSimplifiedMode,
												&itemIndexNewDefaultSession, &itemIndexNewDefaultSessionSimplifiedMode);
		getMenusAndMenuItemIndicesByCommandID(kCommandNewSessionShell, &newShellSessionMenu,
												&newShellSessionMenuSimplifiedMode,
												&itemIndexNewShellSession, &itemIndexNewShellSessionSimplifiedMode);
		getMenusAndMenuItemIndicesByCommandID(kCommandNewSessionDialog, &newSessionDialogMenu,
												&newSessionDialogMenuSimplifiedMode,
												&itemIndexNewSessionDialog, &itemIndexNewSessionDialogSimplifiedMode);
		
		if (nullptr != newShellSessionMenu)
		{
			removeMenuItemModifier(newShellSessionMenu, itemIndexNewShellSession);
		}
		if (nullptr != newShellSessionMenuSimplifiedMode)
		{
			removeMenuItemModifier(newShellSessionMenuSimplifiedMode, itemIndexNewShellSessionSimplifiedMode);
		}
		if (nullptr != newSessionDialogMenu)
		{
			removeMenuItemModifier(newSessionDialogMenu, itemIndexNewSessionDialog);
		}
		if (nullptr != newSessionDialogMenuSimplifiedMode)
		{
			removeMenuItemModifier(newSessionDialogMenuSimplifiedMode, itemIndexNewSessionDialogSimplifiedMode);
		}
		if (nullptr != newDefaultSessionMenu)
		{
			removeMenuItemModifier(newDefaultSessionMenu, itemIndexNewDefaultSession);
		}
		if (nullptr != newDefaultSessionMenuSimplifiedMode)
		{
			removeMenuItemModifier(newDefaultSessionMenuSimplifiedMode, itemIndexNewDefaultSessionSimplifiedMode);
		}
		
		// now assign the modifiers appropriately based on the given command
		switch (inCommandNShortcutCommand)
		{
		case kCommandNewSessionShell:
			// apply shift key equivalent to dialog-based command,
			// and the unshifted equivalent to the New Session command
			{
				MenuRef			menu = nullptr;
				MenuItemIndex	itemIndex = 0;
				
				
				menu = newSessionDialogMenu;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewSessionDialog;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
				}
				
				menu = newSessionDialogMenuSimplifiedMode;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewSessionDialogSimplifiedMode;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
				}
				
				menu = newShellSessionMenu;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewShellSession;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				}
				
				menu = newShellSessionMenuSimplifiedMode;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewShellSessionSimplifiedMode;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				}
			}
			break;
		
		case kCommandNewSessionDefaultFavorite:
			// apply shift key equivalent to dialog-based command,
			// and the unshifted equivalent to the Default item
			{
				MenuRef			menu = nullptr;
				MenuItemIndex	itemIndex = 0;
				
				
				menu = newSessionDialogMenu;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewSessionDialog;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
				}
				
				menu = newSessionDialogMenuSimplifiedMode;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewSessionDialogSimplifiedMode;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
				}
				
				menu = newDefaultSessionMenu;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewDefaultSession;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				}
				
				menu = newDefaultSessionMenuSimplifiedMode;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewDefaultSessionSimplifiedMode;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				}
			}
			break;
		
		case kCommandNewSessionDialog:
			// apply shift key equivalent to the new-shell command,
			// and the unshifted equivalent to new-session-dialog
			{
				MenuRef			menu = nullptr;
				MenuItemIndex	itemIndex = 0;
				
				
				menu = newShellSessionMenu;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewShellSession;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
				}
				
				menu = newShellSessionMenuSimplifiedMode;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewShellSessionSimplifiedMode;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
				}
				
				menu = newSessionDialogMenu;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewSessionDialog;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				}
				
				menu = newSessionDialogMenuSimplifiedMode;
				if (nullptr != menu)
				{
					itemIndex = itemIndexNewSessionDialogSimplifiedMode;
					SetItemCmd(menu, itemIndex, KEYCHAR_NEWSESSION);
					SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
}// setNewCommand


/*!
A short-cut for invoking setUpFormatFavoritesMenu() and
other setUp*Menu() routines.  In general, you should use
this routine instead of directly calling any of those.

(3.1)
*/
static void
setUpDynamicMenus ()
{
	setUpSessionFavoritesMenu(GetMenuRef(kMenuIDFile));
	setUpScreenSizeFavoritesMenu(GetMenuRef(kMenuIDView));
	setUpFormatFavoritesMenu(GetMenuRef(kMenuIDView));
	setUpMacroSetsMenu(GetMenuRef(kMenuIDKeys));
	setUpTranslationTablesMenu(GetMenuRef(kMenuIDKeys));
	setUpWindowMenu(GetMenuRef(kMenuIDWindow));
	setUpScriptsMenu(GetMenuRef(kMenuIDScripts));
}// setUpDynamicMenus


/*!
Destroys and rebuilds the menu items that automatically change
a terminal format.

(3.1)
*/
static void
setUpFormatFavoritesMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandFormatDefault, 1/* which match to return */,
										&inMenu, &defaultIndex);
	if (noErr == error)
	{
		// erase previous items
		if (0 != gNumberOfFormatMenuItemsAdded)
		{
			(OSStatus)DeleteMenuItems(inMenu, defaultIndex + 1/* first item */, gNumberOfFormatMenuItemsAdded);
		}
		
		// add the names of all defined formats to the menu;
		// update global count of items added at that location
		gNumberOfFormatMenuItemsAdded = 0;
		(Preferences_ResultCode)Preferences_InsertContextNamesInMenu(kPreferences_ClassFormat, inMenu,
																		defaultIndex, 1/* indentation level */,
																		gNumberOfFormatMenuItemsAdded);
		
		// also fix the indentation of the Default choice, as this
		// cannot be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		
		// also indent the customization item
		error = GetIndMenuItemWithCommandID(inMenu, kCommandFormat, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		}
	}
}// setUpFormatFavoritesMenu


/*!
Destroys and rebuilds the menu items that automatically change
the active macro set.

(3.1)
*/
static void
setUpMacroSetsMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandMacroSetNone, 1/* which match to return */,
										&inMenu, &defaultIndex);
	if (noErr == error)
	{
		// erase previous items
		if (0 != gNumberOfMacroSetMenuItemsAdded)
		{
			(OSStatus)DeleteMenuItems(inMenu, defaultIndex + 1/* first item */, gNumberOfMacroSetMenuItemsAdded);
		}
		
		// add the names of all macro sets to the menu;
		// update global count of items added at that location
		gNumberOfMacroSetMenuItemsAdded = 0;
		(Preferences_ResultCode)Preferences_InsertContextNamesInMenu(kPreferences_ClassMacroSet, inMenu,
																		defaultIndex, 1/* indentation level */,
																		gNumberOfMacroSetMenuItemsAdded);
		
		// also fix the indentation of the None choice, as this
		// cannot be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
	}
}// setUpMacroSetsMenu


/*!
Destroys and rebuilds the menu items that automatically open
sessions with particular Session Favorite names.

(3.1)
*/
static void
setUpScreenSizeFavoritesMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandLargeScreen, 1/* which match to return */,
										&inMenu, &defaultIndex);
	if (noErr == error)
	{
		// fix the indentation of the choices, as this cannot
		// be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		error = GetIndMenuItemWithCommandID(inMenu, kCommandSmallScreen, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
			error = GetIndMenuItemWithCommandID(inMenu, kCommandTallScreen, 1/* which match to return */,
												&inMenu, &defaultIndex);
			if (noErr == error)
			{
				(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
			}
		}
		
		// also indent the customization item
		error = GetIndMenuItemWithCommandID(inMenu, kCommandSetScreenSize, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		}
	}
}// setUpScreenSizeFavoritesSubmenu


/*!
Destroys and rebuilds the contents of the “Scripts”
menu, reading all available scripts and executables
from the Scripts Menu Items folder.

This routine sets a global flag while it is working,
so that future calls cannot initiate a rebuild if
one is already in progress.

Also, the modification time of the directory is
tracked so that future calls will not rebuild when
no changes have been made.

(3.0)
*/
static void
setUpScriptsMenu	(MenuRef	inMenu)
{
	static Boolean		gScriptsMenuRebuilding = false; // flags to keep things “thread safe”
	
	
	if (false == gScriptsMenuRebuilding)
	{
		MenuRef		menu = inMenu;
		
		
		gScriptsMenuRebuilding = true;
	#define SCRIPTS_MENU_PROGRESS_WINDOW 1
		if (nullptr != menu)
		{
			static unsigned long	lastModDate = 0L;
			unsigned long			date = 0L;
			FSSpec					scriptsFolder;
			OSStatus				error = noErr;
			Boolean					canUseScriptsMenu = true; // if not, delete the menu
			
			
			// locate the Scripts folder, and find out when it was last modified
			error = Folder_GetFSSpec(kFolder_RefScriptsMenuItems, &scriptsFolder);
			date = FileUtilities_GetDirectoryDateFromFSSpec(&scriptsFolder, kFileUtilitiesDateOfModification);
			
			// only rebuild the Scripts menu if the Scripts Menu Items folder was found
			// and the user has actually changed the folder contents
			if (error != noErr) canUseScriptsMenu = false;
			
			if ((canUseScriptsMenu) && (lastModDate != date))
			{
				enum
				{
					kMaximumScriptsInMenu = 31		// no more than this many script files will be listed in the menu
				};
				FSSpec*				array = (FSSpec*)Memory_NewPtr(kMaximumScriptsInMenu * sizeof(FSSpec));
				UInt32				arrayLength = kMaximumScriptsInMenu;
				register UInt32		i = 0L;
				register UInt16		j = 0;
				FileInfo			info;
				Boolean				useFile = true,
									firstScript = true,		// as soon as a divider is put in the menu, this flag overturns
									anyTopItems = false;	// set if there are any script filenames beginning with "!"
			#if SCRIPTS_MENU_PROGRESS_WINDOW
				ProgressDialogRef	progressDialog = nullptr;
			#endif
				SInt16				numberOfSpecialScriptMenuItems = 0;
				SInt16				oldResFile = CurResFile();
				
				
			#if SCRIPTS_MENU_PROGRESS_WINDOW
				// create a progress dialog
				{
					Str255			text;
					
					
					GetIndString(text, rStringsStatus, siStatusBuildingScriptsMenu);
					progressDialog = ProgressDialog_New(text, false/* modal */);
					
					// specify a title for the Dock’s menu
					{
						CFStringRef				titleCFString = nullptr;
						UIStrings_ResultCode	stringResult = UIStrings_Copy
																(kUIStrings_ScriptsMenuProgressWindowIconName,
																	titleCFString);
						
						
						if (stringResult == kUIStrings_ResultCodeSuccess)
						{
							SetWindowAlternateTitle(ProgressDialog_ReturnWindow(progressDialog), titleCFString);
							CFRelease(titleCFString);
						}
					}
				}
				ProgressDialog_SetProgressIndicator(progressDialog, kTelnetProgressIndicatorIndeterminate);
				
				// show the progress window
				ProgressDialog_Display(progressDialog);
			#endif
				
				// start out by deleting all items from the menu
				(OSStatus)DeleteMenuItems(menu, 1/* first item */, CountMenuItems(menu));
				
				// synchronously search for scripts
				error = FileUtilities_GetTypedFilesInDirectory(&scriptsFolder, 'osas', 'ToyS', array, &arrayLength);
				if (error != noErr) arrayLength = 0;
				
				// Iterate through all found files, adding each one to the menu.
				if (noErr == error)
				{
					for (numberOfSpecialScriptMenuItems = 0, i = 0, j = 0; i < arrayLength; ++i)
					{
						useFile = true;
						if (FSpGetFInfo(&array[i], REINTERPRET_CAST(&info, FInfo*)) == noErr)
						{
							// skip invisible files
							useFile = (!(info.finderFlags & kIsInvisible));
						}
						if (useFile)
						{
							MenuItemIndex	insertAfterIndex = 0;
							
							
							++j;
							if (array[i].name[1] == '!')
							{
								// any file names starting with "!" go in the special, top half of the menu
								insertAfterIndex = numberOfSpecialScriptMenuItems++;
								StringUtilities_PClipHead(array[i].name, 1); // remove the "!"
								anyTopItems = true;
								if (firstScript)
								{
									// add a divider, as well
									++j;
									InsertMenuItem(menu, "\p(-", 0);
									firstScript = false;
								}
							}
							else
							{
								insertAfterIndex = j - 1;	// add to the bottom of the menu
							}
							
							// check for a ".scpt" suffix, and strip it if it exists
							if (StringUtilities_PEndsWith(array[i].name, "\p.scpt"))
							{
								StringUtilities_PClipTail(array[i].name, 5/* length of ".scpt" */);
							}
							
							// insert some item, then change its text; this avoids Menu Manager “pre-processing”
							InsertMenuItem(menu, "\p<script>", insertAfterIndex);
							SetMenuItemText(menu, insertAfterIndex + 1, array[i].name);
						}
					}
				}
				gNumberOfSpecialScriptMenuItems = numberOfSpecialScriptMenuItems;
				Memory_DisposePtr((Ptr*)&array);
				
				// if there are no scripts, remove the Scripts menu
				if (arrayLength < 1) canUseScriptsMenu = false;
				
			#if SCRIPTS_MENU_PROGRESS_WINDOW
				ProgressDialog_Dispose(&progressDialog);
			#endif
				UseResFile(oldResFile);
			}
			
			// if for some reason the Scripts menu cannot be used, delete it
			if (0 == CountMenuItems(menu))
			{
				DeleteMenu(kMenuIDScripts);
				DrawMenuBar();
			}
			
			lastModDate = date; // remember this for next time (it could save a lot of work!)
		}
		
		gScriptsMenuRebuilding = false;
	}
}// setUpScriptsMenu


/*!
Destroys and rebuilds the menu items that automatically open
sessions with particular Session Favorite names.

(3.1)
*/
static void
setUpSessionFavoritesMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandNewSessionDefaultFavorite, 1/* which match to return */,
										&inMenu, &defaultIndex);
	if (noErr == error)
	{
		// erase previous items
		if (0 != gNumberOfSessionMenuItemsAdded)
		{
			(OSStatus)DeleteMenuItems(inMenu, defaultIndex + 1/* first item */, gNumberOfSessionMenuItemsAdded);
		}
		
		// add the names of all session configurations to the menu;
		// update global count of items added at that location
		gNumberOfSessionMenuItemsAdded = 0;
		(Preferences_ResultCode)Preferences_InsertContextNamesInMenu(kPreferences_ClassSession, inMenu,
																		defaultIndex, 1/* indentation level */,
																		gNumberOfSessionMenuItemsAdded);
		
		// also fix the indentation of the Default choice, as this
		// cannot be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		
		// also indent related items
		error = GetIndMenuItemWithCommandID(inMenu, kCommandNewSessionShell, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		}
		error = GetIndMenuItemWithCommandID(inMenu, kCommandNewSessionLoginShell, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		}
		error = GetIndMenuItemWithCommandID(inMenu, kCommandNewSessionDialog, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		}
	}
	
	// check to see if a key equivalent should be applied
	setNewCommand(gNewCommandShortcutEffect);
}// setUpSessionFavoritesSubmenu


/*!
Destroys and rebuilds the menu items that automatically change
the active translation table.

(3.1)
*/
static void
setUpTranslationTablesMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandTranslationTableNone, 1/* which match to return */,
										&inMenu, &defaultIndex);
	if (noErr == error)
	{
		// erase previous items
		if (0 != gNumberOfTranslationTableMenuItemsAdded)
		{
			(OSStatus)DeleteMenuItems(inMenu, defaultIndex + 1/* first item */, gNumberOfTranslationTableMenuItemsAdded);
		}
		
		// add the names of all translation tables to the menu;
		// update global count of items added at that location
		gNumberOfTranslationTableMenuItemsAdded = 0;
		//(Preferences_ResultCode)Preferences_InsertContextNamesInMenu(kPreferences_ClassTranslationTable, inMenu,
		//																defaultIndex, 1/* indentation level */,
		//																gNumberOfTranslationTableMenuItemsAdded);
		
		// also fix the indentation of the None choice, as this
		// cannot be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
	}
}// setUpTranslationTablesMenu


/*!
Destroys and rebuilds the menu items representing open session
windows and their states.

(3.1)
*/
static void
setUpWindowMenu		(MenuRef	inMenu)
{
	MenuItemIndex const			kFirstWindowItemIndex = returnFirstWindowItemAnchor(inMenu);
	MenuItemIndex const			kDividerIndex = kFirstWindowItemIndex - 1;
	MenuItemIndex const			kPreDividerIndex = kDividerIndex - 1;
	MyMenuItemInsertionInfo		insertWhere;
	
	
	// set up callback data
	bzero(&insertWhere, sizeof(insertWhere));
	insertWhere.menu = inMenu;
	insertWhere.afterItemIndex = kDividerIndex;
	
	// erase previous items
	if (0 != gNumberOfWindowMenuItemsAdded)
	{
		(OSStatus)DeleteMenuItems(inMenu, kDividerIndex/* first item */,
									gNumberOfWindowMenuItemsAdded +
										gNumberOfSpecialWindowMenuItemsAdded/* divider too */);
	}
	
	// add the names of all open session windows to the menu;
	// update global count of items added at that location
	gNumberOfWindowMenuItemsAdded = 0;
	SessionFactory_ForEachSessionDo(kSessionFactory_SessionFilterFlagAllSessions &
										~kSessionFactory_SessionFilterFlagConsoleSessions,
									addWindowMenuItemSessionOp,
									&insertWhere/* data 1: menu info */, 0L/* data 2: undefined */,
									&gNumberOfWindowMenuItemsAdded/* result: MenuItemIndex*, number of items added */);
	
	// if any were added, include a dividing line (note also that this
	// item must be counted above in the code to erase the old items)
	gNumberOfSpecialWindowMenuItemsAdded = 0;
	if (gNumberOfWindowMenuItemsAdded > 0)
	{
		if (noErr == InsertMenuItemTextWithCFString(inMenu, CFSTR("-")/* divider */, kPreDividerIndex,
													0L/* attributes */, 0L/* command ID */))
		{
			++gNumberOfSpecialWindowMenuItemsAdded;
		}
	}
}// setUpWindowMenu


/*!
Worker routine for setWindowMenuItemMarkForSession().

Use this method to set the item mark for the menu item
representing the specified session so the mark reflects
the connection state.  In MacTelnet 3.0, connection
menu items are now “marked” with icons.  Consequently,
their use requires allocation of memory.  At shutdown
time, call this method with a menu mark equal to
"kMy_WindowMenuMarkTypeDispose" to dispose of any memory
that this routine might allocate.  (This is automatically
performed by MenuBar_Done().)

IMPORTANT:	The value of "inSessionOrNull" may only be
			nullptr when the mark type reflects a global
			operation - currently, the only global
			operation is "kMy_WindowMenuMarkTypeDispose".

(3.0)
*/
static void
setWindowMenuItemMark	(MenuRef				inMenu,
						 MenuItemIndex			inItemIndex,
						 My_WindowMenuMarkType	inMarkType)
{
	static IconSuiteRef		liveConnectionIcons = nullptr;
	static IconSuiteRef		openingConnectionIcons = nullptr;
	static IconSuiteRef		deadConnectionIcons = nullptr;
	
	
	if (inMarkType == kMy_WindowMenuMarkTypeDispose)
	{
		// destroy allocated icons
		if (nullptr != liveConnectionIcons)
		{
			(OSStatus)DisposeIconSuite(liveConnectionIcons, false/* dispose icon data */), liveConnectionIcons = nullptr;
		}
		if (nullptr != openingConnectionIcons)
		{
			(OSStatus)DisposeIconSuite(openingConnectionIcons, false/* dispose icon data */), openingConnectionIcons = nullptr;
		}
		if (nullptr != deadConnectionIcons)
		{
			(OSStatus)DisposeIconSuite(deadConnectionIcons, false/* dispose icon data */), deadConnectionIcons = nullptr;
		}
	}
	else
	{
		// the menu and item must be properly defined for these cases!
		assert(nullptr != inMenu);
		assert(0 != inItemIndex);
		OSStatus		error = noErr;
		Boolean			doSet = false;
		IconSuiteRef	icons = nullptr;
		
		
		// set up icons as necessary
		switch (inMarkType)
		{
		case kMy_WindowMenuMarkTypeSessionLive:
			if (nullptr == liveConnectionIcons)
			{
				error = GetIconSuite(&liveConnectionIcons, kIconSuiteConnectionActive, kSelectorAllSmallData);
				doSet = (noErr == error);
				unless (doSet) liveConnectionIcons = nullptr;
			}
			else doSet = true;
			icons = liveConnectionIcons;
			break;
		
		case kMy_WindowMenuMarkTypeSessionOpening:
			if (nullptr == openingConnectionIcons)
			{
				error = GetIconSuite(&openingConnectionIcons, kIconSuiteConnectionOpening, kSelectorAllSmallData);
				doSet = (noErr == error);
				unless (doSet) openingConnectionIcons = nullptr;
			}
			else doSet = true;
			icons = openingConnectionIcons;
			break;
		
		case kMy_WindowMenuMarkTypeSessionDead:
			if (nullptr == deadConnectionIcons)
			{
				error = GetIconSuite(&deadConnectionIcons, kIconSuiteConnectionDead, kSelectorAllSmallData);
				doSet = (noErr == error);
				unless (doSet) deadConnectionIcons = nullptr;
			}
			else doSet = true;
			icons = deadConnectionIcons;
			break;
		
		case kMy_WindowMenuMarkTypeDisableIcon:
		case kMy_WindowMenuMarkTypeEnableIcon:
		default:
			break;
		}
		
		if (inMarkType == kMy_WindowMenuMarkTypeEnableIcon)
		{
			// set the style of the item to normal, undoing the style setting of "kMy_WindowMenuMarkTypeDisableIcon"
			SetItemStyle(inMenu, inItemIndex, normal);
			
			// enabling the icon itself is only possible with Mac OS 8.5 or later
			EnableMenuItemIcon(inMenu, inItemIndex);
		}
		else if (inMarkType == kMy_WindowMenuMarkTypeDisableIcon)
		{
			// set the style of the item to italic, which makes it more obvious that a window is hidden
			SetItemStyle(inMenu, inItemIndex, italic);
			
			// disabling the icon itself is only possible with Mac OS 8.5 or later
			DisableMenuItemIcon(inMenu, inItemIndex);
		}
		else
		{
			if (doSet)
			{
				// set icon of Window menu item
				error = SetMenuItemIconHandle(inMenu, inItemIndex, kMenuIconSuiteType, icons);
			}
		}
	}
}// setWindowMenuItemMark


/*!
Looks at the terminal window obscured state, session
status, and anything else about the given session
that is significant from a state point of view, and
then updates its Window menu item appropriately.

If you do not provide the menu, the global Window menu
is used.  If you set the item index to zero, then the
proper item for the given session is calculated.

If you already know the menu and item for a session,
you should provide those parameters to speed things up.

(3.1)
*/
static void
setWindowMenuItemMarkForSession		(SessionRef		inSession,
									 MenuRef		inWindowMenuOrNull,
									 MenuItemIndex	inItemIndexOrZero)
{
	assert(nullptr != inSession);
	My_WindowMenuMarkType	menuMark = kMy_WindowMenuMarkTypeSessionLive;
	MenuRef					menu = (nullptr == inWindowMenuOrNull)
									? GetMenuRef(kMenuIDWindow)
									: inWindowMenuOrNull;
	MenuItemIndex			itemIndex = (0 == inItemIndexOrZero)
										? getWindowMenuItemIndexForSession(inSession)
										: inItemIndexOrZero;
	
	
	// first, set the icon
	switch (Session_GetState(inSession))
	{
	case kSession_StateBrandNew:
	case kSession_StateInitialized:
		menuMark = kMy_WindowMenuMarkTypeSessionOpening;
		break;
	
	case kSession_StateDead:
	case kSession_StateImminentDisposal:
		menuMark = kMy_WindowMenuMarkTypeSessionDead;
		break;
	
	case kSession_StateActiveUnstable:
	case kSession_StateActiveStable:
		menuMark = kMy_WindowMenuMarkTypeSessionLive;
		break;
	
	default:
		// ???
		break;
	}
	setWindowMenuItemMark(menu, itemIndex, menuMark);
	
	// now, set the disabled/enabled state
	{
		TerminalWindowRef	terminalWindow = Session_ReturnActiveTerminalWindow(inSession);
		
		
		if (TerminalWindow_IsObscured(terminalWindow))
		{
			setWindowMenuItemMark(menu, itemIndex, kMy_WindowMenuMarkTypeDisableIcon);
		}
		else
		{
			setWindowMenuItemMark(menu, itemIndex, kMy_WindowMenuMarkTypeEnableIcon);
		}
	}
}// setWindowMenuItemMarkForSession


/*!
Sets the menus appropriately based on whether or not the user
requested simplified-user-interface mode.  In simplified-
user-interface mode, there are fewer menu items and fewer
command key equivalents to make the interface less daunting
and easier for novices to use.

(3.0)
*/
static void
simplifyMenuBar		(Boolean		inSetToSimplifiedMode)
{
	if (inSetToSimplifiedMode)
	{
		// simplify menus
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDEdit), kMenuAttrHidden/* attributes to set */,
										0/* attributes to clear */);
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDTerminal), kMenuAttrHidden/* attributes to set */,
										0/* attributes to clear */);
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDKeys), kMenuAttrHidden/* attributes to set */,
										0/* attributes to clear */);
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDSimpleEdit), 0/* attributes to set */,
										kMenuAttrHidden/* attributes to clear */);
	}
	else
	{
		// change menus back to normal
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDSimpleEdit), kMenuAttrHidden/* attributes to set */,
										0/* attributes to clear */);
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDEdit), 0/* attributes to set */,
										kMenuAttrHidden/* attributes to clear */);
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDTerminal), 0/* attributes to set */,
										kMenuAttrHidden/* attributes to clear */);
		(OSStatus)ChangeMenuAttributes(GetMenuRef(kMenuIDKeys), 0/* attributes to set */,
										kMenuAttrHidden/* attributes to clear */);
	}
	DrawMenuBar();
}// simplifyMenuBar


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines whether the specified item should be
enabled, and automatically sets its checkmark properly.

(3.0)
*/
static Boolean
stateTrackerCheckableItems		(UInt32				inCommandID,
								 MenuRef			inMenu,
								 MenuItemIndex		inItemNumber)
{
	SessionRef				currentSession = nullptr;
	TerminalScreenRef		currentScreen = nullptr;
	ConnectionDataPtr		currentConnectionDataPtr = nullptr;
	Boolean					connectionCommandResult = areSessionRelatedItemsEnabled();
	Boolean					checked = false;
	Boolean					result = false;
	
	
	// set up convenient data pointers
	if (areSessionRelatedItemsEnabled())
	{
		WindowRef		frontWindow = EventLoop_GetRealFrontWindow();
		
		
		if (nullptr != frontWindow)
		{
			TerminalWindowRef		terminalWindow = TerminalWindow_ReturnFromWindow(frontWindow);
			
			
			if (nullptr != terminalWindow)
			{
				currentSession = SessionFactory_ReturnUserFocusSession();
				currentScreen = (nullptr == terminalWindow) ? nullptr : TerminalWindow_ReturnScreenWithFocus(terminalWindow);
				currentConnectionDataPtr = (nullptr == currentSession) ? nullptr : Session_ConnectionDataPtr(currentSession); // DEPRECATED
			}
		}
	}
	
	switch (inCommandID)
	{
	case kCommandSpeechEnabled:
		result = connectionCommandResult;
		if (nullptr != currentSession)
		{
			checked = (result) ? Session_SpeechIsEnabled(currentSession) : false;
		}
		break;
	
	case kCommandDisableBell:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? !Terminal_BellIsEnabled(currentScreen) : false;
		break;
	
	case kCommandEcho:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Session_LocalEchoIsEnabled(currentSession) : false;
		break;
	
	case kCommandWrapMode:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Terminal_LineWrapIsEnabled(currentScreen) : false;
		break;
	
	case kCommandClearScreenSavesLines:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Terminal_SaveLinesOnClearIsEnabled(currentScreen) : false;
		break;
	
	case kCommandDeletePressSendsBackspace:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? (!currentConnectionDataPtr->bsdel) : false;
		break;
	
	case kCommandDeletePressSendsDelete:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? (currentConnectionDataPtr->bsdel) : false;
		break;
	
	case kCommandEMACSArrowMapping:
		// this is a VT220-only option, so disable the command otherwise
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		result = connectionCommandResult;
		if (result) result = Terminal_EmulatorIsVT220(currentScreen);
		if (nullptr != currentSession) checked = (result) ? (currentConnectionDataPtr->arrowmap) : false;
		break;
	
	case kCommandLocalPageUpDown:
		// this is a VT220-only option, so disable the command otherwise
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		result = connectionCommandResult;
		if (result) result = Terminal_EmulatorIsVT220(currentScreen);
		if (nullptr != currentSession) checked = (result) ? Session_PageKeysControlTerminalView(currentSession) : false;
		break;
	
	case kCommandWatchForActivity:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Session_ActivityNotificationIsEnabled(currentSession) : false;
		break;
	
	case kCommandSuspendNetwork:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Session_NetworkIsSuspended(currentSession) : false;
		break;
	
	default:
		// unknown command!
		break;
	}
	
	CheckMenuItem(inMenu, inItemNumber, checked);
	
	return result;
}// stateTrackerCheckableItems


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines whether the specified session-related
commands are supposed to be enabled.  See also the
stateTrackerCheckableItems() method, which handles some
session-related checkmarked items.

(3.0)
*/
static Boolean
stateTrackerGenericSessionItems		(UInt32				inCommandID,
									 MenuRef			inMenu,
									 MenuItemIndex		inItemNumber)
{
	WindowRef	frontWindow = EventLoop_GetRealFrontWindow();
	Boolean		result = false;
	
	
	switch (inCommandID)
	{
	case kCommandCloseConnection:
	case kCommandKillProcessesKeepWindow:
		// this applies to any window with a close box
		result = (nullptr != frontWindow);
		if (result)
		{
			WindowAttributes	attributes = kWindowNoAttributes;
			
			
			if (noErr == GetWindowAttributes(frontWindow, &attributes))
			{
				result = (0 != (attributes & kWindowCloseBoxAttribute));
			}
		}
		break;
	
	case kCommandCaptureToFile:
	case kCommandEndCaptureToFile:
		result = areSessionRelatedItemsEnabled();
		{
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(EventLoop_GetRealFrontWindow());
			
			
			if ((result) && (nullptr != terminalWindow))
			{
				TerminalScreenRef	screen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
				
				
				result = Terminal_FileCaptureInProgress(screen);
				if (kCommandCaptureToFile == inCommandID)
				{
					// begin-capture command has opposite state of file capture in progress
					result = !result;
				}
				setMenuItemVisibility(inMenu, inItemNumber, result);
			}
		}
		break;
	
	case kCommandSaveSession:
	case kCommandNewDuplicateSession:
	case kCommandFindCursor:
	case kCommandLargeScreen:
	case kCommandSmallScreen:
	case kCommandTallScreen:
	case kCommandSetScreenSize:
	case kCommandBiggerText:
	case kCommandSmallerText:
	case kCommandFullScreen:
	case kCommandFormat:
	case kCommandFormatDefault:
	case kCommandFullScreenModal:
	case kCommandTerminalEmulatorSetup:
	case kCommandJumpScrolling:
	case kCommandSetKeys:
	case kCommandMacroSetNone:
	case kCommandClearEntireScrollback:
	case kCommandResetGraphicsCharacters:
	case kCommandResetTerminal:
	case kCommandChangeWindowTitle:
	case kCommandStackWindows:
	case kCommandNextWindow:
	case kCommandNextWindowHideCurrent:
	case kCommandPreviousWindow:
	case kCommandPreviousWindowHideCurrent:
		result = areSessionRelatedItemsEnabled();
		if (inCommandID == kCommandFormat)
		{
			// this item isn’t available if the “rebuilding font list” thread is still running
			result = (result && gFontMenusAvailable);
		}
		else if ((inCommandID == kCommandSetScreenSize) ||
					(inCommandID == kCommandChangeWindowTitle))
		{
			// these items are also available for the console window
			result = TerminalWindow_ExistsFor(frontWindow);
		}
		else if ((inCommandID == kCommandNextWindow) ||
				(inCommandID == kCommandNextWindowHideCurrent) ||
				(inCommandID == kCommandPreviousWindow) ||
				(inCommandID == kCommandPreviousWindowHideCurrent))
		{
			result = (result && (nullptr != GetNextWindow(EventLoop_GetRealFrontWindow())));
		}
		break;
	
	default:
		// unknown command!
		break;
	}
	return result;
}// stateTrackerGenericSessionItems


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines whether the specified Network menu
data-sending item should be enabled.

(3.0)
*/
static Boolean
stateTrackerNetSendItems	(UInt32				inCommandID,
							 MenuRef			UNUSED_ARGUMENT(inMenu),
							 MenuItemIndex		UNUSED_ARGUMENT(inItemNumber))
{
	Boolean			result = false;
	
	
	switch (inCommandID)
	{
	case kCommandSendInternetProtocolNumber:
	case kCommandSendAreYouThere:
	case kCommandSendAbortOutput:
	case kCommandSendBreak:
	case kCommandSendInterruptProcess:
	case kCommandSendEraseCharacter:
	case kCommandSendEraseLine:
	case kCommandSendEndOfFile:
	case kCommandSendSync:
		result = areSessionRelatedItemsEnabled();
		break;
	
	default:
		// unknown command!
		break;
	}
	return result;
}// stateTrackerNetSendItems


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines whether the specified Network menu
data-sending item should be enabled.

(3.0)
*/
static Boolean
stateTrackerPrintingItems	(UInt32				inCommandID,
							 MenuRef			UNUSED_ARGUMENT(inMenu),
							 MenuItemIndex		UNUSED_ARGUMENT(inItemNumber))
{
	Boolean		result = false;
	
	
	switch (inCommandID)
	{
	case kCommandPrint:
	case kCommandPrintOne:
	case kCommandPrintScreen:
	case kCommandPageSetup:
		{
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(EventLoop_GetRealFrontWindow());
			
			
			result = (nullptr != terminalWindow);
			if (result)
			{
				// for selection-dependent Print items, make sure text is selected
				if ((kCommandPrint == inCommandID) ||
					(kCommandPrintOne == inCommandID))
				{
					TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
					
					
					result = TerminalView_TextSelectionExists(view);
				}
			}
			else if (kCommandPrint == inCommandID)
			{
				// TEK graphics can also be printed
				SInt16		graphicNumber = 0;
				
				
				result = TektronixRealGraphics_IsRealGraphicsWindow(EventLoop_GetRealFrontWindow(), &graphicNumber);
			}
		}
		break;
	
	default:
		// unknown command!
		break;
	}
	return result;
}// stateTrackerPrintingItems


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines whether the specified show/hide item
should be enabled, and sets its item text appropriately.

(3.0)
*/
static Boolean
stateTrackerShowHideItems	(UInt32			inCommandID,
							 MenuRef		inMenu,
							 MenuItemIndex	inItemNumber)
{
	Boolean		result = false;
	
	
	switch (inCommandID)
	{
	case kCommandFind:
	case kCommandFindAgain:
	case kCommandFindPrevious:
		{
			WindowRef		window = EventLoop_GetRealFrontWindow();
			
			
			if (TerminalWindow_ExistsFor(window))
			{
				result = true;
			}
			else
			{
				// not applicable to window
				result = false;
			}
		}
		break;
	
	case kCommandShowClipboard:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber, false == Clipboard_WindowIsVisible());
		break;
	
	case kCommandHideClipboard:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber, Clipboard_WindowIsVisible());
		break;
	
	case kCommandMinimizeWindow:
		{
			WindowRef	frontWindow = EventLoop_GetRealFrontWindow();
			
			
			result = (nullptr != frontWindow); // there must be at least one window
		}
		break;
	
	case kCommandZoomWindow:
	case kCommandMaximizeWindow:
		{
			WindowRef	frontWindow = EventLoop_GetRealFrontWindow();
			
			
			result = (nullptr != frontWindow); // there must be at least one window
			if (result)
			{
				WindowAttributes	attributes = kWindowNoAttributes;
				
				
				if (noErr == GetWindowAttributes(frontWindow, &attributes))
				{
					result = (0 != (attributes & kWindowFullZoomAttribute));
				}
			}
		}
		break;
	
	case kCommandHideFrontWindow:
		result = (areSessionRelatedItemsEnabled()/* implicitly checks for a frontmost terminal window */ &&
					SessionFactory_CountIsAtLeastOne()); // there must be at least one connection window open!
		break;
	
	case kCommandHideOtherWindows:
		// TMP - incomplete, should really be disabled if all other terminal windows are hidden
		result = (areSessionRelatedItemsEnabled()/* implicitly checks for a frontmost terminal window */ &&
					(SessionFactory_GetCount() > 1)); // there must be multiple connection windows open!
		break;
	
	case kCommandShowAllHiddenWindows:
		// TMP - incomplete, should really be disabled unless some window is hidden
		result = true;
		break;
	
	case kCommandKioskModeDisable:
		result = FlagManager_Test(kFlagKioskMode);
		break;
	
	case kCommandShowConnectionStatus:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber, false == InfoWindow_IsVisible());
		break;
	
	case kCommandHideConnectionStatus:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber, InfoWindow_IsVisible());
		break;
	
	case kCommandShowMacros:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								false == IsWindowVisible(MacroSetupWindow_GetWindow()));
		break;
	
	case kCommandHideMacros:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								IsWindowVisible(MacroSetupWindow_GetWindow()));
		break;
	
	case kCommandShowCommandLine:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber, (false == CommandLine_IsVisible()));
		break;
	
	case kCommandHideCommandLine:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber, CommandLine_IsVisible());
		break;
	
	case kCommandShowControlKeys:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								false == IsWindowVisible(Keypads_GetWindow(kKeypads_WindowTypeControlKeys)));
		break;
	
	case kCommandHideControlKeys:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								IsWindowVisible(Keypads_GetWindow(kKeypads_WindowTypeControlKeys)));
		break;
	
	case kCommandShowFunction:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								false == IsWindowVisible(Keypads_GetWindow(kKeypads_WindowTypeFunctionKeys)));
		break;
	
	case kCommandHideFunction:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								IsWindowVisible(Keypads_GetWindow(kKeypads_WindowTypeFunctionKeys)));
		break;
	
	case kCommandShowKeypad:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								false == IsWindowVisible(Keypads_GetWindow(kKeypads_WindowTypeVT220Keys)));
		break;
	
	case kCommandHideKeypad:
		result = true;
		setMenuItemVisibility(inMenu, inItemNumber,
								IsWindowVisible(Keypads_GetWindow(kKeypads_WindowTypeVT220Keys)));
		break;
	
	default:
		// unknown command!
		break;
	}
	return result;
}// stateTrackerShowHideItems


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines whether the specified standard Edit menu
item (Undo, Redo, Cut, Copy, Paste, Clear, Select All, or
a variation) should be enabled.  It also handles the
save-text and handle-URL commands, since they depend on
whether text is selected, much as Copy does.

(3.0)
*/
static Boolean
stateTrackerStandardEditItems	(UInt32			inCommandID,
								 MenuRef		inMenu,
								 MenuItemIndex	inItemNumber)
{
	TerminalViewRef		currentTerminalView = nullptr;
	WindowRef			frontWindow = EventLoop_GetRealFrontWindow();
	Boolean				isDialog = false;
	Boolean				isReadOnly = false;
	Boolean				isTerminal = false;
	Boolean				isTEK = false;
	SInt16				tekGraphicID = -1;
	Boolean				result = false;
	
	
	// determine any window properties that might affect Edit items
	if (nullptr != frontWindow)
	{
		isDialog = (GetWindowKind(frontWindow) == kDialogWindowKind);
		isReadOnly = (GetWindowKind(frontWindow) == WIN_CONSOLE); // is the frontmost window read-only?
		isTerminal = TerminalWindow_ExistsFor(frontWindow); // terminal windows include the FTP log, too
		isTEK = TektronixRealGraphics_IsRealGraphicsWindow(frontWindow, &tekGraphicID);
	}
	
	currentTerminalView = TerminalView_ReturnUserFocusTerminalView();
	switch (inCommandID)
	{
	case kCommandUndo:
		// set up the Edit menu properly to reflect the action
		// (if any) that can be undone
		{
			CFStringRef		commandTextCFString = nullptr;
			
			
			Undoables_GetUndoCommandInfo(commandTextCFString, &result);
			SetMenuItemTextWithCFString(inMenu, inItemNumber, commandTextCFString);
		}
		break;
	
	case kCommandRedo:
		// set up the Edit menu properly to reflect the action
		// (if any) that can be redone
		{
			CFStringRef		commandTextCFString = nullptr;
			
			
			Undoables_GetRedoCommandInfo(commandTextCFString, &result);
			SetMenuItemTextWithCFString(inMenu, inItemNumber, commandTextCFString);
		}
		break;
	
	case kCommandCut:
	case kCommandClear:
		if (isDialog) result = true;
		else result = false; // MacTelnet does not support these commands at all
		break;
	
	case kCommandSaveText:
	case kCommandHandleURL:
	case kCommandCopy:
	case kCommandCopyTable:
	case kCommandCopyAndPaste:
	case kCommandFixCharacterTranslation:
	case kCommandSpeakSelectedText:
		if (isTEK)
		{
			result = false; // text selection commands do not apply to graphics
			if (inCommandID == kCommandCopy)
			{
				// can copy graphics as long as data is available
				result = true;
			}
		}
		else
		{
			if ((inCommandID == kCommandCopy) && (isDialog))
			{
				// modeless dialogs and sheets can Copy
				result = true;
			}
			else
			{
				result = isTerminal;
				if (nullptr != currentTerminalView) result = (result && TerminalView_TextSelectionExists(currentTerminalView));
				if (result)
				{
					if (inCommandID == kCommandHandleURL)
					{
					//	// INCOMPLETE - these checks should ideally be done
						if (isTerminal)
						{
							UniformResourceLocatorType	urlKind = kNotURL;
							Handle						selectedText = TerminalView_ReturnSelectedTextAsNewHandle
															(currentTerminalView, 
																0/* Copy Using Tabs For Spaces info */,
																kTerminalView_TextFlagInline);
							
							
							if (nullptr == selectedText) result = false;
							else
							{
								urlKind = URL_GetTypeFromDataHandle(selectedText);
								if (urlKind == kNotURL) result = false; // disable command for non-URL text selections
								Memory_DisposeHandle(&selectedText);
							}
						}
					//	if ((clipboard) && (clipboard is not text or clipboard text is not a URL)) result = false;
					}
				}
				if (inCommandID == kCommandCopyAndPaste)
				{
					result = (result && (!isReadOnly)); // can’t type in a read-only window!
				}
			}
		}
		break;
	
	case kCommandPaste:
		result = areSessionRelatedItemsEnabled() && (!isReadOnly);
		if (result)
		{
		#if TARGET_API_MAC_OS8
			long	offset = 0L;
			
			
			// there must be text on the clipboard in order for Paste to work
			result = (GetScrap(0L, kScrapFlavorTypeText, &offset) > 0L);
		#else
			ScrapRef			currentScrap = nullptr;
			ScrapFlavorFlags	currentScrapFlags = 0L;
			
			
			if (GetCurrentScrap(&currentScrap) == noErr)
			{
				result = (GetScrapFlavorFlags(currentScrap, kScrapFlavorTypeText, &currentScrapFlags) == noErr);
			}
			else result = false;
		#endif
		}
		break;
	
	case kCommandSelectAll:
	case kCommandSelectAllWithScrollback:
	case kCommandSelectNothing:
		if ((isTerminal) || (isDialog))
		{
			result = true;
		}
		else
		{
			// not applicable to window
			result = false;
		}
		break;
	
	default:
		// unknown command!
		break;
	}
	return result;
}// stateTrackerStandardEditItems


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines whether TEK commands should be enabled.

(3.0)
*/
static Boolean
stateTrackerTEKItems	(UInt32				inCommandID,
						 MenuRef			inMenu,
						 MenuItemIndex		inItemNumber)
{
	Boolean		result = false;
	
	
	switch (inCommandID)
	{
	case kCommandTEKPageCommand:
	case kCommandTEKPageClearsScreen:
		if (TerminalWindow_ExistsFor(EventLoop_GetRealFrontWindow()))
		{
			SessionRef		session = SessionFactory_ReturnUserFocusSession();
			
			
			if (nullptr != session)
			{
				result = areTEKRelatedItemsEnabled();
				if (result) result = Session_TEKIsEnabled(session);
				if (inCommandID == kCommandTEKPageClearsScreen)
				{
					CheckMenuItem(inMenu, inItemNumber,
									(result)
									? !Session_TEKPageCommandOpensNewWindow(session)
									: false);
				}
			}
		}
		break;
	
	default:
		// unknown command!
		break;
	}
	return result;
}// stateTrackerTEKItems


/*!
This routine, of standard MenuCommandStateTrackerProcPtr
form, determines the item icon and mark for connection
items in the Window menu.

Although the menu reference is given, this method assumes
you are referring to the Window menu and does not check
that the specified menu “really is” the Window menu.

Icons are currently not set using this routine; see
MenuBar_SetWindowMenuItemMarkForSession().

(3.0)
*/
static Boolean
stateTrackerWindowMenuWindowItems	(UInt32			UNUSED_ARGUMENT(inCommandID),
									 MenuRef		inMenu,
									 MenuItemIndex	inItemNumber)
{
	WindowRef	screenWindow = getWindowFromWindowMenuItemIndex(inItemNumber);
	Boolean		result = true;
	
	
	if (IsWindowHilited(screenWindow))
	{
		// check the active window in the menu
		CheckMenuItem(inMenu, inItemNumber, true);
	}
	else
	{
		SInt16		zeroBasedIndex = (inItemNumber - returnFirstWindowItemAnchor(inMenu));
		
		
		// remove any mark, initially
		CheckMenuItem(inMenu, inItemNumber, false);
		
		// the first 9 items are numbered, for the user’s convenience
		if (zeroBasedIndex < 10) SetItemMark(inMenu, inItemNumber, '1' + zeroBasedIndex);
		
		// use the Mac OS X convention of bullet-marking windows with unsaved changes
		if (IsWindowModified(screenWindow)) SetItemMark(inMenu, inItemNumber, kBulletCharCode);
		
		// use the Mac OS X convention of diamond-marking minimized windows
		if (IsWindowCollapsed(screenWindow)) SetItemMark(inMenu, inItemNumber, kDiamondCharCode);
	}
	
	return result;
}// stateTrackerWindowMenuWindowItems

// BELOW IS REQUIRED NEWLINE TO END FILE
