/*###############################################################

	SpecialKeySequencesDialog.cp
	
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

// standard-C includes
#include <climits>

// standard-C++ includes
#include <map>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <NIBLoader.h>

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "DialogUtilities.h"
#include "HelpSystem.h"
#include "Keypads.h"
#include "Session.h"
#include "SpecialKeySequencesDialog.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the view IDs in the
"Dialog" NIB from the package "SpecialKeySequencesDialog.nib".
*/
static HIViewID const		idMyButtonChangeInterruptKey	= { 'Intr', 0/* ID */ };
static HIViewID const		idMyButtonChangeSuspendKey		= { 'Susp', 0/* ID */ };
static HIViewID const		idMyButtonChangeResumeKey		= { 'Resu', 0/* ID */ };

#pragma mark Types

struct MySpecialKeysDialog
{
	MySpecialKeysDialog		(SessionRef, SpecialKeySequencesDialog_CloseNotifyProcPtr);
	~MySpecialKeysDialog	();
	
	SpecialKeySequencesDialog_Ref					selfRef;			//!< identical to address of structure, but typed as ref
	SessionRef										session;			//!< the session whose keys are changed by this dialog
	NIBWindow										dialogWindow;		//!< acts as the Mac OS window for the dialog
	HIViewWrap										buttonInterrupt;	//!< when selected, key clicks modify the Interrupt key
	HIViewWrap										buttonResume;		//!< when selected, key clicks modify the Resume key
	HIViewWrap										buttonSuspend;		//!< when selected, key clicks modify the Suspend key
	CarbonEventHandlerWrap							buttonHICommandsHandler;	//!< invoked when a button is clicked
	SpecialKeySequencesDialog_CloseNotifyProcPtr	closeNotifyProc;	//!< routine to call when the dialog is dismissed
	HelpSystem_WindowKeyPhraseSetter				contextualHelpSetup;//!< ensures proper contextual help for this window
	Session_EventKeys								keys;
};

typedef MySpecialKeysDialog*		MySpecialKeysDialogPtr;
typedef MySpecialKeysDialog const*	MySpecialKeysDialogConstPtr;

typedef MemoryBlockPtrLocker< SpecialKeySequencesDialog_Ref, MySpecialKeysDialog >		MySpecialKeysDialogPtrLocker;
typedef LockAcquireRelease< SpecialKeySequencesDialog_Ref, MySpecialKeysDialog >		MySpecialKeysDialogAutoLocker;

typedef std::map< UInt8, CFStringRef >	CharacterToCFStringMap;		//!< e.g. '\0' maps to CFSTR("^@")

#pragma mark Internal Method Prototypes

static CharacterToCFStringMap&	initCharacterToCFStringMap				();
static void						makeAllBevelButtonsUseTheSystemFont		(HIWindowRef);
static pascal OSStatus			receiveHICommand						(EventHandlerCallRef, EventRef, void*);
static OSStatus					setCurrentPendingKeyCharacter			(MySpecialKeysDialogPtr, char);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	MySpecialKeysDialogPtrLocker&	gSpecialKeysDialogPtrLocks ()	{ static MySpecialKeysDialogPtrLocker x; return x; }
	CharacterToCFStringMap&			gCharacterToCFStringMap ()	{ return initCharacterToCFStringMap(); }
}



#pragma mark Public Methods

/*!
Creates a Session and returns a reference to it.  If
any problems occur, nullptr is returned.

In general, you should NOT create sessions this way;
use the Session Factory module.  In particular, note
that Session_New() performs MINIMAL INITIALIZATION,
and you need to call various Session_...() APIs to
set up the new session appropriately.  When finished
initializing, change the session state by calling
Session_SetState(session, kSession_StateInitialized),
so that any modules listening for session state
changes will realize that a new, initialized session
has arrived.

(3.0)
*/
SpecialKeySequencesDialog_Ref
SpecialKeySequencesDialog_New	(SessionRef										inSession,
								 SpecialKeySequencesDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	SpecialKeySequencesDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new MySpecialKeysDialog(inSession, inCloseNotifyProcPtr),
									SpecialKeySequencesDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Destroys a dialog created with SpecialKeySequencesDialog_New().

(3.1)
*/
void
SpecialKeySequencesDialog_Dispose	(SpecialKeySequencesDialog_Ref*		inoutRefPtr)
{
	MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), *inoutRefPtr);
	
	
	if (gSpecialKeysDialogPtrLocks().returnLockCount(*inoutRefPtr) > 1/* auto-locker keeps at least one lock */)
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked dialog; outstanding locks",
						gSpecialKeysDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, MySpecialKeysDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Displays a sheet allowing the user to edit special key
