/*###############################################################

	EventLoop.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#include "UniversalDefines.h"

// standard-C++ includes
#include <algorithm>
#include <map>
#include <vector>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <QuickTime/QuickTime.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <Cursors.h>
#include <Embedding.h>
#include <HIViewWrap.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <WindowInfo.h>

// resource includes
#include "DialogResources.h"
#include "GeneralResources.h"
#include "MenuResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "Commands.h"
#include "ConnectionData.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventInfoControlScope.h"
#include "EventInfoWindowScope.h"
#include "EventLoop.h"
#include "HelpSystem.h"
#include "MenuBar.h"
#include "Preferences.h"
#include "QuillsSession.h"
#include "RasterGraphicsScreen.h"
#include "TektronixRealGraphics.h"
#include "TektronixVirtualGraphics.h"
#include "Terminology.h"



#pragma mark Constants

typedef UInt32 MouseRegionType;
enum
{
	kMouseRegionTypeNowhere = 0,
	kMouseRegionTypeTerminalTextSelection = 1,
	kMouseRegionTypeTerminalScreen = 2,
	kMouseRegionTypeGraphicsScreen = 3
};

/*!
Information stored in the control event info collection.
*/
typedef SInt32 MyControlEventInfoType;
enum
{
	kMyControlEventInfoTypeEventTargetRef		= FOUR_CHAR_CODE('cetg')		// data: MyControlEventTargetRef
};

/*!
Information stored in the window event info collection.
*/
typedef SInt32 MyWindowEventInfoType;
enum
{
	kMyWindowEventInfoTypeEventTargetRef		= FOUR_CHAR_CODE('wetg')		// data: MyWindowEventTargetRef
};

#pragma mark Types

typedef std::map< EventLoop_KeyEquivalent, ControlRef >		EventLoopKeyEquivalentToControlMap;

struct MyControlEventTarget
{
	ControlRef			control;
	ListenerModel_Ref	listenerModel;
};
typedef MyControlEventTarget*					MyControlEventTargetPtr;
typedef MyControlEventTarget const*				MyControlEventTargetConstPtr;
typedef MyControlEventTarget**					MyControlEventTargetHandle;
typedef struct OpaqueMyControlEventTarget**		MyControlEventTargetRef;

typedef MemoryBlockHandleLocker< MyControlEventTargetRef, MyControlEventTarget >	MyControlEventTargetHandleLocker;

struct MyGlobalEventTarget
{
	ListenerModel_Ref	listenerModel;
};
typedef MyGlobalEventTarget*				MyGlobalEventTargetPtr;
typedef MyGlobalEventTarget const*			MyGlobalEventTargetConstPtr;
typedef MyGlobalEventTarget**				MyGlobalEventTargetHandle;
typedef struct OpaqueMyGlobalEventTarget**	MyGlobalEventTargetRef;

typedef MemoryBlockHandleLocker< MyGlobalEventTargetRef, MyGlobalEventTarget >		MyGlobalEventTargetHandleLocker;

struct MyWindowEventTarget
{
	WindowRef							window;
	ListenerModel_Ref					listenerModel;
	EventLoopKeyEquivalentToControlMap	keyEquivalentsToControls;
	ControlRef							defaultButton;
	ControlRef							cancelButton;
};
typedef MyWindowEventTarget*				MyWindowEventTargetPtr;
typedef MyWindowEventTarget const*			MyWindowEventTargetConstPtr;
typedef MyWindowEventTarget**				MyWindowEventTargetHandle;
typedef struct OpaqueMyWindowEventTarget**	MyWindowEventTargetRef;

typedef MemoryBlockPtrLocker< MyWindowEventTargetRef, MyWindowEventTarget >		MyWindowEventTargetPtrLocker;

#pragma mark Internal Method Prototypes

static EventLoop_KeyEquivalent	createKeyboardEquivalent		(SInt16, SInt16, Boolean);
static void						disposeControlEventTarget		(MyControlEventTargetRef*);
static void						disposeGlobalEventTarget		(MyGlobalEventTargetRef*);
static void						disposeWindowEventTarget		(MyWindowEventTargetRef*);
static void						eventNotifyGlobal				(EventLoop_GlobalEvent, void*);
static Boolean					eventNotifyForControl			(EventLoop_ControlEvent, ControlRef, void*);
static Boolean					eventNotifyForWindow			(EventLoop_WindowEvent, WindowRef, void*);
static Boolean					eventTargetExistsForControl		(ControlRef);
static Boolean					eventTargetExistsForWindow		(WindowRef);
static MyControlEventTargetRef	findControlEventTarget			(ControlRef);
static MyWindowEventTargetRef	findWindowEventTarget			(WindowRef);
static Boolean					handleActivate					(EventRecord*, Boolean);
static Boolean					handleKeyDown					(EventRecord*);
static Boolean					handleUpdate					(EventRecord*);
static Boolean					isAnyListenerForControlEvent	(ControlRef, EventLoop_ControlEvent);
static Boolean					isAnyListenerForWindowEvent		(WindowRef, EventLoop_WindowEvent);
static Boolean					isCancelButton					(ControlRef);
static Boolean					isDefaultButton					(ControlRef);
static ControlRef				matchesKeyboardEquivalent		(WindowRef, SInt16, SInt16, Boolean);
static ControlRef				matchesKeyboardEquivalent		(WindowRef, EventLoop_KeyEquivalent);
static MyControlEventTargetRef	newControlEventTarget			(ControlRef);
static MyGlobalEventTargetRef	newGlobalEventTarget			();
static MyWindowEventTargetRef	newStandardWindowEventTarget	(WindowRef);
static pascal OSStatus			receiveApplicationSwitch		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			receiveMouseWheelEvent			(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			receiveServicePerformEvent		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			receiveSheetOpening				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			updateModifiers					(EventHandlerCallRef, EventRef, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	MyGlobalEventTargetRef				gGlobalEventTarget = nullptr;
	Collection							gControlEventInfo = nullptr,
										gWindowEventInfo = nullptr;
	SInt16								gHaveInstalledNotification = 0;
	UInt32								gTicksWaitNextEvent = 0L;
	NMRec*								gBeepNotificationPtr = nullptr;
	MyControlEventTargetHandleLocker&	gControlEventTargetHandleLocks ()	{ static MyControlEventTargetHandleLocker x; return x; }
	MyGlobalEventTargetHandleLocker&	gGlobalEventTargetHandleLocks ()	{ static MyGlobalEventTargetHandleLocker x; return x; }
	MyWindowEventTargetPtrLocker&		gWindowEventTargetPtrLocks ()		{ static MyWindowEventTargetPtrLocker x; return x; }
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
																				kEventRawKeyModifiersChanged),
																		nullptr/* user data */);
	Console_Assertion					_2(gCarbonEventModifiersHandler.isInstalled(), __FILE__, __LINE__);
	CarbonEventHandlerWrap				gCarbonEventMouseWheelHandler(GetApplicationEventTarget(),
																		receiveMouseWheelEvent,
																		CarbonEventSetInClass
																			(CarbonEventClass(kEventClassMouse),
																				kEventMouseWheelMoved),
																		nullptr/* user data */);
#if 0
	// don’t check for errors, it’s not critical if this handler is installed
	Console_Assertion					_3(gCarbonEventMouseWheelHandler.isInstalled(), __FILE__, __LINE__);
#endif
	CarbonEventHandlerWrap				gCarbonEventServiceHandler(GetApplicationEventTarget(),
																	receiveServicePerformEvent,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassService),
																			kEventServicePerform),
																	nullptr/* user data */);
	Console_Assertion					_4(gCarbonEventServiceHandler.isInstalled(), __FILE__, __LINE__);
	CarbonEventHandlerWrap				gCarbonEventSwitchHandler(GetApplicationEventTarget(),
																	receiveApplicationSwitch,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassApplication),
																			kEventAppActivated, kEventAppDeactivated),
																	nullptr/* user data */);
	Console_Assertion					_5(gCarbonEventSwitchHandler.isInstalled(), __FILE__, __LINE__);
	EventHandlerUPP						gCarbonEventSheetOpeningUPP = nullptr;
	EventHandlerRef						gCarbonEventSheetOpeningHandler = nullptr;
}



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
EventLoop_ResultCode
EventLoop_Init ()
{
	EventLoop_ResultCode	result = noErr;
	OSStatus				error = noErr;
	
	
	// set the sleep time (3.0 - don’t use preferences value, it’s not user-specifiable anymore)
	gTicksWaitNextEvent = 60; // make this larger to increase likelihood of high-frequency timers firing on time
	
	// install a callback that detects toolbar sheets
	if (FlagManager_Test(kFlagOS10_4API))
	{
		EventTypeSpec const		whenToolbarSheetOpens[] =
								{
									{ kEventClassWindow, kEventWindowSheetOpening }
								};
		
		
		gCarbonEventSheetOpeningUPP = NewEventHandlerUPP(receiveSheetOpening);
		error = InstallApplicationEventHandler(gCarbonEventSheetOpeningUPP,
												GetEventTypeCount(whenToolbarSheetOpens),
												whenToolbarSheetOpens, nullptr/* user data */,
												&gCarbonEventSheetOpeningHandler/* event handler reference */);
		// don’t check for errors, it’s not critical if this handler is installed
	}
	
	// create listener models to handle event notifications
	gGlobalEventTarget = newGlobalEventTarget();
	gControlEventInfo = NewCollection();
	gWindowEventInfo = NewCollection();
	
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
	
	// destroy any control event targets that were allocated
	{
		MyControlEventTargetRef		eventTarget = nullptr;
		SInt32						itemCount = CountTaggedCollectionItems
												(gControlEventInfo, kMyControlEventInfoTypeEventTargetRef);
		SInt32						itemSizeActualSize = sizeof(eventTarget);
		register SInt32				i = 0;
		OSStatus					error = noErr;
		
		
		for (i = 1; i <= itemCount; ++i)
		{
			error = GetTaggedCollectionItem(gControlEventInfo, kMyControlEventInfoTypeEventTargetRef, i,
											&itemSizeActualSize, &eventTarget);
			if (error == noErr) disposeControlEventTarget(&eventTarget);
		}
	}
	DisposeCollection(gControlEventInfo), gControlEventInfo = nullptr;
	
	// destroy any window event targets that were allocated
	{
		MyWindowEventTargetRef	eventTarget = nullptr;
		SInt32					itemCount = CountTaggedCollectionItems
											(gWindowEventInfo, kMyWindowEventInfoTypeEventTargetRef);
		SInt32					itemSizeActualSize = sizeof(eventTarget);
		register SInt32			i = 0;
		OSStatus				error = noErr;
		
		
		for (i = 1; i <= itemCount; ++i)
		{
			error = GetTaggedCollectionItem(gWindowEventInfo, kMyWindowEventInfoTypeEventTargetRef, i,
											&itemSizeActualSize, &eventTarget);
			if (error == noErr) disposeWindowEventTarget(&eventTarget);
		}
	}
	DisposeCollection(gWindowEventInfo), gWindowEventInfo = nullptr;
}// Done


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
EventLoop_CurrentModifiers ()
{
	// under Carbon, a callback updates a global variable whenever
	// the state of a modifier key changes; so, just return that value
	return gCarbonEventModifiers;
}// CurrentModifiers


