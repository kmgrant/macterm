/*!	\file PopoverManager.objc++.h
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

	Interface Library
	© 1998-2022 by Kevin Grant
	
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

#pragma once

// Mac includes
@import Cocoa;

// library includes
#import <Popover.objc++.h>



#pragma mark Constants

enum PopoverManager_AnimationType
{
	kPopoverManager_AnimationTypeStandard	= 0,	//!< open with balloon, close with fade-out
	kPopoverManager_AnimationTypeMinimal	= 1,	//!< open and close with fade
	kPopoverManager_AnimationTypeNone		= 2		//!< open and close without animation
};

enum PopoverManager_BehaviorType
{
	kPopoverManager_BehaviorTypeStandard	= 0,	//!< popover can be implicitly dismissed
	kPopoverManager_BehaviorTypeDialog		= 1,	//!< popover can never be implicitly dismissed
	kPopoverManager_BehaviorTypeFloating	= 2		//!< popover remains displayed above most other elements
};

#pragma mark Types

#ifdef __OBJC__

/*!
Manages popover behavior over a particular parent view.
*/
@interface PopoverManager_WC : NSWindowController< Popover_ResizeDelegate > @end


// This is defined as an Objective-C object so it is compatible
// with ARC rules (e.g. strong references).
typedef PopoverManager_WC*		PopoverManager_Ref;


/*!
Classes that are passed as delegates to PopoverManager_New()
must conform to this protocol.
*/
@protocol PopoverManager_Delegate < NSObject > //{

@required

	// return the proper position of the popover arrow tip (if any), relative
	// to its parent window; also called during window resizing
	- (NSPoint)
	popoverManager:(PopoverManager_Ref _Nonnull)_
	idealAnchorPointForFrame:(NSRect)_
	parentWindow:(NSWindow* _Nullable)_;

	// return the desired popover arrow placement
	- (Popover_Properties)
	popoverManager:(PopoverManager_Ref _Nonnull)_
	idealArrowPositionForFrame:(NSRect)_
	parentWindow:(NSWindow* _Nullable)_;

	// provide initial dimensions for popover
	- (void)
	popoverManager:(PopoverManager_Ref _Nonnull)_
	getIdealSize:(NSSize* _Nonnull)_;

@optional

	// provide YES for one or both axes that should allow resizing to take place;
	// if not implemented, the assumption is that the window can resize both ways
	- (void)
	popoverManager:(PopoverManager_Ref _Nonnull)_
	getHorizontalResizeAllowed:(BOOL* _Nonnull)_
	getVerticalResizeAllowed:(BOOL* _Nonnull)_;

@end //}

#else

class PopoverManager_WC;
typedef PopoverManager_WC*		PopoverManager_Ref;

#endif // __OBJC__



#pragma mark Public Methods

PopoverManager_Ref _Nullable
	PopoverManager_New							(Popover_Window* _Nonnull					inPopover,
												 NSView* _Nonnull							inLogicalFirstResponder,
												 id< PopoverManager_Delegate > _Nullable	inDelegate,
												 PopoverManager_AnimationType				inAnimation,
												 PopoverManager_BehaviorType				inBehavior,
												 NSView* _Nullable							inParentView);

void
	PopoverManager_DisplayPopover				(PopoverManager_Ref _Nonnull	inRef);

void
	PopoverManager_RemovePopover				(PopoverManager_Ref _Nonnull	inRef,
												 Boolean						inIsConfirming);

void
	PopoverManager_SetAnimationType				(PopoverManager_Ref _Nonnull	inRef,
												 PopoverManager_AnimationType	inAnimation);

void
	PopoverManager_SetBehaviorType				(PopoverManager_Ref _Nonnull	inRef,
												 PopoverManager_BehaviorType	inBehavior);

void
	PopoverManager_UseIdealLocationAfterDelay	(PopoverManager_Ref _Nonnull	inRef,
												 Float32						inDelay);

// BELOW IS REQUIRED NEWLINE TO END FILE
