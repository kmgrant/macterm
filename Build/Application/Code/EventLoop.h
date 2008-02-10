/*!	\file EventLoop.h
	\brief Interface to input device and application events.
	
	Use this module to receive notification whenever events
	occur, to ask if events have occurred, or even to cause
	events to occur.  This module is also used to run the
	application event loop.
	
	The idea is simple - if you have code that needs to know
	about events, DO NOT hack the Event Loop module with a set
	of special cases; rather, install listeners for the things
	you need to know about, and let your handlers do the rest
	in a highly localized context.  This greatly minimizes the
	amount of information that the Event Loop needs to know,
	and helps minimize data sharing and the number of APIs
	that may otherwise be necessary to communicate between
	code modules.
	
	The event tracking system is something entirely new to
	MacTelnet 3.0.  Its revolutionary, decentralized event-
	handling scheme is flexible enough to use Classic or Carbon
	Events under the covers, but is still API-compatible with
	Classic code (that doesn’t use CarbonLib), giving Classic
	code the benefits of some Carbon Events-like behaviors and
	APIs.  And yet, compared to Carbon Events this scheme is
	more straightforward to use because it passes around C data
	structures that have everything you need to know about a
	given kind of event (no series of API calls is necessary to
	get at the information).
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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

#ifndef __EVENTLOOP__
#define __EVENTLOOP__

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <ListenerModel.h>

// MacTelnet includes
#include "ConstantsRegistry.h"



#pragma mark Constants

/*!
Possible return values from Event Loop module routines.
*/
typedef OSStatus EventLoop_Result;
enum
{
	kEventLoop_ResultOK							= 0,	//!< no error
	kEventLoop_ResultBooleanListenerRequired	= 1		//!< ListenerModel_NewBooleanListener() was not used to create the
														//!  ListenerModel_ListenerRef for a callback
};

/*!
Events that MacTelnet allows other modules to “listen” for, via EventLoop_StartMonitoringControl().
Control events always supersede window events of the same kind, and if a control event is absorbed
by a control event listener, it is never sent to any listener for the same event on the window (or
to the remaining control event listeners in the queue, for that matter).
*/
enum EventLoop_ControlEvent
{
	kEventLoop_ControlEventClick						= '√Ctl',	//!< mouse is UP in a specific control in an active window
																	//!  (context: EventInfoControlScope_ClickPtr)
	kEventLoop_ControlEventKeyPress						= 'KCtl',	//!< a key has been pressed while a specific control is focused
																	//!  (context: EventInfoControlScope_KeyPressPtr)
	kEventLoop_ControlEventTrack						= '|Ctl'	//!< mouse is DOWN in a specific control in an active window
																	//!  (context: EventInfoControlScope_ClickPtr)
};

/*!
Events that MacTelnet allows other modules to “listen” for, via EventLoop_StartMonitoringWindow().
Window events always supersede global events of the same kind, and if a window event is absorbed
by a window event listener, it is never sent to any listener for the same event globally (or to
the remaining window event listeners in the queue, for that matter).
*/
enum EventLoop_WindowEvent
{
	kEventLoop_WindowEventContentTrack					= '|Win',	//!< mouse is DOWN in a control in an active window, but has not
																	//!  been released (context: EventInfoWindowScopeContentClickPtr)
	kEventLoop_WindowEventKeyPress						= 'KWin',	//!< a key has been pressed in the active window
																	//!  (context: EventInfoWindowScopeKeyPressPtr)
	kEventLoop_WindowEventScrolling						= 'SWin',	//!< a device with scrolling capabilities such as a mouse’s
																	//!  scroll wheel has initiated scrolling either horizontally,
																	//!  vertically, or in both directions, for the active window
																	//!  (context: EventInfoWindowScopeScrollPtr)
	kEventLoop_WindowEventUpdate						= 'UWin'	//!< the specified window needs redrawing (context: 
																	//!  EventInfoWindowScopeUpdatePtr
};

/*!
Events that MacTelnet allows other modules to “listen” for, via EventLoop_StartMonitoring().
All control and window event types are also valid global events, so that you can monitor all
controls or all windows for a certain kind of event.
*/
typedef FourCharCode EventLoop_GlobalEvent;
enum
{
	kEventLoop_GlobalEventNewIterationMainLoop			= 'NxtE',	//!< the event loop has iterated one more time; useful to
																	//!  perform unusual but periodic tasks (context: nullptr)
	kEventLoop_GlobalEventSuspendResume					= 'Swch'	//!< the current process is either about to become the active
																	//!  process, or about to become inactive (context: nullptr)
};

/*!
Key combination that can be mapped to a control such as button.
*/
typedef FourCharCode EventLoop_KeyEquivalent;
enum
{
	kEventLoop_KeyEquivalentDefaultButton				= 'RtEn',	//!< the Return or Enter key (activates default button in a dialog)
	kEventLoop_KeyEquivalentCancelButton				= 'Esc.',	//!< the Escape key or command-period (.) combination (Cancel button)
	kEventLoop_KeyEquivalentFirstLetter					= '1stL',	//!< command + the first letter of the button name
	kEventLoop_KeyEquivalentArbitraryCommandLetter		= 'CK\0\0',	//!< IMPORTANT: *add* this constant to a Unicode character value, to
																	//!  set an arbitrary key equivalent (e.g. to get the command-A
																	//!  sequence, use "kEventLoop_KeyEquivalentArbitraryLetter + 'A'")
	kEventLoop_KeyEquivalentArbitraryCommandVirtualKey	= 'VK\0\0'	//!< IMPORTANT: *add* this constant to a virtual key code value, to
																	//!  set an arbitrary key equivalent (e.g. to get the command-delete
																	//!  sequence, use "kEventLoop_KeyEquivalentArbitraryVirtualKey + 0x33")
};


