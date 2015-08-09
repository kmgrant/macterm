/*!	\file PopoverManager.mm
	\brief Provides common support code that is generally
	needed by any window that acts like a popover.
*/
/*###############################################################

	Interface Library
	© 1998-2015 by Kevin Grant
	
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

#import "PopoverManager.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <Popover.objc++.h>



#pragma mark Types

@interface PopoverManager_Handler : NSWindowController //{
{
@public
	PopoverManager_Ref				selfRef;				// identical to address of structure, but typed as ref
	id< PopoverManager_Delegate >	delegate;				// used to determine dynamic popover information
	Popover_Window*					containerWindow;		// holds the popover itself (note: is an NSWindow subclass)
	NSView*							logicalFirstResponder;	// the view to give initial keyboard focus to, in "display" method
	PopoverManager_AnimationType	animationType;			// specifies how to open and close the popover window
	PopoverManager_BehaviorType		behaviorType;			// specifies how the popover window responds to other events
	NSWindow*						parentCocoaWindow;		// the window the popover is relative to, if Cocoa
	HIWindowRef						parentCarbonWindow;		// the window the popover is relative to, if Carbon
	CarbonEventHandlerWrap*			activationHandlerPtr;	// embellishes Carbon Event for activating window
	CarbonEventHandlerWrap*			minimizeHandlerPtr;		// embellishes Carbon Event for minimizing window
	CarbonEventHandlerWrap*			resizeHandlerPtr;		// embellishes Carbon Event for resizing window
}

// class methods
	+ (PopoverManager_Handler*)
	popoverHandlerFromRef:(PopoverManager_Ref)_;

// initializers
	- (instancetype)
	initForCocoaWindow:(NSWindow*)_
	orCarbonWindow:(HIWindowRef)_
	popover:(Popover_Window*)_
	firstResponder:(NSView*)_
	animationType:(PopoverManager_AnimationType)_
	behavior:(PopoverManager_BehaviorType)_
	delegate:(id< PopoverManager_Delegate >)_;

// new methods
	- (void)
	display;
	- (NSPoint)
	idealAnchorPointForFrame:(NSRect)_
	parentWindow:(NSWindow*)_;
	- (Popover_Properties)
	idealArrowPositionForFrame:(NSRect)_
	parentWindow:(NSWindow*)_;
	- (void)
	moveToIdealPosition;
	- (void)
	popOver;
	- (void)
	popUnder;
	- (void)
	removeWindowAfterDelayWithAcceptance:(BOOL)_;
	- (void)
	removeWindowWithAcceptance:(BOOL)_;
	- (void)
	removeWindowWithAcceptance:(BOOL)_
	afterDelay:(float)_;
	- (void)
	setToIdealSize;

// accessors
	- (NSSize)
	idealSize;
	- (BOOL)
	isVisible;
	- (NSWindow*)
	parentCocoaWindow;

@end //}

#pragma mark Internal Method Prototypes
namespace {

OSStatus	receiveWindowActivationChange	(EventHandlerCallRef, EventRef, void*);
OSStatus	receiveWindowCollapse			(EventHandlerCallRef, EventRef, void*);
OSStatus	receiveWindowResize				(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Constructs a new popover manager, where the parent window is
Cocoa-based.

(2.7)
*/
PopoverManager_Ref
PopoverManager_New	(Popover_Window*				inPopover,
					 NSView*						inLogicalFirstResponder,
					 id< PopoverManager_Delegate >	inDelegate,
					 PopoverManager_AnimationType	inAnimation,
					 PopoverManager_BehaviorType	inBehavior,
					 NSWindow*						inParentWindow)
{
	PopoverManager_Ref	result = nullptr;
	
	
	result = (PopoverManager_Ref)[[PopoverManager_Handler alloc]
									initForCocoaWindow:inParentWindow orCarbonWindow:nullptr popover:inPopover
														firstResponder:inLogicalFirstResponder
														animationType:inAnimation behavior:inBehavior
														delegate:inDelegate];
	return result;
}// New


