/*!	\file PrefPanelMacros.mm
	\brief Implements the Macros panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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
#import <Carbon/Carbon.h> // for kVK... virtual key codes (TEMPORARY; deprecated)
@import CoreServices;

// library includes
#import <CFRetainRelease.h>
#import <Console.h>
#import <Localization.h>
#import <ListenerModel.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "MacroManager.h"
#import "Panel.h"
#import "Preferences.h"
#import "UIStrings.h"
#import "UTF8Decoder.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>


#pragma mark Types

/*!
Implements the macro-editing panel.
*/
@interface PrefPanelMacros_EditorVC : Panel_ViewManager< Panel_Delegate,
															PrefsWindow_PanelInterface > //{

@end //}


/*!
An object for user interface bindings; prior to use,
set a (Macro-Set-class) Preferences Context and a
particular macro index.  The macro name can be
changed, causing the corresponding preferences (the
macro name) to be updated.
*/
@interface PrefPanelMacros_MacroInfo : PrefsContextManager_Object< GenericPanelNumberedList_ItemBinding > //{

// initializers
	- (instancetype)
	init NS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithDefaultContextInClass:(Quills::Prefs::Class)_ NS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithIndex:(Preferences_Index)_ NS_DESIGNATED_INITIALIZER;

// accessors
	@property (readonly) Preferences_Index
	preferencesIndex;
	@property (readonly) NSString*
	macroIndexLabel;
	@property (strong) NSString*
	macroName;

@end //}


/*!
Private properties.
*/
@interface PrefPanelMacros_ViewManager () //{

// accessors
	@property (strong) PrefPanelMacros_EditorVC*
	macroEditorVC;
	@property (readonly) PrefPanelMacros_MacroInfo*
	selectedMacroInfo;

@end //}


/*!
Implements SwiftUI interaction for the macro-editing panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelMacros_EditorActionHandler : NSObject< Keypads_ControlKeyResponder,
															UIPrefsMacroEditor_ActionHandling > //{

// new methods
	- (void)
	configureForIndex:(Preferences_Index)_;
	- (void)
	setPreference:(MacroManager_KeyID*)_
	fromUIEnum:(MacroManager_KeyBinding)_
	ordinaryCharacter:(NSString*)_;
	- (void)
	setUIEnum:(MacroManager_KeyBinding*)_
	ordinaryCharacter:(NSString**)_
	fromPreference:(MacroManager_KeyID)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (assign) Preferences_Tag
	indexedTagMacroAction; // MUST be set by "configureForIndex:" only
	@property (assign) Preferences_Tag
	indexedTagMacroContents; // MUST be set by "configureForIndex:" only
	@property (assign) Preferences_Tag
	indexedTagMacroKey; // MUST be set by "configureForIndex:" only
	@property (assign) Preferences_Tag
	indexedTagMacroModifiers; // MUST be set by "configureForIndex:" only
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsMacroEditor_Model*
	viewModel;

@end //}


/*!
Private properties.
*/
@interface PrefPanelMacros_EditorVC () //{

// accessors
	@property (strong) PrefPanelMacros_EditorActionHandler*
	actionHandler;
	@property (assign) CGRect
	idealFrame;

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
Return or update user interface icon for item in list.

This is used to show the presence or absence of a macro.

