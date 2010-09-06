/*
**************************************************************************
	SpacingConstants.r
	
	© 1998-2004 by Kevin Grant
	
	This file contains constants for “standard” dimensions and spacings
	for controls.  They are largely based on the Apple Human Interface
	Guidelines and the Aqua Layout Guidelines.  They are used both in
	resource files to set up window and dialog box controls consistently,
	and in source files, in rare cases (such as the Preferences window)
	where controls are set up programmatically.
	
	All of the measurements are in pixels.  The following conventions
	should be noticeable:
	- constants containing "_WD" represent widths
	- constants containing "_HT" represent heights
	- constants prefixed by "HSP_" represent horizontal-only spacings
	- constants prefixed by "VSP_" represent vertical-only spacings
	- constants containing "CUSHION" or "SPACE_EXTRA" are 2D spacings
	- an "_SMALL" suffix refers to the small version of a control
**************************************************************************
*/

#ifndef __SPACINGCONSTANTS__
#define __SPACINGCONSTANTS__



/*!
Control Sizes

Purpose:
	To define the minimum, maximum, or sometimes only size a
	control of a particular type can have.  This guarantees
	consistency across the user interface, and in the case of
	switching to a different set of guidelines (AQUA_GUI) it
	is possible to instantly change all metrics as well.

Usage:
	When sizing dialog items, always check this list to see if
	there is an appropriate constant for the kind of control
	you are sizing, and use the constant for the width or height
	(as appropriate).
	
	You should also use these constants when laying out other
	controls; for example, to specify the position of a control
	you can use these constants to figure out how large any
	intermediate controls will be, and determine the correct
	pixel offset.
	
	Finally, these offsets can usually be used to precisely
	define the width or height of a dialog box; knowing the
	contents of a dialog, you can literally define its size
	in terms of the sizes of its controls, such that the
	resultant dialog is exactly as large as it needs to be,
	no bigger and no smaller!
*/

// standard button dimensions
enum
{
	// push button minimum acceptable width, and standard height
	BUTTON_WD_MINIMUM					= 69,
	BUTTON_HT							= 20,
	BUTTON_HT_SMALL						= 17,
	
	// checkboxes and radio buttons
	CHECKBOX_HT							= 21,
	RADIOBUTTON_HT						= CHECKBOX_HT,
	
	// the standard help button; its height is BUTTON_HT
	HELP_BUTTON_WD						= 21,
	
	// bevel buttons for choosing a color
	COLORBOX_WD							= 48,
	COLORBOX_HT							= 22,
	
	// disclosure triangles
	DISCLOSURETRIANGLE_WD				= 12,
	DISCLOSURETRIANGLE_HT				= 12,
	
	// double-arrow-button controls, which are always the same size
	LITTLEARROWS_WD						= 13,
	LITTLEARROWS_HT						= 23,
	
	// asynchronous arrow animations, which are always the same size
	CHASING_ARROWS_WD					= 16,
	CHASING_ARROWS_HT					= 16,
	
	// pop-up menu buttons
	POPUPMENUBUTTONONLY_WD				= 24,
	POPUPMENUBUTTON_HT					= 21,
	
	// tab buttons; the necessary clearance to place controls below a north-pointing tab
	TAB_HT_BIG							= 22,
	TAB_HT_SMALL						= 16
};

// sizes for other kinds of controls
enum
{
	// thermometer controls, which are always the same height
	PROGRESSBAR_HT						= 24,
	
	// standard-sized scroll bars
	SCROLL_BAR_WD						= 16,
	SCROLL_BAR_HT						= SCROLL_BAR_WD,
	SCROLL_BAR_WD_SMALL					= 13,
	SCROLL_BAR_HT_SMALL					= SCROLL_BAR_WD,
	
	// basic slider controls
	SLIDER_WD							= 18,
	SLIDER_HT							= SLIDER_WD,
	
	// static or editable text, in the regular system font, huge system font, or small system font
	TEXT_HT_BIG							= 16,
	TEXT_HT_HUGE						= 24,
	TEXT_HT_SMALL						= 14,
	
	// horizontal or vertical etched lines, which are always the same thickness
	SEPARATOR_WD						= 4,
	SEPARATOR_HT						= SEPARATOR_WD
};

// icon sizes
enum
{
	// “small” icons
	ICON_WD_SMALL						= 32,
	ICON_HT_SMALL						= 32,
	
	// “standard” icons
	ICON_WD_BIG							= 64,
	ICON_HT_BIG							= 64,
	
	// “huge” icons
	ICON_WD_HUGE						= 128,
	ICON_HT_HUGE						= 128,
	
