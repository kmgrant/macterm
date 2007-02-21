/*
**************************************************************************
	TelnetMiscellaneousText.h
	
	MacTelnet
		© 1998-2005 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains some of the localizable strings used by MacTelnet.
	
**************************************************************************
*/

#ifndef __TELNETMISCELLANEOUSTEXT__
#define __TELNETMISCELLANEOUSTEXT__

// MacTelnet includes
#include "ApplicationVersion.h"



#ifdef REZ

/*
Commonly-used terms.  These are used in several strings, to
make them consistent and easier to localize.  By defining terms
here, you automatically define words in several other strings
that have the indicated key names substituted in them.
*/

// session configurations
#define SESSION_SINGULAR									"session"
#define A_SESSION											"a "SESSION_SINGULAR
#define SESSION_PLURAL										"sessions"
#define SESSION_POSSESSIVE									"session’s"
#define SESSION_FAVORITES_SINGULAR							"Session Favorite"
#define SESSION_FAVORITES_PLURAL							"Session Favorites"
#define SESSION_FAVORITES_DEFAULT_NAME						"Default"

// session files
#define SAVED_SET											"session description"

// terminal configurations
#define TERMINAL_SINGULAR									"Terminal"
#define A_TERMINAL											"a terminal"
#define TERMINAL_PLURAL										"terminals"
#define TERMINAL_FAVORITES_DEFAULT_NAME						"Default"

// proxy configurations
#define PROXY_SINGULAR										"Proxy"
#define A_PROXY												"a proxy server configuration"
#define PROXY_PLURAL										"proxy server configurations"
#define PROXY_PLURAL_AS_TITLE								"Proxies"



/*
Custom file kind strings.
*/
#define KIND_RESOURCE_STRING_apnm			MacTelnet
#define KIND_RESOURCE_STRING_TEXT				"set of macros"
#define KIND_RESOURCE_STRING_CONF			SAVED_SET
#define KIND_RESOURCE_STRING_pref				"preferences file"



/*
Important folder names.

WARNING: For maximum compatibility, these should be 31 characters or less.
*/

#define CONFIGURATION_DATA_HELP_FOLDER_NAME \
	"MacTelnet.help"



/*
Preference file names.

WARNING: For maximum compatibility, these should be 31 characters or less.
*/

// general preferences file
#define PREFERENCES_FILE_NAME \
	"MacTelnet Preferences"

// file containing the list of “good” (monospaced) fonts and “bad” (other) fonts
#define PREFERENCES_FONT_LIST_FILE_NAME \
	"MacTelnet Font List"

// automatically-preserved macros
#define MISC_MACRO_SET_1_FILE_NAME \
	"MacTelnet Macro Set 1"

#define MISC_MACRO_SET_2_FILE_NAME \
	"MacTelnet Macro Set 2"

#define MISC_MACRO_SET_3_FILE_NAME \
	"MacTelnet Macro Set 3"

#define MISC_MACRO_SET_4_FILE_NAME \
	"MacTelnet Macro Set 4"

#define MISC_MACRO_SET_5_FILE_NAME \
	"MacTelnet Macro Set 5"



/*
Important file names.

WARNING: For maximum compatibility, these should be 31 characters or less.
*/

#define MISC_BELL_SOUND_FILE_NAME \
	"Bell"



/*
Miscellaneous strings.
*/

// this string may appear on a terminal screen when the user uses the “interrupt process” key sequence
#define INTERRUPT_PROCESS_TERMINAL_FEEDBACK \
	"\n\r[MacTelnet: Interrupt Process]\n\r"

#define MISC_NEW_SESSION_DEFAULT_NAME \
	"untitled "SESSION_SINGULAR

#define MISC_NEW_TERMINAL_DEFAULT_NAME \
	"untitled terminal"

#define MISC_WINDOW_TITLE_ADVISED_USER_NAME \
	"Use username="

#define MISC_WINDOW_TITLE_ADVISED_PASSWORD \
	", password="

#define MISC_MAC_OS_8_5_FIND_FILE_NAME \
	"Sherlock"

