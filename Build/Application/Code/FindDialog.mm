/*!	\file FindDialog.mm
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

#import "FindDialog.h"
#import <UniversalDefines.h>

// standard-C includes
#import <climits>

// standard-C++ includes
#import <vector>

// Mac includes
#import <objc/objc-runtime.h>
@import Cocoa;

// library includes
#import <AlertMessages.h>
#import <CFRetainRelease.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <MemoryBlocks.h>
#import <Popover.objc++.h>
#import <PopoverManager.objc++.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "FindDialog.h"
#import "HelpSystem.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "UIStrings.h"



#pragma mark Types

/*!
Private properties.
*/
@interface FindDialog_Object () //{

// accessors
	//! Options set when the user interface is closed.
	@property (assign) FindDialog_Options
	cachedOptions;
	//! Holds the Find dialog view.
	@property (strong) Popover_Window*
	containerWindow;
	//! When searches are performed, this captures a few previous query strings.
	@property (strong) NSMutableArray*/* of NSString* */
	historyArray;
	//! Initial size of view.
	@property (assign) NSSize
	idealViewSize;
	//! The view that implements the majority of the interface.
	@property (strong) NSView*
	managedView;
	//! Block to run when the dialog is dismissed.
	@property (strong) FindDialog_OnCloseBlock
	onCloseBlock;
	//! Manages common aspects of popover window behavior.
	@property (strong) PopoverManager_Ref
	popoverMgr;
	//! Parent terminal window whose terminal screens are being searched.
	@property (assign) TerminalWindowRef
	terminalWindow;
	//! Loads the Find interface.
	@property (strong) FindDialog_VC*
	viewMgr;

@end //}


/*!
The private class interface.
*/
@interface FindDialog_Object (FindDialog_ObjectInternal) //{

// initializers
	- (instancetype)
	initForTerminalWindow:(TerminalWindowRef)_
	onCloseBlock:(FindDialog_OnCloseBlock)_
	previousSearches:(NSMutableArray*)_
	initialOptions:(FindDialog_Options)_;

// new methods
	- (void)
	clearSearchHighlightingInContext:(FindDialog_SearchContext)_;
	- (void)
	display;
	- (UInt32)
	initiateSearchFor:(NSString*)_
	regularExpression:(BOOL)_
	ignoringCase:(BOOL)_
	allTerminals:(BOOL)_
	notFinal:(BOOL)_
	didSearch:(BOOL*)_;
	- (void)
	removeWithAcceptance:(BOOL)_;
	- (void)
	zoomToSearchResults;

// PopoverManager_Delegate
	// (undeclared)

@end //}


/*!
Private properties.
*/
@interface FindDialog_VC () //{

// accessors
	//! Object that operates the search interface.
	@property (assign) id< FindDialog_VCDelegate >
	responder;
	//! Parent terminal window whose terminal screens are being searched.
	@property (assign) TerminalWindowRef
	terminalWindow;

@end //}


