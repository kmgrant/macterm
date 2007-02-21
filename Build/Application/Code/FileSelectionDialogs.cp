/*###############################################################

	FileSelectionDialogs.cp
	
	Interface Library 1.3
	© 1998-2006 by Kevin Grant
	
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

#include "UniversalDefines.h"

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AppleEventUtilities.h>
#include <Embedding.h>
#include <FileSelectionDialogs.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>



#pragma mark Internal Method Prototypes

static OSStatus				getAEDescData			(AEDesc const*, DescType*, void*, ByteCount, ByteCount*);
static NavTypeListHandle	newOpenHandle			(OSType, UInt32, OSType[]);
static OSStatus				sendOpenAE				(AEDescList);
static void					setUpWTitleProcessName	(NavDialogOptions*, ConstStringPtr);



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
	result = NavDisposeReply(inoutReplyPtr);
	
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
				FSSpec	fileSpec;
				
				
				result = FSCreateFileUnicode(&saveDirectory, fileNameLength, buffer, 0/* catalog info bits */,
												nullptr/* catalog info */, &outUserSaveFile, &fileSpec);
				
				// now set the old creator and type info; if this fails it
				// probably just means the file system is not HFS-based
				if (noErr == result)
				{
					FileInfo	fileInfo;
					
					
					// retrieve current settings so only some things change
					if (noErr == FSpGetFInfo(&fileSpec, REINTERPRET_CAST(&fileInfo, FInfo*)))
					{
						fileInfo.fileType = inNewFileType;
						fileInfo.fileCreator = inNewFileCreator;
						(OSStatus)FSpSetFInfo(&fileSpec, REINTERPRET_CAST(&fileInfo, FInfo const*));
					}
				}
			}
		}
		
		Memory_DisposePtr(REINTERPRET_CAST(&buffer, Ptr*));
	}
	return result;
}// CreateOrFindUserSaveFile


/*!
This routine is useful when you need to ask the
user to select a directory.

(1.0)
*/
OSStatus
FileSelectionDialogs_GetDirectory	(ConstStr255Param		inPromptMessage,
									 ConstStr255Param		inDialogTitle,
									 UInt32					inPrefKey,
									 NavDialogOptionFlags	inFlags,
									 NavEventProcPtr		inEventProc,
									 FSSpec*				outFileSpecPtr)
{	
	NavReplyRecord		theReply;
	NavDialogOptions	dialogOptions;
	OSStatus			result = noErr;
	NavEventUPP			eventUPP = NewNavEventUPP(inEventProc);
	
	
	result = NavGetDefaultDialogOptions(&dialogOptions);
	dialogOptions.dialogOptionFlags = 0;
	dialogOptions.dialogOptionFlags |= inFlags;
	
	PLstrcpy(dialogOptions.message, inPromptMessage);
	setUpWTitleProcessName(&dialogOptions, inDialogTitle);
	dialogOptions.preferenceKey = inPrefKey;
	
	// display “choose directory” dialog
	Embedding_DeactivateFrontmostWindow();
	result = NavChooseFolder(nullptr, &theReply, &dialogOptions, eventUPP,
								nullptr, (NavCallBackUserData)nullptr);
	Embedding_RestoreFrontmostWindow();
	DisposeNavEventUPP(eventUPP), eventUPP = nullptr;;
	
	// NOTE: for some reason, *aliases* to directories result in the PARENT folder...how is this happening?...
	if ((theReply.validRecord) && (noErr == result))
	{
		// grab the target FSSpec from the AEDesc
		AEDesc		resultDesc;
		
		
		result = AECoerceDesc(&(theReply.selection), typeFSS, &resultDesc);
		if (noErr == result)
		{
			result = getAEDescData(&resultDesc, nullptr/* result type */, outFileSpecPtr, sizeof(FSSpec),
									nullptr/* actual size */);
		}
		AEDisposeDesc(&resultDesc);
		
		if (noErr == result) result = NavDisposeReply(&theReply);
		else (OSStatus)NavDisposeReply(&theReply); // preserves previous error
	}
	return result;
}// GetDirectory


