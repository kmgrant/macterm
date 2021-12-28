/*!	\file PrefPanelTerminals.mm
	\brief Implements the Terminals panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#import "PrefPanelTerminals.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

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
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "Emulation.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "PreferenceValue.objc++.h"
#import "Terminal.h"
#import "UIStrings.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>


#pragma mark Internal Method Prototypes

/*!
Implements SwiftUI interaction for the “emulation” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelTerminals_EmulationActionHandler : NSObject< UIPrefsTerminalEmulation_ActionHandling > //{
{
@private
	PrefsContextManager_Object*			_prefsMgr;
	UIPrefsTerminalEmulation_Model*		_viewModel;
}

// new methods
	- (BOOL)
	resetToDefaultGetFlagWithTag:(Preferences_Tag)_;
	- (Emulation_FullType)
	returnPreferenceForUIEnum:(UIPrefsTerminalEmulation_BaseEmulatorType)_;
	- (UIPrefsTerminalEmulation_BaseEmulatorType)
	returnUIEnumForPreference:(Emulation_FullType)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsTerminalEmulation_Model*
	viewModel;

@end //}


/*!
Implements SwiftUI interaction for the “terminal options” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelTerminals_OptionsActionHandler : NSObject< UIPrefsTerminalOptions_ActionHandling > //{
{
@private
	PrefsContextManager_Object*		_prefsMgr;
	UIPrefsTerminalOptions_Model*	_viewModel;
}

// new methods
	- (BOOL)
	resetToDefaultGetFlagWithTag:(Preferences_Tag)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsTerminalOptions_Model*
	viewModel;

@end //}


/*!
Implements SwiftUI interaction for the “terminal screen size” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelTerminals_ScreenActionHandler : NSObject< UIPrefsTerminalScreen_ActionHandling > //{
{
@private
	PrefsContextManager_Object*		_prefsMgr;
	UIPrefsTerminalScreen_Model*	_viewModel;
}

// new methods
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsTerminalScreen_Model*
	viewModel;

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
PrefPanelTerminals_NewEmulationPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads emulation-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalEmulatorType);
	tagList.push_back(kPreferences_TagTerminalAnswerBackMessage);
	tagList.push_back(kPreferences_TagTerminal24BitColorEnabled);
	tagList.push_back(kPreferences_TagITermGraphicsEnabled);
	tagList.push_back(kPreferences_TagVT100FixLineWrappingBug);
	tagList.push_back(kPreferences_TagSixelGraphicsEnabled);
	tagList.push_back(kPreferences_TagXTerm256ColorsEnabled);
	tagList.push_back(kPreferences_TagXTermBackgroundColorEraseEnabled);
	tagList.push_back(kPreferences_TagXTermColorEnabled);
	tagList.push_back(kPreferences_TagXTermGraphicsEnabled);
	tagList.push_back(kPreferences_TagXTermWindowAlterationEnabled);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewEmulationPaneTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelTerminals_NewOptionsPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads option-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalLineWrap);
	tagList.push_back(kPreferences_TagDataReceiveDoNotStripHighBit);
	tagList.push_back(kPreferences_TagTerminalClearSavesLines);
	tagList.push_back(kPreferences_TagMapKeypadTopRowForVT220);
	tagList.push_back(kPreferences_TagPageKeysControlLocalTerminal);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewOptionsPaneTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.0)
*/
Preferences_TagSetRef
PrefPanelTerminals_NewScreenPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads screen-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalScreenColumns);
	tagList.push_back(kPreferences_TagTerminalScreenRows);
	tagList.push_back(kPreferences_TagTerminalScreenScrollbackRows);
	tagList.push_back(kPreferences_TagTerminalScreenScrollbackType);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewScreenPaneTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelTerminals_NewTagSet ()
{
	Preferences_TagSetRef	result = Preferences_NewTagSet(40); // arbitrary initial capacity
	Preferences_TagSetRef	emulationTags = PrefPanelTerminals_NewEmulationPaneTagSet();
	Preferences_TagSetRef	screenTags = PrefPanelTerminals_NewScreenPaneTagSet();
	Preferences_TagSetRef	optionTags = PrefPanelTerminals_NewOptionsPaneTagSet();
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_TagSetMerge(result, emulationTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, screenTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, optionTags);
	assert(kPreferences_ResultOK == prefsResult);
	
	Preferences_ReleaseTagSet(&emulationTags);
	Preferences_ReleaseTagSet(&screenTags);
	Preferences_ReleaseTagSet(&optionTags);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods

#pragma mark -
@implementation PrefPanelTerminals_ViewManager //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers = @[
										[[PrefPanelTerminals_OptionsVC alloc] init],
										[[PrefPanelTerminals_EmulationVC alloc] init],
										[[PrefPanelTerminals_ScreenVC alloc] init],
									];
	NSString*	panelName = NSLocalizedStringFromTable(@"Terminals", @"PrefPanelTerminals",
														@"the name of this panel");
	NSImage*	panelIcon = nil;
	
	
	if (@available(macOS 11.0, *))
	{
		panelIcon = [NSImage imageWithSystemSymbolName:@"chevron.right" accessibilityDescription:self.panelName];
	}
	else
	{
		panelIcon = [NSImage imageNamed:@"IconForPrefPanelTerminals"];
	}
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Terminals"
										localizedName:panelName
										localizedIcon:panelIcon
										viewManagerArray:subViewManagers];
	if (nil != self)
	{
	}
	return self;
}// init


