/*!	\file CFRetainRelease.h
	\brief Convenient wrapper class for a Core Foundation object
	(such as a CFStringRef, CFArrayRef, etc.).
	
	Use this to ensure a reference is automatically retained at
	object construction or duplication time, and automatically
	released when the object is destroyed.  Wrappers mean that
	custom constructors and destructors are not required for a
	class if it simply wants to hold onto a Core Foundation
	object reference indefinitely.
	
	Debugging assertions are in place that call Core Foundation
	APIs to verify the type IDs of references both when you
	first construct an instance of this class and whenever you
	ask for it to be converted into a specific (non-CFTypeRef)
	type.
	
	This class also allows references to be null.  You can make
	an existing object of this type equal to nullptr by calling
	the clear() method, or by assigning to it a temporary
	instance with no arguments, such as
	"myCFRetainReleaseInstance = CFRetainRelease();"; or, use
	"myCFRetainReleaseInstance = CFRetainRelease(nullptr);" if
	you want to be more explicit (these are equivalent).
	
	Finally, CFRetainRelease can be release-only: it can be
	initialized by a reference that is already retained (such as
	something returned by a Create function), in which case no
	CFRetain() is called but CFRelease() is still called.
*/
/*###############################################################

	Data Access Library 2.6
	© 1998-2012 by Kevin Grant
	
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

#ifndef __CFRETAINRELEASE__
#define __CFRETAINRELEASE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>



#pragma mark Types

/*!
Use instead of a regular Core Foundation reference in
order to have the reference automatically retained with
CFRetain() when constructed, assigned or copied, and
released with CFRelease() when it goes out of scope or
is assigned, etc.

It is possible to have a nullptr value, and no CFRetain()
or CFRelease() occurs in this case.  It is therefore safe
to initialize to nullptr and later assign a value that
should be retained and released, etc.
*/
class CFRetainRelease
{
public:
	inline
	CFRetainRelease ();
	
	inline
	CFRetainRelease (CFRetainRelease const&);
	
	inline
	CFRetainRelease (CFArrayRef, bool = false);
	
	inline
	CFRetainRelease (CFBundleRef, bool = false);
	
	inline
	CFRetainRelease (CFDictionaryRef, bool = false);
	
	inline
	CFRetainRelease (CFMutableArrayRef, bool = false);
	
	inline
	CFRetainRelease (CFMutableDictionaryRef, bool = false);
	
	inline
	CFRetainRelease (CFMutableStringRef, bool = false);
	
	inline
	CFRetainRelease (CFStringRef, bool = false);
	
	inline
	CFRetainRelease (CFTypeRef, bool = false);
	
	inline
	CFRetainRelease (HIObjectRef, bool = false);
	
	inline
	CFRetainRelease (PasteboardRef, bool = false);
	
	virtual inline
	~CFRetainRelease ();
	
	// IMPORTANT: It is clearer to call setCFTypeRef(), that is recommended.
	// This assignment operator only exists to satisfy STL container implementations
	// and other generic constructs that could not know about specific class methods.
	virtual inline CFRetainRelease&
	operator = (CFRetainRelease const&);
	
	virtual inline bool
	operator == (CFRetainRelease const&);
	
	virtual inline bool
	operator != (CFRetainRelease const&);
	
	inline void
	clear ();
	
	inline bool
	exists () const;
	
	inline bool
	isMutable () const;
	
	inline CFArrayRef
	returnCFArrayRef () const;
	
	inline CFBundleRef
	returnCFBundleRef () const;
	
	inline CFDictionaryRef
	returnCFDictionaryRef () const;
	
	inline CFMutableArrayRef
	returnCFMutableArrayRef () const;
	
	inline CFMutableDictionaryRef
	returnCFMutableDictionaryRef () const;
	
	inline CFMutableStringRef
	returnCFMutableStringRef () const;
	
	inline CFStringRef
	returnCFStringRef () const;
	
	inline CFTypeRef
	returnCFTypeRef () const;
	
	inline HIObjectRef
	returnHIObjectRef () const;
	
	inline PasteboardRef
	returnPasteboardRef () const;
	
	inline void
	setCFMutableArrayRef (CFMutableArrayRef, bool = false);
	
	inline void
	setCFMutableDictionaryRef (CFMutableDictionaryRef, bool = false);
	
