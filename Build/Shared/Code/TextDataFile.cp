/*###############################################################

	TextDataFile.cp
	
	Data Access Library 1.3
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
#include <cctype>
#include <cstdio>
#include <cstring>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>
#include <StringUtilities.h>
#include <TextDataFile.h>



#pragma mark Types

struct My_TextDataFile
{
	TextDataFile_LineEndingStyle	lineEndings;	//!< what character constitutes an end-of-line
	
	struct
	{
		SInt16			refNum;			//!< reference number of open file
	} file;
};
typedef My_TextDataFile*		My_TextDataFilePtr;
typedef My_TextDataFilePtr*		My_TextDataFileHandle;

typedef MemoryBlockHandleLocker< TextDataFile_Ref, My_TextDataFile >	My_TextDataFileHandleLocker;
typedef LockAcquireRelease< TextDataFile_Ref, My_TextDataFile >			My_TextDataFileHandleAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_TextDataFileHandleLocker&	gTextDataFileHandleLocks ()	{ static My_TextDataFileHandleLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static char*							getNameValue			(char*, char** = nullptr);
static TextDataFile_LineEndingStyle		guessLineEndingStyle	(SInt16);
static SInt32							readLine				(SInt16, char*, SInt32, TextDataFile_LineEndingStyle);



#pragma mark Public Methods

/*!
Creates a new Text Data File object that accesses
the specified file opened for read and write access.

Optionally you can specify what line endings to use.
This is definitely used for writing, however it only
defines the *default* value when reading - a Text
Data File is smart enough to scan for the apparent
line ending scheme when reading a new file, and will
only use the specified style if it cannot determine
what style the file really uses.

If any problems occur, nullptr is returned; otherwise,
a reference to the new object is returned.

(1.0)
*/
TextDataFile_Ref
TextDataFile_New	(SInt16							inFileReferenceNumber,
					 TextDataFile_LineEndingStyle	inLineEndings)
{
	TextDataFile_Ref	result = nullptr;
	
	
	result = REINTERPRET_CAST(Memory_NewHandle(sizeof(My_TextDataFile)), TextDataFile_Ref);
	if (result != nullptr)
	{
		My_TextDataFileHandleAutoLocker		ptr(gTextDataFileHandleLocks(), result);
		
		
		if (ptr != nullptr)
		{
			ptr->file.refNum = inFileReferenceNumber;
			ptr->lineEndings = guessLineEndingStyle(ptr->file.refNum);
		}
	}
	return result;
}// New


/*!
Deletes the in-memory representation of a Text Data File
that you created with TextDataFile_New().  The file is
NOT closed, because this module did not open it in the
first place.

(1.0)
*/
void
TextDataFile_Dispose	(TextDataFile_Ref*	inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		{
			My_TextDataFileHandleAutoLocker		ptr(gTextDataFileHandleLocks(), *inoutRefPtr);
			
			
			ptr->file.refNum = -1;
		}
		Memory_DisposeHandle(REINTERPRET_CAST(inoutRefPtr, Handle*));
	}
}// Dispose


/*!
Writes a comment to your text data file.  Since the
order of your operations is retained, you can invoke
this routine to write a comment immediately prior to
invoking a routine to set a name’s value, such as
TextDataFile_AddNameValue(), and the comment will
appear immediately prior to the assignment statement
in the resultant text file.

The specified text is automatically preceded with the
proper comment syntax (a hash mark, '#') so you
shouldn’t include anything in "inCommentTextPtr"
except the actual text of the comment.

If successful, "true" is returned.

(1.0)
*/
Boolean
TextDataFile_AddComment		(TextDataFile_Ref	inRef,
							 char const*		inCommentTextPtr)
{
	My_TextDataFileHandleAutoLocker		ptr(gTextDataFileHandleLocks(), inRef);
	Boolean								result = false;
	
	
	if ((ptr != nullptr) && (inCommentTextPtr != nullptr))
	{
		SInt32		count = 0;
		OSStatus	error = noErr;
		
		
		// precede the text with a comment hash-mark ('#'), and then write the comment text
		count = 2;
		error = FSWrite(ptr->file.refNum, &count, "# ");
		
		if (error == noErr)
		{
			// write comment string
			count = CPP_STD::strlen((char*)inCommentTextPtr);
			error = FSWrite(ptr->file.refNum, &count, inCommentTextPtr);
			
			if (error == noErr)
			{
				// write newline
				count = 1;
				error = FSWrite(ptr->file.refNum, &count,
								(ptr->lineEndings == kTextDataFile_LineEndingStyleUNIX)
								? "\012" : "\015");
				
				if (error == noErr)
				{
					result = true;
				}
			}
		}
	}
	return result;
}// AddComment


