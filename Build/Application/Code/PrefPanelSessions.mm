/*!	\file PrefPanelSessions.mm
	\brief Implements the Sessions panel of Preferences.
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

#import "PrefPanelSessions.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>
#import <map>

// Unix includes
extern "C"
{
#	include <netdb.h>
}
#import <strings.h>

// Mac includes
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <BoundName.objc++.h>
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
#import "HelpSystem.h"
#import "Keypads.h"
#import "Panel.h"
#import "Preferences.h"
#import "ServerBrowser.h"
#import "Session.h"
#import "UIStrings.h"
#import "VectorInterpreter.h"



#pragma mark Constants
namespace {

/*!
Used to distinguish different types of control-key
mappings.
*/
enum My_KeyType
{
	kMy_KeyTypeNone = 0,		//!< none of the mappings
	kMy_KeyTypeInterrupt = 1,	//!< “interrupt process” mapping
	kMy_KeyTypeResume = 2,		//!< “resume output” mapping
	kMy_KeyTypeSuspend = 3		//!< “suspend output” mapping
};

} // anonymous namespace

#pragma mark Types
namespace {

typedef std::map< UInt8, CFStringRef >	My_CharacterToCFStringMap;	//!< e.g. '\0' maps to CFSTR("^@")

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean		copyRemoteCommandLineString			(Session_Protocol, CFStringRef, UInt16, CFStringRef,
												 CFStringRef&);
UInt16		readPreferencesForRemoteServers		(Preferences_ContextRef, Boolean, Session_Protocol&,
												 CFStringRef&, UInt16&, CFStringRef&);

} // anonymous namespace


/*!
The private class interface.
*/
@interface PrefPanelSessions_ResourceViewManager (PrefPanelSessions_ResourceViewManagerInternal) //{

// new methods
	- (NSArray*)
	primaryDisplayBindingKeys;
	- (void)
	setCommandLineFromSession:(Preferences_ContextRef)_;
	- (void)
	updateCommandLineFromRemotePreferences;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelSessions_DataFlowViewManager (PrefPanelSessions_DataFlowViewManagerInternal) //{

// new methods
	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelSessions_GraphicsViewManager (PrefPanelSessions_GraphicsViewManagerInternal) //{

// new methods
	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelSessions_KeyboardViewManager (PrefPanelSessions_KeyboardViewManagerInternal) //{

// new methods
	- (NSArray*)
	primaryDisplayBindingKeys;
	- (void)
	resetGlobalState;

// accessors
	- (My_KeyType)
	editedKeyType;
	- (void)
	setEditedKeyType:(My_KeyType)_;

@end //}


