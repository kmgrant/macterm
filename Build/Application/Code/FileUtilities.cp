/*!	\file FileUtilities.cp
	\brief Useful methods for dealing with files.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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

// BELOW IS REQUIRED NEWLINE TO END FILE
