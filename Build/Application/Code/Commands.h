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
		© 1998-2018 by Kevin Grant.
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
#define kCommandFullScreenToggle				'Kios'

// File menu
#define kCommandHandleURL						'HURL'
#define kCommandSaveSelection					'SvSl'
#define kCommandCaptureToFile					'Capt'
#define kCommandEndCaptureToFile				'CapE'
#define kCommandPrint							kHICommandPrint
#define kCommandPrintScreen						'PrSc'

// Edit menu
// WARNING: These are referenced by value in the MainMenus.nib file!
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
#define kCommandZoomMaximumSize					'ZmMx'
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
#define kCommandShowConnectionStatus			'ShCS'
#define kCommandHideConnectionStatus			'HiCS'

// Help menu
#define kCommandMainHelp						kHICommandAppHelp
#define kCommandContextSensitiveHelp			'?Ctx'

// terminal view page control
#define kCommandTerminalViewPageUp				'TVPU'
#define kCommandTerminalViewPageDown			'TVPD'
#define kCommandTerminalViewHome				'TVPH'
#define kCommandTerminalViewEnd					'TVPE'

// commands currently used only in dialogs
#define kCommandDisplayPrefPanelFormats			'SPrF'		// “Preferences“ window
#define kCommandDisplayPrefPanelFormatsANSI		'SPFA'		// multiple interfaces
#define kCommandDisplayPrefPanelFormatsNormal	'SPFN'		// multiple interfaces
#define kCommandDisplayPrefPanelGeneral			'SPrG'		// “Preferences“ window
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
#define kCommandTerminalNewWorkspace			'MTab'		// terminal window tab drawers

// commands used only in contextual menus
#define kCommandSpeakSelectedText				'SpkS'
#define kCommandStopSpeaking					'SpkE'

/*!
These MUST agree with "MainMenuCocoa.xib".  In the Carbon days,
these were menu IDs.  For Cocoa, they are the "tag" values on
each of the menus in the main menu.  So you could ask NSApp for
"mainMenu", and then use "itemWithTag" with an ID below to find
an NSMenuItem for the title, whose "submenu" is the NSMenu
containing all of the items.
*/
enum
{
	kCommands_MenuIDApplication = 512,
	kCommands_MenuIDFile = 513,
	kCommands_MenuIDEdit = 514,
	kCommands_MenuIDView = 515,
	kCommands_MenuIDTerminal = 516,
	kCommands_MenuIDKeys = 517,
	kCommands_MenuIDMacros = 518,
	kCommands_MenuIDWindow = 519,
		kCommands_MenuItemIDPrecedingWindowList = 123,
	kCommands_MenuIDHelp = 520,
	kCommands_MenuIDDebug = 521
};

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

// new methods: menu items
	- (NSMenuItem*)
	newMenuItemForAction:(SEL)_
	itemTitle:(NSString*)_
	ifEnabled:(BOOL)_;

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
	- (IBAction)
	performTerminalLED1Toggle:(id)_;
	- (IBAction)
	performTerminalLED2Toggle:(id)_;
	- (IBAction)
	performTerminalLED3Toggle:(id)_;
	- (IBAction)
	performTerminalLED4Toggle:(id)_;

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
	performFormatTextMaximum:(id)_;

@end //}

/*!
Actions that affect a window’s properties, placement and size.
*/
@interface Commands_Executor (Commands_ModifyingWindows) //{

// actions
	- (IBAction)
	performCloseAll:(id)_;
	- (IBAction)
	performMiniaturizeAll:(id)_;
	- (IBAction)
	performZoomAll:(id)_;
	- (IBAction)
	mergeAllWindows:(id)_; // match OS name of selector (available only in later OS versions)
	- (IBAction)
	moveTabToNewWindow:(id)_; // match OS name of selector (available only in later OS versions)
	- (IBAction)
	performArrangeInFront:(id)_;
	- (IBAction)
	performHideWindow:(id)_;
	- (IBAction)
	performHideOtherWindows:(id)_;
	- (IBAction)
	performMaximize:(id)_;
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
	- (IBAction)
	toggleClipboard:(id)_;

@end //}

/*!
Actions to enter or exit Full Screen or tab Exposé.
*/
@interface Commands_Executor (Commands_SwitchingModes) //{

// actions
	- (IBAction)
	toggleFullScreen:(id)_;
	- (IBAction)
	toggleTabOverview:(id)_; // match OS name of selector (available only in later OS versions)

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
	- (IBAction)
	toggleTabBar:(id)_; // match OS name of selector (available only in later OS versions)

@end //}

/*!
Temporary methods.
*/
@interface Commands_Executor (Commands_TransitionFromCarbon) //{

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

#ifdef __OBJC__
Boolean
	Commands_ViaFirstResponderPerformSelector	(SEL					inSelector,
											 id							inObjectOrNil = nil);
#endif

//@}

//!\name Cocoa Menu Utilities
//@{

Commands_Result
	Commands_InsertPrefNamesIntoMenu		(Quills::Prefs::Class		inClass,
											 NSMenu*					inoutMenu,
											 SInt64						inAtItemIndex,
											 SInt16						inInitialIndent,
											 SEL						inAction);

#ifdef __OBJC__
NSMenuItem*
	Commands_NewMenuItemForAction			(SEL						inActionSelector,
											 CFStringRef				inPreferredTitle,
											 Boolean					inMustBeEnabled = false);
#endif

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
