/*!	\file TerminalWindow.h
	\brief Second-highest level of abstraction for local or
	remote shells.
	
	A terminal window is the entity which manages the Mac OS
	window, terminal views, scroll bars, toolbars and other
	elements that make up a terminal window.
	
	Where possible, use the APIs in Session.h to indirectly
	affect a terminal window as a result of a session operation.
	Similarly, look here before considering the use of even
	lower-level APIs from TerminalView.h, etc.
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
#ifdef __OBJC__
@class NSWindow;
@class TerminalToolbar_Delegate;
@class TerminalView_Controller;
@class TerminalView_ScrollableRootVC;
#else
class NSWindow;
class TerminalToolbar_Delegate;
class TerminalView_Controller;
class TerminalView_ScrollableRootVC;
#endif
#include <CoreServices/CoreServices.h>

// library includes
#ifdef __OBJC__
#	include <CoreUI.objc++.h>
#endif
#include <ListenerModel.h>
#ifdef __OBJC__
#	include <PopoverManager.objc++.h>
#endif

// application includes
#include "Preferences.h"
#include "TerminalScreenRef.typedef.h"
#ifdef __OBJC__
#	include "TerminalToolbar.objc++.h"
#endif
#include "TerminalView.h"



#pragma mark Constants

enum TerminalWindow_Result
{
	kTerminalWindow_ResultOK						= 0,	//!< no error occurred
	kTerminalWindow_ResultGenericFailure			= 1,	//!< unspecified problem
	kTerminalWindow_ResultInsufficientBufferSpace	= 2,	//!< not enough room in a provided array, for example
	kTerminalWindow_ResultInvalidReference			= 3		//!< TerminalWindowRef is not recognized
};

/*!
Setting changes that MacTerm allows other modules to “listen” for,
via TerminalWindow_StartMonitoring().
*/
enum TerminalWindow_Change
{
	kTerminalWindow_ChangeIconTitle			= 'NIcT',	//!< the title of a monitored Terminal Window’s
														//!  collapsed Dock tile has changed (context:
														//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeObscuredState		= 'ShHd',	//!< a monitored Terminal Window has been
														//!  hidden or redisplayed (context:
														//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeScreenDimensions	= 'Size',	//!< the screen dimensions of a monitored
														//!  Terminal Window have changed (context:
														//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeWindowTitle		= 'NWnT'	//!< the title of a monitored Terminal Window
														//!  has changed (context: TerminalWindowRef)
};

/*!
Unique descriptors for collections of terminal views.  For example,
these might be used to describe the collection of all views in the
entire window, or only the currently focused view, etc.
*/
enum TerminalWindow_ViewGroup
{
	kTerminalWindow_ViewGroupEverything		= '****',	//!< contains EVERY view in the window
	kTerminalWindow_ViewGroupActive			= 'Frnt',	//!< contains all views in the visible tab
};

#pragma mark Types

#include "TerminalWindowRef.typedef.h"

#ifdef __OBJC__

/*!
An object that can display a floating information bubble
on a terminal window or elsewhere on the screen.  This is
used during live resize and in response to certain other
events (such as an interrupted process).  It is also used
in Local Echo mode to show “invisible” characters.
*/
@interface TerminalWindow_InfoBubble : NSObject < PopoverManager_Delegate > //{
{
@private
	NSTextField*		_textView;
	NSView*				_paddingView;
	Popover_Window*		_infoWindow;
	PopoverManager_Ref	_popoverMgr;
	NSPoint				_idealWindowRelativeAnchorPoint;
	CGFloat				_delayBeforeRemoval;
	BOOL				_releaseOnClose;
}

// class methods
	+ (instancetype)
	sharedInfoBubble;

// initializers
	- (instancetype)
	initWithStringValue:(NSString*)_;

// accessors
	@property (assign) CGFloat
	delayBeforeRemoval; // set to 0 to disable auto-remove (then call "removeWithAnimation")
	@property (assign) BOOL
	releaseOnClose;
	@property (strong) NSString*
	stringValue; // updates display, and also resets "delayBeforeRemoval" to the default value

// new methods
	- (void)
	display; // automatically removed after "delayBeforeRemoval", if nonzero
	- (void)
	moveBelowCursorInTerminalWindow:(TerminalWindowRef)_;
	- (void)
	moveToCenterScreen:(NSScreen*)_;
	- (void)
	removeWithAnimation;
	- (void)
	reset; // call this before using the shared bubble for a new purpose

@end //}


/*!
Custom window class for terminals; mostly unchanged
from the base.
*/
@interface TerminalWindow_Object : NSWindow //{
{
@private
	BOOL	_constrainingToScreenSuspended;
}

