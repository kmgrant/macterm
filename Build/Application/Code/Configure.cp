/*###############################################################

	Configure.cp
	
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
#include <cctype>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <FileSelectionDialogs.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <WindowInfo.h>

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "MenuResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "ColorBox.h"
#include "Configure.h"
#include "ConstantsRegistry.h"
#include "DialogTransitions.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "HelpSystem.h"
#include "Preferences.h"
#include "Session.h"
#include "Terminal.h"



#pragma mark Constants

enum
{
	// dialog item indices
	iNameNewSessionFavoriteCreateButton = 1,
	iNameNewSessionFavoriteCancelButton = 2,
	iNameNewSessionFavoriteAppleGuideButton = 3,
	iNameNewSessionFavoriteFieldLabelText = 4,
	iNameNewSessionFavoriteNameField = 5,
	TermOKButton			= 1,
	TermCancelButton		= 2,
	TermNameStatText		= 3,	// terminal configuration name label
	TermName				= 4,	// terminal configuration name field
	TermANSI				= 5,	// ANSI escape sequences checkbox
	TermXterm				= 6,	// Xterm escape sequences checkbox
	Termvtwrap				= 7,	// text wrap mode checkbox
	Termarrow				= 8,	// EMACS arrow keys checkbox
	Termeightbit			= 9,	// 8-bit connection checkbox
	Termclearsave			= 10,	// clear screen saves lines checkbox
	TermRemapKeypad			= 11,	// VT220 keypad checkbox
	TermMAT					= 12,	// map page-up, page-down, etc. checkbox
	TermMetaIsCmdCntrol		= 14,	// use control-cmd as meta
	TermMetaIsOption		= 15,	// use option as meta
	TermMetaIsOff			= 16,	// no meta
	TermVT100				= 18,	// VT100 emulation radio button
	TermVT220				= 19,	// VT220 emulation radio button
	TermDumb				= 20,	// dumb terminal radio button
	TermAnswerback			= 22,	// perceived emulator field
	TermFontPopup			= 26,	// font pop-up menu
	TermFontSize			= 27,	// font size field
	TermWidth				= 29,	// terminal width field
	TermHeight				= 31,	// terminal height field
	TermScrollback			= 33,	// scrollback field
	TermNFcolor				= 36,	// normal text color box
	TermBoldFcolor			= 37,	// bold text color box
	TermBFcolor				= 38,	// blinking text color box
	TermNBcolor				= 40,	// normal background color box
	TermBoldBcolor			= 41,	// bold background color box
	TermBBcolor				= 42,	// blinking background color box
	TermHelpButton			= 43	// Apple Guide button
};

//
// internal methods
//

static pascal void		adjustNameNewSessionFavoriteDialog		(WindowRef		inWindow,
																 SInt32			inDeltaSizeX,
																 SInt32			inDeltaSizeY,
																 SInt32			inData);



//
// public methods
//

/*!
Displays a dialog box with a text field asking the user
to name a new configuration.  If the user cancels, "false"
is returned; otherwise, "true" is returned.

You can optionally specify the title of the window; if
you don’t specify a title (by passing nullptr for the
"inWindowTitle" parameter), the window’s title is blank.

You must specify the initial text for the name field; if
you don’t want any default text, pass an empty string
(if you pass nullptr, this routine does nothing!).  On
output, the name field string is set to an empty string
if the user cancels, or the user’s chosen name text if
the user does not cancel.

Rectangles can be specified for open and close transition
actions; if a rectangle is given, then the dialog will
display as if it came from the rectangle (the default is
a rectangle from the center of the screen).  The close
transition only takes place if the user clicks OK.

(3.0)
*/
Boolean
Configure_NameNewConfigurationDialogDisplay		(ConstStringPtr		inWindowTitle,
												 StringPtr			inoutName,
												 Rect const*		inTransitionOpenRectOrNull,
												 Rect const*		inTransitionCloseRectOrNull)
{
	Boolean		result = false;
	
	
	if (inoutName != nullptr)
	{
		WindowInfoRef		windowInfo = nullptr;
		DialogRef			dialogPtr = nullptr;
		WindowRef			windowPtr = nullptr,
							parentWindow = nullptr;
		DialogItemIndex		itemHit = 0;
		
		
		GetNewDialogWithWindowInfo(kDialogIDNewSessionFavoriteName, &dialogPtr, &windowInfo);
		windowPtr = GetDialogWindow(dialogPtr);
		
		// set the window title properly
		if (inWindowTitle != nullptr)
		{
			CFStringRef		titleCFString = CFStringCreateWithPascalString
											(kCFAllocatorDefault, inWindowTitle, kCFStringEncodingMacRoman);
			
			
			if (titleCFString != nullptr)
			{
				SetWindowTitleWithCFString(windowPtr, titleCFString);
				CFRelease(titleCFString), titleCFString = nullptr;
			}
		}
		else SetWindowTitleWithCFString(windowPtr, CFSTR(""));
		
		SetDialogDefaultItem(dialogPtr, iNameNewSessionFavoriteCreateButton);
		SetDialogCancelItem(dialogPtr, iNameNewSessionFavoriteCancelButton);
		
		SetDialogItemControlText(dialogPtr, iNameNewSessionFavoriteNameField, inoutName);
		
		itemHit = 0;
		
		// adjust the Create and Cancel buttons so they are big enough for their titles
		(UInt16)Localization_AdjustDialogButtons(dialogPtr, true/* OK */, true/* Cancel */);
		Localization_AdjustHelpButtonItem(dialogPtr, iNameNewSessionFavoriteAppleGuideButton);
		Localization_HorizontallyPlaceItems(dialogPtr, iNameNewSessionFavoriteFieldLabelText, iNameNewSessionFavoriteNameField);
		
		parentWindow = EventLoop_GetRealFrontWindow();
		DialogTransitions_DisplayFromRectangle(GetDialogWindow(dialogPtr), parentWindow, inTransitionOpenRectOrNull);
		
		Cursors_UseArrow();
		
		// set up the Window Info information
		{
			WindowResizeResponderProcPtr	resizeProc = adjustNameNewSessionFavoriteDialog;
			Rect							rect;
			
			
			(OSStatus)GetWindowBounds(GetDialogWindow(dialogPtr), kWindowContentRgn, &rect);
			
			WindowInfo_SetWindowDescriptor(windowInfo, kConstantsRegistry_WindowDescriptorNameNewFavorite);
			WindowInfo_SetWindowResizeLimits(windowInfo,
													rect.bottom - rect.top + 1/* minimum height */,
													rect.right - rect.left + 1/* minimum width */,
													rect.bottom - rect.top + 1/* maximum height */,
													rect.right - rect.left + 140/* maximum width - arbitrary */);
			WindowInfo_SetWindowResizeResponder(windowInfo, resizeProc, 0L);
		}
		WindowInfo_SetForDialog(dialogPtr, windowInfo);
		
		{
			ControlRef		fieldControl = nullptr;
			
			
			(OSStatus)GetDialogItemAsControl(dialogPtr, iNameNewSessionFavoriteNameField, &fieldControl);
			(OSStatus)SetKeyboardFocus(GetDialogWindow(dialogPtr), fieldControl, kControlFocusNextPart);
		}
		
		{
			ModalFilterUPP		filterProc = NewModalFilterUPP(StandardDialogEventFilter);
			
			
			do
			{
				ModalDialog(filterProc, &itemHit);
				if (itemHit == iNameNewSessionFavoriteAppleGuideButton)
				{
					HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseFavorites);
				}
			} while ((itemHit != iNameNewSessionFavoriteCreateButton) && (itemHit != iNameNewSessionFavoriteCancelButton));
		}
		
		if (itemHit == iNameNewSessionFavoriteCreateButton)
		{
			// return the title
			GetDialogItemControlText(dialogPtr, iNameNewSessionFavoriteNameField, inoutName);
			if (PLstrlen(inoutName) <= 0) PLstrcpy(inoutName, EMPTY_PSTRING);
			DialogTransitions_CloseToRectangle(GetDialogWindow(dialogPtr), parentWindow, inTransitionCloseRectOrNull);
			result = true;
		}
		else
		{
			// cancelled
			PLstrcpy(inoutName, EMPTY_PSTRING);
			DialogTransitions_CloseCancel(GetDialogWindow(dialogPtr));
			result = false;
		}
		WindowInfo_Dispose(windowInfo);
		DisposeDialog(dialogPtr), dialogPtr = nullptr;
	}
	
	return result;
}// NameNewConfigurationDialogDisplay


