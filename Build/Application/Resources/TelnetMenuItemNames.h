/*
**************************************************************************
	TelnetMenuItemNames.h
	
	MacTelnet
		© 1998-2005 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains strings for menu command names.
	
	Try to view this file in the font “Monaco 10”.
	
	Menu names are the words that appear in the menu bar at the top of
	the screen while the program is running, and are represented by the
	#defines beginning with “MENUNAME_...”.
	
	Submenu names are used for menu items that have a triangle beside
	them (“cascade menus” or “hierarchical menus”), and are named using
	#defines beginning with “SUBMENUNAME_...”.
**************************************************************************
*/

#ifndef __TELNETMENUITEMNAMES__
#define __TELNETMENUITEMNAMES__

// MacTelnet includes
#include "ApplicationVersion.h"



#define MENUNAME_FILE			"Workspace"
#define MENUNAME_EDIT			"Edit"
#define MENUNAME_VIEW			"View"
#define MENUNAME_TERMINAL		"Terminal"
#define MENUNAME_KEYBOARD		"Keys"
#define MENUNAME_NETWORK		"Network"
#define MENUNAME_WINDOW			"Window"
#define MENUNAME_SCRIPTS		"Scripts"

#define SUBMENUNAME_FAV			"New Session From Favorites"
#define SUBMENUNAME_PREFS		"Preferences"
#define SUBMENUNAME_TEK			"TEK"
#define SUBMENUNAME_WEB			"Internet Resources"
#define SUBMENUNAME_SETWINDOWWS	"Set Workspace"

// attributes applicable to all “in-menu titles” (which are basically disabled items)
#define MARK_MENUTITLES			noMark
#define STYLE_MENUTITLES		bold | underline

// Notes on “INAME_...” Constants
//
// The suffixes of these constants is the capitalized
// version of everything following the corresponding
// "kCommand..." constant in "MenuResources.h".

/*
Some menu commands can appear in contextual menus.
These items have two “versions”: the menu item
version, and the contextual menu item version that
is more sensible for the context of the item.  Any
command that can appear in a contextual menu also
has a “CINAME_...” definition that should reflect
the item when it is applied to something in
particular.

Key names (in “KEY_...” defines) can be duplicated,
because they refer only to the character, not the
modifier keys.  For example, Command-W has the same
key as Command-Option-W, but these are different
menu item equivalents.

Try not to change “MODIFIER_...” constants.  They
are there to give flexibility in localization, but
are better left untouched.  Hopefully you can just
change the “KEY_...” and that will be sufficient.

Items whose text is dynamic is represented here as
an arbitrary description in angle brackets <>, and
NEED NOT be localized.  Following each such name
are an indented list of dynamic names, which should
be localized.

Items that display submenus are not listed below;
they are listed as “SUBMENUNAME_...” defines, above.
*/
// 
#define INAME_ABOUT						"About MacTelnet"