@end //} PrefPanelTerminals_ViewManager


#pragma mark -
@implementation PrefPanelTerminals_EmulationActionHandler //{


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
		_viewModel = [[UIPrefsTerminalEmulation_Model alloc] initWithRunner:self]; // transfer ownership
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
Translate from UI-specified base emulator type constant to
the equivalent constant stored in Preferences.

TEMPORARY; this is primarily needed because it is tricky to
expose certain Objective-C types in Swift.  If those can be
consolidated, this mapping can go away.

See also "returnUIEnumForPreference:".

(2020.10)
*/
- (Emulation_FullType)
returnPreferenceForUIEnum:(UIPrefsTerminalEmulation_BaseEmulatorType)	aUIEnum
{
	Emulation_FullType		result = kEmulation_FullTypeXTerm256Color;
	
	
	switch (aUIEnum)
	{
	case UIPrefsTerminalEmulation_BaseEmulatorTypeNone:
		result = kEmulation_FullTypeDumb;
		break;
	
	case UIPrefsTerminalEmulation_BaseEmulatorTypeVt100:
		result = kEmulation_FullTypeVT100;
		break;
	
	case UIPrefsTerminalEmulation_BaseEmulatorTypeVt102:
		result = kEmulation_FullTypeVT102;
		break;
	
	case UIPrefsTerminalEmulation_BaseEmulatorTypeVt220:
		result = kEmulation_FullTypeVT220;
		break;
	
	case UIPrefsTerminalEmulation_BaseEmulatorTypeXterm:
	default:
		result = kEmulation_FullTypeXTerm256Color;
		break;
	}
	
	return result;
}// returnPreferenceForUIEnum:


