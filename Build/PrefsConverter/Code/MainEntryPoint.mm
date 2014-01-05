/*!	\file MainEntryPoint.mm
	\brief Front end to the Preferences Converter application.
*/
/*###############################################################

	MacTerm Preferences Converter
		© 2004-2014 by Kevin Grant.
	
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

#import "UniversalDefines.h"

// standard-C++ includes
#import <iostream>

// Mac includes
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>

// library includes
#import <CFDictionaryManager.h>
#import <CFKeyValueInterface.h>
#import <CocoaUserDefaults.h>



#pragma mark Constants
namespace {

// the legacy MacTelnet bundle ID
CFStringRef const	kMacTelnetApplicationID = CFSTR("com.mactelnet.MacTelnet");

// the new bundle ID for MacTerm
CFStringRef const	kMacTermApplicationID = CFSTR("net.macterm.MacTerm");

enum My_PrefsResult
{
	kMy_PrefsResultOK				= 0,	//!< no error
	kMy_PrefsResultGenericFailure	= 1		//!< some error
};

} // anonymous namespace

#pragma mark Variables
namespace {

Boolean		gFinished = false;			//!< used to control exit warnings to the user

} // anonymous namespace

#pragma mark Internal Method Prototypes

/*!
The delegate for NSApplication.
*/
@interface PrefsConverterAppDelegate : NSObject //{

// NSApplicationDelegate
	- (void)
	applicationDidFinishLaunching:(NSNotification*)_;

@end //}

namespace {

My_PrefsResult		actionVersion3	();
My_PrefsResult		actionVersion4	();
My_PrefsResult		actionVersion5	();
My_PrefsResult		actionVersion6	();
My_PrefsResult		actionVersion7	();

} // anonymous namespace



#pragma mark Public Methods

/*!
Main entry point.
*/
int
main	(int			argc,
		 const char*	argv[])
{
	return NSApplicationMain(argc, argv);
}


#pragma mark Internal Methods
namespace {

/*!
Upgrades from version 2 to version 3.  Some keys are
now obsolete that were never available to users so
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
now obsolete so they are deleted.

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
now obsolete so they are deleted.

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
							
							
							UNUSED_RETURN(CFIndex)CFStringFindAndReplace(newDomainCFString.returnCFMutableStringRef(), kMacTelnetApplicationID,
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
Upgrades from version 6 to version 7.  Some keys are
now obsolete so they are deleted.

(4.1)
*/
My_PrefsResult
actionVersion7 ()
{
	My_PrefsResult		result = kMy_PrefsResultOK;
	
	
	// these settings are no longer used
	CFPreferencesSetAppValue(CFSTR("terminal-capture-file-alias-id"), nullptr/* delete value */, kMacTermApplicationID);
	CFPreferencesSetAppValue(CFSTR("terminal-capture-file-creator-code"), nullptr/* delete value */, kMacTermApplicationID);
	CFPreferencesSetAppValue(CFSTR("terminal-capture-file-open-with-application"), nullptr/* delete value */, kMacTermApplicationID);
	CFPreferencesSetAppValue(CFSTR("terminal-capture-folder"), nullptr/* delete value */, kMacTermApplicationID);
	
	return result;
}// actionVersion7

} // anonymous namespace


@implementation PrefsConverterAppDelegate

- (void)
applicationDidFinishLaunching:(NSNotification*)		aNotification
{
#pragma unused(aNotification)

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
	SInt16 const	kCurrentPrefsVersion = 7;
	CFIndex			diskVersion = 0;
	Boolean			doConvert = false;
	Boolean			conversionSuccessful = false;
	My_PrefsResult	actionResult = kMy_PrefsResultOK;
	
	
	// bring this process to the front
	[NSApp activateIgnoringOtherApps:YES];
	
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
		if (diskVersion < 7)
		{
			// Version 7 deleted some obsolete keys.
			actionResult = actionVersion7();
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
				UNUSED_RETURN(Boolean)CFPreferencesAppSynchronize(kMacTermApplicationID);
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
			NSString*	button1 = nil;
			NSString*	messageText = nil;
			NSString*	helpText = nil;
			NSAlert*	alertBox = nil;
			//int			clickedButton = NSAlertFirstButtonReturn;
			
			
			if (conversionSuccessful)
			{
				button1 = NSLocalizedString(@"Continue", @"button label");
				messageText = NSLocalizedString(@"Your preferences have been saved in an updated format, and outdated files (if any) have been moved to the Trash.",
												@"displayed upon successful conversion");
				helpText = NSLocalizedString(@"This version of MacTerm will now be able to read your existing preferences.",
												@"displayed upon successful conversion");
				alertBox = [NSAlert alertWithMessageText:messageText defaultButton:button1 alternateButton:nil
															otherButton:nil informativeTextWithFormat:helpText, nil];
			}
			else
			{
				// identify problem
				button1 = NSLocalizedString(@"OK", @"button label");
				switch (1) // temporary; there are no specific problems to construct explanation messages for, yet
				{
				default:
					messageText = NSLocalizedString(@"This version of MacTerm cannot read your existing preferences.  Normally, old preferences are converted automatically but this has failed for some reason.",
													@"displayed upon unsuccessful conversion, generic reason");
					helpText = NSLocalizedString(@"Please check for file and disk problems, and try again.  Default preferences will be used instead.",
													@"displayed upon unsuccessful conversion, generic reason");
					alertBox = [NSAlert alertWithMessageText:messageText defaultButton:button1 alternateButton:nil
																otherButton:nil informativeTextWithFormat:helpText, nil];
					[alertBox setIcon:[NSImage imageNamed:NSImageNameCaution]];
					break;
				}
			}
			
			// display an error to the user
			UNUSED_RETURN(int)[alertBox runModal];
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
				NSString*	button1 = NSLocalizedString(@"Quit Preferences Converter", @"button label");
				NSString*	messageText = NSLocalizedString(@"Your preferences have been migrated to a new format.  Please run MacTerm again to use the migrated settings.",
															@"displayed upon successful conversion");
				NSString*	helpText = NSLocalizedString(@"This version of MacTerm will now be able to read your existing preferences.",
															@"displayed upon successful conversion");
				//int			clickedButton = NSAlertFirstButtonReturn;
				
				
				// display a message to the user
				UNUSED_RETURN(int)[[NSAlert alertWithMessageText:messageText defaultButton:button1 alternateButton:nil
																	otherButton:nil informativeTextWithFormat:helpText, nil]
									runModal];
			}
		}
	}
	else
	{
		// no action was requested
		gFinished = true;
	}
	
	// quit the preferences converter application
	[NSApp terminate:nil];
}// applicationDidFinishLaunching:


@end // PrefsConverterAppDelegate

// BELOW IS REQUIRED NEWLINE TO END FILE
