/*!	\file DuplicateDialog.h
	\brief A sheet used to enter a different name for something.
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

#ifndef __DUPLICATEDIALOG__
#define __DUPLICATEDIALOG__

// Mac includes
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef struct OpaqueDuplicateDialog**	DuplicateDialogRef;

#pragma mark Callbacks

/*!
Duplicate Dialog Close Notification Method

When a Duplicate dialog is closed, this method is
invoked.  Use this to know exactly when it is safe
to call DuplicateDialog_Dispose().
*/
typedef void	 (*DuplicateDialogCloseNotifyProcPtr)	(DuplicateDialogRef		inDialogThatClosed,
														 Boolean				inOKButtonPressed,
														 void*					inUserData);
inline void
InvokeDuplicateDialogCloseNotifyProc	(DuplicateDialogCloseNotifyProcPtr	inUserRoutine,
										 DuplicateDialogRef					inDialogThatClosed,
										 Boolean							inOKButtonPressed,
										 void*								inUserData)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed, inUserData);
}



#pragma mark Public Methods

DuplicateDialogRef
	DuplicateDialog_New						(DuplicateDialogCloseNotifyProcPtr	inCloseNotifyProcPtr,
											 CFStringRef						inInitialNameString,
											 void*								inCloseNotifyProcUserData);

void
	DuplicateDialog_Dispose					(DuplicateDialogRef*				inoutDialogPtr);

void
	DuplicateDialog_Display					(DuplicateDialogRef					inDialog,
											 WindowRef							inParentWindow);

void
	DuplicateDialog_GetNameString			(DuplicateDialogRef					inDialog,
											 CFStringRef&						outString);

void
	DuplicateDialog_StandardCloseNotifyProc	(DuplicateDialogRef					inDialogThatClosed,
											 Boolean							inOKButtonPressed,
											 void*								inUserData);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
