/*!	\file CocoaAnimation.mm
	\brief Animation utilities wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.8
	© 2008-2011 by Kevin Grant
	
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <AutoPool.objc++.h>
#import <CocoaFuture.objc++.h>
#include "Console.h"



#pragma mark Constants
namespace {

/*!
Effects used by CocoaAnimation_WindowFrameAnimator.  These
determine any other changes (besides the window frame) that
should occur during an animation.
*/
enum My_AnimationEffect
{
	kMy_AnimationEffectNone		= 0,	//!< no special effects
	kMy_AnimationEffectFadeIn	= 1,	//!< alpha value of window gradually becomes opaque
	kMy_AnimationEffectFadeOut	= 2		//!< alpha value of window gradually becomes transparent
};

/*!
Time distributions used by CocoaAnimation_WindowFrameAnimator.
These determine how frame delays are scaled at each stage.
*/
enum My_AnimationTimeDistribution
{
	kMy_AnimationTimeDistributionLinear		= 0,	//!< equal delays
	kMy_AnimationTimeDistributionEaseOut	= 1		//!< slower at the end, faster at the start
};

/*!
Transitions used by CocoaAnimation_WindowFrameAnimator.  These
determine the kinds of steps that are taken to move from the
source frame to the target frame.
*/
enum My_AnimationTransition
{
	kMy_AnimationTransitionSlide	= 0		//!< window moves straight from source to destination frame
};

} // anonymous namespace

@interface CocoaAnimation_WindowFrameAnimator : NSObject
{
	NSWindow*			borderlessWindow;
	NSWindow*			actualWindow;
	NSRect				originalFrame;
	NSRect				targetFrame;
	unsigned int		frameCount;
	unsigned int		currentFrame;
	NSTimeInterval*		frameDelays;
	float*				frameAlphas;
	float*				frameOffsetsH;
	float*				frameOffsetsV;
	float				frameUnitH;
	float				frameUnitV;
}

// designated initializer
- (id)
initWithTransition:(My_AnimationTransition)_
imageWindow:(NSWindow*)_
finalWindow:(NSWindow*)_
fromFrame:(NSRect)_
toFrame:(NSRect)_
delayDistribution:(My_AnimationTimeDistribution)_
effect:(My_AnimationEffect)_
simplified:(BOOL)_;

- (void)
animationStep:(id)_;

@end

