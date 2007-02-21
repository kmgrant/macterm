/*!	\file WindowTitleDialog.h
	\brief Implements a dialog box for changing the title of a
	terminal window.
	
	On Mac OS X, this dialog box is physically attached to its
	parent window, using the new window-modal “sheet” dialog
	type.
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

#ifndef __WINDOWTITLEDIALOG__
#define __WINDOWTITLEDIALOG__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "SessionRef.typedef.h"



#pragma mark Types

typedef struct OpaqueWindowTitleDialog**		WindowTitleDialogRef;

#pragma mark Callbacks

/*!
Window Title Dialog Close Notification Method

When a window title dialog is closed, this method
is invoked.  Use this to know exactly when it is
safe to call WindowTitleDialog_Dispose().
*/
typedef void	 (*WindowTitleDialogCloseNotifyProcPtr)	(WindowTitleDialogRef	inDialogThatClosed,
														 Boolean				inOKButtonPressed);
inline void
InvokeWindowTitleDialogCloseNotifyProc		(WindowTitleDialogCloseNotifyProcPtr	inUserRoutine,
											 WindowTitleDialogRef					inDialogThatClosed,
											 Boolean								inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

void
	WindowTitleDialog_StandardCloseNotifyProc	(WindowTitleDialogRef					inDialogThatClosed,
												 Boolean								inOKButtonPressed);

WindowTitleDialogRef
	WindowTitleDialog_New						(WindowRef								inParentWindow,
												 WindowTitleDialogCloseNotifyProcPtr	inCloseNotifyProcPtr =
																							WindowTitleDialog_StandardCloseNotifyProc);

WindowTitleDialogRef
	WindowTitleDialog_NewForSession				(SessionRef								inSession,
												 WindowTitleDialogCloseNotifyProcPtr	inCloseNotifyProcPtr =
																							WindowTitleDialog_StandardCloseNotifyProc);

void
	WindowTitleDialog_Dispose					(WindowTitleDialogRef*					inoutDialogPtr);

void
	WindowTitleDialog_Display					(WindowTitleDialogRef					inDialog);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
