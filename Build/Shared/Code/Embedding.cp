/*###############################################################

	Embedding.cp
	
	Interface Library 2.4
	© 1998-2011 by Kevin Grant
	
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
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <QuickTime/QuickTime.h>

// library includes
#include <AlertMessages.h>
#include <ColorUtilities.h>
#include <Cursors.h>
#include <Embedding.h>
#include <MemoryBlocks.h>



#pragma mark Types

struct My_OffscreenDeviceLoopParams
{
	Embedding_OffscreenOpProcPtr	proc;
	WindowRef						window;
	ControlRef						control;
	SInt32							data1;
	SInt32							data2;
	SInt32							dumpMode;
	OSStatus						result;
};
typedef struct My_OffscreenDeviceLoopParams		My_OffscreenDeviceLoopParams;
typedef My_OffscreenDeviceLoopParams*			My_OffscreenDeviceLoopParamsPtr;

#pragma mark Internal Method Prototypes

static void		offscreenDumpDeviceLoop			(short, short, GDHandle, long);
static Boolean	setControlActiveOperation		(ControlRef, SInt16, SInt16, GDHandle, SInt32, SInt32);
static Boolean	swapControlsOperation			(ControlRef, SInt16, SInt16, GDHandle, SInt32, SInt32);



#pragma mark Public Methods

/*!
Creates offscreen graphics worlds for two images,
and then builds a composite image with an alpha
channel that uses the mask image to determine alpha
values.

Use CGImageRelease() on the resultant image when
you are finished using it.

(1.0)
*/
OSStatus
Embedding_BuildCGImageFromPictureAndMask	(PicHandle		inPicture,
											 PicHandle		inMask,
											 CGImageRef*	outCGImagePtr)
{
	OSStatus	result = noErr;
	
	
	if ((inPicture == nullptr) || (inMask == nullptr)) result = memPCErr;
	else
	{
		GWorldPtr	imageGraphicsWorld = nullptr;
		GWorldPtr	maskGraphicsWorld = nullptr;
		Rect		pictureFrame;
		Rect		maskFrame;
		
		
		(Rect*)QDGetPictureBounds(inPicture, &pictureFrame);
		(Rect*)QDGetPictureBounds(inMask, &maskFrame);
		
		// create offscreen graphics worlds in which to draw the image and its mask
		result = NewGWorld(&imageGraphicsWorld, 32/* bits per pixel */, &pictureFrame, nullptr/* color table */, nullptr/* device */,
							0/* flags */);
		if (result == noErr)
		{
			result = NewGWorld(&maskGraphicsWorld, 32/* bits per pixel */, &maskFrame, nullptr/* color table */, nullptr/* device */,
								0/* flags */);
			if (result == noErr)
			{
				// draw the icon and its mask into separate offscreen buffers
				GDHandle	oldDevice = nullptr;
				GrafPtr		oldPort = nullptr;
				
				
				GetGWorld(&oldPort, &oldDevice);
				SetGWorld(imageGraphicsWorld, nullptr);
				DrawPicture(inPicture, &pictureFrame);
				SetGWorld(maskGraphicsWorld, nullptr);
				DrawPicture(inMask, &maskFrame);
				SetGWorld(oldPort, oldDevice);
				
				result = CreateCGImageFromPixMaps(GetGWorldPixMap(imageGraphicsWorld), GetGWorldPixMap(maskGraphicsWorld), outCGImagePtr);
				DisposeGWorld(maskGraphicsWorld), maskGraphicsWorld = nullptr;
			}
			DisposeGWorld(imageGraphicsWorld), imageGraphicsWorld = nullptr;
		}
	}
	
	return result;
}// BuildCGImageFromPictureAndMask


/*!
Use this convenience method to invoke the
Embedding_OffscreenControlOperationInMode()
routine with a "srcCopy" (normal) drawing
mode.

(1.0)
*/
OSStatus
Embedding_OffscreenControlOperation		(WindowRef						inForWhichWindow,
										 ControlRef						inForWhichControlOrNullForRoot,
										 Embedding_OffscreenOpProcPtr	inWhatToDo,
										 SInt32							inData1,
										 SInt32							inData2)
{
	return Embedding_OffscreenControlOperationInMode(inForWhichWindow, inForWhichControlOrNullForRoot, inWhatToDo, inData1, inData2, srcCopy);
}// OffscreenControlOperation


