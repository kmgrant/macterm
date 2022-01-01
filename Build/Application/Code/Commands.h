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
		© 1998-2022 by Kevin Grant.
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

These are all deprecated and are being gradually replaced
by methods on objects in the Cocoa responder chain.
*/

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
Actions that involve the invocation of macros.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_MacroInvoking //{

@required

// actions
	- (IBAction)
	performActionForMacro:(id _Nullable)_;

@end //}

/*!
Actions to change the current macro set.  For other types
of key bindings, see Commands_TerminalKeyMapping.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_MacroSwitching //{

@optional

// actions
	- (IBAction)
	performMacroSwitchNone:(id _Nullable)_;
	- (IBAction)
	performMacroSwitchDefault:(id _Nullable)_;
	- (IBAction)
	performMacroSwitchByFavoriteName:(id _Nullable)_;
	- (IBAction)
	performMacroSwitchNext:(id _Nullable)_;
	- (IBAction)
	performMacroSwitchPrevious:(id _Nullable)_;

@end //}

/*!
Actions related to printing.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_Printing //{

@required

// actions
	- (IBAction)
	performPrintScreen:(id _Nullable)_;
	- (IBAction)
	performPrintSelection:(id _Nullable)_;

@end //}

/*!
Actions for killing or restarting running processes.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_SessionProcessControlling //{

@optional

// actions
	- (IBAction)
	performKill:(id _Nullable)_;
	- (IBAction)
	performRestart:(id _Nullable)_;

@end //}

/*!
Actions for controlling data flow of a session, such as
suspend and resume.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_SessionThrottling //{

@optional

// actions
	- (IBAction)
	performInterruptProcess:(id _Nullable)_;
	- (IBAction)
	performJumpScrolling:(id _Nullable)_;
	- (IBAction)
	performSuspendToggle:(id _Nullable)_;

@end //}

/*!
Actions for accessing text via standard system commands;
see also Commands_TerminalEditing.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardEditing //{

@required

// actions
	- (IBAction)
	copy:(id _Nullable)_;

@optional

// actions
	- (IBAction)
	paste:(id _Nullable)_;
	- (IBAction)
	selectAll:(id _Nullable)_;
	- (IBAction)
	selectNone:(id _Nullable)_;

@end //}

/*!
Actions based on finding locations within a text buffer.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardSearching //{

@required

// actions
	- (IBAction)
	performFind:(id _Nullable)_;

@optional

// actions
	- (IBAction)
	performFindCursor:(id _Nullable)_;
	- (IBAction)
	performFindNext:(id _Nullable)_;
	- (IBAction)
	performFindPrevious:(id _Nullable)_;
	- (IBAction)
	performSendMenuItemText:(id _Nullable)_; // from completions menu; “types” the menu item title text
	- (IBAction)
	performShowCompletions:(id _Nullable)_;

@end //}

/*!
Actions for speech control via standard system commands.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardSpeechHandling //{

@required

// actions
	- (IBAction)
	performSpeechToggle:(id _Nullable)_;
	- (IBAction)
	startSpeaking:(id _Nullable)_;
	- (IBAction)
	stopSpeaking:(id _Nullable)_;

@end //}

/*!
Actions for performing an action or reversing it.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardUndoRedo //{

@required

// actions
	- (IBAction)
	performUndo:(id _Nullable)_;

@optional

// actions
	- (IBAction)
	performRedo:(id _Nullable)_;

@end //}

/*!
Actions for entering or exiting Full Screen view, or
otherwise changing the size of a view quickly.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardViewZooming //{

@optional

// actions
	- (IBAction)
	performMaximize:(id _Nullable)_;
	- (IBAction)
	toggleFullScreen:(id _Nullable)_;

@end //}

/*!
Actions that apply to all regular open windows.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardWindowGrouping //{

@optional

// actions
	- (IBAction)
	performArrangeInFront:(id _Nullable)_;
	- (IBAction)
	performCloseAll:(id _Nullable)_;
	- (IBAction)
	performMiniaturizeAll:(id _Nullable)_;
	- (IBAction)
	performZoomAll:(id _Nullable)_;

@end //}

/*!
Actions to cycle through windows.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardWindowSwitching //{

@required

// actions
	- (IBAction)
	orderFrontNextWindow:(id _Nullable)_;
	- (IBAction)
	orderFrontNextWindowHidingPrevious:(id _Nullable)_;
	- (IBAction)
	orderFrontPreviousWindow:(id _Nullable)_;
	- (IBAction)
	orderFrontSpecificWindow:(id _Nullable)_;

@end //}

/*!
Actions for using window tabs via standard system commands.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_StandardWindowTabbing //{

@optional

// actions
	- (IBAction)
	mergeAllWindows:(id _Nullable)_;
	- (IBAction)
	moveTabToNewWindow:(id _Nullable)_;

@end //}

/*!
Actions for terminal-specific editing commands; see also
Commands_StandardEditing.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalEditing //{

@required

// actions
	- (IBAction)
	performAssessBracketedPasteMode:(id _Nullable)_; // not a real action; used for updating menu state
	- (IBAction)
	performCopyWithTabSubstitution:(id _Nullable)_;
	- (IBAction)
	performSelectEntireScrollbackBuffer:(id _Nullable)_;

@optional

// actions
	- (IBAction)
	performCopyAndPaste:(id _Nullable)_;

@end //}

/*!
Actions to configure terminal event handlers.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalEventHandling //{

@required

// actions
	- (IBAction)
	performBellToggle:(id _Nullable)_;
	- (IBAction)
	performSetActivityHandlerNone:(id _Nullable)_;
	- (IBAction)
	performSetActivityHandlerNotifyOnNext:(id _Nullable)_;
	- (IBAction)
	performSetActivityHandlerNotifyOnIdle:(id _Nullable)_;
	- (IBAction)
	performSetActivityHandlerSendKeepAliveOnIdle:(id _Nullable)_;

@end //}

/*!
Actions to control the capture of data to a file.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalFileCapturing //{

@required

// actions
	- (IBAction)
	performCaptureBegin:(id _Nullable)_;
	- (IBAction)
	performCaptureEnd:(id _Nullable)_;
	- (IBAction)
	performSaveSelection:(id _Nullable)_;

@end //}

/*!
Actions affecting keyboard behavior in terminal windows.
See also Commands_MacroSwitching.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalKeyMapping //{

@required

// actions
	- (IBAction)
	performDeleteMapToBackspace:(id _Nullable)_;
	- (IBAction)
	performDeleteMapToDelete:(id _Nullable)_;
	- (IBAction)
	performEmacsCursorModeToggle:(id _Nullable)_;
	- (IBAction)
	performLocalPageKeysToggle:(id _Nullable)_;
	- (IBAction)
	performMappingCustom:(id _Nullable)_;
	- (IBAction)
	performSetFunctionKeyLayoutRxvt:(id _Nullable)_;
	- (IBAction)
	performSetFunctionKeyLayoutVT220:(id _Nullable)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermX11:(id _Nullable)_;
	- (IBAction)
	performSetFunctionKeyLayoutXTermXFree86:(id _Nullable)_;

@end //}

/*!
Actions to change various terminal behaviors.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalModeSwitching //{

@required

// actions
	- (IBAction)
	performLineWrapToggle:(id _Nullable)_;
	- (IBAction)
	performLocalEchoToggle:(id _Nullable)_;
	- (IBAction)
	performReset:(id _Nullable)_;
	- (IBAction)
	performSaveOnClearToggle:(id _Nullable)_;
	- (IBAction)
	performScrollbackClear:(id _Nullable)_;
	- (IBAction)
	performTerminalLED1Toggle:(id _Nullable)_;
	- (IBAction)
	performTerminalLED2Toggle:(id _Nullable)_;
	- (IBAction)
	performTerminalLED3Toggle:(id _Nullable)_;
	- (IBAction)
	performTerminalLED4Toggle:(id _Nullable)_;

@end //}

/*!
Actions to change the displayed part of a terminal view.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalScreenPaging //{

@required

// actions
	- (IBAction)
	performTerminalViewPageDown:(id _Nullable)_;
	- (IBAction)
	performTerminalViewPageEnd:(id _Nullable)_;
	- (IBAction)
	performTerminalViewPageHome:(id _Nullable)_;
	- (IBAction)
	performTerminalViewPageUp:(id _Nullable)_;

@end //}

/*!
Actions to change number of rows/columns in terminal views.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TerminalScreenResizing //{

@optional

// actions
	- (IBAction)
	performScreenResizeCustom:(id _Nullable)_;
	- (IBAction)
	performScreenResizeNarrower:(id _Nullable)_;
	- (IBAction)
	performScreenResizeShorter:(id _Nullable)_;
	- (IBAction)
	performScreenResizeStandard:(id _Nullable)_;
	- (IBAction)
	performScreenResizeTall:(id _Nullable)_;
	- (IBAction)
	performScreenResizeTaller:(id _Nullable)_;
	- (IBAction)
	performScreenResizeWide:(id _Nullable)_;
	- (IBAction)
	performScreenResizeWider:(id _Nullable)_;

@end //}

/*!
Actions that affect fonts, colors and other format settings.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_TextFormatting //{

@optional

// actions
	- (IBAction)
	performFormatByFavoriteName:(id _Nullable)_;
	- (IBAction)
	performFormatCustom:(id _Nullable)_;
	- (IBAction)
	performFormatDefault:(id _Nullable)_;
	- (IBAction)
	performFormatTextBigger:(id _Nullable)_;
	- (IBAction)
	performFormatTextMaximum:(id _Nullable)_;
	- (IBAction)
	performFormatTextSmaller:(id _Nullable)_;
	- (IBAction)
	performTranslationSwitchByFavoriteName:(id _Nullable)_;
	- (IBAction)
	performTranslationSwitchCustom:(id _Nullable)_;
	- (IBAction)
	performTranslationSwitchDefault:(id _Nullable)_;

@end //}

/*!
Actions for opening a selected URL.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_URLSelectionHandling //{

@required

// actions
	- (IBAction)
	performOpenURL:(id _Nullable)_;

@end //}

/*!
Actions related to existing vector graphics windows.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_VectorGraphicsModifying //{

@required

// actions
	- (IBAction)
	performGraphicsCanvasResizeTo100Percent:(id _Nullable)_;

@end //}

/*!
Actions related to new vector graphics windows.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_VectorGraphicsOpening //{

@required

// actions
	- (IBAction)
	performNewTEKPage:(id _Nullable)_;
	- (IBAction)
	performPageClearToggle:(id _Nullable)_;

@end //}

/*!
Action to specify the name of a window.

(Described as a protocol so that selector names appear in
one location.  These are actually implemented at different
points in the responder chain, such as views or windows.)
*/
@protocol Commands_WindowRenaming //{

