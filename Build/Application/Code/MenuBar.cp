/*###############################################################

	MenuBar.cp
	
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
#include "GeneralResources.h"
#include "MenuResources.h"

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
#include "MacroManager.h"
#include "MenuBar.h"
#include "Preferences.h"
#include "ProgressDialog.h"
#include "RasterGraphicsScreen.h"
#include "Session.h"
#include "SessionFactory.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "ThreadEntryPoints.h"
#include "UIStrings.h"
#include "URL.h"
#include "VectorCanvas.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

SInt16 const	kInsertMenuAfterAllOthers = 0;	// for use with InsertMenu (a constant Menus.h REALLY SHOULD HAVE)

} // anonymous namespace

#pragma mark Types
namespace {

struct MyMenuItemInsertionInfo
{
	MenuRef			menu;				//!< the menu in which to create the item
	MenuItemIndex	afterItemIndex;		//!< the existing item after which the new item should appear
};
typedef MyMenuItemInsertionInfo const*		MyMenuItemInsertionInfoConstPtr;

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_ListenerRef	gPreferenceChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionStateChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionCountChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionWindowStateChangeEventListener = nullptr;
MenuItemIndex				gNumberOfSessionMenuItemsAdded = 0;
MenuItemIndex				gNumberOfFormatMenuItemsAdded = 0;
MenuItemIndex				gNumberOfMacroSetMenuItemsAdded = 0;
MenuItemIndex				gNumberOfSpecialWindowMenuItemsAdded = 0;
MenuItemIndex				gNumberOfTranslationTableMenuItemsAdded = 0;
MenuItemIndex				gNumberOfWindowMenuItemsAdded = 0;
MenuItemIndex				gNumberOfSpecialScriptMenuItems = 0;
Boolean						gUsingTabs = false;
Boolean						gSimplifiedMenuBar = false;
Boolean						gFontMenusAvailable = false;
UInt32						gNewCommandShortcutEffect = kCommandNewSessionDefaultFavorite;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean				addWindowMenuItemForSession				(SessionRef, MyMenuItemInsertionInfoConstPtr, CFStringRef);
void				addWindowMenuItemSessionOp				(SessionRef, void*, SInt32, void*);
void				adjustMenuItem							(MenuRef, MenuItemIndex, UInt32);
void				adjustMenuItemByCommandID				(UInt32);
Boolean				areSessionRelatedItemsEnabled			();
void				buildMenuBar							();
void				executeScriptByMenuEvent				(MenuRef, MenuItemIndex);
MenuItemIndex		getMenuAndMenuItemIndexByCommandID		(UInt32, MenuRef*);
void				getMenuItemAdjustmentProc				(MenuRef, MenuItemIndex, MenuCommandStateTrackerProcPtr*);
void				getMenusAndMenuItemIndicesByCommandID	(UInt32, MenuRef*, MenuRef*, MenuItemIndex*,
																MenuItemIndex*);
void				installMenuItemStateTrackers			();
void				preferenceChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				removeMenuItemModifier					(MenuRef, MenuItemIndex);
void				removeMenuItemModifiers					(MenuRef);
MenuItemIndex		returnFirstWindowItemAnchor				(MenuRef);
SessionRef			returnMenuItemSession					(MenuRef, MenuItemIndex);
SessionRef			returnTEKSession						();
MenuItemIndex		returnWindowMenuItemIndexForSession		(SessionRef);
void				sessionCountChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				sessionStateChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void				sessionWindowStateChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
#if 0
void				setMenuEnabled							(MenuRef, Boolean);
#endif
void				setMenuItemEnabled						(MenuRef, UInt16, Boolean);
inline OSStatus		setMenuItemVisibility					(MenuRef, MenuItemIndex, Boolean);
void				setMenusHaveKeyEquivalents				(Boolean, Boolean);
void				setNewCommand							(UInt32);
void				setUpDynamicMenus						();
void				setUpFormatFavoritesMenu				(MenuRef);
void				setUpMacroSetsMenu						(MenuRef);
void				setUpNotificationsMenu					(MenuRef);
void				setUpScreenSizeFavoritesMenu			(MenuRef);
void				setUpSessionFavoritesMenu				(MenuRef);
void				setUpScriptsMenu						(MenuRef);
void				setUpTranslationTablesMenu				(MenuRef);
void				setUpWindowMenu							(MenuRef);
void				setWindowMenuItemMarkForSession			(SessionRef, MenuRef = nullptr, MenuItemIndex = 0);
void				simplifyMenuBar							(Boolean);
Boolean				stateTrackerCheckableItems				(UInt32, MenuRef, MenuItemIndex);
Boolean				stateTrackerGenericSessionItems			(UInt32, MenuRef, MenuItemIndex);
Boolean				stateTrackerNetSendItems				(UInt32, MenuRef, MenuItemIndex);
Boolean				stateTrackerPrintingItems				(UInt32, MenuRef, MenuItemIndex);
Boolean				stateTrackerShowHideItems				(UInt32, MenuRef, MenuItemIndex);
Boolean				stateTrackerStandardEditItems			(UInt32, MenuRef, MenuItemIndex);
Boolean				stateTrackerTEKItems					(UInt32, MenuRef, MenuItemIndex);
Boolean				stateTrackerWindowMenuWindowItems		(UInt32, MenuRef, MenuItemIndex);

} // anonymous namespace



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
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_TagArrangeWindowsUsingTabs,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_TagMenuItemKeys,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_TagNewCommandShortcutEffect,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_TagSimplifiedUserInterface,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_ChangeContextName,
							false/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
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
				UIStrings_Result	stringResult = kUIStrings_ResultOK;
				CFStringRef			itemNameCFString = nullptr;
				
				
				stringResult = UIStrings_Copy(kUIStrings_HelpSystemShowTagsCommandName, itemNameCFString);
				if (stringResult.ok())
				{
					(OSStatus)InsertMenuItemTextWithCFString(helpMenu, itemNameCFString,
																itemIndex++, 0/* attributes */,
																kCommandShowHelpTags);
					CFRelease(itemNameCFString), itemNameCFString = nullptr;
				}
				stringResult = UIStrings_Copy(kUIStrings_HelpSystemHideTagsCommandName, itemNameCFString);
				if (stringResult.ok())
				{
					(OSStatus)InsertMenuItemTextWithCFString(helpMenu, itemNameCFString,
															itemIndex++, 0/* attributes */,
															kCommandHideHelpTags);
					CFRelease(itemNameCFString), itemNameCFString = nullptr;
				}
			}
		}
	}
	
	// initialize macro selections so that the module will handle menu commands
	{
		MacroManager_Result		macrosResult = MacroManager_SetCurrentMacros(MacroManager_ReturnDefaultMacros());
		
		
		assert(macrosResult.ok());
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
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagArrangeWindowsUsingTabs);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagMenuItemKeys);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagNewCommandShortcutEffect);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagSimplifiedUserInterface);
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
			// TEMPORARY - LOCALIZE THIS
			kMacrosMenuTitleWidthApproximation = 66,
			kWindowMenuTitleWidthApproximation = 72, // pixels
			kWindowMenuTitleLeftEdgeNormalApproximation = 400, // pixel offset
			kWindowMenuTitleLeftEdgeSimpleModeApproximation = 272 // pixel offset
		};
		UInt16			macrosMenuWidth = kMacrosMenuTitleWidthApproximation;
		OSStatus		error = noErr;
		MenuAttributes	menuAttributes = 0;
		
		
		// the Macros menu is not always visible, so account for its location
		error = GetMenuAttributes(MenuBar_ReturnMacrosMenu(), &menuAttributes);
		if (noErr == error)
		{
			if (menuAttributes & kMenuAttrHidden)
			{
				macrosMenuWidth = 0;
			}
		}
		
		switch (inMenuBarMenuSpecifier)
		{
		case kMenuBar_MenuWindow:
			// approximate the rectangle of the Window menu, which is different in Simplified User
			// Interface mode (there’s gotta be a beautiful way to get this just right, but I’m
			// not finding it now)
			*outMenuBarMenuTitleRect = (**(GetMainDevice())).gdRect;
			outMenuBarMenuTitleRect->bottom = GetMBarHeight();
			outMenuBarMenuTitleRect->left += macrosMenuWidth;
			outMenuBarMenuTitleRect->left += (gSimplifiedMenuBar)
												? kWindowMenuTitleLeftEdgeSimpleModeApproximation
												: kWindowMenuTitleLeftEdgeNormalApproximation;
			outMenuBarMenuTitleRect->right = outMenuBarMenuTitleRect->left + kWindowMenuTitleWidthApproximation;
			
		#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
			{
				// since the values are based on an unscaled screen, multiply accordingly
				Float32		scaleFactor = HIGetScaleFactor();
				
				
				outMenuBarMenuTitleRect->left = STATIC_CAST(outMenuBarMenuTitleRect->left, Float32) * scaleFactor;
				outMenuBarMenuTitleRect->top = STATIC_CAST(outMenuBarMenuTitleRect->top, Float32) * scaleFactor;
				outMenuBarMenuTitleRect->right = STATIC_CAST(outMenuBarMenuTitleRect->right, Float32) * scaleFactor;
				outMenuBarMenuTitleRect->bottom = STATIC_CAST(outMenuBarMenuTitleRect->bottom, Float32) * scaleFactor;
			}
		#endif
			
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
							 MenuItemIndex		inMenuItemIndex)
{
	UInt32		commandID = 0L;
	UInt16		menuID = GetMenuID(inMenu);
	OSStatus	error = noErr;
	Boolean		result = true;
	
	
	error = GetMenuItemCommandID(inMenu, inMenuItemIndex, &commandID);
	
	switch (menuID)
	{
	case kMenuIDScripts:
		// execute script using name of menu item
		executeScriptByMenuEvent(inMenu, inMenuItemIndex);
		break;
	
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
MenuBar_IsMenuCommandEnabled	(UInt32		inCommandID)
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
}// IsMenuCommandEnabled


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
Returns a handle to the menu to use for macros.

(4.0)
*/
MenuRef
MenuBar_ReturnMacrosMenu ()
{
	return GetMenuRef(kMenuIDMacros);
}// ReturnMacrosMenu


/*!
Determines the item index of a menu item that has
the specified identical text.  If the item is not
in the menu, 0 is returned.

(4.0)
*/
MenuItemIndex
MenuBar_ReturnMenuItemIndexByItemText	(MenuRef		inMenu,
										 CFStringRef	inItemText)
{
	MenuItemIndex const		kItemCount = CountMenuItems(inMenu);
	CFStringRef				itemCFString = nullptr;
	MenuItemIndex			i = 0;
	MenuItemIndex			result = 0;
	
	
	// look for an identical item
	for (i = 1; i <= kItemCount; ++i)
	{
		OSStatus	error = CopyMenuItemTextAsCFString(inMenu, i, &itemCFString);
		
		
		if (noErr == error)
		{
			Boolean		stopNow = (kCFCompareEqualTo == CFStringCompare(itemCFString, inItemText,
																		0/* options */));
			
			
			CFRelease(itemCFString), itemCFString = nullptr;
			if (stopNow)
			{
				result = i;
				break;
			}
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
Associates a menu item with a function that reports the proper
state of the item at any given time.  A menu item property is
used to store the information.

See also MenuBar_SetMenuItemStateTrackerProcByCommandID().

(3.0)
*/
void
MenuBar_SetMenuItemStateTrackerProc		(MenuRef							inMenu,
										 MenuItemIndex						inItemIndex,
										 MenuCommandStateTrackerProcPtr		inProc)
{
	if ((nullptr != inMenu) && (inItemIndex > 0))
	{
		OSStatus	error = noErr;
		
		
		error = SetMenuItemProperty(inMenu, inItemIndex, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_MenuPropertyTypeStateTrackerProcPtr,
									sizeof(inProc), &inProc);
		assert_noerr(error);
	}
}// SetMenuItemStateTrackerProc


/*!
Associates a menu item with a function that reports the proper
state of the item at any given time.  A menu item property is
used to store the information.

See also MenuBar_SetMenuItemStateTrackerProc().

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
	if ((nullptr != menuHandle1) && (0 != itemIndex1))
	{
		MenuBar_SetMenuItemStateTrackerProc(menuHandle1, itemIndex1, inProc);
	}
	if ((nullptr != menuHandle2) && (0 != itemIndex2))
	{
		MenuBar_SetMenuItemStateTrackerProc(menuHandle2, itemIndex2, inProc);
	}
}// SetMenuItemStateTrackerProcByCommandID


/*!
This method ensures that everything about the specified
menu command (such as item text, enabled state, checked
states and menu enabled state) is correct for the context
of the program at the time this method is invoked.

(3.1)
*/
void
MenuBar_SetUpMenuItemState	(UInt32		inCommandID)
{
	adjustMenuItemByCommandID(inCommandID);
}// SetUpMenuItemState


#pragma mark Internal Methods
namespace {

/*!
Adds a session’s name to the Window menu, arranging so
that future selections of the new menu item will cause
the window to be selected, and so that the item’s state
is synchronized with that of the session and its window.

Returns true only if an item was added.

(3.0)
*/
Boolean
addWindowMenuItemForSession		(SessionRef							inSession,
								 MyMenuItemInsertionInfoConstPtr	inMenuInfoPtr,
								 CFStringRef						inWindowName)
{
	OSStatus	error = noErr;
	Boolean		result = false;
	
	
	error = InsertMenuItemTextWithCFString(inMenuInfoPtr->menu, inWindowName/* item name */,
											inMenuInfoPtr->afterItemIndex, kMenuItemAttrIgnoreMeta/* attributes */,
											kCommandSessionByWindowName/* command ID */);
	if (noErr == error)
	{
		// set icon appropriately for the state
		setWindowMenuItemMarkForSession(inSession, inMenuInfoPtr->menu, inMenuInfoPtr->afterItemIndex + 1);
		
		// attach meta-data to the new menu command; this allows it to have its
		// checkmark set automatically, etc.
		MenuBar_SetMenuItemStateTrackerProc(inMenuInfoPtr->menu, inMenuInfoPtr->afterItemIndex + 1,
											stateTrackerWindowMenuWindowItems);
		
		// attach the given session as a property of the new item
		error = SetMenuItemProperty(inMenuInfoPtr->menu, inMenuInfoPtr->afterItemIndex + 1,
									AppResources_ReturnCreatorCode(), kConstantsRegistry_MenuItemPropertyTypeSessionRef,
									sizeof(inSession), &inSession);
		assert_noerr(error);
		
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
void
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
		nameCFString = Session_ReturnResourceLocationCFString(inSession);
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
void
adjustMenuItem	(MenuRef		inMenu,
				 MenuItemIndex	inItemNumber,
				 UInt32			inOverridingCommandIDOrZero)
{
	MenuCommand						commandID = inOverridingCommandIDOrZero;
	OSStatus						error = noErr;
	MenuCommandStateTrackerProcPtr	proc = nullptr;
	
	
	if (commandID == 0) error = GetMenuItemCommandID(inMenu, inItemNumber, &commandID);
	if (error != noErr) commandID = 0;
	
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
void
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
Many menu items are all jointly enabled or disabled
based on the same basic factor - whether any session
window is frontmost.  Use this method to determine if
session-related menu items should be enabled.

(3.0)
*/
Boolean
areSessionRelatedItemsEnabled ()
{
	Boolean		result = false;
#if 0
	WindowRef	frontWindow = EventLoop_ReturnRealFrontWindow();
	
	
	if (nullptr != frontWindow)
	{
		short		windowKind = GetWindowKind(EventLoop_ReturnRealFrontWindow());
		
		
		result = ((windowKind == WIN_CNXN) || (windowKind == WIN_SHELL));
	}
#else
	result = (nullptr != SessionFactory_ReturnUserFocusSession());
#endif
	return result;
}// areSessionRelatedItemsEnabled
	
	
/*!
This method adds menus to the menu bar, and identifies
which menus are submenus of menu items.

(3.0)
*/
void
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
	
	// the AppleDockMenu Info.plist entry does not seem to
	// work anymore, so set the menu manually
	{
		NIBLoader	menuFile(AppResources_ReturnBundleForNIBs(), CFSTR("MenuForDockIcon"));
		assert(menuFile.isLoaded());
		MenuRef		dockMenu = nullptr;
		OSStatus	error = noErr;
		
		
		error = CreateMenuFromNib(menuFile.returnNIB(), CFSTR("DockMenu"), &dockMenu);
		assert_noerr(error);
		error = SetApplicationDockTileMenu(dockMenu);
		//assert_noerr(error); // this is not critical enough to care about if it fails
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
															AppResources_ReturnCreatorCode(),
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
void
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
MenuItemIndex
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
Retrieves the command state tracking routine stored in a
property of the specified menu item, or nullptr if there is
none.

(3.0)
*/
void
getMenuItemAdjustmentProc	(MenuRef							inMenu,
							 MenuItemIndex						inItemIndex,
							 MenuCommandStateTrackerProcPtr*	outProcPtrPtr)
{
	OSStatus	error = noErr;
	UInt32		actualSize = 0;
	
	
	error = GetMenuItemProperty(inMenu, inItemIndex, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_MenuPropertyTypeStateTrackerProcPtr,
								sizeof(*outProcPtrPtr), &actualSize, outProcPtrPtr);
	if (noErr != error) *outProcPtrPtr = nullptr;
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
void
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
void
installMenuItemStateTrackers ()
{
	// Application
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandAboutThisApplication, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFullScreenModal, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandKioskModeDisable, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowNetworkNumbers, nullptr/* no state tracker necessary */);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendInternetProtocolNumber, stateTrackerNetSendItems);
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
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNewDuplicateSession, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHandleURL, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSaveText, stateTrackerStandardEditItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandCaptureToFile, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandEndCaptureToFile, stateTrackerGenericSessionItems);
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
	
	// View
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSmallScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTallScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandLargeScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSetScreenSize, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandBiggerText, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFullScreen, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSmallerText, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFormatDefault, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandFormat, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTEKPageCommand, stateTrackerTEKItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTEKPageClearsScreen, stateTrackerTEKItems);
	
	// Terminal
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSuspendNetwork, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendInterruptProcess, stateTrackerNetSendItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandBellEnabled, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandEcho, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandWrapMode, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandClearScreenSavesLines, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandJumpScrolling, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTerminalEmulatorSetup, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandWatchNothing, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandWatchForActivity, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandWatchForInactivity, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTransmitOnInactivity, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSpeechEnabled, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandClearEntireScrollback, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandResetGraphicsCharacters, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandResetTerminal, stateTrackerGenericSessionItems);
	
	// Map
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandDeletePressSendsBackspace, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandDeletePressSendsDelete, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandEMACSArrowMapping, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandLocalPageUpDown, stateTrackerCheckableItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSetKeys, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandMacroSetNone, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandMacroSetDefault, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTranslationTableDefault, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSetTranslationTable, stateTrackerGenericSessionItems);
	
	// Macros
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro1, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro2, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro3, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro4, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro5, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro6, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro7, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro8, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro9, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro10, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro11, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandSendMacro12, stateTrackerGenericSessionItems);
	
	// Window
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandMinimizeWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandZoomWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandMaximizeWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandTerminalNewWorkspace, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandChangeWindowTitle, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideFrontWindow, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideOtherWindows, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowAllHiddenWindows, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandStackWindows, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNextWindow, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandNextWindowHideCurrent, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandPreviousWindow, stateTrackerGenericSessionItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowConnectionStatus, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandHideConnectionStatus, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowCommandLine, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowKeypad, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowFunction, stateTrackerShowHideItems);
	MenuBar_SetMenuItemStateTrackerProcByCommandID(kCommandShowControlKeys, stateTrackerShowHideItems);
	// state trackers for connection items are installed dynamically
}// installMenuItemStateTrackers


/*!
Invoked whenever a monitored preference value is changed
(see MenuBar_Init() to see which preferences are monitored).
This routine responds by ensuring that menu states are
up to date for the changed preference.

(3.0)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					inEventContextPtr,
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Preferences_ChangeContext*		contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	size_t							actualSize = 0L;
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagArrangeWindowsUsingTabs:
		unless (Preferences_GetData(kPreferences_TagArrangeWindowsUsingTabs,
									sizeof(gUsingTabs), &gUsingTabs,
									&actualSize) == kPreferences_ResultOK)
		{
			gUsingTabs = false; // assume tabs are not being used, if preference can’t be found
		}
		break;
	
	case kPreferences_TagMenuItemKeys:
		{
			Boolean		flag = false;
			
			
			unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(flag), &flag, &actualSize) ==
					kPreferences_ResultOK)
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
									&actualSize) == kPreferences_ResultOK)
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
									&actualSize) == kPreferences_ResultOK)
		{
			gSimplifiedMenuBar = false; // assume normal mode, if preference can’t be found
		}
		simplifyMenuBar(gSimplifiedMenuBar);
		break;
	
	case kPreferences_ChangeNumberOfContexts:
	case kPreferences_ChangeContextName:
		// regenerate menu items
		// TEMPORARY: maybe this should be more granulated than it is
		setUpDynamicMenus();
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
inline void
removeMenuItemModifier	(MenuRef		inMenu,
						 MenuItemIndex	inItemIndex)
{
	if (nullptr != inMenu)
	{
		(OSStatus)SetMenuItemCommandKey(inMenu, inItemIndex, false/* virtual key */, '\0'/* character */);
		(OSStatus)SetMenuItemModifiers(inMenu, inItemIndex, kMenuNoCommandModifier);
		(OSStatus)SetMenuItemKeyGlyph(inMenu, inItemIndex, kMenuNullGlyph);
	}
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
void
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
MenuItemIndex
returnFirstWindowItemAnchor		(MenuRef	inWindowMenu)
{
	MenuItemIndex	result = 0;
	MenuRef			menuContainingMatch = nullptr;
	OSStatus		error = GetIndMenuItemWithCommandID
							(inWindowMenu, kCommandShowKeypad/* this command should be last in the menu */,
								1/* which match to return */, &menuContainingMatch, &result);
	
	
	assert_noerr(error);
	assert(menuContainingMatch == inWindowMenu);
	
	// skip the dividing line item, then point to the next item (so +2)
	result += 2;
	
	return result;
}// returnFirstWindowItemAnchor