/*!
Use instead of FrontWindow() to return the frontmost
non-floating window that might be behind a frontmost
modal dialog box.

(3.0)
*/
WindowRef
EventLoop_GetRealFrontNonDialogWindow ()
{
	WindowRef	result = FrontNonFloatingWindow();
	
	
	return result;
}// GetRealFrontNonDialogWindow


/*!
Use instead of FrontWindow() to return the active
non-floating window.

(3.0)
*/
WindowRef
EventLoop_GetRealFrontWindow ()
{
	return ActiveNonFloatingWindow();
}// GetRealFrontWindow


/*!
This method handles Color Picker 2.0+ event
requests in as simple a way as possible: by
responding to update events.  Time is also
given to other running threads.

(3.0)
*/
pascal Boolean
EventLoop_HandleColorPickerUpdate	(EventRecord*		inoutEventPtr)
{
	Boolean		result = pascal_false; // event handled?
	
	
	switch (inoutEventPtr->what)
	{
	case updateEvt:
		result = EventLoop_HandleEvent(inoutEventPtr);
		break;
	
	default:
		break;
	}
	return result;
}// HandleColorPickerUpdate


/*!
Handles all window and modeless dialog box events,
and returns "true" if the specified event was handled.

Note that this routine is very careful in judging the
circumstances under which a value of true is returned.
The Mac OS ultimately assumes that an event that is
"handled" will not be needed by anything else, and
removes such an event from the queue.  The most likely
event for which "true" will get returned is for update
events, because this routine knows if it recognizes
the window requiring updating or not and knows that,
once a window is updated, the event can safely be
discarded.

(2.6)
*/
Boolean
EventLoop_HandleEvent		(EventRecord*		inoutEventPtr)
{
	Boolean		result = false;
	
	
	switch (inoutEventPtr->what)
	{
	case mouseDown:
		result = false;
		break;
	
	case updateEvt: // a window needs all or part of it redrawn
		result = handleUpdate(inoutEventPtr);
		break;
	
	case keyDown:
	case autoKey: // a key was pressed
		result = handleKeyDown(inoutEventPtr);
		break;
	
	case diskEvt: // disk inserted?
		// automatically handled under Carbon
		break;
	
	case activateEvt: // a window was moved in front of or behind another window
		result = handleActivate(inoutEventPtr, inoutEventPtr->modifiers & activeFlag);
		break;
	
	case osEvt: // possibly an application-switch event or mouse-moved event
		switch ((inoutEventPtr->message & osEvtMessageMask) >> 24)
		{
			case suspendResumeMessage:
				// this is now handled via Carbon Events (see kEventClassApplication)
				break;
			
			case mouseMovedMessage:
				// this should be handled on a control-by-control basis
				break;
			
			default:
				// unknown OS event
				break;
		}
		break;
	
	case kHighLevelEvent: // Apple Events - script commands, for example
		AEProcessAppleEvent(inoutEventPtr);
		result = true;
		break;
	
	default: // who knows WHAT the OS is asking
		break;
	}
	
	return result;
}// HandleEvent


/*!
This method handles Navigation Services event
requests in as simple a way as possible: by
responding to update events.  Time is also
given to other running threads.

(3.0)
*/
pascal void
EventLoop_HandleNavigationUpdate	(NavEventCallbackMessage	inMessage, 
									 NavCBRecPtr				inParameters, 
									 NavCallBackUserData		UNUSED_ARGUMENT(inUserData))
{
	if (inMessage == kNavCBEvent)
	{
		switch (inParameters->eventData.eventDataParms.event->what)
		{
		case updateEvt:
			EventLoop_HandleEvent(inParameters->eventData.eventDataParms.event);
			break;
		
		default:
			break;
		}
	}
}// HandleNavigationUpdate


