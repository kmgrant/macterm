/*!	\file PrefPanelWorkspaces.mm
	\brief Implements the Workspaces panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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

#import "PrefPanelWorkspaces.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <vector>

// Unix includes
#import <strings.h>

// Mac includes
#import <objc/objc-runtime.h>
@import CoreServices;

// library includes
#import <CFRetainRelease.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <Localization.h>
#import <ListenerModel.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "Panel.h"
#import "Preferences.h"
#import "SessionFactory.h"
#import "UIStrings.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>


#pragma mark Types

/*!
Implements SwiftUI interaction for the “workspace options” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelWorkspaces_OptionsActionHandler : NSObject< UIPrefsWorkspaceOptions_ActionHandling > //{
{
@private
	PrefsContextManager_Object*		_prefsMgr;
	UIPrefsWorkspaceOptions_Model*	_viewModel;
}

// new methods
	- (BOOL)
	resetToDefaultGetFlagWithTag:(Preferences_Tag)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsWorkspaceOptions_Model*
	viewModel;

@end //}


/*!
Private properties.
*/
@interface PrefPanelWorkspaces_OptionsVC () //{

// accessors
	@property (assign) CGRect
	idealFrame;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelWorkspaces_WindowSessionValue (PrefPanelWorkspaces_WindowSessionValueInternal) //{

// new methods
	- (void)
	rebuildSessionList;

@end //}


/*!
Private properties.
*/
@interface PrefPanelWorkspaces_WindowsViewManager () //{

// accessors
	@property (strong) PrefPanelWorkspaces_WindowEditorViewManager*
	windowEditorViewManager;

@end //}


/*!
Private properties.
*/
@interface PrefPanelWorkspaces_WindowEditorViewManager () //{

// accessors
	@property (weak, readonly) PrefPanelWorkspaces_WindowsViewManager*
	windowsViewManager;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelWorkspaces_WindowEditorViewManager (PrefPanelWorkspaces_WindowEditorViewManagerInternal) //{

// new methods
	- (void)
	configureForIndex:(Preferences_Index)_;

// accessors
	@property (readonly) NSArray*
	primaryDisplayBindingKeys;

@end //}



#pragma mark Public Methods

/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelWorkspaces_NewTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	for (Preferences_Index i = 1; i <= kPreferences_MaximumWorkspaceSize; ++i)
	{
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowSessionFavorite, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowCommandType, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowFrameBounds, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowScreenBounds, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowTitle, i));
	}
	tagList.push_back(kPreferences_TagArrangeWindowsUsingTabs);
	tagList.push_back(kPreferences_TagArrangeWindowsFullScreen);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods

#pragma mark -
@implementation PrefPanelWorkspaces_ViewManager //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers =
				@[
					[[PrefPanelWorkspaces_OptionsVC alloc] init],
					[[PrefPanelWorkspaces_WindowsViewManager alloc] init],
				];
	NSString*	panelName = NSLocalizedStringFromTable(@"Workspaces", @"PrefPanelWorkspaces",
														@"the name of this panel");
	NSImage*	panelIcon = nil;
	
	
	if (@available(macOS 11.0, *))
	{
		panelIcon = [NSImage imageWithSystemSymbolName:@"rectangle.3.offgrid" accessibilityDescription:self.panelName];
	}
	else
	{
		panelIcon = [NSImage imageNamed:@"IconForPrefPanelWorkspaces"];
	}
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Workspaces"
										localizedName:panelName
										localizedIcon:panelIcon
										viewManagerArray:subViewManagers];
	if (nil != self)
	{
	}
	return self;
}// init


@end //} PrefPanelWorkspaces_ViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_OptionsActionHandler //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		_prefsMgr = nil; // see "panelViewManager:initializeWithContext:"
		_viewModel = [[UIPrefsWorkspaceOptions_Model alloc] initWithRunner:self]; // transfer ownership
	}
	return self;
}// init


#pragma mark New Methods


/*!
Helper function for protocol methods; deletes the
given preference tag and returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetFlagWithTag:(Preferences_Tag)		aTag
{
	BOOL	result = NO;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:aTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", aTag);
	}
	
	// return default value
	{
		Boolean				preferenceValue = false;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, aTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", aTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = ((preferenceValue) ? YES : NO);
		}
	}
	
	return result;
}// resetToDefaultGetFlagWithTag:


/*!
Updates the view model’s observed properties based on
current preferences context data.

This is only needed when changing contexts.

See also "dataUpdated", which should be roughly the
inverse of this.

(2020.11)
*/
- (void)
updateViewModelFromPrefsMgr
{
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	Boolean					isDefault = false; // reused below
	
	
	// allow initialization of "isDefault..." values without triggers
	self.viewModel.defaultOverrideInProgress = YES;
	self.viewModel.disableWriteback = YES;
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagArrangeWindowsFullScreen;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.autoFullScreenEnabled = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultAutoFullScreen = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagArrangeWindowsUsingTabs;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.windowTabsEnabled = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultUseTabs = isDefault; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::WORKSPACE);
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsWorkspaceOptions_ActionHandling


/*!
Called by the UI when the user has made a change.

Currently this is called for any change to any setting so the
only way to respond is to copy all model data to the preferences
context.  If performance or other issues arise, it is possible
to expand the protocol to have (say) per-setting callbacks but
for now this is simpler and sufficient.

See also "updateViewModelFromPrefsMgr", which should be roughly
the inverse of this.

(2020.11)
*/
- (void)
dataUpdated
{
	Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
	
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagArrangeWindowsFullScreen;
		Boolean				preferenceValue = self.viewModel.autoFullScreenEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagArrangeWindowsUsingTabs;
		Boolean				preferenceValue = self.viewModel.windowTabsEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
}// dataUpdated


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetAutoFullScreen
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagArrangeWindowsFullScreen];
}// resetToDefaultGetAutoFullScreen


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetUseTabs
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagArrangeWindowsUsingTabs];
}// resetToDefaultGetUseTabs


