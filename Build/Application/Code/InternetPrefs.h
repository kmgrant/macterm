/*!	\file InternetPrefs.h
	\brief Utility functions that require Internet Config 
	on Mac OS 8 and Mac OS 9 to look up system-wide
	Internet preferences.
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

#ifndef __INTERNETPREFS__
#define __INTERNETPREFS__



#pragma mark Constants

enum UniformResourceLocatorType
{
	// must be consecutive and zero-based
	kNotURL = 0,
	kMailtoURL = 1,
	kNewsURL = 2,
	kNntpURL = 3,
	kFtpURL = 4,
	kHttpURL = 5,
	kGopherURL = 6,
	kWaisURL = 7,
	kTelnetURL = 8,
	kRloginURL = 9,
	kTn3270URL = 10,
	kFingerURL = 11,
	kWhoisURL = 12,
	kQuickTimeURL = 13,
	kSshURL = 14,
	kSftpURL = 15,
	kXmanpageURL = 16
};

enum
{
	kInternetConfigUnknownHelper = '----'
};



#pragma mark Public Methods

void
	InternetPrefs_Init							();

void
	InternetPrefs_Done							();

OSType
	InternetPrefs_HelperApplicationForURLType	(UniformResourceLocatorType		urlKind);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
