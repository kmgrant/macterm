/*!	\file GenericDialog.h
	\brief Allows a user interface that is both a panel
	and a dialog to be displayed as a modal dialog or
	sheet.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSObject;
@class NSView;
#else
class NSObject;
class NSView;
#endif

// library includes
#ifdef __OBJC__
#	import <CoreUI.objc++.h>
#	import <PopoverManager.objc++.h>
@class CoreUI_Button;
@class CoreUI_HelpButton;
#endif
#include <RetainRelease.template.h>

// application includes
#include "Panel.h"



#pragma mark Constants

enum GenericDialog_DialogEffect
{
	kGenericDialog_DialogEffectCloseNormally		= 0,	//!< sheet closes with animation
	kGenericDialog_DialogEffectCloseImmediately		= 1,	//!< sheet closes without animation (e.g. a Close button, or Cancel in rare cases)
	kGenericDialog_DialogEffectNone					= 2		//!< no effect on the sheet (e.g. command button)
};

enum GenericDialog_ItemID
{
	kGenericDialog_ItemIDNone = 0,			//!< no item
	kGenericDialog_ItemIDButton1 = 1,		//!< primary button (typically “OK”)
	kGenericDialog_ItemIDButton2 = 2,		//!< second button (typically “Cancel”)
	kGenericDialog_ItemIDButton3 = 3,		//!< third button (e.g. “Don’t Save”)
	kGenericDialog_ItemIDHelpButton = 4		//!< help button
};

#pragma mark Types

typedef struct GenericDialog_OpaqueRef*		GenericDialog_Ref;


/*!
Block used for responding to button clicks in dialogs.
*/
typedef void (^GenericDialog_ButtonActionBlock)(void);


/*!
Block used for tearing down a dialog’s custom implementation.
*/
typedef void (^GenericDialog_CleanupBlock)(void);


#ifdef __OBJC__

