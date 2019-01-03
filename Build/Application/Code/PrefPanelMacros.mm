/*!	\file PrefPanelMacros.mm
	\brief Implements the Macros panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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

#import "PrefPanelMacros.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <iostream>
#import <sstream>
#import <vector>

// Unix includes
#import <strings.h>

// Mac includes
#import <CoreServices/CoreServices.h>

// library includes
#import <CFRetainRelease.h>
#import <Console.h>
#import <Localization.h>
#import <ListenerModel.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "MacroManager.h"
#import "Panel.h"
#import "Preferences.h"
#import "UIStrings.h"


#pragma mark Types

/*!
Private properties.
*/
@interface PrefPanelMacros_ViewManager () //{

// accessors
	@property (strong) PrefPanelMacros_MacroEditorViewManager*
	macroEditorViewManager;

@end //}


/*!
Private properties.
*/
@interface PrefPanelMacros_MacroEditorViewManager () //{

@end //}


/*!
The private class interface.
*/
@interface PrefPanelMacros_MacroEditorViewManager (PrefPanelMacros_MacroEditorViewManagerInternal) //{

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
PrefPanelMacros_NewTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	for (Preferences_Index i = 1; i <= kMacroManager_MaximumMacroSetSize; ++i)
	{
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKey, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroKeyModifiers, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroAction, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroContents, i));
	}
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods

#pragma mark -
@implementation PrefPanelMacros_MacroInfo //{


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

The class argument must be Quills::Prefs::MACRO_SET.

(2017.06)
*/
- (instancetype)
initWithDefaultContextInClass:(Quills::Prefs::Class)	aClass
{
	assert(Quills::Prefs::MACRO_SET == aClass);
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
macroIndexLabel
{
	return [[NSNumber numberWithUnsignedInteger:self.preferencesIndex] stringValue];
}// macroIndexLabel


/*!
Accessor.

(4.1)
*/
- (NSString*)
macroName
{
	NSString*	result = @"";
	
	
	if (0 != self.preferencesIndex)
	{
		Preferences_Tag const	macroNameIndexedTag = Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedMacroName, self.preferencesIndex);
		CFStringRef				nameCFString = nullptr;
		Preferences_Result		prefsResult = Preferences_ContextGetData
												(self.currentContext, macroNameIndexedTag,
													sizeof(nameCFString), &nameCFString,
													false/* search defaults */);
		
		
		if (kPreferences_ResultBadVersionDataNotAvailable == prefsResult)
		{
			// this is possible for indexed tags, as not all indexes
			// in the range may have data defined; ignore
			//Console_Warning(Console_WriteValue, "failed to retrieve macro name preference, error", prefsResult);
		}
		else if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to retrieve macro name preference, error", prefsResult);
		}
		else
		{
			result = BRIDGE_CAST(nameCFString, NSString*);
		}
	}
	
	return result;
}
- (void)
setMacroName:(NSString*)	aMacroName
{
	if (0 != self.preferencesIndex)
	{
		Preferences_Tag const	macroNameIndexedTag = Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedMacroName, self.preferencesIndex);
		CFStringRef				asCFStringRef = BRIDGE_CAST(aMacroName, CFStringRef);
		Preferences_Result		prefsResult = Preferences_ContextSetData
												(self.currentContext, macroNameIndexedTag,
													sizeof(asCFStringRef), &asCFStringRef);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set macro name preference, error", prefsResult);
		}
	}
}// setMacroName:


#pragma mark GenericPanelNumberedList_ItemBinding


/*!
Return strong reference to user interface string representing
numbered index in list.

Returns the "macroIndexLabel".

(4.1)
*/
- (NSString*)
numberedListIndexString
{
	return self.macroIndexLabel;
}// numberedListIndexString


/*!
Return or update user interface string for name of item in list.

Accesses the "macroName".

(4.1)
*/
- (NSString*)
numberedListItemName
{
	return self.macroName;
}
- (void)
setNumberedListItemName:(NSString*)		aName
{
	self.macroName = aName;
}// setNumberedListItemName:


