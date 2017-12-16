/*!	\file Commands.h
	\brief A list of all command IDs, and a mechanism for
	invoking the application’s main features.
	
	A command is a series of primitive actions that leads to a
	result; usually, there is a menu item for each command (but
	this is not required; for example, a command might be used
	to operate a toolbar item).
	
	This TEMPORARILY serves as a single point of binding for
	Cocoa-based menu commands and other interface elements.  It
	is necessary because Carbon-based windows and Cocoa-based
	windows DO NOT mix all that well, and commands generally
	will not work in the usual Cocoa first-responder way.  The
	temporary work-around is to implement most commands here,
	and bind only to these classes (not the first responder),
	so that they can handle Carbon/Cocoa details properly.
	When ALL Carbon-based windows are finally gone, it will
	then be possible to migrate the various methods into the
	most appropriate classes/files, e.g. for specific windows.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
typedef void* SEL;
class NSMenu;
class NSMenuItem;
#endif
#include <CoreServices/CoreServices.h>

// library includes
#include <ListenerModel.h>
#include <ResultCode.template.h>

// application includes
#include <QuillsPrefs.h>



#pragma mark Constants

typedef ResultCode< UInt16 >	Commands_Result;
Commands_Result const	kCommands_ResultOK(0);				//!< no error
Commands_Result const	kCommands_ResultParameterError(1);	//!< bad input - for example, invalid listener type

enum Commands_NameType
{
	kCommands_NameTypeDefault		= 0,	//!< the name of the command in normal context (such as in a menu item)
	kCommands_NameTypeShort			= 1		//!< a short version of the name (such as in a toolbar item)
};

/*!
Command IDs

WARNING:	Although all source code should refer to these
			IDs only via the constants below, a number of
			Interface Builder NIB files for MacTerm will
			refer to these by value.  Do not arbitrarily
			change command IDs without realizing all the
			places they may be used.

These must all be unique, and Apple reserves any IDs whose
letters are all-lowercase.

Where appropriate, command IDs for actions that the OS is
familiar with - such as creating files, closing windows,
printing documents, etc. - match the IDs that the OS itself
uses.  This has a number of advantages.  For one, a lot of
things “just work right” as a result (the OS knows when to
disable the Minimize Window menu command, for example, since
it can tell whether the frontmost window has a minimization
button).  Also, using standard commands means that MacTerm
will receive events properly when future OS capabilities are
added (for example, window content controls might gain
contextual menus that can automatically execute the right
MacTerm commands, such as Cut, Copy, Paste or Undo).
*/

// Application (Apple) menu
// WARNING: These are referenced by value in the MainMenus.nib and
//          MenuForDockIcon.nib files!
#define kCommandAboutThisApplication			kHICommandAbout
#define kCommandFullScreenToggle				'Kios'
#define kCommandCheckForUpdates					'ChUp'
#define kCommandURLHomePage						'.com'
#define kCommandURLAuthorMail					'Mail'

// File menu
#define kCommandNewSessionDefaultFavorite		'NSDF'
#define kCommandNewSessionLoginShell			'NLgS'
#define kCommandNewSessionShell					'NShS'
#define kCommandNewSessionDialog				'NSDg'
#define kCommandRestoreWorkspaceDefaultFavorite	'RWDF'
#define kCommandOpenSession						kHICommandOpen
#define kCommandCloseConnection					kHICommandClose
#define kCommandSaveSession						kHICommandSaveAs
#define kCommandNewDuplicateSession				'NewD'
#define kCommandKillProcessesKeepWindow			'Kill'
#define kCommandRestartSession					'RSsn'
#define kCommandHandleURL						'HURL'
#define kCommandSaveSelection					'SvSl'
#define kCommandCaptureToFile					'Capt'
#define kCommandEndCaptureToFile				'CapE'
#define kCommandPrint							kHICommandPrint
#define kCommandPrintScreen						'PrSc'

// Edit menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandUndo							kHICommandUndo
#define kCommandRedo							kHICommandRedo
#define kCommandCut								kHICommandCut
#define kCommandCopy							kHICommandCopy
#define kCommandCopyTable						'CpyT'
#define kCommandCopyAndPaste					'CpPs'
#define kCommandPaste							kHICommandPaste
#define kCommandClear							kHICommandClear
#define kCommandFind							'Find'
#define kCommandFindAgain						'FndN'
#define kCommandFindPrevious					'FndP'
#define kCommandShowCompletions					'SCmp'
#define kCommandFindCursor						'FndC'
#define kCommandSelectAll						kHICommandSelectAll
#define kCommandSelectAllWithScrollback			'SlSb'
#define kCommandSelectNothing					'Sel0'
#define kCommandShowClipboard					'ShCl'
#define kCommandHideClipboard					'HiCl'

// View menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandWiderScreen						'WidI'
#define kCommandNarrowerScreen					'WidD'
#define kCommandTallerScreen					'HgtI'
#define kCommandShorterScreen					'HgtD'
#define kCommandSmallScreen						'StdW'
#define kCommandTallScreen						'Tall'
#define kCommandLargeScreen						'Wide'
#define kCommandSetScreenSize					'SSiz'
#define kCommandBiggerText						'FSzB'
#define kCommandZoomMaximumSize					'ZmMx'
#define kCommandSmallerText						'FSzS'
#define kCommandFormatDefault					'FmtD'
#define kCommandFormatByFavoriteName			'FFav'
#define kCommandFormat							'Text'
#define	kCommandTEKPageCommand					'TEKP'
#define	kCommandTEKPageClearsScreen				'TEKC'

