/*!	\file PrefPanelWorkspaces.h
	\brief Implements the Workspaces panel of Preferences.
	
	This panel defines groups of Sessions and their
	layout settings.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#ifndef __PREFPANELWORKSPACES__
#define __PREFPANELWORKSPACES__

// application includes
#include "GenericPanelNumberedList.h"
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
@interface PrefPanelWorkspaces_ViewManager : GenericPanelTabs_ViewManager @end


/*!
Loads a NIB file that defines the Options pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelWorkspaces_OptionsViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSSize							idealSize;
	NSMutableDictionary*			byKey;
}

// accessors
	- (PreferenceValue_Flag*)
	automaticallyEnterFullScreen; // binding
	- (PreferenceValue_Flag*)
	useTabbedWindows; // binding

@end //}


/*!
An object for user interface bindings; prior to use,
set a (Workspace-class) Preferences Context and a
particular window index.  The window name can be
changed, causing the corresponding preferences (the
window title) to be updated.
*/
@interface PrefPanelWorkspaces_WindowInfo : PrefsContextManager_Object< GenericPanelNumberedList_ListItemHeader > //{

// accessors
	@property (readonly) NSString*
	windowIndexLabel;
	@property (strong) NSString*
	windowName;

@end //}


@class PrefPanelWorkspaces_WindowEditorViewManager;


/*!
Loads a NIB file that defines the Windows pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelWorkspaces_WindowsViewManager : GenericPanelNumberedList_ViewManager< GenericPanelNumberedList_Master > //{
{
@private
	PrefPanelWorkspaces_WindowEditorViewManager*	_windowEditorViewManager;
}

// accessors
	@property (readonly) PrefPanelWorkspaces_WindowInfo*
	selectedWindowInfo;

@end //}


/*!
Manages bindings for the window boundaries preference.
*/
@interface PrefPanelWorkspaces_WindowBoundariesValue : PreferenceValue_Inherited //{
{
@private
	PreferenceValue_Flag*	frameObject;
	PreferenceValue_Flag*	screenBoundsObject;
}

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

// accessors
	// (value is set externally by panel; only bindings are for inherited/enabled)

@end //}


/*!
Manages bindings for the window session preference.
*/
@interface PrefPanelWorkspaces_WindowSessionValue : PreferenceValue_Inherited //{
{
@private
	PreferenceValue_Number*				commandTypeObject;
	PreferenceValue_CollectionBinding*	sessionObject;
}

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

// accessors
	- (NSArray*)
	sessionInfoArray; // binding
	- (id)
	currentSessionInfo;
	- (void)
	setCurrentSessionInfo:(id)_; // binding

@end //}


/*!
Loads a NIB file that defines the sub-pane that edits
a single window in the Windows pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelWorkspaces_WindowEditorViewManager : Panel_ViewManager<	Panel_Delegate,
																			PrefsWindow_PanelInterface > //{
{
@private
	NSSize					idealSize;
	NSMutableDictionary*	byKey;
}

// actions
	- (IBAction)
	performSetBoundary:(id)_;

// accessors
	- (PrefPanelWorkspaces_WindowBoundariesValue*)
	windowBoundaries; // binding
	- (PrefPanelWorkspaces_WindowSessionValue*)
	windowSession; // binding

@end //}

#endif // __OBJC__


#pragma mark Public Methods

Panel_Ref
	PrefPanelWorkspaces_New				();

Preferences_TagSetRef
	PrefPanelWorkspaces_NewTagSet		();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
