/*!	\file InfoWindow.mm
	\brief A window showing status for all sessions.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#import <UniversalDefines.h>

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
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>
#import <TouchBar.objc++.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "InfoWindow.h"
#import "Session.h"
#import "SessionFactory.h"
#import "TerminalToolbar.objc++.h"
#import "UIStrings.h"



#pragma mark Constants
namespace {

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
@interface InfoWindow_SessionRow : NSObject //{
{
@public
	SessionRef				session;
	CFAbsoluteTime			activationAbsoluteTime;
	NSMutableDictionary*	dataByKey;
}

// initializers
	- (instancetype)
	initWithSession:(SessionRef)_
	andActivationTime:(CFAbsoluteTime)_;

// new methods
	- (id)
	objectForKey:(NSString*)_;
	- (void)
	setObject:(id)_
	forKey:(NSString*)_;

@end //}

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

/*!
The private class interface.
*/
@interface InfoWindow_Controller (InfoWindow_ControllerInternal) //{

// new methods
	- (InfoWindow_SessionRow*)
	infoForRow:(int)_;
	- (InfoWindow_SessionRow*)
	infoForSession:(SessionRef)_;
	- (void)
	removeSession:(SessionRef)_;

// methods of the form required by "setDoubleAction:"
	- (void)
	didDoubleClickInView:(id)_;

@end //}



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
		
		
		// get visibility preference for the Clipboard
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagWasSessionInfoShowing,
									sizeof(windowIsVisible), &windowIsVisible))
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
		UNUSED_RETURN(Preferences_Result)Preferences_SetData(kPreferences_TagWasSessionInfoShowing,
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


#pragma mark -
@implementation InfoWindow_SessionRow //{


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
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
}// initWithSession:andActivationTime:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[dataByKey release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


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
	return [NSNumber numberWithUnsignedInteger:STATIC_CAST(Session_TimeOfActivation(self->session), NSUInteger)];
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


@end //} InfoWindow_SessionRow


#pragma mark -
@implementation InfoWindow_Controller //{


static InfoWindow_Controller*	gInfoWindow_Controller = nil;


#pragma mark Class Methods


/*!
Returns the singleton.

(4.0)
*/
+ (instancetype)
sharedInfoWindowController
{
	if (nil == gInfoWindow_Controller)
	{
		gInfoWindow_Controller = [[self.class allocWithZone:NULL] init];
	}
	return gInfoWindow_Controller;
}// sharedInfoWindowController


#pragma mark Initializers


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super initWithWindowNibName:@"InfoWindowCocoa"];
	if (nil != self)
	{
		self->dataArray = [[NSMutableArray alloc] init]; // elements are of type InfoWindow_SessionRow*
		_touchBarController = nil; // created on demand
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
	[_touchBarController release];
	[dataArray release];
	[super dealloc];
}// dealloc


#pragma mark NSResponder


/*!
On OS 10.12.1 and beyond, returns a Touch Bar to display
at the top of the hardware keyboard (when available) or
in any Touch Bar simulator window.

This method should not be called except by the OS.

(2016.11)
*/
- (NSTouchBar*)
makeTouchBar
{
	NSTouchBar*		result = nil;
	
	
	if (nil == _touchBarController)
	{
		_touchBarController = [[TouchBar_Controller alloc] initWithNibName:@"InfoWindowTouchBarCocoa"];
		_touchBarController.customizationIdentifier = kConstantsRegistry_TouchBarIDInfoWindowMain;
		_touchBarController.customizationAllowedItemIdentifiers =
		@[
			FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFlexibleSpace",
							NSTouchBarItemIdentifierFlexibleSpace),
			FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceSmall",
							NSTouchBarItemIdentifierFixedSpaceSmall),
			FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceLarge",
							NSTouchBarItemIdentifierFixedSpaceLarge)
		];
		// (NOTE: default item identifiers are set in the NIB)
	}
	
	// the controller should force the NIB to load and define
	// the Touch Bar, using the settings above and in the NIB
	result = _touchBarController.touchBar;
	assert(nil != result);
	
	return result;
}// makeTouchBar


#pragma mark NSTableViewDataSource


/*!
Returns the size of the internal array.

(4.0)
*/
- (int)
numberOfRowsInTableView:(NSTableView*)	tableView
{
	if (self->infoTable == tableView)
	{
		return [self->dataArray count];
	}
	return 0;
}// numberOfRowsInTableView:


/*!
Returns the object that represents the specified row in the
table.

(4.0)
*/
- (id)
tableView:(NSTableView*)					tableView
objectValueForTableColumn:(NSTableColumn*)	tableColumn
row:(int)									row
{
	assert(self->infoTable == tableView);
	return [[self infoForRow:row] objectForKey:[tableColumn identifier]];
}// tableView:objectValueForTableColumn:row:


/*!
Changes the object that defines the data for the specified
column of the given row in the table.

(4.0)
*/
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
		assert([object isKindOfClass:NSString.class]);
		NSString*				asString = (NSString*)object;
		InfoWindow_SessionRow*	rowData = [self infoForRow:row];
		
		
		if (nil != rowData)
		{
			Session_SetWindowUserDefinedTitle(rowData->session, (CFStringRef)asString);
		}
	}
}// tableView:setObjectValue:forTableColumn:row:


/*!
Responds when the sorting criteria change for the table.

(4.0)
*/
- (void)
tableView:(NSTableView*)				tableView
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


