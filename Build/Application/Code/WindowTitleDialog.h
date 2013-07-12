/*!	\file WindowTitleDialog.h
	\brief Implements a dialog box for changing the title
	of a terminal window.
	
	The interface has the appearance of a popover window
	pointing at the current title in the window frame.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#ifndef __WINDOWTITLEDIALOG__
#define __WINDOWTITLEDIALOG__

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// application includes
#include "SessionRef.typedef.h"
#include "VectorWindowRef.typedef.h"



#pragma mark Types

typedef struct WindowTitleDialog_OpaqueStruct*		WindowTitleDialog_Ref;

#ifdef __OBJC__

@class WindowTitleDialog_ViewManager;

/*!
Classes that are delegates of WindowTitleDialog_ViewManager
must conform to this protocol.
*/
@protocol WindowTitleDialog_ViewManagerChannel //{

	// use this opportunity to create and display a window to wrap the Rename view
	- (void)
	titleDialog:(WindowTitleDialog_ViewManager*)_
	didLoadManagedView:(NSView*)_;

	// perform the window rename yourself, but no need to update the user interface since it should be destroyed
	- (void)
	titleDialog:(WindowTitleDialog_ViewManager*)_
	didFinishUsingManagedView:(NSView*)_
	acceptingRename:(BOOL)_
	finalTitle:(NSString*)_;

	// return an NSString* to use for the initial title text field value
	- (NSString*)
	titleDialog:(WindowTitleDialog_ViewManager*)_
	returnInitialTitleTextForManagedView:(NSView*)_;

@end //}


/*!
Implements the Rename interface.  See "WindowTitleDialogCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface WindowTitleDialog_ViewManager : NSObject //{
{
	IBOutlet NSView*		managedView;
	IBOutlet NSTextField*	titleField;
@private
	id< WindowTitleDialog_ViewManagerChannel >	responder;
	NSWindow*									parentCocoaWindow;
	HIWindowRef									parentCarbonWindow;
	NSString*									titleText;
}

// initializers
	- (id)
	initForCocoaWindow:(NSWindow*)_
	orCarbonWindow:(HIWindowRef)_
	responder:(id< WindowTitleDialog_ViewManagerChannel >)_; // designated initializer
	- (id)
	initForCarbonWindow:(HIWindowRef)_
	responder:(id< WindowTitleDialog_ViewManagerChannel >)_;
	- (id)
	initForCocoaWindow:(NSWindow*)_
	responder:(id< WindowTitleDialog_ViewManagerChannel >)_;

// new methods
	- (NSView*)
	logicalFirstResponder;

// accessors
	- (NSString*)
	titleText; // binding
	- (void)
	setTitleText:(NSString*)_;

// actions
	- (IBAction)
	performCloseAndRename:(id)_;
	- (IBAction)
	performCloseAndRevert:(id)_;

@end //}

#endif // __OBJC__

#pragma mark Callbacks

/*!
Window Title Dialog Close Notification Method

When a window title dialog is closed, this method
is invoked.  Use this to know exactly when it is
safe to call WindowTitleDialog_Dispose().
*/
typedef void	 (*WindowTitleDialog_CloseNotifyProcPtr)	(WindowTitleDialog_Ref	inDialogThatClosed,
															 Boolean				inOKButtonPressed);
inline void
WindowTitleDialog_InvokeCloseNotifyProc		(WindowTitleDialog_CloseNotifyProcPtr	inUserRoutine,
											 WindowTitleDialog_Ref					inDialogThatClosed,
											 Boolean								inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

void
	WindowTitleDialog_StandardCloseNotifyProc	(WindowTitleDialog_Ref					inDialogThatClosed,
												 Boolean								inOKButtonPressed);

WindowTitleDialog_Ref
	WindowTitleDialog_NewForSession				(SessionRef								inSession,
												 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr =
																							WindowTitleDialog_StandardCloseNotifyProc);

WindowTitleDialog_Ref
	WindowTitleDialog_NewForVectorCanvas		(VectorWindow_Ref						inCanvasWindow,
												 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr =
																							WindowTitleDialog_StandardCloseNotifyProc);

void
	WindowTitleDialog_Dispose					(WindowTitleDialog_Ref*					inoutDialogPtr);

void
	WindowTitleDialog_Display					(WindowTitleDialog_Ref					inDialog);

void
	WindowTitleDialog_Remove					(WindowTitleDialog_Ref					inDialog);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
