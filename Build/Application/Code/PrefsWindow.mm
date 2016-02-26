/*!	\file PrefsWindow.mm
	\brief Implements the shell of the Preferences window.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#import "PrefsWindow.h"
#import <UniversalDefines.h>

// standard-C++ includes
#import <algorithm>
#import <functional>
#import <vector>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <FileSelectionDialogs.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>
#import <WindowInfo.h>

// application includes
#import "AppResources.h"
#import "ConstantsRegistry.h"
#import "EventLoop.h"
#import "FileUtilities.h"
#import "HelpSystem.h"
#import "Panel.h"
#import "Preferences.h"
#import "PrefPanelFormats.h"
#import "PrefPanelFullScreen.h"
#import "PrefPanelGeneral.h"
#import "PrefPanelMacros.h"
#import "PrefPanelSessions.h"
#import "PrefPanelTerminals.h"
#import "PrefPanelTranslations.h"
#import "PrefPanelWorkspaces.h"
#import "QuillsPrefs.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

/*!
The data for "kMy_PrefsWindowSourceListDataType" should be an
NSKeyedArchive of an NSIndexSet object (dragged row index).
*/
NSString*	kMy_PrefsWindowSourceListDataType = @"net.macterm.MacTerm.prefswindow.sourcelistdata";

} // anonymous namespace

#pragma mark Types

/*!
This class represents a particular preferences context
in the source list.
*/
@interface PrefsWindow_Collection : NSObject< NSCopying > //{
{
@private
	Preferences_ContextRef		preferencesContext;
	BOOL						isDefault;
}

// initializers
	- (instancetype)
	initWithPreferencesContext:(Preferences_ContextRef)_
	asDefault:(BOOL)_;

// accessors
	- (NSString*)
	boundName;
	- (void)
	setBoundName:(NSString*)_; // binding
	- (NSString*)
	description;
	- (void)
	setDescription:(NSString*)_; // binding
	- (BOOL)
	isEditable; // binding
	- (Preferences_ContextRef)
	preferencesContext;

// NSCopying
	- (id)
	copyWithZone:(NSZone*)_;

// NSObject
	- (unsigned int)
	hash;
	- (BOOL)
	isEqual:(id)_;

@end //}

#pragma mark Internal Method Prototypes
namespace {

CFDictionaryRef			createSearchDictionary			();
OSStatus				receiveHICommand				(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

/*!
The private class interface.
*/
@interface PrefsWindow_Controller (PrefsWindow_ControllerInternal) //{

// class methods
	+ (void)
	popUpResultsForQuery:(NSString*)_
	inSearchField:(NSSearchField*)_;
	+ (BOOL)
	queryString:(NSString*)_
	matchesTargetPhrase:(NSString*)_;
	+ (BOOL)
	queryString:(NSString*)_
	withWordArray:(NSArray*)_
	matchesTargetPhrase:(NSString*)_;

// new methods
	- (void)
	displayPanel:(Panel_ViewManager< PrefsWindow_PanelInterface >*)_
	withAnimation:(BOOL)_;
	- (void)
	displayPanelOrTabWithIdentifier:(NSString*)_
	withAnimation:(BOOL)_;
	- (void)
	rebuildSourceList;
	- (void)
	setSourceListHidden:(BOOL)_
	newWindowFrame:(NSRect)_;

// accessors
	- (Quills::Prefs::Class)
	currentPreferencesClass;
	- (Preferences_ContextRef)
	currentPreferencesContext;

// methods of the form required by NSSavePanel
	- (void)
	didEndExportPanel:(NSSavePanel*)_
	returnCode:(int)_
	contextInfo:(void*)_;

// methods of the form required by NSOpenPanel
	- (void)
	didEndImportPanel:(NSOpenPanel*)_
	returnCode:(int)_
	contextInfo:(void*)_;

@end //}

#pragma mark Variables
namespace {

CarbonEventHandlerWrap		gPrefsCommandHandler(GetApplicationEventTarget(),
													receiveHICommand,
													CarbonEventSetInClass
														(CarbonEventClass(kEventClassCommand),
															kEventCommandProcess),
													nullptr/* user data */);
Console_Assertion			_1(gPrefsCommandHandler.isInstalled(), __FILE__, __LINE__);
NSDictionary*				gSearchDataDictionary ()	{ static CFDictionaryRef x = createSearchDictionary(); return BRIDGE_CAST(x, NSDictionary*); }

} // anonymous namespace


#pragma mark Public Methods


/*!
Copies the specified settings into a new global preferences
collection of the appropriate type.

If you want the user to immediately see the new settings,
pass an appropriate nonzero command ID that will be used to
request the display of a specific panel in the window.

(4.1)
*/
void
PrefsWindow_AddCollection		(Preferences_ContextRef		inReferenceContextToCopy,
								 Preferences_TagSetRef		inTagSetOrNull,
								 UInt32						inPrefPanelShowCommandIDOrZero)
{
	// create and immediately save a new named context, which
	// triggers callbacks to update Favorites lists, etc.
	Preferences_ContextWrap		newContext(Preferences_NewContextFromFavorites
											(Preferences_ContextReturnClass(inReferenceContextToCopy),
												nullptr/* name, or nullptr to auto-generate */),
											true/* is retained */);
	
	
	if (false == newContext.exists())
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "unable to create a new Favorite for copying local changes");
	}
	else
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextSave(newContext.returnRef());
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "unable to save the new context!");
		}
		else
		{
			prefsResult = Preferences_ContextCopy(inReferenceContextToCopy, newContext.returnRef(), inTagSetOrNull);
			if (kPreferences_ResultOK != prefsResult)
			{
				Sound_StandardAlert();
				Console_Warning(Console_WriteLine, "unable to copy local changes into the new Favorite!");
			}
			else
			{
				if (0 != inPrefPanelShowCommandIDOrZero)
				{
					// trigger an automatic switch to focus a related part of the Preferences window
					Commands_ExecuteByIDUsingEvent(inPrefPanelShowCommandIDOrZero);
				}
			}
		}
	}
}// AddCollection


#pragma mark Internal Methods
namespace {

/*!
Creates and returns a Core Foundation dictionary that can be used
to match search keywords to parts of the Preferences window (as
defined in the application bundle’s "PreferencesSearch.plist").

Returns nullptr if unsuccessful for any reason.

(4.1)
*/
CFDictionaryRef
createSearchDictionary ()
{
	CFDictionaryRef		result = nullptr;
	CFRetainRelease		fileCFURLObject(CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), CFSTR("PreferencesSearch"),
																CFSTR("plist")/* type string */, nullptr/* subdirectory path */),
										true/* is retained */);
	
	
	if (fileCFURLObject.exists())
	{
		CFURLRef	fileURL = REINTERPRET_CAST(fileCFURLObject.returnCFTypeRef(), CFURLRef);
		CFDataRef   fileData = nullptr;
		SInt32		errorCode = 0;
		
		
		unless (CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, fileURL, &fileData, 
															nullptr/* properties */, nullptr/* desired properties */, &errorCode))
		{
			// Not all data was successfully retrieved, but let the caller determine if anything
			// important is missing.
			// NOTE: Technically, the error code returned in "errorCode" is not an OSStatus.
			//       If negative, it is an Apple error, and if positive it is scheme-specific.
			Console_WriteValue("error reading raw data of 'PreferencesSearch.plist'", (SInt32)errorCode);
		}
		
		{
			CFPropertyListRef   	propertyList = nullptr;
			CFPropertyListFormat	actualFormat = kCFPropertyListXMLFormat_v1_0;
			CFErrorRef				errorObject = nullptr;
			
			
			propertyList = CFPropertyListCreateWithData(kCFAllocatorDefault, fileData, kCFPropertyListImmutable, &actualFormat, &errorObject);
			if (nullptr != errorObject)
			{
				CFRetainRelease		searchErrorCFString(CFErrorCopyDescription(errorObject), true/* is retained */);
				
				
				Console_WriteValueCFString("failed to create dictionary from 'PreferencesSearch.plist', error", searchErrorCFString.returnCFStringRef());
				Console_WriteValue("actual format of file is type", (SInt32)actualFormat);
				CFRelease(errorObject), errorObject = nullptr;
			}
			else
			{
				// the file actually contains a dictionary
				result = CFUtilities_DictionaryCast(propertyList);
			}
		}
		
		// finally, release the file data
		CFRelease(fileData), fileData = nullptr;
	}
	
	return result;
}// createSearchDictionary


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the preferences toolbar.  Responds by changing
the currently-displayed panel.

(3.1)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// execute a command selected from the toolbar
				switch (received.commandID)
				{
				case kCommandDisplayPrefPanelFormats:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorFormats, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				case kCommandDisplayPrefPanelGeneral:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorGeneral, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				case kCommandDisplayPrefPanelKiosk:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorKiosk, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				case kCommandDisplayPrefPanelMacros:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorMacros, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				case kCommandDisplayPrefPanelSessions:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorSessions, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				case kCommandDisplayPrefPanelTerminals:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorTerminals, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				case kCommandDisplayPrefPanelTranslations:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorTranslations, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				case kCommandDisplayPrefPanelWorkspaces:
					[[PrefsWindow_Controller sharedPrefsWindowController]
						displayPanelOrTabWithIdentifier:BRIDGE_CAST(kConstantsRegistry_PrefPanelDescriptorWorkspaces, NSString*)
														withAnimation:NO];
					result = noErr;
					break;
				
				default:
					// ???
					break;
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand

} // anonymous namespace


