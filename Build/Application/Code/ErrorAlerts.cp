/*###############################################################

	ErrorAlerts.cp
	
	MacTelnet
		© 1998-2007 by Kevin Grant.
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
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <Console.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// resource includes
#include "DialogResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "DialogUtilities.h"
#include "ErrorAlerts.h"
#include "MainEntryPoint.h"
#include "RecordAE.h"
#include "Terminology.h"
#include "UIStrings.h"



#pragma mark Public Methods

/*!
Call this routine before calling any other
routines from this module.

(3.0)
*/
void
ErrorAlerts_Init ()
{
}// Init


/*!
Call this routine when you are finished
using this module.

(3.0)
*/
void
ErrorAlerts_Done ()
{
}// Done


/*!
Displays the specified Note alert to the user
with no help text, emitting no system beep.

(3.0)
*/
void
ErrorAlerts_DisplayNoteMessage	(SInt16							inStringListResourceID,
								 UInt16							inStringIndex,
								 UIStrings_AlertWindowCFString	inTitleStringID)
{
	Str255				dialogText;
	CFStringRef			titleCFString = nullptr;
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	
	
	GetIndString(dialogText, inStringListResourceID, inStringIndex);
	stringResult = UIStrings_Copy(inTitleStringID, titleCFString);
	
	// send a Display Alert Apple Event
	{
		TelnetAlertParameterBlock	parameters;
		
		
		bzero(&parameters, sizeof(parameters));
		parameters.dialog.type = kAlertNoteAlert;
		parameters.text.large = dialogText;
		if (stringResult.ok())
		{
			parameters.text.titleCFString = titleCFString;
		}
		(SInt16)ErrorAlerts_DisplayParameterizedAlert(&parameters);
	}
	CFRelease(titleCFString);
}// DisplayNoteMessage


/*!
Displays the specified Note alert to the user
with no help text or title, emitting no system
beep.

(3.0)
*/
void
ErrorAlerts_DisplayNoteMessageNoTitle	(SInt16		inStringListResourceID,
										 UInt16		inStringIndex)
{
	Str255		dialogText;
	
	
	GetIndString(dialogText, inStringListResourceID, inStringIndex);
	
	// send a Display Alert Apple Event
	{
		TelnetAlertParameterBlock	parameters;
		
		
		bzero(&parameters, sizeof(parameters));
		parameters.dialog.type = kAlertNoteAlert;
		parameters.text.large = dialogText;
		(SInt16)ErrorAlerts_DisplayParameterizedAlert(&parameters);
	}
}// DisplayNoteMessageNoTitle


