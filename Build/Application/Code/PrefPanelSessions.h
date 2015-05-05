/*!	\file PrefPanelSessions.h
	\brief Implements the Sessions panel of Preferences.
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

#ifndef __PREFPANELSESSIONS__
#define __PREFPANELSESSIONS__

// application includes
#include "GenericPanelTabs.h"
#include "Panel.h"
#include "Preferences.h"
#ifdef __OBJC__
#	include "PreferenceValue.objc++.h"
#	include "PrefsContextManager.objc++.h"
#endif
#include "ServerBrowser.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Loads a NIB file that defines a panel view with tabs
and provides the sub-panels that the tabs contain.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelSessions_ViewManager : GenericPanelTabs_ViewManager @end


/*!
Loads a NIB file that defines the Resource pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelSessions_ResourceViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface,
																		ServerBrowser_DataChangeObserver > //{
{
	IBOutlet NSView*					commandLineTextField;
@private
	PrefsContextManager_Object*			prefsMgr;
	ListenerModel_StandardListener*		preferenceChangeListener;
	NSRect								idealFrame;
	NSMutableDictionary*				byKey;
	ServerBrowser_Ref					_serverBrowser;
	NSIndexSet*							_sessionFavoriteIndexes;
	NSArray*							_sessionFavorites;
	BOOL								isEditingRemoteShell;
}

// accessors: preferences
	- (PreferenceValue_StringByJoiningArray*)
	commandLine; // binding
	- (PreferenceValue_CollectionBinding*)
	formatFavorite; // binding
	- (PreferenceValue_CollectionBinding*)
	terminalFavorite; // binding
	- (PreferenceValue_CollectionBinding*)
	translationFavorite; // binding

// accessors: low-level user interface state
	- (BOOL)
	isEditingRemoteShell; // binding
	@property (retain) NSIndexSet*
	sessionFavoriteIndexes; // binding
	@property (retain, readonly) NSArray*
	sessionFavorites; // binding

// accessors: internal bindings
	- (PreferenceValue_String*)
	serverHost;
	- (PreferenceValue_Number*)
	serverPort;
	- (PreferenceValue_Number*)
	serverProtocol;
	- (PreferenceValue_String*)
	serverUserID;

// actions
	- (IBAction)
	performSetCommandLineToDefaultShell:(id)_; // binding
	- (IBAction)
	performSetCommandLineToLogInShell:(id)_; // binding
	- (IBAction)
	performSetCommandLineToRemoteShell:(id)_; // binding

@end //}


/*!
Manages bindings for the capture-file preferences.
*/
@interface PrefPanelSessions_CaptureFileValue : PreferenceValue_Inherited //{
{
@private
	PreferenceValue_Flag*				enabledObject;
	PreferenceValue_Flag*				allowSubsObject;
	PreferenceValue_String*				fileNameObject;
	PreferenceValue_FileSystemObject*	directoryPathObject;
}

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

// accessors
	- (BOOL)
	isEnabled;
	- (void)
	setEnabled:(BOOL)_; // binding
	- (BOOL)
	allowSubstitutions;
	- (void)
	setAllowSubstitutions:(BOOL)_; // binding
	- (NSURL*)
	directoryPathURLValue;
	- (void)
	setDirectoryPathURLValue:(NSURL*)_; // binding
	- (NSString*)
	fileNameStringValue;
	- (void)
	setFileNameStringValue:(NSString*)_; // binding

@end //}


/*!
Loads a NIB file that defines the Data Flow pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelSessions_DataFlowViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
}

// accessors
	- (PreferenceValue_Flag*)
	localEcho; // binding
	- (PreferenceValue_Number*)
	lineInsertionDelay; // binding
	- (PreferenceValue_Number*)
	scrollingDelay; // binding
	- (PrefPanelSessions_CaptureFileValue*)
	captureToFile; // binding

@end //}


/*!
Manages bindings for the control-key preferences.
The only valid preference tags are those that store
raw control-key characters, such as
"kPreferences_TagKeyInterruptProcess".
*/
@interface PrefPanelSessions_ControlKeyValue : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_;

// accessors
	- (NSString*)
	stringValue;
	- (void)
	setStringValue:(NSString*)_; // binding

@end //}


/*!
Manages bindings for the Emacs-meta-key mapping preference.
*/
@interface PrefPanelSessions_EmacsMetaValue : PreferenceValue_Array //{

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

@end //}


/*!
Manages bindings for the new-line preference.
*/
@interface PrefPanelSessions_NewLineValue : PreferenceValue_Array //{

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

@end //}


/*!
Loads a NIB file that defines the Keyboard pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelSessions_KeyboardViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
	BOOL							isEditingKeyInterruptProcess;
	BOOL							isEditingKeyResume;
	BOOL							isEditingKeySuspend;
}

// accessors: preference values
	- (PreferenceValue_Flag*)
	deleteKeySendsBackspace; // binding
	- (PreferenceValue_Flag*)
	emacsArrowKeys; // binding
	- (PrefPanelSessions_ControlKeyValue*)
	keyInterruptProcess; // binding
	- (PrefPanelSessions_ControlKeyValue*)
	keyResume; // binding
	- (PrefPanelSessions_ControlKeyValue*)
	keySuspend; // binding
	- (PrefPanelSessions_EmacsMetaValue*)
	mappingForEmacsMeta; // binding
	- (PrefPanelSessions_NewLineValue*)
	mappingForNewLine; // binding

// accessors: low-level user interface state
	- (BOOL)
	isEditingKeyInterruptProcess; // binding
	- (BOOL)
	isEditingKeyResume; // binding
	- (BOOL)
	isEditingKeySuspend; // binding

// actions
	- (IBAction)
	performChooseInterruptProcessKey:(id)_; // binding
	- (IBAction)
	performChooseResumeKey:(id)_; // binding
	- (IBAction)
	performChooseSuspendKey:(id)_; // binding

@end //}


/*!
Manages bindings for the TEK-mode preference.
*/
@interface PrefPanelSessions_GraphicsModeValue : PreferenceValue_Array //{

// initializers
	- (instancetype)
	initWithContextManager:(PrefsContextManager_Object*)_;

@end //}


/*!
Loads a NIB file that defines the Graphics pane.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelSessions_GraphicsViewManager : Panel_ViewManager< Panel_Delegate,
																		PrefsWindow_PanelInterface > //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSRect							idealFrame;
	NSMutableDictionary*			byKey;
}

// accessors
	- (PrefPanelSessions_GraphicsModeValue*)
	graphicsMode; // binding
	- (PreferenceValue_Flag*)
	pageClearsScreen; // binding

@end //}

#endif // __OBJC__



#pragma mark Public Methods

Panel_Ref
	PrefPanelSessions_New						();

Panel_Ref
	PrefPanelSessions_NewDataFlowPane			();

Preferences_TagSetRef
	PrefPanelSessions_NewDataFlowPaneTagSet		();

Panel_Ref
	PrefPanelSessions_NewGraphicsPane			();

Preferences_TagSetRef
	PrefPanelSessions_NewGraphicsPaneTagSet		();

Panel_Ref
	PrefPanelSessions_NewKeyboardPane			();

Preferences_TagSetRef
	PrefPanelSessions_NewKeyboardPaneTagSet		();

Panel_Ref
	PrefPanelSessions_NewResourcePane			();

Preferences_TagSetRef
	PrefPanelSessions_NewResourcePaneTagSet		();

Preferences_TagSetRef
	PrefPanelSessions_NewTagSet					();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
