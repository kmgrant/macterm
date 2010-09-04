/*###############################################################

	InfoWindow.mm
	
	MacTelnet
		© 1998-2010 by Kevin Grant.
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

#import "UniversalDefines.h"

// standard-C includes
#import <climits>
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <map>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <Cursors.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>
#import <WindowInfo.h>

// MacTelnet includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "EventLoop.h"
#import "InfoWindow.h"
#import "Session.h"
#import "SessionFactory.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

// NOTE: do not ever change these, that would only break user preferences
NSString*	kMyToolbarItemIDNewSessionDefaultFavorite	= @"com.mactelnet.MacTelnet.toolbaritem.newsessiondefault";
NSString*	kMyToolbarItemIDNewSessionLogInShell		= @"com.mactelnet.MacTelnet.toolbaritem.newsessionloginshell";
NSString*	kMyToolbarItemIDNewSessionShell				= @"com.mactelnet.MacTelnet.toolbaritem.newsessionshell";
NSString*	kMyToolbarItemIDStackWindows				= @"com.mactelnet.MacTelnet.toolbaritem.stackwindows";

// the following are also used in "InfoWindowCocoa.xib"
NSString*	kMyInfoColumnCommand		= @"Command";
NSString*	kMyInfoColumnCreationTime	= @"CreationTime";
NSString*	kMyInfoColumnDevice			= @"Device";
NSString*	kMyInfoColumnStatus			= @"Status";
NSString*	kMyInfoColumnWindow			= @"Window";

} // anonymous namespace

#pragma mark Types

/*!
This class holds information displayed in a row of the table.
*/
@interface InfoWindow_SessionRow : NSObject
{
@public
	SessionRef				session;
	CFAbsoluteTime			activationAbsoluteTime;
	NSMutableDictionary*	dataByKey;
}

- (id)
initWithSession:(SessionRef)		aSession
andActivationTime:(CFAbsoluteTime)	aTimeInterval;

- (id)
objectForKey:(NSString*)	aKey;

- (void)
setObject:(id)		anObject
forKey:(NSString*)	aKey;
@end

/*!
Toolbar item “Default”.
*/
@interface InfoWindow_ToolbarItemNewSessionDefaultFavorite : NSToolbarItem
{
}
@end

/*!
Toolbar item “Log-In Shell”.
*/
@interface InfoWindow_ToolbarItemNewSessionLogInShell : NSToolbarItem
{
}
@end

/*!
Toolbar item “Shell”.
*/
@interface InfoWindow_ToolbarItemNewSessionShell : NSToolbarItem
{
}
@end

/*!
Toolbar item “Arrange All Windows in Front”.
*/
@interface InfoWindow_ToolbarItemStackWindows : NSToolbarItem
{
}
@end

