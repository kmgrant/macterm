/*!	\file VectorWindow.h
	\brief A window with a vector graphics canvas.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#pragma once

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSWindow;
#endif

// library includes
#include <ListenerModel.h>
#include <ResultCode.template.h>
#ifdef __OBJC__
#	include <TouchBar.objc++.h>
#endif
#include <WindowTitleDialog.h>

// application includes
#include "Commands.h"
#include "SessionRef.typedef.h"
#include "TerminalView.h"
#ifdef __OBJC__
@class VectorCanvas_View;
#endif
#include "VectorInterpreterRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from Vector Window module routines.
*/
typedef ResultCode< UInt16 >	VectorWindow_Result;
VectorWindow_Result const	kVectorWindow_ResultOK(0);					//!< no error
VectorWindow_Result const	kVectorWindow_ResultInvalidReference(1);	//!< given VectorWindow_Ref is not valid
VectorWindow_Result const	kVectorWindow_ResultParameterError(2);		//!< invalid input (e.g. a null pointer)

/*!
Events that other modules may “listen” for, via
VectorWindow_StartMonitoring().
*/
typedef ListenerModel_Event		VectorWindow_Event;
enum
{
	kVectorWindow_EventWillClose		= 'Clos'	//!< window is about to close (context: VectorWindow_Ref)
};

#pragma mark Types

#include "VectorWindowRef.typedef.h"

#ifdef __OBJC__

/*!
Implements a Cocoa window to display a vector graphics
canvas.  See "VectorWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface VectorWindow_Controller : NSWindowController < Commands_WindowRenaming,
															TerminalView_ClickDelegate > //{
{
	IBOutlet TerminalView_BackgroundView*	matteView;
	IBOutlet VectorCanvas_View*				canvasView;
@private
	VectorWindow_Ref		selfRef;
	ListenerModel_Ref		changeListenerModel;
	VectorInterpreter_Ref	interpreterRef;
	WindowTitleDialog_Ref	renameDialog;
	TouchBar_Controller*		_touchBarController;
}

// initializers
	- (instancetype)
	initWithInterpreter:(VectorInterpreter_Ref)_;

// accessors
	- (VectorCanvas_View*)
	canvasView;
	- (void)
	setCanvasView:(VectorCanvas_View*)_;

@end //}

#else

class VectorWindow_Controller;

#endif // __OBJC__



#pragma mark Public Methods

//!\name Creating and Destroying Vector Graphics Windows
//@{

VectorWindow_Ref
	VectorWindow_New					(VectorInterpreter_Ref		inData);

void
	VectorWindow_Retain					(VectorWindow_Ref			inRef);

void
	VectorWindow_Release				(VectorWindow_Ref*			inoutRefPtr);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

VectorWindow_Controller*
	VectorWindow_ReturnController		(VectorWindow_Ref			inWindow);

VectorInterpreter_Ref
	VectorWindow_ReturnInterpreter		(VectorWindow_Ref			inWindow);

//@}

//!\name Cocoa View and Window Management
//@{

VectorWindow_Result
	VectorWindow_Display				(VectorWindow_Ref			inWindow);

NSWindow*
	VectorWindow_ReturnNSWindow			(VectorWindow_Ref			inWindow);

//@}

//!\name Event Monitoring
//@{

VectorWindow_Result
	VectorWindow_StartMonitoring		(VectorWindow_Ref			inWindow,
										 VectorWindow_Event			inForWhichEvent,
										 ListenerModel_ListenerRef	inListener);

VectorWindow_Result
	VectorWindow_StopMonitoring			(VectorWindow_Ref			inWindow,
										 VectorWindow_Event			inForWhichEvent,
										 ListenerModel_ListenerRef	inListener);

//@}

//!\name Miscellaneous
//@{

void
	VectorWindow_CopyTitle				(VectorWindow_Ref			inWindow,
										 CFStringRef&				outTitle);

VectorWindow_Result
	VectorWindow_SetTitle				(VectorWindow_Ref			inWindow,
										 CFStringRef				inTitle);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
