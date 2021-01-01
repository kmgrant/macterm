/*!	\file RegionUtilities.cp
	\brief A powerful set of rectangle, region and window
	utilities.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2021 by Kevin Grant
	
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
RegionUtilities_CenterCGRectIn	(CGRect&		inoutInner,
								 CGRect const&	inOuter)
{
	// the algorithm offsets the first rectangle based on the difference between
	// the center points of the 2nd and 1st rectangles
	inoutInner = CGRectOffset(inoutInner, CGRectGetMidX(inOuter) - CGRectGetMidX(inoutInner),
								CGRectGetMidY(inOuter) - CGRectGetMidY(inoutInner));
}// CenterCGRectIn

// BELOW IS REQUIRED NEWLINE TO END FILE
