/*###############################################################

	AppleEventUtilities.cp
	
	Useful (read: indispensable) routines when programming
	AppleScript into applications.  Several routines in this
	file are derived from Apple’s sample code, and have the
	appropriate copyrights.
	
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
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <Console.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "FileUtilities.h"
#include "UIStrings.h"



#pragma mark Public Methods

/*!
Adds an error to the reply record; at least the
error code is provided, and optionally you can
accompany it with an equivalent message.

You should use this to return error codes from
ALL Apple Events; if you do, return "noErr" from
the handler but potentially define "inError" as
a specific error code.

(3.0)
*/
OSStatus
AppleEventUtilities_AddErrorToReply		(ConstStringPtr		inErrorMessageOrNull,
										 OSStatus			inError,
										 AppleEventPtr		inoutReplyAppleEventPtr)
{
	OSStatus	result = noErr;
	
	
	// the reply must exist
	if (AppleEventUtilities_IsValidReply(inoutReplyAppleEventPtr))
	{
		// put error code
		if (inError != noErr)
		{
			result = AEPutParamPtr(inoutReplyAppleEventPtr, keyErrorNumber, typeSInt16,
									&inError, sizeof(inError));
		}
		
		// if provided, also put the error message
		if (inErrorMessageOrNull != nullptr)
		{
			result = AEPutParamPtr(inoutReplyAppleEventPtr, keyErrorString, typeChar,
									inErrorMessageOrNull + 1/* skip length byte */,
									STATIC_CAST(PLstrlen(inErrorMessageOrNull) * sizeof(UInt8), Size));
		}
	}
	else
	{
		// no place to put the data - do nothing
		result = errAEReplyNotValid;
	}
	
	return result;
}// AddErrorToReply


/*!
Adds a result to a reply record.  Usually,
this is what you do in response to a given
Apple Event.  If you provide a non-zero
error code, the result is added to the
reply record as an Apple Event error number
or error message, depending on the data.

(3.0)
*/
OSStatus
AppleEventUtilities_AddResultToReply	(AEDesc*	inResultAppleEventPtr,
										 AEDesc*	inoutReplyAppleEventPtr,
										 OSStatus	inError)
{
	OSStatus		result = inError;
	
	
	// the reply must exist, and there must be a result to put in it
	if ((inoutReplyAppleEventPtr->descriptorType != typeNull) &&
		(inResultAppleEventPtr->descriptorType != typeNull))
	{
		if (inError == noErr)
		{
			// a normal result
			result = AEPutParamDesc(inoutReplyAppleEventPtr, keyDirectObject, inResultAppleEventPtr);
		}
		else
		{
			// abnormal result; maybe error feedback is being given (check possible feedback types)
			switch (inResultAppleEventPtr->descriptorType)
			{
				case typeSInt16: // error code
					(OSStatus)AEPutParamDesc(inoutReplyAppleEventPtr, keyErrorNumber,
												inResultAppleEventPtr);
					break;
				
				case typeChar: // error message
					(OSStatus)AEPutParamDesc(inoutReplyAppleEventPtr, keyErrorString,
												inResultAppleEventPtr);
					break;
				
				default:
					result = errAETypeError;
					break;
			}
		}
	}
	
	return result;
}// AddResultToReply


/*!
Use this Carbon-compliant routine to access data
in an Apple Event descriptor structure.  You may
not directly access the data handle, because this
approach is not supported under Carbon.  This
routine behaves exactly like the Carbon routine
AEGetDescData(), except it works on pre-Carbon
systems.

Use AEGetDescDataSize() beforehand if you wish to
allocate a precise amount of storage before using
this routine.

WARNING:	You must allocate the space yourself.
			Calling this routine will not provide
			you with a pointer to existing data;
			you must create a sufficiently large
			buffer, and then use this routine to
			copy descriptor data into your new
			buffer.

(3.0)
*/
OSStatus
AppleEventUtilities_CopyDescriptorData		(AEDesc const*	inDescriptorPtr,
											 void*			outDataPtr,
											 Size 			inDataPtrMaximumSize,
											 Size*			outActualSizePtr)
{
	return AppleEventUtilities_CopyDescriptorDataAs(inDescriptorPtr->descriptorType,
													inDescriptorPtr, outDataPtr, inDataPtrMaximumSize,
													outActualSizePtr);
}// CopyDescriptorData


