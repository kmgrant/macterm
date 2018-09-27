/*!	\file AddressDialog.mm
	\brief Cocoa implementation of the IP addresses panel.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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
#import <BoundName.objc++.h>
#import <CFRetainRelease.h>
#import <Console.h>
#import <CocoaExtensions.objc++.h>
#import <ListenerModel.h>

// application includes
#import "Network.h"



#pragma mark Types

/*!
Implements an object wrapper for IP addresses, that allows them
to be easily inserted into user interface elements without
losing less user-friendly information about each address.
*/
@interface AddressDialog_Address : BoundName_Object //{

// initializers
	- (instancetype)
	init;
	- (instancetype)
	initWithBoundName:(NSString*)_;
	- (instancetype)
	initWithDescription:(NSString*)_ NS_DESIGNATED_INITIALIZER;

@end //}


/*!
The private class interface.
*/
@interface AddressDialog_PanelController (AddressDialog_PanelControllerInternal) //{

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
@implementation AddressDialog_Address //{


#pragma mark Initializers


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
init
{
	return [self initWithDescription:@""];
}// init


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithBoundName:(NSString*)		aString
{
	return [self initWithDescription:aString];
}// initWithBoundName:


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithDescription:(NSString*)		aString
{
	self = [super initWithBoundName:aString];
	if (nil != self)
	{
	}
	return self;
}// initWithDescription:


@end //} AddressDialog_Address


#pragma mark -
@implementation AddressDialog_AddressArrayController //{


#pragma mark Initializers


