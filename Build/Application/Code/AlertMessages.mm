/*!	\file AlertMessages.mm
	\brief Handles Cocoa-based alerts and notifications.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2013 by Kevin Grant
	
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

#import <AlertMessages.h>
#import <UniversalDefines.h>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <AutoPool.objc++.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <Cursors.h>
#import <Embedding.h>
#import <HIViewWrap.h>
#import <IconManager.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <Registrar.template.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "DialogUtilities.h"
#import "EventLoop.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Dialog" and "Sheet" NIBs from the package "AlertMessages.nib".
*/
HIViewID const	idMyButtonDefault		= { 'Dflt', 0/* ID */ };
HIViewID const	idMyButtonCancel		= { 'Canc', 0/* ID */ };
HIViewID const	idMyButtonOther			= { 'Othr', 0/* ID */ };
HIViewID const	idMyButtonHelp			= { 'Help', 0/* ID */ };
HIViewID const	idMyTextTitle			= { 'Titl', 0/* ID */ };
HIViewID const	idMyTextMain			= { 'Main', 0/* ID */ };
HIViewID const	idMyTextHelp			= { 'Smal', 0/* ID */ };
HIViewID const	idMyStopIcon			= { 'Stop', 0/* ID */ };
HIViewID const	idMyNoteIcon			= { 'Note', 0/* ID */ };
HIViewID const	idMyCautionIcon			= { 'Caut', 0/* ID */ };
HIViewID const	idMyApplicationIcon		= { 'AppI', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

typedef MemoryBlockReferenceTracker< AlertMessages_BoxRef >		My_RefTracker;
typedef Registrar< AlertMessages_BoxRef, My_RefTracker >		My_RefRegistrar;

struct My_AlertMessage
{
public:
	My_AlertMessage ();
	~My_AlertMessage ();
	
	My_RefRegistrar									refValidator;			//!< ensures this reference is recognized as a valid one
	AlertMessages_BoxRef							selfRef;
	
	AlertMessages_ModalWindowController*			asModal;				//!< set to "nil" unless this is an application-modal alert
	AlertMessages_ModalWindowController*			asSheet;				//!< set to "nil" unless this is a window-modal alert
	AlertMessages_NotificationWindowController*		asNotification;			//!< set to "nil" unless this is a modeless alert
	AlertMessages_WindowController*					targetWindowController;	//!< message target; set to match one of the above
	
	// TEMPORARY; most of what follows has traditionally been needed
	// to drive Carbon-based interfaces, and it will all go away
	// once the transition to Cocoa is complete
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
	UInt16							itemHit;
	Boolean							isCompletelyModeless;
	Boolean							isSheet;
	Boolean							isSheetForWindowCloseWarning;
	WindowRef						dialogWindow;
	WindowRef						parentWindow;
	NSWindow*						parentNSWindow;
	EventHandlerRef					buttonHICommandsHandler;
	AlertMessages_CloseNotifyProcPtr	sheetCloseNotifier;
	void*							sheetCloseNotifierUserData;
};
typedef My_AlertMessage*	My_AlertMessagePtr;

typedef MemoryBlockPtrLocker< AlertMessages_BoxRef, My_AlertMessage >	My_AlertPtrLocker;
typedef LockAcquireRelease< AlertMessages_BoxRef, My_AlertMessage >		My_AlertAutoLocker;

} // anonymous namespace

#pragma mark Variables
namespace {

Boolean				gNotificationIsBackgrounded = false;
Boolean				gUseSpeech = false;
Boolean				gIsInited = false;
Boolean				gDebuggingEnabled = true;
UInt16				gNotificationPreferences = kAlert_NotifyDisplayDiamondMark;
Str255				gNotificationMessage;
NMRecPtr			gNotificationPtr = nullptr;
EventHandlerUPP		gAlertButtonHICommandUPP = nullptr;
My_AlertPtrLocker&	gAlertPtrLocks ()	{ static My_AlertPtrLocker x; return x; }
My_RefTracker&		gAlertBoxValidRefs ()	{ static My_RefTracker x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSStatus		backgroundNotification			();
void			badgeApplicationDockTile		();
id				flagNo							();
void			handleItemHitCarbon				(My_AlertMessagePtr, DialogItemIndex);
void			newButtonString					(CFRetainRelease&, UIStrings_ButtonCFString);
void			prepareForDisplay				(My_AlertMessagePtr);
OSStatus		receiveHICommand				(EventHandlerCallRef, EventRef, void*);
void			setCarbonAlertVisibility		(My_AlertMessagePtr, Boolean, Boolean);
OSStatus		standardAlert					(My_AlertMessagePtr, AlertType, CFStringRef, CFStringRef);

} // anonymous namespace

/*!
The private class interface.
*/
@interface AlertMessages_WindowController (AlertMessages_WindowControllerInternal) //{

