/*!	\file UIStrings.cp
	\brief An interface to retrieve localized strings intended
	for the User Interface, independently of their storage
	format on disk.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#include "UIStrings.h"

#include <UniversalDefines.h>

// standard-C includes
#include <cstdlib>

// standard-C++ includes
#include <algorithm>
#include <functional>
#include <vector>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>



#pragma mark Public Methods

/*!
Locates the specified alert window string and returns
a reference to a Core Foundation string.  Since a copy
is made, you must call CFRelease() on the returned
string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_AlertWindowCFString	inWhichString,
				 CFStringRef&					outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_AlertWindowANSIColorsResetPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Reset to the 16 standard ANSI colors?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowANSIColorsResetPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowCloseName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Close Window"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCloseName"));
		break;
	
	case kUIStrings_AlertWindowCloseHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("You will lose any unsaved changes."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCloseHelpText"));
		break;
	
	case kUIStrings_AlertWindowClosePrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The processes in this window will be forced to quit.  Close anyway?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowClosePrimaryText"));
		break;
	
	case kUIStrings_AlertWindowCopyToDefaultPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Overwrite the Default in this category\nwith “%1$@” settings?\n\nConsider backing up your\nDefault settings first."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCopyToDefaultPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowCopyToDefaultHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Backups can be created by duplicating a collection or exporting it to a file.\n\nNote that settings in Default determine inherited values in other collections, and they have special uses that are outlined in MacTerm Help."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCopyToDefaultHelpText"));
		break;
	
	case kUIStrings_AlertWindowCommandFailedHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Please report this unexpected problem to the MacTerm maintainers.  Click Quit to exit immediately, or Continue to ignore this error (not recommended)."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCommandFailedHelpText"));
		break;
	
	case kUIStrings_AlertWindowCommandFailedPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The command could not be completed because an error of type %1$d occurred."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCommandFailedPrimaryText; %1$d will be an error code"));
		break;
	
	case kUIStrings_AlertWindowCFFileExistsPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The command could not be completed because the requested file is already in use (error %1$d)."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCFFileExistsPrimaryText; %1$d will be an error code"));
		break;
	
	case kUIStrings_AlertWindowCFNoWritePermissionPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The command could not be completed because the requested disk location is not allowed to change (error %1$d)."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowCFNoWritePermissionPrimaryText; %1$d will be an error code"));
		break;
	
	case kUIStrings_AlertWindowDeleteCollectionPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The selected collection and all of its settings will be deleted.  Continue?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowDeleteCollectionPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowDeleteCollectionHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("This cannot be undone.  Any other settings that refer to this collection will use Default instead."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowDeleteCollectionHelpText"));
		break;
	
	case kUIStrings_AlertWindowExcessiveErrorsPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("A significant number of data errors are occurring.  For stability reasons, MacTerm has avoided trying to render some of the text."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowExcessiveErrorsPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowExcessiveErrorsHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("It is possible that the current Text Translation (set in the Map menu) is inaccurate."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowExcessiveErrorsHelpText"));
		break;
	
	case kUIStrings_AlertWindowGenericCannotUndoHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("This cannot be undone."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowGenericCannotUndoHelpText"));
		break;
	
	case kUIStrings_AlertWindowKillSessionName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Force-Quit Processes"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowKillSessionName"));
		break;
	
	case kUIStrings_AlertWindowKillSessionHelpText:
	case kUIStrings_AlertWindowRestartSessionHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("You will lose any unsaved changes in running programs but the window will stay open and text will be preserved."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowKillSessionHelpText or kUIStrings_AlertWindowRestartSessionHelpText"));
		break;
	
	case kUIStrings_AlertWindowKillSessionPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Kill every process in this window?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowKillSessionPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowMacroExportNothingPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("No macros are active."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowMacroExportNothingPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowMacroExportNothingHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("To change the active macro set or define macros, use the Preferences window."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowMacroExportNothingHelpText"));
		break;
	
	case kUIStrings_AlertWindowMacroImportWarningPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Your active macro set will be modified.  Import anyway?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowMacroImportWarningPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowMacroImportWarningHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The active macro set is whichever one is selected in the Map menu."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowMacroImportWarningHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifyActivityPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("New data in watched window."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyActivityPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowNotifyActivityTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Session Active"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyActivityTitle"));
		break;
	
	case kUIStrings_AlertWindowNotifyInactivityPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("No data lately in watched window."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyActivityPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowNotifyInactivityTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Session Idle"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyInactivityTitle"));
		break;
	
	case kUIStrings_AlertWindowNotifyProcessDieTemplate:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Process exited unsuccessfully (code %1$d)."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyProcessDieTemplate; %1$d is the nonzero exit status code"));
		break;
	
	case kUIStrings_AlertWindowNotifyProcessDieTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Session Failed"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyProcessDieTitle"));
		break;
	
	case kUIStrings_AlertWindowNotifyProcessExitPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Process exited successfully."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyProcessExitPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowNotifyProcessExitTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Session Ended"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyProcessExitTitle"));
		break;
	
	case kUIStrings_AlertWindowNotifyProcessSignalTemplate:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Process unexpectedly quit (exited with signal %1$d)."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifyProcessSignalTemplate; %1$d is the signal number"));
		break;
	
	case kUIStrings_AlertWindowPasteLinesWarningName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Multi-Line Paste"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowPasteLinesWarningName"));
		break;
	
	case kUIStrings_AlertWindowPasteLinesWarningPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Join into a single line before pasting?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowPasteLinesWarningPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowPasteLinesWarningHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("In terminal applications such as shells, entering multiple lines may produce unexpected results."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowPasteLinesWarningHelpText"));
		break;
	
	case kUIStrings_AlertWindowRestartSessionName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Restart Session"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowRestartSessionName"));
		break;
	
	//case kUIStrings_AlertWindowRestartSessionHelpText:
		// see kUIStrings_AlertWindowKillSessionHelpText
		//break;
	
	case kUIStrings_AlertWindowRestartSessionPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The processes in this window will be forced to quit.  Restart anyway?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowRestartSessionPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowScriptErrorName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Script Error"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowScriptErrorName"));
		break;
	
	case kUIStrings_AlertWindowScriptErrorHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("An error of type %1$d occurred."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowScriptErrorHelpText; %1$d will be an error code"));
		break;
	
	case kUIStrings_AlertWindowShowIPAddressesPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Your computer has at least the following IP addresses:"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowShowIPAddressesPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitUsageHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The command was not run correctly (for example, wrong number of arguments)."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitUsageHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitDataErrHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Some of the given data was incorrect."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitDataErrHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitNoInputHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("One or more input files does not exist or is not readable."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitNoInputHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitNoUserHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Specified user was not found."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitNoUserHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitNoHostHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Specified host was not found."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitNoHostHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitUnavailHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Requested service is unavailable."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitUnavailHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitSoftwareHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Internal software error."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitSoftwareHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitOSErrHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("A system error has occurred (not enough resources, for example)."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitOSErrorHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitOSFileHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("A critical operating system file is missing."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitOSFileHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitCreateHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Requested output file could not be created."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitCreateHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitIOErrHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("An input or output error has occurred."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitIOErrorHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitTempFailHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("This failure appears to be temporary; please try again."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitTempFailHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitProtocolHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Server-side error in protocol."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitProtocolHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitNoPermHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("You do not have permission to perform the requested action."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitNoPermHelpText"));
		break;
	
	case kUIStrings_AlertWindowNotifySysExitConfigHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Error in configuration."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowNotifySysExitConfigHelpText"));
		break;
	
	case kUIStrings_AlertWindowUpdateCheckHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("A new version may be available on the web!"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowUpdateCheckHelpText"));
		break;
	
	case kUIStrings_AlertWindowUpdateCheckPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("It has been some time since this copy of MacTerm was updated."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowUpdateCheckPrimaryText"));
		break;
	
	case kUIStrings_AlertWindowUpdateCheckTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("MacTerm Update"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowUpdateCheckTitle"));
		break;
	
	case kUIStrings_AlertWindowQuitName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Quit MacTerm"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowQuitName"));
		break;
	
	case kUIStrings_AlertWindowQuitHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Recently-opened windows will be closed automatically."), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowQuitHelpText"));
		break;
	
	case kUIStrings_AlertWindowQuitPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Review running sessions?"), CFSTR("Alerts"),
													CFSTR("kUIStrings_AlertWindowQuitPrimaryText"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (alert window strings)


/*!
Locates the specified button title string and returns
a reference to a Core Foundation string.  Since a copy
is made, you must call CFRelease() on the returned
string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_ButtonCFString	inWhichString,
				 CFStringRef&				outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_ButtonOK:
		outString = CFCopyLocalizedStringFromTable(CFSTR("OK"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonOK"));
		break;
	
	case kUIStrings_ButtonCancel:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Cancel"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonCancel"));
		break;
	
	case kUIStrings_ButtonSave:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Save"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonSave"));
		break;
	
	case kUIStrings_ButtonDelete:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Delete"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonDelete"));
		break;
	
	case kUIStrings_ButtonOverwrite:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Overwrite"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonOverwrite"));
		break;
	
	case kUIStrings_ButtonStartSession:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Start Session"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonStartSession"));
		break;
	
	case kUIStrings_ButtonDontSave:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Don't Save"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonDontSave"));
		break;
	
	case kUIStrings_ButtonYes:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Yes"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonYes"));
		break;
	
	case kUIStrings_ButtonNo:
		outString = CFCopyLocalizedStringFromTable(CFSTR("No"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonNo"));
		break;
	
	case kUIStrings_ButtonClose:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Close"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonClose"));
		break;
	
	case kUIStrings_ButtonKill:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Force Quit"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonKill"));
		break;
	
	case kUIStrings_ButtonRestart:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Restart"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonRestart"));
		break;
	
	case kUIStrings_ButtonQuit:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Quit"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonQuit"));
		break;
	
	case kUIStrings_ButtonContinue:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Continue"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonContinue"));
		break;
	
	case kUIStrings_ButtonIgnore:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Ignore"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonIgnore"));
		break;
	
	case kUIStrings_ButtonCheckForUpdatesWithEllipsis:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Check for Updates..."), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonCheckForUpdatesWithEllipsis"));
		break;
	
	case kUIStrings_ButtonReviewWithEllipsis:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Review..."), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonReviewWithEllipsis"));
		break;
	
	case kUIStrings_ButtonDiscardAll:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Discard All"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonDiscardAll"));
		break;
	
	case kUIStrings_ButtonPasteNormally:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Paste Normally"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonPasteNormally"));
		break;
	
	case kUIStrings_ButtonMakeOneLine:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Join"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonMakeOneLine"));
		break;
	
	case kUIStrings_ButtonHelpAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("help"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonHelpAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonAddAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("add"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonAddAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonRemoveAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("remove"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonRemoveAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonMoveUpAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("move up"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonMoveUpAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonMoveDownAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("move down"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonMoveDownAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonColorBoxAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("color"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonColorBoxAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonEditTextArrowsAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("edit text numerical value"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonEditTextArrowsAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonEditTextHistoryAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("recent edit text values"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonEditTextHistoryAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	case kUIStrings_ButtonPopUpMenuArrowsAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("pop up menu selection"), CFSTR("Buttons"),
													CFSTR("kUIStrings_ButtonPopUpMenuArrowsAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (button strings)


/*!
Locates the specified string used by the clipboard window.
Returns a reference to a Core Foundation string.  Since a
copy is made, you must call CFRelease() on the returned
string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_ClipboardWindowCFString	inWhichString,
				 CFStringRef&						outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_ClipboardWindowShowCommand:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Show Clipboard"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowShowCommand"));
		break;
	
	case kUIStrings_ClipboardWindowHideCommand:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Hide Clipboard"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowHideCommand"));
		break;
	
	case kUIStrings_ClipboardWindowIconName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Clipboard"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowIconName"));
		break;
	
	case kUIStrings_ClipboardWindowLabelWidth:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Width:"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowLabelWidth; used to label the width property of a copied image"));
		break;
	
	case kUIStrings_ClipboardWindowLabelHeight:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Height:"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowLabelHeight; used to label the height property of a copied image"));
		break;
	
	case kUIStrings_ClipboardWindowPixelDimensionTemplate:
		outString = CFCopyLocalizedStringFromTable(CFSTR("%1$d pixels"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowPixelDimensionTemplate; %1$d is a numeric pixel value"));
		break;
	
	case kUIStrings_ClipboardWindowOnePixel:
		outString = CFCopyLocalizedStringFromTable(CFSTR("one pixel"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowOnePixel; used to represent a size of exactly one pixel"));
		break;
	
	case kUIStrings_ClipboardWindowGenericKindText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("text"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowGenericKindText"));
		break;
	
	case kUIStrings_ClipboardWindowGenericKindImage:
		outString = CFCopyLocalizedStringFromTable(CFSTR("image"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowGenericKindImage"));
		break;
	
	case kUIStrings_ClipboardWindowDataSizeTemplate:
		outString = CFCopyLocalizedStringFromTable(CFSTR("%1$@%2$d %3$@"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowDataSizeTemplate; %1$@ is used if the value is approximate, and should have trailing whitespace; %2$d is a numeric size value; %3$@ is a units string, like MB"));
		break;
	
	case kUIStrings_ClipboardWindowDataSizeApproximately:
		outString = CFCopyLocalizedStringFromTable(CFSTR("about "), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowDataSizeApproximately; substituted into the data size string whenever the size value is not exact, e.g. about 3 MB; must have a trailing space!!!"));
		break;
	
	case kUIStrings_ClipboardWindowUnitsByte:
		outString = CFCopyLocalizedStringFromTable(CFSTR("byte"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowUnitsByte; a single byte"));
		break;
	
	case kUIStrings_ClipboardWindowUnitsBytes:
		outString = CFCopyLocalizedStringFromTable(CFSTR("bytes"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowUnitsBytes; units of bytes"));
		break;
	
	case kUIStrings_ClipboardWindowUnitsK:
		outString = CFCopyLocalizedStringFromTable(CFSTR("K"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowUnitsK; units of kilobytes"));
		break;
	
	case kUIStrings_ClipboardWindowUnitsMB:
		outString = CFCopyLocalizedStringFromTable(CFSTR("MB"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowUnitsMB; units of megabytes"));
		break;
	
	case kUIStrings_ClipboardWindowValueUnknown:
		outString = CFCopyLocalizedStringFromTable(CFSTR("unknown"), CFSTR("ClipboardWindow"),
													CFSTR("kUIStrings_ClipboardWindowValueUnknown; used to represent any unknown property"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (clipboard strings)


/*!
Locates the specified string used by the command line
floating window.  Returns a reference to a Core Foundation
string.  Since a copy is made, you must call CFRelease()
on the returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_CommandLineCFString	inWhichString,
				 CFStringRef&					outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_CommandLineHelpTextCommandArgumentTemplate:
		outString = CFCopyLocalizedStringFromTable(CFSTR("%1$@ %2$@ - %3$@"), CFSTR("CommandLine"),
													CFSTR("kUIStrings_CommandLineHelpTextCommandArgumentTemplate; %1$@ will be a command name; %2$@ will be an argument to that command; %3$@ will be a description of the argument"));
		break;
	
	case kUIStrings_CommandLineHelpTextCommandTemplate:
		outString = CFCopyLocalizedStringFromTable(CFSTR("%1$@ - %2$@"), CFSTR("CommandLine"),
													CFSTR("kUIStrings_CommandLineHelpTextCommandTemplate; %1$@ will be a command name; %2$@ will be a description of the command"));
		break;
	
	case kUIStrings_CommandLineHelpTextDefault:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Text is sent to the terminal window."), CFSTR("CommandLine"),
													CFSTR("kUIStrings_CommandLineHelpTextDefault"));
		break;
	
	case kUIStrings_CommandLineHelpTextFreeInput:
		outString = CFCopyLocalizedStringFromTable(CFSTR("This text will be sent to the frontmost session window."), CFSTR("CommandLine"),
													CFSTR("kUIStrings_CommandLineHelpTextFreeInput"));
		break;
	
	case kUIStrings_CommandLineHistoryMenuAccessibilityDesc:
		outString = CFCopyLocalizedStringFromTable(CFSTR("recent command lines"), CFSTR("CommandLine"),
													CFSTR("kUIStrings_CommandLineHistoryMenuAccessibilityDesc; lowercase with minimal punctuation"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (command line strings)


/*!
Locates the specified string used by a context-sensitive
pop-up menu.  Returns a reference to a Core Foundation
string.  Since a copy is made, you must call CFRelease()
on the returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_ContextualMenuCFString	inWhichString,
				 CFStringRef&						outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_ContextualMenuArrangeAllInFront:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Arrange All Windows in Front"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuArrangeAllInFront"));
		break;
	
	case kUIStrings_ContextualMenuChangeBackground:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Change Background..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuChangeBackground"));
		break;
	
	case kUIStrings_ContextualMenuCloseThisWindow:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Close This Window"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuCloseThisWindow"));
		break;
	
	case kUIStrings_ContextualMenuCopyToClipboard:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Copy"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuCopyToClipboard"));
		break;
	
	case kUIStrings_ContextualMenuCopyUsingTabsForSpaces:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Copy with Tab Substitution"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuCopyUsingTabsForSpaces"));
		break;
	
	case kUIStrings_ContextualMenuCustomFormat:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Custom Format..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuCustomFormat"));
		break;
	
	case kUIStrings_ContextualMenuCustomScreenDimensions:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Custom Screen Size..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuCustomScreenDimensions"));
		break;
	
	case kUIStrings_ContextualMenuFindInThisWindow:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Find..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuFindInThisWindow"));
		break;
	
	case kUIStrings_ContextualMenuFixCharacterTranslation:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Fix Character Translation..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuFixCharacterTranslation"));
		break;
	
	case kUIStrings_ContextualMenuFullScreenEnter:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Enter Full Screen"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuFullScreenEnter"));
		break;
	
	case kUIStrings_ContextualMenuFullScreenExit:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Exit Full Screen"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuFullScreenExit"));
		break;
	
	case kUIStrings_ContextualMenuGroupTitleMacros:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Macros Using Selected Text:"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuGroupTitleMacros"));
		break;
	
	case kUIStrings_ContextualMenuHideThisWindow:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Hide This Window"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuHideThisWindow"));
		break;
	
	case kUIStrings_ContextualMenuMoveToNewWorkspace:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Move to New Workspace"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuMoveToNewWorkspace"));
		break;
	
	case kUIStrings_ContextualMenuOpenThisResource:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Open URL"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuOpenThisResource"));
		break;
	
	case kUIStrings_ContextualMenuPasteText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Paste"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuPasteText"));
		break;
	
	case kUIStrings_ContextualMenuPrintSelectedText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Print Selected Text..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuPrintSelectedText"));
		break;
	
	case kUIStrings_ContextualMenuPrintScreen:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Print Screen..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuPrintScreen"));
		break;
	
	case kUIStrings_ContextualMenuRenameThisWindow:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Rename This Window..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuRenameThisWindow"));
		break;
	
	case kUIStrings_ContextualMenuSaveSelectedText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Save Selection..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuSaveSelectedText"));
		break;
	
	case kUIStrings_ContextualMenuShowCompletions:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Show Completions"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuShowCompletions"));
		break;
	
	case kUIStrings_ContextualMenuSpeakSelectedText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Start Speaking Text"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuSpeakSelectedText"));
		break;
	
	case kUIStrings_ContextualMenuStopSpeaking:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Stop Speaking"), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuStopSpeaking"));
		break;
	
	case kUIStrings_ContextualMenuSpecialKeySequences:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Custom Key Sequences..."), CFSTR("ContextualMenus"),
													CFSTR("kUIStrings_ContextualMenuSpecialKeySequences"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (contextual menu item strings)


/*!
Locates the specified string used for a file or folder
name, and returns a reference to a Core Foundation
string.  Since a copy is made, you must call CFRelease()
on the returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_FileOrFolderCFString		inWhichString,
				 CFStringRef&						outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_FileDefaultCaptureFile:
		outString = CFCopyLocalizedStringFromTable(CFSTR("untitled.txt"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FileDefaultCaptureFile; please localize the name but keep the .txt suffix"));
		break;
	
	case kUIStrings_FileDefaultExportPreferences:
		outString = CFCopyLocalizedStringFromTable(CFSTR("untitled.plist"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FileDefaultExportPreferences; please localize the name but keep the .plist suffix"));
		break;
	
	case kUIStrings_FileDefaultImageFile:
		outString = CFCopyLocalizedStringFromTable(CFSTR("untitled.png"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FileDefaultImageFile; please localize the name but keep the .png suffix"));
		break;
	
	case kUIStrings_FileDefaultMacroSet:
		outString = CFCopyLocalizedStringFromTable(CFSTR("untitled.macros"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FileDefaultMacroSet; please localize the name but keep the .macros suffix"));
		break;
	
	case kUIStrings_FileDefaultSession:
		outString = CFCopyLocalizedStringFromTable(CFSTR("untitled.session"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FileDefaultSession; please localize the name but keep the .session suffix"));
		break;
	
	case kUIStrings_FileNameDockTileAttentionPicture:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("DockTileAttention.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameDockTileAttentionMask:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("DockTileAttentionMask.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame1Picture:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof1.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame1Mask:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof1Mask.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame2Picture:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof2.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame2Mask:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof2Mask.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame3Picture:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof3.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame3Mask:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof3Mask.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame4Picture:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof4.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame4Mask:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof4Mask.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame5Picture:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof5.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FileNameToolbarPoofFrame5Mask:
		// localization of this string is unnecessary; however an explicit retain is now
		// needed because the caller is required to release the string later on
		outString = CFSTR("ToolbarItemPoof5Mask.pict");
		CFRetain(outString);
		break;
	
	case kUIStrings_FolderNameApplicationPreferences:
		outString = CFCopyLocalizedStringFromTable(CFSTR("MacTelnet Preferences"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FolderNameApplicationPreferences; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationRecentSessions:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Recent Sessions"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FolderNameApplicationRecentSessions; warning, this should be consistent with previous strings of this type because it is used to find existing preferences"));
		break;
	
	case kUIStrings_FolderNameApplicationScriptsMenuItems:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Scripts Menu Items"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FolderNameApplicationScriptsMenuItems; warning, this should be consistent with previous strings of this type because it is used to find existing scripts"));
		break;
	
	case kUIStrings_FolderNameHomeLibraryLogs:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Logs"), CFSTR("FileOrFolderNames"),
													CFSTR("kUIStrings_FolderNameHomeLibraryLogs; should EXACTLY MATCH what Mac OS X uses to name this folder (we would ask the system for this name if we could)"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (file or folder strings)


/*!
Locates the specified string used by the help system,
either for display in a menu item or as a search string.
Returns a reference to a Core Foundation string.  Since
a copy is made, you must call CFRelease() on the
returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_HelpSystemCFString	inWhichString,
				 CFStringRef&					outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_HelpSystemName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("MacTerm Help"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemName; must match AppleTitle in HTML"));
		break;
	
	case kUIStrings_HelpSystemContextualHelpCommandName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Contextual Help"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemContextualHelpCommandName"));
		break;
	
	case kUIStrings_HelpSystemShowTagsCommandName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Show Help Tags"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemShowTagsCommandName"));
		break;
	
	case kUIStrings_HelpSystemHideTagsCommandName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Hide Help Tags"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemHideTagsCommandName"));
		break;
	
	case kUIStrings_HelpSystemTopicHelpCreatingSessions:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Help Creating Sessions"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemTopicHelpCreatingSessions; should match topic title"));
		break;
	
	case kUIStrings_HelpSystemTopicHelpSearchingForText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Help Searching For Text"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemTopicHelpSearchingForText; should match topic title"));
		break;
	
	case kUIStrings_HelpSystemTopicHelpSettingTheScreenSize:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Help Setting The Screen Size"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemTopicHelpSettingTheScreenSize; should match topic title"));
		break;
	
	case kUIStrings_HelpSystemTopicHelpUsingTheCommandLine:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Help Using The Floating Command Line"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemTopicHelpUsingTheCommandLine; should match topic title"));
		break;
	
	case kUIStrings_HelpSystemTopicHelpWithPreferences:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Help With Preferences"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemTopicHelpWithPreferences; should match topic title"));
		break;
	
	case kUIStrings_HelpSystemTopicHelpWithScreenFormatting:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Help With Screen Formatting"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemTopicHelpWithScreenFormatting; should match topic title"));
		break;
	
	case kUIStrings_HelpSystemTopicHelpWithTerminalSettings:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Help With Terminal Settings"), CFSTR("HelpSystem"),
													CFSTR("kUIStrings_HelpSystemTopicHelpWithTerminalSettings; should match topic title"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (help system strings)


/*!
Locates the specified string used by Preferences-related interfaces
(not in any one preferences panel) and returns a reference to a Core
Foundation string.  Since a copy is made, you must call CFRelease()
on the returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_PreferencesWindowCFString	inWhichString,
				 CFStringRef&							outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_PreferencesWindowAddToFavoritesButton:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Add to Preferences"), CFSTR("PreferencesWindow"),
													CFSTR("kUIStrings_PreferencesWindowAddToFavoritesButton"));
		break;
	
	case kUIStrings_PreferencesWindowDefaultFavoriteName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Default"), CFSTR("PreferencesWindow"),
													CFSTR("kUIStrings_PreferencesWindowDefaultFavoriteName"));
		break;
	
	case kUIStrings_PreferencesWindowExportCopyDefaults:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Copy from Default as needed (inheritance not preserved on import)"), CFSTR("PreferencesWindow"),
													CFSTR("kUIStrings_PreferencesWindowExportCopyDefaults"));
		break;
	
	case kUIStrings_PreferencesWindowExportCopyDefaultsHelpText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("When sharing files between users, copying Default settings is recommended (as this creates a complete snapshot)."), CFSTR("PreferencesWindow"),
													CFSTR("kUIStrings_PreferencesWindowExportCopyDefaultsHelpText"));
		break;
	
	case kUIStrings_PreferencesWindowListHeaderNumber:
		outString = CFCopyLocalizedStringFromTable(CFSTR("#"), CFSTR("PreferencesWindow"),
													CFSTR("kUIStrings_PreferencesListHeaderNumber"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (preferences window strings)


/*!
Locates the specified string used by a progress window,
and returns a reference to a Core Foundation string.
Since a copy is made, you must call CFRelease() on the
returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_ProgressWindowCFString	inWhichString,
				 CFStringRef&						outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_ProgressWindowFontListIconName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Font List Rebuilding Progress"), CFSTR("ProgressWindows"),
													CFSTR("kUIStrings_ProgressWindowFontListIconName"));
		break;
	
	case kUIStrings_ProgressWindowPrintingPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Printing..."), CFSTR("ProgressWindows"),
													CFSTR("kUIStrings_ProgressWindowPrintingPrimaryText"));
		break;
	
	case kUIStrings_ProgressWindowScriptsMenuIconName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Scripts Menu Rebuilding Progress"), CFSTR("ProgressWindows"),
													CFSTR("kUIStrings_ProgressWindowScriptsMenuIconName"));
		break;
	
	case kUIStrings_ProgressWindowScriptsMenuPrimaryText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Searching for Scripts menu items..."), CFSTR("ProgressWindows"),
													CFSTR("kUIStrings_ProgressWindowScriptsMenuPrimaryText"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (progress window strings)


/*!
Locates the specified string used by the “session info”
window, and returns a reference to a Core Foundation
string.  Since a copy is made, you must call CFRelease()
on the returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_SessionInfoWindowCFString	inWhichString,
				 CFStringRef&							outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_SessionInfoWindowIconName:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Session Info"), CFSTR("SessionInfoWindow"),
													CFSTR("kUIStrings_SessionInfoWindowIconName"));
		break;
	
	case kUIStrings_SessionInfoWindowStatusProcessNewborn:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Running (just opened)"), CFSTR("SessionInfoWindow"),
													CFSTR("kUIStrings_SessionInfoWindowStatusProcessNewborn"));
		break;
	
	case kUIStrings_SessionInfoWindowStatusProcessRunning:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Running"), CFSTR("SessionInfoWindow"),
													CFSTR("kUIStrings_SessionInfoWindowStatusProcessRunning"));
		break;
	
	case kUIStrings_SessionInfoWindowStatusProcessTerminated:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Not Running"), CFSTR("SessionInfoWindow"),
													CFSTR("kUIStrings_SessionInfoWindowStatusProcessTerminated"));
		break;
	
	case kUIStrings_SessionInfoWindowStatusTerminatedAtTime:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Not Running, Since %1$@"), CFSTR("SessionInfoWindow"),
													CFSTR("kUIStrings_SessionInfoWindowStatusProcessTerminated; %1$@ is the time of termination"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (session info window strings)


/*!
Locates the specified string used for a system-provided
UI element (like a file chooser or color picker), and
returns a reference to a Core Foundation string.  Since
a copy is made, you must call CFRelease() on the returned
string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_SystemDialogCFString		inWhichString,
				 CFStringRef&						outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_SystemDialogPromptCaptureToFile:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Enter a name for the file to contain captured text."),
													CFSTR("SystemDialogs"),
													CFSTR("kUIStrings_SystemDialogPromptCaptureToFile"));
		break;
	
	case kUIStrings_SystemDialogPromptOpenPrefs:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Choose one or more settings files to open."),
													CFSTR("SystemDialogs"),
													CFSTR("kUIStrings_SystemDialogPromptOpenPrefs"));
		break;
	
	case kUIStrings_SystemDialogPromptOpenSession:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Choose one or more session files to open."),
													CFSTR("SystemDialogs"),
													CFSTR("kUIStrings_SystemDialogPromptOpenSession"));
		break;
	
	case kUIStrings_SystemDialogPromptSavePrefs:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Enter a name for the file to contain your settings."),
													CFSTR("SystemDialogs"),
													CFSTR("kUIStrings_SystemDialogPromptSavePrefs"));
		break;
	
	case kUIStrings_SystemDialogPromptSaveSelectedImage:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Enter a name for the file to contain the selected image."),
													CFSTR("SystemDialogs"),
													CFSTR("kUIStrings_SystemDialogPromptSaveSelectedImage"));
		break;
	
	case kUIStrings_SystemDialogPromptSaveSelectedText:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Enter a name for the file to contain the selected text."),
													CFSTR("SystemDialogs"),
													CFSTR("kUIStrings_SystemDialogPromptSaveSelectedText"));
		break;
	
	case kUIStrings_SystemDialogPromptSaveSession:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Enter a name for the file to contain your session."),
													CFSTR("SystemDialogs"),
													CFSTR("kUIStrings_SystemDialogPromptSaveSession"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (system dialog strings)


/*!
Locates the specified string used by a terminal window,
and returns a reference to a Core Foundation string.
Since a copy is made, you must call CFRelease() on the
returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_TerminalCFString		inWhichString,
				 CFStringRef&					outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_TerminalAccessibilityDescription:
		outString = CFCopyLocalizedStringFromTable(CFSTR("terminal screen"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalAccessibilityDescription; must be all-lowercase without punctuation"));
		break;
	
	case kUIStrings_TerminalInterruptProcess:
		outString = CFCopyLocalizedStringFromTable(CFSTR("[Interrupted]"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalInterruptProcess"));
		break;
	
	case kUIStrings_TerminalNewCommandsKeyCharacter:
		outString = CFCopyLocalizedStringFromTable(CFSTR("n"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalNewCommandsKeyCharacter; used for some menu command keys, this should be only one lowercase Unicode character"));
		break;
	
	case kUIStrings_TerminalPrintFromTerminalJobTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Print From Terminal: MacTerm"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalPrintFromTerminalJobTitle"));
		break;
	
	case kUIStrings_TerminalPrintScreenJobTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Print Screen: MacTerm"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalPrintScreenJobTitle"));
		break;
	
	case kUIStrings_TerminalPrintSelectionJobTitle:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Print Selection: MacTerm"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalPrintSelectionJobTitle"));
		break;
	
	case kUIStrings_TerminalResumeOutput:
		outString = CFCopyLocalizedStringFromTable(CFSTR("[Resumed]"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalResumeOutput"));
		break;
	
	case kUIStrings_TerminalSuspendOutput:
		outString = CFCopyLocalizedStringFromTable(CFSTR("[Suspended]"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalSuspendOutput"));
		break;
	
	case kUIStrings_TerminalSearchNothingFound:
		outString = CFCopyLocalizedStringFromTable(CFSTR("The text was not found."), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalSearchNothingFound"));
		break;
	
	case kUIStrings_TerminalSearchNumberOfMatches:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Matches: %1$u"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalSearchNumberOfMatches; %1$u will be the number of times the query found a match"));
		break;
	
	case kUIStrings_TerminalVectorGraphicsRedirect:
		outString = CFCopyLocalizedStringFromTable(CFSTR("[Output in Canvas Window]"), CFSTR("Terminal"),
													CFSTR("kUIStrings_TerminalVectorGraphicsRedirect"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (terminal strings)


/*!
Locates the specified string used by a toolbar item
label, and returns a reference to a Core Foundation
string.  Since a copy is made, you must call CFRelease()
on the returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_ToolbarItemCFString	inWhichString,
				 CFStringRef&					outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_ToolbarItemArrangeAllInFront:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Arrange All Windows in Front"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemArrangeAllInFront"));
		break;
	
	case kUIStrings_ToolbarItemBell:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Bell"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemBell"));
		break;
	
	case kUIStrings_ToolbarItemCustomizeToolbar:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Customize"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemCustomizeToolbar"));
		break;
	
	case kUIStrings_ToolbarItemFullScreen:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Full Screen"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarFullScreen"));
		break;
	
	case kUIStrings_ToolbarItemHideFrontWindow:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Hide"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemHideFrontWindow"));
		break;
	
	case kUIStrings_ToolbarItemKillSession:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Force Quit"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemKillSession"));
		break;
	
	case kUIStrings_ToolbarItemNewSessionDefault:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Default"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemNewSessionDefault"));
		break;
	
	case kUIStrings_ToolbarItemNewSessionLoginShell:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Log-In Shell"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemNewSessionLoginShell"));
		break;
	
	case kUIStrings_ToolbarItemNewSessionShell:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Shell"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemNewSessionShell"));
		break;
	
	case kUIStrings_ToolbarItemPrint:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Print"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemPrint"));
		break;
	
	case kUIStrings_ToolbarItemRestartSession:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Restart"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemRestartSession"));
		break;
	
	case kUIStrings_ToolbarItemSearch:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Search"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemSearch"));
		break;
	
	case kUIStrings_ToolbarItemSuspendNetwork:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Suspend (Scroll Lock)"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemSuspendNetwork"));
		break;
	
	case kUIStrings_ToolbarItemTerminalLED1:
		outString = CFCopyLocalizedStringFromTable(CFSTR("L1"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemTerminalLED1"));
		break;
	
	case kUIStrings_ToolbarItemTerminalLED2:
		outString = CFCopyLocalizedStringFromTable(CFSTR("L2"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemTerminalLED2"));
		break;
	
	case kUIStrings_ToolbarItemTerminalLED3:
		outString = CFCopyLocalizedStringFromTable(CFSTR("L3"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemTerminalLED3"));
		break;
	
	case kUIStrings_ToolbarItemTerminalLED4:
		outString = CFCopyLocalizedStringFromTable(CFSTR("L4"), CFSTR("ToolbarItems"),
													CFSTR("kUIStrings_ToolbarItemTerminalLED4"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (toolbar item strings)


/*!
Locates the specified string used for an Undo or Redo
command, and returns a reference to a Core Foundation string.
Since a copy is made, you must call CFRelease() on the
returned string when you are finished with it.

\retval kUIStrings_ResultOK
if the string is copied successfully

\retval kUIStrings_ResultNoSuchString
if the given tag is invalid

\retval kUIStrings_ResultCannotGetString
if an OS error occurred

(3.0)
*/
UIStrings_Result
UIStrings_Copy	(UIStrings_UndoCFString		inWhichString,
				 CFStringRef&				outString)
{
	UIStrings_Result	result = kUIStrings_ResultOK;
	
	
	// IMPORTANT: The external utility program "genstrings" is not smart enough to
	//            figure out the proper string table name if you do not inline it.
	//            If you replace the CFSTR() calls with string constants, they will
	//            NOT BE PARSED CORRECTLY and consequently you won’t be able to
	//            automatically generate localizable ".strings" files.
	switch (inWhichString)
	{
	case kUIStrings_UndoActionNameDimensionChanges:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Dimension Changes"),
													CFSTR("Undo"), CFSTR("kUIStrings_UndoActionNameDimensionChanges"));
		break;
	
	case kUIStrings_UndoActionNameFormatChanges:
		outString = CFCopyLocalizedStringFromTable(CFSTR("Format Changes"),
													CFSTR("Undo"), CFSTR("kUIStrings_UndoActionNameFormatChanges"));
		break;
	
	default:
		// ???
		result = kUIStrings_ResultNoSuchString;
		break;
	}
	return result;
}// Copy (undo/redo strings)

// BELOW IS REQUIRED NEWLINE TO END FILE
