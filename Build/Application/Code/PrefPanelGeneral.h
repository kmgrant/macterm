/*!	\file PrefPanelGeneral.h
	\brief Implements the General panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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
@interface PrefPanelGeneral_ViewManager : GenericPanelTabs_ViewManager @end


@class PrefPanelGeneral_FullScreenActionHandler; // implemented internally


/*!
Implements the “Full Screen” panel.
*/
@interface PrefPanelGeneral_FullScreenVC : Panel_ViewManager< Panel_Delegate,
																PrefsWindow_PanelInterface > //{
{
@private
	NSRect										_idealFrame;
	PrefPanelGeneral_FullScreenActionHandler*	_actionHandler;
}

// accessors
	@property (strong) PrefPanelGeneral_FullScreenActionHandler*
	actionHandler;

@end //}


/*!
Loads a NIB file that defines the Notifications pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelGeneral_NotificationsViewManager : Panel_ViewManager< Panel_Delegate,
																			PrefsWindow_PanelInterface > //{
{
	NSMutableArray*		soundNames;
@private
	PrefsContextManager_Object*		prefsMgr;
	NSIndexSet*						soundNameIndexes;
	BOOL							_didLoadView;
}

// accessors
	- (BOOL)
	alwaysUseVisualBell;
	- (void)
	setAlwaysUseVisualBell:(BOOL)_; // binding
	- (BOOL)
	backgroundBellsSendNotifications;
	- (void)
	setBackgroundBellsSendNotifications:(BOOL)_; // binding
	- (BOOL)
	isBackgroundNotificationNone;
	- (void)
	setBackgroundNotificationNone:(BOOL)_; // binding
	- (BOOL)
	isBackgroundNotificationChangeDockIcon;
	- (void)
	setBackgroundNotificationChangeDockIcon:(BOOL)_; // binding
	- (BOOL)
	isBackgroundNotificationAnimateIcon;
	- (void)
	setBackgroundNotificationAnimateIcon:(BOOL)_; // binding
	- (BOOL)
	isBackgroundNotificationDisplayMessage;
	- (void)
	setBackgroundNotificationDisplayMessage:(BOOL)_; // binding
	- (NSArray*)
	soundNames;
	- (NSIndexSet*)
	soundNameIndexes;
	- (void)
	setSoundNameIndexes:(NSIndexSet*)_; // binding

@end //}


@class PrefPanelGeneral_OptionsActionHandler; // implemented internally


/*!
Implements the “general options” panel.
*/
@interface PrefPanelGeneral_OptionsVC : Panel_ViewManager< Panel_Delegate,
															PrefsWindow_PanelInterface > //{
{
@private
	NSRect									_idealFrame;
	PrefPanelGeneral_OptionsActionHandler*	_actionHandler;
}

// accessors
	@property (strong) PrefPanelGeneral_OptionsActionHandler*
	actionHandler;

@end //}


/*!
Loads a NIB file that defines the Special pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelGeneral_SpecialViewManager : Panel_ViewManager< Panel_Delegate,
																	PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSMutableDictionary*			byKey;
}

// accessors: cursor settings
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

// accessors: other
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
	- (PreferenceValue_Number*)
	spacesPerTab; // binding

// actions
	- (IBAction)
	performSetWindowStackingOrigin:(id)_;

// validators
	- (BOOL)
	validateSpacesPerTab:(id*)_
	error:(NSError**)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

Preferences_TagSetRef
	PrefPanelGeneral_NewFullScreenTagSet	();

Preferences_TagSetRef
	PrefPanelGeneral_NewNotificationsTagSet	();

Preferences_TagSetRef
	PrefPanelGeneral_NewOptionsTagSet		();

Preferences_TagSetRef
	PrefPanelGeneral_NewSpecialTagSet		();

Preferences_TagSetRef
	PrefPanelGeneral_NewTagSet				();

// BELOW IS REQUIRED NEWLINE TO END FILE
