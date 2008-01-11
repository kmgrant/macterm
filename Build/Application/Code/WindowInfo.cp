/*###############################################################

	WindowInfo.cp
	
	Interface Library 2.0
	© 1998-2008 by Kevin Grant
	
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

// standard-C includes
#include <climits>

// library includes
#include <Embedding.h>
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>
#include <Releases.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"



#pragma mark Types

struct My_WindowInfo
{
	WindowInfo_Descriptor	windowDescriptor;
	Boolean					isFloatingWindow;		// is window considered a floating window?
	Boolean					isSheetWindow;			// is window attached to its parent?
	Boolean					isPotentialDropTarget;	// can the window EVER receive a drop?
	Boolean					noDynamicResize;		// force window to drag a resize outline?
	union
	{
		struct
		{
			SInt16		minimumHeight;
			SInt16		minimumWidth;
			SInt16		maximumHeight;
			SInt16		maximumWidth;
		} asShorts;
		Rect		asRect; // identical to the above information: convenient for calling GrowWindow()
	} sizeLimitRect;
	
	Point								preferredLocation;
	Point								preferredSize;
	WindowInfo_ContextualMenuProcPtr	contextualMenuNotifyMethod;
	WindowInfo_ResizeResponderProcPtr	resizeNotifyMethod;
	SInt32								resizeNotifyMethodData;
	void*								auxiliaryDataPtr;
};
typedef struct My_WindowInfo	My_WindowInfo;
typedef struct My_WindowInfo*	My_WindowInfoPtr;

typedef MemoryBlockHandleLocker< WindowInfo_Ref, My_WindowInfo >	My_WindowInfoHandleLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_WindowInfoHandleLocker	gWindowInfoHandleLocks;
	Boolean						gLiveResizing = true;
}



#pragma mark Public Methods

/*!
To create a new Window Info object, use this method.  To
assign the new feature set to a window, call one of the
WindowInfo_SetForDialog() or WindowInfo_SetForWindow()
methods.  To destroy the feature set you created with
this routine, call WindowInfo_Dispose().

(1.0)
*/
WindowInfo_Ref
WindowInfo_New ()
{
	WindowInfo_Ref		result = nullptr;
	
	
	result = REINTERPRET_CAST(Memory_NewHandle(sizeof(My_WindowInfo)), WindowInfo_Ref);
	WindowInfo_Init(result);
	
	return result;
}// New


/*!
To destroy a reference to Window Info
information previously created with the
WindowInfo_New() method, call this method.

WARNING: Calling this method makes the given
         reference completely invalid.  Do not
		 use it again.  Also, if you stored any
		 auxiliary data pointer in the window
		 features record that needs disposing,
		 use WindowInfo_ReturnAuxiliaryDataPtr()
		 to obtain and destroy that data first,
		 then call this method.

(1.0)
*/
void
WindowInfo_Dispose	(WindowInfo_Ref		inWindowInfoRef)
{
	if (inWindowInfoRef != nullptr)
	{
		Memory_DisposeHandle(REINTERPRET_CAST(&inWindowInfoRef, Handle*));
	}
}// Dispose


/*!
To initialize all of the fields of a new window
features structure to which you have a reference,
call this method.  Normally you do not need to
use this method, because WindowInfo_New() calls
it automatically upon creation of every new
reference.

(1.0)
*/
void
WindowInfo_Init		(WindowInfo_Ref		inoutWindowInfoRef)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inoutWindowInfoRef);
	
	
	windowInfoPtr->windowDescriptor = kWindowInfo_InvalidDescriptor;
	windowInfoPtr->isFloatingWindow = false;
	windowInfoPtr->isPotentialDropTarget = false;
	windowInfoPtr->noDynamicResize = false;
	windowInfoPtr->sizeLimitRect.asShorts.minimumHeight =
		windowInfoPtr->sizeLimitRect.asShorts.minimumWidth = 0;
	windowInfoPtr->sizeLimitRect.asShorts.maximumHeight =
		windowInfoPtr->sizeLimitRect.asShorts.maximumWidth = SHRT_MAX;
	windowInfoPtr->preferredLocation.h =
		windowInfoPtr->preferredLocation.v = 0;
	windowInfoPtr->contextualMenuNotifyMethod = nullptr;
	windowInfoPtr->resizeNotifyMethod = nullptr;
	windowInfoPtr->resizeNotifyMethodData = 0L;
	windowInfoPtr->auxiliaryDataPtr = nullptr;
	gWindowInfoHandleLocks.releaseLock(inoutWindowInfoRef, &windowInfoPtr);
}// Init


