/*###############################################################

	FileUtilities.cp
	
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

// standard-C includes
#ifdef __MWERKS__
#	include <eof.h>
#else
#	include <cstdio>
#endif
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <MemoryBlocks.h>
#include <StringUtilities.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "FileUtilities.h"
#include "Preferences.h"
#include "RecordAE.h"
#include "Terminology.h"



#pragma mark Constants

#define STRING_PATHNAME_DELIMITER "\p:"

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Boolean		gGetFilesInProgress = false;
}

#pragma mark Internal Method Prototypes

static OSStatus		getFilesInDirectory				(FSSpec const*, Boolean, OSType, OSType, FSSpec*, UInt32*);
static pascal void	ioCompletionGetFiles			(ParmBlkPtr);
static OSStatus		threadedGetFilesInDirectory		(FSSpec const*, Boolean, OSType, OSType, FSSpec*, UInt32*,
														UInt32);
static OSStatus		threadedGetFilesInDirectory2	(FSSpec const*, Boolean, OSType, OSType, FSSpec*, UInt32*,
														UInt32);



#pragma mark Public Methods

/*!
Uses the Apple-recommended way to downgrade an
FSRef to an FSSpec (which is generally needed
when using APIs that do not yet accept newer
File Manager types).

(3.1)
*/
OSStatus
FileUtilities_ConvertFSRefToFSSpec	(FSRef const&	inFSRef,
									 FSSpec&		outFSSpec)
{
	OSStatus	result = FSGetCatalogInfo(&inFSRef, kFSCatInfoNone, nullptr/* catalog info */,
											nullptr/* Unicode name */, &outFSSpec, nullptr/* parent FSRef */);
	
	
	return result;
}// ConvertFSRefToFSSpec


/*!
Determines if the specified file is an alias, and if
so, modifies the file specification to point to the
original file (if possible).  It does this by checking
for the “is alias” Finder Info bit, and if set, by
reading the first alias resource in the file, and if
successful, by resolving the alias record in that
resource.  In other words, it resolves the damned
alias without any pretty stuff.  For some reason, no
Alias Manager API is quite this simple (sigh).

(3.0)
*/
OSStatus
FileUtilities_EnsureOriginalFile	(FSSpec*		inoutFSSpecPtr)
{
	FileInfo		fileInfo;
	OSStatus		result = noErr;
	
	
	// if the file is an alias, resolve it first
	result = FSpGetFInfo(inoutFSSpecPtr, (FInfo*)&fileInfo);
	if ((result == noErr) && (fileInfo.finderFlags & kIsAlias))
	{
		AliasHandle		fileAlias = nullptr;
		SInt16			aliasResourceFile = -1;
		
		
		aliasResourceFile = FSpOpenResFile(inoutFSSpecPtr, fsRdPerm);
		result = ResError();
		if (result == noErr)
		{
			UseResFile(aliasResourceFile);
			if (Count1Resources(rAliasType) <= 0) result = resNotFound;
			else
			{
				// get the alias record and resolve it
				fileAlias = (AliasHandle)Get1IndResource(rAliasType, 1);
				result = ResError();
				
				if (result == noErr)
				{
					Boolean		aliasChanged = false;
					
					
					result = ResolveAlias(nullptr/* base */, fileAlias, inoutFSSpecPtr, &aliasChanged);
				}
			}
		}
	}
	return result;
}// EnsureOriginalFile


/*!
This convenient routine can be used to perform an
asynchronous search of a directory for which you
have a file specification, returning an array of
file specification records for each file in that
directory.

(3.0)
*/
OSStatus
FileUtilities_GetAllFilesInDirectory	(FSSpec const*		inDirectorySpecPtr,
										 FSSpec*			inFileArrayStoragePtr,
										 UInt32*			inoutFileMaxCountFileActualCountPtr)
{
	OSStatus	result = noErr;
	
	
	result = threadedGetFilesInDirectory2(inDirectorySpecPtr, false/* use signatures */, '----', '----',
											inFileArrayStoragePtr, inoutFileMaxCountFileActualCountPtr,
											300/* maximum blockage time, in ticks */);
	
	return result;
}// GetAllFilesInDirectory


/*!
This routine can be used to determine the directory
ID of a directory described by a file system
specification record.

(3.0)
*/
long
FileUtilities_ReturnDirectoryIDFromFSSpec	(FSSpec const*		inFSSpecPtr)
{
	StrFileName		name;
	CInfoPBRec		thePB;
	long			result = 0L;
	
	
	PLstrcpy(name, inFSSpecPtr->name);
	thePB.dirInfo.ioCompletion = 0L;
	thePB.dirInfo.ioNamePtr = name;
	thePB.dirInfo.ioVRefNum = inFSSpecPtr->vRefNum;
	thePB.dirInfo.ioFDirIndex = 0;
	thePB.dirInfo.ioDrDirID = inFSSpecPtr->parID;
	
	if (PBGetCatInfoSync(&thePB) == noErr) result = thePB.dirInfo.ioDrDirID;
	
	return result;
}// ReturnDirectoryIDFromFSSpec