/*!
Translate an emulator constant read from Preferences to the
equivalent constant in the UI.

TEMPORARY; this is primarily needed because it is tricky to
expose certain Objective-C types in Swift.  If those can be
consolidated, this mapping can go away.

See also "returnPreferenceForUIEnum:".

(2020.10)
*/
- (UIPrefsTerminalEmulation_BaseEmulatorType)
returnUIEnumForPreference:(Emulation_FullType)		aPreferenceValue
{
	UIPrefsTerminalEmulation_BaseEmulatorType		result = UIPrefsTerminalEmulation_BaseEmulatorTypeXterm;
	
	
	switch (aPreferenceValue)
	{
	case kEmulation_FullTypeDumb:
		result = UIPrefsTerminalEmulation_BaseEmulatorTypeNone;
		break;
	
	case kEmulation_FullTypeVT100:
		result = UIPrefsTerminalEmulation_BaseEmulatorTypeVt100;
		break;
	
	case kEmulation_FullTypeVT102:
		result = UIPrefsTerminalEmulation_BaseEmulatorTypeVt102;
		break;
	
	case kEmulation_FullTypeVT220:
		result = UIPrefsTerminalEmulation_BaseEmulatorTypeVt220;
		break;
	
	case kEmulation_FullTypeXTerm256Color:
	default:
		result = UIPrefsTerminalEmulation_BaseEmulatorTypeXterm;
		break;
	}
	
	return result;
}// returnUIEnumForPreference:


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
	Boolean					isDefaultAllTweaks = true; // initial value; see below
	
	
	// allow initialization of "isDefault..." values without triggers
	self.viewModel.defaultOverrideInProgress = YES;
	self.viewModel.disableWriteback = YES;
	
	// update base emulation type
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalEmulatorType;
		UInt32				preferenceValue = 0;
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
			self.viewModel.selectedBaseEmulatorType = [self returnUIEnumForPreference:preferenceValue]; // SwiftUI binding
			self.viewModel.isDefaultBaseEmulator = isDefault; // SwiftUI binding
		}
	}
	
	// update identity string
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalAnswerBackMessage;
		CFStringRef			preferenceValue = CFSTR("");
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
			self.viewModel.terminalIdentity = BRIDGE_CAST(preferenceValue, NSString*); // SwiftUI binding
			self.viewModel.isDefaultIdentity = isDefault; // SwiftUI binding
		}
	}
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminal24BitColorEnabled;
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
			self.viewModel.tweak24BitColorEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagITermGraphicsEnabled;
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
			self.viewModel.tweakITermGraphicsEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagVT100FixLineWrappingBug;
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
			self.viewModel.tweakVT100FixLineWrappingBugEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagSixelGraphicsEnabled;
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
			self.viewModel.tweakSixelGraphicsEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTerm256ColorsEnabled;
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
			self.viewModel.tweakXTerm256ColorsEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermBackgroundColorEraseEnabled;
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
			self.viewModel.tweakXTermBackgroundColorEraseEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermColorEnabled;
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
			self.viewModel.tweakXTermColorEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermGraphicsEnabled;
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
			self.viewModel.tweakXTermGraphicsEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermWindowAlterationEnabled;
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
			self.viewModel.tweakXTermWindowAlterationEnabled = preferenceValue; // SwiftUI binding
			isDefaultAllTweaks = (isDefaultAllTweaks && isDefault); // see below; used to update binding
		}
	}
	self.viewModel.isDefaultEmulationTweaks = isDefaultAllTweaks; // SwiftUI binding
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::TERMINAL);
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsTerminalEmulation_ActionHandling


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
	
	
	// update base emulation type
	{
		Preferences_Tag			preferenceTag = kPreferences_TagTerminalEmulatorType;
		Emulation_FullType		enumPrefValue = [self returnPreferenceForUIEnum:self.viewModel.selectedBaseEmulatorType];
		UInt32					preferenceValue = STATIC_CAST(enumPrefValue, UInt32);
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	
	// update identity string
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalAnswerBackMessage;
		CFStringRef			preferenceValue = BRIDGE_CAST(self.viewModel.terminalIdentity, CFStringRef);
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
		Preferences_Tag		preferenceTag = kPreferences_TagTerminal24BitColorEnabled;
		Boolean				preferenceValue = self.viewModel.tweak24BitColorEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagITermGraphicsEnabled;
		Boolean				preferenceValue = self.viewModel.tweakITermGraphicsEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagVT100FixLineWrappingBug;
		Boolean				preferenceValue = self.viewModel.tweakVT100FixLineWrappingBugEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagSixelGraphicsEnabled;
		Boolean				preferenceValue = self.viewModel.tweakSixelGraphicsEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTerm256ColorsEnabled;
		Boolean				preferenceValue = self.viewModel.tweakXTerm256ColorsEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermBackgroundColorEraseEnabled;
		Boolean				preferenceValue = self.viewModel.tweakXTermBackgroundColorEraseEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermColorEnabled;
		Boolean				preferenceValue = self.viewModel.tweakXTermColorEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermGraphicsEnabled;
		Boolean				preferenceValue = self.viewModel.tweakXTermGraphicsEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagXTermWindowAlterationEnabled;
		Boolean				preferenceValue = self.viewModel.tweakXTermWindowAlterationEnabled;
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
Deletes any local override for the given setting and
returns the Default value.

(2020.11)
*/
- (UIPrefsTerminalEmulation_BaseEmulatorType)
resetToDefaultGetSelectedBaseEmulatorType
{
	UIPrefsTerminalEmulation_BaseEmulatorType	result = UIPrefsTerminalEmulation_BaseEmulatorTypeXterm;
	Preferences_Tag								preferenceTag = kPreferences_TagTerminalEmulatorType;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		UInt32				preferenceValue = 0;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = [self returnUIEnumForPreference:STATIC_CAST(preferenceValue, Emulation_FullType)];
		}
	}
	
	return result;
}// resetToDefaultGetSelectedBaseEmulatorType