	- (void)
	abortAlert;
	- (void)
	deltaSizeWindow:(float)_;
	- (void)
	didFinishAlertWithButton:(unsigned int)_
	fromEvent:(BOOL)_;
	- (NSImage*)
	imageForIconImageName:(NSString*)_;
	- (void)
	setDisplayedDialogText:(NSString*)_;
	- (void)
	setDisplayedHelpText:(NSString*)_;
	- (void)
	setStringProperty:(NSString**)_
	withName:(NSString*)_
	toValue:(NSString*)_;

@end //}



#pragma mark Public Methods

/*!
This routine should be called once, before any
other Alert routine.  The corresponding method
Alert_Done() should be invoked once at the end
of the program (most likely), after the Alert
module is not needed anymore.

The Alert module needs the reference number of
your application’s resource file.  You usually
call Alert_Init() at startup time.

(1.0)
*/
void
Alert_Init ()
{
	gAlertButtonHICommandUPP = NewEventHandlerUPP(receiveHICommand);
	
	// register application icon, because the "AlertMessages.nib" file
	// relies on this type/creator combination for the icon badge
	{
		IconRef		result = nullptr;
		FSRef		iconFile;
		
		
		if (AppResources_GetArbitraryResourceFileFSRef
			(AppResources_ReturnBundleIconFilenameNoExtension(),
				CFSTR("icns")/* type */, iconFile))
		{
			if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
													kConstantsRegistry_IconServicesIconApplication,
													&iconFile, &result))
			{
				// failed!
				Console_Warning(Console_WriteLine, "failed to register application icon for use in alert messages");
			}
		}
	}
	
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
Creates an application-modal alert without displaying
it (this gives you the opportunity to specify its
characteristics using other methods from this module).
When you are finished with the alert, use the method
Alert_Dispose() to destroy it.

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
AlertMessages_BoxRef
Alert_New ()
{
	AlertMessages_BoxRef	result = nullptr;
	
	
	try
	{
		My_AlertMessagePtr		alertPtr = new My_AlertMessage();
		
		
		if (nullptr != alertPtr)
		{
			alertPtr->targetWindowController = alertPtr->asModal =
				[[AlertMessages_ModalWindowController alloc] init];
			if (nil == alertPtr->targetWindowController)
			{
				delete alertPtr;
			}
			else
			{
				[alertPtr->targetWindowController setDataPtr:alertPtr];
				
				result = REINTERPRET_CAST(alertPtr, AlertMessages_BoxRef);
			}
		}
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Constructs a notification window.

A modeless alert is no different than a sheet in terms of
handling, except that it is not associated with any window.  And
since it is not modal, the user can literally ignore the alert
forever.  (The Finder displays these kinds of alerts if problems
arise during file copies, for example.)

When you subsequently display the alert, you will actually have
to defer handling of the alert until the notifier you specify is
invoked.  For alerts with a single button for which you don’t
care about the result, just use "Alert_StandardCloseNotifyProc"
as your notifier; for all other kinds of alerts, pass your own
custom routine so you can tell which button was hit.

Currently, the initial location of a modeless alert is chosen
automatically.  It is also auto-saved as a user preference, so
that new alerts will generally appear where the user wants them
to be.

(2.3)
*/
AlertMessages_BoxRef
Alert_NewModeless	(AlertMessages_CloseNotifyProcPtr	inCloseNotifyProcPtr,
					 void*								inCloseNotifyProcUserData)
{
	AutoPool				_;
	AlertMessages_BoxRef	result = nullptr;
	
	
	try
	{
		My_AlertMessagePtr		alertPtr = new My_AlertMessage();
		
		
		if (nullptr != alertPtr)
		{
			alertPtr->targetWindowController = alertPtr->asNotification =
				[[AlertMessages_NotificationWindowController alloc] init];
			if (nil == alertPtr->targetWindowController)
			{
				delete alertPtr;
			}
			else
			{
				[alertPtr->targetWindowController setDataPtr:alertPtr];
				
				alertPtr->isCompletelyModeless = true;
				alertPtr->sheetCloseNotifier = inCloseNotifyProcPtr;
				alertPtr->sheetCloseNotifierUserData = inCloseNotifyProcUserData;
				
				result = REINTERPRET_CAST(alertPtr, AlertMessages_BoxRef);
			}
		}
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// NewModeless


/*!
Constructs a translucent sheet alert, as opposed to a
movable modal dialog box.

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
a single button for which you don’t care about the
result, just use "Alert_StandardCloseNotifyProc"
as your notifier; for all other kinds of alerts,
pass your own custom routine so you can tell which
button was hit.

(2.5)
*/
AlertMessages_BoxRef
Alert_NewWindowModal	(NSWindow*							inParentWindow,
						 Boolean							inIsParentWindowCloseWarning,
						 AlertMessages_CloseNotifyProcPtr	inCloseNotifyProcPtr,
						 void*								inCloseNotifyProcUserData)
{
	AutoPool				_;
	AlertMessages_BoxRef	result = nullptr;
	
	
	try
	{
		My_AlertMessagePtr		alertPtr = new My_AlertMessage();
		
		
		if (nullptr != alertPtr)
		{
			alertPtr->parentNSWindow = inParentWindow;
			alertPtr->targetWindowController = alertPtr->asSheet =
				[[AlertMessages_ModalWindowController alloc] init];
			if (nil == alertPtr->targetWindowController)
			{
				delete alertPtr;
			}
			else
			{
				[alertPtr->targetWindowController setDataPtr:alertPtr];
				
				alertPtr->isSheet = true;
				alertPtr->isSheetForWindowCloseWarning = inIsParentWindowCloseWarning;
				
				alertPtr->sheetCloseNotifier = inCloseNotifyProcPtr;
				alertPtr->sheetCloseNotifierUserData = inCloseNotifyProcUserData;
				
				result = REINTERPRET_CAST(alertPtr, AlertMessages_BoxRef);
			}
		}
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// NewWindowModal (NSWindow*, ...)


/*!
DEPRECATED.  For displaying alerts over Carbon windows only.
See also the method of the same name that accepts an "NSWindow*".

(2.5)
*/
AlertMessages_BoxRef
Alert_NewWindowModal	(HIWindowRef						inParentWindow,
						 Boolean							inIsParentWindowCloseWarning,
						 AlertMessages_CloseNotifyProcPtr	inCloseNotifyProcPtr,
						 void*								inCloseNotifyProcUserData)
{
	AlertMessages_BoxRef	result = nullptr;
	
	
	try
	{
		My_AlertMessagePtr		alertPtr = new My_AlertMessage();
		
		
		if (nullptr != alertPtr)
		{
			alertPtr->isSheet = true;
			alertPtr->isSheetForWindowCloseWarning = inIsParentWindowCloseWarning;
			alertPtr->parentWindow = inParentWindow;
			alertPtr->sheetCloseNotifier = inCloseNotifyProcPtr;
			alertPtr->sheetCloseNotifierUserData = inCloseNotifyProcUserData;
			
			result = REINTERPRET_CAST(alertPtr, AlertMessages_BoxRef);
		}
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// NewWindowModal (HIWindowRef, ...)


/*!
When you are finished with an alert that you
created with Alert_New(), call this routine
to destroy it.  The reference is automatically
set to nullptr.

(1.0)
*/
void
Alert_Dispose	(AlertMessages_BoxRef*		inoutAlert)
{
	if (nullptr != inoutAlert)
	{
		if (gAlertBoxValidRefs().end() == gAlertBoxValidRefs().find(*inoutAlert))
		{
			Console_Warning(Console_WriteValueAddress, "attempt to Alert_Dispose() with invalid reference", *inoutAlert);
		}
		else
		{
			delete *(REINTERPRET_CAST(inoutAlert, My_AlertMessagePtr*)), *inoutAlert = nullptr;
		}
	}
}// Dispose


/*!
Hits an item in an open alert window as if the user
hit the item.  Generally used in open sheets to force
behavior (for example, forcing a sheet to Cancel and
slide closed).

(1.0)
*/
void
Alert_Abort		(AlertMessages_BoxRef	inAlert)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nil != alertPtr->targetWindowController)
	{
		[alertPtr->targetWindowController abortAlert];
	}
	else
	{
		handleItemHitCarbon(alertPtr, kAlert_ItemButtonNone);
	}
}// Abort


/*!
Posts a general notification to the queue, with
no particular message.  So this grabs the user’s
attention for something other than an alert
message.  The exact form of notification depends
on the notification preferences specified via
Alert_SetNotificationPreferences().

(1.1)
*/
void
Alert_BackgroundNotification ()
{
	UNUSED_RETURN(OSStatus)backgroundNotification();
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
		AlertMessages_BoxRef	box = nullptr;
		
		
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
otherwise send information to your application’s
debugging systems.

This isn’t really an alert routine, but it has
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
		currentProcessID.lowLongOfPSN = kCurrentProcess; // don’t use GetCurrentProcess()!
		UNUSED_RETURN(OSStatus)AECreateDesc(typeProcessSerialNumber,
											&currentProcessID, sizeof(ProcessSerialNumber), &currentProcessAddress);
		UNUSED_RETURN(OSStatus)AECreateAppleEvent(inEventClass, inEventID, &currentProcessAddress, kAutoGenerateReturnID, kAnyTransactionID, &consoleWriteEvent);
		UNUSED_RETURN(OSStatus)AEPutParamPtr(&consoleWriteEvent, keyDirectObject, cString, inSendWhat,
												STATIC_CAST(CPP_STD::strlen(inSendWhat) * sizeof(char), Size));
		UNUSED_RETURN(OSStatus)AESend(&consoleWriteEvent, &reply, kAENoReply | kAENeverInteract | kAEDontRecord,
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
otherwise send information to your application’s
debugging systems.

This isn’t really an alert routine, but it has
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
		currentProcessID.lowLongOfPSN = kCurrentProcess; // don’t use GetCurrentProcess()!
		UNUSED_RETURN(OSStatus)AECreateDesc(typeProcessSerialNumber,
											&currentProcessID, sizeof(ProcessSerialNumber), &currentProcessAddress);
		UNUSED_RETURN(OSStatus)AECreateAppleEvent(inEventClass, inEventID, &currentProcessAddress, kAutoGenerateReturnID, kAnyTransactionID, &consoleWriteEvent);
		UNUSED_RETURN(OSStatus)AEPutParamPtr(&consoleWriteEvent, keyDirectObject, cString, inSendWhat + 1,
												STATIC_CAST(PLstrlen(inSendWhat) * sizeof(UInt8), Size));
		UNUSED_RETURN(OSStatus)AESend(&consoleWriteEvent, &reply, kAENoReply | kAENeverInteract | kAEDontRecord,
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
  number (kAlert_ItemButton1, etc.) by invoking the
  Alert_ItemHit() method.
- For window-modal alerts (Mac OS X only), the sheet
  will be displayed and then this routine will return
  immediately.  When the alert is finally dismissed,
  the notification routine specified by
  Alert_NewWindowModal() will be invoked, at which
  time you find out which button (if any) was chosen.
  Only at that point is it safe to dispose of the
  alert with Alert_Dispose().

The "inAnimated" flag is currently only respected for
application-modal alerts, and only means something on
Mac OS X 10.7 and beyond (where the window can “pop”
open).  If you suppress animation, the window is
displayed immediately without the standard animation.
This is not recommended except in special situations.

If the program is in the background (as specified by
a call to Alert_SetIsBackgrounded()), a notification
is posted.  The alert is displayed immediately, but
the notification lets the user know that it’s there.
Your application must call Alert_ServiceNotification()
whenever a “resume” event is detected in your event
loop, in order to service notifications posted by the
Alert module.

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
Alert_Display	(AlertMessages_BoxRef	inAlert,
				 Boolean				inAnimated)
{
	OSStatus	result = noErr;
	
	
	// only attempt to display an alert if the module was properly initialized
	if (gIsInited)
	{
		My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
		
		
		result = backgroundNotification();
		Cursors_UseArrow();
		if (nil != alertPtr->targetWindowController)
		{
			// Cocoa implementation
			prepareForDisplay(alertPtr);
			if (alertPtr->isSheet)
			{
				if (nil != alertPtr->asSheet)
				{
					[NSApp beginSheet:[alertPtr->targetWindowController window]
										modalForWindow:alertPtr->parentNSWindow
										modalDelegate:alertPtr->asSheet
										didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:)
										contextInfo:inAlert];
				}
			}
			else if (alertPtr->isCompletelyModeless)
			{
				if (nil != alertPtr->asNotification)
				{
					[[alertPtr->targetWindowController window] orderBack:NSApp];
				}
			}
			else
			{
				NSWindow*	window = [alertPtr->targetWindowController window];
				
				
				if (inAnimated)
				{
					if (FlagManager_Test(kFlagOS10_7API) && [window respondsToSelector:@selector(setAnimationBehavior:)])
					{
						[window setAnimationBehavior:FUTURE_SYMBOL(5, NSWindowAnimationBehaviorAlertPanel)];
					}
				}
				else
				{
					if (FlagManager_Test(kFlagOS10_7API) && [window respondsToSelector:@selector(setAnimationBehavior:)])
					{
						[window setAnimationBehavior:FUTURE_SYMBOL(2, NSWindowAnimationBehaviorNone)];
					}
				}
				[NSApp beginSheet:window modalForWindow:nil modalDelegate:nil didEndSelector:nil contextInfo:nil];
				UNUSED_RETURN(int)[NSApp runModalForWindow:window];
				[NSApp endSheet:window];
				Alert_ServiceNotification(); // just in case it wasn’t removed properly
				[window orderOut:NSApp];
			}
			
			// speak text if necessary - UNIMPLEMENTED
		}
		else
		{
			// legacy Carbon implementation; now only needed for certain sheets
		#if 1
			result = standardAlert(alertPtr, alertPtr->alertType, alertPtr->dialogTextCFString.returnCFStringRef(),
									alertPtr->helpTextCFString.returnCFStringRef());
		#else
			//(OSStatus)CreateStandardAlert(alertPtr->alertType, alertPtr->dialogTextCFString.returnCFStringRef(),
			//								alertPtr->helpTextCFString.returnCFStringRef(),
			//								&alertPtr->params, &alertPtr->itemHit, &someDialog);
		#endif
		}
	}
	else
	{
		result = notInitErr;
	}
	
	return result;
}// Display


/*!
Determines the number of the button that was selected
by the user.  A value of 1, 2, or 3 indicates the
respective button positions for the default button,
cancel button, and “other” button.  The value 4
indicates the help button.  If the alert times out
(that is, it “closed itself”), 0 is returned.

(1.0)
*/
UInt16
Alert_ItemHit	(AlertMessages_BoxRef	inAlert)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	UInt16					result = kAlert_ItemButton3;
	
	
	if (nullptr != alertPtr) result = alertPtr->itemHit;
	return result;
}// ItemHit


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
	AlertMessages_BoxRef	box = nullptr;
	
	
	box = Alert_New(); // no help button, single OK button
	Alert_SetHelpButton(box, inIsHelpButton);
	Alert_SetTextCFStrings(box, inDialogText, inHelpText);
	Alert_SetType(box, kAlertNoteAlert);
	Alert_Display(box);
	Alert_Dispose(&box);
}// Message


/*!
Automatically reports 16-bit or 32-bit error result
codes.  If the specified error is either "noErr" or
"userCanceledErr", no alert is displayed and false is
returned; otherwise, true is returned to indicate a
serious error.

If "inAssertion" is true, the alert offers the user
the option to quit the program or continue.  If the
user chooses to quit, ExitToShell() is invoked.
Without an assertion flag, only a single Continue
button is provided in the alert.

(1.0)
*/
Boolean
Alert_ReportOSStatus	(OSStatus	inErrorCode,
						 Boolean	inAssertion)
{
	Boolean			result = true;
	
	
	if ((inErrorCode == userCanceledErr) || (inErrorCode == noErr)) result = false;
	
	if (result)
	{
		UIStrings_Result		stringResult = kUIStrings_ResultOK;
		AlertMessages_BoxRef	box = nullptr;
		CFStringRef				primaryTextCFString = nullptr;
		CFStringRef				primaryTextTemplateCFString = nullptr;
		CFStringRef				helpTextCFString = nullptr;
		Boolean					doExit = false;
		
		
		// generate dialog text; some “common” errors may have custom messages
		switch (inErrorCode)
		{
		case dupFNErr:
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowCFFileExistsPrimaryText, primaryTextTemplateCFString);
			if (stringResult.ok())
			{
				primaryTextCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
																primaryTextTemplateCFString, inErrorCode);
			}
			else
			{
				primaryTextCFString = CFSTR("");
			}
			break;
		
		case wrPermErr:
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowCFNoWritePermissionPrimaryText, primaryTextTemplateCFString);
			if (stringResult.ok())
			{
				primaryTextCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
																primaryTextTemplateCFString, inErrorCode);
			}
			else
			{
				primaryTextCFString = CFSTR("");
			}
			break;
		
		default:
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowCommandFailedPrimaryText, primaryTextTemplateCFString);
			if (stringResult.ok())
			{
				primaryTextCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
																primaryTextTemplateCFString, inErrorCode);
			}
			else
			{
				primaryTextCFString = CFSTR("");
			}
			break;
		}
		
		// generate help text
		if (inAssertion)
		{
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowCommandFailedHelpText, helpTextCFString);
			if (false == stringResult.ok())
			{
				helpTextCFString = CFSTR("");
			}
		}
		else
		{
			helpTextCFString = CFSTR("");
		}
		
		box = Alert_New();
		Alert_SetHelpButton(box, false);
		if (inAssertion)
		{
			Alert_SetParamsFor(box, kAlert_StyleOKCancel);
			{
				CFRetainRelease		buttonString;
				
				
				newButtonString(buttonString, kUIStrings_ButtonQuit);
				if (buttonString.exists())
				{
					Alert_SetButtonText(box, kAlert_ItemButton1, buttonString.returnCFStringRef());
				}
				newButtonString(buttonString, kUIStrings_ButtonContinue);
				if (buttonString.exists())
				{
					Alert_SetButtonText(box, kAlert_ItemButton2, buttonString.returnCFStringRef());
				}
			}
			Alert_SetType(box, kAlertStopAlert);
		}
		else
		{
			Alert_SetParamsFor(box, kAlert_StyleOK);
			{
				CFRetainRelease		buttonString;
				
				
				newButtonString(buttonString, kUIStrings_ButtonContinue);
				if (buttonString.exists())
				{
					Alert_SetButtonText(box, kAlert_ItemButton1, buttonString.returnCFStringRef());
				}
			}
			Alert_SetType(box, kAlertCautionAlert);
		}
		Alert_SetTextCFStrings(box, primaryTextCFString, helpTextCFString);
		
		Alert_Display(box);
		if (inAssertion)
		{
			doExit = (Alert_ItemHit(box) == kAlert_ItemButton1);
		}
		else
		{
			doExit = false;
		}
		Alert_Dispose(&box);
		
		if (doExit)
		{
			ExitToShell();
		}
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
		UNUSED_RETURN(OSStatus)NMRemove(gNotificationPtr);
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

	kAlert_ItemButton1
	kAlert_ItemButton2
	kAlert_ItemButton3

You may also pass nullptr to clear the button entirely, in
which case it is not displayed in the alert box.

The string is retained, so you do not have to be concerned
with your alert being affected if your string reference
goes away.

(1.0)
*/
void
Alert_SetButtonText		(AlertMessages_BoxRef	inAlert,
						 UInt16					inWhichButton,
						 CFStringRef			inNewText)
{
	AutoPool				_;
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
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
		case kAlert_ItemButton2:
			if (nil != alertPtr->targetWindowController)
			{
				[alertPtr->targetWindowController setSecondaryButtonText:(NSString*)inNewText];
			}
			stringPtr = &alertPtr->params.cancelText;
			break;
		
		case kAlert_ItemButton3:
			if (nil != alertPtr->targetWindowController)
			{
				[alertPtr->targetWindowController setTertiaryButtonText:(NSString*)inNewText];
			}
			stringPtr = &alertPtr->params.otherText;
			break;
		
		case kAlert_ItemButton1:
		default:
			if (nil != alertPtr->targetWindowController)
			{
				[alertPtr->targetWindowController setPrimaryButtonText:(NSString*)inNewText];
				// this button is always visible
			}
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
Alert_SetHelpButton		(AlertMessages_BoxRef	inAlert,
						 Boolean				inIsHelpButton)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->params.helpButton = inIsHelpButton;
	}
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
the application.  Another option is to also bounce the
Dock icon.  The last level is to do all of the above
things, and also display a message (specified with
Alert_SetNotificationMessage()) that tells the user
your application requires his or her attention.

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
Alert_SetParamsFor	(AlertMessages_BoxRef	inAlert,
					 UInt16					inAlertStyle)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	CFRetainRelease			tempCFString;
	
	
	if (nullptr != alertPtr)
	{
		switch (inAlertStyle)
		{
		case kAlert_StyleOK:
			newButtonString(tempCFString, kUIStrings_ButtonOK);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton1, tempCFString.returnCFStringRef());
			}
			Alert_SetButtonText(inAlert, kAlert_ItemButton2, nullptr);
			Alert_SetButtonText(inAlert, kAlert_ItemButton3, nullptr);
			alertPtr->params.defaultButton = kAlert_ItemButton1;
			alertPtr->params.cancelButton = 0;
			break;
		
		case kAlert_StyleCancel:
			newButtonString(tempCFString, kUIStrings_ButtonCancel);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton1, tempCFString.returnCFStringRef());
			}
			Alert_SetButtonText(inAlert, kAlert_ItemButton2, nullptr);
			Alert_SetButtonText(inAlert, kAlert_ItemButton3, nullptr);
			alertPtr->params.defaultButton = kAlert_ItemButton1;
			alertPtr->params.cancelButton = 0;
			break;
		
		case kAlert_StyleOKCancel:
		case kAlert_StyleOKCancel_CancelIsDefault:
			newButtonString(tempCFString, kUIStrings_ButtonOK);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton1, tempCFString.returnCFStringRef());
			}
			newButtonString(tempCFString, kUIStrings_ButtonCancel);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton2, tempCFString.returnCFStringRef());
			}
			Alert_SetButtonText(inAlert, kAlert_ItemButton3, nullptr);
			if (inAlertStyle == kAlert_StyleOKCancel_CancelIsDefault)
			{
				alertPtr->params.defaultButton = kAlert_ItemButton2;
				alertPtr->params.cancelButton = kAlert_ItemButton1;
			}
			else
			{
				alertPtr->params.defaultButton = kAlert_ItemButton1;
				alertPtr->params.cancelButton = kAlert_ItemButton2;
			}
			break;
		
		case kAlert_StyleYesNo:
		case kAlert_StyleYesNo_NoIsDefault:
			newButtonString(tempCFString, kUIStrings_ButtonYes);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton1, tempCFString.returnCFStringRef());
			}
			newButtonString(tempCFString, kUIStrings_ButtonNo);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton2, tempCFString.returnCFStringRef());
			}
			Alert_SetButtonText(inAlert, kAlert_ItemButton3, nullptr);
			if (inAlertStyle == kAlert_StyleYesNo_NoIsDefault)
			{
				alertPtr->params.defaultButton = kAlert_ItemButton2;
				alertPtr->params.cancelButton = kAlert_ItemButton1;
			}
			else
			{
				alertPtr->params.defaultButton = kAlert_ItemButton1;
				alertPtr->params.cancelButton = kAlert_ItemButton2;
			}
			break;
		
		case kAlert_StyleYesNoCancel:
		case kAlert_StyleYesNoCancel_NoIsDefault:
		case kAlert_StyleYesNoCancel_CancelIsDefault:
			newButtonString(tempCFString, kUIStrings_ButtonYes);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton1, tempCFString.returnCFStringRef());
			}
			newButtonString(tempCFString, kUIStrings_ButtonNo);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton2, tempCFString.returnCFStringRef());
			}
			newButtonString(tempCFString, kUIStrings_ButtonCancel);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton3, tempCFString.returnCFStringRef());
			}
			alertPtr->params.defaultButton = kAlert_ItemButton1;
			alertPtr->params.cancelButton = kAlert_ItemButton3;
			if (inAlertStyle == kAlert_StyleYesNoCancel_NoIsDefault)
			{
				alertPtr->params.defaultButton = kAlert_ItemButton2;
			}
			else if (inAlertStyle == kAlert_StyleYesNoCancel_CancelIsDefault)
			{
				alertPtr->params.defaultButton = kAlert_ItemButton3;
			}
			else
			{
				alertPtr->params.defaultButton = kAlert_ItemButton1;
			}
			alertPtr->params.cancelButton = kAlert_ItemButton3;
			break;
		
		case kAlert_StyleDontSaveCancelSave:
			newButtonString(tempCFString, kUIStrings_ButtonSave);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton1, tempCFString.returnCFStringRef());
			}
			newButtonString(tempCFString, kUIStrings_ButtonCancel);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton2, tempCFString.returnCFStringRef());
			}
			newButtonString(tempCFString, kUIStrings_ButtonDontSave);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton3, tempCFString.returnCFStringRef());
			}
			alertPtr->params.defaultButton = kAlert_ItemButton1;
			alertPtr->params.cancelButton = kAlert_ItemButton2;
			break;
		
		default:
			break;
		}
	}
}// SetParamsFor


