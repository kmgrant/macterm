/*!	\file PrefPanelWorkspaces.mm
	\brief Implements the Workspaces panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <Localization.h>
#import <ListenerModel.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "Panel.h"
#import "Preferences.h"
#import "UIStrings.h"



#pragma mark Types

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
The private class interface.
*/
@interface PrefPanelWorkspaces_OptionsViewManager (PrefPanelWorkspaces_OptionsViewManagerInternal) //{

// accessors
	@property (readonly) NSArray*
	primaryDisplayBindingKeys;

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
	- (void)
	didFinishSettingWindowBoundary;

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
					[[[PrefPanelWorkspaces_OptionsViewManager alloc] init] autorelease],
					[[[PrefPanelWorkspaces_WindowsViewManager alloc] init] autorelease],
				];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Workspaces"
										localizedName:NSLocalizedStringFromTable(@"Workspaces", @"PrefPanelWorkspaces",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelWorkspaces"]
										viewManagerArray:subViewManagers];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


@end //} PrefPanelWorkspaces_ViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_OptionsViewManager //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelWorkspaceOptionsCocoa" delegate:self context:nullptr];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
automaticallyEnterFullScreen
{
	return [self->byKey objectForKey:@"automaticallyEnterFullScreen"];
}// automaticallyEnterFullScreen


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
useTabbedWindows
{
	return [self->byKey objectForKey:@"useTabbedWindows"];
}// useTabbedWindows


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(void*)			aContext
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
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagArrangeWindowsFullScreen
														contextManager:self->prefsMgr] autorelease]
					forKey:@"automaticallyEnterFullScreen"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagArrangeWindowsUsingTabs
														contextManager:self->prefsMgr] autorelease]
					forKey:@"useTabbedWindows"];
	
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
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
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
display the new data set.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// now apply the specified settings
	[self->prefsMgr setCurrentContext:REINTERPRET_CAST(newDataSet, Preferences_ContextRef)];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
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
	return @"net.macterm.prefpanels.Workspaces.Options";
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
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelWorkspaces", @"the name of this panel");
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


@end //} PrefPanelWorkspaces_OptionsViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_OptionsViewManager (PrefPanelWorkspaces_OptionsViewManagerInternal) //{


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"automaticallyEnterFullScreen", @"useTabbedWindows"];
}// primaryDisplayBindingKeys


@end //} PrefPanelWorkspaces_OptionsViewManager (PrefPanelWorkspaces_OptionsViewManagerInternal)


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


