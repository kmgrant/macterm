/*!	\file GenericDialog.mm
	\brief Allows a user interface that is both a panel
	and a dialog to be displayed as a modal dialog or
	sheet.
	
	Note that this module is in transition and is not yet a
	Cocoa-only user interface.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#import "GenericDialog.h"
#import <UniversalDefines.h>

// standard-C includes
#import <climits>
#import <cstdlib>
#import <cstring>

// standard-C++ includes
#import <map>
#import <utility>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>
#import <Popover.objc++.h>
#import <PopoverManager.objc++.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "Session.h"
#import "Terminology.h"
#import "UIStrings.h"



#pragma mark Types
namespace {

typedef std::map< UInt32, GenericDialog_DialogEffect >		My_DialogEffectsByCommandID;

struct My_GenericDialog
{
	My_GenericDialog	(NSWindow*, HIWindowRef, Panel_ViewManager*, void*,
						 GenericDialog_CloseNotifyProcPtr);
	
	~My_GenericDialog	();
	
	void
	loadViewManager ();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	GenericDialog_Ref						selfRef;						//!< identical to address of structure, but typed as ref
	NSWindow*								parentWindow;					//!< the terminal window for which this dialog applies
	HIWindowRef								parentCarbonWindow;				//!< legacy; if parent is a Carbon window, specify it here
	Boolean									isModal;						//!< if false, the dialog is a sheet
	GenericDialog_ViewManager*				containerViewManager;			//!< new-style; object for rendering user interface around primary view (such as OK and Cancel buttons)
	Panel_ViewManager*						hostedViewManager;				//!< new-style; the Cocoa view manager for the primary user interface
	HISize									panelIdealSize;					//!< the dimensions most appropriate for displaying the UI
	PopoverManager_Ref						popoverManager;					//!< object to help display popover window
	Popover_Window*							popoverWindow;					//!< new-style, popover variant; contains a Cocoa view in a popover frame
	GenericDialog_CloseNotifyProcPtr		closeNotifyProc;				//!< routine to call when the dialog is dismissed
	My_DialogEffectsByCommandID				closeEffects;					//!< custom sheet-closing effects for certain commands
	void*									userDataPtr;					//!< optional; external data
};
typedef My_GenericDialog*			My_GenericDialogPtr;
typedef My_GenericDialog const*		My_GenericDialogConstPtr;

typedef MemoryBlockPtrLocker< GenericDialog_Ref, My_GenericDialog >		My_GenericDialogPtrLocker;
typedef LockAcquireRelease< GenericDialog_Ref, My_GenericDialog >		My_GenericDialogAutoLocker;

} // anonymous namespace


/*!
The private class interface.
*/
@interface GenericDialog_ViewManager (GenericDialog_ViewManagerInternal) //{

// new methods
	- (void)
	setStringProperty:(NSString**)_
	withName:(NSString*)_
	toValue:(NSString*)_;
	- (void)
	updateButtonLayout;

@end //}

