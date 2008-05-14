/*!	\file AddressDialog.mm
	\brief Cocoa implementation of the IP addresses panel.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <AutoPool.objc++.h>
#import <Console.h>

// MacTelnet includes
#import "AddressDialog.h"
#import "Network.h"



#pragma mark Public Methods

/*!
Shows or hides the IP Addresses panel.

(3.1)
*/
void
AddressDialog_Display ()
{
	AutoPool	_;
	
	
	[[AddressDialog_PanelController sharedAddressPanelController] showWindow:NSApp];
}// Display


#pragma mark Internal Methods

@implementation AddressDialog_AddressArrayController

- (id)init
{
	self = [super init];
	// drag and drop support
	[addressTableView registerForDraggedTypes:[NSArray arrayWithObjects:NSStringPboardType, nil]];
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	[addressTableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
#endif
	return self;
}

/*!
Adds the selected table strings to the specified drag.

NOTE: This is only called on Mac OS X 10.4 and beyond.

(3.1)
*/
- (BOOL)tableView:(NSTableView*)			inTable
		writeRowsWithIndexes:(NSIndexSet*)	inRowIndices
		toPasteboard:(NSPasteboard*)		inoutPasteboard
{
#pragma unused(inTable)
	NSString*	dragData = [[self arrangedObjects] objectAtIndex:[inRowIndices firstIndex]];
	BOOL		result = YES;
	
	
	// add the row string to the drag; only one can be selected at at time
	[inoutPasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:self];
	[inoutPasteboard setString:dragData forType:NSStringPboardType];
	return result;
}

@end // AddressDialog_AddressArrayController

@implementation AddressDialog_PanelController

static AddressDialog_PanelController*	gAddressDialog_PanelController = nil;
+ (id)sharedAddressPanelController
{
	if (nil == gAddressDialog_PanelController)
	{
		gAddressDialog_PanelController = [[AddressDialog_PanelController allocWithZone:NULL] init];
	}
	return gAddressDialog_PanelController;
}

- (id)init
{
	self = [super initWithWindowNibName:@"AddressDialogCocoa"];
	
	// find IP addresses and store them in the bound array
	{
		typedef std::vector< CFRetainRelease >		My_AddressList;
		My_AddressList	addressList;
		Boolean			addressesFound = Network_CopyIPAddresses(addressList);
		
		
		addressArray = [[NSMutableArray alloc] init];
		if (addressesFound)
		{
			for (My_AddressList::size_type i = 0; i < addressList.size(); ++i)
			{
				NSString*	addressString = (NSString*)addressList[i].returnCFStringRef();
				
				
				[addressArray addObject:addressString];
			}
		}
	}
	
	return self;
}

@end // AddressDialog_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
