//
//  MAAttachedWindow.m
//
//  Created by Matt Gemmell on 27/09/2007.
//  Copyright 2007 Magic Aubergine.
//
//  Updated version by Kevin Grant.
//  Copyright 2011 Kevin Grant.
//  - Minor changes to add Mac OS X 10.3 support.
//  - Removed scaling factors entirely because Core Graphics now handles scaling
//    automatically and all previous APIs have been deprecated by Apple.
//  - Converted all geometry calculations to rely exclusively on the frame of
//    the popover window without storing any other geometry information.  This is
//    not only more “normal” for custom windows, but it enables nice features
//    like being able to call "setFrame:display:animate:" to resize a popover!
//    Note that it is still reasonable and typical to use "setPoint:side:" to
//    relocate a popover.
//  - Implemented routines like "viewRectForFrameRect:" to aid calculations both
//    inside and outside this module.  Also created some class methods and
//    instance methods for various calculations, and updated the initializer to
//    use these so that a correct content rectangle can be passed directly to
//    the superclass initializer for the window.  This occasionally makes the
//    code more verbose, but in initializers it is far safer to be explicit and
//    pass known values to class methods instead of using instance methods.
//  - Removed configurable macros at the beginning of the module; these either
//    turned out to be unnecessary or confined to the designated initializer so
//    local constants were created in the initializer instead.
//

#import "MAAttachedWindow.h"


@interface MAAttachedWindow (MAPrivateMethods)

// Geometry
+ (MAWindowPosition)_bestSideForAutomaticPositionOfViewWithSize:(NSSize)_ andMargin:(float)_ arrowHeight:(float)_ arrowInset:(float)_
																distance:(float)_ at:(NSPoint)_ onParentWindow:(NSWindow *)_;
+ (NSPoint)_idealFrameOriginForSize:(NSSize)_ arrowInset:(float)_ distance:(float)_ at:(NSPoint)_ side:(MAWindowPosition)_;
- (NSPoint)_idealFrameOriginForPoint:(NSPoint)_;
+ (NSRect)_viewRectForFrameRect:(NSRect)_ viewMargin:(float)_ arrowHeight:(float)_ distance:(float)_ side:(MAWindowPosition)_;
+ (NSRect)_frameRectForViewRect:(NSRect)_ viewMargin:(float)_ arrowHeight:(float)_ distance:(float)_ side:(MAWindowPosition)_;
- (void)_fixViewFrame;
+ (float)_arrowInsetWithCornerRadius:(float)_ baseWidth:(float)_;
- (float)_arrowInset;

// Drawing
- (void)_updateBackground;
- (NSColor *)_backgroundColorPatternImage;
- (NSBezierPath *)_backgroundPath;
- (void)_appendArrowToPath:(NSBezierPath *)path;
- (void)_redisplay;

@end

@implementation MAAttachedWindow


#pragma mark Initializers


