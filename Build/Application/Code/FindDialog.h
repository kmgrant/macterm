/*!	\file FindDialog.h
	\brief Used to perform searches in the scrollback
	buffers of terminal windows.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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
#ifdef __OBJC__
#	include <Cocoa/Cocoa.h>
#endif
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// library includes
#ifdef __OBJC__
#	import <PopoverManager.objc++.h>
#endif

// application includes
#include "TerminalWindowRef.typedef.h"



#pragma mark Constants

typedef UInt16 FindDialog_Options;
enum
{
	kFindDialog_OptionsAllOff				= 0,
	kFindDialog_OptionCaseInsensitive		= (1 << 0),
	kFindDialog_OptionAllOpenTerminals		= (1 << 1),
	kFindDialog_OptionNotFinal				= (1 << 2), // internal option
	kFindDialog_OptionDoNotScrollToMatch	= (1 << 3), // internal option
	kFindDialog_OptionRegularExpression		= (1 << 4),
	kFindDialog_OptionsDefault				= kFindDialog_OptionCaseInsensitive
};

enum FindDialog_SearchContext
{
	kFindDialog_SearchContextLocal			= 0,	//!< current window
	kFindDialog_SearchContextGlobal			= 1		//!< all windows
};

#pragma mark Types

#ifdef __OBJC__

@class FindDialog_VC;

/*!
Classes that are delegates of FindDialog_VC
must conform to this protocol.
*/
@protocol FindDialog_VCDelegate //{

	// use this opportunity to create and display a window to wrap the Find view
	- (void)
	findDialog:(FindDialog_VC* _Nonnull)_
	didLoadManagedView:(NSView* _Nonnull)_;

	// remove search highlighting
	- (void)
	findDialog:(FindDialog_VC* _Nonnull)_
	clearSearchHighlightingInContext:(FindDialog_SearchContext)_;

	// perform the search yourself, then call the view manager’s "updateUserInterfaceWithMatches:didSearch:"
	- (void)
	findDialog:(FindDialog_VC* _Nonnull)_
	didSearchInManagedView:(NSView* _Nonnull)_
	withQuery:(NSString* _Nullable)_;

	// perform a search yourself, but no need to update the user interface since it should be destroyed
	- (void)
	findDialog:(FindDialog_VC* _Nonnull)_
	didFinishUsingManagedView:(NSView* _Nonnull)_
	acceptingSearch:(BOOL)_
	finalOptions:(FindDialog_Options)_;

@end //}


/*!
Implements the Find interface.  See "FindDialogCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface FindDialog_VC : NSViewController< NSTextFieldDelegate > //{

// initializers
	- (instancetype _Nullable)
	initForTerminalWindow:(TerminalWindowRef _Nonnull)_
	responder:(id< FindDialog_VCDelegate > _Nonnull)_
	initialOptions:(FindDialog_Options)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (NSView* _Nonnull)
	logicalFirstResponder;
	- (void)
	updateUserInterfaceWithMatches:(UInt32)_
	didSearch:(BOOL)_;

// actions
	- (IBAction)
	orderFrontContextualHelp:(id _Nullable)_;
	- (IBAction)
	performCloseAndRevert:(id _Nullable)_;
	- (IBAction)
	performCloseAndSearch:(id _Nullable)_;
	- (IBAction)
	performSearch:(id _Nullable)_;

// accessors
	//! Determines value of “A=a” checkbox.
	@property (assign) BOOL
	caseInsensitiveSearch; // binding
	//! Determines value of “All Windows” scope.
	@property (assign) BOOL
	multiTerminalSearch; // binding
	//! Determines value of “Regex” checkbox.
	@property (assign) BOOL
	regularExpressionSearch; // binding
	//! Non-UI property that should be consistent with scope
	//! (either current window or all windows).
	@property (readonly) FindDialog_SearchContext
	searchContext;
	//! View that contains the search query text and
	//! a menu of recent searches.
	@property (strong, nonnull) IBOutlet NSSearchField*
	searchField;
	//! Determines visibility of search progress indicator.
	@property (assign) BOOL
	searchProgressHidden; // binding
	//! Determines hidden state of warning icon (set if there
	//! is at least one match for the query, or no query set).
	@property (assign) BOOL
	successfulSearch; // binding
	//! Raw query string, which is interpreted based on other
	//! search parameters (e.g. "caseInsensitiveSearch" or
	//! "regularExpressionSearch").
	@property (strong, nullable) NSString*
	searchText; // binding
	//! Status message, such as an error when no match is found
	//! or text that displays the number of matches.
	@property (strong, nullable) NSString*
	statusText; // binding

@end //}


/*!
Allows field actions to affect the search panel state.
*/
@interface FindDialog_SearchField : NSSearchField //{

// accessors
	//! The parent view controller.
	@property (weak, nullable) IBOutlet FindDialog_VC*
	viewManager;

@end //}


/*!
Manages the Find user interface.
*/
@interface FindDialog_Object : NSObject< FindDialog_VCDelegate, PopoverManager_Delegate > @end

#else

class FindDialog_Object;

#endif // __OBJC__

// This is defined as an Objective-C object so it is compatible
// with ARC rules (e.g. strong references).
typedef FindDialog_Object*		FindDialog_Ref;



#pragma mark Callbacks

/*!
Find Dialog Close Notification Method

This is called when the Find interface is removed; respond in
any way required, e.g. saving state.
*/
typedef void (^FindDialog_OnCloseBlock)	(FindDialog_Ref _Nonnull, FindDialog_Options);



#pragma mark Public Methods

FindDialog_Ref _Nullable
	FindDialog_New					(TerminalWindowRef _Nonnull			inTerminalWindow,
									 FindDialog_OnCloseBlock _Nullable	inOnCloseBlock,
									 CFMutableArrayRef _Nonnull			inoutQueryStringHistory,
									 FindDialog_Options					inFlags = kFindDialog_OptionsAllOff);

void
	FindDialog_Display				(FindDialog_Ref _Nonnull			inDialog);

UInt32
	FindDialog_SearchWithoutDialog	(CFStringRef _Nullable				inQueryBaseOrNullToClear,
									 TerminalWindowRef _Nonnull			inStartTerminalWindow,
									 FindDialog_Options					inFlags,
									 Boolean* _Nullable					outDidSearchOrNull = nullptr);

// BELOW IS REQUIRED NEWLINE TO END FILE
