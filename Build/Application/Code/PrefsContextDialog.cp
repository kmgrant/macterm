/*###############################################################

	PrefsContextDialog.cp
	
	MacTelnet
		© 1998-2011 by Kevin Grant.
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
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <MemoryBlockPtrLocker.template.h>
#include <SoundSystem.h>

// MacTelnet includes
#include "GenericDialog.h"
#include "Panel.h"
#include "PrefsContextDialog.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Types
namespace {

struct My_PrefsContextDialog
{
	My_PrefsContextDialog	(HIWindowRef, Panel_Ref, Preferences_ContextRef, PrefsContextDialog_DisplayOptions,
							 GenericDialog_CloseNotifyProcPtr);
	
	~My_PrefsContextDialog	();
	
	PrefsContextDialog_Ref				selfRef;			// identical to address of structure, but typed as ref
	Boolean								wasDisplayed;		// controls whose responsibility it is to destroy the Generic Dialog
	Preferences_ContextWrap				originalDataModel;	// data used to initialize the dialog; updated when the dialog is accepted
	Preferences_TagSetRef				originalKeys;		// the set of keys that were defined in the original data model, before user changes
	Preferences_ContextWrap				temporaryDataModel;	// data used to initialize the dialog, and store any changes made
	GenericDialog_Ref					genericDialog;		// handles most of the work
	GenericDialog_CloseNotifyProcPtr	closeNotifyProc;	// routine to call when the dialog is dismissed
	CarbonEventHandlerWrap				commandHandler;		// responds to certain command events in the panel’s window
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

void		handleDialogClose	(GenericDialog_Ref, Boolean);
OSStatus	receiveHICommand	(EventHandlerCallRef, EventRef, void*);

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
PrefsContextDialog_New	(HIWindowRef						inParentWindowOrNullForModalDialog,
						 Panel_Ref							inHostedPanel,
						 Preferences_ContextRef				inoutData,
						 PrefsContextDialog_DisplayOptions	inOptions,
						 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	PrefsContextDialog_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_PrefsContextDialog(inParentWindowOrNullForModalDialog, inHostedPanel,
															inoutData, inOptions, inCloseNotifyProcPtr), PrefsContextDialog_Ref);
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
		Console_Warning(Console_WriteValue, "attempt to dispose of locked preferences context dialog; outstanding locks",
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
My_PrefsContextDialog	(HIWindowRef						inParentWindowOrNullForModalDialog,
						 Panel_Ref							inHostedPanel,
						 Preferences_ContextRef				inoutData,
						 PrefsContextDialog_DisplayOptions	inOptions,
						 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef				(REINTERPRET_CAST(this, PrefsContextDialog_Ref)),
wasDisplayed		(false),
originalDataModel	(inoutData),
originalKeys		(Preferences_NewTagSet(inoutData)),
temporaryDataModel	(Preferences_NewCloneContext(inoutData, true/* must detach */), true/* is retained */),
genericDialog		(GenericDialog_New(inParentWindowOrNullForModalDialog, inHostedPanel,
										temporaryDataModel.returnRef(), handleDialogClose,
										Panel_SendMessageGetHelpKeyPhrase(inHostedPanel))),
