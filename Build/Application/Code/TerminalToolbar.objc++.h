/*!	\file TerminalToolbar.objc++.h
	\brief Items used in the toolbars of terminal windows.
	
	This module focuses on the Cocoa implementation even
	though Carbon-based toolbar items are still in use.
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
@class CocoaExtensions_ObserverSpec;
@class ListenerModel_StandardListener;

// application includes
#import "SessionRef.typedef.h"
#import "TerminalScreenRef.typedef.h"
#import "TerminalViewRef.typedef.h"
#import "TerminalWindowRef.typedef.h"



#pragma mark Constants

extern NSString*	kTerminalToolbar_ItemIDCustomize;
extern NSString*	kTerminalToolbar_ItemIDNewSessionDefaultFavorite;
extern NSString*	kTerminalToolbar_ItemIDNewSessionLogInShell;
extern NSString*	kTerminalToolbar_ItemIDNewSessionShell;
extern NSString*	kTerminalToolbar_ItemIDStackWindows;
extern NSString*	kTerminalToolbar_DelegateSessionWillChangeNotification; // no userInfo is defined for this notification
extern NSString*	kTerminalToolbar_DelegateSessionDidChangeNotification; // no userInfo is defined for this notification
extern NSString*	kTerminalToolbar_ObjectDidChangeDisplayModeNotification; // no userInfo is defined for this notification
extern NSString*	kTerminalToolbar_ObjectDidChangeSizeModeNotification; // no userInfo is defined for this notification
extern NSString*	kTerminalToolbar_ObjectDidChangeVisibilityNotification; // no userInfo is defined for this notification

enum TerminalToolbar_TextLabelLayout
{
	kTerminalToolbar_TextLabelLayoutCenterJustified		= 0,
	kTerminalToolbar_TextLabelLayoutLeftJustified		= 1,
	kTerminalToolbar_TextLabelLayoutRightJustified		= 2,
};

#pragma mark Types

/*!
Although NSToolbar itself does not provide a way to detect a
change in the display, TerminalToolbar_Object does provide a
mechanism.  Implement this protocol in an NSToolbarItem to
indicate that the item should be notified of display changes.
*/
@protocol TerminalToolbar_DisplayModeSensitive < NSObject > //{

@required

	// respond to change in NSToolbarDisplayMode value for a TerminalToolbar_Object
	- (void)
	didChangeDisplayModeForToolbar:(NSToolbar*)_;

@end //}


/*!
Although NSToolbar itself does not provide a way for an item
to detect when it is added or removed from a toolbar, this is
possible using TerminalToolbar_Object.  Implement this protocol
in an NSToolbarItem to determine when that particular item is
added to a toolbar or removed from a toolbar.
*/
@protocol TerminalToolbar_ItemAddRemoveSensitive < NSObject > //{

@required

	// respond to item being added to valid toolbar (non-nil)
	- (void)
	item:(NSToolbarItem*)_
	willEnterToolbar:(NSToolbar*)_;

	// respond to item being removed from valid toolbar (non-nil)
	- (void)
	item:(NSToolbarItem*)_
	didExitToolbar:(NSToolbar*)_;

@end //}


/*!
This protocol is an explicit mechanism for deciding how an item
will look in a customization sheet.  The only mechanism Cocoa
has is "toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:"
(from the toolbar delegate), which indicates whether or not a
requested item is destined for the customization sheet.  With
this protocol, which is checked by TerminalToolbar_Delegate, an
item can simply return a proxy item that is directly returned for
the customization palette case.
*/
@protocol TerminalToolbar_ItemHasPaletteProxy < NSObject > //{

@required

	// return item for use in a customization sheet (be sure to set a "paletteLabel")
	- (NSToolbarItem*)
	paletteProxyToolbarItemWithIdentifier:(NSString*)_;

@end //}


/*!
Although NSToolbar itself does not provide a way to detect a
change in the size, TerminalToolbar_Object does provide a
mechanism.  Implement this protocol in an NSToolbarItem to
indicate that the item should be notified of size changes.
*/
@protocol TerminalToolbar_SizeSensitive < NSObject > //{

