/*!	\file Undoables.cp
	\brief Full support for Undo using abstract commands.
*/
/*###############################################################

	Contexts Library
	© 1998-2016 by Kevin Grant
	
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

#include <Undoables.h>
#include <UniversalDefines.h>

// standard-C++ includes
#include <algorithm>
#include <deque>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <MemoryBlockPtrLocker.template.h>



#pragma mark Types
namespace {

struct MyUndoData
{
	MyUndoData (): isRedoable(false), undoCommandName(), redoCommandName(), actionProc(nullptr),
					contextID(kUndoables_ContextIdentifierInvalid), contextPtr(nullptr),
					selfRef(REINTERPRET_CAST(this, Undoables_ActionRef)) {}
	
	Boolean							isRedoable;			// can this action be redone after being undone?
	CFRetainRelease					undoCommandName;	// name of Undo item in Edit menu
	CFRetainRelease					redoCommandName;	// name of Redo item in Edit menu
	Undoables_ActionProcPtr			actionProc;			// callback for handling various commands
	Undoables_ContextIdentifier		contextID;			// user-defined unique ID describing the context data
	void*							contextPtr;			// user-defined data, passed whenever callback is used
	Undoables_ActionRef				selfRef;			// redundant, convenient reference to this structure
};
typedef MyUndoData*		MyUndoDataPtr;

typedef MemoryBlockPtrLocker< Undoables_ActionRef, MyUndoData >		MyUndoDataPtrLocker;
typedef LockAcquireRelease< Undoables_ActionRef, MyUndoData >		MyUndoDataAutoLocker;
typedef std::deque< Undoables_ActionRef >							MyUndoStack;

} // anonymous namespace

#pragma mark Variables
namespace {

CFRetainRelease&					gDefaultUndoName ()		{ static CFRetainRelease x; return x; }
CFRetainRelease&					gDefaultRedoName ()		{ static CFRetainRelease x; return x; }
MyUndoDataPtrLocker&				gUndoDataPtrLocks ()	{ static MyUndoDataPtrLocker x; return x; }
Undoables_UndoHandlingMechanism		gUndoStackType = kUndoables_UndoHandlingMechanismOnlyOne;
MyUndoStack&						gUndoStack ()	{ static MyUndoStack x; return x; }
MyUndoStack&						gRedoStack ()	{ static MyUndoStack x; return x; }

} // anonymous namespace


#pragma mark Public Methods

/*!
Call this routine before any other in this module.
Currently, only one Undo handling mechanism is
supported.  If multiple Undo support (an “Undoable
action stack”) is implemented in the future, it
would be requested using a different parameter to
this initialization routine.

Pass two strings that will be returned by
Undoables_GetUndoCommandInfo() and
Undoables_GetRedoCommandInfo() in the event that no
pending action is installed (and therefore no
command text).  The second item is optional -
specify nullptr if your application does not even
have a Redo item in its Edit menu.  Note that it is
better to have a Redo item even if your application
does not support it - keep in mind that if you use
Undoables_GetRedoCommandInfo() and your application
does not support Redo, you will still get back the
correct item text and state for the Redo item!  You
therefore are encouraged to always specify item text
for both Undo and Redo.

IMPORTANT:	I disagree with the pseudo-standard
			taken by many Macintosh programs that
			names inactive Undo “Can’t Undo”.  This
			is wrong, period.  This introduces a
			double-negative, because the disabled
			state of the command obviously implies
			that the most recent action “can’t” be
			done.  In English locales, the parameters
			to this routine should be Pascal strings
			with the terms “Undo” and “Redo”,
			respectively.

(1.0)
*/
void
Undoables_Init	(Undoables_UndoHandlingMechanism	inUndoHandlingMechanism,
				 CFStringRef						inDisabledUndoCommandName,
				 CFStringRef						inDisabledRedoCommandNameOrNullIfRedoIsNotUsed)
{
	gUndoStackType = inUndoHandlingMechanism;
	gDefaultUndoName().setWithRetain(inDisabledUndoCommandName);
	if (inDisabledRedoCommandNameOrNullIfRedoIsNotUsed == nullptr)
	{
		gDefaultRedoName().setWithRetain(CFSTR(""));
	}
	else
	{
		gDefaultRedoName().setWithRetain(inDisabledRedoCommandNameOrNullIfRedoIsNotUsed);
	}
}// Init