/*!
Use this routine if you need to ask the user
where an application is.  The given prompt and
title only apply if Navigation Services is
available.

Returns "true" only if the output file is set.

(3.0)
*/
Boolean
GetApplicationSpec	(ConstStr255Param		inPrompt,
					 ConstStr255Param		inTitle,
					 FSSpec*				outSpec)
{
	Boolean		result = false;
	FileInfo	fi;
	OSType		fileType;
	OSType		types[] = {'APPL'};
	OSStatus	error = noErr;
	
	
	Alert_ReportOSStatus(error = FileSelectionDialogs_GetFile
									(inPrompt, inTitle, kConstantsRegistry_ApplicationCreatorSignature,
										kPreferences_NavPrefKeyChooseTextEditor,
										kNavNoTypePopup |
										kNavDontAutoTranslate |
										kNavSupportPackages |
										kNavDontAddTranslateItems, 1, types,
										EventLoop_HandleNavigationUpdate, outSpec, &fileType, &fi));
	result = (error == noErr);
	return result;
}// GetApplicationSpec


/*!
Use this routine whenever you need to ask the user to
choose a text editing application.  Now uses
Navigation Services in version 3.0.

Returns "true" only if the type was retrieved.

(2.6)
*/
Boolean
GetApplicationType	(OSType*	type)
{
	Boolean		result = false;
	FileInfo	fi;
	Str255		prompt,
				title;
	
	
	// updated for Navigation Services support
	GetIndString(prompt, rStringsNavigationServices, siNavPromptSelectTextFile);
	GetIndString(title, rStringsNavigationServices, siNavDialogTitleExampleTextApplication);
	{
		FSSpec		file;
		OSType		fileType;
		OSType		types[] = { 'APPL' };
		OSStatus	error = noErr;
		
		
		Alert_ReportOSStatus(error = FileSelectionDialogs_GetFile
										(prompt, title, kConstantsRegistry_ApplicationCreatorSignature,
											kPreferences_NavPrefKeyChooseTextEditor,
											kNavNoTypePopup |
											kNavDontAutoTranslate |
											kNavSupportPackages |
											kNavDontAddTranslateItems, 1, types,
											EventLoop_HandleNavigationUpdate, &file, &fileType, &fi));
		result = (error == noErr);
	}
	
	if (result) BlockMoveData(&fi.fileCreator, type, sizeof(OSType)); // copy the application creator type
		
	return result;
}// GetApplicationType


