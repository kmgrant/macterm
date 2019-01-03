/*!	\file MemoryBlocks.cp
	\brief Memory management routines.
*/
/*###############################################################

	Data Access Library
	© 1998-2019 by Kevin Grant
	
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

#include <MemoryBlocks.h>
#include <UniversalDefines.h>

// standard-C++ includes
#include <map>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlockPtrLocker.template.h>



#pragma mark Types
namespace {

/*!
Stores the pointers that belong to a pairing.  It is
these pairings that represent the official presence
of weak pointers; maps are just a backup mechanism
to allow pairings to be found.
*/
struct My_WeakPair
{
	void*	sourceRef_ = nullptr;
	void*	targetRef_ = nullptr;
};

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean		unitTest000_Begin ();

} // anonymous namespace

#pragma mark Variables
namespace {

std::multimap< void*, MemoryBlocks_WeakPairRef >&	gWeakPairsBySource()	{ static std::multimap< void*, MemoryBlocks_WeakPairRef > _; return _; }
std::multimap< void*, MemoryBlocks_WeakPairRef >&	gWeakPairsByTarget()	{ static std::multimap< void*, MemoryBlocks_WeakPairRef > _; return _; }

} // anonymous namespace



#pragma mark Public Methods

/*!
A unit test for this module.  This should always
be run before a release, after any substantial
changes are made, or if you suspect bugs!  It
should also be EXPANDED as new functionality is
proposed (ideally, a test is written before the
functionality is added).

(2016.03)
*/
void
Memory_RunTests ()
{
	UInt16		totalTests = 0;
	UInt16		failedTests = 0;
	
	
	++totalTests; if (false == unitTest000_Begin()) ++failedTests;
	
	Console_WriteUnitTestReport("MemoryBlocks", failedTests, totalTests);
}// RunTests


/*!
Destroys the memory allocated by a Mac OS Memory Manager
handle, and then sets the handle to nullptr.  Since one
frequently compares a handle with the value nullptr to see
if it’s valid, it makes sense to ensure that a handle is
nullptr when it actually gets disposed.  Favor this method
over a call to the Mac OS Memory Manager routine
DisposeHandle().

(1.0)
*/
void
Memory_DisposeHandle	(Handle*	inoutHandlePtr)
{
	if (nullptr != inoutHandlePtr)
	{
		DisposeHandle(*inoutHandlePtr);
		*inoutHandlePtr = nullptr;
	}
}// DisposeHandle


/*!
Destroys the memory allocated by a Mac OS Memory Manager
pointer, and then sets the pointer to nullptr.  Since one
frequently compares a pointer with the value nullptr to see
if it’s valid, it makes sense to ensure that a pointer is
nullptr when it actually gets disposed.  Favor this method
over a call to the Mac OS Memory Manager routine
DisposePtr().

(1.0)
*/
void
Memory_DisposePtr	(Ptr*	inoutPtrPtr)
{
	if (nullptr != inoutPtrPtr)
	{
		DisposePtr(*inoutPtrPtr);
		*inoutPtrPtr = nullptr;
	}
}// DisposePtr


/*!
Frees a memory block previously allocated with the
Memory_NewPtrInterruptSafe() routine.  This routine
uses an interrupt-safe allocator.

(1.0)
*/
void
Memory_DisposePtrInterruptSafe	(void**		inoutPtrPtr)
{
	if (nullptr != inoutPtrPtr)
	{
		::free(*inoutPtrPtr);
		*inoutPtrPtr = nullptr;
	}
}// DisposePtrInterruptSafe


/*!
Allocates a new Mac OS Memory Manager memory block and
returns a handle to it.  If there is not enough contiguous
free space in the heap zone to successfully perform the
allocation, a nullptr handle is returned.

If "inIsCritical" is true, emergency memory space is not
considered when making the request, meaning that it is
then possible for memory to become dangerously low in
order for this request to complete successfully.

(1.0)
*/
Handle
Memory_NewHandle	(Size		inDesiredNumberOfBytes,
					 Boolean	UNUSED_ARGUMENT(inIsCritical))
{
	Handle		result = NewHandleClear(inDesiredNumberOfBytes);
	
	
	return result;
}// NewHandle