/*!
To determine the largest size that a resizable dialog
box can have, call this method.

(1.0)
*/
void
WindowInfo_GetWindowMaximumDimensions	(WindowInfo_Ref		inWindowInfoRef,
										 Point*				outMaximumSizePtr)
{
	if (outMaximumSizePtr != nullptr)
	{
		My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
		
		
		SetPt(outMaximumSizePtr, windowInfoPtr->sizeLimitRect.asShorts.maximumWidth,
							windowInfoPtr->sizeLimitRect.asShorts.maximumHeight);
		gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
	}
}// GetWindowMaximumDimensions


/*!
To determine the smallest size that a resizable dialog
box can have, call this method.

(1.0)
*/
void
WindowInfo_GetWindowMinimumDimensions	(WindowInfo_Ref		inWindowInfoRef,
										 Point*				outMinimumSizePtr)
{
	if (outMinimumSizePtr != nullptr)
	{
		My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
		
		
		SetPt(outMinimumSizePtr, windowInfoPtr->sizeLimitRect.asShorts.minimumWidth,
							windowInfoPtr->sizeLimitRect.asShorts.minimumHeight);
		gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
	}
}// GetWindowMinimumDimensions


/*!
To determine the smallest and largest sizes that a
resizable dialog box can have, call this method.
The returned rectangle pointer is specially set up
to be ideal input to the GrowWindow() system call.
You’re welcome.

(1.0)
*/
Rect*
WindowInfo_ReturnWindowResizeLimits		(WindowInfo_Ref		inWindowInfoRef)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	Rect*				result = nullptr;
	
	
	result = &windowInfoPtr->sizeLimitRect.asRect;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
	return result;
}// ReturnWindowResizeLimits