/*!
This method displays and handles events in the
Terminal Favorite Editor dialog box.  If the
user clicks OK, "true" is returned; otherwise,
"false" is returned.

(3.0)
*/
Boolean
EditTerminal	(Preferences_Class		inClass,
				 CFStringRef			inContextName,
				 Rect const*			inTransitionOpenRectOrNull)
{
	Boolean					result = false;
	Preferences_ContextRef	preferencesContext = nullptr;
	
	
	preferencesContext = Preferences_NewContext(inClass, inContextName);
	if (preferencesContext == nullptr) result = false;
	else
	{
		Preferences_Result		preferencesResult = kPreferences_ResultOK;
		DialogRef				dialog = nullptr;
		WindowRef				parentWindow = nullptr;
		ModalFilterUPP			filterUPP = nullptr;
		DialogItemIndex			selectedItemIndex = 0;
		long					scratchlong = 0;
		Boolean					wasInAliasText = false;
		Boolean					flag = false;
		Str255					scratchPstring;
		
		
		dialog = GetNewDialog(kDialogIDTerminalEditor, nullptr/* storage */, kFirstWindowOfClass);
		
		// adjust the OK and Cancel buttons so they are big enough for their titles
		SetDialogDefaultItem(dialog, DLOGOk);
		SetDialogCancelItem(dialog, DLOGCancel);
		SetDialogTracksCursor(dialog, true);
		(UInt16)Localization_AdjustDialogButtons(dialog, true/* OK */, true/* Cancel */);
		Localization_AdjustHelpButtonItem(dialog, TermHelpButton);
		
	#if TARGET_API_MAC_CARBON
		(OSStatus)HMSetDialogHelpFromBalloonRsrc(dialog, kDialogBalloonsIDTerminalEditor, 1/* starting item */);
	#endif
		
		HideDialogItem(dialog, TermName); // no longer used
		
		selectedItemIndex = 3;
		
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagANSIColorsEnabled,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, TermANSI, (flag) ? kControlCheckBoxCheckedValue
														: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagXTermSequencesEnabled,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, TermXterm, (flag) ? kControlCheckBoxCheckedValue
															: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalLineWrap,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, Termvtwrap, (flag) ? kControlCheckBoxCheckedValue
															: kControlCheckBoxUncheckedValue);
		}
		{
			Session_EMACSMetaKey	metaKey = kSession_EMACSMetaKeyOff;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagEMACSMetaKey,
															sizeof(metaKey), &metaKey);
			if (preferencesResult != kPreferences_ResultOK) metaKey = kSession_EMACSMetaKeyOff;
			switch (metaKey)
			{
			case kSession_EMACSMetaKeyControlCommand:
				SetDialogItemValue(dialog, TermMetaIsCmdCntrol, kControlRadioButtonCheckedValue);
				SetDialogItemValue(dialog, TermMetaIsOption, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermMetaIsOff, kControlRadioButtonUncheckedValue);
				break;
			
			case kSession_EMACSMetaKeyOption:
				SetDialogItemValue(dialog, TermMetaIsCmdCntrol, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermMetaIsOption, kControlRadioButtonCheckedValue);
				SetDialogItemValue(dialog, TermMetaIsOff, kControlRadioButtonUncheckedValue);
				break;
			
			case kSession_EMACSMetaKeyOff:
			default:
				SetDialogItemValue(dialog, TermMetaIsCmdCntrol, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermMetaIsOption, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermMetaIsOff, kControlRadioButtonCheckedValue);
				break;
			}
			DeactivateDialogItem(dialog, TermMetaIsOption); // 3.0 - this hack is not supported
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagMapArrowsForEMACS,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, Termarrow, (flag) ? kControlCheckBoxCheckedValue
															: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagPageKeysControlLocalTerminal,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, TermMAT, (flag) ? kControlCheckBoxCheckedValue
														: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalClearSavesLines,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, Termclearsave, (flag) ? kControlCheckBoxCheckedValue
																: kControlCheckBoxUncheckedValue);
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagDataReceiveDoNotStripHighBit,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, Termeightbit, (flag) ? kControlCheckBoxCheckedValue
															: kControlCheckBoxUncheckedValue);
		}
		{
			Terminal_Emulator	emulatorType = kTerminal_EmulatorVT100;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalEmulatorType,
															sizeof(emulatorType), &emulatorType);
			if (preferencesResult != kPreferences_ResultOK) emulatorType = kTerminal_EmulatorVT100;
			switch (emulatorType)
			{
			case kTerminal_EmulatorDumb:
				SetDialogItemValue(dialog, TermVT100, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermVT220, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermDumb, kControlRadioButtonCheckedValue);
				break;
			
			case kTerminal_EmulatorVT220:
				SetDialogItemValue(dialog, TermVT100, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermVT220, kControlRadioButtonCheckedValue);
				SetDialogItemValue(dialog, TermDumb, kControlRadioButtonUncheckedValue);
				break;
			
			case kTerminal_EmulatorVT100:
			default:
				SetDialogItemValue(dialog, TermVT100, kControlRadioButtonCheckedValue);
				SetDialogItemValue(dialog, TermVT220, kControlRadioButtonUncheckedValue);
				SetDialogItemValue(dialog, TermDumb, kControlRadioButtonUncheckedValue);
				break;
			}
			
			if (emulatorType != kTerminal_EmulatorVT220)
			{
				HideDialogItem(dialog,TermRemapKeypad);
				HideDialogItem(dialog,TermMAT);
			}
		}
		{
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagMapKeypadTopRowForVT220,
															sizeof(flag), &flag);
			if (preferencesResult != kPreferences_ResultOK) flag = false;
			SetDialogItemValue(dialog, TermRemapKeypad, (flag) ? kControlCheckBoxCheckedValue
																: kControlCheckBoxUncheckedValue);
		}
		{
			UInt16		dimension = 0;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalScreenColumns,
															sizeof(dimension), &dimension);
			if (preferencesResult != kPreferences_ResultOK) dimension = 80; // arbitrary
			SetDialogItemNumericalText(dialog, TermWidth, dimension);
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalScreenRows,
															sizeof(dimension), &dimension);
			if (preferencesResult != kPreferences_ResultOK) dimension = 24; // arbitrary
			SetDialogItemNumericalText(dialog, TermHeight, dimension);
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalScreenScrollbackRows,
															sizeof(dimension), &dimension);
			if (preferencesResult != kPreferences_ResultOK) dimension = 200; // arbitrary
			SetDialogItemNumericalText(dialog, TermScrollback, dimension);
		}
		{
			SInt16		fontSize = 0;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagFontSize,
															sizeof(fontSize), &fontSize);
			if (preferencesResult != kPreferences_ResultOK) fontSize = 12; // arbitrary
			SetDialogItemNumericalText(dialog, TermFontSize, fontSize);
		}
		{
			Str255		answerBack;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalAnswerBackMessage,
															sizeof(answerBack), answerBack);
			if (preferencesResult != kPreferences_ResultOK) PLstrcpy(answerBack, "\pvt100"); // arbitrary
			SetDialogItemControlText(dialog, TermAnswerback, answerBack);
		}
		
		// pre-select the appropriate font in the menu
		{
			Str255			fontName;
			CFStringRef		fontNameCFString;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagFontName,
															sizeof(fontName), fontName);
			if (preferencesResult != kPreferences_ResultOK) PLstrcpy(fontName, "\pMonaco"); // arbitrary
			
			fontNameCFString = CFStringCreateWithPascalString(kCFAllocatorDefault, fontName, kCFStringEncodingMacRoman);
			if (nullptr != fontNameCFString)
			{
				MenuHandle			fontMenu = nullptr;
				ControlRef			fontMenuButton = nullptr;
				CFStringRef			menuItemCFString = nullptr;
				Size				actualSize = 0L;
				register SInt16		i = 0;
				
				
				GetDialogItemAsControl(dialog, TermFontPopup, &fontMenuButton);
				(OSStatus)GetControlData(fontMenuButton, kControlMenuPart, kControlPopupButtonMenuHandleTag, sizeof(fontMenu), (Ptr)&fontMenu, &actualSize);
				for (i = CountMenuItems(fontMenu); i > 0; i--)
				{
					if (noErr == CopyMenuItemTextAsCFString(fontMenu, i, &menuItemCFString))
					{
						if (kCFCompareEqualTo == CFStringCompare
													(fontNameCFString, menuItemCFString, kCFCompareCaseInsensitive))
						{
							SetControl32BitValue(fontMenuButton, i);
						}
						CFRelease(menuItemCFString), menuItemCFString = nullptr;
					}
				}
				CFRelease(fontNameCFString), fontNameCFString = nullptr;
			}
		}
		
		{
			RGBColor	colorValue;
			
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalColorNormalForeground,
															sizeof(colorValue), &colorValue);
			if (preferencesResult != kPreferences_ResultOK)
			{
				colorValue.red = colorValue.green = colorValue.blue = 0; // arbitrary
			}
			//ColorBox_AttachToUserPaneDialogItem(dialog, TermNFcolor, &colorValue);
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalColorNormalBackground,
															sizeof(colorValue), &colorValue);
			if (preferencesResult != kPreferences_ResultOK)
			{
				colorValue.red = colorValue.green = colorValue.blue = 0; // arbitrary
			}
			//ColorBox_AttachToUserPaneDialogItem(dialog, TermNBcolor, &colorValue);
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalColorBoldForeground,
															sizeof(colorValue), &colorValue);
			if (preferencesResult != kPreferences_ResultOK)
			{
				colorValue.red = colorValue.green = colorValue.blue = 0; // arbitrary
			}
			//ColorBox_AttachToUserPaneDialogItem(dialog, TermBoldFcolor, &colorValue);
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalColorBoldBackground,
															sizeof(colorValue), &colorValue);
			if (preferencesResult != kPreferences_ResultOK)
			{
				colorValue.red = colorValue.green = colorValue.blue = 0; // arbitrary
			}
			//ColorBox_AttachToUserPaneDialogItem(dialog, TermBoldBcolor, &colorValue);
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalColorBlinkingForeground,
															sizeof(colorValue), &colorValue);
			if (preferencesResult != kPreferences_ResultOK)
			{
				colorValue.red = colorValue.green = colorValue.blue = 0; // arbitrary
			}
			//ColorBox_AttachToUserPaneDialogItem(dialog, TermBFcolor, &colorValue);
			
			preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalColorBlinkingBackground,
															sizeof(colorValue), &colorValue);
			if (preferencesResult != kPreferences_ResultOK)
			{
				colorValue.red = colorValue.green = colorValue.blue = 0; // arbitrary
			}
			//ColorBox_AttachToUserPaneDialogItem(dialog, TermBBcolor, &colorValue);
		}
		
		// set up certain text fields to only allow numerical input
		{
			ControlRef				field = nullptr;
			ControlKeyFilterUPP		upp = NumericalLimiterKeyFilterUPP();
			
			
			GetDialogItemAsControl(dialog, TermFontSize, &field);
			(OSStatus)SetControlData(field, kControlNoPart, kControlKeyFilterTag, sizeof(upp), &upp);
			GetDialogItemAsControl(dialog, TermWidth, &field);
			(OSStatus)SetControlData(field, kControlNoPart, kControlKeyFilterTag, sizeof(upp), &upp);
			GetDialogItemAsControl(dialog, TermHeight, &field);
			(OSStatus)SetControlData(field, kControlNoPart, kControlKeyFilterTag, sizeof(upp), &upp);
			GetDialogItemAsControl(dialog, TermScrollback, &field);
			(OSStatus)SetControlData(field, kControlNoPart, kControlKeyFilterTag, sizeof(upp), &upp);
		}
		
		// adjust the OK and Cancel buttons so they are big enough for their titles
		(UInt16)Localization_AdjustDialogButtons(dialog, true/* OK */, true/* Cancel */);
		
		wasInAliasText = false;
		
		// display dialog and handle events
		parentWindow = EventLoop_GetRealFrontWindow();
		DialogTransitions_DisplayFromRectangle(GetDialogWindow(dialog), parentWindow, inTransitionOpenRectOrNull);
		
		// set the focus
		FocusDialogItem(dialog, TermWidth);
		
		filterUPP = NewModalFilterUPP(StandardDialogEventFilter);
		do
		{
			ModalDialog(filterUPP, &selectedItemIndex);
			switch (selectedItemIndex)
			{
			case TermHelpButton:
				HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseTerminals);
				break;
			
			case TermANSI:
			case TermXterm:
			case Termvtwrap:
			case Termarrow:
			case TermMAT:
			case Termeightbit:
			case Termclearsave:
			case TermRemapKeypad:
				FlipCheckBox(dialog, selectedItemIndex);
				break;
			
			case TermVT100:
				SetDialogItemValue(dialog, TermVT100, 1);
				SetDialogItemValue(dialog, TermVT220, 0);
				SetDialogItemValue(dialog, TermDumb, 0);
				SetDialogItemControlText(dialog, TermAnswerback, "\pvt100");
				DrawOneDialogItem(dialog, TermAnswerback);
				HideDialogItem(dialog,TermRemapKeypad);
				HideDialogItem(dialog,TermMAT);
				break;
			
			case TermVT220:
				SetDialogItemValue(dialog, TermVT100, 0);
				SetDialogItemValue(dialog, TermVT220, 1);
				SetDialogItemValue(dialog, TermDumb, 0);
				SetDialogItemControlText(dialog, TermAnswerback, "\pvt220");
				DrawOneDialogItem(dialog, TermAnswerback);
				ShowDialogItem(dialog,TermRemapKeypad);
				ShowDialogItem(dialog,TermMAT);
				break;
			
			case TermDumb:
				SetDialogItemValue(dialog, TermVT100, 0);
				SetDialogItemValue(dialog, TermVT220, 0);
				SetDialogItemValue(dialog, TermDumb, 1);
				SetDialogItemControlText(dialog, TermAnswerback, "\p");
				DrawOneDialogItem(dialog, TermAnswerback);
				ShowDialogItem(dialog,TermRemapKeypad);
				HideDialogItem(dialog,TermMAT);
				break;
			
			case TermMetaIsCmdCntrol:
				SetDialogItemValue(dialog, TermMetaIsOption, 0);
				SetDialogItemValue(dialog, TermMetaIsOff, 0);
				SetDialogItemValue(dialog, TermMetaIsCmdCntrol, 1);
				break;
			
			case TermMetaIsOption:
				SetDialogItemValue(dialog, TermMetaIsOff, 0);
				SetDialogItemValue(dialog, TermMetaIsCmdCntrol, 0);
				SetDialogItemValue(dialog, TermMetaIsOption, 1);
				break;
			
			case TermMetaIsOff:
				SetDialogItemValue(dialog, TermMetaIsCmdCntrol, 0);
				SetDialogItemValue(dialog, TermMetaIsOption, 0);
				SetDialogItemValue(dialog, TermMetaIsOff, 1);
				break;
			
			case TermNFcolor:
			case TermNBcolor:
			case TermBoldFcolor:
			case TermBoldBcolor:
			case TermBFcolor:
			case TermBBcolor:
				{
					ControlRef		control = nullptr;
					
					
					(OSStatus)GetDialogItemAsControl(dialog, selectedItemIndex, &control);
					ColorBox_UserSetColor(control);
				}
				break;
			
			default:
				break;
			}
		} while ((DLOGOk != selectedItemIndex) && (DLOGCancel != selectedItemIndex));
		DisposeModalFilterUPP(filterUPP), filterUPP = nullptr;
		
		if (DLOGCancel == selectedItemIndex)
		{
			DialogTransitions_CloseCancel(GetDialogWindow(dialog));
			DisposeDialog(dialog), dialog = nullptr;
			result = false;
		}
		else
		{
			// update color preferences
			{
				ControlRef		control = nullptr;
				RGBColor		colorValue;
				
				
				(OSStatus)GetDialogItemAsControl(dialog, TermNFcolor, &control);
				ColorBox_GetColor(control, &colorValue);
				preferencesResult = Preferences_ContextSetData
									(preferencesContext, kPreferences_TagTerminalColorNormalForeground,
										sizeof(colorValue), &colorValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
				(OSStatus)GetDialogItemAsControl(dialog, TermNBcolor, &control);
				ColorBox_GetColor(control, &colorValue);
				preferencesResult = Preferences_ContextSetData
									(preferencesContext, kPreferences_TagTerminalColorNormalBackground,
										sizeof(colorValue), &colorValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
				(OSStatus)GetDialogItemAsControl(dialog, TermBoldFcolor, &control);
				ColorBox_GetColor(control, &colorValue);
				preferencesResult = Preferences_ContextSetData
									(preferencesContext, kPreferences_TagTerminalColorBoldForeground,
										sizeof(colorValue), &colorValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
				(OSStatus)GetDialogItemAsControl(dialog, TermBoldBcolor, &control);
				ColorBox_GetColor(control, &colorValue);
				preferencesResult = Preferences_ContextSetData
									(preferencesContext, kPreferences_TagTerminalColorBoldBackground,
										sizeof(colorValue), &colorValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
				(OSStatus)GetDialogItemAsControl(dialog, TermBFcolor, &control);
				ColorBox_GetColor(control, &colorValue);
				preferencesResult = Preferences_ContextSetData
									(preferencesContext, kPreferences_TagTerminalColorBlinkingForeground,
										sizeof(colorValue), &colorValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
				(OSStatus)GetDialogItemAsControl(dialog, TermBBcolor, &control);
				ColorBox_GetColor(control, &colorValue);
				preferencesResult = Preferences_ContextSetData
									(preferencesContext, kPreferences_TagTerminalColorBlinkingBackground,
										sizeof(colorValue), &colorValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			
			{
				MenuHandle		fontMenu = nullptr;
				ControlRef		fontMenuButton = nullptr;
				Size			actualSize = 0L;
				
				
				(OSStatus)GetDialogItemAsControl(dialog, TermFontPopup, &fontMenuButton);
				if (noErr == GetControlData(fontMenuButton, kControlMenuPart, kControlPopupButtonMenuHandleTag,
											sizeof(fontMenu), &fontMenu, &actualSize))
				{
					CFStringRef		menuItemCFString = nullptr;
					
					
					if (noErr == CopyMenuItemTextAsCFString(fontMenu, GetControl32BitValue(fontMenuButton),
															&menuItemCFString))
					{
						if (CFStringGetPascalString(menuItemCFString, scratchPstring, sizeof(scratchPstring),
													kCFStringEncodingMacRoman))
						{
							if (PLstrlen(scratchPstring) > 63) scratchPstring[0] = 63;
							preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagFontName,
																			sizeof(scratchPstring), scratchPstring);
							if (preferencesResult != kPreferences_ResultOK)
							{
								// error...
							}
						}
						CFRelease(menuItemCFString), menuItemCFString = nullptr;
					}
				}
			}
			{
				UInt16		fontSize = 0;
				
				
				GetDialogItemControlText(dialog, TermFontSize, scratchPstring);
				StringToNum(scratchPstring, &scratchlong);
				if (scratchlong > 24) scratchlong = 24;
				if (scratchlong < 4) scratchlong = 4;
				fontSize = STATIC_CAST(scratchlong, UInt16);
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagFontSize,
																sizeof(fontSize), &fontSize);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, TermANSI));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagANSIColorsEnabled,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, TermXterm));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagXTermSequencesEnabled,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, Termvtwrap));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalLineWrap,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			
			{
				Session_EMACSMetaKey	metaKeyType = kSession_EMACSMetaKeyOff;
				
				
				if (GetDialogItemValue(dialog, TermMetaIsCmdCntrol) != kControlRadioButtonUncheckedValue)
				{
					metaKeyType = kSession_EMACSMetaKeyControlCommand;
				}
				else if (GetDialogItemValue(dialog, TermMetaIsOption) != kControlRadioButtonUncheckedValue)
				{
					metaKeyType = kSession_EMACSMetaKeyOption;
				}
				
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagEMACSMetaKey,
																sizeof(metaKeyType), &metaKeyType);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, Termarrow));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagMapArrowsForEMACS,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, TermMAT));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagPageKeysControlLocalTerminal,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, Termeightbit));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagDataReceiveDoNotStripHighBit,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, Termclearsave));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagTerminalClearSavesLines,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				flag = (kControlCheckBoxCheckedValue == GetDialogItemValue(dialog, TermRemapKeypad));
				preferencesResult = Preferences_ContextGetData(preferencesContext, kPreferences_TagMapKeypadTopRowForVT220,
																sizeof(flag), &flag);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				Terminal_Emulator	emulatorType = kTerminal_EmulatorVT100;
				
				
				if (GetDialogItemValue(dialog, TermDumb) != kControlRadioButtonUncheckedValue)
				{
					emulatorType = kTerminal_EmulatorDumb;
				}
				else if (GetDialogItemValue(dialog, TermVT220) != kControlRadioButtonUncheckedValue)
				{
					emulatorType = kTerminal_EmulatorVT220;
				}
				
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTerminalEmulatorType,
																sizeof(emulatorType), &emulatorType);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			{
				UInt16		dimensionValue = 0;
				
				
				GetDialogItemControlText(dialog, TermWidth, scratchPstring);
				StringToNum(scratchPstring, &scratchlong);
				if (scratchlong > 133) scratchlong = 133;
				if (scratchlong < 10) scratchlong = 10;
				dimensionValue = STATIC_CAST(scratchlong, UInt16);
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTerminalScreenColumns,
																sizeof(dimensionValue), &dimensionValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
				
				GetDialogItemControlText(dialog, TermHeight, scratchPstring);
				StringToNum(scratchPstring, &scratchlong);
				if (scratchlong > 80) scratchlong = 80;
				if (scratchlong < 10) scratchlong = 10;
				dimensionValue = STATIC_CAST(scratchlong, UInt16);
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTerminalScreenRows,
																sizeof(dimensionValue), &dimensionValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
				
				GetDialogItemControlText(dialog, TermHeight, scratchPstring);
				StringToNum(scratchPstring, &scratchlong);
				if (scratchlong > 50000) scratchlong = 50000;
				if (scratchlong < 24) scratchlong = 24;
				dimensionValue = STATIC_CAST(scratchlong, UInt16);
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTerminalScreenScrollbackRows,
																sizeof(dimensionValue), &dimensionValue);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			
			{
				GetDialogItemControlText(dialog, TermAnswerback, scratchPstring);
				if (PLstrlen(scratchPstring) > 63) scratchPstring[0] = 63;
				preferencesResult = Preferences_ContextSetData(preferencesContext, kPreferences_TagTerminalAnswerBackMessage,
																sizeof(scratchPstring), scratchPstring);
				if (preferencesResult != kPreferences_ResultOK)
				{
					// error...
				}
			}
			
			(Preferences_Result)Preferences_ContextSave(preferencesContext);
			
		#if 0
			ColorBox_DetachFromUserPaneDialogItem(dialog, TermNFcolor);
			ColorBox_DetachFromUserPaneDialogItem(dialog, TermNBcolor);
			ColorBox_DetachFromUserPaneDialogItem(dialog, TermBoldFcolor);
			ColorBox_DetachFromUserPaneDialogItem(dialog, TermBoldBcolor);
			ColorBox_DetachFromUserPaneDialogItem(dialog, TermBFcolor);
			ColorBox_DetachFromUserPaneDialogItem(dialog, TermBBcolor);
		#endif
			
			DialogTransitions_CloseToRectangle(GetDialogWindow(dialog), parentWindow, inTransitionOpenRectOrNull);
			DisposeDialog(dialog), dialog = nullptr;
			
			// resource was added or changed, so result is success
			result = true;
		}
		Preferences_ReleaseContext(&preferencesContext);
	}
	
	return result;
}// EditTerminal