#pragma mark Variables
namespace {

My_KeyType		gEditedKeyType = kMy_KeyTypeNone; // shared across all occurrences; see "setEditedKeyType:"

} // anonymous namespace



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
PrefPanelSessions_NewDataFlowPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads screen-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagLocalEchoEnabled);
	tagList.push_back(kPreferences_TagPasteNewLineDelay);
	tagList.push_back(kPreferences_TagScrollDelay);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewDataFlowPaneTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelSessions_NewGraphicsPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads screen-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagTektronixMode);
	tagList.push_back(kPreferences_TagTektronixPAGEClearsScreen);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewGraphicsPaneTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelSessions_NewKeyboardPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads screen-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagKeyInterruptProcess);
	tagList.push_back(kPreferences_TagKeySuspendOutput);
	tagList.push_back(kPreferences_TagKeyResumeOutput);
	tagList.push_back(kPreferences_TagMapArrowsForEmacs);
	tagList.push_back(kPreferences_TagEmacsMetaKey);
	tagList.push_back(kPreferences_TagMapDeleteToBackspace);
	tagList.push_back(kPreferences_TagNewLineMapping);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewKeyboardPaneTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelSessions_NewResourcePaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads screen-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagCommandLine);
	tagList.push_back(kPreferences_TagAssociatedFormatFavoriteDarkMode);
	tagList.push_back(kPreferences_TagAssociatedFormatFavoriteLightMode);
	tagList.push_back(kPreferences_TagAssociatedMacroSetFavorite);
	tagList.push_back(kPreferences_TagAssociatedTerminalFavorite);
	tagList.push_back(kPreferences_TagAssociatedTranslationFavorite);
	tagList.push_back(kPreferences_TagServerProtocol);
	tagList.push_back(kPreferences_TagServerHost);
	tagList.push_back(kPreferences_TagServerPort);
	tagList.push_back(kPreferences_TagServerUserID);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewResourcePaneTagSet


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelSessions_NewTagSet ()
{
	Preferences_TagSetRef	result = Preferences_NewTagSet(40); // arbitrary initial capacity
	Preferences_TagSetRef	resourceTags = PrefPanelSessions_NewResourcePaneTagSet();
	Preferences_TagSetRef	dataFlowTags = PrefPanelSessions_NewDataFlowPaneTagSet();
	Preferences_TagSetRef	keyboardTags = PrefPanelSessions_NewKeyboardPaneTagSet();
	Preferences_TagSetRef	graphicsTags = PrefPanelSessions_NewGraphicsPaneTagSet();
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_TagSetMerge(result, resourceTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, dataFlowTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, keyboardTags);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_TagSetMerge(result, graphicsTags);
	assert(kPreferences_ResultOK == prefsResult);
	
	Preferences_ReleaseTagSet(&resourceTags);
	Preferences_ReleaseTagSet(&dataFlowTags);
	Preferences_ReleaseTagSet(&keyboardTags);
	Preferences_ReleaseTagSet(&graphicsTags);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods
namespace {

/*!
Since everything is “really” a local Unix command, this
routine uses the given remote options and updates the
command line field with appropriate values.

Every value should be defined, but if a string is empty
or the port number is 0, it is skipped in the resulting
command line.

Returns "true" only if the update was successful.  The
result "outStringOrNull" is only defined if true is
returned.

(3.1)
*/
Boolean
copyRemoteCommandLineString		(Session_Protocol	inProtocol,
								 CFStringRef		inHostName,
								 UInt16				inPortNumber,
								 CFStringRef		inUserID,
								 CFStringRef&		outStringOrNull)
{
	CFRetainRelease		newCommandLineObject(CFStringCreateMutable(kCFAllocatorDefault,
																	0/* length, or 0 for unlimited */),
												CFRetainRelease::kAlreadyRetained);
	NSString*			portString = (0 == inPortNumber)
										? nil
										: [[NSNumber numberWithUnsignedInt:inPortNumber] stringValue];
	Boolean				result = false;
	
	
	outStringOrNull = nullptr; // initially...
	
	// the host field is required when updating the command line
	if ((nullptr != inHostName) && (0 == CFStringGetLength(inHostName)))
	{
		inHostName = nullptr;
	}
	
	if (newCommandLineObject.exists() && (nullptr != inHostName))
	{
		CFMutableStringRef	newCommandLineCFString = newCommandLineObject.returnCFMutableStringRef(); // for convenience in loop
		Boolean				standardLoginOptionAppend = false;
		Boolean				standardHostNameAppend = false;
		Boolean				standardPortAppend = false;
		Boolean				standardIPv6Append = false;
		
		
		// see what is available
		// NOTE: In the following, whitespace should probably be stripped
		//       from all field values first, since otherwise the user
		//       could enter a non-empty but blank value that would become
		//       a broken command line.
		if ((nullptr != inUserID) && (0 == CFStringGetLength(inUserID)))
		{
			inUserID = nullptr;
		}
		
		// TEMPORARY - convert all of this logic to Python!
		
		// set command based on the protocol, and add arguments
		// based on the specific command and the available data
		result = true; // initially...
		switch (inProtocol)
		{
		case kSession_ProtocolSFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/sftp"));
			if (nullptr != inHostName)
			{
				// sftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != inUserID)
				{
					CFStringAppend(newCommandLineCFString, inUserID);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if (nil != portString)
			{
				// sftp uses "-oPort=port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -oPort="));
				CFStringAppend(newCommandLineCFString, BRIDGE_CAST(portString, CFStringRef));
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			break;
		
		case kSession_ProtocolSSH1:
		case kSession_ProtocolSSH2:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ssh"));
			if (kSession_ProtocolSSH2 == inProtocol)
			{
				// -2: protocol version 2
				CFStringAppend(newCommandLineCFString, CFSTR(" -2 "));
			}
			else
			{
				// -1: protocol version 1
				CFStringAppend(newCommandLineCFString, CFSTR(" -1 "));
			}
			if (nil != portString)
			{
				// ssh uses "-p port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -p "));
				CFStringAppend(newCommandLineCFString, BRIDGE_CAST(portString, CFStringRef));
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardIPv6Append = true; // ssh supports a "-6" argument if the host is exactly in IP version 6 format
			standardLoginOptionAppend = true; // ssh supports a "-l userid" option
			standardHostNameAppend = true; // ssh supports a standalone server host argument
			break;
		
		default:
			// ???
			result = false;
			break;
		}
		
		if (result)
		{
			// since commands tend to follow similar conventions, this section
			// appends options in “standard” form if supported by the protocol
			// (if MOST commands do not have an option, append it in the above
			// switch, instead)
			if ((standardIPv6Append) && (nullptr != inHostName) &&
				(CFStringFind(inHostName, CFSTR(":"), 0/* options */).length > 0))
			{
				// -6: address is in IPv6 format
				CFStringAppend(newCommandLineCFString, CFSTR(" -6 "));
			}
			if ((standardLoginOptionAppend) && (nullptr != inUserID))
			{
				// standard form is a Unix command accepting a "-l" option
				// followed by the user ID for login
				CFStringAppend(newCommandLineCFString, CFSTR(" -l "));
				CFStringAppend(newCommandLineCFString, inUserID);
			}
			if ((standardHostNameAppend) && (nullptr != inHostName))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the host name of the server to connect to
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if ((standardPortAppend) && (nil != portString))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the port number on the server to connect to, AFTER the
				// standalone host name option
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, BRIDGE_CAST(portString, CFStringRef));
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			
			if (0 == CFStringGetLength(newCommandLineCFString))
			{
				result = false;
			}
			else
			{
				outStringOrNull = newCommandLineCFString;
				CFRetain(outStringOrNull);
				result = true;
			}
		}
	}
	
	return result;
}// copyRemoteCommandLineString


/*!
Reads all preferences from the specified context that define a
remote server, optionally using global defaults as a fallback
for undefined settings.

Every result is defined, even if it is an empty string or a
zero value.  CFRelease() should be called on all strings.

Returns the number of preferences read successfully, which is
normally 4.  Any unsuccessful reads will have arbitrary values.

(4.0)
*/
UInt16
readPreferencesForRemoteServers		(Preferences_ContextRef		inSettings,
									 Boolean					inSearchDefaults,
									 Session_Protocol&			outProtocol,
									 CFStringRef&				outHostNameCopy,
									 UInt16&					outPortNumber,
									 CFStringRef&				outUserIDCopy)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	UInt16					protocolValue = 0;
	UInt16					result = 0;
	
	
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerProtocol, sizeof(protocolValue),
												&protocolValue, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		outProtocol = STATIC_CAST(protocolValue, Session_Protocol);
		++result;
	}
	else
	{
		outProtocol = kSession_ProtocolSSH1; // arbitrary
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerHost, sizeof(outHostNameCopy),
												&outHostNameCopy, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outHostNameCopy = CFSTR(""); // arbitrary
		CFRetain(outHostNameCopy);
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerPort, sizeof(outPortNumber),
												&outPortNumber, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outPortNumber = 22; // arbitrary
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerUserID, sizeof(outUserIDCopy),
												&outUserIDCopy, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outUserIDCopy = CFSTR(""); // arbitrary
		CFRetain(outUserIDCopy);
	}
	
	return result;
}// readPreferencesForRemoteServers

} // anonymous namespace


#pragma mark -
@implementation PrefPanelSessions_ViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers = @[
										[[[PrefPanelSessions_ResourceViewManager alloc] init] autorelease],
										[[[PrefPanelSessions_DataFlowViewManager alloc] init] autorelease],
										[[[PrefPanelSessions_KeyboardViewManager alloc] init] autorelease],
										[[[PrefPanelSessions_GraphicsViewManager alloc] init] autorelease],
									];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Sessions"
										localizedName:NSLocalizedStringFromTable(@"Sessions", @"PrefPanelSessions",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelSessions"]
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


@end // PrefPanelSessions_ViewManager


#pragma mark -
@implementation PrefPanelSessions_ResourceViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelSessionResourceCocoa" delegate:self context:nullptr];
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
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
																kPreferences_ChangeContextName);
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring([preferenceChangeListener listenerRef],
																kPreferences_ChangeNumberOfContexts);
	[preferenceChangeListener release];
	[_sessionFavoriteIndexes release];
	[_sessionFavorites release];
	[prefsMgr release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


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
	case kPreferences_ChangeNumberOfContexts:
		// contexts were renamed, added or removed; destroy and rebuild the session menu
		{
			CFArrayRef				contextNamesCFArray = nullptr;
			Preferences_Result		prefsResult = Preferences_CreateContextNameArray
													(Quills::Prefs::SESSION, contextNamesCFArray,
														true/* start with Default */);
			
			
			[self willChangeValueForKey:@"sessionFavorites"];
			[_sessionFavorites release], _sessionFavorites = nil;
			if (kPreferences_ResultOK == prefsResult)
			{
				NSMutableArray*		mutableArray = [[NSMutableArray alloc] initWithArray:BRIDGE_CAST(contextNamesCFArray, NSArray*)];
				
				
				// the first item is a do-nothing action that is set initially
				// (CRITICAL to ensure that bindings cannot inadvertently force
				// a customized command line to always mirror some session);
				// note that "setSessionFavoriteIndexes:" depends on the index
				// values implied by the array allocated here
				[mutableArray insertObject:NSLocalizedStringFromTable(@"Select…", @"PrefPanelSessions", @"initial selection for “copy a Session” menu")
											atIndex:0];
				_sessionFavorites = mutableArray;
			}
			else
			{
				Console_Warning(Console_WriteValue, "failed to refresh session list for Resource tab, error", prefsResult);
				_sessionFavorites = [@[] retain];
			}
			[self didChangeValueForKey:@"sessionFavorites"];
		}
		break;
	
	default:
		// ???
		break;
	}
}// model:preferenceChange:context:


#pragma mark Accessors: Preferences


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_StringByJoiningArray*)
commandLine
{
	return [self->byKey objectForKey:@"commandLine"];
}// commandLine


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_CollectionBinding*)
formatFavoriteLightMode
{
	return [self->byKey objectForKey:@"formatFavoriteLightMode"];
}// formatFavoriteLightMode