/*!
This routine can be used to determine the “last
modified” time of a directory described by a
file system specification record.  You can use
this routine to determine if the user has added
or removed files from a directory (by saving
an initial modification time and later comparing
it against another time).

(3.0)
*/
unsigned long
FileUtilities_ReturnDirectoryDateFromFSSpec		(FSSpec const*		inFSSpecPtr,
												 FileUtilitiesDate	inWhichDate)
{
	StrFileName		name;
	CInfoPBRec		thePB;
	unsigned long	result = 0L;
	
	
	PLstrcpy(name, inFSSpecPtr->name);
	thePB.dirInfo.ioCompletion = 0L;
	thePB.dirInfo.ioNamePtr = name;
	thePB.dirInfo.ioVRefNum = inFSSpecPtr->vRefNum;
	thePB.dirInfo.ioFDirIndex = 0;
	thePB.dirInfo.ioDrDirID = inFSSpecPtr->parID;
	
	if (PBGetCatInfoSync(&thePB) == noErr) switch (inWhichDate)
	{
		case kFileUtilitiesDateOfModification:
			result = thePB.dirInfo.ioDrMdDat;
			break;
		
		case kFileUtilitiesDateOfCreation:
			result = thePB.dirInfo.ioDrCrDat;
			break;
		
		case kFileUtilitiesDateOfBackup:
			result = thePB.dirInfo.ioDrBkDat;
			break;
		
		default:
			// ???
			result = 0L;
			break;
	}
	
	return result;
}// ReturnDirectoryModificationTimeFromFSSpec


/*!
To copy the name of the specified directory into
the given string, use this method.  To determine
the name of a volume, pass "fsRtDirID" for the
directory ID.

(2.6)
*/
void
FileUtilities_GetDirectoryName	(short		inVRefNum,
								 long		inDirID,
								 Str32		outName)
{
	CInfoPBRec		theInfo;
	
	
	bzero(&theInfo, sizeof(CInfoPBRec));
	theInfo.dirInfo.ioVRefNum = inVRefNum;
	theInfo.dirInfo.ioDrDirID = inDirID;
	theInfo.dirInfo.ioNamePtr = outName;
	theInfo.dirInfo.ioFDirIndex = -1; // -1 == directory info only
	PBGetCatInfoSync(&theInfo);
}// GetDirectoryName


/*!
Determines the volume reference number of a
volume, given its index.  If any problems
occur, an error code is returned.

Updated to use modern File Manager data
structures and calls in 3.0.

(2.6)
*/
OSStatus
FileUtilities_GetIndVolume		(short		inIndex,
								 short*		outVRefNum)
{
	HParamBlockRec	pb;
	OSStatus		result = noErr;
	
	
	pb.volumeParam.ioCompletion = nullptr;
	pb.volumeParam.ioNamePtr = nullptr;
	pb.volumeParam.ioVolIndex = inIndex;
	
	result = PBHGetVInfoSync(&pb);
	
	*outVRefNum = pb.volumeParam.ioVRefNum;
	return result;
}// GetIndVolume


/*!
Determines the last-modified date and time of a file.
If any problems occur, an error code is returned.

(2.6)
*/
OSStatus
FileUtilities_GetLastModDateTime	(FSSpec*			inFSSpecPtr,
									 unsigned long*		outLastModDateTime)
{
	CInfoPBRec	pBlock;
	OSStatus	result = noErr;
	
	
	pBlock.hFileInfo.ioNamePtr = inFSSpecPtr->name;
	pBlock.hFileInfo.ioVRefNum = inFSSpecPtr->vRefNum;
	pBlock.hFileInfo.ioFDirIndex = 0;
	pBlock.hFileInfo.ioDirID = inFSSpecPtr->parID;
	result = PBGetCatInfoSync(&pBlock);
	if (result == noErr)
	{
		*outLastModDateTime = pBlock.hFileInfo.ioFlMdDat;
	}
	return result;
}// GetLastModDateTime