/*!
To handle a mouse-down event in the size region of a
window that has general window data associated with it,
call this convenient method.  Using window data, this
method automatically determines the resize constraints
on the window, and, if the user resizes the window, the
universal procedure pointer to invoke in order to
handle the new window size.  The Mac OS “user state”
for the window is automatically updated to reflect the
new size.

WARNING: Do not pass the Mac OS window pointer of a
         window whose reference constant field refers
		 to anything but a reference to general window
		 data as defined by this module.

(1.0)
*/
SInt16
WindowInfo_GrowWindow	(HIWindowRef		inWindow,
						 EventRecord*		inoutEventPtr)
{
	WindowInfo_Ref	windowInfoRef = WindowInfo_ReturnFromWindow(inWindow);
	SInt16			result = 0;
	
	
	if (windowInfoRef == nullptr) Sound_StandardAlert();
	else
	{
		My_WindowInfoPtr	ptr = gWindowInfoHandleLocks.acquireLock(windowInfoRef);
		long				growResult = 0L;
		Rect				contentRect;
		Boolean				smallWindow = false;
		
		
		// Obtain the “current” size of the window, so when it
		// changes it is possible to calculate deltaX and deltaY.
		// Then, change the rectangle so its origin is (0, 0),
		// to make it easier to work with.  Set a flag if the
		// window is small; small windows are dynamically resized
		// one pixel at a time, instead of using a larger grid.
		{
			RgnHandle		contentRegion = Memory_NewRegion();
			
			
			GetWindowRegion(inWindow, kWindowContentRgn, contentRegion);
			GetRegionBounds(contentRegion, &contentRect);
			Memory_DisposeRegion(&contentRegion);
			if (((contentRect.right - contentRect.left) < 100) ||
				((contentRect.bottom - contentRect.top) < 64)) smallWindow = true; // arbitrary rules here
		}
		
		if ((gLiveResizing) && (!ptr->noDynamicResize))
		{
			RgnHandle		region = Memory_NewRegion();
			
			
			if (region != nullptr)
			{
				Rect				structureRect,
									pinnedRect;
				Point				oldMouse,
									currentMouse;
				SInt32				deltaX = 0L,
									deltaY = 0L;
				Boolean				done = false,
									haveAppearance1_1 = false; // minimum version for theme drag sounds
				EventRecord			event;
				register UInt16		x = 0;
				
				
				// Appearance 1.1 or later?
				{
					long		sysv = 0L;
					UInt8		majorRev = 0,
								minorRev = 0;
					
					
					(OSStatus)Gestalt(gestaltAppearanceVersion, &sysv);
					majorRev = Releases_ReturnMajorRevisionForVersion(sysv);
					minorRev = Releases_ReturnMinorRevisionForVersion(sysv);
					
					haveAppearance1_1 = (((majorRev == 0x01) && (minorRev >= 0x01)) || (majorRev > 0x01));
				}
				
				// set up the mouse variables and mouse-moved region prior to starting the loop
				{
					Point		globalMouse;
					
					
					GetMouse(&globalMouse);
					LocalToGlobal(&globalMouse);
					
					// Set the initial mouse region to be the initial mouse location, only.
					// Note that the "region" variable should remain unchanged between here
					// and the WaitNextEvent() at the start of the loop!
					SetEmptyRgn(region);
					SetRectRgn(region, globalMouse.h, globalMouse.v, globalMouse.h, globalMouse.v);
					SetPt(&oldMouse, globalMouse.h, globalMouse.v);
					SetPt(&currentMouse, globalMouse.h, globalMouse.v);
				}
				
				// The slop measurements force the mouse to return to its location
				// before a window can be resized, should the mouse previously move
				// in the opposite direction beyond the resize limitations.  If
				// this wasn’t done, it would be possible for the cursor to drift
				// away from the size box, while its movements would still resize
				// the window, which is very frustrating for users.
				//
				// The slop rectangle is defined as the area you get when you map
				// the lower-right corner of the smallest allowed window boundaries
				// to the lower-right corner of the largest allowed boundaries.  If
				// the mouse moves outside of this area, the window size will not
				// change even though the mouse continues to move.
				{
					Point		zero,
								upperBound,
								lowerBound;
					
					
					// this easily determines the global origin of the window, called "zero"
					{
						CGrafPtr	oldPort = nullptr;
						GDHandle	oldDevice = nullptr;
						
						
						GetGWorld(&oldPort, &oldDevice);
						SetPortWindowPort(inWindow);
						SetPt(&zero, 0, 0);
						LocalToGlobal(&zero);
						SetGWorld(oldPort, oldDevice);
					}
					
					// Determine the global coordinates of the lower-right corners
					// of the smallest and largest allowed window sizes by adding
					// the coordinates of "zero" to the dimensions.  Careful, the
					// extreme width/height is often set to SHRT_MAX!  Since this
					// integer is as large as it can be, it is incorrect to simply
					// offset this value!  An explicit boundary check is therefore
					// necessary.
					SetPt(&upperBound, ptr->sizeLimitRect.asShorts.minimumWidth,
										ptr->sizeLimitRect.asShorts.minimumHeight);
					if (upperBound.h < (SHRT_MAX - zero.h)) upperBound.h += zero.h;
					if (upperBound.v < (SHRT_MAX - zero.v)) upperBound.v += zero.v;
					SetPt(&lowerBound, ptr->sizeLimitRect.asShorts.maximumWidth,
										ptr->sizeLimitRect.asShorts.maximumHeight);
					if (lowerBound.h < (SHRT_MAX - zero.h)) lowerBound.h += zero.h;
					if (lowerBound.v < (SHRT_MAX - zero.v)) lowerBound.v += zero.v;
					
					SetRect(&pinnedRect, upperBound.h, upperBound.v, lowerBound.h, lowerBound.v);
				}
				
				// Under Mac OS 8.5 or later, use an Appearance drag sound.
				if (haveAppearance1_1)
				{
					(OSStatus)BeginThemeDragSound(kThemeDragSoundGrowWindow);
				}
				
				// In a special event loop, dynamically resize the specified window.
				// The use of an event loop allows the mouse to be tracked accurately,
				// and it allows the system to periodically service update events in
				// *other* applications while the window is being resized!
				while (!done)
				{
					WaitNextEvent(mUpMask | osMask, &event,
								#if TARGET_API_MAC_OS8
									4L/* sleep ticks */,
								#else
									// on Mac OS X, this value should be huge, to reduce
									// polling drags on the system
									0x7FFFFFFF,
								#endif
									region/* mouse-moved region */);
					
					done = (event.what == mouseUp); // stop when the mouse button is released
					if (event.what == osEvt)
					{
						// when the mouse moves, change the window size
						if (((event.message & osEvtMessageMask) >> 24) == mouseMovedMessage)
						{
							// It is possible to “cheat” by holding the perceived mouse
							// location within the resize limits, thereby making it
							// impossible for the mouse to seem outside this area, no
							// matter where the mouse really is.
							{
								SInt32		pinResult = 0L;
								
								
								pinResult = PinRect(&pinnedRect, event.where/* real mouse location */);
								SetPt(&currentMouse/* perceived location */, LoWord(pinResult), HiWord(pinResult));
							}
							
							deltaX = currentMouse.h - oldMouse.h;
							deltaY = currentMouse.v - oldMouse.v;
							
							SetEmptyRgn(region);
							GetWindowRegion(inWindow, kWindowContentRgn, region);
							GetRegionBounds(region, &contentRect);
							
							// constrain the delta to a multiple of 4 (arbitrary) pixels in each direction
							{
								SInt16		diff = 0;
								
								
								diff = (smallWindow) ? 1 : (deltaX % 4);
								currentMouse.h -= diff;
								deltaX -= diff;
								diff = (smallWindow) ? 1 : (deltaY % 4);
								currentMouse.v -= diff;
								deltaY -= diff;
							}
							
							if ((deltaX) || (deltaY))
							{
								// resize the window frame, and then issue a notification to resize controls, too
								SizeWindow(inWindow, contentRect.right - contentRect.left + deltaX,
													contentRect.bottom - contentRect.top + deltaY, false);
								WindowInfo_NotifyWindowOfResize(inWindow, deltaX, deltaY);
								
								// updates are slow, so don’t perform one *every* time the mouse moves
								if (!(x % 50/* arbitrary; the larger this number, the rarer the updates */))
								{
									SetEmptyRgn(region);
									GetWindowRegion(inWindow, kWindowUpdateRgn, region);
									BeginUpdate(inWindow);
									(OSStatus)Embedding_OffscreenControlOperation(inWindow, nullptr/* control */,
																					nullptr/* procedure */,
																					0L/* data 1 */,
																					0L/* data 2 */);
									EndUpdate(inWindow);
								}
							}
							SetPt(&oldMouse, currentMouse.h, currentMouse.v);
							
							// set the mouse-moved region to be the exact point where the mouse is now
							SetEmptyRgn(region);
							SetRectRgn(region, currentMouse.h, currentMouse.v, currentMouse.h, currentMouse.v);
						}
					}
					++x;
				#if TARGET_API_MAC_OS8
					// it is only safe to do this yield here on a cooperatively-multitasked OS
					(OSStatus)YieldToAnyThread();
				#endif
				}
				
				// get rid of extra events
				FlushEvents(mUpMask | osMask, 0);
				
				// Under Mac OS 8.5 or later, use an Appearance drag sound.
				if (haveAppearance1_1)
				{
					(OSStatus)EndThemeDragSound();
				}
				
				SetEmptyRgn(region);
				GetWindowRegion(inWindow, kWindowStructureRgn, region);
				GetRegionBounds(region, &structureRect);
				SetWindowUserState(inWindow, &structureRect);
				
				Memory_DisposeRegion(&region);
			}
		}
		else
		{
			SetRect(&contentRect, 0, 0, (contentRect.right - contentRect.left),
					(contentRect.bottom - contentRect.top));
			
		#if TARGET_API_MAC_OS8
			growResult = GrowWindow(inWindow, inoutEventPtr->where,
									WindowInfo_ReturnWindowResizeLimits(windowInfoRef));
		#else
			{
				Rect	newContentRect;
				
				
				if (ResizeWindow(inWindow, inoutEventPtr->where,
									WindowInfo_ReturnWindowResizeLimits(windowInfoRef),
									&newContentRect))
				{
					growResult = (newContentRect.right - newContentRect.left) |
									((newContentRect.bottom - newContentRect.top) << 16);
				}
				else
				{
					growResult = 0;
				}
			}
		#endif
			if (growResult != 0)
			{
				SInt32		deltaX = 0L;
				SInt32		deltaY = 0L;
				
				
				// at this point, contentRect is at a (0, 0) origin, so
				// the "right" and "bottom" components are actually the
				// width and height, respectively
				deltaX = LoWord(growResult) - contentRect.right;
				deltaY = HiWord(growResult) - contentRect.bottom;
				
				// display a cool animation that gradually changes the size when resizing dialogs
				if (GetWindowKind(inWindow) == kDialogWindowKind)
				{
					enum
					{
						// increase this value to animate this effect
						kDeltaDiv = 1	// number of times the window size changes during the animation
					};
					SInt16				width = contentRect.right - contentRect.left,
										height = contentRect.bottom - contentRect.top;
					SInt16				divX = 0,
										divY = 0,
										dx = 0,
										dy = 0;
					register SInt16		i = 0;
					
					
					// how much the window changes per animation stage
					divX = FixRound(FixRatio(deltaX, kDeltaDiv));
					divY = FixRound(FixRatio(deltaY, kDeltaDiv));
					
					// animate!
					for (i = 0; i < kDeltaDiv; ++i)
					{
						// calculate the amount to change the window size by, and avoid
						// problems due to limited precision by ensuring the “real”
						// change in size is ultimately used (otherwise, rounding error
						// would potentially make the window size a few pixels out)
						width += divX;
						dx += divX;
						if (deltaX >= 0) while (dx > deltaX) --dx, --divX, --width;
						else while (dx < deltaX) ++dx, ++divX, ++width;
						height += divY;
						dy += divY;
						if (deltaY >= 0) while (dy > deltaY) --dy, --divY, --height;
						else while (dy < deltaY) ++dy, ++divY, ++height;
						
						// resize the window and its controls
						SizeWindow(inWindow, width, height, false);
						WindowInfo_NotifyWindowOfResize(inWindow, divX, divY);
					}
				}
				else
				{
					// resize the window and its controls
					SizeWindow(inWindow, contentRect.right - contentRect.left + deltaX,
								contentRect.bottom - contentRect.top + deltaY, false);
					WindowInfo_NotifyWindowOfResize(inWindow, deltaX, deltaY);
				}
				
				// update the zoom box settings
				{
					RgnHandle	region = Memory_NewRegion();
					
					
					if (region != nullptr)
					{
						Rect	structureRect;
						
						
						GetWindowRegion(inWindow, kWindowStructureRgn, region);
						GetRegionBounds(region, &structureRect);
						Memory_DisposeRegion(&region);
						SetWindowUserState(inWindow, &structureRect);
					}
				}
			}
		}
		gWindowInfoHandleLocks.releaseLock(windowInfoRef, &ptr);
	}
	
	return result;
}// GrowWindow


