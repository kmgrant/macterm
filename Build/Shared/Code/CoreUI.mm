/*!	\file CoreUI.mm
	\brief Implements base user interface elements.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#import "CoreUI.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>



#pragma mark Public Methods

#pragma mark -
@implementation CoreUI_Button //{


#pragma mark NSView


/*!
Invoked by the OS to find the boundaries of the focus ring,
and by "drawRect:" for a transition period to render the
focus ring while depending on an older SDK.

NOTE:	Invoked by the system only on OS 10.7 or later, and
		less often than one might think.

(2016.10)
*/
- (void)
drawFocusRingMask
{
	// IMPORTANT: make consistent with "focusRingMaskBounds";
	// fill the exact shape of the focus ring however (not just
	// the boundaries of it); the settings below are meant to
	// match the current system appearance/layout (as such, in
	// the future they may require adjustment)
	NSRect		focusRingDrawingBounds = [self focusRingMaskBounds];
	
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithRoundedRect:focusRingDrawingBounds xRadius:3 yRadius:3] fill];
}// drawFocusRingMask


/*!
Corrects the appearance of focus rings in views on top
of visual-effect views or other layer-backed views.  If
the view has the keyboard focus (first responder), the
superclass "drawRect:" is called without focus and
then "drawFocusRingMask" is called with a ring-only
style in place.

(2016.10)
*/
- (void)
drawRect:(NSRect)	aRect
{
	if (NO == [self isKeyboardFocusOnSelf])
	{
		// normal case; superclass handles this fine
		[super drawRect:aRect];
	}
	else
	{
		// temporarily resign first responder to “trick” the
		// base class into not rendering a focus ring…
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:nil];
		[super drawRect:aRect];
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:self];
		
		// now draw the focus ring in the right place
		// (TEMPORARY, hopefully; linking to future SDK may
		// allow the button to be drawn correctly)
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMask];
		[NSGraphicsContext restoreGraphicsState];
	}
}// drawRect:


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked only on OS 10.7 or later, and less often
		than one might think.

(2016.10)
*/
- (NSRect)
focusRingMaskBounds
{
	// IMPORTANT: make consistent with "drawFocusRingMask"
	NSRect		result = NSInsetRect(self.bounds, 7, 7);
	
	
	// button frames are weird; they occupy a space much larger
	// than the rounded-rectangle of the button itself and not
	// even perfectly-centered within that space…
	result.origin.y -= 2; // meant to match system layout
	result.size.height += 1; // meant to match system layout
	
	return result;
}// focusRingMaskBounds


@end //}


#pragma mark -
@implementation CoreUI_ColorButton //{


#pragma mark NSView


/*!
Invoked by the OS to find the boundaries of the focus ring,
and by "drawRect:" for a transition period to render the
focus ring while depending on an older SDK.

NOTE:	Invoked by the system only on OS 10.7 or later, and
		less often than one might think.

(2016.10)
*/
- (void)
drawFocusRingMask
{
	// IMPORTANT: make consistent with "focusRingMaskBounds";
	// fill the exact shape of the focus ring however (not just
	// the boundaries of it); the settings below are meant to
	// match the current system appearance/layout (as such, in
	// the future they may require adjustment)
	NSRect		focusRingDrawingBounds = [self focusRingMaskBounds];
	
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithRect:focusRingDrawingBounds] fill];
}// drawFocusRingMask


/*!
Corrects the appearance of focus rings in views on top
of visual-effect views or other layer-backed views.  If
the view has the keyboard focus (first responder), the
superclass "drawRect:" is called without focus and
then "drawFocusRingMask" is called with a ring-only
style in place.

(2016.10)
*/
- (void)
drawRect:(NSRect)	aRect
{
	if (NO == [self isKeyboardFocusOnSelf])
	{
		// normal case; superclass handles this fine
		[super drawRect:aRect];
	}
	else
	{
		// temporarily resign first responder to “trick” the
		// base class into not rendering a focus ring…
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:nil];
		[super drawRect:aRect];
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:self];
		
		// now draw the focus ring in the right place
		// (TEMPORARY, hopefully; linking to future SDK may
		// allow the button to be drawn correctly)
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMask];
		[NSGraphicsContext restoreGraphicsState];
	}
}// drawRect:


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked only on OS 10.7 or later, and less often
		than one might think.

(2016.10)
*/
- (NSRect)
focusRingMaskBounds
{
	// IMPORTANT: make consistent with "drawFocusRingMask"
	NSRect		result = NSInsetRect(self.bounds, 5, 5);
	
	
	return result;
}// focusRingMaskBounds


@end //}


#pragma mark -
@implementation CoreUI_HelpButton //{


#pragma mark NSView


