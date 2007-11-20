/*
**************************************************************************
	LocalizedAlertMessages.h
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains strings that appear in the form of Note, Stop or
	Caution alerts to the user.
	
	It may be helpful to view this file in the system font.
**************************************************************************
*/

#ifndef __LOCALIZEDALERTMESSAGES__
#define __LOCALIZEDALERTMESSAGES__



#pragma mark Constants

/*
alert titles
*/

// used if a fatal error occurs while MacTelnet is launching
#define LOCALIZED_WindowName_LaunchError \
	"Startup Error"

// if MacTelnet is working properly, this alert title will never be used!
#define LOCALIZED_WindowName_ProgramException \
	"Fatal Error"

// appears when the close box of a connection window is clicked, or the Disconnect/Close command is used
#define LOCALIZED_WindowName_Close \
	"Close"

// appears when the Quit command has been used while more than one terminal window is open
#define LOCALIZED_WindowName_Quit \
	"Quit"

// appears when a script is run and encounters an error
#define LOCALIZED_WindowName_ScriptError \
	"Script Error"

// appears whenever a macro export operation is attempted on an empty set of macros
#define LOCALIZED_WindowName_MacroExportNotPossible \
	"No Macros Defined"

// used when the “open timeout” for a session expires
#define LOCALIZED_WindowName_ConnectionOpeningFailed \
	"Connection Timed Out"

// appears when the user is asked whether or not to accept an unknown host key
#define LOCALIZED_WindowName_NewHostKey \
	"New Host Key"

// appears when the user is asked whether or not to accept a host key that is different than one previously saved
#define LOCALIZED_WindowName_HostKeyChanged \
	"Inconsistent Host Key"



/*
general messages
*/

#define ALERTTEXT_TOO_MANY_OPEN_CONNECTIONS \
	"Sorry, but there are too many "SESSION_PLURAL" open."

#define ALERTTEXT_TOO_MANY_OPEN_CONNECTIONS_HELP_TEXT \
	"You need to close one of the open "SESSION_PLURAL" before opening any more."

#define ALERTTEXT_DOMAIN_NAMES_WONT_WORK_HELP_TEXT \
	"If you have aborted a "SESSION_SINGULAR", you may ignore this problem."

#define ALERTTEXT_IP_ADDRESS_NOT_ATTAINABLE \
	"Cannot determine your computer’s IP address.  Perhaps you are not connected to the Internet."

// %a=(error number)
#define ALERTTEXT_DOMAIN_NAME_RESOLVER_OPEN_FAILED_BECAUSE \
	"The domain name resolver could not be opened because an error of type %a occurred."

#define ALERTTEXT_WILL_NOT_BE_ABLE_TO_OPEN_THAT_MANY_CONNECTIONS \
	"It will not be possible to open that many "SESSION_PLURAL" at once."

#define ALERTTEXT_PRINTING_FAILED \
	"The selection could not be printed."

#define ALERTTEXT_PRINTING_FAILED_HELP_TEXT \
	"Please try to print the selection again, or print a different selection."

// %a=(host name)
#define ALERTTEXT_CONNECTION_TERMINATED_IMMEDIATELY \
	"The new "SESSION_SINGULAR" for “%a” connected, but then immediately terminated."

// %a=(protocol name), %b=(port number), %c=(host name)
#define ALERTTEXT_CONNECTION_TERMINATED_IMMEDIATELY_HELP_TEXT \
	"Perhaps there is no %a server running on port %b of “%c”."

// %a=(host name), %b=(reason for problem); see ALERTTEXT_DNR… strings, below
#define ALERTTEXT_CONNECTION_OPENING_FAILED \
	"A "SESSION_SINGULAR" for “%a” could not be started, because %b."

#define ALERTTEXT_DNR_GENERAL_ERROR \
	"of unexpected difficulties in looking up the name"