/*!
Accessor.

(2019.03)
*/
- (PreferenceValue_CollectionBinding*)
formatFavoriteDarkMode
{
	return [self->byKey objectForKey:@"formatFavoriteDarkMode"];
}// formatFavoriteDarkMode


/*!
Accessor.

(2020.07)
*/
- (PreferenceValue_CollectionBinding*)
macroSetFavorite
{
	return [self->byKey objectForKey:@"macroSetFavorite"];
}// macroSetFavorite


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_CollectionBinding*)
terminalFavorite
{
	return [self->byKey objectForKey:@"terminalFavorite"];
}// terminalFavorite


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_CollectionBinding*)
translationFavorite
{
	return [self->byKey objectForKey:@"translationFavorite"];
}// translationFavorite


#pragma mark Accessors: Low-Level User Interface State


/*!
Accessor.

(4.1)
*/
- (BOOL)
isEditingRemoteShell
{
	return isEditingRemoteShell;
}// isEditingRemoteShell


/*!
Accessor.

(4.0)
*/
- (NSIndexSet*)
sessionFavoriteIndexes
{
	return [[_sessionFavoriteIndexes retain] autorelease];
}
- (void)
setSessionFavoriteIndexes:(NSIndexSet*)		indexes
{
	if (indexes != _sessionFavoriteIndexes)
	{
		NSUInteger const	selectedIndex = [indexes firstIndex];
		
		
		[self willChangeValueForKey:@"sessionFavoriteIndexes"];
		
		[_sessionFavoriteIndexes release];
		_sessionFavoriteIndexes = [indexes retain];
		
		[self didChangeValueForKey:@"sessionFavoriteIndexes"];
		
		// copy the corresponding session’s command line to the field
		switch (selectedIndex)
		{
		case 0:
			{
				// header item
				// (no action)
			}
			break;
		
		case 1:
			{
				// Default
				Preferences_ContextRef	defaultContext = nullptr;
				Preferences_Result		prefsResult = Preferences_GetDefaultContext(&defaultContext,
																					Quills::Prefs::SESSION);
				
				
				//Console_WriteLine("setting command line using Default context");
				if ((nullptr != defaultContext) && (kPreferences_ResultOK == prefsResult))
				{
					[self setCommandLineFromSession:defaultContext];
				}
				else
				{
					// failed...
					Console_Warning(Console_WriteValue, "failed to copy command line from default context, error", prefsResult);
					Sound_StandardAlert();
				}
			}
			break;
		
		default:
			{
				NSString*					collectionName = [_sessionFavorites objectAtIndex:selectedIndex];
				CFStringRef					asCFString = BRIDGE_CAST(collectionName, CFStringRef);
				Preferences_ContextWrap		sessionContext(Preferences_NewContextFromFavorites
															(Quills::Prefs::SESSION, asCFString),
																Preferences_ContextWrap::kAlreadyRetained);
				
				
				//Console_WriteValueCFString("setting command line using context", asCFString);
				if (sessionContext.exists())
				{
					[self setCommandLineFromSession:sessionContext.returnRef()];
				}
				else
				{
					// failed...
					Console_Warning(Console_WriteValueCFString, "failed to copy command line from collection name", asCFString);
					Sound_StandardAlert();
				}
			}
			break;
		}
	}
}// setSessionFavoriteIndexes:


/*!
Accessor.

(4.0)
*/
- (NSArray*)
sessionFavorites
{
	return _sessionFavorites;
}


#pragma mark Accessors: Internal Bindings


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_String*)
serverHost
{
	return [self->byKey objectForKey:@"serverHost"];
}// serverHost


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
serverPort
{
	return [self->byKey objectForKey:@"serverPort"];
}// serverPort


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
serverProtocol
{
	return [self->byKey objectForKey:@"serverProtocol"];
}// serverProtocol


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_String*)
serverUserID
{
	return [self->byKey objectForKey:@"serverUserID"];
}// serverUserID


#pragma mark Actions

/*!
Overwrites the command line with the user’s default shell location.

(4.1)
*/
- (IBAction)
performSetCommandLineToDefaultShell:(id)	sender
{
#pragma unused(sender)
	CFArrayRef		argumentCFArray = nullptr;
	Local_Result	localResult = kLocal_ResultOK;
	Boolean			isError = true;
	
	
	localResult = Local_GetDefaultShellCommandLine(argumentCFArray);
	if ((kLocal_ResultOK == localResult) && (nullptr != argumentCFArray))
	{
		NSArray*	asNSArray = BRIDGE_CAST(argumentCFArray, NSArray*);
		
		
		[self.commandLine setStringValue:[asNSArray componentsJoinedByString:@" "]];
		isError = false;
		CFRelease(argumentCFArray), argumentCFArray = nullptr;
	}
	
	if (isError)
	{
		// failed...
		Console_Warning(Console_WriteLine, "failed to set log-in shell command");
		Sound_StandardAlert();
	}
}// performSetCommandLineToDefaultShell:


/*!
Overwrites the command line with a command to start a log-in shell.

(4.1)
*/
- (IBAction)
performSetCommandLineToLogInShell:(id)	sender
{
#pragma unused(sender)
	CFArrayRef		argumentCFArray = nullptr;
	Local_Result	localResult = kLocal_ResultOK;
	Boolean			isError = true;
	
	
	localResult = Local_GetLoginShellCommandLine(argumentCFArray);
	if ((kLocal_ResultOK == localResult) && (nullptr != argumentCFArray))
	{
		NSArray*	asNSArray = BRIDGE_CAST(argumentCFArray, NSArray*);
		
		
		[self.commandLine setStringValue:[asNSArray componentsJoinedByString:@" "]];
		isError = false;
		CFRelease(argumentCFArray), argumentCFArray = nullptr;
	}
	
	if (isError)
	{
		// failed...
		Console_Warning(Console_WriteLine, "failed to set log-in shell command");
		Sound_StandardAlert();
	}
}// performSetCommandLineToLogInShell:


/*!
Overwrites the command line with whatever is derived from the
remote server browser panel.  If the panel has not been
displayed yet, it is initialized based on the remote server
preferences.

(4.1)
*/
- (IBAction)
performSetCommandLineToRemoteShell:(id)		sender
{
#pragma unused(sender)
	if (nil != _serverBrowser)
	{
		ServerBrowser_Remove(_serverBrowser);
	}
	else
	{
		NSWindow*	window = self.managedView.window;
		NSPoint		fieldLocalPoint = NSMakePoint(CGFLOAT_DIV_2(NSWidth(self->commandLineTextField.bounds)),
													CGFLOAT_DIV_2(NSHeight(self->commandLineTextField.bounds)));
		CGPoint		browserPointOrigin = NSPointToCGPoint
											([self->commandLineTextField convertPoint:fieldLocalPoint
																						toView:nil/* make window coordinates */]);
		
		
		_serverBrowser = ServerBrowser_New(window, browserPointOrigin, self/* observer */);
		
		// update the browser with the new settings (might be sharing
		// a previously-constructed interface with old data)
		ServerBrowser_Configure(_serverBrowser, STATIC_CAST([self.serverProtocol.numberValue unsignedIntegerValue], Session_Protocol),
								BRIDGE_CAST(self.serverHost.stringValue, CFStringRef),
								STATIC_CAST([self.serverPort.numberValue unsignedIntegerValue], UInt16),
								BRIDGE_CAST(self.serverUserID.stringValue, CFStringRef));
		
		// display server browser (closed by the user)
		ServerBrowser_Display(_serverBrowser);
	}
}// performSetCommandLineToRemoteShell:


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
	_serverBrowser = nil;
	_sessionFavoriteIndexes = [[NSIndexSet alloc] init];
	_sessionFavorites = [@[] retain];
	
	self->prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:4/* arbitrary; number of settings */];
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
	assert(nil != commandLineTextField);
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
	[self->byKey setObject:[[[PreferenceValue_StringByJoiningArray alloc]
								initWithPreferencesTag:kPreferences_TagCommandLine
														contextManager:self->prefsMgr
														characterSetForSplitting:[NSCharacterSet whitespaceAndNewlineCharacterSet]
														stringForJoiningElements:@" "]
							autorelease]
					forKey:@"commandLine"];
	[self->byKey setObject:[[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedFormatFavoriteLightMode
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::FORMAT
														includeDefault:YES]
							autorelease]
					forKey:@"formatFavoriteLightMode"];
	[self->byKey setObject:[[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedFormatFavoriteDarkMode
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::FORMAT
														includeDefault:YES]
							autorelease]
					forKey:@"formatFavoriteDarkMode"];
	[self->byKey setObject:[[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedMacroSetFavorite
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::MACRO_SET
														includeDefault:YES]
							autorelease]
					forKey:@"macroSetFavorite"];
	[self->byKey setObject:[[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedTerminalFavorite
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::TERMINAL
														includeDefault:YES]
							autorelease]
					forKey:@"terminalFavorite"];
	[self->byKey setObject:[[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedTranslationFavorite
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::TRANSLATION
														includeDefault:YES]
							autorelease]
					forKey:@"translationFavorite"];
	
	// these keys are not directly bound in the UI but they
	// are convenient for setting remote-server preferences
	[self->byKey setObject:[[[PreferenceValue_String alloc]
								initWithPreferencesTag:kPreferences_TagServerHost
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"serverHost"];
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagServerPort
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeSInt16]
							autorelease]
					forKey:@"serverPort"];
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagServerProtocol
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16]
							autorelease]
					forKey:@"serverProtocol"];
	[self->byKey setObject:[[[PreferenceValue_String alloc]
								initWithPreferencesTag:kPreferences_TagServerUserID
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"serverUserID"];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
	}
	
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
	
	// reset session short-cut menu
	self.sessionFavoriteIndexes = [NSIndexSet indexSetWithIndex:0];
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
	return [NSImage imageNamed:@"IconForPrefPanelSessions"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Sessions.Resource";
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
	return NSLocalizedStringFromTable(@"Resource", @"PrefPanelSessions", @"the name of this panel");
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
	return Quills::Prefs::SESSION;
}// preferencesClass


#pragma mark ServerBrowser_DataChangeObserver

/*!
Updates the command line field appropriately for the given
change in server protocol.

(4.1)
*/
- (void)
serverBrowser:(ServerBrowser_VC*)	aBrowser
didSetProtocol:(Session_Protocol)	aProtocol
{
#pragma unused(aBrowser)
	// save value in preferences (although only the command line
	// “matters”, this allows most of the command to be reconstructed
	// later when the user edits remote-server properties)
	self.serverProtocol.numberValue = [NSNumber numberWithUnsignedInteger:STATIC_CAST(aProtocol, UInt16)];
	
	// reconstruct command line accordingly
	[self updateCommandLineFromRemotePreferences];
}// serverBrowser:didSetProtocol:


/*!
Updates the command line field appropriately for the given
change in server host name.

(4.1)
*/
- (void)
serverBrowser:(ServerBrowser_VC*)	aBrowser
didSetHostName:(NSString*)			aHostName
{
#pragma unused(aBrowser)
	// save value in preferences (although only the command line
	// “matters”, this allows most of the command to be reconstructed
	// later when the user edits remote-server properties)
	self.serverHost.stringValue = aHostName;
	
	// reconstruct command line accordingly
	[self updateCommandLineFromRemotePreferences];
}// serverBrowser:didSetHostName:


/*!
Updates the command line field appropriately for the given
change in server port number.

(4.1)
*/
- (void)
serverBrowser:(ServerBrowser_VC*)	aBrowser
didSetPortNumber:(NSUInteger)		aPortNumber
{
#pragma unused(aBrowser)
	// save value in preferences (although only the command line
	// “matters”, this allows most of the command to be reconstructed
	// later when the user edits remote-server properties)
	self.serverPort.numberValue = [NSNumber numberWithUnsignedInteger:aPortNumber];
	
	// reconstruct command line accordingly
	[self updateCommandLineFromRemotePreferences];
}// serverBrowser:didSetPortNumber:


/*!
Updates the command line field appropriately for the given
change in user ID.

(4.1)
*/
- (void)
serverBrowser:(ServerBrowser_VC*)	aBrowser
didSetUserID:(NSString*)			aUserID
{
#pragma unused(aBrowser)
	// save value in preferences (although only the command line
	// “matters”, this allows most of the command to be reconstructed
	// later when the user edits remote-server properties)
	self.serverUserID.stringValue = aUserID;
	
	// reconstruct command line accordingly
	[self updateCommandLineFromRemotePreferences];
}// serverBrowser:didSetUserID:


/*!
Ensures that server-related settings are saved.

(4.1)
*/
- (void)
serverBrowserDidClose:(ServerBrowser_VC*)	aBrowser
{
#pragma unused(aBrowser)
	[self willChangeValueForKey:@"isEditingRemoteShell"];
	self->isEditingRemoteShell = NO;
	[self didChangeValueForKey:@"isEditingRemoteShell"];
	
	ServerBrowser_Dispose(&_serverBrowser);
}// serverBrowserDidClose:


@end // PrefPanelSessions_ResourceViewManager


#pragma mark -
@implementation PrefPanelSessions_ResourceViewManager (PrefPanelSessions_ResourceViewManagerInternal)


#pragma mark New Methods


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"commandLine", @"serverHost", @"serverPort", @"serverProtocol",
				@"serverUserID", @"terminalFavorite",
				@"formatFavoriteLightMode", @"formatFavoriteDarkMode",
				@"macroSetFavorite", @"translationFavorite"];
}// primaryDisplayBindingKeys


