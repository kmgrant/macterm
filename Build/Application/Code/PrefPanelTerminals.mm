/*!	\file PrefPanelTerminals.mm
	\brief Implements the Terminals panel of Preferences.
	
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

#import "PrefPanelTerminals.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Unix includes
#import <strings.h>

// Mac includes
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <BoundName.objc++.h>
#import <CocoaExtensions.objc++.h>
#import <ColorUtilities.h>
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


#pragma mark Internal Method Prototypes

/*!
The private class interface.
*/
@interface PrefPanelTerminals_EmulationViewManager (PrefPanelTerminals_EmulationViewManagerInternal) //{

	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelTerminals_OptionsViewManager (PrefPanelTerminals_OptionsViewManagerInternal) //{

	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelTerminals_ScreenViewManager (PrefPanelTerminals_ScreenViewManagerInternal) //{

	- (NSArray*)
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
PrefPanelTerminals_NewEmulationPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads emulation-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTerminalEmulatorType);
	tagList.push_back(kPreferences_TagTerminalAnswerBackMessage);
	tagList.push_back(kPreferences_TagVT100FixLineWrappingBug);
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
@implementation PrefPanelTerminals_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers = @[
										[[[PrefPanelTerminals_OptionsViewManager alloc] init] autorelease],
										[[[PrefPanelTerminals_EmulationViewManager alloc] init] autorelease],
										[[[PrefPanelTerminals_ScreenViewManager alloc] init] autorelease],
									];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Terminals"
										localizedName:NSLocalizedStringFromTable(@"Terminals", @"PrefPanelTerminals",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelTerminals"]
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


@end // PrefPanelTerminals_ViewManager


#pragma mark -
@implementation PrefPanelTerminals_BaseEmulatorValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeDumb
																description:NSLocalizedStringFromTable
																			(@"None (“Dumb”)", @"PrefPanelTerminals"/* table */,
																				@"emulator disabled")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeVT100
																description:NSLocalizedStringFromTable
																			(@"VT100", @"PrefPanelTerminals"/* table */,
																				@"emulator of VT100 terminal device")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeVT102
																description:NSLocalizedStringFromTable
																			(@"VT102", @"PrefPanelTerminals"/* table */,
																				@"emulator of VT102 terminal device")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeVT220
																description:NSLocalizedStringFromTable
																			(@"VT220", @"PrefPanelTerminals"/* table */,
																				@"emulator of VT220 terminal device")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kEmulation_FullTypeXTerm256Color
																description:NSLocalizedStringFromTable
																			(@"XTerm", @"PrefPanelTerminals"/* table */,
																				@"emulator of XTerm terminal program")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:kPreferences_TagTerminalEmulatorType
											contextManager:aContextMgr
											preferenceCType:kPreferenceValue_CTypeUInt32
											valueDescriptorArray:descriptorArray];
	if (nil != self)
	{
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
	[super dealloc];
}// dealloc


@end // PrefPanelTerminals_BaseEmulatorValue


