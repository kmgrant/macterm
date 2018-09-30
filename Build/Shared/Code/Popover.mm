/*!	\file Popover.mm
	\brief Implements a popover-style window.
*/
/*###############################################################

	Popover Window 1.4 (based on MAAttachedWindow)
	MAAttachedWindow © 2007 by Magic Aubergine
	Popover Window © 2011-2018 by Kevin Grant
	
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

// standard-C++ includes
#import <cmath>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>



#pragma mark Constants
namespace {

CGFloat const		kMy_ResizeClickWidth = 7.0; // arbitrary; space from edge that still registers as a resize request

} // anonymous namespace


#pragma mark Internal Method Prototypes
namespace {

NSColor*	colorRGBA	(CGFloat, CGFloat, CGFloat, CGFloat = 1.0);

} // anonymous namespace


/*!
The private class interface.
*/
@interface Popover_Window (Popover_WindowInternal) //{

// class methods: general
	+ (CGFloat)
	arrowInsetWithCornerRadius:(CGFloat)_
	baseWidth:(CGFloat)_;
	+ (Popover_Properties)
	arrowPlacement:(Popover_Properties)_;
	+ (Popover_Properties)
	bestSideForViewOfSize:(NSSize)_
	cornerMargins:(NSSize)_
	arrowHeight:(CGFloat)_
	arrowInset:(CGFloat)_
	at:(NSPoint)_
	onParentWindow:(NSWindow*)_
	preferredSide:(Popover_Properties)_;
	+ (NSPoint)
	idealFrameOriginForSize:(NSSize)_
	arrowInset:(CGFloat)_
	at:(NSPoint)_
	side:(Popover_Properties)_;
	+ (BOOL)
	isGraphiteTheme;
	+ (Popover_Properties)
	windowPlacement:(Popover_Properties)_;

// class methods: frame conversion
	+ (NSRect)
	contentRectForFrameRect:(NSRect)_
	arrowHeight:(CGFloat)_
	side:(Popover_Properties)_;
	+ (NSPoint)
	convertToScreenFromWindow:(NSWindow*)_
	point:(NSPoint)_;
	+ (NSPoint)
	convertToWindow:(NSWindow*)_
	fromScreenPoint:(NSPoint)_;
	+ (NSRect)
	embeddedViewRectForContentSize:(NSSize)_
	cornerMargins:(NSSize)_;
	+ (NSRect)
	frameRectForContentRect:(NSRect)_
	arrowHeight:(CGFloat)_
	side:(Popover_Properties)_;
	+ (NSSize)
	frameSizeForEmbeddedViewSize:(NSSize)_
	cornerMargins:(NSSize)_
	arrowHeight:(CGFloat)_;

// new methods
	- (void)
	appendArrowToPath:(NSBezierPath*)_;
	- (NSPoint)
	convertToScreenFromWindowPoint:(NSPoint)_;
	- (NSPoint)
	convertToWindowFromScreenPoint:(NSPoint)_;
	- (void)
	fixViewFrame;
	- (NSPoint)
	idealFrameOriginForPoint:(NSPoint)_;
	- (NSImage*)
	newContentWindowImage;
	- (NSImage*)
	newFullWindowImage;
	- (void)
	redisplay;
	- (void)
	removeTrackingAreas;
	- (void)
	resetTrackingAreas;
	- (void)
	updateBackground;

// accessors: colors
	- (NSColor*)
	colorAlertFrameOuter;
	- (NSColor*)
	colorAlertFramePrimary;
	- (NSColor*)
	colorDialogFrameOuter;
	- (NSColor*)
	colorDialogFramePrimary;
	- (NSColor*)
	colorPopoverBackground;
	- (NSColor*)
	colorPopoverFrameOuter;
	- (NSColor*)
	colorPopoverFramePrimary;
	- (NSColor*)
	colorWithName:(NSString*)_
	grayName:(NSString*)_
	defaultColor:(NSColor*)_
	defaultGray:(NSColor*)_;

// accessors: other
	- (CGFloat)
	arrowInset;
	- (Popover_Properties)
	arrowPlacement;
	- (NSColor*)
	backgroundFrameImageAsColor;
	- (NSBezierPath*)
	backgroundPath;
	- (NSRect)
	contentEdgeFrameBottom;
	- (NSRect)
	contentEdgeFrameLeft;
	- (NSRect)
	contentEdgeFrameRight;
	- (NSRect)
	contentEdgeFrameTop;
	- (NSRect)
	contentFrame;
	- (NSView*)
	contentViewAsNSView;
	- (NSSize)
	cornerMargins;
	- (Popover_Properties)
	windowPlacement;

@end //}

@interface Popover_Window () //{

// accessors
	@property (assign) BOOL
	allowHorizontalResize;
	@property (assign) BOOL
	allowVerticalResize;
	@property (copy) NSImage*
	animationContentImage;
	@property (copy) NSImage*
	animationFullImage;
	@property (copy) NSColor*
	borderOuterDisplayColor;
	@property (copy) NSColor*
	borderPrimaryDisplayColor;
	@property (assign) BOOL
	isBeingResizedByUser;
	@property (assign) Popover_WindowStyle
	lastAppliedWindowStyle;
	@property (assign) BOOL
	layoutInProgress;
	@property (strong) NSMutableArray*
	registeredObservers;
	@property (strong) NSTrackingArea*
	trackBottomEdge;
	@property (strong) NSTrackingArea*
	trackLeftEdge;
	@property (strong) NSTrackingArea*
	trackMainFrame;
	@property (strong) NSTrackingArea*
	trackRightEdge;
	@property (strong) NSTrackingArea*
	trackTopEdge;
	@property (assign) NSPoint
	userResizeClickScreenPoint;
	@property (assign) CGFloat
	userResizeOriginalCenterX;
	@property (assign) BOOL
	userResizeTop;
	@property (assign) BOOL
	userResizeLeft;
	@property (assign) BOOL
	userResizeRight;
	@property (assign) BOOL
	userResizeBottom;
	@property (assign) Popover_Properties
	windowPropertyFlags;

@end //}


#pragma mark Internal Methods

#pragma mark -
@implementation Popover_Window //{


#pragma mark Properties


/*!
Set automatically when "resizeDelegate" changes, to the
value provided by the delegate.

In addition, resizing is constrained if the window reaches
the minimum or maximum size set on the NSWindow.
*/
@synthesize allowHorizontalResize = _allowHorizontalResize;

/*!
Set automatically when "resizeDelegate" changes, to the
value provided by the delegate.

In addition, resizing is constrained if the window reaches
the minimum or maximum size set on the NSWindow.
*/
@synthesize allowVerticalResize = _allowVerticalResize;

/*!
An image of cached window content only; used for animations.
*/
@synthesize animationContentImage = _animationContentImage;

/*!
An image of the window frame and content; used for animations.
*/
@synthesize animationFullImage = _animationFullImage;

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
When the window position puts the arrow near a corner of
the frame, this specifies how close to the corner the
arrow is.  If a rounded corner appears then the arrow is
off to the side; otherwise the arrow is right in the
corner.
*/
@synthesize hasRoundCornerBesideArrow = _hasRoundCornerBesideArrow;

/*!
Set internally, only during resize; see "mouseDown:".
*/
@synthesize isBeingResizedByUser = _isBeingResizedByUser;

/*!
The window style given at initialization time, or the last
style used with "applyWindowStyle:".
*/
@synthesize lastAppliedWindowStyle = _lastAppliedWindowStyle;

/*!
Set internally, only during layout; see "redisplay".
*/
@synthesize layoutInProgress = _layoutInProgress;

/*!
The popover background color is used to construct an image
that the NSWindow superclass uses for rendering.

Use this property instead of the NSWindow "backgroundColor"
property because the normal background color is overridden
to contain the entire rendering of the popover window frame
(as a pattern image).
*/
@synthesize popoverBackgroundColor = _popoverBackgroundColor;

/*!
Stores information on key-value observers.
*/
@synthesize registeredObservers = _registeredObservers;

/*!
If set, this object can be queried to guide resize behavior
(such as to decide that only one axis allows resizing).
*/
@synthesize resizeDelegate = _resizeDelegate;

/*!
Top-edge tracking; installed only for resizable windows.
*/
@synthesize trackTopEdge = _trackTopEdge;

/*!
Left-edge tracking; installed only for resizable windows.
*/
@synthesize trackLeftEdge = _trackLeftEdge;

/*!
Interior frame tracking; installed for all windows to make
sure the cursor is reset to an arrow.
*/
@synthesize trackMainFrame = _trackMainFrame;

/*!
Right-edge tracking; installed only for resizable windows.
*/
@synthesize trackRightEdge = _trackRightEdge;

/*!
Bottom-edge tracking; installed only for resizable windows.
*/
@synthesize trackBottomEdge = _trackBottomEdge;

/*!
Set internally, only during resize; see "mouseDown:".
*/
@synthesize userResizeClickScreenPoint = _userResizeClickScreenPoint;

/*!
Set internally, only during resize; see "mouseDown:".
*/
@synthesize userResizeOriginalCenterX = _userResizeOriginalCenterX;

/*!
Set internally, only during resize; see "mouseDown:".
*/
@synthesize userResizeTop = _userResizeTop;
@synthesize userResizeLeft = _userResizeLeft;
@synthesize userResizeRight = _userResizeRight;
@synthesize userResizeBottom = _userResizeBottom;

/*!
The arrangement of the window content relative to any arrow.
This is the basis for the "arrowPlacement" and "windowPlacement"
convenience methods.
*/
@synthesize windowPropertyFlags = _windowPropertyFlags;


#pragma mark Initializers


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithContentRect:(NSRect)	aRect
#if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_6
styleMask:(NSUInteger)			aStyleMask
#else
styleMask:(NSWindowStyleMask)	aStyleMask
#endif
backing:(NSBackingStoreType)	aBackingStoreType
defer:(BOOL)					aDeferFlag
{
#pragma unused(aRect, aStyleMask, aBackingStoreType, aDeferFlag)
	assert(false && "invalid way to initialize derived class");
	return [self initWithView:nil windowStyle:kPopover_WindowStyleNormal
								arrowStyle:kPopover_ArrowStyleNone
								attachedToPoint:NSZeroPoint
								inWindow:nil vibrancy:NO];
}// initWithContentRect:styleMask:backing:defer:


/*!
Convenience initializer; assumes a value for vibrancy.

(2017.07)
*/
- (instancetype)
initWithView:(NSView*)				aView
windowStyle:(Popover_WindowStyle)	aWindowStyle
arrowStyle:(Popover_ArrowStyle)		anArrowStyle
attachedToPoint:(NSPoint)			aPoint
inWindow:(NSWindow*)				aWindow
{
	return [self initWithView:aView windowStyle:aWindowStyle
								arrowStyle:anArrowStyle
								attachedToPoint:aPoint
								inWindow:aWindow vibrancy:YES];
}// initWithView:windowStyle:arrowStyle:attachedToPoint:inWindow:


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

