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

#import <CocoaAnimation.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <AutoPool.objc++.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>



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
	float				frameDeltaSizeH;
	float				frameDeltaSizeV;
}

// designated initializer
- (id)
initWithTransition:(My_AnimationTransition)_
imageWindow:(NSWindow*)_
finalWindow:(NSWindow*)_
fromFrame:(NSRect)_
toFrame:(NSRect)_
totalDelay:(NSTimeInterval)_
delayDistribution:(My_AnimationTimeDistribution)_
effect:(My_AnimationEffect)_
simplified:(BOOL)_;

- (id)
initWithTransition:(My_AnimationTransition)_
imageWindow:(NSWindow*)_
finalWindow:(NSWindow*)_
fromFrame:(NSRect)_
toFrame:(NSRect)_
totalDelay:(NSTimeInterval)_
delayDistribution:(My_AnimationTimeDistribution)_
effect:(My_AnimationEffect)_;

- (void)
animationStep:(id)_;

@end

#pragma mark Internal Method prototypes
namespace {

NSWindow*	createImageWindowFrom	(NSWindow*, NSRect);

} // anonymous namespace



#pragma mark Public Methods

/*!
Animates "inTargetWindow" from a point that is staggered
relative to "inRelativeWindow", in a way that suggests the
target window is a copy of the relative window.  Use this
for commands that duplicate existing windows.

(1.8)
*/
void
CocoaAnimation_TransitionWindowForDuplicate		(NSWindow*		inTargetWindow,
												 NSWindow*		inRelativeToWindow)
{
	AutoPool	_;
	
	
	// show the window offscreen so its image is defined
	[inTargetWindow setFrameTopLeftPoint:NSMakePoint(-2000, -2000)];
	[inTargetWindow orderFront:nil];
	
	// animate the change
	{
		float const		kAnimationDelay = 0.001;
		NSWindow*		imageWindow = [createImageWindowFrom(inTargetWindow, [[inTargetWindow contentView] bounds]) autorelease];
		NSRect			oldFrame = [inRelativeToWindow frame];
		NSRect			newFrame = NSZeroRect;
		NSRect			mainScreenFrame = [[NSScreen mainScreen] visibleFrame];
		
		
		// if by some chance the target window is offscreen, ignore its location
		// and put the window somewhere onscreen
		if (NO == NSContainsRect(mainScreenFrame, oldFrame))
		{
			oldFrame.origin = NSMakePoint(250, 250); // arbitrary
		}
		
		// copy only the relative window’s location, not its size
		oldFrame.size = [inTargetWindow frame].size;
		
		// target a new location that is slightly offset from the related window
		newFrame.origin = oldFrame.origin;
		newFrame.origin.x += 20/* arbitrary */;
		newFrame.origin.y -= 20/* arbitrary */;
		newFrame.size = [inTargetWindow frame].size;
		
		// as a precaution, arrange to move the window to the correct
		// location after a short delay (the animation may fail)
		{
			NSArray*	frameCoordinates = [NSArray arrayWithObjects:
												[NSNumber numberWithFloat:newFrame.origin.x],
												[NSNumber numberWithFloat:newFrame.origin.y],
												[NSNumber numberWithFloat:newFrame.size.width],
												[NSNumber numberWithFloat:newFrame.size.height],
												nil];
			
			
			[inTargetWindow performSelector:@selector(setFrameWithArray:) withObject:frameCoordinates
											afterDelay:(kAnimationDelay + 0.25)];
		}
		
		// animate!
		[imageWindow orderFront:nil];
		[imageWindow setLevel:[inTargetWindow level]];
		[[[CocoaAnimation_WindowFrameAnimator alloc]
			initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:inTargetWindow
								fromFrame:oldFrame toFrame:newFrame totalDelay:0.05 delayDistribution:kMy_AnimationTimeDistributionLinear
								effect:kMy_AnimationEffectFadeIn] autorelease];
	}
}// TransitionWindowForDuplicate