@end //}


#pragma mark -
@implementation PrefPanelWorkspaces_OptionsVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelWorkspaces_OptionsActionHandler*	actionHandler = [[PrefPanelWorkspaces_OptionsActionHandler alloc] init];
	NSView*										newView = [UIPrefsWorkspaceOptions_ObjC makeView:actionHandler.viewModel];
	
	
	self = [super initWithView:newView delegate:self context:actionHandler/* transfer ownership (becomes "actionHandler" property in "panelViewManager:initializeWithContext:") */];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


#pragma mark Panel_Delegate


/*!
The first message ever sent, triggered by the call to the
superclass "initWithView:delegate:context:" in "init";
this functions as the rest of initialization and then
the definition of "self" and properties is complete.

Upon return, "self" will be defined and return to "init".

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext/* PrefPanelWorkspaces_OptionsActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelWorkspaces_OptionsActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelWorkspaces_OptionsActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	self.actionHandler = actionHandler; // transfer ownership
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self.logicalFirstResponder = self.view;
	self.logicalLastResponder = self.view;
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	*outEditType = kPanel_EditTypeInspector;
}// panelViewManager:requestingEditType:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
#pragma unused(aViewManager, aContainerView)
	// remember initial frame (it might be changed later)
	self.idealFrame = CGRectMake(0, 0, 460, 200); // somewhat arbitrary; see SwiftUI code/playground
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	*outIdealSize = self.idealFrame.size;
}


/*!
Responds to a request for contextual help in this panel.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager, sender)
	CFRetainRelease		keyPhrase(UIStrings_ReturnCopy(kUIStrings_HelpSystemTopicHelpWithPreferences),
									CFRetainRelease::kAlreadyRetained);
	
	
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpWithSearch(keyPhrase.returnCFStringRef());
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	// apply the specified settings
	[self.actionHandler.prefsMgr setCurrentContext:REINTERPRET_CAST(newDataSet, Preferences_ContextRef)];
	
	// update the view by changing the model’s observed variables
	[self.actionHandler updateViewModelFromPrefsMgr];
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).  Responds by saving preferences.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager, aContainerView)
	if (isAccepted)
	{
		Preferences_Result	prefsResult = Preferences_Save();
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "failed to save preferences!");
		}
	}
	else
	{
		// revert - UNIMPLEMENTED (not supported)
	}
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_ViewManager


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(2020.11)
*/
- (NSImage*)
panelIcon
{
	if (@available(macOS 11.0, *))
	{
		return [NSImage imageWithSystemSymbolName:@"rectangle.3.offgrid" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelWorkspaces"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Workspaces.Options";
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

(2020.11)
*/
- (NSString*)
panelName
{
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelWorkspaces", @"the name of this panel");
}// panelName


/*!
Returns information on which directions are most useful for
resizing the panel.  For instance a window container may
disallow vertical resizing if no panel in the window has
any reason to resize vertically.

IMPORTANT:	This is only a hint.  Panels must be prepared
			to resize in both directions.

(2020.11)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintHorizontal;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(2020.11)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::WORKSPACE;
}// preferencesClass


@end //} PrefPanelWorkspaces_OptionsVC


#pragma mark -
@implementation PrefPanelWorkspaces_SessionDescriptor //{


@synthesize commandType = _commandType;
@synthesize sessionFavoriteName = _sessionFavoriteName;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
	}
	return self;
}// init


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
description
{
	NSString*	result = nil;
	
	
	if (nil == self.commandType)
	{
		// use Session Favorite name
		result = self.sessionFavoriteName;
	}
	else
	{
		// use command type to form a description
		UInt32 const	commandType = STATIC_CAST([self.commandType unsignedIntegerValue], UInt32);
		
		
		switch (commandType)
		{
		case 0:
			result = NSLocalizedStringFromTable(@"None (No Window)",
												@"PrefPanelWorkspaces"/* table */,
												@"description for “no window”");
			break;
		
		case kSessionFactory_SpecialSessionDefaultFavorite:
			result = NSLocalizedStringFromTable(@"Default",
												@"PrefPanelWorkspaces"/* table */,
												@"description of special workspace session type: Default");
			break;
		
		case kSessionFactory_SpecialSessionLogInShell:
			result = NSLocalizedStringFromTable(@"Log-In Shell",
												@"PrefPanelWorkspaces"/* table */,
												@"description of special workspace session type: Log-In Shell");
			break;
		
		case kSessionFactory_SpecialSessionShell:
			result = NSLocalizedStringFromTable(@"Shell",
												@"PrefPanelWorkspaces"/* table */,
												@"description of special workspace session type: Shell");
			break;
		
		case kSessionFactory_SpecialSessionInteractiveSheet:
			result = NSLocalizedStringFromTable(@"Custom New Session",
												@"PrefPanelWorkspaces"/* table */,
												@"description of special workspace session type: Custom New Session");
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// description


@end //}


#pragma mark -
@implementation PrefPanelWorkspaces_WindowInfo //{


@synthesize preferencesIndex = _preferencesIndex;


#pragma mark Initializers


/*!
Designated initializer.

(2017.06)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		self->_preferencesIndex = 0;
	}
	return self;
}// init


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

The class argument must be Quills::Prefs::WORKSPACE.

(2017.06)
*/
- (instancetype)
initWithDefaultContextInClass:(Quills::Prefs::Class)	aClass
{
	assert(Quills::Prefs::WORKSPACE == aClass);
	self = [super initWithDefaultContextInClass:aClass];
	if (nil != self)
	{
		self->_preferencesIndex = 0;
	}
	return self;
}// initWithDefaultContextInClass:


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithIndex:(Preferences_Index)	anIndex
{
	self = [super init];
	if (nil != self)
	{
		assert(anIndex >= 1);
		self->_preferencesIndex = anIndex;
		_workspacePrefListener = nil; // installed by "setCurrentContext:"
	}
	return self;
}// initWithIndex:


/*!
Destructor.

(2020.04)
*/
- (void)
dealloc
{
	if (nullptr != _workspacePrefListener)
	{
		Preferences_Tag const	kThisCommandTypeTag = Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowCommandType, self.preferencesIndex);
		Preferences_Tag const	kThisSessionFavoriteTag = Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedWindowSessionFavorite, self.preferencesIndex);
		
		
		Preferences_ContextStopMonitoring(self.currentContext, [_workspacePrefListener listenerRef], kThisCommandTypeTag);
		Preferences_ContextStopMonitoring(self.currentContext, [_workspacePrefListener listenerRef], kThisSessionFavoriteTag);
	}
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
windowIndexLabel
{
	return [[NSNumber numberWithUnsignedInteger:self.preferencesIndex] stringValue];
}// windowIndexLabel


/*!
Accessor.

(4.1)
*/
- (NSString*)
windowName
{
	NSString*	result = @"";
	
	
	if (0 != self.preferencesIndex)
	{
		Preferences_Tag const	windowTitleIndexedTag = Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowTitle, self.preferencesIndex);
		CFStringRef				nameCFString = nullptr;
		Preferences_Result		prefsResult = Preferences_ContextGetData
												(self.currentContext, windowTitleIndexedTag,
													sizeof(nameCFString), &nameCFString,
													false/* search defaults */);
		
		
		if (kPreferences_ResultBadVersionDataNotAvailable == prefsResult)
		{
			// this is possible for indexed tags, as not all indexes
			// in the range may have data defined; ignore
			//Console_Warning(Console_WriteValue, "failed to retrieve window title preference, error", prefsResult);
		}
		else if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to retrieve window title preference, error", prefsResult);
		}
		else
		{
			result = BRIDGE_CAST(nameCFString, NSString*);
		}
	}
	
	return result;
}
- (void)
setWindowName:(NSString*)	aWindowName
{
	if (0 != self.preferencesIndex)
	{
		Preferences_Tag const	windowTitleIndexedTag = Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowTitle, self.preferencesIndex);
		CFStringRef				asCFStringRef = BRIDGE_CAST(aWindowName, CFStringRef);
		Preferences_Result		prefsResult = Preferences_ContextSetData
												(self.currentContext, windowTitleIndexedTag,
													sizeof(asCFStringRef), &asCFStringRef);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set window title preference, error", prefsResult);
		}
	}
}// setWindowName:


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored preference changes.  See the
initializer for the set of tags that is monitored.