(2020.04)
*/
- (NSImage*)
numberedListItemIconImage
{
	NSImage*	result = nil;
	
	
	if (@available(macOS 11.0, *))
	{
		result = [NSImage imageWithSystemSymbolName:@"command" accessibilityDescription:@""];
	}
	else
	{
		result = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnPrefPanelMacrosIconFilenameNoExtension(), NSString*)];
	}
	
	if (0 != self.preferencesIndex)
	{
		// UNIMPLEMENTED; if icon is assigned to a macro, use it
	}
	
	return result;
}// numberedListItemIconImage


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


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	PrefPanelMacros_EditorVC*	newViewManager = [[PrefPanelMacros_EditorVC alloc] init];
	NSString*					panelName = NSLocalizedStringFromTable(@"Macros", @"PrefPanelMacros",
																		@"the name of this panel");
	NSImage*					panelIcon = nil;
	
	
	if (@available(macOS 11.0, *))
	{
		panelIcon = [NSImage imageWithSystemSymbolName:@"command" accessibilityDescription:self.panelName];
	}
	else
	{
		panelIcon = [NSImage imageNamed:@"IconForPrefPanelMacros"];
	}
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Macros"
										localizedName:panelName
										localizedIcon:panelIcon
										master:self detailViewManager:newViewManager];
	if (nil != self)
	{
		//_macroEditorViewManager = newViewManager;
		_macroEditorVC = newViewManager;
	}
	return self;
}// init


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
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:1],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:2],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:3],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:4],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:5],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:6],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:7],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:8],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:9],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:10],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:11],
							[[PrefPanelMacros_MacroInfo alloc] initWithIndex:12],
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
@implementation PrefPanelMacros_EditorActionHandler //{


/*!
Designated initializer.

(2021.02)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		_prefsMgr = nil; // see "panelViewManager:initializeWithContext:"
		_viewModel = [[UIPrefsMacroEditor_Model alloc] initWithRunner:self]; // transfer ownership
	}
	return self;
}// init


#pragma mark New Methods


/*!
Creates and/or configures index-dependent internal objects used
for value bindings.

NOTE:	This does NOT trigger any will/did callbacks because it
		is typically called in places where these are already
		being called.  Callbacks are necessary to ensure that
		user interface elements are updated, for instance.

(2021.02)
*/
- (void)
configureForIndex:(Preferences_Index)	anIndex
{
	self.indexedTagMacroAction = Preferences_ReturnTagVariantForIndex
									(kPreferences_TagIndexedMacroAction, anIndex);
	self.indexedTagMacroContents = Preferences_ReturnTagVariantForIndex
									(kPreferences_TagIndexedMacroContents, anIndex);
	self.indexedTagMacroKey = Preferences_ReturnTagVariantForIndex
								(kPreferences_TagIndexedMacroKey, anIndex);
	self.indexedTagMacroModifiers = Preferences_ReturnTagVariantForIndex
									(kPreferences_TagIndexedMacroKeyModifiers, anIndex);
}// configureForIndex:


/*!
Translate from UI-specified key code constant to the equivalent
key ID constant stored in Preferences.

If the return value is "kMacroManager_KeyBindingOrdinaryCharacter",
the ordinary character value applies.

See also "returnUIEnumForPreference:".

(2021.02)
*/
- (void)
setPreference:(MacroManager_KeyID*)		outKeyID
fromUIEnum:(MacroManager_KeyBinding)	aUIEnum
ordinaryCharacter:(NSString*)			aUnicodeSequenceIfOrdinaryKeyBinding
{
	UInt16		virtualKeyCode = 0;
	
	
	switch (aUIEnum)
	{
	case kMacroManager_KeyBindingOrdinaryCharacter:
		{
			UnicodeScalarValue		characterCode = StringUtilities_ReturnUnicodeSymbol(BRIDGE_CAST(aUnicodeSequenceIfOrdinaryKeyBinding, CFStringRef));
			
			
			if (kUTF8Decoder_InvalidUnicodeCodePoint == characterCode)
			{
				if (aUnicodeSequenceIfOrdinaryKeyBinding.length > 0)
				{
					Console_Warning(Console_WriteValueCFString, "failed to find a single Unicode value from string", BRIDGE_CAST(aUnicodeSequenceIfOrdinaryKeyBinding, CFStringRef));
				}
				*outKeyID = MacroManager_MakeKeyID(false/* is virtual key */, 0);
			}
			else
			{
				*outKeyID = MacroManager_MakeKeyID(false/* is virtual key */, characterCode);
			}
		}
		break;
	
	case kMacroManager_KeyBindingBackwardDelete:
		virtualKeyCode = kVK_Delete;
		break;
	
	case kMacroManager_KeyBindingForwardDelete:
		virtualKeyCode = kVK_ForwardDelete;
		break;
	
	case kMacroManager_KeyBindingHome:
		virtualKeyCode = kVK_Home;
		break;
	
	case kMacroManager_KeyBindingEnd:
		virtualKeyCode = kVK_End;
		break;
	
	case kMacroManager_KeyBindingPageUp:
		virtualKeyCode = kVK_PageUp;
		break;
	
	case kMacroManager_KeyBindingPageDown:
		virtualKeyCode = kVK_PageDown;
		break;
	
	case kMacroManager_KeyBindingUpArrow:
		virtualKeyCode = kVK_UpArrow;
		break;
	
	case kMacroManager_KeyBindingDownArrow:
		virtualKeyCode = kVK_DownArrow;
		break;
	
	case kMacroManager_KeyBindingLeftArrow:
		virtualKeyCode = kVK_LeftArrow;
		break;
	
	case kMacroManager_KeyBindingRightArrow:
		virtualKeyCode = kVK_RightArrow;
		break;
	
	case kMacroManager_KeyBindingClear:
		virtualKeyCode = kVK_ANSI_KeypadClear;
		break;
	
	case kMacroManager_KeyBindingEscape:
		virtualKeyCode = kVK_Escape;
		break;
	
	case kMacroManager_KeyBindingReturn:
		virtualKeyCode = kVK_Return;
		break;
	
	case kMacroManager_KeyBindingEnter:
		virtualKeyCode = kVK_ANSI_KeypadEnter;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF1:
		virtualKeyCode = kVK_F1;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF2:
		virtualKeyCode = kVK_F2;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF3:
		virtualKeyCode = kVK_F3;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF4:
		virtualKeyCode = kVK_F4;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF5:
		virtualKeyCode = kVK_F5;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF6:
		virtualKeyCode = kVK_F6;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF7:
		virtualKeyCode = kVK_F7;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF8:
		virtualKeyCode = kVK_F8;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF9:
		virtualKeyCode = kVK_F9;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF10:
		virtualKeyCode = kVK_F10;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF11:
		virtualKeyCode = kVK_F11;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF12:
		virtualKeyCode = kVK_F12;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF13:
		virtualKeyCode = kVK_F13;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF14:
		virtualKeyCode = kVK_F14;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF15:
		virtualKeyCode = kVK_F15;
		break;
	
	case kMacroManager_KeyBindingFunctionKeyF16:
		virtualKeyCode = kVK_F16;
		break;
	
	default:
		Console_Warning(Console_WriteValue, "macro editor: unexpected key binding enum", aUIEnum);
		break;
	}
	
	if (kMacroManager_KeyBindingOrdinaryCharacter != aUIEnum)
	{
		*outKeyID = MacroManager_MakeKeyID(true/* is virtual key */, virtualKeyCode);
	}
}// setPreference:fromUIEnum:ordinaryCharacter:


/*!
Translate a key binding constant read from Preferences to the
equivalent values in the UI.

If the return value is "kMacroManager_KeyBindingOrdinaryCharacter"
then the "ordinaryCharacter:" parameter is filled in with the
additional binding extracted from the key ID (this can be used to
set "self.viewModel.macroOrdinaryKey").  Otherwise this is set to
an empty string.

See also "setPreference:fromUIEnum:ordinaryCharacter:".

(2021.02)
*/
- (void)
setUIEnum:(MacroManager_KeyBinding*)	outBindingPtr
ordinaryCharacter:(NSString**)			outUnicodeSequenceIfOrdinaryKeyBinding
fromPreference:(MacroManager_KeyID)		aKeyID
{
	if (MacroManager_KeyIDIsVirtualKey(aKeyID))
	{
		UInt16		keyCode = MacroManager_KeyIDKeyCode(aKeyID);
		
		
		*outUnicodeSequenceIfOrdinaryKeyBinding = @"";
		switch (keyCode)
		{
		case kVK_Delete:
			*outBindingPtr = kMacroManager_KeyBindingBackwardDelete;
			break;
		
		case kVK_ForwardDelete:
			*outBindingPtr = kMacroManager_KeyBindingForwardDelete;
			break;
		
		case kVK_Home:
			*outBindingPtr = kMacroManager_KeyBindingHome;
			break;
		
		case kVK_End:
			*outBindingPtr = kMacroManager_KeyBindingEnd;
			break;
		
		case kVK_PageUp:
			*outBindingPtr = kMacroManager_KeyBindingPageUp;
			break;
		
		case kVK_PageDown:
			*outBindingPtr = kMacroManager_KeyBindingPageDown;
			break;
		
		case kVK_UpArrow:
			*outBindingPtr = kMacroManager_KeyBindingUpArrow;
			break;
		
		case kVK_DownArrow:
			*outBindingPtr = kMacroManager_KeyBindingDownArrow;
			break;
		
		case kVK_LeftArrow:
			*outBindingPtr = kMacroManager_KeyBindingLeftArrow;
			break;
		
		case kVK_RightArrow:
			*outBindingPtr = kMacroManager_KeyBindingRightArrow;
			break;
		
		case kVK_ANSI_KeypadClear:
			*outBindingPtr = kMacroManager_KeyBindingClear;
			break;
		
		case kVK_Escape:
			*outBindingPtr = kMacroManager_KeyBindingEscape;
			break;
		
		case kVK_Return:
			*outBindingPtr = kMacroManager_KeyBindingReturn;
			break;
		
		case kVK_ANSI_KeypadEnter:
			*outBindingPtr = kMacroManager_KeyBindingEnter;
			break;
		
		case kVK_F1:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF1;
			break;
		
		case kVK_F2:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF2;
			break;
		
		case kVK_F3:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF3;
			break;
		
		case kVK_F4:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF4;
			break;
		
		case kVK_F5:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF5;
			break;
		
		case kVK_F6:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF6;
			break;
		
		case kVK_F7:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF7;
			break;
		
		case kVK_F8:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF8;
			break;
		
		case kVK_F9:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF9;
			break;
		
		case kVK_F10:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF10;
			break;
		
		case kVK_F11:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF11;
			break;
		
		case kVK_F12:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF12;
			break;
		
		case kVK_F13:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF13;
			break;
		
		case kVK_F14:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF14;
			break;
		
		case kVK_F15:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF15;
			break;
		
		case kVK_F16:
			*outBindingPtr = kMacroManager_KeyBindingFunctionKeyF16;
			break;
		
		default:
			Console_Warning(Console_WriteValue, "macro editor: unexpected virtual key code", keyCode);
			break;
		}
	}
	else
	{
		UInt16		keyEquivalentUnicode = MacroManager_KeyIDKeyCode(aKeyID);
		
		
		// note: this should match logic in MacroManager.mm to set menu keys
		if (0 == keyEquivalentUnicode)
		{
			*outUnicodeSequenceIfOrdinaryKeyBinding = @"";
		}
		else
		{
			*outUnicodeSequenceIfOrdinaryKeyBinding = [NSString stringWithCharacters:&keyEquivalentUnicode length:1];
		}
		*outBindingPtr = kMacroManager_KeyBindingOrdinaryCharacter;
	}
}// setUIEnum:ordinaryCharacter:fromPreference:


/*!
Updates the view model’s observed properties based on
current preferences context data.

This is only needed when changing contexts.

See also "dataUpdated", which should be roughly the
inverse of this.

(2021.02)
*/
- (void)
updateViewModelFromPrefsMgr
{
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	Boolean					isDefault = false; // reused below
	Boolean					isDefaultMacroAction = true; // initial value; see below
	Boolean					isDefaultMacroKey = true; // initial value; see below
	
	
	// allow initialization of "isDefault..." values without triggers
	self.viewModel.defaultOverrideInProgress = YES;
	self.viewModel.disableWriteback = YES;
	
	// update settings
	{
		Preferences_Tag		preferenceTag = self.indexedTagMacroKey;
		UInt32				preferenceValue = 0;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
			self.viewModel.selectedKeyBinding = kMacroManager_KeyBindingOrdinaryCharacter; // SwiftUI binding
			self.viewModel.macroOrdinaryKey = @""; // SwiftUI binding
		}
		else
		{
			MacroManager_KeyID			asKeyID = STATIC_CAST(preferenceValue, MacroManager_KeyID);
			MacroManager_KeyBinding		keyBinding = self.viewModel.selectedKeyBinding;
			NSString*					ordinaryCharacterSequence = @"";
			
			
			[self setUIEnum:&keyBinding ordinaryCharacter:&ordinaryCharacterSequence fromPreference:asKeyID];
			self.viewModel.selectedKeyBinding = keyBinding; // SwiftUI binding
			self.viewModel.macroOrdinaryKey = ordinaryCharacterSequence; // SwiftUI binding
			isDefaultMacroKey = (isDefaultMacroKey && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = self.indexedTagMacroModifiers;
		UInt32				preferenceValue = 0;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
			self.viewModel.macroModifiers = 0;
		}
		else
		{
			self.viewModel.macroModifiers = STATIC_CAST(preferenceValue, MacroManager_ModifierKeyMask); // SwiftUI binding
			isDefaultMacroKey = (isDefaultMacroKey && isDefault); // see below; used to update binding
		}
	}
	self.viewModel.isDefaultMacroKey = isDefaultMacroKey; // SwiftUI binding
	{
		Preferences_Tag		preferenceTag = self.indexedTagMacroAction;
		UInt32				preferenceValue = 0;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
			self.viewModel.selectedAction = kMacroManager_ActionSendTextProcessingEscapes; // SwiftUI binding
		}
		else
		{
			self.viewModel.selectedAction = STATIC_CAST(preferenceValue, MacroManager_Action); // SwiftUI binding
			isDefaultMacroAction = (isDefaultMacroAction && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = self.indexedTagMacroContents;
		CFStringRef			preferenceValue = nullptr;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
			self.viewModel.macroContent = @""; // SwiftUI binding
		}
		else
		{
			self.viewModel.macroContent = BRIDGE_CAST(preferenceValue, NSString*); // SwiftUI binding
			isDefaultMacroAction = (isDefaultMacroAction && isDefault); // see below; used to update binding
		}
	}
	self.viewModel.isDefaultMacroAction = isDefaultMacroAction; // SwiftUI binding
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::WORKSPACE);
}// updateViewModelFromPrefsMgr


#pragma mark Keypads_ControlKeyResponder


/*!
Received when the Control Keys keypad is closed while
this class instance is the current keypad responder (as
set by Keypads_SetResponder()).

This handles the event by updating the UI and state.

(2021.02)
*/
- (void)
controlKeypadHidden
{
	// remove annotations from buttons that indicate binding to “Control Keys”
	self.viewModel.isKeypadBindingToMacroContentInsertion = NO;
}// controlKeypadHidden


/*!
Received when the Control Keys keypad is used to select
a control key while this class instance is the current
keypad responder (as set by Keypads_SetResponder()).

This handles the event by inserting an equivalent
octal escape sequence into the macro content field.

(2021.02)
*/
- (void)
controlKeypadSentCharacterCode:(NSNumber*)	asciiChar
{
	NSString*	originalContents = [self.viewModel.macroContent copy];
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
	
	self.viewModel.macroContent = newContents;
	
	// save new preferences
	[self dataUpdated]; // "viewModel.runner.dataUpdated()" in SwiftUI
}// controlKeypadSentCharacterCode:


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

