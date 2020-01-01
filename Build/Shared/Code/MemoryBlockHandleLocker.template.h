/*!	\file MemoryBlockHandleLocker.template.h
	\brief A refinement of MemoryBlockLocker that works when
	the underlying memory block is a Mac OS "Handle".
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
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlockLocker.template.h>



#pragma mark Types

/*!
This class can be used to “safely” acquire and release locks on a
relocatable block, because a request to unlock the handle will not
actually call HUnlock() until all acquisitions have been undone.
If you use this class, you should not lock or unlock the handle on
your own, because that will corrupt the state maintained for the
handle.
*/
template < typename structure_reference_type, typename structure_type, bool debugged = false >
class MemoryBlockHandleLocker:
public MemoryBlockLocker< structure_reference_type, structure_type, debugged >
{
public:
	//! returns the value of the handle’s master pointer, guaranteed to be stable while the handle is locked
	structure_type*
	acquireLock	(structure_reference_type	inReference) override;
	
	//! nullifies a copy of the master pointer value so the caller can no longer use it; might unlock the underlying handle
	void
	releaseLock	(structure_reference_type	inReference,
				 structure_type**			inoutPtrPtr) override;

protected:

private:
};



#pragma mark Public Methods

template < typename structure_reference_type, typename structure_type, bool debugged >
structure_type*
MemoryBlockHandleLocker< structure_reference_type, structure_type, debugged >::
acquireLock	(structure_reference_type	inReference)
{
	structure_type*		result = nullptr;
	UInt16				newLockCount = 0;
#ifndef NDEBUG
	UInt16				oldLockCount = this->returnLockCount(inReference);
#endif
	
	
	HLock(REINTERPRET_CAST(inReference, Handle));
	result = *(REINTERPRET_CAST(inReference, structure_type**));
	newLockCount = this->incrementLockCount(inReference);
	if (debugged)
	{
		// log that a lock was acquired, and show where the lock came from
		this->logLockState("acquired lock", inReference, newLockCount);
	}
	assert(newLockCount > oldLockCount);
	return result;
}// acquireLock


template < typename structure_reference_type, typename structure_type, bool debugged >
void
MemoryBlockHandleLocker< structure_reference_type, structure_type, debugged >::
releaseLock	(structure_reference_type	inReference,
			 structure_type**			inoutPtrPtr)
{
	// BE SURE THIS IMPLEMENTATION IS SYNCHRONIZED WITH THE “CONSTANT” VERSION, ABOVE
	UInt16	newLockCount = 0;
#ifndef NDEBUG
	UInt16	oldLockCount = this->returnLockCount(inReference);
#endif
	
	
	assert(oldLockCount > 0);
	newLockCount = this->decrementLockCount(inReference);
	if (debugged)
	{
		// log that a lock was released, and show where the release came from
		this->logLockState("released lock", inReference, newLockCount);
	}
	assert(newLockCount < oldLockCount);
	if (newLockCount == 0) HUnlock(REINTERPRET_CAST(inReference, Handle));
	if (inoutPtrPtr != nullptr) *inoutPtrPtr = nullptr;
}// releaseLock

// BELOW IS REQUIRED NEWLINE TO END FILE
