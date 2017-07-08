/*!	\file AlertMessages.h
	\brief Greatly simplified and convenient interface to
	alert messages, be they modal dialogs or sheets.
	
	Also, since alerts so frequently tie into background
	notification schemes, this module handles background
	alerts as well (including badging the Dock icon if
	necessary, etc.).
*/
/*###############################################################

	Interface Library
	© 1998-2017 by Kevin Grant
	
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSWindow;
#endif
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// library includes
#ifdef __OBJC__
#	import <CocoaFuture.objc++.h>
#endif
#include <RetainRelease.template.h>

// application includes
#ifdef __OBJC__
@class AlertMessages_ContentView;
@class AlertMessages_WindowDraggingIcon;
#endif
#include "GenericDialog.h"
#include "Panel.h"



#pragma mark Constants

/*!
Pass one of these to Alert_SetNotificationPreferences()
to decide how the application should respond to alerts
that appear in the background.
*/
enum
{
	kAlert_NotifyDoNothing = 0,
	kAlert_NotifyDisplayDiamondMark = 1,
	kAlert_NotifyDisplayIconAndDiamondMark = 2,
	kAlert_NotifyAlsoDisplayAlert = 3
};

/*!
Styles allow multiple properties of an alert to be
set to standard values in a single call.
*/
enum
{
	kAlert_StyleOK,					//!< preset the primary button to be named “OK”
	kAlert_StyleCancel,				//!< preset the primary button to be named “Cancel”
	kAlert_StyleOKCancel,			//!< preset the primary button to be named “OK” and
									//!  the second button to be named “Cancel”
	kAlert_StyleDontSaveCancelSave	//!< standard three-button layout and button names
};

/*!
Item identifiers are used in APIs that make settings
that apply to only certain items (such as the title,
or the code that is invoked by a button press).

This is a number instead of a named enumeration so
that it can be used in certain APIs that expect
numbers.
*/
enum
{
	kAlert_ItemButtonNone = 0,	//!< no item at all (useful for variables)
	kAlert_ItemButton1 = 1,		//!< primary button (e.g. “OK”)
	kAlert_ItemButton2 = 2,		//!< secondary button (e.g. “Cancel”)
	kAlert_ItemButton3 = 3,		//!< third button (e.g. “Don’t Save”)
	kAlert_ItemHelpButton = 4	//!< round “?” button
};

/*!
The icon ID is a way to request a standard icon (or
lack of icon) in the window.  New alerts start with
"kAlert_IconIDDefault".
*/
enum Alert_IconID
{
	kAlert_IconIDNone = 0,		//!< no icon
	kAlert_IconIDDefault = 1,	//!< caution icon (inverted triangle with “!”)
	kAlert_IconIDStop = 2,		//!< currently the same as the default case but may change
	kAlert_IconIDNote = 3		//!< for simple messages (currently uses application icon)
};

#pragma mark Types

#ifdef __OBJC__

