/*!	\file TerminalToolbar.mm
	\brief Items used in the toolbars of terminal windows.
*/
/*###############################################################

	MacTerm
		© 1998-2013 by Kevin Grant.
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
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <ListenerModel.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "Preferences.h"
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

#pragma mark Internal Method Prototypes
namespace {

OSStatus	receiveNewSystemUIMode		(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

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
CarbonEventHandlerWrap			gNewSystemUIModeHandler(GetApplicationEventTarget(),
														receiveNewSystemUIMode,
														CarbonEventSetInClass
															(CarbonEventClass(kEventClassApplication),
																kEventAppSystemUIModeChanged),
														nullptr/* user data */);
Console_Assertion				_1(gNewSystemUIModeHandler.isInstalled(), __FILE__, __LINE__);

} // anonymous namespace




#pragma mark Internal Methods
namespace {

/*!
Embellishes "kEventAppSystemUIModeChanged" of "kEventClassApplication"
by ensuring that toolbar windows remain attached to a screen edge when
the application enters or exits Full Screen mode, hides the Dock or
otherwise changes the space available for windows on the display.

NOTE:	This is Carbon-based because the Carbon APIs for switching
		the user interface mode are currently called.  If that ever
		changes to use a newer (and presumably Cocoa-based) method,
		then a new way of detecting the mode-switch must be found.

(4.0)
*/
OSStatus
receiveNewSystemUIMode	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassApplication);
	assert(kEventKind == kEventAppSystemUIModeChanged);
	
	// determine the new mode (if needed)
	{
		//UInt32		newUIMode = kUIModeNormal;
		//OSStatus	error = CarbonEventUtilities_GetEventParameter
		//						(inEvent, kEventParamSystemUIMode, typeUInt32, newUIMode);
		
		
		[TerminalToolbar_Window refreshWindows];
	}
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveNewSystemUIMode

} // anonymous namespace


@implementation NSToolbar (TerminalToolbar_NSToolbarExtensions)


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


@end // NSToolbar (TerminalToolbar_NSToolbarExtensions)


@implementation TerminalToolbar_Delegate


/*!
Designated initializer.

(4.0)
*/
- (id)
initForToolbar:(NSToolbar*)		aToolbar
experimentalItems:(BOOL)		experimentalFlag
{
#pragma unused(aToolbar)
	self = [super init];
	if (nil != self)
	{
		self->associatedSession = nullptr;
		self->allowExperimentalItems = experimentalFlag;
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
		[[NSNotificationCenter defaultCenter] postNotificationName:kTerminalToolbar_DelegateSessionWillChangeNotification
																	object:self];
		self->associatedSession = aSession;
		[[NSNotificationCenter defaultCenter] postNotificationName:kTerminalToolbar_DelegateSessionDidChangeNotification
																	object:self];
	}
}// setSession


#pragma mark NSToolbarDelegate


