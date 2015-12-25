/*!	\file GenericPanelNumberedList.mm
	\brief Implements a kind of master-detail view where the
	master list displays indexed values (such as certain kinds
	of preferences).
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

#import "GenericPanelNumberedList.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <CocoaExtensions.objc++.h>

// application includes
#import "Panel.h"
#import "PrefsWindow.h"

#pragma mark Types

@interface GenericPanelNumberedList_ViewManager (GenericPanelNumberedList_ViewManagerInternal) //{

// new methods
	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


#pragma mark Public Methods

#pragma mark -
@implementation GenericPanelNumberedList_ViewManager


@synthesize itemHeaderSortDescriptors = _itemHeaderSortDescriptors;
@synthesize listItemHeaders = _listItemHeaders;


/*!
Designated initializer.

The detail view manager must be consistent with the usage of
the parent panel; for instance if you intend to put the parent
into the Preferences window then the detail view manager must
support PrefsWindow_PanelInterface; otherwise it only needs to
support Panel_Delegate.

(4.1)
*/
- (instancetype)
initWithIdentifier:(NSString*)					anIdentifier
localizedName:(NSString*)						aName
localizedIcon:(NSImage*)						anImage
master:(id< GenericPanelNumberedList_Master >)	aDriver
detailViewManager:(Panel_ViewManager*)			aViewManager
{
	self = [super initWithNibNamed:@"GenericPanelNumberedListCocoa"
									delegate:self
									context:@{
												@"identifier": anIdentifier,
												@"localizedName": aName,
												@"localizedIcon": anImage,
												@"masterDriver": aDriver,
												@"detailViewManager": aViewManager,
											}];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// initWithIdentifier:localizedName:localizedIcon:master:detailViewManager:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	detailViewManager.panelParent = nil;
	[identifier release];
	[localizedName release];
	[localizedIcon release];
	[_listItemHeaderIndexes release];
	[_listItemHeaders release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Allows the title of the name column to be customized
(for example, to say “Window Name” in a list of window
configurations).

(4.1)
*/
- (NSString*)
headingTitleForNameColumn
{
	NSTableColumn*		nameColumn = [self->masterView.tableColumns objectAtIndex:1];
	
	
	return [nameColumn.headerCell stringValue];
}
- (void)
setHeadingTitleForNameColumn:(NSString*)	aNewTitle
{
	NSTableColumn*		nameColumn = [self->masterView.tableColumns objectAtIndex:1];
	
	
	[nameColumn.headerCell setStringValue:aNewTitle];
}// setHeadingTitleForNameColumn:


/*!
Allows the title of the name column to be customized
(for example, to say “Window Name” in a list of window
configurations).

(4.1)
*/
- (NSIndexSet*)
listItemHeaderIndexes
{
	return [[_listItemHeaderIndexes retain] autorelease];
}
- (void)
setListItemHeaderIndexes:(NSIndexSet*)		anIndexSet
{
	if (_listItemHeaderIndexes != anIndexSet)
	{
		GenericPanelNumberedList_DataSet	oldStruct;
		GenericPanelNumberedList_DataSet	newStruct;
		
		
		[self willChangeValueForKey:@"listItemHeaderIndexes"];
		
		bzero(&oldStruct, sizeof(oldStruct));
		bzero(&newStruct, sizeof(newStruct));
		oldStruct.parentPanelDataSetOrNull = nullptr;
		oldStruct.selectedListItem = [_listItemHeaderIndexes firstIndex];
		if (NSNotFound == oldStruct.selectedListItem)
		{
			oldStruct.selectedListItem = 0; // require a selection at all times
		}
		newStruct.parentPanelDataSetOrNull = nullptr;
		
		// update the value
		[_listItemHeaderIndexes release];
		_listItemHeaderIndexes = [anIndexSet retain];
		newStruct.selectedListItem = [_listItemHeaderIndexes firstIndex];
		if (NSNotFound == newStruct.selectedListItem)
		{
			newStruct.selectedListItem = 0; // require a selection at all times
		}
		
		// notify the master of the new selection
		[self->masterDriver numberedListViewManager:self
													didChangeFromDataSet:&oldStruct
													toDataSet:&newStruct];
		
		// notify the detail view of the new selection
		[self->detailViewManager.delegate panelViewManager:self->detailViewManager
															didChangeFromDataSet:&oldStruct
															toDataSet:&newStruct];
		
		[self didChangeValueForKey:@"listItemHeaderIndexes"];
	}
}// setListItemHeaderIndexes:


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(void*)			aContext
{
#pragma unused(aViewManager)
	// IMPORTANT: see the initializer for the construction of this dictionary and the names of keys that are used
	NSDictionary*		asDictionary = STATIC_CAST(aContext, NSDictionary*);
	NSString*			givenIdentifier = [asDictionary objectForKey:@"identifier"];
	NSString*			givenName = [asDictionary objectForKey:@"localizedName"];
	NSImage*			givenIcon = [asDictionary objectForKey:@"localizedIcon"];
	id< GenericPanelNumberedList_Master >	givenDriver = [asDictionary objectForKey:@"masterDriver"];
	Panel_ViewManager*	givenDetailViewManager = [asDictionary objectForKey:@"detailViewManager"];
	
	
	self->identifier = [givenIdentifier retain];
	self->localizedName = [givenName retain];
	self->localizedIcon = [givenIcon retain];
	self->_itemHeaderSortDescriptors = [[@[
											// uses "id< GenericPanelNumberedList_ListItemHeader >" method names
											[NSSortDescriptor sortDescriptorWithKey:@"self.numberedListIndexString" ascending:YES
																					selector:@selector(localizedStandardCompare:)],
											[NSSortDescriptor sortDescriptorWithKey:@"self.numberedListItemName" ascending:YES
																					selector:@selector(localizedStandardCompare:)]
										] retain] autorelease];
	self->_listItemHeaderIndexes = [[NSIndexSet alloc] initWithIndex:0];
	self->_listItemHeaders = [NSArray array];
	self->masterDriver = givenDriver;
	self->detailViewManager = [givenDetailViewManager retain];
	self->detailViewManager.panelParent = self;
	
	assert(nil != self->detailViewManager);
	
	[self->masterDriver initializeNumberedListViewManager:self];
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	*outEditType = kPanel_EditTypeNormal;
	[self->detailViewManager.delegate panelViewManager:aViewManager requestingEditType:outEditType];
}// panelViewManager:requestingEditType:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
#pragma unused(aViewManager, aContainerView)
	assert(nil != masterView);
	assert(nil != detailView);
	
	NSTabViewItem*		tabItem = [[NSTabViewItem alloc] initWithIdentifier:[self->detailViewManager panelIdentifier]];
	
	
	[tabItem setView:[self->detailViewManager managedView]];
	[tabItem setInitialFirstResponder:[self->detailViewManager logicalFirstResponder]];
	[self->detailView addTabViewItem:tabItem];
	[tabItem release];
	
	//[self->detailView selectTabViewItemWithIdentifier:[self->detailViewManager panelIdentifier]];
	
	[self->masterDriver containerViewDidLoadForNumberedListViewManager:self];
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	NSRect		masterFrame = [self->masterView frame];
	NSRect		defaultDetailFrame = [self->detailView frame];
	NSSize		panelIdealSize = defaultDetailFrame.size;
	
	
	*outIdealSize = panelIdealSize;
	[self->detailViewManager.delegate panelViewManager:aViewManager requestingIdealSize:&panelIdealSize];
	outIdealSize->width = NSWidth(masterFrame) + 64/* sum of end and middle paddings */ + MAX(panelIdealSize.width, outIdealSize->width);
	outIdealSize->height = MAX(NSHeight(masterFrame), 32/* approximate header/line size */ + outIdealSize->height);
}// panelViewManager:requestingIdealSize:


/*!
Responds to a request for contextual help in this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager)
	// forward to detail view
	[self->detailViewManager.delegate panelViewManager:self->detailViewManager didPerformContextSensitiveHelp:sender];
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager)
	// forward to detail view
	[self->detailViewManager.delegate panelViewManager:self->detailViewManager willChangePanelVisibility:aVisibility];
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager)
	// forward to detail view
	[self->detailViewManager.delegate panelViewManager:self->detailViewManager didChangePanelVisibility:aVisibility];
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

The message is forwarded to the detail view but the data type
of the context is changed to "GeneralPanelNumberedList_DataSet"
(where only the "parentPanelDataSetOrNull" field can be
expected to change from old to new).

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager)
	GenericPanelNumberedList_DataSet	oldStruct;
	GenericPanelNumberedList_DataSet	newStruct;
	
	
	// the "listItemHeaders" property needs to send update
	// notifications so that (for instance) list displays
	// will be refreshed
	[self willChangeValueForKey:@"listItemHeaders"];
	
	bzero(&oldStruct, sizeof(oldStruct));
	oldStruct.parentPanelDataSetOrNull = oldDataSet;
	oldStruct.selectedListItem = [self.listItemHeaderIndexes firstIndex];
	if (NSNotFound == oldStruct.selectedListItem)
	{
		oldStruct.selectedListItem = 0; // require a selection at all times
	}
	bzero(&newStruct, sizeof(newStruct));
	newStruct.parentPanelDataSetOrNull = newDataSet;
	newStruct.selectedListItem = 0; // reset the current selection
	
	[self->masterDriver numberedListViewManager:self
												didChangeFromDataSet:&oldStruct
												toDataSet:&newStruct];
	
	[self->detailViewManager.delegate panelViewManager:self->detailViewManager
														didChangeFromDataSet:&oldStruct
														toDataSet:&newStruct];
	
	[self didChangeValueForKey:@"listItemHeaders"];
	
	// change the displayed selection
	self.listItemHeaderIndexes = [NSIndexSet indexSetWithIndex:newStruct.selectedListItem];
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).

The message is forwarded to the detail view.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager)
	// forward to detail view
	[self->detailViewManager.delegate panelViewManager:self->detailViewManager didFinishUsingContainerView:aContainerView userAccepted:isAccepted];
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_Parent


/*!
Selects the panel with the given panel identifier, or does
nothing if the identifier does not match any panel.

Currently the animation flag has no effect.

(4.1)
*/
- (void)
panelParentDisplayChildWithIdentifier:(NSString*)	anIdentifier
withAnimation:(BOOL)								isAnimated
{
#pragma unused(anIdentifier, isAnimated)
	// nothing needed
}// panelParentDisplayChildWithIdentifier:withAnimation:


/*!
Returns an enumerator over the Panel_ViewManager* objects
for the panels in this view.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	return [[[@[self->detailViewManager] retain] autorelease] objectEnumerator];
}// panelParentEnumerateChildViewManagers


#pragma mark Panel_ViewManager


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [[self->localizedIcon retain] autorelease];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return [[self->identifier retain] autorelease];
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

(4.1)
*/
- (NSString*)
panelName
{
	return [[self->localizedName retain] autorelease];
}// panelName


/*!
Returns information on which directions are most useful for
resizing the panel.  For instance a window container may
disallow vertical resizing if no panel in the window has
any reason to resize vertically.

IMPORTANT:	This is only a hint.  Panels must be prepared
			to resize in both directions.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	// return the detail view’s constraint
	Panel_ResizeConstraint	result = [self->detailViewManager panelResizeAxes];
	
	
	return result;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

IMPORTANT:	This interface is implemented for convenience but may
			only be used if the delegate of the detail view manager
			supports "PrefsWindow_PanelInterface".

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	Quills::Prefs::Class	result = Quills::Prefs::GENERAL;
	
	
	if ([self->detailViewManager conformsToProtocol:@protocol(PrefsWindow_PanelInterface)])
	{
		result = [STATIC_CAST(self->detailViewManager, id< PrefsWindow_PanelInterface >) preferencesClass];
	}
	
	return result;
}// preferencesClass


@end // GenericPanelNumberedList_ViewManager


#pragma mark -
@implementation GenericPanelNumberedList_ViewManager (GenericPanelNumberedList_ViewManagerInternal) //{


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"listItemHeaderIndexes", @"listItemHeaders"];
}


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
