/*!	\file CocoaAnimation.mm
	\brief Animation utilities wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2023 by Kevin Grant
	
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
@import Cocoa;

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>



#pragma mark Constants
namespace {

size_t const	kCurveLength = 8; // arbitrary (number of elements in curve arrays)

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
Transitions used by CocoaAnimation_WindowFrameAnimator.  These
determine the kinds of steps that are taken to move from the
source frame to the target frame.
*/
enum My_AnimationTransition
{
	kMy_AnimationTransitionSlide	= 0		//!< window moves straight from source to destination frame
};

} // anonymous namespace

#pragma mark Types

@class CocoaAnimation_WindowFrameAnimator;


/*!
A class of window that has no active content, just
an image that appears similar to another window.
This window persists for the duration of animations
and then disappears (usually with some effect like
a reduced size or alpha fade-out).

The public CocoaAnimation_TransitionWindow...() APIs
create image windows and associate animation managers
with them.  That way, the animation’s lifetime is
extended to match the image window itself.
*/
@interface CocoaAnimation_ImageWindow : NSWindow //{

// accessors
	@property (assign) NSTimeInterval
	animationResizeTime;
	@property (strong) CocoaAnimation_WindowFrameAnimator*
	frameAnimationManager;

@end //}


/*!
A class for performing animations on windows that
works on older Mac OS X versions (that is, before
Core Animation was available).
*/
@interface CocoaAnimation_WindowFrameAnimator : NSObject //{

// new methods
	- (void)
	nextAnimationStep;

// accessors
	@property (strong) NSWindow*
	actualWindow;
	@property (assign) BOOL
	animationDone;
	@property (weak) CocoaAnimation_ImageWindow*
	borderlessWindow;
	@property (assign) unsigned int
	currentStep;
	@property (assign) float*
	frameAlphas;
	@property (assign) unsigned int
	frameCount;
	@property (assign) float
	frameDeltaSizeH;
	@property (assign) float
	frameDeltaSizeV;
	@property (assign) float*
	frameOffsetsH;
	@property (assign) float*
	frameOffsetsV;
	@property (assign) float
	frameUnitH;
	@property (assign) float
	frameUnitV;
	@property (assign) NSRect
	originalFrame;
	@property (assign) NSRect
	targetFrame;

// initializers
	- (instancetype)
	initWithTransition:(My_AnimationTransition)_
	imageWindow:(CocoaAnimation_ImageWindow*)_
	finalWindow:(NSWindow*)_
	fromFrame:(NSRect)_
	toFrame:(NSRect)_
	effect:(My_AnimationEffect)_
	simplified:(BOOL)_ NS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithTransition:(My_AnimationTransition)_
	imageWindow:(CocoaAnimation_ImageWindow*)_
	finalWindow:(NSWindow*)_
	fromFrame:(NSRect)_
	toFrame:(NSRect)_
	effect:(My_AnimationEffect)_;

@end //}


#pragma mark Internal Method Prototypes
namespace {

CocoaAnimation_ImageWindow*		createImageWindowFromImage			(NSWindow*, NSImage*, NSRect = NSZeroRect);
CocoaAnimation_ImageWindow*		createImageWindowFromWindowRect		(NSWindow*, NSRect);

} // anonymous namespace