@required

	// respond to change in NSToolbarSizeMode value for a TerminalToolbar_Object
	- (void)
	didChangeSizeForToolbar:(NSToolbar*)_;

@end //}


/*!
This protocol allows a toolbar item to monitor its view for
changes in IDEAL size, which is simpler than navigating the
mess of potential recursion with observers and other conflicts.
This reduces the number of situations where the view frame
changes to one: when its ideal layout is known.
*/
@protocol TerminalToolbar_ViewFrameSensitive < NSObject > //{

@required

	// respond to change in IDEAL view height
	- (void)
	view:(NSView*)_
	didSetIdealFrameHeight:(CGFloat)_;

@end //}


/*!
This protocol allows a toolbar item to monitor its view for
changes in the window.  (Oddly, it is incredibly difficult
for a toolbar item or even a toolbar to figure out what
window it’s in.)
*/
@protocol TerminalToolbar_ViewWindowSensitive < NSObject > //{

@required

	// respond to actual change in current view window
	- (void)
	view:(NSView*)_
	didEnterWindow:(NSWindow*)_;

	// respond to proposed change in current view window
	- (void)
	willChangeWindowForView:(NSView*)_;

@end //}


/*!
An instance of this object should be created in order to handle
delegate requests for an NSToolbar that is meant to control
terminal windows.  If any items are instantiated that depend on
the state of an active session, they will be unavailable to the
user unless "setSession:" has been called.  (This session can
be changed as often as needed, e.g. to implement a floating
toolbar.)
*/
@interface TerminalToolbar_Delegate : NSObject < NSToolbarDelegate > //{
{
@private
	SessionRef		associatedSession;
	BOOL			allowExperimentalItems;
}

// initializers
	- (instancetype)
	initForToolbar:(NSToolbar*)_
	experimentalItems:(BOOL)_;

// accessors
	- (SessionRef)
	session;
	- (void)
	setSession:(SessionRef)_;

@end //}


/*!
Extensions to NSToolbar that are valid whenever the "delegate"
of the toolbar is of class TerminalToolbar_Delegate.
*/
@interface NSToolbar (TerminalToolbar_NSToolbarExtensions) //{

// accessors
	- (TerminalToolbar_Delegate*)
	terminalToolbarDelegate;
	- (SessionRef)
	terminalToolbarSession;

@end //}


/*!
Base class for items that need to monitor the session that is
associated with their toolbar’s delegate.
*/
@interface TerminalToolbar_SessionDependentItem : NSToolbarItem < TerminalToolbar_ItemAddRemoveSensitive > //{
{
@private
	SessionRef		_sessionHint;
}

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

// accessors
	- (SessionRef)
	session;
	- (void)
	setSessionHint:(SessionRef)_;
	- (TerminalScreenRef)
	terminalScreen;
	- (TerminalViewRef)
	terminalView;
	- (TerminalWindowRef)
	terminalWindow;

// overrides for subclasses
	- (void)
	didChangeSession;
	- (void)
	willChangeSession;

@end //}


/*!
Base class for items that display a particular LED.
*/
@interface TerminalToolbar_LEDItem : TerminalToolbar_SessionDependentItem //{
{
@private
	ListenerModel_StandardListener*		screenChangeListener;
	unsigned int						indexOfLED;
}

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_
	oneBasedIndexOfLED:(unsigned int)_;

@end //}


/*!
Toolbar item “Bell”.
*/
@interface TerminalToolbar_ItemBell : TerminalToolbar_SessionDependentItem //{
{
@private
	ListenerModel_StandardListener*		screenChangeListener;
}
@end //}


/*!
Toolbar item “Customize”.
*/
@interface TerminalToolbar_ItemCustomize : NSToolbarItem @end


/*!
Toolbar item “Force Quit”.
*/
@interface TerminalToolbar_ItemForceQuit : TerminalToolbar_SessionDependentItem //{
{
@private
	ListenerModel_StandardListener*		sessionChangeListener;
}
@end //}


/*!
Toolbar item “Full Screen”.
*/
@interface TerminalToolbar_ItemFullScreen : TerminalToolbar_SessionDependentItem @end


/*!
Toolbar item “Hide”.
*/
@interface TerminalToolbar_ItemHide : TerminalToolbar_SessionDependentItem @end


