/*!	\file CFKeyValueInterface.h
	\brief Creates an abstraction layer over a dictionary that
	uses Core Foundation keys and values, while providing
	convenient access APIs for common types.
	
	Using this interface, you can set or get values that have
	different sources underneath; currently, the possible
	choices are a CFDictionary or Core Foundation Preferences.
*/
/*###############################################################

	Data Access Library
	© 1998-2020 by Kevin Grant
	
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

// Data Access library includes
#include "CFDictionaryManager.h"
#include "CFRetainRelease.h"



#pragma mark Types

/*!
Contains context information, so that when settings
are stored or retrieved, it is clear where they are.
*/
class CFKeyValueInterface
{
public:
	//! cleans up an interface parent’s data, and any subclass data
	virtual inline
	~CFKeyValueInterface	();
	
	//! inserts array value into dictionary
	virtual void
	addArray	(CFStringRef	inKey,
				 CFArrayRef		inValue) = 0;
	
	//! inserts data value into dictionary
	virtual void
	addData		(CFStringRef	inKey,
				 CFDataRef		inValue) = 0;
	
	//! inserts true/false value into dictionary
	virtual void
	addFlag		(CFStringRef	inKey,
				 Boolean		inValue) = 0;
	
	//! inserts floating-point value into dictionary
	virtual void
	addFloat	(CFStringRef	inKey,
				 Float32		inValue) = 0;
	
	//! inserts short integer value into dictionary
	virtual void
	addInteger	(CFStringRef	inKey,
				 SInt16			inValue) = 0;
	
	//! inserts long integer value into dictionary
	virtual void
	addLong		(CFStringRef	inKey,
				 SInt32			inValue) = 0;
	
	//! inserts string value into dictionary
	virtual void
	addString	(CFStringRef	inKey,
				 CFStringRef	inValue) = 0;
	
	//! a primitive in case none of the others is sufficient
	virtual void
	addValue	(CFStringRef		inKey,
				 CFPropertyListRef	inValue) = 0;
	
	//! removes a value from the dictionary
	virtual void
	deleteValue		(CFStringRef	inKey) = 0;
	
	//! returns true only if a key exists in the dictionary
	virtual Boolean
	exists	(CFStringRef	inKey) const = 0;
	
	//! retrieves an array value from the dictionary (use only if the value really is an array!); release it yourself!
	virtual CFArrayRef
	returnArrayCopy		(CFStringRef	inKey) const = 0;
	
	//! retrieves a true or false value from the dictionary (use only if the value really is a Boolean!)
	virtual Boolean
	returnFlag		(CFStringRef	inKey) const = 0;
	
	//! retrieves a floating-point value from the dictionary (use only if the value really is a number!)
	virtual Float32
	returnFloat		(CFStringRef	inKey) const = 0;
	
	//! retrieves a short integer value from the dictionary (use only if the value really is a number!)
	virtual SInt16
	returnInteger	(CFStringRef	inKey) const = 0;
	
	//! creates an array of CFStringRef values for each key used in this context
	virtual CFArrayRef
	returnKeyListCopy () const = 0;
	
	//! retrieves a long integer value from the dictionary (use only if the value really is a number!)
	virtual SInt32
	returnLong		(CFStringRef	inKey) const = 0;
	
	//! retrieves a string value from the dictionary (use only if the value really is a string!); release it yourself!
	virtual CFStringRef
	returnStringCopy	(CFStringRef	inKey) const = 0;
	
	//! a primitive in case none of the others is sufficient
	virtual CFPropertyListRef
	returnValueCopy		(CFStringRef	inKey) const = 0;
};