// Terminal menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandSuspendNetwork					'Susp'
#define kCommandSendInterruptProcess			'IP  '
#define kCommandBellEnabled						'Bell'
#define kCommandEcho							'Echo'
#define kCommandWrapMode						'Wrap'
#define kCommandClearScreenSavesLines			'CSSL'
#define kCommandJumpScrolling					'Jump'
#define kCommandWatchNothing					'WOff'
#define kCommandWatchForActivity				'Notf'
#define kCommandWatchForInactivity				'Idle'
#define kCommandTransmitOnInactivity			'KAlv'
#define kCommandSpeechEnabled					'Talk'
#define kCommandClearEntireScrollback			'ClSB'
#define kCommandResetTerminal					'RTrm'

// Map menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandDeletePressSendsBackspace		'DBks'	// multiple interfaces
#define kCommandDeletePressSendsDelete			'DDel'	// multiple interfaces
#define kCommandEmacsArrowMapping				'Emac'	// multiple interfaces
#define kCommandLocalPageUpDown					'LcPg'
#define kCommandSetKeys							'SetK'
#define kCommandTranslationTableDefault			'XltD'
#define kCommandTranslationTableByFavoriteName	'XFav'
#define kCommandSetTranslationTable				'Xlat'

// Window menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandMinimizeWindow					kHICommandMinimizeWindow
#define kCommandZoomWindow						kHICommandZoomWindow
#define kCommandMaximizeWindow					'Maxm'
#define kCommandChangeWindowTitle				'WinT'
#define kCommandHideFrontWindow					'HdFW'
#define kCommandHideOtherWindows				'HdOW'
#define kCommandShowAllHiddenWindows			'ShAW'
#define kCommandStackWindows					'StkW'
#define kCommandNextWindow						'NxtW'
#define kCommandNextWindowHideCurrent			'NxWH'
#define kCommandPreviousWindow					'PrvW'
#define kCommandPreviousWindowHideCurrent		'PrWH'
#define kCommandShowConnectionStatus			'ShCS'
#define kCommandHideConnectionStatus			'HiCS'
#define kCommandShowCommandLine					'ShCL'
#define kCommandShowNetworkNumbers				'CIPn'
#define kCommandShowControlKeys					'ShCK'
#define kCommandShowFunction					'ShFn'
#define kCommandShowKeypad						'ShKp'
#define kCommandSessionByWindowName				'Wind'

// Debug menu
#define kCommandDebuggingOptions				'Dbug'

// Help menu
#define kCommandMainHelp						kHICommandAppHelp
#define kCommandContextSensitiveHelp			'?Ctx'
#define kCommandShowHelpTags					'STag'
#define kCommandHideHelpTags					'HTag'

// color box
#define kCommandColorCursorBackground			'Curs'
#define kCommandColorMatteBackground			'Mtte'
#define kCommandColorBlinkingForeground			'BlTx'
#define kCommandColorBlinkingBackground			'BlBk'
#define kCommandColorBoldForeground				'BTxt'
#define kCommandColorBoldBackground				'BBkg'
#define kCommandColorNormalForeground			'NTxt'
#define kCommandColorNormalBackground			'NBkg'
#define kCommandColorBlack						'Cblk'
#define kCommandColorBlackEmphasized			'CBlk'
#define kCommandColorRed						'Cred'
#define kCommandColorRedEmphasized				'CRed'
#define kCommandColorGreen						'Cgrn'
#define kCommandColorGreenEmphasized			'CGrn'
#define kCommandColorYellow						'Cyel'
#define kCommandColorYellowEmphasized			'CYel'
#define kCommandColorBlue						'Cblu'
#define kCommandColorBlueEmphasized				'CBlu'
#define kCommandColorMagenta					'Cmag'
#define kCommandColorMagentaEmphasized			'CMag'
#define kCommandColorCyan						'Ccyn'
#define kCommandColorCyanEmphasized				'CCyn'
#define kCommandColorWhite						'Cwht'
#define kCommandColorWhiteEmphasized			'CWht'

