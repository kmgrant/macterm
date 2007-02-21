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

#ifndef __UISTRINGS__
#define __UISTRINGS__

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <ResultCode.template.h>



#pragma mark Constants

typedef ResultCode< UInt16 >	UIStrings_ResultCode;
UIStrings_ResultCode const		kUIStrings_ResultCodeSuccess(0);			//!< no error
UIStrings_ResultCode const		kUIStrings_ResultCodeNoSuchString(1);		//!< tag is invalid for given string category
UIStrings_ResultCode const		kUIStrings_ResultCodeCannotGetString(2);	//!< probably an OS error, the string cannot be retrieved

/*!
Alert Window String Table ("Alerts.strings")

Title strings, message text, and help text for all alerts (whether
modal dialog boxes or sheets).  Since button names are used elsewhere
besides in alerts and their key names tend to collide with those of
the title strings, they are in a separate Buttons table.
*/
enum UIStrings_AlertWindowCFString
{
	kUIStrings_AlertWindowCloseName						= 'ImmC',
	kUIStrings_AlertWindowCommandFailedHelpText			= 'HCmd',
	kUIStrings_AlertWindowCommandFailedPrimaryText		= 'PCmd',
	kUIStrings_AlertWindowConnectionOpeningFailedName	= 'Xcxn',
	kUIStrings_AlertWindowMacroExportNothingPrimaryText	= 'MENW',
	kUIStrings_AlertWindowMacroExportNothingHelpText	= 'MENH',
	kUIStrings_AlertWindowMacroImportWarningPrimaryText	= 'MIOW',
	kUIStrings_AlertWindowMacroImportWarningHelpText	= 'MIOH',
	kUIStrings_AlertWindowRuntimeExceptionName			= 'RTEx',
	kUIStrings_AlertWindowScriptErrorName				= 'Scpt',
	kUIStrings_AlertWindowScriptErrorHelpText			= 'HScp',
	kUIStrings_AlertWindowShowIPAddressesPrimaryText	= 'IPAd',
	kUIStrings_AlertWindowStartupErrorName				= 'NoGo',
	kUIStrings_AlertWindowUpdateAvailableName			= 'UpdA',
	kUIStrings_AlertWindowUpdateAvailableHelpText		= 'UpdH',
	kUIStrings_AlertWindowUpdateAvailablePrimaryText	= 'UpdP',
	kUIStrings_AlertWindowUpdateCheckErrorPrimaryText	= 'UpCE',
	kUIStrings_AlertWindowUpToDateName					= 'UpTD',
	kUIStrings_AlertWindowUpToDatePrimaryText			= 'UpTP',
	kUIStrings_AlertWindowQuitName						= 'ImmQ'
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
	kUIStrings_ButtonCopyToClipboard					= 'Copy',
	kUIStrings_ButtonDiscardAll							= 'Disc',
	kUIStrings_ButtonDontSave							= 'Kill',
	kUIStrings_ButtonEditTextArrowsAccessibilityDesc	= 'ETAA',
	kUIStrings_ButtonEditTextHistoryAccessibilityDesc	= 'HiMA',
	kUIStrings_ButtonHelpAccessibilityDesc				= 'HlpA',
	kUIStrings_ButtonNo									= ' No ',
	kUIStrings_ButtonOK									= ' OK ',
	kUIStrings_ButtonOpenMacroEditor					= 'OMcE',
	kUIStrings_ButtonPopUpMenuArrowsAccessibilityDesc	= 'MnAA',
	kUIStrings_ButtonQuit								= 'Quit',
	kUIStrings_ButtonReviewWithEllipsis					= 'Revu',
	kUIStrings_ButtonSave								= 'Save',
	kUIStrings_ButtonStop								= 'Stop',
	kUIStrings_ButtonVisitMainWebSite					= 'VWeb',
	kUIStrings_ButtonYes								= 'Yes '
};

