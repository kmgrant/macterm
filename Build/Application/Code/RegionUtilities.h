/*!	\file RegionUtilities.h
	\brief A powerful set of rectangle, region and window
	utilities.
*/
/*###############################################################

	Interface Library 1.3
	© 1998-2006 by Kevin Grant
	
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

#include "UniversalDefines.h"

#ifndef __REGIONUTILITIES__
#define __REGIONUTILITIES__

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

#define CenterRectIn(i,o)	RegionUtilities_CenterRectIn(i,o)
void
	RegionUtilities_CenterRectIn				(Rect*				inoutInner,
												 Rect const*		inOuter);

void
	RegionUtilities_GetPositioningBounds		(WindowRef			inWindow,
												 Rect*				outRectPtr);

// RETURNS THE RECTANGLE OF THE DEVICE CONTAINING THE LARGEST PART OF THE GIVEN WINDOW
void
	RegionUtilities_GetWindowDeviceGrayRect		(WindowRef			inWindow,
												 Rect*				outRectPtr);

void
	RegionUtilities_GetWindowMaximumBounds		(WindowRef			inWindow,
												 Rect*				outNewBoundsPtr,
												 Rect*				outOldBoundsPtrOrNull,
												 Boolean			inNoInsets = false);

#define LocalToGlobalRegion(r)	RegionUtilities_LocalToGlobal(r)
void
	RegionUtilities_LocalToGlobal				(RgnHandle			inoutRegion);

Boolean
	RegionUtilities_NearPoints					(Point				inPoint1,
												 Point				inPoint2);

void
	RegionUtilities_RedrawControlOnNextUpdate	(ControlRef			inControl);

void
	RegionUtilities_RedrawWindowOnNextUpdate	(WindowRef			inWindow);

void
	RegionUtilities_SetControlUpToDate			(ControlRef			inControl);

void
	RegionUtilities_SetWindowUpToDate			(WindowRef			inWindow);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
