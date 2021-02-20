/*!	\file UIStrings.h
	\brief An interface to retrieve localized strings intended
	for the User Interface, independently of their storage
	format on disk.
	
	Symbolic tags are given to each localized string.  If all
	code modules use these APIs to retrieve strings instead of,
	say, GetIndString(), then it is possible to hide the
	underlying string representation and move to new formats
	more easily (for example, a Mac OS X Localizable.strings
	file).
	
	Strings are grouped by user interface object, because:
	- string tables can be loaded as needed (for example, when
	  a new window opens)
	- string tables are less likely to change (a large file
	  will certainly change as strings are added, but a focused
	  file only changes if its underlying UI object is updated)
	- debugging is easier (if you open a dialog and see that
	  some of its strings aren’t localized, you can fix it by
	  editing just one file - and you know exactly which file!)
	
	Also, since on Mac OS X you can’t have a unique localized
	string value unless its key (English wording) is unique,
	the goal is to collect strings that are unlikely to contain
	any duplicates.  For example, if I stuck the "Preferences"
	menu command in the same file as the "Preferences" window
	title, these two strings would not be allowed to vary when
	translated into other languages.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <ResultCode.template.h>



#pragma mark Constants

typedef ResultCode< UInt16 >	UIStrings_Result;
UIStrings_Result const		kUIStrings_ResultOK(0);					//!< no error
UIStrings_Result const		kUIStrings_ResultNoSuchString(1);		//!< tag is invalid for given string category
UIStrings_Result const		kUIStrings_ResultCannotGetString(2);	//!< probably an OS error, the string cannot be retrieved

/*!
Alert Window String Table ("Alerts.strings")

Title strings, message text, and help text for all alerts (whether
modal dialog boxes or sheets).  Since button names are used elsewhere
besides in alerts and their key names tend to collide with those of
the title strings, they are in a separate Buttons table.
*/
enum UIStrings_AlertWindowCFString
{
	kUIStrings_AlertWindowANSIColorsResetPrimaryText	= 'ANSI',
	kUIStrings_AlertWindowCloseName						= 'ImmC',
	kUIStrings_AlertWindowCloseHelpText					= 'HCls',
	kUIStrings_AlertWindowClosePrimaryText				= 'PCls',
	kUIStrings_AlertWindowCommandFailedHelpText			= 'HCmd',
	kUIStrings_AlertWindowCommandFailedPrimaryText		= 'PCmd',
	kUIStrings_AlertWindowCopyToDefaultPrimaryText		= 'CpyD',
	kUIStrings_AlertWindowCopyToDefaultHelpText			= 'CpyH',
	kUIStrings_AlertWindowCFFileExistsPrimaryText		= 'PDup',
	kUIStrings_AlertWindowCFNoWritePermissionPrimaryText= 'PNWr',
	kUIStrings_AlertWindowDeleteCollectionPrimaryText	= 'DlCD',
	kUIStrings_AlertWindowDeleteCollectionHelpText		= 'DlCH',
	kUIStrings_AlertWindowExcessiveErrorsPrimaryText	= 'PMnE',
	kUIStrings_AlertWindowExcessiveErrorsHelpText		= 'HMnE',
	kUIStrings_AlertWindowGenericCannotUndoHelpText		= 'HNUn',
	kUIStrings_AlertWindowKillSessionName				= 'KWnT',
	kUIStrings_AlertWindowKillSessionHelpText			= 'HKWn',
	kUIStrings_AlertWindowKillSessionPrimaryText		= 'PKWn',
	kUIStrings_AlertWindowMacroExportNothingPrimaryText	= 'MENW',
	kUIStrings_AlertWindowMacroExportNothingHelpText	= 'MENH',
	kUIStrings_AlertWindowMacroImportWarningPrimaryText	= 'MIOW',
	kUIStrings_AlertWindowMacroImportWarningHelpText	= 'MIOH',
	kUIStrings_AlertWindowNotifyActivityPrimaryText		= 'PNtA',
	kUIStrings_AlertWindowNotifyActivityTitle			= 'TNtA',
	kUIStrings_AlertWindowNotifyInactivityPrimaryText	= 'PNtI',
	kUIStrings_AlertWindowNotifyInactivityTitle			= 'TNtI',
	kUIStrings_AlertWindowNotifyProcessDieTemplate		= 'PPDi',
	kUIStrings_AlertWindowNotifyProcessDieTitle			= 'TPDi',
	kUIStrings_AlertWindowNotifyProcessExitPrimaryText	= 'PPEx',
	kUIStrings_AlertWindowNotifyProcessExitTitle		= 'TPEx',
	kUIStrings_AlertWindowNotifyProcessSignalTemplate	= 'PPSg',
	kUIStrings_AlertWindowNotifySysExitUsageHelpText	= 'SE64',
	kUIStrings_AlertWindowNotifySysExitDataErrHelpText	= 'SE65',
	kUIStrings_AlertWindowNotifySysExitNoInputHelpText	= 'SE66',
	kUIStrings_AlertWindowNotifySysExitNoUserHelpText	= 'SE67',
	kUIStrings_AlertWindowNotifySysExitNoHostHelpText	= 'SE68',
	kUIStrings_AlertWindowNotifySysExitUnavailHelpText	= 'SE69',
	kUIStrings_AlertWindowNotifySysExitSoftwareHelpText	= 'SE70',
	kUIStrings_AlertWindowNotifySysExitOSErrHelpText	= 'SE71',
	kUIStrings_AlertWindowNotifySysExitOSFileHelpText	= 'SE72',
	kUIStrings_AlertWindowNotifySysExitCreateHelpText	= 'SE73',
	kUIStrings_AlertWindowNotifySysExitIOErrHelpText	= 'SE74',
	kUIStrings_AlertWindowNotifySysExitTempFailHelpText	= 'SE75',
	kUIStrings_AlertWindowNotifySysExitProtocolHelpText	= 'SE76',
	kUIStrings_AlertWindowNotifySysExitNoPermHelpText	= 'SE77',
	kUIStrings_AlertWindowNotifySysExitConfigHelpText	= 'SE78',
	kUIStrings_AlertWindowPasteLinesWarningName			= 'PstN',
	kUIStrings_AlertWindowPasteLinesWarningHelpText		= 'PstH',
	kUIStrings_AlertWindowPasteLinesWarningPrimaryText	= 'PstP',
	kUIStrings_AlertWindowRestartSessionName			= 'RWnT',
	kUIStrings_AlertWindowRestartSessionHelpText		= 'HRWn',
	kUIStrings_AlertWindowRestartSessionPrimaryText		= 'PRWn',
	kUIStrings_AlertWindowScriptErrorName				= 'Scpt',
	kUIStrings_AlertWindowScriptErrorHelpText			= 'HScp',
	kUIStrings_AlertWindowShowIPAddressesPrimaryText	= 'IPAd',
	kUIStrings_AlertWindowUpdateCheckHelpText			= 'HUpd',
	kUIStrings_AlertWindowUpdateCheckPrimaryText		= 'PUpd',
	kUIStrings_AlertWindowUpdateCheckTitle				= 'TUpd',
	kUIStrings_AlertWindowQuitName						= 'ImmQ',
	kUIStrings_AlertWindowQuitHelpText					= 'HQui',
	kUIStrings_AlertWindowQuitPrimaryText				= 'PQui'
};

