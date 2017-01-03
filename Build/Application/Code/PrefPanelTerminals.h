/*!	\file PrefPanelTerminals.h
	\brief Implements the Terminals panel of Preferences.
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

#ifndef __PREFPANELTERMINALS__
#define __PREFPANELTERMINALS__

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


/*!
Manages bindings for the base emulator preference.
*/
@interface PrefPanelTerminals_BaseEmulatorValue : PreferenceValue_Array //{

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

@end //}


/*!
Manages bindings for the emulation tweaks preference.
*/
@interface PrefPanelTerminals_EmulationTweaksValue : PreferenceValue_Inherited //{
{
	NSArray*	featureArray; // array of Preference_Value* with "description" property
}

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

// accessors
	- (NSArray*)
	featureArray; // binding

@end //}


/*!
Loads a NIB file that defines the Emulation pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelTerminals_EmulationViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
	IBOutlet NSTableView*			tweaksTableView;
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
}

// accessors
	- (PrefPanelTerminals_BaseEmulatorValue*)
	baseEmulator; // binding
	- (PrefPanelTerminals_EmulationTweaksValue*)
	emulationTweaks; // binding
	- (PreferenceValue_String*)
	identity; // binding

@end //}


/*!
Loads a NIB file that defines the Options pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelTerminals_OptionsViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
}

// accessors
	- (PreferenceValue_Flag*)
	wrapLines; // binding
	- (PreferenceValue_Flag*)
	eightBit; // binding
	- (PreferenceValue_Flag*)
	saveLinesOnClear; // binding
	- (PreferenceValue_Flag*)
	normalKeypadTopRow; // binding
	- (PreferenceValue_Flag*)
	localPageKeys; // binding

@end //}


/*!
Manages bindings for the scrollback preference.
*/
@interface PrefPanelTerminals_ScrollbackValue : PreferenceValue_Inherited //{
{
	NSArray*					behaviorArray;
	PreferenceValue_Number*		behaviorObject;
	PreferenceValue_Number*		rowsObject;
}

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

// accessors
	- (NSArray*)
	behaviorArray; // binding
	- (id)
	currentBehavior;
	- (void)
	setCurrentBehavior:(id)_; // binding
	- (NSString*)
	rowsNumberStringValue;
	- (void)
	setRowsNumberStringValue:(NSString*)_; // binding
	- (BOOL)
	rowsEnabled; // binding

@end //}


/*!
Loads a NIB file that defines the Screen pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelTerminals_ScreenViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
}

// accessors
	- (PreferenceValue_Number*)
	screenWidth; // binding
	- (PreferenceValue_Number*)
	screenHeight; // binding
	- (PrefPanelTerminals_ScrollbackValue*)
	scrollback; // binding

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

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