/*!
“Implements” all CFKeyValueInterface methods by
assuming another class has those methods available.
*/
template < typename key_value_interface_delegate >
class CFKeyValueInterfaceAdapter:
public CFKeyValueInterface
{
public:
	CFKeyValueInterfaceAdapter	(key_value_interface_delegate&		inDelegate);
	
	virtual void
	addArray	(CFStringRef	inKey,
				 CFArrayRef		inValue)
	{
		_delegate.addArray(inKey, inValue);
	}
	
	virtual void
	addData		(CFStringRef	inKey,
				 CFDataRef		inValue)
	{
		_delegate.addData(inKey, inValue);
	}
	
	virtual void
	addFlag		(CFStringRef	inKey,
				 Boolean		inValue)
	{
		_delegate.addFlag(inKey, inValue);
	}
	
	virtual void
	addFloat	(CFStringRef	inKey,
				 Float32		inValue)
	{
		_delegate.addFloat(inKey, inValue);
	}
	
	virtual void
	addInteger	(CFStringRef	inKey,
				 SInt16			inValue)
	{
		_delegate.addInteger(inKey, inValue);
	}
	
	virtual void
	addLong		(CFStringRef	inKey,
				 SInt32			inValue)
	{
		_delegate.addLong(inKey, inValue);
	}
	
	virtual void
	addString	(CFStringRef	inKey,
				 CFStringRef	inValue)
	{
		_delegate.addString(inKey, inValue);
	}
	
	virtual void
	addValue	(CFStringRef		inKey,
				 CFPropertyListRef	inValue)
	{
		_delegate.addValue(inKey, inValue);
	}
	
	virtual void
	deleteValue		(CFStringRef	inKey)
	{
		_delegate.deleteValue(inKey);
	}
	
	virtual Boolean
	exists	(CFStringRef	inKey)
	const
	{
		return _delegate.exists(inKey);
	}
	
	virtual CFArrayRef
	returnArrayCopy		(CFStringRef	inKey)
	const
	{
		return _delegate.returnArrayCopy(inKey);
	}
	
	virtual Boolean
	returnFlag		(CFStringRef	inKey)
	const
	{
		return _delegate.returnFlag(inKey);
	}
	
	virtual Float32
	returnFloat		(CFStringRef	inKey)
	const
	{
		return _delegate.returnFloat(inKey);
	}
	
	virtual SInt16
	returnInteger	(CFStringRef	inKey)
	const
	{
		return _delegate.returnInteger(inKey);
	}
	
	virtual CFArrayRef
	returnKeyListCopy ()
	const
	{
		return _delegate.returnKeyListCopy();
	}
	
	virtual SInt32
	returnLong		(CFStringRef	inKey)
	const
	{
		return _delegate.returnLong(inKey);
	}
	
	virtual CFStringRef
	returnStringCopy	(CFStringRef	inKey)
	const
	{
		return _delegate.returnStringCopy(inKey);
	}
	
	virtual CFPropertyListRef
	returnValueCopy		(CFStringRef	inKey)
	const
	{
		return _delegate.returnValueCopy(inKey);
	}

private:
	key_value_interface_delegate&	_delegate;
};

/*!
Contains context information, so that when settings
are stored or retrieved, it is clear where they are.
*/
class CFKeyValueDictionary:
public CFKeyValueInterface
{
public:
	explicit
	CFKeyValueDictionary	(CFMutableDictionaryRef		inTarget);
	
	explicit
	CFKeyValueDictionary	(CFDictionaryRef	inSource);
	
	
	// CFKeyValueInterface overrides
	
	//! inserts array value into dictionary
	inline void
	addArray	(CFStringRef	inKey,
				 CFArrayRef		inValue) override;
	
	//! inserts data value into dictionary
	inline void
	addData		(CFStringRef	inKey,
				 CFDataRef		inValue) override;
	
	//! inserts true/false value into dictionary
	inline void
	addFlag		(CFStringRef	inKey,
				 Boolean		inValue) override;
	
	//! inserts floating-point value into dictionary
	inline void
	addFloat	(CFStringRef	inKey,
				 Float32		inValue) override;
	
	//! inserts short integer value into dictionary
	inline void
	addInteger	(CFStringRef	inKey,
				 SInt16			inValue) override;
	
	//! inserts short integer value into dictionary
	inline void
	addLong		(CFStringRef	inKey,
				 SInt32			inValue) override;
	
	//! inserts string value into dictionary
	inline void
	addString	(CFStringRef	inKey,
				 CFStringRef	inValue) override;
	
	//! inserts arbitrary value into dictionary
	inline void
	addValue	(CFStringRef		inKey,
				 CFPropertyListRef	inValue) override;
	
	//! removes an arbitrary value from the dictionary
	inline void
	deleteValue		(CFStringRef	inKey) override;
	
	//! returns true only if a key exists in the dictionary
	inline Boolean
	exists	(CFStringRef	inKey) const override;
	
	//! retrieves an array value from the dictionary (use only if the value really is an array!)
	inline CFArrayRef
	returnArrayCopy		(CFStringRef	inKey) const override;
	
	//! retrieves a true or false value from the dictionary (use only if the value really is a Boolean!)
	inline Boolean
	returnFlag		(CFStringRef	inKey) const override;
	
	//! retrieves a floating-point value from the dictionary (use only if the value really is a number!)
	inline Float32
	returnFloat		(CFStringRef	inKey) const override;
	
	//! retrieves a short integer value from the dictionary (use only if the value really is a number!)
	inline SInt16
	returnInteger	(CFStringRef	inKey) const override;
	
	//! creates an array of CFStringRef values for each key used in this context
	inline CFArrayRef
	returnKeyListCopy () const override;
	
	//! retrieves a long integer value from the dictionary (use only if the value really is a number!)
	inline SInt32
	returnLong		(CFStringRef	inKey) const override;
	
	//! retrieves a string value from the dictionary (use only if the value really is a string!)
	inline CFStringRef
	returnStringCopy	(CFStringRef	inKey) const override;
	
	//! retrieves an arbitrary value from the dictionary
	inline CFPropertyListRef
	returnValueCopy		(CFStringRef	inKey) const override;
	
	
	// new methods
	
	//! return the managed dictionary in a form that cannot be changed
	inline CFDictionaryRef
	returnDictionary () const;
	
	//! return the managed dictionary
	inline CFMutableDictionaryRef
	returnMutableDictionary () const;
	
	//! changes the managed dictionary
	inline void
	setDictionary	(CFMutableDictionaryRef		inNewDictionary);

private:
	CFDictionaryManager		_dataDictionary;	//!< contains the dictionary, and handles changes to it
};