/*!
If the specified menu item has an associated SessionRef,
it is returned; otherwise, nullptr is returned.  This
should work for the session items in the Window menu.

(4.0)
*/
SessionRef
returnMenuItemSession	(MenuRef		inMenu,
						 MenuItemIndex	inIndex)
{
	SessionRef		result = nullptr;
	UInt32			actualSize = 0;
	OSStatus		error = noErr;
	
	
	// find the session corresponding to the selected menu item
	error = GetMenuItemProperty(inMenu, inIndex, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_MenuItemPropertyTypeSessionRef,
								sizeof(result), &actualSize, &result);
	if (noErr != error)
	{
		result = nullptr;
	}
	return result;
}// returnMenuItemSession


/*!
Returns the session currently applicable to TEK commands;
defined if the focus window is either a session terminal,
or a vector graphic that came from a session.

(3.1)
*/
SessionRef
returnTEKSession ()
{
	SessionRef		result = SessionFactory_ReturnUserFocusSession();
	
	
	if (nullptr == result)
	{
		// if the active session is not a terminal, it may be a graphic
		// which can be used to trace to its session
		HIWindowRef		frontWindow = EventLoop_ReturnRealFrontWindow();
		
		
		if (nullptr != frontWindow)
		{
			VectorCanvas_Ref	canvasForWindow = VectorCanvas_ReturnFromWindow(frontWindow);
			
			
			if (nullptr != canvasForWindow) result = VectorCanvas_ReturnListeningSession(canvasForWindow);
		}
	}
	return result;
}// returnTEKSession