sequences, and returns immediately.

IMPORTANT:	Invoking this routine means it is no longer your
			responsibility to call SpecialKeySequencesDialog_Dispose():
			this is done at an appropriate time after the
			user closes the dialog and after your notification
			routine has been called.

(3.1)
*/
void
SpecialKeySequencesDialog_Display  (SpecialKeySequencesDialog_Ref	inRef)
{
	MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), inRef);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// show the control keys palette
		Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetWindowEventTarget(ptr->dialogWindow));
		
		// display the dialog
		ShowSheetWindow(ptr->dialogWindow, Session_ReturnActiveWindow(ptr->session));
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
}// Display


/*!
Returns the session for which this dialog is setting
special key sequences.

(3.1)
*/
SessionRef
SpecialKeySequencesDialog_ReturnSession		(SpecialKeySequencesDialog_Ref	inRef)
{
	MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), inRef);
	SessionRef						result = nullptr;
	
	
	result = ptr->session;
	return result;
}// ReturnSession


/*!
Responds when the user dismisses a Special Key Sequences
sheet.  Any confirmed changes to the Interrupt, Suspend
and Resume keys are saved in the session configuration.

(3.1)
*/
void
SpecialKeySequencesDialog_StandardCloseNotifyProc	(SpecialKeySequencesDialog_Ref		inDialogThatClosed,
													 Boolean							inOKButtonPressed)
{
	MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), inDialogThatClosed);
	
	
	if (inOKButtonPressed)
	{
		// update the session to reflect the user’s changes
		Session_SetEventKeys(ptr->session, ptr->keys);
	}
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
Constructor.  See SpecialKeySequencesDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
MySpecialKeysDialog::
MySpecialKeysDialog	(SessionRef										inSession,
					 SpecialKeySequencesDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, SpecialKeySequencesDialog_Ref)),
session							(inSession),
dialogWindow					(NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("PrefPanelSessions"), CFSTR("Keyboard"))
									<< NIBLoader_AssertWindowExists),
buttonInterrupt					(dialogWindow.returnHIViewWithID(idMyButtonChangeInterruptKey)
									<< HIViewWrap_AssertExists),
buttonResume					(dialogWindow.returnHIViewWithID(idMyButtonChangeResumeKey)
									<< HIViewWrap_AssertExists),
buttonSuspend					(dialogWindow.returnHIViewWithID(idMyButtonChangeSuspendKey)
									<< HIViewWrap_AssertExists),
buttonHICommandsHandler			(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this->selfRef/* user data */),
closeNotifyProc					(inCloseNotifyProcPtr),
contextualHelpSetup				(this->dialogWindow, kHelpSystem_KeyPhraseSpecialKeys),
keys							(Session_ReturnEventKeys(inSession))
{
	// make all bevel buttons use a larger font than is provided by default in a NIB
	makeAllBevelButtonsUseTheSystemFont(this->dialogWindow);
	
	// make the main bevel buttons use a bold font
	Localization_SetControlThemeFontInfo(this->buttonInterrupt, kThemeAlertHeaderFont);
	Localization_SetControlThemeFontInfo(this->buttonSuspend, kThemeAlertHeaderFont);
	Localization_SetControlThemeFontInfo(this->buttonResume, kThemeAlertHeaderFont);
	
	// initialize other controls
	SetControl32BitValue(this->buttonSuspend, kControlCheckBoxCheckedValue);
	setCurrentPendingKeyCharacter(this, this->keys.suspend);
	SetControl32BitValue(this->buttonSuspend, kControlCheckBoxUncheckedValue);
	SetControl32BitValue(this->buttonResume, kControlCheckBoxCheckedValue);
	setCurrentPendingKeyCharacter(this, this->keys.resume);
	SetControl32BitValue(this->buttonResume, kControlCheckBoxUncheckedValue);
	SetControl32BitValue(this->buttonInterrupt, kControlCheckBoxCheckedValue);
	setCurrentPendingKeyCharacter(this, this->keys.interrupt);
	
	// ensure handlers were installed
	assert(buttonHICommandsHandler.isInstalled());
}// MySpecialKeysDialog 2-argument constructor


