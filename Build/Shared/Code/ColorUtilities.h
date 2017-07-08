/*!	\file ColorUtilities.h
	\brief Various routines commonly needed for graphics.
	Note that some of this behavior has now been made
	obsolete by Cocoa’s core abilities and the NSColor
	extensions added by the Cocoa Extensions module.
*/
/*###############################################################

	Interface Library
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

//!\name Core Graphics Helpers
//@{

// DEPRECATED
CGDeviceColor
	ColorUtilities_CGDeviceColorMake			(RGBColor const&		inColor);

// DEPRECATED
RGBColor
	ColorUtilities_QuickDrawColorMake			(CGDeviceColor const&	inColor);

//@}

//!\name Extremely Handy QuickDraw Port Color Manipulators
//@{

// DEPRECATED
void
	GetLighterColors							(RGBColor*				outLighterForegroundColorOrNull,
												 RGBColor*				outLighterBackgroundColorOrNull);

// DEPRECATED
void
	UseInactiveColors							();

// DEPRECATED
void
	UseLighterColors							();

//@}

//!\name Changing the Pen or Background to a Standard Pattern
//@{

// DEPRECATED
void
	ColorUtilities_SetBlackPenPattern			();

// DEPRECATED
void
	ColorUtilities_SetGrayPenPattern			();

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
