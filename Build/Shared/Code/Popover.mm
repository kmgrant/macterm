/*!	\file Popover.mm
	\brief Implements a popover-style window.
*/
/*###############################################################

	Popover Window 1.4 (based on MAAttachedWindow)
	MAAttachedWindow © 2007 by Magic Aubergine
	Popover Window © 2011-2017 by Kevin Grant
	
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

// library includes
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>



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
	+ (BOOL)
	isGraphiteTheme;
	+ (Popover_Properties)
	windowPlacement:(Popover_Properties)_;

// class methods: frame conversion
	+ (NSPoint)
	convertToScreenFromWindow:(NSWindow*)_
	point:(NSPoint)_;
	+ (NSPoint)
	convertToWindow:(NSWindow*)_
	fromScreenPoint:(NSPoint)_;
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
	applyStyle:(Popover_WindowStyle)_;
	- (NSPoint)
	convertToScreenFromWindowPoint:(NSPoint)_;
	- (NSPoint)
	convertToWindowFromScreenPoint:(NSPoint)_;
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

@interface Popover_Window () //{

// accessors
	@property (copy) NSColor*
	borderOuterDisplayColor;
	@property (copy) NSColor*
	borderPrimaryDisplayColor;

@end //}


#pragma mark Internal Methods

#pragma mark -
@implementation Popover_Window


/*!
The height determines how “slender” the frame arrow’s triangle is.
*/
@synthesize arrowHeight = _arrowHeight;

/*!
The outer border color is used to render the boundary of
the popover window.  See also "setBorderPrimaryColor:".

The border thickness is determined by "setBorderWidth:".
If the outer and primary colors are the same then the
border appears to be that thickness; otherwise the width
is divided roughly evenly between the two colors.
*/
@synthesize borderOuterColor = _borderOuterColor;

/*!
The outer border display color is what is currently used
for rendering, which may either be the value of the
property "borderOuterColor" or a temporary setting such
as a gray border for inactive windows.
*/
@synthesize borderOuterDisplayColor = _borderOuterDisplayColor;

/*!
The primary border color is used to render a frame inside
the outer border (see "setBorderOuterColor:").

The border thickness is determined by "setBorderWidth:".
If the outer and primary colors are the same then the
border appears to be that thickness; otherwise the width
is divided roughly evenly between the two colors.
*/
@synthesize borderPrimaryColor = _borderPrimaryColor;

/*!
The primary border display color is what is currently used
for rendering, which may either be the value of the
property "borderPrimaryColor" or a temporary setting such
as a gray border for inactive windows.
*/
@synthesize borderPrimaryDisplayColor = _borderPrimaryDisplayColor;

/*!
Specifies whether or not the frame has an arrow displayed.
Even if there is no arrow, space is allocated for the arrow
so that it can appear or disappear without the user seeing
the window resize.
*/
@synthesize hasArrow = _hasArrow;

/*!
When the window position puts the arrow near a corner of
the frame, this specifies how close to the corner the
arrow is.  If a rounded corner appears then the arrow is
off to the side; otherwise the arrow is right in the
corner.
*/
@synthesize hasRoundCornerBesideArrow = _hasRoundCornerBesideArrow;

/*!
The popover background color is used to construct an image
that the NSWindow superclass uses for rendering.

Use this property instead of the NSWindow "backgroundColor"
property because the normal background color is overridden
to contain the entire rendering of the popover window frame
(as a pattern image).
*/
@synthesize popoverBackgroundColor = _popoverBackgroundColor;


#pragma mark Initializers


/*!
Convenience initializer; assumes a value for vibrancy.

(1.4)
*/
- (instancetype)
initWithView:(NSView*)			aView
style:(Popover_WindowStyle)		aStyle
attachedToPoint:(NSPoint)		aPoint
inWindow:(NSWindow*)			aWindow
{
	return [self initWithView:aView style:aStyle attachedToPoint:aPoint
								inWindow:aWindow vibrancy:YES];
}


