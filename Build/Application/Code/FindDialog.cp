/*###############################################################

	FindDialog.cp
	
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

// standard-C++ includes
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <DialogAdjust.h>
#include <FlagManager.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <IconManager.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "FindDialog.h"
#include "HelpSystem.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "UIStrings.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Dialog" NIB from the package "FindDialog.nib".
*/
enum
{
	kSignatureMyButtonSearch			= FOUR_CHAR_CODE('Find'),
	kSignatureMyButtonCancel			= FOUR_CHAR_CODE('Canc'),
	kSignatureMyButtonHelp				= FOUR_CHAR_CODE('Help'),
	kSignatureMyLabelSearchKeywords		= FOUR_CHAR_CODE('KLbl'),
	kSignatureMyFieldSearchKeywords		= FOUR_CHAR_CODE('KeyW'),
	kSignatureMyKeywordHistoryMenu		= FOUR_CHAR_CODE('HMnu'),
	kSignatureMyTextNotFound			= FOUR_CHAR_CODE('NotF'),
	kSignatureMyIconNotFound			= FOUR_CHAR_CODE('Icon'),
	kSignatureMyArrowsSearchProgress	= FOUR_CHAR_CODE('Prog'),
	kSignatureMyCheckBoxCaseSensitive	= FOUR_CHAR_CODE('XCsS'),
	kSignatureMyCheckBoxOldestFirst		= FOUR_CHAR_CODE('XOLF')
};
static HIViewID const		idMyButtonSearch			= { kSignatureMyButtonSearch,				0/* ID */ };
static HIViewID const		idMyButtonCancel			= { kSignatureMyButtonCancel,				0/* ID */ };
static HIViewID const		idMyButtonHelp				= { kSignatureMyButtonHelp,					0/* ID */ };
static HIViewID const		idMyLabelSearchKeywords		= { kSignatureMyLabelSearchKeywords,		0/* ID */ };
static HIViewID const		idMyFieldSearchKeywords		= { kSignatureMyFieldSearchKeywords,		0/* ID */ };
static HIViewID const		idMyKeywordHistoryMenu		= { kSignatureMyKeywordHistoryMenu,			0/* ID */ };
static HIViewID const		idMyTextNotFound			= { kSignatureMyTextNotFound,				0/* ID */ };
static HIViewID const		idMyIconNotFound			= { kSignatureMyIconNotFound,				0/* ID */ };
static HIViewID const		idMyArrowsSearchProgress	= { kSignatureMyArrowsSearchProgress,		0/* ID */ };
static HIViewID const		idMyCheckBoxCaseSensitive	= { kSignatureMyCheckBoxCaseSensitive,		0/* ID */ };
static HIViewID const		idMyCheckBoxOldestFirst		= { kSignatureMyCheckBoxOldestFirst,		0/* ID */ };

enum
{
	kMy_KeywordHistorySize = 5
};

#pragma mark Types

typedef std::vector< CFStringRef >		KeywordHistoryList;

struct My_FindDialog
{
	My_FindDialog	(TerminalWindowRef				inTerminalWindow,
					 FindDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
					 FindDialog_Options				inFlags);
	~My_FindDialog ();
	
	FindDialog_Ref							selfRef;					//!< identical to address of structure, but typed as ref
	TerminalWindowRef						terminalWindow;				//!< the terminal window for which this dialog applies
	NIBWindow								dialogWindow;				//!< the dialogÕs window
	HIViewWrap								buttonSearch;				//!< Search button
	HIViewWrap								buttonCancel;				//!< Cancel button
	HIViewWrap								labelKeywords;				//!< the label for the keyword text field
	HIViewWrap								fieldKeywords;				//!< the text field containing search keywords
	HIViewWrap								popUpMenuKeywordHistory;	//!< pop-up menu showing previous searches
	HIViewWrap								textNotFound;				//!< message stating that queried text was not found
	HIViewWrap								iconNotFound;				//!< icon that appears when text was not found
	HIViewWrap								arrowsSearchProgress;		//!< the progress indicator during searches
	HIViewWrap								checkboxCaseSensitive;		//!< checkbox indicating whether ÒsimilarÓ letters match
	HIViewWrap								checkboxOldestLinesFirst;	//!< checkbox indicating search direction
	HIViewWrap								buttonHelp;					//!< help button
	