/*!
Fills in the command-line field using the given Session context.
If the Session does not have a command line argument array, the
command line is generated based on remote server information;
and if even that fails, the Default settings will be used.

(4.1)
*/
- (void)
setCommandLineFromSession:(Preferences_ContextRef)		inSession
{
	CFArrayRef				argumentListCFArray = nullptr;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_ContextGetData(inSession, kPreferences_TagCommandLine,
												sizeof(argumentListCFArray), &argumentListCFArray);
	if (kPreferences_ResultOK == prefsResult)
	{
		[self.commandLine setStringValue:[BRIDGE_CAST(argumentListCFArray, NSArray*) componentsJoinedByString:@" "]];
	}
	else
	{
		// ONLY if no actual command line was found, generate a
		// command line based on other settings (like host name)
		Session_Protocol	givenProtocol = kSession_ProtocolSSH1;
		CFStringRef			commandLineCFString = nullptr;
		CFStringRef			hostCFString = nullptr;
		CFStringRef			userCFString = nullptr;
		UInt16				portNumber = 0;
		UInt16				preferenceCountOK = 0;
		Boolean				updateOK = false;
		
		
		preferenceCountOK = readPreferencesForRemoteServers(inSession, true/* search defaults too */,
															givenProtocol, hostCFString, portNumber, userCFString);
		if (4 != preferenceCountOK)
		{
			Console_Warning(Console_WriteLine, "unable to read one or more remote server preferences!");
		}
		updateOK = copyRemoteCommandLineString(givenProtocol, hostCFString, portNumber, userCFString, commandLineCFString);
		if (false == updateOK)
		{
			Console_Warning(Console_WriteLine, "unable to update some part of command line based on given preferences!");
		}
		else
		{
			[self.commandLine setStringValue:BRIDGE_CAST(commandLineCFString, NSString*)];
		}
		
		CFRelease(hostCFString), hostCFString = nullptr;
		CFRelease(userCFString), userCFString = nullptr;
	}
}// setCommandLineFromSession


/*!
Fills in the command-line field using the current
preferences context’s remote-server preferences ONLY,
discarding any customizations the user may have done.

Currently this reads the protocol, host, port, and
user ID values.

(4.1)
*/
- (void)
updateCommandLineFromRemotePreferences
{
	CFStringRef		newCommandLine = nullptr;
	
	
	// reconstruct command line accordingly
	if (false == copyRemoteCommandLineString(STATIC_CAST([self.serverProtocol.numberValue integerValue], Session_Protocol),
												BRIDGE_CAST(self.serverHost.stringValue, CFStringRef),
												STATIC_CAST([self.serverPort.numberValue integerValue], UInt16),
												BRIDGE_CAST(self.serverUserID.stringValue, CFStringRef),
												newCommandLine))
	{
		Console_Warning(Console_WriteLine, "failed to update command line string to match change in remote server setting");
		self.commandLine.stringValue = @"";
	}
	else
	{
		self.commandLine.stringValue = BRIDGE_CAST(newCommandLine, NSString*);
		CFRelease(newCommandLine), newCommandLine = nullptr;
	}
}// updateCommandLineFromRemotePreferences


@end // PrefPanelSessions_ResourceViewManager (PrefPanelSessions_ResourceViewManagerInternal)


#pragma mark -
@implementation PrefPanelSessions_CaptureFileValue


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
		self->enabledObject = [[PreferenceValue_Flag alloc]
									initWithPreferencesTag:kPreferences_TagCaptureAutoStart
															contextManager:aContextMgr];
		self->allowSubsObject = [[PreferenceValue_Flag alloc]
									initWithPreferencesTag:kPreferences_TagCaptureFileNameAllowsSubstitutions
															contextManager:aContextMgr];
		self->fileNameObject = [[PreferenceValue_String alloc]
									initWithPreferencesTag:kPreferences_TagCaptureFileName
															contextManager:aContextMgr];
		self->directoryPathObject = [[PreferenceValue_FileSystemObject alloc]
										initWithURLInfoPreferencesTag:kPreferences_TagCaptureFileDirectoryURL
																		contextManager:aContextMgr isDirectory:YES];
		
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
	[enabledObject release];
	[allowSubsObject release];
	[fileNameObject release];
	[directoryPathObject release];
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
	[self didChangeValueForKey:@"isEnabled"];
	[self didChangeValueForKey:@"allowSubstitutions"];
	[self didChangeValueForKey:@"directoryPathURLValue"];
	[self didChangeValueForKey:@"fileNameStringValue"];
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
	[self willChangeValueForKey:@"fileNameStringValue"];
	[self willChangeValueForKey:@"directoryPathURLValue"];
	[self willChangeValueForKey:@"allowSubstitutions"];
	[self willChangeValueForKey:@"isEnabled"];
}// prefsContextWillChange:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
isEnabled
{
	return [[self->enabledObject numberValue] boolValue];
}
- (void)
setEnabled:(BOOL)	aFlag
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"isEnabled"];
	
	[self->enabledObject setNumberValue:((aFlag) ? @(YES) : @(NO))];
	
	[self didChangeValueForKey:@"isEnabled"];
	[self didSetPreferenceValue];
}// setEnabled:


/*!
Accessor.

(4.1)
*/
- (BOOL)
allowSubstitutions
{
	return [[self->allowSubsObject numberValue] boolValue];
}
- (void)
setAllowSubstitutions:(BOOL)	aFlag
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"allowSubstitutions"];
	
	[self->allowSubsObject setNumberValue:((aFlag) ? @(YES) : @(NO))];
	
	[self didChangeValueForKey:@"allowSubstitutions"];
	[self didSetPreferenceValue];
}// setAllowSubstitutions:


/*!
Accessor.

(4.1)
*/
- (NSURL*)
directoryPathURLValue
{
	return [self->directoryPathObject URLValue];
}
- (void)
setDirectoryPathURLValue:(NSURL*)	aURL
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"directoryPathURLValue"];
	
	if (nil == aURL)
	{
		[self->directoryPathObject setNilPreferenceValue];
	}
	else
	{
		[self->directoryPathObject setURLValue:aURL];
	}
	
	[self didChangeValueForKey:@"directoryPathURLValue"];
	[self didSetPreferenceValue];
}// setDirectoryPathURLValue:


/*!
Accessor.

(4.1)
*/
- (NSString*)
fileNameStringValue
{
	return [self->fileNameObject stringValue];
}
- (void)
setFileNameStringValue:(NSString*)	aString
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"fileNameStringValue"];
	
	if (nil == aString)
	{
		[self->fileNameObject setNilPreferenceValue];
	}
	else
	{
		[self->fileNameObject setStringValue:aString];
	}
	
	[self didChangeValueForKey:@"fileNameStringValue"];
	[self didSetPreferenceValue];
}// setFileNameStringValue:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = ([self->enabledObject isInherited] && [self->allowSubsObject isInherited] &&
						[self->fileNameObject isInherited] && [self->directoryPathObject isInherited]);
	
	
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
	[self willChangeValueForKey:@"directoryPathURLValue"];
	[self willChangeValueForKey:@"fileNameStringValue"];
	[self willChangeValueForKey:@"allowSubstitutions"];
	[self willChangeValueForKey:@"isEnabled"];
	[self->directoryPathObject setNilPreferenceValue];
	[self->fileNameObject setNilPreferenceValue];
	[self->allowSubsObject setNilPreferenceValue];
	[self->enabledObject setNilPreferenceValue];
	[self didChangeValueForKey:@"isEnabled"];
	[self didChangeValueForKey:@"allowSubstitutions"];
	[self didChangeValueForKey:@"fileNameStringValue"];
	[self didChangeValueForKey:@"directoryPathURLValue"];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end // PrefPanelSessions_CaptureFileValue