/*!
To display an alert described in a parameter block by
sending a Display Alert Apple Event back to MacTelnet,
use this method.  The return value is one of the
standard Mac OS "kAlertStdAlert...Button" constants,
or -1 to indicate that the alert timed out with no
specific button being chosen.

Incomplete.

(3.0)
*/
SInt16
ErrorAlerts_DisplayParameterizedAlert	(TelnetAlertParameterBlockConstPtr		inParameters)
{
	SInt16		result = -1; // a "kAlertStdAlert...Button" constant, or -1 for “timed out”
	
	
	if (inParameters != nullptr)
	{
		// send a Display Alert Apple Event
		AppleEvent		displayAlertEvent;
		AppleEvent		reply;
		ConstStringPtr	string = nullptr;
		
		
		Alert_ReportOSStatus(RecordAE_CreateRecordableAppleEvent(kMySuite, kTelnetEventIDAlert,
																	&displayAlertEvent));
		
		// text parameters
		{
			// dialog text - REQUIRED
			string = inParameters->text.large;
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyDirectObject,
									cString, string + 1, PLstrlen(string) * sizeof(UInt8));
		}
		if (inParameters->text.small != nullptr)
		{
			// help text
			string = inParameters->text.small;
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMyHelpText,
									cString, string + 1, PLstrlen(string) * sizeof(UInt8));
		}
		
		// title parameter
		if (inParameters->text.titleCFString != nullptr)
		{
			// alert title as CFString
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMyName,
									typeCFStringRef, &inParameters->text.titleCFString,
									sizeof(inParameters->text.titleCFString));
		}
		
		// button parameters - UNIMPLEMENTED
		{
		}
		
		// flag parameters
		{
			// alert sound
			Boolean		alertBeeps = inParameters->flags.alertSound;
			
			
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMyAlertSound,
									typeBoolean, &alertBeeps, sizeof(alertBeeps));
		}
		{
			// help button
			Boolean		isHelpButton = inParameters->flags.helpButton;
			
			
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMyHelpButton,
									typeBoolean, &isHelpButton, sizeof(isHelpButton));
		}
		{
			// network disruption
			Boolean		isNetworkDisruption = inParameters->flags.networkDisruption;
			
			
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMyNetworkDisruption,
									typeBoolean, &isNetworkDisruption, sizeof(isNetworkDisruption));
		}
		{
			// speech
			Boolean		isSpeech = inParameters->flags.speech;
			
			
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMySpeech,
									typeBoolean, &isSpeech, sizeof(isSpeech));
		}
		{
			// title bar
			Boolean		isMovable = !inParameters->flags.notTitled;
			
			
			(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMyTitled,
									typeBoolean, &isMovable, sizeof(isMovable));
		}
		
		// icon parameters
		{
			// alert icon
			FourCharCode	alertType = kTelnetEnumeratedClassAlertIconTypeNote;
			Boolean			doSet = true;
			
			
			switch (inParameters->dialog.type)
			{
				case kAlertNoteAlert:
					alertType = kTelnetEnumeratedClassAlertIconTypeNote;
					break;
				
				case kAlertStopAlert:
					alertType = kTelnetEnumeratedClassAlertIconTypeStop;
					break;
				
				case kAlertCautionAlert:
					alertType = kTelnetEnumeratedClassAlertIconTypeCaution;
					break;
				
				default:
					doSet = false;
					break;
			}
			if (doSet)
			{
				(OSStatus)AEPutParamPtr(&displayAlertEvent, keyAEParamMyWithIcon,
										typeEnumerated, &alertType, sizeof(alertType));
			}
		}
		
		// display the alert by sending an Apple Event; if it fails, report the error
		Alert_ReportOSStatus(AESend(&displayAlertEvent, &reply,
									kAENoReply | kAENeverInteract,// | kAEDontRecord,
									kAENormalPriority, kAEDefaultTimeout, nullptr, nullptr));
		AEDisposeDesc(&displayAlertEvent);
		
		// reply - unimplemented
	}
	
	return result;
}// DisplayParameterizedAlert


/*!
Displays the specified Stop alert to the user
with no help text, emitting a system beep.

(3.0)
*/
void
ErrorAlerts_DisplayStopMessage	(SInt16							inStringListResourceID,
								 UInt16							inStringIndex,
								 UIStrings_AlertWindowCFString	inTitleStringID)
{
	Str255				dialogText;
	CFStringRef			titleCFString = nullptr;
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	
	
	GetIndString(dialogText, inStringListResourceID, inStringIndex);
	stringResult = UIStrings_Copy(inTitleStringID, titleCFString);
	
	// send a Display Alert Apple Event
	{
		TelnetAlertParameterBlock	parameters;
		
		
		bzero(&parameters, sizeof(parameters));
		parameters.dialog.type = kAlertStopAlert;
		parameters.flags.alertSound = true;
		parameters.text.large = dialogText;
		if (stringResult.ok())
		{
			parameters.text.titleCFString = titleCFString;
		}
		(SInt16)ErrorAlerts_DisplayParameterizedAlert(&parameters);
	}
	CFRelease(titleCFString);
}// DisplayStopMessage


/*!
Displays the specified Stop alert to the user with
no help text or title, emitting a system beep.

(3.0)
*/
void
ErrorAlerts_DisplayStopMessageNoTitle	(SInt16		inStringListResourceID,
										 UInt16		inStringIndex)
{
	InterfaceLibAlertRef	box = nullptr;
	Str255					dialogText;
	
	
	GetIndString(dialogText, inStringListResourceID, inStringIndex);
	box = Alert_New();
	Alert_SetParamsFor(box, kAlert_StyleCancel);
	Alert_SetHelpButton(box, false);
	Alert_SetText(box, dialogText, "\p");
	Alert_SetType(box, kAlertStopAlert);
	Sound_StandardAlert();
	Alert_Display(box);
	Alert_Dispose(&box);
}// DisplayStopMessageNoTitle