	inline void
	setCFMutableStringRef (CFMutableStringRef, bool = false);
	
	inline void
	setCFTypeRef (CFTypeRef, bool = false);

protected:

private:
	// IMPORTANT: code below assumes these are all the same size and inherit from CFTypeRef
	union
	{
		union
		{
			CFTypeRef			unspecified;
			CFArrayRef			array;
			CFDictionaryRef		dictionary;
			HIObjectRef			humanInterfaceObject;
			CFStringRef			string;
		} _constant;
		
		union
		{
			CFTypeRef				unspecified;
			CFMutableArrayRef		array;
			CFBundleRef				bundle;
			CFMutableDictionaryRef	dictionary;
			CFMutableStringRef		string;
			PasteboardRef			pasteboard;
		} _modifiable;
	} _typeAs;
	
	bool	_isMutable;
};



#pragma mark Public Methods

/*!
Creates a nullptr reference.

(1.1)
*/
CFRetainRelease::
CFRetainRelease ()
:
_isMutable(false)
{
	_typeAs._constant.unspecified = nullptr;
	assert(nullptr == _typeAs._constant.unspecified);
}// default constructor


/*!
Creates a new reference using the value of an
existing one.  CFRetain() is called on the
reference.

(1.1)
*/
CFRetainRelease::
CFRetainRelease		(CFRetainRelease const&		inCopy)
:
_isMutable(inCopy._isMutable)
{
	_typeAs = inCopy._typeAs;
	assert(_typeAs._constant.unspecified == inCopy._typeAs._constant.unspecified);
	if (nullptr != _typeAs._constant.unspecified)
	{
		CFRetain(_typeAs._constant.unspecified);
	}
}// copy constructor


/*!
Creates a new reference using the value of an
existing one that is a generic Core Foundation
type.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFTypeRef		inType,
					 bool			inIsAlreadyRetained)
:
_isMutable(false)
{
	_typeAs._constant.unspecified = inType;
	assert(_typeAs._constant.unspecified == inType);
	if ((nullptr != _typeAs._constant.unspecified) &&
		(false == inIsAlreadyRetained))
	{
		CFRetain(_typeAs._constant.unspecified);
	}
}// unspecified type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Array
type.  In debug mode, an assertion failure will
occur if the given reference is not a CFArrayRef.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFArrayRef		inType,
					 bool			inIsAlreadyRetained)
:
_isMutable(false)
{
	_typeAs._constant.array = inType;
	assert(_typeAs._constant.array == inType);
	if (nullptr != _typeAs._constant.array)
	{
		assert(CFGetTypeID(_typeAs._constant.array) == CFArrayGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._constant.array);
		}
	}
}// array type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Bundle
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFBundleRef.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFBundleRef	inType,
					 bool			inIsAlreadyRetained)
:
_isMutable(true)
{
	_typeAs._modifiable.bundle = inType;
	assert(_typeAs._modifiable.bundle == inType);
	if (nullptr != _typeAs._modifiable.bundle)
	{
		assert(CFGetTypeID(_typeAs._modifiable.bundle) == CFBundleGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._modifiable.bundle);
		}
	}
}// bundle type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Dictionary
type.  In debug mode, an assertion failure will
occur if the given reference is not a CFDictionaryRef.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFDictionaryRef	inType,
					 bool				inIsAlreadyRetained)
:
_isMutable(false)
{
	_typeAs._constant.dictionary = inType;
	assert(_typeAs._constant.dictionary == inType);
	if (nullptr != _typeAs._constant.dictionary)
	{
		assert(CFGetTypeID(_typeAs._constant.dictionary) == CFDictionaryGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._constant.dictionary);
		}
	}
}// dictionary type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Mutable Array
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFMutableArrayRef.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableArrayRef		inType,
					 bool					inIsAlreadyRetained)
:
_isMutable(true)
{
	_typeAs._modifiable.array = inType;
	assert(_typeAs._modifiable.array == inType);
	if (nullptr != _typeAs._modifiable.array)
	{
		assert(CFGetTypeID(_typeAs._modifiable.array) == CFArrayGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._modifiable.array);
		}
	}
}// mutable array type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Mutable Dictionary
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFMutableDictionaryRef.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableDictionaryRef		inType,
					 bool						inIsAlreadyRetained)
:
_isMutable(true)
{
	_typeAs._modifiable.dictionary = inType;
	assert(_typeAs._modifiable.dictionary == inType);
	if (nullptr != _typeAs._modifiable.dictionary)
	{
		assert(CFGetTypeID(_typeAs._modifiable.dictionary) == CFDictionaryGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._modifiable.dictionary);
		}
	}
}// mutable dictionary type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Mutable String
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFMutableStringRef.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableStringRef		inType,
					 bool					inIsAlreadyRetained)
:
_isMutable(true)
{
	_typeAs._modifiable.string = inType;
	assert(_typeAs._modifiable.string == inType);
	if (nullptr != _typeAs._modifiable.string)
	{
		assert(CFGetTypeID(_typeAs._modifiable.string) == CFStringGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._modifiable.string);
		}
	}
}// mutable string type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation String
type.  In debug mode, an assertion failure will
occur if the given reference is not a CFStringRef.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFStringRef	inType,
					 bool			inIsAlreadyRetained)
:
_isMutable(false)
{
	_typeAs._constant.string = inType;
	assert(_typeAs._constant.string == inType);
	if (nullptr != _typeAs._constant.string)
	{
		assert(CFGetTypeID(_typeAs._constant.string) == CFStringGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._constant.string);
		}
	}
}// string type constructor


/*!
Creates a new reference using the value of an
existing one that is a Human Interface Object
type.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(HIObjectRef	inType,
					 bool			inIsAlreadyRetained)
:
_isMutable(false)
{
	_typeAs._constant.humanInterfaceObject = inType;
	assert(_typeAs._constant.humanInterfaceObject == inType);
	if ((nullptr != _typeAs._constant.humanInterfaceObject) &&
		(false == inIsAlreadyRetained))
	{
		CFRetain(_typeAs._constant.humanInterfaceObject);
	}
}// human interface object type constructor


/*!
Creates a new reference using the value of an
existing one that is a Pasteboard type.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is true.  Regardless,
CFRelease() is called at destruction or
reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.1)
*/
CFRetainRelease::
CFRetainRelease		(PasteboardRef	inType,
					 bool			inIsAlreadyRetained)
