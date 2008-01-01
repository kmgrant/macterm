/*###############################################################

	SizeDialog.cp
	
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

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "AppResources.h"
#include "BasicTypesAE.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "HelpSystem.h"
#include "RecordAE.h"
#include "SizeDialog.h"
#include "Terminal.h"
#include "Terminology.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Dialog" NIB from the package "SizeDialog.nib".
*/
static HIViewID const	idMyLabelWidth					= { FOUR_CHAR_CODE('WLbl'), 0/* ID */ };
static HIViewID const	idMyFieldWidth					= { FOUR_CHAR_CODE('WFld'), 0/* ID */ };
static HIViewID const	idMyArrowsWidth					= { FOUR_CHAR_CODE('WArr'), 0/* ID */ };
static HIViewID const	idMyLabelHeight					= { FOUR_CHAR_CODE('HLbl'), 0/* ID */ };
static HIViewID const	idMyFieldHeight					= { FOUR_CHAR_CODE('HFld'), 0/* ID */ };
static HIViewID const	idMyArrowsHeight				= { FOUR_CHAR_CODE('HArr'), 0/* ID */ };
static HIViewID const	idMyButtonHelp					= { FOUR_CHAR_CODE('Help'), 0/* ID */ };
static HIViewID const	idMyButtonResize				= { FOUR_CHAR_CODE('Rsiz'), 0/* ID */ };
static HIViewID const	idMyButtonCancel				= { FOUR_CHAR_CODE('Canc'), 0/* ID */ };

enum
{
	// the maximum width is dynamic, memory permitting (see also
	// "Preferences.cp", in the terminal validation function, if
	// you change these)
	kSizeMinimumWidth = 10,
	kSizeMinimumHeight = 10,
	kSizeMaximumHeight = 200
};

#pragma mark Types

struct SizeDialog
{	
	SizeDialog	(TerminalWindowRef,
				 SizeDialogCloseNotifyProcPtr);
	
	~SizeDialog	();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	TerminalSizeDialogRef				selfRef;				//!< identical to address of structure, but typed as ref
	TerminalWindowRef					terminalWindow;			//!< the terminal window for which this dialog applies
	WindowRef							screenWindow;			//!< the window for which this dialog applies
	NIBWindow							dialogWindow;			//!< acts as the Mac OS window for the dialog
	WindowInfoRef						windowInfo;				//!< auxiliary data for the dialog
	UInt16								displayedColumns;		//!< number of columns most recently given by user in UI
	UInt16								displayedRows;			//!< number of rows most recently given by user in UI
	HIViewWrap							labelWidth;				//!< label for columns field
	HIViewWrap							fieldWidth;				//!< number of columns
	HIViewWrap							arrowsWidth;			//!< adjuster for number of columns
	HIViewWrap							labelHeight;			//!< label for rows field
	HIViewWrap							fieldHeight;			//!< number of rows
	HIViewWrap							arrowsHeight;			//!< adjuster for number of rows
	HIViewWrap							buttonHelp;				//!< displays context-sensitive help on this dialog
	HIViewWrap							buttonResize;			//!< resizes window according to user’s configuration
	HIViewWrap							buttonCancel;			//!< aborts
	CarbonEventHandlerWrap				buttonHICommandsHandler;//!< invoked when a dialog button is clicked
	CommonEventHandlers_NumericalFieldArrowsRef	widthArrowsHandler;		//!< syncs arrows and field
	CommonEventHandlers_NumericalFieldArrowsRef	heightArrowsHandler;	//!< syncs arrows and field
	HelpSystem_WindowKeyPhraseSetter	contextualHelpSetup;	//!< ensures proper contextual help for this window
	SizeDialogCloseNotifyProcPtr		closeNotifyProc;		//!< routine to call when the dialog is dismissed
};
typedef SizeDialog*			SizeDialogPtr;
typedef SizeDialogPtr*		SizeDialogHandle;