/*!
Destructor.

(2018.08)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


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
		result = [[self.sessionFavoriteName retain] autorelease];
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
		
		case kCommandNewSessionDefaultFavorite:
			result = NSLocalizedStringFromTable(@"Default",
												@"PrefPanelWorkspaces"/* table */,
												@"description of special workspace session type: Default");
			break;
		
		case kCommandNewSessionLoginShell:
			result = NSLocalizedStringFromTable(@"Log-In Shell",
												@"PrefPanelWorkspaces"/* table */,
												@"description of special workspace session type: Log-In Shell");
			break;
		
		case kCommandNewSessionShell:
			result = NSLocalizedStringFromTable(@"Shell",
												@"PrefPanelWorkspaces"/* table */,
												@"description of special workspace session type: Shell");
			break;
		
		case kCommandNewSessionDialog:
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
	}
	return self;
}// initWithIndex:


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
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Workspaces.Windows"
										localizedName:NSLocalizedStringFromTable(@"Windows", @"PrefPanelWorkspaces",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelWorkspaces"]
										master:self detailViewManager:newViewManager];
	if (nil != self)
	{
		_windowEditorViewManager = newViewManager;
	}
	return self;
}// init


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[_windowEditorViewManager release];
	[super dealloc];
}// dealloc


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
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:1] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:2] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:3] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:4] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:5] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:6] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:7] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:8] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:9] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] initWithIndex:10] autorelease],
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
	[frameObject release];
	[screenBoundsObject release];
	[super dealloc];
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
	[_descriptorArray release];
	[super dealloc];
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
	return [[_descriptorArray retain] autorelease];
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
			self->sessionObject.currentValueDescriptor = [[[PreferenceValue_StringDescriptor alloc]
															initWithStringValue:selectedObject.sessionFavoriteName
																				description:selectedObject.sessionFavoriteName]
															autorelease];
		}
		else
		{
			// selection is one of the special session types
			self->commandTypeObject.numberValue = selectedObject.commandType;
			[self->sessionObject setNilPreferenceValue];
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
	newDesc = [[[PrefPanelWorkspaces_SessionDescriptor alloc] init] autorelease];
	newDesc.commandType = [NSNumber numberWithInteger:0];
	newDesc.sessionFavoriteName = nil;
	[_descriptorArray addObject:newDesc];
	
	// add items for each user Session Favorite (including Default)
	for (PreferenceValue_StringDescriptor* sessionDesc in [self->sessionObject valueDescriptorArray])
	{
		newDesc = [[[PrefPanelWorkspaces_SessionDescriptor alloc] init] autorelease];
		newDesc.commandType = nil;
		newDesc.sessionFavoriteName = [sessionDesc describedStringValue];
		[_descriptorArray addObject:newDesc];
	}
	
	// add an item that means “open the Log-In Shell session in the window”
	newDesc = [[[PrefPanelWorkspaces_SessionDescriptor alloc] init] autorelease];
	newDesc.commandType = [NSNumber numberWithInteger:kCommandNewSessionLoginShell];
	newDesc.sessionFavoriteName = nil;
	[_descriptorArray addObject:newDesc];
	
	// add an item that means “open the (non-log-in) Shell session in the window”
	newDesc = [[[PrefPanelWorkspaces_SessionDescriptor alloc] init] autorelease];
	newDesc.commandType = [NSNumber numberWithInteger:kCommandNewSessionShell];
	newDesc.sessionFavoriteName = nil;
	[_descriptorArray addObject:newDesc];
	
	// add an item that means “open the Custom New Session sheet when the window opens”
	newDesc = [[[PrefPanelWorkspaces_SessionDescriptor alloc] init] autorelease];
	newDesc.commandType = [NSNumber numberWithInteger:kCommandNewSessionDialog];
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


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


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
		
		
		Keypads_SetArrangeWindowPanelBinding(self, @selector(didFinishSettingWindowBoundary),
												Preferences_ReturnTagVariantForIndex
												(kPreferences_TagIndexedWindowFrameBounds, currentIndex),
												typeHIRect,
												Preferences_ReturnTagVariantForIndex
												(kPreferences_TagIndexedWindowScreenBounds, currentIndex),
												typeHIRect,
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
	noneDesc = [[[PrefPanelWorkspaces_SessionDescriptor alloc] init] autorelease];
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
initializeWithContext:(void*)			aContext
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
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
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
		[self->byKey setObject:[[[PrefPanelWorkspaces_WindowBoundariesValue alloc]
									initWithContextManager:self->prefsMgr index:anIndex] autorelease]
						forKey:@"windowBoundaries"];
	}
	else
	{
		self.windowBoundaries.preferencesIndex = anIndex;
	}
	
	if (nil == self.windowSession)
	{
		[self->byKey setObject:[[[PrefPanelWorkspaces_WindowSessionValue alloc]
									initWithContextManager:self->prefsMgr index:anIndex] autorelease]
						forKey:@"windowSession"];
	}
	else
	{
		self.windowSession.preferencesIndex = anIndex;
	}
}// configureForIndex:


/*!
This is invoked when the user indicates that he or she is
finished setting the target window boundary.

(4.1)
*/
- (void)
didFinishSettingWindowBoundary
{
	// trigger updates to inheritance checkbox, etc.
	[self.windowBoundaries willSetPreferenceValue];
	[self.windowSession willSetPreferenceValue];
	[self.windowSession didSetPreferenceValue];
	[self.windowBoundaries didSetPreferenceValue];
	//self.windowSession.currentValueDescriptor = self.windowSession.currentValueDescriptor;
}// didFinishSettingWindowBoundary


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
