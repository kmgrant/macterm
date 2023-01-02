/*!	\file GenericDialog.mm
	\brief Allows a user interface that is both a panel
	and a dialog to be displayed as a modal dialog or
	sheet.
	
	Note that this module is in transition and is not yet a
	Cocoa-only user interface.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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
@import Cocoa;
@import CoreServices;

// library includes
#import <AlertMessages.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlocks.h>
#import <Popover.objc++.h>
#import <PopoverManager.objc++.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "Session.h"
#import "UIStrings.h"



#pragma mark Types
namespace {

typedef std::map< GenericDialog_ItemID, GenericDialog_DialogEffect >	My_DialogEffectsByItemID;

struct My_GenericDialog
{
	My_GenericDialog	(NSView*, Panel_ViewManager*, void*, Boolean = false);
	
	~My_GenericDialog	();
	
	void
	loadViewManager ();
	
	// IMPORTANT: DATA MEMBER ORDER HAS A CRITICAL EFFECT ON CONSTRUCTOR CODE EXECUTION ORDER.  DO NOT CHANGE!!!
	GenericDialog_Ref				selfRef;				//!< identical to address of structure, but typed as ref
	NSView* __weak					modalToView;			//!< if not nil, a view whose hierarchy is unusable while the dialog is open
	Boolean							isAlert;				//!< this may affect the window appearance or behavior
	Boolean							delayedKeyEquivalents;	//!< key equivalents such as the default button are only set a short time after the dialog is displayed
	GenericDialog_ViewManager* __strong		containerViewManager;	//!< new-style; object for rendering user interface around primary view (such as OK and Cancel buttons)
	Panel_ViewManager* __strong		hostedViewManager;		//!< new-style; the Cocoa view manager for the primary user interface
	PopoverManager_Ref				popoverManager;			//!< object to help display popover window
	Popover_Window* __strong		popoverWindow;			//!< new-style, popover variant; contains a Cocoa view in a popover frame
	My_DialogEffectsByItemID		closeEffects;			//!< custom sheet-closing effects for certain items
	void*							userDataPtr;			//!< optional; external data
	NSObject* __strong				userDataObject;			//!< optional; external data but (since it is an NSObject*) it is retained
	Memory_WeakRefEraser			weakRefEraser;			//!< at destruction time, clears weak references that involve this object
};
typedef My_GenericDialog*			My_GenericDialogPtr;
typedef My_GenericDialog const*		My_GenericDialogConstPtr;

typedef MemoryBlockPtrLocker< GenericDialog_Ref, My_GenericDialog >			My_GenericDialogPtrLocker;
typedef LockAcquireRelease< GenericDialog_Ref, My_GenericDialog >			My_GenericDialogAutoLocker;
typedef MemoryBlockReferenceLocker< GenericDialog_Ref, My_GenericDialog >	My_GenericDialogReferenceLocker;

} // anonymous namespace


/*!
This view can be used to debug the boundaries of
the panel.
*/
@interface GenericDialog_PanelView : NSTabView //{

// NSView
	- (void)
	drawRect:(NSRect)_;

@end //}


/*!
Private properties.
*/
@interface GenericDialog_ViewManager () //{

// accessors
	//! The panel’s unique identication string in dotted-name format.
	@property (strong, nonnull) NSString*
	customPanelIdentifier;
	//! The panel’s user-visible icon image.
	@property (strong, nonnull) NSImage*
	customPanelLocalizedIcon;
	//! The panel’s user-visible title.
	@property (strong, nonnull) NSString*
	customPanelLocalizedName;
	//! External reference; alias for this object.
	@property (assign) GenericDialog_Ref
	dialogRef;
	//! Created on demand by "makeTouchBar"; this is different
	//! than the NSResponder "touchBar" property, which has
	//! side effects such as archiving, etc.
	@property (strong) NSTouchBar*
	dynamicTouchBar;
	//! Ideal size, taking into account actual dialog configuration.
	@property (assign) CGSize
	idealManagedViewSize;
	//! Captures initial size from XIB for use in later size queries.
	@property (assign) CGSize
	initialPanelSize;
	//! View controller for dialog content.
	@property (strong) Panel_ViewManager*
	mainViewManager;

@end //}


/*!
The private class interface.
*/
@interface GenericDialog_ViewManager (GenericDialog_ViewManagerInternal) //{

// new methods
	- (void)
	performActionFrom:(id)_
	forButton:(GenericDialog_ItemID)_;
	- (void)
	setStringProperty:(NSString* __strong*)_
	withName:(NSString*)_
	toValue:(NSString*)_;
	- (void)
	updateButtonLayout;

@end //}

