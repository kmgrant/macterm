/*
**************************************************************************
	TelnetBalloonText.h
	
	MacTelnet
		© 1998-2005 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains all resource data consisting of strings of text used for any kind of help balloon.
	
	Please try to view this file in the font “Geneva 9”.
	
	The symbol “\n” in a string means “new line”, and should usually be left alone.
**************************************************************************
*/

#ifndef __TELNETBALLOONTEXT__
#define __TELNETBALLOONTEXT__

#ifdef REZ

// MacTelnet includes
#include "ApplicationVersion.h"
#include "TelnetButtonNames.h"
#include "TelnetMenuItemNames.h"



/*********************************************
		MENU AND MENU ITEM BALLOON HELP STRINGS
*********************************************/

// generic convenience strings
#define DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM \
	"Not available because the active window (if any) is not a terminal window."



/*
APPLE MENU BALLOONS
*/

#define TEXT_BALLOON_MENU_APPLE_ENABLED_ITEM_ABOUTTELNET \
	"To find out more about MacTelnet, such as its version number and who its authors are, choose this item."

/****************************************************************************/

/*
FILE MENU BALLOONS
*/

#define FILE_MENU_NAME_FOR_BALLOON	MENUNAME_FILE

#define TEXT_BALLOON_ENABLED_MENU_FILE \
	FILE_MENU_NAME_FOR_BALLOON" menu\n\nUse this menu to open or close windows, work with configuration sets and macro files, or print selections of text or graphics."

#define TEXT_BALLOON_ENABLED_MENU_FILE_SIMPLE \
	FILE_MENU_NAME_FOR_BALLOON" menu\n\nUse this menu to open or close windows or print selections of text or graphics."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_NEWSESSION \
	"To create a terminal window, choose this item."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_NEWSESSIONDIALOG \
	"To run a local or remote Unix application, choose this item."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_OPENSESSION \
	"To reopen one or more sessions you previously saved, choose this item."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_CLOSECONNECTION \
	"To remove the active window or force a session to end, choose this item.  Normally, you should allow sessions to terminate on their own."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_CLOSEWORKSPACE \
	"To remove the active window and all other windows belonging to its workspace, choose this item."

#define TEXT_BALLOON_MENU_FILE_DISABLED_ITEM_CLOSEWORKSPACE \
	"Removes the active window and all other windows belonging to its workspace.  Not available because there is only one window in the active workspace."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_SAVESESSION \
	"To save the attributes of the active window and all other windows in its workspace, choose this item."

#define TEXT_BALLOON_MENU_FILE_DISABLED_ITEM_SAVESESSION \
	"Saves the attributes of the active window and all other windows in its workspace.  Not available because no terminal windows are open."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_SAVESELECTEDTEXT \
	"To save the entire selection of the active window as a text file, choose this item."

#define TEXT_BALLOON_MENU_FILE_DISABLED_ITEM_SAVESELECTEDTEXT \
	"Saves the entire selection of the active window as a text file.  Not available because nothing is selected."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_IMPORTMACROSET \
	"To replace your current set of 10 macros (preserving the other 40) with macros in a file, choose this item."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_EXPORTCURRENTMACROSET \
	"To save your current set of 10 macros (ignoring the other 40) in a file, choose this item."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_NEWDUPLICATESESSION \
	"To create a new session with all the parameters of the active session window, choose this item."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_OPENSELECTEDRESOURCE \
	"To launch the Internet application appropriate for accessing the selected URL, choose this item."

#define TEXT_BALLOON_MENU_FILE_DISABLED_ITEM_OPENSELECTEDRESOURCE \
	"Launches the Internet application appropriate for accessing the selected URL.  Not available because no URL is selected."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_PRINT \
	"To print the current selection of text or graphics using custom print settings, choose this item."

#define TEXT_BALLOON_MENU_FILE_DISABLED_ITEM_PRINT \
	"Prints the current selection of text or graphics.  Not available because nothing is selected."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_PRINTONE \
	"To print one copy of the current selection of text or graphics immediately, using default print settings, choose this item."

#define TEXT_BALLOON_MENU_FILE_DISABLED_ITEM_PRINTONE \
	"Prints one copy of the current selection of text or graphics immediately, using default print settings.  Not available because nothing is selected."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_PRINTSCREEN \
	"To print one copy of the bottommost screenful of text immediately, using default print settings, choose this item.\n\nTip: On extended keyboards, the Print Screen key (F13) also invokes this command."

#define TEXT_BALLOON_MENU_FILE_ENABLED_ITEM_PAGESETUP \
	"To display the standard Page Setup dialog box, for formatting printed pages, choose this item."

#define TEXT_BALLOON_MENU_FILE_DISABLED_ITEM_PAGESETUP \
	"Displays the standard Page Setup dialog box, for formatting printed pages.  Not available because a printing component is missing."

/****************************************************************************/

/*
EDIT MENU BALLOONS
*/

