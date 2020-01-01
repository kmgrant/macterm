/*!	\file PrefPanelMacros.h
	\brief Implements the Macros panel of Preferences.
	
	This panel provides several advantages over the traditional
	macro editor: for one, it allows many sets of macros to be
	in memory at once.  Also, by re-classifying macros as a
	preferences category, persistence of macros is maintained
	(when before exporting and importing was necessary).  The
	modeless nature of the new Preferences window also means
	that macros can now be modified more easily than before,
	with instant effects.
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
#include "GenericPanelNumberedList.h"
#include "Keypads.h"
#include "Panel.h"
#include "Preferences.h"
#include "PreferenceValue.objc++.h"
#ifdef __OBJC__
#	include "PrefsContextManager.objc++.h"
#endif
#include "PrefsWindow.h"


#pragma mark Types

#ifdef __OBJC__

/*!
An object for user interface bindings; prior to use,
set a (Macro-Set-class) Preferences Context and a
particular macro index.  The macro name can be
changed, causing the corresponding preferences (the
macro name) to be updated.
*/
@interface PrefPanelMacros_MacroInfo : PrefsContextManager_Object< GenericPanelNumberedList_ItemBinding > //{
{
@private
	Preferences_Index	_preferencesIndex;
}

// initializers
	- (instancetype)
	init NS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithDefaultContextInClass:(Quills::Prefs::Class)_ NS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithIndex:(Preferences_Index)_ NS_DESIGNATED_INITIALIZER;

// accessors
	@property (readonly) Preferences_Index
	preferencesIndex;
	@property (readonly) NSString*
	macroIndexLabel;
	@property (strong) NSString*
	macroName;

@end //}


@class PrefPanelMacros_MacroEditorViewManager;


/*!
Loads a NIB file that defines the Windows pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelMacros_ViewManager : GenericPanelNumberedList_ViewManager< GenericPanelNumberedList_Master > //{
{
@private
	PrefPanelMacros_MacroEditorViewManager*		_macroEditorViewManager;
}

// accessors
	@property (readonly) PrefPanelMacros_MacroInfo*
	selectedMacroInfo;

@end //}


/*!
Manages bindings for the Action mapping preference.
*/
@interface PrefPanelMacros_ActionValue : PreferenceValue_Array //{

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

@end //}


/*!
Manages bindings for the Invoke With mapping preference.
*/
@interface PrefPanelMacros_InvokeWithValue : PreferenceValue_Array //{

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

// accessors
	- (NSString*)
	currentOrdinaryCharacter;
	- (void)
	setCurrentOrdinaryCharacter:(NSString*)_; // binding
	- (BOOL)
	isOrdinaryCharacter; // binding

@end //}


/*!
Manages bindings for the Modifiers mapping preference.
*/
@interface PrefPanelMacros_ModifiersValue : PreferenceValue_Array //{

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

@end //}


/*!
Loads a NIB file that defines this panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelMacros_MacroEditorViewManager : Panel_ViewManager< Keypads_ControlKeyResponder,
																		Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	IBOutlet NSTextField*			contentsField;
	NSSize							idealSize;
	BOOL							_bindControlKeyPad;
	NSMutableDictionary*			byKey;
}

// actions
	- (IBAction)
	performInsertControlKeyCharacter:(id)_;

// accessors
	@property (assign) BOOL
	bindControlKeyPad; // binding
	- (PrefPanelMacros_ActionValue*)
	macroAction; // binding
	- (PreferenceValue_String*)
	macroContents; // binding
	- (PrefPanelMacros_InvokeWithValue*)
	macroKey; // binding
	- (PrefPanelMacros_ModifiersValue*)
	macroModifiers; // binding

@end //}

#endif // __OBJC__


#pragma mark Public Methods

Preferences_TagSetRef
	PrefPanelMacros_NewTagSet			();

// BELOW IS REQUIRED NEWLINE TO END FILE
