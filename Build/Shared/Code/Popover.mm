/*!	\file Popover.mm
	\brief Implements a popover-style window.
*/
/*###############################################################

	Popover Window 1.1 (based on MAAttachedWindow)
	MAAttachedWindow © 2007 by Magic Aubergine
	Popover Window © 2011-2015 by Kevin Grant
	
	Based on the MAAttachedWindow code created by Matt Gemmell
	on September 27, 2007 and distributed under the terms in
	the file "Licenses/MattGemmell.txt" located in the MacTerm
	source distribution.
	
	Popover Window adds Mac OS X 10.3 compatibility, makes the
	window more Cocoa-compatible (animated resizing now works,
	for example) and adds a better auto-positioning algorithm,
	among other enhancements.  Code has also been reworked to
	match MacTerm standards.

###############################################################*/

#import "Popover.objc++.h"
#import <UniversalDefines.h>

// standard-C includes
#import <math.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>



#pragma mark Internal Method Prototypes

/*!
The private class interface.
*/
@interface Popover_Window (Popover_WindowInternal) //{

// class methods: general
	+ (float)
	arrowInsetWithCornerRadius:(float)_
	baseWidth:(float)_;
	+ (Popover_Properties)
	arrowPlacement:(Popover_Properties)_;
	+ (Popover_Properties)
	bestSideForViewOfSize:(NSSize)_
	andMargin:(float)_
	arrowHeight:(float)_
	arrowInset:(float)_
	at:(NSPoint)_
	onParentWindow:(NSWindow*)_
	preferredSide:(Popover_Properties)_;
	+ (NSPoint)
	idealFrameOriginForSize:(NSSize)_
	arrowInset:(float)_
	at:(NSPoint)_
	side:(Popover_Properties)_;
	+ (Popover_Properties)
	windowPlacement:(Popover_Properties)_;

// class methods: frame conversion
	+ (NSRect)
	frameRectForViewRect:(NSRect)_
	viewMargin:(float)_
	arrowHeight:(float)_
	side:(Popover_Properties)_;
	+ (NSRect)
	viewRectForFrameRect:(NSRect)_
	viewMargin:(float)_
	arrowHeight:(float)_
	side:(Popover_Properties)_;

// new methods
	- (void)
	appendArrowToPath:(NSBezierPath*)_;
	- (void)
	fixViewFrame;
	- (NSPoint)
	idealFrameOriginForPoint:(NSPoint)_;
	- (void)
	redisplay;
	- (void)
	updateBackground;

// accessors
	- (float)
	arrowInset;
	- (Popover_Properties)
	arrowPlacement;
	- (NSColor*)
	backgroundColorPatternImage;
	- (NSBezierPath*)
	backgroundPath;
	- (Popover_Properties)
	windowPlacement;

@end //}


#pragma mark Internal Methods

#pragma mark -
@implementation Popover_Window


/*!
Designated initializer.

A popover embeds a single view (which can of course contain any number
of other views).  Although you can later set the window frame in any
way you wish, at construction time you only provide a point that
determines where the tip of the window frame’s arrow is (the distance
indicates how far the arrow is from that point).

If a parent window is provided then "point" is in the coordinates of
that window; otherwise the "point" is in screen coordinates.

The initial window size is based on the view size and some default
values for margin, etc. and the location is also chosen automatically.
You may change any property of the popover later however, and ideally
before the window is shown to the user.

(1.1)
*/
- (instancetype)
initWithView:(NSView*)		aView
attachedToPoint:(NSPoint)	aPoint
inWindow:(NSWindow*)		aWindow
{
	if (nil == aView)
	{
		self = nil;
	}
	else
	{
		BOOL const		kDefaultCornerIsRounded = YES;
		float const		kDefaultArrowBaseWidth = 20.0;
		float const		kDefaultArrowHeight = 16.0;
		float const		kDefaultCornerRadius = 8.0;
		float const		kDefaultCornerRadiusForArrowCorner = (kDefaultCornerIsRounded)
																? kDefaultCornerRadius
																: 0;
		float const		kDefaultMargin = 2.0;
		float const		kDefaultArrowInset = [self.class arrowInsetWithCornerRadius:kDefaultCornerRadiusForArrowCorner
																					baseWidth:kDefaultArrowBaseWidth];
		
		
		// Create initial contentRect for the attached window (can reset frame at any time).
		// Note that the content rectangle given to NSWindow is just the frame rectangle;
		// the illusion of a window margin is created during rendering.
		Popover_Properties	side = [self.class bestSideForViewOfSize:[aView frame].size
																		andMargin:kDefaultMargin
																		arrowHeight:kDefaultArrowHeight
																		arrowInset:kDefaultArrowInset
																		at:aPoint
																		onParentWindow:aWindow
																		preferredSide:kPopover_PositionBottom];
		NSRect		frameRectViewRelative = [self.class frameRectForViewRect:[aView frame]
																				viewMargin:kDefaultMargin
																				arrowHeight:kDefaultArrowHeight
																				side:side];
		NSRect		contentRectOnScreen = NSZeroRect;
		
		
		contentRectOnScreen.size = frameRectViewRelative.size;
		contentRectOnScreen.origin = [self.class idealFrameOriginForSize:contentRectOnScreen.size
																			arrowInset:kDefaultArrowInset
																			at:((aWindow)
																				? [aWindow convertBaseToScreen:aPoint]
																				: aPoint)
																			side:side];
		
		self = [super initWithContentRect:contentRectOnScreen styleMask:NSBorderlessWindowMask
											backing:NSBackingStoreBuffered defer:NO];
		if (nil != self)
		{
			self->popoverParentWindow = aWindow;
			self->embeddedView = aView;
			self->windowPropertyFlags = side;
			self->isAutoPositioning = YES;
			
			// configure window
			[super setBackgroundColor:[NSColor clearColor]];
			[self setMovableByWindowBackground:NO];
			[self setExcludedFromWindowsMenu:YES];
			[self setAlphaValue:1.0];
			[self setOpaque:NO];
			[self setHasShadow:YES];
			[self useOptimizedDrawing:YES];
			
			// set up some sensible defaults for display
			self->popoverBackgroundColor = [[NSColor colorWithCalibratedWhite:0.1 alpha:0.75] copy];
			self->borderOuterColor = [[NSColor grayColor] copy];
			self->borderPrimaryColor = [[NSColor whiteColor] copy];
			self->borderWidth = 2.0;
			self->viewMargin = kDefaultMargin;
			self->arrowBaseWidth = kDefaultArrowBaseWidth;
			self->arrowHeight = kDefaultArrowHeight;
			self->hasArrow = YES;
			self->cornerRadius = kDefaultCornerRadius;
			self->hasRoundCornerBesideArrow = kDefaultCornerIsRounded;
			self->resizeInProgress = NO;
			
			// add view as subview
			[[self contentView] addSubview:self->embeddedView];
			
			// update the view
			[self fixViewFrame];
			[self updateBackground];
			
			// subscribe to notifications
			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResize:)
																name:NSWindowDidResizeNotification object:self];
		}
	}
	return self;
}