#define ALERTTEXT_DNR_INTERRUPTED_ERROR \
	"the name lookup was interrupted"

#define ALERTTEXT_DNR_OUT_OF_MEMORY \
	"Open Transport has run out of memory"

#define ALERTTEXT_DNR_NETWORK_UNREACHABLE \
	"the network is unreachable"

#define ALERTTEXT_DNR_TIMED_OUT \
	"the maximum waiting time has elapsed (you could try again)"

#define ALERTTEXT_DNR_CONFIGURATION_ERROR \
	"your computer does not appear to be properly configured for networking"

#define ALERTTEXT_DNR_NAME_DOES_NOT_EXIST \
	"no existing "SESSION_FAVORITES_SINGULAR", domain name, or IP address matches it"

// %a=(OS error number), %b=(internal error number)
#define ALERTTEXT_OSERR_AND_INTERNALERR_HELP_TEXT \
	"An error of type %a has occurred (internal %b)."

#define ALERTTEXT_CANT_REMOVE_DEFAULT_LIST_ITEM \
	"Sorry, you cannot remove the default item in this list."

// %a=(configuration data); see “Configuration Data” strings
#define ALERTTEXT_DUPLICATE_CONFIGURATION \
	"There is already %a with that name."

// %a=(plural configuration data); see “Configuration Data” strings
#define ALERTTEXT_DUPLICATE_CONFIGURATION_HELP_TEXT \
	"All %a must have distinct names.  Please enter a different name."

// %a=(reason); see “Operation Failed Messages”
#define ALERTTEXT_COMMAND_CANNOT_CONTINUE \
	"This command cannot continue because %a."

// %a=(OS error number)
#define ALERTTEXT_SCRIPT_OSERR_HELP_TEXT \
	"An error of type %a has occurred."



/*
operation failed messages (substituted into ALERTTEXT_COMMAND_CANNOT_CONTINUE, above)
*/

#define ALERTTEXT_OPFAILED_CANT_CREATE_FILE \
	"the file could not be created"

#define ALERTTEXT_OPFAILED_CANT_OPEN_FILE \
	"the file could not be opened"

#define ALERTTEXT_OPFAILED_OUT_OF_MEMORY \
	"there is not enough free memory"

#define ALERTTEXT_OPFAILED_BAD_SET \
	"the "SAVED_SET" contains a keyword that is not valid"

#define ALERTTEXT_OPFAILED_NO_PRINTER \
	"no printer has been selected for this computer"



/*
status messages
*/

#define ALERTTEXT_STATUS_PRINTING_IN_PROGRESS \
	"Printing selection (-. cancels)…"

// appears at startup when the font list needs rebuilding
#define ALERTTEXT_STATUS_BUILDING_FONT_LIST \
	"Saving names of available fonts…"

#define ALERTTEXT_STATUS_BUILDING_SCRIPTS_MENU \
	"Building "MENUNAME_SCRIPTS" menu…"

#define ALERTTEXT_STATUS_GENERATING_KEYS \
	"Generating keys…"

// %a=(host)
#define ALERTTEXT_STATUS_NOT_CONNECTED \
	"Disconnected from “%a”."

// %a=(host), %b=(time), %c=(date, which may be “today” or “yesterday”)
#define ALERTTEXT_STATUS_CONNECTED_SINCE \
	"Connected to “%a” since %b on %c."

// relative date: today
#define ALERTTEXT_STATUS_TIME_TODAY \
	"today"

// relative date: yesterday
#define ALERTTEXT_STATUS_TIME_YESTERDAY \
	"yesterday"



/*
“note” alerts
*/

#define ALERTTEXT_MACRO_IMPORT_SUCCESSFUL \
	"Your macros have been imported successfully.  To view, manipulate or find out the key equivalent for a macro, use the Preferences window."

#define ALERTTEXT_SHOW_IP_ADDRESS \
	"Your computer has at least the following IP addresses:"

