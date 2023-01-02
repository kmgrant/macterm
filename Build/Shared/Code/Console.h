/*!	\file Console.h
	\brief Provides access to the debugging console (and log
	file).
	
	Use this to log messages, primarily targeted at programming
	problems.  Note that if Console_Init() is never called, no
	other APIs have effect; you can use this to effectively
	disable debugging messages throughout the application.
	
	The generic Console_WriteLine() routine exists, however
	there are also several other specialized variants that make
	it easy to write comments alongside data that has a common
	type.
*/
/*###############################################################

	Data Access Library
	© 1998-2023 by Kevin Grant
	
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

#pragma once

// standard-C++ includes
#include <iosfwd>
#include <string>

// Mac includes
#include <AvailabilityMacros.h>
#include <CoreServices/CoreServices.h>



#pragma mark Types

/*!
Simply declare a variable of this type in a block,
and if its input is false it will trigger an
assertion failure in debugging mode.

When would you use this?  Say you have an object that
constructs successfully (throwing no exceptions) but
still holds an error condition in a flag.  You might
declare one of these assertion objects in the same
block to test for the error condition.

There is also a do-nothing default constructor, so
that you can disable an assertion simply by failing
to initialize it!  For example, if you declare some
assertions in a class, the constructor’s member
initialization list could contain "#ifdef DEBUG"
lines around assertion initializers.
*/
struct Console_Assertion
{
public:
	inline
	Console_Assertion		();
	
	inline
	Console_Assertion		(Boolean,
							 char const*,
							 unsigned long,
							 char const* = "");
};


/*!
Simply declare a variable of this type at the top
of a block, and any console output written within
the block will be indented.  When the block exits,
the previous indentation level is restored.
*/
struct Console_BlockIndent
{
public:
	inline
	Console_BlockIndent		();
	
	inline
	~Console_BlockIndent	();
};


/*!
A class that prints a message on construction and
destruction, thereby allowing you to track a block
simply by declaring a variable of this type at the
beginning.

If control never leaves the block (for example,
just before a crash), you will see the enter message
with no exit message.
*/
struct Console_BlockTracker
{
public:
	inline
	Console_BlockTracker	(char const*	inName);
	
	inline
	~Console_BlockTracker	();

private:
	std::string		_name;
};



//!\name Initialization
//@{

// CALL THIS ROUTINE ONCE, BEFORE ANY OTHER CONSOLE ROUTINE, AFTER TERMINAL SCREEN MODULES ARE INITIALIZED
void
	Console_Init					();

// CALL THIS ROUTINE AFTER YOU ARE PERMANENTLY DONE WITH THE CONSOLE
void
	Console_Done					();

//@}

//!\name Writing Messages to the Console
//@{

// WRITES AN ASSERTION-FAILED MESSAGE ONLY IF THE SPECIFIED CONDITION IS FALSE
Boolean
	Console_Assert					(char const*		inAssertionName,
									 Boolean			inCondition);

Boolean
	__Console_WarningsTriggerCrashTraces	();

