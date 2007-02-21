/*
**************************************************************************
	TelnetFormatDialog.r
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file defines the “Format Terminal” dialog box.
**************************************************************************
*/
#ifdef REZ

#include "Carbon.r"
#include "CoreServices.r"

#endif

#include "ControlResources.h"
#include "DialogResources.h"
#include "MenuResources.h"
#include "SpacingConstants.r"
#include "TelnetButtonNames.h"


/*
*******************************************
	FORMAT DIALOG BOX
*******************************************
*/

#define POPUPCHARACTERSETLIST_ITEMCOUNT 6

/* the entire dialog box area */
#define FORMAT_DIALOG_WD		340
#define	FORMAT_DIALOG_HT		(VSP_TAB_AND_DIALOG + TAB_HT_BIG + VSP_TABPANE_AND_TAB + SPACE_EXTRA_CONTAINER + \
									(18 * POPUPCHARACTERSETLIST_ITEMCOUNT) + SPACE_EXTRA_CONTAINER + \
									VSP_BUTTON_AND_CONTROL + POPUPMENUBUTTON_HT + VSP_CONTROLS + \
									LITTLEARROWS_HT + VSP_CONTROLS + \
									COLORBOX_HT + VSP_CONTROLS + COLORBOX_HT + VSP_TABPANE_AND_TAB + \
									VSP_TABS_AND_CONTROL + BUTTON_HT + VSP_BUTTON_AND_DIALOG)

#ifdef REZ

#define DIALOG_WD	FORMAT_DIALOG_WD
#define DIALOG_HT	FORMAT_DIALOG_HT

// “Edit” group box dimensions
#define POPUPEDITMENU_TITLE_WD			40
#if 0
#define EDIT_MENU_GROUP_T				VSP_GROUPBOX_AND_DIALOG
#define EDIT_MENU_GROUP_L				HSP_GROUPBOX_AND_DIALOG
#define EDIT_MENU_GROUP_B				(DIALOG_HT - VSP_BUTTON_AND_DIALOG - BUTTON_HT - VSP_BUTTON_AND_CONTROL)
#define EDIT_MENU_GROUP_R				(DIALOG_WD - HSP_GROUPBOX_AND_DIALOG)
#else
#define EDIT_MENU_GROUP_T				VSP_CONTROL_AND_DIALOG
#define EDIT_MENU_GROUP_L				HSP_TAB_AND_DIALOG
#define EDIT_MENU_GROUP_B				(DIALOG_HT - VSP_TABS_AND_CONTROL - BUTTON_HT - VSP_BUTTON_AND_DIALOG)
#define EDIT_MENU_GROUP_R				(DIALOG_WD - HSP_TAB_AND_DIALOG)
#endif

// helpful values
#define EDIT_MENU_GROUP_INTERIOR_L		(EDIT_MENU_GROUP_L + HSP_TABS_AND_CONTROL)
#define EDIT_MENU_GROUP_INTERIOR_WD		(EDIT_MENU_GROUP_R - EDIT_MENU_GROUP_L - 2 * HSP_GROUPBOX_AND_CONTROL)

// other controls’ dimensions
#define BOTTOM_CHECKBOX_T				(EDIT_MENU_GROUP_B + VSP_CONTROLS)
#define BOTTOM_CHECKBOX_L				HSP_CONTROL_AND_DIALOG
#define BOTTOM_CHECKBOX_B				(BOTTOM_CHECKBOX_T + CHECKBOX_HT)
#define BOTTOM_CHECKBOX_R				(DIALOG_WD - HSP_CONTROL_AND_DIALOG)

// font group
#define POPUPCHARACTERSETLIST_T			(EDIT_MENU_GROUP_T + TAB_HT_BIG + VSP_TABPANE_AND_TAB + SPACE_EXTRA_CONTAINER)
#define POPUPCHARACTERSETLIST_L			EDIT_MENU_GROUP_INTERIOR_L
#define POPUPCHARACTERSETLIST_WD		EDIT_MENU_GROUP_INTERIOR_WD
#define POPUPCHARACTERSETLIST_HT		(18 * POPUPCHARACTERSETLIST_ITEMCOUNT)
#define POPUPFONTMENU_T					(POPUPCHARACTERSETLIST_T + POPUPCHARACTERSETLIST_HT + SPACE_EXTRA_CONTAINER \
											+ VSP_BUTTONS)
