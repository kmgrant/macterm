/*###############################################################

	WARNING:	The encoding of this file MUST be MacRoman, due to
			certain string constants used in conversion
			methods.
	
	StringUtilities.cp
	
	This file contains useful methods for string management.
	
	Data Access Library 1.4
	© 1998-2008 by Kevin Grant
	
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
#include <MemoryBlocks.h>
#include <StringUtilities.h>



#pragma mark Internal Method Prototypes
namespace {

Boolean		isAmong			(CharParameter		inCharacter,
							 ConstStringPtr		inCharacterList);

Handle		refToHandle		(TokenSetRef		inRef);

} // anonymous namespace



#pragma mark Public Methods

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
#if TARGET_API_MAC_OS8
	// traditional Mac OS has C2PStr
	CPP_STD::strcpy(REINTERPRET_CAST(outBuffer, char*), inString);
	C2PStr(REINTERPRET_CAST(outBuffer, char*));
#else
	// Carbon offers a new CopyCStringToPascal function
	CopyCStringToPascal(inString, outBuffer);
#endif
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
#if TARGET_API_MAC_OS8
	// traditional Mac OS has C2PStr
	return C2PStr(inoutString);
#else
	// Carbon offers a new CopyCStringToPascal function
	CopyCStringToPascal(inoutString, REINTERPRET_CAST(inoutString, StringPtr));
	return REINTERPRET_CAST(inoutString, StringPtr);
#endif
}// CToPInPlace


/*!
Appends a string representation of a signed or
unsigned number to the given string.

(1.0)
*/
void
StringUtilities_PAppendNumber	(StringPtr		inoutString,
								 SInt32			inNumber)
{
	Str255		string;
	
	
	NumToString((long)inNumber, string);
	PLstrcat(inoutString, string);
}// PAppendNumber


/*!
Builds a custom string no longer than 255 characters out
of a source string containing metacharacters, using
substitution strings that correspond to each instance of
a metacharacter.  Metacharacters may appear in the string
in any order.  Substitution strings must be either empty
or nonempty strings (that is, not nullptr).

This function exists to supplant the ParamText() system
call, because the system call re-uses its substitution
strings (not correct, say, when multiple alert messages
are being displayed at the same time).  Also, while
ParamText() is limited to 4 substitutions, this routine
allows an arbitrary number (with practical limits defined
by the 255-character maximum size of a Pascal string).

(1.0)
*/
void
StringUtilities_PBuild	(StringPtr						inoutString,
						 UInt16							inSubstitutionListLength,
						 StringSubstitutionSpec const	inSubstitutionList[])
{
	Str255			stringBuffer;
	UInt8			srcCharCount = 0,	// template characters scanned, so far
					destCharCount = 0,	// destination string character count, so far
					charLeftCount = 0;	// number of characters left in source string
	UInt8 const		kSrcMaxCharCount = PLstrlen(inoutString);
	UInt8*			destPtr = &stringBuffer[1];	// make destination a temporary buffer, initially
	UInt8 const*	srcPtr = &inoutString[1];	// read template string from input
	UInt8			currentChar = '\0';
	ConstStringPtr	substitutionString = nullptr;
	UInt8			substitutionTagLength = 0;
	UInt8			i = 0;
	
	
	// build a customized string using the template and
	// the substitution strings
	while (srcCharCount < kSrcMaxCharCount)
	{
		charLeftCount = kSrcMaxCharCount - srcCharCount;
		
		// check current character
		currentChar = *srcPtr;
		
		// see if the current character is the start of a
		// substitution string; if so, keep checking until
		// an ENTIRE substitution string is found, partial
		// strings are not significant
		for (i = 0, substitutionString = nullptr;
			((i < inSubstitutionListLength) && (substitutionString == nullptr)); ++i)
		{
			// determine which tag (if any) this will be, based on the index
			UInt8 const*	tagSrcPtr = srcPtr;
			
			
			substitutionString = inSubstitutionList[i].substitutionString;
			substitutionTagLength = PLstrlen(inSubstitutionList[i].tagString);
			
			// there must be sufficient room left in the string to contain the tag
			// (if not, then stop now, as it’s not a tag after all)
			if (charLeftCount >= substitutionTagLength)
			{
				// scan ahead to confirm that an entire tag string has been found
				// (if a tag has not really been found, skip it)
				for (UInt8 j = 1; ((j <= substitutionTagLength) && (substitutionString != nullptr));
						++j, ++tagSrcPtr)
				{
					if (*tagSrcPtr != inSubstitutionList[i].tagString[j])
					{
						// incomplete tag - not a metacharacter, stop here
						substitutionString = nullptr;
					}
				}
			}
			else
			{
				substitutionString = nullptr;
				substitutionTagLength = 0;
			}
		}
		
		// if a substitution string is needed, add it in (and skip its tag string)
		if (substitutionString != nullptr)
		{
			UInt8 const		substitutionStringLength = PLstrlen(substitutionString);
			
			
			// append substitution string to destination string so far
			// (skip the length byte of the Pascal string source buffer)
			BlockMoveData(1 + substitutionString, destPtr, substitutionStringLength);
			
			// skip newly-substituted text in destination string
			destCharCount += substitutionStringLength;
			destPtr += substitutionStringLength;
			
			// skip to next non-tag character in source (template) string;
			// the tag includes the current character, so no "+ 1" here
			srcCharCount += substitutionTagLength;
			srcPtr += substitutionTagLength;
		}
		else
		{
			// not substituting, so the source character should be copied
			*destPtr = currentChar;
			
			// skip copied character in destination string
			++destCharCount;
			++destPtr;
			
			// skip to next character in source (template) string
			++srcCharCount;
			++srcPtr;
		}
	}
	
	// overwrite original string with the version containing substitutions;
	// but first set the length to change it into Pascal string format
	stringBuffer[0] = destCharCount;
	PLstrcpy(inoutString, stringBuffer);
}// PBuild


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
To dispose of memory allocated by a returned
handle of tokens from StringUtilities_PTokenize(),
use this method.