/*!
Sets the text message (and optional help text underneath)
for an alert.  If you do not want a text to appear (no
help text, for example), pass an empty string.

(1.1)
*/
void
Alert_SetTextCFStrings	(AlertMessages_BoxRef	inAlert,
						 CFStringRef			inDialogText,
						 CFStringRef			inHelpText)
{
	AutoPool				_;
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		if (nil != alertPtr->targetWindowController)
		{
			[alertPtr->targetWindowController setDialogText:(NSString*)inDialogText];
			[alertPtr->targetWindowController setHelpText:(NSString*)inHelpText];
		}
		
		alertPtr->dialogTextCFString.setCFTypeRef(inDialogText);
		alertPtr->helpTextCFString.setCFTypeRef(inHelpText);
	}
}// SetTextCFStrings


/*!
Sets the title of an alert’s window.  The title may
not show up unless an alert is also made movable.
To remove an alert’s title, pass nullptr.

The given string is retained automatically and
released when the alert is destroyed or another
title is set.

(1.1)
*/
void
Alert_SetTitleCFString	(AlertMessages_BoxRef	inAlert,
						 CFStringRef			inNewTitle)
{
	AutoPool				_;
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		if (nil != alertPtr->targetWindowController)
		{
			[alertPtr->targetWindowController setTitleText:(NSString*)inNewTitle];
		}
		
		alertPtr->titleCFString.setCFTypeRef(inNewTitle);
	}
}// SetTitle