(2017.07)
*/
- (instancetype)
initWithView:(NSView*)					aView
windowStyle:(Popover_WindowStyle)		aWindowStyle
arrowStyle:(Popover_ArrowStyle)			anArrowStyle
attachedToPoint:(NSPoint)				aPoint
inWindow:(NSWindow*)					aWindow
vibrancy:(BOOL)							aVisualEffectFlag
{
	if (nil == aView)
	{
		self = nil;
	}
	else
	{
		BOOL const		kDefaultCornerIsRounded = YES;
		CGFloat const	kDefaultArrowBaseWidth = 30.0; // see also "applyArrowStyle:"
		CGFloat const	kDefaultArrowHeight = 0.0; // see also "applyArrowStyle:"
		CGFloat const	kDefaultCornerRadius = 8.0;
		CGFloat const	kDefaultCornerRadiusForArrowCorner = (kDefaultCornerIsRounded)
																? kDefaultCornerRadius
																: 0;
		CGFloat const	kDefaultMargin = 2.0;
		CGFloat const	kDefaultArrowInset = [self.class arrowInsetWithCornerRadius:kDefaultCornerRadiusForArrowCorner
																					baseWidth:kDefaultArrowBaseWidth];
		
		
		// Create initial contentRect for the attached window (can reset frame at any time).
		// Note that the content rectangle given to NSWindow is just the frame rectangle;
		// the illusion of a window margin is created during rendering.
		Popover_Properties	side = [self.class bestSideForViewOfSize:[aView frame].size
																		cornerMargins:NSMakeSize(kDefaultMargin, kDefaultMargin)
																		arrowHeight:kDefaultArrowHeight
																		arrowInset:kDefaultArrowInset
																		at:aPoint
																		onParentWindow:aWindow
																		preferredSide:kPopover_PositionBottom];
		NSSize		frameSize = [self.class frameSizeForEmbeddedViewSize:[aView frame].size
																			cornerMargins:NSMakeSize(kDefaultMargin, kDefaultMargin)
																			arrowHeight:kDefaultArrowHeight];
		NSRect		contentRectOnScreen = NSZeroRect;
		
		
		contentRectOnScreen.size = frameSize;
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
			[super setBackgroundColor:[NSColor clearColor]];
			
			[self setMovableByWindowBackground:NO];
			[self setExcludedFromWindowsMenu:YES];
			[self setPreventsApplicationTerminationWhenModal:NO];
			[self setAlphaValue:1.0];
			[self setOpaque:NO];
			[self setHasShadow:YES];
			
			self->_registeredObservers = [[NSMutableArray alloc] init];
			self->popoverParentWindow = aWindow;
			self->embeddedView = aView;
			
			self->_animationContentImage = nil;
			self->_animationFullImage = nil;
			self->_trackTopEdge = nil;
			self->_trackLeftEdge = nil;
			self->_trackMainFrame = nil;
			self->_trackRightEdge = nil;
			self->_trackBottomEdge = nil;
			self->_borderOuterColor = [[NSColor grayColor] copy];
			self->_borderOuterDisplayColor = [self->_borderOuterColor copy];
			self->_borderPrimaryColor = [[NSColor whiteColor] copy];
			self->_borderPrimaryDisplayColor = [self->_borderPrimaryColor copy];
			self->_popoverBackgroundColor = [[NSColor colorWithCalibratedWhite:0.1f alpha:0.75f] copy];
			self->_resizeDelegate = nil;
			self->_userResizeClickScreenPoint = NSZeroPoint;
			self->_userResizeOriginalCenterX = 0.0;
			self->_arrowBaseWidth = kDefaultArrowBaseWidth;
			self->_arrowHeight = kDefaultArrowHeight;
			self->_borderWidth = (kDefaultMargin - 1.0f);
			self->_cornerRadius = kDefaultCornerRadius;
			self->_viewMargin = kDefaultMargin;
			self->_layoutInProgress = NO;
			self->_allowHorizontalResize = NO;
			self->_allowVerticalResize = NO;
			self->_hasRoundCornerBesideArrow = kDefaultCornerIsRounded;
			self->_isBeingResizedByUser = NO;
			self->_userResizeTop = NO;
			self->_userResizeLeft = NO;
			self->_userResizeRight = NO;
			self->_userResizeBottom = NO;
			self->_windowPropertyFlags = (kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow);
			
			// add view as subview ("fixViewFrame" will set the layout)
			[[self contentViewAsNSView] addSubview:self->embeddedView];
			
			// ensure that the display is updated after certain changes
			[self.registeredObservers addObject:[[self newObserverFromSelector:@selector(borderOuterColor)] autorelease]];
			[self.registeredObservers addObject:[[self newObserverFromSelector:@selector(borderPrimaryColor)] autorelease]];
			[self.registeredObservers addObject:[[self newObserverFromSelector:@selector(hasRoundCornerBesideArrow)] autorelease]];
			[self.registeredObservers addObject:[[self newObserverFromSelector:@selector(popoverBackgroundColor)] autorelease]];
			[self.registeredObservers addObject:[[self newObserverFromSelector:@selector(resizeDelegate)] autorelease]];
			[self.registeredObservers addObject:[[self newObserverFromSelector:@selector(windowPropertyFlags)] autorelease]];
			[self.registeredObservers addObject:[[self newObserverFromSelector:@selector(effectiveAppearance) ofObject:NSApp options:0] autorelease]];
			
			// ensure that frame-dependent property changes will cause the
			// frame to be synchronized with the new values (note that the
			// "setBorderWidth:" method already requires the border to fit
			// within the margin so that value is not monitored here)
			[self.registeredObservers addObject:[self newObserverFromSelector:@selector(arrowHeight)
																				ofObject:self
																				options:(NSKeyValueObservingOptionNew |
																							NSKeyValueObservingOptionOld)]];
			[self.registeredObservers addObject:[self newObserverFromSelector:@selector(viewMargin)
																				ofObject:self
																				options:(NSKeyValueObservingOptionNew |
																							NSKeyValueObservingOptionOld)]];
			
			// subscribe to notifications
			[self whenObject:self postsNote:NSWindowDidBecomeKeyNotification
								performSelector:@selector(windowDidBecomeKey:)];
			[self whenObject:self postsNote:NSWindowDidResignKeyNotification
								performSelector:@selector(windowDidResignKey:)];
			[self whenObject:self postsNote:NSWindowDidResizeNotification
								performSelector:@selector(windowDidResize:)];
			
			// use the style parameters to modify default values
			[self applyWindowStyle:aWindowStyle];
			[self applyArrowStyle:anArrowStyle];
			
			// initialize frame and background
			[self fixViewFrame];
			[self updateBackground];
			
			// construct the content image before the NSVisualEffectView
			// is added, otherwise it may not render the content at all
			// (therefore this image is not updated when content changes,
			// which is fine for opening animations)
			self->_animationContentImage = [self newContentWindowImage];
			
			// defer full image creation until needed (that way, cached
			// content can be combined with an accurate frame layout)
			self->_animationFullImage = nil;
			
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
					NSView*					parentView = [[self->embeddedView subviews] objectAtIndex:0];
					NSRect					visualFrame = NSMakeRect(0, 0, NSWidth(parentView.frame),
																		NSHeight(parentView.frame));
					NSVisualEffectView*		visualEffectObject = [[NSVisualEffectView alloc]
																	initWithFrame:NSRectToCGRect(visualFrame)];
					
					
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
		}
	}
	return self;
}// initWithView:windowStyle:arrowStyle:attachedToPoint:inWindow:vibrancy:


/*!
Destructor.

(1.0)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
	
	[_animationContentImage release];
	[_animationFullImage release];
	[_borderOuterColor release];
	[_borderOuterDisplayColor release];
	[_borderPrimaryColor release];
	[_borderPrimaryDisplayColor release];
	[_popoverBackgroundColor release];
	[_trackTopEdge release];
	[_trackLeftEdge release];
	[_trackMainFrame release];
	[_trackRightEdge release];
	[_trackBottomEdge release];
	
	// remove observers registered by initializer
	[self removeObserversSpecifiedInArray:self.registeredObservers];
	[_registeredObservers release];
	
	[super dealloc];
}// dealloc


#pragma mark New Methods: Utilities


/*!
Sets the "hasArrow", "hasRoundCornerBesideArrow",
"arrowBaseWidth" and "arrowHeight" properties
simultaneously to a standard configuration.

(2017.06)
*/
- (void)
applyArrowStyle:(Popover_ArrowStyle)	anArrowStyle
{
	switch (anArrowStyle)
	{
	case kPopover_ArrowStyleNone:
		self.hasRoundCornerBesideArrow = YES;
		self.arrowBaseWidth = 0.0;
		self.arrowHeight = 0.0;
		break;
	
	case kPopover_ArrowStyleDefaultMiniSize:
		self.hasRoundCornerBesideArrow = YES;
		self.arrowBaseWidth = 14.0;
		self.arrowHeight = 5.0;
		break;
	
	case kPopover_ArrowStyleDefaultSmallSize:
		self.hasRoundCornerBesideArrow = YES;
		self.arrowBaseWidth = 21.0;
		self.arrowHeight = 9.0;
		break;
	
	case kPopover_ArrowStyleDefaultRegularSize:
		self.hasRoundCornerBesideArrow = YES;
		self.arrowBaseWidth = 28.0;
		self.arrowHeight = 13.0;
		break;
	
	default:
		// ???
		assert(false && "unsupported arrow style type");
		break;
	}
}// applyArrowStyle:


/*!
Presets a wide variety of window properties to produce a
standard appearance.  This only affects appearance and
not behavior but the code that runs a popover window
should ensure that the window behavior is consistent.

Generally this is only set at initialization time.  If
you dynamically change a window’s appearance, you should
probably do so while the window is not visible to avoid
any jarring effects (that is, you can reuse a window as
long as it is not displayed).

(2017.06)
*/
- (void)
applyWindowStyle:(Popover_WindowStyle)		aWindowStyle
{
	switch (aWindowStyle)
	{
	case kPopover_WindowStyleNormal:
		// an actual popover window with an arrow, that is
		// typically transient (any click outside dismisses it);
		// caller can later decide to remove the arrow
		self.backgroundColor = [self colorPopoverBackground];
		self.borderOuterColor = [self colorPopoverFrameOuter];
		self.borderPrimaryColor = [self colorPopoverFramePrimary];
		self.viewMargin = 3.0f;
		self.borderWidth = 2.2f;
		self.cornerRadius = 4.0f;
		break;
	
	case kPopover_WindowStyleAlertAppModal:
		// not really a popover but an application-modal dialog box
		// for displaying an alert message
		[self applyWindowStyle:kPopover_WindowStyleDialogAppModal];
		self.borderOuterColor = [self colorAlertFrameOuter];
		self.borderPrimaryColor = [self colorAlertFramePrimary];
		break;
	
	case kPopover_WindowStyleAlertSheet:
		// not really a popover but a window-modal dialog for
		// displaying an alert message
		[self applyWindowStyle:kPopover_WindowStyleAlertAppModal];
		self.viewMargin = 5.0f;
		self.borderWidth = 6.0f;
		self.cornerRadius = 3.0f;
		break;
	
	case kPopover_WindowStyleDialogAppModal:
		// not really a popover but an application-modal dialog box
		[self applyWindowStyle:kPopover_WindowStyleNormal];
		self.borderOuterColor = [self colorDialogFrameOuter];
		self.borderPrimaryColor = [self colorDialogFramePrimary];
		self.viewMargin = 10.0f;
		self.borderWidth = 9.0f;
		self.cornerRadius = 9.0f;
		break;
	
	case kPopover_WindowStyleDialogSheet:
		// not really a popover but a window-modal dialog
		[self applyWindowStyle:kPopover_WindowStyleDialogAppModal];
		self.viewMargin = 7.0f;
		self.borderWidth = 6.0f;
		self.cornerRadius = 3.0f;
		break;
	
	case kPopover_WindowStyleHelp:
		// a floating window that typically contains only help text
		if (@available(macOS 10.14, *))
		{
			self.backgroundColor = [NSColor controlAccentColor];
		}
		else
		{
			if ([self.class isGraphiteTheme])
			{
				self.backgroundColor = colorRGBA(0.45f, 0.45f, 0.45f, 0.96f);
			}
			else
			{
				self.backgroundColor = colorRGBA(0.0f, 0.20f, 0.45f, 0.96f);
			}
		}
		self.borderOuterColor = [NSColor whiteColor];
		self.borderPrimaryColor = [NSColor blackColor];
		self.viewMargin = 6.0f;
		self.borderWidth = 5.0f;
		self.cornerRadius = 5.0f;
		break;
	
	default:
		// ???
		assert(false && "unsupported window style type");
		break;
	}
	
	self.lastAppliedWindowStyle = aWindowStyle;
}// applyWindowStyle:


/*!
Returns the region that the window should occupy globally if the
embedded view has the given size.

Note that the embedded view may be smaller than the "contentView".
The "contentView" is effectively internal, used to create the
necessary illusions of a window frame (such as by displaying the
appropriate resize cursors).  From the user’s point of view, the
main content of the window occupies the specified view size only.

(2017.06)
*/
- (NSRect)
frameRectForViewSize:(NSSize)	aSize
{
	NSSize const	frameSize = [self.class frameSizeForEmbeddedViewSize:aSize
																			cornerMargins:[self cornerMargins]
																			arrowHeight:self.arrowHeight];
	
	
	return NSMakeRect(self.frame.origin.x, self.frame.origin.y,
						frameSize.width, frameSize.height);
}// frameRectForViewSize:


#pragma mark New Methods: Window Location


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
	NSRect						windowFrame = self.frame; // ignore original origin, use only size
	Popover_Properties const	kOldWindowPlacement = [self windowPlacement];
	Popover_Properties const	kNewWindowPlacement = [self.class windowPlacement:aSide];
	
	
	// since the frame includes the arrow space and a window may relocate
	// its arrow to a different side, the “minimum” and “maximum” frame
	// sizes adjust when the arrow swaps between horizontal and vertical
	if ((kPopover_PropertyPlaceFrameAboveArrow == kOldWindowPlacement) ||
		(kPopover_PropertyPlaceFrameBelowArrow == kOldWindowPlacement))
	{
		// detect change from vertical to horizontal arrow
		if ((kPopover_PropertyPlaceFrameLeftOfArrow == kNewWindowPlacement) ||
			(kPopover_PropertyPlaceFrameRightOfArrow == kNewWindowPlacement))
		{
			if (self.minSize.width < 10000/* arbitrary; do not exceed max. */)
			{
				self.minSize = NSMakeSize(self.minSize.width + self.arrowHeight,
											self.minSize.height - self.arrowHeight);
			}
			
			if (self.maxSize.width < 10000/* arbitrary; do not exceed max. */)
			{
				self.maxSize = NSMakeSize(self.maxSize.width + self.arrowHeight,
											self.maxSize.height - self.arrowHeight);
			}
		}
	}
	else
	{
		// detect change from horizontal to vertical arrow
		if ((kPopover_PropertyPlaceFrameAboveArrow == kNewWindowPlacement) ||
			(kPopover_PropertyPlaceFrameBelowArrow == kNewWindowPlacement))
		{
			if (self.minSize.height < 10000/* arbitrary; do not exceed max. */)
			{
				self.minSize = NSMakeSize(self.minSize.width - self.arrowHeight,
											self.minSize.height + self.arrowHeight);
			}
			
			if (self.maxSize.height < 10000/* arbitrary; do not exceed max. */)
			{
				self.maxSize = NSMakeSize(self.maxSize.width - self.arrowHeight,
											self.maxSize.height + self.arrowHeight);
			}
		}
	}
	
	// set these properties first so that "frameRectForViewSize:" sees them
	self.windowPropertyFlags = aSide;
	
	// constrain as needed
	if (windowFrame.size.width < self.minSize.width)
	{
		windowFrame.size.width = self.minSize.width;
	}
	if (windowFrame.size.height < self.minSize.height)
	{
		windowFrame.size.height = self.minSize.height;
	}
	if (windowFrame.size.width > self.maxSize.width)
	{
		windowFrame.size.width = self.maxSize.width;
	}
	if (windowFrame.size.height > self.maxSize.height)
	{
		windowFrame.size.height = self.maxSize.height;
	}
	
	// update the window frame
	{
		NSPoint		idealOrigin = [self.class idealFrameOriginForSize:windowFrame.size
																		arrowInset:self.arrowInset
																		at:((nil != self->popoverParentWindow)
																			? [self.class convertToScreenFromWindow:self->popoverParentWindow
																													point:aPoint]
																			: aPoint)
																		side:aSide];
		
		
		windowFrame.origin.x = idealOrigin.x;
		windowFrame.origin.y = idealOrigin.y;
		[self setFrame:windowFrame display:NO];
	}
}// setPoint:onSide:


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
	Popover_Properties	chosenSide = [self.class bestSideForViewOfSize:self->embeddedView.frame.size
																		cornerMargins:[self cornerMargins]
																		arrowHeight:self.arrowHeight
																		arrowInset:self.arrowInset
																		at:aPoint
																		onParentWindow:self->popoverParentWindow
																		preferredSide:aSide];
	
	
	[self setPoint:aPoint onSide:chosenSide];
}// setPointWithAutomaticPositioning:preferredSide:


#pragma mark Accessors: General


/*!
Accessor.

The base width determines how “fat” the frame arrow’s triangle is.

(1.0)
*/
- (CGFloat)
arrowBaseWidth
{
	return self->_arrowBaseWidth;
}
- (void)
setArrowBaseWidth:(CGFloat)		aValue
{
	CGFloat		maxWidth = (MIN(NSWidth(self->embeddedView.frame), NSHeight(self->embeddedView.frame)) +
							CGFLOAT_TIMES_2(MIN([self cornerMargins].width, [self cornerMargins].height))) -
							self.cornerRadius;
	
	
	if (self.hasRoundCornerBesideArrow)
	{
		maxWidth -= self.cornerRadius;
	}
	
	if (aValue <= maxWidth)
	{
		self->_arrowBaseWidth = aValue;
	}
	else
	{
		self->_arrowBaseWidth = maxWidth;
	}
	
	[self redisplay];
}// setArrowBaseWidth:


/*!
Accessor.

The border is drawn inside the viewMargin area, expanding inwards; it
does not increase the width/height of the window.  You can use
"setBorderWidth:" and "setViewMargin:" together to achieve the exact
look/geometry you want.

(1.0)
*/
- (CGFloat)
borderWidth
{
	return self->_borderWidth;
}
- (void)
setBorderWidth:(CGFloat)	aValue
{
	if (self->_borderWidth != aValue)
	{
		CGFloat		maxBorderWidth = (self.viewMargin - 1.0f);
		
		
		if (aValue <= maxBorderWidth)
		{
			self->_borderWidth = aValue;
		}
		else
		{
			self->_borderWidth = maxBorderWidth;
		}
		
		[self updateBackground];
	}
}// setBorderWidth:


/*!
Accessor.

The radius in pixels of the arc used to draw curves at the
corners of the popover frame.

(1.0)
*/
- (CGFloat)
cornerRadius
{
	return self->_cornerRadius;
}
- (void)
setCornerRadius:(CGFloat)	aValue
{
	CGFloat		maxRadius = CGFLOAT_DIV_2((MIN(NSWidth(self->embeddedView.frame), NSHeight(self->embeddedView.frame)) +
											CGFLOAT_TIMES_2(MIN([self cornerMargins].width, [self cornerMargins].height))) -
											self.arrowBaseWidth);
	
	
	if (aValue <= maxRadius)
	{
		self->_cornerRadius = aValue;
	}
	else
	{
		self->_cornerRadius = maxRadius;
	}
	self->_cornerRadius = MAX(self->_cornerRadius, 0.0);
	
	// synchronize arrow size
	self.arrowBaseWidth = self->_arrowBaseWidth;
}// setCornerRadius:


/*!
Accessor.

Specifies whether or not the frame has an arrow displayed.
This is set implicitly via "setArrowStyle:" or through an
initializer.
*/
- (BOOL)
hasArrow
{
	BOOL	result = (self.arrowHeight > 0);
	
	
	return result;
}// hasArrow


/*!
Accessor.

The style-specified distance between the edge of the view and
the window edge.  Additional space can be inserted if there
are resize handles.

(1.0)
*/
- (CGFloat)
viewMargin
{
	return self->_viewMargin;
}
- (void)
setViewMargin:(CGFloat)		aValue
{
	if (self->_viewMargin != aValue)
	{
		self->_viewMargin = MAX(aValue, 0.0);
	}
}// setViewMargin:


#pragma mark CocoaAnimation_WindowImageProvider