/*!
Destructor.  See SpecialKeySequencesDialog_Dispose().

(3.1)
*/
MySpecialKeysDialog::
~MySpecialKeysDialog ()
{
	// remove and dispose callbacks
	DisposeWindow(this->dialogWindow.operator WindowRef());
}// MySpecialKeysDialog destructor


/*!
Returns a reference to a map from character codes (which
may be invisible ASCII) to descriptive strings; the map
is automatically initialized the first time this is called.

(3.1)
*/
static CharacterToCFStringMap&
initCharacterToCFStringMap ()
{
	// instantiate only one of these
	static CharacterToCFStringMap	result;
	
	
	if (result.empty())
	{
		// set up map of ASCII codes to CFStrings, for convenience
		result['@' - '@'] = CFSTR("^@");
		result['A' - '@'] = CFSTR("^A");
		result['B' - '@'] = CFSTR("^B");
		result['C' - '@'] = CFSTR("^C");
		result['D' - '@'] = CFSTR("^D");
		result['E' - '@'] = CFSTR("^E");
		result['F' - '@'] = CFSTR("^F");
		result['G' - '@'] = CFSTR("^G");
		result['H' - '@'] = CFSTR("^H");
		result['I' - '@'] = CFSTR("^I");
		result['J' - '@'] = CFSTR("^J");
		result['K' - '@'] = CFSTR("^K");
		result['L' - '@'] = CFSTR("^L");
		result['M' - '@'] = CFSTR("^M");
		result['N' - '@'] = CFSTR("^N");
		result['O' - '@'] = CFSTR("^O");
		result['P' - '@'] = CFSTR("^P");
		result['Q' - '@'] = CFSTR("^Q");
		result['R' - '@'] = CFSTR("^R");
		result['S' - '@'] = CFSTR("^S");
		result['T' - '@'] = CFSTR("^T");
		result['U' - '@'] = CFSTR("^U");
		result['V' - '@'] = CFSTR("^V");
		result['W' - '@'] = CFSTR("^W");
		result['X' - '@'] = CFSTR("^X");
		result['Y' - '@'] = CFSTR("^Y");
		result['Z' - '@'] = CFSTR("^Z");
		result['[' - '@'] = CFSTR("^[");
		result['\\' - '@'] = CFSTR("^\\");
		result[']' - '@'] = CFSTR("^]");
		result['~' - '@'] = CFSTR("^~");
		result['\?' - '@'] = CFSTR("^?");
		assert(result.size() == 32/* number of entries above */);
	}
	
	return result;
}// initializeCharacterStringMap