#pragma mark -
@implementation PrefsWindow_Collection


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesContext:(Preferences_ContextRef)		aContext
asDefault:(BOOL)										aFlag
{
	self = [super init];
	if (nil != self)
	{
		self->preferencesContext = aContext;
		if (nullptr != self->preferencesContext)
		{
			Preferences_RetainContext(self->preferencesContext);
		}
		self->isDefault = aFlag;
	}
	return self;
}// initWithPreferencesContext:asDefault:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	Preferences_ReleaseContext(&preferencesContext);
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

IMPORTANT:	The "boundName" key is ONLY required because older
			versions of Mac OS X do not seem to work properly
			when bound to the "description" accessor.  (Namely,
			the OS seems to stubbornly use its own "description"
			instead of invoking the right one.)  In the future
			this might be removed and rebound to "description".

(4.1)
*/
- (NSString*)
boundName
{
	NSString*	result = @"?";
	
	
	if (self->isDefault)
	{
		result = NSLocalizedStringFromTable(@"Default", @"PreferencesWindow", @"the name of the Default collection");
	}
	else if (false == Preferences_ContextIsValid(self->preferencesContext))
	{
		Console_Warning(Console_WriteValueAddress, "binding to source list has become invalid; preferences context", self->preferencesContext);
	}
	else
	{
		CFStringRef				nameCFString = nullptr;
		Preferences_Result		prefsResult = Preferences_ContextGetName(self->preferencesContext, nameCFString);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = BRIDGE_CAST(nameCFString, NSString*);
		}
	}
	return result;
}
- (void)
setBoundName:(NSString*)	aString
{
	if (self->isDefault)
	{
		// changing the name is illegal
	}
	else if (false == Preferences_ContextIsValid(self->preferencesContext))
	{
		Console_Warning(Console_WriteValueAddress, "binding to source list has become invalid; preferences context", self->preferencesContext);
	}
	else
	{
		Preferences_Result		prefsResult = Preferences_ContextRename(self->preferencesContext, (CFStringRef)aString);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteValueCFString, "failed to rename context, given name", (CFStringRef)aString);
		}
	}
}// setBoundName:


/*!
Accessor.

(4.1)
*/
- (BOOL)
isEditable
{
	return (NO == self->isDefault);
}// isEditable


/*!
Accessor.

(4.1)
*/
- (NSString*)
description
{
	return [self boundName];
}
- (void)
setDescription:(NSString*)	aString
{
	[self setBoundName:aString];
}// setDescription:


/*!
Accessor.

(4.1)
*/
- (Preferences_ContextRef)
preferencesContext
{
	return preferencesContext;
}// preferencesContext


#pragma mark NSCopying


/*!
Copies this instance by retaining the original context.

(4.1)
*/
- (id)
copyWithZone:(NSZone*)	aZone
{
	PrefsWindow_Collection*		result = [[self.class allocWithZone:aZone] init];
	
	
	if (nil != result)
	{
		result->preferencesContext = self->preferencesContext;
		Preferences_RetainContext(result->preferencesContext);
		result->isDefault = self->isDefault;
	}
	return result;
}// copyWithZone


#pragma mark NSObject


/*!
Returns a hash code for this object (a value that should
ideally change whenever a property significant to "isEqual:"
is changed).

(4.1)
*/
- (unsigned int)
hash
{
	return ((unsigned int)self->preferencesContext);
}// hash


/*!
Returns true if the given object is considered equivalent
to this one (which is the case if its "preferencesContext"
is the same).

(4.1)
*/
- (BOOL)
isEqual:(id)	anObject
{
	BOOL	result = NO;
	
	
	if ([anObject isKindOfClass:self.class])
	{
		PrefsWindow_Collection*		asCollection = (PrefsWindow_Collection*)anObject;
		
		
		result = ([asCollection preferencesContext] == [self preferencesContext]);
	}
	return result;
}// isEqual:


@end // PrefsWindow_Collection


#pragma mark -
@implementation PrefsWindow_Controller


static PrefsWindow_Controller*	gPrefsWindow_Controller = nil;


@synthesize searchText = _searchText;


/*!
Returns the singleton.

(4.1)
*/
+ (id)
sharedPrefsWindowController
{
	if (nil == gPrefsWindow_Controller)
	{
		gPrefsWindow_Controller = [[self.class allocWithZone:NULL] init];
	}
	return gPrefsWindow_Controller;
}// sharedPrefsWindowController


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithWindowNibName:@"PrefsWindowCocoa"];
	if (nil != self)
	{
		self->currentPreferenceCollectionIndexes = [[NSIndexSet alloc] init];
		self->currentPreferenceCollections = [[NSMutableArray alloc] init];
		self->panelIDArray = [[NSMutableArray arrayWithCapacity:7/* arbitrary */] retain];
		self->panelsByID = [[NSMutableDictionary dictionaryWithCapacity:7/* arbitrary */] retain];
		self->windowSizesByID = [[NSMutableDictionary dictionaryWithCapacity:7/* arbitrary */] retain];
		self->windowMinSizesByID = [[NSMutableDictionary dictionaryWithCapacity:7/* arbitrary */] retain];
		self->extraWindowContentSize = NSZeroSize; // set later
		self->activePanel = nil;
		self->_searchText = [@"" copy];
		
		// install a callback that finds out about changes to available preferences collections
		{
			Preferences_Result		error = kPreferences_ResultOK;
			
			
			self->preferenceChangeListener = [[ListenerModel_StandardListener alloc]
												initWithTarget:self
																eventFiredSelector:@selector(model:preferenceChange:context:)];
			
			error = Preferences_StartMonitoring([self->preferenceChangeListener listenerRef], kPreferences_ChangeContextName,
												false/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
			error = Preferences_StartMonitoring([self->preferenceChangeListener listenerRef], kPreferences_ChangeNumberOfContexts,
												true/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
		}
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
	[self ignoreWhenObjectsPostNotes];
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
																kPreferences_ChangeContextName);
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
																kPreferences_ChangeNumberOfContexts);
	[preferenceChangeListener release];
	[currentPreferenceCollectionIndexes release];
	[currentPreferenceCollections release];
	[panelIDArray release];
	[panelsByID release];
	[windowSizesByID release];
	[windowMinSizesByID release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSIndexSet*)
currentPreferenceCollectionIndexes
{
	// TEMPORARY; should retrieve translation table that is saved in preferences
	return [[currentPreferenceCollectionIndexes retain] autorelease];
}
- (void)
setCurrentPreferenceCollectionIndexes:(NSIndexSet*)		indexes
{
	if (indexes != currentPreferenceCollectionIndexes)
	{
		NSUInteger					oldIndex = [self->currentPreferenceCollectionIndexes firstIndex];
		NSUInteger					newIndex = [indexes firstIndex];
		PrefsWindow_Collection*		oldCollection = ((NSNotFound != oldIndex) && (oldIndex < currentPreferenceCollections.count))
													? [currentPreferenceCollections objectAtIndex:oldIndex]
													: nil;
		PrefsWindow_Collection*		newCollection = ((NSNotFound != newIndex) && (newIndex < currentPreferenceCollections.count))
													? [currentPreferenceCollections objectAtIndex:newIndex]
													: nil;
		Preferences_ContextRef		oldDataSet = (nil != oldCollection)
													? [oldCollection preferencesContext]
													: nullptr;
		Preferences_ContextRef		newDataSet = (nil != newCollection)
													? [newCollection preferencesContext]
													: nullptr;
		
		
		[currentPreferenceCollectionIndexes release];
		currentPreferenceCollectionIndexes = [indexes retain];
		
		// write all the preference data in memory to disk
		UNUSED_RETURN(Preferences_Result)Preferences_Save();
		
		// notify the panel that a new data set has been selected
		[self->activePanel.delegate panelViewManager:self->activePanel
														didChangeFromDataSet:oldDataSet toDataSet:newDataSet];
	}
}// setCurrentPreferenceCollectionIndexes:


/*!
Accessor.

(4.1)
*/
- (NSArray*)
currentPreferenceCollections
{
	return [[currentPreferenceCollections retain] autorelease];
}
+ (BOOL)
automaticallyNotifiesObserversOfCurrentPreferenceCollections
{
	return NO;
}// automaticallyNotifiesObserversOfCurrentPreferenceCollections


#pragma mark Actions


/*!
Adds a new preferences collection.  Also immediately enters
editing mode on the new collection so that the user may
change its name.

(4.1)
*/
- (IBAction)
performAddNewPreferenceCollection:(id)	sender
{
#pragma unused(sender)
	// NOTE: notifications are sent when a context is created as a Favorite
	// so the source list should automatically resynchronize
	Preferences_ContextRef	newContext = Preferences_NewContextFromFavorites
											([self currentPreferencesClass], nullptr/* auto-set name */);
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_ContextSave(newContext);
	if (kPreferences_ResultOK != prefsResult)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteValue, "failed to save newly-created context; error", prefsResult);
	}
	
	// reset the selection to focus on the new item, and automatically
	// assume the user wants to rename it
	[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:
															([self->currentPreferenceCollections count] - 1)]];
	[self performRenamePreferenceCollection:nil];
}// performAddNewPreferenceCollection:


