/*!	\file TouchBar.objc++.h
	\brief Implements keyboard Touch Bars and their items.
*/
/*###############################################################

	Interface Library
	© 1998-2018 by Kevin Grant
	
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

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Types


/*!
Allows the NSTouchBar to be specified in a separate
file and loaded only when the runtime supports it.

For simplicity, properties can be set on this class
to determine how the Touch Bar is initialized when it
is first loaded (setting default item identifiers,
etc.).  This is also the only way to make settings
when the SDK version is lower than the runtime.

The Touch Bar interface in the NIB should generally
be configured to send commands to the first responder.

The view is unused; it just simplifies NIB loading.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TouchBar_Controller : NSViewController //{
{
	IBOutlet NSTouchBar*	_touchBar;
@private
	NSArray*	_customizationAllowedItemIdentifiers;
	NSString*	_customizationIdentifier;
}

// initializers
	- (instancetype)
	initWithNibName:(NSString*)_;

// accessors
	@property (strong) NSArray*
	customizationAllowedItemIdentifiers;
	@property (strong) NSString*
	customizationIdentifier;
	@property (strong, readonly) NSTouchBar*
	touchBar;

@end //}


/*!
This subclass is a hack to work around the fact that
NIB-provided color picker items do not work unless
modified programmatically.  It may disappear in the
future if Apple fixes the default behavior (based on
Xcode 8.1 release notes, this is a known issue filed
as rdar://28670596).

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TouchBar_ControllerWithColorPicker : TouchBar_Controller //{
{
	IBOutlet NSColorPickerTouchBarItem*		_colorPickerTouchBarItem;
}

// initializers
	- (instancetype)
	initWithNibName:(NSString*)_;

// accessors
	@property (strong, readonly) NSColorPickerTouchBarItem*
	colorPickerTouchBarItem;

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
