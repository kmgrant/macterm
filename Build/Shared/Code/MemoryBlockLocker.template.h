/*!	\file MemoryBlockLocker.template.h
	\brief Provides a locking mechanism for an opaque reference
	that may really point to a relocatable block of memory.
	
	This can be used to implement opaque reference types for
	objects not meant to be accessed directly in C.  The class
	is abstract, as it does not handle any particular kind of
	memory block; create a subclass to do that.
*/
/*###############################################################

	Data Access Library
	© 1998-2021 by Kevin Grant
	
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

// pseudo-standard-C++ includes
#if __has_include(<tr1/unordered_map>)
#	include <tr1/unordered_map>
#	ifndef unordered_map_namespace
#		define unordered_map_namespace std::tr1
#	endif
#elif __has_include(<unordered_map>)
#	include <unordered_map>
#	ifndef unordered_map_namespace
#		define unordered_map_namespace std
#	endif
#else
#	error "Do not know how to find <unordered_map> with this compiler."
#endif

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <MemoryBlockReferenceTracker.template.h> // for _AddrToLongHasher



#pragma mark Types

/*!
Generic interface defining a locking mechanism for memory blocks.
Whether static or relocatable, these basic functions can be used
to convert from “stable” reference types to potentially mutable
pointer types, invoking all necessary Memory Manager calls.  This
class is a repository containing lock counts for as many references
of the same type as you wish.  To add a reference, simply try to
lock it for the first time with acquireLock().  To remove a
reference, unlock all locks on it.
*/
template < typename structure_reference_type, typename structure_type, bool debugged = false >
class MemoryBlockLocker
{
public:
	//! exists only because subclasses are expected
	virtual
	~MemoryBlockLocker		();
	
	//! stabilizes the specified reference’s mutable memory block and returns a pointer to its stable location
	//! (or "null", on error)
	virtual structure_type*
	acquireLock				(structure_reference_type			inReference) = 0;
	
	//! clears all locks; USE WITH CARE
	inline void
	clear					();
	
	//! determines if there are any locks on the specified reference’s memory block
	inline bool
	isLocked				(structure_reference_type			inReference) const;
	
	//! writes a stack trace and notes the current lock count; this helps with
	//! debugging, to show exactly where locks are added/removed
	void
	logLockState			(char const*						inDescription,
							 structure_reference_type			inReference,
							 UInt16								inProposedLockCount) const;
	
	//! nullifies a pointer to a mutable memory block; once all locks are cleared, the block can be relocated or purged, etc.
	virtual void
	releaseLock				(structure_reference_type			inReference,
							 structure_type**					inoutPtrPtr) = 0;
	
	//! the number of locks acquired without being released (should be 0 if a reference is free)
	UInt16
	returnLockCount			(structure_reference_type			inReference) const;

protected:
	//! decreases the number of locks on a reference, returning the new value
	//! (MUST be used by all releaseLock() implementations)
	UInt16
	decrementLockCount		(structure_reference_type			inReference);
	
	//! increases the number of locks on a reference, returning the new value
	//! (MUST be used by all acquireLock() implementations)
	UInt16
	incrementLockCount		(structure_reference_type			inReference);

private:
	typedef unordered_map_namespace::unordered_map< structure_reference_type, UInt16,
													_AddrToLongHasher< structure_reference_type > >		CountMapType;
	CountMapType	_mapObject;		//!< repository for reference lock count information
};


/*!
A useful wrapper that you could declare in a block so that
a lock is automatically acquired upon entry and released
upon block exit.
*/
template < typename structure_reference_type, typename structure_type, bool debugged = false >
class LockAcquireRelease
{
	typedef MemoryBlockLocker< structure_reference_type, structure_type, debugged >		LockerType;

public:
	//! acquires a lock
	LockAcquireRelease		(LockerType&				inLocker,
							 structure_reference_type	inReference);
	
	//! releases a lock
	virtual
	~LockAcquireRelease		();
	
	//! refers directly to the internal pointer
	inline operator structure_type*		();
	
	//! dereferences the internal pointer
	inline structure_type&
	operator *				();
	
	//! dereferences the internal pointer
	inline structure_type const&
	operator *				() const;
	
	//! refers directly to the internal pointer
	inline structure_type*
	operator ->				();
	
	//! refers directly to the internal pointer
	inline structure_type const*
	operator ->				() const;
	
	//! returns the class instance managing locks (use with care)
	inline LockerType&
	locker					();

protected:

private:
	LockerType&					_locker;	//!< repository for reference lock count information
	structure_reference_type	_ref;		//!< reference to data
	structure_type*				_ptr;		//!< once locked, a direct pointer to the referenced data
};



