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
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSView;
class NSWindow;
#endif

// library includes
#include <ResultCode.template.h>

// application includes
#include "SessionRef.typedef.h"
#include "VectorInterpreterID.typedef.h"



#pragma mark Constants

/*!
Possible return values from Vector Canvas module routines.
*/
typedef ResultCode< UInt16 >	VectorCanvas_Result;
VectorCanvas_Result const	kVectorCanvas_ResultOK(0);					//!< no error
VectorCanvas_Result const	kVectorCanvas_ResultInvalidReference(1);	//!< given VectorCanvas_Ref is not valid
VectorCanvas_Result const	kVectorCanvas_ResultParameterError(2);		//!< invalid input (e.g. a null pointer)

enum VectorCanvas_Target
{
	kVectorCanvas_TargetScreenPixels		= 0,	//!< canvas will open a new TEK window
	kVectorCanvas_TargetQuickDrawPicture	= 1,	//!< canvas will target a QuickDraw picture
	kVectorCanvas_TargetNSImage				= 2		//!< canvas will target an NSImage
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
@interface VectorCanvas_View : NSControl
{
@private
	void*	internalViewPtr;
}
@end


/*!
Implements a Cocoa window to display a vector graphics
canvas.  See "VectorGraphicsWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface VectorCanvas_WindowController : NSWindowController
{
	IBOutlet VectorCanvas_View*		canvasView;
}

@end

#else

class VectorCanvas_View;
class VectorCanvas_WindowController;

#endif // __OBJC__



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

// DEPRECATED
PicHandle
	VectorCanvas_ReturnNewQuickDrawPicture	(VectorInterpreter_ID	inGraphicID);

VectorInterpreter_ID
	VectorCanvas_ReturnInterpreterID	(VectorCanvas_Ref		inRef);

SessionRef
	VectorCanvas_ReturnListeningSession	(VectorCanvas_Ref		inRef);

// DEPRECATED (RESIZE THE VIEW DIRECTLY)
SInt16
	VectorCanvas_SetBounds				(Rect const*			inBoundsPtr);

SInt16
	VectorCanvas_SetPenColor			(VectorCanvas_Ref		inRef,
										 SInt16					inColor);

//@}

//!\name Cocoa NSView and Mac OS HIView Management
//@{

// WILL BE DEPRECATED IN FAVOR OF COCOA
VectorCanvas_Ref
	VectorCanvas_ReturnFromWindow		(HIWindowRef			inWindow);

// WILL BE DEPRECATED IN FAVOR OF COCOA
HIWindowRef
	VectorCanvas_ReturnWindow			(VectorCanvas_Ref		inRef);

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