/*!
Clipboard Window String Table ("ClipboardWindow.strings")

Strings appearing in the window used by Show Clipboard.
*/
enum UIStrings_ClipboardWindowCFString
{
	kUIStrings_ClipboardWindowIconName					= 'Icon',
	kUIStrings_ClipboardWindowDisplaySizePercentage		= 'Pcnt',
	kUIStrings_ClipboardWindowDescriptionEmpty			= 'Empt',
	kUIStrings_ClipboardWindowDescriptionTemplate		= 'Desc',
	kUIStrings_ClipboardWindowContentTypeText			= 'Text',
	kUIStrings_ClipboardWindowContentTypeUnicodeText	= 'Unic',
	kUIStrings_ClipboardWindowContentTypePicture		= 'Pict',
	kUIStrings_ClipboardWindowContentTypeUnknown		= 'Unkn',
	kUIStrings_ClipboardWindowDescriptionApproximately	= 'Aprx',
	kUIStrings_ClipboardWindowUnitsByte					= 'SzB1',
	kUIStrings_ClipboardWindowUnitsBytes				= 'SzBt',
	kUIStrings_ClipboardWindowUnitsK					= 'SzKB',
	kUIStrings_ClipboardWindowUnitsMB					= 'SzMB'
};

/*!
Command Line String Table ("CommandLine.strings")

Strings used by the floating command line window.
*/
enum UIStrings_CommandLineCFString
{
	kUIStrings_CommandLineHelpTextCommandArgumentTemplate	= 'HCAT',
	kUIStrings_CommandLineHelpTextCommandTemplate			= 'HCTm',
	kUIStrings_CommandLineHelpTextDefault					= 'HDef',
	kUIStrings_CommandLineHelpTextFreeInput					= 'HFIP',
	kUIStrings_CommandLineHistoryMenuAccessibilityDesc		= 'HiMA'
};

/*!
Contextual Menu Items String Table ("ContextualMenus.strings")

Strings used for commands in context-sensitive pop-up menus.
*/
enum UIStrings_ContextualMenuCFString
{
	kUIStrings_ContextualMenuArrangeAllInFront			= 'StkW',
	kUIStrings_ContextualMenuCloseThisWindow			= 'Kill',
	kUIStrings_ContextualMenuCopyToClipboard			= 'Copy',
	kUIStrings_ContextualMenuCopyUsingTabsForSpaces		= 'CpyT',
	kUIStrings_ContextualMenuCustomFormat				= 'Font',
	kUIStrings_ContextualMenuCustomScreenDimensions		= 'ScnS',
	kUIStrings_ContextualMenuFindInThisWindow			= 'Find',
	kUIStrings_ContextualMenuFixCharacterTranslation	= 'FixT',
	kUIStrings_ContextualMenuHideThisWindow				= 'Hide',
	kUIStrings_ContextualMenuOpenThisResource			= 'OURL',
	kUIStrings_ContextualMenuPasteText					= 'Pste',
	kUIStrings_ContextualMenuPrintSelectionNow			= 'Prn1',
	kUIStrings_ContextualMenuRenameThisWindow			= 'Renm',
	kUIStrings_ContextualMenuSaveSelectedText			= 'Save',
	kUIStrings_ContextualMenuSpeakSelectedText			= 'SpkS',
	kUIStrings_ContextualMenuSpecialKeySequences		= 'Keys'
};

