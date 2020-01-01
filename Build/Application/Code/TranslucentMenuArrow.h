/*!	\file TranslucentMenuArrow.h
	\brief A menu view that can appear to integrate
	into unusual backgrounds, such as window frames.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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
#endif



#pragma mark Types

#ifdef __OBJC__

/*!
A transparent menu arrow view can be used as the content view
of a frameless window to give the illusion of a menu that is
attached to something (such as the window title bar).  The
background color changes slightly when the mouse hovers over
the view.

Once the frameless window’s content view is being rendered by
this view, construct a transparent pop-up menu with the desired
content and make it an immediate subclass of the view (sized to
stay in sync with the frameless window).  Then, any clicks will
pop up a menu that appears to be coming from the translucent
menu arrow view.
*/
@interface TranslucentMenuArrow_View : NSView //{
{
@private
	NSTrackingRectTag	trackingRectTag;
	BOOL				hoverState;
}

// initializers
	- (instancetype)
	initWithFrame:(NSRect)_;

@end //}

#endif // __OBJC__

// BELOW IS REQUIRED NEWLINE TO END FILE