/*!
Specifies a name-value pair with a string value.  In
the text file, this will appear on a single line in
some format, such as "name = value".

Only alphanumeric characters, i.e. A-Z, a-z and 0-9,
as well as the underscore ('_'), are allowed in names.

Values may contain most characters, even a carriage-
return (octal 15), as long as no carriage-return
character within the value is followed by a line-feed
(octal 12).  (A line-feed following a carriage-return
marks a Mac OS end-of-line.)

If the name was successfully assigned a value, "true"
is returned; otherwise, "false" is returned.

(1.0)
*/
Boolean
TextDataFile_AddNameValue	(TextDataFile_Ref				inRef,
							 char const*					inClassNameOrNull,
							 char const*					inPropertyName,
							 char const*					inValue,
							 TextDataFile_ValueBrackets		inBrackets)
{
	My_TextDataFileHandleAutoLocker		ptr(gTextDataFileHandleLocks(), inRef);
	Boolean								result = false;
	
	
	if (ptr != nullptr)
	{
		SInt32		count = 0;
		OSStatus	error = noErr;
		
		
		if (inClassNameOrNull != nullptr)
		{
			// write class name and a dot (.)
			count = CPP_STD::strlen((char*)inClassNameOrNull);
			error = FSWrite(ptr->file.refNum, &count, inClassNameOrNull);
			if (error == noErr)
			{
				count = 1;
				error = FSWrite(ptr->file.refNum, &count, ".");
			}
		}
		
		if (error == noErr)
		{
			// write property name
			count = CPP_STD::strlen((char*)inPropertyName);
			error = FSWrite(ptr->file.refNum, &count, inPropertyName);
			
			if (error == noErr)
			{
				// write equal sign
				count = 3;
				error = FSWrite(ptr->file.refNum, &count, " = ");
				
				if (error == noErr)
				{
					// write an open bracket, if one is needed
					switch (inBrackets)
					{
					case kTextDataFile_ValueBracketsDoubleQuotes:
						{
							char	bracket[] = { '\"' };
							
							
							count = 1;
							error = FSWrite(ptr->file.refNum, &count, bracket);
						}
						break;
					
					case kTextDataFile_ValueBracketsCurlyBraces:
						{
							char	bracket[] = { '{' };
							
							
							count = 1;
							error = FSWrite(ptr->file.refNum, &count, bracket);
						}
						break;
					
					case kTextDataFile_ValueBracketsNone:
					default:
						break;
					}
					
					// write value string
					count = CPP_STD::strlen((char*)inValue);
					error = FSWrite(ptr->file.refNum, &count, inValue);
					
					// write a close bracket, if one is needed
					switch (inBrackets)
					{
					case kTextDataFile_ValueBracketsDoubleQuotes:
						{
							char	bracket[] = { '\"' };
							
							
							count = 1;
							error = FSWrite(ptr->file.refNum, &count, bracket);
						}
						break;
					
					case kTextDataFile_ValueBracketsCurlyBraces:
						{
							char	bracket[] = { '}' };
							
							
							count = 1;
							error = FSWrite(ptr->file.refNum, &count, bracket);
						}
						break;
					
					case kTextDataFile_ValueBracketsNone:
					default:
						break;
					}
					
					if (error == noErr)
					{
						// write newline
						count = 1;
						error = FSWrite(ptr->file.refNum, &count,
										(ptr->lineEndings == kTextDataFile_LineEndingStyleUNIX)
										? "\012" : "\015");
						
						if (error == noErr)
						{
							result = true;
						}
					}
				}
			}
		}
	}
	return result;
}// AddNameValue


