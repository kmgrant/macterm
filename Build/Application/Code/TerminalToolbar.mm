/*!	\file TerminalToolbar.mm
	\brief Items used in the toolbars of terminal windows.
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
NSString*	kMy_ToolbarItemIDPrint						= @"net.macterm.MacTerm.toolbaritem.print";
NSString*	kMy_ToolbarItemIDSuspend					= @"net.macterm.MacTerm.toolbaritem.suspend";
NSString*	kMy_ToolbarItemIDTabs						= @"net.macterm.MacTerm.toolbaritem.tabs";
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
	@property (strong) NSButton*
	button;
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
@interface TerminalToolbar_TextLabel () //{

// accessors
	@property (assign) BOOL
	disableFrameMonitor;
	@property (assign) BOOL
	frameDisplayEnabled;
	@property (assign) BOOL
	gradientFadeEnabled;

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
Private properties.
*/
@interface TerminalToolbar_WindowTitleLabel () //{

// accessors
	@property (strong) CocoaExtensions_ObserverSpec*
	windowTitleObserver;

@end //}

/*!
Private properties.
*/
@interface TerminalToolbar_ItemWindowTitle () //{

// accessors
	@property (assign) BOOL
	disableFrameMonitor;
	@property (strong) TerminalToolbar_WindowTitleLabel*
	textView;

@end //}

/*!
The private class interface.
*/
@interface TerminalToolbar_ItemWindowTitle (TerminalToolbar_ItemWindowTitleInternal) //{

// new methods
	- (void)
	resetMinMaxSizesForHeight:(CGFloat)_;
	- (void)
	setStateForToolbar:(NSToolbar*)_;

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
@interface TerminalToolbar_Object () //{

// accessors
	@property (assign) NSTextAlignment
	titleJustification;

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
		self->associatedSession = nullptr;
		self->allowExperimentalItems = experimentalFlag;
		
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
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

The session associated with the toolbar delegate can be read by
certain toolbar items in order to maintain their states.