:
_isMutable(true)
{
	_typeAs._modifiable.pasteboard = inType;
	assert(_typeAs._modifiable.pasteboard == inType);
	if (nullptr != _typeAs._modifiable.pasteboard)
	{
		assert(CFGetTypeID(_typeAs._modifiable.pasteboard) == PasteboardGetTypeID());
		if (false == inIsAlreadyRetained)
		{
			CFRetain(_typeAs._modifiable.pasteboard);
		}
	}
}// pasteboard type constructor


/*!
Calls CFRelease() on the reference kept by this
class instance, if any.

(1.1)
*/
CFRetainRelease::
~CFRetainRelease ()
{
	if (nullptr != _typeAs._constant.unspecified) CFRelease(_typeAs._constant.unspecified);
}// destructor


/*!
Equivalent to using the copy constructor to make
a new instance, but updates this instance.

(1.1)
*/
CFRetainRelease&
CFRetainRelease::
operator =	(CFRetainRelease const&		inCopy)
{
	if (&inCopy != this)
	{
		setCFTypeRef(inCopy.returnCFTypeRef());
		_isMutable = inCopy._isMutable;
	}
	return *this;
}// operator =


/*!
Performs an equality check on a pair of reference
objects.  This is defined as the result of CFEqual().
This allows you to embed a CFRetainRelease object
sensibly in something like an STL container.

(1.2)
*/
bool
CFRetainRelease::
operator ==	(CFRetainRelease const&		inOther)
{
	return CFEqual(returnCFTypeRef(), inOther.returnCFTypeRef());
}// operator ==


/*!
The inverse of operator ==().

(1.2)
*/
bool
CFRetainRelease::
operator !=	(CFRetainRelease const&		inOther)
{
	return ! operator == (inOther);
}// operator !=


/*!
Sets this reference to nullptr, calling CFRelease()
(if necessary) on the previous value.

(1.1)
*/
void
CFRetainRelease::
clear ()
{
	if (nullptr != _typeAs._constant.unspecified)
	{
		CFRelease(_typeAs._constant.unspecified);
	}
	_typeAs._constant.unspecified = nullptr;
	_isMutable = false;
}// clear


