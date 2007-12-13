/*###############################################################

	AddressDialog.cp
	
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

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <MemoryBlockPtrLocker.template.h>
#include <NIBLoader.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "AddressDialog.h"
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "HelpSystem.h"
#include "Network.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in the
"Dialog" NIB from the package "AddressDialog.nib".
*/
static HIViewID const	idMyStaticTextMainMessage		= { FOUR_CHAR_CODE('Main'), 0/* ID */ };
static HIViewID const	idMyDataBrowserAddresses		= { FOUR_CHAR_CODE('ALst'), 0/* ID */ };
static HIViewID const	idMyButtonHelp					= { FOUR_CHAR_CODE('Help'), 0/* ID */ };
static HIViewID const	idMyButtonDone					= { FOUR_CHAR_CODE('Done'), 0/* ID */ };
static HIViewID const	idMyButtonCopyToClipboard		= { FOUR_CHAR_CODE('Copy'), 0/* ID */ };

/*!
The following MUST match what is in AddressDialog.nib!
They also cannot use any of Apple’s reserved IDs (0 to 1023).
*/
enum
{
	kMy_DataBrowserPropertyIDAddress		= FOUR_CHAR_CODE('Addr')
};

#pragma mark Types

typedef std::vector< CFRetainRelease >		My_AddressList;

struct My_AddressDialog
{	
	My_AddressDialog	(AddressDialog_CloseNotifyProcPtr);
	
	~My_AddressDialog	();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	AddressDialog_Ref					selfRef;				//!< identical to address of structure, but typed as ref
	NIBWindow							dialogWindow;			//!< acts as the Mac OS window for the dialog
	HIViewWrap							buttonHelp;				//!< displays context-sensitive help on this dialog
	HIViewWrap							buttonDone;				//!< dismisses the window with no action
	HIViewWrap							buttonCopyToClipboard;	//!< copies IP addresses to the clipboard
	HIViewWrap							dataBrowserAddresses;	//!< the list of IP addresses
	CarbonEventHandlerWrap				buttonHICommandsHandler;//!< invoked when a dialog button is clicked
	HelpSystem_WindowKeyPhraseSetter	contextualHelpSetup;	//!< ensures proper contextual help for this window
	AddressDialog_CloseNotifyProcPtr	closeNotifyProc;		//!< routine to call when the dialog is dismissed
	My_AddressList						addressList;			//!< IP address strings
};
typedef My_AddressDialog*	My_AddressDialogPtr;

