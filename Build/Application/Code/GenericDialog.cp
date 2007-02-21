/*###############################################################

	GenericDialog.cp
	
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
#include <climits>
#include <cstdlib>
#include <cstring>
#include <utility>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <SoundSystem.h>

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "DialogUtilities.h"
#include "GenericDialog.h"
#include "HelpSystem.h"
#include "Session.h"
#include "Terminology.h"
#include "UIStrings.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Dialog" NIB from the package "GenericDialog.nib".
*/
enum
{
	kSignatureMyButtonHelp			= FOUR_CHAR_CODE('Help'),
	kSignatureMyButtonOK			= FOUR_CHAR_CODE('Okay'),
	kSignatureMyButtonCancel		= FOUR_CHAR_CODE('Canc')
};

#pragma mark Types

struct MyGenericDialog
{
	MyGenericDialog		(TerminalWindowRef,
						 GenericDialog_CloseNotifyProcPtr);
	
	~MyGenericDialog	();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	GenericDialog_Ref						selfRef;						//!< identical to address of structure, but typed as ref
	TerminalWindowRef						terminalWindow;					//!< the terminal window for which this dialog applies
	WindowRef								screenWindow;					//!< the window for which this dialog applies
	NIBWindow								dialogWindow;					//!< acts as the Mac OS window for the dialog
	HIViewWrap								buttonHelp;						//!< displays context-sensitive help on this dialog
	HIViewWrap								buttonOK;						//!< accepts the user’s changes
	HIViewWrap								buttonCancel;					//!< aborts
	CarbonEventHandlerWrap					buttonHICommandsHandler;		//!< invoked when a dialog button is clicked
	GenericDialog_CloseNotifyProcPtr		closeNotifyProc;				//!< routine to call when the dialog is dismissed
	CommonEventHandlers_WindowResizer		windowResizeHandler;			//!< invoked when a window has been resized
	HelpSystem_WindowKeyPhraseSetter		contextualHelpSetup;			//!< ensures proper contextual help for this window
};
typedef MyGenericDialog*		MyGenericDialogPtr;
typedef MyGenericDialog const*	MyGenericDialogConstPtr;

typedef MemoryBlockPtrLocker< GenericDialog_Ref, MyGenericDialog >	MyGenericDialogPtrLocker;
typedef LockAcquireRelease< GenericDialog_Ref, MyGenericDialog >	MyGenericDialogAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	MyGenericDialogPtrLocker&	gGenericDialogPtrLocks()	{ static MyGenericDialogPtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static Boolean				handleItemHit				(MyGenericDialogPtr, OSType);
static void					handleNewSize				(WindowRef, Float32, Float32, void*);
static pascal OSStatus		receiveHICommand			(EventHandlerCallRef, EventRef, void*);
static Boolean				shouldOKButtonBeActive		(MyGenericDialogPtr);



#pragma mark Public Methods

/*!
This method is used to create a dialog box.  It creates
the dialog box invisibly, and sets up dialog views.

(3.1)
*/
GenericDialog_Ref
GenericDialog_New	(TerminalWindowRef					inTerminalWindow,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	GenericDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new MyGenericDialog(inTerminalWindow, inCloseNotifyProcPtr), GenericDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Call this method to destroy a dialog box and its associated
data structures.  On return, your copy of the dialog reference
is set to nullptr.

(3.1)
*/
void
GenericDialog_Dispose	(GenericDialog_Ref*	inoutRefPtr)
{
	if (gGenericDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked generic dialog; outstanding locks",
							gGenericDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, MyGenericDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Displays a sheet on a terminal window, and returns immediately.

IMPORTANT:	Invoking this routine means it is no longer your
			responsibility to call GenericDialog_Dispose():
			this is done at an appropriate time after the
			user closes the dialog and after your notification
			routine has been called.

(3.1)
*/
void
GenericDialog_Display	(GenericDialog_Ref		inDialog)
{
	MyGenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// display the dialog
		ShowSheetWindow(ptr->dialogWindow, TerminalWindow_ReturnWindow(ptr->terminalWindow));
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
}// Display


/*!
Returns a reference to the terminal window being
customized by a particular dialog.

(3.1)
*/
TerminalWindowRef
GenericDialog_ReturnTerminalWindow		(GenericDialog_Ref	inDialog)
{
	MyGenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	TerminalWindowRef			result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->terminalWindow;
	
	return result;
}// ReturnTerminalWindow


/*!
The default handler for closing a dialog.

(3.1)
*/
void
GenericDialog_StandardCloseNotifyProc	(GenericDialog_Ref	UNUSED_ARGUMENT(inDialogThatClosed),
										 Boolean			UNUSED_ARGUMENT(inOKButtonPressed))
{
	// do nothing
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
Constructor.  See GenericDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
MyGenericDialog::
MyGenericDialog		(TerminalWindowRef					inTerminalWindow,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, GenericDialog_Ref)),
terminalWindow					(inTerminalWindow),
screenWindow					(TerminalWindow_ReturnWindow(inTerminalWindow)),
dialogWindow					(NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("GenericDialog"), CFSTR("Sheet"))
									<< NIBLoader_AssertWindowExists),
buttonHelp						(dialogWindow.returnHIViewWithCode(kSignatureMyButtonHelp)
									<< HIViewWrap_AssertExists
									<< DialogUtilities_SetUpHelpButton),
buttonOK						(dialogWindow.returnHIViewWithCode(kSignatureMyButtonOK)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_SetActiveState(shouldOKButtonBeActive(this))),
buttonCancel					(dialogWindow.returnHIViewWithCode(kSignatureMyButtonCancel)
									<< HIViewWrap_AssertExists),
buttonHICommandsHandler			(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this->selfRef/* user data */),
closeNotifyProc					(inCloseNotifyProcPtr),
windowResizeHandler				(),
contextualHelpSetup				(this->dialogWindow, kHelpSystem_KeyPhraseDefault)
{
	// install a callback that responds as a window is resized
	{
		Rect		currentBounds;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(this->dialogWindow, kWindowContentRgn, &currentBounds);
		assert(noErr == error);
		this->windowResizeHandler.install(this->dialogWindow, handleNewSize, this->selfRef/* user data */,
											currentBounds.right - currentBounds.left/* minimum width */,
											currentBounds.bottom - currentBounds.top/* minimum height */,
											currentBounds.right - currentBounds.left + 200/* arbitrary maximum width */,
											currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(this->windowResizeHandler.isInstalled());
	}
	
	// ensure other handlers were installed
	assert(buttonHICommandsHandler.isInstalled());
}// MyGenericDialog 2-argument constructor


/*!
Destructor.  See GenericDialog_Dispose().

(3.1)
*/
MyGenericDialog::
~MyGenericDialog ()
{
	DisposeWindow(this->dialogWindow.operator WindowRef());
}// MyGenericDialog destructor


/*!
Responds to view selections in the dialog box.
If a button is pressed, the window is closed and
the close notifier routine is called.

This routine has the option of changing the item
index; this will only affect the modal dialog
(i.e. it has no effect for the sheet).

Returns "true" only if the specified item hit is
to be IGNORED.

(3.1)
*/
static Boolean
handleItemHit	(MyGenericDialogPtr		inPtr,
				 OSType					inControlCode)
{
	Boolean		result = false;
	
	
	switch (inControlCode)
	{
	case kSignatureMyButtonOK:
		// user accepted - close the dialog with an appropriate transition for acceptance
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			GenericDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, true/* OK was pressed */);
		}
		break;
	
	case kSignatureMyButtonCancel:
		// hide the terminal since its customizing sheet is gone
		HideWindow(inPtr->screenWindow);
		
		// user cancelled - close the dialog with an appropriate transition for cancelling
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			GenericDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, false/* OK was pressed */);
		}
		break;
	
	case kSignatureMyButtonHelp:
		HelpSystem_DisplayHelpInCurrentContext();
		break;
	
	default:
		break;
	}
	
	return result;
}// handleItemHit


