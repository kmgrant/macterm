/*!	\file VectorCanvas.h
	\brief Renders vector graphics, onscreen or offscreen.
	
	Based on work by Gaige B. Paulsen and Aaron Contorer.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSView;
#endif

// library includes
#include <ResultCode.template.h>

// application includes
#include "SessionRef.typedef.h"
#include "VectorInterpreterRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from Vector Canvas module routines.
*/
typedef ResultCode< UInt16 >	VectorCanvas_Result;
VectorCanvas_Result const	kVectorCanvas_ResultOK(0);					//!< no error
VectorCanvas_Result const	kVectorCanvas_ResultInvalidReference(1);	//!< given VectorCanvas_Ref is not valid
VectorCanvas_Result const	kVectorCanvas_ResultParameterError(2);		//!< invalid input (e.g. a null pointer)

enum VectorCanvas_PathPurpose
{
	kVectorCanvas_PathPurposeGraphics	= 0,	//!< this is producing a drawing, or an unknown entity
	kVectorCanvas_PathPurposeText		= 1		//!< this is producing some kind of character in a font
};

enum VectorCanvas_PathTarget
{
	kVectorCanvas_PathTargetPrimary		= 0,	//!< the entire vector graphic will be modified
	kVectorCanvas_PathTargetScrap		= 1		//!< only the “scrap” path will be modified (see VectorCanvas_ScrapPathReset())
};

#pragma mark Types

typedef struct VectorCanvas_OpaqueStruct*	VectorCanvas_Ref;

#ifdef __OBJC__

/*!
Implements the main rendering part of the Vector Canvas.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface VectorCanvas_View : NSControl //{
{
@private
	VectorInterpreter_Ref	interpreterRef;
}

// accessors
	- (VectorInterpreter_Ref)
	interpreterRef;
	- (void)
	setInterpreterRef:(VectorInterpreter_Ref)_;

// actions
	- (id)
	canPerformCopy:(id <NSValidatedUserInterfaceItem>)_;
	- (IBAction)
	performCopy:(id)_;
	- (id)
	canPerformPrintScreen:(id <NSValidatedUserInterfaceItem>)_;
	- (IBAction)
	performPrintScreen:(id)_;
	- (id)
	canPerformPrintSelection:(id <NSValidatedUserInterfaceItem>)_;
	- (IBAction)
	performPrintSelection:(id)_;

@end //}

#else

class VectorCanvas_View;

#endif // __OBJC__



#pragma mark Public Methods

//!\name Creating and Destroying Vector Graphics Contexts
//@{

VectorCanvas_Ref
	VectorCanvas_New					(VectorInterpreter_Ref	inData);

void
	VectorCanvas_Retain					(VectorCanvas_Ref		inRef);

void
	VectorCanvas_Release				(VectorCanvas_Ref*		inoutPtr);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

void
	VectorCanvas_AudioEvent				(VectorCanvas_Ref		inRef);

void
	VectorCanvas_ClearCaches			(VectorCanvas_Ref		inRef);

VectorCanvas_Result
	VectorCanvas_DrawLine				(VectorCanvas_Ref		inRef,
										 SInt16					inStartX,
										 SInt16					inStartY,
										 SInt16					inEndX,
										 SInt16					inEndY,
										 VectorCanvas_PathPurpose	inPurpose,
										 VectorCanvas_PathTarget	inTarget = kVectorCanvas_PathTargetPrimary);

void
	VectorCanvas_InvalidateView			(VectorCanvas_Ref		inRef);

SInt16
	VectorCanvas_MonitorMouse			(VectorCanvas_Ref		inRef);

VectorInterpreter_Ref
	VectorCanvas_ReturnInterpreter		(VectorCanvas_Ref		inRef);

VectorCanvas_Result
	VectorCanvas_ScrapPathFill			(VectorCanvas_Ref		inRef,
										 SInt16					inFillColor,
										 Float32				inFrameWidthOrZero = 0.0);

VectorCanvas_Result
	VectorCanvas_ScrapPathReset			(VectorCanvas_Ref		inRef);

VectorCanvas_Result
	VectorCanvas_SetCanvasNSView		(VectorCanvas_Ref		inRef,
										 VectorCanvas_View*		inNSViewBasedRenderer);

VectorCanvas_Result
	VectorCanvas_SetPenColor			(VectorCanvas_Ref		inRef,
										 SInt16					inColor,
										 VectorCanvas_PathPurpose	inPurpose);

//@}

//!\name Session Attachments
//@{

SessionRef
	VectorCanvas_ReturnListeningSession	(VectorCanvas_Ref		inRef);

SInt16
	VectorCanvas_SetListeningSession	(VectorCanvas_Ref			inRef,
										 SessionRef					inSession);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
