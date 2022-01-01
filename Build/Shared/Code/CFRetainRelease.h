/*!	\file CFRetainRelease.h
	\brief Convenient wrapper class for a Core Foundation object
	(such as a CFStringRef, CFArrayRef, etc.).
	
	See also "RetainRelease.template.h", which generalizes the
	retain/release mechanisms at the cost of not allowing more
	than one reference type per object.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>



#pragma mark Types

/*!
Use instead of a regular Core Foundation reference in
order to have the reference automatically retained with
CFRetain() when constructed, assigned or copied, and
released with CFRelease() when it goes out of scope or
is assigned, etc.

Unlike the RetainRelease template, CFRetainRelease can
be changed at any time to store any Core Foundation type,
which makes it useful for things like containers and for
cases that require both mutable and constant references.
It is more dynamic however, meaning that certain mistakes
can only be found as runtime assertions (RetainRelease
templates can catch some errors at compile time).

The set of explicitly-handled types is arbitrary, and
can be extended as needed for convenience.  Note that it
is always possible to store a Core Foundation reference
of any kind into CFRetainRelease by using the constructor
that accepts CFTypeRef; and, by calling returnCFTypeRef()
and casting, the value can be retrieved.

It is possible to have a nullptr value, and no CFRetain()
or CFRelease() occurs in this case.  It is therefore safe
to initialize to nullptr and later assign a value that
should be retained and released, etc.
*/
class CFRetainRelease
{
public:
	enum ReferenceMutability
	{
		kReferenceConstant,		//!< reference can only be returned as a constant type
		kReferenceMutable		//!< reference can be returned as either constant or mutable (e.g.
								//!  returnCFStringRef() and returnCFMutableStringRef() both work
								//!  if the reference was initialized from a mutable string); note
								//!  that mutability cannot be “detected” from a reference value
								//!  so it is stated explicitly where appropriate
	};
	
	enum ReferenceState
	{
		kNotYetRetained,			//!< retain before storing, and release when done
		kAlreadyRetained			//!< no retain, release when done (e.g. newly-allocated data)
	};
	
	inline
	CFRetainRelease ();
	
	inline
	CFRetainRelease (CFRetainRelease const&);
	
	explicit inline
	CFRetainRelease (CFArrayRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFBundleRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFDataRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFDictionaryRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFMutableArrayRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFMutableDataRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFMutableDictionaryRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFMutableSetRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFMutableStringRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFReadStreamRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFSetRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFStringRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFTypeRef, ReferenceState); // main constructor
	
	explicit inline
	CFRetainRelease (CFURLRef, ReferenceState);
	
	explicit inline
	CFRetainRelease (CFWriteStreamRef, ReferenceState);
	
	virtual inline
	~CFRetainRelease ();
	
	// IMPORTANT: Calls to setWithRetain() or setWithNoRetain() are recommended.
	// This assignment operator only exists to satisfy STL container implementations
	// and other generic constructs that could not know about specific class methods.
	// Since an assignment operator cannot give “already retained, release only”
	// behavior, it assumes that every assigned reference must be retained.
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
	
	inline CFDataRef
	returnCFDataRef () const;
	
	inline CFDictionaryRef
	returnCFDictionaryRef () const;
	
	inline CFMutableArrayRef
	returnCFMutableArrayRef () const;
	
	inline CFMutableDataRef
	returnCFMutableDataRef () const;
	
	inline CFMutableDictionaryRef
	returnCFMutableDictionaryRef () const;
	
	inline CFMutableSetRef
	returnCFMutableSetRef () const;
	
	inline CFMutableStringRef
	returnCFMutableStringRef () const;
	
	inline CFReadStreamRef
	returnCFReadStreamRef () const;
	
	inline CFSetRef
	returnCFSetRef () const;
	
	inline CFStringRef
	returnCFStringRef () const;
	
	inline CFTypeRef
	returnCFTypeRef () const;
	
	inline CFURLRef
	returnCFURLRef () const;
	
	inline CFWriteStreamRef
	returnCFWriteStreamRef () const;
	
	static inline void
	safeRelease	(CFTypeRef);
	
	static inline void
	safeRetain	(CFTypeRef);
	
	template < typename arg_type >
	inline void
	setMutableWithNoRetain (arg_type);
	
	template < typename arg_type >
	inline void
	setMutableWithRetain (arg_type);
	
	template < typename arg_type >
	inline void
	setWithNoRetain (arg_type);
	
