/*###############################################################

	NewSessionDialog.cp
	
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
#include <climits>
#include <cstdlib>
#include <cstring>
#include <utility>

// Unix includes
extern "C"
{
#	include <netdb.h>
}

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
#include <IconManager.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "ApplicationVersion.h"
#include "DialogResources.h"
#include "MenuResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "DNR.h"
#include "HelpSystem.h"
#include "NetEvents.h"
#include "NewSessionDialog.h"
#include "Preferences.h"
#include "Session.h"
#include "SessionFactory.h"
#include "Terminology.h"
#include "UIStrings.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Dialog" NIB from the package "NewSessionDialog.nib".
*/
enum
{
	kSignatureMyHelpTextCommandLine			= FOUR_CHAR_CODE('CmdH'),
	kSignatureMyFieldCommandLine			= FOUR_CHAR_CODE('CmdL'),
	kSignatureMyPopUpMenuTerminal			= FOUR_CHAR_CODE('Term'),
	kSignatureMyArrowsTerminal				= FOUR_CHAR_CODE('TArr'),
	kSignatureMyHelpTextTerminal			= FOUR_CHAR_CODE('THlp'),
	kSignatureMySeparatorRemoteSessions		= FOUR_CHAR_CODE('RmSD'),
	kSignatureMyPopUpMenuProtocol			= FOUR_CHAR_CODE('Prot'),
	kSignatureMyButtonProtocolOptions		= FOUR_CHAR_CODE('POpt'),
	kSignatureMyFieldHostName				= FOUR_CHAR_CODE('Host'),
	kSignatureMyButtonLookUpHostName		= FOUR_CHAR_CODE('Look'),
	kSignatureMyAsyncArrowsLookUpHostName   = FOUR_CHAR_CODE('Wait'),
	kSignatureMyFieldPortNumber				= FOUR_CHAR_CODE('Port'),
	kSignatureMyFieldUserID					= FOUR_CHAR_CODE('User'),
	kSignatureMySeparatorBottom				= FOUR_CHAR_CODE('BotD'),
	kSignatureMyButtonHelp					= FOUR_CHAR_CODE('Help'),
	kSignatureMyButtonGo					= FOUR_CHAR_CODE('GoDo'),
	kSignatureMyButtonCancel				= FOUR_CHAR_CODE('Canc')
};

#pragma mark Types

struct MyNewSessionDialog
{
	MyNewSessionDialog	(TerminalWindowRef,
						 NewSessionDialog_CloseNotifyProcPtr);
	
	~MyNewSessionDialog	();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	NewSessionDialog_Ref					selfRef;						//!< identical to address of structure, but typed as ref
	TerminalWindowRef						terminalWindow;					//!< the terminal window for which this dialog applies
	WindowRef								screenWindow;					//!< the window for which this dialog applies
	NIBWindow								dialogWindow;					//!< acts as the Mac OS window for the dialog
	WindowInfoRef							windowInfo;						//!< auxiliary data for the dialog
	HIViewWrap								helpTextCommandLine;			//!< indication of what belongs in the command line field
	HIViewWrap								fieldCommandLine;				//!< text of Unix command line to run
	HIViewWrap								popUpMenuTerminal;				//!< menu of terminal favorites
	HIViewWrap								arrowsTerminal;					//!< arrows to navigate terminal favorites menu
	HIViewWrap								helpTextTerminal;				//!< indication that a terminal choice is optional
	HIViewWrap								separatorRemoteSessions;		//!< dividing line above remote session options
	HIViewWrap								popUpMenuProtocol;				//!< menu of available protocols such as TELNET and SSH
	HIViewWrap								buttonProtocolOptions;			//!< might display a dialog of protocol-specific options
	HIViewWrap								fieldHostName;					//!< the name, IP address or other identifier for the remote host
	HIViewWrap								buttonLookUpHostName;			//!< resolves a human-readable name into an IP address
	HIViewWrap								asyncArrowsLookUpHostName;		//!< displays progress while a host name is being resolved
	HIViewWrap								fieldPortNumber;				//!< the port number on which to attempt a connection on the host
	HIViewWrap								fieldUserID;					//!< the (usually 8-character) login name to use for the server
	HIViewWrap								separatorBottom;				//!< dividing line above action buttons
	HIViewWrap								buttonHelp;						//!< displays context-sensitive help on this dialog
	HIViewWrap								buttonGo;						//!< starts the session according to the userÕs configuration
	HIViewWrap								buttonCancel;					//!< aborts
	Session_Protocol						selectedProtocol;				//!< indicates the protocol new remote sessions should use
	CarbonEventHandlerWrap					buttonHICommandsHandler;		//!< invoked when a dialog button is clicked
	CarbonEventHandlerWrap					whenHostNameChangedHandler;		//!< invoked when the host name field changes
	CarbonEventHandlerWrap					whenPortNumberChangedHandler;	//!< invoked when the port number field changes
	CarbonEventHandlerWrap					whenUserIDChangedHandler;		//!< invoked when the user ID field changes
	CarbonEventHandlerWrap					whenCommandLineChangedHandler;	//!< invoked when the command line field changes
	CarbonEventHandlerWrap					whenLookupCompleteHandler;		//!< invoked when a DNS query finally returns
	NewSessionDialog_CloseNotifyProcPtr		closeNotifyProc;				//!< routine to call when the dialog is dismissed
	CommonEventHandlers_PopUpMenuArrowsRef	terminalArrowsHandler;			//!< invoked when the terminal menu arrows are clicked
	CommonEventHandlers_WindowResizer		windowResizeHandler;			//!< invoked when a window has been resized
	HelpSystem_WindowKeyPhraseSetter		contextualHelpSetup;			//!< ensures proper contextual help for this window
	Boolean									noLookupInProgress;				//!< true if a DNS lookup is waiting to complete
};
typedef MyNewSessionDialog*			MyNewSessionDialogPtr;
typedef MyNewSessionDialog const*	MyNewSessionDialogConstPtr;

