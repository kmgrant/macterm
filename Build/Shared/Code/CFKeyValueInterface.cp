/*###############################################################

	CFKeyValueInterface.cp
	
	Data Access Library 1.3
	© 1998-2006 by Kevin Grant
	
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
#include <CFKeyValueInterface.h>



#pragma mark Public Methods

/*!
Constructor.

(1.3)
*/
CFKeyValueDictionary::
CFKeyValueDictionary	(CFMutableDictionaryRef		inTarget)
:
CFKeyValueInterface(),
_dataDictionary(inTarget)
{
}// CFKeyValueDictionary 3-argument constructor


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
