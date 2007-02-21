/*
**************************************************************************
	LocalizedPrefPanelGeneral.h
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains localized strings for the General panel of the
	Preferences window.
	
**************************************************************************
*/

#ifndef __LOCALIZEDPREFPANELGENERAL__
#define __LOCALIZEDPREFPANELGENERAL__



#pragma mark Constants

// This is the name of the panel as shown in the Preferences window.
#define LOCALIZED_ButtonName_PrefPanelGeneral_PanelName \
	"General"

// This labels a tab of option checkboxes in the General pane.
#define LOCALIZED_TabName_PrefPanelGeneral_Options \
	"Options"

// This labels a tab of miscellaneous settings in the General pane.
#define LOCALIZED_TabName_PrefPanelGeneral_Special \
	"Special"

// This labels a tab of color boxes in the General pane.
#define LOCALIZED_TabName_PrefPanelGeneral_Colors \
	"Colors"

// This labels a tab of background notification settings in the General pane.
#define LOCALIZED_TabName_PrefPanelGeneral_Notification \
	"Notification"

#define LOCALIZED_GroupBoxName_PrefPanelGeneral_Cursor \
	"Cursor"

#define LOCALIZED_CheckboxName_PrefPanelGeneral_CursorFlashing \
	"Flashing"

#define LOCALIZED_GroupBoxName_PrefPanelGeneral_WindowStackingOrigin \
	"Window Stacking Origin (in pixels)"

#define LOCALIZED_GroupBoxName_PrefPanelGeneral_CopyUsingTabsForSpaces \
	"Copy Using Tabs For Spaces"

#define LOCALIZED_GroupBoxName_PrefPanelGeneral_CaptureFileCreator \
	"Capture File Creator"

#define LOCALIZED_ButtonName_PrefPanelGeneral_FromApplication \
	"From Application…"

#define LOCALIZED_GroupBoxName_PrefPanelGeneral_CommandNOptions \
	"N Key Equivalent"

#define LOCALIZED_RadioButtonName_PrefPanelGeneral_CommandNCreatesShellSession \
	"New Shell Session"

#define LOCALIZED_RadioButtonName_PrefPanelGeneral_CommandNCreatesSessionFromDefaultFavorite \
	"New Session From Default Favorite"

#define LOCALIZED_RadioButtonName_PrefPanelGeneral_CommandNDisplaysDialog \
	"New Session Dialog"

// This labels a button that resets all 16 ANSI colors to their default values.
#define LOCALIZED_ButtonName_PrefPanelGeneral_SetToDefaultColors \
	"Set to Default Colors…"

#define LOCALIZED_GroupBoxName_PrefPanelGeneral_BackgroundNotification \
	"Background Notification"

#define LOCALIZED_CheckboxName_PrefPanelGeneral_NotifyOfBeeps \
	"Notify of Terminal Beeps"

#define LOCALIZED_CheckboxName_PrefPanelGeneral_VisualBell \
	"Visual Bell (No Terminal Audio)"

#define LOCALIZED_CheckboxName_PrefPanelGeneral_MarginBell \
	"Margin Bell"

// Notification preferences; the user wants nothing to happen when an alert appears in the background.
#define LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodNone \
	"Do Nothing"

// Notification preferences; the user wants a modest indicator that something has happened.
#define LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodDiamond \
	"Display a special icon in the Dock"

// Notification preferences; the user wants an obvious indicator that something has happened.
#define LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodFlashIcon \
	"…and bounce Dock icon"

// Notification preferences; the user wants to be interrupted (in any application) if MacTelnet displays an alert.
#define LOCALIZED_RadioButtonName_PrefPanelGeneral_NotificationMethodDisplayAlert \
	"…and display a message"

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
