/*!	\file AlertMessages.mm
	\brief Handles Cocoa-based alerts and notifications.
	
	Note that this is in transition from Carbon to Cocoa,
	and is not yet taking advantage of most of Cocoa.
*/
/*###############################################################

	Interface Library
	© 1998-2022 by Kevin Grant
	
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
#import <objc/objc-runtime.h>
@import ApplicationServices;
@import CoreServices;

// library includes
#import <CFRetainRelease.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlocks.h>
#import <Registrar.template.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "GenericDialog.h"
#import "HelpSystem.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

/*!
Choose a value that will not match something returned
by "requestUserAttention:".
*/
NSInteger const		kInvalidUserAttentionRequestID = -1234;

} // anonymous namespace

#pragma mark Types
namespace {

typedef MemoryBlockReferenceTracker< AlertMessages_BoxRef >		My_RefTracker;
typedef Registrar< AlertMessages_BoxRef, My_RefTracker >		My_RefRegistrar;

struct My_AlertMessage
{
public:
	My_AlertMessage ();
	
	My_RefRegistrar			refValidator;		//!< ensures this reference is recognized as a valid one
	AlertMessages_BoxRef	selfRef;
	AlertMessages_VC*		viewController;		//!< panel for icon and text content of alert message; owned by "genericDialog"
	GenericDialog_Wrap		genericDialog;		//!< wraps alert panel in a window, with buttons, and displays it
	Alert_IconID			iconID;
	NSWindow*				parentNSWindow;
};
typedef My_AlertMessage*	My_AlertMessagePtr;

typedef MemoryBlockPtrLocker< AlertMessages_BoxRef, My_AlertMessage >		My_AlertPtrLocker;
typedef LockAcquireRelease< AlertMessages_BoxRef, My_AlertMessage >			My_AlertAutoLocker;
typedef MemoryBlockReferenceLocker< AlertMessages_BoxRef, My_AlertMessage >	My_AlertReferenceLocker;

} // anonymous namespace


/*!
A subclass of NSImageView that allows the user to drag
the window when it is clicked.  Used to
*/
@interface AlertMessages_WindowDraggingIcon : NSImageView //{

// accessors
	//! This subclass exists to better support movable alert
	//! windows.  If this property is set to YES then the
	//! user can drag the window even if the initial click
	//! is on the alert icon.
	@property (assign) BOOL
	mouseDownCanMoveWindow;

@end //}


/*!
Private properties.
*/
@interface AlertMessages_VC () //{

// accessors
	//! The primary text in the alert message.
	@property (strong) IBOutlet NSTextField*
	dialogTextUI;
	//! The secondary (small) text in the alert message.  Hidden if empty.
	@property (strong) IBOutlet NSTextField*
	helpTextUI;
	//! Generic description of icon (as opposed to image name).
	@property (assign) Alert_IconID
	iconID;
	//! The name of the NSImage assigned to the icon view.
	@property (strong) NSString*
	iconImageName;
	//! The frame that fits all the title, dialog and help text exactly.
	@property (assign) NSSize
	idealFrameSize;
	//! The size of the icon, which sets the minimum height of the content.
	@property (assign) NSSize
	idealIconSize;
	//! The icon image view, which can also drag the window when clicked.
	@property (strong) IBOutlet AlertMessages_WindowDraggingIcon*
	mainIconUI;
	//! A large (and usually short) top line for the alert.  Hidden if empty.
	@property (strong) IBOutlet NSTextField*
	titleTextUI;

@end //}


/*!
The private class interface.
*/
@interface AlertMessages_VC (AlertMessages_VCInternal) //{

// new methods
	- (NSImage*)
	imageForIconImageName:(NSString*)_;
	- (void)
	setUpFonts;

@end //}