/*!
Returns "true" only if a window can *ever* be a
drag-and-drop target.  This says nothing about
what kind of data might be dropped, or even if a
drop is taking place; it only helps to filter
out windows that can never receive drops of any
kind.

(1.0)
*/
Boolean
WindowInfo_IsPotentialDropTarget	(WindowInfo_Ref		inWindowInfoRef)
{
	Boolean				result = false;
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	if (windowInfoPtr != nullptr) result = (windowInfoPtr->isPotentialDropTarget);
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
	return result;
}// IsPotentialDropTarget


/*!
To determine if a window is a utility window
that can remain active even while another window
is “frontmost”, use this method.  This simply
returns a flag previously set by a call to
WindowInfo_SetWindowFloating().

(1.0)
*/
Boolean
WindowInfo_IsWindowFloating	(WindowInfo_Ref		inWindowInfoRef)
{
	Boolean				result = false;
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	if (windowInfoPtr != nullptr) result = (windowInfoPtr->isFloatingWindow);
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
	return result;
}// IsWindowFloating


/*!
To call the contextual menu responder method
from a particular Window Info structure to
which you have a reference, call this method.
If a window does not have any contextual menu
procedure in its general window data, this
routine does nothing.

WARNING:	Do not invoke this routine if you
			have not associated Window Info
			with the specified Mac OS window
			using WindowInfo_SetForWindow() or
			WindowInfo_SetForDialog().

(1.0)
*/
void
WindowInfo_NotifyWindowOfContextualMenu	(HIWindowRef	inWindow,
										 Point			inGlobalMouse)
{
	WindowInfo_Ref		windowInfoRef = WindowInfo_ReturnFromWindow(inWindow);
	My_WindowInfoPtr	windowInfoPtr = nullptr;
	
	
	if (windowInfoRef != nullptr)
	{
		windowInfoPtr = gWindowInfoHandleLocks.acquireLock(windowInfoRef);
		if (windowInfoPtr->contextualMenuNotifyMethod != nullptr)
		{
			WindowInfo_InvokeContextualMenuProc(windowInfoPtr->contextualMenuNotifyMethod,
												inWindow, inGlobalMouse, nullptr/* tmp */, 0/* tmp*/);
		}
		gWindowInfoHandleLocks.releaseLock(windowInfoRef, &windowInfoPtr);
	}
}// NotifyWindowOfContextualMenu


