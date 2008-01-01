/*!	\file FileUtilities.h
	\brief Useful methods for dealing with files.
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

#ifndef __FILEUTILITIES__
#define __FILEUTILITIES__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef SInt16 FileUtilitiesDate;
enum
{
	kFileUtilitiesDateOfModification = 0,
	kFileUtilitiesDateOfCreation = 1,
	kFileUtilitiesDateOfBackup = 2
};



#pragma mark Public Methods

//!\name File, Directory and Volume Routines
//@{

OSStatus
	FileUtilities_ConvertFSRefToFSSpec				(FSRef const&		inFSRef,
													 FSSpec&			outFSSpec);

OSStatus
	FileUtilities_EnsureOriginalFile				(FSSpec*			inoutFSSpecPtr);

OSStatus
	FileUtilities_GetAllFilesInDirectory			(FSSpec const*		inDirectorySpecPtr,
													 FSSpec*			inFileArrayStoragePtr,
													 UInt32*			inoutFileMaxCountFileActualCountPtr);

OSStatus
	FileUtilities_GetTypedFilesInDirectory			(FSSpec const*		inDirectorySpecPtr,
													 OSType				inDesiredTypeSignature,
													 OSType				inDesiredCreatorSignature,
													 FSSpec*			inFileArrayStoragePtr,
													 UInt32*			inoutFileMaxCountFileActualCountPtr);

void
	FileUtilities_GetDirectoryName					(short				inVRefNum,
													 long				inDirID,
													 Str32				outName);

OSStatus
	FileUtilities_GetIndVolume						(short				inIndex,
													 short*				outVRefNum);

OSStatus
	FileUtilities_GetLastModDateTime				(FSSpec*			inFSSpecPtr,
													 unsigned long*		outLastModDateTime);

OSStatus
	FileUtilities_GetPathnameFromDirectoryID		(long				inDirectoryID,
													 short				inVolumeReferenceNumber,
													 StringPtr			outFullPathNameString);

OSStatus
	FileUtilities_GetPathnameFromFSSpec				(FSSpec const*		inDirectorySpecPtr,
													 StringPtr			outFullPathNameString,
													 Boolean			inIsDirectory);

OSStatus
	FileUtilities_GetPOSIXPathnameFromFSSpec		(FSSpec const*		inDirectorySpecPtr,
													 StringPtr			outFullPathNameString,
													 Boolean			inIsDirectory);

OSStatus
	FileUtilities_LaunchApplicationFromFSSpec		(FSSpec*			inFSSpecPtr);

OSStatus
	FileUtilities_OpenDocument						(FSRef const&		inFSRef);

OSStatus
	FileUtilities_OpenDocument						(FSSpec const*		inFSSpecPtr);

OSStatus
	FileUtilities_OpenDocuments						(AEDescList const&	inList);

OSStatus
	FileUtilities_PersistentCreate					(FSSpec*			inoutFSSpecPtr,
													 OSType				inCreator,
													 OSType				inType,
													 ScriptCode			inScriptTag);

unsigned long
	FileUtilities_ReturnDirectoryDateFromFSSpec		(FSSpec const*		inFSSpecPtr,
													 FileUtilitiesDate	inWhichDate);

long
	FileUtilities_ReturnDirectoryIDFromFSSpec		(FSSpec const*		inFSSpecPtr);

short
	FileUtilities_ReturnVolumeRefNumberForName		(Str32				inVolumeName);

SInt32
	FileUtilities_TextToHandle						(SInt16				inFileRefNum,
													 Handle				inoutTextHandle);

OSStatus
	FileUtilities_VolHasDesktopDB					(short				inVRefNum,
													 Boolean*			outHasDesktop);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
