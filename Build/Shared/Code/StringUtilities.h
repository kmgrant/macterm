/*!	\file StringUtilities.h
	\brief General-purpose routines for dealing with text.
*/
/*###############################################################
	
	Data Access Library 2.6
	� 1998-2011 by Kevin Grant
	
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

#include <UniversalDefines.h>

#ifndef __STRINGUTILITIES__
#define __STRINGUTILITIES__

// standard-C++ includes
#include <string>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef UInt16 StringUtilitiesTruncationMethod;
enum
{
	kStringUtilities_TruncateAtEnd = 0,
	kStringUtilities_TruncateAtMiddle = 1,
	kStringUtilities_TruncateAtBeginning = 2
};



#pragma mark Public Methods

//!\name String Tokenizing
//@{

CFArrayRef
	StringUtilities_CFNewStringsWithLines		(CFStringRef			inString);

//@}

//!\name String Manipulation Utilities
//@{

void
	StringUtilities_PClipHead					(StringPtr								inoutString,
												 UInt16									inHowManyCharacters);

void
	StringUtilities_PClipTail					(StringPtr								inoutString,
												 UInt16									inHowManyCharacters);

void
	StringUtilities_PInsert						(StringPtr								outDestinationString,
												 ConstStringPtr							inSourceString);

Boolean
	StringUtilities_PTruncate					(StringPtr								inoutString,
												 UInt32									inAcceptableMaximumWidth,
												 StringUtilitiesTruncationMethod		inTruncationMethod);

//@}

//!\name String Conversion Utilities
//@{

void
	StringUtilities_CFToUTF8					(CFStringRef							inString,
												 std::string&							outBuffer);

void
	StringUtilities_CToP						(char const*							inString,
												 Str255									outBuffer);

StringPtr
	StringUtilities_CToPInPlace					(char									inoutString[]);

void
	StringUtilities_PToC						(ConstStringPtr							inString,
												 char									outBuffer[]);

char*
	StringUtilities_PToCInPlace					(StringPtr								inoutString);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