#define POPUPFONTMENU_L					(POPUPCHARACTERSETLIST_L - 3)
#define POPUPFONTMENU_WD				POPUPCHARACTERSETLIST_WD
#define POPUPFONTMENU_TITLE_WD			60
#define FONT_ITEMS_CONTAINER_T			(POPUPCHARACTERSETLIST_T - 2)
#define FONT_ITEMS_CONTAINER_L			(POPUPCHARACTERSETLIST_L - 2)
#define FONT_ITEMS_CONTAINER_B			(FONT_ITEMS_CONTAINER_T + POPUPCHARACTERSETLIST_HT + VSP_BUTTON_AND_CONTROL + \
											POPUPMENUBUTTON_HT + 4)
#define FONT_ITEMS_CONTAINER_R			(FONT_ITEMS_CONTAINER_L + EDIT_MENU_GROUP_INTERIOR_WD + 4)

// font size group
#define FONTSIZE_LABEL_T				(POPUPFONTMENU_T + BUTTON_HT + VSP_CONTROLS + SPACE_EXTRA_TEXTEDIT)
#define FONTSIZE_LABEL_L				EDIT_MENU_GROUP_INTERIOR_L
#define FONTSIZE_LABEL_B				(FONTSIZE_LABEL_T + TEXT_HT_BIG)
#define FONTSIZE_LABEL_R				(FONTSIZE_LABEL_L + POPUPFONTMENU_TITLE_WD - SPACE_EXTRA_TEXTEDIT - 2)
#define FONTSIZE_FIELD_T				FONTSIZE_LABEL_T
#define FONTSIZE_FIELD_L				(POPUPFONTMENU_L + POPUPFONTMENU_TITLE_WD + SPACE_EXTRA_TEXTEDIT)
#define FONTSIZE_FIELD_B				(FONTSIZE_FIELD_T + TEXT_HT_BIG)
#define FONTSIZE_FIELD_R				(FONTSIZE_FIELD_L + 50)
#define FONTSIZE_ARROWS_T				(FONTSIZE_LABEL_T + (TEXT_HT_BIG - LITTLEARROWS_HT) / 2)
#define FONTSIZE_ARROWS_L				(FONTSIZE_FIELD_R + SPACE_EXTRA_TEXTEDIT + HSP_CONTROLS)
#define FONTSIZE_ARROWS_B				(FONTSIZE_ARROWS_T + LITTLEARROWS_HT)
#define FONTSIZE_ARROWS_R				(FONTSIZE_ARROWS_L + LITTLEARROWS_WD)
#define FONTSIZE_POPUPMENU_T			(FONTSIZE_LABEL_T + (TEXT_HT_BIG - BUTTON_HT) / 2)
#define FONTSIZE_POPUPMENU_L			(FONTSIZE_ARROWS_R + HSP_CONTROLS)
#define FONTSIZE_POPUPMENU_B			(FONTSIZE_POPUPMENU_T + BUTTON_HT)
#define FONTSIZE_POPUPMENU_R			(FONTSIZE_POPUPMENU_L + POPUPMENUBUTTONONLY_WD)
#define FONTSIZE_ITEMS_CONTAINER_T		(FONTSIZE_ARROWS_T - 2)
#define FONTSIZE_ITEMS_CONTAINER_L		(FONTSIZE_LABEL_L - 2)
#define FONTSIZE_ITEMS_CONTAINER_B		(FONTSIZE_FIELD_T + TEXT_HT_BIG + SPACE_EXTRA_TEXTEDIT + 4)
#define FONTSIZE_ITEMS_CONTAINER_R		(FONTSIZE_ITEMS_CONTAINER_L + EDIT_MENU_GROUP_INTERIOR_WD + 4)

// foreground color group
#define COLORBOX_FOREGROUND_T			(FONTSIZE_LABEL_B + SPACE_EXTRA_TEXTEDIT + VSP_BUTTONS)
#define COLORBOX_FOREGROUND_L			EDIT_MENU_GROUP_INTERIOR_L
#define COLORBOX_FOREGROUND_LABEL_T		(COLORBOX_FOREGROUND_T + (COLORBOX_HT - TEXT_HT_BIG) / 2)
#define COLORBOX_FOREGROUND_LABEL_L		(EDIT_MENU_GROUP_INTERIOR_L + COLORBOX_WD + HSP_CONTROLS)
#define FONTFORECOLOR_ITEMS_CONTAINER_T	(COLORBOX_FOREGROUND_T - 2)
#define FONTFORECOLOR_ITEMS_CONTAINER_L	(COLORBOX_FOREGROUND_L - 2)
#define FONTFORECOLOR_ITEMS_CONTAINER_B	(FONTFORECOLOR_ITEMS_CONTAINER_T + BUTTON_HT + 4)
#define FONTFORECOLOR_ITEMS_CONTAINER_R	(FONTFORECOLOR_ITEMS_CONTAINER_L + EDIT_MENU_GROUP_INTERIOR_WD + 4)