/*!
To call the window resize responder method
from a particular Window Info structure to
which you have a reference, call this method.
If a window does not have any resize response
procedure in its general window data, this
routine does nothing.

WARNING:	Do not invoke this routine if you
			have not associated Window Info
			with the specified Mac OS window
			using WindowInfo_SetForWindow() or
			WindowInfo_SetForDialog().

(1.0)
*/
void
WindowInfo_NotifyWindowOfResize		(HIWindowRef	inWindow,
									 SInt32			inDeltaSizeX, 
									 SInt32			inDeltaSizeY)
{
	WindowInfo_Ref		windowInfoRef = WindowInfo_ReturnFromWindow(inWindow);
	My_WindowInfoPtr	windowInfoPtr = nullptr;
	
	
	if (windowInfoRef != nullptr)
	{
		windowInfoPtr = gWindowInfoHandleLocks.acquireLock(windowInfoRef);
		if (windowInfoPtr->resizeNotifyMethod != nullptr)
		{
			WindowInfo_InvokeResizeResponderProc(windowInfoPtr->resizeNotifyMethod,
												inWindow, inDeltaSizeX, inDeltaSizeY,
												windowInfoPtr->resizeNotifyMethodData);
		}
		gWindowInfoHandleLocks.releaseLock(windowInfoRef, &windowInfoPtr);
	}
}// NotifyWindowOfResize