/*!
Toolbar item “L1”.
*/
@interface TerminalToolbar_ItemLED1 : TerminalToolbar_LEDItem @end


/*!
Toolbar item “L2”.
*/
@interface TerminalToolbar_ItemLED2 : TerminalToolbar_LEDItem @end


/*!
Toolbar item “L3”.
*/
@interface TerminalToolbar_ItemLED3 : TerminalToolbar_LEDItem @end


/*!
Toolbar item “L4”.
*/
@interface TerminalToolbar_ItemLED4 : TerminalToolbar_LEDItem @end


/*!
Toolbar item “Default”.
*/
@interface TerminalToolbar_ItemNewSessionDefaultFavorite : NSToolbarItem @end


/*!
Toolbar item “Log-In Shell”.
*/
@interface TerminalToolbar_ItemNewSessionLogInShell : NSToolbarItem @end


/*!
Toolbar item “Shell”.
*/
@interface TerminalToolbar_ItemNewSessionShell : NSToolbarItem @end


/*!
Toolbar item “Print”.
*/
@interface TerminalToolbar_ItemPrint : TerminalToolbar_SessionDependentItem @end


/*!
Toolbar item “Arrange in Front”.
*/
@interface TerminalToolbar_ItemStackWindows : NSToolbarItem @end


/*!
Toolbar item “Suspend”.
*/
@interface TerminalToolbar_ItemSuspend : TerminalToolbar_SessionDependentItem //{
{
@private
	ListenerModel_StandardListener*		sessionChangeListener;
}
@end //}


/*!
Toolbar item “Tabs”.
*/
@interface TerminalToolbar_ItemTabs : NSToolbarItem //{
{
@private
	NSSegmentedControl*		segmentedControl;
	NSArray*				targets;
	SEL						action;
}

// new methods
	- (void)
	setTabTargets:(NSArray*)_
	andAction:(SEL)_;

@end //}


/*!
A sample object type that can be used to represent a tab in
the object array of a TerminalToolbar_ItemTabs instance.
*/
@interface TerminalToolbar_TabSource : NSObject //{
{
@private
	NSAttributedString*		description;
}

// initializers
	- (instancetype)
	initWithDescription:(NSAttributedString*)_;

// actions
	- (void)
	performAction:(id)_;

// accessors
	- (NSAttributedString*)
	attributedDescription;
	- (NSString*)
	toolTip;

@end //}


/*!
Base toolbar item for close/minimize/zoom buttons.
*/
@interface TerminalToolbar_ItemWindowButton : NSToolbarItem < TerminalToolbar_ItemAddRemoveSensitive >
{
@private
	CocoaExtensions_ObserverSpec*	_viewWindowObserver;
	NSButton*						_button;
}

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

@end //}


/*!
Toolbar item “Close”.
*/
@interface TerminalToolbar_ItemWindowButtonClose : TerminalToolbar_ItemWindowButton //{

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

@end //}


/*!
Toolbar item “Minimize”.
*/
@interface TerminalToolbar_ItemWindowButtonMinimize : TerminalToolbar_ItemWindowButton //{

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

@end //}


/*!
Toolbar item “Zoom”.
*/
@interface TerminalToolbar_ItemWindowButtonZoom : TerminalToolbar_ItemWindowButton //{

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

@end //}


/*!
A subclass of NSTextField that allows the user to drag
the window when it is clicked.  Also automatically
adjusts font to fit better, and uses fading as part of
eventual truncation.
*/
@interface TerminalToolbar_TextLabel : NSTextField //{
{
	BOOL										_disableFrameMonitor : 1;
	BOOL										_frameDisplayEnabled : 1;
	BOOL										_gradientFadeEnabled : 1;
	BOOL										_mouseDownCanMoveWindow : 1;
	BOOL										_smallSize : 1;
	TerminalToolbar_TextLabelLayout				_labelLayout;
	id< TerminalToolbar_ViewFrameSensitive >	_idealSizeMonitor;
}

// class methods
	+ (CGImageRef)
	newFadeMaskImageWithSize:(NSSize)_
	labelLayout:(TerminalToolbar_TextLabelLayout)_;