/*!
Specifies a name-value pair that has a Core Foundation
string value.  In the text file, this will appear on a
single line in some format, such as "name = value".

If the name was successfully assigned a value, "true"
is returned; otherwise, "false" is returned.

(1.1)
*/
Boolean
TextDataFile_AddNameValueCFString	(TextDataFile_Ref				inRef,
									 char const*					inClassNameOrNull,
									 char const*					inPropertyName,
									 CFStringRef					inValue,
									 TextDataFile_ValueBrackets		inBrackets)
{
	char const*		stringPtr = CFStringGetCStringPtr(inValue, CFStringGetFastestEncoding(inValue));
	char			buffer[256/* arbitrary */];
	Boolean			result = false;
	
	
	if (stringPtr == nullptr)
	{
		// failed to get C string directly; fine, just make a copy
		if (CFStringGetCString(inValue, buffer, sizeof(buffer), CFStringGetFastestEncoding(inValue)))
		{
			stringPtr = buffer;
		}
	}
	if (stringPtr != nullptr)
	{
		result = TextDataFile_AddNameValue(inRef, inClassNameOrNull, inPropertyName, stringPtr, inBrackets);
	}
	return result;
}// AddNameValueCFString


/*!
Specifies a name-value pair that has a true or false
value.  In the text file, this will appear on a single
line in some format, such as "name = yes" or "name = no".

If the name was successfully assigned a value, "true" is
returned; otherwise, "false" is returned.

(1.0)
*/
Boolean
TextDataFile_AddNameValueFlag	(TextDataFile_Ref				inRef,
								 char const*					inClassNameOrNull,
								 char const*					inPropertyName,
								 Boolean const					inValue,
								 TextDataFile_ValueBrackets		inBrackets)
{
	return TextDataFile_AddNameValue(inRef, inClassNameOrNull, inPropertyName, (inValue) ? "yes" : "no", inBrackets);
}// AddNameValueFlag


/*!
Specifies a name-value pair that has a numerical value.
In the text file, this will appear on a single line in
some format, such as "name = 1325" or "name = -241".

If the name was successfully assigned a value, "true"
is returned; otherwise, "false" is returned.

(1.0)
*/
Boolean
TextDataFile_AddNameValueNumber		(TextDataFile_Ref				inRef,
									 char const*					inClassNameOrNull,
									 char const*					inPropertyName,
									 SInt32							inValue,
									 TextDataFile_ValueBrackets		inBrackets)
{
	char	buffer[10];
	
	
	CPP_STD::sprintf(buffer, "%ld", inValue);
	return TextDataFile_AddNameValue(inRef, inClassNameOrNull, inPropertyName, buffer, inBrackets);
}// AddNameValueNumber


/*!
Specifies a name-value pair that has a Pascal string
value.  In the text file, this will appear on a single
line in some format, such as "name = value".

If the name was successfully assigned a value, "true"
is returned; otherwise, "false" is returned.

(1.0)
*/
Boolean
TextDataFile_AddNameValuePString	(TextDataFile_Ref				inRef,
									 char const*					inClassNameOrNull,
									 char const*					inPropertyName,
									 ConstStringPtr					inValue,
									 TextDataFile_ValueBrackets		inBrackets)
{
	char	buffer[255];
	
	
	StringUtilities_PToC(inValue, buffer);
	return TextDataFile_AddNameValue(inRef, inClassNameOrNull, inPropertyName, buffer, inBrackets);
}// AddNameValuePString