/*!
Use this routine to obtain a Pascal string
representation of a directory pathname,
knowing only its directory ID and disk volume
reference number.

If the specified directory can't be found,
the OS error code "dirNFErr" is returned;
otherwise, "noErr" is returned.  If a call to
PBGetCatInfoSync() fails, this routine may
also return one of its error codes.

(2.6)
*/
OSStatus
FileUtilities_GetPathnameFromDirectoryID	(long		inDirectoryID,
											 short		inVolumeReferenceNumber,
											 StringPtr	outFullPathNameString)
{
	CInfoPBRec	block;
	Str255		directoryName;
	OSStatus	result = noErr;
	Boolean		isError = false;
	
	
	outFullPathNameString[0] = '\0';
	
	block.dirInfo.ioDrParID = inDirectoryID;
	block.dirInfo.ioNamePtr = directoryName;
	do
	{
		block.dirInfo.ioVRefNum = inVolumeReferenceNumber;
		block.dirInfo.ioFDirIndex = -1;
		block.dirInfo.ioDrDirID = block.dirInfo.ioDrParID;
		result = PBGetCatInfoSync(&block);
		if (result != noErr) isError = true;
		if (!isError)
		{
			PLstrcat(directoryName, STRING_PATHNAME_DELIMITER);
			StringUtilities_PInsert(outFullPathNameString, directoryName);
		}
	} while ((block.dirInfo.ioDrDirID != fsRtDirID) && (!isError));
	
	if (!isError) StringUtilities_PInsert(outFullPathNameString, STRING_PATHNAME_DELIMITER);
	
	return result;
}// GetPathnameFromDirectoryID


/*!
Use this routine to obtain a Pascal string
representation of a directory pathname,
knowing only its file system specification.
If either the “file name” of the given file
specification is the name of a directory,
or if the file name is blank, then the
resultant path will refer to either a
directory or a volume.

If the specified directory can’t be found,
the OS error code "dirNFErr" is returned;
otherwise, "noErr" is returned.  If a call to
PBGetCatInfoSync() fails, this routine may
also return one of its error codes.

(3.0)
*/
OSStatus
FileUtilities_GetPathnameFromFSSpec		(FSSpec const*	inDirectorySpecPtr,
										 StringPtr		outFullPathNameString,
										 Boolean		inIsDirectory)
{
	OSStatus	result = noErr;
	
	
	if ((inDirectorySpecPtr == nullptr) || (outFullPathNameString == nullptr)) result = memPCErr;
	else
	{
		CInfoPBRec	block;
		Str255		directoryName;
		Boolean		isError = false,
					catName = false;
		FSSpec		file;
		
		
		// First assume the specification might be that of an alias.
		// If so, resolve it; if not, use the file that was given.
		{
			Preferences_AliasID		aliasID = Preferences_NewAlias(inDirectorySpecPtr);
			
			
			if (aliasID != kPreferences_InvalidAliasID)
			{
				unless (Preferences_AliasParse(aliasID, &file))
				{
					file = *inDirectorySpecPtr;
				}
				Preferences_DisposeAlias(aliasID), aliasID = kPreferences_InvalidAliasID;
			}
		}
		
		// Construct the HFS path for the file or directory (directory paths end with a trailing colon).
		outFullPathNameString[0] = '\0';
		
		if ((file.name != nullptr) && (PLstrlen(file.name))) catName = true;
		block.dirInfo.ioDrParID = file.parID;
		block.dirInfo.ioNamePtr = directoryName;
		
		do
		{
			block.dirInfo.ioVRefNum = file.vRefNum;
			block.dirInfo.ioFDirIndex = -1;
			block.dirInfo.ioDrDirID = block.dirInfo.ioDrParID;
			result = PBGetCatInfoSync(&block);
			if (result != noErr) isError = true;
			if (!isError)
			{
				PLstrcat(directoryName, STRING_PATHNAME_DELIMITER);
				StringUtilities_PInsert(outFullPathNameString, directoryName);
			}
		} while ((block.dirInfo.ioDrDirID != fsRtDirID) && (!isError));
		
		if (!isError) StringUtilities_PInsert(outFullPathNameString, STRING_PATHNAME_DELIMITER);
		
		if (catName)
		{
			// for file specifications that include a filename, append the name onto the end
			// of the parent directory’s pathname specification
			PLstrcat(outFullPathNameString, file.name);
			if (inIsDirectory) PLstrcat(outFullPathNameString, STRING_PATHNAME_DELIMITER);
		}
	}
	
	return result;
}// GetPathnameFromFSSpec


/*!
Use this routine to obtain a Pascal string
representation of a directory pathname in
the Unix file system, knowing only its HFS
file system specification.  If either the
“file name” of the given file specification
is the name of a directory, or if the file
name is blank, then the resultant path will
refer to either a directory or a volume.

WARNING:	The given string is assumed to be
			large enough for 255 characters.

\retval noErr
if the pathname was obtained successfully

\retval a Mac OS File Manager error
if there were problems converting the given
file specification record to a pathname

(3.0)
*/
OSStatus
FileUtilities_GetPOSIXPathnameFromFSSpec	(FSSpec const*		inDirectorySpecPtr,
											 StringPtr			outFullPathNameString,
											 Boolean			inIsDirectory)
{
	FSRef		fileDef;
	OSStatus	result = FSpMakeFSRef(inDirectorySpecPtr, &fileDef);
	
	
	if (result == noErr)
	{
		result = FSRefMakePath(&fileDef, REINTERPRET_CAST(outFullPathNameString, UInt8*),
								255 * sizeof(UInt8)/* number of characters maximum */);
		if (result == noErr)
		{
			StringUtilities_CToPInPlace(REINTERPRET_CAST(outFullPathNameString, CStringPtr));
			if (inIsDirectory) PLstrcat(outFullPathNameString, "\p/");
		}
	}
	return result;
}// GetPOSIXPathnameFromFSSpec