/*!
A context specifically for storing defaults.  It
doesn’t actually manage a dictionary, it uses the
Core Foundation Preferences APIs instead; though,
the consistency of this API compared to that of
other contexts is useful.
*/
class CFKeyValuePreferences:
public CFKeyValueInterface
{
public:
	explicit inline
	CFKeyValuePreferences	(CFStringRef	inTargetApplication = kCFPreferencesCurrentApplication);
	
	
	// CFKeyValueInterface overrides
	
	//! inserts array value into Core Foundation Preferences
	inline void
	addArray	(CFStringRef	inKey,
				 CFArrayRef		inValue) override;
	
	//! inserts data value into Core Foundation Preferences
	inline void
	addData		(CFStringRef	inKey,
				 CFDataRef		inValue) override;
	
	//! inserts true/false value into Core Foundation Preferences
	inline void
	addFlag		(CFStringRef	inKey,
				 Boolean		inValue) override;
	
	//! inserts floating-point value into Core Foundation Preferences
	void
	addFloat	(CFStringRef	inKey,
				 Float32		inValue) override;
	
	//! inserts short integer value into Core Foundation Preferences
	void
	addInteger	(CFStringRef	inKey,
				 SInt16			inValue) override;
	
	//! inserts short integer value into Core Foundation Preferences
	void
	addLong		(CFStringRef	inKey,
				 SInt32			inValue) override;
	
	//! inserts string value into Core Foundation Preferences
	inline void
	addString	(CFStringRef	inKey,
				 CFStringRef	inValue) override;
	
	//! inserts arbitrary value into Core Foundation Preferences
	inline void
	addValue	(CFStringRef		inKey,
				 CFPropertyListRef	inValue) override;
	
	//! removes an arbitrary value from Core Foundation Preferences
	inline void
	deleteValue		(CFStringRef	inKey) override;
	
	//! returns true only if a preference in Core Foundation Preferences has the given key
	inline Boolean
	exists	(CFStringRef	inKey) const override;
	
	//! retrieves an array value from global preferences (use only if the value really is an array!)
	inline CFArrayRef
	returnArrayCopy		(CFStringRef	inKey) const override;
	
	//! retrieves a true or false value from global preferences (use only if the value really is a Boolean!)
	inline Boolean
	returnFlag		(CFStringRef	inKey) const override;
	
	//! retrieves a floating-point value from global preferences (use only if the value really is a number!)
	inline Float32
	returnFloat		(CFStringRef	inKey) const override;
	
	//! retrieves a short integer value from global preferences (use only if the value really is a number!)
	inline SInt16
	returnInteger	(CFStringRef	inKey) const override;
	
	//! creates an array of CFStringRef values for each key used in this context
	inline CFArrayRef
	returnKeyListCopy () const override;
	
	//! retrieves a long integer value from global preferences (use only if the value really is a number!)
	inline SInt32
	returnLong		(CFStringRef	inKey) const override;
	
	//! retrieves a string value from global preferences (use only if the value really is a string!)
	inline CFStringRef
	returnStringCopy	(CFStringRef	inKey) const override;
	
	//! retrieves an arbitrary value from global preferences
	inline CFPropertyListRef
	returnValueCopy		(CFStringRef	inKey) const override;
	
	
	// new methods
	
	//! returns the domain in which preferences are saved
	inline CFStringRef
	returnTargetApplication () const;

private:
	CFRetainRelease		_targetApplication;
};



