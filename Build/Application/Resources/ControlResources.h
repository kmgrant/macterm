/*!	\file ControlResources.h
	\brief Describes 'CNTL' resources.
	
	IMPORTANT:	The constants in this file are cumulative between
				sections, as defined by the major bases at the
				top of the file.
				
				The base for the “next” dialog box is always set
				as one greater than the last control ID for the
				previous one listed in its section.  (Upon
				careful examination of this file, the pattern
				should be obvious.)  If you add any controls to
				a group, make sure you not only add one to the
				offset of the previous control in the group, but
				that you also change the offset for the base of
				the following group.
*/
/*###############################################################

	MacTelnet
		© 1998-2005 by Kevin Grant.
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

#ifndef __CONTROLRESOURCES__
#define __CONTROLRESOURCES__



// defines resource ID range for 'CNTL' resources
#define	DIALOG_CONTROL_BASE_ID			200										/* controls used in dialog windows */
#define	WINDOW_CONTROL_BASE_ID			(DIALOG_CONTROL_BASE_ID + 300)			/* controls used in non-dialog windows */
#define	REZ_DITL_CONTROL_BASE_ID		(WINDOW_CONTROL_BASE_ID + 100)			/* avoid changing this base if at all possible! */
#define	REUSABLE_CONTROL_BASE_ID		(REZ_DITL_CONTROL_BASE_ID + 300)		/* controls used in dozens of places */



/*####################### REUSABLE CONTROLS #######################*/

/*
controls from "InterfaceLibAppResources.r"
*/
#define kIDAppleGuideButton							12000
#define kInterfaceLibControlBaseID					32000

// controls for alerts
#define kInterfaceLibControlButtonNormal			kInterfaceLibControlBaseID
#define kInterfaceLibControlAlertIconStop			(kInterfaceLibControlBaseID + 2)
#define kInterfaceLibControlAlertIconNote			(kInterfaceLibControlBaseID + 3)
#define kInterfaceLibControlAlertIconCaution		(kInterfaceLibControlBaseID + 4)

/*
controls from "TelnetReusableControls.r"
*/
#define kIDAnchorUserPane					REUSABLE_CONTROL_BASE_ID
#define kIDCancelButton						(REUSABLE_CONTROL_BASE_ID + 2)
#define kIDColorBox							(REUSABLE_CONTROL_BASE_ID + 4)
#define kIDLittleArrows						(REUSABLE_CONTROL_BASE_ID + 11)
#define kIDOKButton							(REUSABLE_CONTROL_BASE_ID + 12)
#define kIDRadioButton						(REUSABLE_CONTROL_BASE_ID + 14)
#define kIDStaticText						(REUSABLE_CONTROL_BASE_ID + 16)
#define kIDTabUserPane						(REUSABLE_CONTROL_BASE_ID + 18)



/*####################### REZ 'DITL' DEPENDENT CONTROLS #######################*/

// These definitions are temporary.  Some of the “old” Telnet 2.6 dialogs are
// not defined by Rez, but rather by Derezed-ResEdit data.  As a result, any
// reference to modern Appearance controls requires the data to be changed in
// ResEdit (because I’m too lazy to rewrite the soon-to-die older dialogs in
// Rez input format).  A separate control ID base has been created for these
// “special” Appearance controls, because control IDs of other windows tend
// to fluctuate easily (as more controls are added to one dialog, the IDs of
// all dialogs that follow it in this file are offset), which would require
// me to open ResEdit and alter references to 'CNTL' IDs to match.  Since this
// dependency exists, do NOT change the REZ_DITL_CONTROL_BASE_ID unless you
// ALSO change the appropriate ResEdit 'DITL's, Derez them and replace the
// code in the appropriate '.r' file for Telnet; otherwise, Telnet will crash
// or display the wrong control when it opens a dialog that uses a Rez-'DITL'
// control.  Got that?  No?  In a nutshell, don’t touch anything, and this mess
// will be gone in the next few releases or so.

/*
*******************************************
	CONFIGURATION LIST DIALOG BOX
*******************************************
*/

#define CONFIGURATIONS_CONTROL_BASE_ID							REZ_DITL_CONTROL_BASE_ID

#define kIDConfigurationsList									CONFIGURATIONS_CONTROL_BASE_ID

/*
*******************************************
	COLOR BOX CONTROLS IN OLDER DIALOGS
*******************************************
*/

#define OLDCOLORBOX_CONTROL_BASE_ID								(CONFIGURATIONS_CONTROL_BASE_ID + 1)

#define kIDRezDITLColorBox										OLDCOLORBOX_CONTROL_BASE_ID