#pragma mark Variables
namespace {

Boolean						gIsAnimationAllowed = false;
Boolean						gNotificationIsBackgrounded = false;
Boolean						gNotificationIsIconBadged = false;
Boolean						gUseSpeech = false;
UInt16						gNotificationPreferences = kAlertMessages_NotificationTypeMarkDockIcon;
NSInteger					gNotificationRequestID = kInvalidUserAttentionRequestID;
My_AlertPtrLocker&			gAlertPtrLocks ()	{ static My_AlertPtrLocker x; return x; }
My_AlertReferenceLocker&	gAlertRefLocks ()	{ static My_AlertReferenceLocker x; return x; }
My_RefTracker&				gAlertBoxValidRefs ()	{ static My_RefTracker x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean					backgroundNotification				();
void					badgeApplicationDockTile			();
void					newButtonString						(CFRetainRelease&, UIStrings_ButtonCFString);
GenericDialog_ItemID	returnGenericItemIDForAlertItem		(UInt16);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates an application-modal alert without displaying
it (this gives you the opportunity to specify its
characteristics using other methods from this module).
When you are finished with the alert, use the method
Alert_Release().  (It is recommended that you make use
of the AlertMessages_BoxWrap class for this however.)

Generally, you call this routine to create an alert,
then call other methods to set up the attributes of
the new alert (title, help button, dialog filter
procedure, etc.).  When the alert is configured, use
Alert_Display() or a similar method to display it.

By default, all new alerts are marked as movable,
with no help button, with a single OK button and no
other buttons.

IMPORTANT:	If unsuccessful, nullptr is returned.

(2016.03)
*/
AlertMessages_BoxRef
Alert_NewApplicationModal ()
{
	AlertMessages_BoxRef	result = Alert_NewWindowModal(nullptr/* parent window */);
	
	
	return result;
}// NewApplicationModal


/*!
Constructs a translucent sheet alert if the parent window
is not nil; otherwise, constructs a movable and modal
window (see also Alert_NewApplicationModal(), which is a
short-cut for this 2nd case).

IMPORTANT:	If unsuccessful, nullptr is returned.

(2016.03)
*/
AlertMessages_BoxRef
Alert_NewWindowModal	(NSWindow*		inParentWindow)
{
	AlertMessages_BoxRef	result = nullptr;
	
	
	try
	{
		My_AlertMessagePtr		alertPtr = new My_AlertMessage();
		
		
		if (nullptr != alertPtr)
		{
			alertPtr->parentNSWindow = inParentWindow;
			// the window is not required to be defined because a
			// lack of window is used by Alert_NewApplicationModal()
			//assert(nil != alertPtr->parentNSWindow);
			
			alertPtr->viewController = [[AlertMessages_VC alloc] init];
			assert(nil != alertPtr->viewController);
			
			alertPtr->genericDialog.setWithNoRetain(GenericDialog_New
													(inParentWindow.contentView, alertPtr->viewController/* transfer ownership */,
														nullptr/* data set */, true/* is alert style */));
			assert(alertPtr->genericDialog.exists());
			
			// set a default help button action
			Alert_SetButtonResponseBlock(alertPtr->selfRef, kAlert_ItemHelpButton, ^{ HelpSystem_DisplayHelpWithoutContext(); });
			
			result = alertPtr->selfRef;
			assert(nullptr != result);
			
			Alert_Retain(result);
		}
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// NewWindowModal


/*!
Adds a lock on the specified reference, preventing it from
being deleted.  See Alert_Release().

NOTE:	Usually, you should let AlertMessages_BoxWrap objects
		handle retain/release for you.

(2016.03)
*/
void
Alert_Retain	(AlertMessages_BoxRef	inAlert)
{
	gAlertRefLocks().acquireLock(inAlert);
}// Retain


/*!
Releases one lock on the specified alert and deletes the alert
*if* no other locks remain.  Your copy of the reference is set
to nullptr either way.

NOTE:	Usually, you should let AlertMessages_BoxWrap objects
		handle retain/release for you.

(2016.03)
*/
void
Alert_Release	(AlertMessages_BoxRef*		inoutAlertPtr)
{
	if (gAlertBoxValidRefs().end() == gAlertBoxValidRefs().find(*inoutAlertPtr))
	{
		Console_Warning(Console_WriteValueAddress, "attempt to Alert_Release() with invalid reference", *inoutAlertPtr);
	}
	else
	{
		gAlertRefLocks().releaseLock(*inoutAlertPtr);
		unless (gAlertRefLocks().isLocked(*inoutAlertPtr))
		{
			if (gAlertPtrLocks().isLocked(*inoutAlertPtr))
			{
				Console_Warning(Console_WriteValue, "attempt to dispose of locked alert; outstanding locks",
								gAlertPtrLocks().returnLockCount(*inoutAlertPtr));
			}
			else
			{
				delete *(REINTERPRET_CAST(inoutAlertPtr, My_AlertMessagePtr*)), *inoutAlertPtr = nullptr;
			}
		}
	}
	*inoutAlertPtr = nullptr;
}// Release


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
	UNUSED_RETURN(Boolean)backgroundNotification();
}// BackgroundNotification


/*!
Disables the window-closing animation that occurs when the
primary button is selected.

NOTE:	This is a very special case and it is really only meant
		for alerts that guard against closing the parent window.

(4.1)
*/
void
Alert_DisableCloseAnimation		(AlertMessages_BoxRef	inAlert)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	GenericDialog_SetItemDialogEffect(alertPtr->genericDialog.returnRef(), kGenericDialog_ItemIDButton1,
										kGenericDialog_DialogEffectCloseImmediately);
}// DisableCloseAnimation


/*!
Use this method to display an alert message.  For alerts with
multiple buttons, Alert_SetButtonResponseBlock() should be used
to determine button actions.

Application-modal alerts will block until the alert is dismissed
(either by user action or timeout) and this call will not return
until the alert is closed.

Conversely, window-modal alerts display asynchronously and this
call will return immediately.  The response blocks for buttons
can therefore be called at any time.

The "isAnimated" flag prevents all animations.  You can prevent
animations globally (regardless of the "isAnimated" value) by
calling Alert_SetIsAnimationAllowed().  You can prevent the
alert-close animation for primary-button clicks by calling the
Alert_DisableCloseAnimation() method (useful for alerts that
warn against closing the parent window).  In general, animation
controls should only be used to prevent alert animations from
interfering with other animations.

If the application is in the background (as specified by a call
to Alert_SetIsBackgrounded()), the user’s preferred background
notification scheme is applied — such as an animating Dock icon.
Alert_ServiceNotification() must be called when the application
“resumes” so that the notification will be cleared properly.

(2016.05)
*/
void
Alert_Display	(AlertMessages_BoxRef	inAlert,
				 Boolean				inAnimated)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	GenericDialog_Ref		genericDialog = alertPtr->genericDialog.returnRef();
	Boolean					animationFinalFlag = (inAnimated && gIsAnimationAllowed);
	
	
	UNUSED_RETURN(Boolean)backgroundNotification();
	
	[[NSCursor arrowCursor] set];
	
	if (nullptr != genericDialog)
	{
		// the first two buttons will close the alert by default but
		// they will also animate by default; set this based on the
		// animation flag above
		if (false == animationFinalFlag)
		{
			// animation is disabled; any buttons that are set to
			// perform an animated close should now close immediately
			if (kGenericDialog_DialogEffectCloseNormally ==
				GenericDialog_ReturnItemDialogEffect(genericDialog, kGenericDialog_ItemIDButton1))
			{
				GenericDialog_SetItemDialogEffect(genericDialog, kGenericDialog_ItemIDButton1,
													kGenericDialog_DialogEffectCloseImmediately);
			}
			if (kGenericDialog_DialogEffectCloseNormally ==
				GenericDialog_ReturnItemDialogEffect(genericDialog, kGenericDialog_ItemIDButton2))
			{
				GenericDialog_SetItemDialogEffect(genericDialog, kGenericDialog_ItemIDButton2,
													kGenericDialog_DialogEffectCloseImmediately);
			}
		}
		// regardless of the animation setting, the third button’s
		// dialog effect MUST be set explicitly (to close the dialog)
		// because the third button does not close by default
		GenericDialog_SetItemDialogEffect(genericDialog, kGenericDialog_ItemIDButton3,
											(animationFinalFlag)
											? kGenericDialog_DialogEffectCloseNormally
											: kGenericDialog_DialogEffectCloseImmediately);
		
		// display the dialog; since Alert_Release() overwrites its
		// target variable, a local copy of the value is created
		{
			__block AlertMessages_BoxRef	dummyBox = inAlert;
			
			
			Alert_Retain(dummyBox), GenericDialog_Display(genericDialog, animationFinalFlag,
															^{ Alert_Release(&dummyBox); }); // retains dialog until it is dismissed
		}
		
		// speak text if necessary - UNIMPLEMENTED
	}
	else
	{
		Console_Warning(Console_WriteLine, "cannot display, alert is not initialized");
	}
}// Display


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
	AlertMessages_BoxWrap	box(Alert_NewApplicationModal(), AlertMessages_BoxWrap::kAlreadyRetained);
	
	
	Alert_SetHelpButton(box.returnRef(), inIsHelpButton);
	Alert_SetTextCFStrings(box.returnRef(), inDialogText, inHelpText);
	Alert_SetIcon(box.returnRef(), kAlert_IconIDNote);
	Alert_Display(box.returnRef()); // retains alert until it is dismissed
}// Message