/*!
Animates "inTargetWindow" in a way that suggests it is being
hidden.  The "inEndLocation" should be the rectangle of some
user interface element that would redisplay the window, in
the same coordinate system as an NSWindow’s "frame" would be.

In order to avoid potential problems with the lifetime of a
window, the specified window is actually hidden immediately
and is replaced with a borderless window that renders the
exact same image!  The animation is then applied only to the
window that appears to be the original window.

(1.8)
*/
void
CocoaAnimation_TransitionWindowForHide	(NSWindow*		inTargetWindow,
										 CGRect			inEndLocation)
{
	AutoPool	_;
	NSWindow*	imageWindow = [createImageWindowFrom(inTargetWindow, [[inTargetWindow contentView] bounds]) autorelease];
	NSRect		oldFrame = [imageWindow frame];
	NSRect		newFrame = NSZeroRect;
	
	
	newFrame.origin.x = inEndLocation.origin.x;
	newFrame.origin.y = inEndLocation.origin.y;
	newFrame.size.width = inEndLocation.size.width;
	newFrame.size.height = inEndLocation.size.height;
	
	// animate!
	[imageWindow orderFront:nil];
	[imageWindow setLevel:[inTargetWindow level]];
	[[[CocoaAnimation_WindowFrameAnimator alloc]
		initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
							fromFrame:oldFrame toFrame:newFrame totalDelay:0.04 delayDistribution:kMy_AnimationTimeDistributionEaseOut
							effect:kMy_AnimationEffectNone] autorelease];
	
	// hide the original window immediately; the animation on the
	// image window can take however long it needs to complete
	[inTargetWindow orderOut:nil];
}// TransitionWindowForHide


/*!
Animates "inTargetWindow" in a way that suggests it is being
destroyed.

In order to avoid potential problems with the lifetime of a
window, the specified window is actually hidden immediately
and is replaced with a borderless window that renders the
exact same image!  The animation is then applied only to the
window that appears to be the original window.

(1.8)
*/
void
CocoaAnimation_TransitionWindowForRemove	(NSWindow*		inTargetWindow)
{
	AutoPool	_;
	
	
	if ([inTargetWindow isVisible])
	{
		NSWindow*	imageWindow = [createImageWindowFrom(inTargetWindow, [[inTargetWindow contentView] bounds]) autorelease];
		NSRect		oldFrame = [imageWindow frame];
		NSRect		newFrame = [imageWindow frame];
		NSRect		screenFrame = [[inTargetWindow screen] frame];
		
		
		// target a new location that appears to toss the window away
		newFrame.origin.x += (screenFrame.size.width / 3)/* arbitrary */;
		newFrame.origin.y -= (screenFrame.size.height / 3)/* arbitrary */;
		newFrame.size.width /= 4; // arbitrary
		newFrame.size.height /= 4; // arbitrary
		
		// animate!
		[imageWindow orderFront:nil];
		[imageWindow setLevel:[inTargetWindow level]];
		[[[CocoaAnimation_WindowFrameAnimator alloc]
			initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
								fromFrame:oldFrame toFrame:newFrame totalDelay:0.03 delayDistribution:kMy_AnimationTimeDistributionEaseOut
								effect:kMy_AnimationEffectFadeOut] autorelease];
		
		// hide the original window immediately; the animation on the
		// image window can take however long it needs to complete
		[inTargetWindow orderOut:nil];
	}
}// TransitionWindowForRemove


/*!
Animates the specified section of "inTargetWindow" in a way that
makes it seem to “open”, starting at the specified frame.  The
rectangle "inStartLocation" is relative to the boundaries of the
content view, not its frame (in other words, the location and
scaling of the frame are irrelevant).

The animation takes place entirely in a separate window, although
the initial frames are meant to appear integrated with the
original window.

(1.8)
*/
void
CocoaAnimation_TransitionWindowSectionForOpen	(NSWindow*		inTargetWindow,
												 CGRect			inStartLocation)
{
	AutoPool	_;
	NSRect		targetWindowFrame = [inTargetWindow contentRectForFrameRect:[inTargetWindow frame]];
	NSWindow*	imageWindow = [createImageWindowFrom
								(inTargetWindow, NSMakeRect(inStartLocation.origin.x, inStartLocation.origin.y,
															inStartLocation.size.width, inStartLocation.size.height)) autorelease];
	NSRect		oldFrame = [imageWindow frame];
	NSRect		newFrame = oldFrame;
	
	
	// this effect uses a shadow
	[imageWindow setHasShadow:YES];
	
	// make the section appear to zoom open
	newFrame.size.width *= 4; // arbitrary
	newFrame.size.height *= 4; // arbitrary
	newFrame.origin.x = oldFrame.origin.x - ((newFrame.size.width - oldFrame.size.width) / 2.0);
	newFrame.origin.y = oldFrame.origin.y - ((newFrame.size.height - oldFrame.size.height) / 2.0);
	
	// animate!
	[imageWindow orderFront:nil];
	[imageWindow setLevel:[inTargetWindow level]];
	[[[CocoaAnimation_WindowFrameAnimator alloc]
		initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
							fromFrame:oldFrame toFrame:newFrame totalDelay:0.05 delayDistribution:kMy_AnimationTimeDistributionEaseOut
							effect:kMy_AnimationEffectFadeOut] autorelease];
}// TransitionWindowSectionForOpen


