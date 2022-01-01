/*!	\file URL.cp
	\brief Helpers for working with uniform resource
	locators (Internet addresses).
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#include "URL.h"
#include <UniversalDefines.h>

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
#include <CFRetainRelease.h>
#include <Console.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// application includes
#include "AppResources.h"
#include "QuillsSession.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalView.h"



#pragma mark Constants

#define CR			0x0D	/* the carriage return character */

#define IS_WHITE_SPACE_CHARACTER(a)			(' ' == (a) || '\t' == (a))
#define IS_WHITE_SPACE_OR_CR_CHARACTER(a)	(IS_WHITE_SPACE_CHARACTER(a) || CR == (a))

#pragma mark Variables
namespace {

char const*		gURLSchemeNames[] =
{
	// WARNING:	This MUST match the order of the list
	//			URL_Type in "URL.h"; this is assumed
	//			within code in this file.
	":",
	"file:",
	"finger:",
	"ftp:",
	"gopher:",
	"http:",
	"https:",
	"mailto:",	
	"news:",
	"nntp:",
	"rlogin:",
	"telnet:",
	"tn3270:",
	"rtsp:", // QuickTime
	"sftp:",
	"ssh:",
	"wais:",
	"whois:",
	"x-man-page:",
	nullptr // this list must end with a nullptr
};

} // anonymous namespace


#pragma mark Public Methods

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
	CFRetainRelease		urlObject(TerminalView_ReturnSelectedTextCopyAsUnicode
									(inView, 0, kTerminalView_TextFlagInline),
									CFRetainRelease::kAlreadyRetained);
	Boolean				openFailed = true;
	
	
	if (urlObject.exists())
	{
		CFStringRef		urlAsCFString = urlObject.returnCFStringRef();
		URL_Type		urlKind = URL_ReturnTypeFromCFString(urlAsCFString);
		
		
		if (kURL_TypeInvalid != urlKind)
		{
			std::string		urlUTF8;
			
			
			// ideally, this should reuse the URL Apple Event handler;
			// but during the transition to Cocoa, that is more complex
			// than is convenient, so Quills is just called directly
			StringUtilities_CFToUTF8(urlAsCFString, urlUTF8);
			if (false == urlUTF8.empty())
			{
				TerminalView_ZoomOpenFromSelection(inView);
				try
				{
					Quills::Session::handle_url(urlUTF8);
					openFailed = false;
				}
				catch (std::exception const&	e)
				{
					CFStringRef			titleCFString = CFSTR("Exception while trying to handle URL"); // LOCALIZE THIS
					CFRetainRelease		messageCFString(CFStringCreateWithCString
														(kCFAllocatorDefault, e.what(), kCFStringEncodingUTF8),
														CFRetainRelease::kAlreadyRetained); // LOCALIZE THIS?
					
					
					Console_WriteScriptError(titleCFString, messageCFString.returnCFStringRef());
				}
			}
		}
	}
	
	if (openFailed)
	{
		Console_Warning(Console_WriteLine, "cannot open URL");
		Sound_StandardAlert();
	}
}// HandleForScreenView


/*!
Uses the appropriate Internet helper application to
handle a URL corresponding to the given constant.
This API is a simple way to cause special addresses
to be opened (for example, the home page for this
program) without knowing the exact URL or the method
required to open it.

Returns "true" only if the open attempt succeeds.

(3.0)
*/
Boolean
URL_OpenInternetLocation	(URL_InternetLocation	inSpecialInternetLocationToOpen)
{
	CFRetainRelease		urlCFString;
	Boolean				result = false;
	
	
	switch (inSpecialInternetLocationToOpen)
	{
	case kURL_InternetLocationApplicationHomePage:
		urlCFString.setWithRetain(CFBundleGetValueForInfoDictionaryKey
									(AppResources_ReturnBundleForInfo(), CFSTR("MyHomePageURL")));
		if (URL_ParseCFString(urlCFString.returnCFStringRef()))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationApplicationUpdatesPage:
		urlCFString.setWithRetain(CFBundleGetValueForInfoDictionaryKey
									(AppResources_ReturnBundleForInfo(), CFSTR("MyUpdatesURL")));
		if (URL_ParseCFString(urlCFString.returnCFStringRef()))
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
the URL.  If the URL is not something MacTerm understands,
another application may be launched instead.

WARNING:	You probably want Quills::Session::handle_url()
			instead.  Quills respects script-based callbacks
			first, which is where the bulk of even the
			default implementation (much less any user code)
			exists.  Quills automatically calls this routine
			as a fallback when all else fails.

\retval true
if the string is a URL and it is opened successfully

\retval false
if parsing failed

(3.1)
*/
Boolean
URL_ParseCFString	(CFStringRef	inURLString)
{
	Boolean		result = false; // initially...
	
	
	if (nullptr != inURLString)
	{
		URL_Type	urlKind = kURL_TypeInvalid;
		
		
		urlKind = URL_ReturnTypeFromCFString(inURLString);
		if (kURL_TypeInvalid != urlKind)
		{
			CFRetainRelease		theCFURL(CFURLCreateWithString(kCFAllocatorDefault, inURLString, nullptr/* base URL */),
											CFRetainRelease::kAlreadyRetained);
			
			
			if (theCFURL.exists())
			{
				OSStatus	openError = LSOpenCFURLRef(theCFURL.returnCFURLRef(), nullptr/* URL of actual item opened */);
				
				
				if (noErr == openError)
				{
					result = true;
				}
				else
				{
					Console_WriteValue("URL open error", openError);
				}
			}
		}
	}
	return result;
}// ParseCFString


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
URL_Type
URL_ReturnTypeFromCFString		(CFStringRef	inURLCFString)
{
	URL_Type			result = kURL_TypeInvalid;
	CFMutableStringRef	urlCopyCFString = CFStringCreateMutableCopy
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
				result = URL_ReturnTypeFromCharacterRange(buffer, buffer + kStringLength);
			}
			delete [] buffer, buffer = nullptr;
		}
		CFRelease(urlCopyCFString), urlCopyCFString = nullptr;
	}
	
	return result;
}// ReturnTypeFromCFString


/*!
Parses the given data handle, assumed to be a
string, and returns what kind of URL it seems
to represent.

Whitespace is NOT stripped in this version of
the routine.

(3.1)
*/
URL_Type
URL_ReturnTypeFromCharacterRange	(char const*	inBegin,
									 char const*	inPastEnd)
{
	URL_Type	result = kURL_TypeInvalid;
	SInt16		i = 0;
	
	
	// look for a match on the prefix (e.g. "http:")
	for (i = 0; (nullptr != gURLSchemeNames[i]); ++i)
	{
		char const* const	kCStringPtr = gURLSchemeNames[i];
		
		
		// obviously the URL must be at least as long as its prefix!
		if ((inPastEnd - inBegin) > STATIC_CAST(std::strlen(kCStringPtr), ptrdiff_t))
		{
			if (std::equal(kCStringPtr, kCStringPtr + std::strlen(kCStringPtr), inBegin))
			{
				result = STATIC_CAST(i, URL_Type);
				break;
			}
		}
	}
	return result;
}// ReturnTypeFromCharacterRange

// BELOW IS REQUIRED NEWLINE TO END FILE
