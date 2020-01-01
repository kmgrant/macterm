/*!	\file VectorWindow.mm
	\brief A window with a vector graphics canvas.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#import "VectorWindow.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstdio>
#import <cstring>

// standard-C++ includes
#import <set>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <ListenerModel.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlocks.h>
#import <TouchBar.objc++.h>
#import <WindowTitleDialog.h>

// application includes
#import "Console.h"
#import "ConstantsRegistry.h"
#import "EventLoop.h"
#import "Preferences.h"
#import "VectorInterpreter.h"



#pragma mark Types
namespace {

typedef std::set< VectorWindow_Controller* >	My_VectorWindowRefTracker;

} // anonymous namespace

#pragma mark Internal Method Prototypes

/*!
The private class interface.
*/
@interface VectorWindow_Controller (VectorWindow_ControllerInternal) //{

// class methods
	+ (VectorWindow_Controller*)
	windowControllerForWindowRef:(VectorWindow_Ref)_;

// new methods
	- (void)
	onEvent:(VectorWindow_Event)_
	notifyListener:(ListenerModel_ListenerRef)_;
	- (void)
	removeListener:(ListenerModel_ListenerRef)_
	forEvent:(VectorWindow_Event)_;

// accessors
	- (VectorWindow_Ref)
	canvasWindow;
	- (VectorInterpreter_Ref)
	interpreterRef;
	- (void)
	setInterpreterRef:(VectorInterpreter_Ref)_;

// notifications
	- (void)
	windowWillClose:(NSNotification*)_;

@end //}

#pragma mark Variables
namespace {

My_VectorWindowRefTracker&		gVectorCanvasWindowValidRefs ()	{ static My_VectorWindowRefTracker x; return x; }

} // anonymous namespace


#pragma mark Public Methods

/*!
Creates a new window controller.

Once a window is created with the specified interpreter, the
interpreter is retained (it is released when the window is
closed or the window controller is otherwise deallocated).

Note that the corresponding window of this controller retains
*itself* so ultimately the user is in control of its demise.
Releasing the reference you create will not be sufficient to
kill the controller, the window itself must also be closed.
If you must “force” the controller to die, close its window
first.

(4.1)
*/
VectorWindow_Ref
VectorWindow_New	(VectorInterpreter_Ref		inData)
{
	VectorWindow_Controller*	controller = [[VectorWindow_Controller alloc] initWithInterpreter:inData];
	
	
	return [controller canvasWindow];
}// New


/*!
Adds a lock on the specified reference.  This indicates you are
using the window, so attempts by anyone else to delete the window
with VectorWindow_Release() will fail until you release your lock
(and everyone else releases locks they may have).

(4.1)
*/
void
VectorWindow_Retain		(VectorWindow_Ref	inWindow)
{
	VectorWindow_Controller*	controller = VectorWindow_ReturnController(inWindow);
	
	
	[controller retain];
}// Retain


/*!
Releases one lock on the specified window created with
VectorWindow_New() and deletes the window *if* no other
locks remain.  Your copy of the reference is set to
nullptr.

(4.1)
*/
void
VectorWindow_Release	(VectorWindow_Ref*		inoutRefPtr)
{
	if (nullptr != inoutRefPtr)
	{
		VectorWindow_Controller*	controller = VectorWindow_ReturnController(*inoutRefPtr);
		
		
		[controller release], *inoutRefPtr = nullptr;
	}
}// Release


/*!
Provides a copy of the window title string, or nullptr on error.

(4.1)
*/
void
VectorWindow_CopyTitle	(VectorWindow_Ref	inWindow,
						 CFStringRef&		outTitle)
{
	VectorWindow_Controller*	controller = VectorWindow_ReturnController(inWindow);
	
	
	outTitle = nullptr;
	if (nil != controller)
	{
		NSWindow*	owningWindow = [controller window];
		
		
		outTitle = BRIDGE_CAST([[owningWindow title] copy], CFStringRef);
	}
}// CopyTitle


/*!
Displays the given window, makes it key and orders it in front.
This API only exists to allow non-Cocoa modules to access the
capabilities of the underlying Objective-C window object.

\retval kVectorWindow_ResultOK
if there are no errors

\retval kVectorWindow_ResultInvalidReference
if the specified window controller is unrecognized

(4.1)
*/
VectorWindow_Result
VectorWindow_Display	(VectorWindow_Ref	inWindow)
{
	VectorWindow_Result			result = kVectorWindow_ResultInvalidReference;
	VectorWindow_Controller*	controller = VectorWindow_ReturnController(inWindow);
	
	
	if (nil != controller)
	{
		// show the window, which will also trigger an initial rendering of the canvas
		[[controller window] makeKeyAndOrderFront:nil];
		result = kVectorWindow_ResultOK;
	}
	return result;
}// Display


