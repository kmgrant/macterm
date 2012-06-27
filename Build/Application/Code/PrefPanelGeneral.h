/*!	\file PrefPanelGeneral.h
	\brief Implements the General panel of Preferences.
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

#ifndef __PREFPANELGENERAL__
#define __PREFPANELGENERAL__

// application includes
#include "GenericPanelTabs.h"
#include "Panel.h"
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
@interface PrefPanelGeneral_ViewManager : GenericPanelTabs_ViewManager
{
}

@end


/*!
Loads a NIB file that defines the Options pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelGeneral_OptionsViewManager : Panel_ViewManager< Panel_Delegate, PrefsWindow_PanelInterface >
{
@private
	PrefsContextManager_Object*		prefsMgr;
}

// accessors

- (BOOL)
noWindowCloseOnProcessExit;
- (void)
setNoWindowCloseOnProcessExit:(BOOL)_; // binding

- (BOOL)
noAutomaticNewWindows;
- (void)
setNoAutomaticNewWindows:(BOOL)_; // binding

- (BOOL)
fadeInBackground;
- (void)
setFadeInBackground:(BOOL)_; // binding

- (BOOL)
invertSelectedText;
- (void)
setInvertSelectedText:(BOOL)_; // binding

- (BOOL)
automaticallyCopySelectedText;
- (void)
setAutomaticallyCopySelectedText:(BOOL)_; // binding

- (BOOL)
moveCursorToTextDropLocation;
- (void)
setMoveCursorToTextDropLocation:(BOOL)_; // binding

- (BOOL)
doNotDimBackgroundTerminalText;
- (void)
setDoNotDimBackgroundTerminalText:(BOOL)_; // binding

- (BOOL)
doNotWarnAboutMultiLinePaste;
- (void)
setDoNotWarnAboutMultiLinePaste:(BOOL)_; // binding

- (BOOL)
treatBackquoteLikeEscape;
- (void)
setTreatBackquoteLikeEscape:(BOOL)_; // binding

- (BOOL)
focusFollowsMouse;
- (void)
setFocusFollowsMouse:(BOOL)_; // binding

@end


/*!
Loads a NIB file that defines the Special pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelGeneral_SpecialViewManager : Panel_ViewManager< Panel_Delegate, PrefsWindow_PanelInterface >
{
@private
	PrefsContextManager_Object*		prefsMgr;
}

// accessors

- (BOOL)
cursorFlashes;
- (void)
setCursorFlashes:(BOOL)_; // binding

- (BOOL)
cursorShapeIsBlock;
- (void)
setCursorShapeIsBlock:(BOOL)_; // binding

- (BOOL)
cursorShapeIsThickUnderline;
- (void)
setCursorShapeIsThickUnderline:(BOOL)_; // binding

- (BOOL)
cursorShapeIsThickVerticalBar;
- (void)
setCursorShapeIsThickVerticalBar:(BOOL)_; // binding

- (BOOL)
cursorShapeIsUnderline;
- (void)
setCursorShapeIsUnderline:(BOOL)_; // binding

- (BOOL)
cursorShapeIsVerticalBar;
- (void)
setCursorShapeIsVerticalBar:(BOOL)_; // binding

- (BOOL)
isNewCommandCustomNewSession;
- (void)
setNewCommandCustomNewSession:(BOOL)_; // binding

- (BOOL)
isNewCommandDefaultSessionFavorite;
- (void)
setNewCommandDefaultSessionFavorite:(BOOL)_; // binding

- (BOOL)
isNewCommandLogInShell;
- (void)
setNewCommandLogInShell:(BOOL)_; // binding

- (BOOL)
isNewCommandShell;
- (void)
setNewCommandShell:(BOOL)_; // binding

- (BOOL)
isWindowResizeEffectTerminalScreenSize;
- (void)
setWindowResizeEffectTerminalScreenSize:(BOOL)_; // binding

- (BOOL)
isWindowResizeEffectTextSize;
- (void)
setWindowResizeEffectTextSize:(BOOL)_; // binding

- (NSString*)
spacesPerTab;
- (void)
setSpacesPerTab:(NSString*)_; // binding

// actions

- (IBAction)
performSetWindowStackingOrigin:(id)_;

// validators

- (BOOL)
validateSpacesPerTab:(id*)_
error:(NSError**)_;

@end

#endif // __OBJC__



#pragma mark Public Methods

Panel_Ref
	PrefPanelGeneral_New					();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