#pragma mark Variables
namespace {

SInt16		gAnimationsInProgress = 0;

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
@autoreleasepool {
	// show the window offscreen so its image is defined
	[inTargetWindow setFrameTopLeftPoint:NSMakePoint(-5000, -5000)];
	[inTargetWindow orderFront:nil];
	
	// animate the change
	{
		NSRect							oldFrame = inRelativeToWindow.frame;
		NSRect							newFrame = NSZeroRect;
		NSRect							mainScreenFrame = [NSScreen mainScreen].visibleFrame;
		CocoaAnimation_ImageWindow*		imageWindow = createImageWindowFromWindowRect(inTargetWindow, STATIC_CAST(inTargetWindow.contentView, NSView*).frame);
		
		
		NSLog(@"? %@", imageWindow);
		// target a new location that is slightly offset from the related window
		newFrame.origin = oldFrame.origin;
		newFrame.origin.x += 20/* arbitrary */;
		newFrame.origin.y -= 20/* arbitrary */;
		newFrame.size = inTargetWindow.frame.size;
		
		// aim to keep the top-left corner on the screen
		if (NO == NSPointInRect(NSMakePoint(newFrame.origin.x, newFrame.origin.y + NSHeight(newFrame)), mainScreenFrame))
		{
			// fix location (it is relative to the bottom-left of the screen)
			newFrame.origin.x = (mainScreenFrame.origin.x + 150); // arbitrary
			newFrame.origin.y = (mainScreenFrame.origin.y + mainScreenFrame.size.height - newFrame.size.height - 40); // arbitrary
		}
		
		// as a precaution, arrange to move the window to the correct
		// location after a short delay (the animation may fail)
		__weak decltype(inTargetWindow)		weakTargetWindow = inTargetWindow;
		CocoaExtensions_RunLater(0.30/* arbitrary */,
									^{ [weakTargetWindow setFrame:newFrame display:YES]; });
		
		// animate!
		[imageWindow orderFront:nil];
		imageWindow.level = inTargetWindow.level;
		imageWindow.frameAnimationManager = [[CocoaAnimation_WindowFrameAnimator alloc]
												initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:inTargetWindow
																	fromFrame:oldFrame toFrame:newFrame
																	effect:kMy_AnimationEffectFadeIn];
	}
}// @autoreleasepool
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
@autoreleasepool {
	CocoaAnimation_ImageWindow*		imageWindow = createImageWindowFromWindowRect(inTargetWindow, STATIC_CAST(inTargetWindow.contentView, NSView*).frame);
	NSRect							oldFrame = imageWindow.frame;
	NSRect							newFrame = NSZeroRect;
	
	
	newFrame.origin.x = inEndLocation.origin.x;
	newFrame.origin.y = inEndLocation.origin.y;
	newFrame.size.width = inEndLocation.size.width;
	newFrame.size.height = inEndLocation.size.height;
	
	// animate!
	[imageWindow orderFront:nil];
	imageWindow.level = inTargetWindow.level;
	imageWindow.frameAnimationManager = [[CocoaAnimation_WindowFrameAnimator alloc]
											initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
																fromFrame:oldFrame toFrame:newFrame
																effect:kMy_AnimationEffectNone];
	
	// hide the original window immediately; the animation on the
	// image window can take however long it needs to complete
	[inTargetWindow orderOut:nil];
}// @autoreleasepool
}// TransitionWindowForHide


/*!
Animates "inTargetWindow" to a new frame "inEndLocation"
(which should be an NSRect for the frame expressed as a
CGRect structure).

The animation does not hide the window and it only moves the
real window once (after a delay).  The animation makes it
appear as if the window physically occupies intermediate
positions however.

(1.10)
*/
void
CocoaAnimation_TransitionWindowForMove	(NSWindow*		inTargetWindow,
										 CGRect			inEndLocation)
{
@autoreleasepool {
	CocoaAnimation_ImageWindow*		imageWindow = createImageWindowFromWindowRect(inTargetWindow, STATIC_CAST(inTargetWindow.contentView, NSView*).frame);
	NSRect							oldFrame = imageWindow.frame;
	NSRect							newFrame = NSZeroRect;
	
	
	newFrame.origin.x = inEndLocation.origin.x;
	newFrame.origin.y = inEndLocation.origin.y;
	newFrame.size.width = inEndLocation.size.width;
	newFrame.size.height = inEndLocation.size.height;
	
	// animate!
	[imageWindow orderFront:nil];
	imageWindow.level = inTargetWindow.level;
	imageWindow.frameAnimationManager = [[CocoaAnimation_WindowFrameAnimator alloc]
											initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
																fromFrame:oldFrame toFrame:newFrame
																effect:kMy_AnimationEffectFadeIn];
	
	// the original window moves after a short delay
	__weak decltype(inTargetWindow)		weakTargetWindow = inTargetWindow;
	CocoaExtensions_RunLater(0.4, ^{ [weakTargetWindow setFrame:newFrame display:YES]; });
}// @autoreleasepool
}// TransitionWindowForMove