// background color group
#define COLORBOX_BACKGROUND_T			(COLORBOX_FOREGROUND_T + COLORBOX_HT + VSP_BUTTONS)
#define COLORBOX_BACKGROUND_L			COLORBOX_FOREGROUND_L
#define COLORBOX_BACKGROUND_LABEL_T		(COLORBOX_BACKGROUND_T + (COLORBOX_HT - TEXT_HT_BIG) / 2)
#define COLORBOX_BACKGROUND_LABEL_L		COLORBOX_FOREGROUND_LABEL_L
#define FONTBACKCOLOR_ITEMS_CONTAINER_T	(COLORBOX_BACKGROUND_T - 2)
#define FONTBACKCOLOR_ITEMS_CONTAINER_L	(COLORBOX_BACKGROUND_L - 2)
#define FONTBACKCOLOR_ITEMS_CONTAINER_B	(FONTBACKCOLOR_ITEMS_CONTAINER_T + BUTTON_HT + 4)
#define FONTBACKCOLOR_ITEMS_CONTAINER_R	(FONTBACKCOLOR_ITEMS_CONTAINER_L + EDIT_MENU_GROUP_INTERIOR_WD + 4)



resource 'DLOG' (kDialogIDFormat, FORMAT_WINDOWNAME, purgeable)
{
	{ 0, 0, DIALOG_HT, DIALOG_WD },
// see also the value of DIALOG_IS_SHEET in "FormatDialog.c"
	kWindowSheetProc,
	invisible,
	0, // close box
	0,
	kDialogIDFormat,
	FORMAT_WINDOWNAME,
	centerParentWindow
};

resource 'dlgx' (kDialogIDFormat)
{
	versionZero
	{
		kDialogFlagsUseThemeBackground		|
		kDialogFlagsUseControlHierarchy		|
		kDialogFlagsHandleMovableModal		|
		kDialogFlagsUseThemeControls
	}
};

resource 'DITL' (kDialogIDFormat, FORMAT_WINDOWNAME, purgeable)
{
	{
		// 1
		{ 0, 0/* auto-sized and auto-positioned button */, -1, -1 },
		Control { enabled, kIDOKButton },
		
		// 2
		{ 0, 0/* auto-sized and auto-positioned button */, -1, -1 },
		Control { enabled, kIDCancelButton },
		
		// 3
		{ 0, 0/* auto-sized and auto-positioned button */, -1, -1 },
		Control { enabled, kIDAppleGuideButton },
		
		// 4
		{ EDIT_MENU_GROUP_T, EDIT_MENU_GROUP_L, -1, -1 },
		Control { enabled, kIDFormatEditMenuPrimaryGroup },
		
		// 5
		{ FONT_ITEMS_CONTAINER_T, FONT_ITEMS_CONTAINER_L, -1, -1 },
		Control { disabled, kIDFormatEditMenuFontItemsPane },
		
		// 6
		{ FONTSIZE_ITEMS_CONTAINER_T, FONTSIZE_ITEMS_CONTAINER_L, -1, -1 },
		Control { disabled, kIDFormatEditMenuFontSizeItemsPane },
		
		// 7
		{ FONTSIZE_LABEL_T, FONTSIZE_LABEL_L, -1, -1 },
		Control { disabled, kIDStaticText },
		
		// 8
		{ FONTSIZE_FIELD_T, FONTSIZE_FIELD_L, -1, -1 },
		Control { disabled, kIDFormatEditSizeField },
		
		// 9
		{ FONTSIZE_ARROWS_T, FONTSIZE_ARROWS_L, -1, -1 },
		Control { enabled, kIDLittleArrows },
		
		// 10
		{ FONTSIZE_POPUPMENU_T, FONTSIZE_POPUPMENU_L, -1, -1 },
		Control { enabled, kIDFormatEditSizePopUpMenu },
		
		// 11
		{ POPUPCHARACTERSETLIST_T, POPUPCHARACTERSETLIST_L, -1, -1 },
		Control { enabled, kIDFormatEditCharacterSetListBox },
		
		// 12
		{ POPUPFONTMENU_T, POPUPFONTMENU_L, -1, -1 },
		Control { enabled, kIDFormatEditFontPopUpMenu },
		
		// 13
		{ FONTFORECOLOR_ITEMS_CONTAINER_T, FONTFORECOLOR_ITEMS_CONTAINER_L,
			FONTFORECOLOR_ITEMS_CONTAINER_B, FONTFORECOLOR_ITEMS_CONTAINER_R },
		Control { disabled, kIDFormatEditMenuForeColorItemsPane },
		
		// 14
		{ COLORBOX_FOREGROUND_T, COLORBOX_FOREGROUND_L, -1, -1 },
		Control { enabled, kIDColorBox },
		
		// 15
		{ COLORBOX_FOREGROUND_LABEL_T, COLORBOX_FOREGROUND_LABEL_L, -1, -1 },
		Control { disabled, kIDStaticText },
		
		// 16
		{ FONTBACKCOLOR_ITEMS_CONTAINER_T, FONTBACKCOLOR_ITEMS_CONTAINER_L,
			FONTBACKCOLOR_ITEMS_CONTAINER_B, FONTBACKCOLOR_ITEMS_CONTAINER_R },
		Control { disabled, kIDFormatEditMenuBackColorItemsPane },
		
		// 17
		{ COLORBOX_BACKGROUND_T, COLORBOX_BACKGROUND_L, -1, -1 },
		Control { enabled, kIDColorBox },
		
		// 18
		{ COLORBOX_BACKGROUND_LABEL_T, COLORBOX_BACKGROUND_LABEL_L, -1, -1 },
		Control { disabled, kIDStaticText }
	}
};