/*!
This convenient routine can be used to perform an
asynchronous search of a directory for which you
have a file specification, returning an array of
file specification records for each file in that
directory whose type and creator match those
given.

(3.0)
*/
OSStatus
FileUtilities_GetTypedFilesInDirectory	(FSSpec const*		inDirectorySpecPtr,
										 OSType				inDesiredTypeSignature,
										 OSType				inDesiredCreatorSignature,
										 FSSpec*			inFileArrayStoragePtr,
										 UInt32*			inoutFileMaxCountFileActualCountPtr)
{
	OSStatus	result = noErr;
	
	
	result = threadedGetFilesInDirectory2(inDirectorySpecPtr, true/* use signatures */, inDesiredTypeSignature,
											inDesiredCreatorSignature, inFileArrayStoragePtr,
											inoutFileMaxCountFileActualCountPtr, 300/* maximum blockage time, in ticks */);
	
	return result;
}// GetTypedFilesInDirectory


/*!
This routine launches an application with standard
launch flags and (under OS 8.5 or later) the
“launch application” Finder theme sound.  If any
problems occur, an error code is returned; if not,
"noErr" is returned.

(3.0)
*/
OSStatus
FileUtilities_LaunchApplicationFromFSSpec	(FSSpec*	inFSSpecPtr)
{
	LaunchParamBlockRec		launchInfo;
	OSStatus				result = noErr;
	
	
	launchInfo.reserved1 = 0L;
	launchInfo.reserved2 = 0;
	launchInfo.launchBlockID = extendedBlock;
	launchInfo.launchEPBLength = extendedBlockLen;
	launchInfo.launchFileFlags = 0;
	launchInfo.launchControlFlags = launchContinue | launchNoFileFlags;
	launchInfo.launchAppSpec = inFSSpecPtr;
	launchInfo.launchAppParameters = nullptr;
	result = LaunchApplication(&launchInfo);
	if (result == noErr)
	{
		(OSStatus)PlayThemeSound(kThemeSoundLaunchApp);
	}
	
	return result;
}// LaunchApplicationFromFSSpec


/*!
Use this convenient routine to fire an Apple Event
back to MacTelnet (that is hence recordable) to “open”
the specified file via an “open documents” event.  If
you are opening a file for any reason, you should
generally handle it by calling this routine, and then
“really” open the file within the implementation of
the “open documents” Apple Event handler.

(3.1)
*/
OSStatus
FileUtilities_OpenDocument	(FSRef const&	inFSRef)
{
	OSStatus		result = noErr;
	AEDescList		docList;
	
	
	// Create a “list” of documents containing only the specified file, add the file’s
	// OS data structure and then send it back to MacTelnet as an “open documents” event.
	// A list is used because it is more versatile.
	result = AppleEventUtilities_InitAEDesc(&docList);
	if (noErr == result)
	{
		result = AECreateList(nullptr/* factoring */, 0/* size */,
								false/* is a record */, &docList);
		if (noErr == result)
		{
			result = AEPutPtr(&docList, 1/* where to put it */, typeFSRef, &inFSRef, sizeof(FSRef));
			if (noErr == result)
			{
				FileUtilities_OpenDocuments(docList);
			}
			AEDisposeDesc(&docList);
		}
	}
	
	return result;
}// OpenDocument


/*!
Deprecated in favor of the version of this routine
that accepts an "FSRef".

(3.0)
*/
OSStatus
FileUtilities_OpenDocument	(FSSpec const*		inFSSpecPtr)
{
	OSStatus		result = noErr;
	AEDescList		docList;
	
	
	// Create a “list” of documents containing only the specified file, add the file’s
	// OS data structure and then send it back to MacTelnet as an “open documents” event.
	// A list is used because it is more versatile.
	result = AppleEventUtilities_InitAEDesc(&docList);
	if (noErr == result)
	{
		result = AECreateList(nullptr/* factoring */, 0/* size */,
								false/* is a record */, &docList);
		if (noErr == result)
		{
			result = AEPutPtr(&docList, 1/* where to put it */, typeFSS, inFSSpecPtr, sizeof(*inFSSpecPtr));
			if (noErr == result)
			{
				FileUtilities_OpenDocuments(docList);
			}
			AEDisposeDesc(&docList);
		}
	}
	
	return result;
}// OpenDocument


