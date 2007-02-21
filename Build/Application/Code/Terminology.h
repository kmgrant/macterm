/*!	\file Terminology.h
	\brief Four-character codes for the AppleScript Dictionary.
	
	This file contains Apple Events and corresponding classes
	that MacTelnet recognizes (for scriptability).  All of this
	information is very important!  Grouped four-character codes
	must be unique within the group - the group is determined by
	a common prefix and/or suffix in the name.
	
	\attention
	Changes to this file should be reflected in the scripting
	definition (currently "Dictionary.sdef", which is compiled
	into Rez input for an 'aete' resource), and vice-versa.
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

#ifndef __TERMINOLOGY__
#define __TERMINOLOGY__

// MacTelnet includes
#include "ConstantsRegistry.h"



#pragma mark Constants

// suite types
#define kASRequiredSuite											'aevt'	// a.k.a. kCoreEventClass
#define kASStandardAdditionsUserInteractionSuite					'syso'
#define kASTypeDefinitionsSuite										'tpdf'
#define kAESpecialClassPropertiesParentType							'c@#^'
#define kMySuite													kConstantsRegistry_ApplicationCreatorSignature

// IDs for all possible class properties, and their corresponding data types; the list below must be unique,
// so that scripts can be compiled unambiguously; HOWEVER, note that source code should NEVER USE THE
// CONSTANTS BELOW; instead, look for constants defined later (in terms of those below) that are tied to a
// specific class, so that the intentions of the programmer are made clear and code is less error-prone

// IDs for all possible parameters, and their corresponding data types; the list below must be unique, so
// that scripts can be compiled unambiguously; HOWEVER, note that source code should NEVER USE THE CONSTANTS
// BELOW; instead, look for constants defined later (in terms of those below) that are tied to a specific
// event, so that the intentions of the programmer are made clear and code is less error-prone
#define   keyAEParamMyAlertSound									'ASnd'
#define valueAEParamMyAlertSound										typeBoolean
#define   keyAEParamMyAs											keyAERequestedType
#define valueAEParamMyAs												typeType
#define   keyAEParamMyAsSheetOf										'PWin'
#define valueAEParamMyAsSheetOf											cMyWindow
#define   keyAEParamMyButtons										'BtnL'
#define valueAEParamMyButtons											cList
#define   keyAEParamMyCopies										'#Cpy'
#define valueAEParamMyCopies											typeSInt16
#define   keyAEParamMyEach											keyAEObjectClass
#define valueAEParamMyEach												typeType
#define   keyAEParamMyEchoing										'Echo'
#define valueAEParamMyEchoing											enumBooleanValues
#define   keyAEParamMyFromSource									'FrSW'
#define valueAEParamMyFromSource										cMySessionWindow
#define   keyAEParamMyGivingUpAfter									'givu' // DEFINED BY APPLE, DO NOT CHANGE
#define valueAEParamMyGivingUpAfter										typeSInt16
#define   keyAEParamMyHelpButton									'?Btn'
#define valueAEParamMyHelpButton										typeBoolean
#define   keyAEParamMyHelpText										'?Txt'
#define valueAEParamMyHelpText											cIntlText
#define   keyAEParamMyName											keyAEName
#define valueAEParamMyName												cIntlText
#define   keyAEParamMyNew											keyAEObjectClass
#define valueAEParamMyNew												typeType
#define   keyAEParamMyNewline										'S+NL'
#define valueAEParamMyNewline											enumBooleanValues
#define   keyAEParamMyNetworkDisruption								'ADsN'
#define valueAEParamMyNetworkDisruption									typeBoolean
#define   keyAEParamMyProtocol										'Prcl'
#define valueAEParamMyProtocol											kTelnetEnumeratedClassConnectionMethod
#define   keyAEParamMySpeech										'ASpk'
#define valueAEParamMySpeech											typeBoolean
#define   keyAEParamMyTabsInPlaceOfMultipleSpaces					'CpTb'
#define valueAEParamMyTabsInPlaceOfMultipleSpaces						enumBooleanValues
#define   keyAEParamMyTCPPort										'Port'
#define valueAEParamMyTCPPort											typeSInt16
#define   keyAEParamMyTitled										'HTtl'
#define valueAEParamMyTitled											enumBooleanValues
#define   keyAEParamMyTo											keyAEData
#define valueAEParamMyTo												typeWildCard
#define   keyAEParamMyToDestination									'ToSW'
#define valueAEParamMyToDestination										cMySessionWindow
#define   keyAEParamMyToHost										'OHst'
#define valueAEParamMyToHost											typeChar
#define   keyAEParamMyWithIcon										'disp' // DEFINED BY APPLE, DO NOT CHANGE
#define valueAEParamMyWithIcon											kTelnetEnumeratedClassAlertIconType
#define   keyAEParamMyWithProperties								keyAEProperties
#define valueAEParamMyWithProperties									cRecord
#define   keyAEParamMyVia											'OPrx'
#define valueAEParamMyVia												cMyProxyServerConfiguration

// other values that correspond to specific events (that is, return values or unnamed parameters)
#define valueAEResultParamForEvent_connect								cMyConnection
#define valueAEDirectParamForEvent_count							cReference
#define valueAEResultParamForEvent_count								typeSInt32
#define valueAEDirectParamForEvent_dataSize							cReference
#define valueAEResultParamForEvent_dataSize								typeSInt32
#define valueAEDirectParamForEvent_delete							cReference
#define valueAEDirectParamForEvent_displayAlert						cIntlText
#define valueAEResultParamForEvent_displayAlert							cMyDialogReply
#define valueAEDirectParamForEvent_duplicate						cReference
#define valueAEResultParamForEvent_duplicate							cReference
#define valueAEDirectParamForEvent_exists							cReference
#define valueAEResultParamForEvent_exists								typeBoolean
#define valueAEDirectParamForEvent_expect1							typeWildCard
#define valueAEDirectParamForEvent_expect2							kTelnetEnumeratedClassSpecialCharacter
#define valueAEDirectParamForEvent_get								cReference
#define valueAEResultParamForEvent_get									typeWildCard
#define valueAEResultParamForEvent_make									cReference
#define valueAEDirectParamForEvent_openLocation						cReference
#define valueAEResultParamForEvent_openLocation							typeSInt16
#define valueAEDirectParamForEvent_print							cReference
#define valueAEDirectParamForEvent_select							cReference
#define valueAEDirectParamForEvent_send1							typeWildCard
#define valueAEDirectParamForEvent_send2							kTelnetEnumeratedClassSpecialCharacter
#define valueAEDirectParamForEvent_set								cReference
#define valueAEDirectParamForEvent_spawnProcess						typeChar
#define valueAEResultParamForEvent_spawnProcess							cMyConnection

/*###########################################
	Standard Suite Extensions
###########################################*/

