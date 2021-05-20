// public framework header for MacTermQuills.framework
#ifndef MACTERMQUILLS_H
#define MACTERMQUILLS_H

#import <CoreFoundation/CoreFoundation.h>


//
// SwiftUI-Dependent Enumerations
//
// The following enumerations are directly used in SwiftUI and
// must therefore be exported from the main framework instead
// of being limited to internal header files.  The compiler
// only allows framework-style #include statements in this file
// so each "enum" was MOVED from its internal header instead of
// putting a #include reference here.
//
// Each "enum" must also have a Swift equivalent, which is
// handled by macOS macros such as CF_CLOSED_ENUM(...) below.
// These use CF_...ENUM() instead of NS_...ENUM() because they
// can be dependencies of pure C/C++ code, and would not be
// able to build against the <Foundation/Foundation.h> header.
//
// NOTE: Swift has certain conventions for determining the
// default “Swift name” for the values below so SwiftUI code
// would use those auto-converted names.  For example, the
// name "kSession_FunctionKeyLayoutXTerm" is conventionally
// "Session_FunctionKeyLayout.xTerm" or just ".xTerm" within
// the SwiftUI bindings.
//

/*!
Pass one of these to Alert_SetNotificationPreferences()
to decide how the application should respond to alerts
that appear in the background.
*/
typedef CF_CLOSED_ENUM(SInt16, AlertMessages_NotificationType)
{
	kAlertMessages_NotificationTypeDoNothing						= 0,	//! no action
	kAlertMessages_NotificationTypeMarkDockIcon						= 1,	//! icon is badged without animation
	kAlertMessages_NotificationTypeMarkDockIconAndBounceOnce		= 2,	//! icon is badged and bounces once
	kAlertMessages_NotificationTypeMarkDockIconAndBounceRepeatedly	= 3		//! icon is badged and bounces until the user responds
};

/*!
Possible ways for macros to interpret their content and act on it. 
*/
typedef CF_CLOSED_ENUM(UInt32, MacroManager_Action)
{
	kMacroManager_ActionSendTextVerbatim			= 'MAEV',	//!< macro content is a string to send as-is (no
																//!  metacharacters allowed)
	kMacroManager_ActionSendTextProcessingEscapes	= 'MAET',	//!< macro content is a string to send (perhaps
																//!  with metacharacters to be substituted)
	kMacroManager_ActionHandleURL					= 'MAOU',	//!< macro content is a URL to be opened
	kMacroManager_ActionNewWindowWithCommand		= 'MANW',	//!< macro content is a Unix command line to be
																//!  executed in a new terminal window
	kMacroManager_ActionSelectMatchingWindow		= 'MASW',	//!< macro content is a string to use as a search
																//!  key against the titles of open windows; the
																//!  next window with a matching title is activated
	kMacroManager_ActionFindTextVerbatim			= 'MAFV',	//!< macro content is a string to send as-is (no
																//!  metacharacters allowed)
	kMacroManager_ActionFindTextProcessingEscapes	= 'MAFS'	//!< macro content is a string to send (perhaps
																//!  with metacharacters to be substituted)
};

/*!
Predefined virtual keys that are selectable in Preferences
(additional key bindings are implied by using an “ordinary
key” selection and additional characters). 
*/
typedef CF_CLOSED_ENUM(UInt32, MacroManager_KeyBinding)
{
	kMacroManager_KeyBindingOrdinaryCharacter	= 0,	//!< additional character(s) required to define fully (e.g. binding is a letter key)
	kMacroManager_KeyBindingBackwardDelete		= 1,	//!< backward delete (⌫) key
	kMacroManager_KeyBindingForwardDelete		= 2,	//!< forward delete (⌦) key
	kMacroManager_KeyBindingHome				= 3,	//!< home key
	kMacroManager_KeyBindingEnd					= 4,	//!< end key
	kMacroManager_KeyBindingPageUp				= 5,	//!< page up key
	kMacroManager_KeyBindingPageDown			= 6,	//!< page down key
	kMacroManager_KeyBindingUpArrow				= 7,	//!< up arrow key
	kMacroManager_KeyBindingDownArrow			= 8,	//!< down arrow key
	kMacroManager_KeyBindingLeftArrow			= 9,	//!< left arrow key
	kMacroManager_KeyBindingRightArrow			= 10,	//!< right arrow key
	kMacroManager_KeyBindingClear				= 11,	//!< clear (⌧) key
	kMacroManager_KeyBindingEscape				= 12,	//!< escape key
	kMacroManager_KeyBindingReturn				= 13,	//!< return key
	kMacroManager_KeyBindingEnter				= 14,	//!< enter key
	kMacroManager_KeyBindingFunctionKeyF1		= 15,	//!< F1
	kMacroManager_KeyBindingFunctionKeyF2		= 16,	//!< F2
	kMacroManager_KeyBindingFunctionKeyF3		= 17,	//!< F3
	kMacroManager_KeyBindingFunctionKeyF4		= 18,	//!< F4
	kMacroManager_KeyBindingFunctionKeyF5		= 19,	//!< F5
	kMacroManager_KeyBindingFunctionKeyF6		= 20,	//!< F6
	kMacroManager_KeyBindingFunctionKeyF7		= 21,	//!< F7
	kMacroManager_KeyBindingFunctionKeyF8		= 22,	//!< F8
	kMacroManager_KeyBindingFunctionKeyF9		= 23,	//!< F9
	kMacroManager_KeyBindingFunctionKeyF10		= 24,	//!< F10
	kMacroManager_KeyBindingFunctionKeyF11		= 25,	//!< F11
	kMacroManager_KeyBindingFunctionKeyF12		= 26,	//!< F12
	kMacroManager_KeyBindingFunctionKeyF13		= 27,	//!< F13
	kMacroManager_KeyBindingFunctionKeyF14		= 28,	//!< F14
	kMacroManager_KeyBindingFunctionKeyF15		= 29,	//!< F15 
	kMacroManager_KeyBindingFunctionKeyF16		= 30	//!< F16
};

