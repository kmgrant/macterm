/*###############################################################

	Alert.cp
	
	This module has been created to make it possible to build
	more powerful alert windows than even the Mac OS 8 Toolbox
	allows by default.
	
	Interface Library 1.2
	© 1998-2006 by Kevin Grant
	
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <Console.h>
#include <Cursors.h>
#include <Embedding.h>
#include <HIViewWrap.h>
#include <IconManager.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "InterfaceLibAppResources.r"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "UIStrings.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Dialog" and "Sheet" NIBs from the package "AlertMessages.nib".
*/
static HIViewID const		idMyButtonDefault			= { FOUR_CHAR_CODE('Dflt'),		0/* ID */ };
static HIViewID const		idMyButtonCancel			= { FOUR_CHAR_CODE('Canc'),		0/* ID */ };
static HIViewID const		idMyButtonOther				= { FOUR_CHAR_CODE('Othr'),		0/* ID */ };
static HIViewID const		idMyButtonHelp				= { FOUR_CHAR_CODE('Help'),		0/* ID */ };
static HIViewID const		idMyTextTitle				= { FOUR_CHAR_CODE('Titl'),		0/* ID */ };
static HIViewID const		idMyTextMain				= { FOUR_CHAR_CODE('Main'),		0/* ID */ };
static HIViewID const		idMyTextHelp				= { FOUR_CHAR_CODE('Smal'),		0/* ID */ };
static HIViewID const		idMyStopIcon				= { FOUR_CHAR_CODE('Stop'),		0/* ID */ };
static HIViewID const		idMyNoteIcon				= { FOUR_CHAR_CODE('Note'),		0/* ID */ };
static HIViewID const		idMyCautionIcon				= { FOUR_CHAR_CODE('Caut'),		0/* ID */ };
static HIViewID const		idMyApplicationIcon			= { FOUR_CHAR_CODE('AppI'),		0/* ID */ };

#pragma mark Types

struct InterfaceLibAlert
{
public:
	InterfaceLibAlert();
	~InterfaceLibAlert();
	
	AlertStdCFStringAlertParamRec   params;
	AlertType						alertType;
	CFRetainRelease					dialogTextCFString;
	CFRetainRelease					helpTextCFString;
	CFRetainRelease					titleCFString;
	ControlRef						buttonDefault;		//!< rightmost button
	ControlRef						buttonCancel;		//!< no-action alert dismissal button
	ControlRef						buttonOther;		//!< 3rd button of arbitrary size
	ControlRef						buttonHelp;			//!< context-sensitive help display button
	ControlRef						textTitle;			//!< optional large text displaying alert title
	ControlRef						textMain;			//!< required text displaying main message
	ControlRef						textHelp;			//!< optional small text displaying auxiliary info
	ControlRef						mainIcon;			//!< alert icon displayed
	ControlRef						stopIcon;			//!< alert icon from NIB
	ControlRef						noteIcon;			//!< alert icon from NIB
	ControlRef						cautionIcon;		//!< alert icon from NIB
	ControlRef						applicationIcon;	//!< application icon overlay
	SInt16							itemHit;
	Boolean							isSheet;
	Boolean							isSheetForWindowCloseWarning;
	WindowRef						dialogWindow;
	WindowRef						parentWindow;
	EventHandlerRef					buttonHICommandsHandler;
	AlertMessages_CloseNotifyProcPtr	sheetCloseNotifier;
	void*							sheetCloseNotifierUserData;
	InterfaceLibAlertRef			selfRef;
};
typedef InterfaceLibAlert*		InterfaceLibAlertPtr;

typedef MemoryBlockPtrLocker< InterfaceLibAlertRef, InterfaceLibAlert >		AlertPtrLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Boolean				gNotificationIsBackgrounded = false;
	Boolean				gUseSpeech = false;
	Boolean				gIsInited = false;
	Boolean				gDebuggingEnabled = true;
	UInt16				gNotificationPreferences = kAlert_NotifyDisplayDiamondMark;
	short				gApplicationResourceFile = 0;
	Str255				gNotificationMessage;
	NMRecPtr			gNotificationPtr = nullptr;
	EventHandlerUPP		gAlertButtonHICommandUPP = nullptr;
	AlertPtrLocker&		gAlertPtrLocks ()	{ static AlertPtrLocker x; return x; }
}

//
// internal methods
//

static OSStatus					backgroundNotification			();

static OSStatus					badgeApplicationDockTile		();

static CGImageRef				createCGImageForCautionIcon		();

static void						handleItemHit					(InterfaceLibAlertPtr		inPtr,
																 DialogItemIndex			inItemIndex);

static void						newButtonString					(CFStringRef&				outStringStorage,
																 UIStrings_ButtonCFString	inButtonStringType);

static pascal OSStatus			receiveHICommand				(EventHandlerCallRef		inHandlerCallRef,
																 EventRef					inEvent,
																 void*						inInterfaceLibAlertPtr);

static void						setAlertVisibility				(InterfaceLibAlertPtr		inPtr,
																 Boolean					inIsVisible,
																 Boolean					inAnimate);

static OSStatus					standardAlert					(InterfaceLibAlertPtr		inAlert,
																 AlertType					inAlertType,
																 CFStringRef				inDialogText,
																 CFStringRef				inHelpText);



//
// public methods
//
#pragma mark -

/*!
Constructor.  See Alert_New().

(1.1)
*/
InterfaceLibAlert::
InterfaceLibAlert()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
params(),
alertType(kAlertNoteAlert),
dialogTextCFString(),
helpTextCFString(),
titleCFString(),
buttonDefault(nullptr),
buttonCancel(nullptr),
buttonOther(nullptr),
buttonHelp(nullptr),
textTitle(nullptr),
textMain(nullptr),
textHelp(nullptr),
mainIcon(nullptr),
stopIcon(nullptr),
noteIcon(nullptr),
cautionIcon(nullptr),
applicationIcon(nullptr),
itemHit(kAlertStdAlertOtherButton),
isSheet(false),
isSheetForWindowCloseWarning(false),
dialogWindow(nullptr),
parentWindow(nullptr),
buttonHICommandsHandler(nullptr),
sheetCloseNotifier(nullptr),
sheetCloseNotifierUserData(nullptr),
selfRef(REINTERPRET_CAST(this, InterfaceLibAlertRef))
{
	assert_noerr(GetStandardAlertDefaultParams(&this->params, kStdCFStringAlertVersionOne));
	this->params.movable = true;
}// InterfaceLibAlert default constructor


/*!
Destructor.  See Alert_Dispose().

(1.1)
*/
InterfaceLibAlert::
~InterfaceLibAlert ()
{
	// clean up
	if (nullptr != this->buttonHICommandsHandler)
	{
		RemoveEventHandler(this->buttonHICommandsHandler), this->buttonHICommandsHandler = nullptr;
	}
	if (nullptr != this->dialogWindow) DisposeWindow(this->dialogWindow), this->dialogWindow = nullptr;
}// InterfaceLibAlert destructor


/*!
This routine should be called once, before any
other Alert routine.  The corresponding method
Alert_Done() should be invoked once at the end
of the program (most likely), after the Alert
module is not needed anymore.

The Alert module needs the reference number of
your applicationÕs resource file.  You usually
call Alert_Init() at startup time.

(1.0)
*/
void
Alert_Init	(short		inApplicationResourceFile)
{
	gApplicationResourceFile = inApplicationResourceFile;
	gAlertButtonHICommandUPP = NewEventHandlerUPP(receiveHICommand);
	gIsInited = true;
}// Init


