/*!	\file SizeDialog.h
	\brief Implements the terminal screen size dialog.
	
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

#ifndef __SIZEDIALOG__
#define __SIZEDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "TerminalWindow.h"



#pragma mark Types

typedef struct OpaqueTerminalSizeDialog**		TerminalSizeDialogRef;

#pragma mark Callbacks

/*!
Size Dialog Close Notification Method

Invoked when a button is clicked that closes the
dialog.  Use this to perform any post-processing.
See also SizeDialog_StandardCloseNotifyProc().

You should NOT call SizeDialog_Dispose() in this
routine!
*/
typedef void	 (*SizeDialogCloseNotifyProcPtr)	(TerminalSizeDialogRef		inDialogThatClosed,
													 Boolean 					inOKButtonPressed);
inline void
InvokeSizeDialogCloseNotifyProc		(SizeDialogCloseNotifyProcPtr	inUserRoutine,
									 TerminalSizeDialogRef			inDialogThatClosed,
									 Boolean						inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

TerminalSizeDialogRef
	SizeDialog_New									(TerminalWindowRef				inTerminalWindow,
													 SizeDialogCloseNotifyProcPtr	inCloseNotifyProcPtr);

void
	SizeDialog_Dispose								(TerminalSizeDialogRef*			inoutDialogPtr);

void
	SizeDialog_Display								(TerminalSizeDialogRef			inDialog);

void
	SizeDialog_GetDisplayedDimensions				(TerminalSizeDialogRef			inDialog,
													 UInt16&						outColumns,
													 UInt16&						outRows);

TerminalWindowRef
	SizeDialog_GetParentTerminalWindow				(TerminalSizeDialogRef			inDialog);

void
	SizeDialog_SendRecordableDimensionChangeEvents	(SInt16							inNewColumns,
													 SInt16							inNewRows);

void
	SizeDialog_StandardCloseNotifyProc				(TerminalSizeDialogRef			inDialogThatClosed,
													 Boolean						inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
