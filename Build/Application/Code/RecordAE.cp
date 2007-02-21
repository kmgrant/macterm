/*###############################################################

	RecordAE.cp
	
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <MemoryBlocks.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "BasicTypesAE.h"
#include "ConstantsRegistry.h"
#include "RecordAE.h"
#include "Terminology.h"



#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	AEEventHandlerUPP		gRecordingBeginUPP = nullptr;		//!< wrapper for callback that hears about new recordings
	AEEventHandlerUPP		gRecordingEndUPP = nullptr;			//!< wrapper for callback that hears about halted recordings
	EventHandlerUPP			gWindowClosedUPP = nullptr;			//!< wrapper for callback that hears about windows that have closed
	EventHandlerRef			gWindowClosedHandler = nullptr;
	EventHandlerUPP			gWindowCollapseToggleUPP = nullptr;	//!< wrapper for callback that hears about windows that have closed
	EventHandlerRef			gWindowCollapseToggleHandler = nullptr;
	EventHandlerUPP			gWindowZoomedUPP = nullptr;			//!< wrapper for callback that hears about windows that have closed
	EventHandlerRef			gWindowZoomedHandler = nullptr;
	AEAddressDesc			gSelfAddress;						//!< allows application to be recordable
	ProcessSerialNumber		gSelfProcessID;						//!< identifies application in terms of an OS process
	SInt32					gRecordingCount = 0L;				//!< number of recordings taking place
}

#pragma mark Internal Method Prototypes

static pascal OSErr		handleRecordingBegunEvent				(AppleEvent const*, AppleEvent*, SInt32);
static pascal OSErr		handleRecordingTerminatedEvent			(AppleEvent const*, AppleEvent*, SInt32);
static pascal OSStatus  receiveWindowClosed						(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus  receiveWindowCollapsedOrExpanded		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus  receiveWindowZoomed						(EventHandlerCallRef, EventRef, void*);
static OSStatus			startRecording							();
static OSStatus			stopRecording							();



#pragma mark Public Methods

/*!
This method initializes the Apple Event handlers
for the Required Suite, and creates an address
descriptor so MacTelnet can send events to itself
(for recordability).

\retval kRecordAE_ResultCodeSuccess
if all handlers were installed successfully

\retval kRecordAE_ResultCodeCannotInitialize
if any handler failed to install

(3.0)
*/
RecordAE_ResultCode
RecordAE_Init ()
{
	RecordAE_ResultCode		result = kRecordAE_ResultCodeCannotInitialize;
	OSStatus				error = noErr;
	
	
	// Set up a self-addressed, stamped envelope so MacTelnet can perform all
	// of its operations by making an end run through the system.  This odd
	// but powerful approach allows recording applications, such as the
	// Script Editor, to automatically write scripts based on what users do
	// in MacTelnet!
	gSelfProcessID.highLongOfPSN = 0;
	gSelfProcessID.lowLongOfPSN = kCurrentProcess; // don’t use GetCurrentProcess()!
	error = AECreateDesc(typeProcessSerialNumber, &gSelfProcessID, sizeof(gSelfProcessID), &gSelfAddress);
	if (error == noErr)
	{
		// recording notification event handlers
		gRecordingBeginUPP = NewAEEventHandlerUPP(handleRecordingBegunEvent);
		error = AEInstallEventHandler(kCoreEventClass, kAENotifyStartRecording, gRecordingBeginUPP,
										0L/* context */, false/* is system handler */);
		if (error == noErr)
		{
			gRecordingEndUPP = NewAEEventHandlerUPP(handleRecordingTerminatedEvent);
			error = AEInstallEventHandler(kCoreEventClass, kAENotifyStopRecording, gRecordingEndUPP,
											0L/* context */, false/* is system handler */);
			if (error == noErr)
			{
				// okay!
				result = kRecordAE_ResultCodeSuccess;
			}
		}
	}
	
	// Also, since these events are only sent if a recording begins while
	// the program is running, the proper setup still won’t occur if the
	// user happens to be recording something already when the program
	// starts up.  To detect this case, ask the Apple Event Manager.
	{
		SInt32		countOfRecordingProcesses = 0;
		
		
		error = AEManagerInfo(keyAERecorderCount, &countOfRecordingProcesses);
		if ((error == noErr) && (countOfRecordingProcesses > 0))
		{
			// send a fake recording-begun event
			(OSStatus)startRecording();
		}
	}
	
	return result;
}// Init


