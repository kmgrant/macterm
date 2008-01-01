/*###############################################################

	AboutBox.cp
	
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

// standard-C includes
#include <cctype>
#include <climits>
#include <cstdlib>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#include <QuickTime/QuickTime.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <Console.h>
#include <Cursors.h>
#include <Embedding.h>
#include <FlagManager.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <WindowInfo.h>

// resource includes
#include "GeneralResources.h"

// MacTelnet includes
#include "AboutBox.h"
#include "AppResources.h"
#include "Commands.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "StatusIcon.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Dialog" NIB from the package "AboutBox.nib".
*/
enum
{
	kSignatureMyUserPaneAppIcon			= FOUR_CHAR_CODE('Icon'),
	kSignatureMyTextAppName				= FOUR_CHAR_CODE('Name'),
	kSignatureMyTextAppVersion			= FOUR_CHAR_CODE('Vers'),
	kSignatureMyTextAppDescription		= FOUR_CHAR_CODE('Desc'),
	kSignatureMyTextAppCopyrightLine1	= FOUR_CHAR_CODE('Cpy1'),
	kSignatureMyTextAppCopyrightLine2	= FOUR_CHAR_CODE('Cpy2'),
	kSignatureMyTextAppCopyrightLine3	= FOUR_CHAR_CODE('Cpy3')
};
namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	HIViewID const		idMyUserPaneAppIcon			= { kSignatureMyUserPaneAppIcon,			0/* ID */ };
	HIViewID const		idMyTextAppName				= { kSignatureMyTextAppName,				0/* ID */ };
	HIViewID const		idMyTextAppVersion			= { kSignatureMyTextAppVersion,				0/* ID */ };
	HIViewID const		idMyTextAppDescription  	= { kSignatureMyTextAppDescription,			0/* ID */ };
	HIViewID const		idMyTextAppCopyrightLine1	= { kSignatureMyTextAppCopyrightLine1,		0/* ID */ };
	HIViewID const		idMyTextAppCopyrightLine2	= { kSignatureMyTextAppCopyrightLine2,		0/* ID */ };
	HIViewID const		idMyTextAppCopyrightLine3	= { kSignatureMyTextAppCopyrightLine3,		0/* ID */ };
}

#pragma mark Internal Method Prototypes

static void				displayCreditsWindow				();
static OSStatus			handleAboutBoxCommand				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static pascal OSStatus	receiveCreditsSheetButtonHICommand  (EventHandlerCallRef, EventRef, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowRef					gStandardAboutBoxWindow = nullptr;
	WindowRef					gCreditsWindow = nullptr;					//!< window holding credits and license information
	StatusIconRef				gAboutBoxApplicationIcon = nullptr;
	EventHandlerUPP				gButtonHICommandsUPP = nullptr;				//!< wrapper for credits button callback function
	EventHandlerRef				gButtonHICommandsHandler = nullptr;			//!< invoked when the Done button is clicked in the credits
	ListenerModel_ListenerRef   gCommandExecutionEventListener = nullptr;	//!< invoked when interesting menu commands are used
}



#pragma mark Public Methods