@required

// actions
	- (IBAction)
	performRename:(id _Nullable)_;

@end //}

/*!
Implements an interface for menu commands to target.  See
"MainMenuCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Commands_Executor : NSObject< Commands_MacroInvoking,
											Commands_MacroSwitching,
											Commands_StandardUndoRedo,
											Commands_StandardViewZooming,
											Commands_StandardWindowGrouping,
											Commands_StandardWindowSwitching,
											Commands_VectorGraphicsOpening,
											NSUserInterfaceValidations > //{

// class methods
	+ (instancetype _Nullable)
	sharedExecutor;

// new methods: explicit validation (rarely needed)
	- (BOOL)
	defaultValidationForAction:(SEL _Nonnull)_
	sourceItem:(id <NSValidatedUserInterfaceItem> _Nullable)_;
	- (BOOL)
	validateAction:(SEL _Nonnull)_
	sender:(id _Nullable)_
	sourceItem:(id <NSValidatedUserInterfaceItem> _Nullable)_;

// new methods: menu items
	- (NSMenuItem* _Nullable)
	newMenuItemForAction:(SEL _Nonnull)_
	itemTitle:(NSString* _Nonnull)_
	ifEnabled:(BOOL)_;

@end //}

/*!
Implements NSApplicationDelegate and NSApplicationNotifications;
see Commands.mm.
*/
@interface Commands_Executor (Commands_ApplicationCoreEvents) @end

