/*###############################################################

	Commands.cp
	
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
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

// Unix includes
extern "C"
{
#	include <sys/socket.h>
}

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// resource includes
#include "MenuResources.h"
#include "StringResources.h"

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <Console.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>
#include <Undoables.h>
#include <UniversalPrint.h>

// MacTelnet includes
#include "AddressDialog.h"
#include "AppleEventUtilities.h"
#include "AppResources.h"
#include "BasicTypesAE.h"
#include "Commands.h"
#include "Clipboard.h"
#include "CommandLine.h"
#include "Configure.h"
#include "ContextualMenuBuilder.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Folder.h"
#include "HelpSystem.h"
#include "Keypads.h"
#include "MacroManager.h"
#include "MenuBar.h"
#include "Network.h"
#include "PrefsWindow.h"
#include "RasterGraphicsScreen.h"
#include "RecordAE.h"
#include "Session.h"
#include "SessionDescription.h"
#include "SessionFactory.h"
#include "TelnetPrinting.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "UIStrings.h"
#include "URL.h"
#include "VectorCanvas.h"
#include "WindowTitleDialog.h"



#pragma mark Types
namespace {

struct ScreenTranslationInfo
{
	SInt16		oldTable;		//!< previous translation table number
	SInt16		newTable;		//!< number of translation table to be used from now on
};
typedef ScreenTranslationInfo*		ScreenTranslationInfoPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void		activateAnotherWindow					(Boolean, Boolean);
void		changeNotifyForCommandExecution			(UInt32);
Boolean		isAnyListenerForCommandExecution		(UInt32);
void		showWindowTerminalWindowOp				(TerminalWindowRef, void*, SInt32, void*);
Boolean		translateScreenLine						(TerminalScreenRef, char*, UInt16, UInt16, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_Ref&	gCommandExecutionListenerModel		(Boolean	inDispose = false)
{
	static ListenerModel_Ref x = ListenerModel_New(kListenerModel_StyleNonEventNotHandledErr,
													kConstantsRegistry_ListenerModelDescriptorCommandExecution);
	
	
	if (inDispose) ListenerModel_Dispose(&x);
	return x;
}

} // anonymous namespace



#pragma mark Public Methods

/*!
Determines the most appropriate string to describe
the specified command, given the name type constraint.
Most commands are described by exactly one string,
but occasionally you can get a different string that
is more appropriate for the context you specify.  If
no string specifically matching your context exists,
a more general string is returned.

Returns true as long as at least one string is found
and copied successfully.  Note in particular that you
will not see a false return value when there is no
exact match for your requested string, as long as at
least one other string for the command exists.

(3.0)
*/
Boolean
Commands_CopyCommandName	(UInt32				inCommandID,
							 Commands_NameType  inNameType,
							 CFStringRef&		outName)
{
	Boolean		result = false;
	Boolean		useMenuCommandName = false;
	
	
	switch (inCommandID)
	{
	case kCommandNewSessionDefaultFavorite:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemNewSessionDefault, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandNewSessionLoginShell:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemNewSessionLoginShell, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandNewSessionShell:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemNewSessionShell, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandDisplayWindowContextualMenu:
		switch (inNameType)
		{
		case kCommands_NameTypeDefault:
		default:
			// TEMPORARY - REVIEW THIS
			result = false;
			break;
		}
		break;
	
	default:
		// attempt to derive the name from a matching menu command
		useMenuCommandName = true;
		break;
	}
	
	// sometimes, the menu command name should be used
	if (useMenuCommandName)
	{
		OSStatus	error = noErr;
		
		
		error = MenuBar_CopyMenuItemTextByCommandID(inCommandID, outName);
		if (noErr == error)
		{
			// success!
			result = true;
		}
	}
	
	return result;
}// CopyCommandName