// accessors
	@property (assign) BOOL
	constrainingToScreenSuspended;

@end //}


/*!
The type of view managed by "TerminalWindow_RootVC".
Currently used to handle layout (until a later SDK can
be adopted, where the view controller might directly
perform more view tasks).
*/
@interface TerminalWindow_RootView : CoreUI_LayoutView //{
{
@private
	NSMutableArray*		_scrollableTerminalRootViews;
}

// new methods
	- (void)
	addScrollableRootView:(TerminalView_ScrollableRootView*)_;
	- (NSEnumerator*)
	enumerateScrollableRootViews;
	- (void)
	removeScrollableRootView:(TerminalView_ScrollableRootView*)_;

@end //}


/*!
Custom root view controller that holds a scroll bar and
one or more terminal view controllers.  This is also
responsible for the layout of window views such as the
terminal scroll controllers and any displayed “bars”.
*/
@interface TerminalWindow_RootVC : NSViewController < CoreUI_ViewLayoutDelegate > //{
{
@private
	NSMutableArray*		_terminalScrollControllers;
}

// accessors
	- (TerminalWindow_RootView*)
	rootView;

// new methods
	- (void)
	addScrollControllerBeyondEdge:(NSRectEdge)_;
	- (NSEnumerator*)
	enumerateScrollControllers;
	- (void)
	enumerateTerminalViewControllersUsingBlock:(TerminalView_ControllerBlock)_;
	- (void)
	removeScrollController:(TerminalView_ScrollableRootVC*)_;

@end //}


/*!
Implements a window controller for a window that holds
at least one terminal view as a parent.  See
"TerminalWindowCocoa.xib".

A TerminalWindowRef should own this controller.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalWindow_Controller : NSWindowController < Commands_StandardSearching,
															Commands_TerminalScreenResizing,
															Commands_TextFormatting,
															Commands_WindowRenaming,
															NSWindowDelegate > //{
{
@private
	NSRect						_preFullScreenFrame;
	TerminalWindowRef			_terminalWindowRef;
	TerminalToolbar_Delegate*	_toolbarDelegate;
	TerminalWindow_RootVC*		_rootVC;
	NSString*					_preferredMacroSetName;
}

// initializers
	- (instancetype)
	initWithTerminalVC:(TerminalView_Controller*)_
	owner:(TerminalWindowRef)_;

// accessors
	- (TerminalWindow_RootVC*)
	rootViewController;
	@property (strong) NSString*
	preferredMacroSetName;
	@property (assign) TerminalWindowRef
	terminalWindowRef;

// new methods
	- (void)
	enumerateTerminalViewControllersUsingBlock:(TerminalView_ControllerBlock)_;
	- (void)
	setWindowButtonsHidden:(BOOL)_;

@end //}


@interface NSWindow (TerminalWindow_NSWindowExtensions) //{

// new methods
	- (TerminalWindow_Controller*)
	terminalWindowController;
	- (TerminalWindowRef)
	terminalWindowRef;

@end //}

#endif // __OBJC__

#pragma mark Callbacks

/*!
Terminal View Block

This is used in TerminalWindow_ForEachTerminalView().  If the stop
flag is set by the block, iteration will end early.

Note that it is sometimes more appropriate to iterate over Sessions
or Terminal Windows.  Carefully consider what you are trying to do
so that you iterate at the right level of abstraction.
*/
typedef void (^TerminalWindow_TerminalViewBlock)	(TerminalViewRef, Boolean&);



#pragma mark Public Methods

//!\name Creating and Destroying Terminal Windows
//@{

// DO NOT CREATE TERMINAL WINDOWS THIS WAY (USE SessionFactory METHODS, INSTEAD)
TerminalWindowRef
	TerminalWindow_New								(Preferences_ContextRef		inTerminalInfoOrNull = nullptr,
													 Preferences_ContextRef		inFontInfoOrNull = nullptr,
													 Preferences_ContextRef		inTranslationInfoOrNull = nullptr,
													 Boolean					inNoStagger = false);

void
	TerminalWindow_Dispose							(TerminalWindowRef*			inoutRefPtr);

Boolean
	TerminalWindow_IsValid							(TerminalWindowRef			inRef);

//@}

//!\name Terminal Window Information
//@{

void
	TerminalWindow_CopyWindowTitle					(TerminalWindowRef			inRef,
													 CFStringRef&				outName);

// TEMPORARY, FOR COCOA TRANSITION
void
	TerminalWindow_Focus							(TerminalWindowRef			inRef);