// Keypad buttons
#define kCommandKeypadControlAtSign				'CK^@' //!< ASCII 0 (NULL)
#define kCommandKeypadControlA					'CK^A' //!< ASCII 1
#define kCommandKeypadControlB					'CK^B' //!< ASCII 2
#define kCommandKeypadControlC					'CK^C' //!< ASCII 3
#define kCommandKeypadControlD					'CK^D' //!< ASCII 4
#define kCommandKeypadControlE					'CK^E' //!< ASCII 5
#define kCommandKeypadControlF					'CK^F' //!< ASCII 6
#define kCommandKeypadControlG					'CK^G' //!< ASCII 7 (BELL)
#define kCommandKeypadControlH					'CK^H' //!< ASCII 8
#define kCommandKeypadControlI					'CK^I' //!< ASCII 9 (TAB)
#define kCommandKeypadControlJ					'CK^J' //!< ASCII 10
#define kCommandKeypadControlK					'CK^K' //!< ASCII 11
#define kCommandKeypadControlL					'CK^L' //!< ASCII 12
#define kCommandKeypadControlM					'CK^M' //!< ASCII 13 (CR)
#define kCommandKeypadControlN					'CK^N' //!< ASCII 14
#define kCommandKeypadControlO					'CK^O' //!< ASCII 15
#define kCommandKeypadControlP					'CK^P' //!< ASCII 16
#define kCommandKeypadControlQ					'CK^Q' //!< ASCII 17
#define kCommandKeypadControlR					'CK^R' //!< ASCII 18
#define kCommandKeypadControlS					'CK^S' //!< ASCII 19
#define kCommandKeypadControlT					'CK^T' //!< ASCII 20
#define kCommandKeypadControlU					'CK^U' //!< ASCII 21
#define kCommandKeypadControlV					'CK^V' //!< ASCII 22
#define kCommandKeypadControlW					'CK^W' //!< ASCII 23
#define kCommandKeypadControlX					'CK^X' //!< ASCII 24
#define kCommandKeypadControlY					'CK^Y' //!< ASCII 25
#define kCommandKeypadControlZ					'CK^Z' //!< ASCII 26
#define kCommandKeypadControlLeftSquareBracket	'CK^[' //!< ASCII 27 (ESC)
#define kCommandKeypadControlBackslash			'CK^\\' //!< ASCII 28
#define kCommandKeypadControlRightSquareBracket	'CK^]' //!< ASCII 29
#define kCommandKeypadControlCaret				'CK^^' //!< ASCII 30
#define kCommandKeypadControlUnderscore			'CK^_' //!< ASCII 31
#define kCommandKeypadFunction1					'VF1 '
#define kCommandKeypadFunction2					'VF2 '
#define kCommandKeypadFunction3					'VF3 '
#define kCommandKeypadFunction4					'VF4 '
#define kCommandKeypadFunction5					'VF5 '
#define kCommandKeypadFunction6					'VF6 '
#define kCommandKeypadFunction7					'VF7 '
#define kCommandKeypadFunction8					'VF8 '
#define kCommandKeypadFunction9					'VF9 '
#define kCommandKeypadFunction10				'VF10'
#define kCommandKeypadFunction11				'VF11'
#define kCommandKeypadFunction12				'VF12'
#define kCommandKeypadFunction13				'VF13'
#define kCommandKeypadFunction14				'VF14'
#define kCommandKeypadFunction15				'VF15' //!< “help”
#define kCommandKeypadFunction16				'VF16' //!< “do”
#define kCommandKeypadFunction17				'VF17'
#define kCommandKeypadFunction18				'VF18'
#define kCommandKeypadFunction19				'VF19'
#define kCommandKeypadFunction20				'VF20'
#define kCommandKeypadFind						'KFnd'
#define kCommandKeypadInsert					'KIns'
#define kCommandKeypadDelete					'KDel'
#define kCommandKeypadSelect					'KSel'
#define kCommandKeypadPageUp					'KPgU'
#define kCommandKeypadPageDown					'KPgD'
#define kCommandKeypadLeftArrow					'KALt'
#define kCommandKeypadUpArrow					'KAUp'
#define kCommandKeypadDownArrow					'KADn'
#define kCommandKeypadRightArrow				'KARt'
#define kCommandKeypadProgrammableFunction1		'KPF1'
#define kCommandKeypadProgrammableFunction2		'KPF2'
#define kCommandKeypadProgrammableFunction3		'KPF3'
#define kCommandKeypadProgrammableFunction4		'KPF4'
#define kCommandKeypad0							'KNm0'
#define kCommandKeypad1							'KNm1'
#define kCommandKeypad2							'KNm2'
#define kCommandKeypad3							'KNm3'
#define kCommandKeypad4							'KNm4'
#define kCommandKeypad5							'KNm5'
#define kCommandKeypad6							'KNm6'
#define kCommandKeypad7							'KNm7'
#define kCommandKeypad8							'KNm8'
#define kCommandKeypad9							'KNm9'
#define kCommandKeypadPeriod					'KPrd'
#define kCommandKeypadComma						'KCom'
#define kCommandKeypadDash						'KDsh'
#define kCommandKeypadEnter						'KEnt'

// terminal view page control
#define kCommandTerminalViewPageUp				'TVPU'
#define kCommandTerminalViewPageDown			'TVPD'
#define kCommandTerminalViewHome				'TVPH'
#define kCommandTerminalViewEnd					'TVPE'

