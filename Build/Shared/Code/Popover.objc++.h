/*!	\file Popover.objc++.h
	\brief Implements a popover-style window.
	
	Unlike popovers in Lion, these function on many older
	versions of Mac OS X (tested back to Mac OS X 10.3).
	They are also more flexible, allowing different colors
	for instance.
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaAnimation.h>


#pragma mark Constants

/*!
Use these with "setStandardPropertiesForArrowStyle:" to
create the specified standard arrow appearance.  See that
method for more information on the affected properties.

If you do not want an arrow at all, set the style to
"kPopover_ArrowStyleNone".
*/
typedef enum
{
	kPopover_ArrowStyleNone					= 0,
	kPopover_ArrowStyleDefaultRegularSize	= 1,
	kPopover_ArrowStyleDefaultSmallSize		= 2,
	kPopover_ArrowStyleDefaultMiniSize		= 3
} Popover_ArrowStyle;


/*!
Window properties of the same type occupy the same bit range, but
unrelated properties are in different ranges; they can therefore be
combined.  For example, a window can be positioned to the left of
an arrow that is in the center position.

The "kPopover_PropertyArrow..." constants determine where the arrow
appears along its edge.  The “beginning” is relative to the top-left
corner of the window so an arrow pointing vertically on the rightmost
part of the top or bottom window edge would be at the “end” of its
edge (as would an arrow pointing horizontally on the bottommost part
of the left or right window edges).

The "kPopover_PlaceFrame..." constants determine where the window
frame appears to be from the user’s point of view relative to the
arrow (in reality the window occupies a bigger frame).
*/
typedef unsigned int Popover_Properties;
enum
{
	kPopover_PropertyShiftArrow					= 0, // shift only (invalid value)
	kPopover_PropertyMaskArrow					= (0x03 << kPopover_PropertyShiftArrow), // mask only (invalid value)
	kPopover_PropertyArrowMiddle				= (0x00 << kPopover_PropertyShiftArrow),
	kPopover_PropertyArrowBeginning				= (0x01 << kPopover_PropertyShiftArrow),
	kPopover_PropertyArrowEnd					= (0x02 << kPopover_PropertyShiftArrow),
	
	kPopover_PropertyShiftPlaceFrame			= 2, // shift only (invalid value)
	kPopover_PropertyMaskPlaceFrame				= (0x03 << kPopover_PropertyShiftPlaceFrame), // mask only (invalid value)
	kPopover_PropertyPlaceFrameBelowArrow		= (0x00 << kPopover_PropertyShiftPlaceFrame),
	kPopover_PropertyPlaceFrameLeftOfArrow		= (0x01 << kPopover_PropertyShiftPlaceFrame),
	kPopover_PropertyPlaceFrameRightOfArrow		= (0x02 << kPopover_PropertyShiftPlaceFrame),
	kPopover_PropertyPlaceFrameAboveArrow		= (0x03 << kPopover_PropertyShiftPlaceFrame)
};


/*!
DEPRECATED.  Use window properties individually.

Popover window positions are relative to the point passed to the
constructor, e.g. kPopover_PositionBottomRight will put the window
below the point and towards the right, kPopover_PositionTop will
horizontally center it above the point, kPopover_PositionRightTop
will put the window to the right and above the point, and so on.

Note that it is also possible to request automatic positioning using
"setPointWithAutomaticPositioning:preferredSide:".  If that is used
then the window is given the “best” possible position but the given
preferred side is used if that side is tied with any other candidate.
*/
enum
{
	kPopover_PositionBottom			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow,
	kPopover_PositionBottomRight	= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameBelowArrow,
	kPopover_PositionBottomLeft		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameBelowArrow,
	kPopover_PositionLeft			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameLeftOfArrow,
	kPopover_PositionLeftBottom		= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameLeftOfArrow,
	kPopover_PositionLeftTop		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameLeftOfArrow,
	kPopover_PositionRight			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameRightOfArrow,
	kPopover_PositionRightBottom	= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameRightOfArrow,
	kPopover_PositionRightTop		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameRightOfArrow,
	kPopover_PositionTop			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameAboveArrow,
	kPopover_PositionTopRight		= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameAboveArrow,
	kPopover_PositionTopLeft		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameAboveArrow
};

/*!
At initialization time, use one of these styles to
preset a wide variety of window properties (useful
for producing standard appearances).  They only
affect appearance and not behavior but you should
ensure that the window behavior is consistent.
*/
typedef enum
{
	kPopover_WindowStyleNormal			= 0,
	kPopover_WindowStyleHelp			= 1,
	kPopover_WindowStyleDialogAppModal	= 2,
	kPopover_WindowStyleDialogSheet		= 3,
	kPopover_WindowStyleAlertAppModal	= 4,
	kPopover_WindowStyleAlertSheet		= 5
} Popover_WindowStyle;


