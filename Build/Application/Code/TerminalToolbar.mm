/*!	\file TerminalToolbar.mm
	\brief Items used in the toolbars of terminal windows.
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

#import "TerminalToolbar.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <CGContextSaveRestore.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <ListenerModel.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "EventLoop.h"
#import "MacroManager.h"
#import "Session.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalWindow.h"



#pragma mark Constants
namespace {

// NOTE: do not ever change these, that would only break user preferences
NSString*	kMy_ToolbarItemIDBell						= @"net.macterm.MacTerm.toolbaritem.bell";
NSString*	kMy_ToolbarItemIDForceQuit					= @"net.macterm.MacTerm.toolbaritem.kill";
NSString*	kMy_ToolbarItemIDFullScreen					= @"net.macterm.MacTerm.toolbaritem.fullscreen";
NSString*	kMy_ToolbarItemIDHide						= @"net.macterm.MacTerm.toolbaritem.hide";
NSString*	kMy_ToolbarItemIDLED1						= @"net.macterm.MacTerm.toolbaritem.led1";
NSString*	kMy_ToolbarItemIDLED2						= @"net.macterm.MacTerm.toolbaritem.led2";
NSString*	kMy_ToolbarItemIDLED3						= @"net.macterm.MacTerm.toolbaritem.led3";
NSString*	kMy_ToolbarItemIDLED4						= @"net.macterm.MacTerm.toolbaritem.led4";
NSString*	kMy_ToolbarItemIDMacro1						= @"net.macterm.MacTerm.toolbaritem.macro1";
NSString*	kMy_ToolbarItemIDMacro2						= @"net.macterm.MacTerm.toolbaritem.macro2";
NSString*	kMy_ToolbarItemIDMacro3						= @"net.macterm.MacTerm.toolbaritem.macro3";
NSString*	kMy_ToolbarItemIDMacro4						= @"net.macterm.MacTerm.toolbaritem.macro4";
NSString*	kMy_ToolbarItemIDMacro5						= @"net.macterm.MacTerm.toolbaritem.macro5";
NSString*	kMy_ToolbarItemIDMacro6						= @"net.macterm.MacTerm.toolbaritem.macro6";
NSString*	kMy_ToolbarItemIDMacro7						= @"net.macterm.MacTerm.toolbaritem.macro7";
NSString*	kMy_ToolbarItemIDMacro8						= @"net.macterm.MacTerm.toolbaritem.macro8";
NSString*	kMy_ToolbarItemIDMacro9						= @"net.macterm.MacTerm.toolbaritem.macro9";
NSString*	kMy_ToolbarItemIDMacro10					= @"net.macterm.MacTerm.toolbaritem.macro10";
NSString*	kMy_ToolbarItemIDMacro11					= @"net.macterm.MacTerm.toolbaritem.macro11";
NSString*	kMy_ToolbarItemIDMacro12					= @"net.macterm.MacTerm.toolbaritem.macro12";
NSString*	kMy_ToolbarItemIDNotifications				= @"net.macterm.MacTerm.toolbaritem.notifications";
NSString*	kMy_ToolbarItemIDPrint						= @"net.macterm.MacTerm.toolbaritem.print";
NSString*	kMy_ToolbarItemIDSuspend					= @"net.macterm.MacTerm.toolbaritem.suspend";
NSString*	kMy_ToolbarItemIDWindowButtonClose			= @"net.macterm.MacTerm.toolbaritem.windowbuttonclose";
NSString*	kMy_ToolbarItemIDWindowButtonMinimize		= @"net.macterm.MacTerm.toolbaritem.windowbuttonminimize";
NSString*	kMy_ToolbarItemIDWindowButtonZoom			= @"net.macterm.MacTerm.toolbaritem.windowbuttonzoom";
NSString*	kMy_ToolbarItemIDWindowTitle				= @"net.macterm.MacTerm.toolbaritem.windowtitle";
NSString*	kMy_ToolbarItemIDWindowTitleLeft			= @"net.macterm.MacTerm.toolbaritem.windowtitleleft";
NSString*	kMy_ToolbarItemIDWindowTitleRight			= @"net.macterm.MacTerm.toolbaritem.windowtitleright";

} // anonymous namespace

// some IDs are shared (NOTE: do not ever change these, that would only break user preferences)
NSString*	kTerminalToolbar_ItemIDCustomize					= @"net.macterm.MacTerm.toolbaritem.customize";
NSString*	kTerminalToolbar_ItemIDNewSessionDefaultFavorite	= @"net.macterm.MacTerm.toolbaritem.newsessiondefault";
NSString*	kTerminalToolbar_ItemIDNewSessionLogInShell			= @"net.macterm.MacTerm.toolbaritem.newsessionloginshell";
NSString*	kTerminalToolbar_ItemIDNewSessionShell				= @"net.macterm.MacTerm.toolbaritem.newsessionshell";
NSString*	kTerminalToolbar_ItemIDStackWindows					= @"net.macterm.MacTerm.toolbaritem.stackwindows";

NSString*	kTerminalToolbar_DelegateSessionWillChangeNotification		=
				@"kTerminalToolbar_DelegateSessionWillChangeNotification";
NSString*	kTerminalToolbar_DelegateSessionDidChangeNotification		=
				@"kTerminalToolbar_DelegateSessionDidChangeNotification";
NSString*	kTerminalToolbar_ObjectDidChangeDisplayModeNotification		=
				@"kTerminalToolbar_ObjectDidChangeDisplayModeNotification";
NSString*	kTerminalToolbar_ObjectDidChangeSizeModeNotification		=
				@"kTerminalToolbar_ObjectDidChangeSizeModeNotification";
NSString*	kTerminalToolbar_ObjectDidChangeVisibilityNotification		=
				@"kTerminalToolbar_ObjectDidChangeVisibilityNotification";

#pragma mark Types

/*!
Private properties.
*/
@interface TerminalToolbar_Delegate () //{

// accessors
	//! If YES, unfinished items are available for testing.
	@property (assign) BOOL
	allowExperimentalItems;
	//! The toolbar that this is a delegate for.
	@property (weak) NSToolbar*
	toolbar;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_Delegate (TerminalToolbar_DelegateInternal) //{

// new methods
	- (void)
	updateItemsForNewDisplayModeInToolbar:(NSToolbar*)_;
	- (void)
	updateItemsForNewSizeInToolbar:(NSToolbar*)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_SessionDependentItem () //{

// accessors
	//! For compliance with "TerminalToolbar_SessionHinting".
	@property (assign) SessionRef
	sessionHint;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_SessionDependentItem (TerminalToolbar_SessionDependentItemInternal) //{

// new methods
	- (void)
	installSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)_;
	- (void)
	removeSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_SessionDependentMenuItem () //{

// accessors
	//! For compliance with "TerminalToolbar_SessionHinting".
	@property (assign) SessionRef
	sessionHint;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_SessionDependentMenuItem (TerminalToolbar_SessionDependentMenuItemInternal) //{

// new methods
	- (void)
	installSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)_;
	- (void)
	removeSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_TextLabel () //{

// accessors
	//! Disables this particular view’s "textViewFrameDidChange:".
	//! This may seem redundant with "postsFrameChangedNotifications"
	//! from NSView but that property can auto-trigger a notification
	//! when its value is set back to YES, which makes it impossible
	//! to prevent a loop when disabling/restoring notifications from
	//! the callback itself (there would be an endless loop and crash).
	@property (assign) BOOL
	disableFrameMonitor;
	//! If set, the frame is rendered along with the text.  (This
	//! is currently useful only in toolbar items, and may move to
	//! a different view to enable a frame rectangle that is far
	//! different from the text rectangle.)
	@property (assign) BOOL
	frameDisplayEnabled;
	//! This should only be set in direct response to measuring the
	//! required space and determining that there is not enough
	//! room for all the text using the current font.  It tells the
	//! "drawRect:" implementation to use a fade-out effect as the
	//! text is truncated.
	@property (assign) BOOL
	gradientFadeEnabled;
	//! Stores information on text "stringValue" property observer.
	@property (strong) CocoaExtensions_ObserverSpec*
	stringValueObserver;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_TextLabel (TerminalToolbar_TextLabelInternal) //{

// new methods
	- (CGSize)
	idealFrameSizeForString:(NSString*)_;
	- (void)
	layOutLabelText;

@end //}

/*!
An NSBox subclass for the toolbar window title that responds
to mouse events in a similar way (e.g. double-click to zoom).
*/
@interface TerminalToolbar_WindowTitleBox : NSBox //{

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_WindowTitleLabel () //{

// accessors
	//! Stores information on window "title" property observer.
	@property (strong) CocoaExtensions_ObserverSpec*
	windowTitleObserver;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_LEDItem () //{

// accessors
	//! Some terminals allow several LEDs so this determines
	//! which LED is represented by the item.
	@property (assign) unsigned int
	indexOfLED;
	//! This is notified of changes to terminal screen properties.
	@property (strong) ListenerModel_StandardListener*
	screenChangeListener;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_LEDItem (TerminalToolbar_LEDItemInternal) //{

// methods of the form required by ListenerModel_StandardListener
	- (void)
	model:(ListenerModel_Ref)_
	screenChange:(ListenerModel_Event)_
	context:(void*)_;

// new methods
	- (void)
	setStateFromScreen:(TerminalScreenRef)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemBell () //{

// accessors
	//! This is notified of changes to terminal screen properties.
	@property (strong) ListenerModel_StandardListener*
	screenChangeListener;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemBell (TerminalToolbar_ItemBellInternal) //{

// methods of the form required by ListenerModel_StandardListener
	- (void)
	model:(ListenerModel_Ref)_
	screenChange:(ListenerModel_Event)_
	context:(void*)_;

// new methods
	- (void)
	setStateFromScreen:(TerminalScreenRef)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemForceQuit () //{

// accessors
	//! This is notified of changes to session properties.
	@property (strong) ListenerModel_StandardListener*
	sessionChangeListener;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemForceQuit (TerminalToolbar_ItemForceQuitInternal) //{

// methods of the form required by ListenerModel_StandardListener
	- (void)
	model:(ListenerModel_Ref)_
	sessionChange:(ListenerModel_Event)_
	context:(void*)_;

// new methods
	- (void)
	setStateFromSession:(SessionRef)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemMacro () //{

// accessors
	//! Constrains height of view to fit title and/or icon.
	@property (strong) NSLayoutConstraint*
	heightConstraint;
	//! This is notified when a macro set is modified.
	@property (strong) ListenerModel_StandardListener*
	macroManagerChangeListener;
	//! This is notified when preferences are modified.
	@property (strong) ListenerModel_StandardListener*
	preferenceChangeListener;
	//! The tag is used to keep track of a particular macro in
	//! the selected set of macros.
	@property (assign) NSInteger
	tag;
	//! Allows the item to occupy a flexible width.
	@property (strong) NSLayoutConstraint*
	widthConstraintMax;
	//! Encourages the item to not truncate the title.
	@property (strong) NSLayoutConstraint*
	widthConstraintMin;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemMacro (TerminalToolbar_ItemMacroInternal) //{

// methods of the form required by ListenerModel_StandardListener
	- (void)
	model:(ListenerModel_Ref)_
	macroManagerChange:(ListenerModel_Event)_
	context:(void*)_;
	- (void)
	model:(ListenerModel_Ref)_
	preferenceChange:(ListenerModel_Event)_
	context:(void*)_;

// new methods
	- (NSButton*)
	actionButton;
	- (NSSize)
	calculateIdealSize;
	- (NSBox*)
	containerViewAsBox;
	- (void)
	setStateForToolbar:(NSToolbar*)_;
	- (void)
	setTagUnconditionally:(NSInteger)_;
	- (void)
	updateSizeConstraints;
	- (void)
	updateWithPreferenceValuesFromMacroSet:(Preferences_ContextRef)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemNotifyOnActivity () //{

// accessors
	//! This is implemented to fix a bug in macOS 10.15, which
	//! could crash at runtime in toolbars because "isBordered"
	//! is invoked by the OS on NSMenuToolbarItem instances
	//! (which does not otherwise define the method).
	//!
	//! By subclassing NSMenuToolbarItem to implement the method,
	//! the system will not raise an exception on macOS 10.15.
	//! (This is not an issue on macOS 11 Big Sur.)
	@property (assign, getter=isBordered) BOOL
	bordered;
	//! This is notified of changes to session properties.
	@property (strong) ListenerModel_StandardListener*
	sessionChangeListener;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemNotifyOnActivity (TerminalToolbar_ItemNotifyOnActivityInternal) //{

// methods of the form required by ListenerModel_StandardListener
	- (void)
	model:(ListenerModel_Ref)_
	sessionChange:(ListenerModel_Event)_
	context:(void*)_;

// new methods
	- (void)
	session:(SessionRef)_
	setAsCurrentBinding:(BOOL)_;
	- (void)
	setStateFromSession:(SessionRef)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemSuspend () //{

// accessors
	//! This is notified of changes to session properties.
	@property (strong) ListenerModel_StandardListener*
	sessionChangeListener;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemSuspend (TerminalToolbar_ItemSuspendInternal) //{

// methods of the form required by ListenerModel_StandardListener
	- (void)
	model:(ListenerModel_Ref)_
	sessionChange:(ListenerModel_Event)_
	context:(void*)_;

// new methods
	- (void)
	setStateFromSession:(SessionRef)_;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemWindowButton () //{

// accessors
	//! The view that displays the window button (a standard
	//! Close, Minimize or Zoom button).
	@property (strong) NSButton*
	button;
	//! Stores information on view "window" property observer.
	@property (strong) CocoaExtensions_ObserverSpec*
	viewWindowObserver;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemWindowButton (TerminalToolbar_ItemWindowButtonInternal) //{

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemWindowTitle () //{

// accessors
	//! Disables frame notifications to prevent possible looping.
	@property (assign) BOOL
	disableFrameMonitor;
	//! Constrains the height of the view to fit the largest
	//! possible title text.
	@property (strong) NSLayoutConstraint*
	heightConstraint;
	//! Allows the item to occupy a flexible width.
	@property (strong) NSLayoutConstraint*
	widthConstraintMax;
	//! Encourages the item to not truncate the title.
	@property (strong) NSLayoutConstraint*
	widthConstraintMin;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemWindowTitle (TerminalToolbar_ItemWindowTitleInternal) //{

// new methods
	- (NSSize)
	calculateIdealSize;
	- (NSBox*)
	containerViewAsBox;
	- (void)
	setStateForToolbar:(NSToolbar*)_;
	- (TerminalToolbar_WindowTitleLabel*)
	textView;
	- (void)
	updateSizeConstraints;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_Object () //{

// accessors
	//! This is updated when special window title items
	//! are placed in the toolbar so that you can easily
	//! determine the placement of the title (left,
	//! center or right).
	@property (assign) NSTextAlignment
	titleJustification;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_Window () //{

// accessors
	//! This keeps track of sheets so that window anchoring to
	//! screen edges can be temporarily disabled.
	@property (assign) BOOL
	isDisplayingSheet;
	//! This keeps track as any session is activated or deactivated
	//! so that the floating toolbar can update itself.
	@property (strong) ListenerModel_StandardListener*
	sessionFactoryChangeListener;
	//! This determines which items are allowed in terminal toolbars.
	@property (strong) TerminalToolbar_Delegate*
	toolbarDelegate;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_Window (TerminalToolbar_WindowInternal) //{

// class methods
	+ (void)
	refreshWindows;

// methods of the form required by ListenerModel_StandardListener
	- (void)
	model:(ListenerModel_Ref)_
	sessionFactoryChange:(ListenerModel_Event)_
	context:(void*)_;

// new methods
	- (void)
	setFrameBasedOnRect:(NSRect)_;
	- (void)
	setFrameWithPossibleAnimation;

// notifications
	- (void)
	applicationDidChangeScreenParameters:(NSNotification*)_;
	- (void)
	toolbarDidChangeDisplayMode:(NSNotification*)_;
	- (void)
	toolbarDidChangeSizeMode:(NSNotification*)_;
	- (void)
	toolbarDidChangeVisibility:(NSNotification*)_;
	- (void)
	windowDidBecomeKey:(NSNotification*)_;
	- (void)
	windowDidResignKey:(NSNotification*)_;
	- (void)
	windowWillBeginSheet:(NSNotification*)_;
	- (void)
	windowDidEndSheet:(NSNotification*)_;

@end //}

#pragma mark Variables
namespace {

TerminalToolbar_Window*			gSharedTerminalToolbar = nil;

} // anonymous namespace



#pragma mark -
@implementation NSToolbar (TerminalToolbar_NSToolbarExtensions) //{


#pragma mark New Methods


/*!
Extends NSToolbar to return the "delegate" of the toolbar as an
instance of class TerminalToolbar_Delegate (just a raw cast).

(4.0)
*/
- (TerminalToolbar_Delegate*)
terminalToolbarDelegate
{
	TerminalToolbar_Delegate*	result = nil;
	id							object = [self delegate];
	
	
	if ([object isKindOfClass:[TerminalToolbar_Delegate class]])
	{
		result = (TerminalToolbar_Delegate*)object;
	}
	
	return result;
}// terminalToolbarDelegate


/*!
Extends NSToolbar (assuming the "delegate" of the toolbar is of
class TerminalToolbar_Delegate) so that it is easy to retrieve
the associated session.  This invokes the "session" accessor
method on the delegate.

(4.0)
*/
- (SessionRef)
terminalToolbarSession
{
	SessionRef		result = [[self terminalToolbarDelegate] session];
	
	
	return result;
}// terminalToolbarSession


@end //} NSToolbar (TerminalToolbar_NSToolbarExtensions)


#pragma mark -
@implementation TerminalToolbar_Delegate //{


@synthesize session = _session;


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initForToolbar:(NSToolbar*)		aToolbar
experimentalItems:(BOOL)		experimentalFlag
{
	self = [super init];
	if (nil != self)
	{
		_allowExperimentalItems = experimentalFlag;
		_session = nullptr;
		_toolbar = aToolbar;
		
		[self whenObject:aToolbar postsNote:kTerminalToolbar_ObjectDidChangeDisplayModeNotification
							performSelector:@selector(toolbarDidChangeDisplayMode:)];
		[self whenObject:aToolbar postsNote:kTerminalToolbar_ObjectDidChangeSizeModeNotification
							performSelector:@selector(toolbarDidChangeSizeMode:)];
		
		[self updateItemsForNewDisplayModeInToolbar:aToolbar];
		[self updateItemsForNewSizeInToolbar:aToolbar];
	}
	return self;
}// initForToolbar:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark Accessors


// (See description in class interface.)
- (SessionRef)
session
{
	return _session;
}
- (void)
setSession:(SessionRef)		aSession
{
	if (_session != aSession)
	{
		[self postNote:kTerminalToolbar_DelegateSessionWillChangeNotification];
		_session = aSession;
		[self postNote:kTerminalToolbar_DelegateSessionDidChangeNotification];
	}
}// setSession


#pragma mark Notifications


/*!
Keeps track of changes to the mode (such as “text only”)
so that the window layout can adapt if necessary.

(2018.03)
*/
- (void)
toolbarDidChangeDisplayMode:(NSNotification*)		aNotification
{
	if (NO == [aNotification.object isKindOfClass:NSToolbar.class])
	{
		Console_Warning(Console_WriteLine, "toolbar display-change notification expected an NSToolbar object!");
	}
	else
	{
		NSToolbar*		asToolbar = STATIC_CAST(aNotification.object, NSToolbar*);
		
		
		[self updateItemsForNewDisplayModeInToolbar:asToolbar];
	}
}// toolbarDidChangeDisplayMode:


/*!
Keeps track of changes to the icon and label size
so that the window layout can adapt if necessary.

(2018.03)
*/
- (void)
toolbarDidChangeSizeMode:(NSNotification*)		aNotification
{
	if (NO == [aNotification.object isKindOfClass:NSToolbar.class])
	{
		Console_Warning(Console_WriteLine, "toolbar size-change notification expected an NSToolbar object!");
	}
	else
	{
		NSToolbar*		asToolbar = STATIC_CAST(aNotification.object, NSToolbar*);
		
		
		[self updateItemsForNewSizeInToolbar:asToolbar];
	}
}// toolbarDidChangeSizeMode:


#pragma mark NSToolbarDelegate


