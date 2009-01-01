/*###############################################################

	PrefsContextDialog.cp
	
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

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <MemoryBlockPtrLocker.template.h>

// MacTelnet includes
#include "GenericDialog.h"
#include "PrefsContextDialog.h"



#pragma mark Types
namespace {

struct My_PrefsContextDialog
{
	My_PrefsContextDialog	(HIWindowRef, Panel_Ref, Preferences_ContextRef);
	
	~My_PrefsContextDialog	();
	
	PrefsContextDialog_Ref		selfRef;			// identical to address of structure, but typed as ref
	Boolean						wasDisplayed;		// controls whose responsibility it is to destroy the Generic Dialog
	Preferences_ContextRef		originalDataModel;	// data used to initialize the dialog, and store any changes made
	Preferences_ContextRef		temporaryDataModel;	// data used to initialize the dialog, and store any changes made
	GenericDialog_Ref			genericDialog;		// handles most of the work
};
typedef My_PrefsContextDialog*		My_PrefsContextDialogPtr;

typedef MemoryBlockPtrLocker< PrefsContextDialog_Ref, My_PrefsContextDialog >	My_PrefsContextDialogPtrLocker;
typedef LockAcquireRelease< PrefsContextDialog_Ref, My_PrefsContextDialog >		My_PrefsContextDialogAutoLocker;

}// anonymous namespace

#pragma mark Variables
namespace {

My_PrefsContextDialogPtrLocker&		gPrefsContextDialogPtrLocks()  { static My_PrefsContextDialogPtrLocker x; return x; }

}// anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void	handleDialogClose	(GenericDialog_Ref, Boolean);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new window-modal sheet to host the specified panel.

Unlike Generic Dialog, this module has specific knowledge of
how to deal with panels that edit standard Preferences Contexts:
namely, if the user commits changes to the dialog, all keys
from the temporary context are automatically copied into the
original source (and otherwise, they are discarded).

You can therefore pass in any panel that might otherwise work
in the Preferences window, and use it as a sheet to edit some
temporary settings!

See PrefsContextDialog_Display() for more information on actions
that occur when the dialog is closed.

(4.0)
*/
PrefsContextDialog_Ref
PrefsContextDialog_New	(HIWindowRef				inParentWindowOrNullForModalDialog,
						 Panel_Ref					inHostedPanel,
						 Preferences_ContextRef		inoutData)
{
	PrefsContextDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_PrefsContextDialog(inParentWindowOrNullForModalDialog, inHostedPanel,
															inoutData), PrefsContextDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Destroys the dialog box and its associated data structures.
On return, your copy of the dialog reference is set to
nullptr.

(4.0)
*/
void
PrefsContextDialog_Dispose	(PrefsContextDialog_Ref*	inoutRefPtr)
{
	if (gPrefsContextDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked preferences context dialog; outstanding locks",
							gPrefsContextDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_PrefsContextDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
This method displays and asynchronously handles events in
the given dialog box.  If the dialog is confirmed, the
Preferences context given at construction time is updated
automatically; otherwise, changes are discarded.

(4.0)
*/
void
PrefsContextDialog_Display	(PrefsContextDialog_Ref		inDialog)
{
	My_PrefsContextDialogAutoLocker		ptr(gPrefsContextDialogPtrLocks(), inDialog);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		ptr->wasDisplayed = true;
		GenericDialog_Display(ptr->genericDialog);
	}
}// Display


/*!
Returns the Generic Dialog used to implement this interface.
You can use this reference to further customize the dialog in
a variety of ways; see "GenericDialog.h".

(4.0)
*/
GenericDialog_Ref
PrefsContextDialog_ReturnGenericDialog	(PrefsContextDialog_Ref		inDialog)
{
	My_PrefsContextDialogAutoLocker		ptr(gPrefsContextDialogPtrLocks(), inDialog);
	GenericDialog_Ref					result = nullptr;
	
	
	if (nullptr != ptr)
	{
		result = ptr->genericDialog;
	}
	return result;
}// ReturnGenericDialog


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See PrefsContextDialog_New().

(4.0)
*/
My_PrefsContextDialog::
My_PrefsContextDialog	(HIWindowRef				inParentWindowOrNullForModalDialog,
						 Panel_Ref					inHostedPanel,
						 Preferences_ContextRef		inoutData)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef				(REINTERPRET_CAST(this, PrefsContextDialog_Ref)),
wasDisplayed		(false),
originalDataModel	(inoutData),
temporaryDataModel	(Preferences_NewCloneContext(inoutData, true/* must detach */)),
genericDialog		(GenericDialog_New(inParentWindowOrNullForModalDialog, inHostedPanel,
										temporaryDataModel, handleDialogClose,
										Panel_SendMessageGetHelpKeyPhrase(inHostedPanel)))
{
	// note that the cloned context is implicitly retained
	Preferences_RetainContext(this->originalDataModel);
	
	// remember reference for use in the callback
	GenericDialog_SetImplementation(genericDialog, this);
}// My_PrefsContextDialog 2-argument constructor


/*!
Destructor.  See PrefsContextDialog_Dispose().

(4.0)
*/
My_PrefsContextDialog::
~My_PrefsContextDialog ()
{
	Preferences_ReleaseContext(&this->originalDataModel);
	Preferences_ReleaseContext(&this->temporaryDataModel);
	
	// if the dialog is displayed, then Generic Dialog takes over responsibility to dispose of it
	if (false == this->wasDisplayed) GenericDialog_Dispose(&this->genericDialog);
}// My_PrefsContextDialog destructor


/*!
Responds to a close of a Preferences Context Dialog by
committing changes (if OK was pressed) between the temporary
context and the original context.

(4.0)
*/
void
handleDialogClose	(GenericDialog_Ref		inDialogThatClosed,
					 Boolean				inOKButtonPressed)
{
	My_PrefsContextDialogPtr	dataPtr = REINTERPRET_CAST(GenericDialog_ReturnImplementation(inDialogThatClosed), My_PrefsContextDialogPtr);
	
	
	if (inOKButtonPressed)
	{
		// copy temporary context key-value pairs back into original context
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextCopy(dataPtr->temporaryDataModel, dataPtr->originalDataModel);
	}
}// handleDialogClose

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