/*!
To minimize the flickering caused by the sequential drawing
of many controls in a window, use this routine.  It will
make the given control of a window invisible, and then call
your routine to perform actions on controls in the window.
When your routine returns, the given control is made visible
and the entire operation (which will be in an offscreen
graphics buffer) will then be dumped to the screen.  The
entire port rectangle of the window is validated.

This has the benefit of completely avoiding flickering,
since all changes you make will appear to occur instantly,
not one after another.  However, if there is a problem with
managing an offscreen graphics world (not enough memory, for
instance), the routine you provide is simply invoked without
operating on a buffer (in other words, flickering occurs in
this case).

I believe there is a small memory leak in here someplace,
but I’m not sure where.  Heh heh heh heh heh ha (*cough*).

IMPORTANT:	Only the given control is drawn in the offscreen
			buffer, so this routine will only work for
			windows that do ALL of their drawing using
			embedded controls in a Mac OS 8 Appearance
			control hierarchy.  If you have special drawing
			to do, you had better create a user pane control
			and define a custom drawing routine to perform
			your special drawing; that way, you can still
			take advantage of this routine.

(1.0)
*/
OSStatus
Embedding_OffscreenControlOperationInMode	(WindowRef						inForWhichWindow,
											 ControlRef						inForWhichControlOrNullForRoot,
											 Embedding_OffscreenOpProcPtr	inWhatToDo,
											 SInt32							inData1,
											 SInt32							inData2,
											 SInt32							inDrawingMode)
{
	OSStatus	result = noErr;
	
	
	if (Embedding_WindowUsesCompositingMode(inForWhichWindow))
	{
		// compositing windows do not benefit from the fancy crap; just
		// do what was asked, and the results will be correct!
		HIViewRef	view = inForWhichControlOrNullForRoot;
		
		
		if (nullptr == view)
		{
			result = HIViewFindByID(HIViewGetRoot(inForWhichWindow), kHIViewWindowContentID, &view);
		}
		if (noErr == result)
		{
			(Boolean)Embedding_InvokeOffscreenOpProc(inWhatToDo, view, 8/* default depth */, 0/* default device flags */,
														GetMainDevice(), inData1, inData2);
		}
	}
	else
	{
		// normal windows have to do it the traditional, complex way!
		ControlRef	control = inForWhichControlOrNullForRoot;
		GrafPtr		oldPort = nullptr;
		
		
		GetPort(&oldPort);
		SetPortWindowPort(inForWhichWindow);
		
		if (control == nullptr) result = GetRootControl(inForWhichWindow, &control);
		if (result == noErr)
		{
			Rect							controlBounds;
			RgnHandle						oldClipRgn = nullptr;
			RgnHandle						newClipRgn = nullptr;
			My_OffscreenDeviceLoopParams	data;
			
			
			data.proc = inWhatToDo;
			data.window = inForWhichWindow;
			data.control = control;
			data.data1 = inData1;
			data.data2 = inData2;
			data.dumpMode = inDrawingMode;
			data.result = noErr;
			
			newClipRgn = Memory_NewRegion();
			oldClipRgn = Memory_NewRegion();
			
			// get the arrangement of this control
			GetControlBounds(control, &controlBounds);
			if (result == noErr)
			{
				if ((newClipRgn != nullptr) && (oldClipRgn != nullptr))
				{
					DeviceLoopDrawingUPP	upp = nullptr;
					
					
					// note that "oldClipRgn" is being used for multiple purposes here, despite its name
					RectRgn(newClipRgn, &controlBounds);
					GetPortVisibleRegion(GetWindowPort(inForWhichWindow), oldClipRgn);
					SectRgn(oldClipRgn, newClipRgn, newClipRgn);
					GetClip(oldClipRgn);
					SetClip(newClipRgn);
					
					// call DeviceLoop() to make sure the offscreen dump looks correct, no matter how many monitors it may cross
					upp = NewDeviceLoopDrawingUPP(offscreenDumpDeviceLoop);
					DeviceLoop(newClipRgn, upp, (long)&data/* user data required by the DeviceLoopDrawingUPP */, 0/* DeviceLoopFlags */);
					DisposeDeviceLoopDrawingUPP(upp), upp = nullptr;
					result = data.result;
					
					SetClip(oldClipRgn);
				}
				else
				{
					data.result = memFullErr;
				}
				
				// check for failures
				if (data.result != noErr)
				{
					// offscreen dump failed somehow - just invoke the routine manually, with “good guesses” of unknown parameters
					(OSStatus)SetUpControlBackground(control, ColorUtilities_ReturnCurrentDepth(GetWindowPort(inForWhichWindow)),
														IsPortColor(GetWindowPort(inForWhichWindow)));
					EraseRect(&controlBounds);
					if (inWhatToDo != nullptr)
					{
						// desperation: invoke the routine to draw the control anyway, providing reasonable default values
						(Boolean)Embedding_InvokeOffscreenOpProc(inWhatToDo, control, 8/* default depth */, 0/* default device flags */,
																	GetMainDevice(), inData1, inData2);
					}
					DrawOneControl(control);
				}
				
				if ((oldClipRgn != nullptr) && (newClipRgn != nullptr))
				{
					// Note that the following statements “really should” be conditionalized
					// beyond the preprocessor level, within a statement that checks for the
					// presence of the Mac OS 8.5 Window Manager.  However, since Carbon
					// *does* include the new Window Manager, and since the old routines all
					// work under even Mac OS 8.5 and beyond (but not Carbon), a TARGET_API_
					// conditional is used for simplified code.
					ValidWindowRgn(inForWhichWindow, newClipRgn);
				}
				else
				{
					ValidWindowRect(inForWhichWindow, &controlBounds);
				}
			}
			if (oldClipRgn != nullptr) Memory_DisposeRegion(&oldClipRgn);
			if (newClipRgn != nullptr) Memory_DisposeRegion(&newClipRgn);
		}
		SetPort(oldPort);
	}
	return result;
}// OffscreenControlOperationInMode