/*!
Modifier keys that are supported by macros.
*/
typedef CF_OPTIONS(UInt32, MacroManager_ModifierKeyMask)
{
	kMacroManager_ModifierKeyMaskCommand	= (1 << 0),		//!< command key (⌘)
	kMacroManager_ModifierKeyMaskControl	= (1 << 1),		//!< control key (⌃)
	kMacroManager_ModifierKeyMaskOption		= (1 << 2),		//!< option key (⌥)
	kMacroManager_ModifierKeyMaskShift		= (1 << 3)		//!< shift key (⇧)
};

/*!
Possible mappings to simulate a meta key on a Mac keyboard
(useful for the Emacs text editor).
*/
typedef CF_CLOSED_ENUM(UInt16, Session_EmacsMetaKey)
{
	kSession_EmacsMetaKeyOff = 0,				//!< no mapping
	kSession_EmacsMetaKeyShiftOption = 1,		//!< by holding down shift and option keys, meta is simulated
	kSession_EmacsMetaKeyOption = 2				//!< by holding down option key, meta is simulated
};

/*!
The keyboard layout to assume when a numbered function
key is activated by Session_UserInputFunctionKey().

Note that currently all keyboard layouts send exactly
the same sequences for keys F5-F12, but can differ
significantly for other ranges of keys.
*/
typedef CF_CLOSED_ENUM(UInt32, Session_FunctionKeyLayout)
{
	kSession_FunctionKeyLayoutVT220			= 0,	//!< keys F6 through F20 send traditional VT220 sequences, and since they do not overlap
													//!  F1-F4 are mapped to the VT100 PF1-PF4; F5 is mapped to the XTerm value; this is also
													//!  known as the “multi-gnome-terminal” layout
	kSession_FunctionKeyLayoutXTerm			= 1,	//!< keys F1 through F12 are similar to VT100 and VT220; keys F13-F48 send XTerm sequences
	kSession_FunctionKeyLayoutXTermXFree86	= 2,	//!< similar to "kSession_FunctionKeyLayoutX11XTerm", except that the following key ranges
													//!  send the values defined by the XTerm on XFree86: F1-F4, F13-F16, F25-F28, F37-F40;
													//!  also known as the “gnome-terminal” layout, and a superset of what GNU “screen” uses
	kSession_FunctionKeyLayoutRxvt			= 3		//!< very similar to "kSession_FunctionKeyLayoutVT220"; but F1-F4 follow XTerm instead of
													//!  the VT100, F21-F44 have completely unique mappings, and there is no F45-F48
};

/*!
Which characters will be sent when a new-line is requested.
*/
typedef CF_CLOSED_ENUM(UInt16, Session_NewlineMode)
{
	kSession_NewlineModeMapCR		= 0,	//!< newline means “carriage return” only (Classic Mac OS systems)
	kSession_NewlineModeMapCRLF		= 1,	//!< newline means “carriage return, line feed” (MS-DOS or Windows systems)
	kSession_NewlineModeMapCRNull	= 2,	//!< BSD 4.3 Unix; newline means “carriage return, null”
	kSession_NewlineModeMapLF		= 3		//!< newline means “line feed” only (Unix systems)
};

/*!
Protocols supported by a Session.
*/
typedef CF_CLOSED_ENUM(UInt16, Session_Protocol)
{
	kSession_ProtocolSFTP			= 0,	//!< secure file transfer protocol
	kSession_ProtocolSSH1			= 1,	//!< secure shell protocol, version 1
	kSession_ProtocolSSH2			= 2,	//!< secure shell protocol, version 2
};

/*!
Determines the shape of the cursor, when rendered.
*/
typedef CF_CLOSED_ENUM(UInt16, Terminal_CursorType)
{
	kTerminal_CursorTypeBlock					= 0,	//!< solid, filled rectangle
	kTerminal_CursorTypeUnderscore				= 1,	//!< 1-pixel-high underline
	kTerminal_CursorTypeVerticalLine			= 2,	//!< standard Mac insertion point appearance
	kTerminal_CursorTypeThickUnderscore			= 3,	//!< 2-pixel-high underscore, makes cursor easier to see
	kTerminal_CursorTypeThickVerticalLine		= 4		//!< 2-pixel-wide vertical line, makes cursor easier to see
};

/*!
How scrollback lines are allocated.
*/
typedef CF_CLOSED_ENUM(UInt16, Terminal_ScrollbackType)
{
	kTerminal_ScrollbackTypeDisabled		= 0,	//!< no lines are saved
	kTerminal_ScrollbackTypeFixed			= 1,	//!< a specific number of rows is read from the preferences
	kTerminal_ScrollbackTypeUnlimited		= 2,	//!< rows are allocated continuously, memory permitting
	kTerminal_ScrollbackTypeDistributed		= 3		//!< allocations favor the active window and starve rarely-used windows
};

/*!
The command set, which determines how input data streams
are interpreted.
*/
typedef CF_CLOSED_ENUM(UInt16, VectorInterpreter_Mode)
{
	kVectorInterpreter_ModeDisabled		= 0,	//!< TEK 4014 command set
	kVectorInterpreter_ModeTEK4014		= 4014,	//!< TEK 4014 command set
	kVectorInterpreter_ModeTEK4105		= 4105	//!< TEK 4105 command set
};

#endif