#pragma mark Variables
namespace {

My_GenericDialogPtrLocker&		gGenericDialogPtrLocks ()		{ static My_GenericDialogPtrLocker x; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
This method is used to create a Cocoa-based popover view.

The format of "inDataSetPtr" is entirely defined by the
type of panel that the dialog is hosting.  The data is
passed to the panel with Panel_SendMessageNewDataSet().

If "inParentWindowOrNullForModalDialog" is nullptr, the
window is automatically made application-modal; otherwise,
it is a sheet.

(4.1)
*/
GenericDialog_Ref
GenericDialog_New	(HIWindowRef						inParentWindowOrNullForModalDialog,
					 Panel_ViewManager*					inHostedViewManager,
					 void*								inDataSetPtr,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	GenericDialog_Ref	result = nullptr;
	
	
	assert(nil != inHostedViewManager);
	
	try
	{
		result = REINTERPRET_CAST(new My_GenericDialog
									(CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(inParentWindowOrNullForModalDialog),
										inParentWindowOrNullForModalDialog, inHostedViewManager,
										inDataSetPtr, inCloseNotifyProcPtr),
									GenericDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Call this method to destroy a dialog box and its associated
data structures.  On return, your copy of the dialog reference
is set to nullptr.

(3.1)
*/
void
GenericDialog_Dispose	(GenericDialog_Ref*		inoutRefPtr)
{
	if (gGenericDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked generic dialog; outstanding locks",
						gGenericDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_GenericDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Specifies that there should be a third button with a custom
action attached.  The given block is run on the main thread
when the button is clicked.

The button is automatically sized to be large enough for the
given title string, and automatically placed to the left of
other command buttons.

Currently, this will not work for more than one extra button.

(4.1)
*/
void
GenericDialog_AddButton		(GenericDialog_Ref		inDialog,
							 CFStringRef			inButtonTitle,
							 void					(^inResponseBlock)())
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (nil != ptr->hostedViewManager)
	{
		// new way (Cocoa)
		ptr->loadViewManager();
		assert(nil != ptr->containerViewManager);
		ptr->containerViewManager.thirdButtonName = BRIDGE_CAST(inButtonTitle, NSString*);
		ptr->containerViewManager.thirdButtonBlock = inResponseBlock;
	}
	else
	{
		// not supported in legacy Carbon mode
		Console_Warning(Console_WriteLine, "GenericDialog_AddButton() not supported on Carbon-based dialogs anymore");
	}
}// AddButton


/*!
Displays the dialog.  If the dialog is modal, this call will
block until the dialog is finished.  Otherwise, a sheet will
appear over the parent window and this call will return
immediately.

A view with the specified ID must exist in the window, to be
the initial keyboard focus.

IMPORTANT:	Invoking this routine means it is no longer your
			responsibility to call GenericDialog_Dispose():
			this is done at an appropriate time after the
			user closes the dialog and after your notification
			routine has been called.

(3.1)
*/
void
GenericDialog_Display	(GenericDialog_Ref		inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (nullptr == ptr) Alert_ReportOSStatus(paramErr);
	else
	{
		if (nil != ptr->hostedViewManager)
		{
			//
			// new method: use Cocoa
			//
			
			Boolean		noAnimations = false;
			
			
			// determine if animation should occur
			unless (kPreferences_ResultOK ==
					Preferences_GetData(kPreferences_TagNoAnimations,
										sizeof(noAnimations), &noAnimations))
			{
				noAnimations = false; // assume a value, if preference can’t be found
			}
			
			ptr->loadViewManager();
			assert(nil != ptr->containerViewManager);
			
			if (nil == ptr->popoverWindow)
			{
				ptr->popoverWindow = [[Popover_Window alloc] initWithView:ptr->containerViewManager.managedView
									  										style:kPopover_WindowStyleDialogSheet
																			attachedToPoint:NSMakePoint(0, 0)/* TEMPORARY */
																			inWindow:ptr->parentWindow];
			}
			
			if (nil == ptr->popoverManager)
			{
				ptr->popoverManager = PopoverManager_New(ptr->popoverWindow,
															[ptr->containerViewManager logicalFirstResponder],
															ptr->containerViewManager/* delegate */,
															(noAnimations)
															? kPopoverManager_AnimationTypeNone
															: kPopoverManager_AnimationTypeStandard,
															kPopoverManager_BehaviorTypeDialog,
															ptr->parentCarbonWindow);
			}
			
			PopoverManager_DisplayPopover(ptr->popoverManager);
		}
		else
		{
			assert(false && "generic dialog expected a hosted view manager");
		}
	}
}// Display


/*!
Returns whatever was set with GenericDialog_SetImplementation,
or nullptr.

(3.1)
*/
void*
GenericDialog_ReturnImplementation	(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	void*						result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->userDataPtr;
	
	return result;
}// ReturnImplementation


/*!
Returns the view manager assigned when the dialog was
constructed.

(4.1)
*/
Panel_ViewManager*
GenericDialog_ReturnViewManager		(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	Panel_ViewManager*			result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->hostedViewManager;
	
	return result;
}// ReturnViewManager


/*!
Specifies a new name for the button corresponding to the
given command.  The specified button is automatically
resized to fit the title, and other action buttons are
moved (and possibly resized) to make room.

Use kHICommandOK to refer to the default button.  No other
command IDs are currently supported!

IMPORTANT:	This API currently only works for the
			standard buttons, not custom command IDs.

(4.0)
*/
void
GenericDialog_SetCommandButtonTitle		(GenericDialog_Ref		inDialog,
										 UInt32					inCommandID,
										 CFStringRef			inButtonTitle)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (nil != ptr->hostedViewManager)
	{
		// new method: use Cocoa
		if (kHICommandOK == inCommandID)
		{
			ptr->loadViewManager();
			assert(nil != ptr->containerViewManager);
			ptr->containerViewManager.primaryActionButtonName = BRIDGE_CAST(inButtonTitle, NSString*);
		}
	}
	else
	{
		assert(false && "generic dialog expected a hosted view manager");
	}
}// SetCommandButtonTitle


/*!
Specifies the effect that a command has on the dialog.

Use kHICommandOK and kHICommandCancel to refer to the
two standard buttons.

Note that by default, the OK and Cancel buttons have the
effect of "kGenericDialog_DialogEffectCloseNormally", and
any extra buttons have no effect at all (that is,
"kGenericDialog_DialogEffectNone").

IMPORTANT:	This API currently only works for the
			standard buttons, not custom command IDs.

(3.1)
*/
void
GenericDialog_SetCommandDialogEffect	(GenericDialog_Ref				inDialog,
										 UInt32							inCommandID,
										 GenericDialog_DialogEffect		inEffect)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->closeEffects[inCommandID] = inEffect;
}// SetCommandDialogEffect


/*!
Associates arbitrary user data with your dialog.
Retrieve with GenericDialog_ReturnImplementation().

(3.1)
*/
void
GenericDialog_SetImplementation		(GenericDialog_Ref	inDialog,
									 void*				inContext)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->userDataPtr = inContext;
}// SetImplementation


/*!
The default handler for closing a dialog.

(3.1)
*/
void
GenericDialog_StandardCloseNotifyProc	(GenericDialog_Ref	UNUSED_ARGUMENT(inDialogThatClosed),
										 Boolean			UNUSED_ARGUMENT(inOKButtonPressed))
{
	// do nothing
}// StandardCloseNotifyProc


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See GenericDialog_New().

Note that this is constructed in extreme object-oriented
fashion!  Literally the entire construction occurs within
the initialization list.  This design is unusual, but it
forces good object design.

(3.1)
*/
My_GenericDialog::
My_GenericDialog	(NSWindow*							inParentNSWindowOrNull,
					 HIWindowRef						inParentCarbonWindowOrNull,
					 Panel_ViewManager*					inHostedViewManagerOrNull,
					 void*								inDataSetPtr,
					 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, GenericDialog_Ref)),
parentWindow					(inParentNSWindowOrNull),
parentCarbonWindow				(inParentCarbonWindowOrNull),
isModal							(nil == inParentNSWindowOrNull),
containerViewManager			(nil), // set later if necessary
hostedViewManager				([inHostedViewManagerOrNull retain]),
panelIdealSize					(CGSizeMake(0, 0)), // set later
popoverManager					(nullptr), // created as needed
popoverWindow					(nil), // set later if necessary
closeNotifyProc					(inCloseNotifyProcPtr),
closeEffects					(),
userDataPtr						(nullptr)
{
	// now notify the panel of its data
	[inHostedViewManagerOrNull.delegate panelViewManager:inHostedViewManagerOrNull
															didChangeFromDataSet:nullptr
															toDataSet:inDataSetPtr];
}// My_GenericDialog constructor


/*!
Destructor.  See GenericDialog_Dispose().

(3.1)
*/
My_GenericDialog::
~My_GenericDialog ()
{
	if (nullptr != popoverManager)
	{
		PopoverManager_RemovePopover(popoverManager, false/* is confirming */);
		PopoverManager_Dispose(&popoverManager);
	}
	
	if (nil != popoverWindow)
	{
		[popoverWindow release];
	}
	
	[hostedViewManager release];
}// My_GenericDialog destructor


/*!
Constructs the internal view manager object if necessary,
and transfers ownership of this object to that manager.

Cocoa only.

(4.1)
*/
void
My_GenericDialog::
loadViewManager ()
{
	if (nil == this->containerViewManager)
	{
		this->containerViewManager = [[GenericDialog_ViewManager alloc]
										initWithRef:this->selfRef/* transfer ownership here */
														identifier:[this->hostedViewManager panelIdentifier]
														localizedName:[this->hostedViewManager panelName]
														localizedIcon:[this->hostedViewManager panelIcon]
														viewManager:this->hostedViewManager];
	}
}// loadViewManager

} // anonymous namespace


#pragma mark -
@implementation GenericDialog_ViewManager


@synthesize thirdButtonBlock = _thirdButtonBlock;


#pragma mark Initializers

/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithRef:(GenericDialog_Ref)		aDialogRef
identifier:(NSString*)				anIdentifier
localizedName:(NSString*)			aName
localizedIcon:(NSImage*)			anImage
viewManager:(Panel_ViewManager*)	aViewManager
{
	self = [super initWithNibNamed:@"GenericDialogCocoa"
									delegate:self
									context:@{
												@"identifier": anIdentifier,
												@"localizedName": aName,
												@"localizedIcon": anImage,
												@"viewManager": aViewManager,
											}];
	if (nil != self)
	{
		self->dialogRef = aDialogRef;
		
		aViewManager.panelParent = self;
		
		// set an initial button value for bindings to use
		self.primaryActionButtonName = [BRIDGE_CAST(UIStrings_ReturnCopy(kUIStrings_ButtonOK), NSString*) autorelease];
		
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// initWithRef:identifier:localizedName:localizedIcon:viewManager:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
	
	GenericDialog_Dispose(&dialogRef);
	mainViewManager.panelParent = nil;
	[identifier release];
	[localizedName release];
	[localizedIcon release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
primaryActionButtonName
{
	return [[self->_primaryActionButtonName copy] autorelease];
}
- (void)
setPrimaryActionButtonName:(NSString*)		aString
{
	[self setStringProperty:&_primaryActionButtonName withName:@"primaryActionButtonName" toValue:aString];
	
	// NOTE: UI updates are OK here because there are no
	// bindings that can be set ahead of construction
	// (initialization occurs at view-load time); otherwise
	// it would be necessary to keep track of pending updates
	[self updateButtonLayout];
}// setPrimaryActionButtonName:


/*!
Accessor.

(4.1)
*/
- (NSString*)
thirdButtonName
{
	return [[self->_thirdButtonName copy] autorelease];
}
- (void)
setThirdButtonName:(NSString*)		aString
{
	[self setStringProperty:&_thirdButtonName withName:@"thirdButtonName" toValue:aString];
	
	// NOTE: UI updates are OK here because there are no
	// bindings that can be set ahead of construction
	// (initialization occurs at view-load time); otherwise
	// it would be necessary to keep track of pending updates
	[self updateButtonLayout];
}// setThirdButtonName:


#pragma mark Actions


/*!
Invoked when the user clicks the “third button” in the dialog.
See the "thirdButtonName" property.

(4.1)
*/
- (IBAction)
performThirdButtonAction:(id)	sender
{
#pragma unused(sender)
	if (nil != self.thirdButtonBlock)
	{
		self.thirdButtonBlock();
	}
}// performThirdButtonAction


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(void*)			aContext
{
#pragma unused(aViewManager)
	// IMPORTANT: see the initializer for the construction of this dictionary and the names of keys that are used
	NSDictionary*		asDictionary = STATIC_CAST(aContext, NSDictionary*);
	NSString*			givenIdentifier = [asDictionary objectForKey:@"identifier"];
	NSString*			givenName = [asDictionary objectForKey:@"localizedName"];
	NSImage*			givenIcon = [asDictionary objectForKey:@"localizedIcon"];
	Panel_ViewManager*	givenViewManager = [asDictionary objectForKey:@"viewManager"];
	
	
	self->identifier = [givenIdentifier retain];
	self->localizedName = [givenName retain];
	self->localizedIcon = [givenIcon retain];
	self->mainViewManager = [givenViewManager retain];
	
	assert(nil != self->mainViewManager);
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	// forward to child view
	*outEditType = kPanel_EditTypeNormal;
	[self->mainViewManager.delegate panelViewManager:aViewManager requestingEditType:outEditType];
}// panelViewManager:requestingEditType:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
	assert(nil != self->actionButton);
	assert(nil != self->cancelButton);
	assert(nil != self->otherButton);
	assert(nil != self->helpButton);
	assert(nil != self->viewContainer);
	
	assert(aViewManager == self);
	assert(aContainerView == self.managedView);
	
	[self whenObject:self->viewContainer postsNote:NSViewFrameDidChangeNotification
						performSelector:@selector(parentViewFrameDidChange:)];
	
	// determine ideal size of embedded panel, and calculate the
	// ideal size of the entire window accordingly
	{
		NSRect		containerFrame = [aContainerView frame];
		NSSize		panelIdealSize;
		Float32		initialWidth = NSWidth(containerFrame);
		Float32		initialHeight = NSHeight(containerFrame);
		
		
		[self->mainViewManager.delegate panelViewManager:self->mainViewManager requestingIdealSize:&panelIdealSize];
		[self->mainViewManager.managedView setFrameSize:panelIdealSize];
		
		if (panelIdealSize.width > initialWidth)
		{
			initialWidth = panelIdealSize.width;
		}
		initialHeight += panelIdealSize.height;
		
		// resize the container (and the window, through constraints)
		self->initialPanelSize = NSMakeSize(initialWidth, initialHeight);
		[aContainerView setFrameSize:self->initialPanelSize];
	}
	
	// there is only one “tab” (and the frame and background are
	// hidden) but using NSTabView allows several important
	// subview-management tasks to be taken care of automatically,
	// such as the view size and initial keyboard focus
	{
		NSRect				panelFrame = self->mainViewManager.managedView.frame;
		NSTabViewItem*		tabItem = [[NSTabViewItem alloc] initWithIdentifier:[aViewManager panelIdentifier]];
		
		
		[tabItem setView:self->mainViewManager.managedView];
		[tabItem setInitialFirstResponder:[self->mainViewManager logicalFirstResponder]];
		[self->viewContainer addTabViewItem:tabItem];
		[tabItem release];
		
		// anchor at the top of the window
		panelFrame.origin.y = NSHeight([[self->viewContainer superview] frame]) - NSHeight(panelFrame);
		self->viewContainer.frame = panelFrame;
	}
	
	// link the key view chains of the panel and the dialog
	[self->mainViewManager logicalLastResponder].nextKeyView = [super logicalFirstResponder];
	[super logicalLastResponder].nextKeyView = [self->mainViewManager logicalFirstResponder];
	
	[self updateButtonLayout];
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	// copy the size used for the popover
	*outIdealSize = [self idealSize];
}// panelViewManager:requestingIdealSize:


/*!
Responds to a request for contextual help in this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager)
	// forward to child view
	[self->mainViewManager.delegate panelViewManager:self->mainViewManager didPerformContextSensitiveHelp:sender];
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager)
	// forward to child view
	[self->mainViewManager.delegate panelViewManager:self->mainViewManager willChangePanelVisibility:aVisibility];
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager)
	// forward to child view
	[self->mainViewManager.delegate panelViewManager:self->mainViewManager didChangePanelVisibility:aVisibility];
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

The message is forwarded to the detail view but the data type
of the context is changed to "GeneralPanelNumberedList_DataSet"
(where only the "parentPanelDataSetOrNull" field can be
expected to change from old to new).

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager)
	// forward to child view
	[self->mainViewManager.delegate panelViewManager:self->mainViewManager didChangeFromDataSet:oldDataSet toDataSet:newDataSet];
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).

The message is forwarded to the detail view.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager)
	// forward to child view
	[self->mainViewManager.delegate panelViewManager:self->mainViewManager didFinishUsingContainerView:aContainerView userAccepted:isAccepted];
	
	// forward to original caller
	{
		My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), self->dialogRef);
		UInt32						commandID = (isAccepted) ? kHICommandOK : kHICommandCancel;
		Boolean						animateClose = (ptr->closeEffects.end() == ptr->closeEffects.find(commandID)) ||
													(kGenericDialog_DialogEffectCloseNormally == ptr->closeEffects[commandID]);
		
		
		GenericDialog_InvokeCloseNotifyProc(ptr->closeNotifyProc, ptr->selfRef, isAccepted/* OK was pressed */);
		
		if (false == animateClose)
		{
			PopoverManager_SetAnimationType(ptr->popoverManager, kPopoverManager_AnimationTypeNone);
		}
		PopoverManager_RemovePopover(ptr->popoverManager, isAccepted);
	}
	
