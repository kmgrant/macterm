/*!	\file AppleEventUtilities.cp
	\brief Useful routines for firing off Apple Events.
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#include "AppleEventUtilities.h"
#include <UniversalDefines.h>

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

// application includes
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
					UNUSED_RETURN(OSStatus)AEPutParamDesc(inoutReplyAppleEventPtr, keyErrorNumber,
															inResultAppleEventPtr);
					break;
				
				case typeChar: // error message
					UNUSED_RETURN(OSStatus)AEPutParamDesc(inoutReplyAppleEventPtr, keyErrorString,
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
			// under Carbon, there is a new API for this purpose
			result = AEGetDescData(descPtr, outDataPtr, inDataPtrMaximumSize);
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
OSErr
AppleEventUtilities_ObjectExists	(AppleEvent const*	inAppleEventPtr,
									 AppleEvent*		outReplyAppleEventPtr, 
									 SInt32				UNUSED_ARGUMENT(inData))
{
	AEDesc		directObject,
				directToken,
				replyDesc;
	OSErr		result = noErr;
	
	
	UNUSED_RETURN(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
	UNUSED_RETURN(OSStatus)AppleEventUtilities_InitAEDesc(&directToken);
	UNUSED_RETURN(OSStatus)AppleEventUtilities_InitAEDesc(&replyDesc);
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “exists” event");
	
	UNUSED_RETURN(OSStatus)AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
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
