/*!	\file CFDictionaryManager.cp
	\brief Adds strongly typed interfaces to manipulate data
	in a mutable Core Foundation dictionary whose entries are
	entirely Core Foundation types.
*/
/*###############################################################

	Data Access Library
	© 1998-2022 by Kevin Grant
	
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

#include <CFDictionaryManager.h>
#include <UniversalDefines.h>



#pragma mark Public Methods

/*!
Adds or overwrites a key value with a floating-point
number (which is automatically retained by the dictionary).

(1.4)
*/
void
CFDictionaryManager::
addFloat	(CFStringRef	inKey,
			 Float32		inValue)
{
	if (false == _dictionary.isMutable())
	{
		throw std::logic_error("warning, attempt to add a floating-point value to an immutable dictionary");
	}
	
	CFNumberRef		valueCFNumber = CFNumberCreate(CFGetAllocator(returnCFMutableDictionaryRef()), kCFNumberFloat32Type, &inValue);
	
	
	if (nullptr != valueCFNumber)
	{
		CFDictionarySetValue(returnCFMutableDictionaryRef(), inKey, valueCFNumber);
		CFRelease(valueCFNumber); valueCFNumber = nullptr;
	}
}// addFloat


/*!
Adds or overwrites a key value with a short integer
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFDictionaryManager::
addInteger	(CFStringRef	inKey,
			 SInt16			inValue)
{
	if (false == _dictionary.isMutable())
	{
		throw std::logic_error("warning, attempt to add an integer value to an immutable dictionary");
	}
	
	CFNumberRef		valueCFNumber = CFNumberCreate(CFGetAllocator(returnCFMutableDictionaryRef()), kCFNumberSInt16Type, &inValue);
	
	
	if (nullptr != valueCFNumber)
	{
		CFDictionarySetValue(returnCFMutableDictionaryRef(), inKey, valueCFNumber);
		CFRelease(valueCFNumber); valueCFNumber = nullptr;
	}
}// addInteger


/*!
Adds or overwrites a key value with a long integer
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFDictionaryManager::
addLong		(CFStringRef	inKey,
			 SInt32			inValue)
{
	if (false == _dictionary.isMutable())
	{
		throw std::logic_error("warning, attempt to add a long integer value to an immutable dictionary");
	}
	
	CFNumberRef		valueCFNumber = CFNumberCreate(CFGetAllocator(returnCFMutableDictionaryRef()), kCFNumberSInt32Type, &inValue);
	
	
	if (nullptr != valueCFNumber)
	{
		CFDictionarySetValue(returnCFMutableDictionaryRef(), inKey, valueCFNumber);
		CFRelease(valueCFNumber); valueCFNumber = nullptr;
	}
}// addLong


/*!
Returns the specified key’s value, automatically
cast into a Core Foundation array type.  Do not
use this unless you know the value is an array!

The value is retained and must be released by
the caller.

(1.3)
*/
CFArrayRef
CFDictionaryManager::
returnArrayCopy		(CFStringRef	inKey)
const
{
	CFArrayRef		result = nullptr;
	void const*		voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		result = CFUtilities_ArrayCast(voidPointerValue);
		if (nullptr != result) CFRetain(result);
	}
	return result;
}// returnArrayCopy


/*!
Returns the specified key’s value, automatically
cast into a true or false value.  Do not use this
unless you know the value is a Boolean!

(1.3)
*/
Boolean
CFDictionaryManager::
returnFlag	(CFStringRef	inKey)
const
{
	Boolean			result = 0;
	void const*		voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		CFBooleanRef	valueCFBoolean = CFUtilities_BooleanCast(voidPointerValue);
		
		
		if (nullptr != valueCFBoolean)
		{
			result = (kCFBooleanTrue == valueCFBoolean);
		}
	}
	return result;
}// returnFlag


/*!
Returns the specified key’s value, automatically
cast into a floating-point number type.  Do not
use this unless you know the value is a number!

(1.4)
*/
Float32
CFDictionaryManager::
returnFloat		(CFStringRef	inKey)
const
{
	Float32			result = 0;
	void const*		voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		CFNumberRef		valueCFNumber = CFUtilities_NumberCast(voidPointerValue);
		
		
		if (nullptr != valueCFNumber)
		{
			UNUSED_RETURN(Boolean)CFNumberGetValue(valueCFNumber, kCFNumberFloat32Type, &result);
		}
	}
	return result;
}// returnFloat


/*!
Returns the specified key’s value, automatically
cast into a long integer type.  Do not use this
unless you know the value is a number!

This routine has limited support for parsing a
property that is technically a string, but “looks
like” a number.  However, the origin as a string
is not remembered, so changing the value later
with addInteger() will make the key an integer
value.

(1.3)
*/
SInt32
CFDictionaryManager::
returnLong	(CFStringRef	inKey)
const
{
	SInt32			result = 0;
	void const*		voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		CFPropertyListRef	valueCFProperty = CFUtilities_PropertyListCast(voidPointerValue);
		
		
		if (nullptr != valueCFProperty)
		{
			if (CFNumberGetTypeID() == CFGetTypeID(valueCFProperty))
			{
				CFNumberRef		valueCFNumber = CFUtilities_NumberCast(voidPointerValue);
				
				
				if (nullptr != valueCFNumber)
				{
					UNUSED_RETURN(Boolean)CFNumberGetValue(valueCFNumber, kCFNumberSInt32Type, &result);
				}
			}
			else if (CFStringGetTypeID() == CFGetTypeID(valueCFProperty))
			{
				CFStringRef		valueCFString = CFUtilities_StringCast(voidPointerValue);
				
				
				if (nullptr != valueCFString)
				{
					result = CFStringGetIntValue(valueCFString);
				}
			}
		}
	}
	return result;
}// returnLong


/*!
Returns the specified key’s value, automatically
cast into a Core Foundation string type.  Do not
use this unless you know the value is a string!

The result is retained and should be released
when you are finished with it.

(1.3)
*/
CFStringRef
CFDictionaryManager::
returnStringCopy	(CFStringRef	inKey)
const
{
	CFStringRef		result = nullptr;
	void const*		voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		result = CFUtilities_StringCast(voidPointerValue);
		if (nullptr != result) CFRetain(result);
	}
	return result;
}// returnStringCopy


/*!
Returns the specified key’s value, automatically
cast into a Core Foundation property list type.

The result is retained and should be released
when you are finished with it.

(2.0)
*/
CFPropertyListRef
CFDictionaryManager::
returnValueCopy		(CFStringRef	inKey)
const
{
	CFPropertyListRef	result = nullptr;
	void const*			voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		result = CFUtilities_PropertyListCast(voidPointerValue);
		if (nullptr != result) CFRetain(result);
	}
	return result;
}// returnValueCopy

// BELOW IS REQUIRED NEWLINE TO END FILE