/*!
Destructor.

(1.0)
*/
- (void)
dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[borderOuterColor release];
	[borderPrimaryColor release];
	[popoverBackgroundColor release];
	
	[super dealloc];
}


#pragma mark General


/*!
Returns the region that the window should occupy globally if the
embedded view has the given local frame.

(1.1)
*/
- (NSRect)
frameRectForViewRect:(NSRect)	aRect
{
	return [self.class frameRectForViewRect:aRect viewMargin:[self viewMargin] arrowHeight:[self arrowHeight]
											side:self->windowPropertyFlags];
}


/*!
Returns the region that the embedded view should occupy if the
window has the given global frame.

(1.1)
*/
- (NSRect)
viewRectForFrameRect:(NSRect)	aRect
{
	return [self.class viewRectForFrameRect:aRect viewMargin:[self viewMargin] arrowHeight:[self arrowHeight]
											side:self->windowPropertyFlags];
}


/*!
Arranges the window so its arrow is pointing at the specified point and
is on the given side, even if that placement obscures the window (see
also "setPointWithAutomaticPositioning:preferredSide:").

The "point" is relative to the parent window, if any was specified at
construction time; otherwise it is in screen coordinates.

(1.0)
*/
- (void)
setPoint:(NSPoint)				aPoint
onSide:(Popover_Properties)		aSide
{
	// set these properties first so that "frameRectForViewRect:" sees them
	self->isAutoPositioning = NO;
	self->windowPropertyFlags = aSide;
	
	// update the window frame
	{
		NSRect		windowFrame = [self frameRectForViewRect:[self->embeddedView frame]]; // ignore origin, use only size
		NSPoint		idealOrigin = [self.class idealFrameOriginForSize:windowFrame.size
																		arrowInset:[self arrowInset]
																		at:((self->popoverParentWindow)
																			? [self->popoverParentWindow convertBaseToScreen:aPoint]
																			: aPoint)
																		side:aSide];
		
		
		windowFrame.origin.x = idealOrigin.x;
		windowFrame.origin.y = idealOrigin.y;
		NSDisableScreenUpdates();
		[self setFrame:windowFrame display:NO];
		[self fixViewFrame];
		[self updateBackground];
		NSEnableScreenUpdates();
	}
}


/*!
Arranges the window so its arrow is pointing at the specified point, in
whatever position ensures that the window stays on screen.  If there are
multiple “equally good” candidates for the window position and one of
them is the preferred side then the preferred side is used.

The given point is relative to the parent window, if any was specified
at construction time; otherwise it is in screen coordinates.

The automatic placement algorithm prefers a position horizontally
centered below the specified point, aligned with the parent window and
within the parent window’s frame.

(1.1)
*/
- (void)
setPointWithAutomaticPositioning:(NSPoint)	aPoint
preferredSide:(Popover_Properties)			aSide
{
	Popover_Properties	chosenSide = [self.class bestSideForViewOfSize:self->viewFrame.size
																		andMargin:[self viewMargin]
																		arrowHeight:[self arrowHeight]
																		arrowInset:[self arrowInset]
																		at:aPoint
																		onParentWindow:self->popoverParentWindow
																		preferredSide:aSide];
	
	
	[self setPoint:aPoint onSide:chosenSide];
	self->isAutoPositioning = YES;
}


#pragma mark Accessors