/*!
Designated initializer.

A popover embeds a single view (which can of course contain any number
of other views).

The style pre-configures various properties of the frame to produce
standard styles.  Although you can change properties later, it is
highly recommended that you use the defaults.

At construction time you only provide a point that determines where the
tip of the window frame’s arrow is (the distance indicates how far the
arrow is from that point).  This anchor point is used even if the
style of the window does not actually render an arrow; thus, for styles
such as sheets, you simply specify the center-top point by default.

If a parent window is provided then "point" is in the coordinates of
that window; otherwise the "point" is in screen coordinates.

The initial window size is based on the view size and some default
values for margin, etc. and the location is also chosen automatically.
You may change any property of the popover later however, and ideally
before the window is shown to the user.

If the vibrancy flag is set, an NSVisualEffectView is inserted into
the view hierarchy so that the entire window appears to be translucent
while blurring the background.

(1.4)
*/
- (instancetype)
initWithView:(NSView*)			aView
style:(Popover_WindowStyle)		aStyle
attachedToPoint:(NSPoint)		aPoint
inWindow:(NSWindow*)			aWindow
vibrancy:(BOOL)					aVisualEffectFlag
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
			self->registeredObservers = [[NSMutableArray alloc] init];
			
			self->popoverParentWindow = aWindow;
			self->embeddedView = aView;
			self->windowPropertyFlags = side;
			self->isAutoPositioning = YES;
			
			// configure window
			[super setBackgroundColor:[NSColor clearColor]];
			[self setMovableByWindowBackground:NO];
			[self setExcludedFromWindowsMenu:YES];
			[self setPreventsApplicationTerminationWhenModal:NO];
			[self setAlphaValue:1.0];
			[self setOpaque:NO];
			[self setHasShadow:YES];
			[self useOptimizedDrawing:YES];
			
			// set up some sensible defaults for display
			self->_popoverBackgroundColor = [[NSColor colorWithCalibratedWhite:0.1f alpha:0.75f] copy];
			self->_borderOuterColor = [[NSColor grayColor] copy];
			self->_borderOuterDisplayColor = [self->_borderOuterColor copy];
			self->_borderPrimaryColor = [[NSColor whiteColor] copy];
			self->_borderPrimaryDisplayColor = [self->_borderPrimaryColor copy];
			self->borderWidth = 2.0;
			self->viewMargin = kDefaultMargin;
			self->arrowBaseWidth = kDefaultArrowBaseWidth;
			self->_arrowHeight = kDefaultArrowHeight;
			self->_hasArrow = YES;
			self->cornerRadius = kDefaultCornerRadius;
			self->_hasRoundCornerBesideArrow = kDefaultCornerIsRounded;
			self->resizeInProgress = NO;
			
			// add view as subview
			[[self contentView] addSubview:self->embeddedView];
			
			// if supported by the runtime OS, add a vibrancy effect to
			// make the entire window more translucent than transparent
			// (NOTE: this does not currently mix well with the arrow
			// rendering in all cases, because the arrow background
			// does not include the vibrancy effect; this might be
			// fixable later by reimplementing the window to use a more
			// elaborate scheme for arrows, such as view clipping)
			if (aVisualEffectFlag)
			{
				@try
				{
					NSView*		parentView = [[self->embeddedView subviews] objectAtIndex:0];
					id			visualEffectObject = CocoaFuture_AllocInitVisualEffectViewWithFrame
														(NSRectToCGRect(parentView.frame));
					
					
					if (nil != visualEffectObject)
					{
						NSView*		visualEffectView = STATIC_CAST(visualEffectObject, NSView*);
						NSArray*	subviews = [[[parentView subviews] copy] autorelease];
						
						
						if (nil == subviews)
						{
							// this should never happen...
							subviews = @[];
						}
						
						visualEffectView.autoresizingMask = (NSViewMinXMargin | NSViewWidthSizable | NSViewMaxXMargin |
																NSViewMinYMargin | NSViewHeightSizable | NSViewMaxYMargin);
						
						// keep the views from disappearing when they are removed temporarily
						[subviews enumerateObjectsUsingBlock:^(id object, NSUInteger index, BOOL *stop)
						{
						#pragma unused(index, stop)
							NSView*		asView = STATIC_CAST(object, NSView*);
							
							
							[[object retain] autorelease];
							[asView removeFromSuperview];
						}];
						
						[visualEffectView setSubviews:subviews];
						[parentView setWantsLayer:YES];
						[parentView addSubview:visualEffectView];
						
						[visualEffectObject release], visualEffectObject = nil;
					}
					else
					{
						// OS is too old to support this effect (no problem)
					}
				}
				@catch (NSException*	inException)
				{
					// since this is just a visual adornment, do not allow
					// exceptions to cause problems; just log and continue
					Console_Warning(Console_WriteValueCFString, "visual effect adornment failed, exception",
									BRIDGE_CAST([inException name], CFStringRef));
				}
			}
			
			// ensure that the display is updated after certain changes
			[registeredObservers addObject:[[self newObserverFromSelector:@selector(arrowHeight)] autorelease]];
			[registeredObservers addObject:[[self newObserverFromSelector:@selector(borderOuterColor)] autorelease]];
			[registeredObservers addObject:[[self newObserverFromSelector:@selector(borderPrimaryColor)] autorelease]];
			[registeredObservers addObject:[[self newObserverFromSelector:@selector(hasArrow)] autorelease]];
			[registeredObservers addObject:[[self newObserverFromSelector:@selector(hasRoundCornerBesideArrow)] autorelease]];
			[registeredObservers addObject:[[self newObserverFromSelector:@selector(popoverBackgroundColor)] autorelease]];
			
			// ensure that frame-dependent property changes will cause the
			// frame to be synchronized with the new values (note that the
			// "setBorderWidth:" method already requires the border to fit
			// within the margin so that value is not monitored here)
			[registeredObservers addObject:[self newObserverFromSelector:@selector(viewMargin)
																			ofObject:self
																			options:(NSKeyValueObservingOptionNew |
																						NSKeyValueObservingOptionOld)
																			context:nullptr]];
			
			// subscribe to notifications
			[self whenObject:self postsNote:NSWindowDidBecomeKeyNotification
								performSelector:@selector(windowDidBecomeKey:)];
			[self whenObject:self postsNote:NSWindowDidResignKeyNotification
								performSelector:@selector(windowDidResignKey:)];
			[self whenObject:self postsNote:NSWindowDidResizeNotification
								performSelector:@selector(windowDidResize:)];
			
			// use the style parameter to modify default values
			[self applyStyle:aStyle];
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
	[self ignoreWhenObjectsPostNotes];
	
	[_borderOuterColor release];
	[_borderOuterDisplayColor release];
	[_borderPrimaryColor release];
	[_borderPrimaryDisplayColor release];
	[_popoverBackgroundColor release];
	
	// remove observers registered by initializer
	[self removeObserversSpecifiedInArray:registeredObservers];
	[registeredObservers release];
	
	[super dealloc];
}


