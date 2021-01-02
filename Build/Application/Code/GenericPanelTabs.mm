/*!	\file GenericPanelTabs.mm
	\brief Implements a panel that can contain others.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#import "GenericPanelTabs.h"
#import <UniversalDefines.h>

// standard-C++ includes
#import <map>
#import <utility>

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

// note: a std::map is used because NSView does not meet the requirements
// for the key type of an NSDictionary
typedef std::map< NSView*, Panel_ViewManager* >		GenericPanelTabs_ViewManagerByView;

/*!
Private properties.
*/
@interface GenericPanelTabs_ViewManager () //{

// accessors
	//! Panel associated with currently-selected tab.
	@property (weak) Panel_ViewManager*
	activePanel;
	//! The panel’s unique identication string in dotted-name format.
	@property (strong, nonnull) NSString*
	customPanelIdentifier;
	//! The panel’s user-visible icon image.
	@property (strong, nonnull) NSImage*
	customPanelLocalizedIcon;
	//! The panel’s user-visible title.
	@property (strong, nonnull) NSString*
	customPanelLocalizedName;
	//! All view controllers represented by panel tabs.
	@property (strong, nonnull) NSArray*
	viewManagerArray;
	//! Allows view controllers to be found based on the views they control.
	@property (assign, nonnull) GenericPanelTabs_ViewManagerByView*
	viewManagerByView;

@end //}


#pragma mark Public Methods

#pragma mark -
@implementation GenericPanelTabs_ViewManager


#pragma mark Initializers


/*!
Designated initializer.

The array must consist of view manager objects that are
consistent with the usage of the tabs panel itself; for
instance if you intend to put the tabs into the Preferences
window then EVERY view manager in the array must support
PrefsWindow_PanelInterface; otherwise they need only
support Panel_Delegate.

(4.1)
*/
- (instancetype)
initWithIdentifier:(NSString*)	anIdentifier
localizedName:(NSString*)		aName
localizedIcon:(NSImage*)		anImage
viewManagerArray:(NSArray*)		anArray
{
	NSDictionary*	contextDictionary = @{
											@"identifier": anIdentifier,
											@"localizedName": aName,
											@"localizedIcon": anImage,
											@"viewManagerArray": anArray,
										};
	
	
	self = [super initWithNibNamed:@"GenericPanelTabsCocoa" delegate:self context:contextDictionary];
	if (nil != self)
	{
		for (Panel_ViewManager* viewManager in self.viewManagerArray)
		{
			viewManager.panelParent = self;
		}
	}
	return self;
}// initWithIdentifier:localizedName:localizedIcon:viewManagerArray:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	for (Panel_ViewManager* viewManager in self.viewManagerArray)
	{
		viewManager.panelParent = nil;
	}
	delete _viewManagerByView;
}// dealloc


#pragma mark Initializers Disabled From Superclass


/*!
Compiler expects this superclass designated initializer to
be defined but this variant is not supported.

(2021.01)
*/
- (instancetype)
initWithNibNamed:(NSString*)		aNibName
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithIdentifier:nil localizedName:nil localizedIcon:nil viewManagerArray:nil];
}// initWithNibNamed:delegate:context:


/*!
Compiler expects this superclass designated initializer to
be defined but this variant is not supported.

(2021.01)
*/
- (instancetype)
initWithView:(NSView*)				aView
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithIdentifier:nil localizedName:nil localizedIcon:nil viewManagerArray:nil];
}// initWithView:delegate:context:


#pragma mark Actions


/*!
Responds to a title segment selection by switching the
active panel.

It is not possible to tell NSSegmentedControl to bind
different actions to each title button so this single
entry point responds to any change in selection.

(2019.07)
*/
- (void)
performTitleSegmentUpdate:(id)	sender
{
#pragma unused(sender)
	if (STATIC_CAST(self.tabTitles.selectedSegment, NSUInteger) >= self.viewManagerArray.count)
	{
		Console_Warning(Console_WriteLine, "current segment index is larger than number of panels; ignoring action");
	}
	else
	{
		Panel_ViewManager*	viewMgr = [self.viewManagerArray objectAtIndex:self.tabTitles.selectedSegment];
		
		
		[self.tabView selectTabViewItemWithIdentifier:viewMgr.panelIdentifier];
	}
}// performTitleSegmentUpdate:


#pragma mark NSColorPanel


/*!
Forwards this message to the active tab’s panel if the active
tab’s panel implements a "changeColor:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeColor:(id)	sender
{
	if ([self.activePanel respondsToSelector:@selector(changeColor:)])
	{
		[self.activePanel changeColor:sender];
	}
}// changeColor:


#pragma mark NSFontPanel


/*!
Forwards this message to the active tab’s panel if the active
tab’s panel implements a "changeFont:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeFont:(id)		sender
{
	if ([self.activePanel respondsToSelector:@selector(changeFont:)])
	{
		[self.activePanel changeFont:sender];
	}
}// changeFont:


#pragma mark NSTabViewDelegate


/*!
Keeps track of the current tab so that certain messages can be
forwarded to the appropriate panel.

(4.1)
*/
- (void)
tabView:(NSTabView*)					aTabView
didSelectTabViewItem:(NSTabViewItem*)	anItem
{
#pragma unused(aTabView)
	auto				toPair = self.viewManagerByView->find(anItem.view);
	Panel_ViewManager*	newPanel = ((self.viewManagerByView->end() != toPair)
									? toPair->second
									: nil);
	
	
	if (nil != self.activePanel)
	{
		[self.activePanel.delegate panelViewManager:self.activePanel didChangePanelVisibility:kPanel_VisibilityHidden];
	}
	
	if (nil != newPanel)
	{
		// find segment
		NSUInteger	i = 0;
		for (Panel_ViewManager* viewMgr in self.viewManagerArray)
		{
			if ([viewMgr.panelIdentifier isEqualToString:newPanel.panelIdentifier])
			{
				[self.tabTitles setSelected:YES forSegment:i];
				break;
			}
			++i;
		}
		
		self.activePanel = newPanel;
		[self.activePanel.delegate panelViewManager:self.activePanel didChangePanelVisibility:kPanel_VisibilityDisplayed];
	}
	else
	{
		Console_Warning(Console_WriteLine, "tab view has selected item with no associated view manager");
		self.activePanel = nil;
	}
}// tabView:didSelectTabViewItem:


/*!
Notifies the proposed panel that it is about to become
visible, and notifies any current panel that it is
about to become invisible.

(2016.09)
*/
- (void)
tabView:(NSTabView*)					aTabView
willSelectTabViewItem:(NSTabViewItem*)	anItem
{
#pragma unused(aTabView)
	auto				toPair = self.viewManagerByView->find([anItem view]);
	Panel_ViewManager*	newPanel = ((self.viewManagerByView->end() != toPair)
									? toPair->second
									: nil);
	
	
	if (nil != self.activePanel)
	{
		[self.activePanel.delegate panelViewManager:self.activePanel willChangePanelVisibility:kPanel_VisibilityHidden];
	}
	
	if (nil != newPanel)
	{
		[newPanel.delegate panelViewManager:newPanel willChangePanelVisibility:kPanel_VisibilityDisplayed];
	}
}// tabView:willSelectTabViewItem:


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext
{
#pragma unused(aViewManager)
	// IMPORTANT: see the initializer for the construction of this dictionary and the names of keys that are used
	NSDictionary*	asDictionary = (NSDictionary*)aContext;
	NSString*		givenIdentifier = [asDictionary objectForKey:@"identifier"];
	NSString*		givenName = [asDictionary objectForKey:@"localizedName"];
	NSImage*		givenIcon = [asDictionary objectForKey:@"localizedIcon"];
	NSArray*		givenViewManagers = [asDictionary objectForKey:@"viewManagerArray"];
	
	
	self.activePanel = nil;
	self.customPanelIdentifier = givenIdentifier;
	self.customPanelLocalizedName = givenName;
	self.customPanelLocalizedIcon = givenIcon;
	self.viewManagerArray = [givenViewManagers copy];
	
	// an NSView* object is not a valid key in an NSMutableDictionary
	// but it will certainly work as a key in an STL map
	self.viewManagerByView = new GenericPanelTabs_ViewManagerByView();
	for (Panel_ViewManager* viewMgr in givenViewManagers)
	{
		self.viewManagerByView->insert(std::make_pair([viewMgr managedView], viewMgr));
	}
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
	// consult all tabs
	*outEditType = kPanel_EditTypeNormal;
	for (Panel_ViewManager* viewMgr in self.viewManagerArray)
	{
		Panel_EditType		newEditType = kPanel_EditTypeNormal;
		
		
		[viewMgr.delegate panelViewManager:viewMgr requestingEditType:&newEditType];
		switch (newEditType)
		{
		case kPanel_EditTypeNormal:
			// ignore in case any panel requires inspector-mode
			break;
		case kPanel_EditTypeInspector:
		default:
			*outEditType = newEditType;
			break;
		}
	}
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
	assert(nil != self.tabTitles);
	assert(nil != self.tabView);
	
	// arrange to be notified of certain changes
	self.tabView.delegate = self;
	
	// create tabs for every view that was provided
	if (@available(macOS 11.0, *))
	{
		// if possible, make tabs larger and shift down to make room
		self.tabTitles.controlSize = NSControlSizeLarge;
		self.tabTitles.frame = NSMakeRect(self.tabTitles.frame.origin.x,
											self.tabTitles.frame.origin.y - 6/* arbitrary shift for larger size */,
											self.tabTitles.frame.size.width,
											self.tabTitles.frame.size.height);
	}
	self.tabTitles.segmentCount = self.viewManagerArray.count;
	self.tabTitles.target = self;
	self.tabTitles.action = @selector(performTitleSegmentUpdate:);
	NSUInteger	i = 0;
	for (Panel_ViewManager* viewMgr in self.viewManagerArray)
	{
		NSTabViewItem*	tabItem = [[NSTabViewItem alloc] initWithIdentifier:[viewMgr panelIdentifier]];
		
		
		tabItem.label = [viewMgr panelName];
		tabItem.view = [viewMgr managedView];
		tabItem.initialFirstResponder = [viewMgr logicalFirstResponder];
		
		[self.tabView addTabViewItem:tabItem];
		
		[self.tabTitles setLabel:[viewMgr panelName] forSegment:i];
		[self.tabTitles setSelected:(0 == i) forSegment:i];
		++i;
	}
	[self.tabTitles sizeToFit];
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
	NSRect			containerFrame = [[self managedView] frame];
	NSRect			tabContentRect = [self.tabView contentRect];
	
	
	*outIdealSize = containerFrame.size;
	
	// find ideal size after considering all tabs
	for (Panel_ViewManager* viewMgr in self.viewManagerArray)
	{
		NSSize		panelIdealSize = *outIdealSize;
		
		
		[viewMgr.delegate panelViewManager:viewMgr requestingIdealSize:&panelIdealSize];
		outIdealSize->width = MAX(panelIdealSize.width, outIdealSize->width);
		outIdealSize->height = MAX(panelIdealSize.height, outIdealSize->height);
	}
	
	// add in the additional space required by the tabs themselves
	outIdealSize->width += (NSWidth(containerFrame) - NSWidth(tabContentRect));
	outIdealSize->height += (NSHeight(containerFrame) - NSHeight(tabContentRect));
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
	// forward to active tab
	[self.activePanel.delegate panelViewManager:self.activePanel didPerformContextSensitiveHelp:sender];
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
	// forward to active tab
	[self.activePanel.delegate panelViewManager:self.activePanel willChangePanelVisibility:aVisibility];
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager, aVisibility)
	// forward to active tab
	[self.activePanel.delegate panelViewManager:self.activePanel didChangePanelVisibility:aVisibility];
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

The message is forwarded to each tab’s panel in turn.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
	// forward to all tabs
	for (Panel_ViewManager* viewMgr in self.viewManagerArray)
	{
		[viewMgr.delegate panelViewManager:viewMgr didChangeFromDataSet:oldDataSet toDataSet:newDataSet];
	}
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).

