/*!	\file EventLoop.mm
	\brief The main loop, and event-related utilities.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#import "EventLoop.h"
#import <UniversalDefines.h>

// standard-C++ includes
#import <algorithm>
#import <map>
#import <vector>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <QuickTime/QuickTime.h>

// library includes
#import <AlertMessages.h>
#import <AutoPool.objc++.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <Embedding.h>
#import <GrowlSupport.h>
#import <HIViewWrap.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlockHandleLocker.template.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <StringUtilities.h>
#import <WindowInfo.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "MenuBar.h"
#import "QuillsSession.h"
#import "Session.h"
#import "Terminology.h"



#pragma mark Types
namespace {

struct My_GlobalEventTarget
{
	ListenerModel_Ref	listenerModel;
};
typedef My_GlobalEventTarget*				My_GlobalEventTargetPtr;
typedef My_GlobalEventTarget const*			My_GlobalEventTargetConstPtr;
typedef My_GlobalEventTarget**				My_GlobalEventTargetHandle;
typedef struct OpaqueMy_GlobalEventTarget**	My_GlobalEventTargetRef;

typedef MemoryBlockHandleLocker< My_GlobalEventTargetRef, My_GlobalEventTarget >	My_GlobalEventTargetHandleLocker;
typedef LockAcquireRelease< My_GlobalEventTargetRef, My_GlobalEventTarget >			My_GlobalEventTargetAutoLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void					disposeGlobalEventTarget		(My_GlobalEventTargetRef*);
void					eventNotifyGlobal				(EventLoop_GlobalEvent, void*);
My_GlobalEventTargetRef	newGlobalEventTarget			();
OSStatus				receiveApplicationSwitch		(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveHICommand				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveSheetOpening				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowActivated			(EventHandlerCallRef, EventRef, void*);
OSStatus				updateModifiers					(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

NSAutoreleasePool*					gGlobalPool = nil;
My_GlobalEventTargetRef				gGlobalEventTarget = nullptr;
SInt16								gHaveInstalledNotification = 0;
UInt32								gTicksWaitNextEvent = 0L;
NMRec*								gBeepNotificationPtr = nullptr;
My_GlobalEventTargetHandleLocker&	gGlobalEventTargetHandleLocks ()	{ static My_GlobalEventTargetHandleLocker x; return x; }
CarbonEventHandlerWrap				gCarbonEventHICommandHandler(GetApplicationEventTarget(),
																	receiveHICommand,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassCommand),
																			kEventCommandProcess,
																			kEventCommandUpdateStatus),
																	nullptr/* user data */);
Console_Assertion					_1(gCarbonEventHICommandHandler.isInstalled(), __FILE__, __LINE__);
UInt32								gCarbonEventModifiers = 0L; // current modifier key states; updated by the callback
CarbonEventHandlerWrap				gCarbonEventModifiersHandler(GetApplicationEventTarget(),
																	updateModifiers,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassKeyboard),
																			kEventRawKeyModifiersChanged,
																			kEventRawKeyUp),
																	nullptr/* user data */);
Console_Assertion					_2(gCarbonEventModifiersHandler.isInstalled(), __FILE__, __LINE__);
CarbonEventHandlerWrap				gCarbonEventSwitchHandler(GetApplicationEventTarget(),
																receiveApplicationSwitch,
																CarbonEventSetInClass
																	(CarbonEventClass(kEventClassApplication),
																		kEventAppActivated, kEventAppDeactivated),
																nullptr/* user data */);
Console_Assertion					_3(gCarbonEventSwitchHandler.isInstalled(), __FILE__, __LINE__);
CarbonEventHandlerWrap				gCarbonEventWindowActivateHandler(GetApplicationEventTarget(),
																		receiveWindowActivated,
																		CarbonEventSetInClass
																			(CarbonEventClass(kEventClassWindow),
																				kEventWindowActivated),
																		nullptr/* user data */);
Console_Assertion					_4(gCarbonEventWindowActivateHandler.isInstalled(), __FILE__, __LINE__);
EventHandlerUPP						gCarbonEventSheetOpeningUPP = nullptr;
EventHandlerRef						gCarbonEventSheetOpeningHandler = nullptr;

} // anonymous namespace



#pragma mark Public Methods

