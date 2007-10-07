/*###############################################################

	URL.cp
	
	MacTelnet
		© 1998-2007 by Kevin Grant.
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

// standard-C++ includes
#include <algorithm>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <Console.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "BasicTypesAE.h"
#include "DialogUtilities.h"
#include "InternetPrefs.h"
#include "RecordAE.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "URL.h"



#pragma mark Constants

#define CR			0x0D	/* the carriage return character */

#define IS_WHITE_SPACE_CHARACTER(a)			(' ' == (a) || '\t' == (a))
#define IS_WHITE_SPACE_OR_CR_CHARACTER(a)	(IS_WHITE_SPACE_CHARACTER(a) || CR == (a))

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	char*	gURLSchemeNames[] =
	{
		// WARNING:	This MUST match the order of the types
		//			in "InternetPrefs.h", this is assumed
		//			within code in this file.
		":",
		"mailto:",	
		"news:",
		"nntp:",
		"ftp:",
		"http:",
		"gopher:",
		"wais:",
		"telnet:",
		"rlogin:",
		"tn3270:",
		"finger:",
		"whois:",
		"rtsp:",
		"ssh:",
		"sftp:",
		"x-man-page:",
		nullptr // this list must end with a nullptr
	};
}


#pragma mark Public Methods

/*!
Parses the given Core Foundation string and returns
what kind of URL it seems to represent.

Any whitespace on either end of the string is
stripped prior to checks.

FUTURE: Could probably rely on CFURLGetBytes() and
CFURLGetByteRangeForComponent() to do part of this
gruntwork, only in a way that is flexible to many
more types of URLs.

(3.1)
*/
UniformResourceLocatorType
URL_GetTypeFromCFString		(CFStringRef	inURLCFString)
{
	UniformResourceLocatorType	result = kNotURL;
	CFMutableStringRef			urlCopyCFString = CFStringCreateMutableCopy
													(kCFAllocatorDefault, 0/* length or 0 for unlimited */,
														inURLCFString);
	
	
	if (nullptr != urlCopyCFString)
	{
		CFStringTrimWhitespace(urlCopyCFString);
		{
			CFStringEncoding const	kEncoding = kCFStringEncodingUTF8;
			size_t const			kStringLength = CFStringGetLength(urlCopyCFString);
			size_t const			kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
														(kStringLength, kEncoding);
			char*					buffer = nullptr;
			
			
			buffer = new char[kBufferSize];
			if (CFStringGetCString(urlCopyCFString, buffer, kBufferSize, kEncoding))
			{
				result = URL_GetTypeFromCharacterRange(buffer, buffer + kStringLength);
			}
			delete [] buffer, buffer = nullptr;
		}
		CFRelease(urlCopyCFString), urlCopyCFString = nullptr;
	}
	
	return result;
}// GetTypeFromCFString


/*!
Parses the given data handle, assumed to be a
string, and returns what kind of URL it seems
to represent.

Whitespace is NOT stripped in this version of
the routine.

(3.1)
*/
UniformResourceLocatorType
URL_GetTypeFromCharacterRange	(char const*	inBegin,
								 char const*	inPastEnd)
{
	UniformResourceLocatorType	result = kNotURL;
	register SInt16				i = 0;
	
	
	// look for a match on the prefix (e.g. "http:")
	for (i = 0; (nullptr != gURLSchemeNames[i]); ++i)
	{
		char const* const	kCStringPtr = gURLSchemeNames[i];
		
		
		// obviously the URL must be at least as long as its prefix!
		if ((inPastEnd - inBegin) > STATIC_CAST(std::strlen(kCStringPtr), ptrdiff_t))
		{
			if (std::equal(kCStringPtr, kCStringPtr + std::strlen(kCStringPtr), inBegin))
			{
				result = STATIC_CAST(i, UniformResourceLocatorType);
				break;
			}
		}
	}
	return result;
}// GetTypeFromCharacterRange


/*!
Parses the given data handle, assumed to be a
string, and returns what kind of URL it seems
to represent.

Any whitespace on either end of the string is
stripped prior to checks.

(2.6)
*/
UniformResourceLocatorType
URL_GetTypeFromDataHandle	(Handle		inURLDataHandle)
{
	UniformResourceLocatorType	result = kNotURL;
	char const*					textBegin = *inURLDataHandle;
	char const*					textEnd = *inURLDataHandle +
											(GetHandleSize(inURLDataHandle) / sizeof(char));
	
	
	// strip off leading white space and lagging white space
	while (IS_WHITE_SPACE_OR_CR_CHARACTER(*textBegin) && textBegin < textEnd) ++textBegin;
	while (IS_WHITE_SPACE_OR_CR_CHARACTER(*textBegin) && textBegin < textEnd) --textEnd;
	
	// get URL type
	result = URL_GetTypeFromCharacterRange(textBegin, textEnd);
	
	return result;
}// GetTypeFromDataHandle


