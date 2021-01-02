/*!	\file AddressDialog.mm
	\brief Cocoa implementation of the IP addresses panel.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#import "AddressDialog.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CFRetainRelease.h>
#import <Console.h>
#import <CocoaExtensions.objc++.h>
#import <ListenerModel.h>

// application includes
#import "Network.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>



#pragma mark Types

/*!
Implements the IP Addresses window controller.
*/
@interface AddressDialog_PanelController : NSWindowController <UIAddressList_ActionHandling> //{
{
	ListenerModel_StandardListener*		_networkChangeListener;
	UIAddressList_Model*				_viewModel;
}

// class methods
	+ (id)
	sharedAddressPanelController;

// accessors
	- (UIAddressList_Model*)
	viewModel;

// new methods
	- (void)
	model:(ListenerModel_Ref)_
	networkChange:(ListenerModel_Event)_
	context:(void*)_;

@end //}


#pragma mark Public Methods

/*!
Shows or hides the IP Addresses panel.

(3.1)
*/
void
AddressDialog_Display ()
{
@autoreleasepool {
	[[AddressDialog_PanelController sharedAddressPanelController] showWindow:NSApp];
}// @autoreleasepool
}// Display


#pragma mark Internal Methods


#pragma mark -
@implementation AddressDialog_PanelController //{


static AddressDialog_PanelController*	gAddressDialog_PanelController = nil;


#pragma mark Class Methods


/*!
Returns the singleton.

(3.1)
*/
+ (id)
sharedAddressPanelController
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gAddressDialog_PanelController = [[self.class allocWithZone:NULL] init];
	});
	return gAddressDialog_PanelController;
}


#pragma mark Initializers


/*!
Designated initializer.

(3.1)
*/
- (instancetype)
init
{
	UIAddressList_Model*	viewModel = [[UIAddressList_Model alloc] initWithRunner:self];
	NSViewController*		viewController = [UIAddressList_ObjC makeViewController:viewModel];
	NSPanel*				panelObject = [NSPanel windowWithContentViewController:viewController];
	
	
	panelObject.styleMask = (NSWindowStyleMaskTitled |
								NSWindowStyleMaskClosable |
								NSWindowStyleMaskMiniaturizable |
								NSWindowStyleMaskResizable |
								NSWindowStyleMaskUtilityWindow);
	panelObject.title = @"";
	
	self = [super initWithWindow:panelObject];
	if (nil != self)
	{
		// set up the network change listener
		{
			Network_Result		networkResult = kNetwork_ResultOK;
			
			
			self->_networkChangeListener = [[ListenerModel_StandardListener alloc]
											initWithTarget:self
															eventFiredSelector:@selector(model:networkChange:context:)];
			
			networkResult = Network_StartMonitoring(kNetwork_ChangeAddressListWillRebuild, [self->_networkChangeListener listenerRef],
													0/* flags */);
			if (NO == networkResult.ok())
			{
				Console_Warning(Console_WriteLine, "address dialog failed to start monitoring will-rebuild-address-list; progress indicator may not be updated");
			}
			networkResult = Network_StartMonitoring(kNetwork_ChangeAddressListDidRebuild, [self->_networkChangeListener listenerRef],
													0/* flags */);
			if (NO == networkResult.ok())
			{
				Console_Warning(Console_WriteLine, "address dialog failed to start monitoring did-rebuild-address-list; UI may not be updated");
			}
		}
		self->_viewModel = viewModel;
		
		self.windowFrameAutosaveName = @"IPAddresses"; // (for backward compatibility, never change this)
		
		// force interface to initialize (via callbacks)
		[self refreshWithAddressList:self.viewModel];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	Network_StopMonitoring(kNetwork_ChangeAddressListWillRebuild, [self->_networkChangeListener listenerRef]);
	Network_StopMonitoring(kNetwork_ChangeAddressListDidRebuild, [self->_networkChangeListener listenerRef]);
}// dealloc


#pragma mark Accessors


/*!
Returns the object that binds to the UI.

(2020.09)
*/
- (UIAddressList_Model*)
viewModel
{
	return _viewModel;
}// viewModel


#pragma mark Actions


/*!
Rebuilds the list of IP addresses based on the current
address assignments for the network cards in the computer.

(2020.09)
*/
- (IBAction)
refreshWithAddressList:(UIAddressList_Model*)	aModel
{
#pragma unused(aModel)
	Network_ResetLocalHostAddresses(); // triggers callbacks as needed
}// refreshWithAddressList:


#pragma mark New Methods


/*!
Called when a monitored network event occurs.  See
"init" for the set of events that is monitored.

(2016.10)
*/
- (void)
model:(ListenerModel_Ref)				aModel
networkChange:(ListenerModel_Event)		anEvent
context:(void*)							aContext
{
#pragma unused(aModel, aContext)
	switch (anEvent)
	{
	case kNetwork_ChangeAddressListWillRebuild:
		{
			// due to bindings, this will show a progress indicator
			self.viewModel.refreshInProgress = YES;
		}
		break;
	
	case kNetwork_ChangeAddressListDidRebuild:
		{
			CFArrayRef									localRawAddressStringArrayCopy = nullptr;
			NSMutableArray<UIAddressList_ItemModel*>*	mutableArray = [[NSMutableArray<UIAddressList_ItemModel*> alloc] init];
			
			
			// find the addresses (this makes a copy); since this is
			// in response to a “did rebuild” event, the list should
			// always be complete
			Network_CopyLocalHostAddresses(localRawAddressStringArrayCopy);
			
			// create matching objects in bound array (this will cause
			// the table in the panel to be updated, through bindings)
			for (id object : BRIDGE_CAST(localRawAddressStringArrayCopy, NSArray*))
			{
				assert([object isKindOfClass:NSString.class]);
				NSString*	asString = STATIC_CAST(object, NSString*);
				
				
				[mutableArray addObject:[[UIAddressList_ItemModel alloc] initWithString:asString]];
			}
			self.viewModel.addressArray = mutableArray;
			
			// due to bindings, this will hide the progress indicator
			// (should be handled by the refresh step above but this is
			// done just to make sure it is gone)
			self.viewModel.refreshInProgress = NO;
			
			CFRelease(localRawAddressStringArrayCopy); localRawAddressStringArrayCopy = nullptr;
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"address dialog: network change handler received unexpected event", anEvent);
		break;
	}
}// model:networkChange:context:


@end //} AddressDialog_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