/*!
This routine should be called once, after the
Alert module is no longer needed.

(1.0)
*/
void
Alert_Done ()
{
	Alert_ServiceNotification();
	gAlertPtrLocks().clear();
	DisposeEventHandlerUPP(gAlertButtonHICommandUPP), gAlertButtonHICommandUPP = nullptr;
	gIsInited = false;
}// Done


/*!
Creates a new alert window, but does not display it
(this gives you the opportunity to specify its
characteristics using other methods from this
module).  When you are finished with the alert, use
the method Alert_Dispose() to destroy it.

Generally, you call this routine to create an alert,
then call other methods to set up the attributes of
the new alert (title, help button, dialog filter
procedure, etc.).  When the alert is configured, use
Alert_Display() or a similar method to display it.

By default, all new alerts are marked as movable,
with no help button, with a single OK button and no
other buttons.

IMPORTANT:	If unsuccessful, nullptr is returned.

(1.0)
*/
InterfaceLibAlertRef
Alert_New ()
{
	InterfaceLibAlertRef	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new InterfaceLibAlert, InterfaceLibAlertRef);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
When you are finished with an alert that you
created with Alert_New(), call this routine
to destroy it.  The reference is automatically
set to nullptr.

(1.0)
*/
void
Alert_Dispose	(InterfaceLibAlertRef*		inoutAlert)
{
	if (nullptr != inoutAlert)
	{
		InterfaceLibAlertPtr	ptr = gAlertPtrLocks().acquireLock(*inoutAlert);
		
		
		delete ptr;
		gAlertPtrLocks().releaseLock(*inoutAlert, &ptr);
		*inoutAlert = nullptr;
	}
}// Dispose


/*!
Posts a general notification to the queue, with
no particular message.  So this grabs the userÕs
attention for something other than an alert
message.  The exact form of notification depends
on the notification preferences specified via
Alert_SetNotificationPreferences().

(1.1)
*/
void
Alert_BackgroundNotification ()
{
	(OSStatus)backgroundNotification();
}// BackgroundNotification


/*!
Displays a note alert with an OK button, the message
"Checkpoint" and the given descriptive text.  This
method will alter alert settings, so subsequently
displayed alerts will default to having no help
button and a single OK button.

(1.0)
*/
void
Alert_DebugCheckpoint	(CFStringRef	inDescription)
{
	if (gDebuggingEnabled)
	{
		InterfaceLibAlertRef	box = nullptr;
		
		
		box = Alert_New(); // no help button, single OK button
		Alert_SetTitleCFString(box, CFSTR("Debug Checkpoint"));
		Alert_SetTextCFStrings(box, CFSTR("Checkpoint"), inDescription);
		Alert_SetType(box, kAlertNoteAlert);
		Alert_Display(box);
		Alert_Dispose(&box);
	}
}// DebugCheckpoint


/*!
Sends an Apple Event to the current process of
the specified class, setting a "keyDirectObject"
to "typeChar" data containing the information in
the given Pascal string.  This is useful if your
application supports AppleScript and can put this
string somewhere useful for debugging purposes
(such as in a log file).  It is also useful for
debugging shared libraries, since they cannot
otherwise send information to your applicationÕs
debugging systems.

This isnÕt really an alert routine, but it has
similar applications to alert debugging routines,
so it is listed here.

(1.2)
*/
void
Alert_DebugSendCStringToSelf	(char const*		inSendWhat,
								 AEEventClass		inEventClass,
								 AEEventID			inEventID)
{
	if (gDebuggingEnabled)
	{
		AppleEvent				consoleWriteEvent,
								reply;
		AEAddressDesc			currentProcessAddress;
		ProcessSerialNumber		currentProcessID;
		
		
		currentProcessID.highLongOfPSN = 0;
		currentProcessID.lowLongOfPSN = kCurrentProcess; // donÕt use GetCurrentProcess()!
		(OSStatus)AECreateDesc(typeProcessSerialNumber,
								&currentProcessID, sizeof(ProcessSerialNumber), &currentProcessAddress);
		(OSStatus)AECreateAppleEvent(inEventClass, inEventID, &currentProcessAddress, kAutoGenerateReturnID, kAnyTransactionID, &consoleWriteEvent);
		(OSStatus)AEPutParamPtr(&consoleWriteEvent, keyDirectObject, cString, inSendWhat,
								STATIC_CAST(CPP_STD::strlen(inSendWhat) * sizeof(char), Size));
		(OSStatus)AESend(&consoleWriteEvent, &reply, kAENoReply | kAENeverInteract | kAEDontRecord,
						kAENormalPriority, kAEDefaultTimeout, nullptr, nullptr);
		AEDisposeDesc(&consoleWriteEvent);
	}
}// DebugSendCStringToSelf


/*!
Sends an Apple Event to the current process of
the specified class, setting a "keyDirectObject"
to "typeChar" data containing the information in
the given Pascal string.  This is useful if your
application supports AppleScript and can put this
string somewhere useful for debugging purposes
(such as in a log file).  It is also useful for
debugging shared libraries, since they cannot
otherwise send information to your applicationÕs
debugging systems.

This isnÕt really an alert routine, but it has
similar applications to alert debugging routines,
so it is listed here.

(1.0)
*/
void
Alert_DebugSendStringToSelf		(ConstStringPtr		inSendWhat,
								 AEEventClass		inEventClass,
								 AEEventID			inEventID)
{
	if (gDebuggingEnabled)
	{
		AppleEvent				consoleWriteEvent,
								reply;
		AEAddressDesc			currentProcessAddress;
		ProcessSerialNumber		currentProcessID;
		
		
		currentProcessID.highLongOfPSN = 0;
		currentProcessID.lowLongOfPSN = kCurrentProcess; // donÕt use GetCurrentProcess()!
		(OSStatus)AECreateDesc(typeProcessSerialNumber,
								&currentProcessID, sizeof(ProcessSerialNumber), &currentProcessAddress);
		(OSStatus)AECreateAppleEvent(inEventClass, inEventID, &currentProcessAddress, kAutoGenerateReturnID, kAnyTransactionID, &consoleWriteEvent);
		(OSStatus)AEPutParamPtr(&consoleWriteEvent, keyDirectObject, cString, inSendWhat + 1,
								STATIC_CAST(PLstrlen(inSendWhat) * sizeof(UInt8), Size));
		(OSStatus)AESend(&consoleWriteEvent, &reply, kAENoReply | kAENeverInteract | kAEDontRecord,
						kAENormalPriority, kAEDefaultTimeout, nullptr, nullptr);
		AEDisposeDesc(&consoleWriteEvent);
	}
}// DebugSendStringToSelf


/*!
Turns on or off the debugging routines, such
as Alert_DebugCheckpoint().   Use this to
instantly disable the effects of all calls
without having to remove the calls from your
code.

(1.0)
*/
void
Alert_DebugSetEnabled	(Boolean	inEnabled)
{
	gDebuggingEnabled = inEnabled;
}// DebugSetEnabled


