/*!	\file MenuUtilities.mm
	\brief Implements helper code for use with NSMenu.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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

#import "MenuUtilities.objc++.h"
#import <UniversalDefines.h>

// Mac includes
@import Cocoa;



#pragma mark Public Methods

@implementation MenuUtilities_DashSeparatorDelegate //{


#pragma mark NSMenuDelegate


/*!
Modifies the specified menu so that items named "-" (dash)
are replaced by real separator items.

(2016.10)
*/
- (void)
menuNeedsUpdate:(NSMenu*)	aMenu
{
	// TEMPORARY; over time, this could be made more general
	NSPredicate*	isSeparator = [NSPredicate predicateWithFormat:@"title == '-'"];
	NSArray*		fakeSeparators = [[aMenu itemArray]
										filteredArrayUsingPredicate:isSeparator];
	
	
	for (NSMenuItem* separatorMarkerItem in fakeSeparators)
	{
		[aMenu insertItem:[NSMenuItem separatorItem] atIndex:[aMenu indexOfItem:separatorMarkerItem]];
		[aMenu removeItem:separatorMarkerItem];
	}
}// menuNeedsUpdate:


@end //} MenuUtilities_DashSeparatorDelegate


#pragma mark Public Methods

/*!
Sets the state of a user interface item based on a flag.
Currently only works for menu items and checkmarks.

(2020.01)
*/
void
MenuUtilities_SetItemCheckMark	(id <NSValidatedUserInterfaceItem>	inItem,
								 BOOL								inIsChecked)
{
	if ([STATIC_CAST(inItem, id) isKindOfClass:[NSMenuItem class]])
	{
		NSMenuItem*		asMenuItem = STATIC_CAST(inItem, NSMenuItem*);
		
		
		[asMenuItem setState:((inIsChecked) ? NSControlStateValueOn : NSControlStateValueOff)];
	}
}// SetItemCheckMark

// BELOW IS REQUIRED NEWLINE TO END FILE
