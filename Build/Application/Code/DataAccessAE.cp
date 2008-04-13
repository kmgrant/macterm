/*###############################################################

	DataAccessAE.cp
	
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

// standard-C includes
#include <cctype>
#include <cstring>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "BasicTypesAE.h"
#include "DataAccessAE.h"
#include "GetPropertyAE.h"
#include "ObjectClassesAE.h"
#include "Terminology.h"



//
// internal methods
//

static OSStatus		getData			(AEDesc*			inFromWhichObject,
									 DescType			inGetDataOfThisType,
									 AEDesc*			outDataDescriptor,
									 Boolean*			outUsedDesiredDataType);

static OSStatus		getDataSize		(AEDesc*			inFromWhichObject,
									 DescType			inGetDataOfThisType,
									 AEDesc*			outDataDescriptor,
									 Boolean*			outUsedDesiredDataType);

static OSStatus		setData			(AEDesc const*		inForWhichObject,
									 AEDesc*			inDataDescriptor);



//
// public methods
//

/*!
Handles the following event types:

kAECoreSuite
------------
	kAEGetData

(3.0)
*/
pascal OSErr
DataAccessAE_HandleGetData		(AppleEvent const*	inAppleEventPtr,
								 AppleEvent*		outReplyAppleEventPtr,
								 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr		result = errAEEventNotHandled;
	AEDesc		directObjectDescriptor,
				resultDescriptor;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObjectDescriptor);
	(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
	
	// get the object whose data is to be returned
	result = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObjectDescriptor);
	if (result == noErr)
	{
		DescType	returnedType = '----';
		DescType	requestedType = '----';
		Size		actualSize = 0L;
		
		
		// try to return the data in a form that is specifically requested
		(OSStatus)AEGetParamPtr(inAppleEventPtr, keyAEParamMyAs, typeType, &returnedType, &requestedType,
								sizeof(requestedType), &actualSize);
		result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (result == noErr)
		{
			// get the data
			AEDesc		booleanDesc;
			Boolean		usedDesiredDataType = false; // was the type given in the "as" parameter respected?
			
			
			(OSStatus)AppleEventUtilities_InitAEDesc(&booleanDesc);
			
			result = getData(&directObjectDescriptor, requestedType, &resultDescriptor, &usedDesiredDataType);
			if (BasicTypesAE_CreateBooleanDesc(usedDesiredDataType, &booleanDesc) == noErr)
			{
				// AppleScript 1.5: add a "keyAppHandledCoercion" to the reply
				(OSStatus)AEPutParamDesc(&resultDescriptor, keyAppHandledCoercion, &booleanDesc);
			}
			result = AppleEventUtilities_AddResultToReply(&resultDescriptor, outReplyAppleEventPtr, result);
		}
	}
	
	if (directObjectDescriptor.dataHandle != nullptr) AEDisposeDesc(&directObjectDescriptor);
	if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
	
	return result;
}// HandleGetData


/*!
Handles the following event types:

kAECoreSuite
------------
	kAEGetDataSize

(3.0)
*/
pascal OSErr
DataAccessAE_HandleGetDataSize	(AppleEvent const*	inAppleEventPtr,
								 AppleEvent*		outReplyAppleEventPtr,
								 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr		result = errAEEventNotHandled;
	AEDesc		directObjectDescriptor,
				resultDescriptor;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObjectDescriptor);
	(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
	
	// get the object whose data size is to be returned
	result = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObjectDescriptor);
	if (result == noErr)
	{
		DescType	returnedType = '----';
		DescType	requestedType = '----';
		Size		actualSize = 0L;
		
		
		// try to return the data in a form that is specifically requested
		(OSStatus)AEGetParamPtr(inAppleEventPtr, keyAEParamMyAs, typeType, &returnedType, &requestedType,
								sizeof(requestedType), &actualSize);
		result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (result == noErr)
		{
			// get the data size
			Boolean		usedDesiredDataType = false; // was the type given in the "as" parameter respected?
			
			
			result = getDataSize(&directObjectDescriptor, requestedType, &resultDescriptor, &usedDesiredDataType);
			result = AppleEventUtilities_AddResultToReply(&resultDescriptor, outReplyAppleEventPtr, result);
		}
	}
	
	if (directObjectDescriptor.dataHandle != nullptr) AEDisposeDesc(&directObjectDescriptor);
	if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
	
	return result;
}// HandleGetDataSize