While some preferences can be changed directly from
a binding, "numberedListItemIconImage" could be
affected by indirect preference changes so those are
monitored.

(2020.04)
*/
- (void)
model:(ListenerModel_Ref)				aModel
preferenceChange:(ListenerModel_Event)	anEvent
context:(void*)							aContext
{
#pragma unused(aModel)
	Preferences_Tag const	kThisCommandTypeTag = Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedWindowCommandType, self.preferencesIndex);
	Preferences_Tag const	kThisSessionFavoriteTag = Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowSessionFavorite, self.preferencesIndex);
	
	
	if ((kThisCommandTypeTag == anEvent) || (kThisSessionFavoriteTag == anEvent))
	{
		// respond to changes in these settings by refreshing the icon in the master list
		[self willChangeValueForKey:@"numberedListItemIconImage"];
		[self didChangeValueForKey:@"numberedListItemIconImage"];
	}
}// model:preferenceChange:context:


#pragma mark GenericPanelNumberedList_ItemBinding


/*!
Return strong reference to user interface string representing
numbered index in list.

Returns the "windowIndexLabel".

(4.1)
*/
- (NSString*)
numberedListIndexString
{
	return self.windowIndexLabel;
}// numberedListIndexString


/*!
Return or update user interface icon for item in list.

This is used to show the presence or absence of a window.

(2020.04)
*/
- (NSImage*)
numberedListItemIconImage
{
	NSString*	imageName = nil;
	NSString*	systemSymbolName = nil; // used for SF Symbols on macOS 11+
	NSString*	descriptionString = nil;
	NSImage*	result = nil;
	
	
	if (0 != self.preferencesIndex)
	{
		Preferences_Tag const	windowCommandTypeIndexedTag = Preferences_ReturnTagVariantForIndex
																(kPreferences_TagIndexedWindowCommandType, self.preferencesIndex);
		Preferences_Tag const	windowSessionIndexedTag = Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedWindowSessionFavorite, self.preferencesIndex);
		CFStringRef				sessionFavoriteNameCFString = nullptr;
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextGetData(self.currentContext, windowSessionIndexedTag,
													sizeof(sessionFavoriteNameCFString), &sessionFavoriteNameCFString,
													false/* search defaults */);
		if (kPreferences_ResultOK == prefsResult)
		{
			// session binding; use session icon
			systemSymbolName = @"macwindow";
			descriptionString = BRIDGE_CAST(sessionFavoriteNameCFString, NSString*);
			imageName = BRIDGE_CAST(AppResources_ReturnSessionStatusActiveIconFilenameNoExtension()/* arbitrary; TEMPORARY */, NSString*);
		}
		else
		{
			// no session binding; check for special command binding
			UInt32		sessionCommandID = 0;
			
			
			prefsResult = Preferences_ContextGetData(self.currentContext, windowCommandTypeIndexedTag,
														sizeof(sessionCommandID), &sessionCommandID,
														false/* search defaults */);
			if (kPreferences_ResultOK == prefsResult)
			{
				// command binding; use command icon
				switch (sessionCommandID)
				{
				case 0:
					// (special value for None)
					break;
				
				case kSessionFactory_SpecialSessionDefaultFavorite:
					systemSymbolName = @"heart";
					descriptionString = NSLocalizedStringFromTable(@"Default",
																	@"PrefPanelWorkspaces"/* table */,
																	@"description of special workspace session type: Default");
					imageName = BRIDGE_CAST(AppResources_ReturnNewSessionDefaultIconFilenameNoExtension(), NSString*);
					break;
				
				case kSessionFactory_SpecialSessionInteractiveSheet:
					systemSymbolName = @"text.and.command.macwindow";
					descriptionString = NSLocalizedStringFromTable(@"Custom New Session",
																	@"PrefPanelWorkspaces"/* table */,
																	@"description of special workspace session type: Custom New Session");
					imageName = BRIDGE_CAST(AppResources_ReturnPrefPanelWorkspacesIconFilenameNoExtension()/* arbitrary; TEMPORARY */, NSString*);
					break;
				
				case kSessionFactory_SpecialSessionLogInShell:
					systemSymbolName = @"terminal";
					descriptionString = NSLocalizedStringFromTable(@"Log-In Shell",
																	@"PrefPanelWorkspaces"/* table */,
																	@"description of special workspace session type: Log-In Shell");
					imageName = BRIDGE_CAST(AppResources_ReturnNewSessionLogInShellIconFilenameNoExtension(), NSString*);
					break;
				
				case kSessionFactory_SpecialSessionShell:
					systemSymbolName = @"terminal.fill";
					descriptionString = NSLocalizedStringFromTable(@"Shell",
																	@"PrefPanelWorkspaces"/* table */,
																	@"description of special workspace session type: Shell");
					imageName = BRIDGE_CAST(AppResources_ReturnNewSessionShellIconFilenameNoExtension(), NSString*);
					break;
				
				default:
					// ???
					break;
				}
			}
		}
	}
	
	if (nil != systemSymbolName)
	{
		if (@available(macOS 11.0, *))
		{
			result = [NSImage imageWithSystemSymbolName:systemSymbolName accessibilityDescription:descriptionString];
		}
	}
	
	if ((nil == result) && (nil != imageName))
	{
		result = [NSImage imageNamed:imageName];
	}
	
	return result;
}// numberedListItemIconImage


