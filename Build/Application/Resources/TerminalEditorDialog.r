/*
**************************************************************************
	TelnetTerminalEditorDialog.r
	
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
	"TERMINAL EDITOR" DIALOG BOX
*******************************************
*/

#define POPUPFONTMENU_WD			95

#ifdef REZ

// INCOMPLETE DEFINITION - CHANGING COMPLETELY IN THE NEAR FUTURE

resource 'DLOG' (kDialogIDTerminalEditor, "Terminal Favorite Editor", purgeable)
{
	{ 68, 69, 457, 611 },
	kWindowMovableModalDialogProc,
	invisible,
	noGoAway,
	0,
	kDialogIDTerminalEditor,
	TERMINALEDITOR_WINDOWNAME,
	alertPositionMainScreen
};

resource 'dlgx' (kDialogIDTerminalEditor)
{
	versionZero
	{
		kDialogFlagsUseThemeBackground |
		kDialogFlagsUseControlHierarchy |
		kDialogFlagsHandleMovableModal |
		kDialogFlagsUseThemeControls
	}
};

resource 'DITL' (kDialogIDTerminalEditor, "Terminal Editor", purgeable)
{
	{
		// 1
		{ 0, 0/* auto-sized and auto-positioned button */, -1, -1 },
		Control { enabled, kIDOKButton },
		
		// 2
		{ 0, 0/* auto-sized and auto-positioned button */, -1, -1 },
		Control { enabled, kIDCancelButton },
		
		// 3
		{ 12, 10, 28, 138 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_TERMINALNAME },
		
		// 4
		{ 12, 148, 28, 394 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_TERMINAL_UNTITLED },
		
		// 5
		{ 41, 17, 59, 197 },
		CheckBox { enabled, PREFERENCES_TERMINAL_CBNAME_ANSICOLORS },
		
		// 6
		{ 60, 17, 78, 197 },
		CheckBox { enabled, PREFERENCES_TERMINAL_CBNAME_XTERM },
		
		// 7
		{ 79, 17, 97, 197 },
		CheckBox { enabled, PREFERENCES_TERMINAL_CBNAME_WRAPLINES },
		
		// 8
		{ 98, 17, 116, 197 },
		CheckBox { enabled, PREFERENCES_TERMINAL_CBNAME_EMACSARROWS },
		
		// 9
		{ 41, 204, 59, 384 },
		CheckBox { enabled, PREFERENCES_TERMINAL_CBNAME_8BITS },
		
		// 10
		{ 60, 204, 78, 384 },
		CheckBox { enabled, PREFERENCES_TERMINAL_CBNAME_SAVECLEAR },
		
		// 11
		{ 79, 204, 97, 384 },
		Checkbox { enabled, PREFERENCES_TERMINAL_CBNAME_NORMKEYPAD },
		
		// 12
		{ 98, 204, 116, 384 },
		CheckBox { enabled, PREFERENCES_TERMINAL_CBNAME_PAGEKEYS },
		
		// 13
		{ 42, 390, 58, 524 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_EMACSMETA },
		
		// 14
		{ 61, 429, 77, 519 },
		RadioButton { enabled, "^ \0x11"/* kMenuCommandGlyph */},
		
		// 15
		{ 80, 429, 96, 519 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_OPTION },
		
		// 16
		{ 99, 429, 115, 519 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_NONE },
		
		// 17
		{ 134, 10, 150, 265 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_EMULATION },
		
		// 18
		{ 153, 48, 169, 148 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_VT100 },
		
		// 19
		{ 172, 48, 188, 148 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_VT220 },
		
		// 20
		{ 191, 48, 207, 148 },
		RadioButton { enabled, PREFERENCES_SESSION_RBNAME_NONE },
		
		// 21
		{ 153, 172, 169, 327 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_PERCEIVEDEMULATOR },
		
		// 22
		{ 153, 333, 169, 453 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_TERMINAL_VT100 },
		
		// 23
		{ 178, 172, 206, 532 },
		StaticText { disabled, HELPTEXT_TERMINALEDITOR_ANSWERBACK },
		
		// 24
		{ 231, 9, 247, 264 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_WINDOWAPPEARANCE },
		
		// 25
		{ 258, 48, 274, 82 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_FONT },
		
		// 26
		{ 256, 88, 277, 178 },
		Control { enabled, kIDTerminalEditorFontPopUpMenu },
		
		// 27
		{ 258, 201, 274, 231 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_TERMINAL_9 },
		
		// 28
		{ 288, 48, 304, 102 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_SCREEN },
		
		// 29
		{ 288, 113, 304, 143 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_TERMINAL_80 },
		
		// 30
		{ 288, 152, 304, 177 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_BY },
		
		// 31
		{ 288, 186, 304, 216 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_TERMINAL_24 },
		
		// 32
		{ 318, 48, 334, 123 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_SCROLLBACK },
		
		// 33
		{ 318, 132, 334, 168 },
		EditText { enabled, DIALOGTEXT_PREFERENCES_TERMINAL_200 },
		
		// 34
		{ 236, 363, 250, 516 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_NORMBOLDBLINK },
		
		// 35
		{ 258, 258, 274, 357 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_FOREGROUND },
		
		// 36
		{ 257, 363, 275, 411 },
		Control { enabled, kIDRezDITLColorBox },
		
		// 37
		{ 257, 417, 275, 465 },
		Control { enabled, kIDRezDITLColorBox },
		
		// 38
		{ 257, 471, 275, 519 },
		Control { enabled, kIDRezDITLColorBox },
		
		// 39
		{ 288, 258, 304, 357 },
		StaticText { disabled, DIALOGTEXT_PREFERENCES_TERMINAL_BACKGROUND },
		
		// 40
		{ 287, 363, 305, 411 },
		Control { enabled, kIDRezDITLColorBox },
		
		// 41
		{ 287, 417, 305, 465 },
		Control { enabled, kIDRezDITLColorBox },
		
		// 42
		{ 287, 471, 305, 519 },
		Control { enabled, kIDRezDITLColorBox },
		
		// 43
		{ 345, 10, 367, 32 },
		Control { enabled, kIDAppleGuideButton }
	}
};

