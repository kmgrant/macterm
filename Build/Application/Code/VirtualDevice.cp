/*###############################################################

	VirtualDevice.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

// library includes
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>

// application includes
#include "VirtualDevice.h"



#pragma mark Constants

enum
{
	PIXEL_DEPTH = 8
};

#pragma mark Types

struct My_VirtualDevice
{
	GWorldPtr		whichWorld;		//!< GDevice created off-screen
	CTabHandle		colorTable;		//!< graphics world’s color table
	Rect			bounds;			//!< boundary rectangle for virtual device
};
typedef My_VirtualDevice*	My_VirtualDevicePtr;

typedef MemoryBlockPtrLocker< VirtualDevice_Ref, My_VirtualDevice >		My_VirtualDevicePtrLocker;
typedef LockAcquireRelease< VirtualDevice_Ref, My_VirtualDevice >		My_VirtualDeviceAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_VirtualDevicePtrLocker&	gVDevicePtrLocks ()		{ static My_VirtualDevicePtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static Boolean	isOK	(VirtualDevice_Result);



#pragma mark Public Methods

/*!
This method creates a new virtual device
with a specific boundary and color table.

Possible return codes:

"kVirtualDevice_ResultInsufficientMemory"
	There is not enough memory to create
	a device.  The output device reference
	is set to nullptr.

"kVirtualDevice_ResultPointerCheck"
	A specified pointer was nullptr.

"kVirtualDevice_ResultEmptyBoundaries"
	Successfully created a new video device,
	but the specified rectangle was empty.
	Therefore, the new device was given a
	default rectangle that may not be the
	desired boundary.

"kVirtualDevice_ResultOK"
	Successfully created a new video device.

(3.0)
*/
VirtualDevice_Result
VirtualDevice_New	(VirtualDevice_Ref*		inoutVirtualDeviceRefPtr,
					 Rect const*			inInitialDeviceBounds,
					 PaletteHandle			inInitialPaletteHandle)
{
	VirtualDevice_Result	result = kVirtualDevice_ResultOK;
	
	
	if (inoutVirtualDeviceRefPtr == nullptr) result = kVirtualDevice_ResultPointerCheck;
	else
	{
		*inoutVirtualDeviceRefPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(My_VirtualDevice)),
														VirtualDevice_Ref);
		if (*inoutVirtualDeviceRefPtr == nullptr)
		{
			result = kVirtualDevice_ResultInsufficientMemory;
		}
		else
		{
			My_VirtualDeviceAutoLocker	ptr(gVDevicePtrLocks(), *inoutVirtualDeviceRefPtr);
			CGrafPtr					oldPort = nullptr;
			GDHandle					oldDevice = nullptr;
			
			
			GetGWorld(&oldPort, &oldDevice); // save old settings
			
			// copy the given rectangle
			SetRect(&ptr->bounds, 0, 0, 0, 0);
			if (inInitialDeviceBounds == nullptr) result = kVirtualDevice_ResultPointerCheck;
			else
			{
				SetRect(&ptr->bounds, inInitialDeviceBounds->left, inInitialDeviceBounds->top,
							inInitialDeviceBounds->right, inInitialDeviceBounds->bottom);
				if (EmptyRect(&ptr->bounds))
				{
					// empty rectangles are probably indicative of greater
					// problems, but allow them anyway...
					result = kVirtualDevice_ResultEmptyBoundaries;
				}
			}
			
			// still fine?
			if (isOK(result))
			{
				// copy "inInitialPaletteHandle" into a new color table handle
				ptr->colorTable = (CTabHandle)Memory_NewHandle(sizeof(ColorTable));
				if (ptr->colorTable != nullptr)
				{
					OSStatus	error = noErr;
					
					
					Palette2CTab(inInitialPaletteHandle, ptr->colorTable);
					
					// initialize the graphics world
					#if 0
					error = NewGWorld(&ptr->whichWorld, PIXEL_DEPTH, &ptr->bounds,
										ptr->colorTable,
										(GDHandle)nullptr/* device */,
										0/* flags */);
					#else
					// debug - try creating a basic (default) world
					error = NewGWorld(&ptr->whichWorld, PIXEL_DEPTH, &ptr->bounds,
										(CTabHandle)nullptr/* colors */,
										(GDHandle)nullptr/* device */,
										0/* flags */);
					#endif
					
					if ((ptr->whichWorld == nullptr) || (error != noErr))
					{
						if (ptr->whichWorld == nullptr)
						{
							DisposeGWorld(ptr->whichWorld), ptr->whichWorld = nullptr;
							Memory_DisposeHandle(REINTERPRET_CAST(&ptr->colorTable, Handle*));
						}
						result = kVirtualDevice_ResultInsufficientMemory;
					}
					
					// still fine?
					if (isOK(result))
					{
						PixMapHandle	pixMap = nullptr;
						Rect			rect;
						
						
						SetGWorld(ptr->whichWorld, nullptr);
						pixMap = GetGWorldPixMap(ptr->whichWorld);
						GetPortBounds(ptr->whichWorld, &rect);
						EraseRect(&rect);
						SetGWorld(oldPort, oldDevice);
					}
				}
				else
				{
					result = kVirtualDevice_ResultInsufficientMemory;
				}
			}
		}
	}
	
	unless (isOK(result))
	{
		// invalidate the returned reference in the event of a serious error
		if (inoutVirtualDeviceRefPtr != nullptr) *inoutVirtualDeviceRefPtr = nullptr;
	}
	
	return result;
}// New