/*!
If two controls overlap precisely (like tab
panes), you can use this routine to hide the
first control and display the second control
in one smooth motion (memory permitting).
The result is that the second control is
drawn without flickering caused by erasing
the first control or drawing parts embedded
in the second control.

(1.0)
*/
OSStatus
Embedding_OffscreenSwapOverlappingControls	(WindowRef		inForWhichWindow,
											 ControlRef		inControlToHide,
											 ControlRef		inControlToDisplay)
{
	OSStatus	result = noErr;
	
	
	result = Embedding_OffscreenControlOperation(inForWhichWindow, inControlToDisplay, swapControlsOperation, (SInt32)inControlToHide, 0L);
	return result;
}// OffscreenSwapOverlappingControls


/*!
Returns "true" only if the specified window is in
compositing mode, meaning that its hierarchy uses
a more efficient drawing model.

In compositing mode, control drawing is all done
relative to view coordinates, it cannot be done
with any “raw” drawing routines like DrawOneControl(),
and it cannot erase its background first.

(1.3)
*/
Boolean
Embedding_WindowUsesCompositingMode		(WindowRef		inWindow)
{
	UInt32		windowAttributes = 0L;
	Boolean		result = false;
	
	
	if (noErr == GetWindowAttributes(inWindow, &windowAttributes))
	{
		// a raw attribute value is required as long as an older Mac OS X SDK
		// is used that does not declare the named constant (which was added
		// in Mac OS X 10.2)
		result = (0 != (windowAttributes & FUTURE_SYMBOL(1L << 19, kWindowCompositingAttribute)));
	}
	
	return result;
}// WindowUsesCompositingMode


#pragma mark Internal Methods