/*!
Return or update user interface string for name of item in list.

Accesses the "windowName".

(4.1)
*/
- (NSString*)
numberedListItemName
{
	return self.windowName;
}
- (void)
setNumberedListItemName:(NSString*)		aName
{
	self.windowName = aName;
}// setNumberedListItemName:


#pragma mark Preferences_ContextManagerObject


/*!
Detects changes to the context and also installs monitors.

(2020.04)
*/
- (void)
setCurrentContext:(Preferences_ContextRef)	aNewContext
{
	Preferences_Tag const	kThisCommandTypeTag = Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedWindowCommandType, self.preferencesIndex);
	Preferences_Tag const	kThisSessionFavoriteTag = Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowSessionFavorite, self.preferencesIndex);
	
	
	// stop monitoring the previous preferences collection
	// (see also "dealloc")
	if ((aNewContext != self.currentContext) && (nil != self.currentContext))
	{
		Preferences_ContextStopMonitoring(self.currentContext, [self->_workspacePrefListener listenerRef], kThisCommandTypeTag);
		Preferences_ContextStopMonitoring(self.currentContext, [self->_workspacePrefListener listenerRef], kThisSessionFavoriteTag);
	}
	
	[super setCurrentContext:aNewContext];
	
	// monitor certain settings that might change without using this UI
	// (but that might affect what is displayed)
	if (nil == self->_workspacePrefListener)
	{
		self->_workspacePrefListener = [[ListenerModel_StandardListener alloc]
										initWithTarget:self
														eventFiredSelector:@selector(model:preferenceChange:context:)];
	}
	Preferences_ContextStartMonitoring(aNewContext, [self->_workspacePrefListener listenerRef], kThisCommandTypeTag);
	Preferences_ContextStartMonitoring(aNewContext, [self->_workspacePrefListener listenerRef], kThisSessionFavoriteTag);
}// setCurrentContext:


@end //}


#pragma mark -
@implementation PrefPanelWorkspaces_WindowsViewManager //{


@synthesize windowEditorViewManager = _windowEditorViewManager;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	PrefPanelWorkspaces_WindowEditorViewManager*	newViewManager = [[PrefPanelWorkspaces_WindowEditorViewManager alloc] init];
	NSString*										panelName = NSLocalizedStringFromTable(@"Windows", @"PrefPanelWorkspaces",
																							@"the name of this panel");
	NSImage*										panelIcon = nil;
	
	
	if (@available(macOS 11.0, *))
	{
		panelIcon = [NSImage imageWithSystemSymbolName:@"rectangle.3.offgrid" accessibilityDescription:self.panelName];
	}
	else
	{
		panelIcon = [NSImage imageNamed:@"IconForPrefPanelWorkspaces"];
	}
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Workspaces.Windows"
										localizedName:panelName
										localizedIcon:panelIcon
										master:self detailViewManager:newViewManager];
	if (nil != self)
	{
		_windowEditorViewManager = newViewManager;
	}
	return self;
}// init


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowInfo*)
selectedWindowInfo
{
	PrefPanelWorkspaces_WindowInfo*		result = nil;
	NSUInteger							currentIndex = [self.listItemBindingIndexes firstIndex];
	
	
	if (NSNotFound != currentIndex)
	{
		result = [self.listItemBindings objectAtIndex:currentIndex];
	}
	
	return result;
}// selectedWindowInfo


