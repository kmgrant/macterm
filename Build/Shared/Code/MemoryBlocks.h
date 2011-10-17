/*!	\file MemoryBlocks.h
	\brief Memory management routines.
	
	This is largely legacy code.
*/
/*###############################################################

	Data Access Library 2.6
	� 1998-2011 by Kevin Grant
	
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

#ifndef __MEMORYBLOCKS__
#define __MEMORYBLOCKS__

// standard-C++ includes
#include <map>

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef Boolean MemoryBlockLifetime;
enum
{
	kMemoryBlockLifetimeShort = true,		// memory block only intended to be used for a short period of time
	kMemoryBlockLifetimeLong = false		// memory block expected to remain allocated indefinitely
};



#pragma mark Public Methods

/*###############################################################
	MEMORY MANAGEMENT SETUP
###############################################################*/

// CALL THIS IN EVERY STATIC INITIALIZER; INTERNALLY IT ENSURES INITIALIZATION IS DONE ONLY ONCE
void
	Memory_Init							();

/*###############################################################
	INDISPENSABLE MEMORY MANAGEMENT ROUTINES
###############################################################*/

Handle
	Memory_NewHandle					(Size					inDesiredNumberOfBytes,
										 Boolean				inIsCritical = false);

Handle
	Memory_NewHandleInProperZone		(Size					inDesiredNumberOfBytes,
										 MemoryBlockLifetime	inBlockLifeExpectancy,
										 Boolean				inIsCritical = false);

void
	Memory_DisposeHandle				(Handle*				inoutHandle);

// USE THIS ROUTINE TO CLARIFY YOUR INTENT WHEN DISPOSING OF A HANDLE
inline void
	Memory_ReleaseDetachedResource		(Handle*				inoutHandlePtr)
	{
		Memory_DisposeHandle(inoutHandlePtr);
	}

Ptr
	Memory_NewPtr						(Size					inDesiredNumberOfBytes);

void*
	Memory_NewPtrInterruptSafe			(Size					inDesiredNumberOfBytes);

void
	Memory_DisposePtr					(Ptr*					inoutPtr);

void
	Memory_DisposePtrInterruptSafe		(void**					inoutPtr);

OSStatus
	Memory_SetHandleSize				(Handle					inoutHandle,
										 Size					inNewHandleSizeInBytes);

/*###############################################################
	QUICKDRAW MEMORY MANAGEMENT UTILITIES
###############################################################*/

RgnHandle
	Memory_NewRegion					();

void
	Memory_DisposeRegion				(RgnHandle*				inoutRegion);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