/*!
Call this method at the start of the program,
before it is necessary to track events.  This
method creates the global mouse region, among
other things.

\retval noErr
always; no errors are currently defined

(3.0)
*/
EventLoop_Result
EventLoop_Init ()
{
	EventLoop_Result	result = noErr;
	
	
	// a pool must be constructed here, because NSApplicationMain()
	// is never called
	gGlobalPool = [[NSAutoreleasePool alloc] init];
	
	// ensure that Cocoa globals are defined (NSApp); this has to be
	// done separately, and not with NSApplicationMain(), because
	// some modules can only initialize after Cocoa is ready (and
	// not all modules are using Cocoa, yet)
	{
		AutoPool	_;
		NSBundle*	mainBundle = nil;
		NSString*	mainFileName = nil;
		BOOL		loadOK = NO;
		
		
		[NSApplication sharedApplication];
		assert(nil != NSApp);
		mainBundle = [NSBundle mainBundle];
		assert(nil != mainBundle);
		mainFileName = (NSString*)[mainBundle objectForInfoDictionaryKey:@"NSMainNibFile"]; // e.g. "MainMenuCocoa"
		assert(nil != mainFileName);
		loadOK = [NSBundle loadNibNamed:mainFileName owner:NSApp];
		assert(loadOK);
	}
	
	// support Growl notifications if possible
	GrowlSupport_Init();
	
	// set the sleep time (3.0 - don’t use preferences value, it’s not user-specifiable anymore)
	gTicksWaitNextEvent = 60; // make this larger to increase likelihood of high-frequency timers firing on time
	
	// install a callback that detects toolbar sheets
	{
		EventTypeSpec const		whenToolbarSheetOpens[] =
								{
									{ kEventClassWindow, kEventWindowSheetOpening }
								};
		OSStatus				error = noErr;
		
		
		gCarbonEventSheetOpeningUPP = NewEventHandlerUPP(receiveSheetOpening);
		error = InstallApplicationEventHandler(gCarbonEventSheetOpeningUPP,
												GetEventTypeCount(whenToolbarSheetOpens),
												whenToolbarSheetOpens, nullptr/* user data */,
												&gCarbonEventSheetOpeningHandler/* event handler reference */);
		if (noErr != error)
		{
			// it’s not critical if this handler is installed
			Console_Warning(Console_WriteValue, "failed to install event loop sheet-opening handler, error", error);
		}
	}
	
	// create listener models to handle event notifications
	gGlobalEventTarget = newGlobalEventTarget();
	
	return result;
}// Init


/*!
Call this method at the end of the program,
when other clean-up is being done and it is
no longer necessary to track events.

(3.0)
*/
void
EventLoop_Done ()
{
	RemoveEventHandler(gCarbonEventSheetOpeningHandler), gCarbonEventSheetOpeningHandler = nullptr;
	DisposeEventHandlerUPP(gCarbonEventSheetOpeningUPP), gCarbonEventSheetOpeningUPP = nullptr;
	
	// destroy global event target
	disposeGlobalEventTarget(&gGlobalEventTarget);
	
	//[gGlobalPool release];
}// Done


/*!
This method handles Color Picker 2.0+ event
requests in as simple a way as possible: by
responding to update events.  Time is also
given to other running threads.

(3.0)
*/
Boolean
EventLoop_HandleColorPickerUpdate	(EventRecord*		UNUSED_ARGUMENT(inoutEventPtr))
{
	Boolean		result = false; // event handled?
	
	
	// no longer important with Carbon Events
	return result;
}// HandleColorPickerUpdate


/*!
This method handles Navigation Services event
requests in as simple a way as possible: by
responding to update events.  Time is also
given to other running threads.

(3.0)
*/
void
EventLoop_HandleNavigationUpdate	(NavEventCallbackMessage	UNUSED_ARGUMENT(inMessage), 
									 NavCBRecPtr				UNUSED_ARGUMENT(inParameters), 
									 NavCallBackUserData		UNUSED_ARGUMENT(inUserData))
{
	// no longer important with Carbon Events
}// HandleNavigationUpdate


/*!
Performs whatever action is appropriate in response
to a user request to zoom the specified window.  The
window zooms based on whatever state it is currently
in (ideal state, user state).

(3.0)
*/
void
EventLoop_HandleZoomEvent	(HIWindowRef	inWindow)
{
	OSStatus	error = noErr;
	EventRef	zoomEvent = nullptr;
	
	
	error = CreateEvent(kCFAllocatorDefault, kEventClassWindow, kEventWindowZoom,
						GetCurrentEventTime(), kEventAttributeNone, &zoomEvent);
	assert_noerr(error);
	error = SetEventParameter(zoomEvent, kEventParamDirectObject, typeWindowRef, sizeof(inWindow), &inWindow);
	assert_noerr(error);
	SendEventToWindow(zoomEvent, inWindow);
}// HandleZoomEvent


