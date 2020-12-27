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
