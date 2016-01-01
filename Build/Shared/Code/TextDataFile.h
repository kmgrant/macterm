/*!	\file TextDataFile.h
	\brief Simple reader and writer of key-value-pair-based
	ASCII files.
	
	NCSA Telnet traditionally used simple key-value ASCII files
	to store session data.  However, there were certain odd
	limitations to their implementation (for example, it was
	not generic, it required keys to be ordered and the total
	number of keys in the file had to match a fixed count).
	This module changes most of that NCSA code to be as flexible
	as possible - particularly useful on Mac OS X, where the use
	of resource forks is deprecated.
	
	To use a Text Data File, simply create an object of this
	type and then manipulate it by operating on individual key-
	value pairs.  For example, to read all the data in a file,
	just call TextDataFile_GetNextNameValue() repeatedly until
	no more data is found, and then take advantage of the string
	manipulation utility methods to attribute special meaning to
	the string values you receive.  To write data, invoke one of
	the TextDataFile_AddNameValue...() routines.  Disposing of
	the object closes the file, saving your changes.
*/
/*###############################################################

	Data Access Library
	© 1998-2016 by Kevin Grant
	
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

#ifndef __TEXTDATAFILE__
#define __TEXTDATAFILE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

enum TextDataFile_LineEndingStyle
{
	kTextDataFile_LineEndingStyleMacintosh		= 0,
	kTextDataFile_LineEndingStyleUNIX			= 1
};

enum TextDataFile_ValueBrackets
{
	kTextDataFile_ValueBracketsNone				= 0,	//!< indicates values should be written verbatim
	kTextDataFile_ValueBracketsDoubleQuotes		= 1,	//!< indicates that a string value should have quotes around it
	kTextDataFile_ValueBracketsCurlyBraces		= 2		//!< indicates that a string value should have curly braces around it
};

#pragma mark Types

typedef struct TextDataFile_OpaqueStructure**		TextDataFile_Ref;



#pragma mark Public Methods

/*###############################################################
	CREATING AND DESTROYING NAME-VALUE PAIR FILES
###############################################################*/

// THE SPECIFIED FILE MAY ALREADY EXIST - IF SO, ALL OF ITS DATA IS READ INTO MEMORY
TextDataFile_Ref
	TextDataFile_New						(SInt16							inFileReferenceNumber,
											 TextDataFile_LineEndingStyle	inLineEndings = kTextDataFile_LineEndingStyleMacintosh);

void
	TextDataFile_Dispose					(TextDataFile_Ref*				inoutRefPtr);

/*###############################################################
	MANAGING THE FILE
###############################################################*/

// POINTS BACK TO THE TOP OF THE FILE WITHOUT CLOSING THE FILE; AFFECTS TextDataFile_GetNext...() OPERATIONS
Boolean
	TextDataFile_ResetPointer				(TextDataFile_Ref				inRef);

/*###############################################################
	READING A TEXT DATA FILE
###############################################################*/

#if DATA_ACCESS_LIB_UNICODE_SUPPORT
Boolean
	TextDataFile_GetNextNameUnicodeValue	(TextDataFile_Ref				inRef,
											 char*							outNamePtr,
											 SInt16							inNameLength,
											 UniChar*						outValuePtr,
                              				 UniCharCount					inValueLength);
#endif

Boolean
	TextDataFile_GetNextNameValue			(TextDataFile_Ref				inRef,
											 char*							outNamePtr,
											 SInt16							inNameLength,
											 char*							outValuePtr,
											 SInt16							inValueLength,
											 Boolean						inStripBrackets = false);

/*###############################################################
	DATA AND STRING CONVERSION UTILITIES
###############################################################*/

Boolean
	TextDataFile_StringStripEndBrackets		(char*							inoutStringPtr,
											 char**							outClippedStringPtr);

Boolean
	TextDataFile_StringToFlag				(char const*					inStringPtr,
											 Boolean*						outValuePtr);

Boolean
	TextDataFile_StringToNumber				(char const*					inStringPtr,
											 SInt32*						outValuePtr);

Boolean
	TextDataFile_StringToRectangle			(char const*					inStringPtr,
											 Rect*							outValuePtr);

Boolean
	TextDataFile_StringToRGBColor			(char const*					inStringPtr,
											 CGDeviceColor*					outValuePtr);

/*###############################################################
	SAVING TO A TEXT DATA FILE
###############################################################*/

Boolean
	TextDataFile_AddComment					(TextDataFile_Ref				inRef,
											 char const*					inCommentTextPtr);

#if DATA_ACCESS_LIB_UNICODE_SUPPORT
Boolean
	TextDataFile_AddNameUnicodeValue		(TextDataFile_Ref				inRef,
											 char const*					inClassNameOrNull,
											 char const*					inPropertyName,
											 UniChar const*					inValue,
                              				 UniCharCount					inValueLength,
											 TextDataFile_ValueBrackets		inBrackets = kTextDataFile_ValueBracketsNone);
#endif

Boolean
	TextDataFile_AddNameValue				(TextDataFile_Ref				inRef,
											 char const*					inClassNameOrNull,
											 char const*					inPropertyName,
											 char const*					inValue,
											 TextDataFile_ValueBrackets		inBracket = kTextDataFile_ValueBracketsDoubleQuotes);

Boolean
	TextDataFile_AddNameValueCFString		(TextDataFile_Ref				inRef,
											 char const*					inClassNameOrNull,
											 char const*					inPropertyName,
											 CFStringRef					inValue,
											 TextDataFile_ValueBrackets		inBrackets = kTextDataFile_ValueBracketsDoubleQuotes);

Boolean
	TextDataFile_AddNameValueFlag			(TextDataFile_Ref				inRef,
											 char const*					inClassNameOrNull,
											 char const*					inPropertyName,
											 Boolean						inValue,
											 TextDataFile_ValueBrackets		inBrackets = kTextDataFile_ValueBracketsNone);

Boolean
	TextDataFile_AddNameValueNumber			(TextDataFile_Ref				inRef,
											 char const*					inClassNameOrNull,
											 char const*					inPropertyName,
											 SInt32							inValue,
											 TextDataFile_ValueBrackets		inBrackets = kTextDataFile_ValueBracketsNone);

Boolean
	TextDataFile_AddNameValueRectangle		(TextDataFile_Ref				inRef,
											 char const*					inClassNameOrNull,
											 char const*					inPropertyName,
											 Rect const*					inValuePtr,
											 TextDataFile_ValueBrackets		inBrackets = kTextDataFile_ValueBracketsCurlyBraces);

Boolean
	TextDataFile_AddNameValueRGBColor		(TextDataFile_Ref				inRef,
											 char const*					inClassNameOrNull,
											 char const*					inPropertyName,
											 CGDeviceColor const*			inValuePtr,
											 TextDataFile_ValueBrackets		inBrackets = kTextDataFile_ValueBracketsCurlyBraces);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