#pragma mark Variables
namespace {

My_GenericDialogPtrLocker&			gGenericDialogPtrLocks ()	{ static My_GenericDialogPtrLocker x; return x; }
My_GenericDialogReferenceLocker&	gGenericDialogRefLocks ()	{ static My_GenericDialogReferenceLocker x; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
This method is used to create a Cocoa-based popover view
that runs a dialog.  If "iModalToViewOrNullForAppModalDialog"
is nullptr, the window is automatically application-modal;
otherwise, it is a sheet.

The specified view manager is retained by this call (since
the dialog might require access to the panel for an unknown
period of time, under user control).  Therefore, you can
release your copy of the reference after this call returns.

The format of "inDataSetPtr" is entirely defined by the type
of panel that the dialog is hosting.  The data is passed to
the panel with Panel_SendMessageNewDataSet().

You must use GenericDialog_Release() when finished.  (It is
recommended that you make use of the GenericDialog_Wrap
class for this however.)

(4.1)
*/
GenericDialog_Ref
GenericDialog_New	(NSView*				inModalToViewOrNullForAppModalDialog,
					 Panel_ViewManager*		inHostedViewManager,
					 void*					inDataSetPtr,
					 Boolean				inIsAlert)
{
	GenericDialog_Ref	result = nullptr;
	
	
	assert(nil != inHostedViewManager);
	
	try
	{
		result = REINTERPRET_CAST(new My_GenericDialog
									(inModalToViewOrNullForAppModalDialog,
										inHostedViewManager, inDataSetPtr, inIsAlert),
									GenericDialog_Ref);
		GenericDialog_Retain(result);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Adds a lock on the specified reference, preventing it from
being deleted.  See GenericDialog_Release().

NOTE:	Usually, you should let GenericDialog_Wrap objects
		handle retain/release for you.

(2.6)
*/
void
GenericDialog_Retain	(GenericDialog_Ref	inRef)
{
	gGenericDialogRefLocks().acquireLock(inRef);
}// Retain


/*!
Releases one lock on the specified dialog and deletes the dialog
*if* no other locks remain.  Your copy of the reference is set
to nullptr either way.

NOTE:	Usually, you should let GenericDialog_Wrap objects
		handle retain/release for you.

(2.6)
*/
void
GenericDialog_Release	(GenericDialog_Ref*		inoutRefPtr)
{
	gGenericDialogRefLocks().releaseLock(*inoutRefPtr);
	unless (gGenericDialogRefLocks().isLocked(*inoutRefPtr))
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
	}
	*inoutRefPtr = nullptr;
}// Release


/*!
Displays the dialog.  If the dialog is modal, this call will block
until the dialog is finished.  Otherwise, a sheet will appear over
the parent window and this call will return immediately.

The release block is executed when the dialog is removed by the
user.  While technically this block can do anything, it is better
to use GenericDialog_SetItemResponseBlock() for reactions to each
button, and use the release block for general cleanup procedures.

IMPORTANT:	When a GenericDialog_Ref is “owned” by another object,
			the correct way to call GenericDialog_Display() is to
			“retain” that parent object first, then display the
			dialog with a release-block that will (eventually)
			release the object.  This allows the lifetime of the
			owner to be extended until at least as long as the
			user is still using the dialog.

(2016.05)
*/
void
GenericDialog_Display	(GenericDialog_Ref				inDialog,
						 Boolean						inAnimated,
						 GenericDialog_CleanupBlock		inImplementationReleaseBlock)
{
	Popover_Window*		newPopoverWindow = nil;
	Boolean				shouldRunModal = false;
	Boolean				userPrefersNoAnimation = false;
	Boolean				noAnimation = false;
	
	
	// determine if animation should occur
	unless (kPreferences_ResultOK ==
			Preferences_GetData(kPreferences_TagNoAnimations,
								sizeof(userPrefersNoAnimation), &userPrefersNoAnimation))
	{
		userPrefersNoAnimation = false; // assume a value, if preference can’t be found
	}
	noAnimation = ((false == inAnimated) || (userPrefersNoAnimation));
	
	// operations that require access to the object should occur
	// inside a local lock-block that is released prior to
	// processing dialog events (the application-modal case);
	// otherwise, a button click might try to destroy the dialog
	// and fail to do so
	{
		My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
		
		
		if (nullptr == ptr)
		{
			Sound_StandardAlert(); // TEMPORARY (display alert message?)
		}
		else
		{
			if (nil != ptr->hostedViewManager)
			{
				shouldRunModal = (nil == ptr->modalToView);
				
				ptr->loadViewManager();
				assert(nil != ptr->containerViewManager);
				
				if (nil == ptr->popoverWindow)
				{
					newPopoverWindow = [[Popover_Window alloc] initWithView:ptr->containerViewManager.managedView
																			windowStyle:((ptr->isAlert)
																						? ((shouldRunModal)
																							? kPopover_WindowStyleAlertAppModal
																							: kPopover_WindowStyleAlertSheet)
																						: ((shouldRunModal)
																							? kPopover_WindowStyleDialogAppModal
																							: kPopover_WindowStyleDialogSheet))
																			arrowStyle:kPopover_ArrowStyleNone
																			attachedToPoint:NSMakePoint(0, 0)/* TEMPORARY */
																			inWindow:ptr->modalToView.window];
					ptr->popoverWindow = newPopoverWindow;
					ptr->popoverWindow.arrowHeight = 0;
					
					// make application-modal windows movable
					if (shouldRunModal)
					{
						[ptr->popoverWindow setMovableByWindowBackground:YES];
					}
				}
				
				if (nil == ptr->popoverManager)
				{
					ptr->popoverManager = PopoverManager_New(ptr->popoverWindow,
																[ptr->containerViewManager logicalFirstResponder],
																ptr->containerViewManager/* delegate */,
																(noAnimation)
																? kPopoverManager_AnimationTypeNone
																: kPopoverManager_AnimationTypeStandard,
																kPopoverManager_BehaviorTypeDialog,
																ptr->modalToView);
				}
				
				ptr->containerViewManager.cleanupBlock = inImplementationReleaseBlock;
				
				// apply size constraints to the window as appropriate
				{
					NSSize		idealSize = ptr->popoverWindow.frame.size;
					NSRect		idealFrame = ptr->popoverWindow.frame;
					
					
					[ptr->containerViewManager.delegate panelViewManager:ptr->containerViewManager requestingIdealSize:&idealSize];
					idealFrame = [ptr->popoverWindow frameRectForViewSize:idealSize];
					ptr->popoverWindow.minSize = idealFrame.size;
					[ptr->popoverWindow setFrame:idealFrame display:NO];
				}
				
				[ptr->containerViewManager.delegate panelViewManager:ptr->containerViewManager willChangePanelVisibility:kPanel_VisibilityDisplayed];
				PopoverManager_DisplayPopover(ptr->popoverManager);
				[ptr->containerViewManager.delegate panelViewManager:ptr->containerViewManager didChangePanelVisibility:kPanel_VisibilityDisplayed];
			}
			else
			{
				assert(false && "generic dialog expected a hosted view manager");
			}
		}
	}
	
	if (shouldRunModal)
	{
		// IMPORTANT: the PopoverManager_DisplayPopover() routine must
		// display the window immediately when there is no parent window;
		// if it tries to animate, the following modal loop may begin
		// before an animation is able to complete (there are ways
		// around this but it’s much better for the user to just show
		// the window immediately anyway)
		UNUSED_RETURN(long)[NSApp runModalForWindow:newPopoverWindow];
	}
}// Display


/*!
Sends a fake cancellation message to cause the dialog to be
removed from the screen.

WARNING:	If there are no remaining retains of the dialog,
			this action could cause the underlying structure
			to be destroyed.  Therefore, if you plan to do
			anything else with the object afterward, you
			must call GenericDialog_Retain() beforehand.

(4.1)
*/
void
GenericDialog_Remove	(GenericDialog_Ref		inDialog)
{
	GenericDialog_ViewManager*		viewManager = nil;
	
	
	// find the view manager and then release the lock
	// (otherwise the object cannot be destroyed)
	{
		My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
		
		
		viewManager = ptr->containerViewManager;
	}
	
	// send a simulated button click to hide and destroy
	// the dialog
	[viewManager performActionFrom:nil forButton:kGenericDialog_ItemIDButton2];
}// Remove


/*!
Returns value set with GenericDialog_SetImplementation(),
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
Returns value set with GenericDialog_SetImplementationNSObject(),
or nil.

(2021.01)
*/
NSObject*
GenericDialog_ReturnImplementationNSObject	(GenericDialog_Ref	inDialog)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	NSObject*					result = nullptr;
	
	
	if (nullptr != ptr) result = ptr->userDataObject;
	
	return result;
}// ReturnImplementationNSObject


/*!
Returns the effect that an action has on the dialog, as
set by GenericDialog_SetItemDialogEffect() (or, if never
set, the default for the button ID).

(2016.05)
*/
GenericDialog_DialogEffect
GenericDialog_ReturnItemDialogEffect	(GenericDialog_Ref		inDialog,
										 GenericDialog_ItemID	inItemID)
{
	GenericDialog_DialogEffect		result = kGenericDialog_DialogEffectCloseNormally;
	My_GenericDialogAutoLocker		ptr(gGenericDialogPtrLocks(), inDialog);
	auto							toEffect = ptr->closeEffects.find(inItemID);
	
	
	// see if the effect has been overridden
	if (toEffect != ptr->closeEffects.end())
	{
		result = toEffect->second;
	}
	
	return result;
}// ReturnItemDialogEffect


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
Specifies whether or not key equivalents for buttons
are installed only after a short delay.

This is strongly recommended for alerts that may
display important messages or have destructive
commands (such as alerts), as it prevents the user
from inadvertently dismissing the window the instant
it appears.

(2016.12)
*/
void
GenericDialog_SetDelayedKeyEquivalents	(GenericDialog_Ref		inDialog,
										 Boolean				inKeyEquivalentsDelayed)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->delayedKeyEquivalents = inKeyEquivalentsDelayed;
}// SetDelayedKeyEquivalents


/*!
Associates arbitrary user data with your dialog.
Retrieve with GenericDialog_ReturnImplementation().

(3.1)
*/
void
GenericDialog_SetImplementation		(GenericDialog_Ref		inDialog,
									 void*					inContext)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->userDataPtr = inContext;
}// SetImplementation


