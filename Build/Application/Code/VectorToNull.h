/*!	\file VectorToNull.h
	\brief Routines for "null" device - calling them has no
	effect, but they are compatible with all normal graphics
	callbacks.
	
	Originally created by Aaron Contorer in 1987 for NCSA.
*/
/*###############################################################

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

#ifndef __VECTORTONULL__
#define __VECTORTONULL__



#pragma mark Public Methods

//!\name Creating and Destroying Vector Graphics Contexts
//@{

SInt16
	VectorToNull_New						();

SInt16
	VectorToNull_Dispose					(SInt16				inDevice);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

void
	VectorToNull_DataLine					(SInt16				inDevice,
											 SInt16				inData,
											 SInt16				inCount);

SInt16
	VectorToNull_DrawDot					(SInt16				inDevice,
											 SInt16				inX,
											 SInt16				inY);

SInt16
	VectorToNull_DrawLine					(SInt16				inDevice,
											 SInt16				inStartX,
											 SInt16				inStartY,
											 SInt16				inEndX,
											 SInt16				inEndY);

char const*
	VectorToNull_ReturnDeviceName			();

void
	VectorToNull_SetCallbackData			(SInt16				inDevice,
											 SInt16				inTektronixVirtualGraphicsRef,
											 SInt16				inData2,
											 SInt16				inData3,
											 SInt16				inData4,
											 SInt16				inData5);

SInt16
	VectorToNull_SetBounds					(Rect const*		inBoundsPtr);

void
	VectorToNull_SetCharacterMode			(SInt16				inDevice,
											 SInt16				inRotation,
											 SInt16				inSize);

SInt16
	VectorToNull_SetPenColor				(SInt16				inDevice,
											 SInt16				inColorIndex);

//@}

//!\name Generic Do-Nothing Routines
//@{

void
	VectorToNull_DoNothingIntArgReturnVoid	(SInt16				inUnused);

void
	VectorToNull_DoNothingNoArgReturnVoid	();

short
	VectorToNull_DoNothingIntArgReturnZero	(SInt16				inUnused);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