/*!
Specifies a name-value pair that has a QuickDraw
rectangle value.  In the text file, this will
appear on a single line in some format, such as
"bounds = {0,10,450,200}", where the order of the
numbers is left, top, right, bottom.

If the name was successfully assigned a value,
"true" is returned; otherwise, "false" is returned.

(1.0)
*/
Boolean
TextDataFile_AddNameValueRectangle	(TextDataFile_Ref				inRef,
									 char const*					inClassNameOrNull,
									 char const*					inPropertyName,
									 Rect const*					inValuePtr,
									 TextDataFile_ValueBrackets		inBrackets)
{
	char	buffer[100];
	
	
	CPP_STD::sprintf(buffer, "%d,%d,%d,%d", inValuePtr->left, inValuePtr->top, inValuePtr->right, inValuePtr->bottom);
	return TextDataFile_AddNameValue(inRef, inClassNameOrNull, inPropertyName, buffer, inBrackets);
}// AddNameValueRectangle


/*!
Specifies a name-value pair that has an RGB color
value.  In the text file, this will appear on a
single line in some format, such as
"name = {65535,255,0}".

If the name was successfully assigned a value,
"true" is returned; otherwise, "false" is returned.

(1.0)
*/
Boolean
TextDataFile_AddNameValueRGBColor	(TextDataFile_Ref				inRef,
									 char const*					inClassNameOrNull,
									 char const*					inPropertyName,
									 RGBColor const*				inValuePtr,
									 TextDataFile_ValueBrackets		inBrackets)
{
	char	buffer[100];
	
	
	CPP_STD::sprintf(buffer, "%d,%d,%d", inValuePtr->red, inValuePtr->green, inValuePtr->blue);
	return TextDataFile_AddNameValue(inRef, inClassNameOrNull, inPropertyName, buffer, inBrackets);
}// AddNameValueRGBColor


/*!
Returns the value associated with the specified name.
If the value could be read, "true" is returned and
the specified string will be filled in; otherwise,
"false" is returned.

If "inStripBrackets" is "true", the value string will
have its end caps removed automatically - see the
TextDataFile_StringStripEndBrackets() routine for
details on what pairs of characters are considered
end brackets.

Each string is null-terminated.

(1.0)
*/
Boolean
TextDataFile_GetNextNameValue	(TextDataFile_Ref	inRef,
								 char*				outNamePtr,
								 SInt16				inNameLength,
								 char*				outValuePtr,
								 SInt16				inValueLength,
								 Boolean			inStripBrackets)
{
	My_TextDataFileHandleAutoLocker		ptr(gTextDataFileHandleLocks(), inRef);
	Boolean								result = false;
	
	
	if (ptr != nullptr)
	{
		char	buffer[256];
		
		
		// read up to the next newline, or end-of-file
		// (the buffer is to be null-terminated afterwards)
		if (readLine(ptr->file.refNum, buffer, sizeof(buffer), ptr->lineEndings) > 0)
		{
			char*	valuePtr = nullptr;
			char*	namePtr = getNameValue(buffer, &valuePtr); // on output, "buffer" is modified
			
			
			// check for brackets
			if (inStripBrackets)
			{
				char*	strippedStringPtr = nullptr;
				
				
				if (TextDataFile_StringStripEndBrackets(valuePtr, &strippedStringPtr))
				{
					valuePtr = strippedStringPtr;
				}
			}
			
			// copy the name and value
			CPP_STD::strncpy(outNamePtr, namePtr, inNameLength - 1);
			CPP_STD::strncpy(outValuePtr, valuePtr, inValueLength - 1);
			result = true;
		}
	}
	return result;
}// GetNextNameValue


/*!
Resets the file position of the specified data file
to point to the top of the file.  This affects all
TextDataFile_GetNext...() routines.

If the reset succeeds, "true" is returned; otherwise,
"false" is returned.

(1.0)
*/
Boolean
TextDataFile_ResetPointer	(TextDataFile_Ref	inRef)
{
	My_TextDataFileHandleAutoLocker		ptr(gTextDataFileHandleLocks(), inRef);
	Boolean								result = false;
	
	
	if (ptr != nullptr)
	{
		if (SetFPos(ptr->file.refNum, fsFromStart/* mode */,
					0/* offset from position indicated by mode */) == noErr)
		{
			// success
			result = true;
		}
	}
	return result;
}// ResetPointer