@end //}


#pragma mark -
@implementation PrefPanelMacros_ViewManager //{


@synthesize macroEditorViewManager = _macroEditorViewManager;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	PrefPanelMacros_MacroEditorViewManager*		newViewManager = [[PrefPanelMacros_MacroEditorViewManager alloc] init];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Macros"
										localizedName:NSLocalizedStringFromTable(@"Macros", @"PrefPanelMacros",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelMacros"]
										master:self detailViewManager:newViewManager];
	if (nil != self)
	{
		_macroEditorViewManager = newViewManager;
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
	[_macroEditorViewManager release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelMacros_MacroInfo*)
selectedMacroInfo
{
	PrefPanelMacros_MacroInfo*		result = nil;
	NSUInteger						currentIndex = [self.listItemBindingIndexes firstIndex];
	
	
	if (NSNotFound != currentIndex)
	{
		result = [self.listItemBindings objectAtIndex:currentIndex];
	}
	
	return result;
}// selectedMacroInfo


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
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:1] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:2] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:3] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:4] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:5] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:6] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:7] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:8] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:9] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:10] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:11] autorelease],
							[[[PrefPanelMacros_MacroInfo alloc] initWithIndex:12] autorelease],
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
	aViewManager.headingTitleForNameColumn = NSLocalizedStringFromTable(@"Macro Name", @"PrefPanelMacros", @"the title for the item name column");
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
			for (PrefPanelMacros_MacroInfo* eachInfo in aViewManager.listItemBindings)
			{
				[eachInfo setCurrentContext:asContext];
			}
		}
	}
}// numberedListViewManager:didChangeFromDataSet:toDataSet:


@end //} PrefPanelMacros_ViewManager


#pragma mark -
@implementation PrefPanelMacros_ActionValue //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionSendTextVerbatim
																description:NSLocalizedStringFromTable
																			(@"Enter Text Verbatim", @"PrefPanelMacros"/* table */,
																				@"macro action: enter text verbatim")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionSendTextProcessingEscapes
																description:NSLocalizedStringFromTable
																			(@"Enter Text with Substitutions", @"PrefPanelMacros"/* table */,
																				@"macro action: enter text with substitutions")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionHandleURL
																description:NSLocalizedStringFromTable
																			(@"Open URL", @"PrefPanelMacros"/* table */,
																				@"macro action: open URL")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionNewWindowWithCommand
																description:NSLocalizedStringFromTable
																			(@"New Window with Command", @"PrefPanelMacros"/* table */,
																				@"macro action: new window with command")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionSelectMatchingWindow
																description:NSLocalizedStringFromTable
																			(@"Select Window by Title", @"PrefPanelMacros"/* table */,
																				@"macro action: select window by title")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionFindTextVerbatim
																description:NSLocalizedStringFromTable
																			(@"Find in Local Terminal Verbatim", @"PrefPanelMacros"/* table */,
																				@"macro action: find in local terminal verbatim")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ActionFindTextProcessingEscapes
																description:NSLocalizedStringFromTable
																			(@"Find in Local Terminal with Substitutions", @"PrefPanelMacros"/* table */,
																				@"macro action: find in local terminal with substitutions")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:0L/* set externally later */
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


@end //} PrefPanelMacros_ActionValue