(1.0)
*/
void
StringUtilities_PDisposeTokens		(UInt16			inArgC,
									 TokenSetRef*	inoutArgV)
{
	if (inoutArgV != nullptr)
	{
		register UInt16		i = 0;
		StringHandle		stringHandle = nullptr;
		Handle				handle = refToHandle(*inoutArgV);
		
		
		HLock(handle);
		for (i = 0; i < inArgC; ++i)
		{
			stringHandle = ((StringHandle*)*handle)[i];
			DisposeHandle((Handle)stringHandle);
		}
		DisposeHandle(handle), handle = nullptr, *inoutArgV = nullptr;
	}
}// PDisposeTokens


/*!
To determine if the *very last* characters of the
specified string are identical to the given suffix,
call this method.

(1.1)
*/
Boolean
StringUtilities_PEndsWith	(ConstStringPtr		inLongString,
							 ConstStringPtr		inSuffixString)
{
	Str255		buffer;
	
	
	// make a Pascal string fragment matching the last part of the given string
	BlockMoveData(inLongString + PLstrlen(inLongString) - PLstrlen(inSuffixString) + 1,
					buffer + 1/* skip length byte */, PLstrlen(inSuffixString) * sizeof(UInt8));
	buffer[0] = PLstrlen(inSuffixString);
	
	return (!PLstrcmp(buffer, inSuffixString));
}// PEndsWith


/*!
To extract the first line of characters from the
specified buffer into a string, use this method.
If there are no new-line characters in the first
255 characters of the buffer, then the first 255
characters are considered to be the line (Pascal
string limitations).  The source buffer is not
affected by this call.  The "outLine" string is
the first line of text.  You can “clip” the
first line of text by incrementing the given
source pointer when this routine returns, using
the length of the resultant string.

This routine is useful for accepting drags of
text chunks, when the destination is a one-line
text field.  It is also useful for iterating
through a text buffer line-by-line.

Note that while the specified source data is
“raw” data, the resultant string is a Pascal
string (with a leading length byte).

(1.0)
*/
void
StringUtilities_PExtractFirstLine	(UInt8 const*	inSourcePtr,
									 Size			inSize,
									 StringPtr		outLine)
{
	if ((outLine != nullptr) && (inSourcePtr != nullptr))
	{
		register short		i = 0;
		
		
		// find the first line
		for (i = 0; i < inSize; ++i)
		{
			if (inSourcePtr[i] == '\n') break;
		}
		
		if (i >= inSize) i = inSize; // ...if the end of the buffer was reached, make a minor adjustment
		if (i > 255) i = 255; // ... if the chosen size is bigger than 255 characters, stop it at 255 (Pascal string limitations)
		outLine[0] = i; // set the size byte of this Pascal string buffer
		if (i != 0) BlockMoveData(inSourcePtr, outLine + 1, i); // copy the data to it
	}
}// PExtractFirstLine


/*!
To acquire a token from a token handle returned
from a call to StringUtilities_PTokenize(), use
this convenience method.

WARNING:	Do not pass a handle that was not
			returned via a call to the method
			StringUtilities_PTokenize().

(1.0)
*/
void
StringUtilities_PGetToken	(UInt16			inZeroBasedTokenNumber,
							 TokenSetRef	inArgV,
							 StringPtr		outString)
{
	if ((outString != nullptr) && (inArgV != nullptr))
	{
		StringHandle	stringHandle = nullptr;
		Handle			handle = refToHandle(inArgV);
		
		
		HLock(handle);
		if (STATIC_CAST(GetHandleSize(handle), size_t) >= ((inZeroBasedTokenNumber + 1) * sizeof(StringHandle)))
		{
			stringHandle = ((StringHandle*)*handle)[inZeroBasedTokenNumber];
			if (stringHandle != nullptr) PLstrcpy(outString, *stringHandle);
		}
		HUnlock(handle);
	}
}// PGetToken


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
To determine if the *very* first characters of the
specified string (not including the length byte, of
course) are identical to the given prefix, call this
method.

