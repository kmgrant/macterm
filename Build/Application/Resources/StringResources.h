/*!	\file StringResources.h
	\brief IDs and indices for strings in 'STR#' resources.
	
	On Mac OS X, this is being replaced as quickly as possible
	with localizable strings in UIStrings.cp.  In fact, strings
	listed below may already have been superseded by API calls
	in UIStrings.h (check there first).
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#ifndef __STRINGRESOURCES__
#define __STRINGRESOURCES__

//
// INTERNAL string list resource IDs and indices (will eventually be moved to a source file)
// DIRECT USE IS NOW DEPRECATED
//

enum
{
	rStringsGeneralMessages = 2000,
	siPrintingFailed = 6,
	siPrintingFailedHelpText = 7,
	siOSErrAndInternalErrHelpText = 9,
	siCommandCannotContinue = 13,
	siIPAddressNotAttainable = 14
};

enum
{
	rStringsOperationFailed = 2001,
	siOpFailedCantCreateFile = 1,
	siOpFailedCantOpenFile = 2,
	siOpFailedOutOfMemory = 3,
	siOpFailedNoPrinter = 5
};

enum
{
	rStringsMiscellaneous = 2002,
	siMacOS8_5AndBeyondFindFileName = 6,
	siPreMacOS8_5FindFileName = 7,
	siPickNewColor = 8,
	siNone = 9,
	siSessionName = 17
};

enum
{
	rStringsStatus = 2003,
	siStatusPrintingInProgress = 1,
	siStatusBuildingScriptsMenu = 3
};

enum
{
	rStringsNavigationServices = 2006,
	siNavDialogTitleExampleBinaryApplication = 1,
	siNavDialogTitleExampleTextApplication = 2,
	siNavDialogTitleDefaultDirectory = 3,
	siNavDialogTitleExportMacros = 4,
	siNavDialogTitleImportMacros = 5,
	siNavDialogTitleSaveSet = 6,
	siNavDialogTitleLoadSet = 7,
	siNavDialogTitleSaveCapturedText = 8,
	siNavDialogTitleLocateApplication = 9,
	siNavPromptSelectBinaryFile = 10,
	siNavPromptSelectTextFile = 11,
	siNavPromptSelectDefaultDirectory = 12,
	siNavPromptExportMacrosToFile = 13,
	siNavPromptImportMacrosFromFile = 14,
	siNavPromptSaveConfigurationSet = 15,
	siNavPromptLoadConfigurationSet = 16,
	siNavPromptSaveCapturedText = 17,
	siNavPromptLocateApplication = 18
};

enum
{
	rStringsNoteAlerts = 2013,
	siActivityDetected = 9,
	siNotificationAlert = 10,
	siScriptErrorHelpText = 11
};

enum
{
	rStringsCautionAlerts = 2014,
	siConnectionsAreOpenQuitAnyway = 1,
	siConnectionsAreOpenQuitAnywayHelpText = 2,
	siConfirmProcessKill = 6,
	siConfirmProcessKillHelpText = 7,
	siConfirmANSIColorsReset = 11,
	siProcedureNotUndoableHelpText = 12,
};

enum
{
	rStringsStartupErrors = 2015,
	siUnableToInstallAppleEventHandlers = 9,
	siUnableToInitOpenTransport = 10
};

enum
{
	rStringsMemoryErrors = 2021,
	siMemErrorCannotCreateRasterGraphicsWindow = 2
};

enum
{
	rStringsMiscellaneousStaticText = 2027,
	siStaticTextFormatSizeLabel = 7,
	siStaticTextFormatForegroundColor = 8,
	siStaticTextFormatBackgroundColor = 9
};

enum
{
	rStringsImportantFileNames = 2029,
	siImportantFileNameTerminalBellSound = 5,
	siImportantFileNameMacroSet1 = 9 // all of the macro set file names must be consecutive!
};

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
