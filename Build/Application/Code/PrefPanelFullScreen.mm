/*!	\file PrefPanelFullScreen.mm
	\brief Implements the Full Screen panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#import "PrefPanelFullScreen.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <HelpSystem.h>
#import <Localization.h>
#import <MemoryBlocks.h>

// application includes
#import "ConstantsRegistry.h"
#import "Panel.h"
#import "Preferences.h"
#import "UIStrings.h"


#pragma mark Internal Method Prototypes

/*!
The private class interface.
*/
@interface PrefPanelFullScreen_ViewManager (PrefPanelFullScreen_ViewManagerInternal) @end


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
PrefPanelFullScreen_NewTagSet ()
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
}// NewTagSet


#pragma mark Internal Methods

#pragma mark -
@implementation PrefPanelFullScreen_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelFullScreenCocoa" delegate:self context:nullptr];
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
	return [NSImage imageNamed:@"IconForPrefPanelKiosk"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.FullScreen";
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
	return NSLocalizedStringFromTable(@"Full Screen", @"PrefPanelFullScreen", @"the name of this panel");
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


@end // PrefPanelFullScreen_ViewManager


#pragma mark -
@implementation PrefPanelFullScreen_ViewManager (PrefPanelFullScreen_ViewManagerInternal)


@end // PrefPanelFullScreen_ViewManager (PrefPanelFullScreen_ViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