#pragma mark Public Methods

/*###############################################################
	INITIALIZING AND FINISHING WITH EVENTS
###############################################################*/

EventLoop_Result
	EventLoop_Init								();

void
	EventLoop_Done								();

/*###############################################################
	RUNNING THE EVENT LOOP
###############################################################*/

void
	EventLoop_Run								();

void
	EventLoop_Terminate							();

/*###############################################################
	AUTOMATIC HANDLING OF EVENTS
###############################################################*/

Boolean
	EventLoop_RegisterButtonClickKey			(ControlRef							inButton,
												 EventLoop_KeyEquivalent			inKeyEquivalent);

Boolean
	EventLoop_UnregisterButtonClickKey			(ControlRef							inButton,
												 EventLoop_KeyEquivalent			inKeyEquivalent);

/*###############################################################
	MANUAL HANDLING OF EVENTS
###############################################################*/

// DEPRECATED
EventLoop_Result
	EventLoop_StartMonitoring					(EventLoop_GlobalEvent				inForWhatEvent,
												 ListenerModel_ListenerRef			inListener);

// DEPRECATED
EventLoop_Result
	EventLoop_StartMonitoringControl			(EventLoop_ControlEvent				inForWhatEvent,
												 ControlRef							inForWhichControlOrNullToReceiveEventsForAllControls,
												 ListenerModel_ListenerRef			inListener);

// DEPRECATED
EventLoop_Result
	EventLoop_StartMonitoringWindow				(EventLoop_WindowEvent				inForWhatEvent,
												 WindowRef							inForWhichWindowOrNullToReceiveEventsForAllWindows,
												 ListenerModel_ListenerRef			inListener);

// DEPRECATED
EventLoop_Result
	EventLoop_StopMonitoring					(EventLoop_GlobalEvent				inForWhatEvent,
												 ListenerModel_ListenerRef			inListener);

// DEPRECATED
EventLoop_Result
	EventLoop_StopMonitoringControl				(EventLoop_ControlEvent				inForWhatEvent,
												 ControlRef							inForWhichControlOrNullToStopReceivingEventsForAllControls,
												 ListenerModel_ListenerRef			inListener);

// DEPRECATED
EventLoop_Result
	EventLoop_StopMonitoringWindow				(EventLoop_WindowEvent				inForWhatEvent,
												 WindowRef							inForWhichWindowOrNullToStopReceivingEventsForAllWindows,
												 ListenerModel_ListenerRef			inListener);

/*###############################################################
	RESPONDING TO EVENTS
###############################################################*/

// DEPRECATED
pascal Boolean
	EventLoop_HandleColorPickerUpdate			(EventRecord*						inoutEventPtr);

// DEPRECATED
Boolean
	EventLoop_HandleEvent						(EventRecord*						inoutEventPtr);

// DEPRECATED
pascal void
	EventLoop_HandleNavigationUpdate			(NavEventCallbackMessage			inMessage,
												 NavCBRecPtr						inParameters,
												 NavCallBackUserData				inUserData);

/*###############################################################
	ANNOUNCING EVENT OCCURRENCES FROM EXTERNAL SOURCES
###############################################################*/

// DEPRECATED
void
	EventLoop_HandleDialogDisplayEvent			(DialogRef							inDialog,
												 Boolean							inIsDialogBeingDisplayed);

// DEPRECATED
void
	EventLoop_HandleDialogIdleEvent				(DialogRef							inDialog);

// DEPRECATED
void
	EventLoop_HandleZoomEvent					(WindowRef							inWindow);

// DEPRECATED
void
	EventLoop_SelectBehindDialogWindows			(WindowRef							inWindowToSelect);

// DEPRECATED
void
	EventLoop_SelectOverRealFrontWindow			(WindowRef							inWindowToSelect);

/*###############################################################
	SCANNING THE QUEUE FOR STILL-UNHANDLED EVENTS
###############################################################*/

// DEPRECATED
Boolean
	EventLoop_IsNextCancel						();

// DEPRECATED
Boolean
	EventLoop_IsNextDoubleClick					(Point*								outGlobalMouseLocationPtr);

/*###############################################################
	KEYBOARD STATE INFORMATION
###############################################################*/

Boolean
	EventLoop_IsCapsLockKeyDown					();

Boolean
	EventLoop_IsCommandKeyDown					();

Boolean
	EventLoop_IsOptionKeyDown					();

Boolean
	EventLoop_IsShiftKeyDown					();

// FAVOR EVENT-SPECIFIC KEY STATE INFORMATION IF IT IS AVAILABLE
EventModifiers
	EventLoop_ReturnCurrentModifiers			();

/*###############################################################
	MISCELLANEOUS
###############################################################*/

// DEPRECATED
WindowRef
	EventLoop_ReturnRealFrontNonDialogWindow	();

// DEPRECATED
WindowRef
	EventLoop_ReturnRealFrontWindow				();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