// classes, with properties
#define cMyApplication														cApplication
#define		pMyActiveMacroSet										'RAAM'
#define			cpMyActiveMacroSet											cMyMacroSet
#define		pMyClipboard											'RACl'
#define			cpMyClipboard												cMyClipboardWindow
#define		pMyFolderScriptsMenuItems								'RAfS'
#define			cpMyFolderScriptsMenuItems									typeFSS
#define		pMyFolderStartupItems									'RAfO'
#define			cpMyFolderStartupItems										typeFSS
#define		pMyIPAddress											'RAIP'
#define			cpMyIPAddress												typeChar
#define		pMyIsFrontmost											pIsFrontProcess
#define			cpMyIsFrontmost												typeBoolean
#define		pMyPreferences											'RAPf'
#define			cpMyPreferences												cMyApplicationPreferences
#define		pMyProgramName											pName
#define			cpMyProgramName												cIntlText
#define		pMyProgramVersion										pVersion
#define			cpMyProgramVersion											cVersion
#define cMyWindow															cWindow
#define		pMyWindowBounds											pBounds
#define			cpMyWindowBounds											cQDRectangle
#define		pMyWindowHasCloseBox									pHasCloseBox
#define			cpMyWindowHasCloseBox										typeBoolean
#define		pMyWindowHasCollapseBox									'RW=='
#define			cpMyWindowHasCollapseBox									typeBoolean
#define		pMyWindowHasTitleBar									pHasTitleBar
#define			cpMyWindowHasTitleBar										typeBoolean
#define		pMyWindowHasToolbar										'RWM$'
#define			cpMyWindowHasToolbar										typeBoolean
#define		pMyWindowHidden											'RWOb'
#define			cpMyWindowHidden											typeBoolean
#define		pMyWindowIndexInWindowList								pIndex
#define			cpMyWindowIndexInWindowList									typeSInt16
#define		pMyWindowIsCollapsed									'RWSh'
#define			cpMyWindowIsCollapsed										typeBoolean
#define		pMyWindowIsFloating										pIsFloating
#define			cpMyWindowIsFloating										typeBoolean
#define		pMyWindowIsModal										pIsModal
#define			cpMyWindowIsModal											typeBoolean
#define		pMyWindowIsResizeable									pIsResizable
#define			cpMyWindowIsResizeable										typeBoolean
#define		pMyWindowIsVisible										pVisible
#define			cpMyWindowIsVisible											typeBoolean
#define		pMyWindowIsZoomable										pIsZoomable
#define			cpMyWindowIsZoomable										typeBoolean
#define		pMyWindowIsZoomed										pIsZoomed
#define			cpMyWindowIsZoomed											typeBoolean
#define		pMyWindowName											pName
#define			cpMyWindowName												cIntlText
#define		pMyWindowPosition										'RWxy'
#define			cpMyWindowPosition											cQDPoint

