/*
**************************************************************************
	InterfaceLibAppResources.r
	
	Interface Library 1.1
	© 1998-2004 by Kevin Grant
	
	This file contains resource descriptions for resources required by
	the Interface Library alone.  You should #include this file in one
	of your application’s Rez input files.
**************************************************************************
*/

#ifdef REZ
#include "Carbon.r"
#include "CoreServices.r"
#endif

#include "ControlResources.h"
#include "DialogResources.h"
#include "SpacingConstants.r"



#define INTERFACELIB_ALERT_ICON_LEFT			20
#define INTERFACELIB_ALERT_DIALOG_WD			420

#ifdef REZ

/* controls */

/*
A standard help button.  This control has a fixed size, and an icon
that gets its image from the system software.
*/
resource 'CNTL' (kIDAppleGuideButton, "Help Button", purgeable)
{
	{ 0, 0, 26, 26 },
	0,
	visible,
	kHelpIconResource, // will be changed programmatically to use a 32-bit icon
	kControlBehaviorPushbutton | kControlContentIconSuiteRes,
	496, // the round button control def-proc, for which there is no constant yet
	0,
	""
};


/*
*******************************************
	STANDARD ALERT DIALOG BOX
*******************************************
*/

/* a standard alert stop icon */
resource 'CNTL' (kInterfaceLibControlAlertIconStop, "Stop Icon", purgeable)
{
	{ 0, 0, ICON_HT_ALERT, ICON_WD_ALERT },
	0, // icon ID
	visible,
	0,
	0,
	kControlIconNoTrackProc,
	0,
	""
};

/* a standard alert note icon */
resource 'CNTL' (kInterfaceLibControlAlertIconNote, "Note Icon", purgeable)
{
	{ 0, 0, ICON_HT_ALERT, ICON_WD_ALERT },
	1, // icon ID
	visible,
	0,
	0,
	kControlIconNoTrackProc,
	0,
	""
};

/* a standard alert caution icon */
resource 'CNTL' (kInterfaceLibControlAlertIconCaution, "Caution Icon", purgeable)
{
	{ 0, 0, ICON_HT_ALERT, ICON_WD_ALERT },
	2, // icon ID
	visible,
	0,
	0,
	kControlIconNoTrackProc,
	0,
	""
};

#endif /* ifdef REZ */

// BELOW IS REQUIRED NEWLINE TO END FILE