	// standard icon size for alerts
	ICON_WD_ALERT						= 64,
	ICON_HT_ALERT						= 64,
	
	// standard icon size for toolbars
	ICON_WD_TOOLBAR						= 32,
	ICON_HT_TOOLBAR						= 32,
	
	// standard small icon size for toolbars
	ICON_WD_TOOLBAR_SMALL				= 24,
	ICON_HT_TOOLBAR_SMALL				= 24
};

// cushions, which define space both horizontally and vertically that must surround controls of specific types
enum
{
	SPACE_EXTRA_CONTAINER				= 3,				// difference between the perceived edges and the actual (outset) edges
	SPACE_EXTRA_TEXTEDIT				= 4,				// difference between the control edges and the visible (outset) edges
	SPACE_EXTRA_TEXT_TOP				= 3,				// necessary additional space for static text below some kind of top edge
	MINIMUM_BUTTON_TITLE_CUSHION		= 20,				// horizontal space between a button’s text title and its outer edges
	MINIMUM_PLACARD_CUSHION				= 2,				// space between a placard’s edges and its content, ignoring bevels
	MINIMUM_TICK_MARKS_CUSHION			= 6					// space padded to a slider’s size to make room for tick marks
};



/*!
Dialog Edge Spacings

Purpose:
	To define the exact spacing between the edges of a modal,
	movable modal or modeless dialog box and the dialog items
	closest to those edges.

Usage:
	When positioning dialog items along the top, left, right
	or bottom edges of a dialog, use these spacings to figure
	out the proper pixel offset.
*/
enum
{
	// space between button’s edges and dialog box interior
	HSP_BUTTON_AND_DIALOG				= 24,
	VSP_BUTTON_AND_DIALOG				= 20,
	
	// space between group box’s edge and dialog box interior
	HSP_GROUPBOX_AND_DIALOG				= 20,
	VSP_GROUPBOX_AND_DIALOG				= 20,
	
	// space between bottom/left/right tab control edges and dialog box interior (north-facing tabs)
	HSP_TAB_AND_DIALOG					= 20,
	VSP_TAB_AND_DIALOG					= 17,
	
	// space between static text control edges and dialog box interior; add SPACE_EXTRA_TEXTEDIT for editable text areas
	HSP_TEXT_AND_DIALOG					= 18,
	VSP_TEXT_AND_DIALOG					= HSP_TEXT_AND_DIALOG,
	
	// for any control that doesn’t have a more suitable constant above, use the ones below
	HSP_CONTROL_AND_DIALOG				= 14,
	VSP_CONTROL_AND_DIALOG				= HSP_CONTROL_AND_DIALOG
};



/*!
Control Spacings

Purpose:
	To define the minimum amount of space that needs to
	exist between controls so they are not cramped and no
	graphics drawing glitches occur, etc.

Usage:
	If you are placing two controls of the same type, look
	for an constant and (if one exists) use it to define
	the distance between those controls.
	
	Sometimes, there are standard spacings between controls
	of two different types; if that is applicable, be sure
	to use the proper constant from the list below.
*/

// spacings between controls of the same type
enum
{
	// for any control that doesn’t have a more suitable constant below, use these
	HSP_CONTROLS						= 12,
	VSP_CONTROLS						= 8,
	
	// spacing between two push button controls
	HSP_BUTTONS							= 12,
	VSP_BUTTONS							= 10,
	
	// spacing between two checkboxes or radio buttons
	HSP_CHECKBOXES						= HSP_BUTTONS,
	VSP_CHECKBOXES						= 6,
	HSP_RADIOS							= HSP_CHECKBOXES,
	VSP_RADIOS							= VSP_CHECKBOXES,
	
	// spacing between pop-up menu buttons
	HSP_POPUPMENUBUTTONS				= 4,
	VSP_POPUPMENUBUTTONS				= 6,
	
	// spacing between two text fields
	HSP_EDITABLETEXTAREAS				= 20, // (2 * SPACE_EXTRA_TEXTEDIT + HSP_CONTROLS)
	VSP_EDITABLETEXTAREAS				= 16, // (2 * SPACE_EXTRA_TEXTEDIT + VSP_CONTROLS)
	
	// spacing between primary or secondary group boxes
	HSP_GROUPBOXES						= 10,
	VSP_GROUPBOXES						= 6
};

// spacings between controls of different types
enum
{
	// spacing between a push button and some non-button control
	HSP_BUTTON_AND_CONTROL				= 12,
	VSP_BUTTON_AND_CONTROL				= 12,
	
	// space between a bevel button and its small-font text label beneath
	VSP_BUTTON_AND_TEXT_LABEL			= 4,
	
