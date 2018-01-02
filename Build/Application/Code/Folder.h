/*!	\file Folder.h
	\brief Abstract way to find folders of importance in the
	system and among those created by the application.
	
	It is important to rely on this API for consistent results,
	Use this whenever possible, certainly instead of direct
	path dependencies!  (In the future, this should be exposed
	to Python as well.)
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef UInt16 Folder_Ref;
enum
{
	// folders defined by the application
	kFolder_RefApplicationSupport = 1,			// the application’s folder in the Application Support folder
	
	kFolder_RefPreferences = 3,					// legacy folder for auxiliary files in the preferences
												//   folder of the user currently logged in
	
	kFolder_RefScriptsMenuItems = 4,			// legacy “Scripts Menu Items” folder
	
	kFolder_RefUserLogs = 6,					// where the debugging log goes, or any other log
	
	// folders defined by Mac OS X
	kFolder_RefMacApplicationSupport = 9,		// the Application Support folder
	
	kFolder_RefMacLibrary = 13,					// the Library folder for the user currently logged in
	
	kFolder_RefMacPreferences = 14,				// the Preferences folder for the user currently logged in
	
	kFolder_RefMacTemporaryItems = 15,			// the (invisible) Temporary Items folder
};



#pragma mark Public Methods

// RETURNS A FOLDER, CREATING IT IF IT DOESN’T EXIST (THE FILE NAME IS ALWAYS AN EMPTY STRING)
OSStatus
	Folder_GetFSRef			(Folder_Ref		inFolderType,
							 FSRef&			outFolderFSRef);

// BELOW IS REQUIRED NEWLINE TO END FILE