#define MISC_PRE_MAC_OS_8_5_FIND_FILE_NAME \
	"Find File"

#define MISC_PICK_NEW_COLOR_PROMPT \
	"Please select a new color."

#define MISC_TRANSLATION_TABLE_TYPE_NONE \
	"None"

#define MISC_PRINTOUT_SESSION_NAME_STRING \
	"Session Name:"

#define MISC_WINDOID_NAME_KEYPAD \
	"VT220 Keypad"

#define MISC_WINDOID_NAME_FUNCTION \
	"Function Keys"

#define MISC_WINDOID_NAME_CONTROL_KEYS \
	"Control Keys"



/*
Navigation Services dialog strings.  These are the window titles and
prompts of all file dialog boxes that MacTelnet ever displays.
*/

#define NAVIGATION_SERVICES_DIALOG_TITLE_EXAMPLE_BINARY_FILE \
	"Example Binary File"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_EXAMPLE_BINARY_FILE \
	"Select a file whose type and creator you wish to copy."

#define NAVIGATION_SERVICES_DIALOG_TITLE_TEXT_EDITING_APPLICATION \
	"Text Editing Application"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_TEXT_EDITING_APPLICATION \
	"Select an application that will be able to open the file."

#define NAVIGATION_SERVICES_DIALOG_TITLE_SELECT_DEFAULT_DIRECTORY \
	"Select Default Directory"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_SELECT_DEFAULT_DIRECTORY \
	"Please choose a directory."

#define NAVIGATION_SERVICES_DIALOG_TITLE_EXPORT_MACROS \
	"Export Macros"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_EXPORT_MACROS \
	"Enter a name for a text file to contain your macros."

#define NAVIGATION_SERVICES_DIALOG_TITLE_IMPORT_MACROS \
	"Import Macros"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_IMPORT_MACROS \
	"Select a file to import macros from."

#define NAVIGATION_SERVICES_DIALOG_TITLE_SAVE_SESSION_DESCRIPTION \
	"Save Workspace"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_SAVE_SESSION_DESCRIPTION \
	"Enter a name for the file to contain your settings."

#define NAVIGATION_SERVICES_DIALOG_TITLE_LOAD_SESSION_DESCRIPTION \
	"Open Workspace"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_LOAD_SESSION_DESCRIPTION \
	"Select a workspace to open."

#define NAVIGATION_SERVICES_DIALOG_TITLE_SAVE_CAPTURED_TEXT \
	"Save Captured Text"

#define NAVIGATION_SERVICES_DIALOG_PROMPT_SAVE_CAPTURED_TEXT \
	"Enter a name for a text file to contain the captured text."

// %a=(the name of an application program)
#define NAVIGATION_SERVICES_DIALOG_TITLE_LOCATE_APPLICATION \
	"Locate “%a”"

// %a=(the name of an application program)
#define NAVIGATION_SERVICES_DIALOG_PROMPT_LOCATE_APPLICATION \
	"Please locate the “%a” application before continuing."



/*
Standard File dialog strings.  These are the window titles and
prompts of all file dialog boxes that MacTelnet ever displays.
*/

#define STANDARDFILE_SAVEAS \
	"Save as:"

#define STANDARDFILE_CHOOSEFOLDER \
	"Choose a Folder:"



/*
Help text strings.  These are displayed in the small system font, in
dialog boxes or floating windows.

MacTelnet automatically sizes dialog box help items, so the text
you write will never be truncated as long as it does not exceed 255
characters (after substitution of indicated common terms).  Other
surrounding components in any dialog box will automatically be
moved or resized appropriately to make room for whatever you
write here.
*/

#define HELPTEXT_COMMAND_LINE_DEFAULT \
	"Type a line and press Return or Tab.  URLs are handled locally, anything else is sent to the frontmost session."

#define HELPTEXT_FORMAT_DIALOG_FRONTMOST_WINDOW \
	"Changes affect only the frontmost session window."

#define HELPTEXT_PASSWORDENTRY_CAPSLOCKDOWN \
	"Caps Lock is active."