#pragma mark -
@implementation PrefPanelMacros_InvokeWithValue //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	Boolean const	kFlagIsVirtualKey = true;
	Boolean const	kFlagIsRealKey = false;
	
	
	// create a sentinel object (exact values do not matter, the only
	// thing that is ever used is the pointer identity); this allows
	// the interface to decide when to return a key-character binding
	self.placeholderDescriptor = [[[PreferenceValue_IntegerDescriptor alloc]
									initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsRealKey, 0/* exact value is never used */), UInt32)
															description:NSLocalizedStringFromTable
																		(@"Ordinary Character", @"PrefPanelMacros"/* table */,
																			@"key: ordinary character")]
									autorelease];
	
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									self.placeholderDescriptor,
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_Delete/* 0x33 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌫ Backward Delete", @"PrefPanelMacros"/* table */,
																				@"key: backward delete")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_ForwardDelete/* 0x75 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌦ Forward Delete", @"PrefPanelMacros"/* table */,
																				@"key: forward delete")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_Home/* 0x73 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"↖︎ Home", @"PrefPanelMacros"/* table */,
																				@"key: home")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_End/* 0x77 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"↘︎ End", @"PrefPanelMacros"/* table */,
																				@"key: end")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_PageUp/* 0x74 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇞ Page Up", @"PrefPanelMacros"/* table */,
																				@"key: page up")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_PageDown/* 0x79 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇟ Page Down", @"PrefPanelMacros"/* table */,
																				@"key: page down")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_UpArrow/* 0x7E */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇡ Up Arrow", @"PrefPanelMacros"/* table */,
																				@"key: up arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_DownArrow/* 0x7D */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇣ Down Arrow", @"PrefPanelMacros"/* table */,
																				@"key: down arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_LeftArrow/* 0x7B */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇠ Left Arrow", @"PrefPanelMacros"/* table */,
																				@"key: left arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_RightArrow/* 0x7C */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⇢ Right Arrow", @"PrefPanelMacros"/* table */,
																				@"key: right arrow")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_ANSI_KeypadClear/* 0x47 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌧ Clear", @"PrefPanelMacros"/* table */,
																				@"key: clear")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_Escape/* 0x35 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⎋ Escape", @"PrefPanelMacros"/* table */,
																				@"key: escape")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_Return/* 0x24 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"↩︎ Return", @"PrefPanelMacros"/* table */,
																				@"key: return")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_ANSI_KeypadEnter/* 0x4C */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"⌤ Enter", @"PrefPanelMacros"/* table */,
																				@"key: enter")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F1/* 0x7A */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F1 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F1")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F2/* 0x78 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F2 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F2")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F3/* 0x63 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F3 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F3")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F4/* 0x76 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F4 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F4")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F5/* 0x60 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F5 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F5")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F6/* 0x61 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F6 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F6")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F7/* 0x62 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F7 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F7")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F8/* 0x64 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F8 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F8")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F9/* 0x65 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F9 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F9")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F10/* 0x6D */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F10 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F10")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F11/* 0x67 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F11 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F11")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F12/* 0x6F */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F12 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F12")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F13/* 0x69 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F13 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F13")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F14/* 0x6B */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F14 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F14")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F15/* 0x71 */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F15 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F15")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:STATIC_CAST(MacroManager_MakeKeyID(kFlagIsVirtualKey, kVK_F16/* 0x6A */), UInt32)
																description:NSLocalizedStringFromTable
																			(@"F16 Function Key", @"PrefPanelMacros"/* table */,
																				@"key: F16")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:0L/* set externally later */
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


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
currentOrdinaryCharacter
{
	PreferenceValue_IntegerDescriptor*		asDesc = STATIC_CAST(self.currentValueDescriptor, PreferenceValue_IntegerDescriptor*);
	NSString*								result = @"";
	
	
	// most values map to specific virtual keys; if the current
	// setting is the “ordinary character” entry however, then
	// a non-empty value should be returned for this field
	if (self.placeholderDescriptor == asDesc)
	{
		// read the preferences directly to find an appropriate string
		Preferences_Tag			keyTag = self.preferencesTag; // set externally
		UInt32					intResult = 0;
		Preferences_Result		prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, keyTag,
																			sizeof(intResult), &intResult,
																			true/* search defaults */);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to read macro ordinary-key preference; error", prefsResult);
		}
		else
		{
			MacroManager_KeyID		asKeyID = STATIC_CAST(intResult, MacroManager_KeyID);
			
			
			if (false == MacroManager_KeyIDIsVirtualKey(asKeyID))
			{
				// current preference contains a real-key mapping;
				// return the corresponding character as a string
				unichar		stringData[] = { MacroManager_KeyIDKeyCode(asKeyID) };
				
				
				result = [NSString stringWithCharacters:stringData length:1];
			}
		}
	}
	
	return result;
}
- (void)
setCurrentOrdinaryCharacter:(NSString*)		aKeyString
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentOrdinaryCharacter"];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	[self willChangeValueForKey:@"isOrdinaryCharacter"];
	
	if (nil == aKeyString)
	{
		[self setNilPreferenceValue];
	}
	else if (aKeyString.length > 1)
	{
		Console_Warning(Console_WriteValueCFString, "ignoring character mapping with length greater than 1", BRIDGE_CAST(aKeyString, CFStringRef));
	}
	else
	{
		// TEMPORARY; this setting is not really portable across
		// different languages and keyboard types; at some point
		// a character code will not be representable
		unichar					characterCode = [aKeyString characterAtIndex:0];
		Preferences_Tag			keyTag = self.preferencesTag; // set externally
		MacroManager_KeyID		keyID = MacroManager_MakeKeyID(false/* is virtual key */, characterCode);
		UInt32					intValue = STATIC_CAST(keyID, UInt32);
		Preferences_Result		prefsResult = Preferences_ContextSetData(self.prefsMgr.currentContext, keyTag,
																			sizeof(intValue), &intValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to set macro ordinary-key preference; error", prefsResult);
		}
		
		// update the descriptor (represented by a menu item selection)
		// to indicate that the macro maps to an ordinary key; the
		// associated integer value MUST match the preference setting
		// to prevent future bindings from clearing the setting
		STATIC_CAST(self.placeholderDescriptor, PreferenceValue_IntegerDescriptor*).describedIntegerValue = intValue;
		self.currentValueDescriptor = self.placeholderDescriptor;
	}
	
	[self didChangeValueForKey:@"isOrdinaryCharacter"];
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didChangeValueForKey:@"currentOrdinaryCharacter"];
	[self didSetPreferenceValue];
}// setCurrentOrdinaryCharacter:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isOrdinaryCharacter
{
	BOOL	result = (NO == [[self currentOrdinaryCharacter] isEqualToString:@""]);
	
	
	return result;
}// isOrdinaryCharacter


