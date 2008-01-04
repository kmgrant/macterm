/*###############################################################

	DuplicateDialog.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <CommonEventHandlers.h>
#include <DialogAdjust.h>
#include <HIViewWrap.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "Console.h"
#include "DialogUtilities.h"
#include "DuplicateDialog.h"
#include "HelpSystem.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Dialog" NIB from the package "DuplicateDialog.nib".
*/
enum
{
	kSignatureMyButtonCreate			= FOUR_CHAR_CODE('Crea'),
	kSignatureMyButtonCancel			= FOUR_CHAR_CODE('Canc'),
	kSignatureMyButtonHelp				= FOUR_CHAR_CODE('Help'),
	kSignatureMyLabelNewName			= FOUR_CHAR_CODE('TLbl'),
	kSignatureMyFieldNewName			= FOUR_CHAR_CODE('Titl')
};
static HIViewID const		idMyButtonCreate	= { kSignatureMyButtonCreate,   0/* ID */ };
static HIViewID const		idMyButtonCancel	= { kSignatureMyButtonCancel,   0/* ID */ };
static HIViewID const		idMyButtonHelp		= { kSignatureMyButtonHelp,		0/* ID */ };
static HIViewID const		idMyLabelNewName	= { kSignatureMyLabelNewName,   0/* ID */ };
static HIViewID const		idMyFieldNewName	= { kSignatureMyFieldNewName,   0/* ID */ };

#pragma mark Types

struct My_DuplicateDialog
{	
	HIWindowRef								dialogWindow;				//!< the dialog’s window
	ControlRef								buttonCreate,				//!< Create button
											buttonCancel,				//!< Cancel button
											labelNewName,				//!< the label for the name text field
											fieldNewName;				//!< the text field containing the name of the duplicate
	HIViewWrap								buttonHelp;					//!< help button
	
	DuplicateDialog_CloseNotifyProcPtr		closeNotifyProc;			//!< routine to call when the dialog is dismissed
	void*									closeNotifyProcUserData;	//!< passed to the close notification routine
	EventHandlerUPP							buttonHICommandsUPP;		//!< wrapper for button callback function
	EventHandlerRef							buttonHICommandsHandler;	//!< invoked when a dialog button is clicked
	CommonEventHandlers_WindowResizer		windowResizeHandler;		//!< invoked when a window has been resized
	DuplicateDialog_Ref						selfRef;					//!< identical to address of structure, but typed as ref
};
typedef My_DuplicateDialog*		My_DuplicateDialogPtr;

typedef MemoryBlockPtrLocker< DuplicateDialog_Ref, My_DuplicateDialog >		My_DuplicateDialogPtrLocker;

#pragma mark Internal Method Prototypes

static Boolean				handleItemHit			(My_DuplicateDialogPtr, HIViewID const&);

static void					handleNewSize			(WindowRef, Float32, Float32, void*);

static pascal OSStatus		receiveHICommand		(EventHandlerCallRef, EventRef, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_DuplicateDialogPtrLocker&	gDuplicateDialogPtrLocks()	{ static My_DuplicateDialogPtrLocker x; return x; }
}



#pragma mark Public Methods