/*!
Returns the controller of the specified window, or nil if it
has been released.

This indirection prevents invalid window references from being
used (a possibility in external modules).

(4.1)
*/
VectorWindow_Controller*
VectorWindow_ReturnController	(VectorWindow_Ref	inWindow)
{
	VectorWindow_Controller*	result = [VectorWindow_Controller windowControllerForWindowRef:inWindow];
	
	
	if (gVectorCanvasWindowValidRefs().end() == gVectorCanvasWindowValidRefs().find(result))
	{
		// a reference that has been released is a bad reference
		result = nil;
	}
	return result;
}// ReturnController


/*!
Returns the "interpreterRef" property of the window controller,
or nullptr if the window is invalid.

(4.1)
*/
VectorInterpreter_Ref
VectorWindow_ReturnInterpreter		(VectorWindow_Ref	inWindow)
{
	return [VectorWindow_ReturnController(inWindow) interpreterRef];
}// ReturnInterpreter


/*!
Returns the "window" property of the window controller, or
nil if the window is invalid.

(4.1)
*/
NSWindow*
VectorWindow_ReturnNSWindow		(VectorWindow_Ref	inWindow)
{
	return [VectorWindow_ReturnController(inWindow) window];
}// ReturnNSWindow


/*!
Sets the name of the graphics window.

\retval kVectorWindow_ResultOK
if there are no errors

\retval kVectorWindow_ResultInvalidReference
if the specified window controller is unrecognized

(4.1)
*/
VectorWindow_Result
VectorWindow_SetTitle	(VectorWindow_Ref	inWindow,
						 CFStringRef		inTitle)
{
	VectorWindow_Result			result = kVectorWindow_ResultInvalidReference;
	VectorWindow_Controller*	controller = VectorWindow_ReturnController(inWindow);
	
	
	if (nil != controller)
	{
		NSWindow*	owningWindow = [controller window];
		
		
		[owningWindow setTitle:(NSString*)inTitle];
		result = kVectorWindow_ResultOK;
	}
	return result;
}// SetTitle


