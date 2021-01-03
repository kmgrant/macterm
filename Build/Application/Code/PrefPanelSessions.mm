/*!	\file PrefPanelSessions.mm
	\brief Implements the Sessions panel of Preferences.
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

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>



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
Implements SwiftUI interaction for the “Data Flow” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelSessions_DataFlowActionHandler : NSObject< UIPrefsSessionDataFlow_ActionHandling > //{
{
@private
	PrefsContextManager_Object*		_prefsMgr;
	UIPrefsSessionDataFlow_Model*	_viewModel;
}

// new methods
	- (NSInteger)
	resetToDefaultGetDelayValueWithTag:(Preferences_Tag)_;
	- (BOOL)
	resetToDefaultGetFlagWithTag:(Preferences_Tag)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsSessionDataFlow_Model*
	viewModel;

@end //}


/*!
Implements SwiftUI interaction for the “Keyboard” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelSessions_KeyboardActionHandler : NSObject< Keypads_ControlKeyResponder,
																UIPrefsSessionKeyboard_ActionHandling > //{
{
@private
	PrefsContextManager_Object*		_prefsMgr;
	UIPrefsSessionKeyboard_Model*	_viewModel;
}

// new methods
	- (BOOL)
	resetToDefaultGetFlagWithTag:(Preferences_Tag)_;
	- (char)
	returnKeyCharPreferenceForUIEnum:(UIKeypads_KeyID)_;
	- (BOOL)
	setKeyID:(UIKeypads_KeyID*)_
	fromKeyCharPreference:(char)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsSessionKeyboard_Model*
	viewModel;

@end //}


/*!
Implements SwiftUI interaction for the “vector graphics” panel.

This is technically only a separate internal class because the main
view controller must be visible in the header but a Swift-defined
protocol for the view controller must be implemented somewhere.
Swift imports are not safe to do from header files but they can be
done from this implementation file, and used by this internal class.
*/
@interface PrefPanelSessions_GraphicsActionHandler : NSObject< UIPrefsSessionGraphics_ActionHandling > //{
{
@private
	PrefsContextManager_Object*		_prefsMgr;
	UIPrefsSessionGraphics_Model*	_viewModel;
}

// new methods
	- (BOOL)
	resetToDefaultGetFlagWithTag:(Preferences_Tag)_;
	- (void)
	updateViewModelFromPrefsMgr;

