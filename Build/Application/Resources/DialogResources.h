/*!	\file DialogResources.h
	\brief Constants describing resources associated with
	windows and dialogs, such as icons, dialog item lists, and
	dialog box balloons.
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

#ifndef __DIALOGRESOURCES__
#define __DIALOGRESOURCES__



// defines resource ID range for 'WIND' resources
#define WINDOW_BASE_ID				1000

// defines resource ID range for 'DLOG', 'DITL', and 'dlgx' resources
#define	DIALOG_BASE_ID				700
#define kInterfaceLibDialogBaseID   32000

// defines resource ID range for 'hdlg'/'STR#' resources THAT ARE NOT INSTALLED BY DEFAULT
#define DIALOG_BALLOONS_BASE_ID		2100

// defines resource ID range for 'STR#' resources applicable to dialog boxes
#define DIALOG_STRINGS_BASE_ID		1000

// defines resource ID range for icon suites ('icl8', 'ics4', etc.) NOTE THAT ICONS IN RANGE 128, 129, etc. ARE USED BY BUNDLES
#define	ICON_SUITE_BASE_ID			200

// defines resource ID range for icons ('ICON', 'cicn')
#define	COLOR_ICON_BASE_ID			128

/*
small icon resource IDs
*/

// 'SICN' resource for Notification Manager postings
#define kAlert_NotificationSICNResourceID		12000

/*
color icon resource IDs
*/
#define	kColorIconSecurity						COLOR_ICON_BASE_ID

/*
icon suite resource IDs
*/

#define kIconSuiteConnectionOpening				(ICON_SUITE_BASE_ID + 100)
#define kIconSuiteConnectionActive				(kIconSuiteConnectionOpening + 1)
#define kIconSuiteConnectionDead				(kIconSuiteConnectionOpening + 2)

#define kIconSuiteCursorShapeBlock				(ICON_SUITE_BASE_ID + 140)
#define kIconSuiteCursorShapeVerticalBar		(kIconSuiteCursorShapeBlock + 1)
#define kIconSuiteCursorShapeUnderline			(kIconSuiteCursorShapeBlock + 2)
#define kIconSuiteCursorShapeThickVerticalBar	(kIconSuiteCursorShapeBlock + 3)
#define kIconSuiteCursorShapeThickUnderline		(kIconSuiteCursorShapeBlock + 4)

/*
alerts
*/

#define kAlertIDMemoryLow										200

/*
windows and dialogs
*/
#define kInterfaceLibAlertDialogID				kInterfaceLibDialogBaseID
#define kInterfaceLibNonmovableAlertDialogID	(kInterfaceLibDialogBaseID + 1)
#define kInterfaceLibSheetAlertDialogID			(kInterfaceLibDialogBaseID + 2)

#define kWindowIDPreferences									(WINDOW_BASE_ID + 2)

#define kDialogIDNewSessions									(DIALOG_BASE_ID + 3)
#define kDialogIDFormat											(DIALOG_BASE_ID + 7)
#define kDialogIDScreenSize										(DIALOG_BASE_ID + 8)
#define kDialogIDNewSessionFavoriteName							(DIALOG_BASE_ID + 9)
#define kDialogIDProxyServerSetup								(DIALOG_BASE_ID + 10)
// WARNING -	due to internal DLOG references to the resource IDs,
//				these MUST remain in the "+11" and "+12" positions!
#define kDialogIDSessionFavoriteEditor							(DIALOG_BASE_ID + 11)
#define kDialogIDTerminalEditor									(DIALOG_BASE_ID + 12)
//
#define kDialogIDPasswordEntry									(DIALOG_BASE_ID + 14)
#define kDialogIDKioskSetup										(DIALOG_BASE_ID + 18)

/*
window and dialog balloons
*/
#define kDialogBalloonsIDFormat									(DIALOG_BALLOONS_BASE_ID + 2)
#define kDialogBalloonsIDKioskSetup								(DIALOG_BALLOONS_BASE_ID + 3)
#define kDialogBalloonsIDNewSessionFavoriteName					(DIALOG_BALLOONS_BASE_ID + 5)
#define kDialogBalloonsIDPasswordEntry							(DIALOG_BALLOONS_BASE_ID + 7)
#define kDialogBalloonsIDScreenSize								(DIALOG_BALLOONS_BASE_ID + 10)
#define kDialogBalloonsIDSessionFavoriteEditor					(DIALOG_BALLOONS_BASE_ID + 11)
#define kDialogBalloonsIDTerminalEditor							(DIALOG_BALLOONS_BASE_ID + 13)

/*
window and dialog strings
*/
#define rStringsDialogFormat									(DIALOG_STRINGS_BASE_ID + 2)
#define rStringsDialogKioskSetup								(DIALOG_STRINGS_BASE_ID + 3)
#define rStringsDialogNewSessionFavoriteName					(DIALOG_STRINGS_BASE_ID + 5)
//#define rStringsDialogNewSessions								(DIALOG_STRINGS_BASE_ID + 6)
#define rStringsDialogPasswordEntry								(DIALOG_STRINGS_BASE_ID + 7)
#define rStringsDialogScreenSize								(DIALOG_STRINGS_BASE_ID + 10)
#define rStringsDialogSessionFavoriteEditor						(DIALOG_STRINGS_BASE_ID + 11)
#define rStringsDialogTerminalEditor							(DIALOG_STRINGS_BASE_ID + 13)


/*	#defines for the DLOG and DITL resources */

#define	DLOGOk		1
#define DLOGCancel	2

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
