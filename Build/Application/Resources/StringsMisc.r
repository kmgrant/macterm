/*
**************************************************************************
	StringsMisc.r
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file describes resource data consisting of strings of text used
	for everything from dialog box messages to button titles.
	
	GOING AWAY.  This file is a placeholder for strings not currently
	defined in more specific files.  Strings are soon going to be put in
	more focused places (for example, control text for a dialog box will
	go into a file specific to that dialog box).
	
	Please try to view this file in the font “Monaco 10”.
**************************************************************************
*/

#ifndef __STRINGSMISC_R__
#define __STRINGSMISC_R__

#include "CoreServices.r"

#include "ApplicationVersion.h"
#include "PreferenceResources.h"
#include "LocalizedAlertMessages.h"
#include "LocalizedPrefPanelFavorites.h"
#include "LocalizedPrefPanelGeneral.h"
#include "TelnetButtonNames.h"
#include "TelnetMenuItemNames.h"
#include "TelnetMiscellaneousText.h"


resource 'STR#' (2000, "General messages", purgeable)
{
	{
		/*1*/ALERTTEXT_TOO_MANY_OPEN_CONNECTIONS,
		/*2*/ALERTTEXT_TOO_MANY_OPEN_CONNECTIONS_HELP_TEXT,
		/*3*/ALERTTEXT_DOMAIN_NAMES_WONT_WORK_HELP_TEXT,
		/*4*/ALERTTEXT_DOMAIN_NAME_RESOLVER_OPEN_FAILED_BECAUSE,
		/*5*/ALERTTEXT_WILL_NOT_BE_ABLE_TO_OPEN_THAT_MANY_CONNECTIONS,
		/*6*/ALERTTEXT_PRINTING_FAILED,
		/*7*/ALERTTEXT_PRINTING_FAILED_HELP_TEXT,
		/*8*/ALERTTEXT_CONNECTION_OPENING_FAILED,
		/*9*/ALERTTEXT_OSERR_AND_INTERNALERR_HELP_TEXT,
		/*10*/ALERTTEXT_CANT_REMOVE_DEFAULT_LIST_ITEM,
		/*11*/ALERTTEXT_DUPLICATE_CONFIGURATION,
		/*12*/ALERTTEXT_DUPLICATE_CONFIGURATION_HELP_TEXT,
		/*13*/ALERTTEXT_COMMAND_CANNOT_CONTINUE,
		/*14*/ALERTTEXT_IP_ADDRESS_NOT_ATTAINABLE,
		/*15*/ALERTTEXT_CONNECTION_TERMINATED_IMMEDIATELY,
		/*16*/ALERTTEXT_CONNECTION_TERMINATED_IMMEDIATELY_HELP_TEXT
	}
};


resource 'STR#' (2001, "Operation Failed Messages", purgeable)
{
	{
		/*1*/ALERTTEXT_OPFAILED_CANT_CREATE_FILE,
		/*2*/ALERTTEXT_OPFAILED_CANT_OPEN_FILE,
		/*3*/ALERTTEXT_OPFAILED_OUT_OF_MEMORY,
		/*4*/ALERTTEXT_OPFAILED_BAD_SET,
		/*5*/ALERTTEXT_OPFAILED_NO_PRINTER
	}
};


resource 'STR#' (2002, "Miscellaneous Strings", purgeable)
{
	{
		/*1*/"",
		/*2*/MISC_NEW_SESSION_DEFAULT_NAME,
		/*3*/MISC_NEW_TERMINAL_DEFAULT_NAME,
		/*4*/MISC_WINDOW_TITLE_ADVISED_USER_NAME,
		/*5*/MISC_WINDOW_TITLE_ADVISED_PASSWORD,
		/*6*/MISC_MAC_OS_8_5_FIND_FILE_NAME,
		/*7*/MISC_PRE_MAC_OS_8_5_FIND_FILE_NAME,
		/*8*/MISC_PICK_NEW_COLOR_PROMPT,
		/*9*/MISC_TRANSLATION_TABLE_TYPE_NONE,
		/*10*/"",
		/*11*/"",
		/*12*/"",
		/*13*/"",
		/*14*/"",
		/*15*/"",
		/*16*/"",
		/*17*/MISC_PRINTOUT_SESSION_NAME_STRING,
		/*18*/"",
		/*19*/MISC_WINDOID_NAME_KEYPAD,
		/*20*/MISC_WINDOID_NAME_FUNCTION,
		/*21*/"",
		/*22*/MISC_WINDOID_NAME_CONTROL_KEYS,
		/*23*/"",
		/*24*/INTERRUPT_PROCESS_TERMINAL_FEEDBACK
	}
};