/*!
To obtain the auxiliary data pointer of a
particular Window Info structure to which
you have a reference, call this method.  Do not
pass a nullptr reference to this method!

(1.0)
*/
void*
WindowInfo_ReturnAuxiliaryDataPtr	(WindowInfo_Ref		inWindowInfoRef)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	void*				result = nullptr;
	
	
	result = windowInfoPtr->auxiliaryDataPtr;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
	return result;
}// ReturnAuxiliaryDataPtr


/*!
To obtain a reference to the general window data for a
particular dialog box, call this method.

WARNING: The Window Info information is assumed to
         exist in the reference constant field of the
		 dialog’s window.  If you provide a dialog whose
		 window has no Window Info but has a non-nullptr
		 reference constant, using the resultant pointer
		 will most likely cause a pointer referencing
		 error and crash the program.

(1.0)
*/
WindowInfo_Ref
WindowInfo_ReturnFromDialog		(DialogRef	inDialog)
{
	return WindowInfo_ReturnFromWindow(GetDialogWindow(inDialog));
}// ReturnFromDialog


/*!
To obtain a reference to the general window data for a
particular window, call this method.

The Window Info information is assumed to exist as a
specific property on the window.  If that property is
not present, nullptr is returned.

(1.0)
*/
WindowInfo_Ref
WindowInfo_ReturnFromWindow		(HIWindowRef	inWindow)
{
	WindowInfo_Ref		result = nullptr;
	
	
	if (inWindow != nullptr)
	{
		OSStatus	error = noErr;
		
		
		error = GetWindowProperty(inWindow, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_WindowPropertyTypeWindowInfoRef,
									sizeof(result), nullptr/* actual size */, &result);
		if (error != noErr)
		{
			result = nullptr;
		}
	}
	return result;
}// ReturnFromWindow