resource 'dftb' (kDialogIDTerminalEditor, "Terminal Preferences (old): Font Auxiliary Info")
{
	versionZero
	{
		{
			skipItem { }, // 1
			skipItem { }, // 2
			skipItem { }, // 3
			skipItem { }, // 4
			skipItem { }, // 5
			skipItem { }, // 6
			skipItem { }, // 7
			skipItem { }, // 8
			skipItem { }, // 9
			skipItem { }, // 10
			skipItem { }, // 11
			skipItem { }, // 12
			skipItem { }, // 13
			skipItem { }, // 14
			skipItem { }, // 15
			skipItem { }, // 16
			skipItem { }, // 17
			skipItem { }, // 18
			skipItem { }, // 19
			skipItem { }, // 20
			skipItem { }, // 21
			skipItem { }, // 22
			dataItem // 23
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kThemeSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			skipItem { }, // 24
			skipItem { }, // 25
			skipItem { }, // 26
			skipItem { }, // 27
			skipItem { }, // 28
			skipItem { }, // 29
			skipItem { }, // 30
			skipItem { }, // 31
			skipItem { }, // 32
			skipItem { }, // 33
			dataItem // 34
			{
				kDialogFontUseFontMask | kDialogFontUseSizeMask, // flags determine what data isn't ignored
				kThemeSmallSystemFont, // font
				10, // font size
				0, // font style
				0, // text mode
				0, // justification
				0, 0, 0, // RGB forecolor
				0, 0, 0, // RGB backcolor
				"" // font name
			},
			skipItem { }, // 35
			skipItem { }, // 36
			skipItem { }, // 37
			skipItem { }, // 38
			skipItem { }, // 39
			skipItem { }, // 40
			skipItem { }, // 41
			skipItem { } // 42
		}
	};
};

/* pop-up menu controls */

resource 'CNTL' (kIDTerminalEditorFontPopUpMenu, "Terminal Editor: Font Pop-Up Menu", purgeable)
{
	{ 0, 0, POPUPMENUBUTTON_HT, POPUPFONTMENU_WD },
	popupTitleLeftJust, // see page 5-119 of IM:MTE for valid values
	visible,
	0, // title width in pixels
	kPopUpMenuIDFont, // menu resource ID
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,
	0,
	""
};

#endif