/*!
Returns the Generic Dialog object that is used to display
the alert.

Note that it is preferable to use methods from this module
to affect alert windows.  The Generic Dialog accessor is
mostly useful for cases of direct window management, such
as the case of holding a reference to any open dialog (alert
or otherwise).

IMPORTANT:	It is possible for an AlertMessages_BoxRef to
			be released while its Generic Dialog is still
			on screen.  In other words, the Generic Dialog
			may “out-live” the alert object, existing only
			to run the user interface itself.  This is a
			common case, since user input is asynchronous.
			The Generic Dialog is owned by the running
			interface, and is released when the dialog is
			removed.

(2016.03)
*/
GenericDialog_Ref
Alert_ReturnGenericDialog		(AlertMessages_BoxRef	inAlert)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	GenericDialog_Ref		result = alertPtr->genericDialog.returnRef();
	
	
	return result;
}// ReturnGenericDialog


/*!
Call this routine when a resume event occurs to clear
any background notifications that may have been posted.

(1.0)
*/
void
Alert_ServiceNotification ()
{
@autoreleasepool {
	if ((kInvalidUserAttentionRequestID != gNotificationRequestID) ||
		(gNotificationIsIconBadged))
	{
		// remove any caution icon badge
		[NSApp setApplicationIconImage:nil];
		
		[NSApp cancelUserAttentionRequest:gNotificationRequestID];
		gNotificationRequestID = kInvalidUserAttentionRequestID;
		gNotificationIsIconBadged = false;
	}
}// @autoreleasepool
}// ServiceNotification