/*!
Accessor.

The base width determines how “fat” the frame arrow’s triangle is.

(1.0)
*/
- (float)
arrowBaseWidth
{
	return arrowBaseWidth;
}
- (void)
setArrowBaseWidth:(float)	aValue
{
	float	maxWidth = (MIN(self->viewFrame.size.width, self->viewFrame.size.height) +
						(self->viewMargin * 2.0)) - self->cornerRadius;
	
	
	if (self->hasRoundCornerBesideArrow)
	{
		maxWidth -= self->cornerRadius;
	}
	
	if (aValue <= maxWidth)
	{
		arrowBaseWidth = aValue;
	}
	else
	{
		arrowBaseWidth = maxWidth;
	}
	
	[self redisplay];
}


/*!
Accessor.

The height determines how “slender” the frame arrow’s triangle is.

(1.0)
*/
- (float)
arrowHeight
{
	return arrowHeight;
}
- (void)
setArrowHeight:(float)	aValue
{
	if (arrowHeight != aValue)
	{
		arrowHeight = aValue;
		
		[self redisplay];
	}
}


/*!
Accessor.

The outer border color is used to render the boundary of
the popover window.  See also "setBorderPrimaryColor:".

The border thickness is determined by "setBorderWidth:".
If the outer and primary colors are the same then the
border appears to be that thickness; otherwise the width
is divided roughly evenly between the two colors.

(1.1)
*/
- (NSColor*)
borderOuterColor
{
	return [[borderOuterColor retain] autorelease];
}
- (void)
setBorderOuterColor:(NSColor*)	aValue
{
	if (borderOuterColor != aValue)
	{
		[borderOuterColor release];
		borderOuterColor = [aValue copy];
		
		[self updateBackground];
	}
}


/*!
Accessor.

The primary border color is used to render a frame inside
the outer border (see "setBorderOuterColor:").

The border thickness is determined by "setBorderWidth:".
If the outer and primary colors are the same then the
border appears to be that thickness; otherwise the width
is divided roughly evenly between the two colors.

(1.1)
*/
- (NSColor*)
borderPrimaryColor
{
	return [[borderPrimaryColor retain] autorelease];
}
- (void)
setBorderPrimaryColor:(NSColor*)	aValue
{
	if (borderPrimaryColor != aValue)
	{
		[borderPrimaryColor release];
		borderPrimaryColor = [aValue copy];
		
		[self updateBackground];
	}
}


/*!
Accessor.

The border is drawn inside the viewMargin area, expanding inwards; it
does not increase the width/height of the window.  You can use
"setBorderWidth:" and "setViewMargin:" together to achieve the exact
look/geometry you want.

(1.0)
*/
- (float)
borderWidth
{
	return borderWidth;
}
- (void)
setBorderWidth:(float)	aValue
{
	if (borderWidth != aValue)
	{
		float	maxBorderWidth = viewMargin;
		
		
		if (aValue <= maxBorderWidth)
		{
			borderWidth = aValue;
		}
		else
		{
			borderWidth = maxBorderWidth;
		}
		
		[self updateBackground];
	}
}


/*!
Accessor.

The radius in pixels of the arc used to draw curves at the
corners of the popover frame.

(1.0)
*/
- (float)
cornerRadius
{
	return cornerRadius;
}
- (void)
setCornerRadius:(float)		aValue
{
	float	maxRadius = ((MIN(self->viewFrame.size.width, self->viewFrame.size.height) + (self->viewMargin * 2.0)) -
							self->arrowBaseWidth) / 2.0;
	
	
	if (aValue <= maxRadius)
	{
		cornerRadius = aValue;
	}
	else
	{
		cornerRadius = maxRadius;
	}
	cornerRadius = MAX(cornerRadius, 0.0);
	
	// synchronize arrow size
	[self setArrowBaseWidth:self->arrowBaseWidth];
}


/*!
Accessor.

Specifies whether or not the frame has an arrow displayed.
Even if there is no arrow, space is allocated for the arrow
so that it can appear or disappear without the user seeing
the window resize.

(1.0)
*/
- (BOOL)
hasArrow
{
	return hasArrow;
}
- (void)
setHasArrow:(BOOL)	aValue
{
	if (hasArrow != aValue)
	{
		hasArrow = aValue;
		
		[self updateBackground];
	}
}


/*!
Accessor.

When the window position puts the arrow near a corner of
the frame, this specifies how close to the corner the
arrow is.  If a rounded corner appears then the arrow is
off to the side; otherwise the arrow is right in the
corner.

(1.0)
*/
- (BOOL)
hasRoundCornerBesideArrow
{
	return hasRoundCornerBesideArrow;
}
- (void)
setHasRoundCornerBesideArrow:(BOOL)		aValue
{
	if (hasRoundCornerBesideArrow != aValue)
	{
		hasRoundCornerBesideArrow = aValue;
		
		[self redisplay];
	}
}


/*!
Accessor.

The popover background color is used to construct an image
that the NSWindow superclass uses for rendering.

Use this property instead of the NSWindow "backgroundColor"
property because the normal background color is overridden
to contain the entire rendering of the popover window frame
(as a pattern image).

(1.0)
*/
- (NSColor*)
popoverBackgroundColor
{
	return [[self->popoverBackgroundColor retain] autorelease];
}
- (void)
setPopoverBackgroundColor:(NSColor*)	aValue
{
	if (self->popoverBackgroundColor != aValue)
	{
		[self->popoverBackgroundColor release];
		self->popoverBackgroundColor = [aValue copy];
		
		[self updateBackground];
	}
}


