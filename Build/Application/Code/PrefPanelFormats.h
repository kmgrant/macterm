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
Presents methods that greatly simplify bindings to
color values and the various states of the user
interface elements that control them.
*/
@interface PrefPanelFormats_StandardColorContent : NSObject
{
@private
	PrefsContextManager_Object*		prefsMgr;
	Preferences_Tag					preferencesTag;
}

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_;

// accessors

- (NSColor*)
colorValue;
- (void)
setColorValue:(NSColor*)_; // binding

- (BOOL)
isInheritEnabled; // binding

- (BOOL)
isInherited;
- (void)
setInherited:(BOOL)_; // binding

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

- (PrefPanelFormats_StandardColorContent*)
blackBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
blackNormalColor; // binding

- (PrefPanelFormats_StandardColorContent*)
redBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
redNormalColor; // binding

- (PrefPanelFormats_StandardColorContent*)
greenBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
greenNormalColor; // binding

- (PrefPanelFormats_StandardColorContent*)
yellowBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
yellowNormalColor; // binding

- (PrefPanelFormats_StandardColorContent*)
blueBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
blueNormalColor; // binding

- (PrefPanelFormats_StandardColorContent*)
magentaBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
magentaNormalColor; // binding

- (PrefPanelFormats_StandardColorContent*)
cyanBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
cyanNormalColor; // binding

- (PrefPanelFormats_StandardColorContent*)
whiteBoldColor; // binding

- (PrefPanelFormats_StandardColorContent*)
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
