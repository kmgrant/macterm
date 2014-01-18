/*!	\file Panel.h
	\brief Abstract interface to allow panel-based windows
	to be easily constructed.
	
	The C++ API is for Carbon legacy; Cocoa-based panels
	should use a Cocoa-based NIB file to define a container
	view and link it to a Panel_ViewManager subclass (the
	file’s owner).
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

#ifndef __PANEL__
#define __PANEL__

// standard-C++ includes
#include <functional>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif
#include <CoreServices/CoreServices.h>

// application includes
#include "Console.h"



#pragma mark Constants

typedef CFStringRef Panel_Kind;
CFStringRef const kPanel_InvalidKind = CFSTR("net.macterm.prefpanels.invalid");

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

typedef struct Panel_OpaqueRef*		Panel_Ref;

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
@interface Panel_ViewManager : NSObject //{
{
	IBOutlet NSView*	managedView;
	IBOutlet NSView*	logicalFirstResponder;
	IBOutlet NSView*	logicalLastResponder;
@private
	id< Panel_Delegate >	_delegate;
	SEL						_panelDisplayAction;
	id						_panelDisplayTarget;
	id< Panel_Parent >		_panelParent;
}

// initializers
	- (id)
	initWithNibNamed:(NSString*)_
	delegate:(id< Panel_Delegate >)_
	context:(void*)_;

// accessors
	@property (assign) id< Panel_Delegate >
	delegate;
	- (NSView*)
	logicalFirstResponder;
	- (NSView*)
	logicalLastResponder;
	- (NSView*)
	managedView;
	@property (assign) SEL
	panelDisplayAction;
	@property (assign) id
	panelDisplayTarget;
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

#pragma mark Callbacks

/*!
Panel State Changed Notification Method (Carbon Only)

This routine type is used to signal a panel when the
dialog box that contains it has changed some aspect of
the panel.  This kind of notification should always be
invoked by dialog box code when the code operates on a
panel.  A flexible dialog box will send every kind of
change notification, even seemingly unimportant ones
(such as changing the active state), so that any kind
of panel is definitely able to work in the dialog.
All dialogs must send the “one time” property changes
in a logical order.

NOTE:	Carbon Events have made the previous interface
		somewhat redundant.  If there seems to be
		something important missing here, look for an
		equivalent Carbon Event (e.g. install a
		kEventClassControl/kEventControlBoundsChanged
		event on the panel’s container view to find out
		when the panel size changes).

The possible messages are as follows:

kPanel_MessageCreateViews
	Carbon-based windows must fire this property change
	exactly once, immediately after the window is
	created.  This allows panels to create their views
	in the right window.

kPanel_MessageDestroyed
	This property indicates that the panel is just about
	to be disposed of.  Your routine should dispose of
	any private data structures that are only used by
	your panel.  This message is issued only when the
	Panel_Dispose() method is called.

kPanel_MessageFocusFirst
	This property indicates that the user interface is
	ready to be used and the keyboard focus should be
	set to the first logical view (typically a text
	field).  If this is not handled, some view will be
	focused arbitrarily by the container of the panel.

kPanel_MessageFocusGained
	This message is only sent from Carbon-based panels.
	This property indicates that the user has shifted
	focus to another view.  The given view is the one
	that has gained focus, and therefore is now taking
	keyboard input.  If the specified view is not one
	you recognize, do not do anything with it!

kPanel_MessageFocusLost
	This message is only sent from Carbon-based panels.
	This property indicates that the user has shifted
	focus to another view.  The given view is the one
	that has lost focus, and therefore is not taking
	keyboard input anymore.  If the specified view is
	not one you recognize, do not do anything with it!

kPanel_MessageGetEditType
	Sent to determine how a panel behaves: by default it
	is modeless and any changes to its fields affect a
	single set of data immediately.  But, an “inspector”
	type of panel is expected to be able to auto-save and
	reset its fields when the active data set is changed
	(see the kPanel_MessageNewDataSet event for more info).
	A theoretical 3rd mode NOT supported is full modality,
	where a panel is expected to completely confirm or
	discard all its changes before returning control to
	its caller.  Your handler should return the edit type
	"kPanel_ResponseEditTypeModelessSingle" or
	"kPanel_ResponseEditTypeInspector".

kPanel_MessageGetGrowBoxLook
	Sent each time the panel is displayed to determine how
	the window resize box should look (transparent or
	opaque).  It is transparent by default.  If your panel
	displays a list at the bottom or otherwise wants the
	boxed appearance, return "kPanel_ResponseGrowBoxOpaque",
	otherwise return kPanel_ResponseGrowBoxTransparent".

kPanel_MessageGetHelpKeyPhrase
	Sent each time the panel is displayed to determine the
	behavior of a dialog help button.  The Help System
	module uses key phrases identified by codes as a simple
	way to describe what search should occur when the dialog
	help button is clicked.  Return 0 to use the default
	phrase.

kPanel_MessageGetIdealSize
	Sent AFTER view creation to determine the ideal
	dimensions of the panel.  This might be used by the
	owning window to resize itself if it is too small.
	Your handler should fill in the data structure and
	return "kPanel_ResponseSizeProvided".

kPanel_MessageGetUsefulResizeAxes
	Offers a hint as to the kind of resizing that is most
	useful for the content.  Although all panels should be
	capable of resizing in any direction, some may only be
	useful to resize in one direction and some may not be
	worth resizing at all.  If a panel is wrapped in a
	window with a size box, this information might be used
	to determine if the window is even resizable.

kPanel_MessageNewAppearanceTheme
	This property indicates that the application has
	received a theme switch event, and therefore should
	recalculate the size of views.  The panel generally
	responds by re-setting its minimum size, which the
	dialog can subsequently check.

kPanel_MessageNewDataSet:
	Sent only for inspector panels (this attribute is set
	by handling kPanel_MessageGetEditType appropriately).
	Notifies a panel that the active data set has changed,
	so the panel should reset all its fields and update
	itself to match the specified new data set.

kPanel_MessageNewVisibility
	This property indicates that the window has hidden or
	shown a panel.  For some panels, such as ones that
	provide audible feedback, it may be important to know
	when the panel is no longer showing.  The window is
	free to fire this message when the panel may not be
	“really” invisible, but is obscured in some way (such
	as by another, larger view in the same window).
*/
typedef UInt32 Panel_Message;
enum
{
	kPanel_MessageCreateViews = '1win',					// data: -> HIWindowRef*, the owning Carbon window
	kPanel_MessageDestroyed = 'tobe',					// data: -> void*, the auxiliary data pointer
	kPanel_MessageFocusFirst = 'focf',					// data: -> nullptr
	kPanel_MessageFocusGained = 'focg',					// data: -> HIViewRef*, the field gaining focus
	kPanel_MessageFocusLost = 'focl',					// data: -> HIViewRef*, the field losing focus
	kPanel_MessageGetEditType = 'edit',					// data: -> nullptr
		kPanel_ResponseEditTypeInspector = 1,				// result code of the above
		kPanel_ResponseEditTypeModelessSingle = 0,			// result code of the above (default if panel does not provide)
	kPanel_MessageGetGrowBoxLook = 'grow',				// data: <- nullptr
		kPanel_ResponseGrowBoxOpaque = 1,					// result code of the above
		kPanel_ResponseGrowBoxTransparent = 0,				// result code of the above (default if panel does not provide)
	kPanel_MessageGetHelpKeyPhrase = 'help',			// data: <- HelpSystem_KeyPhrase
	kPanel_MessageGetIdealSize = 'idsz',				// data: <- HISize*
		kPanel_ResponseSizeProvided = 1,					// result code of the above
		kPanel_ResponseSizeNotProvided = 0,					// result code of the above (default if panel does not provide)
	kPanel_MessageGetUsefulResizeAxes = 'axes',			// data: <- nullptr
		kPanel_ResponseResizeVertical = 3,					// result code of the above
		kPanel_ResponseResizeHorizontal = 2,				// result code of the above
		kPanel_ResponseResizeNotNeeded = 1,					// result code of the above
		kPanel_ResponseResizeBothAxes = 0,					// result code of the above (default if panel does not provide)
	kPanel_MessageNewAppearanceTheme = 'athm',			// data: -> nullptr
	kPanel_MessageNewDataSet = 'cset',					// data: -> Panel_DataSetTransition*
	kPanel_MessageNewVisibility = 'visb'				// data: -> Boolean*, “is now visible?”
};
typedef SInt32 (*Panel_ChangeProcPtr)	(Panel_Ref		inRef,
										 Panel_Message	inMessage,
										 void*			inDataPtr);