/*###########################################
	URL Suite
###########################################*/

#define kStandardURLSuite											'GURL'

// implemented / extended events

// event "handle URL"
#define kStandardURLEventIDGetURL									'GURL'

/*###########################################
	MacTelnet Suite
###########################################*/

// classes, with properties
#define cMyApplicationPreferences									'TPrf'
#define		pMyApplicationPreferencesCursorShape					'RPCs'
#define			cpMyApplicationPreferencesCursorShape					kTelnetEnumeratedClassCursorShape
#define		pMyApplicationPreferencesSimplifiedUserInterface		'RPST'
#define			cpMyApplicationPreferencesSimplifiedUserInterface		typeBoolean
#define		pMyApplicationPreferencesVisualBell						'RPVB'
#define			cpMyApplicationPreferencesVisualBell					typeBoolean
#define		pMyApplicationPreferencesWindowStackingOrigin			'RPSO'
#define			cpMyApplicationPreferencesWindowStackingOrigin			cQDPoint
#define cMyClipboardWindow											'TClp'
#define		pMyClipboardWindowContents								'RcDt'
#define cMyConnection												'TCxn'
#define		pMyConnectionLocationNameOrIPAddress					'RCNI'
#define		pMyConnectionLocationPortNumber							'RCPN'
#define		pMyConnectionProxy										'RCPx'
#define cMyDialogReply												'askr' // DEFINED BY APPLE, DO NOT CHANGE
#define		pMyDialogReplyButtonReturned							'bhit' // DEFINED BY APPLE, DO NOT CHANGE
#define		pMyDialogReplyGaveUp									'gavu' // DEFINED BY APPLE, DO NOT CHANGE
#define cMyFileCapture												'TFlC'
#define		pMyFileCaptureConnection								'RFCn'
#define			cpMyFileCaptureConnection								cMyConnection
#define		pMyFileCaptureCreator									'RFFC'
#define			cpMyFileCaptureCreator									typeApplSignature
#define		pMyFileCaptureFile										'RFFl'
#define			cpMyFileCaptureFile										typeAlias
#define		pMyFileCaptureStatus									'RFSt'
#define			cpMyFileCaptureStatus									kTelnetEnumeratedClassFileCaptureStatus
#define		pMyFileCaptureText										'RFTx'
#define			cpMyFileCaptureText										typeChar
#define cMyFormat													'TFmt'
#define		pMyFormatFontFamilyName									'RfAa'
#define			cpMyFormatFontFamilyName								cIntlText
#define		pMyFormatFontSize										'RfSz'
#define			cpMyFormatFontSize										typeSInt16
#define		pMyFormatColorText										'RfCT'
#define		pMyFormatColorBackground								'RfCB'
#define cMyMacro													'TMcr'
#define		pMyMacroKeyEquivalent									'RmKE'
#define		pMyMacroTextString										'RmTS'
#define		pMyMacroName											pName
#define cMyMacroSet													'TMcS'
#define		pMyMacroSetKeyEquivalents								'RMKE'
#define cMyProxyServerConfiguration									'TPrx'
#define		pMyProxyServerLocationNameOrIPAddress					'RpNI'
#define		pMyProxyServerLocationPortNumber						'RpPN'
#define		pMyProxyServerMethod									'RpMt'
#define		pMyProxyServerAuthenticationType						'RpAT'
#define		pMyProxyServerUserName									'RpUN'
#define cMySessionWindow											'TSWn'
#define		pMySessionWindowRemote									'RSRm'
#define		pMySessionWindowStatus									'RSSt'
#define cMyTerminalScreen											'TTSc'
#define cMyTerminalWindow											'TTWn'
#define		pMyTerminalWindowColumns								'RTvc'
#define			cpMyTerminalWindowColumns								typeSInt16
#define		pMyTerminalWindowRows									'RTvr'
#define			cpMyTerminalWindowRows									typeSInt16
#define		pMyTerminalWindowScrollBack								'RTir'
#define			cpMyTerminalWindowScrollBack							typeSInt16
#define		pMyTerminalWindowOpacity								'RTop'
#define			cpMyTerminalWindowOpacity								typeFloat
#define		pMyTerminalWindowNormalFormat							'RTNF'
#define			cpMyTerminalWindowNormalFormat							cMyFormat
#define		pMyTerminalWindowBoldFormat								'RTBF'
#define			cpMyTerminalWindowBoldFormat							cMyFormat
#define		pMyTerminalWindowBlinkingFormat							'RTFF'
#define			cpMyTerminalWindowBlinkingFormat						cMyFormat
#define		pMyTerminalWindowContents								'RTtx'
#define			cpMyTerminalWindowContents								typeChar
#define		pMyTerminalWindowScreenContents							'RTsc'
#define			cpMyTerminalWindowScreenContents						typeChar
#define		pMyTerminalWindowScrollbackContents						'RTsb'
#define			cpMyTerminalWindowScrollbackContents					typeChar
#define cMyTextBuffer												'BTxt'