/*!
Allocates a new Mac OS Memory Manager memory block and
returns a pointer to it.  If there is not enough contiguous
free space in the heap zone to successfully perform the
allocation, nullptr is returned.

(1.0)
*/
Ptr
Memory_NewPtr	(Size	inDesiredNumberOfBytes)
{
	// NOTE: For performance, on Mac OS X calloc() should really be used instead.
	//       When linking against Mach-O, this code should be changed.
	Ptr		result = NewPtrClear(inDesiredNumberOfBytes);
	
	
	return result;
}// NewPtr


/*!
Allocates a new memory block and returns a pointer to it.
This routine uses an interrupt-safe allocator.

(1.0)
*/
void*
Memory_NewPtrInterruptSafe		(Size		inDesiredNumberOfBytes)
{
	void*		result = ::malloc(STATIC_CAST(inDesiredNumberOfBytes, size_t));
	
	
	return result;
}// NewPtrInterruptSafe


/*!
Constructs an object to represent a weak pairing between
two other objects.  Both objects should keep a copy of the
resulting reference, and invoke Memory_ReleaseWeakPair()
when that object is destroyed.

Whichever side releases its pairing first will nullify its
entry in the pairing object.  That way, when the other side
goes looking for its dependency, it will find "nullptr".

Effectively, a weak-pairing has a reference count of 2 so
it will be disposed of when both the source and target
release their references to the pairing.

IMPORTANT:	The source object should be the ONLY holder of
			a MemoryBlocks_WeakPairRef, to ensure that
			object lifetimes are easy to track.  If you
			must have access from other places, consider
			having the 2nd location retain the source
			object and expose an API on the source that
			returns the weak reference’s current value.

IMPORTANT:	This is a type-unchecked primitive function.
			The type-checked "MemoryBlocks_WeakPairWrap"
			template (that also has RAII support) is the
			strongly-recommended way to manage weak pairs.

(2016.03)
*/
MemoryBlocks_WeakPairRef
Memory_NewWeakPair	(void*		inSourceRef,
					 void*		inTargetRef)
{
	My_WeakPair*				ptr = new My_WeakPair();
	MemoryBlocks_WeakPairRef	result = REINTERPRET_CAST(ptr, MemoryBlocks_WeakPairRef);
	
	
	// store the references in the pairing (this allows the right
	// reference to be nullified later, if the target is destroyed
	// first)
	ptr->sourceRef_ = inSourceRef;
	
	// allow for reasonably-quick access to the pairing from the
	// source side
	gWeakPairsBySource().insert(std::make_pair(inSourceRef, result));
	
	Memory_WeakPairSetTargetRef(result, inTargetRef);
	
	return result;
}// NewWeakPair


/*!
Releases a weak-pairing.

This REALLY should only be done by the act of destroying
the source object in the pairing.  Using the helper class
"MemoryBlocks_WeakPairWrap" is highly recommended (also,
that class is type-protected by templates).

(2016.03)
*/
void
Memory_ReleaseWeakPair	(MemoryBlocks_WeakPairRef*		inoutRefPtr)
{
	Boolean			destroyPair = false;
	My_WeakPair*	asPtr = REINTERPRET_CAST(*inoutRefPtr, My_WeakPair*);
	auto			foundRange = gWeakPairsBySource().equal_range(asPtr->sourceRef_);
	
	
	// WARNING: retain/release counts are NOT implemented; a pairing
	// is expected to (only) belong to its source object, and that
	// object cannot implement assign/copy of such an object without
	// retain/release capability (INCOMPLETE but unnecessary?)
	destroyPair = true;
	
	// remove reference to this pair that may exist from the source;
	// it is possible for the same source to be mapped to multiple
	// pairings so find the exact reference (if it exists)
	for (auto iter = foundRange.first; iter != foundRange.second; ++iter)
	{
		if (*inoutRefPtr == (*iter).second)
		{
			//Console_WriteValueAddress("found and removed reference to pairing", asPtr); // debug
			//Console_WriteValueAddress("from source", asPtr->sourceRef_); // debug
			// drop the source-side reference to the pairing
			gWeakPairsBySource().erase(iter);
			break;
		}
	}
	
	// remove reference to this pair that may exist from the target;
	// it is possible for the same target to be mapped to multiple
	// pairings so find the exact reference (if it exists)
	foundRange = gWeakPairsByTarget().equal_range(asPtr->targetRef_);
	for (auto iter = foundRange.first; iter != foundRange.second; ++iter)
	{
		if (*inoutRefPtr == (*iter).second)
		{
			//Console_WriteValueAddress("found and removed reference to pairing", asPtr); // debug
			//Console_WriteValueAddress("from target", asPtr->targetRef_); // debug
			// drop the target-side reference to the pairing
			gWeakPairsByTarget().erase(iter);
			break;
		}
	}
	
	if (destroyPair)
	{
		delete asPtr, asPtr = nullptr;
	}
	
	*inoutRefPtr = nullptr;
}// ReleaseWeakPair


