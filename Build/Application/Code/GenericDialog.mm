/*!	\file GenericDialog.mm
	\brief Allows a user interface that is both a panel
	and a dialog to be displayed as a modal dialog or
	sheet.
	
	Note that this module is in transition and is not yet a
	Cocoa-only user interface.
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#import "GenericDialog.h"
#import <UniversalDefines.h>

// standard-C includes
#import <climits>
#import <cstdlib>
#import <cstring>

// standard-C++ includes
#import <map>
#import <utility>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaBasic.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "DialogUtilities.h"
#import "EventLoop.h"
#import "HelpSystem.h"
#import "Session.h"
#import "Terminology.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Dialog" NIB from the package "GenericDialog.nib".
*/
HIViewID const	idMyUserPanePanelMargins	= { 'Panl', 0/* ID */ };
HIViewID const	idMyButtonHelp				= { 'Help', 0/* ID */ };
HIViewID const	idMyButtonOK				= { 'Okay', 0/* ID */ };
HIViewID const	idMyButtonCancel			= { 'Canc', 0/* ID */ };
HIViewID const	idMyButtonOther				= { 'Othr', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

typedef std::map< UInt32, GenericDialog_DialogEffect >		My_DialogEffectsByCommandID;

struct My_GenericDialog
{
	My_GenericDialog	(NSWindow*, Panel_Ref, void*,
						 GenericDialog_CloseNotifyProcPtr, HelpSystem_KeyPhrase);
	
	~My_GenericDialog	();
	
	void
	autoArrangeButtons ();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	GenericDialog_Ref						selfRef;						//!< identical to address of structure, but typed as ref
	NSWindow*								parentWindow;					//!< the terminal window for which this dialog applies
	Boolean									isModal;						//!< if false, the dialog is a sheet
	Panel_Ref								hostedPanel;					//!< the panel implementing the primary user interface
	void*									dataSetPtr;						//!< data that is given to the panel; represents what is being edited
	HISize									panelIdealSize;					//!< the dimensions most appropriate for displaying the UI
	NIBWindow								dialogWindow;					//!< acts as the Mac OS window for the dialog
	HIViewWrap								userPaneForMargins;				//!< used to determine location of, and space around, the hosted panel
	HIViewWrap								buttonHelp;						//!< displays context-sensitive help on this dialog
	HIViewWrap								buttonOK;						//!< accepts the user’s changes
	HIViewWrap								buttonCancel;					//!< aborts
	HIViewWrap								buttonOther;					//!< optional
	CarbonEventHandlerWrap					buttonHICommandsHandler;		//!< invoked when a dialog button is clicked
	GenericDialog_CloseNotifyProcPtr		closeNotifyProc;				//!< routine to call when the dialog is dismissed
	My_DialogEffectsByCommandID				closeEffects;					//!< custom sheet-closing effects for certain commands
	CommonEventHandlers_WindowResizer		windowResizeHandler;			//!< invoked when a window has been resized
	HelpSystem_WindowKeyPhraseSetter		contextualHelpSetup;			//!< ensures proper contextual help for this window
	void*									userDataPtr;					//!< optional; external data
};
typedef My_GenericDialog*			My_GenericDialogPtr;
typedef My_GenericDialog const*		My_GenericDialogConstPtr;

typedef MemoryBlockPtrLocker< GenericDialog_Ref, My_GenericDialog >		My_GenericDialogPtrLocker;
typedef LockAcquireRelease< GenericDialog_Ref, My_GenericDialog >		My_GenericDialogAutoLocker;

} // anonymous namespace