namespace {

typedef std::vector< Terminal_RangeDescription >	My_TerminalRangeList;

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a Find window attached to the specified terminal
window, which is also set to search that terminal window.

Display the window with FindDialog_Display().  The user
can close the window at any time, but the Find Dialog
reference remains valid until you release it (use ARC,
e.g. a strong reference).  Your close notification block
is passed arguments that indicate the state of the UI at
the time the dialog is hidden.

(3.0)
*/
FindDialog_Ref
FindDialog_New  (TerminalWindowRef			inTerminalWindow,
				 FindDialog_OnCloseBlock	inBlock,
				 CFMutableArrayRef			inoutQueryStringHistory,
				 FindDialog_Options			inFlags)
{
	FindDialog_Ref		result = nullptr;
	
	
	result = [[FindDialog_Object alloc]
				initForTerminalWindow:inTerminalWindow
										onCloseBlock:inBlock
										previousSearches:BRIDGE_CAST(inoutQueryStringHistory, NSMutableArray*)
										initialOptions:inFlags];
	return result;
}// New


/*!
This method displays and handles events in the
Find dialog box.  When the user clicks a button
in the dialog, its disposal callback is invoked.

(3.0)
*/
void
FindDialog_Display	(FindDialog_Ref		inDialog)
{
	if (nullptr == inDialog)
	{
		Sound_StandardAlert(); // TEMPORARY (display alert message?)
	}
	else
	{
		// load the view asynchronously and eventually display it in a window
		[inDialog display];
	}
}// Display


/*!
Performs a highlight-all-results search as if the user had
initiated a search with the Find interface, except the
search is done programmatically.

If the query string is nullptr, no search is performed and
highlighting is cleared in the given context (one window or
all windows).

Returns the number of matches.

(4.1)
*/
UInt32
FindDialog_SearchWithoutDialog		(CFStringRef			inQueryBaseOrNullToClear,
									 TerminalWindowRef		inStartTerminalWindow,
									 FindDialog_Options		inFlags,
									 Boolean*				outDidSearchOrNull)
{
	UInt32										result = 0;
	__block std::vector< TerminalWindowRef >	allWindowsList;
	
	
	// remove highlighting from any previous searches
	// and find all terminal window objects for later use
	if (inFlags & kFindDialog_OptionAllOpenTerminals)
	{
		SessionFactory_ForEachTerminalWindow
		(^(TerminalWindowRef	inTerminalWindow,
		   Boolean&				UNUSED_ARGUMENT(outStop))
		{
			if (TerminalWindow_IsValid(inTerminalWindow))
			{
				TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(inTerminalWindow);
				
				
				TerminalView_FindNothing(view);
				allWindowsList.push_back(inTerminalWindow);
			}
		});
	}
	else
	{
		if (TerminalWindow_IsValid(inStartTerminalWindow))
		{
			TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(inStartTerminalWindow);
			
			
			TerminalView_FindNothing(view);
		}
	}
	
	if ((nullptr == inQueryBaseOrNullToClear) ||
		(0 == CFStringGetLength(inQueryBaseOrNullToClear)))
	{
		if (nullptr != outDidSearchOrNull)
		{
			*outDidSearchOrNull = false;
		}
	}
	else
	{
		Boolean										queryOK = true;
		CFIndex										searchQueryLength = CFStringGetLength(inQueryBaseOrNullToClear);
		NSCharacterSet*								whitespaceSet = [NSCharacterSet whitespaceCharacterSet];
		NSString*									asNSString = BRIDGE_CAST(inQueryBaseOrNullToClear, NSString*);
		NSString*									trimmedString = [asNSString stringByTrimmingCharactersInSet:whitespaceSet];
		std::vector< TerminalWindowRef >			singleWindowList;
		std::vector< TerminalWindowRef > const*		searchedWindows = &singleWindowList;
		
		
		if (nullptr != outDidSearchOrNull)
		{
			*outDidSearchOrNull = true; // initially...
		}
		
		if (inFlags & kFindDialog_OptionNotFinal)
		{
			queryOK = false; // initially...
			if (STATIC_CAST(searchQueryLength, UInt32) >= 2/* arbitrary */)
			{
				//
				// if a search string is not final then it is being entered
				// interactively by the user; use this opportunity to avoid
				// expensive dynamic searches
				//
				
				// (no heuristics defined yet)
				queryOK = true;
			}
		}
		
		// NEVER search for strings that contain only whitespace, as this
		// is a sure way to bring the search engine to its knees and it
		// has no real value to the user
		if (0 == trimmedString.length)
		{
			queryOK = false;
		}
		
		if (false == queryOK)
		{
			// cannot find a query string; abort the search
			singleWindowList.clear();
			if (nullptr != outDidSearchOrNull)
			{
				*outDidSearchOrNull = false;
			}
		}
		else if (inFlags & kFindDialog_OptionAllOpenTerminals)
		{
			// search more than one terminal and highlight results in all!
			searchedWindows = &allWindowsList;
		}
		else
		{
			// search only the active session
			singleWindowList.push_back(inStartTerminalWindow);
		}
		
		for (auto terminalWindowRef : *searchedWindows)
		{
			TerminalViewRef			view = TerminalWindow_ReturnViewWithFocus(terminalWindowRef);
			TerminalScreenRef		screen = TerminalWindow_ReturnScreenWithFocus(terminalWindowRef);
			My_TerminalRangeList	searchResults;
			Terminal_SearchFlags	flags = 0;
			Terminal_Result			searchStatus = kTerminal_ResultOK;
			
			
			// initiate synchronous (should be asynchronous!) search
			if ((nullptr != inQueryBaseOrNullToClear) && (searchQueryLength > 0))
			{
				// configure search
				if (inFlags & kFindDialog_OptionRegularExpression)
				{
					flags |= kTerminal_SearchFlagsRegularExpression;
				}
				if (0 == (inFlags & kFindDialog_OptionCaseInsensitive))
				{
					flags |= kTerminal_SearchFlagsCaseSensitive;
				}
				
				// if the string ends with an apparent new-line character and this
				// can never match (due to the way lines are stored), magically
				// interpret it as a restriction that matches can only occur at
				// the end of lines
				{
					unichar		tailCharacter = [asNSString characterAtIndex:(asNSString.length - 1)];
					
					
					if (('\n' == tailCharacter) || ('\r' == tailCharacter))
					{
						// interpret as “matches only at end of line”; for
						// convenience, Terminal_Search() implicitly ignores
						// trailing new-line characters in this mode so that
						// the query string does not need to strip them
						flags |= kTerminal_SearchFlagsMatchOnlyAtLineEnd;
					}
				}
				
				// initiate synchronous (should it be asynchronous?) search
				searchStatus = Terminal_Search(screen, inQueryBaseOrNullToClear, flags, searchResults);
				if (kTerminal_ResultOK == searchStatus)
				{
					if (false == searchResults.empty())
					{
						// the count is global (if multi-terminal, reflects results from all terminals)
						result += STATIC_CAST(searchResults.size(), UInt32);
						
						// highlight search results
						for (auto rangeDesc : searchResults)
						{
							TerminalView_CellRange		highlightRange;
							TerminalView_Result			viewResult = kTerminalView_ResultOK;
							
							
							// translate this result range into cell anchors for highlighting
							viewResult = TerminalView_TranslateTerminalScreenRange(view, rangeDesc, highlightRange);
							if (kTerminalView_ResultOK == viewResult)
							{
								TerminalView_FindVirtualRange(view, highlightRange);
							}
						}
						
						// scroll to the first result
						if (0 == (inFlags & kFindDialog_OptionDoNotScrollToMatch))
						{
							// show the user where the text is; delay this slightly to avoid
							// animation interference caused by the closing of the popover
							CocoaExtensions_RunLater(0.1/* seconds */,
							^{
								TerminalView_ZoomToSearchResults(TerminalWindow_ReturnViewWithFocus(terminalWindowRef));
							});
						}
					}
				}
			}
		}
	}
	
	return result;
}// SearchWithoutDialog


#pragma mark Internal Methods


#pragma mark -
@implementation FindDialog_Object //{


@synthesize historyArray = _historyArray;


#pragma mark FindDialog_VCDelegate


/*!
Called when a FindDialog_VC has finished loading and
initializing its view; responds by displaying the view
in a window and giving it keyboard focus.

Since this may be invoked multiple times, the window is
only created during the first invocation.

(4.0)
*/
- (void)
findDialog:(FindDialog_VC*)		aViewMgr
didLoadManagedView:(NSView*)	aManagedView
{
	self.managedView = aManagedView;
	self.idealViewSize = aManagedView.frame.size;
	if (nil == self.containerWindow)
	{
		NSWindow*	parentWindow = TerminalWindow_ReturnNSWindow(self.terminalWindow);
		
		
		self.containerWindow = [[Popover_Window alloc] initWithView:aManagedView
																	windowStyle:kPopover_WindowStyleNormal
																	arrowStyle:kPopover_ArrowStyleNone
																	attachedToPoint:NSZeroPoint/* see delegate */
																	inWindow:parentWindow];
		self.containerWindow.releasedWhenClosed = NO;
		{
			NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(self.terminalWindow);
			NSView*		parentView = STATIC_CAST(cocoaWindow.contentView, NSView*);
			
			
			self.popoverMgr = PopoverManager_New(self.containerWindow, [aViewMgr logicalFirstResponder],
													self/* delegate */, kPopoverManager_AnimationTypeNone,
													kPopoverManager_BehaviorTypeStandard, // e.g. dismiss by clicking outside
													parentView);
		}
		PopoverManager_DisplayPopover(self.popoverMgr);
	}
}// findDialog:didLoadManagedView:


/*!
Called when highlighting should be removed.

(4.1)
*/
- (void)
findDialog:(FindDialog_VC*)										aViewMgr
clearSearchHighlightingInContext:(FindDialog_SearchContext)		aContext
{
#pragma unused(aViewMgr)
	[self clearSearchHighlightingInContext:aContext];
}// findDialog:clearSearchHighlightingInContext:


/*!
Called when the user has triggered a search, either by
starting to type text or hitting the default button.
The specified string is for convenience, it should be
equivalent to "[aViewMgr searchText]".

(4.0)
*/
- (void)
findDialog:(FindDialog_VC*)			aViewMgr
didSearchInManagedView:(NSView*)	aManagedView
withQuery:(NSString*)				searchText
{
#pragma unused(aManagedView)
	BOOL		didSearch = NO;
	UInt32		matchCount = [self initiateSearchFor:searchText
														regularExpression:aViewMgr.regularExpressionSearch
														ignoringCase:aViewMgr.caseInsensitiveSearch
														allTerminals:aViewMgr.multiTerminalSearch
														notFinal:YES didSearch:&didSearch];
	
	
	[aViewMgr updateUserInterfaceWithMatches:matchCount didSearch:didSearch];
}// findDialog:didSearchInManagedView:withQuery:


/*!
Called when the user has taken some action that would
complete his or her interaction with the view; a
sensible response is to close any containing window.
If the search is accepted, initiate one final search;
otherwise, undo any highlighting and restore the search
results that were in effect before the Find dialog was
opened.

(4.0)
*/
- (void)
findDialog:(FindDialog_VC*)				aViewMgr
didFinishUsingManagedView:(NSView*)		aManagedView
acceptingSearch:(BOOL)					acceptedSearch
finalOptions:(FindDialog_Options)		options
{
#pragma unused(aViewMgr, aManagedView)
	NSString*	searchText = [aViewMgr searchText];
	BOOL		isRegEx = aViewMgr.regularExpressionSearch;
	BOOL		caseInsensitive = aViewMgr.caseInsensitiveSearch;
	BOOL		multiTerminal = aViewMgr.multiTerminalSearch;
	
	
	self.cachedOptions = options;
	
	// make search history persistent for the window
	NSMutableArray*		recentSearchesArray = self.historyArray;
	if ((acceptedSearch) && (nil != searchText))
	{
		[recentSearchesArray removeObject:searchText]; // remove any older copy of this search phrase
		[recentSearchesArray insertObject:searchText atIndex:0];
		aViewMgr.searchField.recentSearches = [recentSearchesArray copy];
	}
	
	// hide the popover
	[self removeWithAcceptance:acceptedSearch];
	
	// highlight search results
	if (acceptedSearch)
	{
		BOOL	didSearch = NO;
		
		
		UNUSED_RETURN(UInt32)[self initiateSearchFor:searchText
														regularExpression:isRegEx
														ignoringCase:caseInsensitive
														allTerminals:multiTerminal
														notFinal:NO
														didSearch:&didSearch];
	}
	else
	{
		// user cancelled; try to return to the previous search
		if ((nil != recentSearchesArray) && ([recentSearchesArray count] > 0))
		{
			BOOL	didSearch = NO;
			
			
			searchText = STATIC_CAST([recentSearchesArray objectAtIndex:0], NSString*);
			UNUSED_RETURN(UInt32)[self initiateSearchFor:searchText
															regularExpression:isRegEx
															ignoringCase:caseInsensitive
															allTerminals:NO
															notFinal:NO
															didSearch:&didSearch];
		}
		else
		{
			// no previous search available; remove all highlighting
			[self clearSearchHighlightingInContext:aViewMgr.searchContext];
		}
	}
	
	// notify of close
	if (nullptr != self.onCloseBlock)
	{
		self.onCloseBlock(self, self.cachedOptions);
	}
}// findDialog:didFinishUsingManagedView:acceptingSearch:finalOptions:


#pragma mark PopoverManager_Delegate


/*!
Assists the dynamic resize of a popover window by indicating
whether or not there are per-axis constraints on resizing.

(2017.06)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getHorizontalResizeAllowed:(BOOL*)		outHorizontalFlagPtr
getVerticalResizeAllowed:(BOOL*)		outVerticalFlagPtr
{
#pragma unused(aPopoverManager)
	*outHorizontalFlagPtr = YES;
	*outVerticalFlagPtr = NO;
}// popoverManager:getHorizontalResizeAllowed:getVerticalResizeAllowed:


/*!
Returns the initial view size for the popover.

(2017.06)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getIdealSize:(NSSize*)					outSizePtr
{
#pragma unused(aPopoverManager)
	*outSizePtr = self.idealViewSize;
}// popoverManager:getIdealSize:


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForFrame:parentWindow:".

(4.0)
*/
- (NSPoint)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealAnchorPointForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentWindow)
	NSPoint		result = NSZeroPoint;
	
	
	if (nil != self.managedView)
	{
		NSRect		managedViewFrame = self.managedView.frame;
		
		
		result = NSMakePoint(parentFrame.size.width - managedViewFrame.size.width - 16.0f/* arbitrary */, 0.0f);
	}
	return result;
}// popoverManager:idealAnchorPointForFrame:parentWindow:


/*!
Returns arrow placement information for the popover.

(4.0)
*/
- (Popover_Properties)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentWindow, parentFrame)
	Popover_Properties	result = kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameAboveArrow;
	
	
	return result;
}// idealArrowPositionForFrame:parentWindow:


@end //}


#pragma mark -
@implementation FindDialog_Object (FindDialog_ObjectInternal) //{


#pragma mark Initializers


/*!
Designated initializer.

The specified array of previous search strings is held only
by weak reference and should not be deallocated until after
this class instance is destroyed.

(4.0)
*/
- (instancetype)
initForTerminalWindow:(TerminalWindowRef)	aTerminalWindow
onCloseBlock:(FindDialog_OnCloseBlock)		aBlock
previousSearches:(NSMutableArray*)			aStringArray
initialOptions:(FindDialog_Options)			options
{
	self = [super init];
	if (nil != self)
	{
		_cachedOptions = options;
		_containerWindow = nil;
		_historyArray = aStringArray;
		_idealViewSize = NSZeroSize;
		_managedView = nil;
		_onCloseBlock = aBlock;
		_popoverMgr = nullptr;
		assert(nil != aStringArray);
		_terminalWindow = aTerminalWindow;
		_viewMgr = nil;
	}
	return self;
}// initForTerminalWindow:onCloseBlock:previousSearches:initialOptions:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	Memory_EraseWeakReferences(BRIDGE_CAST(self, void*));
}// dealloc


