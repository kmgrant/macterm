/*!	\file WindowTitleDialog.h
	\brief Implements a dialog box for changing the title
	of a terminal window.
	
	The interface has the appearance of a popover window
	pointing at the current title in the window frame.
*/
/*###############################################################

	Interface Library
	© 1998-2019 by Kevin Grant
	
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef struct WindowTitleDialog_OpaqueStruct*		WindowTitleDialog_Ref;

/*!
A block that is invoked to retrieve a window title
when initializing the dialog for new use.  Typically
this is implemented by reading some parent window’s
current title string.
*/
typedef CFStringRef (^WindowTitleDialog_ReturnTitleCopyBlock)();

/*!
A block that is invoked when the dialog is closed.
If the user accepted, a non-nullptr string will be
given; typically this is used to update some parent
window’s title.
*/
typedef void (^WindowTitleDialog_CloseNotifyBlock)(CFStringRef/* new title or nullptr when cancelled */);

#ifdef __OBJC__

@class WindowTitleDialog_VC;

/*!
Classes that are delegates of WindowTitleDialog_VC
must conform to this protocol.
*/
@protocol WindowTitleDialog_VCDelegate //{

	// use this opportunity to create and display a window to wrap the Rename view
	- (void)
	titleDialog:(WindowTitleDialog_VC*)_
	didLoadManagedView:(NSView*)_;

	// perform the window rename yourself, but no need to update the user interface since it should be destroyed
	- (void)
	titleDialog:(WindowTitleDialog_VC*)_
	didFinishUsingManagedView:(NSView*)_
	acceptingRename:(BOOL)_
	finalTitle:(NSString*)_;

	// return an NSString* to use for the initial title text field value
	- (NSString*)
	titleDialog:(WindowTitleDialog_VC*)_
	returnInitialTitleTextForManagedView:(NSView*)_;

@end //}


/*!
Implements the Rename interface.  See "WindowTitleDialogCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface WindowTitleDialog_VC : NSViewController //{
{
	IBOutlet NSTextField*	titleField;
@private
	id< WindowTitleDialog_VCDelegate >	_responder;
	NSWindow*							_parentCocoaWindow;
	NSString*							_titleText;
}

// initializers
	- (instancetype)
	initForCocoaWindow:(NSWindow*)_
	responder:(id< WindowTitleDialog_VCDelegate >)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (NSView*)
	logicalFirstResponder;

// accessors
	@property (strong) NSString*
	titleText; // binding

// actions
	- (IBAction)
	performCloseAndRename:(id)_;
	- (IBAction)
	performCloseAndRevert:(id)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

WindowTitleDialog_Ref
	WindowTitleDialog_NewWindowModal					(NSWindow*								inCocoaParentWindow,
													 Boolean									inIsAnimated,
													 WindowTitleDialog_ReturnTitleCopyBlock	inInitBlock,
													 WindowTitleDialog_CloseNotifyBlock		inFinalBlock);

void
	WindowTitleDialog_Dispose						(WindowTitleDialog_Ref*					inoutDialogPtr);

void
	WindowTitleDialog_Display						(WindowTitleDialog_Ref					inDialog);

// BELOW IS REQUIRED NEWLINE TO END FILE
