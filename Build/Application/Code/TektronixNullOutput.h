/*!	\file TektronixNullOutput.h
	\brief Routines for "null" device - calling them has no
	effect, but they are compatible with all normal graphics
	callbacks.
	
	Originally created by Aaron Contorer in 1987 for NCSA.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#ifndef __TEKTRONIXNULLOUTPUT__
#define __TEKTRONIXNULLOUTPUT__



#pragma mark Public Methods

//!\name Creating and Destroying Vector Graphics Contexts
//@{

SInt16
	TektronixNullOutput_New							();

SInt16
	TektronixNullOutput_Dispose						(SInt16				inDevice);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

void
	TektronixNullOutput_DataLine					(SInt16				inDevice,
													 SInt16				inData,
													 SInt16				inCount);

SInt16
	TektronixNullOutput_DrawDot						(SInt16				inDevice,
													 SInt16				inX,
													 SInt16				inY);

SInt16
	TektronixNullOutput_DrawLine					(SInt16				inDevice,
													 SInt16				inStartX,
													 SInt16				inStartY,
													 SInt16				inEndX,
													 SInt16				inEndY);

char const*
	TektronixNullOutput_ReturnDeviceName			();

void
	TektronixNullOutput_SetCallbackData				(SInt16				inDevice,
													 SInt16				inTektronixVirtualGraphicsRef,
													 SInt16				inData2,
													 SInt16				inData3,
													 SInt16				inData4,
													 SInt16				inData5);

void
	TektronixNullOutput_SetCharacterMode			(SInt16				inDevice,
													 SInt16				inRotation,
													 SInt16				inSize);

SInt16
	TektronixNullOutput_SetBounds					(Rect const*		inBoundsPtr);

SInt16
	TektronixNullOutput_SetPenColor					(SInt16				inDevice,
													 SInt16				inColorIndex);

//@}

//!\name Generic Do-Nothing Routines
//@{

void
	TektronixNullOutput_DoNothingIntArgReturnVoid	(SInt16				inUnused);

void
	TektronixNullOutput_DoNothingNoArgReturnVoid	();

short
	TektronixNullOutput_DoNothingIntArgReturnZero	(SInt16				inUnused);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
