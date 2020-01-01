/*!	\file GenericDialog.h
	\brief Allows a user interface that is both a panel
	and a dialog to be displayed as a modal dialog or
	sheet.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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
@class NSView;
#else
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
{
@private
	IBOutlet NSTabView*			viewContainer;
	IBOutlet CoreUI_Button*		actionButton;
	IBOutlet CoreUI_Button*		cancelButton;
	IBOutlet CoreUI_Button*		otherButton;
	IBOutlet CoreUI_HelpButton*	helpButton;
	void					(^_cleanupBlock)();
	void					(^_helpButtonBlock)();
	void					(^_primaryButtonBlock)();
	void					(^_secondButtonBlock)();
	void					(^_thirdButtonBlock)();
	NSString*				_primaryButtonName;
	NSString*				_secondButtonName;
	NSString*				_thirdButtonName;
	NSString*				identifier;
	NSString*				localizedName;
	NSImage*				localizedIcon;
	NSSize					idealManagedViewSize;
	NSSize					initialPanelSize;
	Panel_ViewManager*		mainViewManager;
	GenericDialog_Ref		dialogRef;
}

// initializers
	- (instancetype)
	initWithRef:(GenericDialog_Ref)_
	identifier:(NSString*)_
	localizedName:(NSString*)_
	localizedIcon:(NSImage*)_
	viewManager:(Panel_ViewManager*)_;

// accessors: user interface settings
	@property (copy) void
	(^cleanupBlock)();
	@property (copy) void
	(^helpButtonBlock)();
	@property (copy) void
	(^primaryButtonBlock)();
	@property (strong) NSString*
	primaryButtonName; // binding
	@property (copy) void
	(^secondButtonBlock)();
	@property (strong) NSString*
	secondButtonName; // binding
	@property (copy) void
	(^thirdButtonBlock)();
	@property (strong) NSString*
	thirdButtonName; // binding

// actions
	- (IBAction)
	performThirdButtonAction:(id)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

#ifdef __OBJC__
// NOTE: SPECIFIED VIEW MANAGER IS RETAINED BY THIS CALL
GenericDialog_Ref
	GenericDialog_New							(NSView*							inModalToViewOrNullForAppModalDialog,
												 Panel_ViewManager*					inHostedPanel,
												 void*								inDataSetPtr,
												 Boolean							inIsAlert = false);
#endif

void
	GenericDialog_Retain						(GenericDialog_Ref					inDialog);

void
	GenericDialog_Release						(GenericDialog_Ref*					inoutDialogPtr);

void
	GenericDialog_Display						(GenericDialog_Ref					inDialog,
												 Boolean							inAnimated,
												 void								(^inImplementationReleaseBlock)());

void
	GenericDialog_Remove						(GenericDialog_Ref					inDialog);

void*
	GenericDialog_ReturnImplementation			(GenericDialog_Ref					inDialog);

GenericDialog_DialogEffect
	GenericDialog_ReturnItemDialogEffect		(GenericDialog_Ref					inDialog,
												 GenericDialog_ItemID				inItemID);

Panel_ViewManager*
	GenericDialog_ReturnViewManager				(GenericDialog_Ref					inDialog);

void
	GenericDialog_SetDelayedKeyEquivalents		(GenericDialog_Ref					inDialog,
												 Boolean							inKeyEquivalentsDelayed);

void
	GenericDialog_SetImplementation				(GenericDialog_Ref					inDialog,
												 void*								inDataPtr);

void
	GenericDialog_SetItemDialogEffect			(GenericDialog_Ref					inDialog,
												 GenericDialog_ItemID				inItemID,
												 GenericDialog_DialogEffect			inEffect);

void
	GenericDialog_SetItemResponseBlock			(GenericDialog_Ref					inDialog,
												 GenericDialog_ItemID				inItemID,
												 void								(^inResponseBlock)());

void
	GenericDialog_SetItemTitle					(GenericDialog_Ref					inDialog,
												 GenericDialog_ItemID				inItemID,
												 CFStringRef						inButtonTitle);



#pragma mark Types Dependent on Method Names

// DO NOT USE DIRECTLY.
struct _GenericDialog_RefMgr
{
	typedef GenericDialog_Ref	reference_type;
	
	static void
	retain	(reference_type		inRef)
	{
		GenericDialog_Retain(inRef);
	}
	
	static void
	release	(reference_type		inRef)
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