// convenience strings - you should localize these if you use them!
#define THE_CLIPBOARD \
	"the clipboard"

#define THE_SELECTION \
	"the selected text or graphics"



#define TEXT_BALLOON_ENABLED_MENU_EDIT \
	MENUNAME_EDIT" menu\n\nUse this menu to manipulate selections of text or graphics or see what is on the clipboard."

#define TEXT_BALLOON_ENABLED_MENU_EDIT_SIMPLE \
	MENUNAME_EDIT" menu\n\nUse this menu to manipulate selections of text or graphics."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_UNDO \
	"To cancel the last action you took, choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_UNDO \
	"Cancels the last action you took.  Not available because your last action was not possible to undo."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_REDO \
	"To cancel the effects of your last Undo operation, choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_REDO \
	"Cancels the effects of your last Undo operation.  Not available because you have not used Undo."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_CUT \
	"To delete "THE_SELECTION", placing a copy of it on "THE_CLIPBOARD", choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_CUT \
	"Deletes "THE_SELECTION", placing a copy of it on "THE_CLIPBOARD".  Not available because nothing is selected, or because it is not possible to remove the selection in the active window."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_COPY \
	"To place a copy of "THE_SELECTION" on "THE_CLIPBOARD", choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_COPY \
	"Places a copy of "THE_SELECTION" on "THE_CLIPBOARD".  Not available because nothing is selected."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_COPYTABLE \
	"To place a copy of the selected text on "THE_CLIPBOARD", replacing a certain number of consecutive spaces with single tab characters, choose this item.  You set the space count in General Preferences."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_PASTE \
	"To insert the text on "THE_CLIPBOARD" into the active terminal window as if it were typed, choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_PASTE \
	"Inserts the text on "THE_CLIPBOARD" into the active terminal window as if it were typed.  Not available because there is no text on "THE_CLIPBOARD", or because there is nowhere to paste it."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_COPYANDPASTE \
	"To insert the selected text into the active terminal window as if it were typed, choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_COPYANDPASTE \
	"Inserts the selected text into the active terminal window as if it were typed.  Not available because there is no text selected, or because there is no way to type it."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_CLEAR \
	"To delete "THE_SELECTION", choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_CLEAR \
	"Deletes "THE_SELECTION".  Not available because nothing is selected, or because it is not possible to remove the selection in the active window."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_FIND \
	"To search for text in the active window, choose this item."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_FINDAGAIN \
	"To automatically repeat your most recent search for text in the active window, choose this item."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_FINDCURSOR \
	"To briefly animate the active terminal window’s cursor, choose this item."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_SELECTALL \
	"To select everything currently visible in the active window, choose this item."

#define TEXT_BALLOON_MENU_EDIT_DISABLED_ITEM_SELECTALL \
	"Selects everything currently visible in the active window.  Not available because the active window cannot be edited."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_SHOWCLIPBOARD \
	"To display or remove a window showing the contents of "THE_CLIPBOARD", choose this item.  MacTelnet can only render the contents of "THE_CLIPBOARD" if the data is text or graphics."

#define TEXT_BALLOON_MENU_EDIT_ENABLED_ITEM_PREFERENCES \
	"To change MacTelnet settings, create "SESSION_FAVORITES_PLURAL", edit macros, or manage an FTP server, choose this item."

/****************************************************************************/

/*
VIEW MENU BALLOONS
*/

// convenience strings
#define ENABLED_VIEW_MENU_MESSAGE \
	MENUNAME_VIEW" menu\n\nUse this menu to change appearance characteristics of the active terminal window, such as the screen size, font, font size and terminal colors."

#define ENABLED_VIEW_MENU_TIP \
	"\n\nTip: You can change ANSI colors using Preferences."



#define TEXT_BALLOON_ENABLED_MENU_VIEW \
	ENABLED_VIEW_MENU_MESSAGE""ENABLED_VIEW_MENU_TIP

