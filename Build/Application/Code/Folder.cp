/*!	\file Folder.cp
	\brief Abstract way to find folders of importance in the
	system and among those created by the application.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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
	
	
	switch (inFolderType)
	{
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