/*!
Changes the theme font of all bevel button controls
in the specified window to use the system font
(which automatically sets the font size).  By default
a NIB makes bevel buttons use a small font, so this
routine corrects that.

(3.0)
*/
static void
makeAllBevelButtonsUseTheSystemFont		(HIWindowRef	inWindow)
{
	OSStatus	error = noErr;
	HIViewRef	contentView = nullptr;
	
	
	error = HIViewFindByID(HIViewGetRoot(inWindow), kHIViewWindowContentID, &contentView);
	if (error == noErr)
	{
		ControlKind		kindInfo;
		HIViewRef		button = HIViewGetFirstSubview(contentView);
		
		
		while (nullptr != button)
		{
			error = GetControlKind(button, &kindInfo);
			if (noErr == error)
			{
				if ((kControlKindSignatureApple == kindInfo.signature) &&
					(kControlKindBevelButton == kindInfo.kind))
				{
					// only change bevel buttons
					Localization_SetControlThemeFontInfo(button, kThemeSystemFont);
				}
			}
			button = HIViewGetNextView(button);
		}
	}
}// makeAllButtonsUseTheSystemFont


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the special key sequences dialog.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inDialogRef)
{
	OSStatus						result = eventNotHandledErr;
	SpecialKeySequencesDialog_Ref	ref = REINTERPRET_CAST(inDialogRef, SpecialKeySequencesDialog_Ref);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			enum
			{
				kInitialChosenCharValue		= 'X'
			};
			char	chosenChar = kInitialChosenCharValue;
			
			
			switch (received.commandID)
			{
			case kHICommandOK:
				{
					MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), ref);
					
					
					// user accepted - close the dialog with an appropriate transition for acceptance
					HideSheetWindow(ptr->dialogWindow);
					
					// notify of close
					if (nullptr != ptr->closeNotifyProc)
					{
						SpecialKeySequencesDialog_InvokeCloseNotifyProc(ptr->closeNotifyProc, ptr->selfRef,
																		true/* OK was pressed */);
					}
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				SpecialKeySequencesDialog_Dispose(&ref);
				// reset the keypad
				Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, nullptr);
				break;
			
			case kHICommandCancel:
				{
					MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), ref);
					
					
					// user cancelled - close the dialog with an appropriate transition for cancelling
					HideSheetWindow(ptr->dialogWindow);
					
					// notify of close
					if (nullptr != ptr->closeNotifyProc)
					{
						SpecialKeySequencesDialog_InvokeCloseNotifyProc(ptr->closeNotifyProc, ptr->selfRef,
																		false/* OK was pressed */);
					}
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				SpecialKeySequencesDialog_Dispose(&ref);
				// reset the keypad
				Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, nullptr);
				break;
			
			case kCommandEditInterruptKey:
				{
					MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), ref);
					
					
					// show the control keys palette and target the button
					Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetWindowEventTarget(ptr->dialogWindow));
					
					// change the active button
					SetControl32BitValue(ptr->buttonInterrupt, kControlCheckBoxCheckedValue);
					SetControl32BitValue(ptr->buttonSuspend, kControlCheckBoxUncheckedValue);
					SetControl32BitValue(ptr->buttonResume, kControlCheckBoxUncheckedValue);
				}
				break;
			
			case kCommandEditSuspendKey:
				{
					MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), ref);
					
					
					// show the control keys palette and target the button
					Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetWindowEventTarget(ptr->dialogWindow));
					
					// change the active button
					SetControl32BitValue(ptr->buttonInterrupt, kControlCheckBoxUncheckedValue);
					SetControl32BitValue(ptr->buttonSuspend, kControlCheckBoxCheckedValue);
					SetControl32BitValue(ptr->buttonResume, kControlCheckBoxUncheckedValue);
				}
				break;
			
			case kCommandEditResumeKey:
				{
					MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), ref);
					
					
					// show the control keys palette and target the button
					Keypads_SetEventTarget(kKeypads_WindowTypeControlKeys, GetWindowEventTarget(ptr->dialogWindow));
					
					// change the active button
					SetControl32BitValue(ptr->buttonInterrupt, kControlCheckBoxUncheckedValue);
					SetControl32BitValue(ptr->buttonSuspend, kControlCheckBoxUncheckedValue);
					SetControl32BitValue(ptr->buttonResume, kControlCheckBoxCheckedValue);
				}
				break;
			
			case kCommandKeypadControlAtSign:
				chosenChar = '@' - '@';
				break;
			
			case kCommandKeypadControlA:
				chosenChar = 'A' - '@';
				break;
			
			case kCommandKeypadControlB:
				chosenChar = 'B' - '@';
				break;
			
			case kCommandKeypadControlC:
				chosenChar = 'C' - '@';
				break;
			
			case kCommandKeypadControlD:
				chosenChar = 'D' - '@';
				break;
			
			case kCommandKeypadControlE:
				chosenChar = 'E' - '@';
				break;
			
			case kCommandKeypadControlF:
				chosenChar = 'F' - '@';
				break;
			
			case kCommandKeypadControlG:
				chosenChar = 'G' - '@';
				break;
			
			case kCommandKeypadControlH:
				chosenChar = 'H' - '@';
				break;
			
			case kCommandKeypadControlI:
				chosenChar = 'I' - '@';
				break;
			
			case kCommandKeypadControlJ:
				chosenChar = 'J' - '@';
				break;
			
			case kCommandKeypadControlK:
				chosenChar = 'K' - '@';
				break;
			
			case kCommandKeypadControlL:
				chosenChar = 'L' - '@';
				break;
			
			case kCommandKeypadControlM:
				chosenChar = 'M' - '@';
				break;
			
			case kCommandKeypadControlN:
				chosenChar = 'N' - '@';
				break;
			
			case kCommandKeypadControlO:
				chosenChar = 'O' - '@';
				break;
			
			case kCommandKeypadControlP:
				chosenChar = 'P' - '@';
				break;
			
			case kCommandKeypadControlQ:
				chosenChar = 'Q' - '@';
				break;
			
			case kCommandKeypadControlR:
				chosenChar = 'R' - '@';
				break;
			
			case kCommandKeypadControlS:
				chosenChar = 'S' - '@';
				break;
			
			case kCommandKeypadControlT:
				chosenChar = 'T' - '@';
				break;
			
			case kCommandKeypadControlU:
				chosenChar = 'U' - '@';
				break;
			
			case kCommandKeypadControlV:
				chosenChar = 'V' - '@';
				break;
			
			case kCommandKeypadControlW:
				chosenChar = 'W' - '@';
				break;
			
			case kCommandKeypadControlX:
				chosenChar = 'X' - '@';
				break;
			
			case kCommandKeypadControlY:
				chosenChar = 'Y' - '@';
				break;
			
			case kCommandKeypadControlZ:
				chosenChar = 'Z' - '@';
				break;
			
			case kCommandKeypadControlLeftSquareBracket:
				chosenChar = '[' - '@';
				break;
			
			case kCommandKeypadControlBackslash:
				chosenChar = '\\' - '@';
				break;
			
			case kCommandKeypadControlRightSquareBracket:
				chosenChar = ']' - '@';
				break;
			
			case kCommandKeypadControlTilde:
				chosenChar = '~' - '@';
				break;
			
			case kCommandKeypadControlQuestionMark:
				chosenChar = '\?' - '@';
				break;
			
			case kCommandContextSensitiveHelp:
				// open the Help Viewer to the right topic
				HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseSpecialKeys);
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the window is open
				result = eventNotHandledErr;
				break;
			}
			
			// if the chosen character is not the default value, a control key in
			// the palette was clicked; use it to change the Interrupt, Suspend or
			// Resume key (whichever is active)
			if (chosenChar != kInitialChosenCharValue)
			{
				MySpecialKeysDialogAutoLocker	ptr(gSpecialKeysDialogPtrLocks(), ref);
				
				
				(OSStatus)setCurrentPendingKeyCharacter(ptr, chosenChar);
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Changes the appropriate pending-key variable to the
selected value, based on which button is currently
highlighted by the user.  Also updates the control
title of the highlighted button.

\retval noErr
if successful

\retval paramErr
if the given character is not supported

\retval (other)
any OSStatus value based on Mac OS APIs used

(3.0)
*/
static OSStatus
setCurrentPendingKeyCharacter   (MySpecialKeysDialogPtr		inPtr,
								 char						inCharacter)
{
	CFStringRef		newTitle = gCharacterToCFStringMap()[inCharacter];
	HIViewRef		view = nullptr;
	OSStatus		result = noErr;
	
	
	if (newTitle == nullptr) result = paramErr;
	else
	{
		// one of the 3 buttons should always be active
		if (GetControlValue(inPtr->buttonInterrupt) == kControlCheckBoxCheckedValue)
		{
			inPtr->keys.interrupt = inCharacter;
			view = inPtr->buttonInterrupt;
		}
		else if (GetControlValue(inPtr->buttonSuspend) == kControlCheckBoxCheckedValue)
		{
			inPtr->keys.suspend = inCharacter;
			view = inPtr->buttonSuspend;
		}
		else if (GetControlValue(inPtr->buttonResume) == kControlCheckBoxCheckedValue)
		{
			inPtr->keys.resume = inCharacter;
			view = inPtr->buttonResume;
		}
		else
		{
			// ???
			result = paramErr;
		}
		result = SetControlTitleWithCFString(view, newTitle);
	}
	
	return result;
}// setCurrentPendingKeyCharacter

// BELOW IS REQUIRED NEWLINE TO END FILE
