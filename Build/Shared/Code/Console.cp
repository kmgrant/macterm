/*!	\file Console.h
	\brief Provides access to the debugging console (and log
	file).
*/
/*###############################################################

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

#include <Console.h>

#ifndef NDEBUG

#include <UniversalDefines.h>

// standard-C includes
#include <iostream>
#include <sstream>
#include <string>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <GrowlSupport.h>
#include <ListenerModel.h>
#include <MemoryBlocks.h>
#include <StringUtilities.h>



#pragma mark Constants

#define DEBUGGING_PREFIX_MAX				200
#define DEBUGGING_SPACES_PER_INDENT			4

#pragma mark Variables
namespace {

SInt16		gIndentationLevel = 0;
char		gIndentPrefix[DEBUGGING_PREFIX_MAX]; // arbitrary size
Boolean		gConsoleInitialized = false;

}



#pragma mark Public Methods

/*!
This method sets up the initial configurations for
the debugging console window.  Call this method
if you want other methods from this file to have
any effect.

The console remains invisible; if you want to show
it, use Console_SetVisible().

IMPORTANT:	To turn off debugging completely (and
			render all subsequent Console_XYZ()
			calls ineffective), simply avoid calling
			Console_Init().

(1.0)
*/
void
Console_Init ()
{
	// set up indentation
	CPP_STD::strcpy(gIndentPrefix, "");
	gConsoleInitialized = true;
}// Init


/*!
This method cleans up the console window by
destroying data structures allocated by
Console_Init().  It also resets the screen
ID to an invalid integer so that future calls
to write to the console have no effect.

(1.0)
*/
void
Console_Done ()
{
	if (gConsoleInitialized)
	{
		gConsoleInitialized = false;
	}
}// Done


/*!
Checks the specified condition and prints an
assertion-failed message using the given assertion
name ONLY if the condition evaluates to "false".

Returns "false" only if the assertion fails (that
is, "true" is good!).

(1.0)
*/
Boolean
Console_Assert	(char const*	inAssertionName,
				 Boolean		inCondition)
{
	Boolean		result = true;
	
	
	unless (inCondition)
	{
		Console_WriteValueCString("ASSERTION FAILURE", inAssertionName);
		result = false;
	}
	
	return result;
}// Assert


/*!
Signals the start of a new function in the
console’s output.  This lets you organize printed
statements, since the console increases the
indentation level of everything that is subsequently
written to the console.  To end the output for a
function, use Console_EndFunction(), which decreases
the indentation level of subsequent writes.

You cannot call this routine too often - if you call
it more than a few hundred times, it will silently
have no effect.

(1.0)
*/
void
Console_BeginFunction ()
{
	SInt16		i = 0;
	
	
	if ((gIndentationLevel += DEBUGGING_SPACES_PER_INDENT) > DEBUGGING_PREFIX_MAX) gIndentationLevel = DEBUGGING_PREFIX_MAX;
	CPP_STD::strcpy(gIndentPrefix, "");
	for (i = 0; i < gIndentationLevel; i++) CPP_STD::strcat(gIndentPrefix, " ");
}// BeginFunction


/*!
Signals the end of a function in the console’s
output that was originally indented with a call
to Console_BeginFunction().

You cannot call this routine too often - if you
call it more than a few hundred times, it will
silently have no effect.

(1.0)
*/
void
Console_EndFunction ()
{
	SInt16		i = 0;
	
	
	if ((gIndentationLevel -= DEBUGGING_SPACES_PER_INDENT) < 0) gIndentationLevel = 0;
	CPP_STD::strcpy(gIndentPrefix, "");
	for (i = 0; i < gIndentationLevel; i++) CPP_STD::strcat(gIndentPrefix, " ");
}// EndFunction


/*!
Writes a standard horizontal line (made of dashes)
to the console, followed by a carriage return and
line feed.  Use this routine for drawing lines in
case one day the console is not implemented in the
way it currently is.

(1.0)
*/
void
Console_WriteHorizontalRule	()
{
	Console_WriteLine("----------------------------------------------------------------");
}// WriteHorizontalRule


/*!
Writes data to the console, followed by a carriage
return and line feed, and preceded by the identifier
of the current thread.  Pass an empty string to
write just a new-line character.

The specified text is automatically indented by the
appropriate amount, depending on how many times
Console_BeginFunction() has been called without a
balancing Console_EndFunction() call.

(1.0)
*/
void
Console_WriteLine	(char const*	inString)
{
	if (gConsoleInitialized)
	{
		std::cerr << "MacTerm: " << gIndentPrefix << inString << "\015\012";
		std::cerr.flush();
	}
}// WriteLine


