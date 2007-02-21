/*!	\file PreferenceResources.h
	\brief Constants describing preference resources, such as
	standard resource types, file names, and resource IDs.
*/
/*###############################################################

	MacTelnet
		© 1998-2005 by Kevin Grant.
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

#ifndef __PREFERENCERESOURCES__
#define __PREFERENCERESOURCES__



// file type and creator of the settings file
#define	kPreferencesFileCreatorSignature	'kEVg'
#define	kPreferencesFileType				'pref'

// resource types of preference resources, either in the
// preferences file (custom) or in the application file (defaults)=
#define	FTPSERVERPREFS_RESTYPE				'FPRF'
#define	SESSIONPREFS_RESTYPE				'SPRF'
#define	TERMINALPREFS_RESTYPE				'TPRF'
#define	PROXYPREFS_RESTYPE					'‡PRF'
#define	FTPUSERPREFS_RESTYPE				'FTPu'
#define	WINDOWPREFS_RESTYPE					'WPRF'

// “...DEFAULTID” names are resource IDs of preference defaults
// in the application resource file.
#define SESSIONPREFS_DEFAULTID				128
#define	TERMINALPREFS_DEFAULTID				128
#define	PROXYPREFS_DEFAULTID	 			128
#define	WINDOWPREFS_DEFAULTID				128
#define ANSICOLORS_DEFAULTID				9999
// “...ID” names are the preferences as the user modified them,
// in the preferences resource file.  For sessions and terminals,
// resource names correspond to session and terminal names, and
// new resource IDs are indexed from the default one.=
#define	WINDOWPREFS_ID						128
#define ANSICOLORS_ID						12000

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
