/*!	\file AppResources.cp
	\brief Easy access to resources located in application
	resource files.
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

#include "AppResources.h"
#include <UniversalDefines.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>

// application includes
#include "Local.h"
#include "UIStrings.h"



#pragma mark Variables
namespace {

CFRetainRelease&	gApplicationBundle ()	{ static CFRetainRelease x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSStatus	launchResourceApplication	(CFStringRef);

} // anonymous namespace



#pragma mark Public Methods

/*!
Call this before any other routines in this module.

Provide the name of the bundle containing resources.
Normally this is CFBundleGetMainBundle(), however
in a scripting scenario (for instance) this may have
to be reset explicitly.

(3.1)
*/
void
AppResources_Init	(CFBundleRef	inApplicationBundle)
{
	if (nullptr != inApplicationBundle)
	{
		gApplicationBundle() = inApplicationBundle;
	}
}// Init


/*!
Provides an FSRef for the requested resource
file from one of the Resources subdirectories
of the main bundle.

Returns "true" only if the file can be found.

(3.1)
*/
Boolean
AppResources_GetArbitraryResourceFileFSRef	(CFStringRef	inName,
											 CFStringRef	inTypeOrNull,
											 FSRef&			outFSRef)
{
	Boolean		result = false;
	CFURLRef	resourceFileURL = nullptr;
	
	
	resourceFileURL = CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), inName, inTypeOrNull,
												nullptr/* no particular subdirectory */);
	if (nullptr != resourceFileURL)
	{
		if (CFURLGetFSRef(resourceFileURL, &outFSRef))
		{
			result = true;
		}
		CFRelease(resourceFileURL);
	}
	
	return result;
}// GetArbitraryResourceFileFSRef


/*!
Launches a separate application informing the user
that MacTerm has crashed.  This should be called
from within a termination signal handler.

(3.0)
*/
AppResources_Result
AppResources_LaunchCrashCatcher ()
{
	return launchResourceApplication(CFSTR("BugReporter.app"));
}// LaunchCrashCatcher


/*!
Launches a separate application that locates and
converts old user preferences to the new format.

(3.1)
*/
AppResources_Result
AppResources_LaunchPreferencesConverter ()
{
	return launchResourceApplication(CFSTR("PrefsConverter.app"));
}// LaunchPreferencesConverter


/*!
ALWAYS use this instead of CFBundleGetMainBundle(),
to retrieve the bundle that contains resources.  In
general, be explicit about bundles at all times
(e.g. do not use CreateNibReference(), instead use
CreateNibReferenceWithCFBundle(), etc.).

The CFBundle API views the main bundle as the place
where the parent executable lives, and this is not
correct in cases like a scripting environment where
the interpreter is the parent.  This API ensures the
right bundle for resources can still be used.

See also the various other routines for determining
the bundle appropriate for a task.

(3.1)
*/
CFBundleRef
AppResources_ReturnApplicationBundle ()
{
	CFBundleRef		result = gApplicationBundle().returnCFBundleRef();
	
	
	assert(nullptr != result);
	return result;
}// ReturnApplicationBundle


/*!
ALWAYS use this instead of CFBundleGetMainBundle(),
to retrieve the bundle that contains application
information (such as the version).

See also the various other routines for determining
the bundle appropriate for a task.

(3.1)
*/
CFBundleRef
AppResources_ReturnBundleForInfo ()
{
	// currently, this is the same as the primary bundle
	CFBundleRef		result = gApplicationBundle().returnCFBundleRef();
	
	
	assert(nullptr != result);
	return result;
}// ReturnBundleForInfo


/*!
ALWAYS use this instead of CFBundleGetMainBundle(),
to retrieve the bundle that contains NIBs.  In
general, be explicit about bundles at all times
(e.g. do not use CreateNibReference(), instead use
CreateNibReferenceWithCFBundle(), etc.).

See also the various other routines for determining
the bundle appropriate for a task.

(3.1)
*/
CFBundleRef
AppResources_ReturnBundleForNIBs ()
{
	// currently, this is the same as the primary bundle
	CFBundleRef		result = gApplicationBundle().returnCFBundleRef();
	
	
	assert(nullptr != result);
	return result;
}// ReturnBundleForNIBs


