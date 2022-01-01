/*!	\file CoreUI.objc++.h
	\brief Implements base user interface elements.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
@import Cocoa;



#pragma mark Types

/*!
This protocol helps to work around some basic view layout
tasks that seem like they should be easier to do when
using an NSViewController in pre-auto-layout arrangements.

If an NSViewController subclass implements this protocol
and the corresponding NSView subclass stores the reference
to the view controller only in terms of the delegate, the
NSView method "resizeSubviewsWithOldSize:" can simply call
the delegate (i.e. the view controller).  The controller
already has to keep track of child controllers anyway and
it may want to further delegate layout responsibilities.

The convenience view class "CoreUI_LayoutView" contains a
"layoutDelegate" property and a default implementation of
"resizeSubviewsWithOldSize:" to call the delegate.
*/
@protocol CoreUI_ViewLayoutDelegate < NSObject > //{

@required

	// typically an NSView implements "resizeSubviewsWithOldSize:"
	// by invoking this method on a stored delegate object
	- (void)
	layoutDelegateForView:(NSView*)_
	resizeSubviewsWithOldSize:(NSSize)_;

@end //}


/*!
Custom subclass for action buttons in order to provide
a way to customize behavior when needed.

This customizes the focus-ring layout and rendering to
correct display glitches in layer-backed hierarchies.
(Theoretically these can be fixed by adopting later
SDKs but for now this is the only way forward.)

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_Button : NSButton //{

// NSView
	- (void)
	drawFocusRingMask;
	- (void)
	drawRect:(NSRect)_;
	- (NSRect)
	focusRingMaskBounds;

@end //}


/*!
Custom subclass for color wells in order to provide
a way to customize behavior when needed.

This customizes the focus-ring layout and rendering to
correct display glitches in layer-backed hierarchies.
(Theoretically these can be fixed by adopting later
SDKs but for now this is the only way forward.)

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_ColorButton : NSColorWell //{

// NSView
	- (void)
	drawFocusRingMask;
	- (void)
	drawRect:(NSRect)_;
	- (NSRect)
	focusRingMaskBounds;

@end //}


/*!
Custom subclass for help buttons in order to provide
a way to customize behavior when needed.

This customizes the focus-ring layout and rendering to
correct display glitches in layer-backed hierarchies.
(Theoretically these can be fixed by adopting later
SDKs but for now this is the only way forward.)

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_HelpButton : NSButton //{

// NSView
	- (void)
	drawFocusRingMask;
	- (void)
	drawRect:(NSRect)_;
	- (NSRect)
	focusRingMaskBounds;

@end //}


/*!
This implements view layout in terms of a delegate.

If an NSViewController subclass primarily exists to
manage other view controllers as children, it can
set CoreUI_LayoutView as its view’s class and then
act as the delegate for the layout of subviews.
Since it already knows the child view controllers,
it makes more sense for the controller to be told
when to do layout instead of expecting the view to
redundantly keep track of these things, especially
if the layout may rely on state that could only be
known to the controller.

In later SDKs with auto-layout, it may be possible
to use the view controller for constraints.  Until
then, this is a viable alternative.
*/
@interface CoreUI_LayoutView : NSView //{

// accessors
	@property (assign) id < CoreUI_ViewLayoutDelegate >
	layoutDelegate;

@end //}


/*!
Custom subclass for menu buttons in order to provide
a way to customize behavior when needed.

This customizes the focus-ring layout and rendering to
correct display glitches in layer-backed hierarchies.
(Theoretically these can be fixed by adopting later
SDKs but for now this is the only way forward.)

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_MenuButton : NSPopUpButton //{

// NSView
	- (void)
	drawFocusRingMask;
	- (void)
	drawRect:(NSRect)_;
	- (NSRect)
	focusRingMaskBounds;

@end //}


/*!
Custom subclass for scroll views in order to provide
a way to customize behavior when needed.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_ScrollView : NSScrollView //{

@end //}


/*!
Custom subclass for square buttons in order to provide
a way to customize behavior when needed.

This customizes the focus-ring layout and rendering to
correct display glitches in layer-backed hierarchies.
(Theoretically these can be fixed by adopting later
SDKs but for now this is the only way forward.)

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_SquareButton : NSButton //{

// NSView
	- (void)
	drawFocusRingMask;
	- (void)
	drawRect:(NSRect)_;
	- (NSRect)
	focusRingMaskBounds;

@end //}


/*!
Custom subclass for square button cells in order to
provide a way to customize behavior when needed.

This customizes the focus-ring layout and rendering to
correct display glitches in layer-backed hierarchies.
(Theoretically these can be fixed by adopting later
SDKs but for now this is the only way forward.)

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_SquareButtonCell : NSButtonCell //{

// NSCell
	- (void)
	drawFocusRingMaskWithFrame:(NSRect)_
	inView:(NSView*)_;
	- (NSRect)
	focusRingMaskBoundsForFrame:(NSRect)_
	inView:(NSView*)_;

@end //}


/*!
Custom subclass for table views in order to provide
a way to customize behavior when needed.

This customizes the focus-ring layout and rendering to
correct display glitches in layer-backed hierarchies.
(Theoretically these can be fixed by adopting later
SDKs but for now this is the only way forward.)

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CoreUI_Table : NSTableView //{

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
