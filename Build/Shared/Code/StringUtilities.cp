/*!	\file StringUtilities.cp
	\brief General-purpose routines for dealing with text.
*/
/*###############################################################
	
	Data Access Library
	© 1998-2015 by Kevin Grant
	
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

#include <StringUtilities.h>
#include <UniversalDefines.h>

// standard-C includes
#include <cstring>

// standard-C++ includes
#include <string>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>



#pragma mark Public Methods

/*!
Creates a new array containing as many strings as necessary
to hold all of the lines from the original string.  You must
release the array yourself with CFRelease().

The strings are determined using CFStringGetLineBounds(), so
any sequence that means “new line” could cause the split.
None of the new-line sequences are included.

Although no problems are expected when searching for lines,
since an iterative search for lines is performed there is a
guard against infinite looping.  The search is arbitrarily
terminated after SHRT_MAX attempts to find lines have been
made.

(2.6)
*/
CFArrayRef
StringUtilities_CFNewStringsWithLines	(CFStringRef	inString)
{
	CFIndex const		kLength = CFStringGetLength(inString);
	SInt16				loopGuard = 0;
	CFRange				startCharacter = CFRangeMake(0, 1/* count */);
	CFIndex				lineEndIndex = 0;
	CFIndex				lineEndWithTerminator = 0;
	CFMutableArrayRef	mutableResult = CFArrayCreateMutable(kCFAllocatorDefault, 0/* capacity, or zero for no limit */,
																&kCFTypeArrayCallBacks);
	CFArrayRef			result = mutableResult;
	
	
	while ((lineEndIndex < kLength) && (++loopGuard < SHRT_MAX/* arbitrary */))
	{
		CFRetainRelease		lineCFString;
		
		
		CFStringGetLineBounds(inString, startCharacter, &startCharacter.location, &lineEndWithTerminator, &lineEndIndex);
		lineCFString.setCFTypeRef(CFStringCreateWithSubstring(kCFAllocatorDefault, inString,
																CFRangeMake(startCharacter.location,
																			(lineEndIndex - startCharacter.location))),
									true/* is retained */);
		CFArrayAppendValue(mutableResult, lineCFString.returnCFStringRef());
		startCharacter.location = lineEndWithTerminator;
	}
	return result;
}// CFNewStringsWithLines


/*!
Converts a Core Foundation string into a C++ standard string
in UTF-8 encoding.

WARNING:	The result will be an empty string on failure.

(1.4)
*/
void
StringUtilities_CFToUTF8	(CFStringRef	inCFString,
							 std::string&	outBuffer)
{
	outBuffer.clear();
	if (nullptr != inCFString)
	{
		CFStringEncoding const		kDesiredEncoding = kCFStringEncodingUTF8;
		char const* const			kCharPtr = CFStringGetCStringPtr(inCFString, kDesiredEncoding);
		
		
		if (nullptr != kCharPtr)
		{
			outBuffer = std::string(kCharPtr);
		}
		else
		{
			// direct access failed; try copying
			size_t const	kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
												(CFStringGetLength(inCFString), kDesiredEncoding);
			char*			buffer = new char[kBufferSize];
			
			
			CFStringGetCString(inCFString, buffer, kBufferSize, kDesiredEncoding);
			outBuffer = buffer;
			delete [] buffer;
		}
	}
}// CFToUTF8

// BELOW IS REQUIRED NEWLINE TO END FILE
