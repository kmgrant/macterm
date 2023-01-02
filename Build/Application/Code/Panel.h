/*!	\file Panel.h
	\brief Abstract interface to allow panel-based windows
	to be easily constructed.
	
	Panels should use a NIB file to define a container view and
	link it to a Panel_ViewManager subclass (the file’s owner),
	or they should call "initWithView:delegate:context:" to use
	a container NSView that is already in memory.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSString;
#endif
#include <CoreServices/CoreServices.h>

// application includes
#include "Console.h"



#pragma mark Constants

/*!
A panel should send this notification to the default center
when its "panelViewManager:requestingIdealSize:" method will
return a different value than before.  Panel containers
typically respond to this by adjusting a parent window size.

The notification should not be sent by most panels, as it is
not sent for the initial ideal-size value.

No userInfo is defined for this notification.  Panels typically
already know the original ideal size, and they can find the new
size by calling "panelViewManager:requestingIdealSize:" again.
*/
extern NSString* _Nonnull	kPanel_IdealSizeDidChangeNotification;

/*!
An “edit type” describes how a panel behaves: is it
implicitly used for a single data store or can it
represent other data stores (e.g. in a master-detail
view)?
*/
enum Panel_EditType
{
	kPanel_EditTypeNormal = 0,		//!< always overwrites a single data source
	kPanel_EditTypeInspector = 1	//!< is expected to be able to change data sources at will, updating the UI
};

/*!
Specifies which resize behavior is sensible for the panel.
Useful in aggregates (like tab views) to decide how the
overall window should behave.
*/
enum Panel_ResizeConstraint
{
	kPanel_ResizeConstraintBothAxes = 0,	//!< both horizontal and vertical resizes make sense
	kPanel_ResizeConstraintNone = 1,		//!< the panel should not be resized
	kPanel_ResizeConstraintHorizontal = 2,	//!< only left or right resizes are supported
	kPanel_ResizeConstraintVertical = 3		//!< only top or bottom resizes are supported
};

/*!
A state of visibility helps panels to decide what they
should enable (e.g. sounds or animations, or auxiliary
floating windows).
*/
enum Panel_Visibility
{
	kPanel_VisibilityDisplayed = 0,		//!< panel would normally be seen by the user (though its window may be hidden)
	kPanel_VisibilityObscured = 1,		//!< panel would normally be seen by the user but it is not visible (e.g. window or application hidden);
										//!  this state might be used to disable resource-intensive things that have no purpose when the panel
										//!  cannot be seen by the user, such as animations
	kPanel_VisibilityHidden = 2			//!< panel is explicitly set to an invisible state (e.g. tab view has another tab in front)
};

#pragma mark Types

struct Panel_DataSetTransition
{
	void* _Nullable		oldDataSet;		// set to nullptr if not applicable (e.g. deleted)
	void* _Nullable		newDataSet;		// set to nullptr for a full reset with no new data (e.g. select nothing)
};

#ifdef __OBJC__

@class Panel_ViewManager;

/*!
Classes that are delegates of Panel_ViewManager must
conform to this protocol.
*/
@protocol Panel_Delegate < NSObject > //{

@required

	// superclass minimally initialized, no NIB loaded yet; perform subclass initializations needed this early, e.g. so that NIB-provided bindings succeed
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	initializeWithContext:(NSObject* _Nullable)_;

	// manager needs to know how the panel behaves; respond by filling in the "requestingEditType:" parameter
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	requestingEditType:(Panel_EditType* _Nonnull)_;

	// view containing the panel has been loaded but no window has been created yet (WARNING: subclasses that delegate to themselves will not be initialized yet)
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	didLoadContainerView:(NSView* _Nonnull)_;

	// manager needs to know the size the panel would prefer to have; respond by filling in the "requestingIdealSize:" parameter
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	requestingIdealSize:(NSSize* _Nonnull)_;

	// user has requested context-sensitive help; argument to "didPerformContextSensitiveHelp:" is the sender of the action
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	didPerformContextSensitiveHelp:(id _Nullable)_;

	// view will be redisplayed or obscured (e.g. in a tab view, because another tab is about to be displayed)
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	willChangePanelVisibility:(Panel_Visibility)_;

	// view has now been redisplayed or obscured (e.g. in a tab view, because another tab has been displayed)
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	didChangePanelVisibility:(Panel_Visibility)_;

	// data set to be represented by the view has changed; for inspector-style views this can happen more than once
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	didChangeFromDataSet:(void* _Nullable)_
	toDataSet:(void* _Nullable)_;

	// sent when containing window, etc. will go away; save settings if accepted but no need to update the user interface because it will be destroyed
	- (void)
	panelViewManager:(Panel_ViewManager* _Nonnull)_
	didFinishUsingContainerView:(NSView* _Nonnull)_
	userAccepted:(BOOL)_;

@end //}

/*!
Classes that conform to this protocol are responsible for
managing multiple child panels.
*/
@protocol Panel_Parent < NSObject > //{

@required

	// sent when a particular child (e.g. a tab in a tab view) should become visible and focused
	- (void)
	panelParentDisplayChildWithIdentifier:(NSString* _Nonnull)_
	withAnimation:(BOOL)_;

	// respond with the number of items that "panelParentEnumerateChildViewManagers" would cover
	- (NSUInteger)
	panelParentChildCount;

	// respond with an ordered enumeration of all Panel_ViewManager* values managed by the parent
	- (NSEnumerator* _Nonnull)
	panelParentEnumerateChildViewManagers;

