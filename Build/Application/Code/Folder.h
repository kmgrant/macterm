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
	
	For convenience, Folder_GetFSSpec() returns file
	specifications with empty “file names”.  This means that the
	volume information is filled in, the “file name” is an empty
	string, and the “parent ID” is actually the ID of the
	directory itself.  This makes it easy to construct file
	specifications for any item inside a directory, since you
	need only fill in the file name appropriately and call
	FSMakeFSSpec().  Alternately, you can call FSMakeFSSpec()
	immediately on the returned FSSpec, and the File Manager
	will reconstruct the file specification so that the
	“file name” is the name of the directory and the “parent ID”
	is the ID of the parent folder.
*/
/*###############################################################

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

#ifndef __FOLDERMANAGER__
#define __FOLDERMANAGER__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef SInt16 Folder_Ref;
enum
{
	// folders defined by MacTelnet
	kFolder_RefApplication = 0,					// The folder containing the application program FILE; this
												//	is where mandatory files (such as shared libraries and
												//	the MacTelnet Help launcher) should exist.  This is NOT
												//  where user files (such as the Scripts Menu Items folder)
												//  go, because on Mac OS X the application folder is not
												//  accessible - it is hidden by the package (which looks
												//  like a file).  In general, to look for user files that
												//  should go in the “application folder”, use the constant
												//  "kFolder_RefPackageParent", which returns the correct
												//  folder on both Mac OS 8/9 and Mac OS X.
	
	kFolder_RefApplicationSupport = 1,			// MacTelnet’s folder in the Application Support folder.
	
	kFolder_RefFavorites = 2,					// MacTelnet’s “Favorites” folder, in the preferences folder.
	
	kFolder_RefPackageParent = 3,				// The folder that LOOKS LIKE it contains MacTelnet’s
												//   application program file.  On Mac OS 8, this is the same
												//   as "kFolder_RefApplication"; however, on Mac OS X,
												//   this refers to the folder that contains the application
												//   program package (which looks like a normal application
												//   icon in Mac OS X).  This folder is “user-accessible” at
												//   all times, whereas "kFolder_RefApplication" is hidden
												//   for Mac OS X users.  If you want the user to places files
												//   (such as Scripts Menu Items) into a place relative to the
												//   location of MacTelnet, use this constant to describe the
												//   folder instead of "kFolder_RefApplication".
	
	kFolder_RefPreferences = 4,					// The folder “MacTelnet Preferences”, in the preferences
												//   folder of the user currently logged in.
	
	kFolder_RefScriptsMenuItems = 5,			// MacTelnet’s “Scripts Menu Items” folder.
	
	kFolder_RefStartupItems = 6,				// MacTelnet’s “Startup Items” folder, in MacTelnet’s
												//   preferences folder.
	
	kFolder_RefUserLogs = 7,					// where the MacTelnet Debugging Log goes, or any other log
	
	kFolder_RefUserMacroFavorites = 8,			// “Macro Sets” folder, in the Favorites folder.
	
	kFolder_RefUserProxyFavorites = 9,			// “Proxies” folder, in the Favorites folder.
	
	kFolder_RefUserSessionFavorites = 10,		// “Sessions” folder, in the Favorites folder.
	
	kFolder_RefUserTerminalFavorites = 11,		// “Terminals” folder, in the Favorites folder.
	
	kFolder_RefRecentSessions = 12,				// “Recent Sessions” folder, in MacTelnet’s preferences folder.
	
	// folders defined by the Mac OS
	kFolder_RefMacPreferences = -1,				// the Preferences folder for the user currently logged in
	
	kFolder_RefMacSystem = -2,					// the System Folder
	
	kFolder_RefMacTemporaryItems = -3,			// the (invisible) Temporary Items folder
	
	kFolder_RefMacFavorites = -4,				// the Favorites folder for the user currently logged in
	
	kFolder_RefMacDesktop = -5,					// the Desktop for the user currently logged in
	
	kFolder_RefMacHelp = -6,					// the system-wide Help folder
	
	kFolder_RefMacLibrary = -7,					// the Library folder for the user currently logged in
	
	kFolder_RefMacTrash = -8,					// the Trash
	
	kFolder_RefMacApplicationSupport = -9		// the Application Support folder
};



#pragma mark Public Methods

// RETURNS A FOLDER, CREATING IT IF IT DOESN’T EXIST (THE FILE NAME IS ALWAYS AN EMPTY STRING)
OSStatus
	Folder_GetFSSpec		(Folder_Ref		inFolderType,
							 FSSpec*		outFolderFSSpecPtr);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