/*!
Searches for weak-reference pairs that point to the given
reference, and sets the targets of those pairs to nullptr.

This is meant to be used by target objects that otherwise
have no knowledge of weak references.  Generally, objects
that are valid targets of weak references should contain
a field of type "Memory_WeakRefEraser" (which will call
this method automatically at destruction time, preferably
before any other fields are torn down).  It can also make
sense to erase weak references prior to actual destruction
of an object, especially for objects that support the
concept of an invalid state.

See also MemoryBlocks_WeakPairWrap for the source-side
reference.

NOTE:	This algorithm is not optimized; it may need
		tuning if many weak references are created.

(2016.03)
*/
void
Memory_EraseWeakReferences		(void*		inRefForObjectToBeDestroyed)
{
	// nullify references to this object that may exist from a source
	// (TEMPORARY; this is not optimized for the case of a ridiculous
	// number of weak references; the assumption is that there will
	// not be a lot of weak references around...)
	auto	foundRange = gWeakPairsByTarget().equal_range(inRefForObjectToBeDestroyed);
	for (auto iter = foundRange.first; iter != foundRange.second; ++iter)
	{
		My_WeakPair*	asPtr = REINTERPRET_CAST((*iter).second, My_WeakPair*);
		
		
		// nullify reference but do not destroy the pairing
		// (it is still held by the source)
		//Console_WriteValueAddress("found and nullified target reference in pairing", asPtr); // debug
		//Console_WriteValueAddress("nullified reference to target", inRefForObjectToBeDestroyed); // debug
		asPtr->targetRef_ = nullptr;
	}
	
	// delete all entries for the target however (the pairing
	// objects will therefore only be reachable from their
	// source objects)
	gWeakPairsByTarget().erase(foundRange.first, foundRange.second);
}// EraseWeakReferences


/*!
Resizes a Mac OS Memory Manager memory block to which
you have a handle.  If there is not enough contiguous
free space in the heap zone to successfully perform the
resize operation, "memFullErr" is returned; otherwise,
"noErr" is returned.

Note that a locked handle may fail to be resizable
because of its current location in the heap.  If at all
possible, unlock a Handle before passing it to this
routine.

(1.0)
*/
OSStatus
Memory_SetHandleSize	(Handle		inoutHandle,
						 Size		inNewHandleSizeInBytes)
{
	OSStatus	result = noErr;
	
	
	// first attempt; set the handle size, which may take some time depending
	// on how much memory needs to be moved to make the resize possible; this
	// may fail for a variety of reasons
	SetHandleSize(inoutHandle, inNewHandleSizeInBytes);
	result = MemError();
	
	return result;
}// SetHandleSize


/*!
Returns one of the references of a weak pairing: the
first one given to Memory_NewWeakPair().

It is up to you to keep track of the types of objects
given to a pairing when it is constructed, to know
what kind of object is returned here!

(2016.03)
*/
void*
Memory_WeakPairReturnSourceRef	(MemoryBlocks_WeakPairRef	inRef)
{
	My_WeakPair*	asPtr = REINTERPRET_CAST(inRef, My_WeakPair*);
	
	
	return asPtr->sourceRef_;
}// WeakPairReturnSourceRef


