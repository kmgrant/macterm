/*###############################################################

	CoreSuiteAE.cp
	
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <SoundSystem.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "CoreSuiteAE.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "GenesisAE.h"
#include "MacroManager.h"
#include "ObjectClassesAE.h"
#include "SessionFactory.h"
#include "Terminology.h"



//
// internal methods
//

static OSStatus		closeObject			(AEDesc const*		inWhat,
										 AEDesc*			outReply,
										 DescType			inSaveOption);

static OSStatus		closeToken			(AEDesc const*		inObjectTokenDescriptor,
										 DescType			inSaveOption);

static OSStatus		countElements		(AEDesc const*		inFromWhichObject,
										 DescType			inGetDataOfThisType,
										 AEDesc*			outCount);

static OSStatus		selectObject		(AEDesc const*		inWhichObject,
										 AEDesc*			outReply);

static OSStatus		selectToken			(AEDesc const*		inObjectTokenDescriptor);



//
// public methods
//

/*!
Handles the following event types:

kAECoreSuite
------------
	kAEClose

(3.0)
*/
pascal OSErr
CoreSuiteAE_Close	(AppleEvent const*	inAppleEventPtr,
					 AppleEvent*		outReplyAppleEventPtr, 
					 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: ÒcloseÓ event");
	
	error = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
	if (error == noErr)
	{
		AEDesc		directObject;
		
		
		(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
		error = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
		if (error == noErr)
		{
			Size		actualSize = 0L;
			DescType	returnedType,
						saveOption = kAEAsk; // unless otherwise specified, ask whether to discard open windows
			
			
			// if the optional parameter was given as to whether to close
			// open windows without prompting the user, retrieve it
			error = AEGetParamPtr(inAppleEventPtr, keyAESaveOptions, typeEnumerated, &returnedType,
									&saveOption, sizeof(saveOption), &actualSize);
			if (error == errAEDescNotFound) error = noErr;
			if (error == noErr)
			{
				// closes the specified object; if itÕs a terminal window and save-prompting
				// is specified and the window contains unsaved changes, the event will
				// suspend until the user responds to an alert message
				error = closeObject(inAppleEventPtr, outReplyAppleEventPtr, saveOption);
			}
		}
		if (directObject.dataHandle != nullptr) AEDisposeDesc(&directObject);
	}
	
	Console_WriteValue("error", error);
	Console_EndFunction();
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// Close


/*!
Handles the following event types:

kAECoreSuite
------------
	kAECountElements

(3.0)
*/
pascal OSErr
CoreSuiteAE_CountElements	(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr, 
							 SInt32				UNUSED_ARGUMENT(inData))
{
	AEDesc		directObject,
				replyDesc;
	Size		actualSize = 0L;
	OSErr		result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: ÒcountÓ event");
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
	(OSStatus)AppleEventUtilities_InitAEDesc(&replyDesc);
	
	result = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
	
	// set up class parameter (required) and then handle the event
	if (result == noErr)
	{
		DescType		returnedType = typeWildCard;
		DescType		classType = typeNull;
		
		
		result = AEGetParamPtr(inAppleEventPtr, keyAEParamMyEach, typeType, &returnedType, &classType,
								sizeof(classType), &actualSize);
		if (result == noErr) result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (result == noErr) result = STATIC_CAST(countElements(&directObject, classType, &replyDesc), OSErr);
	}
	
	result = STATIC_CAST(AppleEventUtilities_AddResultToReply(&replyDesc, outReplyAppleEventPtr, result), OSErr);
	
	if (directObject.dataHandle != nullptr) AEDisposeDesc(&directObject);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// CountElements


/*!
This callback routine, of OSLCountUPP form,
counts the number of items in the specified
container object.

Incomplete.

(3.0)
*/
pascal OSErr
CoreSuiteAE_CountElementsOfContainer	(DescType		inTypeOfCountedElements,
										 DescType		inContainerClass,
										 AEDesc const*	inContainerTokenDescriptor,
										 SInt32*		outCount)
{
	OSErr		result = noErr;
	AEDesc		containerDesc;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc);
	
	// ensure the data is a token
	result = STATIC_CAST(AppleEventUtilities_DuplicateOrResolveObject
							(inContainerTokenDescriptor, &containerDesc), OSErr);
	if (result == noErr)
	{
		// initialize count to illegal value to catch errors
		*outCount = -1L;
		
		switch (inTypeOfCountedElements)
		{
			case cMyWindow:
				// count all open windows
				*outCount = 0L;
				if ((inContainerClass == typeNull) || (inContainerClass == cApplication))
				{
					WindowRef	window = EventLoop_ReturnRealFrontWindow();
					
					
					while (window != nullptr)
					{
						++(*outCount);
						window = GetNextWindow(window);
					}
				}
				break;
			
			case cMySessionWindow:
				// count all open session windows
				*outCount = SessionFactory_ReturnCount();
				break;
			
			case cMyTerminalWindow:
				// count all open terminal windows, which includes both sessions and simple consoles
				// unimplemented!
				*outCount = SessionFactory_ReturnCount();
				break;
			
			case cMyMacroSet:
				// count all macro sets
				*outCount = 0L;
				if (inContainerClass == typeNull)
				{
					// this value is currently a constant
					*outCount = MACRO_SET_COUNT;
				}
				break;
			
			case cMyMacro:
				// count all macros in a set
				*outCount = 0L;
				if (inContainerClass == cMyMacroSet)
				{
					// this value is currently a constant
					*outCount = MACRO_COUNT;
				}
				break;
			
			case cMyApplicationPreferences:
			case cMyClipboardWindow:
			case cMyApplication:
				// return 1 for objects that exist in exactly once place (who knows
				// why theyÕd ever be counted, but hey, itÕs gotta be correct...)
				*outCount = 1L;
				break;
			
			default:
				result = errAECantHandleClass;
				break;
		}
	}
	return result;
}// CountElementsOfContainer


/*!
Handles the following event types:

kAECoreSuite
------------
	kAESelect

(3.0)
*/
pascal OSErr
CoreSuiteAE_SelectObject	(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr, 
							 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	AEDesc        	directObject,
					replyDesc;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: ÒselectÓ event");
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
	(OSStatus)AppleEventUtilities_InitAEDesc(&replyDesc);
	
	error = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
	
	// set up parameters (none for this event) and then handle the event
	if (error == noErr)
	{
		error = STATIC_CAST(selectObject(&directObject, &replyDesc), OSErr);
	}
	
	error = STATIC_CAST(AppleEventUtilities_AddResultToReply
						(&replyDesc, outReplyAppleEventPtr, error), OSErr);
	
	if (directObject.dataHandle != nullptr) AEDisposeDesc(&directObject);
	
	Console_WriteValue("error", error);
	Console_EndFunction();
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// SelectObject


//
// internal methods
//
#pragma mark -

/*!
Closes one or more objects, given a reference
or a token.

(3.0)
*/
static OSStatus
closeObject		(AEDesc const*	inWhat,
				 AEDesc*		outReply,
				 DescType		inSaveOption)
{
	AEDesc		tokenDescriptor;
	OSStatus	result = noErr;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	
	result = AppleEventUtilities_DuplicateOrResolveObject(inWhat, &tokenDescriptor);
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		Console_WriteValueFourChars("to type", tokenDescriptor.descriptorType);
		switch (tokenDescriptor.descriptorType)
		{
			case typeAEList:
				result = AECreateList(nullptr, 0, false/* is a record */, outReply);
				if (result == noErr)
				{
					AEDesc		listItemDescriptor,
								resultDescriptor;
					DescType	returnedType;
					long		itemIndex = 0L;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&listItemDescriptor);
					(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
					
					result = AECountItems(&tokenDescriptor, &itemIndex);
					for (; itemIndex > 0; --itemIndex)
					{
						result = AEGetNthDesc(&tokenDescriptor, itemIndex, typeWildCard,
												&returnedType, &listItemDescriptor);
						
						// handle the request by adding the resultant data to the end of the list
						if (result == noErr) result = closeObject(&listItemDescriptor, &resultDescriptor,
																	inSaveOption);
						if (result == noErr) result = AEPutDesc(&tokenDescriptor/* list */, itemIndex,
																&resultDescriptor/* item to add */);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
						if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
					}
					result = AEDuplicateDesc(&tokenDescriptor, outReply);
				}
				break;
			
			default:
				result = closeToken(&tokenDescriptor, inSaveOption);
				// construct reply descriptor here - currently there does not need to be one
				break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	return result;
}// closeObject


/*!
Closes the specified object, using its token.
Depending on the type of object, this could
have different results: for example, a window
is closed by possibly interacting with the
user and removing the window from the screen,
but a file is closed by flushing buffers and
calling the File Manager.

(3.0)
*/
static OSStatus
closeToken	(AEDesc const*	inObjectTokenDescriptor,
			 DescType		inSaveOption)
{
	OSStatus				result = noErr;
	AEDesc					objectDesc;
	ObjectClassesAE_Token   token;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
	
	// ensure the data is a token
	result = AppleEventUtilities_DuplicateOrResolveObject(inObjectTokenDescriptor, &objectDesc);
	if (result == noErr) result = GenesisAE_CreateTokenFromObjectSpecifier(inObjectTokenDescriptor, &token);
	if (result == noErr)
	{
		Console_WriteValueFourChars("token describes object of type", token.as.object.eventClass);
		if (GenesisAE_FirstClassIs(token.as.object.eventClass, cMySessionWindow))
		{
			// close session window
			// UNIMPLEMENTED
			Sound_StandardAlert();
			result = userCanceledErr;
		}
		else if (GenesisAE_FirstClassIs(token.as.object.eventClass, cMyWindow))
		{
			// close some other type of window
			// TEMPORARY!!!
			//CloseWindow(token.as.object.data.window.ref);
		}
		else
		{
			// closing objects of the given type is not supported or not possible
			result = errAECantHandleClass;
		}
	}
	return result;
}// closeToken


/*!
Returns a count of the elements in the specified
container (or a sum of the counts in a list of
containers).

(3.0)
*/
static OSStatus
countElements		(AEDesc const*		inFromWhichObject,
					 DescType			inGetDataOfThisType,
					 AEDesc*			outCount)
{
	AEDesc		tokenDescriptor;
	OSStatus	result = noErr;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	
	Console_WriteValueFourChars("desired type", inGetDataOfThisType);
	
	result = AppleEventUtilities_DuplicateOrResolveObject(inFromWhichObject, &tokenDescriptor);
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		Console_WriteValueFourChars("to type", tokenDescriptor.descriptorType);
		switch (tokenDescriptor.descriptorType)
		{
			case typeAEList:
				result = AECreateList(nullptr, 0, false/* is a record */, outCount);
				if (result == noErr)
				{
					AEDesc		listItemDescriptor,
								resultDescriptor;
					DescType	returnedType;
					long		itemIndex = 0L;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&listItemDescriptor);
					(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
					
					result = AECountItems(&tokenDescriptor, &itemIndex);
					for (; itemIndex > 0; --itemIndex)
					{
						result = AEGetNthDesc(&tokenDescriptor, itemIndex, typeWildCard,
												&returnedType, &listItemDescriptor);
						
						// handle the request by adding the resultant data to the end of the list
						if (result == noErr) result = countElements(&listItemDescriptor,
																	inGetDataOfThisType, &resultDescriptor);
						if (result == noErr) result = AEPutDesc(&tokenDescriptor/* list */, itemIndex,
																&resultDescriptor/* item to add */);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
						if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
					}
					result = AEDuplicateDesc(&tokenDescriptor, outCount);
				}
				break;
			
			default:
				{
					SInt32		elementCount = 0L;
					
					
					result = CoreSuiteAE_CountElementsOfContainer(inGetDataOfThisType,
																	tokenDescriptor.descriptorType,
																	&tokenDescriptor, &elementCount);
					if (result == noErr) result = AECreateDesc(valueAEResultParamForEvent_count, &elementCount,
																sizeof(elementCount), outCount);
				}
				break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	return result;
}// countElements


/*!
Selects one or more objects, given a reference
or a token.

(3.0)
*/
static OSStatus
selectObject		(AEDesc const*		inWhichObject,
					 AEDesc*			outReply)
{
	AEDesc		tokenDescriptor;
	OSStatus	result = noErr;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	
	result = AppleEventUtilities_DuplicateOrResolveObject(inWhichObject, &tokenDescriptor);
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		Console_WriteValueFourChars("to type", tokenDescriptor.descriptorType);
		switch (tokenDescriptor.descriptorType)
		{
			case typeAEList:
				result = AECreateList(nullptr, 0, false/* is a record */, outReply);
				if (result == noErr)
				{
					AEDesc		listItemDescriptor,
								resultDescriptor;
					DescType	returnedType;
					long		itemIndex = 0L;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&listItemDescriptor);
					(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
					
					result = AECountItems(&tokenDescriptor, &itemIndex);
					for (; itemIndex > 0; --itemIndex)
					{
						result = AEGetNthDesc(&tokenDescriptor, itemIndex, typeWildCard,
												&returnedType, &listItemDescriptor);
						
						// handle the request by adding the resultant data to the end of the list
						if (result == noErr) result = selectObject(&listItemDescriptor, &resultDescriptor);
						if (result == noErr) result = AEPutDesc(&tokenDescriptor/* list */, itemIndex,
																&resultDescriptor/* item to add */);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
						if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
					}
					result = AEDuplicateDesc(&tokenDescriptor, outReply);
				}
				break;
			
			default:
				result = selectToken(&tokenDescriptor);
				// construct reply descriptor here - currently there does not need to be one
				break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	return result;
}// selectObject


/*!
Selects the specified object, using its token.
Depending on the type of object, this could
have different results: for example, a window
is selected by bringing it to the front, but
text is selected by highlighting it.

(3.0)
*/
static OSStatus
selectToken		(AEDesc const*		inObjectTokenDescriptor)
{
	OSStatus				result = noErr;
	AEDesc					objectDesc;
	ObjectClassesAE_Token   token;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
	
	// ensure the data is a token
	result = AppleEventUtilities_DuplicateOrResolveObject(inObjectTokenDescriptor, &objectDesc);
	if (result == noErr) result = GenesisAE_CreateTokenFromObjectSpecifier(inObjectTokenDescriptor, &token);
	if (result == noErr)
	{
		Console_WriteValueFourChars("token describes object of type", token.as.object.eventClass);
		if (GenesisAE_FirstClassIs(token.as.object.eventClass, cMyWindow))
		{
			// bring a window in front of all other windows of its type
			EventLoop_SelectBehindDialogWindows(token.as.object.data.window.ref);
			result = noErr;
		}
		else
		{
			// selecting objects of the given type is not supported or not possible
			result = errAECantHandleClass;
		}
	}
	return result;
}// selectToken

// BELOW IS REQUIRED NEWLINE TO END FILE