/*!
Use this method to display an alert described by an
Interface Library Alert object.  This will have one of
two effects:
- For application-modal alerts, the call will block
  until the alert is dismissed (either by user action
  or timeout).  If the alert has more than one button,
  the item hit can be found as a standard alert button
  number (kAlertStdAlertOKButton, etc.) by invoking
  the Alert_ItemHit() method.
- For window-modal alerts (Mac OS X only), the sheet
  will be displayed and then this routine will return
  immediately.  When the alert is finally dismissed,
  the notification routine specified by
  Alert_MakeWindowModal() will be invoked, at which
  time you find out which button (if any) was chosen.
  Only at that point is it safe to dispose of the
  alert with Alert_Dispose().

If the program is in the background (as specified by
a call to Alert_SetIsBackgrounded()), a notification
is posted.  The alert is displayed immediately, but
the notification lets the user know that itÕs there.
Your application must call Alert_ServiceNotification()
whenever a ÒresumeÓ event is detected in your event
loop, in order to service notifications posted by the
Alert module.  The Alert module assumes that it will
find a small icon ('SICN') resource with an ID number
of "kAlert_NotificationSICNResourceID" that it can use
as a Notification Manager icon in the process menu.

Another Alert module alert box can be visible while
this method is invoked, and all displayed alerts will
still be displayed properly.  This is better than the
Mac OS default implementation - a bug in the
StandardAlert() implementation prevents simultaneous
display of alerts, because it causes all displayed
alerts to show the same alert text.

You must have called Alert_Init() properly to display
any kind of alert.  If this module has not been
initialized, "notInitErr" is returned.

(1.0)
*/
OSStatus
Alert_Display	(InterfaceLibAlertRef	inAlert)
{
	OSStatus	result = noErr;
	
	
	// only attempt to display an alert if the module was properly initialized
	if (gIsInited)
	{
		InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
		
		
		result = backgroundNotification();
		Embedding_DeactivateFrontmostWindow();
		Cursors_UseArrow();
	#if 1
		result = standardAlert(alertPtr, alertPtr->alertType, alertPtr->dialogTextCFString.returnCFStringRef(),
								alertPtr->helpTextCFString.returnCFStringRef());
	#else
		//(OSStatus)CreateStandardAlert(alertPtr->alertType, alertPtr->dialogTextCFString.returnCFStringRef(),
		//								alertPtr->helpTextCFString.returnCFStringRef(),
		//								&alertPtr->params, &alertPtr->itemHit, &someDialog);
	#endif
		Embedding_RestoreFrontmostWindow();
		gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
	}
	else
	{
		result = notInitErr;
	}
	
	return result;
}// Display


/*!
Hits an item in an open alert window as if the user
hit the item.  Generally used in open sheets to force
behavior (for example, forcing a sheet to Cancel and
slide closed).

(1.0)
*/
void
Alert_HitItem	(InterfaceLibAlertRef	inAlert,
				 SInt16					inItemIndex)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	handleItemHit(alertPtr, inItemIndex);
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// HitItem


/*!
Determines the number of the button that was selected
by the user.  A value of 1, 2, or 3 indicates the
respective button positions for the default button,
cancel button, and ÒotherÓ button.  The value 4
indicates the help button.  If the alert times out
(that is, it Òclosed itselfÓ), 0 is returned.

(1.0)
*/
SInt16
Alert_ItemHit	(InterfaceLibAlertRef	inAlert)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	SInt16					result = kAlertStdAlertOtherButton;
	
	
	if (nullptr != alertPtr) result = alertPtr->itemHit;
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
	return result;
}// ItemHit


/*!
Turns the specified alert into a translucent sheet,
as opposed to a movable modal dialog box.

The "inIsParentWindowCloseWarning" option specifies
how the sheet is closed; the Aqua Human Interface
Guidelines suggest removing a sheet immediately
without animation (along with its parent window) if
the default action button for the sheet causes the
parent window to close.

Since sheets are essentially modeless, when you
subsequently display the alert you will actually
have to defer handling of the alert until the
notifier you specify is invoked.  For alerts with
a single button for which you donÕt care about the
result, just use "Alert_StandardCloseNotifyProc"
as your notifier; for all other kinds of alerts,
pass your own custom routine so you can tell which
button was hit.

(1.0)
*/
void
Alert_MakeWindowModal	(InterfaceLibAlertRef				inAlert,
						 WindowRef							inParentWindow,
						 Boolean							inIsParentWindowCloseWarning,
						 AlertMessages_CloseNotifyProcPtr	inCloseNotifyProcPtr,
						 void*								inCloseNotifyProcUserData)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->isSheet = true;
		alertPtr->isSheetForWindowCloseWarning = inIsParentWindowCloseWarning;
		alertPtr->parentWindow = inParentWindow;
		alertPtr->sheetCloseNotifier = inCloseNotifyProcPtr;
		alertPtr->sheetCloseNotifierUserData = inCloseNotifyProcUserData;
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// MakeWindowModal


/*!
Displays a note alert with an OK button, the given
message and help text, and optionally a help button.

(1.0)
*/
void
Alert_Message	(CFStringRef	inDialogText,
				 CFStringRef	inHelpText,
				 Boolean		inIsHelpButton)
{
	InterfaceLibAlertRef	box = nullptr;
	
	
	box = Alert_New(); // no help button, single OK button
	Alert_SetHelpButton(box, inIsHelpButton);
	Alert_SetTextCFStrings(box, inDialogText, inHelpText);
	Alert_SetType(box, kAlertNoteAlert);
	Alert_Display(box);
	Alert_Dispose(&box);
}// Message


/*!
Automatically reports 16-bit error results.  If the
specified error is either "noErr" or "userCanceledErr",
no alert is displayed and false is returned; otherwise,
true is returned to indicate a serious error.  The
alert offers the user the option to quit the program or
continue.  If the user chooses to quit, ExitToShell()
is invoked.

(1.0)
*/
Boolean
Alert_ReportOSErr	(OSErr	inErrorCode)
{
	OSStatus		error = 0L;
	
	
	error = inErrorCode;
	return Alert_ReportOSStatus(error);
}// ReportOSErr


/*!
Automatically reports 16-bit or 32-bit error result
codes.  If the specified error is either "noErr" or
"userCanceledErr", no alert is displayed and false is
returned; otherwise, true is returned to indicate a
serious error.  The alert offers the user the option
to quit the program or continue.  If the user chooses
to quit, ExitToShell() is invoked.

(1.0)
*/
Boolean
Alert_ReportOSStatus	(OSStatus	inErrorCode)
{
	Boolean			result = true;
	
	
	if ((inErrorCode == userCanceledErr) || (inErrorCode == noErr)) result = false;
	
	if (result)
	{
		UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
		InterfaceLibAlertRef	box = nullptr;
		CFStringRef				primaryTextCFString = nullptr;
		CFStringRef				primaryTextTemplateCFString = nullptr;
		CFStringRef				helpTextCFString = nullptr;
		Boolean					doExit = false;
		
		
		// generate dialog text
		stringResult = UIStrings_Copy(kUIStrings_AlertWindowCommandFailedPrimaryText, primaryTextTemplateCFString);
		if (stringResult == kUIStrings_ResultCodeSuccess)
		{
			primaryTextCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
															primaryTextTemplateCFString, inErrorCode);
		}
		else
		{
			primaryTextCFString = CFSTR("");
		}
		
		// generate help text
		stringResult = UIStrings_Copy(kUIStrings_AlertWindowCommandFailedHelpText, helpTextCFString);
		if (false == stringResult.ok())
		{
			helpTextCFString = CFSTR("");
		}
		
		box = Alert_New();
		Alert_SetHelpButton(box, false);
		Alert_SetParamsFor(box, kAlert_StyleOKCancel);
		{
			CFStringRef		quitButtonText = nullptr;
			CFStringRef		continueButtonText = nullptr;
			
			
			newButtonString(quitButtonText, kUIStrings_ButtonQuit);
			if (nullptr != quitButtonText)
			{
				Alert_SetButtonText(box, kAlertStdAlertOKButton, quitButtonText);
				CFRelease(quitButtonText), quitButtonText = nullptr;
			}
			newButtonString(continueButtonText, kUIStrings_ButtonContinue);
			if (nullptr != continueButtonText)
			{
				Alert_SetButtonText(box, kAlertStdAlertCancelButton, continueButtonText);
				CFRelease(continueButtonText), continueButtonText = nullptr;
			}
		}
		Alert_SetTextCFStrings(box, primaryTextCFString, helpTextCFString);
		Alert_SetType(box, kAlertStopAlert);
		Alert_Display(box);
		doExit = (Alert_ItemHit(box) == kAlertStdAlertOKButton);
		Alert_Dispose(&box);
		if (doExit) ExitToShell();
	}
	return result;
}// ReportOSStatus