/*!
Returns the index of the item in the Window menu
corresponding to the specified session’s window.

(4.0)
*/
MenuItemIndex
returnWindowMenuItemIndexForSession		(SessionRef		inSession)
{
	MenuRef const			kMenu = GetMenuRef(kMenuIDWindow);
	MenuItemIndex const		kStartItem = returnFirstWindowItemAnchor(kMenu);
	MenuItemIndex const		kPastEndItem = kStartItem + SessionFactory_ReturnCount();
	OSStatus				error = noErr;
	MenuItemIndex			result = 0;
	
	
	for (MenuItemIndex i = kStartItem; i != kPastEndItem; ++i)
	{
		SessionRef		itemSession = nullptr;
		UInt32			actualSize = 0;
		
		
		// find the session corresponding to the selected menu item
		error = GetMenuItemProperty(kMenu, i, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_MenuItemPropertyTypeSessionRef,
									sizeof(itemSession), &actualSize, &itemSession);
		if (noErr == error)
		{
			if (itemSession == inSession)
			{
				result = i;
			}
		}
	}
	return result;
}// returnWindowMenuItemIndexForSession


/*!
Invoked whenever the Session Factory changes (see
MenuBar_Init() to see which states are monitored).
This routine responds by ensuring that the Window
menu is up to date.

(3.0)
*/
void
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
void
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
			
			
			switch (Session_ReturnState(session))
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
void
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
			
			
			if (Session_GetWindowUserDefinedTitle(session, text) == kSession_ResultOK)
			{
				MenuItemIndex	itemIndex = returnWindowMenuItemIndexForSession(session);
				
				
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
void
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
inline void
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
inline OSStatus
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
void
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
			// note...do NOT remove modifiers from the Macros menu, even if it
			// is visible, as macros now rely on the menu key matching system
			// to operate!!!
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
void
setNewCommand	(UInt32		inCommandNShortcutCommand)
{
	size_t		actualSize = 0;
	Boolean		menuCommandKeys = false;
	
	
	// determine if menus are supposed to display key equivalents at all...
	unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(menuCommandKeys), &menuCommandKeys,
			&actualSize) == kPreferences_ResultOK)
	{
		menuCommandKeys = true; // assume menu items have key equivalents, if preference can’t be found
	}
	
	if (menuCommandKeys)
	{
		MenuRef				defaultMenu = nullptr;
		MenuRef				shellMenu = nullptr;
		MenuRef				logInShellMenu = nullptr;
		MenuRef				dialogMenu = nullptr;
		MenuItemIndex		shellItemIndex = 0;
		MenuItemIndex		logInShellItemIndex = 0;
		MenuItemIndex		dialogItemIndex = 0;
		MenuItemIndex		defaultItemIndex = 0;
		UInt16				keyCharacter = 'N'; // this default is ignored as long as the string value is found below
		
		
		// determine the character that should be used for all key equivalents
		{
			CFStringRef			charCFString = nullptr;
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			
			
			stringResult = UIStrings_Copy(kUIStrings_TerminalNewCommandsKeyCharacter, charCFString);
			if (stringResult.ok())
			{
				keyCharacter = CFStringGetCharacterAtIndex(charCFString, 0);
				CFRelease(charCFString), charCFString = nullptr;
			}
			else
			{
				assert(false && "unable to find key equivalent for New commands");
			}
		}
		
		// IMPORTANT: For, er, simplicity, since there is no longer a simplified version
		// of the File menu, secondary menus are ignored here and key equivalents are
		// just flipped in the single File menu.  If a simplified File menu is ever
		// added back, this code has to check and flip the key equivalents in the
		// secondary menu, too.
		getMenusAndMenuItemIndicesByCommandID(kCommandNewSessionDefaultFavorite, &defaultMenu,
												nullptr/* simple menu */,
												&defaultItemIndex, nullptr/* simple item index */);
		getMenusAndMenuItemIndicesByCommandID(kCommandNewSessionShell, &shellMenu,
												nullptr/* simple menu */,
												&shellItemIndex, nullptr/* simple item index */);
		getMenusAndMenuItemIndicesByCommandID(kCommandNewSessionLoginShell, &logInShellMenu,
												nullptr/* simple menu */,
												&logInShellItemIndex, nullptr/* simple item index */);
		getMenusAndMenuItemIndicesByCommandID(kCommandNewSessionDialog, &dialogMenu,
												nullptr/* simple menu */,
												&dialogItemIndex, nullptr/* simple item index */);
		
		// first clear the non-command-key modifiers of certain “New” commands
		removeMenuItemModifier(defaultMenu, defaultItemIndex);
		removeMenuItemModifier(shellMenu, shellItemIndex);
		removeMenuItemModifier(logInShellMenu, logInShellItemIndex);
		removeMenuItemModifier(dialogMenu, dialogItemIndex);
		
		// Modifiers are assigned appropriately based on the given command.
		// If a menu item is assigned to command-N, then Default is given
		// whatever key equivalent the item would have otherwise had.  The
		// NIB is always overridden, although the keys in the NIB act like
		// comments for the key equivalents used in the code below.
		switch (inCommandNShortcutCommand)
		{
		case kCommandNewSessionDefaultFavorite:
			// default case: everything here should match NIB assignments
			{
				MenuRef			menu = nullptr;
				MenuItemIndex	itemIndex = 0;
				
				
				menu = defaultMenu;
				itemIndex = defaultItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				
				menu = shellMenu;
				itemIndex = shellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuControlModifier);
				
				menu = logInShellMenu;
				itemIndex = logInShellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuOptionModifier);
				
				menu = dialogMenu;
				itemIndex = dialogItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
			}
			break;
		
		case kCommandNewSessionShell:
			// swap Default and Shell key equivalents
			{
				MenuRef			menu = nullptr;
				MenuItemIndex	itemIndex = 0;
				
				
				menu = defaultMenu;
				itemIndex = defaultItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuControlModifier);
				
				menu = shellMenu;
				itemIndex = shellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				
				menu = logInShellMenu;
				itemIndex = logInShellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuOptionModifier);
				
				menu = dialogMenu;
				itemIndex = dialogItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
			}
			break;
		
		case kCommandNewSessionLoginShell:
			// swap Default and Log-In Shell key equivalents
			{
				MenuRef			menu = nullptr;
				MenuItemIndex	itemIndex = 0;
				
				
				menu = defaultMenu;
				itemIndex = defaultItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuOptionModifier);
				
				menu = shellMenu;
				itemIndex = shellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuControlModifier);
				
				menu = logInShellMenu;
				itemIndex = logInShellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
				
				menu = dialogMenu;
				itemIndex = dialogItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
			}
			break;
		
		case kCommandNewSessionDialog:
			// swap Default and Custom New Session key equivalents
			{
				MenuRef			menu = nullptr;
				MenuItemIndex	itemIndex = 0;
				
				
				menu = defaultMenu;
				itemIndex = defaultItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuShiftModifier);
				
				menu = shellMenu;
				itemIndex = shellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuControlModifier);
				
				menu = logInShellMenu;
				itemIndex = logInShellItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, kMenuOptionModifier);
				
				menu = dialogMenu;
				itemIndex = dialogItemIndex;
				SetMenuItemCommandKey(menu, itemIndex, false/* virtual key */, keyCharacter);
				SetMenuItemModifiers(menu, itemIndex, 0); // add command modifier
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
void
setUpDynamicMenus ()
{
	setUpSessionFavoritesMenu(GetMenuRef(kMenuIDFile));
	setUpScreenSizeFavoritesMenu(GetMenuRef(kMenuIDView));
	setUpFormatFavoritesMenu(GetMenuRef(kMenuIDView));
	setUpMacroSetsMenu(GetMenuRef(kMenuIDKeys));
	setUpNotificationsMenu(GetMenuRef(kMenuIDTerminal));
	setUpTranslationTablesMenu(GetMenuRef(kMenuIDKeys));
	setUpWindowMenu(GetMenuRef(kMenuIDWindow));
	setUpScriptsMenu(GetMenuRef(kMenuIDScripts));
}// setUpDynamicMenus


