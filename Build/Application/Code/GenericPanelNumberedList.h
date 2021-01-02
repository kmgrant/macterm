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

#pragma once

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
parent and the index of the selected item in the list
(relative to the original array, ignoring sorting).

If the parent panel’s data set has changed, both the old
and new copies of the structure will have the same
selected list item.

If only the selected list item has changed, the parent
panel context will be unknown and set to "nullptr";
only the selected list item values will be defined.
*/
struct GenericPanelNumberedList_DataSet
{
	NSUInteger			selectedDataArrayIndex;
	void* _Nullable		parentPanelDataSetOrNull;
};


/*!
Declares the user interface properties of a list item.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@protocol GenericPanelNumberedList_ItemBinding < NSObject > //{

@required

	// return strong reference to user interface string representing numbered index in list
	- (NSString* _Nullable)
	numberedListIndexString;

	// return strong reference to user interface icon representing item in list
	- (NSImage* _Nullable)
	numberedListItemIconImage;

	// return or update user interface string for name of item in list
	- (NSString* _Nullable)
	numberedListItemName;
	- (void)
	setNumberedListItemName:(NSString* _Nullable)_;

@end //}


/*!
Declares methods that are called as the user interacts
with the master view.  Typically an object must handle
these methods so that changes to the selection have
the appropriate effect on the detail view.
*/
@protocol GenericPanelNumberedList_Master < NSObject > //{

@required

	// the very first call; use this to ensure the data in the list is
	// defined so that any bindings will work properly (e.g. set the
	// property "listItemBindings" to an array of new objects)
	- (void)
	initializeNumberedListViewManager:(GenericPanelNumberedList_ViewManager* _Nonnull)_;

	// respond to new selection in list (or, initial appearance of panel)
	- (void)
	numberedListViewManager:(GenericPanelNumberedList_ViewManager* _Nonnull)_
	didChangeFromDataSet:(GenericPanelNumberedList_DataSet* _Nullable)_
	toDataSet:(GenericPanelNumberedList_DataSet* _Nullable)_;

@optional

	// invoked after list view has been loaded; use this opportunity to
	// customize the UI, e.g. set properties like "headingTitleForNameColumn")
	- (void)
	containerViewDidLoadForNumberedListViewManager:(GenericPanelNumberedList_ViewManager* _Nonnull)_;

@end //}


/*!
Loads a NIB file that defines this panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface GenericPanelNumberedList_ViewManager : Panel_ViewManager< NSSplitViewDelegate,
																		Panel_Delegate,
																		Panel_Parent,
																		PrefsWindow_PanelInterface > //{

// accessors
	//! The superview of the embedded panel, and the view whose frame
	//! defines the right-hand region of the split view.
	@property (strong, nonnull) IBOutlet NSView*
	detailContainer;
	//! The embedded panel.
	@property (strong, nonnull) IBOutlet NSTabView*
	detailView;
	//! Allows the title of the icon column to be changed.
	@property (strong, nonnull) NSString*
	headingTitleForIconColumn;
	//! Allows the title of the name column to be changed (for example,
	//! to say “Window Name” in a list of window configurations).
	@property (strong, nonnull) NSString*
	headingTitleForNameColumn;
	//! The object that controls access to an array of elements in the
	//! numbered list (bound to the columns of the table view).
	@property (strong, nonnull) IBOutlet NSArrayController*
	itemArrayController;
	//! The rules for sorting columns of the numbered list.
	@property (strong, nonnull) NSArray*
	itemBindingSortDescriptors; // binding
	//! Selected item from "listItemBindings" (when changed,
	//! the master is notified).
	@property (strong, nonnull) NSIndexSet*
	listItemBindingIndexes; // binding
	//! The objects that are managed by "itemArrayController".
	@property (strong, nonnull) NSArray*
	listItemBindings; // binding
	//! The superview of the numbered list, and the view whose frame
	//! defines the left-hand region of the split view.
	@property (strong, nonnull) IBOutlet NSView*
	masterContainer;
	//! The numbered list itself; see also "masterContainer".
	@property (strong, nonnull) IBOutlet NSTableView*
	masterView;
	//! The superview of the master and detail views, with a separator line.
	@property (strong, nonnull) IBOutlet NSSplitView*
	splitView;

// initializers
	- (instancetype _Nullable)
	initWithIdentifier:(NSString* _Nonnull)_
	localizedName:(NSString* _Nonnull)_
	localizedIcon:(NSImage* _Nonnull)_
	master:(id< GenericPanelNumberedList_Master > _Nonnull)_
	detailViewManager:(Panel_ViewManager* _Nonnull)_ NS_DESIGNATED_INITIALIZER;

@end //}

#endif // __OBJC__

// BELOW IS REQUIRED NEWLINE TO END FILE
