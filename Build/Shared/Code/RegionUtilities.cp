/*!	\file RegionUtilities.cp
	\brief A powerful set of rectangle, region and window
	utilities.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2018 by Kevin Grant
	
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

#include <RegionUtilities.h>
#include <UniversalDefines.h>

// library includes
#include <CGContextSaveRestore.h>
#include <Console.h>
#include <MemoryBlocks.h>



#pragma mark Public Methods

/*!
A high level shape drawing routine for a Quartz
graphics context that can be used to replace calls
to QuickDraw rounded-rectangle drawing routines
like FrameRoundRect() and PaintRoundRect().

Based on a technical note from Apple.

(3.1)
*/
void
RegionUtilities_AddRoundedRectangleToPath	(CGContextRef		inContext,
											 CGRect const&		inFrame,
											 float				inCurveWidth,
											 float				inCurveHeight)
{
	if ((0 == inCurveWidth) || (0 == inCurveHeight))
	{
		// without curves, this is just a regular rectangle
		CGContextAddRect(inContext, inFrame);
	}
	else
	{
		CGContextSaveRestore	contextLock(inContext);
		
		
		// start in the lower-left corner
		CGContextTranslateCTM(inContext, CGRectGetMinX(inFrame), CGRectGetMinY(inFrame));
		
		// normalize
		CGContextScaleCTM(inContext, inCurveWidth, inCurveHeight);
		
		// define the entire path
		{
			float const		kScaledWidth = CGRectGetWidth(inFrame) / inCurveWidth;
			float const		kScaledHalfWidth = kScaledWidth * 0.5f;
			float const		kScaledHeight = CGRectGetHeight(inFrame) / inCurveHeight;
			float const		kScaledHalfHeight = kScaledHeight * 0.5f;
			float const		kRadius = 1.0;
			
			
			// Since CGContextAddArcToPoint() draws both the curved part
			// and the line segments leading up to the curve, each call
			// is constructing more of the shape than is first apparent.
			// The order and starting point are also important, as the
			// “current” point is one of 3 key points defining the
			// tangential lines for each curve.  Each equal "L" piece of
			// the rectangle is drawn in turn, generating circular corners.
			CGContextMoveToPoint(inContext, kScaledWidth, kScaledHalfHeight);
			CGContextAddArcToPoint(inContext, kScaledWidth/* x1 */, kScaledHeight/* y1 */,
									kScaledHalfWidth/* x2 */, kScaledHeight/* y2 */, kRadius);
			CGContextAddArcToPoint(inContext, 0/* x1 */, kScaledHeight/* y1 */,
									0/* x2 */, kScaledHalfHeight/* y2 */, kRadius);
			CGContextAddArcToPoint(inContext, 0/* x1 */, 0/* y1 */,
									kScaledHalfWidth/* x2 */, 0/* y2 */, kRadius);
			CGContextAddArcToPoint(inContext, kScaledWidth/* x1 */, 0/* y1 */,
									kScaledWidth/* x2 */, kScaledHalfHeight/* y2 */, kRadius);
			CGContextClosePath(inContext);
		}
	}
}// AddRoundedRectangleToPath


/*!
Modifies the first rectangle to be positioned
exactly in the center of the second rectangle.
The size is unchanged.

(3.1)
*/
void
RegionUtilities_CenterHIRectIn	(HIRect&		inoutInner,
								 HIRect const&	inOuter)
{
	// the algorithm offsets the first rectangle based on the difference between
	// the center points of the 2nd and 1st rectangles
	inoutInner = CGRectOffset(inoutInner, CGRectGetMidX(inOuter) - CGRectGetMidX(inoutInner),
								CGRectGetMidY(inOuter) - CGRectGetMidY(inoutInner));
}// CenterHIRectIn


/*!
Modifies the first rectangle to be positioned
exactly in the center of the second rectangle.
The size is unchanged.

(2.6)
*/
void
RegionUtilities_CenterRectIn	(Rect*			inoutInner,
								 Rect const*	inOuter)
{
	// the algorithm offsets the first rectangle based on the difference between
	// the center points of the 2nd and 1st rectangles, which conceptually is this:
	//		RegionUtilities_OffsetRect(inoutInner, (inOuter->left + INTEGER_DIV_2(inOuter->right - inOuter->left)) -
	//									(inoutInner->left + INTEGER_DIV_2(inoutInner->right - inoutInner->left)),
	//								(inOuter->top + INTEGER_DIV_2(inOuter->bottom - inOuter->top)) -
	//									(inoutInner->top + INTEGER_DIV_2(inoutInner->bottom - inoutInner->top)));
	// however, it’s possible to multiply out the above equation and reduce it
	// to the following, simpler form; since at first glance the simpler form
	// is less obviously correct, the long expansion is included above for your
	// peace of mind :)
	RegionUtilities_OffsetRect(inoutInner,
								STATIC_CAST(INTEGER_DIV_2(inOuter->right + inOuter->left - inoutInner->right - inoutInner->left), SInt16),
								STATIC_CAST(INTEGER_DIV_2(inOuter->bottom + inOuter->top - inoutInner->bottom - inoutInner->top), SInt16));
}// CenterRectIn