/*!
Destroys and rebuilds the menu items that automatically change
a terminal format.

(3.1)
*/
void
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
		(Preferences_Result)Preferences_InsertContextNamesInMenu(kPreferences_ClassFormat, inMenu,
																	defaultIndex, 1/* indentation level */,
																	kCommandFormatByFavoriteName,
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
		
		// ensure these items are inactive except for terminal windows
		for (UInt32 i = 1; i <= gNumberOfFormatMenuItemsAdded; ++i)
		{
			MenuItemIndex	itemIndex = 0;
			
			
			error = GetIndMenuItemWithCommandID(inMenu/* starting point */, kCommandFormatByFavoriteName,
												i/* which matching item to return */,
												nullptr/* matching menu */, &itemIndex);
			if (noErr == error)
			{
				MenuBar_SetMenuItemStateTrackerProc(inMenu, itemIndex, stateTrackerGenericSessionItems);
			}
		}
	}
}// setUpFormatFavoritesMenu


/*!
Destroys and rebuilds the menu items that automatically change
the active macro set.

(3.1)
*/
void
setUpMacroSetsMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	noneIndex = 0;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandMacroSetNone, 1/* which match to return */,
										&inMenu, &noneIndex);
	if (noErr == error)
	{
		// find the key item to use as an anchor point
		error = GetIndMenuItemWithCommandID(inMenu, kCommandMacroSetDefault, 1/* which match to return */,
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
			(Preferences_Result)Preferences_InsertContextNamesInMenu(kPreferences_ClassMacroSet, inMenu,
																		defaultIndex, 1/* indentation level */,
																		kCommandMacroSetByFavoriteName,
																		gNumberOfMacroSetMenuItemsAdded);
			
			// ensure these items are inactive except for terminal windows
			for (UInt32 i = 1; i <= gNumberOfMacroSetMenuItemsAdded; ++i)
			{
				MenuItemIndex	itemIndex = 0;
				
				
				error = GetIndMenuItemWithCommandID(inMenu/* starting point */, kCommandMacroSetByFavoriteName,
													i/* which matching item to return */,
													nullptr/* matching menu */, &itemIndex);
				if (noErr == error)
				{
					MenuBar_SetMenuItemStateTrackerProc(inMenu, itemIndex, stateTrackerGenericSessionItems);
				}
			}
		}
		
		// also fix the indentation of the None and Default choices, as
		// these cannot be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, noneIndex, 1/* number of indents */);
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
	}
}// setUpMacroSetsMenu


