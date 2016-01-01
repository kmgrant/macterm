/*!	\file FileSelectionDialogs.cp
	\brief Convenient front ends to Navigation Services.
*/
/*###############################################################

	Interface Library
	© 1998-2016 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
    This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.

    You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <FileSelectionDialogs.h>
#include <UniversalDefines.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <CocoaBasic.h>
#include <Localization.h>
#include <MemoryBlocks.h>



#pragma mark Public Methods

/*!
Use this method after messing with a file to
complete the save by doing any necessary
translation and then disposing of the specified
reply record.

(1.0)
*/
OSStatus
FileSelectionDialogs_CompleteSave	(NavReplyRecord*	inoutReplyPtr)
{
	OSStatus	result = noErr;
	
	
	if (inoutReplyPtr->validRecord)
	{
		result = NavCompleteSave(inoutReplyPtr, kNavTranslateInPlace);
	}
	UNUSED_RETURN(OSStatus)NavDisposeReply(inoutReplyPtr);
	
	return result;
}// CompleteSave


/*!
Within a Navigation Services callback for a window-modal or
modeless Carbon-based save dialog, retrieve the reply record
and use this routine to automatically create a reference to
the user’s save file.  If necessary, the file is created; in
this case, the specified creator and type codes are also
assigned to the new file automatically (but if the file
already exists, its type and creator are not touched).  This
routine uses Unicode filenames.

A temporary file is also created, into which you can write
the file’s data before saving it.  This is just safer; you
can use FSExchangeObjects() after writing data to the temporary
file, in order to “save” the data where the user wants it, and
then use FSDeleteObject() to delete the user’s original file
that is then junk.

Carbon only - used for modeless or window-modal save dialogs.

Returns "noErr" only if the file and its temporary were created
or found successfully; if so, "outUserSaveFile" and
"outTemporaryFile" will be valid.

(3.0)
*/
OSStatus
FileSelectionDialogs_CreateOrFindUserSaveFile	(NavReplyRecord const&	inReply,
												 OSType					inNewFileCreator,
												 OSType					inNewFileType,
												 FSRef&					outUserSaveFile,
												 FSRef&					outTemporaryFile)
{
	// when using the Carbon APIs, the selection is a one-item list
	// containing a DIRECTORY, that combines with "inReply.saveFileName"
	// to produce the complete location and name of the saved file
	CFIndex		fileNameLength = CFStringGetLength(inReply.saveFileName);
	UniChar*	buffer = REINTERPRET_CAST(Memory_NewPtr(fileNameLength * sizeof(UniChar)), UniChar*);
	OSStatus	result = noErr;
	
	
	if (nullptr == buffer) result = memFullErr;
	else
	{
		FSRef		saveDirectory;
		AEDesc 		resultDesc;
		AEKeyword	keyword;
		
		
		// get a Unicode representation of the file name
		CFStringGetCharacters(inReply.saveFileName, CFRangeMake(0, fileNameLength), buffer);
		
		// figure out what directory the file belongs in
		result = AEGetNthDesc(&inReply.selection, 1, typeFSRef, &keyword, &resultDesc);
		if (noErr == result)
		{
			result = AEGetDescData(&resultDesc, &saveDirectory, sizeof(saveDirectory));
			AEDisposeDesc(&resultDesc);
		}
		
		if (noErr == result)
		{
			FSRef	temporaryDirectory;
			
			
			result = FSFindFolder(kUserDomain, kTemporaryFolderType, kCreateFolder, &temporaryDirectory);
			if (noErr != result)
			{
				// unable to find the Temporary Items directory in the expected place;
				// try finding one on the disk where the user wants to actually save
				FSCatalogInfo	saveDirectoryInfo;
				
				
				result = FSGetCatalogInfo(&saveDirectory, kFSCatInfoVolume, &saveDirectoryInfo, nullptr/* name */,
											nullptr/* spec record */, nullptr/* parent directory */);
				if (noErr == result)
				{
					result = FSFindFolder(saveDirectoryInfo.volume, kTemporaryFolderType, kCreateFolder,
											&temporaryDirectory);
				}
			}
			
			if (noErr == result)
			{
				// select a location for a temporary file
				result = FSCreateFileUnicode(&temporaryDirectory, fileNameLength, buffer, 0/* catalog info bits */,
												nullptr/* catalog info */, &outTemporaryFile, nullptr/* spec record */);
			}
		}
		
		if (noErr == result)
		{
			if (inReply.replacing)
			{
				// file exists; overwrite, but first get a reference to it
				result = FSMakeFSRefUnicode(&saveDirectory, fileNameLength, buffer,
											CFStringGetFastestEncoding(inReply.saveFileName), &outUserSaveFile);
			}
			else
			{
				// file did not previously exist; create it
				result = FSCreateFileUnicode(&saveDirectory, fileNameLength, buffer, 0/* catalog info bits */,
												nullptr/* catalog info */, &outUserSaveFile, nullptr/* spec record */);
				
				// now set the old creator and type info; if this fails it
				// probably just means the file system is not HFS-based
				if (noErr == result)
				{
					CFURLRef	absolutePathURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &outUserSaveFile);
					
					
					if (nullptr != absolutePathURL)
					{
						CFRetainRelease		absolutePathCFString(CFURLCopyFileSystemPath(absolutePathURL, kCFURLPOSIXPathStyle),
																	true/* is retained */);
						
						
						UNUSED_RETURN(Boolean)CocoaBasic_SetFileTypeCreator(absolutePathCFString.returnCFStringRef(), inNewFileType, inNewFileCreator);
						CFRelease(absolutePathURL), absolutePathURL = nullptr;
					}
				}
			}
		}
		
		Memory_DisposePtr(REINTERPRET_CAST(&buffer, Ptr*));
	}
	return result;
}// CreateOrFindUserSaveFile

// BELOW IS REQUIRED NEWLINE TO END FILE
