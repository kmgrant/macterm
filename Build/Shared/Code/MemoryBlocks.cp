/*!	\file MemoryBlocks.cp
	\brief Memory management routines.
*/
/*###############################################################

	Data Access Library
	© 1998-2015 by Kevin Grant
	
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

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlockHandleLocker.template.h>



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

(1.0)
*/
RgnHandle
Memory_NewRegion ()
{
	RgnHandle		result = NewRgn();
	
	
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
	OSStatus	result = noErr;
	
	
	// first attempt; set the handle size, which may take some time depending
	// on how much memory needs to be moved to make the resize possible; this
	// may fail for a variety of reasons
	SetHandleSize(inoutHandle, inNewHandleSizeInBytes);
	result = MemError();
	
	return result;
}// SetHandleSize

// BELOW IS REQUIRED NEWLINE TO END FILE