#define HELPTEXT_PASSWORDENTRY_WRONGPASSWORDNOWAIT \
	"User name or password incorrect."

#define HELPTEXT_PASSWORDENTRY_WRONGPASSWORDWAIT \
	"Incorrect - please wait before retrying."

#define HELPTEXT_PREFERENCES_GENERAL_NOTIFICATION_TELNET_MAY_NEED_YOUR_ATTENTION \
	"MacTelnet may need to get your attention when it is not the frontmost application.  To avoid " \
	"immediate interruption of your work in other applications, you can choose a more moderate notification " \
	"mechanism, or none whatsoever."

#define HELPTEXT_PROXYSETUP_SERVERINFO_TIP \
	"Tip: You can use AppleScript to dynamically change proxy server settings.  " \
	"In some environments, this may be necessary - check with your system " \
	"administrator for more information."

#define HELPTEXT_PROXYSETUP_DIRECTDOMAINS_DESCRIPTION \
	"Some domains, usually those inside the “firewall” of an internal network, are " \
	"possible to access directly, instead of through a proxy server.  If there are " \
	"any domains you would like to access directly, add them to the list below."

#define HELPTEXT_SETKEYS_HOWTOSET \
	"To set a key sequence, select the text in the field next to its action and then " \
	"type the key sequence on the keyboard.  Blank entries are disabled."

#define HELPTEXT_TERMINALEDITOR_ANSWERBACK \
	"Enter an answer-back message indicating the type of terminal you want the " \
	"remote host to think you are using."



/*
Dialog text strings.  These are miscellaneous, short strings that
are displayed in various places (usually as labels) in dialogs.

MacTelnet automatically sizes dialog box text items, so the text
you write will never be truncated.  Other surrounding components
in any dialog box will automatically be moved or resized
appropriately to make room for whatever you write here.
*/

// this string appears as an error message in the Find dialog
#define DIALOGTEXT_FIND_NOTHINGFOUND \
	"The text was not found."

// this string labels the font size information in the new “Format” dialog box
#define DIALOGTEXT_FORMAT_SIZE_LABEL \
	"Size:"

// this string appears after a color box in the new “Format” dialog box
#define DIALOGTEXT_FORMAT_COLOR_LABEL_FOREGROUND \
	"Foreground Color"

// this string appears after a color box in the new “Format” dialog box
#define DIALOGTEXT_FORMAT_COLOR_LABEL_BACKGROUND \
	"Background Color"

// this string labels the top-left text field in the “New Sessions” dialog box, representing the host name
#define DIALOGTEXT_NEWSESSIONS_HOST_LABEL \
	"Host:"

// this string labels the top-right text field in the “New Sessions” dialog box, representing the port number
#define DIALOGTEXT_NEWSESSIONS_PORT_LABEL \
	"Port:"

// this string labels the top-left text field in the “New Sessions” dialog box, representing the local Unix command line
#define DIALOGTEXT_NEWSESSIONS_COMMANDLINE_LABEL \
	"Unix Command Line:"

// this string labels the first text field in an authentication dialog box
#define DIALOGTEXT_PASSWORDENTRY_LABEL_USERNAME \
	"User ID:"

// this string labels the second text field in an authentication dialog box
#define DIALOGTEXT_PASSWORDENTRY_LABEL_PASSWORD \
	"Password:"

// this string appears in the Special tab under General Preferences
#define DIALOGTEXT_PREFERENCES_GENERAL_CURSOR_SHAPE \
	"Shape:"

// this string appears in the Special tab under General Preferences
#define DIALOGTEXT_PREFERENCES_GENERAL_STACKING_ORIGIN_LEFT \
	"Left:"

// this string appears in the Special tab under General Preferences
#define DIALOGTEXT_PREFERENCES_GENERAL_STACKING_ORIGIN_TOP \
	"Top:"

// this string appears in the Special tab under General Preferences
#define DIALOGTEXT_PREFERENCES_GENERAL_COPY_TABLE_THRESHOLD \
	"One tab equals"