#pragma mark -
@implementation PrefPanelTerminals_EmulationTweaksValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		NSMutableArray*		asMutableArray = [[[NSMutableArray alloc] init] autorelease];
		id					valueObject = nil;
		
		
		self->featureArray = [asMutableArray retain];
		
		// 24-bit color support
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagTerminal24BitColorEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"24-Bit Color (Millions)",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// VT100 line-wrapping bug
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagVT100FixLineWrappingBug
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"VT100 Fix Line Wrapping Bug",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm 256-color support
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTerm256ColorsEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm 256 Colors",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm BCE
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermBackgroundColorEraseEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Background Color Erase",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm Color
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermColorEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Color",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm Graphics
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermGraphicsEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Graphics Characters",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
		// XTerm Window Alteration
		valueObject = [[[PreferenceValue_Flag alloc]
						initWithPreferencesTag:kPreferences_TagXTermWindowAlterationEnabled
												contextManager:aContextMgr]
						autorelease];
		[[valueObject propertiesByKey] setObject:NSLocalizedStringFromTable(@"XTerm Window Alteration",
																			@"PrefPanelTerminals"/* table */,
																			@"description of terminal feature")
													forKey:@"description"];
		[asMutableArray addObject:valueObject];
		
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
	[featureArray release];
	[super dealloc];
}// dealloc


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


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSArray*)
featureArray
{
	return [[featureArray retain] autorelease];
}// featureArray


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = YES; // initially...
	
	
	for (UInt16 i = 0; i < [[self featureArray] count]; ++i)
	{
		PreferenceValue_Flag*	asValue = (PreferenceValue_Flag*)[[self featureArray] objectAtIndex:i];
		
		
		if (NO == [asValue isInherited])
		{
			result = NO;
			break;
		}
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
	for (UInt16 i = 0; i < [[self featureArray] count]; ++i)
	{
		PreferenceValue_Flag*	asValue = (PreferenceValue_Flag*)[[self featureArray] objectAtIndex:i];
		
		
		[asValue setNilPreferenceValue];
	}
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end // PrefPanelTerminals_EmulationTweaksValue


#pragma mark -
@implementation PrefPanelTerminals_EmulationViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelTerminalEmulationCocoa" delegate:self context:nullptr];
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
- (PrefPanelTerminals_BaseEmulatorValue*)
baseEmulator
{
	return [self->byKey objectForKey:@"baseEmulator"];
}// baseEmulator


/*!
Accessor.

(4.1)
*/
- (PrefPanelTerminals_EmulationTweaksValue*)
emulationTweaks
{
	return [self->byKey objectForKey:@"emulationTweaks"];
}// emulationTweaks


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_String*)
identity
{
	return [self->byKey objectForKey:@"identity"];
}// identity


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
	assert(nil != tweaksTableView);
	assert(nil != byKey);
	assert(nil != prefsMgr);
	
	// do not show highlighting in this table
	[tweaksTableView setSelectionHighlightStyle:NSTableViewSelectionHighlightStyleNone];
	
	// remember frame from XIB (it might be changed later)
	self->idealFrame = [aContainerView frame];
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PrefPanelTerminals_BaseEmulatorValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"baseEmulator"];
	[self->byKey setObject:[[[PrefPanelTerminals_EmulationTweaksValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"emulationTweaks"];
	[self->byKey setObject:[[[PreferenceValue_String alloc]
								initWithPreferencesTag:kPreferences_TagTerminalAnswerBackMessage
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"identity"];
	
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
	*outIdealSize = self->idealFrame.size;
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
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
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

(4.1)
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end // PrefPanelTerminals_EmulationViewManager


#pragma mark -
@implementation PrefPanelTerminals_EmulationViewManager (PrefPanelTerminals_EmulationViewManagerInternal)


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
				@"baseEmulator", @"emulationTweaks",
				@"identity",
			];
}// primaryDisplayBindingKeys


@end // PrefPanelTerminals_EmulationViewManager (PrefPanelTerminals_EmulationViewManagerInternal)


#pragma mark -
@implementation PrefPanelTerminals_OptionsViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelTerminalOptionsCocoa" delegate:self context:nullptr];
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
- (PreferenceValue_Flag*)
wrapLines
{
	return [self->byKey objectForKey:@"wrapLines"];
}// wrapLines


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
eightBit
{
	return [self->byKey objectForKey:@"eightBit"];
}// eightBit


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
saveLinesOnClear
{
	return [self->byKey objectForKey:@"saveLinesOnClear"];
}// saveLinesOnClear


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
normalKeypadTopRow
{
	return [self->byKey objectForKey:@"normalKeypadTopRow"];
}// normalKeypadTopRow


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
localPageKeys
{
	return [self->byKey objectForKey:@"localPageKeys"];
}// localPageKeys


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
	self->idealFrame = [aContainerView frame];
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagTerminalLineWrap
														contextManager:self->prefsMgr] autorelease]
					forKey:@"wrapLines"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagDataReceiveDoNotStripHighBit
														contextManager:self->prefsMgr] autorelease]
					forKey:@"eightBit"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagTerminalClearSavesLines
														contextManager:self->prefsMgr] autorelease]
					forKey:@"saveLinesOnClear"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagMapKeypadTopRowForVT220
														contextManager:self->prefsMgr
														inverted:YES] autorelease]
					forKey:@"normalKeypadTopRow"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagPageKeysControlLocalTerminal
														contextManager:self->prefsMgr] autorelease]
					forKey:@"localPageKeys"];
	
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
	*outIdealSize = self->idealFrame.size;
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
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
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

