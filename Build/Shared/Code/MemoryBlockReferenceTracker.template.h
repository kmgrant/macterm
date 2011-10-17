/*!	\file MemoryBlockReferenceTracker.template.h
	\brief Keeps track of every address that is considered
	“valid”.  Usually coupled with instances of the
	Registrar class, which will automatically add and
	remove addresses at construction and destruction time
	respectively.
*/
/*###############################################################

	Data Access Library 2.6
	© 1998-2011 by Kevin Grant
	
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