/*!
Attempts to handle a command at the application level,
given its ID (all IDs are declared in "Commands.h").
Returns true only if the event is handled successfully.

Commands can currently be handled in three ways.  First,
Commands_StartHandlingExecution() might have been
invoked to register a local handler (this approach is
deprecated), and if so, it is invoked exclusively.
Otherwise, if the command ID matches one handled at the
application level, it is handled.  The third method is
to install a Carbon Event handler on a more local context
(such as a window), and this is NOT handled here; see
Commands_ExecuteByIDUsingEvent() for that case.

IMPORTANT:	This works ONLY for command IDs that are
			handled here, at the application level.  Many
			events are handled in more local contexts.
			Commands_ExecuteByIDUsingEvent() should
			generally be used instead, so that a command
			invocation request reaches handlers no matter
			where they are installed.

(3.0)
*/
Boolean
Commands_ExecuteByID	(UInt32		inCommandID)
{
	Boolean		result = true;
	
	
	if (isAnyListenerForCommandExecution(inCommandID))
	{
		// TEMPORARY - the following does not return any indicator
		//             as to whether the command was handled!
		changeNotifyForCommandExecution(inCommandID);
	}
	else
	{
		// no handler for the specified command; check against
		// the following list of known commands
		ConnectionDataPtr	currentConnectionDataPtr = nullptr;
		SessionRef			frontSession = nullptr;
		TerminalWindowRef	frontTerminalWindow = nullptr;
		TerminalScreenRef	activeScreen = nullptr;
		TerminalViewRef		activeView = nullptr;
		Boolean				isSession = false; // initially...
		Boolean				isTerminal = false; // initially...
		Boolean				isDialog = false;
		
		
		if (EventLoop_ReturnRealFrontWindow() != nullptr)
		{
			isDialog = (kDialogWindowKind == GetWindowKind(EventLoop_ReturnRealFrontWindow()));
		}
		
		// TEMPORARY: This is a TON of legacy context crap.  Most of this
		// should be refocused or removed.  In addition, it is expensive
		// to do it per-command, it should probably be put into a more
		// global context that is automatically updated whenever the user
		// focus session is changed (and only then).
		frontSession = SessionFactory_ReturnUserFocusSession();
		currentConnectionDataPtr = (nullptr == frontSession) ? nullptr : Session_ConnectionDataPtr(frontSession); // TEMPORARY
		frontTerminalWindow = (nullptr == frontSession) ? nullptr : Session_ReturnActiveTerminalWindow(frontSession);
		activeScreen = (nullptr == frontTerminalWindow) ? nullptr : TerminalWindow_ReturnScreenWithFocus(frontTerminalWindow);
		activeView = (nullptr == frontTerminalWindow) ? nullptr : TerminalWindow_ReturnViewWithFocus(frontTerminalWindow);
		isSession = ((nullptr != frontSession) && (nullptr != currentConnectionDataPtr));
		isTerminal = ((nullptr != activeScreen) && (nullptr != activeView));
		
		switch (inCommandID)
		{
		//case kCommandAboutThisApplication:
		//	see AboutBox.cp
		//  break;
		
		//case kCommandCreditsAndLicenseInfo:
		//	see AboutBox.cp
		//  break;
		
		case kCommandCheckForUpdates:
			// NOTE: There used to be an elaborate HTTP download-and-scan section
			// here, to consult the MacPAD file, determine whether the version is
			// up-to-date, potentially display release notes and a download link,
			// etc.  It was determined that this was not desirable: for one thing,
			// the default implementation was modal and could hang significantly
			// if the Internet connection was down; also, it could take longer than
			// simply downloading a web page, and it was still somewhat unclear
			// whether it would work long-term.  Opening a web page is simple, is
			// pretty fast, is something the user can bookmark in other ways and
			// is easy to maintain, so that is the final solution.
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationApplicationUpdatesPage);
			break;
		
		case kCommandURLHomePage:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationApplicationHomePage);
			break;
		
		case kCommandURLAuthorMail:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationApplicationSupportEMail);
			break;
		
		case kCommandURLSourceLicense:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationSourceCodeLicense);
			break;
		
		case kCommandURLProjectStatus:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationSourceForgeProject);
			break;
		
		//case kCommandNewSessionLoginShell:
		//case kCommandNewSessionShell:
		//case kCommandNewSessionDialog:
		//case kCommandNewSessionDefaultFavorite:
		//case kCommandNewSessionByFavoriteName:
		//	see SessionFactory.cp
		//	break;
		
		case kCommandOpenSession:
			SessionDescription_Load();
			break;
		
		case kCommandOpenScriptMenuItemsFolder:
			{
				FSRef		folderRef;
				OSStatus	error = noErr;
				
				
				error = Folder_GetFSRef(kFolder_RefScriptsMenuItems, folderRef);
				if (noErr == error)
				{
					error = LSOpenFSRef(&folderRef, nullptr/* launched item */);
				}
				if (noErr != error)
				{
					Sound_StandardAlert();
				}
			}
			break;
		
		case kCommandCloseConnection:
			// automatically handled by the OS
			break;
		
		case kCommandSaveSession:
			if (isSession) Session_DisplaySaveDialog(frontSession);
			break;
		
		case kCommandSaveText:
			TerminalView_DisplaySaveSelectedTextUI(activeView);
			break;
		
		case kCommandImportMacroSet:
			{
				Boolean		doImport = false;
				
				
				// only warn the user if there are any macros in the active set that can be destroyed
				if (Macros_AllEmpty(Macros_ReturnActiveSet())) doImport = true;
				else
				{
					InterfaceLibAlertRef	box = nullptr;
					UIStrings_Result		stringResult = kUIStrings_ResultOK;
					CFStringRef				dialogText = nullptr;
					CFStringRef				helpText = nullptr;
					Boolean					releaseDialogText = false;
					Boolean					releaseHelpText = false;
					
					
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowMacroImportWarningPrimaryText, dialogText);
					releaseDialogText = (stringResult.ok());
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowMacroImportWarningHelpText, helpText);
					releaseHelpText = (stringResult.ok());
					
					box = Alert_New();
					Alert_SetHelpButton(box, false);
					Alert_SetParamsFor(box, kAlert_StyleOKCancel);
					Alert_SetTextCFStrings(box, dialogText, helpText);
					Alert_SetType(box, kAlertCautionAlert);
					Sound_StandardAlert();
					Alert_Display(box);
					if (Alert_ItemHit(box) == kAlertStdAlertOKButton) doImport = true;
					Alert_Dispose(&box);
					
					if (releaseDialogText)
					{
						CFRelease(dialogText), dialogText = nullptr;
					}
					if (releaseHelpText)
					{
						CFRelease(helpText), helpText = nullptr;
					}
				}
				
				if (doImport)
				{
					MacroSet						temp = Macros_NewSet();
					MacroManager_InvocationMethod   mode = kMacroManager_InvocationMethodCommandDigit;
					
					
					if (Macros_ImportFromText(temp, (FSSpec*)nullptr, &mode))
					{
						// Then the user did not cancel and no errors occurred,
						// so the macros must have imported successfully.  Tell
						// the user the good news.
						Macros_Copy(temp, Macros_ReturnActiveSet());
						Macros_SetMode(mode);
					}
					Macros_DisposeSet(&temp);
				}
			}
			break;
		
		case kCommandExportCurrentMacroSet:
			// only stop the user if there are no macros in the active set (and therefore no macros to export)
			if (Macros_AllEmpty(Macros_ReturnActiveSet()))
			{
				InterfaceLibAlertRef	box = nullptr;
				UIStrings_Result		stringResult = kUIStrings_ResultOK;
				CFStringRef				dialogText = nullptr;
				CFStringRef				helpText = nullptr;
				CFStringRef				openEditorText = nullptr;
				Boolean					releaseDialogText = false;
				Boolean					releaseHelpText = false;
				Boolean					releaseOpenEditorText = false;
				Boolean					doShowEditor = false;
				
				
				stringResult = UIStrings_Copy(kUIStrings_AlertWindowMacroExportNothingPrimaryText, dialogText);
				releaseDialogText = (stringResult.ok());
				stringResult = UIStrings_Copy(kUIStrings_AlertWindowMacroExportNothingHelpText, helpText);
				releaseHelpText = (stringResult.ok());
				stringResult = UIStrings_Copy(kUIStrings_ButtonOpenMacroEditor, openEditorText);
				releaseOpenEditorText = (stringResult.ok());
				
				// send a Display Alert Apple Event
				box = Alert_New();
				Alert_SetHelpButton(box, false);
				Alert_SetParamsFor(box, kAlert_StyleOK);
				Alert_SetButtonText(box, kAlertStdAlertOtherButton, openEditorText);
				Alert_SetTextCFStrings(box, dialogText, helpText);
				Alert_SetType(box, kAlertStopAlert);
				Sound_StandardAlert();
				Alert_Display(box);
				if (Alert_ItemHit(box) == kAlertStdAlertOtherButton) doShowEditor = true;
				Alert_Dispose(&box);
				
				if (releaseDialogText)
				{
					CFRelease(dialogText), dialogText = nullptr;
				}
				if (releaseHelpText)
				{
					CFRelease(helpText), helpText = nullptr;
				}
				if (releaseOpenEditorText)
				{
					CFRelease(openEditorText), openEditorText = nullptr;
				}
				
				if (doShowEditor)
				{
					Commands_ExecuteByID(kCommandShowMacros);
				}
			}
			else
			{
				Macros_ExportToText(Macros_ReturnActiveSet(), nullptr/* no file; prompt user */, Macros_ReturnMode());
			}
			break;
		
		case kCommandNewDuplicateSession:
			{
				SessionRef		newSession = nullptr;
				
				
				newSession = SessionFactory_NewCloneSession(nullptr/* terminal window */, frontSession);
				
				// report any errors to the user
				if (nullptr == newSession)
				{
					// UNIMPLEMENTED!!!
				}
			}
			break;
		
		case kCommandHandleURL:
			// open the appropriate helper application for the URL in the selected
			// text (which may be MacTelnet itself), and send a “handle URL” event
			if (isSession) URL_HandleForScreenView(activeScreen, activeView);
			else Sound_StandardAlert();
			break;
		
		case kCommandFindFiles:
			// send a Launch Find Apple Event, to allow scripts that may be recording to pick up the command
			{
				AppleEvent		launchFindEvent;
				AppleEvent		reply;
				OSStatus		error = noErr;
				
				
				error = AppleEventUtilities_InitAEDesc(&launchFindEvent);
				assert_noerr(error);
				error = AppleEventUtilities_InitAEDesc(&reply);
				assert_noerr(error);
				
				error = RecordAE_CreateRecordableAppleEvent(kMySuite, kTelnetEventIDLaunchFind,
															&launchFindEvent);
				if (noErr == error)
				{
					error = AESend(&launchFindEvent, &reply, kAENoReply | kAENeverInteract, kAENormalPriority,
									kAEDefaultTimeout, nullptr, nullptr);
					AEDisposeDesc(&launchFindEvent);
				}
				Alert_ReportOSStatus(error);
			}
			break;
		
		case kCommandPrint:
			// print the selection using the print dialog
			UniversalPrint_SetMode(kUniversalPrint_ModeNormal);
			TelnetPrinting_PrintSelection();
			break;
		
		case kCommandPrintOne:
			// print the selection immediately, assuming
			// only one copy is desired and the previous
			// print parameters will be used
			UniversalPrint_SetMode(kUniversalPrint_ModeOneCopy);
			TelnetPrinting_PrintSelection();
			break;
		
		case kCommandPrintScreen:
			{
				TerminalView_CellRange	startEnd;
				TerminalView_CellRange	oldStartEnd;
				Boolean					isRectangular = false;
				Boolean					isSelection = TerminalView_TextSelectionExists(activeView);
				
				
				// preserve any existing selection
				if (isSelection)
				{
					TerminalView_GetSelectedTextAsVirtualRange(activeView, oldStartEnd);
					isRectangular = TerminalView_TextSelectionIsRectangular(activeView);
				}
				else
				{
					oldStartEnd = std::make_pair(std::make_pair(0, 0), std::make_pair(0, 0));
				}
				
				// select the entire visible screen and print it
				startEnd.first = std::make_pair(0, 0);
				startEnd.second = std::make_pair(Terminal_ReturnColumnCount(activeScreen),
													Terminal_ReturnRowCount(activeScreen));
				TerminalView_SelectVirtualRange(activeView, startEnd);
				UniversalPrint_SetMode(kUniversalPrint_ModeNormal);
				TelnetPrinting_PrintSelection();
				
				// clear the full screen selection, restoring any previous selection
				TerminalView_MakeSelectionsRectangular(activeView, isRectangular);
				TerminalView_SelectVirtualRange(activeView, oldStartEnd);
			}
			break;
		
		case kCommandPageSetup:
			// work with a saved reference to a printing record in the preferences file
			//TelnetPrinting_PageSetup(AppResources_ReturnResFile(kAppResources_FileIDPreferences));
			if (isSession) Session_DisplayPrintPageSetupDialog(frontSession);
			break;
		
		case kHICommandQuit:
			// send a Quit Apple Event, to allow scripts that may be recording to pick up the command
			{
				AppleEvent	quitEvent;
				AppleEvent	reply;
				OSStatus	error = noErr;
				
				
				error = AppleEventUtilities_InitAEDesc(&quitEvent);
				assert_noerr(error);
				error = AppleEventUtilities_InitAEDesc(&reply);
				assert_noerr(error);
				error = RecordAE_CreateRecordableAppleEvent(kASRequiredSuite, kAEQuitApplication, &quitEvent);
				if (noErr == error)
				{
					Boolean		confirm = (SessionFactory_ReturnStateCount(kSession_StateActiveStable) > 0);
					
					
					if (confirm)
					{
						AEDesc		saveOptionsDesc;
						DescType	saveOptions = kAEAsk;
						
						
						// add confirmation message parameter based on whether any connections are open
						error = AppleEventUtilities_InitAEDesc(&saveOptionsDesc);
						assert_noerr(error);
						error = BasicTypesAE_CreateEnumerationDesc(saveOptions, &saveOptionsDesc);
						if (noErr == error)
						{
							error = AEPutParamDesc(&quitEvent, keyAESaveOptions, &saveOptionsDesc);
						}
					}
					
					// send the message to quit
					error = AESend(&quitEvent, &reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout,
									nullptr/* idle routine */, nullptr/* filter routine */);
					
					AEDisposeDesc(&quitEvent);
				}
				assert_noerr(error);
			}
			break;
		
		case kCommandUndo:
			Undoables_UndoLastAction();
			break;
		
		case kCommandRedo:
			Undoables_RedoLastUndo();
			break;
		
		case kCommandCut:
		case kCommandCopy:
		case kCommandCopyTable:
		case kCommandClear:
			if ((inCommandID == kCommandCut) ||
				(inCommandID == kCommandCopy) ||
				(inCommandID == kCommandCopyTable))
			{
				// all of these operations involving copying to the clipboard
				SInt16		j = 0;
				
				
				j = MacRGfindwind(EventLoop_ReturnRealFrontWindow()); // ICR window?
				if (j >= 0) MacRGcopy(EventLoop_ReturnRealFrontWindow()); // copy the ICR window
				else if (isDialog)
				{
					if (inCommandID == kCommandCut) DialogCut(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
					else if (inCommandID == kCommandCopy) DialogCopy(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
				}
				else
				{
					if (VectorCanvas_GetFromWindow(EventLoop_ReturnRealFrontWindow(), &j))
					{
						Clipboard_GraphicsToScrap(j); // copy graphics
					}
					else
					{
						// copy text by sending a recordable Apple Event back to MacTelnet
						AppleEvent		copyEvent;
						AppleEvent		reply;
						OSStatus		error = noErr;
						
						
						error = AppleEventUtilities_InitAEDesc(&copyEvent);
						assert_noerr(error);
						error = AppleEventUtilities_InitAEDesc(&reply);
						assert_noerr(error);
						
						error = RecordAE_CreateRecordableAppleEvent(kMySuite, kTelnetEventIDCopyToClipboard,
																		&copyEvent);
						if (noErr == error)
						{
							// include “copy using spaces for tabs” option
							{
								Boolean		copyTable = (inCommandID == kCommandCopyTable);
								
								
								error = AEPutParamPtr(&copyEvent, keyAEParamMyTabsInPlaceOfMultipleSpaces,
														typeBoolean, &copyTable, sizeof(copyTable));
							}
							
							error = AESend(&copyEvent, &reply, kAENoReply | kAENeverInteract, kAENormalPriority,
											kAEDefaultTimeout, nullptr, nullptr);
							
							AEDisposeDesc(&copyEvent);
						}
						Alert_ReportOSStatus(error);
					}
				}
			}
			if ((inCommandID == kCommandCut) || (inCommandID == kCommandClear))
			{
				// delete selection -- unimplemented, impossible to implement?
				// at least, impossible until MacTelnet supports “generic”
				// terminal windows (i.e. windows that are not tied to read-only
				// concepts like connections to remote servers)
				if (isDialog)
				{
					DialogDelete(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
				}
			}
			break;
		
		case kCommandCopyAndPaste:
			if (isSession) Clipboard_TextType(activeView, frontSession); // type if there is a window to type into
			break;
		
		case kCommandPaste:	
			if (isSession) Session_UserInputPaste(frontSession); // paste if there is a window to paste into
			else if (isDialog) DialogPaste(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
			break;
		
		//case kCommandFind:
		//case kCommandFindAgain:
		//	see TerminalWindow.cp
		//	break;
		
		//case kCommandFindCursor:
		//	see TerminalWindow.cp
		//	break;
		
		case kCommandSelectAll: // text only
		case kCommandSelectAllWithScrollback: // text only
		case kCommandSelectNothing: // text only
			if (isDialog)
			{
				// select the entire text in the focused field
				// unimplemented
				Sound_StandardAlert();
			}
			else if (isSession)
			{
				if (inCommandID == kCommandSelectAllWithScrollback)
				{
					// select entire scrollback buffer
					TerminalView_SelectEntireBuffer(activeView);
				}
				else if (inCommandID == kCommandSelectNothing)
				{
					// remove selection
					TerminalView_SelectNothing(activeView);
				}
				else
				{
					// select only the bottomost windowful of text
					TerminalView_SelectMainScreen(activeView);
				}
			}
			break;
		
		case kCommandShowClipboard:
			Clipboard_SetWindowVisible(true);
			break;
		
		case kCommandHideClipboard:
			Clipboard_SetWindowVisible(false);
			break;
		
		case kHICommandPreferences:
			PrefsWindow_Display();
			break;
		
		//case kCommandLargeScreen:
		//case kCommandSmallScreen:
		//case kCommandTallScreen:
		//	see TerminalWindow.cp
		//	break;
		
		//case kCommandSetScreenSize:
		//	see TerminalWindow.cp
		//	break;
		
		//case kCommandBiggerText:
		//case kCommandSmallerText:
		//	see TerminalWindow.cp
		//	break;
		
		//case kCommandFormatDefault:
		//case kCommandFormatByFavoriteName:
		//case kCommandFormat:
		//	see TerminalWindow.cp
		//	break;
		
		//case kCommandFullScreen:
		//case kCommandFullScreenModal:
		//	see TerminalWindow.cp
		//	break;
		
		//case kHICommandShowToolbar:
		//case kHICommandHideToolbar:
		//case kHICommandCustomizeToolbar:
		//	see EventLoop.cp
		//	break;
			
		case kCommandTerminalEmulatorSetup:
			// UNIMPLEMENTED
			Sound_StandardAlert();
			break;
			
		case kCommandBellEnabled:
			if (isTerminal)
			{
				Terminal_SetBellEnabled(activeScreen, !Terminal_BellIsEnabled(activeScreen));
			}
			break;
		
		case kCommandEcho:
			if (isSession)
			{
				// toggle local echo, if possible
				if (Session_LocalEchoIsEnabled(frontSession) && (currentConnectionDataPtr->kblen > 0))
				{
					// if not empty, send buffer
					Session_SendData(frontSession, currentConnectionDataPtr->kbbuf, currentConnectionDataPtr->kblen);
					currentConnectionDataPtr->kblen = 0;
				}
				Session_SetLocalEchoEnabled(frontSession, !Session_LocalEchoIsEnabled(frontSession));
			}
			break;
			
		case kCommandWrapMode:
			if (isTerminal)
			{
				Terminal_SetLineWrapEnabled(activeScreen, !Terminal_LineWrapIsEnabled(activeScreen));
			}
			break;
		
		case kCommandClearScreenSavesLines:
			if (isTerminal)
			{
				Terminal_SetSaveLinesOnClear(activeScreen, !Terminal_SaveLinesOnClearIsEnabled(activeScreen));
			}
			break;
		
		case kCommandJumpScrolling:
			if (isSession)
			{
				Session_FlushNetwork(frontSession);
			}
			break;
		
		case kCommandCaptureToFile:
			if (isSession)
			{
				Session_DisplayFileCaptureSaveDialog(frontSession);
				//if (Terminal_FileCaptureBegin(activeScreen, nullptr/* let user choose a file */))
				//{
				//	// then capturing is on
				//}
			}
			break;
		
		case kCommandEndCaptureToFile:
			if (isSession)
			{
				Terminal_FileCaptureEnd(activeScreen);
			}
			break;
		
		case kCommandTEKPageCommand:
			// Tektronix page command
			if (isSession)
			{
				if (!Session_TEKHasTargetGraphic(frontSession) ||
					Session_TEKPageCommandOpensNewWindow(frontSession))
				{
					if (Session_TEKPageCommandOpensNewWindow(frontSession))
					{
						// leave current TEK mode, then re-enter to open a new window
						Session_TEKDetachTargetGraphic(frontSession); // this removes the data target
					}
					
					unless (Session_TEKHasTargetGraphic(frontSession))
					{
						// then there is no current TEK window for this screen - make one
						// (this also makes it one of the session’s data targets)
						unless (Session_TEKCreateTargetGraphic(frontSession))
						{
							// error - can’t create TEK window; get out of TEK mode
							// UNIMPLEMENTED
						}
					}
				}
			}
			break;
		
		case kCommandTEKPageClearsScreen:
			// toggle TEK page behavior
			if (isSession)
			{
				Session_TEKSetPageCommandOpensNewWindow(frontSession, false == Session_TEKPageCommandOpensNewWindow(frontSession));
			}
			break;
		
		case kCommandSpeechEnabled:
			if (isSession)
			{
				if (Session_SpeechIsEnabled(frontSession))
				{
					Session_SpeechPause(frontSession);
					Session_SetSpeechEnabled(frontSession, false);
				}
				else
				{
					Session_SetSpeechEnabled(frontSession, true);
					Session_SpeechResume(frontSession);
				}
			}
			break;
		
		case kCommandSpeakSelectedText:
			if (isTerminal)
			{
				TerminalView_GetSelectedTextAsAudio(activeView);
			}
			break;
		
		case kCommandClearEntireScrollback:
			if (isTerminal)
			{
				TerminalView_DeleteScrollback(activeView);
			}
			break;
		
		case kCommandResetGraphicsCharacters:
			if (isTerminal)
			{
				Terminal_Reset(activeScreen, kTerminal_ResetFlagsGraphicsCharacters);
			}
			break;
		
		case kCommandResetTerminal:
			if (isTerminal)
			{
				Terminal_Reset(activeScreen);
			}
			break;
		
		case kCommandDeletePressSendsBackspace:
		case kCommandDeletePressSendsDelete:
			if (isSession)
			{
				currentConnectionDataPtr->bsdel = (inCommandID == kCommandDeletePressSendsDelete);
			}
			break;
		
		case kCommandEMACSArrowMapping:
			if (isSession)
			{
				currentConnectionDataPtr->arrowmap = !currentConnectionDataPtr->arrowmap;
			}
			break;
		
		case kCommandLocalPageUpDown:
			if (isSession)
			{
				currentConnectionDataPtr->pgupdwn = !currentConnectionDataPtr->pgupdwn;
			}
			break;
		
		case kCommandSetKeys:
			if (isSession)
			{
				Session_DisplaySpecialKeySequencesDialog(frontSession);
			}
			break;
		
		case kCommandShowNetworkNumbers:
			{
				AddressDialog_Ref		dialog = AddressDialog_New(AddressDialog_StandardCloseNotifyProc);
				
				
				AddressDialog_Display(dialog); // automatically disposed when the user clicks a button
			}
			break;
		
		case kCommandSendInternetProtocolNumber:
			if (isSession)
			{
				std::string		ipAddressString;
				int				addressType = 0;
				
				
				// send whatever keystrokes are still buffered
				if (Session_LocalEchoIsEnabled(frontSession) && (currentConnectionDataPtr->kblen > 0))
				{
					// if not empty, send buffer
					Session_FlushUserInputBuffer(frontSession);
				}
				
				// now send the requested information
				if (Network_CurrentIPAddressToString(ipAddressString, addressType)) // 3.0
				{
					Session_SendData(frontSession, ipAddressString.c_str(), ipAddressString.size());
					if (Session_LocalEchoIsEnabled(frontSession))
					{
						Session_ReceiveData(frontSession, ipAddressString.c_str(), ipAddressString.size());
					}
				}
				else Sound_StandardAlert(); // IP-to-string failed in this case
			}
			break;

		case kCommandSendInterruptProcess:
			Session_UserInputInterruptProcess(frontSession, false/* record to scripts */);
			break;
		
		case kCommandWatchNothing:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchNothing);
			}
			break;
		
		case kCommandWatchForActivity:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchForPassiveData);
			}
			break;
		
		case kCommandWatchForInactivity:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchForInactivity);
			}
			break;
		
		case kCommandTransmitOnInactivity:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchForKeepAlive);
			}
			break;
			
		case kCommandSuspendNetwork:
			if (nullptr != frontSession)
			{
				Session_SetNetworkSuspended(frontSession, !Session_NetworkIsSuspended(frontSession));
			}
			break;
		
		case kCommandMinimizeWindow:
			// behave as if the user clicked the collapse box of the frontmost window
			{
				WindowRef		frontWindow = EventLoop_ReturnRealFrontWindow();
				Boolean			collapsing = false;
				
				
				if (nullptr != frontWindow)
				{
				#if TARGET_API_MAC_CARBON
					WindowRef		sheetParentWindow = nullptr;
					
					
					// is the frontmost window a sheet?
					if (GetSheetWindowParent(frontWindow, &sheetParentWindow) == noErr)
					{
						// the front window is a sheet; minimize its parent window instead
						frontWindow = sheetParentWindow;
					}
				#endif
					collapsing = !IsWindowCollapsed(frontWindow);
					CollapseWindow(frontWindow, collapsing);
				}
			}
			break;
		
		case kCommandZoomWindow:
		case kCommandMaximizeWindow:
			// do not zoom windows this way, pass events to specific windows
			// (for example, EventLoop_HandleZoomEvent())
			result = false;
			break;
		
		case kCommandChangeWindowTitle:
			// display a dialog allowing the user to change the title of a terminal window
			if (EventLoop_ReturnRealFrontWindow() == nullptr) Sound_StandardAlert();
			else
			{
				WindowTitleDialog_Ref		dialog = WindowTitleDialog_NewForSession(frontSession);
				
				
				WindowTitleDialog_Display(dialog); // automatically disposed when the user clicks a button
			}
			break;
		
		//case kCommandHideFrontWindow:
		//	see TerminalWindow.cp
		//	break;
		
		//case kCommandHideOtherWindows:
		//	see TerminalWindow.cp
		//	break;
		
		case kCommandShowAllHiddenWindows:
			// show all windows
			SessionFactory_ForEveryTerminalWindowDo(showWindowTerminalWindowOp, 0L/* data 1 - unused */,
													0L/* data 2 - unused */, nullptr/* pointer to result - unused */);
			break;
		
		//case kCommandKioskModeDisable:
		//	see TerminalWindow.cp
		//	break;
		
		case kCommandStackWindows:
			{
				// on Mac OS X, this command also requires that all application windows come to the front
				ProcessSerialNumber		psn;
				
				
				if (GetCurrentProcess(&psn) == noErr)
				{
					(OSStatus)SetFrontProcess(&psn);
				}
				
				// arrange windows in a diagonal pattern
				TerminalWindow_StackWindows();
			}
			break;
		
		case kCommandNextWindow:
			// activate the next window in the window list (terminal windows only)
			activateAnotherWindow(false/* previous */, false/* hide current */);
			break;
		
		case kCommandNextWindowHideCurrent:
			// activate the next window in the window list (terminal windows only),
			// but first obscure the frontmost terminal window
			activateAnotherWindow(false/* previous */, true/* hide current */);
			break;
		
		case kCommandPreviousWindow:
			// activate the previous window in the window list (terminal windows only)
			activateAnotherWindow(true/* previous */, false/* hide current */);
			break;
		
		case kCommandPreviousWindowHideCurrent:
			// activate the previous window in the window list (terminal windows only),
			// but first obscure the frontmost terminal window
			activateAnotherWindow(true/* previous */, true/* hide current */);
			break;
		
		//case kCommandShowConnectionStatus:
		//case kCommandHideConnectionStatus:
		//	see InfoWindow.cp
		//	break;
		
		case kCommandShowCommandLine:
			{
				WindowRef	window = CommandLine_ReturnWindow();
				
				
				ShowWindow(window);
				CommandLine_Focus();
			}
			break;
		
		case kCommandHideCommandLine:
			if (CommandLine_IsVisible())
			{
				WindowRef	window = CommandLine_ReturnWindow();
				
				
				(OSStatus)SetUserFocusWindow(REINTERPRET_CAST(kUserFocusAuto, WindowRef));
				HideWindow(window);
			}
			break;
		
		//kCommandShowHidePrefCollectionsDrawer:
		//kCommandDisplayPrefPanelFormats:
		//kCommandDisplayPrefPanelGeneral:
		//kCommandDisplayPrefPanelKiosk:
		//kCommandDisplayPrefPanelMacros:
		//kCommandDisplayPrefPanelScripts:
		//kCommandDisplayPrefPanelSessions:
		//kCommandDisplayPrefPanelTerminals:
		//	see PrefsWindow.cp
		//	break;
		
		//kCommandShowMacros:
		//kCommandHideMacros:
		//	see MacroSetupWindow.cp
		//	break;
		
		//kCommandShowOrHideMacros:
		//	see MacroSetupWindow.cp
		//	break;
		
		case kCommandShowControlKeys:
			{
				WindowRef	window = Keypads_ReturnWindow(kKeypads_WindowTypeControlKeys);
				
				
				ShowWindow(window);
			}
			break;
		
		case kCommandHideControlKeys:
			{
				WindowRef	window = Keypads_ReturnWindow(kKeypads_WindowTypeControlKeys);
				
				
				HideWindow(window);
			}
			break;
		
		case kCommandShowFunction:
			{
				WindowRef	window = Keypads_ReturnWindow(kKeypads_WindowTypeFunctionKeys);
				
				
				ShowWindow(window);
			}
			break;
		
		case kCommandHideFunction:
			{
				WindowRef	window = Keypads_ReturnWindow(kKeypads_WindowTypeFunctionKeys);
				
				
				HideWindow(window);
			}
			break;
		
		case kCommandShowKeypad:
			{
				WindowRef	window = Keypads_ReturnWindow(kKeypads_WindowTypeVT220Keys);
				
				
				ShowWindow(window);
			}
			break;
		
		case kCommandHideKeypad:
			{
				WindowRef	window = Keypads_ReturnWindow(kKeypads_WindowTypeVT220Keys);
				
				
				HideWindow(window);
			}
			break;
		
		case kCommandMainHelp:
			HelpSystem_DisplayHelpWithoutContext();
			break;
		
		case kCommandContextSensitiveHelp:
			// open MacTelnet Help to a particular topic
			HelpSystem_DisplayHelpInCurrentContext();
			break;
		
		case kCommandShowHelpTags:
			HMSetHelpTagsDisplayed(true);
			break;
		
		case kCommandHideHelpTags:
			HMSetHelpTagsDisplayed(false);
			break;
		
		case kCommandDisplayWindowContextualMenu: // GOING AWAY
			// display contextual menu for main focus area
			if (frontTerminalWindow != nullptr)
			{
				WindowRef		window = TerminalWindow_ReturnWindow(frontTerminalWindow);
				EventRecord		fakeEvent;
				
				
				fakeEvent.what = mouseDown;
				fakeEvent.message = 0;
				fakeEvent.when = 0;
				fakeEvent.modifiers = EventLoop_ReturnCurrentModifiers();
				GetGlobalMouse(&fakeEvent.where);
				(OSStatus)ContextualMenuBuilder_DisplayMenuForWindow(window, &fakeEvent, inContent);
			}
			break;
		
		case kCommandToggleMacrosMenuVisibility:
			{
				Preferences_Result		preferencesResult = kPreferences_ResultOK;
				size_t					actualSize = 0;
				Boolean					isVisible = false;
				
				
				preferencesResult = Preferences_GetData(kPreferences_TagMacrosMenuVisible,
														sizeof(isVisible), &isVisible,
														&actualSize);
				unless (preferencesResult == kPreferences_ResultOK)
				{
					isVisible = false; // assume a value, if preference can’t be found
				}
				
				isVisible = !isVisible;
				
				preferencesResult = Preferences_SetData(kPreferences_TagMacrosMenuVisible,
														sizeof(isVisible), &isVisible);
			}
			break;
		
		case kCommandSendMacro1:
			MacroManager_UserInputMacroString(frontSession, 0/* zero-based macro number */);
			break;
		
		case kCommandSendMacro2:
			MacroManager_UserInputMacroString(frontSession, 1/* zero-based macro number */);
			break;
		
		case kCommandSendMacro3:
			MacroManager_UserInputMacroString(frontSession, 2/* zero-based macro number */);
			break;
		
		case kCommandSendMacro4:
			MacroManager_UserInputMacroString(frontSession, 3/* zero-based macro number */);
			break;
		
		case kCommandSendMacro5:
			MacroManager_UserInputMacroString(frontSession, 4/* zero-based macro number */);
			break;
		
		case kCommandSendMacro6:
			MacroManager_UserInputMacroString(frontSession, 5/* zero-based macro number */);
			break;
		
		case kCommandSendMacro7:
			MacroManager_UserInputMacroString(frontSession, 6/* zero-based macro number */);
			break;
		
		case kCommandSendMacro8:
			MacroManager_UserInputMacroString(frontSession, 7/* zero-based macro number */);
			break;
		
		case kCommandSendMacro9:
			MacroManager_UserInputMacroString(frontSession, 8/* zero-based macro number */);
			break;
		
		case kCommandSendMacro10:
			MacroManager_UserInputMacroString(frontSession, 9/* zero-based macro number */);
			break;
		
		case kCommandSendMacro11:
			MacroManager_UserInputMacroString(frontSession, 10/* zero-based macro number */);
			break;
		
		case kCommandSendMacro12:
			MacroManager_UserInputMacroString(frontSession, 11/* zero-based macro number */);
			break;
		
		case kCommandChangeMacroKeysToCommandKeypad:
		case kCommandChangeMacroKeysToFunctionKeys:
			{
				// change the mode; since the Macro Setup window listens
				// for macro mode changes, this will trigger changes to
				// the window’s radio buttons and text labels
				Macros_SetMode((inCommandID == kCommandChangeMacroKeysToFunctionKeys)
								? kMacroManager_InvocationMethodFunctionKeys
								: kMacroManager_InvocationMethodCommandDigit);
				
				// now send an event back so that scripts can record this
				// (man, what a ton of code for one simple thing, huh?)
				{
					OSStatus		error = noErr;
					AppleEvent		setDataEvent;
					AppleEvent		reply;
					FourCharCode	newKeys = (inCommandID == kCommandChangeMacroKeysToFunctionKeys)
												? kTelnetEnumeratedClassMacroSetKeyEquivalentsFunctionKeys
												: kTelnetEnumeratedClassMacroSetKeyEquivalentsCommandDigit;
					
					
					error = RecordAE_CreateRecordableAppleEvent(kAECoreSuite, kAESetData, &setDataEvent);
					if (error == noErr)
					{
						AEDesc		containerDesc; // never changes, in this case
						AEDesc		keyDesc;
						AEDesc		objectDesc;
						AEDesc		propertyDesc;
						AEDesc		valueDesc;
						
						
						(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc);
						(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
						(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
						(OSStatus)AppleEventUtilities_InitAEDesc(&propertyDesc);
						(OSStatus)AppleEventUtilities_InitAEDesc(&valueDesc);
						
						// send a request for "macro set <index>", which resides in a null container;
						// then, request the "key equivalents" property, and finally, issue an event
						// that will change its value
						(OSStatus)BasicTypesAE_CreateSInt32Desc(Macros_ReturnActiveSetNumber(), &keyDesc);
						error = CreateObjSpecifier(cMyMacroSet, &containerDesc,
													formAbsolutePosition, &keyDesc, true/* dispose inputs */,
													&objectDesc);
						if (error != noErr) Console_WriteValue("macro set access error", error);
						(OSStatus)BasicTypesAE_CreatePropertyIDDesc(pMyMacroSetKeyEquivalents, &keyDesc);
						error = CreateObjSpecifier(cProperty, &objectDesc,
													formPropertyID, &keyDesc, true/* dispose inputs */,
													&propertyDesc);
						if (error != noErr) Console_WriteValue("property access error", error);
						if (error == noErr)
						{
							// reference to the property to be set - REQUIRED
							(OSStatus)AEPutParamDesc(&setDataEvent, keyDirectObject, &propertyDesc);
							
							// the property’s new value - REQUIRED
							(OSStatus)BasicTypesAE_CreateEnumerationDesc(newKeys, &valueDesc);
							(OSStatus)AEPutParamDesc(&setDataEvent, keyAEParamMyTo, &valueDesc);
							
							// send the event, which will record it into any open script and actually change the value
							(OSStatus)AESend(&setDataEvent, &reply, kAENoReply | kAENeverInteract | kAEDontExecute,
												kAENormalPriority, kAEDefaultTimeout, nullptr/* idle routine */,
												nullptr/* filter routine */);
						}
						AEDisposeDesc(&propertyDesc);
						AEDisposeDesc(&valueDesc);
					}
					AEDisposeDesc(&setDataEvent);
				}
			}
			break;
		
		case kCommandTerminalViewPageUp:
			TerminalView_ScrollRowsTowardBottomEdge(activeView, Terminal_ReturnRowCount(activeScreen));
			break;
		
		case kCommandTerminalViewPageDown:
			TerminalView_ScrollRowsTowardTopEdge(activeView, Terminal_ReturnRowCount(activeScreen));
			break;
		
		case kCommandTerminalViewHome:
			TerminalView_ScrollToBeginning(activeView);
			break;
		
		case kCommandTerminalViewEnd:
			TerminalView_ScrollToEnd(activeView);
			break;
		
		default:
			result = false;
			break;
		}
	}
	return result;
}// ExecuteByID