/*!
Call this routine when you are finished using this
module.  This routine sends “dispose” messages to
the callbacks of all actions that are still in the
Undo and Redo stacks.

(1.0)
*/
void
Undoables_Done ()
{
	MyUndoStack const*	stacks[] = { &gUndoStack(), &gRedoStack() };
	
	
	for (UInt16 i = 0; i < (sizeof(stacks) / sizeof(MyUndoStack const*)); ++i)
	{
		while (false == stacks[i]->empty())
		{
			Undoables_ActionRef			ref = stacks[i]->front();
			Undoables_ActionProcPtr		userRoutine = nullptr;
			void*						contextPtr = nullptr;
			
			
			// IMPORTANT: The reference MUST be unlocked prior to invoking the
			//            disposal routine, since that will trigger a call to
			//            Undoables_DisposeAction() (which requires all locks
			//            to be gone).
			{
				MyUndoDataAutoLocker	ptr(gUndoDataPtrLocks(), ref);
				
				
				userRoutine = ptr->actionProc;
				contextPtr = ptr->contextPtr;
			}
			Undoables_InvokeActionProc(userRoutine, kUndoables_ActionInstructionDispose, ref, contextPtr);
			Undoables_RemoveAction(ref);
		}
	}
}// Done


/*!
As soon as you execute an undoable action (in response
to a menu command, for instance), you can create an
Undoable Action for it that provides code and data
necessary to reverse the operation at an arbitrary
point later on.

Specify the name of the menu item to use for the
undoable action (for example, “Undo Cut”).  If the
action can also be redone later, provide a menu item
name for its Redo equivalent (“Redo Cut”, in this
example).  To specify that an action cannot be redone,
simply pass nullptr in the second parameter.

Provide a pointer to a procedure that, given the
additional context identifier and data pointer, will
be able to either Undo or Redo the action specified.

Your "inHowToUndoAction" routine is also responsible
for disposing of any allocated memory.

(2.0)
*/
Undoables_ActionRef
Undoables_NewAction		(CFStringRef					inUndoCommandName,
						 CFStringRef					inRedoCommandNameOrNullIfNotRedoable,
						 Undoables_ActionProcPtr		inHowToUndoAction,
						 Undoables_ContextIdentifier	inUserDefinedContextIdentifier,
						 void*							inUserDefinedContextPtr)
{
	Undoables_ActionRef		result = REINTERPRET_CAST(new MyUndoData, Undoables_ActionRef);
	
	
	if (result != nullptr)
	{
		MyUndoDataAutoLocker	ptr(gUndoDataPtrLocks(), result);
		
		
		ptr->isRedoable = (inRedoCommandNameOrNullIfNotRedoable != nullptr);
		ptr->undoCommandName.setWithRetain(inUndoCommandName);
		if (inRedoCommandNameOrNullIfNotRedoable == nullptr)
		{
			ptr->redoCommandName.setWithRetain(CFSTR(""));
		}
		else
		{
			ptr->redoCommandName.setWithRetain(inRedoCommandNameOrNullIfNotRedoable);
		}
		ptr->actionProc = inHowToUndoAction;
		ptr->contextID = inUserDefinedContextIdentifier;
		ptr->contextPtr = inUserDefinedContextPtr;
		ptr->selfRef = result;
	}
	
	return result;
}// NewAction