#pragma mark Utilities


/*!
Sets the "hasArrow", "hasRoundCornerBesideArrow",
"arrowBaseWidth" and "arrowHeight" properties
simultaneously to a standard configuration.

(1.4)
*/
- (void)
setStandardArrowProperties:(BOOL)	aHasArrowFlag
{
	if (aHasArrowFlag)
	{
		self.hasArrow = YES;
		self.hasRoundCornerBesideArrow = YES;
		self.arrowBaseWidth = 30.0;
		self.arrowHeight = 15.0;
	}
	else
	{
		self.hasArrow = NO;
		self.hasRoundCornerBesideArrow = YES;
		self.arrowBaseWidth = 0.0;
		self.arrowHeight = 0.0;
	}
}


/*!
Returns the region that the window should occupy globally if the
embedded view has the given local frame.

(1.1)
*/
- (NSRect)
frameRectForViewRect:(NSRect)	aRect
{
	return [self.class frameRectForViewRect:aRect viewMargin:self.viewMargin arrowHeight:self.arrowHeight
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
	return [self.class viewRectForFrameRect:aRect viewMargin:self.viewMargin arrowHeight:self.arrowHeight
											side:self->windowPropertyFlags];
}


#pragma mark Window Location


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
																		arrowInset:self.arrowInset
																		at:((nil != self->popoverParentWindow)
																			? [self.class convertToScreenFromWindow:self->popoverParentWindow
																													point:aPoint]
																			: aPoint)
																		side:aSide];
		
		
		windowFrame.origin.x = idealOrigin.x;
		windowFrame.origin.y = idealOrigin.y;
		//NSDisableScreenUpdates();
		[self setFrame:windowFrame display:NO];
		[self fixViewFrame];
		[self updateBackground];
		//NSEnableScreenUpdates();
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
																		andMargin:self.viewMargin
																		arrowHeight:self.arrowHeight
																		arrowInset:self.arrowInset
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
						(self->viewMargin * 2.0f)) - self->cornerRadius;
	
	
	if (self.hasRoundCornerBesideArrow)
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
	float	maxRadius = ((MIN(self->viewFrame.size.width, self->viewFrame.size.height) + (self->viewMargin * 2.0f)) -
							self->arrowBaseWidth) / 2.0f;
	
	
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
	self.arrowBaseWidth = self.arrowBaseWidth;
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
	}
}


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to key values by updating dependent
states such as frame rectangles or the display.

