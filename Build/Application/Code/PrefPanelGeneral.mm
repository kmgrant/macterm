/*!	\file PrefPanelGeneral.mm
	\brief Implements the General panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
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

#import "PrefPanelGeneral.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Unix includes
#import <strings.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <BoundName.objc++.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <HelpSystem.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "Keypads.h"
#import "Panel.h"
#import "Preferences.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "UIStrings.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>



#pragma mark Types

/*!
Implements SwiftUI interaction for the “Full Screen” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelGeneral_FullScreenActionHandler : NSObject< UIPrefsGeneralFullScreen_ActionHandling > //{
{
@private
	PrefsContextManager_Object*			_prefsMgr;
	UIPrefsGeneralFullScreen_Model*		_viewModel;
}

// new methods
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsGeneralFullScreen_Model*
	viewModel;

@end //}


/*!
Implements SwiftUI interaction for the “Notifications” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelGeneral_NotificationsActionHandler : NSObject< UIPrefsGeneralNotifications_ActionHandling > //{
{
@private
	PrefsContextManager_Object*				_prefsMgr;
	UIPrefsGeneralNotifications_Model*		_viewModel;
}

// new methods
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsGeneralNotifications_Model*
	viewModel;

@end //}


/*!
Implements SwiftUI interaction for the “general options” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelGeneral_OptionsActionHandler : NSObject< UIPrefsGeneralOptions_ActionHandling > //{
{
@private
	PrefsContextManager_Object*		_prefsMgr;
	UIPrefsGeneralOptions_Model*	_viewModel;
}

// new methods
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsGeneralOptions_Model*
	viewModel;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelGeneral_SpecialViewManager (PrefPanelGeneral_SpecialViewManagerInternal) //{

// new methods
	- (void)
	notifyDidChangeValueForCursorShape;
	- (void)
	notifyWillChangeValueForCursorShape;
	- (void)
	notifyDidChangeValueForNewCommand;
	- (void)
	notifyWillChangeValueForNewCommand;
	- (void)
	notifyDidChangeValueForWindowResizeEffect;
	- (void)
	notifyWillChangeValueForWindowResizeEffect;
	- (NSArray*)
	primaryDisplayBindingKeys;

// preference setting accessors
	- (Terminal_CursorType)
	readCursorTypeWithDefaultValue:(Terminal_CursorType)_;
	- (UInt32)
	readNewCommandShortcutEffectWithDefaultValue:(UInt32)_;
	- (UInt16)
	readSpacesPerTabWithDefaultValue:(UInt16)_;
	- (BOOL)
	writeCursorType:(Terminal_CursorType)_;
	- (BOOL)
	writeNewCommandShortcutEffect:(UInt32)_;
	- (BOOL)
	writeSpacesPerTab:(UInt16)_;

@end //}



#pragma mark Public Methods

/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using the Full Screen user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(2018.05)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewFullScreenTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagKioskShowsMenuBar);
	tagList.push_back(kPreferences_TagKioskShowsScrollBar);
	tagList.push_back(kPreferences_TagKioskShowsWindowFrame);
	tagList.push_back(kPreferences_TagKioskAllowsForceQuit);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewFullScreenTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using the Notifications user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewNotificationsTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagVisualBell);
	tagList.push_back(kPreferences_TagNotifyOfBeeps);
	tagList.push_back(kPreferences_TagBellSound);
	tagList.push_back(kPreferences_TagNotification);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewNotificationsTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using the Options user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewOptionsTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagDontAutoClose);
	tagList.push_back(kPreferences_TagDontDimBackgroundScreens);
	tagList.push_back(kPreferences_TagPureInverse);
	tagList.push_back(kPreferences_TagCopySelectedText);
	tagList.push_back(kPreferences_TagCursorMovesPriorToDrops);
	tagList.push_back(kPreferences_TagNoPasteWarning);
	tagList.push_back(kPreferences_TagMapBackquote);
	tagList.push_back(kPreferences_TagDontAutoNewOnApplicationReopen);
	tagList.push_back(kPreferences_TagFocusFollowsMouse);
	tagList.push_back(kPreferences_TagFadeBackgroundWindows);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewOptionsTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using the Special user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewSpecialTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalCursorType);
	tagList.push_back(kPreferences_TagCursorBlinks);
	tagList.push_back(kPreferences_TagTerminalResizeAffectsFontSize);
	tagList.push_back(kPreferences_TagCopyTableThreshold);
	tagList.push_back(kPreferences_TagWindowStackingOrigin);
	tagList.push_back(kPreferences_TagNewCommandShortcutEffect);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewSpecialTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelGeneral_NewTagSet ()
{
	Preferences_TagSetRef	result = Preferences_NewTagSet(40); // arbitrary initial capacity
	Preferences_TagSetRef	optionTags = PrefPanelGeneral_NewOptionsTagSet();
	Preferences_TagSetRef	specialTags = PrefPanelGeneral_NewSpecialTagSet();
	Preferences_TagSetRef	fullScreenTags = PrefPanelGeneral_NewFullScreenTagSet();
	Preferences_TagSetRef	notificationTags = PrefPanelGeneral_NewNotificationsTagSet();
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_TagSetMerge(result, optionTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, specialTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, fullScreenTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, notificationTags);
	assert(kPreferences_ResultOK == prefsResult);
	
	Preferences_ReleaseTagSet(&optionTags);
	Preferences_ReleaseTagSet(&specialTags);
	Preferences_ReleaseTagSet(&fullScreenTags);
	Preferences_ReleaseTagSet(&notificationTags);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods

#pragma mark -
@implementation PrefPanelGeneral_ViewManager //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers = @[
										[[[PrefPanelGeneral_OptionsVC alloc] init] autorelease],
										[[[PrefPanelGeneral_SpecialViewManager alloc] init] autorelease],
										[[[PrefPanelGeneral_FullScreenVC alloc] init] autorelease],
										[[[PrefPanelGeneral_NotificationsVC alloc] init] autorelease],
									];
	NSString*	panelName = NSLocalizedStringFromTable(@"General", @"PrefPanelGeneral",
														@"the name of this panel");
	NSImage*	panelIcon = nil;
	
	
	if (@available(macOS 11.0, *))
	{
		panelIcon = [NSImage imageWithSystemSymbolName:@"gearshape" accessibilityDescription:self.panelName];
	}
	else
	{
		panelIcon = [NSImage imageNamed:@"IconForPrefPanelGeneral"];
	}
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.General"
										localizedName:panelName
										localizedIcon:panelIcon
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


@end //} PrefPanelGeneral_ViewManager


#pragma mark -
@implementation PrefPanelGeneral_FullScreenActionHandler //{


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
		_viewModel = [[UIPrefsGeneralFullScreen_Model alloc] initWithRunner:self]; // transfer ownership
	}
	return self;
}// init


/*!
Destructor.

(2020.11)
*/
- (void)
dealloc
{
	[_prefsMgr release];
	[_viewModel release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


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
	
	
	// allow initialization of values without triggers
	self.viewModel.disableWriteback = YES;
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKioskShowsScrollBar;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.showScrollBar = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKioskShowsMenuBar;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.showMenuBar = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKioskShowsWindowFrame;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.showWindowFrame = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKioskAllowsForceQuit;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.allowForceQuit = preferenceValue; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsGeneralOptions_ActionHandling


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
		Preferences_Tag		preferenceTag = kPreferences_TagKioskShowsScrollBar;
		Boolean				preferenceValue = self.viewModel.showScrollBar;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKioskShowsMenuBar;
		Boolean				preferenceValue = self.viewModel.showMenuBar;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKioskShowsWindowFrame;
		Boolean				preferenceValue = self.viewModel.showWindowFrame;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKioskAllowsForceQuit;
		Boolean				preferenceValue = self.viewModel.allowForceQuit;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
}// dataUpdated


@end //}


#pragma mark -
@implementation PrefPanelGeneral_FullScreenVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelGeneral_FullScreenActionHandler*	actionHandler = [[PrefPanelGeneral_FullScreenActionHandler alloc] init];
	NSView*										newView = [UIPrefsGeneralFullScreen_ObjC makeView:actionHandler.viewModel];
	
	
	self = [super initWithView:newView delegate:self context:actionHandler/* transfer ownership (becomes "actionHandler" property in "panelViewManager:initializeWithContext:") */];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


/*!
Destructor.

(2020.11)
*/
- (void)
dealloc
{
	[_actionHandler release];
	[super dealloc];
}// dealloc


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
initializeWithContext:(void*)			aContext/* PrefPanelGeneral_FullScreenActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelGeneral_FullScreenActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelGeneral_FullScreenActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 450, 180); // somewhat arbitrary; see SwiftUI code/playground
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self->logicalFirstResponder = self.view;
	self->logicalLastResponder = self.view;
	
	// update the view by changing the model’s observed variables (since this panel
	// does not manage collections, "panelViewManager:didChangeFromDataSet:toDataSet:"
	// will never be called so the view must be initialized from the context here)
	[self.actionHandler updateViewModelFromPrefsMgr];
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
	*outEditType = kPanel_EditTypeNormal;
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
	_idealFrame = [aContainerView frame];
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
	*outIdealSize = _idealFrame.size;
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

Not applicable to this panel because it only sets global
(Default) preferences.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
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
		return [NSImage imageWithSystemSymbolName:@"arrow.up.left.and.arrow.down.right" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelGeneral"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.General.FullScreen";
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
	return NSLocalizedStringFromTable(@"Full Screen", @"PrefPanelGeneral", @"the name of this panel");
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
	return Quills::Prefs::GENERAL;
}// preferencesClass


@end //} PrefPanelGeneral_FullScreenVC


#pragma mark -
@implementation PrefPanelGeneral_NotificationsActionHandler //{


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
		_viewModel = [[UIPrefsGeneralNotifications_Model alloc] initWithRunner:self]; // transfer ownership
		
		// fill in names of system sounds
		{
			NSArray*															soundNamesOnly = BRIDGE_CAST(CocoaBasic_ReturnUserSoundNames(), NSArray*);
			NSMutableArray< UIPrefsGeneralNotification_BellSoundItemModel* >*	modelArray = [[[NSMutableArray< UIPrefsGeneralNotification_BellSoundItemModel* > alloc] init] autorelease];
			
			
			for (NSString* aSoundName in soundNamesOnly)
			{
				UIPrefsGeneralNotification_BellSoundItemModel*		newItem = [[[UIPrefsGeneralNotification_BellSoundItemModel alloc]
																				initWithSoundName:aSoundName helpText:nil] autorelease];
				
				
				[modelArray addObject:newItem]; // (copies item)
			}
			self.viewModel.bellSoundItems = modelArray; // (copies array)
		}
	}
	return self;
}// init


/*!
Destructor.

(2020.11)
*/
- (void)
dealloc
{
	[_prefsMgr release];
	[_viewModel release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


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
	
	
	// allow initialization of values without triggers
	self.viewModel.disableWriteback = YES;
	
	// update notification sound
	{
		Preferences_Tag		preferenceTag = kPreferences_TagBellSound;
		CFStringRef			preferenceValue = CFSTR("");
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			NSString*	asNSString = BRIDGE_CAST(preferenceValue, NSString*);
			
			
			// see "Preferences.h" for special string values
			if ([asNSString isEqualToString:@"off"])
			{
				// no sound
				self.viewModel.selectedBellSoundID = UIPrefsGeneralNotification_BellSoundItemModel.offItemModel.uniqueID; // SwiftUI binding
			}
			else if ([asNSString isEqualToString:@""])
			{
				// default alert sound
				self.viewModel.selectedBellSoundID = UIPrefsGeneralNotification_BellSoundItemModel.defaultItemModel.uniqueID; // SwiftUI binding
			}
			else
			{
				// named sound
				self.viewModel.selectedBellSoundID = UIPrefsGeneralNotification_BellSoundItemModel.defaultItemModel.uniqueID; // initially...
				for (UIPrefsGeneralNotification_BellSoundItemModel* anItem in self.viewModel.bellSoundItems)
				{
					if ([asNSString isEqualToString:anItem.soundName])
					{
						self.viewModel.selectedBellSoundID = anItem.uniqueID; // SwiftUI binding
						break;
					}
				}
			}
		}
	}
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagVisualBell;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.visualBell = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagNotifyOfBeeps;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.bellNotificationInBackground = preferenceValue; // SwiftUI binding
		}
	}
	
	// update background notification type
	{
		Preferences_Tag		preferenceTag = kPreferences_TagNotification;
		SInt16				preferenceValue = 0;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			UIPrefsGeneralNotifications_BackgroundAction	bindingValue = UIPrefsGeneralNotifications_BackgroundActionModifyIcon;
			
			
			switch (preferenceValue)
			{
			case kAlert_NotifyDoNothing:
				bindingValue = UIPrefsGeneralNotifications_BackgroundActionNone;
				break;
			
			case kAlert_NotifyDisplayIconAndDiamondMark:
				bindingValue = UIPrefsGeneralNotifications_BackgroundActionAndBounceIcon;
				break;
			
			case kAlert_NotifyAlsoDisplayAlert:
				bindingValue = UIPrefsGeneralNotifications_BackgroundActionAndBounceRepeatedly;
				break;
			
			case kAlert_NotifyDisplayDiamondMark:
			default:
				bindingValue = UIPrefsGeneralNotifications_BackgroundActionModifyIcon;
				break;
			}
			self.viewModel.backgroundNotificationAction = bindingValue; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsGeneralOptions_ActionHandling


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
	
	
	// update notification sound
	{
		Preferences_Tag		preferenceTag = kPreferences_TagBellSound;
		CFStringRef			preferenceValue = CFSTR("");
		
		
		if ([self.viewModel.selectedBellSoundID isEqual:UIPrefsGeneralNotification_BellSoundItemModel.offItemModel.uniqueID])
		{
			preferenceValue = CFSTR("off"); // special value; see "Preferences.h"
		}
		else if ([self.viewModel.selectedBellSoundID isEqual:UIPrefsGeneralNotification_BellSoundItemModel.defaultItemModel.uniqueID])
		{
			preferenceValue = CFSTR(""); // special value; see "Preferences.h"
		}
		else
		{
			for (UIPrefsGeneralNotification_BellSoundItemModel* anItem in self.viewModel.bellSoundItems)
			{
				if ([self.viewModel.selectedBellSoundID isEqual:anItem.uniqueID])
				{
					// note: only sound names match their labels; special items like “Off” DO NOT (see above)
					preferenceValue = BRIDGE_CAST(anItem.soundName, CFStringRef);
					break;
				}
			}
		}
		
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagVisualBell;
		Boolean				preferenceValue = self.viewModel.visualBell;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagNotifyOfBeeps;
		Boolean				preferenceValue = self.viewModel.bellNotificationInBackground;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	
	// update background notification type
	{
		Preferences_Tag					preferenceTag = kPreferences_TagNotification;
		/*AlertMessages_NotifyType*/UInt16	enumPrefValue = kAlert_NotifyDisplayDiamondMark;
		
		
		switch (self.viewModel.backgroundNotificationAction)
		{
		case UIPrefsGeneralNotifications_BackgroundActionNone:
			enumPrefValue = kAlert_NotifyDoNothing;
			break;
		
		case UIPrefsGeneralNotifications_BackgroundActionAndBounceIcon:
			enumPrefValue = kAlert_NotifyDisplayIconAndDiamondMark;
			break;
		
		case UIPrefsGeneralNotifications_BackgroundActionAndBounceRepeatedly:
			enumPrefValue = kAlert_NotifyAlsoDisplayAlert;
			break;
		
		case UIPrefsGeneralNotifications_BackgroundActionModifyIcon:
		default:
			enumPrefValue = kAlert_NotifyDisplayDiamondMark;
			break;
		}
		
		SInt16				preferenceValue = STATIC_CAST(enumPrefValue, SInt16);
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
Plays the currently-selected notification sound, if any,
by using the sound name to look up a system sound.

(Called by the UI in response to user selections.)

(2020.11)
*/
- (void)
playSelectedBellSound
{
	if ([self.viewModel.selectedBellSoundID isEqual:UIPrefsGeneralNotification_BellSoundItemModel.offItemModel.uniqueID])
	{
		// do nothing
	}
	else if ([self.viewModel.selectedBellSoundID isEqual:UIPrefsGeneralNotification_BellSoundItemModel.defaultItemModel.uniqueID])
	{
		// user-specified alert sound
		NSBeep();
	}
	else
	{
		// named alert sound
		Boolean		foundName = false;
		
		
		for (UIPrefsGeneralNotification_BellSoundItemModel* anItem in self.viewModel.bellSoundItems)
		{
			if ([self.viewModel.selectedBellSoundID isEqual:anItem.uniqueID])
			{
				CocoaBasic_PlaySoundByName(BRIDGE_CAST(anItem.soundName, CFStringRef));
				foundName = true;
				break;
			}
		}
		
		if (false == foundName)
		{
			Console_Warning(Console_WriteLine, "sound play failed; unrecognized form of selected bell ID");
		}
	}
}// playSelectedBellSound


@end //}


#pragma mark -
@implementation PrefPanelGeneral_NotificationsVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelGeneral_NotificationsActionHandler*	actionHandler = [[PrefPanelGeneral_NotificationsActionHandler alloc] init];
	NSView*											newView = [UIPrefsGeneralNotifications_ObjC makeView:actionHandler.viewModel];
	
	
	self = [super initWithView:newView delegate:self context:actionHandler/* transfer ownership (becomes "actionHandler" property in "panelViewManager:initializeWithContext:") */];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


/*!
Destructor.

(2020.11)
*/
- (void)
dealloc
{
	[_actionHandler release];
	[super dealloc];
}// dealloc


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
initializeWithContext:(void*)			aContext/* PrefPanelGeneral_NotificationsActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelGeneral_NotificationsActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelGeneral_NotificationsActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 450, 320); // somewhat arbitrary; see SwiftUI code/playground
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self->logicalFirstResponder = self.view;
	self->logicalLastResponder = self.view;
	
	// update the view by changing the model’s observed variables (since this panel
	// does not manage collections, "panelViewManager:didChangeFromDataSet:toDataSet:"
	// will never be called so the view must be initialized from the context here)
	[self.actionHandler updateViewModelFromPrefsMgr];
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
	*outEditType = kPanel_EditTypeNormal;
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
	_idealFrame = [aContainerView frame];
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
	*outIdealSize = _idealFrame.size;
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

Not applicable to this panel because it only sets global
(Default) preferences.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
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
		return [NSImage imageWithSystemSymbolName:@"bell.badge" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelGeneral"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.General.Notifications";
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
	return NSLocalizedStringFromTable(@"Notifications", @"PrefPanelGeneral", @"the name of this panel");
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
	return Quills::Prefs::GENERAL;
}// preferencesClass


@end //} PrefPanelGeneral_NotificationsVC


#pragma mark -
@implementation PrefPanelGeneral_OptionsActionHandler //{


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
		_viewModel = [[UIPrefsGeneralOptions_Model alloc] initWithRunner:self]; // transfer ownership
	}
	return self;
}// init


/*!
Destructor.

(2020.11)
*/
- (void)
dealloc
{
	[_prefsMgr release];
	[_viewModel release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


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
	
	
	// allow initialization of values without triggers
	self.viewModel.disableWriteback = YES;
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagDontAutoClose;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.noWindowCloseOnExit = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagDontAutoNewOnApplicationReopen;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.noAutoNewWindows = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagFadeBackgroundWindows;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.fadeInBackground = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagPureInverse;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.invertSelectedText = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCopySelectedText;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.autoCopySelectedText = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCursorMovesPriorToDrops;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.moveCursorToTextDropLocation = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagDontDimBackgroundScreens;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.doNotDimBackgroundTerminalText = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagNoPasteWarning;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.doNotWarnAboutMultiLinePaste = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapBackquote;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.mapBackquoteToEscape = preferenceValue; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagFocusFollowsMouse;
		Boolean				preferenceValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		false/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.focusFollowsMouse = preferenceValue; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsGeneralOptions_ActionHandling


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
		Preferences_Tag		preferenceTag = kPreferences_TagDontAutoClose;
		Boolean				preferenceValue = self.viewModel.noWindowCloseOnExit;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagDontAutoNewOnApplicationReopen;
		Boolean				preferenceValue = self.viewModel.noAutoNewWindows;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagFadeBackgroundWindows;
		Boolean				preferenceValue = self.viewModel.fadeInBackground;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagPureInverse;
		Boolean				preferenceValue = self.viewModel.invertSelectedText;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCopySelectedText;
		Boolean				preferenceValue = self.viewModel.autoCopySelectedText;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCursorMovesPriorToDrops;
		Boolean				preferenceValue = self.viewModel.moveCursorToTextDropLocation;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagDontDimBackgroundScreens;
		Boolean				preferenceValue = self.viewModel.doNotDimBackgroundTerminalText;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagNoPasteWarning;
		Boolean				preferenceValue = self.viewModel.doNotWarnAboutMultiLinePaste;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapBackquote;
		Boolean				preferenceValue = self.viewModel.mapBackquoteToEscape;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagFocusFollowsMouse;
		Boolean				preferenceValue = self.viewModel.focusFollowsMouse;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
}// dataUpdated


@end //}


#pragma mark -
@implementation PrefPanelGeneral_OptionsVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelGeneral_OptionsActionHandler*		actionHandler = [[PrefPanelGeneral_OptionsActionHandler alloc] init];
	NSView*										newView = [UIPrefsGeneralOptions_ObjC makeView:actionHandler.viewModel];
	
	
	self = [super initWithView:newView delegate:self context:actionHandler/* transfer ownership (becomes "actionHandler" property in "panelViewManager:initializeWithContext:") */];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


/*!
Destructor.

(2020.11)
*/
- (void)
dealloc
{
	[_actionHandler release];
	[super dealloc];
}// dealloc


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
initializeWithContext:(void*)			aContext/* PrefPanelGeneral_OptionsActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelGeneral_OptionsActionHandler*		actionHandler = STATIC_CAST(aContext, PrefPanelGeneral_OptionsActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 480, 320); // somewhat arbitrary; see SwiftUI code/playground
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self->logicalFirstResponder = self.view;
	self->logicalLastResponder = self.view;
	
	// update the view by changing the model’s observed variables (since this panel
	// does not manage collections, "panelViewManager:didChangeFromDataSet:toDataSet:"
	// will never be called so the view must be initialized from the context here)
	[self.actionHandler updateViewModelFromPrefsMgr];
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
	*outEditType = kPanel_EditTypeNormal;
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
	_idealFrame = [aContainerView frame];
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
	*outIdealSize = _idealFrame.size;
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

Not applicable to this panel because it only sets global
(Default) preferences.

(2020.11)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
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
		return [NSImage imageWithSystemSymbolName:@"gearshape" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelGeneral"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.General.Options";
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
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelGeneral", @"the name of this panel");
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
	return Quills::Prefs::GENERAL;
}// preferencesClass


@end //} PrefPanelGeneral_OptionsVC


#pragma mark -
@implementation PrefPanelGeneral_SpecialViewManager //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelGeneralSpecialCocoa" delegate:self context:nullptr];
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
	[prefsMgr release];
	[byKey release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorFlashes
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagCursorBlinks defaultValue:NO];
}
- (void)
setCursorFlashes:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagCursorBlinks];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save cursor-flashing preference");
	}
}// setCursorFlashes:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsBlock
{
	return (kTerminal_CursorTypeBlock == [self readCursorTypeWithDefaultValue:kTerminal_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsBlock
{
	return NO;
}
- (void)
setCursorShapeIsBlock:(BOOL)	aFlag
{
	if ([self cursorShapeIsBlock] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminal_CursorTypeBlock];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-block preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsBlock:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsThickUnderline
{
	return (kTerminal_CursorTypeThickUnderscore == [self readCursorTypeWithDefaultValue:kTerminal_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsThickUnderline
{
	return NO;
}
- (void)
setCursorShapeIsThickUnderline:(BOOL)	aFlag
{
	if ([self cursorShapeIsThickUnderline] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminal_CursorTypeThickUnderscore];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-thick-underline preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsThickUnderline:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsThickVerticalBar
{
	return (kTerminal_CursorTypeThickVerticalLine == [self readCursorTypeWithDefaultValue:kTerminal_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsThickVerticalBar
{
	return NO;
}
- (void)
setCursorShapeIsThickVerticalBar:(BOOL)		aFlag
{
	if ([self cursorShapeIsThickVerticalBar] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminal_CursorTypeThickVerticalLine];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-thick-vertical-bar preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsThickVerticalBar:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsUnderline
{
	return (kTerminal_CursorTypeUnderscore == [self readCursorTypeWithDefaultValue:kTerminal_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsUnderline
{
	return NO;
}
- (void)
setCursorShapeIsUnderline:(BOOL)	aFlag
{
	if ([self cursorShapeIsUnderline] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminal_CursorTypeUnderscore];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-underline preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsUnderline:


/*!
Accessor.

(4.1)
*/
- (BOOL)
cursorShapeIsVerticalBar
{
	return (kTerminal_CursorTypeVerticalLine == [self readCursorTypeWithDefaultValue:kTerminal_CursorTypeBlock]);
}
+ (BOOL)
automaticallyNotifiesObserversOfCursorShapeIsVerticalBar
{
	return NO;
}
- (void)
setCursorShapeIsVerticalBar:(BOOL)		aFlag
{
	if ([self cursorShapeIsVerticalBar] != aFlag)
	{
		[self notifyWillChangeValueForCursorShape];
		
		BOOL	writeOK = [self writeCursorType:kTerminal_CursorTypeVerticalLine];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save cursor-shape-vertical-bar preference");
		}
		
		[self notifyDidChangeValueForCursorShape];
	}
}// setCursorShapeIsVerticalBar:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isWindowResizeEffectTerminalScreenSize
{
	BOOL	preferenceFlag = [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize
																		defaultValue:NO];
	
	
	return (NO == preferenceFlag);
}
- (void)
setWindowResizeEffectTerminalScreenSize:(BOOL)	aFlag
{
	if ([self isWindowResizeEffectTerminalScreenSize] != aFlag)
	{
		[self notifyWillChangeValueForWindowResizeEffect];
		
		BOOL	preferenceFlag = (NO == aFlag);
		BOOL	writeOK = [self->prefsMgr writeFlag:preferenceFlag
													forPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save window-resize-affects-font-size preference");
		}
		
		[self notifyDidChangeValueForWindowResizeEffect];
	}
}// setWindowResizeEffectTerminalScreenSize:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isWindowResizeEffectTextSize
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize defaultValue:NO];
}
- (void)
setWindowResizeEffectTextSize:(BOOL)	aFlag
{
	if ([self isWindowResizeEffectTextSize] != aFlag)
	{
		[self notifyWillChangeValueForWindowResizeEffect];
		
		BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagTerminalResizeAffectsFontSize];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save window-resize-affects-font-size preference");
		}
		
		[self notifyDidChangeValueForWindowResizeEffect];
	}
}// setWindowResizeEffectTextSize:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandCustomNewSession
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kSessionFactory_SpecialSessionDefaultFavorite];
	
	
	return (kSessionFactory_SpecialSessionInteractiveSheet == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandCustomNewSession
{
	return NO;
}
- (void)
setNewCommandCustomNewSession:(BOOL)	aFlag
{
	if ([self isNewCommandCustomNewSession] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kSessionFactory_SpecialSessionInteractiveSheet];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-custom-session preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandCustomNewSession:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandDefaultSessionFavorite
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kSessionFactory_SpecialSessionDefaultFavorite];
	
	
	return (kSessionFactory_SpecialSessionDefaultFavorite == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandDefaultSessionFavorite
{
	return NO;
}
- (void)
setNewCommandDefaultSessionFavorite:(BOOL)	aFlag
{
	if ([self isNewCommandDefaultSessionFavorite] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kSessionFactory_SpecialSessionDefaultFavorite];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-default-favorite preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandDefaultSessionFavorite:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandLogInShell
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kSessionFactory_SpecialSessionDefaultFavorite];
	
	
	return (kSessionFactory_SpecialSessionLogInShell == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandLogInShell
{
	return NO;
}
- (void)
setNewCommandLogInShell:(BOOL)	aFlag
{
	if ([self isNewCommandLogInShell] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kSessionFactory_SpecialSessionLogInShell];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-log-in-shell preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandLogInShell:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isNewCommandShell
{
	UInt32	value = [self readNewCommandShortcutEffectWithDefaultValue:kSessionFactory_SpecialSessionDefaultFavorite];
	
	
	return (kSessionFactory_SpecialSessionShell == value);
}
+ (BOOL)
automaticallyNotifiesObserversOfNewCommandShell
{
	return NO;
}
- (void)
setNewCommandShell:(BOOL)	aFlag
{
	if ([self isNewCommandShell] != aFlag)
	{
		[self notifyWillChangeValueForNewCommand];
		
		BOOL	writeOK = [self writeNewCommandShortcutEffect:kSessionFactory_SpecialSessionShell];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save new-command-is-shell preference");
		}
		
		[self notifyDidChangeValueForNewCommand];
	}
}// setNewCommandLogInShell:


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
spacesPerTab
{
	return [self->byKey objectForKey:@"spacesPerTab"];
}// spacesPerTab


#pragma mark Actions


/*!
Responds to a request to set the window stacking origin.

(4.1)
*/
- (IBAction)
performSetWindowStackingOrigin:(id)	sender
{
#pragma unused(sender)
	Keypads_SetArrangeWindowPanelBinding(kPreferences_TagWindowStackingOrigin, kPreferences_DataTypeCGPoint);
	Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, true);
}// performSetWindowStackingOrigin:


#pragma mark Validators


/*!
Validates the number of spaces entered by the user, returning an
appropriate error (and a NO result) if the number is incorrect.

(4.1)
*/
- (BOOL)
validateSpacesPerTab:(id*/* NSString* */)	ioValue
error:(NSError**)						outError
{
	BOOL	result = NO;
	
	
	if (nil == *ioValue)
	{
		result = YES;
	}
	else
	{
		// first strip whitespace
		*ioValue = [[*ioValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] retain];
		
		// while an NSNumberFormatter is more typical for validation,
		// the requirements for this numerical value are quite simple
		NSScanner*	scanner = [NSScanner scannerWithString:*ioValue];
		int			value = 0;
		
		
		// the error message below should agree with the range enforced here...
		if ([scanner scanInt:&value] && [scanner isAtEnd] && (value >= 1) && (value <= 24/* arbitrary */))
		{
			result = YES;
		}
		else
		{
			if (nil != outError) result = NO;
			else result = YES; // cannot return NO when the error instance is undefined
		}
		
		if (NO == result)
		{
			*outError = [NSError errorWithDomain:(NSString*)kConstantsRegistry_NSErrorDomainAppDefault
							code:kConstantsRegistry_NSErrorBadUserID
							userInfo:@{
											NSLocalizedDescriptionKey: NSLocalizedStringFromTable
																		(@"“Copy with Tab Substitution” requires a count from 1 to 24 spaces.",
																			@"PrefPanelGeneral"/* table */,
																			@"message displayed for bad numbers"),
										}];
		}
	}
	return result;
}// validateSpacesPerTab:error:


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
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:8/* arbitrary; number of expected settings */];
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
	*outEditType = kPanel_EditTypeNormal;
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
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagCopyTableThreshold
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16] autorelease]
					forKey:@"spacesPerTab"];
	
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
	*outIdealSize = [[self managedView] frame].size;
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
display the new data set.