closeNotifyProc		(inCloseNotifyProcPtr),
commandHandler		(GetWindowEventTarget(Panel_ReturnOwningWindow(GenericDialog_ReturnHostedPanel(genericDialog))),
						receiveHICommand,
						CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
						this/* user data */)
{
	if (0 == (inOptions & kPrefsContextDialog_DisplayOptionNoAddToPrefsButton))
	{
		// add a button for copying the context to user defaults
		CFStringRef			buttonCFString = nullptr;
		UIStrings_Result	stringResult = UIStrings_Copy
											(kUIStrings_PreferencesWindowAddToFavoritesButton, buttonCFString);
		
		
		if (stringResult.ok())
		{
			// this command is received by the local handler
			GenericDialog_AddButton(genericDialog, buttonCFString, kCommandPreferencesNewFavorite);
			CFRelease(buttonCFString), buttonCFString = nullptr;
		}
	}
	
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
	Preferences_ReleaseTagSet(&this->originalKeys);
	
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
	My_PrefsContextDialogPtr	dataPtr = REINTERPRET_CAST(GenericDialog_ReturnImplementation(inDialogThatClosed),
															My_PrefsContextDialogPtr);
	
	
	if (inOKButtonPressed)
	{
		// copy temporary context key-value pairs back into original context;
		// since it is possible for tags to be deleted, first copy all of
		// the defaults for the keys that were originally in place, then
		// copy the new values “on top”; the net result is a correct set
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		Preferences_ContextRef	defaultContext = nullptr;
		
		
		// start with the defaults
		prefsResult = Preferences_GetDefaultContext(&defaultContext, Preferences_ContextReturnClass
																		(dataPtr->originalDataModel.returnRef()));
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "unable to find default context");
		}
		else
		{
			// TEMPORARY - technically, defaults only need to be copied for the
			// keys that are actually different between the current set and
			// the original set; but for now, just copy everything
			prefsResult = Preferences_ContextCopy(defaultContext, dataPtr->originalDataModel.returnRef(),
													dataPtr->originalKeys);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "unable to save defaults to context");
			}
		}
		
		// copy the new settings on top
		{
			Preferences_TagSetRef	currentKeys = Preferences_NewTagSet(dataPtr->temporaryDataModel.returnRef());
			
			
			if (nullptr == currentKeys)
			{
				Console_Warning(Console_WriteLine, "unable to create key list for context changes");
			}
			else
			{
				prefsResult = Preferences_ContextCopy(dataPtr->temporaryDataModel.returnRef(),
														dataPtr->originalDataModel.returnRef(), currentKeys);
				Preferences_ReleaseTagSet(&currentKeys);
			}
		}
	}
	
	// now call any user-provided routine
	if (nullptr != dataPtr->closeNotifyProc)
	{
		GenericDialog_InvokeCloseNotifyProc(dataPtr->closeNotifyProc, inDialogThatClosed, inOKButtonPressed);
	}
}// handleDialogClose


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for this sheet.

(3.1)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inContextPtr)
{
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	My_PrefsContextDialog*	dataPtr = REINTERPRET_CAST(inContextPtr, My_PrefsContextDialog*);
	HICommand				received;
	OSStatus				result = eventNotHandledErr;
	
	
	// determine the command in question
	result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
	
	// if the command information was found, proceed
	if (noErr == result)
	{
		// don’t claim to have handled any commands not shown below
		result = eventNotHandledErr;
		
		switch (kEventKind)
		{
		case kEventCommandProcess:
			// execute a command
			switch (received.commandID)
			{
			case kCommandPreferencesNewFavorite:
				// in this context, this command actually means “new favorite
				// using the settings from this window”
				{
					// create and immediately save a new named context, which
					// triggers callbacks to update Favorites lists, etc.
					Preferences_ContextRef const	kReferenceContext = dataPtr->temporaryDataModel.returnRef();
					Preferences_ContextWrap			newContext(Preferences_NewContextFromFavorites
																(Preferences_ContextReturnClass(kReferenceContext),
																	nullptr/* name, or nullptr to auto-generate */),
																true/* is retained */);
					
					
					if (false == newContext.exists())
					{
						Sound_StandardAlert();
						Console_Warning(Console_WriteLine, "unable to create a new Favorite for copying local changes");
					}
					else
					{
						Preferences_Result		prefsResult = kPreferences_ResultOK;
						
						
						prefsResult = Preferences_ContextSave(newContext.returnRef());
						if (kPreferences_ResultOK != prefsResult)
						{
							Sound_StandardAlert();
							Console_Warning(Console_WriteLine, "unable to save the new context!");
						}
						else
						{
							prefsResult = Preferences_ContextCopy(kReferenceContext, newContext.returnRef());
							if (kPreferences_ResultOK != prefsResult)
							{
								Sound_StandardAlert();
								Console_Warning(Console_WriteLine, "unable to copy local changes into the new Favorite!");
							}
							else
							{
								// trigger an automatic switch to focus a related part of the Preferences window
								Commands_ExecuteByIDUsingEvent
								(Panel_ReturnShowCommandID(GenericDialog_ReturnHostedPanel(dataPtr->genericDialog)));
								
								// success!
							}
						}
					}
					result = noErr;
				}
				break;
			
			default:
				// ???
				break;
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	return result;
}// receiveHICommand

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
