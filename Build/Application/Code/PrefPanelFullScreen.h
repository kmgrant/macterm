/*!	\file PrefPanelFullScreen.h
	\brief Implements the Full Screen panel of Preferences.
	
	This panel lets the user configure Full Screen mode.
*/
/*###############################################################

	MacTerm
		© 1998-2013 by Kevin Grant.
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

#ifndef __PREFPANELFULLSCREEN__
#define __PREFPANELFULLSCREEN__

// application includes
#include "Panel.h"
#include "Preferences.h"
#include "PrefsContextManager.objc++.h"
#include "PrefsWindow.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Loads a NIB file that defines this panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelFullScreen_ViewManager : Panel_ViewManager< Panel_Delegate,
																PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
}

// accessors
	- (BOOL)
	isForceQuitEnabled;
	- (void)
	setForceQuitEnabled:(BOOL)_; // binding
	- (BOOL)
	isMenuBarShownOnDemand;
	- (void)
	setMenuBarShownOnDemand:(BOOL)_; // binding
	- (BOOL)
	isScrollBarVisible;
	- (void)
	setScrollBarVisible:(BOOL)_; // binding
	- (BOOL)
	isWindowFrameVisible;
	- (void)
	setWindowFrameVisible:(BOOL)_; // binding
	- (BOOL)
	offSwitchWindowEnabled;
	- (void)
	setOffSwitchWindowEnabled:(BOOL)_; // binding
	- (BOOL)
	superfluousEffectsEnabled;
	- (void)
	setSuperfluousEffectsEnabled:(BOOL)_; // binding

@end //}

#endif // __OBJC__



#pragma mark Public Methods

// CARBON LEGACY
Panel_Ref
	PrefPanelFullScreen_New			();

Preferences_TagSetRef
	PrefPanelFullScreen_NewTagSet	();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
