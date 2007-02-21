/*!	\file EventInfoWindowScope.h
	\brief Data structures used as contexts for main event loop
	events specific to one window.
	
	Defined in a separate file to better-organize the data and
	to reduce the number of files depending on one header.
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

#ifndef __EVENTINFOWINDOWSCOPE__
#define __EVENTINFOWINDOWSCOPE__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

/*!
Data structure providing the context for event
handlers notified about the following:

kEventLoop_WindowEventContentTrack
*/
struct EventInfoWindowScope_ContentClick
{
	WindowRef			window;				//!< which active windowÕs content region was touched
	EventRecord			event;				//!< information on the click
};
typedef EventInfoWindowScope_ContentClick*		EventInfoWindowScope_ContentClickPtr;

/*!
Data structure providing the context for event
handlers notified about the following:

kEventLoop_WindowEventKeyPress
*/
struct EventInfoWindowScope_KeyPress
{
	WindowRef			window;				//!< which window contains the keyboard focus control
	EventRecord			event;				//!< information on the key press; DEPRECATED, avoid using if possible
	SInt16				characterCode;		//!< code uniquifying the character corresponding to the key pressed
	SInt16				virtualKeyCode;		//!< code uniquifying the key pressed
	Boolean				commandDown;		//!< the state of the Command modifier key
	Boolean				controlDown;		//!< the state of the Control modifier key
	Boolean				optionDown;			//!< the state of the Option modifier key
	Boolean				shiftDown;			//!< the state of the Shift modifier key
};
typedef EventInfoWindowScope_KeyPress*		EventInfoWindowScope_KeyPressPtr;

/*!
Data structure providing the context for event
handlers notified about the following:

kEventLoop_WindowEventScrolling
*/
struct EventInfoWindowScope_Scroll
{
	WindowRef			window;				//!< which window contains the scroll bar control(s)
	
	// a scroll event might be bi-directional, so check the "affected" flags in each structure
	
	struct
	{
		ControlPartCode		scrollBarPartCode;	//!< information on the scroll direction
		Boolean				affected;			//!< was the event for this type of scroll bar?
	} horizontal;
	
	struct
	{
		ControlPartCode		scrollBarPartCode;	//!< information on the scroll direction
		Boolean				affected;			//!< was the event for this type of scroll bar?
	} vertical;
};
typedef EventInfoWindowScope_Scroll*	EventInfoWindowScope_ScrollPtr;

/*!
Data structure providing the context for event
handlers notified about the following:

kEventLoop_WindowEventUpdate
*/
struct EventInfoWindowScope_Update
{
	WindowRef			window;				//!< the window to update
	RgnHandle			visibleRegion;		//!< the part of the window that is visible
	CGrafPtr			drawingPort;		//!< the graphics port where update drawing should be done
	GDHandle			drawingDevice;		//!< the device whose characteristics should influence drawing
};
typedef EventInfoWindowScope_Update*	EventInfoWindowScope_UpdatePtr;

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
