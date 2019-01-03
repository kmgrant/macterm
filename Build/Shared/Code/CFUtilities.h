/*!	\file CFUtilities.h
	\brief Useful routines to deal with Core Foundation types.
	
	Core Foundation APIs do not validate input for efficiency
	reasons, but you can use these “safe” operators to create
	debugging-mode assertions around certain kinds of operations
	on Core Foundation types.
*/
/*###############################################################

	Data Access Library
	© 1998-2019 by Kevin Grant
	
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

// Mac includes
#include <CoreFoundation/CoreFoundation.h>



#pragma mark Public Methods

//!\name Type Conversion Utilities
//@{

/*!
Given an address that is really a CFArrayRef,
returns the value as a CFArrayRef.  In debugging
mode, asserts that the input really is a CFArrayRef.

(1.1)
*/
inline CFArrayRef
	CFUtilities_ArrayCast				(void const*		inApparentCFArrayRef)
	{
		if (nullptr != inApparentCFArrayRef)
		{
			assert(CFArrayGetTypeID() == CFGetTypeID(inApparentCFArrayRef));
		}
		return REINTERPRET_CAST(inApparentCFArrayRef, CFArrayRef);
	}

/*!
Given an address that is really a CFBooleanRef,
returns the value as a CFBooleanRef.  In debugging
mode, asserts that the input really is a CFBooleanRef.

(1.1)
*/
inline CFBooleanRef
	CFUtilities_BooleanCast				(void const*		inApparentCFBooleanRef)
	{
		if (nullptr != inApparentCFBooleanRef)
		{
			assert(CFBooleanGetTypeID() == CFGetTypeID(inApparentCFBooleanRef));
		}
		return REINTERPRET_CAST(inApparentCFBooleanRef, CFBooleanRef);
	}

/*!
Given an address that is really a CFDataRef,
returns the value as a CFDataRef.  In debugging
mode, asserts that the input really is a CFDataRef.

(1.2)
*/
inline CFDataRef
	CFUtilities_DataCast				(void const*		inApparentCFDataRef)
	{
		if (nullptr != inApparentCFDataRef)
		{
			assert(CFDataGetTypeID() == CFGetTypeID(inApparentCFDataRef));
		}
		return REINTERPRET_CAST(inApparentCFDataRef, CFDataRef);
	}

/*!
Given an address that is really a CFDictionaryRef,
returns the value as a CFDictionaryRef.  In debugging
mode, asserts that the input really is a CFDictionaryRef.

(1.1)
*/
inline CFDictionaryRef
	CFUtilities_DictionaryCast			(void const*		inApparentCFDictionaryRef)
	{
		if (nullptr != inApparentCFDictionaryRef)
		{
			assert(CFDictionaryGetTypeID() == CFGetTypeID(inApparentCFDictionaryRef));
		}
		return REINTERPRET_CAST(inApparentCFDictionaryRef, CFDictionaryRef);
	}

/*!
Given an address that is really a CFMutableArrayRef, returns
the value as a CFMutableArrayRef.  In debugging mode, asserts
that the input really is a CFMutableArrayRef.

(1.1)
*/
inline CFMutableArrayRef
	CFUtilities_MutableArrayCast		(void*		inApparentCFMutableArrayRef)
	{
		if (nullptr != inApparentCFMutableArrayRef)
		{
			assert(CFArrayGetTypeID() == CFGetTypeID(inApparentCFMutableArrayRef));
		}
		return REINTERPRET_CAST(inApparentCFMutableArrayRef, CFMutableArrayRef);
	}

/*!
Given an address that is really a CFMutableDictionaryRef,
returns the value as a CFMutableDictionaryRef.  In debugging
mode, asserts that the input really is a CFMutableDictionaryRef.

(1.1)
*/
inline CFMutableDictionaryRef
	CFUtilities_MutableDictionaryCast	(void*		inApparentCFMutableDictionaryRef)
	{
		if (nullptr != inApparentCFMutableDictionaryRef)
		{
			assert(CFDictionaryGetTypeID() == CFGetTypeID(inApparentCFMutableDictionaryRef));
		}
		return REINTERPRET_CAST(inApparentCFMutableDictionaryRef, CFMutableDictionaryRef);
	}