#pragma mark Variables
namespace {

ListenerModel_ListenerRef	gSessionAttributeChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionStateChangeEventListener = nullptr;
ListenerModel_ListenerRef	gInfoWindowDisplayEventListener = nullptr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void		refreshDisplay				();
void		sessionAttributeChanged		(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void		sessionStateChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
OSStatus	showHideInfoWindow			(ListenerModel_Ref, ListenerModel_Event, void*, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
This method sets up the initial configurations for
the session status window.  Call this method before
calling any other session status window methods
from this file.

Support for scroll bars has not been implemented
yet.  Nonetheless, the commented-out code for adding
the controls is here.

(3.0)
*/
void
InfoWindow_Init	()
{
	// install notification routines to find out when session states change
	gSessionAttributeChangeEventListener = ListenerModel_NewStandardListener(sessionAttributeChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeResourceLocation, gSessionAttributeChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowObscured, gSessionAttributeChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowTitle, gSessionAttributeChangeEventListener);
	gSessionStateChangeEventListener = ListenerModel_NewStandardListener(sessionStateChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, gSessionStateChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeStateAttributes, gSessionStateChangeEventListener);
	
	// ask to be told when a “show/hide info window” command occurs
	gInfoWindowDisplayEventListener = ListenerModel_NewOSStatusListener(showHideInfoWindow);
	Commands_StartHandlingExecution(kCommandShowConnectionStatus, gInfoWindowDisplayEventListener);
	Commands_StartHandlingExecution(kCommandHideConnectionStatus, gInfoWindowDisplayEventListener);
	
	// if the window was open at last Quit, construct it right away;
	// otherwise, wait until it is requested by the user
	{
		Boolean		windowIsVisible = false;
		size_t		actualSize = 0L;
		
		
		// get visibility preference for the Clipboard
		unless (Preferences_GetData(kPreferences_TagWasSessionInfoShowing,
									sizeof(windowIsVisible), &windowIsVisible,
									&actualSize) == kPreferences_ResultOK)
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		
		if (windowIsVisible)
		{
			InfoWindow_SetVisible(true);
		}
	}
}// Init


/*!
This method cleans up the session status window by
destroying data structures allocated by InfoWindow_Init().

(3.0)
*/
void
InfoWindow_Done	()
{
	// save visibility preferences implicitly
	{
		Boolean		windowIsVisible = false;
		
		
		windowIsVisible = InfoWindow_IsVisible();
		(Preferences_Result)Preferences_SetData(kPreferences_TagWasSessionInfoShowing,
												sizeof(Boolean), &windowIsVisible);
	}
	
	Commands_StopHandlingExecution(kCommandShowConnectionStatus, gInfoWindowDisplayEventListener);
	Commands_StopHandlingExecution(kCommandHideConnectionStatus, gInfoWindowDisplayEventListener);
	ListenerModel_ReleaseListener(&gInfoWindowDisplayEventListener);
	
	SessionFactory_StopMonitoringSessions(kSession_ChangeStateAttributes, gSessionStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeState, gSessionStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowTitle, gSessionAttributeChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowObscured, gSessionAttributeChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeResourceLocation, gSessionAttributeChangeEventListener);
	ListenerModel_ReleaseListener(&gSessionStateChangeEventListener);
	ListenerModel_ReleaseListener(&gSessionAttributeChangeEventListener);
}// Done


/*!
Returns "true" only if the session status window is
set to be visible.

(3.0)
*/
Boolean
InfoWindow_IsVisible ()
{
	return ([[[InfoWindow_Controller sharedInfoWindowController] window] isVisible]) ? true : false;
}// IsVisible


/*!
Returns the currently highlighted session in the list,
or nullptr if there is nothing selected.

(3.1)
*/
SessionRef
InfoWindow_ReturnSelectedSession ()
{
	SessionRef				result = nullptr;
	InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
	int						selectedRow = [controller->infoTable selectedRow];
	
	
	if (selectedRow >= 0)
	{
		InfoWindow_SessionRow*	selectedRowData = [controller infoForRow:selectedRow];
		
		
		if (nil != selectedRowData)
		{
			result = selectedRowData->session;
		}
	}
	return result;
}// ReturnSelectedSession


/*!
Shows or hides the session status window.

(3.0)
*/
void
InfoWindow_SetVisible	(Boolean	inIsVisible)
{
	if (inIsVisible)
	{
		[[InfoWindow_Controller sharedInfoWindowController] showWindow:NSApp];
	}
	else
	{
		[[InfoWindow_Controller sharedInfoWindowController] close];
	}
}// SetVisible


#pragma mark Internal Methods
namespace {

/*!
Redraws the list display.  Use this when you have caused
it to change in some way (by closing a session window,
for example).

(3.0)
*/
void
refreshDisplay ()
{
	InfoWindow_Controller*		controller = [InfoWindow_Controller sharedInfoWindowController];
	InfoWindow_SessionRow*		selectedRowData = nil;
	int							oldSelectedRow = [controller->infoTable selectedRow];
	
	
	// update the new Cocoa-based table
	if (oldSelectedRow >= 0)
	{
		selectedRowData = [controller infoForRow:oldSelectedRow];
	}
	[controller->dataArray sortUsingDescriptors:[controller->infoTable sortDescriptors]];
	[controller->infoTable reloadData];
	if (nil != selectedRowData)
	{
		[controller->infoTable selectRowIndexes:[NSIndexSet indexSetWithIndex:
															[controller->dataArray indexOfObject:selectedRowData]]
												byExtendingSelection:NO];
	}
}// refreshDisplay


/*!
Invoked whenever the title of a session window is
changed; this routine responds by updating the
appropriate text in the list.

(3.0)
*/
void
sessionAttributeChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inSessionChange,
							 void*					inEventContextPtr,
							 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inSessionChange)
	{
	case kSession_ChangeWindowObscured:
		// update list item text to show dimmed status icon, if appropriate
		{
			Boolean				isObscured = false;
			SessionRef			session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			TerminalWindowRef	terminalWindow = Session_ReturnActiveTerminalWindow(session);
			
			
			if (nullptr != terminalWindow)
			{
				InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
				CFStringRef				iconNameCFString = nullptr;
				
				
				isObscured = TerminalWindow_IsObscured(terminalWindow);
				if (Session_GetStateIconName(session, iconNameCFString).ok())
				{
					InfoWindow_SessionRow*	rowData = [controller infoForSession:session];
					NSImage*				iconImage = [NSImage imageNamed:(NSString*)iconNameCFString];
					BOOL					releaseImage = NO;
					
					
					if (isObscured)
					{
						// apply a transform to make the icon appear dimmed
						NSSize		imageSize = [iconImage size];
						NSImage*	dimmedImage = [[NSImage alloc] initWithSize:imageSize];
						
						
						[dimmedImage lockFocus];
						[iconImage drawInRect:NSMakeRect(0, 0, imageSize.width, imageSize.height)
												fromRect:NSMakeRect(0, 0, imageSize.width, imageSize.height)
												operation:NSCompositeCopy fraction:0.5/* alpha */];
						[dimmedImage unlockFocus];
						iconImage = dimmedImage;
						releaseImage = YES;
					}
					[rowData setObject:iconImage forKey:kMyInfoColumnStatus];
					refreshDisplay();
					if (releaseImage)
					{
						[iconImage release];
					}
				}
			}
		}
		break;
	
	case kSession_ChangeResourceLocation:
		// update list item text to show new resource location
		{
			SessionRef				session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			CFStringRef				newURLCFString = Session_ReturnResourceLocationCFString(session);
			InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
			
			
			[[controller infoForSession:session] setObject:(NSString*)newURLCFString
															forKey:kMyInfoColumnCommand];
			refreshDisplay();
		}
		break;
	
	case kSession_ChangeWindowTitle:
		// update list item text to show new window title
		{
			SessionRef				session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			CFStringRef				titleCFString = nullptr;
			InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
			
			
			if (Session_GetWindowUserDefinedTitle(session, titleCFString).ok())
			{
				[[controller infoForSession:session] setObject:(NSString*)titleCFString
																forKey:kMyInfoColumnWindow];
			}
			else
			{
				[[controller infoForSession:session] setObject:[NSString string]
																forKey:kMyInfoColumnWindow];
			}
			refreshDisplay();
		}
		break;
	
	default:
		// ???
		break;
	}
}// sessionAttributeChanged


/*!
Invoked whenever a monitored session state is changed
(see InfoWindow_Init() to see which states are monitored).
This routine responds by ensuring that the list is up to
date for the changed session state.

(3.0)
*/
void
sessionStateChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inSessionChange,
						 void*					inEventContextPtr,
						 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inSessionChange)
	{
	case kSession_ChangeState:
		// update list item text to reflect new session state
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
			switch (Session_ReturnState(session))
			{
			case kSession_StateBrandNew:
				// here for completeness; there is no way to find out about transitions to the initial state
				break;
			
			case kSession_StateInitialized:
				// change appropriate list item’s contents - unimplemented
				break;
			
			case kSession_StateActiveUnstable:
				{
					CFStringRef				titleCFString = nullptr;
					InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
					InfoWindow_SessionRow*	newInfo = [[InfoWindow_SessionRow alloc]
														initWithSession:session
																		andActivationTime:Session_TimeOfActivation
																							(session)];
					CFStringRef				iconNameCFString = nullptr;
					CFLocaleRef				userLocale = CFLocaleCopyCurrent();
					CFDateFormatterRef		dateFormatter = CFDateFormatterCreate
															(kCFAllocatorDefault, userLocale,
																kCFDateFormatterNoStyle/* date style */,
																kCFDateFormatterLongStyle/* time style */);
					Boolean					setDateOK = false;
					
					
					CFRelease(userLocale), userLocale = nullptr;
					if (nullptr != dateFormatter)
					{
						CFStringRef		creationTimeCFString = CFDateFormatterCreateStringWithAbsoluteTime
																(kCFAllocatorDefault, dateFormatter,
																	newInfo->activationAbsoluteTime);
						
						
						if (nullptr != creationTimeCFString)
						{
							[newInfo setObject:(NSString*)creationTimeCFString
												forKey:kMyInfoColumnCreationTime];
							setDateOK = true;
							CFRelease(creationTimeCFString), creationTimeCFString = nullptr;
						}
						CFRelease(dateFormatter), dateFormatter = nullptr;
					}
					if (false == setDateOK)
					{
						[newInfo setObject:[NSString string] forKey:kMyInfoColumnCreationTime];
					}
					if (Session_GetStateIconName(session, iconNameCFString).ok())
					{
						NSImage*	iconImage = [NSImage imageNamed:(NSString*)iconNameCFString];
						
						
						[newInfo setObject:iconImage forKey:kMyInfoColumnStatus];
					}
					[newInfo setObject:(NSString*)(Session_ReturnResourceLocationCFString(session))
										forKey:kMyInfoColumnCommand];
					[newInfo setObject:(NSString*)(Session_ReturnPseudoTerminalDeviceNameCFString(session))
										forKey:kMyInfoColumnDevice];
					if (Session_GetWindowUserDefinedTitle(session, titleCFString).ok())
					{
						[newInfo setObject:(NSString*)titleCFString forKey:kMyInfoColumnWindow];
					}
					else
					{
						[newInfo setObject:[NSString string] forKey:kMyInfoColumnWindow];
					}
					// if a session window was terminated and then reopened, it is possible
					// that there is still an entry in the table for the session that has
					// (again) become active; so, remove any old item first
					[controller removeSession:session];
					[controller->dataArray addObject:newInfo];
					[controller->infoTable reloadData];
					[newInfo release];
				}
				break;
			
			case kSession_StateActiveStable:
				// change appropriate list item’s contents
				refreshDisplay();
				break;
			
			case kSession_StateDead:
				// change appropriate list item’s contents
				{
					InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
					CFStringRef				iconNameCFString = nullptr;
					
					
					if (Session_GetStateIconName(session, iconNameCFString).ok())
					{
						InfoWindow_SessionRow*	rowData = [controller infoForSession:session];
						NSImage*				iconImage = [NSImage imageNamed:(NSString*)iconNameCFString];
						
						
						[rowData setObject:iconImage forKey:kMyInfoColumnStatus];
					}
				}
				refreshDisplay();
				break;
			
			case kSession_StateImminentDisposal:
				// delete the row in the list corresponding to the disappearing session, and its window title and URL caches
				{
					InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
					
					
					[controller removeSession:session];
				}
				refreshDisplay();
				break;
			
			default:
				// other session status values have no effect on the session status window!
				break;
			}
		}
		break;
	