/*!
Sends the entire given list of files back to the
current application in an “open documents” event,
which will cause the files to be opened (as well
as having the action recorded into scripts, if
recording is in progress).

Currently, the list must contain FSSpec or FSRef
entries.

(3.1)
*/
OSStatus
FileUtilities_OpenDocuments		(AEDescList const&	inList)
{
	OSStatus	result = noErr;
	AppleEvent	openDocumentsEvent;
	
	
	// send the list back to MacTelnet as an “open documents” event
	result = AppleEventUtilities_InitAEDesc(&openDocumentsEvent);
	if (noErr == result)
	{
		result = RecordAE_CreateRecordableAppleEvent(kASRequiredSuite, kAEOpenDocuments,
														&openDocumentsEvent);
		if (noErr == result)
		{
			result = AEPutParamDesc(&openDocumentsEvent, keyDirectObject, &inList);
			if (noErr == result)
			{
				AppleEvent		dummyReply;
				
				
				result = AppleEventUtilities_InitAEDesc(&dummyReply);
				if (noErr == result)
				{
					// finally, send the “open documents” event!
					result = AESend(&openDocumentsEvent, &dummyReply, kAENoReply | kAEAlwaysInteract,
									kAENormalPriority, kAEDefaultTimeout, nullptr/* idle routine */,
									nullptr/* filter routine */);
				}
			}
			AEDisposeDesc(&openDocumentsEvent);
		}
	}
	
	return result;
}// OpenDocuments


/*!
To create a file in the specified location with the
specified name, or to create a file with a similar
name in the case of a duplicate, use this method.
Instead of failing with "dupFNErr" with FSpCreate(),
you can use this routine to be persistent about
using FSpCreate() repeatedly, until an error other
than "dupFNErr" occurs.  If any error occurs other
than "dupFNErr", it is returned.

If an alternate file name is used (because the given
one already exists), the resultant file specification
will be modified.

(3.0)
*/
OSStatus
FileUtilities_PersistentCreate		(FSSpec*		inoutFSSpecPtr,
									 OSType			inCreator,
									 OSType			inType,
									 ScriptCode		inScriptTag)
{
	OSStatus	result = noErr;
	
	
	if (inoutFSSpecPtr != nullptr)
	{
		FSSpec		file;
		
		
		result = FSMakeFSSpec(inoutFSSpecPtr->vRefNum, inoutFSSpecPtr->parID, inoutFSSpecPtr->name, &file);
		if ((result == noErr) || (result == fnfErr))
		{
			SInt16		i = 1;
			Str31		numString;
			Str255		originalFileName;
			
			
			PLstrcpy(originalFileName, file.name);
			do
			{
				if (!i) result = FSpCreate(&file, inCreator, inType, inScriptTag); // first try creating the specified file
				else
				{
					// try to create a similar file (like “File 1”, “File 2”, etc.)
					StringSubstitutionSpec const	metaMappings[] =
													{
														{ "\p%a", originalFileName },
														{ "\p%b", numString }
													};
					
					
					PLstrcpy(file.name, "\p%a %b");
					NumToString(i, numString);
					StringUtilities_PBuild(file.name, sizeof(metaMappings) / sizeof(StringSubstitutionSpec), metaMappings);
					result = FSMakeFSSpec(file.vRefNum, file.parID, file.name, &file);
					if ((result == noErr) || (result == fnfErr)) result = FSpCreate(&file, inCreator, inType, inScriptTag);
				}
				++i;
			}
			while (result == dupFNErr);
		}
		
		*inoutFSSpecPtr = file;
	}
	else result = memPCErr;
	
	return result;
}// PersistentCreate


/*!
Converts the given volume name into a disk
volume reference number.  If the named
volume cannot be found, the default volume
reference number is returned.

(2.6)
*/
short
FileUtilities_ReturnVolumeRefNumberForName	(Str32		inVolumeName)
{
	short	result = -1;
	
	
#if TARGET_API_MAC_OS8
	if (HSetVol(inVolumeName, 0, fsRtDirID) == noErr) GetVol(nullptr, &result);
#else
	HGetVol(inVolumeName, &result, nullptr/* directory ID */);
#endif
	
	return result;
}// ReturnVolumeRefNumberByName


/*!
Reads a text file into the specified handle, resizing
it if necessary to fit the data.  The number of bytes
read from the file is returned.

This routine currently reads data one byte at a time.

If any problems occur, 0 is returned.

(3.0)
*/
SInt32
FileUtilities_TextToHandle		(SInt16		inFileRefNum,
								 Handle		inoutTextHandle)
{
	char		character = '\0';
	OSStatus	error = noErr;
	SInt32		readHowMany = 1,
				count = 0L,
				result = 0L;
	
	
	// read file to determine its size
	count = 0L;
	while (character != EOF)
	{
		if ((error = FSRead(inFileRefNum, &readHowMany, &character)) == eofErr) character = EOF;
		else
		{
			if (readHowMany != 1) readHowMany = 1; // reset count (read failed)
			else ++count;
		}
	}
	
	// attempt to size handle appropriately
	error = Memory_SetHandleSize(inoutTextHandle, count);
	if (error != noErr) result = 0L; // failure - return 0 to indicate no bytes were read
	else
	{
		// success
		SInt32		capacityLength = GetHandleSize(inoutTextHandle);
		char*		ptr = nullptr;
		SInt8		hState = HGetState(inoutTextHandle);
		
		
		// read data into properly-sized handle
		readHowMany = 1;
		count = 0L;
		HLockHi(inoutTextHandle);
		for (ptr = *inoutTextHandle; (*ptr != EOF) && (count < capacityLength); ++ptr)
		{
			if ((error = FSRead(inFileRefNum, &readHowMany, ptr)) == eofErr) *ptr = EOF;
			else
			{
				if (readHowMany != 1) readHowMany = 1; // reset count (read failed)
				else ++count;
			}
		}
		result = count;
		HSetState(inoutTextHandle, hState);
	}
	
	return result;
}// TextToHandle