/*!
Performs whatever action is appropriate in response
to a user request to zoom the specified window.  The
window zooms based on whatever state it is currently
in (ideal state, user state).

(3.0)
*/
void
EventLoop_HandleZoomEvent	(WindowRef		inWindow)
{
	OSStatus	error = noErr;
	EventRef	zoomEvent = nullptr;
	
	
	error = CreateEvent(kCFAllocatorDefault, kEventClassWindow, kEventWindowZoom,
						GetCurrentEventTime(), kEventAttributeNone, &zoomEvent);
	assert(error == noErr);
	error = SetEventParameter(zoomEvent, kEventParamDirectObject, typeWindowRef, sizeof(inWindow), &inWindow);
	assert(error == noErr);
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
This routine gets the next key event from the
event queue and returns true if it is a "cancel"
(command-period) event.  Use this to monitor
the keyboard for user cancellations while a
dialog box is displayed and/or the normal event
loop is not running.

(3.0)
*/
Boolean
EventLoop_IsNextCancel ()
{
	Boolean			result = false;
	EventRecord		event;
	
	
	while (WaitNextEvent(keyDownMask | keyUpMask, &event, gTicksWaitNextEvent, nullptr/* mouse-moved region */))
	{
		if ((event.modifiers & cmdKey) && (event.message & charCodeMask) == '.') result = true;
	}
	return result;
}// IsNextCancel


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

(3.0)
*/
Boolean
EventLoop_IsNextDoubleClick		(Point*		outGlobalMouseLocationPtr)
{
	UInt32 const	kDoubleTimeInTicks = GetDblTime();
	Boolean			result = false;
	
	
	{
		EventTypeSpec const		whenMouseButtonDown[] =
								{
									{ kEventClassMouse, kEventMouseDown }
								};
		EventRef				mouseDownEvent = nullptr;
		OSStatus				error = noErr;
		
		
		error = ReceiveNextEvent(GetEventTypeCount(whenMouseButtonDown), whenMouseButtonDown,
									TicksToEventTime(kDoubleTimeInTicks)/* timeout */, true/* pull event from queue */,
									&mouseDownEvent);
		if (error == noErr)
		{
			HIPoint		mouseDownLocation;
			
			
			// retrieve mouse location from event
			error = CarbonEventUtilities_GetEventParameter(mouseDownEvent, kEventParamMouseLocation, typeHIPoint,
															mouseDownLocation);
			SetPt(outGlobalMouseLocationPtr, STATIC_CAST(mouseDownLocation.x, SInt16),
					STATIC_CAST(mouseDownLocation.y, SInt16));
		}
		result = ((error == noErr) && (!IsShowContextualMenuEvent(mouseDownEvent)));
		ReleaseEvent(mouseDownEvent); // necessary because “pull event from queue” flag set in ReceiveNextEvent() call above
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
Arranges for the specified key to automatically
simulate a click in the given button.  The click
generates a real click event, so if you listen
for clicks in a button, you will also receive
events when a button is acted upon using the
keyboard and the key combination indicated here.

You should remove the keystroke from the registry
before destroying the indicated control, using
the routine EventLoop_UnregisterButtonClickKey().

Returns "true" only if the key was successfully
registered.

(3.0)
*/
Boolean
EventLoop_RegisterButtonClickKey	(ControlRef					inButton,
									 EventLoop_KeyEquivalent	inKeyEquivalent)
{
	Boolean					result = false;
	MyWindowEventTargetRef	ref = findWindowEventTarget(GetControlOwner(inButton));
	
	
	if (ref != nullptr)
	{
		MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
		
		
		// set a mapping between the given key equivalent and the given button
		try
		{
			ptr->keyEquivalentsToControls[inKeyEquivalent] = inButton;
			assert(ptr->keyEquivalentsToControls.find(inKeyEquivalent) != ptr->keyEquivalentsToControls.end());
			
			// check to see if the existing default or Cancel buttons for
			// the specified control’s window match the given control; if
			// so and the given key equivalent is not appropriate, then
			// the given button must no longer be “special”
			if ((ptr->defaultButton == inButton) && (inKeyEquivalent != kEventLoop_KeyEquivalentDefaultButton))
			{
				Boolean		defaultFlag = false;
				
				
				// re-assigning key equivalent; reset default button
				ptr->defaultButton = nullptr;
				(OSStatus)SetControlData(inButton, kControlButtonPart, kControlPushButtonDefaultTag,
											sizeof(defaultFlag), &defaultFlag);
			}
			if ((ptr->cancelButton == inButton) && (inKeyEquivalent != kEventLoop_KeyEquivalentCancelButton))
			{
				Boolean		cancelFlag = false;
				
				
				// re-assigning key equivalent; reset Cancel button
				ptr->cancelButton = nullptr;
				(OSStatus)SetControlData(inButton, kControlButtonPart, kControlPushButtonCancelTag,
											sizeof(cancelFlag), &cancelFlag);
			}
			
			// if the given button is being assigned a default or Cancel
			// key equivalent, then remember this (for efficiency whenever
			// it is necessary to find these special buttons later);
			// also, automatically add a default button ring, etc.
			if (inKeyEquivalent == kEventLoop_KeyEquivalentDefaultButton)
			{
				Boolean		defaultFlag = true;
				
				
				ptr->defaultButton = inButton;
				(OSStatus)SetControlData(inButton, kControlButtonPart, kControlPushButtonDefaultTag,
											sizeof(defaultFlag), &defaultFlag);
			}
			if (inKeyEquivalent == kEventLoop_KeyEquivalentCancelButton)
			{
				Boolean		cancelFlag = true;
				
				
				ptr->cancelButton = inButton;
				(OSStatus)SetControlData(inButton, kControlButtonPart, kControlPushButtonCancelTag,
											sizeof(cancelFlag), &cancelFlag);
			}
			result = true;
		}
		catch (std::bad_alloc)
		{
			// not enough memory?!?!?
		}
		gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
	}
	
	return result;
}// RegisterButtonClickKey


/*!
Runs the main event loop, NOT RETURNING until a
quit operation is invoked.  This routine can ONLY
be invoked from the main application thread.

(3.0)
*/
void
EventLoop_Run ()
{
	RunApplicationEventLoop();
}// Run


/*!
Use instead of SelectWindow() to bring a window
in front, unless the frontmost window is a modal
dialog box.

(3.0)
*/
void
EventLoop_SelectBehindDialogWindows		(WindowRef		inWindow)
{
	SelectWindow(inWindow);
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
EventLoop_SelectOverRealFrontWindow		(WindowRef		inWindow)
{
	// I give up, dammit; it’s too hard to prevent
	// activate events from occurring on Mac OS 8/9.
	SelectWindow(inWindow);
}// SelectOverRealFrontWindow


/*!
Arranges for a callback to be invoked whenever a global
event occurs (such as an application switch).

Use EventLoop_StopListeningForGlobalEvent() to remove
the listener in the future.  You MUST do this prior to
disposing of your "ListenerModel_ListenerRef", otherwise
the application will crash when it tries to invoke the
disposed listener.

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to an event.  See "EventLoop.h" for comments
			on what the context means for each type of
			global event.

(3.0)
*/
EventLoop_ResultCode
EventLoop_StartMonitoring	(EventLoop_GlobalEvent		inForWhatEvent,
							 ListenerModel_ListenerRef	inListener)
{
	EventLoop_ResultCode	result = noErr;
	MyGlobalEventTargetPtr	ptr = gGlobalEventTargetHandleLocks().acquireLock(gGlobalEventTarget);
	
	
	// add a listener to the specified target’s listener model for the given event
	result = ListenerModel_AddListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
	if (result == paramErr) result = kEventLoop_ResultCodeBooleanListenerRequired;
	gGlobalEventTargetHandleLocks().releaseLock(gGlobalEventTarget, &ptr);
	
	return result;
}// StartMonitoring


/*!
Arranges for a callback to be invoked whenever an event
occurs in a control (such as a click in a frontmost
window, or a click in a background window).  Pass nullptr
as the control to have your notifier invoked whenever
the specified event occurs for ANY control!

Use EventLoop_StopMonitoringControl() to remove the
listener in the future.  You MUST do this prior to
disposing of your "ListenerModel_ListenerRef", otherwise
the application will crash when it tries to invoke the
disposed listener.

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to an event.  See "EventLoop.h" for comments
			on what the context means for each type of
			control event.

\retval kEventLoop_ResultCodeSuccess
if no error occurred

\retval kEventLoop_ResultCodeBooleanListenerRequired
if "inListener" is not a Boolean listener; event
callbacks use the Boolean return value to state whether
an event has been COMPLETELY handled (and therefore
should be absorbed instead of being sent to additional
callbacks)

(3.0)
*/
EventLoop_ResultCode
EventLoop_StartMonitoringControl	(EventLoop_ControlEvent		inForWhatEvent,
									 ControlRef					inForWhichControlOrNullToReceiveEventsForAllControls,
									 ListenerModel_ListenerRef	inListener)
{
	EventLoop_ResultCode	result = noErr;
	
	
	if (inForWhichControlOrNullToReceiveEventsForAllControls == nullptr)
	{
		// monitor all controls
		result = EventLoop_StartMonitoring(inForWhatEvent, inListener);
	}
	else
	{
		// monitor specific control
		MyControlEventTargetRef		ref = findControlEventTarget(inForWhichControlOrNullToReceiveEventsForAllControls);
		
		
		if (ref != nullptr)
		{
			MyControlEventTargetPtr		ptr = gControlEventTargetHandleLocks().acquireLock(ref);
			
			
			// add a listener to the specified target’s listener model for the given event
			result = ListenerModel_AddListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
			if (result == paramErr) result = kEventLoop_ResultCodeBooleanListenerRequired;
			gControlEventTargetHandleLocks().releaseLock(ref, &ptr);
		}
	}
	return result;
}// StartMonitoringControl


/*!
Arranges for a callback to be invoked whenever an event
occurs in a window (such as an update, zoom, or close).
Pass nullptr as the window to have your notifier invoked
whenever the specified event occurs for ANY window!

Use EventLoop_StopMonitoringWindow() to remove the
listener in the future.  You MUST do this prior to
disposing of your "ListenerModel_ListenerRef", otherwise
the application will crash when it tries to invoke the
disposed listener.

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to an event.  See "EventLoop.h" for comments
			on what the context means for each type of
			window event.

\retval kEventLoop_ResultCodeSuccess
if no error occurred

\retval kEventLoop_ResultCodeBooleanListenerRequired
if "inListener" is not a Boolean listener; event
callbacks use the Boolean return value to state whether
an event has been COMPLETELY handled (and therefore
should be absorbed instead of being sent to additional
callbacks)

(3.0)
*/
EventLoop_ResultCode
EventLoop_StartMonitoringWindow		(EventLoop_WindowEvent		inForWhatEvent,
									 WindowRef					inForWhichWindowOrNullToReceiveEventsForAllWindows,
									 ListenerModel_ListenerRef	inListener)
{
	EventLoop_ResultCode	result = noErr;
	
	
	if (inForWhichWindowOrNullToReceiveEventsForAllWindows == nullptr)
	{
		// monitor all windows
		result = EventLoop_StartMonitoring(inForWhatEvent, inListener);
	}
	else
	{
		// monitor specific window
		MyWindowEventTargetRef	ref = findWindowEventTarget(inForWhichWindowOrNullToReceiveEventsForAllWindows);
		
		
		if (ref != nullptr)
		{
			MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
			
			
			// add a listener to the specified target’s listener model for the given event
			result = ListenerModel_AddListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
			if (result == paramErr) result = kEventLoop_ResultCodeBooleanListenerRequired;
			gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
		}
	}
	return result;
}// StartMonitoringWindow


/*!
Arranges for a callback to no longer be invoked whenever a
global event occurs (such as an application switch).

\retval noErr
always; no errors are currently defined

(3.0)
*/
EventLoop_ResultCode
EventLoop_StopMonitoring	(EventLoop_GlobalEvent		inForWhatEvent,
							 ListenerModel_ListenerRef	inListener)
{
	EventLoop_ResultCode	result = noErr;
	MyGlobalEventTargetPtr	ptr = gGlobalEventTargetHandleLocks().acquireLock(gGlobalEventTarget);
	
	
	// remove a listener from the specified target’s listener model for the given event
	ListenerModel_RemoveListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
	gGlobalEventTargetHandleLocks().releaseLock(gGlobalEventTarget, &ptr);
	
	return result;
}// StopMonitoring


/*!
Arranges for a callback to no longer be invoked whenever
an event occurs in a control (such as a mouse click).
Pass nullptr as the control to keep your notifier from
receiving the specified event for ANY control!

\retval noErr
always; no errors are currently defined

(3.0)
*/
EventLoop_ResultCode
EventLoop_StopMonitoringControl		(EventLoop_ControlEvent		inForWhatEvent,
									 ControlRef					inForWhichControlOrNullToStopReceivingEventsForAllControls,
									 ListenerModel_ListenerRef	inListener)
{
	EventLoop_ResultCode	result = noErr;
	
	
	if (inForWhichControlOrNullToStopReceivingEventsForAllControls == nullptr)
	{
		// ignore all controls
		result = EventLoop_StopMonitoring(inForWhatEvent, inListener);
	}
	else
	{
		// ignore specific control
		MyControlEventTargetRef		ref = findControlEventTarget(inForWhichControlOrNullToStopReceivingEventsForAllControls);
		
		
		if (ref != nullptr)
		{
			MyControlEventTargetPtr		ptr = gControlEventTargetHandleLocks().acquireLock(ref);
			
			
			// remove a listener from the specified target’s listener model for the given event
			ListenerModel_RemoveListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
			gControlEventTargetHandleLocks().releaseLock(ref, &ptr);
		}
	}
	return result;
}// StopMonitoringControl


/*!
Arranges for a callback to no longer be invoked whenever
an event occurs in a window (such as an update, zoom, or
close).  Pass nullptr as the window to keep your notifier
from receiving the specified event for ANY window!

\retval noErr
always; no errors are currently defined

(3.0)
*/
EventLoop_ResultCode
EventLoop_StopMonitoringWindow	(EventLoop_WindowEvent		inForWhatEvent,
								 WindowRef					inForWhichWindowOrNullToStopReceivingEventsForAllWindows,
								 ListenerModel_ListenerRef	inListener)
{
	EventLoop_ResultCode	result = noErr;
	
	
	if (inForWhichWindowOrNullToStopReceivingEventsForAllWindows == nullptr)
	{
		// ignore all windows
		result = EventLoop_StopMonitoring(inForWhatEvent, inListener);
	}
	else
	{
		// ignore specific window
		MyWindowEventTargetRef	ref = findWindowEventTarget(inForWhichWindowOrNullToStopReceivingEventsForAllWindows);
		
		
		if (ref != nullptr)
		{
			MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
			
			
			// remove a listener from the specified target’s listener model for the given event
			ListenerModel_RemoveListenerForEvent(ptr->listenerModel, inForWhatEvent, inListener);
			gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
		}
	}
	return result;
}// StopMonitoringWindow


/*!
Kills the main event loop, causing EventLoop_Run()
to return.  This should be done from within the
Quit Apple Event handler, and not usually any place
else.

(3.0)
*/
void
EventLoop_Terminate ()
{
	QuitApplicationEventLoop();
}// Terminate


/*!
Removes any key equivalents registered for the
specified button, so that subsequent uses of
the keystroke will not trigger clicks in the
button.

Returns "true" only if the key was successfully
unregistered.

(3.0)
*/
Boolean
EventLoop_UnregisterButtonClickKey	(ControlRef					inButton,
									 EventLoop_KeyEquivalent	inKeyEquivalent)
{
	Boolean					result = false;
	MyWindowEventTargetRef	ref = findWindowEventTarget(GetControlOwner(inButton));
	
	
	if (ref != nullptr)
	{
		MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
		
		
		// remove the mapping between the given key equivalent and the given button
		if (ptr->keyEquivalentsToControls.find(inKeyEquivalent) != ptr->keyEquivalentsToControls.end())
		{
			ptr->keyEquivalentsToControls.erase(ptr->keyEquivalentsToControls.find(inKeyEquivalent));
			result = (ptr->keyEquivalentsToControls.find(inKeyEquivalent) == ptr->keyEquivalentsToControls.end());
		}
		gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
	}
	
	return result;
}// UnregisterButtonClickKey


#pragma mark Internal Methods

/*!
Converts the specified event-based keystroke
information into a single constant that
describes the entire keystroke.

See EventLoop.h for details on what a
"EventLoop_KeyboardEquivalent" can be.

(3.0)
*/
static EventLoop_KeyEquivalent
createKeyboardEquivalent	(SInt16		inCharacterCode,
							 SInt16		inVirtualKeyCode,
							 Boolean	inCommandDown)
{
	EventLoop_KeyEquivalent		 result = '----';
	
	
	if (inCommandDown)
	{
		// check for command-<character> equivalents
		if (inCharacterCode == '.') result = kEventLoop_KeyEquivalentCancelButton;
		else if (inCharacterCode != 0) result = kEventLoop_KeyEquivalentArbitraryCommandLetter | inCharacterCode;
		else result = kEventLoop_KeyEquivalentArbitraryCommandVirtualKey | inVirtualKeyCode;
	}
	else
	{
		// consider one-key equivalents
		switch (inCharacterCode)
		{
		case 0x0D: // Return
		case 0x03: // Enter
			result = kEventLoop_KeyEquivalentDefaultButton;
			break;
		
		default:
			// ???
			switch (inVirtualKeyCode)
			{
				case 0x35: // Escape
					result = kEventLoop_KeyEquivalentCancelButton;
				
				default:
					// ???
					break;
			}
			break;
		}
	#if 0
		case kEventLoop_KeyEquivalentFirstLetter:
			if (inCommandDown)
			{
				OSStatus		error = noErr;
				CFStringRef		titleString = nullptr;
				
				
				error = CopyControlTitleAsCFString(inControl, &titleString);
				if (error == noErr)
				{
					UniChar		firstLetter = 0;
					
					
					// command + first character of button title, case-insensitive
					firstLetter = CFStringGetCharacterAtIndex(titleString, 0/* index */);
					result = (firstLetter == inCharacterCode);
					unless (result)
					{
						CFMutableStringRef		mutableString = CFStringCreateMutable(kCFAllocatorDefault,
																						1/* length */);
						
						
						if (mutableString != nullptr)
						{
							CFStringAppendCharacters(mutableString, &firstLetter, 1/* number of characters */);
							CFStringLowercase(mutableString, nullptr/* locale data */);
							firstLetter = CFStringGetCharacterAtIndex(mutableString, 0/* index */);
							result = (firstLetter == inCharacterCode);
							CFRelease(mutableString);
						}
					}
					CFRelease(titleString);
				}
			}
	#endif
	}
	return result;
}// createKeyboardEquivalent


/*!
Destroys an event target reference created with the
newControlEventTarget() routine.

(3.0)
*/
static void
disposeControlEventTarget	(MyControlEventTargetRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		MyControlEventTargetPtr		ptr = gControlEventTargetHandleLocks().acquireLock(*inoutRefPtr);
		
		
		if (ptr != nullptr)
		{
			// dispose of data members
			ListenerModel_Dispose(&ptr->listenerModel);
		}
		gControlEventTargetHandleLocks().releaseLock(*inoutRefPtr, &ptr);
		Memory_DisposeHandle(REINTERPRET_CAST(inoutRefPtr, Handle*));
	}
}// disposeControlEventTarget


/*!
Destroys an event target reference created with the
newGlobalEventTarget() routine.

(3.0)
*/
static void
disposeGlobalEventTarget	(MyGlobalEventTargetRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		MyGlobalEventTargetPtr	ptr = gGlobalEventTargetHandleLocks().acquireLock(*inoutRefPtr);
		
		
		if (ptr != nullptr)
		{
			// dispose of data members
			ListenerModel_Dispose(&ptr->listenerModel);
		}
		gGlobalEventTargetHandleLocks().releaseLock(*inoutRefPtr, &ptr);
		Memory_DisposeHandle(REINTERPRET_CAST(inoutRefPtr, Handle*));
	}
}// disposeGlobalEventTarget


/*!
Destroys an event target reference created with the
newWindowEventTarget() routine.

(3.0)
*/
static void
disposeWindowEventTarget	(MyWindowEventTargetRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(*inoutRefPtr);
		
		
		if (ptr != nullptr)
		{
			// dispose of data members
			ListenerModel_Dispose(&ptr->listenerModel);
		}
		gWindowEventTargetPtrLocks().releaseLock(*inoutRefPtr, &ptr);
		delete *(REINTERPRET_CAST(inoutRefPtr, MyWindowEventTargetPtr*)), *inoutRefPtr = nullptr;
	}
}// disposeWindowEventTarget


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
static void
eventNotifyGlobal	(EventLoop_GlobalEvent	inEventThatOccurred,
					 void*					inContextPtr)
{
	MyGlobalEventTargetPtr	ptr = gGlobalEventTargetHandleLocks().acquireLock(gGlobalEventTarget);
	
	
	// invoke listener callback routines appropriately
	ListenerModel_NotifyListenersOfEvent(ptr->listenerModel, inEventThatOccurred, inContextPtr);
	gGlobalEventTargetHandleLocks().releaseLock(gGlobalEventTarget, &ptr);
}// eventNotifyGlobal


/*!
Notifies all listeners for the specified control
event, in an appropriate order, passing the given
context to the listener.  For some events, the
context may be modified - in those cases, check
the context after invoking this routine to see if
anything has changed.

Returns "true" only if some handler completely
handled the event.

IMPORTANT:	The context must make sense for the
			type of event; see "EventLoop.h" for
			the type of context associated with
			each control event type.

(3.0)
*/
static Boolean
eventNotifyForControl	(EventLoop_ControlEvent		inEventThatOccurred,
						 ControlRef					inForWhichControl,
						 void*						inoutContextPtr)
{
	MyControlEventTargetRef		ref = findControlEventTarget(inForWhichControl);
	Boolean						result = false;
	
	
	if (ref != nullptr)
	{
		MyControlEventTargetPtr		ptr = gControlEventTargetHandleLocks().acquireLock(ref);
		
		
		// invoke listener callback routines appropriately, from the specified control’s event target
		ListenerModel_NotifyListenersOfEvent(ptr->listenerModel, inEventThatOccurred, inoutContextPtr,
												REINTERPRET_CAST(&result, Boolean*));
		gControlEventTargetHandleLocks().releaseLock(ref, &ptr);
	}
	
	// also notify callbacks that don’t care which control it is
	unless (result) eventNotifyGlobal(inEventThatOccurred, inoutContextPtr);
	
	return result;
}// eventNotifyForControl


/*!
Notifies all listeners for the specified window
event, in an appropriate order, passing the given
context to the listener.  For some events, the
context may be modified - in those cases, check
the context after invoking this routine to see if
anything has changed.

IMPORTANT:	The context must make sense for the
			type of event; see "EventLoop.h" for
			the type of context associated with
			each window event type.

(3.0)
*/
static Boolean
eventNotifyForWindow	(EventLoop_WindowEvent	inEventThatOccurred,
						 WindowRef				inForWhichWindow,
						 void*					inoutContextPtr)
{
	MyWindowEventTargetRef	ref = findWindowEventTarget(inForWhichWindow);
	Boolean					result = false;
	
	
	if (ref != nullptr)
	{
		MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
		
		
		// invoke listener callback routines appropriately, from the specified window’s event target
		ListenerModel_NotifyListenersOfEvent(ptr->listenerModel, inEventThatOccurred, inoutContextPtr,
												REINTERPRET_CAST(&result, Boolean*));
		gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
	}
	
	// also notify callbacks that don’t care which window it is
	unless (result) eventNotifyGlobal(inEventThatOccurred, inoutContextPtr);
	
	return result;
}// eventNotifyForWindow


/*!
Determines if the specified control has had an
event target created for it.  Event targets store
the list of event listeners, and other information,
for particular controls in windows.

You can use this when events occur to determine if
there is any point in trying to notify listeners.
Most controls will not have had listeners installed
for them, and automatically allocating an event
target for each control is wasteful.  In addition,
you might need to perform some other course of
action in the event that no target is available
(for example, iterate over a known list of controls
to figure out what to do in response to an event).

(3.0)
*/
static Boolean
eventTargetExistsForControl		(ControlRef		inForWhichControl)
{
	Boolean		result = false;
	SInt32		uniqueID = REINTERPRET_CAST(inForWhichControl, SInt32);
	
	
	if (inForWhichControl != nullptr)
	{
		OSStatus		error = noErr;
		
		
		error = GetCollectionItemInfo(gControlEventInfo, kMyControlEventInfoTypeEventTargetRef, uniqueID,
										REINTERPRET_CAST(kCollectionDontWantIndex, SInt32*),
										REINTERPRET_CAST(kCollectionDontWantSize, SInt32*),
										REINTERPRET_CAST(kCollectionDontWantAttributes, SInt32*));
		if (error == noErr) result = true;
	}
	return result;
}// eventTargetExistsForControl


/*!
Determines if the specified window has had an event
target created for it.  Event targets store the list
of event listeners, and other information, for
particular windows.

You can use this when events occur to determine if
there is any point in trying to notify listeners.
Not all windows will have had listeners installed
for them, so automatically allocating an event
target for each window is wasteful.  In addition,
you might need to perform some other course of
action in the event that no target is available
(for example, iterate over a known list of windows
to figure out what to do in response to an event).

(3.0)
*/
static Boolean
eventTargetExistsForWindow		(WindowRef		inForWhichWindow)
{
	Boolean		result = false;
	SInt32		uniqueID = REINTERPRET_CAST(inForWhichWindow, SInt32);
	
	
	if (inForWhichWindow != nullptr)
	{
		OSStatus	error = noErr;
		
		
		error = GetCollectionItemInfo(gWindowEventInfo, kMyWindowEventInfoTypeEventTargetRef, uniqueID,
										REINTERPRET_CAST(kCollectionDontWantIndex, SInt32*),
										REINTERPRET_CAST(kCollectionDontWantSize, SInt32*),
										REINTERPRET_CAST(kCollectionDontWantAttributes, SInt32*));
		if (error == noErr) result = true;
	}
	return result;
}// eventTargetExistsForWindow


/*!
Creates or returns the existing event target for
the specified control.  Event targets store the
list of event listeners, and other information,
for particular controls in windows.

(3.0)
*/
static MyControlEventTargetRef
findControlEventTarget		(ControlRef		inForWhichControl)
{
	MyControlEventTargetRef		result = nullptr;
	SInt32						uniqueID = REINTERPRET_CAST(inForWhichControl, SInt32);
	OSStatus					error = noErr;
		
		
	if (GetCollectionItemInfo(gControlEventInfo, kMyControlEventInfoTypeEventTargetRef, uniqueID,
								REINTERPRET_CAST(kCollectionDontWantIndex, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantSize, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantAttributes, SInt32*)) ==
		collectionItemNotFoundErr)
	{
		// no target exists for this control - create one
		result = newControlEventTarget(inForWhichControl);
		error = AddCollectionItem(gControlEventInfo, kMyControlEventInfoTypeEventTargetRef, uniqueID,
									sizeof(result), &result);
		if (error != noErr)
		{
			// error!
			Console_WriteValue("control event target add-to-collection error", error);
			result = nullptr;
		}
	}
	else
	{
		SInt32		itemSizeActualSize = sizeof(result);
		
		
		// a target already exists for this control - return it
		error = GetCollectionItem(gControlEventInfo, kMyControlEventInfoTypeEventTargetRef, uniqueID,
									&itemSizeActualSize, &result);
		if (error != noErr)
		{
			// error!
			Console_WriteValue("control event target get-collection-item error", error);
			result = nullptr;
		}
	}
	return result;
}// findControlEventTarget


/*!
Creates or returns the existing event target for
the specified window.  Event targets store the
list of event listeners, and other information,
for particular windows.

(3.0)
*/
static MyWindowEventTargetRef
findWindowEventTarget	(WindowRef		inForWhichWindow)
{
	MyWindowEventTargetRef		result = nullptr;
	SInt32						uniqueID = REINTERPRET_CAST(inForWhichWindow, SInt32);
	OSStatus					error = noErr;
	
	
	if (GetCollectionItemInfo(gWindowEventInfo, kMyWindowEventInfoTypeEventTargetRef, uniqueID,
								REINTERPRET_CAST(kCollectionDontWantIndex, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantSize, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantAttributes, SInt32*)) ==
		collectionItemNotFoundErr)
	{
		// no target exists for this window - create one
		result = newStandardWindowEventTarget(inForWhichWindow);
		error = AddCollectionItem(gWindowEventInfo, kMyWindowEventInfoTypeEventTargetRef, uniqueID,
									sizeof(result), &result);
		if (error != noErr)
		{
			// error!
			Console_WriteValue("window event target add-to-collection error", error);
			result = nullptr;
		}
	}
	else
	{
		SInt32		itemSizeActualSize = sizeof(result);
		
		
		// a target already exists for this window - return it
		error = GetCollectionItem(gWindowEventInfo, kMyWindowEventInfoTypeEventTargetRef, uniqueID,
									&itemSizeActualSize, &result);
		if (error != noErr)
		{
			// error!
			Console_WriteValue("window event target get-collection-item error", error);
			result = nullptr;
		}
	}
	return result;
}// findWindowEventTarget


/*!
Handles events issued when windows are activated
or deactivated.  Returns true if the event was
completely handled, false otherwise.

(3.0)
*/
static Boolean
handleActivate		(EventRecord*	inoutEventPtr,
					 Boolean		inActivating)
{
	WindowRef	window = nullptr;
	Boolean		result = false;
	
	
	window = REINTERPRET_CAST(inoutEventPtr->message, WindowRef);
	
	if (inActivating)
	{
		// assume that if the user activates a collapsed window, that he or she wants it uncollapsed
		if (IsWindowCollapsed(window)) (OSStatus)CollapseWindow(window, false);
	}
	
	unless (result)
	{
		// Appearance windows with root controls can be handled easily
		(OSStatus)Embedding_OffscreenSetRootControlActive(window, inActivating);
		result = true;
	}
	
	return result;
}// handleActivate


/*!
Responds to key presses in any window.

(3.0)
*/
static Boolean
handleKeyDown	(EventRecord*	inoutEventPtr)
{
	Boolean		result = false;
	
	
	// menu commands are handled via Carbon Events on Mac OS X
	
	unless (result)
	{
		EventInfoWindowScope_KeyPress	keyPressInfo;
		ControlRef						keyEquivalentControl = nullptr;
		Boolean							someHandlerAbsorbedEvent = false;
		
		
		// figure out which window has focus
		keyPressInfo.window = EventLoop_GetRealFrontWindow();
		keyPressInfo.event = *inoutEventPtr; // TEMPORARY - best not to rely on this structure elsewhere
		keyPressInfo.characterCode = inoutEventPtr->message & charCodeMask;
		keyPressInfo.virtualKeyCode = ((inoutEventPtr->message & keyCodeMask) >> 8);
		keyPressInfo.commandDown = ((inoutEventPtr->modifiers & cmdKey) != 0);
		keyPressInfo.controlDown = ((inoutEventPtr->modifiers & controlKey) != 0);
		keyPressInfo.optionDown = ((inoutEventPtr->modifiers & optionKey) != 0);
		keyPressInfo.shiftDown = ((inoutEventPtr->modifiers & shiftKey) != 0);
		
		// typing into a terminal window will have an effect even if the window
		// is collapsed; to ensure that the user can see this, force a collapsed,
		// *frontmost* window to uncollapse as a key is pressed
		if (IsWindowCollapsed(keyPressInfo.window))
		{
			(OSStatus)CollapseWindow(keyPressInfo.window, false);
		}
		
		// determine what to do with the keystroke; perhaps the keystroke
		// has been explicitly mapped to a button - if so, fire off an
		// event for the matching button; otherwise, propagate the event
		keyEquivalentControl = matchesKeyboardEquivalent(keyPressInfo.window, keyPressInfo.characterCode,
															keyPressInfo.virtualKeyCode, keyPressInfo.commandDown);
		if (keyEquivalentControl != nullptr)
		{
			// then a keyboard equivalent for a button was pressed;
			// simulate the button click and fire off a click event
			EventInfoControlScope_Click		clickInfo;
			
			
			clickInfo.control = keyEquivalentControl;
			clickInfo.controlPart = kControlButtonPart;
			clickInfo.event = *inoutEventPtr;
			FlashButtonControl(keyEquivalentControl, isDefaultButton(keyEquivalentControl),
								isCancelButton(keyEquivalentControl));
			someHandlerAbsorbedEvent = eventNotifyForControl(kEventLoop_ControlEventClick, clickInfo.control,
																REINTERPRET_CAST(&clickInfo, void*)/* context */);
		}
		else if (isAnyListenerForWindowEvent(keyPressInfo.window, kEventLoop_WindowEventKeyPress))
		{
			// notify listeners of key-press events that a window key press has occurred;
			// as soon as a listener absorbs the event, no remaining listeners are notified
			someHandlerAbsorbedEvent = eventNotifyForWindow(kEventLoop_WindowEventKeyPress, keyPressInfo.window,
															REINTERPRET_CAST(&keyPressInfo, void*)/* context */);
		}
		
		unless (someHandlerAbsorbedEvent)
		{
			// If there is no key press handler for the entire window,
			// exhibit the default behavior, which sends the key to
			// the user focus control.  If the control maps the key to
			// an active part of the control, a key press event is then
			// sent to interested listeners.  In this way, you can
			// almost always rely on the default behavior for handling
			// control key presses, and yet still find out about key
			// events when you need to (for example, in text fields
			// where a Return key press initiates an action).
			ControlRef		focusControl = nullptr;
			OSStatus		error = noErr;
			
			
			error = GetKeyboardFocus(keyPressInfo.window, &focusControl);
			if ((error == noErr) && (focusControl != nullptr))
			{
				// fire off key press events
				if (isAnyListenerForControlEvent(focusControl, kEventLoop_ControlEventKeyPress))
				{
					EventInfoControlScope_KeyPress		controlKeyPressInfo;
					
					
					// somebody asked to be told about key presses for this control
					controlKeyPressInfo.control = focusControl;
					controlKeyPressInfo.characterCode = keyPressInfo.characterCode;
					controlKeyPressInfo.virtualKeyCode = keyPressInfo.virtualKeyCode;
					controlKeyPressInfo.commandDown = keyPressInfo.commandDown;
					controlKeyPressInfo.controlDown = keyPressInfo.controlDown;
					controlKeyPressInfo.optionDown = keyPressInfo.optionDown;
					controlKeyPressInfo.shiftDown = keyPressInfo.shiftDown;
					someHandlerAbsorbedEvent = eventNotifyForControl
												(kEventLoop_ControlEventKeyPress, controlKeyPressInfo.control,
													REINTERPRET_CAST(&controlKeyPressInfo, void*)/* context */);
				}
				
				// send the key event to the control
				unless (someHandlerAbsorbedEvent)
				{
					if (keyPressInfo.characterCode == 0x09/* tab */)
					{
						// if there is no key press handler for the entire window,
						// exhibit the default behavior, which interprets Tab and
						// Shift-Tab to mean “alter the current user focus control”
						if (keyPressInfo.shiftDown)
						{
							error = ReverseKeyboardFocus(keyPressInfo.window);
						}
						else
						{
							error = AdvanceKeyboardFocus(keyPressInfo.window);
						}
					}
					else
					{
						(ControlPartCode)HandleControlKey(focusControl, keyPressInfo.virtualKeyCode,
															keyPressInfo.characterCode, keyPressInfo.event.modifiers);
					}
				}
			}
		}
	}
	
	return result;
}// handleKeyDown


/*!
Responds to update events for windows.  Returns
true if the event was handled, false otherwise.

(3.0)
*/
static Boolean
handleUpdate	(EventRecord*		inoutEventPtr)
{
	CGrafPtr		oldPort = nullptr;
	CGrafPtr		drawingPort = nullptr;
	GDHandle		oldDevice = nullptr;
	GDHandle		drawingDevice = nullptr;
	WindowRef		windowToUpdate = (WindowRef)inoutEventPtr->message;
	RgnHandle		visibleRegion = Memory_NewRegion();
	Boolean			result = false;
	
	
	GetGWorld(&oldPort, &oldDevice);
	
	// set the port to the event’s intended target
	SetPortWindowPort(windowToUpdate);
	GetGWorld(&drawingPort, &drawingDevice);
	if (visibleRegion != nullptr) GetPortVisibleRegion(drawingPort, visibleRegion);
	
	// do not waste time calculating and “drawing” windows that cannot be seen
	// (although receiving update events for invisible ports is very rare...)
	if (visibleRegion != nullptr) unless (EmptyRgn(visibleRegion))
	{
		WindowInfoRef		windowInfoRef = nullptr;
		
		
		if (isAnyListenerForWindowEvent(windowToUpdate, kEventLoop_WindowEventUpdate))
		{
			EventInfoWindowScope_Update		updateInfo;
			
			
			// notify listeners of update events that a window needs updating;
			// as soon as a listener absorbs the event, no remaining listeners
			// are notified
			updateInfo.window = windowToUpdate;
			updateInfo.visibleRegion = visibleRegion;
			updateInfo.drawingPort = drawingPort;
			updateInfo.drawingDevice = drawingDevice;
			eventNotifyForWindow(kEventLoop_WindowEventUpdate, updateInfo.window,
									REINTERPRET_CAST(&updateInfo, void*)/* context */);
		}
		else
		{
			// scan for known windows
			windowInfoRef = WindowInfo_GetFromWindow(windowToUpdate);
			if (windowInfoRef != nullptr)
			{
				WindowInfoDescriptor		windowDescriptor = WindowInfo_GetWindowDescriptor(windowInfoRef);
				
				
				switch (windowDescriptor)
				{
				case kConstantsRegistry_WindowDescriptorFormat:
				case kConstantsRegistry_WindowDescriptorNameNewFavorite:
				case kConstantsRegistry_WindowDescriptorSessionEditor:
					// IMPORTANT - it is only safe to call DrawDialog() on a known MacTelnet dialog
					BeginUpdate(windowToUpdate);
					UpdateDialog(GetDialogFromWindow(windowToUpdate), visibleRegion);
					EndUpdate(windowToUpdate);
					result = true;
					break;
				
				default:
					break;
				}
			}
			
			unless (result)
			{
				switch (GetWindowKind(windowToUpdate))
				{
				case WIN_ICRG: // an Interactive Color Raster Graphics window
					if (MacRGupdate(windowToUpdate)) {/* error */}
					result = true;
					break;
					
				case WIN_TEK: // a Tektronix display
					if (RGupdate(windowToUpdate) == 0) TekDisable(RGgetVG(windowToUpdate));
					else {/* error */}
					result = true;
					break;
				
				default:
					// redraw content controls; for any modern window, this is sufficient
					BeginUpdate(windowToUpdate);
					UpdateControls(windowToUpdate, visibleRegion);
					EndUpdate(windowToUpdate);
					result = true;
					break;
				}
			}
		}
	}
	
	Memory_DisposeRegion(&visibleRegion);
	SetGWorld(oldPort, oldDevice);
	
	return result;
}// handleUpdate


/*!
Returns "true" only if there is at least one
listener installed for the given event on the
specified control.  Use this to determine if
there is any handler for a particular event, or
if you should handle the event yourself.

(3.0)
*/
static Boolean
isAnyListenerForControlEvent	(ControlRef					inForWhichControl,
								 EventLoop_ControlEvent		inForWhichEvent)
{
	Boolean		result = eventTargetExistsForControl(inForWhichControl);
	
	
	if (result)
	{
		MyControlEventTargetRef		ref = findControlEventTarget(inForWhichControl);
		
		
		if (ref != nullptr)
		{
			MyControlEventTargetPtr		ptr = gControlEventTargetHandleLocks().acquireLock(ref);
			
			
			result = ListenerModel_IsAnyListenerForEvent(ptr->listenerModel, inForWhichEvent);
			gControlEventTargetHandleLocks().releaseLock(ref, &ptr);
		}
	}
	return result;
}// isAnyListenerForControlEvent


/*!
Returns "true" only if there is at least one
listener installed for the given event on the
specified window.  Use this to determine if
there is any handler for a particular event, or
if you should handle the event yourself.

(3.0)
*/
static Boolean
isAnyListenerForWindowEvent		(WindowRef					inForWhichWindow,
								 EventLoop_WindowEvent		inForWhichEvent)
{
	Boolean		result = eventTargetExistsForWindow(inForWhichWindow);
	
	
	if (result)
	{
		MyWindowEventTargetRef	ref = findWindowEventTarget(inForWhichWindow);
		
		
		if (ref != nullptr)
		{
			MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
			
			
			result = ListenerModel_IsAnyListenerForEvent(ptr->listenerModel, inForWhichEvent);
			gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
		}
	}
	return result;
}// isAnyListenerForWindowEvent


/*!
Applies a formula to figure out whether the
specified control is the Cancel button for
its window.

(3.0)
*/
static Boolean
isCancelButton		(ControlRef		inControl)
{
	Boolean					result = false;
	MyWindowEventTargetRef	ref = findWindowEventTarget(GetControlOwner(inControl));
	
	
	if (ref != nullptr)
	{
		MyWindowEventTargetConstPtr		ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
		
		
		result = (ptr->cancelButton == inControl);
		gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
	}
	return result;
}// isCancelButton


/*!
Applies a formula to figure out whether the
specified control is the default button for
its window.

(3.0)
*/
static Boolean
isDefaultButton		(ControlRef		inControl)
{
	Boolean					result = false;
	MyWindowEventTargetRef	ref = findWindowEventTarget(GetControlOwner(inControl));
	
	
	if (ref != nullptr)
	{
		MyWindowEventTargetConstPtr		ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
		
		
		result = (ptr->defaultButton == inControl);
		gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
	}
	return result;
}// isDefaultButton


/*!
Returns nullptr unless the specified character
or keyboard key matches any registered keystroke
mapping for a control when the given modifier
keys are held down.

(3.0)
*/
static inline ControlRef
matchesKeyboardEquivalent	(WindowRef	inWindow,
							 SInt16		inCharacterCode,
							 SInt16		inVirtualKeyCode,
							 Boolean	inCommandDown)
{
	return matchesKeyboardEquivalent(inWindow, createKeyboardEquivalent(inCharacterCode, inVirtualKeyCode, inCommandDown));
}// matchesKeyboardEquivalent


/*!
Returns nullptr unless the specified keystroke
information matches any registered keystroke
mapping for a control.

(3.0)
*/
static ControlRef
matchesKeyboardEquivalent	(WindowRef					inWindow,
							 EventLoop_KeyEquivalent	inKeyEquivalent)
{
	MyWindowEventTargetRef	ref = findWindowEventTarget(inWindow);
	ControlRef				result = nullptr;
	
	
	if (ref != nullptr)
	{
		MyWindowEventTargetPtr								ptr = gWindowEventTargetPtrLocks().acquireLock(ref);
		EventLoopKeyEquivalentToControlMap::const_iterator	keyEquivalentToControlIterator;
		
		
		// see if there is a mapping for the given key equivalent
		keyEquivalentToControlIterator = ptr->keyEquivalentsToControls.find(inKeyEquivalent);
		if (keyEquivalentToControlIterator != ptr->keyEquivalentsToControls.end())
		{
			result = keyEquivalentToControlIterator->second;
		}
		gWindowEventTargetPtrLocks().releaseLock(ref, &ptr);
	}
	
	return result;
}// matchesKeyboardEquivalent


/*!
Creates an event target reference for the specified
control.  Control events have higher priority than
standard windows, but lower priority than floating
windows.

Dispose of this with disposeControlEventTarget().

(3.0)
*/
static MyControlEventTargetRef
newControlEventTarget	(ControlRef		inForWhichControl)
{
	MyControlEventTargetRef		result = REINTERPRET_CAST(Memory_NewHandle(sizeof(MyControlEventTarget)),
															MyControlEventTargetRef);
	
	
	if (result != nullptr)
	{
		MyControlEventTargetPtr		ptr = gControlEventTargetHandleLocks().acquireLock(result);
		
		
		ptr->control = inForWhichControl;
		ptr->listenerModel = ListenerModel_New(kListenerModel_StyleLogicalOR,
												kConstantsRegistry_ListenerModelDescriptorEventTargetForControl);
		gControlEventTargetHandleLocks().releaseLock(result, &ptr);
	}
	return result;
}// newControlEventTarget


/*!
Creates the global event target, used for all events
that do not have a more specific target.  Dispose of
this with disposeGlobalEventTarget().

(3.0)
*/
static MyGlobalEventTargetRef
newGlobalEventTarget ()
{
	MyGlobalEventTargetRef	result = REINTERPRET_CAST(Memory_NewHandle(sizeof(MyGlobalEventTarget)),
														MyGlobalEventTargetRef);
	
	
	if (result != nullptr)
	{
		MyGlobalEventTargetPtr	ptr = gGlobalEventTargetHandleLocks().acquireLock(result);
		
		
		ptr->listenerModel = ListenerModel_New(kListenerModel_StyleLogicalOR,
												kConstantsRegistry_ListenerModelDescriptorEventTargetGlobal);
		gGlobalEventTargetHandleLocks().releaseLock(result, &ptr);
	}
	return result;
}// newGlobalEventTarget


/*!
Creates an event target reference for the specified
non-floating window.  Non-floating windows have a lower
event notification priority than floating windows do.

Dispose of this with disposeWindowEventTarget().

(3.0)
*/
static MyWindowEventTargetRef
newStandardWindowEventTarget	(WindowRef		inForWhichWindow)
{
	MyWindowEventTargetRef	result = REINTERPRET_CAST(new MyWindowEventTarget, MyWindowEventTargetRef);
	
	
	if (result != nullptr)
	{
		MyWindowEventTargetPtr	ptr = gWindowEventTargetPtrLocks().acquireLock(result);
		
		
		ptr->window = inForWhichWindow;
		ptr->listenerModel = ListenerModel_New(kListenerModel_StyleLogicalOR,
												kConstantsRegistry_ListenerModelDescriptorEventTargetForWindow);
		ptr->defaultButton = nullptr;
		ptr->cancelButton = nullptr;
		gWindowEventTargetPtrLocks().releaseLock(result, &ptr);
	}
	return result;
}// newStandardWindowEventTarget


/*!
Handles "kEventAppActivated" and "kEventAppDeactivated"
of "kEventClassApplication".

Invoked by Mac OS X whenever this application becomes
frontmost (resumed/activated) or another application
becomes frontmost when this application was formerly
frontmost (suspended/deactivated).

(3.0)
*/
static pascal OSStatus
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
Handles "kEventCommandProcess" and "kEventCommandUpdateStatus"
of "kEventClassCommand".

(3.0)
*/
static pascal OSStatus
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
		if (result == noErr)
		{
			switch (kEventKind)
			{
			case kEventCommandUpdateStatus:
				// Determine the proper state of the given command.
				//
				// WARNING:	Be VERY EFFICIENT in this code.  Since
				//			menu commands are allowed to have single-
				//			key equivalents, the Menu Manager will
				//			invoke this EVERY TIME A KEY IS PRESSED,
				//			not to mention many other times during
				//			the main event loop.  Typing will be very
				//			*slow* if this code is doing anything
				//			complicated.
				switch (received.commandID)
				{
				case kHICommandPreferences:
					{
						// set the item state for the “Preferences…” item
						Boolean		enabled = false;
						Boolean		menuKeyEquivalents = false;
						size_t		actualSize = 0;
						
						
						// this item is disabled if the Preferences window is in front
						if (EventLoop_GetRealFrontWindow() == nullptr) enabled = true;
						else
						{
							WindowInfoRef			windowInfoRef = nullptr;
							WindowInfoDescriptor	windowDescriptor = kInvalidWindowInfoDescriptor;
							
							
							windowInfoRef = WindowInfo_GetFromWindow(EventLoop_GetRealFrontWindow());
							if (windowInfoRef != nullptr)
							{
								windowDescriptor = WindowInfo_GetWindowDescriptor(windowInfoRef);
							}
							enabled = (windowDescriptor != kConstantsRegistry_WindowDescriptorPreferences);
						}
						if (enabled) EnableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						else DisableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						
						// set the menu key
						unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(menuKeyEquivalents),
													&menuKeyEquivalents, &actualSize) ==
								kPreferences_ResultCodeSuccess)
						{
							menuKeyEquivalents = true; // assume key equivalents, if preference can’t be found
						}
						(OSStatus)SetMenuItemCommandKey(received.menu.menuRef, received.menu.menuItemIndex,
														false/* is virtual key code */, (menuKeyEquivalents) ? ',' : '\0');
					}
					break;
				
				case kCommandContextSensitiveHelp:
					{
						// set the item state for the context-sensitive help item
						HelpSystem_ResultCode	helpSystemResult = kHelpSystem_ResultCodeSuccess;
						Boolean					menuKeyEquivalents = false;
						size_t					actualSize = 0;
						CFStringRef				itemText = nullptr;
						
						
						// set enabled state; inactive only if no particular help context is set
						if (HelpSystem_GetCurrentContextKeyPhrase() == kHelpSystem_KeyPhraseDefault)
						{
							DisableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
						else
						{
							EnableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
						
						// set the item text
						helpSystemResult = HelpSystem_CopyKeyPhraseCFString
											(HelpSystem_GetCurrentContextKeyPhrase(), itemText);
						if (helpSystemResult == kHelpSystem_ResultCodeSuccess)
						{
							SetMenuItemTextWithCFString(received.menu.menuRef, received.menu.menuItemIndex, itemText);
							CFRelease(itemText);
						}
						
						// set the menu key
						unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(menuKeyEquivalents),
													&menuKeyEquivalents, &actualSize) ==
								kPreferences_ResultCodeSuccess)
						{
							menuKeyEquivalents = true; // assume key equivalents, if preference can’t be found
						}
						(OSStatus)SetMenuItemCommandKey(received.menu.menuRef, received.menu.menuItemIndex,
														false/* is virtual key code */, (menuKeyEquivalents) ? '?' : '\0');
						SetMenuItemModifiers(received.menu.menuRef, received.menu.menuItemIndex,
											(menuKeyEquivalents) ? kMenuControlModifier : kMenuNoModifiers);
					}
					break;
				
				case kCommandShowHelpTags:
				case kCommandHideHelpTags:
					{
						// set the item state for the “Show Help Tags” item
						Boolean		menuKeyEquivalents = false;
						Boolean		hideCondition = false;
						size_t		actualSize = 0;
						
						
						// hide item if it is not applicable; otherwise show it
						hideCondition = (received.commandID == kCommandShowHelpTags)
											? HMAreHelpTagsDisplayed()
											: (false == HMAreHelpTagsDisplayed());
						if (hideCondition)
						{
							ChangeMenuItemAttributes(received.menu.menuRef, received.menu.menuItemIndex,
														kMenuItemAttrHidden/* attributes to set */, 0L);
						}
						else
						{
							ChangeMenuItemAttributes(received.menu.menuRef, received.menu.menuItemIndex,
														0L, kMenuItemAttrHidden/* attributes to clear */);
						}
						
						// set the menu key
						unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(menuKeyEquivalents),
													&menuKeyEquivalents, &actualSize) ==
								kPreferences_ResultCodeSuccess)
						{
							menuKeyEquivalents = true; // assume key equivalents, if preference can’t be found
						}
						(OSStatus)SetMenuItemCommandKey(received.menu.menuRef, received.menu.menuItemIndex,
														false/* is virtual key code */, (menuKeyEquivalents) ? '?' : '\0');
						SetMenuItemModifiers(received.menu.menuRef, received.menu.menuItemIndex,
											(menuKeyEquivalents) ? kMenuOptionModifier : kMenuNoModifiers);
					}
					break;
				
				case kHICommandCustomizeToolbar:
					{
						// set the item state for the “Show Toolbar” item
						WindowRef		window = GetUserFocusWindow();
						HIToolbarRef	targetToolbar = nullptr;
						OptionBits		toolbarAttributes = 0;
						OSStatus		error = (nullptr == window)
													? STATIC_CAST(memPCErr, OSStatus)
													: GetWindowToolbar(window, &targetToolbar);
						
						
						if ((noErr == error) && (nullptr != targetToolbar))
						{
							error = HIToolbarGetAttributes(targetToolbar, &toolbarAttributes);
						}
						
						if ((noErr != error) || (nullptr == targetToolbar) ||
							(0 == (kHIToolbarIsConfigurable & toolbarAttributes)))
						{
							// the customize command does not apply if no window is open,
							// if there is no toolbar, or if the toolbar is not configurable
							DisableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
						else
						{
							EnableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
					}
					break;
				
				case kHICommandShowToolbar:
				case kHICommandHideToolbar:
					{
						// set the item state for the “Show Toolbar” and “Hide Toolbar” items
						HIWindowRef			window = GetUserFocusWindow();
						WindowAttributes	windowAttributes = 0;
						OSStatus			error = (nullptr == window)
														? STATIC_CAST(memPCErr, OSStatus)
														: GetWindowAttributes(window, &windowAttributes);
						Boolean				toolbarCanBeHidden = false;
						
						
						if ((noErr == error) &&
							(windowAttributes & kWindowToolbarButtonAttribute) &&
							IsWindowToolbarVisible(window))
						{
							toolbarCanBeHidden = true;
						}
						
						// hide item if it is not applicable; otherwise show it
						{
							// for windows without toolbars or windows where the toolbar
							// state cannot be changed, hide the Hide Toolbar item
							// and dim the Show Toolbar item; otherwise, show the one
							// that is applicable for the current toolbar state
							Boolean		hideCondition = (received.commandID == kHICommandShowToolbar)
															? toolbarCanBeHidden
															: (false == toolbarCanBeHidden);
							
							
							if (hideCondition)
							{
								ChangeMenuItemAttributes(received.menu.menuRef, received.menu.menuItemIndex,
															kMenuItemAttrHidden/* attributes to set */, 0L);
							}
							else
							{
								ChangeMenuItemAttributes(received.menu.menuRef, received.menu.menuItemIndex,
															0L, kMenuItemAttrHidden/* attributes to clear */);
							}
						}
						
						// disable the Show Toolbar item if there is no window or no toolbar
						// or a toolbar that cannot be explicitly hidden
						if ((noErr != error) ||
							(0 == (windowAttributes & kWindowToolbarButtonAttribute)))
						{
							DisableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
						else
						{
							EnableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
					}
					break;
				
				default:
					{
						UInt32		modifiers = 0L;
						
						
						// determine the modifier key states
						result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32,
																		modifiers);
						
						// NOTE: The following type cast to EventModifiers is okay because
						//       only the bottom bits are required for menu processing and
						//       these are compatible.  But strictly speaking the entire
						//       EventModifiers type is not completely compatible with the
						//       UInt32 returned by Carbon Events.
						MenuBar_SetUpMenuItemState(received.commandID, STATIC_CAST(modifiers, EventModifiers));
					}
					break;
				}
				break;
			
			case kEventCommandProcess:
				// Execute a command selected from a menu.
				//
				// IMPORTANT: This could imply ANY form of menu selection, whether from
				//            the menu bar, from a contextual menu, or from a pop-up menu!
				{
					UInt32		modifiers = 0;
					OSStatus	error = noErr;
					
					
					// determine state of modifier keys when command was chosen
					error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32,
																	modifiers);
					
					if (received.commandID != 0)
					{
						Boolean		commandOK = false;
						
						
						// execute the command; note that EventModifiers is not entirely
						// compatible with the UInt32 returned by Carbon Events, but the
						// masks used for command modifications - shiftKey, optionKey,
						// commandKey and controlKey - ARE COMPATIBLE with the UInt32
						(Boolean)Commands_ModifyID(&received.commandID, STATIC_CAST(modifiers, EventModifiers));
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
						// items without command IDs must be parsed individually; note that the EventModifiers
						// type is not entirely compatible with the UInt32 returned by Carbon Events, but for
						// the purposes of handling menu commands all the important bits (cmdKey, shiftKey, etc.)
						// are in fact directly compatible
						if (MenuBar_HandleMenuCommand
								(received.menu.menuRef, received.menu.menuItemIndex,
									STATIC_CAST(modifiers, EventModifiers)))
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
Handles "kEventMouseWheelMoved" of "kEventClassMouse".

Invoked by Mac OS X whenever a mouse with a scrolling
function is used on the frontmost window.  (On Mac OS 9,
support for mouse wheels is automatic.)

(3.0)
*/
static pascal OSStatus
receiveMouseWheelEvent	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent),
					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassMouse);
	assert(kEventKind == kEventMouseWheelMoved);
	{
		EventMouseWheelAxis		axis = kEventMouseWheelAxisY;
		
		
		// find out which way the mouse wheel moved
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, axis);
		
		// if the axis information was found, continue
		if (result == noErr)
		{
			SInt32		delta = 0;
			UInt32		modifiers = 0;
			
			
			// determine modifier keys pressed during scroll
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
			result = noErr; // ignore modifier key parameter if absent
			
			// determine how far the mouse wheel was scrolled
			// and in which direction; negative means up/left,
			// positive means down/right
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeLongInteger, delta);
			
			// if all information can be found, proceed with scrolling
			if (result == noErr)
			{
				EventInfoWindowScope_Scroll		scrollInfo;
				
				
				// simply construct an internal “window scrolling” event and
				// send it to all listeners who care to know about it
				if (GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef,
										nullptr/* actual type */, sizeof(scrollInfo.window),
										nullptr/* actual size */, &scrollInfo.window) != noErr)
				{
					// cannot find information (implies Mac OS X 10.0.x) - fine, assume frontmost window
					scrollInfo.window = EventLoop_GetRealFrontWindow();
				}
				scrollInfo.horizontal.affected = (axis == kEventMouseWheelAxisX);
				scrollInfo.vertical.affected = (axis == kEventMouseWheelAxisY);
				if (scrollInfo.horizontal.affected)
				{
					if (delta > 0)
					{
						scrollInfo.horizontal.scrollBarPartCode = (modifiers & optionKey)
																	? kControlPageUpPart : kControlUpButtonPart;
					}
					else
					{
						scrollInfo.horizontal.scrollBarPartCode = (modifiers & optionKey)
																	? kControlPageDownPart : kControlDownButtonPart;
					}
				}
				if (scrollInfo.vertical.affected)
				{
					if (delta > 0)
					{
						scrollInfo.vertical.scrollBarPartCode = (modifiers & optionKey)
																	? kControlPageUpPart : kControlUpButtonPart;
					}
					else
					{
						scrollInfo.vertical.scrollBarPartCode = (modifiers & optionKey)
																	? kControlPageDownPart : kControlDownButtonPart;
					}
				}
				eventNotifyForWindow(kEventLoop_WindowEventScrolling, scrollInfo.window, &scrollInfo);
				result = noErr;
			}
		}
	}
	return result;
}// receiveMouseWheelEvent