#define TEXT_BALLOON_DISABLED_MENU_VIEW \
	ENABLED_VIEW_MENU_MESSAGE"  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_LARGESCREEN \
	"To set the screen size of the active window to be 132 columns wide and 24 rows high, choose this item.  If your screen is not large enough to make a window this size, scroll bars will be enabled."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_LARGESCREEN \
	"Sets the screen size of the active window to be 132 columns wide and 24 rows high.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_SMALLSCREEN \
	"To set the screen size of the active window to be 80 columns wide and 24 rows high, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_SMALLSCREEN \
	"Sets the screen size of the active window to be 80 columns wide and 24 rows high.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_SETSCREENSIZE \
	"To set the screen size of the active window to be an arbitrary number of rows and columns, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_SETSCREENSIZE \
	"Sets the screen size of the active window to be an arbitrary number of rows and columns.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_SHOWHIDETOOLBAR \
	"To collapse or expand the top part of the active window, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_SHOWHIDETOOLBAR \
	"Collapses or expands the top part of the active window.  Not available because the active window has no toolbar."

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_CUSTOMIZETOOLBAR \
	"To modify the contents of the top part of the active window, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_CUSTOMIZETOOLBAR \
	"Lets you modify the contents of the top part of the active window.  Not available because the active window has no toolbar."

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_SHOWHIDESTATUSBAR \
	"To collapse or expand the narrow status text region of the active window, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_SHOWHIDESTATUSBAR \
	"Collapses or expands the narrow status text region of the active window.  Not available because the active window has no status bar."

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_BIGGERTEXT \
	"To increase the font size for the active terminal window, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_BIGGERTEXT \
	"Increases the font size for the active terminal window.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_SMALLERTEXT \
	"To decrease the font size for the active terminal window, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_SMALLERTEXT \
	"Decreases the font size for the active terminal window.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_FULLSCREEN \
	"To set the largest possible font size for the active terminal window (such that the window stays within its display), choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_FULLSCREEN \
	"Sets the largest possible font size for the active terminal window (such that the window stays within its display).  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_FORMAT \
	"To change the character set, font, font size, or colors used for the active terminal window, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_FORMAT \
	"Changes the character set, font, font size, or colors used for the active terminal window.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_VIEW_ENABLED_ITEM_FULLSCREENMODAL \
	"To make the active terminal window take over your entire display, according to kiosk options you specify, choose this item."

#define TEXT_BALLOON_MENU_VIEW_DISABLED_ITEM_FULLSCREENMODAL \
	"Makes the active terminal window take over your entire display, according to kiosk options you specify.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

/****************************************************************************/

/*
TERMINAL MENU BALLOONS
*/

// convenience strings
#define ENABLED_TERMINAL_MENU_MESSAGE \
	MENUNAME_TERMINAL" menu\n\nUse this menu to temporarily change session parameters or initiate or terminate a capture to a file."



#define TEXT_BALLOON_ENABLED_MENU_TERMINAL \
	ENABLED_TERMINAL_MENU_MESSAGE

#define TEXT_BALLOON_DISABLED_MENU_TERMINAL \
	ENABLED_TERMINAL_MENU_MESSAGE"  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_CAPTURETOFILE \
	"To begin capturing all new text arriving in the active terminal window to a file that you select, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_CAPTURETOFILE \
	"Begins or ends a file capture.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_CHECKED_ITEM_CAPTURETOFILE \
	"To end the file capture in progress, choose this item.  Any text appearing in the window after you choose this item will not get captured."

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_DISABLEBELL \
	"To disable any audio or visual signals for the active terminal window, choose this item.  Useful if you’ve triggered a series of bells and don’t want to be annoyed by repetitive bells or flashes."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_DISABLEBELL \
	"Disables any audio or visual signals for the active terminal window.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_CHECKED_ITEM_DISABLEBELL \
	"To enable audio and/or visual signals for the active terminal window, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_ECHO \
	"To force characters to be displayed locally by MacTelnet as you type on the keyboard, choose this item.  (Ordinarily, characters are displayed only if the server returns them to MacTelnet.)"

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_ECHO \
	"Specifies how characters are handled (by the client or by the server) when keys are pressed on the keyboard.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_CHECKED_ITEM_ECHO \
	"To force characters you type to be handled exclusively by the server, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_WRAPMODE \
	"To force text to continue on the next line of a terminal window instead of being clipped when it reaches the right edge of the screen, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_WRAPMODE \
	"Toggles whether text continues on the next line of a terminal window instead of being clipped when it reaches the right edge of the screen.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_CHECKED_ITEM_WRAPMODE \
	"To clip text when it reaches the right edge of the terminal screen, instead of allowing it to wrap to the next line, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_CLEARSCREENSAVESLINES \
	"To have all visible rows of the terminal screen destroyed whenever a “clear screen” command is encountered, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_CLEARSCREENSAVESLINES \
	"Toggles whether the visible rows are appended to the scrollback whenever a “clear screen” command is encountered.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_CHECKED_ITEM_CLEARSCREENSAVESLINES \
	"To have all visible rows preserved in the scrollback buffer when a “clear screen” command is encountered, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_JUMPSCROLLING \
	"To flush the network, choose this item.  Useful for speeding up the terminal when the remote computer sends back a lot of text at once."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_JUMPSCROLLING \
	"Flushes the network.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_TEKTRONIX \
	"Contains vector graphics options and commands.  Not available because no session window is active, or because the current session has TEK inhibited."

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_SPEECHENABLED \
	"To hear your computer’s voice speak text as it arrives in the active session window, choose this item.  Note that not all text can be accurately spoken."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_SPEECHENABLED \
	"Causes your computer to read text back to you as it arrives in the active session window.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_SPEAKSELECTEDTEXT \
	"To hear your computer’s voice speak the selected text, choose this item."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_SPEAKSELECTEDTEXT \
	"Causes your computer to read the selected text back to you.  Not available because nothing is selected."

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_RESETGRAPHICS \
	"To convert all currently-displayed graphics characters to plain text, choose this item.  Useful if a remote program causes your window to become unreadable."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_RESETGRAPHICS \
	"Converts all currently-displayed graphics characters to plain text.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_TERMINAL_ENABLED_ITEM_RESETTERMINAL \
	"To restore the original state of the terminal, clearing the screen and homing the cursor, choose this item.  Useful if a remote program causes your window to become unreadable."