/*!
Determines the state of the Caps Lock key when you
do not have access to an event record.  Returns
"true" only if the key is down.

(1.0)
*/
Boolean
EventLoop_IsCapsLockKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & alphaLock) != 0);
}// IsCapsLockKeyDown


/*!
Determines the state of the  key when you do not
have access to an event record.  Returns "true"
only if the key is down.

(1.0)
*/
Boolean
EventLoop_IsCommandKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & cmdKey) != 0);
}// IsCommandKeyDown


/*!
Determines the state of the Control key when you
do not have access to an event record.  Returns
"true" only if the key is down.

(4.0)
*/
Boolean
EventLoop_IsControlKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & controlKey) != 0);
}// IsControlKeyDown


/*!
This routine blocks until a mouse-down event occurs
or the “double time” has elapsed.  Returns "true"
only if it is a “double-click” event.

Only if "true" is returned, the resulting mouse
location in global coordinates is returned.

You normally invoke this immediately after you have
received a *mouse-up* event to see if the user is
actually intending to click twice.

Despite its name, this routine can detect triple-
clicks, etc. simply by being used repeatedly.  It
can also detect “one and a half clicks” because it
does not wait for a subsequent mouse-up.

DEPRECATED.  This may be necessary to detect certain
mouse behavior in raw event environments during
Carbon porting but it would generally NOT be this
difficult in a pure Cocoa event scheme.

(3.0)
*/
Boolean
EventLoop_IsNextDoubleClick		(HIWindowRef	inWindow,
								 Point&			outGlobalMouseLocation)
{
	Boolean		result = false;
	NSEvent*	nextClick = [NSApp nextEventMatchingMask:NSLeftMouseDownMask
															untilDate:[NSDate dateWithTimeIntervalSinceNow:[NSEvent doubleClickInterval]]
															inMode:NSEventTrackingRunLoopMode dequeue:YES];
	
	
	// NOTE: this used to be accomplished with Carbon event-handling
	// code but as of Mac OS X 10.9 the event loop seems unable to
	// reliably return the next click event; thus, Cocoa is used
	// (and this is a horrible coordinate-translation hack; it
	// reproduces the required behavior for now but it also just
	// underscores how critical it is that everything finally be
	// moved natively to Cocoa)
	if (nil != nextClick)
	{
		NSWindow*	eventWindow = [nextClick window];
		
		
		if (eventWindow == CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(inWindow))
		{
			NSPoint		clickLocation = [nextClick locationInWindow];
			
			
			clickLocation = [eventWindow convertBaseToScreen:clickLocation];
			clickLocation.y = [[eventWindow screen] frame].size.height;
			outGlobalMouseLocation.h = STATIC_CAST(clickLocation.x, SInt16);
			outGlobalMouseLocation.v = STATIC_CAST(clickLocation.y, SInt16);
			clickLocation = [eventWindow localToGlobalRelativeToTopForPoint:[nextClick locationInWindow]];
			outGlobalMouseLocation.h = STATIC_CAST(clickLocation.x, SInt16);
			outGlobalMouseLocation.v = STATIC_CAST(clickLocation.y, SInt16);
			result = ([nextClick clickCount] > 1);
		}
	}
	
	return result;
}// IsNextDoubleClick


/*!
Determines the state of the Option key when you
do not have access to an event record.  Returns
"true" only if the key is down.

(3.0)
*/
Boolean
EventLoop_IsOptionKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & optionKey) != 0);
}// IsOptionKeyDown


/*!
Determines the state of the Shift key when
you do not have access to an event record.
Returns "true" only if the key is down.

(3.0)
*/
Boolean
EventLoop_IsShiftKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & shiftKey) != 0);
}// IsShiftKeyDown