/*!
Deprecated in favor of FileSelectionDialogs_GetFiles().

(1.0)
*/
OSStatus
FileSelectionDialogs_GetFile	(ConstStr255Param		inPromptMessage,
								 ConstStr255Param		inDialogTitle,
								 OSType					inApplicationSignature,
								 UInt32					inPrefKey,
								 NavDialogOptionFlags	inFlags,
								 UInt32					inNumTypes,
								 OSType					inTypeList[],
								 NavEventProcPtr		inEventProc,
								 FSSpec*				inoutFileSpecPtr,
								 OSType*				inoutFileType,
								 FileInfo*				outFileInfo)
{	
	NavReplyRecord		theReply;
	NavDialogOptions	dialogOptions;
	NavTypeListHandle	openList = nullptr;
	NavEventUPP			eventUPP = NewNavEventUPP(inEventProc);
	OSStatus			result = noErr;
	
	
	result = NavGetDefaultDialogOptions(&dialogOptions);
	dialogOptions.dialogOptionFlags = 0;
	dialogOptions.dialogOptionFlags |= inFlags;
	
	// add customized settings
	PLstrcpy(dialogOptions.message, inPromptMessage);
	setUpWTitleProcessName(&dialogOptions, inDialogTitle);
	dialogOptions.preferenceKey = inPrefKey;
	
	// make sure file selection mode is right
	if (nullptr != inoutFileSpecPtr)
	{
		if (0 != (dialogOptions.dialogOptionFlags & kNavAllowMultipleFiles))
		{
			dialogOptions.dialogOptionFlags -= kNavAllowMultipleFiles;
		}
	}
	else
	{
		if (0 == (dialogOptions.dialogOptionFlags & kNavAllowMultipleFiles))
		{
			dialogOptions.dialogOptionFlags |= kNavAllowMultipleFiles;
		}
	}
	
	// restrict displayed file types if a list of types is given
	if (nullptr != inTypeList)
	{
		openList = newOpenHandle(inApplicationSignature, inNumTypes, inTypeList);
		if (openList) HLock((Handle)openList);
	}
	
	Embedding_DeactivateFrontmostWindow();
	result = NavGetFile(nullptr, &theReply, &dialogOptions, eventUPP, nullptr,
						nullptr, openList, nullptr);
	Embedding_RestoreFrontmostWindow();
	DisposeNavEventUPP(eventUPP), eventUPP = nullptr;;
	
	if (memFullErr == result) result = userCanceledErr;
	
	if ((noErr == result) && (theReply.validRecord))
	{
		if (nullptr != inoutFileSpecPtr)
		{
			AEDesc 		resultDesc;
			AEKeyword	keyword;
			
			
			// single-file opening
			result = AEGetNthDesc(&theReply.selection, 1, typeFSS, &keyword, &resultDesc);
			if (noErr == result)
			{
				(OSStatus)AEGetDescData(&resultDesc, inoutFileSpecPtr, sizeof(FSSpec));
			}
			
			result = FSpGetFInfo(inoutFileSpecPtr, (FInfo*)outFileInfo);
			if (noErr == result)
			{
				if (nullptr != inoutFileType) *inoutFileType = outFileInfo->fileType;
			}
		}
		else
		{
			// multiple files opening: use AppleEvents
			result = sendOpenAE(theReply.selection);
		}
		
		NavDisposeReply(&theReply);
	}
	
	if (nullptr != openList)
	{
		HUnlock((Handle)openList);
		DisposeHandle((Handle)openList);
		openList = nullptr;
	}
	
	return result;
}// GetFile