/*!
Button Title String Table ("Buttons.strings")

Strings used as button titles, usually in multiple places.  Note that
the button strings for a specific window are likely to be stored in a
NIB for that window, so this tends to be used only for buttons created
on the fly (such as in alerts).
*/
enum UIStrings_ButtonCFString
{
	kUIStrings_ButtonCancel								= 'Cncl',
	kUIStrings_ButtonClose								= 'Clos',
	kUIStrings_ButtonContinue							= 'Cont',
	kUIStrings_ButtonDelete								= 'Dlte',
	kUIStrings_ButtonDiscardAll							= 'Disc',
	kUIStrings_ButtonDontSave							= 'DntS',
	kUIStrings_ButtonIgnore								= 'Ignr',
	kUIStrings_ButtonKill								= 'Kill',
	kUIStrings_ButtonMakeOneLine						= 'OneL',
	kUIStrings_ButtonNo									= ' No ',
	kUIStrings_ButtonOK									= ' OK ',
	kUIStrings_ButtonOverwrite							= 'Ovrw',
	kUIStrings_ButtonPasteAsIs							= 'Pste',
	kUIStrings_ButtonReset								= 'Rset',
	kUIStrings_ButtonRestart							= 'Rsrt',
	kUIStrings_ButtonQuit								= 'Quit',
	kUIStrings_ButtonReviewWithEllipsis					= 'Revu',
	kUIStrings_ButtonSave								= 'Save',
	kUIStrings_ButtonStartSession						= 'SSss',
	kUIStrings_ButtonYes								= 'Yes '
};

