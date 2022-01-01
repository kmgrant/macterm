/*!	\file RegionUtilities.h
	\brief A powerful set of rectangle, region and window
	utilities.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2022 by Kevin Grant
	
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
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

void
	RegionUtilities_AddRoundedRectangleToPath	(CGContextRef		inContext,
												 CGRect const&		inFrame,
												 float				inCurveWidth,
												 float				inCurveHeight);

void
	RegionUtilities_CenterCGRectIn				(CGRect&			inoutInner,
												 CGRect const&		inOuter);

// BELOW IS REQUIRED NEWLINE TO END FILE