#pragma mark Public Methods

template < typename structure_reference_type, typename structure_type, bool debugged >
MemoryBlockLocker< structure_reference_type, structure_type, debugged >::
~MemoryBlockLocker ()
{
}// ~MemoryBlockLocker


template < typename structure_reference_type, typename structure_type, bool debugged >
void
MemoryBlockLocker< structure_reference_type, structure_type, debugged >::
clear ()
{
	_mapObject.clear();
}// clear


template < typename structure_reference_type, typename structure_type, bool debugged >
UInt16
MemoryBlockLocker< structure_reference_type, structure_type, debugged >::
decrementLockCount	(structure_reference_type	inReference)
{
	UInt16		result = 0;
	auto		toCount = _mapObject.find(inReference);
	
	
	if (_mapObject.end() != toCount)
	{
		--(toCount->second);
		result = toCount->second;
		if (0 == result)
		{
			// delete the item when the count reaches zero
			_mapObject.erase(toCount);
		}
	}
	return result;
}// decrementLockCount


template < typename structure_reference_type, typename structure_type, bool debugged >
UInt16
MemoryBlockLocker< structure_reference_type, structure_type, debugged >::
incrementLockCount	(structure_reference_type	inReference)
{
	UInt16		result = 0;
	auto		toCount = _mapObject.find(inReference);
	
	
	// add the item if it is not present
	if (_mapObject.end() == toCount)
	{
		_mapObject[inReference] = 0;
		toCount = _mapObject.find(inReference);
	}
	
	assert(_mapObject.end() != toCount);
	
	++(toCount->second);
	result = toCount->second;
	
	return result;
}// incrementLockCount


template < typename structure_reference_type, typename structure_type, bool debugged >
bool
MemoryBlockLocker< structure_reference_type, structure_type, debugged >::
isLocked	(structure_reference_type	inReference)
const
{
	// if any lock count is currently stored in the collection for
	// the given reference, then that reference is considered locked
	return (_mapObject.end() != _mapObject.find(inReference));
}// isLocked


template < typename structure_reference_type, typename structure_type, bool debugged >
void
MemoryBlockLocker< structure_reference_type, structure_type, debugged >::
logLockState	(char const*				inDescription,
				 structure_reference_type	inReference,
				 UInt16						inLockCount)
const
{
	// log that a lock was acquired, and show where the lock came from
	Console_WriteValueAddress(inDescription, inReference);
	Console_WriteValue("new lock count", inLockCount);
	Console_WriteStackTrace();
}// logLockState


template < typename structure_reference_type, typename structure_type, bool debugged >
UInt16
MemoryBlockLocker< structure_reference_type, structure_type, debugged >::
returnLockCount		(structure_reference_type	inReference)
const
{
	UInt16		result = 0;
	auto		toCount = _mapObject.find(inReference);
	
	
	if (_mapObject.end() != toCount)
	{
		result = toCount->second;
	}
	return result;
}// returnLockCount


template < typename structure_reference_type, typename structure_type, bool debugged >
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
LockAcquireRelease	(LockAcquireRelease< structure_reference_type, structure_type, debugged >::LockerType&	inLocker,
							 structure_reference_type														inReference)
:
_locker(inLocker),
_ref(inReference),
_ptr(inLocker.acquireLock(inReference))
{
}// LockAcquireRelease


template < typename structure_reference_type, typename structure_type, bool debugged >
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
~LockAcquireRelease ()
{
	_locker.releaseLock(_ref, &_ptr);
}// ~LockAcquireRelease


template < typename structure_reference_type, typename structure_type, bool debugged >
typename LockAcquireRelease< structure_reference_type, structure_type, debugged >::LockerType&
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
locker ()
{
	return _locker;
}// locker


template < typename structure_reference_type, typename structure_type, bool debugged >
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
operator structure_type* ()
{
	return &(this->operator *());
}// operator structure_type*


template < typename structure_reference_type, typename structure_type, bool debugged >
structure_type&
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
operator * ()
{
	return *_ptr;
}// operator *


template < typename structure_reference_type, typename structure_type, bool debugged >
structure_type const&
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
operator * ()
const
{
	return *_ptr;
}// operator * const


template < typename structure_reference_type, typename structure_type, bool debugged >
structure_type*
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
operator -> ()
{
	return &(this->operator *());
}// operator ->


template < typename structure_reference_type, typename structure_type, bool debugged >
structure_type const*
LockAcquireRelease< structure_reference_type, structure_type, debugged >::
operator -> ()
const
{
	return &(this->operator *());
}// operator -> const

// BELOW IS REQUIRED NEWLINE TO END FILE
