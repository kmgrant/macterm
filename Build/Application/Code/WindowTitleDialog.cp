/*###############################################################

	WindowTitleDialog.cp
	
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

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CocoaBasic.h>
#include <Console.h>
#include <DialogAdjust.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>

// application includes
#include "AppResources.h"
#include "Commands.h"
#include "CommonEventHandlers.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Session.h"
#include "VectorCanvas.h"
#include "WindowTitleDialog.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Dialog" NIB from the package "WindowTitleDialog.nib".
*/
enum
{
	kSignatureMyButtonRename		= 'Renm',
	kSignatureMyButtonCancel		= 'Canc',
	kSignatureMyLabelWindowTitle	= 'TLbl',
	kSignatureMyFieldWindowTitle	= 'Titl'
};
static HIViewID const		idMyButtonRename			= { kSignatureMyButtonRename,			0/* ID */ };
static HIViewID const		idMyButtonCancel			= { kSignatureMyButtonCancel,			0/* ID */ };
static HIViewID const		idMyLabelWindowTitle		= { kSignatureMyLabelWindowTitle,		0/* ID */ };
static HIViewID const		idMyFieldWindowTitle		= { kSignatureMyFieldWindowTitle,		0/* ID */ };

#pragma mark Types

struct My_WindowTitleDialog
{
	My_WindowTitleDialog	(HIWindowRef,
							 WindowTitleDialog_CloseNotifyProcPtr);
	
	~My_WindowTitleDialog	();
	
	WindowTitleDialog_Ref					selfRef;					// convenient reference to this structure
	SessionRef								session;					// the session, if any, to which this applies
	VectorCanvas_Ref						canvas;						// the canvas, if any, to which this applies
	HIWindowRef								screenWindow;				// the terminal window for which this dialog applies
	NIBWindow								dialogWindow;				// the dialog’s window
	HIViewWrap								buttonRename;				// Rename button
	HIViewWrap								buttonCancel;				// Cancel button
	HIViewWrap								labelTitle;					// the label for the title text field
	HIViewWrap								fieldTitle;					// the text field containing the new window title
	
	WindowTitleDialog_CloseNotifyProcPtr	closeNotifyProc;			// routine to call when the dialog is dismissed
	CarbonEventHandlerWrap					buttonHICommandsHandler;	// invoked when a dialog button is clicked
	CommonEventHandlers_WindowResizer		windowResizeHandler;		// invoked when a window has been resized
};
typedef My_WindowTitleDialog*	My_WindowTitleDialogPtr;