/*!
Handles "kEventServicePerform" of "kEventClassService".

Invoked by Mac OS X whenever an item in the Services
menu is invoked for a piece of data.

(3.0)
*/
static pascal OSStatus
receiveServicePerformEvent	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassService);
	assert(kEventKind == kEventServicePerform);
	{
		ScrapRef	sourceScrap = nullptr;
		
		
		// grab the scrap that contains the data on which to perform the Open URL service
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamScrapRef, typeScrapRef, sourceScrap);
		
		// if the scrap source was found, proceed
		if (noErr == result)
		{
			// retrieve text from the given scrap and interpret it as a URL
			Boolean				isAnyScrap = false;		// is any text on the clipboard?
			ScrapFlavorFlags	scrapFlags = 0L;
			
			
			isAnyScrap = (noErr == GetScrapFlavorFlags(sourceScrap, kScrapFlavorTypeText, &scrapFlags));
			if (isAnyScrap)
			{
				long	length = 0L;	// the number of bytes in the scrap
				
				
				// allocate a buffer large enough to hold the data, then copy it
				result = GetScrapFlavorSize(sourceScrap, kScrapFlavorTypeText, &length);
				if (noErr == result)
				{
					Handle		handle = Memory_NewHandle(length); // for characters
					
					
					if (nullptr != handle)
					{
						result = GetScrapFlavorData(sourceScrap, kScrapFlavorTypeText, &length, *handle);
						if (noErr == result)
						{
							// handle the URL!
							std::string		urlString(*handle, *handle + GetHandleSize(handle));
							
							
							Quills::Session::handle_url(urlString);
						}
						Memory_DisposeHandle(&handle);
					}
				}
			}
		}
		Alert_ReportOSStatus(result);
	}
	return result;
}// receiveServicePerformEvent