// commands currently used only in dialogs
#define kCommandAlertOtherButton				'Othr'		// alerts
#define kCommandCreditsAndLicenseInfo			'Cred'		// about box
#define kCommandEditFontAndSize					'EdFS'		// multiple interfaces
#define kCommandEditBackupFont					'EdBF'		// multiple interfaces
#define kCommandUseBackupFont					'XUBF'		// multiple interfaces
#define kCommandShowProtocolOptions				'POpt'		// multiple interfaces
#define kCommandLookUpSelectedHostName			'Look'		// multiple interfaces
#define kCommandCopyLogInShellCommandLine		'CmLS'		// multiple interfaces
#define kCommandCopyShellCommandLine			'CmSh'		// multiple interfaces
#define kCommandCopySessionDefaultCommandLine	'CmDf'		// multiple interfaces
#define kCommandCopySessionFavoriteCommandLine	'CmFv'		// multiple interfaces
#define kCommandEditCommandLine					'ECmd'		// multiple interfaces
#define kCommandTerminalDefault					'TrmD'		// multiple interfaces
#define kCommandTerminalByFavoriteName			'TFav'		// multiple interfaces
#define kCommandDisplayPrefPanelFormats			'SPrF'		// “Preferences“ window
#define kCommandDisplayPrefPanelFormatsANSI		'SPFA'		// multiple interfaces
#define kCommandDisplayPrefPanelFormatsNormal	'SPFN'		// multiple interfaces
#define kCommandDisplayPrefPanelGeneral			'SPrG'		// “Preferences“ window
#define kCommandDisplayPrefPanelKiosk			'SPrK'		// “Preferences“ window
#define kCommandDisplayPrefPanelMacros			'SPrM'		// “Preferences“ window
#define kCommandDisplayPrefPanelSessions		'SPrS'		// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsDataFlow	'SPSD'	// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsGraphics	'SPSG'	// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsKeyboard	'SPSK'	// “Preferences“ window
#define kCommandDisplayPrefPanelSessionsResource	'SPSR'	// “Preferences“ window
#define kCommandDisplayPrefPanelTerminals		'SPrT'		// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsEmulation	'SPTE'	// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsHacks	'SPTH'		// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsOptions	'SPTO'	// “Preferences“ window
#define kCommandDisplayPrefPanelTerminalsScreen	'SPTS'		// “Preferences“ window
#define kCommandDisplayPrefPanelTranslations	'SPrX'		// “Preferences“ window
#define kCommandDisplayPrefPanelWorkspaces		'SPrW'		// “Preferences“ window
#define kCommandAutoSetCursorColor				'AuCr'		// multiple interfaces
#define kCommandRestoreToDefault				'MkDf'		// multiple interfaces
#define kCommandPrefCursorBlock					'CrBl'		// “Preferences” window
#define kCommandPrefCursorUnderline				'CrUn'		// “Preferences” window
#define kCommandPrefCursorVerticalBar			'CrVB'		// “Preferences” window
#define kCommandPrefCursorThickUnderline		'CrBU'		// “Preferences” window
#define kCommandPrefCursorThickVerticalBar		'CrBV'		// “Preferences” window
#define kCommandPrefSetWindowLocation			'WLoc'		// “Preferences” window
#define kCommandPrefWindowResizeSetsScreenSize	'WRSS'		// “Preferences” window
#define kCommandPrefWindowResizeSetsFontSize	'WRFS'		// “Preferences” window
#define kCommandPrefCommandNOpensDefault		'CNDf'		// “Preferences” window
#define kCommandPrefCommandNOpensShell			'CNSh'		// “Preferences” window
#define kCommandPrefCommandNOpensLogInShell		'CNLI'		// “Preferences” window
#define kCommandPrefCommandNOpensCustomSession	'CNDg'		// “Preferences” window
#define kCommandPrefBellOff						'NoBp'		// “Preferences” window
#define kCommandPrefBellSystemAlert				'BpBl'		// “Preferences” window
#define kCommandPrefBellLibrarySound			'BpLb'		// “Preferences” window
#define kCommandPrefOpenGrowlPreferencesPane	'Grwl'		// “Preferences” window
#define kCommandToggleMacrosMenuVisibility		'McMn'		// “Preferences” window
#define kCommandEditMacroKey					'SMKy'		// “Preferences” window
#define kCommandSetMacroKeyTypeOrdinaryChar		'MKCh'		// “Preferences” window
#define kCommandSetMacroKeyTypeBackwardDelete	'MKBD'		// “Preferences” window
#define kCommandSetMacroKeyTypeForwardDelete	'MKFD'		// “Preferences” window
#define kCommandSetMacroKeyTypeHome				'MKHm'		// “Preferences” window
#define kCommandSetMacroKeyTypeEnd				'MKEd'		// “Preferences” window
#define kCommandSetMacroKeyTypePageUp			'MKPU'		// “Preferences” window
#define kCommandSetMacroKeyTypePageDown			'MKPD'		// “Preferences” window
#define kCommandSetMacroKeyTypeUpArrow			'MKUA'		// “Preferences” window
#define kCommandSetMacroKeyTypeDownArrow		'MKDA'		// “Preferences” window
#define kCommandSetMacroKeyTypeLeftArrow		'MKLA'		// “Preferences” window
#define kCommandSetMacroKeyTypeRightArrow		'MKRA'		// “Preferences” window
#define kCommandSetMacroKeyTypeClear			'MKCl'		// “Preferences” window
#define kCommandSetMacroKeyTypeEscape			'MKEs'		// “Preferences” window
#define kCommandSetMacroKeyTypeReturn			'MKRt'		// “Preferences” window
#define kCommandSetMacroKeyTypeEnter			'MKEn'		// “Preferences” window
#define kCommandSetMacroKeyTypeF1				'MKF1'		// “Preferences” window
#define kCommandSetMacroKeyTypeF2				'MKF2'		// “Preferences” window
#define kCommandSetMacroKeyTypeF3				'MKF3'		// “Preferences” window
#define kCommandSetMacroKeyTypeF4				'MKF4'		// “Preferences” window
#define kCommandSetMacroKeyTypeF5				'MKF5'		// “Preferences” window
#define kCommandSetMacroKeyTypeF6				'MKF6'		// “Preferences” window
#define kCommandSetMacroKeyTypeF7				'MKF7'		// “Preferences” window
#define kCommandSetMacroKeyTypeF8				'MKF8'		// “Preferences” window
#define kCommandSetMacroKeyTypeF9				'MKF9'		// “Preferences” window
#define kCommandSetMacroKeyTypeF10				'MKFa'		// “Preferences” window
#define kCommandSetMacroKeyTypeF11				'MKFb'		// “Preferences” window
#define kCommandSetMacroKeyTypeF12				'MKFc'		// “Preferences” window
#define kCommandSetMacroKeyTypeF13				'MKFd'		// “Preferences” window
#define kCommandSetMacroKeyTypeF14				'MKFe'		// “Preferences” window
#define kCommandSetMacroKeyTypeF15				'MKFf'		// “Preferences” window
#define kCommandSetMacroKeyTypeF16				'MKFg'		// “Preferences” window
#define kCommandSetMacroKeyModifierCommand		'McMC'		// “Preferences” window
#define kCommandSetMacroKeyModifierControl		'McML'		// “Preferences” window
#define kCommandSetMacroKeyModifierOption		'McMO'		// “Preferences” window
#define kCommandSetMacroKeyModifierShift		'McMS'		// “Preferences” window
#define kCommandSetMacroKeyAllowOnlyInMacroMode	'XRMM'		// “Preferences” window
#define kCommandSetMacroActionEnterTextWithSub	'MAET'		// “Preferences” window
#define kCommandSetMacroActionEnterTextVerbatim	'MAEV'		// “Preferences” window
#define kCommandSetMacroActionFindTextWithSub	'MAFS'		// “Preferences” window
#define kCommandSetMacroActionFindTextVerbatim	'MAFV'		// “Preferences” window
#define kCommandSetMacroActionOpenURL			'MAOU'		// “Preferences” window
#define kCommandSetMacroActionNewWindowCommand	'MANW'		// “Preferences” window
#define kCommandSetMacroActionSelectWindow		'MASW'		// “Preferences” window
#define kCommandSetMacroActionBeginMacroMode	'MAMM'		// “Preferences” window
#define kCommandEditMacroTextWithControlKeys	'EMTC'		// “Preferences” window
#define kCommandSetTEKModeDisabled				'RTNo'		// “Preferences” window
#define kCommandSetTEKModeTEK4014				'4014'		// “Preferences” window
#define kCommandSetTEKModeTEK4105				'4105'		// “Preferences” window
#define kCommandSetTEKPageClearsScreen			'XPCS'		// “Preferences” window
#define kCommandSetWorkspaceSessionNone			'WSNo'		// “Preferences” window
#define kCommandSetWorkspaceSessionDefault		'WSDf'		// “Preferences” window
#define kCommandSetWorkspaceSessionByFavoriteName	'WSFv'	// “Preferences” window
#define kCommandSetWorkspaceSessionShell		'WSSh'		// “Preferences” window
#define kCommandSetWorkspaceSessionLogInShell	'WSLI'		// “Preferences” window
#define kCommandSetWorkspaceSessionCustom		'WSDg'		// “Preferences” window
#define kCommandSetWorkspaceDisplayRegions1x1	'R1x1'		// multiple interfaces
#define kCommandSetWorkspaceDisplayRegions2x2	'R2x2'		// multiple interfaces
#define kCommandSetWorkspaceDisplayRegions3x3	'R3x3'		// multiple interfaces
#define kCommandSetWorkspaceWindowPosition		'SPos'		// multiple interfaces
#define kCommandSetEmulatorANSIBBS				'EmAB'		// multiple interfaces
#define kCommandSetEmulatorVT100				'E100'		// multiple interfaces
#define kCommandSetEmulatorVT102				'E102'		// multiple interfaces
#define kCommandSetEmulatorVT220				'E220'		// multiple interfaces
#define kCommandSetEmulatorVT320				'E320'		// multiple interfaces
#define kCommandSetEmulatorVT420				'E420'		// multiple interfaces
#define kCommandSetEmulatorXTermOriginal		'EmXT'		// multiple interfaces
#define kCommandSetEmulatorNone					'EDmb'		// multiple interfaces
#define kCommandSetScrollbackTypeDisabled		'ScNo'		// multiple interfaces
#define kCommandSetScrollbackTypeFixed			'ScFx'		// multiple interfaces
#define kCommandSetScrollbackTypeUnlimited		'ScUL'		// multiple interfaces
#define kCommandSetScrollbackTypeDistributed	'ScDs'		// multiple interfaces
#define kCommandSetScrollbackUnitsRows			'SbUR'		// multiple interfaces
#define kCommandSetScrollbackUnitsKilobytes		'SbUK'		// multiple interfaces
#define kCommandRetrySearch						'RFnd'		// “Find” dialog
#define kCommandResetANSIColors					'ANSD'		// “Preferences” window
#define kCommandOpenScriptMenuItemsFolder		'OSMI'		// “Preferences” window
#define kCommandEditInterruptKey				'SIKy'		// multiple interfaces
#define kCommandEditResumeKey					'SRKy'		// multiple interfaces
#define kCommandEditSuspendKey					'SSKy'		// multiple interfaces
#define kCommandSetMetaNone						'EMNo'		// multiple interfaces
#define kCommandSetMetaOptionKey				'EMOp'		// multiple interfaces
#define kCommandSetMetaShiftAndOptionKeys		'EMSO'		// multiple interfaces
#define kCommandSetNewlineCarriageReturnLineFeed	'CRLF'	// multiple interfaces
#define kCommandSetNewlineCarriageReturnNull	'CR00'		// multiple interfaces
#define kCommandSetNewlineCarriageReturnOnly	'NLCR'		// multiple interfaces
#define kCommandSetNewlineLineFeedOnly			'NLLF'		// multiple interfaces
#define kCommandToggleTerminalLED1				'LED1'		// terminal window toolbars
#define kCommandToggleTerminalLED2				'LED2'		// terminal window toolbars
#define kCommandToggleTerminalLED3				'LED3'		// terminal window toolbars
#define kCommandToggleTerminalLED4				'LED4'		// terminal window toolbars
#define kCommandTerminalNewWorkspace			'MTab'		// terminal window tab drawers