- (MAAttachedWindow *)initWithView:(NSView *)view
				   attachedToPoint:(NSPoint)point
						  inWindow:(NSWindow *)parentWindow
							onSide:(MAWindowPosition)side
						atDistance:(float)distance
{
	BOOL const DEFAULT_IS_ARROW_CORNER_ROUNDED = YES;
	float const DEFAULT_ARROW_BASE_WIDTH = 20.0;
	float const DEFAULT_ARROW_HEIGHT = 16.0;
	float const DEFAULT_CORNER_RADIUS = 8.0;
	float const DEFAULT_CORNER_RADIUS_FOR_ARROW_CORNER = ((DEFAULT_IS_ARROW_CORNER_ROUNDED) ? DEFAULT_CORNER_RADIUS : 0);
	float const DEFAULT_MARGIN = 2.0;
	float const ARROW_INSET = [[self class] _arrowInsetWithCornerRadius:DEFAULT_CORNER_RADIUS_FOR_ARROW_CORNER baseWidth:DEFAULT_ARROW_BASE_WIDTH];
	
	// Insist on having a valid view.
	if (!view) {
		return nil;
	}
	
	// Work out what side to put the window on if it's "automatic".
	if (side == MAPositionAutomatic) {
		side = [[self class] _bestSideForAutomaticPositionOfViewWithSize:[view frame].size andMargin:DEFAULT_MARGIN
																			arrowHeight:DEFAULT_ARROW_HEIGHT
																			arrowInset:ARROW_INSET
																			distance:distance at:point
																			onParentWindow:parentWindow];
	}
	
	// Create initial contentRect for the attached window (can reset frame at any time).
	// Note that the content rectangle given to NSWindow is just the frame rectangle;
	// the illusion of a window margin is created during rendering.
	NSRect frameRectViewRelative = [[self class] _frameRectForViewRect:[view frame] viewMargin:DEFAULT_MARGIN arrowHeight:DEFAULT_ARROW_HEIGHT
																		distance:distance side:side];
	NSRect contentRectOnScreen = NSZeroRect;
	contentRectOnScreen.size = frameRectViewRelative.size;
	contentRectOnScreen.origin = [[self class] _idealFrameOriginForSize:contentRectOnScreen.size arrowInset:ARROW_INSET distance:distance
																		 at:((parentWindow) ? [parentWindow convertBaseToScreen:point] : point)
																		 side:side];
	
	if ((self = [super initWithContentRect:contentRectOnScreen styleMask:NSBorderlessWindowMask
											backing:NSBackingStoreBuffered defer:NO])) {
		_view = view;
		_window = parentWindow;
		_side = side;
		_distance = distance;
		
		// Configure window characteristics.
		[super setBackgroundColor:[NSColor clearColor]];
		[self setMovableByWindowBackground:NO];
		[self setExcludedFromWindowsMenu:YES];
		[self setAlphaValue:1.0];
		[self setOpaque:NO];
		[self setHasShadow:YES];
		[self useOptimizedDrawing:YES];
		
		// Set up some sensible defaults for display.
		_MABackgroundColor = [[NSColor colorWithCalibratedWhite:0.1 alpha:0.75] copy];
		borderColor = [[NSColor whiteColor] copy];
		borderWidth = 2.0;
		viewMargin = DEFAULT_MARGIN;
		arrowBaseWidth = DEFAULT_ARROW_BASE_WIDTH;
		arrowHeight = DEFAULT_ARROW_HEIGHT;
		hasArrow = YES;
		cornerRadius = DEFAULT_CORNER_RADIUS;
		drawsRoundCornerBesideArrow = DEFAULT_IS_ARROW_CORNER_ROUNDED;
		_resizing = NO;
		
		// Add view as subview of our contentView.
		[[self contentView] addSubview:_view];
		
		// Update the view.
		[self _fixViewFrame];
		[self _updateBackground];
		
		// Subscribe to notifications for when we change size.
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(windowDidResize:)
													 name:NSWindowDidResizeNotification
												   object:self];
	}
	return self;
}


- (MAAttachedWindow *)initWithView:(NSView *)view
				   attachedToPoint:(NSPoint)point
						  inWindow:(NSWindow *)window
						atDistance:(float)distance
{
	return [self initWithView:view attachedToPoint:point
					 inWindow:window onSide:MAPositionAutomatic
				   atDistance:distance];
}


- (MAAttachedWindow *)initWithView:(NSView *)view
				   attachedToPoint:(NSPoint)point
							onSide:(MAWindowPosition)side
						atDistance:(float)distance
{
	return [self initWithView:view attachedToPoint:point
					 inWindow:nil onSide:side
				   atDistance:distance];
}


- (MAAttachedWindow *)initWithView:(NSView *)view
				   attachedToPoint:(NSPoint)point
						atDistance:(float)distance
{
	return [self initWithView:view attachedToPoint:point
					 inWindow:nil onSide:MAPositionAutomatic
				   atDistance:distance];
}


- (MAAttachedWindow *)initWithView:(NSView *)view
				   attachedToPoint:(NSPoint)point
						  inWindow:(NSWindow *)window
{
	return [self initWithView:view attachedToPoint:point
					 inWindow:window onSide:MAPositionAutomatic
				   atDistance:0];
}


- (MAAttachedWindow *)initWithView:(NSView *)view
				   attachedToPoint:(NSPoint)point
							onSide:(MAWindowPosition)side
{
	return [self initWithView:view attachedToPoint:point
					 inWindow:nil onSide:side
				   atDistance:0];
}


- (MAAttachedWindow *)initWithView:(NSView *)view
				   attachedToPoint:(NSPoint)point
{
	return [self initWithView:view attachedToPoint:point
					 inWindow:nil onSide:MAPositionAutomatic
				   atDistance:0];
}


- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[borderColor release];
	[_MABackgroundColor release];
	
	[super dealloc];
}


#pragma mark Geometry