#define TEXT_BALLOON_MENU_TERMINAL_DISABLED_ITEM_RESETTERMINAL \
	"Restores the original states of several options, including VT and graphics modes, text wrapping, and tabs.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

/****************************************************************************/

/*
KEYS MENU BALLOONS
*/

// convenience strings
#define ENABLED_KEYBOARD_MENU_MESSAGE \
	MENUNAME_KEYBOARD" menu\n\nUse this menu to change the character and key mappings for the active terminal window."



#define TEXT_BALLOON_ENABLED_MENU_KEYBOARD \
	ENABLED_KEYBOARD_MENU_MESSAGE

#define TEXT_BALLOON_DISABLED_MENU_KEYBOARD \
	ENABLED_KEYBOARD_MENU_MESSAGE"  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_KEYBOARD_ENABLED_ITEM_DELETEPRESSSENDSBACKSPACE \
	"To map the delete key on your keyboard to the backspace character, choose this item.\n\nTip: Holding down Shift while pressing Delete will temporarily use the backspace character instead of delete."

#define TEXT_BALLOON_MENU_KEYBOARD_DISABLED_ITEM_DELETEPRESSSENDSBACKSPACE \
	"Maps the delete key on your keyboard to the backspace character.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_KEYBOARD_CHECKED_ITEM_DELETEPRESSSENDSBACKSPACE \
	"The delete key on your keyboard is currently mapped to the backspace character.  To remap it to the delete character, choose the item below this one."

#define TEXT_BALLOON_MENU_KEYBOARD_ENABLED_ITEM_DELETEPRESSSENDSDELETE \
	"To map the delete key on your keyboard to the delete character, choose this item.\n\nTip: Holding down Shift while pressing Delete will temporarily use the delete character instead of backspace."

#define TEXT_BALLOON_MENU_KEYBOARD_DISABLED_ITEM_DELETEPRESSSENDSDELETE \
	"Maps the delete key on your keyboard to the delete character.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_KEYBOARD_CHECKED_ITEM_DELETEPRESSSENDSDELETE \
	"The delete key on your keyboard is currently mapped to the delete character.  To remap it to the backspace character, choose the item above this one."

#define TEXT_BALLOON_MENU_KEYBOARD_ENABLED_ITEM_SETKEYS \
	"To assign the key sequences for the Interrupt, Suspend and Resume operations, choose this item.  If you are using a remote application that requires control keys, you may want to check these key mappings for conflicts."

#define TEXT_BALLOON_MENU_KEYBOARD_DISABLED_ITEM_SETKEYS \
	"Assigns the key sequences for the Interrupt, Suspend and Resume operations.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_KEYBOARD_ENABLED_ITEM_EMACSARROWMAPPING \
	"To map the arrow keys on your keyboard to the standard movement key sequences for the “EMACS” remote application, choose this item."

#define TEXT_BALLOON_MENU_KEYBOARD_DISABLED_ITEM_EMACSARROWMAPPING \
	"Toggles the mapping of arrow keys on your keyboard to the standard movement key sequences for the “EMACS” remote application.  Not available because the active session is using a VT100 emulator."

#define TEXT_BALLOON_MENU_KEYBOARD_CHECKED_ITEM_EMACSARROWMAPPING \
	"To no longer map the arrow keys on your keyboard to the standard movement key sequences for the “EMACS” remote application, choose this item."

#define TEXT_BALLOON_MENU_KEYBOARD_ENABLED_ITEM_LOCALPAGEUPDOWN \
	"To use the page up, page down, home and end keys to affect the terminal display, choose this item."

#define TEXT_BALLOON_MENU_KEYBOARD_DISABLED_ITEM_LOCALPAGEUPDOWN \
	"Toggles the use of page up, page down, home and end keys as either special VT220 keys or normal keys.  Not available because the active session is using a VT100 emulator."

#define TEXT_BALLOON_MENU_KEYBOARD_CHECKED_ITEM_LOCALPAGEUPDOWN \
	"To send the page up, page down, home and end keys to the remote computer instead of using them to affect your local terminal display, choose this item."

/****************************************************************************/

/*
NETWORK MENU BALLOONS
*/

// convenience strings
#define ENABLED_NETWORK_MENU_MESSAGE \
	MENUNAME_NETWORK" menu\n\nUse this menu to determine your computer’s IP address, send special commands, or manage network activity."

#define MOST_HOSTS_IGNORE_COMMAND_ADDENDUM \
	"\n\nNote: Most hosts do not respond to this command correctly."