/*!
This method is used to initialize the Duplicate dialog
box.  It creates the dialog box invisibly, and searches
only the contents of the specified window.

(3.0)
*/
DuplicateDialog_Ref
DuplicateDialog_New		(DuplicateDialog_CloseNotifyProcPtr		inCloseNotifyProcPtr,
						 CFStringRef							inInitialNameString,
						 void*									inCloseNotifyProcUserData)
{
	DuplicateDialog_Ref		result = REINTERPRET_CAST(new My_DuplicateDialog, DuplicateDialog_Ref);
	
	
	if (result != nullptr)
	{
		My_DuplicateDialogPtr	ptr = gDuplicateDialogPtrLocks().acquireLock(result);
		
		
		// load the NIB containing this dialog (automatically finds the right localization)
		ptr->dialogWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
										CFSTR("DuplicateDialog"), CFSTR("Dialog"))
							<< NIBLoader_AssertWindowExists;
		
		ptr->closeNotifyProc = inCloseNotifyProcPtr;
		ptr->closeNotifyProcUserData = inCloseNotifyProcUserData;
		ptr->selfRef = result; // handy to avoid multiple casts and other problems elsewhere
		
		// find references to all controls that are needed for any operation
		// (button clicks, dealing with text or responding to window resizing)
		{
			OSStatus	error = noErr;
			
			
			error = GetControlByID(ptr->dialogWindow, &idMyButtonCreate, &ptr->buttonCreate);
			assert(error == noErr);
			error = GetControlByID(ptr->dialogWindow, &idMyButtonCancel, &ptr->buttonCancel);
			assert(error == noErr);
			error = GetControlByID(ptr->dialogWindow, &idMyLabelNewName, &ptr->labelNewName);
			assert(error == noErr);
			error = GetControlByID(ptr->dialogWindow, &idMyFieldNewName, &ptr->fieldNewName);
			assert(error == noErr);
			ptr->buttonHelp.setCFTypeRef(HIViewWrap(idMyButtonHelp, ptr->dialogWindow));
			assert(ptr->buttonHelp.exists());
		}
		
		// make the help button icon appearance and state appropriate
		DialogUtilities_SetUpHelpButton(ptr->buttonHelp);
		
		if (inInitialNameString != nullptr)
		{
			// initialize the name field
			SetControlTextWithCFString(ptr->fieldNewName, inInitialNameString);
		}
		
		// install a callback that responds to buttons in the window
		{
			EventTypeSpec const		whenButtonClicked[] =
									{
										{ kEventClassCommand, kEventCommandProcess }
									};
			OSStatus				error = noErr;
			
			
			ptr->buttonHICommandsUPP = NewEventHandlerUPP(receiveHICommand);
			error = InstallWindowEventHandler(ptr->dialogWindow, ptr->buttonHICommandsUPP, GetEventTypeCount(whenButtonClicked),
												whenButtonClicked, result/* user data */,
												&ptr->buttonHICommandsHandler/* event handler reference */);
			assert(error == noErr);
		}
		
		// install a callback that responds as a window is resized
		{
			Rect		currentBounds;
			OSStatus	error = noErr;
			
			
			error = GetWindowBounds(ptr->dialogWindow, kWindowContentRgn, &currentBounds);
			assert_noerr(error);
			ptr->windowResizeHandler.install(ptr->dialogWindow, handleNewSize, result/* user data */,
												currentBounds.right - currentBounds.left/* minimum width */,
												currentBounds.bottom - currentBounds.top/* minimum height */,
												currentBounds.right - currentBounds.left + 50/* arbitrary maximum width */,
												currentBounds.bottom - currentBounds.top/* maximum height */);
			assert(ptr->windowResizeHandler.isInstalled());
		}
		
		gDuplicateDialogPtrLocks().releaseLock(result, &ptr);
	}
	
	return result;
}// New


/*!
Call this method to destroy the duplicate dialog
box and its associated data structures.  On return,
your copy of the dialog reference is set to nullptr.

(3.0)
*/
void
DuplicateDialog_Dispose		(DuplicateDialog_Ref*	inoutDialogPtr)
{
	if (inoutDialogPtr != nullptr)
	{
		My_DuplicateDialogPtr	ptr = gDuplicateDialogPtrLocks().acquireLock(*inoutDialogPtr);
		
		
		// clean up the Help System
		HelpSystem_SetWindowKeyPhrase(ptr->dialogWindow, kHelpSystem_KeyPhraseDefault);
		
		// release all memory occupied by the dialog
		RemoveEventHandler(ptr->buttonHICommandsHandler);
		DisposeEventHandlerUPP(ptr->buttonHICommandsUPP);
		DisposeWindow(ptr->dialogWindow);
		delete ptr;
		gDuplicateDialogPtrLocks().releaseLock(*inoutDialogPtr, &ptr);
		*inoutDialogPtr = nullptr;
	}
}// Dispose


