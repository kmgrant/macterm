/*!	\file ContextSensitiveMenu.mm
	\brief Simplifies handling of contextual menus.
*/
/*###############################################################

	Contexts Library
	© 1998-2020 by Kevin Grant
	
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

// library includes
#import <CFRetainRelease.h>



#pragma mark Variables
namespace {

UInt16				gAddedGroupItemCount = 0;
CFRetainRelease		gGroupTitle;

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

The item is automatically changed to indented form
if it is in a group that has a title.

(2.0)
*/
void
ContextSensitiveMenu_AddItem	(NSMenu*		inToWhichMenu,
								 NSMenuItem*	inItem)
{
	if ((0 == gAddedGroupItemCount) && ([inToWhichMenu numberOfItems] > 0))
	{
		// only insert a separator if there is something above it
		[inToWhichMenu addItem:[NSMenuItem separatorItem]];
	}
	
	if ((0 == gAddedGroupItemCount) && gGroupTitle.exists())
	{
		// regardless of whether or not a separator was added, a
		// titled group always inserts its title item first; the
		// group-title item should have the same appearance as
		// other “title items” (small, bold and disabled), which
		// is accomplished by using an attributed string
		NSString*				titleString = BRIDGE_CAST(gGroupTitle.returnCFStringRef(), NSString*);
		NSDictionary*			fontDict = @{NSFontAttributeName: [NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]]};
		NSMenuItem*				groupItem = [[[NSMenuItem alloc] initWithTitle:titleString action:nil keyEquivalent:@""]
												autorelease];
		NSAttributedString*		attributedTitle = [[[NSAttributedString alloc] initWithString:titleString attributes:fontDict]
													autorelease];
		
		
		[groupItem setAttributedTitle:attributedTitle];
		[inToWhichMenu addItem:groupItem];
	}
	
	if (gGroupTitle.exists())
	{
		// items belonging to a group with a title are all
		// indented underneath the title
		[inItem setIndentationLevel:1];
	}
	
	// finally, add the item itself!
	[inToWhichMenu addItem:inItem];
	++gAddedGroupItemCount;
}// AddItem


/*!
Marks subsequent calls to ContextSensitiveMenu_AddItem()
as belonging to a new group of related items.  The first
item to be added will be preceded by a separator item.
If no items are added, no separator is ever inserted.

If "inTitleOrNull" is not nullptr, a “title style” menu
item is automatically inserted immediately after the
separator, if and only if the separator is ever inserted.
Note that when using a title in this way, application
style guidelines suggest that the title should end with
a colon (:) and all of the items in the group should be
indented.

(2.0)
*/
void
ContextSensitiveMenu_NewItemGroup	(CFStringRef	inTitleOrNull)
{
	gAddedGroupItemCount = 0;
	if (nullptr == inTitleOrNull)
	{
		gGroupTitle.clear();
	}
	else
	{
		gGroupTitle.setWithRetain(inTitleOrNull);
	}
}// NewItemGroup

// BELOW IS REQUIRED NEWLINE TO END FILE
