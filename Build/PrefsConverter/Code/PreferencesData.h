/*###############################################################

	PreferencesData.h
	
	This file contains the layout of preferences structures as
	they were defined in NCSA Telnet 2.6 and MacTelnet 3.0.
	The format of these structures CANNOT change, because the
	layout is (unfortunately) required to read the binary data
	from old resource files correctly.
	
	Newer MacTelnet preferences are in XML format.
	
	MacTelnet Preferences Converter
		© 2004-2011 by Kevin Grant.
	
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

#ifndef __PREFERENCESDATA__
#define __PREFERENCESDATA__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>



#pragma mark Constants

enum
{
	kEmacsMetaKeyOff = 0,
	kEmacsMetaKeyControlCommand = 1,
	kEmacsMetaKeyOption = 2
};

enum
{
	kTerminalCursorTypeBlock = 0,
	kTerminalCursorTypeUnderscore = 1,
	kTerminalCursorTypeVerticalLine	= 2,
	kTerminalCursorTypeThickUnderscore = 3,
	kTerminalCursorTypeThickVerticalLine = 4
};

#pragma mark Types

struct ApplicationPrefs
{
	// This is the general preferences structure.  Note that, as
	// a preference structure that is dumped to disk as a whole,
	// you CANNOT MOVE and CANNOT CHANGE any parts of this
	// structure that are already defined without becoming
	// backwards-incompatible with older preferences files.  You
	// can only use the padding at the end of the structure (and
	// if you do, you must decrease the padding array to
	// accommodate your additions and keep the structure size
	// EXACTLY the same as it was before).
	UInt16				version;					// The version number of this resource.
	
	UInt16				cursorType;					// A "kTerminalCursorType..." constant.
	
	UInt16				copyTableThreshold;			// Number of spaces that equal one tab.
	
	UInt16				align1;						// filler to align the next field to 32 bits.
	
	UInt32				maxTicksWaitNextEvent;		// Timeslice passed to WaitNextEvent.
	
	OSType				captureFileCreator;			// Creator signature for session capture files.
	
	UInt8				windowsDontGoAway,			// Connection windows remain open after a connection closes?
						backgroundNotification,		// Terminal bells cause a Notification Manager posting?
						menusHaveKeyEquivalents,	// Show (and recognize) key shortcuts for menus?
						remapBackquoteToEscape,		// Remap “`” to escape?
						cursorBlinks,				// Blink the cursor?
						menusShowKeypadAndFunction,	// no longer used, reserved for backwards compatibility
						visualBell,					// Bell never sounds?
						destroyKerberosTickets,		// Destroy Kerberos tickets when a window closes?
						headersInitiallyCollapsed,	// Connection window headers are initially collapsed?
						dynamicResizing,			// Dragging the size box causes window size to change?
						useBackgroundPicture,		// Terminal windows use 'PICT' 1024?
						dontDimBackgroundScreens,	// Terminal colors never change for background windows?
						newSessionsDialogOnStartup,	// Show New Sessions dialog as soon as MacTelnet starts?
						invertedTextHighlighting,	// Highlight text by inverting, not using the highlight color?
						copySelectedText,			// Automatically copy selected text?
						autoCursorMoveOnDrop,		// Automatically issue cursor movement sequences to the drop location?
						marginBell,					// Sound bell when cursor gets close to far edge of terminal screen?
						doNotInvokeNewOnApplicationReopen;	// Although Aqua Human Interface Guidelines say to create a
															// new window if none are open when an application is reopened,
															// this preference overrides that standard behavior because it
															// may be annoying for users that do not use local terminal
															// windows.
	
	UInt8				unusedFlags[8];				// space for several additional Booleans in the future
	
	UInt8				wasClipboardShowing,		// implicit window visibility state saving
						wasCommandLineShowing,
						wasControlKeypadShowing,
						wasFunctionKeypadShowing,
						wasMacroKeypadShowing,
						wasVT220KeypadShowing;
	
	UInt16				openTimeout,				// no longer used, Time to Open Connections in seconds
						sendTimeout;				// no longer used, Time to Send Data (and receive ACK) in seconds
	
	UInt8				simplifiedUserInterface,	// Use “Simplified User Interface” mode?
						directConnect;				// no longer used, reserved for backwards compatibility
	
	UInt16				notificationPrefs;			// A "kAlert_Notify..." constant.
	
	SInt16				sherlockVRefNum;
	UInt16				align2;						// to align the next field to 32 bits.
	SInt32				sherlockDirID;
	Str31				sherlockName;				// used only in alpha releases, so it can be re-used
	
	SInt16				sherlockAliasID;			// this is used if Sherlock or Find File can’t be found
	
	UInt16				align3;						// to align the next section to 32 bits.
	
	SInt16				windowStackingOriginLeft,	// top-left corner in pixels of the first window that opens
						windowStackingOriginTop;
	
	UInt32				newCommandShortcutEffect;	// A "kCommand..." constant.  Which operation command-N invokes.
	
	SInt16				padding[88];
};
typedef ApplicationPrefs*		ApplicationPrefsPtr;
typedef ApplicationPrefsPtr*	ApplicationPrefsHandle;

struct TerminalPrefs
{
	// This is the terminal preferences structure.  It specifies
	// what goes in a Terminal Configuration.  Note that, as a
	// preference structure that is dumped to disk as a whole,
	// you CANNOT MOVE and CANNOT CHANGE any parts of this
	// structure that are already defined without becoming
	// backwards-incompatible with older preferences files.  You
	// can only use the padding at the end of the structure (and
	// if you do, you must decrease the padding array to
	// accommodate your additions and keep the structure size
	// EXACTLY the same as it was before).
	SInt16			version;
	
	RGBColor		foregroundNormalColor,
					backgroundNormalColor,
					foregroundBlinkingColor,
					backgroundBlinkingColor,
					foregroundBoldColor,
					backgroundBoldColor;
	
	SInt16			emulation,				// A "kTerminalEmulator..." constant
					columnCount,			// Width of the terminal screen in characters
					rowCount,				// Height of the terminal screen in characters
					fontSize,				// Size of DisplayFont to use to display text
					scrollbackBufferSize;	// Number of lines to save in scroll buffer
	
	UInt8			usesANSIColors,			// Recognize ANSI color sequences
					usesXTermSequences,		// Recognize Xterm sequences
					usesVTWrap,				// Use VT wrap mode
					metaKey,				// Emacs meta key - simulated with Macintosh key combinations
					usesEmacsArrows,		// Arrow keys and mouse position are Emacs flavor
					mapsPageJumpKeys,		// Map PageUp, PageDown, Home, End. (MAT == Mark Tamsky)
					usesEightBits,			// Don’t strip the high bit
					savesOnClear;			// Save cleared lines
	
	Str63			normalFont;				// Font to use to display text
	
	Str32			answerBackMessage;		// Response to send when asked what terminal is being emulated
	UInt8	 		remapKeypad;			// remap keypad (2.7 CCP)
	
	SInt16			padding[98];
};
typedef TerminalPrefs*		TerminalPrefsPtr;
typedef TerminalPrefsPtr*	TerminalPrefsHandle;



struct SessionPrefs
{
	// This is the session preferences structure.  It specifies
	// what goes in a Session Favorite.  Note that, as a
	// preference structure that is dumped to disk as a whole,
	// you CANNOT MOVE and CANNOT CHANGE any parts of this
	// structure that are already defined without becoming
	// backwards-incompatible with older preferences files.  You
	// can only use the padding at the end of the structure (and
	// if you do, you must decrease the padding array to
	// accommodate your additions and keep the structure size
	// EXACTLY the same as it was before).
	UInt16			version;				// version of this structure
		
	UInt16			port;					// port to connect to
	SInt16			modeForTEK;				// a "kTektronixMode…" constant (see tekdefs.h)
	SInt16			pasteMethod;			// no longer supported
	UInt16			pasteBlockSize;			// no longer supported
	UInt16			pad1;
	
	UInt32			ipAddress;				// IP address of the host; unused
	
	UInt8			forceSave,				// unused
					mapCR,					// CR NULL newlines; unused
					lineMode,				// telnet line mode - unused
					unused,
					tekPageClears,			// clear TEK window vs. create new one
					halfDuplex;				// half duplex required
	
	SInt8			deleteMapping,			// 0 means delete sends backspace, 1 means delete sends delete
					interruptKey,			// ASCII code of control key that sends the interrupt-process sequence
					suspendKey,				// ASCII code of control key that sends the stop sequence
					resumeKey;				// ASCII code of control key that sends the start sequence
	
	Str32			terminalEmulationName,	// name of terminal emulator to use
					translationTableName;	// name of translation table to use by default
	
	Str63			hostName;				// DNS name of the host
	
	UInt8			authenticate,			// Kerberos authentication - unused
					encrypt,				// Encrypted session - unused
					localEcho,				// whether or not text sent to the server is copied to the terminal
					autoCaptureToFile;		// automatically begin a file capture when this session opens
	
	SInt16			netBlockSize;			// size of read buffer
	
	SInt16			captureFileAliasID;		// ID of 'alis' resource identifying auto-capture destination file
	
	Str32			proxyServer;			// name of the proxy server configuration - unused
	UInt8			pad2;
	UInt16			pad3;
	
	UInt32			translationEncoding;	// destination encoding for translated text, by default
	
	SInt16			padding[78];
};
typedef SessionPrefs*		SessionPrefsPtr;
typedef SessionPrefsPtr*	SessionPrefsHandle;

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