#define ALERTTEXT_ACTIVITY_DETECTED \
	"Information has appeared in one of your watched "SESSION_SINGULAR" windows."

#define ALERTTEXT_NOTIFICATION_ALERT \
	"MacTelnet requires your attention."



/*
“caution” alerts
*/

#define ALERTTEXT_CRITICALLY_LOW_MEMORY \
	"This program is running dangerously low on memory.  Please close some windows, or quit as soon as possible to avoid a crash."

// very generic
#define ALERTTEXT_LOCATING_MENTIONED_COMMAND_HELP_TEXT \
	"Click the question-mark icon for assistance."

#define ALERTTEXT_CONFIRM_ANSI_COLORS_RESET \
	"Replace with the 16 default ANSI colors?"

// very generic
#define ALERTTEXT_PROCEDURE_NOT_UNDOABLE_HELP_TEXT \
	"This procedure cannot be undone."

#define ALERTTEXT_PREFERENCES_SAVED_ARE_NEWER \
	"The preferences on disk are from a newer version of MacTelnet.  Do you still want to save preferences, removing the data not relevant to this version of MacTelnet?"

// %a=(configuration name: user, terminal or session)
#define ALERTTEXT_CONFIRM_REMOVE_LIST_ITEM \
	"Remove “%a” from the list?"

// %a=(number of items selected)
#define ALERTTEXT_CONFIRM_REMOVE_MANY_LIST_ITEMS \
	"Delete the %a selected items in the list?"

// %a=(host name)
#define ALERTTEXT_CONFIRM_ACCEPT_HOST_KEY \
	""

#define ALERTTEXT_CONFIRM_ACCEPT_HOST_KEY_HELP_TEXT \
	""

// %a=(host name)
#define ALERTTEXT_INCONSISTENT_HOST_KEY \
	""

// %a=(host name)
#define ALERTTEXT_INCONSISTENT_HOST_KEY_HELP_TEXT \
	""

#define ALERTTEXT_PREFERENCES_CANNOT_BE_LOADED \
	"Preferences were not read completely due to an unexpected error.  Some saved settings may not take " \
	"effect, but MacTelnet will still work without them."

#define ALERTTEXT_PREFERENCES_CANNOT_BE_LOADED_HELP_TEXT \
	"If this problem persists, your preferences files may be corrupted.  Next time, try disabling your " \
	"MacTelnet Preferences folder by moving it to the Desktop (a default will appear in its place)."

#define ALERTTEXT_PREFERENCES_ON_DISK_ARE_OLDER \
	"Preferences found on disk appear to be from an older version of this application - defaults will be " \
	"created as needed."

#define ALERTTEXT_PREFERENCES_ON_DISK_ARE_OLDER_HELP_TEXT \
	"Files will be upgraded automatically, so you should not see this message again."

#define ALERTTEXT_PREFERENCES_ON_DISK_ARE_NEWER \
	"Preferences found on disk appear to be from a newer version of this application - not all of your " \
	"saved settings may be used."

#define ALERTTEXT_PREFERENCES_ON_DISK_ARE_NEWER_HELP_TEXT \
	"MacTelnet will preserve the file, not disturbing any data it is unfamiliar with."



/*
startup errors
*/

#define ALERTTEXT_PREFERENCES_CORRUPTED_ATTEMPT_FIX \
	"The preferences file cannot be read properly.  It might be damaged, and may be possible to repair using default settings.  Repair the preferences file?"

#define ALERTTEXT_PREFERENCES_REPAIR_FAILED \
	"The preferences file could not be repaired.  Please quit, delete the preferences file for this program and try again."

#define ALERTTEXT_VITAL_RESOURCE_MISSING \
	"A vital application resource appears to be missing or is corrupted.  Please replace the MacTelnet application with a new copy."

#define ALERTTEXT_UNABLE_TO_OPEN_MACTCP \
	"Sorry, MacTelnet cannot run because it was unable to start up the TCP driver."

