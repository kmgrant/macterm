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


/*!
Private properties.
*/
@interface GenericPanelNumberedList_ViewManager () //{

// accessors
	@property (strong) Panel_ViewManager*
	detailViewManager;
	@property (strong) id< GenericPanelNumberedList_Master >
	masterDriver;

@end //}


/*!
The private class interface.
*/
@interface GenericPanelNumberedList_ViewManager (GenericPanelNumberedList_ViewManagerInternal) //{

// new methods
	- (CGFloat)
	idealDetailContainerWidth;
	- (NSArray*)
	primaryDisplayBindingKeys;

@end //}


#pragma mark Public Methods

#pragma mark -
@implementation GenericPanelNumberedList_ViewManager


/*!
The superview of the embedded panel, and the view whose frame
defines the right-hand region of the split view.
*/
@synthesize detailContainer = _detailContainer;

/*!
The embedded panel.
*/
@synthesize detailView = _detailView;

/*!
The object that is the view controller for the embedded panel.
*/
@synthesize detailViewManager = _detailViewManager;

/*!
The object that controls access to an array of elements in the
numbered list (bound to the columns of the table view).
*/
@synthesize itemArrayController = _itemArrayController;

/*!
The rules for sorting columns of the numbered list.
*/
@synthesize itemBindingSortDescriptors = _itemBindingSortDescriptors;

/*!
The objects that are managed by "itemArrayController".
*/
@synthesize listItemBindings = _listItemBindings;

/*!
The superview of the numbered list, and the view whose frame
defines the left-hand region of the split view.
*/
@synthesize masterContainer = _masterContainer;

/*!
The object that is notified of activity in the master, such as
clicks in different items of the numbered list.
*/
@synthesize masterDriver = _masterDriver;

/*!
The numbered list itself; see also "masterContainer".
*/
@synthesize masterView = _masterView;

