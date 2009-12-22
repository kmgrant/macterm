/*###############################################################

	MemoryBlocks.cp
	
	Utility library for more-controlled memory management.
	Consistent use of this module has many benefits, including
	more grace if the program runs out of memory, more ability
	to gauge memory requirements, and increased stability.
	
	Data Access Library 1.3
	© 1998-2009 by Kevin Grant
	
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

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>



#pragma mark Public Methods

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
Destroys a QuickDraw region allocated with Memory_NewRegion().
On output, the region handle is automatically set to nullptr
to prevent you from accidentally using it once it is destroyed.

(1.0)
*/
void
Memory_DisposeRegion	(RgnHandle*		inoutRegionPtr)
{
	if (nullptr != inoutRegionPtr)
	{
		DisposeRgn(*inoutRegionPtr);
		*inoutRegionPtr = nullptr;
	}
}// DisposeRegion


/*!
Allocates a new Mac OS Memory Manager memory block and
returns a handle to it.  If there is not enough contiguous
free space in the heap zone to successfully perform the
allocation, a nullptr handle is returned.

If "inIsCritical" is true, emergency memory space is not
considered when making the request, meaning that it is
then possible for memory to become dangerously low in
order for this request to complete successfully.

IMPORTANT:	You should not normally use Memory_NewHandle();
			instead, use Memory_NewHandleInProperZone(),
			which lets you specify additionally how your
			handle will be used.  Based on whether the new
			handle is going to be needed for a long time or
			a short time, Memory_NewHandleInProperZone()
			automatically calls ReserveMem() or MoveHHi()
			appropriately.

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
returns a handle to it, automatically calling either
MoveHHi() or ReserveMem(), as appropriate, based on
whether you need the handle for a short period of time.
If there is not enough contiguous free space in the
heap zone to successfully perform the allocation, a
nullptr handle is returned.

If you say you are using the handle for a long time, it
is AUTOMATICALLY LOCKED, and should not be unlocked
unless you have a good reason!

In general, you should call this routine for all new
dynamic memory allocations.  This helps to maintain as
much contiguous free space as possible for as long as
possible, reducing the likelihood that future memory
requests will fail.

(3.0)
*/
Handle
Memory_NewHandleInProperZone	(Size					inDesiredNumberOfBytes,
								 MemoryBlockLifetime	inBlockLifeExpectancy,
								 Boolean				inIsCritical)
{
	Handle		result = nullptr;
	
	
	// handles sticking around for a long time should have space reserved for them ahead of time
	unless (kMemoryBlockLifetimeShort == inBlockLifeExpectancy) ReserveMem(inDesiredNumberOfBytes);
	
	// allocate space
	result = Memory_NewHandle(inDesiredNumberOfBytes, inIsCritical);
	if (IsHandleValid(result))
	{
		// handles sticking around for a short period of time should be high in the heap zone;
		// conversely, those that don’t should be locked throughout their entire existence
		if (kMemoryBlockLifetimeShort == inBlockLifeExpectancy) MoveHHi(result);
		else HLock(result);
	}
	
	return result;
}// NewHandleInProperZone


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
	Ptr		result = nullptr;
	
	
	// NOTE: For performance, on Mac OS X calloc() should really be used instead.
	//       When linking against Mach-O, this code should be changed.
	result = NewPtrClear(inDesiredNumberOfBytes);
	
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
	void*		result = nullptr;
	
	
	result = ::malloc(STATIC_CAST(inDesiredNumberOfBytes, size_t));
	return result;
}// NewPtrInterruptSafe


/*!
Allocates a new QuickDraw region that can be used
with any of the standard QuickDraw routines that
manipulate regions.  Use Memory_DisposeRegion()
to destroy this region when you’re done.

There are certain conditions under which it may be
impossible to detect failed region allocations:
specifically, if QDError() returns "noErr" and
memory conditions are extremely low, a region may
in fact be garbage but not “seem like it”.  This
routine performs a sanity check on new regions to
make sure there really is enough space in the grow
zone to accommodate the region that QuickDraw
claims to have created, and returns nullptr if an
otherwise-“valid” region is accidentally allocated.

(1.0)
*/
RgnHandle
Memory_NewRegion ()
{
	RgnHandle		result = nullptr;
	Size			growSize = 0;
	Size			freeSize = 0;
	
	
	freeSize = MaxMem(&growSize); // CARBON NOTE: Calls to MaxMem() are almost certainly not needed on Mac OS X.
	if (noErr == MemError())
	{
		result = NewRgn();
	}
	return result;
}// NewRegion


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
	Size		originalSizeInBytes = GetHandleSize(inoutHandle);
	OSStatus	result = noErr;
	
	
	// first attempt; set the handle size, which may take some time depending
	// on how much memory needs to be moved to make the resize possible; this
	// may fail for a variety of reasons
	SetHandleSize(inoutHandle, inNewHandleSizeInBytes);
	result = MemError();
	
	// now check for failures; some can perhaps be eliminated with extra effort
	// and a 2nd attempt to set the handle size
	if (memFullErr == result)
	{
		// move the Handle high in the heap, eliminate bubbles (compact memory)
		// and try again; the Mac OS Memory Manager doesn’t do this itself for
		// some reason
		MoveHHi(inoutHandle);
		CompactMem(maxSize);
		SetHandleSize(inoutHandle, inNewHandleSizeInBytes);
		result = MemError();
	}
	return result;
}// SetHandleSize

// BELOW IS REQUIRED NEWLINE TO END FILE
