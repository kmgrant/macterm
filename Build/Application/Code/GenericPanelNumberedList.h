/*!	\file GenericPanelNumberedList.h
	\brief Implements a kind of master-detail view where the
	master list displays indexed values (such as certain kinds
	of preferences).
	
	The detail panel is automatically placed next to the list.
	The combined view itself supports the Panel interface,
	allowing the list-panel combination to be dropped into any
	container that support panels (like the Preferences window).
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#ifndef __GENERICPANELNUMBEREDLIST__
#define __GENERICPANELNUMBEREDLIST__

// Mac includes
#include <CoreFoundation/CoreFoundation.h>
#ifdef __OBJC__
@class NSView;
#else
class NSView;
#endif

// application includes
#include "Panel.h"
#include "PrefsWindow.h"



#pragma mark Types

#ifdef __OBJC__

@class GenericPanelNumberedList_ViewManager;


/*!
When "panelViewManager:didChangeFromDataSet:toDataSet:"
is called on the detail view, the “data set” is of this
structure type.  It contains both the data set from the
parent and the index of the selected item in the list.

If the parent panel’s data set has changed, both the old
and new copies of the structure will have the same
selected list item.

If only the selected list item has changed, the parent
panel context will be unknown and set to "nullptr";
only the selected list item values will be defined.
*/
struct GenericPanelNumberedList_DataSet
{
	NSUInteger	selectedListItem;
	void*		parentPanelDataSetOrNull;
};


/*!
Declares the user interface properties of a list item.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@protocol GenericPanelNumberedList_ListItemHeader < NSObject > //{

@required

	// return strong reference to user interface string representing numbered index in list
	- (NSString*)
	numberedListIndexString;

	// return or update user interface string for name of item in list
	- (NSString*)
	numberedListItemName;
	- (void)
	setNumberedListItemName:(NSString*)_;

@end //}


/*!
Declares methods that are called as the user interacts
with the master view.  Typically an object must handle
these methods so that changes to the selection have
the appropriate effect on the detail view.
*/
@protocol GenericPanelNumberedList_Master //{

@required

	// the very first call; use this to ensure the data in the list is
	// defined so that any bindings will work properly (e.g. set the
	// property "listItemHeaders" to an array of new objects)
	- (void)
	initializeNumberedListViewManager:(GenericPanelNumberedList_ViewManager*)_;

	// respond to new selection in list (or, initial appearance of panel)
	- (void)
	numberedListViewManager:(GenericPanelNumberedList_ViewManager*)_
	didChangeFromDataSet:(GenericPanelNumberedList_DataSet*)_
	toDataSet:(GenericPanelNumberedList_DataSet*)_;

@optional

	// invoked after list view has been loaded; use this opportunity to
	// customize the UI, e.g. set properties like "headingTitleForNameColumn")
	- (void)
	containerViewDidLoadForNumberedListViewManager:(GenericPanelNumberedList_ViewManager*)_;

@end //}


/*!
Loads a NIB file that defines this panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface GenericPanelNumberedList_ViewManager : Panel_ViewManager< Panel_Delegate, Panel_Parent,
																		PrefsWindow_PanelInterface > //{
{
	IBOutlet NSTableView*	masterView;
	IBOutlet NSTabView*		detailView;
@private
	NSString*								identifier;
	NSString*								localizedName;
	NSImage*								localizedIcon;
	id< GenericPanelNumberedList_Master >	masterDriver;
	Panel_ViewManager*						detailViewManager;
	NSIndexSet*								_listItemHeaderIndexes;
	NSArray*								_listItemHeaders;
	NSArray*								_itemHeaderSortDescriptors;
}

// accessors
	@property (strong) NSString*
	headingTitleForNameColumn;
	@property (strong) NSArray*
	itemHeaderSortDescriptors; // binding
	@property (retain) NSIndexSet*
	listItemHeaderIndexes; // binding; selected item from "listItemHeaders" (when changed, the master is notified)
	@property (retain) NSArray*
	listItemHeaders; // binding; array of id< GenericPanelNumberedList_ListItemHeader >

// initializers
	- (instancetype)
	initWithIdentifier:(NSString*)_
	localizedName:(NSString*)_
	localizedIcon:(NSImage*)_
	master:(id< GenericPanelNumberedList_Master >)_
	detailViewManager:(Panel_ViewManager*)_;

@end //}

#endif // __OBJC__

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