/*!
The superview of the master and detail views, with a separator line.
*/
@synthesize splitView = _splitView;


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
	self.detailViewManager.panelParent = nil;
	[_detailContainer release];
	[_detailView release];
	[identifier release];
	[_itemArrayController release];
	[_itemBindingSortDescriptors release];
	[localizedName release];
	[localizedIcon release];
	[_listItemBindingIndexes release];
	[_listItemBindings release];
	[_masterContainer release];
	[_masterDriver release];
	[_masterView release];
	[_splitView release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Allows the title of the icon column to be customized.

(2020.04)
*/
- (NSString*)
headingTitleForIconColumn
{
	NSTableColumn*		nameColumn = [self.masterView.tableColumns objectAtIndex:1];
	
	
	return [nameColumn.headerCell stringValue];
}
- (void)
setHeadingTitleForIconColumn:(NSString*)	aNewTitle
{
	NSTableColumn*		nameColumn = [self.masterView.tableColumns objectAtIndex:1];
	
	
	[nameColumn.headerCell setStringValue:aNewTitle];
}// setHeadingTitleForIconColumn:


/*!
Allows the title of the name column to be customized
(for example, to say “Window Name” in a list of window
configurations).

(4.1)
*/
- (NSString*)
headingTitleForNameColumn
{
	NSTableColumn*		nameColumn = [self.masterView.tableColumns objectAtIndex:2];
	
	
	return [nameColumn.headerCell stringValue];
}
- (void)
setHeadingTitleForNameColumn:(NSString*)	aNewTitle
{
	NSTableColumn*		nameColumn = [self.masterView.tableColumns objectAtIndex:2];
	
	
	[nameColumn.headerCell setStringValue:aNewTitle];
}// setHeadingTitleForNameColumn:


/*!
Allows the title of the name column to be customized
(for example, to say “Window Name” in a list of window
configurations).

(4.1)
*/
- (NSIndexSet*)
listItemBindingIndexes
{
	return [[_listItemBindingIndexes retain] autorelease];
}
- (void)
setListItemBindingIndexes:(NSIndexSet*)		anIndexSet
{
	if (_listItemBindingIndexes != anIndexSet)
	{
		GenericPanelNumberedList_DataSet	oldStruct;
		GenericPanelNumberedList_DataSet	newStruct;
		NSUInteger							oldBindingIndex = [self.listItemBindingIndexes firstIndex];
		NSUInteger							newBindingIndex = [anIndexSet firstIndex];
		
		
		[self willChangeValueForKey:@"listItemBindingIndexes"];
		
		bzero(&oldStruct, sizeof(oldStruct));
		bzero(&newStruct, sizeof(newStruct));
		oldStruct.parentPanelDataSetOrNull = nullptr;
		if (oldBindingIndex < [self.itemArrayController.arrangedObjects count])
		{
			oldStruct.selectedDataArrayIndex = [self.listItemBindings indexOfObject:[self.itemArrayController.arrangedObjects objectAtIndex:oldBindingIndex]];
		}
		if (NSNotFound == oldStruct.selectedDataArrayIndex)
		{
			oldStruct.selectedDataArrayIndex = 0; // require a selection at all times
		}
		
		// update the value
		[_listItemBindingIndexes release];
		_listItemBindingIndexes = [anIndexSet retain];
		newStruct.parentPanelDataSetOrNull = nullptr;
		if (newBindingIndex < [self.itemArrayController.arrangedObjects count])
		{
			newStruct.selectedDataArrayIndex = [self.listItemBindings indexOfObject:[self.itemArrayController.arrangedObjects objectAtIndex:newBindingIndex]];
		}
		if (NSNotFound == newStruct.selectedDataArrayIndex)
		{
			newStruct.selectedDataArrayIndex = 0; // require a selection at all times
		}
		
		// notify the master of the new selection
		[self.masterDriver numberedListViewManager:self
													didChangeFromDataSet:&oldStruct
													toDataSet:&newStruct];
		
		// notify the detail view of the new selection
		[self.detailViewManager.delegate panelViewManager:self.detailViewManager
															didChangeFromDataSet:&oldStruct
															toDataSet:&newStruct];
		
		[self didChangeValueForKey:@"listItemBindingIndexes"];
	}
}// setListItemBindingIndexes:


#pragma mark NSSplitViewDelegate


/*!
Ensures that the right-hand side of the split does not
make the panel any smaller than its minimum size.

(2017.07)
*/
- (CGFloat)
splitView:(NSSplitView*)			sender
constrainMaxCoordinate:(CGFloat)	aProposedMaxX
ofSubviewAt:(NSInteger)				aDividerIndex
{
#pragma unused(sender)
	CGFloat		result = aProposedMaxX;
	
	
	assert(0 == aDividerIndex);
	
	// LOCALIZE THIS
	result = MIN(NSWidth(self.splitView.frame) - [self idealDetailContainerWidth], aProposedMaxX);
	
	return result;
}// splitView:constrainMaxCoordinate:ofSubviewAt:


/*!
Ensures that the right-hand side of the split does not
make the panel any smaller than its minimum size.

(2017.07)
*/
- (CGFloat)
splitView:(NSSplitView*)			sender
constrainMinCoordinate:(CGFloat)	aProposedMinX
ofSubviewAt:(NSInteger)				aDividerIndex
{
#pragma unused(sender)
	CGFloat		result = aProposedMinX;
	
	
	assert(0 == aDividerIndex);
	
	return result;
}// splitView:constrainMinCoordinate:ofSubviewAt:


/*!
Ensures that resizing applies to the panel, unless the
existing list size would cramp the panel below its
minimum size.

(2017.07)
*/
- (void)
splitView:(NSSplitView*)			sender
resizeSubviewsWithOldSize:(NSSize)	anOriginalSize
{
#pragma unused(sender)
	NSSize		newSize = self.splitView.frame.size;
	NSSize		sizeDifference = NSMakeSize(newSize.width - anOriginalSize.width,
											newSize.height - anOriginalSize.height);
	CGFloat		masterWidth = 0; // initially...
	CGFloat		detailWidth = 0; // initially...
	CGFloat		suggestedDetailWidth = MAX(NSWidth(self.detailContainer.frame) + (sizeDifference.width * 0.75/* arbitrary */),
											[self idealDetailContainerWidth]);
	
	
	// LOCALIZE THIS (assumes left-to-right arrangement)
	if (suggestedDetailWidth > [self idealDetailContainerWidth])
	{
		// when relatively large, assign an arbitrary width for the list
		// (note: this seems to avoid a LOT of quirks in NSSplitView)
		masterWidth = 280.0; // arbitrary
	}
	else
	{
		// distribute the change
		masterWidth = (newSize.width - suggestedDetailWidth - self.splitView.dividerThickness);
		if (masterWidth < 0)
		{
			masterWidth = 0;
		}
	}
	detailWidth = (newSize.width - masterWidth - self.splitView.dividerThickness);
	
	self.masterContainer.frame = NSMakeRect(0, 0, masterWidth, newSize.height);
	self.detailContainer.frame = NSMakeRect(self.masterContainer.frame.origin.x + NSWidth(self.masterContainer.frame) + self.splitView.dividerThickness,
											self.masterContainer.frame.origin.y, detailWidth, newSize.height);
}// splitView:resizeSubviewsWithOldSize:


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
	self->_itemBindingSortDescriptors = [@[
											// uses "id< GenericPanelNumberedList_ItemBinding >" method names
											[NSSortDescriptor sortDescriptorWithKey:@"numberedListIndexString" ascending:YES
																					selector:@selector(localizedStandardCompare:)],
											[NSSortDescriptor sortDescriptorWithKey:@"numberedListItemIconImage" ascending:YES
																					selector:@selector(imageNameCompare:)],
											[NSSortDescriptor sortDescriptorWithKey:@"numberedListItemName" ascending:YES
																					selector:@selector(localizedStandardCompare:)]
										] retain];
	self->_listItemBindingIndexes = [[NSIndexSet alloc] initWithIndex:0];
	self->_listItemBindings = [[NSArray alloc] init];
	self.masterDriver = givenDriver;
	_detailViewManager = [givenDetailViewManager retain];
	self.detailViewManager.panelParent = self;
	
	assert(nil != self.detailViewManager);
	
	[self.masterDriver initializeNumberedListViewManager:self];
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
	[self.detailViewManager.delegate panelViewManager:aViewManager requestingEditType:outEditType];
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
	assert(nil != self.itemArrayController);
	assert(nil != self.masterContainer);
	assert(nil != self.masterView);
	assert(nil != self.detailContainer);
	assert(nil != self.detailView);
	assert(nil != self.detailViewManager);
	assert(nil != self.splitView);
	assert(self == self.splitView.delegate);
	
	NSTabViewItem*		tabItem = [[NSTabViewItem alloc] initWithIdentifier:[self.detailViewManager panelIdentifier]];
	
	
	[tabItem setView:[self.detailViewManager managedView]];
	[tabItem setInitialFirstResponder:[self.detailViewManager logicalFirstResponder]];
	[self.detailView addTabViewItem:tabItem];
	[tabItem release];
	
	if ([self.masterDriver respondsToSelector:@selector(containerViewDidLoadForNumberedListViewManager:)])
	{
		[self.masterDriver containerViewDidLoadForNumberedListViewManager:self];
	}
	
	[[self.masterView enclosingScrollView] setNextKeyView:[self.detailViewManager logicalFirstResponder]];
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
	NSRect		containerFrame = [[self managedView] frame];
	NSRect		masterFrame = self.masterContainer.frame;
	//NSRect		detailFrame = self.detailContainer.frame;
	NSRect		defaultDetailViewFrame = self.detailView.frame;
	NSSize		panelIdealSize = defaultDetailViewFrame.size; // initially...
	
	
	[self.detailViewManager.delegate panelViewManager:aViewManager requestingIdealSize:&panelIdealSize];
	panelIdealSize.width = MAX(panelIdealSize.width, NSWidth(defaultDetailViewFrame));
	panelIdealSize.height = MAX(panelIdealSize.height, NSHeight(defaultDetailViewFrame));
	
	// set total size, including space required by numbered-list interface
	*outIdealSize = panelIdealSize;
	outIdealSize->width += (NSWidth(masterFrame) + self.splitView.dividerThickness);
	outIdealSize->height += (NSHeight(containerFrame) - NSHeight(defaultDetailViewFrame));
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
	[self.detailViewManager.delegate panelViewManager:self.detailViewManager didPerformContextSensitiveHelp:sender];
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
	[self.detailViewManager.delegate panelViewManager:self.detailViewManager willChangePanelVisibility:aVisibility];
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
	[self.detailViewManager.delegate panelViewManager:self.detailViewManager didChangePanelVisibility:aVisibility];
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
	NSUInteger							oldBindingIndex = [self.listItemBindingIndexes firstIndex];
	
	
	// the "listItemBindings" property needs to send update
	// notifications so that (for instance) list displays
	// will be refreshed
	[self willChangeValueForKey:@"listItemBindings"];
	
	bzero(&oldStruct, sizeof(oldStruct));
	oldStruct.parentPanelDataSetOrNull = oldDataSet;
	if (oldBindingIndex < [self.itemArrayController.arrangedObjects count])
	{
		oldStruct.selectedDataArrayIndex = [self.listItemBindings indexOfObject:[self.itemArrayController.arrangedObjects objectAtIndex:oldBindingIndex]];
	}
	if (NSNotFound == oldStruct.selectedDataArrayIndex)
	{
		oldStruct.selectedDataArrayIndex = 0; // require a selection at all times
	}
	bzero(&newStruct, sizeof(newStruct));
	newStruct.parentPanelDataSetOrNull = newDataSet;
	newStruct.selectedDataArrayIndex = 0; // reset the current selection
	
	[self.masterDriver numberedListViewManager:self
												didChangeFromDataSet:&oldStruct
												toDataSet:&newStruct];
	
	[self.detailViewManager.delegate panelViewManager:self.detailViewManager
														didChangeFromDataSet:&oldStruct
														toDataSet:&newStruct];
	
	[self didChangeValueForKey:@"listItemBindings"];
	
	// change the displayed selection
	self.listItemBindingIndexes = [NSIndexSet indexSetWithIndex:0];
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
	[self.detailViewManager.delegate panelViewManager:self.detailViewManager didFinishUsingContainerView:aContainerView userAccepted:isAccepted];
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
Returns the number of items that will be covered by a call
to "panelParentEnumerateChildViewManagers".

(2017.03)
*/
- (NSUInteger)
panelParentChildCount
{
	return 1;
}// panelParentChildCount


/*!
Returns an enumerator over the Panel_ViewManager* objects
for the panels in this view.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	return [[[@[self.detailViewManager] retain] autorelease] objectEnumerator];
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
	Panel_ResizeConstraint	result = [self.detailViewManager panelResizeAxes];
	
	
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
	
	
	if ([self.detailViewManager conformsToProtocol:@protocol(PrefsWindow_PanelInterface)])
	{
		result = [STATIC_CAST(self.detailViewManager, id< PrefsWindow_PanelInterface >) preferencesClass];
	}
	
	return result;
}// preferencesClass


@end // GenericPanelNumberedList_ViewManager


#pragma mark -
@implementation GenericPanelNumberedList_ViewManager (GenericPanelNumberedList_ViewManagerInternal) //{


/*!
Returns the “ideal” width for the "detailContainer" view, which
should represent the frame of the right-hand side of the split
view (useful in split-view delegate methods, for instance).

(2017.07)
*/
- (CGFloat)
idealDetailContainerWidth
{
	CGFloat		result = 0; // see below
	NSSize		panelIdealSize = self.detailViewManager.view.frame.size;
	
	
	// LOCALIZE THIS
	// IMPORTANT: For simplicity, the layout (in Interface Builder)
	// is such that the width of the embedded panel is identical to
	// the width of the "detailContainer".  This allows the ideal
	// width of the detail view to be sufficient to decide the width
	// of the entire detail container.  If that ever changes, e.g.
	// if border insets are introduced, this function must change.
	[self.detailViewManager.delegate panelViewManager:self.detailViewManager
														requestingIdealSize:&panelIdealSize];
	result = panelIdealSize.width;
	
	return result;
}// idealDetailContainerWidth


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"listItemBindingIndexes", @"listItemBindings"];
}


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