(1.3)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
#pragma unused(anObject, aContext)
	//if (NO == self.disableObservers)
	{
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			NSRect		newFrame = self.frame;
			// IMPORTANT: these will only be defined if the call to add
			// the observer includes the appropriate options
			id			oldValue = [aChangeDictionary objectForKey:NSKeyValueChangeOldKey];
			id			newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
			
			
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(arrowHeight)))
			{
				// fix layout and update background
				[self redisplay];
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(borderOuterColor)))
			{
				[self updateBackground];
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(borderPrimaryColor)))
			{
				[self updateBackground];
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(hasArrow)))
			{
				[self updateBackground];
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(hasRoundCornerBesideArrow)))
			{
				// fix layout and update background
				[self redisplay];
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(popoverBackgroundColor)))
			{
				[self updateBackground];
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(viewMargin)))
			{
				float const		kOldFloat = [oldValue floatValue];
				float const		kNewFloat = [newValue floatValue];
				
				
				// synchronize corner radius (and arrow base width)
				self.cornerRadius = self->cornerRadius;
				
				// adjust frame to make space for the difference in margin
				newFrame.size.width += (kNewFloat - kOldFloat);
				newFrame.size.height += (kNewFloat - kOldFloat);
				[self setFrame:newFrame display:NO];
				[self fixViewFrame];
			}
		}
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
	self.popoverBackgroundColor = aValue;
}


#pragma mark NSWindowNotifications


/*!
Responds to window activation by restoring the frame colors.

(1.4)
*/
- (void)
windowDidBecomeKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.borderOuterDisplayColor = self.borderOuterColor;
	self.borderPrimaryDisplayColor = self.borderPrimaryColor;
	[self updateBackground];
}