/*!
Returns the four-character code representing the application,
used with many system APIs as a quick way to produce a value
that is reasonably unique compared to other applications.

(3.1)
*/
UInt32
AppResources_ReturnCreatorCode ()
{
	static UInt32	gCreatorCode = 0;
	
	
	// only need to look it up once, then remember it
	if (0 == gCreatorCode)
	{
		CFBundleRef		infoBundle = AppResources_ReturnBundleForInfo();
		UInt32			typeCode = 0;
		
		
		assert(nullptr != infoBundle);
		CFBundleGetPackageInfo(infoBundle, &typeCode, &gCreatorCode);
		assert(0 != gCreatorCode);
	}
	return gCreatorCode;
}// ReturnCreatorCode


#pragma mark Internal Methods
namespace {

/*!
Attempts to open an application in this application’s
bundle that has the specified name.

(3.0)
*/
OSStatus
launchResourceApplication	(CFStringRef	inName)
{
	OSStatus	result = noErr;
	CFURLRef	resourceURL = nullptr;
	
	
#if 1
	// if placed in a .lproj directory...
	resourceURL = CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), inName,
											nullptr/* type string */, nullptr/* subdirectory path */);
#else
	// if placed in the MacOS folder (execution is not reliable if placed here?)
	resourceURL = CFBundleCopyAuxiliaryExecutableURL(AppResources_ReturnApplicationBundle(), inName);
#endif
	if (resourceURL != nullptr)
	{
	#if 1
		// the resource URL provides a bundle folder path; now use the bundle to locate the main executable file
		CFRetainRelease		executableBundle(CFBundleCreate(kCFAllocatorDefault, resourceURL),
												true/* is retained */);
		
		
		if (false == executableBundle.exists()) result = paramErr;
		else
		{
			CFURLRef	executableURL = CFBundleCopyExecutableURL(executableBundle.returnCFBundleRef());
			
			
			if (executableURL == nullptr) result = fnfErr;
			else
			{
				// given the path to the executable, launch it and BLOCK so that
				// the launched program must exit before continuing here
				UInt8	unixPath[PATH_MAX];
				
				
				if (CFURLGetFileSystemRepresentation(executableURL, true/* resolve against base */,
														unixPath, sizeof(unixPath)))
				{
					Local_Result	spawnError = kLocal_ResultOK;
					UInt8			escapedPath[PATH_MAX];
					
					
					// IMPORTANT: The (eventual) system() call will run a SHELL, so
					// the path must be escaped accordingly.  This is NOT perfect
					// escaping, and should really IMPROVE, but single-quoting is
					// certainly good enough to cover a very common case (spaces).
					// A better solution is to individually recognize and escape,
					// using a backslash (\), every character that the shell thinks
					// is significant.  TEMPORARY.
					escapedPath[0] = '\'';
					escapedPath[1] = '\0';
					std::strncpy(REINTERPRET_CAST(escapedPath, char*) + 1, REINTERPRET_CAST(unixPath, char const*),
									sizeof(escapedPath) - 3/* quotes + terminator */);
					std::strncat(REINTERPRET_CAST(escapedPath, char*), "\'", 1);
					
					spawnError = Local_SpawnProcessAndWaitForTermination(REINTERPRET_CAST(unixPath, char const*));
					if (spawnError != kLocal_ResultOK)
					{
						// kind of an arbitrary OS error, but reasonably close
						// in meaning to what the real problem is
						result = errOSACantLaunch;
					}
				}
				else
				{
					// kind of an arbitrary OS error, but reasonably close
					// in meaning to what the real problem is
					result = kURLInvalidURLError;
				}
				CFRelease(executableURL), executableURL = nullptr;
			}
		}
	#else
		// do not use Launch Services, because it cannot “block” and
		// wait for the launched application to actually quit (there
		// may be some other way to do this, such as setting up some
		// kind of Carbon Event handler, but that seems too complex
		// compared to just using the Unix system() routine)
		LSLaunchURLSpec		config;
		
		
		std::memset(&config, 0, sizeof(config));
		config.appURL = resourceURL;
		config.itemURLs = nullptr;
		config.passThruParams = nullptr;
		config.launchFlags = kLSLaunchDontAddToRecents;
		config.asyncRefCon = nullptr;
		result = LSOpenFromURLSpec(&config, nullptr/* launched URL */);
	#endif
		CFRelease(resourceURL), resourceURL = nullptr;
	}
	
	Console_WriteValue("launch result", result);
	return result;
}// launchResourceApplication

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