/*!
Determines if the specified "kEventTextInputUnicodeForKeyEvent" event
of class "kEventClassTextInput" is an Escape key or command-period key
press, or if it would otherwise activate the Cancel button of a window
when the button does not have direct keyboard focus.

Returns false for any other kind of event.

(4.0)
*/
Boolean
EventLoop_KeyIsActivatingCancelButton	(EventRef	inEvent)
{
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	Boolean			result = false;
	
	
	// NOTE: the IsUserCancelEventRef() API was tried, but it does not
	// seem to have the right effect for the kind of event handled below
	if ((kEventClassTextInput == kEventClass) &&
		(kEventTextInputUnicodeForKeyEvent == kEventKind))
	{
		EventRef	originalKeyPressEvent = nullptr;
		OSStatus	error = noErr;
		
		
		error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent,
														typeEventRef, originalKeyPressEvent);
		if (noErr == error)
		{
			UInt32		modifiers = 0;
			UInt32		keyCode = '\0';
			
			
			UNUSED_RETURN(OSStatus)CarbonEventUtilities_GetEventParameter(originalKeyPressEvent, kEventParamKeyModifiers,
																			typeUInt32, modifiers);
			error = CarbonEventUtilities_GetEventParameter
					(originalKeyPressEvent, kEventParamKeyCode, typeUInt32, keyCode);
			if (noErr == error)
			{
				// WARNING: technically, while the Escape key code is universal, the
				// key code for “period“ is dependent on the keyboard layout; TEMPORARY
				result = (((kVK_Escape == keyCode) && (0 == modifiers)) ||
							((kVK_ANSI_Period == keyCode) && (cmdKey == modifiers)));
			}
		}
	}
	return result;
}// KeyIsActivatingCancelButton


/*!
Determines if the specified "kEventTextInputUnicodeForKeyEvent" event
of class "kEventClassTextInput" is a Return key or Enter key press,
or if it would otherwise activate the default button of a window when
the button does not have direct keyboard focus.

Returns false for any other kind of event.

(4.0)
*/
Boolean
EventLoop_KeyIsActivatingDefaultButton	(EventRef	inEvent)
{
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	Boolean			result = false;
	
	
	if ((kEventClassTextInput == kEventClass) &&
		(kEventTextInputUnicodeForKeyEvent == kEventKind))
	{
		EventRef	originalKeyPressEvent = nullptr;
		OSStatus	error = noErr;
		
		
		error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent,
														typeEventRef, originalKeyPressEvent);
		if (noErr == error)
		{
			UInt32		modifiers = 0;
			UInt32		keyCode = '\0';
			
			
			UNUSED_RETURN(OSStatus)CarbonEventUtilities_GetEventParameter(originalKeyPressEvent, kEventParamKeyModifiers,
																			typeUInt32, modifiers);
			error = CarbonEventUtilities_GetEventParameter
					(originalKeyPressEvent, kEventParamKeyCode, typeUInt32, keyCode);
			if ((noErr == error) && (0 == modifiers))
			{
				// WARNING: technically, while the Return key code is universal, the
				// key code for Enter is dependent on the keyboard layout; TEMPORARY
				result = ((kVK_Return == keyCode) || (kVK_ANSI_KeypadEnter == keyCode));
			}
		}
	}
	return result;
}// KeyIsActivatingDefaultButton


/*!
Determines the absolutely current state of the
modifier keys and the mouse button.  The
modifiers are somewhat incomplete (for example,
no checking is done for the “right shift key”,
etc.), but all the common modifiers (command,
control, shift, option, caps lock, and the
mouse button) are returned.

(3.0)
*/
EventModifiers
EventLoop_ReturnCurrentModifiers ()
{
	// under Carbon, a callback updates a global variable whenever
	// the state of a modifier key changes; so, just return that value
	return gCarbonEventModifiers;
}// ReturnCurrentModifiers


/*!
Use instead of FrontWindow() to return the active
non-floating window.

(3.0)
*/
HIWindowRef
EventLoop_ReturnRealFrontWindow ()
{
	return ActiveNonFloatingWindow();
}// ReturnRealFrontWindow


/*!
Runs the main event loop, AND DOES NOT RETURN.  This routine
can ONLY be invoked from the main application thread.

To effectively run code “after” this point, modify the
application delegate’s "terminate:" method.

(3.0)
*/
void
EventLoop_Run ()
{
	AutoPool	_;
	
	
	[NSApp run];
}// Run