resource 'STR#' (2003, "Status Strings", purgeable)
{
	{
		/*1*/ALERTTEXT_STATUS_PRINTING_IN_PROGRESS,
		/*2*/ALERTTEXT_STATUS_BUILDING_FONT_LIST,
		/*3*/ALERTTEXT_STATUS_BUILDING_SCRIPTS_MENU,
		/*4*/ALERTTEXT_STATUS_GENERATING_KEYS,
		/*5*/ALERTTEXT_STATUS_NOT_CONNECTED,
		/*6*/ALERTTEXT_STATUS_CONNECTED_SINCE,
		/*7*/ALERTTEXT_STATUS_TIME_TODAY,
		/*8*/ALERTTEXT_STATUS_TIME_YESTERDAY
	}
};


resource 'STR#' (2004, "DNR Error Messages", purgeable)
{
	{
		/*1*/ALERTTEXT_DNR_GENERAL_ERROR,
		/*2*/ALERTTEXT_DNR_INTERRUPTED_ERROR,
		/*3*/ALERTTEXT_DNR_OUT_OF_MEMORY,
		/*4*/ALERTTEXT_DNR_NETWORK_UNREACHABLE,
		/*5*/ALERTTEXT_DNR_TIMED_OUT,
		/*6*/ALERTTEXT_DNR_CONFIGURATION_ERROR,
		/*7*/ALERTTEXT_DNR_NAME_DOES_NOT_EXIST
	}
};


// 'STR#' 2005 - see OnlyClassic.r.


/*
The first half of these strings are titles for Navigation Services
(file-choosing) dialog boxes, and the last half are the messages
displayed when the corresponding dialogs appear.
*/
resource 'STR#' (2006, "Navigation Services Dialog Strings", purgeable)
{
	{
		/*1*/NAVIGATION_SERVICES_DIALOG_TITLE_EXAMPLE_BINARY_FILE,
		/*2*/NAVIGATION_SERVICES_DIALOG_TITLE_TEXT_EDITING_APPLICATION,
		/*3*/NAVIGATION_SERVICES_DIALOG_TITLE_SELECT_DEFAULT_DIRECTORY,
		/*4*/NAVIGATION_SERVICES_DIALOG_TITLE_EXPORT_MACROS,
		/*5*/NAVIGATION_SERVICES_DIALOG_TITLE_IMPORT_MACROS,
		/*6*/NAVIGATION_SERVICES_DIALOG_TITLE_SAVE_SESSION_DESCRIPTION,
		/*7*/NAVIGATION_SERVICES_DIALOG_TITLE_LOAD_SESSION_DESCRIPTION,
		/*8*/NAVIGATION_SERVICES_DIALOG_TITLE_SAVE_CAPTURED_TEXT,
		/*9*/NAVIGATION_SERVICES_DIALOG_TITLE_LOCATE_APPLICATION, 
		/*10*/NAVIGATION_SERVICES_DIALOG_PROMPT_EXAMPLE_BINARY_FILE,
		/*11*/NAVIGATION_SERVICES_DIALOG_PROMPT_TEXT_EDITING_APPLICATION,
		/*12*/NAVIGATION_SERVICES_DIALOG_PROMPT_SELECT_DEFAULT_DIRECTORY,
		/*13*/NAVIGATION_SERVICES_DIALOG_PROMPT_EXPORT_MACROS,
		/*14*/NAVIGATION_SERVICES_DIALOG_PROMPT_IMPORT_MACROS,
		/*15*/NAVIGATION_SERVICES_DIALOG_PROMPT_SAVE_SESSION_DESCRIPTION,
		/*16*/NAVIGATION_SERVICES_DIALOG_PROMPT_LOAD_SESSION_DESCRIPTION,
		/*17*/NAVIGATION_SERVICES_DIALOG_PROMPT_SAVE_CAPTURED_TEXT,
		/*18*/NAVIGATION_SERVICES_DIALOG_PROMPT_LOCATE_APPLICATION
	}
};