/*!
Uses Growl to notify the user about errors in script code.  If
Growl is not installed, attempts to send the same information
to the console log.

Since these messages are more likely to be presented directly
to the user, they should be localized, and are therefore given
as CFStrings.

(2.2)
*/
void
Console_WriteScriptError	(CFStringRef		inTitle,
							 CFStringRef		inDescription)
{
	if (GrowlSupport_IsAvailable())
	{
		CFStringRef		growlNotificationName = CFSTR("Script error"); // MUST match "Growl Registration Ticket.growlRegDict"
		
		
		GrowlSupport_Notify(growlNotificationName, inTitle, inDescription);
	}
	else
	{
		char const*		cStringTitle = CFStringGetCStringPtr(inTitle, kCFStringEncodingUTF8);
		
		
		if (nullptr == cStringTitle)
		{
			cStringTitle = "Script error";
		}
		Console_WriteValueCFString(cStringTitle, inDescription);
	}
}// WriteScriptError


/*!
Writes a line to the console that includes the time
and date.

(1.0)
*/
void
Console_WriteTimeStamp	(char const*	inLabel)
{
	char			dateString[64];
	char			timeString[40];
	unsigned long	date = 0L;
	
	
	GetDateTime(&date);
	DateString(date, longDate, REINTERPRET_CAST(dateString, StringPtr), nullptr/* intlHandle */);
	StringUtilities_PToCInPlace(REINTERPRET_CAST(dateString, StringPtr));
	TimeString(date, true/* want seconds */, REINTERPRET_CAST(timeString, StringPtr), nullptr/* intlHandle */);
	StringUtilities_PToCInPlace(REINTERPRET_CAST(timeString, StringPtr));
	CPP_STD::strcat(dateString, " at ");
	CPP_STD::strcat(dateString, timeString);
	CPP_STD::strcat(dateString, ":");
	Console_WriteLine(dateString);
	Console_WriteLine(inLabel);
}// WriteTimeStamp


