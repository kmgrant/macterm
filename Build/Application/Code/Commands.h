/*!	\file Commands.h
	\brief A list of all command IDs, and a mechanism for
	invoking MacTelnet’s main features.
	
	A command is a series of primitive actions that leads to a
	result; usually, there is a menu item for each command (but
	this is not required; for example, a command might be used
	to operate a toolbar item).
	
	Note that although commands would seem to highly correlate
	with AppleScript, this is NOT the case.  The scripting terms
	tend to be more primitive and object-oriented and not as
	limited as “execute command X”.  For example, the command
	Minimize Window might actually be represented as “set the
	collapsed of the window whose title is "foobar" to true”
	when recorded into a script.  Therefore code that implements
	scripting terms should NOT use this module in general, to
	avoid possible sources of recursion.
*/
/*###############################################################

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

#ifndef REZ
#	include "UniversalDefines.h"
#endif

#ifndef __COMMANDS__
#define __COMMANDS__

// Mac includes
#ifndef REZ
#   include <CoreServices/CoreServices.h>
#endif

// library includes
#ifndef REZ
#	include <ListenerModel.h>
#endif
#include <ResultCode.template.h>



#pragma mark Constants

#ifndef REZ
typedef ResultCode< UInt16 >	Commands_Result;
Commands_Result const	kCommands_ResultOK(0);				//!< no error
Commands_Result const	kCommands_ResultParameterError(1);	//!< bad input - for example, invalid listener type
#endif

enum Commands_NameType
{
	kCommands_NameTypeDefault		= 0,	//!< the name of the command in normal context (such as in a menu item)
	kCommands_NameTypeShort			= 1		//!< a short version of the name (such as in a toolbar item)
};

/*!
Command IDs

WARNING:	Although all source code should refer to these
			IDs only via the constants below, a number of
			Interface Builder NIB files for MacTelnet will
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
button).  Also, using standard commands means that MacTelnet
will receive events properly when future OS capabilities are
added (for example, window content controls might gain
contextual menus that can automatically execute the right
MacTelnet commands, such as Cut, Copy, Paste or Undo).
*/

// Application (Apple) menu
// WARNING: These are referenced by value in the MainMenus.nib and
//          MenuForDockIcon.nib files!
#define kCommandAboutThisApplication			kHICommandAbout
#define kCommandFullScreenModal					'Kios'
#define kCommandKioskModeDisable				'KskQ'		// also used in Kiosk Mode off-switch floater
#define kCommandShowNetworkNumbers				'CIPn'
#define kCommandSendInternetProtocolNumber		'SIPn'
#define kCommandCheckForUpdates					'ChUp'
#define kCommandURLHomePage						'.com'
#define kCommandURLAuthorMail					'Mail'
#define kCommandURLSourceLicense				'CGPL'
#define kCommandURLProjectStatus				'Proj'

// File menu
#define kCommandNewSessionDefaultFavorite		kHICommandNew
#define kCommandNewSessionByFavoriteName		'NFav'
#define kCommandNewSessionLoginShell			'NLgS'
#define kCommandNewSessionShell					'NShS'
#define kCommandNewSessionDialog				'New…'
#define kCommandOpenSession						kHICommandOpen
#define kCommandCloseConnection					kHICommandClose
#define kCommandSaveSession						kHICommandSaveAs
#define kCommandNewDuplicateSession				'NewD'
#define kCommandHandleURL						'HURL'
#define kCommandSaveText						'SvTx'
#define kCommandCaptureToFile					'Capt'
#define kCommandEndCaptureToFile				'CapE'
#define kCommandImportMacroSet					'IMcr'
#define kCommandExportCurrentMacroSet			'EMcr'
#define kCommandPageSetup						kHICommandPageSetup
#define kCommandPrint							kHICommandPrint
#define kCommandPrintOne						'Pr1C'
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
#define kCommandFindCursor						'FndC'
#define kCommandSelectAll						kHICommandSelectAll
#define kCommandSelectAllWithScrollback			'SlSb'
#define kCommandSelectNothing					'Sel0'
#define kCommandShowClipboard					'ShCl'
#define kCommandHideClipboard					'HiCl'