/*!
Checks the first and last characters of the specified
string for “matching brackets”, and strips them if
they exist.  For efficiency, this is accomplished by
modifying the existing string and returning a pointer
into the existing string (that is, one character past
the beginning of the given string).

The following pairs are checked:
	""	''	“”	‘’	``	[]	{}	()	<>	||

Returns "true" only if the specified string was
changed.  Also, "outClippedStringPtr" is only defined
when "true" is returned!

(1.0)
*/
Boolean
TextDataFile_StringStripEndBrackets		(char*		inoutStringPtr,
										 char**		outClippedStringPtr)
{
	Boolean		result = false;
	SInt16		stringLength = CPP_STD::strlen(inoutStringPtr);
	
	
	if ((inoutStringPtr != nullptr) && (stringLength >= 2))
	{
		switch (inoutStringPtr[0])
		{
			case '\"':
				result = (inoutStringPtr[stringLength - 1] == '\"');
				break;
			
			case '\'':
				result = (inoutStringPtr[stringLength - 1] == '\'');
				break;
			
			case '“':
				result = (inoutStringPtr[stringLength - 1] == '”');
				break;
			
			case '‘':
				result = (inoutStringPtr[stringLength - 1] == '’');
				break;
			
			case '`':
				result = (inoutStringPtr[stringLength - 1] == '`');
				break;
			
			case '[':
				result = (inoutStringPtr[stringLength - 1] == ']');
				break;
			
			case '{':
				result = (inoutStringPtr[stringLength - 1] == '}');
				break;
			
			case '(':
				result = (inoutStringPtr[stringLength - 1] == ')');
				break;
			
			case '<':
				result = (inoutStringPtr[stringLength - 1] == '>');
				break;
			
			case '|':
				result = (inoutStringPtr[stringLength - 1] == '|');
				break;
			
			default:
				// ???
				break;
		}
		
		if (result)
		{
			// clip end caps; using a null chracter allows
			// the rest of the original string to be the
			// “new” string, without copying any of the data
			inoutStringPtr[0] = '\0';
			inoutStringPtr[stringLength - 1] = '\0';
			*outClippedStringPtr = 1 + inoutStringPtr;
		}
	}
	return result;
}// StringStripEndBrackets


/*!
Determines a Boolean value using the contents of the
specified string.  If the value could be read, "true"
is returned and the specified flag will be filled in;
otherwise, "false" is returned.

A special heuristic is used to analyze the text and
determine whether it has affirmative or negative
meaning.  This routine is only guaranteed to work if
you write a flag to the file in the first place using
the TextDataFile_AddNameValueFlag() method; but it
definitely recognizes common phrases (for example,
"yes" and "1").

(1.0)
*/
Boolean
TextDataFile_StringToFlag	(char const*	inStringPtr,
							 Boolean*		outValuePtr)
{
	Boolean		result = false;
	
	
	if ((outValuePtr != nullptr) && (inStringPtr != nullptr))
	{
		char	buffer[32];
		
		
		// currently a little bit simplistic, but...
		CPP_STD::strncpy(buffer, inStringPtr, sizeof(buffer));
		StringUtilities_CToPInPlace(buffer);
		if (EqualString(REINTERPRET_CAST(buffer, StringPtr), "\pyes", false/* case sensitive */,
						true/* diacritical sensitive */))
		{
			*outValuePtr = true;
		}
		else
		{
			*outValuePtr = false;
		}
		result = true;
	}
	return result;
}// StringToFlag