void
	TerminalWindow_GetFontAndSize					(TerminalWindowRef			inRef,
													 CFStringRef*				outFontFamilyNameOrNull,
													 CGFloat*					outFontSizeOrNull);

void
	TerminalWindow_GetScreens						(TerminalWindowRef			inRef,
													 UInt16						inArrayLength,
													 TerminalScreenRef*			outScreenArray,
													 UInt16*					outActualCountOrNull);

void
	TerminalWindow_GetScreenDimensions				(TerminalWindowRef			inRef,
													 UInt16*					outColumnCountPtrOrNull,
													 UInt16*					outRowCountPtrOrNull);

// TEMPORARY, FOR COCOA TRANSITION
Boolean
	TerminalWindow_IsFocused						(TerminalWindowRef			inRef);

Boolean
	TerminalWindow_IsFullScreen						(TerminalWindowRef			inRef);

Boolean
	TerminalWindow_IsFullScreenMode					();

Boolean
	TerminalWindow_IsObscured						(TerminalWindowRef			inRef);

Boolean
	TerminalWindow_ReconfigureViewsInGroup			(TerminalWindowRef			inRef,
													 TerminalWindow_ViewGroup	inViewGroup,
													 Preferences_ContextRef		inContext,
													 Quills::Prefs::Class		inPrefsClass);

NSWindow*
	TerminalWindow_ReturnNSWindow					(TerminalWindowRef			inRef);

TerminalScreenRef
	TerminalWindow_ReturnScreenWithFocus			(TerminalWindowRef			inRef);

TerminalViewRef
	TerminalWindow_ReturnViewWithFocus				(TerminalWindowRef			inRef);

// TEMPORARY, FOR COCOA TRANSITION
void
	TerminalWindow_Select							(TerminalWindowRef			inRef,
													 Boolean					inFocus = true);

void
	TerminalWindow_SetFontAndSize					(TerminalWindowRef			inRef,
													 CFStringRef				inFontFamilyNameOrNull,
													 CGFloat					inFontSizeOrZero);

Boolean
	TerminalWindow_SetFontRelativeSize				(TerminalWindowRef			inRef,
													 Float32					inDeltaFontSize,
													 Float32					inAbsoluteLimitOrZero = 0,
													 Boolean					inAllowUndo = true);

void
	TerminalWindow_SetObscured						(TerminalWindowRef			inRef,
													 Boolean					inIsHidden);

void
	TerminalWindow_SetScreenDimensions				(TerminalWindowRef			inRef,
													 UInt16						inNewColumnCount,
													 UInt16						inNewRowCount,
													 Boolean					inAllowUndo = true);

void
	TerminalWindow_SetIconTitle						(TerminalWindowRef			inRef,
													 CFStringRef				inName);

void
	TerminalWindow_SetWindowTitle					(TerminalWindowRef			inRef,
													 CFStringRef				inName);

// TEMPORARY, FOR COCOA TRANSITION
void
	TerminalWindow_SetVisible						(TerminalWindowRef			inRef,
													 Boolean					inIsVisible);

//@}

//!\name Iterating Over Terminal Views
//@{

TerminalWindow_Result
	TerminalWindow_ForEachTerminalView				(TerminalWindowRef			inRef,
													 TerminalWindow_TerminalViewBlock		inBlock);

//@}

//!\name Terminal Window Operations
//@{

void
	TerminalWindow_DisplayCustomFormatUI			(TerminalWindowRef			inRef);

void
	TerminalWindow_DisplayCustomScreenSizeUI		(TerminalWindowRef			inRef);

void
	TerminalWindow_DisplayCustomTranslationUI		(TerminalWindowRef			inRef);

void
	TerminalWindow_DisplayTextSearchDialog			(TerminalWindowRef			inRef);

Boolean
	TerminalWindow_IsTab							(TerminalWindowRef			inRef);

// API UNDER EVALUATION
void
	TerminalWindow_StackWindows						();

void
	TerminalWindow_StartMonitoring					(TerminalWindowRef			inRef,
													 TerminalWindow_Change		inForWhatChange,
													 ListenerModel_ListenerRef	inListener);

void
	TerminalWindow_StopMonitoring					(TerminalWindowRef			inRef,
													 TerminalWindow_Change		inForWhatChange,
													 ListenerModel_ListenerRef	inListener);

//@}

//!\name Getting Information From Mac OS Windows
//@{

TerminalWindowRef
	TerminalWindow_ReturnFromMainWindow				();

TerminalWindowRef
	TerminalWindow_ReturnFromKeyWindow				();

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
