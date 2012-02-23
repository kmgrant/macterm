/*!	\file TerminalToolbar.h
	\brief Items used in the toolbars of terminal windows.
	
	This module focuses on the Cocoa implementation even
	though Carbon-based toolbar items are still in use.
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
@class ListenerModel_StandardListener;

// application includes
#import "SessionRef.typedef.h"
#import "TerminalScreenRef.typedef.h"
#import "TerminalViewRef.typedef.h"
#import "TerminalWindow.h"



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

#pragma mark Types

/*!
An instance of this object should be created in order to handle
delegate requests for an NSToolbar that is meant to control
terminal windows.  If any items are instantiated that depend on
the state of an active session, they will be unavailable to the
user unless "setSession:" has been called.  (This session can
be changed as often as needed, e.g. to implement a floating
toolbar.)
*/
@interface TerminalToolbar_Delegate : NSObject
{
@private
	SessionRef		associatedSession;
	BOOL			allowExperimentalItems;
}

- (id)
initForToolbar:(NSToolbar*)_
experimentalItems:(BOOL)_;

// accessors

- (SessionRef)
session;
- (void)
setSession:(SessionRef)_;

@end // TerminalToolbar_Delegate


/*!
Extensions to NSToolbar that are valid whenever the "delegate"
of the toolbar is of class TerminalToolbar_Delegate.
*/
@interface NSToolbar (TerminalToolbar_NSToolbarExtensions)

- (TerminalToolbar_Delegate*)
terminalToolbarDelegate;

- (SessionRef)
terminalToolbarSession;

@end // NSToolbar (TerminalToolbar_NSToolbarExtensions)


/*!
Base class for items that need to monitor the session that is
associated with their toolbar’s delegate.
*/
@interface TerminalToolbar_SessionDependentItem : NSToolbarItem
{
}

- (id)
initWithItemIdentifier:(NSString*)_;

// accessors

- (SessionRef)
session;

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

@end


/*!
Base class for items that display a particular LED.
*/
@interface TerminalToolbar_LEDItem : TerminalToolbar_SessionDependentItem
{
@private
	ListenerModel_StandardListener*		screenChangeListener;
	unsigned int						indexOfLED;
}

- (id)
initWithItemIdentifier:(NSString*)_
oneBasedIndexOfLED:(unsigned int)_;

@end


/*!
Toolbar item “Bell”.
*/
@interface TerminalToolbar_ItemBell : TerminalToolbar_SessionDependentItem
{
@private
	ListenerModel_StandardListener*		screenChangeListener;
}
@end


/*!
Toolbar item “Customize”.
*/
@interface TerminalToolbar_ItemCustomize : NSToolbarItem
{
}
@end


/*!
Toolbar item “Force Quit”.
*/
@interface TerminalToolbar_ItemForceQuit : TerminalToolbar_SessionDependentItem
{
@private
	ListenerModel_StandardListener*		sessionChangeListener;
}
@end


/*!
Toolbar item “Full Screen”.
*/
@interface TerminalToolbar_ItemFullScreen : TerminalToolbar_SessionDependentItem
{
}
@end


/*!
Toolbar item “Hide”.
*/
@interface TerminalToolbar_ItemHide : TerminalToolbar_SessionDependentItem
{
}
@end


/*!
Toolbar item “L1”.
*/
@interface TerminalToolbar_ItemLED1 : TerminalToolbar_LEDItem
{
}
@end


/*!
Toolbar item “L2”.
*/
@interface TerminalToolbar_ItemLED2 : TerminalToolbar_LEDItem
{
}
@end


/*!
Toolbar item “L3”.
*/
@interface TerminalToolbar_ItemLED3 : TerminalToolbar_LEDItem
{
}
@end


/*!
Toolbar item “L4”.
*/
@interface TerminalToolbar_ItemLED4 : TerminalToolbar_LEDItem
{
}
@end


/*!
Toolbar item “Default”.
*/
@interface TerminalToolbar_ItemNewSessionDefaultFavorite : NSToolbarItem
{
}
@end


/*!
Toolbar item “Log-In Shell”.
*/
@interface TerminalToolbar_ItemNewSessionLogInShell : NSToolbarItem
{
}
@end


/*!
Toolbar item “Shell”.
*/
@interface TerminalToolbar_ItemNewSessionShell : NSToolbarItem
{
}
@end


/*!
Toolbar item “Print”.
*/
@interface TerminalToolbar_ItemPrint : TerminalToolbar_SessionDependentItem
{
}
@end


/*!
Toolbar item “Arrange in Front”.
*/
@interface TerminalToolbar_ItemStackWindows : NSToolbarItem
{
}
@end


/*!
Toolbar item “Suspend”.
*/
@interface TerminalToolbar_ItemSuspend : TerminalToolbar_SessionDependentItem
{
@private
	ListenerModel_StandardListener*		sessionChangeListener;
}
@end


/*!
Toolbar item “Tabs”.
*/
@interface TerminalToolbar_ItemTabs : NSToolbarItem
{
@private
	NSSegmentedControl*		segmentedControl;
	NSArray*				targets;
	SEL						action;
}

- (void)
setTabTargets:(NSArray*)_
andAction:(SEL)_;

@end


/*!
A sample object type that can be used to represent a tab in
the object array of a TerminalToolbar_ItemTabs instance.
*/
@interface TerminalToolbar_TabSource : NSObject
{
@private
	NSAttributedString*		description;
}

- (id)
initWithDescription:(NSAttributedString*)_;

- (NSAttributedString*)
attributedDescription;

- (void)
performAction:(id)_;

- (NSString*)
toolTip;

@end // TerminalToolbar_TabSource


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
@interface TerminalToolbar_Object : NSToolbar
{
}

- (id)
initWithIdentifier:(NSString*)_;

// toolbar overrides

- (void)
setDisplayMode:(NSToolbarDisplayMode)_;

- (void)
setSizeMode:(NSToolbarSizeMode)_;

- (void)
setVisible:(BOOL)_;

@end


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
@interface TerminalToolbar_Window : NSPanel
{
@private
	ListenerModel_StandardListener*		sessionFactoryChangeListener;
	TerminalToolbar_Delegate*			toolbarDelegate;
	BOOL								isDisplayingSheet : 1;
}

- (id)
initWithContentRect:(NSRect)_
screen:(NSScreen*)_;

@end

// BELOW IS REQUIRED NEWLINE TO END FILE
