/*!	\file PopoverManager.h
	\brief Provides common support code that is generally
	needed by any window that acts like a popover.
	
	This module takes care of details like responding to
	parent window changes (e.g. relocating the popover
	as the parent resizes) and automatically hiding the
	popover in certain situations.  It also handles any
	animation.
	
	You typically start by creating a Popover_Window
	with the required views, and then using this routine
	to specify that window and its parent.  Once you use
	this interface to show the popover, its behavior is
	largely managed for you.  You may also force the
	popover to be hidden through this interface, so that
	you can use consistent animation in that case.
	
	Currently this module supports Cocoa-based popovers
	on top of Carbon-based windows only.  In the future,
	as needed, it will make sense to support Cocoa parent
	windows too.
*/
/*###############################################################

	Interface Library 2.7
	© 1998-2012 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
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

#ifndef __POPOVERMANAGER__
#define __POPOVERMANAGER__

// Mac includes
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

// library includes
#import <Popover.objc++.h>



#pragma mark Constants

enum PopoverManager_AnimationType
{
	kPopoverManager_AnimationTypeStandard	= 0,	//!< open with balloon, close with fade-out
	kPopoverManager_AnimationTypeMinimal	= 1,	//!< open and close with fade
	kPopoverManager_AnimationTypeNone		= 2		//!< open and close without animation
};

#pragma mark Types

typedef struct PopoverManager_OpaqueStruct*		PopoverManager_Ref;

@protocol PopoverManager_Delegate

// return the proper position of the popover arrow tip (if any), relative
// to its parent window; also called during window resizing
- (NSPoint)
idealAnchorPointForParentWindowFrame:(NSRect)_;

// return the desired popover arrow placement
- (Popover_Properties)
idealArrowPositionForParentWindowFrame:(NSRect)_;

// return the dimensions the popover should initially have
- (NSSize)
idealSize;

@end // PopoverManager_Delegate



#pragma mark Public Methods

PopoverManager_Ref
	PopoverManager_New							(Popover_Window*				inPopover,
												 NSView*						inLogicalFirstResponder,
												 id< PopoverManager_Delegate >	inDelegate,
												 PopoverManager_AnimationType	inAnimation,
												 NSWindow*						inParentWindow);

PopoverManager_Ref
	PopoverManager_New							(Popover_Window*				inPopover,
												 NSView*						inLogicalFirstResponder,
												 id< PopoverManager_Delegate >	inDelegate,
												 PopoverManager_AnimationType	inAnimation,
												 HIWindowRef					inParentWindow);

void
	PopoverManager_Dispose						(PopoverManager_Ref*			inoutRefPtr);

void
	PopoverManager_DisplayPopover				(PopoverManager_Ref				inRef);

void
	PopoverManager_RemovePopover				(PopoverManager_Ref				inRef);

void
	PopoverManager_UseIdealLocationAfterDelay	(PopoverManager_Ref				inRef,
												 Float32						inDelay);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
