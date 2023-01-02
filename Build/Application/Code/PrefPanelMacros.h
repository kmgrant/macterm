/*!	\file PrefPanelMacros.h
	\brief Implements the Macros panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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

// application includes
#include "GenericPanelNumberedList.h"
#include "Preferences.h"


#pragma mark Types

#ifdef __OBJC__

/*!
Creates a panel with a numbered list of macros in a
set, combined with an editor for the selected item.
*/
@interface PrefPanelMacros_ViewManager : GenericPanelNumberedList_ViewManager< GenericPanelNumberedList_Master > @end

#endif // __OBJC__


#pragma mark Public Methods

Preferences_TagSetRef
	PrefPanelMacros_NewTagSet			();

// BELOW IS REQUIRED NEWLINE TO END FILE