// should be consistent with "_frameRectForViewRect:viewMargin:arrowHeight:distance:side:"
+ (NSRect)_viewRectForFrameRect:(NSRect)aRect
					 viewMargin:(float)viewMargin
					arrowHeight:(float)arrowHeight
					   distance:(float)distance
						   side:(MAWindowPosition)side
{
	NSRect result = aRect;
	float const OFFSET_ARROW_SIDE = (arrowHeight + distance + viewMargin);
	float const OFFSET_NON_ARROW_SIDE = (viewMargin);
	// Account for arrow size and position (whether or not arrow is visible).
	switch (side) {
		case MAPositionLeft:
		case MAPositionLeftTop:
		case MAPositionLeftBottom:
			result.origin.x += OFFSET_NON_ARROW_SIDE;
			result.origin.y += OFFSET_NON_ARROW_SIDE;
			result.size.width -= (OFFSET_NON_ARROW_SIDE + OFFSET_ARROW_SIDE);
			result.size.height -= (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			break;
		case MAPositionRight:
		case MAPositionRightTop:
		case MAPositionRightBottom:
			result.origin.x += OFFSET_ARROW_SIDE;
			result.origin.y += OFFSET_NON_ARROW_SIDE;
			result.size.width -= (OFFSET_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			result.size.height -= (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			break;
		case MAPositionBottom:
		case MAPositionBottomLeft:
		case MAPositionBottomRight:
			result.origin.x += OFFSET_NON_ARROW_SIDE;
			result.origin.y += OFFSET_NON_ARROW_SIDE;
			result.size.width -= (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			result.size.height -= (OFFSET_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			break;
		case MAPositionTop:
		case MAPositionTopLeft:
		case MAPositionTopRight:
		default:
			result.origin.x += OFFSET_NON_ARROW_SIDE;
			result.origin.y += OFFSET_ARROW_SIDE;
			result.size.width -= (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			result.size.height -= (OFFSET_NON_ARROW_SIDE + OFFSET_ARROW_SIDE);
			break;
	}
	return result;
}


// should be consistent with "_viewRectForFrameRect:viewMargin:arrowHeight:distance:side:"
+ (NSRect)_frameRectForViewRect:(NSRect)aRect
					 viewMargin:(float)viewMargin
					arrowHeight:(float)arrowHeight
					   distance:(float)distance
						   side:(MAWindowPosition)side
{
	NSRect result = aRect;
	float const OFFSET_ARROW_SIDE = (arrowHeight + distance + viewMargin);
	float const OFFSET_NON_ARROW_SIDE = (viewMargin);
	// Account for arrow size and position (whether or not arrow is visible).
	switch (side) {
		case MAPositionLeft:
		case MAPositionLeftTop:
		case MAPositionLeftBottom:
			result.origin.x -= OFFSET_NON_ARROW_SIDE;
			result.origin.y -= OFFSET_NON_ARROW_SIDE;
			result.size.width += (OFFSET_NON_ARROW_SIDE + OFFSET_ARROW_SIDE);
			result.size.height += (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			break;
		case MAPositionRight:
		case MAPositionRightTop:
		case MAPositionRightBottom:
			result.origin.x -= OFFSET_ARROW_SIDE;
			result.origin.y -= OFFSET_NON_ARROW_SIDE;
			result.size.width += (OFFSET_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			result.size.height += (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			break;
		case MAPositionBottom:
		case MAPositionBottomLeft:
		case MAPositionBottomRight:
			result.origin.x -= OFFSET_NON_ARROW_SIDE;
			result.origin.y -= OFFSET_NON_ARROW_SIDE;
			result.size.width += (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			result.size.height += (OFFSET_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			break;
		case MAPositionTop:
		case MAPositionTopLeft:
		case MAPositionTopRight:
		default:
			result.origin.x -= OFFSET_NON_ARROW_SIDE;
			result.origin.y -= OFFSET_ARROW_SIDE;
			result.size.width += (OFFSET_NON_ARROW_SIDE + OFFSET_NON_ARROW_SIDE);
			result.size.height += (OFFSET_NON_ARROW_SIDE + OFFSET_ARROW_SIDE);
			break;
	}
	return result;
}


+ (NSPoint)_idealFrameOriginForSize:(NSSize)frameSize
						 arrowInset:(float)arrowInset
						   distance:(float)distance
								 at:(NSPoint)point
							   side:(MAWindowPosition)side
{
	// Position frame origin appropriately for _side, accounting for arrow-inset.
	NSPoint result = point;
	float halfWidth = frameSize.width / 2.0;
	float halfHeight = frameSize.height / 2.0;
	switch (side) {
		case MAPositionTopLeft:
			result.x -= frameSize.width - arrowInset;
			break;
		case MAPositionTopRight:
			result.x -= arrowInset;
			break;
		case MAPositionBottomLeft:
			result.y -= frameSize.height;
			result.x -= frameSize.width - arrowInset;
			break;
		case MAPositionBottom:
			result.y -= frameSize.height;
			result.x -= halfWidth;
			break;
		case MAPositionBottomRight:
			result.x -= arrowInset;
			result.y -= frameSize.height;
			break;
		case MAPositionLeftTop:
			result.x -= frameSize.width;
			result.y -= arrowInset;
			break;
		case MAPositionLeft:
			result.x -= frameSize.width;
			result.y -= halfHeight;
			break;
		case MAPositionLeftBottom:
			result.x -= frameSize.width;
			result.y -= frameSize.height - arrowInset;
			break;
		case MAPositionRightTop:
			result.y -= arrowInset;
			break;
		case MAPositionRight:
			result.y -= halfHeight;
			break;
		case MAPositionRightBottom:
			result.y -= frameSize.height - arrowInset;
			break;
		case MAPositionTop:
		default:
			result.x -= halfWidth;
			break;
	}
	// Account for distance in new window frame.
	switch (side) {
		case MAPositionLeft:
		case MAPositionLeftTop:
		case MAPositionLeftBottom:
			result.x -= distance;
			break;
		case MAPositionRight:
		case MAPositionRightTop:
		case MAPositionRightBottom:
			result.x += distance;
			break;
		case MAPositionBottom:
		case MAPositionBottomLeft:
		case MAPositionBottomRight:
			result.y -= distance;
			break;
		case MAPositionTop:
		case MAPositionTopLeft:
		case MAPositionTopRight:
		default:
			result.y += distance;
			break;
	}
    return result;
}


// per-instance version of _idealFrameOriginForSize:arrowInset:distance:at:side:
// (typically should not be called from an initializer)
- (NSPoint)_idealFrameOriginForPoint:(NSPoint)aPoint
{
	return [[self class] _idealFrameOriginForSize:[self frame].size arrowInset:[self _arrowInset] distance:_distance
													at:((_window) ? [_window convertBaseToScreen:aPoint] : aPoint) side:_side];
}


- (void)_fixViewFrame
{
	_viewFrame = [self viewRectForFrameRect:[self frame]];
	_viewFrame.origin = [self convertScreenToBase:_viewFrame.origin];
	[_view setFrame:_viewFrame];
}


+ (MAWindowPosition)_bestSideForAutomaticPositionOfViewWithSize:(NSSize)viewSize
													  andMargin:(float)viewMargin
													arrowHeight:(float)arrowHeight
													 arrowInset:(float)arrowInset
													   distance:(float)distance
															 at:(NSPoint)aPoint
												 onParentWindow:(NSWindow *)aWindow
{
	// Get all relevant geometry in screen coordinates.
	NSRect screenFrame;
	if (aWindow && [aWindow screen]) {
		screenFrame = [[aWindow screen] visibleFrame];
	} else {
		screenFrame = [[NSScreen mainScreen] visibleFrame];
	}
	NSPoint pointOnScreen = (aWindow) ? [aWindow convertBaseToScreen:aPoint] : aPoint;
	viewSize.width += viewMargin * 2.0;
	viewSize.height += viewMargin * 2.0;
	MAWindowPosition side = MAPositionBottom; // By default, position us centered below.
	float scaledArrowHeight = arrowHeight + distance;
	
	// We'd like to display directly below the specified point, since this gives a
	// sense of a relationship between the point and this window. Check there's room.
	if (pointOnScreen.y - viewSize.height - scaledArrowHeight < NSMinY(screenFrame)) {
		// We'd go off the bottom of the screen. Try the right.
		if (pointOnScreen.x + viewSize.width + scaledArrowHeight >= NSMaxX(screenFrame)) {
			// We'd go off the right of the screen. Try the left.
			if (pointOnScreen.x - viewSize.width - scaledArrowHeight < NSMinX(screenFrame)) {
				// We'd go off the left of the screen. Try the top.
				if (pointOnScreen.y + viewSize.height + scaledArrowHeight < NSMaxY(screenFrame)) {
					side = MAPositionTop;
				}
			} else {
				side = MAPositionLeft;
			}
		} else {
			side = MAPositionRight;
		}
	}
	
	float halfWidth = viewSize.width / 2.0;
	float halfHeight = viewSize.height / 2.0;
	
	NSRect parentFrame = (aWindow) ? [aWindow frame] : screenFrame;
	
	// We're currently at a primary side.
	// Try to avoid going outwith the parent area in the secondary dimension,
	// by checking to see if an appropriate corner side would be better.
	switch (side) {
		case MAPositionRight:
		case MAPositionLeft:
			// Check to see if we go beyond the bottom edge of the parent area.
			if (pointOnScreen.y - halfHeight < NSMinY(parentFrame)) {
				// We go beyond the bottom edge. Try using top position.
				if (pointOnScreen.y + viewSize.height - arrowInset < NSMaxY(screenFrame)) {
					// We'd still be on-screen using top, so use it.
					if (side == MAPositionRight) {
						side = MAPositionRightTop;
					} else {
						side = MAPositionLeftTop;
					}
				}
			} else if (pointOnScreen.y + halfHeight >= NSMaxY(parentFrame)) {
				// We go beyond the top edge. Try using bottom position.
				if (pointOnScreen.y - viewSize.height + arrowInset >= NSMinY(screenFrame)) {
					// We'd still be on-screen using bottom, so use it.
					if (side == MAPositionRight) {
						side = MAPositionRightBottom;
					} else {
						side = MAPositionLeftBottom;
					}
				}
			}
			break;
		case MAPositionBottom:
		case MAPositionTop:
		default:
			// Check to see if we go beyond the left edge of the parent area.
			if (pointOnScreen.x - halfWidth < NSMinX(parentFrame)) {
				// We go beyond the left edge. Try using right position.
				if (pointOnScreen.x + viewSize.width - arrowInset < NSMaxX(screenFrame)) {
					// We'd still be on-screen using right, so use it.
					if (side == MAPositionBottom) {
						side = MAPositionBottomRight;
					} else {
						side = MAPositionTopRight;
					}
				}
			} else if (pointOnScreen.x + halfWidth >= NSMaxX(parentFrame)) {
				// We go beyond the right edge. Try using left position.
				if (pointOnScreen.x - viewSize.width + arrowInset >= NSMinX(screenFrame)) {
					// We'd still be on-screen using left, so use it.
					if (side == MAPositionBottom) {
						side = MAPositionBottomLeft;
					} else {
						side = MAPositionTopLeft;
					}
				}
			}
			break;
	}
	
	return side;
}


+ (float)_arrowInsetWithCornerRadius:(float)cornerRadius
						   baseWidth:(float)arrowBaseWidth
{
	return (cornerRadius + (arrowBaseWidth / 2.0));
}


- (float)_arrowInset
{
	return [[self class] _arrowInsetWithCornerRadius:((drawsRoundCornerBesideArrow) ? cornerRadius : 0) baseWidth:arrowBaseWidth];
}


#pragma mark Drawing


- (void)_updateBackground
{
	// Call NSWindow's implementation of -setBackgroundColor: because we override
	// it in this class to let us set the entire background image of the window
	// as an NSColor patternImage.
	NSDisableScreenUpdates();
	[super setBackgroundColor:[self _backgroundColorPatternImage]];
	if ([self isVisible]) {
		[self display];
		[self invalidateShadow];
	}
	NSEnableScreenUpdates();
}


- (NSColor *)_backgroundColorPatternImage
{
	NSImage *bg = [[NSImage alloc] initWithSize:[self frame].size];
	NSRect bgRect = NSZeroRect;
	bgRect.size = [bg size];
	
	[bg lockFocus];
	NSBezierPath *bgPath = [self _backgroundPath];
	[NSGraphicsContext saveGraphicsState];
	[bgPath addClip];
	
	// Draw background.
	[_MABackgroundColor set];
	[bgPath fill];
	
	// Draw border if appropriate.
	if (borderWidth > 0) {
		// Double the borderWidth since we're drawing inside the path.
		[bgPath setLineWidth:(borderWidth * 2.0)];
		[borderColor set];
		[bgPath stroke];
	}
	
	[NSGraphicsContext restoreGraphicsState];
	[bg unlockFocus];
	
	return [NSColor colorWithPatternImage:[bg autorelease]];
}


- (NSBezierPath *)_backgroundPath
{
	/*
	 Construct path for window background, taking account of:
	 1. hasArrow
	 2. _side
	 3. drawsRoundCornerBesideArrow
	 4. arrowBaseWidth
	 5. arrowHeight
	 6. cornerRadius
	 */
	
	float scaleFactor = 1.0;
	float scaledRadius = cornerRadius * scaleFactor;
	float scaledArrowWidth = arrowBaseWidth * scaleFactor;
	float halfArrowWidth = scaledArrowWidth / 2.0;
	NSRect contentArea = NSInsetRect(_viewFrame,
									 -viewMargin * scaleFactor,
									 -viewMargin * scaleFactor);
	float minX = ceilf(NSMinX(contentArea) * scaleFactor + 0.5f);
	float midX = NSMidX(contentArea) * scaleFactor;
	float maxX = floorf(NSMaxX(contentArea) * scaleFactor - 0.5f);
	float minY = ceilf(NSMinY(contentArea) * scaleFactor + 0.5f);
	float midY = NSMidY(contentArea) * scaleFactor;
	float maxY = floorf(NSMaxY(contentArea) * scaleFactor - 0.5f);
	
	NSBezierPath *path = [NSBezierPath bezierPath];
	[path setLineJoinStyle:NSRoundLineJoinStyle];
	
	// Begin at top-left. This will be either after the rounded corner, or
	// at the top-left point if cornerRadius is zero and/or we're drawing
	// the arrow at the top-left or left-top without a rounded corner.
	NSPoint currPt = NSMakePoint(minX, maxY);
	if (scaledRadius > 0 &&
		(drawsRoundCornerBesideArrow ||
			(_side != MAPositionBottomRight && _side != MAPositionRightBottom))
		) {
		currPt.x += scaledRadius;
	}
	
	NSPoint endOfLine = NSMakePoint(maxX, maxY);
	BOOL shouldDrawNextCorner = NO;
	if (scaledRadius > 0 &&
		(drawsRoundCornerBesideArrow ||
		 (_side != MAPositionBottomLeft && _side != MAPositionLeftBottom))
		) {
		endOfLine.x -= scaledRadius;
		shouldDrawNextCorner = YES;
	}
	
	[path moveToPoint:currPt];
	
	// If arrow should be drawn at top-left point, draw it.
	if (_side == MAPositionBottomRight) {
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionBottom) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(midX - halfArrowWidth, maxY)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionBottomLeft) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(endOfLine.x - scaledArrowWidth, maxY)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	}
	
	// Line to end of this side.
	[path lineToPoint:endOfLine];
	
	// Rounded corner on top-right.
	if (shouldDrawNextCorner) {
		[path appendBezierPathWithArcFromPoint:NSMakePoint(maxX, maxY)
									   toPoint:NSMakePoint(maxX, maxY - scaledRadius)
										radius:scaledRadius];
	}
	
	
	// Draw the right side, beginning at the top-right.
	endOfLine = NSMakePoint(maxX, minY);
	shouldDrawNextCorner = NO;
	if (scaledRadius > 0 &&
		(drawsRoundCornerBesideArrow ||
		 (_side != MAPositionTopLeft && _side != MAPositionLeftTop))
		) {
		endOfLine.y += scaledRadius;
		shouldDrawNextCorner = YES;
	}
	
	// If arrow should be drawn at right-top point, draw it.
	if (_side == MAPositionLeftBottom) {
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionLeft) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(maxX, midY + halfArrowWidth)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionLeftTop) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(maxX, endOfLine.y + scaledArrowWidth)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	}
	
	// Line to end of this side.
	[path lineToPoint:endOfLine];
	
	// Rounded corner on bottom-right.
	if (shouldDrawNextCorner) {
		[path appendBezierPathWithArcFromPoint:NSMakePoint(maxX, minY)
									   toPoint:NSMakePoint(maxX - scaledRadius, minY)
										radius:scaledRadius];
	}
	
	
	// Draw the bottom side, beginning at the bottom-right.
	endOfLine = NSMakePoint(minX, minY);
	shouldDrawNextCorner = NO;
	if (scaledRadius > 0 &&
		(drawsRoundCornerBesideArrow ||
		 (_side != MAPositionTopRight && _side != MAPositionRightTop))
		) {
		endOfLine.x += scaledRadius;
		shouldDrawNextCorner = YES;
	}
	
	// If arrow should be drawn at bottom-right point, draw it.
	if (_side == MAPositionTopLeft) {
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionTop) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(midX + halfArrowWidth, minY)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionTopRight) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(endOfLine.x + scaledArrowWidth, minY)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	}
	
	// Line to end of this side.
	[path lineToPoint:endOfLine];
	
	// Rounded corner on bottom-left.
	if (shouldDrawNextCorner) {
		[path appendBezierPathWithArcFromPoint:NSMakePoint(minX, minY)
									   toPoint:NSMakePoint(minX, minY + scaledRadius)
										radius:scaledRadius];
	}
	
	
	// Draw the left side, beginning at the bottom-left.
	endOfLine = NSMakePoint(minX, maxY);
	shouldDrawNextCorner = NO;
	if (scaledRadius > 0 &&
		(drawsRoundCornerBesideArrow ||
		 (_side != MAPositionRightBottom && _side != MAPositionBottomRight))
		) {
		endOfLine.y -= scaledRadius;
		shouldDrawNextCorner = YES;
	}
	
	// If arrow should be drawn at left-bottom point, draw it.
	if (_side == MAPositionRightTop) {
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionRight) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(minX, midY - halfArrowWidth)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	} else if (_side == MAPositionRightBottom) {
		// Line to relevant point before arrow.
		[path lineToPoint:NSMakePoint(minX, endOfLine.y - scaledArrowWidth)];
		// Draw arrow.
		[self _appendArrowToPath:path];
	}
	
	// Line to end of this side.
	[path lineToPoint:endOfLine];
	
	// Rounded corner on top-left.
	if (shouldDrawNextCorner) {
		[path appendBezierPathWithArcFromPoint:NSMakePoint(minX, maxY)
									   toPoint:NSMakePoint(minX + scaledRadius, maxY)
										radius:scaledRadius];
	}
	
	[path closePath];
	return path;
}


- (void)_appendArrowToPath:(NSBezierPath *)path
{
	if (!hasArrow) {
		return;
	}
	
	float scaleFactor = 1.0;
	float scaledArrowWidth = arrowBaseWidth * scaleFactor;
	float halfArrowWidth = scaledArrowWidth / 2.0;
	float scaledArrowHeight = arrowHeight * scaleFactor;
	NSPoint currPt = [path currentPoint];
	NSPoint tipPt = currPt;
	NSPoint endPt = currPt;
	
	// Note: we always build the arrow path in a clockwise direction.
	switch (_side) {
		case MAPositionLeft:
		case MAPositionLeftTop:
		case MAPositionLeftBottom:
			// Arrow points towards right. We're starting from the top.
			tipPt.x += scaledArrowHeight;
			tipPt.y -= halfArrowWidth;
			endPt.y -= scaledArrowWidth;
			break;
		case MAPositionRight:
		case MAPositionRightTop:
		case MAPositionRightBottom:
			// Arrow points towards left. We're starting from the bottom.
			tipPt.x -= scaledArrowHeight;
			tipPt.y += halfArrowWidth;
			endPt.y += scaledArrowWidth;
			break;
		case MAPositionBottom:
		case MAPositionBottomLeft:
		case MAPositionBottomRight:
			// Arrow points towards top. We're starting from the left.
			tipPt.y += scaledArrowHeight;
			tipPt.x += halfArrowWidth;
			endPt.x += scaledArrowWidth;
			break;
		case MAPositionTop:
		case MAPositionTopLeft:
		case MAPositionTopRight:
		default:
			// Arrow points towards bottom. We're starting from the right.
			tipPt.y -= scaledArrowHeight;
			tipPt.x -= halfArrowWidth;
			endPt.x -= scaledArrowWidth;
			break;
	}
	
	[path lineToPoint:tipPt];
	[path lineToPoint:endPt];
}


- (void)_redisplay
{
	if (_resizing) {
		return;
	}
	
	_resizing = YES;
	NSDisableScreenUpdates();
	[self _fixViewFrame];
	[self _updateBackground];
	NSEnableScreenUpdates();
	_resizing = NO;
}


#pragma mark Window Behaviour


- (BOOL)canBecomeMainWindow
{
	return NO;
}


- (BOOL)canBecomeKeyWindow
{
	return YES;
}


- (BOOL)isExcludedFromWindowsMenu
{
	return YES;
}


- (BOOL)validateMenuItem:(NSMenuItem *)item
{
	if (_window) {
		return [_window validateMenuItem:item];
	}
	return [super validateMenuItem:item];
}


- (IBAction)performClose:(id)sender
{
	if (_window) {
		[_window performClose:sender];
	} else {
		[super performClose:sender];
	}
}


#pragma mark Notification handlers


- (void)windowDidResize:(NSNotification *)note
{
#pragma unused(note)
	[self _redisplay];
}


#pragma mark Accessors


- (void)setPoint:(NSPoint)point side:(MAWindowPosition)side
{
	if (side == MAPositionAutomatic) {
		side = [[self class] _bestSideForAutomaticPositionOfViewWithSize:[self frame].size andMargin:[self viewMargin]
																			arrowHeight:[self arrowHeight]
																			arrowInset:[self _arrowInset]
																			distance:_distance at:point
																			onParentWindow:_window];
	}
	_side = side;
	NSRect windowFrame = [self frameRectForViewRect:[_view frame]]; // ignore origin, use only size
	NSPoint idealOrigin = [self _idealFrameOriginForPoint:point];
	windowFrame.origin.x = idealOrigin.x;
	windowFrame.origin.y = idealOrigin.y;
	NSDisableScreenUpdates();
	[self setFrame:windowFrame display:NO];
	[self _fixViewFrame];
	[self _updateBackground];
	NSEnableScreenUpdates();
}


- (NSColor *)windowBackgroundColor {
	return [[_MABackgroundColor retain] autorelease];
}


- (void)setBackgroundColor:(NSColor *)value {
	if (_MABackgroundColor != value) {
		[_MABackgroundColor release];
		_MABackgroundColor = [value copy];
		
		[self _updateBackground];
	}
}


- (NSColor *)borderColor {
	return [[borderColor retain] autorelease];
}


- (void)setBorderColor:(NSColor *)value {
	if (borderColor != value) {
		[borderColor release];
		borderColor = [value copy];
		
		[self _updateBackground];
	}
}


- (float)borderWidth {
	return borderWidth;
}


- (void)setBorderWidth:(float)value {
	if (borderWidth != value) {
		float maxBorderWidth = viewMargin;
		if (value <= maxBorderWidth) {
			borderWidth = value;
		} else {
			borderWidth = maxBorderWidth;
		}
		
		[self _updateBackground];
	}
}


- (float)viewMargin {
	return viewMargin;
}


- (void)setViewMargin:(float)value {
	if (viewMargin != value) {
		viewMargin = MAX(value, 0.0);
		
		// Adjust cornerRadius appropriately (which will also adjust arrowBaseWidth).
		[self setCornerRadius:cornerRadius];
	}
}


- (float)arrowBaseWidth {
	return arrowBaseWidth;
}


- (void)setArrowBaseWidth:(float)value {
	float maxWidth = (MIN(_viewFrame.size.width, _viewFrame.size.height) +
					  (viewMargin * 2.0)) - cornerRadius;
	if (drawsRoundCornerBesideArrow) {
		maxWidth -= cornerRadius;
	}
	if (value <= maxWidth) {
		arrowBaseWidth = value;
	} else {
		arrowBaseWidth = maxWidth;
	}
	
	[self _redisplay];
}


- (float)arrowHeight {
	return arrowHeight;
}


- (void)setArrowHeight:(float)value {
	if (arrowHeight != value) {
		arrowHeight = value;
		
		[self _redisplay];
	}
}


- (float)hasArrow {
	return hasArrow;
}


- (void)setHasArrow:(float)value {
	if (hasArrow != value) {
		hasArrow = value;
		
		[self _updateBackground];
	}
}


- (float)cornerRadius {
	return cornerRadius;
}


- (void)setCornerRadius:(float)value {
	float maxRadius = ((MIN(_viewFrame.size.width, _viewFrame.size.height) +
						(viewMargin * 2.0)) - arrowBaseWidth) / 2.0;
	if (value <= maxRadius) {
		cornerRadius = value;
	} else {
		cornerRadius = maxRadius;
	}
	cornerRadius = MAX(cornerRadius, 0.0);
	
	// Adjust arrowBaseWidth appropriately.
	[self setArrowBaseWidth:arrowBaseWidth];
}


- (float)drawsRoundCornerBesideArrow {
	return drawsRoundCornerBesideArrow;
}


- (void)setDrawsRoundCornerBesideArrow:(float)value {
	if (drawsRoundCornerBesideArrow != value) {
		drawsRoundCornerBesideArrow = value;
		
		[self _redisplay];
	}
}


- (void)setBackgroundImage:(NSImage *)value
{
	if (value) {
		[self setBackgroundColor:[NSColor colorWithPatternImage:value]];
	}
}


#pragma mark General


- (NSRect)viewRectForFrameRect:(NSRect)aRect
{
	return [[self class] _viewRectForFrameRect:aRect viewMargin:[self viewMargin] arrowHeight:[self arrowHeight] distance:_distance side:_side];
}


- (NSRect)frameRectForViewRect:(NSRect)aRect
{
	return [[self class] _frameRectForViewRect:aRect viewMargin:[self viewMargin] arrowHeight:[self arrowHeight] distance:_distance side:_side];
}


@end