/*!
Animates the specified section of "inTargetWindow" in a way
that makes it seem like matching text in a search, starting at
the specified frame.  The rectangle "inStartLocation" is relative
to the boundaries of the content view, not its frame (in other
words, the location and scaling of the frame are irrelevant).

The animation takes place entirely in a separate window, although
the initial frames are meant to appear integrated with the
original window.

(1.8)
*/
void
CocoaAnimation_TransitionWindowSectionForSearchResult	(NSWindow*		inTargetWindow,
														 CGRect			inStartLocation)
{
	AutoPool	_;
	NSRect		targetWindowFrame = [inTargetWindow contentRectForFrameRect:[inTargetWindow frame]];
	NSWindow*	imageWindow = [createImageWindowFrom
								(inTargetWindow, NSMakeRect(inStartLocation.origin.x, inStartLocation.origin.y,
															inStartLocation.size.width, inStartLocation.size.height)) autorelease];
	NSRect		oldFrame = [imageWindow frame];
	NSRect		newFrame = oldFrame;
	
	
	// this effect uses a shadow
	[imageWindow setHasShadow:YES];
	
	// make the section appear to zoom down slightly;
	// also make the closing frame bigger than the
	// original region so that the selected area
	// appears to be floating the entire time
	newFrame.size.width += 8; // arbitrary
	newFrame.size.height += 8; // arbitrary
	newFrame.origin.x = oldFrame.origin.x - ((newFrame.size.width - oldFrame.size.width) / 2.0);
	newFrame.origin.y = oldFrame.origin.y - ((newFrame.size.height - oldFrame.size.height) / 2.0);
	oldFrame.size.width += 25; // arbitrary
	oldFrame.size.height += 25; // arbitrary
	oldFrame.origin.x = newFrame.origin.x - ((oldFrame.size.width - newFrame.size.width) / 2.0);
	oldFrame.origin.y = newFrame.origin.y - ((oldFrame.size.height - newFrame.size.height) / 2.0);
	
	// animate!
	[imageWindow orderFront:nil];
	[imageWindow setLevel:[inTargetWindow level]];
	[[[CocoaAnimation_WindowFrameAnimator alloc]
		initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
							fromFrame:oldFrame toFrame:newFrame totalDelay:0.08 delayDistribution:kMy_AnimationTimeDistributionLinear
							effect:kMy_AnimationEffectFadeIn] autorelease];
}// TransitionWindowSectionForSearchResult


#pragma mark Internal Methods
namespace {

/*!
Creates a new, borderless window whose content view is an
image view that renders the specified portion of the given
window.   The rectangle is a section of the content view’s
bounds (that is, the unscaled region independent of any
chosen frame), so use "[[inWindow contentView] bounds]" to
capture the entire window.

This is very useful as a basis for animations, because it
allows a window to appear to be moving in a way that does
not require the original window to even exist.  It also
tends to be much more lightweight.

(1.8)
*/
NSWindow*
createImageWindowFrom	(NSWindow*		inWindow,
						 NSRect			inContentViewSection)
{
	NSWindow*	result = nil;
	NSView*		originalContentView = [inWindow contentView];
	NSRect		zeroOriginBounds = inContentViewSection;
	
	
	zeroOriginBounds.origin = NSZeroPoint;
	
	// capture the image of the original window
	{
		NSBitmapImageRep*	imageRep = nil;
		NSImage*			windowImage = [[[NSImage alloc] init] autorelease];
		NSImageView*		imageView = [[[NSImageView alloc] initWithFrame:zeroOriginBounds] autorelease];
		NSRect				newFrame = [inWindow frame];
		
		
		// capture the contents of the original window
		[originalContentView lockFocus];
		imageRep = [[[NSBitmapImageRep alloc] initWithFocusedViewRect:inContentViewSection] autorelease];
		[originalContentView unlockFocus];
		[windowImage addRepresentation:imageRep];
		[imageView setImageScaling:NSScaleToFit];
		[imageView setImage:windowImage];
		
		// set up the window to cover its original content exactly
		newFrame.origin.x += inContentViewSection.origin.x;
		newFrame.origin.y += inContentViewSection.origin.y;
		newFrame.size = inContentViewSection.size;
		
		// now construct a fake window to display the same thing
		result = [[NSWindow alloc] initWithContentRect:newFrame styleMask:NSBorderlessWindowMask
														backing:NSBackingStoreBuffered defer:NO];
		[result setContentView:imageView];
	}
	
	return result;
}// createImageWindowFrom

} // anonymous namespace