/*!
This routine is adequate for 99% of “open file”
dialog needs on Mac OS 9 or Mac OS X.

All selected files are automatically put into
Apple Events and sent as “open document” events
to the current application.  If you want to allow
multiple files to be opened at once, be sure to
include "kNavAllowMultipleFiles" in "inFlags".

(1.4)
*/
OSStatus
FileSelectionDialogs_GetFiles	(CFStringRef			inPromptMessage,
								 CFStringRef			inDialogTitle,
								 OSType					inApplicationSignature,
								 UInt32					inPrefKey,
								 NavDialogOptionFlags	inFlags,
								 UInt32					inNumTypes,
								 OSType					inTypeList[],
								 NavEventProcPtr		inEventProc)
{	
	NavDialogCreationOptions	dialogOptions;
	NavTypeListHandle			openList = nullptr;
	NavDialogRef				navigationServicesDialog = nullptr;
	OSStatus					result = noErr;
	
	
	result = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (noErr == result)
	{
		dialogOptions.optionFlags = inFlags;
		Localization_GetCurrentApplicationNameAsCFString(&dialogOptions.clientName);
		dialogOptions.preferenceKey = inPrefKey;
		
		dialogOptions.message = inPromptMessage;
		dialogOptions.windowTitle = inDialogTitle;
	}
	
	// restrict displayed file types if a list of types is given
	if (nullptr != inTypeList)
	{
		openList = newOpenHandle(inApplicationSignature, inNumTypes, inTypeList);
		if (openList) HLock(REINTERPRET_CAST(openList, Handle));
	}
	
	result = NavCreateGetFileDialog(&dialogOptions, openList, inEventProc/* event routine */,
									nullptr/* preview routine */, nullptr/* filter routine */,
									nullptr/* client data */, &navigationServicesDialog);
	
	if (noErr == result)
	{
		// display the dialog; since this is not a sheet, the call
		// will not return until the user is finished with the dialog
		result = NavDialogRun(navigationServicesDialog);
		if (noErr == result)
		{
			NavReplyRecord		replyInfo;
			
			
			// figure out what the user did
			result = NavDialogGetReply(navigationServicesDialog, &replyInfo);
			if (noErr == result)
			{
				sendOpenAE(replyInfo.selection);
				NavDisposeReply(&replyInfo);
			}
		}
		
		NavDialogDispose(navigationServicesDialog), navigationServicesDialog = nullptr;
	}
	
	if (nullptr != openList)
	{
		HUnlock(REINTERPRET_CAST(openList, Handle));
		DisposeHandle(REINTERPRET_CAST(openList, Handle)), openList = nullptr;
	}
	
	return result;
}// GetFiles


/*!
Use this method to get access to zero or more
file specification records that are embedded
in an Apple Event description.

(1.0)
*/
OSStatus
FileSelectionDialogs_GetFSSpecArrayFromAEDesc	(AEDesc const*		inAEDescPtr,
												 void*				outFSSpecArray,
												 long*				inoutFSSpecArrayLengthPtr)
{
	FSSpecArrayPtr	fileArray = (FSSpecArrayPtr)outFSSpecArray;
	OSStatus		result = noErr;
	
	
	fileArray = (FSSpecArrayPtr)Memory_NewPtr(sizeof(FSSpec) * (*inoutFSSpecArrayLengthPtr));
	if (noErr == MemError())
	{
		register SInt16		i = 0;
		
		
		*(void**)outFSSpecArray = fileArray;
		
		for (i = 1; i <= (*inoutFSSpecArrayLengthPtr); i++)
		{
			AEDesc	desc;
			FSSpec	spec;
			
			
			result = AEGetNthDesc(inAEDescPtr, i, typeFSS, nullptr, &desc);
			if (noErr == result)
			{
				result = FileSelectionDialogs_GetFSSpecFromAEDesc(&desc, &spec);
				*fileArray++ = spec;
			}
			else
			{
				break;
			}
		}
	}
	return result;
}// GetFSSpecArrayFromAEDesc


/*!
Use this method to get access to a file specification
record that is embedded in an Apple Event description,
in a Carbon-compliant manner.

(1.0)
*/
OSStatus
FileSelectionDialogs_GetFSSpecFromAEDesc	(AEDesc const*		inAEDescPtr,
											 FSSpec*			outFSSpecPtr)
{
	AEDesc		tempDesc = { typeNull, nullptr };
	OSStatus	result = noErr;
	
	
	if (typeFSS == inAEDescPtr->descriptorType)
	{
		(OSStatus)AEGetDescData(inAEDescPtr, outFSSpecPtr, sizeof(FSSpec));
	}
	else
	{
		result = AECoerceDesc(inAEDescPtr, typeFSS, &tempDesc);
		if (noErr == result)
		{
			(OSStatus)AEGetDescData(&tempDesc, outFSSpecPtr, sizeof(FSSpec));
		}
	}
	
	AEDisposeDesc(&tempDesc);
	return result;
}// GetFSSpecFromAEDesc