// accessors
	@property (strong) PrefsContextManager_Object*
	prefsMgr;
	@property (strong) UIPrefsSessionGraphics_Model*
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
PrefPanelSessions_NewDataFlowPaneTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads screen-pane preferences from the context of a data set
	tagList.push_back(kPreferences_TagLocalEchoEnabled);
	tagList.push_back(kPreferences_TagPasteNewLineDelay);
	tagList.push_back(kPreferences_TagScrollDelay);
	tagList.push_back(kPreferences_TagCaptureAutoStart);
	tagList.push_back(kPreferences_TagCaptureFileDirectoryURL);
	tagList.push_back(kPreferences_TagCaptureFileName);
	tagList.push_back(kPreferences_TagCaptureFileNameAllowsSubstitutions);
	
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
										[[PrefPanelSessions_ResourceViewManager alloc] init],
										[[PrefPanelSessions_DataFlowVC alloc] init],
										[[PrefPanelSessions_KeyboardVC alloc] init],
										[[PrefPanelSessions_GraphicsVC alloc] init],
									];
	NSString*	panelName = NSLocalizedStringFromTable(@"Sessions", @"PrefPanelSessions",
														@"the name of this panel");
	NSImage*	panelIcon = nil;
	
	
	if (@available(macOS 11.0, *))
	{
		panelIcon = [NSImage imageWithSystemSymbolName:@"heart" accessibilityDescription:self.panelName];
	}
	else
	{
		panelIcon = [NSImage imageNamed:@"IconForPrefPanelSessions"];
	}
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Sessions"
										localizedName:panelName
										localizedIcon:panelIcon
										viewManagerArray:subViewManagers];
	if (nil != self)
	{
	}
	return self;
}// init


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
			_sessionFavorites = nil;
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
				_sessionFavorites = @[];
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
	return _sessionFavoriteIndexes;
}
- (void)
setSessionFavoriteIndexes:(NSIndexSet*)		indexes
{
	if (indexes != _sessionFavoriteIndexes)
	{
		NSUInteger const	selectedIndex = [indexes firstIndex];
		
		
		[self willChangeValueForKey:@"sessionFavoriteIndexes"];
		
		_sessionFavoriteIndexes = indexes;
		
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
initializeWithContext:(NSObject*)		aContext
{
#pragma unused(aViewManager, aContext)
	_serverBrowser = nil;
	_sessionFavoriteIndexes = [[NSIndexSet alloc] init];
	_sessionFavorites = @[];
	
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
	[self->byKey setObject:[[PreferenceValue_StringByJoiningArray alloc]
								initWithPreferencesTag:kPreferences_TagCommandLine
														contextManager:self->prefsMgr
														characterSetForSplitting:[NSCharacterSet whitespaceAndNewlineCharacterSet]
														stringForJoiningElements:@" "]
					forKey:@"commandLine"];
	[self->byKey setObject:[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedFormatFavoriteLightMode
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::FORMAT
														includeDefault:YES]
					forKey:@"formatFavoriteLightMode"];
	[self->byKey setObject:[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedFormatFavoriteDarkMode
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::FORMAT
														includeDefault:YES]
					forKey:@"formatFavoriteDarkMode"];
	[self->byKey setObject:[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedMacroSetFavorite
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::MACRO_SET
														includeDefault:YES]
					forKey:@"macroSetFavorite"];
	[self->byKey setObject:[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedTerminalFavorite
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::TERMINAL
														includeDefault:YES]
					forKey:@"terminalFavorite"];
	[self->byKey setObject:[[PreferenceValue_CollectionBinding alloc]
								initWithPreferencesTag:kPreferences_TagAssociatedTranslationFavorite
														contextManager:self->prefsMgr
														sourceClass:Quills::Prefs::TRANSLATION
														includeDefault:YES]
					forKey:@"translationFavorite"];
	
	// these keys are not directly bound in the UI but they
	// are convenient for setting remote-server preferences
	[self->byKey setObject:[[PreferenceValue_String alloc]
								initWithPreferencesTag:kPreferences_TagServerHost
														contextManager:self->prefsMgr]
					forKey:@"serverHost"];
	[self->byKey setObject:[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagServerPort
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeSInt16]
					forKey:@"serverPort"];
	[self->byKey setObject:[[PreferenceValue_Number alloc]
								initWithPreferencesTag:kPreferences_TagServerProtocol
														contextManager:self->prefsMgr
														preferenceCType:kPreferenceValue_CTypeUInt16]
					forKey:@"serverProtocol"];
	[self->byKey setObject:[[PreferenceValue_String alloc]
								initWithPreferencesTag:kPreferences_TagServerUserID
														contextManager:self->prefsMgr]
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
	if (@available(macOS 11.0, *))
	{
		return [NSImage imageWithSystemSymbolName:@"bookmark" accessibilityDescription:self.panelName];
	}
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
@implementation PrefPanelSessions_DataFlowActionHandler //{


/*!
Designated initializer.

(2020.12)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		_prefsMgr = nil; // see "panelViewManager:initializeWithContext:"
		_viewModel = [[UIPrefsSessionDataFlow_Model alloc] initWithRunner:self]; // transfer ownership
	}
	return self;
}// init


#pragma mark New Methods


/*!
Deletes any local override for the given delay value
and returns the Default value (in milliseconds, for
UI display),.

(2020.12)
*/
- (NSInteger)
resetToDefaultGetDelayValueWithTag:(Preferences_Tag)	aTag
{
	NSInteger	result = 0;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:aTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", aTag);
	}
	
	// return default value
	{
		Preferences_TimeInterval	preferenceValue = false; // in seconds
		Boolean						isDefault = false;
		Preferences_Result			prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, aTag,
																				sizeof(preferenceValue), &preferenceValue,
																				true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", aTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			// note: UI value is in milliseconds but stored value is not
			preferenceValue = roundf(preferenceValue / kPreferences_TimeIntervalMillisecond);
			result = STATIC_CAST(preferenceValue, NSInteger);
		}
	}
	
	return result;
}// resetToDefaultGetDelayValueWithTag:


/*!
Helper function for protocol methods; deletes the
given preference tag and returns the Default value.

(2020.12)
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

(2020.12)
*/
- (void)
updateViewModelFromPrefsMgr
{
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	Boolean					isDefault = false; // reused below
	Boolean					isDefaultAutoCapture = true; // initial value; see below
	
	
	// allow initialization of "isDefault..." values without triggers
	self.viewModel.defaultOverrideInProgress = YES;
	self.viewModel.disableWriteback = YES;
	
	// update settings
	{
		Preferences_Tag		preferenceTag = kPreferences_TagLocalEchoEnabled;
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
			self.viewModel.isLocalEchoEnabled = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultLocalEchoEnabled = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag				preferenceTag = kPreferences_TagPasteNewLineDelay;
		Preferences_TimeInterval	preferenceValue = 0;
		Preferences_Result			prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																				sizeof(preferenceValue), &preferenceValue,
																				true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			// note: UI value is in milliseconds but stored value is not
			preferenceValue = roundf(preferenceValue / kPreferences_TimeIntervalMillisecond);
			self.viewModel.lineInsertionDelayValue = STATIC_CAST(preferenceValue, NSInteger); // SwiftUI binding
			self.viewModel.isDefaultLineInsertionDelay = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag				preferenceTag = kPreferences_TagScrollDelay;
		Preferences_TimeInterval	preferenceValue = 0;
		Preferences_Result			prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																				sizeof(preferenceValue), &preferenceValue,
																				true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			// note: UI value is in milliseconds but stored value is not
			preferenceValue = roundf(preferenceValue / kPreferences_TimeIntervalMillisecond);
			self.viewModel.scrollingDelayValue = STATIC_CAST(preferenceValue, NSInteger); // SwiftUI binding
			self.viewModel.isDefaultScrollingDelay = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCaptureAutoStart;
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
			self.viewModel.isAutoCaptureToDirectoryEnabled = preferenceValue; // SwiftUI binding
			isDefaultAutoCapture = (isDefaultAutoCapture && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag			preferenceTag = kPreferences_TagCaptureFileDirectoryURL;
		Preferences_URLInfo		preferenceValue = { nullptr, false/* is stale */ };
		Preferences_Result		prefsResult = Preferences_ContextGetData(sourceContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue,
																			true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to get local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			self.viewModel.captureFileDirectory = BRIDGE_CAST(preferenceValue.urlRef, NSURL*); // SwiftUI binding
			isDefaultAutoCapture = (isDefaultAutoCapture && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCaptureFileName;
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
			self.viewModel.captureFileName = BRIDGE_CAST(preferenceValue, NSString*); // SwiftUI binding
			isDefaultAutoCapture = (isDefaultAutoCapture && isDefault); // see below; used to update binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCaptureFileNameAllowsSubstitutions;
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
			self.viewModel.isCaptureFileNameGenerated = preferenceValue; // SwiftUI binding
			isDefaultAutoCapture = (isDefaultAutoCapture && isDefault); // see below; used to update binding
		}
	}
	self.viewModel.isDefaultAutoCaptureToDirectoryEnabled = isDefaultAutoCapture; // SwiftUI binding
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::SESSION);
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsSessionDataFlow_ActionHandling


/*!
Called by the UI when the user has made a change.

Currently this is called for any change to any setting so the
only way to respond is to copy all model data to the preferences
context.  If performance or other issues arise, it is possible
to expand the protocol to have (say) per-setting callbacks but
for now this is simpler and sufficient.

See also "updateViewModelFromPrefsMgr", which should be roughly
the inverse of this.

(2020.12)
*/
- (void)
dataUpdated
{
	Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
	
	
	// update settings
	{
		Preferences_Tag		preferenceTag = kPreferences_TagLocalEchoEnabled;
		Boolean				preferenceValue = self.viewModel.isLocalEchoEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag				preferenceTag = kPreferences_TagPasteNewLineDelay;
		// note: UI value is in milliseconds but stored value is not
		Preferences_TimeInterval	preferenceValue = (STATIC_CAST(self.viewModel.lineInsertionDelayValue, Preferences_TimeInterval) * kPreferences_TimeIntervalMillisecond);
		Preferences_Result			prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																				sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag				preferenceTag = kPreferences_TagScrollDelay;
		// note: UI value is in milliseconds but stored value is not
		Preferences_TimeInterval	preferenceValue = (STATIC_CAST(self.viewModel.scrollingDelayValue, Preferences_TimeInterval) * kPreferences_TimeIntervalMillisecond);
		Preferences_Result			prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																				sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCaptureAutoStart;
		Boolean				preferenceValue = self.viewModel.isAutoCaptureToDirectoryEnabled;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag			preferenceTag = kPreferences_TagCaptureFileDirectoryURL;
		Preferences_URLInfo		preferenceValue = { BRIDGE_CAST(self.viewModel.captureFileDirectory, CFURLRef), false/* is stale */ };
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCaptureFileName;
		CFStringRef			preferenceValue = BRIDGE_CAST(self.viewModel.captureFileName, CFStringRef);
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagCaptureFileNameAllowsSubstitutions;
		Boolean				preferenceValue = self.viewModel.isCaptureFileNameGenerated;
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

(2020.12)
*/
- (BOOL)
resetToDefaultGetLocalEchoEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagLocalEchoEnabled];
}// resetToDefaultGetLocalEchoEnabled


/*!
Deletes any local override for the given value and
returns the Default value.

(2020.12)
*/
- (NSInteger)
resetToDefaultGetLineInsertionDelay
{
	return [self resetToDefaultGetDelayValueWithTag:kPreferences_TagPasteNewLineDelay];
}// resetToDefaultGetLineInsertionDelay


/*!
Deletes any local override for the given value and
returns the Default value.

(2020.12)
*/
- (NSInteger)
resetToDefaultGetScrollingDelay
{
	return [self resetToDefaultGetDelayValueWithTag:kPreferences_TagScrollDelay];
}// resetToDefaultGetScrollingDelay


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.12)
*/
- (BOOL)
resetToDefaultGetAutoCaptureToDirectoryEnabled
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagCaptureAutoStart];
}// resetToDefaultGetAutoCaptureToDirectoryEnabled


/*!
Deletes any local override for the given URL and
returns the Default value.

(2020.12)
*/
- (NSURL*)
resetToDefaultGetCaptureFileDirectory
{
	NSURL*				result = [NSURL fileURLWithPath:@"/var/tmp"];
	Preferences_Tag		preferenceTag = kPreferences_TagCaptureFileDirectoryURL;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		Preferences_URLInfo		preferenceValue = { nullptr, false/* is stale */ };
		Boolean					isDefault = false;
		Preferences_Result		prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue,
																			true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = BRIDGE_CAST(preferenceValue.urlRef, NSURL*);
		}
	}
	
	return result;
}// resetToDefaultGetCaptureFileDirectory