/*!
Use this method to set the icon for an alert.
A “plain” alert type has no icon.

(1.0)
*/
void
Alert_SetType	(AlertMessages_BoxRef	inAlert,
				 AlertType				inNewType)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->alertType = inNewType;
	}
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
The standard responder to a closed window-modal (sheet)
or modeless (notification window) alert, this routine simply
destroys the specified alert, ignoring any buttons clicked.

If for some reason a nullptr alert is given, this callback
has no effect.

This notifier is only appropriate if you display an alert with
a single button choice.  If you need to know which button was
chosen, call Alert_NewWindowModal() or Alert_NewModeless()
with your own notifier instead of this one, remembering to
call Alert_Dispose() within your custom notifier (or, just
call this routine).

(1.0)
*/
void
Alert_StandardCloseNotifyProc	(AlertMessages_BoxRef	inAlertThatClosed,
								 SInt16					UNUSED_ARGUMENT(inItemHit),
								 void*					UNUSED_ARGUMENT(inUserData))
{
	if (nullptr != inAlertThatClosed)
	{
		Alert_Dispose(&inAlertThatClosed);
	}
}// StandardCloseNotifyProc


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See Alert_New().

(1.1)
*/
My_AlertMessage::
My_AlertMessage()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
refValidator(REINTERPRET_CAST(this, AlertMessages_BoxRef), gAlertBoxValidRefs()),
selfRef(REINTERPRET_CAST(this, AlertMessages_BoxRef)),
asModal(nil),
asNotification(nil),
targetWindowController(nil),
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
itemHit(kAlert_ItemButton3),
isCompletelyModeless(false),
isSheet(false),
isSheetForWindowCloseWarning(false),
dialogWindow(nullptr),
parentWindow(nullptr),
parentNSWindow(nil),
buttonHICommandsHandler(nullptr),
sheetCloseNotifier(nullptr),
sheetCloseNotifierUserData(nullptr)
{
	assert_noerr(GetStandardAlertDefaultParams(&this->params, kStdCFStringAlertVersionOne));
	this->params.movable = true;
}// My_AlertMessage default constructor


/*!
Destructor.  See Alert_Dispose().

(1.1)
*/
My_AlertMessage::
~My_AlertMessage ()
{
	AutoPool		_;
	
	
	// clean up
	if (nil != this->targetWindowController)
	{
		[this->targetWindowController release], this->targetWindowController = nil;
	}
	if (nullptr != this->buttonHICommandsHandler)
	{
		RemoveEventHandler(this->buttonHICommandsHandler), this->buttonHICommandsHandler = nullptr;
	}
	if (nullptr != this->dialogWindow)
	{
		DisposeWindow(this->dialogWindow), this->dialogWindow = nullptr;
	}
}// My_AlertMessage destructor


/*!
Posts a general notification to the queue, with
no particular message.  So this grabs the user’s
attention for something other than an alert
message.  The exact form of notification depends
on the notification preferences specified via
Alert_SetNotificationPreferences().

(1.1)
*/
OSStatus
backgroundNotification ()
{
	OSStatus	result = noErr;
	
	
	if ((gNotificationIsBackgrounded) && (nullptr == gNotificationPtr))
	{
		gNotificationPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(NMRec)), NMRecPtr);
		if (nullptr == gNotificationPtr) result = memFullErr;
		else
		{
			gNotificationPtr->qLink = nullptr;
			gNotificationPtr->qType = nmType;
			gNotificationPtr->nmMark = 0; // index of menu item to mark
			gNotificationPtr->nmIcon = nullptr;
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
			
			// badge the application’s Dock tile with a caution icon, if that preference is set
			if (gNotificationPreferences >= kAlert_NotifyDisplayDiamondMark)
			{
				badgeApplicationDockTile();
			}
			
			result = NMInstall(gNotificationPtr); // TEMP - should use AEInteractWithUser here, instead...
		}
	}
	return result;
}// backgroundNotification