Not applicable to this panel because it only sets global
(Default) preferences.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
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
		return [NSImage imageWithSystemSymbolName:@"gearshape.2" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelGeneral"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.General.Special";
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
	return NSLocalizedStringFromTable(@"Special", @"PrefPanelGeneral", @"the name of this panel");
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
	return Quills::Prefs::GENERAL;
}// preferencesClass


@end //} PrefPanelGeneral_SpecialViewManager


#pragma mark -
@implementation PrefPanelGeneral_SpecialViewManager (PrefPanelGeneral_SpecialViewManagerInternal) //{


#pragma mark New Methods


/*!
Send when changing any of the properties that affect the
shape of the cursor (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForCursorShape
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"cursorShapeIsVerticalBar"];
	[self didChangeValueForKey:@"cursorShapeIsUnderline"];
	[self didChangeValueForKey:@"cursorShapeIsThickVerticalBar"];
	[self didChangeValueForKey:@"cursorShapeIsThickUnderline"];
	[self didChangeValueForKey:@"cursorShapeIsBlock"];
}
- (void)
notifyWillChangeValueForCursorShape
{
	[self willChangeValueForKey:@"cursorShapeIsBlock"];
	[self willChangeValueForKey:@"cursorShapeIsThickUnderline"];
	[self willChangeValueForKey:@"cursorShapeIsThickVerticalBar"];
	[self willChangeValueForKey:@"cursorShapeIsUnderline"];
	[self willChangeValueForKey:@"cursorShapeIsVerticalBar"];
}// notifyWillChangeValueForCursorShape


/*!
Send when changing any of the properties that affect the
command-N key mapping (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForNewCommand
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"newCommandShell"];
	[self didChangeValueForKey:@"newCommandLogInShell"];
	[self didChangeValueForKey:@"newCommandDefaultSessionFavorite"];
	[self didChangeValueForKey:@"newCommandCustomNewSession"];
}
- (void)
notifyWillChangeValueForNewCommand
{
	[self willChangeValueForKey:@"newCommandCustomNewSession"];
	[self willChangeValueForKey:@"newCommandDefaultSessionFavorite"];
	[self willChangeValueForKey:@"newCommandLogInShell"];
	[self willChangeValueForKey:@"newCommandShell"];
}// notifyWillChangeValueForNewCommand


/*!
Send when changing any of the properties that affect the
window resize effect (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForWindowResizeEffect
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"windowResizeEffectTextSize"];
	[self didChangeValueForKey:@"windowResizeEffectTerminalScreenSize"];
}
- (void)
notifyWillChangeValueForWindowResizeEffect
{
	[self willChangeValueForKey:@"windowResizeEffectTerminalScreenSize"];
	[self willChangeValueForKey:@"windowResizeEffectTextSize"];
}// notifyWillChangeValueForWindowResizeEffect


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[
				@"spacesPerTab",
			];
}// primaryDisplayBindingKeys


/*!
Returns the current user preference for the cursor shape,
or the specified default value if no preference exists.

(4.1)
*/
- (Terminal_CursorType)
readCursorTypeWithDefaultValue:(Terminal_CursorType)	aDefaultValue
{
	Terminal_CursorType		result = aDefaultValue;
	Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagTerminalCursorType,
																sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readCursorTypeWithDefaultValue:


/*!
Returns the current user preference for “command-N mapping”,
or the specified default value if no preference exists.

(4.1)
*/
- (UInt32)
readNewCommandShortcutEffectWithDefaultValue:(UInt32)	aDefaultValue
{
	UInt32				result = aDefaultValue;
	Preferences_Result	prefsResult = Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
															sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readNewCommandShortcutEffectWithDefaultValue:


/*!
Returns the current user preference for spaces per tab,
or the specified default value if no preference exists.

(4.1)
*/
- (UInt16)
readSpacesPerTabWithDefaultValue:(UInt16)	aDefaultValue
{
	UInt16				result = aDefaultValue;
	Preferences_Result	prefsResult = Preferences_GetData(kPreferences_TagCopyTableThreshold,
															sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readSpacesPerTabWithDefaultValue:


/*!
Writes a new user preference for the cursor shape and
returns YES only if this succeeds.

(4.1)
*/
- (BOOL)
writeCursorType:(Terminal_CursorType)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagTerminalCursorType,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeCursorType:


/*!
Writes a new user preference for “command-N mapping” and
returns YES only if this succeeds.

(4.1)
*/
- (BOOL)
writeNewCommandShortcutEffect:(UInt32)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagNewCommandShortcutEffect,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeNewCommandShortcutEffect:


/*!
Writes a new user preference for spaces per tab and
returns YES only if this succeeds.

(4.1)
*/
- (BOOL)
writeSpacesPerTab:(UInt16)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagCopyTableThreshold,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeSpacesPerTab:


@end //} PrefPanelGeneral_SpecialViewManager (PrefPanelGeneral_SpecialViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