/*!
Disposes of a virtual device previously created
with a call to VirtualDevice_New().

(3.0)
*/
void
VirtualDevice_Dispose	(VirtualDevice_Ref*		inoutVirtualDeviceRefPtr)
{
	if (inoutVirtualDeviceRefPtr != nullptr)
	{
		{
			My_VirtualDeviceAutoLocker	ptr(gVDevicePtrLocks(), *inoutVirtualDeviceRefPtr);
			
			
			if (ptr->colorTable != nullptr)
			{
				Memory_DisposeHandle(REINTERPRET_CAST(&ptr->colorTable, Handle*));
			}
			if (ptr->whichWorld != nullptr) DisposeGWorld(ptr->whichWorld), ptr->whichWorld = nullptr;
		}
		Memory_DisposePtr(REINTERPRET_CAST(inoutVirtualDeviceRefPtr, Ptr*));
	}
}// Dispose


/*!
If you need to use CopyBits() for a virtual device,
use this routine to access the pixel map in terms
of a bitmap.  DON’T use this routine for any other
purpose, since a BitMap is only supported under
Carbon for use with CopyBits().

(3.0)
*/
VirtualDevice_Result
VirtualDevice_GetBitMapForCopyBits	(VirtualDevice_Ref	inVirtualDeviceRef,
									 BitMap const**		outBitMapPtrPtr)
{
	VirtualDevice_Result	result = kVirtualDevice_ResultOK;
	
	
	if (outBitMapPtrPtr == nullptr) result = kVirtualDevice_ResultPointerCheck;
	else
	{
		My_VirtualDeviceAutoLocker	ptr(gVDevicePtrLocks(), inVirtualDeviceRef);
		
		
		if (ptr == nullptr) result = kVirtualDevice_ResultPointerCheck;
		else
		{
			*outBitMapPtrPtr = GetPortBitMapForCopyBits(ptr->whichWorld);
		}
	}
	
	return result;
}// GetBitMapForCopyBits


/*!
Provides the bounding rectangle of a virtual device.
The boundaries of the specified device are copied
into the given rectangle.

(3.0)
*/
VirtualDevice_Result
VirtualDevice_GetBounds		(VirtualDevice_Ref	inVirtualDeviceRef,
							 Rect*				outRectPtr)
{
	VirtualDevice_Result	result = kVirtualDevice_ResultOK;
	
	
	if (outRectPtr == nullptr) result = kVirtualDevice_ResultPointerCheck;
	else
	{
		My_VirtualDeviceAutoLocker	ptr(gVDevicePtrLocks(), inVirtualDeviceRef);
		
		
		if (ptr == nullptr) result = kVirtualDevice_ResultPointerCheck;
		else
		{
			SetRect(outRectPtr,
					ptr->bounds.left, ptr->bounds.top, ptr->bounds.right, ptr->bounds.bottom);
		}
	}
	
	return result;
}// GetBounds