	// by contract, the Generic Dialog takes control of the
	// object if the dialog is displayed; destroy it (this
	// must happen outside the lock-block above)
	GenericDialog_Dispose(&self->dialogRef);
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_Parent


/*!
Selects the panel with the given panel identifier, or does
nothing if the identifier does not match any panel.

Currently the animation flag has no effect.

(4.1)
*/
- (void)
panelParentDisplayChildWithIdentifier:(NSString*)	anIdentifier
withAnimation:(BOOL)								isAnimated
{
#pragma unused(anIdentifier, isAnimated)
	// nothing needed
}// panelParentDisplayChildWithIdentifier:withAnimation:


/*!
Returns an enumerator over the Panel_ViewManager* objects
for the panels in this view.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	return [[[@[self->mainViewManager] retain] autorelease] objectEnumerator];
}// panelParentEnumerateChildViewManagers


#pragma mark Panel_ViewManager


/*!
Overrides the base to return the logical first responder
of the embedded panel (so that keyboard entry starts in
that panel).

Note that the last responder is not overridden, and the
loop is set in "panelViewManager:didLoadContainerView:".

(4.1)
*/
- (NSView*)
logicalFirstResponder
{
	return [self->mainViewManager logicalFirstResponder];
}// logicalFirstResponder


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [[self->localizedIcon retain] autorelease];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return [[self->identifier retain] autorelease];
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

(4.1)
*/
- (NSString*)
panelName
{
	return [[self->localizedName retain] autorelease];
}// panelName


/*!
Returns the corresponding result from the child view manager.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	// return the child view’s constraint
	Panel_ResizeConstraint	result = [self->mainViewManager panelResizeAxes];
	
	
	return result;
}// panelResizeAxes


#pragma mark PopoverManager_Delegate


/*!
Returns the proper position of the popover arrow tip (if any),
relative to its parent window; also called during window resizing.

(4.1)
*/
- (NSPoint)
idealAnchorPointForFrame:(NSRect)	parentFrame
parentWindow:(NSWindow*)			parentWindow
{
	NSRect		contentFrame = [parentWindow contentRectForFrameRect:parentFrame];
	NSPoint		result = NSMakePoint(0, 0);
	
	
	result.x = (NSWidth(parentFrame) / 2.0);
	result.y = ((parentFrame.origin.y - contentFrame.origin.y) + NSHeight(contentFrame));
	
	result.y -= 20; // arbitrary; make it easier to drag the parent window
	
	return result;
}// idealAnchorPointForFrame:parentWindow:


/*!
Returns the desired popover arrow placement.

(4.1)
*/
- (Popover_Properties)
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(parentFrame, parentWindow)
	Popover_Properties		result = (kPopover_PropertyArrowMiddle |
										kPopover_PropertyPlaceFrameBelowArrow);
	
	
	return result;
}// idealArrowPositionForFrame:parentWindow:


/*!
Returns the dimensions the popover should initially have.

(4.1)
*/
- (NSSize)
idealSize
{
	return self->initialPanelSize;
}// idealSize


#pragma mark NSColorPanel


/*!
Forwards this message to the main view manager if the
panel implements a "changeColor:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeColor:(id)	sender
{
	if ([self->mainViewManager respondsToSelector:@selector(changeColor:)])
	{
		[self->mainViewManager changeColor:sender];
	}
}// changeColor:


#pragma mark NSFontPanel


/*!
Forwards this message to the main view manager if the
panel implements a "changeFont:" method.

NOTE:	It is possible that one day panels will be set up
		as responders themselves and placed directly in
		the responder chain.  For now this is sufficient.

(4.1)
*/
- (void)
changeFont:(id)		sender
{
	if ([self->mainViewManager respondsToSelector:@selector(changeFont:)])
	{
		[self->mainViewManager changeFont:sender];
	}
}// changeFont:


#pragma mark Notifications


/*!
Invoked when the parent view frame is changed.  The layout
of action buttons in the dialog depends on the width of the
main view.

(4.1)
*/
- (void)
parentViewFrameDidChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	[self updateButtonLayout];
}// parentViewFrameDidChange:


@end // GenericDialog_ViewManager


@implementation GenericDialog_ViewManager (GenericDialog_ViewManagerInternal)