/*!
Accessor.

The distance between the edge of the view and the window edge.

(1.0)
*/
- (float)
viewMargin
{
	return viewMargin;
}
- (void)
setViewMargin:(float)	aValue
{
	if (viewMargin != aValue)
	{
		viewMargin = MAX(aValue, 0.0);
		
		// synchronize corner radius (and arrow base width)
		[self setCornerRadius:self->cornerRadius];
	}
}


#pragma mark NSObject (NSMenuValidation)


/*!
Forwards menu validation requests to the parent window, making
the popover act like a secondary window.

(1.0)
*/
- (BOOL)
validateMenuItem:(NSMenuItem*)	item
{
	if (self->popoverParentWindow)
	{
		return [self->popoverParentWindow validateMenuItem:item];
	}
	return [super validateMenuItem:item];
}


#pragma mark NSWindow


/*!
Overrides the NSWindow implementation.

(1.0)
*/
- (BOOL)
canBecomeMainWindow
{
	return NO;
}


/*!
Overrides the NSWindow implementation.

(1.0)
*/
- (BOOL)
canBecomeKeyWindow
{
	return YES;
}


/*!
Overrides the NSWindow implementation.

(1.0)
*/
- (BOOL)
isExcludedFromWindowsMenu
{
	return YES;
}


/*!
Overrides the NSWindow implementation to allow any associated
parent window to handle “Close” commands, making the popover act
like a secondary window.

(1.0)
*/
- (IBAction)
performClose:(id)	sender
{
	if (self->popoverParentWindow)
	{
		[self->popoverParentWindow performClose:sender];
	}
	else
	{
		[super performClose:sender];
	}
}


/*!
Overrides the NSWindow implementation of the background color to
cause an equivalent (semi-transparent) image to be the background.

(1.0)
*/
- (void)
setBackgroundColor:(NSColor*)	aValue
{
	[self setPopoverBackgroundColor:aValue];
}


#pragma mark NSWindowNotifications


/*!
Responds to window resize notifications by redrawing the background.

(1.0)
*/
- (void)
windowDidResize:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self redisplay];
}


@end // Popover_Window


#pragma mark -
@implementation Popover_Window (Popover_WindowInternal)


/*!
If the window frame is currently set to include a triangular
arrow head then this adds an arrow outline of the proper
orientation and thickness to the given path.

(1.0)
*/
- (void)
appendArrowToPath:(NSBezierPath*)	aPath
{
	if (hasArrow)
	{
		float const		kScaleFactor = 1.0;
		float const		kScaledArrowWidth = self->arrowBaseWidth * kScaleFactor;
		float const		kHalfScaledArrowWidth = kScaledArrowWidth / 2.0;
		float const		kScaledArrowHeight = self->arrowHeight * kScaleFactor;
		NSPoint			tipPt = [aPath currentPoint];
		NSPoint			endPt = [aPath currentPoint];
		
		
		// all paths go clockwise
		switch ([self windowPlacement])
		{
		case kPopover_PropertyPlaceFrameLeftOfArrow:
			// arrow points to the right starting from the top
			tipPt.x += kScaledArrowHeight;
			tipPt.y -= kHalfScaledArrowWidth;
			endPt.y -= kScaledArrowWidth;
			break;
		
		case kPopover_PropertyPlaceFrameRightOfArrow:
			// arrow points to the left starting from the bottom
			tipPt.x -= kScaledArrowHeight;
			tipPt.y += kHalfScaledArrowWidth;
			endPt.y += kScaledArrowWidth;
			break;
		
		case kPopover_PropertyPlaceFrameAboveArrow:
			// arrow points downward starting from the right
			tipPt.y -= kScaledArrowHeight;
			tipPt.x -= kHalfScaledArrowWidth;
			endPt.x -= kScaledArrowWidth;
			break;
		
		case kPopover_PropertyPlaceFrameBelowArrow:
		default:
			// arrow points upward starting from the left
			tipPt.y += kScaledArrowHeight;
			tipPt.x += kHalfScaledArrowWidth;
			endPt.x += kScaledArrowWidth;
			break;
		}
		
		[aPath lineToPoint:tipPt];
		[aPath lineToPoint:endPt];
	}
}


/*!
Returns the offset from the nearest corner that the arrow’s
tip should have, if applicable.

(1.0)
*/
- (float)
arrowInset
{
	float const		kRadius = (self->hasRoundCornerBesideArrow)
								? self->cornerRadius
								: 0;
	
	
	return [self.class arrowInsetWithCornerRadius:kRadius baseWidth:self->arrowBaseWidth];
}


/*!
A helper to determine the offset from the nearest corner that
an arrow’s tip should have, given the specified corner arc size
and width of the arrow’s triangular base.

(1.0)
*/
+ (float)
arrowInsetWithCornerRadius:(float)	aCornerRadius
baseWidth:(float)					anArrowBaseWidth
{
	return (aCornerRadius + (anArrowBaseWidth / 2.0));
}


/*!
Returns the arrow placement portion of the specified properties.

(1.1)
*/
+ (Popover_Properties)
arrowPlacement:(Popover_Properties)		aFlagSet
{
	BOOL	result = (aFlagSet & kPopover_PropertyMaskArrow);
	
	
	return result;
}


/*!
Returns the arrow placement portion of the current properties.

(1.1)
*/
- (Popover_Properties)
arrowPlacement
{
	return [self.class arrowPlacement:self->windowPropertyFlags];
}


