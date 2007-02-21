/*###############################################################

	SessionEditorDialog.cp
	
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
#include <cctype>
#include <climits>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFUtilities.h>
#include <Console.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "DialogResources.h"
#include "GeneralResources.h"
#include "MenuResources.h"
#include "SessionEditorDialog.r"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Clipboard.h"
#include "ConstantsRegistry.h"
#include "DialogTransitions.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "FileUtilities.h"
#include "Folder.h"
#include "HelpSystem.h"
#include "Preferences.h"
#include "SessionEditorDialog.h"
#include "tekdefs.h"
#include "Terminal.h"
#include "TextTranslation.h"
#include "UIStrings.h"



#pragma mark Constants

enum
{
	// dialog item indices
	SessOK = 1,
	SessCancel = 2,
	SessTEKinhib = 3,
	SessTEK4014 = 4,
	SessTEK4105 = 5,
	SessPasteQuick = 6,
	SessPasteBlock = 7,
	SessDeleteDel = 8,
	SessDeleteBS = 9,
	SessForceSave = 10,
	SessBezerkeley = 11,
	SessLinemode = 12,
	SessTEKclear = 13,
	SessHalfDuplex = 14,
	SessAuthenticate = 15,
	SessEncrypt = 16,
	SessLocalEcho = 17,
	SessAlias = 18,
	SessHostName = 19,
	SessPort = 20,
	SessPasteBlockSize = 21,
	SessInterrupt = 22,
	SessSuspend = 23,
	SessResume = 24,
	SessTermPopup = 25,
	SessTransTablePopup = 26,
	SessSafeItem = 27, // this is actually the Host label
	SessAliasStatText = 28,
	SessNetworkBlockSize = 39,
	SessAutoCaptureFilePath = 40,
	SessAutoCaptureSetFile = 41,
	SessAutoCaptureToFile = 42,
	SessHelpButton = 43
};

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	DialogRef			gSessionEditorDialog = nullptr; // the Mac OS dialog pointer
	WindowInfoRef		gSessionEditorDialogWindowInfo = nullptr;
}

//
// internal methods
//

static void				done							();

static void				init							();

static pascal Boolean	sessionEditorDialogEventFilter	(DialogRef						inDialog,
														 EventRecord*					inoutEventPtr,
														 short*							outItemIndex);

static void				setItemStates					();



//
// public methods
//

/*!
This method displays and handles events in the
Session Favorite Editor dialog box.  If the
user clicks OK, "true" is returned; otherwise,
"false" is returned.

(3.0)
*/
Boolean
SessionEditorDialog_Display		(Preferences_Class		inClass,
								 CFStringRef			inContextName,
								 Rect const*			inTransitionOpenRectOrNull)
{
	Boolean					result = false;
	Preferences_ContextRef	preferencesContext = nullptr;
	
	
	preferencesContext = Preferences_NewContext(inClass, inContextName);
	if (preferencesContext == nullptr) result = false;
	else
	{
		enum
		{
			kNumberOfNonEncodingItems = 0/*1*/		// the “None” item preceding all text encodings in the menu
		};
		DialogItemIndex			selectedItemIndex = 0;
		SInt16					scratchshort = 0;
		SInt32					scratchlong = 0L;
		Boolean					wasInAliasText = false;
		Boolean					flag = false;
		FSSpec					pendingCaptureFile;
		Preferences_AliasID		pendingCaptureFileAliasID = kPreferences_InvalidAliasID;
		Str255					scratchPstring;
		MenuHandle				characterSetMenu = nullptr;
		MenuHandle				terminalsMenu = nullptr;
		WindowRef				parentWindow = nullptr;
		ModalFilterUPP			filterUPP = nullptr;
		Preferences_ResultCode	preferencesResult = kPreferences_ResultCodeSuccess;
		
		
		init();
		
		selectedItemIndex = 3;
		
		// set up Terminals pop-up menu
		{
			CFArrayRef		terminalNameCFStringCFArray = nullptr;
			
			
			preferencesResult = Preferences_CreateContextNameArray(kPreferences_ClassTerminal,
																	terminalNameCFStringCFArray);
			if (preferencesResult == kPreferences_ResultCodeSuccess)
			{
				ControlRef		popUpMenuControl = nullptr;
				CFIndex			count = 0;
				CFIndex			i = 0;
				
				
				(OSStatus)GetDialogItemAsControl(gSessionEditorDialog, SessTermPopup, &popUpMenuControl);
				(OSStatus)CreateNewMenu(kMenuIDSessionEditorTerminals, 0/* menu attributes */, &terminalsMenu);
				SetMenuTitleWithCFString(terminalsMenu, CFSTR(""));
				count = CFArrayGetCount(terminalNameCFStringCFArray);
				for (i = 0; i < count; ++i)
				{
					(OSStatus)AppendMenuItemTextWithCFString
								(terminalsMenu, CFUtilities_StringCast
												(CFArrayGetValueAtIndex(terminalNameCFStringCFArray, i)),
									0L/* attributes */, 0L/* command ID */, nullptr/* new item */);
				}
				EnableMenuItem(terminalsMenu, 0);
				SetControl32BitMinimum(popUpMenuControl, 1);
				SetControl32BitMaximum(popUpMenuControl, count);
				(OSStatus)SetControlData(popUpMenuControl, kControlMenuPart, kControlPopupButtonMenuHandleTag,
											sizeof(terminalsMenu), &terminalsMenu);
				
				CFRelease(terminalNameCFStringCFArray), terminalNameCFStringCFArray = nullptr;
			}
		}
		
		// set up Character Set pop-up menu
		{
			ControlRef		popUpMenuControl = nullptr;
			SInt16			count = 0;
			
			
			(OSStatus)GetDialogItemAsControl(gSessionEditorDialog, SessTransTablePopup, &popUpMenuControl);
			(OSStatus)CreateNewMenu(kMenuIDSessionEditorTranslationTables, 0/* menu attributes */, &characterSetMenu);
			SetMenuTitleWithCFString(characterSetMenu, CFSTR(""));
			//GetIndString(scratchPstring, rStringsMiscellaneous, siNone); // "None" string
			//AppendMenu(characterSetMenu, "\p<none>"); // avoid Menu Manager interpolation
			//SetMenuItemText(characterSetMenu, CountMenuItems(characterSetMenu), scratchPstring);
			TextTranslation_AppendCharacterSetsToMenu(characterSetMenu, 0/* levels of indentation */);
			count = CountMenuItems(characterSetMenu);
			EnableMenuItem(characterSetMenu, 0);
			SetControl32BitMinimum(popUpMenuControl, 1);
			SetControl32BitMaximum(popUpMenuControl, count);
			(OSStatus)SetControlData(popUpMenuControl, kControlMenuPart, kControlPopupButtonMenuHandleTag,
										sizeof(characterSetMenu), &characterSetMenu);
		}
		
		AppResources_UseResFile(kAppResources_FileIDPreferences);
		
		
		{
			TektronixMode	tekMode = kTektronixModeNotAllowed;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTektronixMode,
															sizeof(tekMode), &tekMode);
			if (preferencesResult != kPreferences_ResultCodeSuccess) tekMode = kTektronixModeNotAllowed;
			switch (tekMode)
			{
			case kTektronixMode4105:
				SetDialogItemValue(gSessionEditorDialog, SessTEKinhib, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4014, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4105, kControlRadioButtonCheckedValue);
				break;
			
			case kTektronixMode4014:
				SetDialogItemValue(gSessionEditorDialog, SessTEKinhib, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4014, kControlRadioButtonCheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4105, kControlRadioButtonUncheckedValue);
				break;
			
			case kTektronixModeNotAllowed:
			default:
				SetDialogItemValue(gSessionEditorDialog, SessTEKinhib, kControlRadioButtonCheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4014, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4105, kControlRadioButtonUncheckedValue);
				break;
			}
		}
		{
			Clipboard_PasteMethod	pasteMethod = kClipboard_PasteMethodStandard;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTektronixMode,
															sizeof(pasteMethod), &pasteMethod);
			if (preferencesResult != kPreferences_ResultCodeSuccess) pasteMethod = kClipboard_PasteMethodStandard;
			switch (pasteMethod)
			{
			case kClipboard_PasteMethodBlock:
				SetDialogItemValue(gSessionEditorDialog, SessPasteQuick, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessPasteBlock, kControlRadioButtonCheckedValue);
				break;
			
			case kClipboard_PasteMethodStandard:
			default:
				SetDialogItemValue(gSessionEditorDialog, SessPasteQuick, kControlRadioButtonCheckedValue);
				SetDialogItemValue(gSessionEditorDialog, SessPasteBlock, kControlRadioButtonUncheckedValue);
				break;
			}
		}
		{
			SInt16		pasteBlockSize = 0;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagPasteBlockSize,
															sizeof(pasteBlockSize), &pasteBlockSize);
			if (preferencesResult != kPreferences_ResultCodeSuccess) pasteBlockSize = 128; // arbitrary
			SetDialogItemNumericalText(gSessionEditorDialog, SessPasteBlockSize, pasteBlockSize);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagMapDeleteToBackspace,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultCodeSuccess) flag = false;
			SetDialogItemValue(gSessionEditorDialog, SessDeleteDel, (flag) ? kControlRadioButtonUncheckedValue
																			: kControlRadioButtonCheckedValue);
			SetDialogItemValue(gSessionEditorDialog, SessDeleteBS, (flag) ? kControlRadioButtonCheckedValue
																			: kControlRadioButtonUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagMapCarriageReturnToCRNull,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultCodeSuccess) flag = false;
			SetDialogItemValue(gSessionEditorDialog, SessBezerkeley, (flag) ? kControlCheckBoxCheckedValue
																			: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagLineModeEnabled,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultCodeSuccess) flag = false;
			SetDialogItemValue(gSessionEditorDialog, SessLinemode, (flag) ? kControlCheckBoxCheckedValue
																			: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTektronixPAGEClearsScreen,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultCodeSuccess) flag = false;
			SetDialogItemValue(gSessionEditorDialog, SessTEKclear, (flag) ? kControlCheckBoxCheckedValue
																			: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagLocalEchoEnabled,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultCodeSuccess) flag = false;
			SetDialogItemValue(gSessionEditorDialog, SessLocalEcho, (flag) ? kControlCheckBoxCheckedValue
																			: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagLocalEchoHalfDuplex,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultCodeSuccess) flag = false;
			SetDialogItemValue(gSessionEditorDialog, SessHalfDuplex, (flag) ? kControlCheckBoxCheckedValue
																			: kControlCheckBoxUncheckedValue);
		}
		{
			CFStringRef		hostCFString = nullptr;
			ControlRef		textControl = nullptr;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagServerHost,
															sizeof(hostCFString), &hostCFString);
			if (preferencesResult != kPreferences_ResultCodeSuccess) hostCFString = CFSTR(""); // arbitrary
			if (noErr == GetDialogItemAsControl(gSessionEditorDialog, SessHostName, &textControl))
			{
				SetControlTextWithCFString(textControl, hostCFString);
			}
		}
		{
			ControlRef		textControl = nullptr;
			
			
			if (noErr == GetDialogItemAsControl(gSessionEditorDialog, SessAlias, &textControl))
			{
				SetControlTextWithCFString(textControl, inContextName);
			}
		}
		{
			SInt16		portNumber = 0;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagServerPort,
															sizeof(portNumber), &portNumber);
			if (preferencesResult != kPreferences_ResultCodeSuccess) portNumber = 23; // arbitrary
			SetDialogItemNumericalText(gSessionEditorDialog, SessPort, portNumber);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagAutoCaptureToFile,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultCodeSuccess) flag = false;
			SetDialogItemValue(gSessionEditorDialog, SessAutoCaptureToFile, (flag) ? kControlCheckBoxCheckedValue
																					: kControlCheckBoxUncheckedValue);
		}
		if (GetDialogItemValue(gSessionEditorDialog, SessAutoCaptureToFile) == kControlCheckBoxCheckedValue)
		{
			ActivateDialogItem(gSessionEditorDialog, SessAutoCaptureSetFile);
			ActivateDialogItem(gSessionEditorDialog, SessAutoCaptureFilePath);
		}
		else
		{
			DeactivateDialogItem(gSessionEditorDialog, SessAutoCaptureSetFile);
			DeactivateDialogItem(gSessionEditorDialog, SessAutoCaptureFilePath);
		}
		{
			// set up control keys
			char	controlKey = '\0';
			
			
			scratchPstring[0] = 2;
			scratchPstring[1] = '^';
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagKeyInterruptProcess,
															sizeof(controlKey), &controlKey);
			if (preferencesResult != kPreferences_ResultCodeSuccess) controlKey = '\0'; // arbitrary
			scratchPstring[2] = controlKey ^ 64; // remember, '^' means XOR!
			SetDialogItemControlText(gSessionEditorDialog, SessInterrupt, scratchPstring);
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagKeyResumeOutput,
															sizeof(controlKey), &controlKey);
			if (preferencesResult != kPreferences_ResultCodeSuccess) controlKey = '\0'; // arbitrary
			scratchPstring[2] = controlKey ^ 64; // remember, '^' means XOR!
			SetDialogItemControlText(gSessionEditorDialog, SessResume, scratchPstring);
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagKeySuspendOutput,
															sizeof(controlKey), &controlKey);
			if (preferencesResult != kPreferences_ResultCodeSuccess) controlKey = '\0'; // arbitrary
			scratchPstring[2] = controlKey ^ 64; // remember, '^' means XOR!
			SetDialogItemControlText(gSessionEditorDialog, SessSuspend, scratchPstring);
		}
		{
			// set up terminals menu
			CFStringRef		associatedTerminalName = nullptr;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagAssociatedTerminalFavorite,
															sizeof(associatedTerminalName), &associatedTerminalName);
			if (preferencesResult != kPreferences_ResultCodeSuccess) associatedTerminalName = CFSTR(""); // arbitrary
			
			for (scratchshort = CountMenuItems(terminalsMenu); scratchshort; --scratchshort)
			{
				CFStringRef		itemTextCFString = nullptr;
				
				
				if (noErr == CopyMenuItemTextAsCFString(terminalsMenu, scratchshort, &itemTextCFString))
				{
					if (kCFCompareEqualTo == CFStringCompare(itemTextCFString, associatedTerminalName, 0/* flags */))
					{
						SetDialogItemValue(gSessionEditorDialog, SessTermPopup, scratchshort);
					}
					CFRelease(itemTextCFString), itemTextCFString = nullptr;
				}
			}
		}
		{
			TextEncoding	textEncoding = kTextEncodingMacRoman;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTextEncoding,
															sizeof(textEncoding), &textEncoding);
			if (preferencesResult != kPreferences_ResultCodeSuccess) textEncoding = kTextEncodingMacRoman; // arbitrary
			// 3.0 - use Text Encoding Converters in the menu
			SetDialogItemValue(gSessionEditorDialog, SessTransTablePopup,
								TextTranslation_GetCharacterSetIndex(textEncoding) + kNumberOfNonEncodingItems);
		}
		
		// initialize this structure, in case FSMakeFSSpec() fails
		PLstrcpy(pendingCaptureFile.name, EMPTY_PSTRING);
		pendingCaptureFile.vRefNum = -1;
		pendingCaptureFile.parID = fsRtDirID;
		
		// the “capture file” alias record is stored as a separate resource - find it or create it
		if (GetDialogItemValue(gSessionEditorDialog, SessAutoCaptureToFile) == kControlCheckBoxCheckedValue)
		{
			Preferences_AliasID		aliasID = kPreferences_InvalidAliasID;
			Str255					string;
			OSStatus				error = noErr;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagCaptureFileAlias,
															sizeof(aliasID), &aliasID);
			if (kPreferences_ResultCodeSuccess != preferencesResult)
			{
				// set to empty (or error message?)
				SetDialogItemControlText(gSessionEditorDialog, SessAutoCaptureFilePath, EMPTY_PSTRING);
			}
			else
			{
				// 3.0 - read an alias record for the default directory
				if (Preferences_AliasIsStored(aliasID))
				{
					// a capture file preference exists - read it and resolve it (re-saving changes if necessary)
					pendingCaptureFileAliasID = Preferences_NewSavedAlias(aliasID);
					(Boolean)Preferences_AliasParse(pendingCaptureFileAliasID, &pendingCaptureFile);
				}
				else
				{
					// no capture file preference exists - create a new one with defaults (on the
					// desktop, if possible; otherwise, create it at the root of the startup disk)
					error = Folder_GetFSSpec(kFolder_RefMacDesktop, &pendingCaptureFile);
					if (error == noErr)
					{
						error = UIStrings_MakeFSSpec(pendingCaptureFile.vRefNum, pendingCaptureFile.parID,
														kUIStrings_FileDefaultCaptureFile,
														&pendingCaptureFile);
					}
					if ((error != noErr) && (error != fnfErr))
					{
						error = UIStrings_MakeFSSpec(-1, fsRtDirID,
														kUIStrings_FileDefaultCaptureFile,
														&pendingCaptureFile);
					}
					
					if (error == fnfErr)
					{
						OSType		captureFileCreator;
						size_t		actualSize = 0L;
						
						
						// get the user’s Capture File Creator preference, if possible
						unless (Preferences_GetData(kPreferences_TagCaptureFileCreator,
													sizeof(captureFileCreator), &captureFileCreator,
													&actualSize) == kPreferences_ResultCodeSuccess)
						{
							captureFileCreator = 'ttxt'; // default to SimpleText if a preference can’t be found
						}
						error = FSpCreate(&pendingCaptureFile, captureFileCreator, 'TEXT',
											GetScriptManagerVariable(smSysScript));
					}
					if (error == noErr)
					{
						// create an 'alis' resource in the preferences file, and remember its ID
						pendingCaptureFileAliasID = Preferences_NewAlias(&pendingCaptureFile);
					}
					else
					{
						pendingCaptureFileAliasID = kPreferences_InvalidAliasID;
						Console_WriteValue("capture file preference creation error", error);
					}
				}
				if (kPreferences_InvalidAliasID == pendingCaptureFileAliasID)
				{
					Sound_StandardAlert(); // TEMPORARY - this is not error handling
					Console_WriteLine("capture file alias could not be created");
				}
				FileUtilities_GetPathnameFromFSSpec(&pendingCaptureFile, string, false/* is directory */);
				
				// update the text area to show the capture file pathname
				SetDialogItemCenterTruncatedText(gSessionEditorDialog, SessAutoCaptureFilePath, string,
												kThemeSmallSystemFont);
			}
		}
		
		// hide ignored settings for now
		HideDialogItem(gSessionEditorDialog, SessForceSave);
		HideDialogItem(gSessionEditorDialog, SessAuthenticate);
		HideDialogItem(gSessionEditorDialog, SessEncrypt);
		
		setItemStates(); // 3.0
		
		SelectDialogItemText(gSessionEditorDialog, SessAlias, 0, 32767);
		
		wasInAliasText = false;
		
		parentWindow = EventLoop_GetRealFrontWindow();
		DialogTransitions_DisplayFromRectangle(GetDialogWindow(gSessionEditorDialog), parentWindow, inTransitionOpenRectOrNull);
		
		// set the focus
		FocusDialogItem(gSessionEditorDialog, SessHostName);
		
		filterUPP = NewModalFilterUPP(sessionEditorDialogEventFilter);
		do
		{
			ModalDialog(filterUPP, &selectedItemIndex);
			switch (selectedItemIndex)
			{
			case SessHelpButton:
				HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseFavorites);
				break;
			
			case SessAutoCaptureToFile:
			case SessAutoCaptureSetFile:
				{
					Boolean		choose = false;
					
					
					if (SessAutoCaptureToFile == selectedItemIndex)
					{
						// then the checkbox was selected - change its state
						FlipCheckBox(gSessionEditorDialog, selectedItemIndex);
						setItemStates();
						
						// if the user turns on the auto-capture-file option,
						// automatically “click” the Set button
						choose = (GetDialogItemValue(gSessionEditorDialog, selectedItemIndex) == kControlCheckBoxCheckedValue);
						if (choose) FlashButton(gSessionEditorDialog, SessAutoCaptureSetFile);
					}
					else choose = true;
					
					if ((choose) && Terminal_FileCaptureSaveDialog(&pendingCaptureFile))
					{
						// show path to file in the dialog box
						Str255		string;
						
						
						FileUtilities_GetPathnameFromFSSpec(&pendingCaptureFile, string, false/* is directory */);
						SetDialogItemCenterTruncatedText(gSessionEditorDialog, SessAutoCaptureFilePath, string,
															kThemeSmallSystemFont);
					}
				}
				break;
			
			case SessForceSave:
			case SessBezerkeley:
			case SessLinemode:
			case SessTEKclear:
			case SessHalfDuplex:
			case SessAuthenticate:
			case SessEncrypt:
			case SessLocalEcho:
				FlipCheckBox(gSessionEditorDialog, selectedItemIndex);
				setItemStates();
				break;
			
			case SessTEKinhib:
				SetDialogItemValue(gSessionEditorDialog, SessTEKinhib, 1);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4014, 0);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4105, 0);
				break;
			
			case SessTEK4014:
				SetDialogItemValue(gSessionEditorDialog, SessTEKinhib, 0);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4014, 1);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4105, 0);
				break;
			
			case SessTEK4105:
				SetDialogItemValue(gSessionEditorDialog, SessTEKinhib, 0);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4014, 0);
				SetDialogItemValue(gSessionEditorDialog, SessTEK4105, 1);
				break;
			
			case SessPasteQuick:
				SetDialogItemValue(gSessionEditorDialog, SessPasteQuick, 1);
				SetDialogItemValue(gSessionEditorDialog, SessPasteBlock, 0);
				break;
			
			case SessPasteBlock:
				SetDialogItemValue(gSessionEditorDialog, SessPasteQuick, 0);
				SetDialogItemValue(gSessionEditorDialog, SessPasteBlock, 1);
				break;
			
			case SessDeleteDel:
				SetDialogItemValue(gSessionEditorDialog, SessDeleteDel, 1);
				SetDialogItemValue(gSessionEditorDialog, SessDeleteBS, 0);
				break;
	
			case SessDeleteBS:
				SetDialogItemValue(gSessionEditorDialog, SessDeleteDel, 0);
				SetDialogItemValue(gSessionEditorDialog, SessDeleteBS, 1);
				break;
			
			case SessInterrupt:
			case SessSuspend:
			case SessResume:
				GetDialogItemControlText(gSessionEditorDialog, selectedItemIndex, scratchPstring);
				if ((scratchPstring[1] < 32) && (scratchPstring[1] > 0))
				{
					scratchPstring[0] = 2;
					scratchPstring[2] = scratchPstring[1] ^ 64;
					scratchPstring[1] = '^';
					SetDialogItemControlText(gSessionEditorDialog, selectedItemIndex, scratchPstring);
				}
				break;
			
			case SessAlias:
				wasInAliasText = true;
				break;
			
			default:
				break;
			}
		} while ((SessOK != selectedItemIndex) && (SessCancel != selectedItemIndex));
		DisposeModalFilterUPP(filterUPP), filterUPP = nullptr;
		
		if (SessCancel == selectedItemIndex)
		{
			// discarded; do nothing
			DialogTransitions_CloseCancel(GetDialogWindow(gSessionEditorDialog));
			done();
			result = false;			// No changes should be made.
		}
		else
		{
			Boolean		saveError = false;
			
			
			//
			// saved; update preferences
			//
			
			{
				CFStringRef		associatedTerminalName = nullptr;
				
				
				if (noErr != CopyMenuItemTextAsCFString(terminalsMenu, GetDialogItemValue(gSessionEditorDialog, SessTermPopup),
														&associatedTerminalName))
				{
					saveError = true;
				}
				else
				{
					preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagAssociatedTerminalFavorite,
																	sizeof(associatedTerminalName), &associatedTerminalName);
					if (preferencesResult != kPreferences_ResultCodeSuccess)
					{
						saveError = true;
					}
					CFRelease(associatedTerminalName), associatedTerminalName = nullptr;
				}
			}
			{
				TextEncoding	savedEncoding = TextTranslation_GetIndexedCharacterSet
												(GetDialogItemValue(gSessionEditorDialog, SessTransTablePopup) -
													kNumberOfNonEncodingItems);
				
				
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTextEncoding,
																sizeof(savedEncoding), &savedEncoding);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				TektronixMode	tekMode = kTektronixModeNotAllowed;
				
				
				if (GetDialogItemValue(gSessionEditorDialog, SessTEK4105) == kControlCheckBoxCheckedValue) tekMode = kTektronixMode4105;
				else if (GetDialogItemValue(gSessionEditorDialog, SessTEK4014) == kControlCheckBoxCheckedValue) tekMode = kTektronixMode4014;
				else tekMode = kTektronixModeNotAllowed;
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTektronixMode,
																sizeof(tekMode), &tekMode);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				Clipboard_PasteMethod	pasteMethod = (kControlRadioButtonCheckedValue == GetDialogItemValue
														(gSessionEditorDialog, SessPasteQuick))
														? kClipboard_PasteMethodStandard
														: kClipboard_PasteMethodBlock;
				
				
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagPasteMethod,
																sizeof(pasteMethod), &pasteMethod);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				// create an alias record for the capture file
				Preferences_AliasChange(pendingCaptureFileAliasID, &pendingCaptureFile);
				Preferences_AliasSave(pendingCaptureFileAliasID, EMPTY_PSTRING/* name */);
				
				// now point to that alias in the preference setting
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagCaptureFileAlias,
																sizeof(pendingCaptureFileAliasID), &pendingCaptureFileAliasID);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(gSessionEditorDialog, SessAutoCaptureToFile));
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagAutoCaptureToFile,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				flag = (kControlRadioButtonCheckedValue == GetDialogItemValue(gSessionEditorDialog, SessDeleteBS));
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagMapDeleteToBackspace,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(gSessionEditorDialog, SessBezerkeley));
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagMapCarriageReturnToCRNull,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(gSessionEditorDialog, SessLinemode));
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagLineModeEnabled,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(gSessionEditorDialog, SessTEKclear));
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTektronixPAGEClearsScreen,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(gSessionEditorDialog, SessLocalEcho));
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagLocalEchoEnabled,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(gSessionEditorDialog, SessHalfDuplex));
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagLocalEchoHalfDuplex,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				SInt16		portNumber = 0;
				
				
				GetDialogItemControlText(gSessionEditorDialog, SessPort, scratchPstring);
				StringToNum(scratchPstring, &scratchlong);
				if (scratchlong > 65530) scratchlong = 65530;
				if (scratchlong < 1) scratchlong = 1;
				portNumber = STATIC_CAST(scratchlong, SInt16);
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagServerPort,
																sizeof(portNumber), &portNumber);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				SInt16		pasteBlockSize = 0;
				
				
				GetDialogItemControlText(gSessionEditorDialog, SessPort, scratchPstring);
				StringToNum(scratchPstring, &scratchlong);
				if (scratchlong > 4097) scratchlong = 4097;
				if (scratchlong < 10) scratchlong = 10;
				pasteBlockSize = STATIC_CAST(scratchlong, SInt16);
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagPasteBlockSize,
																sizeof(pasteBlockSize), &pasteBlockSize);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			{
				CFStringRef		hostNameCFString = nullptr;
				
				
				GetDialogItemControlTextAsCFString(gSessionEditorDialog, SessHostName, hostNameCFString);
				if (nullptr == hostNameCFString) saveError = true;
				else
				{
					preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagServerHost,
																	sizeof(hostNameCFString), &hostNameCFString);
					if (preferencesResult != kPreferences_ResultCodeSuccess)
					{
						saveError = true;
					}
				}
			}
			{
				char	controlKey = '\0';
				
				
				GetDialogItemControlText(gSessionEditorDialog, SessInterrupt, scratchPstring);
				if (scratchPstring[0])
				{
					controlKey = CPP_STD::toupper(scratchPstring[2]) ^ 64;
					preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagKeyInterruptProcess,
																	sizeof(controlKey), &controlKey);
					if (preferencesResult != kPreferences_ResultCodeSuccess)
					{
						saveError = true;
					}
				}
				else
				{
					saveError = true;
				}
				
				GetDialogItemControlText(gSessionEditorDialog, SessResume, scratchPstring);
				if (scratchPstring[0])
				{
					controlKey = CPP_STD::toupper(scratchPstring[2]) ^ 64;
					preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagKeyResumeOutput,
																	sizeof(controlKey), &controlKey);
					if (preferencesResult != kPreferences_ResultCodeSuccess)
					{
						saveError = true;
					}
				}
				else
				{
					saveError = true;
				}
				
				GetDialogItemControlText(gSessionEditorDialog, SessSuspend, scratchPstring);
				if (scratchPstring[0])
				{
					controlKey = CPP_STD::toupper(scratchPstring[2]) ^ 64;
					preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagKeySuspendOutput,
																	sizeof(controlKey), &controlKey);
					if (preferencesResult != kPreferences_ResultCodeSuccess)
					{
						saveError = true;
					}
				}
				else
				{
					saveError = true;
				}
			}
			{
				SInt16		blockSize = 0;
				
				
				GetDialogItemControlText(gSessionEditorDialog, SessNetworkBlockSize, scratchPstring);
				StringToNum(scratchPstring, &scratchlong);
				if (scratchlong > 4096) scratchlong = 4096;
				if (scratchlong < 512) scratchlong = 512;
				blockSize = STATIC_CAST(scratchlong, SInt16);
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagDataReadBufferSize,
																sizeof(blockSize), &blockSize);
				if (preferencesResult != kPreferences_ResultCodeSuccess)
				{
					saveError = true;
				}
			}
			
			//{
			//if GetDialogItemControlText(gSessionEditorDialog, SessAlias, inoutPreferencesResourceName)
			//then Preferences_ContextRename()
			//}
			
			(Preferences_ResultCode)Preferences_ContextSave(preferencesContext);
			
			DialogTransitions_CloseToRectangle(GetDialogWindow(gSessionEditorDialog), parentWindow, inTransitionOpenRectOrNull);
			
			done();
			
			Preferences_DisposeAlias(pendingCaptureFileAliasID);
			DisposeMenu(characterSetMenu), characterSetMenu = nullptr;
			DisposeMenu(terminalsMenu), terminalsMenu = nullptr;
			
			// a resource has been changed or added, so result is true
			result = true;
		}
		Preferences_ReleaseContext(&preferencesContext);
	}
	
	return result;
}// Display


