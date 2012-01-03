/*!	\file Embedding.h
	\brief Greatly simplified use of offscreen graphics worlds,
	with particular attention to (human interface) control
	embedding hierarchies.
	
	The Embedding module allows you to use offscreen graphics
	worlds easily, automatically taking into account multiple
	monitors and automatically using the best technology
	available to perform the graphics swapping.  You can perform
	offscreen operations on a *per-control* basis, which allows
	you to draw entire windows (using a root control) or parts
	of a window (via a user pane) via offscreen graphics worlds.
	
	There are also miscellaneous other utility routines that
	make use of offscreen graphics worlds to function.
*/
/*###############################################################

	Interface Library 2.4
	© 1998-2012 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
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

#ifndef __EMBEDDING__
#define __EMBEDDING__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Callbacks

/*!
Offscreen Operation Method

An offscreen operation method should perform all
necessary changes to controls in a window.  The
Embedding_OffscreenControlOperation() routine
will always try to save the entire specified
control for a window in its buffer, so there is
no need for an offscreen operation method to draw
a control unless it is going to change its
appearance in some way.  You generally write
offscreen operation methods that perform a number
of collective tasks on controls, such as setting
the enabled state of more than one control.  Both
pieces of data provided to this method are
arbitrary, and specified when the method
Embedding_OffscreenControlOperation() is called.

Do not call DeviceLoop() in your routine; this
method is designed to provide you with all of
the information that DeviceLoop() would normally
provide.
*/
typedef Boolean (*Embedding_OffscreenOpProcPtr)	(ControlRef			inSpecificControlOrRoot,
												 SInt16				inColorDepth,
												 SInt16				inDeviceFlags,
												 GDHandle			inTargetDevice,
												 SInt32				inData1,
												 SInt32				inData2);
inline Boolean
Embedding_InvokeOffscreenOpProc	(Embedding_OffscreenOpProcPtr	inUserRoutine,
								 ControlRef						inControl,
								 SInt16							inColorDepth,
								 SInt16							inDeviceFlags,
								 GDHandle						inTargetDevice,
								 SInt32							inData1,
								 SInt32							inData2)
{
	return (*inUserRoutine)(inControl, inColorDepth, inDeviceFlags, inTargetDevice, inData1, inData2);
}



#pragma mark Public Methods

//!\name Composited Image Utilities
//@{

OSStatus
	Embedding_BuildCGImageFromPictureAndMask	(PicHandle						inPicture,
												 PicHandle						inMask,
												 CGImageRef*					outCGImagePtr);

//@}

//!\name Working With Compositing Mode Windows
//@{

Boolean
	Embedding_WindowUsesCompositingMode			(WindowRef						inWindow);

//@}

//!\name Offscreen Embedded-View Drawing Routines
//@{

OSStatus
	Embedding_OffscreenControlOperation			(WindowRef						inForWhichWindow,
												 ControlRef						inForWhichControlOrNullForRoot,
												 Embedding_OffscreenOpProcPtr	inUPP,
												 SInt32							inData1,
												 SInt32							inData2);

OSStatus
	Embedding_OffscreenControlOperationInMode	(WindowRef						inForWhichWindow,
												 ControlRef						inForWhichControlOrNullForRoot,
												 Embedding_OffscreenOpProcPtr	inUPP,
												 SInt32							inData1,
												 SInt32							inData2,
												 SInt32							inDrawingMode);

OSStatus
	Embedding_OffscreenSwapOverlappingControls	(WindowRef						inForWhichWindow,
												 ControlRef						inControlToHide,
												 ControlRef						inControlToDisplay);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