// Workspace
#define INAME_NEWSESSION				"<new>"
#define		NORMAL_NEWSESSION					"New Shell Session"			// creates a session window running the user’s default shell
#define		OPTION_NEWSESSION					"New Login Shell Session"	// creates a session window running /usr/bin/login
#define		KEY_NEWSESSION						"N"
#define		KEYCHAR_NEWSESSION					'N' // make this the same as the above; also used for the dialog-based command, below
#define INAME_NEWSESSIONDIALOG			"New Session…"					// displays a dialog for opening one or more new connections
#define INAME_OPENSESSION				"Open Workspace…"				// displays a file dialog box to load a session configuration set
#define		KEY_OPENSESSION						"O"
#define INAME_CLOSECONNECTION			"<close>"
#define		NORMAL_CLOSECONNECTION				"Close Window"			// destroys the frontmost window
#define		OPTION_CLOSECONNECTION				"Close All Windows"		// destroys all windows
#define		CINAME_CLOSECONNECTION				"Close Window"			// Close command, in the context of a specific window
#define		KEY_CLOSECONNECTION					"W"
#define		NORMAL_DISCONNECT					"Disconnect"			// terminates the frontmost window’s connection
#define		OPTION_DISCONNECT					"Disconnect All"		// terminates all connections
#define		CINAME_DISCONNECT					"Disconnect"			// Disconnect command, in the context of a specific window
#define		NORMAL_KILLPROCESS					"Kill Process"			// kills the process running in the frontmost window
#define		OPTION_KILLPROCESS					"Kill All"				// kills processes running in all windows
#define		CINAME_KILLPROCESS					"Kill Process"			// Kill Process command, in the context of a specific window
#define INAME_CLOSEWORKSPACE			"Close Workspace"				// closes every window belonging to the workspace of the frontmost window
#define		KEY_CLOSEWORKSPACE					"W"
#define INAME_SAVESESSION				"Save Workspace As…"			// displays a file dialog box to save a session configuration set
#define		KEY_SAVESESSION						"S"
#define INAME_SAVETEXT					"Save Selected Text…"			// displays a file dialog box to save selected text to a file
#define		CINAME_SAVETEXT						"Save Selected Text…"
#define		KEY_SAVETEXT						"S"
#define INAME_IMPORTMACROSET			"Import Macro Set…"				// displays a file dialog box to import a macro set
#define INAME_EXPORTCURRENTMACROSET		"Export Current Macro Set…"		// displays a file dialog box to export the current set of macros
#define INAME_NEWDUPLICATESESSION		"Duplicate Session"				// opens a new session with attributes identical to those of the frontmost one
#define		KEY_NEWDUPLICATESESSION				"D"
#define INAME_HANDLEURL					"Open Selected URL"				// treats the selected text as a URL and handles it
#define		CINAME_HANDLEURL					"Open Selected URL"
#define		KEY_HANDLEURL						"U"
#define INAME_PAGESETUP					"Page Setup…"					// displays the standard Page Setup dialog box for printing selections
#define		KEY_PAGESETUP						"P"
#define INAME_PRINT						"Print Selection…"				// displays a standard print dialog to print selected text or graphics
#define		KEY_PRINT							"P"
#define INAME_PRINTONE					"Print Selection Now"			// prints one copy of selected text or graphics
#define		CINAME_PRINTONE						"Print Selection Now"
#define INAME_PRINTSCREEN				"Print Screen"
#define INAME_QUIT						"Quit"							// standard text: item to quit the program
#define		KEY_QUIT							"Q"
#define		KEYCHAR_QUIT						'Q' // make this the same as the above

// Edit
#define INAME_UNDO						"Undo"							// standard text: item to undo an action
#define		INAME_UNDO_DIMENSIONCHANGES			"Undo Dimension Changes"
#define		INAME_UNDO_FORMATCHANGES			"Undo Format Changes"
#define		KEY_UNDO							"Z"
#define INAME_REDO						"Redo"							// standard text: item to redo an action
#define		INAME_REDO_DIMENSIONCHANGES			"Redo Dimension Changes"
#define		INAME_REDO_FORMATCHANGES			"Redo Format Changes"
#define		KEY_REDO							"Z"
#define INAME_CUT						"Cut"							// standard text: item to cut a selection to the clipboard
#define		KEY_CUT								"X"
#define INAME_COPY						"Copy"							// standard text: item to copy a selection to the clipboard
#define		CINAME_COPY							"Copy Selected Text"
#define		KEY_COPY							"C"
#define INAME_COPYTABLE					"Copy Using Tabs For Spaces"	// copies text, but replaces spaces with tabs
#define		CINAME_COPYTABLE					"Copy Using Tabs For Spaces"
#define		KEY_COPYTABLE						"C"
#define INAME_COPYANDPASTE				"Copy & Paste"					// types the selected text
#define		KEY_COPYANDPASTE					"V"
#define INAME_PASTE						"Paste"							// standard text: item to paste the clipboard contents into a selection
#define		CINAME_PASTE						"Paste Text"
#define		KEY_PASTE							"V"
#define INAME_CLEAR						"Delete"						// standard text: item to delete a selection
#define INAME_FIND						"Find…"							// standard text: item to display Find dialog
#define		CINAME_FIND							"Find…"
#define		KEY_FIND							"F"
#define INAME_FINDAGAIN					"<find again>"
#define		NORMAL_FINDAGAIN					"Find Next"				// standard text: item for automatically repeating the previous Find
#define		SHIFT_FINDAGAIN						"Find Previous"
#define		KEY_FINDAGAIN						"G"
#define INAME_FINDCURSOR				"Find Cursor"					// displays an animation highlighting the cursor location
#define		KEY_FINDCURSOR						"*"
#define INAME_SELECTALL					"<select all>"
#define		NORMAL_SELECTALL					"Select All"			// standard text: item to highlight all visible text
#define		SHIFT_SELECTALL						"Select Nothing"
#define		OPTION_SELECTALL					"Select Entire Scrollback Buffer"
#define		KEY_SELECTALL						"A"
#define INAME_SHOWCLIPBOARD				"<clipboard>"
#define		SHOWNAME_SHOWCLIPBOARD				"Show Clipboard"		// standard text: item to display the clipboard window
#define		HIDENAME_SHOWCLIPBOARD				"Hide Clipboard"		// standard text: item to close the clipboard window
#define INAME_PREFERENCES				"Preferences…"					// standard text: item to display consolidated Preferences dialog box
#define		KEY_PREFERENCES						","
#define		KEYCHAR_PREFERENCES					',' // make this the same as the above

