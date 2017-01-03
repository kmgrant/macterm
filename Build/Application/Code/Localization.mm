/*!	\file Localization.mm
	\brief Helps to make this program easier to translate into
	other languages and cultures.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2017 by Kevin Grant
	
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
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

// library includes
#import <CFRetainRelease.h>
#import <Console.h>

// application includes
#import "AppResources.h"



#pragma mark Variables
namespace {

CFRetainRelease		gApplicationNameCFString;
Boolean				gLeftToRight = true;		// text reads left-to-right?

} // anonymous namespace



#pragma mark Public Methods

/*!
This routine should be called once, before any
other Localization Utilities routine.  It allows
you to set flags that affect operation of all of
the module’s functions, and (optionally) to set
the file descriptor of a localization resource
file.

(1.0)
*/
void
Localization_Init	(UInt32		inFlags)
{
	gApplicationNameCFString.setWithRetain(CFBundleGetValueForInfoDictionaryKey
											(AppResources_ReturnBundleForInfo(), CFSTR("CFBundleName")));
	gLeftToRight = !(inFlags & kLocalization_InitFlagReadTextRightToLeft);
}// Init


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
	Float32		xOffset = 0;
	Float32		yOffset = 15; // arbitrary
	Float32		buttonWidth = NSWidth([inHelpButton frame]);
	Float32		buttonHeight = NSHeight([inHelpButton frame]);
	
	
	if (false == Localization_IsLeftToRight())
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
		Float32		defaultYOffset = [firstButton frame].origin.y;
		Float32		xOffset = 0;
		Float32		yOffset = defaultYOffset;
		Float32		buttonHeight = NSHeight([firstButton frame]);
		
		
		if (false == Localization_IsLeftToRight())
		{
			// in right-to-left locales, the action buttons
			// are anchored in the bottom-left corner and
			// the default button is leftmost
			xOffset = 20/* arbitrary but according to UI guidelines */;
			for (NSButton* button in asNSArray)
			{
				UInt16		buttonWidth = Localization_AutoSizeNSButton(button);
				
				
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
				UInt16		buttonWidth = Localization_AutoSizeNSButton(button);
				
				
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

(2.5)
*/
UInt16
Localization_AutoSizeNSButton	(NSButton*		inButton,
								 UInt16			inMinimumWidth,
								 Boolean		inResize)
{
@autoreleasepool {
	UInt16		result = inMinimumWidth;
	
	
	if (nil != inButton)
	{
		NSString*	stringValue = [inButton title];
		NSFont*		font = [inButton font];
		CGFloat		stringWidth = [stringValue sizeWithAttributes:@{
																		NSFontAttributeName: font
																	}].width;
		
		
		result = STATIC_CAST(stringWidth + INTEGER_TIMES_2(MINIMUM_BUTTON_TITLE_CUSHION), UInt16);
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


/*!
Returns a copy of the name of this process.
Unreliable unless Localization_Init() has
been called.

(3.0)
*/
void
Localization_GetCurrentApplicationNameAsCFString	(CFStringRef*		outProcessDisplayNamePtr)
{
	*outProcessDisplayNamePtr = gApplicationNameCFString.returnCFStringRef();
}// GetCurrentApplicationName


/*!
Determines if the Localization module was
initialized such that text reads left to
right (such as in North America).

(1.0)
*/
Boolean
Localization_IsLeftToRight ()
{
	Boolean		result = gLeftToRight;
	
	
	return result;
}// IsLeftToRight


/*!
Saves the current font, font size, font style,
and text mode of the current graphics port.

(1.0)
*/
void
Localization_PreservePortFontState	(GrafPortFontState*		outState)
{
	if (outState != nullptr)
	{
		GrafPtr		port = nullptr;
		
		
		GetPort(&port);
		outState->fontID = GetPortTextFont(port);
		outState->fontSize = GetPortTextSize(port);
		outState->fontStyle = GetPortTextFace(port);
		outState->textMode = GetPortTextMode(port);
	}
}// PreservePortFontState


/*!
Restores the font, font size, font style, and
text mode of the current graphics port.

(1.0)
*/
void
Localization_RestorePortFontState	(GrafPortFontState const*	inState)
{
	if (inState != nullptr)
	{
		TextFont(inState->fontID);
		TextSize(inState->fontSize);
		TextFace(inState->fontStyle);
		TextMode(inState->textMode);
	}
}// RestorePortFontState

// BELOW IS REQUIRED NEWLINE TO END FILE