/*!
Clipboard Window String Table ("ClipboardWindow.strings")

Strings appearing in the window used by Show Clipboard.
*/
enum UIStrings_ClipboardWindowCFString
{
	kUIStrings_ClipboardWindowShowCommand				= 'ShwC',
	kUIStrings_ClipboardWindowHideCommand				= 'HidC',
	kUIStrings_ClipboardWindowIconName					= 'Icon',
	kUIStrings_ClipboardWindowLabelWidth				= 'LWid',
	kUIStrings_ClipboardWindowLabelHeight				= 'LHgt',
	kUIStrings_ClipboardWindowPixelDimensionTemplate	= 'DPix',
	kUIStrings_ClipboardWindowOnePixel					= '1Pix',
	kUIStrings_ClipboardWindowGenericKindText			= 'Text',
	kUIStrings_ClipboardWindowGenericKindImage			= 'Imag',
	kUIStrings_ClipboardWindowDataSizeTemplate			= 'DSiz',
	kUIStrings_ClipboardWindowDataSizeApproximately		= 'Aprx',
	kUIStrings_ClipboardWindowUnitsByte					= 'SzB1',
	kUIStrings_ClipboardWindowUnitsBytes				= 'SzBt',
	kUIStrings_ClipboardWindowUnitsK					= 'SzKB',
	kUIStrings_ClipboardWindowUnitsMB					= 'SzMB',
	kUIStrings_ClipboardWindowValueUnknown				= 'Unkn'
};

/*!
Contextual Menu Items String Table ("ContextualMenus.strings")

Strings used for commands in context-sensitive pop-up menus.
*/
enum UIStrings_ContextualMenuCFString
{
	kUIStrings_ContextualMenuArrangeAllInFront			= 'StkW',
	kUIStrings_ContextualMenuChangeBackground			= 'SBkg',
	kUIStrings_ContextualMenuCloseThisWindow			= 'Kill',
	kUIStrings_ContextualMenuCopyToClipboard			= 'Copy',
	kUIStrings_ContextualMenuCopyUsingTabsForSpaces		= 'CpyT',
	kUIStrings_ContextualMenuCustomFormat				= 'Font',
	kUIStrings_ContextualMenuCustomScreenDimensions		= 'ScnS',
	kUIStrings_ContextualMenuFindInThisWindow			= 'Find',
	kUIStrings_ContextualMenuFixCharacterTranslation	= 'FixT',
	kUIStrings_ContextualMenuFullScreenEnter			= 'EnFS',
	kUIStrings_ContextualMenuFullScreenExit				= 'ExFS',
	kUIStrings_ContextualMenuGroupTitleClipboardMacros	= 'GrMC',
	kUIStrings_ContextualMenuGroupTitleSelectionMacros	= 'GrMS',
	kUIStrings_ContextualMenuHideThisWindow				= 'Hide',
	kUIStrings_ContextualMenuMoveToNewWorkspace			= 'MTab',
	kUIStrings_ContextualMenuOpenThisResource			= 'OURL',
	kUIStrings_ContextualMenuPrintScreen				= 'PrSc',
	kUIStrings_ContextualMenuPrintSelectedCanvasRegion	= 'PrSv',
	kUIStrings_ContextualMenuPrintSelectedText			= 'PrSl',
	kUIStrings_ContextualMenuPasteText					= 'Pste',
	kUIStrings_ContextualMenuRenameThisWindow			= 'Renm',
	kUIStrings_ContextualMenuSaveSelectedText			= 'Save',
	kUIStrings_ContextualMenuSpeakSelectedText			= 'SpkS',
	kUIStrings_ContextualMenuStopSpeaking				= 'SpkE',
	kUIStrings_ContextualMenuShowCompletions			= 'SCmp',
	kUIStrings_ContextualMenuSpecialKeySequences		= 'Keys',
	kUIStrings_ContextualMenuZoomCanvas100Percent		= 'Z100',
};

/*!
File or Folder Names String Table ("FileOrFolderNames.strings")

The titles of special files or folders on disk; for example, used to
find preferences or error logs.
*/
enum UIStrings_FileOrFolderCFString
{
	kUIStrings_FileDefaultCaptureFile					= 'DefC',
	kUIStrings_FileDefaultExportPreferences				= 'DefP',
	kUIStrings_FileDefaultImageFile						= 'DefI',
	kUIStrings_FileDefaultMacroSet						= 'DefM',
	kUIStrings_FileDefaultSession						= 'DefS'
};

/*!
Help System String Table ("HelpSystem.strings")

These strings are used to interact with the Help Viewer.
*/
enum UIStrings_HelpSystemCFString
{
	kUIStrings_HelpSystemTopicHelpCreatingSessions		= 'Sess',
	kUIStrings_HelpSystemTopicHelpSearchingForText		= 'Find',
	kUIStrings_HelpSystemTopicHelpSettingTheScreenSize	= 'SSiz',
	kUIStrings_HelpSystemTopicHelpUsingTheCommandLine	= 'CmdL',
	kUIStrings_HelpSystemTopicHelpWithPreferences		= 'Pref',
	kUIStrings_HelpSystemTopicHelpWithScreenFormatting	= 'Font',
	kUIStrings_HelpSystemTopicHelpWithTerminalSettings	= 'Term'
};