#pragma mark GeneralPanelNumberedList_Master


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
initializeNumberedListViewManager:(GenericPanelNumberedList_ViewManager*)	aViewManager
{
	NSArray*			listData =
						@[
							// create as many managers as there are supported indexes
							// (see the Preferences module)
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:1],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:2],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:3],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:4],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:5],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:6],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:7],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:8],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:9],
							[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:10],
						];
	
	
	aViewManager.listItemBindings = listData;
}// initializeNumberedListViewManager:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
containerViewDidLoadForNumberedListViewManager:(GenericPanelNumberedList_ViewManager*)	aViewManager
{
	// customize numbered-list interface
	aViewManager.headingTitleForIconColumn = NSLocalizedStringFromTable(@"Type", @"PrefPanelWorkspaces", @"the title for the item icon column");
	aViewManager.headingTitleForNameColumn = NSLocalizedStringFromTable(@"Window Name", @"PrefPanelWorkspaces", @"the title for the item name column");
}// containerViewDidLoadForNumberedListViewManager:


/*!
Responds to a change in data set by updating the context that
is associated with each item in the list.

Note that this is also called when the selected index changes
and the context has not changed (context may be nullptr), and
this variant is ignored.

(4.1)
*/
- (void)
numberedListViewManager:(GenericPanelNumberedList_ViewManager*)		aViewManager
didChangeFromDataSet:(GenericPanelNumberedList_DataSet*)			oldStructPtr
toDataSet:(GenericPanelNumberedList_DataSet*)						newStructPtr
{
#pragma unused(aViewManager, oldStructPtr)
	
	if (nullptr != newStructPtr)
	{
		Preferences_ContextRef		asContext = REINTERPRET_CAST(newStructPtr->parentPanelDataSetOrNull, Preferences_ContextRef);
		
		
		// if the context is nullptr, only the index has changed;
		// since the index is not important here, it is ignored
		if (nullptr != asContext)
		{
			// update each object in the list to use the new context
			// (so that, for instance, the right names are displayed)
			for (PrefPanelWorkspaces_WindowInfo* eachInfo in aViewManager.listItemBindings)
			{
				[eachInfo setCurrentContext:asContext];
			}
		}
	}
}// numberedListViewManager:didChangeFromDataSet:toDataSet:


@end //} PrefPanelWorkspaces_WindowsViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_WindowBoundariesValue //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
index:(Preferences_Index)								anIndex
{
	assert(anIndex > 0);
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		self->frameObject = [[PreferenceValue_Rect alloc]
								initWithPreferencesTag:Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowFrameBounds, anIndex)
														contextManager:aContextMgr
														preferenceRectType:kPreferenceValue_RectTypeHIRect];
		self->screenBoundsObject = [[PreferenceValue_Rect alloc]
									initWithPreferencesTag:Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedWindowScreenBounds, anIndex)
															contextManager:aContextMgr
															preferenceRectType:kPreferenceValue_RectTypeHIRect];
		
		// monitor the preferences context manager so that observers
		// of preferences in sub-objects can be told to expect changes
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextWillChangeNotification
							performSelector:@selector(prefsContextWillChange:)];
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextDidChangeNotification
							performSelector:@selector(prefsContextDidChange:)];
	}
	return self;
}// initWithContextManager:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (Preferences_Index)
preferencesIndex
{
	return _preferencesIndex;
}
- (void)
setPreferencesIndex:(Preferences_Index)		anIndex
{
	if (_preferencesIndex != anIndex)
	{
		// technically the context may not have changed (only the index)
		// but the response is exactly the same, as settings may be updated
		[self prefsContextWillChange:nil];
		_preferencesIndex = anIndex;
		self->frameObject.preferencesTag = Preferences_ReturnTagVariantForIndex
											(kPreferences_TagIndexedWindowFrameBounds, anIndex);
		self->screenBoundsObject.preferencesTag = Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedWindowScreenBounds, anIndex);
		[self prefsContextDidChange:nil];
	}
}// setPreferencesIndex:


#pragma mark New Methods


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextDidChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextWillChange:"
	[self didSetPreferenceValue];
}// prefsContextDidChange:


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextWillChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextDidChange:"
	[self willSetPreferenceValue];
}// prefsContextWillChange:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = ([self->frameObject isInherited] && [self->screenBoundsObject isInherited]);
	
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self willSetPreferenceValue];
	[self->frameObject setNilPreferenceValue];
	[self->screenBoundsObject setNilPreferenceValue];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end //} PrefPanelWorkspaces_WindowBoundariesValue


#pragma mark -
@implementation PrefPanelWorkspaces_WindowSessionValue //{


@synthesize preferencesIndex = _preferencesIndex;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
index:(Preferences_Index)								anIndex
{
	assert(anIndex > 0);
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		self->commandTypeObject = [[PreferenceValue_Number alloc]
									initWithPreferencesTag:Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedWindowCommandType, anIndex)
															contextManager:aContextMgr
															preferenceCType:kPreferenceValue_CTypeUInt32];
		self->sessionObject = [[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowSessionFavorite, anIndex)
														contextManager:aContextMgr
														sourceClass:Quills::Prefs::SESSION
														includeDefault:YES
														didRebuildTarget:self
														didRebuildSelector:@selector(rebuildSessionList)];
		self->_descriptorArray = [[NSMutableArray alloc] init];
		[self rebuildSessionList];
		
		// monitor the preferences context manager so that observers
		// of preferences in sub-objects can be told to expect changes
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextWillChangeNotification
							performSelector:@selector(prefsContextWillChange:)];
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextDidChangeNotification
							performSelector:@selector(prefsContextDidChange:)];
	}
	return self;
}// initWithContextManager:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

