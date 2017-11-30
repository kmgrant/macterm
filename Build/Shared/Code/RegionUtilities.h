/*!	\file RegionUtilities.h
	\brief A powerful set of rectangle, region and window
	utilities.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2017 by Kevin Grant
	
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

#pragma once

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

void
	RegionUtilities_AddRoundedRectangleToPath	(CGContextRef		inContext,
												 CGRect const&		inFrame,
												 float				inCurveWidth,
												 float				inCurveHeight);

void
	RegionUtilities_CenterHIRectIn				(HIRect&			inoutInner,
												 HIRect const&		inOuter);

void
	RegionUtilities_CenterRectIn				(Rect*				inoutInner,
												 Rect const*		inOuter);

Boolean
	RegionUtilities_EqualRects					(Rect const*			inRect1Ptr,
												 Rect const*			inRect2Ptr);

void
	RegionUtilities_GetPositioningBounds		(WindowRef			inWindow,
												 Rect*				outRectPtr);

// RETURNS THE RECTANGLE OF THE DEVICE CONTAINING THE LARGEST PART OF THE GIVEN WINDOW
void
	RegionUtilities_GetWindowDeviceGrayRect		(WindowRef			inWindow,
												 Rect*				outRectPtr);

Boolean
	RegionUtilities_GetWindowDirectDisplayID	(WindowRef			inWindow,
												 CGDirectDisplayID&	outLargestAreaDeviceID);

void
	RegionUtilities_GetWindowMaximumBounds		(WindowRef			inWindow,
												 Rect*				outNewBoundsPtr,
												 Rect*				outOldBoundsPtrOrNull,
												 Boolean			inNoInsets = false);

Boolean
	RegionUtilities_NearPoints					(Point				inPoint1,
												 Point				inPoint2);

void
	RegionUtilities_OffsetRect					(Rect*				inoutRectPtr,
												 SInt16				inDeltaX,
												 SInt16				inDeltaY);

void
	RegionUtilities_SetPoint						(Point*				inoutPointPtr,
												 SInt16				inX,
												 SInt16				inY);

void
	RegionUtilities_SetRect						(Rect*				inoutRectPtr,
												 SInt16				inLeft,
												 SInt16				inTop,
												 SInt16				inRight,
												 SInt16				inBottom);

// BELOW IS REQUIRED NEWLINE TO END FILE