/*!
Associates an arbitrary Objective-C object (retained).
Retrieve with GenericDialog_ReturnImplementationNSObject().

The object can be released by calling this API again using
an argument of "nil", or by disposing of the dialog.

(2021.01)
*/
void
GenericDialog_SetImplementationNSObject		(GenericDialog_Ref		inDialog,
											 NSObject*				inContext)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->userDataObject = inContext;
}// SetImplementationNSObject


/*!
Specifies the effect that an action has on the dialog.

Note that by default, the OK and Cancel buttons have the
effect of "kGenericDialog_DialogEffectCloseNormally", and
any extra buttons have no effect at all (that is,
"kGenericDialog_DialogEffectNone").

(3.1)
*/
void
GenericDialog_SetItemDialogEffect	(GenericDialog_Ref				inDialog,
									 GenericDialog_ItemID			inItemID,
									 GenericDialog_DialogEffect		inEffect)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	ptr->closeEffects[inItemID] = inEffect;
}// SetItemDialogEffect


/*!
Specifies the response for an action button.

IMPORTANT:	This API currently only works for the
			standard buttons, not custom command IDs.

(4.1)
*/
void
GenericDialog_SetItemResponseBlock	(GenericDialog_Ref		inDialog,
									 GenericDialog_ItemID	inItemID,
									 void					(^inResponseBlock)(),
									 Boolean				inIsHarmfulAction)
{
	My_GenericDialogAutoLocker		ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (nil != ptr->hostedViewManager)
	{
		ptr->loadViewManager();
		assert(nil != ptr->containerViewManager);
		if (kGenericDialog_ItemIDButton2 == inItemID)
		{
			ptr->containerViewManager.secondButtonBlock = inResponseBlock;
		}
		else if (kGenericDialog_ItemIDButton3 == inItemID)
		{
			ptr->containerViewManager.thirdButtonBlock = inResponseBlock;
		}
		else if (kGenericDialog_ItemIDHelpButton == inItemID)
		{
			ptr->containerViewManager.helpButtonBlock = inResponseBlock;
		}
		else
		{
			ptr->containerViewManager.primaryButtonBlock = inResponseBlock;
		}
		
		if (inIsHarmfulAction)
		{
			ptr->containerViewManager.harmfulActionItemID = inItemID;
		}
	}
	else
	{
		assert(false && "generic dialog expected a hosted view manager");
	}
}// SetItemResponseBlock


/*!
Specifies a new name for the button corresponding to the
given ID.  The specified button is automatically resized
to fit the title, and other action buttons are moved (and
possibly resized) to make room.

The title can be set to nullptr for buttons 2 and 3 in
order to hide them.  This is achieved through bindings.

(4.0)
*/
void
GenericDialog_SetItemTitle	(GenericDialog_Ref		inDialog,
							 GenericDialog_ItemID	inItemID,
							 CFStringRef			inButtonTitle)
{
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), inDialog);
	
	
	if (nil != ptr->hostedViewManager)
	{
		if (kGenericDialog_ItemIDButton1 == inItemID)
		{
			ptr->loadViewManager();
			assert(nil != ptr->containerViewManager);
			ptr->containerViewManager.primaryButtonName = BRIDGE_CAST(inButtonTitle, NSString*);
		}
		else if (kGenericDialog_ItemIDButton2 == inItemID)
		{
			ptr->loadViewManager();
			assert(nil != ptr->containerViewManager);
			// there is a binding that sets the hidden state based
			// on "nil" name values
			ptr->containerViewManager.secondButtonName = BRIDGE_CAST(inButtonTitle, NSString*);
		}
		else if (kGenericDialog_ItemIDButton3 == inItemID)
		{
			ptr->loadViewManager();
			assert(nil != ptr->containerViewManager);
			// there is a binding that sets the hidden state based
			// on "nil" name values
			ptr->containerViewManager.thirdButtonName = BRIDGE_CAST(inButtonTitle, NSString*);
		}
		else
		{
			assert(false && "unsupported item ID");
		}
	}
	else
	{
		assert(false && "generic dialog expected a hosted view manager");
	}
}// SetItemTitle


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
My_GenericDialog	(NSView*				inModalToNSViewOrNull,
					 Panel_ViewManager*		inHostedViewManagerOrNull,
					 void*					inDataSetPtr,
					 Boolean				inIsAlert)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef							(REINTERPRET_CAST(this, GenericDialog_Ref)),
modalToView						(inModalToNSViewOrNull),
isAlert							(inIsAlert),
delayedKeyEquivalents			(false),
containerViewManager			(nil), // set later if necessary
hostedViewManager				(inHostedViewManagerOrNull),
popoverManager					(nullptr), // created as needed
popoverWindow					(nil), // set later if necessary
closeEffects					(),
userDataPtr						(nullptr),
weakRefEraser					(this)
{
	// now notify the panel of its data
	[inHostedViewManagerOrNull.delegate panelViewManager:inHostedViewManagerOrNull
															didChangeFromDataSet:nullptr
															toDataSet:inDataSetPtr];
}// My_GenericDialog constructor


/*!
Destructor.  See GenericDialog_Release().

(3.1)
*/
My_GenericDialog::
~My_GenericDialog ()
{
	userDataObject = nil;
	
	if (nullptr != popoverManager)
	{
		[containerViewManager.delegate panelViewManager:containerViewManager willChangePanelVisibility:kPanel_VisibilityHidden];
		PopoverManager_RemovePopover(popoverManager, false/* is confirming */);
	}
}// My_GenericDialog destructor