#define CONFIGURABLE_WITH_SET_KEYS_ADDENDUM \
	"You can configure a control-key equivalent for this action via the Keys menu."



#define TEXT_BALLOON_ENABLED_MENU_NETWORK \
	ENABLED_NETWORK_MENU_MESSAGE

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDFTPCOMMAND \
	"To send a command to the remote host that will automatically initiate an FTP connection back to your FTP server, choose this item."

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDFTPCOMMAND \
	"Sends a command to the remote host that will automatically initiate an FTP connection back to your FTP server.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDINTERNETPROTOCOLNUMBER \
	"To type your computer’s IP number instantly, choose this item."

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDINTERNETPROTOCOLNUMBER \
	"Types your computer’s IP number.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SHOWNETWORKNUMBERS \
	"To view your computer’s IP address or copy it to "THE_CLIPBOARD", choose this item."

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDSYNC \
	"To send a TCP-urgent data mark, causing the server to discard all input it has not yet processed, choose this item."

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDSYNC \
	"Sends a TCP-urgent data mark, causing the server to discard all input it has not yet processed.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDBREAK \
	"To send a break sequence, which may have special meaning to the remote host, choose this item."

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDBREAK \
	"Sends a break character, which may have special meaning to the remote host.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDINTERRUPTPROCESS \
	"To tell the remote host to stop the current process and throw away all pending output, choose this item.  "CONFIGURABLE_WITH_SET_KEYS_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDINTERRUPTPROCESS \
	"Tells the remote host to stop the current process and throw away all pending output.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDABORTOUTPUT \
	"To tell the remote host to throw away all output from the current process, choose this item."MOST_HOSTS_IGNORE_COMMAND_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDABORTOUTPUT \
	"Tells the remote host to throw away all output from the current process.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDAREYOUTHERE \
	"To query the remote host for “alive” status, choose this item."

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDAREYOUTHERE \
	"Queries the remote host for “alive” status.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDERASECHARACTER \
	"To backspace by one, choose this item."MOST_HOSTS_IGNORE_COMMAND_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDERASECHARACTER \
	"Backspaces by one.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDERASELINE \
	"To attempt to erase the entire line of text you have entered, choose this item."MOST_HOSTS_IGNORE_COMMAND_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDERASELINE \
	"Attempts to erase the entire line of text you have entered.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SENDENDOFFILE \
	"To send a standard end-of-file character, choose this item.  This is the Control-D character."

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SENDENDOFFILE \
	"Sends a standard end-of-file character.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_WATCHFORACTIVITY \
	"To begin watching the active connection for data transfer events, choose this item.  This is useful for connections that are slow to respond or potentially unresponsive."

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_WATCHFORACTIVITY \
	"Begins a watch of the active session window, for any data transfer events.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_CHECKED_ITEM_WATCHFORACTIVITY \
	"To turn off the activity watch on the active session window, choose this item.  By choosing this item, you will not see an alert message when data next appears in the active window."

#define TEXT_BALLOON_MENU_NETWORK_ENABLED_ITEM_SUSPENDNETWORK \
	"To turn on Scroll Lock, suspending data display in the active session window, choose this item.  On some keyboards, F14 is equivalent.  "CONFIGURABLE_WITH_SET_KEYS_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_DISABLED_ITEM_SUSPENDNETWORK \
	"Toggles Scroll Lock.  "DISABLED_TERMINAL_RELATED_ACTION_ADDENDUM

#define TEXT_BALLOON_MENU_NETWORK_CHECKED_ITEM_SUSPENDNETWORK \
	"To turn off Scroll Lock, allowing data to once again appear in the active session window, choose this item."

/****************************************************************************/

/*
WINDOW MENU BALLOONS
*/

// convenience strings
#define ENABLED_WINDOW_MENU_MESSAGE \
	MENUNAME_WINDOW" menu\n\nUse this menu to switch windows, hide, show or stack windows, or change a window’s title."

#define ONE_OR_FEWER_WINDOWS_ADDENDUM \
	"Not available because there is one or fewer session windows open."