/*!
This method, of standard DeviceLoopDrawingUPP form,
handles offscreen drawing correctly no matter what
the device characteristics are.

(1.0)
*/
static void
offscreenDumpDeviceLoop		(short		inColorDepth,
							 short		inDeviceFlags,
							 GDHandle	inTargetDevice,
							 long		inDataPtr)
{
	My_OffscreenDeviceLoopParamsPtr		dataPtr = REINTERPRET_CAST(inDataPtr, My_OffscreenDeviceLoopParamsPtr);
	GrafPtr								oldPort = nullptr;
	
	
	GetPort(&oldPort);
	if (dataPtr != nullptr) //if (dataPtr->result == noErr) // any residual error from previous device loops should continue the termination
	{	
		Rect		drawingRect;
		Rect		controlRect;
		GWorldPtr	world = nullptr;
		GDHandle	oldDevice = nullptr;
		CGrafPtr	colorWindowPort = GetWindowPort(dataPtr->window);
		CGrafPtr	oldColorPort = nullptr;
		
		
		GetPortBounds(colorWindowPort, &drawingRect);
		GetControlBounds(dataPtr->control, &controlRect);
		
		// save old port
		dataPtr->result = NewGWorld(&world, inColorDepth, &drawingRect, (CTabHandle)nullptr, (GDHandle)inTargetDevice, 0L/* flags */);
		if (dataPtr->result == noErr)
		{
			PixMapHandle	pixels = nullptr;
			
			
			// set port to the new port
			GetGWorld(&oldColorPort, &oldDevice);
			SetGWorld(world, (GDHandle)nullptr);
			
			// get the offscreen graphics world’s pixel map
			pixels = GetGWorldPixMap(world);
			
			// perform the offscreen control operation, and then do the blitter as quickly and easily as possible
			if (LockPixels(pixels))
			{
				(OSStatus)SetUpControlBackground(dataPtr->control, inColorDepth, ColorUtilities_IsColorDevice(inTargetDevice));
				EraseRect(&drawingRect);
				
				// change the controls as specified by the user-defined routine
				if (dataPtr->proc != nullptr)
				{
					(Boolean)Embedding_InvokeOffscreenOpProc(dataPtr->proc, dataPtr->control, inColorDepth, inDeviceFlags,
																inTargetDevice, dataPtr->data1, dataPtr->data2);
				}
				
				// now dump the offscreen world to the onscreen window in the most efficient way possible
				{
					ControlRef		root = nullptr;
					long			quickTimeVersion = 0L;
					Boolean			isQuickTimeAvailable = (Gestalt(gestaltQuickTimeVersion, &quickTimeVersion) == noErr);
					Boolean			drawUsingCopyBits = true;
					OSStatus		rootError = noErr;
					
					
					rootError = GetRootControl(dataPtr->window, &root);
					
					// when using QuickTime, it is difficult to constrain drawing to the control alone
					// (at least, I couldn’t find an easy way); so, unless the desired control to draw
					// is the root control (which spans the entire window), QuickTime cannot be used
					// even if it is available
					if (rootError == noErr) drawUsingCopyBits = ((!isQuickTimeAvailable) || (root != dataPtr->control));
					
					if (drawUsingCopyBits)
					{
						// no QuickTime, so use CopyBits() as usual
						RGBColor	oldForeColor;
						RGBColor	oldBackColor;
						RGBColor	solid;
						
						
						// when using CopyBits(), it is easy to constrain drawing to the control alone
						DrawControlInCurrentPort(dataPtr->control);
						SetGWorld(colorWindowPort, nullptr/* device */);
						
						// set background to white and foreground to black, so CopyBits() doesn’t screw anything up
						GetForeColor(&oldForeColor);
						GetBackColor(&oldBackColor);
						
						// dump the offscreen to the onscreen; check for errors, since in such case a simple control draw can be issued
						solid.red = solid.blue = solid.green = 0;
						RGBForeColor(&solid);
						solid.red = solid.blue = solid.green = RGBCOLOR_INTENSITY_MAX;
						RGBBackColor(&solid);
						
						CopyBits(GetPortBitMapForCopyBits(world)/* source pixel map */,
								GetPortBitMapForCopyBits(colorWindowPort)/* destination pixel map */,
								&controlRect/* source rectangle */,
								&controlRect/* destination rectangle */,
								dataPtr->dumpMode/* drawing mode */, (RgnHandle)nullptr/* mask */);
						dataPtr->result = QDError();
						
						RGBForeColor(&oldForeColor);
						RGBBackColor(&oldBackColor);
					}
					else
					{
						// Use QuickTime’s Image Compression Manager for ultra-quick (and possibly
						// graphics-accelerated) offscreen-to-onscreen copying (not to mention a
						// hell of a lot less code!).  This is the only Carbon-compliant solution
						// (although supposedly Apple is planning to tweak the hell out of the
						// CopyBits() routine because so many gigabytes of existing code uses it,
						// hence such weird things like the GetPortBitMapForCopyBits() accessor).
						ImageDescriptionHandle		image = nullptr;
						
						
						DrawControlInCurrentPort(root);
						SetGWorld(colorWindowPort, nullptr/* device */);
						MakeImageDescriptionForPixMap(pixels, &image);
						dataPtr->result = DecompressImage(GetPixBaseAddr(pixels), image, GetPortPixMap(colorWindowPort),
															nullptr/* source rectangle - nullptr means decompress entire image */,
															&drawingRect/* destination rectangle */,
															dataPtr->dumpMode/* drawing mode */, nullptr/* mask */);
						DisposeHandle((Handle)image), image = nullptr;
					}
				}
				
				// the pixel map is no longer being directly used
				UnlockPixels(pixels);
			}
			else dataPtr->result = memPCErr;
			
			// restore to saved
			SetGWorld(oldColorPort, oldDevice);
			if (world != nullptr) DisposeGWorld(world), world = nullptr;
		}
	}
	
	SetPort(oldPort);
}// offscreenDumpDeviceLoop