/*!
Responds when the specified kind of toolbar item should be
constructed for the given toolbar.

(4.0)
*/
- (NSToolbarItem*)
toolbar:(NSToolbar*)				toolbar
itemForItemIdentifier:(NSString*)	itemIdentifier
willBeInsertedIntoToolbar:(BOOL)	willBeInToolbar
{
#pragma unused(toolbar)
	NSToolbarItem*		result = nil;
	
	
	// NOTE: no need to handle standard items here
	if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDBell])
	{
		result = [[TerminalToolbar_ItemBell alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDCustomize])
	{
		result = [[TerminalToolbar_ItemCustomize alloc] init];
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDForceQuit])
	{
		result = [[TerminalToolbar_ItemForceQuit alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDFullScreen])
	{
		// this is redundant with the abilities of "TerminalToolbar_ItemWindowButtonZoom"
		// but it is more convenient, especially when exiting Full Screen (also, the
		// “off switch” option has been removed, this is effectively its replacement)
		result = [[TerminalToolbar_ItemFullScreen alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDHide])
	{
		result = [[TerminalToolbar_ItemHide alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED1])
	{
		result = [[TerminalToolbar_ItemLED1 alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED2])
	{
		result = [[TerminalToolbar_ItemLED2 alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED3])
	{
		result = [[TerminalToolbar_ItemLED3 alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED4])
	{
		result = [[TerminalToolbar_ItemLED4 alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro1])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 1;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro2])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 2;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro3])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 3;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro4])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 4;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro5])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 5;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro6])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 6;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro7])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 7;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro8])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 8;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro9])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 9;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro10])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 10;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro11])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 11;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDMacro12])
	{
		result = [[TerminalToolbar_ItemMacro alloc] initWithItemIdentifier:itemIdentifier];
		result.tag = 12;
		result.visibilityPriority = NSToolbarItemVisibilityPriorityLow;
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDNewSessionDefaultFavorite])
	{
		result = [[TerminalToolbar_ItemNewSessionDefaultFavorite alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDNewSessionLogInShell])
	{
		result = [[TerminalToolbar_ItemNewSessionLogInShell alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDNewSessionShell])
	{
		result = [[TerminalToolbar_ItemNewSessionShell alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDNotifications])
	{
		result = [[TerminalToolbar_ItemNotifyOnActivity alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDPrint])
	{
		result = [[TerminalToolbar_ItemPrint alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDStackWindows])
	{
		result = [[TerminalToolbar_ItemStackWindows alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDSuspend])
	{
		result = [[TerminalToolbar_ItemSuspend alloc] init];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowButtonClose])
	{
		result = [[TerminalToolbar_ItemWindowButtonClose alloc] initWithItemIdentifier:itemIdentifier];
		//result.visibilityPriority = NSToolbarItemVisibilityPriorityHigh;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowButtonMinimize])
	{
		result = [[TerminalToolbar_ItemWindowButtonMinimize alloc] initWithItemIdentifier:itemIdentifier];
		//result.visibilityPriority = NSToolbarItemVisibilityPriorityHigh;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowButtonZoom])
	{
		result = [[TerminalToolbar_ItemWindowButtonZoom alloc] initWithItemIdentifier:itemIdentifier];
		//result.visibilityPriority = NSToolbarItemVisibilityPriorityHigh;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitle])
	{
		result = [[TerminalToolbar_ItemWindowTitle alloc] initWithItemIdentifier:itemIdentifier];
		result.visibilityPriority = NSToolbarItemVisibilityPriorityHigh;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleLeft])
	{
		result = [[TerminalToolbar_ItemWindowTitleLeft alloc] initWithItemIdentifier:itemIdentifier];
		result.visibilityPriority = NSToolbarItemVisibilityPriorityHigh;
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleRight])
	{
		result = [[TerminalToolbar_ItemWindowTitleRight alloc] initWithItemIdentifier:itemIdentifier];
		result.visibilityPriority = NSToolbarItemVisibilityPriorityHigh;
	}
	
	// initialize session information if available (this isn’t really
	// necessary for new toolbars full of items; it is only important
	// when the user customizes the toolbar, to ensure that new items
	// immediately display appropriate data from the target session)
	if ((willBeInToolbar) && [result conformsToProtocol:@protocol(TerminalToolbar_SessionHinting)])
	{
		id< TerminalToolbar_SessionHinting >	asSessionHinting = STATIC_CAST(result, id< TerminalToolbar_SessionHinting >);
		
		
		[asSessionHinting setSessionHint:self.session];
	}
	
	// if this item is being constructed for use in a customization sheet
	// and the original has a special palette appearance, replace it with
	// that proxy item
	if ((NO == willBeInToolbar) && [result conformsToProtocol:@protocol(TerminalToolbar_ItemHasPaletteProxy)])
	{
		id< TerminalToolbar_ItemHasPaletteProxy >	asImplementer = STATIC_CAST(result, id< TerminalToolbar_ItemHasPaletteProxy >);
		NSToolbarItem*								proxyItem = [asImplementer paletteProxyToolbarItemWithIdentifier:itemIdentifier];
		
		
		if (nil == proxyItem)
		{
			Console_Warning(Console_WriteValueCFString, "failed to create palette proxy for toolbar item, identifier", BRIDGE_CAST(itemIdentifier, CFStringRef));
		}
		else
		{
			result = proxyItem;
		}
	}
	
	return result;
}// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:


/*!
Returns the identifiers for the kinds of items that can appear
in the given toolbar.

(4.0)
*/
- (NSArray*)
toolbarAllowedItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	NSMutableArray*		result = [NSMutableArray arrayWithCapacity:0];
	
	
	if (self.allowExperimentalItems)
	{
		// (nothing yet)
	}
	
	[result addObjectsFromArray:@[
									// NOTE: these are ordered with their palette arrangement in mind
									kTerminalToolbar_ItemIDNewSessionDefaultFavorite,
									kTerminalToolbar_ItemIDNewSessionLogInShell,
									kTerminalToolbar_ItemIDNewSessionShell,
									NSToolbarSpaceItemIdentifier,
									NSToolbarFlexibleSpaceItemIdentifier,
									kMy_ToolbarItemIDWindowButtonClose,
									kMy_ToolbarItemIDWindowButtonMinimize,
									kMy_ToolbarItemIDWindowButtonZoom,
									kMy_ToolbarItemIDLED1,
									kMy_ToolbarItemIDLED2,
									kMy_ToolbarItemIDLED3,
									kMy_ToolbarItemIDLED4,
									kMy_ToolbarItemIDMacro1,
									kMy_ToolbarItemIDMacro2,
									kMy_ToolbarItemIDMacro3,
									kMy_ToolbarItemIDMacro4,
									kMy_ToolbarItemIDMacro5,
									kMy_ToolbarItemIDMacro6,
									kMy_ToolbarItemIDMacro7,
									kMy_ToolbarItemIDMacro8,
									kMy_ToolbarItemIDMacro9,
									kMy_ToolbarItemIDMacro10,
									kMy_ToolbarItemIDMacro11,
									kMy_ToolbarItemIDMacro12,
									kMy_ToolbarItemIDFullScreen,
									kMy_ToolbarItemIDWindowTitleLeft,
									kMy_ToolbarItemIDWindowTitle,
									kMy_ToolbarItemIDWindowTitleRight,
									kMy_ToolbarItemIDPrint,
									kMy_ToolbarItemIDHide,
									kMy_ToolbarItemIDForceQuit,
									kMy_ToolbarItemIDSuspend,
									kMy_ToolbarItemIDBell,
									kMy_ToolbarItemIDNotifications,
									kTerminalToolbar_ItemIDStackWindows,
									kTerminalToolbar_ItemIDCustomize,
								]];
	
	return result;
}// toolbarAllowedItemIdentifiers


/*!
Returns the identifiers for the items that will appear in the
given toolbar whenever the user has not customized it.

(4.0)
*/
- (NSArray*)
toolbarDefaultItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	// this list should not contain any “experimental” items
	return @[
				kMy_ToolbarItemIDWindowButtonClose,
				kMy_ToolbarItemIDWindowButtonMinimize,
				kMy_ToolbarItemIDWindowButtonZoom,
				NSToolbarSpaceItemIdentifier,
				kMy_ToolbarItemIDHide,
				kMy_ToolbarItemIDForceQuit,
				kMy_ToolbarItemIDSuspend,
				kMy_ToolbarItemIDWindowTitle,
				kMy_ToolbarItemIDBell,
				kMy_ToolbarItemIDNotifications,
				kMy_ToolbarItemIDPrint,
				kMy_ToolbarItemIDFullScreen, // now redundant with "kMy_ToolbarItemIDWindowButtonZoom"
				NSToolbarSpaceItemIdentifier,
				NSToolbarSpaceItemIdentifier,
				NSToolbarSpaceItemIdentifier,
				kTerminalToolbar_ItemIDCustomize,
			];
}// toolbarDefaultItemIdentifiers


/*!
Called when an item has been removed from a toolbar.

(2018.03)
*/
- (void)
toolbarDidRemoveItem:(NSNotification*)		aNotification
{
	if (NO == [aNotification.object isKindOfClass:NSToolbar.class])
	{
		Console_Warning(Console_WriteLine, "toolbar delegate did-remove-item notification expected an NSToolbar object!");
	}
	else
	{
		NSToolbar*					asToolbar = STATIC_CAST(aNotification.object, NSToolbar*);
		TerminalToolbar_Object*		asTerminalToolbar = ([asToolbar isKindOfClass:TerminalToolbar_Object.class] ? STATIC_CAST(asToolbar, TerminalToolbar_Object*) : nil);
		id							itemValue = [aNotification.userInfo objectForKey:@"item"];
		
		
		if (nil == itemValue)
		{
			Console_Warning(Console_WriteLine, "toolbar delegate did-remove-item notification expected an 'item' dictionary key");
		}
		else if (NO == [itemValue isKindOfClass:NSToolbarItem.class])
		{
			Console_Warning(Console_WriteLine, "toolbar delegate did-remove-item notification expected an NSToolbarItem 'item' value!");
		}
		else
		{
			NSToolbarItem*		asToolbarItem = STATIC_CAST(itemValue, NSToolbarItem*);
			
			
			if ([asToolbarItem.itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleLeft] ||
				[asToolbarItem.itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleRight])
			{
				// assume title is returning to center position
				asTerminalToolbar.titleJustification = NSTextAlignmentCenter;
			}
			
			if ([asToolbarItem conformsToProtocol:@protocol(TerminalToolbar_ItemAddRemoveSensitive)])
			{
				id< TerminalToolbar_ItemAddRemoveSensitive >	asImplementer = STATIC_CAST(asToolbarItem, id< TerminalToolbar_ItemAddRemoveSensitive >);
				
				
				[asImplementer item:asToolbarItem didExitToolbar:asToolbar];
			}
			else
			{
				//Console_Warning(Console_WriteValueCFString, "removed item does not implement 'TerminalToolbar_ItemAddRemoveSensitive'; class", BRIDGE_CAST(NSStringFromClass(asToolbarItem.class), CFStringRef)); // debug
				//Console_Warning(Console_WriteValueAddress, "removed item does not implement 'TerminalToolbar_ItemAddRemoveSensitive'", BRIDGE_CAST(asToolbarItem, void*)); // debug
			}
		}
	}
}// toolbarDidRemoveItem:


/*!
Called when an item is about to be added to a toolbar.

(2018.03)
*/
- (void)
toolbarWillAddItem:(NSNotification*)	aNotification
{
	if (NO == [aNotification.object isKindOfClass:NSToolbar.class])
	{
		Console_Warning(Console_WriteLine, "toolbar delegate will-add-item notification expected an NSToolbar object!");
	}
	else
	{
		NSToolbar*					asToolbar = STATIC_CAST(aNotification.object, NSToolbar*);
		TerminalToolbar_Object*		asTerminalToolbar = ([asToolbar isKindOfClass:TerminalToolbar_Object.class] ? STATIC_CAST(asToolbar, TerminalToolbar_Object*) : nil);
		id							itemValue = [aNotification.userInfo objectForKey:@"item"];
		
		
		if (nil == itemValue)
		{
			Console_Warning(Console_WriteLine, "toolbar delegate will-add-item notification expected an 'item' dictionary key");
		}
		else if (NO == [itemValue isKindOfClass:NSToolbarItem.class])
		{
			Console_Warning(Console_WriteLine, "toolbar delegate will-add-item notification expected an NSToolbarItem 'item' value!");
		}
		else
		{
			NSToolbarItem*		asToolbarItem = STATIC_CAST(itemValue, NSToolbarItem*);
			
			
			// keep track of unusual title positions
			if ([asToolbarItem.itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleLeft])
			{
				asTerminalToolbar.titleJustification = NSTextAlignmentLeft;
			}
			else if ([asToolbarItem.itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleRight])
			{
				asTerminalToolbar.titleJustification = NSTextAlignmentRight;
			}
			else if ([asToolbarItem.itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitle])
			{
				asTerminalToolbar.titleJustification = NSTextAlignmentCenter;
			}
			
			if ([asToolbarItem conformsToProtocol:@protocol(TerminalToolbar_ItemAddRemoveSensitive)])
			{
				id< TerminalToolbar_ItemAddRemoveSensitive >	asImplementer = STATIC_CAST(asToolbarItem, id< TerminalToolbar_ItemAddRemoveSensitive >);
				
				
				[asImplementer item:asToolbarItem willEnterToolbar:asToolbar];
			}
			else
			{
				//Console_Warning(Console_WriteValueCFString, "added item does not implement 'TerminalToolbar_ItemAddRemoveSensitive'; class", BRIDGE_CAST(NSStringFromClass(asToolbarItem.class), CFStringRef)); // debug
				//Console_Warning(Console_WriteValueAddress, "added item does not implement 'TerminalToolbar_ItemAddRemoveSensitive'", BRIDGE_CAST(asToolbarItem, void*)); // debug
			}
		}
	}
}// toolbarWillAddItem:


@end //} TerminalToolbar_Delegate


#pragma mark -
@implementation TerminalToolbar_Delegate (TerminalToolbar_DelegateInternal) //{


#pragma mark Internal Methods

/*!
Asks all toolbar items to update themselves based on the
display mode setting of the toolbar (such as “icon only”).

This only affects items that implement the protocol
"TerminalToolbar_DisplayModeSensitive".

(2018.03)
*/
- (void)
updateItemsForNewDisplayModeInToolbar:(NSToolbar*)		aToolbar
{
	for (NSToolbarItem* anItem in aToolbar.items)
	{
		if ([anItem conformsToProtocol:@protocol(TerminalToolbar_DisplayModeSensitive)])
		{
			id< TerminalToolbar_DisplayModeSensitive >		asImplementer = STATIC_CAST(anItem, id< TerminalToolbar_DisplayModeSensitive >);
			
			
			[asImplementer didChangeDisplayModeForToolbar:aToolbar];
		}
	}
}// updateItemsForNewDisplayModeInToolbar:


/*!
Asks all toolbar items to update themselves based on the
size setting of the toolbar (such as “small”).

This only affects items that implement the protocol
"TerminalToolbar_SizeSensitive".

(2018.03)
*/
- (void)
updateItemsForNewSizeInToolbar:(NSToolbar*)		aToolbar
{
	for (NSToolbarItem* anItem in aToolbar.items)
	{
		if ([anItem conformsToProtocol:@protocol(TerminalToolbar_SizeSensitive)])
		{
			id< TerminalToolbar_SizeSensitive >		asImplementer = STATIC_CAST(anItem, id< TerminalToolbar_SizeSensitive >);
			
			
			[asImplementer didChangeSizeForToolbar:aToolbar];
		}
	}
}// updateItemsForNewSizeInToolbar:


@end //} TerminalToolbar_Delegate (TerminalToolbar_DelegateInternal)


#pragma mark -
@implementation TerminalToolbar_ItemBell //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDBell];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Bell", @"toolbar item name; for toggling the terminal bell sound");
		self.paletteLabel = self.label;
		
		self.screenChangeListener = [[ListenerModel_StandardListener alloc]
										initWithTarget:self
														eventFiredSelector:@selector(model:screenChange:context:)];
		
		[self setStateFromScreen:nullptr];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self willChangeSession]; // automatically stop monitoring the screen
}// dealloc


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performBellToggle:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id							result = [super copyWithZone:zone];
	TerminalToolbar_ItemBell*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemBell.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemBell*);
	asSelf.screenChangeListener = [self.screenChangeListener copy];
	asSelf.screenChangeListener.target = asSelf;
	
	return result;
}// copyWithZone:


#pragma mark TerminalToolbar_SessionDependentItem


/*!
Invoked by the superclass whenever the target session of
the owning toolbar is about to change.

Responds by removing monitors on the previous session’s
screen buffers.

(4.0)
*/
- (void)
willChangeSession
{
	[super willChangeSession];
	
	TerminalScreenRef	screen = [self terminalScreen];
	
	
	if (nullptr != screen)
	{
		Terminal_StopMonitoring(screen, kTerminal_ChangeAudioState, self.screenChangeListener.listenerRef);
		[self setStateFromScreen:nil];
	}
}// willChangeSession


/*!
Invoked by the superclass whenever the target session of
the owning toolbar has changed.

Responds by updating the icon state.

(4.0)
*/
- (void)
didChangeSession
{
	[super didChangeSession];
	
	TerminalScreenRef	screen = [self terminalScreen];
	
	
	if (nullptr != screen)
	{
		[self setStateFromScreen:screen];
		Terminal_StartMonitoring(screen, kTerminal_ChangeAudioState, self.screenChangeListener.listenerRef);
	}
}// didChangeSession


@end //} TerminalToolbar_ItemBell


#pragma mark -
@implementation TerminalToolbar_ItemBell (TerminalToolbar_ItemBellInternal) //{


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored screen event occurs.  See
"didChangeSession" for the set of events that is monitored.

(4.0)
*/
- (void)
model:(ListenerModel_Ref)				aModel
screenChange:(ListenerModel_Event)		anEvent
context:(void*)							aContext
{
#pragma unused(aModel)
	switch (anEvent)
	{
	case kTerminal_ChangeAudioState:
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(aContext, TerminalScreenRef);
			
			
			[self setStateFromScreen:screen];
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"bell toolbar item: terminal change handler received unexpected event", anEvent);
		break;
	}
}// model:screenChange:context:


#pragma mark New Methods


/*!
Updates the toolbar icon based on the current bell state
of the given screen buffer.  If "nullptr" is given then
the icon is reset.

The icon is based on the effect the item will have when
used, NOT the current state of the bell.  So for
instance, a bell-off icon means “clicking this will
turn off the bell and the bell is currently on”.

(4.0)
*/
- (void)
setStateFromScreen:(TerminalScreenRef)		aScreen
{
	if ((nullptr == aScreen) || Terminal_BellIsEnabled(aScreen))
	{
		self.toolTip = NSLocalizedString(@"Turn off terminal bell", @"toolbar item tooltip; turn off terminal bell sound");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"speaker.slash" accessibilityDescription:self.toolTip];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnBellOffIconFilenameNoExtension(), NSString*)];
		}
	}
	else
	{
		self.toolTip = NSLocalizedString(@"Turn on terminal bell", @"toolbar item tooltip; turn on terminal bell sound");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"speaker.wave.3" accessibilityDescription:self.toolTip];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnBellOnIconFilenameNoExtension(), NSString*)];
		}
	}
}// setStateFromScreen:


@end //} TerminalToolbar_ItemBell (TerminalToolbar_ItemBellInternal)


#pragma mark -
@implementation TerminalToolbar_ItemCustomize //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kTerminalToolbar_ItemIDCustomize];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Customize", @"toolbar item name; for customizing the toolbar");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"wrench.and.screwdriver" accessibilityDescription:self.toolTip];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnCustomizeToolbarIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(runToolbarCustomizationPalette:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


#pragma mark NSToolbarItem


/*!
Since Apple’s fancy Full Screen toolbar mechanism breaks
some fundamental assumptions about toolbars (including
that a view in an item continues to belong to its original
window instead of some special class of window), the
“Customize” action is disabled entirely while in Full Screen.

(2018.03)
*/
- (void)
validate
{
	self.enabled = ([NSApp keyWindow].toolbar.allowsUserCustomization || [NSApp mainWindow].toolbar.allowsUserCustomization);
}// validate


@end //} TerminalToolbar_ItemCustomize