/*!
Constructs the internal view manager object if necessary.

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
										initWithRef:this->selfRef
													identifier:[this->hostedViewManager panelIdentifier]
													localizedName:[this->hostedViewManager panelName]
													localizedIcon:[this->hostedViewManager panelIcon]
													viewManager:this->hostedViewManager];
	}
}// loadViewManager

} // anonymous namespace


#pragma mark -
@implementation GenericDialog_PanelView //{


#pragma mark NSView


/*!
Starts by asking the superclass to draw.  And then, if
enabled, performs custom drawing on top for debugging.

(2016.05)
*/
- (void)
drawRect:(NSRect)	aRect
{
	[super drawRect:aRect];
	
#if 0
	// for debugging; display a red rectangle to show
	// the area occupied by the view
	CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
	
	
	CGContextSetRGBStrokeColor(drawingContext, 1.0, 0.0, 0.0, 1.0/* alpha */);
	CGContextSetLineWidth(drawingContext, 2.0);
	CGContextStrokeRect(drawingContext, NSRectToCGRect(NSInsetRect(self.bounds, 1.0, 1.0)));
#endif
}// drawRect:


@end //}


#pragma mark -
@implementation GenericDialog_ViewManager


@synthesize primaryButtonName = _primaryButtonName;
@synthesize secondButtonName = _secondButtonName;
@synthesize thirdButtonName = _thirdButtonName;


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
	GenericDialog_Retain(aDialogRef);
	self = [super initWithNibNamed:@"GenericDialogCocoa"
									delegate:self
									context:@{
												@"identifier": anIdentifier,
												@"localizedName": aName,
												@"localizedIcon": anImage,
												@"viewManager": aViewManager,
												@"dialogRef": [NSValue valueWithPointer:aDialogRef],
											}];
	if (nil != self)
	{
		aViewManager.panelParent = self;
		
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
	Memory_EraseWeakReferences(BRIDGE_CAST(self, void*));
	
	[self ignoreWhenObjectsPostNotes];
	
	self.mainViewManager.panelParent = nil;
	
	if (nullptr != _dialogRef)
	{
		GenericDialog_Release(&_dialogRef);
	}
}// dealloc


#pragma mark Initializers Disabled From Superclass


/*!
Compiler expects this superclass designated initializer to
be defined but this variant is not supported.

(2021.01)
*/
- (instancetype)
initWithNibNamed:(NSString*)		aNibName
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithRef:nullptr identifier:nil localizedName:nil localizedIcon:nil viewManager:nil];
}// initWithNibNamed:delegate:context:


/*!
Compiler expects this superclass designated initializer to
be defined but this variant is not supported.

(2021.01)
*/
- (instancetype)
initWithView:(NSView*)				aView
delegate:(id< Panel_Delegate >)		aDelegate
context:(NSObject*)					aContext
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithRef:nullptr identifier:nil localizedName:nil localizedIcon:nil viewManager:nil];
}// initWithView:delegate:context:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
primaryButtonName
{
	return _primaryButtonName;
}
- (void)
setPrimaryButtonName:(NSString*)	aString
{
	[self setStringProperty:&_primaryButtonName withName:@"primaryButtonName" toValue:aString];
	
	// NOTE: UI updates are OK here because there are no
	// bindings that can be set ahead of construction
	// (initialization occurs at view-load time); otherwise
	// it would be necessary to keep track of pending updates
	[self updateButtonLayout];
}// setPrimaryButtonName:


/*!
Accessor.

(4.1)
*/
- (NSString*)
secondButtonName
{
	return _secondButtonName;
}
- (void)
setSecondButtonName:(NSString*)	aString
{
	[self setStringProperty:&_secondButtonName withName:@"secondButtonName" toValue:aString];
	
	// if there is no name, remove the button from the key loop
	[self.cancelButton setRefusesFirstResponder:(nil == aString)];
	
	// NOTE: UI updates are OK here because there are no
	// bindings that can be set ahead of construction
	// (initialization occurs at view-load time); otherwise
	// it would be necessary to keep track of pending updates
	[self updateButtonLayout];
}// setSecondButtonName:


/*!
Accessor.

(4.1)
*/
- (NSString*)
thirdButtonName
{
	return _thirdButtonName;
}
- (void)
setThirdButtonName:(NSString*)		aString
{
	[self setStringProperty:&_thirdButtonName withName:@"thirdButtonName" toValue:aString];
	
	// if there is no name, remove the button from the key loop
	[self.otherButton setRefusesFirstResponder:(nil == aString)];
	
	// for the third button, infer a suitable command key
	if ((nil != aString) && (aString.length > 0))
	{
		// lowercase the “first letter” to avoid implicit shift-key binding
		NSString*	keyCharString = [[aString substringWithRange:[aString rangeOfComposedCharacterSequenceAtIndex:0]] localizedLowercaseString];
		
		
		self.otherButton.keyEquivalent = keyCharString;
		self.otherButton.keyEquivalentModifierMask = NSEventModifierFlagCommand;
	}
	
	// NOTE: UI updates are OK here because there are no
	// bindings that can be set ahead of construction
	// (initialization occurs at view-load time); otherwise
	// it would be necessary to keep track of pending updates
	[self updateButtonLayout];
}// setThirdButtonName:


#pragma mark Actions


/*!
Invoked when the user clicks the help button in the dialog.
See the "helpButtonBlock" property.

(2021.03)
*/
- (IBAction)
performHelpButtonAction:(id)	sender
{
	if (self.panelHasContextualHelp)
	{
		[self panelViewManager:self didPerformContextSensitiveHelp:sender];
	}
}// performHelpButtonAction:


/*!
Invoked when the user clicks the first button in the dialog.
See the "primaryButtonName" property.

(4.1)
*/
- (IBAction)
performPrimaryButtonAction:(id)		sender
{
	[self performActionFrom:sender forButton:kGenericDialog_ItemIDButton1];
}// performPrimaryButtonAction:


/*!
Invoked when the user clicks the “second button” in the dialog.
See the "secondButtonName" property.

(4.1)
*/
- (IBAction)
performSecondButtonAction:(id)	sender
{
	[self performActionFrom:sender forButton:kGenericDialog_ItemIDButton2];
}// performSecondButtonAction:


/*!
Invoked when the user clicks the “third button” in the dialog.
See the "thirdButtonName" property.

(4.1)
*/
- (IBAction)
performThirdButtonAction:(id)	sender
{
	[self performActionFrom:sender forButton:kGenericDialog_ItemIDButton3];
}// performThirdButtonAction:


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(NSObject*)		aContext
{
#pragma unused(aViewManager)
	// IMPORTANT: see the initializer for the construction of this dictionary and the names of keys that are used
	NSDictionary*		asDictionary = STATIC_CAST(aContext, NSDictionary*);
	NSString*			givenIdentifier = [asDictionary objectForKey:@"identifier"];
	NSString*			givenName = [asDictionary objectForKey:@"localizedName"];
	NSImage*			givenIcon = [asDictionary objectForKey:@"localizedIcon"];
	Panel_ViewManager*	givenViewManager = [asDictionary objectForKey:@"viewManager"];
	NSValue*			givenDialogRefValue = [asDictionary objectForKey:@"dialogRef"];
	
	
	_harmfulActionItemID = kGenericDialog_ItemIDNone;
	
	self.customPanelIdentifier = givenIdentifier;
	self.customPanelLocalizedName = givenName;
	self.customPanelLocalizedIcon = givenIcon;
	self.mainViewManager = givenViewManager;
	{
		assert(nil != givenDialogRefValue);
		GenericDialog_Ref	givenDialogRef = STATIC_CAST([givenDialogRefValue pointerValue], GenericDialog_Ref);
		
		
		self.dialogRef = givenDialogRef;
	}
	
	assert(nil != self.mainViewManager);
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
	[self.mainViewManager.delegate panelViewManager:aViewManager requestingEditType:outEditType];
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
	assert(nil != self.actionButton);
	assert(nil != self.cancelButton);
	assert(nil != self.otherButton);
	assert(nil != self.helpButton);
	assert(nil != self.viewContainer);
	
	assert(aViewManager == self);
	assert(aContainerView == self.managedView);
	
	[self whenObject:self.managedView.window postsNote:NSWindowDidResizeNotification
						performSelector:@selector(parentViewFrameDidChange:)];
	[self whenObject:self.mainViewManager.delegate postsNote:kPanel_IdealSizeDidChangeNotification
						performSelector:@selector(childViewIdealSizeDidChange:)];
	
	// determine ideal size of embedded panel, and calculate the
	// ideal size of the entire window accordingly
	{
		NSRect		containerFrame = [aContainerView frame];
		Float32		initialWidth = NSWidth(containerFrame);
		Float32		initialHeight = NSHeight(containerFrame);
		
		
		// see also "childViewIdealSizeDidChange:"
		[self.mainViewManager.delegate panelViewManager:self.mainViewManager requestingIdealSize:&_idealManagedViewSize];
		[self.mainViewManager.managedView setFrameSize:self.idealManagedViewSize];
		
		if (self.idealManagedViewSize.width > initialWidth)
		{
			initialWidth = self.idealManagedViewSize.width;
		}
		initialHeight += self.idealManagedViewSize.height;
		
		// resize the container (and the window, through constraints)
		self.initialPanelSize = NSMakeSize(initialWidth, initialHeight);
		[aContainerView setFrameSize:self.initialPanelSize];
	}
	
	// there is only one “tab” (and the frame and background are
	// hidden) but using NSTabView allows several important
	// subview-management tasks to be taken care of automatically,
	// such as the view size and initial keyboard focus
	{
		NSRect				panelFrame = self.mainViewManager.managedView.frame;
		NSTabViewItem*		tabItem = [[NSTabViewItem alloc] initWithIdentifier:[aViewManager panelIdentifier]];
		
		
		tabItem.view = self.mainViewManager.managedView;
		tabItem.initialFirstResponder = [self.mainViewManager logicalFirstResponder];
		[self.viewContainer addTabViewItem:tabItem];
		
		// anchor at the top of the window
		panelFrame.origin.y = NSHeight(self.viewContainer.superview.frame) - NSHeight(panelFrame);
		self.viewContainer.frame = panelFrame;
	}
	
	// link the key view chains of the panel and the dialog
	assert(nil != [self.mainViewManager logicalFirstResponder]);
	assert(nil != [self.mainViewManager logicalLastResponder]);
	[self.mainViewManager logicalLastResponder].nextKeyView = self.actionButton;
	self.helpButton.nextKeyView = [self.mainViewManager logicalFirstResponder];
	
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
	My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), self.dialogRef);
	NSWindow*					viewWindow = aViewManager.view.window;
	NSSize						popoverFrameSize = viewWindow.frame.size;
	
	
	*outIdealSize = popoverFrameSize; // set a default in case the queries fail below
	if (nullptr != ptr)
	{
		// copy the size of the main view in the popover
		[self popoverManager:ptr->popoverManager getIdealSize:&popoverFrameSize];
		{
			NSRect		mockFrame = NSMakeRect(0, 0, popoverFrameSize.width, popoverFrameSize.height);
			NSRect		contentFrame = [viewWindow contentRectForFrameRect:mockFrame];
			
			
			*outIdealSize = contentFrame.size;
		}
	}
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
	[self.mainViewManager.delegate panelViewManager:self.mainViewManager didPerformContextSensitiveHelp:sender];
	
	// perform help block
	[self performActionFrom:sender forButton:kGenericDialog_ItemIDHelpButton];
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
	// determine if the key equivalents for buttons should be
	// initially disabled; if so, remove them here and install
	// a delayed invocation to put them back
	if ((kPanel_VisibilityDisplayed == aVisibility) &&
		(nullptr != self.dialogRef))
	{
		My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), self.dialogRef);
		auto						setDestructiveActionBlock =
									^(NSButton* aButton)
									{
										if (@available(macOS 11.0, *))
										{
											aButton.hasDestructiveAction = YES;
										}
										
										// NSAlert and NSSavePanel will auto-style buttons based on the
										// "hasDestructiveAction" property but otherwise the system
										// currently does nothing with this; in addition, older OS
										// versions do not have the property; to work around this, the
										// button title is styled manually
										NSShadow*		shadow = [[NSShadow alloc] init];
										shadow.shadowColor = [NSColor windowBackgroundColor];
										shadow.shadowOffset = NSMakeSize(0.0, 0.0); // Cartesian coordinates
										shadow.shadowBlurRadius = 6.0;
										NSDictionary*	attributeDictNormal =
										@{
											NSForegroundColorAttributeName: [NSColor systemRedColor],
											NSFontAttributeName: [NSFont boldSystemFontOfSize:0.0],
											NSShadowAttributeName: shadow, 
										};
										NSDictionary*	attributeDictSelected =
										@{
											NSForegroundColorAttributeName: [NSColor selectedControlTextColor],
											NSFontAttributeName: [NSFont boldSystemFontOfSize:0.0],
											NSShadowAttributeName: shadow, 
										};
										aButton.attributedTitle = [[NSAttributedString alloc]
																	initWithString:aButton.title
																					attributes:attributeDictNormal];
										aButton.attributedAlternateTitle = [[NSAttributedString alloc]
																			initWithString:aButton.title
																							attributes:attributeDictSelected];
									};
		auto						setKeyEquivalentsBlock =
									^{
										NSButton*	keyButton = self.cancelButton;
										
										
										if ((nil == keyButton) || (keyButton.isHidden))
										{
											keyButton = self.actionButton;
										}
										UNUSED_RETURN(BOOL)[self.managedView.window makeFirstResponder:keyButton];
										self.managedView.window.defaultButtonCell = self.actionButton.cell;
										[self.managedView.window enableKeyEquivalentForDefaultButtonCell];
										[self.actionButton display];
									};
		
		
		// if there is any “harmful” action, mark it as such
		if (@available(macOS 11.0, *))
		{
			switch (self.harmfulActionItemID)
			{
			case kGenericDialog_ItemIDButton1:
				if (nil != self.actionButton)
				{
					setDestructiveActionBlock(self.actionButton);
				}
				break;
			
			case kGenericDialog_ItemIDButton2:
				if (nil != self.cancelButton)
				{
					setDestructiveActionBlock(self.cancelButton);
				}
				break;
			
			case kGenericDialog_ItemIDButton3:
				if (nil != self.otherButton)
				{
					setDestructiveActionBlock(self.otherButton);
				}
				break;
			
			case kGenericDialog_ItemIDNone:
			case kGenericDialog_ItemIDHelpButton:
			default:
				// ???
				break;
			}
		}
		
		if (kGenericDialog_ItemIDButton1 == self.harmfulActionItemID)
		{
			[self.managedView.window disableKeyEquivalentForDefaultButtonCell];
			self.managedView.window.defaultButtonCell = nil;
		}
		else
		{
			// delay the availability of the Return-key mapping if requested
			// or if there is exactly one button in the dialog
			if ((ptr->delayedKeyEquivalents) ||
				((nil == self.secondButtonBlock) && (nil == self.thirdButtonBlock)))
			{
				[self.managedView.window disableKeyEquivalentForDefaultButtonCell];
				self.managedView.window.defaultButtonCell = nil;
				UNUSED_RETURN(BOOL)[self.managedView.window makeFirstResponder:self.helpButton];
				CocoaExtensions_RunLater(1.0/* arbitrary delay in seconds */, setKeyEquivalentsBlock);
			}
			else
			{
				setKeyEquivalentsBlock();
			}
		}
	}
	
	// forward to child view
	[self.mainViewManager.delegate panelViewManager:self.mainViewManager willChangePanelVisibility:aVisibility];
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
	[self.mainViewManager.delegate panelViewManager:self.mainViewManager didChangePanelVisibility:aVisibility];
	
	// fix initial focus of buttons for alerts
	if (kPanel_VisibilityDisplayed == aVisibility)
	{
		My_GenericDialogAutoLocker		ptr(gGenericDialogPtrLocks(), self.dialogRef);
		
		
		if (ptr->isAlert)
		{
			if (nil != self.cancelButton)
			{
				[aViewManager.view.window makeFirstResponder:self.cancelButton];
			}
			else if (nil != self.otherButton)
			{
				[aViewManager.view.window makeFirstResponder:self.otherButton];
			}
			else if (nil != self.actionButton)
			{
				[aViewManager.view.window makeFirstResponder:self.actionButton];
			}
		}
	}
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
	[self.mainViewManager.delegate panelViewManager:self.mainViewManager didChangeFromDataSet:oldDataSet toDataSet:newDataSet];
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
	[self.mainViewManager.delegate panelViewManager:self.mainViewManager didFinishUsingContainerView:aContainerView userAccepted:isAccepted];
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
Returns the number of items that will be covered by a call
to "panelParentEnumerateChildViewManagers".