#pragma mark New Methods


/*!
A helper to make string-setters less cumbersome to write.

(4.1)
*/
- (void)
setStringProperty:(NSString**)		propertyPtr
withName:(NSString*)				propertyName
toValue:(NSString*)					aString
{
	if (aString != *propertyPtr)
	{
		[self willChangeValueForKey:propertyName];
		
		if (nil == aString)
		{
			*propertyPtr = [@"" retain];
		}
		else
		{
			[*propertyPtr autorelease];
			*propertyPtr = [aString copy];
		}
		
		[self didChangeValueForKey:propertyName];
	}
}// setStringProperty:withName:toValue:


/*!
Automatically resizes all action buttons and lays them out
according to normal interface guidelines and the current
localization (right-to-left versus left-to-right).

(4.1)
*/
-
(void)
updateButtonLayout
{
	// do nothing unless views are loaded
	if (nil != self->actionButton)
	{
		NSArray*	buttonArray = @[self->actionButton, self->cancelButton, self->otherButton];
		
		
		Localization_ArrangeNSButtonArray(BRIDGE_CAST(buttonArray, CFArrayRef));
		Localization_AdjustHelpNSButton(self->helpButton);
	}
}// updateButtonLayout


@end // GenericDialog_ViewManager (GenericDialog_ViewManagerInternal)


// BELOW IS REQUIRED NEWLINE TO END FILE
