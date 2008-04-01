/*!	\file VectorCanvas.h
	\brief UI elements for vector graphics screens.
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

#ifndef __VECTORCANVAS__
#define __VECTORCANVAS__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// MacTelnet includes
#include "SessionRef.typedef.h"
#include "tekdefs.h"



#pragma mark Public Methods

//!\name Initialization
//@{

void
	VectorCanvas_Init					();

//@}

//!\name Creating and Destroying Vector Graphics Contexts
//@{

SInt16
	VectorCanvas_New					();

SInt16
	VectorCanvas_Dispose				(SInt16			inCanvasID);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

void
	VectorCanvas_AudioEvent				(SInt16			inCanvasID);

SInt16
	VectorCanvas_ClearScreen			(SInt16			inCanvasID);

void
	VectorCanvas_CursorHide				();

void
	VectorCanvas_CursorLock				();

void
	VectorCanvas_CursorShow				();

void
	VectorCanvas_DataLine				(SInt16			inCanvasID,
										 SInt16			inData,
										 SInt16			inCount);

SInt16
	VectorCanvas_DrawDot				(SInt16			inCanvasID,
										 SInt16			inX,
										 SInt16			inY);

SInt16
	VectorCanvas_DrawLine				(SInt16			inCanvasID,
										 SInt16			inStartX,
										 SInt16			inStartY,
										 SInt16			inEndX,
										 SInt16			inEndY);

void
	VectorCanvas_FinishPage				(SInt16			inCanvasID);

SInt16
	VectorCanvas_MonitorMouse			(SInt16			inCanvasID);

char const*
	VectorCanvas_ReturnDeviceName		();

void
	VectorCanvas_SetCallbackData		(SInt16			inCanvasID,
										 SInt16			inVectorInterpreterRef,
										 SInt16			inData2);

void
	VectorCanvas_SetCharacterMode		(SInt16			inCanvasID,
										 SInt16			inRotation,
										 SInt16			inSize);

void
	VectorCanvas_SetGraphicsMode		();

SInt16
	VectorCanvas_SetPenColor			(SInt16			inCanvasID,
										 SInt16			inColor);

void
	VectorCanvas_SetTextMode			();

void
	VectorCanvas_Uncover				(SInt16			inCanvasID);

//@}

//!\name Miscellaneous
//@{

Boolean
	VectorCanvas_GetFromWindow			(HIWindowRef	inWindow,
										 SInt16*		outDeviceIDPtr);

SInt16
	VectorCanvas_SetListeningSession	(SInt16			inCanvasID,
										 SessionRef		inSession);

SInt16
	VectorCanvas_SetTitle				(SInt16			inCanvasID,
										 CFStringRef	inTitle);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