/*!
Deletes any local override for the given file name
and returns the Default value.

(2020.12)
*/
- (NSString*)
resetToDefaultGetCaptureFileName
{
	NSString*			result = @"";
	Preferences_Tag		preferenceTag = kPreferences_TagCaptureFileName;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		CFStringRef				preferenceValue = CFSTR("");
		Boolean					isDefault = false;
		Preferences_Result		prefsResult = Preferences_ContextGetData(self.prefsMgr.currentContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue,
																			true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to read default preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
		else
		{
			result = BRIDGE_CAST(preferenceValue, NSString*);
		}
	}
	
	return result;
}// resetToDefaultGetCaptureFileName


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.12)
*/
- (BOOL)
resetToDefaultGetCaptureFileNameIsGenerated
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagCaptureFileNameAllowsSubstitutions];
}// resetToDefaultGetCaptureFileNameIsGenerated


@end //} PrefPanelSessions_DataFlowActionHandler


#pragma mark -
@implementation PrefPanelSessions_DataFlowVC //{


/*!
Designated initializer.

(2020.12)
*/
- (instancetype)
init
{
	PrefPanelSessions_DataFlowActionHandler*	actionHandler = [[PrefPanelSessions_DataFlowActionHandler alloc] init];
	NSView*										newView = [UIPrefsSessionDataFlow_ObjC makeView:actionHandler.viewModel];
	
	
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

(2020.12)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext/* PrefPanelSessions_DataFlowActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelSessions_DataFlowActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelSessions_DataFlowActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 520, 330); // somewhat arbitrary; see SwiftUI code/playground
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self.logicalFirstResponder = self.view;
	self.logicalLastResponder = self.view;
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(2020.12)
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

(2020.12)
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

(2020.12)
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

(2020.12)
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

(2020.12)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(2020.12)
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

(2020.12)
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

(2020.12)
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

(2020.12)
*/
- (NSImage*)
panelIcon
{
	if (@available(macOS 11.0, *))
	{
		return [NSImage imageWithSystemSymbolName:@"arrow.left.arrow.right" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelSessions"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.12)
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

(2020.12)
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

(2020.12)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintHorizontal;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(2020.12)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::SESSION;
}// preferencesClass


@end //} PrefPanelSessions_KeyboardVC


#pragma mark -
@implementation PrefPanelSessions_KeyboardActionHandler //{


/*!
Designated initializer.

(2020.12)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		_prefsMgr = nil; // see "panelViewManager:initializeWithContext:"
		_viewModel = [[UIPrefsSessionKeyboard_Model alloc] initWithRunner:self]; // transfer ownership
	}
	return self;
}// init


#pragma mark New Methods


/*!
Helper function for protocol methods; deletes the
given preference tag and returns the Default value.

(2020.12)
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
Translate from UI-specified key constant to the equivalent
ASCII character constant stored in Preferences.

TEMPORARY; this is primarily needed because it is tricky to
expose certain Objective-C types in Swift.  If those can be
consolidated, this mapping can go away.

See also "setKeyID:fromKeyCharPreference:".

(2020.12)
*/
- (char)
returnKeyCharPreferenceForUIEnum:(UIKeypads_KeyID)	aUIEnum
{
	char	result = '\0';
	
	
	switch (aUIEnum)
	{
	case UIKeypads_KeyIDControlNull:
		result = 0x00;
		break;
	
	case UIKeypads_KeyIDControlA:
		result = 0x01;
		break;
	
	case UIKeypads_KeyIDControlB:
		result = 0x02;
		break;
	
	case UIKeypads_KeyIDControlC:
		result = 0x03;
		break;
	
	case UIKeypads_KeyIDControlD:
		result = 0x04;
		break;
	
	case UIKeypads_KeyIDControlE:
		result = 0x05;
		break;
	
	case UIKeypads_KeyIDControlF:
		result = 0x06;
		break;
	
	case UIKeypads_KeyIDControlG:
		result = 0x07;
		break;
	
	case UIKeypads_KeyIDControlH:
		result = 0x08;
		break;
	
	case UIKeypads_KeyIDControlI:
		result = 0x09;
		break;
	
	case UIKeypads_KeyIDControlJ:
		result = 0x0A;
		break;
	
	case UIKeypads_KeyIDControlK:
		result = 0x0B;
		break;
	
	case UIKeypads_KeyIDControlL:
		result = 0x0C;
		break;
	
	case UIKeypads_KeyIDControlM:
		result = 0x0D;
		break;
	
	case UIKeypads_KeyIDControlN:
		result = 0x0E;
		break;
	
	case UIKeypads_KeyIDControlO:
		result = 0x0F;
		break;
	
	case UIKeypads_KeyIDControlP:
		result = 0x10;
		break;
	
	case UIKeypads_KeyIDControlQ:
		result = 0x11;
		break;
	
	case UIKeypads_KeyIDControlR:
		result = 0x12;
		break;
	
	case UIKeypads_KeyIDControlS:
		result = 0x13;
		break;
	
	case UIKeypads_KeyIDControlT:
		result = 0x14;
		break;
	
	case UIKeypads_KeyIDControlU:
		result = 0x15;
		break;
	
	case UIKeypads_KeyIDControlV:
		result = 0x16;
		break;
	
	case UIKeypads_KeyIDControlW:
		result = 0x17;
		break;
	
	case UIKeypads_KeyIDControlX:
		result = 0x18;
		break;
	
	case UIKeypads_KeyIDControlY:
		result = 0x19;
		break;
	
	case UIKeypads_KeyIDControlZ:
		result = 0x1A;
		break;
	
	case UIKeypads_KeyIDControlLeftSquareBracket:
		result = 0x1B;
		break;
	
	case UIKeypads_KeyIDControlBackslash:
		result = 0x1C;
		break;
	
	case UIKeypads_KeyIDControlRightSquareBracket:
		result = 0x1D;
		break;
	
	case UIKeypads_KeyIDControlCaret:
		result = 0x1E;
		break;
	
	case UIKeypads_KeyIDControlUnderscore:
		result = 0x1F;
		break;
	
	default:
		// only control keys are expected; other key codes not recognized
		// ???
		break;
	}
	
	return result;
}// returnKeyCharPreferenceForUIEnum:


/*!
Translates an ASCII code into a UI binding for a control key;
only invisible control characters are supported (i.e. in
the range 0x00-0x1F inclusive).

Returns YES if the character is supported and translated
(written to "outKeyID"); NO if the character is invalid.

(2020.12)
*/
- (BOOL)
setKeyID:(UIKeypads_KeyID*)		outKeyID
fromKeyCharPreference:(char)	aControlKeyChar
{
	BOOL	result = YES; // initially...
	
	
	if (nullptr == outKeyID)
	{
		result = NO;
	}
	else
	{
		// note: could move this mapping to keypad code or perhaps find
		// some better implementation...
		switch (aControlKeyChar)
		{
		case 0x00:
			*outKeyID = UIKeypads_KeyIDControlNull;
			break;
		
		case 0x01:
			*outKeyID = UIKeypads_KeyIDControlA;
			break;
		
		case 0x02:
			*outKeyID = UIKeypads_KeyIDControlB;
			break;
		
		case 0x03:
			*outKeyID = UIKeypads_KeyIDControlC;
			break;
		
		case 0x04:
			*outKeyID = UIKeypads_KeyIDControlD;
			break;
		
		case 0x05:
			*outKeyID = UIKeypads_KeyIDControlE;
			break;
		
		case 0x06:
			*outKeyID = UIKeypads_KeyIDControlF;
			break;
		
		case 0x07:
			*outKeyID = UIKeypads_KeyIDControlG;
			break;
		
		case 0x08:
			*outKeyID = UIKeypads_KeyIDControlH;
			break;
		
		case 0x09:
			*outKeyID = UIKeypads_KeyIDControlI;
			break;
		
		case 0x0A:
			*outKeyID = UIKeypads_KeyIDControlJ;
			break;
		
		case 0x0B:
			*outKeyID = UIKeypads_KeyIDControlK;
			break;
		
		case 0x0C:
			*outKeyID = UIKeypads_KeyIDControlL;
			break;
		
		case 0x0D:
			*outKeyID = UIKeypads_KeyIDControlM;
			break;
		
		case 0x0E:
			*outKeyID = UIKeypads_KeyIDControlN;
			break;
		
		case 0x0F:
			*outKeyID = UIKeypads_KeyIDControlO;
			break;
		
		case 0x10:
			*outKeyID = UIKeypads_KeyIDControlP;
			break;
		
		case 0x11:
			*outKeyID = UIKeypads_KeyIDControlQ;
			break;
		
		case 0x12:
			*outKeyID = UIKeypads_KeyIDControlR;
			break;
		
		case 0x13:
			*outKeyID = UIKeypads_KeyIDControlS;
			break;
		
		case 0x14:
			*outKeyID = UIKeypads_KeyIDControlT;
			break;
		
		case 0x15:
			*outKeyID = UIKeypads_KeyIDControlU;
			break;
		
		case 0x16:
			*outKeyID = UIKeypads_KeyIDControlV;
			break;
		
		case 0x17:
			*outKeyID = UIKeypads_KeyIDControlW;
			break;
		
		case 0x18:
			*outKeyID = UIKeypads_KeyIDControlX;
			break;
		
		case 0x19:
			*outKeyID = UIKeypads_KeyIDControlY;
			break;
		
		case 0x1A:
			*outKeyID = UIKeypads_KeyIDControlZ;
			break;
		
		case 0x1B:
			*outKeyID = UIKeypads_KeyIDControlLeftSquareBracket;
			break;
		
		case 0x1C:
			*outKeyID = UIKeypads_KeyIDControlBackslash;
			break;
		
		case 0x1D:
			*outKeyID = UIKeypads_KeyIDControlRightSquareBracket;
			break;
		
		case 0x1E:
			*outKeyID = UIKeypads_KeyIDControlCaret;
			break;
		
		case 0x1F:
			*outKeyID = UIKeypads_KeyIDControlUnderscore;
			break;
		
		default:
			// ???
			result = NO;
			break;
		}
	}
	
	return result;
}// setKeyID:fromKeyCharPreference:


/*!
Updates the view model’s observed properties based on
current preferences context data.

This is only needed when changing contexts.

See also "dataUpdated", which should be roughly the
inverse of this.

(2020.12)
*/
- (void)
updateViewModelFromPrefsMgr
{
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	Boolean					isDefault = false; // reused below
	
	
	// allow initialization of "isDefault..." values without triggers
	self.viewModel.defaultOverrideInProgress = YES;
	self.viewModel.disableWriteback = YES;
	
	// update settings
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKeyInterruptProcess;
		char				preferenceValue = 0;
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
			UIKeypads_KeyID		keyBinding = UIKeypads_KeyIDControlNull;
			
			
			// convert stored value into a UI value
			if (NO == [self setKeyID:&keyBinding fromKeyCharPreference:preferenceValue])
			{
				// ???
				Console_Warning(Console_WriteValueFourChars, "failed to translate default preference to key code for tag", preferenceTag);
			}
			else
			{
				self.viewModel.interruptKeyMapping = keyBinding;
				self.viewModel.isDefaultInterruptKeyMapping = isDefault;
			}
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKeySuspendOutput;
		char				preferenceValue = 0;
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
			UIKeypads_KeyID		keyBinding = UIKeypads_KeyIDControlNull;
			
			
			// convert stored value into a UI value
			if (NO == [self setKeyID:&keyBinding fromKeyCharPreference:preferenceValue])
			{
				// ???
				Console_Warning(Console_WriteValueFourChars, "failed to translate default preference to key code for tag", preferenceTag);
			}
			else
			{
				self.viewModel.suspendKeyMapping = keyBinding;
				self.viewModel.isDefaultSuspendKeyMapping = isDefault;
			}
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagKeyResumeOutput;
		char				preferenceValue = 0;
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
			UIKeypads_KeyID		keyBinding = UIKeypads_KeyIDControlNull;
			
			
			// convert stored value into a UI value
			if (NO == [self setKeyID:&keyBinding fromKeyCharPreference:preferenceValue])
			{
				// ???
				Console_Warning(Console_WriteValueFourChars, "failed to translate default preference to key code for tag", preferenceTag);
			}
			else
			{
				self.viewModel.resumeKeyMapping = keyBinding;
				self.viewModel.isDefaultResumeKeyMapping = isDefault;
			}
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapArrowsForEmacs;
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
			self.viewModel.arrowKeysMapToEmacs = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultEmacsCursorArrows = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagEmacsMetaKey;
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
			self.viewModel.selectedMetaMapping = STATIC_CAST(preferenceValue, Session_EmacsMetaKey); // SwiftUI binding
			self.viewModel.isDefaultEmacsMetaMapping = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapDeleteToBackspace;
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
			self.viewModel.deleteSendsBackspace = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultDeleteMapping = isDefault; // SwiftUI binding
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagNewLineMapping;
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
			self.viewModel.selectedNewlineMapping = STATIC_CAST(preferenceValue, Session_NewlineMode); // SwiftUI binding
			self.viewModel.isDefaultNewlineMapping = isDefault; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::SESSION);
}// updateViewModelFromPrefsMgr


#pragma mark Keypads_ControlKeyResponder


/*!
Received when the Control Keys keypad is closed while
this class instance is the current keypad responder (as
set by Keypads_SetResponder()).

This handles the event by updating the UI and state.

(2020.12)
*/
- (void)
controlKeypadHidden
{
	// remove annotations from buttons that indicate binding to “Control Keys”
	self.viewModel.isKeypadBindingToInterruptKey = NO;
	self.viewModel.isKeypadBindingToSuspendKey = NO;
	self.viewModel.isKeypadBindingToResumeKey = NO;
}// controlKeypadHidden


/*!
Received when the Control Keys keypad is used to select
a control key while this class instance is the current
keypad responder (as set by Keypads_SetResponder()).

This handles the event by updating the currently-selected
key mapping preference, if any is in effect.

(2020.12)
*/
- (void)
controlKeypadSentCharacterCode:(NSNumber*)	asciiChar
{
	// convert to printable ASCII letter
	UIKeypads_KeyID		keyBinding = UIKeypads_KeyIDControlS; // see below
	
	
	if (NO == [self setKeyID:&keyBinding fromKeyCharPreference:[asciiChar charValue]])
	{
		// ???
		Sound_StandardAlert();
		Console_Warning(Console_WriteValue, "session keyboard UI: “Control Keys” sent back a key click with unsupported character code", [asciiChar charValue]);
	}
	else
	{
		// due to bindings in SwiftUI, the following assignments will cause button updates
		if (self.viewModel.isKeypadBindingToInterruptKey)
		{
			self.viewModel.interruptKeyMapping = keyBinding;
		}
		else if (self.viewModel.isKeypadBindingToSuspendKey)
		{
			self.viewModel.suspendKeyMapping = keyBinding;
		}
		else if (self.viewModel.isKeypadBindingToResumeKey)
		{
			self.viewModel.resumeKeyMapping = keyBinding;
		}
		else
		{
			// ???
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "session keyboard UI: “Control Keys” sent back a key click but nothing is currently bound");
		}
		
		// save new preferences
		[self dataUpdated]; // "viewModel.runner.dataUpdated()" in SwiftUI
	}
}// controlKeypadSentCharacterCode:


#pragma mark UIPrefsSessionKeyboard_ActionHandling


/*!
Called by the UI when the user has made a change.

Currently this is called for any change to any setting so the
only way to respond is to copy all model data to the preferences
context.  If performance or other issues arise, it is possible
to expand the protocol to have (say) per-setting callbacks but
for now this is simpler and sufficient.

See also "updateViewModelFromPrefsMgr", which should be roughly
the inverse of this.

(2020.12)
*/
- (void)
dataUpdated
{
	Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
	
	
	// update settings
	{
		Preferences_Tag			preferenceTag = kPreferences_TagKeyInterruptProcess;
		char					preferenceValue = [self returnKeyCharPreferenceForUIEnum:self.viewModel.interruptKeyMapping];
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag			preferenceTag = kPreferences_TagKeySuspendOutput;
		char					preferenceValue = [self returnKeyCharPreferenceForUIEnum:self.viewModel.suspendKeyMapping];
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag			preferenceTag = kPreferences_TagKeyResumeOutput;
		char					preferenceValue = [self returnKeyCharPreferenceForUIEnum:self.viewModel.resumeKeyMapping];
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapArrowsForEmacs;
		Boolean				preferenceValue = self.viewModel.arrowKeysMapToEmacs;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag			preferenceTag = kPreferences_TagEmacsMetaKey;
		Session_EmacsMetaKey	enumPrefValue = self.viewModel.selectedMetaMapping;
		UInt16					preferenceValue = STATIC_CAST(enumPrefValue, UInt16);
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag		preferenceTag = kPreferences_TagMapDeleteToBackspace;
		Boolean				preferenceValue = self.viewModel.deleteSendsBackspace;
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																		sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	{
		Preferences_Tag			preferenceTag = kPreferences_TagNewLineMapping;
		Session_NewlineMode		enumPrefValue = self.viewModel.selectedNewlineMapping;
		UInt16					preferenceValue = STATIC_CAST(enumPrefValue, UInt16);
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
}// dataUpdated


/*!
Displays the “Control Keys” palette, bound to the given
setting, allowing the user to change the key mapping.

See also "controlKeypadSentCharacterCode:".

(2020.12)
*/
- (void)
selectNewInterruptKeyWithViewModel:(UIPrefsSessionKeyboard_Model*)	viewModel
{
	viewModel.isKeypadBindingToInterruptKey = YES;
	Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self); // see "controlKeypadSentCharacterCode:"
	Keypads_SetVisible(kKeypads_WindowTypeControlKeys, true);
}// selectNewInterruptKeyWithViewModel:


/*!
Displays the “Control Keys” palette, bound to the given
setting, allowing the user to change the key mapping.

See also "controlKeypadSentCharacterCode:".

(2020.12)
*/
- (void)
selectNewSuspendKeyWithViewModel:(UIPrefsSessionKeyboard_Model*)	viewModel
{
	viewModel.isKeypadBindingToSuspendKey = YES;
	Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self); // see "controlKeypadSentCharacterCode:"
	Keypads_SetVisible(kKeypads_WindowTypeControlKeys, true);
}// selectNewSuspendKeyWithViewModel:


/*!
Displays the “Control Keys” palette, bound to the given
setting, allowing the user to change the key mapping.

See also "controlKeypadSentCharacterCode:".

(2020.12)
*/
- (void)
selectNewResumeKeyWithViewModel:(UIPrefsSessionKeyboard_Model*)	viewModel
{
	viewModel.isKeypadBindingToResumeKey = YES;
	Keypads_SetResponder(kKeypads_WindowTypeControlKeys, self); // see "controlKeypadSentCharacterCode:"
	Keypads_SetVisible(kKeypads_WindowTypeControlKeys, true);
}// selectNewResumeKeyWithViewModel:


/*!
Deletes any local override for the given key mapping
and returns the Default value.

(2020.12)
*/
- (UIKeypads_KeyID)
resetToDefaultGetInterruptKeyMapping
{
	UIKeypads_KeyID		result = UIKeypads_KeyIDControlNull;
	Preferences_Tag		preferenceTag = kPreferences_TagKeyInterruptProcess;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		char				preferenceValue = 0;
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
			// convert stored value into a UI value
			if (NO == [self setKeyID:&result fromKeyCharPreference:preferenceValue])
			{
				// ???
				Console_Warning(Console_WriteValueFourChars, "failed to translate default preference to key code for tag", preferenceTag);
				result = UIKeypads_KeyIDControlNull;
			}
		}
	}
	
	return result;
}// resetToDefaultGetInterruptKeyMapping


