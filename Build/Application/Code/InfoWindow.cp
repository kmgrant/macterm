/*###############################################################

	InfoWindow.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#include "UniversalDefines.h"

// standard-C includes
#include <climits>
#include <cstring>

// standard-C++ includes
#include <algorithm>
#include <map>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <Cursors.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "InfoWindow.h"
#include "Session.h"
#include "SessionFactory.h"
#include "UIStrings.h"



#pragma mark Constants

// The following MUST match what is in InfoWindow.nib!
// They also cannot use any of Apple’s reserved IDs (0 to 1023).
enum
{
	kMyDataBrowserPropertyIDWindow			= FOUR_CHAR_CODE('Wind'),
	kMyDataBrowserPropertyIDResource		= FOUR_CHAR_CODE('Rsrc'),
	kMyDataBrowserPropertyIDStatus			= FOUR_CHAR_CODE('Stat'),
	kMyDataBrowserPropertyIDCreationTime	= FOUR_CHAR_CODE('Open')
};

#pragma mark Types

typedef std::map< SessionRef, SInt32 >				SessionToCreationTimeMap;
typedef std::map< SessionRef, CFRetainRelease >		SessionToURLMap;
typedef std::map< SessionRef, CFRetainRelease >		SessionToWindowTitleMap;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowRef								gSessionStatusWindow = nullptr;
	WindowInfoRef							gSessionStatusWindowInfo = nullptr;
	HIViewRef								gSessionStatusDataBrowser = nullptr;
	Boolean									gIsShowing = false;
	CommonEventHandlers_WindowResizer		gSessionStatusWindowResizeHandler;
	ListenerModel_ListenerRef				gSessionAttributeChangeEventListener = nullptr;
	ListenerModel_ListenerRef				gSessionStateChangeEventListener = nullptr;
	ListenerModel_ListenerRef				gInfoWindowDisplayEventListener = nullptr;
	EventHandlerRef							gToolbarEventHandler = nullptr;			//!< invoked when the system needs a list of toolbar items, etc.
	EventHandlerUPP							gToolbarEventUPP = nullptr;				//!< wrapper for toolbar event callback function
	EventHandlerRef							gToolbarItemClickHandler = nullptr;		//!< invoked when a toolbar command is selected
	EventHandlerUPP							gToolbarItemClickUPP = nullptr;			//!< wrapper for window close callback function
	EventHandlerRef							gToolbarItemCreateHandler = nullptr;	//!< invoked when the system needs to create toolbar items
	EventHandlerUPP							gToolbarItemCreateUPP = nullptr;		//!< wrapper for toolbar item creation function
	EventHandlerRef							gWindowClosingHandler = nullptr;		//!< invoked when the session info window closes
	EventHandlerUPP							gWindowClosingUPP = nullptr;			//!< wrapper for window close callback function
	EventHandlerRef							gWindowCursorChangeHandler = nullptr;	//!< invoked when the cursor shape needs to be modified
	EventHandlerUPP							gWindowCursorChangeUPP = nullptr;		//!< wrapper for window cursor change callback function
	SessionToCreationTimeMap&				gSessionToCreationTimeMap ()	{ static SessionToCreationTimeMap x; return x; }
	SessionToURLMap&						gSessionToURLMap ()		{ static SessionToURLMap x; return x; }
	SessionToWindowTitleMap&				gSessionToWindowTitleMap ()		{ static SessionToWindowTitleMap x; return x; }
}

#pragma mark Internal Method Prototypes

static pascal OSStatus	accessDataBrowserItemData	(ControlRef, DataBrowserItemID, DataBrowserPropertyID,
														DataBrowserItemDataRef, Boolean);
static pascal Boolean	compareDataBrowserItems		(ControlRef, DataBrowserItemID, DataBrowserItemID, DataBrowserPropertyID);
static void				handleNewSize				(WindowRef, Float32, Float32, void*);
static pascal void		monitorDataBrowserItems		(ControlRef, DataBrowserItemID, DataBrowserItemNotification);
static pascal OSStatus	receiveHICommand			(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveToolbarRequest		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveWindowClosing		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveWindowCursorChange	(EventHandlerCallRef, EventRef, void*);
static void				refreshDisplay				();
static void				sessionAttributeChanged		(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				sessionStateChanged			(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				setDataBrowserColumnWidths	();
static OSStatus			showHideInfoWindow			(ListenerModel_Ref, ListenerModel_Event, void*, void*);



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
	WindowRef	sessionStatusWindow = nullptr;
	OSStatus	error = noErr;
	
	
	// load the NIB containing this window (automatically finds the right localization)
	sessionStatusWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("InfoWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	gSessionStatusWindow = sessionStatusWindow;
	
	// on 10.4, use the special unified toolbar appearance
	if (FlagManager_Test(kFlagOS10_4API))
	{
		(OSStatus)ChangeWindowAttributes
					(sessionStatusWindow,
						FUTURE_SYMBOL(1 << 7, kWindowUnifiedTitleAndToolbarAttribute)/* attributes to set */,
						0/* attributes to clear */);
	}
	
	// create toolbar icons
	{
		HIToolbarRef	commandIcons = nullptr;
		
		
		if (noErr == HIToolbarCreate(kConstantsRegistry_HIToolbarIDSessionInfo,
										kHIToolbarAutoSavesConfig | kHIToolbarIsConfigurable, &commandIcons))
		{
			// IMPORTANT: Do not invoke toolbar manipulation APIs at this stage,
			// until the event handlers below are installed.  A saved toolbar may
			// contain references to items that only the handlers below can create;
			// manipulation APIs often trigger creation of the entire toolbar, so
			// that means some saved items would fail to be inserted properly.
			
			// install a callback that can create any items needed for this
			// toolbar (used in the toolbar and in the customization sheet, etc.)
			{
				EventTypeSpec const		whenToolbarItemsRequired[] =
										{
											{ kEventClassToolbar, kEventToolbarCreateItemWithIdentifier }
										};
				
				
				gToolbarItemCreateUPP = NewEventHandlerUPP(Commands_HandleCreateToolbarItem);
				error = InstallEventHandler(HIObjectGetEventTarget(commandIcons), gToolbarItemCreateUPP,
											GetEventTypeCount(whenToolbarItemsRequired), whenToolbarItemsRequired,
											nullptr/* user data */, &gToolbarItemCreateHandler/* event handler reference */);
				assert_noerr(error);
			}
			
			// install a callback that specifies which items are in the toolbar by
			// default, and which items are available in the customization sheet
			{
				EventTypeSpec const		whenToolbarItemListRequired[] =
										{
											{ kEventClassToolbar, kEventToolbarGetAllowedIdentifiers },
											{ kEventClassToolbar, kEventToolbarGetDefaultIdentifiers }
										};
				
				
				gToolbarEventUPP = NewEventHandlerUPP(receiveToolbarRequest);
				error = InstallEventHandler(HIObjectGetEventTarget(commandIcons), gToolbarEventUPP,
											GetEventTypeCount(whenToolbarItemListRequired), whenToolbarItemListRequired,
											nullptr/* user data */, &gToolbarEventHandler/* event handler reference */);
				assert_noerr(error);
			}
			
			// Check preferences for a stored toolbar; if one exists, leave the
			// toolbar display mode and size untouched, as the user will have
			// specified one; otherwise, initialize it to the desired look.
			//
			// IMPORTANT: This is a bit of a hack, as it relies on the key name
			// that Mac OS X happens to use for toolbar preferences as of 10.4.
			// If that ever changes, this code will be pointless.
			CFPropertyListRef	toolbarConfigPref = CFPreferencesCopyAppValue
													(CFSTR("HIToolbar Config com.mactelnet.toolbar.sessioninfo"),
														kCFPreferencesCurrentApplication);
			Boolean				usingSavedToolbar = false;
			if (nullptr != toolbarConfigPref)
			{
				usingSavedToolbar = true;
				CFRelease(toolbarConfigPref), toolbarConfigPref = nullptr;
			}
			unless (usingSavedToolbar)
			{
				(OSStatus)HIToolbarSetDisplayMode(commandIcons, kHIToolbarDisplayModeLabelOnly);
				(OSStatus)HIToolbarSetDisplaySize(commandIcons, kHIToolbarDisplaySizeSmall);
			}
			
			// put the toolbar in the window
			error = SetWindowToolbar(sessionStatusWindow, commandIcons);
			assert_noerr(error);
			error = ShowHideWindowToolbar(sessionStatusWindow, true/* show */, false/* animate */);
			assert_noerr(error);
			
			CFRelease(commandIcons);
		}
	}
	
	// create the window’s controls
	{
		DataBrowserCallbacks	callbacks;
		HIViewID				id;
		
		
		// find data browser (created in NIB)
		id.signature = FOUR_CHAR_CODE('List'); // MUST match InfoWindow.nib!
		id.id = 0;
		error = GetControlByID(sessionStatusWindow, &id, &gSessionStatusDataBrowser);
		assert_noerr(error);
		
		// define a callback for specifying what data belongs in the list
		callbacks.version = kDataBrowserLatestCallbacks;
		error = InitDataBrowserCallbacks(&callbacks);
		assert_noerr(error);
		callbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(accessDataBrowserItemData);
		assert(nullptr != callbacks.u.v1.itemDataCallback);
		callbacks.u.v1.itemCompareCallback = NewDataBrowserItemCompareUPP(compareDataBrowserItems);
		assert(nullptr != callbacks.u.v1.itemCompareCallback);
		callbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(monitorDataBrowserItems);
		assert(nullptr != callbacks.u.v1.itemNotificationCallback);
		
		// attach data not specified in NIB
		error = SetDataBrowserCallbacks(gSessionStatusDataBrowser, &callbacks);
		assert_noerr(error);
		
		// initialize sort column
		error = SetDataBrowserSortProperty(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDCreationTime);
		assert_noerr(error);
		
		// set other nice things (most are set in the NIB already)
		if (FlagManager_Test(kFlagOS10_4API))
		{
			(OSStatus)DataBrowserChangeAttributes(gSessionStatusDataBrowser,
													FUTURE_SYMBOL(1 << 1, kDataBrowserAttributeListViewAlternatingRowColors)/* attributes to set */,
													0/* attributes to clear */);
		}
		(OSStatus)SetDataBrowserListViewUsePlainBackground(gSessionStatusDataBrowser, false);
		setDataBrowserColumnWidths();
		{
			DataBrowserPropertyFlags	flags = 0;
			
			
			error = GetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDWindow, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserPropertyIsMutable; // technically set already in the NIB, but just in case...
				error = SetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDWindow, flags);
			}
			error = GetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDResource, &flags);
			if (noErr == error)
			{
				error = SetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDResource, flags);
			}
			error = GetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDStatus, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserTruncateTextAtEnd; // this way, only the time/date information is lost first
				error = SetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDStatus, flags);
			}
			error = GetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDCreationTime, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserDateTimeTimeOnly; // show time of day in creation time column
				flags |= kDataBrowserDateTimeSecondsToo; // high granularity is useful; many windows could open in the same minute
				error = SetDataBrowserPropertyFlags(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDCreationTime, flags);
			}
		}
		
		// read user preferences for the column ordering, if any
		{
			Preferences_ResultCode	preferencesResult = kPreferences_ResultCodeSuccess;
			Preferences_ContextRef	globalContext = nullptr;
			
			
			preferencesResult = Preferences_GetDefaultContext(&globalContext);
			if (kPreferences_ResultCodeSuccess == preferencesResult)
			{
				CFArrayRef		orderCFArray = nullptr;
				
				
				preferencesResult = Preferences_ContextGetData(globalContext, kPreferences_TagInfoWindowColumnOrdering,
																sizeof(orderCFArray), &orderCFArray);
				if (kPreferences_ResultCodeSuccess == preferencesResult)
				{
					CFRange const	kWholeArray = CFRangeMake(0, CFArrayGetCount(orderCFArray));
					
					
					(OSStatus)SetDataBrowserTableViewColumnPosition
								(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDWindow,
									CFArrayGetFirstIndexOfValue(orderCFArray, kWholeArray, CFSTR("window")));
					(OSStatus)SetDataBrowserTableViewColumnPosition
								(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDResource,
									CFArrayGetFirstIndexOfValue(orderCFArray, kWholeArray, CFSTR("resource")));
					(OSStatus)SetDataBrowserTableViewColumnPosition
								(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDStatus,
									CFArrayGetFirstIndexOfValue(orderCFArray, kWholeArray, CFSTR("status")));
					(OSStatus)SetDataBrowserTableViewColumnPosition
								(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDCreationTime,
									CFArrayGetFirstIndexOfValue(orderCFArray, kWholeArray, CFSTR("open-time")));
					CFRelease(orderCFArray), orderCFArray = nullptr;
				}
			}
		}
	}
	
	// enable window drag tracking, so the data browser column moves and toolbar item drags work
	(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(gSessionStatusWindow, true);
	
	// set up the Window Info information
	gSessionStatusWindowInfo = WindowInfo_New();
	WindowInfo_SetWindowDescriptor(gSessionStatusWindowInfo, kConstantsRegistry_WindowDescriptorConnectionStatus);
	WindowInfo_SetWindowPotentialDropTarget(gSessionStatusWindowInfo, true/* can receive data via drag-and-drop */);
	WindowInfo_SetForWindow(sessionStatusWindow, gSessionStatusWindowInfo);
	
	// install a callback to find out about resizes (and to restrict window size)
	gSessionStatusWindowResizeHandler.install(gSessionStatusWindow, handleNewSize, nullptr/* user data */,
												550/* arbitrary minimum width */,
												90/* arbitrary minimum height */,
												1800/* arbitrary maximum width */,
												400/* arbitrary maximum height */);
	assert(gSessionStatusWindowResizeHandler.isInstalled());
	
	// concatenate MacTelnet’s name onto the end of the window title, so it’s obvious which
	// application’s session status window it is; also, on Mac OS X, set the item text for
	// the Dock’s window menu
	{
		Str255							titleString;
		Str255							appName;
		StringSubstitutionSpec const	metaMappings[] =
										{
											{ kStringSubstitutionDefaultTag1, appName }
										};
		
		
		GetWTitle(sessionStatusWindow, titleString);
		Localization_GetCurrentApplicationName(appName);
		StringUtilities_PBuild(titleString, sizeof(metaMappings) / sizeof(StringSubstitutionSpec), metaMappings);
		SetWTitle(sessionStatusWindow, titleString);
		
		// specify a different title for the Dock’s menu, one which doesn’t include the application name
		{
			CFStringRef				titleCFString = nullptr;
			UIStrings_ResultCode	stringResult = UIStrings_Copy(kUIStrings_SessionInfoWindowIconName, titleCFString);
			
			
			if (stringResult.ok())
			{
				SetWindowAlternateTitle(sessionStatusWindow, titleCFString);
				CFRelease(titleCFString);
			}
		}
	}
	
	// Place the session status window in the position indicated in the Human Interface Guidelines.
	// With themes, the window frame size cannot be guessed reliably.  Therefore, first, take
	// the difference between the left edges of the window content and structure region boxes
	// and make that the left edge of the window.
	{
		Rect	screenRect;
		Rect	structureRect;
		Rect	contentRect;
		
		
		RegionUtilities_GetPositioningBounds(sessionStatusWindow, &screenRect);
		(OSStatus)GetWindowBounds(sessionStatusWindow, kWindowContentRgn, &contentRect);
		(OSStatus)GetWindowBounds(sessionStatusWindow, kWindowStructureRgn, &structureRect);
		{
			Rect		initialBounds;
			Float32		fh = 0.0;
			Float32		fw = 0.0;
			
			
			// use main screen size to calculate the (arbitrary) initial session status window size
			fh = 0.14 * (screenRect.bottom - screenRect.top);
			fw = 0.65 * (screenRect.right - screenRect.left);
			
			// define the default window size, and make the window that size initially
			SetRect(&initialBounds, screenRect.left + (contentRect.left - structureRect.left) + 4,
									screenRect.bottom - ((short)fh) - 4, 0, 0);
			SetRect(&initialBounds, initialBounds.left, initialBounds.top,
									initialBounds.left + (short)fw,
									initialBounds.top + (short)fh);
			SetWindowStandardState(sessionStatusWindow, &initialBounds);
			(OSStatus)SetWindowBounds(sessionStatusWindow, kWindowContentRgn, &initialBounds);
		}
	}
	
	// install a callback that hides the window instead of destroying it, when it should be closed
	{
		EventTypeSpec const		whenWindowClosing[] =
								{
									{ kEventClassWindow, kEventWindowClose }
								};
		
		
		gWindowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
		error = InstallWindowEventHandler(sessionStatusWindow, gWindowClosingUPP, GetEventTypeCount(whenWindowClosing),
											whenWindowClosing, nullptr/* user data */,
											&gWindowClosingHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that sets the cursor for various parts of the data browser
	// (NOTE: perhaps this will not be necessary in future data browser versions;
	// right now, the standard handler does not seem to deal with cursors)
	{
		EventTypeSpec const		whenCursorChangeRequired[] =
								{
									{ kEventClassWindow, kEventWindowCursorChange }
								};
		
		
		gWindowCursorChangeUPP = NewEventHandlerUPP(receiveWindowCursorChange);
		error = InstallWindowEventHandler(sessionStatusWindow, gWindowCursorChangeUPP, GetEventTypeCount(whenCursorChangeRequired),
											whenCursorChangeRequired, sessionStatusWindow/* user data */,
											&gWindowCursorChangeHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that responds to toolbar item commands
	{
		EventTypeSpec const		whenToolbarItemHit[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		
		
		gToolbarItemClickUPP = NewEventHandlerUPP(receiveHICommand);
		error = InstallWindowEventHandler(sessionStatusWindow, gToolbarItemClickUPP,
											GetEventTypeCount(whenToolbarItemHit), whenToolbarItemHit,
											nullptr/* user data */, &gToolbarItemClickHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install notification routines to find out when session states change
	gSessionAttributeChangeEventListener = ListenerModel_NewStandardListener(sessionAttributeChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeResourceLocation, gSessionAttributeChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowObscured, gSessionAttributeChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowTitle, gSessionAttributeChangeEventListener);
	gSessionStateChangeEventListener = ListenerModel_NewStandardListener(sessionStateChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, gSessionStateChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeStateAttributes, gSessionStateChangeEventListener);
	
	// finally, ask to be told when a “show/hide info window” command occurs
	gInfoWindowDisplayEventListener = ListenerModel_NewOSStatusListener(showHideInfoWindow);
	Commands_StartHandlingExecution(kCommandShowConnectionStatus, gInfoWindowDisplayEventListener);
	Commands_StartHandlingExecution(kCommandHideConnectionStatus, gInfoWindowDisplayEventListener);
	
	// restore the visible state implicitly saved at last Quit
	{
		Boolean		windowIsVisible = false;
		size_t		actualSize = 0L;
		
		
		// get visibility preference for the Clipboard
		unless (Preferences_GetData(kPreferences_TagWasSessionInfoShowing,
									sizeof(windowIsVisible), &windowIsVisible,
									&actualSize) == kPreferences_ResultCodeSuccess)
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		InfoWindow_SetVisible(windowIsVisible);
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
		(Preferences_ResultCode)Preferences_SetData(kPreferences_TagWasSessionInfoShowing,
													sizeof(Boolean), &windowIsVisible);
	}
	
	// set user preferences for the column ordering; note that
	// GetDataBrowserUserState() could be used, however this
	// data is not easily generated and therefore not as good
	// as strings for placement in DefaultPreferences.plist
	{
		Preferences_ResultCode	preferencesResult = kPreferences_ResultCodeSuccess;
		Preferences_ContextRef	globalContext = nullptr;
		
		
		preferencesResult = Preferences_GetDefaultContext(&globalContext);
		if (kPreferences_ResultCodeSuccess == preferencesResult)
		{
			enum
			{
				kNumberOfColumns = 4	// IMPORTANT: set this to the number of CFArrayAppendValue() calls that are made
			};
			CFMutableArrayRef	orderCFArray = CFArrayCreateMutable(kCFAllocatorDefault, kNumberOfColumns,
																	&kCFTypeArrayCallBacks);
			
			
			if (nullptr != orderCFArray)
			{
				DataBrowserTableViewColumnIndex		windowColumnIndex = 0;
				DataBrowserTableViewColumnIndex		resourceColumnIndex = 0;
				DataBrowserTableViewColumnIndex		statusColumnIndex = 0;
				DataBrowserTableViewColumnIndex		openTimeColumnIndex = 0;
				CFStringRef							stringToAppend = nullptr;
				
				
				// find current column positions
				(OSStatus)GetDataBrowserTableViewColumnPosition
							(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDWindow, &windowColumnIndex);
				(OSStatus)GetDataBrowserTableViewColumnPosition
							(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDResource, &resourceColumnIndex);
				(OSStatus)GetDataBrowserTableViewColumnPosition
							(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDStatus, &statusColumnIndex);
				(OSStatus)GetDataBrowserTableViewColumnPosition
							(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDCreationTime, &openTimeColumnIndex);
				
				// add entries to the list for each column, in the right place
				for (CFIndex i = 0; i < kNumberOfColumns; ++i)
				{
					if (windowColumnIndex == STATIC_CAST(i, UInt32)) stringToAppend = CFSTR("window");
					else if (resourceColumnIndex == STATIC_CAST(i, UInt32)) stringToAppend = CFSTR("resource");
					else if (statusColumnIndex == STATIC_CAST(i, UInt32)) stringToAppend = CFSTR("status");
					else if (openTimeColumnIndex == STATIC_CAST(i, UInt32)) stringToAppend = CFSTR("open-time");
					
					if (stringToAppend != nullptr) CFArrayAppendValue(orderCFArray, stringToAppend);
				}
				
				preferencesResult = Preferences_ContextSetData(globalContext, kPreferences_TagInfoWindowColumnOrdering,
																sizeof(orderCFArray), &orderCFArray);
				CFRelease(orderCFArray), orderCFArray = nullptr;
			}
		}
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
	
	RemoveEventHandler(gWindowClosingHandler), gWindowClosingHandler = nullptr;
	DisposeEventHandlerUPP(gWindowClosingUPP), gWindowClosingUPP = nullptr;
	RemoveEventHandler(gWindowCursorChangeHandler), gWindowCursorChangeHandler = nullptr;
	DisposeEventHandlerUPP(gWindowCursorChangeUPP), gWindowCursorChangeUPP = nullptr;
	RemoveEventHandler(gToolbarEventHandler), gToolbarEventHandler = nullptr;
	DisposeEventHandlerUPP(gToolbarEventUPP), gToolbarEventUPP = nullptr;
	RemoveEventHandler(gToolbarItemClickHandler), gToolbarItemClickHandler = nullptr;
	DisposeEventHandlerUPP(gToolbarItemClickUPP), gToolbarItemClickUPP = nullptr;
	RemoveEventHandler(gToolbarItemCreateHandler), gToolbarItemCreateHandler = nullptr;
	DisposeEventHandlerUPP(gToolbarItemCreateUPP), gToolbarItemCreateUPP = nullptr;
	
	DisposeWindow(gSessionStatusWindow), gSessionStatusWindow = nullptr;
	WindowInfo_Dispose(gSessionStatusWindowInfo);
}// Done


/*!
Returns the Mac OS window pointer for the session
status window.

(3.0)
*/
WindowRef
InfoWindow_GetWindow ()
{
	return gSessionStatusWindow;
}// GetWindow


/*!
Returns "true" only if the session status window is
set to be visible.

(3.0)
*/
Boolean
InfoWindow_IsVisible ()
{
	return gIsShowing;
}// IsVisible


/*!
Shows or hides the session status window.

(3.0)
*/
void
InfoWindow_SetVisible	(Boolean	inIsVisible)
{
	WindowRef	sessionStatusWindow = InfoWindow_GetWindow();
	
	
	gIsShowing = inIsVisible;
	if (inIsVisible)
	{
		ShowWindow(sessionStatusWindow);
		EventLoop_SelectBehindDialogWindows(sessionStatusWindow);
	}
	else
	{
		HideWindow(sessionStatusWindow);
	}
}// SetVisible


#pragma mark Internal Methods

/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
static pascal OSStatus
accessDataBrowserItemData	(ControlRef					inDataBrowser,
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	OSStatus	result = noErr;
	SessionRef	session = REINTERPRET_CAST(inItemID, SessionRef);
	
	
	assert(gSessionStatusDataBrowser == inDataBrowser);
	if (false == inSetValue)
	{
		switch (inPropertyID)
		{
		case kDataBrowserItemIsEditableProperty:
			result = SetDataBrowserItemDataBooleanValue(inItemData, true/* is editable */);
			break;
		
		case kMyDataBrowserPropertyIDCreationTime:
			// return the creation date and time
			{
				SessionToCreationTimeMap::iterator	sessionToCreationTimeIterator;
				
				
				sessionToCreationTimeIterator = gSessionToCreationTimeMap().find(session);
				if (sessionToCreationTimeIterator != gSessionToCreationTimeMap().end())
				{
					SInt32		dateTime = gSessionToCreationTimeMap()[session];
					
					
					if (dateTime != 0)
					{
						result = SetDataBrowserItemDataDateTime(inItemData, dateTime);
					}
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDResource:
			// return the text string with the session resource location
			{
				SessionToURLMap::iterator	sessionToURLIterator;
				
				
				sessionToURLIterator = gSessionToURLMap().find(session);
				if (sessionToURLIterator != gSessionToURLMap().end())
				{
					CFStringRef		theCFString = gSessionToURLMap()[session].returnCFStringRef();
					
					
					if (theCFString != nullptr)
					{
						result = SetDataBrowserItemDataText(inItemData, theCFString);
					}
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDStatus:
			// return the text string representing the current state of the session
			{
				CFStringRef		statusCFString = nullptr;
				IconRef			statusIconRef = nullptr;
				
				
				if (Session_GetStateString(session, statusCFString).ok())
				{
					result = SetDataBrowserItemDataText(inItemData, statusCFString);
				}
				if (Session_CopyStateIconRef(session, statusIconRef).ok())
				{
					TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(session);
					
					
					// use the session’s ordinary status icon for this column,
					// but dim the icon if the window for the session is hidden
					result = SetDataBrowserItemDataIcon(inItemData, statusIconRef);
					if (nullptr != terminalWindow)
					{
						SetDataBrowserItemDataIconTransform(inItemData,
															TerminalWindow_IsObscured(terminalWindow)
															? kTransformDisabled
															: kTransformNone);
					}
					ReleaseIconRef(statusIconRef), statusIconRef = nullptr;
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDWindow:
			// return the text string with the window title
			{
				SessionToWindowTitleMap::iterator	sessionToWindowTitleIterator;
				
				
				sessionToWindowTitleIterator = gSessionToWindowTitleMap().find(session);
				if (sessionToWindowTitleIterator != gSessionToWindowTitleMap().end())
				{
					CFStringRef		titleCFString = gSessionToWindowTitleMap()[session].returnCFStringRef();
					
					
					if (nullptr != titleCFString)
					{
						result = SetDataBrowserItemDataText(inItemData, titleCFString);
					}
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	else
	{
		switch (inPropertyID)
		{
		case kMyDataBrowserPropertyIDCreationTime:
		case kMyDataBrowserPropertyIDResource:
		case kMyDataBrowserPropertyIDStatus:
			// read-only
			result = paramErr;
			break;
		
		case kMyDataBrowserPropertyIDWindow:
			// user has changed the window title; update the window
			{
				CFStringRef		newTitle = nullptr;
				
				
				result = GetDataBrowserItemDataText(inItemData, &newTitle);
				if (noErr == result)
				{
					Session_SetWindowUserDefinedTitle(session, newTitle);
					result = noErr;
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// accessDataBrowserItemData


/*!
A standard DataBrowserItemCompareProcPtr, this
method compares items in the list.

(3.1)
*/
static pascal Boolean
compareDataBrowserItems		(ControlRef					UNUSED_ARGUMENT(inDataBrowser),
							 DataBrowserItemID			inItemOne,
							 DataBrowserItemID			inItemTwo,
							 DataBrowserPropertyID		inSortProperty)
{
	SessionRef		session1 = REINTERPRET_CAST(inItemOne, SessionRef);
	SessionRef		session2 = REINTERPRET_CAST(inItemTwo, SessionRef);
	Boolean			result = false;
	
	
	if ((nullptr == session1) && (nullptr != session2)) result = true;
	else if ((nullptr != session1) && (nullptr != session2))
	{
		CFStringRef		string1 = nullptr;
		CFStringRef		string2 = nullptr;
		
		
		switch (inSortProperty)
		{
		case kMyDataBrowserPropertyIDCreationTime:
			{
				SessionToCreationTimeMap::iterator	sessionToCreationTimeIterator;
				UInt32								dateTime1 = 0;
				UInt32								dateTime2 = 0;
				
				
				sessionToCreationTimeIterator = gSessionToCreationTimeMap().find(session1);
				if (sessionToCreationTimeIterator != gSessionToCreationTimeMap().end())
				{
					dateTime1 = gSessionToCreationTimeMap()[session1];
				}
				
				sessionToCreationTimeIterator = gSessionToCreationTimeMap().find(session2);
				if (sessionToCreationTimeIterator != gSessionToCreationTimeMap().end())
				{
					dateTime2 = gSessionToCreationTimeMap()[session2];
				}
				
				if ((0 == dateTime1) && (0 != dateTime2)) result = true;
				else if ((0 == dateTime1) || (0 == dateTime2)) result = false;
				else
				{
					result = (dateTime1 < dateTime2);
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDResource:
			{
				SessionToURLMap::iterator	sessionToURLIterator;
				
				
				sessionToURLIterator = gSessionToURLMap().find(session1);
				if (sessionToURLIterator != gSessionToURLMap().end())
				{
					string1 = gSessionToURLMap()[session1].returnCFStringRef();
				}
				
				sessionToURLIterator = gSessionToURLMap().find(session2);
				if (sessionToURLIterator != gSessionToURLMap().end())
				{
					string2 = gSessionToURLMap()[session2].returnCFStringRef();
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDStatus:
			{
				(Session_ResultCode)Session_GetStateString(session1, string1);
				(Session_ResultCode)Session_GetStateString(session2, string2);
			}
			break;
		
		case kMyDataBrowserPropertyIDWindow:
			{
				SessionToWindowTitleMap::iterator	sessionToWindowTitleIterator;
				
				
				sessionToWindowTitleIterator = gSessionToWindowTitleMap().find(session1);
				if (sessionToWindowTitleIterator != gSessionToWindowTitleMap().end())
				{
					string1 = gSessionToWindowTitleMap()[session1].returnCFStringRef();
				}
				
				sessionToWindowTitleIterator = gSessionToWindowTitleMap().find(session2);
				if (sessionToWindowTitleIterator != gSessionToWindowTitleMap().end())
				{
					string2 = gSessionToWindowTitleMap()[session2].returnCFStringRef();
				}
			}
			break;
		
		default:
			// ???
			break;
		}
		
		// check for nullptr, because CFStringCompare() will not deal with it
		if ((nullptr == string1) && (nullptr != string2)) result = true;
		else if ((nullptr == string1) || (nullptr == string2)) result = false;
		else
		{
			result = (kCFCompareLessThan == CFStringCompare(string1, string2,
															kCFCompareCaseInsensitive | kCFCompareLocalized/* options */));
		}
	}
	return result;
}// compareDataBrowserItems


/*!
Adjusts the contents of the session information window based
on the specified change in dimensions.

(3.0)
*/
static void
handleNewSize	(WindowRef	inWindow,
				 Float32	inDeltaX,
				 Float32	inDeltaY,
				 void*		UNUSED_ARGUMENT(inContext))
{
	Rect	oldBrowserBounds;
	
	
	assert(inWindow == gSessionStatusWindow);
	GetControlBounds(gSessionStatusDataBrowser, &oldBrowserBounds);
	oldBrowserBounds.right += STATIC_CAST(inDeltaX, SInt16);
	oldBrowserBounds.bottom += STATIC_CAST(inDeltaY, SInt16);
	SetControlBounds(gSessionStatusDataBrowser, &oldBrowserBounds);
#if 0
	(OSStatus)AutoSizeDataBrowserListViewColumns(gSessionStatusDataBrowser);
#else
	setDataBrowserColumnWidths();
#endif
}// handleNewSize


/*!
Responds to changes in the data browser.  Currently,
most of the possible messages are ignored, but this
is used to select specific windows.

(3.1)
*/
static pascal void
monitorDataBrowserItems		(ControlRef						inDataBrowser,
							 DataBrowserItemID				inItemID,
							 DataBrowserItemNotification	inMessage)
{
	SessionRef	session = REINTERPRET_CAST(inItemID, SessionRef);
	
	
	assert(gSessionStatusDataBrowser == inDataBrowser);
	switch (inMessage)
	{
	case kDataBrowserItemDoubleClicked:
		Session_Select(session);
		break;
	
	default:
		// not all messages are supported
		break;
	}
}// monitorDataBrowserItems


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the Session Info window toolbar.  Responds by
executing the selected command.

(3.1)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// execute a command selected from the toolbar
				switch (received.commandID)
				{
				default:
					// ???
					break;
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Handles "kEventToolbarGetAllowedIdentifiers" and
"kEventToolbarGetDefaultIdentifiers" from "kEventClassToolbar"
for the Session Info window toolbar.  Responds by updating
the given lists of identifiers.

(3.1)
*/
static pascal OSStatus
receiveToolbarRequest	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassToolbar);
	{
		HIToolbarRef	toolbarRef = nullptr;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbar, typeHIToolbarRef, toolbarRef);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventToolbarGetAllowedIdentifiers:
				{
					CFMutableArrayRef	allowedIdentifiers = nullptr;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMutableArray,
																	typeCFMutableArrayRef, allowedIdentifiers);
					if (noErr == result)
					{
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDNewDefaultSession);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDNewShellSession);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDNewLoginSession);
						CFArrayAppendValue(allowedIdentifiers, kConstantsRegistry_HIToolbarItemIDStackWindows);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarSeparatorIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarSpaceIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarFlexibleSpaceIdentifier);
						CFArrayAppendValue(allowedIdentifiers, kHIToolbarCustomizeIdentifier);
					}
				}
				break;
			
			case kEventToolbarGetDefaultIdentifiers:
				{
					CFMutableArrayRef	defaultIdentifiers = nullptr;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMutableArray,
																	typeCFMutableArrayRef, defaultIdentifiers);
					if (noErr == result)
					{
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarFlexibleSpaceIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDNewDefaultSession);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSeparatorIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDNewShellSession);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSeparatorIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kConstantsRegistry_HIToolbarItemIDNewLoginSession);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarSeparatorIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarCustomizeIdentifier);
						CFArrayAppendValue(defaultIdentifiers, kHIToolbarFlexibleSpaceIdentifier);
					}
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// receiveToolbarRequest


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for the session info window.  The default handler
destroys windows, but the session info window should
only be hidden or shown (as it always remains in
memory).

(3.0)
*/
static pascal OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (noErr == result)
		{
			if (window == gSessionStatusWindow)
			{
				Commands_ExecuteByID(kCommandHideConnectionStatus);
				result = noErr; // event is completely handled
			}
			else
			{
				// ???
				result = eventNotHandledErr;
			}
		}
	}
	
	return result;
}// receiveWindowClosing


/*!
Handles "kEventWindowCursorChange" of "kEventClassWindow",
or "kEventRawKeyModifiersChanged" of "kEventClassKeyboard",
for the session status window.

(3.1)
*/
static pascal OSStatus
receiveWindowCursorChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert((kEventClass == kEventClassWindow) && (kEventKind == kEventWindowCursorChange));
	
	// do not change the cursor if this window is not active
	if (GetUserFocusWindow() == gSessionStatusWindow)
	{
		Point	globalMouse;
		
		
		Cursors_UseArrow();
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, globalMouse);
		if (noErr == result)
		{
			UInt32		modifiers = 0;
			Boolean		wasSet = false;
			
			
			// try to vary the cursor according to key modifiers, but it’s no
			// catastrophe if this information isn’t available
			(OSStatus)CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
			
			// finally, set the cursor
			{
				Point	localMouse = globalMouse;
				
				
				QDGlobalToLocalPoint(GetWindowPort(gSessionStatusWindow), &localMouse);
				result = HandleControlSetCursor(gSessionStatusDataBrowser, localMouse, modifiers, &wasSet);
			}
		}
	}
	
	return result;
}// receiveWindowCursorChange


/*!
Redraws the list display.  Use this when you have caused
it to change in some way (by closing a session window,
for example).

(3.0)
*/
static void
refreshDisplay ()
{
	(OSStatus)UpdateDataBrowserItems(gSessionStatusDataBrowser, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDWindow);
	(OSStatus)UpdateDataBrowserItems(gSessionStatusDataBrowser, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDResource);
	(OSStatus)UpdateDataBrowserItems(gSessionStatusDataBrowser, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDStatus);
	(OSStatus)UpdateDataBrowserItems(gSessionStatusDataBrowser, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDCreationTime);
}// refreshDisplay


/*!
Invoked whenever the title of a session window is
changed; this routine responds by updating the
appropriate text in the list.

(3.0)
*/
static void
sessionAttributeChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inSessionChange,
							 void*					inEventContextPtr,
							 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inSessionChange)
	{
	case kSession_ChangeWindowObscured:
		// update list item text to show dimmed status icon, if appropriate
		refreshDisplay();
		break;
	
	case kSession_ChangeResourceLocation:
		// update list item text to show new resource location
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			CFStringRef		newURLCFString = Session_GetResourceLocationCFString(session);
			
			
			gSessionToURLMap()[session].setCFTypeRef(newURLCFString);
			refreshDisplay();
		}
		break;
	
	case kSession_ChangeWindowTitle:
		// update list item text to show new window title
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			CFStringRef		titleCFString = nullptr;
			
			
			(Session_ResultCode)Session_GetWindowUserDefinedTitle(session, titleCFString);
			gSessionToWindowTitleMap()[session].setCFTypeRef(titleCFString);
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
static void
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
			
			
			switch (Session_GetState(session))
			{
			case kSession_StateBrandNew:
				// here for completeness; there is no way to find out about transitions to the initial state
				break;
			
			case kSession_StateInitialized:
				// change appropriate list item’s contents - unimplemented
				break;
			
			case kSession_StateActiveUnstable:
				{
					CFStringRef		titleCFString = nullptr;
					
					
					// find out all session information for initialization purposes
					// (registered callbacks handle any changes later on)
					gSessionToCreationTimeMap()[session] = Session_TimeOfActivation(session);
					gSessionToURLMap()[session].setCFTypeRef(Session_GetResourceLocationCFString(session));
					if (Session_GetWindowUserDefinedTitle(session, titleCFString).ok())
					{
						gSessionToWindowTitleMap()[session].setCFTypeRef(titleCFString);
					}
					
					// insert a row into the session list (the data responding routine will fill in its contents);
					// in order for the data routine to figure out the user-defined window title, a cache is created
					{
						DataBrowserItemID	ids[] = { REINTERPRET_CAST(session, DataBrowserItemID) };
						
						
						(OSStatus)AddDataBrowserItems(gSessionStatusDataBrowser, kDataBrowserNoItem/* parent item */,
														sizeof(ids) / sizeof(DataBrowserItemID), ids,
														kDataBrowserItemNoProperty/* pre-sort property */);
					}
				}
				break;
			
			case kSession_StateActiveStable:
				// change appropriate list item’s contents
				refreshDisplay();
				break;
			
			case kSession_StateDead:
				// change appropriate list item’s contents
				refreshDisplay();
				break;
			
			case kSession_StateImminentDisposal:
				// delete the row in the list corresponding to the disappearing session, and its window title and URL caches
				{
					DataBrowserItemID	ids[] = { REINTERPRET_CAST(session, DataBrowserItemID) };
					
					
					(OSStatus)RemoveDataBrowserItems(gSessionStatusDataBrowser, kDataBrowserNoItem/* parent item */,
														sizeof(ids) / sizeof(DataBrowserItemID), ids,
														kDataBrowserItemNoProperty/* pre-sort property */);
				}
				gSessionToCreationTimeMap().erase(session);
				gSessionToURLMap().erase(session);
				gSessionToWindowTitleMap().erase(session);
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
		refreshDisplay();
		break;
	
	default:
		// ???
		break;
	}
}// sessionStateChanged


/*!
Sets the widths of the data browser columns
proportionately based on the total width.

(3.1)
*/
static void
setDataBrowserColumnWidths ()
{
	Rect	containerRect;
	
	
	(OSStatus)GetWindowBounds(gSessionStatusWindow, kWindowContentRgn, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		availableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */;
		UInt16		integerWidth = 0;
		Float32		calculatedWidth = 0;
		UInt16		totalWidthSoFar = 0;
		
		
		// leave status string space fixed
		if (noErr == GetDataBrowserTableViewNamedColumnWidth(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDStatus,
																&integerWidth))
		{
			availableWidth -= integerWidth;
		}
		
		// leave creation time space fixed
		if (noErr == GetDataBrowserTableViewNamedColumnWidth(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDCreationTime,
																&integerWidth))
		{
			availableWidth -= integerWidth;
		}
		
		//
		// divide all remaining space between the variable-sized columns
		//
		
		// window title space is relatively small
		calculatedWidth = availableWidth * 0.40/* arbitrary */;
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDWindow, STATIC_CAST(calculatedWidth, UInt16));
		totalWidthSoFar += STATIC_CAST(calculatedWidth, UInt16);
		
		// remaining column receives all remaining space
		calculatedWidth = availableWidth - totalWidthSoFar;
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(gSessionStatusDataBrowser, kMyDataBrowserPropertyIDResource, availableWidth - totalWidthSoFar);
	}
}// setDataBrowserColumnWidths


/*!
Invoked when the visible state of the Session Info
Window should be changed.

The result is "eventNotHandledErr" if the command was
not actually executed - this frees other possible
handlers to take a look.  Any other return value
including "noErr" terminates the command sequence.

(3.0)
*/
static OSStatus
showHideInfoWindow	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	UNUSED_ARGUMENT(inCommandID),
					 void*					inEventContextPtr,
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Commands_ExecutionEventContextPtr	commandInfoPtr = REINTERPRET_CAST(inEventContextPtr, Commands_ExecutionEventContextPtr);
	OSStatus							result = eventNotHandledErr;
	
	
	assert((kCommandShowConnectionStatus == commandInfoPtr->commandID) ||
			(kCommandHideConnectionStatus == commandInfoPtr->commandID));
	InfoWindow_SetVisible(kCommandShowConnectionStatus == commandInfoPtr->commandID);
	return result;
}// showHideInfoWindow

// BELOW IS REQUIRED NEWLINE TO END FILE
