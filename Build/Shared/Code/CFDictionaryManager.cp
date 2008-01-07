/*###############################################################

	CFDictionaryManager.cp
	
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

// library includes
#include <CFDictionaryManager.h>



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
	CFNumberRef		valueCFNumber = nullptr;
	
	
	valueCFNumber = CFNumberCreate(CFGetAllocator(returnCFMutableDictionaryRef()), kCFNumberFloat32Type, &inValue);
	if (nullptr != valueCFNumber)
	{
		CFDictionarySetValue(returnCFMutableDictionaryRef(), inKey, valueCFNumber);
		CFRelease(valueCFNumber), valueCFNumber = nullptr;
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
	CFNumberRef		valueCFNumber = nullptr;
	
	
	valueCFNumber = CFNumberCreate(CFGetAllocator(returnCFMutableDictionaryRef()), kCFNumberSInt16Type, &inValue);
	if (nullptr != valueCFNumber)
	{
		CFDictionarySetValue(returnCFMutableDictionaryRef(), inKey, valueCFNumber);
		CFRelease(valueCFNumber), valueCFNumber = nullptr;
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
	CFNumberRef		valueCFNumber = nullptr;
	
	
	valueCFNumber = CFNumberCreate(CFGetAllocator(returnCFMutableDictionaryRef()), kCFNumberSInt32Type, &inValue);
	if (nullptr != valueCFNumber)
	{
		CFDictionarySetValue(returnCFMutableDictionaryRef(), inKey, valueCFNumber);
		CFRelease(valueCFNumber), valueCFNumber = nullptr;
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
			(Boolean)CFNumberGetValue(valueCFNumber, kCFNumberFloat32Type, &result);
		}
	}
	return result;
}// returnFloat


/*!
Returns the specified key’s value, automatically
cast into a short integer type.  Do not use this
unless you know the value is a number!

(1.3)
*/
SInt16
CFDictionaryManager::
returnInteger	(CFStringRef	inKey)
const
{
	SInt16			result = 0;
	void const*		voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		CFNumberRef		valueCFNumber = CFUtilities_NumberCast(voidPointerValue);
		
		
		if (nullptr != valueCFNumber)
		{
			(Boolean)CFNumberGetValue(valueCFNumber, kCFNumberSInt16Type, &result);
		}
	}
	return result;
}// returnInteger


/*!
Returns the specified key’s value, automatically
cast into a long integer type.  Do not use this
unless you know the value is a number!

(1.3)
*/
SInt32
CFDictionaryManager::
returnLong	(CFStringRef	inKey)
const
{
	SInt32			result = 0L;
	void const*		voidPointerValue = nullptr;
	
	
	if (CFDictionaryGetValueIfPresent(returnCFDictionaryRef(), inKey, &voidPointerValue))
	{
		CFNumberRef		valueCFNumber = CFUtilities_NumberCast(voidPointerValue);
		
		
		if (nullptr != valueCFNumber)
		{
			(Boolean)CFNumberGetValue(valueCFNumber, kCFNumberSInt32Type, &result);
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

// BELOW IS REQUIRED NEWLINE TO END FILE
