/*###############################################################

	AppResources.cp
	
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
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>

// resource includes
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Local.h"
#include "UIStrings.h"



#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	CFRetainRelease&	gApplicationBundle ()	{ static CFRetainRelease x; return x; }
	SInt16				gResourceFilePreferences = -1;
}

#pragma mark Internal Method Prototypes

static OSStatus		createPictureFromFile		(SInt16, PicHandle&);
static OSStatus		launchResourceApplication	(CFStringRef);
static OSStatus		openPictureWithName			(UIStrings_FileOrFolderCFString, SInt16*);



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
Locates and opens the picture files in the Mac OS X
application bundle that combine to make an application
dock tile overlay of an “attention” icon, renders them
in memory using QuickDraw and returns handles to them
(you might subsequently call a routine like
Embedding_BuildCGImageFromPictureAndMask() to combine
the image and mask).  Call KillPicture() when finished
with each picture.

Note that you cannot have a QuickDraw picture open
when you invoke this routine.

\retval noErr
if the pictures were created successfully

\retval any Mac OS File Manager error
if there are problems opening the file (e.g. "fnfErr"
if the file can’t be found)

(3.0)
*/
AppResources_Result
AppResources_GetDockTileAttentionPicture	(PicHandle&		outPicture,
											 PicHandle&		outMask)
{
	SInt16					fileRefNum = 0;
	AppResources_Result		result = openPictureWithName
										(kUIStrings_FileNameDockTileAttentionPicture,
											&fileRefNum);
	
	
	if (result == noErr)
	{
		result = createPictureFromFile(fileRefNum, outPicture);
		if (result == noErr)
		{
			FSClose(fileRefNum), fileRefNum = 0;
			result = openPictureWithName(kUIStrings_FileNameDockTileAttentionMask, &fileRefNum);
			if (result == noErr)
			{
				result = createPictureFromFile(fileRefNum, outMask);
				FSClose(fileRefNum), fileRefNum = 0;
			}
		}
	}
	return result;
}// GetDockTileAttentionPicture


/*!
Locates and opens the splash screen picture file in
the Mac OS X application bundle, renders the picture
in memory using QuickDraw and returns a handle to it.
Call KillPicture() when finished with the picture.

Note that you cannot have a QuickDraw picture open
when you invoke this routine.

\retval noErr
if the picture was created successfully

\retval any Mac OS File Manager error
if there are problems opening the file (e.g. "fnfErr"
if the file can’t be found)

(3.0)
*/
AppResources_Result
AppResources_GetSplashScreenPicture		(PicHandle&		outPicture)
{
	SInt16					fileRefNum = 0;
	AppResources_Result		result = openPictureWithName
										(kUIStrings_FileNameSplashScreenPicture,
											&fileRefNum);
	
	
	if (result == noErr)
	{
		result = createPictureFromFile(fileRefNum, outPicture);
		FSClose(fileRefNum);
	}
	return result;
}// GetSplashScreenPicture


/*!
Locates and opens the picture files in the Mac OS X
application bundle that form the specified frame of the
toolbar “poof” animation, renders them in memory using
QuickDraw and returns handles to the frame and its mask.
You might subsequently call a routine like
Embedding_BuildCGImageFromPictureAndMask() to combine
the images and masks.  Call KillPicture() when finished
with each picture.

Note that you cannot have a QuickDraw picture open when
you invoke this routine.

\retval noErr
if the pictures were created successfully

\retval paramErr
if the specified animation stage index is not valid

\retval any Mac OS File Manager error
if there are problems opening the file (e.g. "fnfErr" if
the file can’t be found)

(3.0)
*/
AppResources_Result
AppResources_GetToolbarPoofPictures		(UInt16			inZeroBasedAnimationStageIndex,
										 PicHandle&		outFramePicture,
										 PicHandle&		outFrameMask)
{
	AppResources_Result		result = (inZeroBasedAnimationStageIndex >= 5)
										? STATIC_CAST(paramErr, AppResources_Result)
										: STATIC_CAST(noErr, AppResources_Result);
	
	
	if (result == noErr)
	{
		SInt16							fileRefNum = 0;
		UIStrings_FileOrFolderCFString	pictureName = kUIStrings_FileNameToolbarPoofFrame1Picture;
		UIStrings_FileOrFolderCFString	maskName = kUIStrings_FileNameToolbarPoofFrame1Mask;
		
		
		switch (inZeroBasedAnimationStageIndex)
		{
		case 1:
			pictureName = kUIStrings_FileNameToolbarPoofFrame1Picture;
			maskName = kUIStrings_FileNameToolbarPoofFrame1Mask;
			break;
		
		case 2:
			pictureName = kUIStrings_FileNameToolbarPoofFrame2Picture;
			maskName = kUIStrings_FileNameToolbarPoofFrame2Mask;
			break;
		
		case 3:
			pictureName = kUIStrings_FileNameToolbarPoofFrame3Picture;
			maskName = kUIStrings_FileNameToolbarPoofFrame3Mask;
			break;
		
		case 4:
			pictureName = kUIStrings_FileNameToolbarPoofFrame4Picture;
			maskName = kUIStrings_FileNameToolbarPoofFrame4Mask;
			break;
		
		case 5:
			pictureName = kUIStrings_FileNameToolbarPoofFrame5Picture;
			maskName = kUIStrings_FileNameToolbarPoofFrame5Mask;
			break;
		
		default:
			// ???
			break;
		}
		
		openPictureWithName(pictureName, &fileRefNum);
		if (result == noErr)
		{
			result = createPictureFromFile(fileRefNum, outFramePicture);
			if (result == noErr)
			{
				FSClose(fileRefNum), fileRefNum = 0;
				result = openPictureWithName(maskName, &fileRefNum);
				if (result == noErr)
				{
					result = createPictureFromFile(fileRefNum, outFrameMask);
					FSClose(fileRefNum), fileRefNum = 0;
				}
			}
		}
	}
	return result;
}// GetToolbarPoofPictures


