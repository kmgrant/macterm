/*!	\file MainEntryPoint.cp
	\brief Front end to the Preferences Converter application.
*/
/*###############################################################

	MacTerm Preferences Converter
		© 2004-2013 by Kevin Grant.
	
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

// standard-C++ includes
#include <iostream>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFDictionaryManager.h>
#include <CFKeyValueInterface.h>
#include <CocoaUserDefaults.h>
#include <UniversalDefines.h>

// MacTerm Preferences Converter includes
#include "PreferencesData.h"



#pragma mark Constants
namespace {

// the legacy MacTelnet bundle ID
CFStringRef const	kMacTelnetApplicationID = CFSTR("com.mactelnet.MacTelnet");

// the new bundle ID for MacTerm
CFStringRef const	kMacTermApplicationID = CFSTR("net.macterm.MacTerm");

enum
{
	kASRequiredSuite = kCoreEventClass
};

typedef SInt16 My_MacTelnetFolder;
enum
{
	// folders defined by MacTelnet
	kMy_MacTelnetFolderPreferences = 4,			// the “MacTelnet Preferences” folder in the user’s Preferences folder
	
	// folders defined by the Mac OS
	kMy_MacTelnetFolderMacPreferences = -1,		// the Preferences folder for the user currently logged in
};

/*!
File or Folder Names String Table ("FileOrFolderNames.strings")

The titles of special files or folders on disk; for example, used to
find preferences or error logs.
*/
enum My_FolderStringType
{
	kUIStrings_FolderNameApplicationFavorites			= 'AFav',
	kUIStrings_FolderNameApplicationFavoritesMacros		= 'AFFM',
	kUIStrings_FolderNameApplicationFavoritesProxies	= 'AFPx',
	kUIStrings_FolderNameApplicationFavoritesSessions	= 'AFSn',
	kUIStrings_FolderNameApplicationFavoritesTerminals	= 'AFTm',
	kUIStrings_FolderNameApplicationPreferences			= 'APrf',
	kUIStrings_FolderNameApplicationScriptsMenuItems	= 'AScM',
	kUIStrings_FolderNameApplicationStartupItems		= 'AStI'
};

enum My_PrefsResult
{
	kMy_PrefsResultOK				= 0,	//!< no error
	kMy_PrefsResultGenericFailure	= 1		//!< some error
};

enum My_StringResult
{
	kMy_StringResultOK				= 0,	//!< no error
	kMy_StringResultNoSuchString	= 1,	//!< tag is invalid for given string category
	kMy_StringResultCannotGetString	= 2		//!< probably an OS error, the string cannot be retrieved
};

} // anonymous namespace

#pragma mark Variables
namespace {

AEAddressDesc		gSelfAddress;				//!< used to target send-to-self Apple Events
ProcessSerialNumber	gSelfProcessID;				//!< used to target send-to-self Apple Events
SInt16				gOldPrefsFileRefNum = 0;	//!< when the resource fork of the old preferences file is
												//!  open, this is its file reference number
Boolean				gFinished = false;			//!< used to control exit warnings to the user

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

My_PrefsResult		actionVersion3				();
My_PrefsResult		actionVersion4				();
My_PrefsResult		actionVersion5				();
My_PrefsResult		actionVersion6				();
OSStatus			addErrorToReply				(OSStatus, AppleEventPtr);
Boolean				convertRGBColorToCFArray	(RGBColor const*, CFArrayRef&);
My_StringResult		copyFileOrFolderCFString	(My_FolderStringType, CFStringRef*);
Boolean				installRequiredHandlers		();
OSErr				receiveApplicationOpen		(AppleEvent const*, AppleEvent*, SInt32);
OSErr				receiveApplicationPrefs		(AppleEvent const*, AppleEvent*, SInt32);
OSErr				receiveApplicationReopen	(AppleEvent const*, AppleEvent*, SInt32);
OSErr				receiveApplicationQuit		(AppleEvent const*, AppleEvent*, SInt32);
OSErr				receiveOpenDocuments		(AppleEvent const*, AppleEvent*, SInt32);
OSErr				receivePrintDocuments		(AppleEvent const*, AppleEvent*, SInt32);
Boolean				setMacTelnetCoordPreference	(CFStringRef, SInt16, SInt16);
void				setMacTelnetPreference		(CFStringRef, CFPropertyListRef);
void				setMacTermPreference		(CFStringRef, CFPropertyListRef);

} // anonymous namespace



#pragma mark Public Methods