/*!
Determines if a volume supports the desktop database.
If any problems occur, an error code is returned.

(2.6)
*/
OSStatus
FileUtilities_VolHasDesktopDB	(short			inVRefNum,
								 Boolean*		outHasDesktop)
{
	HParamBlockRec			pb;
	GetVolParmsInfoBuffer	info;
	OSStatus				result = noErr;
	
	
	pb.ioParam.ioCompletion = nullptr;
	pb.ioParam.ioNamePtr = nullptr;
	pb.ioParam.ioVRefNum = inVRefNum;
	pb.ioParam.ioBuffer = (Ptr)&info;
	pb.ioParam.ioReqCount = sizeof(info);
	result = PBHGetVolParmsSync(&pb);
	*outHasDesktop = ((result == noErr) && ((info.vMAttrib & (1L << bHasDesktopMgr)) != 0));
	
	return result;
}// VolHasDesktopDB


#pragma mark Internal Methods

/*!
This convenient routine can be used to perform a
synchronous search of a directory for which you
have a file specification, returning an array of
file specification records for each file in that
directory.

(3.0)
*/
static OSStatus
getFilesInDirectory		(FSSpec const*		inDirectorySpecPtr,
						 Boolean			inRequireSignatures,
						 OSType				inDesiredTypeSignature,
						 OSType				inDesiredCreatorSignature,
						 FSSpec*			inFileArrayStoragePtr,
						 UInt32*			inoutFileMaxCountFileActualCountPtr)
{
	return threadedGetFilesInDirectory(inDirectorySpecPtr, inRequireSignatures, inDesiredTypeSignature,
										inDesiredCreatorSignature, inFileArrayStoragePtr, inoutFileMaxCountFileActualCountPtr,
										0/* maximum delay - 0 means never yield to threads */);
}// getFilesInDirectory


/*!
This is an I/O completion routine for the threaded
get-files-in-directory routines.  It disposes of
the UPP properly and clears the in-progress flag.

(3.0)
*/
static pascal void
ioCompletionGetFiles		(ParmBlkPtr		inParamBlockPtr)
{
	//Console_WriteValue("done, result", inParamBlockPtr->fileParam.ioResult);
	DisposeIOCompletionUPP(inParamBlockPtr->fileParam.ioCompletion), inParamBlockPtr->fileParam.ioCompletion = nullptr;
	gGetFilesInProgress = false;
}// ioCompletionGetFiles


