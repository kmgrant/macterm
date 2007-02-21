/*###############################################################

	QuillsBase.cp
	
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

// standard-C++ includes
#include <string>

// library includes
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <Console.h>

// MacTelnet includes
#include "AppResources.h"
#include "Initialize.h"
#include "MainEntryPoint.h"
#include "QuillsBase.h"



#pragma mark Public Methods
namespace Quills {

/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Base::all_init ()
{
	Initialize_ApplicationStartup(CFBundleGetMainBundle());
	
#if 0
	// should it ever become necessary to initialize the application
	// from an explicit bundle location on disk (passed as input), the
	// following code will convert the path to a real bundle reference
	CFRetainRelease		pathCFString = CFStringCreateWithCString
										(kCFAllocatorDefault, inBundlePath.c_str(),
											kCFStringEncodingUTF8);
	
	
	if (nullptr != pathCFString.returnCFStringRef())
	{
		Console_WriteValueCFString("given bundle URL", pathCFString.returnCFStringRef());
		CFRetainRelease		pathCFURL = CFURLCreateWithFileSystemPath
										(kCFAllocatorDefault, pathCFString.returnCFStringRef(),
											kCFURLPOSIXPathStyle, true/* is directory */);
		
		
		if (nullptr != pathCFURL.returnCFTypeRef())
		{
			CFURLRef		pathAsURL = CFUtilities_URLCast(pathCFURL.returnCFTypeRef());
			CFBundleRef		appBundle = CFBundleCreate(kCFAllocatorDefault, pathAsURL);
			
			
			if (nullptr != appBundle)
			{
				Initialize_ApplicationStartup(appBundle);
				CFRelease(appBundle), appBundle = nullptr;
			}
		}
	}
#endif
}// all_init


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Base::all_done ()
{
	Initialize_ApplicationShutdown();
	MainEntryPoint_ImmediatelyQuit();
}// all_done


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
std::string
Base::version ()
{
	std::string		result;
	CFBundleRef		mainBundle = AppResources_ReturnBundleForInfo();
	assert(nullptr != mainBundle);
	CFStringRef		bundleVersionCFString = REINTERPRET_CAST(CFBundleGetValueForInfoDictionaryKey
																(mainBundle, CFSTR("CFBundleVersion")),
																CFStringRef);
	
	
	if (nullptr != bundleVersionCFString)
	{
		assert(CFStringGetTypeID() == CFGetTypeID(bundleVersionCFString));
		char const*		bundleVersionCString = CFStringGetCStringPtr
												(bundleVersionCFString, kCFStringEncodingMacRoman);
		
		
		if (nullptr != bundleVersionCString)
		{
			result = bundleVersionCString;
		}
	}
	return result;
}// version


} // namespace Quills

// BELOW IS REQUIRED NEWLINE TO END FILE
