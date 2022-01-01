/*!	\file MainEntryPoint.swift
	\brief Front end to the Preferences Converter application.
*/
/*###############################################################

	MacTerm Preferences Converter
		© 2004-2022 by Kevin Grant.
	
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

// Mac includes
import AppKit
import Foundation



// MARK: Constants

// the legacy MacTelnet bundle ID
let kMacTelnetApplicationID = "com.mactelnet.MacTelnet"

// the new bundle ID for MacTerm
let kMacTermApplicationID = "net.macterm.MacTerm"


// MARK: Types

/*!
The delegate for NSApplication.
*/
@NSApplicationMain
class PrefsConverterAppDelegate : NSObject, NSApplicationDelegate
{

// MARK: Public Methods

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
func
actionVersion3 () -> Bool
{
	NSLog("MacTerm Preferences Converter: migrating settings to prefs-version 3")
	
	var result = false
	if let defaultsForMacTelnet = UserDefaults(suiteName:kMacTelnetApplicationID)
	{
		// this is now "favorite-macro-sets" and redefined as a simple array of domain names
		defaultsForMacTelnet.removeObject(forKey:"favorite-macros")
		
		// this is now "favorite-formats" and redefined as a simple array of domain names
		defaultsForMacTelnet.removeObject(forKey:"favorite-styles")
		
		result = true
	}
	
	return result
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
func
actionVersion4 () -> Bool
{
	NSLog("MacTerm Preferences Converter: migrating settings to prefs-version 4")
	
	var result = false
	if let defaultsForMacTelnet = UserDefaults(suiteName:kMacTelnetApplicationID)
	{
		// this setting is no longer used
		defaultsForMacTelnet.removeObject(forKey:"menu-command-set-simplified")
		
		// this setting is no longer used
		defaultsForMacTelnet.removeObject(forKey:"menu-key-equivalents")
		
		// these settings are no longer used
		defaultsForMacTelnet.removeObject(forKey:"macro-menu-name-string")
		defaultsForMacTelnet.removeObject(forKey:"macro-menu-visible")
		defaultsForMacTelnet.removeObject(forKey:"menu-macros-visible")
		defaultsForMacTelnet.removeObject(forKey:"menu-visible")
		
		// these settings are no longer used
		defaultsForMacTelnet.removeObject(forKey:"window-macroeditor-position-pixels")
		defaultsForMacTelnet.removeObject(forKey:"window-macroeditor-size-pixels")
		defaultsForMacTelnet.removeObject(forKey:"window-macroeditor-visible")
		
		result = true
	}
	
	return result
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
func
actionVersion5 () -> Bool
{
	NSLog("MacTerm Preferences Converter: migrating settings to prefs-version 5")
	
	var result = false
	if let defaultsForMacTelnet = UserDefaults(suiteName:kMacTelnetApplicationID)
	{
		// these settings are no longer used
		defaultsForMacTelnet.removeObject(forKey:"window-commandline-position-pixels")
		defaultsForMacTelnet.removeObject(forKey:"window-commandline-size-pixels")
		defaultsForMacTelnet.removeObject(forKey:"window-controlkeys-position-pixels")
		defaultsForMacTelnet.removeObject(forKey:"window-functionkeys-position-pixels")
		defaultsForMacTelnet.removeObject(forKey:"window-preferences-position-pixels")
		defaultsForMacTelnet.removeObject(forKey:"window-sessioninfo-column-order")
		defaultsForMacTelnet.removeObject(forKey:"window-vt220keys-position-pixels")
		
		result = true
	}
	
	return result
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
func
actionVersion6 () -> Bool
{
	NSLog("MacTerm Preferences Converter: migrating settings to prefs-version 6")
	
	let result = true
	
	
	// look for references to the old domain and update those settings accordingly
	if let defaultsForMacTelnet = UserDefaults(suiteName:kMacTelnetApplicationID),
		let defaultsForMacTerm = UserDefaults(suiteName:kMacTermApplicationID)
	{
		// copy everything from the base set into the new domain
		UserDefaults.standard.setPersistentDomain(defaultsForMacTelnet.dictionaryRepresentation(), forName:kMacTermApplicationID)
		
		// copy any other domains as needed
		for aKey in [
						"favorite-formats",
						"favorite-macro-sets",
						"favorite-sessions",
						"favorite-terminals",
						"favorite-translations",
						"favorite-workspaces"
					]
		{
			if let valueList = defaultsForMacTelnet.stringArray(forKey:aKey)
			{
				if valueList.count > 0
				{
					var newArray = [String]()
					
					
					for oldDomain in valueList
					{
						let searchRange = Range(NSMakeRange(0, oldDomain.count), in:oldDomain)
						let newDomain = oldDomain.replacingOccurrences(of:kMacTelnetApplicationID,
																		with:kMacTermApplicationID,
																		options:[.caseInsensitive],
																		range:searchRange)
						
						
						// store only the new domain, and copy all settings from the
						// old collection domain to the new collection domain; at
						// that point, the old settings can be removed
						newArray.append(newDomain)
						
						if let oldPrefs = UserDefaults(suiteName:oldDomain)
						{
							UserDefaults.standard.setPersistentDomain(oldPrefs.dictionaryRepresentation(), forName:newDomain)
						}
						
						// new settings were successfully saved; delete the old ones
						defaultsForMacTelnet.removePersistentDomain(forName:oldDomain)
					}
					defaultsForMacTerm.set(newArray, forKey:aKey)
				}
			}
		}
	}
	
	// final step: delete all the old settings
	UserDefaults.standard.removePersistentDomain(forName:kMacTelnetApplicationID)
	
	return result
}// actionVersion6


/*!
Upgrades from version 6 to version 7.  Some keys are
now obsolete so they are deleted.

(4.1)
*/
func
actionVersion7 () -> Bool
{
	NSLog("MacTerm Preferences Converter: migrating settings to prefs-version 7")
	
	var result = false
	if let defaultsForMacTerm = UserDefaults(suiteName:kMacTermApplicationID)
	{
		// these settings are no longer used
		defaultsForMacTerm.removeObject(forKey:"terminal-capture-file-alias-id")
		defaultsForMacTerm.removeObject(forKey:"terminal-capture-file-creator-code")
		defaultsForMacTerm.removeObject(forKey:"terminal-capture-file-open-with-application")
		defaultsForMacTerm.removeObject(forKey:"terminal-capture-folder")
		
		result = true
	}
	
	return result
}// actionVersion7


/*!
Upgrades from version 7 to version 8.  Data that used
alias format is converted into bookmark data.

(2017.05)
*/
func
actionVersion8 () -> Bool
{
	NSLog("MacTerm Preferences Converter: migrating settings to prefs-version 8")
	
	guard let defaultsForMacTerm = UserDefaults(suiteName:kMacTermApplicationID) else { return false }
	let result = true
	var allSessionDomains = [String]()
	
	// find all possible domains that could have old settings
	if let favoriteSessionDomains = defaultsForMacTerm.stringArray(forKey:"favorite-sessions")
	{
		allSessionDomains += favoriteSessionDomains
	}
	allSessionDomains.append(kMacTermApplicationID)
	
	// convert old alias records to new bookmarks in all
	// the places that have them; then, delete the old ones
	for searchDomain in allSessionDomains
	{
		guard let defaultsForSearchDomain = UserDefaults(suiteName:searchDomain) else { continue }
		if let _ = defaultsForSearchDomain.object(forKey:"terminal-capture-directory-alias")
		{
			if let aliasData = defaultsForSearchDomain.data(forKey:"terminal-capture-directory-alias")
			{
				// convert the alias to a bookmark and store that instead
				let bookmarkData = CFURLCreateBookmarkDataFromAliasRecord(kCFAllocatorDefault, aliasData as CFData).takeRetainedValue() as Data
				
				// store the bookmark under the new key name (this must
				// agree with what MacTerm uses to read it now)
				defaultsForSearchDomain.set(bookmarkData, forKey:"terminal-capture-directory-bookmark")
				
				// debug: recreate the URL to see what it pointed to
				// (this helps to see that the conversion was OK)
				if true
				{
					do
					{
						var isStale = false
						let recreatedURL = try URL(resolvingBookmarkData:bookmarkData,
													options:[.withoutUI/*, .withoutMounting*/],
													relativeTo:nil, bookmarkDataIsStale:&isStale)
						NSLog("recreated URL: \(recreatedURL)") // debug
					}
					catch
					{
						NSLog("failed to recreate URL, error: \(error)")
					}
				}
			}
			else
			{
				// expected the saved value to be a data blob
				NSLog("MacTerm Preferences Converter: actionVersion8(): subdomain 'terminal-capture-directory-alias' value does not have raw data type")
			}
			
			// finally, remove the original alias data from this domain
			defaultsForSearchDomain.removeObject(forKey:"terminal-capture-directory-alias")
		}
	}
	
	return result
}// actionVersion8

// MARK: NSApplicationDelegate

/*!
Effectively the main entry point; called by the OS when
the launch is complete.

(4.0)
*/
func
applicationDidFinishLaunching(_/* <- critical underscore; without it, no app launch (different method!) */ aNotification: Notification)
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
	let kCurrentPrefsVersion = 8
	var diskVersion = 0
	var doConvert = false
	var conversionSuccessful = false
	
	
	// bring this process to the front
	NSApp.activate(ignoringOtherApps:true)
	
	// figure out what version of preferences is on disk; it is also
	// possible “no version” is there, indicating a need for legacy updates
	if let defaultsForMacTerm = UserDefaults(suiteName:kMacTermApplicationID)
	{
		// the application domain must match what was used FOR THE VERSION
		// OF PREFERENCES DESIRED; in this case, versions 6 and beyond are
		// in the MacTerm domain
		if let _ = defaultsForMacTerm.object(forKey:"prefs-version")
		{
			// sanity check the saved version number; it ought to be no smaller
			// than the first version where the new domain was used (6)
			diskVersion = defaultsForMacTerm.integer(forKey:"prefs-version")
			assert(diskVersion >= 6, "transition to MacTerm domain started at version 6; it does not make sense for any older number to be present")
		}
		else
		{
			// the application domain must match what was used FOR THE VERSION
			// OF PREFERENCES DESIRED; in this case, versions 5 and older were
			// in the MacTelnet domain, and later versions are now MacTerm, so
			// we must check the legacy domain as a fallback
			if let defaultsForMacTelnet = UserDefaults(suiteName:kMacTelnetApplicationID),
				let _ = defaultsForMacTelnet.object(forKey:"prefs-version")
			{
				diskVersion = defaultsForMacTelnet.integer(forKey:"prefs-version")
				assert(diskVersion <= 5, "transition from MacTelnet domain started at version 6; it does not make sense for any newer number to be present")
			}
			else
			{
				// perform all legacy updates
				diskVersion = 0
			}
		}
	}
	
	// based on the version of the preferences on disk (which is zero
	// if there is no version), decide what to do, and how to interact
	// with the user
	if kCurrentPrefsVersion == diskVersion
	{
		// preferences are identical, nothing to do; if the application
		// was launched automatically, then it should not have been
		NSLog("MacTerm Preferences Converter: prefs-version on disk (\(diskVersion)) is current, nothing to convert")
		doConvert = false
	}
	else if diskVersion > kCurrentPrefsVersion
	{
		// preferences are newer, nothing to do; if the application
		// was launched automatically, then it should not have been
		NSLog("MacTerm Preferences Converter: prefs-version on disk (\(diskVersion)) is newer than this converter (\(kCurrentPrefsVersion)), unable to convert")
		doConvert = false
	}
	else
	{
		// preferences on disk are too old; conversion is required
		doConvert = true
	}
	
	// convert preferences if necessary
	if doConvert
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
		conversionSuccessful = true // initially...
		if diskVersion < 1
		{
			// These preferences are very old (resource forks from the
			// MacTelnet program) and as of MacTerm 4.1 they are no
			// longer auto-converted...they are just ignored.
			conversionSuccessful = false
		}
		if diskVersion < 2
		{
			// Version 2 was experimental and not used in practice.
			//if !actionVersion2() { conversionSuccessful = false }
		}
		if diskVersion < 3
		{
			// Version 3 changed some key names and deleted obsolete keys.
			if !actionVersion3() { conversionSuccessful = false }
		}
		if diskVersion < 4
		{
			// Version 4 deleted some obsolete keys.
			if !actionVersion4() { conversionSuccessful = false }
		}
		if diskVersion < 5
		{
			// Version 5 deleted some obsolete keys.
			if !actionVersion5() { conversionSuccessful = false }
		}
		if diskVersion < 6
		{
			// Version 6 migrated settings into a new domain.
			if !actionVersion6() { conversionSuccessful = false }
		}
		if diskVersion < 7
		{
			// Version 7 deleted some obsolete keys.
			if !actionVersion7() { conversionSuccessful = false }
		}
		if diskVersion < 8
		{
			// Version 8 converts alias data into bookmark data.
			if !actionVersion8() { conversionSuccessful = false }
		}
		// IMPORTANT: The maximum version considered here should be the
		//            current value of "kCurrentPrefsVersion"!!!
		//if diskVersion < X
		//{
		//	// Perform actions that update ONLY the immediately preceding
		//	// version to this one; the cumulative effects of each check
		//	// above will naturally allow really old preferences to update.
		//	. . .
		//}
		
		// If conversion is successful, save a preference indicating the
		// current preferences version; MacTerm will read that setting
		// in the future to determine if this program ever needs to be
		// run again.
		//
		// NOTE:	For debugging, you may wish to disable this block.
		if conversionSuccessful
		{
			if let defaultsForMacTerm = UserDefaults(suiteName:kMacTermApplicationID)
			{
				// The preference key used here MUST match what MacTerm uses to
				// read the value in, for obvious reasons.  Similarly, the application
				// domain must match what is in MacTerm’s Info.plist file.
				defaultsForMacTerm.set(kCurrentPrefsVersion, forKey:"prefs-version")
			}
		}
		
		// This is probably not something the user really needs to see.
		NSLog("MacTerm Preferences Converter: conversion successful, new prefs-version is \(kCurrentPrefsVersion)")
		// display status to the user
		if true
		{
			// tell the user about the conversions that occurred
			let alertBox = NSAlert()
			var button1: String? = nil
			var messageText: String? = nil
			var helpText: String? = nil
			
			
			if conversionSuccessful
			{
				let enButton1 = "Continue"
				let enMessageText = "Your preferences have been saved in an updated format, and outdated files (if any) have been moved to the Trash."
				let enHelpText = "This version of MacTerm will now be able to read your existing preferences."
				button1 = NSLocalizedString(enButton1, comment:"button label")
				messageText = NSLocalizedString(enMessageText, comment:"displayed upon successful conversion")
				helpText = NSLocalizedString(enHelpText, comment:"displayed upon successful conversion")
				alertBox.messageText = messageText ?? enMessageText
				alertBox.informativeText = helpText ?? enHelpText
				/*var unused: NSButton =*/alertBox.addButton(withTitle:button1 ?? enButton1)
			}
			else
			{
				// identify problem
				switch (1) // temporary; there are no specific problems to construct explanation messages for, yet
				{
				default:
					let enMessageText = "This version of MacTerm cannot read your existing preferences.  Normally, old preferences are converted automatically but this has failed for some reason."
					let enHelpText = "Please check for file and disk problems, and try again.  Default preferences will be used instead."
					let enButton1 = "OK"
					messageText = NSLocalizedString(enMessageText, comment:"displayed upon unsuccessful conversion, generic reason")
					helpText = NSLocalizedString(enHelpText, comment:"displayed upon unsuccessful conversion, generic reason")
					button1 = NSLocalizedString(enButton1, comment:"button label")
					alertBox.alertStyle = NSAlert.Style.critical
					alertBox.messageText = messageText ?? enMessageText
					alertBox.informativeText = helpText ?? enHelpText
					/*var unused: NSButton =*/alertBox.addButton(withTitle:button1 ?? enButton1)
				}
			}
			
			// display an error to the user
			///*var unused: NSApplication.ModalResponse =*/alertBox.runModal()
		}
		
		// on older versions of Mac OS X, it is necessary to require the user to
		// restart the main application instead of simply continuing
		if let varValue = ProcessInfo.processInfo.environment["CONVERTER_ASK_USER_TO_RESTART"]
		{
			if varValue == "1"
			{
				// prompt the user to restart; the actual forced-quit is not performed
				// within the converter, but rather within the main Python script front-end
				let alertBox = NSAlert()
				let button1: String = NSLocalizedString("Quit Preferences Converter", comment:"button label")
				let messageText: String = NSLocalizedString("Your preferences have been migrated to a new format.  Please run MacTerm again to use the migrated settings.",
															comment:"displayed upon successful conversion")
				let helpText: String = NSLocalizedString("This version of MacTerm will now be able to read your existing preferences.",
															comment:"displayed upon successful conversion")
				
				
				// display a message to the user
				alertBox.messageText = messageText
				alertBox.informativeText = helpText
				/*var unused: NSButton =*/alertBox.addButton(withTitle:button1)
				
				/*var unused: NSApplication.ModalResponse =*/alertBox.runModal()
			}
		}
	}
	else
	{
		// no action was requested
	}
	
	// quit the preferences converter application
	NSApp.terminate(nil)
}// applicationDidFinishLaunching

}// PrefsConverterAppDelegate

// BELOW IS REQUIRED NEWLINE TO END FILE
