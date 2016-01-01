/*!	\file MemoryBlockPtrLocker.template.h
	\brief A refinement of MemoryBlockLocker that works when
	the underlying memory block is located with a simple pointer.
*/
/*###############################################################

	Data Access Library
	© 1998-2016 by Kevin Grant
	
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

#ifndef __MEMORYBLOCKPTRLOCKER__
#define __MEMORYBLOCKPTRLOCKER__

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
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
	typedef void (*DisposeProcPtr)(structure_type*);
	
	//! create a locker that optionally calls a dispose routine when lock count returns to zero
	MemoryBlockPtrLocker	(DisposeProcPtr = nullptr);
	
	//! reinterprets the reference as a pointer; nullptr is allowed
	structure_type*
	acquireLock	(structure_reference_type	inReference) override;
	
	//! for basic pointer locks, an unlock simply means “set my copy to nullptr so I don’t use it again”
	void
	releaseLock	(structure_reference_type	inReference,
				 structure_type**			inoutPtrPtr) override;
	
	//! test routine
	static Boolean
	unitTest ();

protected:

private:
	DisposeProcPtr		_disposer;
	bool				_requireLocks;	//!< in the destruct phase ONLY, this is cleared to
										//!  prevent callbacks from looping back; it is
										//!  implicit in that phase that the structure is
										//!  locked (and is in the process of being destroyed)
};

struct MemoryBlockPtrLocker_TestClass
{
	int		x;
};
typedef struct MemoryBlockPtrLocker_TestClass*		MemoryBlockPtrLocker_TestClassRef;



#pragma mark Public Methods

/*!
A unit test for this module.  This should always
be run before a release, after any substantial
changes are made, or if you suspect bugs!  It
should also be EXPANDED as new functionality is
proposed (ideally, a test is written before the
functionality is added).

(4.0)
*/
inline void
MemoryBlockPtrLocker_RunTests ()
{
	UInt16		totalTests = 0;
	UInt16		failedTests = 0;
	
	
	++totalTests;
	if (false == MemoryBlockPtrLocker<MemoryBlockPtrLocker_TestClassRef, MemoryBlockPtrLocker_TestClass>::unitTest())
	{
		++failedTests;
	}
	
	Console_WriteUnitTestReport("Memory Block Ptr Locker", failedTests, totalTests);
}// RunTests


template < typename structure_reference_type, typename structure_type >
MemoryBlockPtrLocker< structure_reference_type, structure_type >::
MemoryBlockPtrLocker	(DisposeProcPtr		inDisposer)
:
_disposer(inDisposer),
_requireLocks(true)
{
}// MemoryBlockPtrLocker 1-argument constructor


template < typename structure_reference_type, typename structure_type >
structure_type*
MemoryBlockPtrLocker< structure_reference_type, structure_type >::
acquireLock	(structure_reference_type	inReference)
{
	structure_type*		result = nullptr;
	
	
	result = REINTERPRET_CAST(inReference, structure_type*);
	if (_requireLocks)
	{
		UInt16	newLockCount = 0;
	#ifndef NDEBUG
		UInt16	oldLockCount = this->returnLockCount(inReference);
	#endif
		
		
		newLockCount = this->incrementLockCount(inReference);
		assert(newLockCount > oldLockCount);
	}
	return result;
}// acquireLock


template < typename structure_reference_type, typename structure_type >
void
MemoryBlockPtrLocker< structure_reference_type, structure_type >::
releaseLock	(structure_reference_type	inReference,
			 structure_type**			inoutPtrPtr)
{
	if (_requireLocks)
	{
		UInt16	newLockCount = 0;
	#ifndef NDEBUG
		UInt16	oldLockCount = this->returnLockCount(inReference);
	#endif
		
		
		assert(oldLockCount > 0);
		newLockCount = this->decrementLockCount(inReference);
		assert(newLockCount < oldLockCount);
		if ((0 == newLockCount) && (nullptr != _disposer))
		{
			_requireLocks = false;
			(*_disposer)(*inoutPtrPtr);
		}
		if (inoutPtrPtr != nullptr) *inoutPtrPtr = nullptr;
	}
}// releaseLock