/*!
Animates "inTargetWindow" in a way that suggests it is being
destroyed.

If "inIsConfirming" is true, the animation may change to show
that changes in the window are being accepted (as opposed to
discarded entirely), which is a useful distinction for dialogs.

In order to avoid potential problems with the lifetime of a
window, the specified window is actually hidden immediately
and is replaced with a borderless window that renders the
exact same image!  The animation is then applied only to the
window that appears to be the original window.

(1.8)
*/
void
CocoaAnimation_TransitionWindowForRemove	(NSWindow*		inTargetWindow,
											 Boolean		inIsConfirming)
{
@autoreleasepool {
	if ([inTargetWindow isVisible])
	{
		CocoaAnimation_ImageWindow*		imageWindow = createImageWindowFromWindowRect(inTargetWindow, STATIC_CAST(inTargetWindow.contentView, NSView*).frame);
		NSRect							oldFrame = imageWindow.frame;
		NSRect							newFrame = imageWindow.frame;
		NSRect							screenFrame = inTargetWindow.screen.frame;
		
		
		if (inIsConfirming)
		{
			// target a new location that appears to preserve the window’s changes
			newFrame.origin.x += (oldFrame.size.width / 4);
			newFrame.origin.y += (oldFrame.size.height / 4);
			newFrame.size.width /= 2; // arbitrary
			newFrame.size.height /= 2; // arbitrary
		}
		else
		{
			// target a new location that appears to toss the window away
			newFrame.origin.x += (screenFrame.size.width / 3)/* arbitrary */;
			newFrame.origin.y -= (screenFrame.size.height / 3)/* arbitrary */;
			newFrame.size.width /= 4; // arbitrary
			newFrame.size.height /= 4; // arbitrary
		}
		
		// animate!
		[imageWindow orderFront:nil];
		imageWindow.level = inTargetWindow.level;
		imageWindow.frameAnimationManager = [[CocoaAnimation_WindowFrameAnimator alloc]
												initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
																	fromFrame:oldFrame toFrame:newFrame
																	effect:kMy_AnimationEffectFadeOut];
		
		// hide the original window immediately; the animation on the
		// image window can take however long it needs to complete
		[inTargetWindow orderOut:nil];
	}
}// @autoreleasepool
}// TransitionWindowForRemove