/*!
This method is used to initialize the Standard About Box.
It creates the dialog box invisibly, and sets up dialog
controls.

(3.0)
*/
void
AboutBox_Init ()
{
	ControlRef		iconUserPane = nullptr;
	ControlRef		appNameText = nullptr;
	ControlRef		appVersionText = nullptr;
	ControlRef		appDescText = nullptr;
	ControlRef		appCopyrightTextLine1 = nullptr;
	ControlRef		appCopyrightTextLine2 = nullptr;
	ControlRef		appCopyrightTextLine3 = nullptr;
	
	
	Cursors_DeferredUseWatch(30); // if it takes more than half a second to initialize, show the watch cursor
	
	// load the NIB containing this dialog (automatically finds the right localization)
	gStandardAboutBoxWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
										CFSTR("AboutBox"), CFSTR("Dialog")) << NIBLoader_AssertWindowExists;
	
	// ensure this window does not show up in the Dock menu or elsewhere
	(OSStatus)ChangeWindowAttributes(gStandardAboutBoxWindow, 0/* attributes to set */,
										kWindowInWindowMenuAttribute/* attributes to clear */);
	
	// find references to all controls that are needed for any operation
	{
		OSStatus	error = noErr;
		
		
		error = GetControlByID(gStandardAboutBoxWindow, &idMyUserPaneAppIcon, &iconUserPane);
		assert(error == noErr);
		error = GetControlByID(gStandardAboutBoxWindow, &idMyTextAppName, &appNameText);
		assert(error == noErr);
		error = GetControlByID(gStandardAboutBoxWindow, &idMyTextAppVersion, &appVersionText);
		assert(error == noErr);
		error = GetControlByID(gStandardAboutBoxWindow, &idMyTextAppDescription, &appDescText);
		assert(error == noErr);
		error = GetControlByID(gStandardAboutBoxWindow, &idMyTextAppCopyrightLine1, &appCopyrightTextLine1);
		assert(error == noErr);
		error = GetControlByID(gStandardAboutBoxWindow, &idMyTextAppCopyrightLine2, &appCopyrightTextLine2);
		assert(error == noErr);
		error = GetControlByID(gStandardAboutBoxWindow, &idMyTextAppCopyrightLine3, &appCopyrightTextLine3);
		assert(error == noErr);
	}
	
	// read the actual bundle name and version string from theInfo.plist file and set the text to match
	{
		CFRetainRelease		locker(CFBundleGetValueForInfoDictionaryKey
									(AppResources_ReturnBundleForInfo(), CFSTR("CFBundleName")));
		
		
		SetControlTextWithCFString(appNameText, locker.returnCFStringRef());
	}
	{
		CFRetainRelease		locker(CFBundleGetValueForInfoDictionaryKey
									(AppResources_ReturnBundleForInfo(), CFSTR("CFBundleShortVersionString")));
		
		
		SetControlTextWithCFString(appVersionText, locker.returnCFStringRef());
	}
	
	// create an application icon and put it where itÕs supposed to be
	gAboutBoxApplicationIcon = StatusIcon_New(gStandardAboutBoxWindow);
	StatusIcon_AddStageFromIconRef(gAboutBoxApplicationIcon, 'AppI', AppResources_ReturnBundleIconFilenameNoExtension(),
									kConstantsRegistry_ApplicationCreatorSignature,
									kConstantsRegistry_IconServicesIconApplication/* icon type */,
									0L/* ticks of delay in animation - N/A */);
	
	// use the resource-defined user pane boundaries to define the iconÕs boundaries
	{
		ControlRef		iconControl = nullptr;
		Rect			bounds;
		
		
		GetControlBounds(iconUserPane, &bounds);
		iconControl = StatusIcon_ReturnContainerView(gAboutBoxApplicationIcon);
		MoveControl(iconControl, bounds.left, bounds.top);
		SizeControl(iconControl, bounds.right - bounds.left, bounds.bottom - bounds.top);
	}
	
	// make the application name bold and centered, as per the Aqua Human Interface Guidelines;
	// also, center it, because Interface Builder cannot do this for all Mac OS versions
	{
		ControlFontStyleRec		styleRecord;
		OSStatus				error = noErr;
		
		
		styleRecord.flags = kControlUseThemeFontIDMask | kControlAddToMetaFontMask | kControlUseSizeMask | kControlUseJustMask;
		styleRecord.font = kThemeAlertHeaderFont;
		styleRecord.size = 14; // as per Standard About Box specifications in Aqua HIG
		styleRecord.just = teCenter;
		error = SetControlFontStyle(appNameText, &styleRecord);
		assert(error == noErr);
	}
	
	// make the description use the right font and size, as per the Aqua HIG
	{
		ControlFontStyleRec		styleRecord;
		OSStatus				error = noErr;
		
		
		styleRecord.flags = kControlUseThemeFontIDMask | kControlAddToMetaFontMask | kControlUseSizeMask;
		styleRecord.font = kThemeSmallSystemFont;
		styleRecord.size = 11; // as per Standard About Box specifications in Aqua HIG
		error = SetControlFontStyle(appDescText, &styleRecord);
		assert(error == noErr);
	}
	
	// make the version and copyright use the right font and size, as per the Aqua HIG;
	// also, center it, because Interface Builder cannot do this for all Mac OS versions
	{
		ControlFontStyleRec		styleRecord;
		OSStatus				error = noErr;
		
		
		styleRecord.flags = kControlUseThemeFontIDMask | kControlAddToMetaFontMask | kControlUseSizeMask | kControlUseJustMask;
		styleRecord.font = kThemeLabelFont;
		styleRecord.size = 10; // as per Standard About Box specifications in Aqua HIG
		styleRecord.just = teCenter;
		error = SetControlFontStyle(appVersionText, &styleRecord);
		assert(error == noErr);
		error = SetControlFontStyle(appCopyrightTextLine1, &styleRecord);
		assert(error == noErr);
		error = SetControlFontStyle(appCopyrightTextLine2, &styleRecord);
		assert(error == noErr);
		error = SetControlFontStyle(appCopyrightTextLine3, &styleRecord);
		assert(error == noErr);
	}
	
	// install a callback that finds out when the About command or Display Credits commands are chosen
	gCommandExecutionEventListener = ListenerModel_NewOSStatusListener(handleAboutBoxCommand, nullptr/* context */);
	assert(gCommandExecutionEventListener != nullptr);
	Commands_StartHandlingExecution(kCommandAboutThisApplication, gCommandExecutionEventListener);
	Commands_StartHandlingExecution(kCommandCreditsAndLicenseInfo, gCommandExecutionEventListener);
}// Init


