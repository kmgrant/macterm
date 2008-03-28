/*###############################################################

	VectorToBitmap.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// MacTelnet includes
#include "TektronixMacPictureOutput.h"



#pragma mark Constants
namespace {

enum
{
	INXMAX = 4096,
	INYMAX = 4096
};

} // anonymous namespace

#pragma mark Variables
namespace {

Boolean		gRGMPbusy = false;	// is device already in use?
SInt16		gRGMPwidth = 0;
SInt16		gRGMPheight = 0;
SInt16		gRGMPxoffset = 0;
SInt16		gRGMPyoffset = 0;
SInt16		gRGMPcolor[] =
			{
				30,			// black
				33,			// white
				205,		// red
				341,		// green
				409,		// blue
				273,		// cyan
				137,		// magenta
				69			// yellow
			};

} // anonymous namespace


#pragma mark Public Methods

/*!
Initializes this module.

(2.6)
*/
void
VectorToBitmap_Init ()
{
	gRGMPbusy = false;
	//gRGMPwidth = INXMAX;
	//gRGMPheight = INYMAX;
	gRGMPxoffset = 0;
	gRGMPyoffset = 0;
}// Init


/*!
Prepares to create a new Mac picture - this
effectively resets the origin.

(2.6)
*/
SInt16
VectorToBitmap_New ()
{
	SInt16		result = 0;
	
	
	gRGMPbusy = true;
	//gRGMPwidth = INXMAX;
	//gRGMPheight = INYMAX;
	gRGMPxoffset = 0;
	gRGMPyoffset = 0;
	
	return result;
}// New


/*!
Signals the end of a picture created with
VectorToBitmap_New().

\retval 0
always

(2.6)
*/
SInt16
VectorToBitmap_Dispose	(SInt16		UNUSED_ARGUMENT(inDevice))
{
	SInt16		result = 0;
	
	
	gRGMPbusy = false;
	return result;
}// Dispose


/*!
A “data line” callback.  Not sure what this is supposed
to do - but since the default TEK device does not support
this capability, the Mac picture output does not support
it either.

(2.6)
*/
void
VectorToBitmap_DataLine		(SInt16		UNUSED_ARGUMENT(inDevice),
							 SInt16		UNUSED_ARGUMENT(inData),
							 SInt16		UNUSED_ARGUMENT(inCount))
{
}// DataLine


/*!
Renders a single pixel at the specified location
in the given picture.

\retval 0
always

(2.6)
*/
SInt16
VectorToBitmap_DrawDot	(SInt16		UNUSED_ARGUMENT(inDevice),
						 SInt16		inX,
						 SInt16		inY)
{
	SInt16		result = 0;
	
	
	MoveTo(inX, inY);
	LineTo(inX, inY); // Line(1, 1)?
	return result;
}// DrawDot


/*!
Renders a line between the two given points within
the given picture.

\retval 0
always

(2.6)
*/
SInt16
VectorToBitmap_DrawLine	(SInt16		UNUSED_ARGUMENT(inDevice),
						 SInt16		inStartX,
						 SInt16		inStartY,
						 SInt16		inEndX,
						 SInt16		inEndY)
{
	SInt16		result = 0;
	
	
	MoveTo(gRGMPxoffset + (SInt16) (STATIC_CAST(inStartX, SInt32) * gRGMPwidth / INXMAX),
			gRGMPyoffset + gRGMPheight - (SInt16) (STATIC_CAST(inStartY, SInt32) * gRGMPheight / INYMAX));
	LineTo(gRGMPxoffset + (SInt16) (STATIC_CAST(inEndX, SInt32) * gRGMPwidth/INXMAX),
			gRGMPyoffset + gRGMPheight - (SInt16) (STATIC_CAST(inEndY, SInt32) * gRGMPheight / INYMAX));
	return result;
}// DrawLine


/*!
Returns the name of the device rendering
QuickDraw pictures.

(2.6)
*/
char const*
VectorToBitmap_ReturnDeviceName ()
{
	return "Macintosh PICTURE output";
}// ReturnDeviceName


/*!
Sets the boundaries for the given QuickDraw
picture.  The top-left corner can be set to
non-zero values to offset graphics drawings.

(2.6)
*/
SInt16
VectorToBitmap_SetBounds	(Rect const*	inBoundsPtr)
{
	SInt16		result = 0;
	
	
	gRGMPheight= inBoundsPtr->bottom - inBoundsPtr->top;
	gRGMPwidth = inBoundsPtr->right - inBoundsPtr->left;
	gRGMPyoffset = inBoundsPtr->top;
	gRGMPxoffset = inBoundsPtr->left;
	
	return result;
}// SetBounds


/*!
A “set callback information” TEK callback for
setting arbitrary data needed within other
callbacks.

(2.6)
*/
void
VectorToBitmap_SetCallbackData	(SInt16		UNUSED_ARGUMENT(inDevice),
								 SInt16		UNUSED_ARGUMENT(inTektronixVirtualGraphicsRef),
								 SInt16		UNUSED_ARGUMENT(inData2),
								 SInt16		UNUSED_ARGUMENT(inData3),
								 SInt16		UNUSED_ARGUMENT(inData4),
								 SInt16		UNUSED_ARGUMENT(inData5))
{
}// SetCallbackData


/*!
A “character mode” callback.  Apparently sets rotation
and size - but this is not currently supported by this
type of TEK output.

(2.6)
*/
void
VectorToBitmap_SetCharacterMode		(SInt16		UNUSED_ARGUMENT(inDevice),
									 SInt16		UNUSED_ARGUMENT(inRotation),
									 SInt16		UNUSED_ARGUMENT(inSize))
{
}// SetCharacterMode


/*!
Changes the foreground drawing color for the
specified QuickDraw picture.

\retval 0
always

(2.6)
*/
SInt16
VectorToBitmap_SetPenColor	(SInt16		UNUSED_ARGUMENT(inDevice),
							 SInt16		inColorIndex)
{
	SInt16		result = 0;
	
	
	ForeColor(STATIC_CAST(gRGMPcolor[inColorIndex], SInt32));
	return result;
}// SetPenColor

// BELOW IS REQUIRED NEWLINE TO END FILE