#pragma mark Public Methods

//!\name Creating and Destroying Panels (Done by Customizers)
//@{

Panel_Ref
	Panel_New							(Panel_ChangeProcPtr		inProc);

void
	Panel_Dispose						(Panel_Ref*					inoutRefPtr);

//@}

//!\name Utilities
//@{

void
	Panel_CalculateTabFrame				(HIViewRef					inPanelContainerView,
										 Point*						outTabFrameTopLeft,
										 Point*						outTabFrameWidthHeight);

void
	Panel_CalculateTabFrame				(Float32					inPanelContainerWidth,
										 Float32					inPanelContainerHeight,
										 HIPoint&					outTabFrameTopLeft,
										 HISize&					outTabFrameWidthHeight);

void
	Panel_GetTabPaneInsets				(Point*						outTabPaneTopLeft,
										 Point*						outTabPaneBottomRight);

SInt32
	Panel_PropagateMessage				(Panel_Ref					inRef,
										 Panel_Message				inMessage,
										 void*						inDataPtr);

OSStatus
	Panel_SetButtonIcon					(HIViewRef					inView,
										 Panel_Ref					inRef);

OSStatus
	Panel_SetToolbarItemIconAndLabel	(HIToolbarItemRef			inItem,
										 Panel_Ref					inRef);

//@}

//!\name Methods for Dialogs
//@{

