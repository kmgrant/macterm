/*###############################################################

	ConnectionAE.cp
	
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
#include <cctype>
#include <cstring>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <MemoryBlocks.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "BasicTypesAE.h"
#include "ConnectionAE.h"
#include "GenesisAE.h"
#include "ObjectClassesAE.h"
#include "Session.h"
#include "Terminology.h"



#pragma mark Internal Method Prototypes

static OSStatus			expectData		(AEDesc const*, AEDesc const*, UInt32);
static void				sendCR			(SessionRef, Boolean);
static OSStatus			sendData		(AEDesc const*, AEDesc const*, Boolean, Boolean);



#pragma mark Public Methods

/*!
Handles the following event types:

kMySuite
--------
	kTelnetEventIDConnectionOpen

(3.0)
*/
pascal OSErr
ConnectionAE_HandleConnectionOpen	(AppleEvent const*		inAppleEventPtr,
									 AppleEvent*			outReplyAppleEventPtr,
									 SInt32					UNUSED_ARGUMENT(inData))
{
	DescType			returnedType = typeWildCard;
	Str255				hostText;
	Str255				titleText;
	Size				actualSize = 0L;
	OSErr				result = noErr;
	OSStatus			error = noErr; // generic error with no lasting meaning
	
	
	// set up host parameter: this is a required parameter
	{
		error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyToHost, typeChar, &returnedType, hostText + 1,
								sizeof(hostText) - (1 * sizeof(UInt8)), &actualSize);
		hostText[0] = (UInt8)(actualSize / sizeof(UInt8));
		if (error != noErr) PLstrcpy(hostText, "\p");
	}
	
	// set up port-number parameter: defaults to the telnet port if the parameter is not specified
	{
		SInt16		port = 23; // the default telnet port
		
		
		error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyTCPPort, typeSInt16, &returnedType, &port,
								sizeof(port), &actualSize);
		if (error == noErr)
		{
			Str31		portString;
			SInt32		portAsLong = port;
			
			
			NumToString(portAsLong, portString);
			PLstrcat(hostText, "\p:");
			PLstrcat(hostText, portString);
		}
	}
	
	// set up window title parameter: defaults to nothing if the parameter is not specified
	{
		error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyName, typeChar, &returnedType, titleText + 1,
								sizeof(titleText) - (1 * sizeof(UInt8)), &actualSize);
		titleText[0] = (UInt8)(actualSize / sizeof(UInt8));
		if (error != noErr) PLstrcpy(titleText, "\p");
	}
	
	{
	#if 0
		// UNIMPLEMENTED
		// set up connection method parameter: defaults to telnet if the parameter is not specified
		{
			FourCharCode	method = kTelnetEnumeratedClassConnectionMethodTelnet;
			
			
			error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyProtocol, valueAEParamMyProtocol, &returnedType, &method,
									sizeof(method), &actualSize);
			(**theConnectionDataHandle).method = method;
		}
	#endif
		
	#if 0
		// UNIMPLEMENTED
		// set up open timeout parameter: defaults to the preferences value if the parameter is not specified
		{
			UInt32		timeout = 10; // assume a 10-second timeout, preference is obsolete
			
			
			error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyGivingUpAfter, typeUInt32, &returnedType, &timeout,
									sizeof(timeout), &actualSize);
			(**theConnectionDataHandle).openTimeout = timeout;
		}
	#endif
		
		// temp - somehow, this call produces an error, because this event specifies it has no direct object?
		//result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (result == noErr)
		{
			// make the connection
			// UNIMPLEMENTED
			
			// check to see if the reply is valid - if so, return a connection record
			if (AppleEventUtilities_IsValidReply(outReplyAppleEventPtr))
			{
				// return the new connection - incomplete
				SInt32		junk = 0L;
				
				
				error = AEPutParamPtr(outReplyAppleEventPtr, keyDirectObject, cMyConnection, &junk/* wrong, temp. */,
										sizeof(SInt32/* wrong, temp. */));
			}
		}
		
		if (result != noErr)
		{
			// throw an exception
			AEDesc		errorDesc;
			
			
			(OSStatus)AppleEventUtilities_InitAEDesc(&errorDesc);
			(OSStatus)BasicTypesAE_CreateSInt32Desc(result, &errorDesc);
			(OSStatus)AppleEventUtilities_AddResultToReply(&errorDesc, outReplyAppleEventPtr, result);
		}
	}
	
	return result;
}// HandleConnectionOpen


