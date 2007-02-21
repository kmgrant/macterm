/*!	\file MemoryBlockPtrLocker.template.h
	\brief A refinement of MemoryBlockLocker that works when
	the underlying memory block is located with a simple pointer.
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

#ifndef __MEMORYBLOCKPTRLOCKER__
#define __MEMORYBLOCKPTRLOCKER__

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlockLocker.template.h>



#pragma mark Types

/*!
This class can be used to “safely” acquire and release locks on
a static block.  This class is mainly provided so that you can
enforce a locking structure using static blocks and later decide
to use relocatable blocks, or vice-versa, without code changes.
(Even so, it is handy because you’d avoid typecasting opaque
reference types to pointers and vice-versa.)
*/
template < typename structure_reference_type, typename structure_type >
class MemoryBlockPtrLocker:
public MemoryBlockLocker< structure_reference_type, structure_type >
{
public:
	//! reinterprets the reference as a pointer; nullptr is allowed
	structure_type*
	acquireLock	(structure_reference_type	inReference);
	
	//! for basic pointer locks, an unlock simply means “set my copy to nullptr so I don’t use it again”
	void
	releaseLock	(structure_reference_type	inReference,
				 structure_type const**		inoutPtrPtr);
	
	//! for basic pointer locks, an unlock simply means “set my copy to nullptr so I don’t use it again”
	void
	releaseLock	(structure_reference_type	inReference,
				 structure_type**			inoutPtrPtr);

protected:

private:
};



#pragma mark Public Methods

template < typename structure_reference_type, typename structure_type >
structure_type*
MemoryBlockPtrLocker< structure_reference_type, structure_type >::
acquireLock	(structure_reference_type	inReference)
{
	structure_type*		result = nullptr;
	UInt16				newLockCount = 0;
#ifndef NDEBUG
	UInt16				oldLockCount = returnLockCount(inReference);
#endif
	
	
	result = REINTERPRET_CAST(inReference, structure_type*);
	newLockCount = incrementLockCount(inReference);
	assert(newLockCount > oldLockCount);
	return result;
}// acquireLock


template < typename structure_reference_type, typename structure_type >
void
MemoryBlockPtrLocker< structure_reference_type, structure_type >::
releaseLock	(structure_reference_type	inReference,
			 structure_type**			inoutPtrPtr)
{
	// BE SURE THIS IMPLEMENTATION IS SYNCHRONIZED WITH THE “NON-CONSTANT” VERSION, BELOW
	UInt16	newLockCount = 0;
#ifndef NDEBUG
	UInt16	oldLockCount = returnLockCount(inReference);
#endif
	
	
	assert(oldLockCount > 0);
	newLockCount = decrementLockCount(inReference);
	assert(newLockCount < oldLockCount);
	if (inoutPtrPtr != nullptr) *inoutPtrPtr = nullptr;
}// releaseLock


template < typename structure_reference_type, typename structure_type >
void
MemoryBlockPtrLocker< structure_reference_type, structure_type >::
releaseLock	(structure_reference_type	inReference,
			 structure_type const**		inoutPtrPtr)
{
	// BE SURE THIS IMPLEMENTATION IS SYNCHRONIZED WITH THE “CONSTANT” VERSION, ABOVE
	UInt16	newLockCount = 0;
#ifndef NDEBUG
	UInt16	oldLockCount = returnLockCount(inReference);
#endif
	
	
	assert(oldLockCount > 0);
	newLockCount = decrementLockCount(inReference);
	assert(newLockCount < oldLockCount);
	if (inoutPtrPtr != nullptr) *inoutPtrPtr = nullptr;
}// releaseLock

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