/*!
Specifies the response for an action button.

(4.1)
*/
void
Alert_SetButtonResponseBlock	(AlertMessages_BoxRef	inAlert,
								 UInt16					inWhichButton,
								 void					(^inResponseBlock)(),
								 Boolean				inIsHarmfulAction)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		if (NO == alertPtr->genericDialog.exists())
		{
			Console_Warning(Console_WriteLine, "unable to set button response block for alert that is not using Generic Dialog");
		}
		else
		{
			GenericDialog_ItemID	itemID = returnGenericItemIDForAlertItem(inWhichButton);
			
			
			if (kGenericDialog_ItemIDNone == itemID)
			{
				Console_Warning(Console_WriteValue, "invalid button number for response block", inWhichButton);
			}
			else
			{
				GenericDialog_SetItemResponseBlock(alertPtr->genericDialog.returnRef(), itemID, inResponseBlock, inIsHarmfulAction);
			}
		}
	}
}// SetButtonResponseBlock


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

When text is set for "kAlert_ItemButton3", the label is
used to automatically infer a command key; the NSButton
method "keyEquivalent" is used to set a key character and
"keyEquivalentModifierMask" is "NSEventModifierFlagCommand".

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
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		if (NO == alertPtr->genericDialog.exists())
		{
			Console_Warning(Console_WriteLine, "unable to set button text for alert that is not using Generic Dialog");
		}
		else
		{
			GenericDialog_ItemID	itemID = returnGenericItemIDForAlertItem(inWhichButton);
			
			
			if (kGenericDialog_ItemIDNone == itemID)
			{
				Console_Warning(Console_WriteValue, "invalid button number when setting text", inWhichButton);
			}
			else
			{
				GenericDialog_SetItemTitle(alertPtr->genericDialog.returnRef(), itemID, inNewText);
			}
		}
	}
}// SetButtonText


