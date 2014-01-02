/*!	\file MenuBar.cp
	\brief Utilities for working with menus.
	
	This module is now mostly obsolete, as the majority
	of Cocoa-based menus are now handled directly by the
	Commands module.
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#include "MenuBar.h"
#include <UniversalDefines.h>

// standard-C includes
#include <algorithm>
#include <climits>

// library includes
#include <Console.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// application includes
#include "Commands.h"



#pragma mark Public Methods

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
Deprecated; just use Commands_ExecuteByID() from now on.

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

// BELOW IS REQUIRED NEWLINE TO END FILE