/*!
Call this method in the exit method of the program
to ensure that this dialogÕs memory allocations
get destroyed.  You can also call this method after
you are done using this dialog, provided you call
AboutBox_Init() before you use this dialog again.

(3.0)
*/
void
AboutBox_Done ()
{
	Commands_StopHandlingExecution(kCommandAboutThisApplication, gCommandExecutionEventListener);
	Commands_StopHandlingExecution(kCommandCreditsAndLicenseInfo, gCommandExecutionEventListener);
	ListenerModel_ReleaseListener(&gCommandExecutionEventListener);
	
	AboutBox_Remove();
	
	if (gAboutBoxApplicationIcon != nullptr) StatusIcon_Dispose(&gAboutBoxApplicationIcon);
	DisposeWindow(gStandardAboutBoxWindow), gStandardAboutBoxWindow = nullptr;
}// Done


/*!
Displays a Standard About Box, containing
the application icon and basic information
on versioning.  Buttons are also present
to show extra information or check for
updates.

(3.0)
*/
void
AboutBox_Display ()
{
	unless (IsValidWindowRef(gStandardAboutBoxWindow)) AboutBox_Init();
	if (gStandardAboutBoxWindow == nullptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// display the dialog
		unless (IsWindowVisible(gStandardAboutBoxWindow)) ShowWindow(gStandardAboutBoxWindow);
		EventLoop_SelectBehindDialogWindows(gStandardAboutBoxWindow);
		Cursors_UseArrow();
		
		// advance focus (the default button can already be easily hit with
		// the keyboard, so focusing the next button makes it equally easy
		// for the user to choose that option with a key)
		// WARNING: this depends on the front-to-back order in the NIB!!!
		{
			OSStatus		error = noErr;
			
			
			// advance twice: once to focus the default, again to focus the other button
			error = HIViewAdvanceFocus(HIViewGetRoot(gStandardAboutBoxWindow), 0/* modifiers */);
			error = HIViewAdvanceFocus(HIViewGetRoot(gStandardAboutBoxWindow), 0/* modifiers */);
		}
	}
}// Display


/*!
Use this method to hide the Standard About Box.

The dialog remains in memory and can be re-displayed
using AboutBox_Display().  To destroy the dialog,
use the method AboutBox_Done().

(3.0)
*/
void
AboutBox_Remove ()
{
	if (gStandardAboutBoxWindow != nullptr)
	{
		HideWindow(gStandardAboutBoxWindow);
	}
}// Remove


#pragma mark Internal Methods

