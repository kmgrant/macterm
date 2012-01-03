/*!	\file RetainRelease.template.h
	\brief Convenient template class to call retain and release
	routines that follow a certain convention, via RAII.
	
	Use this to ensure a reference is automatically retained at
	object construction or duplication time, and automatically
	released when the object is destroyed.  Wrappers mean that
	custom constructors and destructors are not required for a
	class if it simply wants to hold onto a reference-counted
	object reference indefinitely.
	
	This class also allows references to be null.  You can make
	an existing object of this type equal to nullptr by calling
	the clear() method, or by assigning to it a temporary
	instance with no arguments, such as
	"myRetainReleaseInstance = RetainRelease();"; or, use
	"myRetainReleaseInstance = RetainRelease(nullptr);" if
	you want to be more explicit (these are equivalent).
	
	Finally, RetainRelease can be release-only: it can be
	initialized by a reference that is already retained (such as
	something returned by a ..._New() function), in which case
	no retain is called but the release is still called.
	
	WARNING:	Objective-C++ breaks RAII when it comes to
				exceptions; destructors are NOT called if the
				scope exits because an NSException* was thrown!
				You may still declare wrapper objects such as
				this within code that calls Objective-C, but
				you should have a "@try" block with a "@finally"
				block that uses clear() on RetainRelease objects.
				This has the advantage of being a conditional
				release; if the object contains a non-nil pointer
				then it will be released, otherwise nothing is
				done.  Note again that this is only necessary in
				code that might call Objective-C and trigger an
				exception; it is not necessary for pure C++.
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

#ifndef __RETAINRELEASE__
#define __RETAINRELEASE__



#pragma mark Types

/*!
Use instead of a regular reference in order to have the
reference automatically retained with the specified retainer
function when constructed, assigned or copied, and released
with the specified release function when it goes out of scope
or is assigned, etc.

It is possible to have a nullptr value, and no retain or
release occurs in this case.  It is therefore safe to initialize
to nullptr and later assign a value that should be retained and
released, etc.

IMPORTANT:	The reference manager type must be struct with two
			static methods, both of which return void and take a
			single argument of a reference type.  That reference
			type should be available as a public typedef in the
			struct, named "reference_type".
*/
template < typename reference_mgr_type >
class RetainRelease
{
public:
	typedef typename reference_mgr_type::reference_type		reference_type;
	
	inline
	RetainRelease ();
	
	inline
	RetainRelease (RetainRelease const&);
	
	inline
	RetainRelease (reference_type, bool = false);
	
	virtual inline
	~RetainRelease ();
	
	// IMPORTANT: It is clearer to call setRef(), so that is recommended.
	// This assignment operator only exists to satisfy STL container implementations
	// and other generic constructs that could not know about specific class methods.
	virtual inline RetainRelease&
	operator = (RetainRelease const&);
	
	virtual inline bool
	operator == (RetainRelease const&);
	
	virtual inline bool
	operator != (RetainRelease const&);
	
	inline void
	clear ();
	
	inline bool
	exists () const;
	
	inline reference_type
	returnRef () const;
	
	inline void
	setRef (reference_type, bool = false);

protected:
	inline void
	release (reference_type) const;
	
	inline void
	retain (reference_type) const;

private:
	reference_type		_ref;
};



#pragma mark Public Methods

/*!
Creates a nullptr reference.

(2.4)
*/
template < typename reference_mgr_type >
RetainRelease<reference_mgr_type>::
RetainRelease ()
:
_ref(nullptr)
{
}// default constructor


/*!
Creates a new reference using the value of an existing one.
The retainer is called on the reference.

(2.4)
*/
template < typename reference_mgr_type >
RetainRelease<reference_mgr_type>::
RetainRelease	(RetainRelease const&	inCopy)
:
_ref(inCopy._ref)
{
	if (nullptr != _ref)
	{
		this->retain(_ref);
	}
}// copy constructor