/*!
Returns the entire popover window’s frame and background image
as an NSColor so that it can be rendered naturally by NSWindow.

(1.0)
*/
- (NSColor*)
backgroundColorPatternImage
{
	NSColor*	result = nil;
	NSImage*	patternImage = [[NSImage alloc] initWithSize:[self frame].size];
	
	
	[patternImage lockFocus];
	[NSGraphicsContext saveGraphicsState];
	{
		BOOL const		kDebug = NO;
		NSBezierPath*	sourcePath = [self backgroundPath];
		NSRect			localRect = [self frame];
		
		
		localRect.origin.x = 0;
		localRect.origin.y = 0;
		
		if (kDebug)
		{
			NSRectClip(localRect);
		}
		else
		{
			[sourcePath addClip];
		}
		
		[self->popoverBackgroundColor set];
		[sourcePath fill];
		if ([self borderWidth] > 0)
		{
			// double width since drawing is clipped inside the path
			[sourcePath setLineWidth:([self borderWidth] * 2.0)];
			[self->borderPrimaryColor set];
			[sourcePath stroke];
			[sourcePath setLineWidth:[self borderWidth] - 1.0]; // arbitrary shift
			[self->borderOuterColor set];
			[sourcePath stroke];
		}
		
		if (kDebug)
		{
			// for debugging: show the entire frame’s actual rectangle
			[[NSColor redColor] set];
			NSFrameRectWithWidth(localRect, 3.0/* arbitrary */);
		}
	}
	[NSGraphicsContext restoreGraphicsState];
	[patternImage unlockFocus];
	
	result = [NSColor colorWithPatternImage:patternImage];
	
	[patternImage autorelease];
	
	return result;
}


/*!
Returns a path describing the window frame, taking into account
the triangular arrow head’s visibility and attributes.

(1.0)
*/
- (NSBezierPath*)
backgroundPath
{
	float const		kScaleFactor = 1.0;
	float const		kScaledArrowWidth = self->arrowBaseWidth * kScaleFactor;
	float const		kHalfScaledArrowWidth = kScaledArrowWidth / 2.0;
	float const		kScaledCornerRadius = (self->cornerRadius * kScaleFactor);
	float const		kScaledMargin = (self->viewMargin * kScaleFactor);
	NSRect const	kContentArea = NSInsetRect(self->viewFrame, -kScaledMargin, -kScaledMargin);
	float const		kMidX = NSMidX(kContentArea) * kScaleFactor;
	float const		kMidY = NSMidY(kContentArea) * kScaleFactor;
	float const		kMinX = ceilf(NSMinX(kContentArea) * kScaleFactor + 0.5f);
	float const		kMaxX = floorf(NSMaxX(kContentArea) * kScaleFactor - 0.5f);
	float const		kMinY = ceilf(NSMinY(kContentArea) * kScaleFactor + 0.5f);
	float const		kMaxY = floorf(NSMaxY(kContentArea) * kScaleFactor - 0.5f);
	NSPoint			endOfLine = NSZeroPoint;
	BOOL			isRoundedCorner = (kScaledCornerRadius > 0);
	BOOL			shouldDrawNextCorner = NO;
	NSBezierPath*	result = [NSBezierPath bezierPath];
	
	
	[result setLineJoinStyle:NSRoundLineJoinStyle];
	
	//
	// top side
	//
	
	// set starting point (top-left)
	{
		NSPoint		startingPoint = NSMakePoint(kMinX, kMaxY);
		
		
		// offset by any rounded corner that’s in effect
		if ((isRoundedCorner) &&
			((hasRoundCornerBesideArrow) ||
				((kPopover_PositionBottomRight != self->windowPropertyFlags) &&
					(kPopover_PositionRightBottom != self->windowPropertyFlags))))
		{
			startingPoint.x += kScaledCornerRadius;
		}
		
		endOfLine = NSMakePoint(kMaxX, kMaxY);
		if ((isRoundedCorner) &&
			((hasRoundCornerBesideArrow) ||
				((kPopover_PositionBottomLeft != self->windowPropertyFlags) &&
					(kPopover_PositionLeftBottom != self->windowPropertyFlags))))
		{
			endOfLine.x -= kScaledCornerRadius;
			shouldDrawNextCorner = YES;
		}
		
		[result moveToPoint:startingPoint];
	}
	
	// draw arrow
	if (kPopover_PositionBottomRight == self->windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionBottom == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMidX - kHalfScaledArrowWidth, kMaxY)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionBottomLeft == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(endOfLine.x - kScaledArrowWidth, kMaxY)];
		[self appendArrowToPath:result];
	}
	
	// draw frame line
	[result lineToPoint:endOfLine];
	
	// draw rounded corner
	if (shouldDrawNextCorner)
	{
		[result appendBezierPathWithArcFromPoint:NSMakePoint(kMaxX, kMaxY)
													toPoint:NSMakePoint(kMaxX, kMaxY - kScaledCornerRadius)
													radius:kScaledCornerRadius];
	}
	
	//
	// right side
	//
	
	// set starting point (top-right)
	endOfLine = NSMakePoint(kMaxX, kMinY);
	shouldDrawNextCorner = NO;
	if ((isRoundedCorner) &&
		((hasRoundCornerBesideArrow) ||
			((kPopover_PositionTopLeft != self->windowPropertyFlags) &&
				(kPopover_PositionLeftTop != self->windowPropertyFlags))))
	{
		endOfLine.y += kScaledCornerRadius;
		shouldDrawNextCorner = YES;
	}
	
	// draw arrow
	if (kPopover_PositionLeftBottom == self->windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionLeft == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMaxX, kMidY + kHalfScaledArrowWidth)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionLeftTop == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMaxX, endOfLine.y + kScaledArrowWidth)];
		[self appendArrowToPath:result];
	}
	
	// draw frame line
	[result lineToPoint:endOfLine];
	
	// draw rounded corner
	if (shouldDrawNextCorner)
	{
		[result appendBezierPathWithArcFromPoint:NSMakePoint(kMaxX, kMinY)
													toPoint:NSMakePoint(kMaxX - kScaledCornerRadius, kMinY)
													radius:kScaledCornerRadius];
	}
	
	//
	// bottom side
	//
	
	// set starting point (bottom-right)
	endOfLine = NSMakePoint(kMinX, kMinY);
	shouldDrawNextCorner = NO;
	if ((isRoundedCorner) &&
		((hasRoundCornerBesideArrow) ||
			((kPopover_PositionTopRight != self->windowPropertyFlags) &&
				(kPopover_PositionRightTop != self->windowPropertyFlags))))
	{
		endOfLine.x += kScaledCornerRadius;
		shouldDrawNextCorner = YES;
	}
	
	// draw arrow
	if (kPopover_PositionTopLeft == self->windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionTop == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMidX + kHalfScaledArrowWidth, kMinY)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionTopRight == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(endOfLine.x + kScaledArrowWidth, kMinY)];
		[self appendArrowToPath:result];
	}
	
	// draw frame line
	[result lineToPoint:endOfLine];
	
	// draw rounded corner
	if (shouldDrawNextCorner)
	{
		[result appendBezierPathWithArcFromPoint:NSMakePoint(kMinX, kMinY)
													toPoint:NSMakePoint(kMinX, kMinY + kScaledCornerRadius)
													radius:kScaledCornerRadius];
	}
	
	//
	// left side
	//
	
	// set starting point (bottom-left)
	endOfLine = NSMakePoint(kMinX, kMaxY);
	shouldDrawNextCorner = NO;
	if ((isRoundedCorner) &&
		((hasRoundCornerBesideArrow) ||
			((kPopover_PositionRightBottom != self->windowPropertyFlags) &&
				(kPopover_PositionBottomRight != self->windowPropertyFlags))))
	{
		endOfLine.y -= kScaledCornerRadius;
		shouldDrawNextCorner = YES;
	}
	
	// draw arrow
	if (kPopover_PositionRightTop == self->windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionRight == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMinX, kMidY - kHalfScaledArrowWidth)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionRightBottom == self->windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMinX, endOfLine.y - kScaledArrowWidth)];
		[self appendArrowToPath:result];
	}
	
	// draw frame line
	[result lineToPoint:endOfLine];
	
	// draw rounded corner
	if (shouldDrawNextCorner)
	{
		[result appendBezierPathWithArcFromPoint:NSMakePoint(kMinX, kMaxY)
													toPoint:NSMakePoint(kMinX + kScaledCornerRadius, kMaxY)
													radius:kScaledCornerRadius];
	}
	
	[result closePath];
	
	return result;
}