/*!
Disposes of the specified action, rendering all
references to it invalid.  The Undo and Redo stacks
are first searched and any references to the given
action are removed automatically.

IMPORTANT:	The “dispose” callback instruction is
			NOT sent by this routine.  In fact, you
			typically call this routine *in response*
			to such an instruction, as part of your
			callback’s clean-up code.

(2.0)
*/
void
Undoables_DisposeAction		(Undoables_ActionRef*	inoutRefPtr)
{
	if (gUndoDataPtrLocks().isLocked(*inoutRefPtr))
	{
		//Console_Warning(Console_WriteLine, "attempt to dispose of locked undoable action");
	}
	else
	{
		MyUndoDataAutoLocker	ptr(gUndoDataPtrLocks(), *inoutRefPtr);
		
		
		if (ptr != nullptr)
		{
			// locate the reference in all stacks and remove all occurrences
			Undoables_RemoveAction(*inoutRefPtr);
			
			// destroy the action data structure
			delete ptr;
		}
	}
}// DisposeAction


/*!
Adds the specified action to the top of the Undo stack.
The action is not allowed to exist multiple times in the
stack, so if the action exists elsewhere in the Undo
stack or in the Redo stack, it is first removed before
becoming the top of the Undo stack.

NOTE:	The only way to get items into the Redo stack
		is to call Undoables_UndoLastAction(), which
		migrates the top action in the Undo stack to
		the Redo stack.

(2.0)
*/
void
Undoables_AddAction		(Undoables_ActionRef	inActionToAdd)
{
	Undoables_RemoveAction(inActionToAdd);
	gUndoStack().push_front(inActionToAdd);
}// AddAction


/*!
Returns the ID of the specified action (given at
construction time), or "kInvalidUndoableContextIdentifier"
if the given reference is invalid.

You typically use this ID to help you figure out what
data the context of an action represents, from within
your undo callback routine.

(2.0)
*/
Undoables_ContextIdentifier
Undoables_ReturnActionID	(Undoables_ActionRef	inActionToAdd)
{
	Undoables_ContextIdentifier		result = kUndoables_ContextIdentifierInvalid;
	MyUndoDataAutoLocker			ptr(gUndoDataPtrLocks(), inActionToAdd);
	
	
	if (ptr != nullptr)
	{
		result = ptr->contextID;
	}
	return result;
}// ReturnActionID


/*!
Determines the state and correct text for the Redo
item in your application.  You really specify this
information yourself, when Undoables_AddAction() is
called.  This routine only keeps track of actions and
reports an accurate state based on whether *it* knows
how to handle Redo at the moment, and if so, exactly
what Redo command is going to be executed if you were
to call Undoables_RedoLastUndo().

(1.0)
*/
void
Undoables_GetRedoCommandInfo	(CFStringRef&	outCommandText,
								 Boolean*		outCommandEnabled)
{
	if (gRedoStack().empty())
	{
		outCommandText = gDefaultRedoName().returnCFStringRef();
	}
	else
	{
		Undoables_ActionRef		ref = gRedoStack().front();
		MyUndoDataAutoLocker	ptr(gUndoDataPtrLocks(), ref);
		
		
		if (ptr != nullptr) outCommandText = ptr->redoCommandName.returnCFStringRef();
		else outCommandText = gDefaultRedoName().returnCFStringRef();
	}
	if (outCommandEnabled != nullptr) *outCommandEnabled = !gRedoStack().empty();
}// GetRedoCommandInfo


/*!
Determines the state and correct text for the Undo
item in your application.  You really specify this
information yourself, when Undoables_AddAction() is
called.  This routine only keeps track of actions and
reports an accurate state based on whether *it* knows
how to handle Undo at the moment, and if so, exactly
what Undo command is going to be executed if you were
to call Undoables_UndoLastAction().

(1.0)
*/
void
Undoables_GetUndoCommandInfo	(CFStringRef&	outCommandText,
								 Boolean*		outCommandEnabled)
{
	if (gUndoStack().empty())
	{
		outCommandText = gDefaultUndoName().returnCFStringRef();
	}
	else
	{
		Undoables_ActionRef		ref = gUndoStack().front();
		MyUndoDataAutoLocker	ptr(gUndoDataPtrLocks(), ref);
		
		
		if (ptr != nullptr) outCommandText = ptr->undoCommandName.returnCFStringRef();
		else outCommandText = gDefaultUndoName().returnCFStringRef();
	}
	if (outCommandEnabled != nullptr) *outCommandEnabled = !gUndoStack().empty();
}// GetUndoCommandInfo


