/*!	\file Popover.objc++.h
	\brief Implements a popover-style window.
*/
/*###############################################################

	Popover Window 1.4 (based on MAAttachedWindow)
	MAAttachedWindow © 2007 by Magic Aubergine
	Popover Window © 2011-2021 by Kevin Grant
	
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
@import Cocoa;

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
	popover:(Popover_Window* _Nonnull)_
	getHorizontalResizeAllowed:(BOOL* _Nonnull)_
	getVerticalResizeAllowed:(BOOL* _Nonnull)_;

@end //}


/*!
A popover-style window that works on many versions of the OS.

This class handles only the visual parts of a popover, not the
equally-important behavioral aspects.  To help display and manage
this window, see "PopoverManager.objc++.h".

Note that accessor methods are generally meant to configure the
window before displaying it.  The user should not normally see
the window change its appearance while it is on screen.  One
exception to this is "hasArrow", which will simply update the
frame appearance.
*/
@interface Popover_Window : NSWindow < CocoaAnimation_WindowImageProvider > //{

// initializers
	- (instancetype _Nullable)
	initWithView:(NSView* _Nonnull)_
	windowStyle:(Popover_WindowStyle)_
	arrowStyle:(Popover_ArrowStyle)_
	attachedToPoint:(NSPoint)_
	inWindow:(NSWindow* _Nullable)_;
	- (instancetype _Nullable)
	initWithView:(NSView* _Nonnull)_
	windowStyle:(Popover_WindowStyle)_
	arrowStyle:(Popover_ArrowStyle)_
	attachedToPoint:(NSPoint)_
	inWindow:(NSWindow* _Nullable)_
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
	//! The outer border color is used to render the boundary of the
	//! popover window.  See also "setBorderPrimaryColor:".
	//!
	//! The border thickness is determined by "setBorderWidth:".  If
	//! the outer and primary colors are the same then the border
	//! appears to be that thickness; otherwise the width is divided
	//! roughly evenly between the two colors.
	@property (strong, nonnull) NSColor*
	borderOuterColor;
	//! The primary border color is used to render a frame inside the
	//! outer border (see "setBorderOuterColor:").
	//!
	//! The border thickness is determined by "setBorderWidth:".  If
	//! the outer and primary colors are the same then the border
	//! appears to be that thickness; otherwise the width is divided
	//! roughly evenly between the two colors.
	@property (strong, nonnull) NSColor*
	borderPrimaryColor;
	//! The popover background color is used to construct an image
	//! that the NSWindow superclass uses for rendering.
	//!
	//! Use this property instead of the NSWindow "backgroundColor"
	//! property because the normal background color is overridden
	//! to contain the entire rendering of the popover window frame
	//! (as a pattern image).
	@property (strong, nonnull) NSColor*
	popoverBackgroundColor;

// accessors: general
	//! The base width determines how “fat” the frame arrow’s triangle is.
	@property (assign) CGFloat
	arrowBaseWidth;
	//! The height determines how “slender” the frame arrow’s triangle is.
	@property (assign) CGFloat
	arrowHeight;
	//! The border is drawn inside the viewMargin area, expanding inwards;
	//! it does not increase the width/height of the window.  You can use
	//! "borderWidth" and "viewMargin" together to achieve the exact
	//! look/geometry you want.
	@property (assign) CGFloat
	borderWidth;
	//! The radius in pixels of the arc used to draw curves at the
	//! corners of the popover frame.
	@property (assign) CGFloat
	cornerRadius;
	//! When the window position puts the arrow near a corner of the
	//! frame, this specifies how close to the corner the arrow is.
	//! If a rounded corner appears then the arrow is off to the side;
	//! otherwise the arrow is right in the corner.
	@property (assign) BOOL
	hasRoundCornerBesideArrow;
	//! Specifies whether or not the frame has an arrow displayed.
	//! This is set implicitly via "setArrowStyle:" or through an
	//! initializer.
	@property (readonly) BOOL
	hasArrow;
	//! If set, this object can be queried to guide resize behavior
	//! (such as to decide that only one axis allows resizing).
	@property (assign, nullable) id< Popover_ResizeDelegate >
	resizeDelegate;
	//! The style-specified distance between the edge of the view and
	//! the window edge.  Additional space can be inserted if there
	//! are resize handles.
	@property (assign) CGFloat
	viewMargin;

// NSWindow
	- (void)
	setBackgroundColor:(NSColor* _Nullable)_; // DO NOT USE; RESERVED FOR RENDERING

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