// commands used only in contextual menus
#define kCommandSpeakSelectedText				'SpkS'
#define kCommandStopSpeaking					'SpkE'

#pragma mark Types

struct Commands_ExecutionEventContext
{
	UInt32		commandID;		//!< which command the event is for
};
typedef Commands_ExecutionEventContext*		Commands_ExecutionEventContextPtr;

#ifdef __OBJC__

/*!
Implements an interface for menu commands to target.  See
"MainMenuCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Commands_Executor : NSObject< NSUserInterfaceValidations > //{
{
@private
	NSString*	_fullScreenCommandName;
}

// class methods
	+ (instancetype)
	sharedExecutor;

// accessors
	@property (strong) NSString*
	fullScreenCommandName;

// new methods: explicit validation (rarely needed)
	- (BOOL)
	defaultValidationForAction:(SEL)_
	sender:(id)_;
	- (BOOL)
	validateAction:(SEL)_
	sender:(id)_;

@end //}

/*!
Implements NSApplicationDelegate and NSApplicationNotifications;
see Commands.mm.
*/
@interface Commands_Executor (Commands_ApplicationCoreEvents) @end

/*!
Actions related to capturing terminal data to a file.
*/
@interface Commands_Executor (Commands_Capturing) //{

// actions
	- (IBAction)
	performCaptureBegin:(id)_;
	- (IBAction)
	performCaptureEnd:(id)_;
	- (IBAction)
	performPrintScreen:(id)_;
	- (IBAction)
	performPrintSelection:(id)_;
	- (IBAction)
	performSaveSelection:(id)_;

