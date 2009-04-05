/*!	\file Folder.h
	\brief Abstract way to find folders of importance in the
	system and among those created by MacTelnet.
	
	The Folder Manager has been created in MacTelnet 3.0 so that
	MacTelnet manages folder information dynamically and without
	global variables.  Unlike Telnet 2.6, folders can always be
	found in version 3.0, even if the user moves or renames a
	folder while MacTelnet is running.  Also, using these methods
	guarantees consistent results, since you can request a folder
	in terms of a context, instead of using hard-coded file or
	path information.  You should use this whenever possible!
*/
/*###############################################################

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

#ifndef __FOLDERMANAGER__
#define __FOLDERMANAGER__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef UInt16 Folder_Ref;
enum
{
	// folders defined by MacTelnet
	kFolder_RefApplicationSupport = 1,			// MacTelnet’s folder in the Application Support folder.
	
	kFolder_RefFavorites = 2,					// MacTelnet’s “Favorites” folder, in the preferences folder.
	
	kFolder_RefPreferences = 3,					// The folder “MacTelnet Preferences”, in the preferences
												//   folder of the user currently logged in.
	
	kFolder_RefScriptsMenuItems = 4,			// MacTelnet’s “Scripts Menu Items” folder.
	
	kFolder_RefStartupItems = 5,				// MacTelnet’s “Startup Items” folder, in MacTelnet’s
												//   preferences folder.
	
	kFolder_RefUserLogs = 6,					// where the MacTelnet Debugging Log goes, or any other log
	
	kFolder_RefUserMacroFavorites = 7,			// “Macro Sets” folder, in the Favorites folder.
	
	kFolder_RefRecentSessions = 8,				// “Recent Sessions” folder, in MacTelnet’s preferences folder.
	
	// folders defined by Mac OS X
	kFolder_RefMacApplicationSupport = 9,		// the Application Support folder
	
	kFolder_RefMacDesktop = 10,					// the Desktop for the user currently logged in
	
	kFolder_RefMacFavorites = 11,				// the Favorites folder for the user currently logged in
	
	kFolder_RefMacHelp = 12,					// the system-wide Help folder
	
	kFolder_RefMacLibrary = 13,					// the Library folder for the user currently logged in
	
	kFolder_RefMacPreferences = 14,				// the Preferences folder for the user currently logged in
	
	kFolder_RefMacTemporaryItems = 15,			// the (invisible) Temporary Items folder
	
	kFolder_RefMacTrash = 16					// the Trash
};



#pragma mark Public Methods

// RETURNS A FOLDER, CREATING IT IF IT DOESN’T EXIST (THE FILE NAME IS ALWAYS AN EMPTY STRING)
OSStatus
	Folder_GetFSRef			(Folder_Ref		inFolderType,
							 FSRef&			outFolderFSRef);

// DEPRECATED
OSStatus
	Folder_GetFSSpec		(Folder_Ref		inFolderType,
							 FSSpec*		outFolderFSSpecPtr);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
