/*!	\file Undoables.h
	\brief Full support for Undo using abstract commands.
	
	With the power of this module at your fingertips, there’s no
	excuse not to support Undo and Redo in your application!
	Using a simple but very flexible interface, you can register
	actions as being undoable as soon as they occur.  This
	module will retain relevant data (a “context”), as well as a
	pointer to the action routine that can use your context to
	perform an Undo or Redo on demand.  The code could not be
	simpler - just use one function call in response to the user
	selecting Undo, and a different function call if the user
	specifies Redo.  This method handles the rest - you can even
	find out useful information about what the correct command
	text and enabled state is for the Undo and Redo items in the
	Edit menu (because they are specified at action registration
	time).
*/
/*###############################################################

	Contexts Library
	© 1998-2018 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

/*!
Possible instructions to give to a callback routine.
*/
typedef SInt16 Undoables_ActionInstruction;
enum
{
	kUndoables_ActionInstructionUndo	= 1,	//!< requesting procedure to undo the action that is apparently defined by the context
	kUndoables_ActionInstructionRedo	= 2,	//!< requesting procedure to redo the action that is apparently defined by the context
	kUndoables_ActionInstructionDispose	= 3		//!< the action is being destroyed for some reason - deallocate context’s memory, etc.
												//!  and then invoke Undoables_DisposeAction()
};

/*!
User-defined ID for a generic context.
*/
typedef SInt32 Undoables_ContextIdentifier;
enum
{
	kUndoables_ContextIdentifierInvalid		= '----'	//!< do not use this ID
};

/*!
Style of Undo to support; one at a time, or stacked.
*/
typedef SInt16 Undoables_UndoHandlingMechanism;
enum
{
	kUndoables_UndoHandlingMechanismOnlyOne		= 0,	//!< next undoable action throws away previous; “remove” replaces Redo action
	kUndoables_UndoHandlingMechanismMultiple	= 1		//!< next undoable action on Undo stack; “remove” moves action to Redo stack
};

#pragma mark Types

typedef struct Undoables_OpaqueAction*		Undoables_ActionRef;

#pragma mark Callbacks

/*!
Undo Action Method

This routine is called when the top action on the
Undoables stack is told to Undo itself.  Enough
information is passed to the routine that you
should be able to undo any operation.

IMPORTANT:	Your routine should not respond to any
			instruction it does not recognize!
*/
typedef void (*Undoables_ActionProcPtr)		(Undoables_ActionInstruction	inDoWhat,
											 Undoables_ActionRef			inApplicableAction,
											 void*							inContextPtr);
inline void
Undoables_InvokeActionProc	(Undoables_ActionProcPtr		inUserRoutine,
							 Undoables_ActionInstruction	inDoWhat,
							 Undoables_ActionRef			inApplicableAction,
							 void*							inContextPtr)
{
	(*inUserRoutine)(inDoWhat, inApplicableAction, inContextPtr);
}



/*###############################################################
	INITIALIZING AND FINISHING WITH UNDOABLES
###############################################################*/

// CALL THIS ROUTINE ONCE, BEFORE ANY OTHER UNDOABLES ROUTINE
void
	Undoables_Init						(Undoables_UndoHandlingMechanism	inUndoHandlingMechanism,
										 CFStringRef						inDisabledUndoCommandName,
										 CFStringRef						inDisabledRedoCommandNameOrNullIfRedoIsNotUsed);

// CALL THIS ROUTINE AFTER YOU ARE PERMANENTLY DONE WITH UNDOABLES
void
	Undoables_Done						();

/*###############################################################
	CREATING AND DESTROYING UNDOABLE ACTIONS
###############################################################*/

Undoables_ActionRef
	Undoables_NewAction					(CFStringRef						inUndoCommandName,
										 CFStringRef						inRedoCommandNameOrNullIfNotRedoable,
										 Undoables_ActionProcPtr			inHowToUndoAction,
										 Undoables_ContextIdentifier		inUserDefinedContextIdentifier,
										 void*								inUserDefinedContextPtr);

void
	Undoables_DisposeAction				(Undoables_ActionRef*				inoutRefPtr);

/*###############################################################
	GETTING INFORMATION ABOUT ACTIONS
###############################################################*/

Undoables_ContextIdentifier
	Undoables_ReturnActionID			(Undoables_ActionRef				inRef);

/*###############################################################
	MANAGING THE STACKS OF UNDOABLE AND REDOABLE OPERATIONS
###############################################################*/

void
	Undoables_AddAction					(Undoables_ActionRef				inActionToAdd);

void
	Undoables_RedoLastUndo				();

void
	Undoables_RemoveAction				(Undoables_ActionRef				inActionToRemove);

void
	Undoables_UndoLastAction			();

/*###############################################################
	SETTING UP THE STATES OF EDIT MENU ITEMS
###############################################################*/

void
	Undoables_GetRedoCommandInfo		(CFStringRef&						outCommandText,
										 Boolean*							outCommandEnabled);

void
	Undoables_GetUndoCommandInfo		(CFStringRef&						outCommandText,
										 Boolean*							outCommandEnabled);

// BELOW IS REQUIRED NEWLINE TO END FILE