/*!
This method displays and handles events in the
Duplicate dialog box.  When the user clicks a
button in the dialog, its disposal callback is
invoked.

(3.0)
*/
void
DuplicateDialog_Display		(DuplicateDialog_Ref	inDialog,
							 WindowRef				inParentWindow)
{
	My_DuplicateDialogPtr	ptr = gDuplicateDialogPtrLocks().acquireLock(inDialog);
	
	
	if (ptr == nullptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// display the dialog
		ShowSheetWindow(ptr->dialogWindow, inParentWindow);
		
		// set keyboard focus
		(OSStatus)SetKeyboardFocus(ptr->dialogWindow, ptr->fieldNewName, kControlEditTextPart);
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
	gDuplicateDialogPtrLocks().releaseLock(inDialog, &ptr);
}// Display


/*!
Returns the string most recently entered into the
search field by the user, WITHOUT retaining it.

(3.0)
*/
void
DuplicateDialog_GetNameString	(DuplicateDialog_Ref	inDialog,
								 CFStringRef&			outString)
{
	My_DuplicateDialogPtr	ptr = gDuplicateDialogPtrLocks().acquireLock(inDialog);
	
	
	if (ptr != nullptr)
	{
		GetControlTextAsCFString(ptr->fieldNewName, outString);
	}
	gDuplicateDialogPtrLocks().releaseLock(inDialog, &ptr);
}// GetNameString


/*!
If you only need a close notification procedure for
the purpose of disposing of the Duplicate Dialog
reference (and don’t otherwise care when a Duplicate
Dialog closes), you can pass this standard routine to
DuplicateDialog_New() as your notification procedure.

(3.0)
*/
void
DuplicateDialog_StandardCloseNotifyProc		(DuplicateDialog_Ref	inDialogThatClosed,
											 Boolean				UNUSED_ARGUMENT(inOKButtonPressed),
											 void*					UNUSED_ARGUMENT(inUserData))
{
	DuplicateDialog_Dispose(&inDialogThatClosed);
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
Responds to control selections in the dialog box.
If a button is pressed, the window is closed and
the close notifier routine is called; otherwise,
an adjustment is made to the display.

This routine has the option of changing the item
index; this will only affect the modal dialog
(i.e. it has no effect for the sheet).

Returns "true" only if the specified item hit is
to be IGNORED.

(3.0)
*/
static Boolean
handleItemHit	(My_DuplicateDialogPtr	inPtr,
				 HIViewID const&		inHIViewID)
{
	Boolean		result = false;
	
	
	switch (inHIViewID.signature)
	{
	case kSignatureMyButtonCreate:
		// close the dialog
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (inPtr->closeNotifyProc != nullptr)
		{
			DuplicateDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef,
													true/* OK pressed */, inPtr->closeNotifyProcUserData);
		}
		break;
	
	case kSignatureMyButtonCancel:
		// user cancelled - close the dialog with an appropriate transition for cancelling
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (inPtr->closeNotifyProc != nullptr)
		{
			DuplicateDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef,
													false/* OK pressed */, inPtr->closeNotifyProcUserData);
		}
		break;
	
	case kSignatureMyButtonHelp:
		HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseDuplicate);
		break;
	
	default:
		break;
	}
	
	return result;
}// handleItemHit


/*!
Moves or resizes the controls in Duplicate dialogs.

(3.0)
*/
static void
handleNewSize	(WindowRef	UNUSED_ARGUMENT(inWindow),
				 Float32	inDeltaX,
				 Float32	inDeltaY,
				 void*		inDuplicateDialogRef)
{
	// only horizontal changes are significant to this dialog
	if (inDeltaX)
	{
		DuplicateDialog_Ref		ref = REINTERPRET_CAST(inDuplicateDialogRef, DuplicateDialog_Ref);
		My_DuplicateDialogPtr	ptr = gDuplicateDialogPtrLocks().acquireLock(ref);
		SInt32					truncDeltaX = STATIC_CAST(inDeltaX, SInt32);
		SInt32					truncDeltaY = STATIC_CAST(inDeltaY, SInt32);
		
		
		DialogAdjust_BeginControlAdjustment(ptr->dialogWindow);
		{
			// controls which are moved horizontally
			if (Localization_IsLeftToRight())
			{
				DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonCreate, truncDeltaX);
				DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonCancel, truncDeltaX);
			}
			else
			{
				DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonHelp, truncDeltaX);
			}
			
			// controls which are resized horizontally
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->labelNewName, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->fieldNewName, truncDeltaX);
		}
		DialogAdjust_EndAdjustment(truncDeltaX, truncDeltaY); // moves and resizes controls properly
		gDuplicateDialogPtrLocks().releaseLock(ref, &ptr);
	}
}// handleNewSize


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in duplicate dialogs.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inDuplicateDialogRef)
{
	OSStatus				result = eventNotHandledErr;
	DuplicateDialog_Ref		ref = REINTERPRET_CAST(inDuplicateDialogRef, DuplicateDialog_Ref);
	My_DuplicateDialogPtr	ptr = gDuplicateDialogPtrLocks().acquireLock(ref);
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
				// accept new name
				if (handleItemHit(ptr, idMyButtonCreate)) result = eventNotHandledErr;
				break;
			
			case kHICommandCancel:
				if (handleItemHit(ptr, idMyButtonCancel)) result = eventNotHandledErr;
				break;
			
			case kCommandContextSensitiveHelp:
				// open the Help Viewer to the right topic
				if (handleItemHit(ptr, idMyButtonHelp)) result = eventNotHandledErr;
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the sheet is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	gDuplicateDialogPtrLocks().releaseLock(ref, &ptr);
	return result;
}// receiveHICommand

// BELOW IS REQUIRED NEWLINE TO END FILE