/*!
Preferences Window String Table ("PreferencesWindow.strings")

Identifies localizable strings used in preferences panels.
*/
enum UIStrings_PreferencesWindowCFString
{
	kUIStrings_PreferencesWindowAddToFavoritesButton		= 'AFBt',
	kUIStrings_PreferencesWindowDefaultFavoriteName			= 'DefF',
	kUIStrings_PreferencesWindowExportCopyDefaults			= 'ExCD',
	kUIStrings_PreferencesWindowExportCopyDefaultsHelpText	= 'ECDH',
	kUIStrings_PreferencesWindowListHeaderNumber			= 'Numb'
};

/*!
Session Info Window String Table ("SessionInfoWindow.strings")

Strings that appear in the Session Info (status) window.
*/
enum UIStrings_SessionInfoWindowCFString
{
	kUIStrings_SessionInfoWindowIconName				= 'Icon',
	kUIStrings_SessionInfoWindowStatusProcessNewborn	= 'Newb',
	kUIStrings_SessionInfoWindowStatusProcessRunning	= 'Runn',
	kUIStrings_SessionInfoWindowStatusProcessTerminated	= 'Dead',
	kUIStrings_SessionInfoWindowStatusTerminatedAtTime	= 'DTim'
};

/*!
System Dialogs String Table ("SystemDialogs.strings")

Strings used with open and save dialogs, color pickers, etc.
*/
enum UIStrings_SystemDialogCFString
{
	kUIStrings_SystemDialogPromptCaptureToFile		= 'PmCF',
	kUIStrings_SystemDialogPromptOpenPrefs			= 'PmOP',
	kUIStrings_SystemDialogPromptOpenSession		= 'PmOS',
	kUIStrings_SystemDialogPromptSavePrefs			= 'PmSP',
	kUIStrings_SystemDialogPromptSaveSelectedImage	= 'PmSI',
	kUIStrings_SystemDialogPromptSaveSelectedText	= 'PmST',
	kUIStrings_SystemDialogPromptSaveSession		= 'PmSS'
};

/*!
Terminal String Table ("Terminal.strings")

Strings used in terminal windows.
*/
enum UIStrings_TerminalCFString
{
	kUIStrings_TerminalDynamicResizeFontSize				= 'DRFS',
	kUIStrings_TerminalDynamicResizeWidthHeight				= 'DRWH',
	kUIStrings_TerminalInterruptProcess						= 'Intr',
	kUIStrings_TerminalNewCommandsKeyCharacter				= 'NewK',
	kUIStrings_TerminalPrintFromTerminalJobTitle			= 'PTrm',
	kUIStrings_TerminalPrintScreenJobTitle					= 'PScr',
	kUIStrings_TerminalPrintSelectionJobTitle				= 'PSel',
	kUIStrings_TerminalResumeOutput							= 'Resu',
	kUIStrings_TerminalSuspendOutput						= 'Susp',
	kUIStrings_TerminalSearchNothingFound					= 'NotF',
	kUIStrings_TerminalSearchNumberOfMatches				= 'NumM',
	kUIStrings_TerminalVectorGraphicsRedirect				= 'Vect',
};

/*!
Undo String Table ("Undo.strings")

Strings used to describe reversible actions as menu commands.
*/
enum UIStrings_UndoCFString
{
	kUIStrings_UndoActionNameDimensionChanges	= 'UndD',
	kUIStrings_UndoActionNameFormatChanges		= 'UndF'
};



#pragma mark Public Methods

//!\name Retrieving Strings
//@{

UIStrings_Result
	UIStrings_Copy						(UIStrings_AlertWindowCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_ButtonCFString						inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_ClipboardWindowCFString				inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_ContextualMenuCFString				inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_FileOrFolderCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_HelpSystemCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_PreferencesWindowCFString			inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_SessionInfoWindowCFString			inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_SystemDialogCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_TerminalCFString						inWhichString,
										 CFStringRef&									outString);

UIStrings_Result
	UIStrings_Copy						(UIStrings_UndoCFString							inWhichString,
										 CFStringRef&									outString);

/*!
Invokes any UIStrings_Copy() method above but returns nullptr
for any error (otherwise returning a new string).

(4.1)
*/
template < typename string_enum >
CFStringRef
	UIStrings_ReturnCopy				(string_enum									inWhichString)
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			result = nullptr;
		
		
		stringResult = UIStrings_Copy(inWhichString, result);
		if (false == stringResult.ok())
		{
			result = nullptr;
		}
		return result;
	}

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