typedef MemoryBlockPtrLocker< NewSessionDialog_Ref, MyNewSessionDialog >	MyNewSessionDialogPtrLocker;
typedef LockAcquireRelease< NewSessionDialog_Ref, MyNewSessionDialog >		MyNewSessionDialogAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	MyNewSessionDialogPtrLocker&	gNewSessionDialogPtrLocks()	{ static MyNewSessionDialogPtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static void					createSession					(MyNewSessionDialogPtr);
static Boolean				handleItemHit					(MyNewSessionDialogPtr, OSType);
static void					handleNewSize					(WindowRef, Float32, Float32, void*);
static Boolean				lookup							(MyNewSessionDialogPtr);
static void					lookupDoneUI					(MyNewSessionDialogPtr);
static void					lookupWaitUI					(MyNewSessionDialogPtr);
static pascal OSStatus		receiveCommandLineChanged		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveFieldChanged				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveLookupComplete			(EventHandlerCallRef, EventRef, void*);
static MenuItemIndex		returnProtocolMenuCommandIndex	(MyNewSessionDialogPtr, UInt32);
static Boolean				shouldGoButtonBeActive			(MyNewSessionDialogPtr);
static Boolean				shouldLookUpButtonBeActive		(MyNewSessionDialogPtr);
static Boolean				updateCommandLine				(MyNewSessionDialogPtr);
static void					updatePortNumberField			(MyNewSessionDialogPtr);
static void					updateResetLocalFields			(MyNewSessionDialogPtr);
static void					updateResetRemoteFields			(MyNewSessionDialogPtr);
static void					updateUsingSessionFavorite		(MyNewSessionDialogPtr, Preferences_ContextRef);



#pragma mark Public Methods

/*!
This method is used to create a New Session dialog
box.  It creates the dialog box invisibly, and
sets up dialog controls.

(3.0)
*/
NewSessionDialog_Ref
NewSessionDialog_New	(TerminalWindowRef						inTerminalWindow,
						 NewSessionDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	NewSessionDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new MyNewSessionDialog(inTerminalWindow, inCloseNotifyProcPtr), NewSessionDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Call this method to destroy a New Session dialog
box and its associated data structures.  On return,
your copy of the dialog reference is set to nullptr.

(3.0)
*/
void
NewSessionDialog_Dispose	(NewSessionDialog_Ref*	inoutRefPtr)
{
	if (gNewSessionDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked new session dialog; outstanding locks",
							gNewSessionDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, MyNewSessionDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Displays a sheet allowing the user to run a command in a
terminal window, and returns immediately.

IMPORTANT:	Invoking this routine means it is no longer your
			responsibility to call NewSessionDialog_Dispose():
			this is done at an appropriate time after the
			user closes the dialog and after your notification
			routine has been called.

(3.0)
*/
void
NewSessionDialog_Display	(NewSessionDialog_Ref		inDialog)
{
	MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), inDialog);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// display the dialog
		ShowSheetWindow(ptr->dialogWindow, TerminalWindow_ReturnWindow(ptr->terminalWindow));
		HIViewSetNextFocus(HIViewGetRoot(ptr->dialogWindow), ptr->fieldHostName);
		HIViewAdvanceFocus(HIViewGetRoot(ptr->dialogWindow), 0/* modifier keys */);
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
}// Display


/*!
Returns a reference to the terminal window being
customized by a particular dialog.

(3.0)
*/
TerminalWindowRef
NewSessionDialog_ReturnTerminalWindow		(NewSessionDialog_Ref	inDialog)
{
	MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), inDialog);
	TerminalWindowRef				result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->terminalWindow;
	
	return result;
}// ReturnTerminalWindow


/*!
The default handler for closing a New Session dialog.

(3.0)
*/
void
NewSessionDialog_StandardCloseNotifyProc	(NewSessionDialog_Ref	UNUSED_ARGUMENT(inDialogThatClosed),
											 Boolean				UNUSED_ARGUMENT(inOKButtonPressed))
{
	// do nothing
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
Constructor.  See NewSessionDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
MyNewSessionDialog::
MyNewSessionDialog	(TerminalWindowRef						inTerminalWindow,
					 NewSessionDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, NewSessionDialog_Ref)),
terminalWindow					(inTerminalWindow),
screenWindow					(TerminalWindow_ReturnWindow(inTerminalWindow)),
dialogWindow					(NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("PrefPanelSessions"), CFSTR("Resource"))
									<< NIBLoader_AssertWindowExists),
windowInfo						(WindowInfo_New()),
helpTextCommandLine				(dialogWindow.returnHIViewWithCode(kSignatureMyHelpTextCommandLine)
									<< HIViewWrap_AssertExists),
fieldCommandLine				(dialogWindow.returnHIViewWithCode(kSignatureMyFieldCommandLine)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(UnixCommandLineLimiter)),
popUpMenuTerminal				(dialogWindow.returnHIViewWithCode(kSignatureMyPopUpMenuTerminal)
									<< HIViewWrap_AssertExists),
arrowsTerminal					(dialogWindow.returnHIViewWithCode(kSignatureMyArrowsTerminal)
									<< HIViewWrap_AssertExists),
helpTextTerminal				(dialogWindow.returnHIViewWithCode(kSignatureMyHelpTextTerminal)
									<< HIViewWrap_AssertExists),
separatorRemoteSessions			(dialogWindow.returnHIViewWithCode(kSignatureMySeparatorRemoteSessions)
									<< HIViewWrap_AssertExists),
popUpMenuProtocol				(dialogWindow.returnHIViewWithCode(kSignatureMyPopUpMenuProtocol)
									<< HIViewWrap_AssertExists),