// this string appears in the Special tab under General Preferences
#define DIALOGTEXT_PREFERENCES_GENERAL_COPY_TABLE_THRESHOLD_UNITS \
	"spaces"

// ANSI color labels used in the Colors tab under General Preferences
#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_HEADER \
	"ANSI Colors"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_HEADER \
	"Normal"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_EMPHASIZED_HEADER \
	"Emphasized"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_BLACK \
	"Black"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_RED \
	"Red"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_GREEN \
	"Green"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_YELLOW \
	"Yellow"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_BLUE \
	"Blue"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_MAGENTA \
	"Magenta"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_CYAN \
	"Cyan"

#define DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_WHITE \
	"White"

// this string appears in the Notification tab under General Preferences
#define DIALOGTEXT_PREFERENCES_GENERAL_NOTIFICATION_IF_TELNET_NEEDS_YOUR_ATTENTION \
	"If MacTelnet needs your attention, it should:"

// this string labels the Name field in the “Proxy Server Setup” dialog box
#define DIALOGTEXT_PROXYSERVER_NAME_LABEL \
	"Name:"

// this string labels the Server Info Host field in the “Proxy Server Setup” dialog box
#define DIALOGTEXT_PROXYSERVER_SERVERHOST_LABEL \
	"Name or IP Address:"

// this string labels the Server Info Port field in the “Proxy Server Setup” dialog box
#define DIALOGTEXT_PROXYSERVER_PORTNUMBER_LABEL \
	"Port Number:"

// this string labels the Server Info Protocol field in the “Proxy Server Setup” dialog box
#define DIALOGTEXT_PROXYSERVER_PROTOCOL_LABEL \
	"Protocol:"

// this string labels the Authentication User Name field in the “Proxy Server Setup” dialog box
#define DIALOGTEXT_PROXYSERVER_AUTHENTICATIONNAME_LABEL \
	"User Name:"

// this string labels the Authentication Password field in the “Proxy Server Setup” dialog box
#define DIALOGTEXT_PROXYSERVER_AUTHENTICATIONPASSWORD_LABEL \
	"Password:"

// this string labels the Direct Domains entry field in the “Proxy Server Setup” dialog box
#define DIALOGTEXT_PROXYSERVER_DIRECTDOMAIN_LABEL \
	"Domain Name:"

#define DIALOGTEXT_PREFERENCES_SESSION_50000 \
	"50000"

#define DIALOGTEXT_PREFERENCES_SESSION_200 \
	"200"

#define DIALOGTEXT_PREFERENCES_SESSION_HOSTNAME \
	"Host Name"

#define DIALOGTEXT_PREFERENCES_SESSION_ALIAS \
	"Alias"

#define DIALOGTEXT_PREFERENCES_SESSION_PORT \
	"Port"

#define DIALOGTEXT_PREFERENCES_SESSION_TEKGRAPHICS \
	"TEK Graphics"

#define DIALOGTEXT_PREFERENCES_SESSION_PASTEMETHOD \
	"Paste Method"

#define DIALOGTEXT_PREFERENCES_SESSION_DELETESENDS \
	"Delete Sends"

#define DIALOGTEXT_PREFERENCES_SESSION_INTERRUPT \
	"Interrupt"

#define DIALOGTEXT_PREFERENCES_SESSION_SUSPEND \
	"Suspend"

#define DIALOGTEXT_PREFERENCES_SESSION_RESUME \
	"Resume"

#define DIALOGTEXT_PREFERENCES_SESSION_TERMINAL \
	"Terminal"

#define DIALOGTEXT_PREFERENCES_SESSION_CHARSET \
	"Character Set"

#define DIALOGTEXT_PREFERENCES_SESSION_NETBLOCK \
	"Network Block Size"

#define DIALOGTEXT_PREFERENCES_SESSION_EDITTEXT \
	"Edit Text"

// Terminal editor strings
#define DIALOGTEXT_PREFERENCES_TERMINAL_TERMINALNAME \
	"Terminal Name:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_UNTITLED \
	"untitled"