/*!
Use this Carbon-compliant routine to access data
in an Apple Event descriptor structure and coerce
it (if possible) to the specified type.

Use AEGetDescDataSize() beforehand if you wish to
allocate a precise amount of storage before using
this routine.

WARNING:	You must allocate the space yourself.
			Calling this routine will not provide
			you with a pointer to existing data;
			you must create a sufficiently large
			buffer, and then use this routine to
			copy descriptor data into your new
			buffer.

(3.0)
*/
OSStatus
AppleEventUtilities_CopyDescriptorDataAs	(DescType		inDesiredType,
											 AEDesc const*	inDescriptorPtr,
											 void*			outDataPtr,
											 Size 			inDataPtrMaximumSize,
											 Size*			outActualSizePtr)
{
	OSStatus		result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("copying descriptor data");
	if ((inDescriptorPtr == nullptr) || (outDataPtr == nullptr)) result = memPCErr;
	else
	{
		AEDesc*		descPtr = (AEDesc*)Memory_NewPtr(sizeof(AEDesc));
		
		
		if (descPtr != nullptr)
		{
			descPtr->descriptorType = typeNull;
			descPtr->dataHandle = nullptr;
			if (inDesiredType != inDescriptorPtr->descriptorType)
			{
				//Console_WriteLine("source and destination types differ; coercing...");
				result = AECoerceDesc(inDescriptorPtr, inDesiredType, descPtr);
			}
			else
			{
				//Console_WriteLine("source and destination types identical; duplicating...");
				result = AEDuplicateDesc(inDescriptorPtr, descPtr);
			}
		}
		else result = memFullErr;
		
		if (result == noErr)
		{
		#if TARGET_API_MAC_OS8
			Size		byteCountToCopy = AEGetDescDataSize(descPtr);
			
			
			// copy the data, reporting an error (but not aborting the copy) if there’s not enough room
			Console_WriteLine("determining how much to copy");
			if (byteCountToCopy > inDataPtrMaximumSize)
			{
				Console_WriteLine("too little space to copy completely!");
				byteCountToCopy = inDataPtrMaximumSize;
				result = memSCErr;
			}
			BlockMoveData(*(descPtr->dataHandle), outDataPtr, byteCountToCopy);
		#else
			// under Carbon, there is a new API for this purpose
			result = AEGetDescData(descPtr, outDataPtr, inDataPtrMaximumSize);
		#endif
			if (outActualSizePtr != nullptr) *outActualSizePtr = AEGetDescDataSize(descPtr);
		}
		if (descPtr != nullptr) Memory_DisposePtr((Ptr*)&descPtr);
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// CopyDescriptorDataAs


/*!
Resolves the specified descriptor if it is an object
specifier (returning the resolved token), or otherwise
duplicates the given descriptor, returning the
duplicate.  You typically use this in all event handlers,
to ensure that you have a token descriptor and not a
reference to an object.

(3.0)
*/
OSStatus
AppleEventUtilities_DuplicateOrResolveObject	(AEDesc const*	inDescriptor,
												 AEDesc*		outDuplicate)
{
	OSStatus		result = noErr;
	
	
	if (inDescriptor->descriptorType == typeObjectSpecifier)
	{
		Console_WriteLine("given an object reference");
		result = AEResolve(inDescriptor, kAEIDoMinimum, outDuplicate);
	}
	else result = AEDuplicateDesc(inDescriptor, outDuplicate);
	
	return result;
}// DuplicateOrResolveObject


/*!
Connects to the default scripting component and
executes the script defined by the specified data.

You typically obtain data by reading from resources
of type "kOSAGenericScriptingComponentSubtype" in
compiled script files.

Specify "false" for "inNotifyUser" if you want to
have control over errors that a script might
encounter (in either case, the error code of any
error is returned).  If "true" is specified, this
routine automatically displays an error alert with
the proper message if any errors occur.

(3.0)
*/
OSStatus
AppleEventUtilities_ExecuteScript	(Handle		inScriptData,
									 Boolean	UNUSED_ARGUMENT(inNotifyUser))
{
	OSStatus			result = noErr;
	ComponentInstance	scriptingComponent = OpenDefaultComponent
												(kOSAComponentType,
													kOSAGenericScriptingComponentSubtype);
	
	
	if (scriptingComponent != nullptr)
	{
		AEDesc		scriptDataDescriptor;
		OSAID		scriptID = 0;
		OSAID		resultID = 0;
		
		
		(OSStatus)AppleEventUtilities_InitAEDesc(&scriptDataDescriptor);
		result = AECreateDesc(typeOSAGenericStorage, *inScriptData, GetHandleSize(inScriptData),
								&scriptDataDescriptor);
		result = OSALoad(scriptingComponent, &scriptDataDescriptor,
						kOSAModeNull, &scriptID);
		if (result == noErr)
		{
			result = OSAExecute(scriptingComponent, scriptID, kOSANullScript,
								kOSAModeNull, &resultID);
			(OSStatus)OSADispose(scriptingComponent, scriptID);
			(OSStatus)OSADispose(scriptingComponent, resultID);
			if (result == errOSAScriptError)
			{
				// the script reported an error - display it to the user
				UIStrings_Result	stringResult = kUIStrings_ResultOK;
				CFStringRef			titleCFString = nullptr;
				CFStringRef			messageCFString = nullptr;
				CFStringRef			helpTextCFString = nullptr;
				CFStringRef			helpTextTemplateCFString = nullptr;
				AEDesc				errorMessageDescriptor;
				AEDesc				errorNumberDescriptor;
				AEDesc				titleDescriptor;
				OSStatus			error = noErr;
				
				
				(OSStatus)AppleEventUtilities_InitAEDesc(&errorMessageDescriptor);
				(OSStatus)AppleEventUtilities_InitAEDesc(&errorNumberDescriptor);
				(OSStatus)AppleEventUtilities_InitAEDesc(&titleDescriptor);
				
				// get the error number (as the function result), and display it in the help text
				if (OSAScriptError(scriptingComponent, kOSAErrorNumber,
									typeSInt16, &errorNumberDescriptor) == noErr)
				{
					error = AppleEventUtilities_CopyDescriptorData(&errorNumberDescriptor,
																	&result, sizeof(result),
																	nullptr/* actual size storage */);
					if (error == noErr)
					{
						(OSStatus)AEDisposeDesc(&errorNumberDescriptor);
						stringResult = UIStrings_Copy(kUIStrings_AlertWindowScriptErrorHelpText,
														helpTextTemplateCFString);
						if (stringResult.ok())
						{
							helpTextCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
																		helpTextTemplateCFString,
																		result/* substituted number */);
						}
						else
						{
							helpTextCFString = CFSTR("");
						}
					}
				}
				
				// use the exact error message from the script, if provided
				if (OSAScriptError(scriptingComponent, kOSAErrorMessage,
										cStringClass, &errorMessageDescriptor) == noErr)
				{
					Str255		message;
					
					
					error = AppleEventUtilities_CopyDescriptorData(&errorMessageDescriptor,
																	message + 1, sizeof(message) - 1,
																	nullptr/* actual size storage */);
					if (error == noErr)
					{
						message[0] = STATIC_CAST(AEGetDescDataSize(&errorMessageDescriptor), UInt8);
						(OSStatus)AEDisposeDesc(&errorMessageDescriptor);
						
						// "OSA.h" does not indicate any particular encoding for the error
						// message, so U.S. English Mac Roman is assumed.
						messageCFString = CFStringCreateWithPascalString
											(kCFAllocatorDefault, message, kCFStringEncodingMacRoman);
					}
				}
				
				// get the alert title
				UIStrings_Copy(kUIStrings_AlertWindowScriptErrorName, titleCFString);
				
				// display an alert for important errors
				if ((result != noErr) && (result != userCanceledErr))
				{
					InterfaceLibAlertRef	box = nullptr;
					
					
					box = Alert_New();
					Alert_SetHelpButton(box, false);
					Alert_SetParamsFor(box, kAlert_StyleCancel);
					Alert_SetTextCFStrings(box, messageCFString, helpTextCFString);
					Alert_SetTitleCFString(box, titleCFString);
					Alert_SetType(box, kAlertStopAlert);
					Sound_StandardAlert();
					Alert_Display(box);
					Alert_Dispose(&box);
				}
				if (messageCFString != nullptr) CFRelease(messageCFString), messageCFString = nullptr;
				if (helpTextCFString != nullptr) CFRelease(helpTextCFString), helpTextCFString = nullptr;
				CFRelease(titleCFString), titleCFString = nullptr;
			}
		}
		(OSStatus)CloseComponent(scriptingComponent);
	}
	else result = errOSACantOpenComponent;
	
	return result;
}// ExecuteScript


/*!
Runs a script from a file.  In Mac OS 8.x and 9.x,
this file would be expected to contain a resource
of type "kOSAGenericScriptingComponentSubtype",
from which the necessary script data is read.

If the specified file refers to an alias, this
routine automatically resolves it before trying
to execute it as a script.

Specify "false" for "inNotifyUser" if you want to
have control over errors that a script might
encounter (in either case, the error code of any
error is returned).  If "true" is specified, this
routine automatically displays an error alert with
the proper message if any errors occur.

(3.0)
*/
OSStatus
AppleEventUtilities_ExecuteScriptFile	(FSSpec const*	inFile,
										 Boolean		inNotifyUser)
{
	OSStatus	result = noErr;
	FSSpec		file;
	
	
	// if this file is in fact an alias, ensure the original file is used for reading resources
	(OSStatus)FSMakeFSSpec(inFile->vRefNum, inFile->parID, inFile->name, &file);
	result = FileUtilities_EnsureOriginalFile(&file);
	
	if (result == noErr)
	{
		SInt16		scriptResourceFile = -1;
		
		
		scriptResourceFile = FSpOpenResFile(&file, fsRdPerm);
		result = ResError();
		if (result == noErr)
		{
			Handle		scriptData = nullptr;
			SInt16		oldResFile = CurResFile();
			
			
			UseResFile(scriptResourceFile);
			if (Count1Resources(kOSAScriptResourceType) <= 0) result = resNotFound;
			else
			{
				// get first available script
				scriptData = Get1IndResource(kOSAScriptResourceType, 1);
				result = ResError();
			}
			if (result == noErr)
			{
				if (scriptData == nullptr) result = resNotFound;
				else
				{
					DetachResource(scriptData);
					result = AppleEventUtilities_ExecuteScript(scriptData, inNotifyUser);
					
					// free memory occupied by the resource data
					Memory_ReleaseDetachedResource(&scriptData);
				}
			}
			UseResFile(oldResFile);
		}
		else
		{
			// failed...could probably tell the user something useful here
		}
	}
	
	return result;
}// ExecuteScriptFile


/*!
Initializes an "AEDesc" data structure.  Since
Carbon may eventually make Apple Event structures
opaque, consistent use of this routine is
recommended to ease future development efforts.

You typically replace this...

	AEDesc	desc = { typeNull, nullptr };

...with this:

	AEDesc	desc;
	(OSStatus)AppleEventUtilities_InitAEDesc(&desc);

(3.0)
*/
OSStatus
AppleEventUtilities_InitAEDesc		(AEDesc*	inoutDescriptorPtr)
{
	OSStatus		result = noErr;
	
	
	if (inoutDescriptorPtr == nullptr) result = memPCErr;
	else
	{
		inoutDescriptorPtr->descriptorType = typeNull;
		inoutDescriptorPtr->dataHandle = nullptr;
	}
	return result;
}// InitAEDesc


/*!
This method determines if the specified event is
a valid reply event.  Specifically, an invalid
reply is one with nullptr data and a type of
"typeNull".

(3.0)
*/
Boolean
AppleEventUtilities_IsValidReply		(AppleEventPtr		inAppleEventPtr)
{
	Boolean		result = false;
	
	
	if (inAppleEventPtr != nullptr) result = ((inAppleEventPtr->descriptorType != typeNull) ||
												(inAppleEventPtr->dataHandle != nullptr));
	return result;
}// IsValidReply


/*!
To determine if an Apple Event object exists,
use this method.

(3.0)
*/
pascal OSErr
AppleEventUtilities_ObjectExists	(AppleEvent const*	inAppleEventPtr,
									 AppleEvent*		outReplyAppleEventPtr, 
									 SInt32				UNUSED_ARGUMENT(inData))
{
	AEDesc		directObject,
				directToken,
				replyDesc;
	OSErr		result = noErr;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
	(OSStatus)AppleEventUtilities_InitAEDesc(&directToken);
	(OSStatus)AppleEventUtilities_InitAEDesc(&replyDesc);
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “exists” event");
	
	(OSStatus)AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
	result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
	if (result == noErr)
	{
		Boolean       exists = false;
		
		
		if (directObject.descriptorType == typeNull) exists = true; // the application always exists!
		else
		{
			result = AEResolve(&directObject, kAEIDoMinimum, &directToken);
			if (result == noErr)
			{
				Console_WriteLine("successfully resolved");
				if (directToken.descriptorType == typeAEList)
				{
					long		itemCount = 0L;
					
					
					// for lists, objects “exist” if the list is not empty
					Console_WriteLine("given a list of objects; any non-empty list should exist");
					result = AECountItems(&directToken, &itemCount);
					if (result == noErr)
					{
						if (itemCount) exists = true;
						else exists = false;
					}
				}
				else exists = true;
			}
			else exists = false;
		}
		
		if (result == noErr)
		{
			result = AECreateDesc(typeBoolean, (Ptr)&exists, sizeof(exists), &replyDesc);
			if (result == noErr)
			{
				// return whether or not the object exists
				result = STATIC_CAST(AppleEventUtilities_AddResultToReply
										(&replyDesc, outReplyAppleEventPtr, result), OSErr);
			}
		}
	}
	
	if (directObject.dataHandle) AEDisposeDesc(&directObject);
	if (directToken.dataHandle) AEDisposeToken(&directToken);
	if (replyDesc.dataHandle) AEDisposeDesc(&replyDesc);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// ObjectExists


/*!
This method determines if the required events for the
specified Apple Event were received properly.  If so,
"noErr" is returned; otherwise, "errAEEventNotHandled"
is returned.

Bug: (?)	It appears as though any event that states
			it has no direct object will cause this
			routine to report a parameter error.  I
			can’t figure out why this would be; it
			neither appears to be in this code, nor in
			the user terminology resource.  Any ideas?

(3.0)
*/
OSErr
AppleEventUtilities_RequiredParametersError		(AppleEvent const*	inAppleEventPtr)
{
	DescType	returnedType = '----';
	Size		actualSize = 0;
	OSErr		result = noErr;
	
	
	result = AEGetAttributePtr(inAppleEventPtr, keyMissedKeywordAttr,
								typeWildCard, &returnedType, nullptr/* data pointer */,
								0/* maximum size */, &actualSize);
	if (result == errAEDescNotFound)
	{
		// got all the required parameters, so the event is fine
		result = noErr;
	}
	else if (result == noErr)
	{
		// “missed keyword” attribute exists, so a required parameter was not provided; this is an error
		result = errAEParamMissed;
	}
	
	return result;
}// RequiredParametersError


/*!
If you send an Apple Event and receive a reply
record, you can use this routine to determine
if any error code ("keyErrorNumber") was added
to the reply, indicating that something may
have gone wrong.

If an error exists, "outErrorPtrOrNull" is set
and "true" is returned; otherwise, "false" is
returned and "outErrorPtrOrNull" is not defined.

If you are only interested in *whether* there
was an error, and not in any particular error,
set "outErrorPtrOrNull" to nullptr.

(3.0)
*/
Boolean
AppleEventUtilities_RetrieveReplyError	(AppleEventPtr	inReplyAppleEventPtr,
										 OSStatus*		outErrorPtrOrNull)
{
	Boolean		result = false;
	DescType	actualType = typeNull;
	Size		actualSize = 0L;
	OSStatus	eventError = noErr;
	OSStatus	error = noErr;
	
	
	error = AEGetParamPtr(inReplyAppleEventPtr, keyErrorNumber, typeSInt32,
							&actualType, &eventError, sizeof(eventError), &actualSize);
	result = ((error == noErr) && (eventError != noErr));
	if (outErrorPtrOrNull != nullptr) *outErrorPtrOrNull = eventError;
	
	return result;
}// RetrieveReplyError

// BELOW IS REQUIRED NEWLINE TO END FILE
