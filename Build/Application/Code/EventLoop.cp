/*###############################################################

	EventLoop.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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
#include "GeneralResources.h"
#include "MenuResources.h"

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
#include "Terminology.h"
#include "VectorCanvas.h"
#include "VectorInterpreter.h"



#pragma mark Constants

/*!
Information stored in the view event info collection.
*/
typedef SInt32 My_ViewEventInfoType;
enum
{
	kMy_ViewEventInfoTypeEventTargetRef			= 'cetg'		// data: My_ViewEventTargetRef
};

/*!
Information stored in the window event info collection.
*/
typedef SInt32 My_WindowEventInfoType;
enum
{
	kMy_WindowEventInfoTypeEventTargetRef		= 'wetg'		// data: My_WindowEventTargetRef
};

#pragma mark Types

struct My_ViewEventTarget
{
	HIViewRef			control;
	ListenerModel_Ref	listenerModel;
};
typedef My_ViewEventTarget*						My_ViewEventTargetPtr;
typedef My_ViewEventTarget const*				My_ViewEventTargetConstPtr;
typedef My_ViewEventTarget**					My_ViewEventTargetHandle;
typedef struct My_OpaqueViewEventTarget**		My_ViewEventTargetRef;

typedef MemoryBlockHandleLocker< My_ViewEventTargetRef, My_ViewEventTarget >	My_ViewEventTargetHandleLocker;
typedef LockAcquireRelease< My_ViewEventTargetRef, My_ViewEventTarget >			My_ViewEventTargetAutoLocker;

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

struct My_WindowEventTarget
{
	HIWindowRef				window;
	ListenerModel_Ref		listenerModel;
	HIViewRef				defaultButton;
	HIViewRef				cancelButton;
};
typedef My_WindowEventTarget*				My_WindowEventTargetPtr;
typedef My_WindowEventTarget const*			My_WindowEventTargetConstPtr;
typedef My_WindowEventTarget**				My_WindowEventTargetHandle;
typedef struct My_OpaqueWindowEventTarget**	My_WindowEventTargetRef;

typedef MemoryBlockPtrLocker< My_WindowEventTargetRef, My_WindowEventTarget >	My_WindowEventTargetPtrLocker;
typedef LockAcquireRelease< My_WindowEventTargetRef, My_WindowEventTarget >		My_WindowEventTargetAutoLocker;

#pragma mark Internal Method Prototypes