/*!
This convenient routine can be used to perform a
synchronous or asynchronous search of a directory
for which you have a file specification, returning
an array of file specification records for each
file in that directory (or a subset of them that
corresponds to certain criteria).

To yield to threads (highly recommended), pass a
nonzero value for "inMaximumDelayTime".  The
synchronous search will block for at most that
much time before yielding to a thread.  This
method will not return until the search finishes,
but many threads may be yielded to before then.

(3.0)
*/
static OSStatus
threadedGetFilesInDirectory		(FSSpec const*		inDirectorySpecPtr,
								 Boolean			inRequireSignatures,
								 OSType				inDesiredTypeSignature,
								 OSType				inDesiredCreatorSignature,
								 FSSpec*			inFileArrayStoragePtr,
								 UInt32*			inoutFileMaxCountFileActualCountPtr,
								 UInt32				inMaximumDelayTime)
{
	OSStatus		result = noErr;
	CSParam*		paramBlockPtr = (CSParam*)Memory_NewPtr(sizeof(CSParam));
	
	
	if (paramBlockPtr == nullptr) result = memFullErr;
	else
	{
		enum
		{
			kSearchPerformanceEnhancingBufferSizeInK = 16
		};
		CInfoPBPtr		lowerRangePtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(CInfoPBRec)), CInfoPBPtr),
						upperRangePtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(CInfoPBRec)), CInfoPBPtr);
		
		
		if ((lowerRangePtr == nullptr) || (upperRangePtr == nullptr)) result = memFullErr;
		
		if (result == noErr)
		{
			Ptr		bufferPtr = Memory_NewPtr(INTEGER_KILOBYTES(kSearchPerformanceEnhancingBufferSizeInK));
			
			
			if (bufferPtr != nullptr)
			{
				// Perform a basic search, for all files, allocating (if possible) a
				// fairly generous “helper buffer” to make this as quick as possible.
				paramBlockPtr->ioCompletion = nullptr;
				paramBlockPtr->ioNamePtr = nullptr;
				paramBlockPtr->ioVRefNum = inDirectorySpecPtr->vRefNum;
				paramBlockPtr->ioMatchPtr = inFileArrayStoragePtr;
				if (inoutFileMaxCountFileActualCountPtr != nullptr)
				{
					paramBlockPtr->ioReqMatchCount = *inoutFileMaxCountFileActualCountPtr;
				}
				else result = memPCErr;
				
				paramBlockPtr->ioSearchBits = fsSBFlParID;
				if (inRequireSignatures) paramBlockPtr->ioSearchBits |= fsSBFlFndrInfo;
				
				// set up the criteria for “matching” files
				{
					lowerRangePtr->hFileInfo.ioNamePtr =
						upperRangePtr->hFileInfo.ioNamePtr = nullptr;
					if (inRequireSignatures)
					{
						lowerRangePtr->hFileInfo.ioFlFndrInfo.fdType = inDesiredTypeSignature;
						lowerRangePtr->hFileInfo.ioFlFndrInfo.fdCreator = inDesiredCreatorSignature;
					}
					else
					{
						lowerRangePtr->hFileInfo.ioFlFndrInfo.fdType =
							lowerRangePtr->hFileInfo.ioFlFndrInfo.fdCreator = STATIC_CAST(0xFFFFFFFF, OSType);
					}
					upperRangePtr->hFileInfo.ioFlFndrInfo.fdType =
						upperRangePtr->hFileInfo.ioFlFndrInfo.fdCreator = STATIC_CAST(0xFFFFFFFF, OSType);
					lowerRangePtr->hFileInfo.ioFlParID =
						upperRangePtr->hFileInfo.ioFlParID = FileUtilities_ReturnDirectoryIDFromFSSpec(inDirectorySpecPtr);
					upperRangePtr->hFileInfo.ioFlFndrInfo.fdFlags = 0;
					upperRangePtr->hFileInfo.ioFlFndrInfo.fdLocation.h = 0;
					upperRangePtr->hFileInfo.ioFlFndrInfo.fdLocation.v = 0;
					upperRangePtr->hFileInfo.ioFlFndrInfo.fdFldr = 0;
					
					// set up the search parameters to use the data just defined
					paramBlockPtr->ioSearchInfo1 = lowerRangePtr;
					paramBlockPtr->ioSearchInfo2 = upperRangePtr;
				}
				paramBlockPtr->ioSearchTime = inMaximumDelayTime;
				
				paramBlockPtr->ioCatPosition.initialize = 0L;
				
				paramBlockPtr->ioOptBuffer = bufferPtr;
				if (paramBlockPtr->ioOptBuffer != nullptr)
				{
					paramBlockPtr->ioOptBufSize = GetPtrSize(bufferPtr);
				}
				
				if (result == noErr)
				{
					// PBCatSearchSync() returns "noErr" if the time expires or if
					// the search hits the maximum number of files
					while ((result == noErr) && (!paramBlockPtr->ioActMatchCount))
					{
						result = PBCatSearchSync(paramBlockPtr);
						
						// maximum number of files found?
						if ((result == noErr) &&
							(STATIC_CAST(paramBlockPtr->ioActMatchCount, UInt32) >= *inoutFileMaxCountFileActualCountPtr))
						{
							break;
						}
						
						// if the volume changes in mid-search, restart the search, to be safe
						if (result == catChangedErr)
						{
							result = noErr;
							paramBlockPtr->ioCatPosition.initialize = 0L;
							continue;
						}
						
						// yield to another thread before trying to continue the search
						//if (inMaximumDelayTime) GenericThreads_YieldToAnother();
					}
					
					// now do something with the files found, if no errors occurred
					if ((result == eofErr) || ((result == noErr) && (paramBlockPtr->ioActMatchCount > 0)))
					{
						if (result == eofErr) result = noErr; // having to search the entire volume is not an error
						if (inoutFileMaxCountFileActualCountPtr != nullptr)
						{
							*inoutFileMaxCountFileActualCountPtr = paramBlockPtr->ioActMatchCount;
						}
						else result = memPCErr;
					}
				}
				Memory_DisposePtr(&bufferPtr);
			}
			else result = memFullErr;
		}
		
		Memory_DisposePtr(REINTERPRET_CAST(&paramBlockPtr, Ptr*));
		Memory_DisposePtr(REINTERPRET_CAST(&lowerRangePtr, Ptr*));
		Memory_DisposePtr(REINTERPRET_CAST(&upperRangePtr, Ptr*));
	}
	
	return result;
}// threadedGetFilesInDirectory