(1.0)
*/
Boolean
StringUtilities_PStartsWith		(ConstStringPtr		inLongString,
								 ConstStringPtr		inPrefixString)
{
	return (!PLstrncmp(inLongString, inPrefixString, PLstrlen(inPrefixString)));
}// PStartsWith


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
#if TARGET_API_MAC_OS8
	// traditional Mac OS has P2CStr
	PLstrcpy(REINTERPRET_CAST(outBuffer, StringPtr), inString);
	P2CStr(REINTERPRET_CAST(outBuffer, StringPtr));
#else
	// Carbon offers a new CopyPascalStringToC function
	CopyPascalStringToC(inString, outBuffer);
#endif
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
#if TARGET_API_MAC_OS8
	// traditional Mac OS has P2CStr
	return P2CStr(inoutString);
#else
	// Carbon offers a new CopyPascalStringToC function
	CopyPascalStringToC(inoutString, REINTERPRET_CAST(inoutString, char*));
	return REINTERPRET_CAST(inoutString, char*);
#endif
}// PToCInPlace


/*!
Conveniently tokenizes a string in the most common
way, by using spaces and tabs (“white space”) as token
delimiters and invoking StringUtilities_PTokenizeBy().
The resultant token set, and number of tokens in the
set, are returned.

If any problems occur, the output handle will be nullptr.

(1.0)
*/
void
StringUtilities_PTokenize	(ConstStringPtr		inString,
							 UInt16*			outArgC,
							 TokenSetRef*		outArgV)
{
	StringUtilities_PTokenizeBy(inString, "\p \t", outArgC, outArgV);
}// PTokenize


/*!
Takes a Pascal string, removes all characters resident
in the string "inTokenSeparators", and places the
resultant substrings, as StringHandles, into a Token Set.
The number of StringHandles in the set is returned in
"outArgC".

If any problems occur, the output handle will be nullptr.

(1.0)
*/
void
StringUtilities_PTokenizeBy		(ConstStringPtr		inString,
								 ConstStringPtr		inTokenSeparators,
								 UInt16*			outArgC,
								 TokenSetRef*		outArgV)
{
	if ((outArgC != nullptr) && (outArgV != nullptr))
	{
		Str255				buffer,
							token;
		register UInt16		i = 0;
		OSStatus			error = noErr;
		
		
		*outArgC = 0;
		*outArgV = (TokenSetRef)Memory_NewHandle(sizeof(StringHandle));
		error = MemError();
		if ((error == noErr) && (*outArgV != nullptr))
		{
			Handle		handle = refToHandle(*outArgV);
			
			
			// walk the  buffer, creating tokens as they’re found and shortening the buffer...
			PLstrcpy(buffer, inString);
			while (PLstrlen(buffer) > 0)
			{
				// ...find the next token separator
				while ((!isAmong(buffer[i + 1], inTokenSeparators)) &&
						(i < PLstrlen(buffer)))
				{
					++i;
				}
				
				// ...copy the token into a separate string
				token[0] = '\0', PLstrncpy(token, buffer, i);
				
				// ...ignore multiple token separators (such as multiple whitespace characters)
				while (isAmong(buffer[i + 1], inTokenSeparators) &&
						(i < PLstrlen(buffer)))
				{
					++i;
				}
				
				// ...remove all the characters parsed thus far from the buffer
				StringUtilities_PClipHead(buffer, i), i = 0;
				
				// ...if a token has been found, place it in the resultant token set
				if (PLstrlen(token) > 0)
				{
					++(*outArgC);
					error = Memory_SetHandleSize(handle, (*outArgC) * sizeof(StringHandle));
					if (error == noErr)
					{
						HLock(handle);
						((StringHandle*)*handle)[(*outArgC) - 1] = NewString(token);
						HUnlock(handle);
					}
					if (error != noErr) break;
				}
			}
			if (error != noErr)
			{
				// returning all the tokens failed - clean up and return zero
				StringUtilities_PDisposeTokens(*outArgC, outArgV);
				*outArgC = 0;
			}
		}
	}
}// PTokenizeBy


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


#pragma mark Internal Methods
namespace {

/*!
To determine if the specified character
is one of the characters in the given
string, use this method.

(1.0)
*/
Boolean
isAmong		(CharParameter		inCharacter,
			 ConstStringPtr		inCharacterList)
{
	Boolean		result = false;
	
	
	if (inCharacterList != nullptr)
	{
		register SInt16		i = 0;
		
		
		for (i = 1; ((i <= PLstrlen(inCharacterList)) && (!result)); ++i)
		{
			result = (inCharacter == inCharacterList[i]);
		}
	}
	return result;
}// isAmong


/*!
To acquire a Mac OS Handle representation
of a token set, use this method.

(1.0)
*/
Handle
refToHandle		(TokenSetRef		inRef)
{
	return ((Handle)inRef);
}// refToHandle

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
