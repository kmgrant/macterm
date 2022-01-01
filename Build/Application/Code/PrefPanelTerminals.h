/*!	\file PrefPanelTerminals.h
	\brief Implements the Terminals panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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
#include "GenericPanelTabs.h"
#include "Panel.h"
#include "Preferences.h"
#ifdef __OBJC__
#	include "PreferenceValue.objc++.h"
#	include "PrefsContextManager.objc++.h"
#endif
#include "PrefsWindow.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Loads a NIB file that defines a panel view with tabs
and provides the sub-panels that the tabs contain.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelTerminals_ViewManager : GenericPanelTabs_ViewManager @end


@class PrefPanelTerminals_EmulationActionHandler; // implemented internally


/*!
Implements the “emulation” panel.
*/
@interface PrefPanelTerminals_EmulationVC : Panel_ViewManager< Panel_Delegate,
																PrefsWindow_PanelInterface > //{
{
@private
	NSRect										_idealFrame;
	PrefPanelTerminals_EmulationActionHandler*	_actionHandler;
}

// accessors
	@property (strong) PrefPanelTerminals_EmulationActionHandler*
	actionHandler;

@end //}


@class PrefPanelTerminals_OptionsActionHandler; // implemented internally


/*!
Implements the “terminal options” panel.
*/
@interface PrefPanelTerminals_OptionsVC : Panel_ViewManager< Panel_Delegate,
																PrefsWindow_PanelInterface > //{
{
@private
	NSRect										_idealFrame;
	PrefPanelTerminals_OptionsActionHandler*	_actionHandler;
}

// accessors
	@property (strong) PrefPanelTerminals_OptionsActionHandler*
	actionHandler;

@end //}


@class PrefPanelTerminals_ScreenActionHandler; // implemented internally


/*!
Implements the “terminal screen size” panel.
*/
@interface PrefPanelTerminals_ScreenVC : Panel_ViewManager< Panel_Delegate,
															PrefsWindow_PanelInterface > //{
{
@private
	NSRect										_idealFrame;
	PrefPanelTerminals_ScreenActionHandler*		_actionHandler;
}

// accessors
	@property (strong) PrefPanelTerminals_ScreenActionHandler*
	actionHandler;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

Preferences_TagSetRef
	PrefPanelTerminals_NewEmulationPaneTagSet	();

Preferences_TagSetRef
	PrefPanelTerminals_NewOptionsPaneTagSet		();

Preferences_TagSetRef
	PrefPanelTerminals_NewScreenPaneTagSet		();

Preferences_TagSetRef
	PrefPanelTerminals_NewTagSet				();

// BELOW IS REQUIRED NEWLINE TO END FILE
