/*###############################################################

	Folder.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// MacTelnet includes
#include "FileUtilities.h"
#include "Folder.h"
#include "UIStrings.h"



#pragma mark Internal Method Prototypes

static void		transformFolderFSSpec		(FSSpec*);



#pragma mark Public Methods

/*!
Fills in a new-style file system specification record with
information for a particular folder.  If the folder doesnÕt
exist, it is created.

The name information for the resultant file record is left
blank, so that you may fill in any name you wish.  That way,
you can either use FSMakeFSRef() to obtain a file specification
for a file contained in the requested folder, or you can leave
the name blank, and FSMakeFSRef() will fill in the name of the
folder and adjust the parent directory ID appropriately.

If no problems occur, "noErr" is returned.  If the given folder
type is not recognized as one of the valid types,
"invalidFolderTypeErr" is returned.

(3.1)
*/
OSStatus
Folder_GetFSRef		(Folder_Ref		inFolderType,
					 FSRef&			outFolderFSRef)
{
	OSStatus	result = noErr;
	FSRef		parentFolderRef;
	
	
	switch (inFolderType)
	{
	case kFolder_RefApplicationSupport: // the ÒMacTelnetÓ folder inside the Application Support folder
		result = Folder_GetFSRef(kFolder_RefMacApplicationSupport, parentFolderRef);
		if (noErr == result)
		{
			CFStringRef		supportFolderNameCFString = nullptr;
			
			
			Localization_GetCurrentApplicationNameAsCFString(&supportFolderNameCFString);
			if (nullptr != supportFolderNameCFString)
			{
				UniCharCount const	kBufferSize = CFStringGetLength(supportFolderNameCFString);
				UniChar*			buffer = new UniChar[kBufferSize];
				UInt32				unusedDirID = 0L;
				
				
				CFStringGetCharacters(supportFolderNameCFString, CFRangeMake(0, kBufferSize), buffer);
				
				// if no MacTelnet folder exists in the Application Support folder, create it
				result = FSMakeFSRefUnicode(&parentFolderRef, kBufferSize, buffer,
											CFStringGetSmallestEncoding(supportFolderNameCFString),
											&outFolderFSRef);
				if (noErr != result)
				{
					result = FSCreateDirectoryUnicode(&parentFolderRef, kBufferSize, buffer,
														kFSCatInfoNone, nullptr/* info to set */,
														&outFolderFSRef, nullptr/* old-style specification record */,
														&unusedDirID);
				}
				
				delete [] buffer, buffer = nullptr;
				CFRelease(supportFolderNameCFString), supportFolderNameCFString = nullptr;
			}
		}
		break;
	
	case kFolder_RefFavorites: // the ÒFavoritesÓ folder inside the MacTelnet preferences folder
		result = Folder_GetFSRef(kFolder_RefPreferences, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no Favorites folder exists in MacTelnetÕs preferences folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationFavorites,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefPreferences: // the ÒMacTelnet PreferencesÓ folder inside the system ÒPreferencesÓ folder
		result = Folder_GetFSRef(kFolder_RefMacPreferences, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no MacTelnet Preferences folder exists in the current userÕs Preferences folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationPreferences,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefScriptsMenuItems: // the MacTelnet ÒScripts Menu ItemsÓ folder, in MacTelnetÕs preferences folder
		result = Folder_GetFSRef(kFolder_RefPreferences, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no Scripts Menu Items folder exists in the current userÕs Preferences folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationScriptsMenuItems,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefStartupItems: // the MacTelnet ÒStartup ItemsÓ folder, in MacTelnetÕs preferences folder
		result = Folder_GetFSRef(kFolder_RefPreferences, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no Startup Items folder exists in the MacTelnet Preferences folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationStartupItems,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefUserLogs: // the ÒLogsÓ folder inside the userÕs home directory
		result = Folder_GetFSRef(kFolder_RefMacLibrary, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no Logs folder exists in the Library folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameHomeLibraryLogs,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefUserMacroFavorites: // the ÒMacro SetsÓ folder inside MacTelnetÕs Favorites folder (in preferences)
		result = Folder_GetFSRef(kFolder_RefFavorites, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no folder exists in the Favorites folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationFavoritesMacros,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefRecentSessions: // the ÒRecent SessionsÓ folder inside MacTelnetÕs Application Support folder
		result = Folder_GetFSRef(kFolder_RefApplicationSupport, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no folder exists in the MacTelnet Preferences folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationRecentSessions,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefMacApplicationSupport: // the Mac OS ÒApplication SupportÓ folder for the current user
		result = FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacDesktop: // the Mac OS Desktop folder for the current user
		result = FSFindFolder(kUserDomain, kDesktopFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacFavorites: // the Mac OS ÒFavoritesÓ folder for the current user
		result = FSFindFolder(kUserDomain, kFavoritesFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacHelp: // the Mac OS ÒHelpÓ folder for the current user
		result = FSFindFolder(kUserDomain, kHelpFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacLibrary: // the Mac OS ÒLibraryÓ folder for the current user
		result = FSFindFolder(kUserDomain, kDomainLibraryFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacPreferences: // the Mac OS ÒPreferencesÓ folder for the current user
		result = FSFindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacTemporaryItems: // the invisible Mac OS ÒTemporary ItemsÓ folder
		result = FSFindFolder(kUserDomain, kTemporaryFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacTrash: // the Trash for the current user
		result = FSFindFolder(kUserDomain, kTrashFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	default:
		// ???
		result = invalidFolderTypeErr;
		break;
	}
	
	return result;
}// GetFSRef


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
	OSStatus	result = noErr;
	
	
	if (nullptr == outFolderFSSpecPtr) result = memPCErr;
	else
	{
		switch (inFolderType)
		{
		case kFolder_RefApplicationSupport: // the ÒMacTelnetÓ folder inside the Application Support folder
			result = Folder_GetFSSpec(kFolder_RefMacApplicationSupport, outFolderFSSpecPtr);
			if (noErr == result)
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
			if (noErr == result)
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
		
		case kFolder_RefPreferences: // the ÒMacTelnet PreferencesÓ folder inside the system ÒPreferencesÓ folder
			result = Folder_GetFSSpec(kFolder_RefMacPreferences, outFolderFSSpecPtr);
			if (noErr == result)
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
		
		case kFolder_RefScriptsMenuItems: // the MacTelnet ÒScripts Menu ItemsÓ folder, in MacTelnetÕs preferences folder
			result = Folder_GetFSSpec(kFolder_RefPreferences, outFolderFSSpecPtr);
			if (noErr == result)
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
			if (noErr == result)
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
		
		case kFolder_RefUserLogs: // the ÒLogsÓ folder inside the userÕs home directory
			// on Mac OS X, logs go into ~/Library/Logs
			result = Folder_GetFSSpec(kFolder_RefMacLibrary, outFolderFSSpecPtr);
			if (noErr == result)
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
			break;
		
		case kFolder_RefUserMacroFavorites: // the ÒMacro SetsÓ folder inside MacTelnetÕs Favorites folder (in preferences)
			result = Folder_GetFSSpec(kFolder_RefFavorites, outFolderFSSpecPtr);
			if (noErr == result)
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
		
		case kFolder_RefRecentSessions: // the ÒRecent SessionsÓ folder inside MacTelnetÕs Application Support folder
			result = Folder_GetFSSpec(kFolder_RefApplicationSupport, outFolderFSSpecPtr);
			if (noErr == result)
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
			result = FindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacDesktop: // the Mac OS Desktop folder for the current user
			result = FindFolder(kUserDomain, kDesktopFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacFavorites: // the Mac OS ÒFavoritesÓ folder for the current user
			result = FindFolder(kUserDomain, kFavoritesFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacHelp: // the Mac OS ÒHelpÓ folder for the current user
			result = FindFolder(kUserDomain, kHelpFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacLibrary: // the Mac OS ÒLibraryÓ folder for the current user
			result = FindFolder(kUserDomain, kDomainLibraryFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacPreferences: // the Mac OS ÒPreferencesÓ folder for the current user
			result = FindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacTemporaryItems: // the invisible Mac OS ÒTemporary ItemsÓ folder
			result = FindFolder(kUserDomain, kTemporaryFolderType, kCreateFolder,
								&(outFolderFSSpecPtr->vRefNum), &(outFolderFSSpecPtr->parID));
			outFolderFSSpecPtr->name[0] = 0;
			break;
		
		case kFolder_RefMacTrash: // the Trash for the current user
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
	inoutFolderFSSpecPtr->parID = FileUtilities_ReturnDirectoryIDFromFSSpec(inoutFolderFSSpecPtr);
	PLstrcpy(inoutFolderFSSpecPtr->name, EMPTY_PSTRING);
}// transformFolderFSSpec

// BELOW IS REQUIRED NEWLINE TO END FILE
