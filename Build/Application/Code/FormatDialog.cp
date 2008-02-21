/*###############################################################

	FormatDialog.cp
	
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

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <MemoryBlockPtrLocker.template.h>

// MacTelnet includes
#include "FormatDialog.h"
#include "GenericDialog.h"
#include "PrefPanelFormats.h"



#pragma mark Types
namespace {

struct My_FormatDialog
{
	My_FormatDialog		(HIWindowRef, Preferences_ContextRef, GenericDialog_CloseNotifyProcPtr);
	
	~My_FormatDialog	();
	
	FormatDialog_Ref		selfRef;			// identical to address of structure, but typed as ref
	Boolean					wasDisplayed;		// controls whose responsibility it is to destroy the Generic Dialog
	Preferences_ContextRef	dataModel;			// data used to initialize the dialog, and store any changes made
	GenericDialog_Ref		genericDialog;		// handles most of the work
};
typedef My_FormatDialog*		My_FormatDialogPtr;
typedef My_FormatDialogPtr*		My_FormatDialogHandle;

typedef MemoryBlockPtrLocker< FormatDialog_Ref, My_FormatDialog >	My_FormatDialogPtrLocker;
typedef LockAcquireRelease< FormatDialog_Ref, My_FormatDialog >		My_FormatDialogAutoLocker;

}// anonymous namespace

#pragma mark Variables
namespace {

My_FormatDialogPtrLocker&	gFormatDialogPtrLocks()  { static My_FormatDialogPtrLocker x; return x; }

}// anonymous namespace

#pragma mark Internal Method Prototypes



#pragma mark Public Methods

/*!
Creates a new Format Dialog.  This functions very much like the
Preferences panel, except it is in a modal dialog and any changes
are restricted to the specified preferences context.

(3.1)
*/
FormatDialog_Ref
FormatDialog_New	(HIWindowRef						inParentWindowOrNullForModalDialog,
					 Preferences_ContextRef				inoutData,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	FormatDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_FormatDialog(inParentWindowOrNullForModalDialog,
														inoutData, inCloseNotifyProcPtr), FormatDialog_Ref);
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

(3.1)
*/
void
FormatDialog_Dispose	(FormatDialog_Ref*	inoutRefPtr)
{
	if (gFormatDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked format dialog; outstanding locks",
							gFormatDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_FormatDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
This method displays and handles events in the
Format dialog box.  If the user clicks OK,
"true" is returned; otherwise, "false" is
returned.

(3.0)
*/
void
FormatDialog_Display	(FormatDialog_Ref	inDialog)
{
	My_FormatDialogAutoLocker	ptr(gFormatDialogPtrLocks(), inDialog);
	
	
	if (ptr == nullptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		ptr->wasDisplayed = true;
		GenericDialog_Display(ptr->genericDialog);
	}
}// Display


#pragma mark Internal Methods

/*!
Constructor.  See FormatDialog_New().

(3.1)
*/
My_FormatDialog::
My_FormatDialog		(HIWindowRef						inParentWindowOrNullForModalDialog,
					 Preferences_ContextRef				inoutData,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef				(REINTERPRET_CAST(this, FormatDialog_Ref)),
wasDisplayed		(false),
dataModel			(inoutData),
genericDialog		(GenericDialog_New(inParentWindowOrNullForModalDialog, PrefPanelFormats_New(),
										inCloseNotifyProcPtr, kHelpSystem_KeyPhraseFormatting))
{
	Preferences_RetainContext(this->dataModel);
}// My_FormatDialog 2-argument constructor


/*!
Destructor.  See GenericDialog_Dispose().

(3.1)
*/
My_FormatDialog::
~My_FormatDialog ()
{
	Preferences_ReleaseContext(&this->dataModel);
	
	// if the dialog is displayed, then Generic Dialog takes over responsibility to dispose of it
	if (false == this->wasDisplayed) GenericDialog_Dispose(&this->genericDialog);
}// My_FormatDialog destructor

// BELOW IS REQUIRED NEWLINE TO END FILE