// View menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandSmallScreen						'StdW'
#define kCommandTallScreen						'Tall'
#define kCommandLargeScreen						'Wide'
#define kCommandSetScreenSize					'SSiz'
#define kCommandBiggerText						'FSzB'
#define kCommandFullScreen						'Full'
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
#define kCommandTerminalEmulatorSetup			'Emul'
#define kCommandWatchNothing					'WOff'
#define kCommandWatchForActivity				'Notf'
#define kCommandWatchForInactivity				'Idle'
#define kCommandTransmitOnInactivity			'KAlv'
#define kCommandSpeechEnabled					'Talk'
#define kCommandClearEntireScrollback			'ClSB'
#define kCommandResetGraphicsCharacters			'NoGr'
#define kCommandResetTerminal					'RTrm'

// Map menu
// WARNING: These are referenced by value in the MainMenus.nib file!
#define kCommandDeletePressSendsBackspace		'DBks'
#define kCommandDeletePressSendsDelete			'DDel'
#define kCommandEMACSArrowMapping				'EMAC'
#define kCommandLocalPageUpDown					'LcPg'
#define kCommandSetKeys							'SetK'
#define kCommandMacroSetNone					'XMcr'
#define kCommandMacroSetDefault					'McrD'
#define kCommandMacroSetByFavoriteName			'MFav'
#define kCommandTranslationTableNone			'XXlt'
#define kCommandTranslationTableDefault			'XltD'
#define kCommandTranslationTableByFavoriteName	'XFav'
#define kCommandFixCharacterTranslation			'FixT'

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
#define kCommandShowMacros						'ShMc'
#define kCommandHideMacros						'HiMc'
#define kCommandShowCommandLine					'ShCL'
#define kCommandShowControlKeys					'ShCK'
#define kCommandShowFunction					'ShFn'
#define kCommandShowKeypad						'ShKp'

// Help menu
#define kCommandMainHelp						kHICommandAppHelp
#define kCommandContextSensitiveHelp			'?Ctx'
#define kCommandShowHelpTags					'STag'
#define kCommandHideHelpTags					'HTag'

// color box
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
#define kCommandKeypadControlTilde				'CK^~' //!< ASCII 30
#define kCommandKeypadControlQuestionMark		'CK^?' //!< ASCII 31
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