buttonProtocolOptions			(dialogWindow.returnHIViewWithCode(kSignatureMyButtonProtocolOptions)
									<< HIViewWrap_AssertExists),
fieldHostName					(dialogWindow.returnHIViewWithCode(kSignatureMyFieldHostName)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(HostNameLimiter)),
buttonLookUpHostName			(dialogWindow.returnHIViewWithCode(kSignatureMyButtonLookUpHostName)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_SetActiveState(shouldLookUpButtonBeActive(this))),
asyncArrowsLookUpHostName		(dialogWindow.returnHIViewWithCode(kSignatureMyAsyncArrowsLookUpHostName)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_SetVisibleState(false)/* initially hidden */),
fieldPortNumber					(dialogWindow.returnHIViewWithCode(kSignatureMyFieldPortNumber)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(NumericalLimiter)),
fieldUserID						(dialogWindow.returnHIViewWithCode(kSignatureMyFieldUserID)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(NoSpaceLimiter)),
separatorBottom					(dialogWindow.returnHIViewWithCode(kSignatureMySeparatorBottom)
									<< HIViewWrap_AssertExists),
buttonHelp						(dialogWindow.returnHIViewWithCode(kSignatureMyButtonHelp)
									<< HIViewWrap_AssertExists
									<< DialogUtilities_SetUpHelpButton),
buttonGo						(dialogWindow.returnHIViewWithCode(kSignatureMyButtonGo)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_SetActiveState(shouldGoButtonBeActive(this))),
buttonCancel					(dialogWindow.returnHIViewWithCode(kSignatureMyButtonCancel)
									<< HIViewWrap_AssertExists),
selectedProtocol				(kSession_ProtocolSSH1), // TEMPORARY: probably should read NIB and determine default menu item to find protocol value
buttonHICommandsHandler			(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this->selfRef/* user data */),
whenHostNameChangedHandler		(GetControlEventTarget(this->fieldHostName), receiveFieldChanged,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this->selfRef/* user data */),
whenPortNumberChangedHandler	(GetControlEventTarget(this->fieldPortNumber), receiveFieldChanged,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this->selfRef/* user data */),
whenUserIDChangedHandler		(GetControlEventTarget(this->fieldUserID), receiveFieldChanged,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this->selfRef/* user data */),
whenCommandLineChangedHandler	(GetControlEventTarget(this->fieldCommandLine), receiveCommandLineChanged,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this->selfRef/* user data */),
whenLookupCompleteHandler		(GetApplicationEventTarget(), receiveLookupComplete,
									CarbonEventSetInClass(CarbonEventClass(kEventClassNetEvents_DNS),
															kEventNetEvents_HostLookupComplete),
									this->selfRef/* user data */),
