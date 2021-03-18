/*!	\file VectorWindow.mm
	\brief A window with a vector graphics canvas.
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
#import <Registrar.template.h>
#import <WindowTitleDialog.h>

// application includes
#import "Console.h"
#import "ConstantsRegistry.h"
#import "EventLoop.h"
#import "Preferences.h"
#import "VectorInterpreter.h"



#pragma mark Types
namespace {

typedef MemoryBlockReferenceTracker< VectorWindow_Ref >		My_RefTracker;
typedef Registrar< VectorWindow_Ref, My_RefTracker >		My_RefRegistrar;

/*!
Internal representation of a VectorWindow_Ref.
*/
struct My_VectorWindow
{
	My_VectorWindow		(VectorInterpreter_Ref);
	~My_VectorWindow ();
	
	My_RefRegistrar						refValidator;		// ensures this reference is recognized as a valid one
	VectorWindow_Ref					selfRef;			// redundant reference to self, for convenience
	VectorWindow_Controller* __strong	windowController;	// controller for the main NSWindow (responds to events, etc.)
};
typedef My_VectorWindow*		My_VectorWindowPtr;
typedef My_VectorWindow const*	My_VectorWindowConstPtr;

typedef std::set< VectorWindow_Ref >										My_VectorWindowRefTracker;
typedef MemoryBlockPtrLocker< VectorWindow_Ref, My_VectorWindow >			My_VectorWindowPtrLocker;
typedef LockAcquireRelease< VectorWindow_Ref, My_VectorWindow >				My_VectorWindowAutoLocker;
typedef MemoryBlockReferenceLocker< VectorWindow_Ref, My_VectorWindow >		My_VectorWindowReferenceLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes

/*!
Private properties.
*/
@interface VectorWindow_Controller () //{

// accessors
	@property (assign) ListenerModel_Ref
	changeListenerModel;
	//! Created on demand by "makeTouchBar"; this is different
	//! than the NSResponder "touchBar" property, which has
	//! side effects such as archiving, etc.
	@property (strong) NSTouchBar*
	dynamicTouchBar;
	@property (assign) VectorInterpreter_Ref
	interpreterRef;
	@property (strong) WindowTitleDialog_Ref
	renameDialog;

@end //}


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

// notifications
	- (void)
	windowWillClose:(NSNotification*)_;

@end //}

#pragma mark Variables
namespace {

My_RefTracker&					gVectorWindowValidRefs ()	{ static My_RefTracker x; return x; }
My_VectorWindowPtrLocker&		gVectorWindowPtrLocks ()	{ static My_VectorWindowPtrLocker x; return x; }

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
	VectorWindow_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_VectorWindow(inData),
									VectorWindow_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


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
	if (gVectorWindowPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteLine, "attempt to dispose of locked vector graphics window");
	}
	else
	{
		// clean up
		{
			My_VectorWindowAutoLocker	ptr(gVectorWindowPtrLocks(), *inoutRefPtr);
			
			
			delete ptr;
		}
		*inoutRefPtr = nullptr;
	}
}// Release


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
	VectorWindow_Controller*	result = nil;
	
	
	// a reference that has been released is a bad reference
	if (gVectorWindowValidRefs().end() != gVectorWindowValidRefs().find(inWindow))
	{
		result = [VectorWindow_Controller windowControllerForWindowRef:inWindow];
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
		
		
		owningWindow.title = BRIDGE_CAST(inTitle, NSString*);
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
namespace {

/*!
Constructor.  See VectorWindow_New().

(2021.01)
*/
My_VectorWindow::
My_VectorWindow		(VectorInterpreter_Ref		inData)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
refValidator(REINTERPRET_CAST(this, VectorWindow_Ref), gVectorWindowValidRefs()),
selfRef(REINTERPRET_CAST(this, VectorWindow_Ref)),
windowController([[VectorWindow_Controller alloc]
					initWithInterpreter:inData
										owner:REINTERPRET_CAST(this, VectorWindow_Ref)])
{
}// MyVectorWindow constructor


/*!
Destructor.  See VectorWindow_Release().

(2021.01)
*/
My_VectorWindow::
~My_VectorWindow ()
{
}// MyVectorWindow destructor

} // anonymous namespace


#pragma mark -
@implementation VectorWindow_Controller //{


@synthesize interpreterRef = _interpreterRef;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithInterpreter:(VectorInterpreter_Ref)		anInterpreter
owner:(VectorWindow_Ref)						aVectorWindowRef
{
	self = [super initWithWindowNibName:@"VectorWindowCocoa"];
	if (nil != self)
	{
		_changeListenerModel = ListenerModel_New(kListenerModel_StyleStandard, 'VWIN');
		_dynamicTouchBar = nil; // created on demand
		_interpreterRef = anInterpreter;
		VectorInterpreter_Retain(anInterpreter);
		_renameDialog = nullptr;
		_vectorWindowRef = aVectorWindowRef;
	}
	return self;
}// initWithInterpreter:owner:


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
	
	[self ignoreWhenObjectsPostNotes];
	if (nullptr != self.changeListenerModel)
	{
		ListenerModel_Dispose(&_changeListenerModel);
	}
	if (nullptr != self.interpreterRef)
	{
		VectorInterpreter_Release(&_interpreterRef);
	}
}// dealloc


