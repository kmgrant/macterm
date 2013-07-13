/*!	\file ContextSensitiveMenu.mm
	\brief Simplifies handling of contextual menus.
*/
/*###############################################################

	Contexts Library 2.0
	© 1998-2013 by Kevin Grant
	
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

#import <ContextSensitiveMenu.h>
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Variables
namespace {

UInt16	gAddedGroupItemCount = 0;

} // anonymous namespace



#pragma mark Public Methods

/*!
This MUST be called prior to constructing any new menu.
(It resets internal state that is used to track the
presence of item groups.)

(2.0)
*/
void
ContextSensitiveMenu_Init ()
{
	// reinitialize ALL globals
	gAddedGroupItemCount = 0;
}// Init


/*!
Unconditionally adds an item to the specified menu.
If ContextSensitiveMenu_NewItemGroup() has been
called and no dividing-line has yet been inserted
for the group, a separator is inserted first.

(2.0)
*/
void
ContextSensitiveMenu_AddItem	(NSMenu*		inToWhichMenu,
								 NSMenuItem*	inItem)
{
	if ((0 == gAddedGroupItemCount) && ([inToWhichMenu numberOfItems] > 0))
	{
		[inToWhichMenu addItem:[NSMenuItem separatorItem]];
	}
	[inToWhichMenu addItem:inItem];
	++gAddedGroupItemCount;
}// AddItem


/*!
Marks subsequent calls to ContextSensitiveMenu_AddItem()
as belonging to a new group of related items.  The first
item to be added will be preceded by a separator item.
If no items are added, no separator is ever inserted.

(2.0)
*/
void
ContextSensitiveMenu_NewItemGroup ()
{
	gAddedGroupItemCount = 0;
}// NewItemGroup

// BELOW IS REQUIRED NEWLINE TO END FILE