/*!
Invoked when the help button is clicked.

(4.1)
*/
- (IBAction)
performContextSensitiveHelp:(id)	sender
{
#pragma unused(sender)
	[self->activePanel performContextSensitiveHelp:sender];
}// performContextSensitiveHelp:


/*!
Adds a new preferences collection by making a copy of the
one selected in the source list.  Also immediately enters
editing mode on the new collection so that the user may
change its name.

(4.1)
*/
- (IBAction)
performDuplicatePreferenceCollection:(id)	sender
{
#pragma unused(sender)
	UInt16						selectedIndex = [self->sourceListTableView selectedRow];
	PrefsWindow_Collection*		baseCollection = [self->currentPreferenceCollections objectAtIndex:selectedIndex];
	Preferences_ContextRef		baseContext = [baseCollection preferencesContext];
	
	
	if (false == Preferences_ContextIsValid(baseContext))
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "failed to clone context; current selection is invalid");
	}
	else
	{
		// NOTE: notifications are sent when a context is created as a Favorite
		// so the source list should automatically resynchronize
		Preferences_ContextRef	newContext = Preferences_NewCloneContext(baseContext, false/* detach */);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextSave(newContext);
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteValue, "failed to save duplicated context; error", prefsResult);
		}
		
		// reset the selection to focus on the new item, and automatically
		// assume the user wants to rename it
		[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:
																([self->currentPreferenceCollections count] - 1)]];
		[self performRenamePreferenceCollection:nil];
	}
}// performDuplicatePreferenceCollection:


/*!
Saves the contents of a preferences collection to a file.

(4.1)
*/
- (IBAction)
performExportPreferenceCollectionToFile:(id)	sender
{
#pragma unused(sender)
	NSSavePanel*	savePanel = [NSSavePanel savePanel];
	CFStringRef		saveFileCFString = nullptr;
	CFStringRef		promptCFString = nullptr;
	
	
	(UIStrings_Result)UIStrings_Copy(kUIStrings_FileDefaultExportPreferences, saveFileCFString);
	(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptSavePrefs, promptCFString);
	[savePanel setMessage:(NSString*)promptCFString];
	
	[savePanel beginSheetForDirectory:nil file:(NSString*)saveFileCFString
										modalForWindow:[self window] modalDelegate:self
										didEndSelector:@selector(didEndExportPanel:returnCode:contextInfo:)
										contextInfo:nullptr];
}// performExportPreferenceCollectionToFile:


/*!
Adds a new preferences collection by asking the user for a
file from which to import settings.

(4.1)
*/
- (IBAction)
performImportPreferenceCollectionFromFile:(id)	sender
{
#pragma unused(sender)
	NSOpenPanel*	openPanel = [NSOpenPanel openPanel];
	NSArray*		allowedTypes = @[
										@"com.apple.property-list",
										@"plist"/* redundant, needed for older systems */,
										@"xml",
									];
	CFStringRef		promptCFString = nullptr;
	
	
	(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptOpenPrefs, promptCFString);
	[openPanel setMessage:(NSString*)promptCFString];
	
	[openPanel beginSheetForDirectory:nil file:nil types:allowedTypes
										modalForWindow:[self window] modalDelegate:self
										didEndSelector:@selector(didEndImportPanel:returnCode:contextInfo:)
										contextInfo:nullptr];
}// performImportPreferenceCollectionFromFile:


/*!
Deletes the currently-selected preference collection from
the source list (and saved user preferences).

(4.1)
*/
- (IBAction)
performRemovePreferenceCollection:(id)	sender
{
#pragma unused(sender)
	UInt16		selectedIndex = [self->sourceListTableView selectedRow];
	
	
	if (0 == selectedIndex)
	{
		// the Default collection; this should never be possible to remove
		// (and should be prevented by other guards before this point, but
		// just in case...)
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "attempt to delete the Default context");
	}
	else
	{
		PrefsWindow_Collection*		deadCollection = [self->currentPreferenceCollections objectAtIndex:selectedIndex];
		Preferences_ContextRef		deadContext = [deadCollection preferencesContext];
		
		
		if (false == Preferences_ContextIsValid(deadContext))
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "failed to delete context; current selection is invalid");
		}
		else
		{
			Preferences_Result		prefsResult = kPreferences_ResultOK;
			
			
			// NOTE: notifications are sent when a context is deleted from Favorites
			// so the source list should automatically resynchronize
			prefsResult = Preferences_ContextDeleteFromFavorites(deadContext), deadCollection = nil, deadContext = nullptr;
			if (kPreferences_ResultOK != prefsResult)
			{
				Sound_StandardAlert();
				Console_Warning(Console_WriteValue, "failed to delete currently-selected context; error", prefsResult);
			}
			
			// reset the selection, as it will be invalidated when the target item is removed
			[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:0]];
		}
	}
}// performRemovePreferenceCollection:


/*!
Enters editing mode on the currently-selected preference
collection in the source list.

(4.1)
*/
- (IBAction)
performRenamePreferenceCollection:(id)	sender
{
#pragma unused(sender)
	UInt16		selectedIndex = [self->sourceListTableView selectedRow];
	
	
	// do not allow the Default item to be edited (this is
	// also ensured by the "isEditable" method on the objects
	// in the array)
	if (0 == selectedIndex)
	{
		Sound_StandardAlert();
	}
	else
	{
		[self->sourceListTableView editColumn:0 row:selectedIndex withEvent:nil select:NO];
		[[self->sourceListTableView currentEditor] selectAll:nil];
	}
}// performRenamePreferenceCollection:


/*!
Responds to the user typing in the search field.

(4.1)
*/
- (IBAction)
performSearch:(id)		sender
{
	if ([sender isKindOfClass:[NSSearchField class]])
	{
		NSSearchField*		searchField = REINTERPRET_CAST(sender, NSSearchField*);
		
		
		[PrefsWindow_Controller popUpResultsForQuery:self.searchText inSearchField:searchField];
	}
	else
	{
		Console_Warning(Console_WriteLine, "received 'performSearch:' message from unexpected sender");
	}
}// performSearch:


/*!
Responds to a “remove collection” segment click.

(No other commands need to be handled because they are
already bound to items in menus that the other segments
display.)

(4.1)
*/
- (IBAction)
performSegmentedControlAction:(id)	sender
{
	if ([sender isKindOfClass:[NSSegmentedControl class]])
	{
		NSSegmentedControl*		segments = REINTERPRET_CAST(sender, NSSegmentedControl*);
		
		
		// IMPORTANT: this should agree with the button arrangement
		// in the Preferences window’s NIB file...
		if (0 == [segments selectedSegment])
		{
			// this is redundantly handled as the “quick click” action
			// for the button; it is also in the segment’s pop-up menu
			[self performAddNewPreferenceCollection:sender];
		}
		else if (1 == [segments selectedSegment])
		{
			[self performRemovePreferenceCollection:sender];
		}
	}
	else
	{
		Console_Warning(Console_WriteLine, "received 'performSegmentedControlAction:' message from unexpected sender");
	}
}// performSegmentedControlAction:


#pragma mark NSColorPanel


/*!
Since this window controller is in the responder chain, it
may receive events from NSColorPanel that have not been
handled anywhere else.  This responds by forwarding the
message to the active panel if the active panel implements
a "changeColor:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeColor:(id)	sender
{
	if ([self->activePanel respondsToSelector:@selector(changeColor:)])
	{
		[self->activePanel changeColor:sender];
	}
}// changeColor:


#pragma mark NSFontPanel


/*!
Since this window controller is in the responder chain, it
may receive events from NSFontPanel that have not been
handled anywhere else.  This responds by forwarding the
message to the active panel if the active panel implements
a "changeFont:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeFont:(id)		sender
{
	if ([self->activePanel respondsToSelector:@selector(changeFont:)])
	{
		[self->activePanel changeFont:sender];
	}
}// changeFont:


#pragma mark NSTableDataSource


/*!
Returns the number of rows in the source list.  This is not
strictly necessary with bindings on later Mac OS X versions.
This is however a “required” method in the original protocol
so the table views will not work on older Mac OS X versions
unless this method is implemented.

(4.1)
*/
- (int)
numberOfRowsInTableView:(NSTableView*)	aTableView
{
#pragma unused(aTableView)
	int		result = [self->currentPreferenceCollections count];
	
	
	return result;
}// numberOfRowsInTableView:


/*!
Returns the object for the given source list row.  This is not
strictly necessary with bindings on later Mac OS X versions.
This is however a “required” method in the original protocol
so the table views will not work on older Mac OS X versions
unless this method is implemented.

(4.1)
*/
- (id)
tableView:(NSTableView*)					aTableView
objectValueForTableColumn:(NSTableColumn*)	aTableColumn
row:(int)									aRowIndex
{
#pragma unused(aTableView, aTableColumn)
	id		result = [self->currentPreferenceCollections objectAtIndex:aRowIndex];
	
	
	return result;
}// tableView:objectValueForTableColumn:row:


/*!
For drag-and-drop support; adds the data for the given rows
to the specified pasteboard.

NOTE:	This is defined for Mac OS X 10.3 support; it may be
		removed in the future.  See also the preferred method
		"tableView:writeRowIndexes:toPasteboard:".

(4.1)
*/
- (BOOL)
tableView:(NSTableView*)		aTableView
writeRows:(NSArray*)			aRowArray
toPasteboard:(NSPasteboard*)	aPasteboard
{
#pragma unused(aTableView)
	BOOL				result = YES; // initially...
	NSMutableIndexSet*	indexSet = [[NSMutableIndexSet alloc] init];
	
	
	// drags are confined to the table, so it’s only necessary to copy row numbers;
	// note that the Mac OS X 10.3 version gives arrays of NSNumber and the actual
	// representation (in "tableView:writeRowIndexes:toPasteboard:") is NSIndexSet*
	for (NSNumber* numberObject in aRowArray)
	{
		if (0 == [numberObject intValue])
		{
			// the item at index zero is Default, which may not be moved
			result = NO;
			break;
		}
		[indexSet addIndex:[numberObject intValue]];
	}
	
	if (result)
	{
		NSData*		copiedData = [NSKeyedArchiver archivedDataWithRootObject:indexSet];
		
		
		[aPasteboard declareTypes:@[kMy_PrefsWindowSourceListDataType] owner:self];
		[aPasteboard setData:copiedData forType:kMy_PrefsWindowSourceListDataType];
	}
	
	[indexSet release];
	
	return result;
}// tableView:writeRows:toPasteboard:


/*!
For drag-and-drop support; adds the data for the given rows
to the specified pasteboard.

This is the version used by Mac OS X 10.4 and beyond; see
also "tableView:writeRows:toPasteboard:".

(4.1)
*/
- (BOOL)
tableView:(NSTableView*)		aTableView
writeRowIndexes:(NSIndexSet*)	aRowSet
toPasteboard:(NSPasteboard*)	aPasteboard
{
#pragma unused(aTableView)
	BOOL	result = NO;
	
	
	// the item at index zero is Default, which may not be moved
	if (NO == [aRowSet containsIndex:0])
	{
		// drags are confined to the table, so it’s only necessary to copy row numbers
		NSData*		copiedData = [NSKeyedArchiver archivedDataWithRootObject:aRowSet];
		
		
		[aPasteboard declareTypes:@[kMy_PrefsWindowSourceListDataType] owner:self];
		[aPasteboard setData:copiedData forType:kMy_PrefsWindowSourceListDataType];
		result = YES;
	}
	return result;
}// tableView:writeRowIndexes:toPasteboard:


/*!
For drag-and-drop support; validates a drag operation.

(4.1)
*/
- (NSDragOperation)
tableView:(NSTableView*)							aTableView
validateDrop:(id< NSDraggingInfo >)					aDrop
proposedRow:(int)									aTargetRow
proposedDropOperation:(NSTableViewDropOperation)	anOperation
{
#pragma unused(aTableView, aDrop)
	NSDragOperation		result = NSDragOperationMove;
	
	
	switch (anOperation)
	{
	case NSTableViewDropOn:
		// rows may only be inserted above, not dropped on top
		result = NSDragOperationNone;
		break;
	
	case NSTableViewDropAbove:
		if (0 == aTargetRow)
		{
			// row zero always contains the Default item so nothing else may move there
			result = NSDragOperationNone;
		}
		break;
	
	default:
		// ???
		result = NSDragOperationNone;
		break;
	}
	
	return result;
}// tableView:validateDrop:proposedRow:proposedDropOperation:


/*!
For drag-and-drop support; moves the specified table row.

(4.1)
*/
- (BOOL)
tableView:(NSTableView*)					aTableView
acceptDrop:(id< NSDraggingInfo >)			aDrop
row:(int)									aTargetRow
dropOperation:(NSTableViewDropOperation)	anOperation
{
#pragma unused(aTableView, aDrop)
	BOOL	result = NO;
	
	
	switch (anOperation)
	{
	case NSTableViewDropOn:
		// rows may only be inserted above, not dropped on top
		result = NO;
		break;
	
	case NSTableViewDropAbove:
		{
			NSPasteboard*	pasteboard = [aDrop draggingPasteboard];
			NSData*			rowIndexSetData = [pasteboard dataForType:kMy_PrefsWindowSourceListDataType];
			NSIndexSet*		rowIndexes = [NSKeyedUnarchiver unarchiveObjectWithData:rowIndexSetData];
			
			
			if (nil != rowIndexes)
			{
				int		draggedRow = [rowIndexes firstIndex];
				
				
				if (draggedRow != aTargetRow)
				{
					PrefsWindow_Collection*		movingCollection = [self->currentPreferenceCollections objectAtIndex:draggedRow];
					Preferences_Result			prefsResult = kPreferences_ResultOK;
					
					
					if (aTargetRow >= STATIC_CAST([self->currentPreferenceCollections count], int))
					{
						// move to the end of the list (not a real target row)
						PrefsWindow_Collection*		targetCollection = [self->currentPreferenceCollections lastObject];
						
						
						if ([targetCollection preferencesContext] != [movingCollection preferencesContext])
						{
							// NOTE: notifications are sent when a context in Favorites is rearranged
							// so the source list should automatically resynchronize
							prefsResult = Preferences_ContextRepositionRelativeToContext
											([movingCollection preferencesContext], [targetCollection preferencesContext],
												false/* insert before */);
						}
					}
					else
					{
						PrefsWindow_Collection*		targetCollection = [self->currentPreferenceCollections objectAtIndex:aTargetRow];
						
						
						if ([targetCollection preferencesContext] != [movingCollection preferencesContext])
						{
							// NOTE: notifications are sent when a context in Favorites is rearranged
							// so the source list should automatically resynchronize
							prefsResult = Preferences_ContextRepositionRelativeToContext
											([movingCollection preferencesContext], [targetCollection preferencesContext],
												true/* insert before */);
						}
					}
					
					if (kPreferences_ResultOK != prefsResult)
					{
						Sound_StandardAlert();
						Console_Warning(Console_WriteValue, "failed to reorder preferences contexts; error", prefsResult);
					}
					
					result = YES;
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// tableView:acceptDrop:row:dropOperation:


#pragma mark NSTableViewDelegate


/*!
Informed when the selection in the source list changes.

This is not strictly necessary with bindings, on later versions
of Mac OS X.  On at least Panther however it appears that panels
are not switched properly through bindings alone; a notification
must be used as a backup.

(4.1)
*/
- (void)
tableViewSelectionDidChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self setCurrentPreferenceCollectionIndexes:[self->sourceListTableView selectedRowIndexes]];
}// tableViewSelectionDidChange:


#pragma mark NSToolbarDelegate


/*!
Responds when the specified kind of toolbar item should be
constructed for the given toolbar.

(4.1)
*/
- (NSToolbarItem*)
toolbar:(NSToolbar*)				toolbar
itemForItemIdentifier:(NSString*)	itemIdentifier
willBeInsertedIntoToolbar:(BOOL)	flag
{
#pragma unused(toolbar, flag)
	NSToolbarItem*		result = [[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
	Panel_ViewManager*	itemPanel = [self->panelsByID objectForKey:itemIdentifier];
	
	
	// NOTE: no need to handle standard items here
	assert(nil != itemPanel);
	[result setLabel:[itemPanel panelName]];
	[result setImage:[itemPanel panelIcon]];
	[result setAction:itemPanel.panelDisplayAction];
	[result setTarget:itemPanel.panelDisplayTarget];
	
	return result;
}// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:


/*!
Returns the identifiers for the kinds of items that can appear
in the given toolbar.

(4.1)
*/
- (NSArray*)
toolbarAllowedItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	NSArray*	result = [[self->panelIDArray copy] autorelease];
	
	
	return result;
}// toolbarAllowedItemIdentifiers:


/*!
Returns the identifiers for the items that will appear in the
given toolbar whenever the user has not customized it.  Since
this toolbar is not customized, the result is the same as
"toolbarAllowedItemIdentifiers:".

(4.1)
*/
- (NSArray*)
toolbarDefaultItemIdentifiers:(NSToolbar*)	toolbar
{
	return [self toolbarAllowedItemIdentifiers:toolbar];
}// toolbarDefaultItemIdentifiers:


/*!
Returns the identifiers for the items that may appear in a
“selected” state (which is all of them).

(4.1)
*/
- (NSArray*)
toolbarSelectableItemIdentifiers:(NSToolbar*)	toolbar
{
	return [self toolbarAllowedItemIdentifiers:toolbar];
}// toolbarSelectableItemIdentifiers:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.1)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	assert(nil != windowFirstResponder);
	assert(nil != windowLastResponder);
	assert(nil != containerTabView);
	assert(nil != sourceListBackdrop);
	assert(nil != sourceListContainer);
	assert(nil != sourceListTableView);
	assert(nil != sourceListSegmentedControl);
	assert(nil != sourceListHelpButton);
	assert(nil != mainViewHelpButton);
	assert(nil != verticalSeparator);
	
	NSRect const	kOriginalContainerFrame = [self->containerTabView frame];
	
	
	// create all panels
	{
		Panel_ViewManager*		newViewMgr = nil;
		
		
		for (Class viewMgrClass in
				@[
					[PrefPanelGeneral_ViewManager class],
					[PrefPanelMacros_ViewManager class],
					[PrefPanelWorkspaces_ViewManager class],
					[PrefPanelSessions_ViewManager class],
					[PrefPanelTerminals_ViewManager class],
					[PrefPanelFormats_ViewManager class],
					[PrefPanelTranslations_ViewManager class],
					[PrefPanelFullScreen_ViewManager class]
				])
		{
			newViewMgr = [[viewMgrClass alloc] init];
			[self->panelIDArray addObject:[newViewMgr panelIdentifier]];
			[self->panelsByID setObject:newViewMgr forKey:[newViewMgr panelIdentifier]]; // retains allocated object
			newViewMgr.panelDisplayAction = @selector(performDisplaySelfThroughParent:);
			newViewMgr.panelDisplayTarget = newViewMgr;
			newViewMgr.panelParent = self;
			if ([newViewMgr conformsToProtocol:@protocol(Panel_Parent)])
			{
				for (Panel_ViewManager* childViewMgr in [REINTERPRET_CAST(newViewMgr, id< Panel_Parent >) panelParentEnumerateChildViewManagers])
				{
					childViewMgr.panelDisplayAction = @selector(performDisplaySelfThroughParent:);
					childViewMgr.panelDisplayTarget = childViewMgr;
				}
			}
			[newViewMgr release], newViewMgr = nil;
		}
	}
	
	// create toolbar; has to be done programmatically, because
	// IB only supports them in 10.5; which makes sense, you know,
	// since toolbars have only been in the OS since 10.0, and
	// hardly any applications would have found THOSE useful...
	{
		NSString*		toolbarID = @"PreferencesToolbar"; // do not ever change this; that would only break user preferences
		NSToolbar*		windowToolbar = [[[NSToolbar alloc] initWithIdentifier:toolbarID] autorelease];
		
		
		[windowToolbar setAllowsUserCustomization:NO];
		[windowToolbar setAutosavesConfiguration:NO];
		[windowToolbar setDisplayMode:NSToolbarDisplayModeIconAndLabel];
		[windowToolbar setSizeMode:NSToolbarSizeModeRegular];
		[windowToolbar setDelegate:self];
		[[self window] setToolbar:windowToolbar];
	}
	
	// disable the zoom button (Full Screen is graphically glitchy);
	// see also code in "didEndSheet:"
	{
		// you would think this should be NSWindowFullScreenButton but since Apple
		// redefined the Zoom button for Full Screen on later OS versions, it is
		// necessary to disable Zoom instead
		NSButton*		zoomButton = [self.window standardWindowButton:NSWindowZoomButton];
		
		
		zoomButton.enabled = NO;
	}
	
	// ensure that the changes above are re-applied after sheets close
	[self whenObject:self.window postsNote:NSWindowDidEndSheetNotification
						performSelector:@selector(didEndSheet:)];
	
	self.window.excludedFromWindowsMenu = YES;
	
	// “indent” the toolbar items slightly so they are further away
	// from the window’s controls (such as the close button)
	for (UInt16 i = 0; i < 1/* arbitrary */; ++i)
	{
		[[[self window] toolbar] insertItemWithItemIdentifier:NSToolbarSpaceItemIdentifier atIndex:0];
	}
	
	// remember how much bigger the window’s content is than the container view
	{
		NSRect	contentFrame = [[self window] contentRectForFrameRect:[[self window] frame]];
		
		
		self->extraWindowContentSize = NSMakeSize(NSWidth(contentFrame) - NSWidth(kOriginalContainerFrame),
													NSHeight(contentFrame) - NSHeight(kOriginalContainerFrame));
	}
	
	// add each panel as a subview and hide all except the first;
	// give them all the initial size of the window (in reality
	// they will be changed to a different size before display)
	for (NSString* panelIdentifier in self->panelIDArray)
	{
		Panel_ViewManager*	viewMgr = [self->panelsByID objectForKey:panelIdentifier];
		NSTabViewItem*		tabItem = [[NSTabViewItem alloc] initWithIdentifier:panelIdentifier];
		NSView*				panelContainer = [viewMgr managedView];
		NSRect				panelFrame = kOriginalContainerFrame;
		NSSize				panelIdealSize = panelFrame.size;
		
		
		[viewMgr.delegate panelViewManager:viewMgr requestingIdealSize:&panelIdealSize];
		
		// due to layout constraints, it is sufficient to make the
		// panel container match the parent view frame (except with
		// local origin); once the window is resized to its target
		// frame, the panel will automatically resize again
		panelFrame.origin.x = 0;
		panelFrame.origin.y = 0;
		[panelContainer setFrame:panelFrame];
		[self->panelsByID setObject:viewMgr forKey:panelIdentifier];
		
		// initialize the “remembered window size” for each panel
		// (this changes whenever the user resizes the window)
		{
			NSArray*	sizeArray = nil;
			NSSize		windowSize = NSMakeSize(panelIdealSize.width + self->extraWindowContentSize.width,
												panelIdealSize.height + self->extraWindowContentSize.height);
			
			
			// only inspector-style windows include space for a source list
			if (kPanel_EditTypeInspector != [viewMgr panelEditType])
			{
				windowSize.width -= NSWidth([self->sourceListContainer frame]);
			}
			
			// choose a frame size that uses the panel’s ideal size
			sizeArray = @[
							[NSNumber numberWithFloat:windowSize.width],
							[NSNumber numberWithFloat:windowSize.height],
						];
			[self->windowSizesByID setObject:sizeArray forKey:panelIdentifier];
			
			// also require (for now, at least) that the window be
			// no smaller than this initial size, whenever this
			// particular panel is displayed
			[self->windowMinSizesByID setObject:sizeArray forKey:panelIdentifier];
		}
		
		[tabItem setView:panelContainer];
		[tabItem setInitialFirstResponder:[viewMgr logicalFirstResponder]];
		
		[self->containerTabView addTabViewItem:tabItem];
		[tabItem release];
	}
	
	// enable drag-and-drop in the source list
	[self->sourceListTableView registerForDraggedTypes:@[kMy_PrefsWindowSourceListDataType]];
	
	// be notified of source list changes; not strictly necessary on
	// newer OS versions (where bindings work perfectly) but it seems
	// to be necessary for correct behavior on Panther at least
	[self whenObject:self->sourceListTableView postsNote:NSTableViewSelectionDidChangeNotification
						performSelector:@selector(tableViewSelectionDidChange:)];
	
	// find out when the window closes or becomes key
	[self whenObject:self.window postsNote:NSWindowDidBecomeKeyNotification
					performSelector:@selector(windowDidBecomeKey:)];
	[self whenObject:self.window postsNote:NSWindowWillCloseNotification
					performSelector:@selector(windowWillClose:)];
}// windowDidLoad


#pragma mark NSWindowDelegate


/*!
Responds to becoming the key window by selecting a panel
if none has been selected.

(4.1)
*/
- (void)
windowDidBecomeKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	// if no panel has been selected, choose one (this is the normal
	// case but since it is possible for actions such as “Add to
	// Preferences” to request direct access to panels, it may be
	// that the first open request has chosen a specific panel)
	if (nil == self->activePanel)
	{
		// show the first panel while simultaneously resizing the
		// window to an appropriate frame and showing any auxiliary
		// interface that the panel requests (such as a source list)
		[self displayPanel:[self->panelsByID objectForKey:[self->panelIDArray objectAtIndex:0]] withAnimation:NO];
		[self setSourceListHidden:YES newWindowFrame:self.window.frame];
	}
}// windowDidBecomeKey:


/*!
Responds to a close of the preferences window by saving
any changes.

(4.1)
*/
- (void)
windowWillClose:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	Preferences_Result	prefsResult = Preferences_Save();
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "failed to save preferences to disk, error", prefsResult);
	}
	
	// a window does not receive a “did close” message so there is
	// no choice except to notify a panel of both “will” and ”did”
	// messages at the same time
	[self->activePanel.delegate panelViewManager:self->activePanel willChangePanelVisibility:kPanel_VisibilityHidden];
	[self->activePanel.delegate panelViewManager:self->activePanel didChangePanelVisibility:kPanel_VisibilityHidden];
}// windowWillClose:


#pragma mark Panel_Parent


/*!
Switches to the specified panel.

(4.1)
*/
- (void)
panelParentDisplayChildWithIdentifier:(NSString*)	anIdentifier
withAnimation:(BOOL)								isAnimated
{
	[self displayPanelOrTabWithIdentifier:anIdentifier withAnimation:isAnimated];
}// panelParentDisplayChildWithIdentifier:withAnimation:


/*!
Returns the primary panels displayed in this window.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	// TEMPORARY; this is not an ordered list (it should be)
	return [self->panelsByID objectEnumerator];
}// panelParentEnumerateChildViewManagers


#pragma mark Notifications


/*!
Handles a bug in Mac OS X where a sheet will restore
access to all window buttons (overriding changes made
by "windowDidLoad").

(4.1)
*/
- (void)
didEndSheet:(NSNotification*)	aNotification
{
	// re-apply the window button changes made in "windowDidLoad"
	NSButton*		zoomButton = [self.window standardWindowButton:NSWindowZoomButton];
	
	
	zoomButton.enabled = NO;
}// didEndSheet:


@end // PrefsWindow_Controller


#pragma mark -
@implementation PrefsWindow_Controller (PrefsWindow_ControllerInternal)


#pragma mark Class Methods


/*!
Tries to match the given phrase against the search data for
preferences panels and shows the user which panels (and
possibly tabs) are relevant.

To cancel the search, pass an empty or "nil" string.

(4.1)
*/
+ (void)
popUpResultsForQuery:(NSString*)	aQueryString
inSearchField:(NSSearchField*)		aSearchField
{
	NSUInteger const			kSetCapacity = 25; // IMPORTANT: set to value greater than number of defined panels and tabs!
	NSMutableSet*				matchingPanelIDs = [[NSMutableSet alloc] initWithCapacity:kSetCapacity];
	NSMutableArray*				orderedMatchingPanels = [NSMutableArray arrayWithCapacity:kSetCapacity];
	NSDictionary*				searchData = gSearchDataDictionary();
	NSArray*					queryStringWords = [aQueryString componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]; // LOCALIZE THIS
	PrefsWindow_Controller*		controller = [self sharedPrefsWindowController];
	
	
	// TEMPORARY; do the lazy thing and iterate over the data in
	// the exact form it is stored in the bundle dictionary; the
	// data is ordered in the way most convenient for writing so
	// it’s not optimal for searching; if performance proves to
	// be bad, the data could be indexed in memory but for now
	// it is probably fine this way
	for (NSString* candidatePanelIdentifier in searchData)
	{
		id		candidatePanelData = [searchData objectForKey:candidatePanelIdentifier];
		
		
		if (NO == [candidatePanelData respondsToSelector:@selector(objectAtIndex:)])
		{
			Console_Warning(Console_WriteValueCFString, "panel search data should map to an array, for key",
							BRIDGE_CAST(candidatePanelIdentifier, CFStringRef));
		}
		else
		{
			NSArray*	searchPhraseArray = REINTERPRET_CAST(candidatePanelData, NSArray*);
			
			
			for (id object in searchPhraseArray)
			{
				// currently, every object in the array must be a string
				if (NO == [object respondsToSelector:@selector(characterAtIndex:)])
				{
					Console_Warning(Console_WriteValueCFString, "array in panel search data should contain only string values, for key",
									BRIDGE_CAST(candidatePanelIdentifier, CFStringRef));
				}
				else
				{
					// TEMPORARY; looking up localized strings on every query is
					// wasteful; since the search data ought to be reverse-indexed
					// anyway, a better solution is to reverse-index the data and
					// simultaneously capture all localized phrases
					NSString*			asKeyPhrase = REINTERPRET_CAST(object, NSString*);
					CFRetainRelease		localizedKeyPhrase(CFBundleCopyLocalizedString(AppResources_ReturnApplicationBundle(),
																						BRIDGE_CAST(asKeyPhrase, CFStringRef), nullptr/* default value */,
																						CFSTR("PreferencesSearch")/* base name of ".strings" file */),
															true/* is retained */);
					
					
					if (localizedKeyPhrase.exists())
					{
						asKeyPhrase = BRIDGE_CAST(localizedKeyPhrase.returnCFStringRef(), NSString*);
					}
					
					if ([self queryString:aQueryString withWordArray:queryStringWords matchesTargetPhrase:asKeyPhrase])
					{
						[matchingPanelIDs addObject:candidatePanelIdentifier];
					}
				}
			}
		}
	}
	
	// find the panel objects that correspond to the matching IDs;
	// display each panel only once, and iterate over panels in
	// the order they appear in the window so that results are
	// nicely sorted (this assumes that tabs also iterate in the
	// order that tabs are displayed; see "GenericPanelTabs.mm")
	for (NSString* mainPanelIdentifier in controller->panelIDArray)
	{
		Panel_ViewManager*		mainPanel = [controller->panelsByID objectForKey:mainPanelIdentifier];
		
		
		if ([matchingPanelIDs containsObject:mainPanelIdentifier])
		{
			// match applies to an entire category
			[orderedMatchingPanels addObject:mainPanel];
		}
		else
		{
			// target panel is not a whole category; search for any
			// tabs that were included in the matched set
			if ([mainPanel conformsToProtocol:@protocol(Panel_Parent)])
			{
				id< Panel_Parent >	asParent = REINTERPRET_CAST(mainPanel, id< Panel_Parent >);
				
				
				for (Panel_ViewManager* subPanel in [asParent panelParentEnumerateChildViewManagers])
				{
					if ([matchingPanelIDs containsObject:[subPanel panelIdentifier]])
					{
						[orderedMatchingPanels addObject:subPanel];
					}
				}
			}
		}
	}
	[matchingPanelIDs release];
	
	// display a menu with search results
	{
		static NSMenu*		popUpResultsMenu = nil;
		
		
		if (nil != popUpResultsMenu)
		{
			// clear current menu
			[popUpResultsMenu removeAllItems];
		}
		else
		{
			// create new menu
			popUpResultsMenu = [[NSMenu alloc] initWithTitle:@""];
		}
		
		// pop up a menu of results whose commands will display the matching panels
		for (Panel_ViewManager* panelObject in orderedMatchingPanels)
		{
			NSImage*				panelIcon = [[panelObject panelIcon] copy];
			NSString*				panelName = [panelObject panelName];
			NSString*				displayItemName = (nil != panelName)
														? panelName
														: @"";
			NSMenuItem*				panelDisplayItem = [[NSMenuItem alloc] initWithTitle:displayItemName
																							action:panelObject.panelDisplayAction
																							keyEquivalent:@""];
			
			
			panelIcon.size = NSMakeSize(24, 24); // shrink default image, which is too large
			[panelDisplayItem setImage:panelIcon];
			[panelDisplayItem setTarget:panelObject.panelDisplayTarget]; // action is set in initializer above
			[popUpResultsMenu addItem:panelDisplayItem];
			[panelIcon release];
			[panelDisplayItem release];
		}
		
		if ([orderedMatchingPanels count] > 0)
		{
			NSPoint		menuLocation = NSMakePoint(4/* arbitrary */, NSHeight([aSearchField bounds]) + 4/* arbitrary */);
			
			
			[popUpResultsMenu popUpMenuPositioningItem:nil/* menu item */ atLocation:menuLocation inView:aSearchField];
		}
	}
}// popUpResultsForQuery:inSearchField:


/*!
Returns YES if the specified strings match according to
the rules of searching preferences.

(4.1)
*/
+ (BOOL)
queryString:(NSString*)				aQueryString
matchesTargetPhrase:(NSString*)		aSetPhrase
{
	NSRange		lowestCommonRange = NSMakeRange(0, MIN([aSetPhrase length], [aQueryString length]));
	BOOL		result = NO;
	
	
	// for now, search against the beginning of phrases; could later
	// add more intelligence such as introducing synonyms,
	// misspellings and other “fuzzy” logic
	if (NSOrderedSame == [aSetPhrase compare:aQueryString options:(NSCaseInsensitiveSearch | NSDiacriticInsensitiveSearch)
												range:lowestCommonRange])
	{
		result = YES;
	}
	
	return result;
}// queryString:matchesTargetPhrase:


/*!
Returns YES if the specified strings match according to
the rules of searching preferences.  The given array of
words should be consistent with the query string but
split into tokens (e.g. on whitespace).

(4.1)
*/
+ (BOOL)
queryString:(NSString*)				aQueryString
withWordArray:(NSArray*)			anArrayOfWordsInQueryString
matchesTargetPhrase:(NSString*)		aSetPhrase
{
	BOOL	result = [self queryString:aQueryString matchesTargetPhrase:aSetPhrase];
	
	
	if (NO == result)
	{
		// if the phrase doesn’t exactly match, try to
		// match individual words from the query string
		// (in the future, might want to return a “rank”
		// to indicate how closely the result matched)
		for (NSString* word in anArrayOfWordsInQueryString)
		{
			if ([self queryString:word matchesTargetPhrase:aSetPhrase])
			{
				result = YES;
				break;
			}
		}
	}
	
	return result;
}// queryString:withWordArray:matchesTargetPhrase:


#pragma mark New Methods


/*!
Returns the class of preferences that are currently being
edited.  This determines the contents of the source list,
the type of context that is created when the user adds
something new, etc.

(4.1)
*/
- (Quills::Prefs::Class)
currentPreferencesClass
{
	Quills::Prefs::Class	result = [self->activePanel preferencesClass];
	
	
	return result;
}// currentPreferencesClass