@end //}


/*!
Loads a NIB file containing a single primary view bound
to an owning object that subclasses Panel_ViewManager.

The delegate specifies how the view is to be used in a
larger context (e.g. a tab view in a window).

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Panel_ViewManager : NSViewController //{

// initializers
	- (instancetype _Nullable)
	initWithNibNamed:(NSString* _Nonnull)_
	delegate:(id< Panel_Delegate > _Nullable)_
	context:(NSObject* _Nullable)_ NS_DESIGNATED_INITIALIZER;
	- (instancetype _Nullable)
	initWithView:(NSView* _Nonnull)_
	delegate:(id< Panel_Delegate > _Nullable)_
	context:(NSObject* _Nullable)_ NS_DESIGNATED_INITIALIZER;

// accessors: other
	//! This object is used to customize panel behavior, and is
	//! almost certainly needed to produce the desired results. 
	@property (assign, nullable) id< Panel_Delegate >
	delegate;
	//! The "panelDisplayAction" is sent to "panelDisplayTarget"
	//! when the user wants this panel to appear.  This could
	//! be used by a menu command or other special feature of
	//! the UI to cause a panel to appear in an unusual way;
	//! for example, Preferences window panels are often shown
	//! through this action.
	//!
	//! The Panel_Parent protocol defines the method
	//! "panelParentDisplayChildWithIdentifier:withAnimation:",
	//! which can be implemented with the help of these target
	//! and action property values. 
	@property (assign, nullable) SEL
	panelDisplayAction;
	//! The object that "panelDisplayAction" should be sent to.
	@property (assign, nullable) id
	panelDisplayTarget;
	//! Set if this panel has any useful help action.  This is
	//! given a default value of YES only if the "delegate"
	//! has "panelViewManager:didPerformContextSensitiveHelp:"
	//! implemented.  You can also set it yourself before the
	//! panel appears.
	@property (assign) BOOL
	panelHasContextualHelp;
	//! This should be set by special panels that act as parents
	//! of other panels (e.g. the Preferences window, a set of
	//! tabs, a master-detail view, or similar construct).  This
	//! property allows a sub-panel to easily find its direct
	//! parent panel.
	//!
	//! See the "Panel_Parent" protocol above for details on the
	//! APIs of parents; "panelParentEnumerateChildViewManagers"
	//! for example is a way to go from parent to child.
	@property (assign, nullable) id< Panel_Parent >
	panelParent; // should be set by parents when adding or removing children

// accessors: read-only values
	//! Use to ensure that user interface elements are fully
	//! defined before doing things that may depend on the UI.
	@property (readonly) BOOL
	isPanelUserInterfaceLoaded;
	//! This is the main view, it contains the entire panel.
	//! This property exists for historical reasons, before the
	//! base class of Panel_ViewManager was an NSViewController.
	//! Now, this has the same value as "self.view" but note that
	//! a Panel does not expect its view to change dynamically so
	//! setting "self.view" is not recommended.
	@property (readonly, nonnull) NSView*
	managedView;
	//! Returns the type of editing that this panel does: either
	//! it edits a single data set, or it is able to continuously
	//! update itself as data sets are changed (see the delegate
	//! method "panelViewManager:didChangeFromDataSet:toDataSet:").
	//!
	//! This invokes "panelViewManager:requestingEditType:" on the
	//! delegate.
	@property (readonly) Panel_EditType
	panelEditType;

// accessors: XIB outlets
	//! Returns the view that a window ought to focus first using
	//! NSWindow’s "makeFirstResponder:".  The actual first responder
	//! at runtime will depend on what else is in the window, e.g.
	//! there may be a parent panel with other controls that will
	//! logically precede those in this panel.
	//!
	//! When a XIB is used, this is required at XIB loading time.
	//! If "initWithView:delegate:context:" is used though, this
	//! property may be set directly before the panel is displayed.
	@property (assign, nullable) IBOutlet NSView*
	logicalFirstResponder;
	//! The last view of the panel that can receive focus for user input.
	//!
	//! When a XIB is used, this is required at XIB loading time.
	//! If "initWithView:delegate:context:" is used though, this
	//! property may be set directly before the panel is displayed.
	@property (assign, nullable) IBOutlet NSView*
	logicalLastResponder;

// actions
	- (IBAction)
	orderFrontContextualHelp:(id _Nullable)_;
	- (IBAction)
	performCloseAndAccept:(id _Nullable)_;
	- (IBAction)
	performCloseAndDiscard:(id _Nullable)_;
	- (IBAction)
	performDisplaySelfThroughParent:(id _Nullable)_;

// overrides for subclasses
	- (NSImage* _Nonnull)
	panelIcon; // required (not implemented in base)
	- (NSString* _Nonnull)
	panelIdentifier; // required (not implemented in base)
	- (NSString* _Nonnull)
	panelName; // required (not implemented in base)
	- (Panel_ResizeConstraint)
	panelResizeAxes; // required (not implemented in base)

@end //}

#else

// this is mostly a useless declaration (as it is Objective-C only)
// though it is helpful if other code refers to it only as a pointer
class Panel_ViewManager;

#endif // __OBJC__

// BELOW IS REQUIRED NEWLINE TO END FILE