	case kSession_ChangeStateAttributes:
		// change appropriate list item’s contents
		{
			SessionRef				session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			InfoWindow_Controller*	controller = [InfoWindow_Controller sharedInfoWindowController];
			CFStringRef				iconNameCFString = nullptr;
			
			
			if (Session_GetStateIconName(session, iconNameCFString).ok())
			{
				InfoWindow_SessionRow*	rowData = [controller infoForSession:session];
				NSImage*				iconImage = [NSImage imageNamed:(NSString*)iconNameCFString];
				
				
				[rowData setObject:iconImage forKey:kMyInfoColumnStatus];
			}
		}
		refreshDisplay();
		break;
	
	default:
		// ???
		break;
	}
}// sessionStateChanged


/*!
Invoked when the visible state of the Session Info
Window should be changed.

The result is "eventNotHandledErr" if the command was
not actually executed - this frees other possible
handlers to take a look.  Any other return value
including "noErr" terminates the command sequence.

(3.0)
*/
OSStatus
showHideInfoWindow	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	UNUSED_ARGUMENT(inCommandID),
					 void*					inEventContextPtr,
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Commands_ExecutionEventContextPtr	commandInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																			Commands_ExecutionEventContextPtr);
	OSStatus							result = eventNotHandledErr;
	
	
	assert((kCommandShowConnectionStatus == commandInfoPtr->commandID) ||
			(kCommandHideConnectionStatus == commandInfoPtr->commandID));
	InfoWindow_SetVisible(kCommandShowConnectionStatus == commandInfoPtr->commandID);
	return result;
}// showHideInfoWindow

} // anonymous namespace