/*!
Returns the most recently constructed rendering of the entire
popover window’s frame and content.  Currently, the content
portion is only created once (at initialization time) but the
frame portion is refreshed as needed.

(2017.06)
*/
- (NSImage*)
windowImage
{
	NSImage*	result = nil;
	
	
	if (nil == self->_animationFullImage)
	{
		// first call, or previous image was invalidated;
		// generate a new complete image
		self->_animationFullImage = [self newFullWindowImage];
	}
	
	result = self.animationFullImage;
	
	return result;
}// windowImage


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
	BOOL	handled = NO;
	
	
	if ([self observerArray:self.registeredObservers containsContext:aContext])
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			NSRect		newFrame = self.frame;
			// IMPORTANT: these will only be defined if the call to add
			// the observer includes the appropriate options
			id			oldValue = [aChangeDictionary objectForKey:NSKeyValueChangeOldKey];
			id			newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
			
			
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(arrowHeight)))
			{
				float const		kOldFloat = [oldValue floatValue];
				float const		kNewFloat = [newValue floatValue];
				NSRect const	kViewFrame = [self.class
												contentRectForFrameRect:self.frame
																		arrowHeight:kOldFloat
																		side:self.windowPropertyFlags];
				NSRect const	kNewFrame = [self.class
												frameRectForContentRect:kViewFrame
																		arrowHeight:kNewFloat
																		side:self.windowPropertyFlags];
				
				
				// when the arrow height changes, the frame grows or shrinks
				[self setFrame:kNewFrame display:NO];
				
				// frame image is out of date
				[_animationFullImage release], _animationFullImage = nil;
				
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
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(effectiveAppearance)))
			{
				// NSApplication has changed appearance, e.g. Dark or not; this
				// may cause the colors themselves to be different so apply the
				// window style again
				[self applyWindowStyle:self.lastAppliedWindowStyle];
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
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(resizeDelegate)))
			{
				// set delegate-defined properties
				BOOL	allowHorizontalResize = YES;
				BOOL	allowVerticalResize = YES;
				
				
				if (nil != self.resizeDelegate)
				{
					if ([self.resizeDelegate respondsToSelector:@selector(popover:getHorizontalResizeAllowed:getVerticalResizeAllowed:)])
					{
						[self.resizeDelegate popover:self getHorizontalResizeAllowed:&allowHorizontalResize getVerticalResizeAllowed:&allowVerticalResize];
						self.allowHorizontalResize = allowHorizontalResize;
						self.allowVerticalResize = allowVerticalResize;
						
						// adjust frame to make space for the difference in margin
						if (allowHorizontalResize || allowVerticalResize)
						{
							if (allowHorizontalResize)
							{
								newFrame.size.width += CGFLOAT_TIMES_2(kMy_ResizeClickWidth);
							}
							if (allowVerticalResize)
							{
								newFrame.size.height += CGFLOAT_TIMES_2(kMy_ResizeClickWidth);
							}
							[self setFrame:newFrame display:NO];
							[self fixViewFrame];
						}
					}
				}
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(viewMargin)))
			{
				float const		kOldFloat = [oldValue floatValue];
				float const		kNewFloat = [newValue floatValue];
				
				
				// synchronize corner radius (and arrow base width)
				self.cornerRadius = self.cornerRadius;
				
				// adjust frame to make space for the difference in margin
				newFrame.size.width += CGFLOAT_TIMES_2(kNewFloat - kOldFloat);
				newFrame.size.height += CGFLOAT_TIMES_2(kNewFloat - kOldFloat);
				[self setFrame:newFrame display:NO];
				[self fixViewFrame];
				
				// frame image is out of date
				[_animationFullImage release], _animationFullImage = nil;
			}
			else if (KEY_PATH_IS_SEL(aKeyPath, @selector(windowPropertyFlags)))
			{
				// fix layout and update background
				[self redisplay];
			}
			else
			{
				Console_Warning(Console_WriteValueCFString, "valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


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
}// validateMenuItem:


#pragma mark NSResponder


/*!
Responds to a mouse-down on the edge of the popover by
allowing the user to resize the window.

It is unfortunate that Apple does not have better support
for resizing-from-any-edge (including cursors) in custom
windows.  If a custom window is given the resize property,
Cocoa obnoxiously adds a FRAME and a ZOOM button to the
window even though the frame has no role in resizing!  If
that is ever fixed, this custom approach might be avoided;
until then, the only way to have a nice resize behavior is
to reimplement the entire thing manually.

(2017.05)
*/
- (void)
mouseDown:(NSEvent*)	aMouseEvent
{
	BOOL	isHandled = NO;
	
	
	if ((self.allowHorizontalResize || self.allowVerticalResize) &&
		((nil == aMouseEvent.window) || (self == aMouseEvent.window)))
	{
		NSPoint		windowMousePoint = [aMouseEvent locationInWindow];
		NSPoint		contentMousePoint = [[self contentViewAsNSView] convertPoint:windowMousePoint fromView:nil];
		
		
		self.userResizeLeft = (self.allowHorizontalResize && NSPointInRect(contentMousePoint, [self contentEdgeFrameLeft]));
		self.userResizeRight = (self.allowHorizontalResize && NSPointInRect(contentMousePoint, [self contentEdgeFrameRight]));
		self.userResizeTop = (self.allowVerticalResize && NSPointInRect(contentMousePoint, [self contentEdgeFrameTop]));
		self.userResizeBottom = (self.allowVerticalResize && NSPointInRect(contentMousePoint, [self contentEdgeFrameBottom]));
		
		if ((self.userResizeBottom) || (self.userResizeTop) ||
			(self.userResizeLeft) || (self.userResizeRight))
		{
			isHandled = YES;
			self.isBeingResizedByUser = YES; // see "mouseUp:"
			self.userResizeClickScreenPoint = [self convertBaseToScreen:[aMouseEvent locationInWindow]]; // see "mouseUp:"
			self.userResizeOriginalCenterX = NSMidX(self.frame);
			
			// NOTE: the Apple any-direction window resize cursors are not
			// part of the standard NSCursor API; while there are hacks
			// that could be done to resolve this (like copying the PDF
			// data from HIServices.framework), the vast majority of
			// affected windows are only resizable along one axis anyway
			// so the less-ideal official cursors are sufficient
			if (((self.userResizeLeft) && (self.userResizeTop)) ||
				((self.userResizeLeft) && (self.userResizeBottom)) ||
				((self.userResizeRight) && (self.userResizeTop)) ||
				((self.userResizeRight) && (self.userResizeBottom)))
			{
				// INCOMPLETE: compare window size to maximum/minimum as appropriate
				// and change the cursor if there is a size restriction
				[[NSCursor crosshairCursor] set];
			}
			else if ((self.userResizeLeft) || (self.userResizeRight))
			{
				// INCOMPLETE: compare window size to maximum/minimum as appropriate
				// and change the cursor if there is a size restriction
				[[NSCursor resizeLeftRightCursor] set];
			}
			else
			{
				// INCOMPLETE: compare window size to maximum/minimum as appropriate
				// and change the cursor if there is a size restriction
				[[NSCursor resizeUpDownCursor] set];
			}
		}
	}
	
	if (NO == isHandled)
	{
		[super mouseDown:aMouseEvent];
	}
}// mouseDown:


/*!
Responds to a mouse-drag event by resizing the window, if
a resize is in progress.

(2017.05)
*/
- (void)
mouseDragged:(NSEvent*)		aMouseEvent
{
	NSPoint		targetPoint = [self convertBaseToScreen:[aMouseEvent locationInWindow]]; // see "mouseUp:"
	//NSRect		relevantScreenFrame = [[self screen] visibleFrame];
	
	
	// UNIMPLEMENTED: constrain mouse point to screen
	
	if (((nil == aMouseEvent.window) || (self == aMouseEvent.window)) &&
		(self.isBeingResizedByUser))
	{
		BOOL		isHorizontallyCentered = ((kPopover_PropertyArrowMiddle == [self arrowPlacement]) &&
												((kPopover_PropertyPlaceFrameBelowArrow == [self windowPlacement]) ||
													(kPopover_PropertyPlaceFrameAboveArrow == [self windowPlacement])));
		NSRect		newFrame = self.frame;
		CGFloat		mouseDeltaX = ((self.userResizeLeft || self.userResizeRight)
									? (targetPoint.x - self.userResizeClickScreenPoint.x)
									: 0);
		CGFloat		mouseDeltaY = ((self.userResizeTop || self.userResizeBottom)
									? (targetPoint.y - self.userResizeClickScreenPoint.y)
									: 0);
		CGFloat		deltaX = 0;
		CGFloat		deltaY = 0;
		CGFloat		deltaW = 0;
		CGFloat		deltaH = 0;
		
		
		// the effect of moving the mouse depends on where the original
		// click location was, relative to the frame; for instance,
		// dragging to the right on the left edge is shrinking the
		// frame but on the right edge it would be growing the frame
		if (self.userResizeLeft)
		{
			deltaX = mouseDeltaX;
			deltaW = mouseDeltaX * ((isHorizontallyCentered) ? -2.0 : -1.0);
		}
		else
		{
			deltaX = mouseDeltaX * ((isHorizontallyCentered) ? -1.0 : 0.0);
			deltaW = mouseDeltaX * ((isHorizontallyCentered) ? 2.0 : 1.0);
		}
		
		if (self.userResizeBottom)
		{
			deltaY = mouseDeltaY;
			deltaH = -mouseDeltaY;
		}
		else
		{
			deltaH = mouseDeltaY;
		}
		
		// propose a new frame, without considering constraints yet
		newFrame.origin.x += deltaX;
		newFrame.origin.y += deltaY;
		newFrame.size.width += deltaW;
		newFrame.size.height += deltaH;
		
		// apply any window size constraints; if the size is constrained,
		// also refuse to alter the origin of the window
		{
			CGFloat const	kPreCheckWidth = NSWidth(newFrame);
			CGFloat const	kPreCheckHeight = NSHeight(newFrame);
			
			
			newFrame.size.width = MIN(NSWidth(newFrame), self.maxSize.width);
			newFrame.size.width = MAX(NSWidth(newFrame), self.minSize.width);
			newFrame.size.height = MIN(NSHeight(newFrame), self.maxSize.height);
			newFrame.size.height = MAX(NSHeight(newFrame), self.minSize.height);
			if (NSWidth(newFrame) != kPreCheckWidth)
			{
				newFrame.origin.x = self.frame.origin.x;
			}
			if (NSHeight(newFrame) != kPreCheckHeight)
			{
				newFrame.origin.y = self.frame.origin.y;
			}
		}
		
		// since floating-point errors can accumulate over a series of
		// delta values, a horizontally-centered frame may “drift” without
		// a correction factor to ensure the same absolute center as before
		if (isHorizontallyCentered)
		{
			newFrame.origin.x = (self.userResizeOriginalCenterX - CGFLOAT_DIV_2(NSWidth(newFrame)));
		}
		
		// resize the window and regenerate the popover background to match;
		// arbitrarily ignore slight mouse movements
		if ((std::abs(mouseDeltaX) >= 1.0) || (std::abs(mouseDeltaY) >= 1.0))
		{
			if (NO == self.allowHorizontalResize)
			{
				newFrame.origin.x = self.frame.origin.x;
				newFrame.size.width = NSWidth(self.frame);
			}
			if (NO == self.allowVerticalResize)
			{
				newFrame.origin.y = self.frame.origin.y;
				newFrame.size.height = NSHeight(self.frame);
			}
			[self setFrame:newFrame display:NO];
			[self redisplay];
			
			// save click point for next drag track
			self.userResizeClickScreenPoint = targetPoint;
		}
	}
	else
	{
		[super mouseUp:aMouseEvent];
	}
}// mouseDragged:


/*!
Updates the cursor in resizable windows.

(2017.05)
*/
- (void)
mouseEntered:(NSEvent*)		aMouseEvent
{
	BOOL		isHandled = NO;
	NSPoint		windowMousePoint = [aMouseEvent locationInWindow];
	NSPoint		contentMousePoint = [[self contentViewAsNSView]
										convertPoint:windowMousePoint fromView:nil];
	
	
	if (aMouseEvent.trackingArea == self.trackMainFrame)
	{
		if (NO == self.isBeingResizedByUser)
		{
			[[NSCursor arrowCursor] set];
		}
		isHandled = YES;
	}
	
	if (self.allowHorizontalResize)
	{
		if ((aMouseEvent.trackingArea == self.trackLeftEdge) ||
			(aMouseEvent.trackingArea == self.trackRightEdge))
		{
			isHandled = YES;
			if ((self.allowVerticalResize) &&
				(NSPointInRect(contentMousePoint, [self contentEdgeFrameTop]) ||
					NSPointInRect(contentMousePoint, [self contentEdgeFrameBottom])))
			{
				// actually intersects the corner
				[[NSCursor crosshairCursor] set];
			}
			else if (NSWidth(self.frame) == self.minSize.width)
			{
				if (aMouseEvent.trackingArea == self.trackLeftEdge)
				{
					[[NSCursor resizeLeftCursor] set];
				}
				else
				{
					[[NSCursor resizeRightCursor] set];
				}
			}
			else
			{
				[[NSCursor resizeLeftRightCursor] set];
			}
		}
	}
	
	if (self.allowVerticalResize)
	{
		if ((aMouseEvent.trackingArea == self.trackTopEdge) ||
			(aMouseEvent.trackingArea == self.trackBottomEdge))
		{
			isHandled = YES;
			if ((self.allowHorizontalResize) &&
				(NSPointInRect(contentMousePoint, [self contentEdgeFrameLeft]) ||
					NSPointInRect(contentMousePoint, [self contentEdgeFrameRight])))
			{
				// actually intersects the corner
				[[NSCursor crosshairCursor] set];
			}
			else if (NSHeight(self.frame) == self.minSize.height)
			{
				if (aMouseEvent.trackingArea == self.trackTopEdge)
				{
					[[NSCursor resizeUpCursor] set];
				}
				else
				{
					[[NSCursor resizeDownCursor] set];
				}
			}
			else
			{
				[[NSCursor resizeUpDownCursor] set];
			}
		}
	}
	
	if (NO == isHandled)
	{
		[super mouseEntered:aMouseEvent];
	}
}// mouseEntered:


/*!
Updates the cursor in resizable windows.

(2017.05)
*/
- (void)
mouseExited:(NSEvent*)		aMouseEvent
{
	BOOL	isHandled = NO;
	
	
	if (self.allowHorizontalResize)
	{
		if ((aMouseEvent.trackingArea == self.trackLeftEdge) ||
			(aMouseEvent.trackingArea == self.trackRightEdge))
		{
			if (NO == self.isBeingResizedByUser)
			{
				[[NSCursor arrowCursor] set];
			}
			isHandled = YES;
		}
	}
	
	if (self.allowVerticalResize)
	{
		if ((aMouseEvent.trackingArea == self.trackTopEdge) ||
			(aMouseEvent.trackingArea == self.trackBottomEdge))
		{
			if (NO == self.isBeingResizedByUser)
			{
				[[NSCursor arrowCursor] set];
			}
			isHandled = YES;
		}
	}
	
	// during resize, ignore all mouse-exit cursor changes
	if ((NO == self.isBeingResizedByUser) && (NO == isHandled))
	{
		[super mouseExited:aMouseEvent];
	}
}// mouseExited:


/*!
Responds to a mouse-up event by clearing the resize flag.

(2017.05)
*/
- (void)
mouseUp:(NSEvent*)	aMouseEvent
{
	if ((nil == aMouseEvent.window) || (self == aMouseEvent.window))
	{
		self.isBeingResizedByUser = NO; // see "mouseDown:"
		self.userResizeClickScreenPoint = NSZeroPoint;
		self.userResizeOriginalCenterX = 0.0;
		self.userResizeTop = NO;
		self.userResizeLeft = NO;
		self.userResizeBottom = NO;
		self.userResizeRight = NO;
		[self resetTrackingAreas];
	}
	
	// always pass the event up the chain
	[super mouseUp:aMouseEvent];
}// mouseUp:


#pragma mark NSWindow


/*!
Overrides the NSWindow implementation.

(1.0)
*/
- (BOOL)
canBecomeMainWindow
{
	return NO;
}// canBecomeMainWindow


/*!
Overrides the NSWindow implementation.

(1.0)
*/
- (BOOL)
canBecomeKeyWindow
{
	return YES;
}// canBecomeKeyWindow


/*!
Overrides the NSWindow implementation.

(1.0)
*/
- (BOOL)
isExcludedFromWindowsMenu
{
	return YES;
}// isExcludedFromWindowsMenu


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
}// performClose:


/*!
Overrides the NSWindow implementation of the background color to
cause an equivalent (semi-transparent) image to be the background.

(1.0)
*/
- (void)
setBackgroundColor:(NSColor*)	aValue
{
	self.popoverBackgroundColor = aValue;
}// setBackgroundColor:


#pragma mark NSWindowNotifications


/*!
Responds to window activation by restoring the frame colors
and installing cursor-tracking areas in resizable windows.

(1.4)
*/
- (void)
windowDidBecomeKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.borderOuterDisplayColor = self.borderOuterColor;
	self.borderPrimaryDisplayColor = self.borderPrimaryColor;
	[self resetTrackingAreas];
	[self updateBackground];
}// windowDidBecomeKey:


/*!
Responds to window deactivation by graying the frame colors.

(1.4)
*/
- (void)
windowDidResignKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.borderOuterDisplayColor = [self colorWithName:@"DialogFrameOuterInactive"
														grayName:@"DialogFrameOuterInactive"
														defaultColor:colorRGBA(0.95f, 0.95f, 0.95f, 1.0f)
														defaultGray:colorRGBA(0.95f, 0.95f, 0.95f, 1.0f)];
	self.borderPrimaryDisplayColor = [self colorWithName:@"DialogFramePrimaryInactive"
															grayName:@"DialogFramePrimaryInactive"
															defaultColor:colorRGBA(0.65f, 0.65f, 0.65f, 1.0f)
															defaultGray:colorRGBA(0.65f, 0.65f, 0.65f, 1.0f)];
	[self removeTrackingAreas];
	[self updateBackground];
}// windowDidResignKey:


/*!
Responds to window resize notifications by redrawing the background.

(1.0)
*/
- (void)
windowDidResize:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self redisplay];
}// windowDidResize:


@end //} Popover_Window


#pragma mark -
@implementation Popover_Window (Popover_WindowInternal) //{


#pragma mark Class Methods: General


/*!
A helper to determine the offset from the nearest corner that
an arrow’s tip should have, given the specified corner arc size
and width of the arrow’s triangular base.

(1.0)
*/
+ (CGFloat)
arrowInsetWithCornerRadius:(CGFloat)	aCornerRadius
baseWidth:(CGFloat)						anArrowBaseWidth
{
	return (aCornerRadius + (anArrowBaseWidth / 2.0f));
}// arrowInsetWithCornerRadius:baseWidth:


/*!
Returns the arrow placement portion of the specified properties.

(1.1)
*/
+ (Popover_Properties)
arrowPlacement:(Popover_Properties)		aFlagSet
{
	Popover_Properties		result = (aFlagSet & kPopover_PropertyMaskArrow);
	
	
	return result;
}// arrowPlacement:


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
cornerMargins:(NSSize)				aMarginSize
arrowHeight:(CGFloat)				anArrowHeight
arrowInset:(CGFloat)				anArrowInset
at:(NSPoint)						aPoint
onParentWindow:(NSWindow*)			aWindow
preferredSide:(Popover_Properties)	aSide
{
	// pretend the given view size actually included the margins
	// (the margins are not otherwise needed)
	NSSize			effectiveViewSize = NSMakeSize(aViewSize.width + CGFLOAT_TIMES_2(aMarginSize.width),
													aViewSize.height + CGFLOAT_TIMES_2(aMarginSize.height));
	NSRect const	kScreenFrame = ((nil != aWindow) && (nil != [aWindow screen]))
									? [[aWindow screen] visibleFrame]
									: [[NSScreen mainScreen] visibleFrame];
	NSPoint const	kPointOnScreen = (nil != aWindow)
										? [self.class convertToScreenFromWindow:aWindow point:aPoint]
										: aPoint;
	Popover_Properties		result = kPopover_PositionBottom;
	
	
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
	CGFloat						greatestArea = 0.0;
	
	
	// find the window and arrow placement that leaves the greatest
	// portion of the popover view on screen
	result = 0; // initially...
	for (size_t i = 0; i < kCandidateCount; ++i)
	{
		NSRect		contentRect = NSMakeRect(0, 0, effectiveViewSize.width, effectiveViewSize.height);
		NSRect		frameRect = [self.class frameRectForContentRect:contentRect
																	arrowHeight:anArrowHeight
																	side:kCandidatePositions[i]];
		NSPoint		idealOrigin = [self.class idealFrameOriginForSize:frameRect.size arrowInset:anArrowInset
																		at:kPointOnScreen side:kCandidatePositions[i]];
		
		
		// adjust to position on screen
		frameRect.origin.x = idealOrigin.x;
		frameRect.origin.y = idealOrigin.y;
		
		// see if this rectangle shows more of the view than the previous best
		{
			NSRect const	kIntersectionRegion = NSIntersectionRect(frameRect, kScreenFrame);
			CGFloat const	kIntersectionArea = (kIntersectionRegion.size.width * kIntersectionRegion.size.height);
			
			
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
}// bestSideForViewOfSize:andMargin:arrowHeight:arrowInset:at:onParentWindow:preferredSide:


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
arrowInset:(CGFloat)				anArrowInset
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
}// idealFrameOriginForSize:arrowInset:at:side:


/*!
Returns YES only if the current system appearance is
using the graphite theme (grayscale).  This is used
to determine if frames should use color.

Use this only as a hint on macOS 10.14 or later, as
there are more appearance-specific APIs available.

(2016.06)
*/
+ (BOOL)
isGraphiteTheme
{
	return (NSGraphiteControlTint == [NSColor currentControlTint]);
}// isGraphiteTheme


/*!
Returns the window placement portion of the specified properties.

(1.1)
*/
+ (Popover_Properties)
windowPlacement:(Popover_Properties)	aFlagSet
{
	Popover_Properties		result = (aFlagSet & kPopover_PropertyMaskPlaceFrame);
	
	
	return result;
}// windowPlacement:


#pragma mark Class Methods: Frame Conversion


/*!
Returns an NSView frame rectangle (relative to its window content
view) appropriate for the specified window-relative frame and
related parameters.

This boundary is not completely visible to the user because the
arrow and all of the space around the arrow appears in the frame
rectangle too.

This should be consistent with the calculations for the method
"frameRectForContentRect:arrowHeight:side:".

(2017.06)
*/
+ (NSRect)
contentRectForFrameRect:(NSRect)	aRect
arrowHeight:(CGFloat)				anArrowHeight
side:(Popover_Properties)			aSide
{
	CGFloat const	kOffsetArrowSideH = anArrowHeight;
	CGFloat const	kOffsetArrowSideV = anArrowHeight;
	CGFloat const	kOffsetNonArrowSideH = 0;
	CGFloat const	kOffsetNonArrowSideV = 0;
	NSRect			result = aRect;
	
	
	switch (aSide)
	{
	case kPopover_PositionLeft:
	case kPopover_PositionLeftTop:
	case kPopover_PositionLeftBottom:
		result.origin.x += kOffsetNonArrowSideH;
		result.origin.y += kOffsetNonArrowSideV;
		result.size.width -= (kOffsetNonArrowSideH + kOffsetArrowSideH);
		result.size.height -= (kOffsetNonArrowSideV + kOffsetNonArrowSideV);
		break;
	
	case kPopover_PositionRight:
	case kPopover_PositionRightTop:
	case kPopover_PositionRightBottom:
		result.origin.x += kOffsetArrowSideH;
		result.origin.y += kOffsetNonArrowSideV;
		result.size.width -= (kOffsetArrowSideH + kOffsetNonArrowSideH);
		result.size.height -= (kOffsetNonArrowSideV + kOffsetNonArrowSideV);
		break;
	
	case kPopover_PositionBottom:
	case kPopover_PositionBottomLeft:
	case kPopover_PositionBottomRight:
		result.origin.x += kOffsetNonArrowSideH;
		result.origin.y += kOffsetNonArrowSideV;
		result.size.width -= (kOffsetNonArrowSideH + kOffsetNonArrowSideH);
		result.size.height -= (kOffsetArrowSideV + kOffsetNonArrowSideV);
		break;
	
	case kPopover_PositionTop:
	case kPopover_PositionTopLeft:
	case kPopover_PositionTopRight:
	default:
		result.origin.x += kOffsetNonArrowSideH;
		result.origin.y += kOffsetArrowSideV;
		result.size.width -= (kOffsetNonArrowSideH + kOffsetNonArrowSideH);
		result.size.height -= (kOffsetNonArrowSideV + kOffsetArrowSideV);
		break;
	}
	
	return result;
}// contentRectForFrameRect:arrowHeight:side:


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
}// convertToScreenFromWindow:point:


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
}// convertToWindow:fromScreenPoint:


/*!
Returns an NSView frame rectangle (relative to its parent view)
appropriate for the specified content size and related parameters.

This should be consistent with calculations performed by the
"frameSizeForEmbeddedViewSize:cornerMargins:arrowHeight:" method.

(2017.07)
*/
+ (NSRect)
embeddedViewRectForContentSize:(NSSize)		aContentSize
cornerMargins:(NSSize)						aMarginSize
{
	NSRect		result = NSMakeRect(aMarginSize.width, aMarginSize.height, 0, 0);
	
	
	result.size.width = (aContentSize.width - CGFLOAT_TIMES_2(aMarginSize.width));
	result.size.height = (aContentSize.height - CGFLOAT_TIMES_2(aMarginSize.height));
	
	return result;
}// embeddedViewRectForContentSize:cornerMargins:


/*!
Returns an NSWindow frame rectangle (relative to the window
position on screen) appropriate for the specified content view
size and related parameters.

The content rectangle represents everything that the user can
interact with, which includes resize handles but not any arrow.
This is different from the “embedded view”, which may be
slightly inset from the content view frame.

This should be consistent with the calculations for the method
"contentRectForFrameRect:arrowHeight:side:".

(2017.06)
*/
+ (NSRect)
frameRectForContentRect:(NSRect)	aRect
arrowHeight:(CGFloat)				anArrowHeight
side:(Popover_Properties)			aSide
{
	CGFloat const	kOffsetArrowSideH = anArrowHeight;
	CGFloat const	kOffsetArrowSideV = anArrowHeight;
	CGFloat const	kOffsetNonArrowSideH = 0;
	CGFloat const	kOffsetNonArrowSideV = 0;
	NSRect			result = aRect;
	
	
	switch (aSide)
	{
	case kPopover_PositionLeft:
	case kPopover_PositionLeftTop:
	case kPopover_PositionLeftBottom:
		result.origin.x -= kOffsetNonArrowSideH;
		result.origin.y -= kOffsetNonArrowSideV;
		result.size.width += (kOffsetNonArrowSideH + kOffsetArrowSideH);
		result.size.height += (kOffsetNonArrowSideV + kOffsetNonArrowSideV);
		break;
	
	case kPopover_PositionRight:
	case kPopover_PositionRightTop:
	case kPopover_PositionRightBottom:
		result.origin.x -= kOffsetArrowSideH;
		result.origin.y -= kOffsetNonArrowSideV;
		result.size.width += (kOffsetArrowSideH + kOffsetNonArrowSideH);
		result.size.height += (kOffsetNonArrowSideV + kOffsetNonArrowSideV);
		break;
	
	case kPopover_PositionBottom:
	case kPopover_PositionBottomLeft:
	case kPopover_PositionBottomRight:
		result.origin.x -= kOffsetNonArrowSideH;
		result.origin.y -= kOffsetNonArrowSideV;
		result.size.width += (kOffsetNonArrowSideH + kOffsetNonArrowSideH);
		result.size.height += (kOffsetArrowSideV + kOffsetNonArrowSideV);
		break;
	
	case kPopover_PositionTop:
	case kPopover_PositionTopLeft:
	case kPopover_PositionTopRight:
	default:
		result.origin.x -= kOffsetNonArrowSideH;
		result.origin.y -= kOffsetArrowSideV;
		result.size.width += (kOffsetNonArrowSideH + kOffsetNonArrowSideH);
		result.size.height += (kOffsetNonArrowSideV + kOffsetArrowSideV);
		break;
	}
	
	return result;
}// frameRectForContentRect:arrowHeight:side:


/*!
Returns the width and height that the entire frame should
have in order to accommodate an embedded view of the given
size.

This should be consistent with calculations performed by
"embeddedViewRectForContentSize:cornerMargins:".

(2017.06)
*/
+ (NSSize)
frameSizeForEmbeddedViewSize:(NSSize)	anEmbeddedViewSize
cornerMargins:(NSSize)					aMarginSize
arrowHeight:(CGFloat)					anArrowHeight
{
	NSSize const	contentSize = NSMakeSize(anEmbeddedViewSize.width + CGFLOAT_TIMES_2(aMarginSize.width),
												anEmbeddedViewSize.height + CGFLOAT_TIMES_2(aMarginSize.height));
	NSRect			frameRect = [self frameRectForContentRect:NSMakeRect(0, 0, contentSize.width,
																				contentSize.height)
											arrowHeight:anArrowHeight
											side:kPopover_PositionBottom/* unimportant for size */];
	NSSize			result = frameRect.size;
	
	
	return result;
}// frameSizeForEmbeddedViewSize:cornerMargins:arrowHeight:


#pragma mark New Methods


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
		CGFloat const	kScaleFactor = 1.0;
		CGFloat const	kScaledArrowWidth = self.arrowBaseWidth * kScaleFactor;
		CGFloat const	kHalfScaledArrowWidth = kScaledArrowWidth / 2.0f;
		CGFloat const	kScaledArrowHeight = self.arrowHeight * kScaleFactor;
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
}// appendArrowToPath:


/*!
The per-window version of "convertToScreenFromWindow:point:".

(1.2)
*/
- (NSPoint)
convertToScreenFromWindowPoint:(NSPoint)	aPoint
{
	return [self.class convertToScreenFromWindow:self point:aPoint];
}// convertToScreenFromWindowPoint:


/*!
The per-window version of "convertToWindow:fromScreenPoint:".

(1.2)
*/
- (NSPoint)
convertToWindowFromScreenPoint:(NSPoint)	aPoint
{
	return [self.class convertToWindow:self fromScreenPoint:aPoint];
}// convertToWindowFromScreenPoint:


/*!
Updates the location and size of the content view and embedded view
so they are correct for the current configuration of the popover
(arrow position, offsets, resize handles, etc.).

Note that the embedded view frame will be forced to leave the
expected amount of margin relative to the content view, even if the
embedded view would be cramped by doing so!  To avoid improper view
sizes, the initial size of the window frame should always be set by
calculating the frame in terms of the content (such as by calling
"frameRectForContentRect:arrowHeight:side:" or another class method)
and any changes to key frame properties should cause the entire
frame to be recalculated.  That way, by the time this method is
invoked, the frame will be large enough to hold content at the
expected size.

(1.1)
*/
- (void)
fixViewFrame
{
	NSRect		contentFrame = [self contentFrame];
	NSRect		viewFrame = [self.class embeddedViewRectForContentSize:contentFrame.size
																		cornerMargins:[self cornerMargins]];
	
	
	[self contentViewAsNSView].frame = contentFrame;
	self->embeddedView.frame = viewFrame;
}// fixViewFrame


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
												side:self.windowPropertyFlags];
}// idealFrameOriginForPoint:


/*!
Returns a rendering of the popover window’s content.  This is
typically only needed for animations.  See also the method
"backgroundFrameImageAsColor".

The caller must release the image.

IMPORTANT: Once the window has been fully initialized, the
presence of a visual effect view may prevent the content from
being included in the image (in other words, only the frame
will be visible).  This is called early in the initialization
phase so that the initial image is OK.

(2017.06)
*/
- (NSImage*)
newContentWindowImage
{
	BOOL		wasVisible = self.isVisible;
	NSPoint		oldFrameOrigin = self.frame.origin;
	NSImage*	result = [[NSImage alloc] initWithSize:self->embeddedView.frame.size];
	
	
	if (NO == wasVisible)
	{
		// show the window offscreen so its image is defined
		[self setFrameOrigin:NSMakePoint(-5000, -5000)];
		[self orderFront:nil];
	}
	
	@try
	{
		NSBitmapImageRep*	imageRep = nil;
		
		
		// create an image with the content of the window (no frame)
		[self->embeddedView lockFocus];
		imageRep = [[NSBitmapImageRep alloc] initWithFocusedViewRect:self->embeddedView.bounds];
		[self->embeddedView unlockFocus];
		[result addRepresentation:imageRep];
		[imageRep release], imageRep = nil;
	}
	@catch (NSException*	inException)
	{
		Console_Warning(Console_WriteValueCFString, "failed to create window content image, exception",
						BRIDGE_CAST([inException name], CFStringRef));
	}
	
	// restore window position and visibility
	if (NO == wasVisible)
	{
		[self orderOut:NSApp];
		[self setFrameOrigin:oldFrameOrigin];
	}
	
	return result;
}// newContentWindowImage


/*!
Returns a rendering of the entire popover window’s frame and
the cached content image.  This is typically only needed for
animations.  See also the method "backgroundFrameImageAsColor".

The caller must release the image.

(2017.06)
*/
- (NSImage*)
newFullWindowImage
{
	BOOL		wasVisible = self.isVisible;
	NSPoint		oldFrameOrigin = self.frame.origin;
	NSColor*	patternColor = [self backgroundFrameImageAsColor];
	NSImage*	result = nil;
	
	
	if (NO == wasVisible)
	{
		// show the window offscreen so its image is defined
		[self setFrameOrigin:NSMakePoint(-5000, -5000)];
		[self orderFront:nil];
	}
	
	@try
	{
		NSRect		windowRelativeFrame = NSMakeRect
											([self contentViewAsNSView].frame.origin.x + self->embeddedView.frame.origin.x,
												[self contentViewAsNSView].frame.origin.y + self->embeddedView.frame.origin.y,
												NSWidth(self->embeddedView.frame), NSHeight(self->embeddedView.frame));
		
		
		// create image with the frame of the window (no content)
		result = [[patternColor patternImage] copy];
		
		// now overlay the cached content of the window
		[result lockFocus];
		[self->_animationContentImage drawInRect:windowRelativeFrame fromRect:NSZeroRect
													operation:NSCompositeSourceOver fraction:1.0];
		[result unlockFocus];
	}
	@catch (NSException*	inException)
	{
		Console_Warning(Console_WriteValueCFString, "failed to draw window content image, exception",
						BRIDGE_CAST([inException name], CFStringRef));
	}
	
	// restore window position and visibility
	if (NO == wasVisible)
	{
		[self orderOut:NSApp];
		[self setFrameOrigin:oldFrameOrigin];
	}
	
	return result;
}// newFullWindowImage


/*!
Forces the window to redo its layout and render itself again.

(1.0)
*/
- (void)
redisplay
{
	if (NO == self.layoutInProgress)
	{
		self.layoutInProgress = YES;
		//NSDisableScreenUpdates();
		[self fixViewFrame];
		[self updateBackground];
		//NSEnableScreenUpdates();
		self.layoutInProgress = NO;
	}
}// redisplay


/*!
Frees any previously-installed tracking areas.  See also
"resetTrackingAreas", which does this implicitly before
installing new tracking areas.

(2017.05)
*/
- (void)
removeTrackingAreas
{
	NSView*		targetView = [self contentViewAsNSView];
	
	
	if (nil != self.trackMainFrame)
	{
		[targetView removeTrackingArea:self.trackMainFrame];
		[_trackMainFrame release], _trackMainFrame = nil;
	}
	
	if (nil != self.trackLeftEdge)
	{
		[targetView removeTrackingArea:self.trackLeftEdge];
		[_trackLeftEdge release], _trackLeftEdge = nil;
	}
	
	if (nil != self.trackRightEdge)
	{
		[targetView removeTrackingArea:self.trackRightEdge];
		[_trackRightEdge release], _trackRightEdge = nil;
	}
	
	if (nil == self.trackTopEdge)
	{
		[targetView removeTrackingArea:self.trackTopEdge];
		[_trackTopEdge release], _trackTopEdge = nil;
	}
	
	if (nil == self.trackBottomEdge)
	{
		[targetView removeTrackingArea:self.trackBottomEdge];
		[_trackBottomEdge release], _trackBottomEdge = nil;
	}
}// removeTrackingAreas


/*!
If the window is resizable, this installs tracking areas for
the current edges (removing existing ones as appropriate).  If
the window is not resizable, this does nothing.  Also, if the
window is resizable in only one direction, only the appropriate
edges have tracking areas installed.

(2017.05)
*/
- (void)
resetTrackingAreas
{
	NSView*					targetView = [self contentViewAsNSView];
	NSRect					defaultCursorBounds = [self contentFrame];
	NSSize					cornerMargins = [self cornerMargins];
	NSTrackingAreaOptions	trackingAreaOptions = (NSTrackingMouseEnteredAndExited |
													NSTrackingActiveInActiveApp);
	
	
	[self removeTrackingAreas];
	
	defaultCursorBounds.origin = NSZeroPoint;
	defaultCursorBounds.origin.x += cornerMargins.width;
	defaultCursorBounds.origin.y += cornerMargins.height;
	defaultCursorBounds.size.width -= CGFLOAT_TIMES_2(cornerMargins.width);
	defaultCursorBounds.size.height -= CGFLOAT_TIMES_2(cornerMargins.height);
	
	// NOTE: old one removed by "removeTrackingAreas" call above
	_trackMainFrame = [[NSTrackingArea alloc]
						initWithRect:defaultCursorBounds options:trackingAreaOptions
										owner:self userInfo:nil];
	[targetView addTrackingArea:self.trackMainFrame];
	
	if ((self.allowHorizontalResize) || (self.allowVerticalResize))
	{
		if (self.allowHorizontalResize)
		{
			// NOTE: old ones removed by "removeTrackingAreas" call above
			_trackLeftEdge = [[NSTrackingArea alloc]
								initWithRect:[self contentEdgeFrameLeft] options:trackingAreaOptions
												owner:self userInfo:nil];
			[targetView addTrackingArea:self.trackLeftEdge];
			_trackRightEdge = [[NSTrackingArea alloc]
								initWithRect:[self contentEdgeFrameRight] options:trackingAreaOptions
												owner:self userInfo:nil];
			[targetView addTrackingArea:self.trackRightEdge];
		}
		
		if (self.allowVerticalResize)
		{
			// NOTE: old ones removed by "removeTrackingAreas" call above
			_trackTopEdge = [[NSTrackingArea alloc]
								initWithRect:[self contentEdgeFrameTop] options:trackingAreaOptions
												owner:self userInfo:nil];
			[targetView addTrackingArea:self.trackTopEdge];
			_trackBottomEdge = [[NSTrackingArea alloc]
									initWithRect:[self contentEdgeFrameBottom] options:trackingAreaOptions
													owner:self userInfo:nil];
			[targetView addTrackingArea:self.trackBottomEdge];
		}
	}
}// resetTrackingAreas


/*!
Forces the window to render itself again.

(1.0)
*/
- (void)
updateBackground
{
	// frame image is out of date
	[_animationFullImage release], _animationFullImage = nil;
	
	//NSDisableScreenUpdates();
	// call superclass to avoid overridden version from this class
	@autoreleasepool
	{
		[super setBackgroundColor:[self backgroundFrameImageAsColor]];
	}
	
	if ([self isVisible])
	{
		[self display];
		[self invalidateShadow];
	}
	//NSEnableScreenUpdates();
}// updateBackground


#pragma mark Accessors: Colors