/*!
Animates "inTargetWindow" as if it were a sheet opening on a
parent window.

If "inTargetWindow" implements CocoaAnimation_WindowImageProvider,
that protocol determines the appearance of the animation window;
otherwise, the image is inferred by asking the "contentView" of
"inTargetWindow" to render itself (which may omit frame details).

(1.10)
*/
void
CocoaAnimation_TransitionWindowForSheetOpen		(NSWindow*		inTargetWindow,
												 NSWindow*		UNUSED_ARGUMENT(inRelativeToWindow))
{
@autoreleasepool {
	NSRect		actualFrame = inTargetWindow.frame;
	BOOL		useAnimation = ((NSWidth(actualFrame) * NSHeight(actualFrame)) < 250000/* arbitrary */); // avoid if window is large
	
	
	if (useAnimation)
	{
		float const						kAnimationDelay = 0.002f;
		float const						kXInset = (0.18f * NSWidth(actualFrame)); // arbitrary
		float const						kYInset = (0.18f * NSHeight(actualFrame)); // keep aspect ratio the same
		NSRect							oldFrame = actualFrame;
		NSRect							newFrame = actualFrame;
		CocoaAnimation_ImageWindow*		imageWindow = nil;
		
		
		// show the window offscreen so its image is defined
		[inTargetWindow setFrameTopLeftPoint:NSMakePoint(-5000, -5000)];
		[inTargetWindow orderFront:nil];
		
		if ([inTargetWindow conformsToProtocol:@protocol(CocoaAnimation_WindowImageProvider)])
		{
			id< CocoaAnimation_WindowImageProvider >	asProvider = STATIC_CAST(inTargetWindow,
																					id< CocoaAnimation_WindowImageProvider >);
			NSImage*	frameImage = [asProvider windowImage];
			
			
			imageWindow = createImageWindowFromImage(inTargetWindow, frameImage);
		}
		else
		{
			imageWindow = createImageWindowFromWindowRect(inTargetWindow, STATIC_CAST(inTargetWindow.contentView, NSView*).frame);
		}
		
		// start from a location that is slightly offset from the target window
		oldFrame.origin.x += kXInset; // arbitrary
		oldFrame.origin.y += kYInset; // arbitrary
		oldFrame.size.width -= (2.0 * kXInset);
		oldFrame.size.height -= (2.0 * kYInset);
		
		// as a precaution, arrange to move the window to the correct
		// location after a short delay (the animation may fail)
		__weak decltype(inTargetWindow)		weakTargetWindow = inTargetWindow;
		CocoaExtensions_RunLater((kAnimationDelay + 0.25),
									^{ [weakTargetWindow setFrame:newFrame display:YES]; });
		
		// animate!
		[imageWindow orderFront:nil];
		imageWindow.level = (1 + inTargetWindow.level);
		imageWindow.frameAnimationManager = [[CocoaAnimation_WindowFrameAnimator alloc]
												initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:inTargetWindow
																	fromFrame:oldFrame toFrame:newFrame
																	effect:kMy_AnimationEffectFadeIn];
	}
	else
	{
		// no animation; show the window immediately
		[inTargetWindow setFrame:actualFrame display:NO];
		[inTargetWindow orderFront:nil];
	}
}// @autoreleasepool
}// TransitionWindowForSheetOpen


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
@autoreleasepool {
	CocoaAnimation_ImageWindow*		imageWindow = createImageWindowFromWindowRect
													(inTargetWindow, NSMakeRect(inStartLocation.origin.x, inStartLocation.origin.y,
																				inStartLocation.size.width, inStartLocation.size.height));
	NSRect							oldFrame = imageWindow.frame;
	NSRect							newFrame = oldFrame;
	
	
	// this effect uses a shadow
	imageWindow.hasShadow = YES;
	
	// make the section appear to zoom open
	newFrame.size.width *= 4; // arbitrary
	newFrame.size.height *= 4; // arbitrary
	newFrame.origin.x = oldFrame.origin.x - CGFLOAT_DIV_2(newFrame.size.width - oldFrame.size.width);
	newFrame.origin.y = oldFrame.origin.y - CGFLOAT_DIV_2(newFrame.size.height - oldFrame.size.height);
	
	// animate!
	[imageWindow orderFront:nil];
	imageWindow.level = inTargetWindow.level;
	imageWindow.frameAnimationManager = [[CocoaAnimation_WindowFrameAnimator alloc]
											initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
																fromFrame:oldFrame toFrame:newFrame
																effect:kMy_AnimationEffectFadeOut];
}// @autoreleasepool
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
@autoreleasepool {
	CocoaAnimation_ImageWindow*		imageWindow = createImageWindowFromWindowRect
													(inTargetWindow, NSMakeRect(inStartLocation.origin.x, inStartLocation.origin.y,
																				inStartLocation.size.width, inStartLocation.size.height));
	NSRect							oldFrame = imageWindow.frame;
	NSRect							newFrame = oldFrame;
	
	
	// this effect uses a shadow
	imageWindow.hasShadow = YES;
	
	// make the section appear to zoom down slightly;
	// also make the closing frame bigger than the
	// original region so that the selected area
	// appears to be floating the entire time
	newFrame.size.width += 8; // arbitrary
	newFrame.size.height += 8 * (newFrame.size.height / newFrame.size.width); // arbitrary
	newFrame.origin.x = oldFrame.origin.x - CGFLOAT_DIV_2(newFrame.size.width - oldFrame.size.width);
	newFrame.origin.y = oldFrame.origin.y - CGFLOAT_DIV_2(newFrame.size.height - oldFrame.size.height);
	oldFrame.size.width += 128; // arbitrary
	oldFrame.size.height += 128 * (newFrame.size.height / newFrame.size.width); // arbitrary
	oldFrame.origin.x = newFrame.origin.x - CGFLOAT_DIV_2(oldFrame.size.width - newFrame.size.width);
	oldFrame.origin.y = newFrame.origin.y - CGFLOAT_DIV_2(oldFrame.size.height - newFrame.size.height);
	
	// animate!
	[imageWindow orderFront:nil];
	imageWindow.level = inTargetWindow.level;
	imageWindow.frameAnimationManager = [[CocoaAnimation_WindowFrameAnimator alloc]
											initWithTransition:kMy_AnimationTransitionSlide imageWindow:imageWindow finalWindow:nil
																fromFrame:oldFrame toFrame:newFrame
																effect:kMy_AnimationEffectFadeIn];
}// @autoreleasepool
}// TransitionWindowSectionForSearchResult