/*!
Specifies whether a help button appears in an alert.
Help buttons should not appear unless help is
available!

(1.0)
*/
void
Alert_SetHelpButton		(AlertMessages_BoxRef	inAlert,
						 Boolean				inIsHelpButton)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->viewController.panelHasContextualHelp = inIsHelpButton;
	}
}// SetHelpButton


/*!
Sets the icon for an alert.  If the icon is set to
"kAlert_IconIDNone" then nothing is displayed.

(2016.03)
*/
void
Alert_SetIcon	(AlertMessages_BoxRef	inAlert,
				 Alert_IconID			inIcon)
{
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->iconID = inIcon;
		
		// attempt to update the view controller; this will
		// generally fail because the UI probably isn’t
		// loaded yet
		alertPtr->viewController.iconID = inIcon;
	}
}// SetIcon


/*!
This sets a global override for animation flags so that
even if animation is requested in Alert_Display(), it
has no effect.

This should correspond to a user preference setting.

(2016.03)
*/
void
Alert_SetIsAnimationAllowed		(Boolean	inIsAnimationAllowed)
{
	gIsAnimationAllowed = inIsAnimationAllowed;
}// SetIsAnimationAllowed


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
			break;
		
		case kAlert_StyleCancel:
			newButtonString(tempCFString, kUIStrings_ButtonCancel);
			if (tempCFString.exists())
			{
				Alert_SetButtonText(inAlert, kAlert_ItemButton1, tempCFString.returnCFStringRef());
			}
			Alert_SetButtonText(inAlert, kAlert_ItemButton2, nullptr);
			Alert_SetButtonText(inAlert, kAlert_ItemButton3, nullptr);
			break;
		
		case kAlert_StyleOKCancel:
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
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->viewController.dialogText = BRIDGE_CAST(inDialogText, NSString*);
		alertPtr->viewController.helpText = BRIDGE_CAST(inHelpText, NSString*);
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
	My_AlertAutoLocker		alertPtr(gAlertPtrLocks(), inAlert);
	
	
	if (nullptr != alertPtr)
	{
		alertPtr->viewController.titleText = BRIDGE_CAST(inNewTitle, NSString*);
	}
}// SetTitle


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


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See the Alert_New...() methods.

(1.1)
*/
My_AlertMessage::
My_AlertMessage ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
refValidator(REINTERPRET_CAST(this, AlertMessages_BoxRef), gAlertBoxValidRefs()),
selfRef(REINTERPRET_CAST(this, AlertMessages_BoxRef)),
viewController(nil),
genericDialog(),
iconID(kAlert_IconIDDefault),
parentNSWindow(nil)
{
}// My_AlertMessage default constructor


/*!
If the application is in the background and no previous
notification is pending, this method will request the
user’s attention.  The exact form of notification depends
on calls to Alert_SetNotificationPreferences().

Returns true only if a new user notification was posted.

(2016.05)
*/
Boolean
backgroundNotification ()
{
	Boolean		result = false;
	
	
	if ((gNotificationIsBackgrounded) && (kInvalidUserAttentionRequestID == gNotificationRequestID))
	{
		// badge the application’s Dock tile with a caution icon, if that preference is set
		switch (gNotificationPreferences)
		{
		case kAlertMessages_NotificationTypeMarkDockIcon:
			badgeApplicationDockTile();
			break;
		
		case kAlertMessages_NotificationTypeMarkDockIconAndBounceOnce:
			badgeApplicationDockTile();
			gNotificationRequestID = [NSApp requestUserAttention:NSInformationalRequest];
			break;
		
		case kAlertMessages_NotificationTypeMarkDockIconAndBounceRepeatedly:
			badgeApplicationDockTile();
			gNotificationRequestID = [NSApp requestUserAttention:NSCriticalRequest];
			break;
		
		case kAlertMessages_NotificationTypeDoNothing:
		default:
			break;
		}
		
		result = true;
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
@autoreleasepool
{
	NSImage*	appIconImage = [[NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnBundleIconFilenameNoExtension(), NSString*)] copy];
	NSImage*	overlayImage = [NSImage imageNamed:NSImageNameCaution];
	NSSize		imageSize = [appIconImage size];
	
	
	imageSize.width /= 2.0;
	imageSize.height /= 2.0;
	[appIconImage lockFocus];
	// the image location is somewhat arbitrary, and should probably be made configurable; TEMPORARY
	[overlayImage drawInRect:NSMakeRect(imageSize.width/* x coordinate */, 0/* y coordinate */,
										imageSize.width/* width */, imageSize.height/* height */)
								fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0];
	[appIconImage unlockFocus];
	
	[NSApp setApplicationIconImage:appIconImage];
	
	gNotificationIsIconBadged = true;
}// @autoreleasepool
}// badgeApplicationDockTile


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
	outStringStorage.setWithNoRetain(stringRef);
}// newButtonString