/*!
Cleans up this module by removing global event
handlers, etc.

(3.0)
*/
void
RecordAE_Done ()
{
	AERemoveEventHandler(kCoreEventClass, kAENotifyStartRecording, gRecordingBeginUPP, false/* is system handler */);
	DisposeAEEventHandlerUPP(gRecordingBeginUPP), gRecordingBeginUPP = nullptr;
	AERemoveEventHandler(kCoreEventClass, kAENotifyStopRecording, gRecordingEndUPP, false/* is system handler */);
	DisposeAEEventHandlerUPP(gRecordingEndUPP), gRecordingEndUPP = nullptr;
	
	if (gRecordingCount)
	{
		// somehow the recording stuff is still active; explicitly force it to quit
		(OSStatus)stopRecording();
	}
	
	AEDisposeDesc(&gSelfAddress);
}// Done


/*!
Creates an Apple Event using AECreateAppleEvent(), with
the most likely parameters for “send to self” events:
RecordAE_GetSelfAddress(), kAutoGenerateReturnID, and
kAnyTransactionID.

(3.0)
*/
OSStatus
RecordAE_CreateRecordableAppleEvent		(DescType		inEventClass,
										 DescType		inEventID,
										 AppleEvent*	outAppleEventPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateAppleEvent(inEventClass, inEventID, RecordAE_GetSelfAddress(),
								kAutoGenerateReturnID, kAnyTransactionID, outAppleEventPtr);
	return result;
}// CreateRecordableAppleEvent


/*!
Returns the address of the current application, used
for event routing.  The result will not be valid if
you have not used RecordAE_Init().

(3.0)
*/
AEAddressDesc const*
RecordAE_GetSelfAddress ()
{
	return &gSelfAddress;
}// GetSelfAddress


#pragma mark Internal Methods