#define DIALOGTEXT_PREFERENCES_TERMINAL_EMACSMETA \
	"EMACS Meta Key:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_EMULATION \
	"Emulation:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_PERCEIVEDEMULATOR \
	"Perceived Emulator:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_VT100 \
	"vt100"

#define DIALOGTEXT_PREFERENCES_TERMINAL_WINDOWAPPEARANCE \
	"Window Appearance:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_FONT \
	"Font:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_9 \
	"9"

#define DIALOGTEXT_PREFERENCES_TERMINAL_SCREEN \
	"Screen:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_80 \
	"80"

#define DIALOGTEXT_PREFERENCES_TERMINAL_BY \
	"by"

#define DIALOGTEXT_PREFERENCES_TERMINAL_24 \
	"24"

#define DIALOGTEXT_PREFERENCES_TERMINAL_SCROLLBACK \
	"Scrollback:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_200 \
	"200"

#define DIALOGTEXT_PREFERENCES_TERMINAL_NORMBOLDBLINK \
	"Normal, Bold, Blinking"

#define DIALOGTEXT_PREFERENCES_TERMINAL_FOREGROUND \
	"Foreground:"

#define DIALOGTEXT_PREFERENCES_TERMINAL_BACKGROUND \
	"Background:"

#define DIALOGTEXT_SETKEYS_INTERRUPT_LABEL \
	"Interrupts a Process"

#define DIALOGTEXT_SETKEYS_SUSPEND_LABEL \
	"Suspends Output"

#define DIALOGTEXT_SETKEYS_RESUME_LABEL \
	"Resumes Output"

// part of the Terminal Screen Size dialog box
#define DIALOGTEXT_SCREENSIZE_WIDTH_LABEL \
	"Width:"

// part of the Terminal Screen Size dialog box
#define DIALOGTEXT_SCREENSIZE_HEIGHT_LABEL \
	"Height:"

// part of the Terminal Screen Size dialog box
#define DIALOGTEXT_SCREENSIZE_WIDTH_UNITS \
	"columns"

// part of the Terminal Screen Size dialog box
#define DIALOGTEXT_SCREENSIZE_HEIGHT_UNITS \
	"rows"

// this string labels the title text field in the “Rename Window” dialog box
#define DIALOGTEXT_SETWINDOWTITLE_LABEL \
	"New Name:"



/*
Clipboard strings.  See “ TelnetLocalization.h” if you need to
alter the position of the units for your language (for example,
"bytes 30" instead of "30 bytes").
*/

// %a=(data type, see the 3 following strings), %b=(approximate or not, see below), %c=(size), %d=(units)
// Note that %b and %c are directly touching, with no space between them.
#define CLIPBOARD_CONTENTS_SUBSTITUTION_STRING \
	"The Clipboard contains %a (%b%c %d)."

#define CLIPBOARD_CONTENTS_DATA_TYPE_TEXT \
	"a text selection"

#define CLIPBOARD_CONTENTS_DATA_TYPE_PICTURE \
	"a picture"

#define CLIPBOARD_CONTENTS_DATA_TYPE_UNSUPPORTED \
	"information that MacTelnet does not know how to display"

#define CLIPBOARD_CONTENTS_APPROXIMATION_STATEMENT_WITH_TRAILING_SPACE \
	"about "

#define CLIPBOARD_CONTENTS_EMPTY_STRING \
	"The Clipboard is currently empty."

// ??=(scaling percentage)
#define CLIPBOARD_CONTENTS_SCALING_AMOUNT_SUBSTITUTION_STRING \
	"Displayed at ??% of its actual size."

#define CLIPBOARD_SIZE_UNIT_BYTES_SINGULAR \
	"byte"

#define CLIPBOARD_SIZE_UNIT_BYTES_PLURAL \
	"bytes"

#define CLIPBOARD_SIZE_UNIT_KILOBYTES \
	"K"

#define CLIPBOARD_SIZE_UNIT_MEGABYTES \
	"MB"



/*
Splash screen strings.  You can make these whatever you like,
limited by the space required to render them in the specified
font face on the splash screen.

Do not specify a font face that is not installed by default with
your localized version of the Mac OS, because then users may
not see the text rendered correctly.
*/