/*!
Determines a signed, numeric value using the contents
of the specified string.  If the value could be read,
"true" is returned and the specified number will be
filled in; otherwise, "false" is returned.

(1.0)
*/
Boolean
TextDataFile_StringToNumber		(char const*	inStringPtr,
								 SInt32*		outValuePtr)
{
	Boolean		result = false;
	
	
	if ((outValuePtr != nullptr) && (inStringPtr != nullptr))
	{
		char	buffer[32]; // this buffer has to fit the sign and digits of any number up to LONG_MAX
		
		
		if (CPP_STD::strlen(inStringPtr) < sizeof(buffer))
		{
			CPP_STD::strncpy(buffer, inStringPtr, sizeof(buffer));
			StringUtilities_CToPInPlace(buffer);
			StringToNum(REINTERPRET_CAST(buffer, StringPtr), outValuePtr);
			result = true;
		}
	}
	return result;
}// StringToNumber


/*!
Determines a left-top-right-bottom rectangular value using
the contents of the specified string.  If the value could
be read, "true" is returned and the specified rectangle
will be filled in; otherwise, "false" is returned.

IMPORTANT:  Always strip brackets.  The input string will
			be expected to be of the form "l,t,r,b".

(1.0)
*/
Boolean
TextDataFile_StringToRectangle	(char const*	inStringPtr,
								 Rect*			outValuePtr)
{
	Boolean		result = false;
	
	
	if ((outValuePtr != nullptr) && (inStringPtr != nullptr))
	{
		int		l = 0;
		int		t = 0;
		int		r = 0;
		int		b = 0;
		
		
		if (4 == CPP_STD::sscanf(inStringPtr, "%d,%d,%d,%d", &l, &t, &r, &b))
		{
			// four numbers successfully read
			outValuePtr->left = l;
			outValuePtr->top = t;
			outValuePtr->right = r;
			outValuePtr->bottom = b;
			result = true;
		}
	}
	return result;
}// StringToRectangle


/*!
Determines a red-green-blue color value using the contents
of the specified string.  If the value could be read,
"true" is returned and the specified color data will be
filled in; otherwise, "false" is returned.

IMPORTANT:  Always strip brackets.  The input string will
			be expected to be of the form "red,green,blue".

(1.0)
*/
Boolean
TextDataFile_StringToRGBColor	(char const*	inStringPtr,
								 RGBColor*		outValuePtr)
{
	Boolean		result = false;
	
	
	if ((outValuePtr != nullptr) && (inStringPtr != nullptr))
	{
		unsigned int	r = 0;
		unsigned int	g = 0;
		unsigned int	b = 0;
		
		
		if (3 == CPP_STD::sscanf(inStringPtr, "%u,%u,%u", &r, &g, &b))
		{
			// three numbers successfully read
			outValuePtr->red = r;
			outValuePtr->green = g;
			outValuePtr->blue = b;
			result = true;
		}
	}
	return result;
}// StringToRGBColor


#pragma mark Internal Methods

/*!
Searches for the name-value delimiter within the
given line of text and returns a pointer to the
“name” part, overwriting the character that
follows with a null character.  Similarly, if
"outValuePtrOrNull" is not nullptr, the offset of
the value string is provided.

IMPORTANT:	Whitespace may cause the name to start
			past the beginning of the given string;
			so there is a chance you will lose the
			address of your buffer by calling this
			routine.  Also, since the buffer may be
			modified, make a copy of it beforehand
			if necessary.

(1.0)
*/
static char*
getNameValue	(char*		inoutLine,
				 char**		outValuePtrOrNull)
{
	char const* const	startPtr = inoutLine;
	SInt16 const		originalLength = CPP_STD::strlen(inoutLine);
	char*				result = inoutLine;
	char*				ptr = nullptr;
	
	
	// skip leading whitespace in name
	while (CPP_STD::isspace(*result)) ++result;
	ptr = result;
	
	// find first trailing whitespace or equal-sign
	while ((*ptr != '\0') && !CPP_STD::isspace(*ptr) && (*ptr != '=')) ++ptr;
	
	// terminate to identify name string, and increment to start value string
	*ptr++ = '\0';
	
	// if there is anything left of the string, continue
	if ((outValuePtrOrNull != nullptr) && (ptr < (startPtr + originalLength)))
	{
		// skip whitespace before equal-sign
		while (CPP_STD::isspace(*ptr)) ++ptr;
		
		// skip equal-sign in value
		if (*ptr == '=') ++ptr;
		
		// skip whitespace after equal-sign
		while (CPP_STD::isspace(*ptr)) ++ptr;
		
		// set value to the remainder
		*outValuePtrOrNull = ptr;
	}
	
	return result;
}// getNameValue