#define TEXT_BALLOON_ENABLED_MENU_WINDOW \
	ENABLED_WINDOW_MENU_MESSAGE

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_HIDEFRONTWINDOW \
	"To obscure the active session window from view, choose this item.  Hiding a window allows a connection to continue to be active without taking up space on your screen."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_HIDEFRONTWINDOW \
	"Obscures the active session window from view.  "ONE_OR_FEWER_WINDOWS_ADDENDUM

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_HIDEOTHERWINDOWS \
	"To obscure all windows except for the active session window, choose this item.  Equivalent to invoking the above command on these windows."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_HIDEOTHERWINDOWS \
	"Obscures all windows except for the active session window.  "ONE_OR_FEWER_WINDOWS_ADDENDUM

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_SHOWALLHIDDENWINDOWS \
	"To redisplay all windows previously obscured using one of the two commands above, choose this item."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_SHOWALLHIDDENWINDOWS \
	"Redisplays all windows previously obscured using one of the two commands above.  Not available because no windows are hidden."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_NEXTWINDOW \
	"To move between open session windows, choose this item.\n\nTip: There are Option and Shift key variants of this command."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_NEXTWINDOW \
	"Activates a different session window.  "ONE_OR_FEWER_WINDOWS_ADDENDUM

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_STACKWINDOWS \
	"To align the top-left corners of all windows along an invisible, diagonal line, choose this item."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_STACKWINDOWS \
	"Aligns the top-left corners of all windows along an invisible, diagonal line.  "ONE_OR_FEWER_WINDOWS_ADDENDUM

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_CHANGEWINDOWTITLE \
	"To set the title of the active session window, choose this item."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_CHANGEWINDOWTITLE \
	"Allows you to set the title of the active session window.  Not available because the active window’s title cannot be changed, or because no windows are open."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_SHOWSESSIONSTATUS \
	"To show or hide a window containing information on all open remote terminal windows, choose this item."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_SHOWMACROS \
	"To display or remove the macro editor, choose this item."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_SHOWCOMMANDLINE \
	"To display or remove the command line window, choose this item.  You can use this to enter text destined for the active session window, or to handle URLs."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_SHOWCONTROLKEYS \
	"To display or remove the control keys window, choose this item.  You can use this keypad to “type” characters normally generated on terminals by holding down the Control key and typing a character."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_SHOWCONTROLKEYS \
	"Displays or removes the control keys window.  Not available because the VT220 keys cannot be used when no session windows are open."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_SHOWFUNCTION \
	"To display or remove the VT220 function keys window, choose this item.  You can use this keypad to “type” function keys, as well as special keys (such as “help” and “do”)."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_SHOWFUNCTION \
	"Displays or removes the VT220 function keys window.  Not available because the VT220 keys cannot be used when no session windows are open."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_SHOWKEYPAD \
	"To display or remove the VT220 keypad window, choose this item.  You can use this keypad to see how keys on your extended keyboard are mapped for VT220, and you can “type” these keys by clicking the key buttons in the keypad window."

#define TEXT_BALLOON_MENU_WINDOW_DISABLED_ITEM_SHOWKEYPAD \
	"Displays or removes the VT220 keypad window.  Not available because the VT220 keys cannot be used when no session windows are open."

#define TEXT_BALLOON_MENU_WINDOW_ENABLED_ITEM_GENERIC_CONNECTION \
	"To bring this connection’s window to the front if it is open, or to determine the status of the connection if it is not open, choose this item."

#define TEXT_BALLOON_MENU_WINDOW_CHECKED_ITEM_GENERIC_CONNECTION \
	"This item is checked because the active window is this connection’s window.  If you type on the keyboard or click keys in a keypad window, your actions will affect only the active window."

/****************************************************************************/

/*
SCRIPTS MENU BALLOONS
*/

#define TEXT_BALLOON_ENABLED_MENU_SCRIPTS \
	MENUNAME_SCRIPTS" menu\n\nUse this menu to run your favorite scripts, located in MacTelnet’s Scripts Menu Items folder."

#define TEXT_BALLOON_MENU_SCRIPTS_ENABLED_ITEM_GENERIC_SCRIPT \
	"To run the script in MacTelnet’s Scripts Menu Items folder with this name, choose this item."

/****************************************************************************/

/*
TEKTRONIX SUBMENU BALLOONS
*/

#define TEXT_BALLOON_MENU_TEKTRONIX_ENABLED_ITEM_TEKPAGECOMMAND \
	"To generate a TEK PAGE command, choose this item.  This will display a TEK graphics window or, if the item below is checked, clear the contents of any visible graphics window."

#define TEXT_BALLOON_MENU_TEKTRONIX_ENABLED_ITEM_TEKPAGECLEARSSCREEN \
	"To have any existing graphics window clear its contents instead of opening a new window when a PAGE command is encountered, choose this item."

#define TEXT_BALLOON_MENU_TEKTRONIX_CHECKED_ITEM_TEKPAGECLEARSSCREEN \
	"To open a new graphics window when a PAGE command is encountered instead of clearing the contents of any visible one, choose this item."

/****************************************************************************/

/*
INTERNET RESOURCES SUBMENU BALLOONS
*/

#define TEXT_BALLOON_MENU_TELNETONTHEWEB_ENABLED_ITEM_TELNETHOMEPAGE \
	"To launch your preferred web browser and automatically visit the official web site of this program, choose this item."

#define TEXT_BALLOON_MENU_TELNETONTHEWEB_ENABLED_ITEM_TELNETAUTHORMAIL \
	"To launch your preferred E-mail application and automatically create a new message addressed to the author of this program, choose this item."

#define TEXT_BALLOON_MENU_TELNETONTHEWEB_ENABLED_ITEM_TELNETSOURCELICENSE \
	"To launch your preferred web browser and automatically view the GNU General Public License, choose this item."