/*!
Deletes any local override for the given key mapping
and returns the Default value.

(2020.12)
*/
- (UIKeypads_KeyID)
resetToDefaultGetSuspendKeyMapping
{
	UIKeypads_KeyID		result = UIKeypads_KeyIDControlNull;
	Preferences_Tag		preferenceTag = kPreferences_TagKeySuspendOutput;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		char				preferenceValue = 0;
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
			// convert stored value into a UI value
			if (NO == [self setKeyID:&result fromKeyCharPreference:preferenceValue])
			{
				// ???
				Console_Warning(Console_WriteValueFourChars, "failed to translate default preference to key code for tag", preferenceTag);
				result = UIKeypads_KeyIDControlNull;
			}
		}
	}
	
	return result;
}// resetToDefaultGetSuspendKeyMapping


/*!
Deletes any local override for the given key mapping
and returns the Default value.

(2020.12)
*/
- (UIKeypads_KeyID)
resetToDefaultGetResumeKeyMapping
{
	UIKeypads_KeyID		result = UIKeypads_KeyIDControlNull;
	Preferences_Tag		preferenceTag = kPreferences_TagKeyResumeOutput;
	
	
	// delete local preference
	if (NO == [self.prefsMgr deleteDataForPreferenceTag:preferenceTag])
	{
		Console_Warning(Console_WriteValueFourChars, "failed to delete preference with tag", preferenceTag);
	}
	
	// return default value
	{
		char				preferenceValue = 0;
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
			// convert stored value into a UI value
			if (NO == [self setKeyID:&result fromKeyCharPreference:preferenceValue])
			{
				// ???
				Console_Warning(Console_WriteValueFourChars, "failed to translate default preference to key code for tag", preferenceTag);
				result = UIKeypads_KeyIDControlNull;
			}
		}
	}
	
	return result;
}// resetToDefaultGetResumeKeyMapping


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.12)
*/
- (BOOL)
resetToDefaultGetArrowKeysMapToEmacs
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagMapArrowsForEmacs];
}// resetToDefaultGetArrowKeysMapToEmacs


