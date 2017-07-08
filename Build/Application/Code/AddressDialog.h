/*!	\file AddressDialog.h
	\brief Implements the IP address dialog.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif



#ifdef __OBJC__

@class ListenerModel_StandardListener;

/*!
Implements drag and drop for the addresses table.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface AddressDialog_AddressArrayController : NSArrayController //{
{
	IBOutlet NSTableView*	addressTableView;
}

// NSTableViewDataSource
	- (BOOL)
	tableView:(NSTableView*)_
	writeRowsWithIndexes:(NSIndexSet*)_
	toPasteboard:(NSPasteboard*)_;

@end //}


/*!
Implements the IP Addresses window.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface AddressDialog_Window : NSPanel //{
{
}

@end //}


/*!
Implements the IP Addresses window controller.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface AddressDialog_PanelController : NSWindowController //{
{
	ListenerModel_StandardListener*		_networkChangeListener;
	NSMutableArray*						_addressObjectArray;
	BOOL								_rebuildInProgress;
}

// class methods
	+ (id)
	sharedAddressPanelController;

// accessors
	@property (strong) NSArray*/* of AddressDialog_Address objects */
	addressObjectArray; // binding
	@property (assign) BOOL
	rebuildInProgress; // binding

// actions
	- (IBAction)
	performIPAddressListRefresh:(id)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

void
	AddressDialog_Display						();

// BELOW IS REQUIRED NEWLINE TO END FILE
