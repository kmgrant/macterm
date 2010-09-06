/*###############################################################

	RecordAE.cp
	
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <MemoryBlocks.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "ConstantsRegistry.h"
#include "RecordAE.h"
#include "Terminology.h"



#pragma mark Variables
namespace {

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

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSErr		handleRecordingBegunEvent			(AppleEvent const*, AppleEvent*, SInt32);
OSErr		handleRecordingTerminatedEvent		(AppleEvent const*, AppleEvent*, SInt32);
OSStatus	startRecording						();
OSStatus	stopRecording						();

} // anonymous namespace



#pragma mark Public Methods

/*!
This method initializes the Apple Event handlers
for the Required Suite, and creates an address
descriptor so MacTelnet can send events to itself
(for recordability).

\retval kRecordAE_ResultOK
if all handlers were installed successfully

\retval kRecordAE_ResultCannotInitialize
if any handler failed to install

(3.0)
*/
RecordAE_Result
RecordAE_Init ()
{
	RecordAE_Result		result = kRecordAE_ResultCannotInitialize;
	OSStatus			error = noErr;
	
	
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
				result = kRecordAE_ResultOK;
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
	
	
	result = AECreateAppleEvent(inEventClass, inEventID, RecordAE_ReturnSelfAddress(),
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
RecordAE_ReturnSelfAddress ()
{
	return &gSelfAddress;
}// ReturnSelfAddress


#pragma mark Internal Methods
namespace {

/*!
Handles event "kAENotifyStartRecording" of "kCoreEventClass".

(3.0)
*/
OSErr
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
OSErr
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
Worker routine for recording-begun handler; exists primarily
so that it can be called without faking an Apple Event.

(3.0)
*/
OSStatus
startRecording ()
{
	OSStatus	result = noErr;
	
	
	// Hmmm...is it possible for multiple recordings to be taking place at once?
	// Assume that it is, and keep track of the number of recordings in progress.
	++gRecordingCount;
	FlagManager_Set(kFlagAppleScriptRecording, true);
	
	return result;
}// startRecording


/*!
Worker routine for recording-terminated handler; exists
primarily so that it can be called without faking an
Apple Event.

(3.0)
*/
OSStatus
stopRecording ()
{
	OSStatus	result = noErr;
	
	
	// Hmmm...is it possible for multiple recordings to be taking place at once?
	// Assume that it is, and keep track of recordings; clear the flag only when
	// all recordings have been terminated.
	--gRecordingCount;
	if (gRecordingCount <= 0) FlagManager_Set(kFlagAppleScriptRecording, false);
	
	return result;
}// stopRecording

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
