/*!	\file MenuBar.h
	\brief Lists methods for menu management.
	
	This has been largely deprecated in favor of Cocoa.
*/
/*###############################################################

	MacTelnet
		© 1998-2010 by Kevin Grant.
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



#pragma mark Public Methods

//!\name Responding to Commands
//@{

Boolean
	MenuBar_HandleMenuCommand						(MenuRef						inMenu,
													 MenuItemIndex					inMenuItemIndex);

Boolean
	MenuBar_HandleMenuCommandByID					(UInt32							inCommandID);

//@}

//!\name Utilities
//@{

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