#define ALERTTEXT_UNABLE_TO_CREATE_PREFERENCES_FILE \
	"Sorry, a preferences file could not be created for some reason."

#define ALERTTEXT_WRONG_SYSENVIRONS_VERSION \
	"Sorry, this program will not run on your computer for some reason."

#define ALERTTEXT_MAC_OS_VERSION_TOO_OLD \
	"Sorry, cannot run on this version of the Mac OS.  Please check the web site to see if there is an alternate version that does."

#define ALERTTEXT_MAC_ROM_TOO_OLD \
	"Sorry, MacTelnet cannot run on this computer because the computer ROM is smaller than 128 K."

#define ALERTTEXT_UNABLE_TO_INSTALL_APPLE_EVENT_HANDLERS \
	"Sorry, a problem occurred while trying to set up inter-application communications."

#define ALERTTEXT_UNABLE_TO_INIT_OPEN_TRANSPORT \
	"Sorry, MacTelnet cannot run because Open Transport could not be set up properly."



/*
network-related errors
*/

#define ALERTTEXT_NETERROR_UNABLE_TO_GET_TCP_STATUS \
	"Sorry, MacTelnet could not determine the status of TCP on this computer."

#define ALERTTEXT_NETERROR_UNABLE_TO_GET_IP_ADDRESS \
	"Sorry, MacTelnet could not get an IP address from TCP."

#define ALERTTEXT_NETERROR_CANT_OPEN_SOCKET_OPEN_FAILED \
	"Sorry, MacTelnet cannot open this "SESSION_SINGULAR" (a TCP socket could not be opened)."

#define ALERTTEXT_NETERROR_CANT_ABORT_CONNECTION \
	"MacTelnet could not abort this "SESSION_SINGULAR" properly — perhaps some of your TCP settings are incorrect."

// %a=(domain name, IP address or Session Favorites name)
#define ALERTTEXT_NETERROR_HOST_OR_GATEWAY_HAS_STILL_NOT_RESPONDED \
	"The host or gateway “%a” has still not responded.  MacTelnet is terminating its attempts to open this "SESSION_SINGULAR"."

// %a=(domain name or Session Favorites name), %b=(last known IP address of host)
#define ALERTTEXT_NETERROR_DNR_FAILED_COULD_USE_CACHED_ADDRESS \
	"The current address of “%a” cannot be determined.  Would you like to try connecting to the last known address of this host, “%b”?"

#define ALERTTEXT_NETERROR_DNR_FAILED_COULD_USE_CACHED_ADDRESS_HELP_TEXT \
	"The domain name system is not responding, so the current IP address of your host cannot be found right now."

#define ALERTTEXT_NETERROR_HOST_OR_GATEWAY_HAS_STILL_NOT_RESPONDED_HELP_TEXT \
	"If you want to wait longer for this "SESSION_SINGULAR" to open next time, increase the Open Timeout in General Preferences."

#define ALERTTEXT_NETERROR_FATAL_CANNOT_ALLOCATE_TCP_BUFFER \
	"MacTelnet unexpectedly failed to allocate a TCP buffer, and must be shut down."



/*
disk-related errors
*/

#define ALERTTEXT_RESERROR_DISK_IS_FULL \
	"Sorry, there is not enough space remaining on the disk to complete this operation."



/*
memory-related errors
*/

#define ALERTTEXT_MEMERROR_CANNOT_LOAD_SET \
	"Sorry, there is not enough memory available to load the "SAVED_SET" you selected."

#define ALERTTEXT_MEMERROR_CANNOT_CREATE_RASTER_GRAPHICS_WINDOW \
	"Sorry, there is not enough memory available to open a raster graphics window at this time."

#define ALERTTEXT_MEMERROR_FATAL_OUT_OF_MEMORY \
	"There is not enough memory available to continue."

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