/*!
Deletes any local override for the given setting and
returns the Default value.

(2020.11)
*/
- (NSString*)
resetToDefaultGetIdentity
{
	NSString*			result = @"";
	Preferences_Tag		preferenceTag = kPreferences_TagTerminalAnswerBackMessage;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		CFStringRef			preferenceValue = CFSTR("");
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = [BRIDGE_CAST(preferenceValue, NSString*) copy];
		}
	}
	
	return result;
}// resetToDefaultGetIdentity


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweak24BitColorEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagTerminal24BitColorEnabled];
}// resetToDefaultGetTweak24BitColorEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakITermGraphicsEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagITermGraphicsEnabled];
}// resetToDefaultGetTweakITermGraphicsEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakVT100FixLineWrappingBugEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagVT100FixLineWrappingBug];
}// resetToDefaultGetTweakVT100FixLineWrappingBugEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakSixelGraphicsEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagSixelGraphicsEnabled];
}// resetToDefaultGetTweakSixelGraphicsEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakXTerm256ColorsEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagXTerm256ColorsEnabled];
}// resetToDefaultGetTweakXTerm256ColorsEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakXTermBackgroundColorEraseEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagXTermBackgroundColorEraseEnabled];
}// resetToDefaultGetTweakXTermBackgroundColorEraseEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakXTermColorEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagXTermColorEnabled];
}// resetToDefaultGetTweakXTermColorEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakXTermGraphicsEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagXTermGraphicsEnabled];
}// resetToDefaultGetTweakXTermGraphicsEnabled


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetTweakXTermWindowAlterationEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagXTermWindowAlterationEnabled];
}// resetToDefaultGetTweakXTermWindowAlterationEnabled


@end //}