/*!
Call this routine when a resume event occurs
to see if any notifications need servicing
(usually displays one or more alert windows
that were deferred because the program was
backgrounded).

(1.0)
*/
void
Alert_ServiceNotification ()
{
	if (nullptr != gNotificationPtr)
	{
		RestoreApplicationDockTileImage(); // remove any caution icon badge
		(OSStatus)NMRemove(gNotificationPtr);
		if (nullptr != gNotificationPtr->nmIcon)
		{
			ReleaseResource(gNotificationPtr->nmIcon), gNotificationPtr->nmIcon = nullptr;
		}
		Memory_DisposePtr(REINTERPRET_CAST(&gNotificationPtr, Ptr*));
	}
}// ServiceNotification


/*!
Sets the label of a specific button in subsequently-
displayed alerts.  This information could be overridden
by alert styles, so you should always use
Alert_SetParamsFor() before calling this method.  The
constant that identifies the button text to change is
one of the standard alert constants below:

	kAlertStdAlertOKButton
	kAlertStdAlertCancelButton
	kAlertStdAlertOtherButton

In addition, you are allowed to specify for the button
text one of the following (as opposed to a string):

	kAlertDefaultOKText, for kAlertStdAlertOKButton
	kAlertDefaultCancelText, for kAlertStdAlertCancelButton
	kAlertDefaultOtherText, for kAlertStdAlertOtherButton

You may also pass nullptr to clear the button entirely, in
which case it is not displayed in the alert box.

The string is retained, so you do not have to be concerned
with your alert being affected if your string reference
goes away.

(1.0)
*/
void
Alert_SetButtonText		(InterfaceLibAlertRef	inAlert,
						 short					inWhichButton,
						 CFStringRef			inNewText)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		CFStringRef*	stringPtr = nullptr;
		
		
		// if given an actual string, make sure the reference remains valid indefinitely
		unless ((nullptr == inNewText) || (inNewText == REINTERPRET_CAST(-1/* kAlertDefaultOtherText, etc. */, CFStringRef)))
		{
			CFRetain(inNewText);
		}
		
		// figure out where to store the button string
		switch (inWhichButton)
		{
		case kAlertStdAlertCancelButton:
			stringPtr = &alertPtr->params.cancelText;
			break;
		
		case kAlertStdAlertOtherButton:
			stringPtr = &alertPtr->params.otherText;
			break;
		
		case kAlertStdAlertOKButton:
		default:
			stringPtr = &alertPtr->params.defaultText;
			break;
		}
		assert(nullptr != stringPtr);
		
		// free any previously-stored reference, and store the new one
		unless ((nullptr == *stringPtr) || (*stringPtr == REINTERPRET_CAST(-1/* kAlertDefaultOtherText, etc. */, CFStringRef)))
		{
			CFRelease(*stringPtr);
		}
		*stringPtr = inNewText;
		
		gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
	}
}// SetButtonText


/*!
Specifies whether a help button appears in an alert.
Help buttons should not appear unless help is
available!  If the Contexts library or Apple Guide
itself cannot be found, this method will not accept
a parameter value of true.

(1.0)
*/
void
Alert_SetHelpButton		(InterfaceLibAlertRef	inAlert,
						 Boolean				inIsHelpButton)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->params.helpButton = inIsHelpButton;
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// SetHelpButton


/*!
Use this method whenever your application encounters
a switch event, so that the Alert module knows whether
the application is in the background (suspended) or
not.  This information is used to determine if a
notification should be posted when any attempt is made
to display an alert.

(1.0)
*/
void
Alert_SetIsBackgrounded		(Boolean	inIsApplicationSuspended)
{
	gNotificationIsBackgrounded = inIsApplicationSuspended;
}// SetIsBackgrounded


/*!
Specifies whether an alert box is movable.  Apple
Human Interface Guidelines strongly suggest movable
alerts, and there is no real reason not to use them;
however, this option is available.

(1.0)
*/
void
Alert_SetMovable	(InterfaceLibAlertRef	inAlert,
					 Boolean				inIsMovable)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->params.movable = inIsMovable;
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// SetMovable


/*!
If you have used Alert_SetNotificationPreferences()
to specify that a message be displayed in the event
of a background alert, you can use this method to
specify what that message should say.

(1.0)
*/
void
Alert_SetNotificationMessage	(ConstStringPtr		inMessage)
{
	PLstrcpy(gNotificationMessage, inMessage);
}// SetNotificationMessage


/*!
Use this method to determine what (if any) background
notification steps the Alert module will take if an
attempt is made to display an alert while the program
is in the background.  The simplest preference is to
simply display the alert normally, and let the user
discover its presence on his or her own.  The next
level is to display a diamond mark in the process
menu that identifies the presence of an alert box in
the application.  Another option is to also flash an
icon (of ID "kAlert_NotificationSICNResourceID" and
type 'SICN', in the application resource file) in the
process menuÕs title area.  The last level is to do
all of the above things, and also display a message
(specified with Alert_SetNotificationMessage()) that
tells the user your application requires his or her
attention.

(1.0)
*/
void
Alert_SetNotificationPreferences	(UInt16		inNotificationPreferences)
{
	gNotificationPreferences = inNotificationPreferences;
}// SetNotificationPreferences