/*!
Handles "kEventWindowSheetOpening" of "kEventClassWindow".

Invoked by Mac OS X whenever a sheet is about to open.
This handler could prevent the opening by returning
"userCanceledErr".

(3.1)
*/
static pascal OSStatus
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
			// completely cosmetic, so any errors are ignored, etc.; this hack works
			// on 10.4 and 10.5, but do NOT assume it does on 10.6 or 10.3 (for now)
			if (FlagManager_Test(kFlagOS10_4API) && (false == FlagManager_Test(kFlagOS10_6API)) &&
				(noErr == CopyWindowTitleAsCFString(sheetWindow, &sheetTitleCFString)))
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
Invoked by Mac OS X whenever a modifier key’s
state changes (e.g. option, control, command,
or shift).  This routine updates an internal
variable that is used by other functions
(such as EventLoop_IsCommandKeyDown()).

(3.0)
*/
static pascal OSStatus
updateModifiers		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = noErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassKeyboard);
	assert(kEventKind == kEventRawKeyModifiersChanged);
	{
		// extract modifier key bits from the given event
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, gCarbonEventModifiers);
		
		// if the modifier key information was found, proceed
		if (result == noErr)
		{
			if (FlagManager_Test(kFlagSuspended)) result = eventNotHandledErr;
		}
	}
	return result;
}// updateModifiers

// BELOW IS REQUIRED NEWLINE TO END FILE