/*!
Updates a terminal view so that previously-found words are no
longer highlighted.

See the view manager property "searchContext" for a convenient
way to find the current search context (local or global).

(4.1)
*/
- (void)
clearSearchHighlightingInContext:(FindDialog_SearchContext)		aContext
{
	FindDialog_Options		flags = kFindDialog_OptionsAllOff;
	
	
	if (kFindDialog_SearchContextGlobal == aContext)
	{
		flags |= kFindDialog_OptionAllOpenTerminals;
	}
	
	UNUSED_RETURN(UInt32)FindDialog_SearchWithoutDialog(nullptr/* query */, self.terminalWindow, flags);
}// clearSearchHighlightingInContext:


/*!
Creates the Find view asynchronously; when the view is ready,
it calls "findDialog:didLoadManagedView:".

(4.0)
*/
- (void)
display
{
	if (nil == self.viewMgr)
	{
		// no focus is done the first time because this is
		// eventually done in "findDialog:didLoadManagedView:"
		self.viewMgr = [[FindDialog_VC alloc]
						initForTerminalWindow:self.terminalWindow
												responder:self
												initialOptions:self.cachedOptions];
	}
	else
	{
		// window is already loaded, just activate it
		PopoverManager_DisplayPopover(self.popoverMgr);
	}
}// display