// accessors
	@property (assign) id< TerminalToolbar_ViewFrameSensitive >
	idealSizeMonitor;
	@property (assign) BOOL
	mouseDownCanMoveWindow;
	@property (assign) TerminalToolbar_TextLabelLayout
	labelLayout;
	@property (assign) BOOL
	smallSize;

@end //}


/*!
A view that automatically binds its value to the title
of a window.  By default, this window matches any window
that the view is moved into (even if it moves multiple
times) but you can set "overrideWindow" to force the
title to come only from that window.
*/
@interface TerminalToolbar_WindowTitleLabel : TerminalToolbar_TextLabel //{
{
	NSWindow*									_overrideWindow;
	CocoaExtensions_ObserverSpec*				_windowTitleObserver;
	id< TerminalToolbar_ViewWindowSensitive >	_windowMonitor;
}

// initializers
	- (instancetype)
	initWithFrame:(NSRect)_;

// accessors
	@property (strong) NSWindow*
	overrideWindow;
	@property (assign) id< TerminalToolbar_ViewWindowSensitive >
	windowMonitor;

@end //}


/*!
Toolbar item “Window Title”.
*/
@interface TerminalToolbar_ItemWindowTitle : NSToolbarItem < TerminalToolbar_DisplayModeSensitive,
																TerminalToolbar_ItemAddRemoveSensitive,
																TerminalToolbar_ItemHasPaletteProxy,
																TerminalToolbar_SizeSensitive,
																TerminalToolbar_ViewFrameSensitive,
																TerminalToolbar_ViewWindowSensitive >
{
@private
	BOOL								_disableFrameMonitor;
	TerminalToolbar_WindowTitleLabel*	_textView;
}

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

@end //}


/*!
Toolbar item “Left-Aligned Title”.
*/
@interface TerminalToolbar_ItemWindowTitleLeft : TerminalToolbar_ItemWindowTitle < TerminalToolbar_ItemHasPaletteProxy > //{

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

@end //}


/*!
Toolbar item “Right-Aligned Title”.
*/
@interface TerminalToolbar_ItemWindowTitleRight : TerminalToolbar_ItemWindowTitle < TerminalToolbar_ItemHasPaletteProxy > //{

// initializers
	- (instancetype)
	initWithItemIdentifier:(NSString*)_;

@end //}


/*!
Use this subclass to create a terminal toolbar instead
of using NSToolbar directly in order to gain some useful
insights into the toolbar’s state changes.

"kTerminalToolbar_ObjectDidChangeDisplayModeNotification"
can be observed on this toolbar object to find out when
"setDisplayMode:" is used.

"kTerminalToolbar_ObjectDidChangeSizeModeNotification"
can be observed on this toolbar object to find out when
"setSizeMode:" is used.

"kTerminalToolbar_ObjectDidChangeVisibilityNotification"
can be observed on this toolbar object to find out when
"setVisible:" is used.
*/
@interface TerminalToolbar_Object : NSToolbar //{

// initializers
	- (instancetype)
	initWithIdentifier:(NSString*)_;

// NSToolbar
	- (void)
	setDisplayMode:(NSToolbarDisplayMode)_;
	- (void)
	setSizeMode:(NSToolbarSizeMode)_;
	- (void)
	setVisible:(BOOL)_;

@end //}


/*!
A floating panel that displays a terminal toolbar and
automatically adapts items based on whichever terminal
window is the main window.

The standard "toolbar" accessor of the window will
return an instance of a toolbar whose delegate is set
to TerminalToolbar_Delegate (and therefore you can
call the TerminalToolbar_NSToolbarExtensions category’s
methods on this toolbar).
*/
@interface TerminalToolbar_Window : NSPanel //{
{
@private
	ListenerModel_StandardListener*		sessionFactoryChangeListener;
	TerminalToolbar_Delegate*			toolbarDelegate;
	BOOL								isDisplayingSheet;
}

// initializers
	- (instancetype)
	initWithContentRect:(NSRect)_
	#if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_6
	styleMask:(NSUInteger)_
	#else
	styleMask:(NSWindowStyleMask)_
	#endif
	backing:(NSBackingStoreType)_
	defer:(BOOL)_;

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