/*!
Returns true if the internal reference is not nullptr.

(1.1)
*/
bool
CFRetainRelease::
exists ()
const
{
	return (nullptr != _typeAs._constant.unspecified);
}// exists


/*!
Returns true if the internal reference is to an object
that can be changed, from this point of view.

In other words, if you use an immutable reference
constructor to retain a reference to an object that is
technically mutable, isMutable() returns false.  This
class relies on its constructors or reassignment to
determine the mutability of its reference.

(2.0)
*/
bool
CFRetainRelease::
isMutable ()
const
{
	return _isMutable;
}// isMutable


/*!
Convenience method to cast the internal reference
into a CFArrayRef.  In debug mode, an assertion
failure will occur if the reference is not really
a CFArrayRef or CFMutableArrayRef.

(1.1)
*/
CFArrayRef
CFRetainRelease::
returnCFArrayRef ()
const
{
	if (_isMutable) return returnCFMutableArrayRef();
	assert((nullptr == _typeAs._constant.array) || (CFGetTypeID(_typeAs._constant.array) == CFArrayGetTypeID()));
	return _typeAs._constant.array;
}// returnCFArrayRef


/*!
Convenience method to cast the internal reference
into a CFBundleRef.  In debug mode, an assertion
failure will occur if the reference is not really
a CFBundleRef.

(1.4)
*/
CFBundleRef
CFRetainRelease::
returnCFBundleRef ()
const
{
	assert(_isMutable);
	assert((nullptr == _typeAs._modifiable.bundle) || (CFGetTypeID(_typeAs._modifiable.bundle) == CFBundleGetTypeID()));
	return _typeAs._modifiable.bundle;
}// returnCFBundleRef


/*!
Convenience method to cast the internal reference
into a CFDictionaryRef.  In debug mode, an assertion
failure will occur if the reference is not really
a CFDictionaryRef or CFMutableDictionaryRef.

(1.2)
*/
CFDictionaryRef
CFRetainRelease::
returnCFDictionaryRef ()
const
{
	if (_isMutable) return returnCFMutableDictionaryRef();
	assert((nullptr == _typeAs._constant.dictionary) || (CFGetTypeID(_typeAs._constant.dictionary) == CFDictionaryGetTypeID()));
	return _typeAs._constant.dictionary;
}// returnCFDictionaryRef


/*!
Convenience method to cast the internal reference
into a CFMutableArrayRef.  In debug mode, an assertion
failure will occur if the reference is not really
a CFMutableArrayRef.

(1.3)
*/
CFMutableArrayRef
CFRetainRelease::
returnCFMutableArrayRef ()
const
{
	assert(_isMutable);
	assert((nullptr == _typeAs._modifiable.array) || (CFGetTypeID(_typeAs._modifiable.array) == CFArrayGetTypeID()));
	return _typeAs._modifiable.array;
}// returnCFMutableArrayRef


/*!
Convenience method to cast the internal reference
into a CFMutableDictionaryRef.  In debug mode, an
assertion failure will occur if the reference is
not really a CFMutableDictionaryRef.

(1.3)
*/
CFMutableDictionaryRef
CFRetainRelease::
returnCFMutableDictionaryRef ()
const
{
	assert(_isMutable);
	assert((nullptr == _typeAs._modifiable.dictionary) || (CFGetTypeID(_typeAs._modifiable.dictionary) == CFDictionaryGetTypeID()));
	return _typeAs._modifiable.dictionary;
}// returnCFMutableDictionaryRef


/*!
Convenience method to cast the internal reference
into a CFMutableStringRef.  In debug mode, an
assertion failure will occur if the reference is
not really a CFMutableStringRef.

(2.0)
*/
CFMutableStringRef
CFRetainRelease::
returnCFMutableStringRef ()
const
{
	assert(_isMutable);
	assert((nullptr == _typeAs._modifiable.string) || (CFGetTypeID(_typeAs._modifiable.string) == CFStringGetTypeID()));
	return _typeAs._modifiable.string;
}// returnCFMutableStringRef


/*!
Convenience method to cast the internal reference
into a CFStringRef.  In debug mode, an assertion
failure will occur if the reference is not really
a CFStringRef or CFMutableStringRef.

(1.1)
*/
CFStringRef
CFRetainRelease::
returnCFStringRef ()
const
{
	if (_isMutable) return returnCFMutableStringRef();
	assert((nullptr == _typeAs._constant.string) || (CFGetTypeID(_typeAs._constant.string) == CFStringGetTypeID()));
	return _typeAs._constant.string;
}// returnCFStringRef


