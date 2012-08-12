/*!	\file PrefPanelTerminals.h
	\brief Implements the Terminals panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#ifndef __PREFPANELTERMINALS__
#define __PREFPANELTERMINALS__

// application includes
#include "GenericPanelTabs.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefsContextManager.objc++.h"
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
@interface PrefPanelTerminals_ViewManager : GenericPanelTabs_ViewManager
{
}

@end


/*!
Loads a NIB file that defines the Options pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelTerminals_OptionsViewManager : Panel_ViewManager< Panel_Delegate, PrefsWindow_PanelInterface >
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
}

// accessors

- (PrefsWindow_FlagContent*)
wrapLines; // binding

- (PrefsWindow_FlagContent*)
eightBit; // binding

- (PrefsWindow_FlagContent*)
saveLinesOnClear; // binding

- (PrefsWindow_FlagContent*)
normalKeypadTopRow; // binding

- (PrefsWindow_FlagContent*)
localPageKeys; // binding

@end

#endif // __OBJC__



#pragma mark Public Methods

Panel_Ref
	PrefPanelTerminals_New					();

Panel_Ref
	PrefPanelTerminals_NewEmulationPane		();

Panel_Ref
	PrefPanelTerminals_NewOptionsPane		();

Preferences_TagSetRef
	PrefPanelTerminals_NewOptionsPaneTagSet	();

Panel_Ref
	PrefPanelTerminals_NewScreenPane		();

Preferences_TagSetRef
	PrefPanelTerminals_NewScreenPaneTagSet	();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