/*!
Responds to window deactivation by graying the frame colors.

(1.4)
*/
- (void)
windowDidResignKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.borderOuterDisplayColor = [NSColor colorWithCalibratedRed:0.95f green:0.95f blue:0.95f alpha:1.0f];
	self.borderPrimaryDisplayColor = [NSColor colorWithCalibratedRed:0.65f green:0.65f blue:0.65f alpha:1.0f];
	[self updateBackground];
}


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
	if (self.hasArrow)
	{
		float const		kScaleFactor = 1.0;
		float const		kScaledArrowWidth = self->arrowBaseWidth * kScaleFactor;
		float const		kHalfScaledArrowWidth = kScaledArrowWidth / 2.0f;
		float const		kScaledArrowHeight = self.arrowHeight * kScaleFactor;
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
Presets a wide variety of window properties to produce a
standard appearance.  This only affects appearance and
not behavior but the code that runs a popover window
should ensure that the window behavior is consistent.

(1.4)
*/
- (void)
applyStyle:(Popover_WindowStyle)	aStyle
{
	switch (aStyle)
	{
	case kPopover_WindowStyleNormal:
		// an actual popover window with an arrow, that is
		// typically transient (any click outside dismisses it);
		// caller can later decide to remove the arrow
		self.backgroundColor = [NSColor colorWithCalibratedRed:0.9f green:0.9f blue:0.9f alpha:0.95f];
		self.borderOuterColor = [NSColor colorWithCalibratedRed:0.25f green:0.25f blue:0.25f alpha:0.7f];
		self.borderPrimaryColor = [NSColor colorWithCalibratedRed:1.0f green:1.0f blue:1.0f alpha:0.8f];
		self.viewMargin = 3.0f;
		self.borderWidth = 2.2f;
		self.cornerRadius = 4.0f;
		[self setStandardArrowProperties:YES];
		break;
	
	case kPopover_WindowStyleAlertAppModal:
		// not really a popover but an application-modal dialog box
		// for displaying an alert message
		[self applyStyle:kPopover_WindowStyleDialogAppModal];
		if ([self.class isGraphiteTheme])
		{
			self.borderOuterColor = [NSColor colorWithCalibratedRed:0.9f green:0.9f blue:0.9f alpha:1.0f];
			self.borderPrimaryColor = [NSColor colorWithCalibratedRed:0.7f green:0.7f blue:0.7f alpha:1.0f];
		}
		else
		{
			self.borderOuterColor = [NSColor colorWithCalibratedRed:1.0f green:0.9f blue:0.9f alpha:1.0f];
			self.borderPrimaryColor = [NSColor colorWithCalibratedRed:0.75f green:0.7f blue:0.7f alpha:1.0f];
		}
		break;
	
	case kPopover_WindowStyleAlertSheet:
		// not really a popover but a window-modal dialog for
		// displaying an alert message
		[self applyStyle:kPopover_WindowStyleAlertAppModal];
		self.viewMargin = 6.0f;
		self.borderWidth = 6.0f;
		self.cornerRadius = 3.0f;
		break;
	
	case kPopover_WindowStyleDialogAppModal:
		// not really a popover but an application-modal dialog box
		[self applyStyle:kPopover_WindowStyleNormal];
		self.borderOuterColor = [NSColor colorWithCalibratedRed:0.85f green:0.85f blue:0.85f alpha:1.0f];
		self.borderPrimaryColor = [NSColor colorWithCalibratedRed:0.55f green:0.55f blue:0.55f alpha:1.0f];
		self.viewMargin = 9.0f;
		self.borderWidth = 9.0f;
		self.cornerRadius = 9.0f;
		[self setStandardArrowProperties:NO];
		break;
	
	case kPopover_WindowStyleDialogSheet:
		// not really a popover but a window-modal dialog
		[self applyStyle:kPopover_WindowStyleDialogAppModal];
		self.viewMargin = 6.0f;
		self.borderWidth = 6.0f;
		self.cornerRadius = 3.0f;
		break;
	
	case kPopover_WindowStyleHelp:
		// a floating window that typically contains only help text
		if ([self.class isGraphiteTheme])
		{
			self.backgroundColor = [NSColor colorWithCalibratedRed:0.5f green:0.5f blue:0.5f alpha:0.93f];
		}
		else
		{
			self.backgroundColor = [NSColor colorWithCalibratedRed:0.0f green:0.25f blue:0.5f alpha:0.93f];
		}
		self.borderOuterColor = [NSColor whiteColor];
		self.borderPrimaryColor = [NSColor blackColor];
		self.viewMargin = 5.0f;
		self.borderWidth = 5.0f;
		self.cornerRadius = 5.0f;
		[self setStandardArrowProperties:YES];
		break;
	
	default:
		// ???
		assert(false && "unsupported style type");
		break;
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
	float const		kRadius = (self.hasRoundCornerBesideArrow)
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
	return (aCornerRadius + (anArrowBaseWidth / 2.0f));
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
	NSImage*	patternImage = [[NSImage alloc] initWithSize:self.frame.size];
	
	
	[patternImage lockFocus];
	[NSGraphicsContext saveGraphicsState];
	{
		BOOL const		kDebug = NO;
		NSBezierPath*	sourcePath = [self backgroundPath];
		NSRect			localRect = self.frame;
		
		
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
		
		[self.popoverBackgroundColor set];
		[sourcePath fill];
		if (self.borderWidth > 0)
		{
			// double width since drawing is clipped inside the path
			[sourcePath setLineWidth:(self.borderWidth * 2.0f)];
			[self.borderPrimaryDisplayColor set];
			[sourcePath stroke];
			[sourcePath setLineWidth:self.borderWidth - 1.0f]; // arbitrary shift
			[self.borderOuterDisplayColor set];
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
	
	[patternImage release];
	
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
	float const		kHalfScaledArrowWidth = kScaledArrowWidth / 2.0f;
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
			((self.hasRoundCornerBesideArrow) ||
				((kPopover_PositionBottomRight != self->windowPropertyFlags) &&
					(kPopover_PositionRightBottom != self->windowPropertyFlags))))
		{
			startingPoint.x += kScaledCornerRadius;
		}
		
		endOfLine = NSMakePoint(kMaxX, kMaxY);
		if ((isRoundedCorner) &&
			((self.hasRoundCornerBesideArrow) ||
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
		((self.hasRoundCornerBesideArrow) ||
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
		((self.hasRoundCornerBesideArrow) ||
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
		((self.hasRoundCornerBesideArrow) ||
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
										? [self.class convertToScreenFromWindow:aWindow point:aPoint]
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
Given a point in window coordinates (e.g. by passing "nil"
to an NSView conversion method), returns the screen
coordinates.  See also "convertToWindow:fromScreenPoint:".

Currently this is implemented by adding the frame origin.

(1.2)
*/
+ (NSPoint)
convertToScreenFromWindow:(NSWindow*)	aWindow
point:(NSPoint)							aPoint
{
	return NSMakePoint(aPoint.x + NSMinX(aWindow.frame), aPoint.y + NSMinY(aWindow.frame));
}


/*!
Given a point in screen coordinates, returns the window
coordinates.  See also "convertToScreenFromWindow:point:".

Currently this is implemented by subtracting the frame origin.

(1.2)
*/
+ (NSPoint)
convertToWindow:(NSWindow*)		aWindow
fromScreenPoint:(NSPoint)		aPoint
{
	return NSMakePoint(aPoint.x - NSMinX(aWindow.frame), aPoint.y - NSMinY(aWindow.frame));
}


/*!
The per-window version of "convertToScreenFromWindow:point:".

(1.2)
*/
- (NSPoint)
convertToScreenFromWindowPoint:(NSPoint)	aPoint
{
	return [self.class convertToScreenFromWindow:self point:aPoint];
}


/*!
The per-window version of "convertToWindow:fromScreenPoint:".

(1.2)
*/
- (NSPoint)
convertToWindowFromScreenPoint:(NSPoint)	aPoint
{
	return [self.class convertToWindow:self fromScreenPoint:aPoint];
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
	self->viewFrame = [self viewRectForFrameRect:self.frame];
	self->viewFrame.origin = [self convertToWindowFromScreenPoint:self->viewFrame.origin];
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
	return [self.class idealFrameOriginForSize:self.frame.size arrowInset:self.arrowInset
												at:((self->popoverParentWindow)
													? [self.class convertToScreenFromWindow:self->popoverParentWindow
																							point:aPoint]
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
Returns YES only if the current system appearance is
using the graphite theme (grayscale).  This is used
to determine if frames should use color.

(2016.06)
*/
+ (BOOL)
isGraphiteTheme
{
	return (NSGraphiteControlTint == [NSColor currentControlTint]);
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
		//NSDisableScreenUpdates();
		[self fixViewFrame];
		[self updateBackground];
		//NSEnableScreenUpdates();
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
	//NSDisableScreenUpdates();
	// call superclass to avoid overridden version from this class
	@autoreleasepool
	{
		[super setBackgroundColor:[self backgroundColorPatternImage]];
	}
	
	if ([self isVisible])
	{
		[self display];
		[self invalidateShadow];
	}
	//NSEnableScreenUpdates();
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
Returns the window placement portion of this popover’s properties.

(1.1)
*/
- (Popover_Properties)
windowPlacement
{
	return [self.class windowPlacement:self->windowPropertyFlags];
}


@end // Popover_Window (Popover_WindowInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