/*!
Destroys and rebuilds the menu items that automatically change
the current session notification (if any).

(3.1)
*/
void
setUpNotificationsMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandWatchNothing, 1/* which match to return */,
										&inMenu, &defaultIndex);
	if (noErr == error)
	{
		// fix the indentation of the notification choices, as this
		// cannot be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		error = GetIndMenuItemWithCommandID(inMenu, kCommandWatchForActivity, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
			error = GetIndMenuItemWithCommandID(inMenu, kCommandWatchForInactivity, 1/* which match to return */,
												&inMenu, &defaultIndex);
			if (noErr == error)
			{
				(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
				error = GetIndMenuItemWithCommandID(inMenu, kCommandTransmitOnInactivity, 1/* which match to return */,
													&inMenu, &defaultIndex);
				if (noErr == error)
				{
					(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
				}
			}
		}
	}
}// setUpNotificationsMenu


/*!
Destroys and rebuilds the menu items that automatically open
sessions with particular Session Favorite names.

(3.1)
*/
void
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
void
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
			date = FileUtilities_ReturnDirectoryDateFromFSSpec(&scriptsFolder, kFileUtilitiesDateOfModification);
			
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
				ProgressDialog_Ref	progressDialog = nullptr;
			#endif
				SInt16				numberOfSpecialScriptMenuItems = 0;
				SInt16				oldResFile = CurResFile();
				
				
			#if SCRIPTS_MENU_PROGRESS_WINDOW
				{
					UIStrings_Result	stringResult = kUIStrings_ResultOK;
					CFStringRef			dialogCFString = nullptr;
					
					
					// create a progress dialog with a status string
					stringResult = UIStrings_Copy(kUIStrings_ProgressWindowScriptsMenuPrimaryText, dialogCFString);
					assert(stringResult.ok());
					progressDialog = ProgressDialog_New(dialogCFString, false/* modal */);
					
					// specify a title for the Dock’s menu
					stringResult = UIStrings_Copy(kUIStrings_ProgressWindowScriptsMenuIconName, dialogCFString);
					if (stringResult.ok())
					{
						SetWindowAlternateTitle(ProgressDialog_ReturnWindow(progressDialog), dialogCFString);
						CFRelease(dialogCFString);
					}
				}
				ProgressDialog_SetProgressIndicator(progressDialog, kProgressDialog_IndicatorIndeterminate);
				
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
void
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
		(Preferences_Result)Preferences_InsertContextNamesInMenu(kPreferences_ClassSession, inMenu,
																	defaultIndex, 1/* indentation level */,
																	kCommandNewSessionByFavoriteName,
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
void
setUpTranslationTablesMenu	(MenuRef	inMenu)
{
	OSStatus		error = noErr;
	MenuItemIndex	defaultIndex = 0;
	
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(inMenu, kCommandTranslationTableDefault, 1/* which match to return */,
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
		(Preferences_Result)Preferences_InsertContextNamesInMenu(kPreferences_ClassTranslation, inMenu,
																	defaultIndex, 1/* indentation level */,
																	kCommandTranslationTableByFavoriteName,
																	gNumberOfTranslationTableMenuItemsAdded);
		
		// also fix the indentation of the Default choice, as this
		// cannot be set in the NIB and will be wrong (again) if
		// the indicated menu has been reloaded recently
		(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		
		// also indent the customization item
		error = GetIndMenuItemWithCommandID(inMenu, kCommandSetTranslationTable, 1/* which match to return */,
											&inMenu, &defaultIndex);
		if (noErr == error)
		{
			(OSStatus)SetMenuItemIndent(inMenu, defaultIndex, 1/* number of indents */);
		}
		
		// ensure these items are inactive except for terminal windows
		for (UInt32 i = 1; i <= gNumberOfTranslationTableMenuItemsAdded; ++i)
		{
			MenuItemIndex	itemIndex = 0;
			
			
			error = GetIndMenuItemWithCommandID(inMenu/* starting point */, kCommandTranslationTableByFavoriteName,
												i/* which matching item to return */,
												nullptr/* matching menu */, &itemIndex);
			if (noErr == error)
			{
				MenuBar_SetMenuItemStateTrackerProc(inMenu, itemIndex, stateTrackerGenericSessionItems);
			}
		}
	}
}// setUpTranslationTablesMenu


/*!
Destroys and rebuilds the menu items representing open session
windows and their states.

(3.1)
*/
void
setUpWindowMenu		(MenuRef	inMenu)
{
	MenuItemIndex const			kFirstWindowItemIndex = returnFirstWindowItemAnchor(inMenu);
	MenuItemIndex const			kDividerIndex = kFirstWindowItemIndex - 1;
	MenuItemIndex const			kPreDividerIndex = kDividerIndex - 1;
	MyMenuItemInsertionInfo		insertWhere;
	
	
	// set up callback data
	bzero(&insertWhere, sizeof(insertWhere));
	insertWhere.menu = inMenu;
	insertWhere.afterItemIndex = kPreDividerIndex; // because, the divider is not present at insertion time
	
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
void
setWindowMenuItemMarkForSession		(SessionRef		inSession,
									 MenuRef		inWindowMenuOrNull,
									 MenuItemIndex	inItemIndexOrZero)
{
	assert(nullptr != inSession);
	Session_Result			sessionResult = kSession_ResultOK;
	IconRef					stateIconRef = nullptr;
	MenuRef					menu = (nullptr == inWindowMenuOrNull)
									? GetMenuRef(kMenuIDWindow)
									: inWindowMenuOrNull;
	MenuItemIndex			itemIndex = (0 == inItemIndexOrZero)
										? returnWindowMenuItemIndexForSession(inSession)
										: inItemIndexOrZero;
	
	
	sessionResult = Session_CopyStateIconRef(inSession, stateIconRef);
	if (false == sessionResult.ok())
	{
		Console_WriteLine("warning, unable to copy session icon for menu item");
	}
	else
	{
		OSStatus	error = noErr;
		
		
		// ownership of the handle and icon is transferred to the Menu Manager here
		error = SetMenuItemIconHandle(menu, itemIndex, kMenuIconRefType,
										REINTERPRET_CAST(stateIconRef, Handle)/* yep, this is what you have to do...sigh */);
	}
	
	// now, set the disabled/enabled state
	{
		TerminalWindowRef	terminalWindow = Session_ReturnActiveTerminalWindow(inSession);
		
		
		if (TerminalWindow_IsObscured(terminalWindow))
		{
			// set the style of the item to italic, which makes it more obvious that a window is hidden
			SetItemStyle(menu, itemIndex, italic);
			
			// disabling the icon itself is only possible with Mac OS 8.5 or later
			DisableMenuItemIcon(menu, itemIndex);
		}
		else
		{
			// set the style of the item to normal, undoing the style setting of "kMy_WindowMenuMarkTypeDisableIcon"
			SetItemStyle(menu, itemIndex, normal);
			
			// enabling the icon itself is only possible with Mac OS 8.5 or later
			EnableMenuItemIcon(menu, itemIndex);
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
void
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
Boolean
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
		WindowRef		frontWindow = EventLoop_ReturnRealFrontWindow();
		
		
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
	
	case kCommandBellEnabled:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Terminal_BellIsEnabled(currentScreen) : false;
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
	
	case kCommandWatchNothing:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Session_WatchIsOff(currentSession) : false;
		break;
	
	case kCommandWatchForActivity:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Session_WatchIsForPassiveData(currentSession) : false;
		break;
	
	case kCommandWatchForInactivity:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Session_WatchIsForInactivity(currentSession) : false;
		break;
	
	case kCommandTransmitOnInactivity:
		result = connectionCommandResult;
		if (nullptr != currentSession) checked = (result) ? Session_WatchIsForKeepAlive(currentSession) : false;
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
Boolean
stateTrackerGenericSessionItems		(UInt32				inCommandID,
									 MenuRef			inMenu,
									 MenuItemIndex		inItemNumber)
{
	WindowRef	frontWindow = EventLoop_ReturnRealFrontWindow();
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
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(EventLoop_ReturnRealFrontWindow());
			
			
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
	case kCommandFormatByFavoriteName:
	case kCommandFullScreenModal:
	case kCommandTerminalEmulatorSetup:
	case kCommandJumpScrolling:
	case kCommandSetKeys:
	case kCommandMacroSetNone:
	case kCommandMacroSetDefault:
	case kCommandMacroSetByFavoriteName:
	case kCommandTranslationTableDefault:
	case kCommandTranslationTableByFavoriteName:
	case kCommandSetTranslationTable:
	case kCommandClearEntireScrollback:
	case kCommandResetGraphicsCharacters:
	case kCommandResetTerminal:
	case kCommandSendMacro1:
	case kCommandSendMacro2:
	case kCommandSendMacro3:
	case kCommandSendMacro4:
	case kCommandSendMacro5:
	case kCommandSendMacro6:
	case kCommandSendMacro7:
	case kCommandSendMacro8:
	case kCommandSendMacro9:
	case kCommandSendMacro10:
	case kCommandSendMacro11:
	case kCommandSendMacro12:
	case kCommandChangeWindowTitle:
	case kCommandStackWindows:
	case kCommandNextWindow:
	case kCommandNextWindowHideCurrent:
	case kCommandPreviousWindow:
	case kCommandPreviousWindowHideCurrent:
	case kCommandTerminalNewWorkspace:
		result = areSessionRelatedItemsEnabled();
		if (inCommandID == kCommandFormat)
		{
			// this item isn’t available if the “rebuilding font list” thread is still running
			result = (result && gFontMenusAvailable);
		}
		else if (inCommandID == kCommandSetScreenSize)
		{
			result = TerminalWindow_ExistsFor(frontWindow);
		}
		else if (inCommandID == kCommandChangeWindowTitle)
		{
			result = TerminalWindow_ExistsFor(frontWindow) || (nullptr != returnTEKSession());
		}
		else if ((inCommandID == kCommandNextWindow) ||
				(inCommandID == kCommandNextWindowHideCurrent) ||
				(inCommandID == kCommandPreviousWindow) ||
				(inCommandID == kCommandPreviousWindowHideCurrent))
		{
			result = (result && (nullptr != GetNextWindow(EventLoop_ReturnRealFrontWindow())));
		}
		else if (inCommandID == kCommandTerminalNewWorkspace)
		{
			result = (result && gUsingTabs);
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
Boolean
stateTrackerNetSendItems	(UInt32				inCommandID,
							 MenuRef			UNUSED_ARGUMENT(inMenu),
							 MenuItemIndex		UNUSED_ARGUMENT(inItemNumber))
{
	Boolean			result = false;
	
	
	switch (inCommandID)
	{
	case kCommandSendInternetProtocolNumber:
	case kCommandSendInterruptProcess:
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
Boolean
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
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(EventLoop_ReturnRealFrontWindow());
			
			
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
				result = (WIN_TEK == GetWindowKind(EventLoop_ReturnRealFrontWindow()));
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
Boolean
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
			HIWindowRef		window = EventLoop_ReturnRealFrontWindow();
			
			
			if (TerminalWindow_ExistsFor(window))
			{
				result = true;
				if ((kCommandFindAgain == inCommandID) || (kCommandFindPrevious == inCommandID))
				{
					result = TerminalView_SearchResultsExist
								(TerminalWindow_ReturnViewWithFocus(TerminalWindow_ReturnFromWindow(window)));
				}
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
			WindowRef	frontWindow = EventLoop_ReturnRealFrontWindow();
			
			
			result = (nullptr != frontWindow); // there must be at least one window
		}
		break;
	
	case kCommandZoomWindow:
	case kCommandMaximizeWindow:
		{
			WindowRef	frontWindow = EventLoop_ReturnRealFrontWindow();
			
			
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
					(SessionFactory_ReturnCount() > 1)); // there must be multiple connection windows open!
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
	
	case kCommandShowCommandLine:
		// in the Cocoa implementation this really means “show or activate”, so it is always available
		result = true;
		break;
	
	case kCommandShowControlKeys:
		// in the Cocoa implementation this really means “show or activate”, so it is always available
		result = true;
		break;
	
	case kCommandShowFunction:
		// in the Cocoa implementation this really means “show or activate”, so it is always available
		result = true;
		break;
	
	case kCommandShowKeypad:
		// in the Cocoa implementation this really means “show or activate”, so it is always available
		result = true;
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
Boolean
stateTrackerStandardEditItems	(UInt32			inCommandID,
								 MenuRef		inMenu,
								 MenuItemIndex	inItemNumber)
{
	TerminalViewRef		currentTerminalView = nullptr;
	WindowRef			frontWindow = EventLoop_ReturnRealFrontWindow();
	Boolean				isDialog = false;
	Boolean				isReadOnly = false;
	Boolean				isTerminal = false;
	Boolean				isTEK = false;
	Boolean				result = false;
	
	
	// determine any window properties that might affect Edit items
	if (nullptr != frontWindow)
	{
		isDialog = (GetWindowKind(frontWindow) == kDialogWindowKind);
		isReadOnly = (GetWindowKind(frontWindow) == WIN_CONSOLE); // is the frontmost window read-only?
		isTerminal = TerminalWindow_ExistsFor(frontWindow); // terminal windows include the FTP log, too
		isTEK = (WIN_TEK == GetWindowKind(frontWindow));
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
								urlKind = URL_ReturnTypeFromDataHandle(selectedText);
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
		result = areSessionRelatedItemsEnabled() && (!isReadOnly) && Clipboard_ContainsText();
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
Boolean
stateTrackerTEKItems	(UInt32				inCommandID,
						 MenuRef			inMenu,
						 MenuItemIndex		inItemNumber)
{
	Boolean		result = false;
	
	
	switch (inCommandID)
	{
	case kCommandTEKPageCommand:
	case kCommandTEKPageClearsScreen:
		{
			SessionRef		session = returnTEKSession();
			
			
			if (nullptr != session)
			{
				result = true;
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
Boolean
stateTrackerWindowMenuWindowItems	(UInt32			UNUSED_ARGUMENT(inCommandID),
									 MenuRef		inMenu,
									 MenuItemIndex	inItemNumber)
{
	SessionRef	itemSession = returnMenuItemSession(inMenu, inItemNumber);
	Boolean		result = true;
	
	
	if (nullptr != itemSession)
	{
		HIWindowRef const	kSessionActiveWindow = Session_ReturnActiveWindow(itemSession);
		
		
		if (IsWindowHilited(kSessionActiveWindow))
		{
			// check the active window in the menu
			CheckMenuItem(inMenu, inItemNumber, true);
		}
		else
		{
			// remove any mark, initially
			CheckMenuItem(inMenu, inItemNumber, false);
			
			// use the Mac OS X convention of bullet-marking windows with unsaved changes
			if (IsWindowModified(kSessionActiveWindow)) SetItemMark(inMenu, inItemNumber, kBulletCharCode);
			
			// use the Mac OS X convention of diamond-marking minimized windows
			if (IsWindowCollapsed(kSessionActiveWindow)) SetItemMark(inMenu, inItemNumber, kDiamondCharCode);
		}
	}
	return result;
}// stateTrackerWindowMenuWindowItems

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
