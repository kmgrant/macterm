/*!	\file MenuBar.h
	\brief Lists methods for menu management.
	
	MacTelnet 3.0 attempts to make menu access significantly
	more abstract, and adds a number of utilities as well.
	Having said that, there are still a few APIs in here that
	are essentially hacks and they will be removed eventually.
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

#ifndef __MENUBAR__
#define __MENUBAR__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// MacTelnet includes
#include "ConstantsRegistry.h"



#pragma mark Constants

typedef SInt16 MenuBar_Menu;
enum
{
	// menu bar menu specifiers that are independent of Simplified User Interface mode
	kMenuBar_MenuFile = 1,
	kMenuBar_MenuEdit = 2,
	kMenuBar_MenuView = 3,
	kMenuBar_MenuTerminal = 4,
	kMenuBar_MenuKeys = 5,
	kMenuBar_MenuNetwork = 6,
	kMenuBar_MenuWindow = 7
};



#pragma mark Callbacks

/*!
Menu Command State Tracker Method

A request to set the state of a particular menu item in
a particular menu appropriately for the given command ID.

You should NOT modify the menu using by-command-ID APIs,
you should ONLY use the given menu reference and menu
item index.  In other words, do not assume that there is
a command ID association you can glean from the menu item
alone, assume that the given item is the one you should
modify and assume the item represents the given command.

A state tracker should explicitly set any aspect of an
item’s state that could change, such as its mark, icon,
or text.  The enabled state, however, should not be set,
but rather returned as a Boolean flag from the function
call (this is for convenience, as it is the most common
kind of state change).

You should also not set the modifier key information (such
as which modifier key glyphs appear), as this is
determined automatically based on command ID variant
mappings done elsewhere.

Every MacTelnet menu command has a state tracking method
attached to it as its reference constant.  Each such
method reports on the proper state for one or more
menu items.  Then, prior to the user selecting from the
menu bar, using a key equivalent, or opening a contextual
menu, every command’s state tracking method will be
invoked to determine the correct state for each command.
*/
typedef Boolean (*MenuCommandStateTrackerProcPtr)	(UInt32			inCommandID,
													 MenuRef		inMenu,
													 MenuItemIndex	inItem);
inline Boolean
MenuBar_InvokeCommandStateTracker	(MenuCommandStateTrackerProcPtr		inUserRoutine,
									 UInt32								inCommandID,
									 MenuRef							inMenu,
									 MenuItemIndex						inItem)
{
	return (*inUserRoutine)(inCommandID, inMenu, inItem);
}



#pragma mark Public Methods

//!\name Responding to Commands
//@{

Boolean
	MenuBar_HandleMenuCommand						(MenuRef						inMenu,
													 MenuItemIndex					inMenuItemIndex);

Boolean
	MenuBar_HandleMenuCommandByID					(UInt32							inCommandID);

//@}

//!\name Updating Menu Item States
//@{

void
	MenuBar_SetMenuItemStateTrackerProc				(MenuRef						inMenu,
													 MenuItemIndex					inItemIndex,
													 MenuCommandStateTrackerProcPtr	inProc);

void
	MenuBar_SetMenuItemStateTrackerProcByCommandID	(UInt32							inCommandID,
													 MenuCommandStateTrackerProcPtr	inProc);

void
	MenuBar_SetUpMenuItemState						(UInt32							inCommandID);

//@}

//!\name Utilities
//@{

OSStatus
	MenuBar_CopyMenuItemTextByCommandID				(UInt32							inCommandID,
													 CFStringRef&					outText);

Boolean
	MenuBar_GetMenuTitleRectangle					(MenuBar_Menu					inMenuBarMenuSpecifier,
													 Rect*							outMenuBarMenuTitleRect);

void
	MenuBar_GetUniqueMenuItemText					(MenuRef						inMenu,
													 Str255							inoutItemText);

void
	MenuBar_GetUniqueMenuItemTextCFString			(MenuRef						inMenu,
													 CFStringRef					inItemText,
													 CFStringRef&					outUniqueItemText);

Boolean
	MenuBar_IsMenuCommandEnabled					(UInt32							inCommandID);

Boolean
	MenuBar_IsMenuItemUnique						(MenuRef						inMenu,
													 ConstStringPtr					inItemText);

Boolean
	MenuBar_IsMenuItemUniqueCFString				(MenuRef						inMenu,
													 CFStringRef					inItemText);

MenuRef
	MenuBar_NewMenuWithArbitraryID					();

MenuItemIndex
	MenuBar_ReturnMenuItemIndexByItemText			(MenuRef						inMenu,
													 CFStringRef					inItemText);

MenuID
	MenuBar_ReturnUniqueMenuID						();

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
