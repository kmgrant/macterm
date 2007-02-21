/*!	\file VirtualDevice.h
	\brief Interactive Color Raster Graphics screens.
	
	This module handles virtual device allocation for the
	interactive color raster graphics kernel.  Windows that
	mimic real video devices are used for ICR graphics.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#ifndef __VIRTUALDEVICE__
#define __VIRTUALDEVICE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>



#pragma mark Constants

enum VirtualDevice_ResultCode
{
	kVirtualDevice_ResultCodeSuccess = 0,				//!< no error
	kVirtualDevice_ResultCodeInsufficientMemory = -1,	//!< not enough memory to do everything required
	kVirtualDevice_ResultCodePointerCheck = -2,			//!< a null reference or null pointer was provided
	kVirtualDevice_ResultCodeEmptyBoundaries = -3		//!< empty rectangle; default bounds used, otherwise valid result
};

#pragma mark Types

typedef struct OpaqueVirtualDevice*		VirtualDevice_Ref;



#pragma mark Public Methods

//!\name Creating and Destroying Virtual Devices
//@{

VirtualDevice_ResultCode
	VirtualDevice_New						(VirtualDevice_Ref*		inoutVirtualDeviceRefPtr,
											 Rect const*			inInitialDeviceBounds,
											 PaletteHandle			inInitialPaletteHandle);

void
	VirtualDevice_Dispose					(VirtualDevice_Ref*		inoutVirtualDeviceRefPtr);

//@}

//!\name Manipulating Virtual Devices With QuickDraw
//@{

VirtualDevice_ResultCode
	VirtualDevice_GetBitMapForCopyBits		(VirtualDevice_Ref		inVirtualDeviceRef,
											 BitMap const**			outBitMapPtrPtr);

VirtualDevice_ResultCode
	VirtualDevice_GetBounds					(VirtualDevice_Ref		inVirtualDeviceRef,
											 Rect*					outRectPtr);

VirtualDevice_ResultCode
	VirtualDevice_GetGraphicsWorld			(VirtualDevice_Ref		inVirtualDeviceRef,
											 GWorldPtr*				outGWorldPtrPtr);

VirtualDevice_ResultCode
	VirtualDevice_GetPixelMap				(VirtualDevice_Ref		inVirtualDeviceRef,
											 PixMapHandle*			outPixMapHandle);

VirtualDevice_ResultCode
	VirtualDevice_SetColorTable				(VirtualDevice_Ref		inVirtualDeviceRef,
											 PaletteHandle			inPaletteToUseForColorTable);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