resource 'STR#' (2008, "Help Text Strings", purgeable)
{
	{
		/*1*/HELPTEXT_COMMAND_LINE_DEFAULT,
		/*2*/HELPTEXT_FORMAT_DIALOG_FRONTMOST_WINDOW,
		/*3*/HELPTEXT_PREFERENCES_GENERAL_NOTIFICATION_TELNET_MAY_NEED_YOUR_ATTENTION
	}
};


resource 'STR#' (2009, "Clipboard Strings", purgeable)
{
	{
		/*1*/CLIPBOARD_CONTENTS_SUBSTITUTION_STRING,
		/*2*/CLIPBOARD_CONTENTS_DATA_TYPE_TEXT,
		/*3*/CLIPBOARD_CONTENTS_DATA_TYPE_PICTURE,
		/*4*/CLIPBOARD_CONTENTS_DATA_TYPE_UNSUPPORTED,
		/*5*/CLIPBOARD_CONTENTS_EMPTY_STRING,
		/*6*/CLIPBOARD_CONTENTS_SCALING_AMOUNT_SUBSTITUTION_STRING,
		/*7*/CLIPBOARD_CONTENTS_APPROXIMATION_STATEMENT_WITH_TRAILING_SPACE,
		/*8*/CLIPBOARD_SIZE_UNIT_BYTES_PLURAL,
		/*9*/CLIPBOARD_SIZE_UNIT_BYTES_SINGULAR,
		/*10*/CLIPBOARD_SIZE_UNIT_KILOBYTES,
		/*11*/CLIPBOARD_SIZE_UNIT_MEGABYTES
	}
};


resource 'STR#' (2013, "Note Alerts and Help Texts", purgeable)
{
	{
		/*1*/"",
		/*2*/"",
		/*3*/"",
		/*4*/"",
		/*5*/"",
		/*6*/"",
		/*7*/ALERTTEXT_MACRO_IMPORT_SUCCESSFUL,
		/*8*/ALERTTEXT_SHOW_IP_ADDRESS,
		/*9*/ALERTTEXT_ACTIVITY_DETECTED,
		/*10*/ALERTTEXT_NOTIFICATION_ALERT,
		/*11*/ALERTTEXT_SCRIPT_OSERR_HELP_TEXT
	}
};


