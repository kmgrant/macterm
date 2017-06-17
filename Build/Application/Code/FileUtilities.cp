/*!	\file FileUtilities.cp
	\brief Useful methods for dealing with files.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#include "FileUtilities.h"
#include <UniversalDefines.h>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>
#include <MemoryBlocks.h>

// application includes
#include "AppleEventUtilities.h"
#include "ConstantsRegistry.h"
#include "Folder.h"
#include "RecordAE.h"



#pragma mark Public Methods

/*!
Use this convenient routine to fire an Apple Event
back to MacTerm (that is hence recordable) to “open”
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
	// OS data structure and then send it back to MacTerm as an “open documents” event.
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
Sends the entire given list of files back to the
current application in an “open documents” event,
which will cause the files to be opened (as well
as having the action recorded into scripts, if
recording is in progress).

Currently, the list must contain FSRef entries.

(3.1)
*/
OSStatus
FileUtilities_OpenDocuments		(AEDescList const&	inList)
{
	OSStatus	result = noErr;
	AppleEvent	openDocumentsEvent;
	
	
	// send the list back to MacTerm as an “open documents” event
	result = AppleEventUtilities_InitAEDesc(&openDocumentsEvent);
	if (noErr == result)
	{
		result = RecordAE_CreateRecordableAppleEvent('aevt'/* a.k.a. kCoreEventClass */, kAEOpenDocuments,
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
Creates a unique new temporary file, and opens it for
read and write, returning its reference number.  When
finished, call FSCloseFork() on the file.  Returns a
positive number, and defines "outTemporaryFile", only
if successful.

It is your responsibility to subsequently delete the
created temporary file.  (Worst case, the system does
this on its own, or leaves the item in the Trash for
recovery by the user on the next boot.)

This function is not completely secure because a race
condition exists between the time a file is opened,
and the time its "outTemporaryFile" structure is
subsequently used.  You are encouraged to write and
read from the file while it is open, and to never
use it again when it is closed.

A better approach to creating temporary files is the
mkstemp() system call, however this returns Unix file
descriptors and cannot be used with the File Manager.

(4.0)
*/
SInt16
FileUtilities_OpenTemporaryFile		(FSRef&		outTemporaryFile)
{
	SInt16		result = -1;
	FSRef		temporaryFilesFolder;
	OSStatus	error = noErr;
	
	
	error = Folder_GetFSRef(kFolder_RefMacTemporaryItems, temporaryFilesFolder);
	if (noErr != error)
	{
		Console_Warning(Console_WriteValue, "failed to find folder for temporary files, error", error);
	}
	else
	{
		UniChar				fileName[32/* arbitrary! */];
		CFRetainRelease		buffer(CFStringCreateMutableWithExternalCharactersNoCopy
									(kCFAllocatorDefault, fileName, 0/* size */, sizeof(fileName) / sizeof(UniChar)/* capacity */,
										kCFAllocatorNull/* deallocator */), CFRetainRelease::kAlreadyRetained);
		
		
		if (false == buffer.exists())
		{
			Console_Warning(Console_WriteLine, "failed to create temporary file, out of memory");
		}
		else
		{
			CFMutableStringRef const	kBufferAsCFString = buffer.returnCFMutableStringRef();
			unsigned int				count = 0;
			
			
			do
			{
				CFStringDelete(kBufferAsCFString, CFRangeMake(0, CFStringGetLength(kBufferAsCFString)));
				CFStringAppendFormat(kBufferAsCFString, nullptr/* options */,
										CFSTR("macterm%u.tmp")/* format */, count);
				//Console_WriteValueCFString("attempting to create temporary file with name", kBufferAsCFString); // debug
				error = FSCreateFileUnicode(&temporaryFilesFolder, CFStringGetLength(kBufferAsCFString), fileName,
											kFSCatInfoNone, nullptr/* catalog info */, &outTemporaryFile, nullptr/* spec record */);
				++count;
			} while ((errFSForkExists == error) || (dupFNErr == error));
			
			if (noErr != error)
			{
				Console_Warning(Console_WriteValue, "failed to create temporary file, error", error);
			}
			else
			{
				//Console_WriteValueCFString("created temporary file with name", kBufferAsCFString); // debug
				error = FSOpenFork(&outTemporaryFile, 0/* name length */, nullptr/* name */, fsWrPerm, &result);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to open temporary file, error", error);
				}
				else
				{
					// success!
					UNUSED_RETURN(OSStatus)FSSetForkSize(result, fsFromStart, 0);
				}
			}
		}
	}
	return result;
}// OpenTemporaryFile

// BELOW IS REQUIRED NEWLINE TO END FILE
