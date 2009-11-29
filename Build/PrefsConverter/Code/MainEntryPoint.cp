/*###############################################################

	MainEntryPoint.cp
	
	MacTelnet Preferences Converter
		© 2004-2008 by Kevin Grant.
	
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
#include <UniversalDefines.h>

// MacTelnet Preferences Converter includes
#include "PreferencesData.h"



#pragma mark Constants
namespace {

// these should match what is in MacTelnet’s Info.plist file;
// note also that IF MacTelnet’s identifier ever changes,
// the old names may need to be preserved for the purpose of
// reading older preferences
CFStringRef const		kMacTelnetApplicationID = CFSTR("com.mactelnet.MacTelnet");

enum
{
	kASRequiredSuite = kCoreEventClass
};

typedef SInt16 My_MacTelnetFolder;
enum
{
	// folders defined by MacTelnet
	kMy_MacTelnetFolderFavorites = 2,					// MacTelnet’s “Favorites” folder, in the preferences folder.
	
	kMy_MacTelnetFolderPreferences = 4,				// The folder “MacTelnet Preferences”, in the preferences
												//   folder of the user currently logged in.
	
	kMy_MacTelnetFolderScriptsMenuItems = 5,			// MacTelnet’s “Scripts Menu Items” folder.
	
	kMy_MacTelnetFolderStartupItems = 6,				// MacTelnet’s “Startup Items” folder, in MacTelnet’s
												//   preferences folder.
	
	kMy_MacTelnetFolderUserMacroFavorites = 8,		// “Macro Sets” folder, in the Favorites folder.
	
	kMy_MacTelnetFolderUserProxyFavorites = 9,		// “Proxies” folder, in the Favorites folder.
	
	kMy_MacTelnetFolderUserSessionFavorites = 10,		// “Sessions” folder, in the Favorites folder.
	
	kMy_MacTelnetFolderUserTerminalFavorites = 11,	// “Terminals” folder, in the Favorites folder.
	
	kMy_MacTelnetFolderRecentSessions = 12,			// “Recent Sessions” folder, in MacTelnet’s preferences folder.
	
	// folders defined by the Mac OS
	kMy_MacTelnetFolderMacPreferences = -1,			// the Preferences folder for the user currently logged in
	
	kMy_MacTelnetFolderMacTrash = -8					// the Trash
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

My_PrefsResult		actionLegacyUpdates			();
My_PrefsResult		actionVersion3				();
My_PrefsResult		actionVersion4				();
OSStatus			addErrorToReply				(ConstStringPtr, OSStatus, AppleEventPtr);
Boolean				convertRGBColorToCFArray	(RGBColor const*, CFArrayRef&);
My_StringResult		copyFileOrFolderCFString	(My_FolderStringType, CFStringRef*);
long				getDirectoryIDFromFSSpec	(FSSpec const*);
OSStatus			getFolderFSSpec				(My_MacTelnetFolder, FSSpec*);
Boolean				installRequiredHandlers		();
OSStatus			loadPreferencesStructure	(void*, size_t, ResType, SInt16);
OSStatus			makeLocalizedFSSpec			(SInt16, SInt32, My_FolderStringType, FSSpec*);
pascal OSErr		receiveApplicationOpen		(AppleEvent const*, AppleEvent*, SInt32);
pascal OSErr		receiveApplicationPrefs		(AppleEvent const*, AppleEvent*, SInt32);
pascal OSErr		receiveApplicationReopen	(AppleEvent const*, AppleEvent*, SInt32);
pascal OSErr		receiveApplicationQuit		(AppleEvent const*, AppleEvent*, SInt32);
pascal OSErr		receiveOpenDocuments		(AppleEvent const*, AppleEvent*, SInt32);
pascal OSErr		receivePrintDocuments		(AppleEvent const*, AppleEvent*, SInt32);
Boolean				setMacTelnetCoordPreference	(CFStringRef, SInt16, SInt16);
void				setMacTelnetPreference		(CFStringRef, CFPropertyListRef);
void				transformFolderFSSpec		(FSSpec*);

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
Performs all necessary operations to bring data
from the pre-XML era into the CFPreferences era.

Searches for old an preferences file named
"Telnet 3.0 Preferences", and renames it to
"MacTelnet Preferences".

INCOMPLETE
- must read MacTelnet Preferences and import
  resource data into XML
- must delete unused folders

(3.1)
*/
My_PrefsResult
actionLegacyUpdates ()
{
	My_PrefsResult	result = kMy_PrefsResultOK;
	FSSpec			parseFolder;
	OSStatus		error = noErr;
	
	
	error = getFolderFSSpec(kMy_MacTelnetFolderPreferences, &parseFolder);
	if (noErr == error)
	{
		FSSpec		parseFile;
		
		
		// rename any old preferences file
		error = FSMakeFSSpec(parseFolder.vRefNum, parseFolder.parID, "\pTelnet 3.0 Preferences", &parseFile);
		if (noErr == error)
		{
			FileInfo	info;
			
			
			// MacTelnet will have name-locked this file, so remove that
			// attribute before attempting to rename it
			FSpGetFInfo(&parseFile, (FInfo*)&info);
			info.finderFlags &= (~kNameLocked);
			FSpSetFInfo(&parseFile, (FInfo*)&info);
			
			// attempt a rename
			error = FSpRename(&parseFile, "\pMacTelnet Preferences");
		}
		else if (fnfErr == error)
		{
			// not there, no big deal
		}
		else
		{
			// some kind of problem referencing the file...
			result = kMy_PrefsResultGenericFailure;
		}
		
		// load any "MacTelnet Preferences" file
		error = FSMakeFSSpec(parseFolder.vRefNum, parseFolder.parID, "\pMacTelnet Preferences", &parseFile);
		if (noErr == error)
		{
			// open the preferences file and keep its reference number; note that
			// FSpOpenResFile() changes the current resource file
			SInt16		resFile = 0;
			
			
			resFile = FSpOpenResFile(&parseFile, fsRdPerm);
			if (resFile < 0)
			{
				error = ResError();
			}
			else
			{
				gOldPrefsFileRefNum = resFile;
			}
		}
		
		// if the file is actually open, read everything in it,
		// save it in a new format and then Trash the old file
		if (0 != gOldPrefsFileRefNum)
		{
			ApplicationPrefs	appPrefs;
			WindowPrefs			windowPrefs;
			Handle				handle = nullptr;
			SInt16				i = 0;
			SInt16				collectionCount = 0;
			
			
			// read application preferences resource (there should be only one of these);
			// the resource types and IDs here must match what was used in the original file;
			// so no fancy constants, etc. are used here, these values simply must not change
			UseResFile(gOldPrefsFileRefNum);
			error = loadPreferencesStructure(&appPrefs, sizeof(appPrefs), 'APRF'/* type */, 128/* ID */);
			if (noErr == error)
			{
				CFStringRef		prefsKey = nullptr;
				
				
				//
				// convert settings to their CFPreferences equivalents
				//
				
				prefsKey = CFSTR("terminal-cursor-shape");
				switch (appPrefs.cursorType)
				{
				case kTerminalCursorTypeUnderscore:
					setMacTelnetPreference(prefsKey, CFSTR("underline"));
					break;
				
				case kTerminalCursorTypeVerticalLine:
					setMacTelnetPreference(prefsKey, CFSTR("vertical bar"));
					break;
				
				case kTerminalCursorTypeThickUnderscore:
					setMacTelnetPreference(prefsKey, CFSTR("thick underline"));
					break;
				
				case kTerminalCursorTypeThickVerticalLine:
					setMacTelnetPreference(prefsKey, CFSTR("thick vertical bar"));
					break;
				
				case kTerminalCursorTypeBlock:
				default:
					setMacTelnetPreference(prefsKey, CFSTR("block"));
					break;
				}
				
				prefsKey = CFSTR("spaces-per-tab");
				{
					CFNumberRef		numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type,
																&appPrefs.copyTableThreshold);
					
					
					if (nullptr != numberRef)
					{
						setMacTelnetPreference(prefsKey, numberRef);
						CFRelease(numberRef), numberRef = nullptr;
					}
				}
				
				prefsKey = CFSTR("terminal-capture-file-creator-code");
				{
					CFStringRef		stringRef = CFStringCreateWithBytes
												(kCFAllocatorDefault,
													REINTERPRET_CAST(&appPrefs.captureFileCreator, UInt8 const*),
													sizeof(appPrefs.captureFileCreator),
													kCFStringEncodingMacRoman, false/* is external representation */);
					
					
					if (nullptr != stringRef)
					{
						setMacTelnetPreference(prefsKey, stringRef);
						CFRelease(stringRef), stringRef = nullptr;
					}
				}
				
				prefsKey = CFSTR("no-auto-close");
				setMacTelnetPreference(prefsKey, (appPrefs.windowsDontGoAway) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("terminal-when-bell-in-background");
				setMacTelnetPreference(prefsKey, (appPrefs.backgroundNotification) ? CFSTR("notify") : CFSTR("ignore"));
				
				prefsKey = CFSTR("menu-key-equivalents");
				setMacTelnetPreference(prefsKey, (appPrefs.menusHaveKeyEquivalents) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("key-map-backquote");
				setMacTelnetPreference(prefsKey, (appPrefs.remapBackquoteToEscape) ? CFSTR("\\e") : CFSTR(""));
				
				prefsKey = CFSTR("terminal-cursor-blinking");
				setMacTelnetPreference(prefsKey, (appPrefs.cursorBlinks) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("terminal-when-bell");
				setMacTelnetPreference(prefsKey, (appPrefs.visualBell) ? CFSTR("visual") : CFSTR("audio"));
				
				prefsKey = CFSTR("window-terminal-toolbar-invisible");
				setMacTelnetPreference(prefsKey, (appPrefs.headersInitiallyCollapsed) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("terminal-no-dim-on-deactivate");
				setMacTelnetPreference(prefsKey, (appPrefs.dontDimBackgroundScreens) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("terminal-inverse-selections");
				setMacTelnetPreference(prefsKey, (appPrefs.invertedTextHighlighting) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("terminal-auto-copy-on-select");
				setMacTelnetPreference(prefsKey, (appPrefs.copySelectedText) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("terminal-cursor-auto-move-on-drop");
				setMacTelnetPreference(prefsKey, (appPrefs.autoCursorMoveOnDrop) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("terminal-when-cursor-near-right-margin");
				setMacTelnetPreference(prefsKey, (appPrefs.marginBell) ? CFSTR("bell") : CFSTR("ignore"));
				
				prefsKey = CFSTR("no-auto-new");
				setMacTelnetPreference(prefsKey, (appPrefs.doNotInvokeNewOnApplicationReopen) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("window-clipboard-visible");
				setMacTelnetPreference(prefsKey, (appPrefs.wasClipboardShowing) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("window-commandline-visible");
				setMacTelnetPreference(prefsKey, (appPrefs.wasCommandLineShowing) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("window-controlkeys-visible");
				setMacTelnetPreference(prefsKey, (appPrefs.wasControlKeypadShowing) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("window-functionkeys-visible");
				setMacTelnetPreference(prefsKey, (appPrefs.wasFunctionKeypadShowing) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("window-macroeditor-visible");
				setMacTelnetPreference(prefsKey, (appPrefs.wasMacroKeypadShowing) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("window-vt220keys-visible");
				setMacTelnetPreference(prefsKey, (appPrefs.wasVT220KeypadShowing) ? kCFBooleanTrue : kCFBooleanFalse);
				
				prefsKey = CFSTR("menu-command-set-simplified");
				setMacTelnetPreference(prefsKey, kCFBooleanFalse);
				
				prefsKey = CFSTR("when-alert-in-background");
				switch (appPrefs.notificationPrefs)
				{
				case 0: // kAlert_NotifyDoNothing
					setMacTelnetPreference(prefsKey, CFSTR("ignore"));
					break;
				
				case 2: // kAlert_NotifyDisplayIconAndDiamondMark
					setMacTelnetPreference(prefsKey, CFSTR("animate"));
					break;
				
				case 3: // kAlert_NotifyAlsoDisplayAlert
					setMacTelnetPreference(prefsKey, CFSTR("alert"));
					break;
				
				case 1: // kAlert_NotifyDisplayDiamondMark
				default:
					setMacTelnetPreference(prefsKey, CFSTR("badge"));
					break;
				}
				
				prefsKey = CFSTR("window-terminal-position-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, appPrefs.windowStackingOriginLeft,
														appPrefs.windowStackingOriginTop);
				
				prefsKey = CFSTR("new-means");
				switch (appPrefs.newCommandShortcutEffect)
				{
				default:
					setMacTelnetPreference(prefsKey, CFSTR("dialog"));
					break;
				}
			}
			
			// read window preferences resource (there should be only one of these);
			// the resource types and IDs here must match what was used in the original file;
			// so no fancy constants, etc. are used here, these values simply must not change
			UseResFile(gOldPrefsFileRefNum);
			error = loadPreferencesStructure(&windowPrefs, sizeof(windowPrefs), 'WPRF'/* type */, 128/* ID */);
			if (noErr == error)
			{
				CFStringRef		prefsKey = nullptr;
				
				
				//
				// convert settings to their CFPreferences equivalents
				//
				
				prefsKey = CFSTR("window-macroeditor-position-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.macroSetupDialog.location.x,
														windowPrefs.macroSetupDialog.location.y);
				
				prefsKey = CFSTR("window-macroeditor-size-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.macroSetupDialog.size.width,
														windowPrefs.macroSetupDialog.size.height);
				
				prefsKey = CFSTR("window-commandline-position-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.commandLineWindow.location.x,
														windowPrefs.commandLineWindow.location.y);
				
				prefsKey = CFSTR("window-commandline-size-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.macroSetupDialog.size.width,
														0/* height is not used */);
				
				prefsKey = CFSTR("window-vt220keys-position-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.keypadWindow.location.x,
														windowPrefs.keypadWindow.location.y);
				
				prefsKey = CFSTR("window-functionkeys-position-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.functionWindow.location.x,
														windowPrefs.functionWindow.location.y);
				
				prefsKey = CFSTR("window-controlkeys-position-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.controlKeysWindow.location.x,
														windowPrefs.controlKeysWindow.location.y);
				
				prefsKey = CFSTR("window-preferences-position-pixels");
				(Boolean)setMacTelnetCoordPreference(prefsKey, windowPrefs.preferencesWindow.location.x,
														windowPrefs.preferencesWindow.location.y);
			}
			
			//
			// Now iterate over collections (Terminal, Sessions) and
			// convert each one into a new preferences domain; newer
			// versions of MacTelnet save the name of a collection in
			// the main MacTelnet preferences file for two purposes:
			// to support Unicode names, and to have a map between the
			// name and its symbolic value (used to look up its
			// preference domain).
			//
			
			// read Terminals
			UseResFile(gOldPrefsFileRefNum);
			collectionCount = Count1Resources('TPRF');
			if (collectionCount > 0)
			{
				// terminal data is now split into 2 types of collections;
				// specifically, font, size and color are now in a format
				// (the rest of the terminal data is as before)
				CFMutableArrayRef	terminalsArray = nullptr;
				CFMutableArrayRef	stylesArray = nullptr;
				
				
				terminalsArray = CFArrayCreateMutable(kCFAllocatorDefault, collectionCount/* maximum length */,
														&kCFTypeArrayCallBacks);
				stylesArray = CFArrayCreateMutable(kCFAllocatorDefault, collectionCount/* maximum length */,
													&kCFTypeArrayCallBacks);
				for (i = 0; i < collectionCount; ++i)
				{
					UseResFile(gOldPrefsFileRefNum);
					handle = Get1IndResource('TPRF', i + 1/* one-based index */);
					error = ResError();
					if (nullptr != handle)
					{
						TerminalPrefsPtr	dataPtr = REINTERPRET_CAST(*handle, TerminalPrefsPtr);
						
						
						if (nullptr != dataPtr)
						{
							SInt16		resID = 0;
							ResType		resType = '----';
							Str255		resName;
							
							
							GetResInfo(handle, &resID, &resType, resName);
							error = ResError();
							if (noErr == error)
							{
								CFKeyValueInterface*	terminalKeyValueMgrPtr = nullptr;
								CFKeyValueInterface*	formatKeyValueMgrPtr = nullptr;
								
								
								// if these are the Default preferences, store them directly in CFPreferences
								if (0 == PLstrcmp(resName, "\pDefault"))
								{
									terminalKeyValueMgrPtr = new CFKeyValuePreferences(kMacTelnetApplicationID);
									formatKeyValueMgrPtr = new CFKeyValuePreferences(kMacTelnetApplicationID);
								}
								else
								{
									// a non-default collection; create a favorite dictionary,
									// and add a Unicode-friendly name to it
									CFStringRef		resNameCFString = CFStringCreateWithPascalString
																		(kCFAllocatorDefault, resName,
																			kCFStringEncodingMacRoman);
									
									
									if (nullptr != resNameCFString)
									{
										CFDataRef	resNameCFData = CFStringCreateExternalRepresentation
																	(kCFAllocatorDefault, resNameCFString,
																		kCFStringEncodingUnicode/* must match what MacTelnet uses */,
																		'\?'/* loss byte */);
										
										
										if (nullptr != resNameCFData)
										{
											CFMutableDictionaryRef		infoDictionary = nullptr;
											
											
											// create entry for terminal data (minus fonts, now)
											infoDictionary = CFDictionaryCreateMutable
																(kCFAllocatorDefault, 0/* maximum capacity; 0 = infinite */,
																	&kCFTypeDictionaryKeyCallBacks,
																	&kCFTypeDictionaryValueCallBacks);
											if (nullptr != infoDictionary)
											{
												// creating a wrapper automatically retains the original
												// dictionary, so the local reference can be released
												terminalKeyValueMgrPtr = new CFKeyValueDictionary(infoDictionary);
												if (nullptr != terminalKeyValueMgrPtr)
												{
													// attach a Unicode friendly name (and a regular string, just
													// in case the Unicode one is messed up); this automatically
													// retains the data and string, so the local one can be released
													terminalKeyValueMgrPtr->addData(CFSTR("name"), resNameCFData);
													terminalKeyValueMgrPtr->addString(CFSTR("name-string"), resNameCFString);
												}
												
												if (nullptr != terminalsArray)
												{
													CFArrayAppendValue(terminalsArray, infoDictionary);
												}
												else
												{
													// error, unable to include settings in favorites list!
												}
												
												CFRelease(infoDictionary), infoDictionary = nullptr;
											}
											
											// create entry for font and color data
											infoDictionary = CFDictionaryCreateMutable
																(kCFAllocatorDefault, 0/* maximum capacity; 0 = infinite */,
																	&kCFTypeDictionaryKeyCallBacks,
																	&kCFTypeDictionaryValueCallBacks);
											if (nullptr != infoDictionary)
											{
												// creating a wrapper automatically retains the original
												// dictionary, so the local reference can be released
												formatKeyValueMgrPtr = new CFKeyValueDictionary(infoDictionary);
												if (nullptr != formatKeyValueMgrPtr)
												{
													// attach a Unicode friendly name (and a regular string, just
													// in case the Unicode one is messed up); this automatically
													// retains the data and string, so the local one can be released
													formatKeyValueMgrPtr->addData(CFSTR("name"), resNameCFData);
													formatKeyValueMgrPtr->addString(CFSTR("name-string"), resNameCFString);
												}
												
												if (nullptr != stylesArray)
												{
													CFArrayAppendValue(stylesArray, infoDictionary);
												}
												else
												{
													// error, unable to include settings in favorites list!
												}
												
												CFRelease(infoDictionary), infoDictionary = nullptr;
											}
											
											// no longer needed locally
											CFRelease(resNameCFData), resNameCFData = nullptr;
										}
										
										// no longer needed locally
										CFRelease(resNameCFString), resNameCFString = nullptr;
									}
								}
								
								if (nullptr != terminalKeyValueMgrPtr)
								{
									//
									// convert terminal data from the old structure;
									// the polymorphism of the key value manager causes
									// these settings to automatically go either directly
									// into MacTelnet-domain Core Foundation preferences,
									// or a sub-dictionary that is stored in favorites
									//
									
									switch (dataPtr->emulation)
									{
									// these numbers must match values actually saved
									// by older versions of MacTelnet
									case 256:
										terminalKeyValueMgrPtr->addString(CFSTR("terminal-emulator-type"), CFSTR("dumb"));
										break;
									
									case 2:
										terminalKeyValueMgrPtr->addString(CFSTR("terminal-emulator-type"), CFSTR("vt220"));
										break;
									
									case 0:
									default:
										terminalKeyValueMgrPtr->addString(CFSTR("terminal-emulator-type"), CFSTR("vt100"));
										break;
									}
									
									terminalKeyValueMgrPtr->addInteger(CFSTR("terminal-screen-dimensions-columns"), dataPtr->columnCount);
									terminalKeyValueMgrPtr->addInteger(CFSTR("terminal-screen-dimensions-rows"), dataPtr->rowCount);
									terminalKeyValueMgrPtr->addInteger(CFSTR("terminal-scrollback-size-lines"), dataPtr->scrollbackBufferSize);
									
									terminalKeyValueMgrPtr->addFlag(CFSTR("terminal-scrollback-enabled"),
																	(dataPtr->scrollbackBufferSize > 0));
									terminalKeyValueMgrPtr->addFlag(CFSTR("terminal-emulator-xterm-enable-color"),
																	(0 != dataPtr->usesANSIColors));
									terminalKeyValueMgrPtr->addFlag(CFSTR("terminal-emulator-xterm-enable-graphics"),
																	(0 != dataPtr->usesANSIColors/* currently graphics and color share a flag */));
									terminalKeyValueMgrPtr->addFlag(CFSTR("terminal-emulator-xterm-enable-window-alteration-sequences"),
																	(0 != dataPtr->usesXTermSequences));
									terminalKeyValueMgrPtr->addFlag(CFSTR("terminal-line-wrap"),
																	(0 != dataPtr->usesVTWrap));
									terminalKeyValueMgrPtr->addFlag(CFSTR("terminal-clear-saves-lines"),
																	(0 != dataPtr->usesVTWrap));
									
									switch (dataPtr->metaKey)
									{
									// these numbers must match values actually saved
									// by older versions of MacTelnet
									case 2:
										terminalKeyValueMgrPtr->addString(CFSTR("key-map-emacs-meta"), CFSTR("option"));
										break;
									
									case 1:
										terminalKeyValueMgrPtr->addString(CFSTR("key-map-emacs-meta"), CFSTR("control+command"));
										break;
									
									case 0:
									default:
										terminalKeyValueMgrPtr->addString(CFSTR("key-map-emacs-meta"), CFSTR(""));
										break;
									}
									
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-emacs-move-down"),
																		(0 != dataPtr->usesEmacsArrows)
																		? CFSTR("down-arrow") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-emacs-move-left"),
																		(0 != dataPtr->usesEmacsArrows)
																		? CFSTR("left-arrow") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-emacs-move-right"),
																		(0 != dataPtr->usesEmacsArrows)
																		? CFSTR("right-arrow") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-emacs-move-up"),
																		(0 != dataPtr->usesEmacsArrows)
																		? CFSTR("up-arrow") : CFSTR(""));
									
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-terminal-end"),
																		(0 != dataPtr->mapsPageJumpKeys)
																		? CFSTR("end") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-terminal-home"),
																		(0 != dataPtr->mapsPageJumpKeys)
																		? CFSTR("home") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-terminal-page-down"),
																		(0 != dataPtr->mapsPageJumpKeys)
																		? CFSTR("page-down") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-terminal-page-up"),
																		(0 != dataPtr->mapsPageJumpKeys)
																		? CFSTR("page-up") : CFSTR(""));
									
									terminalKeyValueMgrPtr->addFlag(CFSTR("data-receive-do-not-strip-high-bit"),
																	(0 != dataPtr->usesEightBits));
									terminalKeyValueMgrPtr->addFlag(CFSTR("terminal-clear-saves-lines"),
																	(0 != dataPtr->savesOnClear));
									
									{
										CFStringRef		answerBackCFString = CFStringCreateWithPascalString
																				(kCFAllocatorDefault,
																					dataPtr->answerBackMessage,
																					kCFStringEncodingMacRoman);
										
										
										if (nullptr != answerBackCFString)
										{
											terminalKeyValueMgrPtr->addString(CFSTR("terminal-emulator-answerback"),
																				answerBackCFString);
											CFRelease(answerBackCFString), answerBackCFString = nullptr;
										}
									}
									
									// NOT remapping the keypad top row means that these keys send VT220 sequences
									// (for VT220 terminals)
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-vt220-pf1"),
																		(0 == dataPtr->remapKeypad)
																		? CFSTR("keypad-clear") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-vt220-pf2"),
																		(0 == dataPtr->remapKeypad)
																		? CFSTR("keypad-=") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-vt220-pf3"),
																		(0 == dataPtr->remapKeypad)
																		? CFSTR("keypad-/") : CFSTR(""));
									terminalKeyValueMgrPtr->addString(CFSTR("command-key-vt220-pf4"),
																		(0 == dataPtr->remapKeypad)
																		? CFSTR("keypad-*") : CFSTR(""));
								}
								
								if (nullptr != formatKeyValueMgrPtr)
								{
									//
									// convert terminal data from the old structure;
									// the polymorphism of the key value manager causes
									// these settings to automatically go either directly
									// into MacTelnet-domain Core Foundation preferences,
									// or a sub-dictionary that is stored in favorites
									//
									
									{
										CFArrayRef		colorArray = nullptr;
										
										
										if (convertRGBColorToCFArray(&dataPtr->foregroundNormalColor, colorArray))
										{
											formatKeyValueMgrPtr->addArray(CFSTR("terminal-color-normal-foreground-rgb"),
																			colorArray);
											CFRelease(colorArray), colorArray = nullptr;
										}
										if (convertRGBColorToCFArray(&dataPtr->backgroundNormalColor, colorArray))
										{
											formatKeyValueMgrPtr->addArray(CFSTR("terminal-color-normal-background-rgb"),
																			colorArray);
											CFRelease(colorArray), colorArray = nullptr;
										}
										if (convertRGBColorToCFArray(&dataPtr->foregroundBlinkingColor, colorArray))
										{
											formatKeyValueMgrPtr->addArray(CFSTR("terminal-color-blinking-foreground-rgb"),
																			colorArray);
											CFRelease(colorArray), colorArray = nullptr;
										}
										if (convertRGBColorToCFArray(&dataPtr->backgroundBlinkingColor, colorArray))
										{
											formatKeyValueMgrPtr->addArray(CFSTR("terminal-color-blinking-background-rgb"),
																			colorArray);
											CFRelease(colorArray), colorArray = nullptr;
										}
										if (convertRGBColorToCFArray(&dataPtr->foregroundBoldColor, colorArray))
										{
											formatKeyValueMgrPtr->addArray(CFSTR("terminal-color-bold-foreground-rgb"),
																			colorArray);
											CFRelease(colorArray), colorArray = nullptr;
										}
										if (convertRGBColorToCFArray(&dataPtr->backgroundBoldColor, colorArray))
										{
											formatKeyValueMgrPtr->addArray(CFSTR("terminal-color-bold-background-rgb"),
																			colorArray);
											CFRelease(colorArray), colorArray = nullptr;
										}
									}
									
									formatKeyValueMgrPtr->addInteger(CFSTR("terminal-font-size-points"), dataPtr->fontSize);
									
									{
										CFStringRef		fontCFString = CFStringCreateWithPascalString
																		(kCFAllocatorDefault,
																			dataPtr->normalFont,
																			kCFStringEncodingMacRoman);
										
										
										if (nullptr != fontCFString)
										{
											formatKeyValueMgrPtr->addString(CFSTR("terminal-font-family"), fontCFString);
											CFRelease(fontCFString), fontCFString = nullptr;
										}
									}
								}
								
								if (nullptr != terminalKeyValueMgrPtr)
								{
									delete terminalKeyValueMgrPtr, terminalKeyValueMgrPtr = nullptr;
								}
								
								if (nullptr != formatKeyValueMgrPtr)
								{
									delete formatKeyValueMgrPtr, formatKeyValueMgrPtr = nullptr;
								}
							}
						}
						
						// old resource no longer needed
						ReleaseResource(handle), handle = nullptr;
					}
				}
				
				if (nullptr != terminalsArray)
				{
					setMacTelnetPreference(CFSTR("favorite-terminals"), terminalsArray);
					CFRelease(terminalsArray), terminalsArray = nullptr;
				}
				
				if (nullptr != stylesArray)
				{
					setMacTelnetPreference(CFSTR("favorite-styles"), stylesArray);
					CFRelease(stylesArray), stylesArray = nullptr;
				}
			}
			
			// read Sessions
			UseResFile(gOldPrefsFileRefNum);
			collectionCount = Count1Resources('SPRF');
			if (collectionCount > 0)
			{
				CFMutableArrayRef	sessionsArray = nullptr;
				
				
				sessionsArray = CFArrayCreateMutable(kCFAllocatorDefault, collectionCount/* maximum length */,
														&kCFTypeArrayCallBacks);
				for (i = 0; i < collectionCount; ++i)
				{
					UseResFile(gOldPrefsFileRefNum);
					handle = Get1IndResource('SPRF', i + 1/* one-based index */);
					error = ResError();
					if (nullptr != handle)
					{
						SessionPrefsPtr		dataPtr = REINTERPRET_CAST(*handle, SessionPrefsPtr);
						
						
						if (nullptr != dataPtr)
						{
							SInt16		resID = 0;
							ResType		resType = '----';
							Str255		resName;
							
							
							GetResInfo(handle, &resID, &resType, resName);
							error = ResError();
							if (noErr == error)
							{
								CFKeyValueInterface*	sessionKeyValueMgrPtr = nullptr;
								
								
								// if these are the Default preferences, store them directly in CFPreferences
								if (0 == PLstrcmp(resName, "\pDefault"))
								{
									sessionKeyValueMgrPtr = new CFKeyValuePreferences(kMacTelnetApplicationID);
								}
								else
								{
									// a non-default collection; create a favorite dictionary,
									// and add a Unicode-friendly name to it
									CFStringRef		resNameCFString = CFStringCreateWithPascalString
																		(kCFAllocatorDefault, resName,
																			kCFStringEncodingMacRoman);
									
									
									if (nullptr != resNameCFString)
									{
										CFDataRef	resNameCFData = CFStringCreateExternalRepresentation
																	(kCFAllocatorDefault, resNameCFString,
																		kCFStringEncodingUnicode/* must match what MacTelnet uses */,
																		'\?'/* loss byte */);
										
										
										if (nullptr != resNameCFData)
										{
											CFMutableDictionaryRef	infoDictionary = nullptr;
											
											
											// create entry for session data
											infoDictionary = CFDictionaryCreateMutable
																(kCFAllocatorDefault, 0/* maximum capacity; 0 = infinite */,
																	&kCFTypeDictionaryKeyCallBacks,
																	&kCFTypeDictionaryValueCallBacks);
											if (nullptr != infoDictionary)
											{
												// creating a wrapper automatically retains the original
												// dictionary, so the local reference can be released
												sessionKeyValueMgrPtr = new CFKeyValueDictionary(infoDictionary);
												if (nullptr != sessionKeyValueMgrPtr)
												{
													// attach a Unicode friendly name (and a regular string, just
													// in case the Unicode one is messed up); this automatically
													// retains the data and string, so the local one can be released
													sessionKeyValueMgrPtr->addData(CFSTR("name"), resNameCFData);
													sessionKeyValueMgrPtr->addString(CFSTR("name-string"), resNameCFString);
												}
												
												if (nullptr != sessionsArray)
												{
													CFArrayAppendValue(sessionsArray, infoDictionary);
												}
												else
												{
													// error, unable to include settings in favorites list!
												}
												
												CFRelease(infoDictionary), infoDictionary = nullptr;
											}
											
											// no longer needed locally
											CFRelease(resNameCFData), resNameCFData = nullptr;
										}
										
										// no longer needed locally
										CFRelease(resNameCFString), resNameCFString = nullptr;
									}
								}
								
								if (nullptr != sessionKeyValueMgrPtr)
								{
									// this information was implied in older MacTelnet versions
									sessionKeyValueMgrPtr->addString(CFSTR("server-protocol"), CFSTR("telnet"));
									
									sessionKeyValueMgrPtr->addInteger(CFSTR("server-port"), dataPtr->port);
									
									switch (dataPtr->modeForTEK)
									{
									// these numbers must match values actually saved
									// by older versions of MacTelnet
									case 1:
										sessionKeyValueMgrPtr->addString(CFSTR("tek-mode"), CFSTR("4105"));
										break;
									
									case 0:
										sessionKeyValueMgrPtr->addString(CFSTR("tek-mode"), CFSTR("4014"));
										break;
									
									case -1:
									default:
										sessionKeyValueMgrPtr->addString(CFSTR("tek-mode"), CFSTR("off"));
										break;
									}
									
									switch (dataPtr->pasteMethod)
									{
									// these numbers must match values actually saved
									// by older versions of MacTelnet
									case 1:
										sessionKeyValueMgrPtr->addString(CFSTR("data-send-paste-method"), CFSTR("throttled"));
										break;
									
									case 0:
									default:
										sessionKeyValueMgrPtr->addString(CFSTR("data-send-paste-method"), CFSTR("normal"));
										break;
									}
									
									sessionKeyValueMgrPtr->addInteger(CFSTR("data-send-paste-block-size-bytes"), dataPtr->pasteBlockSize);
									sessionKeyValueMgrPtr->addString(CFSTR("key-map-new-line"),
																		(0 != dataPtr->mapCR) ? CFSTR("\\015\\000") : CFSTR("\\015\\012"));
									sessionKeyValueMgrPtr->addFlag(CFSTR("line-mode-enabled"), (0 != dataPtr->lineMode));
									sessionKeyValueMgrPtr->addFlag(CFSTR("tek-page-clears-screen"), (0 != dataPtr->tekPageClears));
									sessionKeyValueMgrPtr->addFlag(CFSTR("data-send-local-echo-half-duplex"), (0 != dataPtr->halfDuplex));
									sessionKeyValueMgrPtr->addString(CFSTR("key-map-delete"),
																		(0 != dataPtr->deleteMapping) ? CFSTR("delete") : CFSTR("backspace"));
									
									{
										UniChar			charArray[2];
										CFStringRef		charCFString = nullptr;
										
										
										charArray[0] = '^';
										
										charArray[1] = dataPtr->interruptKey + '@'; // convert to visible character
										charCFString = CFStringCreateWithCharacters
														(kCFAllocatorDefault, charArray, sizeof(charArray) / sizeof(UniChar));
										if (nullptr != charCFString)
										{
											sessionKeyValueMgrPtr->addString(CFSTR("command-key-interrupt-process"), charCFString);
											CFRelease(charCFString), charCFString = nullptr;
										}
										charArray[1] = dataPtr->suspendKey + '@'; // convert to visible character
										charCFString = CFStringCreateWithCharacters
														(kCFAllocatorDefault, charArray, sizeof(charArray) / sizeof(UniChar));
										if (nullptr != charCFString)
										{
											sessionKeyValueMgrPtr->addString(CFSTR("command-key-suspend-output"), charCFString);
											CFRelease(charCFString), charCFString = nullptr;
										}
										charArray[1] = dataPtr->resumeKey + '@'; // convert to visible character
										charCFString = CFStringCreateWithCharacters
														(kCFAllocatorDefault, charArray, sizeof(charArray) / sizeof(UniChar));
										if (nullptr != charCFString)
										{
											sessionKeyValueMgrPtr->addString(CFSTR("command-key-resume-output"), charCFString);
											CFRelease(charCFString), charCFString = nullptr;
										}
									}
									
									{
										CFStringRef		nameCFString = nullptr;
										
										
										nameCFString = CFStringCreateWithPascalString
														(kCFAllocatorDefault, dataPtr->terminalEmulationName,
															kCFStringEncodingMacRoman);
										if (nullptr != nameCFString)
										{
											sessionKeyValueMgrPtr->addString(CFSTR("terminal-favorite"), nameCFString);
											CFRelease(nameCFString), nameCFString = nullptr;
										}
										
										nameCFString = CFStringCreateWithPascalString
														(kCFAllocatorDefault, dataPtr->translationTableName,
															kCFStringEncodingMacRoman);
										if (nullptr != nameCFString)
										{
											sessionKeyValueMgrPtr->addString(CFSTR("terminal-text-translation-table"), nameCFString);
											CFRelease(nameCFString), nameCFString = nullptr;
										}
										
										nameCFString = CFStringCreateWithPascalString
														(kCFAllocatorDefault, dataPtr->hostName,
															kCFStringEncodingMacRoman);
										if (nullptr != nameCFString)
										{
											sessionKeyValueMgrPtr->addString(CFSTR("server-host"), nameCFString);
											CFRelease(nameCFString), nameCFString = nullptr;
										}
									}
									
									sessionKeyValueMgrPtr->addFlag(CFSTR("data-send-local-echo-enabled"), (0 != dataPtr->halfDuplex));
									
									sessionKeyValueMgrPtr->addFlag(CFSTR("terminal-capture-auto-start"), (0 != dataPtr->autoCaptureToFile));
									
									sessionKeyValueMgrPtr->addInteger(CFSTR("data-receive-buffer-size-bytes"), dataPtr->netBlockSize);
									
									// !!! INCOMPLETE !!!
								}
								
								if (nullptr != sessionKeyValueMgrPtr)
								{
									delete sessionKeyValueMgrPtr, sessionKeyValueMgrPtr = nullptr;
								}
							}
						}
						
						// old resource no longer needed
						ReleaseResource(handle), handle = nullptr;
					}
				}
				
				if (nullptr != sessionsArray)
				{
					setMacTelnetPreference(CFSTR("favorite-sessions"), sessionsArray);
					CFRelease(sessionsArray), sessionsArray = nullptr;
				}
			}
			
			// save XML
			unless (CFPreferencesAppSynchronize(kMacTelnetApplicationID)) result = kMy_PrefsResultGenericFailure;
		}
	}
	
	return result;
}// actionLegacyUpdates


/*!
Upgrades from version 2 to version 3.  Some keys are
now obsolete that were never available to users, so
they are deleted.

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
Adds an error to the reply record; at least the
error code is provided, and optionally you can
accompany it with an equivalent message.

You should use this to return error codes from
ALL Apple Events; if you do, return "noErr" from
the handler but potentially define "inError" as
a specific error code.

(3.1)
*/
OSStatus
addErrorToReply		(ConstStringPtr		inErrorMessageOrNull,
					 OSStatus			inError,
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
		
		// if provided, also put the error message
		if (nullptr != inErrorMessageOrNull)
		{
			result = AEPutParamPtr(inoutReplyAppleEventPtr, keyErrorString, typeChar,
									inErrorMessageOrNull + 1/* skip length byte */,
									STATIC_CAST(PLstrlen(inErrorMessageOrNull) * sizeof(UInt8), Size));
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
This routine can be used to determine the directory
ID of a directory described by a file system
specification record.

(3.1)
*/
long
getDirectoryIDFromFSSpec	(FSSpec const*		inFSSpecPtr)
{
	StrFileName		name;
	CInfoPBRec		thePB;
	long			result = 0L;
	
	
	PLstrcpy(name, inFSSpecPtr->name);
	thePB.dirInfo.ioCompletion = 0L;
	thePB.dirInfo.ioNamePtr = name;
	thePB.dirInfo.ioVRefNum = inFSSpecPtr->vRefNum;
	thePB.dirInfo.ioFDirIndex = 0;
	thePB.dirInfo.ioDrDirID = inFSSpecPtr->parID;
	
	if (noErr == PBGetCatInfoSync(&thePB)) result = thePB.dirInfo.ioDrDirID;
	
	return result;
}// getDirectoryIDFromFSSpec


/*!
Fills in a file system specification record with
information for a particular folder.  If the folder
doesn’t exist, it is created.

The name information for the resultant file
specification record is left blank, so that you may
fill in any name you wish.  That way, you can either
use FSMakeFSSpec() to obtain a file specification
for a file contained in the requested folder, or you
can leave the name blank, and FSMakeFSSpec() will
fill in the name of the folder and adjust the parent
directory ID appropriately.

If no problems occur, "noErr" is returned.  If the
given folder type is not recognized as one of the
valid types, "invalidFolderTypeErr" is returned.

(3.1)
*/
OSStatus
getFolderFSSpec		(My_MacTelnetFolder		inFolderType,
					 FSSpec*				outFolderFSSpecPtr)
{
	OSStatus		result = noErr;
	
	
	if (nullptr == outFolderFSSpecPtr) result = memPCErr;
	else
	{
		switch (inFolderType)
		{
		case kMy_MacTelnetFolderFavorites: // the “Favorites” folder inside the MacTelnet preferences folder
			result = getFolderFSSpec(kMy_MacTelnetFolderPreferences, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavorites, outFolderFSSpecPtr);
				
				// if no Favorites folder exists in MacTelnet’s preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderPreferences: // the “MacTelnet Preferences” folder inside the system “Preferences” folder
			result = getFolderFSSpec(kMy_MacTelnetFolderMacPreferences, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationPreferences, outFolderFSSpecPtr);
				
				// if no MacTelnet Preferences folder exists in the current user’s Preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderScriptsMenuItems: // the MacTelnet “Scripts Menu Items” folder, in MacTelnet’s folder
			result = getFolderFSSpec(kMy_MacTelnetFolderPreferences, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationScriptsMenuItems, outFolderFSSpecPtr);
				
				// if no Scripts Menu Items folder exists in the current user’s Preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderStartupItems: // the MacTelnet “Startup Items” folder, in MacTelnet’s preferences folder
			result = getFolderFSSpec(kMy_MacTelnetFolderPreferences, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationStartupItems, outFolderFSSpecPtr);
				
				// if no Startup Items folder exists in the MacTelnet Preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderUserMacroFavorites: // the “Macro Sets” folder inside MacTelnet’s Favorites folder (in preferences)
			result = getFolderFSSpec(kMy_MacTelnetFolderFavorites, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesMacros, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderUserProxyFavorites: // the “Proxies” folder inside MacTelnet’s Favorites folder (in preferences)
			result = getFolderFSSpec(kMy_MacTelnetFolderFavorites, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesProxies, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderUserSessionFavorites: // the “Sessions” folder inside MacTelnet’s Favorites folder (in preferences)
			result = getFolderFSSpec(kMy_MacTelnetFolderFavorites, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesSessions, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderUserTerminalFavorites: // the “Terminals” folder inside MacTelnet’s Favorites folder (in preferences)
			result = getFolderFSSpec(kMy_MacTelnetFolderFavorites, outFolderFSSpecPtr);
			if (noErr == result)
			{
				long	unusedDirID = 0L;
				
				
				result = makeLocalizedFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesTerminals, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the “parent” directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kMy_MacTelnetFolderMacPreferences: // the Mac OS “Preferences” folder for the current user
			result = FindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kMy_MacTelnetFolderMacTrash: // the Trash for the current user
			result = FindFolder(kUserDomain, kTrashFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		default:
			// ???
			result = invalidFolderTypeErr;
			break;
		}
	}
	
	return result;
}// getFolderFSSpec


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
Fills in a global structure from the current
resource file.

\retval noErr
if the load succeeds

\retval any Resource Manager error
if the load fails

(3.0)
*/
OSStatus
loadPreferencesStructure	(void*		outStructurePtr,
							 size_t		inStructureSize,
							 ResType	inResourceType,
							 SInt16		inResourceID)
{
	Handle		prefsResourceHandle = nullptr;
	OSStatus	result = noErr;
	
	
	prefsResourceHandle = Get1Resource(inResourceType, inResourceID);
	result = ResError();
	if (nullptr == prefsResourceHandle) result = resNotFound;
	else if (noErr == result)
	{
		std::memcpy(outStructurePtr, *prefsResourceHandle, inStructureSize);
		ReleaseResource(prefsResourceHandle);
	}
	
	return result;
}// loadPreferencesStructure


/*!
Locates the specified file or folder name and calls
FSMakeFSSpec() with the given volume and directory
ID information.  The result is an FSSpec with a
Pascal string copy of the given file name string
(unless FSMakeFSSpec() does any further munging).

IMPORTANT: This routine is ironically unfriendly to
           localization.  A future routine (probably
           named makeLocalizedFSRef()) will be able
		   to handle Unicode filenames.

\retval noErr
if the FSSpec is created successfully

\retval paramErr
if no file name with the given ID exists

\retval kTECNoConversionPathErr
if conversion is not possible (arbitrary return value,
used even without Text Encoding Converter)

\retval (other)
if an OS error occurred (note that "fnfErr" is
common and simply means that you are trying to
create a specification for a nonexistent file; that
may be what you are intending to do)

(3.1)
*/
OSStatus
makeLocalizedFSSpec		(SInt16					inVRefNum,
						 SInt32					inDirID,
						 My_FolderStringType	inWhichString,
						 FSSpec*				outFSSpecPtr)
{
	CFStringRef			nameCFString = nullptr;
	My_StringResult		stringResult = kMy_StringResultOK;
	OSStatus			result = noErr;
	
	
	stringResult = copyFileOrFolderCFString(inWhichString, &nameCFString);
	
	// if the string was obtained, call FSMakeFSSpec
	if (kMy_StringResultOK == stringResult)
	{
		Str255		name;
		
		
		if (CFStringGetPascalString(nameCFString, name, sizeof(name), kCFStringEncodingMacRoman/* presumed; but this always has been */))
		{
			// got Pascal string representation OK; so, attempt to create FSSpec!
			result = FSMakeFSSpec(inVRefNum, inDirID, name, outFSSpecPtr);
		}
		else
		{
			// some kind of conversion error...
			result = kTECNoConversionPathErr;
		}
	}
	else
	{
		result = paramErr;
	}
	
	return result;
}// makeLocalizedFSSpec


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEOpenApplication".

(3.1)
*/
pascal OSErr
receiveApplicationOpen	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	// The preferences version MUST match what is used by the version of
	// MacTelnet that ships with this preferences converter; it should
	// be changed here if it is changed in MacTelnet.
	SInt16 const	kCurrentPrefsVersion = 4;
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
		
		
		// The preference key used here MUST match what MacTelnet reads in,
		// for obvious reasons.  Similarly, the application domain must match
		// what MacTelnet used FOR THE VERSION OF PREFERENCES DESIRED.  So if
		// MacTelnet’s identifier changes in the future, its old identifier
		// will still be needed to find the old version number.
		diskVersion = CFPreferencesGetAppIntegerValue
						(CFSTR("prefs-version"), kMacTelnetApplicationID,
							&existsAndIsValidFormat);
		unless (existsAndIsValidFormat)
		{
			diskVersion = 0;
		}
	}
	
	// based on the version of the preferences on disk (which is zero
	// if there is no version), decide what to do, and how to interact
	// with the user
	if (kCurrentPrefsVersion == diskVersion)
	{
		// preferences are identical, nothing to do; if the application
		// was launched automatically, then it should not have been
		std::cerr << "MacTelnet Preferences Converter: prefs-version on disk (" << diskVersion << ") is current, nothing to convert" << std::endl;
		doConvert = false;
	}
	else if (diskVersion > kCurrentPrefsVersion)
	{
		// preferences are newer, nothing to do; if the application
		// was launched automatically, then it should not have been
		std::cerr << "MacTelnet Preferences Converter: prefs-version on disk (" << diskVersion << ") is newer than this converter ("
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
			// Since a “lack of version” is considered version 0, this
			// converter must perform all upgrades required to move
			// legacy NCSA Telnet or MacTelnet 3.0 preferences into the
			// XML era.
			actionResult = actionLegacyUpdates();
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
		// current preferences version; MacTelnet will read that setting
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
				CFPreferencesSetAppValue(CFSTR("prefs-version"), numberRef, kMacTelnetApplicationID);
				(Boolean)CFPreferencesAppSynchronize(kMacTelnetApplicationID);
				CFRelease(numberRef), numberRef = nullptr;
			}
		}
	#endif
		
		// This is probably not something the user really needs to see.
	#if 1
		std::cerr << "MacTelnet Preferences Converter: conversion successful, new prefs-version is " << kCurrentPrefsVersion << std::endl;
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
							(CFSTR("This version of MacTelnet will now be able to read your existing preferences."),
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
									(CFSTR("This version of MacTelnet cannot read your existing preferences.  Normally, old preferences are converted automatically, but this has failed for some reason."),
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
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationOpen


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEReopenApplication".

(3.1)
*/
pascal OSErr
receiveApplicationReopen	(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr,
							 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationReopen


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEShowPreferences".

(3.1)
*/
pascal OSErr
receiveApplicationPrefs		(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr,
							 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationPrefs


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEQuitApplication".

(3.1)
*/
pascal OSErr
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
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationQuit


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEOpenDocuments".

(3.1)
*/
pascal OSErr
receiveOpenDocuments	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveOpenDocuments


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEPrintDocuments".

(3.1)
*/
pascal OSErr
receivePrintDocuments	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receivePrintDocuments


/*!
Like setMacTelnetPreference(), but is useful for creating
coordinate values.  The work of creating a CFArrayRef
containing two CFNumberRefs is done for you.

Returns "true" only if it was possible to create the
necessary Core Foundation types.

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
Updates MacTelnet’s XML preferences as requested.
Also prints to standard output information about
the data being written (if debugging is enabled).

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
Transforms a folder specification so that the
“file name” of the folder is empty, and the
“parent ID” is the ID of the folder itself.
This is very useful, since it then becomes
easy to create file specifications for files
*inside* the folder (and yet the folder
specification itself remains a valid way to
refer to the folder).

(3.1)
*/
void
transformFolderFSSpec		(FSSpec*	inoutFolderFSSpecPtr)
{
	inoutFolderFSSpecPtr->parID = getDirectoryIDFromFSSpec(inoutFolderFSSpecPtr);
	PLstrcpy(inoutFolderFSSpecPtr->name, "\p");
}// transformFolderFSSpec

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