/*!
Examines the currently-selected text of the specified
terminal view for a valid URL.  If it finds one, the
text is flashed briefly and then the proper Apple Events
are fired to open the URL.

(2.6)
*/
void
URL_HandleForScreenView		(TerminalScreenRef	UNUSED_ARGUMENT(inScreen),
							 TerminalViewRef	inView)
{
	Handle						urlH = TerminalView_ReturnSelectedTextAsNewHandle
										(inView, 0, kTerminalView_TextFlagInline);
	UniformResourceLocatorType	urlKind = kNotURL;
	OSType						sig = '----';
	Boolean						isValidHandle = false;
	
	
	isValidHandle = IsHandleValid(urlH);
	if (isValidHandle)
	{
		if (GetHandleSize(urlH) <= 0) isValidHandle = false;
		else
		{
			HLock(urlH);
			
			urlKind = URL_GetTypeFromDataHandle(urlH);
			if (kNotURL != urlKind)
			{
				WindowRef	screenWindow = TerminalView_ReturnWindow(inView);
				//Boolean		selfHandling = URL_TypeIsHandledByAppItself(urlKind);
				AEDesc		eventReply; // contains errors (if any) returned from event
				OSStatus	urlOpenError = noErr;
				
				
				sig = InternetPrefs_HelperApplicationForURLType(urlKind);
				//if ((selfHandling) || (kInternetConfigUnknownHelper != sig))
				{
					TerminalView_FlashSelection(inView);
					//unless (selfHandling)
					{
						// display Òlaunch application zoom rectanglesÓ if an external helper is used
						RgnHandle	selectionRegion = TerminalView_ReturnSelectedTextAsNewRegion(inView);
						Rect		screenRect;
						Rect		selectionRect;
						
						
						if (nullptr != selectionRegion)
						{
							CGrafPtr	oldPort = nullptr;
							GDHandle	oldDevice = nullptr;
							
							
							GetGWorld(&oldPort, &oldDevice);
							SetPortWindowPort(screenWindow);
							LocalToGlobalRegion(selectionRegion);
							SetGWorld(oldPort, oldDevice);
							GetRegionBounds(selectionRegion, &selectionRect);
							RegionUtilities_GetWindowDeviceGrayRect(screenWindow, &screenRect);
							(OSStatus)ZoomRects(&selectionRect, &screenRect, 20/* steps, arbitrary */,
												kZoomAccelerate);
						}
					}
				}
				
				// 3.0 - use AppleScript to record this event into scripts
				isValidHandle = URL_SendGetURLEventToSelf(*urlH, GetHandleSize(urlH), &eventReply);
				if (AppleEventUtilities_RetrieveReplyError(&eventReply, &urlOpenError/* error that occurred */))
				{
					// in the event of an error, show Òdying zoom rectanglesÓ
					Rect	screenRect;
					Rect	selectionRect;
					
					
					SetRect(&selectionRect, 0, 0, 0, 0);
					RegionUtilities_GetWindowDeviceGrayRect(screenWindow, &screenRect);
					(OSStatus)ZoomRects(&screenRect, &selectionRect, 20/* steps, arbitrary */, kZoomDecelerate);
					Alert_ReportOSStatus(urlOpenError);
				}
			}
		}
	}
	
	unless (isValidHandle) Sound_StandardAlert();
}// HandleForScreenView


/*!
Determines whether MacTelnet supports URLs of the
specified type.

See URL_ParseCFString() for actual implementations
of URL handlers.

(3.0)
*/
Boolean
URL_TypeIsHandledByAppItself	(UniformResourceLocatorType		inType)
{
	Boolean		result = false;
	
	
	result = (
				(kTelnetURL == inType) ||
				(kRloginURL == inType) ||
				(kSshURL == inType) ||
				(kFtpURL == inType) ||
				(kSftpURL == inType) ||
				(kXmanpageURL == inType)
			);
	return result;
}// TypeIsHandledByAppItself


