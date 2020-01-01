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
#import "NetEvents.h"
#import "Panel.h"
#import "Preferences.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "UIStrings.h"



#pragma mark Types

/*!
Implements an object wrapper for sound names, that allows them
to be correctly bound into user interface elements.  (On older
Mac OS X versions, raw strings do not bind properly.)
*/
@interface PrefPanelGeneral_SoundInfo : BoundName_Object //{
{
@private
	BOOL	isDefault;
	BOOL	isOff;
}

// initializers
	- (instancetype)
	initAsDefault;
	- (instancetype)
	initAsOff;
	- (instancetype)
	initWithBoundName:(NSString*)_;
	- (instancetype)
	initWithDescription:(NSString*)_;
	- (instancetype)
	initWithDescription:(NSString*)_
	isDefault:(BOOL)_
	isOff:(BOOL)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (void)
	playSound;
	- (NSString*)
	preferenceString;

@end //}


/*!
Private properties.
*/
@interface PrefPanelGeneral_NotificationsViewManager () //{

// accessors
	@property (assign) BOOL
	didLoadView;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelGeneral_FullScreenViewManager (PrefPanelGeneral_FullScreenViewManagerInternal) @end


/*!
The private class interface.
*/
@interface PrefPanelGeneral_NotificationsViewManager (PrefPanelGeneral_NotificationsViewManagerInternal) //{

// new methods
	- (void)
	notifyDidChangeValueForBackgroundNotification;
	- (void)
	notifyWillChangeValueForBackgroundNotification;

// preference setting accessors
	- (SInt16)
	readBackgroundNotificationTypeWithDefaultValue:(SInt16)_;
	- (NSString*)
	readBellSoundNameWithDefaultValue:(NSString*)_;
	- (BOOL)
	writeBackgroundNotificationType:(SInt16)_;
	- (BOOL)
	writeBellSoundName:(NSString*)_;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelGeneral_OptionsViewManager (PrefPanelGeneral_OptionsViewManagerInternal) @end


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
	tagList.push_back(kPreferences_TagKioskNoSystemFullScreenMode);
	tagList.push_back(kPreferences_TagKioskShowsOffSwitch);
	
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
										[[[PrefPanelGeneral_OptionsViewManager alloc] init] autorelease],
										[[[PrefPanelGeneral_SpecialViewManager alloc] init] autorelease],
										[[[PrefPanelGeneral_FullScreenViewManager alloc] init] autorelease],
										[[[PrefPanelGeneral_NotificationsViewManager alloc] init] autorelease],
									];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.General"
										localizedName:NSLocalizedStringFromTable(@"General", @"PrefPanelGeneral",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelGeneral"]
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
@implementation PrefPanelGeneral_SoundInfo //{


#pragma mark Initializers


/*!
Creates an object representing the user’s system-wide
default alert sound.

(4.1)
*/
- (instancetype)
initAsDefault
{
	self = [self initWithDescription:nil isDefault:YES isOff:NO];
	if (nil != self)
	{
	}
	return self;
}// initAsDefault


/*!
Creates an object representing the sound-off state.

(4.1)
*/
- (instancetype)
initAsOff
{
	self = [self initWithDescription:nil isDefault:NO isOff:YES];
	if (nil != self)
	{
	}
	return self;
}// initAsOff


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithBoundName:(NSString*)	aDescription
{
	return [self initWithDescription:aDescription];
}// initWithBoundName:


/*!
Creates an object representing a particular sound name.

(4.1)
*/
- (instancetype)
initWithDescription:(NSString*)		aDescription
{
	self = [self initWithDescription:aDescription isDefault:NO isOff:NO];
	if (nil != self)
	{
	}
	return self;
}// initWithDescription:


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithDescription:(NSString*)		aDescription
isDefault:(BOOL)					aDefaultFlag
isOff:(BOOL)						anOffFlag
{
	self = [super initWithBoundName:aDescription];
	if (nil != self)
	{
		self->isDefault = aDefaultFlag;
		self->isOff = anOffFlag;
		if (aDefaultFlag)
		{
			[self setDescription:NSLocalizedStringFromTable(@"Default", @"PrefPanelGeneral",
															@"label for the default terminal bell sound")];
		}
		else if (anOffFlag)
		{
			[self setDescription:NSLocalizedStringFromTable(@"Off", @"PrefPanelGeneral",
															@"label for turning off the terminal bell sound")];
		}
		else
		{
			[self setDescription:aDescription];
		}
	}
	return self;
}// initWithDescription:isDefault:isOff:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Plays the sound (if any) that is represented by this object.

(4.1)
*/
- (void)
playSound
{
	if (self->isDefault)
	{
		NSBeep();
	}
	else if (self->isOff)
	{
		// do nothing
	}
	else
	{
		CocoaBasic_PlaySoundByName((CFStringRef)[self boundName]);
	}
}// playSound


/*!
Returns the string that should be used when storing this setting
in user preferences.  This is usually the name of the sound, but
for the Off and Default objects it is different.

(4.0)
*/
- (NSString*)
preferenceString
{
	if (self->isDefault)
	{
		return @"";
	}
	else if (self->isOff)
	{
		return @"off";
	}
	return [self boundName];
}// preferenceString


@end //} PrefPanelGeneral_SoundInfo


#pragma mark -
@implementation PrefPanelGeneral_FullScreenViewManager //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelGeneralFullScreenCocoa" delegate:self context:nullptr];
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
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
isForceQuitEnabled
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagKioskAllowsForceQuit defaultValue:NO];
}
- (void)
setForceQuitEnabled:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskAllowsForceQuit];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save Force-Quit-in-full-screen preference");
	}
}// setForceQuitEnabled:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isMenuBarShownOnDemand
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagKioskShowsMenuBar defaultValue:NO];
}
- (void)
setMenuBarShownOnDemand:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsMenuBar];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save menu-bar-on-demand preference");
	}
}// setMenuBarShownOnDemand:


/*!
Accessor.

(4.1)
*/
- (BOOL)
nonSystemMechanismEnabled
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagKioskNoSystemFullScreenMode defaultValue:NO];
}
- (void)
setNonSystemMechanismEnabled:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskNoSystemFullScreenMode];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save full-screen mode preference");
	}
}// setNonSystemMechanismEnabled:


/*!
Accessor.

(4.1)
*/
- (BOOL)
offSwitchWindowEnabled
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagKioskShowsOffSwitch defaultValue:NO];
}
- (void)
setOffSwitchWindowEnabled:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsOffSwitch];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save off-switch-window preference");
	}
}// setOffSwitchWindowEnabled:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isScrollBarVisible
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagKioskShowsScrollBar defaultValue:NO];
}
- (void)
setScrollBarVisible:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsScrollBar];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save scroll-bar-in-full-screen preference");
	}
}// setScrollBarVisible:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isWindowFrameVisible
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagKioskShowsWindowFrame defaultValue:NO];
}
- (void)
setWindowFrameVisible:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagKioskShowsWindowFrame];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save window-frame-in-full-screen preference");
	}
}// setWindowFrameVisible:


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
}// panelViewManager:requestingIdealSize:


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
	return @"net.macterm.prefpanels.General.FullScreen";
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
	return NSLocalizedStringFromTable(@"Full Screen", @"PrefPanelGeneral", @"the name of this panel");
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


@end //} PrefPanelGeneral_FullScreenViewManager


#pragma mark -
@implementation PrefPanelGeneral_FullScreenViewManager (PrefPanelGeneral_FullScreenViewManagerInternal) //{


@end //} PrefPanelGeneral_FullScreenViewManager (PrefPanelGeneral_FullScreenViewManagerInternal)


#pragma mark -
@implementation PrefPanelGeneral_NotificationsViewManager //{


#pragma mark Internally-Declared Properties


