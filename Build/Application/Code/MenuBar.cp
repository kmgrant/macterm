/*!	\file MenuBar.cp
	\brief Utilities for working with menus.
	
	This module is now mostly obsolete, as the majority
	of Cocoa-based menus are now handled directly by the
	Commands module.
*/
/*###############################################################

	MacTelnet
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

#include "UniversalDefines.h"

// standard-C includes
#include <algorithm>
#include <climits>

// library includes
#include <Console.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "Commands.h"
#include "MenuBar.h"



#pragma mark Internal Method Prototypes
namespace {

MenuItemIndex	getMenuAndMenuItemIndexByCommandID		(UInt32, MenuRef*);

} // anonymous namespace



#pragma mark Public Methods

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
"Commands.h", and the modifiers come from a
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


#pragma mark Internal Methods
namespace {

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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