/*!
To avoid compiler warnings, and inform the compiler about
unknown methods, this class is declared.  (MacTelnet does not
yet compile specifically for newer OSes, even though it can
run on them.  It occasionally uses "respondsToSelector:" to
invoke APIs that older headers have not defined.)
*/
@interface NSWindow (NSWindowExtensionsFromLeopard)
- (void)
setCollectionBehavior:(unsigned int)	aBehavior;
@end


@implementation InfoWindow_SessionRow

- (id)
initWithSession:(SessionRef)		aSession
andActivationTime:(CFAbsoluteTime)	aTimeInterval
{
	self = [super init];
	if (nil != self)
	{
		// TEMPORARY: so far there is no concept of retaining/releasing sessions, but there should be...
		self->session = aSession;
		self->activationAbsoluteTime = aTimeInterval;
		self->dataByKey = [[NSMutableDictionary alloc] init];
	}
	return self;
}
- (void)
dealloc
{
	[dataByKey release];
	[super dealloc];
}// dealloc


/*!
This accessor is only needed for sort descriptor bindings,
and its name is used as a column sort key in the NIB.

(4.0)
*/
- (NSString*)
dataCommand
{
	return [dataByKey objectForKey:kMyInfoColumnCommand];
}// dataCommand


/*!
This accessor is only needed for sort descriptor bindings,
and its name is used as a column sort key in the NIB.

It returns an arbitrary numerical value to represent the
time; as far as sorting is concerned, this is sufficient.

(4.0)
*/
- (NSNumber*)
dataCreationTime
{
	return [NSNumber numberWithInt:Session_TimeOfActivation(self->session)];
}// dataCreationTime