/*!
Handles the following event types:

kMySuite
--------
	kTelnetEventIDDataSend

(3.0)
*/
pascal OSErr
ConnectionAE_HandleSend		(AppleEvent const*		inAppleEventPtr,
							 AppleEvent*			UNUSED_ARGUMENT(outReplyAppleEventPtr),
							 SInt32					UNUSED_ARGUMENT(inData))
{
	AEDesc		directObject,
				sessionWindowObject;
	Size		actualSize = 0L;
	OSErr		result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “send” event");
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
	(OSStatus)AppleEventUtilities_InitAEDesc(&sessionWindowObject);
	
	result = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
	
	// set up parameters
	if (result == noErr)
	{
		DescType	returnedType = typeWildCard;
		
		
		// get required parameters first
		result = AEGetParamDesc(inAppleEventPtr, keyAEParamMyToDestination, typeObjectSpecifier, &sessionWindowObject);
		if (result == noErr)
		{
			Boolean		echo = false,
						newLine = false;
			
			
			// get echo parameter; defaults to false if not specified
			(OSStatus)AEGetParamPtr(inAppleEventPtr, keyAEParamMyEchoing, typeBoolean, &returnedType, &echo,
									sizeof(echo), &actualSize);
			
			// get newline parameter; defaults to false if not specified
			(OSStatus)AEGetParamPtr(inAppleEventPtr, keyAEParamMyNewline, typeBoolean, &returnedType, &newLine,
									sizeof(newLine), &actualSize);
			
			// now execute the command, if no required parameters are missing
			result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
			if (result == noErr) result = STATIC_CAST(sendData(&directObject, &sessionWindowObject, echo, newLine), OSErr);
		}
	}
	
	if (directObject.dataHandle != nullptr) AEDisposeDesc(&directObject);
	if (sessionWindowObject.dataHandle != nullptr) AEDisposeDesc(&sessionWindowObject);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// HandleSend


/*!
Handles the following event types:

kMySuite
--------
	kTelnetEventIDDataWait

(3.0)
*/
pascal OSErr
ConnectionAE_HandleWait		(AppleEvent const*		inAppleEventPtr,
							 AppleEvent*			UNUSED_ARGUMENT(outReplyAppleEventPtr),
							 SInt32					UNUSED_ARGUMENT(inData))
{
	AEDesc        	directObject,
					sessionWindowObject;
	Size			actualSize = 0L;
	OSErr         	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “expect” event");
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
	(OSStatus)AppleEventUtilities_InitAEDesc(&sessionWindowObject);
	
	result = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
	
	// set up parameters
	if (result == noErr)
	{
		DescType		returnedType = typeWildCard;
		
		
		// get required parameters first
		result = AEGetParamDesc(inAppleEventPtr, keyAEParamMyFromSource, typeObjectSpecifier, &sessionWindowObject);
		if (result == noErr)
		{
			UInt32		timeout = 10000L;
			
			
			// get timeout parameter; defaults to 10 seconds if not specified
			(OSStatus)AEGetParamPtr(inAppleEventPtr, keyAEParamMyGivingUpAfter, typeUInt32, &returnedType, &timeout,
									sizeof(timeout), &actualSize);
			
			// now execute the command, if no required parameters are missing
			result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
			if (result == noErr) result = AESuspendTheCurrentEvent(inAppleEventPtr);
			if (result == noErr) result = STATIC_CAST(expectData(&directObject, &sessionWindowObject, timeout), OSErr);
		}
	}
	
	if (directObject.dataHandle != nullptr) AEDisposeDesc(&directObject);
	if (sessionWindowObject.dataHandle != nullptr) AEDisposeDesc(&sessionWindowObject);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// HandleWait


#pragma mark Internal Methods

/*!
Waits for the specified data from the specified
session window.

The reply is undefined.

INCOMPLETE

(3.0)
*/
static OSStatus
expectData	(AEDesc const*		inExpectWhat,
			 AEDesc const*		inFromWhichSessionWindow,
			 UInt32				inTimeout)
{
	AEDesc		tokenDescriptor,
				sessionWindowDescriptor;
	OSStatus	result = noErr;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	(OSStatus)AppleEventUtilities_InitAEDesc(&sessionWindowDescriptor);
	
	result = AppleEventUtilities_DuplicateOrResolveObject(inExpectWhat, &tokenDescriptor);
	if (result == noErr)
	{
		result = AppleEventUtilities_DuplicateOrResolveObject(inFromWhichSessionWindow, &sessionWindowDescriptor);
		Console_WriteValueFourChars("session window object type", sessionWindowDescriptor.descriptorType);
	}
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		Console_WriteValueFourChars("to type", tokenDescriptor.descriptorType);
		switch (tokenDescriptor.descriptorType)
		{
			case typeAEList:
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
						if (result == noErr) result = expectData(&listItemDescriptor, inFromWhichSessionWindow, inTimeout);
						if (result == noErr) result = AEPutDesc(&tokenDescriptor/* list */, itemIndex,
																&resultDescriptor/* item to add */);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
						if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
					}
				}
				break;
			
			case typeEnumerated:
				Console_WriteLine("special character");
				{
					FourCharCode	specialCharacter = kTelnetEnumeratedClassSpecialCharacterCarriageReturn;
					Size			actualSize = 0L;
					OSStatus		error = noErr;
					
					
					error = AppleEventUtilities_CopyDescriptorData(&tokenDescriptor, &specialCharacter,
																	sizeof(specialCharacter), &actualSize);
					if (error == noErr)
					{
						ObjectClassesAE_Token   token;
						SessionRef				session = nullptr;
						
						
						if (GenesisAE_CreateTokenFromObjectSpecifier(&sessionWindowDescriptor, &token) == noErr)
						{
							session = token.as.object.data.sessionWindow.session;
						}
						else Console_WriteLine("failed to get token data");
						
						if (session != nullptr) switch (specialCharacter)
						{
							case kTelnetEnumeratedClassSpecialCharacterAbortOutput:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterAreYouThere:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterBreak:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterCarriageReturn:
								sendCR(session, false/* echo */);
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterEndOfFile:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterEraseCharacter:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterEraseLine:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterInterruptProcess:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterLineFeed:
								Session_SendFlushData(session, "\012", 1); // LF
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterSync:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							default:
								// ???
								break;
						}
					}
				}
				break;
			
			default:
				Console_WriteLine("string");
				{
					Ptr		string = nullptr;
					
					
					string = Memory_NewPtr(AEGetDescDataSize(&tokenDescriptor));
					if (string != nullptr)
					{
						Size		actualSize = 0L;
						OSStatus	error = noErr;
						
						
						error = AppleEventUtilities_CopyDescriptorDataAs(cStringClass, &tokenDescriptor, string,
																			GetPtrSize(string), &actualSize);
						if (error == noErr)
						{
							ObjectClassesAE_Token   token;
							SessionRef				session = nullptr;
							
							
							Console_WriteLine("request to expect this line:");
							Console_WriteLine(string);
							if (GenesisAE_CreateTokenFromObjectSpecifier(&sessionWindowDescriptor, &token) == noErr)
							{
								session = token.as.object.data.sessionWindow.session;
								if (session != nullptr)
								{
									// incomplete
									//???
								}
							}
							else Console_WriteLine("failed to get token data");
						}
					}
					else result = memFullErr;
				}
				break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	return result;
}// expectData


/*!
Sends a carriage return sequence.  Depending upon the
user preference of carriage return handling, this may
send CR-LF or CR-null.

(3.0)
*/
static void
sendCR	(SessionRef		inSession,
		 Boolean		inEcho)
{
	Session_SendNewline(inSession, inEcho ? kSession_EchoEnabled : kSession_EchoDisabled);
}// sendCR


/*!
Sends the specified data to the specified session window.
This routine recognizes string data, lists of strings,
and enumerations indicating special characters.

The reply is undefined.

(3.0)
*/
static OSStatus
sendData	(AEDesc const*		inSendWhat,
			 AEDesc const*		inToWhichSessionWindow,
			 Boolean			inEcho,
			 Boolean			inNewLine)
{
	AEDesc		tokenDescriptor,
				sessionWindowDescriptor;
	OSStatus	result = noErr;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	(OSStatus)AppleEventUtilities_InitAEDesc(&sessionWindowDescriptor);
	
	result = AppleEventUtilities_DuplicateOrResolveObject(inSendWhat, &tokenDescriptor);
	if (result == noErr)
	{
		result = AppleEventUtilities_DuplicateOrResolveObject(inToWhichSessionWindow, &sessionWindowDescriptor);
		Console_WriteValueFourChars("session window object type", sessionWindowDescriptor.descriptorType);
	}
	if (result == noErr)
	{
		Console_WriteLine("successfully resolved");
		Console_WriteValueFourChars("to type", tokenDescriptor.descriptorType);
		switch (tokenDescriptor.descriptorType)
		{
			case typeAEList:
				{
					AEDesc		listItemDescriptor,
								resultDescriptor;
					DescType	returnedType;
					long		numberOfItems = 0;
					long		itemIndex = 0L;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&listItemDescriptor);
					(OSStatus)AppleEventUtilities_InitAEDesc(&resultDescriptor);
					
					result = AECountItems(&tokenDescriptor, &numberOfItems);
					for (itemIndex = 1; itemIndex <= numberOfItems; ++itemIndex)
					{
						result = AEGetNthDesc(&tokenDescriptor, itemIndex, typeWildCard,
												&returnedType, &listItemDescriptor);
						
						// handle the request by adding the resultant data to the end of the list
						if (result == noErr) result = sendData(&listItemDescriptor, inToWhichSessionWindow, inEcho, inNewLine);
						if (result == noErr) result = AEPutDesc(&tokenDescriptor/* list */, itemIndex,
																&resultDescriptor/* item to add */);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
						if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
					}
				}
				break;
			
			case typeEnumerated:
				Console_WriteLine("special character");
				{
					FourCharCode	specialCharacter = kTelnetEnumeratedClassSpecialCharacterCarriageReturn;
					Size			actualSize = 0L;
					OSStatus		error = noErr;
					
					
					error = AppleEventUtilities_CopyDescriptorData(&tokenDescriptor, &specialCharacter,
																	sizeof(specialCharacter), &actualSize);
					if (error == noErr)
					{
						ObjectClassesAE_Token   token;
						SessionRef				session = nullptr;
						
						
						if (GenesisAE_CreateTokenFromObjectSpecifier(&sessionWindowDescriptor, &token) == noErr)
						{
							session = token.as.object.data.sessionWindow.session;
						}
						else Console_WriteLine("failed to get token data");
						
						if (session != nullptr) switch (specialCharacter)
						{
							case kTelnetEnumeratedClassSpecialCharacterAbortOutput:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterAreYouThere:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterBreak:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterCarriageReturn:
								sendCR(session, false/* echo */);
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterEndOfFile:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterEraseCharacter:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterEraseLine:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterInterruptProcess:
								Session_UserInputInterruptProcess(session);
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterLineFeed:
								Session_SendFlushData(session, "\012", 1); // LF
								break;
							
							case kTelnetEnumeratedClassSpecialCharacterSync:
								Console_WriteLine("WARNING, unimplemented: send special char.");
								break;
							
							default:
								// ???
								break;
						}
					}
				}
				break;
			
			default:
				Console_WriteLine("string");
				{
					Ptr		string = nullptr;
					
					
					string = Memory_NewPtr(AEGetDescDataSize(&tokenDescriptor));
					if (string != nullptr)
					{
						Size		actualSize = 0L;
						OSStatus	error = noErr;
						
						
						error = AppleEventUtilities_CopyDescriptorDataAs(cStringClass, &tokenDescriptor, string,
																			GetPtrSize(string), &actualSize);
						if (error == noErr)
						{
							ObjectClassesAE_Token   token;
							SessionRef				session = nullptr;
							
							
							Console_WriteLine("request to send this line:");
							Console_WriteLine(string);
							if (GenesisAE_CreateTokenFromObjectSpecifier(&sessionWindowDescriptor, &token) == noErr)
							{
								session = token.as.object.data.sessionWindow.session;
								Session_UserInputString(session, string, GetPtrSize(string),
														false/* send to recording scripts */);
								if (inNewLine) sendCR(session, inEcho);
							}
							else Console_WriteLine("failed to get token data");
						}
					}
					else result = memFullErr;
				}
				break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	return result;
}// sendData

// BELOW IS REQUIRED NEWLINE TO END FILE
