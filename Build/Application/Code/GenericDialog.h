/*!	\file GenericDialog.h
	\brief Allows a user interface that is both a panel
	and a dialog to be displayed as a modal dialog or
	sheet.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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

#ifndef __GENERICDIALOG__
#define __GENERICDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "HelpSystem.h"
#include "Panel.h"



#pragma mark Constants

enum GenericDialog_DialogEffect
{
	kGenericDialog_DialogEffectCloseNormally		= 0,	//!< sheet closes with animation
	kGenericDialog_DialogEffectCloseImmediately		= 1,	//!< sheet closes without animation (e.g. a Close button, or Cancel in rare cases)
	kGenericDialog_DialogEffectNone					= 2		//!< no effect on the sheet (e.g. command button)
};

#pragma mark Types

typedef struct GenericDialog_OpaqueRef*		GenericDialog_Ref;

#pragma mark Callbacks

/*!
Generic Dialog Close Notification Method

Invoked when a button is clicked that closes the
dialog.  Use this to perform any post-processing.
See also GenericDialog_StandardCloseNotifyProc().

You should NOT call GenericDialog_Dispose() in this
routine!
*/
typedef void	 (*GenericDialog_CloseNotifyProcPtr)	(GenericDialog_Ref		inDialogThatClosed,
														 Boolean				inOKButtonPressed);
inline void
GenericDialog_InvokeCloseNotifyProc		(GenericDialog_CloseNotifyProcPtr	inUserRoutine,
										 GenericDialog_Ref					inDialogThatClosed,
										 Boolean							inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

GenericDialog_Ref
	GenericDialog_New							(HIWindowRef						inParentWindowOrNullForModalDialog,
												 Panel_Ref							inHostedPanel,
												 void*								inDataSetPtr,
												 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
												 HelpSystem_KeyPhrase				inHelpButtonAction = kHelpSystem_KeyPhraseDefault);

void
	GenericDialog_Dispose						(GenericDialog_Ref*					inoutDialogPtr);

void
	GenericDialog_AddButton						(GenericDialog_Ref					inDialog,
												 CFStringRef						inButtonTitle,
												 UInt32								inButtonCommandID);

void
	GenericDialog_Display						(GenericDialog_Ref					inDialog);

Panel_Ref
	GenericDialog_ReturnHostedPanel				(GenericDialog_Ref					inDialog);

void*
	GenericDialog_ReturnImplementation			(GenericDialog_Ref					inDialog);

HIWindowRef
	GenericDialog_ReturnParentWindow			(GenericDialog_Ref					inDialog);

void
	GenericDialog_SetCommandDialogEffect		(GenericDialog_Ref					inDialog,
												 UInt32								inCommandID,
												 GenericDialog_DialogEffect			inEffect);

void
	GenericDialog_SetImplementation				(GenericDialog_Ref					inDialog,
												 void*								inDataPtr);

void
	GenericDialog_StandardCloseNotifyProc		(GenericDialog_Ref					inDialogThatClosed,
												 Boolean							inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