#pragma mark Variables
namespace {

My_GenericDialogPtrLocker&		gGenericDialogPtrLocks ()		{ static My_GenericDialogPtrLocker x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean		handleItemHit		(My_GenericDialogPtr, HIViewID const&);
void		handleNewSize		(HIWindowRef, Float32, Float32, void*);
OSStatus	receiveHICommand	(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
This method is used to create a dialog box.  It creates
the dialog box invisibly, and sets up dialog views.

The format of "inDataSetPtr" is entirely defined by the
type of panel that the dialog is hosting.  The data is
passed to the panel with Panel_SendMessageNewDataSet().

If "inParentWindowOrNullForModalDialog" is nullptr, the
window is automatically made application-modal; otherwise,
it is a sheet.

(4.1)
*/
GenericDialog_Ref
GenericDialog_New	(NSWindow*							inParentWindowOrNullForModalDialog,
					 Panel_Ref							inHostedPanel,
					 void*								inDataSetPtr,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
					 HelpSystem_KeyPhrase				inHelpButtonAction)
{
	GenericDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_GenericDialog(inParentWindowOrNullForModalDialog, inHostedPanel,
									inDataSetPtr, inCloseNotifyProcPtr, inHelpButtonAction), GenericDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
This method is used to create a dialog box.  It creates
the dialog box invisibly, and sets up dialog views.

The format of "inDataSetPtr" is entirely defined by the
type of panel that the dialog is hosting.  The data is
passed to the panel with Panel_SendMessageNewDataSet().

If "inParentWindowOrNullForModalDialog" is nullptr, the
window is automatically made application-modal; otherwise,
it is a sheet.

DEPRECATED.  Use the version of this method that accepts
an NSWindow*.

(3.1)
*/
GenericDialog_Ref
GenericDialog_New	(HIWindowRef						inParentWindowOrNullForModalDialog,
					 Panel_Ref							inHostedPanel,
					 void*								inDataSetPtr,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
					 HelpSystem_KeyPhrase				inHelpButtonAction)
{
	GenericDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_GenericDialog(CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(inParentWindowOrNullForModalDialog),
														inHostedPanel, inDataSetPtr, inCloseNotifyProcPtr, inHelpButtonAction),
									GenericDialog_Ref);
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
		Console_Warning(Console_WriteValue, "attempt to dispose of locked generic dialog; outstanding locks",
						gGenericDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_GenericDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Dialogs normally have only an OK and Cancel button; to show a
third button (automatically sized to fit the specified title),
use this method.

IMPORTANT:	Currently, only one additional button is supported.

(3.1)
*/
void
GenericDialog_AddButton		(GenericDialog_Ref		inDialog,
							 CFStringRef			inButtonTitle,
							 UInt32					inButtonCommandID)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	OSStatus					error = noErr;
	
	
	// set up the new button
	error = SetControlTitleWithCFString(ptr->buttonOther, inButtonTitle);
	assert_noerr(error);
	error = SetControlCommandID(ptr->buttonOther, inButtonCommandID);
	assert_noerr(error);
	UNUSED_RETURN(UInt16)Localization_AutoSizeButtonControl(ptr->buttonOther, 0/* minimum width */);
	error = HIViewSetVisible(ptr->buttonOther, true);
	assert_noerr(error);
	
	// now determine where the left edge of the button should be (locale-sensitive)
	ptr->autoArrangeButtons();
}// AddButton


/*!
Displays the dialog.  If the dialog is modal, this call will
block until the dialog is finished.  Otherwise, a sheet will
appear over the parent window and this call will return
immediately.

A view with the specified ID must exist in the window, to be
the initial keyboard focus.

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
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(paramErr);
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
			EventLoop_SelectOverRealFrontWindow(ptr->dialogWindow);
			UNUSED_RETURN(OSStatus)DialogUtilities_SetKeyboardFocus(HIViewWrap(idMyButtonCancel, ptr->dialogWindow));
			Panel_SendMessageFocusFirst(ptr->hostedPanel);
			error = RunAppModalLoopForWindow(ptr->dialogWindow);
			assert_noerr(error);
		}
		else
		{
			ShowSheetWindow(ptr->dialogWindow, REINTERPRET_CAST([ptr->parentWindow windowRef], HIWindowRef));
			UNUSED_RETURN(OSStatus)DialogUtilities_SetKeyboardFocus(HIViewWrap(idMyButtonCancel, ptr->dialogWindow));
			Panel_SendMessageFocusFirst(ptr->hostedPanel);
			CocoaBasic_MakeKeyWindowCarbonUserFocusWindow();
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
Returns whatever was set with GenericDialog_SetImplementation,
or nullptr.

(3.1)
*/
void*
GenericDialog_ReturnImplementation	(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	void*						result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->userDataPtr;
	
	return result;
}// ReturnImplementation


/*!
Returns a reference to the terminal window being
customized by a particular dialog.

(4.1)
*/
NSWindow*
GenericDialog_ReturnParentNSWindow	(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	NSWindow*					result = nil;
	
	
	if (nullptr != ptr) result = ptr->parentWindow;
	
	return result;
}// ReturnParentNSWindow


/*!
Returns a reference to the terminal window being
customized by a particular dialog.

DEPRECATED.  Use GenericDialog_ReturnParentNSWindow().

(3.1)
*/
HIWindowRef
GenericDialog_ReturnParentWindow	(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	HIWindowRef					result = nullptr;
	
	
	if (nullptr != ptr) result = REINTERPRET_CAST([ptr->parentWindow windowRef], HIWindowRef);
	
	return result;
}// ReturnParentWindow


/*!
Specifies a new name for the button corresponding to the
given command.  The specified button is automatically
resized to fit the title, and other action buttons are
moved (and possibly resized) to make room.

Use kHICommandOK to refer to the default button.  No other
command IDs are currently supported!

IMPORTANT:	This API currently only works for the
			standard buttons, not custom command IDs.

(4.0)
*/
void
GenericDialog_SetCommandButtonTitle		(GenericDialog_Ref		inDialog,
										 UInt32					inCommandID,
										 CFStringRef			inButtonTitle)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (kHICommandOK == inCommandID)
	{
		UNUSED_RETURN(OSStatus)SetControlTitleWithCFString(ptr->buttonOK, inButtonTitle);
	}
	ptr->autoArrangeButtons();
}// SetCommandButtonTitle


/*!
Specifies the effect that a command has on the dialog.

Use kHICommandOK and kHICommandCancel to refer to the
two standard buttons.

Note that by default, the OK and Cancel buttons have the
effect of "kGenericDialog_DialogEffectCloseNormally", and
any extra buttons have no effect at all (that is,
"kGenericDialog_DialogEffectNone").

IMPORTANT:	This API currently only works for the
			standard buttons, not custom command IDs.

(3.1)
*/
void
GenericDialog_SetCommandDialogEffect	(GenericDialog_Ref				inDialog,
										 UInt32							inCommandID,
										 GenericDialog_DialogEffect		inEffect)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->closeEffects[inCommandID] = inEffect;
}// SetCommandDialogEffect


/*!
Associates arbitrary user data with your dialog.
Retrieve with GenericDialog_ReturnImplementation().

(3.1)
*/
void
GenericDialog_SetImplementation		(GenericDialog_Ref	inDialog,
									 void*				inContext)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->userDataPtr = inContext;
}// SetImplementation


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
namespace {

/*!
Constructor.  See GenericDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
My_GenericDialog::
My_GenericDialog	(NSWindow*							inParentNSWindowOrNull,
					 Panel_Ref							inHostedPanel,
					 void*								inDataSetPtr,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
					 HelpSystem_KeyPhrase				inHelpButtonAction)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, GenericDialog_Ref)),
parentWindow					(inParentNSWindowOrNull),
isModal							(nil == inParentNSWindowOrNull),
hostedPanel						(inHostedPanel),
dataSetPtr						(inDataSetPtr),
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
buttonOther						(dialogWindow.returnHIViewWithID(idMyButtonOther)
									<< HIViewWrap_AssertExists),
buttonHICommandsHandler			(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this->selfRef/* user data */),
closeNotifyProc					(inCloseNotifyProcPtr),
closeEffects					(),
windowResizeHandler				(),
contextualHelpSetup				(this->dialogWindow, inHelpButtonAction),
userDataPtr						(nullptr)
{
	OSStatus	error = noErr;
	HISize		panelMarginsTopLeft;
	HISize		panelMarginsBottomRight;
	HISize		windowInitialSize;
	HIViewRef	panelContainer = nullptr;
	
	
	// allow the panel to create its controls by providing a pointer to the containing window
	Panel_SendMessageCreateViews(this->hostedPanel, this->dialogWindow);
	Panel_GetContainerView(this->hostedPanel, panelContainer);
	
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
			UNUSED_RETURN(OSStatus)HIGrowBoxViewSetTransparent(growBox, false);
		}
		else
		{
			UNUSED_RETURN(OSStatus)HIGrowBoxViewSetTransparent(growBox, true);
		}
	}
	
	// set up window resizing
	{
		SInt32		resizeRequirements = Panel_SendMessageGetUsefulResizeAxes(this->hostedPanel);
		UInt16		maxWidthOffset = 200; // arbitrary
		UInt16		maxHeightOffset = 100; // arbitrary
		
		
		// adjust offsets
		if (kPanel_ResponseResizeHorizontal == resizeRequirements)
		{
			maxHeightOffset = 0;
		}
		if (kPanel_ResponseResizeVertical == resizeRequirements)
		{
			maxWidthOffset = 0;
		}
		
		// hide the grow box completely if it is not useful to resize the panel
		if (kPanel_ResponseResizeNotNeeded == resizeRequirements)
		{
			UNUSED_RETURN(OSStatus)ChangeWindowAttributes(this->dialogWindow, 0/* attributes to set */,
															kWindowResizableAttribute/* attributes to clear */);
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
			Console_Warning(Console_WriteLine, "unable to determine ideal size of dialog panel");
			panelIdealSize = CGSizeMake(400.0, 250.0); // arbitrary
		}
		windowInitialSize = CGSizeMake(panelMarginsTopLeft.width + this->panelIdealSize.width + panelMarginsBottomRight.width,
										panelMarginsTopLeft.height + this->panelIdealSize.height + panelMarginsBottomRight.height);
		this->windowResizeHandler.setWindowIdealSize(windowInitialSize.width, windowInitialSize.height);
		this->windowResizeHandler.setWindowMinimumSize(windowInitialSize.width, windowInitialSize.height);
		this->windowResizeHandler.setWindowMaximumSize(windowInitialSize.width + maxWidthOffset,
														windowInitialSize.height + maxHeightOffset);
	}
	
	// resize the window; due to event handlers, a resize of the window
	// changes the panel size too
	SizeWindow(this->dialogWindow, STATIC_CAST(windowInitialSize.width, SInt16),
				STATIC_CAST(windowInitialSize.height, SInt16), true/* update */);
	
	// auto-size and auto-arrange the initial buttons
	this->autoArrangeButtons();
	
	// positioning is not done in the NIB because an arbitrarily-sized panel is
	// added programmatically, and affects the size (and centering) of the window
	if (this->isModal)
	{
		HIWindowRef const	kAnchorWindow = GetUserFocusWindow();
		
		
		UNUSED_RETURN(OSStatus)RepositionWindow(this->dialogWindow, kAnchorWindow,
												(nullptr == kAnchorWindow)
												? kWindowCenterOnMainScreen
												: kWindowCenterOnParentWindowScreen);
	}
	
	// now notify the panel of its data
	{
		Panel_DataSetTransition		dataSetTransition;
		
		
		bzero(&dataSetTransition, sizeof(dataSetTransition));
		dataSetTransition.newDataSet = inDataSetPtr;
		Panel_SendMessageNewDataSet(inHostedPanel, dataSetTransition);
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
Resizes and repositions all of the command buttons in
the dialog, so that they fit their current titles and
are evenly spaced along the bottom of the window.

(4.0)
*/
void
My_GenericDialog::
autoArrangeButtons ()
{
	HIViewRef	buttonArray[] = { this->buttonOK, this->buttonCancel, this->buttonOther };
	
	
	Localization_ArrangeButtonArray(buttonArray, sizeof(buttonArray) / sizeof(HIViewRef));
}// autoArrangeButtons


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
Boolean
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
			UNUSED_RETURN(OSStatus)QuitAppModalLoopForWindow(inPtr->dialogWindow);
			HideWindow(inPtr->dialogWindow);
		}
		else
		{
			Boolean		animateClose = (inPtr->closeEffects.end() == inPtr->closeEffects.find(kHICommandOK)) ||
										(kGenericDialog_DialogEffectCloseNormally == inPtr->closeEffects[kHICommandOK]);
			
			
			if (false == animateClose)
			{
				if (noErr == DetachSheetWindow(inPtr->dialogWindow)) HideWindow(inPtr->dialogWindow);
				else animateClose = true;
			}
			if (animateClose) HideSheetWindow(inPtr->dialogWindow);
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
			UNUSED_RETURN(OSStatus)QuitAppModalLoopForWindow(inPtr->dialogWindow);
			HideWindow(inPtr->dialogWindow);
		}
		else
		{
			Boolean		animateClose = (inPtr->closeEffects.end() == inPtr->closeEffects.find(kHICommandCancel)) ||
										(kGenericDialog_DialogEffectCloseNormally == inPtr->closeEffects[kHICommandCancel]);
			
			
			if (false == animateClose)
			{
				if (noErr == DetachSheetWindow(inPtr->dialogWindow)) HideWindow(inPtr->dialogWindow);
				else animateClose = true;
			}
			if (animateClose) HideSheetWindow(inPtr->dialogWindow);
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
void
handleNewSize	(HIWindowRef	UNUSED_ARGUMENT(inWindow),
				 Float32		inDeltaX,
				 Float32		inDeltaY,
				 void*			inGenericDialogRef)
{
	GenericDialog_Ref			ref = REINTERPRET_CAST(inGenericDialogRef, GenericDialog_Ref);
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), ref);
	
	
	if (Localization_IsLeftToRight())
	{
		ptr->buttonOK << HIViewWrap_MoveBy(inDeltaX, inDeltaY);
		ptr->buttonCancel << HIViewWrap_MoveBy(inDeltaX, inDeltaY);
		ptr->buttonOther << HIViewWrap_MoveBy(inDeltaX, inDeltaY);
		ptr->buttonHelp << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	}
	else
	{
		ptr->buttonOK << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		ptr->buttonCancel << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		ptr->buttonOther << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
		ptr->buttonHelp << HIViewWrap_MoveBy(inDeltaX, inDeltaY);
	}
	Panel_Resizer(inDeltaX, inDeltaY, true/* is delta */)(ptr->hostedPanel);
}// handleNewSize


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the new session dialog.

(3.1)
*/
OSStatus
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