#pragma mark NSKeyValueObserving

/*!
Specifies the names of properties that trigger updates
to the given property.  (Called by the Key-Value Coding
mechanism as needed.)

Usually this can be achieved by sending update messages
for multiple properties from a single set-method.  This
approach is used when the set-method is not easily
accessible.

(4.1)
*/
+ (NSSet*)
keyPathsForValuesAffectingValueForKey:(NSString*)	aKey
{
	NSSet*	result = nil;
	
	
	// return other properties that (if changed) should cause
	// the specified property to be updated
	if ([aKey isEqualToString:@"isOrdinaryCharacter"])
	{
		// since the superclass implements "currentValueDescriptor",
		// it is easier to set up the relationship here (this causes
		// the ordinary-character field to be enabled/disabled as
		// the menu selection changes to/from “Ordinary Character”)
		result = [NSSet setWithObjects:@"currentValueDescriptor", nil];
	}
	else
	{
		result = [super keyPathsForValuesAffectingValueForKey:aKey];
	}
	
	return result;
}// keyPathsForValuesAffectingValueForKey:


@end //} PrefPanelMacros_InvokeWithValue


#pragma mark -
@implementation PrefPanelMacros_ModifiersValue //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskCommand
																description:NSLocalizedStringFromTable
																			(@"⌘", @"PrefPanelMacros"/* table */,
																				@"Command key modifier")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskOption
																description:NSLocalizedStringFromTable
																			(@"⌥", @"PrefPanelMacros"/* table */,
																				@"Option key modifier")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskControl
																description:NSLocalizedStringFromTable
																			(@"⌃", @"PrefPanelMacros"/* table */,
																				@"Control key modifier")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kMacroManager_ModifierKeyMaskShift
																description:NSLocalizedStringFromTable
																			(@"⇧", @"PrefPanelMacros"/* table */,
																				@"Shift key modifier")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:0L/* set externally later */
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


