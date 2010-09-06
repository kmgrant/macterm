/*###############################################################

	StringUtilities.h
	
	What powerful data access library would be complete without
	a set of string utilities?  This module contains several
	routines that supplement the Pascal String Functions from
	the Mac OS Universal Interfaces.  The most powerful routine
	is StringUtilities_PBuild(), which puts ParamText() to shame
	by providing similar functionality but using *configurable*
	metacharacters which can collectively be a length other than
	two characters wide.  If you *need* to substitute more than
	4 different things into a string, just put as many different
	metacharacters as you need into the source string, call
	StringUtilities_PMetaSet() to change the current set of 4,
	and re-invoke StringUtilities_PBuild() as many times as you
	need to.  Other useful string utilities let you *insert* a
	string at the beginning of another string (the opposite of
	concatenation), append a number to a string, and even
	translate a MacTCP or Open Transport IP address into string
	form.
	
	Data Access Library 1.3
	� 1998-2004 by Kevin Grant
	
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

#ifndef __STRINGUTILITIES__
#define __STRINGUTILITIES__

// standard-C++ includes
#include <string>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

#define EMPTY_PSTRING					"\p"
#define kStringSubstitutionDefaultTag1	"\p%a"
#define kStringSubstitutionDefaultTag2	"\p%b"
#define kStringSubstitutionDefaultTag3	"\p%c"
#define kStringSubstitutionDefaultTag4	"\p%d"
#define kStringSubstitutionDefaultTag5	"\p%e"
#define kStringSubstitutionDefaultTag6	"\p%f"
enum
{
	kLengthMetacharacterDefault = 2
};

typedef UInt16 StringUtilitiesTruncationMethod;
enum
{
	kStringUtilities_TruncateAtEnd = 0,
	kStringUtilities_TruncateAtMiddle = 1,
	kStringUtilities_TruncateAtBeginning = 2
};

#pragma mark Types

typedef struct OpaqueTokenSet**		TokenSetRef;

struct StringSubstitutionSpec
{
	ConstStringPtr		tagString;				// tag to look for (e.g. "\p^0")
	ConstStringPtr		substitutionString;		// substitution value (e.g. "\pbar", given "\pfoo^0", yields "\pfoobar")
};



#pragma mark Public Methods

/*###############################################################
	STRING TOKENIZING UTILITIES
###############################################################*/

void
	StringUtilities_PDisposeTokens				(UInt16					inArgC,
												 TokenSetRef*			inoutArgV);

void
	StringUtilities_PGetToken					(UInt16					inZeroBasedTokenNumber,
												 TokenSetRef			inArgV,
												 StringPtr				outString);

void
	StringUtilities_PTokenize					(ConstStringPtr			inString,
												 UInt16*				outArgC,
												 TokenSetRef*			outArgV);

void
	StringUtilities_PTokenizeBy					(ConstStringPtr			inString,
												 ConstStringPtr			inTokenSeparators,
												 UInt16*				outArgC,
												 TokenSetRef*			outArgV);

/*###############################################################
	STRING MANIPULATION UTILITIES
###############################################################*/

void
	StringUtilities_PAppendNumber				(StringPtr				inoutString,
												 SInt32					inNumber);

void
	StringUtilities_PBuild						(StringPtr								inoutString,
												 UInt16									inSubstitutionListLength,
												 StringSubstitutionSpec const			inSubstitutionList[]);

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

/*###############################################################
	STRING INFORMATION UTILITIES
###############################################################*/

Boolean
	StringUtilities_PEndsWith					(ConstStringPtr							inLongString,
												 ConstStringPtr							inSuffixString);

Boolean
	StringUtilities_PStartsWith					(ConstStringPtr							inLongString,
												 ConstStringPtr							inPrefixString);

/*###############################################################
	STRING CONVERSION UTILITIES
###############################################################*/

void
	StringUtilities_CFToUTF8					(CFStringRef							inString,
												 std::string&							outBuffer);

void
	StringUtilities_CToP						(char const*							inString,
												 Str255									outBuffer);

StringPtr
	StringUtilities_CToPInPlace					(char									inoutString[]);

void
	StringUtilities_PExtractFirstLine			(UInt8 const*							inSourcePtr,
												 Size									inSize,
												 StringPtr								outLine);

void
	StringUtilities_PToC						(ConstStringPtr							inString,
												 char									outBuffer[]);

char*
	StringUtilities_PToCInPlace					(StringPtr								inoutString);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