#pragma mark -
@implementation PrefPanelTerminals_EmulationVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelTerminals_EmulationActionHandler*		actionHandler = [[PrefPanelTerminals_EmulationActionHandler alloc] init];
	NSView*											newView = [UIPrefsTerminalEmulation_ObjC makeView:actionHandler.viewModel];
	
	
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
initializeWithContext:(NSObject*)		aContext/* PrefPanelTerminals_EmulationActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelTerminals_EmulationActionHandler*		actionHandler = STATIC_CAST(aContext, PrefPanelTerminals_EmulationActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 460, 320); // somewhat arbitrary; see SwiftUI code/playground
	
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
		return [NSImage imageWithSystemSymbolName:@"chevron.right" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Terminals.Emulation";
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
	return NSLocalizedStringFromTable(@"Emulation", @"PrefPanelTerminals", @"the name of this panel");
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end //} PrefPanelTerminals_EmulationVC


#pragma mark -
@implementation PrefPanelTerminals_OptionsActionHandler //{


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
		_viewModel = [[UIPrefsTerminalOptions_Model alloc] initWithRunner:self]; // transfer ownership
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
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalLineWrap;
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
			self.viewModel.lineWrapEnabled = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultWrapLines = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagDataReceiveDoNotStripHighBit;
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
			self.viewModel.eightBitEnabled = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultEightBit = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalClearSavesLines;
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
			self.viewModel.saveLinesOnClearEnabled = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultSaveLinesOnClear = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapKeypadTopRowForVT220;
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
			// NOTE: this setting is stored inverted, versus its display checkbox
			self.viewModel.normalKeypadTopRowEnabled = (false == preferenceValue); // SwiftUI binding
			self.viewModel.isDefaultNormalKeypadTopRow = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagPageKeysControlLocalTerminal;
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
			self.viewModel.localPageKeysEnabled = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultLocalPageKeys = isDefault; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::TERMINAL);
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsTerminalOptions_ActionHandling


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
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalLineWrap;
		Boolean				preferenceValue = self.viewModel.lineWrapEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagDataReceiveDoNotStripHighBit;
		Boolean				preferenceValue = self.viewModel.eightBitEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalClearSavesLines;
		Boolean				preferenceValue = self.viewModel.saveLinesOnClearEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapKeypadTopRowForVT220;
		Boolean				preferenceValue = (false == self.viewModel.normalKeypadTopRowEnabled); // NOTE: this setting is stored inverted, versus its display checkbox
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagPageKeysControlLocalTerminal;
		Boolean				preferenceValue = self.viewModel.localPageKeysEnabled;
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
resetToDefaultGetWrapLines
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagTerminalLineWrap];
}// resetToDefaultGetWrapLines


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetEightBit
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagDataReceiveDoNotStripHighBit];
}// resetToDefaultGetEightBit


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetSaveLinesOnClear
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagTerminalClearSavesLines];
}// resetToDefaultGetSaveLinesOnClear


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetNormalKeypadTopRow
{
	// NOTE: this setting is stored inverted, versus its display checkbox
	return (NO == [self resetToDefaultGetFlagWithTag:kPreferences_TagMapKeypadTopRowForVT220]);
}// resetToDefaultGetNormalKeypadTopRow


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetLocalPageKeys
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagPageKeysControlLocalTerminal];
}// resetToDefaultGetLocalPageKeys


@end //}


#pragma mark -
@implementation PrefPanelTerminals_OptionsVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelTerminals_OptionsActionHandler*	actionHandler = [[PrefPanelTerminals_OptionsActionHandler alloc] init];
	NSView*										newView = [UIPrefsTerminalOptions_ObjC makeView:actionHandler.viewModel];
	
	
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
initializeWithContext:(NSObject*)		aContext/* PrefPanelTerminals_OptionsActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelTerminals_OptionsActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelTerminals_OptionsActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 460, 200); // somewhat arbitrary; see SwiftUI code/playground
	
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
		return [NSImage imageWithSystemSymbolName:@"chevron.right" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Terminals.Options";
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
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelTerminals", @"the name of this panel");
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end //} PrefPanelTerminals_OptionsVC


#pragma mark -
@implementation PrefPanelTerminals_ScreenActionHandler //{


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
		_viewModel = [[UIPrefsTerminalScreen_Model alloc] initWithRunner:self]; // transfer ownership
	}
	return self;
}// init


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
	Boolean					isDefault = false; // reused below
	
	
	// allow initialization of "isDefault..." values without triggers
	self.viewModel.defaultOverrideInProgress = YES;
	self.viewModel.disableWriteback = YES;
	
	// update columns
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenColumns;
		UInt16				preferenceValue = self.viewModel.widthValue;
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
			self.viewModel.widthValue = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultWidth = isDefault; // SwiftUI binding
		}
	}
	
	// update rows
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenRows;
		UInt16				preferenceValue = self.viewModel.heightValue;
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
			self.viewModel.heightValue = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultHeight = isDefault; // SwiftUI binding
		}
	}
	
	// update scrollback settings
	Boolean		isDefaultScrollbackValue = false;
	Boolean		isDefaultScrollbackType = false;
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenScrollbackRows;
		UInt32				preferenceValue = STATIC_CAST(self.viewModel.scrollbackValue, UInt32);
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
			self.viewModel.scrollbackValue = preferenceValue; // SwiftUI binding
			isDefaultScrollbackValue = isDefault; // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenScrollbackType;
		UInt16				preferenceValue = 0;
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
			self.viewModel.selectedScrollbackType = STATIC_CAST(preferenceValue, Terminal_ScrollbackType); // SwiftUI binding
			isDefaultScrollbackType = isDefault; // see below; used to update binding
		}
	}
	self.viewModel.isDefaultScrollback = ((isDefaultScrollbackValue) && (isDefaultScrollbackType)); // SwiftUI binding
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::TERMINAL);
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsTerminalScreen_ActionHandling


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
	
	
	// update columns
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenColumns;
		UInt16				preferenceValue = self.viewModel.widthValue;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	
	// update rows
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenRows;
		UInt16				preferenceValue = self.viewModel.heightValue;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	
	// update scrollback settings
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenScrollbackRows;
		UInt32				preferenceValue = STATIC_CAST(self.viewModel.scrollbackValue, UInt32);
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag				preferenceTag = kPreferences_TagTerminalScreenScrollbackType;
		Terminal_ScrollbackType		enumPrefValue = self.viewModel.selectedScrollbackType;
		UInt16						preferenceValue = STATIC_CAST(enumPrefValue, UInt16);
		Preferences_Result			prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																				sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
}// dataUpdated


/*!
Deletes any local override for the column count setting and
returns the Default value of the setting.

(2020.11)
*/
- (NSInteger)
resetToDefaultGetWidth
{
	NSInteger			result = 0;
	Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenColumns;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		UInt16				preferenceValue = 0;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = preferenceValue;
		}
	}
	
	return result;
}// resetToDefaultGetWidth