/*!
Provides the graphics world of a virtual device.

(3.0)
*/
VirtualDevice_Result
VirtualDevice_GetGraphicsWorld	(VirtualDevice_Ref	inVirtualDeviceRef,
								 GWorldPtr*			outGWorldPtrPtr)
{
	VirtualDevice_Result	result = kVirtualDevice_ResultOK;
	
	
	if (outGWorldPtrPtr == nullptr) result = kVirtualDevice_ResultPointerCheck;
	else
	{
		My_VirtualDeviceAutoLocker	ptr(gVDevicePtrLocks(), inVirtualDeviceRef);
		
		
		if (ptr == nullptr) result = kVirtualDevice_ResultPointerCheck;
		else
		{
			*outGWorldPtrPtr = ptr->whichWorld;
		}
	}
	
	return result;
}// GetGraphicsWorld


/*!
Provides the pixel map of the graphics world of
a virtual device.

(3.0)
*/
VirtualDevice_Result
VirtualDevice_GetPixelMap	(VirtualDevice_Ref	inVirtualDeviceRef,
							 PixMapHandle*		outPixMapHandle)
{
	VirtualDevice_Result	result = kVirtualDevice_ResultOK;
	
	
	if (outPixMapHandle == nullptr) result = kVirtualDevice_ResultPointerCheck;
	else
	{
		My_VirtualDeviceAutoLocker	ptr(gVDevicePtrLocks(), inVirtualDeviceRef);
		
		
		if (ptr == nullptr) result = kVirtualDevice_ResultPointerCheck;
		else
		{
			*outPixMapHandle = GetPortPixMap(ptr->whichWorld);
		}
	}
	
	return result;
}// GetPixelMap


/*!
Specifies the colors to use for a virtual device.

(3.0)
*/
VirtualDevice_Result
VirtualDevice_SetColorTable		(VirtualDevice_Ref	inVirtualDeviceRef,
								 PaletteHandle		inPaletteToUseForColorTable)
{
	VirtualDevice_Result		result = kVirtualDevice_ResultOK;
	My_VirtualDeviceAutoLocker	ptr(gVDevicePtrLocks(), inVirtualDeviceRef);
	
	
	if (ptr == nullptr) result = kVirtualDevice_ResultPointerCheck;
	else
	{
		PixMapHandle	pixMap = nullptr;
		
		
		result = VirtualDevice_GetPixelMap(inVirtualDeviceRef, &pixMap);
		if (isOK(result))
		{
			CTabHandle		colorTable = nullptr;
			
			
			colorTable = (*pixMap)->pmTable;
			if (colorTable == nullptr) result = kVirtualDevice_ResultInsufficientMemory;
			else
			{
				Palette2CTab(inPaletteToUseForColorTable, colorTable);
				
				(*colorTable)->ctSeed = GetCTSeed();					// give the table a unique seed
				(*colorTable)->ctFlags = STATIC_CAST(0x8000, UInt16);   // high bit of 1 means “from device”
				
				// make a 3-bit inverse table
				#if 0
				{
					MakeITable(colorTable, (ptr->whichWorld->gdITable, 3);
				}
				#endif
			}
		}
	}
	
	return result;
}// SetColorTable


#pragma mark Internal Methods

/*!
Returns "true" only if a particular error
code is serious.  This allows functions to
continue even if a result other than
"kVirtualDevice_ResultOK" occurs.

(3.0)
*/
static Boolean
isOK	(VirtualDevice_Result	inError)
{
	Boolean		result = true;
	
	
	switch (inError)
	{
	case kVirtualDevice_ResultOK:
	case kVirtualDevice_ResultEmptyBoundaries:
		result = false;
		break;
	
	case kVirtualDevice_ResultInsufficientMemory:
	case kVirtualDevice_ResultPointerCheck:
	default:
		// most errors are serious
		result = true;
		break;
	}
	
	return result;
}// isOK

// BELOW IS REQUIRED NEWLINE TO END FILE
