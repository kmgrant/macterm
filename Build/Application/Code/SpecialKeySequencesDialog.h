/*!	\file SpecialKeySequencesDialog.h
	\brief Implements a dialog for configuring Interrupt,
	Suspend and Resume key mappings.
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

#ifndef __SPECIALKEYSEQUENCESDIALOG__
#define __SPECIALKEYSEQUENCESDIALOG__

// MacTelnet includes
#include "SessionRef.typedef.h"



#pragma mark Types

typedef struct SpecialKeySequencesDialog_OpaqueRef*		SpecialKeySequencesDialog_Ref;

#pragma mark Callbacks

/*!
Special Key Sequences Dialog Close Notification Method

Invoked when a button is clicked that closes the dialog.
Use this to perform any post-processing.  See also
SpecialKeySequencesDialog_StandardCloseNotifyProc().

You should NOT call SpecialKeySequencesDialog_Dispose()
in this routine!
*/
typedef void	 (*SpecialKeySequencesDialog_CloseNotifyProcPtr)	(SpecialKeySequencesDialog_Ref	inDialogThatClosed,
																	 Boolean						inOKButtonPressed);
inline void
SpecialKeySequencesDialog_InvokeCloseNotifyProc		(SpecialKeySequencesDialog_CloseNotifyProcPtr	inUserRoutine,
													 SpecialKeySequencesDialog_Ref					inDialogThatClosed,
													 Boolean										inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

SpecialKeySequencesDialog_Ref
	SpecialKeySequencesDialog_New		(SessionRef										inSession,
										 SpecialKeySequencesDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr);

void
	SpecialKeySequencesDialog_Dispose	(SpecialKeySequencesDialog_Ref*					inoutDialogPtr);

void
	SpecialKeySequencesDialog_Display   (SpecialKeySequencesDialog_Ref					inDialog);

SessionRef
	SpecialKeySequencesDialog_ReturnSession   (SpecialKeySequencesDialog_Ref			inDialog);

void
	SpecialKeySequencesDialog_StandardCloseNotifyProc	(SpecialKeySequencesDialog_Ref	inDialogThatClosed,
										 Boolean										inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