/*!
Loads a NIB file that defines this panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface GenericDialog_ViewManager : Panel_ViewManager< Panel_Delegate,
															Panel_Parent,
															PopoverManager_Delegate > //{

// initializers
	- (instancetype _Nullable)
	initWithRef:(GenericDialog_Ref _Nonnull)_
	identifier:(NSString* _Nonnull)_
	localizedName:(NSString* _Nonnull)_
	localizedIcon:(NSImage* _Nonnull)_
	viewManager:(Panel_ViewManager* _Nonnull)_ NS_DESIGNATED_INITIALIZER;

// accessors: user interface configuration and responders
	@property (strong, nullable) GenericDialog_CleanupBlock
	cleanupBlock;
	@property (assign) GenericDialog_ItemID
	harmfulActionItemID;
	@property (strong, nullable) GenericDialog_ButtonActionBlock
	helpButtonBlock;
	@property (strong, nullable) GenericDialog_ButtonActionBlock
	primaryButtonBlock;
	@property (strong, nullable) NSString*
	primaryButtonName; // binding
	@property (strong, nullable) GenericDialog_ButtonActionBlock
	secondButtonBlock;
	@property (strong, nullable) NSString*
	secondButtonName; // binding
	@property (strong, nullable) GenericDialog_ButtonActionBlock
	thirdButtonBlock;
	@property (strong, nullable) NSString*
	thirdButtonName; // binding

// accessors: XIB outlets
	@property (strong, nonnull) IBOutlet CoreUI_Button*
	actionButton;
	@property (strong, nonnull) IBOutlet CoreUI_Button*
	cancelButton;
	@property (strong, nonnull) IBOutlet CoreUI_HelpButton*
	helpButton;
	@property (strong, nonnull) IBOutlet CoreUI_Button*
	otherButton;
	@property (strong, nonnull) IBOutlet NSTabView*
	viewContainer;

// actions
	- (IBAction)
	performHelpButtonAction:(id _Nullable)_;
	- (IBAction)
	performPrimaryButtonAction:(id _Nullable)_;
	- (IBAction)
	performSecondButtonAction:(id _Nullable)_;
	- (IBAction)
	performThirdButtonAction:(id _Nullable)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

#ifdef __OBJC__
// NOTE: SPECIFIED VIEW MANAGER IS RETAINED BY THIS CALL
GenericDialog_Ref _Nullable
	GenericDialog_New							(NSView* _Nullable					inModalToViewOrNullForAppModalDialog,
												 Panel_ViewManager* _Nonnull		inHostedPanel,
												 void* _Nullable					inDataSetPtr,
												 Boolean							inIsAlert = false);
#endif

void
	GenericDialog_Retain						(GenericDialog_Ref _Nonnull			inDialog);

void
	GenericDialog_Release						(GenericDialog_Ref _Nonnull* _Nonnull		inoutDialogPtr);

void
	GenericDialog_Display						(GenericDialog_Ref _Nonnull			inDialog,
												 Boolean							inAnimated,
												 GenericDialog_CleanupBlock _Nullable	inImplementationReleaseBlock);

void
	GenericDialog_Remove						(GenericDialog_Ref _Nonnull			inDialog);

void* _Nullable
	GenericDialog_ReturnImplementation			(GenericDialog_Ref _Nonnull			inDialog);

NSObject* _Nullable
	GenericDialog_ReturnImplementationNSObject	(GenericDialog_Ref _Nonnull			inDialog);

GenericDialog_DialogEffect
	GenericDialog_ReturnItemDialogEffect		(GenericDialog_Ref _Nonnull			inDialog,
												 GenericDialog_ItemID				inItemID);

Panel_ViewManager* _Nullable
	GenericDialog_ReturnViewManager				(GenericDialog_Ref _Nonnull			inDialog);

void
	GenericDialog_SetDelayedKeyEquivalents		(GenericDialog_Ref _Nonnull			inDialog,
												 Boolean							inKeyEquivalentsDelayed);

void
	GenericDialog_SetImplementation				(GenericDialog_Ref _Nonnull			inDialog,
												 void* _Nullable					inDataPtr);

void
	GenericDialog_SetImplementationNSObject		(GenericDialog_Ref _Nonnull			inDialog,
												 NSObject* _Nullable				inDataPtr);

void
	GenericDialog_SetItemDialogEffect			(GenericDialog_Ref _Nonnull			inDialog,
												 GenericDialog_ItemID				inItemID,
												 GenericDialog_DialogEffect			inEffect);

void
	GenericDialog_SetItemResponseBlock			(GenericDialog_Ref _Nonnull			inDialog,
												 GenericDialog_ItemID				inItemID,
												 GenericDialog_ButtonActionBlock _Nullable	inResponseBlock,
												 Boolean							inIsHarmfulAction = false);

void
	GenericDialog_SetItemTitle					(GenericDialog_Ref _Nonnull			inDialog,
												 GenericDialog_ItemID				inItemID,
												 CFStringRef _Nullable				inButtonTitle);



#pragma mark Types Dependent on Method Names

// DO NOT USE DIRECTLY.
struct _GenericDialog_RefMgr
{
	typedef GenericDialog_Ref	reference_type;
	
	static void
	retain	(reference_type _Nonnull	inRef)
	{
		GenericDialog_Retain(inRef);
	}
	
	static void
	release	(reference_type _Nonnull	inRef)
	{
		GenericDialog_Release(&inRef);
	}
};

/*!
Allows RAII-based automatic retain and release of a dialog so
you don’t have to call GenericDialog_Release() yourself.  Simply
declare a variable of this type (in a data structure, say),
initialize it as appropriate, and your reference is safe.  Note
that there is a constructor that allows you to store pre-retained
(e.g. newly allocated) references too.
*/
typedef RetainRelease< _GenericDialog_RefMgr >		GenericDialog_Wrap;

// BELOW IS REQUIRED NEWLINE TO END FILE
