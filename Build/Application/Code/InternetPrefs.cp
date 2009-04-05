/*###############################################################

	InternetPrefs.cp
	
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

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "InternetPrefs.h"



#pragma mark Constants

// The following constants MUST all have prefixes
// identical to the UniformResourceLocatorType
// defined in "InternetPrefs.h", and suffixes as
// defined by the macro HELPER_BY_URL_KIND, below.
#define kNotURLHelper		nullptr
#define kMailtoURLHelper	"\pHelper•mailto"
#define kNewsURLHelper		"\pHelper•nntp"
#define kNntpURLHelper		"\pHelper•nntp"
#define kFtpURLHelper		"\pHelper•ftp"
#define kHttpURLHelper		"\pHelper•http"
#define kGopherURLHelper	"\pHelper•gopher"
#define kWaisURLHelper		"\pHelper•wais"
#define kFingerURLHelper	"\pHelper•finger"
#define kWhoisURLHelper		"\pHelper•whois"
// the following are handled by MacTelnet, and hence are nullptr
#define kTelnetURLHelper	nullptr
#define kTn3270URLHelper	nullptr

// macro to construct the above names out of a UniformResourceLocatorType
#define HELPER_BY_URL_KIND(kind)	(kind##Helper)



#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	ICInstance	gInternetConfigInstance = nullptr;
	Boolean		gHaveIC = false;
}



#pragma mark Public Methods

/*!
Call this method once before using any
other routines from this module.

(2.6)
*/
void
InternetPrefs_Init ()
{
	OSStatus		error = noErr;
	ICDirSpecArray	folderSpec;
	
	
	error = ICStart(&gInternetConfigInstance, AppResources_ReturnCreatorCode());
	
	folderSpec[0].vRefNum = -1; // -1 = search for system preferences
	folderSpec[0].dirID = 2;
#if TARGET_API_MAC_OS8
	error = ICFindConfigFile(gInternetConfigInstance, 1, (ICDirSpecArrayPtr)&folderSpec);
#else
	error = cfragNoSymbolErr;
#endif
	gHaveIC = (error == noErr);
}// Init


/*!
Call this method when you are completely
done with this module.

(2.6)
*/
void
InternetPrefs_Done ()
{
	if (gHaveIC)
	{
		(OSStatus)ICStop(gInternetConfigInstance);
	}
}// Done


/*!
Returns the application signature of a helper
application capable of handling the specified
type of URL, or "kInternetConfigUnknownHelper"
if the specified URL is not handled.

(3.0)
*/
OSType
InternetPrefs_HelperApplicationForURLType		(UniformResourceLocatorType		urlKind)
{
	OSType		result = kInternetConfigUnknownHelper;
	
	
	if (gHaveIC)
	{
		ConstStr255Param	key = nullptr;
		
		
		// Use the macro HELPER_BY_URL_KIND to construct
		// a string on the fly; this saves the ludicrous
		// array of strings that were previously stored
		// as global variables in NCSA Telnet 2.6 and
		// not used for ANYTHING else.
		switch (urlKind)
		{
		case kNotURL:
			key = HELPER_BY_URL_KIND(kNotURL);
			break;
		
		case kMailtoURL:
			key = HELPER_BY_URL_KIND(kMailtoURL);
			break;
		
		case kNewsURL:
			key = HELPER_BY_URL_KIND(kNewsURL);
			break;
		
		case kNntpURL:
			key = HELPER_BY_URL_KIND(kNntpURL);
			break;
		
		case kFtpURL:
			key = HELPER_BY_URL_KIND(kFtpURL);
			break;
		
		case kHttpURL:
			key = HELPER_BY_URL_KIND(kHttpURL);
			break;
		
		case kGopherURL:
			key = HELPER_BY_URL_KIND(kGopherURL);
			break;
		
		case kWaisURL:
			key = HELPER_BY_URL_KIND(kWaisURL);
			break;
		
		case kTelnetURL:
			key = HELPER_BY_URL_KIND(kTelnetURL);
			break;
		
		case kTn3270URL:
			key = HELPER_BY_URL_KIND(kTn3270URL);
			break;
		
		case kFingerURL:
			key = HELPER_BY_URL_KIND(kFingerURL);
			break;
		
		case kWhoisURL:
			key = HELPER_BY_URL_KIND(kWhoisURL);
			break;
		
		case kQuickTimeURL:
			result = 'TVOD'; // QuickTime™ Player
			break;
		
		default:
			key = nullptr;
			break;
		}
		if (nullptr != key)
		{
			OSStatus	icError = noErr;
			ICAttr		attr = 0L;
			long		size = 0L;
			ICAppSpec	icAppSpec;
			
			
			size = sizeof(icAppSpec);
			icError = ICBegin(gInternetConfigInstance, icReadOnlyPerm); // icReadOnlyPerm = MacTelnet doesn’t touch it
			icError |= ICGetPref(gInternetConfigInstance, key, &attr, &icAppSpec, &size);
			icError |= ICEnd(gInternetConfigInstance);
			
			if (noErr == icError) result = icAppSpec.fCreator;
		}
	}
	
	return result;
}// HelperApplicationForURLType

// BELOW IS REQUIRED NEWLINE TO END FILE
