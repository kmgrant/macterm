/*!	\file CocoaAnimation.h
	\brief Animation utilities wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2018 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSWindow;
#else
class NSWindow;
#endif


#pragma mark Types

#ifdef __OBJC__

/*!
If a window conforms to this protocol, the provided image will
be used for animation instead of inferring an image by trying
to render the content view into a buffer.
*/
@protocol CocoaAnimation_WindowImageProvider < NSObject > //{

@required

	// return (autoreleased) image rendering entire frame and content, with alpha
	- (NSImage*)
	windowImage;

@end //}

#endif


#pragma mark Public Methods

//!\name Window Animation
//@{

void
	CocoaAnimation_TransitionWindowForDuplicate				(NSWindow*,
															 NSWindow*);

void
	CocoaAnimation_TransitionWindowForHide					(NSWindow*,
															 CGRect);

void
	CocoaAnimation_TransitionWindowForMove					(NSWindow*,
															 CGRect);

void
	CocoaAnimation_TransitionWindowForRemove				(NSWindow*,
															 Boolean = false);

void
	CocoaAnimation_TransitionWindowForSheetOpen				(NSWindow*,
															 NSWindow*);

void
	CocoaAnimation_TransitionWindowSectionForOpen			(NSWindow*,
															 CGRect);

void
	CocoaAnimation_TransitionWindowSectionForSearchResult	(NSWindow*,
															 CGRect);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