/*!
Allocates enough memory to concatenate the specified
components into an "ssh://" URL.  All necessary
additional characters, etc. to make a legal URL are
added.

The host name is required, but the port number and
user name are optional.

If successful, "noErr" is returned.  This is the only
time you should assume the CFString is valid, and if
it is, you should call CFRelease() to free it when
finished.

(3.0)
*/
OSStatus
URL_MakeSSHResourceLocator	(CFStringRef&	outNewURLString,
							 char const*	inHostName,
							 char const*	inPortStringOrNull,
							 char const*	inUserNameOrNull)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == inHostName) result = memPCErr;
	else
	{
		char const* const		kProtocolPrefix = "ssh://";
		CFMutableStringRef		buffer = nullptr;
		CFIndex					bufferSize = 0;
		
		
		// figure out how big the string needs to be
		bufferSize = (1 + CPP_STD::strlen(kProtocolPrefix));
		if (nullptr != inUserNameOrNull) bufferSize += (CPP_STD::strlen(inUserNameOrNull) + 1/* "@" */);
		bufferSize += CPP_STD::strlen(inHostName);
		if (nullptr != inPortStringOrNull) bufferSize += (1/* ":" */ + CPP_STD::strlen(inPortStringOrNull));
		
		// allocate space
		buffer = CFStringCreateMutable(kCFAllocatorDefault, bufferSize);
		if (nullptr == buffer) result = memFullErr;
		else
		{
			// construct a URL string based on information given
			CFStringAppendCString(buffer, kProtocolPrefix, kCFStringEncodingASCII);
			if (nullptr != inUserNameOrNull)
			{
				CFStringAppendCString(buffer, inUserNameOrNull, kCFStringEncodingASCII);
				CFStringAppendCString(buffer, "@", kCFStringEncodingASCII);
			}
			CFStringAppendCString(buffer, inHostName, kCFStringEncodingASCII);
			if (nullptr != inPortStringOrNull)
			{
				CFStringAppendCString(buffer, ":", kCFStringEncodingASCII);
				CFStringAppendCString(buffer, inPortStringOrNull, kCFStringEncodingASCII);
			}
			outNewURLString = buffer;
			// do not release, this is done by the caller
		}
	}
	return result;
}// MakeSSHResourceLocator


/*!
Allocates enough memory to concatenate the specified
components into a "telnet://" URL.  All necessary
additional characters, etc. to make a legal URL are
added.

The host name is required, but the port number and
user name are optional.

If successful, "noErr" is returned.  This is the only
time you should assume the CFString is valid, and if
it is, you should call CFRelease() to free it when
finished.

(3.0)
*/
OSStatus
URL_MakeTelnetResourceLocator	(CFStringRef&	outNewURLString,
								 char const*	inHostName,
								 char const*	inPortStringOrNull,
								 char const*	inUserNameOrNull)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == inHostName) result = memPCErr;
	else
	{
		char const* const		kProtocolPrefix = "telnet://";
		CFMutableStringRef		buffer = nullptr;
		CFIndex					bufferSize = 0;
		
		
		// figure out how big the string needs to be
		bufferSize = (1 + CPP_STD::strlen(kProtocolPrefix));
		if (nullptr != inUserNameOrNull) bufferSize += (CPP_STD::strlen(inUserNameOrNull) + 1/* "@" */);
		bufferSize += CPP_STD::strlen(inHostName);
		if (nullptr != inPortStringOrNull) bufferSize += (1/* ":" */ + CPP_STD::strlen(inPortStringOrNull));
		
		// allocate space
		buffer = CFStringCreateMutable(kCFAllocatorDefault, bufferSize);
		if (nullptr == buffer) result = memFullErr;
		else
		{
			// construct a URL string based on information given
			CFStringAppendCString(buffer, kProtocolPrefix, kCFStringEncodingASCII);
			if (nullptr != inUserNameOrNull)
			{
				CFStringAppendCString(buffer, inUserNameOrNull, kCFStringEncodingASCII);
				CFStringAppendCString(buffer, "@", kCFStringEncodingASCII);
			}
			CFStringAppendCString(buffer, inHostName, kCFStringEncodingASCII);
			if (nullptr != inPortStringOrNull)
			{
				CFStringAppendCString(buffer, ":", kCFStringEncodingASCII);
				CFStringAppendCString(buffer, inPortStringOrNull, kCFStringEncodingASCII);
			}
			outNewURLString = buffer;
			// do not release, this is done by the caller
		}
	}
	return result;
}// MakeTelnetResourceLocator