static void						disposeGlobalEventTarget		(My_GlobalEventTargetRef*);
static void						disposeViewEventTarget			(My_ViewEventTargetRef*);
static void						disposeWindowEventTarget		(My_WindowEventTargetRef*);
static void						eventNotifyGlobal				(EventLoop_GlobalEvent, void*);
static Boolean					eventNotifyForControl			(EventLoop_ControlEvent, HIViewRef, void*);
static Boolean					eventNotifyForWindow			(EventLoop_WindowEvent, HIWindowRef, void*);
static Boolean					eventTargetExistsForControl		(HIViewRef);
static Boolean					eventTargetExistsForWindow		(HIWindowRef);
static My_ViewEventTargetRef	findViewEventTarget				(HIViewRef);
static My_WindowEventTargetRef	findWindowEventTarget			(HIWindowRef);
static Boolean					handleActivate					(EventRecord*, Boolean);
static Boolean					handleKeyDown					(EventRecord*);
static Boolean					handleUpdate					(EventRecord*);
static Boolean					isAnyListenerForViewEvent		(HIViewRef, EventLoop_ControlEvent);
static Boolean					isAnyListenerForWindowEvent		(HIWindowRef, EventLoop_WindowEvent);
static My_GlobalEventTargetRef	newGlobalEventTarget			();
static My_WindowEventTargetRef	newStandardWindowEventTarget	(HIWindowRef);
static My_ViewEventTargetRef	newViewEventTarget				(HIViewRef);
static pascal OSStatus			receiveApplicationSwitch		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			receiveServicePerformEvent		(EventHandlerCallRef, EventRef, void*);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
static pascal OSStatus			receiveSheetOpening				(EventHandlerCallRef, EventRef, void*);
#endif
static pascal OSStatus			receiveWindowActivated			(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus			updateModifiers					(EventHandlerCallRef, EventRef, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_GlobalEventTargetRef				gGlobalEventTarget = nullptr;
	Collection							gViewEventInfo = nullptr;
	Collection							gWindowEventInfo = nullptr;
	SInt16								gHaveInstalledNotification = 0;
	UInt32								gTicksWaitNextEvent = 0L;
	NMRec*								gBeepNotificationPtr = nullptr;
	My_ViewEventTargetHandleLocker&		gViewEventTargetHandleLocks ()		{ static My_ViewEventTargetHandleLocker x; return x; }
	My_GlobalEventTargetHandleLocker&	gGlobalEventTargetHandleLocks ()	{ static My_GlobalEventTargetHandleLocker x; return x; }
	My_WindowEventTargetPtrLocker&		gWindowEventTargetPtrLocks ()		{ static My_WindowEventTargetPtrLocker x; return x; }
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
	CarbonEventHandlerWrap				gCarbonEventServiceHandler(GetApplicationEventTarget(),
																	receiveServicePerformEvent,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassService),
																			kEventServicePerform),
																	nullptr/* user data */);
	Console_Assertion					_3(gCarbonEventServiceHandler.isInstalled(), __FILE__, __LINE__);
	CarbonEventHandlerWrap				gCarbonEventSwitchHandler(GetApplicationEventTarget(),
																	receiveApplicationSwitch,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassApplication),
																			kEventAppActivated, kEventAppDeactivated),
																	nullptr/* user data */);
	Console_Assertion					_4(gCarbonEventSwitchHandler.isInstalled(), __FILE__, __LINE__);
	CarbonEventHandlerWrap				gCarbonEventWindowActivateHandler(GetApplicationEventTarget(),
																			receiveWindowActivated,
																			CarbonEventSetInClass
																				(CarbonEventClass(kEventClassWindow),
																					kEventWindowActivated),
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
EventLoop_Result
EventLoop_Init ()
{
	EventLoop_Result	result = noErr;
	
	
	// set the sleep time (3.0 - don’t use preferences value, it’s not user-specifiable anymore)
	gTicksWaitNextEvent = 60; // make this larger to increase likelihood of high-frequency timers firing on time
	
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
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
		// don’t check for errors, it’s not critical if this handler is installed
	}
#endif
	
	// create listener models to handle event notifications
	gGlobalEventTarget = newGlobalEventTarget();
	gViewEventInfo = NewCollection();
	gWindowEventInfo = NewCollection();
	
	// the update-status handler for Preferences does not appear to be
	// called soon enough to properly initialize its state, so it is
	// explicitly enabled here
	EnableMenuCommand(nullptr/* menu */, kHICommandPreferences);
	
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
		My_ViewEventTargetRef		eventTarget = nullptr;
		SInt32						itemCount = CountTaggedCollectionItems
												(gViewEventInfo, kMy_ViewEventInfoTypeEventTargetRef);
		SInt32						itemSizeActualSize = sizeof(eventTarget);
		register SInt32				i = 0;
		OSStatus					error = noErr;
		
		
		for (i = 1; i <= itemCount; ++i)
		{
			error = GetTaggedCollectionItem(gViewEventInfo, kMy_ViewEventInfoTypeEventTargetRef, i,
											&itemSizeActualSize, &eventTarget);
			if (error == noErr) disposeViewEventTarget(&eventTarget);
		}
	}
	DisposeCollection(gViewEventInfo), gViewEventInfo = nullptr;
	
	// destroy any window event targets that were allocated
	{
		My_WindowEventTargetRef		eventTarget = nullptr;
		SInt32						itemCount = CountTaggedCollectionItems
												(gWindowEventInfo, kMy_WindowEventInfoTypeEventTargetRef);
		SInt32						itemSizeActualSize = sizeof(eventTarget);
		register SInt32				i = 0;
		OSStatus					error = noErr;
		
		
		for (i = 1; i <= itemCount; ++i)
		{
			error = GetTaggedCollectionItem(gWindowEventInfo, kMy_WindowEventInfoTypeEventTargetRef, i,
											&itemSizeActualSize, &eventTarget);
			if (error == noErr) disposeWindowEventTarget(&eventTarget);
		}
	}
	DisposeCollection(gWindowEventInfo), gWindowEventInfo = nullptr;
}// Done


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
EventLoop_HandleZoomEvent	(HIWindowRef	inWindow)
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
EventLoop_SelectBehindDialogWindows		(HIWindowRef	inWindow)
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
EventLoop_SelectOverRealFrontWindow		(HIWindowRef	inWindow)
{
	// I give up, dammit; it’s too hard to prevent
	// activate events from occurring on Mac OS 8/9.
	SelectWindow(inWindow);
}// SelectOverRealFrontWindow


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