#pragma mark Internal Methods
namespace {

/*!
Creates a new, borderless window whose content view is an image
view that renders the specified image.  The image view size is
set to match the frame origin and size of the original window,
regardless of the image size, unless a custom rectangle is given.
(The custom rectangle is relative to the window origin.)

This is very useful as a basis for animations, because it allows
a window to appear to be moving in a way that does not require
the original window to continue to exist after this call returns.

(2017.06)
*/
CocoaAnimation_ImageWindow*
createImageWindowFromImage	(NSWindow*	inWindow,
							 NSImage*	inImage,
							 NSRect		inCustomRectOrZeroRect)
{
	CocoaAnimation_ImageWindow*		result = nil;
	NSRect							windowFrame = inWindow.frame;
	NSRect							contentFrame = [inWindow contentRectForFrameRect:windowFrame];
	NSRect							newFrame = (NSIsEmptyRect(inCustomRectOrZeroRect)
												? windowFrame
												: NSOffsetRect(inCustomRectOrZeroRect, contentFrame.origin.x, contentFrame.origin.y));
	
	
	// guard against a possible exception if the given rectangle is bogus
	if ((false == isfinite(newFrame.origin.x)) || (false == isfinite(newFrame.origin.y)) ||
		(false == isfinite(newFrame.size.width)) || (false == isfinite(newFrame.size.height)))
	{
		Console_Warning(Console_WriteValueFloat4, "failed to create image window from rectangle at infinity",
						newFrame.origin.x, newFrame.origin.y, newFrame.size.width, newFrame.size.height);
		return nil;
	}
	
	// construct a fake window to display the same thing
	result = [[CocoaAnimation_ImageWindow alloc]
				initWithContentRect:newFrame
									styleMask:NSWindowStyleMaskBorderless
									backing:NSBackingStoreBuffered
									defer:NO];
	result.opaque = NO;
	result.backgroundColor = [NSColor clearColor];
	
	// capture the image of the original window (a regular image view is used
	// instead of just assigning the image to a layer because the documentation
	// for CALayer states that setting the "contents" of a layer is not correct
	// when the layer is associated with a view; on macOS High Sierra, an image
	// assigned to a layer in a view is not rendered at all)
	@autoreleasepool
	{
		NSRect			zeroOriginBounds = NSMakeRect(0, 0, NSWidth(newFrame), NSHeight(newFrame));
		NSImageView*	imageView = [[NSImageView alloc] initWithFrame:zeroOriginBounds];
		
		
		imageView.imageScaling = NSImageScaleAxesIndependently;
		imageView.image = inImage;
		result.contentView = imageView;
	}
	
	return result;
}// createImageWindowFromImage


/*!
Creates a new, borderless window whose content view is an
image view that renders the specified portion of the given
window.   The rectangle is a section of the content view’s
bounds (that is, the unscaled region independent of any
chosen frame), so use "inWindow.contentView.frame" to
capture the entire window.

This is very useful as a basis for animations, because it
allows a window to appear to be moving in a way that does
not require the original window to even exist.  It also
tends to be much more lightweight.

(1.8)
*/
CocoaAnimation_ImageWindow*
createImageWindowFromWindowRect		(NSWindow*		inWindow,
									 NSRect			inContentViewSection)
{
	CocoaAnimation_ImageWindow*		result = nil;
	NSView*							originalContentView = inWindow.contentView;
	
	
	// capture the image of the original window
	@autoreleasepool
	{
		NSBitmapImageRep*	imageRep = [originalContentView bitmapImageRepForCachingDisplayInRect:inContentViewSection];
		NSImage*			windowImage = [[NSImage alloc] init];
		
		
		// capture the contents of the original window
		[originalContentView cacheDisplayInRect:inContentViewSection toBitmapImageRep:imageRep];
		[windowImage addRepresentation:imageRep];
		
		// create a window to display this image
		result = createImageWindowFromImage(inWindow, windowImage, inContentViewSection);
	}
	
	return result;
}// createImageWindowFromWindowRect

} // anonymous namespace