#pragma mark Public Methods

/*!
Destructor.

(1.3)
*/
CFKeyValueInterface::
~CFKeyValueInterface ()
{
}// CFKeyValueInterface destructor


/*!
Adds or overwrites a key value with an array
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFKeyValueDictionary::
addArray	(CFStringRef	inKey,
			 CFArrayRef		inValue)
{
	_dataDictionary.addArray(inKey, inValue);
}// addArray


/*!
Adds or overwrites a key value with raw data
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFKeyValueDictionary::
addData		(CFStringRef	inKey,
			 CFDataRef		inValue)
{
	_dataDictionary.addData(inKey, inValue);
}// addData


/*!
Adds or overwrites a key value with true or false
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFKeyValueDictionary::
addFlag		(CFStringRef	inKey,
			 Boolean		inValue)
{
	_dataDictionary.addFlag(inKey, inValue);
}// addFlag


/*!
Adds or overwrites a key value with a floating-point
number (which is automatically retained by the dictionary).

(1.3)
*/
void
CFKeyValueDictionary::
addFloat	(CFStringRef	inKey,
			 Float32		inValue)
{
	_dataDictionary.addFloat(inKey, inValue);
}// addFloat


/*!
Adds or overwrites a key value with a short integer
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFKeyValueDictionary::
addInteger	(CFStringRef	inKey,
			 SInt16			inValue)
{
	_dataDictionary.addInteger(inKey, inValue);
}// addInteger


/*!
Adds or overwrites a key value with a long integer
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFKeyValueDictionary::
addLong		(CFStringRef	inKey,
			 SInt32			inValue)
{
	_dataDictionary.addLong(inKey, inValue);
}// addLong


/*!
Adds or overwrites a key value with a string (which
is automatically retained by the dictionary).

(1.3)
*/
void
CFKeyValueDictionary::
addString	(CFStringRef	inKey,
			 CFStringRef	inValue)
{
	_dataDictionary.addString(inKey, inValue);
}// addString


/*!
Adds or overwrites a key value with a value of any type
(which is automatically retained by the dictionary).

(2.0)
*/
void
CFKeyValueDictionary::
addValue	(CFStringRef		inKey,
			 CFPropertyListRef	inValue)
{
	_dataDictionary.addValue(inKey, inValue);
}// addValue


/*!
Removes a key value from the dictionary.

(2.1)
*/
void
CFKeyValueDictionary::
deleteValue		(CFStringRef	inKey)
{
	_dataDictionary.deleteValue(inKey);
}// deleteValue


/*!
Returns true only if the specified key exists in the
dictionary.

(2.1)
*/
Boolean
CFKeyValueDictionary::
exists	(CFStringRef	inKey)
const
{
	return _dataDictionary.exists(inKey);
}// exists


/*!
Returns the value of the specified key, automatically
cast to a Core Foundation array type.  Do not use this
unless you know the value is actually an array!

(1.3)
*/
CFArrayRef
CFKeyValueDictionary::
returnArrayCopy		(CFStringRef	inKey)
const
{
	return _dataDictionary.returnArrayCopy(inKey);
}// returnArrayCopy


/*!
Returns the dictionary managed by this instance in a form
that cannot be changed.

See also returnMutableDictionary().

(2.0)
*/
CFDictionaryRef
CFKeyValueDictionary::
returnDictionary ()
const
{
	return _dataDictionary.returnCFDictionaryRef();
}// returnDictionary


/*!
Returns the value of the specified key, automatically
cast to a true or false value.  Do not use this
unless you know the value is actually a Boolean!

(1.3)
*/
Boolean
CFKeyValueDictionary::
returnFlag	(CFStringRef	inKey)
const
{
	return _dataDictionary.returnFlag(inKey);
}// returnFlag


/*!
Returns the value of the specified key, automatically
cast to a floating-point type.  Do not use this unless
you know the value is actually a number!

(1.4)
*/
Float32
CFKeyValueDictionary::
returnFloat		(CFStringRef	inKey)
const
{
	return _dataDictionary.returnFloat(inKey);
}// returnFloat