/*!
Moves or resizes the dialog views.

(3.1)
*/
static void
handleNewSize	(WindowRef	inWindow,
				 Float32	inDeltaX,
				 Float32	inDeltaY,
				 void*		inGenericDialogRef)
{
	if (inDeltaX)
	{
		GenericDialog_Ref			ref = REINTERPRET_CAST(inGenericDialogRef, GenericDialog_Ref);
		MyGenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
		
		
		DialogAdjust_BeginControlAdjustment(inWindow);
		{
			DialogItemAdjustment	adjustment;
			SInt32					amountH = 0L;
			
			
			// controls which are moved horizontally
			adjustment = kDialogItemAdjustmentMoveH;
			amountH = STATIC_CAST(inDeltaX, SInt32);
			if (Localization_IsLeftToRight())
			{
				DialogAdjust_AddControl(adjustment, ptr->buttonOK, amountH);
				DialogAdjust_AddControl(adjustment, ptr->buttonCancel, amountH);
			}
			else
			{
				DialogAdjust_AddControl(adjustment, ptr->buttonHelp, amountH);
			}
		}
		DialogAdjust_EndAdjustment(STATIC_CAST(inDeltaX, SInt32), STATIC_CAST(inDeltaY, SInt32)); // moves and resizes controls properly
	}
}// handleNewSize


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the new session dialog.

(3.1)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inGenericDialogRef)
{
	OSStatus			result = eventNotHandledErr;
	GenericDialog_Ref	ref = REINTERPRET_CAST(inGenericDialogRef, GenericDialog_Ref);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			switch (received.commandID)
			{
			case kHICommandOK:
				{
					MyGenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, kSignatureMyButtonOK)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				GenericDialog_Dispose(&ref);
				break;
			
			case kHICommandCancel:
				{
					MyGenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, kSignatureMyButtonCancel)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				GenericDialog_Dispose(&ref);
				break;
			
			case kCommandContextSensitiveHelp:
				{
					MyGenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, kSignatureMyButtonHelp)) result = eventNotHandledErr;
				}
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
Determines whether or not the OK button should
be available to the user.

(3.1)
*/
static Boolean
shouldOKButtonBeActive	(MyGenericDialogPtr		UNUSED_ARGUMENT(inPtr))
{
	return true;
}// shouldOKButtonBeActive

// BELOW IS REQUIRED NEWLINE TO END FILE