	// spacing between the top of a checkbox-groupbox and its interior
	VSP_GROUPCHECKBOX_AND_CONTROL		= 27,
	VSP_GROUPCHECKBOX_AND_TEXT			= 30, // (VSP_GROUPCHECKBOX_AND_CONTROL + SPACE_EXTRA_TEXT_TOP)
	
	// spacing between the top of a pop-up-menu-groupbox and its interior
	VSP_GROUPPOPUP_AND_CONTROL			= 27,
	VSP_GROUPPOPUP_AND_TEXT				= 30, // (VSP_GROUPPOPUP_AND_CONTROL + SPACE_EXTRA_TEXT_TOP)
	
	// spacing between the top of a text-title-groupbox and its interior
	VSP_GROUPBOXTEXT_AND_CONTROL		= 24,
	VSP_GROUPBOXTEXT_AND_TEXT			= 27, // (VSP_GROUPBOXTEXT_AND_CONTROL + SPACE_EXTRA_TEXT_TOP)
	
	// spacing between other edges of a groupbox and its interior
	HSP_GROUPBOX_AND_CONTROL			= 15,
	VSP_GROUPBOX_AND_CONTROL			= HSP_GROUPBOX_AND_CONTROL,
	VSP_GROUPBOX_AND_TEXT				= 18, // (VSP_GROUPBOX_AND_CONTROL + SPACE_EXTRA_TEXT_TOP)
	
	// spacing between a secondary group box and interior help text
	HSP_GROUPBOX_AND_HELPTEXT			= 5,
	VSP_GROUPBOX_AND_HELPTEXT			= HSP_GROUPBOX_AND_HELPTEXT,
	
	// spacing between a small icon and associated text
	HSP_ICON_AND_TEXT					= 4,
	
	// spacing between an invisible pane (like a panel) and its interior
	HSP_PANE_AND_CONTROL				= 2,
	VSP_PANE_AND_CONTROL				= 2,
	
	// spacing between a set of dialog box tabs and anything around the tabs
	HSP_TABS_AND_CONTROL				= HSP_CONTROLS,
	VSP_TABS_AND_CONTROL				= 18,
	
	// spacing between a tab control’s interior and its invisible pane
	HSP_TABPANE_AND_TAB					= 20,
	VSP_TABPANE_AND_TAB					= 10,
	
	// space between a disclosure triangle and its small-font text label
	HSP_TRIANGLE_AND_TEXT_LABEL			= 4
};



/*!
Standard Help Balloon Offsets

Purpose:
	To define the exact point relative to the location of a
	control where a help balloon’s tip should be placed.  It
	is nice to have this consistency across all controls of
	the same type, although that is not strictly necessary.
	
Usage:
	When defining help balloons for controls of specific
	types, be sure to check this list to see if there is a
	standard tip position.
*/
enum
{
	BUTTON_BALLOON_TIP_H				= 60,
	BUTTON_BALLOON_TIP_V				= 10, // (BUTTON_HT / 2)
	SMALL_BUTTON_BALLOON_TIP_H			= 15,
	SMALL_BUTTON_BALLOON_TIP_V			= 10, // (BUTTON_HT / 2)
	POPUPMENUBUTTON_BALLOON_TIP_H		= 10,
	POPUPMENUBUTTON_BALLOON_TIP_V		= 10, // (BUTTON_HT / 2)
	CHECKBOX_BALLOON_TIP_H				= 40,
	CHECKBOX_BALLOON_TIP_V				= 10, // (CHECKBOX_HT / 2)
	RADIOBUTTON_BALLOON_TIP_H			= CHECKBOX_BALLOON_TIP_H,
	RADIOBUTTON_BALLOON_TIP_V			= 10, // (RADIOBUTTON_HT / 2)
	HELPBUTTON_BALLOON_TIP_H			= 15,
	HELPBUTTON_BALLOON_TIP_V			= 5,
	LITTLEARROWS_BALLOON_TIP_H			= 6, // (LITTLEARROWS_WD / 2)
	LITTLEARROWS_BALLOON_TIP_V			= 11, // (LITTLEARROWS_HT / 2)
	LIST_BALLOON_TIP_H					= 50,
	LIST_BALLOON_TIP_V					= 50,
	TEXT_BIG_BALLOON_TIP_H				= 25,
	TEXT_BIG_BALLOON_TIP_V				= 8, // (TEXT_HT_BIG / 2)
	TEXT_SMALL_BALLOON_TIP_H			= 15,
	TEXT_SMALL_BALLOON_TIP_V			= 7 // (TEXT_HT_SMALL / 2)
};

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