#define TEXT_BALLOON_MENU_TELNETONTHEWEB_ENABLED_ITEM_TELNETLIBRARYLICENSE \
	"To launch your preferred web browser and automatically view the GNU Lesser Public License, choose this item."

#define TEXT_BALLOON_MENU_TELNETONTHEWEB_ENABLED_ITEM_TELNETPROJECTSTATUS \
	"To launch your preferred web browser and automatically visit the project status page of this program, choose this item."



/*********************************************
		DIALOG BOX BALLOON HELP STRINGS
*********************************************/

/*
FORMAT DIALOG BALLOONS
*/

// convenience strings
#define FORMAT_FONT_NOT_AVAILABLE_ADDENDUM \
	"  Not available because the format you are editing derives its font from another format."

#define FORMAT_FONTSIZE_NOT_AVAILABLE_ADDENDUM \
	"  Not available because the format you are editing derives its size from another format."

#define FORMAT_FORECOLOR_NOT_AVAILABLE_ADDENDUM \
	"  Not available because the format you are editing derives its text color from another format."

#define FORMAT_BACKCOLOR_NOT_AVAILABLE_ADDENDUM \
	"  Not available because the format you are editing derives its background color from another format."



#define TEXT_BALLOON_DIALOG_FORMAT_ENABLED_POPUPMENU_EDIT_WHAT \
	"To edit a different format, choose an item from this menu."

#define TEXT_BALLOON_DIALOG_FORMAT_ENABLED_POPUPMENU_FONT \
	"To change the font used to draw text in this format, choose an item from this menu.  The currently selected font is being used to render the text of this pop-up menu button."

#define TEXT_BALLOON_DIALOG_FORMAT_DISABLED_POPUPMENU_FONT \
	"This menu is used to change the font used for drawing text in this format."FORMAT_FONT_NOT_AVAILABLE_ADDENDUM

#define TEXT_BALLOON_DIALOG_FORMAT_ENABLED_FIELD_FONTSIZE \
	"Enter here a point size to use for rendering text in the font indicated above.  Alternately, use the controls to the right of this field to change the size shown here."

#define TEXT_BALLOON_DIALOG_FORMAT_DISABLED_FIELD_FONTSIZE \
	"This field is not available because the format you are editing derives its size from another format."

#define TEXT_BALLOON_DIALOG_FORMAT_ENABLED_LITTLEARROWS_FONTSIZE \
	"To make the indicated font size slightly larger or smaller, click one of these arrows."

#define TEXT_BALLOON_DIALOG_FORMAT_DISABLED_LITTLEARROWS_FONTSIZE \
	"Makes minor adjustments to the font size indicated."FORMAT_FONTSIZE_NOT_AVAILABLE_ADDENDUM

#define TEXT_BALLOON_DIALOG_FORMAT_ENABLED_POPUPMENU_FONTSIZE \
	"To change the indicated font size to a pre-defined size, choose an item from this menu.  For some fonts, it is best to choose a supported font size from this menu."

#define TEXT_BALLOON_DIALOG_FORMAT_DISABLED_POPUPMENU_FONTSIZE \
	"Changes the font size indicated to one of the pre-defined sizes."FORMAT_FONTSIZE_NOT_AVAILABLE_ADDENDUM

#define TEXT_BALLOON_DIALOG_FORMAT_ENABLED_COLORBOX_FOREGROUND \
	"To set the color for rendering text with this format, click this button."

#define TEXT_BALLOON_DIALOG_FORMAT_DISABLED_COLORBOX_FOREGROUND \
	"This button is used to change the color of text.  "FORMAT_FORECOLOR_NOT_AVAILABLE_ADDENDUM

#define TEXT_BALLOON_DIALOG_FORMAT_ENABLED_COLORBOX_BACKGROUND \
	"To set the color that normally appears behind text drawn with this format, click this button.  This color also affects anti-aliasing behavior and the appearance of inactive text."

#define TEXT_BALLOON_DIALOG_FORMAT_DISABLED_COLORBOX_BACKGROUND \
	"This button is used to change the color used for regions behind text.  "FORMAT_BACKCOLOR_NOT_AVAILABLE_ADDENDUM

/****************************************************************************/