/*!
Returns window properties to frame the popover window in a way that is
considered optimal for the given constraints.  A preferred side must
be specified to help break ties.

Note that this is a class method so that dependencies on measurements
are more explicit.  This will encourage the development of a future
algorithm that relies on fewer inputs!

(1.1)
*/
+ (Popover_Properties)
bestSideForViewOfSize:(NSSize)		aViewSize
andMargin:(float)					aViewMargin
arrowHeight:(float)					anArrowHeight
arrowInset:(float)					anArrowInset
at:(NSPoint)						aPoint
onParentWindow:(NSWindow*)			aWindow
preferredSide:(Popover_Properties)	aSide
{
	NSRect const	kScreenFrame = ((nil != aWindow) && (nil != [aWindow screen]))
									? [[aWindow screen] visibleFrame]
									: [[NSScreen mainScreen] visibleFrame];
	NSPoint const	kPointOnScreen = (nil != aWindow)
										? [aWindow convertBaseToScreen:aPoint]
										: aPoint;
	Popover_Properties		result = kPopover_PositionBottom;
	
	
	// pretend the given view size actually included the margins
	// (the margins are not otherwise needed)
	aViewSize.width += aViewMargin * 2.0;
	aViewSize.height += aViewMargin * 2.0;
	
	Popover_Properties const	kCandidatePositions[] =
								{
									kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow,
									kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameLeftOfArrow,
									kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameRightOfArrow,
									kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameAboveArrow,
									kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameBelowArrow,
									kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameLeftOfArrow,
									kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameRightOfArrow,
									kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameAboveArrow,
									kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameBelowArrow,
									kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameLeftOfArrow,
									kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameRightOfArrow,
									kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameAboveArrow
								};
	size_t const				kCandidateCount = sizeof(kCandidatePositions) / sizeof(Popover_Properties);
	float						greatestArea = 0.0;
	
	
	// find the window and arrow placement that leaves the greatest
	// portion of the popover view on screen
	result = 0; // initially...
	for (size_t i = 0; i < kCandidateCount; ++i)
	{
		NSRect		viewRect = NSMakeRect(0, 0, aViewSize.width, aViewSize.height);
		NSRect		frameRect = [self.class frameRectForViewRect:viewRect viewMargin:aViewMargin
																	arrowHeight:anArrowHeight side:kCandidatePositions[i]];
		NSPoint		idealOrigin = [self.class idealFrameOriginForSize:frameRect.size arrowInset:anArrowInset
																		at:kPointOnScreen side:kCandidatePositions[i]];
		
		
		// adjust to position on screen
		frameRect.origin.x = idealOrigin.x;
		frameRect.origin.y = idealOrigin.y;
		
		// see if this rectangle shows more of the view than the previous best
		{
			NSRect const	kIntersectionRegion = NSIntersectionRect(frameRect, kScreenFrame);
			float const		kIntersectionArea = (kIntersectionRegion.size.width * kIntersectionRegion.size.height);
			
			
			if (kIntersectionArea > greatestArea)
			{
				greatestArea = kIntersectionArea;
				result = kCandidatePositions[i];
			}
			else if ((kIntersectionArea > (greatestArea - 0.001)) && (kIntersectionArea < (greatestArea + 0.001))) // arbitrary
			{
				// if this is no better than the previous choice but it
				// is the preferred side, choose it anyway
				if (kCandidatePositions[i] == aSide)
				{
					result = aSide;
				}
			}
		}
	}
		
	return result;
}


