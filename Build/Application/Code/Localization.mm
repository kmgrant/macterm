/*!	\file Localization.mm
	\brief Helps to make this program easier to translate into
	other languages and cultures.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2022 by Kevin Grant
	
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

#import <Localization.h>
#import <UniversalDefines.h>

// standard-C includes
#import <cctype>

// Mac includes
@import Cocoa;

// library includes
#import <CFRetainRelease.h>
#import <Console.h>



#pragma mark Public Methods

/*!
Auto-arranges a window’s help button to occupy the
lower-left (for left-to-right localization) or lower-
right (for right-to-left localization) corner of a
window.

(2.7)
*/
void
Localization_AdjustHelpNSButton		(NSButton*		inHelpButton)
{
	CGFloat		xOffset = 0;
	CGFloat		yOffset = 15; // arbitrary
	CGFloat		buttonWidth = NSWidth([inHelpButton frame]);
	CGFloat		buttonHeight = NSHeight([inHelpButton frame]);
	
	
	if (@available(macOS 11.0, *))
	{
		if (NSControlSizeLarge == inHelpButton.controlSize)
		{
			yOffset = 8; // arbitrary (reduce offset because button is bigger)
		}
	}
	
	if (NSUserInterfaceLayoutDirectionRightToLeft == NSApp.userInterfaceLayoutDirection)
	{
		// in right-to-left locales, the help button is in the lower right
		NSRect		parentFrame = [[inHelpButton superview] bounds];
		
		
		xOffset = NSWidth(parentFrame) - 22/* arbitrary but according to UI guidelines */;
		xOffset -= buttonWidth;
		[inHelpButton setFrame:NSMakeRect(xOffset, yOffset, buttonWidth, buttonHeight)];
	}
	else
	{
		// in right-to-left locales, the help button is in the lower left
		xOffset = 20/* arbitrary but according to UI guidelines */;
		[inHelpButton setFrame:NSMakeRect(xOffset, yOffset, buttonWidth, buttonHeight)];
	}
}// AdjustHelpNSButton


/*!
Auto-sizes and auto-arranges a series of action buttons for
a modal window.  Buttons are assumed to be positioned on the
same line horizontally, and located at the bottom-right corner
of a modal window (or the bottom-left corner, if the locale
is right-to-left).  The first button in the given array of
buttons is expected to be the default button (if any), and
buttons are positioned in the order they appear in the array.

(2.7)
*/
void
Localization_ArrangeNSButtonArray	(CFArrayRef		inNSButtonArray)
{
	NSArray*	asNSArray = BRIDGE_CAST(inNSButtonArray, NSArray*);
	
	
	if ([asNSArray count] > 0)
	{
		NSButton*	firstButton = STATIC_CAST([asNSArray objectAtIndex:0], NSButton*);
		CGFloat		defaultYOffset = [firstButton frame].origin.y;
		CGFloat		xOffset = 0;
		CGFloat		yOffset = defaultYOffset;
		CGFloat		buttonHeight = NSHeight([firstButton frame]);
		
		
		if (NSUserInterfaceLayoutDirectionRightToLeft == NSApp.userInterfaceLayoutDirection)
		{
			// in right-to-left locales, the action buttons
			// are anchored in the bottom-left corner and
			// the default button is leftmost
			xOffset = 20.0/* arbitrary but according to UI guidelines */;
			for (NSButton* button in asNSArray)
			{
				CGFloat		buttonWidth = Localization_AutoSizeNSButton(button);
				
				
				// in Cocoa the “frame” of a button appears to also
				// include the space after the button so the offset
				// only changes by the frame width
				[button setFrame:NSMakeRect(xOffset, yOffset, buttonWidth, buttonHeight)];
				xOffset += buttonWidth;
			}
		}
		else
		{
			// in left-to-right locales, the action buttons
			// are anchored in the bottom-right corner and
			// the default button is rightmost
			NSRect		parentFrame = [[STATIC_CAST([asNSArray objectAtIndex:0], NSButton*) superview] bounds];
			
			
			xOffset = NSWidth(parentFrame) - 20/* arbitrary but according to UI guidelines */;
			for (NSButton* button in asNSArray)
			{
				CGFloat		buttonWidth = Localization_AutoSizeNSButton(button);
				
				
				// in Cocoa the “frame” of a button appears to also
				// include the space after the button so the offset
				// only changes by the frame width
				xOffset -= buttonWidth;
				[button setFrame:NSMakeRect(xOffset, yOffset, buttonWidth, buttonHeight)];
			}
		}
	}
}// ArrangeNSButtonArray


/*!
Determines the width of the specified button that would
comfortably fit its current title, or else the specified
minimum width.  The chosen width for the button is returned.
If "inResize" is set to true, the button is resized to this
ideal width (the height and location are not changed).

Although NSControl has "sizeToFit", in practice this does not
work very well for buttons as the button appears to be too
constricted.

Note that this routine currently ignores other attributes of
a button that might affect its layout, such as the presence
of an image.

(2017.06)
*/
CGFloat
Localization_AutoSizeNSButton	(NSButton*		inButton,
								 CGFloat		inMinimumWidth,
								 Boolean		inResize)
{
@autoreleasepool {
	CGFloat		result = inMinimumWidth;
	
	
	if (nil != inButton)
	{
		NSString*	stringValue = [inButton title];
		NSFont*		font = [inButton font];
		CGFloat		stringWidth = [stringValue sizeWithAttributes:@{
																		NSFontAttributeName: font
																	}].width;
		
		
		result = (stringWidth + CGFLOAT_TIMES_2(MINIMUM_BUTTON_TITLE_CUSHION));
		if (result < inMinimumWidth)
		{
			result = inMinimumWidth;
		}
		
		if (inResize)
		{
			NSRect		frame = [inButton frame];
			
			
			frame.size.width = result;
			[inButton setFrame:frame];
		}
	}
	return result;
}// @autoreleasepool
}// AutoSizeNSButton

// BELOW IS REQUIRED NEWLINE TO END FILE