/*
TERMINAL EDITOR DIALOG BALLOONS
*/

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_FIELD_NAME \
	"Enter here a name for this terminal configuration."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_TEXT_DEFAULT \
	"The name of this terminal configuration; the Default configuration is used for any session that does not request a different terminal configuration."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_ANSI \
	"To use special colors and graphics characters, click this box.  You will only see these colors when a remote host returns sequences indicating that they should be used."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_ANSI \
	"To not use special colors or graphics characters, click this box.  Some remote hosts provide a more interesting user interface when ANSI sequences are enabled."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_XTERM \
	"To recognize XTerm sequences, enabling remote hosts to perform actions such as changing the window title, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_XTERM \
	"To ignore XTerm sequences, preventing remote hosts from performing actions such as changing the window title, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_WRAP \
	"To have text continue on the next line of the window instead of being clipped when it reaches the right edge, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_WRAP \
	"To have text clipped when it reaches the right edge of the terminal screen, instead of wrapping to the next line, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_EMACSARROWS \
	"To have arrow keys and cursor movement mouse clicks use sequences compatible with the EMACS text editor, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_EMACSARROWS \
	"To have arrow keys and cursor movement mouse clicks behave normally, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_8BIT \
	"To set the terminal to recognize all 8 bits of each character received from remote hosts, click this box.  Some hosts require this configuration."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_8BIT \
	"To set the terminal to ignore the most significant bit of each character received from remote hosts, click this box.  Some hosts want all 8 bits to be used."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_SAVEONCLEAR \
	"To have all visible rows of the terminal screen preserved whenever a “clear screen” command is encountered, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_SAVEONCLEAR \
	"To have all visible rows of the terminal screen destroyed whenever a “clear screen” command is encountered, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_REMAPKEYPAD \
	"To have the PF1, PF2, PF3 and PF4 keys of the VT220 keypad ignored (allowing normal Mac keypad keys), click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_REMAPKEYPAD \
	"To map the keypad keys “clear”, =, / and * respectively to the PF1, PF2, PF3 and PF4 keys of the VT220 keypad (recommended), click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_CHECKBOX_LOCALPAGEKEYS \
	"To use the “home”, “page up”, “page down” and “end” keys to affect only the local terminal display, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_CHECKBOX_LOCALPAGEKEYS \
	"To set the terminal so that “home”, “page up”, “page down” and “end” keypresses are sent to the remote host, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_RADIOBUTTON_METACTRLCMD \
	"To emulate the meta key using the key combination of the control and command keys, click this box."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_RADIOBUTTON_METACTRLCMD \
	"The highlighting indicates that the control and command keys will emulate the meta key when held down together."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_RADIOBUTTON_METAOPTION \
	"To emulate the meta key using the option key, click here."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_RADIOBUTTON_METAOPTION \
	"The highlighting indicates that the option key will emulate the meta key."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_RADIOBUTTON_METANONE \
	"To not use any key equivalent for the meta key, click here.  This will make the EMACS text editor hard to use."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_RADIOBUTTON_METANONE \
	"The highlighting indicates that there will be no meta key equivalent.  This will make the EMACS text editor hard to use."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_RADIOBUTTON_EMULATEVT100 \
	"To make this terminal emulate the VT100, click here.  Most remote hosts are compatible with the VT100 terminal."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_RADIOBUTTON_EMULATEVT100 \
	"The highlighting indicates that this terminal is configured to emulate the VT100."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_RADIOBUTTON_EMULATEVT220 \
	"To make this terminal emulate the VT220, click here.  The VT220 is more powerful than the VT100, but is not compatible with all hosts."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_RADIOBUTTON_EMULATEVT220 \
	"The highlighting indicates that this terminal is configured to emulate the VT220."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_UNCHECKED_RADIOBUTTON_EMULATENONE \
	"To make this a “dumb” terminal, click here.  No emulation is done, all nonprintable characters are displayed as numbers like this: <255>."

#define TEXT_BALLOON_DIALOG_TERMINALEDITOR_CHECKED_RADIOBUTTON_EMULATENONE \
	"The highlighting indicates that this terminal is configured to be a DUMB terminal.  This is not recommended for most connections."



/*********************************************
		GENERIC BALLOON HELP STRINGS
*********************************************/

/*
These are Balloon strings that appear, usually, in more than
one place.  No matter how the text is worded, do not assume
that the help text has only one context.  Try to localize this
text as closely as possible to the English wording given.
*/

#define TEXT_BALLOON_GENERIC_CONNECTIONS_DISABLED_MENU_ITEM_NONE_OPEN \
	"This item is not available because there are no windows open."

#define TEXT_BALLOON_GENERIC_CONNECTIONS_UNCHECKED_CHECKBOX_UPDATEDEFTERM \
	"To automatically update your Default terminal configuration preference to include the settings above, click this box."

#define TEXT_BALLOON_GENERIC_DIALOG_DISABLED_MENU_ITEM \
	"This item is not available because there is a dialog box on your screen."

#define TEXT_BALLOON_GENERIC_DIALOG_DISABLED_MENU \
	"This menu is not available because there is a dialog box on your screen."

#define TEXT_BALLOON_GENERIC_DIALOG_ENABLED_BUTTON_HELP \
	"To open MacTelnet Help to a topic related to this dialog box, click here."

#define TEXT_BALLOON_GENERIC_DIALOG_DISABLED_BUTTON_HELP \
	"MacTelnet Help may be missing."

#define TEXT_BALLOON_GENERIC_DIALOG_ENABLED_BUTTON_CANCEL \
	"To discard your changes and close this dialog box without doing anything, click here."

#endif /* REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