/*!
This accessor is only needed for sort descriptor bindings,
and its name is used as a column sort key in the NIB.

(4.0)
*/
- (NSString*)
dataDevice
{
	return [dataByKey objectForKey:kMyInfoColumnDevice];
}// dataDevice


/*!
This accessor is only needed for sort descriptor bindings,
and its name is used as a column sort key in the NIB.

It returns an arbitrary numerical value to represent the
current status; as long as the result is consistent for
all sessions with the same state, the sorting will work.

(4.0)
*/
- (NSNumber*)
dataStatus
{
	Session_State				currentState = Session_ReturnState(self->session);
	Session_StateAttributes		currentAttributes = Session_ReturnStateAttributes(self->session);
	
	
	return [NSNumber numberWithInt:(currentState + currentAttributes)/* arbitrary */];
}// dataStatus


/*!
This accessor is only needed for sort descriptor bindings,
and its name is used as a column sort key in the NIB.

(4.0)
*/
- (NSString*)
dataWindow
{
	return [dataByKey objectForKey:kMyInfoColumnWindow];
}// dataWindow


/*!
Returns the display value for the specified key, which will
either be a string or an image, depending on the column key.

Allowed keys are one of the "kMyInfoColumn..." constants.

(4.0)
*/
- (id)
objectForKey:(NSString*)	aKey
{
	return [dataByKey objectForKey:aKey];
}// objectForKey:


/*!
Sets a new value for part of a table row.

Allowed keys are one of the "kMyInfoColumn..." constants.
Therefore, the NIB should be set up to use only those strings
as table column keys.

(4.0)
*/
- (void)
setObject:(id)		anObject
forKey:(NSString*)	aKey
{
	[dataByKey setObject:anObject forKey:aKey];
}// setObject:forKey:

@end // InfoWindow_SessionRow


@implementation InfoWindow_ToolbarItemNewSessionDefaultFavorite

- (id)
init
{
	self = [super initWithItemIdentifier:kMyToolbarItemIDNewSessionDefaultFavorite];
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
}
- (void)
dealloc
{
	[super dealloc];
}// dealloc


- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionDefaultFavorite);
}// performToolbarItemAction:

@end // InfoWindow_ToolbarItemNewSessionDefaultFavorite


@implementation InfoWindow_ToolbarItemNewSessionLogInShell

- (id)
init
{
	self = [super initWithItemIdentifier:kMyToolbarItemIDNewSessionLogInShell];
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
}
- (void)
dealloc
{
	[super dealloc];
}// dealloc


- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionLoginShell);
}// performToolbarItemAction:

@end // InfoWindow_ToolbarItemNewSessionLogInShell


@implementation InfoWindow_ToolbarItemNewSessionShell

- (id)
init
{
	self = [super initWithItemIdentifier:kMyToolbarItemIDNewSessionShell];
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
}
- (void)
dealloc
{
	[super dealloc];
}// dealloc


- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionShell);
}// performToolbarItemAction:

@end // InfoWindow_ToolbarItemNewSessionShell


@implementation InfoWindow_ToolbarItemStackWindows

- (id)
init
{
	self = [super initWithItemIdentifier:kMyToolbarItemIDStackWindows];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnStackWindowsIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Arrange All Windows in Front", @"toolbar item name; for stacking windows")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}
- (void)
dealloc
{
	[super dealloc];
}// dealloc


- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandStackWindows);
}// performToolbarItemAction:

@end // InfoWindow_ToolbarItemStackWindows


@implementation InfoWindow_Controller

static InfoWindow_Controller*	gInfoWindow_Controller = nil;
+ (id)
sharedInfoWindowController
{
	if (nil == gInfoWindow_Controller)
	{
		gInfoWindow_Controller = [[[self class] allocWithZone:NULL] init];
	}
	return gInfoWindow_Controller;
}// sharedInfoWindowController