	template < typename arg_type >
	inline void
	setWithRetain (arg_type);

protected:
	inline void
	storeReference (void*, ReferenceState, ReferenceMutability); // see also setWithNoRetain() and setWithRetain()

private:
	void*					_reference;		//!< any type that supports CFRetain()/CFRelease() (or Objective-C bridge)
	ReferenceMutability		_mutability;		//!< determines use of unions above
};



#pragma mark Public Methods

/*!
Creates a nullptr reference.

(1.1)
*/
CFRetainRelease::
CFRetainRelease ()
:
_reference(nullptr),
_mutability(kReferenceConstant)
{
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
_reference(inCopy._reference),
_mutability(inCopy._mutability)
{
	safeRetain(_reference);
}// copy constructor


/*!
Creates a new reference using the value of an
existing one that is a generic Core Foundation
type.

CFRetain() is called on the reference unless
"inIsAlreadyRetained" is kAlreadyRetained.
Regardless, CFRelease() is called at destruction
or reassignment time.  This allows "inType" to
come directly from a function call that creates
a Core Foundation type.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFTypeRef		inType,
					 ReferenceState	inIsAlreadyRetained)
:
_reference(CONST_CAST(inType, void*)),
_mutability(kReferenceConstant)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// unspecified type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Array
type.  In debug mode, an assertion failure will
occur if the given reference is not a CFArrayRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFArrayRef		inType,
					 ReferenceState	inIsAlreadyRetained)
:
_reference(CONST_CAST(REINTERPRET_CAST(inType, void const*), void*)),
_mutability(kReferenceConstant)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// array type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Bundle
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFBundleRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFBundleRef	inType,
					 ReferenceState	inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// bundle type constructor


/*!
Creates a new reference using the value of an existing
one that is a Core Foundation Data type.  In debug mode,
an assertion failure will occur if the given reference
is not a CFDataRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2017.05)
*/
CFRetainRelease::
CFRetainRelease		(CFDataRef			inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(CONST_CAST(REINTERPRET_CAST(inType, void const*), void*)),
_mutability(kReferenceConstant)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// data type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Dictionary
type.  In debug mode, an assertion failure will
occur if the given reference is not a CFDictionaryRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFDictionaryRef	inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(CONST_CAST(REINTERPRET_CAST(inType, void const*), void*)),
_mutability(kReferenceConstant)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// dictionary type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Set type.
In debug mode, an assertion failure will occur
if the given reference is not a CFSetRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.7)
*/
CFRetainRelease::
CFRetainRelease		(CFSetRef			inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(CONST_CAST(REINTERPRET_CAST(inType, void const*), void*)),
_mutability(kReferenceConstant)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// set type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Mutable Array
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFMutableArrayRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableArrayRef		inType,
					 ReferenceState			inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// mutable array type constructor


/*!
Creates a new reference using the value of an existing
one that is a Core Foundation Mutable Data type.  In
debug mode, an assertion failure will occur if the
given reference is not a CFMutableDataRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2017.05)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableDataRef	inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// mutable data type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Mutable Dictionary
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFMutableDictionaryRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableDictionaryRef		inType,
					 ReferenceState				inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// mutable dictionary type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Mutable Set
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFMutableSetRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.7)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableSetRef	inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// mutable set type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Mutable String
type.  In debug mode, an assertion failure will occur
if the given reference is not a CFMutableStringRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFMutableStringRef		inType,
					 ReferenceState			inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// mutable string type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Read Stream
type.  In debug mode, an assertion failure will
occur if the given reference is not a
CFReadStreamRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2016.09)
*/
CFRetainRelease::
CFRetainRelease		(CFReadStreamRef		inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// read-stream type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation String
type.  In debug mode, an assertion failure will
occur if the given reference is not a CFStringRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2.0)
*/
CFRetainRelease::
CFRetainRelease		(CFStringRef	inType,
					 ReferenceState	inIsAlreadyRetained)
:
_reference(CONST_CAST(REINTERPRET_CAST(inType, void const*), void*)),
_mutability(kReferenceConstant)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// string type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation URL type.
In debug mode, an assertion failure will occur
if the given reference is not a CFURLRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2016.09)
*/
CFRetainRelease::
CFRetainRelease		(CFURLRef			inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(CONST_CAST(REINTERPRET_CAST(inType, void const*), void*)),
_mutability(kReferenceConstant)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// URL type constructor


/*!
Creates a new reference using the value of an
existing one that is a Core Foundation Write Stream
type.  In debug mode, an assertion failure will
occur if the given reference is not a
CFReadStreamRef.

CFRetain() is conditionally called on the reference
based on "inIsAlreadyRetained"; for details, see
comments for main constructor.

(2016.09)
*/
CFRetainRelease::
CFRetainRelease		(CFWriteStreamRef	inType,
					 ReferenceState		inIsAlreadyRetained)
:
_reference(inType),
_mutability(kReferenceMutable)
{
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(_reference);
	}
}// write-stream type constructor


/*!
Calls CFRelease() on the reference kept by this
class instance, if any.

(1.1)
*/
CFRetainRelease::
~CFRetainRelease ()
{
	safeRelease(_reference);
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
		storeReference(inCopy._reference, kNotYetRetained, inCopy._mutability);
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
	safeRelease(_reference);
	_reference = nullptr;
	_mutability = kReferenceConstant;
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
	return (nullptr != returnCFTypeRef());
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
	return (kReferenceMutable == _mutability);
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
	if (this->isMutable())
	{
		return returnCFMutableArrayRef();
	}
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFArrayGetTypeID()));
	return REINTERPRET_CAST(_reference, CFArrayRef);
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
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFBundleGetTypeID()));
	return REINTERPRET_CAST(_reference, CFBundleRef);
}// returnCFBundleRef