/*!
Overlays a caution icon in the corner of the
application’s Dock tile, to indicate an alert
condition.  This should only be done while the
application is backgrounded and an alert has
appeared.

(1.0)
*/
void
badgeApplicationDockTile ()
{
	CocoaBasic_SetDockTileToCautionOverlay();
}// badgeApplicationDockTile


/*!
Returns an object representing a false value, useful in many
simple flag methods.  The given item is not used.

(4.0)
*/
id
flagNo ()
{
	BOOL	result = NO;
	
	
	return [NSNumber numberWithBool:result];
}// flagNo


/*!
Responds to clicks in Carbon-based alert boxes.

DEPRECATED.  Use Cocoa.

(1.0)
*/
void
handleItemHitCarbon		(My_AlertMessagePtr		inPtr,
						 DialogItemIndex		inItemIndex)
{
	switch (inItemIndex)
	{
	case kAlert_ItemButton1:
		inPtr->itemHit = kAlert_ItemButton1;
		{
			Boolean		animate = true;
			
			
			if (inPtr->isSheetForWindowCloseWarning) animate = false;
			setCarbonAlertVisibility(inPtr, false/* visible */, animate/* animate */);
		}
		break;
	
	case kAlert_ItemButton2:
		inPtr->itemHit = kAlert_ItemButton2;
		setCarbonAlertVisibility(inPtr, false/* visible */, true/* animate */);
		break;
	
	case kAlert_ItemButton3:
		inPtr->itemHit = kAlert_ItemButton3;
		setCarbonAlertVisibility(inPtr, false/* visible */, true/* animate */);
		break;
	
	case kAlert_ItemHelpButton:
		inPtr->itemHit = kAlert_ItemHelpButton;
		setCarbonAlertVisibility(inPtr, false/* visible */, false/* animate */);
		break;
	
	case kAlert_ItemButtonNone:
	default:
		inPtr->itemHit = kAlert_ItemButtonNone;
		setCarbonAlertVisibility(inPtr, false/* visible */, false/* animate */);
		break;
	}
}// handleItemHitCarbon


/*!
Use this method to obtain a string containing
a standard button title (ex. Yes, No, Save,
etc.).  Call CFRelease() on the new string when
finished using it.

(1.0)
*/
void
newButtonString		(CFRetainRelease&			outStringStorage,
					 UIStrings_ButtonCFString	inButtonStringType)
{
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	CFStringRef			stringRef = nullptr;
	
	
	stringResult = UIStrings_Copy(inButtonStringType, stringRef);
	outStringStorage.setCFTypeRef(stringRef, true/* is retained */);
}// newButtonString


/*!
Performs last-minute steps to prepare for displaying an
alert message.

(2.5)
*/
void
prepareForDisplay	(My_AlertMessagePtr		inPtr)
{
	AutoPool	_;
	
	
	if (nil != inPtr->targetWindowController)
	{
		[inPtr->targetWindowController setHidesHelpButton:((inPtr->params.helpButton) ? NO : YES)];
		[inPtr->targetWindowController window]; // perform a dummy call to ensure the NIB is loaded before views are adjusted
		
		// perform any last-minute synchronizations between data stored here
		// and the state of the actual user interface elements; this is needed
		// because the old implementation unfortunately depended upon an OS
		// data structure for a lot of its settings; since it is not possible
		// to intercept every possible way that data could be initialized,
		// the best that can be done is to inspect the final values just
		// before the window appears and to adjust accordingly (all of this
		// will go away once the implementation becomes 100% Cocoa, but this
		// is a transitional problem)
		if (kAlertCautionAlert == inPtr->alertType)
		{
			[inPtr->targetWindowController setIconImageName:(NSString*)AppResources_ReturnCautionIconFilenameNoExtension()];
		}
		else
		{
			[inPtr->targetWindowController setIconImageName:nil];
		}
		[inPtr->targetWindowController adjustViews];
	}
}// prepareForDisplay


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the special key sequences dialog.