// macro invokers
#define kCommandSendMacro1						'Mc01'
#define kCommandSendMacro2						'Mc02'
#define kCommandSendMacro3						'Mc03'
#define kCommandSendMacro4						'Mc04'
#define kCommandSendMacro5						'Mc05'
#define kCommandSendMacro6						'Mc06'
#define kCommandSendMacro7						'Mc07'
#define kCommandSendMacro8						'Mc08'
#define kCommandSendMacro9						'Mc09'
#define kCommandSendMacro10						'Mc10'
#define kCommandSendMacro11						'Mc11'
#define kCommandSendMacro12						'Mc12'

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
#define kCommandNewProtocolSelectedFTP			'NPFT'		// “New Session” dialog
#define kCommandNewProtocolSelectedSFTP			'NPSF'		// “New Session” dialog
#define kCommandNewProtocolSelectedSSH1			'NPS1'		// “New Session” dialog
#define kCommandNewProtocolSelectedSSH2			'NPS2'		// “New Session” dialog
#define kCommandNewProtocolSelectedTELNET		'NPTN'		// “New Session” dialog
#define kCommandShowProtocolOptions				'POpt'		// “New Session” dialog
#define kCommandLookUpSelectedHostName			'Look'		// “New Session” dialog
#define kCommandTerminalDefaultFavorite			'DefF'		// “New Session” dialog
#define kCommandChangeMacroKeysToCommandKeypad	'MKCK'		// “Macro Setup” window
#define kCommandChangeMacroKeysToFunctionKeys	'MKFK'		// “Macro Setup” window
#define kCommandShowAdvancedMacroInformation	'AdvM'		// “Macro Setup” window
#define kCommandShowHidePrefCollectionsDrawer	'SPCD'		// “Preferences“ window
#define kCommandDisplayPrefPanelFormats			'SPrF'		// “Preferences“ window
#define kCommandDisplayPrefPanelFormatsANSI		'SPFA'		// multiple interfaces
#define kCommandDisplayPrefPanelFormatsNormal	'SPFN'		// multiple interfaces
#define kCommandDisplayPrefPanelGeneral			'SPrG'		// “Preferences“ window
#define kCommandDisplayPrefPanelKiosk			'SPrK'		// “Preferences“ window
#define kCommandDisplayPrefPanelMacros			'SPrM'		// “Preferences“ window
#define kCommandDisplayPrefPanelScripts			'SPrC'		// “Preferences“ window
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
#define kCommandPrefCursorBlock					'CrBl'		// “Preferences” window
#define kCommandPrefCursorUnderline				'CrUn'		// “Preferences” window
#define kCommandPrefCursorVerticalBar			'CrVB'		// “Preferences” window
#define kCommandPrefCursorThickUnderline		'CrBU'		// “Preferences” window
#define kCommandPrefCursorThickVerticalBar		'CrBV'		// “Preferences” window
#define kCommandPrefWindowResizeSetsScreenSize	'WRSS'		// “Preferences” window
#define kCommandPrefWindowResizeSetsFontSize	'WRFS'		// “Preferences” window
#define kCommandPrefCommandNOpensDefault		'CNDf'		// “Preferences” window
#define kCommandPrefCommandNOpensShell			'CNSh'		// “Preferences” window
#define kCommandPrefCommandNOpensLogInShell		'CNLI'		// “Preferences” window
#define kCommandPrefCommandNOpensCustomSession	'CNDg'		// “Preferences” window
#define kCommandPrefBellOff						'NoBp'		// “Preferences” window
#define kCommandPrefBellSystemAlert				'BpBl'		// “Preferences” window
#define kCommandPrefBellLibrarySound			'BpLb'		// “Preferences” window
#define kCommandToggleMacrosMenuVisibility		'McMn'		// “Preferences” window
#define kCommandPreferencesNewFavorite			'NewC'		// “Preferences” window
#define kCommandPreferencesDuplicateFavorite	'DupC'		// “Preferences” window
#define kCommandPreferencesRenameFavorite		'RnmC'		// “Preferences” window
#define kCommandPreferencesDeleteFavorite		'DelC'		// “Preferences” window
#define kCommandSetTEKModeDisabled				'RTNo'		// “Preferences” window
#define kCommandSetTEKModeTEK4014				'4014'		// “Preferences” window
#define kCommandSetTEKModeTEK4105				'4105'		// “Preferences” window
#define kCommandSetTEKPageClearsScreen			'XPCS'		// “Preferences” window
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
#define kCommandToggleTerminalLED1				'LED1'		// terminal window toolbars
#define kCommandToggleTerminalLED2				'LED2'		// terminal window toolbars
#define kCommandToggleTerminalLED3				'LED3'		// terminal window toolbars
#define kCommandToggleTerminalLED4				'LED4'		// terminal window toolbars
#define kCommandTerminalNewWorkspace			'MTab'		// terminal window tab drawers
#define kCommandSetBackground					'SBkg'		// generic request to open a UI to change a background

// commands no longer used, may be deleted
#define kCommandDisplayWindowContextualMenu		'CMnu'
#define kCommandCloseWorkspace					'ClsA'
#define kCommandKillProcessesKeepWindow			'Kill'
#define kCommandSpeakSelectedText				'SpkS'
#define kCommandFindFiles						'FFil'

#pragma mark Types

#ifndef REZ

struct Commands_ExecutionEventContext
{
	UInt32		commandID;		//!< which command the event is for
};
typedef Commands_ExecutionEventContext*		Commands_ExecutionEventContextPtr;

#endif



#ifndef REZ

#pragma mark Public Methods

//!\name Executing Commands
//@{

// WARNING: NOT THREAD SAFE, USE Commands_ExecuteByIDUsingEvent() TO INSERT A COMMAND INTO THE MAIN THREAD’S QUEUE
Boolean
	Commands_ExecuteByID					(UInt32						inCommandID);

Boolean
	Commands_ExecuteByIDUsingEvent			(UInt32						inCommandID,
											 EventTargetRef				inTarget = nullptr);

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

pascal OSStatus
	Commands_HandleCreateToolbarItem		(EventHandlerCallRef		inHandlerCallRef,
											 EventRef					inEvent,
											 void*						inNullContextPtr);

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

#endif /* REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