/*!
Deletes any local override for the given setting and
returns the Default value.

(2020.12)
*/
- (Session_EmacsMetaKey)
resetToDefaultGetSelectedMetaMapping
{
	Session_EmacsMetaKey	result = kSession_EmacsMetaKeyOff;
	Preferences_Tag			preferenceTag = kPreferences_TagEmacsMetaKey;
	
	
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
			result = STATIC_CAST(preferenceValue, Session_EmacsMetaKey);
		}
	}
	
	return result;
}// resetToDefaultGetSelectedMetaMapping


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.12)
*/
- (BOOL)
resetToDefaultGetDeleteSendsBackspace
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagMapDeleteToBackspace];
}// resetToDefaultGetDeleteSendsBackspace


/*!
Deletes any local override for the given setting and
returns the Default value.

(2020.12)
*/
- (Session_NewlineMode)
resetToDefaultGetSelectedNewlineMapping
{
	Session_NewlineMode		result = kSession_NewlineModeMapLF;
	Preferences_Tag			preferenceTag = kPreferences_TagNewLineMapping;
	
	
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
			result = STATIC_CAST(preferenceValue, Session_NewlineMode);
		}
	}
	
	return result;
}// resetToDefaultGetSelectedNewlineMapping


@end //} PrefPanelSessions_KeyboardActionHandler