typedef MemoryBlockPtrLocker< AddressDialog_Ref, My_AddressDialog >		My_AddressDialogPtrLocker;
typedef LockAcquireRelease< AddressDialog_Ref, My_AddressDialog >		My_AddressDialogAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_AddressDialogPtrLocker&		gAddressDialogPtrLocks ()	{ static My_AddressDialogPtrLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static pascal OSStatus	accessDataBrowserItemData	(ControlRef, DataBrowserItemID, DataBrowserPropertyID,
														DataBrowserItemDataRef, Boolean);
static pascal Boolean	compareDataBrowserItems		(ControlRef, DataBrowserItemID, DataBrowserItemID,
														DataBrowserPropertyID);
static OSStatus			copyAddressToClipboard		(CFStringRef);
static OSStatus			copySelectionToClipboard	(HIViewRef);
static Boolean			handleItemHit				(My_AddressDialogPtr, HIViewID const&);
static pascal void		monitorDataBrowserItems		(ControlRef, DataBrowserItemID, DataBrowserItemNotification);
static pascal OSStatus	receiveHICommand			(EventHandlerCallRef, EventRef, void*);



#pragma mark Public Methods

/*!
Creates the dialog box invisibly and initializes it,
arranging to call the specified routine when dismissed
for any reason.

(3.1)
*/
AddressDialog_Ref
AddressDialog_New	(AddressDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	AddressDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_AddressDialog(inCloseNotifyProcPtr), AddressDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Destroys the dialog box and its associated data structures.
On return, your copy of the dialog reference is set to
nullptr.

(3.1)
*/
void
AddressDialog_Dispose	(AddressDialog_Ref*		inoutRefPtr)
{
	if (gAddressDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked IP address dialog; outstanding locks",
							gAddressDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_AddressDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Displays and handles events in the dialog box.  When the
user clicks a button in the dialog, its disposal callback
is invoked.

IMPORTANT:	Invoking this routine means it is no longer your
			responsibility to call AddressDialog_Dispose():
			this is done at an appropriate time after the
			user closes the dialog and after your
			notification routine has been called.

(3.1)
*/
void
AddressDialog_Display	(AddressDialog_Ref		inDialog)
{
	My_AddressDialogAutoLocker		ptr(gAddressDialogPtrLocks(), inDialog);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		// display the dialog
		ShowWindow(ptr->dialogWindow);
		EventLoop_SelectOverRealFrontWindow(ptr->dialogWindow);
		
		(OSStatus)HIViewSetNextFocus(HIViewGetRoot(ptr->dialogWindow), ptr->dataBrowserAddresses);
		(OSStatus)HIViewAdvanceFocus(HIViewGetRoot(ptr->dialogWindow), 0/* modifier keys */);
		
		(OSStatus)RunAppModalLoopForWindow(ptr->dialogWindow);
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
}// Display


/*!
If you only need a close notification procedure for the
purpose of disposing of the dialog reference (and don’t
otherwise care when the dialog closes), you can pass this
standard routine to AddressDialog_New() as your
notification procedure.

(3.1)
*/
void
AddressDialog_StandardCloseNotifyProc	(AddressDialog_Ref	UNUSED_ARGUMENT(inDialogThatClosed),
										 Boolean			UNUSED_ARGUMENT(inOKButtonPressed))
{
	// do nothing
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
Constructor.  See AddressDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
My_AddressDialog::
My_AddressDialog	(AddressDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, AddressDialog_Ref)),
dialogWindow					(NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("AddressDialog"), CFSTR("Dialog"))
									<< NIBLoader_AssertWindowExists),
buttonHelp						(dialogWindow.returnHIViewWithID(idMyButtonHelp)
									<< HIViewWrap_AssertExists
									<< DialogUtilities_SetUpHelpButton),
buttonDone						(dialogWindow.returnHIViewWithID(idMyButtonDone)
									<< HIViewWrap_AssertExists),
buttonCopyToClipboard			(dialogWindow.returnHIViewWithID(idMyButtonCopyToClipboard)
									<< HIViewWrap_AssertExists),
dataBrowserAddresses			(dialogWindow.returnHIViewWithID(idMyDataBrowserAddresses)
									<< HIViewWrap_AssertExists),
buttonHICommandsHandler			(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this->selfRef/* user data */),
contextualHelpSetup				(this->dialogWindow, kHelpSystem_KeyPhraseIPAddresses),
closeNotifyProc					(inCloseNotifyProcPtr),
addressList						()
{
	// ensure other handlers were installed
	assert(buttonHICommandsHandler.isInstalled());
	
	// initialize data browser callbacks so that IP addresses are displayed
	{
		DataBrowserCallbacks	callbacks;
		OSStatus				error = noErr;
		
		
		// define a callback for specifying what data belongs in the list
		callbacks.version = kDataBrowserLatestCallbacks;
		error = InitDataBrowserCallbacks(&callbacks);
		assert_noerr(error);
		callbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(accessDataBrowserItemData);
		assert(nullptr != callbacks.u.v1.itemDataCallback);
		callbacks.u.v1.itemCompareCallback = NewDataBrowserItemCompareUPP(compareDataBrowserItems);
		assert(nullptr != callbacks.u.v1.itemCompareCallback);
		callbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(monitorDataBrowserItems);
		assert(nullptr != callbacks.u.v1.itemNotificationCallback);
		
		// attach data not specified in NIB
		error = SetDataBrowserCallbacks(dataBrowserAddresses, &callbacks);
		assert_noerr(error);
	}
	
	// attach data to view
	{
		OSStatus	error = noErr;
		
		
		error = SetControlProperty(dataBrowserAddresses, kConstantsRegistry_ControlPropertyCreator,
									kConstantsRegistry_ControlPropertyTypeAddressDialog,
									sizeof(this->selfRef), &this->selfRef);
		assert_noerr(error);
	}
	
	// find IP addresses and add list items to represent them
	{
		Boolean		addressesFound = Network_CopyIPAddresses(this->addressList);
		
		
		if (addressesFound)
		{
			DataBrowserItemID*		idList = new DataBrowserItemID[this->addressList.size()];
			OSStatus				error = noErr;
			
			
			for (My_AddressList::size_type i = 0; i < this->addressList.size(); ++i)
			{
				// cast CFStringRefs into IDs for simplicity
				idList[i] = REINTERPRET_CAST(addressList[i].returnCFStringRef(), DataBrowserItemID);
			}
			
			error = AddDataBrowserItems(dataBrowserAddresses, kDataBrowserNoItem/* parent item */,
										this->addressList.size(), idList,
										kMy_DataBrowserPropertyIDAddress/* pre-sort property */);
			assert_noerr(error);
			
			if (false == this->addressList.empty())
			{
				error = SetDataBrowserSelectedItems(dataBrowserAddresses, 1/* number of items */,
													idList, kDataBrowserItemsAssign);
				assert_noerr(error);
			}
			
			delete [] idList;
		}
	}
	
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	// set other nice things (most are set in the NIB already)
	if (FlagManager_Test(kFlagOS10_4API))
	{
		(OSStatus)DataBrowserChangeAttributes(dataBrowserAddresses,
												FUTURE_SYMBOL(1 << 1, kDataBrowserAttributeListViewAlternatingRowColors)/* attributes to set */,
												0/* attributes to clear */);
	}
#endif
	(OSStatus)SetDataBrowserListViewUsePlainBackground(dataBrowserAddresses, false);
}// My_AddressDialog 2-argument constructor


/*!
Destructor.  See AddressDialog_Dispose().

(3.1)
*/
My_AddressDialog::
~My_AddressDialog ()
{
	DisposeWindow(this->dialogWindow.operator WindowRef());
}// My_AddressDialog destructor


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
static pascal OSStatus
accessDataBrowserItemData	(ControlRef					UNUSED_ARGUMENT(inDataBrowser),
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	OSStatus	result = noErr;
	
	
	if (false == inSetValue)
	{
		switch (inPropertyID)
		{
		case kMy_DataBrowserPropertyIDAddress:
			// return the IP address
			result = SetDataBrowserItemDataText(inItemData, REINTERPRET_CAST(inItemID, CFStringRef));
			break;
		
		default:
			// ???
			break;
		}
	}
	else
	{
		switch (inPropertyID)
		{
		case kMy_DataBrowserPropertyIDAddress:
			// read-only
			result = paramErr;
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// accessDataBrowserItemData


/*!
A standard DataBrowserItemCompareProcPtr, this
method compares items in the list.

(3.1)
*/
static pascal Boolean
compareDataBrowserItems		(ControlRef					UNUSED_ARGUMENT(inDataBrowser),
							 DataBrowserItemID			inItemOne,
							 DataBrowserItemID			inItemTwo,
							 DataBrowserPropertyID		inSortProperty)
{
	Boolean			result = false;
	CFStringRef		string1 = nullptr;
	CFStringRef		string2 = nullptr;
	
	
	switch (inSortProperty)
	{
	case kMy_DataBrowserPropertyIDAddress:
		string1 = REINTERPRET_CAST(inItemOne, CFStringRef);
		string2 = REINTERPRET_CAST(inItemTwo, CFStringRef);
		break;
	
	default:
		// ???
		break;
	}
	
	// check for nullptr, because CFStringCompare() will not deal with it
	if ((nullptr == string1) && (nullptr != string2)) result = true;
	else if ((nullptr == string1) || (nullptr == string2)) result = false;
	else
	{
		result = (kCFCompareLessThan == CFStringCompare(string1, string2, kCFCompareNumerically/* options */));
	}
	
	return result;
}// compareDataBrowserItems


/*!
Copies the specified string to the clipboard.
Returns noErr only if successful.

(3.1)
*/
static OSStatus
copyAddressToClipboard	(CFStringRef	inAddressString)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == inAddressString) result = memPCErr;
	else
	{
		ScrapRef	currentScrap = nullptr;
		
		
		(OSStatus)ClearCurrentScrap();
		result = GetCurrentScrap(&currentScrap);
		if (noErr == result)
		{
			CFStringEncoding const	kEncoding = kCFStringEncodingUTF8;
			size_t const			kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
														(CFStringGetLength(inAddressString), kEncoding);
			
			
			try
			{
				char*	addressBuffer = new char[kBufferSize];
				
				
				if (CFStringGetCString(inAddressString, addressBuffer, kBufferSize, kEncoding))
				{
					result = PutScrapFlavor(currentScrap, kScrapFlavorTypeText, kScrapFlavorMaskNone,
											CFStringGetLength(inAddressString), addressBuffer);
				}
				delete [] addressBuffer;
			}
			catch (std::bad_alloc)
			{
				result = memFullErr;
			}
		}
	}
	
	return result;
}// copyAddressToClipboard


/*!
Copies whatever address is selected in the list to the
clipboard.  Returns noErr only if successful.

(3.1)
*/
static OSStatus
copySelectionToClipboard	(HIViewRef		inDataBrowser)
{
	OSStatus			result = noErr;
	DataBrowserItemID	firstSelectedItem = 0;
	DataBrowserItemID	lastSelectedItem = 0;
	
	
	result = GetDataBrowserSelectionAnchor(inDataBrowser, &firstSelectedItem, &lastSelectedItem);
	assert(firstSelectedItem == lastSelectedItem);
	
	if (noErr == result)
	{
		result = copyAddressToClipboard(REINTERPRET_CAST(firstSelectedItem, CFStringRef));
	}
	
	return result;
}// copySelectionToClipboard


/*!
Responds to view selections in the dialog box.
If a button is pressed, the window is closed and
the close notifier routine is called; otherwise,
an adjustment is made to the display.

Returns "true" only if the specified item hit is
to be IGNORED.

(3.1)
*/
static Boolean
handleItemHit	(My_AddressDialogPtr	inPtr,
				 HIViewID const&		inID)
{
	Boolean		result = false;
	
	
	assert(0 == inID.id);
	if (idMyButtonDone.signature == inID.signature)
	{
		// close the dialog with an appropriate transition for acceptance
		(OSStatus)QuitAppModalLoopForWindow(inPtr->dialogWindow);
		HideWindow(inPtr->dialogWindow);
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			AddressDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, true/* OK pressed */);
		}
	}
	else if (idMyButtonCopyToClipboard.signature == inID.signature)
	{
		// user chose to Copy - close the dialog with an appropriate transition
		(OSStatus)QuitAppModalLoopForWindow(inPtr->dialogWindow);
		HideWindow(inPtr->dialogWindow);
		
		// copy the available IP addresses to the clipboard
		{
			OSStatus	error = noErr;
			
			
			error = copySelectionToClipboard(inPtr->dataBrowserAddresses);
			if (noErr != error) Sound_StandardAlert();
		}
		
		// notify of close
		if (nullptr != inPtr->closeNotifyProc)
		{
			AddressDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef, false/* OK pressed */);
		}
	}
	else if (idMyButtonHelp.signature == inID.signature)
	{
		HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseIPAddresses);
	}
	else
	{
		// ???
	}
	
	return result;
}// handleItemHit