/*
*******************************************
	“SESSION FAVORITE EDITOR” DIALOG BOX
*******************************************
*/

#define SESSIONEDITOR_CONTROL_BASE_ID							(OLDCOLORBOX_CONTROL_BASE_ID + 1)

#define	kIDSessionEditorTerminalsPopUpMenu						SESSIONEDITOR_CONTROL_BASE_ID
#define	kIDSessionEditorTranslationTablePopUpMenu				(SESSIONEDITOR_CONTROL_BASE_ID + 1)

/*
*******************************************
	“TERMINAL EDITOR” DIALOG BOX
*******************************************
*/

#define TERMINALEDITOR_CONTROL_BASE_ID							(SESSIONEDITOR_CONTROL_BASE_ID + 2)

#define	kIDTerminalEditorFontPopUpMenu							TERMINALEDITOR_CONTROL_BASE_ID



/*####################### DIALOG BOX CONTROLS #######################*/

/*
*******************************************
	PREFERENCES DIALOG BOX
*******************************************
*/

#define PREFS_CONTROL_BASE_ID									DIALOG_CONTROL_BASE_ID
// “General”
#define	kIDPrefGeneralAutoCalcANSIColorsButton					(PREFS_CONTROL_BASE_ID + 18)
#define	kIDPrefGeneralBackgroundNotificationGroupBox			(PREFS_CONTROL_BASE_ID + 19)
#define	kIDPrefGeneralBackgroundNotificationBeepsCheckBox		(PREFS_CONTROL_BASE_ID + 20)
#define	kIDPrefGeneralNotificationVisualBellCheckBox			(PREFS_CONTROL_BASE_ID + 21)
#define	kIDPrefGeneralNotificationMarginBellCheckBox			(PREFS_CONTROL_BASE_ID + 22)
// “Favorites”
#define kIDPrefConfigurationsTabs								(PREFS_CONTROL_BASE_ID + 23)
//#define kIDPrefConfigurationsListBox							(PREFS_CONTROL_BASE_ID + 24)
#define kIDPrefConfigurationsNewButton							(PREFS_CONTROL_BASE_ID + 25)
#define kIDPrefConfigurationsNewGroupButton						(PREFS_CONTROL_BASE_ID + 26)
#define kIDPrefConfigurationsRemoveButton						(PREFS_CONTROL_BASE_ID + 27)
#define kIDPrefConfigurationsDuplicateButton					(PREFS_CONTROL_BASE_ID + 28)
#define kIDPrefConfigurationsEditButton							(PREFS_CONTROL_BASE_ID + 29)

/*
*******************************************
	FORMAT DIALOG BOX
*******************************************
*/

#define FORMAT_CONTROL_BASE_ID									(PREFS_CONTROL_BASE_ID + 31)

#define kIDFormatEditMenuPrimaryGroup							FORMAT_CONTROL_BASE_ID
#define kIDFormatEditMenuFontItemsPane							(FORMAT_CONTROL_BASE_ID + 1)
#define kIDFormatEditCharacterSetListBox						(FORMAT_CONTROL_BASE_ID + 2)
#define kIDFormatEditFontPopUpMenu								(FORMAT_CONTROL_BASE_ID + 3)
#define kIDFormatEditMenuFontSizeItemsPane						(FORMAT_CONTROL_BASE_ID + 4)
#define kIDFormatEditSizeLabel									(FORMAT_CONTROL_BASE_ID + 5)
#define kIDFormatEditSizeField									(FORMAT_CONTROL_BASE_ID + 6)
#define kIDFormatEditSizePopUpMenu								(FORMAT_CONTROL_BASE_ID + 7)
#define kIDFormatEditMenuForeColorItemsPane						(FORMAT_CONTROL_BASE_ID + 8)
#define kIDFormatEditForegroundColorLabel						(FORMAT_CONTROL_BASE_ID + 9)
#define kIDFormatEditMenuBackColorItemsPane						(FORMAT_CONTROL_BASE_ID + 10)
#define kIDFormatEditBackgroundColorLabel						(FORMAT_CONTROL_BASE_ID + 11)

/*
*******************************************
	“NEW SESSION FAVORITE” DIALOG BOX
*******************************************
*/

#define NEWSESSIONFAVORITE_CONTROL_BASE_ID						(FORMAT_CONTROL_BASE_ID + 1)

#define	kIDNewSessionFavoriteCreateButton						NEWSESSIONFAVORITE_CONTROL_BASE_ID
#define kIDNewSessionFavoriteNameField							(NEWSESSIONFAVORITE_CONTROL_BASE_ID + 1)

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