/*!
Returns the outer edge color for "kPopover_WindowStyleAlertAppModal"
and "kPopover_WindowStyleAlertSheet".

(2018.09)
*/
- (NSColor*)
colorAlertFrameOuter
{
	NSColor*	result = [self colorWithName:@"AlertFrameOuter"
												grayName:@"DialogFrameOuter"
												defaultColor:colorRGBA(1.0f, 0.9f, 0.9f, 1.0f)
												defaultGray:colorRGBA(0.9f, 0.9f, 0.9f, 1.0f)];
	
	
	return result;
}// colorAlertFrameOuter


/*!
Returns the primary edge color for "kPopover_WindowStyleAlertAppModal"
and "kPopover_WindowStyleAlertSheet".

(2018.09)
*/
- (NSColor*)
colorAlertFramePrimary
{
	NSColor*	result = [self colorWithName:@"AlertFramePrimary"
												grayName:@"DialogFramePrimary"
												defaultColor:colorRGBA(0.75f, 0.7f, 0.7f, 1.0f)
												defaultGray:colorRGBA(0.7f, 0.7f, 0.7f, 1.0f)];
	
	
	return result;
}// colorAlertFramePrimary


/*!
Returns the outer edge color for "kPopover_WindowStyleDialogAppModal"
and "kPopover_WindowStyleDialogSheet".

(2018.09)
*/
- (NSColor*)
colorDialogFrameOuter
{
	NSColor*	result = [self colorWithName:@"DialogFrameOuter"
												grayName:@"DialogFrameOuter"
												defaultColor:colorRGBA(0.85f, 0.85f, 0.85f, 1.0f)
												defaultGray:colorRGBA(0.85f, 0.85f, 0.85f, 1.0f)];
	
	
	return result;
}// colorDialogFrameOuter


/*!
Returns the primary edge color for "kPopover_WindowStyleDialogAppModal"
and "kPopover_WindowStyleDialogSheet".

(2018.09)
*/
- (NSColor*)
colorDialogFramePrimary
{
	NSColor*	result = [self colorWithName:@"DialogFramePrimary"
												grayName:@"DialogFramePrimary"
												defaultColor:colorRGBA(0.55f, 0.55f, 0.55f, 1.0f)
												defaultGray:colorRGBA(0.55f, 0.55f, 0.55f, 1.0f)];
	
	
	return result;
}// colorDialogFramePrimary


/*!
Returns the primary edge color for "kPopover_WindowStyleNormal".

(2018.09)
*/
- (NSColor*)
colorPopoverBackground
{
	NSColor*	result = [self colorWithName:@"PopoverBackground"
												grayName:@"PopoverBackground"
												defaultColor:colorRGBA(0.9f, 0.9f, 0.9f, 0.95f)
												defaultGray:colorRGBA(0.9f, 0.9f, 0.9f, 0.95f)];
	
	
	return result;
}// colorPopoverBackground


/*!
Returns the outer edge color for "kPopover_WindowStyleNormal".

(2018.09)
*/
- (NSColor*)
colorPopoverFrameOuter
{
	NSColor*	result = [self colorWithName:@"PopoverFrameOuter"
												grayName:@"PopoverFrameOuter"
												defaultColor:colorRGBA(0.25f, 0.25f, 0.25f, 0.7f)
												defaultGray:colorRGBA(0.25f, 0.25f, 0.25f, 0.7f)];
	
	
	return result;
}// colorPopoverFrameOuter


/*!
Returns the primary edge color for "kPopover_WindowStyleNormal".

(2018.09)
*/
- (NSColor*)
colorPopoverFramePrimary
{
	NSColor*	result = [self colorWithName:@"PopoverFramePrimary"
												grayName:@"PopoverFramePrimary"
												defaultColor:colorRGBA(1.0f, 1.0f, 1.0f, 0.8f)
												defaultGray:colorRGBA(1.0f, 1.0f, 1.0f, 0.8f)];
	
	
	return result;
}// colorPopoverFramePrimary


/*!
Returns the outer edge color for "kPopover_WindowStyleAlertAppModal".

(2018.09)
*/
- (NSColor*)
colorWithName:(NSString*)	aColorAssetName
grayName:(NSString*)		aGrayColorAssetName
defaultColor:(NSColor*)		aColor
defaultGray:(NSColor*)		aGrayColor
{
	NSString*	colorName = [self.class isGraphiteTheme] ? aGrayColorAssetName : aColorAssetName;
	NSColor*	defaultColor = [self.class isGraphiteTheme] ? aGrayColor : aColor;
	NSColor*	result = defaultColor;
	
	
	if (@available(macOS 10.13, *))
	{
		NSColor*	loadedColor = [NSColor colorNamed:colorName];
		
		
		if (nil == loadedColor)
		{
			Console_Warning(Console_WriteValueCFString, "failed to load color, name",
							BRIDGE_CAST(colorName, CFStringRef));
		}
		else
		{
			result = loadedColor;
		}
	}
	
	return result;
}// colorWithName:grayName:defaultColor:defaultGray:


#pragma mark Accessors: Other


/*!
Returns the offset from the nearest corner that the arrow’s
tip should have, if applicable.

(1.0)
*/
- (CGFloat)
arrowInset
{
	CGFloat const	kRadius = (self.hasRoundCornerBesideArrow)
								? self.cornerRadius
								: 0;
	
	
	return [self.class arrowInsetWithCornerRadius:kRadius baseWidth:self.arrowBaseWidth];
}// arrowInset


/*!
Returns the arrow placement portion of the current properties.

(2017.05)
*/
- (Popover_Properties)
arrowPlacement
{
	return [self.class arrowPlacement:self.windowPropertyFlags];
}// arrowPlacement


