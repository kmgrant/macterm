/*!	\file PrefPanelFormats.h
	\brief Implements the Formats panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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
#include "TerminalScreenRef.typedef.h"
#include "TerminalView.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Loads a NIB file that defines a panel view with tabs
and provides the sub-panels that the tabs contain.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelFormats_ViewManager : GenericPanelTabs_ViewManager @end


/*!
Manages bindings for a character-width preference.
*/
@interface PrefPanelFormats_CharacterWidthContent : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_;

// new methods
	- (Float32)
	readValueSeeIfDefault:(BOOL*)_;

// accessors
	- (NSNumber*)
	numberValue;
	- (void)
	setNumberValue:(NSNumber*)_; // binding

@end //}


/*!
Loads a NIB file that defines the General pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelFormats_GeneralViewManager : Panel_ViewManager< Panel_Delegate,
																	PrefsWindow_PanelInterface > //{
{
	IBOutlet NSView*				sampleViewContainer;
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
	TerminalView_Controller*		sampleTerminalVC;
	TerminalScreenRef				sampleScreenBuffer;
	TerminalViewRef					sampleScreenView;
}

// accessors
	- (PreferenceValue_Color*)
	normalBackgroundColor; // binding
	- (PreferenceValue_Color*)
	normalForegroundColor; // binding
	- (PreferenceValue_Color*)
	boldBackgroundColor; // binding
	- (PreferenceValue_Color*)
	boldForegroundColor; // binding
	- (PreferenceValue_Color*)
	blinkingBackgroundColor; // binding
	- (PreferenceValue_Color*)
	blinkingForegroundColor; // binding
	- (PreferenceValue_Color*)
	cursorBackgroundColor; // binding
	- (PreferenceValue_Flag*)
	autoSetCursorColor; // binding
	- (PreferenceValue_Color*)
	matteBackgroundColor; // binding
	- (PreferenceValue_String*)
	fontFamily; // binding
	- (PreferenceValue_Number*)
	fontSize; // binding
	- (PrefPanelFormats_CharacterWidthContent*)
	characterWidth; // binding

// actions
	- (IBAction)
	performFontSelection:(id)_;

@end //}


/*!
Loads a NIB file that defines the Standard Colors pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelFormats_StandardColorsViewManager : Panel_ViewManager< Panel_Delegate,
																			PrefsWindow_PanelInterface > //{
{
	IBOutlet NSView*				sampleViewContainer;
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
	TerminalView_Controller*		sampleTerminalVC;
	TerminalScreenRef				sampleScreenBuffer;
	TerminalViewRef					sampleScreenView;
}

// accessors
	- (PreferenceValue_Color*)
	blackBoldColor; // binding
	- (PreferenceValue_Color*)
	blackNormalColor; // binding
	- (PreferenceValue_Color*)
	redBoldColor; // binding
	- (PreferenceValue_Color*)
	redNormalColor; // binding
	- (PreferenceValue_Color*)
	greenBoldColor; // binding
	- (PreferenceValue_Color*)
	greenNormalColor; // binding
	- (PreferenceValue_Color*)
	yellowBoldColor; // binding
	- (PreferenceValue_Color*)
	yellowNormalColor; // binding
	- (PreferenceValue_Color*)
	blueBoldColor; // binding
	- (PreferenceValue_Color*)
	blueNormalColor; // binding
	- (PreferenceValue_Color*)
	magentaBoldColor; // binding
	- (PreferenceValue_Color*)
	magentaNormalColor; // binding
	- (PreferenceValue_Color*)
	cyanBoldColor; // binding
	- (PreferenceValue_Color*)
	cyanNormalColor; // binding
	- (PreferenceValue_Color*)
	whiteBoldColor; // binding
	- (PreferenceValue_Color*)
	whiteNormalColor; // binding

// actions
	- (IBAction)
	performResetStandardColors:(id)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

Preferences_TagSetRef
	PrefPanelFormats_NewANSIColorsTagSet	();

Preferences_TagSetRef
	PrefPanelFormats_NewNormalTagSet		();

Preferences_TagSetRef
	PrefPanelFormats_NewTagSet				();

// BELOW IS REQUIRED NEWLINE TO END FILE