#pragma mark -
@implementation PrefPanelSessions_KeyboardVC //{


/*!
Designated initializer.

(2020.12)
*/
- (instancetype)
init
{
	PrefPanelSessions_KeyboardActionHandler*	actionHandler = [[PrefPanelSessions_KeyboardActionHandler alloc] init];
	NSView*										newView = [UIPrefsSessionKeyboard_ObjC makeView:actionHandler.viewModel];
	
	
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

(2020.12)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext/* PrefPanelSessions_KeyboardActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelSessions_KeyboardActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelSessions_KeyboardActionHandler*);
	
	
	actionHandler.prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	
	_actionHandler = actionHandler; // transfer ownership
	_idealFrame = CGRectMake(0, 0, 520, 330); // somewhat arbitrary; see SwiftUI code/playground
	
	// TEMPORARY; not clear how to extract views from SwiftUI-constructed hierarchy;
	// for now, assign to itself so it is not "nil"
	self.logicalFirstResponder = self.view;
	self.logicalLastResponder = self.view;
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(2020.12)
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

(2020.12)
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

(2020.12)
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

(2020.12)
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

(2020.12)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager)
	if (kPanel_VisibilityHidden == aVisibility)
	{
		// unbind and possibly hide the “Control Keys” palette
		Keypads_RemoveResponder(kKeypads_WindowTypeControlKeys, self);
		
		// remove annotations from buttons that indicate binding to “Control Keys”
		self.actionHandler.viewModel.isKeypadBindingToInterruptKey = NO;
		self.actionHandler.viewModel.isKeypadBindingToSuspendKey = NO;
		self.actionHandler.viewModel.isKeypadBindingToResumeKey = NO;
	}
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(2020.12)
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

(2020.12)
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

(2020.12)
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

(2020.12)
*/
- (NSImage*)
panelIcon
{
	if (@available(macOS 11.0, *))
	{
		return [NSImage imageWithSystemSymbolName:@"keyboard" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelSessions"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.12)
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

(2020.12)
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

(2020.12)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintHorizontal;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(2020.12)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::SESSION;
}// preferencesClass


@end //} PrefPanelSessions_KeyboardVC


#pragma mark -
@implementation PrefPanelSessions_GraphicsActionHandler //{


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
		_viewModel = [[UIPrefsSessionGraphics_Model alloc] initWithRunner:self]; // transfer ownership
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
	
	// update TEK mode
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTektronixMode;
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
			self.viewModel.selectedTEKMode = STATIC_CAST(preferenceValue, VectorInterpreter_Mode); // SwiftUI binding
			self.viewModel.isDefaultTEKMode = isDefault; // SwiftUI binding
		}
	}
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTektronixPAGEClearsScreen;
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
			self.viewModel.pageCommandClearsScreen = preferenceValue; // SwiftUI binding
			self.viewModel.isDefaultPageClears = isDefault; // SwiftUI binding
		}
	}
	
	// restore triggers
	self.viewModel.disableWriteback = NO;
	self.viewModel.defaultOverrideInProgress = NO;
	
	// finally, specify “is editing Default” to prevent user requests for
	// “restore to Default” from deleting the Default settings themselves!
	self.viewModel.isEditingDefaultContext = Preferences_ContextIsDefault(sourceContext, Quills::Prefs::SESSION);
}// updateViewModelFromPrefsMgr