/*!
Creates a new reference using the value of an existing one.

The retainer is called on the reference unless the parameter
"inIsAlreadyRetained" is true.  Regardless, the releaser is
called at destruction or reassignment time.  This allows
"inRef" to come directly from a function call that creates
an object.

(2.4)
*/
template < typename reference_mgr_type >
RetainRelease<reference_mgr_type>::
RetainRelease	(reference_type		inRef,
				 bool				inIsAlreadyRetained)
:
_ref(inRef)
{
	_ref = inRef;
	if ((nullptr != _ref) && (false == inIsAlreadyRetained))
	{
		this->retain(_ref);
	}
}// reference initializer constructor


/*!
Calls the releaser on the reference kept by this class
instance, if any.

(2.4)
*/
template < typename reference_mgr_type >
RetainRelease<reference_mgr_type>::
~RetainRelease ()
{
	if (nullptr != _ref)
	{
		this->release(_ref), _ref = nullptr;
	}
}// destructor


/*!
Equivalent to using the copy constructor to make
a new instance, but updates this instance.

(2.4)
*/
template < typename reference_mgr_type >
RetainRelease<reference_mgr_type>&
RetainRelease<reference_mgr_type>::
operator =		(RetainRelease const&	inCopy)
{
	if (&inCopy != this)
	{
		setRef(inCopy.returnRef());
	}
	return *this;
}// operator =


/*!
Performs an equality check on a pair of reference objects.
This allows you to embed a RetainRelease object sensibly in
something like an STL container.

NOTE:	Currently, this is a simple check by-value, and no
		equality operation is sought from the reference manager
		type.  If this ever becomes necessary in the future, it
		would make sense to invoke something like a
		

(2.4)
*/
template < typename reference_mgr_type >
bool
RetainRelease<reference_mgr_type>::
operator ==		(RetainRelease const&	inOther)
{
	return (returnRef() == inOther.returnRef());
}// operator ==


/*!
The inverse of operator ==().

(2.4)
*/
template < typename reference_mgr_type >
bool
RetainRelease<reference_mgr_type>::
operator !=		(RetainRelease const&	inOther)
{
	return ! operator == (inOther);
}// operator !=


/*!
Sets this reference to nullptr, calling the releaser (if
necessary) on the previous value.

(2.4)
*/
template < typename reference_mgr_type >
void
RetainRelease<reference_mgr_type>::
clear ()
{
	if (nullptr != _ref)
	{
		this->release(_ref);
	}
	_ref = nullptr;
}// clear


/*!
Returns true if the internal reference is not nullptr.

(2.4)
*/
template < typename reference_mgr_type >
bool
RetainRelease<reference_mgr_type>::
exists ()
const
{
	return (nullptr != _ref);
}// exists


/*!
Invokes the reference manager’s release() method.

(2.4)
*/
template < typename reference_mgr_type >
void
RetainRelease<reference_mgr_type>::
release		(reference_type		inRef)
const
{
	reference_mgr_type::release(inRef);
}// release


/*!
Invokes the reference manager’s retain() method.

(2.4)
*/
template < typename reference_mgr_type >
void
RetainRelease<reference_mgr_type>::
retain		(reference_type		inRef)
const
{
	reference_mgr_type::retain(inRef);
}// retain


/*!
Returns the reference type that this class instance is
storing (and has retained), or nullptr if the internal
reference is empty.

(2.4)
*/
template < typename reference_mgr_type >
typename RetainRelease<reference_mgr_type>::reference_type
RetainRelease<reference_mgr_type>::
returnRef ()
const
{
	return _ref;
}// returnRef


/*!
Calls the releaser on the reference kept by this class
instance, if any, and replaces it with the given reference.
The retainer is then called on the new reference, if the
reference is not nullptr.

(2.4)
*/
template < typename reference_mgr_type >
void
RetainRelease<reference_mgr_type>::
setRef	(reference_type		inNewType,
		 bool				inIsAlreadyRetained)
{
	if (_ref != inNewType)
	{
		if (nullptr != _ref)
		{
			this->release(_ref);
		}
		_ref = inNewType;
		if ((nullptr != _ref) && (false == inIsAlreadyRetained))
		{
			this->retain(_ref);
		}
	}
}// setRef

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