/*!
Automatically configures the button text, number of
buttons and default button of the specified alert
based on a specific style.  This method should be
called early in the alert setup procedure, since it
has the potential to take precedence over changes
made by other alert routines (such as
Alert_SetButtonText()).

(1.0)
*/
void
Alert_SetParamsFor	(InterfaceLibAlertRef	inAlert,
					 short					inAlertStyle)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	CFStringRef				tempCFString = nullptr;
	
	
	if (nullptr != alertPtr)
	{
		switch (inAlertStyle)
		{
		case kAlert_StyleOK:
			alertPtr->params.defaultText = REINTERPRET_CAST(kAlertDefaultOKText, CFStringRef);
			Alert_SetButtonText(inAlert, kAlertStdAlertOKButton, nullptr);
			Alert_SetButtonText(inAlert, kAlertStdAlertCancelButton, nullptr);
			Alert_SetButtonText(inAlert, kAlertStdAlertOtherButton, nullptr);
			alertPtr->params.defaultButton = kAlertStdAlertOKButton;
			alertPtr->params.cancelButton = 0;
			break;
		
		case kAlert_StyleCancel:
			newButtonString(tempCFString, kUIStrings_ButtonCancel);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertOKButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			Alert_SetButtonText(inAlert, kAlertStdAlertCancelButton, nullptr);
			Alert_SetButtonText(inAlert, kAlertStdAlertOtherButton, nullptr);
			alertPtr->params.defaultButton = kAlertStdAlertOKButton;
			alertPtr->params.cancelButton = 0;
			break;
		
		case kAlert_StyleOKCancel:
		case kAlert_StyleOKCancel_CancelIsDefault:
			Alert_SetButtonText(inAlert, kAlertStdAlertOKButton, REINTERPRET_CAST(kAlertDefaultOKText, CFStringRef));
			Alert_SetButtonText(inAlert, kAlertStdAlertCancelButton, REINTERPRET_CAST(kAlertDefaultCancelText, CFStringRef));
			Alert_SetButtonText(inAlert, kAlertStdAlertOtherButton, nullptr);
			if (inAlertStyle == kAlert_StyleOKCancel_CancelIsDefault)
			{
				alertPtr->params.defaultButton = kAlertStdAlertCancelButton;
				alertPtr->params.cancelButton = kAlertStdAlertOKButton;
			}
			else
			{
				alertPtr->params.defaultButton = kAlertStdAlertOKButton;
				alertPtr->params.cancelButton = kAlertStdAlertCancelButton;
			}
			break;
		
		case kAlert_StyleYesNo:
		case kAlert_StyleYesNo_NoIsDefault:
			newButtonString(tempCFString, kUIStrings_ButtonYes);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertOKButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			newButtonString(tempCFString, kUIStrings_ButtonNo);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertCancelButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			Alert_SetButtonText(inAlert, kAlertStdAlertOtherButton, nullptr);
			if (inAlertStyle == kAlert_StyleYesNo_NoIsDefault)
			{
				alertPtr->params.defaultButton = kAlertStdAlertCancelButton;
				alertPtr->params.cancelButton = kAlertStdAlertOKButton;
			}
			else
			{
				alertPtr->params.defaultButton = kAlertStdAlertOKButton;
				alertPtr->params.cancelButton = kAlertStdAlertCancelButton;
			}
			break;
		
		case kAlert_StyleYesNoCancel:
		case kAlert_StyleYesNoCancel_NoIsDefault:
		case kAlert_StyleYesNoCancel_CancelIsDefault:
			newButtonString(tempCFString, kUIStrings_ButtonYes);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertOKButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			newButtonString(tempCFString, kUIStrings_ButtonNo);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertCancelButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			newButtonString(tempCFString, kUIStrings_ButtonCancel);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertOtherButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			alertPtr->params.defaultButton = kAlertStdAlertOKButton;
			alertPtr->params.cancelButton = kAlertStdAlertOtherButton;
			if (inAlertStyle == kAlert_StyleYesNoCancel_NoIsDefault)
			{
				alertPtr->params.defaultButton = kAlertStdAlertCancelButton;
			}
			else if (inAlertStyle == kAlert_StyleYesNoCancel_CancelIsDefault)
			{
				alertPtr->params.defaultButton = kAlertStdAlertOtherButton;
			}
			else
			{
				alertPtr->params.defaultButton = kAlertStdAlertOKButton;
			}
			alertPtr->params.cancelButton = kAlertStdAlertOtherButton;
			break;
		
		case kAlert_StyleDontSaveCancelSave:
			newButtonString(tempCFString, kUIStrings_ButtonSave);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertOKButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			Alert_SetButtonText(inAlert, kAlertStdAlertCancelButton, REINTERPRET_CAST(kAlertDefaultCancelText, CFStringRef));
			newButtonString(tempCFString, kUIStrings_ButtonDontSave);
			if (nullptr != tempCFString)
			{
				Alert_SetButtonText(inAlert, kAlertStdAlertOtherButton, tempCFString);
				CFRelease(tempCFString), tempCFString = nullptr;
			}
			alertPtr->params.defaultButton = kAlertStdAlertOKButton;
			alertPtr->params.cancelButton = kAlertStdAlertCancelButton;
			break;
		
		default:
			break;
		}
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// SetParamsFor


/*!
Sets the text message (and optional help text underneath)
for an alert.  If you do not want a text to appear (no
help text, for example), pass an empty string.

Deprecated.  Use Alert_SetTextCFStrings() instead.

(1.0)
*/
void
Alert_SetText	(InterfaceLibAlertRef	inAlert,
				 ConstStringPtr			inDialogText,
				 ConstStringPtr			inHelpText)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->dialogTextCFString.setCFTypeRef(CFStringCreateWithPascalString
													(kCFAllocatorDefault, inDialogText, kCFStringEncodingMacRoman));
		alertPtr->helpTextCFString.setCFTypeRef(CFStringCreateWithPascalString
												(kCFAllocatorDefault, inHelpText, kCFStringEncodingMacRoman));
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// SetText


/*!
Sets the text message (and optional help text underneath)
for an alert.  If you do not want a text to appear (no
help text, for example), pass an empty string.

(1.1)
*/
void
Alert_SetTextCFStrings	(InterfaceLibAlertRef	inAlert,
						 CFStringRef			inDialogText,
						 CFStringRef			inHelpText)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->dialogTextCFString.setCFTypeRef(inDialogText);
		alertPtr->helpTextCFString.setCFTypeRef(inHelpText);
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// SetTextCFStrings


/*!
Sets the title of an alertÕs window.  The title may
not show up unless an alert is also made movable.
To remove an alertÕs title, pass nullptr.

The given string is retained automatically and
released when the alert is destroyed or another
title is set.

(1.1)
*/
void
Alert_SetTitleCFString	(InterfaceLibAlertRef	inAlert,
						 CFStringRef			inNewTitle)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->titleCFString.setCFTypeRef(inNewTitle);
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// SetTitle


/*!
Use this method to set the icon for an alert.
A ÒplainÓ alert type has no icon.

(1.0)
*/
void
Alert_SetType	(InterfaceLibAlertRef		inAlert,
				 AlertType					inNewType)
{
	InterfaceLibAlertPtr	alertPtr = gAlertPtrLocks().acquireLock(inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->alertType = inNewType;
	}
	gAlertPtrLocks().releaseLock(inAlert, &alertPtr);
}// SetType


/*!
Use this method to specify whether the alert
text of subsequently-displayed alerts should
be spoken.

(1.0)
*/
void
Alert_SetUseSpeech		(Boolean	inUseSpeech)
{
	gUseSpeech = inUseSpeech;
}// SetUseSpeech


/*!
The standard responder to a closed window-modal
(sheet) alert, this routine simply destroys the
specified alert, ignoring any buttons clicked.

This notifier is only appropriate if you display
an alert with a single button choice.  If you
need to know which button in a sheet was chosen,
call Alert_MakeWindowModal() with your own
notifier instead of this one, remembering to
call Alert_Dispose() within your custom notifier
(or, just call this routine).

(1.0)
*/
void
Alert_StandardCloseNotifyProc	(InterfaceLibAlertRef	inAlertThatClosed,
								 SInt16					UNUSED_ARGUMENT(inItemHit),
								 void*					UNUSED_ARGUMENT(inUserData))
{
	Alert_Dispose(&inAlertThatClosed);
}// StandardCloseNotifyProc


//
// internal methods
//
#pragma mark -

/*!
Posts a general notification to the queue, with
no particular message.  So this grabs the userÕs
attention for something other than an alert
message.  The exact form of notification depends
on the notification preferences specified via
Alert_SetNotificationPreferences().

(1.1)
*/
static OSStatus
backgroundNotification ()
{
	OSStatus	result = noErr;
	
	
	if ((gNotificationIsBackgrounded) && (nullptr == gNotificationPtr))
	{
		gNotificationPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(NMRec)), NMRecPtr);
		if (nullptr == gNotificationPtr) result = memFullErr;
		else
		{
			short		oldResourceFile = CurResFile();
			
			
			gNotificationPtr->qLink = nullptr;
			gNotificationPtr->qType = nmType;
			gNotificationPtr->nmMark = 0; // index of menu item to mark
			UseResFile(gApplicationResourceFile);
			gNotificationPtr->nmIcon = (gNotificationPreferences < kAlert_NotifyDisplayIconAndDiamondMark)
											? nullptr
											: GetResource('SICN', kAlert_NotificationSICNResourceID);
			gNotificationPtr->nmSound = nullptr;
			if (gNotificationPreferences < kAlert_NotifyAlsoDisplayAlert)
			{
				gNotificationPtr->nmStr = nullptr;
			}
			else
			{
				gNotificationPtr->nmStr = gNotificationMessage;
			}
			gNotificationPtr->nmResp = nullptr;
			gNotificationPtr->nmRefCon = 0L;
			
			// badge MacTelnetÕs Dock tile with a caution icon, if that preference is set
			if (gNotificationPreferences >= kAlert_NotifyDisplayDiamondMark) badgeApplicationDockTile();
			
			result = NMInstall(gNotificationPtr); // TEMP - should use AEInteractWithUser here, instead...
			UseResFile(oldResourceFile);
		}
	}
	return result;
}// backgroundNotification


