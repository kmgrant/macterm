/*!	\file InfoWindow.h
	\brief A window showing status for all sessions.
	
	The window is currently just a table, containing a
	number of columns.  It is slightly interactive; the
	window name can be edited, and double-clicking a row
	will activate that terminal window.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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
#include <ApplicationServices/ApplicationServices.h>
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif
#include <CoreServices/CoreServices.h>

// application includes
#include "SessionRef.typedef.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Implements the window class of InfoWindow_Controller.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface InfoWindow_Object : NSWindow //{
{
}

// initializers
	+ (void)
	initialize;
	- (instancetype)
	initWithContentRect:(NSRect)_
	styleMask:(NSUInteger)_
	backing:(NSBackingStoreType)_
	defer:(BOOL)_ NS_DESIGNATED_INITIALIZER;

@end //}

/*!
Implements the Session Info window.  See "InfoWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface InfoWindow_Controller : NSWindowController< NSToolbarDelegate > //{

// class methods
	+ (instancetype)
	sharedInfoWindowController;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

void
	InfoWindow_Init						();

void
	InfoWindow_Done						();

Boolean
	InfoWindow_IsVisible				();

SessionRef
	InfoWindow_ReturnSelectedSession	();

void
	InfoWindow_SetVisible				(Boolean		inIsVisible);

// BELOW IS REQUIRED NEWLINE TO END FILE