@implementation CocoaAnimation_WindowFrameAnimator


/*!
Designated initializer.

(1.8)
*/
- (id)
initWithTransition:(My_AnimationTransition)			aTransition
imageWindow:(NSWindow*)								aBorderlessWindow
finalWindow:(NSWindow*)								theActualWindow
fromFrame:(NSRect)									sourceRect
toFrame:(NSRect)									targetRect
totalDelay:(NSTimeInterval)							aDuration
delayDistribution:(My_AnimationTimeDistribution)	aDistribution
effect:(My_AnimationEffect)							anEffect
simplified:(BOOL)									isSimplified
{
	self = [super init];
	if (nil != self)
	{
		NSTimeInterval		baseDuration = (isSimplified)
											? aDuration / 2.0
											: aDuration;
		
		
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
		
		// start by configuring offsets in terms of a UNIT SQUARE,
		// which means that the sum of all frame offsets should
		// add up to the total number of frames (at the end, all
		// offsets are multiplied by the unit distances to produce
		// actual frame offsets); remember that the frame has its
		// origin in the bottom-left, not the top-left
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
			{
				size_t		i = 0;
				
				
				if (5 == self->frameCount)
				{
					self->frameAlphas[i++] = 0.2;
					self->frameAlphas[i++] = 0.4;
					self->frameAlphas[i++] = 0.6;
					self->frameAlphas[i++] = 0.8;
					self->frameAlphas[i++] = 1.0;
				}
				else
				{
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
				}
				assert(i == self->frameCount);
			}
			break;
		
		case kMy_AnimationEffectFadeOut:
			{
				size_t		i = 0;
				
				
				if (5 == self->frameCount)
				{
					self->frameAlphas[i++] = 0.7;
					self->frameAlphas[i++] = 0.5;
					self->frameAlphas[i++] = 0.3;
					self->frameAlphas[i++] = 0.2;
					self->frameAlphas[i++] = 0.1;
				}
				else
				{
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
				}
				assert(i == self->frameCount);
			}
			break;
		
		default:
			// do nothing
			break;
		}
		
		// configure delays; for some delays an algorithm is applied to
		// create a curve against a unit scale, so that the loop portion
		// just multiplies the linear amount against the curve
		// (TEMPORARY; for large delays the frame count probably should
		// be increased to smooth out the animation over time, and right
		// now the frame count is fairly inflexible)
		{
			float const		kPerUnitLinearDelay = baseDuration / self->frameCount;
			
			
			if (kMy_AnimationTimeDistributionEaseOut == aDistribution)
			{
				// assign initial values assuming a unit curve only; be
				// sure that the total number is equal to the frame count
				// so that any user-specified total delay is not exceeded
				size_t		i = 0;
				
				
				if (5 == self->frameCount)
				{
					self->frameDelays[i++] = 0.7;
					self->frameDelays[i++] = 0.8;
					self->frameDelays[i++] = 1.0;
					self->frameDelays[i++] = 1.2;
					self->frameDelays[i++] = 1.3;
				}
				else
				{
					self->frameDelays[i++] = 0.7;
					self->frameDelays[i++] = 0.8;
					self->frameDelays[i++] = 0.9;
					self->frameDelays[i++] = 0.9;
					self->frameDelays[i++] = 1.0;
					self->frameDelays[i++] = 1.0;
					self->frameDelays[i++] = 1.1;
					self->frameDelays[i++] = 1.1;
					self->frameDelays[i++] = 1.2;
					self->frameDelays[i++] = 1.3;
				}
				assert(i == self->frameCount);
			}
			for (size_t i = 0; i < self->frameCount; ++i)
			{
				if (kMy_AnimationTimeDistributionEaseOut == aDistribution)
				{
					// scale the unit curve accordingly
					self->frameDelays[i] *= kPerUnitLinearDelay;
				}
				else
				{
					// linear
					self->frameDelays[i] = kPerUnitLinearDelay;
				}
			}
		}
		
		// calculate the size of each unit of the animation
		self->frameUnitH = (self->targetFrame.origin.x - self->originalFrame.origin.x) / self->frameCount;
		self->frameUnitV = (self->targetFrame.origin.y - self->originalFrame.origin.y) / self->frameCount;
		self->frameDeltaSizeH = (self->targetFrame.size.width - self->originalFrame.size.width) / self->frameCount;
		self->frameDeltaSizeV = (self->targetFrame.size.height - self->originalFrame.size.height) / self->frameCount;
		
		// finally, scale the offsets based on these units
		for (size_t i = 0; i < self->frameCount; ++i)
		{
			self->frameOffsetsH[i] *= self->frameUnitH;
			self->frameOffsetsV[i] *= self->frameUnitV;
		}
		
		// begin animation
		[self animationStep:nil];
	}
	return self;
}// initWithTransition:imageWindow:finalWindow:fromFrame:toFrame:totalDelay:delayDistribution:effect:simplified:


/*!
Automatically decides when to reduce the complexity of the
animation (on older machines).

(1.8)
*/
- (id)
initWithTransition:(My_AnimationTransition)			aTransition
imageWindow:(NSWindow*)								aBorderlessWindow
finalWindow:(NSWindow*)								theActualWindow
fromFrame:(NSRect)									sourceRect
toFrame:(NSRect)									targetRect
totalDelay:(NSTimeInterval)							aDuration
delayDistribution:(My_AnimationTimeDistribution)	aDistribution
effect:(My_AnimationEffect)							anEffect
{
	return [self initWithTransition:aTransition imageWindow:aBorderlessWindow finalWindow:theActualWindow
									fromFrame:sourceRect toFrame:targetRect totalDelay:aDuration
									delayDistribution:aDistribution effect:anEffect
									simplified:(NSAppKitVersionNumber < NSAppKitVersionNumber10_6)/* arbitrary */];
}// initWithTransition:imageWindow:finalWindow:fromFrame:toFrame:totalDelay:delayDistribution:effect:


/*!
Destructor.

(1.8)
*/
- (void)
dealloc
{
	// if the animation is released too soon, force the correct window location
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

(1.8)
*/
- (void)
animationStep:(id)	unused
{
#pragma unused(unused)
	NSRect	newFrame = self->originalFrame;
	
	
	// the offsets assume a unit square and scale according to the actual
	// difference between the original frame and the target frame; an
	// offset of 1 when there are 5 frames means “20% of the distance from
	// the beginning”, 2 means 40%, etc. so offsets should generally be
	// well represented across the spectrum from 0 to the frame count
	newFrame.origin.x += self->frameOffsetsH[self->currentFrame];
	newFrame.origin.y += self->frameOffsetsV[self->currentFrame];
	if (0 != self->frameDeltaSizeH)
	{
		float	powerOfTwo = 0.0;
		
		
		newFrame.size.width += (self->currentFrame * self->frameDeltaSizeH);
		
		// if the new width is arbitrarily close to a power of 2, it
		// is converted into an exact power of 2 so that optimizations
		// are possible based on this (but not necessarily guaranteed)
		powerOfTwo = (1 << STATIC_CAST(ceilf(log2f(newFrame.size.width)), unsigned int));
		if (fabsf(newFrame.size.width - powerOfTwo) < 5/* pixels; arbitrary */)
		{
			newFrame.size.width = powerOfTwo;
		}
	}
	if (0 != self->frameDeltaSizeV)
	{
		float	powerOfTwo = 0.0;
		
		
		newFrame.size.height += (self->currentFrame * self->frameDeltaSizeV);
		
		// if the new height is arbitrarily close to a power of 2, it
		// is converted into an exact power of 2 so that optimizations
		// are possible based on this (but not necessarily guaranteed)
		powerOfTwo = (1 << STATIC_CAST(ceilf(log2f(newFrame.size.height)), unsigned int));
		if (fabsf(newFrame.size.height - powerOfTwo) < 5/* pixels; arbitrary */)
		{
			newFrame.size.height = powerOfTwo;
		}
	}
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
