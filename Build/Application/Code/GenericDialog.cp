/*###############################################################

	GenericDialog.cp
	
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
static HIViewID const	idMyUserPanePanelMargins	= { 'Panl', 0/* ID */ };
static HIViewID const	idMyButtonHelp				= { 'Help', 0/* ID */ };
static HIViewID const	idMyButtonOK				= { 'Okay', 0/* ID */ };
static HIViewID const	idMyButtonCancel			= { 'Canc', 0/* ID */ };

#pragma mark Types

struct My_GenericDialog
{
	My_GenericDialog	(HIWindowRef,
						 Panel_Ref,
						 GenericDialog_CloseNotifyProcPtr,
						 HelpSystem_KeyPhrase);
	
	~My_GenericDialog	();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	GenericDialog_Ref						selfRef;						//!< identical to address of structure, but typed as ref
	HIWindowRef								parentWindow;					//!< the terminal window for which this dialog applies
	Boolean									isModal;						//!< if false, the dialog is a sheet
	Panel_Ref								hostedPanel;					//!< the panel implementing the primary user interface
	HISize									panelIdealSize;					//!< the dimensions most appropriate for displaying the UI
	NIBWindow								dialogWindow;					//!< acts as the Mac OS window for the dialog
	HIViewWrap								userPaneForMargins;				//!< used to determine location of, and space around, the hosted panel
	HIViewWrap								buttonHelp;						//!< displays context-sensitive help on this dialog
	HIViewWrap								buttonOK;						//!< accepts the user’s changes
	HIViewWrap								buttonCancel;					//!< aborts
	CarbonEventHandlerWrap					buttonHICommandsHandler;		//!< invoked when a dialog button is clicked
	GenericDialog_CloseNotifyProcPtr		closeNotifyProc;				//!< routine to call when the dialog is dismissed
	CommonEventHandlers_WindowResizer		windowResizeHandler;			//!< invoked when a window has been resized
	HelpSystem_WindowKeyPhraseSetter		contextualHelpSetup;			//!< ensures proper contextual help for this window
};
typedef My_GenericDialog*			My_GenericDialogPtr;
typedef My_GenericDialog const*		My_GenericDialogConstPtr;

typedef MemoryBlockPtrLocker< GenericDialog_Ref, My_GenericDialog >		My_GenericDialogPtrLocker;
typedef LockAcquireRelease< GenericDialog_Ref, My_GenericDialog >		My_GenericDialogAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_GenericDialogPtrLocker&	gGenericDialogPtrLocks()	{ static My_GenericDialogPtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static Boolean				handleItemHit				(My_GenericDialogPtr, HIViewID const&);
static void					handleNewSize				(WindowRef, Float32, Float32, void*);
static pascal OSStatus		receiveHICommand			(EventHandlerCallRef, EventRef, void*);



#pragma mark Public Methods

/*!
This method is used to create a dialog box.  It creates
the dialog box invisibly, and sets up dialog views.

If "inParentWindow" is nullptr, the window is automatically
made application-modal; otherwise, it is a sheet.

(3.1)
*/
GenericDialog_Ref
GenericDialog_New	(HIWindowRef						inParentWindow,
					 Panel_Ref							inHostedPanel,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
					 HelpSystem_KeyPhrase				inHelpButtonAction)
{
	GenericDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_GenericDialog(inParentWindow, inHostedPanel,
									inCloseNotifyProcPtr, inHelpButtonAction), GenericDialog_Ref);
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

IMPORTANT:	Since the associated Panel is not created by this
			module, it is not destroyed here.  Be sure to
			use Panel_Dispose() on the hosted panel after
			disposing of the dialog (if necessary, call
			GenericDialog_ReturnHostedPanel() to find the
			reference before the dialog is destroyed).

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
		delete *(REINTERPRET_CAST(inoutRefPtr, My_GenericDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Displays the dialog.  If the dialog is modal, this call will
block until the dialog is finished.  Otherwise, a sheet will
appear over the parent window and this call will return
immediately.

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
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		HIViewRef	panelContainer = nullptr;
		OSStatus	error = noErr;
		
		
		// show the panel
		Panel_GetContainerView(ptr->hostedPanel, panelContainer);
		Panel_SendMessageNewVisibility(ptr->hostedPanel, true/* visible */);
		error = HIViewSetVisible(panelContainer, true/* visible */);
		assert_noerr(error);
		
		// display the dialog
		if (ptr->isModal)
		{
			// handle events
			ShowWindow(ptr->dialogWindow);
			error = RunAppModalLoopForWindow(ptr->dialogWindow);
			assert_noerr(error);
		}
		else
		{
			ShowSheetWindow(ptr->dialogWindow, ptr->parentWindow);
			// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
		}
	}
}// Display


/*!
Returns the panel that composes the primary user interface
of the specified dialog.

(3.1)
*/
Panel_Ref
GenericDialog_ReturnHostedPanel		(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	Panel_Ref					result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->hostedPanel;
	
	return result;
}// ReturnHostedPanel


/*!
Returns a reference to the terminal window being
customized by a particular dialog.

(3.1)
*/
HIWindowRef
GenericDialog_ReturnParentWindow	(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	HIWindowRef					result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->parentWindow;
	
	return result;
}// ReturnParentWindow


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
My_GenericDialog::
My_GenericDialog	(HIWindowRef						inParentWindow,
					 Panel_Ref							inHostedPanel,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
					 HelpSystem_KeyPhrase				inHelpButtonAction)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, GenericDialog_Ref)),