/*!
Translates from Alert module button numbers to those used
by the Generic Dialog module, if possible.

(4.1)
*/
GenericDialog_ItemID
returnGenericItemIDForAlertItem		(UInt16		inWhichButton)
{
	GenericDialog_ItemID	result = kGenericDialog_ItemIDNone;
	
	
	switch (inWhichButton)
	{
	case kAlert_ItemButton1:
		result = kGenericDialog_ItemIDButton1;
		break;
	
	case kAlert_ItemButton2:
		result = kGenericDialog_ItemIDButton2;
		break;
	
	case kAlert_ItemButton3:
		result = kGenericDialog_ItemIDButton3;
		break;
	
	case kAlert_ItemHelpButton:
		result = kGenericDialog_ItemIDHelpButton;
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// returnGenericItemIDForAlertItem

} // anonymous namespace


#pragma mark -
@implementation AlertMessages_VC //{


#pragma mark Internally-Declared Properties


@synthesize iconID = _iconID;
@synthesize iconImageName = _iconImageName;


#pragma mark Initializers


/*!
Common initializer.

(4.1)
*/
- (instancetype)
init
{
	return [self initWithNibNamed:@"AlertMessagesCocoa"];
}// init


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithNibNamed:(NSString*)	aNibName
{
	self = [super initWithNibNamed:aNibName delegate:self context:nullptr];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// initWithNibNamed:


/*!
Designated initializer.

This exists to make the compiler happy, because the
superclass has this designated initializer.  In
reality, use the simpler "initWithNibNamed:".

(2019.10)
*/
- (instancetype)
initWithNibNamed:(NSString*)		aNibName
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	self = [super initWithNibNamed:aNibName delegate:aDelegate context:aContext];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// initWithNibNamed:delegate:context:


/*!
Designated initializer.

This variant of initializer is not currently supported.

(2020.11)
*/
- (instancetype)
initWithView:(NSView*)				aView
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	self = [super initWithView:aView delegate:aDelegate context:aContext];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// initWithView:delegate:context:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark New Methods


/*!
Responds to changes in the size of text views by
updating the ideal frame size of the parent.

(2022.11)
*/
- (void)
viewFrameDidChange:(NSNotification*)	aNotification
{
	if ((aNotification.object == self.dialogTextUI) || (aNotification.object == self.helpTextUI))
	{
		CGFloat		idealHeight = 0;
		
		
		if (NO == self.titleTextUI.hidden)
		{
			idealHeight += NSHeight(self.titleTextUI.frame);
			idealHeight += 10; // should match padding of text stack
		}
		
		if (NO == self.dialogTextUI.hidden)
		{
			idealHeight += self.dialogTextUI.frame.size.height;
		}
		
		if (NO == self.helpTextUI.hidden)
		{
			idealHeight += 10; // should match padding of text stack
			idealHeight += self.helpTextUI.frame.size.height;
		}
		
		// notify observers (e.g. to resize any parent window)
		self.idealFrameSize = NSMakeSize(self.idealFrameSize.width, idealHeight);
		[self postNote:kPanel_IdealSizeDidChangeNotification];
	}
}// viewFrameDidChange:


#pragma mark Accessors


/*!
Accessor for the name of the image to use as the icon.
This is only the base image, which is automatically given
an application icon badge before rendering.  If set to an
empty string, then the icon is a large application icon
that has no badge.

(4.1)
*/
- (NSString*)
iconImageName
{
	return [_iconImageName copy];
}
+ (BOOL)
automaticallyNotifiesObserversOfIconImageName
{
	return NO;
}
- (void)
setIconImageName:(NSString*)	aString
{
	if (aString != _iconImageName)
	{
		[self willChangeValueForKey:@"iconImageName"];
		
		if (nil == aString)
		{
			_iconImageName = @"";
		}
		else
		{
			_iconImageName = [aString copy];
		}
		
		[self didChangeValueForKey:@"iconImageName"];
	}
	[self.mainIconUI setImage:[self imageForIconImageName:aString]];
}// setIconImageName:


#pragma mark NSWindowNotifications


/*!
Responds to window activation by restoring the icon.

(4.1)
*/
- (void)
windowDidBecomeKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.mainIconUI.enabled = YES;
}// windowDidBecomeKey:


/*!
Responds to window deactivation by graying the icon.

(4.1)
*/
- (void)
windowDidResignKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.mainIconUI.enabled = NO;
}// windowDidResignKey:


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext
{
#pragma unused(aViewManager, aContext)
	self.titleText = @"";
	self.dialogText = @"";
	self.helpText = @"";
	self.iconID = kAlert_IconIDDefault; // can change later
	self.iconImageName = NSImageNameCaution; // can change later
	
	// set a default value (can be changed later)
	self.panelHasContextualHelp = NO;
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	*outEditType = kPanel_EditTypeNormal;
}// panelViewManager:requestingEditType:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
#pragma unused(aViewManager, aContainerView)
	assert(nil != self.view);
	assert(nil != self.titleTextUI);
	assert(nil != self.dialogTextUI);
	assert(nil != self.helpTextUI);
	assert(nil != self.mainIconUI);
	
	// initialize views
	[self setUpFonts];
	self.titleTextUI.textColor = [NSColor secondaryLabelColor];
	self.mainIconUI.image = [self imageForIconImageName:self.iconImageName];
	self.mainIconUI.mouseDownCanMoveWindow = YES;
	
	// remember the original NIB view size as ideal
	// (this can be changed by the layout)
	self.idealFrameSize = self.view.frame.size;
	
	// remember the original icon size as a minimum height
	self.idealIconSize = self.mainIconUI.frame.size;
	
	// update ideal size if text views are resized
	//[self whenObject:self.dialogTextUI postsNote:NSViewFrameDidChangeNotification performSelector:@selector(viewFrameDidChange:)];
	//[self whenObject:self.helpTextUI postsNote:NSViewFrameDidChangeNotification performSelector:@selector(viewFrameDidChange:)];
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

