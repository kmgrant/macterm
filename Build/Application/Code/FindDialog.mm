/*!	\file FindDialog.mm
	\brief Used to perform searches in the scrollback
	buffers of terminal windows.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <CFRetainRelease.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <Popover.objc++.h>
#import <PopoverManager.objc++.h>

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
Manages the Find user interface.
*/
@interface FindDialog_Handler : NSObject< FindDialog_VCDelegate, PopoverManager_Delegate > //{
{
	FindDialog_Ref					selfRef;			// identical to address of structure, but typed as ref
	FindDialog_VC*					viewMgr;			// loads the Find interface
	Popover_Window*					containerWindow;	// holds the Find dialog view
	NSView*							managedView;		// the view that implements the majority of the interface
	TerminalWindowRef				terminalWindow;		// the terminal window for which this dialog applies
	PopoverManager_Ref				popoverMgr;			// manages common aspects of popover window behavior
	FindDialog_CloseNotifyProcPtr	closeNotifyProc;	// routine to call when the dialog is dismissed
	FindDialog_Options				cachedOptions;		// options set when the user interface is closed
@private
	NSMutableArray*					_historyArray;
}

// class methods
	+ (FindDialog_Handler*)
	viewHandlerFromRef:(FindDialog_Ref)_;

// initializers
	- (instancetype)
	initForTerminalWindow:(TerminalWindowRef)_
	notificationProc:(FindDialog_CloseNotifyProcPtr)_
	previousSearches:(NSMutableArray*)_
	initialOptions:(FindDialog_Options)_;

// new methods
	- (void)
	clearSearchHighlightingInContext:(FindDialog_SearchContext)_;
	- (void)
	display;
	- (UInt32)
	initiateSearchFor:(NSString*)_
	ignoringCase:(BOOL)_
	allTerminals:(BOOL)_
	notFinal:(BOOL)_
	didSearch:(BOOL*)_;
	- (void)
	removeWithAcceptance:(BOOL)_;
	- (void)
	zoomToSearchResults;

// accessors
	- (FindDialog_Options)
	cachedOptions;
	@property (strong) NSMutableArray*
	historyArray;
	- (TerminalWindowRef)
	terminalWindow;

// FindDialog_VCDelegate
	- (void)
	findDialog:(FindDialog_VC*)_
	didLoadManagedView:(NSView*)_;
	- (void)
	findDialog:(FindDialog_VC*)_
	clearSearchHighlightingInContext:(FindDialog_SearchContext)_;
	- (void)
	findDialog:(FindDialog_VC*)_
	didSearchInManagedView:(NSView*)_
	withQuery:(NSString*)_;
	- (void)
	findDialog:(FindDialog_VC*)_
	didFinishUsingManagedView:(NSView*)_
	acceptingSearch:(BOOL)_
	finalOptions:(FindDialog_Options)_;

// PopoverManager_Delegate
	- (NSPoint)
	idealAnchorPointForFrame:(NSRect)_
	parentWindow:(NSWindow*)_;
	- (Popover_Properties)
	idealArrowPositionForFrame:(NSRect)_
	parentWindow:(NSWindow*)_;
	- (NSSize)
	idealSize;

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
reference remains valid until you release it with a call
to FindDialog_Dispose().  Your close notification routine
may invoke APIs like FindDialog_ReturnOptions() to query
the post-closing state of the window.

(3.0)
*/
FindDialog_Ref
FindDialog_New  (TerminalWindowRef				inTerminalWindow,
				 FindDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr,
				 CFMutableArrayRef				inoutQueryStringHistory,
				 FindDialog_Options				inFlags)
{
	FindDialog_Ref	result = nullptr;
	
	
	result = (FindDialog_Ref)[[FindDialog_Handler alloc]
								initForTerminalWindow:inTerminalWindow notificationProc:inCloseNotifyProcPtr
														previousSearches:(NSMutableArray*)inoutQueryStringHistory
														initialOptions:inFlags];
	return result;
}// New


/*!
Releases the underlying data structure for a Find dialog.

(3.0)
*/
void
FindDialog_Dispose	(FindDialog_Ref*	inoutDialogPtr)
{
	FindDialog_Handler*		ptr = [FindDialog_Handler viewHandlerFromRef:*inoutDialogPtr];
	
	
	[ptr release];
}// Dispose


/*!
This method displays and handles events in the
Find dialog box.  When the user clicks a button
in the dialog, its disposal callback is invoked.

(3.0)
*/
void
FindDialog_Display	(FindDialog_Ref		inDialog)
{
	FindDialog_Handler*		ptr = [FindDialog_Handler viewHandlerFromRef:inDialog];
	
	
	if (nullptr == ptr)
	{
		Alert_ReportOSStatus(paramErr);
	}
	else
	{
		// load the view asynchronously and eventually display it in a window
		[ptr display];
	}
}// Display


