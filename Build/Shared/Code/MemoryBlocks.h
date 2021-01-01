/*!	\file MemoryBlocks.h
	\brief Memory management routines.
	
	This is largely legacy code.
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

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

/*!
See the Memory_WeakRef...() APIs.
*/
typedef struct MemoryBlocks_WeakPair*	MemoryBlocks_WeakPairRef;



#pragma mark Public Methods

//!\name Module Tests
//@{

void
	Memory_RunTests						();

//@}

//!\name Weak References
//@{

// IMPORTANT: TARGET OBJECT MUST CALL Memory_EraseWeakReferences() IN ITS DESTRUCTOR (OR USE Memory_WeakRefEraser)
MemoryBlocks_WeakPairRef
	Memory_NewWeakPair					(void*							inSourceRef,
										 void*							inTargetRef);

void
	Memory_ReleaseWeakPair				(MemoryBlocks_WeakPairRef*		inoutRefPtr);

// SEE ALSO CLASS Memory_WeakRefEraser
void
	Memory_EraseWeakReferences			(void*							inRefForObjectToBeDestroyed);

void*
	Memory_WeakPairReturnSourceRef		(MemoryBlocks_WeakPairRef		inRef);

void*
	Memory_WeakPairReturnTargetRef		(MemoryBlocks_WeakPairRef		inRef);

void
	Memory_WeakPairSetTargetRef			(MemoryBlocks_WeakPairRef		inRef,
										 void*							inNewTarget);

//@}



#pragma mark Types Dependent on Method Names

/*!
For convenience; ensures that any weak references involving
an object will automatically be cleared when the object is
destroyed.  Declare a field of this type in the corresponding
C++ class.  (Typically the public reference type’s value is
the same as the "this" pointer of the object that has a field
of this type so you construct with "this".)
*/
struct Memory_WeakRefEraser
{
	Memory_WeakRefEraser	(void*		inObject)
	: _object(inObject)
	{
	}
	
	~Memory_WeakRefEraser ()
	{
		Memory_EraseWeakReferences(_object);
	}
	
	void*	_object;
};


/*!
Allows RAII-based automatic retain and release of weak references.

Typically if an object of type A needs to hold a weak reference to
another object of type B, the class for A will contain a field of
type MemoryBlocks_WeakPairWrap to manage the reference, and the
class of type B will contain a field of type Memory_WeakRefEraser
to ensure that ALL weak references involving that class are cleared
after the object is destroyed.  This ensures that whenever the
MemoryBlocks_WeakPairWrap object is used to query the reference,
"nullptr" will be returned if an object has been deallocated.
*/
template < typename source_ref_type, typename target_ref_type >
struct MemoryBlocks_WeakPairWrap
{
public:
	//! automatically allocates a pairing from the specified
	//! source object, initially targeting nothing
	MemoryBlocks_WeakPairWrap	(source_ref_type	inSourceRef)
	: _pairingRef(Memory_NewWeakPair(inSourceRef, nullptr))
	{
	}
	
	//! frees the pairing
	~MemoryBlocks_WeakPairWrap ()
	{
		Memory_ReleaseWeakPair(&_pairingRef);
	}
	
	//! explicitly assigns a new target reference value; since
	//! this is a weak reference, there is no concept of
	//! “releasing” the previous reference (instead, the target
	//! object must ensure that the weak reference is released
	//! when it is invalidated or destroyed, such as by holding
	//! a field of type Memory_WeakRefEraser)
	void
	assign	(target_ref_type	inNewTarget)
	{
		Memory_WeakPairSetTargetRef(_pairingRef, inNewTarget);
	}
	
	//! returns one side of the weak-reference pairing
	target_ref_type
	returnTargetRef () const
	{
		return REINTERPRET_CAST(Memory_WeakPairReturnTargetRef(_pairingRef), target_ref_type);
	}

private:
	//! copy is not allowed (must be able to ensure that
	//! the source object is consistent with the pairing)
	MemoryBlocks_WeakPairWrap	(MemoryBlocks_WeakPairWrap const&	inCopy) = delete;
	
	//! assignment is not allowed (must be able to ensure that
	//! the source object is consistent with the pairing)
	MemoryBlocks_WeakPairWrap&
	operator =(MemoryBlocks_WeakPairWrap const&		inCopy) = delete;
	
	MemoryBlocks_WeakPairRef	_pairingRef;
};

// BELOW IS REQUIRED NEWLINE TO END FILE