/*!
Reads a single line from a file.  The number of
characters actually read is returned.  The new-line
characters at the end of a line are automatically
stripped from the string prior to returning.  The
string is also null-terminated.

(1.1)
*/
static TextDataFile_LineEndingStyle
guessLineEndingStyle	(SInt16		inFileRefNum)
{
	TextDataFile_LineEndingStyle	result = kTextDataFile_LineEndingStyleMacintosh;
	long							oldFilePos = 0L;
	char							buffer[32];
	SInt32							bufferMaxBufferActual = sizeof(buffer);
	OSStatus						error = noErr;	
	
	
	// save current file pointer
	GetFPos(inFileRefNum, &oldFilePos);
	
	// scan forward by an arbitrary number of characters, looking for
	// either a carriage-return or line-feed character
	error = FSRead(inFileRefNum, &bufferMaxBufferActual/* in = how much to read, out = how much was read */, buffer);
	assert(STATIC_CAST(bufferMaxBufferActual, size_t) <= sizeof(buffer));
	if ((error == noErr) || (error == eofErr))
	{
		// find new-line style
		SInt16		i = 0;
		
		
		for (i = 0; i < bufferMaxBufferActual; ++i)
		{
			if (buffer[i] == '\015')
			{
				result = kTextDataFile_LineEndingStyleMacintosh;
				break;
			}
			if (buffer[i] == '\012')
			{
				result = kTextDataFile_LineEndingStyleUNIX;
				break;
			}
		}
	}
	
	// restore old file pointer
	SetFPos(inFileRefNum, fsFromStart, oldFilePos/* offset */);
	
	return result;
}// guessLineEndingStyle


/*!
Reads a single line from a file.  The number of
characters actually read is returned.  The new-line
characters at the end of a line are automatically
stripped from the string prior to returning.  The
string is also null-terminated.

(1.0)
*/
static SInt32
readLine	(SInt16							inFileRefNum,
			 char*							outLine,
			 SInt32							inCapacityLength,
			 TextDataFile_LineEndingStyle	inLineEndingStyle)
{
	SInt32		result = 0L;
	SInt32		bufferMaxBufferActual = 0;
	OSStatus	error = noErr;
	Boolean		sawEOF = false;
	Boolean		sawNewLine = false;
	char const	newLineChar = (inLineEndingStyle == kTextDataFile_LineEndingStyleUNIX)
								? '\012' : '\015';
	char*		ptr = nullptr;
	
	
	for (ptr = outLine; (!sawNewLine) && (!sawEOF) && (result < (inCapacityLength - 1)); ++ptr)
	{
		bufferMaxBufferActual = 1; // reset desired number of bytes to read
		error = FSRead(inFileRefNum, &bufferMaxBufferActual/* in = how much to read, out = how much was read */,
						ptr);
		if (error == eofErr) sawEOF = true; // (expected) break on EOF
		else if (error != noErr) sawNewLine = true; // force break on error
		else
		{
			// one more byte successfully retrieved; see if it’s a newline
			assert(bufferMaxBufferActual == 1);
			++result;
			
			if (*ptr == newLineChar)
			{
				sawNewLine = true;
				
				// strip the newline character(s) from the result
				*ptr = '\0'; // strip CR
			}
		}
	}
	
	assert(result < inCapacityLength);
	outLine[result] = '\0'; // explicitly null-terminate
	
	return result;
}// readLine

// BELOW IS REQUIRED NEWLINE TO END FILE
