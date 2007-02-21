/*
**************************************************************************
	TelnetReusableControls.r
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains data for generic controls.
**************************************************************************
*/

#ifndef __REUSABLECONTROLS_R__
#define __REUSABLECONTROLS_R__

#include "Carbon.r"

#include "ControlResources.h"
#include "SpacingConstants.r"
#include "TelnetButtonNames.h"


/*
A user pane for no other purpose than to act as an anchor for
overlaid dialog item lists.
*/
resource 'CNTL' (kIDAnchorUserPane, "Anchor", purgeable)
{
	{ 0, 0, 1, 1 },
	kControlSupportsEmbedding, // control feature bits
	visible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};


/*
This is a standard Cancel button that is 68 pixels wide.
*/
resource 'CNTL' (kIDCancelButton, "Cancel Button", purgeable)
{
	{ 0, 0, BUTTON_HT, 68 },
	0,
	visible,
	0,
	0,
	kControlPushButtonProc,
	0,
	REUSABLE_BTNNAME_CANCEL
};


/*
A user pane for clickable color boxes, such as ANSI colors.
The drawing and tracking routines must be installed
programmatically, however.

WARNING: see Rez-DITL versions for old dialogs!
*/
resource 'CNTL' (kIDColorBox, "Color Box", purgeable)
{
	{ 0, 0, COLORBOX_HT, COLORBOX_WD },
	kControlHandlesTracking, // control feature bits
	visible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};


/*
A “little arrows” control for adjusting things.  This control is
always exactly the same size and is used in only one way, so it
is obviously a standard, reusable control.
*/
resource 'CNTL' (kIDLittleArrows, "Little Arrows", purgeable)
{
	{ 0, 0, LITTLEARROWS_HT, LITTLEARROWS_WD },
	0,
	visible,
	0,
	0,
	kControlLittleArrowsProc,
	0,
	""
};


/*
This is a standard OK button that is 68 pixels wide.
*/
resource 'CNTL' (kIDOKButton, "OK Button", purgeable)
{
	{ 0, 0, BUTTON_HT, 68 },
	0,
	visible,
	0,
	0,
	kControlPushButtonProc,
	0,
	REUSABLE_BTNNAME_OK
};


/*
This is a radio button control with no title.  It is important
to initialize this control to have the proper maximum and minimum
values, hence the definition as a reusable control.
*/
resource 'CNTL' (kIDRadioButton, "Generic Radio Button", purgeable)
{
	{ 0, 0, 0, 0 /* auto-sized and auto-positioned control */ },
	kControlRadioButtonUncheckedValue, // value
	visible,
	kControlRadioButtonMixedValue, // maximum
	kControlRadioButtonUncheckedValue, // minimum
	kControlRadioButtonProc,
	0,
	""
};


/*
A static text control that can be used for anything.  Before using it,
it must be sized properly and have its text contents specified.  If it
is used in a dialog box, its font information can be specified using
an entry in the 'dftb' resource.
*/
resource 'CNTL' (kIDStaticText, "Static Text", purgeable)
{
	{ 0, 0, TEXT_HT_BIG, 0 },
	0,
	visible,
	0,
	0,
	kControlStaticTextProc,
	0,
	""
};


/*
A user pane for no other purpose than to act as a tab container.  It
is initially invisible because its contents are generated invisibly.
*/
resource 'CNTL' (kIDTabUserPane, "Tab Pane", purgeable)
{
	{ 0, 0, 1, 1 },
	kControlSupportsEmbedding, // control feature bits
	invisible,
	0,
	0,
	kControlUserPaneProc,
	0,
	""
};

#endif