/*!
Returns the value of the specified key, automatically
cast to a short integer type.  Do not use this
unless you know the value is actually a number!

(1.3)
*/
SInt16
CFKeyValueDictionary::
returnInteger	(CFStringRef	inKey)
const
{
	return _dataDictionary.returnInteger(inKey);
}// returnInteger


/*!
Returns the list of CFStringRef values for all keys
used in this dictionary.

(2.0)
*/
CFArrayRef
CFKeyValueDictionary::
returnKeyListCopy ()
const
{
	CFDictionaryRef const	kDictionary = _dataDictionary.returnCFDictionaryRef();
	CFIndex const			kDictSize = CFDictionaryGetCount(kDictionary);
	void const**			keyList = new void const*[kDictSize];
	
	
	CFDictionaryGetKeysAndValues(_dataDictionary.returnCFDictionaryRef(), keyList, nullptr/* values */);
	CFArrayRef		result = CFArrayCreate(kCFAllocatorDefault, keyList, kDictSize, &kCFTypeArrayCallBacks);
	delete [] keyList; keyList = nullptr;
	return result;
}// returnKeyListCopy


/*!
Returns the value of the specified key, automatically
cast to a long integer type.  Do not use this
unless you know the value is actually a number!

(1.3)
*/
SInt32
CFKeyValueDictionary::
returnLong	(CFStringRef	inKey)
const
{
	return _dataDictionary.returnLong(inKey);
}// returnLong


/*!
Returns the dictionary managed by this instance.

See also returnDictionary().

(2.0)
*/
CFMutableDictionaryRef
CFKeyValueDictionary::
returnMutableDictionary ()
const
{
	return _dataDictionary.returnCFMutableDictionaryRef();
}// returnMutableDictionary


/*!
Returns the value of the specified key, automatically
cast to a Core Foundation string type.  Do not use this
unless you know the value is actually a string!

(1.3)
*/
CFStringRef
CFKeyValueDictionary::
returnStringCopy	(CFStringRef	inKey)
const
{
	return _dataDictionary.returnStringCopy(inKey);
}// returnStringCopy


/*!
Returns the value of the specified key, automatically
cast to a Core Foundation type.

(2.0)
*/
CFPropertyListRef
CFKeyValueDictionary::
returnValueCopy		(CFStringRef	inKey)
const
{
	return _dataDictionary.returnValueCopy(inKey);
}// returnValueCopy


/*!
Changes the dictionary managed by this instance.

(1.3)
*/
void
CFKeyValueDictionary::
setDictionary	(CFMutableDictionaryRef		inNewDictionary)
{
	_dataDictionary.setCFMutableDictionaryRef(inNewDictionary);
}// setDictionary


/*!
Constructor.

(1.3)
*/
CFKeyValuePreferences::
CFKeyValuePreferences	(CFStringRef	inTargetApplication)
:
CFKeyValueInterface(),
_targetApplication(inTargetApplication, CFRetainRelease::kNotYetRetained)
{
}// CFKeyValuePreferences 2-argument constructor


/*!
Adds or overwrites a key value with an array, in
the global application preferences.

(1.3)
*/
void
CFKeyValuePreferences::
addArray	(CFStringRef	inKey,
			 CFArrayRef		inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, _targetApplication.returnCFStringRef());
}// addArray


/*!
Adds or overwrites a key value with raw data, in
the global application preferences.

(1.3)
*/
void
CFKeyValuePreferences::
addData		(CFStringRef	inKey,
			 CFDataRef		inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, _targetApplication.returnCFStringRef());
}// addData


/*!
Adds or overwrites a key value with true or false,
in the global application preferences.

(1.3)
*/
void
CFKeyValuePreferences::
addFlag		(CFStringRef	inKey,
			 Boolean		inValue)
{
	CFPreferencesSetAppValue(inKey, (inValue) ? kCFBooleanTrue : kCFBooleanFalse,
								_targetApplication.returnCFStringRef());
}// addFlag


/*!
Adds or overwrites a key value with a string,
in the global application preferences.

(1.3)
*/
void
CFKeyValuePreferences::
addString	(CFStringRef	inKey,
			 CFStringRef	inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, _targetApplication.returnCFStringRef());
}// addString


/*!
Adds or overwrites a key value in the global
application preferences.

(1.3)
*/
void
CFKeyValuePreferences::
addValue	(CFStringRef		inKey,
			 CFPropertyListRef	inValue)
{
	CFPreferencesSetAppValue(inKey, inValue, _targetApplication.returnCFStringRef());
}// addValue