/*!
Starts a (synchronous) search of the focused terminal screen.
Returns the number of matches.

If "regularExpression" is YES, the string is considered to be
a regular expression instead of a literal string.  And if
"ignoreCase" is set, case-insensitive matching is done (for
both literal strings and regular expressions).

If "isNotFinal" is YES, certain optimizations are made (mostly
heuristics based on the nature of the query).  If the query
looks like it might be expensive, this function automatically
avoids initiating the search, to keep the user interface very
responsive.  Always set "isNotFinal" to NO for final queries,
i.e. those that cause the dialog to close and results to be
permanently highlighted.

If "isAnimated" is YES, the terminal is asked to scroll to the
position where the first match is found, and a small animation
may run to highlight the text.

The flag "outDidSearch" is set to YES if the search actually
occurred.

(4.0)
*/
- (UInt32)
initiateSearchFor:(NSString*)	queryString
regularExpression:(BOOL)		regularExpression
ignoringCase:(BOOL)				ignoreCase
allTerminals:(BOOL)				allTerminals
notFinal:(BOOL)					isNotFinal
didSearch:(BOOL*)				outDidSearch
{
	CFStringRef					searchQueryCFString = BRIDGE_CAST(queryString, CFStringRef);
	CFIndex						searchQueryLength = 0;
	FindDialog_SearchContext	searchContext = (YES == allTerminals)
												? kFindDialog_SearchContextGlobal
												: kFindDialog_SearchContextLocal;
	UInt32						result = 0;
	
	
	*outDidSearch = YES; // initially...
	
	searchQueryLength = (nullptr == searchQueryCFString) ? 0 : CFStringGetLength(searchQueryCFString);
	if (0 == searchQueryLength)
	{
		// special case; empty queries always “succeed” but deselect everything
		[self clearSearchHighlightingInContext:searchContext];
		*outDidSearch = NO;
	}
	else
	{
		FindDialog_Options		searchFlags = 0;
		Boolean					didSearch = false;
		
		
		if (regularExpression)
		{
			searchFlags |= kFindDialog_OptionRegularExpression;
		}
		
		if (ignoreCase)
		{
			searchFlags |= kFindDialog_OptionCaseInsensitive;
		}
		
		if (isNotFinal)
		{
			searchFlags |= kFindDialog_OptionNotFinal;
			searchFlags |= kFindDialog_OptionDoNotScrollToMatch;
		}
		
		if (allTerminals)
		{
			searchFlags |= kFindDialog_OptionAllOpenTerminals;
		}
		
		result = FindDialog_SearchWithoutDialog(BRIDGE_CAST(queryString, CFStringRef),
												self.terminalWindow, searchFlags, &didSearch);
		*outDidSearch = (true == didSearch);
	}
	
	return result;
}// initiateSearchFor:regularExpression:ignoringCase:allTerminals:notFinal:didSearch:


/*!
Hides the popover.  It can be shown again at any time
using the "display" method.

(4.0)
*/
- (void)
removeWithAcceptance:(BOOL)		isAccepted
{
	if (nil != self.popoverMgr)
	{
		PopoverManager_RemovePopover(self.popoverMgr, isAccepted);
	}
}// removeWithAcceptance:


/*!
Highlights the location of the first search result.

This is in an Objective-C method so that it can be
conveniently invoked after a short delay.

(4.0)
*/
- (void)
zoomToSearchResults
{
	// show the user where the text is
	TerminalView_ZoomToSearchResults(TerminalWindow_ReturnViewWithFocus(self.terminalWindow));
}// zoomToSearchResults


@end //} FindDialog_Object (FindDialog_ObjectInternal)


#pragma mark -
@implementation FindDialog_SearchField //{


#pragma mark NSNibAwaking


/*!
Checks IBOutlet bindings.

(4.1)
*/
- (void)
awakeFromNib
{
	assert(nil != self.delegate); // in this case the delegate is pretty important
	assert(nil != self.viewManager);
}// awakeFromNib


@end //} FindDialog_SearchField


#pragma mark -
@implementation FindDialog_VC //{


@synthesize caseInsensitiveSearch = _caseInsensitiveSearch;
@synthesize regularExpressionSearch = _regularExpressionSearch;
@synthesize multiTerminalSearch = _multiTerminalSearch;


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initForTerminalWindow:(TerminalWindowRef)	aTerminalWindow
responder:(id< FindDialog_VCDelegate >)		aResponder
initialOptions:(FindDialog_Options)			options
{
	self = [super initWithNibName:@"FindDialogCocoa" bundle:nil];
	if (nil != self)
	{
		_responder = aResponder;
		_terminalWindow = aTerminalWindow;
		
		_caseInsensitiveSearch = (0 != (options & kFindDialog_OptionCaseInsensitive));
		_multiTerminalSearch = (0 != (options & kFindDialog_OptionAllOpenTerminals));
		_regularExpressionSearch = (0 != (options & kFindDialog_OptionRegularExpression));
		_searchProgressHidden = YES;
		_successfulSearch = YES;
		_searchText = @"";
		_statusText = @"";
		
		// NSViewController implicitly loads the NIB when the "view"
		// property is accessed; force that here
		[self view];
	}
	return self;
}// initForTerminalWindow:responder:initialOptions:


#pragma mark Initializers Disabled From Superclass


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithCoder:(NSCoder*)	aCoder
{
#pragma unused(aCoder)
	assert(false && "invalid way to initialize derived class");
	return [self initForTerminalWindow:nullptr responder:nil initialOptions:0];
}// initWithCoder:


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithNibName:(NSString*)		aNibName
bundle:(NSBundle*)				aBundle
{
#pragma unused(aNibName, aBundle)
	assert(false && "invalid way to initialize derived class");
	return [self initForTerminalWindow:nullptr responder:nil initialOptions:0];
}// initWithNibName:bundle:


#pragma mark New Methods


/*!
Returns the view that a window ought to focus first
using NSWindow’s "makeFirstResponder:".

(4.0)
*/
- (NSView*)
logicalFirstResponder
{
	return self.searchField;
}// logicalFirstResponder


/*!
Responds to a click in the help button.

(4.0)
*/
- (IBAction)
orderFrontContextualHelp:(id)	sender
{
#pragma unused(sender)
	CFRetainRelease		keyPhrase(UIStrings_ReturnCopy(kUIStrings_HelpSystemTopicHelpSearchingForText),
									CFRetainRelease::kAlreadyRetained);
	
	
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpWithSearch(keyPhrase.returnCFStringRef());
}// orderFrontContextualHelp:


/*!
Cancels the dialog and restores any previous search
results in the target window.

IMPORTANT:	It is appropriate at this time for the
			responder object to release itself (and
			this object).

(4.0)
*/
- (IBAction)
performCloseAndRevert:(id)	sender
{
#pragma unused(sender)
	FindDialog_Options		options = kFindDialog_OptionsAllOff;
	
	
	if (self.regularExpressionSearch)
	{
		options |= kFindDialog_OptionRegularExpression;
	}
	if (self.caseInsensitiveSearch)
	{
		options |= kFindDialog_OptionCaseInsensitive;
	}
	[self.responder findDialog:self
								didFinishUsingManagedView:self.view
								acceptingSearch:NO
								finalOptions:options];
}// performCloseAndRevert:


/*!
Unlike "performSearch:", which can begin a search while
leaving the window open, this method will close the
Find dialog.

IMPORTANT:	It is appropriate at this time for the
			responder object to release itself (and
			this object).

(4.0)
*/
- (IBAction)
performCloseAndSearch:(id)	sender
{
#pragma unused(sender)
	NSString*				queryText = (nil == _searchText)
										? @""
										: [NSString stringWithString:_searchText];
	FindDialog_Options		options = kFindDialog_OptionsAllOff;
	
	
	if (self.regularExpressionSearch)
	{
		options |= kFindDialog_OptionRegularExpression;
	}
	if (self.caseInsensitiveSearch)
	{
		options |= kFindDialog_OptionCaseInsensitive;
	}
	[self.responder findDialog:self
								didSearchInManagedView:self.view
								withQuery:queryText];
	[self.responder findDialog:self
								didFinishUsingManagedView:self.view
								acceptingSearch:YES
								finalOptions:options];
}// performCloseAndSearch:


/*!
Initiates a search using the keywords currently entered
in the field.  See also "performCloseAndSearch:".

(4.0)
*/
- (IBAction)
performSearch:(id)	sender
{
#pragma unused(sender)
	NSString*	queryText = (nil == self.searchText)
							? @""
							: [NSString stringWithString:self.searchText];
	
	
	self.statusText = @"";
	self.searchProgressHidden = NO;
	[self.responder findDialog:self
								didSearchInManagedView:self.view
								withQuery:queryText];
}// performSearch:


/*!
This should be called by a responder after it attempts to
continue searching.

If the search did not occur yet ("didSearch:), the UI status
does not show any matches or any errors.  Otherwise, include
the number of times the query matched: if the count is zero,
the interface shows an error; otherwise, it displays the
number of matches to the user.

There is no need to call "setSearchProgressHidden:" or any
similar methods; this single call will clean up the UI.

(4.0)
*/
- (void)
updateUserInterfaceWithMatches:(UInt32)		matchCount
didSearch:(BOOL)							didSearch
{
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	CFStringRef			statusCFString = nullptr;
	BOOL				releaseStatusString = NO;
	
	
	if (NO == didSearch)
	{
		statusCFString = CFSTR("");
	}
	else if (0 == matchCount)
	{
		stringResult = UIStrings_Copy(kUIStrings_TerminalSearchNothingFound, statusCFString);
		if (false == stringResult.ok())
		{
			statusCFString = nullptr;
		}
		else
		{
			releaseStatusString = YES;
		}
	}
	else
	{
		CFStringRef		templateCFString = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_TerminalSearchNumberOfMatches, templateCFString);
		if (stringResult.ok())
		{
			statusCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */, templateCFString,
														STATIC_CAST(matchCount, unsigned long));
			releaseStatusString = YES;
			CFRelease(templateCFString), templateCFString = nullptr;
		}
	}
	
	// update settings; due to bindings, the user interface will automatically be updated
	self.statusText = BRIDGE_CAST(statusCFString, NSString*);
	self.searchProgressHidden = YES;
	self.successfulSearch = ((matchCount > 0) || (self.searchText.length <= 1));
	
	if ((nullptr != statusCFString) && (releaseStatusString))
	{
		CFRelease(statusCFString), statusCFString = nullptr;
	}
}// updateUserInterfaceWithMatches:didSearch:


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (BOOL)
caseInsensitiveSearch
{
	return _caseInsensitiveSearch;
}
- (void)
setCaseInsensitiveSearch:(BOOL)		isCaseInsensitive
{
	if (_caseInsensitiveSearch != isCaseInsensitive)
	{
		_caseInsensitiveSearch = isCaseInsensitive;
		[self performSearch:NSApp];
	}
}// setCaseInsensitiveSearch:


/*!
Accessor.

(2019.03)
*/
- (BOOL)
regularExpressionSearch
{
	return _regularExpressionSearch;
}
- (void)
setRegularExpressionSearch:(BOOL)	isRegularExpression
{
	if (_regularExpressionSearch != isRegularExpression)
	{
		_regularExpressionSearch = isRegularExpression;
		[self performSearch:NSApp];
	}
}// setRegularExpressionSearch:


/*!
Accessor.

(2016.10)
*/
- (FindDialog_SearchContext)
searchContext
{
	FindDialog_SearchContext		result = ((self.multiTerminalSearch)
											? kFindDialog_SearchContextGlobal
											: kFindDialog_SearchContextLocal);
	
	
	return result;
}// searchContext


/*!
Accessor.

(4.0)
*/
- (BOOL)
multiTerminalSearch
{
	return _multiTerminalSearch;
}
- (void)
setMultiTerminalSearch:(BOOL)	isMultiTerminal
{
	if (_multiTerminalSearch != isMultiTerminal)
	{
		_multiTerminalSearch = isMultiTerminal;
		
		// if the setting is being turned off, erase the
		// search highlighting from other windows (this will
		// not happen automatically once the search flag is
		// restricted to a single window)
		if (NO == _multiTerminalSearch)
		{
			[self.responder findDialog:self
										clearSearchHighlightingInContext:kFindDialog_SearchContextGlobal];
		}
		
		// update search results of current terminal window and
		// (if multi-terminal mode) all other terminal windows
		[self performSearch:NSApp];
	}
}// setMultiTerminalSearch:


#pragma mark NSTextFieldDelegate


/*!
Responds to cancellation by closing the panel and responds to
(say) a Return by closing the panel but preserving the current
search term and search options.  Ignores other key events.

(4.1)
*/
- (BOOL)
control:(NSControl*)		aControl
textView:(NSTextView*)		aTextView
doCommandBySelector:(SEL)	aSelector
{
	BOOL	result = NO;
	
	
	// this delegate is not designed to manage other fields...
	assert(self.searchField == aControl);
	
	if (@selector(cancelOperation:) == aSelector)
	{
		// force the field to be empty and remove highlighting
		[self.responder findDialog:self
									clearSearchHighlightingInContext:self.searchContext];
		aTextView.string = @"";
		[self performCloseAndRevert:self];
		result = YES;
	}
	else if (@selector(insertNewline:) == aSelector)
	{
		[self performCloseAndSearch:self];
		result = YES;
	}
	
	return result;
}// control:textView:doCommandBySelector:


#pragma mark NSViewController


/*!
Invoked by NSViewController once the "self.view" property is set,
after the NIB file is loaded.  This essentially guarantees that
all file-defined user interface elements are now instantiated and
other settings that depend on valid UI objects can now be made.

NOTE:	As future SDKs are adopted, it makes more sense to only
		implement "viewDidLoad" (which was only recently added
		to NSViewController and is not otherwise available).
		This implementation can essentially move to "viewDidLoad".

(4.1)
*/
- (void)
loadView
{
	[super loadView];
	assert(nil != self.searchField);
	
	[self.responder findDialog:self
								didLoadManagedView:self.view];
}// loadView


@end //} FindDialog_VC

// BELOW IS REQUIRED NEWLINE TO END FILE