(3.0)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMy_AlertMessagePtr)
{
	OSStatus				result = eventNotHandledErr;
	My_AlertMessagePtr		ptr = REINTERPRET_CAST(inMy_AlertMessagePtr, My_AlertMessagePtr);
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
				handleItemHitCarbon(ptr, kAlert_ItemButton1);
				break;
			
			case kHICommandCancel:
				// Cancel button
				handleItemHitCarbon(ptr, kAlert_ItemButton2);
				break;
			
			case kCommandAlertOtherButton:
				// 3rd button
				handleItemHitCarbon(ptr, kAlert_ItemButton3);
				break;
			
			case kCommandContextSensitiveHelp:
				// help button
				handleItemHitCarbon(ptr, kAlert_ItemHelpButton);
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the window is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Shows or hides an alert window, optionally with animation
appropriate to the type of alert.

DEPRECATED.  Only needed for Carbon windows, and only for
sheets (as all other styles have moved to Cocoa).

(1.0)
*/
void
setCarbonAlertVisibility	(My_AlertMessagePtr		inPtr,
							 Boolean				inIsVisible,
							 Boolean				inAnimate)
{
	assert(nil == inPtr->targetWindowController);
	assert(inPtr->isSheet);
	
	if (inIsVisible)
	{
		ShowSheetWindow(inPtr->dialogWindow, inPtr->parentWindow);
		CocoaBasic_MakeKeyWindowCarbonUserFocusWindow();
	}
	else
	{
		unless (inAnimate)
		{
			// hide the parent to hide the sheet immediately
			HideWindow(inPtr->parentWindow);
		}
		HideSheetWindow(inPtr->dialogWindow);
		
		AlertMessages_InvokeCloseNotifyProc(inPtr->sheetCloseNotifier, inPtr->selfRef, inPtr->itemHit,
											inPtr->sheetCloseNotifierUserData), inPtr = nullptr;
		// WARNING: after this point the alert has been destroyed!
	}
}// setCarbonAlertVisibility


/*!
Displays a dialog box on the screen that reflects
the parameters of an Interface Library Alert.  Any
errors that occur, preventing the dialog from
displaying, will be returned.  The item number hit
is stored using the given alert reference, and can
be obtained by calling Alert_ItemHit().

On Mac OS X, if the alert is in fact window-modal,
this routine will return immediately WITHOUT having
completed handling the alert.  Instead, the alert’s
notifier is invoked whenever the user closes the
alert.

This method is invoked by Alert_Display().

Legacy, used for the Carbon implementation only.

(1.0)
*/
OSStatus
standardAlert	(My_AlertMessagePtr		inAlert,
				 AlertType				inAlertType,
				 CFStringRef			inDialogText,
				 CFStringRef			inHelpText)
{
	My_AlertMessagePtr		ptr = inAlert;
	OSStatus				result = noErr;
	
	
	assert((ptr->isSheet) && (nil == ptr->targetWindowController));
	
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
										CFSTR("AlertMessages"), ptr->isSheet
																? CFSTR("Sheet")
																: (ptr->isCompletelyModeless)
																	? CFSTR("Modeless")
																	: CFSTR("Dialog"))
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
		
		// set up fonts (since NIBs are too crappy to support this in older Mac OS X versions);
		// note that completely-modeless alerts are generally smaller, so fonts are shrunk for them
		Localization_SetControlThemeFontInfo(ptr->textTitle, (ptr->isCompletelyModeless)
																? kThemeAlertHeaderFont
																: USHRT_MAX/* special font - see Localization_UseThemeFont() */);
		Localization_SetControlThemeFontInfo(ptr->textMain, (ptr->isCompletelyModeless)
																? kThemeSmallEmphasizedSystemFont
																: kThemeAlertHeaderFont);
		Localization_SetControlThemeFontInfo(ptr->textHelp, kThemeSmallSystemFont);
		
		// set the window title to be the application name, on Mac OS X
	#if 0
		{
			CFStringRef		string;
			
			
			Localization_GetCurrentApplicationNameAsCFString(&string);
			SetWindowTitleWithCFString(ptr->dialogWindow, string);
		}
	#endif
		
		// if the alert should have a title, give it one
		if (ptr->titleCFString.exists())
		{
			SetControlTextWithCFString(ptr->textTitle, ptr->titleCFString.returnCFStringRef());
		}
		
		//
		// set up dialog items to reflect alert parameters
		//
		
		if (nullptr != ptr->params.defaultText)
		{
			// if the alert is configured to use the default title (“OK”), copy the appropriate string resource
			if (ptr->params.defaultText == REINTERPRET_CAST(kAlertDefaultOKText, CFStringRef))
			{
				CFRetainRelease		myString;
				
				
				newButtonString(myString, kUIStrings_ButtonOK);
				SetControlTitleWithCFString(ptr->buttonDefault, myString.returnCFStringRef());
			}
			else SetControlTitleWithCFString(ptr->buttonDefault, ptr->params.defaultText);
		}
		else
		{
			CFRetainRelease		myString;
			
			
			newButtonString(myString, kUIStrings_ButtonOK);
			SetControlTitleWithCFString(ptr->buttonDefault, myString.returnCFStringRef());
		}
		
		if (nullptr != ptr->params.cancelText)
		{
			// if the alert is configured to use the default title (“Cancel”), copy the appropriate string resource
			if (ptr->params.cancelText == REINTERPRET_CAST(kAlertDefaultCancelText, CFStringRef))
			{
				CFRetainRelease		myString;
				
				
				newButtonString(myString, kUIStrings_ButtonCancel);
				SetControlTitleWithCFString(ptr->buttonCancel, myString.returnCFStringRef());
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
			// if the alert is configured to use the default title (“Don’t Save”), copy the appropriate string resource
			if (ptr->params.otherText == REINTERPRET_CAST(kAlertDefaultOtherText, CFStringRef))
			{
				CFRetainRelease		myString;
				
				
				newButtonString(myString, kUIStrings_ButtonDontSave);
				SetControlTitleWithCFString(ptr->buttonOther, myString.returnCFStringRef());
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
				kSpacingBetweenTextItems = 10		// width in pixels vertically separating dialog text and help text
			};
			SInt16		textExpanseV = 0;
			SInt16		totalExpanseV = 0; // initialized to vertical position of top edge of dialog text item
			HIRect		floatBounds;
			Rect		controlRect;
			
			
			// use the actual top edge as defined in the NIB
			HIViewGetFrame(ptr->textTitle, &floatBounds);
			totalExpanseV = STATIC_CAST(floatBounds.origin.y, SInt16);
			
			// size and arrange title text, if present
			if (ptr->titleCFString.exists())
			{
				GetControlBounds(ptr->textTitle, &controlRect);
				//(OSStatus)SetControlData(ptr->textTitle, kControlEntireControl, kControlStaticTextCFStringTag,
				//							STATIC_CAST(PLstrlen(ptr->title) * sizeof(UInt8), Size), ptr->title + 1);
				textExpanseV = 20; // should be fixed to properly calculate dimensions, but alas, I’ve been lazy
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
				UNUSED_RETURN(OSStatus)SetWindowBounds(ptr->dialogWindow, kWindowContentRgn, &windowBounds);
			}
			
			// arrange icons and main text areas
			{
				// swap the icon and text if necessary
				Localization_HorizontallyPlaceControls(ptr->mainIcon, ptr->textMain);
				
				// for right-to-left localization only, the alert icon and text will switch places
				if (false == Localization_IsLeftToRight())
				{
					Rect	mainIconRect;
					
					
					GetControlBounds(ptr->mainIcon, &mainIconRect);
					GetControlBounds(ptr->applicationIcon, &controlRect);
					controlRect.right = mainIconRect.left + (controlRect.right - controlRect.left);
					controlRect.left = mainIconRect.left;
					SetControlBounds(ptr->applicationIcon, &controlRect);
				}
				
				// shift the title text and help text to the same horizontal position as the dialog text moved to
				if (nullptr != inHelpText)
				{
					Rect	dialogTextRect;
					Rect	helpTextRect;
					
					
					GetControlBounds(ptr->textMain, &dialogTextRect);
					GetControlBounds(ptr->textHelp, &helpTextRect);
					MoveControl(ptr->textHelp, dialogTextRect.left, helpTextRect.top);
					GetControlBounds(ptr->textTitle, &helpTextRect);
					MoveControl(ptr->textTitle, dialogTextRect.left, helpTextRect.top);
				}
			}
			
			// move the dialog buttons vertically to their proper place in the new dialog
			{
				ControlRef		buttonControls[2] = { ptr->buttonOther, nullptr };
				
				
				// use this “sneaky” approach to correctly set up the vertical component of the third button
				Localization_ArrangeButtonArray(buttonControls, 1);
				
				// auto-arrange the buttons in the OK and Cancel positions
				buttonControls[0] = ptr->buttonDefault;
				buttonControls[1] = ptr->buttonCancel;
				Localization_ArrangeButtonArray(buttonControls, 2);
				
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
			
			// move the help button, if it’s supposed to be there
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
		
		// absorb keyboard events to avoid accidental button activations,
		// e.g. if user presses Return just as a message appears, this
		// should not kill the message
		FlushEvents(keyDown, 0/* stop mask */);
		
		// get the rectangle where the alert box is supposed to be, and animate it into position (with Mac OS 8.5 and beyond)
		setCarbonAlertVisibility(ptr, true/* visible */, true/* animate */);
		
		// advance focus (the default button can already be easily hit with
		// the keyboard, so focusing the next button makes it equally easy
		// for the user to choose that option with a key)
		{
			if (isButton2)
			{
				UNUSED_RETURN(OSStatus)DialogUtilities_SetKeyboardFocus(ptr->buttonCancel);
			}
			else if (isButton3)
			{
				UNUSED_RETURN(OSStatus)DialogUtilities_SetKeyboardFocus(ptr->buttonOther);
			}
		}
		
		unless ((ptr->isSheet) || (ptr->isCompletelyModeless))
		{
			ShowWindow(ptr->dialogWindow);
			EventLoop_SelectOverRealFrontWindow(ptr->dialogWindow);
			RunAppModalLoopForWindow(ptr->dialogWindow);
			
			Alert_ServiceNotification(); // just in case it wasn’t removed properly
			setCarbonAlertVisibility(ptr, false/* visible */, true/* animate */);
		}
	}
	else result = memPCErr;
	
	return result;
}// standardAlert

} // anonymous namespace


@implementation AlertMessages_ModalWindowController


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"AlertMessagesModalCocoa"];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds to the close of a window-modal alert.

(4.1)
*/
- (void)
sheetDidEnd:(NSWindow*)		aSheet
returnCode:(int)			aReturnCode
contextInfo:(void*)			aBoxRef
{
#pragma unused(aSheet, aReturnCode, aBoxRef)
	//AlertMessages_BoxRef	asBox = REINTERPRET_CAST(aBoxRef, AlertMessages_BoxRef);
	
	
	// nothing is needed because the buttons are already bound to actions
}// sheetDidEnd:returnCode:contextInfo:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
}// windowDidLoad


@end // AlertMessages_ModalWindowController


@implementation AlertMessages_NotificationWindowController


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"AlertMessagesModelessCocoa"];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark AlertMessages_WindowController


/*!
This implementation sets a smaller font size than is normally
used for alerts.

(4.0)
*/
- (void)
setUpFonts
{
	[super setUpFonts];
	//[titleTextUI setFont:[NSFont userFontOfSize:18.0/* arbitrary */]];
	[dialogTextUI setFont:[NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]]];
	[helpTextUI setFont:[NSFont systemFontOfSize:[NSFont labelFontSize]]];
}// setUpFonts


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
}// windowDidLoad


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"Notifications";
}// windowFrameAutosaveName


@end // AlertMessages_NotificationWindowController


