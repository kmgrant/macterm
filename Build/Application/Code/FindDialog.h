/*!	\file FindDialog.h
	\brief A sheet used to perform searches in the scrollback
	buffers of terminal windows.
*/
/*###############################################################

	MacTelnet
		© 1998-2009 by Kevin Grant.
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

#ifndef __FINDDIALOG__
#define __FINDDIALOG__

// Mac includes
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "TerminalWindow.h"



#pragma mark Constants

typedef UInt16 FindDialog_Options;
enum
{
	kFindDialog_OptionsAllOff				= 0,
	kFindDialog_OptionsDefault				= kFindDialog_OptionsAllOff,
	kFindDialog_OptionCaseSensitivity		= (1 << 0),
	kFindDialog_OptionOldestLinesFirst		= (1 << 1)
};

#pragma mark Types

typedef struct FindDialog_OpaqueStruct*		FindDialog_Ref;

#pragma mark Callbacks

/*!
Find Dialog Close Notification Method

When a Find dialog is closed, this method is
invoked.  Use this to know exactly when it is
safe to call FindDialog_Dispose().
*/
typedef void	 (*FindDialog_CloseNotifyProcPtr)	(FindDialog_Ref		inDialogThatClosed);
inline void
FindDialog_InvokeCloseNotifyProc	(FindDialog_CloseNotifyProcPtr	inUserRoutine,
									 FindDialog_Ref					inDialogThatClosed)
{
	(*inUserRoutine)(inDialogThatClosed);
}



#pragma mark Public Methods

FindDialog_Ref
	FindDialog_New						(TerminalWindowRef				inTerminalWindow,
										 FindDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
										 CFMutableArrayRef				inoutQueryStringHistory,
										 FindDialog_Options				inFlags = kFindDialog_OptionsAllOff);

void
	FindDialog_Dispose					(FindDialog_Ref*				inoutDialogPtr);

void
	FindDialog_Display					(FindDialog_Ref					inDialog);

FindDialog_Options
	FindDialog_ReturnOptions			(FindDialog_Ref					inDialog);

TerminalWindowRef
	FindDialog_ReturnTerminalWindow		(FindDialog_Ref					inDialog);

void
	FindDialog_StandardCloseNotifyProc	(FindDialog_Ref					inDialogThatClosed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