resource 'STR#' (2014, "Caution Alerts and Help Texts", purgeable)
{
	{
		/*1*/ALERTTEXT_CONNECTIONS_ARE_OPEN_QUIT_ANYWAY,
		/*2*/ALERTTEXT_CONNECTIONS_ARE_OPEN_QUIT_ANYWAY_HELP_TEXT,
		/*3*/ALERTTEXT_CONNECTIONS_ARE_OPEN_CLOSE_ALL_ANYWAY,
		/*4*/ALERTTEXT_CONFIRM_CONNECTION_CLOSE,
		/*5*/ALERTTEXT_CONFIRM_CONNECTION_CLOSE_HELP_TEXT,
		/*6*/ALERTTEXT_CONFIRM_PROCESS_KILL,
		/*7*/ALERTTEXT_CONFIRM_PROCESS_KILL_HELP_TEXT,
		/*8*/ALERTTEXT_LOCATING_MENTIONED_COMMAND_HELP_TEXT,
		/*9*/"",
		/*10*/"",
		/*11*/ALERTTEXT_CONFIRM_ANSI_COLORS_RESET,
		/*12*/ALERTTEXT_PROCEDURE_NOT_UNDOABLE_HELP_TEXT,
		/*13*/ALERTTEXT_PREFERENCES_SAVED_ARE_NEWER,
		/*14*/ALERTTEXT_CONFIRM_REMOVE_LIST_ITEM,
		/*15*/ALERTTEXT_CONFIRM_REMOVE_MANY_LIST_ITEMS,
		/*16*/ALERTTEXT_CONFIRM_ACCEPT_HOST_KEY,
		/*17*/ALERTTEXT_CONFIRM_ACCEPT_HOST_KEY_HELP_TEXT,
		/*18*/ALERTTEXT_INCONSISTENT_HOST_KEY,
		/*19*/ALERTTEXT_INCONSISTENT_HOST_KEY_HELP_TEXT,
		/*20*/ALERTTEXT_PREFERENCES_CANNOT_BE_LOADED,
		/*21*/ALERTTEXT_PREFERENCES_CANNOT_BE_LOADED_HELP_TEXT,
		/*22*/ALERTTEXT_PREFERENCES_ON_DISK_ARE_OLDER,
		/*23*/ALERTTEXT_PREFERENCES_ON_DISK_ARE_OLDER_HELP_TEXT,
		/*24*/ALERTTEXT_PREFERENCES_ON_DISK_ARE_NEWER,
		/*25*/ALERTTEXT_PREFERENCES_ON_DISK_ARE_NEWER_HELP_TEXT
	}
};


resource 'STR#' (2015, "Startup Errors", purgeable)
{
	{
		/*1*/ALERTTEXT_PREFERENCES_CORRUPTED_ATTEMPT_FIX,
		/*2*/ALERTTEXT_PREFERENCES_REPAIR_FAILED,
		/*3*/ALERTTEXT_VITAL_RESOURCE_MISSING,
		/*4*/ALERTTEXT_UNABLE_TO_OPEN_MACTCP,
		/*5*/ALERTTEXT_UNABLE_TO_CREATE_PREFERENCES_FILE,
		/*6*/ALERTTEXT_WRONG_SYSENVIRONS_VERSION,
		/*7*/ALERTTEXT_MAC_OS_VERSION_TOO_OLD,
		/*8*/ALERTTEXT_MAC_ROM_TOO_OLD,
		/*9*/ALERTTEXT_UNABLE_TO_INSTALL_APPLE_EVENT_HANDLERS,
		/*10*/ALERTTEXT_UNABLE_TO_INIT_OPEN_TRANSPORT
	}
};


resource 'STR#' (2016, "Configuration Data", purgeable)
{
	{
		/*1*/A_SESSION,
		/*2*/SESSION_PLURAL,
		/*3*/A_TERMINAL,
		/*4*/TERMINAL_PLURAL,
		/*5*/"", // unused
		/*6*/PREFERENCES_SCRIPTS_BTNNAME,
		/*7*/LOCALIZED_ButtonName_PrefPanelGeneral_PanelName,
		/*8*/PREFERENCES_MACROS_BTNNAME,
		/*9*/PREFERENCES_CONFIGURATIONS_BTNNAME,
		/*10*/"",
		/*11*/"",
		/*12*/"",
		/*13*/"",
		/*14*/"",
		/*15*/LOCALIZED_WindowName_DuplicateSessionFavorite,
		/*16*/LOCALIZED_StaticText_PrefPanelFavorites_SessionDescription,
		/*17*/"",
		/*18*/LOCALIZED_WindowName_DuplicateTerminalFavorite,
		/*19*/LOCALIZED_StaticText_PrefPanelFavorites_TerminalDescription,
		/*20*/"",
		/*21*/"",
		/*22*/"",
		/*23*/SESSION_FAVORITES_DEFAULT_NAME,
		/*24*/TERMINAL_FAVORITES_DEFAULT_NAME
	}
};