/*!
Actions that create new terminal-based sessions.
*/
@interface Commands_Executor (Commands_OpeningSessions) //{

// actions
	- (IBAction)
	performNewDefault:(id _Nullable)_;
	- (IBAction)
	performNewByFavoriteName:(id _Nullable)_;
	- (IBAction)
	performNewLogInShell:(id _Nullable)_;
	- (IBAction)
	performNewShell:(id _Nullable)_;
	- (IBAction)
	performNewCustom:(id _Nullable)_;
	- (IBAction)
	performRestoreWorkspaceDefault:(id _Nullable)_;
	- (IBAction)
	performRestoreWorkspaceByFavoriteName:(id _Nullable)_;
	- (IBAction)
	performOpen:(id _Nullable)_;
	- (IBAction)
	performDuplicate:(id _Nullable)_;
	- (IBAction)
	performSaveAs:(id _Nullable)_;

// methods of the form required by setEventHandler:andSelector:forEventClass:andEventID:
	- (void)
	receiveGetURLEvent:(NSAppleEventDescriptor* _Nonnull)_
	replyEvent:(NSAppleEventDescriptor* _Nonnull)_;

@end //}

/*!
Actions that cause Internet addresses to be accessed.
*/
@interface Commands_Executor (Commands_OpeningWebPages) //{