typedef MemoryBlockPtrLocker< TerminalSizeDialogRef, SizeDialog >	SizeDialogPtrLocker;
typedef LockAcquireRelease< TerminalSizeDialogRef, SizeDialog >		SizeDialogAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	SizeDialogPtrLocker&		gSizeDialogPtrLocks ()	{ static SizeDialogPtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static Boolean			handleItemHit			(SizeDialogPtr, HIViewID const&);
static pascal OSStatus	receiveHICommand		(EventHandlerCallRef, EventRef, void*);



#pragma mark Public Methods

/*!
Constructor.  See SizeDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
SizeDialog::
SizeDialog	(TerminalWindowRef				inTerminalWindow,
			 SizeDialogCloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, TerminalSizeDialogRef)),
terminalWindow					(inTerminalWindow),
screenWindow					(TerminalWindow_ReturnWindow(inTerminalWindow)),
dialogWindow					(NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("SizeDialog"), CFSTR("Dialog"))
									<< NIBLoader_AssertWindowExists),
windowInfo						(WindowInfo_New()),
displayedColumns				(0),
displayedRows					(0),
labelWidth						(dialogWindow.returnHIViewWithID(idMyLabelWidth)
									<< HIViewWrap_AssertExists),
fieldWidth						(dialogWindow.returnHIViewWithID(idMyFieldWidth)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(NumericalLimiter)),
arrowsWidth						(dialogWindow.returnHIViewWithID(idMyArrowsWidth)
									<< HIViewWrap_AssertExists),
labelHeight						(dialogWindow.returnHIViewWithID(idMyLabelHeight)
									<< HIViewWrap_AssertExists),
fieldHeight						(dialogWindow.returnHIViewWithID(idMyFieldHeight)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(NumericalLimiter)),
arrowsHeight					(dialogWindow.returnHIViewWithID(idMyArrowsHeight)
									<< HIViewWrap_AssertExists),
buttonHelp						(dialogWindow.returnHIViewWithID(idMyButtonHelp)
									<< HIViewWrap_AssertExists
									<< DialogUtilities_SetUpHelpButton),
buttonResize					(dialogWindow.returnHIViewWithID(idMyButtonResize)
									<< HIViewWrap_AssertExists),
buttonCancel					(dialogWindow.returnHIViewWithID(idMyButtonCancel)
									<< HIViewWrap_AssertExists),
buttonHICommandsHandler			(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this->selfRef/* user data */),
widthArrowsHandler				(nullptr/* set below */),
heightArrowsHandler				(nullptr/* set below */),
contextualHelpSetup				(this->dialogWindow, kHelpSystem_KeyPhraseScreenSize),
closeNotifyProc					(inCloseNotifyProcPtr)
{
	assert(nullptr != this->terminalWindow);
	assert(IsValidWindowRef(this->screenWindow));
	
	// prevent the user from typing non-digits into the text fields, and initialize the fields and arrows
	{
		UInt16		columns = 0;
		UInt16		rows = 0;
		SInt16		dimension = 0;
		
		
		// determine size of screen (to initialize controls)
		TerminalWindow_GetScreenDimensions(this->terminalWindow, &columns, &rows);
		
		// “width” controls; arrows are restricted by the valid range of input
		dimension = columns;
		SetControlNumericalText(this->fieldWidth, dimension);
		SetControl32BitMaximum(this->arrowsWidth, Terminal_ReturnAllocatedColumnCount());
		SetControl32BitMinimum(this->arrowsWidth, kSizeMinimumWidth);
		SetControl32BitValue(this->arrowsWidth, dimension);
		
		// “height” controls; arrows are restricted by the valid range of input
		dimension = rows;
		SetControlNumericalText(this->fieldHeight, dimension);
		SetControl32BitMaximum(this->arrowsHeight, kSizeMaximumHeight);
		SetControl32BitMinimum(this->arrowsHeight, kSizeMinimumHeight);
		SetControl32BitValue(this->arrowsHeight, dimension);
	}
	
	// set up handlers so arrow clicks change the field values
	(OSStatus)CommonEventHandlers_InstallNumericalFieldArrows
				(this->arrowsWidth, this->fieldWidth, &widthArrowsHandler);
	(OSStatus)CommonEventHandlers_InstallNumericalFieldArrows
				(this->arrowsHeight, this->fieldHeight, &heightArrowsHandler);
	
	// make labels bold (this is done because older OS versions will not recognize a similar NIB setup)
	Localization_SetControlThemeFontInfo(this->labelWidth, kThemeAlertHeaderFont);
	Localization_SetControlThemeFontInfo(this->labelHeight, kThemeAlertHeaderFont);
	
	// set up the Window Info information
	WindowInfo_SetWindowDescriptor(this->windowInfo, kConstantsRegistry_WindowDescriptorScreenSize);
	WindowInfo_SetForWindow(this->dialogWindow, this->windowInfo);
	
	// ensure other handlers were installed
	assert(buttonHICommandsHandler.isInstalled());
}// SizeDialog 2-argument constructor