typedef MemoryBlockPtrLocker< WindowTitleDialog_Ref, My_WindowTitleDialog >		My_WindowTitleDialogPtrLocker;
typedef LockAcquireRelease< WindowTitleDialog_Ref, My_WindowTitleDialog >		My_WindowTitleDialogAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_WindowTitleDialogPtrLocker&	gWindowTitleDialogPtrLocks()	{ static My_WindowTitleDialogPtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static void			handleItemHit		(My_WindowTitleDialogPtr, HIViewID const&);
static void			handleNewSize		(HIWindowRef, Float32, Float32, void*);
static OSStatus		receiveHICommand	(EventHandlerCallRef, EventRef, void*);



#pragma mark Public Methods

/*!
Constructor.  See WindowTitleDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
My_WindowTitleDialog::
My_WindowTitleDialog	(HIWindowRef							inParentWindow,
						 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.,
selfRef					(REINTERPRET_CAST(this, WindowTitleDialog_Ref)),
session					(nullptr),
canvas					(nullptr),
screenWindow			(inParentWindow),
dialogWindow			(NIBWindow(AppResources_ReturnBundleForNIBs(), CFSTR("WindowTitleDialog"), CFSTR("Dialog"))
							<< NIBLoader_AssertWindowExists),
buttonRename			(dialogWindow.returnHIViewWithID(idMyButtonRename)
							<< HIViewWrap_AssertExists),
buttonCancel			(dialogWindow.returnHIViewWithID(idMyButtonCancel)
							<< HIViewWrap_AssertExists),
labelTitle				(dialogWindow.returnHIViewWithID(idMyLabelWindowTitle)
							<< HIViewWrap_AssertExists),
fieldTitle				(dialogWindow.returnHIViewWithID(idMyFieldWindowTitle)
							<< HIViewWrap_AssertExists),
closeNotifyProc			(inCloseNotifyProcPtr),
buttonHICommandsHandler	(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this->selfRef/* user data */),
windowResizeHandler		()
{
	// initialize the title text field
	if (this->fieldTitle.exists())
	{
		CFStringRef		titleString = nullptr;
		OSStatus		error = noErr;
		
		
		CopyWindowTitleAsCFString(inParentWindow, &titleString);
		error = SetControlData(this->fieldTitle, kControlEditTextPart, kControlEditTextCFStringTag,
								sizeof(titleString), &titleString);
		CFRelease(titleString), titleString = nullptr;
	}
	
	// center-justify the title text field
	if (this->fieldTitle.exists())
	{
		ControlFontStyleRec		fontStyle;
		
		
		fontStyle.flags = kControlUseThemeFontIDMask | kControlAddToMetaFontMask | kControlUseJustMask;
		fontStyle.font = kThemeSystemFont;
		fontStyle.just = teJustCenter;
		SetControlFontStyle(this->fieldTitle, &fontStyle);
	}
	
	// install a callback that responds as a window is resized
	{
		Rect		currentBounds;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(this->dialogWindow, kWindowContentRgn, &currentBounds);
		assert_noerr(error);
		(bool)this->windowResizeHandler.install(this->dialogWindow, handleNewSize, this->selfRef/* user data */,
												currentBounds.right - currentBounds.left/* minimum width */,
												currentBounds.bottom - currentBounds.top/* minimum height */,
												currentBounds.right - currentBounds.left + 200/* arbitrary maximum width */,
												currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(this->windowResizeHandler.isInstalled());
	}
	
	// ensure other handlers were installed
	assert(this->buttonHICommandsHandler.isInstalled());
}// My_WindowTitleDialog 2-argument constructor


/*!
Destructor.  See WindowTitleDialog_Dispose().

(3.1)
*/
My_WindowTitleDialog::
~My_WindowTitleDialog ()
{
	DisposeWindow(this->dialogWindow);
}// My_WindowTitleDialog destructor


/*!
This method is used to initialize the generic window title
dialog box.  It creates the dialog box invisibly, and uses
the specified window’s title as the initial field value.

(3.0)
*/
WindowTitleDialog_Ref
WindowTitleDialog_New	(HIWindowRef							inParentWindow,
						 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	WindowTitleDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_WindowTitleDialog(inParentWindow, inCloseNotifyProcPtr), WindowTitleDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
This method is used to initialize a session-specific window
title dialog box.  It creates the dialog box invisibly, and
uses the specified session’s user-defined title as the
initial field value.

When the user changes the title, the session’s user-defined
title is updated (which may affect the title of one or more
windows, but this is up to the Session implementation).

(3.1)
*/
WindowTitleDialog_Ref
WindowTitleDialog_NewForSession		(SessionRef								inSession,
									 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	WindowTitleDialog_Ref			result = WindowTitleDialog_New(Session_ReturnActiveWindow(inSession), inCloseNotifyProcPtr);
	My_WindowTitleDialogAutoLocker	ptr(gWindowTitleDialogPtrLocks(), result);
	
	
	ptr->session = inSession;
	
	// re-initialize the title text field
	if (ptr->fieldTitle.exists())
	{
		CFStringRef			titleString = nullptr;
		Session_Result		sessionResult = kSession_ResultOK;
		OSStatus			error = noErr;
		
		
		sessionResult = Session_GetWindowUserDefinedTitle(inSession, titleString);
		if ((kSession_ResultOK == sessionResult) && (nullptr != titleString))
		{
			error = SetControlData(ptr->fieldTitle, kControlEditTextPart, kControlEditTextCFStringTag,
									sizeof(titleString), &titleString);
		}
	}
	
	return result;
}// NewForSession


/*!
This method is used to initialize a canvas-specific window
title dialog box.  It creates the dialog box invisibly, and
uses the specified vector canvas’ user-defined title as the
initial field value.

When the user changes the title, the canvas’ user-defined
title is updated (which may affect the title of one or more
windows, but this is up to the canvas implementation).

(3.1)
*/
WindowTitleDialog_Ref
WindowTitleDialog_NewForVectorCanvas	(VectorCanvas_Ref						inCanvas,
										 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	WindowTitleDialog_Ref			result = WindowTitleDialog_New(VectorCanvas_ReturnWindow(inCanvas), inCloseNotifyProcPtr);
	My_WindowTitleDialogAutoLocker	ptr(gWindowTitleDialogPtrLocks(), result);
	
	
	ptr->canvas = inCanvas;
	
	// re-initialize the title text field
	if (ptr->fieldTitle.exists())
	{
		CFStringRef		titleString = nullptr;
		OSStatus		error = noErr;
		
		
		VectorCanvas_CopyTitle(inCanvas, titleString);
		if (nullptr != titleString)
		{
			error = SetControlData(ptr->fieldTitle, kControlEditTextPart, kControlEditTextCFStringTag,
									sizeof(titleString), &titleString);
			CFRelease(titleString), titleString = nullptr;
		}
	}
	
	return result;
}// NewForVectorCanvas


/*!
Call this method to destroy a window title dialog
box and its associated data structures.  On return,
your copy of the dialog reference is set to nullptr.

(3.0)
*/
void
WindowTitleDialog_Dispose	(WindowTitleDialog_Ref*		inoutRefPtr)
{
	if (gWindowTitleDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked window title dialog; outstanding locks",
						gWindowTitleDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_WindowTitleDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
This method displays and handles events in the
generic window title dialog box.  When the user
clicks a button in the dialog, its disposal
callback is invoked.

(3.0)
*/
void
WindowTitleDialog_Display	(WindowTitleDialog_Ref		inDialog)
{
	My_WindowTitleDialogAutoLocker		ptr(gWindowTitleDialogPtrLocks(), inDialog);
	
	
	if (ptr == nullptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// display the dialog
		ShowSheetWindow(ptr->dialogWindow, ptr->screenWindow);
		CocoaBasic_MakeKeyWindowCarbonUserFocusWindow();
		
		// set keyboard focus
		(OSStatus)DialogUtilities_SetKeyboardFocus(ptr->fieldTitle);
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
}// Display


/*!
If you only need a close notification procedure for
the purpose of disposing of the window title dialog
reference (and don’t otherwise care when a window
title dialog closes), you can pass this standard
routine to WindowTitleDialog_New() as your notification
procedure.

(3.0)
*/
void
WindowTitleDialog_StandardCloseNotifyProc	(WindowTitleDialog_Ref	inDialogThatClosed,
											 Boolean				UNUSED_ARGUMENT(inOKButtonPressed))
{
	WindowTitleDialog_Dispose(&inDialogThatClosed);
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

(3.0)
*/
static void
handleItemHit	(My_WindowTitleDialogPtr	inPtr,
				 HIViewID const&			inHIViewID)
{
	switch (inHIViewID.signature)
	{
	case kSignatureMyButtonRename:
		// set title of parent window
		{
			OSStatus		error = noErr;
			CFStringRef		titleCFString = nullptr;
			Size			actualSize = 0L;
			
			
			error = GetControlData(inPtr->fieldTitle, kControlEditTextPart, kControlEditTextCFStringTag,
									sizeof(titleCFString), &titleCFString, &actualSize);
			if (noErr == error)
			{
				assert(STATIC_CAST(actualSize, size_t) <= sizeof(titleCFString));
				assert(nullptr != titleCFString);
				assert(CFStringGetTypeID() == CFGetTypeID(titleCFString));
				if (nullptr != inPtr->session)
				{
					Session_SetWindowUserDefinedTitle(inPtr->session, titleCFString);
				}
				else if (nullptr != inPtr->canvas)
				{
					VectorCanvas_SetTitle(inPtr->canvas, titleCFString);
				}
				else
				{
					(OSStatus)SetWindowTitleWithCFString(inPtr->screenWindow, titleCFString);
				}
				CFRelease(titleCFString), titleCFString = nullptr;
			}
		}
		
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (inPtr->closeNotifyProc != nullptr)
		{
			WindowTitleDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, true/* OK pressed */);
		}
		break;
	
	case kSignatureMyButtonCancel:
		// user cancelled - close the dialog with an appropriate transition for cancelling
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (inPtr->closeNotifyProc != nullptr)
		{
			WindowTitleDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, false/* OK pressed */);
		}
		break;
	
	default:
		break;
	}
}// handleItemHit


/*!
Moves or resizes the controls in window title dialogs.

(3.0)
*/
static void
handleNewSize	(HIWindowRef	inWindow,
				 Float32		inDeltaX,
				 Float32		inDeltaY,
				 void*			inWindowTitleDialogRef)
{
	// only horizontal changes are significant to this dialog
	if (inDeltaX)
	{
		WindowTitleDialog_Ref			ref = REINTERPRET_CAST(inWindowTitleDialogRef, WindowTitleDialog_Ref);
		My_WindowTitleDialogAutoLocker	ptr(gWindowTitleDialogPtrLocks(), ref);
		SInt32							truncDeltaX = STATIC_CAST(inDeltaX, SInt32);
		SInt32							truncDeltaY = STATIC_CAST(inDeltaY, SInt32);
		
		
		DialogAdjust_BeginControlAdjustment(inWindow);
		if (Localization_IsLeftToRight())
		{
			// controls which are moved horizontally
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonRename, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonCancel, truncDeltaX);
			
			// controls which are resized horizontally
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->labelTitle, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->fieldTitle, truncDeltaX);
		}
		else
		{
			// controls which are resized horizontally
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->labelTitle, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->fieldTitle, truncDeltaX);
		}
		DialogAdjust_EndAdjustment(truncDeltaX, truncDeltaY);
	}
}// handleNewSize


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in window title dialogs.

(3.0)
*/
static OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inWindowTitleDialogRef)
{
	OSStatus						result = eventNotHandledErr;
	WindowTitleDialog_Ref			ref = REINTERPRET_CAST(inWindowTitleDialogRef, WindowTitleDialog_Ref);
	My_WindowTitleDialogAutoLocker	ptr(gWindowTitleDialogPtrLocks(), ref);
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
			switch (received.commandID)
			{
			case kHICommandOK:
				// change window title
				handleItemHit(ptr, idMyButtonRename);
				break;
			
			case kHICommandCancel:
				// do nothing (but close sheet)
				handleItemHit(ptr, idMyButtonCancel);
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the sheet is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand

// BELOW IS REQUIRED NEWLINE TO END FILE