#pragma mark -
@implementation TerminalToolbar_ItemForceQuit //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDForceQuit];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		_sessionChangeListener = [[ListenerModel_StandardListener alloc]
									initWithTarget:self
													eventFiredSelector:@selector(model:sessionChange:context:)];
		
		[self setStateFromSession:nullptr];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self willChangeSession]; // automatically stop monitoring the screen
}// dealloc


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	SessionRef		targetSession = [self session];
	
	
	if (Session_IsValid(targetSession) && Session_StateIsDead(targetSession))
	{
		if (false == Commands_ViaFirstResponderPerformSelector(@selector(performRestart:), sender))
		{
			Sound_StandardAlert();
		}
	}
	else
	{
		if (false == Commands_ViaFirstResponderPerformSelector(@selector(performKill:), sender))
		{
			Sound_StandardAlert();
		}
	}
}// performToolbarItemAction:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id								result = [super copyWithZone:zone];
	TerminalToolbar_ItemForceQuit*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemForceQuit.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemForceQuit*);
	asSelf.sessionChangeListener = [self.sessionChangeListener copy];
	asSelf.sessionChangeListener.target = asSelf;
	
	return result;
}// copyWithZone:


#pragma mark TerminalToolbar_SessionDependentItem


/*!
Invoked by the superclass whenever the target session of
the owning toolbar is about to change.

Responds by removing monitors on the previous session.

(4.0)
*/
- (void)
willChangeSession
{
	[super willChangeSession];
	
	SessionRef	session = self.session;
	
	
	if (Session_IsValid(session))
	{
		Session_StopMonitoring(session, kSession_ChangeState, self.sessionChangeListener.listenerRef);
	}
	[self setStateFromSession:nullptr];
}// willChangeSession


/*!
Invoked by the superclass whenever the target session of
the owning toolbar has changed.

Responds by updating the icon state.

(4.0)
*/
- (void)
didChangeSession
{
	[super didChangeSession];
	
	SessionRef	session = self.session;
	
	
	[self setStateFromSession:session];
	if (Session_IsValid(session))
	{
		Session_StartMonitoring(session, kSession_ChangeState, self.sessionChangeListener.listenerRef);
	}
}// didChangeSession


@end //} TerminalToolbar_ItemForceQuit


#pragma mark -
@implementation TerminalToolbar_ItemForceQuit (TerminalToolbar_ItemForceQuitInternal) //{


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored session event occurs.  See
"didChangeSession" for the set of events that is monitored.

(4.0)
*/
- (void)
model:(ListenerModel_Ref)				aModel
sessionChange:(ListenerModel_Event)		anEvent
context:(void*)							aContext
{
#pragma unused(aModel)
	switch (anEvent)
	{
	case kSession_ChangeState:
		{
			SessionRef	session = REINTERPRET_CAST(aContext, SessionRef);
			
			
			[self setStateFromSession:session];
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"force-quit toolbar item: session change handler received unexpected event", anEvent);
		break;
	}
}// model:sessionChange:context:


#pragma mark New Methods


/*!
Updates the toolbar icon based on the current state of the
given session.  If "nullptr" is given then the icon is reset.

(4.0)
*/
- (void)
setStateFromSession:(SessionRef)	aSession
{
	if (Session_IsValid(aSession) && Session_StateIsDead(aSession))
	{
		self.label = NSLocalizedString(@"Restart", @"toolbar item name; for killing or restarting the active process");
		self.paletteLabel = self.label;
		self.toolTip = NSLocalizedString(@"Restart process (with original command line)", @"toolbar item tooltip; restart session");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"arrow.rectanglepath" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnRestartSessionIconFilenameNoExtension(), NSString*)];
		}
	}
	else
	{
		self.label = NSLocalizedString(@"Force Quit", @"toolbar item name; for killing or restarting the active process");
		self.paletteLabel = self.label;
		self.toolTip = NSLocalizedString(@"Force process to quit", @"toolbar item tooltip; force-quit session");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"xmark.rectangle" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnKillSessionIconFilenameNoExtension(), NSString*)];
		}
	}
}// setStateFromSession:


@end //} TerminalToolbar_ItemForceQuit (TerminalToolbar_ItemForceQuitInternal)


#pragma mark -
@implementation TerminalToolbar_ItemFullScreen //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDFullScreen];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Full Screen", @"toolbar item name; for entering or exiting Full Screen mode");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"arrow.up.left.and.arrow.down.right" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnFullScreenIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	[NSApp sendAction:@selector(toggleFullScreen:) to:[NSApp mainWindow] from:sender];
}// performToolbarItemAction:


#pragma mark NSToolbarItem


/*!
Returns YES only if the item should be enabled.

(4.1)
*/
- (BOOL)
validateToolbarItem:(NSToolbarItem*)	anItem
{
#pragma unused(anItem)
	BOOL	result = YES;
	
	
	return result;
}// validateToolbarItem:


@end //} TerminalToolbar_ItemFullScreen


#pragma mark -
@implementation TerminalToolbar_ItemHide //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDHide];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Hide", @"toolbar item name; for hiding the frontmost window");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"eye.slash" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnHideWindowIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performHideWindow:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemHide


#pragma mark -
@implementation TerminalToolbar_ItemLED1 //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED1 oneBasedIndexOfLED:1];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"L1", @"toolbar item name; for terminal LED #1");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performTerminalLED1Toggle:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemLED1


#pragma mark -
@implementation TerminalToolbar_ItemLED2 //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED2 oneBasedIndexOfLED:2];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"L2", @"toolbar item name; for terminal LED #2");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performTerminalLED2Toggle:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemLED2


#pragma mark -
@implementation TerminalToolbar_ItemLED3 //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED3 oneBasedIndexOfLED:3];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"L3", @"toolbar item name; for terminal LED #3");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performTerminalLED3Toggle:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemLED3


#pragma mark -
@implementation TerminalToolbar_ItemLED4 //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED4 oneBasedIndexOfLED:4];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"L4", @"toolbar item name; for terminal LED #4");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performTerminalLED4Toggle:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemLED4


#pragma mark -
@implementation TerminalToolbar_ItemMacro //{


@synthesize tag = _tag;


#pragma mark Initializers


/*!
Designated initializer.

(2020.05)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anItemIdentifier
{
	self = [super initWithItemIdentifier:anItemIdentifier];
	if (nil != self)
	{
		NSButton*				actionButton = nil;
		NSBox*					borderView = nil;
		NSLayoutConstraint*		newConstraint = nil; // reused below
		
		
		actionButton = [NSButton buttonWithTitle:@"" target:nil action:@selector(performActionForMacro:)];
		actionButton.buttonType = NSButtonTypeMomentaryChange;
		//actionButton.bordered = YES;
		//actionButton.showsBorderOnlyWhileMouseInside = YES;
		actionButton.bordered = NO;
		actionButton.lineBreakMode = NSLineBreakByTruncatingTail;
		actionButton.translatesAutoresizingMaskIntoConstraints = NO;
		
		borderView = [[NSBox alloc] initWithFrame:NSZeroRect];
		borderView.boxType = NSBoxCustom;
		borderView.borderType = NSLineBorder;
		borderView.borderColor = [NSColor clearColor]; // (changes if customizing)
		borderView.borderWidth = 1.0;
		borderView.contentView = actionButton; // retain
		borderView.translatesAutoresizingMaskIntoConstraints = NO;
		
		// set some constant constraints (others are defined by
		// "updateSizeConstraints"); constraints MUST be archived
		// because toolbar items copy from customization palette
		newConstraint = [actionButton.leadingAnchor constraintEqualToAnchor:borderView.leadingAnchor constant:4.0/* arbitrary */];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		newConstraint = [borderView.trailingAnchor constraintEqualToAnchor:actionButton.trailingAnchor constant:4.0/* arbitrary */];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		newConstraint = [actionButton.topAnchor constraintEqualToAnchor:borderView.topAnchor constant:2.0/* arbitrary */];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		newConstraint = [actionButton.bottomAnchor constraintEqualToAnchor:borderView.bottomAnchor];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		self.view = borderView; // retain
		[self updateSizeConstraints];
		
		self.menuFormRepresentation = [[NSMenuItem alloc] initWithTitle:@"" action:actionButton.action keyEquivalent:@""];
		
		// IMPORTANT: command implementation of "performActionForMacro:"
		// depends on being able to read the "tag" to determine the
		// one-based index of the macro to invoke for this item (as is
		// true for macro menu items with a "tag" and the same action)
		self.action = actionButton.action;
		self.target = actionButton.target;
		self.enabled = YES; // see "validate"
		// see "setTag:", which sets an icon and label (the tag MUST be
		// set before the item can be used)
		[self setTagUnconditionally:0];
		
		// monitor changes to macros so that the toolbar item can be updated
		// (for example, the user renaming the macro); this will also call
		// the "eventFiredSelector:" immediately to initialize
		_macroManagerChangeListener = [[ListenerModel_StandardListener alloc]
										initWithTarget:self
														eventFiredSelector:@selector(model:macroManagerChange:context:)];
		{
			MacroManager_Result		monitorResult = kMacroManager_ResultOK;
			
			
			monitorResult = MacroManager_StartMonitoring(kMacroManager_ChangeMacroSetFrom, self.macroManagerChangeListener.listenerRef);
			if (kMacroManager_ResultOK != monitorResult)
			{
				Console_Warning(Console_WriteValue, "macro toolbar item failed to monitor macro manager, error", monitorResult.code());
			}
			monitorResult = MacroManager_StartMonitoring(kMacroManager_ChangeMacroSetTo, self.macroManagerChangeListener.listenerRef);
			if (kMacroManager_ResultOK != monitorResult)
			{
				Console_Warning(Console_WriteValue, "macro toolbar item failed to monitor macro manager, error", monitorResult.code());
			}
		}
		_preferenceChangeListener = [[ListenerModel_StandardListener alloc]
										initWithTarget:self
														eventFiredSelector:@selector(model:preferenceChange:context:)];
		
		assert(actionButton == [self actionButton]); // test accessor
		assert(borderView == [self containerViewAsBox]); // test accessor
	}
	return self;
}// initWithItemIdentifier:


/*!
Destructor.

(2020.05)
*/
- (void)
dealloc
{
	UNUSED_RETURN(MacroManager_Result)MacroManager_StopMonitoring(kMacroManager_ChangeMacroSetFrom, self.macroManagerChangeListener.listenerRef);
	UNUSED_RETURN(MacroManager_Result)MacroManager_StopMonitoring(kMacroManager_ChangeMacroSetTo, self.macroManagerChangeListener.listenerRef);
	{
		std::vector< Preferences_Tag >		monitoredTags =
											{
												kPreferences_TagIndexedMacroAction,
												kPreferences_TagIndexedMacroContents,
												kPreferences_TagIndexedMacroName,
												kPreferences_TagIndexedMacroKey,
												kPreferences_TagIndexedMacroKeyModifiers
											};
		
		
		if (Preferences_ContextIsValid(MacroManager_ReturnCurrentMacros()) && (self.tag > 0))
		{
			for (auto aTag : monitoredTags)
			{
				Preferences_ContextStopMonitoring(MacroManager_ReturnCurrentMacros(), self.preferenceChangeListener.listenerRef,
													Preferences_ReturnTagVariantForIndex(aTag, STATIC_CAST(self.tag, Preferences_Index)));
			}
		}
	}
}// dealloc


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2020.05)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id								result = [super copyWithZone:zone];
	TerminalToolbar_ItemMacro*		asSelf = nil;
	NSError* /*__autoreleasing*/		error = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemMacro.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemMacro*);
	
	asSelf.preferenceChangeListener = [self.preferenceChangeListener copy];
	asSelf.preferenceChangeListener.target = asSelf;
	
	// views do not support NSCopying; archive instead (also, be sure that
	// any NSLayoutConstraint instances have "shouldBeArchived = YES")
	//asSelf.view = [self.view copy];
	NSData*		archivedView = [NSKeyedArchiver archivedDataWithRootObject:self.view requiringSecureCoding:NO error:&error];
	if (nil != error)
	{
		Console_Warning(Console_WriteValueCFString, "failed to copy toolbar item border, archiving error", BRIDGE_CAST(error.localizedDescription, CFStringRef));
	}
	asSelf.view = [NSKeyedUnarchiver unarchivedObjectOfClass:NSBox.class fromData:archivedView error:&error];
	if (nil != error)
	{
		Console_Warning(Console_WriteValueCFString, "failed to copy toolbar item border, unarchiving error", BRIDGE_CAST(error.localizedDescription, CFStringRef));
	}
	
	return result;
}// copyWithZone:


#pragma mark NSToolbarItem


/*!
Index of macro that this item represents (in the current set).

(2020.05)
*/
- (NSInteger)
tag
{
	// NOTE: NSToolbarItem "setTag:" does not work (it sets values to -1!!!)
	// so this is being overridden with a local property
	//return [super tag];
	return _tag;
}
- (void)
setTag:(NSInteger)	aTag
{
	if (_tag != aTag)
	{
		[self setTagUnconditionally:aTag];
	}
}// setTag:


/*!
Validates the button (in this case, by disabling the button
if the target macro cannot be used right now).

(2020.05)
*/
- (void)
validate
{
	NSButton*	actionButton = [self actionButton];
	
	
	if (0 == self.tag)
	{
		// invalid
		self.enabled = NO;
	}
	else
	{
		self.enabled = [[Commands_Executor sharedExecutor] validateAction:@selector(performActionForMacro:) sender:[self actionButton] sourceItem:self];
	}
	//self.menuFormRepresentation.enabled = self.enabled;
	actionButton.enabled = self.enabled;
	
	// note: once validation occurs, "canPerformActionForMacro:" will have
	// updated the menu item representation
	self.menuFormRepresentation.keyEquivalent = @""; // do not override keys displayed in main menu bar items
	
	[self setStateForToolbar:self.toolbar];
}// validate


#pragma mark TerminalToolbar_DisplayModeSensitive


/*!
Keeps track of changes to the mode (such as “text only”)
so that the window layout can adapt if necessary.

(2020.05)
*/
- (void)
didChangeDisplayModeForToolbar:(NSToolbar*)		aToolbar
{
#pragma unused(aToolbar)
	[self updateSizeConstraints];
	self.view.needsDisplay = YES;
}// didChangeDisplayModeForToolbar:


#pragma mark TerminalToolbar_ItemAddRemoveSensitive


/*!
Called when the given item is about to be added to the
specified toolbar.

(2020.05)
*/
- (void)
item:(NSToolbarItem*)			anItem
willEnterToolbar:(NSToolbar*)	aToolbar
{
	assert(self == anItem);
	[self actionButton].controlSize = (((nil != aToolbar) &&
										(NSToolbarSizeModeSmall == aToolbar.sizeMode))
										? NSControlSizeSmall
										: NSControlSizeRegular);
	[self setStateForToolbar:aToolbar];
	[self updateSizeConstraints];
}// item:willEnterToolbar:


/*!
Called when the specified item has been removed from
the specified toolbar.

(2020.05)
*/
- (void)
item:(NSToolbarItem*)			anItem
didExitToolbar:(NSToolbar*)		aToolbar
{
#pragma unused(aToolbar)
	assert(self == anItem);
}// item:didExitToolbar:


#pragma mark TerminalToolbar_ItemHasPaletteProxy


/*!
Returns the item that represents this item in a customization palette.

(2020.05)
*/
- (NSToolbarItem*)
paletteProxyToolbarItemWithIdentifier:(NSString*)	anIdentifier
{
	NSToolbarItem*		result = [[NSToolbarItem alloc] initWithItemIdentifier:anIdentifier];
	
	
	result.paletteLabel = self.paletteLabel;
	if (@available(macOS 11.0, *))
	{
		result.image = [NSImage imageWithSystemSymbolName:@"command" accessibilityDescription:self.label];
	}
	else
	{
		result.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnPrefPanelMacrosIconFilenameNoExtension(), NSString*)];
	}
	
	return result;
}// paletteProxyToolbarItemWithIdentifier:


#pragma mark TerminalToolbar_SizeSensitive


/*!
Keeps track of changes to the icon and label size
so that the window layout can adapt if necessary.

(2020.05)
*/
- (void)
didChangeSizeForToolbar:(NSToolbar*)	aToolbar
{
	[self actionButton].controlSize = (((nil != aToolbar) &&
										(NSToolbarSizeModeSmall == aToolbar.sizeMode))
										? NSControlSizeSmall
										: NSControlSizeRegular);
	[self setStateForToolbar:aToolbar];
	[self updateSizeConstraints];
	self.view.needsDisplay = YES;
}// didChangeSizeForToolbar:


@end //} TerminalToolbar_ItemMacro


#pragma mark -
@implementation TerminalToolbar_ItemMacro (TerminalToolbar_ItemMacroInternal) //{


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored preference changes.  See the
initializer for the set of events that is monitored.

(2020.05)
*/
- (void)
model:(ListenerModel_Ref)					aModel
macroManagerChange:(ListenerModel_Event)	inMacroManagerChange
context:(void*)								inPreferencesContext
{
#pragma unused(aModel)
	Preferences_ContextRef		prefsContext = REINTERPRET_CAST(inPreferencesContext, Preferences_ContextRef);
	
	
	if (kMacroManager_ChangeMacroSetFrom == inMacroManagerChange)
	{
		std::vector< Preferences_Tag >		monitoredTags =
											{
												kPreferences_TagIndexedMacroAction,
												kPreferences_TagIndexedMacroContents,
												kPreferences_TagIndexedMacroName,
												kPreferences_TagIndexedMacroKey,
												kPreferences_TagIndexedMacroKeyModifiers
											};
		
		
		if (Preferences_ContextIsValid(prefsContext) && (self.tag > 0))
		{
			for (auto aTag : monitoredTags)
			{
				Preferences_ContextStopMonitoring(prefsContext, self.preferenceChangeListener.listenerRef,
													Preferences_ReturnTagVariantForIndex(aTag, STATIC_CAST(self.tag, Preferences_Index)));
			}
		}
		//[self updateWithPreferenceValuesFromMacroSet:prefsContext]; // (update occurs in "kMacroManager_ChangeMacroSetTo" case)
	}
	else if (kMacroManager_ChangeMacroSetTo == inMacroManagerChange)
	{
		std::vector< Preferences_Tag >		monitoredTags =
											{
												kPreferences_TagIndexedMacroAction,
												kPreferences_TagIndexedMacroContents,
												kPreferences_TagIndexedMacroName,
												kPreferences_TagIndexedMacroKey,
												kPreferences_TagIndexedMacroKeyModifiers
											};
		
		
		if (Preferences_ContextIsValid(prefsContext) && (self.tag > 0))
		{
			for (auto aTag : monitoredTags)
			{
				Preferences_ContextStartMonitoring(prefsContext, self.preferenceChangeListener.listenerRef,
													Preferences_ReturnTagVariantForIndex(aTag, STATIC_CAST(self.tag, Preferences_Index)));
			}
		}
		
		[self updateWithPreferenceValuesFromMacroSet:prefsContext]; // update even for nullptr context (“None” is possible user selection)
	}
	else
	{
		// ???
		Console_Warning(Console_WriteValueFourChars, "macro toolbar item callback was invoked for unrecognized macro manager change", inMacroManagerChange);
	}
}// model:macroManagerChange:context:


/*!
Called when a monitored preference changes.  See the
initializer for the set of events that is monitored.

(2020.05)
*/
- (void)
model:(ListenerModel_Ref)				aModel
preferenceChange:(ListenerModel_Event)	inPreferenceTagThatChanged
context:(void*)							inPreferencesContext
{
#pragma unused(aModel)
	Preferences_ContextRef		prefsContext = REINTERPRET_CAST(inPreferencesContext, Preferences_ContextRef);
	
	
	if (nullptr == prefsContext)
	{
		Console_Warning(Console_WriteLine, "macro toolbar item preference-change callback was invoked for nonexistent macro set");
	}
	else
	{
		Preferences_Tag const		kTagWithoutIndex = Preferences_ReturnTagFromVariant(inPreferenceTagThatChanged);
		//Preferences_Index const		kIndexFromTag = Preferences_ReturnTagIndex(inPreferenceTagThatChanged);
		
		
		switch (kTagWithoutIndex)
		{
		case kPreferences_TagIndexedMacroAction:
		case kPreferences_TagIndexedMacroContents:
		case kPreferences_TagIndexedMacroName:
		case kPreferences_TagIndexedMacroKey:		
		case kPreferences_TagIndexedMacroKeyModifiers:
			// user has changed preference in some significant way (such as
			// renaming a macro); update the toolbar item
			[self updateWithPreferenceValuesFromMacroSet:prefsContext];
			break;
		
		default:
			// ???
			break;
		}
	}
}// model:preferenceChange:context:


#pragma mark New Methods