This is read-only and it changes whenever the value of
the property "currentValueDescriptor" changes.

(4.1)
*/
- (BOOL)
hasValue
{
	id			sessionDescriptor = self.currentValueDescriptor.description;
	NSNumber*	commandType = self.currentValueDescriptor.commandType;
	
	
	// the None setting is a special command type of zero (0)
	// so any other value is considered a valid window setting;
	// IMPORTANT: this logic should be consistent with other
	// interpretations of the two preference settings in this
	// class and in user interface bindings
	return (((nil == commandType) && (nil != sessionDescriptor)) ||
			(0 != [commandType unsignedIntegerValue]));
}// hasValue


/*!
Accessor.

(4.1)
*/
- (NSArray*)
valueDescriptorArray
{
	return _descriptorArray;
}// valueDescriptorArray


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_SessionDescriptor*)
currentValueDescriptor
{
	BOOL		isSessionFavoriteUndefined = NO;
	BOOL		isCommandTypeUndefined = NO;
	NSString*	sessionFavoriteName = [self->sessionObject readValueSeeIfDefault:&isSessionFavoriteUndefined];
	NSNumber*	commandType = [self->commandTypeObject readValueSeeIfDefault:&isCommandTypeUndefined];
	PrefPanelWorkspaces_SessionDescriptor*	result = nil;
	
	
	// normally a value will always be returned above and
	// the only way to detect an undefined value is with
	// the “is Default” flag; although, since the source
	// context might BE the Default, the flags might be
	// true and then it is necessary to check the values
	if (Preferences_ContextIsDefault(self->sessionObject.prefsMgr.currentContext, Quills::Prefs::WORKSPACE))
	{
		isSessionFavoriteUndefined = (nil == sessionFavoriteName);
		isCommandTypeUndefined = (nil == commandType);
	}
	
	// IMPORTANT: the historical (Carbon) implementation
	// originally checked the session favorite setting
	// before the command type, and ignored the latter
	// if the former was defined; in case the user has
	// old preferences saved, the Cocoa version uses the
	// same fallback logic
 	if ((NO == isSessionFavoriteUndefined) &&
		(NO == [sessionFavoriteName isEqualToString:@""]))
	{
		// selection is one of the Session Favorite names
		for (PrefPanelWorkspaces_SessionDescriptor* asDesc in self.valueDescriptorArray)
		{
			if ([asDesc.sessionFavoriteName isEqualToString:sessionFavoriteName])
			{
				result = asDesc;
				break;
			}
		}
	}
	else if (NO == isCommandTypeUndefined)
	{
		// selection is one of the special session types
		for (PrefPanelWorkspaces_SessionDescriptor* asDesc in self.valueDescriptorArray)
		{
			if ([asDesc.commandType isEqual:commandType])
			{
				result = asDesc;
				break;
			}
		}
	}
	else
	{
		// in case no preference was ever saved, return None
		//Console_Warning(Console_WriteLine, "unable to find any valid window session setting (built-in command type or associated Session Favorite)");
		result = [self.valueDescriptorArray objectAtIndex:0];
	}
	
	return result;
}
- (void)
setCurrentValueDescriptor:(PrefPanelWorkspaces_SessionDescriptor*)	selectedObject
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	[self willChangeValueForKey:@"hasValue"];
	
	if (nil == selectedObject)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		if (nil == selectedObject.commandType)
		{
			// selection is one of the Session Favorite names
			[self->commandTypeObject setNilPreferenceValue];
			self->sessionObject.currentValueDescriptor = [[PreferenceValue_StringDescriptor alloc]
															initWithStringValue:selectedObject.sessionFavoriteName
																				description:selectedObject.sessionFavoriteName];
		}
		else
		{
			// selection is one of the special session types
			[self->sessionObject setNilPreferenceValue];
			self->commandTypeObject.numberValue = selectedObject.commandType;
		}
	}
	
	[self didChangeValueForKey:@"hasValue"];
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didSetPreferenceValue];
}// setCurrentValueDescriptor:


/*!
Accessor.

(4.1)
*/
- (Preferences_Index)
preferencesIndex
{
	return _preferencesIndex;
}
- (void)
setPreferencesIndex:(Preferences_Index)		anIndex
{
	if (_preferencesIndex != anIndex)
	{
		// technically the context may not have changed (only the index)
		// but the response is exactly the same, as settings may be updated
		[self prefsContextWillChange:nil];
		_preferencesIndex = anIndex;
		self->commandTypeObject.preferencesTag = Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedWindowCommandType, anIndex);
		self->sessionObject.preferencesTag = Preferences_ReturnTagVariantForIndex
												(kPreferences_TagIndexedWindowSessionFavorite, anIndex);
		[self prefsContextDidChange:nil];
	}
}// setPreferencesIndex:


#pragma mark New Methods


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextDidChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextWillChange:"
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didChangeValueForKey:@"hasValue"];
	[self didSetPreferenceValue];
}// prefsContextDidChange:


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextWillChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextDidChange:"
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"hasValue"];
	[self willChangeValueForKey:@"currentValueDescriptor"];
}// prefsContextWillChange:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES;
	// since a fallback occurs between preference values, this must be a bit more
	// complicated than usual
	BOOL		isSessionFavoriteUndefined = NO;
	BOOL		isCommandTypeUndefined = NO;
	BOOL		result = NO;
	NSString*	sessionName = [self->sessionObject readValueSeeIfDefault:&isSessionFavoriteUndefined];
	
	
	UNUSED_RETURN(NSNumber*)[self->commandTypeObject readValueSeeIfDefault:&isCommandTypeUndefined];
	
	if ((isSessionFavoriteUndefined) && (isCommandTypeUndefined))
	{
		result = YES;
	}
	else if ((NO == isSessionFavoriteUndefined) &&
				(NO == [sessionName isEqualToString:@""]))
	{
		result = [self->sessionObject isInherited];
	}
	else if (NO == isCommandTypeUndefined)
	{
		result = [self->commandTypeObject isInherited];
	}
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	[self willChangeValueForKey:@"hasValue"];
	[self->commandTypeObject setNilPreferenceValue];
	[self->sessionObject setNilPreferenceValue];
	[self didChangeValueForKey:@"hasValue"];
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end //} PrefPanelWorkspaces_WindowSessionValue