/*!
Removes a key value from the global application
preferences.

(2.1)
*/
void
CFKeyValuePreferences::
deleteValue		(CFStringRef	inKey)
{
	CFPreferencesSetAppValue(inKey, nullptr/* value */, _targetApplication.returnCFStringRef());
}// deleteValue


/*!
Returns true only if the specified key exists in
the global application preferences.

(2.1)
*/
Boolean
CFKeyValuePreferences::
exists	(CFStringRef	inKey)
const
{
	CFRetainRelease		ignoredValue(CFPreferencesCopyAppValue(inKey, _targetApplication.returnCFStringRef()),
										CFRetainRelease::kAlreadyRetained);
	
	
	return ignoredValue.exists();
}// exists


/*!
Returns the array value of the specified key in
the global application preferences.  Use only if
you know the value is actually an array!

(1.3)
*/
CFArrayRef
CFKeyValuePreferences::
returnArrayCopy		(CFStringRef	inKey)
const
{
	return CFUtilities_ArrayCast(CFPreferencesCopyAppValue(inKey, _targetApplication.returnCFStringRef()));
}// returnArrayCopy


/*!
Returns the true or false value of the specified key
in the global application preferences.  Use only if
you know the value is actually a Boolean!

(1.3)
*/
Boolean
CFKeyValuePreferences::
returnFlag	(CFStringRef	inKey)
const
{
	return CFPreferencesGetAppBooleanValue(inKey, _targetApplication.returnCFStringRef(),
											nullptr/* exists/valid flag pointer */);
}// returnFlag


/*!
Returns the floating-point value of the specified key
in the global application preferences.  Use only if
you know the value is actually a number!

(1.4)
*/
Float32
CFKeyValuePreferences::
returnFloat		(CFStringRef	inKey)
const
{
	CFNumberRef		number = CFUtilities_NumberCast(CFPreferencesCopyAppValue(inKey, _targetApplication.returnCFStringRef()));
	Float32			result = 0;
	
	
	if (nullptr != number)
	{
		UNUSED_RETURN(Boolean)CFNumberGetValue(number, kCFNumberFloat32Type, &result);
		CFRelease(number); number = nullptr;
	}
	
	return result;
}// returnFloat


/*!
Returns the short integer value of the specified key
in the global application preferences.  Use only if
you know the value is actually a number!

(1.3)
*/
SInt16
CFKeyValuePreferences::
returnInteger	(CFStringRef	inKey)
const
{
	return STATIC_CAST(CFPreferencesGetAppIntegerValue(inKey, _targetApplication.returnCFStringRef(),
														nullptr/* exists/valid flag pointer */),
						SInt16);
}// returnInteger


/*!
Returns the list of CFStringRef values for all keys
used in this dictionary.

(2.0)
*/
CFArrayRef
CFKeyValuePreferences::
returnKeyListCopy ()
const
{
	return CFPreferencesCopyKeyList(_targetApplication.returnCFStringRef(), kCFPreferencesCurrentUser,
									kCFPreferencesAnyHost);
}// returnKeyListCopy


/*!
Returns the long integer value of the specified key
in the global application preferences.  Use only if
you know the value is actually a number!

(1.3)
*/
SInt32
CFKeyValuePreferences::
returnLong	(CFStringRef	inKey)
const
{
	return STATIC_CAST(CFPreferencesGetAppIntegerValue(inKey, _targetApplication.returnCFStringRef(),
														nullptr/* exists/valid flag pointer */), SInt32);
}// returnLong


/*!
Returns the string value of the specified key in
the global application preferences.  Use only if
you know the value is actually a string!

(1.3)
*/
CFStringRef
CFKeyValuePreferences::
returnStringCopy	(CFStringRef	inKey)
const
{
	return CFUtilities_StringCast(CFPreferencesCopyAppValue(inKey, _targetApplication.returnCFStringRef()));
}// returnStringCopy


/*!
Returns the value of the specified key in the global
application preferences.

(2.0)
*/
CFPropertyListRef
CFKeyValuePreferences::
returnValueCopy		(CFStringRef	inKey)
const
{
	return CFPreferencesCopyAppValue(inKey, _targetApplication.returnCFStringRef());
}// returnValueCopy


/*!
Returns the domain in which preferences are saved.

(1.3)
*/
CFStringRef
CFKeyValuePreferences::
returnTargetApplication ()
const
{
	return _targetApplication.returnCFStringRef();
}// returnTargetApplication

// BELOW IS REQUIRED NEWLINE TO END FILE