/*!
The base type for Cocoa-based alerts.  This class will not
be used directly in XIB files, but the names it uses for
various properties, etc. are still important and must be
kept in sync with the XIBs that instantiate derived classes.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface AlertMessages_VC : Panel_ViewManager < Panel_Delegate > //{
{
@public
	IBOutlet NSTextField*						titleTextUI;
	IBOutlet NSTextView*						dialogTextUI;
	IBOutlet NSTextView*						helpTextUI;
	IBOutlet AlertMessages_WindowDraggingIcon*	mainIconUI;

@private
	NSMutableArray*		registeredObservers;
	NSSize				idealFrameSize;
	NSSize				idealIconSize;
	NSString*			_titleText;
	NSString*			_dialogText;
	NSString*			_helpText;
	NSString*			iconImageName;
	Alert_IconID		iconID;
}

// new methods
	- (void)
	adjustViews;
	- (void)
	setUpFonts;

// accessors
	@property (strong) NSString*
	dialogText; // binding
	@property (strong) NSString*
	helpText; // binding
	- (NSString*)
	iconImageName;
	- (void)
	setIconImageName:(NSString*)_;
	@property (strong) NSString*
	titleText; // binding

// initializers
	- (instancetype)
	init;
	- (instancetype)
	initWithNibNamed:(NSString*)_ NS_DESIGNATED_INITIALIZER;

@end //}

#endif // __OBJC__

typedef struct AlertMessages_OpaqueBox*		AlertMessages_BoxRef;



#pragma mark Public Methods

//!\name Global Settings
//@{

void
	Alert_SetIsAnimationAllowed			(Boolean					inIsAnimationAllowed);

void
	Alert_SetIsBackgrounded				(Boolean					inIsApplicationSuspended);

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
	Alert_NewApplicationModal			();

AlertMessages_BoxRef
	Alert_NewWindowModal				(NSWindow*							inParentWindow);

AlertMessages_BoxRef
	Alert_NewWindowModalParentCarbon	(HIWindowRef						inParentWindow);

void
	Alert_Retain						(AlertMessages_BoxRef				inAlert);

void
	Alert_Release						(AlertMessages_BoxRef*				inoutAlert);

//@}

//!\name Displaying and Removing Alerts
//@{

void
	Alert_Display						(AlertMessages_BoxRef		inAlert,
										 Boolean					inAnimated = true);

void
	Alert_Message						(CFStringRef				inDialogText,
										 CFStringRef				inHelpText,
										 Boolean					inIsHelpButton);

// DEPRECATED, CARBON LEGACY.
Boolean
	Alert_ReportOSStatus				(OSStatus					inErrorCode,
										 Boolean					inAssertion = false);

GenericDialog_Ref
	Alert_ReturnGenericDialog			(AlertMessages_BoxRef		inAlert);

//@}

//!\name Helper Routines to Specify Alert Window Adornments
//@{

void
	Alert_DisableCloseAnimation			(AlertMessages_BoxRef		inAlert);

void
	Alert_SetButtonResponseBlock		(AlertMessages_BoxRef		inAlert,
										 UInt16						inWhichButton,
										 void						(^inResponseBlock)());

void
	Alert_SetButtonText					(AlertMessages_BoxRef		inAlert,
										 UInt16						inWhichButton,
										 CFStringRef				inNewText);

void
	Alert_SetHelpButton					(AlertMessages_BoxRef		inAlert,
										 Boolean					inIsHelpButton);

void
	Alert_SetIcon						(AlertMessages_BoxRef		inAlert,
										 Alert_IconID				inIcon);

void
	Alert_SetParamsFor					(AlertMessages_BoxRef		inAlert,
										 UInt16						inAlertStyle);

void
	Alert_SetTextCFStrings				(AlertMessages_BoxRef		inAlert,
										 CFStringRef				inDialogText,
										 CFStringRef				inHelpText);

void
	Alert_SetTitleCFString				(AlertMessages_BoxRef		inAlert,
										 CFStringRef				inNewText);

//@}



#pragma mark Types Dependent on Method Names

// DO NOT USE DIRECTLY.
struct _AlertMessages_BoxRefMgr
{
	typedef AlertMessages_BoxRef	reference_type;
	
	static void
	retain	(reference_type		inRef)
	{
		Alert_Retain(inRef);
	}
	
	static void
	release	(reference_type		inRef)
	{
		Alert_Release(&inRef);
	}
};

/*!
Allows RAII-based automatic retain and release of a dialog so
you don’t have to call Alert_Release() yourself.  Simply
declare a variable of this type (in a data structure, say),
initialize it as appropriate, and your reference is safe.  Note
that there is a constructor that allows you to store pre-retained
(e.g. newly allocated) references too.
*/
typedef RetainRelease< _AlertMessages_BoxRefMgr >		AlertMessages_BoxWrap;

// BELOW IS REQUIRED NEWLINE TO END FILE