/*!
Like Commands_ExecuteByID(), except it constructs a
Carbon Event and sends it to the most local event
target that is defined.  The first target attempted
is the user focus; barring that, the active window;
and barring that, the application itself.

Returns true only if the event is handled successfully.

Unlike Commands_ExecuteByID(), this request should
propagate to an appropriate handler no matter where
the handler is installed.

IMPORTANT:	If Commands_StartHandlingExecution() was
			used to register a handler, this function
			will not respect it.  That method of
			handling commands is deprecated, but
			Commands_ExecuteByID() will still invoke
			such handlers.

(3.1)
*/
Boolean
Commands_ExecuteByIDUsingEvent	(UInt32		inCommandID)
{
	EventRef	executeEvent = nullptr;
	OSStatus	error = noErr;
	Boolean		result = false;
	
	
	error = CreateEvent(kCFAllocatorDefault, kEventClassCommand, kEventCommandProcess,
						0/* time of event */, kEventAttributeNone, &executeEvent);
	if (noErr == error)
	{
		HICommand		commandInfo;
		EventTargetRef	whereToStart = GetUserFocusEventTarget();
		
		
		if (nullptr == whereToStart) whereToStart = GetWindowEventTarget(EventLoop_ReturnRealFrontWindow());
		if (nullptr == whereToStart) whereToStart = GetApplicationEventTarget();
		assert(nullptr != whereToStart);
		
		bzero(&commandInfo, sizeof(commandInfo));
		commandInfo.commandID = inCommandID;
		error = SetEventParameter(executeEvent, kEventParamDirectObject, typeHICommand,
									sizeof(commandInfo), &commandInfo);
		if (noErr == error)
		{
			error = SendEventToEventTarget(executeEvent, whereToStart);
			if (noErr == error)
			{
				// success!
				result = true;
			}
		}
	}
	
	return result;
}// ExecuteByIDUsingEvent