/*!
Returns a set of flags indicating whether or not certain
options are enabled for the specified dialog.  This is
only guaranteed to be accurate from within a close
notification routine (after the user interface is hidden);
if the dialog is open this will not necessarily reflect
the current state that the user has set up.

If there are no options enabled, the result will be
"kFindDialog_OptionsAllOff".

(3.0)
*/
FindDialog_Options
FindDialog_ReturnOptions	(FindDialog_Ref		inDialog)
{
	FindDialog_Handler*		ptr = [FindDialog_Handler viewHandlerFromRef:inDialog];
	FindDialog_Options		result = kFindDialog_OptionsAllOff;
	
	
	if (nullptr != ptr)
	{
		result = [ptr cachedOptions];
	}
	return result;
}// ReturnOptions


/*!
Returns a reference to the terminal window that this
dialog is attached to.

(3.0)
*/
TerminalWindowRef
FindDialog_ReturnTerminalWindow		(FindDialog_Ref		inDialog)
{
	FindDialog_Handler*		ptr = [FindDialog_Handler viewHandlerFromRef:inDialog];
	TerminalWindowRef		result = nullptr;
	
	
	if (nullptr != ptr)
	{
		result = [ptr terminalWindow];
	}
	return result;
}// ReturnTerminalWindow


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
FindDialog_SearchWithoutDialog		(CFStringRef					inQueryBaseOrNullToClear,
									 TerminalWindowRef				inStartTerminalWindow,
									 FindDialog_Options				inFlags,
									 Boolean*						outDidSearchOrNull)
{
	UInt32		result = 0;
	
	
	// remove highlighting from any previous searches
	if (inFlags & kFindDialog_OptionAllOpenTerminals)
	{
		SessionFactory_TerminalWindowList const&	searchedWindows = SessionFactory_ReturnTerminalWindowList();
		
		
		for (auto terminalWindowRef : searchedWindows)
		{
			if (TerminalWindow_IsValid(terminalWindowRef))
			{
				TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindowRef);
				
				
				TerminalView_FindNothing(view);
			}
		}
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
		SessionFactory_TerminalWindowList			singleWindowList;
		SessionFactory_TerminalWindowList const*	searchedWindows = &singleWindowList;
		
		
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
		if (0 == [trimmedString length])
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
			searchedWindows = &(SessionFactory_ReturnTerminalWindowList());
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
				if (0 == (inFlags & kFindDialog_OptionCaseInsensitive))
				{
					flags |= kTerminal_SearchFlagsCaseSensitive;
				}
				
				// if the string ends with an apparent new-line character and this
				// can never match (due to the way lines are stored), magically
				// interpret it as a restriction that matches can only occur at
				// the end of lines
				{
					unichar		tailCharacter = [asNSString characterAtIndex:([asNSString length] - 1)];
					
					
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


/*!
The default handler for closing a search dialog.

(3.0)
*/
void
FindDialog_StandardCloseNotifyProc		(FindDialog_Ref		UNUSED_ARGUMENT(inDialogThatClosed))
{
	// do nothing
}// StandardCloseNotifyProc


#pragma mark Internal Methods


#pragma mark -
@implementation FindDialog_Handler


@synthesize historyArray = _historyArray;


/*!
Converts from the opaque reference type to the internal type.

(4.0)
*/
+ (FindDialog_Handler*)
viewHandlerFromRef:(FindDialog_Ref)		aRef
{
	return (FindDialog_Handler*)aRef;
}// viewHandlerFromRef


/*!
Designated initializer.

The specified array of previous search strings is held only
by weak reference and should not be deallocated until after
this class instance is destroyed.

(4.0)
*/
- (instancetype)
initForTerminalWindow:(TerminalWindowRef)			aTerminalWindow
notificationProc:(FindDialog_CloseNotifyProcPtr)	aProc
previousSearches:(NSMutableArray*)					aStringArray
initialOptions:(FindDialog_Options)					options
{
	self = [super init];
	if (nil != self)
	{
		self->selfRef = (FindDialog_Ref)self;
		self->viewMgr = nil;
		self->containerWindow = nil;
		self->managedView = nil;
		self->terminalWindow = aTerminalWindow;
		self->popoverMgr = nullptr;
		self->closeNotifyProc = aProc;
		assert(nil != aStringArray);
		self->_historyArray = [aStringArray retain];
		self->cachedOptions = options;
	}
	return self;
}// initForTerminalWindow:notificationProc:previousSearches:initialOptions:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[_historyArray release];
	[containerWindow release];
	[viewMgr release];
	if (nullptr != popoverMgr)
	{
		PopoverManager_Dispose(&popoverMgr);
	}
	[super dealloc];
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
	if (nil == self->viewMgr)
	{
		// no focus is done the first time because this is
		// eventually done in "findDialog:didLoadManagedView:"
		self->viewMgr = [[FindDialog_VC alloc]
							initForTerminalWindow:[self terminalWindow] responder:self
													initialOptions:self->cachedOptions];
	}
	else
	{
		// window is already loaded, just activate it
		PopoverManager_DisplayPopover(self->popoverMgr);
	}
}// display


/*!
Starts a (synchronous) search of the focused terminal screen.
Returns the number of matches.

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
												[self terminalWindow], searchFlags, &didSearch);
		*outDidSearch = (true == didSearch);
	}
	
	return result;
}// initiateSearchFor:ignoringCase:allTerminals:notFinal:didSearch:


/*!
Hides the popover.  It can be shown again at any time
using the "display" method.

(4.0)
*/
- (void)
removeWithAcceptance:(BOOL)		isAccepted
{
	if (nil != self->popoverMgr)
	{
		PopoverManager_RemovePopover(self->popoverMgr, isAccepted);
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
	TerminalView_ZoomToSearchResults(TerminalWindow_ReturnViewWithFocus([self terminalWindow]));
}// zoomToSearchResults


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (FindDialog_Options)
cachedOptions
{
	return cachedOptions;
}// cachedOptions


/*!
Accessor.

(4.0)
*/
- (TerminalWindowRef)
terminalWindow
{
	return terminalWindow;
}// terminalWindow


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
	self->managedView = aManagedView;
	if (nil == self->containerWindow)
	{
		NSWindow*	parentWindow = TerminalWindow_ReturnNSWindow([self terminalWindow]);
		
		
		self->containerWindow = [[Popover_Window alloc] initWithView:aManagedView
								 										style:kPopover_WindowStyleNormal
																		attachedToPoint:NSZeroPoint/* see delegate */
																		inWindow:parentWindow];
		[self->containerWindow setReleasedWhenClosed:NO];
		[self->containerWindow setStandardArrowProperties:NO];
		self->popoverMgr = PopoverManager_New(self->containerWindow, [aViewMgr logicalFirstResponder],
												self/* delegate */, kPopoverManager_AnimationTypeNone,
												kPopoverManager_BehaviorTypeStandard, // e.g. dismiss by clicking outside
												TerminalWindow_ReturnWindow([self terminalWindow]));
		PopoverManager_DisplayPopover(self->popoverMgr);
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
	BOOL		caseInsensitive = aViewMgr.caseInsensitiveSearch;
	BOOL		multiTerminal = aViewMgr.multiTerminalSearch;
	
	
	self->cachedOptions = options;
	
	// make search history persistent for the window
	NSMutableArray*		recentSearchesArray = self.historyArray;
	if ((acceptedSearch) && (nil != searchText))
	{
		[recentSearchesArray removeObject:searchText]; // remove any older copy of this search phrase
		[recentSearchesArray insertObject:searchText atIndex:0];
		[[aViewMgr searchField] setRecentSearches:[recentSearchesArray copy]];
	}
	
	// hide the popover
	[self removeWithAcceptance:acceptedSearch];
	
	// highlight search results
	if (acceptedSearch)
	{
		Boolean		noAnimations = false;
		BOOL		didSearch = NO;
		
		
		UNUSED_RETURN(UInt32)[self initiateSearchFor:searchText ignoringCase:caseInsensitive allTerminals:multiTerminal
														notFinal:NO didSearch:&didSearch];
	}
	else
	{
		// user cancelled; try to return to the previous search
		if ((nil != recentSearchesArray) && ([recentSearchesArray count] > 0))
		{
			BOOL	didSearch = NO;
			
			
			searchText = STATIC_CAST([recentSearchesArray objectAtIndex:0], NSString*);
			UNUSED_RETURN(UInt32)[self initiateSearchFor:searchText ignoringCase:caseInsensitive allTerminals:NO
															notFinal:NO didSearch:&didSearch];
		}
		else
		{
			// no previous search available; remove all highlighting
			[self clearSearchHighlightingInContext:aViewMgr.searchContext];
		}
	}
	
	// notify of close
	if (nullptr != self->closeNotifyProc)
	{
		FindDialog_InvokeCloseNotifyProc(self->closeNotifyProc, self->selfRef);
	}
}// findDialog:didFinishUsingManagedView:acceptingSearch:finalOptions:


#pragma mark PopoverManager_Delegate


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForFrame:parentWindow:".

(4.0)
*/
- (NSPoint)
idealAnchorPointForFrame:(NSRect)	parentFrame
parentWindow:(NSWindow*)			parentWindow
{
#pragma unused(parentWindow)
	NSPoint		result = NSZeroPoint;
	
	
	if (nil != self->managedView)
	{
		NSRect		managedViewFrame = [self->managedView frame];
		
		
		result = NSMakePoint(parentFrame.size.width - managedViewFrame.size.width - 16.0f/* arbitrary */, 0.0f);
	}
	return result;
}// idealAnchorPointForFrame:parentWindow:


/*!
Returns arrow placement information for the popover.

(4.0)
*/
- (Popover_Properties)
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(parentWindow, parentFrame)
	Popover_Properties	result = kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameAboveArrow;
	
	
	return result;
}// idealArrowPositionForFrame:parentWindow:


/*!
Returns the initial size for the popover.

(4.0)
*/
- (NSSize)
idealSize
{
	NSSize		result = NSZeroSize;
	
	
	if (nil != self->containerWindow)
	{
		NSRect		frameRect = [self->containerWindow frameRectForViewRect:[self->managedView frame]];
		
		
		result = frameRect.size;
	}
	return result;
}// idealSize


@end // FindDialog_Handler


#pragma mark -
@implementation FindDialog_SearchField


#pragma mark NSNibAwaking


/*!
Checks IBOutlet bindings.

(4.1)
*/
- (void)
awakeFromNib
{
	assert(nil != self.delegate); // in this case the delegate is pretty important
	assert(nil != viewManager);
}// awakeFromNib


@end // FindDialog_SearchField


#pragma mark -
@implementation FindDialog_VC


@synthesize searchProgressHidden = _searchProgressHidden;
@synthesize searchText = _searchText;
@synthesize statusText = _statusText;
@synthesize successfulSearch = _successfulSearch;


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
		responder = aResponder;
		terminalWindow = aTerminalWindow;
		_caseInsensitiveSearch = (0 != (options & kFindDialog_OptionCaseInsensitive));
		_multiTerminalSearch = (0 != (options & kFindDialog_OptionAllOpenTerminals));
		_searchProgressHidden = YES;
		_successfulSearch = YES;
		_searchText = [@"" retain];
		_statusText = [@"" retain];
		
		// NSViewController implicitly loads the NIB when the "view"
		// property is accessed; force that here
		[self view];
	}
	return self;
}// initForTerminalWindow:responder:initialOptions:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[_searchText release];
	[_statusText release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Returns the view that a window ought to focus first
using NSWindow’s "makeFirstResponder:".

(4.0)
*/
- (NSView*)
logicalFirstResponder
{
	return [self searchField];
}// logicalFirstResponder


/*!
Responds to a click in the help button.

(4.0)
*/
- (IBAction)
performContextSensitiveHelp:(id)	sender
{
#pragma unused(sender)
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseFind);
}// performContextSensitiveHelp:


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
	
	
	if (self.caseInsensitiveSearch)
	{
		options |= kFindDialog_OptionCaseInsensitive;
	}
	[self->responder findDialog:self didFinishUsingManagedView:self.view
										acceptingSearch:NO finalOptions:options];
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
	
	
	if (self.caseInsensitiveSearch)
	{
		options |= kFindDialog_OptionCaseInsensitive;
	}
	[self->responder findDialog:self didSearchInManagedView:self.view withQuery:queryText];
	[self->responder findDialog:self didFinishUsingManagedView:self.view
										acceptingSearch:YES finalOptions:options];
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
	NSString*	queryText = (nil == _searchText)
							? @""
							: [NSString stringWithString:_searchText];
	
	
	self.statusText = @"";
	self.searchProgressHidden = NO;
	[self->responder findDialog:self didSearchInManagedView:self.view withQuery:queryText];
}// performSearch:


/*!
Returns the view that contains the search query text and
a menu of recent searches.

(4.0)
*/
- (NSSearchField*)
searchField
{
	return searchField;
}// searchField


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
	self.successfulSearch = ((matchCount > 0) || ([_searchText length] <= 1));
	
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
			[self->responder findDialog:self clearSearchHighlightingInContext:kFindDialog_SearchContextGlobal];
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
	assert(self->searchField == aControl);
	
	if (@selector(cancelOperation:) == aSelector)
	{
		// force the field to be empty and remove highlighting
		[self->responder findDialog:self clearSearchHighlightingInContext:self.searchContext];
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
	
	assert(nil != searchField);
	
	[self->responder findDialog:self didLoadManagedView:self.view];
}// loadView


@end // FindDialog_VC

// BELOW IS REQUIRED NEWLINE TO END FILE
