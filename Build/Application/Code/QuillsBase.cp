/*###############################################################

	QuillsBase.cp
	
	MacTelnet
		© 1998-2010 by Kevin Grant.
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
#include <stdexcept>
#include <string>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>
#include <StringUtilities.h>

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
#if 1
	// as long as CFBundle is not confused by which application is
	// the current one, this call is sufficient; for now, the problem
	// is fixed, though in the past the system could mistakenly think
	// that the interpreter (Python) was the application bundle!
	Initialize_ApplicationStartup(CFBundleGetMainBundle());
#else
	// this alternate implementation assumes a given path argument
	// and locates a suitable bundle from that
	std::string		unusedBundlePath = "";
	CFStringRef		pathCFString = CFStringCreateWithCString
									(kCFAllocatorDefault, unusedBundlePath.c_str(),
										kCFStringEncodingUTF8);
	Boolean			success = false;
	
	
	if (nullptr != pathCFString)
	{
		CFURLRef	pathCFURL = CFURLCreateWithFileSystemPath
								(kCFAllocatorDefault, pathCFString,
									kCFURLPOSIXPathStyle, true/* is directory */);
		
		
		if (nullptr != pathCFURL)
		{
			CFRetainRelease		appBundle = CFBundleCreate(kCFAllocatorDefault, pathCFURL);
			
			
			if (appBundle.exists())
			{
				Initialize_ApplicationStartup(appBundle.returnCFBundleRef());
				success = true;
			}
			CFRelease(pathCFURL), pathCFURL = nullptr;
		}
		CFRelease(pathCFString), pathCFString = nullptr;
	}
	
	if (false == success)
	{
		Console_WriteValueCFString("given bundle path", pathCFString);
		throw std::runtime_error("unable to find a bundle at the given path");
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
	Initialize_ApplicationShutDownIsolatedComponents();
#if 0
	// Argh...well, this is a problem!  The final ExitToShell() *might* trigger
	// final releases of things like HIViews for TerminalViews, which in turn
	// triggers object destructor calls that *could* still rely on certain
	// modules being initialized.  In the old days this was no big deal because
	// it was always possible to know the order of teardown.  But as long as
	// the OS can do an end run and destroy objects as late as it damn well
	// pleases, especially as part of THE EXIT SYSTEM CALL, it will be very
	// difficult indeed to tear down modules in a safe order.  So the extremely
	// ugly work-around for now is to simply not tear anything down at all...
	Initialize_ApplicationShutDownRemainingComponents();
#endif
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
		StringUtilities_CFToUTF8(bundleVersionCFString, result);
	}
	return result;
}// version

} // namespace Quills

// BELOW IS REQUIRED NEWLINE TO END FILE