#pragma mark Internal Method prototypes
namespace {

NSWindow*	createImageWindowOf		(NSWindow*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Animates "inTargetWindow" from a point that is staggered
relative to "inRelativeWindow", in a way that suggests the
target window is a copy of the relative window.  Use this
for commands that duplicate existing windows.

If "inSlowTransition" is true, some frames are dropped from
the animation to try to maintain good performance.  This is
recommended on older hardware or operating system versions
but it should not be necessary for modern machines.

(1.8)
*/
void
CocoaAnimation_TransitionWindowForDuplicate		(NSWindow*		inTargetWindow,
												 NSWindow*		inRelativeToWindow,
												 Boolean		inSlowTransition)
{
	AutoPool	_;
	
	
	// show the window offscreen so its image is defined
	[inTargetWindow setFrameTopLeftPoint:NSMakePoint(-2000, -2000)];
	[inTargetWindow orderFront:nil];
	
	// animate the change
	{
		NSWindow*	imageWindow = [createImageWindowOf(inTargetWindow) autorelease];
		NSRect		oldFrame = [inRelativeToWindow frame];
		NSRect		newFrame = [inRelativeToWindow frame];
		
		
		// copy only the relative window’s location, not its size
		oldFrame.size = [inTargetWindow frame].size;
		
		// target a new location that is slightly offset from the related window
		newFrame.origin.x += 20/* arbitrary */;
		newFrame.origin.y -= 20/* arbitrary */;
		newFrame.size = [inTargetWindow frame].size;
		
		// animate!
		[imageWindow orderFront:nil];
		[imageWindow setLevel:[inTargetWindow level]];
		[[[CocoaAnimation_WindowFrameAnimator alloc]
			initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:inTargetWindow
								fromFrame:oldFrame toFrame:newFrame delayDistribution:kMy_AnimationTimeDistributionLinear
								effect:kMy_AnimationEffectFadeIn simplified:((inSlowTransition) ? YES : NO)] autorelease];
	}
}// TransitionWindowForDuplicate


/*!
Animates "inTargetWindow" in a way that suggests it is being
destroyed.

If "inSlowTransition" is true, some frames are dropped from
the animation to try to maintain good performance.  This is
recommended on older hardware or operating system versions
but it should not be necessary for modern machines.

In order to avoid potential problems with the lifetime of a
window, the specified window is actually hidden immediately
and is replaced with a borderless window that renders the
exact same image!  The animation is then applied only to the
window that appears to be the original window.

(1.8)
*/
void
CocoaAnimation_TransitionWindowForRemove	(NSWindow*		inTargetWindow,
											 Boolean		inSlowTransition)
{
	AutoPool	_;
	
	
	if ([inTargetWindow isVisible])
	{
		NSWindow*	imageWindow = [createImageWindowOf(inTargetWindow) autorelease];
		NSRect		oldFrame = [imageWindow frame];
		NSRect		newFrame = [imageWindow frame];
		
		
		// target a new location that appears to toss the window away
		newFrame.origin.x += 250/* arbitrary */;
		newFrame.origin.y -= 450/* arbitrary */;
		
		// animate!
		[imageWindow orderFront:nil];
		[imageWindow setLevel:[inTargetWindow level]];
		[[[CocoaAnimation_WindowFrameAnimator alloc]
			initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
								fromFrame:oldFrame toFrame:newFrame delayDistribution:kMy_AnimationTimeDistributionEaseOut
								effect:kMy_AnimationEffectFadeOut simplified:((inSlowTransition) ? YES : NO)] autorelease];
		
		// hide the original window immediately; the animation on the
		// image window can take however long it needs to complete
		[inTargetWindow orderOut:nil];
	}
}// TransitionWindowForRemove


#pragma mark Internal Methods
namespace {

/*!
Creates a new, borderless window whose content view is an
image view that renders the entire content of the specified
window.

This is very useful as a basis for animations, because it
allows a window to appear to be moving in a way that does
not require the original window to even exist.  It also
tends to be much more lightweight.

(4.0)
*/
NSWindow*
createImageWindowOf		(NSWindow*		inWindow)
{
	NSWindow*	result = nil;
	NSView*		originalContentView = [inWindow contentView];
	NSRect		zeroOriginBounds = [inWindow frame];
	
	
	zeroOriginBounds.origin = NSZeroPoint;
	
	// capture the image of the original window
	{
		NSBitmapImageRep*	imageRep = nil;
		NSImage*			windowImage = [[[NSImage alloc] init] autorelease];
		NSImageView*		imageView = [[[NSImageView alloc] initWithFrame:zeroOriginBounds] autorelease];
		
		
		// capture the contents of the original window
		[originalContentView lockFocus];
		imageRep = [[[NSBitmapImageRep alloc] initWithFocusedViewRect:[originalContentView bounds]] autorelease];
		[originalContentView unlockFocus];
		[windowImage addRepresentation:imageRep];
		[imageView setImage:windowImage];
		
		// now construct a fake window to display the same thing
		result = [[NSWindow alloc] initWithContentRect:[inWindow frame] styleMask:NSBorderlessWindowMask
														backing:NSBackingStoreBuffered defer:NO];
		[result setContentView:imageView];
	}
	
	return result;
}// createImageWindowOf

} // anonymous namespace


@implementation CocoaAnimation_WindowFrameAnimator


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithTransition:(My_AnimationTransition)			aTransition
imageWindow:(NSWindow*)								aBorderlessWindow
finalWindow:(NSWindow*)								theActualWindow
fromFrame:(NSRect)									sourceRect
toFrame:(NSRect)									targetRect
delayDistribution:(My_AnimationTimeDistribution)	aDistribution
effect:(My_AnimationEffect)							anEffect
simplified:(BOOL)									isSimplified
{
	self = [super init];
	if (nil != self)
	{
		self->borderlessWindow = [aBorderlessWindow retain];
		self->actualWindow = [theActualWindow retain];
		self->originalFrame = sourceRect;
		self->targetFrame = targetRect;
		self->frameCount = (isSimplified) ? 5 : 10;
		self->currentFrame = 0;
		self->frameDelays = new NSTimeInterval[self->frameCount];
		if ((kMy_AnimationEffectFadeIn == anEffect) ||
			(kMy_AnimationEffectFadeOut == anEffect))
		{
			self->frameAlphas = new float[self->frameCount];
		}
		else
		{
			self->frameAlphas = nullptr;
		}
		self->frameOffsetsH = new float[self->frameCount];
		self->frameOffsetsV = new float[self->frameCount];
		
		// configure animation frames; remember that the frame has
		// its origin in the bottom-left, not the top-left; the
		// the offsets can be any amount per frame, but the total
		// offset should equal the number of frames above
		switch (aTransition)
		{
		case kMy_AnimationTransitionSlide:
		default:
			// move by even amounts directly toward the destination
			for (size_t i = 0; i < self->frameCount; ++i)
			{
				self->frameOffsetsH[i] = 1 + i;
				self->frameOffsetsV[i] = 1 + i;
			}
			break;
		}
		
		// configure alpha values, if applicable
		switch (anEffect)
		{
		case kMy_AnimationEffectFadeIn:
			if (5 == self->frameCount)
			{
				size_t		i = 0;
				
				
				self->frameAlphas[i++] = 0.2;
				self->frameAlphas[i++] = 0.4;
				self->frameAlphas[i++] = 0.6;
				self->frameAlphas[i++] = 0.8;
				self->frameAlphas[i++] = 1.0;
				assert(i == self->frameCount);
			}
			else
			{
				size_t		i = 0;
				
				
				self->frameAlphas[i++] = 0.2;
				self->frameAlphas[i++] = 0.3;
				self->frameAlphas[i++] = 0.4;
				self->frameAlphas[i++] = 0.5;
				self->frameAlphas[i++] = 0.6;
				self->frameAlphas[i++] = 0.7;
				self->frameAlphas[i++] = 0.8;
				self->frameAlphas[i++] = 0.9;
				self->frameAlphas[i++] = 0.95;
				self->frameAlphas[i++] = 1.0;
				assert(i == self->frameCount);
			}
			break;
		
		case kMy_AnimationEffectFadeOut:
			if (5 == self->frameCount)
			{
				size_t		i = 0;
				
				
				self->frameAlphas[i++] = 0.7;
				self->frameAlphas[i++] = 0.5;
				self->frameAlphas[i++] = 0.3;
				self->frameAlphas[i++] = 0.2;
				self->frameAlphas[i++] = 0.1;
				assert(i == self->frameCount);
			}
			else
			{
				size_t		i = 0;
				
				
				self->frameAlphas[i++] = 0.85;
				self->frameAlphas[i++] = 0.7;
				self->frameAlphas[i++] = 0.55;
				self->frameAlphas[i++] = 0.45;
				self->frameAlphas[i++] = 0.35;
				self->frameAlphas[i++] = 0.25;
				self->frameAlphas[i++] = 0.2;
				self->frameAlphas[i++] = 0.15;
				self->frameAlphas[i++] = 0.1;
				self->frameAlphas[i++] = 0.05;
				assert(i == self->frameCount);
			}
			break;
		
		default:
			// do nothing
			break;
		}
		
		// configure delays
		for (size_t i = 0; i < self->frameCount; ++i)
		{
			if (kMy_AnimationTimeDistributionEaseOut == aDistribution)
			{
				// gradually become slower
				self->frameDelays[i] = 0.002 * self->frameCount + 0.005;
			}
			else
			{
				// linear
				self->frameDelays[i] = 0.002;
			}
		}
		
		// calculate the size of each unit of the animation
		self->frameUnitH = (self->targetFrame.origin.x - self->originalFrame.origin.x) / self->frameCount;
		self->frameUnitV = (self->targetFrame.origin.y - self->originalFrame.origin.y) / self->frameCount;
		
		// begin animation
		[self animationStep:nil];
	}
	return self;
}// initWithTransition:window:fromFrame:toFrame:delayDistribution:effect:simplified:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self->borderlessWindow release];
	[self->actualWindow release];
	delete [] self->frameDelays;
	delete [] self->frameOffsetsH;
	delete [] self->frameOffsetsV;
	if (nullptr != self->frameAlphas)
	{
		delete [] self->frameAlphas;
	}
	[super dealloc];
}// dealloc


/*!
Nudges the window to a new frame according to the current
step in the animation and updates the step number.  If the
last frame has not been reached, a timer is set up to call
this again after a short delay.

(4.0)
*/
- (void)
animationStep:(id)	unused
{
#pragma unused(unused)
	NSRect	newFrame = self->originalFrame;
	
	
	// the offsets assume a unit square and scale according to the actual
	// difference between the original frame and the target frame; so an
	// offset of 1 when there are 5 frames means “20% of the distance from
	// the beginning”, 2 means 40%, etc. so offsets should generally be
	// well represented across the spectrum from 0 to the frame count
	newFrame.origin.x += (self->frameOffsetsH[self->currentFrame] * self->frameUnitH);
	newFrame.origin.y += (self->frameOffsetsV[self->currentFrame] * self->frameUnitV);
	[self->borderlessWindow setFrame:newFrame display:YES];
	
	// adjust alpha channel, if necessary
	if (nullptr != self->frameAlphas)
	{
		[self->borderlessWindow setAlphaValue:self->frameAlphas[self->currentFrame]];
	}
	
	// step to next frame
	++(self->currentFrame);
	if (self->currentFrame < self->frameCount)
	{
		[self performSelector:@selector(animationStep:) withObject:nil afterDelay:self->frameDelays[self->currentFrame]];
	}
	else
	{
		// force final frame to be exact
		[self->borderlessWindow setFrame:self->targetFrame display:YES];
		
		// if a real window is being displayed, show it at this point
		if (nil != self->actualWindow)
		{
			[self->actualWindow setFrame:self->targetFrame display:YES];
			[self->actualWindow orderFront:nil];
		}
		
		// hide the fake window
		[self->borderlessWindow orderOut:nil];
	}
}// animationStep:


@end // CocoaAnimation_WindowFrameAnimator

// BELOW IS REQUIRED NEWLINE TO END FILE