/*!
File or Folder Names String Table ("FileOrFolderNames.strings")

The titles of special files or folders on disk; for example, used to
find preferences or error logs.
*/
enum UIStrings_FileOrFolderCFString
{
	kUIStrings_FileDefaultCaptureFile					= 'DefC',
	kUIStrings_FileDefaultMacroSet						= 'DefM',
	kUIStrings_FileDefaultSession						= 'DefS',
	kUIStrings_FileNameDockTileAttentionPicture			= '!Pic',
	kUIStrings_FileNameDockTileAttentionMask			= '!Msk',
	kUIStrings_FileNameSplashScreenPicture				= 'Titl',
	kUIStrings_FileNameToolbarPoofFrame1Picture			= 'Pf1P',
	kUIStrings_FileNameToolbarPoofFrame1Mask			= 'Pf1M',
	kUIStrings_FileNameToolbarPoofFrame2Picture			= 'Pf2P',
	kUIStrings_FileNameToolbarPoofFrame2Mask			= 'Pf2M',
	kUIStrings_FileNameToolbarPoofFrame3Picture			= 'Pf3P',
	kUIStrings_FileNameToolbarPoofFrame3Mask			= 'Pf3M',
	kUIStrings_FileNameToolbarPoofFrame4Picture			= 'Pf4P',
	kUIStrings_FileNameToolbarPoofFrame4Mask			= 'Pf4M',
	kUIStrings_FileNameToolbarPoofFrame5Picture			= 'Pf5P',
	kUIStrings_FileNameToolbarPoofFrame5Mask			= 'Pf5M',
	kUIStrings_FolderNameApplicationFavorites			= 'AFav',
	kUIStrings_FolderNameApplicationFavoritesMacros		= 'AFFM',
	kUIStrings_FolderNameApplicationFavoritesProxies	= 'AFPx',
	kUIStrings_FolderNameApplicationFavoritesSessions	= 'AFSn',
	kUIStrings_FolderNameApplicationFavoritesTerminals	= 'AFTm',
	kUIStrings_FolderNameApplicationPreferences			= 'APrf',
	kUIStrings_FolderNameApplicationRecentSessions		= 'ARcS',
	kUIStrings_FolderNameApplicationScriptsMenuItems	= 'AScM',
	kUIStrings_FolderNameApplicationStartupItems		= 'AStI',
	kUIStrings_FolderNameHomeLibraryLogs				= 'Logs'
};

/*!
Font List Rebuild Progress Window String Table ("FontListProgressWindow.strings")

Strings used in the progress window that appears while the
contents of the Fonts menu are being determined.
*/
enum UIStrings_FontListProgressWindowCFString
{
	kUIStrings_FontListProgressWindowIconName			= 'Icon'
};

/*!
Help System String Table ("HelpSystem.strings")

These strings are used to interact with the Help Viewer.
*/
enum UIStrings_HelpSystemCFString
{
	kUIStrings_HelpSystemName							= 'Name',
	kUIStrings_HelpSystemContextualHelpCommandName		= 'CHlp',
	kUIStrings_HelpSystemShowTagsCommandName			= 'STag',
	kUIStrings_HelpSystemHideTagsCommandName			= 'HTag',
	kUIStrings_HelpSystemTopicHelpCreatingSessions		= 'Sess',
	kUIStrings_HelpSystemTopicHelpSearchingForText		= 'Find',
	kUIStrings_HelpSystemTopicHelpSettingKeyMappings	= 'Keys',
	kUIStrings_HelpSystemTopicHelpSettingTheScreenSize	= 'SSiz',
	kUIStrings_HelpSystemTopicHelpUsingMacros			= 'Mcro',
	kUIStrings_HelpSystemTopicHelpUsingTheCommandLine	= 'CmdL',
	kUIStrings_HelpSystemTopicHelpWithKioskSetup		= 'Kios',
	kUIStrings_HelpSystemTopicHelpWithPreferences		= 'Pref',
	kUIStrings_HelpSystemTopicHelpWithScreenFormatting	= 'Font',
	kUIStrings_HelpSystemTopicHelpWithSessionFavorites	= 'SFav',
	kUIStrings_HelpSystemTopicHelpWithTerminalSettings	= 'Term'
};

/*!
Macro Setup Window String Table ("MacroSetupWindow.strings")

These strings are used in the Macro Setup window.  Note
that most strings are specified in the NIB, the list
below contains strings that cannot be specified there.
*/
enum UIStrings_MacroSetupWindowCFString
{
	kUIStrings_MacroSetupWindowSetName1					= 'McS1',
	kUIStrings_MacroSetupWindowSetName2					= 'McS2',
	kUIStrings_MacroSetupWindowSetName3					= 'McS3',
	kUIStrings_MacroSetupWindowSetName4					= 'McS4',
	kUIStrings_MacroSetupWindowSetName5					= 'McS5'
};

// see "UIStrings_PrefsWindow.h" for declaration of "UIStrings_PreferencesWindowCFString"