/*!
Destructor.  See SizeDialog_Dispose().

(3.1)
*/
SizeDialog::
~SizeDialog ()
{
	CommonEventHandlers_RemoveNumericalFieldArrows(&this->widthArrowsHandler);
	CommonEventHandlers_RemoveNumericalFieldArrows(&this->heightArrowsHandler);
	DisposeWindow(this->dialogWindow.operator WindowRef());
	WindowInfo_Dispose(this->windowInfo);
}// SizeDialog destructor


/*!
This method is used to initialize the Terminal Screen Size
dialog box.  It creates the dialog box invisibly, and uses
the specified screen ID to set up the controls in the
dialog box to use the screen’s dimensions.

(3.0)
*/
TerminalSizeDialogRef
SizeDialog_New	(TerminalWindowRef				inTerminalWindow,
				 SizeDialogCloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	TerminalSizeDialogRef		result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new SizeDialog(inTerminalWindow, inCloseNotifyProcPtr), TerminalSizeDialogRef);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Call this method to destroy a screen size dialog
box and its associated data structures.  On return,
your copy of the dialog reference is set to nullptr.

(3.0)
*/
void
SizeDialog_Dispose	(TerminalSizeDialogRef*		inoutRefPtr)
{
	if (gSizeDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked terminal resize dialog; outstanding locks",
							gSizeDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, SizeDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
This method displays and handles events in the Terminal Screen
Size dialog box.  When the user clicks a button in the dialog,
its disposal callback is invoked.

IMPORTANT:	Invoking this routine means it is no longer your
			responsibility to call SizeDialog_Dispose():
			this is done at an appropriate time after the
			user closes the dialog and after your notification
			routine has been called.

(3.0)
*/
void
SizeDialog_Display		(TerminalSizeDialogRef		inDialog)
{
	SizeDialogAutoLocker	ptr(gSizeDialogPtrLocks(), inDialog);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// display the dialog
		ShowSheetWindow(ptr->dialogWindow, ptr->screenWindow);
		HIViewSetNextFocus(HIViewGetRoot(ptr->dialogWindow), ptr->fieldWidth);
		HIViewAdvanceFocus(HIViewGetRoot(ptr->dialogWindow), 0/* modifier keys */);
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
}// Display


/*!
Returns the currently-displayed column and row
count from the user interface.

WARNING:	Only valid in a close notification
			routine (after the dialog is closed).

(3.1)
*/
void
SizeDialog_GetDisplayedDimensions	(TerminalSizeDialogRef	inDialog,
									 UInt16&				outColumns,
									 UInt16&				outRows)
{
	SizeDialogAutoLocker	ptr(gSizeDialogPtrLocks(), inDialog);
	
	
	if (nullptr != ptr)
	{
		outColumns = ptr->displayedColumns;
		outRows = ptr->displayedRows;
	}
}// GetDisplayedDimensions


/*!
Returns the terminal window that this dialog is
attached to.

(3.1)
*/
TerminalWindowRef
SizeDialog_ReturnParentTerminalWindow	(TerminalSizeDialogRef	inDialog)
{
	SizeDialogAutoLocker	ptr(gSizeDialogPtrLocks(), inDialog);
	TerminalWindowRef		result = nullptr;
	
	
	if (nullptr != ptr)
	{
		result = ptr->terminalWindow;
	}
	
	return result;
}// ReturnParentTerminalWindow


/*!
Sends Apple Events back to MacTelnet that instruct
it to make the specified changes to the frontmost
window.  THIS DOES NOT ACTUALLY EXECUTE THE EVENTS.
You call this routine simply as a convenience for
the user, in case he or she is recording tasks into
a script; this causes “dimension change” commands
to appear in the script being recorded.

(3.0)
*/
void
SizeDialog_SendRecordableDimensionChangeEvents		(SInt16		inNewColumns,
													 SInt16		inNewRows)
{
	OSStatus	error = noErr;
	AEDesc		setDataEvent;
	AEDesc		replyDescriptor;
	
	
	RecordAE_CreateRecordableAppleEvent(kAECoreSuite, kAESetData, &setDataEvent);
	if (noErr == error)
	{
		AEDesc		containerDesc,
					keyDesc,
					objectDesc,
					propertyDesc,
					valueDesc,
					reference1Desc,
					reference2Desc;
		
		
		(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&propertyDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&valueDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&reference1Desc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&reference2Desc);
		
		// send a request for "window <index>", which resides in a nullptr container;
		// then, request the "columns" and "rows" properties, and finally, issue an
		// event that will change their values as a nifty ordered pair
		(OSStatus)BasicTypesAE_CreateSInt32Desc(1/* frontmost window */, &keyDesc);
		error = CreateObjSpecifier(cMyWindow, &containerDesc,
									formAbsolutePosition, &keyDesc, true/* dispose inputs */,
									&objectDesc);
		if (noErr != error) Console_WriteValue("window access error", error);
		(OSStatus)BasicTypesAE_CreatePropertyIDDesc(pMyTerminalWindowColumns, &keyDesc);
		error = CreateObjSpecifier(cProperty, &objectDesc,
									formPropertyID, &keyDesc, true/* dispose inputs */,
									&reference1Desc);
		if (noErr != error) Console_WriteValue("property access error", error);
		(OSStatus)BasicTypesAE_CreatePropertyIDDesc(pMyTerminalWindowRows, &keyDesc);
		error = CreateObjSpecifier(cProperty, &objectDesc,
									formPropertyID, &keyDesc, true/* dispose inputs */,
									&reference2Desc);
		if (noErr != error) Console_WriteValue("property access error", error);
		
		BasicTypesAE_CreatePairDesc(&reference1Desc, &reference2Desc, &propertyDesc);
		AEDisposeDesc(&reference1Desc);
		AEDisposeDesc(&reference2Desc);
		
		if (noErr == error)
		{
			// reference to the property to be set - REQUIRED
			(OSStatus)AEPutParamDesc(&setDataEvent, keyDirectObject, &propertyDesc);
			
			// the property’s new value - REQUIRED
			(OSStatus)BasicTypesAE_CreateUInt32PairDesc(inNewColumns, inNewRows, &valueDesc);
			(OSStatus)AEPutParamDesc(&setDataEvent, keyAEParamMyTo, &valueDesc);
			
			// send the event, which will record it into any open script;
			// this event is not executed because the screen has already
			// been resized; it is simply for convenience to the user, who
			// will be able to achieve the same effect later by running
			// his or her script
			(OSStatus)AESend(&setDataEvent, &replyDescriptor,
								kAENoReply | kAECanInteract | kAEDontExecute,
								kAENormalPriority, kAEDefaultTimeout, nullptr, nullptr);
		}
		AEDisposeDesc(&propertyDesc);
		AEDisposeDesc(&valueDesc);
	}
	AEDisposeDesc(&setDataEvent);
}// SendRecordableDimensionChangeEvents


/*!
If you only need a close notification procedure
for the purpose of disposing of the Size Dialog
reference (and don’t otherwise care when a Size
Dialog closes), you can pass this standard routine
to SizeDialog_New() as your notification procedure.

(3.0)
*/
void
SizeDialog_StandardCloseNotifyProc		(TerminalSizeDialogRef		UNUSED_ARGUMENT(inDialogThatClosed),
										 Boolean					UNUSED_ARGUMENT(inOKButtonPressed))
{
	// do nothing
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
Responds to control selections in the dialog box.
If a button is pressed, the window is closed and
the close notifier routine is called; otherwise,
an adjustment is made to the display.

The dialog data is passed redundantly by pointer
and reference, for efficiency and so that the
reference can be provided to the notifier.

Returns "true" only if the specified item hit is
to be IGNORED.

(3.0)
*/
static Boolean
handleItemHit	(SizeDialogPtr		inPtr,
				 HIViewID const&	inID)
{
	Boolean		result = false;
	
	
	assert(0 == inID.id);
	if (idMyButtonResize.signature == inID.signature)
	{
		{
			Boolean		notGood = false;
			SInt32		columns = 0L;
			SInt32		rows = 0L;
			
			
			// get the information from the dialog and then kill it
			GetControlNumericalText(inPtr->fieldWidth, &columns);
			GetControlNumericalText(inPtr->fieldHeight, &rows);
			
			// user accepted - close the dialog with an appropriate transition for acceptance
			HideSheetWindow(inPtr->dialogWindow);
			
			// force the width to be within a valid range
			if (columns < kSizeMinimumWidth)
			{
				columns = kSizeMinimumWidth;
				notGood = true;
			}
			else if (columns > Terminal_ReturnAllocatedColumnCount())
			{
				columns = Terminal_ReturnAllocatedColumnCount();
				notGood = true;
			}
			
			// force the height to be within a valid range
			if (rows < kSizeMinimumHeight)
			{
				rows = kSizeMinimumHeight;
				notGood = true;
			}
			else if (rows > kSizeMaximumHeight)
			{
				rows = kSizeMaximumHeight;
				notGood = true;
			}
			
			// if the user entered values out of range, emit a system beep as an indicator
			if (notGood) Sound_StandardAlert();
			
			// save values
			inPtr->displayedColumns = STATIC_CAST(columns, UInt16);
			inPtr->displayedRows = STATIC_CAST(rows, UInt16);
		}
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			InvokeSizeDialogCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, true/* OK pressed */);
		}
	}
	else if (idMyButtonCancel.signature == inID.signature)
	{
		// user cancelled - close the dialog with an appropriate transition for cancelling
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			InvokeSizeDialogCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, false/* OK pressed */);
		}
	}
	else if (idMyButtonHelp.signature == inID.signature)
	{
		HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseScreenSize);
	}
	else
	{
		// ???
	}
	
	return result;
}// handleItemHit


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the terminal size dialog.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inTerminalSizeDialogRef)
{
	OSStatus				result = eventNotHandledErr;
	TerminalSizeDialogRef	ref = REINTERPRET_CAST(inTerminalSizeDialogRef, TerminalSizeDialogRef);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
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
					SizeDialogAutoLocker	ptr(gSizeDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, idMyButtonResize)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				SizeDialog_Dispose(&ref);
				break;
			
			case kHICommandCancel:
				{
					SizeDialogAutoLocker	ptr(gSizeDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, idMyButtonCancel)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				SizeDialog_Dispose(&ref);
				break;
			
			case kCommandContextSensitiveHelp:
				{
					SizeDialogAutoLocker	ptr(gSizeDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, idMyButtonHelp)) result = eventNotHandledErr;
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

// BELOW IS REQUIRED NEWLINE TO END FILE