/*!
Writes a standard unit test report to the console,
with the given information included.  If the failure
count is zero, the output format may be completely
different.

(1.1)
*/
void
Console_WriteUnitTestReport		(char const*	inModuleName,
								 UInt16			inFailureCount,
								 UInt16			inTotalTests)
{
	std::ostringstream	s;
	
	
	s << inModuleName << " module: ";
	if (0 == inFailureCount)
	{
		s << "SUCCESSFUL unit test (total tests: " << inTotalTests << ")";
	}
	else
	{
		s << "FAILURE in " << inFailureCount << " unit test";
		if (1 != inFailureCount) s << "s";
	}
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteUnitTestReport


/*!
Writes the value of a numeric variable, preceded
by its name.  A string of the form "label = value"
is written to the console (with a new-line).

(1.0)
*/
void
Console_WriteValue		(char const*	inLabel,
						 SInt32			inValue)
{
	std::ostringstream	s;
	s << inLabel << " = " << inValue;
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValue


/*!
Writes a pointer value.  A string of the form
"label = value" is written to the console (with a
new-line).

(1.0)
*/
void
Console_WriteValueAddress	(char const*	inLabel,
							 void const*	inAddress)
{
	std::ostringstream	s;
	s << inLabel << " = " << inAddress;
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValueAddress


/*!
Writes a string of 32 1 and 0 digits for the bits
in a flag value.  A string of the form "label = value"
is written to the console (with a new-line).

(1.0)
*/
void
Console_WriteValueBitFlags		(char const*	inLabel,
								 UInt32			in32BitValue)
{
	std::ostringstream	s;
	
	
	s << inLabel << " = ";
	register SInt16		i = 0;
	for (i = 31; i >= 0; --i)
	{
		s << (in32BitValue & (1 << i) ? "1" : "0");
		if ((i % 4) == 0)
		{
			s << " ";
		}
	}
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValueBitFlags


/*!
Writes the value of a Core Foundation string.  A
string of the form "label = “value”" is written
to the console (with a new-line).

(1.0)
*/
void
Console_WriteValueCFString	(char const*	inLabel,
							 CFStringRef	inValue)
{
	std::ostringstream	s;
	
	
	s << inLabel << " = \"";
	if (inValue != nullptr)
	{
		CFStringEncoding const	kEncoding = kCFStringEncodingUTF8;
		size_t const			kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
												(CFStringGetLength(inValue), kEncoding);
		char*					valueString = new char[kBufferSize];
		CFStringGetCString(inValue, valueString, kBufferSize, kEncoding);
		s << valueString;
		delete [] valueString;
	}
	else
	{
		s << "<null>";
	}
	s << "\"";
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// Console_WriteValueCFString


/*!
Writes the type of a Core Foundation object.  A
string of the form "label = “type”" is written
to the console (with a new-line).

(1.1)
*/
void
Console_WriteValueCFTypeOf	(char const*	inLabel,
							 CFTypeRef		inObject)
{
	if (nullptr == inObject)
	{
		Console_WriteValueCFString(inLabel, CFSTR("<null>"));
	}
	else
	{
		CFStringRef		objectTypeCFString = CFCopyTypeIDDescription(CFGetTypeID(inObject));
		
		
		if (nullptr != objectTypeCFString)
		{
			Console_WriteValueCFString(inLabel, objectTypeCFString);
			CFRelease(objectTypeCFString), objectTypeCFString = nullptr;
		}
	}
}// Console_WriteValueCFTypeOf


/*!
Writes a character value, in particular making control
characters visible (e.g. "^A").  A string of the form
"label = value" is written to the console (with a
new-line).

(1.0)
*/
void
Console_WriteValueCharacter		(char const*	inLabel,
								 UInt8			inCharacter)
{
	std::ostringstream		s;
	
	
	s << inLabel << " = ";
	if (inCharacter < ' '/* space */)
	{
		std::string		cStr;
		cStr = inCharacter + '@';
		s << '^' << cStr.c_str();
	}
	else if (inCharacter == ' '/* space */)
	{
		s << "<sp>";
	}
	else if (inCharacter >= 127)
	{
		s << "<" << STATIC_CAST(inCharacter, unsigned int) << ">";
	}
	else
	{
		std::string		cStr;
		cStr = inCharacter;
		s << cStr.c_str();
	}
	
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValueCharacter


/*!
Writes the value of a null-terminated string variable.
A string of the form "label = “value”" is written to
the console (with a new-line).

(1.0)
*/
void
Console_WriteValueCString	(char const*	inLabel,
							 char const*	inValue)
{
	std::ostringstream	s;
	s << inLabel << " = \"" << inValue << "\"";
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValueCString


/*!
Writes the values of 4 floating-point variables, preceded
by a label.  A string of the form
"label = value1, value2, value3, value4" is written to
the console (with a new-line).

(1.1)
*/
void
Console_WriteValueFloat4	(char const*	inLabel,
							 Float32		inValue1,
							 Float32		inValue2,
							 Float32		inValue3,
							 Float32		inValue4)
{
	std::ostringstream	s;
	s
	<< inLabel << " = "
	<< inValue1 << ", " << inValue2 << ", " << inValue3 << ", " << inValue4
	;
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValueFloat4


/*!
Writes the value of an "OSType" variable.  A
string of the form "label = value" is written
to the console (with a new-line).

(1.0)
*/
void
Console_WriteValueFourChars		(char const*	inLabel,
								 FourCharCode	inValue)
{
	CFRetainRelease		typeCFString(CreateTypeStringWithOSType(inValue), true/* is retained */);
	
	
	Console_WriteValueCFString(inLabel, typeCFString.returnCFStringRef());
}// WriteValueFourChars


/*!
Writes the values of 2 numeric variables, preceded
by a label.  A string of the form "label = value1, value2"
is written to the console (with a new-line).

(1.0)
*/
void
Console_WriteValuePair	(char const*	inLabel,
						 SInt32			inValue1,
						 SInt32			inValue2)
{
	std::ostringstream	s;
	s << inLabel << " = " << inValue1 << "," << inValue2;
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValuePair


/*!
Writes the value of a Pascal string variable.  A
string of the form "label = “value”" is written
to the console (with a new-line).

(1.0)
*/
void
Console_WriteValuePString	(char const*		inLabel,
							 ConstStringPtr		inValue)
{
	std::ostringstream	s;
	
	
	s << inLabel << " = \"";
	if (nullptr != inValue)
	{
		char	buffer[256];
		StringUtilities_PToC(inValue, buffer);
		s << buffer;
	}
	else
	{
		s << "<null>";
	}
	s << "\"";
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValuePString


/*!
Writes the value of a std::string variable.  A string
of the form "label = “value”" is written to the console
(with a new-line).

(1.1)
*/
void
Console_WriteValueStdString	(char const*			inLabel,
							 std::string const&		inValue)
{
	std::ostringstream	s;
	s << inLabel << " = \"" << inValue << "\"";
	std::string		sString = s.str();
	Console_WriteLine(sString.c_str());
}// WriteValueStdString


/*!
Works like Console_WriteValueCharacter() for ASCII-range
values, and otherwise prints an escaped Unicode value.

(1.0)
*/
void
Console_WriteValueUnicodePoint	(char const*			inLabel,
								 UnicodeScalarValue		inCodePoint)
{
	if (0 == (inCodePoint & 0xFFFFFF00))
	{
		Console_WriteValueCharacter(inLabel, STATIC_CAST(inCodePoint, UInt8));
	}
	else
	{
		std::ostringstream		s;
		
		
		s << inLabel << " = \\u0x" << std::hex << inCodePoint;
		
		std::string		sString = s.str();
		Console_WriteLine(sString.c_str());
	}
}// WriteValueUnicodePoint


/*!
Return true if Console_Warning() calls should trigger a crash
that is traceable in a debugger (useful in ultra-debug mode
near a final release); otherwise, return false to just make
warnings print their message to the console.

This is a function here instead of in the header, because it
may be tweaked frequently and the header is used A LOT.  This
simple flag change should not cause the entire project to be
recompiled.

(2.0)
*/
Boolean
__Console_WarningsTriggerCrashTraces ()
{
#if 0
	return true;
#else
	return false;
#endif
}// WarningsTriggerCrashTraces

#endif /* ifndef NDEBUG */

// BELOW IS REQUIRED NEWLINE TO END FILE