/*!
Scripts Menu Rebuild Progress Window String Table ("ScriptsMenuProgressWindow.strings")

Strings used in the progress window that appears while the
contents of the Scripts menu are being determined.
*/
enum UIStrings_ScriptsMenuProgressWindowCFString
{
	kUIStrings_ScriptsMenuProgressWindowIconName		= 'Icon'
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
	kUIStrings_SystemDialogPromptCaptureToFile	= 'PmCF',
	kUIStrings_SystemDialogPromptOpenSession	= 'PmOS',
	kUIStrings_SystemDialogPromptSaveSession	= 'PmSS',
	kUIStrings_SystemDialogPromptPickColor		= 'PmPC',
	kUIStrings_SystemDialogTitleOpenSession		= 'TtOS'
};

/*!
Toolbar Item String Table ("ToolbarItems.strings")

Strings used in window toolbars, typically accompanied by icons.
*/
enum UIStrings_ToolbarItemCFString
{
	kUIStrings_ToolbarItemNewSessionDefault		= 'NewD',
	kUIStrings_ToolbarItemNewSessionLoginShell	= 'NewL',
	kUIStrings_ToolbarItemNewSessionShell		= 'NewS',
	kUIStrings_ToolbarItemSearch				= 'Find',
	kUIStrings_ToolbarItemTerminalLED1			= 'LED1',
	kUIStrings_ToolbarItemTerminalLED2			= 'LED2',
	kUIStrings_ToolbarItemTerminalLED3			= 'LED3',
	kUIStrings_ToolbarItemTerminalLED4			= 'LED4'
};

/*!
Terminal String Table ("Terminal.strings")

Strings used in terminal windows.
*/
enum UIStrings_TerminalCFString
{
	kUIStrings_TerminalInterruptProcess			= 'Intr',
	kUIStrings_TerminalResumeOutput				= 'Resu',
	kUIStrings_TerminalSuspendOutput			= 'Susp'
};

/*!
Undo String Table ("Undo.strings")

Strings used to describe reversible actions as menu commands.
*/
enum UIStrings_UndoCFString
{
	kUIStrings_UndoDefault						= 'Undo',
	kUIStrings_RedoDefault						= 'Redo',
	kUIStrings_UndoDimensionChanges				= 'UndD',
	kUIStrings_RedoDimensionChanges				= 'RedD',
	kUIStrings_UndoFormatChanges				= 'UndF',
	kUIStrings_RedoFormatChanges				= 'RedF'
};

/*!
Generic description of a class of strings.  This is not
normally used, because you can just refer to specific
strings above; however, UIStrings_CopyRandom() uses it
to return any string out of a table.
*/
enum UIStrings_StringClass
{
	kUIStrings_StringClassSplashScreen			= 'SpSc'
};



#pragma mark Public Methods

//!\name Retrieving Strings
//@{

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_AlertWindowCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_ButtonCFString						inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_ClipboardWindowCFString				inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_CommandLineCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_ContextualMenuCFString				inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_FileOrFolderCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_FontListProgressWindowCFString		inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_HelpSystemCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_MacroSetupWindowCFString				inWhichString,
										 CFStringRef&									outString);

// see "UIStrings_PrefsWindow.h" for a declaration that accepts "UIStrings_PreferencesWindowCFString"

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_ScriptsMenuProgressWindowCFString	inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_SessionInfoWindowCFString			inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_SystemDialogCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_TerminalCFString						inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_ToolbarItemCFString					inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_Copy						(UIStrings_UndoCFString							inWhichString,
										 CFStringRef&									outString);

UIStrings_ResultCode
	UIStrings_CopyRandom				(UIStrings_StringClass							inWhichStringClass,
										 CFStringRef&									outString);

// CALLS FSMakeFSRefUnicode()
OSStatus
	UIStrings_MakeFSRef					(FSRef const*									inParentRefPtr,
										 UIStrings_FileOrFolderCFString					inWhichString,
										 FSRef*											outFSRefPtr);

// CALLS FSMakeFSSpec()
OSStatus
	UIStrings_MakeFSSpec				(SInt16											inVRefNum,
										 SInt32											inDirID,
										 UIStrings_FileOrFolderCFString					inWhichString,
										 FSSpec*										outFSSpecPtr);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