/*!
Updates the location and size of the embedded content view so
that it is correct for the current configuration of the popover
(arrow position, offsets, etc.).

(1.1)
*/
- (void)
fixViewFrame
{
	self->viewFrame = [self viewRectForFrameRect:[self frame]];
	self->viewFrame.origin = [self convertScreenToBase:self->viewFrame.origin];
	[self->embeddedView setFrame:self->viewFrame];
}


/*!
Returns an NSWindow frame rectangle (relative to the window
position on screen) appropriate for the specified view size and
related parameters.

This boundary is not completely visible to the user because the
arrow and all of the space around the arrow appears in the frame
rectangle too.

This should be consistent with the calculations for the method
"viewRectForFrameRect:viewMargin:arrowHeight:side:".

(1.1)
*/
+ (NSRect)
frameRectForViewRect:(NSRect)	aRect
viewMargin:(float)				aViewMargin
arrowHeight:(float)				anArrowHeight
side:(Popover_Properties)		aSide
{
	float const		kOffsetArrowSide = (anArrowHeight + aViewMargin);
	float const		kOffsetNonArrowSide = (aViewMargin);
	NSRect	result = aRect;
	
	
	switch (aSide)
	{
	case kPopover_PositionLeft:
	case kPopover_PositionLeftTop:
	case kPopover_PositionLeftBottom:
		result.origin.x -= kOffsetNonArrowSide;
		result.origin.y -= kOffsetNonArrowSide;
		result.size.width += (kOffsetNonArrowSide + kOffsetArrowSide);
		result.size.height += (kOffsetNonArrowSide + kOffsetNonArrowSide);
		break;
	
	case kPopover_PositionRight:
	case kPopover_PositionRightTop:
	case kPopover_PositionRightBottom:
		result.origin.x -= kOffsetArrowSide;
		result.origin.y -= kOffsetNonArrowSide;
		result.size.width += (kOffsetArrowSide + kOffsetNonArrowSide);
		result.size.height += (kOffsetNonArrowSide + kOffsetNonArrowSide);
		break;
	
	case kPopover_PositionBottom:
	case kPopover_PositionBottomLeft:
	case kPopover_PositionBottomRight:
		result.origin.x -= kOffsetNonArrowSide;
		result.origin.y -= kOffsetNonArrowSide;
		result.size.width += (kOffsetNonArrowSide + kOffsetNonArrowSide);
		result.size.height += (kOffsetArrowSide + kOffsetNonArrowSide);
		break;
	
	case kPopover_PositionTop:
	case kPopover_PositionTopLeft:
	case kPopover_PositionTopRight:
	default:
		result.origin.x -= kOffsetNonArrowSide;
		result.origin.y -= kOffsetArrowSide;
		result.size.width += (kOffsetNonArrowSide + kOffsetNonArrowSide);
		result.size.height += (kOffsetNonArrowSide + kOffsetArrowSide);
		break;
	}
	return result;
}


/*!
Instead of "idealFrameOriginForSize:arrowInset:at:side:", call this
to automatically use the properties of the current instance (should
not be called from an initializer).

(1.1)
*/
- (NSPoint)
idealFrameOriginForPoint:(NSPoint)		aPoint
{
	return [self.class idealFrameOriginForSize:[self frame].size arrowInset:[self arrowInset]
												at:((self->popoverParentWindow)
													? [self->popoverParentWindow convertBaseToScreen:aPoint]
													: aPoint)
												side:self->windowPropertyFlags];
}