/*!
Invoked by the OS to find the boundaries of the focus ring,
and by "drawRect:" for a transition period to render the
focus ring while depending on an older SDK.

NOTE:	Invoked by the system only on OS 10.7 or later, and
		less often than one might think.

(2016.10)
*/
- (void)
drawFocusRingMask
{
	// IMPORTANT: make consistent with "focusRingMaskBounds";
	// fill the exact shape of the focus ring however (not just
	// the boundaries of it); the settings below are meant to
	// match the current system appearance/layout (as such, in
	// the future they may require adjustment)
	NSRect		focusRingDrawingBounds = [self focusRingMaskBounds];
	
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithOvalInRect:focusRingDrawingBounds] fill];
}// drawFocusRingMask


/*!
Corrects the appearance of focus rings in views on top
of visual-effect views or other layer-backed views.  If
the view has the keyboard focus (first responder), the
superclass "drawRect:" is called without focus and
then "drawFocusRingMask" is called with a ring-only
style in place.

(2016.10)
*/
- (void)
drawRect:(NSRect)	aRect
{
	if (NO == [self isKeyboardFocusOnSelf])
	{
		// normal case; superclass handles this fine
		[super drawRect:aRect];
	}
	else
	{
		// temporarily resign first responder to “trick” the
		// base class into not rendering a focus ring…
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:nil];
		[super drawRect:aRect];
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:self];
		
		// now draw the focus ring in the right place
		// (TEMPORARY, hopefully; linking to future SDK may
		// allow the button to be drawn correctly)
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMask];
		[NSGraphicsContext restoreGraphicsState];
	}
}// drawRect:


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked only on OS 10.7 or later, and less often
		than one might think.

(2016.10)
*/
- (NSRect)
focusRingMaskBounds
{
	// IMPORTANT: make consistent with "drawFocusRingMask"
	NSRect		result = NSInsetRect(self.bounds, 5, 5);
	
	
	// adjust to match help button as drawn by superclass
	result.origin.y -= 2; // shift up for Mojave
	
	return result;
}// focusRingMaskBounds


@end //}


#pragma mark -
@implementation CoreUI_LayoutView //{


#pragma mark Externally-Declared Properties


/*!
This should be set to the view controller (implementing
the protocol), allowing NSView layout to delegate to the
view controller.
*/
@synthesize layoutDelegate = _layoutDelegate;


#pragma mark NSView


/*!
Lays out subviews in response to a size change.

(2018.04)
*/
- (void)
resizeSubviewsWithOldSize:(NSSize)		oldSize
{
	if (nil == self.layoutDelegate)
	{
		Console_Warning(Console_WriteLine, "CoreUI_LayoutView layout requested when no delegate is set");
	}
	else
	{
		[self.layoutDelegate layoutDelegateForView:self
													resizeSubviewsWithOldSize:oldSize];
	}
}// resizeSubviewsWithOldSize:


@end //} CoreUI_LayoutView


#pragma mark -
@implementation CoreUI_MenuButton //{


#pragma mark NSView


/*!
Invoked by the OS to find the boundaries of the focus ring,
and by "drawRect:" for a transition period to render the
focus ring while depending on an older SDK.

NOTE:	Invoked by the system only on OS 10.7 or later, and
		less often than one might think.

(2016.10)
*/
- (void)
drawFocusRingMask
{
	// IMPORTANT: make consistent with "focusRingMaskBounds";
	// fill the exact shape of the focus ring however (not just
	// the boundaries of it); the settings below are meant to
	// match the current system appearance/layout (as such, in
	// the future they may require adjustment)
	NSRect		focusRingDrawingBounds = [self focusRingMaskBounds];
	
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithRoundedRect:focusRingDrawingBounds xRadius:3 yRadius:3] fill];
}// drawFocusRingMask


/*!
Corrects the appearance of focus rings in views on top
of visual-effect views or other layer-backed views.  If
the view has the keyboard focus (first responder), the
superclass "drawRect:" is called without focus and
then "drawFocusRingMask" is called with a ring-only
style in place.

(2016.10)
*/
- (void)
drawRect:(NSRect)	aRect
{
	if (NO == [self isKeyboardFocusOnSelf])
	{
		// normal case; superclass handles this fine
		[super drawRect:aRect];
	}
	else
	{
		// temporarily resign first responder to “trick” the
		// base class into not rendering a focus ring…
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:nil];
		[super drawRect:aRect];
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:self];
		
		// now draw the focus ring in the right place
		// (TEMPORARY, hopefully; linking to future SDK may
		// allow the button to be drawn correctly)
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMask];
		[NSGraphicsContext restoreGraphicsState];
	}
}// drawRect:


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked only on OS 10.7 or later, and less often
		than one might think.

(2016.10)
*/
- (NSRect)
focusRingMaskBounds
{
	// IMPORTANT: make consistent with "drawFocusRingMask"
	NSRect		result = NSInsetRect(self.bounds, 3, 3);
	
	
	// button frames are weird; they occupy a space much larger
	// than the rounded-rectangle of the button itself and not
	// even perfectly-centered within that space…
	result.origin.y -= 0.5; // meant to match system layout
	result.size.width -= 1; // meant to match system layout
	result.size.height -= 1; // meant to match system layout
	
	return result;
}// focusRingMaskBounds


@end //}


#pragma mark -
@implementation CoreUI_ScrollView //{


