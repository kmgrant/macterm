/*!	\file VectorCanvas.h
	\brief Renders vector graphics, onscreen or offscreen.
	
	Based on work by Gaige B. Paulsen and Aaron Contorer.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#include <UniversalDefines.h>

#ifndef __VECTORCANVAS__
#define __VECTORCANVAS__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// application includes
#include "SessionRef.typedef.h"
#include "VectorInterpreterID.typedef.h"



#pragma mark Constants

enum VectorCanvas_Target
{
	kVectorCanvas_TargetScreenPixels		= 0,	//!< canvas will open a new TEK window
	kVectorCanvas_TargetQuickDrawPicture	= 1		//!< canvas will target a QuickDraw picture
};

#pragma mark Types

typedef struct VectorCanvas_OpaqueStruct*	VectorCanvas_Ref;



#pragma mark Public Methods

//!\name Initialization
//@{

void
	VectorCanvas_Init					();

//@}

//!\name Creating and Destroying Vector Graphics Contexts
//@{

VectorCanvas_Ref
	VectorCanvas_New					(VectorInterpreter_ID	inData,
										 VectorCanvas_Target	inTarget);

void
	VectorCanvas_Dispose				(VectorCanvas_Ref*		inoutPtr);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

void
	VectorCanvas_AudioEvent				(VectorCanvas_Ref		inRef);

void
	VectorCanvas_CopyTitle				(VectorCanvas_Ref		inRef,
										 CFStringRef&			outTitle);

void
	VectorCanvas_DisplayWindowRenameUI	(VectorCanvas_Ref		inRef);

SInt16
	VectorCanvas_DrawLine				(VectorCanvas_Ref		inRef,
										 SInt16					inStartX,
										 SInt16					inStartY,
										 SInt16					inEndX,
										 SInt16					inEndY);

void
	VectorCanvas_InvalidateView			(VectorCanvas_Ref		inRef);

SInt16
	VectorCanvas_MonitorMouse			(VectorCanvas_Ref		inRef);

VectorCanvas_Ref
	VectorCanvas_ReturnFromWindow		(HIWindowRef			inWindow);

PicHandle
	VectorCanvas_ReturnNewQuickDrawPicture	(VectorInterpreter_ID	inGraphicID);

HIWindowRef
	VectorCanvas_ReturnWindow			(VectorCanvas_Ref		inRef);

VectorInterpreter_ID
	VectorCanvas_ReturnInterpreterID	(VectorCanvas_Ref		inRef);

SessionRef
	VectorCanvas_ReturnListeningSession	(VectorCanvas_Ref		inRef);

SInt16
	VectorCanvas_SetBounds				(Rect const*			inBoundsPtr);

SInt16
	VectorCanvas_SetPenColor			(VectorCanvas_Ref		inRef,
										 SInt16					inColor);

//@}

//!\name Miscellaneous
//@{

SInt16
	VectorCanvas_SetListeningSession	(VectorCanvas_Ref		inRef,
										 SessionRef				inSession);

SInt16
	VectorCanvas_SetTitle				(VectorCanvas_Ref		inRef,
										 CFStringRef			inTitle);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