/*!
Given an address that is really a CFMutableStringRef, returns
the value as a CFMutableStringRef.  In debugging mode, asserts
that the input really is a CFMutableStringRef.

(1.1)
*/
inline CFMutableStringRef
	CFUtilities_MutableStringCast	(void*		inApparentCFMutableStringRef)
	{
		if (nullptr != inApparentCFMutableStringRef)
		{
			assert(CFStringGetTypeID() == CFGetTypeID(inApparentCFMutableStringRef));
		}
		return REINTERPRET_CAST(inApparentCFMutableStringRef, CFMutableStringRef);
	}

/*!
Given an address that is really a CFNumberRef,
returns the value as a CFNumberRef.  In debugging
mode, asserts that the input really is a CFNumberRef.

(1.1)
*/
inline CFNumberRef
	CFUtilities_NumberCast				(void const*		inApparentCFNumberRef)
	{
		if (nullptr != inApparentCFNumberRef)
		{
			assert(CFNumberGetTypeID() == CFGetTypeID(inApparentCFNumberRef));
		}
		return REINTERPRET_CAST(inApparentCFNumberRef, CFNumberRef);
	}

/*!
Given an address that is a Core Foundation String, Array,
Dictionary, Number, Boolean, Data or Date type, returns the
value as a CFPropertyListRef.  In debugging mode, asserts
that the input really is one of these reference types.

(1.1)
*/
inline CFPropertyListRef
	CFUtilities_PropertyListCast		(void const*		inApparentCFPropertyListRef)
	{
		if (nullptr != inApparentCFPropertyListRef)
		{
			// see CoreFoundation/CFBase.h and definition of "CFPropertyListRef" for
			// comments indicating all possible CF-types that property lists can be;
			// although assertions only run in debugging mode, listing these in order
			// of popularity will ensure as few get-type-ID calls as possible take place
			assert((CFStringGetTypeID() == CFGetTypeID(inApparentCFPropertyListRef)) ||
					(CFArrayGetTypeID() == CFGetTypeID(inApparentCFPropertyListRef)) ||
					(CFDictionaryGetTypeID() == CFGetTypeID(inApparentCFPropertyListRef)) ||
					(CFNumberGetTypeID() == CFGetTypeID(inApparentCFPropertyListRef)) ||
					(CFBooleanGetTypeID() == CFGetTypeID(inApparentCFPropertyListRef)) ||
					(CFDataGetTypeID() == CFGetTypeID(inApparentCFPropertyListRef)) ||
					(CFDateGetTypeID() == CFGetTypeID(inApparentCFPropertyListRef)));
		}
		return REINTERPRET_CAST(inApparentCFPropertyListRef, CFPropertyListRef);
	}

/*!
Given an address that is really a CFStringRef,
returns the value as a CFStringRef.  In debugging
mode, asserts that the input really is a CFStringRef.

(1.1)
*/
inline CFStringRef
	CFUtilities_StringCast				(void const*		inApparentCFStringRef)
	{
		if (nullptr != inApparentCFStringRef)
		{
			assert(CFStringGetTypeID() == CFGetTypeID(inApparentCFStringRef));
		}
		return REINTERPRET_CAST(inApparentCFStringRef, CFStringRef);
	}

/*!
Given an address that is really a CFURLRef,
returns the value as a CFURLRef.  In debugging
mode, asserts that the input really is a CFURLRef.

(1.1)
*/
inline CFURLRef
	CFUtilities_URLCast					(void const*		inApparentCFURLRef)
	{
		if (nullptr != inApparentCFURLRef)
		{
			assert(CFURLGetTypeID() == CFGetTypeID(inApparentCFURLRef));
		}
		return REINTERPRET_CAST(inApparentCFURLRef, CFURLRef);
	}

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