@end //} PrefPanelMacros_ModifiersValue


#pragma mark -
@implementation PrefPanelMacros_MacroEditorViewManager //{


@synthesize bindControlKeyPad = _bindControlKeyPad;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelMacrosCocoa" delegate:self context:nullptr];
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


#pragma mark Actions


/*!
Displays the Control Keys palette to choose a key that
can be transformed into an appropriate substitution
sequence (for instance, inserting "\004" for a ⌃D).

(4.1)
*/
- (IBAction)
performInsertControlKeyCharacter:(id)	sender
{
#pragma unused(sender)
	// toggle display of Control Keys palette (while enabled,
	// responder invokes "controlKeypadSentCharacterCode:")
	if (self.bindControlKeyPad)
	{
		Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self);
	}
	else
	{
		Keypads_RemoveResponder(kKeypads_WindowTypeControlKeys, self);
	}
}// performInsertControlKeyCharacter:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelMacros_ActionValue*)
macroAction
{
	return [self->byKey objectForKey:@"macroAction"];
}// macroAction


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_String*)
macroContents
{
	return [self->byKey objectForKey:@"macroContents"];
}// macroContents


/*!
Accessor.

(4.1)
*/
- (PrefPanelMacros_InvokeWithValue*)
macroKey
{
	return [self->byKey objectForKey:@"macroKey"];
}// macroKey


/*!
Accessor.

(4.1)
*/
- (PrefPanelMacros_ModifiersValue*)
macroModifiers
{
	return [self->byKey objectForKey:@"macroModifiers"];
}// macroModifiers


#pragma mark Keypads_SetResponder() Interface


/*!
Received when the Control Keys keypad is hidden by the
user while this class instance is the current keypad
responder (as set by Keypads_SetResponder()).

This handles the event by updating the button state.

(4.1)
*/
- (void)
controlKeypadHidden
{
	self.bindControlKeyPad = NO;
}// controlKeypadHidden