/*!
Handles event "kAENotifyStartRecording" of "kCoreEventClass".

(3.0)
*/
static pascal OSErr
handleRecordingBegunEvent	(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		UNUSED_ARGUMENT(outReplyAppleEventPtr),
							 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “recording begun” event");
	
	result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
	
	if (result == noErr)
	{
		result = startRecording();
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// handleRecordingBegunEvent


/*!
Handles event "kAENotifyStopRecording" of "kCoreEventClass".

(3.0)
*/
static pascal OSErr
handleRecordingTerminatedEvent	(AppleEvent const*	inAppleEventPtr,
								 AppleEvent*		UNUSED_ARGUMENT(outReplyAppleEventPtr),
								 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “recording terminated” event");
	
	result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
	
	if (result == noErr)
	{
		result = stopRecording();
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// handleRecordingTerminatedEvent


/*!
Embellishes "kEventWindowClosed" of "kEventClassWindow"
for any window, while a script is recording.  A fake
event is sent back that causes an appropriate command
to appear in a recording script.

IMPORTANT:  The “closed” event is handled here, as opposed
			to the “closing” event; this is because many
			handlers tend to override the closing-event
			completely, which would mean this is never
			invoked when it should be.  Technically it is
			slightly more appropriate to record a close
			event the instant a window starts to close,
			but recording it just after a window is hidden
			should be fine too.

(3.0)
*/
static pascal OSStatus
receiveWindowClosed		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent),
					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClosed);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			OSStatus			error = noErr;
			WindowAttributes	attributes = 0L;
			
			
			error = GetWindowAttributes(window, &attributes);
			if ((error == noErr) && (attributes & kWindowCloseBoxAttribute))
			{
				AEDesc		closeEvent,
							replyDescriptor;
				
				
				RecordAE_CreateRecordableAppleEvent(kAECoreSuite, kAEClose, &closeEvent);
				if (error == noErr)
				{
					AEDesc		containerDesc,
								keyDesc,
								objectDesc;
					Str255		windowTitle;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc); // remains a null container
					(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
					(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
					
					// send a request for "window <title>", which resides in a null container;
					// then, issue an event to bring it to the front
					GetWTitle(window, windowTitle);
					(OSStatus)BasicTypesAE_CreatePStringDesc(windowTitle, &keyDesc);
					error = CreateObjSpecifier(cMyWindow, &containerDesc, formName, &keyDesc, true/* dispose inputs */, &objectDesc);
					if (error != noErr) Console_WriteValue("window access error", error);
					if (error == noErr)
					{
						// reference to the window to be selected - REQUIRED
						(OSStatus)AEPutParamDesc(&closeEvent, keyDirectObject, &objectDesc);
						
						// send the event, which will record it into any open script
						(OSStatus)AESend(&closeEvent, &replyDescriptor, kAENoReply | kAEDontExecute,
											kAENormalPriority, kAEDefaultTimeout,
											nullptr/* idle routine */, nullptr/* filter routine */);
					}
				}
				AEDisposeDesc(&closeEvent);
			}
			result = eventNotHandledErr; // IMPORTANT: return "eventNotHandledErr" because this routine only fires no-op events
		}
	}
	
	return result;
}// receiveWindowClosed


/*!
Embellishes "kEventWindowCollapsed" and "kEventWindowExpanded"
of "kEventClassWindow" for any window, while a script is
recording.  A fake event is sent back that causes an
appropriate command to appear in a recording script.

(3.0)
*/
static pascal OSStatus
receiveWindowCollapsedOrExpanded	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef				inEvent,
									 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent),
					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert((kEventKind == kEventWindowCollapsed) || (kEventKind == kEventWindowExpanded));
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			OSStatus			error = noErr;
			WindowAttributes	attributes = 0L;
			
			
			error = GetWindowAttributes(window, &attributes);
			if ((error == noErr) && (attributes & kWindowCollapseBoxAttribute))
			{
				AEDesc		setDataEvent,
							replyDescriptor;
				
				
				RecordAE_CreateRecordableAppleEvent(kAECoreSuite, kAESetData, &setDataEvent);
				if (error == noErr)
				{
					AEDesc		containerDesc,
								keyDesc,
								objectDesc,
								propertyDesc,
								valueDesc;
					Str255		windowTitle;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc); // remains a null container
					(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
					(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
					(OSStatus)AppleEventUtilities_InitAEDesc(&propertyDesc);
					(OSStatus)AppleEventUtilities_InitAEDesc(&valueDesc);
					
					// send a request for "window <title>", which resides in a null container;
					// then, issue an event to minimize it (effectively setting a window property)
					GetWTitle(window, windowTitle);
					(OSStatus)BasicTypesAE_CreatePStringDesc(windowTitle, &keyDesc);
					error = CreateObjSpecifier(cMyWindow, &containerDesc, formName, &keyDesc, true/* dispose inputs */, &objectDesc);
					if (error != noErr) Console_WriteValue("window access error", error);
					(OSStatus)BasicTypesAE_CreatePropertyIDDesc(pMyWindowIsCollapsed, &keyDesc);
					error = CreateObjSpecifier(cProperty, &objectDesc,
												formPropertyID, &keyDesc, true/* dispose inputs */,
												&propertyDesc);
					if (error != noErr) Console_WriteValue("property access error", error);
					if (error == noErr)
					{
						// reference to the window to be minimized or restored - REQUIRED
						(OSStatus)AEPutParamDesc(&setDataEvent, keyDirectObject, &propertyDesc);
						
						// the property’s new value - REQUIRED
						(OSStatus)BasicTypesAE_CreateBooleanDesc((kEventKind == kEventWindowCollapsed), &valueDesc);
						(OSStatus)AEPutParamDesc(&setDataEvent, keyAEParamMyTo, &valueDesc);
						
						// send the event, which will record it into any open script
						(OSStatus)AESend(&setDataEvent, &replyDescriptor, kAENoReply | kAEDontExecute,
											kAENormalPriority, kAEDefaultTimeout,
											nullptr/* idle routine */, nullptr/* filter routine */);
					}
				}
				AEDisposeDesc(&setDataEvent);
			}
			result = eventNotHandledErr; // IMPORTANT: return "eventNotHandledErr" because this routine only fires no-op events
		}
	}
	
	return result;
}// receiveWindowCollapsedOrExpanded


