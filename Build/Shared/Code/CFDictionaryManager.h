/*!	\file CFDictionaryManager.h
	\brief Adds strongly typed interfaces to manipulate data
	in a mutable Core Foundation dictionary whose entries are
	entirely Core Foundation types.
*/
/*###############################################################

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

#ifndef __CFDICTIONARYMANAGER__
#define __CFDICTIONARYMANAGER__

// Data Access library includes
#include "CFRetainRelease.h"
#include "CFUtilities.h"
#include "Console.h"//tmp



#pragma mark Types

/*!
Helps manage a dictionary whose values are Core Foundation types.
Despite the Core Foundation type contents, the APIs often use
convenient C types instead (e.g. for integers and flags).
*/
class CFDictionaryManager
{
public:
	inline
	CFDictionaryManager ();
	
	inline
	CFDictionaryManager	(CFMutableDictionaryRef);
	
	inline CFDictionaryRef
	returnCFDictionaryRef () const;
	
	inline CFMutableDictionaryRef
	returnCFMutableDictionaryRef () const;
	
	inline void
	setCFMutableDictionaryRef	(CFMutableDictionaryRef);
	
	inline void
	addArray	(CFStringRef, CFArrayRef);
	
	inline void
	addData		(CFStringRef, CFDataRef);
	
	inline void
	addFlag		(CFStringRef, Boolean);
	
	void
	addInteger	(CFStringRef, SInt16);
	
	void
	addLong		(CFStringRef, SInt32);
	
	inline void
	addString	(CFStringRef, CFStringRef);
	
	CFArrayRef
	returnArrayCopy	(CFStringRef) const;
	
	Boolean
	returnFlag	(CFStringRef) const;
	
	SInt16
	returnInteger	(CFStringRef) const;
	
	SInt32
	returnLong	(CFStringRef) const;
	
	CFStringRef
	returnStringCopy	(CFStringRef) const;

private:
	CFRetainRelease		_dictionary;
};



#pragma mark Public Methods

/*!
Creates a null reference.

(1.3)
*/
CFDictionaryManager::
CFDictionaryManager ()
:
_dictionary()
{
}// default constructor


/*!
Retains a mutable dictionary.

(1.3)
*/
CFDictionaryManager::
CFDictionaryManager	(CFMutableDictionaryRef		inDictionary)
:
_dictionary(inDictionary)
{
	// defer to superclass
}// 1-argument constructor


/*!
Adds or overwrites a key value with an array
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFDictionaryManager::
addArray	(CFStringRef	inKey,
			 CFArrayRef		inValue)
{
	CFDictionarySetValue(_dictionary.returnCFMutableDictionaryRef(), inKey, inValue);
}// addArray


/*!
Adds or overwrites a key value with raw data
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFDictionaryManager::
addData		(CFStringRef	inKey,
			 CFDataRef		inValue)
{
	CFDictionarySetValue(_dictionary.returnCFMutableDictionaryRef(), inKey, inValue);
}// addData


/*!
Adds or overwrites a key value with true or false
(which is automatically retained by the dictionary).

(1.3)
*/
void
CFDictionaryManager::
addFlag		(CFStringRef	inKey,
			 Boolean		inValue)
{
	CFDictionarySetValue(_dictionary.returnCFMutableDictionaryRef(), inKey, (inValue) ? kCFBooleanTrue : kCFBooleanFalse);
}// addFlag


/*!
Adds or overwrites a key value with a string (which
is automatically retained by the dictionary).

(1.3)
*/
void
CFDictionaryManager::
addString	(CFStringRef	inKey,
			 CFStringRef	inValue)
{
	CFDictionarySetValue(_dictionary.returnCFMutableDictionaryRef(), inKey, inValue);
}// addString


/*!
Although the underlying dictionary is mutable, this
method returns it as an immutable reference, which is
convenient in some use cases.

(1.3)
*/
CFDictionaryRef
CFDictionaryManager::
returnCFDictionaryRef ()
const
{
	return _dictionary.returnCFDictionaryRef();
}// returnCFDictionaryRef


/*!
Returns the CFMutableDictionaryRef being managed.

(1.3)
*/
CFMutableDictionaryRef
CFDictionaryManager::
returnCFMutableDictionaryRef ()
const
{
	return _dictionary.returnCFMutableDictionaryRef();
}// returnCFMutableDictionaryRef


/*!
Changes the CFMutableDictionaryRef being managed.

IMPORTANT:	You should probably only do this once, if
			at all, after a default construction.  Any
			past method calls will not affect the new
			dictionary’s contents.

(1.3)
*/
void
CFDictionaryManager::
setCFMutableDictionaryRef	(CFMutableDictionaryRef		inDictionary)
{
	_dictionary.setCFMutableDictionaryRef(inDictionary);
}// setCFMutableDictionaryRef

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