/*!
Deletes any local override for the row count setting and
returns the Default value of the setting.

(2020.11)
*/
- (NSInteger)
resetToDefaultGetHeight
{
	NSInteger			result = 0;
	Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenRows;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		UInt16				preferenceValue = 0;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = preferenceValue;
		}
	}
	
	return result;
}// resetToDefaultGetHeight


/*!
Deletes any local override for the scrollback row count setting
and returns the Default value of the setting.

(2020.11)
*/
- (NSInteger)
resetToDefaultGetScrollbackRowCount
{
	NSInteger			result = 0;
	Preferences_Tag		preferenceTag = kPreferences_TagTerminalScreenScrollbackRows;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		UInt32				preferenceValue = 0;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = preferenceValue;
		}
	}
	
	return result;
}// resetToDefaultGetScrollbackRowCount


/*!
Deletes any local override for the scrollback-type setting
and returns the Default value of the setting.

(2020.11)
*/
- (Terminal_ScrollbackType)
resetToDefaultGetScrollbackType
{
	Terminal_ScrollbackType		result = kTerminal_ScrollbackTypeDisabled;
	Preferences_Tag				preferenceTag = kPreferences_TagTerminalScreenScrollbackType;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		UInt16				preferenceValue = 0; // Terminal_ScrollbackType
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = STATIC_CAST(preferenceValue, Terminal_ScrollbackType);
		}
	}
	
	return result;
}// resetToDefaultGetScrollbackType


@end //}


#pragma mark -
@implementation PrefPanelTerminals_ScreenVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelTerminals_ScreenActionHandler*		actionHandler = [[PrefPanelTerminals_ScreenActionHandler alloc] init];
	NSView*										newView = [UIPrefsTerminalScreen_ObjC makeView:actionHandler.viewModel];
	
	
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
initializeWithContext:(NSObject*)		aContext/* PrefPanelTerminals_ScreenActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelTerminals_ScreenActionHandler*		actionHandler = STATIC_CAST(aContext, PrefPanelTerminals_ScreenActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 500, 200); // somewhat arbitrary; see SwiftUI code/playground
	
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
		return [NSImage imageWithSystemSymbolName:@"terminal" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Terminals.Screen";
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
	return NSLocalizedStringFromTable(@"Screen", @"PrefPanelTerminals", @"the name of this panel");
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end //} PrefPanelTerminals_ScreenVC

// BELOW IS REQUIRED NEWLINE TO END FILE