(2021.02)
*/
- (void)
dataUpdated
{
	Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
	
	
	// update flags
	{
		MacroManager_KeyID			keyID = 0;
		MacroManager_KeyBinding		keyBinding = self.viewModel.selectedKeyBinding;
		NSString*					ordinaryCharacterSequence = self.viewModel.macroOrdinaryKey;
		
		
		[self setPreference:&keyID fromUIEnum:keyBinding ordinaryCharacter:ordinaryCharacterSequence];
		{
			Preferences_Tag		preferenceTag = self.indexedTagMacroKey;
			UInt32				preferenceValue = STATIC_CAST(keyID, UInt32);
			Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
				Console_Warning(Console_WriteValue, "preference result", prefsResult);
			}
		}
	}
	{
		Preferences_Tag		preferenceTag = self.indexedTagMacroModifiers;
		UInt32				preferenceValue = self.viewModel.macroModifiers;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = self.indexedTagMacroAction;
		UInt32				preferenceValue = self.viewModel.selectedAction;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = self.indexedTagMacroContents;
		CFStringRef			preferenceValue = BRIDGE_CAST(self.viewModel.macroContent, CFStringRef);
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
Returns the Default value for the “ordinary key” field.

(2021.02)
*/
- (NSString*)
getSelectedMacroOrdinaryKey
{
	NSString*				result = @"";
	Preferences_Tag			indexedTag = self.indexedTagMacroKey; // see "configureForIndex:"
	UInt32					preferenceValue = 0;
	Boolean					isDefault = false;
	Preferences_Result		prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, indexedTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", indexedTag);
		Console_Warning(Console_WriteValue, "preference result", prefsResult);
	}
	else
	{
		MacroManager_KeyID			asKeyID = STATIC_CAST(preferenceValue, MacroManager_KeyID);
		MacroManager_KeyBinding		unusedUIEnum = kMacroManager_KeyBindingOrdinaryCharacter;
		
		
		[self setUIEnum:&unusedUIEnum ordinaryCharacter:&result fromPreference:asKeyID];
	}
	
	return result;
}// getSelectedMacroOrdinaryKey


/*!
Deletes any local override for the given value and
returns the Default value.

(2021.02)
*/
- (MacroManager_KeyBinding)
resetToDefaultGetSelectedMacroKeyBinding
{
	MacroManager_KeyBinding		result = kMacroManager_KeyBindingOrdinaryCharacter;
	Preferences_Tag				indexedTag = self.indexedTagMacroKey; // see "configureForIndex:"
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:indexedTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", indexedTag);
	}
	
	// return default value
	{
		UInt32				preferenceValue = 0;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, indexedTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", indexedTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			MacroManager_KeyID	asKeyID = STATIC_CAST(preferenceValue, MacroManager_KeyID);
			NSString*			unusedOrdinaryChar = @"";
			
			
			[self setUIEnum:&result ordinaryCharacter:&unusedOrdinaryChar fromPreference:asKeyID];
		}
	}
	
	return result;
}// resetToDefaultGetSelectedMacroKeyBinding


/*!
Deletes any local override for the given value and
returns the Default value.

(2021.02)
*/
- (MacroManager_ModifierKeyMask)
resetToDefaultGetSelectedModifiers
{
	MacroManager_ModifierKeyMask	result = 0;
	Preferences_Tag					indexedTag = self.indexedTagMacroModifiers; // see "configureForIndex:"
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:indexedTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", indexedTag);
	}
	
	// return default value
	{
		UInt32				preferenceValue = 0;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, indexedTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", indexedTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = STATIC_CAST(preferenceValue, MacroManager_ModifierKeyMask);
		}
	}
	
	return result;
}// resetToDefaultGetSelectedModifiers


/*!
Deletes any local override for the given value and
returns the Default value.

(2021.02)
*/
- (MacroManager_Action)
resetToDefaultGetSelectedAction
{
	MacroManager_Action		result = kMacroManager_ActionSendTextProcessingEscapes;
	Preferences_Tag			indexedTag = self.indexedTagMacroAction; // see "configureForIndex:"
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:indexedTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", indexedTag);
	}
	
	// return default value
	{
		UInt32				preferenceValue = 0;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, indexedTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", indexedTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = STATIC_CAST(preferenceValue, MacroManager_Action);
		}
	}
	
	return result;
}// resetToDefaultGetSelectedAction