closeNotifyProc					(inCloseNotifyProcPtr),
terminalArrowsHandler			(nullptr),
windowResizeHandler				(),
contextualHelpSetup				(this->dialogWindow, kHelpSystem_KeyPhraseConnections),
noLookupInProgress				(true)
{
	// set up the Terminals menu
	// TEMPORARY - this should have a notifier so that it is updated if preferences are changed asynchronously
	{
		MenuRef			favoritesMenu = nullptr;
		MenuItemIndex	numberOfTerminalFavorites = 0;
		OSStatus		error = noErr;
		Size			actualSize = 0;
		
		
		error = GetControlData(this->popUpMenuTerminal, kControlMenuPart,
								kControlPopupButtonMenuRefTag, sizeof(favoritesMenu), &favoritesMenu, &actualSize);
		if (noErr == error)
		{
			// first, erase the menu (except for the Default item)
			if (1 != CountMenuItems(favoritesMenu))
			{
				error = DeleteMenuItems(favoritesMenu, 2/* first item */, CountMenuItems(favoritesMenu) - 1);
			}
			
			// add the names of all terminal configurations to the menu
			(Preferences_ResultCode)Preferences_InsertContextNamesInMenu(kPreferences_ClassTerminal, favoritesMenu,
																			0/* after which item */, 0/* indentation level */,
																			numberOfTerminalFavorites);
			
			// adjust limits appropriately
			SetControl32BitMinimum(this->popUpMenuTerminal, 1);
			SetControl32BitMaximum(this->popUpMenuTerminal, CountMenuItems(favoritesMenu));
			SetControl32BitMinimum(this->arrowsTerminal, GetControl32BitMinimum(this->popUpMenuTerminal));
			SetControl32BitMaximum(this->arrowsTerminal, GetControl32BitMaximum(this->popUpMenuTerminal));
		}
	}
	
	if (nullptr != this->dialogWindow)
	{
		// set the controls properly
		updateResetRemoteFields(this);
		updateResetLocalFields(this);
		updateCommandLine(this);
		
		// set up the Window Info information
		WindowInfo_SetWindowDescriptor(this->windowInfo, kConstantsRegistry_WindowDescriptorNewSessions);
		WindowInfo_SetForWindow(this->dialogWindow, this->windowInfo);
	}
	
	// install a callback that causes arrow clicks to change the menu selection
	{
		OSStatus	error = noErr;
		
		
		error = CommonEventHandlers_InstallPopUpMenuArrows(this->arrowsTerminal, this->popUpMenuTerminal,
															&this->terminalArrowsHandler);
		assert_noerr(error);
	}
	
	// install a callback that responds as a window is resized
	{
		Rect		currentBounds;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(this->dialogWindow, kWindowContentRgn, &currentBounds);
		assert_noerr(error);
		this->windowResizeHandler.install(this->dialogWindow, handleNewSize, this->selfRef/* user data */,
											currentBounds.right - currentBounds.left/* minimum width */,
											currentBounds.bottom - currentBounds.top/* minimum height */,
											currentBounds.right - currentBounds.left + 200/* arbitrary maximum width */,
											currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(this->windowResizeHandler.isInstalled());
	}
	
	// ensure other handlers were installed
	assert(buttonHICommandsHandler.isInstalled());
	assert(whenHostNameChangedHandler.isInstalled());
	assert(whenPortNumberChangedHandler.isInstalled());
	assert(whenUserIDChangedHandler.isInstalled());
	assert(whenCommandLineChangedHandler.isInstalled());
}// MyNewSessionDialog 2-argument constructor


/*!
Destructor.  See NewSessionDialog_Dispose().

(3.1)
*/
MyNewSessionDialog::
~MyNewSessionDialog ()
{
	// remove and dispose callbacks
	(OSStatus)CommonEventHandlers_RemovePopUpMenuArrows(&this->terminalArrowsHandler);
	
	DisposeWindow(this->dialogWindow.operator WindowRef());
	WindowInfo_Dispose(this->windowInfo);
}// MyNewSessionDialog destructor


/*!
Initiates a session using the information from
the New Session sheetÕs user interface elements.

(3.0)
*/
static void
createSession	(MyNewSessionDialogPtr	inPtr)
{
	CFStringRef		inputCFString = nullptr;
	
	
	// all that matters is the command line; the UI ensures
	// that if the user enters remote session information,
	// it is transformed into an appropriate command line
	GetControlTextAsCFString(inPtr->fieldCommandLine, inputCFString);
	if ((nullptr == inputCFString) || (CFStringGetLength(inputCFString) <= 0))
	{
		// there has to be some text entered there; let the user
		// know that a blank is unacceptable
		Sound_StandardAlert();
	}
	else
	{
		// dump the process into the terminal window associated with the dialog
		SessionRef		session = nullptr;
		CFArrayRef		argsArray = CFStringCreateArrayBySeparatingStrings
									(kCFAllocatorDefault, inputCFString, CFSTR(" ")/* LOCALIZE THIS? */);
		
		
		if (nullptr != argsArray)
		{
			session = SessionFactory_NewSessionArbitraryCommand
						(inPtr->terminalWindow, argsArray);
			CFRelease(argsArray), argsArray = nullptr;
		}
	}
}// createSession


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
handleItemHit	(MyNewSessionDialogPtr	inPtr,
				 OSType					inControlCode)
{
	Boolean		result = false;
	
	
	switch (inControlCode)
	{
	case kSignatureMyButtonGo:
		// user accepted - close the dialog with an appropriate transition for acceptance;
		// do this prior to creating the session so that the user focus in the terminal
		// can be set properly as a side effect of creating the new session
		HideSheetWindow(inPtr->dialogWindow);
		
		createSession(inPtr);
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			NewSessionDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, true/* OK was pressed */);
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
			NewSessionDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, false/* OK was pressed */);
		}
		break;
	
	case kSignatureMyButtonHelp:
		HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseConnections);
		break;
	
	case kSignatureMyButtonLookUpHostName:
		if (false == lookup(inPtr))
		{
			Sound_StandardAlert();
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// handleItemHit


/*!
Moves or resizes the controls in New Session dialogs.

(3.0)
*/
static void
handleNewSize	(WindowRef	inWindow,
				 Float32	inDeltaX,
				 Float32	inDeltaY,
				 void*		inNewSessionDialogRef)
{
	if (inDeltaX)
	{
		NewSessionDialog_Ref			ref = REINTERPRET_CAST(inNewSessionDialogRef, NewSessionDialog_Ref);
		MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
		
		
		DialogAdjust_BeginControlAdjustment(inWindow);
		{
			DialogItemAdjustment	adjustment;
			SInt32					amountH = 0L;
			
			
			// controls which are moved horizontally
			adjustment = kDialogItemAdjustmentMoveH;
			amountH = STATIC_CAST(inDeltaX, SInt32);
			DialogAdjust_AddControl(adjustment, ptr->arrowsTerminal, amountH);
			DialogAdjust_AddControl(adjustment, ptr->buttonLookUpHostName, amountH);
			DialogAdjust_AddControl(adjustment, ptr->asyncArrowsLookUpHostName, amountH);
			if (Localization_IsLeftToRight())
			{
				DialogAdjust_AddControl(adjustment, ptr->buttonGo, amountH);
				DialogAdjust_AddControl(adjustment, ptr->buttonCancel, amountH);
			}
			else
			{
				DialogAdjust_AddControl(adjustment, ptr->buttonHelp, amountH);
			}
			
			// controls which are resized horizontally
			adjustment = kDialogItemAdjustmentResizeH;
			amountH = STATIC_CAST(inDeltaX, SInt32);
			DialogAdjust_AddControl(adjustment, ptr->helpTextCommandLine, amountH);
			DialogAdjust_AddControl(adjustment, ptr->fieldCommandLine, amountH);
			DialogAdjust_AddControl(adjustment, ptr->popUpMenuTerminal, amountH);
			DialogAdjust_AddControl(adjustment, ptr->helpTextTerminal, amountH);
			DialogAdjust_AddControl(adjustment, ptr->separatorRemoteSessions, amountH);
			DialogAdjust_AddControl(adjustment, ptr->fieldHostName, amountH);
			DialogAdjust_AddControl(adjustment, ptr->separatorBottom, amountH);
		}
		DialogAdjust_EndAdjustment(STATIC_CAST(inDeltaX, SInt32), STATIC_CAST(inDeltaY, SInt32)); // moves and resizes controls properly
	}
}// handleNewSize


/*!
Performs a DNR lookup of the host given in the dialog
boxÕs main text field, and when complete replaces the
contents of the text field with the resolved IP address
(as a string).

Returns true if the lookup succeeded.

(3.0)
*/
static Boolean
lookup	(MyNewSessionDialogPtr	inPtr)
{
	CFStringRef		textCFString = nullptr;
	Boolean			result = false;
	
	
	GetControlTextAsCFString(inPtr->fieldHostName, textCFString);
	if (CFStringGetLength(textCFString) <= 0)
	{
		// there has to be some text entered there; let the user
		// know that a blank is unacceptable
		Sound_StandardAlert();
	}
	else
	{
		char	hostName[256];
		
		
		lookupWaitUI(inPtr);
		if (CFStringGetCString(textCFString, hostName, sizeof(hostName), kCFStringEncodingMacRoman))
		{
			DNR_ResultCode		lookupAttemptResult = kDNR_ResultCodeSuccess;
			
			
			lookupAttemptResult = DNR_New(hostName, false/* use IP version 4 addresses (defaults to IPv6) */);
			if (false == lookupAttemptResult.ok())
			{
				// could not even initiate, so restore UI
				lookupDoneUI(inPtr);
			}
			else
			{
				// okay so far...
				result = true;
			}
		}
	}
	
	return result;
}// lookup


/*!
Changes the user interface to show that a pending
domain name lookup is complete.

See also lookupWaitUI().

(3.1)
*/
static void
lookupDoneUI	(MyNewSessionDialogPtr	inPtr)
{
	// enable buttons again, if appropriate
	inPtr->noLookupInProgress = true;
	HIViewSetVisible(inPtr->asyncArrowsLookUpHostName, false/* visible */);
	inPtr->buttonGo << HIViewWrap_SetActiveState(shouldGoButtonBeActive(inPtr));
	inPtr->buttonLookUpHostName << HIViewWrap_SetActiveState(shouldLookUpButtonBeActive(inPtr));
	inPtr->fieldHostName << HIViewWrap_SetActiveState(true);
}// lookupDoneUI


/*!
Changes the user interface to show that a domain
name lookup is underway.

See also lookupDoneUI().

(3.1)
*/
static void
lookupWaitUI	(MyNewSessionDialogPtr	inPtr)
{
	// disable buttons while waiting for DNR
	inPtr->buttonGo << HIViewWrap_SetActiveState(false);
	inPtr->buttonLookUpHostName << HIViewWrap_SetActiveState(false);
	inPtr->fieldHostName << HIViewWrap_SetActiveState(false);
	HIViewSetVisible(inPtr->asyncArrowsLookUpHostName, true/* visible */);
	inPtr->noLookupInProgress = false;
}// lookupWaitUI


/*!
Embellishes "kEventTextInputUpdateActiveInputArea" of
"kEventClassTextInput" for the command line field
in the new session dialog.  In this case, updating the
command line only needs to check the state of action
buttons, it doesnÕt need to update the command line
(like any other field change would!).

(3.1)
*/
static pascal OSStatus
receiveCommandLineChanged	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inNewSessionDialogRef)
{
	OSStatus						result = eventNotHandledErr;
	NewSessionDialog_Ref			ref = REINTERPRET_CAST(inNewSessionDialogRef, NewSessionDialog_Ref);
	MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	ptr->buttonGo << HIViewWrap_SetActiveState(shouldGoButtonBeActive(ptr));
	ptr->buttonLookUpHostName << HIViewWrap_SetActiveState(shouldLookUpButtonBeActive(ptr));
	
	return result;
}// receiveCommandLineChanged


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the remote session fields
in the new session dialog.  Since a command line is
constructed based on user entries, every change
requires an update to the command line field.

