/*!	\file PrefPanelTranslations.mm
	\brief Implements the Translations panel of Preferences.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
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

#import "PrefPanelTranslations.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Mac includes
#import <objc/objc-runtime.h>
@import Cocoa;
@import CoreServices;

// library includes
#import <BoundName.objc++.h>
#import <CFRetainRelease.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <Localization.h>
#import <MemoryBlocks.h>

// application includes
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "TextTranslation.h"
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
@interface PrefPanelTranslations_ActionHandler : NSObject< UIPrefsTranslations_ActionHandling > //{

// new methods
	- (void)
	fontPanelWillClose:(id)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsTranslations_Model*
	viewModel;

@end //}


/*!
Private properties.
*/
@interface PrefPanelTranslations_VC () //{

// accessors
	@property (strong) PrefPanelTranslations_ActionHandler*
	actionHandler;

@end //}



#pragma mark Public Methods

/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.0)
*/
Preferences_TagSetRef
PrefPanelTranslations_NewTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	tagList.push_back(kPreferences_TagTextEncodingIANAName);
	tagList.push_back(kPreferences_TagTextEncodingID);
	tagList.push_back(kPreferences_TagBackupFontName);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods

#pragma mark -
@implementation PrefPanelTranslations_ActionHandler //{


/*!
Designated initializer.

(2021.03)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		_prefsMgr = nil; // see "panelViewManager:initializeWithContext:"
		_viewModel = [[UIPrefsTranslations_Model alloc] initWithRunner:self]; // transfer ownership
		
		UInt16		characterSetCount = TextTranslation_ReturnCharacterSetCount();
		auto		mutableArray = [[NSMutableArray< UIPrefsTranslations_ItemModel* > alloc] initWithCapacity:characterSetCount];
		
		
		// build a table of available text encodings
		for (UInt16 i = 1; i <= characterSetCount; ++i)
		{
			CFStringEncoding	encodingID = TextTranslation_ReturnIndexedCharacterSet(i);
			
			
			[mutableArray addObject:[[UIPrefsTranslations_ItemModel alloc] init:encodingID]];
		}
		self.viewModel.translationTableArray = mutableArray;
	}
	return self;
}// init


/*!
Destructor.

(2021.03)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark New Methods


/*!
See "setBackupFontWithViewModel:".

(2021.03)
*/
- (void)
fontPanelWillClose:(id)		sender
{
	self.viewModel.isFontPanelBindingToBackupFont = NO;
}// fontPanelWillClose:


/*!
Updates the view model’s observed properties based on
current preferences context data.

This is only needed when changing contexts.

See also "dataUpdated", which should be roughly the
inverse of this.

(2021.03)
*/
- (void)
updateViewModelFromPrefsMgr
{
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	Boolean					isDefault = false; // reused below
	
	
	// allow initialization of values without triggers
	self.viewModel.disableWriteback = YES;
	
	// update settings
	{
		CFStringEncoding	preferenceValue = TextTranslation_ContextReturnEncoding(sourceContext);
		
		
		if (kCFStringEncodingInvalidId == preferenceValue)
		{
			self.viewModel.selectedEncoding = nil; // SwiftUI binding
		}
		else
		{
			self.viewModel.selectedEncoding = [[UIPrefsTranslations_ItemModel alloc] init:preferenceValue]; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagBackupFontName;
		CFStringRef			preferenceValue = CFSTR("");
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			//Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			//Console_Warning(Console_WriteValue, "preference result", prefsResult);
			self.viewModel.backupFontName = nil; // SwiftUI binding
			self.viewModel.useBackupFont = false; // SwiftUI binding
		}
		else
		{
			self.viewModel.backupFontName = BRIDGE_CAST(preferenceValue, NSString*); // SwiftUI binding
			self.viewModel.useBackupFont = true; // SwiftUI binding
			//self.viewModel.isDefaultBackupFont = isDefault; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsTranslations_ActionHandling


/*!
Called by the UI when the user has made a change.

Currently this is called for any change to any setting so the
only way to respond is to copy all model data to the preferences
context.  If performance or other issues arise, it is possible
to expand the protocol to have (say) per-setting callbacks but
for now this is simpler and sufficient.

See also "updateViewModelFromPrefsMgr", which should be roughly
the inverse of this.

(2021.03)
*/
- (void)
dataUpdated
{
	Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
	
	
	// update settings
	if (nil != self.viewModel.selectedEncoding)
	{
		if (false == TextTranslation_ContextSetEncoding(targetContext, self.viewModel.selectedEncoding.id))
		{
			Console_Warning(Console_WriteLine, "failed to save text encoding");
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagBackupFontName;
		CFStringRef			preferenceValue = BRIDGE_CAST(self.viewModel.backupFontName, CFStringRef);
		
		
		if (nullptr == preferenceValue)
		{
			Preferences_ContextDeleteData(targetContext, preferenceTag);
		}
		else
		{
			Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
				Console_Warning(Console_WriteValue, "preference result", prefsResult);
			}
		}
	}
}// dataUpdated


/*!
Called by the UI when the user asks to select a new
backup font.  This responds by displaying the system
Font Panel (bound to the preference setting).

(2021.03)
*/
- (void)
setBackupFontWithViewModel:(UIPrefsTranslations_Model*)		viewModel
{
	if (nil == viewModel)
	{
		[self ignoreWhenObject:[NSFontPanel sharedFontPanel] postsNote:NSWindowWillCloseNotification];
		[[NSFontPanel sharedFontPanel] orderOut:nil];
	}
	else
	{
		NSFont*		initialFont = [NSFont fontWithName:viewModel.backupFontName size:[NSFont systemFontSize]];
		
		
		viewModel.isFontPanelBindingToBackupFont = YES;
		[[NSFontManager sharedFontManager] setSelectedFont:initialFont isMultiple:NO];
		[[NSFontPanel sharedFontPanel] orderFront:nil];
		[self whenObject:[NSFontPanel sharedFontPanel] postsNote:NSWindowWillCloseNotification performSelector:@selector(fontPanelWillClose:)]; // see "dealloc"
	}
}// setBackupFontWithViewModel


@end //}

#pragma mark -
@implementation PrefPanelTranslations_VC //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	PrefPanelTranslations_ActionHandler*	actionHandler = [[PrefPanelTranslations_ActionHandler alloc] init];
	NSView*									newView = [UIPrefsTranslations_ObjC makeView:actionHandler.viewModel];
	
	
	self = [super initWithView:newView delegate:self context:actionHandler/* transfer ownership (becomes "actionHandler" property in "panelViewManager:initializeWithContext:") */];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


#pragma mark NSFontPanel


/*!
Updates the “backup font” based on the user’s selection
in the system’s Fonts panel.

(2021.03)
*/
- (void)
changeFont:(id)		sender
{
	NSFont*		defaultFont = [NSFont monospacedSystemFontOfSize:[NSFont systemFontSize] weight:NSFontWeightRegular];
	NSFont*		oldFont = [NSFont fontWithName:self.actionHandler.viewModel.backupFontName size:[NSFont systemFontSize]];
	NSFont*		newFont = [sender convertFont:((nil == oldFont) ? defaultFont : oldFont)];
	
	
	self.actionHandler.viewModel.backupFontName = newFont.familyName;
}// changeFont:


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
#pragma unused(aViewManager)
	PrefPanelTranslations_ActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelTranslations_ActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	self.actionHandler = actionHandler; // transfer ownership
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self.logicalFirstResponder = self.view;
	self.logicalLastResponder = self.view;
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
	*outIdealSize = CGSizeMake(500, 350); // somewhat arbitrary; see SwiftUI code/playground;
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

(4.1)
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
		return [NSImage imageWithSystemSymbolName:@"globe" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelTranslations"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Translations";
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
	return NSLocalizedStringFromTable(@"Translations", @"PrefPanelTranslations", @"the name of this panel");
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
	return kPanel_ResizeConstraintBothAxes;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::TRANSLATION;
}// preferencesClass


@end //} PrefPanelTranslations_VC

// BELOW IS REQUIRED NEWLINE TO END FILE