/*!
A replacement for code depending on the old QuickDraw
method EqualRect().

DEPRECATED.  Transitional only.  Allows for SDK migration.

(2016.08)
*/
Boolean
RegionUtilities_EqualRects	(Rect const*			inRect1Ptr,
							 Rect const*			inRect2Ptr)
{
	Boolean		result = false;
	
	
	if ((inRect1Ptr->left == inRect2Ptr->left) &&
		(inRect1Ptr->top == inRect2Ptr->top) &&
		(inRect1Ptr->right == inRect2Ptr->right) &&
		(inRect1Ptr->bottom == inRect2Ptr->bottom))
	{
		result = true;
	}
	return result;
}// EqualRects


/*!
Combines a call to GetWindowGreatestAreaDevice() and
GetAvailableWindowPositioningBounds().

This routine is very useful for implementing operations
such as zooming windows, since you typically need the
available space to make zoomed-state calculations.

(3.0)
*/
void
RegionUtilities_GetPositioningBounds	(HIWindowRef	inWindow,
										 Rect*			outRectPtr)
{
	if (nullptr != outRectPtr)
	{
		GDHandle	windowDevice = nullptr;
		OSStatus	error = GetWindowGreatestAreaDevice(inWindow, kWindowContentRgn, &windowDevice,
														nullptr/* screen size */);
		
		
		if (noErr != error)
		{
			windowDevice = GetMainDevice();
		}
		error = GetAvailableWindowPositioningBounds(windowDevice, outRectPtr);
		if (noErr != error)
		{
			Console_Warning(Console_WriteValue, "failed to find window positioning bounds, error", error);
			SetRect(outRectPtr, 0, 0, 0, 0);
		}
	}
}// GetPositioningBounds


/*!
This powerful routine iterates through the
device list, determines the monitor that
contains the largest chunk of the given
window’s structure region, and then returns
the gray region of that device.  Usually,
this will be the entire chunk of the desktop
that crosses the device, unless it’s the
main device, in which case the region will
have the menu bar height subtracted from it.

You should use this routine in place of the
routine GetScreenBounds() whenever possible.

(3.0)
*/
void
RegionUtilities_GetWindowDeviceGrayRect		(WindowRef	inWindow,
											 Rect*		outRectPtr)
{
	if (outRectPtr != nullptr)
	{
		GDHandle	device = nullptr;
		GDHandle	windowDevice = nullptr;
		Rect		windowRect;
		Rect		areaRect;
		SInt32		area = 0L;
		SInt32		areaMax = 0L;
		
		
		// obtain global structure rectangle of the window
		UNUSED_RETURN(OSStatus)GetWindowBounds(inWindow, kWindowStructureRgn, &windowRect);
		
		// check for monitors in use - find the one containing the largest chunk of the window
		for (device = GetDeviceList(); (device != nullptr); device = GetNextDevice(device))
		{
			if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive))
			{
				UNUSED_RETURN(Boolean)SectRect(&(**device).gdRect, &windowRect, &areaRect);
				
				area = (SInt32)(areaRect.bottom - areaRect.top) * (areaRect.right - areaRect.left);
				if (area > areaMax)
				{
					areaMax = area;
					windowDevice = device;
				}
			}
		}
		if (windowDevice == nullptr) windowDevice = GetMainDevice();
		if ((windowDevice != nullptr) && (*windowDevice != nullptr)) *outRectPtr = (**windowDevice).gdRect;
		if (TestDeviceAttribute(windowDevice, mainScreen)) outRectPtr->top += GetMBarHeight();
	}
}// GetWindowDeviceGrayRect


