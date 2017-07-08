/*!	\file MenuBar.h
	\brief Lists methods for menu management.
	
	This has been largely deprecated in favor of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// application includes
#include "ConstantsRegistry.h"



#pragma mark Constants

// Main Menus
//
// These MUST agree with "MainMenuCocoa.xib".  In the Carbon days,
// these were menu IDs.  But for Cocoa, they are the "tag" values
// on each of the menus in the main menu.  So you could ask NSApp
// for "mainMenu", and then use "itemWithTag" with an ID below to
// find an NSMenuItem for the title, whose "submenu" is the NSMenu
// containing all of the items.
enum
{
	kMenuBar_MenuIDApplication = 512,
	kMenuBar_MenuIDFile = 513,
	kMenuBar_MenuIDEdit = 514,
	kMenuBar_MenuIDView = 515,
	kMenuBar_MenuIDTerminal = 516,
	kMenuBar_MenuIDKeys = 517,
	kMenuBar_MenuIDMacros = 518,
	kMenuBar_MenuIDWindow = 519,
		kMenuBar_MenuItemIDPrecedingWindowList = 123,
	kMenuBar_MenuIDHelp = 520,
	kMenuBar_MenuIDDebug = 521
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

MenuItemIndex
	MenuBar_ReturnMenuItemIndexByItemText			(MenuRef						inMenu,
													 CFStringRef					inItemText);

MenuID
	MenuBar_ReturnUniqueMenuID						();

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