/*!
Deletes any local override for the given value and
returns the Default value.

(2021.02)
*/
- (NSString*)
resetToDefaultGetSelectedMacroContent
{
	NSString*			result = @"";
	Preferences_Tag		indexedTag = self.indexedTagMacroContents; // see "configureForIndex:"
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:indexedTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", indexedTag);
	}
	
	// return default value
	{
		CFStringRef			preferenceValue = nullptr;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, indexedTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", indexedTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = BRIDGE_CAST(preferenceValue, NSString*);
		}
	}
	
	return result;
}// resetToDefaultGetSelectedMacroContent


/*!
Displays the “Control Keys” palette and binds it to the
panel, allowing any clicked keys to be inserted into
the macro content field as octal escape sequences.

(The user can turn this off by closing the palette or
clicking the button that opened it.)

(2021.02)
*/
- (void)
selectControlKeyToInsertBacklashSequenceWithViewModel:(UIPrefsMacroEditor_Model*)	viewModel
{
	viewModel.isKeypadBindingToMacroContentInsertion = YES;
	Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self); // see "controlKeypadSentCharacterCode:"
	Keypads_SetVisible(kKeypads_WindowTypeControlKeys, true);
}// selectControlKeyToInsertBacklashSequenceWithViewModel:


@end //} PrefPanelMacros_EditorActionHandler


#pragma mark -
@implementation PrefPanelMacros_EditorVC //{


/*!
Designated initializer.

(2021.02)
*/
- (instancetype)
init
{
	PrefPanelMacros_EditorActionHandler*	actionHandler = [[PrefPanelMacros_EditorActionHandler alloc] init];
	NSView*									newView = [UIPrefsMacroEditor_ObjC makeView:actionHandler.viewModel];
	
	
	self = [super initWithView:newView delegate:self context:actionHandler/* transfer ownership (becomes "actionHandler" property in "panelViewManager:initializeWithContext:") */];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(2021.02)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext
{
#pragma unused(aViewManager, aContext)
	assert(nil != aContext);
	PrefPanelMacros_EditorActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelMacros_EditorActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	self.actionHandler = actionHandler; // transfer ownership
	self.idealFrame = CGRectMake(0, 0, 460, 280); // somewhat arbitrary; see SwiftUI code/playground
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self.logicalFirstResponder = self.view;
	self.logicalLastResponder = self.view;
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(2021.02)
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

(2021.02)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
#pragma unused(aViewManager)
	// remember initial frame (it might be changed later)
	self.idealFrame = [aContainerView frame];
	
	// since this is an index-based preference panel, set initial value
	[self.actionHandler configureForIndex:1];
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

(2021.02)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	*outIdealSize = self.idealFrame.size;
}// panelViewManager:requestingIdealSize:


/*!
Responds to a request for contextual help in this panel.

(2021.02)
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

(2021.02)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(2021.02)
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

(2021.02)
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
	
	
	// apply the specified settings, if they changed
	if (nullptr != asPrefsContext)
	{
		[self.actionHandler.prefsMgr setCurrentContext:asPrefsContext];
	}
	
	// user may or may not have selected a different macro
	// (the response is currently the same either way)
	[self.actionHandler configureForIndex:asIndex];
	
	// update the view by changing the model’s observed variables
	[self.actionHandler updateViewModelFromPrefsMgr];
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).  Responds by saving preferences.

(2021.02)
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

(2021.02)
*/
- (NSImage*)
panelIcon
{
	if (@available(macOS 11.0, *))
	{
		return [NSImage imageWithSystemSymbolName:@"command" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelMacros"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2021.02)
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

(2021.02)
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

(2021.02)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintHorizontal;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(2021.02)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::MACRO_SET;
}// preferencesClass


@end //} PrefPanelMacros_EditorVC

// BELOW IS REQUIRED NEWLINE TO END FILE
