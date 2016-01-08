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
		© 1998-2016 by Kevin Grant.
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
#include "UIStrings.h"



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
	kUIStrings_PreferencesWindowFavoritesRemoveWarning			= 'DelW',
	kUIStrings_PreferencesWindowFavoritesRemoveWarningHelpText	= 'DelH',
	kUIStrings_PreferencesWindowIconName						= 'Icon',
	kUIStrings_PreferencesWindowListHeaderNumber				= 'Numb'
};

/*!
Macros Preferences Panel String Table ("PrefPanelMacros.strings")

Identifies localizable strings used in the Macros preferences panel.
*/
enum UIStrings_PrefPanelMacrosCFString
{
	kUIStrings_PrefPanelMacrosCategoryName		= 'McrT',
	kUIStrings_PrefPanelMacrosListHeaderName	= 'McNm'
};

/*!
Sessions Preferences Panel String Table ("PrefPanelSessions.strings")

Identifies localizable strings used in the Sessions preferences panel.
*/
enum UIStrings_PrefPanelSessionsCFString
{
	kUIStrings_PrefPanelSessionsCategoryName		= 'SsnT',
	kUIStrings_PrefPanelSessionsDataFlowTabName		= 'SsnD',
	kUIStrings_PrefPanelSessionsGraphicsTabName		= 'SsnV',
	kUIStrings_PrefPanelSessionsKeyboardTabName		= 'SsnC',
	kUIStrings_PrefPanelSessionsResourceTabName		= 'SsnH'
};

/*!
Terminals Preferences Panel String Table ("PrefPanelTerminals.strings")

Identifies localizable strings used in the Terminals preferences panel.
*/
enum UIStrings_PrefPanelTerminalsCFString
{
	kUIStrings_PrefPanelTerminalsCategoryName			= 'TrmT',
	kUIStrings_PrefPanelTerminalsEmulationTabName		= 'TrmE',
	// For simplicity, code in Preferences user interfaces assumes that tags can be
	// used instead of the “proper” enumeration names, for these “named preferences”.
	kUIStrings_PrefPanelTerminals24BitColorEnabled		= kPreferences_TagTerminal24BitColorEnabled,
	kUIStrings_PrefPanelTerminalsVT100FixLineWrapBug	= kPreferences_TagVT100FixLineWrappingBug,
	kUIStrings_PrefPanelTerminalsXTermBackColorErase	= kPreferences_TagXTermBackgroundColorEraseEnabled,
	kUIStrings_PrefPanelTerminalsXTerm256ColorsEnabled	= kPreferences_TagXTerm256ColorsEnabled,
	kUIStrings_PrefPanelTerminalsXTermColorEnabled		= kPreferences_TagXTermColorEnabled,
	kUIStrings_PrefPanelTerminalsXTermGraphicsEnabled	= kPreferences_TagXTermGraphicsEnabled,
	kUIStrings_PrefPanelTerminalsXTermWindowAltEnabled	= kPreferences_TagXTermWindowAlterationEnabled,
	kUIStrings_PrefPanelTerminalsListHeaderTweakName	= 'TrmH',
	kUIStrings_PrefPanelTerminalsOptionsTabName			= 'TrmO',
	kUIStrings_PrefPanelTerminalsScreenTabName			= 'TrmS'
};

/*!
Translations Preferences Panel String Table ("PrefPanelTranslations.strings")

Identifies localizable strings used in the Translations preferences panel.
*/
enum UIStrings_PrefPanelTranslationsCFString
{
	kUIStrings_PrefPanelTranslationsCategoryName			= 'TrnT',
	kUIStrings_PrefPanelTranslationsListHeaderBaseTable		= 'TrnB'
};

/*!
Workspaces Preferences Panel String Table ("PrefPanelWorkspaces.strings")

Identifies localizable strings used in the Workspaces preferences panel.
*/
enum UIStrings_PrefPanelWorkspacesCFString
{
	kUIStrings_PrefPanelWorkspacesCategoryName				= 'WspT',
	kUIStrings_PrefPanelWorkspacesWindowsListHeaderName		= 'WnNm'
};



#pragma mark Public Methods

//!\name Retrieving Strings
//@{

UIStrings_Result
	UIStrings_Copy			(UIStrings_PreferencesWindowCFString		inWhichString,
							 CFStringRef&								outString);

UIStrings_Result
	UIStrings_Copy			(UIStrings_PrefPanelMacrosCFString			inWhichString,
							 CFStringRef&								outString);

UIStrings_Result
	UIStrings_Copy			(UIStrings_PrefPanelSessionsCFString		inWhichString,
							 CFStringRef&								outString);

UIStrings_Result
	UIStrings_Copy			(UIStrings_PrefPanelTerminalsCFString		inWhichString,
							 CFStringRef&								outString);

UIStrings_Result
	UIStrings_Copy			(UIStrings_PrefPanelTranslationsCFString	inWhichString,
							 CFStringRef&								outString);

UIStrings_Result
	UIStrings_Copy			(UIStrings_PrefPanelWorkspacesCFString		inWhichString,
							 CFStringRef&								outString);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