/*!
Returns one of the references of a weak pairing: the
first one given to Memory_NewWeakPair().

It is up to you to keep track of the types of objects
given to a pairing when it is constructed, to know
what kind of object is returned here!

(2016.03)
*/
void*
Memory_WeakPairReturnTargetRef	(MemoryBlocks_WeakPairRef	inRef)
{
	My_WeakPair*	asPtr = REINTERPRET_CAST(inRef, My_WeakPair*);
	
	
	return asPtr->targetRef_;
}// WeakPairReturnTargetRef


/*!
Changes the target object, overriding anything given at
construction time.  To see this value later, call
Memory_WeakPairReturnTargetRef().

(2016.03)
*/
void
Memory_WeakPairSetTargetRef		(MemoryBlocks_WeakPairRef	inRef,
								 void*						inNewTarget)
{
	My_WeakPair*	asPtr = REINTERPRET_CAST(inRef, My_WeakPair*);
	auto			foundRange = gWeakPairsByTarget().equal_range(asPtr->targetRef_);
	
	
	// remove reference to this pair that may exist from the old target
	for (auto iter = foundRange.first; iter != foundRange.second; ++iter)
	{
		if (asPtr->targetRef_ == (*iter).second)
		{
			//Console_WriteValueAddress("found and removed reference to pairing", asPtr); // debug
			//Console_WriteValueAddress("from target", asPtr->targetRef_); // debug
			// drop the target-side reference to the pairing
			gWeakPairsByTarget().erase(iter);
			break;
		}
	}
	
	// allow for reasonably-quick access to the pairing from the
	// target side (except for initial null references, which are
	// pointless to track)
	if (nullptr != inNewTarget)
	{
		gWeakPairsByTarget().insert(std::make_pair(inNewTarget, inRef));
	}
	
	// also store the target in the pairing
	asPtr->targetRef_ = inNewTarget;
}// WeakPairSetTargetRef