This varies as the alert’s properties are changed (such
as text; see auto-layout constraints) but size changes
are not guaranteed to be used by containing windows
unless they are made prior to displaying the panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	*outIdealSize = self.idealFrameSize;
}// panelViewManager:requestingIdealSize:


/*!
Responds to a request for contextual help in this panel.

NOTE:	This does nothing because the alert panel is only
		displayed in a wrapping Generic Dialog (which has
		full support for binding actions to buttons).

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager, sender)
	;
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager)
	if (kPanel_VisibilityDisplayed == aVisibility)
	{
		// note: since the parent is now a stack view, hiding
		// a text view will implicitly consume the spacing
		// to the neighboring element as well
		if (0 == self.titleTextUI.stringValue.length)
		{
			self.titleTextUI.hidden = YES;
		}
		else
		{
			self.titleTextUI.hidden = NO;
			[self.titleTextUI sizeToFit];
		}
	}
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager)
	if (kPanel_VisibilityDisplayed == aVisibility)
	{
		// assume the window is now displayed and subscribe to notifications
		[self whenObject:aViewManager.managedView.window postsNote:NSWindowDidBecomeKeyNotification
							performSelector:@selector(windowDidBecomeKey:)];
		[self whenObject:aViewManager.managedView.window postsNote:NSWindowDidResignKeyNotification
							performSelector:@selector(windowDidResignKey:)];
	}
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

Not applicable to this panel because it is only intended to
display a single alert message.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet, newDataSet)
	// do nothing
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes.

NOTE:	This does nothing because the alert panel is only
		displayed in a wrapping Generic Dialog (which has
		full support for binding actions to buttons).

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager, aContainerView, isAccepted)
	// do nothing (assume Generic Dialog wrapper)
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_ViewManager


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	NSImage*	result = [NSImage imageNamed:self.iconImageName];
	
	
	assert(nil != result);
	return result;
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

Currently the identifier is the same for all alerts.  If
necessary, the interface may change in the future to allow
callers to set different identifiers for each alert.

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.panels.AlertMessage";
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

(4.1)
*/
- (NSString*)
panelName
{
	return NSLocalizedStringFromTable(@"Alert", @"AlertMessage", @"the name of this panel");
}// panelName