/*!
Handles the following event types:

kAECoreSuite
------------
	kAESetData

(3.0)
*/
pascal OSErr
DataAccessAE_HandleSetData		(AppleEvent const*	inAppleEventPtr,
								 AppleEvent*		UNUSED_ARGUMENT(outReplyAppleEventPtr),
								 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr		result = errAEEventNotHandled;
	AEDesc		directObjectDescriptor,
				dataDescriptor;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObjectDescriptor);
	(OSStatus)AppleEventUtilities_InitAEDesc(&dataDescriptor);
	
	// get the object whose data is to be modified
	result = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObjectDescriptor);
	if (result == noErr)
	{
		// obtain the data that should be used to make a modification to the object
		(OSStatus)AEGetParamDesc(inAppleEventPtr, keyAEParamMyTo, typeWildCard, &dataDescriptor);
		result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (result == noErr)
		{
			// set the data
			result = setData(&directObjectDescriptor, &dataDescriptor);
		}
	}
	
	if (directObjectDescriptor.dataHandle != nullptr) AEDisposeDesc(&directObjectDescriptor);
	if (dataDescriptor.dataHandle != nullptr) AEDisposeDesc(&dataDescriptor);
	
	return result;
}// HandleSetData


//
// internal methods
//
#pragma mark -