/*!
Designated initializer.

(3.1)
*/
- (instancetype)
init
{
	self = [super init];
	// drag and drop support
	[addressTableView registerForDraggedTypes:@[NSStringPboardType]];
	[addressTableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
	return self;
}// init


#pragma mark NSTableViewDataSource


/*!
Adds the selected table strings to the specified drag.

NOTE: This is only called on Mac OS X 10.4 and beyond.

(3.1)
*/
- (BOOL)
tableView:(NSTableView*)			inTable
writeRowsWithIndexes:(NSIndexSet*)	inRowIndices
toPasteboard:(NSPasteboard*)		inoutPasteboard
{
#pragma unused(inTable)
	NSString*	dragData = [[[self arrangedObjects] objectAtIndex:[inRowIndices firstIndex]] boundName];
	BOOL		result = YES;
	
	
	// add the row string to the drag; only one can be selected at at time
	[inoutPasteboard declareTypes:@[NSStringPboardType] owner:self];
	[inoutPasteboard setString:dragData forType:NSStringPboardType];
	return result;
}// tableView:writeRowsWithIndexes:toPasteboard:


@end //} AddressDialog_AddressArrayController


#pragma mark -
@implementation AddressDialog_PanelController //{


static AddressDialog_PanelController*	gAddressDialog_PanelController = nil;


@synthesize addressObjectArray = _addressObjectArray;
@synthesize rebuildInProgress = _rebuildInProgress;


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
	self = [super initWithWindowNibName:@"AddressDialogCocoa"];
	if (nil != self)
	{
		self->_addressObjectArray = [[NSMutableArray alloc] init];
		self->_rebuildInProgress = NO;
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
	[_networkChangeListener release];
	[_addressObjectArray release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(2016.10)
*/
- (void)
insertObject:(AddressDialog_Address*)	anAddress
inAddressObjectArrayAtIndex:(unsigned long)	index
{
	[_addressObjectArray insertObject:anAddress atIndex:index];
}
- (void)
removeObjectFromAddressObjectArrayAtIndex:(unsigned long)	index
{
	[_addressObjectArray removeObjectAtIndex:index];
}// removeObjectFromAddressObjectArrayAtIndex:


#pragma mark Actions


/*!
Rebuilds the list of IP addresses based on the current
address assignments for the network cards in the computer.

(4.0)
*/
- (IBAction)
performIPAddressListRefresh:(id)	sender
{
#pragma unused(sender)
	Network_ResetLocalHostAddresses(); // triggers callbacks as needed
}// performIPAddressListRefresh:


#pragma mark NSWindowController


/*!
Initializations that depend on the window being defined
(for everything else, use "init").

(2016.10)
*/
- (void)
windowDidLoad
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
	
	// customize toolbar/title area
	{
		// NOTE: runtime OS is expected to support this feature but
		// while compilation requires legacy SDK (for old Carbon code)
		// it is not possible to just call it
		if (NO == CocoaExtensions_PerformSelectorOnTargetWithValue
					(@selector(setTitleVisibility:), self.window,
						FUTURE_SYMBOL(1, NSWindowTitleHidden)))
		{
			Console_Warning(Console_WriteLine, "failed to set network address window’s title bar visibility");
		}
	}
	
	// force interface to initialize (via callbacks)
	[self performIPAddressListRefresh:NSApp];
}// windowDidLoad


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"IPAddresses";
}// windowFrameAutosaveName


@end //} AddressDialog_PanelController


#pragma mark -
@implementation AddressDialog_PanelController (AddressDialog_PanelControllerInternal) //{


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
			self.rebuildInProgress = YES;
		}
		break;
	
	case kNetwork_ChangeAddressListDidRebuild:
		{
			CFArrayRef		localRawAddressStringArrayCopy = nullptr;
			
			
			// find the addresses (this makes a copy); since this is
			// in response to a “did rebuild” event, the list should
			// always be complete
			Network_CopyLocalHostAddresses(localRawAddressStringArrayCopy);
			
			// create matching objects in bound array (this will cause
			// the table in the panel to be updated, through bindings)
			[[self mutableArrayValueForKeyPath:NSStringFromSelector(@selector(addressObjectArray))]
												removeAllObjects];
			for (id object : BRIDGE_CAST(localRawAddressStringArrayCopy, NSArray*))
			{
				assert([object isKindOfClass:NSString.class]);
				NSString*	asString = STATIC_CAST(object, NSString*);
				
				
				[self insertObject:[[[AddressDialog_Address alloc]
										initWithDescription:asString] autorelease]
									inAddressObjectArrayAtIndex:self->_addressObjectArray.count];
			}
			
			// due to bindings, this will hide the progress indicator
			// (should be handled by the refresh step above but this is
			// done just to make sure it is gone)
			self.rebuildInProgress = NO;
			
			CFRelease(localRawAddressStringArrayCopy), localRawAddressStringArrayCopy = nullptr;
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"address dialog: network change handler received unexpected event", anEvent);
		break;
	}
}// model:networkChange:context:


@end //} AddressDialog_PanelController (AddressDialog_PanelController)


#pragma mark -
@implementation AddressDialog_Window //{


#pragma mark Initializers


/*!
Class initializer.

(2016.10)
*/
+ (void)
initialize
{
	// guard against subclass scenario by doing this at most once
	if (AddressDialog_Window.class == self)
	{
		BOOL	selectorFound = NO;
		
		
		selectorFound = CocoaExtensions_PerformSelectorOnTargetWithValue
						(@selector(setAllowsAutomaticWindowTabbing:), self.class, NO);
	}
}// initialize


/*!
Designated initializer.

(2016.10)
*/
- (instancetype)
initWithContentRect:(NSRect)		contentRect
styleMask:(NSUInteger)			windowStyle
backing:(NSBackingStoreType)		bufferingType
defer:(BOOL)					deferCreation
{
	self = [super initWithContentRect:contentRect styleMask:windowStyle
										backing:bufferingType defer:deferCreation];
	if (nil != self)
	{
	}
	return self;
}// initWithContentRect:styleMask:backing:defer:


/*!
Destructor.

(2016.10)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


@end //} AddressDialog_Window

// BELOW IS REQUIRED NEWLINE TO END FILE