inline bool __Console_CrashTraceably ()
{
	// force a crash so that backtracing to the offending line is easier
	// (the default abort() call screws all of this up)
	int* x = REINTERPRET_CAST(0xFEEDDEAD, int*); *x = 0;
	return false;
}
inline bool __Console_AssertHelper (char const* t, char const* file, unsigned long line)
{
	UNUSED_RETURN(int)printf("MacTerm: ASSERTION FAILURE: %s [%s:%lu]\n", t, file, line);
	__Console_CrashTraceably();
	return false;
}
#if 0
#define assert(e)  \
    ((void) ((e) ? 0 : __Console_AssertHelper(#e, __FILE__, __LINE__)))
#endif

// usage: e.g. static_assert_named(x_is_3, x == 3); fails AT COMPILE TIME with a negative-array-size
// error if the condition in the assertion did not hold (the "n" parameter names the array type);
// useful in things like constant expressions that the compiler can validate during the build
#define static_assert_named(n,e) \
	typedef char n[2 * (e) - 1]

// usage: e.g. Console_TestAssert(x == 3, Console_WriteValue, "str", x); if
// the condition fails, an ASSERTION FAILURE message appears and then the
// specified function is called, e.g. to print a variable’s actual value
#define Console_TestAssert(e, f, args...) \
	do { if (false == Console_Assert(#e, (e))) { f(args); } } while (0)

// usage: e.g. Console_TestAssert(result, x == 3, Console_WriteValue, "str", x);
// if the condition fails, an ASSERTION FAILURE message appears, the specified
// result variable is set to "false" and then the specified function is called,
// e.g. to print a variable’s actual value; this form is useful for unit tests
// that perform a long chain of assertions where any failure should be detected
// (obviously the given flag should be initially "true" before all tests begin)
#define Console_TestAssertUpdate(cumulativeFlag, e, f, args...) \
	do { if (false == Console_Assert(#e, (e))) { cumulativeFlag = false; f(args); } } while (0)

// usage: e.g. Console_Warning(Console_WriteValue, "message", 25); // Console_WriteValue("message", 25);
// the first argument is a function, and all remaining arguments are the function parameters
// IMPORTANT: the "args..." and " , ##" syntax only work in GNU compilers
#define Console_Warning(f, t, args...)  \
	do { char s[256]; (int)snprintf(s, sizeof(s), "warning, %s", t); f(s    , ## args); \
	     if (__Console_WarningsTriggerCrashTraces()) __Console_CrashTraceably(); } while (0)

void
	Console_WriteHorizontalRule		();

void
	Console_WriteLine				(char const*		inString);

void
	Console_WriteScriptError		(CFStringRef		inTitle,
									 CFStringRef		inDescription);

OSStatus
	Console_WriteShapeElement		(int				inMessage,
									 HIShapeRef			inShape,
									 CGRect const*		inRectPtr,
									 void*				inRefCon);

void
	Console_WriteStackTrace			(UInt16				inDepth = 0);

void
	Console_WriteUnitTestReport		(char const*		inModuleName,
									 UInt16				inFailureCount,
									 UInt16				inTotalTests);

void
	Console_WriteValue				(char const*		inLabel,
									 SInt64				inValue);

void
	Console_WriteValueAddress		(char const*		inLabel,
									 void const*		inValue);

void
	Console_WriteValueBitFlags		(char const*		inLabel,
									 UInt32				in32BitValue);

void
	Console_WriteValueCFError		(char const*		inLabel,
									 CFErrorRef			inError);

void
	Console_WriteValueCFString		(char const*		inLabel,
									 CFStringRef		inValue);

void
	Console_WriteValueCFTypeOf		(char const*		inLabel,
									 CFTypeRef			inObject);

void
	Console_WriteValueCharacter		(char const*		inLabel,
									 UInt8				inCharacter);

void
	Console_WriteValueCString		(char const*		inLabel,
									 char const*		inValue);

void
	Console_WriteValueFloat4		(char const*		inLabel,
									 Float32			inValue1,
									 Float32			inValue2,
									 Float32			inValue3,
									 Float32			inValue4);

void
	Console_WriteValueFourChars		(char const*		inLabel,
									 FourCharCode		inValue,
									 std::ostream*		inoutStreamPtrOrNull = nullptr);

void
	Console_WriteValuePair			(char const*		inLabel,
									 SInt64				inValue1,
									 SInt64				inValue2);

void
	Console_WriteValueStdString		(char const*		inLabel,
									 std::string const&	inValue);

void
	Console_WriteValueUnicodePoint	(char const*		inLabel,
									 UnicodeScalarValue	inCodePoint);

//@}

//!\name Indentation of Output
//@{

void
	Console_BeginFunction			();

void
	Console_EndFunction				();

//@}



#pragma mark Inline Methods

/*!
Does nothing.

(3.1)
*/
Console_Assertion::
Console_Assertion ()
{
}


/*!
Checks that the specified assertion has not failed.
If it does, raises an assertion in debug mode with
a traceable crash.

(3.1)
*/
Console_Assertion::
Console_Assertion	(Boolean			inAssertion,
					 char const*		inFile,
					 unsigned long		inLine,
					 char const*		inAssertionName)
{
	if (false == inAssertion) __Console_AssertHelper(inAssertionName, inFile, inLine);
}


/*!
Increases the indentation level.
*/
Console_BlockIndent::
Console_BlockIndent ()
{
	Console_BeginFunction();
}


/*!
Reduces the indentation level.
*/
Console_BlockIndent::
~Console_BlockIndent ()
{
	Console_EndFunction();
}


/*!
Prints a block-entered message.
*/
Console_BlockTracker::
Console_BlockTracker	(char const*	inName)
:
_name(inName)
{
	Console_WriteValueCString("Block entered:", _name.c_str());
}


/*!
Prints a block-exited message.
*/
Console_BlockTracker::
~Console_BlockTracker ()
{
	Console_WriteValueCString("Block exited:", _name.c_str());
}

// BELOW IS REQUIRED NEWLINE TO END FILE