/*!
Embellishes "kEventWindowZoomed"of "kEventClassWindow"
for any window, while a script is recording.  A fake
event is sent back that causes an appropriate command
to appear in a recording script.

(3.0)
*/
static pascal OSStatus
receiveWindowZoomed		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent),
					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowZoomed);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			OSStatus			error = noErr;
			WindowAttributes	attributes = 0L;
			
			
			error = GetWindowAttributes(window, &attributes);
			if ((error == noErr) && ((attributes & kWindowHorizontalZoomAttribute) || (attributes & kWindowVerticalZoomAttribute)))
			{
				AEDesc		setDataEvent,
							replyDescriptor;
				
				
				RecordAE_CreateRecordableAppleEvent(kAECoreSuite, kAESetData, &setDataEvent);
				if (error == noErr)
				{
					AEDesc		containerDesc,
								keyDesc,
								objectDesc,
								propertyDesc,
								valueDesc;
					Str255		windowTitle;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc); // remains a null container
					(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
					(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
					(OSStatus)AppleEventUtilities_InitAEDesc(&propertyDesc);
					(OSStatus)AppleEventUtilities_InitAEDesc(&valueDesc);
					
					// send a request for "window <title>", which resides in a null container;
					// then, issue an event to minimize it (effectively setting a window property)
					GetWTitle(window, windowTitle);
					(OSStatus)BasicTypesAE_CreatePStringDesc(windowTitle, &keyDesc);
					error = CreateObjSpecifier(cMyWindow, &containerDesc, formName, &keyDesc, true/* dispose inputs */, &objectDesc);
					if (error != noErr) Console_WriteValue("window access error", error);
					(OSStatus)BasicTypesAE_CreatePropertyIDDesc(pMyWindowIsZoomed, &keyDesc);
					error = CreateObjSpecifier(cProperty, &objectDesc,
												formPropertyID, &keyDesc, true/* dispose inputs */,
												&propertyDesc);
					if (error != noErr) Console_WriteValue("property access error", error);
					if (error == noErr)
					{
						// reference to the window to be minimized or restored - REQUIRED
						(OSStatus)AEPutParamDesc(&setDataEvent, keyDirectObject, &propertyDesc);
						
						// the property’s new value - REQUIRED
						(OSStatus)BasicTypesAE_CreateBooleanDesc
									(IsWindowInStandardState(window, nullptr/* ideal size */, nullptr/* ideal standard state */),
										&valueDesc);
						(OSStatus)AEPutParamDesc(&setDataEvent, keyAEParamMyTo, &valueDesc);
						
						// send the event, which will record it into any open script
						(OSStatus)AESend(&setDataEvent, &replyDescriptor, kAENoReply | kAEDontExecute,
											kAENormalPriority, kAEDefaultTimeout,
											nullptr/* idle routine */, nullptr/* filter routine */);
					}
				}
				AEDisposeDesc(&setDataEvent);
			}
			result = eventNotHandledErr; // IMPORTANT: return "eventNotHandledErr" because this routine only fires no-op events
		}
	}
	
	return result;
}// receiveWindowZoomed