int
main	(int	argc,
		 char*	argv[])
{
	int		result = 0;
	
	
	// install event handlers that can receive callbacks
	// from the OS within the (blocked) main event loop;
	// this is where all the conversion “work” is done
	{
		Boolean		installedHandlersOK = false;
		
		
		installedHandlersOK = installRequiredHandlers();
		assert(installedHandlersOK);
	}
	
	// configure GUI
	{
		IBNibRef	interfaceBuilderObject = nullptr;
		OSStatus	error = noErr;
		
		
		result = 1; // assume failure unless found otherwise
		error = CreateNibReference(CFSTR("PrefsConverter"), &interfaceBuilderObject);
		if (noErr == error)
		{
			error = SetMenuBarFromNib(interfaceBuilderObject, CFSTR("MenuBar"));
			if (noErr == error)
			{
				result = 0; // success!
			}
			DisposeNibReference(interfaceBuilderObject), interfaceBuilderObject = nullptr;
		}
	}
	
	// display GUI and wait for events (including Quit!)
	if (0 == result)
	{
		RunApplicationEventLoop();
	}
	
	// quit, returning appropriate status to the shell
	return result;
}


#pragma mark Internal Methods
namespace {

/*!
Upgrades from version 2 to version 3.  Some keys are
now obsolete that were never available to users, so
they are deleted.

IMPORTANT:	Even though the program is now MacTerm,
			this legacy code MUST NOT change; the
			update steps are handled sequentially,
			pulling data from older domains and
			versions into the modern domain (even
			if intermediate steps continue to put
			data into a legacy domain, the final
			step imports everything into the modern
			domain).

(3.1)
*/
My_PrefsResult
actionVersion3 ()
{
	My_PrefsResult		result = kMy_PrefsResultOK;
	
	
	// this is now "favorite-macro-sets" and redefined as a simple array of domain names
	CFPreferencesSetAppValue(CFSTR("favorite-macros"), nullptr/* delete value */, kMacTelnetApplicationID);
	
	// this is now "favorite-formats" and redefined as a simple array of domain names
	CFPreferencesSetAppValue(CFSTR("favorite-styles"), nullptr/* delete value */, kMacTelnetApplicationID);
	
	return result;
}// actionVersion3


/*!
Upgrades from version 3 to version 4.  Some keys are
now obsolete, so they are deleted.

IMPORTANT:	Even though the program is now MacTerm,
			this legacy code MUST NOT change; the
			update steps are handled sequentially,
			pulling data from older domains and
			versions into the modern domain (even
			if intermediate steps continue to put
			data into a legacy domain, the final
			step imports everything into the modern
			domain).

(4.0)
*/
My_PrefsResult
actionVersion4 ()
{
	My_PrefsResult		result = kMy_PrefsResultOK;
	
	
	// this setting is no longer used
	CFPreferencesSetAppValue(CFSTR("menu-command-set-simplified"), nullptr/* delete value */, kMacTelnetApplicationID);
	
	// this setting is no longer used
	CFPreferencesSetAppValue(CFSTR("menu-key-equivalents"), nullptr/* delete value */, kMacTelnetApplicationID);
	
	// these settings are no longer used
	CFPreferencesSetAppValue(CFSTR("macro-menu-name-string"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("macro-menu-visible"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("menu-macros-visible"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("menu-visible"), nullptr/* delete value */, kMacTelnetApplicationID);
	
	// these settings are no longer used
	CFPreferencesSetAppValue(CFSTR("window-macroeditor-position-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-macroeditor-size-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-macroeditor-visible"), nullptr/* delete value */, kMacTelnetApplicationID);
	
	return result;
}// actionVersion4


/*!
Upgrades from version 4 to version 5.  Some keys are
now obsolete, so they are deleted.

IMPORTANT:	Even though the program is now MacTerm,
			this legacy code MUST NOT change; the
			update steps are handled sequentially,
			pulling data from older domains and
			versions into the modern domain (even
			if intermediate steps continue to put
			data into a legacy domain, the final
			step imports everything into the modern
			domain).

(4.0)
*/
My_PrefsResult
actionVersion5 ()
{
	My_PrefsResult		result = kMy_PrefsResultOK;
	
	
	// these settings are no longer used
	CFPreferencesSetAppValue(CFSTR("window-commandline-position-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-commandline-size-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-controlkeys-position-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-functionkeys-position-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-preferences-position-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-sessioninfo-column-order"), nullptr/* delete value */, kMacTelnetApplicationID);
	CFPreferencesSetAppValue(CFSTR("window-vt220keys-position-pixels"), nullptr/* delete value */, kMacTelnetApplicationID);
	
	return result;
}// actionVersion5


/*!
Upgrades from version 5 to version 6.  All preferences are
imported into the MacTerm preferences domain.  This is not
completely trivial because there are embedded references
to the old domain, e.g. in lists of Favorites.  So, the
existing preferences’ key-value pairs are scanned to find
the names of all legacy domains, and those domains are
converted.  For example, "com.mactelnet.MacTelnet.formats.1"
might be a user collection, and "net.macterm.MacTerm.formats.1"
would be its converted name.

(4.0)
*/
My_PrefsResult
actionVersion6 ()
{
	My_PrefsResult		result = kMy_PrefsResultOK;
	Boolean				syncOK = false;
	
	
	// copy everything from the base set into the new domain
	CocoaUserDefaults_CopyDomain(kMacTelnetApplicationID, kMacTermApplicationID);
	
	// look for references to the old domain and update those settings accordingly
	{
		CFStringRef		keyLists[] =
						{
							CFSTR("favorite-formats"),
							CFSTR("favorite-macro-sets"),
							CFSTR("favorite-sessions"),
							CFSTR("favorite-terminals"),
							CFSTR("favorite-translations"),
							CFSTR("favorite-workspaces")
						};
		
		
		for (size_t i = 0; i < sizeof(keyLists) / sizeof(CFStringRef); ++i)
		{
			CFRetainRelease		valueList(CFPreferencesCopyAppValue(keyLists[i], kMacTelnetApplicationID),
											true/* is retained */);
			
			
			if (valueList.exists())
			{
				CFArrayRef const	kAsArray = valueList.returnCFArrayRef();
				CFIndex const		kArraySize = CFArrayGetCount(kAsArray);
				
				
				if (kArraySize > 0)
				{
					CFRetainRelease		newArray(CFArrayCreateMutable(kCFAllocatorDefault, 0/* capacity */, &kCFTypeArrayCallBacks),
													true/* is retained */);
					
					
					for (CFIndex j = 0; j < kArraySize; ++j)
					{
						CFStringRef		oldDomainCFString = CFUtilities_StringCast(CFArrayGetValueAtIndex(kAsArray, j));
						
						
						if (nullptr != oldDomainCFString)
						{
							CFRetainRelease		newDomainCFString(CFStringCreateMutableCopy(kCFAllocatorDefault, 0/* maximum length */,
																							oldDomainCFString),
																	true/* is retained */);
							
							
							(CFIndex)CFStringFindAndReplace(newDomainCFString.returnCFMutableStringRef(), kMacTelnetApplicationID,
															kMacTermApplicationID, CFRangeMake(0, CFStringGetLength(newDomainCFString.returnCFStringRef())),
															kCFCompareCaseInsensitive);
							
							// store only the new domain, and copy all settings from the
							// old collection domain to the new collection domain; at
							// that point, the old settings can be removed
							CFArrayAppendValue(newArray.returnCFMutableArrayRef(), newDomainCFString.returnCFStringRef());
							CocoaUserDefaults_CopyDomain(oldDomainCFString, newDomainCFString.returnCFStringRef());
							syncOK = CFPreferencesAppSynchronize(newDomainCFString.returnCFStringRef());
							if (false == syncOK)
							{
								std::cerr << "MacTerm Preferences Converter: actionVersion6(): synchronization of a preference collection failed!" << std::endl;
							}
							else
							{
								// new settings were successfully saved; delete the old ones
								CocoaUserDefaults_DeleteDomain(oldDomainCFString);
								syncOK = CFPreferencesAppSynchronize(oldDomainCFString);
								if (false == syncOK)
								{
									std::cerr << "MacTerm Preferences Converter: actionVersion6(): failed to delete legacy MacTelnet preferences collection" << std::endl;
								}
							}
						}
					}
					CFPreferencesSetAppValue(keyLists[i], newArray.returnCFArrayRef(), kMacTermApplicationID);
					assert(kArraySize == CFArrayGetCount(newArray.returnCFArrayRef()));
				}
			}
		}
	}
	
	// final step: delete all the old settings, but first MAKE SURE that
	// the new settings are readable in the new domain
	syncOK = CFPreferencesAppSynchronize(kMacTermApplicationID);
	if (false == syncOK)
	{
		std::cerr << "MacTerm Preferences Converter: actionVersion6(): synchronization of new MacTerm preferences failed!" << std::endl;
	}
	else
	{
		// new settings were successfully saved; delete the old ones
		CocoaUserDefaults_DeleteDomain(kMacTelnetApplicationID);
	}
	syncOK = CFPreferencesAppSynchronize(kMacTelnetApplicationID);
	if (false == syncOK)
	{
		std::cerr << "MacTerm Preferences Converter: actionVersion6(): failed to delete legacy MacTelnet preferences" << std::endl;
	}
	
	return result;
}// actionVersion6


/*!
Adds an error code to the reply record.

You should use this to return error codes from
ALL Apple Events; if you do, return "noErr" from
the handler but potentially define "inError" as
a specific error code.

(3.1)
*/
OSStatus
addErrorToReply		(OSStatus			inError,
					 AppleEventPtr		inoutReplyAppleEventPtr)
{
	OSStatus	result = noErr;
	
	
	// the reply must exist
	if ((typeNull != inoutReplyAppleEventPtr->descriptorType) ||
		(nullptr != inoutReplyAppleEventPtr->dataHandle))
	{
		// put error code
		if (noErr != inError)
		{
			result = AEPutParamPtr(inoutReplyAppleEventPtr, keyErrorNumber, typeSInt16,
									&inError, sizeof(inError));
		}
	}
	else
	{
		// no place to put the data - do nothing
		result = errAEReplyNotValid;
	}
	
	return result;
}// addErrorToReply


/*!
Creates a Core Foundation Array with 3 values representing
the fraction (real number between 0 and 1) of the overall
red, green and blue color component intensities of the
specified color.  You must release the array and the 3
Core Foundation values in it.

Returns "true" only if successful.

(3.1)
*/
Boolean
convertRGBColorToCFArray	(RGBColor const*	inColorPtr,
							 CFArrayRef&		outArray)
{
	Boolean		result = false;
	
	
	if (nullptr != inColorPtr)
	{
		CFNumberRef		componentValues[] = { nullptr, nullptr, nullptr };
		Float32			floatValue = 0L;
		
		
		floatValue = inColorPtr->red;
		floatValue /= RGBCOLOR_INTENSITY_MAX;
		componentValues[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
		if (nullptr != componentValues[0])
		{
			floatValue = inColorPtr->green;
			floatValue /= RGBCOLOR_INTENSITY_MAX;
			componentValues[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
			if (nullptr != componentValues[1])
			{
				floatValue = inColorPtr->blue;
				floatValue /= RGBCOLOR_INTENSITY_MAX;
				componentValues[2] = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &floatValue);
				if (nullptr != componentValues[2])
				{
					// store colors in array
					outArray = CFArrayCreate(kCFAllocatorDefault,
												REINTERPRET_CAST(componentValues, void const**),
												sizeof(componentValues) / sizeof(CFNumberRef),
												&kCFTypeArrayCallBacks);
					if (nullptr != outArray)
					{
						// success!
						result = true;
					}
					CFRelease(componentValues[2]);
				}
				CFRelease(componentValues[1]);
			}
			CFRelease(componentValues[0]);
		}
	}
	else
	{
		// unexpected input
	}
	
	return result;
}// convertRGBColorToCFArray


/*!
Locates the specified string used for a file or folder
name, and returns a reference to a Core Foundation
string.  Since a copy is made, you must call CFRelease()
on the returned string when you are finished with it.

\retval kMy_StringResultOK
if the string is copied successfully

\retval kMy_StringResultNoSuchString
if the given tag is invalid

\retval kMy_StringResultCannotGetString
if an OS error occurred

(3.1)
*/
My_StringResult
copyFileOrFolderCFString	(My_FolderStringType	inWhichString,
							 CFStringRef*			outStringPtr)
{
	My_StringResult		result = kMy_StringResultOK;
	
	
	assert(nullptr != outStringPtr);
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_FolderNameApplicationFavorites:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("Favorites"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationFavorites; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationFavoritesMacros:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("Macro Sets"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationFavoritesMacros; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationFavoritesProxies:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("Proxies"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationFavoritesProxies; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationFavoritesSessions:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("Sessions"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationFavoritesSessions; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationFavoritesTerminals:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("Terminals"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationFavoritesTerminals; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationPreferences:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("MacTelnet Preferences"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationPreferences; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationScriptsMenuItems:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("Scripts Menu Items"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationScriptsMenuItems; warning, this should be consistent with previous strings of this type because it is used to find existing scripts"));
		break;
	
	case kUIStrings_FolderNameApplicationStartupItems:
		*outStringPtr = CFCopyLocalizedStringFromTable(CFSTR("Startup Items"), CFSTR("FolderNames"),
														CFSTR("kUIStrings_FolderNameApplicationStartupItems; warning, this should be consistent with previous strings of this type because it is used to find existing startup items"));
		break;
	
	default:
		// ???
		result = kMy_StringResultNoSuchString;
		break;
	}
	return result;
}// copyFileOrFolderCFString


/*!
This routine installs Apple Event handlers for
the Required Suite, including the six standard
Apple Events (open and print documents, open,
re-open and quit application, and display
preferences).

If ALL handlers install successfully, "true" is
returned; otherwise, "false" is returned.

(3.1)
*/
Boolean
installRequiredHandlers ()
{
	Boolean			result = false;
	OSStatus		error = noErr;
	AEEventClass	eventClass = '----';
	
	
	eventClass = kASRequiredSuite;
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEOpenApplication,
														NewAEEventHandlerUPP(receiveApplicationOpen),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEReopenApplication,
														NewAEEventHandlerUPP(receiveApplicationReopen),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEOpenDocuments,
														NewAEEventHandlerUPP(receiveOpenDocuments),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEPrintDocuments,
														NewAEEventHandlerUPP(receivePrintDocuments),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEQuitApplication,
														NewAEEventHandlerUPP(receiveApplicationQuit),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEShowPreferences,
														NewAEEventHandlerUPP(receiveApplicationPrefs),
														0L, false);
	result = (noErr == error);
	
	return result;
}// installRequiredHandlers


/*!
Handles the "kASRequiredSuite" Apple Event of type
"kAEOpenApplication".

(3.1)
*/
OSErr
receiveApplicationOpen	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	// The current preferences version MUST match the value that is
	// emitted by "VersionInfo.sh" (the golden value that is mirrored
	// to other generated files in the bundle), because only that
	// value is consulted in order to determine when to run this
	// converter program.  The converter itself is not automatically
	// updated because NEW CODE MUST BE WRITTEN whenever the version
	// is incremented!  You should write a new function of the form
	// "actionVersionX()" and add a call to it below, in a form
	// similar to what has been done for previous versions.  Finally,
	// note that the version should never be decremented.
	SInt16 const	kCurrentPrefsVersion = 6;
	CFIndex			diskVersion = 0;
	Boolean			doConvert = false;
	Boolean			conversionSuccessful = false;
	My_PrefsResult	actionResult = kMy_PrefsResultOK;
	OSStatus		error = noErr;
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	
	
	error = noErr;
	
	// set up self address for Apple Events
	gSelfProcessID.highLongOfPSN = 0;
	gSelfProcessID.lowLongOfPSN = kCurrentProcess; // don’t use GetCurrentProcess()!
	error = AECreateDesc(typeProcessSerialNumber, &gSelfProcessID, sizeof(gSelfProcessID), &gSelfAddress);
	
	// always make this the frontmost application when it runs
	SetFrontProcess(&gSelfProcessID);
	
	// figure out what version of preferences is on disk; it is also
	// possible “no version” is there, indicating a need for legacy updates
	{
		Boolean		existsAndIsValidFormat = false;
		
		
		// the application domain must match what was used FOR THE VERSION
		// OF PREFERENCES DESIRED; in this case, versions 6 and beyond are
		// in the MacTerm domain
		diskVersion = CFPreferencesGetAppIntegerValue
						(CFSTR("prefs-version"), kMacTermApplicationID,
							&existsAndIsValidFormat);
		if (existsAndIsValidFormat)
		{
			// sanity check the saved version number; it ought to be no smaller
			// than the first version where the new domain was used (6)
			assert((diskVersion >= 6) && "transition to MacTerm domain started at version 6; it does not make sense for any older number to be present");
		}
		else
		{
			diskVersion = 0;
		}
		
		// maybe the preferences are saved in an older domain...
		if (0 == diskVersion)
		{
			// the application domain must match what was used FOR THE VERSION
			// OF PREFERENCES DESIRED; in this case, versions 5 and older were
			// in the MacTelnet domain, and later versions are now MacTerm, so
			// we must check the legacy domain as a fallback
			diskVersion = CFPreferencesGetAppIntegerValue
							(CFSTR("prefs-version"), kMacTelnetApplicationID,
								&existsAndIsValidFormat);
			unless (existsAndIsValidFormat)
			{
				diskVersion = 0;
			}
		}
	}
	
	// based on the version of the preferences on disk (which is zero
	// if there is no version), decide what to do, and how to interact
	// with the user
	if (kCurrentPrefsVersion == diskVersion)
	{
		// preferences are identical, nothing to do; if the application
		// was launched automatically, then it should not have been
		std::cerr << "MacTerm Preferences Converter: prefs-version on disk (" << diskVersion << ") is current, nothing to convert" << std::endl;
		doConvert = false;
	}
	else if (diskVersion > kCurrentPrefsVersion)
	{
		// preferences are newer, nothing to do; if the application
		// was launched automatically, then it should not have been
		std::cerr << "MacTerm Preferences Converter: prefs-version on disk (" << diskVersion << ") is newer than this converter ("
					<< kCurrentPrefsVersion << "), unable to convert" << std::endl;
		doConvert = false;
	}
	else
	{
		// preferences on disk are too old; conversion is required
		doConvert = true;
	}
	
	// convert preferences if necessary
	if (doConvert)
	{
		// Perform conversion by calling action routines in a sequence;
		// the checks below should be executed in order of increasing
		// version number.
		//
		// !!! IMPORTANT !!!
		// Actions are performed according to version.  Each clause
		// performs ONLY the operations required to upgrade from the
		// immediately preceding version.  Once defined, the series of
		// actions for a version CANNOT change, for backwards compatibility
		// (similarly, the order in which version numbers are considered
		// and the order actions are performed should not be modified).
		//
		// When new code is required here, add a new clause with a new
		// version number.  Increment "kCurrentPrefsVersion" to that latest
		// (largest) number, BOTH here as well as in MacTelnet (which reads
		// the value from a preference; see below for code that saves the
		// version value, and the code in Preferences.cp in MacTelnet that
		// reads it back).
		if (diskVersion < 1)
		{
			// These preferences are very old (resource forks from the
			// MacTelnet program) and as of MacTerm 4.1 they are no
			// longer auto-converted...they are just ignored.
			actionResult = kMy_PrefsResultGenericFailure;
		}
		if (diskVersion < 2)
		{
			// Version 2 was experimental and not used in practice.
			//actionResult = actionVersion2();
		}
		if (diskVersion < 3)
		{
			// Version 3 changed some key names and deleted obsolete keys.
			actionResult = actionVersion3();
		}
		if (diskVersion < 4)
		{
			// Version 4 deleted some obsolete keys.
			actionResult = actionVersion4();
		}
		if (diskVersion < 5)
		{
			// Version 5 deleted some obsolete keys.
			actionResult = actionVersion5();
		}
		if (diskVersion < 6)
		{
			// Version 6 migrated settings into a new domain.
			actionResult = actionVersion6();
		}
		// IMPORTANT: The maximum version considered here should be the
		//            current value of "kCurrentPrefsVersion"!!!
		//if (diskVersion < X)
		//{
		//	// Perform actions that update ONLY the immediately preceding
		//	// version to this one; the cumulative effects of each check
		//	// above will naturally allow really old preferences to update.
		//	. . .
		//}
		conversionSuccessful = (kMy_PrefsResultOK == actionResult);
		gFinished = true;
		
		// If conversion is successful, save a preference indicating the
		// current preferences version; MacTerm will read that setting
		// in the future to determine if this program ever needs to be
		// run again.
		//
		// NOTE:	For debugging, you may wish to disable this block.
	#if 1
		if (conversionSuccessful)
		{
			CFNumberRef		numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &kCurrentPrefsVersion);
			
			
			if (nullptr != numberRef)
			{
				// The preference key used here MUST match what MacTelnet uses to
				// read the value in, for obvious reasons.  Similarly, the application
				// domain must match what is in MacTelnet’s Info.plist file.
				CFPreferencesSetAppValue(CFSTR("prefs-version"), numberRef, kMacTermApplicationID);
				(Boolean)CFPreferencesAppSynchronize(kMacTermApplicationID);
				CFRelease(numberRef), numberRef = nullptr;
			}
		}
	#endif
		
		// This is probably not something the user really needs to see.
	#if 1
		std::cerr << "MacTerm Preferences Converter: conversion successful, new prefs-version is " << kCurrentPrefsVersion << std::endl;
	#else
		// display status to the user
		{
			// tell the user about the conversions that occurred
			AlertStdCFStringAlertParamRec	params;
			AlertType						alertType = kAlertNoteAlert;
			CFStringRef						messageText = nullptr;
			CFStringRef						helpText = nullptr;
			DialogRef						dialog = nullptr;
			
			
			error = GetStandardAlertDefaultParams(&params, kStdCFStringAlertVersionOne);
			params.defaultButton = kAlertStdAlertOKButton;
			if (conversionSuccessful)
			{
				params.defaultText = CFCopyLocalizedStringFromTable(CFSTR("Continue"), CFSTR("Alerts"), CFSTR("button label"));
				messageText = CFCopyLocalizedStringFromTable
								(CFSTR("Your preferences have been saved in an updated format, and outdated files (if any) have been moved to the Trash."),
									CFSTR("Alerts"), CFSTR("displayed upon successful conversion"));
				helpText = CFCopyLocalizedStringFromTable
							(CFSTR("This version of MacTerm will now be able to read your existing preferences."),
								CFSTR("Alerts"), CFSTR("displayed upon successful conversion"));
			}
			else
			{
				// identify problem
				params.defaultText = REINTERPRET_CAST(kAlertDefaultOKText, CFStringRef);
				switch (1) // temporary; there are no specific problems to construct explanation messages for, yet
				{
				default:
					alertType = kAlertCautionAlert;
					messageText = CFCopyLocalizedStringFromTable
									(CFSTR("This version of MacTerm cannot read your existing preferences.  Normally, old preferences are converted automatically, but this has failed for some reason."),
										CFSTR("Alerts"), CFSTR("displayed upon unsuccessful conversion, generic reason"));
					helpText = CFCopyLocalizedStringFromTable
								(CFSTR("Please check for file and disk problems, and try again.  Default preferences will be used instead."),
									CFSTR("Alerts"), CFSTR("displayed upon unsuccessful conversion, generic reason"));
					break;
				}
			}
			
			if (nullptr != messageText)
			{
				error = CreateStandardAlert(alertType, messageText, helpText, &params, &dialog);
				error = RunStandardAlert(dialog, nullptr/* filter */, nullptr/* item hit */);
			}
		}
	#endif
		
		// on older versions of Mac OS X, it is necessary to require the user to
		// restart the main application instead of simply continuing
		{
			char*	envValue = getenv("CONVERTER_ASK_USER_TO_RESTART");
			
			
			if ((nullptr != envValue) && (0 == strcmp(envValue, "1")))
			{
				// prompt the user to restart; the actual forced-quit is not performed
				// within the converter, but rather within the main Python script front-end
				AlertStdCFStringAlertParamRec	params;
				AlertType						alertType = kAlertStopAlert;
				CFStringRef						messageText = nullptr;
				CFStringRef						helpText = nullptr;
				DialogRef						dialog = nullptr;
				
				
				error = GetStandardAlertDefaultParams(&params, kStdCFStringAlertVersionOne);
				params.defaultButton = kAlertStdAlertOKButton;
				params.defaultText = CFCopyLocalizedStringFromTable(CFSTR("Quit Preferences Converter"), CFSTR("Alerts"), CFSTR("button label"));
				messageText = CFCopyLocalizedStringFromTable
								(CFSTR("Your preferences have been migrated to a new format.  Please run MacTerm again to use the migrated settings."),
									CFSTR("Alerts"), CFSTR("displayed upon successful conversion"));
				helpText = CFCopyLocalizedStringFromTable
							(CFSTR("This version of MacTerm will now be able to read your existing preferences."),
								CFSTR("Alerts"), CFSTR("displayed upon successful conversion"));
				
				if (nullptr != messageText)
				{
					error = CreateStandardAlert(alertType, messageText, helpText, &params, &dialog);
					error = RunStandardAlert(dialog, nullptr/* filter */, nullptr/* item hit */);
				}
			}
		}
	}
	else
	{
		// no action was requested
		gFinished = true;
	}
	
	// send a "quit" Apple Event to terminate the application
	// (and allow recording scripts to see that this happened)
	{
		AppleEvent		quitEvent;
		AppleEvent		reply;
		
		
		AEInitializeDesc(&quitEvent);
		AEInitializeDesc(&reply);
		error = AECreateAppleEvent(kASRequiredSuite, kAEQuitApplication, &gSelfAddress,
									kAutoGenerateReturnID, kAnyTransactionID, &quitEvent);
		if (noErr == error)
		{
			Boolean		confirm = false;
			
			
			if (confirm)
			{
				AEDesc		saveOptionsDesc;
				DescType	saveOptions = kAEYes;
				
				
				// add confirmation message parameter based on whether any connections are open
				AEInitializeDesc(&saveOptionsDesc);
				error = AECreateDesc(typeEnumerated, &saveOptions, sizeof(saveOptions), &saveOptionsDesc);
				error = AEPutParamDesc(&quitEvent, keyAESaveOptions, &saveOptionsDesc);
			}
			
			// send the message to quit
			error = AESend(&quitEvent, &reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout,
							nullptr/* idle routine */, nullptr/* filter routine */);
		}
		AEDisposeDesc(&quitEvent);
	}
	
	(OSStatus)addErrorToReply(error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationOpen


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEReopenApplication".

(3.1)
*/
OSErr
receiveApplicationReopen	(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr,
							 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationReopen


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEShowPreferences".

(3.1)
*/
OSErr
receiveApplicationPrefs		(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr,
							 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationPrefs


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEQuitApplication".

(3.1)
*/
OSErr
receiveApplicationQuit	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	Boolean			doQuit = true;
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	unless (gFinished)
	{
		// warn the user that they may be interrupting something
		AlertStdCFStringAlertParamRec	params;
		DialogRef						dialog = nullptr;
		DialogItemIndex					itemHit = kAlertStdAlertCancelButton;
		
		
		error = GetStandardAlertDefaultParams(&params, kStdCFStringAlertVersionOne);
		params.defaultText = CFCopyLocalizedStringFromTable(CFSTR("Quit"), CFSTR("Alerts"), CFSTR("button label"));
		params.defaultButton = kAlertStdAlertOKButton;
		params.cancelText = REINTERPRET_CAST(kAlertDefaultCancelText, CFStringRef);
		params.cancelButton = kAlertStdAlertCancelButton;
		error = CreateStandardAlert(kAlertCautionAlert,
									CFCopyLocalizedStringFromTable
									(CFSTR("Conversion is not yet complete.  Quit anyway?"),
										CFSTR("Alerts"), CFSTR("displayed if the user tries to quit in the middle of something")),
									CFCopyLocalizedStringFromTable
									(CFSTR("Some of your original preferences may not be converted."),
										CFSTR("Alerts"), CFSTR("displayed if the user tries to quit in the middle of something")),
									&params, &dialog);
		error = RunStandardAlert(dialog, nullptr/* filter */, &itemHit);
		doQuit = ((noErr == error) && (kAlertStdAlertOKButton == itemHit));
	}
	
	if (doQuit)
	{
		// close open files to prevent corruption
		if (0 != gOldPrefsFileRefNum) CloseResFile(gOldPrefsFileRefNum);
		
		// terminate loop, which will return control to main()
		QuitApplicationEventLoop();
	}
	
	(OSStatus)addErrorToReply(error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationQuit


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEOpenDocuments".

(3.1)
*/
OSErr
receiveOpenDocuments	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(error, outReplyAppleEventPtr);
	return result;
}// receiveOpenDocuments


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEPrintDocuments".

(3.1)
*/
OSErr
receivePrintDocuments	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(error, outReplyAppleEventPtr);
	return result;
}// receivePrintDocuments


/*!
Like setMacTelnetPreference(), but is useful for creating
coordinate values.  The work of creating a CFArrayRef
containing two CFNumberRefs is done for you.

Returns "true" only if it was possible to create the
necessary Core Foundation types.

IMPORTANT:	This is a legacy preferences domain.  It no
			longer makes any sense to use this except in
			converters that deal with older versions of
			the program.

(3.1)
*/
Boolean
setMacTelnetCoordPreference		(CFStringRef	inKey,
								 SInt16			inX,
								 SInt16			inY)
{
	CFNumberRef		leftCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &inX);
	Boolean			result = false;
	
	
	if (nullptr != leftCoord)
	{
		CFNumberRef		topCoord = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &inY);
		
		
		if (nullptr != topCoord)
		{
			CFNumberRef		values[] = { leftCoord, topCoord };
			CFArrayRef		coords = CFArrayCreate(kCFAllocatorDefault,
													REINTERPRET_CAST(values, void const**),
													sizeof(values) / sizeof(CFNumberRef),
													&kCFTypeArrayCallBacks);
			
			
			if (nullptr != coords)
			{
				setMacTelnetPreference(inKey, coords);
				result = true; // technically, it’s not possible to see if setMacTelnetPreference() succeeded...
				CFRelease(coords), coords = nullptr;
			}
			CFRelease(topCoord), topCoord = nullptr;
		}
		CFRelease(leftCoord), leftCoord = nullptr;
	}
	
	return result;
}// setMacTelnetCoordPreference


/*!
Updates MacTelnet’s XML preferences as requested.  Also
prints to standard output information about the data being
written (if debugging is enabled).

IMPORTANT:	This is a legacy preferences domain.  It no
			longer makes any sense to use this except in
			converters that deal with older versions of
			the program.

(3.1)
*/
void
setMacTelnetPreference	(CFStringRef		inKey,
						 CFPropertyListRef	inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, kMacTelnetApplicationID);
#if 1
	{
		// for debugging
		CFStringRef		descriptionCFString = CFCopyDescription(inValue);
		char const*		keyString = nullptr;
		char const*		descriptionString = nullptr;
		Boolean			disposeKeyString = false;
		Boolean			disposeDescriptionString = false;
		
		
		keyString = CFStringGetCStringPtr(inKey, CFStringGetFastestEncoding(inKey));
		if (nullptr == keyString)
		{
			// allocate double the size because the string length assumes 16-bit characters
			size_t const	kBufferSize = INTEGER_DOUBLED(CFStringGetLength(inKey));
			char*			bufferPtr = new char[kBufferSize];
			
			
			if (nullptr != bufferPtr)
			{
				keyString = bufferPtr;
				disposeKeyString = true;
				if (CFStringGetCString(inKey, bufferPtr, kBufferSize,
										CFStringGetFastestEncoding(inKey)))
				{
					// no idea what the problem is, but just bail
					std::strcpy(bufferPtr, "(error while trying to copy key data)");
				}
			}
		}
		
		if (nullptr == descriptionCFString)
		{
			descriptionString = "(no value description available)";
		}
		else
		{
			descriptionString = CFStringGetCStringPtr
								(descriptionCFString, CFStringGetFastestEncoding(descriptionCFString));
			if (nullptr == descriptionString)
			{
				// allocate double the size because the string length assumes 16-bit characters
				size_t const	kBufferSize = INTEGER_DOUBLED(CFStringGetLength(descriptionCFString));
				char*			bufferPtr = new char[kBufferSize];
				
				
				if (nullptr != bufferPtr)
				{
					descriptionString = bufferPtr;
					disposeDescriptionString = true;
					if (CFStringGetCString(descriptionCFString, bufferPtr, kBufferSize,
											CFStringGetFastestEncoding(descriptionCFString)))
					{
						// no idea what the problem is, but just bail
						std::strcpy(bufferPtr, "(error while trying to copy description data)");
					}
				}
			}
		}
		
		assert(nullptr != keyString);
		assert(nullptr != descriptionString);
		std::cerr << "saving " << keyString << " " << descriptionString << std::endl;
		
		if (disposeKeyString) delete [] keyString, keyString = nullptr;
		if (disposeDescriptionString) delete [] descriptionString, descriptionString = nullptr;
	}
#endif
}// setMacTelnetPreference


/*!
Updates MacTerm’s XML preferences as requested.

(4.0)
*/
void
setMacTermPreference	(CFStringRef		inKey,
						 CFPropertyListRef	inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, kMacTermApplicationID);
}// setMacTermPreference

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