#pragma mark -
@implementation PrefPanelSessions_DataFlowViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelSessionDataFlowCocoa" delegate:self context:nullptr];
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
- (PrefPanelSessions_CaptureFileValue*)
captureToFile
{
	return [self->byKey objectForKey:@"captureToFile"];
}// captureToFile


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
localEcho
{
	return [self->byKey objectForKey:@"localEcho"];
}// localEcho


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
lineInsertionDelay
{
	return [self->byKey objectForKey:@"lineInsertionDelay"];
}// lineInsertionDelay


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Number*)
scrollingDelay
{
	return [self->byKey objectForKey:@"scrollingDelay"];
}// scrollingDelay


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
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:4/* arbitrary; number of settings */];
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
	[self->byKey setObject:[[[PrefPanelSessions_CaptureFileValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"captureToFile"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagLocalEchoEnabled
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"localEcho"];
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagPasteNewLineDelay
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeFloat64]
							autorelease]
					forKey:@"lineInsertionDelay"];
	// display seconds as milliseconds for this value (should be in sync with units label in NIB!)
	[[self->byKey objectForKey:@"lineInsertionDelay"] setScaleExponent:-3 rounded:YES];
	[self->byKey setObject:[[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagScrollDelay
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeFloat64]
							autorelease]
					forKey:@"scrollingDelay"];
	// display seconds as milliseconds for this value (should be in sync with units label in NIB!)
	[[self->byKey objectForKey:@"scrollingDelay"] setScaleExponent:-3 rounded:YES];
	
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
	return [NSImage imageNamed:@"IconForPrefPanelSessions"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Sessions.DataFlow";
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
	return NSLocalizedStringFromTable(@"Data Flow", @"PrefPanelSessions", @"the name of this panel");
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
	return Quills::Prefs::SESSION;
}// preferencesClass


@end // PrefPanelSessions_DataFlowViewManager


#pragma mark -
@implementation PrefPanelSessions_DataFlowViewManager (PrefPanelSessions_DataFlowViewManagerInternal)


#pragma mark New Methods


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"localEcho", @"lineInsertionDelay", @"scrollingDelay", @"captureToFile"];
}// primaryDisplayBindingKeys


@end // PrefPanelSessions_DataFlowViewManager (PrefPanelSessions_DataFlowViewManagerInternal)


#pragma mark -
@implementation PrefPanelSessions_GraphicsModeValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kVectorInterpreter_ModeDisabled
																description:NSLocalizedStringFromTable
																			(@"Disabled", @"PrefPanelSessions"/* table */,
																				@"graphics-off")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kVectorInterpreter_ModeTEK4014
																description:NSLocalizedStringFromTable
																			(@"TEK 4014 (black and white)", @"PrefPanelSessions"/* table */,
																				@"graphics-4014")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kVectorInterpreter_ModeTEK4105
																description:NSLocalizedStringFromTable
																			(@"TEK 4105 (color)", @"PrefPanelSessions"/* table */,
																				@"graphics-4105")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:kPreferences_TagTektronixMode
											contextManager:aContextMgr
											preferenceCType:kPreferenceValue_CTypeUInt16
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


@end // PrefPanelSessions_GraphicsModeValue


#pragma mark -
@implementation PrefPanelSessions_GraphicsViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelSessionGraphicsCocoa" delegate:self context:nullptr];
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
- (PrefPanelSessions_GraphicsModeValue*)
graphicsMode
{
	return [self->byKey objectForKey:@"graphicsMode"];
}// graphicsMode


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
pageClearsScreen
{
	return [self->byKey objectForKey:@"pageClearsScreen"];
}// pageClearsScreen


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
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:2/* arbitrary; number of settings */];
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
								initWithPreferencesTag:kPreferences_TagTektronixPAGEClearsScreen
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"pageClearsScreen"];
	[self->byKey setObject:[[[PrefPanelSessions_GraphicsModeValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"graphicsMode"];
	
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
	return [NSImage imageNamed:@"IconForPrefPanelSessions"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Sessions.Graphics";
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
	return NSLocalizedStringFromTable(@"Graphics", @"PrefPanelSessions", @"the name of this panel");
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
	return Quills::Prefs::SESSION;
}// preferencesClass


@end // PrefPanelSessions_GraphicsViewManager


#pragma mark -
@implementation PrefPanelSessions_GraphicsViewManager (PrefPanelSessions_GraphicsViewManagerInternal)


#pragma mark New Methods


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"pageClearsScreen", @"graphicsMode"];
}// primaryDisplayBindingKeys


@end // PrefPanelSessions_GraphicsViewManager (PrefPanelSessions_GraphicsViewManagerInternal)


#pragma mark -
@implementation PrefPanelSessions_ControlKeyValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
	}
	return self;
}// initWithPreferencesTag:contextManager:


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
Parses the given label to find the control key (in ASCII)
that it refers to.