/*!
To obtain the window descriptor constant from a
particular general window data reference, call
this method.  Do not pass a nullptr reference to
this method!

(1.0)
*/
WindowInfo_Descriptor
WindowInfo_ReturnWindowDescriptor	(WindowInfo_Ref		inWindowInfoRef)
{
	WindowInfo_Descriptor	result = kWindowInfo_InvalidDescriptor;
	My_WindowInfoPtr		windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	if (windowInfoPtr != nullptr) result = (windowInfoPtr->windowDescriptor);
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
	return result;
}// ReturnWindowDescriptor


/*!
To obtain the auxiliary data pointer of a particular
Window Info structure to which you have a reference,
call this method.

WARNING: If any problems occur, nullptr is returned.

(1.0)
*/
void
WindowInfo_SetAuxiliaryDataPtr	(WindowInfo_Ref		inWindowInfoRef,
								 void*				inAuxiliaryDataPtr)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	windowInfoPtr->auxiliaryDataPtr = inAuxiliaryDataPtr;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
}// SetAuxiliaryDataPtr


/*!
To specify whether windows are resized dynamically
(where the window size physically changes as the
user drags, with no drag outline), use this method.

IMPORTANT:	To globally set this flag for all
			windows, pass nullptr for the object
			reference.  Generally, you always set
			the global flag, and then pass a
			specific Window Info object to reverse
			the setting for particular windows.

(1.0)
*/
void
WindowInfo_SetDynamicResizing		(WindowInfo_Ref		inWindowFeaturesRefOrNullToSetGlobalFlag,
									 Boolean			inLiveResizeEnabled)
{
	if (inWindowFeaturesRefOrNullToSetGlobalFlag != nullptr)
	{
		My_WindowInfoPtr	ptr = gWindowInfoHandleLocks.acquireLock(inWindowFeaturesRefOrNullToSetGlobalFlag);
		
		
		if (ptr != nullptr)
		{
			ptr->noDynamicResize = !inLiveResizeEnabled;
		}
		gWindowInfoHandleLocks.releaseLock(inWindowFeaturesRefOrNullToSetGlobalFlag, &ptr);
	}
	else
	{
		gLiveResizing = inLiveResizeEnabled;
	}
}// SetDynamicResizing


/*!
To specify if a window is a utility window that
can remain active even while another window is
“frontmost”, use this method.  This simply sets
a flag that can subsequently be read by a call
to WindowInfo_IsWindowFloating().

(1.0)
*/
void
WindowInfo_SetWindowFloating	(WindowInfo_Ref		inWindowInfoRef,
								 Boolean			inIsFloating)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	if (windowInfoPtr != nullptr) windowInfoPtr->isFloatingWindow = inIsFloating;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
}// SetWindowFloating


/*!
To specify if a window can EVER receive data
via drag-and-drop, use this method.  This simply
sets a flag that can subsequently be read by a
call to WindowInfo_IsWindowPotentialDropTarget().

(1.0)
*/
void
WindowInfo_SetWindowPotentialDropTarget		(WindowInfo_Ref		inWindowInfoRef,
											 Boolean			inIsPotentialDropTarget)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	if (windowInfoPtr != nullptr) windowInfoPtr->isPotentialDropTarget = inIsPotentialDropTarget;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
}// SetWindowPotentialDropTarget


