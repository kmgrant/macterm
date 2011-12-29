/*!	\file URLAccessAE.cp
	\brief Implements part of the URL Access suite of Apple
	Events (currently, MacTerm only supports "handle URL").
*/
/*###############################################################

	MacTerm
		© 1998-2011 by Kevin Grant.
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

#include "URLAccessAE.h"
#include <UniversalDefines.h>

// standard-C++ includes
#include <stdexcept>

// Unix includes
#include <sys/param.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <Console.h>
#include <MemoryBlocks.h>

// application includes
#include "AppleEventUtilities.h"
#include "QuillsPrefs.h"
#include "QuillsSession.h"
#include "Terminology.h"



#pragma mark Internal Method Prototypes
namespace {

OSStatus	handleURL	(AEDesc const*, AEDesc*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Handles the following event types:

kStandardURLSuite
-----------------
	kStandardURLEventIDGetURL

(3.0)
*/
OSErr
URLAccessAE_HandleUniformResourceLocator		(AppleEvent const*	inAppleEventPtr,
												 AppleEvent*		outReplyAppleEventPtr,
												 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	AEDesc			directObject;
	
	
	Console_BeginFunction();
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&directObject);
	
	Console_WriteLine("AppleScript: “handle URL” event");
	
	error = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeWildCard, &directObject);
	
	if (error == noErr)
	{
		// check for missing parameters
		error = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (error == noErr) error = handleURL(&directObject, outReplyAppleEventPtr);
	}
	
	if (directObject.dataHandle) AEDisposeDesc(&directObject);
	
	Console_WriteValue("result", error);
	Console_EndFunction();
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleUniformResourceLocator


#pragma mark Internal Methods
namespace {

/*!
Handles the specified URL by either opening a telnet
session or launching the appropriate helper application.
If a list of URLs is given, this routine recursively
handles each URL in the list, returning a list of result
codes.  A result code of 0 indicates success!!!

(3.0)
*/
OSStatus
handleURL	(AEDesc const*		inFromWhichObject,
			 AEDesc*			outResult)
{
	AEDesc		tokenDescriptor;
	OSStatus	result = noErr;
	
	
	(OSStatus)AppleEventUtilities_InitAEDesc(&tokenDescriptor);
	
	result = AppleEventUtilities_DuplicateOrResolveObject(inFromWhichObject, &tokenDescriptor);
	if (result == noErr)
	{
		switch (tokenDescriptor.descriptorType)
		{
			case typeAEList:
				result = AECreateList(nullptr, 0, false/* is a record */, outResult);
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
						if (result == noErr) result = handleURL(&listItemDescriptor, &resultDescriptor);
						if (result == noErr) result = AEPutDesc(&tokenDescriptor/* list */, itemIndex,
																&resultDescriptor/* item to add */);
						
						if (listItemDescriptor.dataHandle != nullptr) AEDisposeDesc(&listItemDescriptor);
						if (resultDescriptor.dataHandle != nullptr) AEDisposeDesc(&resultDescriptor);
					}
					result = AEDuplicateDesc(&tokenDescriptor, outResult);
				}
				break;
			
			default:
				{
					Handle		handle = Memory_NewHandle(AEGetDescDataSize(&tokenDescriptor));
					SInt16		resultCode = 0;
					OSStatus	errorCausingNonzeroResult = noErr;
					
					
					if (handle == nullptr)
					{
						resultCode = 1; // failure
						errorCausingNonzeroResult = memPCErr;
					}
					else
					{
						Size	actualSize = 0L;
						
						
						result = AppleEventUtilities_CopyDescriptorData(&tokenDescriptor, *handle,
																		GetHandleSize(handle), &actualSize);
						if (result == noErr)
						{
							std::string					urlString(*handle, *handle + GetHandleSize(handle));
							std::string::size_type		indexOfColon = urlString.find_first_of(':');
							std::string::size_type		indexOfExtension = urlString.find_last_of('.');
							std::string					urlType(urlString.substr(0, indexOfColon));
							std::string					extensionName(urlString.substr
																		(indexOfExtension + 1/* skip dot */,
																			urlString.npos));
							
							
							try
							{
								// the mechanism for opening files is actually not consistent;
								// sometimes a Finder icon may cause an open-documents event,
								// but other times it may be passed in as an open-URL (for a
								// "file" URL type); this inconsistency is a bit maddening
								// and this code only handles the URL variant so please be
								// sure to keep it consistent with the open-document handler
								if (urlType == "file")
								{
									CFRetainRelease		urlCFString(CFStringCreateWithCString
																	(kCFAllocatorDefault, urlString.c_str(),
																		kCFStringEncodingUTF8),
																	true/* is retained */);
									CFRetainRelease		urlCFURL(CFURLCreateWithBytes
																	(kCFAllocatorDefault,
																		REINTERPRET_CAST
																		(urlString.data(), UInt8 const*),
																		urlString.size(), kCFStringEncodingUTF8,
																		nullptr/* base URL */),
																	true/* is retained */);
									UInt8				filePath[MAXPATHLEN];
									Boolean				importOK = false;
									
									
									// note: CFURLGetFileSystemRepresentation() null-terminates the result
									if (CFURLGetFileSystemRepresentation
										(CFUtilities_URLCast(urlCFURL.returnCFTypeRef()), true/* return absolute path */,
											filePath, sizeof(filePath)))
									{
										char const*		kAsCString = REINTERPRET_CAST(filePath, char const*);
										
										
										if ((extensionName == "plist") || (extensionName == "xml"))
										{
											Quills::Prefs::import_from_file(kAsCString, true/* allow renaming */);
											importOK = true;
										}
										else
										{
											Quills::Session::handle_file(kAsCString);
											importOK = true;
										}
									}
									
									if (false == importOK)
									{
										throw std::runtime_error("unspecified problem interpreting URL bytes");
									}
								}
								else
								{
									Quills::Session::handle_url(urlString);
								}
							}
							catch (std::exception const&	e)
							{
								CFStringRef			titleCFString = CFSTR("Exception while trying to handle URL from Apple Event"); // LOCALIZE THIS
								CFRetainRelease		messageCFString(CFStringCreateWithCString
																	(kCFAllocatorDefault, e.what(), kCFStringEncodingUTF8),
																	true/* is retained */); // LOCALIZE THIS?
								
								
								Console_WriteScriptError(titleCFString, messageCFString.returnCFStringRef());
								resultCode = 1; // failure
								errorCausingNonzeroResult = eventNotHandledErr;
							}
						}
						else
						{
							resultCode = 1; // failure
							errorCausingNonzeroResult = result;
						}
						Memory_DisposeHandle(&handle);
					}
					if (resultCode)
					{
						// mention the failure in an error response
						result = AppleEventUtilities_AddErrorToReply(nullptr/* no particular error message */,
																		errorCausingNonzeroResult,
																		outResult);
					}
				}
				break;
		}
	}
	
	if (tokenDescriptor.dataHandle != nullptr) AEDisposeToken(&tokenDescriptor);
	
	return result;
}// handleURL

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
