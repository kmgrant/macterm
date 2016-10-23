/*!	\file CoreUI.mm
	\brief Implements base user interface elements.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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



#pragma mark Public Methods

#pragma mark -
@implementation CoreUI_Button //{


#pragma mark Initializers


/*!
Designated initializer when archived (Interface Builder).

(2016.10)
*/
- (id)
initWithCoder:(NSCoder*)	aDecoder
{
	self = [super initWithCoder:aDecoder];
	if (nil != self)
	{
		// nothing required
	}
	return self;
}// initWithCoder:


/*!
Designated initializer when not archived.

(2016.10)
*/
- (instancetype)
initWithFrame:(NSRect)	aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		// nothing required
	}
	return self;
}// initWithFrame:


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
	NSRect		focusRingDrawingBounds = NSInsetRect(self.bounds, 7, 7);
	
	
	// button frames are weird; they occupy a space much larger
	// than the rounded-rectangle of the button itself and not
	// even perfectly-centered within that space…
	focusRingDrawingBounds.origin.y -= 2; // meant to match system layout
	focusRingDrawingBounds.size.height += 1; // meant to match system layout
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithRoundedRect:focusRingDrawingBounds xRadius:3 yRadius:3] fill];
}// drawFocusRingMask


/*!
Draws the button and any focus ring.

(2016.10)
*/
- (void)
drawRect:(NSRect)	aRect
{
	if ((NO == self.window.isKeyWindow) || (self != [self.window firstResponder]))
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
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMask];
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
	NSRect		result = self.bounds;
	
	
	return result;
}// focusRingMaskBounds


@end //}


#pragma mark -
@implementation CoreUI_HelpButton //{


#pragma mark Initializers


/*!
Designated initializer when archived (Interface Builder).

(2016.10)
*/
- (id)
initWithCoder:(NSCoder*)	aDecoder
{
	self = [super initWithCoder:aDecoder];
	if (nil != self)
	{
		// nothing required
	}
	return self;
}// initWithCoder:


/*!
Designated initializer when not archived.

(2016.10)
*/
- (instancetype)
initWithFrame:(NSRect)	aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		// nothing required
	}
	return self;
}// initWithFrame:


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
	NSRect		focusRingDrawingBounds = NSInsetRect(self.bounds, 3, 3);
	
	
	// adjust to match help button as drawn by superclass
	focusRingDrawingBounds.size.height -= 1; // meant to match system layout
	
	// based on the rules for drawing (for both NSSetFocusRingStyle()
	// and the new-style "focusRingMaskBounds"), “filling” the target
	// area will cause only a focus ring to appear
	[[NSBezierPath bezierPathWithOvalInRect:focusRingDrawingBounds] fill];
}// drawFocusRingMask


/*!
Draws the button and any focus ring.

(2016.10)
*/
- (void)
drawRect:(NSRect)	aRect
{
	if ((NO == self.window.isKeyWindow) || (self != [self.window firstResponder]))
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
		NSSetFocusRingStyle(NSFocusRingOnly);
		[self drawFocusRingMask];
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
	NSRect		result = self.bounds;
	
	
	return result;
}// focusRingMaskBounds


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