/*!
This convenient routine can be used to perform a
synchronous or asynchronous search of a directory
for which you have a file specification, returning
an array of file specification records for each
file in that directory (or a subset of them that
corresponds to certain criteria).

This uses an alternate method for scanning a folder
that might be faster.  However, it does not seem to
function as expected under Carbon!

To yield to threads (highly recommended), pass a
nonzero value for "inMaximumDelayTime".  The
synchronous search will block for at most that
much time before yielding to a thread.  This
method will not return until the search finishes,
but many threads may be yielded to before then.

(3.0)
*/
static OSStatus
threadedGetFilesInDirectory2	(FSSpec const*	inDirectorySpecPtr,
								 Boolean		inRequireSignatures,
								 OSType			inDesiredTypeSignature,
								 OSType			inDesiredCreatorSignature,
								 FSSpec*		inFileArrayStoragePtr,
								 UInt32*		inoutFileMaxCountFileActualCountPtr,
								 UInt32			inMaximumDelayTime)
{
	OSStatus			result = noErr;
	HParamBlockRec*		paramBlockPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(HParamBlockRec)), HParamBlockRec*);
	
	
	if (paramBlockPtr == nullptr) result = memFullErr;
	else if (inoutFileMaxCountFileActualCountPtr == nullptr) result = memPCErr;
	else
	{
		register UInt16		i = 0;
		UInt32				maximum = *inoutFileMaxCountFileActualCountPtr;
		UInt32				originalTicks = TickCount(); // save time to determine approximate timeout
		SInt32				directoryID = FileUtilities_ReturnDirectoryIDFromFSSpec(inDirectorySpecPtr);
		
		
		*inoutFileMaxCountFileActualCountPtr = 0; // no files initially
		for (i = 1; (i < maximum) && (result == noErr); ++i)
		{
			paramBlockPtr->fileParam.ioCompletion = NewIOCompletionUPP(ioCompletionGetFiles);
			paramBlockPtr->fileParam.ioVRefNum = inDirectorySpecPtr->vRefNum;
			paramBlockPtr->fileParam.ioFVersNum = 0;
			paramBlockPtr->fileParam.ioFDirIndex = i;
			paramBlockPtr->fileParam.ioDirID = directoryID;
			paramBlockPtr->fileParam.ioNamePtr = inFileArrayStoragePtr[*inoutFileMaxCountFileActualCountPtr].name;
			
			// asynchronously call get-info; the I/O completion routine will
			// be invoked eventually, which will clear the flag that is set
			// here; otherwise, yield to other threads to pass the time
			gGetFilesInProgress = true;
			//result = PBHGetFInfoAsync(paramBlockPtr);
			//do { GenericThreads_YieldToAnother(); } while (gGetFilesInProgress); // flag is cleared by I/O completion routine
			result = PBHGetFInfoSync(paramBlockPtr);
			
			if (result == noErr)
			{
				// file successfully found; perform checks
				Boolean		accepted = true;
				
				
				// why isn’t this getting set properly by PBHGetFInfoSync()? (also see FSpGetFInfo below)
				inFileArrayStoragePtr[*inoutFileMaxCountFileActualCountPtr].parID = directoryID;
				
				// tmp
				//Console_WriteValueFile("found file", &inFileArrayStoragePtr[*inoutFileMaxCountFileActualCountPtr]);
				
				if (inRequireSignatures)
				{
					// user wants only specific file type and creator
					if (FSpGetFInfo(&inFileArrayStoragePtr[*inoutFileMaxCountFileActualCountPtr],
									&paramBlockPtr->fileParam.ioFlFndrInfo) == noErr)
									// explicit FSpGetFInfo() call done because PBHGetFInfoSync() doesn’t seem
									// to be returning the correct file specification (and therefore the wrong FInfo)
					{
						accepted = ((paramBlockPtr->fileParam.ioFlFndrInfo.fdType == inDesiredTypeSignature) &&
									(paramBlockPtr->fileParam.ioFlFndrInfo.fdCreator == inDesiredCreatorSignature));
					}
				}
				
				if (accepted)
				{
					// this is one of the files desired; increment the counter to remember the file
					++(*inoutFileMaxCountFileActualCountPtr);
				}
			}
			
			// yield to another thread before trying to continue the search
			//if (inMaximumDelayTime) GenericThreads_YieldToAnother();
			
			// if the time is up, abort the search
			if ((TickCount() - originalTicks) > inMaximumDelayTime) result = userCanceledErr;
		}
		if (result == fnfErr) result = noErr; // file-not-found isn’t an error in this case
		
		Memory_DisposePtr(REINTERPRET_CAST(&paramBlockPtr, Ptr*));
	}
	
	return result;
}// threadedGetFilesInDirectory2

// BELOW IS REQUIRED NEWLINE TO END FILE
