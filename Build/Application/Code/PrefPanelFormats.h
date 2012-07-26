/*!	\file PrefPanelFormats.h
	\brief Implements the Formats panel of Preferences.
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

#ifndef __PREFPANELFORMATS__
#define __PREFPANELFORMATS__

// application includes
#include "GenericPanelTabs.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefsContextManager.objc++.h"
#include "PrefsWindow.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalViewRef.typedef.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Loads a NIB file that defines a panel view with tabs
and provides the sub-panels that the tabs contain.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelFormats_ViewManager : GenericPanelTabs_ViewManager
{
}

@end


/*!
Manages bindings for a character-width preference.
*/
@interface PrefPanelFormats_CharacterWidthContent : PrefsWindow_InheritedContent
{
}

// designated initializer
- (id)
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

@end


/*!
Manages bindings for a font-size preference.
*/
@interface PrefPanelFormats_FontSizeContent : PrefsWindow_InheritedContent
{
}

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_;

// new methods

- (NSString*)
readValueSeeIfDefault:(BOOL*)_;

// accessors

- (NSString*)
stringValue;
- (void)
setStringValue:(NSString*)_; // binding

@end


/*!
Loads a NIB file that defines the General pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelFormats_GeneralViewManager : Panel_ViewManager< Panel_Delegate, PrefsWindow_PanelInterface >
{
	IBOutlet NSView*	terminalSampleBackgroundView;
	IBOutlet NSView*	terminalSamplePaddingView;
	IBOutlet NSView*	terminalSampleContentView;
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
	TerminalScreenRef				sampleScreenBuffer;
	TerminalViewRef					sampleScreenView;
}

// accessors

- (PrefsWindow_ColorContent*)
normalBackgroundColor; // binding

- (PrefsWindow_ColorContent*)
normalForegroundColor; // binding

- (PrefsWindow_ColorContent*)
boldBackgroundColor; // binding

- (PrefsWindow_ColorContent*)
boldForegroundColor; // binding

- (PrefsWindow_ColorContent*)
blinkingBackgroundColor; // binding

- (PrefsWindow_ColorContent*)
blinkingForegroundColor; // binding

- (PrefsWindow_ColorContent*)
matteBackgroundColor; // binding

- (PrefsWindow_StringContent*)
fontFamily; // binding

- (PrefPanelFormats_FontSizeContent*)
fontSize; // binding

- (PrefPanelFormats_CharacterWidthContent*)
characterWidth; // binding

// actions

- (IBAction)
performFontSelection:(id)_;

@end


/*!
Loads a NIB file that defines the Standard Colors pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelFormats_StandardColorsViewManager : Panel_ViewManager< Panel_Delegate, PrefsWindow_PanelInterface >
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
}

// accessors

- (PrefsWindow_ColorContent*)
blackBoldColor; // binding

- (PrefsWindow_ColorContent*)
blackNormalColor; // binding

- (PrefsWindow_ColorContent*)
redBoldColor; // binding

- (PrefsWindow_ColorContent*)
redNormalColor; // binding

- (PrefsWindow_ColorContent*)
greenBoldColor; // binding

- (PrefsWindow_ColorContent*)
greenNormalColor; // binding

- (PrefsWindow_ColorContent*)
yellowBoldColor; // binding

- (PrefsWindow_ColorContent*)
yellowNormalColor; // binding

- (PrefsWindow_ColorContent*)
blueBoldColor; // binding

- (PrefsWindow_ColorContent*)
blueNormalColor; // binding

- (PrefsWindow_ColorContent*)
magentaBoldColor; // binding

- (PrefsWindow_ColorContent*)
magentaNormalColor; // binding

- (PrefsWindow_ColorContent*)
cyanBoldColor; // binding

- (PrefsWindow_ColorContent*)
cyanNormalColor; // binding

- (PrefsWindow_ColorContent*)
whiteBoldColor; // binding

- (PrefsWindow_ColorContent*)
whiteNormalColor; // binding

// actions

- (IBAction)
performResetStandardColors:(id)_;

@end

#endif // __OBJC__



#pragma mark Public Methods

Panel_Ref
	PrefPanelFormats_New					();

Panel_Ref
	PrefPanelFormats_NewANSIColorsPane		();

Panel_Ref
	PrefPanelFormats_NewNormalPane			();

Preferences_TagSetRef
	PrefPanelFormats_NewTagSet				();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
