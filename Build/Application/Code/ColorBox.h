/*!	\file ColorBox.h
	\brief User interface element for selecting colors.
	
	The control looks like a bevel button, and has a standard
	size.  If the button is clicked quickly, a standard Color
	Picker appears immediately and the user can select a new
	color or cancel.  If the user clicks and holds on the button,
	a menu of color choices appears.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#include "UniversalDefines.h"

#ifndef __COLORBOX__
#define __COLORBOX__

// Mac includes
#include <Carbon/Carbon.h>



/*!
Color Box Color Change Notification Method

This type of routine is invoked whenever a color box’s
content color is changed.  Since it is possible for the
user to modify the content color beyond your control,
this routine ensures you are notified whenever the color
changes (useful for modeless windows that might need to
respond right away to changes).
*/
typedef void	 (*ColorBox_ChangeNotifyProcPtr)	(HIViewRef			inColorBoxThatWasChanged,
													 RGBColor const*	inNewColor,
													 void*				inContext);
inline void ColorBox_InvokeChangeNotifyProc	(ColorBox_ChangeNotifyProcPtr	inUserRoutine,
											 HIViewRef						inColorBoxThatWasChanged,
											 RGBColor const*				inNewColor,
											 void*							inContext)
{
	(*inUserRoutine)(inColorBoxThatWasChanged, inNewColor, inContext);
}



//!\name Creating and Destroying Color Boxes
//@{

// WHEN FINISHED WITH YOUR COLOR BOX, CALL THE APPROPRIATE DISPOSAL ROUTINE BELOW
OSStatus
	ColorBox_AttachToBevelButton			(HIViewRef						inoutUserPaneView,
											 RGBColor const*				inInitialColorOrNull = nullptr);

void
	ColorBox_DetachFromBevelButton			(HIViewRef						inView);

//@}

//!\name Accessing Colors
//@{

void
	ColorBox_GetColor						(HIViewRef						inView,
											 RGBColor*						outColorPtr);

void
	ColorBox_SetColor						(HIViewRef						inView,
											 RGBColor const*				inColorPtr);

// CALL IN RESPONSE TO A CLICK - DISPLAYS A COLOR PICKER FOR THE USER, AUTOMATICALLY UPDATING THE BUTTON’S COLOR
Boolean
	ColorBox_UserSetColor					(HIViewRef						inView);

//@}

//!\name Receiving Change Notification
//@{

void
	ColorBox_SetColorChangeNotifyProc		(HIViewRef						inView,
											 ColorBox_ChangeNotifyProcPtr	inNotifyProcPtr,
											 void*							inContextPtr);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