/*!
Launches a separate application informing the user
that MacTelnet has crashed.  This should be called
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
To determine a resource file’s reference number
given its resource file descriptor, use this
method.

See also AppResources_UseResFile(), a convenient
routine for changing the current resource file.

(3.0)
*/
SInt16
AppResources_ReturnResFile		(AppResources_FileID	inResourceFileType)
{
	SInt16		result = -1; // IMPORTANT - initialize to an INVALID file descriptor
	
	
	switch (inResourceFileType)
	{
	case kAppResources_FileIDPreferences:
		result = gResourceFilePreferences;
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// ReturnResFile


/*!
To assign a resource file reference number to a
particular resource file descriptor, use this
method.

WARNING:	This is only done once for each type
			of resource file, at the start of
			your program when you’ve determined
			the descriptors of important open
			resource files.  Unless you use one
			or more Resource Manager routines to
			explicitly close and re-open your
			resource files, do not call this
			routine again for the same resource
			file type.

(3.0)
*/
void
AppResources_SetResFile		(AppResources_FileID	inResourceFileType,
							 SInt16					inResourceFileRefNum)
{
	switch (inResourceFileType)
	{
	case kAppResources_FileIDPreferences:
		gResourceFilePreferences = inResourceFileRefNum;
		break;
	
	default:
		// ???
		break;
	}
}// SetResFile


/*!
Use this convenient method to update the
specified resource file (equivalent to calling
the Resource Manager function UpdateResFile()
on the appropriate file reference number).

(3.0)
*/
void
AppResources_UpdateResFile	(AppResources_FileID	inResourceFileType)
{
	UpdateResFile(AppResources_ReturnResFile(inResourceFileType));
}// UpdateResFile


/*!
Use this convenient method to change the
current resource file (equivalent to calling
the Resource Manager function UseResFile()
on the appropriate file reference number).

(3.0)
*/
void
AppResources_UseResFile		(AppResources_FileID	inResourceFileType)
{
	UseResFile(AppResources_ReturnResFile(inResourceFileType));
}// UseResFile


#pragma mark Internal Methods

/*!
Creates a new QuickDraw picture from a picture
file, and returns a handle to the picture data
(which can then be used with DrawPicture(), etc.).
Requires QuickTime.

Uses QuickDraw to open a new picture, so no picture
can be in the process of being drawn when this
routine is invoked.

(3.0)
*/
static OSStatus
createPictureFromFile	(SInt16			inFileReferenceNumber,
						 PicHandle&		outPicture)
{
	OSStatus			result = noErr;
	Rect				pictureBounds;
	OpenCPicParams		params;
	
	
	result = GetPictureFileHeader(inFileReferenceNumber, &pictureBounds, &params);
	if (result == noErr)
	{
		outPicture = OpenCPicture(&params);
		if (outPicture != nullptr)
		{
			result = DrawPictureFile(inFileReferenceNumber, &pictureBounds, nullptr/* progress info */);
			ClosePicture();
		}
	}
	return result;
}// createPictureFromFile


/*!
Attempts to open an application in this application’s
bundle that has the specified name.

(3.0)
*/
static OSStatus
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
		CFBundleRef		executableBundle = CFBundleCreate(kCFAllocatorDefault, resourceURL);
		
		
		if (executableBundle == nullptr) result = paramErr;
		else
		{
			CFURLRef	executableURL = CFBundleCopyExecutableURL(executableBundle);
			
			
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
		CFRelease(resourceURL);
	#endif
	}
	
	Console_WriteValue("launch result", result);
	return result;
}// launchResourceApplication


/*!
Attempts to open a picture file with the specified
name, searching the application bundle.

(3.0)
*/
static OSStatus
openPictureWithName		(UIStrings_FileOrFolderCFString		inName,
						 SInt16*							outFileReferenceNumber)
{
	OSStatus		result = noErr;
	CFStringRef		nameCFString = nullptr;
	
	
	if (UIStrings_Copy(inName, nameCFString).ok())
	{
		CFURLRef	resourceURL = nullptr;
		
		
		resourceURL = CFBundleCopyResourceURL(AppResources_ReturnApplicationBundle(), nameCFString,
												nullptr/* type string */, nullptr/* subdirectory path */);
		if (resourceURL != nullptr)
		{
			FSRef	resourceFSRef;
			
			
			if (CFURLGetFSRef(resourceURL, &resourceFSRef))
			{
				result = FSOpenFork(&resourceFSRef, 0/* fork name length */, nullptr/* fork name */,
									fsCurPerm, outFileReferenceNumber);
			}
			CFRelease(resourceURL);
		}
		CFRelease(nameCFString);
	}
	else
	{
		result = resNotFound;
	}
	return result;
}// openPictureWithName

// BELOW IS REQUIRED NEWLINE TO END FILE
