/*!	\file Panel.mm
	\brief Abstract interface to allow panel-based windows
	to be easily constructed.
	
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

#import "Panel.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>


#pragma mark Constants

NSString*	kPanel_IdealSizeDidChangeNotification =
				@"kPanel_IdealSizeDidChangeNotification";

#pragma mark Public Methods

#pragma mark -
@implementation Panel_ViewManager //{


@synthesize delegate = _delegate;
@synthesize isPanelUserInterfaceLoaded = _isPanelUserInterfaceLoaded;
@synthesize panelDisplayAction = _panelDisplayAction;
@synthesize panelDisplayTarget = _panelDisplayTarget;
@synthesize panelHasContextualHelp = _panelHasContextualHelp;
@synthesize panelParent = _panelParent;


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithCoder:(NSCoder*)	aCoder
{
#pragma unused(aCoder)
	assert(false && "invalid way to initialize derived class");
	return [self initWithNibNamed:@"" delegate:nil context:nil];
}// initWithCoder:


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithNibName:(NSString*)		aNibName
bundle:(NSBundle*)				aBundle
{
#pragma unused(aNibName, aBundle)
	assert(false && "invalid way to initialize derived class");
	return [self initWithNibNamed:@"" delegate:nil context:nil];
}// initWithNibName:bundle:


/*!
Designated initializer.

(2021.01)
*/
- (instancetype)
initWithNibNamed:(NSString*)		aNibName
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	self = [super initWithNibName:aNibName bundle:nil];
	if (nil != self)
	{
		_delegate = aDelegate;
		_isPanelUserInterfaceLoaded = NO;
		_panelDisplayAction = nil;
		_panelDisplayTarget = nil;
		_panelParent = nil;
		
		// by default, assume that a panel has no help if the method to
		// respond to help is not implemented (the property can still
		// be changed however)
		self.panelHasContextualHelp = [self.delegate respondsToSelector:@selector(panelViewManager:didPerformContextSensitiveHelp:)];
		
		// since NIBs can construct lots of objects and bindings it is
		// actually pretty important to have an early hook for subclasses
		// (subclasses may need to initialize certain data in themselves
		// to ensure that their bindings actually succeed)
		[self.delegate panelViewManager:self initializeWithContext:aContext];
		
		// NSViewController implicitly loads the NIB when the "view"
		// property is accessed; force that here
		[self view];
	}
	return self;
}// initWithNibNamed:delegate:context:


/*!
Designated initializer, alternate (when view exists).

(2021.01)
*/
- (instancetype)
initWithView:(NSView*)				aView
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	self = [super initWithNibName:nil bundle:nil];
	if (nil != self)
	{
		_delegate = aDelegate;
		_isPanelUserInterfaceLoaded = NO;
		_panelDisplayAction = nil;
		_panelDisplayTarget = nil;
		_panelParent = nil;
		
		// by default, assume that a panel has no help if the method to
		// respond to help is not implemented (the property can still
		// be changed however)
		self.panelHasContextualHelp = [self.delegate respondsToSelector:@selector(panelViewManager:didPerformContextSensitiveHelp:)];
		
		// when no NIB is present, "view" must be assigned
		self.view = aView;
		assert(nil != self.view);
		
		// since NIBs can construct lots of objects and bindings it is
		// actually pretty important to have an early hook for subclasses
		// (subclasses may need to initialize certain data in themselves
		// to ensure that their bindings actually succeed)
		[self.delegate panelViewManager:self initializeWithContext:aContext];
	}
	return self;
}// initWithView:delegate:context


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Actions


/*!
Instructs the view to display context-sensitive help (e.g. when
the user clicks the help button).

Panel implementations should set the "panelHasContextualHelp"
property if this method is implemented, as panel parents may
decide to hide and/or disable a help button based on that
property.

(2018.11)
*/
- (IBAction)
orderFrontContextualHelp:(id)	sender
{
	[self.delegate panelViewManager:self didPerformContextSensitiveHelp:sender];
}// orderFrontContextualHelp:


/*!
Instructs the view to save all changes and prepare to be torn down
(e.g. in a modal sheet, when the user clicks OK).

(4.1)
*/
- (IBAction)
performCloseAndAccept:(id)	sender
{
#pragma unused(sender)
	[self.delegate panelViewManager:self didFinishUsingContainerView:self.view userAccepted:YES];
}// performCloseAndAccept:


/*!
Instructs the view to discard all changes and prepare to be torn
down (e.g. in a modal sheet, when the user clicks Cancel).

(4.1)
*/
- (IBAction)
performCloseAndDiscard:(id)		sender
{
#pragma unused(sender)
	[self.delegate panelViewManager:self didFinishUsingContainerView:self.view userAccepted:NO];
}// performCloseAndDiscard:


/*!
Ensures that this panel is displayed, or does nothing if no
parent has been assigned.

The parent’s chain is first displayed by invoking the method
"performDisplaySelfThroughParent:" on the parent.  Then,
using this panel’s "panelIdentifier" as an argument, the
parent’s "panelParentDisplayChildWithIdentifier:withAnimation:"
is used to request that this panel be revealed.

For child panels it can be useful to set this selector as the
"panelDisplayAction" property, with the panel itself as the
"panelDisplayTarget".

(4.1)
*/
- (IBAction)
performDisplaySelfThroughParent:(id)	sender
{
#pragma unused(sender)
	if (nil != self.panelParent)
	{
		// not all parents are necessarily panels but if they are,
		// invoke the method all the way up the chain
		if ([self.panelParent respondsToSelector:@selector(performDisplaySelfThroughParent:)])
		{
			[REINTERPRET_CAST(self.panelParent, id) performDisplaySelfThroughParent:nil];
		}
		
		[self.panelParent panelParentDisplayChildWithIdentifier:[self panelIdentifier] withAnimation:YES];
	}
	else
	{
		Console_Warning(Console_WriteValueCFString, "invocation of 'performDisplaySelfThroughParent:' on orphan panel with identifier",
						BRIDGE_CAST([self panelIdentifier], CFStringRef));
	}
}// performDisplaySelfThroughParent:


#pragma mark Accessors


/*!
Returns the view that a window ought to focus first
using NSWindow’s "makeFirstResponder:".

(4.1)
*/
- (NSView*)
logicalFirstResponder
{
	return self->logicalFirstResponder;
}// logicalFirstResponder


/*!
Returns the last view of the panel that can receive focus
for user input.

When panels are instantiated in elaborate interfaces, this
property is used to “connect” the chain of focused views
seamlessly (e.g. so that the last focused view of the
custom panel can Tab directly into the rest of the user
interface, and vice-versa).  A customizing panel should
call NSView’s "setNextKeyView:" appropriately.

(4.1)
*/
- (NSView*)
logicalLastResponder
{
	return self->logicalLastResponder;
}// logicalLastResponder


/*!
Returns the view that contains the entire panel.

NOTE:	This is now redundant with the "view" property of
		the parent NSViewController.  This method may be
		removed in the future.

(4.1)
*/
- (NSView*)
managedView
{
	return self.view;
}// managedView


/*!
Returns the type of editing that this panel does: either
it edits a single data set, or it is able to continuously
update itself as data sets are changed (see the delegate
method "panelViewManager:didChangeFromDataSet:toDataSet:").

This invokes "panelViewManager:requestingEditType:" on the
delegate.

(4.1)
*/
- (Panel_EditType)
panelEditType
{
	Panel_EditType		result = kPanel_EditTypeNormal;
	
	
	[self.delegate panelViewManager:self requestingEditType:&result];
	return result;
}// panelEditType


#pragma mark Overrides for Subclasses


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

This must be implemented by all subclasses.

(4.1)
*/
- (NSImage*)
panelIcon
{
	NSAssert(false, @"panelIcon method must be implemented by Panel_ViewManager subclasses");
	return nil;
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

This must be implemented by all subclasses.

(4.1)
*/
- (NSString*)
panelIdentifier
{
	NSAssert(false, @"panelIdentifier method must be implemented by Panel_ViewManager subclasses");
	return nil;
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

This must be implemented by all subclasses.

(4.1)
*/
- (NSString*)
panelName
{
	NSAssert(false, @"panelName method must be implemented by Panel_ViewManager subclasses");
	return @"";
}// panelName


/*!
Returns information on which directions are most useful for
resizing the panel.  For instance a window container may
disallow vertical resizing if no panel in the window has
any reason to resize vertically.

This must be implemented by all subclasses.

IMPORTANT:	This is only a hint.  Panels must be prepared
			to resize in both directions.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	NSAssert(false, @"panelResizeAxes method must be implemented by Panel_ViewManager subclasses");
	return kPanel_ResizeConstraintBothAxes;
}// panelResizeAxes


#pragma mark NSViewController


/*!
Invoked by NSViewController once the "self.view" property is set,
after the NIB file is loaded.  This essentially guarantees that
all file-defined user interface elements are now instantiated and
other settings that depend on valid UI objects can now be made.

NOTE:	As future SDKs are adopted, it makes more sense to only
		implement "viewDidLoad" (which was only recently added
		to NSViewController and is not otherwise available).
		This implementation can essentially move to "viewDidLoad".

(4.1)
*/
- (void)
loadView
{
	[super loadView];
	
	assert(nil != logicalFirstResponder);
	assert(nil != logicalLastResponder);
	
	self->_isPanelUserInterfaceLoaded = YES;
	
	[self.delegate panelViewManager:self didLoadContainerView:self.view];
}// loadView


@end //} Panel_ViewManager

// BELOW IS REQUIRED NEWLINE TO END FILE