void
	Panel_GetContainerView				(Panel_Ref					inRef,
										 HIViewRef&					outView);

void
	Panel_GetDescription				(Panel_Ref					inRef,
										 CFStringRef&				outDescription);

void
	Panel_GetName						(Panel_Ref					inRef,
										 CFStringRef&				outName);

void
	Panel_GetPreferredSize				(Panel_Ref					inRef,
										 HISize&					outSize);

inline SInt32
	Panel_InvokeChangeProc				(Panel_ChangeProcPtr		inProc,
										 Panel_Ref					inRef,
										 Panel_Message				inMessage,
										 void*						inDataPtr)
	{
		return (*inProc)(inRef, inMessage, inDataPtr);
	}

UInt32
	Panel_ReturnShowCommandID			(Panel_Ref					inRef);

class Panel_Resizer; // STL Unary Function - returns: void, argument: Panel_Ref

void
	Panel_SendMessageCreateViews		(Panel_Ref					inRef,
										 HIWindowRef				inOwningWindow);

void
	Panel_SendMessageFocusFirst			(Panel_Ref					inRef);

void
	Panel_SendMessageFocusGained		(Panel_Ref					inRef,
										 HIViewRef					inViewGainingKeyboardFocus);

void
	Panel_SendMessageFocusLost			(Panel_Ref					inRef,
										 HIViewRef					inViewLosingKeyboardFocus);

SInt32
	Panel_SendMessageGetEditType		(Panel_Ref					inRef);

SInt32
	Panel_SendMessageGetGrowBoxLook		(Panel_Ref					inRef);

SInt32
	Panel_SendMessageGetHelpKeyPhrase	(Panel_Ref					inRef);

SInt32
	Panel_SendMessageGetIdealSize		(Panel_Ref					inRef,
										 HISize&					outData);

SInt32
	Panel_SendMessageGetUsefulResizeAxes	(Panel_Ref				inRef);

void
	Panel_SendMessageNewAppearanceTheme	(Panel_Ref					inRef);

void
	Panel_SendMessageNewDataSet			(Panel_Ref					inRef,
										 Panel_DataSetTransition const&);

void
	Panel_SendMessageNewVisibility		(Panel_Ref					inRef,
										 Boolean					inIsNowVisible);

void
	Panel_SetPreferredSize				(Panel_Ref					inRef,
										 HISize const&				inSize);

//@}

//!\name Methods for Panel Customization Code
//@{

void*
	Panel_ReturnImplementation			(Panel_Ref					inRef);

Panel_Kind
	Panel_ReturnKind					(Panel_Ref					inRef);

HIWindowRef
	Panel_ReturnOwningWindow			(Panel_Ref					inRef);

void
	Panel_SetContainerView				(Panel_Ref					inRef,
										 HIViewRef					inView);

void
	Panel_SetDescription				(Panel_Ref					inRef,
										 CFStringRef				inDescription);

void
	Panel_SetIconRef					(Panel_Ref					inRef,
										 SInt16						inIconServicesVolumeNumber,
										 OSType						inIconServicesCreator,
										 OSType						inIconServicesDescription);

void
	Panel_SetIconRefFromBundleFile		(Panel_Ref					inRef,
										 CFStringRef				inFileNameWithoutExtension,
										 OSType						inIconServicesCreator,
										 OSType						inIconServicesDescription);

void
	Panel_SetImplementation				(Panel_Ref					inRef,
										 void*						inAuxiliaryDataPtr);

void
	Panel_SetKind						(Panel_Ref					inRef,
										 Panel_Kind					inDescriptor);

void
	Panel_SetName						(Panel_Ref					inRef,
										 CFStringRef				inName);

void
	Panel_SetShowCommandID				(Panel_Ref					inRef,
										 UInt32						inCommandID);

//@}



#pragma mark Functor Implementations

/*!
This is a functor that adjusts the width and/or height
of a panel’s container view.

Model of STL Unary Function.

(3.0)
*/
#pragma mark Panel_Resizer
class Panel_Resizer:
public std::unary_function< Panel_Ref/* argument */, void/* return */ >
{
public:
	Panel_Resizer	(Float32	inNewWidth,
					 Float32	inNewHeight,
					 Boolean	inIsDelta = true)
	: _horizontal(inNewWidth), _vertical(inNewHeight), _delta(inIsDelta)
	{
	}
	
	void
	operator ()	(Panel_Ref	inPanel)
	{
		HIViewRef	container = nullptr;
		HIRect		newFrame;
		
		
		// adjust the size of this panel in the list
		Panel_GetContainerView(inPanel, container);
		assert_noerr(HIViewGetFrame(container, &newFrame));
		if (_delta)
		{
			newFrame.size.width += _horizontal;
			newFrame.size.height += _vertical;
		}
		else
		{
			newFrame.size.width = _horizontal;
			newFrame.size.height = _vertical;
		}
		assert_noerr(HIViewSetFrame(container, &newFrame));
	}

protected:

private:
	Float32		_horizontal;
	Float32		_vertical;
	Boolean		_delta;
};

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