/*!
Returns information on which directions are most useful for
resizing the panel.  For instance a window container may
disallow vertical resizing if no panel in the window has
any reason to resize vertically.

IMPORTANT:	This is only a hint.  Panels must be prepared
			to resize in both directions.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintNone;
}// panelResizeAxes


@end //} AlertMessages_VC


#pragma mark -
@implementation AlertMessages_VC (AlertMessages_VCInternal) //{


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (Alert_IconID)
iconID
{
	return _iconID;
}
- (void)
setIconID:(Alert_IconID)	anIconID
{
	if (_iconID != anIconID)
	{
		_iconID = anIconID;
		
		// change the displayed icon image
		switch (anIconID)
		{
		case kAlert_IconIDNone:
			self.iconImageName = @"";
			break;
		
		case kAlert_IconIDNote:
			self.iconImageName = NSImageNameApplicationIcon;
			break;
		
		case kAlert_IconIDStop: // for now, make this the same
		case kAlert_IconIDDefault:
		default:
			self.iconImageName = NSImageNameCaution;
			break;
		}
	}
}// setIconID:


#pragma mark New Methods


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
	NSImage*	appIconImage = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnBundleIconFilenameNoExtension(), NSString*)];
	NSImage*	result = appIconImage; // most alerts just use the application icon
	
	
	if ((nil != anImageName) && ([anImageName length] > 0))
	{
		if (nil == result)
		{
			// failed to find image
			Console_Warning(Console_WriteValueCFString, "failed to find icon for image name", BRIDGE_CAST(anImageName, CFStringRef));
			result = appIconImage;
		}
		else if ([anImageName isEqualToString:NSImageNameApplicationIcon])
		{
			// entire image is application icon
			result = appIconImage;
		}
		else
		{
			// valid image; superimpose small version of it on application icon
			NSImage*	newImage = [NSImage imageNamed:anImageName];
			NSSize		imageSize = NSMakeSize(appIconImage.size.width / 2.0, appIconImage.size.height / 2.0);
			
			
			result = [appIconImage copy];
			[result lockFocus];
			[newImage drawInRect:NSMakeRect(imageSize.width/* x coordinate */, 0/* y coordinate */,
											imageSize.width/* width */, imageSize.height/* height */)
									fromRect:NSZeroRect
									operation:NSCompositingOperationSourceOver
									fraction:1.0];
			[result unlockFocus];
		}
	}
	
	return result;
}// imageForIconImageName:


/*!
This should call "setFont:" on the "dialogTextUI", "helpTextUI"
and "titleTextUI" elements so that they are initialized to
appropriate typefaces and sizes.  The default implementation
follows standard alert requirements, but a subclass may wish to
use smaller fonts, e.g. for modeless notification windows.

(4.1)
*/
- (void)
setUpFonts
{
	//self.titleTextUI.font = [NSFont userFontOfSize:20.0/* arbitrary */];
	self.dialogTextUI.font = [NSFont boldSystemFontOfSize:[NSFont systemFontSize]];
	self.helpTextUI.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
}// setUpFonts


@end //} AlertMessages_VC (AlertMessages_VCInternal)


#pragma mark -
@implementation AlertMessages_WindowDraggingIcon //{


@synthesize mouseDownCanMoveWindow = _mouseDownCanMoveWindow;


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