#pragma mark -
@implementation PrefPanelWorkspaces_WindowSessionValue (PrefPanelWorkspaces_WindowSessionValueInternal) //{


#pragma mark New Methods


/*!
Invoked when the "sessionObject" rebuilds its list of sessions.
This responds by updating the corresponding array of descriptors
(which includes those sessions and some other items such as
standard session types).

(4.1)
*/
- (void)
rebuildSessionList
{
	[_descriptorArray removeAllObjects];
	
	PrefPanelWorkspaces_SessionDescriptor*		newDesc = nil;
	
	
	// add an item that means “no session in this window slot”
	// (should be the first item)
	newDesc = [[PrefPanelWorkspaces_SessionDescriptor alloc] init];
	newDesc.commandType = [NSNumber numberWithInteger:0];
	newDesc.sessionFavoriteName = nil;
	[_descriptorArray addObject:newDesc];
	
	// add items for each user Session Favorite (including Default)
	for (PreferenceValue_StringDescriptor* sessionDesc in [self->sessionObject valueDescriptorArray])
	{
		newDesc = [[PrefPanelWorkspaces_SessionDescriptor alloc] init];
		newDesc.commandType = nil;
		newDesc.sessionFavoriteName = [sessionDesc describedStringValue];
		[_descriptorArray addObject:newDesc];
	}
	
	// add an item that means “open the Log-In Shell session in the window”
	newDesc = [[PrefPanelWorkspaces_SessionDescriptor alloc] init];
	newDesc.commandType = [NSNumber numberWithInteger:kSessionFactory_SpecialSessionLogInShell];
	newDesc.sessionFavoriteName = nil;
	[_descriptorArray addObject:newDesc];
	
	// add an item that means “open the (non-log-in) Shell session in the window”
	newDesc = [[PrefPanelWorkspaces_SessionDescriptor alloc] init];
	newDesc.commandType = [NSNumber numberWithInteger:kSessionFactory_SpecialSessionShell];
	newDesc.sessionFavoriteName = nil;
	[_descriptorArray addObject:newDesc];
	
	// add an item that means “open the Custom New Session sheet when the window opens”
	newDesc = [[PrefPanelWorkspaces_SessionDescriptor alloc] init];
	newDesc.commandType = [NSNumber numberWithInteger:kSessionFactory_SpecialSessionInteractiveSheet];
	newDesc.sessionFavoriteName = nil;
	[_descriptorArray addObject:newDesc];
}// rebuildSessionList


@end //} PrefPanelWorkspaces_WindowSessionValue (PrefPanelWorkspaces_WindowSessionValueInternal)


#pragma mark -
@implementation PrefPanelWorkspaces_WindowEditorViewManager //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelWorkspaceWindowsCocoa" delegate:self context:nil];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


#pragma mark Actions


/*!
Displays a panel to edit the location of the window.

(4.1)
*/
- (IBAction)
performSetBoundary:(id)		sender
{
#pragma unused(sender)
	Preferences_ContextRef const	currentContext = [self.windowsViewManager.selectedWindowInfo currentContext];
	
	
	if (nullptr == currentContext)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "attempt to perform set-boundary with invalid context");
	}
	else
	{
		Preferences_Index const		currentIndex = self.windowsViewManager.selectedWindowInfo.preferencesIndex;
		__weak decltype(self)		weakSelf = self;
		
		
		Keypads_SetArrangeWindowPanelBinding(^{
													// this is invoked when the user is done setting a value;
													// trigger updates to inheritance checkbox, etc.
													[weakSelf.windowBoundaries willSetPreferenceValue];
													[weakSelf.windowSession willSetPreferenceValue];
													[weakSelf.windowSession didSetPreferenceValue];
													[weakSelf.windowBoundaries didSetPreferenceValue];
													//weakSelf.windowSession.currentValueDescriptor = weakSelf.windowSession.currentValueDescriptor;
												},
												Preferences_ReturnTagVariantForIndex
												(kPreferences_TagIndexedWindowFrameBounds, currentIndex),
												kPreferences_DataTypeHIRect,
												Preferences_ReturnTagVariantForIndex
												(kPreferences_TagIndexedWindowScreenBounds, currentIndex),
												kPreferences_DataTypeHIRect,
												currentContext);
		Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, true);
	}
}// performSetBoundary:


/*!
Responds when the user selects the “None (No Window)” item
in the window session pop-up menu.  This sets the
corresponding preferences appropriately.

(4.1)
*/
- (IBAction)
performSetSessionToNone:(id)	sender
{
#pragma unused(sender)
	PrefPanelWorkspaces_SessionDescriptor*		noneDesc = nil;
	
	
	// the value "0" is assumed throughout (and in the
	// Preferences module when processing the value)
	// as a way to indicate an unused window slot
	noneDesc = [[PrefPanelWorkspaces_SessionDescriptor alloc] init];
	noneDesc.commandType = [NSNumber numberWithInteger:0];
	noneDesc.sessionFavoriteName = nil;
	self.windowSession.currentValueDescriptor = noneDesc;
}// performSetSessionToNone:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowBoundariesValue*)
windowBoundaries
{
	return [self->byKey objectForKey:@"windowBoundaries"];
}// windowBoundaries


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowSessionValue*)
windowSession
{
	return [self->byKey objectForKey:@"windowSession"];
}// windowSession


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowsViewManager*)
windowsViewManager
{
	id< Panel_Parent >		parentViewManager = self.panelParent;
	assert(nil != parentViewManager);
	assert([parentViewManager isKindOfClass:PrefPanelWorkspaces_WindowsViewManager.class]);
	PrefPanelWorkspaces_WindowsViewManager*		result = STATIC_CAST(parentViewManager, PrefPanelWorkspaces_WindowsViewManager*);
	
	
	return result;
}// windowsViewManager


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext
{
#pragma unused(aViewManager, aContext)
	self->prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:5/* arbitrary; number of settings */];
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	*outEditType = kPanel_EditTypeInspector;
}// panelViewManager:requestingEditType:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
#pragma unused(aViewManager, aContainerView)
	assert(nil != byKey);
	assert(nil != prefsMgr);
	
	// remember frame from XIB (it might be changed later)
	self->idealSize = [aContainerView frame].size;
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	// NOTE: These can be changed via callback to a different combination
	// of context and index.
	[self configureForIndex:1];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
	}
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	*outIdealSize = self->idealSize;
}


/*!
Responds to a request for contextual help in this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager, sender)
	CFRetainRelease		keyPhrase(UIStrings_ReturnCopy(kUIStrings_HelpSystemTopicHelpWithPreferences),
									CFRetainRelease::kAlreadyRetained);
	
	
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpWithSearch(keyPhrase.returnCFStringRef());
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the data set currently selected by the controlling
parent (numbered list); the assumption is that this will
always be in sync with the data set actually given.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	//GenericPanelNumberedList_DataSet*	oldStructPtr = REINTERPRET_CAST(oldDataSet, GenericPanelNumberedList_DataSet*);
	GenericPanelNumberedList_DataSet*	newStructPtr = REINTERPRET_CAST(newDataSet, GenericPanelNumberedList_DataSet*);
	Preferences_ContextRef				asPrefsContext = REINTERPRET_CAST(newStructPtr->parentPanelDataSetOrNull, Preferences_ContextRef);
	Preferences_Index					asIndex = STATIC_CAST(1 + newStructPtr->selectedDataArrayIndex, Preferences_Index); // index is one-based
	
	
	if (self.isPanelUserInterfaceLoaded)
	{
		// note that all current values will change
		for (NSString* keyName in [self primaryDisplayBindingKeys])
		{
			[self willChangeValueForKey:keyName];
		}
		
		// now apply the specified settings
		if (nullptr != asPrefsContext)
		{
			// user has selected an entirely different preferences collection
			self.windowBoundaries.prefsMgr.currentContext = asPrefsContext;
			self.windowSession.prefsMgr.currentContext = asPrefsContext;
		}
		// user may or may not have selected a different window
		// (the response is currently the same either way)
		[self configureForIndex:asIndex];
		
		// note that all values have changed (causes the display to be refreshed)
		for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
		{
			[self didChangeValueForKey:keyName];
		}
	}
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).  Responds by saving preferences.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager, aContainerView)
	if (isAccepted)
	{
		Preferences_Result	prefsResult = Preferences_Save();
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "failed to save preferences!");
		}
	}
	else
	{
		// revert - UNIMPLEMENTED (not supported)
	}
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_ViewManager


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	if (@available(macOS 11.0, *))
	{
		return [NSImage imageWithSystemSymbolName:@"rectangle.3.offgrid" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelWorkspaces"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Workspaces.Windows.EditWindow";
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

(4.1)
*/
- (NSString*)
panelName
{
	return NSLocalizedStringFromTable(@"Edit Window", @"PrefPanelWorkspaces", @"the name of this panel");
}// panelName


/*!
Returns information on which directions are most useful for
resizing the panel.  For instance a window container may
disallow vertical resizing if no panel in the window has
any reason to resize vertically.

IMPORTANT:	This is only a hint.  Panels must be prepared
			to resize in both directions.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintHorizontal;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::WORKSPACE;
}// preferencesClass


@end //} PrefPanelWorkspaces_WindowEditorViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_WindowEditorViewManager (PrefPanelWorkspaces_WindowEditorViewManagerInternal) //{


#pragma mark New Methods


/*!
Creates and/or configures index-dependent internal objects used
for value bindings.

NOTE:	This does NOT trigger any will/did callbacks because it
		is typically called in places where these are already
		being called.  Callbacks are necessary to ensure that
		user interface elements are updated, for instance.

(4.1)
*/
- (void)
configureForIndex:(Preferences_Index)	anIndex
{
	if (nil == self.windowBoundaries)
	{
		[self->byKey setObject:[[PrefPanelWorkspaces_WindowBoundariesValue alloc]
									initWithContextManager:self->prefsMgr index:anIndex]
						forKey:@"windowBoundaries"];
	}
	else
	{
		self.windowBoundaries.preferencesIndex = anIndex;
	}
	
	if (nil == self.windowSession)
	{
		[self->byKey setObject:[[PrefPanelWorkspaces_WindowSessionValue alloc]
									initWithContextManager:self->prefsMgr index:anIndex]
						forKey:@"windowSession"];
	}
	else
	{
		self.windowSession.preferencesIndex = anIndex;
	}
}// configureForIndex:


#pragma mark Accessors


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"windowBoundaries", @"windowSession"];
}// primaryDisplayBindingKeys


@end //} PrefPanelWorkspaces_WindowEditorViewManager (PrefPanelWorkspaces_WindowEditorViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