The message is forwarded to each tab’s panel in turn.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager, aContainerView, isAccepted)
	// forward to all tabs
	for (Panel_ViewManager* viewMgr in self.viewManagerArray)
	{
		[viewMgr.delegate panelViewManager:viewMgr didFinishUsingContainerView:aContainerView userAccepted:isAccepted];
	}
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_Parent


/*!
Selects the tab with the given panel identifier, or does
nothing if the identifier does not match any tab.

Currently the animation flag has no effect.

(4.1)
*/
- (void)
panelParentDisplayChildWithIdentifier:(NSString*)	anIdentifier
withAnimation:(BOOL)								isAnimated
{
#pragma unused(isAnimated)
	// when the tab view items are created their tab view item
	// identifiers are made the same as their panel identifiers
	// so it is possible to simply ask the tab view to select
	// a tab with the given identifier (this will fail however
	// if the identifier does not match any tab)
	@try
	{
		[self.tabView selectTabViewItemWithIdentifier:anIdentifier];
	}
	@catch (NSException*	anException)
	{
		Console_Warning(Console_WriteValueCFString, "tab view was unable to display child panel with identifier", BRIDGE_CAST(anIdentifier, CFStringRef));
	}
}// panelParentDisplayChildWithIdentifier:withAnimation:


/*!
Returns the number of items that will be covered by a call
to "panelParentEnumerateChildViewManagers".

(2017.03)
*/
- (NSUInteger)
panelParentChildCount
{
	return self.viewManagerArray.count;
}// panelParentChildCount


/*!
Returns an enumerator over the Panel_ViewManager* objects
for the tabs in this view.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	return [self.viewManagerArray objectEnumerator];
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
	return self.customPanelLocalizedIcon;
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return self.customPanelIdentifier;
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
	return self.customPanelLocalizedName;
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
	// consult all tabs and return the tightest constraint possible
	Panel_ResizeConstraint	result = kPanel_ResizeConstraintBothAxes;
	
	
	for (Panel_ViewManager* viewMgr in self.viewManagerArray)
	{
		Panel_ResizeConstraint	panelConstraint = [viewMgr panelResizeAxes];
		
		
		switch (panelConstraint)
		{
		case kPanel_ResizeConstraintNone:
			// completely restrictive; keep
			result = panelConstraint;
			break;
		case kPanel_ResizeConstraintHorizontal:
			// partially restrictive; merge
			switch (result)
			{
			case kPanel_ResizeConstraintHorizontal:
			case kPanel_ResizeConstraintNone:
				// do not change
				break;
			case kPanel_ResizeConstraintVertical:
				// more restrictive
				result = kPanel_ResizeConstraintNone;
				break;
			case kPanel_ResizeConstraintBothAxes:
			default:
				result = panelConstraint;
				break;
			}
			break;
		case kPanel_ResizeConstraintVertical:
			// partially restrictive; merge
			switch (result)
			{
			case kPanel_ResizeConstraintVertical:
			case kPanel_ResizeConstraintNone:
				// do not change
				break;
			case kPanel_ResizeConstraintHorizontal:
				// more restrictive
				result = kPanel_ResizeConstraintNone;
				break;
			case kPanel_ResizeConstraintBothAxes:
			default:
				result = panelConstraint;
				break;
			}
			break;
		case kPanel_ResizeConstraintBothAxes:
		default:
			// not restrictive; ignore
			break;
		}
	}
	return result;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

IMPORTANT:	This interface is implemented for convenience but may
			only be used if the delegates of each view manager
			(each tab) support "PrefsWindow_PanelInterface".

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	// consult all tabs to ensure that they all agree on this
	Quills::Prefs::Class	result = Quills::Prefs::GENERAL;
	
	
	for (Panel_ViewManager* viewMgr in self.viewManagerArray)
	{
		id									panelDelegate = viewMgr.delegate;
		assert([[panelDelegate class] conformsToProtocol:@protocol(PrefsWindow_PanelInterface)]);
		id< PrefsWindow_PanelInterface >	asPrefPanel = (id< PrefsWindow_PanelInterface >)panelDelegate;
		
		
		result = [asPrefPanel preferencesClass];
	}
	return result;
}// preferencesClass


@end // GenericPanelTabs_ViewManager

// BELOW IS REQUIRED NEWLINE TO END FILE