/*!
Return the button used for sending actions to targets.

(2020.05)
*/
- (NSButton*)
actionButton
{
	NSButton*	result = nil;
	
	
	for (NSView* aView in self.view.subviews)
	{
		if ([aView isKindOfClass:NSButton.class])
		{
			result = STATIC_CAST(aView, NSButton*);
			break;
		}
	}
	
	if (nil == result)
	{
		//Console_Warning(Console_WriteLine, "macro toolbar item action button is not defined!"); // debug
	}
	
	return result;
}// actionButton


/*!
Update and return the ideal size for this item (fitting
the title).  A minimum size is enforced.

(2020.05)
*/
- (NSSize)
calculateIdealSize
{
	NSBox*			containerBox = [self containerViewAsBox];
	NSButton*		actionButton = [self actionButton];
	CGFloat const	idealWidth = ceil(actionButton.intrinsicContentSize.width + (2.0 * containerBox.contentViewMargins.width) + (2.0 * containerBox.borderWidth));
	CGFloat const	idealHeight = ceil(actionButton.intrinsicContentSize.height + (2.0 * containerBox.contentViewMargins.height) + (2.0 * containerBox.borderWidth));
	NSSize			result = NSMakeSize(std::max<CGFloat>(24, idealWidth), idealHeight);
	
	
	return result;
}// calculateIdealSize


/*!
Return the main item view as an NSBox class for convenience.

(2020.05)
*/
- (NSBox*)
containerViewAsBox
{
	NSBox*	result = nil;
	
	
	if ([self.view isKindOfClass:NSBox.class])
	{
		result = STATIC_CAST(self.view, NSBox*);
	}
	
	if (nil == result)
	{
		Console_Warning(Console_WriteLine, "macro toolbar item container view is not NSBox type!");
	}
	
	return result;
}// containerViewAsBox


/*!
Synchronizes other properties of the toolbar item with
the properties of its embedded text view, preparing for
display in the given toolbar (which may or may not be
the item’s current toolbar).

(2020.05)
*/
- (void)
setStateForToolbar:(NSToolbar*)		aToolbar
{
	if (aToolbar.customizationPaletteIsRunning)
	{
		[self containerViewAsBox].borderColor = [NSColor lightGrayColor]; // color attempts to match “space item” style
	}
	else
	{
		[self containerViewAsBox].borderColor = [NSColor clearColor];
	}
}// setStateForToolbar:


/*!
Helper function for "setTag:" with the option to force a
value even if the given tag matches the current tag.

This is useful if the current macro preferences context
may have changed (even if the tag did not), or when
initializing. 

(2020.05)
*/
- (void)
setTagUnconditionally:(NSInteger)	aTag
{
	NSButton*	actionButton = [self actionButton];
	
	
	// NOTE: NSToolbarItem "setTag:" does not work (it sets values to -1!!!)
	// so this is being overridden with a local property
	//[super setTag:aTag];
	_tag = aTag;
	
	//self.label = [NSString stringWithFormat:@"%d", (int)aTag]; // arbitrary (INCOMPLETE; use macro name from preference)
	self.label = @""; // view is used instead
	
	actionButton.tag = aTag; // this is critical; it is used by validations and action handlers to determine which macro applies
	//actionButton.title = // set by "updateWithPreferenceValuesFromMacroSet:"
	//actionButton.image = nil; // set by "updateWithPreferenceValuesFromMacroSet:"
	[self updateWithPreferenceValuesFromMacroSet:MacroManager_ReturnCurrentMacros()]; // sets "actionButton.title"
	
	self.paletteLabel = [NSString stringWithFormat:NSLocalizedString(@"Macro %1$d", @"name for macro icon in toolbar customization palette; %1$d will be a macro number from 1 to 12"), (int)aTag];
	self.toolTip = self.paletteLabel; // initial value (overridden if macro name is set)
	
	[self updateSizeConstraints];
}// setTagUnconditionally:


/*!
Specify minimum and maximum width and height, as well as
priorities for content hugging and compression resistance.

(2020.12)
*/
- (void)
updateSizeConstraints
{
	NSSize		idealSize = [self calculateIdealSize];
	
	
	if (nil != self.heightConstraint)
	{
		self.heightConstraint.active = NO;
		[self.view removeConstraint:self.heightConstraint];
	}
	if (nil != self.widthConstraintMax)
	{
		self.widthConstraintMax.active = NO;
		[self.view removeConstraint:self.widthConstraintMax];
	}
	if (nil != self.widthConstraintMin)
	{
		self.widthConstraintMin.active = NO;
		[self.view removeConstraint:self.widthConstraintMin];
	}
	
	// constraints MUST be archived because toolbar items are
	// copied from the customization palette (via archiving)
	self.heightConstraint = [self.view.heightAnchor constraintEqualToConstant:idealSize.height];
	self.heightConstraint.active = YES;
	self.heightConstraint.shouldBeArchived = YES;
	self.widthConstraintMax = [self.view.widthAnchor constraintLessThanOrEqualToConstant:std::max<CGFloat>(24, idealSize.width)];
	self.widthConstraintMax.active = YES;
	self.widthConstraintMax.shouldBeArchived = YES;
	self.widthConstraintMin = [self.view.widthAnchor constraintGreaterThanOrEqualToConstant:std::max<CGFloat>(24, idealSize.width)];
	self.widthConstraintMin.active = YES;
	self.widthConstraintMin.shouldBeArchived = YES;
	
	[self.view setContentHuggingPriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationHorizontal];
	[self.view setContentHuggingPriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationVertical];
	[self.view setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationHorizontal];
	[self.view setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationVertical];
}// updateSizeConstraints


/*!
Updates the toolbar item to reflect the macro at this item’s
assigned index in the current macro set.

(2020.05)
*/
- (void)
updateWithPreferenceValuesFromMacroSet:(Preferences_ContextRef)		aMacroSet
{
	NSButton*	actionButton = [self actionButton];
	NSString*	genericMacroName = [NSString stringWithFormat:NSLocalizedString(@"Macro %1$d",
																					@"macro unnamed toolbar item description; %1$d will be a macro number from 1 to 12"),
																					(int)self.tag];
	BOOL		usePlainAction = ((nullptr == aMacroSet) || (0 == self.tag));
	
	
	[NSAnimationContext beginGrouping];
	[NSAnimationContext currentContext].duration = 0.12;
	
	if (NO == usePlainAction)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		CFStringRef			macroName = nullptr;
		
		
		// use the name of the macro as the button title
		prefsResult = Preferences_ContextGetData(aMacroSet,
													Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, STATIC_CAST(self.tag, Preferences_Index)),
													sizeof(macroName), &macroName, false/* search defaults */);
		if (kPreferences_ResultOK != prefsResult)
		{
			//Console_Warning(Console_WriteValue, "macro toolbar item failed to update macro name, error", prefsResult);
			//Console_Warning(Console_WriteValue, "macro toolbar item failed to update, index", self.tag);
			//Console_Warning(Console_WriteValueFourChars, "macro toolbar item failed to update, tag", Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedMacroName, STATIC_CAST(self.tag, Preferences_Index)));
			usePlainAction = YES;
		}
		else if (0 == CFStringGetLength(macroName))
		{
			usePlainAction = YES;
		}
		else
		{
			NSString*	newTitle = BRIDGE_CAST(macroName, NSString*);
			
			
			// animator performance does not seem to be good; avoid unless
			// the value is actually different
			if (nil != actionButton.image)
			{
				[actionButton animator].image = nil;
				actionButton.imagePosition = NSNoImage;
			}
			
			if (NO == [actionButton.title isEqualToString:newTitle])
			{
				[actionButton animator].title = newTitle;
				self.toolTip = [NSString stringWithFormat:NSLocalizedString(@"Macro %1$d: “%2$@”", @"macro toolbar item tooltip; %1$d will be a macro number from 1 to 12, quoted string will be macro name"), (int)self.tag, newTitle];
			}
		}
		// (INCOMPLETE; could use other properties of macros here, perhaps)
	}
	
	if (usePlainAction)
	{
		// set an arbitrary default title when there is no active macro
		// (or in initial state of unassigned macro index)
		NSImage*	newImage = nil;
		
		
		if (@available(macOS 11.0, *))
		{
			NSString*	symbolName = [NSString stringWithFormat:@"%d.square", (int)self.tag]; // e.g. "1.square", "2.square", ...
			
			
			newImage = [NSImage imageWithSystemSymbolName:symbolName accessibilityDescription:genericMacroName];
		}
		else
		{
			newImage = [NSImage imageNamed:NSImageNameActionTemplate];
		}
		
		// for plain macros (with no distinct name), show the macro number;
		// this can be shown as image-only or as image with text below but
		// the toolbars on Big Sur make it harder to fit text underneath 
	#if 1
		// animator performance does not seem to be good; avoid unless
		// the value is actually different
		if (newImage != actionButton.image)
		{
			[actionButton animator].image = newImage;
			actionButton.imagePosition = NSImageOnly;
		}
		
		[actionButton animator].title = @"";
		self.toolTip = genericMacroName;
	#else
		// animator performance does not seem to be good; avoid unless
		// the value is actually different
		if (newImage != actionButton.image)
		{
			[actionButton animator].image = newImage;
			actionButton.imagePosition = NSImageAbove;
		}
		
		NSString*	newTitle = [NSString stringWithFormat:@"%d", (int)self.tag];
		if (NO == [actionButton.title isEqualToString:newTitle])
		{
			[actionButton animator].title = newTitle;
			self.toolTip = genericMacroName;
		}
	#endif
	}
	
	[self setStateForToolbar:self.toolbar];
	[self updateSizeConstraints];
	
	[NSAnimationContext endGrouping];
	
	// note: title and enabled state of "menuFormRepresentation" is set
	// by the toolbar item validation process (see "validate")
	self.menuFormRepresentation.target = actionButton.target;
	self.menuFormRepresentation.tag = self.tag; // used to identify macro
	
	// it is possible that resizing an item will reveal more items
	// than before, requiring the extra items to be revalidated
	[self.toolbar validateVisibleItems];
}// updateWithCurrentMacroSetPreferenceValue:


@end //} TerminalToolbar_ItemMacroInternal


#pragma mark -
@implementation TerminalToolbar_ItemNewSessionDefaultFavorite //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kTerminalToolbar_ItemIDNewSessionDefaultFavorite];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Default", @"toolbar item name; for opening Default session");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"heart" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnNewSessionDefaultIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	BOOL	didPerform = [NSApp tryToPerform:@selector(performNewDefault:) with:sender];
	
	
	if (NO == didPerform)
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemNewSessionDefaultFavorite


#pragma mark -
@implementation TerminalToolbar_ItemNewSessionLogInShell //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kTerminalToolbar_ItemIDNewSessionLogInShell];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Log-In Shell", @"toolbar item name; for opening log-in shells");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"terminal" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnNewSessionLogInShellIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	BOOL	didPerform = [NSApp tryToPerform:@selector(performNewLogInShell:) with:sender];
	
	
	if (NO == didPerform)
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemNewSessionLogInShell


#pragma mark -
@implementation TerminalToolbar_ItemNewSessionShell //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kTerminalToolbar_ItemIDNewSessionShell];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Shell", @"toolbar item name; for opening shells");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"terminal.fill" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnNewSessionShellIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	BOOL	didPerform = [NSApp tryToPerform:@selector(performNewShell:) with:sender];
	
	
	if (NO == didPerform)
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemNewSessionShell


#pragma mark -
@implementation TerminalToolbar_ItemNotifyOnActivity //{


@synthesize bordered = _bordered;


#pragma mark Initializers


/*!
Designated initializer.

(2021.02)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDNotifications];
	if (nil != self)
	{
		_bordered = NO;
		_sessionChangeListener = [[ListenerModel_StandardListener alloc]
									initWithTarget:self
													eventFiredSelector:@selector(model:sessionChange:context:)];
		
		// populate menu with same items as those in menu bar
		{
			NSMenu*			newMenu = [[NSMenu alloc] initWithTitle:@""];
			NSMenuItem*		newItem = nil; // reused below
			
			
			newItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"None", "toolbar menu item for controlling disabling of notifications")
														action:@selector(performSetActivityHandlerNone:)
														keyEquivalent:@""];
			newItem.target = nil; // use first responder
			// child items do not need an icon (the parent does,
			// and currently parent and child have the same icon)
			//newItem.image = ...;
			//newItem.image.size = NSMakeSize(24, 24); // shrink default image, which is too large
			[newMenu addItem:newItem];
			newItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Notify on Next Activity", "toolbar menu item for controlling notification when new data arrives")
														action:@selector(performSetActivityHandlerNotifyOnNext:)
														keyEquivalent:@""];
			newItem.target = nil; // use first responder
			// child items do not need an icon (the parent does,
			// and currently parent and child have the same icon)
			//newItem.image = ...;
			//newItem.image.size = NSMakeSize(24, 24); // shrink default image, which is too large
			[newMenu addItem:newItem];
			newItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Notify on Inactivity (After Delay)", "toolbar menu item for controlling notification when session becomes idle")
														action:@selector(performSetActivityHandlerNotifyOnIdle:)
														keyEquivalent:@""];
			newItem.target = nil; // use first responder
			// child items do not need an icon (the parent does,
			// and currently parent and child have the same icon)
			//newItem.image = ...;
			//newItem.image.size = NSMakeSize(24, 24); // shrink default image, which is too large
			[newMenu addItem:newItem];
			newItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"Send Keep-Alive on Inactivity", "toolbar menu item for controlling keep-alive when session becomes idle")
														action:@selector(performSetActivityHandlerSendKeepAliveOnIdle:)
														keyEquivalent:@""];
			newItem.target = nil; // use first responder
			// child items do not need an icon (the parent does,
			// and currently parent and child have the same icon)
			//newItem.image = ...;
			//newItem.image.size = NSMakeSize(24, 24); // shrink default image, which is too large
			[newMenu addItem:newItem];
			
			// menu toolbar item settings
			self.menu = newMenu;
			self.showsIndicator = YES;
		}
		
		// basic toolbar item settings
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		//self.enabled = YES; // see "setStateFromSession:"
		self.label = NSLocalizedString(@"Notifications", @"toolbar item name; for selecting a notification mode");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		
		// set image and enabled state
		[self setStateFromSession:nullptr];
	}
	return self;
}// init


/*!
Destructor.

(2021.02)
*/
- (void)
dealloc
{
	[self willChangeSession]; // automatically stop monitoring the screen
}// dealloc


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(2021.02)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	SessionRef		targetSession = [self session];
	
	
	if (Session_IsValid(targetSession))
	{
		// arbitrary; currently this is a toggle (notify on activity, or not)
		// even though more types are possible; the user must select others
		// from the menu but the button acts as a short-cut for some of them
		if (Session_WatchIsOff(targetSession))
		{
			// arbitrarily enable activity notifications (most common type?);
			// user can still pick a different type from the menu
			Session_SetWatch(targetSession, kSession_WatchForPassiveData);
		}
		else
		{
			// turn off notifications
			Session_SetWatch(targetSession, kSession_WatchNothing);
		}
	}
	else
	{
		// should not happen (as item should be disabled when validated)
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2021.02)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id										result = [super copyWithZone:zone];
	TerminalToolbar_ItemNotifyOnActivity*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemNotifyOnActivity.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemNotifyOnActivity*);
	asSelf.bordered = self.bordered;
	asSelf.sessionChangeListener = [self.sessionChangeListener copy];
	asSelf.sessionChangeListener.target = asSelf;
	[self setStateFromSession:self.session];
	
	return result;
}// copyWithZone:


#pragma mark NSToolbarItem


// (See description in class interface.)
- (BOOL)
isBordered
{
	return _bordered;
}
- (void)
setBordered:(BOOL)		aFlag
{
	_bordered = aFlag;
}// setBordered:


#pragma mark TerminalToolbar_ItemAddRemoveSensitive


/*!
Called when the given item is about to be added to the
specified toolbar.

(2021.02)
*/
- (void)
item:(NSToolbarItem*)			anItem
willEnterToolbar:(NSToolbar*)	aToolbar
{
	[super item:anItem willEnterToolbar:aToolbar];
	[self session:[aToolbar terminalToolbarDelegate].session setAsCurrentBinding:YES];
	[self setStateFromSession:[aToolbar terminalToolbarDelegate].session];
}// item:willEnterToolbar:


/*!
Called when the specified item has been removed from
the specified toolbar.

(2021.02)
*/
- (void)
item:(NSToolbarItem*)			anItem
didExitToolbar:(NSToolbar*)		aToolbar
{
	[super item:anItem didExitToolbar:aToolbar];
	[self session:[aToolbar terminalToolbarDelegate].session setAsCurrentBinding:NO];
	[self setStateFromSession:nullptr];
}// item:didExitToolbar:


#pragma mark TerminalToolbar_SessionDependentItem


/*!
Invoked by the superclass whenever the target session of
the owning toolbar is about to change.

Responds by removing monitors on the previous session.

(2021.02)
*/
- (void)
willChangeSession
{
	[super willChangeSession];
	
	SessionRef	session = self.session;
	
	
	[self session:session setAsCurrentBinding:NO];
	[self setStateFromSession:nullptr];
}// willChangeSession


/*!
Invoked by the superclass whenever the target session of
the owning toolbar has changed.

Responds by updating the icon state.

(2021.02)
*/
- (void)
didChangeSession
{
	[super didChangeSession];
	
	SessionRef	session = self.session;
	
	
	[self setStateFromSession:session];
	[self session:session setAsCurrentBinding:YES];
}// didChangeSession


@end //} TerminalToolbar_ItemNotifyOnActivity


#pragma mark -
@implementation TerminalToolbar_ItemNotifyOnActivity (TerminalToolbar_ItemNotifyOnActivityInternal) //{


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored session event occurs.  See
"didChangeSession" for the set of events that is monitored.

(2021.02)
*/
- (void)
model:(ListenerModel_Ref)				aModel
sessionChange:(ListenerModel_Event)		anEvent
context:(void*)							aContext
{
#pragma unused(aModel)
	switch (anEvent)
	{
	case kSession_ChangeWatch:
		{
			SessionRef	session = REINTERPRET_CAST(aContext, SessionRef);
			
			
			[self setStateFromSession:session];
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"notifications toolbar item: session change handler received unexpected event", anEvent);
		break;
	}
}// model:sessionChange:context:


#pragma mark New Methods


/*!
Installs or removes callbacks based on a given SessionRef.
This should occur when a binding is going to change, such
as just before an item will be added, etc. (usually in
direct response to some other notification).

See also "setStateFromSession:", which only updates UI
aspects (such as icon and enabled state) based on a
given SessionRef.

(2021.02)
*/
- (void)
session:(SessionRef)		aSession
setAsCurrentBinding:(BOOL)	isBinding
{
	if (Session_IsValid(aSession))
	{
		if (isBinding)
		{
			Session_StartMonitoring(aSession, kSession_ChangeWatch, self.sessionChangeListener.listenerRef);
		}
		else
		{
			Session_StopMonitoring(aSession, kSession_ChangeWatch, self.sessionChangeListener.listenerRef);
		}
	}
}// session:setAsCurrentBinding:


/*!
Updates the toolbar icon based on the current notifications
of the given session.  If "nullptr" is given then the icon
is reset.

The icon is based on the effect the item will have when used,
NOT the current state of notifications.

See also "session:setAsCurrentBinding:", which installs or
removes callbacks based on a given SessionRef.

(2021.02)
*/
- (void)
setStateFromSession:(SessionRef)	aSession
{
	Boolean const	kSessionIsValid = Session_IsValid(aSession);
	
	
	self.enabled = kSessionIsValid;
	
	if ((kSessionIsValid) && Session_WatchIsOff(aSession))
	{
		self.toolTip = NSLocalizedString(@"Notify on next activity", @"toolbar item tooltip; set notification on activity");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"bell.badge" accessibilityDescription:self.toolTip];
		}
		else
		{
			// no icon is defined for 10.15 users (TEMPORARY?)
		}
	}
	else
	{
		self.toolTip = NSLocalizedString(@"Turn off notifications", @"toolbar item tooltip; remove notification on activity");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"bell.slash" accessibilityDescription:self.toolTip];
		}
		else
		{
			// no icon is defined for 10.15 users (TEMPORARY?)
		}
	}
}// setStateFromSession:


@end //} TerminalToolbar_ItemNotifyOnActivity (TerminalToolbar_ItemNotifyOnActivityInternal)


#pragma mark -
@implementation TerminalToolbar_ItemPrint //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDPrint];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Print", @"toolbar item name; for printing");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"printer" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnPrintIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performPrintSelection:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


@end //} TerminalToolbar_ItemPrint


