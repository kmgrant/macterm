/*###############################################################

	Folder.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#include "UniversalDefines.h"

// standard-C includes
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <Localization.h>
#include <MemoryBlocks.h>
#include <StringUtilities.h>

// resource includes
#include "StringResources.h"

// MacTelnet includes
#include "FileUtilities.h"
#include "Folder.h"
#include "UIStrings.h"



#pragma mark Constants

enum
{
#if TARGET_API_MAC_OS8
	// on Mac OS 8, everything is system-wide
	kMyUserSensitiveDomain = kOnSystemDisk
#else
	// on Mac OS X, many things are user-specific
	kMyUserSensitiveDomain = kUserDomain
#endif
};

#pragma mark Internal Method Prototypes

static void		transformFolderFSSpec		(FSSpec*);



#pragma mark Public Methods

/*!
Fills in a file system specification record with
information for a particular folder.  If the folder
doesnÕt exist, it is created.

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

(3.0)
*/
OSStatus
Folder_GetFSSpec	(Folder_Ref		inFolderType,
					 FSSpec*		outFolderFSSpecPtr)
{
	OSStatus		result = noErr;
	
	
	if (outFolderFSSpecPtr == nullptr) result = memPCErr;
	else
	{
		switch (inFolderType)
		{
		case kFolder_RefApplication: // the folder that ALWAYS contains MacTelnetÕs application (no matter where it ÒappearsÓ to be)
			{
				ProcessSerialNumber		telnetItself;
				
				
				result = GetCurrentProcess(&telnetItself);
				if (result == noErr)
				{
					ProcessInfoRec	info;
					FSSpec			applicationProgramFile;
					
					
					// clear the buffer to zeroes
					bzero(&info, sizeof(info));
					
					// GetProcessInformation() requires that the following
					// three fields be initialized ahead of time.
					info.processInfoLength = sizeof(info);
					info.processName = nullptr; // donÕt get the name
					info.processAppSpec = &applicationProgramFile;
					
					if (GetProcessInformation(&telnetItself, &info) == noErr)
					{
						result = FSMakeFSSpec(info.processAppSpec->vRefNum, info.processAppSpec->parID, EMPTY_PSTRING,
												outFolderFSSpecPtr);
						
						// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
						// (this makes the FSSpec much more useful, because it can then be used to place files *inside* a folder)
						transformFolderFSSpec(outFolderFSSpecPtr);
					}
				}
			}
			break;
		
		case kFolder_RefApplicationSupport: // the ÒMacTelnetÓ folder inside the Application Support folder
			result = Folder_GetFSSpec(kFolder_RefMacApplicationSupport, outFolderFSSpecPtr);
			if (result == noErr)
			{
				Str255		supportFolderName;
				long		unusedDirID = 0L;
				
				
				Localization_GetCurrentApplicationName(supportFolderName);
				result = FSMakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
										supportFolderName, outFolderFSSpecPtr);
				
				// if no MacTelnet folder exists in the Application Support folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefFavorites: // the ÒFavoritesÓ folder inside the MacTelnet preferences folder
			result = Folder_GetFSSpec(kFolder_RefPreferences, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavorites, outFolderFSSpecPtr);
				
				// if no Favorites folder exists in MacTelnetÕs preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefPackageParent: // the folder that Òlooks likeÓ it contains MacTelnetÕs application
		#if TARGET_API_MAC_OS8
			// in Mac OS 8 and 9, the package parent is the same as the application folder
			// because the application ÒisÓ the package
			result = Folder_GetFSSpec(kFolder_RefApplication, outFolderFSSpecPtr);
		#else
			// in Mac OS X, the package Òlooks likeÓ the application, but from a file
			// system standpoint, the application program file is actually three levels
			// down (the application is in the ÒContents/MacOSÓ folder, which in turn is
			// in the ÒMacTelnet.appÓ folder, the parent of which is the package parent)
			result = Folder_GetFSSpec(kFolder_RefApplication, outFolderFSSpecPtr);
			if (result == noErr)
			{
				(OSStatus)FSMakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
										outFolderFSSpecPtr->name, outFolderFSSpecPtr); // this gives the parent ID of the directory...
				(OSStatus)FSMakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
										EMPTY_PSTRING, outFolderFSSpecPtr); // this gives the parent ID of the parent directory...
				(OSStatus)FSMakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
										EMPTY_PSTRING, outFolderFSSpecPtr); // ...therefore, this gives the ID of the parentÕs parentÕs parent...
				(OSStatus)FSMakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
										EMPTY_PSTRING, outFolderFSSpecPtr); // ...and this gives the parent itself 
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
		#endif
			break;
		
		case kFolder_RefPreferences: // the ÒMacTelnet PreferencesÓ folder inside the system ÒPreferencesÓ folder
			result = Folder_GetFSSpec(kFolder_RefMacPreferences, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationPreferences, outFolderFSSpecPtr);
				
				// if no MacTelnet Preferences folder exists in the current userÕs Preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefScriptsMenuItems: // the MacTelnet ÒScripts Menu ItemsÓ folder, in MacTelnetÕs folder
			result = Folder_GetFSSpec(kFolder_RefPreferences, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationScriptsMenuItems, outFolderFSSpecPtr);
				
				// if no Scripts Menu Items folder exists in the current userÕs Preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefStartupItems: // the MacTelnet ÒStartup ItemsÓ folder, in MacTelnetÕs preferences folder
			result = Folder_GetFSSpec(kFolder_RefPreferences, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationStartupItems, outFolderFSSpecPtr);
				
				// if no Startup Items folder exists in the MacTelnet Preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefUserLogs: // the ÒLogsÓ folder inside the userÕs home directory (on Mac OS X), or the application folder
		#if TARGET_API_MAC_OS8
			// in Mac OS 8 and 9, the application folder contains logs by default
			result = Folder_GetFSSpec(kFolder_RefApplication, outFolderFSSpecPtr);
		#else
			// on Mac OS X, logs go into ~/Library/Logs
			result = Folder_GetFSSpec(kFolder_RefMacLibrary, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameHomeLibraryLogs, outFolderFSSpecPtr);
				
				// if no Logs folder exists in the Library folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
		#endif
			break;
		
		case kFolder_RefUserMacroFavorites: // the ÒMacro SetsÓ folder inside MacTelnetÕs Favorites folder (in preferences)
			result = Folder_GetFSSpec(kFolder_RefFavorites, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesMacros, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefUserProxyFavorites: // the ÒProxiesÓ folder inside MacTelnetÕs Favorites folder (in preferences)
			result = Folder_GetFSSpec(kFolder_RefFavorites, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesProxies, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefUserSessionFavorites: // the ÒSessionsÓ folder inside MacTelnetÕs Favorites folder (in preferences)
			result = Folder_GetFSSpec(kFolder_RefFavorites, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesSessions, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefUserTerminalFavorites: // the ÒTerminalsÓ folder inside MacTelnetÕs Favorites folder (in preferences)
			result = Folder_GetFSSpec(kFolder_RefFavorites, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationFavoritesTerminals, outFolderFSSpecPtr);
				
				// if no folder exists in the Favorites folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefRecentSessions: // the ÒRecent SessionsÓ folder inside MacTelnetÕs Application Support folder
			result = Folder_GetFSSpec(kFolder_RefApplicationSupport, outFolderFSSpecPtr);
			if (result == noErr)
			{
				long	unusedDirID = 0L;
				
				
				result = UIStrings_MakeFSSpec(outFolderFSSpecPtr->vRefNum, outFolderFSSpecPtr->parID,
												kUIStrings_FolderNameApplicationRecentSessions, outFolderFSSpecPtr);
				
				// if no folder exists in the MacTelnet Preferences folder, create it
				(OSStatus)FSpDirCreate(outFolderFSSpecPtr, GetScriptManagerVariable(smSysScript), &unusedDirID);
				
				// now reconstruct the FSSpec so the ÒparentÓ directory is the directory itself, and the name is blank
				// (this makes the FSSpec much more useful, because it can then be used to place files *in* a folder)
				transformFolderFSSpec(outFolderFSSpecPtr);
			}
			break;
		
		case kFolder_RefMacApplicationSupport: // the Mac OS ÒApplication SupportÓ folder for the current user
			result = FindFolder(kMyUserSensitiveDomain, kApplicationSupportFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacDesktop: // the Mac OS Desktop folder for the current user
			result = FindFolder(kMyUserSensitiveDomain, kDesktopFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacFavorites: // the Mac OS ÒFavoritesÓ folder for the current user
			result = FindFolder(kMyUserSensitiveDomain, kFavoritesFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacHelp: // the Mac OS ÒHelpÓ folder for the current user
			result = FindFolder(kMyUserSensitiveDomain, kHelpFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
	#if TARGET_API_MAC_CARBON
		case kFolder_RefMacLibrary: // the Mac OS ÒLibraryÓ folder for the current user (Mac OS X only)
			result = FindFolder(kMyUserSensitiveDomain, kDomainLibraryFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
	#endif
		
		case kFolder_RefMacPreferences: // the Mac OS ÒPreferencesÓ folder for the current user
			result = FindFolder(kMyUserSensitiveDomain, kPreferencesFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacSystem: // the Mac OS ÒSystem FolderÓ folder
			result = FindFolder(kOnSystemDisk, kSystemFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacTemporaryItems: // the invisible Mac OS ÒTemporary ItemsÓ folder
			result = FindFolder(kMyUserSensitiveDomain, kTemporaryFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacTrash: // the Trash for the current user
			result = FindFolder(kMyUserSensitiveDomain, kTrashFolderType, kCreateFolder,
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
}// GetFSSpec


#pragma mark Internal Methods

/*!
Transforms a folder specification so that the
Òfile nameÓ of the folder is empty, and the
Òparent IDÓ is the ID of the folder itself.
This is very useful, since it then becomes
easy to create file specifications for files
*inside* the folder (and yet the folder
specification itself remains a valid way to
refer to the folder).

(3.0)
*/
static void
transformFolderFSSpec		(FSSpec*	inoutFolderFSSpecPtr)
{
	inoutFolderFSSpecPtr->parID = FileUtilities_GetDirectoryIDFromFSSpec(inoutFolderFSSpecPtr);
	PLstrcpy(inoutFolderFSSpecPtr->name, EMPTY_PSTRING);
}// transformFolderFSSpec

// BELOW IS REQUIRED NEWLINE TO END FILE