//
// internal methods
//

/*!
Call this method to destroy the dialog box
and its associated data structures.

(3.0)
*/
static void
done ()
{
	WindowInfo_Dispose(gSessionEditorDialogWindowInfo);
	DisposeDialog(gSessionEditorDialog), gSessionEditorDialog = nullptr;
}// done


/*!
This method is used to initialize the Session Favorite
Editor dialog box.  It creates the dialog box invisibly,
and sets up the controls in the dialog box.

(3.0)
*/
static void
init ()
{
	GetNewDialogWithWindowInfo(kDialogIDSessionFavoriteEditor, &gSessionEditorDialog,
								&gSessionEditorDialogWindowInfo);
	
	if (gSessionEditorDialog != nullptr)
	{
		// adjust the OK and Cancel buttons so they are big enough for their titles
		SetDialogDefaultItem(gSessionEditorDialog, SessOK);
		SetDialogCancelItem(gSessionEditorDialog, SessCancel);
		(UInt16)Localization_AdjustDialogButtons(gSessionEditorDialog, true/* OK */, true/* Cancel */);
		Localization_AdjustHelpButtonItem(gSessionEditorDialog, SessHelpButton);
		
		// prevent the user from typing non-digits into certain text fields
		{
			ControlKeyFilterUPP		upp = NumericalLimiterKeyFilterUPP();
			ControlRef				field = nullptr;
			
			
			GetDialogItemAsControl(gSessionEditorDialog, SessPort, &field);
			(OSStatus)SetControlData(field, kControlNoPart, kControlKeyFilterTag, sizeof(upp), &upp);
			GetDialogItemAsControl(gSessionEditorDialog, SessPasteBlockSize, &field);
			(OSStatus)SetControlData(field, kControlNoPart, kControlKeyFilterTag, sizeof(upp), &upp);
			GetDialogItemAsControl(gSessionEditorDialog, SessNetworkBlockSize, &field);
			(OSStatus)SetControlData(field, kControlNoPart, kControlKeyFilterTag, sizeof(upp), &upp);
		}
		
		// set up the Window Info information
		{
			//WindowResizeResponderProcPtr	resizeProc = handleNewSize;
			
			
			WindowInfo_SetWindowDescriptor(gSessionEditorDialogWindowInfo,
											kConstantsRegistry_WindowDescriptorSessionEditor);
			#if 0
			WindowInfo_SetWindowResizeLimits(gSessionEditorDialogWindowInfo,
													?/* minimum height */,
													?/* minimum width */,
													? + 128/* maximum height */,
													? + 50/* maximum width */);
			WindowInfo_SetWindowResizeResponderUPP(gSessionEditorDialogWindowInfo, resizeProc, 0L);
			#endif
		}
		
		WindowInfo_SetForDialog(gSessionEditorDialog, gSessionEditorDialogWindowInfo);
	}
}// init