(4.1)
*/
- (BOOL)
parseControlKey:(NSString*)		aString
controlKeyChar:(char*)			aCharPtr
{
	NSCharacterSet*		controlCharLetters = [NSCharacterSet characterSetWithCharactersInString:
												@"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"];
	NSScanner*			scanner = nil; // created below
	NSString*			controlCharString = nil;
	NSString*			targetLetterString = nil;
	BOOL				scanOK = NO;
	BOOL				result = NO;
	
	
	// first strip whitespace
	aString = [aString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
	
	scanner = [NSScanner scannerWithString:aString];
	
	// looking for a string of the form, e.g. "⌃C"; anything else is unexpected
	// (IMPORTANT: since caret "^" is a valid control character name it is VITAL
	// that the Unicode control character SYMBOL be the first part of the string
	// so that it is seen as something different!)
	scanOK = ([scanner scanUpToCharactersFromSet:controlCharLetters intoString:&controlCharString] &&
				[scanner scanCharactersFromSet:controlCharLetters intoString:&targetLetterString] &&
				[scanner isAtEnd]);
	if (scanOK)
	{
		if ([targetLetterString length] == 1)
		{
			unichar		asUnicode = ([targetLetterString characterAtIndex:0] - '@');
			
			
			if (asUnicode < 32)
			{
				// must be in ASCII control-key range
				*aCharPtr = STATIC_CAST(asUnicode, char);
			}
			else
			{
				scanOK = NO;
			}
		}
		else
		{
			scanOK = NO;
		}
	}
	
	if (scanOK)
	{
		result = YES;
	}
	else
	{
		result = NO;
	}
	
	return result;
}// parseControlKey:controlKeyChar:


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (char)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	char					result = '\0';
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = [[self prefsMgr] currentContext];
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextGetData(sourceContext, [self preferencesTag],
													sizeof(result), &result,
													true/* search defaults */, &isDefault);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to read control-key preference, error", prefsResult);
			result = '\0';
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (true == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
stringValue
{
	BOOL		isDefault = NO;
	char		asChar = [self readValueSeeIfDefault:&isDefault];
	NSString*	result = [NSString stringWithFormat:@"⌃%c", ('@' + asChar)/* convert to printable ASCII */];
	
	
	return result;
}
- (void)
setStringValue:(NSString*)	aControlKeyString
{
	[self willSetPreferenceValue];
	
	if (nil == aControlKeyString)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [[self prefsMgr] deleteDataForPreferenceTag:[self preferencesTag]];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteLine, "failed to remove control-key preference");
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = [[self prefsMgr] currentContext];
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			char	charValue = '\0';
			
			
			// NOTE: The validation method will scrub the string beforehand.
			// IMPORTANT: This is a bit weird...it essentially parses the
			// given value to infer the setting that is actually being made.
			// This was chosen over more “Cocoa-like” approaches such as an
			// NSValueTransformer because the underlying value is not
			// significantly different from a string and any transformed
			// value would at least require a wrapping object.
			if ([self parseControlKey:aControlKeyString controlKeyChar:&charValue])
			{
				Preferences_Result	prefsResult = kPreferences_ResultOK;
				
				
				prefsResult = Preferences_ContextSetData(targetContext, [self preferencesTag],
															sizeof(charValue), &charValue);
				if (kPreferences_ResultOK == prefsResult)
				{
					saveOK = YES;
				}
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteLine, "failed to save control-key preference");
		}
	}
	
	[self didSetPreferenceValue];
}// setStringValue:


#pragma mark Validators


/*!
Validates a control key from a label, returning an appropriate
error (and a NO result) if the value is incomprehensible.

(4.1)
*/
- (BOOL)
validateStringValue:(id*/* NSString* */)	ioValue
error:(NSError**)						outError
{
	BOOL	result = NO;
	
	
	if (nil == *ioValue)
	{
		result = YES;
	}
	else
	{
		char	asciiValue = '\0';
		
		
		result = [self parseControlKey:(NSString*)*ioValue controlKeyChar:&asciiValue];
		if (NO == result)
		{
			if (nullptr == outError)
			{
				// cannot return NO when the error instance is undefined
				result = YES;
			}
			else
			{
				NSString*	errorMessage = nil;
				
				
				errorMessage = NSLocalizedStringFromTable(@"This must be a control-key specification.",
															@"PrefPanelSessions"/* table */,
															@"message displayed for bad control-key values");
				
				*outError = [NSError errorWithDomain:(NSString*)kConstantsRegistry_NSErrorDomainAppDefault
								code:kConstantsRegistry_NSErrorBadNumber
								userInfo:@{ NSLocalizedDescriptionKey: errorMessage }];
			}
		}
	}
	return result;
}// validateStringValue:error:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(char)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setStringValue:nil];
}// setNilPreferenceValue


@end // PrefPanelSessions_ControlKeyValue


#pragma mark -
@implementation PrefPanelSessions_EmacsMetaValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kSession_EmacsMetaKeyOff
																description:NSLocalizedStringFromTable
																			(@"Off", @"PrefPanelSessions"/* table */,
																				@"no meta key mapping")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kSession_EmacsMetaKeyOption
																description:NSLocalizedStringFromTable
																			(@"⌥", @"PrefPanelSessions"/* table */,
																				@"hold down Option for meta")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kSession_EmacsMetaKeyShiftOption
																description:NSLocalizedStringFromTable
																			(@"⇧⌥", @"PrefPanelSessions"/* table */,
																				@"hold down Shift + Option for meta")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:kPreferences_TagEmacsMetaKey
											contextManager:aContextMgr
											preferenceCType:kPreferenceValue_CTypeUInt16
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


@end // PrefPanelSessions_EmacsMetaValue


#pragma mark -
@implementation PrefPanelSessions_NewLineValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	NSArray*	descriptorArray = [[[NSArray alloc] initWithObjects:
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kSession_NewlineModeMapLF
																description:NSLocalizedStringFromTable
																			(@"LF", @"PrefPanelSessions"/* table */,
																				@"line-feed")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kSession_NewlineModeMapCR
																description:NSLocalizedStringFromTable
																			(@"CR", @"PrefPanelSessions"/* table */,
																				@"carriage-return")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kSession_NewlineModeMapCRLF
																description:NSLocalizedStringFromTable
																			(@"CR LF", @"PrefPanelSessions"/* table */,
																				@"carriage-return, line-feed")]
										autorelease],
									[[[PreferenceValue_IntegerDescriptor alloc]
										initWithIntegerValue:kSession_NewlineModeMapCRNull
																description:NSLocalizedStringFromTable
																			(@"CR NULL", @"PrefPanelSessions"/* table */,
																				@"carriage-return, null-character")]
										autorelease],
									nil] autorelease];
	
	
	self = [super initWithPreferencesTag:kPreferences_TagNewLineMapping
											contextManager:aContextMgr
											preferenceCType:kPreferenceValue_CTypeUInt16
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


@end // PrefPanelSessions_NewLineValue


#pragma mark -
@implementation PrefPanelSessions_KeyboardViewManager


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelSessionKeyboardCocoa" delegate:self context:nullptr];
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
deleteKeySendsBackspace
{
	return [self->byKey objectForKey:@"deleteKeySendsBackspace"];
}// deleteKeySendsBackspace


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
emacsArrowKeys
{
	return [self->byKey objectForKey:@"emacsArrowKeys"];
}// emacsArrowKeys


/*!
Accessor.

(4.1)
*/
- (BOOL)
isEditingKeyInterruptProcess
{
	return isEditingKeyInterruptProcess;
}// isEditingKeyInterruptProcess


/*!
Accessor.

(4.1)
*/
- (BOOL)
isEditingKeyResume
{
	return isEditingKeyResume;
}// isEditingKeyResume


/*!
Accessor.

(4.1)
*/
- (BOOL)
isEditingKeySuspend
{
	return isEditingKeySuspend;
}// isEditingKeySuspend


/*!
Accessor.

(4.1)
*/
- (PrefPanelSessions_ControlKeyValue*)
keyInterruptProcess
{
	return [self->byKey objectForKey:@"keyInterruptProcess"];
}// keyInterruptProcess


/*!
Accessor.

(4.1)
*/
- (PrefPanelSessions_ControlKeyValue*)
keyResume
{
	return [self->byKey objectForKey:@"keyResume"];
}// keyResume


/*!
Accessor.

(4.1)
*/
- (PrefPanelSessions_ControlKeyValue*)
keySuspend
{
	return [self->byKey objectForKey:@"keySuspend"];
}// keySuspend


/*!
Accessor.

(4.1)
*/
- (PrefPanelSessions_EmacsMetaValue*)
mappingForEmacsMeta
{
	return [self->byKey objectForKey:@"mappingForEmacsMeta"];
}// mappingForEmacsMeta


/*!
Accessor.

(4.1)
*/
- (PrefPanelSessions_NewLineValue*)
mappingForNewLine
{
	return [self->byKey objectForKey:@"mappingForNewLine"];
}// mappingForNewLine


#pragma mark Actions


/*!
Displays or removes the Control Keys palette.  If the
button is not selected, it is highlighted and any click
in the Control Keys palette will update the preference
value.  If the button is selected, it is reset to a
normal state and control key clicks stop affecting the
preference value.

(4.1)
*/
- (IBAction)
performChooseInterruptProcessKey:(id)	sender
{
#pragma unused(sender)
	[self setEditedKeyType:kMy_KeyTypeInterrupt];
	Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self);
}// performChooseInterruptProcessKey:


/*!
Displays or removes the Control Keys palette.  If the
button is not selected, it is highlighted and any click
in the Control Keys palette will update the preference
value.  If the button is selected, it is reset to a
normal state and control key clicks stop affecting the
preference value.

(4.1)
*/
- (IBAction)
performChooseResumeKey:(id)		sender
{
#pragma unused(sender)
	[self setEditedKeyType:kMy_KeyTypeResume];
	Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self);
}// performChooseResumeKey:


/*!
Displays or removes the Control Keys palette.  If the
button is not selected, it is highlighted and any click
in the Control Keys palette will update the preference
value.  If the button is selected, it is reset to a
normal state and control key clicks stop affecting the
preference value.

(4.1)
*/
- (IBAction)
performChooseSuspendKey:(id)	sender
{
#pragma unused(sender)
	[self setEditedKeyType:kMy_KeyTypeSuspend];
	Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self);
}// performChooseSuspendKey:


#pragma mark Keypads_SetResponder() Interface


/*!
Received when the Control Keys keypad is used to select
a control key while this class instance is the current
keypad responder (as set by Keypads_SetResponder()).

This handles the event by updating the currently-selected
key mapping preference, if any is in effect.

(4.1)
*/
- (void)
controlKeypadSentCharacterCode:(NSNumber*)	asciiChar
{
	// convert to printable ASCII letter
	char const		kPrintableASCII = ('@' + [asciiChar charValue]);
	
	
	// NOTE: This odd-looking approach is useful because the
	// Cocoa bindings can already convert control-character
	// strings (from button titles) into preference settings.
	// Setting a control-character-like string value will
	// actually cause a new preference to be saved.
	switch ([self editedKeyType])
	{
	case kMy_KeyTypeInterrupt:
		[[self keyInterruptProcess] setStringValue:[NSString stringWithFormat:@"⌃%c", kPrintableASCII]];
		break;
	
	case kMy_KeyTypeResume:
		[[self keyResume] setStringValue:[NSString stringWithFormat:@"⌃%c", kPrintableASCII]];
		break;
	
	case kMy_KeyTypeSuspend:
		[[self keySuspend] setStringValue:[NSString stringWithFormat:@"⌃%c", kPrintableASCII]];
		break;
	
	default:
		// no effect
		break;
	}
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
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:7/* arbitrary; number of settings */];
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
								initWithPreferencesTag:kPreferences_TagMapDeleteToBackspace
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"deleteKeySendsBackspace"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagMapArrowsForEmacs
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"emacsArrowKeys"];
	[self->byKey setObject:[[[PrefPanelSessions_ControlKeyValue alloc]
								initWithPreferencesTag:kPreferences_TagKeyInterruptProcess
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"keyInterruptProcess"];
	[self->byKey setObject:[[[PrefPanelSessions_ControlKeyValue alloc]
								initWithPreferencesTag:kPreferences_TagKeyResumeOutput
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"keyResume"];
	[self->byKey setObject:[[[PrefPanelSessions_ControlKeyValue alloc]
								initWithPreferencesTag:kPreferences_TagKeySuspendOutput
														contextManager:self->prefsMgr]
							autorelease]
					forKey:@"keySuspend"];
	[self->byKey setObject:[[[PrefPanelSessions_EmacsMetaValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"mappingForEmacsMeta"];
	[self->byKey setObject:[[[PrefPanelSessions_NewLineValue alloc]
								initWithContextManager:self->prefsMgr]
							autorelease]
					forKey:@"mappingForNewLine"];
	
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
#pragma unused(aViewManager)
	if (kPanel_VisibilityHidden == aVisibility)
	{
		[self resetGlobalState];
	}
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
	[self resetGlobalState];
	
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
	[self resetGlobalState];
	
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
	return [NSImage imageNamed:@"IconForPrefPanelSessions"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Sessions.Keyboard";
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
	return NSLocalizedStringFromTable(@"Keyboard", @"PrefPanelSessions", @"the name of this panel");
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
	return Quills::Prefs::SESSION;
}// preferencesClass


@end // PrefPanelSessions_KeyboardViewManager


#pragma mark -
@implementation PrefPanelSessions_KeyboardViewManager (PrefPanelSessions_KeyboardViewManagerInternal)


#pragma mark New Methods


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"deleteKeySendsBackspace", @"emacsArrowKeys", @"keyInterruptProcess",
				@"keyResume", @"keySuspend", @"mappingForEmacsMeta", @"mappingForNewLine"];
}// primaryDisplayBindingKeys


/*!
Resets anything that is shared across all panels.  This
action should be performed whenever the panel is not in
use, e.g. when it is hidden or dismissed by the user.

(4.1)
*/
- (void)
resetGlobalState
{
	[self setEditedKeyType:kMy_KeyTypeNone];
}// resetGlobalState


#pragma mark Accessors


/*!
Flags the specified key type (if any) as “currently being
edited”.  Through bindings, this will cause zero or more
buttons to enter a highlighted or normal state.

This also changes GLOBAL state.  See "resetGlobalState".

(4.1)
*/
- (My_KeyType)
editedKeyType
{
	return gEditedKeyType;
}
- (void)
setEditedKeyType:(My_KeyType)	aType
{
	[self willChangeValueForKey:@"isEditingKeyInterruptProcess"];
	[self willChangeValueForKey:@"isEditingKeyResume"];
	[self willChangeValueForKey:@"isEditingKeySuspend"];
	gEditedKeyType = aType;
	self->isEditingKeyInterruptProcess = (aType == kMy_KeyTypeInterrupt);
	self->isEditingKeyResume = (aType == kMy_KeyTypeResume);
	self->isEditingKeySuspend = (aType == kMy_KeyTypeSuspend);
	if (aType == kMy_KeyTypeNone)
	{
		Keypads_RemoveResponder(kKeypads_WindowTypeControlKeys, self);
	}
	[self didChangeValueForKey:@"isEditingKeySuspend"];
	[self didChangeValueForKey:@"isEditingKeyResume"];
	[self didChangeValueForKey:@"isEditingKeyInterruptProcess"];
}// setEditedKeyType:


@end // PrefPanelSessions_KeyboardViewManager (PrefPanelSessions_KeyboardViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