(2017.03)
*/
- (NSUInteger)
panelParentChildCount
{
	return 1;
}// panelParentChildCount


/*!
Returns an enumerator over the Panel_ViewManager* objects
for the panels in this view.

(4.1)
*/
- (NSEnumerator*)
panelParentEnumerateChildViewManagers
{
	return [@[self.mainViewManager] objectEnumerator];
}// panelParentEnumerateChildViewManagers


#pragma mark Panel_ViewManager


/*!
Overrides the base to return the logical first responder
of the embedded panel (so that keyboard entry starts in
that panel), or one of the action buttons if the panel
has no key-input items.

Note that the last responder is not overridden, and the
loop is set in "panelViewManager:didLoadContainerView:".

(4.1)
*/
- (NSView*)
logicalFirstResponder
{
	NSView*		result = [self.mainViewManager logicalFirstResponder];
	
	
	if (NO == result.canBecomeKeyView)
	{
		// abnormal panel that has no key-input items; instead,
		// set initial focus to an available action button
		if (self.cancelButton.canBecomeKeyView)
		{
			result = self.cancelButton;
		}
		else if (self.otherButton.canBecomeKeyView)
		{
			result = self.otherButton;
		}
		else
		{
			result = self.actionButton;
		}
	}
	
	return result;
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
	return self.customPanelLocalizedIcon;
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return self.customPanelIdentifier;
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
	return self.customPanelLocalizedName;
}// panelName


/*!
Returns the corresponding result from the child view manager.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	// return the child view’s constraint
	Panel_ResizeConstraint	result = [self.mainViewManager panelResizeAxes];
	
	
	return result;
}// panelResizeAxes


#pragma mark PopoverManager_Delegate


/*!
Assists the dynamic resize of a popover window by indicating
whether or not there are per-axis constraints on resizing.

(2017.05)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getHorizontalResizeAllowed:(BOOL*)		outHorizontalFlagPtr
getVerticalResizeAllowed:(BOOL*)		outVerticalFlagPtr
{
#pragma unused(aPopoverManager)
	Panel_ResizeConstraint const	kConstraints = [self panelResizeAxes];
	
	
	*outHorizontalFlagPtr = ((kPanel_ResizeConstraintHorizontal == kConstraints) ||
								(kPanel_ResizeConstraintBothAxes == kConstraints));
	*outVerticalFlagPtr = ((kPanel_ResizeConstraintVertical == kConstraints) ||
								(kPanel_ResizeConstraintBothAxes == kConstraints));
}// popoverManager:getHorizontalResizeAllowed:getVerticalResizeAllowed:


/*!
Returns the dimensions the popover should initially have.

(2017.05)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getIdealSize:(NSSize*)					outSizePtr
{
#pragma unused(aPopoverManager)
	*outSizePtr = self.initialPanelSize;
}// popoverManager:getIdealSize:


/*!
Returns the proper position of the popover arrow tip (if any),
relative to its parent window; also called during window resizing.

(4.1)
*/
- (NSPoint)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealAnchorPointForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager)
	NSPoint		result = NSMakePoint(0, 0);
	
	
	if (nil != parentWindow)
	{
		NSRect		contentFrame = [parentWindow contentRectForFrameRect:parentFrame];
		
		
		result.x = CGFLOAT_DIV_2(NSWidth(parentFrame));
		result.y = ((parentFrame.origin.y - contentFrame.origin.y) + NSHeight(contentFrame));
		
		// minor adjustment for aesthetics
		result.y += 2;
	}
	else
	{
		// application-wide window with no parent; just center it
		NSScreen*		mainScreen = [NSScreen mainScreen];
		NSRect			screenFrame = mainScreen.frame;
		
		
		// a somewhat-arbitrary placement, typically for alerts
		// (TEMPORARY; this does not consider the actual size of
		// the popover frame, it puts all at the same position)
		result = NSMakePoint(screenFrame.origin.x + CGFLOAT_DIV_2(screenFrame.size.width),
								screenFrame.origin.y + screenFrame.size.height / 5.0f * 4.0f);
	}
	
	return result;
}// popoverManager:idealAnchorPointForFrame:parentWindow:


/*!
Returns the desired popover arrow placement.

(4.1)
*/
- (Popover_Properties)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentFrame, parentWindow)
	Popover_Properties		result = (kPopover_PropertyArrowMiddle |
										kPopover_PropertyPlaceFrameBelowArrow);
	
	
	return result;
}// popoverManager:idealArrowPositionForFrame:parentWindow:


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
	if ([self.mainViewManager respondsToSelector:@selector(changeColor:)])
	{
		[self.mainViewManager changeColor:sender];
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
	if ([self.mainViewManager respondsToSelector:@selector(changeFont:)])
	{
		[self.mainViewManager changeFont:sender];
	}
}// changeFont:


#pragma mark NSResponder


/*!
On OS 10.12.1 and beyond, returns a Touch Bar to display
at the top of the hardware keyboard (when available) or
in any Touch Bar simulator window.

This method should not be called except by the OS.

(2021.03)
*/
- (NSTouchBar*)
makeTouchBar
{
	NSTouchBar*		result = self.dynamicTouchBar;
	
	
	if (nil == result)
	{
		My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), self.dialogRef);
		
		
		// TEMPORARY; for an unknown reason, buttons do not work if
		// the dialog is in an application-modal state (even though
		// they do appear); for now, allow this only for sheets
		if (nil == ptr->modalToView)
		{
			// do not use the Touch Bar for application-modal dialogs
		}
		else
		{
			NSMutableArray< NSTouchBarItem* >*	buttonItems = [[NSMutableArray< NSTouchBarItem* > alloc] init];
			NSButtonTouchBarItem*		button1Item = [NSButtonTouchBarItem
														buttonTouchBarItemWithIdentifier:kConstantsRegistry_TouchBarItemIDGenericButton1
																							title:self.primaryButtonName
																							target:self
																							action:@selector(performPrimaryButtonAction:)];
			NSButtonTouchBarItem*		button2Item = [NSButtonTouchBarItem
														buttonTouchBarItemWithIdentifier:kConstantsRegistry_TouchBarItemIDGenericButton2
																							title:self.secondButtonName
																							target:self
																							action:@selector(performSecondButtonAction:)];
			NSButtonTouchBarItem*		button3Item = [NSButtonTouchBarItem
														buttonTouchBarItemWithIdentifier:kConstantsRegistry_TouchBarItemIDGenericButton3
																							title:self.thirdButtonName
																							target:self
																							action:@selector(performThirdButtonAction:)];
			NSButtonTouchBarItem*		helpItem = nil;
			
			
			if (@available(macOS 11.0, *))
			{
				helpItem = [NSButtonTouchBarItem buttonTouchBarItemWithIdentifier:kConstantsRegistry_TouchBarItemIDHelpButton
																					image:[NSImage imageWithSystemSymbolName:@"questionmark.circle"
																																accessibilityDescription:@"?"]
																					target:self
																					action:@selector(performHelpButtonAction:)];
			}
			else
			{
				helpItem = [NSButtonTouchBarItem buttonTouchBarItemWithIdentifier:kConstantsRegistry_TouchBarItemIDHelpButton
																					title:@"?"
																					target:self
																					action:@selector(performHelpButtonAction:)];
			}
			
			self.dynamicTouchBar = result = [[NSTouchBar alloc] init];
			//result.customizationIdentifier = kConstantsRegistry_TouchBarIDGenericDialog;
			//result.customizationAllowedItemIdentifiers = @[...];
			if (self.panelHasContextualHelp)
			{
				// help button
				[buttonItems addObject:helpItem];
			}
			if ((nil != self.primaryButtonName) && (nil != self.secondButtonName) && (nil != self.thirdButtonName))
			{
				// three buttons
				[buttonItems addObject:button3Item];
				[buttonItems addObject:button2Item];
				[buttonItems addObject:button1Item];
			}
			else if ((nil != self.primaryButtonName) && (nil != self.secondButtonName))
			{
				// two buttons
				[buttonItems addObject:button2Item];
				[buttonItems addObject:button1Item];
			}
			else if (nil != self.primaryButtonName)
			{
				// one button
				[buttonItems addObject:button1Item];
			}
			NSGroupTouchBarItem*	buttonGroupItem = [NSGroupTouchBarItem groupItemWithIdentifier:kConstantsRegistry_TouchBarItemIDGenericButtonGroup
																									items:buttonItems];
			//result.templateItems = [NSSet setWithArray:buttonItems];
			result.templateItems = [NSSet setWithArray:@[ buttonGroupItem ]];
			result.defaultItemIdentifiers =
			@[
				NSTouchBarItemIdentifierFlexibleSpace,
				kConstantsRegistry_TouchBarItemIDGenericButtonGroup,
				NSTouchBarItemIdentifierFlexibleSpace,
				NSTouchBarItemIdentifierOtherItemsProxy,
			];
			result.principalItemIdentifier = kConstantsRegistry_TouchBarItemIDGenericButtonGroup;
			
			helpItem.bezelColor = [NSColor clearColor];
			
			if (kGenericDialog_ItemIDButton3 == self.harmfulActionItemID)
			{
				button3Item.bezelColor = [NSColor systemRedColor];
			}
			
			if (kGenericDialog_ItemIDButton2 == self.harmfulActionItemID)
			{
				button2Item.bezelColor = [NSColor systemRedColor];
			}
			
			if (kGenericDialog_ItemIDButton1 == self.harmfulActionItemID)
			{
				button1Item.bezelColor = [NSColor systemRedColor];
			}
			else
			{
				button1Item.bezelColor = [NSColor controlAccentColor];
			}
		}
	}
	
	return result;
}// makeTouchBar


#pragma mark Notifications