@implementation AlertMessages_WindowController


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithWindowNibName:(NSString*)	aName
{
	self = [super initWithWindowNibName:aName];
	if (nil != self)
	{
		self->dataPtr = nullptr;
		
		self->titleText = [[NSString alloc] initWithString:@""];
		self->dialogText = [[NSString alloc] initWithString:@""];
		self->helpText = [[NSString alloc] initWithString:@""];
		self->primaryButtonText = [[NSString alloc] initWithString:@""];
		self->secondaryButtonText = [[NSString alloc] initWithString:@""];
		self->tertiaryButtonText = [[NSString alloc] initWithString:@""];
		self->iconImageName = nil;
		
		self->hidesHelpButton = NO;
		self->hidesSecondaryButton = YES;
		self->hidesTertiaryButton = YES;
	}
	return self;
}// initWithWindowNibName:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[titleText release];
	[dialogText release];
	[helpText release];
	[primaryButtonText release];
	[secondaryButtonText release];
	[tertiaryButtonText release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Invoke this just before displaying the alert, to finalize the
positions and sizes of both the subviews and the window itself.
For example, if there is no title text, the text below it is
moved up to occupy the same space; and if there is not enough
room to fit the remaining text, the text and the window are
expanded.

(4.0)
*/
- (void)
adjustViews
{
	// make the 3rd button as big as it needs to be
#if 0
	[tertiaryButtonUI sizeToFit];
#else
	UNUSED_RETURN(UInt16)Localization_AutoSizeNSButton(tertiaryButtonUI);
#endif
	
	// hide the title text if there is none, and reclaim the space
	if (0 == [titleText length])
	{
		// include both the height of the title frame and the space between
		// the dialog text and title text frames
		float		dialogTextY = [dialogTextUI frame].origin.y;
		float		dialogTextHeight = [dialogTextUI frame].size.height;
		float		titleTextY = [titleTextUI frame].origin.y;
		float		titleTextHeight = [titleTextUI frame].size.height;
		float		titleTextExpanseY = titleTextHeight + (titleTextY - (dialogTextY + dialogTextHeight));
		NSRect		newFrame = NSZeroRect;
		NSView*		containerView = nil;
		
		
		[titleTextUI setHidden:YES];
		
		containerView = dialogTextUI;
		newFrame = [containerView frame];
		newFrame.origin.y += titleTextExpanseY;
		[containerView setFrame:newFrame];
		
		containerView = helpTextUI;
		newFrame = [containerView frame];
		newFrame.origin.y += titleTextExpanseY;
		[containerView setFrame:newFrame];
		
		[self deltaSizeWindow:-titleTextExpanseY];
	}
	else
	{
		[titleTextUI sizeToFit];
	}
	
	// hide the help text if there is none, and reclaim the space
	if (0 == [helpText length])
	{
		// include both the height of the text frame and the space between
		// the dialog text and help text frames
		float		dialogTextY = [dialogTextUI frame].origin.y;
		float		helpTextY = [helpTextUI frame].origin.y;
		float		helpTextHeight = [helpTextUI frame].size.height;
		float		helpTextExpanseY = helpTextHeight + (dialogTextY - (helpTextY + helpTextHeight));
		
		
		[helpTextUI setHidden:YES];
		[self deltaSizeWindow:-helpTextExpanseY];
	}
	
	// focus a non-default button if possible
	if (NO == [secondaryButtonUI isHidden])
	{
		UNUSED_RETURN(BOOL)[[self window] makeFirstResponder:secondaryButtonUI];
	}
	else if (NO == [tertiaryButtonUI isHidden])
	{
		UNUSED_RETURN(BOOL)[[self window] makeFirstResponder:tertiaryButtonUI];
	}
	else if (NO == [helpButtonUI isHidden])
	{
		UNUSED_RETURN(BOOL)[[self window] makeFirstResponder:helpButtonUI];
	}
	else
	{
		UNUSED_RETURN(BOOL)[[self window] makeFirstResponder:primaryButtonUI];
	}
	// INCOMPLETE
}// adjustViews


/*!
This should call "setFont:" on the "dialogTextUI", "helpTextUI"
and "titleTextUI" elements so that they are initialized to
appropriate typefaces and sizes.  The default implementation
follows standard alert requirements, but a subclass may wish to
use smaller fonts, e.g. for modeless notification windows.

(4.0)
*/
- (void)
setUpFonts
{
	//[titleTextUI setFont:[NSFont userFontOfSize:20.0/* arbitrary */]];
	[dialogTextUI setFont:[NSFont boldSystemFontOfSize:[NSFont systemFontSize]]];
	[helpTextUI setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
}// setUpFonts


#pragma mark Accessors


/*!
Pointer to auxiliary data (historical).  Its actual type is a
pointer to the internal structure, My_AlertMessage.

(4.0)
*/
- (void*)
dataPtr
{
	return dataPtr;
}
- (void)
setDataPtr:(void*)		inDataPtr
{
	dataPtr = inDataPtr;
}// setDataPtr:


/*!
Accessor for the text displayed in normal print by the window.

(4.0)
*/
- (NSString*)
dialogText
{
	return [[dialogText copy] autorelease];
}
+ (id)
autoNotifyOnChangeToDialogText
{
	return flagNo();
}
- (void)
setDialogText:(NSString*)	aString
{
	[self setStringProperty:&dialogText withName:@"dialogText" toValue:aString];
	[self setDisplayedDialogText:self->dialogText];
}// setDialogText:


/*!
Accessor for the text displayed in small print by the window.

(4.0)
*/
- (NSString*)
helpText
{
	return [[helpText copy] autorelease];
}
+ (id)
autoNotifyOnChangeToHelpText
{
	return flagNo();
}
- (void)
setHelpText:(NSString*)		aString
{
	[self setStringProperty:&helpText withName:@"helpText" toValue:aString];
	[self setDisplayedHelpText:self->helpText];
}// setHelpText:


/*!
Whether or not the alert window has a help button showing.

(4.0)
*/
- (BOOL)
hidesHelpButton
{
	return hidesHelpButton;
}
- (void)
setHidesHelpButton:(BOOL)	flag
{
	hidesHelpButton = flag;
}// setHidesHelpButton:


/*!
Whether or not the alert window has a secondary button showing.

(4.0)
*/
- (BOOL)
hidesSecondaryButton
{
	return hidesSecondaryButton;
}
- (void)
setHidesSecondaryButton:(BOOL)	flag
{
	hidesSecondaryButton = flag;
}// setHidesSecondaryButton:


/*!
Whether or not the alert window has a tertiary button showing.

(4.0)
*/
- (BOOL)
hidesTertiaryButton
{
	return hidesTertiaryButton;
}
- (void)
setHidesTertiaryButton:(BOOL)	flag
{
	hidesTertiaryButton = flag;
}// setHidesTertiaryButton:


/*!
Accessor for the name of the image to use as the icon.
This is only the base image, which is automatically given
an application icon badge before rendering.  If set to an
empty string, then the icon is a large application icon
that has no badge.

(4.0)
*/
- (NSString*)
iconImageName
{
	return [[iconImageName copy] autorelease];
}
+ (id)
autoNotifyOnChangeToIconImageName
{
	return flagNo();
}
- (void)
setIconImageName:(NSString*)	aString
{
	[self setStringProperty:&iconImageName withName:@"iconImageName" toValue:aString];
	[mainIconUI setImage:[self imageForIconImageName:aString]];
}// setIconImageName:


/*!
Accessor for the title of the first (and likely default) button.

(4.0)
*/
- (NSString*)
primaryButtonText
{
	return [[primaryButtonText copy] autorelease];
}
+ (id)
autoNotifyOnChangeToPrimaryButtonText
{
	return flagNo();
}
- (void)
setPrimaryButtonText:(NSString*)	aString
{
	[self setStringProperty:&primaryButtonText withName:@"primaryButtonText" toValue:aString];
}// setPrimaryButtonText:


/*!
Accessor for the title of the second (and likely Cancel) button.

(4.0)
*/
- (NSString*)
secondaryButtonText
{
	return [[secondaryButtonText copy] autorelease];
}
+ (id)
autoNotifyOnChangeToSecondaryButtonText
{
	return flagNo();
}
- (void)
setSecondaryButtonText:(NSString*)	aString
{
	[self setStringProperty:&secondaryButtonText withName:@"secondaryButtonText" toValue:aString];
	[self setHidesSecondaryButton:(0 == [aString length])];
}// setSecondaryButtonText:


/*!
Accessor for the title of the third button.

(4.0)
*/
- (NSString*)
tertiaryButtonText
{
	return [[tertiaryButtonText copy] autorelease];
}
+ (id)
autoNotifyOnChangeToTertiaryButtonText
{
	return flagNo();
}
- (void)
setTertiaryButtonText:(NSString*)	aString
{
	[self setStringProperty:&tertiaryButtonText withName:@"tertiaryButtonText" toValue:aString];
	[self setHidesTertiaryButton:(0 == [aString length])];
}// setTertiaryButtonText:


/*!
Accessor for the text displayed in large print by the window.

(4.0)
*/
- (NSString*)
titleText
{
	return [[titleText copy] autorelease];
}
+ (id)
autoNotifyOnChangeToTitleText
{
	return flagNo();
}
- (void)
setTitleText:(NSString*)	aString
{
	[self setStringProperty:&titleText withName:@"titleText" toValue:aString];
}// setTitleText:


#pragma mark Actions


/*!
Responds to a click in the help button.

(4.0)
*/
- (IBAction)
performHelpAction:(id)		sender
{
#pragma unused(sender)
	[self didFinishAlertWithButton:kAlert_ItemHelpButton fromEvent:YES];
}// performHelpAction:


/*!
Responds to a click in the first (and likely default) button.

(4.0)
*/
- (IBAction)
performPrimaryAction:(id)	sender
{
#pragma unused(sender)
	[self didFinishAlertWithButton:kAlert_ItemButton1 fromEvent:YES];
}// performPrimaryAction:


/*!
Responds to a click in the second (typically Cancel) button.

(4.0)
*/
- (IBAction)
performSecondaryAction:(id)		sender
{
#pragma unused(sender)
	[self didFinishAlertWithButton:kAlert_ItemButton2 fromEvent:YES];
}// performSecondaryAction:


/*!
Responds to a click in the third button.

(4.0)
*/
- (IBAction)
performTertiaryAction:(id)		sender
{
#pragma unused(sender)
	[self didFinishAlertWithButton:kAlert_ItemButton3 fromEvent:YES];
}// performTertiaryAction:


#pragma mark NSKeyValueObservingCustomization


/*!
Returns true for keys that manually notify observers
(through "willChangeValueForKey:", etc.).

(4.0)
*/
+ (BOOL)
automaticallyNotifiesObserversForKey:(NSString*)	theKey
{
	BOOL	result = YES;
	SEL		flagSource = NSSelectorFromString([[self class] selectorNameForKeyChangeAutoNotifyFlag:theKey]);
	
	
	if (NULL != class_getClassMethod([self class], flagSource))
	{
		// See selectorToReturnKeyChangeAutoNotifyFlag: for more information on the form of the selector.
		result = [[self performSelector:flagSource] boolValue];
	}
	else
	{
		result = [super automaticallyNotifiesObserversForKey:theKey];
	}
	return result;
}// automaticallyNotifiesObserversForKey:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	assert(nil != helpButtonUI);
	assert(nil != tertiaryButtonUI);
	assert(nil != secondaryButtonUI);
	assert(nil != primaryButtonUI);
	assert(nil != titleTextUI);
	assert(nil != dialogTextUI);
	assert(nil != helpTextUI);
	assert(nil != mainIconUI);
	
	// initialize views
	[self setUpFonts];
	[dialogTextUI setAlignment:NSNaturalTextAlignment];
	[dialogTextUI setDrawsBackground:NO];
	[dialogTextUI setEditable:NO];
	[self setDisplayedDialogText:self->dialogText];
	[helpTextUI setAlignment:NSNaturalTextAlignment];
	[helpTextUI setDrawsBackground:NO];
	[helpTextUI setEditable:NO];
	[self setDisplayedHelpText:self->helpText];
	[mainIconUI setImage:[self imageForIconImageName:[self iconImageName]]];
}// windowDidLoad


@end // AlertMessages_WindowController


@implementation AlertMessages_WindowController (AlertMessages_WindowControllerInternal)


/*!
Aborts a currently-displayed alert without choosing any
particular button.  If a callback was given it is called
with "kAlert_ItemButtonNone" as the hit item.

(4.1)
*/
- (void)
abortAlert
{
	[self didFinishAlertWithButton:kAlert_ItemButtonNone fromEvent:NO];
}// abortAlert


/*!
Changes the height of the window by the specified amount.
Note that the buttons will also move, because they are
configured in the NIB to be attached to the bottom edge.

(4.0)
*/
- (void)
deltaSizeWindow:(float)		deltaV
{
	NSRect		frame = NSZeroRect;
	
	
	frame = [[self window] frame];
	frame.size.height += deltaV;
	[[self window] setFrame:frame display:YES];
}// deltaSizeWindow:


/*!
Responds to a click in a button.  Modeless and window-modal
alerts will be destroyed at this time; application-modal
alerts are assumed to do all their cleanup synchronously.

(4.1)
*/
- (void)
didFinishAlertWithButton:(unsigned int)		aButton
fromEvent:(BOOL)							anEventSource
{
	My_AlertMessage*	alertPtr = (My_AlertMessage*)[self dataPtr];
	
	
	alertPtr->itemHit = aButton;
	
	// now close the alert in an appropriate way
	if (alertPtr->isSheet)
	{
		if ((alertPtr->isSheetForWindowCloseWarning) && (kAlert_ItemButton1 == aButton))
		{
			// suppress sheet-close animation
			// UNIMPLEMENTED; Cocoa does not really allow this
		}
		[NSApp endSheet:[alertPtr->targetWindowController window]];
		[alertPtr->targetWindowController close];
		AlertMessages_InvokeCloseNotifyProc(alertPtr->sheetCloseNotifier, alertPtr->selfRef, alertPtr->itemHit,
											alertPtr->sheetCloseNotifierUserData), alertPtr = nullptr;
		// WARNING: after this point the alert has been destroyed!
	}
	else if (alertPtr->isCompletelyModeless)
	{
		[alertPtr->targetWindowController close];
		AlertMessages_InvokeCloseNotifyProc(alertPtr->sheetCloseNotifier, alertPtr->selfRef, alertPtr->itemHit,
											alertPtr->sheetCloseNotifierUserData), alertPtr = nullptr;
		// WARNING: after this point the alert has been destroyed!
	}
	else
	{
		if (anEventSource)
		{
			[NSApp stopModal];
		}
		else
		{
			[NSApp abortModal];
		}
	}
}// didFinishAlertWithButton:


/*!
Returns an (autoreleased) image for an icon that uses the
image with the given name as the base.  If the name is
not empty, an application icon is superimposed over a copy
of the image before returning; otherwise, the returned
image is the application icon only.

(4.0)
*/
- (NSImage*)
imageForIconImageName:(NSString*)	anImageName
{
	NSImage*	appIconImage = [NSImage imageNamed:(NSString*)AppResources_ReturnBundleIconFilenameNoExtension()];
	NSImage*	result = appIconImage; // most alerts just use the application icon
	
	
	if ((nil != anImageName) && ([anImageName length] > 0))
	{
		result = [[[NSImage imageNamed:anImageName] copy] autorelease];
		if (nil == result)
		{
			// failed to find image
			Console_Warning(Console_WriteValueCFString, "failed to find icon for image name", (CFStringRef)anImageName);
			result = appIconImage;
		}
		else
		{
			// valid image; now superimpose a small application icon
			// in the corner, emulating standard behavior
			NSSize		imageSize = [result size];
			
			
			imageSize.width /= 2.0;
			imageSize.height /= 2.0;
			[result lockFocus];
			[appIconImage drawInRect:NSMakeRect(imageSize.width/* x coordinate */, 0/* y coordinate */,
												imageSize.width/* width */, imageSize.height/* height */)
										fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
			[result unlockFocus];
		}
	}
	
	return result;
}// imageForIconImageName:


/*!
Updates the dialog text in the user interface and also
resizes and rearranges views (and if necessary, the window)
to leave just enough room for the new string.

(4.0)
*/
- (void)
setDisplayedDialogText:(NSString*)	aString
{
	if (nil != dialogTextUI)
	{
		NSView*		containerView = nil;
		NSRect		oldFrame = [dialogTextUI frame];
		NSRect		newFrame = oldFrame;
		float		deltaV = 0;
		
		
		[dialogTextUI setString:aString];
		[dialogTextUI sizeToFit];
		[dialogTextUI scrollRangeToVisible:NSMakeRange([aString length] - 1, 1)];
		newFrame = [dialogTextUI frame];
		deltaV = (newFrame.size.height - oldFrame.size.height);
		
		containerView = dialogTextUI;
		newFrame = [containerView frame];
		newFrame.origin.y -= deltaV;
		newFrame.size.height += deltaV;
		[containerView setFrame:newFrame];
		
		containerView = helpTextUI;
		newFrame = [containerView frame];
		newFrame.origin.y -= deltaV;
		[containerView setFrameOrigin:newFrame.origin];
		
		[self deltaSizeWindow:deltaV];
	}
}// setDisplayedDialogText:


/*!
Updates the help text in the user interface and also
resizes and rearranges views (and if necessary, the window)
to leave just enough room for the new string.

(4.0)
*/
- (void)
setDisplayedHelpText:(NSString*)	aString
{
	if (nil != helpTextUI)
	{
		NSView*		containerView = nil;
		NSRect		oldFrame = [helpTextUI frame];
		NSRect		newFrame = oldFrame;
		float		deltaV = 0;
		
		
		[helpTextUI setString:aString];
		[helpTextUI sizeToFit];
		[helpTextUI scrollRangeToVisible:NSMakeRange([aString length] - 1, 1)];
		newFrame = [helpTextUI frame];
		deltaV = (newFrame.size.height - oldFrame.size.height);
		
		containerView = helpTextUI;
		newFrame = [containerView frame];
		newFrame.origin.y -= deltaV;
		newFrame.size.height += deltaV;
		[containerView setFrame:newFrame];
		
		[self deltaSizeWindow:deltaV];
	}
}// setDisplayedHelpText:


/*!
A helper to make string-setters less cumbersome to write.

(4.0)
*/
- (void)
setStringProperty:(NSString**)		propertyPtr
withName:(NSString*)				propertyName
toValue:(NSString*)					aString
{
	if (aString != *propertyPtr)
	{
		[self willChangeValueForKey:propertyName];
		
		if (nil == aString)
		{
			*propertyPtr = [@"" retain];
		}
		else
		{
			[*propertyPtr autorelease];
			*propertyPtr = [aString copy];
		}
		
		[self didChangeValueForKey:propertyName];
	}
}// setStringProperty:withName:toValue:


@end // AlertMessages_WindowController (AlertMessages_WindowControllerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