(3.1)
*/
static pascal OSStatus
receiveFieldChanged	(EventHandlerCallRef	inHandlerCallRef,
					 EventRef				inEvent,
					 void*					inNewSessionDialogRef)
{
	OSStatus						result = eventNotHandledErr;
	NewSessionDialog_Ref			ref = REINTERPRET_CAST(inNewSessionDialogRef, NewSessionDialog_Ref);
	MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with the command line field
	updateCommandLine(ptr);
	
	return result;
}// receiveFieldChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the new session dialog.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inNewSessionDialogRef)
{
	OSStatus				result = eventNotHandledErr;
	NewSessionDialog_Ref	ref = REINTERPRET_CAST(inNewSessionDialogRef, NewSessionDialog_Ref);
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
					MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, kSignatureMyButtonGo)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				NewSessionDialog_Dispose(&ref);
				break;
			
			case kHICommandCancel:
				{
					MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, kSignatureMyButtonCancel)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				NewSessionDialog_Dispose(&ref);
				break;
			
			case kCommandContextSensitiveHelp:
				{
					MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, kSignatureMyButtonHelp)) result = eventNotHandledErr;
				}
				break;
			
			case kCommandTerminalDefaultFavorite:
				{
					MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
					
					
					// use the Default Terminal Favorite
					if (handleItemHit(ptr, kSignatureMyPopUpMenuTerminal)) result = eventNotHandledErr;
				}
				break;
			
			case kCommandNewProtocolSelectedFTP:
			case kCommandNewProtocolSelectedSFTP:
			case kCommandNewProtocolSelectedSSH1:
			case kCommandNewProtocolSelectedSSH2:
			case kCommandNewProtocolSelectedTELNET:
				// a new protocol menu item was chosen
				{
					MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
					MenuItemIndex const				kItemIndex = returnProtocolMenuCommandIndex(ptr, received.commandID);
					
					
					switch (received.commandID)
					{
					case kCommandNewProtocolSelectedFTP:
						ptr->selectedProtocol = kSession_ProtocolFTP;
						break;
					
					case kCommandNewProtocolSelectedSFTP:
						ptr->selectedProtocol = kSession_ProtocolSFTP;
						break;
					
					case kCommandNewProtocolSelectedSSH1:
						ptr->selectedProtocol = kSession_ProtocolSSH1;
						break;
					
					case kCommandNewProtocolSelectedSSH2:
						ptr->selectedProtocol = kSession_ProtocolSSH2;
						break;
					
					case kCommandNewProtocolSelectedTELNET:
						ptr->selectedProtocol = kSession_ProtocolTelnet;
						break;
					
					default:
						// ???
						break;
					}
					SetControl32BitValue(ptr->popUpMenuProtocol, kItemIndex);
					updatePortNumberField(ptr);
					(Boolean)updateCommandLine(ptr);
					result = noErr;
				}
				break;
			
			case kCommandShowCommandLine:
				// this normally means Òshow command line floaterÓ, but in the context
				// of an active New Session sheet, it means Òselect command line fieldÓ
				{
					MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
					
					
					(OSStatus)SetKeyboardFocus(ptr->dialogWindow, ptr->fieldCommandLine, kControlEditTextPart);
					result = noErr;
				}
				break;
			
			case kCommandShowProtocolOptions:
				// currently, no protocols support options dialogs
				result = eventNotHandledErr;
				break;
			
			case kCommandLookUpSelectedHostName:
				{
					MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, kSignatureMyButtonLookUpHostName)) result = eventNotHandledErr;
				}
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldnÕt be able to select menu commands while the window is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Handles "kEventNetEvents_HostLookupComplete" of
"kEventClassNetEvents_DNS" by updating the text
field containing the remote host name.