(4.1)
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end // PrefPanelTerminals_OptionsViewManager


#pragma mark -
@implementation PrefPanelTerminals_OptionsViewManager (PrefPanelTerminals_OptionsViewManagerInternal)


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
				@"wrapLines", @"eightBit",
				@"saveLinesOnClear", @"normalKeypadTopRow",
				@"localPageKeys",
			];
}// primaryDisplayBindingKeys


@end // PrefPanelTerminals_OptionsViewManager (PrefPanelTerminals_OptionsViewManagerInternal)


#pragma mark -
@implementation PrefPanelTerminals_ScrollbackValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		self->behaviorArray = [[[NSArray alloc] initWithObjects:
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeDisabled
															description:NSLocalizedStringFromTable
																		(@"Off", @"PrefPanelTerminals"/* table */,
																			@"scrollback disabled")]
									autorelease],
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeFixed
															description:NSLocalizedStringFromTable
																		(@"Fixed Size", @"PrefPanelTerminals"/* table */,
																			@"fixed-size scrollback")]
									autorelease],
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeUnlimited
															description:NSLocalizedStringFromTable
																		(@"Unlimited", @"PrefPanelTerminals"/* table */,
																			@"unlimited scrollback")]
									autorelease],
								[[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:kTerminal_ScrollbackTypeDistributed
															description:NSLocalizedStringFromTable
																		(@"Distributed", @"PrefPanelTerminals"/* table */,
																			@"distributed scrollback")]
									autorelease],
								nil] autorelease];
		self->behaviorObject = [[PreferenceValue_Number alloc]
									initWithPreferencesTag:kPreferences_TagTerminalScreenScrollbackType
															contextManager:aContextMgr
															preferenceCType:kPreferenceValue_CTypeUInt16];
		self->rowsObject = [[PreferenceValue_Number alloc]
									initWithPreferencesTag:kPreferences_TagTerminalScreenScrollbackRows
															contextManager:aContextMgr
															preferenceCType:kPreferenceValue_CTypeUInt32];
		
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
	[behaviorArray release];
	[behaviorObject release];
	[rowsObject release];
	[super dealloc];
}// dealloc


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
	[self didChangeValueForKey:@"rowsEnabled"];
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didChangeValueForKey:@"currentBehavior"];
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
	[self willChangeValueForKey:@"currentBehavior"];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	[self willChangeValueForKey:@"rowsEnabled"];
}// prefsContextWillChange:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSArray*)
behaviorArray
{
	return [[behaviorArray retain] autorelease];
}// behaviorArray


/*!
Accessor.

(4.1)
*/
- (id)
currentBehavior
{
	UInt32		currentType = [[self->behaviorObject numberStringValue] intValue];
	id			result = nil;
	
	
	for (UInt16 i = 0; i < [[self behaviorArray] count]; ++i)
	{
		PreferenceValue_IntegerDescriptor*	asInfo = (PreferenceValue_IntegerDescriptor*)
														[[self behaviorArray] objectAtIndex:i];
		
		
		if (currentType == [asInfo describedIntegerValue])
		{
			result = asInfo;
			break;
		}
	}
	return result;
}
- (void)
setCurrentBehavior:(id)		selectedObject
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentBehavior"];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	[self willChangeValueForKey:@"rowsEnabled"];
	
	if (nil == selectedObject)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		PreferenceValue_IntegerDescriptor*	asInfo = (PreferenceValue_IntegerDescriptor*)selectedObject;
		
		
		[self->behaviorObject setNumberStringValue:
								[[NSNumber numberWithInt:[asInfo describedIntegerValue]] stringValue]];
	}
	
	[self didChangeValueForKey:@"rowsEnabled"];
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didChangeValueForKey:@"currentBehavior"];
	[self didSetPreferenceValue];
}// setCurrentBehavior:


/*!
Accessor.

(4.1)
*/
- (BOOL)
rowsEnabled
{
	Terminal_ScrollbackType		currentBehavior = STATIC_CAST([[self->behaviorObject numberStringValue] intValue],
																Terminal_ScrollbackType);
	
	
	return (kTerminal_ScrollbackTypeFixed == currentBehavior);
}// rowsEnabled


/*!
Accessor.

(4.1)
*/
- (NSString*)
rowsNumberStringValue
{
	return [self->rowsObject numberStringValue];
}
- (void)
setRowsNumberStringValue:(NSString*)	aNumberString
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	
	if (nil == aNumberString)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		[self->rowsObject setNumberStringValue:aNumberString];
	}
	
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didSetPreferenceValue];
}// setRowsNumberStringValue:


#pragma mark Validators


/*!
Validates a number entered by the user, returning an appropriate
error (and a NO result) if the number is incorrect.

(4.1)
*/
- (BOOL)
validateRowsNumberStringValue:(id*/* NSString* */)		ioValue
error:(NSError**)									outError
{
	return [self->rowsObject validateNumberStringValue:ioValue error:outError];
}// validateRowsNumberStringValue:error:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = ([self->behaviorObject isInherited] && [self->rowsObject isInherited]);
	
	
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
	[self willChangeValueForKey:@"currentBehavior"];
	[self willChangeValueForKey:@"rowsNumberStringValue"];
	[self willChangeValueForKey:@"rowsEnabled"];
	[self->rowsObject setNilPreferenceValue];
	[self->behaviorObject setNilPreferenceValue];
	[self didChangeValueForKey:@"rowsEnabled"];
	[self didChangeValueForKey:@"rowsNumberStringValue"];
	[self didChangeValueForKey:@"currentBehavior"];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end // PrefPanelTerminals_ScrollbackValue


#pragma mark -
@implementation PrefPanelTerminals_ScreenViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelTerminalScreenCocoa" delegate:self context:nullptr];
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
- (PreferenceValue_Number*)
screenWidth
{
	return [self->byKey objectForKey:@"screenWidth"];
}// screenWidth


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
screenHeight
{
	return [self->byKey objectForKey:@"screenHeight"];
}// screenHeight


/*!
Accessor.

(4.1)
*/
- (PrefPanelTerminals_ScrollbackValue*)
scrollback
{
	return [self->byKey objectForKey:@"scrollback"];
}// scrollback


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
	self->idealFrame = [aContainerView frame];
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagTerminalScreenColumns
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16]
							autorelease]
					forKey:@"screenWidth"];
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagTerminalScreenRows
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16]
							autorelease]
					forKey:@"screenHeight"];
	[self->byKey setObject:[[[PrefPanelTerminals_ScrollbackValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"scrollback"];
	
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
	*outIdealSize = self->idealFrame.size;
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
	return [NSImage imageNamed:@"IconForPrefPanelTerminals"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
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

(4.1)
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
	return Quills::Prefs::TERMINAL;
}// preferencesClass


@end // PrefPanelTerminals_ScreenViewManager


#pragma mark -
@implementation PrefPanelTerminals_ScreenViewManager (PrefPanelTerminals_ScreenViewManagerInternal)


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
				@"screenWidth", @"screenHeight",
				@"scrollback",
			];
}// primaryDisplayBindingKeys


@end // PrefPanelTerminals_ScreenViewManager (PrefPanelTerminals_ScreenViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