/*!
Constructs a new popover manager, where the parent window is
Carbon-based.

(2.7)
*/
PopoverManager_Ref
PopoverManager_New	(Popover_Window*				inPopover,
					 NSView*						inLogicalFirstResponder,
					 id< PopoverManager_Delegate >	inDelegate,
					 PopoverManager_AnimationType	inAnimation,
					 PopoverManager_BehaviorType	inBehavior,
					 HIWindowRef					inParentWindow)
{
	PopoverManager_Ref	result = nullptr;
	
	
	result = (PopoverManager_Ref)[[PopoverManager_Handler alloc]
									initForCocoaWindow:nil orCarbonWindow:inParentWindow popover:inPopover
														firstResponder:inLogicalFirstResponder
														animationType:inAnimation behavior:inBehavior
														delegate:inDelegate];
	return result;
}// New


/*!
Destroys a popover manager and sets your copy of the
reference to nullptr.

(2.7)
*/
void
PopoverManager_Dispose	(PopoverManager_Ref*	inoutRefPtr)
{
	PopoverManager_Handler*		ptr = [PopoverManager_Handler popoverHandlerFromRef:*inoutRefPtr];
	
	
	[ptr release];
}// Dispose


/*!
Uses an appropriate animation sequence to display the
popover window associated with this manager, or simply
gives it keyboard focus and puts it in front if it is
already visible.

(2.7)
*/
void
PopoverManager_DisplayPopover	(PopoverManager_Ref		inRef)
{
	PopoverManager_Handler*		ptr = [PopoverManager_Handler popoverHandlerFromRef:inRef];
	
	
	[ptr display];
}// Display


/*!
Hides the popover with an appropriate animation.  It can
be shown again at any time with PopoverManager_Display().
If a delay is specified, the popover does not disappear
right away; this can be useful to avoid clobbering other
animations that may occur at closing time.

(2.7)
*/
void
PopoverManager_RemovePopover	(PopoverManager_Ref		inRef,
								 Boolean				inIsConfirming)
{
	PopoverManager_Handler*		ptr = [PopoverManager_Handler popoverHandlerFromRef:inRef];
	
	
	[ptr removeWindowWithAcceptance:inIsConfirming];
}// Remove


/*!
Changes the type of animation used for the popover.
This is typically only used just prior to closing a
popover, in order to hide it immediately in certain
situations (such as closing its parent window).

(2.8)
*/
void
PopoverManager_SetAnimationType		(PopoverManager_Ref				inRef,
									 PopoverManager_AnimationType	inAnimation)
{
	PopoverManager_Handler*		ptr = [PopoverManager_Handler popoverHandlerFromRef:inRef];
	
	
	ptr->animationType = inAnimation;
}// SetAnimationType


/*!
Changes the way that a popover responds to other events.

(2.8)
*/
void
PopoverManager_SetBehaviorType		(PopoverManager_Ref				inRef,
									 PopoverManager_BehaviorType	inBehavior)
{
	PopoverManager_Handler*		ptr = [PopoverManager_Handler popoverHandlerFromRef:inRef];
	
	
	ptr->behaviorType = inBehavior;
}// SetBehaviorType


/*!
For popovers that use “automatic” placement, recalculates
the best place for the arrow and relocates the popover so
that it is pointing (in a possibly new direction) at its
original ideal point.  If the arrow is on a different side
of the popover as a result, the frame size may change
slightly to accommodate the arrow along the new edge.

This is done automatically as a side effect of displaying
a popover, so you only need this API in special situations;
for example, if you need to resize the popover window then
you may want to correct its position.

(2.7)
*/
void
PopoverManager_UseIdealLocationAfterDelay	(PopoverManager_Ref		inRef,
											 Float32				inDelay)
{
	PopoverManager_Handler*		ptr = [PopoverManager_Handler popoverHandlerFromRef:inRef];
	
	
	[ptr performSelector:@selector(moveToIdealPosition) withObject:nil afterDelay:inDelay];
}// UseIdealLocationAfterDelay