/*!
Returns true only if the specified column can be selected
all at once.

(4.0)
*/
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
				result = BRIDGE_CAST(stateCFString, NSString*);
			}
		}
	}
	return result;
}// tableView:toolTipForCell:rect:tableColumn:row:mouseLocation:


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
	// TEMPORARY - need to create all custom items
	if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDNewSessionDefaultFavorite])
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
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDStackWindows])
	{
		result = [[[TerminalToolbar_ItemStackWindows alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDCustomize])
	{
		result = [[[TerminalToolbar_ItemCustomize alloc] init] autorelease];
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
	return @[
				kTerminalToolbar_ItemIDNewSessionDefaultFavorite,
				kTerminalToolbar_ItemIDNewSessionShell,
				kTerminalToolbar_ItemIDNewSessionLogInShell,
				kTerminalToolbar_ItemIDCustomize,
				kTerminalToolbar_ItemIDStackWindows,
				NSToolbarSpaceItemIdentifier,
				NSToolbarFlexibleSpaceItemIdentifier,
			];
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
	return @[
				NSToolbarSpaceItemIdentifier,
				NSToolbarSpaceItemIdentifier,
				NSToolbarSpaceItemIdentifier,
				NSToolbarSpaceItemIdentifier,
				NSToolbarSpaceItemIdentifier,
				NSToolbarFlexibleSpaceItemIdentifier,
				kTerminalToolbar_ItemIDNewSessionDefaultFavorite,
				NSToolbarSpaceItemIdentifier,
				kTerminalToolbar_ItemIDNewSessionShell,
				NSToolbarSpaceItemIdentifier,
				kTerminalToolbar_ItemIDNewSessionLogInShell,
				NSToolbarSpaceItemIdentifier,
				kTerminalToolbar_ItemIDCustomize,
				NSToolbarFlexibleSpaceItemIdentifier,
				kTerminalToolbar_ItemIDStackWindows,
				NSToolbarSpaceItemIdentifier,
			];
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
	
	[self.window setExcludedFromWindowsMenu:YES];
	
	// make this window appear on all Spaces by default; note that this only
	// works on later versions of Mac OS X; scan for "NSWindowExtensionsFromLeopard"
	if ([self.window respondsToSelector:@selector(setCollectionBehavior:)])
	{
		[(id)self.window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces];
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
		[self.window setToolbar:windowToolbar];
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


@end //} InfoWindow_Controller


#pragma mark -
@implementation InfoWindow_Controller (InfoWindow_ControllerInternal) //{


#pragma mark New Methods


/*!
Responds when the user double-clicks the specified row in the
table.

(4.0)
*/
- (void)
didDoubleClickInView:(id)	sender
{
	if ([sender isKindOfClass:NSTableView.class])
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


/*!
Returns an object representing the specified row in the table.

(4.0)
*/
- (InfoWindow_SessionRow*)
infoForRow:(int)	row
{
	InfoWindow_SessionRow*	result = nil;
	
	
	if ((row >= 0) && (row < [self numberOfRowsInTableView:self->infoTable]))
	{
		id		infoObject = [self->dataArray objectAtIndex:row];
		assert([infoObject isKindOfClass:InfoWindow_SessionRow.class]);
		
		
		result = (InfoWindow_SessionRow*)infoObject;
	}
	return result;
}// infoForRow:


/*!
Returns an object representing the specified session in the
table.

(4.0)
*/
- (InfoWindow_SessionRow*)
infoForSession:(SessionRef)		aSession
{
	InfoWindow_SessionRow*	result = nil;
	
	
	for (InfoWindow_SessionRow* sessionRowObject in self->dataArray)
	{
		if (aSession == sessionRowObject->session)
		{
			result = sessionRowObject;
			break;
		}
	}
	return result;
}// infoForSession:


/*!
Locates any object representing the specified session in the
table, and removes it.

(4.0)
*/
- (void)
removeSession:(SessionRef)		aSession
{
	int		deletedIndex = -1;
	int		i = 0;
	
	
	for (InfoWindow_SessionRow* sessionRowObject in self->dataArray)
	{
		if (aSession == sessionRowObject->session)
		{
			deletedIndex = i;
			break;
		}
		++i;
	}
	if (deletedIndex >= 0)
	{
		[self->dataArray removeObjectAtIndex:deletedIndex];
	}
}// removeSession:


@end //} InfoWindow_Controller (InfoWindow_ControllerInternal)


#pragma mark -
@implementation InfoWindow_Object //{


#pragma mark Initializers


/*!
Class initializer.

(2016.10)
*/
+ (void)
initialize
{
	// guard against subclass scenario by doing this at most once
	if (InfoWindow_Object.class == self)
	{
		BOOL	selectorFound = NO;
		
		
		selectorFound = CocoaExtensions_PerformSelectorOnTargetWithValue
						(@selector(setAllowsAutomaticWindowTabbing:), self.class, NO);
	}
}// initialize


/*!
Designated initializer.

(2016.10)
*/
- (instancetype)
initWithContentRect:(NSRect)		contentRect
styleMask:(NSUInteger)			windowStyle
backing:(NSBackingStoreType)		bufferingType
defer:(BOOL)					deferCreation
{
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if (nil != self)
	{
	}
	return self;
}// initWithContentRect:styleMask:backing:defer:


/*!
Destructor.

(2016.10)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


@end //} InfoWindow_Object

// BELOW IS REQUIRED NEWLINE TO END FILE