/*!
Overlays a caution icon in the corner of the
applicationÕs Dock tile, to indicate an alert
condition.  This should only be done while the
application is backgrounded and an alert has
appeared.

(1.0)
*/
static OSStatus
badgeApplicationDockTile ()
{
	OSStatus	result = noErr;
	CGImageRef	cautionIconCGImage = createCGImageForCautionIcon();
	
	
	// Core Graphics mixes pixels together, so clear out anything
	// that might have been there before to avoid weird results
	RestoreApplicationDockTileImage();
	
	// badge the application Dock tile
	if (nullptr != cautionIconCGImage)
	{
		result = OverlayApplicationDockTileImage(cautionIconCGImage);
		CGImageRelease(cautionIconCGImage), cautionIconCGImage = nullptr;
	}
	
	return result;
}// badgeApplicationDockTile


/*!
Creates a Core Graphics image rendering the
standard Caution icon, 64x64 pixels in size.

(3.1)
*/
static CGImageRef
createCGImageForCautionIcon ()
{
	PicHandle	cautionIconBadgePicture = nullptr;
	PicHandle	cautionIconMaskPicture = nullptr;
	OSStatus	error = noErr;
	CGImageRef	result = nullptr;
	
	
	// read simple picture resources and blend them into an image with an alpha
	// channel on the fly (i.e. use QuickDraw source, get Core Graphics results)
	error = AppResources_GetDockTileAttentionPicture(cautionIconBadgePicture, cautionIconMaskPicture);
	if ((nullptr != cautionIconBadgePicture) && (nullptr != cautionIconMaskPicture))
	{
		error = Embedding_BuildCGImageFromPictureAndMask(cautionIconBadgePicture, cautionIconMaskPicture, &result);
		if (nullptr != cautionIconBadgePicture) KillPicture(cautionIconBadgePicture);
		if (nullptr != cautionIconMaskPicture) KillPicture(cautionIconMaskPicture);
	}
	return result;
}// createCGImageForCautionIcon


/*!
Responds to clicks in alert boxes.

(1.0)
*/
static void
handleItemHit	(InterfaceLibAlertPtr	inPtr,
				 DialogItemIndex		inItemIndex)
{
	switch (inItemIndex)
	{
	case kAlert_ItemButton1:
		inPtr->itemHit = kAlertStdAlertOKButton;
		{
			Boolean		animate = true;
			
			
			if (inPtr->isSheetForWindowCloseWarning) animate = false;
			setAlertVisibility(inPtr, false/* visible */, animate/* animate */);
		}
		break;
	
	case kAlert_ItemButton2:
		inPtr->itemHit = kAlertStdAlertCancelButton;
		setAlertVisibility(inPtr, false/* visible */, true/* animate */);
		break;
	
	case kAlert_ItemButton3:
		inPtr->itemHit = kAlertStdAlertOtherButton;
		setAlertVisibility(inPtr, false/* visible */, true/* animate */);
		break;
	
	case kAlert_ItemHelpButton:
		inPtr->itemHit = kAlertStdAlertHelpButton;
		setAlertVisibility(inPtr, false/* visible */, false/* animate */);
		break;
	
	default:
		inPtr->itemHit = 0;
		break;
	}
}// handleItemHit