#pragma mark -
@implementation CocoaAnimation_ImageWindow //{


#pragma mark NSWindow


/*!
Overrides animation time so that custom image windows
resize at the required rate.

(2022.12)
*/
- (NSTimeInterval)
animationResizeTime:(NSRect)	newFrame
{
	NSTimeInterval		result = 0;
	
	
	if (0 == self.animationResizeTime)
	{
		result = [super animationResizeTime:newFrame];
	}
	else
	{
		result = self.animationResizeTime;
	}
	
	return result;
}// animationResizeTime:


@end //}


#pragma mark -
@implementation CocoaAnimation_WindowFrameAnimator //{


#pragma mark Initializers


/*!
Designated initializer.

The given CocoaAnimation_ImageWindow is held only by weak
reference.  The lifetime of the animation is expected to
match the lifetime of the image window, which can be done
by setting the image window’s "frameAnimationManager" to
this CocoaAnimation_WindowFrameAnimator instance.

(2021.01)
*/
- (instancetype)
initWithTransition:(My_AnimationTransition)			aTransition
imageWindow:(CocoaAnimation_ImageWindow*)			aBorderlessWindow
finalWindow:(NSWindow*)								theActualWindow
fromFrame:(NSRect)									sourceRect
toFrame:(NSRect)									targetRect
effect:(My_AnimationEffect)							anEffect
simplified:(BOOL)									isSimplified
{
	self = [super init];
	if (nil != self)
	{
		BOOL	reduceFrameRate = (gAnimationsInProgress > 2/* arbitrary */)
									? YES
									: isSimplified;
		
		
		// track animations globally as a crude way to estimate when too much
		// is going on (in which case automatic simplifications are made)
		++gAnimationsInProgress;
		
		_borderlessWindow = aBorderlessWindow;
		_actualWindow = theActualWindow;
		_originalFrame = sourceRect;
		_targetFrame = targetRect;
		_frameCount = (reduceFrameRate) ? 3 : 4;
		_currentStep = 0;
		_animationDone = NO;
		if ((kMy_AnimationEffectFadeIn == anEffect) ||
			(kMy_AnimationEffectFadeOut == anEffect))
		{
			_frameAlphas = new float[kCurveLength];
		}
		else
		{
			_frameAlphas = nullptr;
		}
		_frameOffsetsH = new float[kCurveLength];
		_frameOffsetsV = new float[kCurveLength];
		
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
			for (size_t i = 0; i < kCurveLength; ++i)
			{
				_frameOffsetsH[i] = i;
				_frameOffsetsV[i] = i;
			}
			break;
		}
		
		// configure alpha values, if applicable
		switch (anEffect)
		{
		case kMy_AnimationEffectFadeIn:
			{
				size_t		i = 0;
				
				
				_frameAlphas[i++] = 0.3f;
				_frameAlphas[i++] = 0.4f;
				_frameAlphas[i++] = 0.5f;
				_frameAlphas[i++] = 0.6f;
				_frameAlphas[i++] = 0.7f;
				_frameAlphas[i++] = 0.8f;
				_frameAlphas[i++] = 0.9f;
				_frameAlphas[i++] = 0.95f;
				assert(kCurveLength == i);
			}
			break;
		
		case kMy_AnimationEffectFadeOut:
			{
				size_t		i = 0;
				
				
				_frameAlphas[i++] = 0.8f;
				_frameAlphas[i++] = 0.55f;
				_frameAlphas[i++] = 0.45f;
				_frameAlphas[i++] = 0.35f;
				_frameAlphas[i++] = 0.25f;
				_frameAlphas[i++] = 0.2f;
				_frameAlphas[i++] = 0.15f;
				_frameAlphas[i++] = 0.1f;
				assert(kCurveLength == i);
			}
			break;
		
		default:
			// do nothing
			break;
		}
		
		// calculate the size of each unit of the animation
		_frameUnitH = (_targetFrame.origin.x - _originalFrame.origin.x) / kCurveLength;
		_frameUnitV = (_targetFrame.origin.y - _originalFrame.origin.y) / kCurveLength;
		_frameDeltaSizeH = (_targetFrame.size.width - _originalFrame.size.width) / kCurveLength;
		_frameDeltaSizeV = (_targetFrame.size.height - _originalFrame.size.height) / kCurveLength;
		
		// finally, scale the offsets based on these units
		for (size_t i = 0; i < kCurveLength; ++i)
		{
			_frameOffsetsH[i] *= _frameUnitH;
			_frameOffsetsV[i] *= _frameUnitV;
		}
		
		// begin animation
		[self nextAnimationStep];
		
		// make absolutely sure that the image window is hidden
		// after a short period (otherwise it would leave a
		// floating image on the screen that the user cannot
		// remove)
		__weak decltype(self)	weakSelf = self;
		NSWindow* __strong		borderlessWindow = self.borderlessWindow;
		CocoaExtensions_RunLater(2.0/* seconds; arbitrary */,
									^{
										weakSelf.animationDone = YES;
										[borderlessWindow orderOut:nil];
									});
	}
	return self;
}// initWithTransition:imageWindow:finalWindow:fromFrame:toFrame:effect:simplified:


/*!
Automatically decides when to reduce the complexity of the
animation (on older machines).

(1.8)
*/
- (instancetype)
initWithTransition:(My_AnimationTransition)			aTransition
imageWindow:(CocoaAnimation_ImageWindow*)			aBorderlessWindow
finalWindow:(NSWindow*)								theActualWindow
fromFrame:(NSRect)									sourceRect
toFrame:(NSRect)									targetRect
effect:(My_AnimationEffect)							anEffect
{
	BOOL	isSimplified = ((NSWidth(theActualWindow.frame) * NSHeight(theActualWindow.frame)) > 80000/* arbitrary */);
	
	
	return [self initWithTransition:aTransition imageWindow:aBorderlessWindow finalWindow:theActualWindow
									fromFrame:sourceRect toFrame:targetRect
									effect:anEffect simplified:isSimplified];
}// initWithTransition:imageWindow:finalWindow:fromFrame:toFrame:effect:


/*!
Destructor.

(1.8)
*/
- (void)
dealloc
{
	--gAnimationsInProgress;
	if (gAnimationsInProgress < 0)
	{
		Console_Warning(Console_WriteLine, "animation count incorrectly fell below zero");
		gAnimationsInProgress = 0;
	}
	
	// if the animation is released too soon, force the correct window location
	delete [] _frameOffsetsH;
	delete [] _frameOffsetsV;
	delete [] _frameAlphas;
}// dealloc


