/*!	\file AlertMessages.h
	\brief Greatly simplified and convenient interface to
	alert messages, be they modal dialogs or sheets.
	
	Also, since alerts so frequently tie into background
	notification schemes, this module handles background
	alerts as well (including badging the Dock icon if
	necessary, etc.).
*/
/*###############################################################

	Interface Library 2.0
	© 1998-2007 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

#ifndef __ALERTMESSAGES__
#define __ALERTMESSAGES__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef struct AlertMessages_OpaqueBox*		AlertMessages_BoxRef;
typedef AlertMessages_BoxRef		InterfaceLibAlertRef; // DEPRECATED NAME

enum
{
	// pass one of these to Alert_SetNotificationPreferences()
	kAlert_NotifyDoNothing = 0,
	kAlert_NotifyDisplayDiamondMark = 1,
	kAlert_NotifyDisplayIconAndDiamondMark = 2,
	kAlert_NotifyAlsoDisplayAlert = 3
};

enum
{
	kAlert_StyleOK,
	kAlert_StyleCancel,
	kAlert_StyleOKCancel,
	kAlert_StyleOKCancel_CancelIsDefault,
	kAlert_StyleYesNo,
	kAlert_StyleYesNo_NoIsDefault,
	kAlert_StyleYesNoCancel,
	kAlert_StyleYesNoCancel_NoIsDefault,
	kAlert_StyleYesNoCancel_CancelIsDefault,
	kAlert_StyleDontSaveCancelSave
};

// dialog item indices
enum
{
	kAlert_ItemButton1 = 1,
	kAlert_ItemButton2 = 2,
	kAlert_ItemDialogText = 3,
	kAlert_ItemHelpText = 4,
	kAlert_ItemButton3 = 5,
	kAlert_ItemHelpButton = 6,
	kAlert_ItemTitleText = 7,
	kAlert_ItemBalloons = 8
};

#pragma mark Callbacks

/*!
Alert Close Notification Method

When a window-modal alert is closed (Mac OS X only),
this method is invoked.  Use this to know exactly
when it is safe to call Alert_Dispose().
*/
typedef void (*AlertMessages_CloseNotifyProcPtr)	(AlertMessages_BoxRef	inAlertThatClosed,
													 SInt16					inItemHit,
													 void*					inUserData);
inline void
AlertMessages_InvokeCloseNotifyProc		(AlertMessages_CloseNotifyProcPtr	inUserRoutine,
										 AlertMessages_BoxRef				inAlertThatClosed,
										 SInt16								inItemHit,
										 void*								inUserData)
{
	(*inUserRoutine)(inAlertThatClosed, inItemHit, inUserData);
}



#pragma mark Public Methods

//!\name Module Setup and Teardown
//@{

// CALL THIS ROUTINE ONCE, BEFORE ANY OTHER ALERT ROUTINE
void
	Alert_Init							(SInt16						inApplicationResourceFile);

// CALL THIS ROUTINE AFTER YOU ARE PERMANENTLY DONE WITH ALERTS
void
	Alert_Done							();

//@}

//!\name Global Settings
//@{

void
	Alert_SetIsBackgrounded				(Boolean					inIsApplicationSuspended);

void
	Alert_SetNotificationMessage		(ConstStringPtr				inMessage);

void
	Alert_SetNotificationPreferences	(UInt16						inNotificationPreferences);

void
	Alert_SetUseSpeech					(Boolean					inUseSpeech);

//@}

//!\name Background Notification Handling
//@{

void
	Alert_BackgroundNotification		();

// CALL THIS METHOD WHEN YOUR APPLICATION DETECTS A “RESUME” EVENT
void
	Alert_ServiceNotification			();

//@}

//!\name Creating and Destroying Alert Windows
//@{

AlertMessages_BoxRef
	Alert_New							();

void
	Alert_Dispose						(AlertMessages_BoxRef*		inoutAlert);

//@}

//!\name Displaying Alerts
//@{

OSStatus
	Alert_Display						(AlertMessages_BoxRef		inAlert);

void
	Alert_Message						(CFStringRef				inDialogText,
										 CFStringRef				inHelpText,
										 Boolean					inIsHelpButton);

Boolean
	Alert_ReportOSErr					(OSErr						inErrorCode);

Boolean
	Alert_ReportOSStatus				(OSStatus					inErrorCode);

//@}

//!\name Alert Window Events
//@{

// ONLY WORKS WITH ALERTS THAT ARE CURRENTLY DISPLAYED
void
	Alert_HitItem						(AlertMessages_BoxRef				inAlert,
										 SInt16								inItemIndex);

SInt16
	Alert_ItemHit						(AlertMessages_BoxRef				inAlert);

void
	Alert_MakeModeless					(AlertMessages_BoxRef				inAlert,
										 AlertMessages_CloseNotifyProcPtr	inCloseNotifyProcPtr,
										 void*								inCloseNotifyProcUserData);

void
	Alert_MakeWindowModal				(AlertMessages_BoxRef				inAlert,
										 WindowRef							inParentWindow,
										 Boolean							inIsParentWindowCloseWarning,
										 AlertMessages_CloseNotifyProcPtr	inCloseNotifyProcPtr,
										 void*								inCloseNotifyProcUserData);

void
	Alert_StandardCloseNotifyProc		(AlertMessages_BoxRef				inAlertThatClosed,
										 SInt16								inItemHit,
										 void*								inUserData);

//@}

//!\name Helper Routines to Specify Alert Window Adornments
//@{

void
	Alert_SetButtonText					(AlertMessages_BoxRef		inAlert,
										 SInt16						inWhichButton,
										 CFStringRef				inNewText);

void
	Alert_SetHelpButton					(AlertMessages_BoxRef		inAlert,
										 Boolean					inIsHelpButton);

void
	Alert_SetMovable					(AlertMessages_BoxRef		inAlert,
										 Boolean					inIsMovable);

void
	Alert_SetParamsFor					(AlertMessages_BoxRef		inAlert,
										 SInt16						inAlertStyle);

// DEPRECATED - USE Alert_SetTextCFStrings() INSTEAD
void
	Alert_SetText						(AlertMessages_BoxRef		inAlert,
										 ConstStringPtr				inDialogText,
										 ConstStringPtr				inHelpText);

void
	Alert_SetTextCFStrings				(AlertMessages_BoxRef		inAlert,
										 CFStringRef				inDialogText,
										 CFStringRef				inHelpText);

void
	Alert_SetTitleCFString				(AlertMessages_BoxRef		inAlert,
										 CFStringRef				inNewText);

void
	Alert_SetType						(AlertMessages_BoxRef		inAlert,
										 AlertType					inNewType);

//@}

//!\name Debugging Utilities
//@{

void
	Alert_DebugCheckpoint				(CFStringRef				inDescription);

void
	Alert_DebugSendCStringToSelf		(char const*				inSendWhat,
										 AEEventClass				inEventClass,
										 AEEventID					inEventID);

void
	Alert_DebugSendStringToSelf			(ConstStringPtr				inSendWhat,
										 AEEventClass				inEventClass,
										 AEEventID					inEventID);

void
	Alert_DebugSetEnabled				(Boolean					inIsEnabled);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