/*!
Handles "kEventToolbarCreateItemWithIdentifier" from
"kEventClassToolbar" for any toolbar.  Responds by
creating the requested items.

This routine has special knowledge of certain commands
that have associated icons.  It creates those items
and automatically associates the proper command ID
with them so that the normal command handler will take
care of any clicks on them.

Currently, the context is unused and should be nullptr.

(3.1)
*/
pascal OSStatus
Commands_HandleCreateToolbarItem	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef				inEvent,
									 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassToolbar);
	{
		HIToolbarRef	toolbarRef = nullptr;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbar, typeHIToolbarRef, toolbarRef);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventToolbarCreateItemWithIdentifier:
				{
					CFStringRef		identifierCFString = nullptr;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbarItemIdentifier,
																	typeCFStringRef, identifierCFString);
					if (noErr == result)
					{
						CFTypeRef	itemData = nullptr;
						
						
						// NOTE: configuration data is not always present
						result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbarItemConfigData,
																		typeCFTypeRef, itemData);
						if (noErr != result)
						{
							itemData = nullptr;
							result = noErr;
						}
						
						// create the specified item, if its identifier is recognized
						{
							HIToolbarItemRef	itemRef = nullptr;
							
							
							if (kCFCompareEqualTo == CFStringCompare
														(kConstantsRegistry_HIToolbarItemIDNewShellSession,
															identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandNewSessionShell;
									IconRef			iconRef = nullptr;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = GetIconRef(kOnSystemDisk, kSystemIconsCreator,
														kInternetLocationGenericIcon, &iconRef);
									if (noErr == result)
									{
										result = HIToolbarItemSetIconRef(itemRef, iconRef);
										assert_noerr(result);
										(OSStatus)ReleaseIconRef(iconRef), iconRef = nullptr;
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare
															(kConstantsRegistry_HIToolbarItemIDNewDefaultSession,
																identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandNewSessionDefaultFavorite;
									CFStringRef		nameCFString = nullptr;
									FSRef			iconFile;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									if (AppResources_GetArbitraryResourceFileFSRef
										(AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
											CFSTR("icns")/* type */, iconFile))
									{
										IconRef		iconRef = nullptr;
										
										
										if (noErr == RegisterIconRefFromFSRef
														(AppResources_ReturnCreatorCode(),
															kConstantsRegistry_IconServicesIconPrefPanelSessions,
															&iconFile, &iconRef))
										{
											result = HIToolbarItemSetIconRef(itemRef, iconRef);
											assert_noerr(result);
											(OSStatus)ReleaseIconRef(iconRef), iconRef = nullptr;
										}
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare
															(kConstantsRegistry_HIToolbarItemIDNewLoginSession,
																identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandNewSessionLoginShell;
									IconRef			baseIconRef = nullptr;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = GetIconRef(kOnSystemDisk, kSystemIconsCreator,
														kInternetLocationGenericIcon, &baseIconRef);
									if (noErr == result)
									{
										IconRef		badgeIconRef = nullptr;
										
										
										if (noErr == GetIconRef(kOnSystemDisk, kSystemIconsCreator,
																kLockedBadgeIcon, &badgeIconRef))
										{
											IconRef		finalIconRef = nullptr;
											
											
											if (noErr == CompositeIconRef(baseIconRef, badgeIconRef, &finalIconRef))
											{
												result = HIToolbarItemSetIconRef(itemRef, finalIconRef);
												assert_noerr(result);
												(OSStatus)ReleaseIconRef(finalIconRef), finalIconRef = nullptr;
											}
											(OSStatus)ReleaseIconRef(badgeIconRef), badgeIconRef = nullptr;
										}
										(OSStatus)ReleaseIconRef(baseIconRef), baseIconRef = nullptr;
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare
															(kConstantsRegistry_HIToolbarItemIDStackWindows,
																identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandStackWindows;
									CFStringRef		nameCFString = nullptr;
									FSRef			iconFile;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									if (AppResources_GetArbitraryResourceFileFSRef
										(AppResources_ReturnStackWindowsIconFilenameNoExtension(),
											CFSTR("icns")/* type */, iconFile))
									{
										IconRef		iconRef = nullptr;
										
										
										if (noErr == RegisterIconRefFromFSRef
														(AppResources_ReturnCreatorCode(),
															kConstantsRegistry_IconServicesIconToolbarItemStackWindows,
															&iconFile, &iconRef))
										{
											result = HIToolbarItemSetIconRef(itemRef, iconRef);
											assert_noerr(result);
											(OSStatus)ReleaseIconRef(iconRef), iconRef = nullptr;
										}
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							
							if (nullptr == itemRef)
							{
								result = eventNotHandledErr;
							}
							else
							{
								result = SetEventParameter(inEvent, kEventParamToolbarItem, typeHIToolbarItemRef,
															sizeof(itemRef), &itemRef);
							}
						}
					}
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// HandleCreateToolbarItem


/*!
Arranges for a callback to be invoked in order to
execute the given command.

The specified callback is expected to return an
OSStatus value - a return value of "eventNotHandledErr"
is the ONLY way to indicate that an execution handler
did nothing in response to a particular command, which
permits the Commands module to seek another handler
for the same command.  Returning any other value will
cause your handler to be the last attempt made to
execute the given command.

The context passed to the listener is of type
"Commands_ExecutionEventContextPtr".

\retval kCommands_ResultOK
if no error occurred

\retval kCommands_ResultParameterError
if an OSStatus listener was not provided or otherwise
the listener could not be registered

(3.0)
*/
Commands_Result
Commands_StartHandlingExecution		(UInt32						inImplementedCommand,
									 ListenerModel_ListenerRef	inCommandImplementor)
{
	Commands_Result		result = kCommands_ResultOK;
	OSStatus			error = noErr;
	
	
	error = ListenerModel_AddListenerForEvent(gCommandExecutionListenerModel(), inImplementedCommand, inCommandImplementor);
	if (error != noErr) result = kCommands_ResultParameterError;
	
	return result;
}// StartHandlingExecution


/*!
Arranges for a callback to no longer be invoked when
the specified command needs to be executed.

The given listener should match one installed
previously with Commands_StartHandlingExecution();
otherwise this routine silently fails.

\retval kCommands_ResultOK
always

(3.0)
*/
Commands_Result
Commands_StopHandlingExecution	(UInt32						inImplementedCommand,
								 ListenerModel_ListenerRef	inCommandImplementor)
{
	Commands_Result		result = kCommands_ResultOK;
	
	
	ListenerModel_RemoveListenerForEvent(gCommandExecutionListenerModel(), inImplementedCommand, inCommandImplementor);
	return result;
}// StopHandlingExecution


#pragma mark Internal Methods
namespace {

/*!
Chooses another window in the window list that is adjacent
to the frontmost window.  If "inPreviousInsteadOfNext" is
true, then the previous window is activated instead of the
next one in the list.  If "inHidePreviousWindow" is true
and the frontmost window is a terminal, it is hidden (with
the appropriate visual effects) before the next window is
activated.

(3.0)
*/
void
activateAnotherWindow	(Boolean	inPreviousInsteadOfNext,
						 Boolean	inHidePreviousWindow)
{
	HIWindowRef		frontWindow = EventLoop_ReturnRealFrontWindow();
	
	
	if (nullptr != frontWindow)
	{
		HIWindowRef		nextWindow = nullptr;
		SessionRef		frontSession = SessionFactory_ReturnUserFocusSession();
		
		
		if (nullptr == frontSession)
		{
			// not a session window; so, select the first or last session window
			SessionRef				nextSession = nullptr;
			SessionFactory_Result	factoryError = SessionFactory_GetSessionWithZeroBasedIndex
													((inPreviousInsteadOfNext) ? SessionFactory_ReturnCount() - 1 : 0,
														kSessionFactory_ListInCreationOrder, &nextSession);
			
			
			if (kSessionFactory_ResultOK == factoryError)
			{
				nextWindow = Session_ReturnActiveWindow(nextSession);
			}
		}
		else
		{
			// is a session window; so, select the next or previous session window
			SessionRef				nextSession = nullptr;
			UInt16					frontSessionIndex = 0;
			SessionFactory_Result	factoryError = kSessionFactory_ResultOK;
			
			
			factoryError = SessionFactory_GetZeroBasedIndexOfSession
							(frontSession, kSessionFactory_ListInCreationOrder, &frontSessionIndex);
			if (kSessionFactory_ResultOK == factoryError)
			{
				// adjust the session index to determine the new session;
				// if modified past the bounds, wrap around
				if (inPreviousInsteadOfNext)
				{
					if (frontSessionIndex == 0) frontSessionIndex = SessionFactory_ReturnCount() - 1;
					else --frontSessionIndex;
				}
				else
				{
					if (++frontSessionIndex >= SessionFactory_ReturnCount()) frontSessionIndex = 0;
				}
				
				factoryError = SessionFactory_GetSessionWithZeroBasedIndex
								(frontSessionIndex, kSessionFactory_ListInCreationOrder, &nextSession);
				if (kSessionFactory_ResultOK == factoryError)
				{
					nextWindow = Session_ReturnActiveWindow(nextSession);
				}
			}
		}
		
		if (nullptr == nextWindow)
		{
			// cannot figure out which window to activate
			Sound_StandardAlert();
		}
		else
		{
			unless (IsWindowVisible(nextWindow))
			{
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(nextWindow);
				
				
				// display the window and ensure it is not marked as hidden (in case it was)
				TerminalWindow_SetObscured(terminalWindow, false);
			}
			EventLoop_SelectBehindDialogWindows(nextWindow);
			if (inHidePreviousWindow)
			{
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(frontWindow);
				
				
				if (nullptr != terminalWindow) TerminalWindow_SetObscured(terminalWindow, true);
			}
		}
	}
}// activateAnotherWindow


/*!
Notifies listeners that the specified command needs
executing, until a listener returns a result other
than "eventNotHandledErr".  The listeners of this
event are actually expected to perform an operation
in response to the command.

(3.0)
*/
void
changeNotifyForCommandExecution		(UInt32		inCommand)
{
	Commands_ExecutionEventContext		context;
	
	
	context.commandID = inCommand;
	
	// invoke listener callback routines appropriately, from the specified command’s implementation listener model
	ListenerModel_NotifyListenersOfEvent(gCommandExecutionListenerModel(), inCommand, &context);
}// changeNotifyForCommandExecution


/*!
Returns "true" only if at least one listener
would be invoked when the specified command
needs executing.

(3.0)
*/
Boolean
isAnyListenerForCommandExecution	(UInt32		inCommand)
{
	return ListenerModel_IsAnyListenerForEvent(gCommandExecutionListenerModel(), inCommand);
}// isAnyListenerForCommandExecution


/*!
This routine, of "SessionFactory_TerminalWindowOpProcPtr"
form, redisplays the specified, obscured terminal window.

(3.0)
*/
void
showWindowTerminalWindowOp		(TerminalWindowRef	inTerminalWindow,
								 void*				UNUSED_ARGUMENT(inData1),
								 SInt32				UNUSED_ARGUMENT(inData2),
								 void*				UNUSED_ARGUMENT(inoutResultPtr))
{
	TerminalWindow_SetObscured(inTerminalWindow, false);
}// showWindowTerminalWindowOp


/*!
A method of standard ScreenLineOperationProcPtr form,
this routine uses the Text Translation module to
convert the text on the specified line of a terminal
screen from one encoding to another.

Since this operation modifies the buffer, "true" is
always returned.
*/
Boolean
translateScreenLine		(TerminalScreenRef	inFromWhichScreen,
						 char*				inoutLineTextBuffer,
						 UInt16				inLineTextBufferLength,
						 UInt16				UNUSED_ARGUMENT(inOneBasedLineNumber),
						 void*				inContextPtr)
{
	//ScreenTranslationInfoPtr	translationInfoPtr = REINTERPRET_CAST(inContextPtr, ScreenTranslationInfoPtr);
	Boolean						result = true;
	
	
	#if 1
	//TextTranslation_ConvertTextToMac((UInt8*)inoutLineTextBuffer, inLineTextBufferLength,
	//									translationInfoPtr->oldTable);
	//TextTranslation_ConvertTextFromMac((UInt8*)inoutLineTextBuffer, inLineTextBufferLength,
	//									translationInfoPtr->newTable);
	#else
	// temp - test Text Encoding Converter stuff (doesn’t really work yet)
	{
		// translate the line’s text into a new buffer, then copy it overtop
		{
			Handle		h = nullptr;
			OSStatus	error = noErr;
			
			
			error = TextTranslation_ConvertBufferToNewHandle((UInt8*)inoutLineTextBuffer, inLineTextBufferLength,
																realScreenPtr->text.encodingConverter, &h);
			if (error == noErr)
			{
				// currently, MacTelnet just truncates the text if its translated equivalent is too long
				// (since the maximum line buffer size is many, many bytes, this is VERY unlikely to occur)
				BlockMoveData(*h, (Ptr)inoutLineTextBuffer, INTEGER_MINIMUM(GetHandleSize(h), inLineTextBufferLength));
			}
			else Console_WriteValue("translation error", error);
			Memory_DisposeHandle(&h);
		}
		else Console_WriteLine("could not find screen");
	}
	#endif
	
	return result;
}// translateScreenLine

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
