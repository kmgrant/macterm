/*!	\file MenuBar.cp
	\brief Utilities for working with menus.
	
	This module is now mostly obsolete, as the majority
	of Cocoa-based menus are now handled directly by the
	Commands module.
*/
/*###############################################################

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

#include "UniversalDefines.h"

// standard-C includes
#include <algorithm>
#include <climits>

// library includes
#include <Console.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// resource includes
#include "ApplicationVersion.h"
#include "GeneralResources.h"
#include "MenuResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "MenuBar.h"



#pragma mark Internal Method Prototypes
namespace {

void			adjustMenuItem							(MenuRef, MenuItemIndex, UInt32);
void			adjustMenuItemByCommandID				(UInt32);
MenuItemIndex	getMenuAndMenuItemIndexByCommandID		(UInt32, MenuRef*);
void			getMenuItemAdjustmentProc				(MenuRef, MenuItemIndex, MenuCommandStateTrackerProcPtr*);
void			setMenuItemEnabled						(MenuRef, UInt16, Boolean);

} // anonymous namespace



#pragma mark Public Methods

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
			kWindowMenuTitleWidthApproximation = 72, // pixels
			kWindowMenuTitleLeftEdgeApproximation = 466, // pixel offset
		};
		
		
		switch (inMenuBarMenuSpecifier)
		{
		case kMenuBar_MenuWindow:
			// approximate the rectangle of the Window menu (there’s gotta be a beautiful
			// way to get this just right, but I’m not finding it now)
			*outMenuBarMenuTitleRect = (**(GetMainDevice())).gdRect;
			outMenuBarMenuTitleRect->bottom = GetMBarHeight();
			outMenuBarMenuTitleRect->left += kWindowMenuTitleLeftEdgeApproximation;
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
	MenuRef							menu = nullptr;
	MenuItemIndex					itemIndex = 0;
	MenuCommandStateTrackerProcPtr	proc = nullptr;
	Boolean							result = false;
	
	
	itemIndex = getMenuAndMenuItemIndexByCommandID(inCommandID, &menu);
	if ((nullptr != menu) && (itemIndex > 0)) // only look at the primary menu (both should be the same anyway)
	{
		getMenuItemAdjustmentProc(menu, itemIndex, &proc);
		if (nullptr != proc) result = MenuBar_InvokeCommandStateTracker(proc, inCommandID, menu, itemIndex);
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
	MenuRef			menu = nullptr;
	MenuItemIndex	itemIndex = 0;
	
	
	itemIndex = getMenuAndMenuItemIndexByCommandID(inCommandID, &menu);
	if ((nullptr != menu) && (0 != itemIndex))
	{
		MenuBar_SetMenuItemStateTrackerProc(menu, itemIndex, inProc);
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
Automatically sets the enabled state and, as appropriate, the
mark, item text, etc. for a menu item using only its command ID.

For this method to work, you must have previously associated a
state tracking procedure with the item using
MenuBar_SetMenuItemStateTrackerProcByCommandID().

This is called by MenuBar_SetUpMenuItemState(), and is probably
not needed any place else.

(3.0)
*/
void
adjustMenuItemByCommandID	(UInt32		inCommandID)
{
	MenuRef			menu = nullptr;
	MenuItemIndex	itemIndex = 0;
	
	
	itemIndex = getMenuAndMenuItemIndexByCommandID(inCommandID, &menu);
	if (nullptr != menu)
	{
		MenuRef			foundMenu = nullptr;
		MenuItemIndex	foundIndex = 0;
		OSStatus		error = noErr;
		
		
		adjustMenuItem(menu, itemIndex, inCommandID);
		
		// some commands are repeated several times (e.g. for Favorites lists),
		// requiring iteration to find all of the items that have this command ID
		for (MenuItemIndex i = 2; ((noErr == error) && (i < 50/* arbitrary */)); ++i)
		{
			error = GetIndMenuItemWithCommandID(menu/* starting point */, inCommandID,
												i/* which matching item to return */,
												&foundMenu, &foundIndex);
			if (noErr == error)
			{
				adjustMenuItem(foundMenu, foundIndex, inCommandID);
			}
		}
	}
}// adjustMenuItemByCommandID


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

(3.0)
*/
MenuItemIndex
getMenuAndMenuItemIndexByCommandID	(UInt32		inCommandID,
									 MenuRef*	outMenu)
{
	MenuItemIndex	result = 0;
	OSStatus		error = noErr;
	
	
	// These searches start from the root menu; it may be possible to
	// write a faster scan (no speed test has been done) by making
	// the code more dependent on known commands IDs, but this
	// approach has definite maintainability advantages.
	if (nullptr != outMenu)
	{
		error = GetIndMenuItemWithCommandID(nullptr/* starting point */, inCommandID,
											1/* which matching item to return */,
											outMenu/* matching menu */,
											&result/* matching index, or nullptr */);
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
