/*###############################################################

	NewSessionDialog.cp
	
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
#include <SoundSystem.h>

// MacTelnet includes
#include "GenericDialog.h"
#include "PrefPanelSessions.h"
#include "NewSessionDialog.h"
#include "Preferences.h"
#include "SessionFactory.h"
#include "SessionRef.typedef.h"



#pragma mark Types
namespace {

struct My_NewSessionDialog
{
	My_NewSessionDialog		(TerminalWindowRef, Preferences_ContextRef);
	
	~My_NewSessionDialog	();
	
	NewSessionDialog_Ref		selfRef;			// identical to address of structure, but typed as ref
	TerminalWindowRef			terminalWindow;		// the terminal in which to place new sessions, if any
	Boolean						wasDisplayed;		// controls whose responsibility it is to destroy the Generic Dialog
	Preferences_ContextRef		originalDataModel;	// data used to initialize the dialog, and store any changes made
	Preferences_ContextRef		temporaryDataModel;	// data used to initialize the dialog, and store any changes made
	GenericDialog_Ref			genericDialog;		// handles most of the work
};
typedef My_NewSessionDialog*	My_NewSessionDialogPtr;

typedef MemoryBlockPtrLocker< NewSessionDialog_Ref, My_NewSessionDialog >	My_NewSessionDialogPtrLocker;
typedef LockAcquireRelease< NewSessionDialog_Ref, My_NewSessionDialog >		My_NewSessionDialogAutoLocker;

}// anonymous namespace

#pragma mark Variables
namespace {

My_NewSessionDialogPtrLocker&	gNewSessionDialogPtrLocks()	{ static My_NewSessionDialogPtrLocker x; return x; }

}// anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void	handleDialogClose	(GenericDialog_Ref, Boolean);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a New Session Dialog.  This functions very much like the
Preferences panel, except it is in a modal dialog and the result
of accepting the dialog is to create a new session.

The context defines the initial state of the dialog, and should
generally contain Session-class keys.

See NewSessionDialog_Display() for more information on actions
that occur when the dialog is closed.

(3.0)
*/
NewSessionDialog_Ref
NewSessionDialog_New	(TerminalWindowRef			inParentWindowOrNullForModalDialog,
						 Preferences_ContextRef		inData)
{
	NewSessionDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_NewSessionDialog(inParentWindowOrNullForModalDialog,
															inData), NewSessionDialog_Ref);
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

(3.0)
*/
void
NewSessionDialog_Dispose	(NewSessionDialog_Ref*	inoutRefPtr)
{
	if (gNewSessionDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked new session dialog; outstanding locks",
						gNewSessionDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_NewSessionDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Displays a sheet allowing the user to run a command in a
terminal window, and returns immediately.  If the dialog is confirmed, the
Preferences context given at construction time is updated
automatically; otherwise, changes are discarded.

(3.0)
*/
void
NewSessionDialog_Display	(NewSessionDialog_Ref		inDialog)
{
	My_NewSessionDialogAutoLocker	ptr(gNewSessionDialogPtrLocks(), inDialog);
	
	
	if (ptr == nullptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		ptr->wasDisplayed = true;
		GenericDialog_Display(ptr->genericDialog);
	}
}// Display


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See NewSessionDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
My_NewSessionDialog::
My_NewSessionDialog	(TerminalWindowRef			inParentWindowOrNullForModalDialog,
					 Preferences_ContextRef		inData)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef				(REINTERPRET_CAST(this, NewSessionDialog_Ref)),
terminalWindow		(inParentWindowOrNullForModalDialog),
wasDisplayed		(false),
originalDataModel	(inData),
temporaryDataModel	(Preferences_NewCloneContext(originalDataModel, true/* must detach */)),
genericDialog		(GenericDialog_New(TerminalWindow_ReturnWindow(inParentWindowOrNullForModalDialog),
										PrefPanelSessions_NewResourcePane(), temporaryDataModel,
										handleDialogClose, kHelpSystem_KeyPhraseConnections))
{
	// configure the dialog
	GenericDialog_SetCommandDialogEffect(this->genericDialog, kHICommandCancel, kGenericDialog_DialogEffectCloseImmediately);
	
	// note that the cloned context is implicitly retained
	Preferences_RetainContext(this->originalDataModel);
	
	// remember reference for use in the callback
	GenericDialog_SetImplementation(genericDialog, this);
}// My_NewSessionDialog 2-argument constructor


/*!
Destructor.  See NewSessionDialog_Dispose().

(3.1)
*/
My_NewSessionDialog::
~My_NewSessionDialog ()
{
	Preferences_ReleaseContext(&this->originalDataModel);
	Preferences_ReleaseContext(&this->temporaryDataModel);
	
	// if the dialog is displayed, then Generic Dialog takes over responsibility to dispose of it
	if (false == this->wasDisplayed) GenericDialog_Dispose(&this->genericDialog);
}// MyNewSessionDialog destructor


/*!
Responds to a close of a New Session Dialog by creating a
new session.

(3.1)
*/
void
handleDialogClose	(GenericDialog_Ref		inDialogThatClosed,
					 Boolean				inOKButtonPressed)
{
	My_NewSessionDialogPtr		dataPtr = REINTERPRET_CAST(GenericDialog_ReturnImplementation(inDialogThatClosed), My_NewSessionDialogPtr);
	
	
	if (inOKButtonPressed)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		CFArrayRef				argumentListCFArray = nullptr;
		
		
		// create a session; this information is expected to be defined by
		// the panel as the user makes edits in the user interface
		prefsResult = Preferences_ContextGetData(dataPtr->temporaryDataModel, kPreferences_TagCommandLine,
													sizeof(argumentListCFArray), &argumentListCFArray);
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "unexpected problem retrieving the command line from the panel!");
		}
		else
		{
			// dump the process into the terminal window associated with the dialog
			SessionRef		session = nullptr;
			
			
			session = SessionFactory_NewSessionArbitraryCommand
						(dataPtr->terminalWindow/* could be nullptr to make a new window */, argumentListCFArray,
							dataPtr->temporaryDataModel);
			CFRelease(argumentListCFArray), argumentListCFArray = nullptr;
		}
	}
	else
	{
		// immediately destroy the parent terminal window
		if (nullptr != dataPtr->terminalWindow) TerminalWindow_Dispose(&dataPtr->terminalWindow);
	}
}// handleDialogClose

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