/*!
Returns the context from which the user interface is
currently displayed (and the context that is changed when
the user modifies the interface).

For panels that use the source list this will match the
selection in the list.  For other panels it will be the
default context for the preferences class of the active
panel.

(4.1)
*/
- (Preferences_ContextRef)
currentPreferencesContext
{
	Preferences_ContextRef		result = nullptr;
	
	
	if (kPanel_EditTypeInspector == [self->activePanel panelEditType])
	{
		UInt16						selectedIndex = [self->sourceListTableView selectedRow];
		PrefsWindow_Collection*		selectedCollection = [self->currentPreferenceCollections objectAtIndex:selectedIndex];
		
		
		result = [selectedCollection preferencesContext];
	}
	else
	{
		Preferences_Result	prefsResult = Preferences_GetDefaultContext(&result, [self currentPreferencesClass]);
		
		
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	assert(Preferences_ContextIsValid(result));
	return result;
}// currentPreferencesContext


/*!
A selector of the form required by NSSavePanel; it responds to
an NSOKButton return code by saving the current preferences
context to a file.

(4.1)
*/
- (void)
didEndExportPanel:(NSSavePanel*)	aPanel
returnCode:(int)					aReturnCode
contextInfo:(void*)					aContextPtr
{
#pragma unused(aContextPtr)
	if (NSOKButton == aReturnCode)
	{
		// export the contents of the preferences context (the Preferences
		// module automatically ignores settings that are not part of the
		// right class, e.g. so that Default does not create a huge file)
		CFURLRef			asURL = BRIDGE_CAST([aPanel URL], CFURLRef);
		Preferences_Result	writeResult = Preferences_ContextSaveAsXMLFileURL([self currentPreferencesContext], asURL);
		
		
		if (kPreferences_ResultOK != writeResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteValueCFString, "unable to save file, URL",
							(CFStringRef)[((NSURL*)asURL) absoluteString]);
			Console_Warning(Console_WriteValue, "error", writeResult);
		}
	}
}// didEndExportPanel:returnCode:contextInfo:


/*!
A selector of the form required by NSOpenPanel; it responds to
an NSOKButton return code by opening the selected file(s).
The files are opened via Apple Events because there are already
event handlers defined that have the appropriate effect on
Preferences.

(4.1)
*/
- (void)
didEndImportPanel:(NSOpenPanel*)	aPanel
returnCode:(int)					aReturnCode
contextInfo:(void*)					aContextPtr
{
#pragma unused(aContextPtr)
	if (NSOKButton == aReturnCode)
	{
		// open the files via Apple Events
		for (NSURL* currentFileURL in [aPanel URLs])
		{
			FSRef		fileRef;
			OSStatus	error = noErr;
			
			
			if (CFURLGetFSRef(BRIDGE_CAST(currentFileURL, CFURLRef), &fileRef))
			{
				error = FileUtilities_OpenDocument(fileRef);
			}
			else
			{
				error = kURLInvalidURLError;
			}
			
			if (noErr != error)
			{
				// TEMPORARY; should probably display a more user-friendly alert for this...
				Sound_StandardAlert();
				Console_Warning(Console_WriteValueCFString, "unable to open file, URL",
								BRIDGE_CAST([currentFileURL absoluteString], CFStringRef));
				Console_Warning(Console_WriteValue, "error", error);
			}
		}
	}
}// didEndImportPanel:returnCode:contextInfo:


/*!
If the specified panel is not the active panel, hides
the active panel and then displays the given panel.

The size of the window is synchronized with the new
panel, and the previous window size is saved (each
panel has a unique window size, allowing the user to
choose the ideal layout for each one).

(4.1)
*/
- (void)
displayPanel:(Panel_ViewManager< PrefsWindow_PanelInterface >*)		aPanel
withAnimation:(BOOL)												isAnimated
{
	if (aPanel != self->activePanel)
	{
		NSRect	newWindowFrame = NSZeroRect; // set later
		BOOL	wasShowingSourceList = (kPanel_EditTypeInspector == [self->activePanel panelEditType]);/*((NO == self->sourceListContainer.isHidden) &&
										(self->sourceListContainer.frame.size.width > 0));*/
		BOOL	willShowSourceList = (kPanel_EditTypeInspector == [aPanel panelEditType]); // initially...
		
		
		if ((nil != self->activePanel) && (NO == EventLoop_IsWindowFullScreen(self.window)))
		{
			// remember the window size that the user wanted for the previous panel
			NSRect		contentRect = [[self window] contentRectForFrameRect:[[self window] frame]];
			NSSize		containerSize = NSMakeSize(NSWidth(contentRect), NSHeight(contentRect));
			NSArray*	sizeArray = @[
										[NSNumber numberWithFloat:containerSize.width],
										[NSNumber numberWithFloat:containerSize.height],
									];
			
			
			[self->windowSizesByID setObject:sizeArray forKey:[self->activePanel panelIdentifier]];
		}
		
		// show the new panel
		{
			Panel_ViewManager< PrefsWindow_PanelInterface >*	oldPanel = self->activePanel;
			Panel_ViewManager< PrefsWindow_PanelInterface >*	newPanel = aPanel;
			
			
			[oldPanel.delegate panelViewManager:oldPanel willChangePanelVisibility:kPanel_VisibilityHidden];
			[newPanel.delegate panelViewManager:newPanel willChangePanelVisibility:kPanel_VisibilityDisplayed];
			self->activePanel = newPanel;
			[[[self window] toolbar] setSelectedItemIdentifier:[newPanel panelIdentifier]];
			[self->containerTabView selectTabViewItemWithIdentifier:[newPanel panelIdentifier]];
			[oldPanel.delegate panelViewManager:oldPanel didChangePanelVisibility:kPanel_VisibilityHidden];
			[newPanel.delegate panelViewManager:newPanel didChangePanelVisibility:kPanel_VisibilityDisplayed];
		}
		
		// set the window to the size for the new panel (if none, use the
		// size that was originally used for the panel in its NIB)
		if (NO == EventLoop_IsWindowFullScreen(self.window))
		{
			NSRect		originalFrame = self.window.frame;
			NSRect		newContentRect = [self.window contentRectForFrameRect:originalFrame]; // initially...
			NSSize		minSize = [self.window minSize];
			NSArray*	sizeArray = [self->windowSizesByID objectForKey:[aPanel panelIdentifier]];
			NSArray*	minSizeArray = [self->windowMinSizesByID objectForKey:[aPanel panelIdentifier]];
			
			
			assert(nil != sizeArray);
			assert(nil != minSizeArray);
			
			// constrain the window appropriately for the panel that is displayed
			{
				assert(2 == [minSizeArray count]);
				Float32		newMinWidth = [[minSizeArray objectAtIndex:0] floatValue];
				Float32		newMinHeight = [[minSizeArray objectAtIndex:1] floatValue];
				
				
				minSize = NSMakeSize(newMinWidth, newMinHeight);
				[self.window setContentMinSize:minSize];
			}
			
			// start with the restored size but respect the minimum size
			assert(2 == [sizeArray count]);
			newContentRect.size.width = MAX([[sizeArray objectAtIndex:0] floatValue], minSize.width);
			newContentRect.size.height = MAX([[sizeArray objectAtIndex:1] floatValue], minSize.height);
			
			// since the coordinate system anchors the window at the
			// bottom-left corner and the intent is to keep the top-left
			// corner constant, calculate an appropriate new origin when
			// changing the size (don’t just change the size by itself);
			// this calculation must be performed prior to seeking the
			// intersection with the screen rectangle so that a proposed
			// offscreen location is seen by the intersection check
			newWindowFrame = [[self window] frameRectForContentRect:newContentRect];
			newWindowFrame.origin.y += (NSHeight(originalFrame) - NSHeight(newWindowFrame));
			
			// try to keep the window on screen
			{
				NSRect		onScreenFrame = NSIntersectionRect(newWindowFrame, self.window.screen.visibleFrame);
				
				
				// see if the window is going to remain entirely on the screen...
				if (NO == NSEqualRects(onScreenFrame, newWindowFrame))
				{
					// window will become partially offscreen; try to move it onscreen
					if ((newWindowFrame.origin.x + NSWidth(newWindowFrame)) >
						(self.window.screen.visibleFrame.origin.x + NSWidth(self.window.screen.visibleFrame)))
					{
						// adjust to the left
						newWindowFrame.origin.x = (onScreenFrame.origin.x + NSWidth(onScreenFrame) - NSWidth(newWindowFrame));
					}
					else
					{
						// adjust to the right
						newWindowFrame.origin.x = onScreenFrame.origin.x;
					}
					newWindowFrame.origin.y = onScreenFrame.origin.y;
				}
			}
		}
		else
		{
			// full-screen; size is not changing
			newWindowFrame = self.window.frame;
		}
		
		// if necessary display the source list of available preference collections
		if (willShowSourceList != wasShowingSourceList)
		{
			if (willShowSourceList)
			{
				[self rebuildSourceList];
			}
			
			// resize the window and change the source list visibility
			// at the same time for fluid animation
			[self setSourceListHidden:(NO == willShowSourceList) newWindowFrame:newWindowFrame];
		}
		else
		{
			// resize the window normally
			[self.window setFrame:newWindowFrame display:YES animate:isAnimated];
			
			// source list is still needed, but it will have new content (note
			// that this step is implied if the visibility changes above)
			[self rebuildSourceList];
		}
	}
}// displayPanel:withAnimation:


/*!
If the specified identifier exactly matches that of a
top category panel, this method behaves the same as
"displayPanel:withAnimation:".

If however the given identifier can only be resolved
by the child panels of the main categories, this
method will first display the parent panel (with or
without animation, as indicated) and then issue a
request to switch to the appropriate tab.  In this
way, you can pinpoint a specific part of the window.

(4.1)
*/
- (void)
displayPanelOrTabWithIdentifier:(NSString*)		anIdentifier
withAnimation:(BOOL)							isAnimated
{
	Panel_ViewManager< PrefsWindow_PanelInterface >*	mainPanel = nil;
	Panel_ViewManager< PrefsWindow_PanelInterface >*	candidateMainPanel = nil;
	Panel_ViewManager*									childPanel = nil;
	Panel_ViewManager*									candidateChildPanel = nil;
	
	
	// force the user interface to load (otherwise the panels
	// may not exist and may not be selectable)
	[self.window makeKeyAndOrderFront:nil];
	
	// given the way panels are currently indexed, it is
	// necessary to search main categories and then
	// search panel children (there is no direct map)
	for (NSString* panelIdentifier in self->panelIDArray)
	{
		if ([[panelIdentifier lowercaseString] isEqualToString:[anIdentifier lowercaseString]])
		{
			mainPanel = [self->panelsByID objectForKey:panelIdentifier];
			break;
		}
	}
	
	if (nil == mainPanel)
	{
		// requested identifier does not exactly match a top-level
		// category; search for a child panel that matches
		for (candidateMainPanel in [self->panelsByID objectEnumerator])
		{
			if ([candidateMainPanel conformsToProtocol:@protocol(Panel_Parent)])
			{
				for (candidateChildPanel in [STATIC_CAST(candidateMainPanel, id< Panel_Parent >) panelParentEnumerateChildViewManagers])
				{
					NSString*	childIdentifier = [candidateChildPanel panelIdentifier];
					
					
					if ([[childIdentifier lowercaseString] isEqualTo:[anIdentifier lowercaseString]])
					{
						mainPanel = candidateMainPanel;
						childPanel = candidateChildPanel;
						break;
					}
				}
			}
		}
	}
	
	if (nil != mainPanel)
	{
		[self displayPanel:mainPanel withAnimation:isAnimated];
	}
	
	if (nil != childPanel)
	{
		[childPanel performDisplaySelfThroughParent:nil];
	}
	
	if ((nil == mainPanel) && (nil == childPanel))
	{
		Console_Warning(Console_WriteValueCFString, "unable to display panel with identifier", BRIDGE_CAST(anIdentifier, CFStringRef));
	}
}// displayPanelOrTabWithIdentifier:withAnimation:


/*!
Called when a monitored preference is changed.  See the
initializer for the set of events that is monitored.

(4.1)
*/
- (void)
model:(ListenerModel_Ref)				aModel
preferenceChange:(ListenerModel_Event)	anEvent
context:(void*)							aContext
{
#pragma unused(aModel, aContext)
	switch (anEvent)
	{
	case kPreferences_ChangeContextName:
		// a context has been renamed; refresh the list
		[self rebuildSourceList];
		break;
	
	case kPreferences_ChangeNumberOfContexts:
		// contexts were added or removed; destroy and rebuild the list
		[self rebuildSourceList];
		break;
	
	default:
		// ???
		break;
	}
}// model:preferenceChange:context:


/*!
Rebuilds the array that the source list is bound to, causing
potentially a new list of preferences contexts to be displayed.
The current panel’s preferences class is used to find an
appropriate list of contexts.

(4.1)
*/
- (void)
rebuildSourceList
{
	std::vector< Preferences_ContextRef >	contextList;
	NSIndexSet*								previousSelection = [self currentPreferenceCollectionIndexes];
	id										selectedObject = nil;
	
	
	if (nil == previousSelection)
	{
		previousSelection = [NSIndexSet indexSetWithIndex:0];
	}
	
	if ([previousSelection firstIndex] < [self->currentPreferenceCollections count])
	{
		selectedObject = [self->currentPreferenceCollections objectAtIndex:[previousSelection firstIndex]];
	}
	
	[self willChangeValueForKey:@"currentPreferenceCollections"];
	
	[self->currentPreferenceCollections removeAllObjects];
	if (Preferences_GetContextsInClass([self currentPreferencesClass], contextList))
	{
		// the first item in the returned list is always the default
		BOOL	haveAddedDefault = NO;
		
		
		for (auto prefsContextRef : contextList)
		{
			BOOL						isDefault = (NO == haveAddedDefault);
			PrefsWindow_Collection*		newCollection = [[PrefsWindow_Collection alloc]
															initWithPreferencesContext:prefsContextRef
																						asDefault:isDefault];
			
			
			[self->currentPreferenceCollections addObject:newCollection];
			[newCollection release], newCollection = nil;
			haveAddedDefault = YES;
		}
	}
	
	[self didChangeValueForKey:@"currentPreferenceCollections"];
	
	// attempt to preserve the selection
	{
		unsigned int	newIndex = [self->currentPreferenceCollections indexOfObject:selectedObject];
		
		
		if (NSNotFound != newIndex)
		{
			[self setCurrentPreferenceCollectionIndexes:[NSIndexSet indexSetWithIndex:newIndex]];
		}
	}
}// rebuildSourceList


/*!
Changes the visibility of the source list and the size of
the window, animating any other views to match.

(4.1)
*/
- (void)
setSourceListHidden:(BOOL)		aHiddenFlag
newWindowFrame:(NSRect)			aNewWindowFrame
{
	NSRect		newWindowContentFrame = [self.window contentRectForFrameRect:aNewWindowFrame];
	NSRect		listFrame;
	NSRect		panelFrame;
	
	
	// the darker backdrop behind the list makes the
	// slide animation less jarring
	self->sourceListBackdrop.hidden = aHiddenFlag;
	
	// calculate the new size and location of the panel
	panelFrame = NSMakeRect((aHiddenFlag) ? 0 : NSWidth(sourceListContainer.frame), containerTabView.frame.origin.y,
							NSWidth(newWindowContentFrame) - ((aHiddenFlag) ? 0 : self->extraWindowContentSize.width),
							NSHeight(newWindowContentFrame) - self->extraWindowContentSize.height);
	
	// calculate the new size and location of the list,
	// potentially moving it to an invisible location (after
	// the animation ends, the list is hidden anyway)
	listFrame = sourceListContainer.frame;
	listFrame.origin.x = ((aHiddenFlag) ? -self->extraWindowContentSize.width : 0); // “slide” effect
	listFrame.size.height = NSHeight(panelFrame);
	
	// animate changes to other views (NOTE: can replace this with a
	// more advanced block-based approach when using a later SDK)
	dispatch_async(dispatch_get_main_queue(),
	^{
		// all of the changes in this block will animate simultaneously
		[NSAnimationContext beginGrouping];
		[[NSAnimationContext currentContext] setDuration:0.22];
		{
			[self.window.animator setFrame:aNewWindowFrame display:YES];
			[[self->containerTabView animator] setFrame:panelFrame];
			if (NO == aHiddenFlag)
			{
				sourceListContainer.hidden = aHiddenFlag;
				sourceListSegmentedControl.hidden = aHiddenFlag;
				sourceListHelpButton.hidden = (NO == aHiddenFlag);
				verticalSeparator.hidden = aHiddenFlag;
				mainViewHelpButton.hidden = aHiddenFlag;
			}
			[sourceListContainer.animator setFrame:listFrame];
			[sourceListContainer.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[sourceListSegmentedControl.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[verticalSeparator.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[mainViewHelpButton.animator setAlphaValue:((aHiddenFlag) ? 0.0 : 1.0)];
			[sourceListHelpButton.animator setAlphaValue:((aHiddenFlag) ? 1.0 : 0.0)]; // this alternate help button appears when there is no list
			if (aHiddenFlag)
			{
				sourceListContainer.hidden = aHiddenFlag;
				sourceListSegmentedControl.hidden = aHiddenFlag;
				sourceListHelpButton.hidden = (NO == aHiddenFlag);
				verticalSeparator.hidden = aHiddenFlag;
				mainViewHelpButton.hidden = aHiddenFlag;
			}
		}
		[NSAnimationContext endGrouping];
	});
	// after the animation above completes, refresh the final view
	// (it is unclear why this step is needed; the symptom is that
	// when the source list is hidden, the tab view of the next panel
	// is sometimes not completely redrawn on its left side)
	dispatch_after(dispatch_time(0, 0.32 * NSEC_PER_SEC), dispatch_get_main_queue(),
	^{
		[containerTabView setNeedsDisplay:YES];
	});
}// setSourceListHidden:newWindowFrame:


@end // PrefsWindow_Controller (PrefsWindow_ControllerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
