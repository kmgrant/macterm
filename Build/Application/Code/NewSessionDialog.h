/*!	\file NewSessionDialog.h
	\brief Implements the New Session dialog box.
	
	On Mac OS X, this dialog box is physically attached to its
	terminal window, using the new window-modal “sheet” dialog
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

#ifndef __NEWSESSIONDIALOG__
#define __NEWSESSIONDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "TerminalWindow.h"



#pragma mark Types

typedef struct NewSessionDialog_OpaqueRef*		NewSessionDialog_Ref;

#pragma mark Callbacks

/*!
New Session Dialog Close Notification Method

Invoked when a button is clicked that closes the
dialog.  Use this to perform any post-processing.
See also NewSessionDialog_StandardCloseNotifyProc().

You should NOT call NewSessionDialog_Dispose()
in this routine!
*/
typedef void	 (*NewSessionDialog_CloseNotifyProcPtr)	(NewSessionDialog_Ref	inDialogThatClosed,
														 Boolean				inOKButtonPressed);
inline void
NewSessionDialog_InvokeCloseNotifyProc	(NewSessionDialog_CloseNotifyProcPtr	inUserRoutine,
										 NewSessionDialog_Ref					inDialogThatClosed,
										 Boolean								inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

NewSessionDialog_Ref
	NewSessionDialog_New						(TerminalWindowRef						inTerminalWindow,
												 NewSessionDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr);

void
	NewSessionDialog_Dispose					(NewSessionDialog_Ref*					inoutDialogPtr);

void
	NewSessionDialog_Display					(NewSessionDialog_Ref					inDialog);

TerminalWindowRef
	NewSessionDialog_ReturnTerminalWindow		(NewSessionDialog_Ref					inDialog);

void
	NewSessionDialog_StandardCloseNotifyProc	(NewSessionDialog_Ref					inDialogThatClosed,
												 Boolean								inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
