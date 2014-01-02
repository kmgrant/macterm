/*!	\file FindDialog.h
	\brief A sheet used to perform searches in the scrollback
	buffers of terminal windows.
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#ifndef __FINDDIALOG__
#define __FINDDIALOG__

// Mac includes
#ifdef __OBJC__
#	include <Cocoa/Cocoa.h>
#endif
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// application includes
#include "TerminalWindowRef.typedef.h"



#pragma mark Constants

typedef UInt16 FindDialog_Options;
enum
{
	kFindDialog_OptionsAllOff				= 0,
	kFindDialog_OptionCaseInsensitive		= (1 << 0),
	kFindDialog_OptionAllOpenTerminals		= (1 << 1),
	kFindDialog_OptionsDefault				= kFindDialog_OptionCaseInsensitive
};

enum FindDialog_SearchContext
{
	kFindDialog_SearchContextLocal			= 0,	//!< current window
	kFindDialog_SearchContextGlobal			= 1		//!< all windows
};

#pragma mark Types

typedef struct FindDialog_OpaqueStruct*		FindDialog_Ref;

#ifdef __OBJC__

@class FindDialog_ViewManager;

/*!
Classes that are delegates of FindDialog_ViewManager
must conform to this protocol.
*/
@protocol FindDialog_ViewManagerChannel //{

	// use this opportunity to create and display a window to wrap the Find view
	- (void)
	findDialog:(FindDialog_ViewManager*)_
	didLoadManagedView:(NSView*)_;

	// remove search highlighting
	- (void)
	findDialog:(FindDialog_ViewManager*)_
	clearSearchHighlightingInContext:(FindDialog_SearchContext)_;

	// perform the search yourself, then call the view manager’s "updateUserInterfaceWithMatches:didSearch:"
	- (void)
	findDialog:(FindDialog_ViewManager*)_
	didSearchInManagedView:(NSView*)_
	withQuery:(NSString*)_;

	// perform a search yourself, but no need to update the user interface since it should be destroyed
	- (void)
	findDialog:(FindDialog_ViewManager*)_
	didFinishUsingManagedView:(NSView*)_
	acceptingSearch:(BOOL)_
	finalOptions:(FindDialog_Options)_;

	// return an array of NSString* to use for previous searches
	- (NSMutableArray*)
	findDialog:(FindDialog_ViewManager*)_
	returnHistoryArrayForManagedView:(NSView*)_;

@end //}


/*!
Implements the Find interface.  See "FindDialogCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface FindDialog_ViewManager : NSObject //{
{
	IBOutlet NSImageView*		failureIcon;
	IBOutlet NSTextField*		failureText;
	IBOutlet NSView*			managedView;
	IBOutlet NSSearchField*		searchField;
@private
	id< FindDialog_ViewManagerChannel >		responder;
	TerminalWindowRef						terminalWindow;
	NSMutableArray*							searchHistory;
	NSString*								searchText;
	NSString*								statusText;
	BOOL									caseInsensitiveSearch;
	BOOL									multiTerminalSearch;
	BOOL									searchProgressHidden;
	BOOL									successfulSearch;
}

// initializers
	- (id)
	initForTerminalWindow:(TerminalWindowRef)_
	responder:(id< FindDialog_ViewManagerChannel >)_
	initialOptions:(FindDialog_Options)_;

// new methods
	- (NSView*)
	logicalFirstResponder;
	- (NSSearchField*)
	searchField;
	- (void)
	updateUserInterfaceWithMatches:(unsigned long)_
	didSearch:(BOOL)_;

// actions
	- (IBAction)
	performContextSensitiveHelp:(id)_;
	- (IBAction)
	performCloseAndRevert:(id)_;
	- (IBAction)
	performCloseAndSearch:(id)_;
	- (IBAction)
	performSearch:(id)_;

// accessors
	- (BOOL)
	isCaseInsensitiveSearch; // binding
	- (void)
	setCaseInsensitiveSearch:(BOOL)_;
	- (BOOL)
	isMultiTerminalSearch; // binding
	- (void)
	setMultiTerminalSearch:(BOOL)_;
	- (BOOL)
	isSearchProgressHidden; // binding
	- (void)
	setSearchProgressHidden:(BOOL)_;
	- (BOOL)
	isSuccessfulSearch; // binding
	- (void)
	setSuccessfulSearch:(BOOL)_;
	- (NSString*)
	searchText; // binding
	- (void)
	setSearchText:(NSString*)_;
	- (NSString*)
	statusText; // binding
	- (void)
	setStatusText:(NSString*)_;

@end //}

#endif // __OBJC__

#pragma mark Callbacks

/*!
Find Dialog Close Notification Method

When a Find dialog is closed, this method is
invoked.  Use this to know exactly when it is
safe to call FindDialog_Dispose().
*/
typedef void	 (*FindDialog_CloseNotifyProcPtr)	(FindDialog_Ref		inDialogThatClosed);
inline void
FindDialog_InvokeCloseNotifyProc	(FindDialog_CloseNotifyProcPtr	inUserRoutine,
									 FindDialog_Ref					inDialogThatClosed)
{
	(*inUserRoutine)(inDialogThatClosed);
}



#pragma mark Public Methods

FindDialog_Ref
	FindDialog_New						(TerminalWindowRef				inTerminalWindow,
										 FindDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
										 CFMutableArrayRef				inoutQueryStringHistory,
										 FindDialog_Options				inFlags = kFindDialog_OptionsAllOff);

void
	FindDialog_Dispose					(FindDialog_Ref*				inoutDialogPtr);

void
	FindDialog_Display					(FindDialog_Ref					inDialog);

void
	FindDialog_Remove					(FindDialog_Ref					inDialog);

// ONLY VALID TO CALL FROM YOUR "FindDialog_CloseNotifyProcPtr"
FindDialog_Options
	FindDialog_ReturnOptions			(FindDialog_Ref					inDialog);

// ONLY VALID TO CALL FROM YOUR "FindDialog_CloseNotifyProcPtr"
TerminalWindowRef
	FindDialog_ReturnTerminalWindow		(FindDialog_Ref					inDialog);

void
	FindDialog_StandardCloseNotifyProc	(FindDialog_Ref					inDialogThatClosed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