(3.1)
*/
static pascal OSStatus
receiveLookupComplete	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inNewSessionDialogRef)
{
	OSStatus						result = eventNotHandledErr;
	NewSessionDialog_Ref			ref = REINTERPRET_CAST(inNewSessionDialogRef, NewSessionDialog_Ref);
	MyNewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), ref);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassNetEvents_DNS);
	assert(kEventKind == kEventNetEvents_HostLookupComplete);
	{
		struct hostent*		lookupDataPtr = nullptr;
		
		
		// find the lookup results
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_DirectHostEnt,
														typeNetEvents_StructHostEntPtr, lookupDataPtr);
		if (noErr == result)
		{
			// NOTE: The lookup data could be a linked list of many matches.
			// The first is used arbitrarily.
			if ((nullptr != lookupDataPtr->h_addr_list) && (nullptr != lookupDataPtr->h_addr_list[0]))
			{
				CFStringRef		addressCFString = DNR_CopyResolvedHostAsCFString(lookupDataPtr, 0/* which address */);
				
				
				if (nullptr != addressCFString)
				{
					SetControlTextWithCFString(ptr->fieldHostName, addressCFString);
					CFRelease(addressCFString);
					(OSStatus)HIViewSetNeedsDisplay(ptr->fieldHostName, true);
					result = noErr;
				}
			}
			DNR_Dispose(&lookupDataPtr);
		}
	}
	
	lookupDoneUI(ptr);
	
	return result;
}// receiveLookupComplete


/*!
Returns the item index in the protocol pop-up menu
that matches the specified command.

(3.1)
*/
static MenuItemIndex
returnProtocolMenuCommandIndex	(MyNewSessionDialogPtr	inPtr,
								 UInt32					inCommandID)
{
	MenuItemIndex	result = 0;
	OSStatus		error = GetIndMenuItemWithCommandID(GetControlPopupMenuHandle(inPtr->popUpMenuProtocol),
														inCommandID, 1/* 1 = return first match */,
														nullptr/* menu */, &result);
	assert_noerr(error);
	
	
	return result;
}// returnProtocolMenuCommandIndex


/*!
Determines whether or not the Go button should
be available to the user.

(3.1)
*/
static Boolean
shouldGoButtonBeActive	(MyNewSessionDialogPtr	inPtr)
{
	CFStringRef		commandLineCFString = nullptr;
	
	
	GetControlTextAsCFString(inPtr->fieldCommandLine, commandLineCFString);
	return ((nullptr != commandLineCFString) && (CFStringGetLength(commandLineCFString) > 0));
}// shouldGoButtonBeActive


/*!
Determines whether or not the Look Up button
should be available to the user.

(3.1)
*/
static inline Boolean
shouldLookUpButtonBeActive	(MyNewSessionDialogPtr	inPtr)
{
	return ((inPtr->noLookupInProgress) && shouldGoButtonBeActive(inPtr));
}// shouldLookUpButtonBeActive