/*!
Constructor.

(4.0)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"InfoWindowCocoa"];
	if (nil != self)
	{
		self->dataArray = [[NSMutableArray alloc] init]; // elements are of type InfoWindow_SessionRow*
	}
	return self;
}
- (void)
dealloc
{
	[dataArray release];
	[super dealloc];
}// dealloc


- (InfoWindow_SessionRow*)
infoForRow:(int)	row
{
	InfoWindow_SessionRow*	result = nil;
	
	
	if ((row >= 0) && (row < [self numberOfRowsInTableView:self->infoTable]))
	{
		id		infoObject = [self->dataArray objectAtIndex:row];
		assert([infoObject isKindOfClass:[InfoWindow_SessionRow class]]);
		
		
		result = (InfoWindow_SessionRow*)infoObject;
	}
	return result;
}// infoForRow:


- (InfoWindow_SessionRow*)
infoForSession:(SessionRef)		aSession
{
	InfoWindow_SessionRow*	result = nil;
	NSEnumerator*			eachObject = [self->dataArray objectEnumerator];
	id						currentObject = nil;
	
	
	while ((currentObject = [eachObject nextObject]))
	{
		assert([currentObject isKindOfClass:[InfoWindow_SessionRow class]]);
		InfoWindow_SessionRow*	asSessionRow = (InfoWindow_SessionRow*)currentObject;
		
		
		if (aSession == asSessionRow->session)
		{
			result = asSessionRow;
			break;
		}
	}
	return result;
}// infoForSession:


- (void)
didDoubleClickInView:(id)	sender
{
	if ([sender isKindOfClass:[NSTableView class]])
	{
		NSTableView*	asTableView = (NSTableView*)sender;
		//int			clickedColumn = [asTableView clickedColumn]; // not important in this case
		int				clickedRow = [asTableView clickedRow];
		
		
		if (clickedRow >= 0)
		{
			InfoWindow_SessionRow*	rowData = [self infoForRow:clickedRow];
			
			
			if (nil != rowData)
			{
				Session_Select(rowData->session);
			}
		}
	}
}// didDoubleClickInView:


- (void)
removeSession:(SessionRef)		aSession
{
	NSEnumerator*	eachObject = [self->dataArray objectEnumerator];
	id				currentObject = nil;
	int				deletedIndex = -1;
	int				i = 0;
	
	
	for (; ((currentObject = [eachObject nextObject])); ++i)
	{
		assert([currentObject isKindOfClass:[InfoWindow_SessionRow class]]);
		InfoWindow_SessionRow*	asSessionRow = (InfoWindow_SessionRow*)currentObject;
		
		
		if (aSession == asSessionRow->session)
		{
			deletedIndex = i;
			break;
		}
	}
	if (deletedIndex >= 0)
	{
		[self->dataArray removeObjectAtIndex:deletedIndex];
	}
}// removeSession:


#pragma mark NSTableDataSource

- (int)
numberOfRowsInTableView:(NSTableView*)	tableView
{
	assert(self->infoTable == tableView);
	return [self->dataArray count];
}// numberOfRowsInTableView:


- (id)
tableView:(NSTableView*)					tableView
objectValueForTableColumn:(NSTableColumn*)	tableColumn
row:(int)									row
{
	assert(self->infoTable == tableView);
	return [[self infoForRow:row] objectForKey:[tableColumn identifier]];
}// tableView:objectValueForTableColumn:row:


- (void)
tableView:(NSTableView*)			tableView
setObjectValue:(id)					object
forTableColumn:(NSTableColumn*)		tableColumn
row:(int)							row
{
	NSString*	columnKey = [tableColumn identifier];
	
	
	assert(self->infoTable == tableView);
	[[self infoForRow:row] setObject:object forKey:columnKey];
	if ([columnKey isEqualToString:kMyInfoColumnWindow])
	{
		assert([object isKindOfClass:[NSString class]]);
		NSString*				asString = (NSString*)object;
		InfoWindow_SessionRow*	rowData = [self infoForRow:row];
		
		
		if (nil != rowData)
		{
			Session_SetWindowUserDefinedTitle(rowData->session, (CFStringRef)asString);
		}
	}
}// tableView:setObjectValue:forTableColumn:row:


- (void)tableView:(NSTableView*)		tableView
sortDescriptorsDidChange:(NSArray*)		oldDescriptors
{
#pragma unused(oldDescriptors)
	InfoWindow_SessionRow*	selectedRowData = nil;
	int						oldSelectedRow = [tableView selectedRow];
	
	
	if (oldSelectedRow >= 0)
	{
		selectedRowData = [self infoForRow:oldSelectedRow];
	}
	[self->dataArray sortUsingDescriptors:[tableView sortDescriptors]];
	[tableView reloadData];
	if (nil != selectedRowData)
	{
		[tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:[self->dataArray indexOfObject:selectedRowData]]
									byExtendingSelection:NO];
	}
}// tableView:sortDescriptorsDidChange:


#pragma mark NSTableViewDelegate


- (BOOL)
tableView:(NSTableView*)					tableView
shouldSelectTableColumn:(NSTableColumn*)	tableColumn
{
#pragma unused(tableView, tableColumn)
	return NO;
}// tableView:shouldSelectTableColumn:


/*!
Returns the textual status string as the tooltip for
the icon column of the table.

Note that this was only added to the delegate in
Mac OS X 10.4.

(4.0)
*/
- (NSString*)
tableView:(NSTableView*)		tableView
toolTipForCell:(NSCell*)		aCell
rect:(NSRectPointer)			rect
tableColumn:(NSTableColumn*)	aTableColumn
row:(int)						row
mouseLocation:(NSPoint)			mouseLocation
{
#pragma unused(tableView, aCell, rect, aTableColumn, mouseLocation)
	InfoWindow_SessionRow*	rowData = [self infoForRow:row];
	NSString*				result = nil;
	
	
	if (nil != rowData)
	{
		if ([[aTableColumn identifier] isEqualToString:kMyInfoColumnStatus])
		{
			CFStringRef		stateCFString = nullptr;
			
			
			if (Session_GetStateString(rowData->session, stateCFString).ok())
			{
				result = (NSString*)stateCFString;
			}
		}
	}
	return result;
}// tableView:toolTipForCell:rect:tableColumn:row:mouseLocation:


#pragma mark NSToolbarDelegate