/*!
This method can be used as a file filter procedure for a
Navigation Services dialog box when no special handling
whatsoever is desired.  A return value of true means
“display query item”; a value of false means that the
item should not be displayed.

(1.0)
*/
pascal Boolean
FileSelectionDialogs_NothingFilterProc	(AEDesc*				UNUSED_ARGUMENT(inItem),
										 void*					UNUSED_ARGUMENT(inNavFileOrFolderInfoPtr),
										 NavCallBackUserData	UNUSED_ARGUMENT(inContext),
										 NavFilterModes			UNUSED_ARGUMENT(inFilter))
{
	Boolean					result = pascal_true;
	//NavFileOrFolderInfo*	infoPtr = REINTERPRET_CAST(inNavFileOrFolderInfoPtr, NavFileOrFolderInfo*);
	
	
	//if (typeFSS == inItem->descriptorType)
	//{
	//	if (false == infoPtr->isFolder)
	//	{
			// Use:
			// 'infoPtr->fileAndFolder.fileInfo.finderInfo.fdType'
			// to check for the file type you want to filter.
	//	}
	//}
	return result;
}// NothingFilterProc


/*!
This routine is adequate for 99% of “save file” dialog
needs on Mac OS 9.

(1.0)
*/
OSStatus
FileSelectionDialogs_PutFile	(ConstStr255Param		inPromptMessage,
								 ConstStr255Param		inDialogTitle,
								 ConstStr255Param		inDefaultFileNameOrNull,
								 OSType					inApplicationSignature,
								 OSType					inFileSignature,
								 UInt32					inPrefKey,
								 NavDialogOptionFlags	inFlags,
								 NavEventProcPtr		inEventProc,
								 NavReplyRecord*		outReplyPtr,
								 FSSpec*				outFileSpecPtr)
{
	NavDialogOptions	dialogOptions;
	NavEventUPP			eventUPP = NewNavEventUPP(inEventProc);
	OSStatus			result = noErr;
	
	
	result = NavGetDefaultDialogOptions(&dialogOptions);
	dialogOptions.dialogOptionFlags = 0;
	dialogOptions.dialogOptionFlags |= inFlags;
	
	// add customized settings
	PLstrcpy(dialogOptions.message, inPromptMessage);
	setUpWTitleProcessName(&dialogOptions, inDialogTitle);
	if (nullptr == inDefaultFileNameOrNull) PLstrcpy(dialogOptions.savedFileName, EMPTY_PSTRING);
	else PLstrcpy(dialogOptions.savedFileName, inDefaultFileNameOrNull);
	dialogOptions.preferenceKey = inPrefKey;
	
	Embedding_DeactivateFrontmostWindow();
	result = NavPutFile(nullptr, outReplyPtr, &dialogOptions, eventUPP,
						inFileSignature, inApplicationSignature, nullptr);
	Embedding_RestoreFrontmostWindow();
	DisposeNavEventUPP(eventUPP), eventUPP = nullptr;
	
	if (memFullErr == result) result = userCanceledErr;
	
	if ((noErr == result) && (outReplyPtr->validRecord))
	{
		// grab the target FSSpec from the AEDesc for saving:	
		AEDesc 		resultDesc;
		AEKeyword	keyword;
		Boolean		failed = false;
		
		
		result = AEGetNthDesc(&(outReplyPtr->selection), 1, typeFSS, &keyword, &resultDesc);
		if (noErr == result)
		{
			(OSStatus)AEGetDescData(&resultDesc, outFileSpecPtr, sizeof(FSSpec));
		}
		
		if (!outReplyPtr->replacing)
		{
			if (FSpCreate(outFileSpecPtr, inApplicationSignature,
								inFileSignature, outReplyPtr->keyScript))
			{
				Sound_StandardAlert();
				failed = true;
			}
		}
		
		if (!failed) AEDisposeDesc(&resultDesc);
	}
	return result;
}// PutFile


#pragma mark Internal Methods

/*!
Copies the data from an Apple Event into an arbitrary
handle.  If the specified buffer is too small, this
routine will copy as much data as possible and return
"memFullErr".

(1.0)
*/
static OSStatus
getAEDescData		(AEDesc const*		inDescPtr,
					 DescType*			outTypeCodePtr,
					 void*				outBufferPtr,
					 ByteCount			inMaximumSize,
					 ByteCount*			outActualSizePtr)
{
	ByteCount	dataSize = 0;
	OSStatus	result = noErr;
	
	
	if (nullptr != outTypeCodePtr) *outTypeCodePtr = inDescPtr->descriptorType;
	dataSize = AEGetDescDataSize(inDescPtr);
	if (dataSize > inMaximumSize)
	{
		dataSize = inMaximumSize;
		result = memFullErr;
	}
	if (nullptr != outActualSizePtr) *outActualSizePtr = dataSize;
	result = AEGetDescData(inDescPtr, outBufferPtr, dataSize);
	
	return result;
}// getAEDescData


