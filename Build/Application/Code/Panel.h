/*!	\file Panel.h
	\brief Abstract interface to allow panel-based windows
	to be easily constructed.
	
	Panels should use a NIB file to define a container view and
	link it to a Panel_ViewManager subclass (the file’s owner).
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#ifndef __PANEL__
#define __PANEL__

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
extern NSString*	kPanel_IdealSizeDidChangeNotification;

/*!
An “edit type” describes how a panel behaves: is it
implicitly used for a single data store or can it
represent other data stores (e.g. in a master-detail
view)?  Cocoa only.
*/
enum Panel_EditType
{
	kPanel_EditTypeNormal = 0,		//!< always overwrites a single data source
	kPanel_EditTypeInspector = 1	//!< is expected to be able to change data sources at will, updating the UI
};

/*!
Specifies which resize behavior is sensible for the panel.
Useful in aggregates (like tab views) to decide how the
overall window should behave.  Cocoa only.
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
floating windows).  Cocoa only.
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
	void*	oldDataSet;		// set to nullptr if not applicable (e.g. deleted)
	void*	newDataSet;		// set to nullptr for a full reset with no new data (e.g. select nothing)
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
	panelViewManager:(Panel_ViewManager*)_
	initializeWithContext:(void*)_;

	// manager needs to know how the panel behaves; respond by filling in the "requestingEditType:" parameter
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	requestingEditType:(Panel_EditType*)_;

	// view containing the panel has been loaded but no window has been created yet (WARNING: subclasses that delegate to themselves will not be initialized yet)
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	didLoadContainerView:(NSView*)_;

	// manager needs to know the size the panel would prefer to have; respond by filling in the "requestingIdealSize:" parameter
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	requestingIdealSize:(NSSize*)_;

	// user has requested context-sensitive help; argument to "didPerformContextSensitiveHelp:" is the sender of the action
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	didPerformContextSensitiveHelp:(id)_;

	// view will be redisplayed or obscured (e.g. in a tab view, because another tab is about to be displayed)
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	willChangePanelVisibility:(Panel_Visibility)_;

	// view has now been redisplayed or obscured (e.g. in a tab view, because another tab has been displayed)
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	didChangePanelVisibility:(Panel_Visibility)_;

	// data set to be represented by the view has changed; for inspector-style views this can happen more than once
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	didChangeFromDataSet:(void*)_
	toDataSet:(void*)_;

	// sent when containing window, etc. will go away; save settings if accepted but no need to update the user interface because it will be destroyed
	- (void)
	panelViewManager:(Panel_ViewManager*)_
	didFinishUsingContainerView:(NSView*)_
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
	panelParentDisplayChildWithIdentifier:(NSString*)_
	withAnimation:(BOOL)_;

	// respond with an ordered enumeration of all Panel_ViewManager* values managed by the parent
	- (NSEnumerator*)
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
{
	IBOutlet NSView*	logicalFirstResponder;
	IBOutlet NSView*	logicalLastResponder;
@private
	BOOL					_isPanelUserInterfaceLoaded;
	BOOL					_panelHasContextualHelp;
	id< Panel_Delegate >	_delegate;
	SEL						_panelDisplayAction;
	id						_panelDisplayTarget;
	id< Panel_Parent >		_panelParent;
}

// initializers
	- (instancetype)
	initWithNibNamed:(NSString*)_
	delegate:(id< Panel_Delegate >)_
	context:(void*)_;

// accessors
	@property (assign) id< Panel_Delegate >
	delegate;
	@property (readonly) BOOL
	isPanelUserInterfaceLoaded;
	- (NSView*)
	logicalFirstResponder;
	- (NSView*)
	logicalLastResponder;
	@property (readonly) NSView*
	managedView;
	@property (assign) SEL
	panelDisplayAction;
	@property (assign) id
	panelDisplayTarget;
	@property (assign) BOOL
	panelHasContextualHelp;
	@property (readonly) Panel_EditType
	panelEditType;
	@property (assign) id< Panel_Parent >
	panelParent; // should be set by parents when adding or removing children

// actions
	- (IBAction)
	performCloseAndAccept:(id)_;
	- (IBAction)
	performCloseAndDiscard:(id)_;
	- (IBAction)
	performContextSensitiveHelp:(id)_;
	- (IBAction)
	performDisplaySelfThroughParent:(id)_;

// overrides for subclasses
	- (NSImage*)
	panelIcon; // required (not implemented in base)
	- (NSString*)
	panelIdentifier; // required (not implemented in base)
	- (NSString*)
	panelName; // required (not implemented in base)
	- (Panel_ResizeConstraint)
	panelResizeAxes; // required (not implemented in base)

@end //}

#else

// this is mostly a useless declaration (as it is Objective-C only)
// though it is helpful if other code refers to it only as a pointer
class Panel_ViewManager;

#endif // __OBJC__

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
