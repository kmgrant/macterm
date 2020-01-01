/*!	\file ContextSensitiveMenu.h
	\brief Simplifies handling of contextual menus.
	
	Grouping is a convenient way to insert dividers free
	of headaches like tail dividers and double-dividers.
	All groups with at least one visible item are
	automatically delimited by dividers, and no other
	dividers are inserted.
*/
/*###############################################################

	Contexts Library
	© 1998-2020 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
    This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.

    You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <UniversalDefines.h>

#pragma once

// Mac includes
#ifdef __OBJC__
@class NSMenu;
@class NSMenuItem;
#else
class NSMenu;
class NSMenuItem;
#endif



#pragma mark Public Methods

//!\name Adding Items to Contextual Menus
//@{

// MUST BE CALLED EVERY TIME A NEW CONTEXTUAL MENU IS CONSTRUCTED, PRIOR TO CALLING OTHER APIS
void
	ContextSensitiveMenu_Init					();

void
	ContextSensitiveMenu_AddItem				(NSMenu*			inToWhichMenu,
												 NSMenuItem*		inItem);

// USE THIS METHOD EVERY TIME A DIVIDING LINE *MAY* BE NECESSARY (AVOIDS ADDING MORE THAN ONE DIVIDER)
void
	ContextSensitiveMenu_NewItemGroup			(CFStringRef		inTitleOrNull = nullptr);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