/*!
This routine indirectly handles all Òget dataÓ
Apple Events.

When a script asks for data, AppleScript will
figure out what is being said (for example,
"window 1" means Òa window referenced by
absolute indexÓ) and invoke an object accessor
to construct a token (see "GetObjectAE.cp").
The token that is constructed is INTERNAL to
MacTelnet, and is passed in "inFromWhichObject".
This routine uses the token to determine the
nature of the request, and therefore how to
handle it.  Most ÒdataÓ is attached to a high
level object as a property, so usually the
token will be a property token.

(3.0)
*/
static OSStatus
getData		(AEDesc*	inFromWhichObject,
			 DescType   inGetDataOfThisType,
			 AEDesc*	outDataDescriptor,
			 Boolean*   outUsedDesiredDataType)
{
	AEDesc		tokenDescriptor;
	OSStatus	result = noErr;
	
	
	Console_BeginFunction();
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	
	Console_WriteLine("AppleScript: Òget dataÓ event");
	Console_WriteValueFourChars("desired type", inGetDataOfThisType);
	
	if (inFromWhichObject->descriptorType == typeObjectSpecifier)
	{
		// all thatÕs known is that this is an object; resolve it into a known object type
		// using a previously-installed OSL object accessor (see "InstallAE.cp" for details)
		Console_WriteLine("given an object reference");
		result = AEResolve(inFromWhichObject, kAEIDoMinimum, &tokenDescriptor);
	}
	else result = AEDuplicateDesc(inFromWhichObject, &tokenDescriptor);
	
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		Console_WriteValueFourChars("to type", tokenDescriptor.descriptorType);
		switch (tokenDescriptor.descriptorType)
		{
		case typeAEList:
			{
				long	itemIndex = 0L;
				
				
				result = AECountItems(&tokenDescriptor, &itemIndex);
				if (result == noErr)
				{
					AEDesc		listItemDescriptor,
								resultDescriptor;
					DescType	returnedType = '----';
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&listItemDescriptor);
					(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
					
					for (; itemIndex > 0; --itemIndex)
					{
						result = AEGetNthDesc(&tokenDescriptor, itemIndex, typeWildCard,
												&returnedType, &listItemDescriptor);
						
						// handle the request by adding the resultant data to the end of the list
						if (result == noErr) result = getData(&listItemDescriptor,
																inGetDataOfThisType, &resultDescriptor,
																outUsedDesiredDataType);
						if (result == noErr) result = AEPutDesc(&tokenDescriptor/* list */, itemIndex,
																&resultDescriptor/* item to add */);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
						if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
					}
					result = AEDuplicateDesc(&tokenDescriptor, outDataDescriptor);
				}
			}
			break;
		
		case cMyInternalToken:
			result = GetPropertyAE_DataFromClassProperty(&tokenDescriptor, inGetDataOfThisType,
															outDataDescriptor, outUsedDesiredDataType);
			break;
		
		default:
			// probably a primitive type, no special action necessary?
			result = AEDuplicateDesc(&tokenDescriptor, outDataDescriptor);
			break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getData


/*!
This routine indirectly handles all Òget data sizeÓ
Apple Events.

(3.0)
*/
static OSStatus
getDataSize		(AEDesc*	inFromWhichObject,
				 DescType   inGetDataOfThisType,
				 AEDesc*	outDataDescriptor,
				 Boolean*   outUsedDesiredDataType)
{
	AEDesc			sizeInfoDescriptor;
	OSStatus		result = noErr;
	
	
	Console_BeginFunction();
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&sizeInfoDescriptor);
	
	Console_WriteLine("AppleScript: Òget data sizeÓ event");
	Console_WriteValueFourChars("desired type", inGetDataOfThisType);
	
	if (inFromWhichObject->descriptorType == typeObjectSpecifier)
	{
		// all thatÕs known is that this is an object; resolve it into a known object type
		// using a previously-installed OSL object accessor (see "InstallAE.cp" for details)
		Console_WriteLine("given an object reference");
		result = AEResolve(inFromWhichObject, kAEIDoMinimum, &sizeInfoDescriptor);
	}
	else result = AEDuplicateDesc(inFromWhichObject, &sizeInfoDescriptor);
	
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		
		if (sizeInfoDescriptor.descriptorType == typeAEList)
		{
			// create a list where each corresponding item is the size of the data in the other list item
			result = AECreateList(nullptr, 0, false/* is a record */, outDataDescriptor);
			if (result == noErr)
			{
				AEDesc		listItemDescriptor,
							resultDescriptor;
				DescType	returnedType = '----';
				long		itemIndex = 0L;
				
				
				(OSStatus)AppleEventUtilities_InitAEDesc(&listItemDescriptor);
				(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
				
				result = AECountItems(&sizeInfoDescriptor, &itemIndex);
				for (; itemIndex > 0; --itemIndex)
				{
					result = AEGetNthDesc(&sizeInfoDescriptor, itemIndex, typeWildCard,
											&returnedType, &listItemDescriptor);
					
					// handle the request by adding the resultant data to the end of the list
					if (result == noErr) result = getDataSize(&listItemDescriptor,
															inGetDataOfThisType, &resultDescriptor,
															outUsedDesiredDataType);
					if (result == noErr) result = AEPutDesc(&sizeInfoDescriptor/* list */, itemIndex,
															&resultDescriptor/* item to add */);
					
					if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
					if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
				}
				result = AEDuplicateDesc(&sizeInfoDescriptor, outDataDescriptor);
			}
		}
		else // an internal MacTelnet token type!
		{
			result = GetPropertyAE_DataSizeFromClassProperty(&sizeInfoDescriptor, outDataDescriptor,
																outUsedDesiredDataType);
		}
	}
	
	if (sizeInfoDescriptor.dataHandle != nullptr) AEDisposeToken(&sizeInfoDescriptor);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getDataSize


/*!
This routine indirectly handles all Òset dataÓ
Apple Events.

When a script supplies data, AppleScript will
figure out what is being said (for example,
"window 1" means Òa window referenced by
absolute indexÓ) and invoke an object accessor
to construct a token (see "GetObjectAE.cp").
The token that is constructed is INTERNAL to
MacTelnet, and is passed in "inForWhichObject".
This routine uses the token to determine the
nature of the request, and therefore how to
handle it.  Most ÒdataÓ is attached to a high
level object as a property, so usually the
token will be a property token.

(3.0)
*/
static OSStatus
setData		(AEDesc const*		inForWhichObject,
			 AEDesc*			inDataDescriptor)
{
	AEDesc		tokenDescriptor;
	OSStatus	result = noErr;
	
	
	Console_BeginFunction();
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	
	Console_WriteLine("AppleScript: Òset dataÓ event");
	
	if (inForWhichObject->descriptorType == typeObjectSpecifier)
	{
		// all thatÕs known is that this is an object; resolve it into a known object type
		// using a previously-installed OSL object accessor (see "InstallAE.cp" for details)
		Console_WriteLine("given an object reference");
		result = AEResolve(inForWhichObject, kAEIDoMinimum, &tokenDescriptor);
	}
	else result = AEDuplicateDesc(inForWhichObject, &tokenDescriptor);
	
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		Console_WriteValueFourChars("to type", tokenDescriptor.descriptorType);
		switch (tokenDescriptor.descriptorType)
		{
		case typeAEList:
			{
				long	itemIndex = 0L;
				
				
				result = AECountItems(&tokenDescriptor, &itemIndex);
				if (result == noErr)
				{
					AEDesc		listItemDescriptor;
					DescType	returnedType = '----';
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&listItemDescriptor);
					
					for (; itemIndex > 0; --itemIndex)
					{
						result = AEGetNthDesc(&tokenDescriptor, itemIndex, typeWildCard,
												&returnedType, &listItemDescriptor);
						
						// handle the request by adding the resultant data to the end of the list
						if (result == noErr) result = setData(&listItemDescriptor, inDataDescriptor);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
					}
				}
			}
			break;
		
		default:
			result = GetPropertyAE_DataToClassProperty(&tokenDescriptor, inDataDescriptor);
			break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// setData

// BELOW IS REQUIRED NEWLINE TO END FILE