#pragma mark Types

@class Popover_Window;


/*!
This protocol helps the window-resizing code determine how
to arrange and constrain the window while the user is
dragging the mouse.
*/
@protocol Popover_ResizeDelegate < NSObject > //{

@optional

	// provide YES for one or both axes that should allow resizing to take place;
	// if not implemented, the assumption is that the window can resize both ways
	- (void)
	popover:(Popover_Window*)_
	getHorizontalResizeAllowed:(BOOL*)_
	getVerticalResizeAllowed:(BOOL*)_;

@end //}


/*!
A popover-style window that works on many versions of Mac OS X.

This class handles only the visual parts of a popover, not the
equally-important behavioral aspects.  To help display and manage
this window, see "PopoverManager.objc++.h".

Note that accessor methods are generally meant to configure the
window before displaying it.  The user should not normally see
the window change its appearance while it is on screen.  One
exception to this is "setHasArrow:", which will simply update the
frame appearance.
*/
@interface Popover_Window : NSWindow < CocoaAnimation_WindowImageProvider > //{
{
@private
	NSMutableArray*			_registeredObservers;
	NSWindow*				popoverParentWindow;
	NSView*					embeddedView;
	NSImage*				_animationContentImage;
	NSImage*				_animationFullImage;
	NSTrackingArea*			_trackTopEdge;
	NSTrackingArea*			_trackLeftEdge;
	NSTrackingArea*			_trackMainFrame;
	NSTrackingArea*			_trackRightEdge;
	NSTrackingArea*			_trackBottomEdge;
	NSColor*				_borderOuterColor;
	NSColor*				_borderPrimaryColor;
	NSColor*				_borderOuterDisplayColor;
	NSColor*				_borderPrimaryDisplayColor;
	NSColor*				_popoverBackgroundColor;
	id< Popover_ResizeDelegate >	_resizeDelegate;
	NSPoint					_userResizeClickScreenPoint;
	CGFloat					_userResizeOriginalCenterX;
	CGFloat					_arrowBaseWidth;
	CGFloat					_arrowHeight;
	CGFloat					_borderWidth;
	CGFloat					_cornerRadius;
	CGFloat					_viewMargin;
	BOOL					_layoutInProgress;
	BOOL					_allowHorizontalResize;
	BOOL					_allowVerticalResize;
	BOOL					_hasRoundCornerBesideArrow;
	BOOL					_isBeingResizedByUser;
	BOOL					_userResizeTop;
	BOOL					_userResizeLeft;
	BOOL					_userResizeRight;
	BOOL					_userResizeBottom;
	Popover_Properties		_windowPropertyFlags;
}

// initializers
	- (instancetype)
	initWithContentRect:(NSRect)_
#if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_6
	styleMask:(NSUInteger)_
#else
	styleMask:(NSWindowStyleMask)_
#endif
	backing:(NSBackingStoreType)_
	defer:(BOOL)_ DISABLED_SUPERCLASS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithView:(NSView*)_
	windowStyle:(Popover_WindowStyle)_
	arrowStyle:(Popover_ArrowStyle)_
	attachedToPoint:(NSPoint)_
	inWindow:(NSWindow*)_;
	- (instancetype)
	initWithView:(NSView*)_
	windowStyle:(Popover_WindowStyle)_
	arrowStyle:(Popover_ArrowStyle)_
	attachedToPoint:(NSPoint)_
	inWindow:(NSWindow*)_
	vibrancy:(BOOL)_ NS_DESIGNATED_INITIALIZER;

// new methods: utilities
	- (void)
	applyArrowStyle:(Popover_ArrowStyle)_;
	- (void)
	applyWindowStyle:(Popover_WindowStyle)_;
	- (NSRect)
	frameRectForViewSize:(NSSize)_;

// new methods: window location
	- (void)
	setPoint:(NSPoint)_
	onSide:(Popover_Properties)_;
	- (void)
	setPointWithAutomaticPositioning:(NSPoint)_
	preferredSide:(Popover_Properties)_;

// accessors: colors
	@property (copy) NSColor*
	borderOuterColor;
	@property (copy) NSColor*
	borderPrimaryColor;
	@property (copy) NSColor*
	popoverBackgroundColor;

// accessors: general
	@property (assign) CGFloat
	arrowBaseWidth;
	@property (assign) CGFloat
	arrowHeight;
	@property (assign) CGFloat
	borderWidth;
	@property (assign) CGFloat
	cornerRadius;
	@property (assign) BOOL
	hasRoundCornerBesideArrow;
	@property (readonly) BOOL
	hasArrow;
	@property (assign) id< Popover_ResizeDelegate >
	resizeDelegate;
	@property (assign) CGFloat
	viewMargin;

// NSWindow
	- (void)
	setBackgroundColor:(NSColor*)_; // DO NOT USE; RESERVED FOR RENDERING

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