- (NSToolbarItem*)
toolbar:(NSToolbar*)				toolbar
itemForItemIdentifier:(NSString*)	itemIdentifier
willBeInsertedIntoToolbar:(BOOL)	flag
{
#pragma unused(toolbar, flag)
	NSToolbarItem*		result = nil;
	
	
	// NOTE: no need to handle standard items here
	// TEMPORARY - need to create all custom items
	if ([itemIdentifier isEqualToString:kMyToolbarItemIDNewSessionDefaultFavorite])
	{
		result = [[[InfoWindow_ToolbarItemNewSessionDefaultFavorite alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMyToolbarItemIDNewSessionLogInShell])
	{
		result = [[[InfoWindow_ToolbarItemNewSessionLogInShell alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMyToolbarItemIDNewSessionShell])
	{
		result = [[[InfoWindow_ToolbarItemNewSessionShell alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMyToolbarItemIDStackWindows])
	{
		result = [[[InfoWindow_ToolbarItemStackWindows alloc] init] autorelease];
	}
	return result;
}// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:


- (NSArray*)
toolbarAllowedItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	return [NSArray arrayWithObjects:
						kMyToolbarItemIDNewSessionDefaultFavorite,
						kMyToolbarItemIDNewSessionShell,
						kMyToolbarItemIDNewSessionLogInShell,
						NSToolbarCustomizeToolbarItemIdentifier,
						kMyToolbarItemIDStackWindows,
						NSToolbarSeparatorItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						NSToolbarFlexibleSpaceItemIdentifier,
						nil];
}// toolbarAllowedItemIdentifiers


- (NSArray*)
toolbarDefaultItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	return [NSArray arrayWithObjects:
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						NSToolbarFlexibleSpaceItemIdentifier,
						kMyToolbarItemIDNewSessionDefaultFavorite,
						NSToolbarSeparatorItemIdentifier,
						kMyToolbarItemIDNewSessionShell,
						NSToolbarSeparatorItemIdentifier,
						kMyToolbarItemIDNewSessionLogInShell,
						NSToolbarSeparatorItemIdentifier,
						NSToolbarCustomizeToolbarItemIdentifier,
						NSToolbarFlexibleSpaceItemIdentifier,
						kMyToolbarItemIDStackWindows,
						NSToolbarSpaceItemIdentifier,
						nil];
}// toolbarDefaultItemIdentifiers


#pragma mark NSWindowController

/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init...".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	assert(nil != infoTable);
	
	// make this window appear on all Spaces by default; note that this only
	// works on later versions of Mac OS X; scan for "NSWindowExtensionsFromLeopard"
	if ([[self window] respondsToSelector:@selector(setCollectionBehavior:)])
	{
		[(id)[self window] setCollectionBehavior:FUTURE_SYMBOL(1 << 0, NSWindowCollectionBehaviorCanJoinAllSpaces)];
	}
	
	// create toolbar; has to be done programmatically, because
	// IB only supports them in 10.5; which makes sense, you know,
	// since toolbars have only been in the OS since 10.0, and
	// hardly any applications would have found THOSE useful...
	{
		NSString*		toolbarID = @"SessionInfoToolbar"; // do not ever change this; that would only break user preferences
		NSToolbar*		windowToolbar = [[[NSToolbar alloc] initWithIdentifier:toolbarID] autorelease];
		
		
		// Check preferences for a stored toolbar; if one exists, leave the
		// toolbar display mode and size untouched, as the user will have
		// specified one; otherwise, initialize it to the desired look.
		//
		// IMPORTANT: This is a bit of a hack, as it relies on the key name
		// that Mac OS X happens to use for toolbar preferences as of 10.4.
		// If that ever changes, this code will be pointless.
		{
			CFPropertyListRef	toolbarConfigPref = CFPreferencesCopyAppValue
													(CFSTR("NSToolbar Configuration SessionInfoToolbar"),
														kCFPreferencesCurrentApplication);
			Boolean				usingSavedToolbar = false;
			if (nullptr != toolbarConfigPref)
			{
				usingSavedToolbar = true;
				CFRelease(toolbarConfigPref), toolbarConfigPref = nullptr;
			}
			unless (usingSavedToolbar)
			{
				[windowToolbar setDisplayMode:NSToolbarDisplayModeLabelOnly];
				[windowToolbar setSizeMode:NSToolbarSizeModeSmall];
			}
		}
		[windowToolbar setAllowsUserCustomization:YES];
		[windowToolbar setAutosavesConfiguration:YES];
		[windowToolbar setDelegate:self];
		[[self window] setToolbar:windowToolbar];
	}
	
	// arrange to detect double-clicks on table view rows
	[self->infoTable setDoubleAction:@selector(didDoubleClickInView:)];
	[self->infoTable setTarget:self];
}// windowDidLoad


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"SessionInfo";
}// windowFrameAutosaveName

@end // InfoWindow_Controller

// BELOW IS REQUIRED NEWLINE TO END FILE