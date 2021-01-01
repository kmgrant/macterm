/*!	\file AppResources.h
	\brief Easy access to resources located in application
	resource files.
	
	This can be used for managing files that are probably
	located in the application bundle somewhere.  This
	prevents other code modules from having to know the
	names or locations of files, etc.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSRunningApplication;
#else
class NSRunningApplication;
#endif



#pragma mark Public Methods

//!\name Retrieving Application Resources
//@{

void
	AppResources_Init													(CFBundleRef					inApplicationBundle);

NSRunningApplication*
	AppResources_LaunchBugReporter										(CFDictionaryRef,
																		 CFErrorRef*);

NSRunningApplication*
	AppResources_LaunchPrintPreview										(CFDictionaryRef,
																		 CFErrorRef*);

NSRunningApplication*
	AppResources_LaunchResourceApplication								(CFStringRef,
																		 CFDictionaryRef,
																		 CFErrorRef*);

CFBundleRef
	AppResources_ReturnApplicationBundle								();

CFBundleRef
	AppResources_ReturnBundleForInfo									();

CFBundleRef
	AppResources_ReturnBundleForNIBs									();

//@}

//!\name Icon Names (For Icon Services, Cocoa APIs or NIBs)
//@{

inline CFStringRef
	AppResources_ReturnBellOffIconFilenameNoExtension					()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForBellOff");
}

inline CFStringRef
	AppResources_ReturnBellOnIconFilenameNoExtension					()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForBellOn");
}

inline CFStringRef
	AppResources_ReturnBundleIconFilenameNoExtension					()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForBundle");
}

inline CFStringRef
	AppResources_ReturnContextMenuFilenameNoExtension					()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForContextMenu");
}

inline CFStringRef
	AppResources_ReturnCustomizeToolbarIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForCustomize");
}

inline CFStringRef
	AppResources_ReturnFullScreenIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForFullScreen");
}

inline CFStringRef
	AppResources_ReturnGlyphPatternDarkGrayFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.png"
	return CFSTR("GlyphForPatternDarkGray");
}

inline CFStringRef
	AppResources_ReturnGlyphPatternLightGrayFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.png"
	return CFSTR("GlyphForPatternLightGray");
}

inline CFStringRef
	AppResources_ReturnGlyphPatternMediumGrayFilenameNoExtension		()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.png"
	return CFSTR("GlyphForPatternMediumGray");
}

inline CFStringRef
	AppResources_ReturnHideWindowIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForHide");
}

inline CFStringRef
	AppResources_ReturnKeypadArrowDownIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadArrowDown");
}

inline CFStringRef
	AppResources_ReturnKeypadArrowLeftIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadArrowLeft");
}

inline CFStringRef
	AppResources_ReturnKeypadArrowRightIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadArrowRight");
}

inline CFStringRef
	AppResources_ReturnKeypadArrowUpIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadArrowUp");
}

inline CFStringRef
	AppResources_ReturnKeypadDeleteIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadDelete");
}

inline CFStringRef
	AppResources_ReturnKeypadEnterIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadEnter");
}

inline CFStringRef
	AppResources_ReturnKeypadFindIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadFind");
}

inline CFStringRef
	AppResources_ReturnKeypadInsertIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadInsert");
}

inline CFStringRef
	AppResources_ReturnKeypadPageDownIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadPageDown");
}

inline CFStringRef
	AppResources_ReturnKeypadPageUpIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadPageUp");
}

inline CFStringRef
	AppResources_ReturnKeypadSelectIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKeypadSelect");
}

inline CFStringRef
	AppResources_ReturnKillSessionIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForKillSession");
}

inline CFStringRef
	AppResources_ReturnLEDOffIconFilenameNoExtension					()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForLEDOff");
}

inline CFStringRef
	AppResources_ReturnLEDOnIconFilenameNoExtension						()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForLEDOn");
}

inline CFStringRef
	AppResources_ReturnMacroSetIconFilenameNoExtension					()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForMacroSet");
}

inline CFStringRef
	AppResources_ReturnNewSessionDefaultIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForNewSessionDefault");
}

inline CFStringRef
	AppResources_ReturnNewSessionLogInShellIconFilenameNoExtension		()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForNewSessionLogInShell");
}

inline CFStringRef
	AppResources_ReturnNewSessionShellIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForNewSessionShell");
}

inline CFStringRef
	AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrefPanelFormats");
}

inline CFStringRef
	AppResources_ReturnPrefPanelGeneralIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrefPanelGeneral");
}

inline CFStringRef
	AppResources_ReturnPrefPanelMacrosIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrefPanelMacros");
}

inline CFStringRef
	AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrefPanelSessions");
}

inline CFStringRef
	AppResources_ReturnPrefPanelTerminalsIconFilenameNoExtension		()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrefPanelTerminals");
}

inline CFStringRef
	AppResources_ReturnPrefPanelTranslationsIconFilenameNoExtension		()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrefPanelTranslations");
}

inline CFStringRef
	AppResources_ReturnPrefPanelWorkspacesIconFilenameNoExtension		()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrefPanelWorkspaces");
}

inline CFStringRef
	AppResources_ReturnPrintIconFilenameNoExtension						()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForPrint");
}

inline CFStringRef
	AppResources_ReturnRestartSessionIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForRestartSession");
}

inline CFStringRef
	AppResources_ReturnScrollLockOffIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForScrollLockOff");
}

inline CFStringRef
	AppResources_ReturnScrollLockOnIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForScrollLockOn");
}

inline CFStringRef
	AppResources_ReturnSessionStatusActiveIconFilenameNoExtension		()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForSessionStatusActive");
}

inline CFStringRef
	AppResources_ReturnSessionStatusDeadIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForSessionStatusDead");
}

inline CFStringRef
	AppResources_ReturnStackWindowsIconFilenameNoExtension				()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForStackWindows");
}

inline CFStringRef
	AppResources_ReturnWindowTitleCenterIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForWindowTitleCenter");
}

inline CFStringRef
	AppResources_ReturnWindowTitleLeftIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForWindowTitleLeft");
}

inline CFStringRef
	AppResources_ReturnWindowTitleRightIconFilenameNoExtension			()
{
	// value is <X> in "<bundle>.app/Resources[/<locale>.lproj]/<X>.icns"
	return CFSTR("IconForWindowTitleRight");
}

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
