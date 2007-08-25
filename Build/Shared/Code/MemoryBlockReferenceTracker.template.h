/*!	\file MemoryBlockReferenceTracker.template.h
	\brief Keeps track of every address that is considered
	“valid”.  Usually coupled with instances of the
	Registrar class, which will automatically add and
	remove addresses at construction and destruction time
	respectively.
*/
/*###############################################################

	Data Access Library 1.3
	© 1998-2007 by Kevin Grant
	
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

#ifndef __MEMORYBLOCKREFERENCETRACKER__
#define __MEMORYBLOCKREFERENCETRACKER__

// pseudo-standard-C++ includes
#if __MWERKS__
#   include <hash_set>
#   define hash_set_namespace Metrowerks
#elif (__GNUC__ > 3)
#   include <ext/hash_set>
#   define hash_set_namespace __gnu_cxx
#elif (__GNUC__ == 3)
#   include <ext/hash_set>
#   define hash_set_namespace __gnu_cxx
#elif (__GNUC__ < 3)
#   include <hash_set>
#   define hash_set_namespace
#else
#   include <hash_set>
#   define hash_set_namespace
#endif



#pragma mark Types

/*!
Hash function for pointer data types.
*/
template < typename structure_reference_type >
class _AddrToLongHasher
{
public:
	size_t
	operator ()	(structure_reference_type) const;
};

/*!
Stores a set of references (pointers) to a data structure.
Useful for checking that a reference is “valid” before it is
used.
*/
template < typename structure_reference_type >
class MemoryBlockReferenceTracker:
public hash_set_namespace::hash_set< structure_reference_type,
										_AddrToLongHasher< structure_reference_type > >
{
};

/*!
Automatically adds the specified reference to a set when
constructed, and removes it when destructed.

Typically, you make one of these the first data member of
an internal class whose pointer is tracked by an opaque
reference type.  This way, when the class is constructed
its reference is added to the list and when it is destroyed
it is removed.

Another application is to declare one of these within the
code of a constructor or destructor, attached to a different
list of references that track “unstable” pointers.  This is
useful for debugging if code is called using a pointer to a
data structure that is technically partially defined due to
construction or destruction.

NOTE:	This class is actually quite general and should
		probably be moved elsewhere.
*/
template < typename address_type, typename set_type >
class Registrar
{
public:
	Registrar	(address_type, set_type&);
	~Registrar	();

private:
	// copying is not allowed
	Registrar	(Registrar< address_type, set_type > const&);
	
	// reassignment is not allowed
	Registrar< address_type, set_type >&
	operator =	(Registrar< address_type, set_type > const&);
	
	address_type	_ref;
	set_type&		_registry;
};



#pragma mark Public Methods

template < typename address_type, typename set_type >
Registrar< address_type, set_type >::
Registrar	(address_type	inRef,
			 set_type&		inoutRegistry)
:
_ref(inRef),
_registry(inoutRegistry)
{
	_registry.insert(_ref);
}// Registrar default constructor


template < typename address_type, typename set_type >
Registrar< address_type, set_type >::
~Registrar ()
{
	_registry.erase(_ref);
}// Registrar destructor



#pragma mark Internal Methods

template < typename structure_reference_type >
size_t
_AddrToLongHasher< structure_reference_type >::
operator ()	(structure_reference_type	inAddress)
const
{
	return REINTERPRET_CAST(inAddress, size_t);
}// operator ()

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