/*!
Set to YES when "panelViewManager:didLoadContainerView:"
is completed.

NOTE: In later SDKs, it may be possible to replace this
property with the "viewLoaded" property from the parent
NSViewController class.
*/
@synthesize didLoadView = _didLoadView;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelGeneralNotificationsCocoa" delegate:self context:nullptr];
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
	[soundNameIndexes release];
	[soundNames release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
alwaysUseVisualBell
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagVisualBell defaultValue:NO];
}
- (void)
setAlwaysUseVisualBell:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagVisualBell];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save visual-bell preference");
	}
}// setAlwaysUseVisualBell:


/*!
Accessor.

(4.1)
*/
- (BOOL)
backgroundBellsSendNotifications
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagNotifyOfBeeps defaultValue:NO];
}
- (void)
setBackgroundBellsSendNotifications:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagNotifyOfBeeps];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save background-bell-notification preference");
	}
}// setBackgroundBellsSendNotifications:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationNone
{
	return (kAlert_NotifyDoNothing == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationNone
{
	return NO;
}
- (void)
setBackgroundNotificationNone:(BOOL)	aFlag
{
	if ([self isBackgroundNotificationNone] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyDoNothing];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-none preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationNone:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationChangeDockIcon
{
	return (kAlert_NotifyDisplayDiamondMark == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationChangeDockIcon
{
	return NO;
}
- (void)
setBackgroundNotificationChangeDockIcon:(BOOL)	aFlag
{
	if ([self isBackgroundNotificationChangeDockIcon] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyDisplayDiamondMark];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-change-Dock-icon preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationChangeDockIcon:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationAnimateIcon
{
	return (kAlert_NotifyDisplayIconAndDiamondMark == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationAnimateIcon
{
	return NO;
}
- (void)
setBackgroundNotificationAnimateIcon:(BOOL)		aFlag
{
	if ([self isBackgroundNotificationAnimateIcon] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyDisplayIconAndDiamondMark];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-animate-icon preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationAnimateIcon:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isBackgroundNotificationDisplayMessage
{
	return (kAlert_NotifyAlsoDisplayAlert == [self readBackgroundNotificationTypeWithDefaultValue:kAlert_NotifyDoNothing]);
}
+ (BOOL)
automaticallyNotifiesObserversOfBackgroundNotificationDisplayMessage
{
	return NO;
}
- (void)
setBackgroundNotificationDisplayMessage:(BOOL)		aFlag
{
	if ([self isBackgroundNotificationDisplayMessage] != aFlag)
	{
		[self notifyWillChangeValueForBackgroundNotification];
		
		BOOL	writeOK = [self writeBackgroundNotificationType:kAlert_NotifyAlsoDisplayAlert];
		
		
		if (NO == writeOK)
		{
			Console_Warning(Console_WriteLine, "failed to save background-notification-display-message preference");
		}
		
		[self notifyDidChangeValueForBackgroundNotification];
	}
}// setBackgroundNotificationDisplayMessage:


/*!
Accessor.

(4.0)
*/
- (NSArray*)
soundNames
{
	return [[soundNames retain] autorelease];
}


/*!
Accessor.

(4.0)
*/
- (NSIndexSet*)
soundNameIndexes
{
	return [[soundNameIndexes retain] autorelease];
}
+ (BOOL)
automaticallyNotifiesObserversOfSoundNameIndexes
{
	return NO;
}
- (void)
setSoundNameIndexes:(NSIndexSet*)	indexes
{
	NSUInteger						newIndex = (nil != indexes)
												? [indexes firstIndex]
												: 0;
	PrefPanelGeneral_SoundInfo*		info = ((NSNotFound != newIndex) && (newIndex < [[self soundNames] count]))
											? [[self soundNames] objectAtIndex:newIndex]
											: nil;
	
	
	if (indexes != soundNameIndexes)
	{
		[self willChangeValueForKey:@"soundNameIndexes"];
		
		[soundNameIndexes release];
		soundNameIndexes = [indexes retain];
		
		// once the UI is loaded, start auto-saving the new preference
		// (do not do this earlier, as binding setup should not overwrite
		// previous saves with arbitrary initial values)
		if ((self.isPanelUserInterfaceLoaded) && ([indexes count] > 0))
		{
			BOOL		writeOK = NO;
			NSString*	savedName = [info preferenceString];
			
			
			//Console_Warning(Console_WriteValueCFString, "write bell-sound preference", BRIDGE_CAST(savedName, CFStringRef)); // debug
			if (nil != savedName)
			{
				writeOK = [self writeBellSoundName:savedName];
				if (writeOK)
				{
					// do not play sounds during initialization; only afterward
					if (self.didLoadView)
					{
						[info playSound];
					}
				}
			}
			
			if (NO == writeOK)
			{
				Console_Warning(Console_WriteValueCFString, "failed to save bell-sound preference", BRIDGE_CAST(savedName, CFStringRef));
			}
		}
		
		[self didChangeValueForKey:@"soundNameIndexes"];
	}
	else
	{
		// play the sound either way, even if the user chooses the same item again
		[info playSound];
	}
}// setSoundNameIndexes:


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
	
	// set up the array of bell sounds with a dummy list
	// so that default bindings work (these are corrected
	// after the load completes)
	[self willChangeValueForKey:@"soundNames"];
	[self willChangeValueForKey:@"soundNameIndexes"];
	self->soundNames = [[NSMutableArray alloc] init];
	[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc] initAsOff] autorelease]];
	self->soundNameIndexes = [[NSIndexSet indexSetWithIndex:0] retain];
	self->_didLoadView = NO;
	[self didChangeValueForKey:@"soundNameIndexes"];
	[self didChangeValueForKey:@"soundNames"];
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
	// once the view is loaded (post-auto-bindings), the index
	// values can be set from preferences; if this is done any
	// sooner, values read from preferences can be overwritten
	// by whatever arbitrary index is set by Cocoa bindings
	{
		NSArray*		soundNamesOnly = BRIDGE_CAST(CocoaBasic_ReturnUserSoundNames(), NSArray*);
		NSString*		savedName = [self readBellSoundNameWithDefaultValue:@""];
		unsigned int	currentIndex = 0;
		unsigned int	initialIndex = 0;
		
		
		[self willChangeValueForKey:@"soundNames"];
		[self->soundNames removeAllObjects]; // erase dummy initial values
		[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc] initAsOff] autorelease]];
		if ([savedName isEqualToString:[[self->soundNames lastObject] preferenceString]])
		{
			initialIndex = currentIndex;
		}
		++currentIndex;
		[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc] initAsDefault] autorelease]];
		if ([savedName isEqualToString:[[self->soundNames lastObject] preferenceString]])
		{
			initialIndex = currentIndex;
		}
		++currentIndex;
		if (soundNamesOnly.count > 0)
		{
			// the dash "-" is translated into a separator by the menu delegate
			[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc]
												initWithDescription:@"-"]
											autorelease]];
			++currentIndex;
		}
		for (NSString* soundName in soundNamesOnly)
		{
			[self->soundNames addObject:[[[PrefPanelGeneral_SoundInfo alloc]
												initWithDescription:soundName]
											autorelease]];
			if ([savedName isEqualToString:[[self->soundNames lastObject] preferenceString]])
			{
				initialIndex = currentIndex;
			}
			++currentIndex;
		}
		[self willChangeValueForKey:@"soundNameIndexes"];
		self->soundNameIndexes = [[NSIndexSet indexSetWithIndex:initialIndex] retain];
		[self didChangeValueForKey:@"soundNameIndexes"];
		// note: the system appears to observe "soundNames" (refreshing the
		// array controller with a dependency on "soundNameIndexes") so this
		// call must come last, after the desired index value is in place
		[self didChangeValueForKey:@"soundNames"];
		
		self.didLoadView = YES;
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
}// panelViewManager:requestingIdealSize:


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
	return @"net.macterm.prefpanels.General.Notifications";
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
	return NSLocalizedStringFromTable(@"Notifications", @"PrefPanelGeneral", @"the name of this panel");
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