#pragma mark Internal Methods
namespace {


/*!
Handles "kEventWindowDeactivated" of "kEventClassWindow" for
the parent window of a popover.

Ensures that the popover does not obscure windows other than
its own parent window.

IMPORTANT:	This must be kept in sync with the Cocoa
			equivalents, e.g. "windowDidResignKey:".

(2.7)
*/
OSStatus
receiveWindowActivationChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inHandler)
{
	OSStatus					result = eventNotHandledErr;
	PopoverManager_Handler*		handler = REINTERPRET_CAST(inHandler, PopoverManager_Handler*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert((kEventKind == kEventWindowActivated) || (kEventKind == kEventWindowDeactivated));
	
	if ([handler isVisible])
	{
	#if 0
		[handler removeWindow];
	#else
		if (kPopoverManager_BehaviorTypeDialog == handler->behaviorType)
		{
			if (kEventKind == kEventWindowDeactivated)
			{
				// when the parent window is active, the popover should remain on top
				[handler popUnder];
			}
			else
			{
				// allow other normal windows to sit above background popovers
				[handler popOver];
				HiliteWindow(handler->parentCarbonWindow, false);
			}
		}
	#endif
	}
	result = eventNotHandledErr; // not completely handled
	
	return result;
}// receiveWindowActivationChange


/*!
Handles "kEventWindowCollapse" of "kEventClassWindow" for
the parent window of a popover.

Ensures that the popover disappears before the parent window
is minimized (helps to avoid bugs that are not yet resolved,
such as having the popover remain visible even if the parent
window is minimized).

(2.7)
*/
OSStatus
receiveWindowCollapse	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inHandler)
{
	OSStatus					result = eventNotHandledErr;
	PopoverManager_Handler*		handler = REINTERPRET_CAST(inHandler, PopoverManager_Handler*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowCollapse);
	
	if ([handler isVisible])
	{
		[handler removeWindowAfterDelayWithAcceptance:NO];
	}
	result = eventNotHandledErr; // not completely handled
	
	return result;
}// receiveWindowCollapse


/*!
Handles "kEventWindowBoundsChanged" of "kEventClassWindow"
for the parent window of a popover.

Ensures that the popover remains in its ideal position.

(2.7)
*/
OSStatus
receiveWindowResize		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inHandler)
{
	OSStatus					result = eventNotHandledErr;
	PopoverManager_Handler*		handler = REINTERPRET_CAST(inHandler, PopoverManager_Handler*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowBoundsChanged);
	
	if ([handler isVisible])
	{
		[handler performSelector:@selector(moveToIdealPosition) withObject:nil afterDelay:0.5/* arbitrary */];
	}
	result = eventNotHandledErr; // not completely handled
	
	return result;
}// receiveWindowResize


} // anonymous namespace


#pragma mark -
@implementation PopoverManager_Handler


/*!
Converts from the opaque reference type to the internal type.

(2.7)
*/
+ (PopoverManager_Handler*)
popoverHandlerFromRef:(PopoverManager_Ref)	aRef
{
	return (PopoverManager_Handler*)aRef;
}// popoverHandlerFromRef


/*!
Designated initializer.

The delegate object should conform to the protocol
"PopoverManager_Delegate".  This is not enforced
for simplicity in the C-style header file.

(2.7	)
*/
- (instancetype)
initForCocoaWindow:(NSWindow*)					aCocoaWindow
orCarbonWindow:(HIWindowRef)					aCarbonWindow
popover:(Popover_Window*)						aPopover
firstResponder:(NSView*)						aView
animationType:(PopoverManager_AnimationType)	animationSpec
behavior:(PopoverManager_BehaviorType)			behaviorSpec
delegate:(id< PopoverManager_Delegate >)		anObject
{
	self = [super initWithWindow:aPopover];
	if (nil != self)
	{
		HIWindowRef		windowWatchedForMinimize = nullptr;
		
		
		if (nullptr != aCarbonWindow)
		{
			// if the parent of the popover is a sheet, install the
			// minimization handler on its parent instead
			if (noErr != GetSheetWindowParent(aCarbonWindow, &windowWatchedForMinimize))
			{
				windowWatchedForMinimize = aCarbonWindow;
			}
		}
		
		self->selfRef = (PopoverManager_Ref)self;
		self->delegate = anObject;
		self->containerWindow = aPopover;
		[self->containerWindow retain];
		self->logicalFirstResponder = aView;
		self->animationType = animationSpec;
		self->behaviorType = behaviorSpec;
		self->parentCocoaWindow = aCocoaWindow;
		self->parentCarbonWindow = aCarbonWindow;
		
		if (nullptr != aCarbonWindow)
		{
			// unfortunately Cocoa window notifications do not seem to work for
			// Carbon-based Cocoa windows on all Mac OS X versions, so for now
			// use Carbon Events to detect important changes
			self->activationHandlerPtr = new CarbonEventHandlerWrap
												(GetWindowEventTarget(aCarbonWindow), receiveWindowActivationChange,
													CarbonEventSetInClass(CarbonEventClass(kEventClassWindow),
																			kEventWindowActivated,
																			kEventWindowDeactivated), self/* handler data */);
			self->minimizeHandlerPtr = new CarbonEventHandlerWrap
											(GetWindowEventTarget(windowWatchedForMinimize), receiveWindowCollapse,
												CarbonEventSetInClass(CarbonEventClass(kEventClassWindow),
																		kEventWindowCollapse), self/* handler data */);
			self->resizeHandlerPtr = new CarbonEventHandlerWrap
											(GetWindowEventTarget(aCarbonWindow), receiveWindowResize,
												CarbonEventSetInClass(CarbonEventClass(kEventClassWindow),
																		kEventWindowBoundsChanged), self/* handler data */);
		}
		
		// also monitor the popover itself to know when to auto-hide
		[self whenObject:self->containerWindow postsNote:NSWindowDidResignKeyNotification
							performSelector:@selector(windowDidResignKey:)];
	}
	return self;
}// initForCarbonWindow:popover:


/*!
Destructor.

(2.7)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
	delete self->activationHandlerPtr, activationHandlerPtr = nullptr;
	delete self->minimizeHandlerPtr, minimizeHandlerPtr = nullptr;
	delete self->resizeHandlerPtr, resizeHandlerPtr = nullptr;
	[containerWindow release];
	[super dealloc];
}// dealloc


/*!
Shows the popover view with appropriate animation if it is
invisible, puts it in front and gives it keyboard focus.

(2.7)
*/
- (void)
display
{
	[self setToIdealSize];
	[self moveToIdealPosition];
	switch (self->animationType)
	{
	case kPopoverManager_AnimationTypeNone:
		if ([NSWindow instancesRespondToSelector:@selector(setAnimationBehavior:)])
		{
			// remove window animations
			[self->containerWindow setAnimationBehavior:FUTURE_SYMBOL(2, NSWindowAnimationBehaviorNone)];
		}
		break;
	
	case kPopoverManager_AnimationTypeMinimal:
		if ([NSWindow instancesRespondToSelector:@selector(setAnimationBehavior:)])
		{
			// create fade-in effect; admittedly a bit of a hack...
			[self->containerWindow setAnimationBehavior:FUTURE_SYMBOL(4, NSWindowAnimationBehaviorUtilityWindow)];
		}
		break;
	
	case kPopoverManager_AnimationTypeStandard:
	default:
		if (kPopoverManager_BehaviorTypeDialog == self->behaviorType)
		{
			CocoaAnimation_TransitionWindowForSheetOpen(self->containerWindow, [self parentCocoaWindow]);
		}
		else
		{
			if ([NSWindow instancesRespondToSelector:@selector(setAnimationBehavior:)])
			{
				// create bubble effect; admittedly a bit of a hack...
				[self->containerWindow setAnimationBehavior:FUTURE_SYMBOL(5, NSWindowAnimationBehaviorAlertPanel)];
			}
		}
		break;
	}
	[[self parentCocoaWindow] addChildWindow:self->containerWindow ordered:NSWindowAbove];
	[self popOver];
}// display


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForFrame:parentWindow:".

IMPORTANT:	This must be implemented by the delegate.

See also "moveToIdealPosition".

(2.7)
*/
- (NSPoint)
idealAnchorPointForFrame:(NSRect)	parentFrame
parentWindow:(NSWindow*)			parentWindow
{
#pragma unused(parentWindow)
	NSPoint		result = [self->delegate idealAnchorPointForFrame:parentFrame parentWindow:parentWindow];
	
	
	return result;
}// idealAnchorPointForFrame:parentWindow:


/*!
Returns arrow placement information for the popover.

IMPORTANT:	This must be implemented by the delegate.

(2.7)
*/
- (Popover_Properties)
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
	Popover_Properties	result = [self->delegate idealArrowPositionForFrame:parentFrame parentWindow:parentWindow];
	
	
	return result;
}// idealArrowPositionForFrame:parentWindow:


/*!
Returns the dimensions that the popover should initially have.

IMPORTANT:	This must be implemented by the delegate.

See also "setToIdealSize".

(2.7)
*/
- (NSSize)
idealSize
{
	NSSize		result = [self->delegate idealSize];
	
	
	return result;
}// idealSize


/*!
Returns YES only if the popover is currently displayed.

(2.7)
*/
- (BOOL)
isVisible
{
	return [self->containerWindow isVisible];
}// isVisible


/*!
Moves the popover to its correct position relative to
its parent window.

See also "idealAnchorPointForFrame:parentWindow:".

(2.7)
*/
- (void)
moveToIdealPosition
{
	NSWindow*			parentWindow = [self parentCocoaWindow];
	NSRect				parentFrame = [parentWindow frame];
	NSPoint				popoverLocation = [self idealAnchorPointForFrame:parentFrame parentWindow:parentWindow];
	Popover_Properties	arrowType = [self idealArrowPositionForFrame:parentFrame parentWindow:parentWindow];
	
	
	[self->containerWindow setPointWithAutomaticPositioning:popoverLocation preferredSide:arrowType];
}// moveToIdealPosition


/*!
Returns the Cocoa window that represents the renamed
window, even if that is a Carbon window.

(2.7)
*/
- (NSWindow*)
parentCocoaWindow
{
	NSWindow*	result = self->parentCocoaWindow;
	
	
	if ((nil == result) && (nullptr != self->parentCarbonWindow))
	{
		result = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(self->parentCarbonWindow);
	}
	
	return result;
}// parentCocoaWindow


/*!
Moves the popover window to the appropriate window level
and gives it the keyboard focus.

(2.7)
*/
- (void)
popOver
{
	// TEMPORARY; in Carbon-Cocoa hybrid environments popovers do not necessarily
	// handle the parent window correctly in all situations, so the simplest
	// solution is just to force the parent window to deactivate (in the future
	// when all windows are pure Cocoa, it should be very easy to support cases
	// where popovers do not dim their parent windows at any time)
	[[self parentCocoaWindow] resignMainWindow];
	
	[self->containerWindow setLevel:([[self parentCocoaWindow] level] + 1)];
	[self->containerWindow makeFirstResponder:self->logicalFirstResponder];
	[self->containerWindow makeKeyAndOrderFront:nil];
}// popOver


/*!
Moves the popover window to another window level that
will allow normal windows to overlap it.  This should be
done when the parent window of the popover is deactivated.

(2.7)
*/
- (void)
popUnder
{
	[self->containerWindow setLevel:[[self parentCocoaWindow] level]];
}// popUnder


/*!
Invokes "removeWindowWithAcceptance:afterDelay:" with a short
delay.  Useful when hiding amidst other animations that should
not be interrupted by closing animations.

(2.7)
*/
- (void)
removeWindowAfterDelayWithAcceptance:(BOOL)		isAccepted
{
	[self removeWindowWithAcceptance:isAccepted afterDelay:0.03];
}// removeWindowAfterDelayWithAcceptance:


/*!
Invokes "removeWindowWithAcceptance:afterDelay:" with no delay.
This is the normal case.

(2.7)
*/
- (void)
removeWindowWithAcceptance:(BOOL)	isAccepted
{
	[self removeWindowWithAcceptance:isAccepted afterDelay:0];
}// removeWindowWithAcceptance:


/*!
Hides the popover window, removing it as a child of its
parent window.

The "isAccepted" flag may cause the animation to vary to
show the user that the window’s changes are being accepted
(as opposed to being discarded entirely).

If a delay is specified then the window is only hidden
after that time period.

The "close" method is invoked so you can detect this
event through NSWindowWillCloseNotification.

See also "display".

(2.7)
*/
- (void)
removeWindowWithAcceptance:(BOOL)	isAccepted
afterDelay:(float)					aDelay
{
	// hide the popover; remove the parent window association first to keep
	// the parent window from disappearing on some versions of Mac OS X!
	if ([self->containerWindow parentWindow] == [self parentCocoaWindow])
	{
		[[self parentCocoaWindow] removeChildWindow:self->containerWindow];
	}
	switch (self->animationType)
	{
	case kPopoverManager_AnimationTypeNone:
		if ([NSWindow instancesRespondToSelector:@selector(setAnimationBehavior:)])
		{
			// remove window animations
			[self->containerWindow setAnimationBehavior:FUTURE_SYMBOL(2, NSWindowAnimationBehaviorNone)];
		}
		[self->containerWindow performSelector:@selector(close) withObject:nil afterDelay:aDelay];
		break;
	
	default:
		if (kPopoverManager_BehaviorTypeDialog == self->behaviorType)
		{
			if ([NSWindow instancesRespondToSelector:@selector(setAnimationBehavior:)])
			{
				// remove system-provided window animations because this
				// has a custom close animation style
				[self->containerWindow setAnimationBehavior:FUTURE_SYMBOL(2, NSWindowAnimationBehaviorNone)];
			}
			CocoaAnimation_TransitionWindowForRemove(self->containerWindow, isAccepted);
		}
		else
		{
			// currently, all other closing animations are the same
			if ([self->containerWindow respondsToSelector:@selector(setAnimationBehavior:)])
			{
				// create fade-out effect; admittedly a bit of a hack...
				[self->containerWindow setAnimationBehavior:FUTURE_SYMBOL(4, NSWindowAnimationBehaviorUtilityWindow)];
			}
			[self->containerWindow performSelector:@selector(close) withObject:nil afterDelay:aDelay];
		}
		break;
	}
}// removeWindowWithAcceptance:afterDelay:


/*!
Resizes the popover to be large enough for the minimum
size of its content view.

(2.7)
*/
- (void)
setToIdealSize
{
	NSRect		parentFrame = [self->containerWindow frame];
	NSSize		newSize = [self idealSize];
	
	
	parentFrame.size.width = newSize.width;
	parentFrame.size.height = newSize.height;
	
	[self->containerWindow setFrame:parentFrame display:NO];
}// setToIdealSize


#pragma mark NSColorPanel


/*!
Forwards this message to the main view manager if the
panel implements a "changeColor:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeColor:(id)	sender
{
	NSObject*	asNSObject = STATIC_CAST(self->delegate, NSObject*);
	
	
	if ([asNSObject respondsToSelector:@selector(changeColor:)])
	{
		[asNSObject changeColor:sender];
	}
}// changeColor:


#pragma mark NSFontPanel


/*!
Forwards this message to the main view manager if the
panel implements a "changeFont:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeFont:(id)		sender
{
	NSObject*	asNSObject = STATIC_CAST(self->delegate, NSObject*);
	
	
	if ([asNSObject respondsToSelector:@selector(changeFont:)])
	{
		[asNSObject changeFont:sender];
	}
}// changeFont:


#pragma mark NSWindowNotifications


/*!
Dismisses the popover when the user goes somewhere else,
unless the reason for that focus change was to display
a sheet on top or activate a floating panel.

(2.7)
*/
- (void)
windowDidResignKey:(NSNotification*)	aNotification
{
	NSWindow*	formerKeyWindow = (NSWindow*)[aNotification object];
	
	
	if (formerKeyWindow == self->containerWindow)
	{
		BOOL	removePopover = YES;
		
		
		if (kPopoverManager_BehaviorTypeDialog == self->behaviorType)
		{
			// dialogs try to retain keyboard focus, expecting to be
			// dismissed in an explicit way (e.g. via buttons)
			removePopover = NO;
		#if 0
			if ([NSApp keyWindow] == [self parentCocoaWindow])
			{
				// force clicks in the parent to be ignored
				[self->containerWindow makeKeyWindow];
				[[self parentCocoaWindow] resignMainWindow];
			}
		#endif
		}
		
		if (removePopover)
		{
			if ([self isVisible])
			{
				if ((nil == [formerKeyWindow attachedSheet]) &&
					(NO == [[NSApp keyWindow] isKindOfClass:NSPanel.class]))
				{
					[self removeWindowWithAcceptance:NO];
				}
			}
		}
	}
}// windowDidResignKey:


@end // PopoverManager_Handler

// BELOW IS REQUIRED NEWLINE TO END FILE