/*!
Responds when the specified kind of toolbar item should be
constructed for the given toolbar.

(4.0)
*/
- (NSToolbarItem*)
toolbar:(NSToolbar*)				toolbar
itemForItemIdentifier:(NSString*)	itemIdentifier
willBeInsertedIntoToolbar:(BOOL)	flag
{
#pragma unused(toolbar, flag)
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
		[result addObject:kMy_ToolbarItemIDTabs];
	}
	
	[result addObjectsFromArray:[NSArray arrayWithObjects:
									// NOTE: these are ordered with their palette arrangement in mind
									kTerminalToolbar_ItemIDNewSessionDefaultFavorite,
									kTerminalToolbar_ItemIDNewSessionLogInShell,
									kTerminalToolbar_ItemIDNewSessionShell,
									kTerminalToolbar_ItemIDStackWindows,
									NSToolbarSpaceItemIdentifier,
									NSToolbarFlexibleSpaceItemIdentifier,
									kTerminalToolbar_ItemIDCustomize,
									kMy_ToolbarItemIDFullScreen,
									kMy_ToolbarItemIDLED1,
									kMy_ToolbarItemIDLED2,
									kMy_ToolbarItemIDLED3,
									kMy_ToolbarItemIDLED4,
									kMy_ToolbarItemIDPrint,
									kMy_ToolbarItemIDHide,
									kMy_ToolbarItemIDForceQuit,
									kMy_ToolbarItemIDSuspend,
									kMy_ToolbarItemIDBell,
									nil]];
	
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
	return [NSArray arrayWithObjects:
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						kMy_ToolbarItemIDHide,
						kMy_ToolbarItemIDForceQuit,
						kMy_ToolbarItemIDSuspend,
						NSToolbarFlexibleSpaceItemIdentifier,
						kMy_ToolbarItemIDLED1,
						kMy_ToolbarItemIDLED2,
						kMy_ToolbarItemIDLED3,
						kMy_ToolbarItemIDLED4,
						NSToolbarFlexibleSpaceItemIdentifier,
						kMy_ToolbarItemIDBell,
						kMy_ToolbarItemIDPrint,
						kMy_ToolbarItemIDFullScreen,
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						kTerminalToolbar_ItemIDCustomize,
						nil];
}// toolbarDefaultItemIdentifiers


@end // TerminalToolbar_Delegate


@implementation TerminalToolbar_ItemBell


/*!
Designated initializer.

(4.0)
*/
- (id)
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
	[screenChangeListener release];
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandBellEnabled);
}// performToolbarItemAction:


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


@end // TerminalToolbar_ItemBell


@implementation TerminalToolbar_ItemBell (TerminalToolbar_ItemBellInternal)


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


@end // TerminalToolbar_ItemBell (TerminalToolbar_ItemBellInternal)


@implementation TerminalToolbar_ItemCustomize


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	// TEMPORARY; only doing it this way during Carbon/Cocoa transition
	[[Commands_Executor sharedExecutor] runToolbarCustomizationPaletteSetup:NSApp];
}// performToolbarItemAction:


@end // TerminalToolbar_ItemCustomize


@implementation TerminalToolbar_ItemForceQuit


/*!
Designated initializer.

(4.0)
*/
- (id)
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
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	SessionRef		targetSession = [self session];
	
	
	if (Session_IsValid(targetSession) && Session_StateIsDead(targetSession))
	{
		Commands_ExecuteByIDUsingEvent(kCommandRestartSession);
	}
	else
	{
		Commands_ExecuteByIDUsingEvent(kCommandKillProcessesKeepWindow);
	}
}// performToolbarItemAction:


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
		[self setStateFromSession:nullptr];
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
	
	SessionRef	session = [self session];
	
	
	if (Session_IsValid(session))
	{
		[self setStateFromSession:session];
		Session_StartMonitoring(session, kSession_ChangeState, [self->sessionChangeListener listenerRef]);
	}
}// didChangeSession


@end // TerminalToolbar_ItemForceQuit


@implementation TerminalToolbar_ItemForceQuit (TerminalToolbar_ItemForceQuitInternal)


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


@end // TerminalToolbar_ItemForceQuit (TerminalToolbar_ItemForceQuitInternal)


@implementation TerminalToolbar_ItemFullScreen


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFullScreenModal);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemFullScreen


@implementation TerminalToolbar_ItemHide


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandHideFrontWindow);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemHide


@implementation TerminalToolbar_ItemLED1


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED1);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED1


@implementation TerminalToolbar_ItemLED2


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED2);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED2


@implementation TerminalToolbar_ItemLED3


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED3);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED3


@implementation TerminalToolbar_ItemLED4


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED4);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED4


@implementation TerminalToolbar_ItemNewSessionDefaultFavorite


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionDefaultFavorite);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemNewSessionDefaultFavorite


@implementation TerminalToolbar_ItemNewSessionLogInShell


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionLoginShell);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemNewSessionLogInShell


@implementation TerminalToolbar_ItemNewSessionShell


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionShell);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemNewSessionShell


@implementation TerminalToolbar_ItemPrint


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandPrint);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemPrint


@implementation TerminalToolbar_ItemStackWindows


/*!
Designated initializer.

(4.0)
*/
- (id)
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


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandStackWindows);
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


@end // TerminalToolbar_ItemStackWindows


@implementation TerminalToolbar_ItemSuspend


/*!
Designated initializer.

(4.0)
*/
- (id)
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
	[sessionChangeListener release];
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSuspendNetwork);
}// performToolbarItemAction:


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
		[self setStateFromSession:nullptr];
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
	
	SessionRef	session = [self session];
	
	
	if (Session_IsValid(session))
	{
		[self setStateFromSession:session];
		Session_StartMonitoring(session, kSession_ChangeStateAttributes, [self->sessionChangeListener listenerRef]);
	}
}// didChangeSession


@end // TerminalToolbar_ItemSuspend


@implementation TerminalToolbar_ItemSuspend (TerminalToolbar_ItemSuspendInternal)


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


@end // TerminalToolbar_ItemSuspend (TerminalToolbar_ItemSuspendInternal)


@implementation TerminalToolbar_LEDItem


/*!
Designated initializer.

(4.0)
*/
- (id)
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
	[screenChangeListener release];
	[super dealloc];
}// dealloc


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


@end // TerminalToolbar_LEDItem


@implementation TerminalToolbar_LEDItem (TerminalToolbar_LEDItemInternal)


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
	if ((nullptr == aScreen) || (false == Terminal_LEDIsOn(aScreen, self->indexOfLED)))
	{
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
	}
	else
	{
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOnIconFilenameNoExtension()]];
	}
}// setStateFromScreen:


@end // TerminalToolbar_LEDItem (TerminalToolbar_LEDItemInternal)


@implementation TerminalToolbar_TabSource


/*!
Designated initializer.

(4.0)
*/
- (id)
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


@end // TerminalToolbar_TabSource


@implementation TerminalToolbar_ItemTabs


/*!
Designated initializer.

(4.0)
*/
- (id)
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
		[self setMinSize:NSMakeSize(120, 25)]; // arbitrary
		[self setMaxSize:NSMakeSize(1024, 25)]; // arbitrary
		[self setLabel:@""];
		[self setPaletteLabel:NSLocalizedString(@"Tabs", @"toolbar item name; for tabs")];
		
		[self setTabTargets:[NSArray arrayWithObjects:
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
										nil]
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


@end // TerminalToolbar_ItemTabs


@implementation TerminalToolbar_Object


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithIdentifier:(NSString*)	anIdentifier
{
	self = [super initWithIdentifier:anIdentifier];
	if (nil != self)
	{
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
	[[NSNotificationCenter defaultCenter] postNotificationName:kTerminalToolbar_ObjectDidChangeDisplayModeNotification
																object:self];
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
	[[NSNotificationCenter defaultCenter] postNotificationName:kTerminalToolbar_ObjectDidChangeSizeModeNotification
																object:self];
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
		[[NSNotificationCenter defaultCenter] postNotificationName:kTerminalToolbar_ObjectDidChangeVisibilityNotification
																	object:self];
	}
}// setVisible:


@end // TerminalToolbar_Object


@implementation TerminalToolbar_SessionDependentItem


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithItemIdentifier:(NSString*)		anIdentifier
{
	self = [super initWithItemIdentifier:anIdentifier];
	if (nil != self)
	{
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(sessionWillChange:)
															name:kTerminalToolbar_DelegateSessionWillChangeNotification
															object:[[self toolbar] terminalToolbarDelegate]];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(sessionDidChange:)
															name:kTerminalToolbar_DelegateSessionDidChangeNotification
															object:[[self toolbar] terminalToolbarDelegate]];
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
	[self willChangeSession]; // allows subclasses to automatically stop monitoring the current session
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}// dealloc