/*!
To specify a general window data reference for a
particular dialog box, call this method.

WARNING: The Window Info information must be placed
         in the reference constant field of the specified
		 dialog’s window.  If you provide a window that
		 needs to use the reference constant field for
		 something else, you cannot use Window Info.

(1.0)
*/
void
WindowInfo_SetForDialog	(DialogRef			inDialog,
						 WindowInfo_Ref		inWindowInfoRef)
{
	WindowInfo_SetForWindow(GetDialogWindow(inDialog), inWindowInfoRef);
}// SetForDialog


/*!
To specify a general window data reference for a
particular window, call this method.

WARNING: The Window Info information must be placed
         in the reference constant field of the specified
		 window.  If you provide a window that needs to use
		 the reference constant field for something else,
		 you cannot use Window Info.

(1.0)
*/
void
WindowInfo_SetForWindow	(HIWindowRef		inWindow,
						 WindowInfo_Ref		inWindowInfoRef)
{
	OSStatus	error = noErr;
	
	
	error = SetWindowProperty(inWindow, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_WindowPropertyTypeWindowInfoRef,
								sizeof(inWindowInfoRef), &inWindowInfoRef);
	assert(error == noErr);
}// SetForWindow


/*!
To specify the method that is invoked following
a contextual menu event, use this method.  It is
preferable to attach a callback to a window for
contextual menus, so that contextual menu code
can be placed with the rest of the window’s code,
and not in the event loop.  Therefore, try to
use this routine on as many windows as possible.

(1.0)
*/
void
WindowInfo_SetWindowContextualMenuResponder	(WindowInfo_Ref						inWindowInfoRef,
											 WindowInfo_ContextualMenuProcPtr	inNewProc)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	windowInfoPtr->contextualMenuNotifyMethod = inNewProc;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
}// SetWindowContextualMenuResponder


/*!
To specify the window descriptor constant from a
particular Window Info structure to which you
have a reference, call this method.

(1.0)
*/
void
WindowInfo_SetWindowDescriptor	(WindowInfo_Ref			inWindowInfoRef,
								 WindowInfo_Descriptor	inNewWindowDescriptor)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	windowInfoPtr->windowDescriptor = inNewWindowDescriptor;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
}// SetWindowDescriptor


/*!
To specify the limits on the dimensions of a dialog box
that is associated with Window Info information, call
this method.  A dialog box that does not have a grow box
does not need to define this information in its window
features.

(1.0)
*/
void
WindowInfo_SetWindowResizeLimits	(WindowInfo_Ref		inWindowInfoRef,
									 SInt16				inMinimumHeight,
									 SInt16				inMinimumWidth,
									 SInt16				inMaximumHeight,
									 SInt16				inMaximumWidth)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	windowInfoPtr->sizeLimitRect.asShorts.minimumHeight = inMinimumHeight;
	windowInfoPtr->sizeLimitRect.asShorts.minimumWidth = inMinimumWidth;
	windowInfoPtr->sizeLimitRect.asShorts.maximumHeight = inMaximumHeight;
	windowInfoPtr->sizeLimitRect.asShorts.maximumWidth = inMaximumWidth;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
}// SetWindowResizeLimits


/*!
To specify the method that is invoked following
a size change to a window supporting window
features, call this method.  The resize responder
usually handles rudimentary cleanup tasks, such
as moving and/or resizing content controls and
redrawing any regions that have not been updated.

(1.0)
*/
void
WindowInfo_SetWindowResizeResponder	(WindowInfo_Ref						inWindowInfoRef,
									 WindowInfo_ResizeResponderProcPtr	inNewProc,
									 SInt32								inData)
{
	My_WindowInfoPtr	windowInfoPtr = gWindowInfoHandleLocks.acquireLock(inWindowInfoRef);
	
	
	windowInfoPtr->resizeNotifyMethod = inNewProc;
	windowInfoPtr->resizeNotifyMethodData = inData;
	gWindowInfoHandleLocks.releaseLock(inWindowInfoRef, &windowInfoPtr);
}// SetWindowResizeResponder