/*!
Arranges for a callback to be invoked whenever the given
event occurs for a vector graphics window.

\retval kVectorWindow_ResultOK
if there are no errors

\retval kVectorWindow_ResultInvalidReference
if the specified window controller is unrecognized

(4.1)
*/
VectorWindow_Result
VectorWindow_StartMonitoring	(VectorWindow_Ref			inWindow,
								 VectorWindow_Event			inForWhatEvent,
								 ListenerModel_ListenerRef	inListener)
{
	VectorWindow_Result			result = kVectorWindow_ResultInvalidReference;
	VectorWindow_Controller*	controller = VectorWindow_ReturnController(inWindow);
	
	
	if (nil != controller)
	{
		[controller onEvent:inForWhatEvent notifyListener:inListener];
		result = kVectorWindow_ResultOK;
	}
	return result;
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked whenever
the given event occurs for a vector graphics window.
This reverses the effects of a previous call to
VectorWindow_StartMonitoring().

\retval kVectorWindow_ResultOK
if there are no errors

\retval kVectorWindow_ResultInvalidReference
if the specified window controller is unrecognized

(4.1)
*/
VectorWindow_Result
VectorWindow_StopMonitoring		(VectorWindow_Ref			inWindow,
								 VectorWindow_Event			inForWhatEvent,
								 ListenerModel_ListenerRef	inListener)
{
	VectorWindow_Result			result = kVectorWindow_ResultInvalidReference;
	VectorWindow_Controller*	controller = VectorWindow_ReturnController(inWindow);
	
	
	if (nil != controller)
	{
		[controller removeListener:inListener forEvent:inForWhatEvent];
		result = kVectorWindow_ResultOK;
	}
	return result;
}// StopMonitoring


#pragma mark Internal Methods


#pragma mark -
@implementation VectorWindow_Controller


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithInterpreter:(VectorInterpreter_Ref)		anInterpreter
{
	self = [super initWithWindowNibName:@"VectorWindowCocoa"];
	if (nil != self)
	{
		VectorInterpreter_Retain(anInterpreter);
		self->selfRef = [self canvasWindow];
		self->changeListenerModel = ListenerModel_New(kListenerModel_StyleStandard, 'VWIN');
		self->interpreterRef = anInterpreter;
		self->renameDialog = nullptr;
		self->_touchBarController = nil; // created on demand
		gVectorCanvasWindowValidRefs().insert(self);
	}
	return self;
}// initWithInterpreter:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	// kick the window out of Full Screen if necessary
	if (EventLoop_IsWindowFullScreen(self.window))
	{
		[self.window toggleFullScreen:NSApp];
	}
	
	[_touchBarController release];
	
	gVectorCanvasWindowValidRefs().erase(self);
	[self ignoreWhenObjectsPostNotes];
	[canvasView release];
	if (nullptr != changeListenerModel)
	{
		ListenerModel_Dispose(&changeListenerModel);
	}
	if (nullptr != renameDialog)
	{
		WindowTitleDialog_Dispose(&renameDialog);
	}
	if (nullptr != interpreterRef)
	{
		VectorInterpreter_Release(&interpreterRef);
	}
	
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Displays an interface for setting the title of the canvas window.

(4.1)
*/
- (IBAction)
performRename:(id)	sender
{
#pragma unused(sender)
	if (nullptr == self->renameDialog)
	{
		NSWindow*	windowRef = self.window; // keep "self" from being retained in the blocks below
		Boolean		noAnimations = false;
		
		
		// determine if animation should occur
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNoAnimations,
									sizeof(noAnimations), &noAnimations))
		{
			noAnimations = false; // assume a value, if preference can’t be found
		}
		
		// create the rename interface, specify how to initialize it
		// and specify how to update the title when finished
		self->renameDialog = WindowTitleDialog_NewWindowModal
								(windowRef, (false == noAnimations),
								^()
								{
									// initialize with existing title
									return BRIDGE_CAST(windowRef.title, CFStringRef); // note: need to return a copy
								},
								^(CFStringRef	inNewTitle)
								{
									// if non-nullptr, set new title
									if (nullptr == inNewTitle)
									{
										// user cancelled; ignore
									}
									else
									{
										windowRef.title = BRIDGE_CAST(inNewTitle, NSString*);
									}
								});
	}
	WindowTitleDialog_Display(self->renameDialog);
}
- (id)
canPerformRename:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (VectorCanvas_View*)
canvasView
{
	return canvasView;
}
- (void)
setCanvasView:(VectorCanvas_View*)	aView
{
	if (canvasView != aView)
	{
		if (nil != canvasView)
		{
			[canvasView release];
		}
		if (nil != aView)
		{
			[aView retain];
		}
		canvasView = aView;
	}
}// setCanvasView:


#pragma mark TerminalView_ClickDelegate


/*!
Invoked when a mouse-down event occurs in a background
view (such as the matte).  This responds by translating
the coordinates and then pretending that the event
occurred at the translated location in the canvas view.

This is crucial for usability, as it allows the user to
be “sloppy” when clicking near the edges of the canvas
while still having those clicks take effect.

(2018.06)
*/
- (void)
didReceiveMouseDownEvent:(NSEvent*)		anEvent
forView:(NSView*)						aView
{
#pragma unused(aView)
	// WARNING: the target view must directly handle this event and
	// not forward it to the first responder, otherwise the system
	// would probably enter a recursive loop
	[self.canvasView mouseDown:anEvent];
}// didReceiveMouseDownEvent:forView:


/*!
Invoked when a mouse-dragged event occurs in a background
view (such as the matte).  This responds by translating
the coordinates and then pretending that the event
occurred at the translated location in the canvas view.

This is crucial for usability, as it allows the user to
be “sloppy” when clicking near the edges of the canvas
while still having those clicks take effect.

(2018.06)
*/
- (void)
didReceiveMouseDraggedEvent:(NSEvent*)	anEvent
forView:(NSView*)						aView
{
#pragma unused(aView)
	// WARNING: the target view must directly handle this event and
	// not forward it to the first responder, otherwise the system
	// would probably enter a recursive loop
	[self.canvasView mouseDragged:anEvent];
}// didReceiveMouseDraggedEvent:forView:


/*!
Invoked when a mouse-up event occurs in a background
view (such as the matte).  This responds by translating
the coordinates and then pretending that the event
occurred at the translated location in the canvas view.

This is crucial for usability, as it allows the user to
be “sloppy” when clicking near the edges of the canvas
while still having those clicks take effect.

(2018.06)
*/
- (void)
didReceiveMouseUpEvent:(NSEvent*)	anEvent
forView:(NSView*)					aView
{
#pragma unused(aView)
	// WARNING: the target view must directly handle this event and
	// not forward it to the first responder, otherwise the system
	// would probably enter a recursive loop
	[self.canvasView mouseUp:anEvent];
}// didReceiveMouseUpEvent:forView:


#pragma mark NSResponder


/*!
On OS 10.12.1 and beyond, returns a Touch Bar to display
at the top of the hardware keyboard (when available) or
in any Touch Bar simulator window.

This method should not be called except by the OS.

(2016.11)
*/
- (NSTouchBar*)
makeTouchBar
{
	NSTouchBar*		result = nil;
	
	
	if (nil == _touchBarController)
	{
		_touchBarController = [[TouchBar_Controller alloc] initWithNibName:@"VectorWindowTouchBarCocoa"];
		_touchBarController.customizationIdentifier = kConstantsRegistry_TouchBarIDVectorWindowMain;
		_touchBarController.customizationAllowedItemIdentifiers =
		@[
			kConstantsRegistry_TouchBarItemIDFullScreen,
			FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFlexibleSpace",
							NSTouchBarItemIdentifierFlexibleSpace),
			FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceSmall",
							NSTouchBarItemIdentifierFixedSpaceSmall),
			FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceLarge",
							NSTouchBarItemIdentifierFixedSpaceLarge)
		];
		// (NOTE: default item identifiers are set in the NIB)
	}
	
	// the controller should force the NIB to load and define
	// the Touch Bar, using the settings above and in the NIB
	result = _touchBarController.touchBar;
	assert(nil != result);
	
	return result;
}// makeTouchBar


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.1)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	assert(nil != canvasView);
	assert(nil != matteView);
	
	matteView.clickDelegate = self;
	
	[canvasView setInterpreterRef:[self interpreterRef]];
	
	if ([self.window respondsToSelector:@selector(setAnimationBehavior:)])
	{
		//[self.window setAnimationBehavior:FUTURE_SYMBOL(3, NSWindowAnimationBehaviorDocumentWindow)];
		[self.window setAnimationBehavior:NSWindowAnimationBehaviorDocumentWindow];
	}
	
	// enable Full Screen
	[self.window setCollectionBehavior:([self.window collectionBehavior] | FUTURE_SYMBOL(1 << 7, NSWindowCollectionBehaviorFullScreenPrimary))];
	
	[self whenObject:self.window postsNote:NSWindowWillCloseNotification
						performSelector:@selector(windowWillClose:)];
}// windowDidLoad


@end // VectorWindow_Controller


#pragma mark -
@implementation VectorWindow_Controller (VectorWindow_ControllerInternal)


/*!
Accessor.

(4.1)
*/
+ (VectorWindow_Controller*)
windowControllerForWindowRef:(VectorWindow_Ref)		aWindow
{
	return REINTERPRET_CAST(aWindow, VectorWindow_Controller*);
}// windowControllerForWindowRef:


#pragma mark New Methods


/*!
Installs a handler for an event.

(4.1)
*/
- (void)
onEvent:(VectorWindow_Event)				anEvent
notifyListener:(ListenerModel_ListenerRef)	aListener
{
	ListenerModel_AddListenerForEvent(self->changeListenerModel, anEvent, aListener);
}// onEvent:notifyListener:


/*!
Removes a handler for an event.

(4.1)
*/
- (void)
removeListener:(ListenerModel_ListenerRef)	aListener
forEvent:(VectorWindow_Event)				anEvent
{
	ListenerModel_RemoveListenerForEvent(self->changeListenerModel, anEvent, aListener);
}// removeListener:forEvent:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (VectorWindow_Ref)
canvasWindow
{
	return REINTERPRET_CAST(self, VectorWindow_Ref);
}// canvasWindow


/*!
Accessor.

(4.1)
*/
- (VectorInterpreter_Ref)
interpreterRef
{
	return interpreterRef;
}
- (void)
setInterpreterRef:(VectorInterpreter_Ref)	anInterpreter
{
	if (interpreterRef != anInterpreter)
	{
		if (nullptr != interpreterRef)
		{
			VectorInterpreter_Release(&interpreterRef);
		}
		if (nullptr != anInterpreter)
		{
			VectorInterpreter_Retain(anInterpreter);
		}
		interpreterRef = anInterpreter;
	}
}// setInterpreterRef:


#pragma mark Notifications


/*!
Ensures that the canvas is released when the window closes.

(4.1)
*/
- (void)
windowWillClose:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	// notify listeners outside this module
	ListenerModel_NotifyListenersOfEvent(self->changeListenerModel, kVectorWindow_EventWillClose, self->selfRef/* context */);
}// windowWillClose:


@end // VectorWindow_Controller (VectorWindow_ControllerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