/*!
Use instead of SelectWindow() to bring a window
in front, unless the frontmost window is a modal
dialog box.

(3.0)
*/
void
EventLoop_SelectBehindDialogWindows		(HIWindowRef	inWindow)
{
	SelectWindow(inWindow);
	CocoaBasic_MakeFrontWindowCarbonUserFocusWindow();
}// SelectBehindDialogWindows


/*!
Use instead of SelectWindow() to bring a window
in front of all windows, even modal or floating
windows.  Note that this is only appropriate for
a window that you will be turning into a modal
window.

(3.0)
*/
void
EventLoop_SelectOverRealFrontWindow		(HIWindowRef	inWindow)
{
	SelectWindow(inWindow);
	CocoaBasic_MakeFrontWindowCarbonUserFocusWindow();
}// SelectOverRealFrontWindow


/*!
Manually flags that a window has been made Full Screen.  This is
necessary short-term to deal with several possible differences
in the way a window might become Full Screen (e.g. it could use
the original scheme or the new scheme introduced with Lion, and
it could be a Carbon or Cocoa window).

The context parameter must be consistent with the documentation
in the header file for each event.

(4.1)
*/
void
EventLoop_SendFullScreenWindowEvent		(EventLoop_GlobalEvent		inEvent,
										 void*						inEventContext)
{
	eventNotifyGlobal(inEvent, inEventContext);
}// SendFullScreenWindowEvent


/*!
Arranges for a callback to be invoked whenever a global
event occurs (such as an application switch).

Use EventLoop_StopMonitoring() to remove the listener in
the future.  You MUST do this prior to disposing of your
"ListenerModel_ListenerRef", otherwise the application will
crash when it tries to invoke the disposed listener.

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to an event.  See "EventLoop.h" for comments
			on what the context means for each type of
			global event.

(3.0)
*/
EventLoop_Result
EventLoop_StartMonitoring	(EventLoop_GlobalEvent		inForWhatEvent,
							 ListenerModel_ListenerRef	inListener)
{
	EventLoop_Result				result = noErr;
	My_GlobalEventTargetAutoLocker	ptr(gGlobalEventTargetHandleLocks(), gGlobalEventTarget);
	
	
	// add a listener to the specified target’s listener model for the given event
	result = ListenerModel_AddListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
	if (result == paramErr) result = kEventLoop_ResultBooleanListenerRequired;
	
	return result;
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked whenever a
global event occurs (such as an application switch).

\retval noErr
always; no errors are currently defined

(3.0)
*/
EventLoop_Result
EventLoop_StopMonitoring	(EventLoop_GlobalEvent		inForWhatEvent,
							 ListenerModel_ListenerRef	inListener)
{
	EventLoop_Result				result = noErr;
	My_GlobalEventTargetAutoLocker	ptr(gGlobalEventTargetHandleLocks(), gGlobalEventTarget);
	
	
	// remove a listener from the specified target’s listener model for the given event
	ListenerModel_RemoveListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
	
	return result;
}// StopMonitoring


#pragma mark Internal Methods
namespace {

/*!
Destroys an event target reference created with the
newGlobalEventTarget() routine.

(3.0)
*/
void
disposeGlobalEventTarget	(My_GlobalEventTargetRef*	inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		{
			My_GlobalEventTargetAutoLocker	ptr(gGlobalEventTargetHandleLocks(), *inoutRefPtr);
			
			
			if (ptr != nullptr)
			{
				// dispose of data members
				ListenerModel_Dispose(&ptr->listenerModel);
			}
		}
		Memory_DisposeHandle(REINTERPRET_CAST(inoutRefPtr, Handle*));
	}
}// disposeGlobalEventTarget


/*!
Notifies all listeners for the specified global
event, in an appropriate order, passing the given
context to the listener.

IMPORTANT:	The context must make sense for the
			type of event; see "EventLoop.h" for
			the type of context associated with
			each event type.

(3.0)
*/
void
eventNotifyGlobal	(EventLoop_GlobalEvent	inEventThatOccurred,
					 void*					inContextPtr)
{
	My_GlobalEventTargetAutoLocker	ptr(gGlobalEventTargetHandleLocks(), gGlobalEventTarget);
	
	
	// invoke listener callback routines appropriately
	ListenerModel_NotifyListenersOfEvent(ptr->listenerModel, inEventThatOccurred, inContextPtr);
}// eventNotifyGlobal


/*!
Creates the global event target, used for all events
that do not have a more specific target.  Dispose of
this with disposeGlobalEventTarget().

(3.0)
*/
My_GlobalEventTargetRef
newGlobalEventTarget ()
{
	My_GlobalEventTargetRef		result = REINTERPRET_CAST(Memory_NewHandle(sizeof(My_GlobalEventTarget)),
															My_GlobalEventTargetRef);
	
	
	if (result != nullptr)
	{
		My_GlobalEventTargetAutoLocker	ptr(gGlobalEventTargetHandleLocks(), result);
		
		
		ptr->listenerModel = ListenerModel_New(kListenerModel_StyleLogicalOR,
												kConstantsRegistry_ListenerModelDescriptorEventTargetGlobal);
	}
	return result;
}// newGlobalEventTarget


/*!
Handles "kEventAppActivated" and "kEventAppDeactivated"
of "kEventClassApplication".

Invoked by Mac OS X whenever this application becomes
frontmost (resumed/activated) or another application
becomes frontmost when this application was formerly
frontmost (suspended/deactivated).

(3.0)
*/
OSStatus
receiveApplicationSwitch	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent),
					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassApplication);
	assert((kEventKind == kEventAppActivated) || (kEventKind == kEventAppDeactivated));
	{
		Boolean		resuming = (kEventKind == kEventAppActivated);
		
		
		// scrap conversion is not tied to application switching
		// under Carbon; instead, use the new “promise” mechanism
		// provided by the Carbon Scrap Manager
		
		// set this global flag, which is consulted in various other files
		FlagManager_Set(kFlagSuspended, !resuming);
		
		Alert_SetIsBackgrounded(!resuming); // automatically removes any posted notifications from alerts
		if ((resuming) && (gHaveInstalledNotification))
		{
			// this removes any flashing icons, etc. from the process menu, if a notification was posted
			NMRemove(gBeepNotificationPtr);
			Memory_DisposeHandle(&(gBeepNotificationPtr->nmIcon));
			gHaveInstalledNotification = false;
		}
		
		// 3.0 - service a pending alert
		if (resuming) Alert_ServiceNotification();
		
		// 3.0 - restore color look-up table for the benefit of applications
		// that don’t use the Palette Manager
		unless (resuming) RestoreDeviceClut(nullptr/* restore all devices */);
		
		// 3.0 - notify all listeners of switch events that a switch has occurred
		eventNotifyGlobal(kEventLoop_GlobalEventSuspendResume, nullptr/* context */);
		
		result = noErr; // event is completely handled
	}
	return result;
}// receiveApplicationSwitch