resource 'STR#' (2019, "Network-Related Errors", purgeable)
{
	{
		/*1*/ALERTTEXT_NETERROR_UNABLE_TO_GET_TCP_STATUS,
		/*2*/ALERTTEXT_NETERROR_UNABLE_TO_GET_IP_ADDRESS,
		/*3*/ALERTTEXT_NETERROR_CANT_OPEN_SOCKET_OPEN_FAILED,
		/*4*/ALERTTEXT_NETERROR_CANT_ABORT_CONNECTION,
		/*5*/ALERTTEXT_NETERROR_HOST_OR_GATEWAY_HAS_STILL_NOT_RESPONDED,
		/*6*/ALERTTEXT_NETERROR_HOST_OR_GATEWAY_HAS_STILL_NOT_RESPONDED_HELP_TEXT,
		/*7*/ALERTTEXT_NETERROR_FATAL_CANNOT_ALLOCATE_TCP_BUFFER
	}
};


resource 'STR#' (2020, "Disk-Related Errors", purgeable)
{
	{
		/*1*/ALERTTEXT_RESERROR_DISK_IS_FULL
	}
};


resource 'STR#' (2021, "Memory-Related Errors", purgeable)
{
	{
		/*1*/ALERTTEXT_MEMERROR_CANNOT_LOAD_SET,
		/*2*/ALERTTEXT_MEMERROR_CANNOT_CREATE_RASTER_GRAPHICS_WINDOW,
		/*3*/ALERTTEXT_MEMERROR_FATAL_OUT_OF_MEMORY
	}
};


/*
Please localize these new strings only if you know how they
are localized on a UNIX distribution in another language.  Your
average UNIX user will expect to find the “standard” form of
these commands.

WARNING:	Do not make any of these strings longer than 32
			characters.
*/
resource 'STR#' (2022, "Supported UNIX Commands (WARNING: 32-character limit per string)", purgeable)
{
	{
		/*1*/COMMAND_LINE_COMMAND_NAME_help,
		/*2*/COMMAND_LINE_COMMAND_NAME_lynx,
		/*3*/COMMAND_LINE_COMMAND_NAME_telnet,
		/*4*/COMMAND_LINE_COMMAND_NAME_ftp,
		/*5*/COMMAND_LINE_COMMAND_ALIAS_1_telnet,
		/*6*/COMMAND_LINE_COMMAND_ALIAS_2_telnet
	}
};


/*
These strings appear below the command line as soon as the
user types a valid command as defined in the string set above.

The first string is a generic string that appears only if the user
has typed a command that does not correspond to any of the
recognized MacTelnet command line commands.
*/
resource 'STR#' (2023, "Descriptions of valid command line commands (WARNING: 200-character limit per string)", purgeable)
{
	{
		/*1*/COMMAND_NAME_AND_DESCRIPTION_DISPLAY_TEMPLATE,
		/*2*/COMMAND_DESCRIPTION_UNKNOWN,
		/*3*/COMMAND_ARGUMENT_UNKNOWN,
		/*4*/COMMAND_DESCRIPTION_help,
		/*5*/COMMAND_DESCRIPTION_lynx,
		/*6*/COMMAND_DESCRIPTION_telnet,
		/*7*/COMMAND_DESCRIPTION_ftp,
		/*8*/COMMAND_DESCRIPTION_telnet,
		/*9*/COMMAND_DESCRIPTION_telnet
	}
};


/*
You can enter up to 4 help arguments, but you should provide
at least one.  If the second word that the user enters on the
command line is equal to any of these, MacTelnet Help is invoked
to search for the command’s name (which is the first word on
the command line).
*/
resource 'STR#' (2024, "List of Acceptable UNIX-Like Help Arguments (WARNING: 32-character limit per string)", purgeable)
{
	{
		/*1*/COMMAND_LINE_ARGUMENT_NAME_HELP_1,
		/*2*/COMMAND_LINE_ARGUMENT_NAME_HELP_2,
		/*3*/COMMAND_LINE_ARGUMENT_NAME_HELP_3,
		/*4*/COMMAND_LINE_ARGUMENT_NAME_HELP_4
	}
};