/*!
This method creates a handle to a Navigation Services
file type list, which is an 'open' resource.

(1.0)
*/
static NavTypeListHandle
newOpenHandle	(OSType		inApplicationSignature,
				 UInt32		inNumberOfTypes,
				 OSType		inTypeList[])
{
	NavTypeListHandle	result = nullptr;
	
	
	if (inNumberOfTypes > 0)
	{
		Handle		dataHandle = Memory_NewHandle(sizeof(NavTypeList) + inNumberOfTypes * sizeof(OSType));
		
		
		if (nullptr != dataHandle)
		{
			result = REINTERPRET_CAST(dataHandle, NavTypeListHandle);
			(*result)->componentSignature = inApplicationSignature;
			(*result)->osTypeCount = inNumberOfTypes;
			BlockMoveData(inTypeList, (*result)->osType, inNumberOfTypes * sizeof(OSType));
		}
	}
	
	return result;
}// newOpenHandle


/*!
This method will send a series of 'odoc' Apple Events
for each event in the given event description list.

(1.0)
*/
static OSStatus
sendOpenAE		(AEDescList		inFileList)
{
	OSStatus		result = noErr;
	AEAddressDesc	currentApplication;
	
	
	result = AppleEventUtilities_InitAEDesc(&currentApplication);
	if (noErr == result)
	{
		ProcessSerialNumber		psn;
		
		
		result = GetCurrentProcess(&psn);
		if (noErr == result)
		{
			result = AECreateDesc(typeProcessSerialNumber, &psn, sizeof(ProcessSerialNumber), &currentApplication);
			if (noErr == result)
			{
				AppleEvent		dummyReply;
				
				
				result = AppleEventUtilities_InitAEDesc(&dummyReply);
				if (noErr == result)
				{
					AppleEvent		openDocumentsEvent;
					
					
					result = AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments,
												&currentApplication, kAutoGenerateReturnID,
												kAnyTransactionID, &openDocumentsEvent);
					if (noErr == result)
					{
						result = AEPutParamDesc(&openDocumentsEvent, keyDirectObject, &inFileList);
						if (noErr == result)
						{
							result = AESend(&openDocumentsEvent, &dummyReply, kAENoReply | kAEAlwaysInteract,
											kAENormalPriority, kAEDefaultTimeout, nullptr/* idle routine */,
											nullptr/* filter routine */);
						}
					}
				}
			}
		}
	}
	
	return result;
}// sendOpenAE


/*!
To automatically set up a Navigation Services
dialog options structure to contain a window
title consisting of the specified title, a
colon, and the name of the current process as
a “client name”, use this method.

On output, the "windowTitle" and "clientName"
fields of the NavDialogOptions structure are
filled in.

(1.0)
*/
static void
setUpWTitleProcessName		(NavDialogOptions*		inoutDataPtr,
							 ConstStringPtr			inTitle)
{
	if ((nullptr != inoutDataPtr) && (nullptr != inTitle))
	{
		Str255							titleCopy;
		StringSubstitutionSpec const	metaMappings[] =
										{
											{ kStringSubstitutionDefaultTag1, titleCopy },
											{ kStringSubstitutionDefaultTag2, inoutDataPtr->clientName }
										};
		
		
		PLstrcpy(titleCopy, inTitle);
		
		// %a = (given title), %b = (process name)
		PLstrcpy(inoutDataPtr->windowTitle, "\p%a: %b"); // LOCALIZE THIS
		Localization_GetCurrentApplicationName(inoutDataPtr->clientName);
		StringUtilities_PBuild(inoutDataPtr->windowTitle,
								sizeof(metaMappings) / sizeof(StringSubstitutionSpec),
								metaMappings);
	}
}// setUpWTitleProcessName

// BELOW IS REQUIRED NEWLINE TO END FILE