@end //}


#pragma mark -
@implementation CoreUI_SquareButton //{


#pragma mark NSView


/*!
Invoked by the OS to find the boundaries of the focus ring,
and by "drawRect:" for a transition period to render the
focus ring while depending on an older SDK.

NOTE:	Invoked by the system only on OS 10.7 or later, and
		less often than one might think.

(2016.10)
*/
- (void)
drawFocusRingMask
{
	// IMPORTANT: make consistent with "focusRingMaskBounds";
	// fill the exact shape of the focus ring however (not just
	// the boundaries of it); the settings below are meant to
	// match the current system appearance/layout (as such, in
	// the future they may require adjustment)
	NSRect		focusRingDrawingBounds = [self focusRingMaskBounds];
	
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithRect:focusRingDrawingBounds] fill];
}// drawFocusRingMask


/*!
Corrects the appearance of focus rings in views on top
of visual-effect views or other layer-backed views.  If
the view has the keyboard focus (first responder), the
superclass "drawRect:" is called without focus and
then "drawFocusRingMask" is called with a ring-only
style in place.

(2016.10)
*/
- (void)
drawRect:(NSRect)	aRect
{
	if (NO == [self isKeyboardFocusOnSelf])
	{
		// normal case; superclass handles this fine
		[super drawRect:aRect];
	}
	else
	{
		// temporarily resign first responder to “trick” the
		// base class into not rendering a focus ring…
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:nil];
		[super drawRect:aRect];
		UNUSED_RETURN(BOOL)[self.window makeFirstResponder:self];
		
		// now draw the focus ring in the right place
		// (TEMPORARY, hopefully; linking to future SDK may
		// allow the button to be drawn correctly)
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMask];
		[NSGraphicsContext restoreGraphicsState];
	}
}// drawRect:


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked only on OS 10.7 or later, and less often
		than one might think.

(2016.10)
*/
- (NSRect)
focusRingMaskBounds
{
	// IMPORTANT: make consistent with "drawFocusRingMask"
	NSRect		result = NSInsetRect(self.bounds, 3, 3);
	
	
	return result;
}// focusRingMaskBounds


@end //}


#pragma mark -
@implementation CoreUI_SquareButtonCell //{


#pragma mark NSCell


/*!
Invoked by the OS to find the boundaries of the focus ring,
and by "drawWithFrame:inView:" for a transition period to
render the focus ring while depending on an older SDK.

NOTE:	Invoked by the system only on OS 10.7 or later, and
		less often than one might think.

(2017.03)
*/
- (void)
drawFocusRingMaskWithFrame:(NSRect)		aCellFrame
inView:(NSView*)						aControlView
{
	// IMPORTANT: agree with "focusRingMaskBoundsForFrame:inView";
	// fill the exact shape of the focus ring however (not just
	// the boundaries of it); the settings below are meant to
	// match the current system appearance/layout (as such, in
	// the future they may require adjustment)
	NSRect		focusRingDrawingBounds = [self focusRingMaskBoundsForFrame:aCellFrame inView:aControlView];
	
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithRect:focusRingDrawingBounds] fill];
}// drawFocusRingMaskWithFrame:inView:


/*!
Corrects the appearance of focus rings in views on top
of visual-effect views or other layer-backed views.  If
the view has the keyboard focus (first responder), the
superclass "drawWithFrame:inView:" is called without
focus and then "drawFocusRingMaskWithFrame:inView:" is
called with a ring-only style in place.

(2017.03)
*/
- (void)
drawWithFrame:(NSRect)		aCellFrame
inView:(NSView*)			aControlView
{
	if (NO == [self showsFirstResponder])
	{
		// normal case; superclass handles this fine
		[super drawWithFrame:aCellFrame inView:aControlView];
	}
	else
	{
		// temporarily turn off display of first-responder to “trick”
		// the base class into not rendering a focus ring…
		[self setShowsFirstResponder:NO];
		[super drawWithFrame:aCellFrame inView:aControlView];
		[self setShowsFirstResponder:YES];
		
		// now draw the focus ring in the right place
		// (TEMPORARY, hopefully; linking to future SDK may
		// allow the cell to be drawn correctly)
		[NSGraphicsContext saveGraphicsState];
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMaskWithFrame:aCellFrame inView:aControlView];
		[NSGraphicsContext restoreGraphicsState];
	}
}// drawWithFrame:inView:


/*!
Invoked by the OS to find the boundaries of the focus ring.

NOTE:	Invoked only on OS 10.7 or later, and less often
		than one might think.

(2017.03)
*/
- (NSRect)
focusRingMaskBoundsForFrame:(NSRect)		aCellFrame
inView:(NSView*)						aControlView
{
#pragma unused(aControlView)
	// IMPORTANT: make consistent with "drawFocusRingMaskWithFrame:inView:"
	NSRect		result = NSInsetRect(aCellFrame, 3, 3);
	
	
	return result;
}// focusRingMaskBoundsForFrame:inView:


@end //}


#pragma mark -
@implementation CoreUI_Table //{


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