#define SPLASH_SCREEN_FONT_FAMILY_NAME \
	"Techno"

#define SPLASH_SCREEN_RANDOM_STRING_1 \
	"In a word, “powerful”."

#define SPLASH_SCREEN_RANDOM_STRING_2 \
	"“Sorry, we don’t do Windows.”"

#define SPLASH_SCREEN_RANDOM_STRING_3 \
	"Not finished, but hey, neither is Windows."

#define SPLASH_SCREEN_RANDOM_STRING_4 \
	"Mac’s back, baby."

#define SPLASH_SCREEN_RANDOM_STRING_5 \
	"No, it’s not from Microsoft."

#define SPLASH_SCREEN_RANDOM_STRING_6 \
	"That’s right, MAC."

#define SPLASH_SCREEN_RANDOM_STRING_7 \
	"by Kevin Grant"

#define SPLASH_SCREEN_RANDOM_STRING_8 \
	"Terminal fever?"

#define SPLASH_SCREEN_RANDOM_STRING_9 \
	"Servers sold separately."

#define SPLASH_SCREEN_RANDOM_STRING_10 \
	"Featuring MacTelnet Help."

#define SPLASH_SCREEN_RANDOM_STRING_11 \
	"Ain’t nothin’ like it."

#define SPLASH_SCREEN_RANDOM_STRING_12 \
	"Probably bug-free."

#define SPLASH_SCREEN_RANDOM_STRING_13 \
	"Exclusively for Macintosh."

#define SPLASH_SCREEN_RANDOM_STRING_14 \
	"The ultimate telnet program."



/*
Command line command and argument names.  These strings will only
appear in the command line windoid, nowhere else.  By changing command
and argument names, you change what the user must type in order to use
the respective command.

Please localize these new strings only if you know how they are localized on
a UNIX distribution in another language.  Your average UNIX user will expect
to find the “standard” form of these commands.

WARNING:	Do not make any of these names longer than 32 characters.
						Do not make any description longer than 200 characters.
*/

// These are help arguments, which apply to ANY command and automatically
// open Telnet Help in search mode to look for more on the command that the
// argument was used with.  If you don’t need one of these, make it blank ("").
#define COMMAND_LINE_ARGUMENT_NAME_HELP_1						"-h"
#define COMMAND_LINE_ARGUMENT_NAME_HELP_2						"-help"
#define COMMAND_LINE_ARGUMENT_NAME_HELP_3						"--help"
#define COMMAND_LINE_ARGUMENT_NAME_HELP_4						"help"

#define COMMAND_LINE_COMMAND_NAME_help							"help"
#define		COMMAND_DESCRIPTION_help \
	"Automatically uses MacTelnet Help to perform a search using given keywords."

#define COMMAND_LINE_COMMAND_NAME_lynx							"lynx"
#define		COMMAND_DESCRIPTION_lynx \
	"Launches your preferred web browser."

#define COMMAND_LINE_COMMAND_NAME_telnet						"telnet"
#define COMMAND_LINE_COMMAND_ALIAS_1_telnet						"rlogin"
#define COMMAND_LINE_COMMAND_ALIAS_2_telnet						"open"
#define		COMMAND_DESCRIPTION_telnet \
	"Opens a connection using the TELNET protocol."

#define COMMAND_LINE_COMMAND_NAME_ftp							"ftp"
#define		COMMAND_DESCRIPTION_ftp \
	"Opens a connection using the file transfer protocol."



/*
Command line strings.  These strings will only appear in the command line
windoid, nowhere else.  By changing command names, you change what the
user must type in order to use the respective command.
*/

// a template for command description display; %a=(command name), %b=(command description string)
#define COMMAND_NAME_AND_DESCRIPTION_DISPLAY_TEMPLATE \
	"%a - %b"

#define COMMAND_DESCRIPTION_UNKNOWN \
	"MacTelnet will send this text to the frontmost session window."

// %a=(argument name)
#define COMMAND_ARGUMENT_UNKNOWN \
	"Warning: The %a command does not recognize this argument."


#endif /* REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