/*!
Convenience method to cast the internal reference
into a CFDataRef.  In debug mode, an assertion
failure will occur if the reference is not really
a CFDataRef or CFMutableDataRef.

(2017.05)
*/
CFDataRef
CFRetainRelease::
returnCFDataRef ()
const
{
	if (this->isMutable())
	{
		return returnCFMutableDataRef();
	}
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFDataGetTypeID()));
	return REINTERPRET_CAST(_reference, CFDataRef);
}// returnCFDataRef


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
	if (this->isMutable())
	{
		return returnCFMutableDictionaryRef();
	}
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFDictionaryGetTypeID()));
	return REINTERPRET_CAST(_reference, CFDictionaryRef);
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
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFArrayGetTypeID()));
	return REINTERPRET_CAST(_reference, CFMutableArrayRef);
}// returnCFMutableArrayRef


/*!
Convenience method to cast the internal reference
into a CFMutableDataRef.  In debug mode, an
assertion failure will occur if the reference is
not really a CFMutableDataRef.

(2017.05)
*/
CFMutableDataRef
CFRetainRelease::
returnCFMutableDataRef ()
const
{
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFDataGetTypeID()));
	return REINTERPRET_CAST(_reference, CFMutableDataRef);
}// returnCFMutableDataRef


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
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFDictionaryGetTypeID()));
	return REINTERPRET_CAST(_reference, CFMutableDictionaryRef);
}// returnCFMutableDictionaryRef


/*!
Convenience method to cast the internal reference
into a CFMutableSetRef.  In debug mode, an
assertion failure will occur if the reference is
not really a CFMutableSetRef.

(2.7)
*/
CFMutableSetRef
CFRetainRelease::
returnCFMutableSetRef ()
const
{
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFSetGetTypeID()));
	return REINTERPRET_CAST(_reference, CFMutableSetRef);
}// returnCFMutableSetRef


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
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFStringGetTypeID()));
	return REINTERPRET_CAST(_reference, CFMutableStringRef);
}// returnCFMutableStringRef


/*!
Returns the CFReadStreamRef that this class instance is
storing (and has retained), or nullptr if the internal
reference is empty.

(2016.09)
*/
CFReadStreamRef
CFRetainRelease::
returnCFReadStreamRef ()
const
{
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFReadStreamGetTypeID()));
	return REINTERPRET_CAST(_reference, CFReadStreamRef);
}// returnCFReadStreamRef


/*!
Convenience method to cast the internal reference
into a CFSetRef.  In debug mode, an assertion
failure will occur if the reference is not really
a CFSetRef or CFMutableSetRef.

(2.7)
*/
CFSetRef
CFRetainRelease::
returnCFSetRef ()
const
{
	if (this->isMutable())
	{
		return returnCFMutableSetRef();
	}
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFSetGetTypeID()));
	return REINTERPRET_CAST(_reference, CFSetRef);
}// returnCFSetRef


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
	if (this->isMutable())
	{
		return returnCFMutableStringRef();
	}
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFStringGetTypeID()));
	return REINTERPRET_CAST(_reference, CFStringRef);
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
	return _reference;
}// returnCFTypeRef