resource 'dftb' (kDialogIDFormat, FORMAT_WINDOWNAME": Font Auxiliary Info")
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
			skipItem { } // 18
		}
	};
};

/* primary group boxes */

#if 0
resource 'CNTL' (kIDFormatEditMenuPrimaryGroup, FORMAT_WINDOWNAME": “Edit” Pop-Up Menu Group Box", purgeable)
{
	{ 0, 0, EDIT_MENU_GROUP_B - EDIT_MENU_GROUP_T, EDIT_MENU_GROUP_R - EDIT_MENU_GROUP_L },
	popupTitleLeftJust, // see page 5-119 of IM:MTE for valid values
	visible,
	POPUPEDITMENU_TITLE_WD, // title width in pixels
	kPopUpMenuIDFormatType, // menu resource ID
	kControlGroupBoxPopupButtonProc | kControlPopupVariableWidthVariant,
	0,
	FORMAT_POPUPNAME_EDIT
};
#else
resource kControlTabListResType (kIDFormatEditMenuPrimaryGroup, FORMAT_WINDOWNAME": Tab Auxiliary Info")
{
	versionZero
	{
		{
			0, "Normal"; 0, "Bold"; 0, "Blinking"
			//0, INAME_FORMATNORMALTEXT;
			//0, INAME_FORMATBOLDTEXT;
			//0, INAME_FORMATBLINKINGTEXT
		}
	};
};
resource 'CNTL' (kIDFormatEditMenuPrimaryGroup, FORMAT_WINDOWNAME": Tab Control", purgeable)
{
	{ 0, 0, EDIT_MENU_GROUP_B - EDIT_MENU_GROUP_T, EDIT_MENU_GROUP_R - EDIT_MENU_GROUP_L },
	kIDFormatEditMenuPrimaryGroup, // resource ID of "kControlTabListResType" resource
	visible,
	0,
	0,
	kControlTabLargeNorthProc,
	0,
	""
};
#endif

/* “Edit” group box controls */

// this control is used to embed all font-related controls together, so one DeactivateControl() call can be used
resource 'CNTL' (kIDFormatEditMenuFontItemsPane, FORMAT_WINDOWNAME": Container For Font Controls", purgeable)
{
	{ 0, 0, FONT_ITEMS_CONTAINER_B - FONT_ITEMS_CONTAINER_T, FONT_ITEMS_CONTAINER_R - FONT_ITEMS_CONTAINER_L },
	kControlSupportsEmbedding, // control feature bits
	visible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};

// this control is used to embed all font size-related controls together, so one DeactivateControl() call can be used
resource 'CNTL' (kIDFormatEditMenuFontSizeItemsPane, FORMAT_WINDOWNAME": Container For Font Size Controls", purgeable)
{
	{ 0, 0, FONTSIZE_ITEMS_CONTAINER_B - FONTSIZE_ITEMS_CONTAINER_T,
		FONTSIZE_ITEMS_CONTAINER_R - FONTSIZE_ITEMS_CONTAINER_L },
	kControlSupportsEmbedding, // control feature bits
	visible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};