/*!
Returns a lower-left corner for the window frame that would be
ideal for a popover with the given characteristics.

Note however that the result is only a shifted version of the
original point; so if you want something in screen coordinates
the original point must already be in screen coordinates.

(1.1)
*/
+ (NSPoint)
idealFrameOriginForSize:(NSSize)	aFrameSize
arrowInset:(float)					anArrowInset
at:(NSPoint)						aPoint
side:(Popover_Properties)			aSide
{
	NSPoint		result = aPoint;
	
	
	switch (aSide)
	{
	case kPopover_PositionTopLeft:
		result.x -= (aFrameSize.width - anArrowInset);
		break;
	
	case kPopover_PositionTopRight:
		result.x -= anArrowInset;
		break;
	
	case kPopover_PositionBottomLeft:
		result.y -= aFrameSize.height;
		result.x -= (aFrameSize.width - anArrowInset);
		break;
	
	case kPopover_PositionBottom:
		result.y -= aFrameSize.height;
		result.x -= aFrameSize.width / 2.0;
		break;
	
	case kPopover_PositionBottomRight:
		result.x -= anArrowInset;
		result.y -= aFrameSize.height;
		break;
	
	case kPopover_PositionLeftTop:
		result.x -= aFrameSize.width;
		result.y -= anArrowInset;
		break;
	
	case kPopover_PositionLeft:
		result.x -= aFrameSize.width;
		result.y -= aFrameSize.height / 2.0;
		break;
	
	case kPopover_PositionLeftBottom:
		result.x -= aFrameSize.width;
		result.y -= (aFrameSize.height - anArrowInset);
		break;
	
	case kPopover_PositionRightTop:
		result.y -= anArrowInset;
		break;
	
	case kPopover_PositionRight:
		result.y -= aFrameSize.height / 2.0;
		break;
	
	case kPopover_PositionRightBottom:
		result.y -= (aFrameSize.height - anArrowInset);
		break;
	
	case kPopover_PositionTop:
	default:
		result.x -= aFrameSize.width / 2.0;
		break;
	}
	
    return result;
}


/*!
Forces the window to redo its layout and render itself again.

(1.0)
*/
- (void)
redisplay
{
	if (NO == self->resizeInProgress)
	{
		self->resizeInProgress = YES;
		NSDisableScreenUpdates();
		[self fixViewFrame];
		[self updateBackground];
		NSEnableScreenUpdates();
		self->resizeInProgress = NO;
	}
}


/*!
Forces the window to render itself again.

(1.0)
*/
- (void)
updateBackground
{
	NSDisableScreenUpdates();
	// call superclass to avoid overridden version from this class
	[super setBackgroundColor:[self backgroundColorPatternImage]];
	if ([self isVisible])
	{
		[self display];
		[self invalidateShadow];
	}
	NSEnableScreenUpdates();
}


/*!
Returns an NSView frame rectangle (relative to its window content
view) appropriate for the specified window-relative frame and
related parameters.

This boundary is not completely visible to the user because the
arrow and all of the space around the arrow appears in the frame
rectangle too.

This should be consistent with the calculations for the method
"frameRectForViewRect:viewMargin:arrowHeight:side:".

(1.1)
*/
+ (NSRect)
viewRectForFrameRect:(NSRect)	aRect
viewMargin:(float)				aViewMargin
arrowHeight:(float)				anArrowHeight
side:(Popover_Properties)		aSide
{
	float const		kOffsetArrowSide = (anArrowHeight + aViewMargin);
	float const		kOffsetNonArrowSide = (aViewMargin);
	NSRect	result = aRect;
	
	
	switch (aSide)
	{
	case kPopover_PositionLeft:
	case kPopover_PositionLeftTop:
	case kPopover_PositionLeftBottom:
		result.origin.x += kOffsetNonArrowSide;
		result.origin.y += kOffsetNonArrowSide;
		result.size.width -= (kOffsetNonArrowSide + kOffsetArrowSide);
		result.size.height -= (kOffsetNonArrowSide + kOffsetNonArrowSide);
		break;
	
	case kPopover_PositionRight:
	case kPopover_PositionRightTop:
	case kPopover_PositionRightBottom:
		result.origin.x += kOffsetArrowSide;
		result.origin.y += kOffsetNonArrowSide;
		result.size.width -= (kOffsetArrowSide + kOffsetNonArrowSide);
		result.size.height -= (kOffsetNonArrowSide + kOffsetNonArrowSide);
		break;
	
	case kPopover_PositionBottom:
	case kPopover_PositionBottomLeft:
	case kPopover_PositionBottomRight:
		result.origin.x += kOffsetNonArrowSide;
		result.origin.y += kOffsetNonArrowSide;
		result.size.width -= (kOffsetNonArrowSide + kOffsetNonArrowSide);
		result.size.height -= (kOffsetArrowSide + kOffsetNonArrowSide);
		break;
	
	case kPopover_PositionTop:
	case kPopover_PositionTopLeft:
	case kPopover_PositionTopRight:
	default:
		result.origin.x += kOffsetNonArrowSide;
		result.origin.y += kOffsetArrowSide;
		result.size.width -= (kOffsetNonArrowSide + kOffsetNonArrowSide);
		result.size.height -= (kOffsetNonArrowSide + kOffsetArrowSide);
		break;
	}
	return result;
}


/*!
Returns the window placement portion of the specified properties.

(1.1)
*/
+ (Popover_Properties)
windowPlacement:(Popover_Properties)	aFlagSet
{
	BOOL	result = (aFlagSet & kPopover_PropertyMaskPlaceFrame);
	
	
	return result;
}


/*!
Returns the window placement portion of the specified properties.

(1.1)
*/
- (Popover_Properties)
windowPlacement
{
	return [self.class windowPlacement:self->windowPropertyFlags];
}


@end // Popover_Window (Popover_WindowInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