/*!
Uses the appropriate Internet helper application to
handle a URL corresponding to the given constant.
This API is a simple way to cause special addresses
to be opened (for example, the home page for this
program) without knowing the exact URL or the method
required to open it.

Returns "true" only if the open attempt succeeds.

A routine like URL_SendGetURLEventToSelf() is more
generic to handle arbitrary URLs.

(3.0)
*/
Boolean
URL_OpenInternetLocation	(URL_InternetLocation	inSpecialInternetLocationToOpen)
{
	Boolean		result = false;
	
	
	switch (inSpecialInternetLocationToOpen)
	{
	case kURL_InternetLocationApplicationHomePage:
		if (noErr == URL_ParseCFString(CFSTR("http://www.mactelnet.com/")))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationApplicationSupportEMail:
		if (noErr == URL_ParseCFString(CFSTR("mailto:kevin@ieee.org?Subject=MacTelnet")))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationApplicationUpdatesPage:
		if (noErr == URL_ParseCFString(CFSTR("http://homepage.mac.com/kmg/mactelnet/updates/3.1.0.html")))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationSourceCodeLicense:
		if (noErr == URL_ParseCFString(CFSTR("http://www.gnu.org/copyleft/gpl.html")))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationSourceForgeProject:
		if (noErr == URL_ParseCFString(CFSTR("http://sourceforge.net/projects/mactelnet/")))
		{
			// success!
			result = true;
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// OpenInternetLocation


/*!
Constructs a new session if the specified Core Foundation
string buffer contains a valid URL.  The configuration of
the session tries to match all the information given in
the URL.  If the URL is not something MacTelnet understands,
another application may be launched instead.

WARNING:	You probably want Quills::Session::handle_url()
			instead.  Quills respects script-based callbacks
			first, which is where the bulk of even the
			default implementation (much less any user code)
			exists.  Quills automatically calls this routine
			as a fallback when all else fails.

\retval noErr
if the string is a URL and it is opened successfully

\retval kURLInvalidURLError
if the string is not a URL

\retval (other)
if Launch Services has a problem opening the URL

(3.1)
*/
OSStatus
URL_ParseCFString	(CFStringRef	inURLString)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == inURLString) result = memPCErr;
	else
	{
		UniformResourceLocatorType	urlKind = kNotURL;
		
		
		urlKind = URL_GetTypeFromCFString(inURLString);
		if (kNotURL != urlKind)
		{
			CFURLRef	theCFURL = CFURLCreateWithString(kCFAllocatorDefault, inURLString, nullptr/* base URL */);
			
			
			if (nullptr == theCFURL) result = kURLInvalidURLError; // just a guess!
			else
			{
				result = LSOpenCFURLRef(theCFURL, nullptr/* URL of actual item opened */);
				if (noErr != result) Console_WriteValue("URL open error", result);
				CFRelease(theCFURL);
			}
		}
	}
	return result;
}// ParseCFString


/*!
Sends a "handle URL" event back to MacTelnet.
This should be done for most URL handlers, so
that the user can record the action.

Returns "true" if the event was sent successfully,
"false" otherwise.  Note that this does NOT
represent the success of the URL handling itself.

If you are interested in any Apple Event errors
(usually, you should be!), define storage for
the reply record; otherwise, set the pointer to
nullptr and nothing will be returned.  The reply
record is the place where URL handling errors
will be provided.

(3.0)
*/
Boolean
URL_SendGetURLEventToSelf	(void const*	inDataPtr,
							 Size			inDataSize,
							 AEDesc*		outReplyPtrOrNull)
{
	AppleEvent		handleURLEvent,
					reply;
	AESendMode		sendMode = 0;
	OSStatus		error = noErr;
	Boolean			result = false;
	
	
	error = RecordAE_CreateRecordableAppleEvent(kStandardURLSuite, kStandardURLEventIDGetURL, &handleURLEvent);
	
	// the URL string - REQUIRED
	if (noErr == error) error = AEPutParamPtr(&handleURLEvent, keyDirectObject, cString, inDataPtr, inDataSize);
	
	// handle the URL by sending an Apple Event; if it fails, report the error
	sendMode = kAENeverInteract;
	if (nullptr == outReplyPtrOrNull) sendMode |= kAENoReply;
	else
	{
		sendMode |= kAEWaitReply;
		//AppleEventUtilities_InitAEDesc(&reply);
		error = BasicTypesAE_CreateSInt32Desc(4, &reply);
		Console_WriteValue("create sint32 error", error);
	}
	if (noErr == error) error = AESend(&handleURLEvent, &reply, sendMode,
										kAENormalPriority, kAEDefaultTimeout, nullptr/* idle routine */,
										nullptr/* filter routine */);
	AEDisposeDesc(&handleURLEvent);
	
	result = (noErr == error);
	if (nullptr != outReplyPtrOrNull) *outReplyPtrOrNull = reply;
	return result;
}// SendGetURLEventToSelf


/*!
Searches the text from the given screen surrounding
the given point in the specified view, and if a URL
appears to be beneath the point, the viewÕs text
selection changes to highlight the URL.

Returns "true" only if a URL was found.

(2.6)
*/
Boolean
URL_SetSelectionToProximalURL	(TerminalView_Cell const&	inCommandClickColumnRow,
								 TerminalScreenRef			inScreen,
								 TerminalViewRef			inView)
{
	// TEMPORARY - REIMPLEMENT
	return false;
}// SetSelectionToProximalURL

// BELOW IS REQUIRED NEWLINE TO END FILE