// HIDDEN classes; these are used internally to process Apple Events and resolve objects
#define cMyInternalToken											'TTok'

// event "connect"
#define kTelnetEventIDConnectionOpen								'TCnO'

// event "copy selected text"
#define kTelnetEventIDCopyToClipboard								'TCpy'

// event "display alert"
#define kTelnetEventIDAlert											'TDAl'

// event "send"
#define kTelnetEventIDDataSend										'TDat'

// event "expect"
#define kTelnetEventIDDataWait										'TWai'

// event "launch Sherlock"
#define kTelnetEventIDLaunchFind									'TFnd'

// event "spawn process"
#define kTelnetEventIDProcessSpawn									'TUnx'

// HIDDEN events; these are only used by MacTelnet Help scripts (to do things for the user),
// and by MacTelnet’s libraries, in order to communicate with the debugging console.
#define kTelnetHiddenEventIDConsoleWriteLine						'?WrL'
#define kTelnetHiddenEventIDDisplayDialog							'?Dlg'

// enumerations
#define kTelnetEnumeratedClassAlertIconType							'EAIT'
#define		kTelnetEnumeratedClassAlertIconTypeNote						'note' // DEFINED BY APPLE, DO NOT CHANGE
#define		kTelnetEnumeratedClassAlertIconTypeStop						'stop' // DEFINED BY APPLE, DO NOT CHANGE
#define		kTelnetEnumeratedClassAlertIconTypeCaution					'caut' // DEFINED BY APPLE, DO NOT CHANGE
#define kTelnetEnumeratedClassConnectionMethod						'ECMt'
#define		kTelnetEnumeratedClassConnectionMethodTelnet				'CMt1'
#define		kTelnetEnumeratedClassConnectionMethodFTP					'CMt2'
#define		kTelnetEnumeratedClassConnectionMethodSecureShell2			'CMt3'
#define kTelnetEnumeratedClassCursorShape							'ECur'
#define		kTelnetEnumeratedClassCursorShapeBlock						'CurB'
#define		kTelnetEnumeratedClassCursorShapeVerticalBar				'CurV'
#define		kTelnetEnumeratedClassCursorShapeVerticalBarBold			'CuBV'
#define		kTelnetEnumeratedClassCursorShapeUnderscore					'CurU'
#define		kTelnetEnumeratedClassCursorShapeUnderscoreBold				'CuBU'
#define kTelnetEnumeratedClassFileCaptureStatus						'EFSt'
#define		kTelnetEnumeratedClassFileCaptureStatusNotBegun				'FStU'
#define		kTelnetEnumeratedClassFileCaptureStatusInProgress			'FStC'
#define		kTelnetEnumeratedClassFileCaptureStatusComplete				'FStD'
#define kTelnetEnumeratedClassMacroSetKeyEquivalents				'EMKE'
#define		kTelnetEnumeratedClassMacroSetKeyEquivalentsCommandDigit	'MKED'
#define		kTelnetEnumeratedClassMacroSetKeyEquivalentsFunctionKeys	'MKEF'
#define kTelnetEnumeratedClassProxyServerAuthenticationType			'EPAu'
#define		kTelnetEnumeratedClassProxyServerAuthenticationTypePassword	'PAUP'
#define kTelnetEnumeratedClassProxyServerMethod						'EPrx'
#define		kTelnetEnumeratedClassProxyServerMethodSOCKS4				'PMS4'
#define		kTelnetEnumeratedClassProxyServerMethodSOCKS4A				'PM4A'
#define		kTelnetEnumeratedClassProxyServerMethodSOCKS5				'PMS5'
#define kTelnetEnumeratedClassSessionStatus							'ECSt'
#define		kTelnetEnumeratedClassSessionStatusBrandNew					'CSt0'
#define		kTelnetEnumeratedClassSessionStatusWaitingForDNR			'CStD'
#define		kTelnetEnumeratedClassSessionStatusOpening					'CStO'
#define		kTelnetEnumeratedClassSessionStatusActive					'CStA'
#define		kTelnetEnumeratedClassSessionStatusDead						'CStX'
#define		kTelnetEnumeratedClassSessionStatusImminentDisposal			'CStZ'
#define kTelnetEnumeratedClassSpecialCharacter						'ESpC'
#define		kTelnetEnumeratedClassSpecialCharacterAreYouThere			'SpCY'
#define		kTelnetEnumeratedClassSpecialCharacterAbortOutput			'SpC.'
#define		kTelnetEnumeratedClassSpecialCharacterBreak					'SpC-'
#define		kTelnetEnumeratedClassSpecialCharacterCarriageReturn		'SpCR'
#define		kTelnetEnumeratedClassSpecialCharacterInterruptProcess		'SpCI'
#define		kTelnetEnumeratedClassSpecialCharacterEraseCharacter		'SpCE'
#define		kTelnetEnumeratedClassSpecialCharacterEraseLine				'SpCX'
#define		kTelnetEnumeratedClassSpecialCharacterEndOfFile				'SpCD'
#define		kTelnetEnumeratedClassSpecialCharacterLineFeed				'SpCL'
#define		kTelnetEnumeratedClassSpecialCharacterSync					'SpC='

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