resource 'STR#' (2026, "Miscellaneous Control Titles", purgeable)
{
	{
		/*1*/"",
		/*2*/"",
		/*3*/"",
		/*4*/"",
		/*5*/"",
		/*6*/"",
		/*7*/"",
		/*8*/"",
		/*9*/"",
		/*10*/"",
		/*11*/"",
		/*12*/LOCALIZED_RadioButtonName_PrefPanelGeneral_CommandNCreatesShellSession,
		/*13*/LOCALIZED_RadioButtonName_PrefPanelGeneral_CommandNCreatesSessionFromDefaultFavorite,
		/*14*/LOCALIZED_RadioButtonName_PrefPanelGeneral_CommandNDisplaysDialog,
		/*15*/LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodNone,
		/*16*/LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodDiamond,
		/*17*/LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodFlashIcon,
		/*18*/LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodDisplayAlert,
		/*19*/LOCALIZED_CheckboxName_PrefPanelGeneral_CursorFlashing
	}
};


resource 'STR#' (2027, "Miscellaneous Static Text", purgeable)
{
	{
		/*1*/DIALOGTEXT_PREFERENCES_GENERAL_NOTIFICATION_IF_TELNET_NEEDS_YOUR_ATTENTION,
		/*2*/DIALOGTEXT_PREFERENCES_GENERAL_CURSOR_SHAPE,
		/*3*/DIALOGTEXT_PREFERENCES_GENERAL_STACKING_ORIGIN_LEFT,
		/*4*/DIALOGTEXT_PREFERENCES_GENERAL_STACKING_ORIGIN_TOP,
		/*5*/DIALOGTEXT_PREFERENCES_GENERAL_COPY_TABLE_THRESHOLD,
		/*6*/DIALOGTEXT_PREFERENCES_GENERAL_COPY_TABLE_THRESHOLD_UNITS,
		/*7*/DIALOGTEXT_FORMAT_SIZE_LABEL,
		/*8*/DIALOGTEXT_FORMAT_COLOR_LABEL_FOREGROUND,
		/*9*/DIALOGTEXT_FORMAT_COLOR_LABEL_BACKGROUND,
		/*10*/"",//DIALOGTEXT_SCREENSIZE_WIDTH_LABEL,
		/*11*/"",//DIALOGTEXT_SCREENSIZE_HEIGHT_LABEL,
		/*12*/DIALOGTEXT_SCREENSIZE_WIDTH_UNITS,
		/*13*/DIALOGTEXT_SCREENSIZE_HEIGHT_UNITS,
		/*14*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_BLACK,
		/*15*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_RED,
		/*16*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_GREEN,
		/*17*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_YELLOW,
		/*18*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_BLUE,
		/*19*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_MAGENTA,
		/*20*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_CYAN,
		/*21*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_WHITE,
		/*22*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_HEADER,
		/*23*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_NORMAL_HEADER,
		/*24*/DIALOGTEXT_PREFERENCES_GENERAL_ANSI_EMPHASIZED_HEADER
	}
};


/*
Names of files, in various directories, that MacTelnet uses.
MacTelnet often looks up files exactly by name, so it is
important (for localization and version control purposes) to
keep these names external in a string list resource.

A future version of MacTelnet will use aliases everywhere,
to avoid this limiting approach to file management.
*/
resource 'STR#' (2029, "Important File Names", purgeable)
{
	{
		/*1*/"",
		/*2*/PREFERENCES_FONT_LIST_FILE_NAME,
		/*3*/"",
		/*4*/"",
		/*5*/MISC_BELL_SOUND_FILE_NAME,
		/*6*/"",
		/*7*/"",
		/*8*/"",
		/*9*/MISC_MACRO_SET_1_FILE_NAME,
		/*10*/MISC_MACRO_SET_2_FILE_NAME,
		/*11*/MISC_MACRO_SET_3_FILE_NAME,
		/*12*/MISC_MACRO_SET_4_FILE_NAME,
		/*13*/MISC_MACRO_SET_5_FILE_NAME
	}
};

#endif