/*!
Invokes the top action on the Redo stack, and then
transfers that action to the top of the Undo stack.
If the resultant size of the Undo stack is beyond
the configured maximum, the action is automatically
removed and the dispose message is sent.

This should be the only action taken in response
to the user selecting Redo from your application’s
Edit menu.

(1.0)
*/
void
Undoables_RedoLastUndo ()
{
	unless (gRedoStack().empty())
	{
		Undoables_ActionRef		whatToDo = gRedoStack().front();
		
		
		// remove the action from the Redo stack
		gRedoStack().pop_front();
		
		// invoke the Redo operation
		{
			MyUndoDataAutoLocker	ptr(gUndoDataPtrLocks(), whatToDo);
			
			
			Undoables_InvokeActionProc(ptr->actionProc, kUndoables_ActionInstructionRedo,
										ptr->selfRef, ptr->contextPtr);
		}
		
		// set it up for undoing
		if (gUndoStackType == kUndoables_UndoHandlingMechanismOnlyOne)
		{
			// when not supporting multiple Redo, the stacks are not needed
			gUndoStack().clear();
			gRedoStack().clear();
		}
		gUndoStack().push_front(whatToDo);
	}
}// RedoLastUndo


/*!
Removes the specified action from whatever queue it
is in (Undo or Redo).  The reference remains valid;
you can re-use it by calling Undoables_AddAction().

(2.0)
*/
void
Undoables_RemoveAction	(Undoables_ActionRef	inActionToRemove)
{
	auto	stackIterator = std::find(gUndoStack().begin(), gUndoStack().end(), inActionToRemove);
	
	
	if (stackIterator == gUndoStack().end())
	{
		// not found in Undo stack; try the Redo stack
		stackIterator = std::find(gRedoStack().begin(), gRedoStack().end(), inActionToRemove);
		
		// remove all occurrences from the Redo stack
		while (stackIterator != gRedoStack().end())
		{
			gRedoStack().erase(stackIterator);
			stackIterator = std::find(gRedoStack().begin(), gRedoStack().end(), inActionToRemove);
		}
	}
	else
	{
		// remove all occurrences from the Undo stack
		while (stackIterator != gUndoStack().end())
		{
			// remove from Undo stack
			gUndoStack().erase(stackIterator);
			stackIterator = std::find(gUndoStack().begin(), gUndoStack().end(), inActionToRemove);
		}
	}
}// RemoveAction


/*!
Invokes the top action on the Undo stack, and then
transfers that action to the top of the Redo stack.

This should be the only action taken in response
to the user selecting Undo from your application’s
Edit menu.

(1.0)
*/
void
Undoables_UndoLastAction ()
{
	unless (gUndoStack().empty())
	{
		Undoables_ActionRef		whatToDo = gUndoStack().front();
		MyUndoDataAutoLocker	ptr(gUndoDataPtrLocks(), whatToDo);
		
		
		// remove the action from the Redo stack
		gUndoStack().pop_front();
		
		// invoke the Undo operation
		Undoables_InvokeActionProc(ptr->actionProc, kUndoables_ActionInstructionUndo,
									ptr->selfRef, ptr->contextPtr);
		
		// set it up for undoing
		if (gUndoStackType == kUndoables_UndoHandlingMechanismOnlyOne)
		{
			// when not supporting multiple Undo, the stacks are not needed
			gUndoStack().clear();
			gRedoStack().clear();
		}
		if (ptr->isRedoable)
		{
			gRedoStack().push_front(whatToDo);
		}
	}
}// UndoLastAction

// BELOW IS REQUIRED NEWLINE TO END FILE