	FindDialog_CloseNotifyProcPtr			closeNotifyProc;			//!< routine to call when the dialog is dismissed
	CarbonEventHandlerWrap					buttonHICommandsHandler;	//!< invoked when a dialog button is clicked
	CarbonEventHandlerWrap					fieldKeyPressHandler;		//!< invoked when a key is pressed while the field is focused
	EventHandlerUPP							historyMenuCommandUPP;		//!< wrapper for button callback function
	EventHandlerRef							historyMenuCommandHandler;	//!< invoked when a dialog button is clicked
	CommonEventHandlers_WindowResizer		windowResizeHandler;		//!< invoked when a window has been resized
	MenuRef									keywordHistoryMenuRef;		//!< history menu
	KeywordHistoryList						keywordHistory;				//!< contents of history menu
};
typedef My_FindDialog*		My_FindDialogPtr;

typedef MemoryBlockPtrLocker< FindDialog_Ref, My_FindDialog >	My_FindDialogPtrLocker;

#pragma mark Internal Method Prototypes

static void					addToHistory					(My_FindDialogPtr, CFStringRef);
static Boolean				handleItemHit					(My_FindDialogPtr, HIViewID const&);
static void					handleNewSize					(WindowRef, Float32, Float32, void*);
static pascal OSStatus		receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveHistoryCommandProcess	(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveKeyPress					(EventHandlerCallRef, EventRef, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_FindDialogPtrLocker&		gFindDialogPtrLocks()	{ static My_FindDialogPtrLocker x; return x; }
}



#pragma mark Public Methods

/*!
This method is used to initialize the Find dialog box.
It creates the dialog box invisibly, and searches only
the contents of the specified window.

If a search history is provided, a copy of the array
is made.

(3.0)
*/
My_FindDialog::
My_FindDialog	(TerminalWindowRef				inTerminalWindow,
				 FindDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
				 FindDialog_Options				inFlags)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef						(REINTERPRET_CAST(this, FindDialog_Ref)),
terminalWindow				(inTerminalWindow),
dialogWindow				(NIBWindow(AppResources_ReturnBundleForNIBs(),
										CFSTR("FindDialog"), CFSTR("Dialog"))
								<< NIBLoader_AssertWindowExists),
buttonSearch				(dialogWindow.returnHIViewWithID(idMyButtonSearch)
								<< HIViewWrap_AssertExists),
buttonCancel				(dialogWindow.returnHIViewWithID(idMyButtonCancel)
								<< HIViewWrap_AssertExists),
labelKeywords				(dialogWindow.returnHIViewWithID(idMyLabelSearchKeywords)
								<< HIViewWrap_AssertExists),
fieldKeywords				(dialogWindow.returnHIViewWithID(idMyFieldSearchKeywords)
								<< HIViewWrap_AssertExists),
popUpMenuKeywordHistory		(dialogWindow.returnHIViewWithID(idMyKeywordHistoryMenu)
								<< HIViewWrap_AssertExists),
textNotFound				(dialogWindow.returnHIViewWithID(idMyTextNotFound)
								<< HIViewWrap_AssertExists),
iconNotFound				(dialogWindow.returnHIViewWithID(idMyIconNotFound)
								<< HIViewWrap_AssertExists),
arrowsSearchProgress		(dialogWindow.returnHIViewWithID(idMyArrowsSearchProgress)
								<< HIViewWrap_AssertExists),
checkboxCaseSensitive		(dialogWindow.returnHIViewWithID(idMyCheckBoxCaseSensitive)
								<< HIViewWrap_AssertExists),
checkboxOldestLinesFirst	(dialogWindow.returnHIViewWithID(idMyCheckBoxOldestFirst)
								<< HIViewWrap_AssertExists),
buttonHelp					(dialogWindow.returnHIViewWithID(idMyButtonHelp)
								<< HIViewWrap_AssertExists
								<< DialogUtilities_SetUpHelpButton),
closeNotifyProc				(inCloseNotifyProcPtr),
buttonHICommandsHandler		(GetWindowEventTarget(this->dialogWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this->selfRef/* user data */),
fieldKeyPressHandler		(CarbonEventUtilities_ReturnViewTarget(this->fieldKeywords), receiveKeyPress,
								CarbonEventSetInClass(CarbonEventClass(kEventClassKeyboard), kEventRawKeyDown),
								this->selfRef/* user data */),
historyMenuCommandUPP		(nullptr),
historyMenuCommandHandler	(nullptr),
windowResizeHandler			(),
keywordHistoryMenuRef		(nullptr),
keywordHistory				(kMy_KeywordHistorySize)
{
	// initialize keyword history
	{
		register SInt16		i = 0;
		
		
		for (i = 0; i < kMy_KeywordHistorySize; ++i) this->keywordHistory[i] = nullptr;
	}
	addToHistory(this, CFSTR(""));
	
	// history menu setup
	{
		OSStatus	error = noErr;
		
		
		error = GetBevelButtonMenuRef(this->popUpMenuKeywordHistory, &this->keywordHistoryMenuRef);
		assert_noerr(error);
		
		// in addition, use a 32-bit icon, if available
		{
			IconManagerIconRef		icon = IconManager_NewIcon();
			
			
			if (icon != nullptr)
			{
				if (IconManager_MakeIconRef(icon, kOnSystemDisk, kSystemIconsCreator, kRecentItemsIcon) == noErr)
				{
					(OSStatus)IconManager_SetButtonIcon(this->popUpMenuKeywordHistory, icon);
				}
				IconManager_DisposeIcon(&icon);
			}
		}
		
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		// set accessibility information, if possible
		if (FlagManager_Test(kFlagOS10_4API))
		{
			// set accessibility title
			CFStringRef		accessibilityDescCFString = nullptr;
			
			
			if (UIStrings_Copy(kUIStrings_ButtonEditTextHistoryAccessibilityDesc, accessibilityDescCFString).ok())
			{
				HIViewRef const		kViewRef = this->popUpMenuKeywordHistory;
				HIObjectRef const	kViewObjectRef = REINTERPRET_CAST(kViewRef, HIObjectRef);
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(kViewObjectRef, 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
		}
	#endif
	}
	
	// initialize the search text field
	// UNIMPLEMENTED
	
	// initialize checkboxes
	SetControl32BitValue(this->checkboxCaseSensitive, (inFlags & kFindDialog_OptionCaseSensitivity) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	SetControl32BitValue(this->checkboxOldestLinesFirst, (inFlags & kFindDialog_OptionOldestLinesFirst) ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
	
	// initially hide the arrows and the error message
	(OSStatus)SetControlVisibility(this->textNotFound, false/* visible */, false/* draw */);
	(OSStatus)SetControlVisibility(this->iconNotFound, false/* visible */, false/* draw */);
	(OSStatus)SetControlVisibility(this->arrowsSearchProgress, false/* visible */, false/* draw */);
	
	// install a callback that handles history menu item selections
	{
		EventTypeSpec const		whenHistoryMenuItemSelected[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		OSStatus				error = noErr;
		
		
		this->historyMenuCommandUPP = NewEventHandlerUPP(receiveHistoryCommandProcess);
		error = InstallMenuEventHandler(this->keywordHistoryMenuRef, this->historyMenuCommandUPP,
										GetEventTypeCount(whenHistoryMenuItemSelected),
										whenHistoryMenuItemSelected, this->selfRef/* user data */,
										&this->historyMenuCommandHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that responds as a window is resized
	{
		Rect	currentBounds;
		
		
		(OSStatus)GetWindowBounds(this->dialogWindow, kWindowContentRgn, &currentBounds);
		this->windowResizeHandler.install(this->dialogWindow, handleNewSize, this->selfRef/* user data */,
											currentBounds.right - currentBounds.left/* minimum width */,
											currentBounds.bottom - currentBounds.top/* minimum height */,
											currentBounds.right - currentBounds.left + 150/* arbitrary maximum width */,
											currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(this->windowResizeHandler.isInstalled());
	}
}// My_FindDialog 3-argument constructor


/*!
Destructor.  See FindDialog_Dispose().

(3.0)
*/
My_FindDialog::
~My_FindDialog ()
{
	register SInt16		i = 0;
	
	
	for (i = 0; i < kMy_KeywordHistorySize; ++i)
	{
		if (this->keywordHistory[i] != nullptr) CFRelease(this->keywordHistory[i]), this->keywordHistory[i] = nullptr;
	}
	
	// clean up the Help System
	HelpSystem_SetWindowKeyPhrase(this->dialogWindow, kHelpSystem_KeyPhraseDefault);
	
	// release all memory occupied by the dialog
	DisposeWindow(this->dialogWindow);
}// My_FindDialog destructor


/*!
This method is used to initialize the Find dialog box.
It creates the dialog box invisibly, and searches only
the contents of the specified window.

(3.0)
*/
FindDialog_Ref
FindDialog_New  (TerminalWindowRef				inTerminalWindow,
				 FindDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
				 FindDialog_Options				inFlags)
{
	FindDialog_Ref		result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_FindDialog(inTerminalWindow, inCloseNotifyProcPtr, inFlags), FindDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Call this method to destroy the screen size dialog
box and its associated data structures.  On return,
your copy of the dialog reference is set to nullptr.

(3.0)
*/
void
FindDialog_Dispose   (FindDialog_Ref*	inoutRefPtr)
{
	if (gFindDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteLine("warning, attempt to dispose of locked Find dialog");
	}
	else
	{
		My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(*inoutRefPtr);
		
		
		delete ptr;
		gFindDialogPtrLocks().releaseLock(*inoutRefPtr, &ptr);
		*inoutRefPtr = nullptr;
	}
}// Dispose


/*!
This method displays and handles events in the
Find dialog box.  When the user clicks a button
in the dialog, its disposal callback is invoked.

(3.0)
*/
void
FindDialog_Display		(FindDialog_Ref		inDialog)
{
	My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(inDialog);
	
	
	if (ptr == nullptr) Alert_ReportOSStatus(paramErr);
	else
	{
		// display the dialog
		ShowSheetWindow(ptr->dialogWindow, TerminalWindow_ReturnWindow(ptr->terminalWindow));
		
		// set keyboard focus
		(OSStatus)SetKeyboardFocus(ptr->dialogWindow, ptr->fieldKeywords, kControlEditTextPart);
		
		// handle events; on Mac OS X, the dialog is a sheet and events are handled via callback
	}
	gFindDialogPtrLocks().releaseLock(inDialog, &ptr);
}// Display


/*!
Returns the string most recently entered into the
search field by the user, WITHOUT retaining it.

(3.0)
*/
void
FindDialog_GetSearchString	(FindDialog_Ref		inDialog,
							 CFStringRef&		outString)
{
	My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(inDialog);
	
	
	if (ptr != nullptr)
	{
		GetControlTextAsCFString(ptr->fieldKeywords, outString);
	}
	gFindDialogPtrLocks().releaseLock(inDialog, &ptr);
}// GetSearchString


/*!
Returns a set of flags indicating whether or not
certain options are enabled for the specified dialog
(which may be open or closed).  Use this inside a
custom close notification routine to save user settings,
for example.  If there are no options enabled, the
result will be "kFindDialog_OptionsAllOff".

(3.0)
*/
FindDialog_Options
FindDialog_ReturnOptions	(FindDialog_Ref		inDialog)
{
	My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(inDialog);
	FindDialog_Options	result = kFindDialog_OptionsAllOff;
	
	
	if (ptr != nullptr)
	{
		if (GetControlValue(ptr->checkboxCaseSensitive) == kControlCheckBoxCheckedValue) result |= kFindDialog_OptionCaseSensitivity;
		if (GetControlValue(ptr->checkboxOldestLinesFirst) == kControlCheckBoxCheckedValue) result |= kFindDialog_OptionOldestLinesFirst;
	}
	gFindDialogPtrLocks().releaseLock(inDialog, &ptr);
	return result;
}// ReturnOptions


/*!
Returns a reference to the terminal window that this
dialog is attached to.

(3.0)
*/
TerminalWindowRef
FindDialog_ReturnTerminalWindow		(FindDialog_Ref		inDialog)
{
	My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(inDialog);
	TerminalWindowRef	result = nullptr;
	
	
	if (ptr != nullptr)
	{
		result = ptr->terminalWindow;
	}
	gFindDialogPtrLocks().releaseLock(inDialog, &ptr);
	return result;
}// ReturnTerminalWindow


/*!
If you only need a close notification procedure
for the purpose of disposing of the Find Dialog
reference (and donÕt otherwise care when a Find
Dialog closes), you can pass this standard routine
to FindDialog_New() as your notification procedure.

(3.0)
*/
void
FindDialog_StandardCloseNotifyProc		(FindDialog_Ref		inDialogThatClosed)
{
	FindDialog_Dispose(&inDialogThatClosed);
}// StandardCloseNotifyProc


#pragma mark Internal Methods

/*!
Adds the specified command line to the history buffer,
shifting all commands back one (and therefore destroying
the oldest one) and resetting the history pointer.  The
given string reference is retained.

(3.0)
*/
static void
addToHistory	(My_FindDialogPtr	inPtr,
				 CFStringRef		inText)
{
	register SInt16		i = 0;
	SInt16 const		kLastItem = kMy_KeywordHistorySize - 1;
	
	
	if (inPtr->keywordHistory[kLastItem] != nullptr) CFRelease(&inPtr->keywordHistory[kLastItem]), inPtr->keywordHistory[kLastItem] = nullptr;
	for (i = kLastItem; i > 0; --i) inPtr->keywordHistory[i] = inPtr->keywordHistory[i - 1];
	inPtr->keywordHistory[0] = inText, CFRetain(inText);
	DeleteMenuItems(inPtr->keywordHistoryMenuRef, 1/* first item */,
					CountMenuItems(inPtr->keywordHistoryMenuRef)/* number of items to delete */);
	for (i = 0; i <= kLastItem; ++i)
	{
		AppendMenuItemTextWithCFString(inPtr->keywordHistoryMenuRef, inPtr->keywordHistory[i], 0/* attributes */,
										0/* command ID */, nullptr/* new itemÕs index */);
	}
}// addToHistory


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
handleItemHit	(My_FindDialogPtr	inPtr,
				 HIViewID const&	inHIViewID)
{
	Boolean		result = false;
	
	
	switch (inHIViewID.signature)
	{
	case kSignatureMyButtonSearch:
		{
			Boolean		foundSomething = false;
			
			
			DeactivateControl(inPtr->buttonSearch);
			SetControlVisibility(inPtr->arrowsSearchProgress, true/* visible */, true/* draw */);
			// initiate synchronous (should be asynchronous!) search - unimplemented
			{
				TerminalScreenRef		screen = TerminalWindow_ReturnScreenWithFocus(inPtr->terminalWindow);
				Terminal_SearchFlags	flags = 0;
				Str255					searchPhrase;
				
				
				GetControlText(inPtr->fieldKeywords, searchPhrase);
				if (GetControlValue(inPtr->checkboxCaseSensitive) == kControlCheckBoxCheckedValue)
				{
					flags |= kTerminal_SearchFlagsCaseSensitive;
				}
				unless (GetControlValue(inPtr->checkboxOldestLinesFirst) == kControlCheckBoxCheckedValue)
				{
					flags |= kTerminal_SearchFlagsReverseBuffer;
				}
				foundSomething = Terminal_SearchForPhrase(screen, (char*)(searchPhrase + 1), PLstrlen(searchPhrase), flags);
			}
			SetControlVisibility(inPtr->arrowsSearchProgress, false/* visible */, false/* draw */);
			ActivateControl(inPtr->buttonSearch);
			
			// donÕt close the dialog unless something was found
			if (foundSomething)
			{
				ClearKeyboardFocus(inPtr->dialogWindow);
				HideSheetWindow(inPtr->dialogWindow);
				
				// show the user where the text is
				TerminalView_ZoomToSelection(TerminalWindow_ReturnViewWithFocus(inPtr->terminalWindow));
				
				// notify of close
				if (inPtr->closeNotifyProc != nullptr)
				{
					FindDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef);
				}
			}
			else
			{
				// show an error message and select all of the text in the keywords field for easy replacement
				SetControlVisibility(inPtr->textNotFound, true/* visible */, true/* draw */);
				SetControlVisibility(inPtr->iconNotFound, true/* visible */, true/* draw */);
				result = true; // pretend the OK button was NOT clicked, so the modal dialog stays open
				{
					(OSStatus)ClearKeyboardFocus(inPtr->dialogWindow);
					(OSStatus)SetKeyboardFocus(inPtr->dialogWindow, inPtr->fieldKeywords, kControlEditTextPart);
				}
			}
			
			// remember this string for later
			{
				CFStringRef		newHistoryString = nullptr;
				
				
				FindDialog_GetSearchString(inPtr->selfRef, newHistoryString);
				addToHistory(inPtr, newHistoryString);
			}
		}
		break;
	
	case kSignatureMyButtonCancel:
		// user cancelled - close the dialog with an appropriate transition for cancelling
		ClearKeyboardFocus(inPtr->dialogWindow);
		HideSheetWindow(inPtr->dialogWindow);
		
		// notify of close
		if (inPtr->closeNotifyProc != nullptr)
		{
			FindDialog_InvokeCloseNotifyProc(inPtr->closeNotifyProc, inPtr->selfRef);
		}
		break;
	
	case kSignatureMyButtonHelp:
		HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseFind);
		break;
	
	default:
		break;
	}
	
	return result;
}// handleItemHit


/*!
Moves or resizes the controls in Find dialogs.

(3.0)
*/
static void
handleNewSize	(WindowRef	UNUSED_ARGUMENT(inWindow),
				 Float32	inDeltaX,
				 Float32	inDeltaY,
				 void*		inFindDialogRef)
{
	// only horizontal changes are significant to this dialog
	if (inDeltaX)
	{
		FindDialog_Ref		ref = REINTERPRET_CAST(inFindDialogRef, FindDialog_Ref);
		My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(ref);
		SInt32				truncDeltaX = STATIC_CAST(inDeltaX, SInt32);
		SInt32				truncDeltaY = STATIC_CAST(inDeltaY, SInt32);
		
		
		DialogAdjust_BeginControlAdjustment(ptr->dialogWindow);
		{
			// controls which are moved horizontally
			if (Localization_IsLeftToRight())
			{
				DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonSearch, truncDeltaX);
				DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonCancel, truncDeltaX);
				DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->popUpMenuKeywordHistory, truncDeltaX);
			}
			else
			{
				DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->buttonHelp, truncDeltaX);
			}
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->iconNotFound, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, ptr->arrowsSearchProgress, truncDeltaX);
			
			// controls which are resized horizontally
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->labelKeywords, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->fieldKeywords, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->textNotFound, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->checkboxCaseSensitive, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, ptr->checkboxOldestLinesFirst, truncDeltaX);
		}
		DialogAdjust_EndAdjustment(truncDeltaX, truncDeltaY); // moves and resizes controls properly
		gFindDialogPtrLocks().releaseLock(ref, &ptr);
	}
}// handleNewSize


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in window title dialogs.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inFindDialogRef)
{
	OSStatus			result = eventNotHandledErr;
	FindDialog_Ref		ref = REINTERPRET_CAST(inFindDialogRef, FindDialog_Ref);
	My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(ref);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
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
				if (handleItemHit(ptr, idMyButtonSearch)) result = eventNotHandledErr;
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
				// wouldnÕt be able to select menu commands while the sheet is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	gFindDialogPtrLocks().releaseLock(ref, &ptr);
	return result;
}// receiveHICommand


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the search word history menu.  Responds by updating
the keywords field to match the contents of the selected
item - however, the event is then passed on so that the
default handler can do other things (like update the
pop-up menuÕs currently-checked item).

(3.0)
*/
static pascal OSStatus
receiveHistoryCommandProcess	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inFindDialogRef)
{
	OSStatus			result = eventNotHandledErr;
	FindDialog_Ref		ref = REINTERPRET_CAST(inFindDialogRef, FindDialog_Ref);
	My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(ref);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	commandInfo;
		
		
		// determine the menu and menu item to process
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, commandInfo);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			// presumably this is the history menu!
			assert(commandInfo.menu.menuRef == ptr->keywordHistoryMenuRef);
			assert(commandInfo.menu.menuItemIndex >= 1);
			{
				CFStringRef		command = ptr->keywordHistory[commandInfo.menu.menuItemIndex - 1];
				
				
				SetControlTextWithCFString(ptr->fieldKeywords, (command == nullptr) ? CFSTR("") : command);
			}
		}
	}
	
	gFindDialogPtrLocks().releaseLock(ref, &ptr);
	return result;
}// receiveHistoryCommandProcess


/*!
Handles "kEventRawKeyDown" of "kEventClassKeyboard"
for the search field.  Currently this just hides
the not-found message, but in the future this will
also initiate search-as-you-type.

(3.0)
*/
static pascal OSStatus
receiveKeyPress		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inFindDialogRef)
{
	OSStatus			result = eventNotHandledErr;
	FindDialog_Ref		ref = REINTERPRET_CAST(inFindDialogRef, FindDialog_Ref);
	My_FindDialogPtr	ptr = gFindDialogPtrLocks().acquireLock(ref);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassKeyboard);
	assert(kEventKind == kEventRawKeyDown);
	{
		SetControlVisibility(ptr->textNotFound, false/* visible */, false/* draw */);
		SetControlVisibility(ptr->iconNotFound, false/* visible */, false/* draw */);
	}
	
	gFindDialogPtrLocks().releaseLock(ref, &ptr);
	return result;
}// receiveKeyPress

// BELOW IS REQUIRED NEWLINE TO END FILE