@end //}

/*!
Actions typically associated with the Edit menu.
*/
@interface Commands_Executor (Commands_Editing) //{

// actions
	- (IBAction)
	performUndo:(id)_;
	- (IBAction)
	performRedo:(id)_;
	- (IBAction)
	performCut:(id)_;
	- (IBAction)
	performCopy:(id)_;
	- (IBAction)
	performCopyWithTabSubstitution:(id)_;
	- (IBAction)
	performCopyAndPaste:(id)_;
	- (IBAction)
	performPaste:(id)_;
	- (IBAction)
	performDelete:(id)_;
	- (IBAction)
	performSelectAll:(id)_;
	- (IBAction)
	performSelectNothing:(id)_;
	- (IBAction)
	performSelectEntireScrollbackBuffer:(id)_;

@end //}

/*!
Actions that create new terminal-based sessions.
*/
@interface Commands_Executor (Commands_OpeningSessions) //{

// actions
	- (IBAction)
	performNewDefault:(id)_;
	- (IBAction)
	performNewByFavoriteName:(id)_;
	- (IBAction)
	performNewLogInShell:(id)_;
	- (IBAction)
	performNewShell:(id)_;
	- (IBAction)
	performNewCustom:(id)_;
	- (IBAction)
	performRestoreWorkspaceDefault:(id)_;
	- (IBAction)
	performRestoreWorkspaceByFavoriteName:(id)_;
	- (IBAction)
	performKill:(id)_;
	- (IBAction)
	performRestart:(id)_;
	- (IBAction)
	performOpen:(id)_;
	- (IBAction)
	performDuplicate:(id)_;
	- (IBAction)
	performSaveAs:(id)_;

// methods of the form required by setEventHandler:andSelector:forEventClass:andEventID:
	- (void)
	receiveGetURLEvent:(NSAppleEventDescriptor*)_
	replyEvent:(NSAppleEventDescriptor*)_;

@end //}

/*!
Actions related to vector graphics windows.
*/
@interface Commands_Executor (Commands_OpeningVectorGraphics) //{

// actions
	- (IBAction)
	performNewTEKPage:(id)_;
	- (IBAction)
	performPageClearToggle:(id)_;

@end //}

/*!
Actions that cause Internet addresses to be accessed.
*/
@interface Commands_Executor (Commands_OpeningWebPages) //{

// actions
	- (IBAction)
	performCheckForUpdates:(id)_;
	- (IBAction)
	performGoToMainWebSite:(id)_;
	- (IBAction)
	performOpenURL:(id)_;
	- (IBAction)
	performProvideFeedback:(id)_;

@end //}