/*!
Received when the Control Keys keypad is used to select
a control key while this class instance is the current
keypad responder (as set by Keypads_SetResponder()).

This handles the event by inserting an appropriate macro
key sequence into the content field.  If the field is
currently being edited then the actively-edited string
is changed instead.

(4.1)
*/
- (void)
controlKeypadSentCharacterCode:(NSNumber*)	asciiChar
{
	// NOTE: the base string cannot be "self.macroContents.stringValue"
	// because the corresponding text field might be in editing mode
	// (and the binding would not be up-to-date); therefore the base
	// string comes directly from the field itself
	NSString*	originalContents = self->contentsField.stringValue;
	NSString*	newContents = nil;
	NSUInteger	charValue = [asciiChar unsignedIntegerValue];
	
	
	// insert the corresponding key value into the field
	if (0x08 == charValue)
	{
		// this is the sequence for “backspace”, which has a more
		// convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\b", originalContents];
	}
	else if (0x09 == charValue)
	{
		// this is the sequence for “horizontal tab”, which has a
		// more convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\t", originalContents];
	}
	else if (0x0D == charValue)
	{
		// this is the sequence for “carriage return”, which has
		// a more convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\r", originalContents];
	}
	else if (0x1B == charValue)
	{
		// this is the sequence for “escape”, which has a more
		// convenient and concise representation
		newContents = [NSString stringWithFormat:@"%@\\e", originalContents];
	}
	else
	{
		// generate an escape sequence of the form recognized by
		// the Macro Manager’s “text with substitutions” parser
		char const*			leadingDigits = ((charValue < 8) ? "00" : "0");
		std::ostringstream	stringStream;
		std::string			stringValue;
		
		
		stringStream << std::oct << charValue;
		stringValue = stringStream.str();
		newContents = [NSString stringWithFormat:@"%@\\%s%s", originalContents, leadingDigits, stringValue.c_str()];
	}
	
	self.macroContents.stringValue = newContents;
}// controlKeypadSentCharacterCode:


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
#pragma unused(aViewManager)
	assert(nil != byKey);
	assert(nil != prefsMgr);
	assert(nil != contentsField);
	
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
#pragma unused(aViewManager)
	if (kPanel_VisibilityDisplayed != aVisibility)
	{
		// if the Control Keys palette was displayed to help edit macros,
		// remove the binding when the panel disappears (otherwise the
		// user could click a key later and inadvertently change a macro
		// in the invisible panel)
		Keypads_RemoveResponder(kKeypads_WindowTypeControlKeys, self);
	}
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
			self.macroAction.prefsMgr.currentContext = asPrefsContext;
			self.macroContents.prefsMgr.currentContext = asPrefsContext;
			self.macroKey.prefsMgr.currentContext = asPrefsContext;
			self.macroModifiers.prefsMgr.currentContext = asPrefsContext;
		}
		// user may or may not have selected a different macro
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
	return [NSImage imageNamed:@"IconForPrefPanelMacros"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Macros";
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
	return NSLocalizedStringFromTable(@"Macros", @"PrefPanelMacros", @"the name of this panel");
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
	return Quills::Prefs::MACRO_SET;
}// preferencesClass


@end //} PrefPanelMacros_MacroEditorViewManager


#pragma mark -
@implementation PrefPanelMacros_MacroEditorViewManager (PrefPanelMacros_MacroEditorViewManagerInternal) //{


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
	if (nil == self.macroAction)
	{
		[self->byKey setObject:[[[PrefPanelMacros_ActionValue alloc]
									initWithContextManager:self->prefsMgr] autorelease]
						forKey:@"macroAction"];
	}
	self.macroAction.preferencesTag = Preferences_ReturnTagVariantForIndex
										(kPreferences_TagIndexedMacroAction, anIndex);
	
	if (nil == self.macroContents)
	{
		[self->byKey setObject:[[[PreferenceValue_String alloc]
									initWithPreferencesTag:Preferences_ReturnTagVariantForIndex
															(kPreferences_TagIndexedMacroContents, anIndex)
															contextManager:self->prefsMgr] autorelease]
						forKey:@"macroContents"];
	}
	self.macroContents.preferencesTag = Preferences_ReturnTagVariantForIndex
											(kPreferences_TagIndexedMacroContents, anIndex);
	
	if (nil == self.macroKey)
	{
		[self->byKey setObject:[[[PrefPanelMacros_InvokeWithValue alloc]
									initWithContextManager:self->prefsMgr] autorelease]
						forKey:@"macroKey"];
	}
	self.macroKey.preferencesTag = Preferences_ReturnTagVariantForIndex
									(kPreferences_TagIndexedMacroKey, anIndex);
	
	if (nil == self.macroModifiers)
	{
		[self->byKey setObject:[[[PrefPanelMacros_ModifiersValue alloc]
									initWithContextManager:self->prefsMgr] autorelease]
						forKey:@"macroModifiers"];
	}
	self.macroModifiers.preferencesTag = Preferences_ReturnTagVariantForIndex
											(kPreferences_TagIndexedMacroKeyModifiers, anIndex);
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
	return @[@"macroAction", @"macroContents", @"macroKey", @"macroModifiers"];
}// primaryDisplayBindingKeys


@end //} PrefPanelMacros_MacroEditorViewManager (PrefPanelMacros_MacroEditorViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