/*!
Since everything is ÒreallyÓ a local Unix command,
this routine scans the Remote Session options and
updates the command line field with appropriate
values.

Returns "true" only if the update was successful.

NOTE:	Currently, this routine basically obliterates
		whatever the user may have entered.  A more
		desirable solution is to implement this with
		a find/replace strategy that changes only
		those command line arguments that need to be
		updated, preserving any additional parts of
		the command line.

(3.1)
*/
static Boolean
updateCommandLine	(MyNewSessionDialogPtr	inPtr)
{
	Boolean					result = true;
	CFStringRef				hostNameCFString = nullptr;
	CFMutableStringRef		newCommandLineCFString = CFStringCreateMutable(kCFAllocatorDefault,
																			0/* length, or 0 for unlimited */);
	
	// the host field is required when updating the command line
	GetControlTextAsCFString(inPtr->fieldHostName, hostNameCFString);
	if ((nullptr == hostNameCFString) || (0 == CFStringGetLength(hostNameCFString)))
	{
		hostNameCFString = nullptr;
	}
	
	if ((nullptr == newCommandLineCFString) || (nullptr == hostNameCFString))
	{
		// failed to create string!
		result = false;
	}
	else
	{
		CFStringRef		portNumberCFString = nullptr;
		CFStringRef		userIDCFString = nullptr;
		Boolean			standardLoginOptionAppend = false;
		Boolean			standardHostNameAppend = false;
		Boolean			standardPortAppend = false;
		Boolean			standardIPv6Append = false;
		
		
		// see what is available
		// NOTE: In the following, whitespace should probably be stripped
		//       from all field values first, since otherwise the user
		//       could enter a non-empty but blank value that would become
		//       a broken command line.
		GetControlTextAsCFString(inPtr->fieldPortNumber, portNumberCFString);
		if ((nullptr == portNumberCFString) || (0 == CFStringGetLength(portNumberCFString)))
		{
			portNumberCFString = nullptr;
		}
		GetControlTextAsCFString(inPtr->fieldUserID, userIDCFString);
		if ((nullptr == userIDCFString) || (0 == CFStringGetLength(userIDCFString)))
		{
			userIDCFString = nullptr;
		}
		
		// set command based on the protocol, and add arguments
		// based on the specific command and the available data
		switch (inPtr->selectedProtocol)
		{
		case kSession_ProtocolFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ftp"));
			if (nullptr != hostNameCFString)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != userIDCFString)
				{
					CFStringAppend(newCommandLineCFString, userIDCFString);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, hostNameCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardPortAppend = true; // ftp supports a standalone server port number argument
			break;
		
		case kSession_ProtocolSFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/sftp"));
			if (nullptr != hostNameCFString)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != userIDCFString)
				{
					CFStringAppend(newCommandLineCFString, userIDCFString);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, hostNameCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if (nullptr != portNumberCFString)
			{
				// sftp uses "-oPort=port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -oPort="));
				CFStringAppend(newCommandLineCFString, portNumberCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			break;
		
		case kSession_ProtocolSSH1:
		case kSession_ProtocolSSH2:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ssh"));
			if (kSession_ProtocolSSH2 == inPtr->selectedProtocol)
			{
				// -2: protocol version 2
				CFStringAppend(newCommandLineCFString, CFSTR(" -2 "));
			}
			else
			{
				// -1: protocol version 1
				CFStringAppend(newCommandLineCFString, CFSTR(" -1 "));
			}
			if (nullptr != portNumberCFString)
			{
				// ssh uses "-p port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -p "));
				CFStringAppend(newCommandLineCFString, portNumberCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardIPv6Append = true; // ssh supports a "-6" argument if the host is exactly in IP version 6 format
			standardLoginOptionAppend = true; // ssh supports a "-l userid" option
			standardHostNameAppend = true; // ssh supports a standalone server host argument
			break;
		
		case kSession_ProtocolTelnet:
			// -K: disable automatic login
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/telnet -K "));
			standardIPv6Append = true; // telnet supports a "-6" argument if the host is exactly in IP version 6 format
			standardLoginOptionAppend = true; // telnet supports a "-l userid" option
			standardHostNameAppend = true; // telnet supports a standalone server host argument
			standardPortAppend = true; // telnet supports a standalone server port number argument
			break;
		
		default:
			// ???
			result = false;
			break;
		}
		
		if (result)
		{
			// since commands tend to follow similar conventions, this section
			// appends options in ÒstandardÓ form if supported by the protocol
			// (if MOST commands do not have an option, append it in the above
			// switch, instead)
			if ((standardIPv6Append) && (CFStringFind(hostNameCFString, CFSTR(":"), 0/* options */).length > 0))
			{
				// -6: address is in IPv6 format
				CFStringAppend(newCommandLineCFString, CFSTR(" -6 "));
			}
			if ((standardLoginOptionAppend) && (nullptr != userIDCFString))
			{
				// standard form is a Unix command accepting a "-l" option
				// followed by the user ID for login
				CFStringAppend(newCommandLineCFString, CFSTR(" -l "));
				CFStringAppend(newCommandLineCFString, userIDCFString);
			}
			if ((standardHostNameAppend) && (nullptr != hostNameCFString))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the host name of the server to connect to
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, hostNameCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if ((standardPortAppend) && (nullptr != portNumberCFString))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the port number on the server to connect to, AFTER the
				// standalone host name option
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, portNumberCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			
			// update command line field
			if (0 == CFStringGetLength(newCommandLineCFString))
			{
				result = false;
			}
			else
			{
				SetControlTextWithCFString(inPtr->fieldCommandLine, newCommandLineCFString);
				(OSStatus)HIViewSetNeedsDisplay(inPtr->fieldCommandLine, true);
			}
		}
		
		CFRelease(newCommandLineCFString), newCommandLineCFString = nullptr;
	}
	
	// if unsuccessful, clear the command line
	if (false == result)
	{
		SetControlTextWithCFString(inPtr->fieldCommandLine, CFSTR(""));
		(OSStatus)HIViewSetNeedsDisplay(inPtr->fieldCommandLine, true);
	}
	
	inPtr->buttonGo << HIViewWrap_SetActiveState(shouldGoButtonBeActive(inPtr));
	inPtr->buttonLookUpHostName << HIViewWrap_SetActiveState(shouldLookUpButtonBeActive(inPtr));
	return result;
}// updateCommandLine


/*!
Sets the value of the port number field to
match the current-selected protocol.  Use this
to ÒclearÓ its value appropriately.

(3.0)
*/
static void
updatePortNumberField	(MyNewSessionDialogPtr	inPtr)
{
	SInt16		newPort = 0;
	
	
	switch (inPtr->selectedProtocol)
	{
	case kSession_ProtocolFTP:
		newPort = 21;
		break;
	
	case kSession_ProtocolSFTP:
	case kSession_ProtocolSSH1:
	case kSession_ProtocolSSH2:
		newPort = 22;
		break;
	
	case kSession_ProtocolTelnet:
		newPort = 23;
		break;
	
	default:
		// ???
		break;
	}
	
	if (0 != newPort)
	{
		SetControlNumericalText(inPtr->fieldPortNumber, newPort);
		(OSStatus)HIViewSetNeedsDisplay(inPtr->fieldPortNumber, true);
	}
}// updatePortNumberField


/*!
Restores all local session controls to their
default states.

(3.0)
*/
static void
updateResetLocalFields	(MyNewSessionDialogPtr	inPtr)
{
	// set the default command line to be the userÕs shell
	{
		char const*		shell;
		
		
		shell = getenv("SHELL");
		if (nullptr != shell)
		{
			Str255		shellPString;
			
			
			StringUtilities_CToP(shell, shellPString);
			SetControlText(inPtr->fieldCommandLine, shellPString);
		}
	}
}// updateResetLocalFields


/*!
Restores all remote session controls to their
default states.

(3.0)
*/
static void
updateResetRemoteFields		(MyNewSessionDialogPtr	inPtr)
{
	Preferences_ResultCode		preferencesResult = kPreferences_ResultCodeSuccess;
	Preferences_ContextRef		preferencesContext = nullptr;
	
	
	preferencesResult = Preferences_GetDefaultContext(&preferencesContext, kPreferences_ClassSession);
	if (kPreferences_ResultCodeSuccess == preferencesResult)
	{
		// reset text fields
		updateUsingSessionFavorite(inPtr, preferencesContext);
	}
	SetControlTextWithCFString(inPtr->fieldUserID, CFSTR(""));
}// updateResetRemoteFields


/*!
Use this method to acquire a named resource
containing session preferences, and use its
data to set the controls in this dialog.

(3.0)
*/
static void
updateUsingSessionFavorite	(MyNewSessionDialogPtr		inPtr,
							 Preferences_ContextRef		inContext)
{
	CFStringRef				hostCFString = nullptr;
	Preferences_ResultCode	preferencesResult = kPreferences_ResultCodeSuccess;
	
	
	// assume no host, port or command initially
	SetControl32BitValue(inPtr->popUpMenuProtocol, 1/* WARNING: hard-coded index of command in menu */);
	SetControlTextWithCFString(inPtr->fieldHostName, CFSTR(""));
	SetControlTextWithCFString(inPtr->fieldPortNumber, CFSTR(""));
	SetControlTextWithCFString(inPtr->fieldCommandLine, CFSTR(""));
	
	// get host name for this session
	preferencesResult = Preferences_ContextGetData(inContext, kPreferences_TagServerHost,
													sizeof(hostCFString), &hostCFString);
	if (kPreferences_ResultCodeSuccess == preferencesResult)
	{
		Session_Protocol	protocolPreference = kSession_ProtocolTelnet;
		SInt16				port = 0;
		
		
		SetControlTextWithCFString(inPtr->fieldHostName, hostCFString);
		
		// see if there is a port number preference as well
		preferencesResult = Preferences_ContextGetData(inContext, kPreferences_TagServerPort,
														sizeof(port), &port);
		if (kPreferences_ResultCodeSuccess == preferencesResult)
		{
			SetControlNumericalText(inPtr->fieldPortNumber, port);
		}
		
		// see if there is a protocol preference as well
		preferencesResult = Preferences_ContextGetData(inContext, kPreferences_TagServerProtocol,
														sizeof(protocolPreference), &protocolPreference);
		if (kPreferences_ResultCodeSuccess == preferencesResult)
		{
			inPtr->selectedProtocol = protocolPreference;
			switch (protocolPreference)
			{
			case kSession_ProtocolFTP:
				SetControl32BitValue(inPtr->popUpMenuProtocol,
										returnProtocolMenuCommandIndex(inPtr, kCommandNewProtocolSelectedFTP));
				break;
			
			case kSession_ProtocolSFTP:
				SetControl32BitValue(inPtr->popUpMenuProtocol,
										returnProtocolMenuCommandIndex(inPtr, kCommandNewProtocolSelectedSFTP));
				break;
			
			case kSession_ProtocolSSH1:
				SetControl32BitValue(inPtr->popUpMenuProtocol,
										returnProtocolMenuCommandIndex(inPtr, kCommandNewProtocolSelectedSSH1));
				break;
			
			case kSession_ProtocolSSH2:
				SetControl32BitValue(inPtr->popUpMenuProtocol,
										returnProtocolMenuCommandIndex(inPtr, kCommandNewProtocolSelectedSSH2));
				break;
			
			case kSession_ProtocolTelnet:
				SetControl32BitValue(inPtr->popUpMenuProtocol,
										returnProtocolMenuCommandIndex(inPtr, kCommandNewProtocolSelectedTELNET));
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	else
	{
		CFArrayRef		argumentListCFArray = nullptr;
		
		
		// if no host name, perhaps it is actually a command favorite
		preferencesResult = Preferences_ContextGetData(inContext, kPreferences_TagCommandLine,
														sizeof(argumentListCFArray), &argumentListCFArray);
		if (kPreferences_ResultCodeSuccess == preferencesResult)
		{
			CFStringRef		concatenatedString = CFStringCreateByCombiningStrings
													(kCFAllocatorDefault, argumentListCFArray,
														CFSTR(" ")/* separator */);
			
			
			if (nullptr != concatenatedString)
			{
				SetControlTextWithCFString(inPtr->fieldCommandLine, concatenatedString);
				CFRelease(concatenatedString), concatenatedString = nullptr;
			}
		}
	}
	
	(OSStatus)HIViewSetNeedsDisplay(inPtr->fieldHostName, true);
	(OSStatus)HIViewSetNeedsDisplay(inPtr->fieldPortNumber, true);
	(OSStatus)HIViewSetNeedsDisplay(inPtr->fieldCommandLine, true);
	inPtr->buttonGo << HIViewWrap_SetActiveState(shouldGoButtonBeActive(inPtr));
	inPtr->buttonLookUpHostName << HIViewWrap_SetActiveState(shouldLookUpButtonBeActive(inPtr));
}// updateUsingSessionFavorite

// BELOW IS REQUIRED NEWLINE TO END FILE