/*!
Returns the entire popover window’s frame and background image
as an NSColor so that it can be rendered naturally by NSWindow.

(1.0)
*/
- (NSColor*)
backgroundFrameImageAsColor
{
	NSColor*		result = nil;
	NSImage*		patternImage = [[NSImage alloc] initWithSize:self.frame.size];
	BOOL const		kDebug = NO;
	
	
	[patternImage lockFocus];
	
	[NSGraphicsContext saveGraphicsState];
	{
		NSRect			drawnFrame = [self contentFrame];
		NSBezierPath*	sourcePath = [self backgroundPath];
		
		
		[sourcePath addClip];
		
		[self.popoverBackgroundColor setFill];
		[sourcePath fill];
		if (self.borderWidth > 0)
		{
			// double width since drawing is clipped inside the path
			[sourcePath setLineWidth:(MAX(self.borderWidth * 2.0f, 1.0f))];
			[self.borderPrimaryDisplayColor setStroke];
			[sourcePath stroke];
			[sourcePath setLineWidth:(MAX(self.borderWidth, 2.0f) - 1.0f)]; // arbitrary shift
			[self.borderOuterDisplayColor setStroke];
			[sourcePath stroke];
		}
		
		// draw resize handles on the edges as needed
		if ((self.allowHorizontalResize) || (self.allowVerticalResize))
		{
			CGFloat const	kResizeHandleInset = 0.5;
			CGFloat const	kResizeHandleGrooveSpacing = 1.0;
			CGFloat const	kResizeHandleThickness = 1.0;
			CGFloat const	kResizeHandleLength = ((NSHeight(self.frame) < 180/* arbitrary */)
													? 24.0
													: 48.0);
			NSPoint			drawnBoundsOrigin = drawnFrame.origin;
			NSRect			handleRect;
			NSColor*		outerLineColor = [self colorWithName:@"ResizeHandle1"
																	grayName:@"ResizeHandle1"
																	defaultColor:colorRGBA(0.8f, 0.8f, 0.8f, 1.0f)
																	defaultGray:colorRGBA(0.8f, 0.8f, 0.8f, 1.0f)];
			NSColor*		middleLineColor = [self colorWithName:@"ResizeHandle2"
																	grayName:@"ResizeHandle2"
																	defaultColor:colorRGBA(0.4f, 0.4f, 0.4f, 1.0f)
																	defaultGray:colorRGBA(0.4f, 0.4f, 0.4f, 1.0f)];
			NSColor*		innerLineColor = outerLineColor;
			NSColor*		innerLine2Color = middleLineColor;
			
			
			if (self.allowHorizontalResize)
			{
				// put handles on the left and right edges
				handleRect = NSMakeRect(drawnBoundsOrigin.x + NSWidth(drawnFrame) - 1 - kResizeHandleInset,
										drawnBoundsOrigin.y + CGFLOAT_DIV_2(NSHeight(drawnFrame) - kResizeHandleLength),
										1.0/* arbitrary nonzero width */, kResizeHandleLength);
				[outerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.x -= kResizeHandleGrooveSpacing;
				[middleLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.x -= kResizeHandleGrooveSpacing;
				[innerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.x -= kResizeHandleGrooveSpacing;
				[innerLine2Color setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				
				handleRect = NSMakeRect(drawnBoundsOrigin.x + kResizeHandleInset,
										drawnBoundsOrigin.y + CGFLOAT_DIV_2(NSHeight(drawnFrame) - kResizeHandleLength),
										1.0/* arbitrary nonzero width */, kResizeHandleLength);
				[outerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.x += kResizeHandleGrooveSpacing;
				[middleLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.x += kResizeHandleGrooveSpacing;
				[innerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.x += kResizeHandleGrooveSpacing;
				[innerLine2Color setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
			}
			
			if (self.allowVerticalResize)
			{
				// put handles on the top and bottom edges
				handleRect = NSMakeRect(drawnBoundsOrigin.x + CGFLOAT_DIV_2(NSWidth(drawnFrame) - kResizeHandleLength),
										drawnBoundsOrigin.y + NSHeight(drawnFrame) - 1 - kResizeHandleInset,
										kResizeHandleLength, 1.0/* arbitrary nonzero height */);
				[outerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.y -= kResizeHandleGrooveSpacing;
				[middleLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.y -= kResizeHandleGrooveSpacing;
				[innerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.y -= kResizeHandleGrooveSpacing;
				[innerLine2Color setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				
				handleRect = NSMakeRect(drawnBoundsOrigin.x + CGFLOAT_DIV_2(NSWidth(drawnFrame) - kResizeHandleLength),
										drawnBoundsOrigin.y + kResizeHandleInset,
										kResizeHandleLength, 1.0/* arbitrary nonzero height */);
				[outerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.y += kResizeHandleGrooveSpacing;
				[middleLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.y += kResizeHandleGrooveSpacing;
				[innerLineColor setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
				handleRect.origin.y += kResizeHandleGrooveSpacing;
				[innerLine2Color setFill];
				NSFrameRectWithWidth(handleRect, kResizeHandleThickness);
			}
			
			if ((self.allowHorizontalResize) && (self.allowVerticalResize))
			{
				// when both axes allow resizing, put handles on the corners too
				CGFloat const	kTransformWidth = NSWidth(drawnFrame);
				CGFloat const	kTransformHeight = NSHeight(drawnFrame);
				CGContextRef	drawingContext = REINTERPRET_CAST([[NSGraphicsContext currentContext] graphicsPort],
																	CGContextRef);
				NSColor*		targetColor = nil; // reused below
				NSColor*		outerLineColorRGB = [outerLineColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
				NSColor*		middleLineColorRGB = [middleLineColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
				NSColor*		innerLineColorRGB = [innerLineColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
				NSColor*		innerLine2ColorRGB = [innerLine2Color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
				
				
				// to draw each corner equally, “repeat” a lower-left drawing
				// but each time transform the context in a different way
				for (UInt16 i = 0; i < 4; ++i)
				{
					CGFloat		x0 = 0; // reused below
					CGFloat		x1 = 0; // reused below
					CGFloat		y0 = 0; // reused below
					CGFloat		y1 = 0; // reused below
					
					
					// WARNING: the order matters; each iteration is building
					// on the transformation done by the previous iteration
					switch (i)
					{
					case 0:
						// initial position
						CGContextTranslateCTM(drawingContext, drawnBoundsOrigin.x, drawnBoundsOrigin.y);
						break;
					
					case 1:
						// lower-left to lower-right
						CGContextTranslateCTM(drawingContext, kTransformWidth, 0);
						CGContextScaleCTM(drawingContext, -1, 1);
						break;
					
					case 2:
						// lower-right to upper-right
						CGContextTranslateCTM(drawingContext, 0, kTransformHeight);
						CGContextScaleCTM(drawingContext, 1, -1);
						break;
					
					case 3:
						// upper-right to upper-left
						CGContextTranslateCTM(drawingContext, kTransformWidth, 0);
						CGContextScaleCTM(drawingContext, -1, 1);
						break;
					
					default:
						// ???
						// should only be 4 iterations
						break;
					}
					
					CGContextBeginPath(drawingContext);
					// initial offsets roughly determine the visible length of diagonal grooves
					x0 = -3.5;
					y0 = 7.0;
					x1 = 7.0;
					y1 = -3.5;
					targetColor = outerLineColorRGB;
					CGContextSetRGBStrokeColor(drawingContext,
												[targetColor redComponent],
												[targetColor greenComponent],
												[targetColor blueComponent],
												[targetColor alphaComponent]);
					CGContextMoveToPoint(drawingContext, x0, y0);
					CGContextAddLineToPoint(drawingContext, x1, y1);
					CGContextStrokePath(drawingContext);
					x0 += kResizeHandleGrooveSpacing;
					y0 += kResizeHandleGrooveSpacing;
					x1 += kResizeHandleGrooveSpacing;
					y1 += kResizeHandleGrooveSpacing;
					targetColor = middleLineColorRGB;
					CGContextSetRGBStrokeColor(drawingContext,
												[targetColor redComponent],
												[targetColor greenComponent],
												[targetColor blueComponent],
												[targetColor alphaComponent]);
					CGContextMoveToPoint(drawingContext, x0, y0);
					CGContextAddLineToPoint(drawingContext, x1, y1);
					CGContextStrokePath(drawingContext);
					x0 += kResizeHandleGrooveSpacing;
					y0 += kResizeHandleGrooveSpacing;
					x1 += kResizeHandleGrooveSpacing;
					y1 += kResizeHandleGrooveSpacing;
					targetColor = innerLineColorRGB;
					CGContextSetRGBStrokeColor(drawingContext,
												[targetColor redComponent], [targetColor greenComponent],
												[targetColor blueComponent],
												[targetColor alphaComponent]);
					CGContextMoveToPoint(drawingContext, x0, y0);
					CGContextAddLineToPoint(drawingContext, x1, y1);
					CGContextStrokePath(drawingContext);
					x0 += kResizeHandleGrooveSpacing;
					y0 += kResizeHandleGrooveSpacing;
					x1 += kResizeHandleGrooveSpacing;
					y1 += kResizeHandleGrooveSpacing;
					targetColor = innerLine2ColorRGB;
					CGContextSetRGBStrokeColor(drawingContext,
												[targetColor redComponent],
												[targetColor greenComponent],
												[targetColor blueComponent],
												[targetColor alphaComponent]);
					CGContextMoveToPoint(drawingContext, x0, y0);
					CGContextAddLineToPoint(drawingContext, x1, y1);
					CGContextStrokePath(drawingContext);
				}
			}
		}
	}
	[NSGraphicsContext restoreGraphicsState];
	
	// for debugging: show the entire frame’s actual rectangle (this
	// must be in a separate save/restore bracket to ensure that
	// clipping and line-drawing is correct)
	if (kDebug)
	{
		NSRect		localRect = NSMakeRect(0, 0, NSWidth(self.frame), NSHeight(self.frame));
		
		
		[NSGraphicsContext saveGraphicsState];
		NSRectClip(localRect);
		[[NSColor redColor] setFill];
		NSFrameRectWithWidth(localRect, 3.0/* arbitrary */);
		[NSGraphicsContext restoreGraphicsState];
	}
	
	[patternImage unlockFocus];
	
	result = [NSColor colorWithPatternImage:patternImage];
	
	[patternImage release];
	
	return result;
}// backgroundFrameImageAsColor


/*!
Returns a path describing the window frame, taking into account
the triangular arrow head’s visibility and attributes.

(1.0)
*/
- (NSBezierPath*)
backgroundPath
{
	CGFloat const	kScaleFactor = 1.0;
	CGFloat const	kScaledArrowWidth = self.arrowBaseWidth * kScaleFactor;
	CGFloat const	kHalfScaledArrowWidth = kScaledArrowWidth / 2.0f;
	CGFloat const	kScaledCornerRadius = (self.cornerRadius * kScaleFactor);
	NSRect const	kContentArea = [self contentFrame];
	CGFloat const	kMidX = NSMidX(kContentArea) * kScaleFactor;
	CGFloat const	kMidY = NSMidY(kContentArea) * kScaleFactor;
	CGFloat const	kMinX = ceilf(NSMinX(kContentArea) * kScaleFactor + 0.5f);
	CGFloat const	kMaxX = floorf(NSMaxX(kContentArea) * kScaleFactor - 0.5f);
	CGFloat const	kMinY = ceilf(NSMinY(kContentArea) * kScaleFactor + 0.5f);
	CGFloat const	kMaxY = floorf(NSMaxY(kContentArea) * kScaleFactor - 0.5f);
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
				((kPopover_PositionBottomRight != self.windowPropertyFlags) &&
					(kPopover_PositionRightBottom != self.windowPropertyFlags))))
		{
			startingPoint.x += kScaledCornerRadius;
		}
		
		endOfLine = NSMakePoint(kMaxX, kMaxY);
		if ((isRoundedCorner) &&
			((self.hasRoundCornerBesideArrow) ||
				((kPopover_PositionBottomLeft != self.windowPropertyFlags) &&
					(kPopover_PositionLeftBottom != self.windowPropertyFlags))))
		{
			endOfLine.x -= kScaledCornerRadius;
			shouldDrawNextCorner = YES;
		}
		
		[result moveToPoint:startingPoint];
	}
	
	// draw arrow
	if (kPopover_PositionBottomRight == self.windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionBottom == self.windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMidX - kHalfScaledArrowWidth, kMaxY)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionBottomLeft == self.windowPropertyFlags)
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
			((kPopover_PositionTopLeft != self.windowPropertyFlags) &&
				(kPopover_PositionLeftTop != self.windowPropertyFlags))))
	{
		endOfLine.y += kScaledCornerRadius;
		shouldDrawNextCorner = YES;
	}
	
	// draw arrow
	if (kPopover_PositionLeftBottom == self.windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionLeft == self.windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMaxX, kMidY + kHalfScaledArrowWidth)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionLeftTop == self.windowPropertyFlags)
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
			((kPopover_PositionTopRight != self.windowPropertyFlags) &&
				(kPopover_PositionRightTop != self.windowPropertyFlags))))
	{
		endOfLine.x += kScaledCornerRadius;
		shouldDrawNextCorner = YES;
	}
	
	// draw arrow
	if (kPopover_PositionTopLeft == self.windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionTop == self.windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMidX + kHalfScaledArrowWidth, kMinY)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionTopRight == self.windowPropertyFlags)
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
			((kPopover_PositionRightBottom != self.windowPropertyFlags) &&
				(kPopover_PositionBottomRight != self.windowPropertyFlags))))
	{
		endOfLine.y -= kScaledCornerRadius;
		shouldDrawNextCorner = YES;
	}
	
	// draw arrow
	if (kPopover_PositionRightTop == self.windowPropertyFlags)
	{
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionRight == self.windowPropertyFlags)
	{
		[result lineToPoint:NSMakePoint(kMinX, kMidY - kHalfScaledArrowWidth)];
		[self appendArrowToPath:result];
	}
	else if (kPopover_PositionRightBottom == self.windowPropertyFlags)
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
}// backgroundPath


/*!
Returns the region, relative to the content view, used to
detect resizing from the indicated edge of the window.

If the window is not vertically resizable, this is empty.

(2017.05)
*/
- (NSRect)
contentEdgeFrameBottom
{
	NSRect const	kBaseFrame = [self contentFrame];
	NSRect			result = NSMakeRect(0, 0, NSWidth(kBaseFrame), kMy_ResizeClickWidth);
	
	
	if (NO == self.allowVerticalResize)
	{
		result.size.height = 0;
	}
	
	return result;
}// contentEdgeFrameBottom


/*!
Returns the region, relative to the content view, used to
detect resizing from the indicated edge of the window.

If the window is not horizontally resizable, this is empty.

(2017.05)
*/
- (NSRect)
contentEdgeFrameLeft
{
	NSRect const	kBaseFrame = [self contentFrame];
	NSRect			result = NSMakeRect(0, 0, kMy_ResizeClickWidth, NSHeight(kBaseFrame));
	
	
	if (NO == self.allowHorizontalResize)
	{
		result.size.width = 0;
	}
	
	return result;
}// contentEdgeFrameLeft


/*!
Returns the region, relative to the content view, used to
detect resizing from the indicated edge of the window.

If the window is not horizontally resizable, this is empty.

(2017.05)
*/
- (NSRect)
contentEdgeFrameRight
{
	NSRect const	kBaseFrame = [self contentFrame];
	NSRect			result = NSMakeRect(NSWidth(kBaseFrame) - kMy_ResizeClickWidth, 0,
										kMy_ResizeClickWidth, NSHeight(kBaseFrame));
	
	
	if (NO == self.allowHorizontalResize)
	{
		result.size.width = 0;
	}
	
	return result;
}// contentEdgeFrameRight


/*!
Returns the region, relative to the content view, used to
detect resizing from the indicated edge of the window.

If the window is not vertically resizable, this is empty.

(2017.05)
*/
- (NSRect)
contentEdgeFrameTop
{
	NSRect const	kBaseFrame = [self contentFrame];
	NSRect			result = NSMakeRect(0, NSHeight(kBaseFrame) - kMy_ResizeClickWidth,
										NSWidth(kBaseFrame), kMy_ResizeClickWidth);
	
	
	if (NO == self.allowVerticalResize)
	{
		result.size.height = 0;
	}
	
	return result;
}// contentEdgeFrameTop


/*!
Returns the frame that the content view should have, relative to
its parent (the window).  This subtracts out the space occupied
by the arrow.

(2017.06)
*/
- (NSRect)
contentFrame
{
	return [self.class contentRectForFrameRect:NSMakeRect(0, 0, NSWidth(self.frame), NSHeight(self.frame))
												arrowHeight:self.arrowHeight
												side:self.windowPropertyFlags];
}// contentFrame


/*!
Convenience cast for "contentView" property (at least in the
current SDK, this helps).

(2017.06)
*/
- (NSView*)
contentViewAsNSView
{
	return STATIC_CAST(self.contentView, NSView*);
}// contentViewAsNSView


/*!
Returns the effective view margins for all edges, which includes
the "self.viewMargin" property (leaving room for the border) and
any internal padding that is added.  This does not include the
space occupied by the arrow, if any.  The extra constant space
is meant to leave room for rendering resize handles, where
necessary.  It also appears as a partially-transparent border.

Note that a margin is for ONE side only; the width of the total
margin applies to either the left or right side, and the height
of the total margin applies to either the top or bottom side.
Therefore, if you want to combine this value with something like
the width of a content view, you must double the appropriate
margin measurement to obtain the final distance.

(2017.05)
*/
- (NSSize)
cornerMargins
{
	// the extra padding (appearing as pure transparent background)
	// is somewhat arbitrary but it does need to be enough space for
	// resize handles to be visible in resizable windows
	NSSize		result = NSMakeSize(self.viewMargin + 2, self.viewMargin + 2);
	
	
	return result;
}// cornerMargins


/*!
Returns the window placement portion of this popover’s properties.

(1.1)
*/
- (Popover_Properties)
windowPlacement
{
	return [self.class windowPlacement:self.windowPropertyFlags];
}// windowPlacement


@end //} Popover_Window (Popover_WindowInternal)


#pragma mark Internal Methods
namespace {

/*!
Short-cut for painfully verbose Objective-C color construction.

(2018.09)
*/
NSColor*
colorRGBA	(CGFloat	inRed,
			 CGFloat	inGreen,
			 CGFloat	inBlue,
			 CGFloat	inAlpha)
{
	return [NSColor colorWithCalibratedRed:inRed green:inGreen blue:inBlue alpha:inAlpha];
}// colorRGBA

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