/*!
Returns the CFTypeRef that this class instance is
storing (and has retained), or nullptr if the internal
reference is empty.

Use this if there is no more specific routine to
return the actual type, or if you know you are doing
a raw value check and do not require type assertions.

(1.1)
*/
CFTypeRef
CFRetainRelease::
returnCFTypeRef ()
const
{
	if (_isMutable) return _typeAs._modifiable.unspecified;
	return _typeAs._constant.unspecified;
}// returnCFTypeRef


/*!
Returns the HIObjectRef that this class instance is
storing (and has retained), or nullptr if the internal
reference is empty.

(1.1)
*/
HIObjectRef
CFRetainRelease::
returnHIObjectRef ()
const
{
	return _typeAs._constant.humanInterfaceObject;
}// returnHIObjectRef


/*!
Returns the PasteboardRef that this class instance is
storing (and has retained), or nullptr if the internal
reference is empty.

(2.1)
*/
PasteboardRef
CFRetainRelease::
returnPasteboardRef ()
const
{
	assert(_isMutable);
	return _typeAs._modifiable.pasteboard;
}// returnPasteboardRef


/*!
Use this instead of setCFTypeRef() when the embedded
type is a mutable Core Foundation array.  This is
necessary because CFTypeRef is a constant.

(1.1)
*/
void
CFRetainRelease::
setCFMutableArrayRef	(CFMutableArrayRef		inNewType,
						 bool					inIsAlreadyRetained)
{
	if (nullptr != _typeAs._constant.unspecified) CFRelease(_typeAs._constant.unspecified);
	_typeAs._modifiable.array = inNewType;
	_isMutable = true;
	if ((nullptr != _typeAs._constant.unspecified) &&
		(false == inIsAlreadyRetained))
	{
		CFRetain(_typeAs._constant.unspecified);
	}
}// setCFMutableArrayRef


/*!
Use this instead of setCFTypeRef() when the embedded
type is a mutable Core Foundation dictionary.  This
is necessary because CFTypeRef is a constant.

(1.1)
*/
void
CFRetainRelease::
setCFMutableDictionaryRef	(CFMutableDictionaryRef		inNewType,
							 bool						inIsAlreadyRetained)
{
	if (nullptr != _typeAs._constant.unspecified) CFRelease(_typeAs._constant.unspecified);
	_typeAs._modifiable.dictionary = inNewType;
	_isMutable = true;
	if ((nullptr != _typeAs._constant.unspecified) &&
		(false == inIsAlreadyRetained))
	{
		CFRetain(_typeAs._constant.unspecified);
	}
}// setCFMutableDictionaryRef


/*!
Use this instead of setCFTypeRef() when the embedded
type is a mutable Core Foundation string.  This is
necessary because CFTypeRef is a constant.

(2.0)
*/
void
CFRetainRelease::
setCFMutableStringRef	(CFMutableStringRef		inNewType,
						 bool					inIsAlreadyRetained)
{
	if (nullptr != _typeAs._constant.unspecified) CFRelease(_typeAs._constant.unspecified);
	_typeAs._modifiable.string = inNewType;
	_isMutable = true;
	if ((nullptr != _typeAs._constant.unspecified) &&
		(false == inIsAlreadyRetained))
	{
		CFRetain(_typeAs._constant.unspecified);
	}
}// setCFMutableStringRef


/*!
Calls CFRelease() on the reference kept by this
class instance, if any, and replaces it with the
given reference.  CFRetain() is then called on
the new reference, if the reference is not nullptr.

(1.1)
*/
void
CFRetainRelease::
setCFTypeRef	(CFTypeRef		inNewType,
				 bool			inIsAlreadyRetained)
{
	if (nullptr != _typeAs._constant.unspecified) CFRelease(_typeAs._constant.unspecified);
	_typeAs._constant.unspecified = inNewType;
	if ((nullptr != _typeAs._constant.unspecified) &&
		(false == inIsAlreadyRetained))
	{
		CFRetain(_typeAs._constant.unspecified);
	}
}// setCFTypeRef

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
