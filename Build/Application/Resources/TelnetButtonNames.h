/*
**************************************************************************
	TelnetButtonNames.h
	
	MacTelnet
		© 1998-2005 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains strings for common terms, as well as for checkbox
	and radio button names, and other controls.
**************************************************************************
*/

#ifndef __TELNETBUTTONNAMES__
#define __TELNETBUTTONNAMES__

/*
window titles
*/
#pragma mark window titles

// If a substitution string (%a) appears, it will be replaced by MacTelnet’s application name.
#define FORMAT_WINDOWNAME								"Format Terminal"
#define PROXYSERVER_WINDOWNAME							"Edit Proxy Server Favorite"
#define SCREENSIZE_WINDOWNAME							"Terminal Screen Size"
#define SETKEYS_WINDOWNAME								"Special Key Sequences"
#define SESSIONEDITOR_WINDOWNAME						"Edit Session Favorite"
#define TERMINALEDITOR_WINDOWNAME						"Edit Terminal Favorite"



/*
checkboxes
*/
#pragma mark checkboxes

// Options in Favorites editor dialog boxes.
#define PREFERENCES_SESSION_CBNAME_FORCESSAVE			"Force Save"
#define PREFERENCES_SESSION_CBNAME_BERKELEYCR			"Berkeley 4.3 CR Mode"
#define PREFERENCES_SESSION_CBNAME_LINEMODE				"Line Mode"
#define PREFERENCES_SESSION_CBNAME_PAGECLEAR			"TEK PAGE Clears Graphics Window"
#define PREFERENCES_SESSION_CBNAME_HALFDUPLEX			"Half Duplex"
#define PREFERENCES_SESSION_CBNAME_AUTHENTICATE			"Authenticate"
#define PREFERENCES_SESSION_CBNAME_ENCRYPT				"Encrypt"
#define PREFERENCES_SESSION_CBNAME_LOCALECHO			"Local Echo"
#define PREFERENCES_SESSION_CBNAME_AUTOCAPTURE			"Automatically Capture to File"
#define PREFERENCES_TERMINAL_CBNAME_ANSICOLORS			"ANSI Colors & Graphics"		
#define PREFERENCES_TERMINAL_CBNAME_XTERM				"XTerm Sequences"
#define PREFERENCES_TERMINAL_CBNAME_WRAPLINES			"Wrap Lines (No Truncate)"
#define PREFERENCES_TERMINAL_CBNAME_EMACSARROWS			"EMACS Arrow Keys"
#define PREFERENCES_TERMINAL_CBNAME_8BITS				"Use Full 8 Bits"
#define PREFERENCES_TERMINAL_CBNAME_SAVECLEAR			"Save Lines on Clear"
#define PREFERENCES_TERMINAL_CBNAME_NORMKEYPAD			"Normal Keypad Top Row"
#define PREFERENCES_TERMINAL_CBNAME_PAGEKEYS			"Don’t Send Page Keys"

// This labels the “Via Proxy:” checkbox in the New Sessions dialog.
#define NEWSESSIONS_CBNAME_PROXIES						"Via Proxy:"



/*
pop-up menu buttons
*/
#pragma mark pop-up menu buttons

// This labels the Edit pop-up menu in the Format dialog.
#define FORMAT_POPUPNAME_EDIT							"Edit:"

// This labels the Font pop-up menu in the Format dialog.
#define FORMAT_POPUPNAME_FONT							"Font:"

// This labels the Character Set pop-up menu in the Format dialog.
#define FORMAT_POPUPNAME_CHARACTERSET					"Glyphs:"

// This labels the Show pop-up menu in the Preferences dialog (Favorites panel).
#define PREFERENCES_CONFIGURATIONS_POPUPNAME_SHOW		"Show:"

// This labels the Translation Table pop-up menu in the Session Favorite Editor dialog.
#define SESSIONEDITOR_POPUPNAME_TRANSLATIONTABLE		"Translation Table:"

// This labels the Terminal pop-up menu in the Session Favorite Editor dialog.
#define SESSIONEDITOR_POPUPNAME_DEFAULTTERMINAL			"Terminal:"



/*
push buttons
*/
#pragma mark push buttons

#define REUSABLE_BTNNAME_OK								"OK"
#define REUSABLE_BTNNAME_CANCEL							"Cancel"

#define NEWSESSIONFAVORITE_BTNNAME_CREATE				"Create"

#define PREFERENCES_CONFIGURATIONS_BTNNAME				"Favorites"
#define PREFERENCES_CONFIGURATIONS_BTNNAME_NEW				"New…"
#define PREFERENCES_CONFIGURATIONS_BTNNAME_NEWGROUP			"New Group…"
#define PREFERENCES_CONFIGURATIONS_BTNNAME_REMOVE			"Remove"
#define PREFERENCES_CONFIGURATIONS_BTNNAME_DUPLICATE		"Duplicate…"
#define PREFERENCES_CONFIGURATIONS_BTNNAME_EDIT				"Edit…"
#define PREFERENCES_MACROS_BTNNAME						"Macros"
#define PREFERENCES_SCRIPTS_BTNNAME						"Scripts"
#define PREFERENCES_SESSION_BTNNAME_SET					"Set…"

#define PROXYSETUP_BTNNAME_PASSWORDSET					"Set…"
#define PROXYSETUP_BTNNAME_NEWDOMAIN					"New Domain"
#define PROXYSETUP_BTNNAME_REMOVE						"Remove"



/*
radio buttons
*/
#pragma mark radio buttons

#define PREFERENCES_SESSION_RBNAME_INHIBIT				"Inhibit"
#define PREFERENCES_SESSION_RBNAME_4014					"4014"
#define PREFERENCES_SESSION_RBNAME_4015					"4105"
#define PREFERENCES_SESSION_RBNAME_QUICK				"Quick"
#define PREFERENCES_SESSION_RBNAME_INBLOCKS				"In Blocks of"
#define PREFERENCES_SESSION_RBNAME_DELETE				"Delete"
#define PREFERENCES_SESSION_RBNAME_BACKSPACE			"Backspace"
#define PREFERENCES_SESSION_RBNAME_OPTION				"Option"
#define PREFERENCES_SESSION_RBNAME_NONE					"None"
#define PREFERENCES_SESSION_RBNAME_VT100				"VT100"
#define PREFERENCES_SESSION_RBNAME_VT220				"VT220"



/*
tabs
*/
#pragma mark tabs

#define PREFERENCES_MACROS_TABNAME_SET1					"Set 1"
#define PREFERENCES_MACROS_TABNAME_SET2					"Set 2"
#define PREFERENCES_MACROS_TABNAME_SET3					"Set 3"
#define PREFERENCES_MACROS_TABNAME_SET4					"Set 4"
#define PREFERENCES_MACROS_TABNAME_SET5					"Set 5"

#define PROXYSETUP_TABNAME_SERVERINFO					"Server Info"
#define PROXYSETUP_TABNAME_AUTHENTICATION				"Authentication"
#define PROXYSETUP_TABNAME_DIRECTDOMAINS				"Direct Domains"

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