#pragma mark UIPrefsSessionGraphics_ActionHandling


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
	
	
	// update TEK mode
	{
		Preferences_Tag			preferenceTag = kPreferences_TagTektronixMode;
		VectorInterpreter_Mode	enumPrefValue = self.viewModel.selectedTEKMode;
		UInt16					preferenceValue = STATIC_CAST(enumPrefValue, UInt16);
		Preferences_Result		prefsResult = Preferences_ContextSetData(targetContext, preferenceTag,
																			sizeof(preferenceValue), &preferenceValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to update local copy of preference for tag", preferenceTag);
			Console_Warning(Console_WriteValue, "preference result", prefsResult);
		}
	}
	
	// update flags
	{
		Preferences_Tag		preferenceTag = kPreferences_TagTektronixPAGEClearsScreen;
		Boolean				preferenceValue = self.viewModel.pageCommandClearsScreen;
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
- (VectorInterpreter_Mode)
resetToDefaultGetSelectedTEKMode
{
	VectorInterpreter_Mode		result = kVectorInterpreter_ModeDisabled;
	Preferences_Tag				preferenceTag = kPreferences_TagTerminalEmulatorType;
	
	
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
			result = STATIC_CAST(preferenceValue, VectorInterpreter_Mode);
		}
	}
	
	return result;
}// resetToDefaultGetSelectedTEKMode


