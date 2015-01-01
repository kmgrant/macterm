/*!	\file Folder.cp
	\brief Abstract way to find folders of importance in the
	system and among those created by the application.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#include "Folder.h"
#include <UniversalDefines.h>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <Localization.h>

// application includes
#include "UIStrings.h"



#pragma mark Public Methods

/*!
Fills in a new-style file system specification record with
information for a particular folder.  If the folder doesn’t
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
	case kFolder_RefApplicationSupport: // the “MacTerm” folder inside the Application Support folder
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
				
				// if no MacTerm folder exists in the Application Support folder, create it
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
	
	case kFolder_RefPreferences: // the legacy application preferences folder inside the system “Preferences” folder
		result = Folder_GetFSRef(kFolder_RefMacPreferences, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no folder exists in the current user’s Preferences folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationPreferences,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefScriptsMenuItems: // the legacy “Scripts Menu Items” folder, in the application’s preferences folder
		result = Folder_GetFSRef(kFolder_RefPreferences, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no Scripts Menu Items folder exists in the current user’s Preferences folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameApplicationScriptsMenuItems,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefUserLogs: // the “Logs” folder inside the user’s home directory
		result = Folder_GetFSRef(kFolder_RefMacLibrary, parentFolderRef);
		if (noErr == result)
		{
			UInt32		unusedDirID = 0L;
			
			
			// if no Logs folder exists in the Library folder, create it
			result = UIStrings_CreateFileOrDirectory(parentFolderRef, kUIStrings_FolderNameHomeLibraryLogs,
														outFolderFSRef, &unusedDirID);
		}
		break;
	
	case kFolder_RefMacApplicationSupport: // the Mac OS “Application Support” folder for the current user
		result = FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacLibrary: // the Mac OS “Library” folder for the current user
		result = FSFindFolder(kUserDomain, kDomainLibraryFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacPreferences: // the Mac OS “Preferences” folder for the current user
		result = FSFindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	case kFolder_RefMacTemporaryItems: // the invisible Mac OS “Temporary Items” folder
		result = FSFindFolder(kUserDomain, kTemporaryFolderType, kCreateFolder, &outFolderFSRef);
		break;
	
	default:
		// ???
		result = invalidFolderTypeErr;
		break;
	}
	
	return result;
}// GetFSRef

// BELOW IS REQUIRED NEWLINE TO END FILE