/*!
Determines the available window positioning
bounds for the device that contains the largest
part of the specified window’s structure region,
and then returns the largest size the window may
have according to the Human Interface Guidelines.
This means that the window is capped at a maximum
size no matter how large the device may be, since
it is useless to make windows thousands of pixels
wide just because the monitor is high resolution.

The current window size is also returned, if
that parameter is not nullptr.

This is useful for implementing the Maximize
flavor of the Zoom command for most kinds of
windows.

(3.0)
*/
void
RegionUtilities_GetWindowMaximumBounds	(WindowRef	inWindow,
										 Rect*		outNewBoundsPtr,
										 Rect*		outOldBoundsPtrOrNull,
										 Boolean	inNoInsets)
{
#if 0
	enum
	{
		kMaximumReasonableWidth = 1152,	// arbitrary; no matter how big the monitor, window can’t be wider
		kMaximumReasonableHeight = 768	// arbitrary; no matter how big the monitor, window can’t be higher
	};
#endif
	Rect	maximumScreenBounds;
	Rect	contentRegionBounds;
	Rect	structureRegionBounds;
	
	
	// determine these so that the thickness of all edges of the window frame can be determined
	UNUSED_RETURN(OSStatus)GetWindowBounds(inWindow, kWindowContentRgn, &contentRegionBounds);
	UNUSED_RETURN(OSStatus)GetWindowBounds(inWindow, kWindowStructureRgn, &structureRegionBounds);
	
	// figure out the largest bounding box the window’s structure region can have
	RegionUtilities_GetPositioningBounds(inWindow, &maximumScreenBounds);
	unless (inNoInsets)
	{
		InsetRect(&maximumScreenBounds, 7, 10); // Aqua Human Interface Guidelines - inset from screen edges by many pixels
	}
	
	// if requested, return the old size as well
	if (outOldBoundsPtrOrNull != nullptr) *outOldBoundsPtrOrNull = contentRegionBounds;
	
	// calculate the new bounding box of the content region of the window
	outNewBoundsPtr->top = maximumScreenBounds.top +
							(contentRegionBounds.top - structureRegionBounds.top);
	outNewBoundsPtr->left = maximumScreenBounds.left +
							(contentRegionBounds.left - structureRegionBounds.left);
	outNewBoundsPtr->bottom = maximumScreenBounds.bottom -
								(contentRegionBounds.bottom - structureRegionBounds.bottom);
	outNewBoundsPtr->right = maximumScreenBounds.right -
								(structureRegionBounds.right - contentRegionBounds.right);
	
#if 0
	// cap the window at an arbitrary maximize size, since for HUGE monitors
	// it is basically useless to make a window fill the entire screen
	if ((outNewBoundsPtr->right - outNewBoundsPtr->left) > kMaximumReasonableWidth)
	{
		outNewBoundsPtr->right = outNewBoundsPtr->left + kMaximumReasonableWidth;
	}
	if ((outNewBoundsPtr->bottom - outNewBoundsPtr->top) > kMaximumReasonableHeight)
	{
		outNewBoundsPtr->bottom = outNewBoundsPtr->top + kMaximumReasonableHeight;
	}
#endif
}// GetWindowMaximumBounds


/*!
Returns true only if the specified points are equal or
“close”, as defined by the user interface guidelines
for the amount the mouse can move in any direction and
still be considered in the same place (e.g. for
processing double-clicks).

(3.0)
*/
Boolean
RegionUtilities_NearPoints	(Point	inPoint1,
							 Point	inPoint2)
{
	Boolean		result = ((inPoint1.h == inPoint2.h) && (inPoint1.v == inPoint2.v));
	
	
	unless (result)
	{
		Rect	rect;
		
		
		Pt2Rect(inPoint1, inPoint2, &rect);
		result = (((rect.right - rect.left) < 5) && ((rect.bottom - rect.top) < 5));
	}
	return result;
}// NearPoints


/*!
A replacement for code depending on the old QuickDraw
method OffsetRect().

DEPRECATED.  Transitional only.  Allows for SDK migration.

(2016.08)
*/
void
RegionUtilities_OffsetRect	(Rect*		inoutRectPtr,
							 SInt16		inDeltaX,
							 SInt16		inDeltaY)
{
	inoutRectPtr->left += inDeltaX;
	inoutRectPtr->right += inDeltaX;
	inoutRectPtr->top += inDeltaY;
	inoutRectPtr->bottom += inDeltaY;
}// OffsetRect


/*!
A replacement for code depending on the old QuickDraw
method SetPt().

DEPRECATED.  Transitional only.  Allows for SDK migration.

(2016.08)
*/
void
RegionUtilities_SetPoint		(Point*		inoutPointPtr,
							 SInt16		inX,
							 SInt16		inY)
{
	inoutPointPtr->h = inX;
	inoutPointPtr->v = inY;
}// SetPoint


/*!
A replacement for code depending on the old QuickDraw
method SetRect().

DEPRECATED.  Transitional only.  Allows for SDK migration.

(2016.08)
*/
void
RegionUtilities_SetRect	(Rect*		inoutRectPtr,
						 SInt16		inLeft,
						 SInt16		inTop,
						 SInt16		inRight,
						 SInt16		inBottom)
{
	inoutRectPtr->left = inLeft;
	inoutRectPtr->top = inTop;
	inoutRectPtr->right = inRight;
	inoutRectPtr->bottom = inBottom;
}// SetRect

// BELOW IS REQUIRED NEWLINE TO END FILE