/*!
Actions related to macros.
*/
@interface Commands_Executor (Commands_ManagingMacros) //{

// actions
	- (IBAction)
	performActionForMacro:(id)_;
	- (IBAction)
	performMacroSwitchNone:(id)_;
	- (IBAction)
	performMacroSwitchDefault:(id)_;
	- (IBAction)
	performMacroSwitchByFavoriteName:(id)_;
	- (IBAction)
	performMacroSwitchNext:(id)_;
	- (IBAction)
	performMacroSwitchPrevious:(id)_;

@end //}

/*!
Actions to configure terminal event handlers.
*/
@interface Commands_Executor (Commands_ManagingTerminalEvents) //{

// actions
	- (IBAction)
	performBellToggle:(id)_;
	- (IBAction)
	performSetActivityHandlerNone:(id)_;
	- (IBAction)
	performSetActivityHandlerNotifyOnNext:(id)_;
	- (IBAction)
	performSetActivityHandlerNotifyOnIdle:(id)_;
	- (IBAction)
	performSetActivityHandlerSendKeepAliveOnIdle:(id)_;

@end //}

/*!
Actions affecting keyboard behavior in terminal windows.
*/
@interface Commands_Executor (Commands_ManagingTerminalKeyMappings) //{

// actions
	- (IBAction)
	performDeleteMapToBackspace:(id)_;
	- (IBAction)
	performDeleteMapToDelete:(id)_;
	- (IBAction)
	performEmacsCursorModeToggle:(id)_;
	- (IBAction)
	performLocalPageKeysToggle:(id)_;
	- (IBAction)
	performMappingCustom:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutRxvt:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutVT220:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermX11:(id)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermXFree86:(id)_;
	- (IBAction)
	performTranslationSwitchDefault:(id)_;
	- (IBAction)
	performTranslationSwitchByFavoriteName:(id)_;
	- (IBAction)
	performTranslationSwitchCustom:(id)_;

@end //}

/*!
Actions to change various terminal behaviors.
*/
@interface Commands_Executor (Commands_ManagingTerminalSettings) //{

// actions
	- (IBAction)
	performInterruptProcess:(id)_;
	- (IBAction)
	performJumpScrolling:(id)_;
	- (IBAction)
	performLineWrapToggle:(id)_;
	- (IBAction)
	performLocalEchoToggle:(id)_;
	- (IBAction)
	performReset:(id)_;
	- (IBAction)
	performSaveOnClearToggle:(id)_;
	- (IBAction)
	performScrollbackClear:(id)_;
	- (IBAction)
	performSpeechToggle:(id)_;
	- (IBAction)
	performSuspendToggle:(id)_;

@end //}

/*!
Actions that change the number of rows and/or columns in terminal views.
*/
@interface Commands_Executor (Commands_ModifyingTerminalDimensions) //{

// actions
	- (IBAction)
	performScreenResizeCustom:(id)_;
	- (IBAction)
	performScreenResizeNarrower:(id)_;
	- (IBAction)
	performScreenResizeShorter:(id)_;
	- (IBAction)
	performScreenResizeStandard:(id)_;
	- (IBAction)
	performScreenResizeTall:(id)_;
	- (IBAction)
	performScreenResizeTaller:(id)_;
	- (IBAction)
	performScreenResizeWide:(id)_;
	- (IBAction)
	performScreenResizeWider:(id)_;

@end //}

/*!
Actions that affect fonts, colors and other format settings.
*/
@interface Commands_Executor (Commands_ModifyingTerminalText) //{

// actions
	- (IBAction)
	performFormatDefault:(id)_;
	- (IBAction)
	performFormatByFavoriteName:(id)_;
	- (IBAction)
	performFormatCustom:(id)_;
	- (IBAction)
	performFormatTextBigger:(id)_;
	- (IBAction)
	performFormatTextMaximum:(id)_;
	- (IBAction)
	performFormatTextSmaller:(id)_;

@end //}

/*!
Actions that affect a window’s properties, placement and size.
*/
@interface Commands_Executor (Commands_ModifyingWindows) //{

// actions
	- (IBAction)
	performArrangeInFront:(id)_;
	- (IBAction)
	performHideWindow:(id)_;
	- (IBAction)
	performHideOtherWindows:(id)_;
	- (IBAction)
	performMaximize:(id)_;
	- (IBAction)
	performMoveToNewWorkspace:(id)_;
	- (IBAction)
	performMoveWindowRight:(id)_;
	- (IBAction)
	performMoveWindowLeft:(id)_;
	- (IBAction)
	performMoveWindowDown:(id)_;
	- (IBAction)
	performMoveWindowUp:(id)_;
	- (IBAction)
	performRename:(id)_;
	- (IBAction)
	performShowHiddenWindows:(id)_;

@end //}

/*!
Actions that help the user to find things.
*/
@interface Commands_Executor (Commands_Searching) //{

// actions
	- (IBAction)
	performFind:(id)_;
	- (IBAction)
	performFindNext:(id)_;
	- (IBAction)
	performFindPrevious:(id)_;
	- (IBAction)
	performFindCursor:(id)_;
	- (IBAction)
	performShowCompletions:(id)_;
	- (IBAction)
	performSendMenuItemText:(id)_; // from completions menu; “types” the menu item title text

@end //}