/*!
This method is in standard OffscreenOperationProcPtr
form, and will use "inIsEnabled" (“data 1”) as the
enabled state (true or false) for the given control.
If nonzero, the specified control of the specified
window is activated; otherwise, it is deactivated.
If the desired control state is already in place,
this method has no effect.

If the operation cannot be completed, false is
returned; otherwise, true is returned.

(1.0)
*/
static Boolean
setControlActiveOperation	(ControlRef		inSpecificControlOrRoot,
							 SInt16			UNUSED_ARGUMENT(inColorDepth),
							 SInt16			UNUSED_ARGUMENT(inDeviceFlags),
							 GDHandle		UNUSED_ARGUMENT(inTargetDevice),
							 SInt32			inIsEnabled,
							 SInt32			UNUSED_ARGUMENT(inData2))
{
	Boolean		result = true;
	
	
	if (inSpecificControlOrRoot != nullptr)
	{
		if (inIsEnabled != IsControlActive(inSpecificControlOrRoot))
		{
			if (inIsEnabled) (OSStatus)ActivateControl(inSpecificControlOrRoot);
			else (OSStatus)DeactivateControl(inSpecificControlOrRoot);
		}
	}
	else
	{
		result = false;
	}
	
	return result;
}// setControlActiveOperation


/*!
This method is in standard OffscreenOperationProcPtr
form, and will use "inControlToDisplay" (“data 1”) as
the control that should be displayed (while the
specified control "inControlToHide" is hidden).

If the operation cannot be completed, false is
returned; otherwise, true is returned.

(1.0)
*/
static Boolean
swapControlsOperation	(ControlRef		inControlToDisplay,
						 SInt16			UNUSED_ARGUMENT(inColorDepth),
						 SInt16			UNUSED_ARGUMENT(inDeviceFlags),
						 GDHandle		UNUSED_ARGUMENT(inTargetDevice),
						 SInt32			inControlToHide,
						 SInt32			UNUSED_ARGUMENT(inData2))
{
	OSStatus		error = noErr;
	ControlRef		controlToHide = REINTERPRET_CAST(inControlToHide, ControlRef);
	Boolean			result = true;
	
	
	if (controlToHide != nullptr)
	{
		error = SetControlVisibility(controlToHide, false/* visibility */, false/* draw */);
		if (error != noErr) result = false;
		error = SetControlVisibility(inControlToDisplay, true/* visibility */, true/* draw */);
		if (error != noErr) result = false;
	}
	
	return result;
}// swapControlsOperation

// BELOW IS REQUIRED NEWLINE TO END FILE
