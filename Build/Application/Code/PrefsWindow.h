/*!	\file PrefsWindow.h
	\brief Implements the shell of the Preferences window.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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
#	import <CocoaExtensions.objc++.h>
#	import <CocoaFuture.objc++.h>
#	import <TouchBar.objc++.h>
@class CoreUI_HelpButton;
@class ListenerModel_StandardListener;
#endif

// application includes
#include "Preferences.h"
#include "QuillsPrefs.h"
#ifdef __OBJC__
#	import "Panel.h"
#	import "PrefsContextManager.objc++.h"
@class PrefPanelFormats_ViewManager;
@class PrefPanelFullScreen_ViewManager;
@class PrefPanelGeneral_ViewManager;
@class PrefPanelSessions_ViewManager;
@class PrefPanelTerminals_ViewManager;
@class PrefPanelTranslations_ViewManager;
#endif



#pragma mark Types

#ifdef __OBJC__

/*!
Panels that are destined for the Preferences window must implement
the following methods as well, not just the Panel interface.
*/
@protocol PrefsWindow_PanelInterface < NSObject > //{

	// for convenience, if a panel implements this NSColorPanel message then
	// the window controller will forward the message to the panel
	//- (void)
	//changeColor:(id)_;

	// for convenience, if a panel implements this NSFontPanel message then
	// the window controller will forward the message to the panel
	//- (void)
	//changeFont:(id)_;

	// return the category of settings edited by the panel
	- (Quills::Prefs::Class)
	preferencesClass;

@end //}


/*!
Implements the window class of PrefsWindow_Controller.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefsWindow_Object : NSWindow //{
{
}

// initializers
	+ (void)
	initialize;
	- (instancetype)
	initWithContentRect:(NSRect)_
	styleMask:(NSUInteger)_
	backing:(NSBackingStoreType)_
	defer:(BOOL)_ NS_DESIGNATED_INITIALIZER;

@end //}


/*!
Implements the Cocoa window that wraps the Cocoa version of
the Preferences window that is under development.  See
"PrefsWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefsWindow_Controller : NSWindowController< NSSplitViewDelegate,
														NSToolbarDelegate,
														Panel_Parent > //{
{
	IBOutlet NSView*		windowFirstResponder;
	IBOutlet NSView*		windowLastResponder;
	IBOutlet NSTabView*		containerTabView;
	IBOutlet NSView*		sourceListBackdrop;
	IBOutlet NSView*		sourceListContainer;
	IBOutlet NSTableView*	sourceListTableView;
	IBOutlet NSView*		sourceListSegmentedControl;
	IBOutlet CoreUI_HelpButton*		sourceListHelpButton;
	IBOutlet CoreUI_HelpButton*		mainViewHelpButton;
	IBOutlet NSSearchField*	searchField;
	IBOutlet NSView*		verticalSeparator;
@private
	NSIndexSet*							currentPreferenceCollectionIndexes;
	NSMutableArray*						currentPreferenceCollections;
	NSMutableArray*						panelIDArray; // ordered array of "panelIdentifier" values
	NSMutableDictionary*				panelsByID; // view managers (Panel_ViewManager subclass) from "panelIdentifier" keys
	NSMutableDictionary*				windowSizesByID; // NSArray* values (each with 2 NSNumber*) from "panelIdentifier" keys
	NSMutableDictionary*				windowMinSizesByID; // NSArray* values (each with 2 NSNumber*) from "panelIdentifier" keys
	NSString*							_searchText;
	NSSize								extraWindowContentSize; // stores extra content width and height (not belonging to a panel)
	ListenerModel_StandardListener*		preferenceChangeListener;
	TouchBar_Controller*				_touchBarController; // created on demand
	Panel_ViewManager< PrefsWindow_PanelInterface >*	activePanel;
}

// class methods
	+ (instancetype)
	sharedPrefsWindowController;

// accessors
	@property (strong) NSIndexSet*
	currentPreferenceCollectionIndexes; // binding
	- (NSArray*)
	currentPreferenceCollections; // binding
	@property (copy) NSString*
	searchText; // binding

// actions
	- (IBAction)
	performAddNewPreferenceCollection:(id)_;
	- (IBAction)
	performContextSensitiveHelp:(id)_;
	- (IBAction)
	performCopyPreferenceCollectionToDefault:(id)_;
	- (IBAction)
	performDuplicatePreferenceCollection:(id)_;
	- (IBAction)
	performExportPreferenceCollectionToFile:(id)_;
	- (IBAction)
	performFind:(id)_;
	- (IBAction)
	performImportPreferenceCollectionFromFile:(id)_;
	- (IBAction)
	performRemovePreferenceCollection:(id)_;
	- (IBAction)
	performRenamePreferenceCollection:(id)_;
	- (IBAction)
	performSearch:(id)_;
	- (IBAction)
	performSegmentedControlAction:(id)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

void
	PrefsWindow_AddCollection		(Preferences_ContextRef		inReferenceContextToCopy,
									 Preferences_TagSetRef		inTagSetOrNull = nullptr,
									 UInt32						inPrefPanelShowCommandIDOrZero = 0);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