/*!
Responds to changes in the data browser.  Currently,
most of the possible messages are ignored, but this
is used to automatically copy double-clicked addresses.

(3.1)
*/
static pascal void
monitorDataBrowserItems		(ControlRef						inDataBrowser,
							 DataBrowserItemID				UNUSED_ARGUMENT(inItemID),
							 DataBrowserItemNotification	inMessage)
{
	OSStatus			error = noErr;
	AddressDialog_Ref	ref = nullptr;
	UInt32				actualSize = 0;
	
	
	error = GetControlProperty(inDataBrowser, kConstantsRegistry_ControlPropertyCreator,
								kConstantsRegistry_ControlPropertyTypeAddressDialog,
								sizeof(ref), &actualSize, &ref);
	assert_noerr(error);
	assert(sizeof(ref) == actualSize);
	
	if (nullptr != ref)
	{
		switch (inMessage)
		{
		case kDataBrowserItemDoubleClicked:
			{
				My_AddressDialogAutoLocker		ptr(gAddressDialogPtrLocks(), ref);
				
				
				error = HIViewSimulateClick(ptr->buttonCopyToClipboard, kControlButtonPart,
											0/* modifiers */, nullptr/* part actually clicked */);
			}
			break;
		
		default:
			// not all messages are supported
			break;
		}
	}
}// monitorDataBrowserItems


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the IP address dialog.

(3.1)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inAddressDialogRef)
{
	OSStatus			result = eventNotHandledErr;
	AddressDialog_Ref	ref = REINTERPRET_CAST(inAddressDialogRef, AddressDialog_Ref);
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
					My_AddressDialogAutoLocker		ptr(gAddressDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, idMyButtonDone)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				AddressDialog_Dispose(&ref);
				break;
			
			case kHICommandCopy:
				{
					My_AddressDialogAutoLocker		ptr(gAddressDialogPtrLocks(), ref);
					
					
					if (handleItemHit(ptr, idMyButtonCopyToClipboard)) result = eventNotHandledErr;
				}
				// do this outside the auto-locker block so that
				// all locks are free when disposal is attempted
				AddressDialog_Dispose(&ref);
				break;
			
			case kCommandContextSensitiveHelp:
				{
					My_AddressDialogAutoLocker		ptr(gAddressDialogPtrLocks(), ref);
					
					
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