#pragma mark Actions: Commands_WindowRenaming


/*!
Displays an interface for setting the title of the canvas window.

(4.1)
*/
- (IBAction)
performRename:(id)	sender
{
#pragma unused(sender)
	if (nullptr == self.renameDialog)
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
		self.renameDialog = WindowTitleDialog_NewWindowModal
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
	WindowTitleDialog_Display(self.renameDialog);
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
- (VectorInterpreter_Ref)
interpreterRef
{
	return _interpreterRef;
}
- (void)
setInterpreterRef:(VectorInterpreter_Ref)	anInterpreter
{
	if (_interpreterRef != anInterpreter)
	{
		if (nullptr != _interpreterRef)
		{
			VectorInterpreter_Release(&_interpreterRef);
		}
		if (nullptr != anInterpreter)
		{
			VectorInterpreter_Retain(anInterpreter);
		}
		_interpreterRef = anInterpreter;
	}
}// setInterpreterRef:


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
	NSTouchBar*		result = self.dynamicTouchBar;
	
	
	if (nil == result)
	{
		self.dynamicTouchBar = result = [[NSTouchBar alloc] init];
		result.customizationIdentifier = kConstantsRegistry_TouchBarIDVectorWindowMain;
		result.customizationAllowedItemIdentifiers =
		@[
			kConstantsRegistry_TouchBarItemIDFullScreen,
			NSTouchBarItemIdentifierFlexibleSpace,
			NSTouchBarItemIdentifierFixedSpaceSmall,
			NSTouchBarItemIdentifierFixedSpaceLarge
		];
		result.defaultItemIdentifiers =
		@[
			kConstantsRegistry_TouchBarItemIDFullScreen,
			NSTouchBarItemIdentifierFlexibleSpace,
			NSTouchBarItemIdentifierOtherItemsProxy,
		];
		NSButtonTouchBarItem*		fullScreenItem = [NSButtonTouchBarItem
														buttonTouchBarItemWithIdentifier:kConstantsRegistry_TouchBarItemIDFullScreen
																							image:[NSImage imageNamed:NSImageNameTouchBarEnterFullScreenTemplate]
																							target:nil
																							action:@selector(toggleFullScreen:)];
		fullScreenItem.customizationLabel = NSLocalizedString(@"Full Screen", @"“Full Screen” touch bar item customization palette label");
		if (nil == fullScreenItem)
		{
			NSLog(@"failed to construct one or more vector graphics touch bar items!");
		}
		else
		{
			result.templateItems = [NSSet setWithArray:@[ fullScreenItem ]];
		}
	}
	
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
	assert(nil != self.canvasView);
	assert(nil != self.matteView);
	
	self.matteView.clickDelegate = self;
	
	self.canvasView.interpreterRef = self.interpreterRef;
	
	self.window.animationBehavior = NSWindowAnimationBehaviorDocumentWindow;
	
	// enable Full Screen
	self.window.collectionBehavior = (self.window.collectionBehavior | NSWindowCollectionBehaviorFullScreenPrimary);
	
	[self whenObject:self.window postsNote:NSWindowWillCloseNotification
						performSelector:@selector(windowWillClose:)];
	
	self.window.initialFirstResponder = self.canvasView;
}// windowDidLoad


@end //} VectorWindow_Controller


#pragma mark -
@implementation VectorWindow_Controller (VectorWindow_ControllerInternal) //{


/*!
Accessor.

(4.1)
*/
+ (VectorWindow_Controller*)
windowControllerForWindowRef:(VectorWindow_Ref)		aWindow
{
	VectorWindow_Controller*	result = nil;
	My_VectorWindowAutoLocker	ptr(gVectorWindowPtrLocks(), aWindow);
	
	
	if (nullptr != ptr)
	{
		result = ptr->windowController;
	}
	
	return result;
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
	UNUSED_RETURN(Boolean)ListenerModel_AddListenerForEvent(self.changeListenerModel, anEvent, aListener);
}// onEvent:notifyListener:


/*!
Removes a handler for an event.

(4.1)
*/
- (void)
removeListener:(ListenerModel_ListenerRef)	aListener
forEvent:(VectorWindow_Event)				anEvent
{
	ListenerModel_RemoveListenerForEvent(self.changeListenerModel, anEvent, aListener);
}// removeListener:forEvent:


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
	ListenerModel_NotifyListenersOfEvent(self.changeListenerModel, kVectorWindow_EventWillClose, self.vectorWindowRef/* context */);
}// windowWillClose:


@end //} VectorWindow_Controller (VectorWindow_ControllerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