#pragma mark -
@implementation TerminalToolbar_ItemStackWindows //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kTerminalToolbar_ItemIDStackWindows];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Arrange in Front", @"toolbar item name; for stacking windows");
		self.paletteLabel = self.label;
		self.toolTip = self.label;
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"macwindow.on.rectangle" accessibilityDescription:self.label];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnStackWindowsIconFilenameNoExtension(), NSString*)];
		}
	}
	return self;
}// init


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performArrangeInFront:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


#pragma mark NSToolbarItem


/*!
Returns YES only if the item should be enabled.  By default
this is the case as long as there are windows to arrange.

(4.0)
*/
- (BOOL)
validateToolbarItem:(NSToolbarItem*)	anItem
{
#pragma unused(anItem)
	BOOL	result = (SessionFactory_ReturnCount() > 1);
	
	
	return result;
}// validateToolbarItem:


@end //} TerminalToolbar_ItemStackWindows


#pragma mark -
@implementation TerminalToolbar_ItemSuspend //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDSuspend];
	if (nil != self)
	{
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Suspend (Scroll Lock)", @"toolbar item name; for suspending the running process");
		self.paletteLabel = self.label;
		_sessionChangeListener = [[ListenerModel_StandardListener alloc]
									initWithTarget:self
													eventFiredSelector:@selector(model:sessionChange:context:)];
		[self setStateFromSession:nullptr];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self willChangeSession]; // automatically stop monitoring the session
}// dealloc


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performSuspendToggle:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id								result = [super copyWithZone:zone];
	TerminalToolbar_ItemSuspend*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemSuspend.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemSuspend*);
	asSelf.sessionChangeListener = [self.sessionChangeListener copy];
	asSelf.sessionChangeListener.target = asSelf;
	
	return result;
}// copyWithZone:


#pragma mark TerminalToolbar_SessionDependentItem


/*!
Invoked by the superclass whenever the target session of
the owning toolbar is about to change.

Responds by removing monitors on the previous session.

(4.0)
*/
- (void)
willChangeSession
{
	[super willChangeSession];
	
	SessionRef	session = self.session;
	
	
	if (Session_IsValid(session))
	{
		Session_StopMonitoring(session, kSession_ChangeStateAttributes, self.sessionChangeListener.listenerRef);
	}
	[self setStateFromSession:nullptr];
}// willChangeSession


/*!
Invoked by the superclass whenever the target session of
the owning toolbar has changed.

Responds by updating the icon state.

(4.0)
*/
- (void)
didChangeSession
{
	[super didChangeSession];
	
	SessionRef	session = self.session;
	
	
	[self setStateFromSession:session];
	if (Session_IsValid(session))
	{
		Session_StartMonitoring(session, kSession_ChangeStateAttributes, self.sessionChangeListener.listenerRef);
	}
}// didChangeSession


@end //} TerminalToolbar_ItemSuspend


#pragma mark -
@implementation TerminalToolbar_ItemSuspend (TerminalToolbar_ItemSuspendInternal) //{


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored session event occurs.  See
"didChangeSession" for the set of events that is monitored.

(4.0)
*/
- (void)
model:(ListenerModel_Ref)				aModel
sessionChange:(ListenerModel_Event)		anEvent
context:(void*)							aContext
{
#pragma unused(aModel)
	switch (anEvent)
	{
	case kSession_ChangeStateAttributes:
		{
			SessionRef	session = REINTERPRET_CAST(aContext, SessionRef);
			
			
			[self setStateFromSession:session];
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"suspend-network toolbar item: session change handler received unexpected event", anEvent);
		break;
	}
}// model:sessionChange:context:


#pragma mark New Methods


/*!
Updates the toolbar icon based on the current suspend state
of the given session.  If "nullptr" is given then the icon
is reset.

The icon is based on the effect the item will have when used,
NOT the current state of suspension.

(4.0)
*/
- (void)
setStateFromSession:(SessionRef)	aSession
{
	if (Session_IsValid(aSession) && Session_NetworkIsSuspended(aSession))
	{
		self.toolTip = NSLocalizedString(@"Resume session (no scroll lock)", @"toolbar item tooltip; resume session");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"play.fill" accessibilityDescription:self.toolTip];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnScrollLockOffIconFilenameNoExtension(), NSString*)];
		}
	}
	else
	{
		self.toolTip = NSLocalizedString(@"Suspend session (scroll lock)", @"toolbar item tooltip; suspend session");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"pause.fill" accessibilityDescription:self.toolTip];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnScrollLockOnIconFilenameNoExtension(), NSString*)];
		}
	}
}// setStateFromSession:


@end //} TerminalToolbar_ItemSuspend (TerminalToolbar_ItemSuspendInternal)


#pragma mark -
@implementation TerminalToolbar_LEDItem //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)	anIdentifier
oneBasedIndexOfLED:(unsigned int)	anIndex
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		_indexOfLED = anIndex;
		_screenChangeListener = [[ListenerModel_StandardListener alloc]
									initWithTarget:self
													eventFiredSelector:@selector(model:screenChange:context:)];
		
		[self setStateFromScreen:nullptr];
	}
	return self;
}// initWithItemIdentifier:oneBasedIndexOfLED:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self willChangeSession]; // automatically stop monitoring the screen
}// dealloc


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id							result = [super copyWithZone:zone];
	TerminalToolbar_LEDItem*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_LEDItem.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_LEDItem*);
	asSelf.indexOfLED = self.indexOfLED;
	asSelf.screenChangeListener = [self.screenChangeListener copy];
	asSelf.screenChangeListener.target = asSelf;
	
	return result;
}// copyWithZone:


#pragma mark TerminalToolbar_SessionDependentItem


/*!
Invoked by the superclass whenever the target session of
the owning toolbar is about to change.

Responds by removing monitors on the previous session’s
screen buffers.

(4.0)
*/
- (void)
willChangeSession
{
	[super willChangeSession];
	
	TerminalScreenRef	screen = self.terminalScreen;
	
	
	if (nullptr != screen)
	{
		Terminal_StopMonitoring(screen, kTerminal_ChangeNewLEDState, self.screenChangeListener.listenerRef);
		[self setStateFromScreen:nullptr];
	}
}// willChangeSession


/*!
Invoked by the superclass whenever the target session of
the owning toolbar has changed.

Responds by updating the icon state.

(4.0)
*/
- (void)
didChangeSession
{
	[super didChangeSession];
	
	TerminalScreenRef	screen = self.terminalScreen;
	
	
	if (nullptr != screen)
	{
		[self setStateFromScreen:screen];
		Terminal_StartMonitoring(screen, kTerminal_ChangeNewLEDState, self.screenChangeListener.listenerRef);
	}
}// didChangeSession


@end //} TerminalToolbar_LEDItem


#pragma mark -
@implementation TerminalToolbar_LEDItem (TerminalToolbar_LEDItemInternal) //{


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored screen event occurs.  See
"didChangeSession" for the set of events that is monitored.

(4.0)
*/
- (void)
model:(ListenerModel_Ref)				aModel
screenChange:(ListenerModel_Event)		anEvent
context:(void*)							aContext
{
#pragma unused(aModel)
	switch (anEvent)
	{
	case kTerminal_ChangeNewLEDState:
		{
			TerminalScreenRef	screen = REINTERPRET_CAST(aContext, TerminalScreenRef);
			
			
			[self setStateFromScreen:screen];
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"LED toolbar item: terminal change handler received unexpected event", anEvent);
		break;
	}
}// model:screenChange:context:


#pragma mark New Methods


/*!
Updates the toolbar icon based on the current state of
the LED in the given screen buffer with the appropriate
index.  If "nullptr" is given then the icon is reset.

The icon is based on the effect the item will have when
used, NOT the current state of the bell.  So for
instance, a bell-off icon means “clicking this will
turn off the bell and the bell is currently on”.

(4.0)
*/
- (void)
setStateFromScreen:(TerminalScreenRef)		aScreen
{
	if ((nullptr == aScreen) || (false == Terminal_LEDIsOn(aScreen, STATIC_CAST(self.indexOfLED, SInt16))))
	{
		self.toolTip = NSLocalizedString(@"Turn on this LED", @"toolbar item tooltip; turn off LED");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"smallcircle.circle" accessibilityDescription:self.toolTip];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnLEDOffIconFilenameNoExtension(), NSString*)];
		}
	}
	else
	{
		self.toolTip = NSLocalizedString(@"Turn off this LED", @"toolbar item tooltip; turn on LED");
		if (@available(macOS 11.0, *))
		{
			self.image = [NSImage imageWithSystemSymbolName:@"largecircle.fill.circle" accessibilityDescription:self.toolTip];
		}
		else
		{
			self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnLEDOnIconFilenameNoExtension(), NSString*)];
		}
	}
}// setStateFromScreen:


@end //} TerminalToolbar_LEDItem (TerminalToolbar_LEDItemInternal)


#pragma mark -
@implementation TerminalToolbar_ItemWindowButton //{


@synthesize button = _button;


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		_button = nil; // set later
		_viewWindowObserver = nil; // initially...
		
		self.action = nil;
		self.target = nil;
		self.enabled = YES;
		self.view = nil; // set in "setButton:"
		self.label = @"";
		self.paletteLabel = @""; // set in subclasses
	}
	return self;
}// initWithItemIdentifier:


/*!
Destructor.

(2018.03)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
	[self removeObserverSpecifiedWith:self.viewWindowObserver];
}// dealloc


#pragma mark Accessors


// (See description in class interface.)
- (NSButton*)
button
{
	return _button;
}
- (void)
setButton:(NSButton*)	aWindowButton
{
	if (self.button != aWindowButton)
	{
		_button = aWindowButton;
		self.view = self.button;
		
		[self removeObserverSpecifiedWith:self.viewWindowObserver];
		_viewWindowObserver = [self newObserverFromSelector:@selector(window) ofObject:aWindowButton
															options:(NSKeyValueObservingOptionNew |
																		NSKeyValueObservingOptionOld)];
	}
}// setButton:


#pragma mark Notifications


/*!
Fixes an apparent bug when activating windows by forcing
the window button to redraw itself.

(2018.03)
*/
- (void)
windowDidBecomeMain:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.view.needsDisplay = YES;
}// windowDidBecomeMain:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id									result = [super copyWithZone:zone];
	TerminalToolbar_ItemWindowButton*	asSelf = nil;
	NSError* /*__autoreleasing*/			error = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowButton.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowButton*);
	
	// views do not support NSCopying; archive instead
	//asSelf.button = [self.button copy];
	NSData*		archivedView = [NSKeyedArchiver archivedDataWithRootObject:self.button requiringSecureCoding:NO error:&error];
	if (nil != error)
	{
		Console_Warning(Console_WriteValueCFString, "failed to copy window button toolbar item, archiving error", BRIDGE_CAST(error.localizedDescription, CFStringRef));
	}
	asSelf.button = [NSKeyedUnarchiver unarchivedObjectOfClass:NSButton.class fromData:archivedView error:&error];
	if (nil != error)
	{
		Console_Warning(Console_WriteValueCFString, "failed to copy window button toolbar item, unarchiving error", BRIDGE_CAST(error.localizedDescription, CFStringRef));
	}
	
	return result;
}// copyWithZone:


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to key values by updating dependent
states such as the display.

(2018.03)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
	BOOL	handled = NO;
	
	
	if (aContext == BRIDGE_CAST(_viewWindowObserver, void*))
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(window)))
			{
				id		oldValue = [aChangeDictionary objectForKey:NSKeyValueChangeOldKey];
				id		newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
				
				
				// remove any previous monitor
				if ((nil != oldValue) && ([NSNull null] != oldValue))
				{
					assert([oldValue isKindOfClass:NSWindow.class]);
					NSWindow*	asWindow = STATIC_CAST(oldValue, NSWindow*);
					
					
					[self ignoreWhenObject:asWindow postsNote:NSWindowDidBecomeMainNotification];
				}
				
				//NSLog(@"window changed: %@", aChangeDictionary); // debug
				if ((nil != newValue) && ([NSNull null] != newValue))
				{
					assert([newValue isKindOfClass:NSWindow.class]);
					NSWindow*	asWindow = STATIC_CAST(newValue, NSWindow*);
					Class		customFullScreenWindowClass = NSClassFromString(@"NSToolbarFullScreenWindow");
					
					
					// only change the target of the button if it is moving
					// into a window that is not the magic Full Screen window
					// (WARNING: this is fragile, as it depends entirely on
					// how Apple happens to implement this right now)
					if (nil == customFullScreenWindowClass)
					{
						Console_Warning(Console_WriteLine, "runtime did not find full-screen window class; may need code update");
					}
					
					if ((nil == customFullScreenWindowClass) || (NO == [asWindow isKindOfClass:customFullScreenWindowClass]))
					{
						self.button.target = asWindow;
						
						// start monitoring the window for activation; there is an apparent
						// bug where switching to a window can sometimes not refresh the
						// grayed-out appearance of the window buttons in toolbar items
						if (nil != asWindow)
						{
							[self whenObject:asWindow postsNote:NSWindowDidBecomeMainNotification
												performSelector:@selector(windowDidBecomeMain:)];
						}
					}
				}
			}
			else
			{
				Console_Warning(Console_WriteValueCFString, "valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


#pragma mark TerminalToolbar_ItemAddRemoveSensitive


/*!
Called when the given item is about to be added to the
specified toolbar.

(2018.03)
*/
- (void)
item:(NSToolbarItem*)			anItem
willEnterToolbar:(NSToolbar*)	aToolbar
{
#pragma unused(aToolbar)
	assert(self == anItem);
	// nothing yet
}// item:willEnterToolbar:


/*!
Called when the specified item has been removed from
the specified toolbar.

(2018.03)
*/
- (void)
item:(NSToolbarItem*)			anItem
didExitToolbar:(NSToolbar*)		aToolbar
{
#pragma unused(aToolbar)
	assert(self == anItem);
	// nothing yet
}// item:didExitToolbar:


@end //} TerminalToolbar_ItemWindowButton


#pragma mark -
@implementation TerminalToolbar_ItemWindowButton (TerminalToolbar_ItemWindowButtonInternal) //{


@end //} TerminalToolbar_ItemWindowButton (TerminalToolbar_ItemWindowButtonInternal)


#pragma mark -
@implementation TerminalToolbar_ItemWindowButtonClose //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		self.button = [NSWindow standardWindowButton:NSWindowCloseButton
														forStyleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
																		NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)];
		self.paletteLabel = NSLocalizedString(@"Close", @"toolbar item name; for closing the window");
		self.menuFormRepresentation = [[NSMenuItem alloc]
										initWithTitle:self.paletteLabel
														action:self.button.action
														keyEquivalent:@""];
	}
	return self;
}// initWithItemIdentifier:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id										result = [super copyWithZone:zone];
	TerminalToolbar_ItemWindowButtonClose*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowButtonClose.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowButtonClose*);
	// (nothing needed)
	
	return result;
}// copyWithZone:


#pragma mark NSToolbarItem


/*!
Validates the button (in this case, by disabling the button
if the target window cannot be zoomed right now).

(2018.10)
*/
- (void)
validate
{
	self.button.enabled = [[Commands_Executor sharedExecutor] validateAction:@selector(performClose:) sender:[NSApp keyWindow] sourceItem:self];
}// validate


@end //} TerminalToolbar_ItemWindowButtonClose


#pragma mark -
@implementation TerminalToolbar_ItemWindowButtonMinimize //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		self.button = [NSWindow standardWindowButton:NSWindowMiniaturizeButton
														forStyleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
																		NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)];
		self.paletteLabel = NSLocalizedString(@"Minimize", @"toolbar item name; for minimizing the window");
		self.menuFormRepresentation = [[NSMenuItem alloc]
										initWithTitle:self.paletteLabel
														action:self.button.action
														keyEquivalent:@""];
	}
	return self;
}// initWithItemIdentifier:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id											result = [super copyWithZone:zone];
	TerminalToolbar_ItemWindowButtonMinimize*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowButtonMinimize.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowButtonMinimize*);
	// (nothing needed)
	
	return result;
}// copyWithZone:


#pragma mark NSToolbarItem


/*!
Validates the button (in this case, by disabling the button
if the target window cannot be minimized right now).

(2018.03)
*/
- (void)
validate
{
	self.button.enabled = [[Commands_Executor sharedExecutor] validateAction:@selector(performMiniaturize:) sender:[NSApp keyWindow] sourceItem:self];
}// validate


@end //} TerminalToolbar_ItemWindowButtonMinimize


#pragma mark -
@implementation TerminalToolbar_ItemWindowButtonZoom //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		self.button = [NSWindow standardWindowButton:NSWindowZoomButton
														forStyleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
																		NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)];
		self.paletteLabel = NSLocalizedString(@"Zoom", @"toolbar item name; for zooming the window or controlling Full Screen");
		self.menuFormRepresentation = [[NSMenuItem alloc]
										initWithTitle:self.paletteLabel
														action:self.button.action
														keyEquivalent:@""];
	}
	return self;
}// initWithItemIdentifier:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id										result = [super copyWithZone:zone];
	TerminalToolbar_ItemWindowButtonZoom*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowButtonZoom.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowButtonZoom*);
	// (nothing needed)
	
	return result;
}// copyWithZone:


#pragma mark NSToolbarItem


/*!
Validates the button (in this case, by disabling the button
if the target window cannot be zoomed right now).

(2018.10)
*/
- (void)
validate
{
	self.button.enabled = [[Commands_Executor sharedExecutor] validateAction:@selector(performZoom:) sender:[NSApp keyWindow] sourceItem:self];
}// validate


@end //} TerminalToolbar_ItemWindowButtonZoom


#pragma mark -
@implementation TerminalToolbar_TextLabel //{


@synthesize labelLayout = _labelLayout;
@synthesize mouseDownCanMoveWindow = _mouseDownCanMoveWindow;


#pragma mark Class Methods


/*!
Allocates a new image that renders a gradient fade.

(2018.03)
*/
+ (CGImageRef)
newFadeMaskImageWithSize:(NSSize)				aSize
labelLayout:(TerminalToolbar_TextLabelLayout)	aDirection
{
	CGImageRef			result = nullptr;
	CGColorSpaceRef		grayColorSpace = [[NSColorSpace genericGrayColorSpace] CGColorSpace];
	CGContextRef		maskContext = CGBitmapContextCreate(nullptr/* source data or nullptr for auto-allocate */,
															aSize.width, aSize.height,
															8/* bits per component*/, 0/* bytes per row or 0 for auto */,
															grayColorSpace, kCGImageAlphaNone);
	NSGraphicsContext*	asContextObj = [NSGraphicsContext graphicsContextWithCGContext:maskContext flipped:NO];
	
	
	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext setCurrentContext:asContextObj];
	if (kTerminalToolbar_TextLabelLayoutRightJustified == aDirection)
	{
		NSGradient*		grayGradient = [[NSGradient alloc] initWithColors:@[[NSColor blackColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor]]];
		
		
		[grayGradient drawFromPoint:NSZeroPoint toPoint:NSMakePoint(CGBitmapContextGetWidth(maskContext), 0) options:0];
	}
	else if (kTerminalToolbar_TextLabelLayoutCenterJustified == aDirection)
	{
		NSGradient*		grayGradient = [[NSGradient alloc] initWithColors:@[[NSColor whiteColor], [NSColor whiteColor], [NSColor blackColor], [NSColor whiteColor], [NSColor whiteColor]]];
		
		
		[grayGradient drawFromPoint:NSZeroPoint toPoint:NSMakePoint(CGBitmapContextGetWidth(maskContext), 0) options:0];
	}
	else
	{
		NSGradient*		grayGradient = [[NSGradient alloc] initWithColors:@[[NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor blackColor]]];
		
		
		[grayGradient drawFromPoint:NSZeroPoint toPoint:NSMakePoint(CGBitmapContextGetWidth(maskContext), 0) options:0];
	}
	[NSGraphicsContext restoreGraphicsState];
	
	result = CGBitmapContextCreateImage(maskContext);
	CFRelease(maskContext);
	
    return result;
}// newFadeMaskImageWithSize:direction:


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithFrame:(NSRect)	aRect
{
	self = [super initWithFrame:aRect];
	if (nil != self)
	{
		_disableFrameMonitor = NO;
		_frameDisplayEnabled = NO;
		_gradientFadeEnabled = NO;
		_stringValueObserver = [self newObserverFromSelector:@selector(stringValue) ofObject:self
																options:(NSKeyValueObservingOptionNew)];
		_labelLayout = kTerminalToolbar_TextLabelLayoutLeftJustified; // see also "lineBreakMode" below
		_mouseDownCanMoveWindow = NO;
		_smallSize = NO;
		
		// initialize various text field or view properties (most of these
		// can be changed later to have the desired effect)
		self.allowsDefaultTighteningForTruncation = YES;
		self.bezeled = NO;
		self.bordered = NO;
		self.drawsBackground = NO;
		self.editable = NO;
		//self.lineBreakMode = NSLineBreakByTruncatingTail; // note: set in "layOutLabelText"
		self.maximumNumberOfLines = 1;
		self.selectable = NO;
		self.translatesAutoresizingMaskIntoConstraints = NO;
		
		self.postsFrameChangedNotifications = YES;
		[self whenObject:self postsNote:NSViewFrameDidChangeNotification
							performSelector:@selector(textViewFrameDidChange:)];
		
		[self layOutLabelText];
	}
	return self;
}// initWithFrame:


/*!
Destructor.

(2018.03)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
	[self removeObserverSpecifiedWith:self.stringValueObserver];
}// dealloc


#pragma mark Accessors


// (See description in class interface.)
- (TerminalToolbar_TextLabelLayout)
labelLayout
{
	return _labelLayout;
}
- (void)
setLabelLayout:(TerminalToolbar_TextLabelLayout)	aLayoutValue
{
	if (_labelLayout != aLayoutValue)
	{
		_labelLayout = aLayoutValue;
		
		[self layOutLabelText];
	}
}// setLabelLayout:


#pragma mark Notifications


/*!
Responds when the label view frame is changed by a system API
such as a direct modification of the view’s "frame" property.

See "layOutLabelText" for specific behavior.

(2018.03)
*/
- (void)
textViewFrameDidChange:(NSNotification*)	aNotification
{
	// note: system should prevent this method from even being called
	// while "postsFrameChangedNotifications" is NO but check anyway
	if (self.postsFrameChangedNotifications && (NO == self.disableFrameMonitor))
	{
		if (NO == [aNotification.object isKindOfClass:TerminalToolbar_TextLabel.class])
		{
			Console_Warning(Console_WriteLine, "text label frame-change notification expected an TerminalToolbar_TextLabel object!");
		}
		else
		{
			TerminalToolbar_TextLabel*		asTextView = STATIC_CAST(aNotification.object, TerminalToolbar_TextLabel*);
			assert(self == asTextView);
			
			
			[asTextView layOutLabelText];
		}
	}
}// textViewFrameDidChange:


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to key values by updating dependent
states such as the display.

(2020.12)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
	BOOL	handled = NO;
	
	
	if (aContext == BRIDGE_CAST(_stringValueObserver, void*))
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(stringValue)))
			{
				id		newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
				
				
				//NSLog(@"stringValue changed: %@", aChangeDictionary); // debug
				if ((nil != newValue) && ([NSNull null] != newValue))
				{
					assert([newValue isKindOfClass:NSString.class]);
					//NSString*	asString = STATIC_CAST(newValue, NSString*);
					
					
					// optimistically reset to largest possible font (layout will
					// shrink if there is not in fact enough room)
					[self layOutLabelText];
				}
			}
			else
			{
				Console_Warning(Console_WriteValueCFString, "valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


#pragma mark NSView


/*!
Draws the text label, possibly with a text fade-out effect
for truncation.

(2018.03)
*/
- (void)
drawRect:(NSRect)	aRect
{
	// save/restore context upon return from function;
	// note that "super" is called within this,
	// allowing (for instance) gradient masks to
	// apply to system-supplied text drawing
	CGContextRef			graphicsContext = [[NSGraphicsContext currentContext] CGContext];
	CGContextSaveRestore	_(graphicsContext);
	
	
	if (self.frameDisplayEnabled)
	{
		// presumably in toolbar customization mode; show a light boundary rectangle
		CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
		
		
		CGContextSetRGBStrokeColor(drawingContext, 0.75, 0.75, 0.75, 1.0/* alpha */); // color attempts to match “space item” style
		CGContextSetLineWidth(drawingContext, 1.0);
		CGContextStrokeRect(drawingContext, NSRectToCGRect(NSInsetRect(self.bounds, 1.0, 1.0)));
	}
#if 0
	else
	{
		// for debugging; display a red rectangle to show the area occupied by the view
		CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
		
		
		CGContextSetRGBStrokeColor(drawingContext, 1.0, 0.0, 0.0, 1.0/* alpha */);
		CGContextSetLineWidth(drawingContext, 2.0);
		CGContextStrokeRect(drawingContext, NSRectToCGRect(NSInsetRect(self.bounds, 1.0, 1.0)));
	}
#endif
	
	if (self.gradientFadeEnabled)
	{
		CGImageRef		maskImage = [self.class newFadeMaskImageWithSize:self.bounds.size
																			labelLayout:self.labelLayout];
		
		
		CGContextClipToMask(graphicsContext, NSRectToCGRect(self.bounds), maskImage);
		CFRelease(maskImage); maskImage = nullptr;
	}
	
	//[self.stringValue drawAtPoint:self.bounds.origin withAttributes:nil];
	[super drawRect:aRect];
}// drawRect:


/*!
Calculates the intrinsic content size using the largest
expected font, as opposed to the current font.

This is overridden because the view scales the font when
there is not enough space; the best size is the one that
allows the font to be as large as possible.

(2020.12)
*/
- (NSSize)
intrinsicContentSize
{
	NSSize		result = NSZeroSize;
	NSFont*		originalFont = self.font;
	
	
	self.font = [NSFont titleBarFontOfSize:((self.smallSize) ? 14 : 16)];
	result = [super intrinsicContentSize];
	self.font = originalFont;
	
	return result;
}// intrinsicContentSize


@end //} TerminalToolbar_TextLabel


#pragma mark -
@implementation TerminalToolbar_TextLabel (TerminalToolbar_TextLabelInternal) //{


#pragma mark New Methods


/*!
Returns the size that best fits the given string with
current font settings.

See also "layOutLabelText".

(2018.03)
*/
- (CGSize)
idealFrameSizeForString:(NSString*)		aString
{
	CGSize		result = CGSizeZero;
	
	
	if (nil != aString)
	{
		NSString*	originalValue = self.stringValue;
		
		
		[self removeObserverSpecifiedWith:_stringValueObserver];
		self.stringValue = aString;
		[self invalidateIntrinsicContentSize];
		// use superclass for intrinsic size so that current font is used
		// (otherwise, current class overrides to use largest font)
		result = [super intrinsicContentSize];
		self.stringValue = originalValue; // remove sizing string
		[self invalidateIntrinsicContentSize];
		self.stringValueObserver = [self newObserverFromSelector:@selector(stringValue) ofObject:self
																	options:(NSKeyValueObservingOptionNew)];
	}
	
	return result;
}// idealFrameSizeForString:


/*!
Examines the current text and updates properties to make
the ideal layout: possibly new font, etc.

Invoke this method whenever the text needs to be reset
(frame change, string value change, initialization, etc.).

(2018.03)
*/
- (void)
layOutLabelText
{
	BOOL const		isSmallSize = self.smallSize;
	CGFloat			originalWidth = NSWidth(self.frame);
	//CGFloat			originalHeight = NSHeight(self.frame);
	NSString*		layoutString = (((nil == self.stringValue) || (0 == self.stringValue.length))
									? @"Âgp" // arbitrary; lay out with text, not a blank space
									: self.stringValue);
	
	
	// set initial truncation from layout (may be overridden below
	// if the text must wrap to two lines)
	switch (self.labelLayout)
	{
	case kTerminalToolbar_TextLabelLayoutLeftJustified:
		self.alignment = NSTextAlignmentLeft; // NSControl setting
		self.lineBreakMode = NSLineBreakByTruncatingTail;
		break;
	
	case kTerminalToolbar_TextLabelLayoutRightJustified:
		self.alignment = NSTextAlignmentRight; // NSControl setting
		self.lineBreakMode = NSLineBreakByTruncatingHead;
		break;
	
	case kTerminalToolbar_TextLabelLayoutCenterJustified:
	default:
		self.alignment = NSTextAlignmentCenter; // NSControl setting
		self.lineBreakMode = NSLineBreakByTruncatingMiddle;
		break;
	}
	
	// initialize line count (may be overridden below)
	self.usesSingleLineMode = YES;
	self.maximumNumberOfLines = 1;
	
	// test a variety of font sizes to find the most reasonable one;
	// if text ends up too long anyway then view will squish/truncate
	// (TEMPORARY...should any of this be a user preference?)
	self.font = [NSFont titleBarFontOfSize:((isSmallSize) ? 14 : 16)];
	CGSize		requiredSize = [self idealFrameSizeForString:layoutString];
	if (requiredSize.width > originalWidth)
	{
		self.font = [NSFont titleBarFontOfSize:((isSmallSize) ? 13 : 14)];
		requiredSize = [self idealFrameSizeForString:layoutString];
	}
	if (requiredSize.width > originalWidth)
	{
		self.font = [NSFont titleBarFontOfSize:((isSmallSize) ? 12 : 13)];
		requiredSize = [self idealFrameSizeForString:layoutString];
	}
	if (requiredSize.width > originalWidth)
	{
		// at this point, switch to multiple lines as well
		// (experimental, not working yet)
	#if 0
		self.usesSingleLineMode = NO;
		self.maximumNumberOfLines = 2;
	#endif
		self.font = [NSFont titleBarFontOfSize:((isSmallSize) ? 10 : 12)];
		requiredSize = [self idealFrameSizeForString:layoutString];
	}
	if (requiredSize.width > originalWidth)
	{
		self.font = [NSFont titleBarFontOfSize:((isSmallSize) ? 9 : 10)];
		requiredSize = [self idealFrameSizeForString:layoutString];
	}
	
	// if there is still not enough room, fade out the text
	self.gradientFadeEnabled = (requiredSize.width > originalWidth);
	
	// if the font changes then the ideal text height will change;
	// also, the appearance has changed and text must be redrawn
	BOOL		didNotify = self.disableFrameMonitor;
	NSRect		oldFrame = self.frame;
	CGFloat		newHeight = 0;
	self.disableFrameMonitor = YES; // disable this view’s callback, otherwise setting "frame" will loop back to this point
	[self sizeToFit];
	newHeight = self.frame.size.height;
	self.frame = NSMakeRect(oldFrame.origin.x, oldFrame.origin.y, oldFrame.size.width, newHeight);
	self.disableFrameMonitor = didNotify;
	self.needsDisplay = YES;
}// layOutLabelText


@end //} TerminalToolbar_TextLabel (TerminalToolbar_TextLabelInternal)


#pragma mark -
@implementation TerminalToolbar_WindowTitleBox //{


#pragma mark NSResponder


/*!
Since this appears to be the title of a window, this tries
to handle mouse-down events in a way that is similar to a
normal window.

Currently this is a placeholder; "mouseDownCanMoveWindow"
is set to YES on this view so it is not necessary to use
a custom mouse-down event to let the user move the window.
In the future though it may be desirable to implement
something like a command-click-path-select menu, in which
case this event override would be required.

(2020.12)
*/
- (void)
mouseDown:(NSEvent*)	anEvent
{
	[super mouseDown:anEvent];
}// mouseDown:


/*!
Since this appears to be the title of a window, this
tries to handle a double-click event in a way that
is similar to a normal window.  For example, it
could cause the window to automatically zoom based
on user preferences.

(2020.12)
*/
- (void)
mouseUp:(NSEvent*)	anEvent
{
	if (anEvent.clickCount != 2)
	{
		[super mouseUp:anEvent];
	}
	else
	{
		// emulate standard system behavior, e.g. to auto-zoom; currently
		// this can be found using NSGlobalDomain and AppleActionOnDoubleClick
		// (value will be "Minimize" or "Maximize"); note that if this ever
		// fails to work, it is still possible for the user to get the
		// expected behavior by clicking elsewhere in the window frame (e.g.
		// at the top) but this is more useful because it is a bigger area
		NSUserDefaults*					userDefaults = [NSUserDefaults standardUserDefaults];
		NSDictionary< NSString*, id >*	prefsDict = [userDefaults persistentDomainForName:NSGlobalDomain];
		id								prefValue = [prefsDict objectForKey:@"AppleActionOnDoubleClick"];
		
		
		if (nil == prefValue)
		{
			if (nil == prefsDict)
			{
				Console_Warning(Console_WriteLine, "double-click cannot be handled, no global domain data found");
			}
			else
			{
				Console_Warning(Console_WriteLine, "double-click cannot be handled, no window double-click preference found");
			}
		}
		else
		{
			if ([prefValue isKindOfClass:NSString.class])
			{
				NSString*	asString = STATIC_CAST(prefValue, NSString*);
				
				
				if ([asString isEqualToString:@"Minimize"])
				{
					[self.window performMiniaturize:nil];
				}
				else if ([asString isEqualToString:@"Maximize"])
				{
					[self.window performZoom:nil];
				}
				else
				{
					Console_Warning(Console_WriteValueCFString, "double-click cannot be handled, received unexpected preference value", BRIDGE_CAST(asString, CFStringRef));
				}
			}
			else
			{
				Console_Warning(Console_WriteLine, "double-click cannot be handled, received non-string for double-click preference value");
			}
		}
	}
}// mouseUp:


@end //}


#pragma mark -
@implementation TerminalToolbar_WindowTitleLabel //{


@synthesize overrideWindow = _overrideWindow;


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithFrame:(NSRect)	aRect
{
	self = [super initWithFrame:aRect];
	if (nil != self)
	{
		_overrideWindow = nil;
		_windowMonitor = nil;
		_windowTitleObserver = nil; // initially...
		
		self.labelLayout = kTerminalToolbar_TextLabelLayoutCenterJustified;
	}
	return self;
}// initWithFrame:


/*!
Destructor.

(2018.03)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
	[self removeObserverSpecifiedWith:self.windowTitleObserver];
}// dealloc


#pragma mark Accessors


// (See description in class interface.)
- (NSWindow*)
overrideWindow
{
	return _overrideWindow;
}
- (void)
setOverrideWindow:(NSWindow*)	aWindow
{
	if (_overrideWindow != aWindow)
	{
		_overrideWindow = aWindow;
		[self removeObserverSpecifiedWith:self.windowTitleObserver];
		self.windowTitleObserver = [self newObserverFromSelector:@selector(title) ofObject:aWindow
																	options:(NSKeyValueObservingOptionNew)];
	}
}// setOverrideWindow


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to key values by updating dependent
states such as the display.

(2018.03)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
	BOOL	handled = NO;
	
	
	if (aContext == BRIDGE_CAST(_windowTitleObserver, void*))
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(title)))
			{
				id		newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
				
				
				//NSLog(@"title changed: %@", aChangeDictionary); // debug
				if ((nil != newValue) && ([NSNull null] != newValue))
				{
					assert([newValue isKindOfClass:NSString.class]);
					NSString*	asString = STATIC_CAST(newValue, NSString*);
					
					
					self.stringValue = asString;
				}
			}
			else
			{
				Console_Warning(Console_WriteValueCFString, "valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


#pragma mark NSView


/*!
Invoked when the view’s "window" property changes to its
current value (including a nil value).

(2018.03)
*/
- (void)
viewDidMoveToWindow
{
	[super viewDidMoveToWindow];
	
	// notify any monitor
	[self.windowMonitor view:self didEnterWindow:self.window];
}// viewDidMoveToWindow


/*!
Invoked when the view’s "window" property is about to
change to the specified value (or nil).

(2018.03)
*/
- (void)
viewWillMoveToWindow:(NSWindow*)	aWindow
{
	[super viewWillMoveToWindow:aWindow];
	
	// notify any monitor
	[self.windowMonitor willChangeWindowForView:self];
	
	if (nil == aWindow)
	{
		// it appears that the view can transition to a nil window just
		// before moving to a Full Screen window, which is not helpful
		// so it is explicitly ignored (namely, any observer for the
		// most recent valid window will remain in effect)
	}
	else
	{
		// remove any observer of the current "self.window" object; note,
		// a bizarre thing in Full Screen windows is that a view can be
		// automatically relocated to an entirely different window (which
		// is an unusually big problem when this view is trying to observe
		// its ORIGINAL window) so a special check is made to avoid that
		if ((nil == self.overrideWindow) && (aWindow != self.window) &&
			(self.windowTitleObserver.observedObject != aWindow))
		{
			Class	customFullScreenWindowClass = NSClassFromString(@"NSToolbarFullScreenWindow");
			
			
			if (nil == customFullScreenWindowClass)
			{
				Console_Warning(Console_WriteLine, "runtime did not find full-screen window class; may need code update");
			}
			
			if ((nil == customFullScreenWindowClass) || (NO == [aWindow isKindOfClass:customFullScreenWindowClass]))
			{
				[self removeObserverSpecifiedWith:self.windowTitleObserver];
				_windowTitleObserver = [self newObserverFromSelector:@selector(title) ofObject:aWindow
																		options:(NSKeyValueObservingOptionNew)];
				
				// is this necessary, or is the observer called automatically from the above?
				self.stringValue = ((nil != aWindow.title) ? aWindow.title : @"");
			}
		}
	}
}// viewWillMoveToWindow:


@end //} TerminalToolbar_WindowTitleLabel


#pragma mark -
@implementation TerminalToolbar_ItemWindowTitle //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		TerminalToolbar_WindowTitleLabel*	textView = nil;
		TerminalToolbar_WindowTitleBox*		borderView = nil;
		NSLayoutConstraint*					newConstraint = nil; // reused below
		
		
		textView = [[TerminalToolbar_WindowTitleLabel alloc] initWithFrame:NSZeroRect];
		textView.windowMonitor = self;
		textView.mouseDownCanMoveWindow = YES;
		//textView.font = [NSFont titleBarFontOfSize:9]; // set in TerminalToolbar_WindowTitleLabel
		textView.labelLayout = kTerminalToolbar_TextLabelLayoutCenterJustified; // subclasses can change this; NOTE: also consulted to see if toolbar item should auto-center
		textView.translatesAutoresizingMaskIntoConstraints = NO;
		
		// note: special border subclass is used to get title double-click behavior (otherwise the same as NSBox)
		borderView = [[TerminalToolbar_WindowTitleBox alloc] initWithFrame:NSZeroRect];
		borderView.boxType = NSBoxCustom;
		borderView.borderType = NSLineBorder;
		borderView.borderColor = [NSColor clearColor]; // (changes if customizing)
		borderView.borderWidth = 1.0;
		borderView.contentView = textView; // retain
		borderView.translatesAutoresizingMaskIntoConstraints = NO;
		
		// set some constant constraints (others are defined by
		// "updateSizeConstraints"); constraints MUST be archived
		// because toolbar items copy from customization palette
		newConstraint = [textView.leadingAnchor constraintEqualToAnchor:borderView.leadingAnchor constant:4.0/* arbitrary */];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		newConstraint = [borderView.trailingAnchor constraintEqualToAnchor:textView.trailingAnchor constant:4.0/* arbitrary */];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		newConstraint = [textView.centerYAnchor constraintEqualToAnchor:borderView.centerYAnchor];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		newConstraint = [borderView.topAnchor constraintLessThanOrEqualToAnchor:textView.topAnchor];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		newConstraint = [borderView.bottomAnchor constraintGreaterThanOrEqualToAnchor:textView.bottomAnchor];
		newConstraint.active = YES;
		newConstraint.shouldBeArchived = YES;
		self.view = borderView; // retain
		[self updateSizeConstraints];
		
		// TEMPORARY: "minSize" and "maxSize" now trigger deprecation warnings
		// in the debugger but these settings are required for “flexible width”
		// behavior, despite Auto Layout (reported as FB8929720)
		self.minSize = NSMakeSize(100, 24); // arbitrary
		self.maxSize = NSMakeSize(2048, 32); // arbitrary
		
		self.action = @selector(performToolbarItemAction:);
		self.target = self;
		self.enabled = YES;
		self.label = NSLocalizedString(@"Window Title", @"toolbar item name; for window title");
		self.paletteLabel = NSLocalizedString(@"Window Title", @"toolbar item name; for window title");
		self.toolTip = self.label;
		[self setStateForToolbar:nil];
		
		assert(textView == [self textView]); // test accessor
		assert(borderView == [self containerViewAsBox]); // test accessor
	}
	return self;
}// initWithItemIdentifier:


/*!
Destructor.

(2018.03)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark Actions


/*!
Responds when the toolbar item is used.

(2018.03)
*/
- (void)
performToolbarItemAction:(id)	sender
{
	if (false == Commands_ViaFirstResponderPerformSelector(@selector(performRename:), sender))
	{
		Sound_StandardAlert();
	}
}// performToolbarItemAction:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id									result = [super copyWithZone:zone];
	TerminalToolbar_ItemWindowTitle*	asSelf = nil;
	NSError* /*__autoreleasing*/			error = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowTitle.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowTitle*);
	
	// views do not support NSCopying; archive instead (also, be sure that
	// any NSLayoutConstraint instances have "shouldBeArchived = YES")
	//asSelf.view = [self.view copy];
	NSData*		archivedView = [NSKeyedArchiver archivedDataWithRootObject:self.view requiringSecureCoding:NO error:&error];
	if (nil != error)
	{
		Console_Warning(Console_WriteValueCFString, "failed to copy toolbar item border, archiving error", BRIDGE_CAST(error.localizedDescription, CFStringRef));
	}
	asSelf.view = [NSKeyedUnarchiver unarchivedObjectOfClass:NSBox.class fromData:archivedView error:&error];
	if (nil != error)
	{
		Console_Warning(Console_WriteValueCFString, "failed to copy toolbar item border, unarchiving error", BRIDGE_CAST(error.localizedDescription, CFStringRef));
	}
	
	return result;
}// copyWithZone:


#pragma mark NSToolbarItem


/*!
Validates the item by updating its appearance.

(2020.12)
*/
- (void)
validate
{
	TerminalToolbar_WindowTitleLabel*	textView = [self textView];
	
	
	self.enabled = [self.view.window isKeyWindow];
	//self.menuFormRepresentation.enabled = self.enabled;
	textView.enabled = self.enabled;
	
	[self setStateForToolbar:self.toolbar];
}// validate


#pragma mark TerminalToolbar_DisplayModeSensitive


/*!
Keeps track of changes to the mode (such as “text only”)
so that the window layout can adapt if necessary.

(2018.03)
*/
- (void)
didChangeDisplayModeForToolbar:(NSToolbar*)		aToolbar
{
#pragma unused(aToolbar)
	[self updateSizeConstraints];
	self.view.needsDisplay = YES;
}// didChangeDisplayModeForToolbar:


#pragma mark TerminalToolbar_ItemAddRemoveSensitive


/*!
Called when the given item is about to be added to the
specified toolbar.

If the layout of the "textView" is centered (i.e.
"kTerminalToolbar_TextLabelLayoutCenterJustified"),
the "centeredItemIdentifier" of the given toolbar is
set to be the given item.

(2018.03)
*/
- (void)
item:(NSToolbarItem*)			anItem
willEnterToolbar:(NSToolbar*)	aToolbar
{
	assert(self == anItem);
	self.textView.smallSize = ((nil != aToolbar) &&
								(NSToolbarSizeModeSmall == aToolbar.sizeMode));
	if (kTerminalToolbar_TextLabelLayoutCenterJustified == self.textView.labelLayout)
	{
		// auto-center this toolbar item if possible (in practice
		// the system will shift the item a bit if it is not
		// reasonable to fit other items around it)
		aToolbar.centeredItemIdentifier = anItem.itemIdentifier;
	}
	[self setStateForToolbar:aToolbar];
	[self updateSizeConstraints];
}// item:willEnterToolbar:


/*!
Called when the specified item has been removed from
the specified toolbar.

(2018.03)
*/
- (void)
item:(NSToolbarItem*)			anItem
didExitToolbar:(NSToolbar*)		aToolbar
{
#pragma unused(aToolbar)
	assert(self == anItem);
	if (kTerminalToolbar_TextLabelLayoutCenterJustified == self.textView.labelLayout)
	{
		aToolbar.centeredItemIdentifier = nil;
	}
}// item:didExitToolbar:


#pragma mark TerminalToolbar_ItemHasPaletteProxy


/*!
Returns the item that represents this item in a customization palette.

(2018.03)
*/
- (NSToolbarItem*)
paletteProxyToolbarItemWithIdentifier:(NSString*)	anIdentifier
{
	NSToolbarItem*		result = [[NSToolbarItem alloc] initWithItemIdentifier:anIdentifier];
	
	
	result.paletteLabel = self.paletteLabel;
	result.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleCenterIconFilenameNoExtension(), NSString*)];
	
	return result;
}// paletteProxyToolbarItemWithIdentifier:


#pragma mark TerminalToolbar_SizeSensitive


/*!
Keeps track of changes to the icon and label size
so that the window layout can adapt if necessary.

(2018.03)
*/
- (void)
didChangeSizeForToolbar:(NSToolbar*)	aToolbar
{
	self.textView.smallSize = ((nil != aToolbar) &&
								(NSToolbarSizeModeSmall == aToolbar.sizeMode));
	[self setStateForToolbar:aToolbar];
	[self updateSizeConstraints];
	self.view.needsDisplay = YES;
}// didChangeSizeForToolbar:


#pragma mark TerminalToolbar_ViewWindowSensitive


/*!
Updates the window title item accordingly after the
window title view enters the toolbar and presumably
adopts that window’s title value.

(2018.03)
*/
- (void)
view:(NSView*)					aView
didEnterWindow:(NSWindow*)		aWindow
{
	// note: notification is sent for text view but that is not the main view
	if (aView != self.textView)
	{
		Console_Warning(Console_WriteLine, "window title toolbar item is being notified of a different view’s window changes");
	}
	else
	{
		// set initial state of item for new window’s toolbar
		[self setStateForToolbar:aWindow.toolbar];
		[self updateSizeConstraints];
	}
}// view:didEnterWindow:


/*!
Notified when the toolbar item’s window is about to
change.

(2018.03)
*/
- (void)
willChangeWindowForView:(NSView*)	aView
{
	// note: notification is sent for text view but that is not the main view
	if (aView != self.textView)
	{
		Console_Warning(Console_WriteLine, "window title toolbar item is being notified of a different view’s window changes");
	}
	else
	{
		// (nothing needed)
	}
}// willChangeWindowForView:


@end //} TerminalToolbar_ItemWindowTitle


#pragma mark -
@implementation TerminalToolbar_ItemWindowTitle (TerminalToolbar_ItemWindowTitleInternal) //{


#pragma mark New Methods


/*!
Update and return the ideal size for this item (fitting
the title).  A minimum size is enforced.

(2020.12)
*/
- (NSSize)
calculateIdealSize
{
	NSBox*								containerBox = [self containerViewAsBox];
	TerminalToolbar_WindowTitleLabel*	textView = [self textView];
	CGFloat const	idealWidth = ceil(textView.intrinsicContentSize.width + (2.0 * containerBox.contentViewMargins.width) + (2.0 * containerBox.borderWidth));
	CGFloat const	idealHeight = ceil(textView.intrinsicContentSize.height + (2.0 * containerBox.contentViewMargins.height) + (2.0 * containerBox.borderWidth));
	NSSize			result = NSMakeSize(std::max<CGFloat>(120/* arbitrary */, idealWidth), idealHeight);
	
	
	return result;
}// calculateIdealSize


/*!
Return the main item view as an NSBox class for convenience.

(2020.12)
*/
- (NSBox*)
containerViewAsBox
{
	NSBox*	result = nil;
	
	
	if ([self.view isKindOfClass:NSBox.class])
	{
		result = STATIC_CAST(self.view, NSBox*);
	}
	
	if (nil == result)
	{
		Console_Warning(Console_WriteLine, "window title toolbar item container view is not NSBox type!");
	}
	
	return result;
}// containerViewAsBox


/*!
Synchronizes other properties of the toolbar item with
the properties of its embedded text view, preparing for
display in the given toolbar (which may or may not be
the item’s current toolbar).

(2018.03)
*/
- (void)
setStateForToolbar:(NSToolbar*)		aToolbar
{
	if (aToolbar.customizationPaletteIsRunning)
	{
		[self containerViewAsBox].borderColor = [NSColor lightGrayColor]; // color attempts to match “space item” style
	}
	else
	{
		[self containerViewAsBox].borderColor = [NSColor clearColor];
	}
	
	//BOOL const		isSmallSize = ((nil != aToolbar) &&
	//								(NSToolbarSizeModeSmall == aToolbar.sizeMode));
	BOOL const		isTextOnly = ((nil != aToolbar) &&
									(NSToolbarDisplayModeLabelOnly == aToolbar.displayMode));
	
	
	//self.textView.frameDisplayEnabled = (aToolbar.customizationPaletteIsRunning);
	self.textView.toolTip = self.textView.stringValue;
	
	// this allows text-only toolbars to still display the window title string
	self.label = ((isTextOnly)
					? [NSString stringWithFormat:@"  %@  ", ((nil != self.textView.stringValue) ? self.textView.stringValue : @"")]
					: @"");
	
	self.menuFormRepresentation = [[NSMenuItem alloc] initWithTitle:self.textView.stringValue action:nil keyEquivalent:@""];
	self.menuFormRepresentation.enabled = NO;
}// setStateForToolbar:


/*!
Return the text view used for displaying the title.

(2020.12)
*/
- (TerminalToolbar_WindowTitleLabel*)
textView
{
	TerminalToolbar_WindowTitleLabel*	result = nil;
	
	
	for (NSView* aView in self.view.subviews)
	{
		if ([aView isKindOfClass:TerminalToolbar_WindowTitleLabel.class])
		{
			result = STATIC_CAST(aView, TerminalToolbar_WindowTitleLabel*);
			break;
		}
	}
	
	if (nil == result)
	{
		//Console_Warning(Console_WriteLine, "window title toolbar item text view is not defined!"); // debug
	}
	
	return result;
}// textView


/*!
Specify minimum and maximum width and height, as well as
priorities for content hugging and compression resistance.

(2020.12)
*/
- (void)
updateSizeConstraints
{
	NSSize		idealSize = [self calculateIdealSize];
	
	
	if (nil != self.heightConstraint)
	{
		self.heightConstraint.active = NO;
		[self.view removeConstraint:self.heightConstraint];
	}
	if (nil != self.widthConstraintMax)
	{
		self.widthConstraintMax.active = NO;
		[self.view removeConstraint:self.widthConstraintMax];
	}
	if (nil != self.widthConstraintMin)
	{
		self.widthConstraintMin.active = NO;
		[self.view removeConstraint:self.widthConstraintMin];
	}
	
	// constraints MUST be archived because toolbar items are
	// copied from the customization palette (via archiving)
	self.heightConstraint = [self.view.heightAnchor constraintEqualToConstant:idealSize.height];
	self.heightConstraint.active = YES;
	self.heightConstraint.shouldBeArchived = YES;
#if 0
	self.widthConstraintMax = [self.view.widthAnchor constraintLessThanOrEqualToConstant:2048];
	//self.widthConstraintMax = [self.view.widthAnchor constraintLessThanOrEqualToConstant:idealSize.width];
	self.widthConstraintMax.active = YES;
	self.widthConstraintMax.shouldBeArchived = YES;
#endif
	self.widthConstraintMin = [self.view.widthAnchor constraintGreaterThanOrEqualToConstant:60];
	//self.widthConstraintMin = [self.view.widthAnchor constraintGreaterThanOrEqualToConstant:idealSize.width];
	self.widthConstraintMin.active = YES;
	self.widthConstraintMin.shouldBeArchived = YES;
	// NOTE: see initializer, "minSize" and "maxSize" are still set
	// because “flexible width” does not seem to work otherwise
	
	[self.view setContentHuggingPriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationHorizontal];
	[self.view setContentHuggingPriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationVertical];
	[self.view setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationHorizontal];
	[self.view setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationVertical];
}// updateSizeConstraints


@end //} TerminalToolbar_ItemWindowTitle (TerminalToolbar_ItemWindowTitleInternal)


#pragma mark -
@implementation TerminalToolbar_ItemWindowTitleLeft //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		self.textView.labelLayout = kTerminalToolbar_TextLabelLayoutLeftJustified;
		self.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleLeftIconFilenameNoExtension(), NSString*)]; // TEMPORARY
		self.paletteLabel = NSLocalizedString(@"Left-Aligned Title", @"toolbar item name; for left-aligned window title");
	}
	return self;
}// initWithItemIdentifier:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id										result = [super copyWithZone:zone];
	TerminalToolbar_ItemWindowTitleLeft*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowTitleLeft.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowTitleLeft*);
	// (nothing needed)
	
	return result;
}// copyWithZone:


#pragma mark TerminalToolbar_ItemHasPaletteProxy


/*!
Returns the item that represents this item in a customization palette.

(2018.03)
*/
- (NSToolbarItem*)
paletteProxyToolbarItemWithIdentifier:(NSString*)	anIdentifier
{
	NSToolbarItem*		result = [[NSToolbarItem alloc] initWithItemIdentifier:anIdentifier];
	
	
	result.paletteLabel = self.paletteLabel;
	result.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleLeftIconFilenameNoExtension(), NSString*)];
	
	return result;
}// paletteProxyToolbarItemWithIdentifier:


@end //} TerminalToolbar_ItemWindowTitleLeft


#pragma mark -
@implementation TerminalToolbar_ItemWindowTitleRight //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		self.textView.labelLayout = kTerminalToolbar_TextLabelLayoutRightJustified;
		self.paletteLabel = NSLocalizedString(@"Right-Aligned Title", @"toolbar item name; for right-aligned window title");
	}
	return self;
}// initWithItemIdentifier:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id										result = [super copyWithZone:zone];
	TerminalToolbar_ItemWindowTitleRight*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowTitleRight.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowTitleRight*);
	// (nothing needed)
	
	return result;
}// copyWithZone:


#pragma mark TerminalToolbar_ItemHasPaletteProxy


/*!
Returns the item that represents this item in a customization palette.

(2018.03)
*/
- (NSToolbarItem*)
paletteProxyToolbarItemWithIdentifier:(NSString*)	anIdentifier
{
	NSToolbarItem*		result = [[NSToolbarItem alloc] initWithItemIdentifier:anIdentifier];
	
	
	result.paletteLabel = self.paletteLabel;
	result.image = [NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleRightIconFilenameNoExtension(), NSString*)];
	
	return result;
}// paletteProxyToolbarItemWithIdentifier:


@end //} TerminalToolbar_ItemWindowTitleRight


#pragma mark -
@implementation TerminalToolbar_Object //{


@synthesize titleJustification = _titleJustification;


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithIdentifier:(NSString*)	anIdentifier
{
	self = [super initWithIdentifier:anIdentifier];
	if (nil != self)
	{
		_titleJustification = NSTextAlignmentCenter;
	}
	return self;
}// initWithIdentifier:


#pragma mark NSToolbar


/*!
Notifies of changes to the toolbar icon and/or label display
so the window layout can be updated.  You should observe
"kTerminalToolbar_ObjectDidChangeDisplayModeNotification" to
detect when this is invoked.

(4.0)
*/
- (void)
setDisplayMode:(NSToolbarDisplayMode)	displayMode
{
	[super setDisplayMode:displayMode];
	[self postNote:kTerminalToolbar_ObjectDidChangeDisplayModeNotification];
}// setDisplayMode:


/*!
Notifies of changes to the toolbar icon and/or label size
so the window layout can be updated.  You should observe
"kTerminalToolbar_ObjectDidChangeSizeModeNotification" to
detect when this is invoked.

(4.0)
*/
- (void)
setSizeMode:(NSToolbarSizeMode)		sizeMode
{
	[super setSizeMode:sizeMode];
	[self postNote:kTerminalToolbar_ObjectDidChangeSizeModeNotification];
}// setSizeMode:


/*!
Notifies of changes to the toolbar visibility.  Observe
"kTerminalToolbar_ObjectDidChangeVisibilityNotification"
to detect when this is invoked with a value that is
different from the previous visibility state.

(4.0)
*/
- (void)
setVisible:(BOOL)	isVisible
{
	BOOL	fireNotification = (isVisible != [self isVisible]);
	
	
	[super setVisible:isVisible];
	if (fireNotification)
	{
		[self postNote:kTerminalToolbar_ObjectDidChangeVisibilityNotification];
	}
}// setVisible:


@end //} TerminalToolbar_Object


#pragma mark -
@implementation TerminalToolbar_SessionDependentItem //{


@synthesize sessionHint = _sessionHint;


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		_sessionHint = nullptr;
	}
	return self;
}// initWithItemIdentifier:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self removeSessionDependentItemNotificationHandlersForToolbar:self.toolbar];
}// dealloc


#pragma mark Accessors


/*!
Convenience method for finding the session currently associated
with the toolbar that contains this item (may be "nullptr").

If the item has no toolbar yet, "setSessionHint:" can be used
to influence this return value.

(4.0)
*/
- (SessionRef)
session
{
	SessionRef		result = self.toolbar.terminalToolbarSession;
	
	
	if (nullptr == result)
	{
		result = _sessionHint;
	}
	else
	{
		self.sessionHint = nullptr;
	}
	
	return result;
}// session


// (See description in class interface.)
- (SessionRef)
sessionHint
{
	return _sessionHint;
}
- (void)
setSessionHint:(SessionRef)		aSession
{
	_sessionHint = aSession;
}// setSessionHint


/*!
Convenience method for finding the terminal screen buffer of
the view that is currently focused in the terminal window of
the toolbar’s associated session, if any (may be "nullptr").

(4.0)
*/
- (TerminalScreenRef)
terminalScreen
{
	TerminalWindowRef	terminalWindow = self.terminalWindow;
	TerminalScreenRef	result = (nullptr != terminalWindow)
									? TerminalWindow_ReturnScreenWithFocus(terminalWindow)
									: nullptr;
	
	
	return result;
}// terminalScreen


/*!
Convenience method for finding the terminal view that is
currently focused in the terminal window of the toolbar’s
associated session, if any (may be "nullptr").

(4.0)
*/
- (TerminalViewRef)
terminalView
{
	TerminalWindowRef	terminalWindow = self.terminalWindow;
	TerminalViewRef		result = (nullptr != terminalWindow)
									? TerminalWindow_ReturnViewWithFocus(terminalWindow)
									: nullptr;
	
	
	return result;
}// terminalView


/*!
Convenience method for finding the terminal window that is
currently active in the toolbar’s associated session, if any
(may be "nullptr").

(4.0)
*/
- (TerminalWindowRef)
terminalWindow
{
	SessionRef			session = self.session;
	TerminalWindowRef	result = (Session_IsValid(session))
									? Session_ReturnActiveTerminalWindow(session)
									: nullptr;
	
	
	return result;
}// terminalWindow


#pragma mark Subclass Overrides


/*!
Subclasses should override this to respond when the session
for the item has changed.

(4.0)
*/
- (void)
didChangeSession
{
}// didChangeSession


/*!
Subclasses should override this to respond when the session
for the item is about to change.

(4.0)
*/
- (void)
willChangeSession
{
}// willChangeSession


#pragma mark Notifications


/*!
Triggered by the toolbar’s delegate whenever the associated
session has changed.  The toolbar item should respond by
updating its state.

For convenience, this just invokes another method with more
direct parameters; subclasses should override only that
method.

(4.0)
*/
- (void)
sessionDidChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self didChangeSession];
}// sessionDidChange:


/*!
Triggered by the toolbar’s delegate whenever the associated
session is about to change.  The toolbar item should respond
by undoing any setup it might have done to handle a previous
session’s state.

For convenience, this just invokes another method with more
direct parameters; subclasses should override only that
method.

(4.0)
*/
- (void)
sessionWillChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	[self willChangeSession];
}// sessionWillChange:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id										result = [super copyWithZone:zone];
	TerminalToolbar_SessionDependentItem*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_SessionDependentItem.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_SessionDependentItem*);
	asSelf.sessionHint = self.sessionHint;
	
	return result;
}// copyWithZone:


#pragma mark NSToolbarItem


/*!
Returns YES only if the item should be enabled.  By default
this is the case as long as the associated session is valid.

(4.0)
*/
- (BOOL)
validateToolbarItem:(NSToolbarItem*)	anItem
{
#pragma unused(anItem)
	SessionRef	session = [self session];
	BOOL		result = (true == Session_IsValid(session));
	
	
	return result;
}// validateToolbarItem:


#pragma mark TerminalToolbar_ItemAddRemoveSensitive


/*!
Called when the given item is about to be added to the
specified toolbar.

(2018.03)
*/
- (void)
item:(NSToolbarItem*)			anItem
willEnterToolbar:(NSToolbar*)	aToolbar
{
	assert(self == anItem);
	[self installSessionDependentItemNotificationHandlersForToolbar:aToolbar];
}// item:willEnterToolbar:


/*!
Called when the specified item has been removed from
the specified toolbar.

(2018.03)
*/
- (void)
item:(NSToolbarItem*)			anItem
didExitToolbar:(NSToolbar*)		aToolbar
{
	assert(self == anItem);
	[self removeSessionDependentItemNotificationHandlersForToolbar:aToolbar];
}// item:didExitToolbar:


@end //} TerminalToolbar_SessionDependentItem


#pragma mark -
@implementation TerminalToolbar_SessionDependentItem (TerminalToolbar_SessionDependentItemInternal) //{


#pragma mark New Methods


/*!
This only needs to be called by initializers
and "copyWithZone:".

(2018.03)
*/
- (void)
installSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)		aToolbar
{
	[self whenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionWillChangeNotification
						performSelector:@selector(sessionWillChange:)];
	[self whenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionDidChangeNotification
						performSelector:@selector(sessionDidChange:)];
}// installSessionDependentItemNotificationHandlersForToolbar:


/*!
This only needs to be called by initializers
and "copyWithZone:".

(2018.03)
*/
- (void)
removeSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)		aToolbar
{
	[self ignoreWhenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionWillChangeNotification];
	[self ignoreWhenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionDidChangeNotification];
}// removeSessionDependentItemNotificationHandlersForToolbar:


@end //} TerminalToolbar_SessionDependentItem (TerminalToolbar_SessionDependentItemInternal)


#pragma mark -
@implementation TerminalToolbar_SessionDependentMenuItem //{


@synthesize sessionHint = _sessionHint;


#pragma mark Initializers


/*!
Designated initializer.

(2021.02)
*/
- (instancetype)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		_sessionHint = nullptr;
	}
	return self;
}// initWithItemIdentifier:


/*!
Destructor.

(2021.02)
*/
- (void)
dealloc
{
	[self removeSessionDependentItemNotificationHandlersForToolbar:self.toolbar];
}// dealloc


#pragma mark Accessors


/*!
Convenience method for finding the session currently associated
with the toolbar that contains this item (may be "nullptr").

If the item has no toolbar yet, "setSessionHint:" can be used
to influence this return value.

(2021.02)
*/
- (SessionRef)
session
{
	SessionRef		result = self.toolbar.terminalToolbarSession;
	
	
	if (nullptr == result)
	{
		result = _sessionHint;
	}
	else
	{
		self.sessionHint = nullptr;
	}
	
	return result;
}// session


// (See description in class interface.)
- (SessionRef)
sessionHint
{
	return _sessionHint;
}
- (void)
setSessionHint:(SessionRef)		aSession
{
	_sessionHint = aSession;
}// setSessionHint


/*!
Convenience method for finding the terminal screen buffer of
the view that is currently focused in the terminal window of
the toolbar’s associated session, if any (may be "nullptr").

(2021.02)
*/
- (TerminalScreenRef)
terminalScreen
{
	TerminalWindowRef	terminalWindow = self.terminalWindow;
	TerminalScreenRef	result = (nullptr != terminalWindow)
									? TerminalWindow_ReturnScreenWithFocus(terminalWindow)
									: nullptr;
	
	
	return result;
}// terminalScreen


/*!
Convenience method for finding the terminal view that is
currently focused in the terminal window of the toolbar’s
associated session, if any (may be "nullptr").

(2021.02)
*/
- (TerminalViewRef)
terminalView
{
	TerminalWindowRef	terminalWindow = self.terminalWindow;
	TerminalViewRef		result = (nullptr != terminalWindow)
									? TerminalWindow_ReturnViewWithFocus(terminalWindow)
									: nullptr;
	
	
	return result;
}// terminalView


/*!
Convenience method for finding the terminal window that is
currently active in the toolbar’s associated session, if any
(may be "nullptr").

(2021.02)
*/
- (TerminalWindowRef)
terminalWindow
{
	SessionRef			session = self.session;
	TerminalWindowRef	result = (Session_IsValid(session))
									? Session_ReturnActiveTerminalWindow(session)
									: nullptr;
	
	
	return result;
}// terminalWindow


#pragma mark Subclass Overrides


/*!
Subclasses should override this to respond when the session
for the item has changed.

(2021.02)
*/
- (void)
didChangeSession
{
}// didChangeSession


/*!
Subclasses should override this to respond when the session
for the item is about to change.

(2021.02)
*/
- (void)
willChangeSession
{
}// willChangeSession


#pragma mark Notifications


/*!
Triggered by the toolbar’s delegate whenever the associated
session has changed.  The toolbar item should respond by
updating its state.

For convenience, this just invokes another method with more
direct parameters; subclasses should override only that
method.

(2021.02)
*/
- (void)
sessionDidChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self didChangeSession];
}// sessionDidChange:


/*!
Triggered by the toolbar’s delegate whenever the associated
session is about to change.  The toolbar item should respond
by undoing any setup it might have done to handle a previous
session’s state.

For convenience, this just invokes another method with more
direct parameters; subclasses should override only that
method.

(2021.02)
*/
- (void)
sessionWillChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	[self willChangeSession];
}// sessionWillChange:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2021.02)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id											result = [super copyWithZone:zone];
	TerminalToolbar_SessionDependentMenuItem*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_SessionDependentMenuItem.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_SessionDependentMenuItem*);
	asSelf.sessionHint = self.sessionHint;
	
	return result;
}// copyWithZone:


#pragma mark TerminalToolbar_ItemAddRemoveSensitive


/*!
Called when the given item is about to be added to the
specified toolbar.

(2021.02)
*/
- (void)
item:(NSToolbarItem*)			anItem
willEnterToolbar:(NSToolbar*)	aToolbar
{
	assert(self == anItem);
	[self installSessionDependentItemNotificationHandlersForToolbar:aToolbar];
}// item:willEnterToolbar:


/*!
Called when the specified item has been removed from
the specified toolbar.

(2021.02)
*/
- (void)
item:(NSToolbarItem*)			anItem
didExitToolbar:(NSToolbar*)		aToolbar
{
	assert(self == anItem);
	[self removeSessionDependentItemNotificationHandlersForToolbar:aToolbar];
}// item:didExitToolbar:


@end //} TerminalToolbar_SessionDependentMenuItem


#pragma mark -
@implementation TerminalToolbar_SessionDependentMenuItem (TerminalToolbar_SessionDependentMenuItemInternal) //{


#pragma mark New Methods


/*!
This only needs to be called by initializers
and "copyWithZone:".

(2021.02)
*/
- (void)
installSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)		aToolbar
{
	[self whenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionWillChangeNotification
						performSelector:@selector(sessionWillChange:)];
	[self whenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionDidChangeNotification
						performSelector:@selector(sessionDidChange:)];
}// installSessionDependentItemNotificationHandlersForToolbar:


/*!
This only needs to be called by initializers
and "copyWithZone:".

(2021.02)
*/
- (void)
removeSessionDependentItemNotificationHandlersForToolbar:(NSToolbar*)		aToolbar
{
	[self ignoreWhenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionWillChangeNotification];
	[self ignoreWhenObject:[aToolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionDidChangeNotification];
}// removeSessionDependentItemNotificationHandlersForToolbar:


@end //} TerminalToolbar_SessionDependentMenuItem (TerminalToolbar_SessionDependentMenuItemInternal)


#pragma mark -
@implementation TerminalToolbar_Window //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithContentRect:(NSRect)	aRect
styleMask:(NSWindowStyleMask)	aStyleMask
backing:(NSBackingStoreType)	aBufferingType
defer:(BOOL)					aDeferFlag
{
	self = [super initWithContentRect:aRect styleMask:aStyleMask backing:aBufferingType defer:aDeferFlag];
	if (nil != self)
	{
		self.isDisplayingSheet = NO;
		
		// create toolbar
		{
			NSString*					toolbarID = @"TerminalToolbar"; // do not ever change this; that would only break user preferences
			TerminalToolbar_Object*		windowToolbar = [[TerminalToolbar_Object alloc] initWithIdentifier:toolbarID];
			
			
			self.toolbarDelegate = [[TerminalToolbar_Delegate alloc] initForToolbar:windowToolbar experimentalItems:NO];
			windowToolbar.allowsUserCustomization = YES;
			windowToolbar.autosavesConfiguration = YES;
			windowToolbar.delegate = self.toolbarDelegate;
			[self whenObject:windowToolbar postsNote:kTerminalToolbar_ObjectDidChangeDisplayModeNotification
								performSelector:@selector(toolbarDidChangeDisplayMode:)];
			[self whenObject:windowToolbar postsNote:kTerminalToolbar_ObjectDidChangeSizeModeNotification
								performSelector:@selector(toolbarDidChangeSizeMode:)];
			[self whenObject:windowToolbar postsNote:kTerminalToolbar_ObjectDidChangeVisibilityNotification
								performSelector:@selector(toolbarDidChangeVisibility:)];
			self.toolbar = windowToolbar;
		}
		
		// monitor the active session, but also initialize the value
		// (the callback should update the value in all other cases)
		self.toolbarDelegate.session = SessionFactory_ReturnUserRecentSession();
		self.sessionFactoryChangeListener = [[ListenerModel_StandardListener alloc]
												initWithTarget:self
																eventFiredSelector:
																@selector(model:sessionFactoryChange:context:)];
		SessionFactory_StartMonitoring(kSessionFactory_ChangeActivatingSession,
										self.sessionFactoryChangeListener.listenerRef);
		SessionFactory_StartMonitoring(kSessionFactory_ChangeDeactivatingSession,
										self.sessionFactoryChangeListener.listenerRef);
		
		// panel properties
		self.becomesKeyOnlyIfNeeded = YES;
		self.floatingPanel = YES;
		self.worksWhenModal = NO;
		
		// window properties
		self.hasShadow = NO;
		self.hidesOnDeactivate = YES;
		self.level = NSNormalWindowLevel;
		self.movableByWindowBackground = YES;
		self.releasedWhenClosed = NO;
		[self standardWindowButton:NSWindowCloseButton].hidden = YES;
		[self standardWindowButton:NSWindowMiniaturizeButton].hidden = YES;
		[self standardWindowButton:NSWindowZoomButton].hidden = YES;
		[self standardWindowButton:NSWindowDocumentIconButton].hidden = YES;
		[self standardWindowButton:NSWindowToolbarButton].hidden = YES;
		
		// the toolbar is meant to apply to the active window so it
		// should be visible wherever the user is (even in a
		// Full Screen context)
		self.collectionBehavior = (NSWindowCollectionBehaviorMoveToActiveSpace |
									NSWindowCollectionBehaviorParticipatesInCycle);
		
		// size constraints; the window should have no content height
		// because it is only intended to display the window’s toolbar
		{
			NSSize	limitedSize = NSMakeSize(512, 0); // arbitrary
			
			
			self.contentMinSize = limitedSize;
			limitedSize.width = 2048; // arbitrary
			self.contentMaxSize = limitedSize;
		}
		
		// make the window fill the screen horizontally
		[self setFrameWithPossibleAnimation];
		
		// arrange to auto-zoom when certain window changes occur
		[self whenObject:NSApp postsNote:NSApplicationDidChangeScreenParametersNotification
							performSelector:@selector(applicationDidChangeScreenParameters:)];
		[self whenObject:self postsNote:NSWindowDidBecomeKeyNotification
							performSelector:@selector(windowDidBecomeKey:)];
		[self whenObject:self postsNote:NSWindowDidResignKeyNotification
							performSelector:@selector(windowDidResignKey:)];
		[self whenObject:self postsNote:NSWindowWillBeginSheetNotification
							performSelector:@selector(windowWillBeginSheet:)];
		[self whenObject:self postsNote:NSWindowDidEndSheetNotification
							performSelector:@selector(windowDidEndSheet:)];
		
		if (nil == gSharedTerminalToolbar)
		{
			gSharedTerminalToolbar = self;
		}
	}
	return self;
}// initWithContentRect:screen:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	if (self == gSharedTerminalToolbar)
	{
		gSharedTerminalToolbar = nil;
	}
	if (nullptr != self.sessionFactoryChangeListener)
	{
		SessionFactory_StopMonitoring(kSessionFactory_ChangeActivatingSession,
										self.sessionFactoryChangeListener.listenerRef);
		SessionFactory_StopMonitoring(kSessionFactory_ChangeDeactivatingSession,
										self.sessionFactoryChangeListener.listenerRef);
	}
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark NSWindow


/*!
Overrides the base to prevent the window from moving away
from the edge of its screen.

Either the top or bottom edge is chosen (since a toolbar
is not really designed to arrange items vertically).

Ultimately, the only part of "frameRect" that is going to
be used is one dimension of the size, to set the thickness
of the window along its chosen edge of the screen.

NOTE:	A special exception is made while the window’s
		toolbar is in customization mode, because the
		system might need to move the window in such a
		way as to make room for the customization sheet.

(4.0)
*/
- (NSRect)
constrainFrameRect:(NSRect)		frameRect
toScreen:(NSScreen*)			aScreen
{
	NSRect		result = [super constrainFrameRect:frameRect toScreen:aScreen];
	
	
	// constraints must be disabled when the customization sheet is open
	// so that the window can move (if necessary) to reveal the sheet
	unless (self.isDisplayingSheet)
	{
		NSScreen*	targetScreen = (nil != aScreen)
									? aScreen
									: ((nil != [self screen])
										? [self screen]
										: [NSScreen mainScreen]);
		NSRect		screenFullRect = [targetScreen frame];
		NSRect		screenInteriorRect = [targetScreen visibleFrame];
		
		
		if (result.origin.y < (screenInteriorRect.origin.y + (screenInteriorRect.size.height / 2.0)))
		{
			result.origin.y = screenInteriorRect.origin.y;
		}
		else
		{
			result.origin.y = screenInteriorRect.origin.y + screenInteriorRect.size.height - result.size.height;
		}
		
		// do not obey the system’s behavior for side-Dock placement
		// because this just creates awkward gaps that are almost
		// certainly not covered by anything
		result.origin.x = screenFullRect.origin.x;
		result.size.width = screenFullRect.size.width;
	}
	
	if (result.size.height < 1/* arbitrary */)
	{
		// for some reason on older Mac OS X versions, it is possible
		// that the frame itself will have a height of zero (as opposed
		// to just the content rectangle, where this is expected); to
		// prevent the window from being invisible, force nonzero height
		result.size.height = 1;
	}
	
	return result;
}// constrainFrameRect:toScreen:


/*!
Used only to record when animation has been requested,
so that the window constraints are disabled.

(4.0)
*/
- (void)
setFrame:(NSRect)	aRect
display:(BOOL)		displayFlag
animate:(BOOL)		animateFlag
{
	[super setFrame:aRect display:displayFlag animate:animateFlag];
}// setFrame:display:animate:


@end //} TerminalToolbar_Window


#pragma mark -
@implementation TerminalToolbar_Window (TerminalToolbar_WindowInternal) //{


#pragma mark Class Methods


/*!
Requests that all terminal toolbar windows check their frames
and see if they need to move (to a screen edge).

Currently, this will only notify the first toolbar window
that was ever created.

NOTE:	Most frame changes are triggered per-instance by
		notifications.  This routine is only used in rare
		cases where normal notifications would not work.

(4.0)
*/
+ (void)
refreshWindows
{
	[gSharedTerminalToolbar setFrameWithPossibleAnimation];
}// refreshWindows


#pragma mark Methods of the Form Required by ListenerModel_StandardListener


/*!
Called when a monitored session factory event occurs.  See the
initializer for the set of events that is monitored.

(4.0)
*/
- (void)
model:(ListenerModel_Ref)					aModel
sessionFactoryChange:(ListenerModel_Event)	anEvent
context:(void*)								aContext
{
#pragma unused(aModel)
	switch (anEvent)
	{
	case kSessionFactory_ChangeActivatingSession:
		{
			SessionRef		session = REINTERPRET_CAST(aContext, SessionRef);
			
			
			//Console_WriteValueAddress("terminal toolbar window: session becoming key", session);
			
			// notify delegate, which will in turn fire notifications that
			// session-dependent toolbar items are observing
			self.toolbar.terminalToolbarDelegate.session = session;
		}
		break;
	
	case kSessionFactory_ChangeDeactivatingSession:
		{
			//SessionRef		session = REINTERPRET_CAST(aContext, SessionRef);
			
			
			//Console_WriteValueAddress("terminal toolbar window: session resigning key", session);
		
			// notify delegate, which will in turn fire notifications that
			// session-dependent toolbar items are observing
			self.toolbar.terminalToolbarDelegate.session = nil;
		}
		break;
	
	default:
		// ???
		Console_Warning(Console_WriteValueFourChars,
						"terminal toolbar window: session factory change handler received unexpected event", anEvent);
		break;
	}
}// model:sessionFactoryChange:context:


#pragma mark New Methods


/*!
Sets the frame of the toolbar window to the ideal frame,
which will be “as wide as possible, anchored to screen edge”
(top or bottom).  The chosen screen edge will be whichever
edge is closest to the given rectangle’s origin point.

Animation is implicit based on how far the window must move
to reach its ideal frame.

Only one dimension of the given rectangle’s size will be
used (e.g. to set the window height).

(4.0)
*/
- (void)
setFrameBasedOnRect:(NSRect)	aRect
{
	BOOL		animateFlag = NO;
	CGFloat		oldBase = [self frame].origin.y;
	
	
	aRect = [self constrainFrameRect:aRect toScreen:[self screen]];
	
	// animation depends on how far the new frame is from the old one
	if (fabs(oldBase - aRect.origin.y) > 88/* arbitrary */)
	{
		animateFlag = YES;
	}
	
	[self setFrame:aRect display:NO animate:animateFlag];
}// setFrameBasedOnRect:


/*!
Sets the frame of the toolbar window to the ideal size,
which will be “as wide as possible, anchored to screen edge”
(top or bottom).

(4.0)
*/
- (void)
setFrameWithPossibleAnimation
{
	[self setFrameBasedOnRect:self.frame];
}// setFrameWithPossibleAnimation


#pragma mark NSApplicationNotifications


/*!
Auto-zooms the window in response to a change of screen
so that it tends to stay in the correct position (at
the top or bottom of the display).

(4.0)
*/
- (void)
applicationDidChangeScreenParameters:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	[self setFrameWithPossibleAnimation];
}// applicationDidChangeScreenParameters:


#pragma mark NSWindowNotifications


/*!
Shows the toolbar if it is hidden when the window gains
the user focus.

This balances the fact that any attempt to hide the
toolbar will hide the window.

(4.0)
*/
- (void)
windowDidBecomeKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	if (NO == self.toolbar.isVisible)
	{
		self.toolbar.visible = YES;
	}
}// windowDidBecomeKey:


/*!
Forces the toolbar to remain visible.

(4.0)
*/
- (void)
windowDidResignKey:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	if (NO == self.toolbar.isVisible)
	{
		self.toolbar.visible = YES;
	}
}// windowDidResignKey:


/*!
Keeps track of sheets being displayed, so that a
temporary exception to resizing rules can be made.
If this were not tracked then certain toolbar
arrangements would produce customization sheets
that are off the screen.

(4.0)
*/
- (void)
windowWillBeginSheet:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	self.isDisplayingSheet = YES;
}// windowWillBeginSheet:


/*!
Keeps track of sheets being removed, so that the
frame can be restored properly.

(4.0)
*/
- (void)
windowDidEndSheet:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	if (self.isDisplayingSheet)
	{
		self.isDisplayingSheet = NO;
		[self setFrameWithPossibleAnimation];
	}
}// windowDidEndSheet:


#pragma mark TerminalToolbar_Object Notifications


/*!
Keeps track of changes to the icon and label display
so that the window layout can adapt if necessary.

(4.0)
*/
- (void)
toolbarDidChangeDisplayMode:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self setFrameWithPossibleAnimation];
}// toolbarDidChangeDisplayMode:


/*!
Keeps track of changes to the icon and label size
so that the window layout can adapt if necessary.

(4.0)
*/
- (void)
toolbarDidChangeSizeMode:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	[self setFrameWithPossibleAnimation];
}// toolbarDidChangeSizeMode:


/*!
Keeps track of changes to the toolbar’s visibility
so that the window layout can adapt if necessary.

(4.0)
*/
- (void)
toolbarDidChangeVisibility:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	[self setFrameWithPossibleAnimation];
	
	if (NO == self.toolbar.isVisible)
	{
		// if the toolbar is being hidden, just hide the entire toolbar window
		[self orderOut:nil];
	}
}// toolbarDidChangeVisibility:


@end //} TerminalToolbar_Window (TerminalToolbar_WindowInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
