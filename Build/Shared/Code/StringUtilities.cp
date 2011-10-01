/*!	\file StringUtilities.cp
	\brief General-purpose routines for dealing with text.
*/
/*###############################################################
	
	WARNING:	The encoding of this file MUST be MacRoman, due to
				certain string constants used in conversion
				methods.
	
	Data Access Library 2.6
	© 1998-2011 by Kevin Grant
	
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

// standard-C includes
#include <cstring>

// standard-C++ includes
#include <string>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <StringUtilities.h>



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


/*!
Performs a normal conversion of a Pascal string to a C string
(that is, it copies and converts a buffer of characters whose
first character is a length byte to a second buffer, where
the first character of the second buffer is real and the
string is terminated by a 0 byte).

(1.0)
*/
void
StringUtilities_CToP	(char const*	inString,
						 StringPtr		outBuffer)
{
	// Carbon offers a new CopyCStringToPascal function
	CopyCStringToPascal(inString, outBuffer);
}// CToP


/*!
Performs an in-place conversion of a C string to a Pascal
string (that is, it converts a buffer of characters whose first
character is real and last character is a terminating 0 byte to
a buffer whose first character is a length byte).  This is not
usually recommended, since it requires typecasting and can lead
to bugs if “constness” is casted away incorrectly.

(1.0)
*/
StringPtr
StringUtilities_CToPInPlace		(char		inoutString[])
{
	// Carbon offers a new CopyCStringToPascal function
	CopyCStringToPascal(inoutString, REINTERPRET_CAST(inoutString, StringPtr));
	return REINTERPRET_CAST(inoutString, StringPtr);
}// CToPInPlace


/*!
To clip a certain number of characters
from the beginning of a string, use
this method.

(1.0)
*/
void
StringUtilities_PClipHead	(StringPtr		inoutString,
							 UInt16			inHowManyCharacters)
{
	if (PLstrlen(inoutString) >= inHowManyCharacters)
	{
		inoutString[inHowManyCharacters] = inoutString[0] - inHowManyCharacters;
		PLstrcpy(inoutString, inoutString + inHowManyCharacters);
	}
}// PClipHead


/*!
To truncate a string, use this method.

(1.0)
*/
void
StringUtilities_PClipTail	(StringPtr		inoutString,
							 UInt16			inHowManyCharacters)
{
	inoutString[0] -= inHowManyCharacters;
}// PClipTail


/*!
Inserts the string "inSourceString" at the beginning
of the string "inoutDestinationString".  Unlike the
system call PLstrcat() (which combines two strings
also), this routine modifies the destination string
instead of the source string.

(1.0)
*/
void
StringUtilities_PInsert	(StringPtr			inoutDestinationString,
						 ConstStringPtr		inSourceString)
{
	// make room for new string
	BlockMoveData(inoutDestinationString + 1, inoutDestinationString + *inSourceString + 1,
					*inoutDestinationString);
	
	// copy new string in and adjust the length byte
	BlockMoveData(inSourceString + 1, inoutDestinationString + 1, *inSourceString);
	*inoutDestinationString += *inSourceString;
}// PInsert


/*!
Performs a normal conversion of a Pascal string to a C string
(that is, it copies and converts a buffer of characters whose
first character is a length byte to a second buffer, where
the first character of the second buffer is real and the
string is terminated by a 0 byte).

(1.0)
*/
void
StringUtilities_PToC	(ConstStringPtr		inString,
						 char				outBuffer[])
{
	// Carbon offers a new CopyPascalStringToC function
	CopyPascalStringToC(inString, outBuffer);
}// PToC


/*!
Performs an in-place conversion of a Pascal string to a
C string (that is, it converts a buffer of characters whose
first character is a length byte to a buffer whose first
character is real and last character is a terminating 0 byte).
This is not usually recommended, since it requires typecasting
and can lead to bugs if “constness” is casted away incorrectly.

(1.0)
*/
char*
StringUtilities_PToCInPlace		(StringPtr		inoutString)
{
	// Carbon offers a new CopyPascalStringToC function
	CopyPascalStringToC(inoutString, REINTERPRET_CAST(inoutString, char*));
	return REINTERPRET_CAST(inoutString, char*);
}// PToCInPlace


/*!
To use the font metrics information of the current
graphics port, along with an acceptable “maximum”
string width, to automatically truncate the given
string if needed, use this method.  This routine
relies on the StringWidth() and CharWidth() methods
from QuickDraw.

If the string is truncated, it is modified to
include an ellipsis (…) character.  The position
of the ellipsis depends on the truncation method;
"kStringUtilities_TruncateAtEnd" places it as the
last character in the string, for example.

If truncation was necessary, "true" is returned;
otherwise, "false" is returned.

(1.0)
*/
Boolean
StringUtilities_PTruncate	(StringPtr							inoutString,
							 UInt32								inAcceptableMaximumWidth,
							 StringUtilitiesTruncationMethod	inTruncationMethod)
{
	Boolean		result = (StringWidth(inoutString) > STATIC_CAST(inAcceptableMaximumWidth, SInt16));
	
	
	if (result)
	{
		UInt16 const	kMaximumWidth = inAcceptableMaximumWidth - STATIC_CAST(CharWidth('…'), UInt16);
		
		
		if (PLstrlen(inoutString) > 1) switch (inTruncationMethod)
		{
			case kStringUtilities_TruncateAtBeginning:
				{
					Str255		rightSegment;
					
					
					PLstrcpy(rightSegment, inoutString);
					while (StringWidth(rightSegment) > kMaximumWidth)
					{
						StringUtilities_PClipHead(rightSegment, 1);
					}
					PLstrcpy(inoutString, rightSegment);
					StringUtilities_PInsert(inoutString, "\p…");
				}
				break;
			
			case kStringUtilities_TruncateAtMiddle:
				{
					Str255		leftSegment,
								rightSegment;
					UInt16		leftSize = PLstrlen(inoutString) / 2;
					UInt16		leftWidth = 0;
					UInt16		halfMax = inAcceptableMaximumWidth / 2;
					
					
					PLstrncpy(leftSegment, inoutString, leftSize);
					PLstrcpy(rightSegment, inoutString);
					StringUtilities_PClipHead(rightSegment, leftSize);
					while (StringWidth(leftSegment) > halfMax)
					{
						leftSegment[--(leftSegment[0])] = '…';
					}
					leftWidth = StringWidth(leftSegment);
					while (StringWidth(rightSegment) > (kMaximumWidth - leftWidth))
					{
						StringUtilities_PClipHead(rightSegment, 1);
					}
					PLstrcpy(inoutString, leftSegment);
					PLstrcat(inoutString, rightSegment);
				}
				break;
			
			case kStringUtilities_TruncateAtEnd:
			default:
				while (StringWidth(inoutString) > kMaximumWidth)
				{
					inoutString[--(inoutString[0])] = '…';
				}
				break;
		}
		else if (PLstrlen(inoutString) == 1) inoutString[1] = '…';
	}
	
	return result;
}// PTruncate

// BELOW IS REQUIRED NEWLINE TO END FILE