/*!
Displays the specified Stop alert to the user
with no help text, emitting a system beep.

When the user clicks the Quit button in the
alert, the program is forced to quit.  Thus,
this routine will not return.

WARNING:	Do not implement this routine using
			Apple Events; it may need to be
			called early in the initialization
			code, possibly before Apple Events
			are configured properly.  Display
			this alert the “old-fashioned” way.

(3.0)
*/
void
ErrorAlerts_DisplayStopQuitMessage	(SInt16							inStringListResourceID,
									 UInt16							inStringIndex,
									 UIStrings_AlertWindowCFString	inTitleStringID)
{
	InterfaceLibAlertRef	box = nullptr;
	Str255					dialogText;
	
	
	GetIndString(dialogText, inStringListResourceID, inStringIndex);
	box = Alert_New();
	Alert_SetParamsFor(box, kAlert_StyleCancel);
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			buttonString = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_ButtonQuit, buttonString);
		if (stringResult.ok())
		{
			Alert_SetButtonText(box, kAlertStdAlertOKButton, buttonString);
			CFRelease(buttonString), buttonString = nullptr;
		}
	}
	Alert_SetHelpButton(box, false);
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			titleCFString = nullptr;
		
		
		stringResult = UIStrings_Copy(inTitleStringID, titleCFString);
		if (stringResult.ok())
		{
			Alert_SetTitleCFString(box, titleCFString);
			CFRelease(titleCFString);
		}
	}
	Alert_SetText(box, dialogText, EMPTY_PSTRING);
	Alert_SetType(box, kAlertStopAlert);
	Sound_StandardAlert();
	Alert_Display(box);
	Alert_Dispose(&box);
	
	// shut down MacTelnet
	MainEntryPoint_ImmediatelyQuit();
}// DisplayStopQuitMessage


/*!
Rewritten in MacTelnet 3.0 to use standard
Appearance-savvy alert windows.  The given
message ID must refer to a string index in
the string list resource of ID
"rStringsOperationFailed".

(3.0)
*/
void
ErrorAlerts_OperationFailed		(short		inMessageID,
								 short		inInternalID,
								 OSStatus	inOSID)
{
	Str255		dialogText,
				helpText,
				errorString;
	Str31		osErrorString,
				internalErrorString;
	
	
	GetIndString(dialogText, rStringsGeneralMessages, siCommandCannotContinue);
	GetIndString(errorString, rStringsOperationFailed, inMessageID);
	{
		StringSubstitutionSpec const	metaMappings[] =
										{
											{ kStringSubstitutionDefaultTag1, errorString }
										};
		
		
		StringUtilities_PBuild(dialogText,
								sizeof(metaMappings) / sizeof(StringSubstitutionSpec),
								metaMappings);
	}
	GetIndString(helpText, rStringsGeneralMessages, siOSErrAndInternalErrHelpText);
	NumToString(inOSID, osErrorString);
	NumToString(STATIC_CAST(inInternalID, SInt32), internalErrorString);
	{
		StringSubstitutionSpec const	metaMappings[] =
										{
											{ kStringSubstitutionDefaultTag1, osErrorString },
											{ kStringSubstitutionDefaultTag2, internalErrorString }
										};
		
		
		StringUtilities_PBuild(helpText,
								sizeof(metaMappings) / sizeof(StringSubstitutionSpec),
								metaMappings);
	}
	Console_WriteValuePString("Failed", dialogText);
	Console_WriteValuePString("Reason", helpText);
	
	// send a Display Alert Apple Event
	{
		TelnetAlertParameterBlock	parameters;
		
		
		bzero(&parameters, sizeof(parameters));
		parameters.dialog.type = kAlertStopAlert;
		parameters.flags.alertSound = true;
		parameters.text.large = dialogText;
		parameters.text.small = helpText;
		(SInt16)ErrorAlerts_DisplayParameterizedAlert(&parameters);
	}
}// OperationFailed


/*!
Reports an “out of memory” error.  The specific
point in the MacTelnet source code where the error
took place is hinted by the internal error code
(see "ErrorAlerts.h" for valid values).

(3.0)
*/
void
ErrorAlerts_OutOfMemory		(InternalTelnetMemoryError		inInternalID)
{
	ErrorAlerts_OperationFailed(siOpFailedOutOfMemory, inInternalID, 0);
}// OutOfMemory

// BELOW IS REQUIRED NEWLINE TO END FILE