//
// internal methods
//
#pragma mark -

/*!
This method moves the buttons and resizes the text field
in response to a resize of the “Name New Session Favorite”
movable modal dialog box.

(3.0)
*/
static pascal void
adjustNameNewSessionFavoriteDialog		(WindowRef			inWindow,
										 SInt32				inDeltaSizeX,
										 SInt32				inDeltaSizeY,
										 SInt32				UNUSED_ARGUMENT(inData))
{
	DialogAdjust_BeginDialogItemAdjustment(GetDialogFromWindow(inWindow));
	{
		if (inDeltaSizeX)
		{
			// controls which are moved horizontally
			if (Localization_IsLeftToRight())
			{
				DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iNameNewSessionFavoriteCreateButton, inDeltaSizeX);
				DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iNameNewSessionFavoriteCancelButton, inDeltaSizeX);
			}
			else
			{
				DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iNameNewSessionFavoriteAppleGuideButton, inDeltaSizeX);
				DialogAdjust_AddDialogItem(kDialogItemAdjustmentMoveH, iNameNewSessionFavoriteFieldLabelText, inDeltaSizeX);
			}
			
			// controls which are resized horizontally
			DialogAdjust_AddDialogItem(kDialogItemAdjustmentResizeH, iNameNewSessionFavoriteNameField, inDeltaSizeX);
		}
	}
	DialogAdjust_EndAdjustment(inDeltaSizeX, inDeltaSizeY); // moves and resizes controls properly
}// adjustNameNewSessionFavoriteDialog

// BELOW IS REQUIRED NEWLINE TO END FILE