// actions
	- (IBAction)
	performCheckForUpdates:(id _Nullable)_;
	- (IBAction)
	performGoToMainWebSite:(id _Nullable)_;
	- (IBAction)
	performProvideFeedback:(id _Nullable)_;

@end //}

/*!
Actions that affect a window’s properties, placement and size.
*/
@interface Commands_Executor (Commands_ModifyingWindows) //{

// actions
	- (IBAction)
	performHideWindow:(id _Nullable)_;
	- (IBAction)
	performHideOtherWindows:(id _Nullable)_;
	- (IBAction)
	performMoveWindowRight:(id _Nullable)_;
	- (IBAction)
	performMoveWindowLeft:(id _Nullable)_;
	- (IBAction)
	performMoveWindowDown:(id _Nullable)_;
	- (IBAction)
	performMoveWindowUp:(id _Nullable)_;
	- (IBAction)
	performShowHiddenWindows:(id _Nullable)_;

@end //}

/*!
Actions that display specific windows.
*/
@interface Commands_Executor (Commands_ShowingPanels) //{

// actions
	- (IBAction)
	orderFrontAbout:(id _Nullable)_;
	- (IBAction)
	orderFrontCommandLine:(id _Nullable)_;
	- (IBAction)
	orderFrontContextualHelp:(id _Nullable)_;
	- (IBAction)
	orderFrontControlKeys:(id _Nullable)_;
	- (IBAction)
	orderFrontDebuggingOptions:(id _Nullable)_;
	- (IBAction)
	orderFrontIPAddresses:(id _Nullable)_;
	- (IBAction)
	orderFrontPreferences:(id _Nullable)_;
	- (IBAction)
	orderFrontSessionInfo:(id _Nullable)_;
	- (IBAction)
	orderFrontVT220FunctionKeys:(id _Nullable)_;
	- (IBAction)
	orderFrontVT220Keypad:(id _Nullable)_;
	- (IBAction)
	toggleClipboard:(id _Nullable)_;

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
											 void* _Nullable			inUnusedLegacyPtr = nullptr);

#ifdef __OBJC__
Boolean
	Commands_ViaFirstResponderPerformSelector	(SEL _Nonnull			inSelector,
											 id _Nullable				inObjectOrNil = nil);
#endif

//@}

//!\name Cocoa Menu Utilities
//@{

Commands_Result
	Commands_InsertPrefNamesIntoMenu		(Quills::Prefs::Class		inClass,
											 NSMenu* _Nonnull			inoutMenu,
											 SInt64						inAtItemIndex,
											 SInt16						inInitialIndent,
											 SEL _Nonnull				inAction);

#ifdef __OBJC__
NSMenuItem* _Nullable
	Commands_NewMenuItemForAction			(SEL _Nonnull				inActionSelector,
											 CFStringRef _Nonnull		inPreferredTitle,
											 Boolean					inMustBeEnabled = false);
#endif

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