// View
#define		CINAME_SETSCREENSIZE				"Screen Dimensions…"
#define		CINAME_FORMAT						"Format…"

// Terminal
#define		NORMAL_CAPTURETOFILE				"Begin File Capture…"
#define		CHECKED_CAPTURETOFILE				"End File Capture"
#define		CINAME_SPEAKSELECTEDTEXT			"Speak Selected Text"

// Keys
#define		CINAME_SETKEYS						"Special Key Sequences…"

// Window
#define		SHOWNAME_MINIMIZEFRONTWINDOW		"Restore"
#define		HIDENAME_MINIMIZEFRONTWINDOW		"Minimize"
#define		INAME_ZOOMFRONTWINDOW				"Zoom"
#define		INAME_MAXIMIZEFRONTWINDOW			"Maximize"
#define		CINAME_CHANGEWINDOWTITLE			"Rename…"
#define		CINAME_HIDEFRONTWINDOW				"Hide"
#define		NORMAL_NEXTWINDOW					"Next Window"
#define		SHIFT_NEXTWINDOW					"Previous Window"
#define		OPTION_NEXTWINDOW					"Next Window, Hide Previous"
#define		CINAME_STACKWINDOWS					"Arrange All in Front"
#define		SHOWNAME_SHOWCONNECTIONSTATUS		"Show Session Info"
#define		HIDENAME_SHOWCONNECTIONSTATUS		"Hide Session Info"
#define		SHOWNAME_SHOWCOMMANDLINE			"Show Command Line"
#define		HIDENAME_SHOWCOMMANDLINE			"Hide Command Line"
#define		SHOWNAME_SHOWMACROS					"Show Macro Editor"
#define		HIDENAME_SHOWMACROS					"Hide Macro Editor"
#define		SHOWNAME_SHOWKEYPAD					"Show VT220 Keypad"
#define		HIDENAME_SHOWKEYPAD					"Hide VT220 Keypad"
#define		SHOWNAME_SHOWFUNCTION				"Show VT220 Function Keys"
#define		HIDENAME_SHOWFUNCTION				"Hide VT220 Functon Keys"
#define		SHOWNAME_SHOWCONTROLKEYS			"Show Control Keys"
#define		HIDENAME_SHOWCONTROLKEYS			"Hide Control Keys"
#define		SHOWNAME_SHOWLEDS					"Show Terminal LEDs"
#define		HIDENAME_SHOWLEDS					"Hide Terminal LEDs"

// Help
#define INAME_HELP						"MacTelnet Help"		// should be identical to the help system name; used on Mac OS X
#define		CINAME_HELP							"MacTelnet Help"
#define		CINAME_SHOWBALLOONS					"Show Balloons"			// used in Classic for contextual menus in dialog boxes
#define		CINAME_HIDEBALLOONS					"Hide Balloons"			// used in Classic for contextual menus in dialog boxes
#define INAME_SHOWHELPTAGS				"<help tags>"
#define		SHOWNAME_SHOWHELPTAGS				"Show Help Tags"		// used on Mac OS X for contextual menus in dialog boxes, and in the Help menu
#define		HIDENAME_SHOWHELPTAGS				"Hide Help Tags"		// used on Mac OS X for contextual menus in dialog boxes, and in the Help menu
#define		KEY_SHOWHELPTAGS					"?"
#define		KEYCHAR_SHOWHELPTAGS				'?'

// Size (pop-up menu)
#define INAME_SIZE4						"4"
#define INAME_SIZE9						"9"
#define INAME_SIZE10					"10"
#define INAME_SIZE11					"11"
#define INAME_SIZE12					"12"
#define INAME_SIZE14					"14"
#define INAME_SIZE18					"18"

// format type (pop-up menu)
#define INAME_FORMATNORMALTEXT			"Normal Text"
#define INAME_FORMATBOLDTEXT			"Bold Text"
#define INAME_FORMATBLINKINGTEXT		"Blinking Text"

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