/*!
Tests an instance of this template class.  Returns true only
if successful.  Information on failures is printed to the
console.

(4.0)
*/
template < typename structure_reference_type, typename structure_type >
Boolean
MemoryBlockPtrLocker< structure_reference_type, structure_type >::
unitTest ()
{
	typedef LockAcquireRelease< structure_reference_type, structure_type >		TestAutoLockerClass;
	typedef MemoryBlockPtrLocker< structure_reference_type, structure_type >	TestLockerClass;
	Boolean		result = true;
	
	
	// basic locking
	{
		TestLockerClass				locker;
		structure_reference_type	ref1 = REINTERPRET_CAST(0x1234DEAD, structure_reference_type);
		structure_reference_type	ref2 = REINTERPRET_CAST(0x5678BEEF, structure_reference_type);
		structure_type*				ptr1 = nullptr;
		structure_type*				ptr2 = nullptr;
		
		
		result &= Console_Assert("initial lock count of zero for ref1", !locker.isLocked(ref1));
		result &= Console_Assert("initial lock count of zero for ref2", !locker.isLocked(ref2));
		ptr1 = locker.acquireLock(ref1);
		result &= Console_Assert("lock count increases for ref1", locker.isLocked(ref1));
		result &= Console_Assert("lock count is up to one for ref1", 1 == locker.returnLockCount(ref1));
		ptr2 = locker.acquireLock(ref2);
		locker.releaseLock(ref1, &ptr1);
		result &= Console_Assert("ptr1 is nullified", nullptr == ptr1);
		result &= Console_Assert("ptr2 is not nullified", nullptr != ptr2);
		result &= Console_Assert("lock count decreases for ref1", false == locker.isLocked(ref1));
		result &= Console_Assert("lock count is down to zero for ref1", 0 == locker.returnLockCount(ref1));
		result &= Console_Assert("lock count is up to one for ref2", 1 == locker.returnLockCount(ref2));
		ptr1 = locker.acquireLock(ref2);
		result &= Console_Assert("lock count is up to two for ref2", 2 == locker.returnLockCount(ref2));
		locker.releaseLock(ref2, &ptr2);
		result &= Console_Assert("ptr2 is nullified", nullptr == ptr2);
		result &= Console_Assert("lock count is down to one for ref2", 1 == locker.returnLockCount(ref2));
		locker.releaseLock(ref2, &ptr2);
		result &= Console_Assert("lock count is down to zero for ref2", 0 == locker.returnLockCount(ref2));
	}
	
	// automatic locking
	{
		TestLockerClass				locker;
		structure_reference_type	ref1 = REINTERPRET_CAST(0x1234DEAD, structure_reference_type);
		structure_reference_type	ref2 = REINTERPRET_CAST(0x5678BEEF, structure_reference_type);
		
		
		result &= Console_Assert("initial lock count of zero for ref1", !locker.isLocked(ref1));
		result &= Console_Assert("initial lock count of zero for ref2", !locker.isLocked(ref2));
		{
			TestAutoLockerClass		ptr1(locker, ref1);
			
			
			result &= Console_Assert("lock count increases for ref1", locker.isLocked(ref1));
			result &= Console_Assert("lock count is up to one for ref1", 1 == locker.returnLockCount(ref1));
		}
		{
			TestAutoLockerClass		ptr2(locker, ref2);
			
			
			result &= Console_Assert("ptr2 is not nullified", nullptr != &*ptr2);
			result &= Console_Assert("lock count decreases for ref1", false == locker.isLocked(ref1));
			result &= Console_Assert("lock count is down to zero for ref1", 0 == locker.returnLockCount(ref1));
			result &= Console_Assert("lock count is up to one for ref2", 1 == locker.returnLockCount(ref2));
			{
				TestAutoLockerClass		alsoPtr2(locker, ref2);
				
				
				result &= Console_Assert("lock count is up to two for ref2", 2 == locker.returnLockCount(ref2));
			}
			result &= Console_Assert("lock count is down to one for ref2", 1 == locker.returnLockCount(ref2));
		}
		result &= Console_Assert("lock count is down to zero for ref2", 0 == locker.returnLockCount(ref2));
	}
	
	return result;
}// unitTest

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