#pragma mark Initializers Disabled From Superclass


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2016.09)
*/
- (instancetype)
init
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithTransition:kMy_AnimationTransitionSlide imageWindow:nil
									finalWindow:nil fromFrame:NSZeroRect toFrame:NSZeroRect
									effect:kMy_AnimationEffectNone simplified:NO];
}// init


#pragma mark New Methods


/*!
Nudges the window to a new frame according to the current
step in the animation and updates the step number.  If the
last frame has not been reached, a timer is set up to call
this again after a short delay.

(1.8)
*/
- (void)
nextAnimationStep
{
	NSRect	newFrame = self.originalFrame;
	
	
	// the offsets assume a unit square and scale according to the actual
	// difference between the original frame and the target frame; an
	// offset of 1 when there are 5 frames means “20% of the distance from
	// the beginning”, 2 means 40%, etc. so offsets should generally be
	// well represented across the spectrum from 0 to the frame count
	newFrame.origin.x += self.frameOffsetsH[self.currentStep];
	newFrame.origin.y += self.frameOffsetsV[self.currentStep];
	if (0 != self.frameDeltaSizeH)
	{
		float	powerOfTwo = 0.0;
		
		
		newFrame.size.width += (self.currentStep * self.frameDeltaSizeH);
		
		// if the new width is arbitrarily close to a power of 2, it
		// is converted into an exact power of 2 so that optimizations
		// are possible based on this (but not necessarily guaranteed)
		powerOfTwo = (1 << STATIC_CAST(ceilf(log2f(newFrame.size.width)), unsigned int));
		if (fabsf(STATIC_CAST(newFrame.size.width - powerOfTwo, float)) < 5/* pixels; arbitrary */)
		{
			newFrame.size.width = powerOfTwo;
		}
	}
	if (0 != self.frameDeltaSizeV)
	{
		float	powerOfTwo = 0.0;
		
		
		newFrame.size.height += (self.currentStep * self.frameDeltaSizeV);
		
		// if the new height is arbitrarily close to a power of 2, it
		// is converted into an exact power of 2 so that optimizations
		// are possible based on this (but not necessarily guaranteed)
		powerOfTwo = (1 << STATIC_CAST(ceilf(log2f(newFrame.size.height)), unsigned int));
		if (fabsf(STATIC_CAST(newFrame.size.height - powerOfTwo, float)) < 5/* pixels; arbitrary */)
		{
			newFrame.size.height = powerOfTwo;
		}
	}
	self.borderlessWindow.animationResizeTime = 0.04;
	[self.borderlessWindow setFrame:newFrame display:YES animate:YES/* see "animationResizeTime:" */];
	
	// adjust alpha channel, if necessary
	if (nullptr != self.frameAlphas)
	{
		[self.borderlessWindow setAlphaValue:self.frameAlphas[self.currentStep]];
	}
	
	// step to next frame
	self.currentStep += (kCurveLength / self.frameCount);
	if ((NO == self.animationDone) && (self.currentStep < kCurveLength))
	{
		// this is intentionally a strong reference to prevent the animation
		// from being incomplete; it is retained (at least) by the delayed block
		decltype(self)	blockSelf = self;
		
		
		CocoaExtensions_RunLater(self.borderlessWindow.animationResizeTime,
									^{ [blockSelf nextAnimationStep]; });
	}
	else
	{
		self.animationDone = YES;
		
		// force final frame to be exact
		[self.borderlessWindow setFrame:self.targetFrame display:YES];
		
		// if a real window is being displayed, show it at this point
		if (nil != self.actualWindow)
		{
			[self.actualWindow setFrame:self.targetFrame display:YES];
			[self.actualWindow orderFront:nil];
		}
		
		// hide the fake window
		[self.borderlessWindow orderOut:nil];
	}
}// nextAnimationStep


@end // CocoaAnimation_WindowFrameAnimator //}

// BELOW IS REQUIRED NEWLINE TO END FILE