/*!
Displays the detailed credits and license information window.
The Standard Handler will actually do the right thing for all
the buttons in this window (as the NIB associates command IDs
with all of them) so simply creating and displaying it is
sufficient to handle everything.  Cool, huh?

(3.0)
*/
static void
displayCreditsWindow ()
{
	OSStatus	error = noErr;
	
	
	// in case the window is open when the user invokes this command,
	// do not create a new window; just use the existing one
	unless (IsValidWindowRef(gCreditsWindow))
	{
		// load the NIB containing this dialog (automatically finds the right localization)
		gCreditsWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("AboutBox"), CFSTR("MoreInfo")) << NIBLoader_AssertWindowExists;
		
		// make the headings bold; NOTE that these control IDs MUST AGREE
		// with what is given in the NIB file (errors are ignored because
		// this is purely cosmetic and should not cause anything to fail)
		{
			HIViewID		controlID;
			ControlRef		textControl = nullptr;
			
			
			controlID.signature = 'Cred';
			controlID.id = 0;
			error = GetControlByID(gCreditsWindow, &controlID, &textControl);
			if (error == noErr)
			{
				Localization_SetControlThemeFontInfo(textControl, kThemeAlertHeaderFont);
			}
			controlID.signature = 'LicI';
			controlID.id = 0;
			error = GetControlByID(gCreditsWindow, &controlID, &textControl);
			if (error == noErr)
			{
				Localization_SetControlThemeFontInfo(textControl, kThemeAlertHeaderFont);
			}
		}
		
		// install a callback that responds to the Done button in the window
		{
			EventTypeSpec const		whenButtonClicked[] =
									{
										{ kEventClassCommand, kEventCommandProcess }
									};
			
			
			gButtonHICommandsUPP = NewEventHandlerUPP(receiveCreditsSheetButtonHICommand);
			error = InstallWindowEventHandler(gCreditsWindow, gButtonHICommandsUPP, GetEventTypeCount(whenButtonClicked),
												whenButtonClicked, nullptr/* user data */,
												&gButtonHICommandsHandler/* event handler reference */);
			assert_noerr(error);
		}
		
		error = ShowSheetWindow(gCreditsWindow, gStandardAboutBoxWindow);
		assert_noerr(error);
		
		// advance focus (the default button can already be easily hit with
		// the keyboard, so focusing the next button makes it equally easy
		// for the user to choose that option with a key)
		// WARNING: this depends on the front-to-back order in the NIB!!!
		
		// advance twice: once to focus the default, again to focus the button
		error = HIViewAdvanceFocus(HIViewGetRoot(gCreditsWindow), 0/* modifiers */);
		error = HIViewAdvanceFocus(HIViewGetRoot(gCreditsWindow), 0/* modifiers */);
	}
}// displayCreditsWindow


/*!
Handler for various commands applicable to the About box;
installed in AboutBox_Init().

The result is "eventNotHandledErr" if the command was not
actually executed, or executed completely - this frees other
possible handlers to take a look.  Any other return value
including "noErr" terminates the command sequence.

(3.0)
*/
static OSStatus
handleAboutBoxCommand   (ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inCommandID,
						 void*					UNUSED_ARGUMENT(inEventContextPtr),
						 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	//Commands_ExecutionEventContextPtr	commandInfoPtr = REINTERPRET_CAST(inEventContextPtr, Commands_ExecutionEventContextPtr);
	OSStatus							result = eventNotHandledErr;
	
	
	switch (inCommandID)
	{
	case kCommandAboutThisApplication:
		{
			// since this is also a Dock menu item, it may be executed when MacTelnet isnÕt active;
			// if windows are in the way, the command may appear to have failed; so, the following
			// brings MacTelnet to the front before displaying the About box
			ProcessSerialNumber		psn;
			
			
			if (GetCurrentProcess(&psn) == noErr)
			{
				(OSStatus)SetFrontProcess(&psn);
			}
			AboutBox_Display();
		}
		break;
	
	case kCommandCreditsAndLicenseInfo:
		// open credits sheet
		displayCreditsWindow();
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// handleAboutBoxCommand


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the Done button in the credits sheet.

(3.0)
*/
static pascal OSStatus
receiveCreditsSheetButtonHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef				inEvent,
									 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent),
					kEventKind = GetEventKind(inEvent);
	
	
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
				// close credits sheet
				(OSStatus)HideSheetWindow(gCreditsWindow);
				(OSStatus)RemoveEventHandler(gButtonHICommandsHandler), gButtonHICommandsHandler = nullptr;
				DisposeEventHandlerUPP(gButtonHICommandsUPP), gButtonHICommandsUPP = nullptr;
				DisposeWindow(gCreditsWindow), gCreditsWindow = nullptr;
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldnÕt be able to select menu commands while the sheet is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	return result;
}// receiveCreditsSheetButtonHICommand

// BELOW IS REQUIRED NEWLINE TO END FILE
