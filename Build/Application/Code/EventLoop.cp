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
#include <StringUtilities.h>
#include <WindowInfo.h>

// resource includes
#include "GeneralResources.h"
#include "MenuResources.h"

// MacTelnet includes
#include "Clipboard.h"
#include "Commands.h"
#include "ConnectionData.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "HelpSystem.h"
#include "MenuBar.h"
#include "Preferences.h"
#include "QuillsSession.h"
#include "RasterGraphicsScreen.h"
#include "Session.h"
#include "SessionFactory.h"
#include "TerminalView.h"
#include "TerminalWindow.h"
#include "Terminology.h"
#include "VectorCanvas.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

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

} // anonymous namespace

#pragma mark Types
namespace {

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

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void						disposeGlobalEventTarget		(My_GlobalEventTargetRef*);
void						eventNotifyGlobal				(EventLoop_GlobalEvent, void*);
My_GlobalEventTargetRef	newGlobalEventTarget			();
pascal OSStatus			receiveApplicationSwitch		(EventHandlerCallRef, EventRef, void*);
pascal OSStatus			receiveHICommand				(EventHandlerCallRef, EventRef, void*);
pascal OSStatus			receiveServicesEvent			(EventHandlerCallRef, EventRef, void*);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
pascal OSStatus			receiveSheetOpening				(EventHandlerCallRef, EventRef, void*);
#endif
pascal OSStatus			receiveWindowActivated			(EventHandlerCallRef, EventRef, void*);
pascal OSStatus			updateModifiers					(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

My_GlobalEventTargetRef				gGlobalEventTarget = nullptr;
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
																receiveServicesEvent,
																CarbonEventSetInClass
																	(CarbonEventClass(kEventClassService),
																		kEventServiceGetTypes, kEventServiceCopy, kEventServicePerform),
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
}// Done


/*!
This method handles Color Picker 2.0+ event
requests in as simple a way as possible: by
responding to update events.  Time is also
given to other running threads.

(3.0)
*/
pascal Boolean
EventLoop_HandleColorPickerUpdate	(EventRecord*		UNUSED_ARGUMENT(inoutEventPtr))
{
	Boolean		result = pascal_false; // event handled?
	
	
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
pascal void
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
pascal OSStatus
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
pascal OSStatus
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
Handles "kEventServiceGetTypes", "kEventServiceCopy" and
"kEventServicePerform" of "kEventClassService".

Invoked by Mac OS X whenever an item in the Services menu is
invoked for a piece of data.

(3.0)
*/
pascal OSStatus
receiveServicesEvent	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassService);
	assert((kEventKind == kEventServiceGetTypes) || (kEventKind == kEventServiceCopy) || (kEventKind == kEventServicePerform));
	
	switch (kEventKind)
	{
	case kEventServiceCopy:
	case kEventServiceGetTypes:
		// return data or type information for the current selection;
		// currently, only terminal window text is supported
		{
			SessionRef		activeSession = SessionFactory_ReturnUserFocusSession();
			
			
			if (nullptr != activeSession)
			{
				TerminalWindowRef	activeTerminalWindow = Session_ReturnActiveTerminalWindow(activeSession);
				
				
				if (nullptr != activeTerminalWindow)
				{
					TerminalViewRef		activeTerminalView = TerminalWindow_ReturnViewWithFocus(activeTerminalWindow);
					
					
					if (nullptr != activeTerminalView)
					{
						if (kEventKind == kEventServiceGetTypes)
						{
							// return the data types that are possible to Copy from this window
							if (TerminalView_TextSelectionExists(activeTerminalView))
							{
								CFMutableArrayRef	copiedTypes = nullptr;
								
								
								// grab the scrap that contains the data on which to perform the Open URL service
								result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamServiceCopyTypes, typeCFMutableArrayRef, copiedTypes);
								
								// if the array was found, proceed
								if (noErr == result)
								{
									CFArrayAppendValue(copiedTypes, CFSTR("TEXT")/* ??? necessary ??? */);
									CFArrayAppendValue(copiedTypes, FUTURE_SYMBOL(CFSTR("public.utf16-external-plain-text"), kUTTypeUTF16ExternalPlainText));
									CFArrayAppendValue(copiedTypes, FUTURE_SYMBOL(CFSTR("public.utf8-plain-text"), kUTTypeUTF8PlainText));
									CFArrayAppendValue(copiedTypes, FUTURE_SYMBOL(CFSTR("public.plain-text"), kUTTypePlainText));
								}
							}
						}
						else if (kEventKind == kEventServiceCopy)
						{
							// copy selected text for the Service to use
							PasteboardRef	destinationPasteboard = nullptr;
							
							
							// grab the pasteboard to which the current selection should be copied
							result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamPasteboardRef, typePasteboardRef, destinationPasteboard);
							
							// if the source pasteboard was found, proceed
							if (noErr == result)
							{
								CFStringRef		selectedText = TerminalView_ReturnSelectedTextAsNewUnicode
																(activeTerminalView, 0/* number of spaces to replace with one tab */, 0/* flags */);
								
								
								if (nullptr != selectedText)
								{
									result = Clipboard_AddCFStringToPasteboard(selectedText, destinationPasteboard);
									CFRelease(selectedText), selectedText = nullptr;
								}
							}
						}
						else
						{
							assert(false && "unexpected Services event kind");
						}
					}
				}
			}
		}
		break;
	
	case kEventServicePerform:
		// handle “Open URL in MacTelnet” event
		{
			PasteboardRef	sourcePasteboard = nullptr;
			
			
			// grab the pasteboard that contains the data on which to perform the Open URL service
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamPasteboardRef, typePasteboardRef, sourcePasteboard);
			
			// if the source pasteboard was found, proceed
			if (noErr == result)
			{
				CFStringRef		urlCFString = nullptr;
				CFStringRef		utiCFString = nullptr;
				
				
				// register the pasteboard
				Clipboard_SetPasteboardModified(sourcePasteboard);
				
				// retrieve text from the given pasteboard and interpret it as a URL
				if (Clipboard_CreateCFStringFromPasteboard(urlCFString, utiCFString, sourcePasteboard))
				{
					// handle the URL!
					std::string		urlString;
					
					
					StringUtilities_CFToUTF8(urlCFString, urlString);
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
					if (nullptr != urlCFString) CFRelease(urlCFString), urlCFString = nullptr;
					CFRelease(utiCFString), utiCFString = nullptr;
				}
			}
		}
		break;
	
	default:
		Console_WriteLine("unknown Services event received!");
		break;
	}
	return result;
}// receiveServicesEvent


#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
/*!
Handles "kEventWindowSheetOpening" of "kEventClassWindow".

Invoked by Mac OS X whenever a sheet is about to open.
This handler could prevent the opening by returning
"userCanceledErr".

(3.1)
*/
pascal OSStatus
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
pascal OSStatus
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
pascal OSStatus
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