/*!
Use this method to obtain a string containing
a standard button title (ex. Yes, No, Save,
etc.).  Call CFRelease() on the new string when
finished using it.

(1.0)
*/
static void
newButtonString		(CFStringRef&				outStringStorage,
					 UIStrings_ButtonCFString	inButtonStringType)
{
	UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
	
	
	stringResult = UIStrings_Copy(inButtonStringType, outStringStorage);
}// newButtonString


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the special key sequences dialog.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inInterfaceLibAlertPtr)
{
	OSStatus				result = eventNotHandledErr;
	InterfaceLibAlertPtr	ptr = REINTERPRET_CAST(inInterfaceLibAlertPtr, InterfaceLibAlertPtr);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			switch (received.commandID)
			{
			case kHICommandOK:
				// default button
				handleItemHit(ptr, kAlert_ItemButton1);
				break;
			
			case kHICommandCancel:
				// Cancel button
				handleItemHit(ptr, kAlert_ItemButton2);
				break;
			
			case kCommandAlertOtherButton:
				// 3rd button
				handleItemHit(ptr, kAlert_ItemButton3);
				break;
			
			case kCommandContextSensitiveHelp:
				// help button
				handleItemHit(ptr, kAlert_ItemHelpButton);
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldnÕt be able to select menu commands while the window is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Shows or hide an alert window.  This is ÒspecialÓ
because you can animate the effect.

(1.0)
*/
static void
setAlertVisibility	(InterfaceLibAlertPtr	inPtr,
					 Boolean				inIsVisible,
					 Boolean				inAnimate)
{
	if (inIsVisible)
	{
		if (inPtr->isSheet)
		{
			ShowSheetWindow(inPtr->dialogWindow, inPtr->parentWindow);
		}
		else
		{
			ShowWindow(inPtr->dialogWindow);
			EventLoop_SelectOverRealFrontWindow(inPtr->dialogWindow);
		}
		
		// speak text - UNIMPLEMENTED
	}
	else
	{
		if (inPtr->isSheet)
		{
			unless (inAnimate)
			{
				// hide the parent to hide the sheet immediately
				HideWindow(inPtr->parentWindow);
			}
			HideSheetWindow(inPtr->dialogWindow);
			
			AlertMessages_InvokeCloseNotifyProc(inPtr->sheetCloseNotifier, inPtr->selfRef, inPtr->itemHit,
												inPtr->sheetCloseNotifierUserData);
		}
		else
		{
			QuitAppModalLoopForWindow(inPtr->dialogWindow);
			HideWindow(inPtr->dialogWindow);
		}
	}
}// setAlertVisibility


/*!
Displays a dialog box on the screen that reflects
the parameters of an Interface Library Alert.  Any
errors that occur, preventing the dialog from
displaying, will be returned.  The item number hit
is stored using the given alert reference, and can
be obtained by calling Alert_ItemHit().

On Mac OS X, if the alert is in fact window-modal,
this routine will return immediately WITHOUT having
completed handling the alert.  Instead, the alertÕs
notifier is invoked whenever the user closes the
alert.

This method is invoked by Alert_Display().

(1.0)
*/
static OSStatus
standardAlert	(InterfaceLibAlertPtr	inAlert,
				 AlertType				inAlertType,
				 CFStringRef			inDialogText,
				 CFStringRef			inHelpText)
{
	InterfaceLibAlertPtr	ptr = inAlert;
	OSStatus				result = noErr;
	
	
	if (nullptr != ptr)
	{
		enum
		{
			kTopEdgeIcon = 18		// pixels; system standard offset from the top of the alert box is 10 pixels
		};
		//Boolean				isButton1 = true;
		Boolean				isButton2 = true;
		Boolean				isButton3 = true;
		
		
		// load the NIB containing this dialog (automatically finds the right localization)
		ptr->dialogWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
										CFSTR("AlertMessages"), ptr->isSheet ? CFSTR("Sheet") : CFSTR("Dialog"))
							<< NIBLoader_AssertWindowExists;
		
		// find references to all controls that are needed for any operation
		// (button clicks, responding to window resizing, etc.)
		{
			OSStatus	error = noErr;
			
			
			error = GetControlByID(ptr->dialogWindow, &idMyButtonDefault, &ptr->buttonDefault);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyButtonCancel, &ptr->buttonCancel);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyButtonOther, &ptr->buttonOther);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyButtonHelp, &ptr->buttonHelp);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyTextTitle, &ptr->textTitle);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyTextMain, &ptr->textMain);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyTextHelp, &ptr->textHelp);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyStopIcon, &ptr->stopIcon);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyNoteIcon, &ptr->noteIcon);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyCautionIcon, &ptr->cautionIcon);
			assert_noerr(error);
			error = GetControlByID(ptr->dialogWindow, &idMyApplicationIcon, &ptr->applicationIcon);
			assert_noerr(error);
		}
		
		// set up fonts (since NIBs are too crappy to support this in older Mac OS X versions)
		Localization_SetControlThemeFontInfo(ptr->textTitle, USHRT_MAX/* special font - see Localization_UseThemeFont() */);
		Localization_SetControlThemeFontInfo(ptr->textMain, kThemeAlertHeaderFont);
		Localization_SetControlThemeFontInfo(ptr->textHelp, kThemeSmallSystemFont);
		
		// set the window title to be the application name, on Mac OS X
		{
			CFStringRef		string;
			
			
			Localization_GetCurrentApplicationNameAsCFString(&string);
			SetWindowTitleWithCFString(ptr->dialogWindow, string);
		}
		
		// if the alert should have a title, give it one
		if (ptr->titleCFString.exists())
		{
			SetControlTextWithCFString(ptr->textTitle, ptr->titleCFString.returnCFStringRef());
		}
		
		// use a 32-bit icon for the help button, if possible
		{
			IconManagerIconRef		icon = IconManager_NewIcon();
			
			
			IconManager_MakeIconSuite(icon, kHelpIconResource, kSelectorAllSmallData);
			(OSStatus)IconManager_SetButtonIcon(ptr->buttonHelp, icon);
			IconManager_DisposeIcon(&icon);
		}
		
		//
		// set up dialog items to reflect alert parameters
		//
		
		if (nullptr != ptr->params.defaultText)
		{
			// if the alert is configured to use the default title (ÒOKÓ), copy the appropriate string resource
			if (ptr->params.defaultText == REINTERPRET_CAST(kAlertDefaultOKText, CFStringRef))
			{
				CFStringRef		myString = nullptr;
				
				
				newButtonString(myString, kUIStrings_ButtonOK);
				SetControlTitleWithCFString(ptr->buttonDefault, myString);
				CFRelease(myString);
			}
			else SetControlTitleWithCFString(ptr->buttonDefault, ptr->params.defaultText);
		}
		else
		{
			CFStringRef		myString = nullptr;
			
			
			newButtonString(myString, kUIStrings_ButtonOK);
			SetControlTitleWithCFString(ptr->buttonDefault, myString);
			CFRelease(myString);
		}
		
		if (nullptr != ptr->params.cancelText)
		{
			// if the alert is configured to use the default title (ÒCancelÓ), copy the appropriate string resource
			if (ptr->params.cancelText == REINTERPRET_CAST(kAlertDefaultCancelText, CFStringRef))
			{
				CFStringRef		myString = nullptr;
				
				
				newButtonString(myString, kUIStrings_ButtonCancel);
				SetControlTitleWithCFString(ptr->buttonCancel, myString);
				CFRelease(myString);
			}
			else SetControlTitleWithCFString(ptr->buttonCancel, ptr->params.cancelText);
		}
		else
		{
			ptr->params.cancelText = nullptr;
			SetControlVisibility(ptr->buttonCancel, false/* visible */, false/* draw */);
			isButton2 = false;
		}
		
		if (nullptr != ptr->params.otherText)
		{
			// if the alert is configured to use the default title (ÒDonÕt SaveÓ), copy the appropriate string resource
			if (ptr->params.otherText == REINTERPRET_CAST(kAlertDefaultOtherText, CFStringRef))
			{
				CFStringRef		myString = nullptr;
				
				
				newButtonString(myString, kUIStrings_ButtonDontSave);
				SetControlTitleWithCFString(ptr->buttonOther, myString);
				CFRelease(myString);
			}
			else SetControlTitleWithCFString(ptr->buttonOther, ptr->params.otherText);
		}
		else
		{
			ptr->params.otherText = nullptr;
			SetControlVisibility(ptr->buttonOther, false/* visible */, false/* draw */);
			isButton3 = false;
		}
		
		if (!ptr->params.helpButton) SetControlVisibility(ptr->buttonHelp, false/* visibility */, false/* draw */);
		SetControlTextWithCFString(ptr->textMain, inDialogText);
		if (nullptr != inHelpText) SetControlTextWithCFString(ptr->textHelp, inHelpText);
		else SetControlVisibility(ptr->textHelp, false/* visibility */, false/* draw */);
		
		// set the alert icon properly for the alert type
		{
			switch (inAlertType)
			{
			case kAlertStopAlert:
				ptr->mainIcon = ptr->stopIcon;
				break;
			
			case kAlertNoteAlert:
				ptr->mainIcon = ptr->noteIcon;
				break;
			
			case kAlertCautionAlert:
				ptr->mainIcon = ptr->cautionIcon;
				break;
			
			case kAlertPlainAlert:
			default:
				{
					Rect	bounds;
					
					
					ptr->mainIcon = nullptr;
					
					// for plain alerts, resize the application "badge" as it is now the ENTIRE icon (that
					// is, the normal icon is hidden but it should appear to be a large application icon)
					GetControlBounds(ptr->mainIcon, &bounds);
					SetControlBounds(ptr->applicationIcon, &bounds);
					SetControlVisibility(ptr->mainIcon, false/* visible */, false/* draw */);
				}
				break;
			}
			
			// hide all icons not applicable to the alert
			{
				ControlRef		allIcons[] = { ptr->stopIcon, ptr->noteIcon, ptr->cautionIcon };
				
				
				for (register size_t i = 0; i < sizeof(allIcons) / sizeof(ControlRef); ++i)
				{
					if (ptr->mainIcon != allIcons[i])
					{
						SetControlVisibility(allIcons[i], false/* visible */, false/* draw */);
					}
				}
			}
		}
		
		// make the dialog big enough to fit the text, then display it
		{
			enum
			{
				kTopEdgeText = kTopEdgeIcon,		// system standard is to make the text the same vertical offset as the icon
				kSpacingBetweenTextItems = 10		// width in pixels vertically separating dialog text and help text
			};
			SInt16		textExpanseV = 0;
			SInt16		totalExpanseV = 0; // initialized to vertical position of top edge of dialog text item
			Rect		controlRect;
			
			
			totalExpanseV = kTopEdgeText;
			
			// size and arrange title text, if present
			if (ptr->titleCFString.exists())
			{
				GetControlBounds(ptr->textTitle, &controlRect);
				//(OSStatus)SetControlData(ptr->textTitle, kControlEntireControl, kControlStaticTextCFStringTag,
				//							STATIC_CAST(PLstrlen(ptr->title) * sizeof(UInt8), Size), ptr->title + 1);
				textExpanseV = 22; // should be fixed to properly calculate dimensions, but alas, IÕve been lazy
				//SizeControl(ptr->textTitle, INTERFACELIB_ALERT_DIALOG_WD - HSP_BUTTON_AND_DIALOG - controlRect.left, textExpanseV);
				
				// the text width is automatically set in the NIB
				SizeControl(ptr->textTitle, STATIC_CAST(controlRect.right - controlRect.left, SInt16), textExpanseV);
				MoveControl(ptr->textTitle, controlRect.left, totalExpanseV);
				totalExpanseV += (textExpanseV + kSpacingBetweenTextItems);
			}
			else
			{
				SetControlVisibility(ptr->textTitle, false/* visible */, false/* draw */);
			}
			
			// size and arrange message text (always present)
			GetControlBounds(ptr->textMain, &controlRect);
			{
				Point		dimensions;
				SInt16		offset = 0;
				OSStatus	error = noErr;
				
				
				SetPt(&dimensions, controlRect.right - controlRect.left, controlRect.bottom - controlRect.top);
				error = GetThemeTextDimensions(inDialogText, kThemeAlertHeaderFont, kThemeStateActive,
												true/* wrap to width */, &dimensions, &offset);
				assert_noerr(error);
				
				// set the text size appropriately
				textExpanseV = dimensions.v;
				SizeControl(ptr->textMain, controlRect.right - controlRect.left, textExpanseV);
			}
			MoveControl(ptr->textMain, controlRect.left, totalExpanseV);
			totalExpanseV += textExpanseV;
			
			// size and arrange help text, if present
			if (nullptr != inHelpText)
			{
				totalExpanseV += kSpacingBetweenTextItems;
				GetControlBounds(ptr->textHelp, &controlRect);
				{
					Point		dimensions;
					SInt16		offset = 0;
					OSStatus	error = noErr;
					
					
					SetPt(&dimensions, controlRect.right - controlRect.left, controlRect.bottom - controlRect.top);
					error = GetThemeTextDimensions(inHelpText, kThemeSmallSystemFont, kThemeStateActive,
													true/* wrap to width */, &dimensions, &offset);
					assert_noerr(error);
					
					// set the text size appropriately
					textExpanseV = dimensions.v;
					SizeControl(ptr->textHelp, controlRect.right - controlRect.left, textExpanseV);
				}
				MoveControl(ptr->textHelp, controlRect.left, totalExpanseV);
				totalExpanseV += textExpanseV;
			}
			
			// make the alert window just big enough for any text and buttons being displayed
			{
				Rect	windowBounds;
				
				
				GetWindowBounds(ptr->dialogWindow, kWindowContentRgn, &windowBounds);
				windowBounds.bottom = windowBounds.top +
										STATIC_CAST(totalExpanseV +
													INTEGER_DOUBLED(VSP_BUTTON_AND_DIALOG) + BUTTON_HT,
											SInt16);
				(OSStatus)SetWindowBounds(ptr->dialogWindow, kWindowContentRgn, &windowBounds);
			}
			
			// for right-to-left localization only, the alert icon and text will switch places
			{
				Rect	dialogTextRect;
				
				
				Localization_HorizontallyPlaceControls(ptr->mainIcon, ptr->textMain);
				GetControlBounds(ptr->textMain, &dialogTextRect);
				if (nullptr != inHelpText)
				{
					Rect	helpTextRect;
					
					
					// shift the title text and help text to the same horizontal position as the dialog text moved to
					GetControlBounds(ptr->textHelp, &helpTextRect);
					MoveControl(ptr->textHelp, dialogTextRect.left, helpTextRect.top);
					GetControlBounds(ptr->textTitle, &helpTextRect);
					MoveControl(ptr->textTitle, dialogTextRect.left, helpTextRect.top);
				}
			}
			
			// move the dialog buttons vertically to their proper place in the new dialog
			{
				ControlRef		buttonControls[2] = { ptr->buttonOther, nullptr };
				
				
				// use this ÒsneakyÓ approach to correctly set up the vertical component of the third button
				(UInt16)Localization_ArrangeButtonArray(buttonControls, 1);
				
				// auto-arrange the buttons in the OK and Cancel positions
				buttonControls[0] = ptr->buttonDefault;
				buttonControls[1] = ptr->buttonCancel;
				(UInt16)Localization_ArrangeButtonArray(buttonControls, 2);
				
				if (isButton3)
				{
					// move and size that third button
					SInt16		x = 0;
					UInt16		buttonWidth = 0;
					ControlRef	buttonControl = nullptr;
					
					
					// first resize the button to fit its title
					buttonWidth = Localization_AutoSizeButtonControl(ptr->buttonOther, 0/* minimum width */);
					
					// now determine where the left edge of the button should be (locale-sensitive)
					buttonControl = (isButton2) ? ptr->buttonCancel : ptr->buttonDefault;
					GetControlBounds(buttonControl, &controlRect);
					if (Localization_IsLeftToRight()) x = STATIC_CAST(controlRect.left - HSP_BUTTONS - buttonWidth, SInt16);
					else x = STATIC_CAST(controlRect.right + HSP_BUTTONS, SInt16);
					
					// now move the third button appropriately
					buttonControl = ptr->buttonOther;
					GetControlBounds(buttonControl, &controlRect);
					MoveControl(buttonControl, x, controlRect.top);
				}
			}
			
			// determine if the Help button should be disabled...
			HIViewWrap		helpButtonObject(ptr->buttonHelp);
			DialogUtilities_SetUpHelpButton(helpButtonObject);
			
			// move the help button, if itÕs supposed to be there
			if (ptr->params.helpButton) Localization_AdjustHelpButtonControl(ptr->buttonHelp);
		}
		
		// install a callback that responds to buttons in the window
		{
			EventTypeSpec const		whenButtonClicked[] =
									{
										{ kEventClassCommand, kEventCommandProcess }
									};
			OSStatus				error = noErr;
			
			
			error = InstallWindowEventHandler(ptr->dialogWindow, gAlertButtonHICommandUPP,
												GetEventTypeCount(whenButtonClicked), whenButtonClicked,
												ptr/* user data */,
												&ptr->buttonHICommandsHandler/* event handler reference */);
			assert_noerr(error);
		}
		
		// get the rectangle where the alert box is supposed to be, and animate it into position (with Mac OS 8.5 and beyond)
		setAlertVisibility(ptr, true/* visible */, true/* animate */);
		
		// advance focus (the default button can already be easily hit with
		// the keyboard, so focusing the next button makes it equally easy
		// for the user to choose that option with a key)
		{
			// once for the first button...
			(OSStatus)HIViewAdvanceFocus(HIViewGetRoot(ptr->dialogWindow)/* view */, 0/* modifiers */);
			// ...and again for the 2nd one (this hack depends on layering in the NIB!)
			(OSStatus)HIViewAdvanceFocus(HIViewGetRoot(ptr->dialogWindow)/* view */, 0/* modifiers */);
		}
		
		unless (ptr->isSheet)
		{
			ShowWindow(ptr->dialogWindow);
			EventLoop_SelectOverRealFrontWindow(ptr->dialogWindow);
			RunAppModalLoopForWindow(ptr->dialogWindow);
			
			Alert_ServiceNotification(); // just in case it wasnÕt removed properly
			setAlertVisibility(ptr, false/* visible */, true/* animate */);
			DisposeWindow(ptr->dialogWindow), ptr->dialogWindow = nullptr;
		}
	}
	else result = memPCErr;
	
	return result;
}// standardAlert

// BELOW IS REQUIRED NEWLINE TO END FILE
