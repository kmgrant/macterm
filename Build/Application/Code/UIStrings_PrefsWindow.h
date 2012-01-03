/*!	\file UIStrings_PrefsWindow.h
	\brief Declarations of string identifiers for the
	UIStrings module, specifically for the Preferences
	window.
	
	Since UIStrings is included by many source files,
	this separate header attempts to minimize the chance
	many code files will appear to “need” recompiling
	after a change to a window-specific constant.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#include <UniversalDefines.h>

#ifndef __UISTRINGS_PREFSWINDOW__
#define __UISTRINGS_PREFSWINDOW__

// Mac includes
#include <CoreServices/CoreServices.h>

// application includes
#include "Preferences.h"



#pragma mark Constants

/*!
Preferences Window String Table ("PreferencesWindow.strings")

Identifies localizable strings used in preferences panels.
*/
enum UIStrings_PreferencesWindowCFString
{
	kUIStrings_PreferencesWindowAddToFavoritesButton			= 'AFBt',
	kUIStrings_PreferencesWindowCollectionsDrawerDescription	= 'CDDs',
	kUIStrings_PreferencesWindowCollectionsDrawerShowHideName	= 'SHCD',
	kUIStrings_PreferencesWindowDefaultFavoriteName				= 'DefF',
	kUIStrings_PreferencesWindowFavoritesDuplicateButtonName	= 'DupF',
	kUIStrings_PreferencesWindowFavoritesDuplicateNameTemplate	= 'DupN',
	kUIStrings_PreferencesWindowFavoritesEditButtonName			= 'Edit',
	kUIStrings_PreferencesWindowFavoritesNewButtonName			= 'NewF',
	kUIStrings_PreferencesWindowFavoritesNewGroupButtonName		= 'NewG',
	kUIStrings_PreferencesWindowFavoritesRemoveButtonName		= 'DelF',
	kUIStrings_PreferencesWindowFavoritesCannotRemoveDefault	= 'DelD',
	kUIStrings_PreferencesWindowFavoritesRemoveWarning			= 'DelW',
	kUIStrings_PreferencesWindowFavoritesRemoveWarningHelpText	= 'DelH',
	kUIStrings_PreferencesWindowFormatsCategoryName				= 'ForT',
	kUIStrings_PreferencesWindowFormatsNormalTabName			= 'ForN',
	kUIStrings_PreferencesWindowFormatsANSIColorsTabName		= 'ForA',
	kUIStrings_PreferencesWindowGeneralCategoryName				= 'GenT',
	kUIStrings_PreferencesWindowGeneralNotificationTabName		= 'GenN',
	kUIStrings_PreferencesWindowGeneralOptionsTabName			= 'GenO',
	kUIStrings_PreferencesWindowGeneralSpecialTabName			= 'GenS',
	kUIStrings_PreferencesWindowIconName						= 'Icon',
	kUIStrings_PreferencesWindowKioskCategoryName				= 'KioT',
	kUIStrings_PreferencesWindowListHeaderNumber				= 'Numb',
	kUIStrings_PreferencesWindowMacrosCategoryName				= 'McrT',
	kUIStrings_PreferencesWindowMacrosListHeaderName			= 'McNm',
	kUIStrings_PreferencesWindowScriptsCategoryName				= 'ScrT',
	kUIStrings_PreferencesWindowScriptsListHeaderEvent			= 'ScEv',
	kUIStrings_PreferencesWindowScriptsListHeaderScriptToRun	= 'ScSc',
	kUIStrings_PreferencesWindowSessionsCategoryName			= 'SsnT',
	kUIStrings_PreferencesWindowSessionsDataFlowTabName			= 'SsnD',
	kUIStrings_PreferencesWindowSessionsGraphicsTabName			= 'SsnV',
	kUIStrings_PreferencesWindowSessionsKeyboardTabName			= 'SsnC',
	kUIStrings_PreferencesWindowSessionsResourceTabName			= 'SsnH',
	kUIStrings_PreferencesWindowTerminalsCategoryName			= 'TrmT',
	kUIStrings_PreferencesWindowTerminalsEmulationTabName		= 'TrmE',
	// For simplicity, code in Preferences user interfaces assumes that tags can be
	// used instead of the “proper” enumeration names, for these “named preferences”.
	kUIStrings_PreferencesWindowTerminalsVT100FixLineWrapBug	= kPreferences_TagVT100FixLineWrappingBug,
	kUIStrings_PreferencesWindowTerminalsXTermBackColorErase	= kPreferences_TagXTermBackgroundColorEraseEnabled,
	kUIStrings_PreferencesWindowTerminalsXTerm256ColorsEnabled	= kPreferences_TagXTerm256ColorsEnabled,
	kUIStrings_PreferencesWindowTerminalsXTermColorEnabled		= kPreferences_TagXTermColorEnabled,
	kUIStrings_PreferencesWindowTerminalsXTermGraphicsEnabled	= kPreferences_TagXTermGraphicsEnabled,
	kUIStrings_PreferencesWindowTerminalsXTermWindowAltEnabled	= kPreferences_TagXTermWindowAlterationEnabled,
	kUIStrings_PreferencesWindowTerminalsListHeaderTweakName	= 'TrmH',
	kUIStrings_PreferencesWindowTerminalsOptionsTabName			= 'TrmO',
	kUIStrings_PreferencesWindowTerminalsScreenTabName			= 'TrmS',
	kUIStrings_PreferencesWindowTranslationsCategoryName		= 'TrnT',
	kUIStrings_PreferencesWindowTranslationsListHeaderBaseTable	= 'TrnB',
	kUIStrings_PreferencesWindowWorkspacesCategoryName			= 'WspT',
	kUIStrings_PreferencesWindowWindowsListHeaderName			= 'WnNm'
};



#pragma mark Public Methods

//!\name Retrieving Strings
//@{

UIStrings_Result
	UIStrings_Copy			(UIStrings_PreferencesWindowCFString	inWhichString,
							 CFStringRef&							outString);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
