/*!	\file TouchBar.mm
	\brief Implements keyboard Touch Bars and their items.
*/
/*###############################################################

	Interface Library
	© 1998-2019 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
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

#import "TouchBar.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>



#pragma mark Public Methods


#pragma mark -
@implementation TouchBar_Controller //{


@synthesize customizationAllowedItemIdentifiers = _customizationAllowedItemIdentifiers;
@synthesize customizationIdentifier = _customizationIdentifier;


#pragma mark Initializers


/*!
Designated initializer.

(2016.11)
*/
- (instancetype)
initWithNibName:(NSString*)		aNibName
{
	self = [super initWithNibName:aNibName bundle:nil];
	if (nil != self)
	{
		// see "loadView" for load-dependent initialization
	}
	return self;
}// initWithNibName:


/*!
Destructor.

(2016.11)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(2016.11)
*/
- (NSTouchBar*)
touchBar
{
	// request the "view" property explicitly to force
	// the NIB to load; in reality, the view is never
	// used and the goal is to load the Touch Bar
	self.view.toolTip = @""; // arbitrary setting; make sure compiler does not optimize away unused view
	
	assert(nil != _touchBar);
	return _touchBar;
}// touchBar


#pragma mark NSViewController


/*!
Used to detect loading of the Touch Bar.  (The "view"
binding of the controller is not used; if the
"touchBar" property is requested, the NIB will load
just in time.)

(2016.11)
*/
- (void)
loadView
{
	[super loadView];
	
	assert(nil != _touchBar);
	
	if (@available(macOS 10.12.1, *))
	{
		_touchBar.customizationIdentifier = self.customizationIdentifier;
		_touchBar.customizationAllowedItemIdentifiers = self.customizationAllowedItemIdentifiers;
	}
}// loadView


@end //} TouchBar_Controller


#pragma mark -
@implementation TouchBar_ControllerWithColorPicker //{


@synthesize colorPickerTouchBarItem = _colorPickerTouchBarItem;


#pragma mark Initializers


/*!
Designated initializer.

(2016.11)
*/
- (instancetype)
initWithNibName:(NSString*)		aNibName
{
	self = [super initWithNibName:aNibName bundle:nil];
	if (nil != self)
	{
		// see "loadView" for load-dependent initialization
	}
	return self;
}// initWithNibName:


/*!
Destructor.

(2016.11)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark NSViewController


/*!
Used to detect loading of the Touch Bar.  (The "view"
binding of the controller is not used; if the
"touchBar" property is requested, the NIB will load
just in time.)

(2016.11)
*/
- (void)
loadView
{
	[super loadView];
	
	assert(nil != _colorPickerTouchBarItem);
	
	BOOL	enableOK = CocoaExtensions_PerformSelectorOnTargetWithValue
						(@selector(setEnabled:), _colorPickerTouchBarItem, YES);
	assert(enableOK);
}// loadView


@end //} TouchBar_ControllerWithColorPicker

// BELOW IS REQUIRED NEWLINE TO END FILE
