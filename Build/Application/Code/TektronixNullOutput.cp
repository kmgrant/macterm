/*###############################################################

	TektronixNullOutput.c
	
	MacTelnet
		© 1998-2003 by Kevin Grant.
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
#include "TektronixNullOutput.h"



//
// public methods
//

/*!
Does absolutely nothing - placeholder for a
“new window” TEK callback.

\retval 0
always

(2.6)
*/
SInt16
TektronixNullOutput_New ()
{
	// DO NOTHING!
	return 0;
}// New


/*!
Does absolutely nothing - placeholder for a
“dispose window” TEK callback.

\retval 0
always

(2.6)
*/
SInt16
TektronixNullOutput_Dispose		(SInt16		UNUSED_ARGUMENT(inDevice))
{
	// DO NOTHING!
	return 0;
}// Dispose


/*!
Does absolutely nothing - placeholder for a
“data line” TEK callback.

(2.6)
*/
void
TektronixNullOutput_DataLine	(SInt16		UNUSED_ARGUMENT(inDevice),
								 SInt16		UNUSED_ARGUMENT(inData),
								 SInt16		UNUSED_ARGUMENT(inCount))
{
	// DO NOTHING!
}// DataLine


/*!
Does absolutely nothing, takes a single short
integer argument and returns nothing.  Useful
as a placeholder in TEK callback lists.

(2.6)
*/
void
TektronixNullOutput_DoNothingIntArgReturnVoid	(SInt16		UNUSED_ARGUMENT(inUnused))
{
	// DO NOTHING!
}// DoNothingIntArgReturnVoid


/*!
Does absolutely nothing, takes a single short
integer argument and returns 0.  Useful as a
placeholder in TEK callback lists.

\retval 0
always

(2.6)
*/
SInt16
TektronixNullOutput_DoNothingIntArgReturnZero	(SInt16		UNUSED_ARGUMENT(inUnused))
{
	// DO NOTHING!
	return 0;
}// DoNothingIntArgReturnZero


/*!
Does absolutely nothing, takes no arguments
and returns nothing.  Useful as a placeholder
in TEK callback lists.

(2.6)
*/
void
TektronixNullOutput_DoNothingNoArgReturnVoid ()
{
	// DO NOTHING!
}// DoNothingNoArgReturnVoid


/*!
Does absolutely nothing - placeholder for a
“draw dot” TEK callback.

\retval 0
always

(2.6)
*/
SInt16
TektronixNullOutput_DrawDot		(SInt16		UNUSED_ARGUMENT(inDevice),
								 SInt16		UNUSED_ARGUMENT(inX),
								 SInt16		UNUSED_ARGUMENT(inY))
{
	// DO NOTHING!
	return 0;
}// DrawDot


/*!
Does absolutely nothing - placeholder for a
“draw line” TEK callback.

\retval 0
always

(2.6)
*/
SInt16
TektronixNullOutput_DrawLine	(SInt16		UNUSED_ARGUMENT(inDevice),
								 SInt16		UNUSED_ARGUMENT(inStartX),
								 SInt16		UNUSED_ARGUMENT(inStartY),
								 SInt16		UNUSED_ARGUMENT(inEndX),
								 SInt16		UNUSED_ARGUMENT(inEndY))
{
	// DO NOTHING!
	return 0;
}// DrawLine


/*!
Does absolutely nothing and returns nullptr -
placeholder for a “get device name” TEK
callback.

\retval nullptr
always

(2.6)
*/
char const*
TektronixNullOutput_GetDeviceName ()
{
	// DO NOTHING!
	return nullptr;
}// GetDeviceName


/*!
Does absolutely nothing - placeholder for a
“set callback information” TEK callback.

(2.6)
*/
void
TektronixNullOutput_SetCallbackData		(SInt16		UNUSED_ARGUMENT(inDevice),
										 SInt16		UNUSED_ARGUMENT(inTektronixVirtualGraphicsRef),
										 SInt16		UNUSED_ARGUMENT(inData2),
										 SInt16		UNUSED_ARGUMENT(inData3),
										 SInt16		UNUSED_ARGUMENT(inData4),
										 SInt16		UNUSED_ARGUMENT(inData5))
{
	// DO NOTHING!
}// SetCallbackData


void
TektronixNullOutput_SetCharacterMode	(SInt16		UNUSED_ARGUMENT(inDevice),
										 SInt16		UNUSED_ARGUMENT(inRotation),
										 SInt16		UNUSED_ARGUMENT(inSize))
{
	// DO NOTHING!
}// SetCharacterMode


/*!
Does absolutely nothing - placeholder for a
“set pen color” TEK callback.

\retval 0
always

(2.6)
*/
SInt16
TektronixNullOutput_SetPenColor		(SInt16		UNUSED_ARGUMENT(inDevice),
									 SInt16		UNUSED_ARGUMENT(inColorIndex))
{
	// DO NOTHING!
	return 0;
}// SetPenColor

// BELOW IS REQUIRED NEWLINE TO END FILE
