/*!	\file CFKeyValueInterface.cp
	\brief Creates an abstraction layer over a dictionary that
	uses Core Foundation keys and values, while providing
	convenient access APIs for common types.
*/
/*###############################################################

	Data Access Library
	© 1998-2014 by Kevin Grant
	
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

#include <CFKeyValueInterface.h>
#include <UniversalDefines.h>



#pragma mark Public Methods

/*!
Typical constructor, for mutable dictionaries.

(1.3)
*/
CFKeyValueDictionary::
CFKeyValueDictionary	(CFMutableDictionaryRef		inTarget)
:
CFKeyValueInterface(),
_dataDictionary(inTarget)
{
}// CFKeyValueDictionary mutable reference constructor


/*!
Special constructor to enable read methods on an
immutable dictionary.  Any attempt to call methods
to change the dictionary will have no effect and
will emit warnings in debug mode.

(1.3)
*/
CFKeyValueDictionary::
CFKeyValueDictionary	(CFDictionaryRef	inSource)
:
CFKeyValueInterface(),
_dataDictionary(inSource) // the CFDictionaryManager class automatically disables add routines due to the constant reference
{
}// CFKeyValueDictionary immutable reference constructor


/*!
Adds or overwrites a key value with a floating-point
value, in the global application preferences.

(1.4)
*/
void
CFKeyValuePreferences::
addFloat	(CFStringRef	inKey,
			 Float32		inValue)
{
	CFNumberRef		valueCFNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &inValue);
	
	
	if (nullptr != valueCFNumber)
	{
		CFPreferencesSetAppValue(inKey, valueCFNumber, _targetApplication.returnCFStringRef());
		CFRelease(valueCFNumber), valueCFNumber = nullptr;
	}
}// addFloat


/*!
Adds or overwrites a key value with a short integer,
in the global application preferences.

(1.3)
*/
void
CFKeyValuePreferences::
addInteger	(CFStringRef	inKey,
			 SInt16			inValue)
{
	CFNumberRef		valueCFNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &inValue);
	
	
	if (nullptr != valueCFNumber)
	{
		CFPreferencesSetAppValue(inKey, valueCFNumber, _targetApplication.returnCFStringRef());
		CFRelease(valueCFNumber), valueCFNumber = nullptr;
	}
}// addInteger


/*!
Adds or overwrites a key value with a long integer,
in the global application preferences.

(1.3)
*/
void
CFKeyValuePreferences::
addLong		(CFStringRef	inKey,
			 SInt32			inValue)
{
	CFNumberRef		valueCFNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inValue);
	
	
	if (nullptr != valueCFNumber)
	{
		CFPreferencesSetAppValue(inKey, valueCFNumber, _targetApplication.returnCFStringRef());
		CFRelease(valueCFNumber), valueCFNumber = nullptr;
	}
}// addLong

// BELOW IS REQUIRED NEWLINE TO END FILE