/*!
Invoked when the child view changes its ideal size (that is,
"panelViewManager:requestingIdealSize:" returns a new value).

IMPORTANT:	This only adapts the initial size, as it is only
			expected to change prior to display.  It cannot
			adapt later.

(4.1)
*/
- (void)
childViewIdealSizeDidChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	// this should be in sync with any initialization code
	NSSize		oldSize = self.idealManagedViewSize;
	
	
	// determine the new preferred size
	[self.mainViewManager.delegate panelViewManager:self.mainViewManager requestingIdealSize:&_idealManagedViewSize];
	
	// tweak the initial panel size so that anything else based
	// on this (such as the window layout) is set up correctly
	self.initialPanelSize = CGSizeMake(self.initialPanelSize.width + self.idealManagedViewSize.width - oldSize.width,
										self.initialPanelSize.height + self.idealManagedViewSize.height - oldSize.height);
	
	// notify listeners of this change
	[self postNote:kPanel_IdealSizeDidChangeNotification];
}// childViewIdealSizeDidChange:


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
Invoked when the user clicks a button in the dialog.  See
the action properties, such as "primaryButtonAction".

(4.1)
*/
- (void)
performActionFrom:(id)				sender
forButton:(GenericDialog_ItemID)	aButton
{
#pragma unused(sender)
	// buttons cannot reasonably perform any actions while the
	// dialog is not yet defined
	if (nullptr != self.dialogRef)
	{
		BOOL	userAccepted = (kGenericDialog_ItemIDButton1 == aButton);
		BOOL	keepDialog = ((kGenericDialog_ItemIDHelpButton == aButton) ||
								(kGenericDialog_ItemIDButton3 == aButton));
		
		
		// locally lock/unlock the object in case the subsequent block
		// decides to destroy the dialog
		{
			My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), self.dialogRef);
			BOOL						hasCustomEffect = (ptr->closeEffects.end() != ptr->closeEffects.find(aButton));
			
			
			// determine if the dialog should be automatically destroyed
			if (hasCustomEffect)
			{
				// respect any custom behaviors
				keepDialog = (kGenericDialog_DialogEffectNone == ptr->closeEffects[aButton]);
				if (kGenericDialog_DialogEffectCloseImmediately == ptr->closeEffects[aButton])
				{
					PopoverManager_SetAnimationType(ptr->popoverManager, kPopoverManager_AnimationTypeNone);
				}
			}
		}
		
		if (userAccepted)
		{
			// ensure that any active text edits are applied
			[[NSApp keyWindow] makeFirstResponder:nil];
		}
		
		if (NO == keepDialog)
		{
			// inform the panel that it has finished
			[self panelViewManager:self.mainViewManager
									didFinishUsingContainerView:self.mainViewManager.managedView
									userAccepted:userAccepted];
		}
		
		// perform any action associated with the button; note that
		// action blocks ought not to destroy the dialog (technically
		// they can but it is better for dialogs to use the routine
		// GenericDialog_SetItemDialogEffect() consistently)
		switch (aButton)
		{
		case kGenericDialog_ItemIDButton1:
			if (nil != self.primaryButtonBlock)
			{
				self.primaryButtonBlock();
			}
			break;
		
		case kGenericDialog_ItemIDButton2:
			if (nil != self.secondButtonBlock)
			{
				self.secondButtonBlock();
			}
			break;
		
		case kGenericDialog_ItemIDButton3:
			if (nil != self.thirdButtonBlock)
			{
				self.thirdButtonBlock();
			}
			break;
		
		case kGenericDialog_ItemIDHelpButton:
			if (nil != self.helpButtonBlock)
			{
				self.helpButtonBlock();
			}
			break;
		
		default:
			// ???
			break;
		}
		
		// if required, remove the dialog from view
		if (NO == keepDialog)
		{
			// hide the dialog and (if application-modal) end the modal session
			{
				My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), self.dialogRef);
				
				
				if (nil != ptr->popoverManager)
				{
					PopoverManager_RemovePopover(ptr->popoverManager, userAccepted);
				}
				
				// if a modal session is in progress, end it
				if (nil == ptr->modalToView)
				{
					// TEMPORARY; might want to use NSModalSession objects
					// so that this cannot end just any open dialog
					[NSApp stopModal];
				}
			}
			
			// allow the caller to perform any custom actions prior to the
			// release of the dialog
			if (nil != self.cleanupBlock)
			{
				self.cleanupBlock();
			}
			
			// be paranoid and force the window to hide since that is the
			// intent, regardless of the retain-count of the dialog; this is
			// done after a brief delay because the action that makes the
			// window visible after a button press may not have even happened
			// yet (e.g. a scenario that was found: if user hits a button key
			// equivalent very quickly as the window is animating open, the
			// dialog is released but end-of-animation causes the window to
			// stay open, unable to respond to any further user input); in
			// the future this might be avoided by holding references to
			// animations and explicitly ending them if a window close is
			// initiated by the user, instead of having background tasks that
			// set incorrect window states
			{
				NSWindow __weak*	window = self.mainViewManager.view.window;
				
				
				CocoaExtensions_RunLater(0.5/* arbitrary (exceed animation time) */,
											^{
												[window orderOut:NSApp];
											});
			}
			
			// release the dialog (this must happen outside any lock-block);
			// note that this could destroy the object if there are no other
			// retain calls in effect
			GenericDialog_Release(&_dialogRef);
		}
	}
}// performActionFrom:forButton:


/*!
A helper to make string-setters less cumbersome to write.

(4.1)
*/
- (void)
setStringProperty:(NSString* __strong*)		propertyPtr
withName:(NSString*)						propertyName
toValue:(NSString*)							aString
{
	if (aString != *propertyPtr)
	{
		[self willChangeValueForKey:propertyName];
		
		if (nil == aString)
		{
			*propertyPtr = @"";
		}
		else
		{
			*propertyPtr = [aString copy];
		}
		
		[self didChangeValueForKey:propertyName];
	}
}// setStringProperty:withName:toValue:


/*!
Automatically resizes all action buttons and lays them out
according to normal interface guidelines and the current
localization (right-to-left versus left-to-right).

In addition, on macOS 11 and beyond, buttons are larger if
this is an alert-style dialog.

(4.1)
*/
-
(void)
updateButtonLayout
{
	// do nothing unless views are loaded
	if (nil != self.actionButton)
	{
		NSArray<NSButton*>*			buttonArray = @[self.actionButton, self.cancelButton, self.otherButton];
		My_GenericDialogAutoLocker	ptr(gGenericDialogPtrLocks(), self.dialogRef);
		
		
		if ((nullptr != ptr) && (ptr->isAlert))
		{
			if (@available(macOS 11.0, *))
			{
				for (NSButton* aButton in buttonArray)
				{
					aButton.controlSize = NSControlSizeLarge;
				}
				self.helpButton.controlSize = NSControlSizeLarge;
				[self.helpButton sizeToFit];
			}
		}
		
		Localization_ArrangeNSButtonArray(BRIDGE_CAST(buttonArray, CFArrayRef));
		Localization_AdjustHelpNSButton(self.helpButton);
	}
}// updateButtonLayout


@end // GenericDialog_ViewManager (GenericDialog_ViewManagerInternal)


// BELOW IS REQUIRED NEWLINE TO END FILE