#pragma mark Internal Methods: Unit Tests
namespace {

/*!
Tests various weak-pair APIs and utility classes.

Returns "true" if ALL assertions pass; "false" is
returned if any fail, however messages should be
printed for ALL assertion failures regardless.

(2016.03)
*/
Boolean
unitTest000_Begin ()
{
	Boolean						result = true;
	MemoryBlocks_WeakPairRef	weakPairRef1 = nullptr;
	MemoryBlocks_WeakPairRef	weakPairRef2 = nullptr;
	MemoryBlocks_WeakPairRef	weakPairRef3 = nullptr;
	void*						testSource1 = (void*)(0xabcddcba); // objects don’t have to be real; testing reference pairings only
	void*						testSource2 = (void*)(0x0abcdef0);
	void*						testSource3 = (void*)(0xabc00cba);
	void*						testTarget1 = (void*)(0x12345678);
	void*						testTarget3 = (void*)(0x01234567);
	
	
	// test allocation of weak references
	result &= Console_Assert("target 1 not recognized", gWeakPairsByTarget().find(testTarget1) == gWeakPairsByTarget().end());
	result &= Console_Assert("target 3 not recognized", gWeakPairsByTarget().find(testTarget3) == gWeakPairsByTarget().end());
	weakPairRef1 = Memory_NewWeakPair(testSource1, nullptr);
	result &= Console_Assert("source 1 OK", testSource1 == Memory_WeakPairReturnSourceRef(weakPairRef1));
	result &= Console_Assert("nullptr target OK", nullptr == Memory_WeakPairReturnTargetRef(weakPairRef1));
	result &= Console_Assert("source stored", gWeakPairsBySource().find(testSource1) != gWeakPairsBySource().end());
	result &= Console_Assert("nullptr not stored", gWeakPairsByTarget().find(nullptr) == gWeakPairsByTarget().end());
	
	// test change of target
	Memory_WeakPairSetTargetRef(weakPairRef1, testTarget1);
	result &= Console_Assert("reference target recognized, internal", gWeakPairsByTarget().find(testTarget1) != gWeakPairsByTarget().end());
	result &= Console_Assert("reference target recognized, API", testTarget1 == Memory_WeakPairReturnTargetRef(weakPairRef1));
	
	// test initialization with target as well as multiple
	// weak references to the same target
	weakPairRef2 = Memory_NewWeakPair(testSource2, nullptr);
	result &= Console_Assert("source stored", gWeakPairsBySource().find(testSource2) != gWeakPairsBySource().end());
	result &= Console_Assert("nullptr not stored", gWeakPairsByTarget().find(nullptr) == gWeakPairsByTarget().end());
	Memory_WeakPairSetTargetRef(weakPairRef2, testTarget1);
	result &= Console_Assert("target recognized from second pairing, API", testTarget1 == Memory_WeakPairReturnTargetRef(weakPairRef2));
	{
		auto	foundRange = gWeakPairsByTarget().equal_range(testTarget1);
		result &= Console_Assert("two sources for target now exist", 2 == std::distance(foundRange.first, foundRange.second));
	}
	
	// test initialization with second source
	weakPairRef3 = Memory_NewWeakPair(testSource3, testTarget3);
	result &= Console_Assert("target recognized from third pairing, API", testTarget3 == Memory_WeakPairReturnTargetRef(weakPairRef3));
	{
		auto	foundRange = gWeakPairsByTarget().equal_range(testTarget3);
		result &= Console_Assert("one source for target now exists", 1 == std::distance(foundRange.first, foundRange.second));
	}
	
	// test auto-clear of all weak references upon scope exit
	{
		Memory_WeakRefEraser	autoEraser(testTarget1);
	}
	result &= Console_Assert("target 1 cleared, internal", gWeakPairsByTarget().find(testTarget1) == gWeakPairsByTarget().end());
	result &= Console_Assert("target 3 not cleared, internal", gWeakPairsByTarget().find(testTarget3) != gWeakPairsByTarget().end());
	result &= Console_Assert("source 1 unaffected", gWeakPairsBySource().find(testSource1) != gWeakPairsBySource().end());
	result &= Console_Assert("source 2 unaffected", gWeakPairsBySource().find(testSource2) != gWeakPairsBySource().end());
	result &= Console_Assert("source 3 unaffected", gWeakPairsBySource().find(testSource3) != gWeakPairsBySource().end());
	result &= Console_Assert("pair 1 target nullified, API", nullptr == Memory_WeakPairReturnTargetRef(weakPairRef1));
	result &= Console_Assert("pair 2 target nullified, API", nullptr == Memory_WeakPairReturnTargetRef(weakPairRef2)); // same target aa #1
	result &= Console_Assert("pair 3 target not nullified, API", nullptr != Memory_WeakPairReturnTargetRef(weakPairRef3));
	
	// test teardown of weak references
	Memory_ReleaseWeakPair(&weakPairRef1);
	result &= Console_Assert("reference 1 nullified", nullptr == weakPairRef1);
	result &= Console_Assert("source 1 cleared, internal", gWeakPairsBySource().find(testSource1) == gWeakPairsBySource().end());
	result &= Console_Assert("source 2 not yet cleared, internal", gWeakPairsBySource().find(testSource2) != gWeakPairsBySource().end());
	Memory_ReleaseWeakPair(&weakPairRef2);
	result &= Console_Assert("reference 2 nullified", nullptr == weakPairRef2);
	{
		void*	const	kSource = Memory_WeakPairReturnSourceRef(weakPairRef3);
		void*	const	kTarget = Memory_WeakPairReturnTargetRef(weakPairRef3);
		
		
		Memory_ReleaseWeakPair(&weakPairRef3);
		result &= Console_Assert("reference 3 nullified", nullptr == weakPairRef3);
		result &= Console_Assert("pair 3 source cleared, internal", gWeakPairsBySource().find(kSource) == gWeakPairsBySource().end());
		result &= Console_Assert("pair 3 target cleared, internal", gWeakPairsByTarget().find(kTarget) == gWeakPairsByTarget().end());
	}
	
	return result;
}// unitTest000_Begin

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
