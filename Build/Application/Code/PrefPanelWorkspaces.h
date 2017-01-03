/*!	\file PrefPanelWorkspaces.h
	\brief Implements the Workspaces panel of Preferences.
	
	This panel defines groups of Sessions and their
	layout settings.
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
An object for user interface bindings; it indicates
whether the string value is a Session Favorite name
or a special type of session such as a log-in shell.
*/
@interface PrefPanelWorkspaces_SessionDescriptor : NSObject //{
{
@private
	NSNumber*	_commandType;
	NSString*	_sessionFavoriteName;
}

// accessors
	@property (strong) NSNumber*
	commandType; // nil for Session Favorite, or see corresponding preference setting
	@property (strong, readonly) NSString*
	description; // read-only; Session Favorite name or other description
	@property (strong) NSString*
	sessionFavoriteName; // the name of a Session Favorite; takes precedence if "commandType" is also set

@end //}


/*!
An object for user interface bindings; prior to use,
set a (Workspace-class) Preferences Context and a
particular window index.  The window name can be
changed, causing the corresponding preferences (the
window title) to be updated.
*/
@interface PrefPanelWorkspaces_WindowInfo : PrefsContextManager_Object< GenericPanelNumberedList_ListItemHeader > //{
{
@private
	Preferences_Index	_preferencesIndex;
}

// initializers
	- (instancetype)
	initWithIndex:(Preferences_Index)_ NS_DESIGNATED_INITIALIZER;

// accessors
	@property (readonly) Preferences_Index
	preferencesIndex;
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
	PreferenceValue_Rect*	frameObject;
	PreferenceValue_Rect*	screenBoundsObject;
	Preferences_Index		_preferencesIndex;
}

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_
	index:(Preferences_Index)_ NS_DESIGNATED_INITIALIZER;

// accessors
	// (value is set externally by panel; only bindings are for inherited/enabled)
	@property (assign) Preferences_Index
	preferencesIndex;

@end //}


/*!
Manages bindings for the window session preference.
*/
@interface PrefPanelWorkspaces_WindowSessionValue : PreferenceValue_Inherited //{
{
@private
	PreferenceValue_Number*					commandTypeObject;
	PreferenceValue_CollectionBinding*		sessionObject;
	NSMutableArray*							_descriptorArray;
	Preferences_Index						_preferencesIndex;
}

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_
	index:(Preferences_Index)_ NS_DESIGNATED_INITIALIZER;

// accessors
	- (BOOL)
	hasValue; // binding (read-only; depends on "currentValueDescriptor")
	- (NSArray*)
	valueDescriptorArray; // binding (objects of type PrefPanelWorkspaces_SessionDescriptor)
	- (PrefPanelWorkspaces_SessionDescriptor*)
	currentValueDescriptor;
	- (void)
	setCurrentValueDescriptor:(PrefPanelWorkspaces_SessionDescriptor*)_; // binding
	@property (assign) Preferences_Index
	preferencesIndex;

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
	PrefsContextManager_Object*		prefsMgr;
	NSSize							idealSize;
	NSMutableDictionary*			byKey;
}

// actions
	- (IBAction)
	performSetBoundary:(id)_;
	- (IBAction)
	performSetSessionToNone:(id)_;

// accessors
	- (PrefPanelWorkspaces_WindowBoundariesValue*)
	windowBoundaries; // binding
	- (PrefPanelWorkspaces_WindowSessionValue*)
	windowSession; // binding

@end //}

#endif // __OBJC__


#pragma mark Public Methods

Preferences_TagSetRef
	PrefPanelWorkspaces_NewTagSet		();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
