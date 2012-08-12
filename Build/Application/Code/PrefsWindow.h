/*!	\file PrefsWindow.h
	\brief Implements the shell of the Preferences window.
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

#ifndef __PREFSWINDOW__
#define __PREFSWINDOW__

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif

// library includes
#ifdef __OBJC__
@class ListenerModel_StandardListener;
#else
class ListenerModel_StandardListener;
#endif

// application includes
#include "QuillsPrefs.h"
#ifdef __OBJC__
#	import "Panel.h"
#	import "PrefsContextManager.objc++.h"
@class PrefPanelFormats_ViewManager;
@class PrefPanelFullScreen_ViewManager;
@class PrefPanelGeneral_ViewManager;
@class PrefPanelTerminals_ViewManager;
@class PrefPanelTranslations_ViewManager;
#endif



#pragma mark Types

#ifdef __OBJC__

/*!
Panels that are destined for the Preferences window must implement
the following methods as well, not just the Panel interface.
*/
@protocol PrefsWindow_PanelInterface

// for convenience, if a panel implements this NSColorPanel message then
// the window controller will forward the message to the panel
//- (void)
//changeColor:(id)_;

// for convenience, if a panel implements this NSFontPanel message then
// the window controller will forward the message to the panel
//- (void)
//changeFont:(id)_;

- (Quills::Prefs::Class)
preferencesClass;

@end // PrefsWindow_PanelInterface


/*!
Presents methods that greatly simplify bindings to values and the
various states of the user interface elements that control them.
This class must have subclasses in order to be useful.

Bind a checkbox’s value to the "inherited" path and its enabled
state to the "inheritEnabled" path to implement a box that resets
a preference to the parent context value (by deleting data).

Bind an appropriate editor control (e.g. a color box) to an
appropriate subclass-provided path for editing a particular kind
of value (e.g. in this case, "colorValue" is a likely path).
*/
@interface PrefsWindow_InheritedContent : NSObject
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

- (void)
setInherited:(BOOL)_; // binding (to subclasses, typically)

- (BOOL)
isInheritEnabled; // binding (to subclasses, typically)

- (Preferences_Tag)
preferencesTag;

- (PrefsContextManager_Object*)
prefsMgr;

- (void)
didSetPreferenceValue;
- (void)
willSetPreferenceValue;

// overrides for subclasses (none of these is implemented in the base!)

- (BOOL)
isInherited; // subclasses MUST implement; binding (to subclasses, typically)

- (void)
setNilPreferenceValue; // subclasses MUST implement

@end


/*!
Manages bindings for a single color preference.
*/
@interface PrefsWindow_ColorContent : PrefsWindow_InheritedContent
{
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

@end


/*!
Manages bindings for any preference whose value is
defined to be Boolean.
*/
@interface PrefsWindow_FlagContent : PrefsWindow_InheritedContent
{
	BOOL	inverted;
}

- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_;

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_
inverted:(BOOL)_;

// new methods

- (BOOL)
readValueSeeIfDefault:(BOOL*)_;

// accessors

- (NSNumber*)
numberValue;
- (void)
setNumberValue:(NSNumber*)_; // binding

@end


/*!
Manages bindings for any preference whose value is
defined to be a pointer to a CFStringRef.
*/
@interface PrefsWindow_StringContent : PrefsWindow_InheritedContent
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
Implements the Cocoa window that wraps the Cocoa version of
the Preferences window that is under development.  See
"PrefsWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefsWindow_Controller : NSWindowController
{
	IBOutlet NSView*		windowFirstResponder;
	IBOutlet NSView*		windowLastResponder;
	IBOutlet NSTabView*		containerTabView;
	IBOutlet NSView*		sourceListContainer;
	IBOutlet NSTableView*	sourceListTableView;
	IBOutlet NSView*		sourceListSegmentedControl;
	IBOutlet NSView*		horizontalDividerView;
@private
	NSIndexSet*							currentPreferenceCollectionIndexes;
	NSMutableArray*						currentPreferenceCollections;
	NSMutableArray*						panelIDArray; // ordered array of "panelIdentifier" values
	NSMutableDictionary*				panelsByID; // view managers (Panel_ViewManager subclass) from "panelIdentifier" keys
	NSMutableDictionary*				windowSizesByID; // NSArray* values (each with 2 NSNumber*) from "panelIdentifier" keys
	NSMutableDictionary*				windowMinSizesByID; // NSArray* values (each with 2 NSNumber*) from "panelIdentifier" keys
	NSSize								extraWindowContentSize; // stores extra content width and height (not belonging to a panel)
	BOOL								isSourceListHidden;
	ListenerModel_StandardListener*		preferenceChangeListener;
	Panel_ViewManager< PrefsWindow_PanelInterface >*	activePanel;
	PrefPanelFormats_ViewManager*		formatsPanel;
	PrefPanelGeneral_ViewManager*		generalPanel;
	PrefPanelTerminals_ViewManager*		terminalsPanel;
	PrefPanelTranslations_ViewManager*	translationsPanel;
	PrefPanelFullScreen_ViewManager*	fullScreenPanel;
}

+ (id)
sharedPrefsWindowController;

// accessors

- (NSIndexSet*)
currentPreferenceCollectionIndexes;
- (void)
setCurrentPreferenceCollectionIndexes:(NSIndexSet*)_; // binding

- (NSArray*)
currentPreferenceCollections; // binding

- (BOOL)
isSourceListHidden; // binding
- (BOOL)
isSourceListShowing; // binding
- (void)
setSourceListHidden:(BOOL)_;

// actions

- (IBAction)
performAddNewPreferenceCollection:(id)_;

- (IBAction)
performContextSensitiveHelp:(id)_;

- (IBAction)
performDuplicatePreferenceCollection:(id)_;

- (IBAction)
performExportPreferenceCollectionToFile:(id)_;

- (IBAction)
performImportPreferenceCollectionFromFile:(id)_;

- (IBAction)
performRemovePreferenceCollection:(id)_;

- (IBAction)
performRenamePreferenceCollection:(id)_;

- (IBAction)
performSegmentedControlAction:(id)_;

@end

#endif // __OBJC__



#pragma mark Public Methods

void
	PrefsWindow_Done				();

void
	PrefsWindow_Display				();

void
	PrefsWindow_Remove				();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
