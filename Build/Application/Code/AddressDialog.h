/*!	\file AddressDialog.h
	\brief Implements the IP address dialog.
*/
/*###############################################################

	MacTelnet
		© 1998-2007 by Kevin Grant.
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

#ifndef __ADDRESSDIALOG__
#define __ADDRESSDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef struct AddressDialog_OpaqueStructure**		AddressDialog_Ref;

#pragma mark Callbacks

/*!
IP Address Dialog Close Notification Method

Invoked when a button is clicked that closes the
dialog.  Use this to perform any post-processing.
See also AddressDialog_StandardCloseNotifyProc().

You should NOT call AddressDialog_Dispose() in
this routine!
*/
typedef void	(*AddressDialog_CloseNotifyProcPtr)		(AddressDialog_Ref	inDialogThatClosed,
														 Boolean 			inOKButtonPressed);
inline void
AddressDialog_InvokeCloseNotifyProc	(AddressDialog_CloseNotifyProcPtr	inUserRoutine,
									 AddressDialog_Ref					inDialogThatClosed,
									 Boolean							inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

AddressDialog_Ref
	AddressDialog_New							(AddressDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr);

void
	AddressDialog_Dispose						(AddressDialog_Ref*					inoutDialogPtr);

void
	AddressDialog_Display						(AddressDialog_Ref					inDialog);

void
	AddressDialog_StandardCloseNotifyProc		(AddressDialog_Ref					inDialogThatClosed,
												 Boolean							inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