(4.0)
*/
- (SessionRef)
session
{
	return self->associatedSession;
}
- (void)
setSession:(SessionRef)		aSession
{
	if (self->associatedSession != aSession)
	{
		[self postNote:kTerminalToolbar_DelegateSessionWillChangeNotification];
		self->associatedSession = aSession;
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
		result = [[[TerminalToolbar_ItemBell alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDCustomize])
	{
		result = [[[TerminalToolbar_ItemCustomize alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDForceQuit])
	{
		result = [[[TerminalToolbar_ItemForceQuit alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDFullScreen])
	{
		// this is redundant with the abilities of "TerminalToolbar_ItemWindowButtonZoom"
		// but it is more convenient, especially when exiting Full Screen (also, the
		// “off switch” option has been removed, this is effectively its replacement)
		result = [[[TerminalToolbar_ItemFullScreen alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDHide])
	{
		result = [[[TerminalToolbar_ItemHide alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED1])
	{
		result = [[[TerminalToolbar_ItemLED1 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED2])
	{
		result = [[[TerminalToolbar_ItemLED2 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED3])
	{
		result = [[[TerminalToolbar_ItemLED3 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED4])
	{
		result = [[[TerminalToolbar_ItemLED4 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDNewSessionDefaultFavorite])
	{
		result = [[[TerminalToolbar_ItemNewSessionDefaultFavorite alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDNewSessionLogInShell])
	{
		result = [[[TerminalToolbar_ItemNewSessionLogInShell alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDNewSessionShell])
	{
		result = [[[TerminalToolbar_ItemNewSessionShell alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDPrint])
	{
		result = [[[TerminalToolbar_ItemPrint alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDStackWindows])
	{
		result = [[[TerminalToolbar_ItemStackWindows alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDSuspend])
	{
		result = [[[TerminalToolbar_ItemSuspend alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDTabs])
	{
		result = [[[TerminalToolbar_ItemTabs alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowButtonClose])
	{
		result = [[[TerminalToolbar_ItemWindowButtonClose alloc] initWithItemIdentifier:itemIdentifier] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowButtonMinimize])
	{
		result = [[[TerminalToolbar_ItemWindowButtonMinimize alloc] initWithItemIdentifier:itemIdentifier] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowButtonZoom])
	{
		result = [[[TerminalToolbar_ItemWindowButtonZoom alloc] initWithItemIdentifier:itemIdentifier] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitle])
	{
		result = [[[TerminalToolbar_ItemWindowTitle alloc] initWithItemIdentifier:itemIdentifier] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleLeft])
	{
		result = [[[TerminalToolbar_ItemWindowTitleLeft alloc] initWithItemIdentifier:itemIdentifier] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDWindowTitleRight])
	{
		result = [[[TerminalToolbar_ItemWindowTitleRight alloc] initWithItemIdentifier:itemIdentifier] autorelease];
	}
	
	// initialize session information if available (this isn’t really
	// necessary for new toolbars full of items; it is only important
	// when the user customizes the toolbar, to ensure that new items
	// immediately display appropriate data from the target session)
	if ((willBeInToolbar) && [result isKindOfClass:TerminalToolbar_SessionDependentItem.class])
	{
		TerminalToolbar_SessionDependentItem*	asSessionItem = STATIC_CAST(result, TerminalToolbar_SessionDependentItem*);
		
		
		[asSessionItem setSessionHint:self.session];
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
	
	
	if (self->allowExperimentalItems)
	{
		//[result addObject:kMy_ToolbarItemIDTabs];
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
									kMy_ToolbarItemIDFullScreen,
									kMy_ToolbarItemIDWindowTitleLeft,
									kMy_ToolbarItemIDWindowTitle,
									kMy_ToolbarItemIDWindowTitleRight,
									kMy_ToolbarItemIDPrint,
									kMy_ToolbarItemIDHide,
									kMy_ToolbarItemIDForceQuit,
									kMy_ToolbarItemIDSuspend,
									kMy_ToolbarItemIDBell,
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
				//Console_Warning(Console_WriteValueAddress, "removed item does not implement 'TerminalToolbar_ItemAddRemoveSensitive'", asToolbarItem); // debug
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
				//Console_Warning(Console_WriteValueAddress, "added item does not implement 'TerminalToolbar_ItemAddRemoveSensitive'", asToolbarItem); // debug
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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setLabel:NSLocalizedString(@"Bell", @"toolbar item name; for toggling the terminal bell sound")];
		[self setPaletteLabel:[self label]];
		self->screenChangeListener = [[ListenerModel_StandardListener alloc]
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
	[screenChangeListener release];
	[super dealloc];
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
	asSelf->screenChangeListener = [self->screenChangeListener copy];
	[asSelf->screenChangeListener setTarget:asSelf];
	
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
		Terminal_StopMonitoring(screen, kTerminal_ChangeAudioState, [self->screenChangeListener listenerRef]);
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
		Terminal_StartMonitoring(screen, kTerminal_ChangeAudioState, [self->screenChangeListener listenerRef]);
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
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnBellOffIconFilenameNoExtension()]];
	}
	else
	{
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnBellOnIconFilenameNoExtension()]];
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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnCustomizeToolbarIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Customize", @"toolbar item name; for customizing the toolbar")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		self->sessionChangeListener = [[ListenerModel_StandardListener alloc]
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
	[sessionChangeListener release];
	[super dealloc];
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
	asSelf->sessionChangeListener = [self->sessionChangeListener copy];
	[asSelf->sessionChangeListener setTarget:asSelf];
	
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
	
	SessionRef	session = [self session];
	
	
	if (Session_IsValid(session))
	{
		Session_StopMonitoring(session, kSession_ChangeState, [self->sessionChangeListener listenerRef]);
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
	
	SessionRef	session = [self session];
	
	
	[self setStateFromSession:session];
	if (Session_IsValid(session))
	{
		Session_StartMonitoring(session, kSession_ChangeState, [self->sessionChangeListener listenerRef]);
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
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnRestartSessionIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Restart", @"toolbar item name; for killing or restarting the active process")];
		[self setPaletteLabel:[self label]];
	}
	else
	{
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnKillSessionIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Force Quit", @"toolbar item name; for killing or restarting the active process")];
		[self setPaletteLabel:[self label]];
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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnFullScreenIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Full Screen", @"toolbar item name; for entering or exiting Full Screen mode")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnHideWindowIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Hide", @"toolbar item name; for hiding the frontmost window")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setLabel:NSLocalizedString(@"L1", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setLabel:NSLocalizedString(@"L2", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setLabel:NSLocalizedString(@"L3", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setLabel:NSLocalizedString(@"L4", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnNewSessionDefaultIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Default", @"toolbar item name; for opening Default session")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnNewSessionLogInShellIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Log-In Shell", @"toolbar item name; for opening log-in shells")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnNewSessionShellIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Shell", @"toolbar item name; for opening shells")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnPrintIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Print", @"toolbar item name; for printing")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnStackWindowsIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Arrange in Front", @"toolbar item name; for stacking windows")];
		[self setPaletteLabel:[self label]];
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
	[super dealloc];
}// dealloc


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
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setLabel:NSLocalizedString(@"Suspend (Scroll Lock)", @"toolbar item name; for suspending the running process")];
		[self setPaletteLabel:[self label]];
		self->sessionChangeListener = [[ListenerModel_StandardListener alloc]
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
	[sessionChangeListener release];
	[super dealloc];
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
	asSelf->sessionChangeListener = [self->sessionChangeListener copy];
	[asSelf->sessionChangeListener setTarget:asSelf];
	
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
	
	SessionRef	session = [self session];
	
	
	if (Session_IsValid(session))
	{
		Session_StopMonitoring(session, kSession_ChangeStateAttributes, [self->sessionChangeListener listenerRef]);
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
	
	SessionRef	session = [self session];
	
	
	[self setStateFromSession:session];
	if (Session_IsValid(session))
	{
		Session_StartMonitoring(session, kSession_ChangeStateAttributes, [self->sessionChangeListener listenerRef]);
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
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnScrollLockOffIconFilenameNoExtension()]];
	}
	else
	{
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnScrollLockOnIconFilenameNoExtension()]];
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
		self->indexOfLED = anIndex;
		self->screenChangeListener = [[ListenerModel_StandardListener alloc]
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
	[screenChangeListener release];
	[super dealloc];
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
	asSelf->indexOfLED = self->indexOfLED;
	asSelf->screenChangeListener = [self->screenChangeListener copy];
	[asSelf->screenChangeListener setTarget:asSelf];
	
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
		Terminal_StopMonitoring(screen, kTerminal_ChangeNewLEDState, [self->screenChangeListener listenerRef]);
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
	
	TerminalScreenRef	screen = [self terminalScreen];
	
	
	if (nullptr != screen)
	{
		[self setStateFromScreen:screen];
		Terminal_StartMonitoring(screen, kTerminal_ChangeNewLEDState, [self->screenChangeListener listenerRef]);
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
	if ((nullptr == aScreen) || (false == Terminal_LEDIsOn(aScreen, STATIC_CAST(self->indexOfLED, SInt16))))
	{
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
	}
	else
	{
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOnIconFilenameNoExtension()]];
	}
}// setStateFromScreen:


@end //} TerminalToolbar_LEDItem (TerminalToolbar_LEDItemInternal)


#pragma mark -
@implementation TerminalToolbar_TabSource //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithDescription:(NSAttributedString*)	aDescription
{
	self = [super init];
	if (nil != self)
	{
		self->description = [aDescription copy];
	}
	return self;
}// initWithDescription:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[description release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
The attributed string describing the title of the tab; this
is used in the segmented control as well as any overflow menu.

(4.0)
*/
- (NSAttributedString*)
attributedDescription
{
	return description;
}// attributedDescription


/*!
The tooltip that is displayed when the mouse points to the
tab’s segment in the toolbar.

(4.0)
*/
- (NSString*)
toolTip
{
	return @"";
}// toolTip


#pragma mark Actions


/*!
Performs the action for this tab (namely, to bring something
to the front).  The sender will be an instance of the
"TerminalToolbar_ItemTabs" class.

(4.0)
*/
- (void)
performAction:(id) sender
{
#pragma unused(sender)
}// performAction:


@end //} TerminalToolbar_TabSource


#pragma mark -
@implementation TerminalToolbar_ItemTabs //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDTabs];
	if (nil != self)
	{
		self->segmentedControl = [[NSSegmentedControl alloc] initWithFrame:NSZeroRect];
		[self->segmentedControl setTarget:self];
		[self->segmentedControl setAction:@selector(performSegmentedControlAction:)];
		[self->segmentedControl setSegmentStyle:NSSegmentStyleTexturedSquare];
		
		//[self setAction:@selector(performToolbarItemAction:)];
		//[self setTarget:self];
		[self setEnabled:YES];
		[self setView:self->segmentedControl];
		[self setMinSize:NSMakeSize(120, 25)]; // arbitrary (height changes dynamically)
		[self setMaxSize:NSMakeSize(1024, 25)]; // arbitrary (height changes dynamically)
		[self setLabel:@""];
		[self setPaletteLabel:NSLocalizedString(@"Tabs", @"toolbar item name; for tabs")];
		
		[self setTabTargets:@[
								[[[TerminalToolbar_TabSource alloc]
									initWithDescription:[[[NSAttributedString alloc]
															initWithString:NSLocalizedString
																			(@"Tab 1", @"toolbar item tabs; default segment 0 name")]
															autorelease]] autorelease],
								[[[TerminalToolbar_TabSource alloc]
									initWithDescription:[[[NSAttributedString alloc]
															initWithString:NSLocalizedString
																			(@"Tab 2", @"toolbar item tabs; default segment 1 name")]
															autorelease]] autorelease],
							]
							andAction:nil];
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
	[self->segmentedControl release];
	[self->targets release];
	[super dealloc];
}// dealloc


#pragma mark Actions


/*!
Responds to an action in the menu of a toolbar item by finding
the corresponding object in the target array and making it
perform the action set by "setTabTargets:andAction:".

(4.0)
*/
- (void)
performMenuAction:(id)		sender
{
	if (nullptr != self->action)
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		NSMenu*			menu = [asMenuItem menu];
		
		
		if (nil != menu)
		{
			unsigned int const		kIndex = STATIC_CAST([menu indexOfItem:asMenuItem], unsigned int);
			
			
			if (kIndex < [self->targets count])
			{
				UNUSED_RETURN(id)[[self->targets objectAtIndex:kIndex] performSelector:self->action withObject:self];
			}
		}
	}
}// performMenuAction:


/*!
Responds to an action in a segmented control by finding the
corresponding object in the target array and making it perform
the action set by "setTabTargets:andAction:".

(4.0)
*/
- (void)
performSegmentedControlAction:(id)		sender
{
	if (nullptr != self->action)
	{
		if (self->segmentedControl == sender)
		{
			unsigned int const		kIndex = STATIC_CAST([self->segmentedControl selectedSegment], unsigned int);
			
			
			if (kIndex < [self->targets count])
			{
				UNUSED_RETURN(id)[[self->targets objectAtIndex:kIndex] performSelector:self->action withObject:self];
			}
		}
	}
}// performSegmentedControlAction:


#pragma mark New Methods


/*!
Specifies the model on which the tab display is based.

When any tab is selected the specified selector is invoked
on one of the given objects, with this toolbar item as the
sender.  This is true no matter how the item is found: via
segmented control or overflow menu.

Each object:
- MUST respond to an "attributedDescription" selector that
  returns an "NSAttributedString*" for that tab’s label.
- MAY respond to a "toolTip" selector to return a string for
  that tab’s tooltip.

See the class "TerminalToolbar_TabSource" which is an example
of a valid object for the array.

In the future, additional selectors may be prescribed for the
object (to set an icon, for instance).

(4.0)
*/
- (void)
setTabTargets:(NSArray*)	anObjectArray
andAction:(SEL)				aSelector
{
	if (self->targets != anObjectArray)
	{
		[self->targets release];
		self->targets = [anObjectArray copy];
	}
	self->action = aSelector;
	
	// update the user interface
	[self->segmentedControl setSegmentCount:[self->targets count]];
	[self->segmentedControl setSelectedSegment:0];
	{
		NSMenu*			menuRep = [[[NSMenu alloc] init] autorelease];
		NSMenuItem*		menuItem = [[[NSMenuItem alloc] initWithTitle:@"" action:self->action keyEquivalent:@""] autorelease];
		unsigned int	i = 0;
		
		
		// with "performMenuAction:" and "performSegmentedControlAction:",
		// actions can be handled consistently by the caller; those
		// methods reroute invocations by menu item or segmented control,
		// and present the same sender (this NSToolbarItem) instead
		for (id object in self->targets)
		{
			NSMenuItem*		newItem = [[[NSMenuItem alloc] initWithTitle:[[object attributedDescription] string]
																			action:@selector(performMenuAction:) keyEquivalent:@""]
										autorelease];
			
			
			[newItem setAttributedTitle:[object attributedDescription]];
			[newItem setTarget:self];
			[self->segmentedControl setLabel:[[object attributedDescription] string] forSegment:i];
			if ([object respondsToSelector:@selector(toolTip)])
			{
				[[self->segmentedControl cell] setToolTip:[object toolTip] forSegment:i];
			}
			[menuRep addItem:newItem];
			++i;
		}
		[menuItem setSubmenu:menuRep];
		[self setMenuFormRepresentation:menuItem];
	}
}// setTabTargets:andAction:


#pragma mark NSCopying


/*!
Returns a copy of this object.

(2018.03)
*/
- (id)
copyWithZone:(NSZone*)	zone
{
	id							result = [super copyWithZone:zone];
	TerminalToolbar_ItemTabs*	asSelf = nil;
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemTabs.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemTabs*);
	asSelf->segmentedControl = [self->segmentedControl copy];
	asSelf->targets = [self->targets copy];
	asSelf->action = self->action;
	
	return result;
}// copyWithZone:


@end //} TerminalToolbar_ItemTabs


#pragma mark -
@implementation TerminalToolbar_ItemWindowButton //{


#pragma mark Internally-Declared Properties

/*!
Stores information on view "window" property observer.
*/
@synthesize viewWindowObserver = _viewWindowObserver;


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
		self->_button = nil; // set later
		self->_viewWindowObserver = nil; // initially...
		
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
	[_viewWindowObserver release];
	[_button release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
The view that displays the window button.
*/
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
		[_button autorelease];
		_button = [aWindowButton retain];
		self.view = self.button;
		
		[self removeObserverSpecifiedWith:self.viewWindowObserver];
		_viewWindowObserver = [self newObserverFromSelector:@selector(window) ofObject:aWindowButton
															options:(NSKeyValueChangeSetting)];
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
	[self.view setNeedsDisplay:YES];
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
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowButton.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowButton*);
	
	// views do not support NSCopying; archive instead
	//asSelf->_button = [self->_button copy];
	NSData*		archivedView = [NSKeyedArchiver archivedDataWithRootObject:self->_button];
	asSelf->_button = [NSKeyedUnarchiver unarchiveObjectWithData:archivedView];
	
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
	
	
	if (aContext == self.viewWindowObserver)
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(window)))
			{
				id		oldValue = [aChangeDictionary objectForKey:NSKeyValueChangeOldKey];
				id		newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
				
				
				// remove any previous monitor
				if (nil != oldValue)
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
Called when the specified item has been added to the
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
														forStyleMask:(NSTitledWindowMask | NSClosableWindowMask |
																		NSMiniaturizableWindowMask | NSResizableWindowMask)];
		self.paletteLabel = NSLocalizedString(@"Close", @"toolbar item name; for closing the window");
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
	self.button.enabled = [[Commands_Executor sharedExecutor] validateAction:@selector(performClose:) sender:[NSApp keyWindow]];
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
														forStyleMask:(NSTitledWindowMask | NSClosableWindowMask |
																		NSMiniaturizableWindowMask | NSResizableWindowMask)];
		self.paletteLabel = NSLocalizedString(@"Minimize", @"toolbar item name; for minimizing the window");
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
	self.button.enabled = [[Commands_Executor sharedExecutor] validateAction:@selector(performMiniaturize:) sender:[NSApp keyWindow]];
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
														forStyleMask:(NSTitledWindowMask | NSClosableWindowMask |
																		NSMiniaturizableWindowMask | NSResizableWindowMask)];
		self.paletteLabel = NSLocalizedString(@"Zoom", @"toolbar item name; for zooming the window or controlling Full Screen");
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
	self.button.enabled = [[Commands_Executor sharedExecutor] validateAction:@selector(performZoom:) sender:[NSApp keyWindow]];
}// validate


@end //} TerminalToolbar_ItemWindowButtonZoom


#pragma mark -
@implementation TerminalToolbar_TextLabel //{


#pragma mark Internally-Declared Properties

/*!
This temporarily prevents any response to a change in the
frame of the view, which is important to prevent recursion.
*/
@synthesize disableFrameMonitor = _disableFrameMonitor;

/*!
If set, the frame is rendered along with the text.  (This
is currently useful only in toolbar items, and may move to
a different view to enable a frame rectangle that is far
different from the text rectangle.)
*/
@synthesize frameDisplayEnabled = _frameDisplayEnabled;

/*!
This should only be set in direct response to measuring the
required space and determining that there is not enough
room for all the text using the current font.  It tells the
"drawRect:" implementation to use a fade-out effect as the
text is truncated.
*/
@synthesize gradientFadeEnabled = _gradientFadeEnabled;


#pragma mark Externally-Declared Properties

/*!
A weaker form of frame observer that is only told about
changes to the “ideal” layout (helps toolbar items to
match it).
*/
@synthesize idealSizeMonitor = _idealSizeMonitor;


/*!
If this property is set to YES then the user can drag the
window even if the initial click is on this view.  This
overrides the base NSView class.
*/
@synthesize mouseDownCanMoveWindow = _mouseDownCanMoveWindow;


/*!
Set this to YES to make the font size slightly smaller by
default.  Note that, in addition, the font size adjusts
automatically based on available space.
*/
@synthesize smallSize = _smallSize;


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
	NSGraphicsContext*	asContextObj = [NSGraphicsContext graphicsContextWithGraphicsPort:maskContext flipped:NO];
	
	
	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext setCurrentContext:asContextObj];
	// INCOMPLETE: no center fading is implemented
	if (kTerminalToolbar_TextLabelLayoutRightJustified == aDirection)
	{
		NSGradient*		grayGradient = [[[NSGradient alloc] initWithColors:@[[NSColor blackColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor]]]
										autorelease];
		
		
		[grayGradient drawFromPoint:NSZeroPoint toPoint:NSMakePoint(CGBitmapContextGetWidth(maskContext), 0) options:0];
	}
	else if (kTerminalToolbar_TextLabelLayoutCenterJustified == aDirection)
	{
		NSGradient*		grayGradient = [[[NSGradient alloc] initWithColors:@[[NSColor whiteColor], [NSColor whiteColor], [NSColor blackColor], [NSColor whiteColor], [NSColor whiteColor]]]
										autorelease];
		
		
		[grayGradient drawFromPoint:NSZeroPoint toPoint:NSMakePoint(CGBitmapContextGetWidth(maskContext), 0) options:0];
	}
	else
	{
		NSGradient*		grayGradient = [[[NSGradient alloc] initWithColors:@[[NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor whiteColor], [NSColor blackColor]]]
										autorelease];
		
		
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
		_idealSizeMonitor = nil;
		_mouseDownCanMoveWindow = NO;
		_smallSize = NO;
		self.postsFrameChangedNotifications = YES;
		[self whenObject:self postsNote:NSViewFrameDidChangeNotification
							performSelector:@selector(textViewFrameDidChange:)];
		
		// initialize various label properties
		self.allowsDefaultTighteningForTruncation = YES;
		self.labelLayout = kTerminalToolbar_TextLabelLayoutLeftJustified;
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
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Determines how the text should handle alignment, wrapping and
truncation.

(2018.03)
*/
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
Responds when the window title toolbar item frame is changed
by a system API such as a direct modification of the frame
property on that view.

This item attempts to choose a better font size when it does
not have enough space for the whole string.

(2018.03)
*/
- (void)
textViewFrameDidChange:(NSNotification*)	aNotification
{
	if (NO == self.disableFrameMonitor)
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


#pragma mark NSView


/*!
Draws the text label, possibly with a text fade-out effect
for truncation.

(2018.03)
*/
- (void)
drawRect:(NSRect)	aRect
{
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


@end //} TerminalToolbar_TextLabel


#pragma mark -
@implementation TerminalToolbar_TextLabel (TerminalToolbar_TextLabelInternal) //{


#pragma mark New Methods


/*!
Uses the text view’s own "sizeToFit" temporarily to
calculate the size that the given string would
require with the current font.

(2018.03)
*/
- (CGSize)
idealFrameSizeForString:(NSString*)		aString
{
	CGSize		result = CGSizeZero;
	
	
	if (nil != aString)
	{
		BOOL		originalDisableFlag = self.disableFrameMonitor;
		NSString*	originalValue = self.stringValue;
		NSRect		originalFrame = self.frame;
		
		
		// during measurement-related frame changes, ignore notifications
		self.disableFrameMonitor = YES;
		
		self.stringValue = aString;
		[self sizeToFit];
		result = CGSizeMake(NSWidth(self.frame), NSHeight(self.frame));
		self.frame = originalFrame;
		self.stringValue = originalValue; // remove sizing string
		
		self.disableFrameMonitor = originalDisableFlag;
	}
	
	return result;
}// idealFrameSizeForString:


/*!
Examines the current text and updates properties to make
the ideal layout: possibly new font, etc.  This also
informs the "idealSizeMonitor" object, if any.

Invoke this method whenever the text needs to be reset
(frame change, string value change, initialization, etc.).

(2018.03)
*/
- (void)
layOutLabelText
{
	BOOL const		isSmallSize = self.smallSize;
	CGFloat			originalWidth = NSWidth(self.frame);
	CGSize			requiredSize = CGSizeMake(1 + originalWidth, NSHeight(self.frame));
	NSString*		layoutString = (((nil == self.stringValue) || (0 == self.stringValue.length))
									? @"Âgp" // arbitrary; lay out with text, not a blank space
									: self.stringValue);
	
	
	// set initial truncation from layout (may be overridden below
	// if the text must wrap to two lines)
	switch (self.labelLayout)
	{
	case kTerminalToolbar_TextLabelLayoutLeftJustified:
		self.alignment = NSLeftTextAlignment; // NSControl setting
		self.lineBreakMode = NSLineBreakByTruncatingTail;
		break;
	
	case kTerminalToolbar_TextLabelLayoutRightJustified:
		self.alignment = NSRightTextAlignment; // NSControl setting
		self.lineBreakMode = NSLineBreakByTruncatingHead;
		break;
	
	case kTerminalToolbar_TextLabelLayoutCenterJustified:
	default:
		self.alignment = NSCenterTextAlignment; // NSControl setting
		self.lineBreakMode = NSLineBreakByTruncatingMiddle;
		break;
	}
	
	// initialize line count (may be overridden below)
	self.usesSingleLineMode = YES;
	self.maximumNumberOfLines = 1;
	
	// test a variety of font sizes to find the most reasonable one;
	// if text ends up too long anyway then view will squish/truncate
	// (TEMPORARY...should any of this be a user preference?)
	if (requiredSize.width > originalWidth)
	{
		self.font = [NSFont titleBarFontOfSize:((isSmallSize) ? 14 : 16)];
		requiredSize = [self idealFrameSizeForString:layoutString];
	}
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
	
	// NOTE: this currently is buggy in a dynamic resize scenario, since
	// the font is updated one iteration ahead of the next toolbar layout;
	// if the user were to stop resizing as soon as the font changes, the
	// text is clipped instead of actually using the new minimum size
	// (TEMPORARY; find a fix for this somehow; no forced layout seems to
	// work...)
	[self setNeedsDisplay:YES];
	
	// update frame without triggering notifications (as a frame update
	// may have caused this layout function to be invoked originally)
	{
		BOOL const	wasPostingNotifications = self.postsFrameChangedNotifications;
		
		
		self.postsFrameChangedNotifications = NO;
		self.frame = NSMakeRect(self.frame.origin.x, self.frame.origin.y, NSWidth(self.frame), requiredSize.height);
		self.postsFrameChangedNotifications = wasPostingNotifications; // may trigger immediate posting of notification
	}
	
	// notify any observer
	[self.idealSizeMonitor view:self didSetIdealFrameHeight:requiredSize.height];
}// layOutLabelText


@end //} TerminalToolbar_TextLabel (TerminalToolbar_TextLabelInternal)


#pragma mark -
@implementation TerminalToolbar_WindowTitleLabel //{


#pragma mark Internally-Declared Properties

/*!
Stores information on window "title" property observer.
*/
@synthesize windowTitleObserver = _windowTitleObserver;


#pragma mark Externally-Declared Properties

/*!
Specify a window to take precedence over view’s own window.
*/
@synthesize overrideWindow = _overrideWindow;

/*!
External object to notify when the window changes.
*/
@synthesize windowMonitor = _windowMonitor;


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
		self->_overrideWindow = nil;
		self->_windowMonitor = nil;
		self->_windowTitleObserver = nil; // initially...
		
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
	[_windowTitleObserver release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


- (NSWindow*)
overrideWindow
{
	return [[_overrideWindow retain] autorelease];
}
- (void)
setOverrideWindow:(NSWindow*)	aWindow
{
	if (_overrideWindow != aWindow)
	{
		[_overrideWindow autorelease];
		_overrideWindow = [aWindow retain];
		[self removeObserverSpecifiedWith:self.windowTitleObserver];
		_windowTitleObserver = [self newObserverFromSelector:@selector(title) ofObject:aWindow
																options:(NSKeyValueChangeSetting)];
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
	
	
	if (aContext == self.windowTitleObserver)
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			if (KEY_PATH_IS_SEL(aKeyPath, @selector(title)))
			{
				id		newValue = [aChangeDictionary objectForKey:NSKeyValueChangeNewKey];
				
				
				//NSLog(@"title changed: %@", aChangeDictionary); // debug	
				if (nil != newValue)
				{
					assert([newValue isKindOfClass:NSString.class]);
					NSString*	asString = STATIC_CAST(newValue, NSString*);
					
					
					self.stringValue = asString;
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
																		options:(NSKeyValueChangeSetting)];
				
				// is this necessary, or is the observer called automatically from the above?
				self.stringValue = ((nil != aWindow.title) ? aWindow.title : @"");
				[self layOutLabelText];
			}
		}
	}
}// viewWillMoveToWindow:


@end //} TerminalToolbar_WindowTitleLabel


#pragma mark -
@implementation TerminalToolbar_ItemWindowTitle //{


#pragma mark Internally-Declared Properties

/*!
This temporarily prevents any response to a change in the
frame of the item’s view, which is important to prevent
recursion.
*/
@synthesize disableFrameMonitor = _disableFrameMonitor;

/*!
The view that displays the window title text.
*/
@synthesize textView = _textView;


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
		self->_disableFrameMonitor = NO;
		self->_textView = [[TerminalToolbar_WindowTitleLabel alloc] initWithFrame:NSZeroRect];
		self.textView.idealSizeMonitor = self;
		self.textView.windowMonitor = self;
		self.textView.mouseDownCanMoveWindow = YES;
		self.textView.bezeled = NO;
		self.textView.bordered = NO;
		self.textView.drawsBackground = NO;
		self.textView.editable = NO;
		//self.textView.font = [NSFont titleBarFontOfSize:9]; // set in TerminalToolbar_WindowTitleLabel
		//self.textView.selectable = YES;
		self.textView.selectable = NO;
		
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setView:self.textView];
		[self setLabel:NSLocalizedString(@"Window Title", @"toolbar item name; for window title")];
		[self setPaletteLabel:NSLocalizedString(@"Window Title", @"toolbar item name; for window title")];
		[self setStateForToolbar:nullptr];
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
	[_textView release];
	[super dealloc];
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


#pragma mark Notifications


/*!
Keeps track of sheets being displayed so that the “is
customization sheet running” flag can be checked and
the display of a boundary can be toggled.

NOTE:	It is also possible for the system to enable
		customization by command-dragging toolbar items,
		and it isn’t clear yet how to detect this mode.

(2018.03)
*/
- (void)
windowWillBeginSheet:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	[self setStateForToolbar:self.toolbar]; // shows frame rectangle
}// windowWillBeginSheet:


/*!
Keeps track of sheets being displayed so that the “is
customization sheet running” flag can be checked and
the display of a boundary can be toggled.

(2018.03)
*/
- (void)
windowDidEndSheet:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	[self setStateForToolbar:self.toolbar]; // hides frame rectangle
}// windowDidEndSheet:


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
	
	
	assert([result isKindOfClass:TerminalToolbar_ItemWindowTitle.class]); // parent supports NSCopying so this should have been done properly
	asSelf = STATIC_CAST(result, TerminalToolbar_ItemWindowTitle*);
	asSelf->_disableFrameMonitor = self->_disableFrameMonitor;
	
	// views do not support NSCopying; archive instead
	//asSelf->_textView = [self->_textView copy];
	NSData*		archivedView = [NSKeyedArchiver archivedDataWithRootObject:self->_textView];
	asSelf->_textView = [NSKeyedUnarchiver unarchiveObjectWithData:archivedView];
	
	return result;
}// copyWithZone:


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
	[self.textView layOutLabelText];
	[self resetMinMaxSizesForHeight:NSHeight(self.textView.frame)];
}// didChangeDisplayModeForToolbar:


#pragma mark TerminalToolbar_ItemAddRemoveSensitive


/*!
Called when the specified item has been added to the
specified toolbar.

(2018.03)
*/
- (void)
item:(NSToolbarItem*)			anItem
willEnterToolbar:(NSToolbar*)	aToolbar
{
	assert(self == anItem);
	self.textView.smallSize = ((nil != aToolbar) &&
								(NSToolbarSizeModeSmall == aToolbar.sizeMode));
	[self setStateForToolbar:aToolbar];
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
	[result setImage:[NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleCenterIconFilenameNoExtension(), NSString*)]];
	
	return [result autorelease];
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
	[self.textView layOutLabelText];
	[self resetMinMaxSizesForHeight:NSHeight(self.textView.frame)];
}// didChangeSizeForToolbar:


#pragma mark TerminalToolbar_ViewFrameSensitive


/*!
Updates the minimum and maximum sizes to match.

(2018.03)
*/
- (void)
view:(NSView*)						aView
didSetIdealFrameHeight:(CGFloat)	aPixelHeight
{
	if (aView != self.textView)
	{
		Console_Warning(Console_WriteLine, "window title toolbar item is being notified of a different view’s ideal-frame changes");
	}
	else
	{
		[self resetMinMaxSizesForHeight:aPixelHeight];
	}
}// view:didSetIdealFrameHeight:


#pragma mark TerminalToolbar_ViewWindowSensitive


/*!
Updates the minimum and maximum sizes to match after
the window title view enters the toolbar and presumably
adopts that window’s title value.  Also keeps track of
any toolbar customization sheets on the window so that
the boundary can be displayed in the item.

(2018.03)
*/
- (void)
view:(NSView*)					aView
didEnterWindow:(NSWindow*)		aWindow
{
	if (aView != self.textView)
	{
		Console_Warning(Console_WriteLine, "window title toolbar item is being notified of a different view’s window changes");
	}
	else
	{
		// start monitoring the toolbar customization sheet (NOTE: this
		// does not seem to be sent for Full Screen windows; for now,
		// the Customize item is disabled in Full Screen)
		if (nil != aWindow)
		{
			[self whenObject:aWindow postsNote:NSWindowWillBeginSheetNotification
								performSelector:@selector(windowWillBeginSheet:)];
			[self whenObject:aWindow postsNote:NSWindowDidEndSheetNotification
								performSelector:@selector(windowDidEndSheet:)];
		}
		
		// set initial state of item for new window’s toolbar
		[self setStateForToolbar:aWindow.toolbar];
	}
}// view:didEnterWindow:


/*!
Stops tracking the toolbar customization sheet of the
view’s current window (since that window is about to
change).

(2018.03)
*/
- (void)
willChangeWindowForView:(NSView*)	aView
{
	if (aView != self.textView)
	{
		Console_Warning(Console_WriteLine, "window title toolbar item is being notified of a different view’s window changes");
	}
	else
	{
		// stop monitoring the toolbar customization sheet
		if (nil != self.textView.window)
		{
			[self ignoreWhenObject:self.textView.window postsNote:NSWindowWillBeginSheetNotification];
			[self ignoreWhenObject:self.textView.window postsNote:NSWindowDidEndSheetNotification];
		}
	}
}// willChangeWindowForView:


@end //} TerminalToolbar_ItemWindowTitle


#pragma mark -
@implementation TerminalToolbar_ItemWindowTitle (TerminalToolbar_ItemWindowTitleInternal) //{


#pragma mark New Methods


/*!
Uses a sizing string to attempt to make the text view
automatically return its ideal height.

(2018.03)
*/
- (void)
resetMinMaxSizesForHeight:(CGFloat)		aHeight
{
	[self setMinSize:NSMakeSize(60, aHeight)];
	[self setMaxSize:NSMakeSize(2048, aHeight)];
}// resetMinMaxSizesForHeight:


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
	//BOOL const		isSmallSize = ((nil != aToolbar) &&
	//								(NSToolbarSizeModeSmall == aToolbar.sizeMode));
	BOOL const		isTextOnly = ((nil != aToolbar) &&
									(NSToolbarDisplayModeLabelOnly == aToolbar.displayMode));
	
	
	self.textView.frameDisplayEnabled = (aToolbar.customizationPaletteIsRunning);
	self.textView.toolTip = self.textView.stringValue;
	[self resetMinMaxSizesForHeight:NSHeight(self.textView.frame)]; // see also "view:didSetIdealFrameHeight:"
	
	// this allows text-only toolbars to still display the window title string
	self.label = ((isTextOnly)
					? [NSString stringWithFormat:@"  %@  ", ((nil != self.textView.stringValue) ? self.textView.stringValue : @"")]
					: @"");
	
	self.menuFormRepresentation = [[[NSMenuItem alloc] initWithTitle:self.textView.stringValue action:nil keyEquivalent:@""] autorelease];
	self.menuFormRepresentation.enabled = NO;
}// setStateForToolbar:


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
		[self setImage:[NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleLeftIconFilenameNoExtension(), NSString*)]]; // TEMPORARY
		[self setPaletteLabel:NSLocalizedString(@"Left-Aligned Title", @"toolbar item name; for left-aligned window title")];
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
	[result setImage:[NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleLeftIconFilenameNoExtension(), NSString*)]];
	
	return [result autorelease];
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
		[self setPaletteLabel:NSLocalizedString(@"Right-Aligned Title", @"toolbar item name; for right-aligned window title")];
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
	[result setImage:[NSImage imageNamed:BRIDGE_CAST(AppResources_ReturnWindowTitleRightIconFilenameNoExtension(), NSString*)]];
	
	return [result autorelease];
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


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


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
	[super dealloc];
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
	SessionRef		result = [self.toolbar terminalToolbarSession];
	
	
	if (nullptr == result)
	{
		result = _sessionHint;
	}
	else
	{
		self->_sessionHint = nullptr;
	}
	
	return result;
}// session


/*!
If this item has no toolbar yet (such as, construction during
toolbar customization), "setSessionHint:" can be used to improve
the default setup of the item to match the target toolbar.  This
value is NOT used by the "session" method unless the item has no
toolbar assigned.  In addition, this value is cleared by future
calls to "session" if a toolbar exists.

(2018.03)
*/
- (void)
setSessionHint:(SessionRef)		aSession
{
	self->_sessionHint = aSession;
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
	TerminalWindowRef	terminalWindow = [self terminalWindow];
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
	TerminalWindowRef	terminalWindow = [self terminalWindow];
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
	SessionRef			session = [self session];
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
	asSelf->_sessionHint = self->_sessionHint;
	
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
Called when the specified item has been added to the
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
	[self whenObject:[self.toolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionDidChangeNotification
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
	[self ignoreWhenObject:[self.toolbar terminalToolbarDelegate] postsNote:kTerminalToolbar_DelegateSessionDidChangeNotification];
}// removeSessionDependentItemNotificationHandlersForToolbar:


@end //} TerminalToolbar_SessionDependentItem (TerminalToolbar_SessionDependentItemInternal)


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
		self->isDisplayingSheet = NO;
		
		// create toolbar
		{
			NSString*					toolbarID = @"TerminalToolbar"; // do not ever change this; that would only break user preferences
			TerminalToolbar_Object*		windowToolbar = [[[TerminalToolbar_Object alloc] initWithIdentifier:toolbarID]
															autorelease];
			
			
			self->toolbarDelegate = [[TerminalToolbar_Delegate alloc] initForToolbar:windowToolbar experimentalItems:NO];
			[windowToolbar setAllowsUserCustomization:YES];
			[windowToolbar setAutosavesConfiguration:YES];
			[windowToolbar setDelegate:self->toolbarDelegate];
			[self whenObject:windowToolbar postsNote:kTerminalToolbar_ObjectDidChangeDisplayModeNotification
								performSelector:@selector(toolbarDidChangeDisplayMode:)];
			[self whenObject:windowToolbar postsNote:kTerminalToolbar_ObjectDidChangeSizeModeNotification
								performSelector:@selector(toolbarDidChangeSizeMode:)];
			[self whenObject:windowToolbar postsNote:kTerminalToolbar_ObjectDidChangeVisibilityNotification
								performSelector:@selector(toolbarDidChangeVisibility:)];
			[self setToolbar:windowToolbar];
		}
		
		// monitor the active session, but also initialize the value
		// (the callback should update the value in all other cases)
		[self->toolbarDelegate setSession:SessionFactory_ReturnUserRecentSession()];
		self->sessionFactoryChangeListener = [[ListenerModel_StandardListener alloc]
												initWithTarget:self
																eventFiredSelector:
																@selector(model:sessionFactoryChange:context:)];
		SessionFactory_StartMonitoring(kSessionFactory_ChangeActivatingSession,
										[self->sessionFactoryChangeListener listenerRef]);
		SessionFactory_StartMonitoring(kSessionFactory_ChangeDeactivatingSession,
										[self->sessionFactoryChangeListener listenerRef]);
		
		// panel properties
		[self setBecomesKeyOnlyIfNeeded:YES];
		[self setFloatingPanel:YES];
		[self setWorksWhenModal:NO];
		
		// window properties
		[self setHasShadow:NO];
		[self setHidesOnDeactivate:YES];
		[self setLevel:NSNormalWindowLevel];
		[self setMovableByWindowBackground:YES];
		[self setReleasedWhenClosed:NO];
		[[self standardWindowButton:NSWindowCloseButton] setHidden:YES];
		[[self standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
		[[self standardWindowButton:NSWindowZoomButton] setHidden:YES];
		[[self standardWindowButton:NSWindowDocumentIconButton] setHidden:YES];
		[[self standardWindowButton:NSWindowToolbarButton] setHidden:YES];
		
		// the toolbar is meant to apply to the active window so it
		// should be visible wherever the user is (even in a
		// Full Screen context)
		[self setCollectionBehavior:(NSWindowCollectionBehaviorMoveToActiveSpace |
										NSWindowCollectionBehaviorParticipatesInCycle)];
		
		// size constraints; the window should have no content height
		// because it is only intended to display the window’s toolbar
		{
			NSSize	limitedSize = NSMakeSize(512, 0); // arbitrary
			
			
			[self setContentMinSize:limitedSize];
			limitedSize.width = 2048; // arbitrary
			[self setContentMaxSize:limitedSize];
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
	if (nullptr != sessionFactoryChangeListener)
	{
		SessionFactory_StopMonitoring(kSessionFactory_ChangeActivatingSession,
										[sessionFactoryChangeListener listenerRef]);
		SessionFactory_StopMonitoring(kSessionFactory_ChangeDeactivatingSession,
										[sessionFactoryChangeListener listenerRef]);
		[sessionFactoryChangeListener release];
	}
	[self ignoreWhenObjectsPostNotes];
	[self->toolbarDelegate release];
	[super dealloc];
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
	unless (self->isDisplayingSheet)
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
			[[[self toolbar] terminalToolbarDelegate] setSession:session];
		}
		break;
	
	case kSessionFactory_ChangeDeactivatingSession:
		{
			//SessionRef		session = REINTERPRET_CAST(aContext, SessionRef);
			
			
			//Console_WriteValueAddress("terminal toolbar window: session resigning key", session);
		
			// notify delegate, which will in turn fire notifications that
			// session-dependent toolbar items are observing
			[[[self toolbar] terminalToolbarDelegate] setSession:nil];
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
	[self setFrameBasedOnRect:[self frame]];
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
	if (NO == [[self toolbar] isVisible])
	{
		[[self toolbar] setVisible:YES];
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
	if (NO == [[self toolbar] isVisible])
	{
		[[self toolbar] setVisible:YES];
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
	self->isDisplayingSheet = YES;
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
	if (self->isDisplayingSheet)
	{
		self->isDisplayingSheet = NO;
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
	
	if (NO == [[self toolbar] isVisible])
	{
		// if the toolbar is being hidden, just hide the entire toolbar window
		[self orderOut:nil];
	}
}// toolbarDidChangeVisibility:


@end //} TerminalToolbar_Window (TerminalToolbar_WindowInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