@end //} PrefPanelGeneral_NotificationsViewManager


#pragma mark -
@implementation PrefPanelGeneral_NotificationsViewManager (PrefPanelGeneral_NotificationsViewManagerInternal) //{


#pragma mark New Methods


/*!
Send when changing any of the properties that affect the
command-N key mapping (as they are related).

(4.1)
*/
- (void)
notifyDidChangeValueForBackgroundNotification
{
	// note: should occur in opposite order of corresponding "willChangeValueForKey:" invocations
	[self didChangeValueForKey:@"backgroundNotificationNone"];
	[self didChangeValueForKey:@"backgroundNotificationChangeDockIcon"];
	[self didChangeValueForKey:@"backgroundNotificationAnimateIcon"];
	[self didChangeValueForKey:@"backgroundNotificationDisplayMessage"];
}
- (void)
notifyWillChangeValueForBackgroundNotification
{
	[self willChangeValueForKey:@"backgroundNotificationDisplayMessage"];
	[self willChangeValueForKey:@"backgroundNotificationAnimateIcon"];
	[self willChangeValueForKey:@"backgroundNotificationChangeDockIcon"];
	[self willChangeValueForKey:@"backgroundNotificationNone"];
}// notifyWillChangeValueForBackgroundNotification


/*!
Returns the current user preference for background notifications,
or the specified default value if no preference exists.  The
result will match a "kAlert_Notify..." constant.

(4.1)
*/
- (SInt16)
readBackgroundNotificationTypeWithDefaultValue:(SInt16)		aDefaultValue
{
	SInt16					result = aDefaultValue;
	Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagNotification,
																sizeof(result), &result);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		result = aDefaultValue; // assume default, if preference can’t be found
	}
	return result;
}// readBackgroundNotificationTypeWithDefaultValue:


/*!
Returns the current user preference for the bell sound.

This is the base name of a sound file, or an empty string
(to indicate the system’s default alert sound is played)
or the special string "off" to indicate no sound at all.

(4.1)
*/
- (NSString*)
readBellSoundNameWithDefaultValue:(NSString*)	aDefaultValue
{
	NSString*			result = nil;
	CFStringRef			soundName = nullptr;
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	BOOL				releaseSoundName = YES;
	
	
	// determine user’s preferred sound
	prefsResult = Preferences_GetData(kPreferences_TagBellSound, sizeof(soundName),
										&soundName);
	if (kPreferences_ResultOK != prefsResult)
	{
		soundName = BRIDGE_CAST(aDefaultValue, CFStringRef);
		releaseSoundName = NO;
	}
	
	result = [[((NSString*)soundName) retain] autorelease];
	if (releaseSoundName)
	{
		CFRelease(soundName), soundName = nullptr;
	}
	
	return result;
}// readBellSoundNameWithDefaultValue:


/*!
Writes a new user preference for background notifications and
returns YES only if this succeeds.  The given value must match
a "kAlert_Notify..." constant.

(4.1)
*/
- (BOOL)
writeBackgroundNotificationType:(SInt16)	aValue
{
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagNotification,
															sizeof(aValue), &aValue);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeBackgroundNotificationType:


/*!
Writes a new user preference for the bell sound.  For details
on what the string can be, see the documentation for the
"readBellSoundNameWithDefaultValue" method.

(4.1)
*/
- (BOOL)
writeBellSoundName:(NSString*)	aValue
{
	CFStringRef			asCFStringRef = BRIDGE_CAST(aValue, CFStringRef);
	Preferences_Result	prefsResult = Preferences_SetData(kPreferences_TagBellSound,
															sizeof(asCFStringRef), &asCFStringRef);
	
	
	return (kPreferences_ResultOK == prefsResult);
}// writeBellSoundName:


@end //} PrefPanelGeneral_NotificationsViewManager (PrefPanelGeneral_NotificationsViewManagerInternal)


#pragma mark -
@implementation PrefPanelGeneral_OptionsViewManager //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelGeneralOptionsCocoa" delegate:self context:nullptr];
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
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
noWindowCloseOnProcessExit
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagDontAutoClose defaultValue:NO];
}
- (void)
setNoWindowCloseOnProcessExit:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagDontAutoClose];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save no-auto-close preference");
	}
}// setNoWindowCloseOnProcessExit:


/*!
Accessor.

(4.1)
*/
- (BOOL)
noAutomaticNewWindows
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagDontAutoNewOnApplicationReopen defaultValue:NO];
}
- (void)
setNoAutomaticNewWindows:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagDontAutoNewOnApplicationReopen];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save no-auto-new preference");
	}
}// setNoAutomaticNewWindows:


/*!
Accessor.

(4.1)
*/
- (BOOL)
fadeInBackground
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagFadeBackgroundWindows defaultValue:NO];
}
- (void)
setFadeInBackground:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagFadeBackgroundWindows];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save fade-in-background preference");
	}
}// setFadeInBackground:


/*!
Accessor.

(4.1)
*/
- (BOOL)
invertSelectedText
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagPureInverse defaultValue:NO];
}
- (void)
setInvertSelectedText:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagPureInverse];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save invert-selected-text preference");
	}
}// setInvertSelectedText:


/*!
Accessor.

(4.1)
*/
- (BOOL)
automaticallyCopySelectedText
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagCopySelectedText defaultValue:NO];
}
- (void)
setAutomaticallyCopySelectedText:(BOOL)		aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagCopySelectedText];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save auto-copy-selected-text preference");
	}
}// setAutomaticallyCopySelectedText:


/*!
Accessor.

(4.1)
*/
- (BOOL)
moveCursorToTextDropLocation
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagCursorMovesPriorToDrops defaultValue:NO];
}
- (void)
setMoveCursorToTextDropLocation:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagCursorMovesPriorToDrops];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save move-cursor-to-drop-location preference");
	}
}// setMoveCursorToTextDropLocation:


/*!
Accessor.

(4.1)
*/
- (BOOL)
doNotDimBackgroundTerminalText
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagDontDimBackgroundScreens defaultValue:NO];
}
- (void)
setDoNotDimBackgroundTerminalText:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagDontDimBackgroundScreens];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save do-not-dim preference");
	}
}// setDoNotDimBackgroundTerminalText:


/*!
Accessor.

(4.1)
*/
- (BOOL)
doNotWarnAboutMultiLinePaste
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagNoPasteWarning defaultValue:NO];
}
- (void)
setDoNotWarnAboutMultiLinePaste:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagNoPasteWarning];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save do-not-warn-about-multi-line-paste preference");
	}
}// setDoNotWarnAboutMultiLinePaste:


/*!
Accessor.

(4.1)
*/
- (BOOL)
treatBackquoteLikeEscape
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagMapBackquote defaultValue:NO];
}
- (void)
setTreatBackquoteLikeEscape:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagMapBackquote];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save backquote-is-escape preference");
	}
}// setTreatBackquoteLikeEscape:


/*!
Accessor.

(4.1)
*/
- (BOOL)
focusFollowsMouse
{
	return [self->prefsMgr readFlagForPreferenceTag:kPreferences_TagFocusFollowsMouse defaultValue:NO];
}
- (void)
setFocusFollowsMouse:(BOOL)	aFlag
{
	BOOL	writeOK = [self->prefsMgr writeFlag:aFlag forPreferenceTag:kPreferences_TagFocusFollowsMouse];
	
	
	if (NO == writeOK)
	{
		Console_Warning(Console_WriteLine, "failed to save focus-follows-mouse preference");
	}
}// setFocusFollowsMouse:


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
	return @"net.macterm.prefpanels.General.Options";
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
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelGeneral", @"the name of this panel");
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


@end //} PrefPanelGeneral_OptionsViewManager


#pragma mark -
@implementation PrefPanelGeneral_OptionsViewManager (PrefPanelGeneral_OptionsViewManagerInternal) //{


@end //} PrefPanelGeneral_OptionsViewManager (PrefPanelGeneral_OptionsViewManagerInternal)


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
	Keypads_SetArrangeWindowPanelBinding(kPreferences_TagWindowStackingOrigin, typeNetEvents_CGPoint);
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
