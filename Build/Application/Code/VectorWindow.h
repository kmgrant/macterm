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

// initializers
	- (instancetype _Nullable)
	initWithInterpreter:(VectorInterpreter_Ref _Nonnull)_
	owner:(VectorWindow_Ref _Nonnull)_;

// accessors
	//! The primary vector graphics display.
	@property (strong, nonnull) IBOutlet VectorCanvas_View*
	canvasView;
	//! The colored border display.
	@property (strong, nonnull) IBOutlet TerminalView_BackgroundView*
	matteView;
	//! The Terminal Window object that owns this window controller.
	@property (assign, nullable) VectorWindow_Ref
	vectorWindowRef;

@end //}

#else

class VectorWindow_Controller;

#endif // __OBJC__



#pragma mark Public Methods

//!\name Creating and Destroying Vector Graphics Windows
//@{

VectorWindow_Ref _Nullable
	VectorWindow_New					(VectorInterpreter_Ref _Nonnull			inData);

void
	VectorWindow_Release				(VectorWindow_Ref _Nullable* _Nonnull	inoutRefPtr);

//@}

//!\name Manipulating Vector Graphics Pictures
//@{

VectorWindow_Controller* _Nullable
	VectorWindow_ReturnController		(VectorWindow_Ref _Nullable		inWindow);

VectorInterpreter_Ref _Nullable
	VectorWindow_ReturnInterpreter		(VectorWindow_Ref _Nullable		inWindow);

//@}

//!\name Cocoa View and Window Management
//@{

VectorWindow_Result
	VectorWindow_Display				(VectorWindow_Ref _Nullable		inWindow);

NSWindow* _Nullable
	VectorWindow_ReturnNSWindow			(VectorWindow_Ref _Nullable		inWindow);

//@}

//!\name Event Monitoring
//@{

VectorWindow_Result
	VectorWindow_StartMonitoring		(VectorWindow_Ref _Nullable				inWindow,
										 VectorWindow_Event						inForWhichEvent,
										 ListenerModel_ListenerRef _Nonnull		inListener);

VectorWindow_Result
	VectorWindow_StopMonitoring			(VectorWindow_Ref _Nullable				inWindow,
										 VectorWindow_Event						inForWhichEvent,
										 ListenerModel_ListenerRef _Nonnull		inListener);

//@}

//!\name Miscellaneous
//@{

VectorWindow_Result
	VectorWindow_SetTitle				(VectorWindow_Ref _Nullable		inWindow,
										 CFStringRef _Nullable			inTitle);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