/*!
Worker routine for recording-begun handler; exists primarily
so that it can be called without faking an Apple Event.

(3.0)
*/
static OSStatus
startRecording ()
{
	OSStatus	result = noErr;
	
	
	// Hmmm...is it possible for multiple recordings to be taking place at once?
	// Assume that it is, and keep track of the number of recordings in progress.
	++gRecordingCount;
	FlagManager_Set(kFlagAppleScriptRecording, true);
	
	//
	// install Carbon Event handlers that fire events to recording scripts
	//
	
	// install a callback that sends events like "close window 1" when a window is closed
	{
		EventTypeSpec const		whenWindowClosed[] =
								{
									{ kEventClassWindow, kEventWindowClosed }
								};
		OSStatus				error = noErr;
		
		
		gWindowClosedUPP = NewEventHandlerUPP(receiveWindowClosed);
		error = InstallApplicationEventHandler(gWindowClosedUPP, GetEventTypeCount(whenWindowClosed),
												whenWindowClosed, nullptr/* user data */,
												&gWindowClosedHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that sends events like "set the minimized of window 1 to true" when a window is minimized
	{
		EventTypeSpec const		whenWindowCollapsedOrExpanded[] =
								{
									{ kEventClassWindow, kEventWindowCollapsed },
									{ kEventClassWindow, kEventWindowExpanded }
								};
		OSStatus				error = noErr;
		
		
		gWindowCollapseToggleUPP = NewEventHandlerUPP(receiveWindowCollapsedOrExpanded);
		error = InstallApplicationEventHandler(gWindowCollapseToggleUPP, GetEventTypeCount(whenWindowCollapsedOrExpanded),
												whenWindowCollapsedOrExpanded, nullptr/* user data */,
												&gWindowCollapseToggleHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that sends events like "set the zoomed of window 1 to true" when a window is zoomed
	{
		EventTypeSpec const		whenWindowZoomed[] =
								{
									{ kEventClassWindow, kEventWindowZoomed }
								};
		OSStatus				error = noErr;
		
		
		gWindowZoomedUPP = NewEventHandlerUPP(receiveWindowZoomed);
		error = InstallApplicationEventHandler(gWindowZoomedUPP, GetEventTypeCount(whenWindowZoomed),
												whenWindowZoomed, nullptr/* user data */,
												&gWindowZoomedHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	return result;
}// startRecording


/*!
Worker routine for recording-terminated handler; exists
primarily so that it can be called without faking an
Apple Event.

(3.0)
*/
static OSStatus
stopRecording ()
{
	OSStatus	result = noErr;
	
	
	// Hmmm...is it possible for multiple recordings to be taking place at once?
	// Assume that it is, and keep track of recordings; clear the flag only when
	// all recordings have been terminated.
	--gRecordingCount;
	if (gRecordingCount <= 0) FlagManager_Set(kFlagAppleScriptRecording, false);
	
	//
	// remove Carbon Event handlers installed in handleRecordingBegunEvent()
	//
	
	RemoveEventHandler(gWindowClosedHandler), gWindowClosedHandler = nullptr;
	DisposeEventHandlerUPP(gWindowClosedUPP), gWindowClosedUPP = nullptr;
	RemoveEventHandler(gWindowCollapseToggleHandler), gWindowCollapseToggleHandler = nullptr;
	DisposeEventHandlerUPP(gWindowCollapseToggleUPP), gWindowCollapseToggleUPP = nullptr;
	RemoveEventHandler(gWindowZoomedHandler), gWindowZoomedHandler = nullptr;
	DisposeEventHandlerUPP(gWindowZoomedUPP), gWindowZoomedUPP = nullptr;
	
	return result;
}// stopRecording

// BELOW IS REQUIRED NEWLINE TO END FILE
