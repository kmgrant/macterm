/*!	\file GenericDialog.h
	\brief Allows a user interface that is both a panel
	and a dialog to be displayed as a modal dialog or
	sheet.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#ifndef __GENERICDIALOG__
#define __GENERICDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSWindow;
#else
class NSWindow;
#endif

// library includes
#ifdef __OBJC__
#	import <PopoverManager.objc++.h>
#endif

// application includes
#include "HelpSystem.h"
#include "Panel.h"



#pragma mark Constants

enum GenericDialog_DialogEffect
{
	kGenericDialog_DialogEffectCloseNormally		= 0,	//!< sheet closes with animation
	kGenericDialog_DialogEffectCloseImmediately		= 1,	//!< sheet closes without animation (e.g. a Close button, or Cancel in rare cases)
	kGenericDialog_DialogEffectNone					= 2		//!< no effect on the sheet (e.g. command button)
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
	IBOutlet NSTabView*		viewContainer;
	IBOutlet NSButton*		actionButton;
	IBOutlet NSButton*		cancelButton;
	IBOutlet NSButton*		otherButton;
	IBOutlet NSButton*		helpButton;
	NSString*				_primaryActionButtonName;
	NSString*				_thirdButtonName;
	void					(^_thirdButtonBlock)();
	NSString*				identifier;
	NSString*				localizedName;
	NSImage*				localizedIcon;
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
	@property (strong) NSString*
	primaryActionButtonName;
	@property (strong) void
	(^thirdButtonBlock)();
	@property (strong) NSString*
	thirdButtonName;

// actions
	- (IBAction)
	performThirdButtonAction:(id)_;

@end //}

#endif // __OBJC__

#pragma mark Callbacks

/*!
Generic Dialog Close Notification Method

Invoked when a button is clicked that closes the
dialog.  Use this to perform any post-processing.
See also GenericDialog_StandardCloseNotifyProc().

You should NOT call GenericDialog_Dispose() in this
routine!
*/
typedef void	 (*GenericDialog_CloseNotifyProcPtr)	(GenericDialog_Ref		inDialogThatClosed,
														 Boolean				inOKButtonPressed);
inline void
GenericDialog_InvokeCloseNotifyProc		(GenericDialog_CloseNotifyProcPtr	inUserRoutine,
										 GenericDialog_Ref					inDialogThatClosed,
										 Boolean							inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

#ifdef __OBJC__
// NOTE: SPECIFIED VIEW MANAGER IS RETAINED BY THIS CALL
GenericDialog_Ref
	GenericDialog_New							(HIWindowRef						inParentWindowOrNullForModalDialog,
												 Panel_ViewManager*					inHostedPanel,
												 void*								inDataSetPtr,
												 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
												 HelpSystem_KeyPhrase				inHelpButtonAction = kHelpSystem_KeyPhraseDefault);
#endif

GenericDialog_Ref
	GenericDialog_New							(NSWindow*							inParentWindowOrNullForModalDialog,
												 Panel_Ref							inHostedPanel,
												 void*								inDataSetPtr,
												 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
												 HelpSystem_KeyPhrase				inHelpButtonAction = kHelpSystem_KeyPhraseDefault);

// DEPRECATED; USE GenericDialog_New() WITH AN NSWindow*
GenericDialog_Ref
	GenericDialog_New							(HIWindowRef						inParentWindowOrNullForModalDialog,
												 Panel_Ref							inHostedPanel,
												 void*								inDataSetPtr,
												 GenericDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
												 HelpSystem_KeyPhrase				inHelpButtonAction = kHelpSystem_KeyPhraseDefault);

void
	GenericDialog_Dispose						(GenericDialog_Ref*					inoutDialogPtr);

void
	GenericDialog_AddButton						(GenericDialog_Ref					inDialog,
												 CFStringRef						inButtonTitle,
												 void								(^inResponseBlock)());

void
	GenericDialog_Display						(GenericDialog_Ref					inDialog);

void*
	GenericDialog_ReturnImplementation			(GenericDialog_Ref					inDialog);

void
	GenericDialog_SetCommandButtonTitle			(GenericDialog_Ref					inDialog,
												 UInt32								inCommandID,
												 CFStringRef						inButtonTitle);

void
	GenericDialog_SetCommandDialogEffect		(GenericDialog_Ref					inDialog,
												 UInt32								inCommandID,
												 GenericDialog_DialogEffect			inEffect);

void
	GenericDialog_SetImplementation				(GenericDialog_Ref					inDialog,
												 void*								inDataPtr);

void
	GenericDialog_StandardCloseNotifyProc		(GenericDialog_Ref					inDialogThatClosed,
												 Boolean							inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