#pragma mark Internal Methods

/*!
Destroys an event target reference created with the
newViewEventTarget() routine.

(3.0)
*/
static void
disposeViewEventTarget	(My_ViewEventTargetRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		{
			My_ViewEventTargetAutoLocker	ptr(gViewEventTargetHandleLocks(), *inoutRefPtr);
			
			
			if (ptr != nullptr)
			{
				// dispose of data members
				ListenerModel_Dispose(&ptr->listenerModel);
			}
		}
		Memory_DisposeHandle(REINTERPRET_CAST(inoutRefPtr, Handle*));
	}
}// disposeViewEventTarget


/*!
Destroys an event target reference created with the
newGlobalEventTarget() routine.

(3.0)
*/
static void
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
Destroys an event target reference created with the
newWindowEventTarget() routine.

(3.0)
*/
static void
disposeWindowEventTarget	(My_WindowEventTargetRef*	inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		{
			My_WindowEventTargetAutoLocker	ptr(gWindowEventTargetPtrLocks(), *inoutRefPtr);
			
			
			if (ptr != nullptr)
			{
				// dispose of data members
				ListenerModel_Dispose(&ptr->listenerModel);
			}
		}
		delete *(REINTERPRET_CAST(inoutRefPtr, My_WindowEventTargetPtr*)), *inoutRefPtr = nullptr;
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
	My_GlobalEventTargetAutoLocker	ptr(gGlobalEventTargetHandleLocks(), gGlobalEventTarget);
	
	
	// invoke listener callback routines appropriately
	ListenerModel_NotifyListenersOfEvent(ptr->listenerModel, inEventThatOccurred, inContextPtr);
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
						 HIViewRef					inForWhichControl,
						 void*						inoutContextPtr)
{
	My_ViewEventTargetRef	ref = findViewEventTarget(inForWhichControl);
	Boolean					result = false;
	
	
	if (ref != nullptr)
	{
		My_ViewEventTargetAutoLocker	ptr(gViewEventTargetHandleLocks(), ref);
		
		
		// invoke listener callback routines appropriately, from the specified control’s event target
		ListenerModel_NotifyListenersOfEvent(ptr->listenerModel, inEventThatOccurred, inoutContextPtr,
												REINTERPRET_CAST(&result, Boolean*));
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
						 HIWindowRef			inForWhichWindow,
						 void*					inoutContextPtr)
{
	My_WindowEventTargetRef		ref = findWindowEventTarget(inForWhichWindow);
	Boolean						result = false;
	
	
	if (ref != nullptr)
	{
		My_WindowEventTargetAutoLocker	ptr(gWindowEventTargetPtrLocks(), ref);
		
		
		// invoke listener callback routines appropriately, from the specified window’s event target
		ListenerModel_NotifyListenersOfEvent(ptr->listenerModel, inEventThatOccurred, inoutContextPtr,
												REINTERPRET_CAST(&result, Boolean*));
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
eventTargetExistsForControl		(HIViewRef		inForWhichControl)
{
	Boolean		result = false;
	SInt32		uniqueID = REINTERPRET_CAST(inForWhichControl, SInt32);
	
	
	if (inForWhichControl != nullptr)
	{
		OSStatus		error = noErr;
		
		
		error = GetCollectionItemInfo(gViewEventInfo, kMy_ViewEventInfoTypeEventTargetRef, uniqueID,
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
		
		
		error = GetCollectionItemInfo(gWindowEventInfo, kMy_WindowEventInfoTypeEventTargetRef, uniqueID,
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
static My_ViewEventTargetRef
findViewEventTarget		(HIViewRef		inForWhichControl)
{
	My_ViewEventTargetRef	result = nullptr;
	SInt32					uniqueID = REINTERPRET_CAST(inForWhichControl, SInt32);
	OSStatus				error = noErr;
		
		
	if (GetCollectionItemInfo(gViewEventInfo, kMy_ViewEventInfoTypeEventTargetRef, uniqueID,
								REINTERPRET_CAST(kCollectionDontWantIndex, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantSize, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantAttributes, SInt32*)) ==
		collectionItemNotFoundErr)
	{
		// no target exists for this control - create one
		result = newViewEventTarget(inForWhichControl);
		error = AddCollectionItem(gViewEventInfo, kMy_ViewEventInfoTypeEventTargetRef, uniqueID,
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
		error = GetCollectionItem(gViewEventInfo, kMy_ViewEventInfoTypeEventTargetRef, uniqueID,
									&itemSizeActualSize, &result);
		if (error != noErr)
		{
			// error!
			Console_WriteValue("control event target get-collection-item error", error);
			result = nullptr;
		}
	}
	return result;
}// findViewEventTarget


/*!
Creates or returns the existing event target for
the specified window.  Event targets store the
list of event listeners, and other information,
for particular windows.

(3.0)
*/
static My_WindowEventTargetRef
findWindowEventTarget	(WindowRef		inForWhichWindow)
{
	My_WindowEventTargetRef		result = nullptr;
	SInt32						uniqueID = REINTERPRET_CAST(inForWhichWindow, SInt32);
	OSStatus					error = noErr;
	
	
	if (GetCollectionItemInfo(gWindowEventInfo, kMy_WindowEventInfoTypeEventTargetRef, uniqueID,
								REINTERPRET_CAST(kCollectionDontWantIndex, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantSize, SInt32*),
								REINTERPRET_CAST(kCollectionDontWantAttributes, SInt32*)) ==
		collectionItemNotFoundErr)
	{
		// no target exists for this window - create one
		result = newStandardWindowEventTarget(inForWhichWindow);
		error = AddCollectionItem(gWindowEventInfo, kMy_WindowEventInfoTypeEventTargetRef, uniqueID,
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
		error = GetCollectionItem(gWindowEventInfo, kMy_WindowEventInfoTypeEventTargetRef, uniqueID,
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
		Boolean							someHandlerAbsorbedEvent = false;
		
		
		// figure out which window has focus
		keyPressInfo.window = EventLoop_ReturnRealFrontWindow();
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
		
		// propagate the event
		if (isAnyListenerForWindowEvent(keyPressInfo.window, kEventLoop_WindowEventKeyPress))
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
			HIViewRef		focusControl = nullptr;
			OSStatus		error = noErr;
			
			
			error = GetKeyboardFocus(keyPressInfo.window, &focusControl);
			if ((error == noErr) && (focusControl != nullptr))
			{
				// fire off key press events
				if (isAnyListenerForViewEvent(focusControl, kEventLoop_ControlEventKeyPress))
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
		WindowInfo_Ref		windowInfoRef = nullptr;
		
		
		// scan for known windows
		windowInfoRef = WindowInfo_ReturnFromWindow(windowToUpdate);
		if (windowInfoRef != nullptr)
		{
			WindowInfo_Descriptor	windowDescriptor = WindowInfo_ReturnWindowDescriptor(windowInfoRef);
			
			
			switch (windowDescriptor)
			{
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
isAnyListenerForViewEvent	(HIViewRef					inForWhichControl,
							 EventLoop_ControlEvent		inForWhichEvent)
{
	Boolean		result = eventTargetExistsForControl(inForWhichControl);
	
	
	if (result)
	{
		My_ViewEventTargetRef	ref = findViewEventTarget(inForWhichControl);
		
		
		if (ref != nullptr)
		{
			My_ViewEventTargetAutoLocker	ptr(gViewEventTargetHandleLocks(), ref);
			
			
			result = ListenerModel_IsAnyListenerForEvent(ptr->listenerModel, inForWhichEvent);
		}
	}
	return result;
}// isAnyListenerForViewEvent


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
		My_WindowEventTargetRef		ref = findWindowEventTarget(inForWhichWindow);
		
		
		if (ref != nullptr)
		{
			My_WindowEventTargetAutoLocker	ptr(gWindowEventTargetPtrLocks(), ref);
			
			
			result = ListenerModel_IsAnyListenerForEvent(ptr->listenerModel, inForWhichEvent);
		}
	}
	return result;
}// isAnyListenerForWindowEvent


/*!
Creates an event target reference for the specified
control.  Control events have higher priority than
standard windows, but lower priority than floating
windows.

Dispose of this with disposeViewEventTarget().

(3.0)
*/
static My_ViewEventTargetRef
newViewEventTarget	(HIViewRef		inForWhichControl)
{
	My_ViewEventTargetRef	result = REINTERPRET_CAST(Memory_NewHandle(sizeof(My_ViewEventTarget)),
														My_ViewEventTargetRef);
	
	
	if (result != nullptr)
	{
		My_ViewEventTargetAutoLocker	ptr(gViewEventTargetHandleLocks(), result);
		
		
		ptr->control = inForWhichControl;
		ptr->listenerModel = ListenerModel_New(kListenerModel_StyleLogicalOR,
												kConstantsRegistry_ListenerModelDescriptorEventTargetForControl);
	}
	return result;
}// newViewEventTarget


/*!
Creates the global event target, used for all events
that do not have a more specific target.  Dispose of
this with disposeGlobalEventTarget().

(3.0)
*/
static My_GlobalEventTargetRef
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
Creates an event target reference for the specified
non-floating window.  Non-floating windows have a lower
event notification priority than floating windows do.

Dispose of this with disposeWindowEventTarget().

(3.0)
*/
static My_WindowEventTargetRef
newStandardWindowEventTarget	(HIWindowRef	inForWhichWindow)
{
	My_WindowEventTargetRef		result = REINTERPRET_CAST(new My_WindowEventTarget, My_WindowEventTargetRef);
	
	
	if (result != nullptr)
	{
		My_WindowEventTargetAutoLocker	ptr(gWindowEventTargetPtrLocks(), result);
		
		
		ptr->window = inForWhichWindow;
		ptr->listenerModel = ListenerModel_New(kListenerModel_StyleLogicalOR,
												kConstantsRegistry_ListenerModelDescriptorEventTargetForWindow);
		ptr->defaultButton = nullptr;
		ptr->cancelButton = nullptr;
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
						if (EventLoop_ReturnRealFrontWindow() == nullptr) enabled = true;
						else
						{
							WindowInfo_Ref			windowInfoRef = nullptr;
							WindowInfo_Descriptor	windowDescriptor = kWindowInfo_InvalidDescriptor;
							
							
							windowInfoRef = WindowInfo_ReturnFromWindow(EventLoop_ReturnRealFrontWindow());
							if (windowInfoRef != nullptr)
							{
								windowDescriptor = WindowInfo_ReturnWindowDescriptor(windowInfoRef);
							}
							enabled = (windowDescriptor != kConstantsRegistry_WindowDescriptorPreferences);
						}
						if (enabled) EnableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						else DisableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						
						// set the menu key
						unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(menuKeyEquivalents),
													&menuKeyEquivalents, &actualSize) ==
								kPreferences_ResultOK)
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
						HelpSystem_Result	helpSystemResult = kHelpSystem_ResultOK;
						Boolean				menuKeyEquivalents = false;
						size_t				actualSize = 0;
						CFStringRef			itemText = nullptr;
						
						
						// set enabled state; inactive only if no particular help context is set
						if (HelpSystem_ReturnCurrentContextKeyPhrase() == kHelpSystem_KeyPhraseDefault)
						{
							DisableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
						else
						{
							EnableMenuItem(received.menu.menuRef, received.menu.menuItemIndex);
						}
						
						// set the item text
						helpSystemResult = HelpSystem_CopyKeyPhraseCFString
											(HelpSystem_ReturnCurrentContextKeyPhrase(), itemText);
						if (helpSystemResult == kHelpSystem_ResultOK)
						{
							SetMenuItemTextWithCFString(received.menu.menuRef, received.menu.menuItemIndex, itemText);
							CFRelease(itemText);
						}
						
						// set the menu key
						unless (Preferences_GetData(kPreferences_TagMenuItemKeys, sizeof(menuKeyEquivalents),
													&menuKeyEquivalents, &actualSize) ==
								kPreferences_ResultOK)
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
								kPreferences_ResultOK)
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
						HIWindowRef		window = GetUserFocusWindow();
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
					MenuBar_SetUpMenuItemState(received.commandID);
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
							
							
							try
							{
								Quills::Session::handle_url(urlString);
							}
							catch (std::exception const&	e)
							{
								Console_Warning(Console_WriteValueCString, "caught exception while trying to handle URL for Service",
												e.what());
								result = eventNotHandledErr;
							}
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


#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
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
#endif


/*!
Embellishes "kEventWindowActivated" of "kEventClassWindow"
by resetting the mouse cursor.

(3.1)
*/
static pascal OSStatus
receiveWindowActivated	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowActivated);
	
	Cursors_UseArrow();
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveWindowActivated


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
