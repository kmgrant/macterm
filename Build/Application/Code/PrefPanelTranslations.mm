/*!	\file PrefPanelTranslations.mm
	\brief Implements the Translations panel of Preferences.
	
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

#import "PrefPanelTranslations.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <BoundName.objc++.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <Localization.h>
#import <MemoryBlocks.h>

// application includes
#import "AppResources.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "TextTranslation.h"
#import "UIStrings.h"



#pragma mark Types

/*!
Implements an object wrapper for translation tables, that allows
them to be easily inserted into user interface elements without
losing less user-friendly information about each encoding.
*/
@interface PrefPanelTranslations_TableInfo : BoundName_Object //{
{
@private
	CFStringEncoding	encodingType;
}

// initializers
	- (instancetype)
	initWithEncodingType:(CFStringEncoding)_
	description:(NSString*)_;

// accessors; see "Translation Tables" array controller in the NIB, for key names
	- (CFStringEncoding)
	encodingType;
	- (void)
	setEncodingType:(CFStringEncoding)_;

@end //}

#pragma mark Internal Method Prototypes

/*!
The private class interface.
*/
@interface PrefPanelTranslations_ViewManager (PrefPanelTranslations_ViewManagerInternal) @end



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
@implementation PrefPanelTranslations_TableInfo


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithEncodingType:(CFStringEncoding)		anEncoding
description:(NSString*)						aDescription
{
	self = [super initWithBoundName:aDescription];
	if (nil != self)
	{
		[self setEncodingType:anEncoding];
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

(4.0)
*/
- (CFStringEncoding)
encodingType
{
	return encodingType;
}
- (void)
setEncodingType:(CFStringEncoding)		anEncoding
{
	encodingType = anEncoding;
}// setEncodingType:


@end // PrefPanelTranslations_TableInfo


#pragma mark -
@implementation PrefPanelTranslations_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelTranslationsCocoa" delegate:self context:nullptr];
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
	[translationTables release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
backupFontEnabled
{
	return (NO == [[self backupFontFamilyName] isEqualToString:@""]);
}
- (void)
setBackupFontEnabled:(BOOL)		aFlag
{
	if ([self backupFontEnabled] != aFlag)
	{
		if (aFlag)
		{
			// the default is arbitrary (TEMPORARY; should probably read this from somewhere else)
			NSString*	defaultFont = [[NSFont userFixedPitchFontOfSize:[NSFont systemFontSize]] familyName];
			
			
			if (nil == defaultFont)
			{
				defaultFont = @"Monaco";
			}
			[self setBackupFontFamilyName:defaultFont];
		}
		else
		{
			[self setBackupFontFamilyName:@""];
		}
	}
}// setBackupFontEnabled:


/*!
Accessor.

(4.1)
*/
- (NSString*)
backupFontFamilyName
{
	NSString*	result = @"";
	
	
	if (Preferences_ContextIsValid(self->activeContext))
	{
		CFStringRef			fontName = nullptr;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self->activeContext, kPreferences_TagBackupFontName,
																		sizeof(fontName), &fontName, false/* search defaults too */);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = BRIDGE_CAST(fontName, NSString*);
			[result autorelease];
		}
	}
	return result;
}
- (void)
setBackupFontFamilyName:(NSString*)		aString
{
	if (Preferences_ContextIsValid(self->activeContext))
	{
		CFStringRef			fontName = BRIDGE_CAST(aString, CFStringRef);
		BOOL				saveOK = NO;
		Preferences_Result	prefsResult = Preferences_ContextSetData(self->activeContext, kPreferences_TagBackupFontName,
																		sizeof(fontName), &fontName);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			saveOK = YES;
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteLine, "failed to save backup font preference");
		}
	}
}// setBackupFontFamilyName:


/*!
Accessor.

(4.0)
*/
- (NSIndexSet*)
translationTableIndexes
{
	NSIndexSet*		result = [NSIndexSet indexSetWithIndex:0];
	
	
	if (Preferences_ContextIsValid(self->activeContext))
	{
		CFStringEncoding	encodingID = TextTranslation_ContextReturnEncoding(self->activeContext, kCFStringEncodingInvalidId);
		
		
		if (kCFStringEncodingInvalidId != encodingID)
		{
			UInt16		index = TextTranslation_ReturnCharacterSetIndex(encodingID);
			
			
			if (0 == index)
			{
				// not found
				Console_Warning(Console_WriteValue, "saved encoding is not available on the current system", encodingID);
			}
			else
			{
				// translate from one-based to zero-based
				result = [NSIndexSet indexSetWithIndex:(index - 1)];
			}
		}
	}
	return result;
}
- (void)
setTranslationTableIndexes:(NSIndexSet*)	indexes
{
	if (Preferences_ContextIsValid(self->activeContext))
	{
		BOOL	saveOK = NO;
		
		
		[self willChangeValueForKey:@"translationTableIndexes"];
		
		if ([indexes count] > 0)
		{
			// translate from zero-based to one-based
			UInt16		index = STATIC_CAST(1 + [indexes firstIndex], UInt16);
			
			
			// update preferences
			if (TextTranslation_ContextSetEncoding(self->activeContext, TextTranslation_ReturnIndexedCharacterSet(index),
													false/* via copy */))
			{
				saveOK = YES;
			}
			
			// scroll the list to reveal the current selection
			[self->translationTableView scrollRowToVisible:[indexes firstIndex]];
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteLine, "failed to save translation table preference");
		}
		
		[self didChangeValueForKey:@"translationTableIndexes"];
	}
}// setTranslationTableIndexes:


/*!
Accessor.

(4.1)
*/
- (NSArray*)
translationTables
{
	return [[translationTables retain] autorelease];
}


#pragma mark Actions


/*!
Displays a font panel for editing the backup font.

(4.1)
*/
- (IBAction)
performBackupFontSelection:(id)		sender
{
	[[NSFontPanel sharedFontPanel] orderFront:sender];
}// performBackupFontSelection:


#pragma mark NSFontPanel


/*!
Updates the “backup font” preference based on the user’s
selection in the system’s Fonts panel.

(4.1)
*/
- (void)
changeFont:(id)		sender
{
	NSFont*		oldFont = [NSFont fontWithName:[self backupFontFamilyName] size:[NSFont systemFontSize]];
	NSFont*		newFont = [sender convertFont:oldFont];
	
	
	[self setBackupFontFamilyName:[newFont familyName]];
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
initializeWithContext:(void*)			aContext
{
#pragma unused(aViewManager, aContext)
	UInt16		characterSetCount = TextTranslation_ReturnCharacterSetCount();
	
	
	self->translationTables = [[NSMutableArray alloc] initWithCapacity:characterSetCount];
	self->activeContext = nullptr; // set later
	
	// build a table of available text encodings
	[self willChangeValueForKey:@"translationTables"];
	for (UInt16 i = 1; i <= characterSetCount; ++i)
	{
		CFStringEncoding	encodingID = TextTranslation_ReturnIndexedCharacterSet(i);
		NSString*			encodingName = BRIDGE_CAST(CFStringGetNameOfEncoding(encodingID), NSString*);
		
		
		[self->translationTables addObject:[[[PrefPanelTranslations_TableInfo alloc]
												initWithEncodingType:encodingID description:encodingName]
											autorelease]];
	}
	[self didChangeValueForKey:@"translationTables"];
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
	assert(nil != translationTableView);
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

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	self->activeContext = REINTERPRET_CAST(newDataSet, Preferences_ContextRef);
	
	// invoke every accessor with itself, which has the effect of
	// reading the current preference values (of the now-current
	// new context) and then updating the display based on them
	[self setBackupFontEnabled:[self backupFontEnabled]];
	[self setBackupFontFamilyName:[self backupFontFamilyName]];
	[self setTranslationTableIndexes:[self translationTableIndexes]];
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


@end // PrefPanelTranslations_ViewManager


#pragma mark -
@implementation PrefPanelTranslations_ViewManager (PrefPanelTranslations_ViewManagerInternal)


@end // PrefPanelTranslations_ViewManager (PrefPanelTranslations_ViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