/*!
Handles special key events in the
Session Favorite Editor dialog box.

(3.0)
*/
static pascal Boolean
sessionEditorDialogEventFilter	(DialogRef		inDialog,
								 EventRecord*	inoutEventPtr,
								 short*			outItemIndex)
{
	Boolean		result = pascal_false,
				isButtonAndShouldFlash = false; // simplifies handling of multiple key-to-button maps
	
	
	if (!result) result = (StandardDialogEventFilter(inDialog, inoutEventPtr, outItemIndex));
	else if (isButtonAndShouldFlash) FlashButton(inDialog, *outItemIndex);
	
	return result;
}// sessionEditorDialogEventFilter


/*!
To change the active states of controls
based on the values of certain other
controls, use this method.

(3.0)
*/
static void
setItemStates ()
{
	// disable Half Duplex unless Local Echo is enabled
	if (GetDialogItemValue(gSessionEditorDialog, SessLocalEcho) == kControlCheckBoxCheckedValue)
	{
		ActivateDialogItem(gSessionEditorDialog, SessHalfDuplex);
	}
	else
	{
		DeactivateDialogItem(gSessionEditorDialog, SessHalfDuplex);
	}
	
	// set correct states for Capture File controls
	if (GetDialogItemValue(gSessionEditorDialog, SessAutoCaptureToFile) == kControlCheckBoxCheckedValue)
	{
		ActivateDialogItem(gSessionEditorDialog, SessAutoCaptureSetFile);
		ActivateDialogItem(gSessionEditorDialog, SessAutoCaptureFilePath);
	}
	else
	{
		DeactivateDialogItem(gSessionEditorDialog, SessAutoCaptureSetFile);
		DeactivateDialogItem(gSessionEditorDialog, SessAutoCaptureFilePath);
	}
}// setItemStates

// BELOW IS REQUIRED NEWLINE TO END FILE