/*!
Convenience method for finding the session currently associated
with the toolbar that contains this item (may be "nullptr").

(4.0)
*/
- (SessionRef)
session
{
	return [[self toolbar] terminalToolbarSession];
}// session


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


@end // TerminalToolbar_SessionDependentItem


@implementation TerminalToolbar_Window


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithContentRect:(NSRect)	aRect
screen:(NSScreen*)				aScreen
{
	self = [super initWithContentRect:aRect styleMask:(NSTitledWindowMask | NSUtilityWindowMask | NSClosableWindowMask |
														NSMiniaturizableWindowMask)
										backing:NSBackingStoreBuffered defer:YES screen:aScreen];
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
			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(toolbarDidChangeDisplayMode:)
																name:kTerminalToolbar_ObjectDidChangeDisplayModeNotification
																object:windowToolbar];
			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(toolbarDidChangeSizeMode:)
																name:kTerminalToolbar_ObjectDidChangeSizeModeNotification
																object:windowToolbar];
			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(toolbarDidChangeVisibility:)
																name:kTerminalToolbar_ObjectDidChangeVisibilityNotification
																object:windowToolbar];
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
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidChangeScreenParameters:)
															name:NSApplicationDidChangeScreenParametersNotification
															object:NSApp];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeKey:)
															name:NSWindowDidBecomeKeyNotification object:self];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResignKey:)
															name:NSWindowDidResignKeyNotification object:self];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillBeginSheet:)
															name:NSWindowWillBeginSheetNotification object:self];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidEndSheet:)
															name:NSWindowDidEndSheetNotification object:self];
		
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
	[[NSNotificationCenter defaultCenter] removeObserver:self];
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
	
	
	// initially assume the superclass is the best result
	frameRect = result;
	
	// constraints must be disabled when the customization sheet is open
	// so that the window can move (if necessary) to reveal the sheet
	unless (self->isDisplayingSheet)
	{
		NSScreen*	targetScreen = (nil != aScreen)
									? aScreen
									: ((nil != [self screen])
										? [self screen]
										: [NSScreen mainScreen]);
		NSRect		screenRect = [targetScreen visibleFrame];
		
		
		if (result.origin.y < (screenRect.origin.y + (screenRect.size.height / 2.0)))
		{
			result.origin.y = screenRect.origin.y;
		}
		else
		{
			result.origin.y = screenRect.origin.y + screenRect.size.height - frameRect.size.height;
		}
		result.origin.x = screenRect.origin.x;
		result.size.width = screenRect.size.width;
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


@end // TerminalToolbar_Window


@implementation TerminalToolbar_Window (TerminalToolbar_WindowInternal)


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
	BOOL	animateFlag = NO;
	float	oldBase = [self frame].origin.y;
	
	
	aRect = [self constrainFrameRect:aRect toScreen:[self screen]];
	
	// animation depends on how far the new frame is from the old one
	if (fabsf(oldBase - aRect.origin.y) > 88/* arbitrary */)
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
the user focus.  Also adjusts opacity.

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
	[self setAlphaValue:1.0/* arbitrary */];
}// windowDidBecomeKey:


/*!
Adjusts opacity when the toolbar is not active.

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
	
	// use the same fade value for the toolbar that terminal windows use
	unless (self->isDisplayingSheet)
	{
		Float32		fadeAlpha = 1.0;
		size_t		actualSize = 0;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagFadeAlpha,
				sizeof(fadeAlpha), &fadeAlpha, &actualSize))
		{
			fadeAlpha = 1.0; // assume a value, if preference can’t be found
		}
		
		[self setAlphaValue:fadeAlpha];
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


@end // TerminalToolbar_Window (TerminalToolbar_WindowInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