parentWindow					(inParentWindow),
isModal							(nullptr == inParentWindow),
hostedPanel						(inHostedPanel),
panelIdealSize					(CGSizeMake(0, 0)), // set later
dialogWindow					(NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("GenericDialog"), (this->isModal) ? CFSTR("Dialog") : CFSTR("Sheet"))
									<< NIBLoader_AssertWindowExists),
userPaneForMargins				(dialogWindow.returnHIViewWithID(idMyUserPanePanelMargins)
									<< HIViewWrap_AssertExists),
buttonHelp						(dialogWindow.returnHIViewWithID(idMyButtonHelp)
									<< HIViewWrap_AssertExists
									<< DialogUtilities_SetUpHelpButton),
buttonOK						(dialogWindow.returnHIViewWithID(idMyButtonOK)
									<< HIViewWrap_AssertExists),
buttonCancel					(dialogWindow.returnHIViewWithID(idMyButtonCancel)
									<< HIViewWrap_AssertExists),
buttonHICommandsHandler			(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this->selfRef/* user data */),
closeNotifyProc					(inCloseNotifyProcPtr),
windowResizeHandler				(),
contextualHelpSetup				(this->dialogWindow, inHelpButtonAction)
{
	OSStatus	error = noErr;
	HISize		panelMarginsTopLeft;
	HISize		panelMarginsBottomRight;
	HISize		windowInitialSize;
	HIViewRef	panelContainer = nullptr;
	
	
	// allow the panel to create its controls by providing a pointer to the containing window
	Panel_SendMessageCreateViews(this->hostedPanel, this->dialogWindow);
	Panel_GetContainerView(this->hostedPanel, panelContainer);
	error = HIViewPlaceInSuperviewAt(panelContainer, panelMarginsTopLeft.width, panelMarginsTopLeft.height);
	assert_noerr(error);
	
	// figure out where the panel should be, and how much space is around it
	{
		HIViewWrap	contentView(kHIViewWindowContentID, this->dialogWindow);
		HIRect		contentBounds;
		HIRect		panelFrame;
		
		
		assert(contentView.exists());
		error = HIViewGetBounds(contentView, &contentBounds);
		assert_noerr(error);
		error = HIViewGetFrame(this->userPaneForMargins, &panelFrame);
		assert_noerr(error);
		panelMarginsTopLeft = CGSizeMake(panelFrame.origin.x, panelFrame.origin.y);
		panelMarginsBottomRight = CGSizeMake(contentBounds.size.width - panelFrame.size.width - panelMarginsTopLeft.width,
												contentBounds.size.height - panelFrame.size.height - panelMarginsTopLeft.height);
		
		// set the *initial* size of the panel to match the NIB...but (below)
		// once a delta-size handler is set up and the proper window size is
		// calculated, this will be automatically adjusted to the ideal size
		error = HIViewSetFrame(panelContainer, &panelFrame);
		assert_noerr(error);
	}
	
	// make the grow box transparent by default, which looks better most of the time;
	// however, give the panel the option to change this appearance
	{
		HIViewWrap		growBox(kHIViewWindowGrowBoxID, this->dialogWindow);
		
		
		if (kPanel_ResponseGrowBoxOpaque == Panel_SendMessageGetGrowBoxLook(this->hostedPanel))
		{
			(OSStatus)HIGrowBoxViewSetTransparent(growBox, false);
		}
		else
		{
			(OSStatus)HIGrowBoxViewSetTransparent(growBox, true);
		}
	}
	
	// install a callback that responds as a window is resized
	{
		Rect		currentBounds;
		
		
		error = GetWindowBounds(this->dialogWindow, kWindowContentRgn, &currentBounds);
		assert_noerr(error);
		this->windowResizeHandler.install(this->dialogWindow, handleNewSize, this->selfRef/* user data */,
											currentBounds.right - currentBounds.left/* minimum width */,
											currentBounds.bottom - currentBounds.top/* minimum height */,
											currentBounds.right - currentBounds.left + 200/* arbitrary maximum width */,
											currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(this->windowResizeHandler.isInstalled());
	}
	
	// adjust for ideal size
	if (kPanel_ResponseSizeProvided != Panel_SendMessageGetIdealSize(this->hostedPanel, this->panelIdealSize))
	{
		// some problem setting the size; choose one arbitrarily
		Console_WriteLine("warning, unable to determine ideal size of dialog panel");
		panelIdealSize = CGSizeMake(400.0, 250.0); // arbitrary
	}
	windowInitialSize = CGSizeMake(panelMarginsTopLeft.width + this->panelIdealSize.width + panelMarginsBottomRight.width,
									panelMarginsTopLeft.height + this->panelIdealSize.height + panelMarginsBottomRight.height);
	this->windowResizeHandler.setWindowIdealSize(windowInitialSize.width, windowInitialSize.height);
	this->windowResizeHandler.setWindowMinimumSize(windowInitialSize.width, windowInitialSize.height);
	this->windowResizeHandler.setWindowMaximumSize(windowInitialSize.width + 200/* arbitrary */,
													windowInitialSize.height + 100/* arbitrary */);
	
	// resize the window; due to event handlers, a resize of the window
	// changes the panel size too
	SizeWindow(this->dialogWindow, STATIC_CAST(windowInitialSize.width, SInt16),
				STATIC_CAST(windowInitialSize.height, SInt16), true/* update */);
	
	// positioning is not done in the NIB because an arbitrarily-sized panel is
	// added programmatically, and affects the size (and centering) of the window
	if (this->isModal)
	{
		HIWindowRef const	kAnchorWindow = GetUserFocusWindow();
		
		
		(OSStatus)RepositionWindow(this->dialogWindow, kAnchorWindow,
									(nullptr == kAnchorWindow)
										? kWindowCenterOnMainScreen
										: kWindowCenterOnParentWindowScreen);
	}
	
	// ensure other handlers were installed
	assert(buttonHICommandsHandler.isInstalled());
}// My_GenericDialog 4-argument constructor


/*!
Destructor.  See GenericDialog_Dispose().

(3.1)
*/
My_GenericDialog::
~My_GenericDialog ()
{
	DisposeWindow(this->dialogWindow.operator WindowRef());
}// My_GenericDialog destructor


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
handleItemHit	(My_GenericDialogPtr	inPtr,
				 HIViewID const&		inID)
{
	Boolean		result = false;
	
	
	assert(0 == inID.id);
	if (idMyButtonOK.signature == inID.signature)
	{
		// user accepted - close the dialog with an appropriate transition for acceptance
		Panel_SendMessageNewVisibility(inPtr->hostedPanel, false/* visible */);
		if (inPtr->isModal)
		{
			(OSStatus)QuitAppModalLoopForWindow(inPtr->dialogWindow);
			HideWindow(inPtr->dialogWindow);
		}
		else
		{
			HideSheetWindow(inPtr->dialogWindow);
		}
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			GenericDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, true/* OK was pressed */);
		}
	}
	else if (idMyButtonCancel.signature == inID.signature)
	{
		// user cancelled - close the dialog with an appropriate transition for cancelling
		Panel_SendMessageNewVisibility(inPtr->hostedPanel, false/* visible */);
		if (inPtr->isModal)
		{
			(OSStatus)QuitAppModalLoopForWindow(inPtr->dialogWindow);
			HideWindow(inPtr->dialogWindow);
		}
		else
		{
			HideSheetWindow(inPtr->dialogWindow);
		}
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			GenericDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, false/* OK was pressed */);
		}
	}
	else if (idMyButtonHelp.signature == inID.signature)
	{
		HelpSystem_DisplayHelpInCurrentContext();
	}
	else
	{
		// ???
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
	GenericDialog_Ref			ref = REINTERPRET_CAST(inGenericDialogRef, GenericDialog_Ref);
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
	
	
	if (Localization_IsLeftToRight())
	{
		ptr->buttonOK << HIViewWrap_MoveBy(inDeltaX, inDeltaY);
		ptr->buttonCancel << HIViewWrap_MoveBy(inDeltaX, inDeltaY);
		ptr->buttonHelp << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	}
	else
	{
		ptr->buttonOK << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		ptr->buttonCancel << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		ptr->buttonHelp << HIViewWrap_MoveBy(inDeltaX, inDeltaY);
	}
	Panel_Resizer(inDeltaX, inDeltaY, true/* is delta */)(ptr->hostedPanel);
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
					My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, idMyButtonOK)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				GenericDialog_Dispose(&ref);
				break;
			
			case kHICommandCancel:
				{
					My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, idMyButtonCancel)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				GenericDialog_Dispose(&ref);
				break;
			
			case kCommandContextSensitiveHelp:
				{
					My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
					
					
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