/*!
Deletes any local override for the given flag and
returns the Default value.

(2020.11)
*/
- (BOOL)
resetToDefaultGetPageCommandClearsScreen
{
	return [self resetToDefaultGetFlagWithTag:kPreferences_TagTektronixPAGEClearsScreen];
}// resetToDefaultGetPageCommandClearsScreen


@end //} PrefPanelSessions_GraphicsActionHandler


#pragma mark -
@implementation PrefPanelSessions_GraphicsVC //{


/*!
Designated initializer.

(2020.11)
*/
- (instancetype)
init
{
	PrefPanelSessions_GraphicsActionHandler*	actionHandler = [[PrefPanelSessions_GraphicsActionHandler alloc] init];
	NSView*										newView = [UIPrefsSessionGraphics_ObjC makeView:actionHandler.viewModel];
	
	
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
initializeWithContext:(NSObject*)		aContext/* PrefPanelSessions_GraphicsActionHandler*; see "init" */
{
#pragma unused(aViewManager)
	assert(nil != aContext);
	PrefPanelSessions_GraphicsActionHandler*	actionHandler = STATIC_CAST(aContext, PrefPanelSessions_GraphicsActionHandler*);
	
	
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
		return [NSImage imageWithSystemSymbolName:@"scribble.variable" accessibilityDescription:self.panelName];
	}
	return [NSImage imageNamed:@"IconForPrefPanelSessions"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(2020.11)
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

(2020.11)
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
	return Quills::Prefs::SESSION;
}// preferencesClass


@end //} PrefPanelSessions_GraphicsVC

// BELOW IS REQUIRED NEWLINE TO END FILE