/*!
Returns the CFURLRef that this class instance is
storing (and has retained), or nullptr if the internal
reference is empty.

(2016.09)
*/
CFURLRef
CFRetainRelease::
returnCFURLRef ()
const
{
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFURLGetTypeID()));
	return REINTERPRET_CAST(_reference, CFURLRef);
}// returnCFURLRef


/*!
Returns the CFWriteStreamRef that this class instance is
storing (and has retained), or nullptr if the internal
reference is empty.

(2016.10)
*/
CFWriteStreamRef
CFRetainRelease::
returnCFWriteStreamRef ()
const
{
	assert(this->isMutable());
	assert((nullptr == _reference) || (CFGetTypeID(_reference) == CFWriteStreamGetTypeID()));
	return REINTERPRET_CAST(_reference, CFWriteStreamRef);
}// returnCFWriteStreamRef


/*!
Calls CFRelease() only if the given reference is not nullptr.

(2016.10)
*/
void
CFRetainRelease::
safeRelease		(CFTypeRef	inReferenceOrNull)
{
	if (nullptr != inReferenceOrNull)
	{
		CFRelease(inReferenceOrNull);
	}
}// safeRelease


/*!
Calls CFRetain() only if the given reference is not nullptr.

(2016.10)
*/
void
CFRetainRelease::
safeRetain	(CFTypeRef	inReferenceOrNull)
{
	if (nullptr != inReferenceOrNull)
	{
		CFRetain(inReferenceOrNull);
	}
}// safeRetain


/*!
Equivalent to constructing the object with "kAlreadyRetained"
for a mutable type such as CFMutableStringRef.

IMPORTANT:	Use this variant for any mutable type, otherwise
			an assertion will fail when future attempts are
			made to use the type in a mutable way, such as by
			calling returnCFMutableStringRef().

(2016.10)
*/
template < typename arg_type >
void
CFRetainRelease::
setMutableWithNoRetain		(arg_type	inNewType)
{
	storeReference(inNewType, kAlreadyRetained, kReferenceMutable);
}// setMutableWithNoRetain


/*!
Equivalent to constructing the object with "kNotYetRetained"
for a mutable type such as CFMutableStringRef.

IMPORTANT:	Use this variant for any mutable type, otherwise
			an assertion will fail when future attempts are
			made to use the type in a mutable way, such as by
			calling returnCFMutableStringRef().

(2016.10)
*/
template < typename arg_type >
void
CFRetainRelease::
setMutableWithRetain		(arg_type	inNewType)
{
	storeReference(inNewType, kNotYetRetained, kReferenceMutable);
}// setMutableWithRetain


/*!
Equivalent to constructing the object with "kAlreadyRetained".
Note that this cannot be used to remember mutable state; see
setMutableWithNoRetain() and setMutableWithRetain().

(2016.10)
*/
template < typename arg_type >
void
CFRetainRelease::
setWithNoRetain		(arg_type	inNewType)
{
	storeReference(CONST_CAST(REINTERPRET_CAST(inNewType, void const*), void*),
					kAlreadyRetained, kReferenceConstant);
}// setWithNoRetain


/*!
Equivalent to constructing the object with "kNotYetRetained".
Note that this cannot be used to remember mutable state; see
setMutableWithNoRetain() and setMutableWithRetain().

(2016.10)
*/
template < typename arg_type >
void
CFRetainRelease::
setWithRetain		(arg_type	inNewType)
{
	storeReference(CONST_CAST(REINTERPRET_CAST(inNewType, void const*), void*),
					kNotYetRetained, kReferenceConstant);
}// setWithRetain


/*!
A helper for calls to setWithRetain() or setWithNoRetain()
and similar methods.

Not recommended for constructors, since this will take extra
steps and constructors can initialize fields directly.

Calls CFRelease() on the reference kept by this class instance,
if any, and replaces it with the given reference.  CFRetain()
is then called on the new reference, if the reference is not
nullptr.

(2016.10)
*/
void
CFRetainRelease::
storeReference	(void*					inNewType,
				 ReferenceState			inIsAlreadyRetained,
				 ReferenceMutability		inMutability)
{
	clear();
	_reference = inNewType;
	_mutability = inMutability;
	if (kNotYetRetained == inIsAlreadyRetained)
	{
		safeRetain(inNewType);
	}
}// storeReference

// BELOW IS REQUIRED NEWLINE TO END FILE