// this control is used to embed all foreground color-related controls together, so one DeactivateControl() call can be used
resource 'CNTL' (kIDFormatEditMenuForeColorItemsPane, FORMAT_WINDOWNAME": Container For Foreground Color Controls", purgeable)
{
	{ 0, 0, FONTFORECOLOR_ITEMS_CONTAINER_B - FONTFORECOLOR_ITEMS_CONTAINER_T,
		FONTFORECOLOR_ITEMS_CONTAINER_R - FONTFORECOLOR_ITEMS_CONTAINER_L },
	kControlSupportsEmbedding, // control feature bits
	visible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};

// this control is used to embed all background color-related controls together, so one DeactivateControl() call can be used
resource 'CNTL' (kIDFormatEditMenuBackColorItemsPane, FORMAT_WINDOWNAME": Container For Background Color Controls", purgeable)
{
	{ 0, 0, FONTBACKCOLOR_ITEMS_CONTAINER_B - FONTBACKCOLOR_ITEMS_CONTAINER_T,
		FONTBACKCOLOR_ITEMS_CONTAINER_R - FONTBACKCOLOR_ITEMS_CONTAINER_L },
	kControlSupportsEmbedding, // control feature bits
	visible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};

resource kControlListDescResType (kIDFormatEditCharacterSetListBox, purgeable)
{
	versionZero
	{
		0,				// rows
		1,				// columns
		18,				// cell height
		0,				// cell width - set programmatically
		hasVertScroll,
		noHorizScroll,
		0,				// LDEF
		noGrowSpace
	}
};

resource 'CNTL' (kIDFormatEditCharacterSetListBox, FORMAT_WINDOWNAME": Configurations List Box", purgeable)
{
	{ 0, 0, POPUPCHARACTERSETLIST_HT, POPUPCHARACTERSETLIST_WD },
	kIDFormatEditCharacterSetListBox,	// resource ID of "kControlListDescResType" resource
	visible,
	0,
	0,
	kControlListBoxProc,
	0,
	""
};

resource 'CNTL' (kIDFormatEditFontPopUpMenu, FORMAT_WINDOWNAME": Font Pop-Up Menu", purgeable)
{
	{ 0, 0, BUTTON_HT, POPUPFONTMENU_WD },
	popupTitleLeftJust, // see page 5-119 of IM:MTE for valid values
	visible,
	POPUPFONTMENU_TITLE_WD, // title width in pixels
	kPopUpMenuIDFont, // menu resource ID
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,
	0,
	FORMAT_POPUPNAME_FONT
};

resource 'CNTL' (kIDFormatEditSizeLabel, FORMAT_WINDOWNAME": Size Label", purgeable)
{
	{ 0, 0, FONTSIZE_LABEL_B - FONTSIZE_LABEL_T, FONTSIZE_LABEL_R - FONTSIZE_LABEL_L },
	0,
	visible,
	0,
	0,
	kControlStaticTextProc,
	0,
	""
};

resource 'CNTL' (kIDFormatEditSizeField, FORMAT_WINDOWNAME": Size Field", purgeable)
{
	{ 0, 0, FONTSIZE_FIELD_B - FONTSIZE_FIELD_T, FONTSIZE_FIELD_R - FONTSIZE_FIELD_L },
	0,
	visible,
	0,
	0,
	kControlEditTextProc,//912/*kControlEditUnicodeTextProc*/,
	0,
	""
};

resource 'CNTL' (kIDFormatEditSizePopUpMenu, FORMAT_WINDOWNAME": Size Pop-Up Menu", purgeable)
{
	{ 0, 0, FONTSIZE_POPUPMENU_B - FONTSIZE_POPUPMENU_T, FONTSIZE_POPUPMENU_R - FONTSIZE_POPUPMENU_L },
	popupTitleLeftJust, // see page 5-119 of IM:MTE for valid values
	visible,
	0, // title width in pixels
	kPopUpMenuIDSize, // menu resource ID
	kControlPopupButtonProc | kControlPopupFixedWidthVariant,
	0,
	""
};

resource 'CNTL' (kIDFormatEditForegroundColorLabel, FORMAT_WINDOWNAME": Foreground Color Label", purgeable)
{
	{ 0, 0, TEXT_HT_BIG, BOTTOM_CHECKBOX_R - BOTTOM_CHECKBOX_L },
	0,
	visible,
	0,
	0,
	kControlStaticTextProc,
	0,
	""
};

resource 'CNTL' (kIDFormatEditBackgroundColorLabel, FORMAT_WINDOWNAME": Background Color Label", purgeable)
{
	{ 0, 0, TEXT_HT_BIG, BOTTOM_CHECKBOX_R - BOTTOM_CHECKBOX_L },
	0,
	visible,
	0,
	0,
	kControlStaticTextProc,
	0,
	""
};

#endif