/*!
Handles "kEventCommandProcess" of "kEventClassCommand".

(3.0)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// Execute a command selected from a menu.
				//
				// IMPORTANT: This could imply ANY form of menu selection, whether from
				//            the menu bar, from a contextual menu, or from a pop-up menu!
				{
					if (received.commandID != 0)
					{
						Boolean		commandOK = false;
						
						
						// execute the command
						commandOK = Commands_ExecuteByID(received.commandID);
						if (commandOK)
						{
							result = noErr;
						}
						else
						{
							result = eventNotHandledErr;
						}
					}
					else
					{
						// items without command IDs must be parsed individually
						if (MenuBar_HandleMenuCommand(received.menu.menuRef, received.menu.menuItemIndex))
						{
							result = noErr;
						}
						else
						{
							result = eventNotHandledErr;
						}
					}
				}
				break;
			
			default:
				// ???
				result = eventNotHandledErr;
				break;
			}
		}
	}
	return result;
}// receiveHICommand


/*!
Handles "kEventWindowSheetOpening" of "kEventClassWindow".

Invoked by Mac OS X whenever a sheet is about to open.
This handler could prevent the opening by returning
"userCanceledErr".

(3.1)
*/
OSStatus
receiveSheetOpening		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowSheetOpening);
	
	{
		HIWindowRef		sheetWindow = nullptr;
		
		
		// determine the window that is opening
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, sheetWindow);
		
		// if the window was found, proceed
		if (noErr == result)
		{
			CFStringRef		sheetTitleCFString = nullptr;
			
			
			// use special knowledge of how the Customize Toolbar sheet is set up
			// (/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/
			//	HIToolbox.framework/Versions/A/Resources/English.lproj/Toolbar.nib)
			// to hack in a more Cocoa-like look-and-feel; this is obviously
			// completely cosmetic, so any errors are ignored, etc.
			if (noErr == CopyWindowTitleAsCFString(sheetWindow, &sheetTitleCFString))
			{
				// attempt to identify the customization sheet using its title
				if (kCFCompareEqualTo == CFStringCompare(sheetTitleCFString, CFSTR("Configure Toolbar")/* from NIB */,
															0/* options */))
				{
					HIViewRef	contentView = nullptr;
					
					
					if (noErr == HIViewFindByID(HIViewGetRoot(sheetWindow), kHIViewWindowContentID,
												&contentView))
					{
						//HIViewID const		kPrompt2ID = { 'tcfg', 7 }; // from NIB
						HIViewID const		kShowLabelID = { 'tcfg', 5 }; // from NIB
						HIViewRef			possibleTextView = HIViewGetFirstSubview(contentView);
						
						
						// attempt to find the static text views that are not bold
						while (nullptr != possibleTextView)
						{
							ControlKind		kindInfo;
							HIViewID		viewID;
							
							
							// ensure this is some kind of text view
							if ((noErr == GetControlKind(possibleTextView, &kindInfo)) &&
								(kControlKindSignatureApple == kindInfo.signature) &&
								(kControlKindStaticText == kindInfo.kind))
							{
								// since the text was sized assuming regular font, it needs to
								// become slightly wider to accommodate bold font
								HIRect		textBounds;
								if (noErr == HIViewGetFrame(possibleTextView, &textBounds))
								{
									// the size problem only applies to the main prompt, not the others;
									// this prompt is arbitrarily identified by having a width bigger
									// than any of the other text areas (because it does not have a
									// proper ControlID like all the other customization sheet views)
									if (textBounds.size.width > 260/* arbitrary; uniquely identify the main prompt! */)
									{
										textBounds.size.width += 20; // arbitrary
										HIViewSetFrame(possibleTextView, &textBounds);
									}
								}
								
								// make the text bold!
								Localization_SetControlThemeFontInfo(possibleTextView, kThemeAlertHeaderFont);
								
								if ((noErr == GetControlID(possibleTextView, &viewID)) &&
									(kShowLabelID.signature == viewID.signature) &&
									(kShowLabelID.id == viewID.id))
								{
									// while there is a hack in place anyway, might as well
									// correct the annoying one pixel vertical misalignment
									// of the "Show" label for the pop-up menu in the corner
									HIViewMoveBy(possibleTextView, 0/* delta X */, 1.0/* delta Y */);
									
									// while there is a hack in place anyway, add a colon to
									// the label for consistency with every other dialog in
									// the OS!
									SetControlTextWithCFString(possibleTextView, CFSTR("Show:"));
									
									// resize the label a smidgen to make room for the colon
									if (noErr == HIViewGetFrame(possibleTextView, &textBounds))
									{
										textBounds.size.width += 2; // arbitrary
										HIViewSetFrame(possibleTextView, &textBounds);
									}
								}
							}
							possibleTextView = HIViewGetNextView(possibleTextView);
						}
					}
				}
				CFRelease(sheetTitleCFString), sheetTitleCFString = nullptr;
			}
		}
	}
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveSheetOpening


/*!
Embellishes "kEventWindowActivated" of "kEventClassWindow"
by resetting the mouse cursor.

(3.1)
*/
OSStatus
receiveWindowActivated	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowActivated);
	
	[[NSCursor arrowCursor] set];
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveWindowActivated


/*!
Invoked by Mac OS X whenever a modifier key’s state changes
(e.g. option, control, command, or shift).  This routine updates
an internal variable that is used by other functions (such as
EventLoop_IsCommandKeyDown()).

(3.0)
*/
OSStatus
updateModifiers		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = noErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassKeyboard);
	assert((kEventKind == kEventRawKeyModifiersChanged) || (kEventKind == kEventRawKeyUp));
	{
		// extract modifier key bits from the given event; it is important to
		// track both original key presses (kEventRawKeyModifiersChanged) and
		// key releases (kEventRawKeyUp) to know for sure, and both events
		// are documented as having a "kEventParamKeyModifiers" parameter
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, gCarbonEventModifiers);
		
		// if the modifier key information was found, proceed
		if (noErr == result)
		{
			if (FlagManager_Test(kFlagSuspended)) result = eventNotHandledErr;
		}
		else
		{
			// no modifier data available; assume no modifiers are in use!
			gCarbonEventModifiers = 0L;
		}
	}
	return result;
}// updateModifiers

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