/*!
Actions that display specific windows.
*/
@interface Commands_Executor (Commands_ShowingPanels) //{

// actions
	- (IBAction)
	orderFrontAbout:(id)_;
	- (IBAction)
	orderFrontClipboard:(id)_;
	- (IBAction)
	orderFrontCommandLine:(id)_;
	- (IBAction)
	orderFrontContextualHelp:(id)_;
	- (IBAction)
	orderFrontControlKeys:(id)_;
	- (IBAction)
	orderFrontDebuggingOptions:(id)_;
	- (IBAction)
	orderFrontIPAddresses:(id)_;
	- (IBAction)
	orderFrontPreferences:(id)_;
	- (IBAction)
	orderFrontSessionInfo:(id)_;
	- (IBAction)
	orderFrontVT220FunctionKeys:(id)_;
	- (IBAction)
	orderFrontVT220Keypad:(id)_;

@end //}

/*!
Actions to enter or exit Full Screen.
*/
@interface Commands_Executor (Commands_SwitchingModes) //{

// actions
	- (IBAction)
	toggleFullScreen:(id)_;

@end //}

/*!
Actions to cycle through windows.
*/
@interface Commands_Executor (Commands_SwitchingWindows) //{

// actions
	- (IBAction)
	orderFrontNextWindow:(id)_;
	- (IBAction)
	orderFrontNextWindowHidingPrevious:(id)_;
	- (IBAction)
	orderFrontPreviousWindow:(id)_;
	- (IBAction)
	orderFrontSpecificWindow:(id)_;

@end //}

/*!
These methods are similar to those typically found on NSWindow
derivatives.  They are TEMPORARY, used to transition away from
Carbon.  After observing that Apple’s Cocoa wrappers for Carbon
windows can do unexpected things with “standard” messages (e.g.
when receiving "performClose:"), the choice was made to map the
Cocoa menus to the variants below.  These methods detect when
the action receiver would be a Carbon window, and then they
send the request in the “Carbon way”.  If the action receiver
would be a “real” Cocoa window, the standard action is sent up
the responder chain in the usual way.
*/
@interface Commands_Executor (Commands_TransitionFromCarbon) //{

// actions
	- (IBAction)
	performCloseSetup:(id)_;
	- (IBAction)
	performMinimizeSetup:(id)_;
	- (IBAction)
	performSpeakSelectedText:(id)_;
	- (IBAction)
	performStopSpeaking:(id)_;
	- (IBAction)
	performZoomSetup:(id)_;
	- (IBAction)
	runToolbarCustomizationPaletteSetup:(id)_;
	- (IBAction)
	toggleToolbarShownSetup:(id)_;

// new methods
	- (NSMenuItem*)
	newMenuItemForCommand:(UInt32)_
	itemTitle:(NSString*)_
	ifEnabled:(BOOL)_;

@end //}

#endif



#pragma mark Public Methods

//!\name Initialization
//@{

void
	Commands_Init							();

void
	Commands_Done							();

//@}

//!\name Executing Commands
//@{

// WARNING: NOT THREAD SAFE, USE Commands_ExecuteByIDUsingEvent() TO INSERT A COMMAND INTO THE MAIN THREAD’S QUEUE
Boolean
	Commands_ExecuteByID					(UInt32						inCommandID);

Boolean
	Commands_ExecuteByIDUsingEvent			(UInt32						inCommandID,
											 EventTargetRef				inTarget = nullptr);

void
	Commands_ExecuteByIDUsingEventAfterDelay(UInt32						inCommandID,
											 EventTargetRef				inTarget,
											 Float32					inDelayInSeconds);

//@}

//!\name Retrieving Command Information
//@{

Boolean
	Commands_CopyCommandName				(UInt32						inCommandID,
											 Commands_NameType			inNameType,
											 CFStringRef&				outName);

//@}

//!\name Standard Carbon Event Handlers
//@{

OSStatus
	Commands_HandleCreateToolbarItem		(EventHandlerCallRef		inHandlerCallRef,
											 EventRef					inEvent,
											 void*						inNullContextPtr);

//@}

//!\name Cocoa Menu Utilities
//@{

Commands_Result
	Commands_InsertPrefNamesIntoMenu		(Quills::Prefs::Class		inClass,
											 NSMenu*					inoutMenu,
											 int						inAtItemIndex,
											 int						inInitialIndent,
											 SEL						inAction);

// WARNING: CURRENTLY ONLY IMPLEMENTED FOR CONTEXTUAL MENU COMMAND IDS
NSMenuItem*
	Commands_NewMenuItemForCommand			(UInt32						inCommandID,
											 CFStringRef				inPreferredTitle,
											 Boolean					inMustBeEnabled = false);

//@}

//!\name Installing Callbacks That Handle Commands
//@{

// EVENT CONTEXT PASSED TO LISTENER: Commands_ExecutionEventContextPtr
Commands_Result
	Commands_StartHandlingExecution			(UInt32						inImplementedCommand,
											 ListenerModel_ListenerRef	inCommandImplementor);

// EVENT CONTEXT PASSED TO LISTENER: Commands_ExecutionEventContextPtr
Commands_Result
	Commands_StopHandlingExecution			(UInt32						inImplementedCommand,
											 ListenerModel_ListenerRef	inCommandImplementor);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
