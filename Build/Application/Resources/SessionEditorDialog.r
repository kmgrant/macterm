/*
**************************************************************************
	TelnetSessionEditorDialog.r
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file defines the Session Favorite Editor dialog box.
**************************************************************************
*/
#ifdef REZ

#include "Carbon.r"

#include "ControlResources.h"
#include "DialogResources.h"
#include "MenuResources.h"

#endif

#include "SpacingConstants.r"
#include "TelnetButtonNames.h"
#include "TelnetMiscellaneousText.h"


/*
*******************************************
	"SESSION FAVORITE EDITOR" DIALOG BOX
*******************************************
*/

#ifdef REZ

// INCOMPLETE DEFINITION - CHANGING COMPLETELY IN THE NEAR FUTURE

resource 'DLOG' (kDialogIDSessionFavoriteEditor, "Session Favorite Editor", purgeable)
{
	{ 60, 55, 470, 426 },
	//{ 68, 69, 457, 611 },
	kWindowMovableModalDialogProc,
	invisible,
	noGoAway,
	0,
	kDialogIDSessionFavoriteEditor,
	SESSIONEDITOR_WINDOWNAME,
	alertPositionMainScreen
};

resource 'dlgx' (kDialogIDSessionFavoriteEditor)
{
	versionZero
	{
		kDialogFlagsUseThemeBackground |
		kDialogFlagsUseControlHierarchy |
		kDialogFlagsHandleMovableModal |
		kDialogFlagsUseThemeControls
	}
};

resource 'DITL' (kDialogIDSessionFavoriteEditor, "Session Favorite Editor", purgeable)
{
	{
		// 1
		{ 0, 0/* auto-sized and auto-positioned button */, -1, -1 },
		Control { enabled, kIDOKButton },
		
		// 2
		{ 0, 0/* auto-sized and auto-positioned button */, -1, -1 },
		Control { enabled, kIDCancelButton },
		
		// 3
		{ 63, 86, 79, 161 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_INHIBIT },
		
		// 4
		{ 63, 162, 79, 237 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_4014 },
		
		// 5
		{ 63, 238, 79, 313 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_4015 },
		
		// 6
		{89, 86, 105, 161},
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_QUICK },
		
		// 7
		{ 89, 162, 105, 254 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_INBLOCKS },
		
		// 8
		{ 114, 86, 130, 161 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_DELETE },
		
		// 9
		{ 114, 162, 130, 254 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_BACKSPACE },
		
		// 10
		{164, 10, 180, 240},
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_FORCESSAVE },
		
		// 11
		{ 181, 10, 197, 240 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_BERKELEYCR },
		
		// 12
		{ 198, 10, 214, 240 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_LINEMODE },
		
		// 13
		{ 215, 10, 231, 240 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_PAGECLEAR },
		
		// 14
		{ 249, 10, 265, 150 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_HALFDUPLEX },
		
		// 15
		{ 266, 10, 282, 150 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_AUTHENTICATE },
		
		// 16
		{ 283, 10, 299, 150 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_ENCRYPT },
		
		// 17
		{ 232, 10, 248, 150 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_LOCALECHO },
		
		// 18
		{ 8, 75, 22, 153 },
		EditText { enabled, "" },
		
		// 19
		{ 34, 75, 48, 246 },
		EditText { enabled, "" },
		
		// 20
		{ 34, 314, 48, 358 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_SESSION_50000 },
		
		// 21
		{ 91, 262, 105, 299 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_SESSION_200 },
		
		// 22
		{ 141, 335, 155, 358 },
		EditText { enabled, "" },
		
		// 23
		{ 168, 335, 182, 358 },
		EditText { enabled, "" },
		
		// 24
		{ 195, 335, 209, 358 },
		EditText { enabled, "" },
		
		// 25
		{ 4, 248, 25, 358 },
		Control { enabled, kIDSessionEditorTerminalsPopUpMenu },
		
		// 26
		{ 137, 112, 158, 312 },
		Control { enabled, kIDSessionEditorTranslationTablePopUpMenu },
		
		// 27
		{ 34, 6, 50, 63 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_HOSTNAME },
		
		// 28
		{ 8, 6, 24, 63 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_ALIAS },
		
		// 29
		{ 34, 276, 50, 306 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_PORT },
		
		// 30
		{ 65, 6, 81, 79 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_TEKGRAPHICS },
		
		// 31
		{ 91, 6, 107, 79 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_PASTEMETHOD },
		
		// 32
		{ 116, 6, 132, 79 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_DELETESENDS },
		
		// 33
		{ 141, 272, 157, 327 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_INTERRUPT },
		
		// 34
		{ 168, 272, 184, 327 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_SUSPEND },
		
		// 35
		{ 195, 272, 211, 327 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_RESUME },
		
		// 36
		{ 8, 190, 24, 242 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_TERMINAL },
		
		// 37
		{ 141, 6, 155, 108 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_CHARSET },
		
		// 38
		{ 251, 214, 267, 310 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_SESSION_NETBLOCK },
		
		// 39
		{ 251, 318, 265, 358 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_SESSION_EDITTEXT },
		
		// 40
		{ 338, 91, 354, 360 },
		StaticText { disabled, "" },
		
		// 41
		{ 334, 27, 354, 85 },
		Button { enabled, PREFERENCES_SESSION_BTNNAME_SET },
		
		// 42
		{ 312, 10, 328, 248 },
		CheckBox { enabled, PREFERENCES_SESSION_CBNAME_AUTOCAPTURE },
		
		// 43
		{ 378, 10, 400, 32 },
		Control { enabled, kIDAppleGuideButton }
	}
};

resource 'dftb' (kDialogIDSessionFavoriteEditor, "Session Preferences (old): Font Auxiliary Info")
{
	versionZero
	{
		{
			skipItem { }, // 1
			skipItem { }, // 2
			dataItem // 3
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 4
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 5
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 6
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 7
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 8
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 9
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 10
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 11
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 12
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 13
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 14
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 15
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 16
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 17
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 18
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 19
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 20
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 21
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 22
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 23
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 24
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 25
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 26
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 27
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 28
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 29
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 30
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 31
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 32
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 33
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 34
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 35
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 36
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 37
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 38
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 39
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 40
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 41
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask | kDialogFontUseFaceMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0x01/* bold */, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			dataItem // 42
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kControlFontSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			}
		}
	};
};

/* pop-up menu controls */

resource 'CNTL' (kIDSessionEditorTerminalsPopUpMenu, "Session Editor: "SESSIONEDITOR_POPUPNAME_DEFAULTTERMINAL" Pop-up Menu", purgeable)
{
	{ 0, 0, POPUPMENUBUTTON_HT, 90 },
	popupTitleLeftJust, // see page 5-119 of IM:MTE for valid values
	visible,
	0, // title width in pixels
	-12345, // menu resource ID (-12345 = use a handle, not a resource)
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,
	0,
	SESSIONEDITOR_POPUPNAME_DEFAULTTERMINAL
};

resource 'CNTL' (kIDSessionEditorTranslationTablePopUpMenu, "Session Editor: "SESSIONEDITOR_POPUPNAME_TRANSLATIONTABLE" Pop-up Menu", purgeable)
{
	{ 0, 0, POPUPMENUBUTTON_HT, 155 },
	popupTitleLeftJust, // see page 5-119 of IM:MTE for valid values
	visible,
	0, // title width in pixels
	-12345, // menu resource ID (-12345 = use a handle, not a resource)
	kControlPopupButtonProc,
	0,
	SESSIONEDITOR_POPUPNAME_TRANSLATIONTABLE
};

#endif
