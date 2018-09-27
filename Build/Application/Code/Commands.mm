/*!	\file Commands.mm
	\brief Access to various user commands that tend to
	be available in the menu bar.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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

#import "Commands.h"
#import <UniversalDefines.h>

// standard-C includes
#import <climits>
#import <cstdio>
#import <cstring>
#import <functional>
#import <list>
#import <sstream>
#import <string>

// Unix includes
extern "C"
{
#	import <sys/socket.h>
}

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <Popover.objc++.h>
#import <SoundSystem.h>
#import <Undoables.h>

// application includes
#import "AddressDialog.h"
#import "AppResources.h"
#import "Clipboard.h"
#import "CommandLine.h"
#import "DebugInterface.h"
#import "EventLoop.h"
#import "Folder.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "MacroManager.h"
#import "PrefPanelTranslations.h"
#import "PrefsWindow.h"
#import "PrintTerminal.h"
#import "QuillsEvents.h"
#import "QuillsSession.h"
#import "Session.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "UIStrings.h"
#import "URL.h"
#import "VectorCanvas.h"
#import "VectorInterpreter.h"
#import "VectorWindow.h"



#pragma mark Constants

/*!
Specifies whether the list of available windows is
traversed forwards or backwards when deciding the
next window to activate.
*/
enum My_WindowActivationDirection
{
	kMy_WindowActivationDirectionNext		= 0,
	kMy_WindowActivationDirectionPrevious	= 1,
};

/*!
Specifies zero or more actions to perform when the
active window is changing.
*/
typedef UInt16 My_WindowSwitchingActions;
enum
{
	kMy_WindowSwitchingActionHide	= (1 << 0),
};

#pragma mark Types

/*!
Internal routines.
*/
@interface Commands_Executor (Commands_ExecutorInternal) //{

// class methods
	+ (NSString*)
	selectorNameForValidateActionName:(NSString*)_;
	+ (SEL)
	selectorToValidateAction:(SEL)_;

// new methods
	- (BOOL)
	viaFirstResponderTryToPerformSelector:(SEL)_
	withObject:(id)_;

@end //}


/*!
Calls Commands_ExecuteByIDUsingEvent() after a delay.

This is a transitional object that will not really be needed
once the application moves to Cocoa completely.
*/
@interface Commands_DelayedCommand : NSObject //{
{
@private
	UInt32			commandID;
	EventTargetRef	commandTarget;
	Float32			executionDelay;
}

// initializers
	- (instancetype)
	init DISABLED_SUPERCLASS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initWithCommand:(UInt32)_
	andEventTarget:(EventTargetRef)_
	andDelay:(Float32)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (void)
	execute;

@end //}


/*!
An instance of this class handles the application’s entries in
the Services menu.  Note that the names of messages in this
class should match those published under NSServices (using the
"NSMessage" key) in the generated Info.plist file.
*/
@interface Commands_ServiceProviders : NSObject //{

// registered Service providers
	- (void)
	openPathInShell:(NSPasteboard*)_
	userData:(NSString*)_
	error:(NSString**)_;
	- (void)
	openURL:(NSPasteboard*)_
	userData:(NSString*)_
	error:(NSString**)_;

@end //}


/*!
This class exists so that it is easier to associate a SessionRef
with an NSMenuItem, via setRepresentedObject:.
*/
@interface Commands_SessionWrap : NSObject //{
{
@public
	SessionRef		session;
}

// initializers
	- (instancetype)
	init;
	- (instancetype)
	initWithSession:(SessionRef)_ NS_DESIGNATED_INITIALIZER;

@end //}


namespace {

struct My_MenuItemInsertionInfo
{
	NSMenu*		menu;			//!< the menu in which to create the item
	int			atItemIndex;	//!< the NSMenu index at which the new item should appear
};
typedef My_MenuItemInsertionInfo const*		My_MenuItemInsertionInfoConstPtr;

} // anonymous namespace

#pragma mark Functors
namespace {

/*!
This strictly orders screens so that the main screen is
always in front of any other screen.

Model of STL Binary Predicate.

(4.1)
*/
#pragma mark lessThanIfMainScreen
struct lessThanIfMainScreen:
public std::binary_function< NSScreen*/* argument 1 */, NSScreen*/* argument 2 */, bool/* return */ >
{
	bool
	operator()	(NSScreen*	inScreen1,
				 NSScreen*	inScreen2)
	const
	{
		bool	result = false;
		
		
		// this must be a “strict weak ordering”, otherwise any container
		// that uses it will have incorrect behavior (such as not being
		// able to reach some of its elements properly)
		if (inScreen1 == inScreen2)
		{
			result = false;
		}
		else if ([NSScreen mainScreen] == inScreen1)
		{
			result = true;
		}
		else if ([NSScreen mainScreen] == inScreen2)
		{
			result = false;
		}
		else
		{
			result = (inScreen1 < inScreen2);
		}
		return result;
	}
};

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void				activateAnotherWindow							(My_WindowActivationDirection, My_WindowSwitchingActions = 0);
BOOL				addWindowMenuItemForSession						(SessionRef, NSMenu*, int, CFStringRef);
NSAttributedString*	attributedStringForWindowMenuItemTitle			(NSString*);
void				changeNotifyForCommandExecution					(UInt32);
BOOL				handleQuitReview								();
int					indexOfItemWithAction							(NSMenu*, SEL);
Boolean				isAnyListenerForCommandExecution				(UInt32);
BOOL				isCocoaWindowMoreImportantThanCarbon			(NSWindow*);
BOOL				isWindowVisible									(NSWindow*);
void				preferenceChanged								(ListenerModel_Ref, ListenerModel_Event,
																	 void*, void*);
BOOL				quellAutoNew									();
int					returnFirstWindowItemAnchor						(NSMenu*);
NSMenu*				returnMenu										(UInt32);
SessionRef			returnMenuItemSession							(NSMenuItem*);
SessionRef			returnTEKSession								();
NSMenuItem*			returnWindowMenuItemForSession					(SessionRef);
BOOL				searchResultsExist								();
void				sessionStateChanged								(ListenerModel_Ref, ListenerModel_Event,
																	 void*, void*);
void				sessionWindowStateChanged						(ListenerModel_Ref, ListenerModel_Event,
																	 void*, void*);
void				setItemCheckMark								(id <NSValidatedUserInterfaceItem>, BOOL);
void				setNewCommand									(UInt32);
void				setUpDynamicMenus								();
void				setUpFormatFavoritesMenu						(NSMenu*);
void				setUpMacroSetsMenu								(NSMenu*);
void				setUpSessionFavoritesMenu						(NSMenu*);
void				setUpTranslationTablesMenu						(NSMenu*);
void				setUpWindowMenu									(NSMenu*);
void				setUpWorkspaceFavoritesMenu						(NSMenu*);
void				setWindowMenuItemMarkForSession					(SessionRef, NSMenuItem* = nil);
BOOL				textSelectionExists								();
void				updateFadeAllTerminalWindows					(Boolean);

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_ListenerRef	gPreferenceChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionStateChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionWindowStateChangeEventListener = nullptr;
UInt32						gNewCommandShortcutEffect = kCommandNewSessionDefaultFavorite;
Boolean						gCurrentQuitCancelled = false;
UInt16						gCurrentQuitInitialSessionCount = 0;
UInt16						gCurrentMacroSetIndex = 0;
ListenerModel_Ref&			gCommandExecutionListenerModel	(Boolean	inDispose = false)
{
	static ListenerModel_Ref x = ListenerModel_New(kListenerModel_StyleNonEventNotHandledErr,
													kConstantsRegistry_ListenerModelDescriptorCommandExecution);
	
	
	if (inDispose) ListenerModel_Dispose(&x);
	return x;
}

} // anonymous namespace



#pragma mark Public Methods

/*!
Call this routine once before performing any other
command operations.

(4.0)
*/
void
Commands_Init ()
{
	// set up a callback to receive preference change notifications;
	// this ALSO has the effect of initializing menus, because the
	// callbacks are installed with a request to be invoked immediately
	// with the initial preference value (this has side effects like
	// creating the menu bar, etc.)
	gPreferenceChangeEventListener = ListenerModel_NewStandardListener(preferenceChanged);
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_TagNewCommandShortcutEffect,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts,
							true/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
		prefsResult = Preferences_StartMonitoring
						(gPreferenceChangeEventListener, kPreferences_ChangeContextName,
							false/* notify of initial value */);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	// set up a callback to receive connection state change notifications
	gSessionStateChangeEventListener = ListenerModel_NewStandardListener(sessionStateChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, gSessionStateChangeEventListener);
	gSessionWindowStateChangeEventListener = ListenerModel_NewStandardListener(sessionWindowStateChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowObscured, gSessionWindowStateChangeEventListener);
	SessionFactory_StartMonitoringSessions(kSession_ChangeWindowTitle, gSessionWindowStateChangeEventListener);
	
	// initialize macro selections so that the module will handle menu commands
	{
		MacroManager_Result		macrosResult = MacroManager_SetCurrentMacros(MacroManager_ReturnDefaultMacros());
		
		
		assert(macrosResult.ok());
	}
}// Init


/*!
Call this routine when you are completely done
with commands (at the end of the program).

(4.0)
*/
void
Commands_Done ()
{
	// disable preference change listener
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_TagNewCommandShortcutEffect);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeNumberOfContexts);
	Preferences_StopMonitoring(gPreferenceChangeEventListener, kPreferences_ChangeContextName);
	ListenerModel_ReleaseListener(&gPreferenceChangeEventListener);
	
	// disable session state listeners
	SessionFactory_StopMonitoringSessions(kSession_ChangeState, gSessionStateChangeEventListener);
	ListenerModel_ReleaseListener(&gSessionStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowObscured, gSessionWindowStateChangeEventListener);
	SessionFactory_StopMonitoringSessions(kSession_ChangeWindowTitle, gSessionWindowStateChangeEventListener);
	ListenerModel_ReleaseListener(&gSessionWindowStateChangeEventListener);
}// Done


/*!
Attempts to handle a command at the application level,
given its ID (all IDs are declared in "Commands.h").
Returns true only if the event is handled successfully.

Commands can currently be handled in three ways.  First,
Commands_StartHandlingExecution() might have been
invoked to register a local handler (this approach is
deprecated), and if so, it is invoked exclusively.
Otherwise, if the command ID matches one handled at the
application level, it is handled.  The third method is
to install a Carbon Event handler on a more local context
(such as a window), and this is NOT handled here; see
Commands_ExecuteByIDUsingEvent() for that case.

IMPORTANT:	This works ONLY for command IDs that are
			handled here, at the application level.  Many
			events are handled in more local contexts.
			Commands_ExecuteByIDUsingEvent() should
			generally be used instead, so that a command
			invocation request reaches handlers no matter
			where they are installed.

(3.0)
*/
Boolean
Commands_ExecuteByID	(UInt32		inCommandID)
{
	Boolean		result = true;
	
	
	if (isAnyListenerForCommandExecution(inCommandID))
	{
		// TEMPORARY - the following does not return any indicator
		//             as to whether the command was handled!
		changeNotifyForCommandExecution(inCommandID);
	}
	else
	{
		// no handler for the specified command; check against
		// the following list of known commands
		SessionRef			frontSession = nullptr;
		TerminalWindowRef	frontTerminalWindow = nullptr;
		TerminalScreenRef	activeScreen = nullptr;
		TerminalViewRef		activeView = nullptr;
		Boolean				isSession = false; // initially...
		Boolean				isTerminal = false; // initially...
		
		
		// TEMPORARY: This is a TON of legacy context crap.  Most of this
		// should be refocused or removed.  In addition, it is expensive
		// to do it per-command, it should probably be put into a more
		// global context that is automatically updated whenever the user
		// focus session is changed (and only then).
		frontSession = SessionFactory_ReturnUserRecentSession();
		frontTerminalWindow = (nullptr == frontSession) ? nullptr : Session_ReturnActiveTerminalWindow(frontSession);
		activeScreen = (nullptr == frontTerminalWindow) ? nullptr : TerminalWindow_ReturnScreenWithFocus(frontTerminalWindow);
		activeView = (nullptr == frontTerminalWindow) ? nullptr : TerminalWindow_ReturnViewWithFocus(frontTerminalWindow);
		isSession = (nullptr != frontSession);
		isTerminal = ((nullptr != activeScreen) && (nullptr != activeView));
		
		switch (inCommandID)
		{
		case kCommandAboutThisApplication:
			CocoaBasic_AboutPanelDisplay();
			break;
		
		//case kCommandCreditsAndLicenseInfo:
		//	see AboutBox.cp
		//  break;
		
		case kCommandCheckForUpdates:
			// NOTE: There used to be an elaborate HTTP download-and-scan section
			// here, to consult the MacPAD file, determine whether the version is
			// up-to-date, potentially display release notes and a download link,
			// etc.  It was determined that this was not desirable: for one thing,
			// the default implementation was modal and could hang significantly
			// if the Internet connection was down; also, it could take longer than
			// simply downloading a web page, and it was still somewhat unclear
			// whether it would work long-term.  Opening a web page is simple, is
			// pretty fast, is something the user can bookmark in other ways and
			// is easy to maintain, so that is the final solution.
			if (false == URL_OpenInternetLocation(kURL_InternetLocationApplicationUpdatesPage))
			{
				Sound_StandardAlert();
			}
			break;
		
		case kCommandURLHomePage:
			if (false == URL_OpenInternetLocation(kURL_InternetLocationApplicationHomePage))
			{
				Sound_StandardAlert();
			}
			break;
		
		case kCommandURLAuthorMail:
			{
				CFStringRef				appVersionCFString = CFUtilities_StringCast
																(CFBundleGetValueForInfoDictionaryKey
																	(AppResources_ReturnBundleForInfo(),
																		CFSTR("CFBundleVersion")));
				NSMutableDictionary*	appEnvDict = [[[NSMutableDictionary alloc] initWithCapacity:1/* arbitrary */]
														autorelease];
				NSRunningApplication*	runHandle = nil;
				CFErrorRef				errorRef = nullptr;
				
				
				// MUST correspond to environment variables expected
				// by the Bug Reporter internal application (also,
				// every value must be an NSString* type)
				[appEnvDict setValue:@"1" forKey:@"BUG_REPORTER_MACTERM_COMMENT_EMAIL_ONLY"];
				if (nullptr != appVersionCFString)
				{
					[appEnvDict setValue:BRIDGE_CAST(appVersionCFString, NSString*) forKey:@"BUG_REPORTER_MACTERM_VERSION"];
				}
				runHandle = AppResources_LaunchBugReporter(BRIDGE_CAST(appEnvDict, CFDictionaryRef), &errorRef);
				if (nullptr != errorRef)
				{
					[NSApp presentError:BRIDGE_CAST(errorRef, NSError*)];
				}
				else
				{
					// success (E-mail application should have opened)
				}
			}
			break;
		
		//case kCommandNewSessionLoginShell:
		//case kCommandNewSessionShell:
		//case kCommandNewSessionDialog:
		//case kCommandNewSessionDefaultFavorite:
		//	see SessionFactory.cp
		//	break;
		
		case kCommandOpenSession:
			//SessionDescription_Load();
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "UNIMPLEMENTED; need to rewrite '.session' parser in Python");
			break;
		
		case kCommandCloseConnection:
			// automatically handled by the OS
			break;
		
		case kCommandSaveSession:
			if (isSession) Session_DisplaySaveDialog(frontSession);
			break;
		
		case kCommandSaveSelection:
			TerminalView_DisplaySaveSelectionUI(activeView);
			break;
		
		case kCommandNewDuplicateSession:
			{
				SessionRef		newSession = nullptr;
				
				
				newSession = SessionFactory_NewCloneSession(nullptr/* terminal window */, frontSession);
				
				// report any errors to the user
				if (nullptr == newSession)
				{
					// UNIMPLEMENTED!!!
					Sound_StandardAlert();
				}
			}
			break;
		
		case kCommandKillProcessesKeepWindow:
			{
				Boolean		allowForceQuit = true;
				
				
				// in Full Screen mode, this command might not always be allowed
				if (kPreferences_ResultOK !=
					Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
										&allowForceQuit))
				{
					allowForceQuit = true; // assume a value if the preference cannot be found
				}
				
				if ((false == TerminalWindow_IsFullScreen(frontTerminalWindow)) || (allowForceQuit))
				{
					Session_DisplayTerminationWarning(frontSession, kSession_TerminationDialogOptionKeepWindow);
				}
				else
				{
					Sound_StandardAlert();
				}
			}
			break;
		
		case kCommandRestartSession:
			{
				Boolean		allowForceQuit = true;
				
				
				// in Full Screen mode, this command might not always be allowed
				if (kPreferences_ResultOK !=
					Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
										&allowForceQuit))
				{
					allowForceQuit = true; // assume a value if the preference cannot be found
				}
				
				if ((false == TerminalWindow_IsFullScreen(frontTerminalWindow)) || (allowForceQuit))
				{
					Session_DisplayTerminationWarning(frontSession, kSession_TerminationDialogOptionKeepWindow |
																	kSession_TerminationDialogOptionRestart);
				}
				else
				{
					Sound_StandardAlert();
				}
			}
			break;
		
		case kCommandHandleURL:
			// open the appropriate helper application for the URL in the selected
			// text (which may be MacTerm itself), and send a “handle URL” event
			if (isSession) URL_HandleForScreenView(activeScreen, activeView);
			else Sound_StandardAlert();
			break;
		
		case kCommandPrint:
		case kCommandPrintScreen:
			// print the selection or the screen using the print dialog
			{
				Boolean				printScreen = ((kCommandPrintScreen == inCommandID) ||
													(false == TerminalView_TextSelectionExists(activeView)));
				CFRetainRelease		jobTitle(((printScreen)
												? UIStrings_ReturnCopy(kUIStrings_TerminalPrintScreenJobTitle)
												: UIStrings_ReturnCopy(kUIStrings_TerminalPrintSelectionJobTitle)),
												CFRetainRelease::kAlreadyRetained);
				
				
				if (jobTitle.exists())
				{
					PrintTerminal_JobRef	printJob = (printScreen)
														? PrintTerminal_NewJobFromVisibleScreen
															(activeView, activeScreen, jobTitle.returnCFStringRef())
														: PrintTerminal_NewJobFromSelectedText
															(activeView, jobTitle.returnCFStringRef());
					
					
					if (nullptr != printJob)
					{
						UNUSED_RETURN(PrintTerminal_Result)PrintTerminal_JobSendToPrinter
															(printJob, nil/* window; INCOMPLETE */);
						PrintTerminal_ReleaseJob(&printJob);
					}
				}
			}
			break;
		
		case kCommandUndo:
			{
				NSUndoManager*		undoManager = [[NSApp keyWindow] firstResponder].undoManager;
				
				
				// NOTE: Carbon windows in Cocoa appear to provide their own
				// NSUndoManager instance, which is useless; detect this and
				// ensure NSUndoManager is only used for pure Cocoa windows
				if ((nil != undoManager) && isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
				{
					[undoManager undo];
				}
				else
				{
					// legacy
					Undoables_UndoLastAction();
				}
			}
			break;
		
		case kCommandRedo:
			{
				NSUndoManager*		undoManager = [[NSApp keyWindow] firstResponder].undoManager;
				
				
				// NOTE: Carbon windows in Cocoa appear to provide their own
				// NSUndoManager instance, which is useless; detect this and
				// ensure NSUndoManager is only used for pure Cocoa windows
				if ((nil != undoManager) && isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
				{
					[undoManager redo];
				}
				else
				{
					// legacy
					Undoables_RedoLastUndo();
				}
			}
			break;
		
		case kCommandCut:
		case kCommandCopy:
		case kCommandCopyTable:
			if ((inCommandID == kCommandCut) ||
				(inCommandID == kCommandCopy) ||
				(inCommandID == kCommandCopyTable))
			{
				// all of these operations involving copying to the clipboard
				if (nullptr != activeView)
				{
					CFRetainRelease		selectedNSImageArray(TerminalView_ReturnSelectedImageArrayCopy(activeView),
																CFRetainRelease::kAlreadyRetained); 
					NSArray*			asArray = BRIDGE_CAST(selectedNSImageArray.returnCFArrayRef(), NSArray*);
					
					
					if (selectedNSImageArray.exists() && (asArray.count > 0))
					{
						Boolean		haveCleared = false;
						
						
						for (NSImage* asImage in asArray)
						{
							assert([asImage isKindOfClass:NSImage.class]);
							OSStatus	copyStatus = noErr;
							
							
							copyStatus = Clipboard_AddNSImageToPasteboard(asImage, nullptr/* target pasteboard */,
																			(false == haveCleared)/* clear flag */);
							if (noErr != copyStatus)
							{
								Console_Warning(Console_WriteValue, "failed to Copy image, error", copyStatus);
							}
							else
							{
								// if more than one image is selected, add them all
								haveCleared = true;
							}
						}
					}
					else
					{
						Clipboard_TextToScrap(activeView, (kCommandCopyTable == inCommandID)
															? kClipboard_CopyMethodTable
															: kClipboard_CopyMethodStandard);
					}
				}
			}
			break;
		
		case kCommandCopyAndPaste:
			if (isSession)
			{
				// type if there is a window to type into
				CFArrayRef		clipboardStringArray = nullptr;
				
				
				Clipboard_TextToScrap(activeView, kClipboard_CopyMethodStandard | kClipboard_CopyMethodInline);
				
				if (Clipboard_CreateCFStringArrayFromPasteboard(clipboardStringArray))
				{
					NSArray*		asNSArray = BRIDGE_CAST(clipboardStringArray, NSArray*);
					NSUInteger		lineCount = 0;
					
					
					// NOTE: due to “inline” Copy above, there should not be more than
					// one line retrieved from the Clipboard but this is an iteration
					// for completeness
					for (id anObject in asNSArray)
					{
						if (NO == [anObject isKindOfClass:NSString.class])
						{
							Sound_StandardAlert();
							Console_Warning(Console_WriteLine, "unexpected non-string value read from clipboard");
							break;
						}
						else
						{
							NSString*	asString = STATIC_CAST(anObject, NSString*);
							
							
							Session_UserInputCFString(frontSession, BRIDGE_CAST(asString, CFStringRef));
							++lineCount;
							if (lineCount < asNSArray.count)
							{
								Session_SendNewline(frontSession, kSession_EchoCurrentSessionValue);
							}
						}
					}
					CFRelease(clipboardStringArray); clipboardStringArray = nullptr;
				}
			}
			break;
		
		case kCommandPaste:
			if (isSession)
			{
				Session_Result		sessionResult = Session_UserInputPaste(frontSession); // paste if there is a window to paste into
				
				
				if (false == sessionResult.ok())
				{
					Console_Warning(Console_WriteValue, "failed to paste, error", sessionResult.code());
				}
			}
			else
			{
				// this should not happen with Cocoa if the responder chain is
				// able to forward Paste actions to the active element
				Console_Warning(Console_WriteLine, "failed to paste (not a session window)");
			}
			break;
		
		//case kCommandFind:
		//case kCommandFindAgain:
		//case kCommandFindPrevious:
		//	see TerminalWindow.mm
		//	break;
		
		//case kCommandFindCursor:
		//	see TerminalWindow.mm
		//	break;
		
		case kCommandSelectAll: // text only
		case kCommandSelectAllWithScrollback: // text only
		case kCommandSelectNothing: // text only
			if (isSession)
			{
				if (inCommandID == kCommandSelectAllWithScrollback)
				{
					// select entire scrollback buffer
					TerminalView_SelectEntireBuffer(activeView);
				}
				else if (inCommandID == kCommandSelectNothing)
				{
					// remove selection
					TerminalView_SelectNothing(activeView);
				}
				else
				{
					// select only the bottomost windowful of text
					TerminalView_SelectMainScreen(activeView);
				}
			}
			break;
		
		case kCommandShowClipboard:
			Clipboard_SetWindowVisible(true);
			break;
		
		case kCommandHideClipboard:
			Clipboard_SetWindowVisible(false);
			break;
		
		case kHICommandPreferences:
			[[[PrefsWindow_Controller sharedPrefsWindowController] window] makeKeyAndOrderFront:nil];
			break;
		
		//case kCommandWiderScreen:
		//case kCommandNarrowerScreen:
		//case kCommandTallerScreen:
		//case kCommandShorterScreen:
		//case kCommandLargeScreen:
		//case kCommandSmallScreen:
		//case kCommandTallScreen:
		//	see TerminalWindow.mm
		//	break;
		
		//case kCommandSetScreenSize:
		//	see TerminalWindow.mm
		//	break;
		
		//case kCommandBiggerText:
		//case kCommandSmallerText:
		//	see TerminalWindow.mm
		//	break;
		
		//case kCommandFormatDefault:
		//case kCommandFormatByFavoriteName:
		//case kCommandFormat:
		//	see TerminalWindow.mm
		//	break;
		
		//case kCommandFullScreenToggle:
		//case kCommandZoomMaximumSize:
		//	see TerminalWindow.mm
		//	break;
		
		//case kHICommandShowToolbar:
		//case kHICommandHideToolbar:
		//case kHICommandCustomizeToolbar:
		//	see EventLoop.mm
		//	break;
		
		//case kCommandTerminalDefault:
		//case kCommandTerminalByFavoriteName:
		//	see TerminalWindow.mm
		//	break;
		
		case kCommandBellEnabled:
			if (isTerminal)
			{
				Terminal_SetBellEnabled(activeScreen, !Terminal_BellIsEnabled(activeScreen));
			}
			break;
		
		case kCommandEcho:
			if (isSession)
			{
				Session_SetLocalEchoEnabled(frontSession, !Session_LocalEchoIsEnabled(frontSession));
			}
			break;
		
		case kCommandWrapMode:
			if (isTerminal)
			{
				Terminal_SetLineWrapEnabled(activeScreen, !Terminal_LineWrapIsEnabled(activeScreen));
			}
			break;
		
		case kCommandClearScreenSavesLines:
			if (isTerminal)
			{
				Terminal_SetSaveLinesOnClear(activeScreen, !Terminal_SaveLinesOnClearIsEnabled(activeScreen));
			}
			break;
		
		case kCommandJumpScrolling:
			if (isSession)
			{
				Session_FlushNetwork(frontSession);
			}
			break;
		
		case kCommandCaptureToFile:
			if (isSession)
			{
				Terminal_FileCaptureEnd(activeScreen);
				Session_DisplayFileCaptureSaveDialog(frontSession);
			}
			break;
		
		case kCommandEndCaptureToFile:
			if (isSession)
			{
				Terminal_FileCaptureEnd(activeScreen);
			}
			break;
		
		case kCommandTEKPageCommand:
			{
				SessionRef		sessionForGraphic = frontSession;
				
				
				// allow this command for either session terminal windows, or
				// the graphics themselves (as long as the graphic can be
				// traced to a session)
				if (nullptr == sessionForGraphic)
				{
					sessionForGraphic = returnTEKSession();
				}
				
				if (nullptr != sessionForGraphic)
				{
					// open a new window or clear the buffer of the current one
					Session_TEKNewPage(sessionForGraphic);
				}
			}
			break;
		
		case kCommandTEKPageClearsScreen:
			{
				SessionRef		sessionForGraphic = frontSession;
				
				
				// allow this command for either session terminal windows, or
				// the graphics themselves (as long as the graphic can be
				// traced to a session)
				if (nullptr == sessionForGraphic)
				{
					sessionForGraphic = returnTEKSession();
				}
				
				if (nullptr != sessionForGraphic)
				{
					// toggle this setting
					Session_TEKSetPageCommandOpensNewWindow
					(sessionForGraphic, false == Session_TEKPageCommandOpensNewWindow(sessionForGraphic));
				}
			}
			break;
		
		case kCommandSpeechEnabled:
			if (isSession)
			{
				if (Session_SpeechIsEnabled(frontSession))
				{
					Session_SpeechPause(frontSession);
					Session_SetSpeechEnabled(frontSession, false);
				}
				else
				{
					Session_SetSpeechEnabled(frontSession, true);
					Session_SpeechResume(frontSession);
				}
			}
			break;
		
		case kCommandSpeakSelectedText:
			if (isTerminal)
			{
				TerminalView_GetSelectedTextAsAudio(activeView);
			}
			break;
		
		case kCommandStopSpeaking:
			CocoaBasic_StopSpeaking();
			break;
		
		case kCommandClearEntireScrollback:
			if (isTerminal)
			{
				TerminalView_DeleteScrollback(activeView);
			}
			break;
		
		case kCommandResetTerminal:
			if (isTerminal)
			{
				Terminal_Reset(activeScreen);
			}
			break;
		
		case kCommandDeletePressSendsBackspace:
		case kCommandDeletePressSendsDelete:
			if (isSession)
			{
				Session_EventKeys		keyMappings = Session_ReturnEventKeys(frontSession);
				
				
				keyMappings.deleteSendsBackspace = (inCommandID == kCommandDeletePressSendsBackspace);
				Session_SetEventKeys(frontSession, keyMappings);
			}
			break;
		
		case kCommandEmacsArrowMapping:
			if (isSession)
			{
				Session_EventKeys		keyMappings = Session_ReturnEventKeys(frontSession);
				
				
				keyMappings.arrowsRemappedForEmacs = !(keyMappings.arrowsRemappedForEmacs);
				Session_SetEventKeys(frontSession, keyMappings);
			}
			break;
		
		case kCommandLocalPageUpDown:
			if (isSession)
			{
				Session_EventKeys		keyMappings = Session_ReturnEventKeys(frontSession);
				
				
				keyMappings.pageKeysLocalControl = !(keyMappings.pageKeysLocalControl);
				Session_SetEventKeys(frontSession, keyMappings);
			}
			break;
		
		case kCommandSetKeys:
			if (isSession)
			{
				Session_DisplaySpecialKeySequencesDialog(frontSession);
			}
			break;
		
		//case kCommandTranslationTableDefault:
		//case kCommandTranslationTableByFavoriteName:
		//case kCommandSetTranslationTable:
		//	see TerminalWindow.mm
		//	break;
		
		case kCommandShowNetworkNumbers:
			// in the Cocoa implementation this really means “show or activate”
			AddressDialog_Display();
			break;

		case kCommandSendInterruptProcess:
			Session_UserInputInterruptProcess(frontSession);
			break;
		
		case kCommandWatchNothing:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchNothing);
			}
			break;
		
		case kCommandWatchForActivity:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchForPassiveData);
			}
			break;
		
		case kCommandWatchForInactivity:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchForInactivity);
			}
			break;
		
		case kCommandTransmitOnInactivity:
			if (isSession)
			{
				Session_SetWatch(frontSession, kSession_WatchForKeepAlive);
			}
			break;
			
		case kCommandSuspendNetwork:
			if (nullptr != frontSession)
			{
				Session_SetNetworkSuspended(frontSession, !Session_NetworkIsSuspended(frontSession));
			}
			break;
		
		case kCommandChangeWindowTitle:
			// let the user change the title of certain windows
			{
				if (nullptr != frontSession)
				{
					Session_DisplayWindowRenameUI(frontSession);
				}
				else
				{
					// ???
					Sound_StandardAlert();
				}
			}
			break;
		
		//case kCommandHideFrontWindow:
		//	see TerminalWindow.mm
		//	break;
		
		//case kCommandHideOtherWindows:
		//	see TerminalWindow.mm
		//	break;
		
		case kCommandShowAllHiddenWindows:
			// show all windows
			SessionFactory_ForEachTerminalWindow
			(^(TerminalWindowRef	inTerminalWindow,
			   Boolean&				UNUSED_ARGUMENT(outStopFlag))
			{
				TerminalWindow_SetObscured(inTerminalWindow, false);
			});
			break;
		
		case kCommandStackWindows:
			{
				// on Mac OS X, this command also requires that all application windows come to the front
				NSRunningApplication*	runningApplication = [NSRunningApplication currentApplication];
				
				
				UNUSED_RETURN(BOOL)[runningApplication activateWithOptions:(NSApplicationActivateAllWindows)];
				
				// arrange windows in a diagonal pattern
				TerminalWindow_StackWindows();
			}
			break;
		
		case kCommandNextWindow:
			// activate the next window in the window list (terminal windows only)
			activateAnotherWindow(kMy_WindowActivationDirectionNext);
			break;
		
		case kCommandNextWindowHideCurrent:
			// activate the next window in the window list (terminal windows only),
			// but first obscure the frontmost terminal window
			activateAnotherWindow(kMy_WindowActivationDirectionNext, (kMy_WindowSwitchingActionHide));
			break;
		
		case kCommandPreviousWindow:
			// activate the previous window in the window list (terminal windows only)
			activateAnotherWindow(kMy_WindowActivationDirectionPrevious);
			break;
		
		case kCommandPreviousWindowHideCurrent:
			// activate the previous window in the window list (terminal windows only),
			// but first obscure the frontmost terminal window
			activateAnotherWindow(kMy_WindowActivationDirectionPrevious, (kMy_WindowSwitchingActionHide));
			break;
		
		//case kCommandShowConnectionStatus:
		//case kCommandHideConnectionStatus:
		//	see InfoWindow.cp
		//	break;
		
		case kCommandShowCommandLine:
			// in the Cocoa implementation this really means “show or activate”
			CommandLine_Display();
			break;
		
		//kCommandDisplayPrefPanelFormats:
		//kCommandDisplayPrefPanelGeneral:
		//kCommandDisplayPrefPanelMacros:
		//kCommandDisplayPrefPanelSessions:
		//kCommandDisplayPrefPanelTerminals:
		//	see PrefsWindow.mm
		//	break;
		
		case kCommandShowControlKeys:
			// in the Cocoa implementation this really means “show or activate”
			Keypads_SetVisible(kKeypads_WindowTypeControlKeys, true);
			break;
		
		case kCommandShowFunction:
			// in the Cocoa implementation this really means “show or activate”
			Keypads_SetVisible(kKeypads_WindowTypeFunctionKeys, true);
			break;
		
		case kCommandShowKeypad:
			// in the Cocoa implementation this really means “show or activate”
			Keypads_SetVisible(kKeypads_WindowTypeVT220Keys, true);
			break;
		
		case kCommandDebuggingOptions:
			DebugInterface_Display();
			break;
		
		case kCommandMainHelp:
			HelpSystem_DisplayHelpWithoutContext();
			break;
		
		case kCommandContextSensitiveHelp:
			// open the help system to a particular topic
			HelpSystem_DisplayHelpInCurrentContext();
			break;
		
	#if 0
		case kCommandToggleMacrosMenuVisibility:
			{
				Preferences_Result		preferencesResult = kPreferences_ResultOK;
				Boolean					isVisible = false;
				
				
				preferencesResult = Preferences_GetData(kPreferences_TagMacrosMenuVisible,
														sizeof(isVisible), &isVisible);
				unless (preferencesResult == kPreferences_ResultOK)
				{
					isVisible = false; // assume a value, if preference can’t be found
				}
				
				isVisible = !isVisible;
				
				preferencesResult = Preferences_SetData(kPreferences_TagMacrosMenuVisible,
														sizeof(isVisible), &isVisible);
			}
			break;
	#endif
		
		case kCommandTerminalViewPageUp:
			TerminalView_ScrollRowsTowardBottomEdge(activeView, Terminal_ReturnRowCount(activeScreen));
			break;
		
		case kCommandTerminalViewPageDown:
			TerminalView_ScrollRowsTowardTopEdge(activeView, Terminal_ReturnRowCount(activeScreen));
			break;
		
		case kCommandTerminalViewHome:
			TerminalView_ScrollToBeginning(activeView);
			break;
		
		case kCommandTerminalViewEnd:
			TerminalView_ScrollToEnd(activeView);
			break;
		
		default:
			result = false;
			break;
		}
	}
	return result;
}// ExecuteByID


/*!
Like Commands_ExecuteByID(), except it constructs a
Carbon Event and sends it to the specified target.  If
the target is "nullptr", the first target attempted is
the user focus; barring that, the active window; and
barring that, the application itself.

Returns true only if the event is handled successfully.

Unlike Commands_ExecuteByID(), this request should
propagate to an appropriate handler no matter where
the handler is installed.

IMPORTANT:	If Commands_StartHandlingExecution() was
			used to register a handler, this function
			will not respect it.  That method of
			handling commands is deprecated, but
			Commands_ExecuteByID() will still invoke
			such handlers.

(3.1)
*/
Boolean
Commands_ExecuteByIDUsingEvent	(UInt32				inCommandID,
								 EventTargetRef		inTarget)
{
	Boolean		result = false;
	
	
	// conceptually, this should probably be replaced by calls
	// to NSResponder’s "tryToPerform:with:" or a similar method
	// to go up the chain; no longer implemented, Carbon legacy;
	// this will be a big source of problems at first but it is
	// easier to just look for errors and implement appropriate
	// actions at the right points in the responder chain
	Console_Warning(Console_WriteValueFourChars, "Commands_ExecuteByIDUsingEvent() is no longer implemented; failed to process request, command ID", inCommandID);
	result = Commands_ExecuteByID(inCommandID); // approximation
	
	return result;
}// ExecuteByIDUsingEvent


/*!
Adds to the specified menu a list of the names of valid
contexts in the specified preferences class.

If there is a problem adding anything to the menu, a
non-success code may be returned, although the menu may
still be partially changed.

\retval kCommands_ResultOK
if all context names were found and added successfully

\retval kCommands_ResultParameterError
if "inClass" is not valid

(4.0)
*/
Commands_Result
Commands_InsertPrefNamesIntoMenu	(Quills::Prefs::Class	inClass,
									 NSMenu*				inoutMenu,
									 int					inAtItemIndex,
									 int					inInitialIndent,
									 SEL					inAction)
{
	Commands_Result		result = kCommands_ResultOK;
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	CFArrayRef			nameCFStringCFArray = nullptr;
	
	
	prefsResult = Preferences_CreateContextNameArray(inClass, nameCFStringCFArray);
	if (kPreferences_ResultOK != prefsResult)
	{
		result = kCommands_ResultParameterError;
	}
	else
	{
		// now re-populate the menu using resource information
		int const		kMenuItemCount = CFArrayGetCount(nameCFStringCFArray);
		int				i = 0;
		int				totalItems = 0;
		CFStringRef		nameCFString = nullptr;
		
		
		for (i = 0; i < kMenuItemCount; ++i)
		{
			nameCFString = CFUtilities_StringCast(CFArrayGetValueAtIndex(nameCFStringCFArray, i));
			if (nullptr != nameCFString)
			{
				NSMenuItem*		newItem = nil;
				
				
				newItem = [inoutMenu insertItemWithTitle:(NSString*)nameCFString
															action:inAction keyEquivalent:@""
															atIndex:(inAtItemIndex + totalItems)];
				if (nil != newItem)
				{
					++totalItems;
					[newItem setIndentationLevel:inInitialIndent];
				}
			}
		}
		
		CFRelease(nameCFStringCFArray), nameCFStringCFArray = nullptr;
	}
	
	return result;
}// InsertPrefNamesIntoMenu


/*!
Allocates and initializes a new NSMenuItem instance that
will handle the specified command correctly.  (You must
"release" it yourself.)

If "inMustBeEnabled" is true, the result is nil whenever
the command is not available to the user.  (This is useful
for omitting items from contextual menus.)

WARNING:	This currently only works for command IDs that
		are used for contextual menu items.

DEPRECATED.  This is here for transitional purposes, as it
is useful in some cases (particularly contextual menus) to
refer to items by ID.  However, menu items from now on will
be primarily checked using Objective-C and Cocoa
selectors.

(4.1)
*/
NSMenuItem*
Commands_NewMenuItemForCommand	(UInt32			inCommandID,
								 CFStringRef	inPreferredTitle,
								 Boolean		inMustBeEnabled)
{
	Commands_Executor*		commandExecutor = (Commands_Executor*)[Commands_Executor sharedExecutor];
	NSMenuItem*				result = nil;
	
	
	assert(nil != commandExecutor);
	
	result = [commandExecutor newMenuItemForCommand:inCommandID itemTitle:BRIDGE_CAST(inPreferredTitle, NSString*)
													ifEnabled:inMustBeEnabled];
	
	return result;
}// NewMenuItemForCommand


/*!
Arranges for a callback to be invoked in order to
execute the given command.

The specified callback is expected to return an
OSStatus value - a return value of "eventNotHandledErr"
is the ONLY way to indicate that an execution handler
did nothing in response to a particular command, which
permits the Commands module to seek another handler
for the same command.  Returning any other value will
cause your handler to be the last attempt made to
execute the given command.

The context passed to the listener is of type
"Commands_ExecutionEventContextPtr".

\retval kCommands_ResultOK
if no error occurred

\retval kCommands_ResultParameterError
if an OSStatus listener was not provided or otherwise
the listener could not be registered

(3.0)
*/
Commands_Result
Commands_StartHandlingExecution		(UInt32						inImplementedCommand,
									 ListenerModel_ListenerRef	inCommandImplementor)
{
	Commands_Result		result = kCommands_ResultOK;
	OSStatus			error = noErr;
	
	
	error = ListenerModel_AddListenerForEvent(gCommandExecutionListenerModel(), inImplementedCommand, inCommandImplementor);
	if (error != noErr) result = kCommands_ResultParameterError;
	
	return result;
}// StartHandlingExecution


/*!
Arranges for a callback to no longer be invoked when
the specified command needs to be executed.

The given listener should match one installed
previously with Commands_StartHandlingExecution();
otherwise this routine silently fails.

\retval kCommands_ResultOK
always

(3.0)
*/
Commands_Result
Commands_StopHandlingExecution	(UInt32						inImplementedCommand,
								 ListenerModel_ListenerRef	inCommandImplementor)
{
	Commands_Result		result = kCommands_ResultOK;
	
	
	ListenerModel_RemoveListenerForEvent(gCommandExecutionListenerModel(), inImplementedCommand, inCommandImplementor);
	return result;
}// StopHandlingExecution


#pragma mark Internal Methods
namespace {

/*!
Chooses another window in the window list that is adjacent to the
frontmost window.

If "inActivationDirection" is kMy_WindowActivationDirectionPrevious
then the previous window is activated instead of the next one.

If "inSwitchingActions" contains kMy_WindowSwitchingActionHide and
the frontmost window is a terminal, that window is hidden (with the
appropriate visual effects) before the next window is activated.

(2018.02)
*/
void
activateAnotherWindow	(My_WindowActivationDirection	inActivationDirection,
						 My_WindowSwitchingActions		inSwitchingActions)
{
	typedef std::list< NSWindow* >										My_WindowList;
	typedef std::map< NSScreen*, My_WindowList, lessThanIfMainScreen >	My_WindowsByScreen; // which windows are on which devices?
	
	Boolean const			kIsPrevious = (kMy_WindowActivationDirectionPrevious == inActivationDirection);
	Boolean const			kHidePrevious = (kMy_WindowSwitchingActionHide == (inSwitchingActions & kMy_WindowSwitchingActionHide));
	My_WindowsByScreen		windowsByScreen; // main screen should be visited first
	NSArray*				allWindows = [NSApp windows];
	NSWindow*				currentWindow = nil;
	NSWindow*				nextWindow = nil;
	NSWindow*				firstValidWindow = nil;
	TerminalWindowRef		autoHiddenTerminalWindow = nullptr;
	BOOL					setNext = NO;
	BOOL					doneSearch = NO;
	
	
	if (kHidePrevious)
	{
		autoHiddenTerminalWindow = [[NSApp mainWindow] terminalWindowRef];
		if (false == TerminalWindow_IsValid(autoHiddenTerminalWindow))
		{
			autoHiddenTerminalWindow = nullptr;
		}
	}
	
	// first determine which windows are on each screen so that
	// all windows on one screen are visited before switching
	// (TEMPORARY; in theory this could respond to notifications
	// as windows are created/destroyed or as windows change
	// screens, and otherwise not bother to recalculate the same
	// window map each time)
	for (currentWindow in allWindows)
	{
		NSScreen*		windowScreen = [currentWindow screen];
		My_WindowList&	windowsOnThisScreen = windowsByScreen[windowScreen];
		
		
		if (kIsPrevious)
		{
			windowsOnThisScreen.push_front(currentWindow);
		}
		else
		{
			windowsOnThisScreen.push_back(currentWindow);
		}
	}
	
	// now determine the next or previous window to visit
	// (some of the work is done opportunistically, other
	// work is only done when absolutely required)
	auto	toPair = windowsByScreen.begin();
	auto	endPairs = windowsByScreen.end();
	for (; ((NO == doneSearch) && (toPair != endPairs)); ++toPair)
	{
		auto	toWindow = toPair->second.begin();
		auto	endWindows = toPair->second.end();
		for (; ((NO == doneSearch) && (toWindow != endWindows)); ++toWindow)
		{
			// certain windows are skipped; "currentWindow" is only
			// set when the current window is determined to be valid
			currentWindow = nil;
			
			if (EventLoop_IsMainWindowFullScreen())
			{
				// if Full Screen is active, only rotate between the
				// full-screen terminal windows
				if ([*toWindow isOnActiveSpace] && (nullptr != [*toWindow terminalWindowRef]) &&
					TerminalWindow_IsFullScreen([*toWindow terminalWindowRef]))
				{
					currentWindow = *toWindow;
				}
			}
			else
			{
				// if not Full Screen, rotate normally; invisible
				// windows may only be chosen if they are terminals
				// (hidden terminals are explicitly revealed during
				// keyboard rotation); also, floating windows are
				// explicitly ignored because they all have menu
				// key equivalents that would give them focus
				if ([*toWindow isOnActiveSpace] && [*toWindow canBecomeKeyWindow] && isWindowVisible(*toWindow))
				{
					currentWindow = *toWindow;
				}
			}
			
			if (nil != currentWindow)
			{
				// only rotate between windows on the active Space
				if ([NSApp keyWindow] == currentWindow)
				{
					setNext = YES;
				}
				else
				{
					if (nil == firstValidWindow)
					{
						firstValidWindow = currentWindow;
					}
					
					if (setNext)
					{
						// no need to check any other windows
						nextWindow = currentWindow;
						doneSearch = YES;
					}
				}
			}
		}
	}
	
	if (nil == nextWindow)
	{
		// the front window must have been last in the list;
		// rotate to the beginning of the window list
		nextWindow = firstValidWindow;
	}
	
	// now that another window has been chosen, activate it
	// and see if any other actions need to be taken
	if (nil == nextWindow)
	{
		// cannot figure out which window to activate
		Sound_StandardAlert();
	}
	else
	{
		[nextWindow makeKeyAndOrderFront:nil];
		
		if (nil != autoHiddenTerminalWindow)
		{
			TerminalWindow_SetObscured(autoHiddenTerminalWindow, true);
		}
	}
}// activateAnotherWindow


/*!
Adds a session’s name to the Window menu, arranging so
that future selections of the new menu item will cause
the window to be selected, and so that the item’s state
is synchronized with that of the session and its window.

Returns true only if an item was added.

(4.0)
*/
BOOL
addWindowMenuItemForSession		(SessionRef			inSession,
								 NSMenu*			inMenu,
								 int				inItemIndex,
								 CFStringRef		inWindowName)
{
	BOOL			result = NO;
	NSMenuItem*		newItem = [inMenu insertItemWithTitle:BRIDGE_CAST(inWindowName, NSString*)
															action:nil keyEquivalent:@""
															atIndex:inItemIndex];
	
	
	if (nil != newItem)
	{
		// define an attributed title so that it is possible to italicize the text for hidden windows
		NSAttributedString*		titleString = attributedStringForWindowMenuItemTitle(newItem.title);
		
		
		[newItem setAttributedTitle:titleString];
		
		// set icon appropriately for the state
		setWindowMenuItemMarkForSession(inSession, newItem);
		
		// set action and implied canXYZ: method to set state
		[newItem setAction:@selector(orderFrontSpecificWindow:)];
		
		// attach the given session as a property of the new item
		[newItem setRepresentedObject:[[[Commands_SessionWrap alloc] initWithSession:inSession] autorelease]];
		assert(nil != [newItem representedObject]);
		
		// all done adding this item!
		result = YES;
	}
	return result;
}// addWindowMenuItemForSession


/*!
Returns an autoreleased attributed string suitable for use as
the attributed title of a session item in the Window menu.

This is used to initialize new items and to change their titles
later (as the system will only recognize changes if they remain
attributed strings).

(4.1)
*/
NSAttributedString*
attributedStringForWindowMenuItemTitle		(NSString*		inTitleText)
{
	NSMutableAttributedString*		result = [[[NSMutableAttributedString alloc] initWithString:inTitleText] autorelease];
	
	
	// unfortunately, attributed strings have no properties whatsoever, so even normal text
	// requires explicitly setting the proper system font for menu items!
	[result addAttribute:NSFontAttributeName value:[NSFont menuFontOfSize:[NSFont systemFontSize]]
												range:NSMakeRange(0, result.length)];
	
	return result;
}// attributedStringForWindowMenuItemTitle


/*!
Notifies listeners that the specified command needs
executing, until a listener returns a result other
than "eventNotHandledErr".  The listeners of this
event are actually expected to perform an operation
in response to the command.

(3.0)
*/
void
changeNotifyForCommandExecution		(UInt32		inCommand)
{
	Commands_ExecutionEventContext		context;
	
	
	context.commandID = inCommand;
	
	// invoke listener callback routines appropriately, from the specified command’s implementation listener model
	ListenerModel_NotifyListenersOfEvent(gCommandExecutionListenerModel(), inCommand, &context);
}// changeNotifyForCommandExecution


/*!
Handles a quit by reviewing any open sessions.  Returns
YES only if the application should quit.

(4.0)
*/
BOOL
handleQuitReview ()
{
	__block BOOL	result = NO;
	SInt32			sessionCount = SessionFactory_ReturnCount() -
									SessionFactory_ReturnStateCount(kSession_StateActiveUnstable); // ignore recently-opened windows
	auto			reviewHandler =
					^{
						// “Review…”; so, display alerts for each open session, individually, and
						// quit after all alerts are closed unless the user cancels one of them.
						// A fairly simple way to handle this is to activate each window in turn,
						// and then display a modal alert asking about the frontmost window.  This
						// is not quite as elegant as using sheets, but since it is synchronous it
						// is WAY easier to write code for!  To help the user better understand
						// which window is being referred to, each window is moved to the center
						// of the main screen using a slide animation prior to popping open an
						// alert.  In addition, all background windows become translucent.
						__block Boolean		cancelQuit = false;
						
						
						result = YES;
						
						// iterate over each session in a MODAL fashion, highlighting a window
						// and either displaying an alert or discarding the window if it has only
						// been open a short time
						SessionFactory_ForEachSessionCopyList
						(^(SessionRef	inSession,
						   Boolean&		UNUSED_ARGUMENT(outStopFlag))
						{
							unless (gCurrentQuitCancelled)
							{
								NSWindow*		window = Session_ReturnActiveNSWindow(inSession);
								
								
								if (nil != window)
								{
									Session_TerminationDialogOptions	dialogOptions = kSession_TerminationDialogOptionModal;
									
									
									// when displaying a single Close alert over a single
									// window, a standard opening animation is fine; it is
									// now suppressed however if there will be a series
									// of alert windows opened in a sequence
									if (1 != gCurrentQuitInitialSessionCount)
									{
										dialogOptions |= kSession_TerminationDialogOptionNoAlertAnimation;
									}
									
									// if the window was obscured, show it first
									if ((nil != window) && (false == window.isVisible))
									{
										TerminalWindowRef	terminalWindow = [window terminalWindowRef];
										
										
										TerminalWindow_SetObscured(terminalWindow, false);
									}
									
									// all windows became translucent; make sure the alert one is opaque
									window.alphaValue = 1.0;
									
									// enforce a tiny delay between messages, otherwise it may be hard
									// for the user to realize that a new message has appeared for a
									// different window
									// UNIMPLEMENTED
									
									Session_DisplayTerminationWarning(inSession, dialogOptions,
																		^{
																			// action to take if Quit is cancelled; this flag will
																			// cause the last-animated window to return to its
																			// previous position, and prevent any remaining windows
																			// from displaying Close-confirmation messages
																			gCurrentQuitCancelled = YES;
																		});
									
									cancelQuit = gCurrentQuitCancelled;
								}
							}
						});
						
						if (cancelQuit)
						{
							result = NO;
						}
					};
	
	
	// if the number of “stable” sessions (not recently opened) is just 1,
	// bypass the Quit step and just do the Close question directly;
	// otherwise, start with the Quit prompt and see if the user wants to
	// iteratively perform the Close check on all the stable sessions
	if (sessionCount <= 1)
	{
		// display Close prompt only
		reviewHandler();
	}
	else
	{
		// display a Quit prompt with options that include “Discard All”
		// to quit immediately; “Cancel”; or “Review…” to iteratively
		// display application-modal Close confirmation windows for each
		// stable session window (windows that have opened recently are
		// automatically skipped so as not to waste the user’s time)
		AlertMessages_BoxWrap	box(Alert_NewApplicationModal(),
									AlertMessages_BoxWrap::kAlreadyRetained);
		
		
		Alert_SetHelpButton(box.returnRef(), false);
		Alert_SetParamsFor(box.returnRef(), kAlert_StyleDontSaveCancelSave);
		Alert_DisableCloseAnimation(box.returnRef());
		// set message
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			primaryTextCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowQuitPrimaryText, primaryTextCFString);
			if (stringResult.ok())
			{
				CFStringRef		helpTextCFString = nullptr;
				
				
				stringResult = UIStrings_Copy(kUIStrings_AlertWindowQuitHelpText, helpTextCFString);
				if (stringResult.ok())
				{
					Alert_SetTextCFStrings(box.returnRef(), primaryTextCFString, helpTextCFString);
					CFRelease(helpTextCFString);
				}
				CFRelease(primaryTextCFString);
			}
		}
		// set title
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			titleCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowQuitName, titleCFString);
			if (stringResult.ok())
			{
				Alert_SetTitleCFString(box.returnRef(), titleCFString);
				CFRelease(titleCFString);
			}
		}
		// set buttons
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			buttonCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_ButtonReviewWithEllipsis, buttonCFString);
			if (stringResult.ok())
			{
				Alert_SetButtonText(box.returnRef(), kAlert_ItemButton1, buttonCFString);
				CFRelease(buttonCFString);
			}
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton1, reviewHandler);
		}
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			buttonCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_ButtonDiscardAll, buttonCFString);
			if (stringResult.ok())
			{
				Alert_SetButtonText(box.returnRef(), kAlert_ItemButton3, buttonCFString);
				CFRelease(buttonCFString);
			}
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton3,
											^{
												// “Discard All”; no alerts are displayed but the application still quits
												result = YES;
											});
		}
		Alert_Display(box.returnRef()); // retains alert until it is dismissed
	}
	
	return result;
}// handleQuitReview


/*!
Returns the index of the item in the specified menu
whose "action" method returns the given selector, or
-1 if there is no such item.

(4.0)
*/
int
indexOfItemWithAction	(NSMenu*	inMenu,
						 SEL		inSelector)
{
	int		result = -1;
	
	
	for (NSMenuItem* item in [inMenu itemArray])
	{
		if (inSelector == [item action])
		{
			result = [inMenu indexOfItem:item];
		}
	}
	return result;
}// indexOfItemWithAction


/*!
Returns "true" only if at least one listener
would be invoked when the specified command
needs executing.

(3.0)
*/
Boolean
isAnyListenerForCommandExecution	(UInt32		inCommand)
{
	return ListenerModel_IsAnyListenerForEvent(gCommandExecutionListenerModel(), inCommand);
}// isAnyListenerForCommandExecution


/*!
Returns YES only if the specified window appears to be
something that should override a main Carbon-based
window.  This is important for Edit menu commands, for
instance, containing text fields that might otherwise
not work as the user expects.

TEMPORARY, for transitional period only until all code
is moved to Cocoa.

(4.0)
*/
BOOL
isCocoaWindowMoreImportantThanCarbon	(NSWindow*		inWindow)
{
	BOOL	result = (([inWindow level] != NSNormalWindowLevel) ||
						[inWindow isKindOfClass:[Popover_Window class]] ||
						[[inWindow windowController] isKindOfClass:[VectorWindow_Controller class]] ||
						[[inWindow windowController] isKindOfClass:[PrefsWindow_Controller class]] ||
						[[inWindow windowController] isKindOfClass:[TerminalWindow_Controller class]]);
	
	
	return result;
}// isCocoaWindowMoreImportantThanCarbon


/*!
In certain situations it appears that the "isVisible"
message can return YES for Cocoa windows that are
really Carbon windows, when the window is not visible
(and Carbon does not believe the window is visible).

As a TEMPORARY measure (until the transition to Cocoa
is complete), it may be necessary to use this routine
on Cocoa windows that might be Carbon windows,
instead of sending the "isVisible" message directly.

(4.1)
*/
BOOL
isWindowVisible		(NSWindow*		inWindow)
{
	BOOL	result = [inWindow isVisible];
	
	
	return result;
}// isWindowVisible


/*!
Invoked whenever a monitored preference value is changed (see
Commands_Init() to see which preferences are monitored).  This
routine responds by ensuring that menu states are up to date
for the changed preference.

(4.0)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	//Preferences_ChangeContext*	contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagNewCommandShortcutEffect:
		// update internal variable that matches the value of the preference
		// (easier than calling Preferences_GetData() every time!)
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
									sizeof(gNewCommandShortcutEffect), &gNewCommandShortcutEffect))
		{
			gNewCommandShortcutEffect = kCommandNewSessionDefaultFavorite; // assume command, if preference can’t be found
		}
		setNewCommand(gNewCommandShortcutEffect);
		break;
	
	case kPreferences_ChangeNumberOfContexts:
	case kPreferences_ChangeContextName:
		// regenerate menu items
		// TEMPORARY: maybe this should be more granulated than it is
		setUpDynamicMenus();
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Returns YES only if the user preference is set to suppress
automatic creation of new windows.

This should be considered whenever creating a window
automatically, such as on application re-open or at launch.

(4.0)
*/
BOOL
quellAutoNew ()
{
	Boolean		quellAutoNew = false;
	BOOL		result = NO;
	
	
	// get the user’s “don’t auto-new” application preference, if possible
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagDontAutoNewOnApplicationReopen, sizeof(quellAutoNew),
							&quellAutoNew))
	{
		// assume a value if it cannot be found
		quellAutoNew = false;
	}
	
	result = (true == quellAutoNew);
	
	return result;
}// quellAutoNew


/*!
Returns the item number that the first window-specific
item in the Window menu WOULD have.  This assumes there
is a dividing line between the last known command in the
Window menu and the would-be first window item.

(4.0)
*/
int
returnFirstWindowItemAnchor		(NSMenu*	inWindowMenu)
{
	int		result = [inWindowMenu indexOfItemWithTag:kCommands_MenuItemIDPrecedingWindowList];
	assert(result >= 0); // make sure the tag is set correctly in the NIB
	
	
	// skip the dividing line item, then point to the next item (so +2)
	result += 2;
	
	return result;
}// returnFirstWindowItemAnchor


/*!
Returns the menu from the menu bar that corresponds to
the given "kCommands_MenuID..." constant.

(4.0)
*/
NSMenu*
returnMenu	(UInt32		inMenuID)
{
	assert(nil != NSApp);
	NSMenu*			mainMenu = [NSApp mainMenu];
	assert(nil != mainMenu);
	NSMenuItem*		parentMenuItem = [mainMenu itemWithTag:inMenuID];
	assert(nil != parentMenuItem);
	NSMenu*			result = [parentMenuItem submenu];
	assert(nil != result);
	
	
	return result;
}// returnMenu


/*!
If the specified menu item has an associated SessionRef,
it is returned; otherwise, nullptr is returned.  This
should work for the session items in the Window menu.

(4.0)
*/
SessionRef
returnMenuItemSession	(NSMenuItem*	inItem)
{
	SessionRef				result = nullptr;
	Commands_SessionWrap*	wrap = (Commands_SessionWrap*)[inItem representedObject];
	
	
	if (nil != wrap)
	{
		result = wrap->session;
	}
	return result;
}// returnMenuItemSession


/*!
Returns the session currently applicable to TEK commands;
defined if the active non-floating window is either a session
terminal, or a vector graphic that came from a session.

(4.0)
*/
SessionRef
returnTEKSession ()
{
	SessionRef		result = SessionFactory_ReturnUserFocusSession();
	
	
	if (nullptr == result)
	{
		NSWindow*	window = [NSApp mainWindow];
		
		
		if ([[window windowController] isKindOfClass:[VectorWindow_Controller class]])
		{
			VectorWindow_Controller*	asCanvasWC = (VectorWindow_Controller*)[window windowController];
			VectorCanvas_View*			canvasView = [asCanvasWC canvasView];
			VectorCanvas_Ref			frontCanvas = VectorInterpreter_ReturnCanvas([canvasView interpreterRef]);
			
			
			if (nullptr != frontCanvas)
			{
				result = VectorCanvas_ReturnListeningSession(frontCanvas);
			}
		}
	}
	return result;
}// returnTEKSession


/*!
Returns the item in the Window menu corresponding to the
specified session’s window.

(4.0)
*/
NSMenuItem*
returnWindowMenuItemForSession		(SessionRef		inSession)
{
	NSMenu*			windowMenu = returnMenu(kCommands_MenuIDWindow);
	int const		kItemCount = [windowMenu numberOfItems];
	int const		kStartItem = returnFirstWindowItemAnchor(windowMenu);
	int const		kPastEndItem = kStartItem + SessionFactory_ReturnCount();
	NSMenuItem*		result = nil;
	
	
	for (int i = kStartItem; ((i != kPastEndItem) && (i < kItemCount)); ++i)
	{
		NSMenuItem*		item = [windowMenu itemAtIndex:i];
		SessionRef		itemSession = returnMenuItemSession(item);
		
		
		if (itemSession == inSession)
		{
			result = item;
		}
	}
	return result;
}// returnWindowMenuItemForSession


/*!
Returns true only if there is an active terminal window with
a current text search.

This is a helper for validators that commonly rely on this
state.

(4.0)
*/
BOOL
searchResultsExist ()
{
	BOOL				result = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	
	
	result = (nullptr != terminalWindow);
	if (result)
	{
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		
		
		result = TerminalView_SearchResultsExist(view);
	}
	return result;
}// searchResultsExist


/*!
Invoked whenever a monitored connection state is changed
(see Commands_Init() to see which states are monitored).
This routine responds by ensuring that menu states are
up to date for the changed connection state.

(4.0)
*/
void
sessionStateChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inConnectionSettingThatChanged,
						 void*					inEventContextPtr,
						 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inConnectionSettingThatChanged)
	{
	case kSession_ChangeState:
		// update menu item icon to reflect new connection state
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
			switch (Session_ReturnState(session))
			{
			case kSession_StateActiveUnstable:
			case kSession_StateImminentDisposal:
				// a session is appearing or disappearing; rebuild the session items in the Window menu
				setUpWindowMenu(returnMenu(kCommands_MenuIDWindow));
				break;
			
			case kSession_StateBrandNew:
			case kSession_StateInitialized:
			case kSession_StateDead:
			case kSession_StateActiveStable:
			default:
				// existing item update; just change the icon as needed
				setWindowMenuItemMarkForSession(session);
				break;
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// sessionStateChanged


/*!
Invoked whenever a monitored session state is changed
(see Commands_Init() to see which states are monitored).
This routine responds by ensuring that menu states are
up to date for the changed session state.

(4.0)
*/
void
sessionWindowStateChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inSessionSettingThatChanged,
							 void*					inEventContextPtr,
							 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inSessionSettingThatChanged)
	{
	case kSession_ChangeWindowObscured:
		// update menu item icon to reflect new hidden state
		{
			SessionRef			session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			TerminalWindowRef	terminalWindow = Session_ReturnActiveTerminalWindow(session);
			
			
			if (TerminalWindow_IsObscured(terminalWindow))
			{
				NSWindow*		hiddenWindow = TerminalWindow_ReturnNSWindow(terminalWindow);
				NSScreen*		windowScreen = [hiddenWindow screen];
				
				
				if (nil != hiddenWindow)
				{
					// there is no apparent way to programmatically determine exactly where
					// a menu title will be so this is a glorious hack to approximate it...
					CGFloat		menuBarHeight = [[NSApp mainMenu] menuBarHeight];
					CGRect		windowMenuTitleInvertedCGRect = CGRectMake([windowScreen frame].origin.x + 456/* arbitrary */,
																			[windowScreen frame].origin.y + NSHeight([windowScreen frame]) - menuBarHeight,
																			72/* arbitrary width */, menuBarHeight);
					Boolean		noAnimations = false;
					
					
					// determine if animation should occur
					unless (kPreferences_ResultOK ==
							Preferences_GetData(kPreferences_TagNoAnimations,
												sizeof(noAnimations), &noAnimations))
					{
						noAnimations = false; // assume a value, if preference can’t be found
					}
					
					if (noAnimations)
					{
						[hiddenWindow orderOut:nil];
					}
					else
					{
						// make the window zoom into the Window menu’s title area, for visual feedback
						CocoaAnimation_TransitionWindowForHide(hiddenWindow, windowMenuTitleInvertedCGRect);
					}
				}
			}
			setWindowMenuItemMarkForSession(session);
		}
		break;
	
	case kSession_ChangeWindowTitle:
		// update menu item text to reflect new window name
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			CFStringRef		text = nullptr;
			
			
			if (Session_GetWindowUserDefinedTitle(session, text) == kSession_ResultOK)
			{
				NSMenuItem*				item = returnWindowMenuItemForSession(session);
				NSAttributedString*		asAttributedString = attributedStringForWindowMenuItemTitle(STATIC_CAST(text, NSString*));
				
				
				// NOTE: this MUST use "setAttributedTitle:" (not "setTitle:")
				// because addWindowMenuItemForSession() creates items using
				// attributed titles in the first place and the OS appears to
				// ignore "setTitle:" when there is an attributed title
				[item setAttributedTitle:asAttributedString];
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// sessionWindowStateChanged


/*!
Sets the state of a user interface item based on a flag.
Currently only works for menu items and checkmarks.

(4.0)
*/
void
setItemCheckMark	(id <NSValidatedUserInterfaceItem>	inItem,
					 BOOL								inIsChecked)
{
	NSMenuItem*		asMenuItem = (NSMenuItem*)inItem;
	
	
	[asMenuItem setState:((inIsChecked) ? NSOnState : NSOffState)];
}// setItemCheckMark


/*!
Sets the key equivalent of the specified command
to be command-N, automatically changing other
affected commands to use something else.

This has no effect if the user preference for
no menu command keys is set.

(4.0)
*/
void
setNewCommand	(UInt32		inCommandNShortcutCommand)
{
	CFStringRef		charCFString = nullptr;
	NSString*		charNSString = nil;
	NSMenu*			targetMenu = returnMenu(kCommands_MenuIDFile);
	NSArray*		items = [targetMenu itemArray];
	NSMenuItem*		defaultItem = nil;
	NSMenuItem*		logInShellItem = nil;
	NSMenuItem*		shellItem = nil;
	NSMenuItem*		dialogItem = nil;
	
	
	// determine the character that should be used for all key equivalents
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		
		
		stringResult = UIStrings_Copy(kUIStrings_TerminalNewCommandsKeyCharacter, charCFString);
		if (false == stringResult.ok())
		{
			assert(false && "unable to find key equivalent for New commands");
		}
		else
		{
			// use the object for convenience
			charNSString = BRIDGE_CAST(charCFString, NSString*);
		}
	}
	
	// find the commands in the menu that create sessions; these will
	// have their key equivalents set according to user preferences
	for (NSMenuItem* item in items)
	{
		if (@selector(performNewDefault:) == [item action])
		{
			defaultItem = item;
		}
		else if (@selector(performNewLogInShell:) == [item action])
		{
			logInShellItem = item;
		}
		else if (@selector(performNewShell:) == [item action])
		{
			shellItem = item;
		}
		else if (@selector(performNewCustom:) == [item action])
		{
			dialogItem = item;
		}
	}
	
	// first clear the non-command-key modifiers of certain “New” commands
	[defaultItem setKeyEquivalent:@""];
	[defaultItem setKeyEquivalentModifierMask:0];
	[logInShellItem setKeyEquivalent:@""];
	[logInShellItem setKeyEquivalentModifierMask:0];
	[shellItem setKeyEquivalent:@""];
	[shellItem setKeyEquivalentModifierMask:0];
	[dialogItem setKeyEquivalent:@""];
	[dialogItem setKeyEquivalentModifierMask:0];
	
	// Modifiers are assigned appropriately based on the given command.
	// If a menu item is assigned to command-N, then Default is given
	// whatever key equivalent the item would have otherwise had.  The
	// NIB is always overridden, although the keys in the NIB act like
	// comments for the key equivalents used in the code below.
	switch (inCommandNShortcutCommand)
	{
	case kCommandNewSessionDefaultFavorite:
		// default case: everything here should match NIB assignments
		{
			NSMenuItem*		item = nil;
			
			
			item = defaultItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask];
			
			item = shellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSControlKeyMask];
			
			item = logInShellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
			
			item = dialogItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSShiftKeyMask];
		}
		break;
	
	case kCommandNewSessionShell:
		// swap Default and Shell key equivalents
		{
			NSMenuItem*		item = nil;
			
			
			item = defaultItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSControlKeyMask];
			
			item = shellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask];
			
			item = logInShellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
			
			item = dialogItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSShiftKeyMask];
		}
		break;
	
	case kCommandNewSessionLoginShell:
		// swap Default and Log-In Shell key equivalents
		{
			NSMenuItem*		item = nil;
			
			
			item = defaultItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
			
			item = shellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSControlKeyMask];
			
			item = logInShellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask];
			
			item = dialogItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSShiftKeyMask];
		}
		break;
	
	case kCommandNewSessionDialog:
		// swap Default and Custom New Session key equivalents
		{
			NSMenuItem*		item = nil;
			
			
			item = defaultItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSShiftKeyMask];
			
			item = shellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSControlKeyMask];
			
			item = logInShellItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
			
			item = dialogItem;
			[item setKeyEquivalent:charNSString];
			[item setKeyEquivalentModifierMask:NSCommandKeyMask];
		}
		break;
	
	default:
		// ???
		break;
	}
	
	if (nullptr != charCFString)
	{
		CFRelease(charCFString), charCFString = nullptr;
	}
}// setNewCommand


/*!
A short-cut for invoking setUpFormatFavoritesMenu() and
other setUp*Menu() routines.  In general, you should use
this routine instead of directly calling any of those.

(4.0)
*/
void
setUpDynamicMenus ()
{
	setUpSessionFavoritesMenu(returnMenu(kCommands_MenuIDFile));
	setUpWorkspaceFavoritesMenu(returnMenu(kCommands_MenuIDFile));
	setUpFormatFavoritesMenu(returnMenu(kCommands_MenuIDView));
	setUpMacroSetsMenu(returnMenu(kCommands_MenuIDMacros));
	setUpTranslationTablesMenu(returnMenu(kCommands_MenuIDKeys));
	setUpWindowMenu(returnMenu(kCommands_MenuIDWindow));
}// setUpDynamicMenus


/*!
Destroys and rebuilds the menu items that automatically change
a terminal format.

(4.0)
*/
void
setUpFormatFavoritesMenu	(NSMenu*	inMenu)
{
	int		pastAnchorIndex = 0;
	
	
	// find the key item to use as an anchor point
	pastAnchorIndex = 1 + indexOfItemWithAction(inMenu, @selector(performFormatDefault:));
	
	if (pastAnchorIndex > 0)
	{
		Quills::Prefs::Class	prefsClass = Quills::Prefs::FORMAT;
		SEL						byNameAction = @selector(performFormatByFavoriteName:);
		int						deletedItemIndex = -1;
		Commands_Result			insertResult = kCommands_ResultOK;
		
		
		// erase previous items
		while (-1 != (deletedItemIndex = indexOfItemWithAction(inMenu, byNameAction)))
		{
			[inMenu removeItemAtIndex:deletedItemIndex];
		}
		
		// add the names of all configurations to the menu
		insertResult = Commands_InsertPrefNamesIntoMenu(prefsClass, inMenu, pastAnchorIndex,
														1/* indentation level */, byNameAction);
		if (false == insertResult.ok())
		{
			Console_Warning(Console_WriteLine, "unexpected error inserting favorites into menu");
		}
	}
}// setUpFormatFavoritesMenu


/*!
Destroys and rebuilds the menu items that automatically change
the active macro set.

(4.0)
*/
void
setUpMacroSetsMenu	(NSMenu*	inMenu)
{
	int		pastAnchorIndex = 0;
	
	
	// find the key item to use as an anchor point
	pastAnchorIndex = 1 + indexOfItemWithAction(inMenu, @selector(performMacroSwitchDefault:));
	
	if (pastAnchorIndex > 0)
	{
		Quills::Prefs::Class	prefsClass = Quills::Prefs::MACRO_SET;
		SEL						byNameAction = @selector(performMacroSwitchByFavoriteName:);
		int						deletedItemIndex = -1;
		Commands_Result			insertResult = kCommands_ResultOK;
		
		
		// erase previous items
		while (-1 != (deletedItemIndex = indexOfItemWithAction(inMenu, byNameAction)))
		{
			[inMenu removeItemAtIndex:deletedItemIndex];
		}
		
		// add the names of all configurations to the menu
		insertResult = Commands_InsertPrefNamesIntoMenu(prefsClass, inMenu, pastAnchorIndex,
														1/* indentation level */, byNameAction);
		if (false == insertResult.ok())
		{
			Console_Warning(Console_WriteLine, "unexpected error inserting favorites into menu");
		}
	}
	
	// unlike most collections in menus, macro sets can define their
	// own key equivalents; it is important to update the menu right
	// away, otherwise Cocoa will only do it the next time the user
	// opens the menu with the mouse
	[inMenu update];
}// setUpMacroSetsMenu


/*!
Destroys and rebuilds the menu items that automatically open
sessions with particular Session Favorite names.

(4.0)
*/
void
setUpSessionFavoritesMenu	(NSMenu*	inMenu)
{
	int		pastAnchorIndex = 0;
	
	
	// find the key item to use as an anchor point
	pastAnchorIndex = 1 + indexOfItemWithAction(inMenu, @selector(performNewDefault:));
	
	if (pastAnchorIndex > 0)
	{
		Quills::Prefs::Class	prefsClass = Quills::Prefs::SESSION;
		SEL						byNameAction = @selector(performNewByFavoriteName:);
		int						deletedItemIndex = -1;
		Commands_Result			insertResult = kCommands_ResultOK;
		
		
		// erase previous items
		while (-1 != (deletedItemIndex = indexOfItemWithAction(inMenu, byNameAction)))
		{
			[inMenu removeItemAtIndex:deletedItemIndex];
		}
		
		// add the names of all configurations to the menu
		insertResult = Commands_InsertPrefNamesIntoMenu(prefsClass, inMenu, pastAnchorIndex,
														1/* indentation level */, byNameAction);
		if (false == insertResult.ok())
		{
			Console_Warning(Console_WriteLine, "unexpected error inserting favorites into menu");
		}
	}
	
	// check to see if a key equivalent should be applied
	setNewCommand(gNewCommandShortcutEffect);
}// setUpSessionFavoritesMenu


/*!
Destroys and rebuilds the menu items that automatically change
the active translation table.

(4.0)
*/
void
setUpTranslationTablesMenu	(NSMenu*	inMenu)
{
	int		pastAnchorIndex = 0;
	
	
	// find the key item to use as an anchor point
	pastAnchorIndex = 1 + indexOfItemWithAction(inMenu, @selector(performTranslationSwitchDefault:));
	
	if (pastAnchorIndex > 0)
	{
		Quills::Prefs::Class	prefsClass = Quills::Prefs::TRANSLATION;
		SEL						byNameAction = @selector(performTranslationSwitchByFavoriteName:);
		int						deletedItemIndex = -1;
		Commands_Result			insertResult = kCommands_ResultOK;
		
		
		// erase previous items
		while (-1 != (deletedItemIndex = indexOfItemWithAction(inMenu, byNameAction)))
		{
			[inMenu removeItemAtIndex:deletedItemIndex];
		}
		
		// add the names of all configurations to the menu
		insertResult = Commands_InsertPrefNamesIntoMenu(prefsClass, inMenu, pastAnchorIndex,
														1/* indentation level */, byNameAction);
		if (false == insertResult.ok())
		{
			Console_Warning(Console_WriteLine, "unexpected error inserting favorites into menu");
		}
	}
}// setUpTranslationTablesMenu


/*!
Destroys and rebuilds the menu items representing open session
windows and their states.

(4.0)
*/
void
setUpWindowMenu		(NSMenu*	inMenu)
{
	int const		kFirstWindowItemIndex = returnFirstWindowItemAnchor(inMenu);
	int const		kDividerIndex = kFirstWindowItemIndex - 1;
	int				deletedItemIndex = -1;
	__block int		numberOfWindowMenuItemsAdded = 0;
	
	
	// erase previous items
	while (-1 != (deletedItemIndex = indexOfItemWithAction(inMenu, @selector(orderFrontSpecificWindow:))))
	{
		[inMenu removeItemAtIndex:deletedItemIndex];
	}
	if (kDividerIndex < [inMenu numberOfItems])
	{
		if ([[inMenu itemAtIndex:kDividerIndex] isSeparatorItem])
		{
			[inMenu removeItemAtIndex:kDividerIndex];
		}
	}
	
	// add the names of all open session windows to the menu
	SessionFactory_ForEachSession
	(^(SessionRef	inSession,
	   Boolean&		UNUSED_ARGUMENT(outStopFlag))
	{
		CFStringRef		nameCFString = nullptr;
		
		
		// if the session has a window, use its title if possible
		if (false == Session_GetWindowUserDefinedTitle(inSession, nameCFString).ok())
		{
			// no window yet; find a descriptive string for this session
			// (resource location will be a remote URL or local Unix command)
			nameCFString = Session_ReturnResourceLocationCFString(inSession);
		}
		if (nullptr == nameCFString)
		{
			nameCFString = CFSTR("<no name or URL found>"); // LOCALIZE THIS?
		}
		
		if (addWindowMenuItemForSession(inSession, inMenu, kDividerIndex, nameCFString))
		{
			++numberOfWindowMenuItemsAdded;
		}
	});
	
	// if any were added, include a dividing line (note also that this
	// item must be erased above)
	if (numberOfWindowMenuItemsAdded > 0)
	{
		[inMenu insertItem:[NSMenuItem separatorItem] atIndex:kDividerIndex];
	}
}// setUpWindowMenu


/*!
Destroys and rebuilds the menu items that automatically open
sessions with particular Workspace Favorite names.

(4.0)
*/
void
setUpWorkspaceFavoritesMenu		(NSMenu*	inMenu)
{
	int		pastAnchorIndex = 0;
	
	
	// find the key item to use as an anchor point
	pastAnchorIndex = 1 + indexOfItemWithAction(inMenu, @selector(performRestoreWorkspaceDefault:));
	
	if (pastAnchorIndex > 0)
	{
		Quills::Prefs::Class	prefsClass = Quills::Prefs::WORKSPACE;
		SEL						byNameAction = @selector(performRestoreWorkspaceByFavoriteName:);
		int						deletedItemIndex = -1;
		Commands_Result			insertResult = kCommands_ResultOK;
		
		
		// erase previous items
		while (-1 != (deletedItemIndex = indexOfItemWithAction(inMenu, byNameAction)))
		{
			[inMenu removeItemAtIndex:deletedItemIndex];
		}
		
		// add the names of all configurations to the menu
		insertResult = Commands_InsertPrefNamesIntoMenu(prefsClass, inMenu, pastAnchorIndex,
														1/* indentation level */, byNameAction);
		if (false == insertResult.ok())
		{
			Console_Warning(Console_WriteLine, "unexpected error inserting favorites into menu");
		}
	}
}// setUpWorkspaceFavoritesSubmenu


/*!
Looks at the terminal window obscured state, session
status, and anything else about the given session
that is significant from a state point of view, and
then updates its Window menu item appropriately.

If you do not provide the menu, the global Window menu
is used.  If you set the item index to zero, then the
proper item for the given session is calculated.

If you already know the menu and item for a session,
you should provide those parameters to speed things up.

(4.0)
*/
void
setWindowMenuItemMarkForSession		(SessionRef		inSession,
									 NSMenuItem*	inItemOrNil)
{
	assert(nullptr != inSession);
	Session_Result		sessionResult = kSession_ResultOK;
	NSMenuItem*			item = (nil == inItemOrNil)
								? returnWindowMenuItemForSession(inSession)
								: inItemOrNil;
	
	
	// now, set the disabled/enabled state
	{
		TerminalWindowRef			terminalWindow = Session_ReturnActiveTerminalWindow(inSession);
		NSMutableAttributedString*	styledText = [[[item attributedTitle] mutableCopyWithZone:NULL] autorelease];
		
		
		if ((nullptr != terminalWindow) && TerminalWindow_IsObscured(terminalWindow))
		{
			// set the style of the item to italic, which makes it more obvious that a window is hidden
			[styledText addAttribute:NSObliquenessAttributeName value:[NSNumber numberWithFloat:0.25]
										range:NSMakeRange(0, [styledText length])];
			if (nil != styledText)
			{
				[item setAttributedTitle:styledText];
			}
			
			// disable the icon itself; with Cocoa menus, this cannot be done
			// directly, the image apparently must be modified manually
			// UNIMPLEMENTED
		}
		else
		{
			// set the style of the item to normal
			[styledText removeAttribute:NSObliquenessAttributeName range:NSMakeRange(0, [styledText length])];
			if (nil != styledText)
			{
				[item setAttributedTitle:styledText];
			}
			
			// enable the icon itself; with Cocoa menus, this cannot be done
			// directly, the image apparently must be modified manually
			// UNIMPLEMENTED
		}
	}
}// setWindowMenuItemMarkForSession


/*!
Returns true only if there is an active terminal window whose
view contains selected text.

This is a helper for validators that commonly rely on this
state.

(4.0)
*/
BOOL
textSelectionExists ()
{
	BOOL				result = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	
	
	result = (nullptr != terminalWindow);
	if (result)
	{
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		
		
		result = TerminalView_TextSelectionExists(view);
	}
	return result;
}// textSelectionExists


/*!
Iterates over all terminal windows and applies or removes
a fade effect (based on whether or not this is the active
application, and if the user has set this preference).

This should be invoked upon supsend/resume.

(2018.04)
*/
void
updateFadeAllTerminalWindows	(Boolean	inInactiveState)
{
	Boolean		fadeWindows = false;
	Float32		alpha = 1.0;
	
	
	unless (kPreferences_ResultOK ==
			Preferences_GetData(kPreferences_TagFadeBackgroundWindows,
			sizeof(fadeWindows), &fadeWindows))
	{
		fadeWindows = false; // assume a value, if preference can’t be found
	}
	
	// modify windows
	if (fadeWindows)
	{
		Float32		fadeAlpha = 1.0;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagFadeAlpha,
				sizeof(fadeAlpha), &fadeAlpha))
		{
			fadeAlpha = 1.0; // assume a value, if preference can’t be found
		}
		
		if (inInactiveState)
		{
			alpha = fadeAlpha;
		}
		
		SessionFactory_ForEachTerminalWindow
		(^(TerminalWindowRef	inTerminalWindow,
		   Boolean&				UNUSED_ARGUMENT(outStopFlag))
		{
			NSWindow* const		kWindow  = (nullptr != inTerminalWindow)
											? TerminalWindow_ReturnNSWindow(inTerminalWindow)
											: nil;
			
			
			// NOTE: tab state is implicit for Cocoa windows
			kWindow.alphaValue = alpha;
		});
	}
}// updateFadeAllTerminalWindows

} // anonymous namespace


#pragma mark -
@implementation Commands_DelayedCommand //{


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
init
{
	assert(false && "invalid way to initialize derived class");
	return [self initWithCommand:0L andEventTarget:nullptr andDelay:0.0];
}// init


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithCommand:(UInt32)			aCommand
andEventTarget:(EventTargetRef)		aTarget
andDelay:(Float32)					aDelay
{
	self = [super init];
	if (nil != self)
	{
		self->commandID = aCommand;
		self->commandTarget = aTarget;
		self->executionDelay = aDelay;
	}
	return self;
}// initWithCommand:andEventTarget:andDelay:


/*!
Executes the command after the delay specified
in the initializer has elapsed.

(4.0)
*/
- (void)
execute
{
	CocoaExtensions_RunLater(self->executionDelay, ^{ [self executeWithoutDelay]; });
}// execute


/*!
Internal; invokes the command immediately.

(4.0)
*/
- (void)
executeWithoutDelay
{
	Commands_ExecuteByIDUsingEvent(self->commandID, self->commandTarget);
}// executeWithoutDelay


@end //} Commands_DelayedCommand


#pragma mark -
@implementation Commands_Executor //{


Commands_Executor*		gCommands_Executor = nil;


@synthesize fullScreenCommandName = _fullScreenCommandName;


/*!
Returns the singleton.

(4.0)
*/
+ (instancetype)
sharedExecutor
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		[[self.class allocWithZone:NULL] init];
		assert(nil != gCommands_Executor);
	});
	return gCommands_Executor;
}


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super init];
	
	// initialize dynamic menu item titles
	{
		CFStringRef		titleCFString = nullptr;
		
		
		if (UIStrings_Copy(kUIStrings_ContextualMenuFullScreenEnter, titleCFString).ok())
		{
			self.fullScreenCommandName = BRIDGE_CAST(titleCFString, NSString*);
			CFRelease(titleCFString), titleCFString = nullptr;
		}
	}
	
	// this approach allows the singleton to be constructed from
	// anywhere, even an object in a NIB
	if (nil == gCommands_Executor)
	{
		gCommands_Executor = self;
	}
	
	return self;
}// init


/*!
Destructor.

(2018.08)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark New Methods: General


/*!
Determines whether or not an action (such as a menu command)
should be available to the user, when no other validation
scheme is defined.

This is a last resort and it should use rules that are
overwhelmingly likely to apply to most menu commands.
Currently the only default rule is the assumption that a
command is usable only when there is a user focus session
window (in other words, by default, if no terminal windows
are active with running processes, commands are assumed to
be unavailable).

(4.1)
*/
- (BOOL)
defaultValidationForAction:(SEL)	aSelector
sender:(id)							anObject
{
#pragma unused(aSelector, anObject)
	// It is actually much more common for a command to apply to a
	// terminal window, than to apply at all times.  Therefore, the
	// default behavior is to allow a command only if there is a
	// terminal window active.  Any command that should always be
	// available will need to define its own validation method,
	// and any command that has no other requirements (aside from a
	// terminal) does not need a validator at all.
	SessionRef	userFocusSession = SessionFactory_ReturnUserFocusSession();
	NSWindow*	sessionWindow = Session_ReturnActiveNSWindow(userFocusSession);
	BOOL		result = ((nullptr != userFocusSession) && (sessionWindow == [NSApp keyWindow]));
	
	
	// by default, assume actions cannot be used in Full Screen mode
	// (most of them would have side effects that could mess up a
	// Full Screen view, e.g. by changing the window size)
	if ((result) && EventLoop_IsMainWindowFullScreen())
	{
		result = NO;
	}
	return result;
}// defaultValidationForAction:sender:


/*!
If there is a validation selector in the responder chain for the
given action selector (see "selectorToValidateAction:"), calls
that validator and returns its YES or NO value.  If no validator
exists, this call performs reasonable default validation actions:
it tries to find a validation selector on the Commands_Executor
class itself first, with the same name that it sought in the
responder chain; barring that, it returns the result of
"defaultValidationForAction:sender:".

NOTE:	Menu items with actions and validators in this class
		are handled implicitly.  This routine must only be
		called explicitly when the convention is not followed,
		such as in toolbar item classes whose action methods
		have generic names that do not match their actions in
		the menu bar.  As one example, if your toolbar item
		generic action is "performToolbarItemAction:" and the
		item is meant to invoke "toggleFullScreen:", the
		"validateUserInterfaceItem:" implementation for the
		item could call "[[Command_Executor sharedExecutor]
		validateAction:@selector(toggleFullScreen:)]" to
		determine its YES or NO value implicitly through the
		validation method "canToggleFullScreen:".

(4.1)
*/
- (BOOL)
validateAction:(SEL)	aSelector
sender:(id)				anObject
{
	SEL		validator = [self.class selectorToValidateAction:aSelector];
	BOOL	result = YES;
	
	
	if (nil != validator)
	{
		id		target = [NSApp targetForAction:validator to:nil from:anObject];
		
		
		if (nil != target)
		{
			// See selectorToValidateAction: for more information on the form of the selector.
			result = [[target performSelector:validator withObject:anObject] boolValue];
		}
		else if ([self respondsToSelector:validator])
		{
			// See selectorToValidateAction: for more information on the form of the selector.
			result = [[self performSelector:validator withObject:anObject] boolValue];
		}
		else
		{
			// It is actually much more common for a command to apply to a
			// terminal window, than to apply at all times.  Therefore, the
			// default behavior is to allow a command only if there is a
			// terminal window active.  Any command that should always be
			// available will need to define its own validation method,
			// and any command that has no other requirements (aside from a
			// terminal) does not need a validator at all.
			result = [self defaultValidationForAction:aSelector sender:anObject];
		}
	}
	return result;
}// validateAction:sender:


#pragma mark NSUserInterfaceValidations


/*!
The standard Cocoa interface for validating things like menu
commands.  This method is present here because it must be in
the same class as the methods that perform actions; if the
action methods move, their validation code must move as well.

Returns true only if the specified item should be available
to the user.

(4.0)
*/
- (BOOL)
validateUserInterfaceItem:(id <NSObject, NSValidatedUserInterfaceItem>)		anItem
{
	return [self validateAction:[anItem action] sender:anItem];
}// validateUserInterfaceItem:


// IMPORTANT: These methods are initially trivial, calling into the
// established Carbon command responder chain.  This is TEMPORARY.
// It will eventually make more sense for these methods to directly
// perform their actions.  It will also be more appropriate to use
// the Cocoa responder chain, and relocate the methods to more
// specific places (for example, NSWindowController subclasses).
// This initial design is an attempt to change as little as possible
// while performing a transition from Carbon to Cocoa.


@end //} Commands_Executor


#pragma mark -
@implementation Commands_Executor (Commands_ApplicationCoreEvents) //{


#pragma mark NSApplicationDelegate


/*!
The standard method used by the default AppKit implementation
when requesting that files be opened (such as, when icons are
dragged to the Dock icon).

(4.0)
*/
- (void)
application:(NSApplication*)	sender
openFiles:(NSArray*)			filenames
{
	BOOL	success = YES;
	
	
	for (NSString* path in filenames)
	{
		// Cocoa overzealously pulls paths from the command line and
		// translates them into open-document requests.  The problem is,
		// this is no normal application, the binary is PYTHON, and it
		// is given the path to "RunApplication.py" as an argument.  There
		// appears to be no way to tell AppKit to ignore the command
		// line, so the work-around is to ignore any request to open a
		// path to "RunApplication.py".
		if ([path hasSuffix:@"RunApplication.py"])
		{
			break;
		}
		
		try
		{
			Quills::Session::handle_file(REINTERPRET_CAST([path UTF8String], char const*));
		}
		catch (std::exception const&	e)
		{
			CFStringRef			titleCFString = CFSTR("Exception during file open"); // LOCALIZE THIS
			CFRetainRelease		messageCFString(CFStringCreateWithCString
												(kCFAllocatorDefault, e.what(), kCFStringEncodingUTF8),
												CFRetainRelease::kAlreadyRetained); // LOCALIZE THIS?
			
			
			Console_WriteScriptError(titleCFString, messageCFString.returnCFStringRef());
			success = NO;
		}
	}
	
	if (success)
	{
		[sender replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
	}
	else
	{
		[sender replyToOpenOrPrint:NSApplicationDelegateReplyFailure];
	}
}// application:openFiles:


/*!
The standard method used by the default AppKit implementation
when this application is already open and is being opened again.
Returns YES only if the default behavior should occur.

(4.0)
*/
- (BOOL)
applicationShouldHandleReopen:(NSApplication*)	sender
hasVisibleWindows:(BOOL)						flag
{
#pragma unused(sender)
	BOOL	result = NO;
	
	
	if ((NO == flag) && (NO == quellAutoNew()))
	{
		// handle the case where the application has no open windows and
		// the user double-clicks the application icon in the Finder
		UInt32		newCommandID = kCommandNewSessionDefaultFavorite;
		
		
		// assume that the user is mapping command-N to the same type of session
		// that would be appropriate for opening by default on startup or re-open
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
									sizeof(newCommandID), &newCommandID))
		{
			// assume a value if it cannot be found
			newCommandID = kCommandNewSessionDefaultFavorite;
		}
		
		// no open windows - respond by spawning a new session
		Commands_ExecuteByIDUsingEvent(newCommandID);
		
		// the event is completely handled, so disable the default behavior
		result = NO;
	}
	
	return result;
}// applicationShouldHandleReopen:hasVisibleWindows:


/*!
Part of the application delegate interface.

(4.0)
*/
- (BOOL)
applicationShouldOpenUntitledFile:(NSApplication*)		sender
{
#pragma unused(sender)
	BOOL	result = NO;
	
	
	// TEMPORARY - it is possible this is never called, and it is
	// not immediately clear why; this needs to be debugged, but
	// for now, startup actions are simply handled in the
	// Initialize module
	return result;
}// applicationShouldOpenUntitledFile:


/*!
The standard method used by the default AppKit implementation
when this application is being asked to quit.

(4.0)
*/
- (NSApplicationTerminateReply)
applicationShouldTerminate:(NSApplication*)		sender
{
#pragma unused(sender)
	NSApplicationTerminateReply		result = NSTerminateNow;
	
	
	gCurrentQuitCancelled = false;
	
	gCurrentQuitInitialSessionCount = SessionFactory_ReturnCount();
	
	// kill all open sessions (asking the user as appropriate), and if the
	// user never cancels, *flags* the main event loop to terminate cleanly
	if (NO == handleQuitReview())
	{
		result = NSTerminateCancel;
	}
	
	return result;
}// applicationShouldTerminate:


#pragma mark NSApplicationNotifications


/*!
Performs various actions when this application comes
to the front.

(2018.04)
*/
- (void)
applicationDidBecomeActive:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	//NSApplication*			application = (NSApplication*)[aNotification object];
	
	
	Alert_SetIsBackgrounded(false); // automatically removes any posted notifications from alerts
	Alert_ServiceNotification();
	updateFadeAllTerminalWindows(false);
}// applicationDidBecomeActive:


/*!
Installs the handlers for Services.  The Info.plist of the main
bundle must also advertise the methods of each Service.

(4.0)
*/
- (void)
applicationDidFinishLaunching:(NSNotification*)		aNotification
{
	NSApplication*				application = STATIC_CAST([aNotification object], NSApplication*);
	Commands_ServiceProviders*	providers = [[[Commands_ServiceProviders alloc] init] autorelease];
	
	
	// note: it is not clear if the system retains the given object, so
	// for now it will not be released (it doesn’t need to be released
	// anyway; it will be needed for the entire lifetime of the program)
	[application setServicesProvider:providers];
}// applicationDidFinishLaunching:


/*!
Installs extra Apple Event handlers.  The Info.plist of the main
bundle must also advertise the types of URLs that are supported.

(4.0)
*/
- (void)
applicationWillFinishLaunching:(NSNotification*)	aNotification
{
	NSApplication*			application = (NSApplication*)[aNotification object];
	NSAppleEventManager*	events = [NSAppleEventManager sharedAppleEventManager];
	
	
	[events setEventHandler:[application delegate] andSelector:@selector(receiveGetURLEvent:replyEvent:)
													forEventClass:kInternetEventClass
													andEventID:kAEGetURL];
}// applicationWillFinishLaunching:


/*!
Performs various actions when another application will
come to the front.

(2018.04)
*/
- (void)
applicationWillResignActive:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	//NSApplication*			application = (NSApplication*)[aNotification object];
	
	
	Alert_SetIsBackgrounded(true); // automatically removes any posted notifications from alerts
	updateFadeAllTerminalWindows(true);
}// applicationWillResignActive:


/*!
Notifies any Quills callback that the application has terminated.

(4.0)
*/
- (void)
applicationWillTerminate:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	Quills::Events::_handle_endloop();
}// applicationWillTerminate:


@end //} Commands_Executor (Commands_ApplicationCoreEvents)


#pragma mark -
@implementation Commands_Executor (Commands_Capturing) //{


- (IBAction)
performCaptureBegin:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandCaptureToFile, nullptr/* target */);
	}
}


- (IBAction)
performCaptureEnd:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandEndCaptureToFile, nullptr/* target */);
	}
}
- (id)
canPerformCaptureEnd:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	TerminalScreenRef	currentScreen = (nullptr == terminalWindow)
										? nullptr
										: TerminalWindow_ReturnScreenWithFocus(terminalWindow);
	BOOL				result = NO;
	
	
	if (nullptr != currentScreen)
	{
		result = (Terminal_FileCaptureInProgress(currentScreen) ? YES : NO);
	}
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performPrintScreen:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandPrintScreen, nullptr/* target */);
	}
}
- (id)
canPerformPrintScreen:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = (nullptr != TerminalWindow_ReturnFromMainWindow());
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performPrintSelection:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandPrint, nullptr/* target */);
	}
}
- (id)
canPerformPrintSelection:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSaveSelection:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSaveSelection, nullptr/* target */);
	}
}
- (id)
canPerformSaveSelection:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_Capturing)


#pragma mark -
@implementation Commands_Executor (Commands_Editing) //{


- (IBAction)
performCopy:(id)	sender
{
	if ((NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]) &&
		(NO == [self viaFirstResponderTryToPerformSelector:@selector(copy:) withObject:sender]))
	{
		// assume this is potentially a Carbon window that should (for now) take a different approach;
		// longer-term this will go away and the responder chain will be used everywhere
		Commands_ExecuteByIDUsingEvent(kCommandCopy, nullptr/* target */);
	}
}
- (id)
canPerformCopy:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	NSWindow*			target = [NSApp keyWindow];
	TerminalWindowRef	terminalWindow = [target terminalWindowRef];
	id					keyResponder = [target firstResponder];
	BOOL				result = NO;
	
	
	if (nullptr != terminalWindow)
	{
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		
		
		result = TerminalView_TextSelectionExists(view);
	}
	else if ([keyResponder respondsToSelector:@selector(canPerformCopy:)])
	{
		// TEMPORARY; until the responder chain is fully-enabled, it’s necessary to
		// manually query the responder for a validation routine
		result = [[keyResponder canPerformCopy:anItem] boolValue];
	}
	else if ((nullptr == terminalWindow) && ([keyResponder respondsToSelector:@selector(copy:)]))
	{
		// text view
		result = ([keyResponder respondsToSelector:@selector(isEditable)] && (NO == [keyResponder isEditable]))
					? NO
					: YES;
		if (result)
		{
			result = ([[target fieldEditor:NO/* create */ forObject:keyResponder] selectedRange].length > 0);
		}
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performCopyAndPaste:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandCopyAndPaste, nullptr/* target */);
	}
}
- (id)
canPerformCopyAndPaste:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performCopyWithTabSubstitution:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandCopyTable, nullptr/* target */);
	}
}
- (id)
canPerformCopyWithTabSubstitution:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performCut:(id)		sender
{
	if ((NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]) &&
		(NO == [self viaFirstResponderTryToPerformSelector:@selector(cut:) withObject:sender]))
	{
		// assume this is potentially a Carbon window that should (for now) take a different approach;
		// longer-term this will go away and the responder chain will be used everywhere
		Commands_ExecuteByIDUsingEvent(kCommandCut, nullptr/* target */);
	}
}
- (id)
canPerformCut:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	NSWindow*			target = [NSApp keyWindow];
	TerminalWindowRef	terminalWindow = [target terminalWindowRef];
	id					keyResponder = [target firstResponder];
	BOOL				result = NO;
	
	
	if (nullptr != terminalWindow)
	{
		result = NO;
	}
	else if ([keyResponder respondsToSelector:@selector(cut:)])
	{
		result = ([keyResponder respondsToSelector:@selector(isEditable)] && (NO == [keyResponder isEditable]))
					? NO
					: YES;
		if (result)
		{
			result = ([[target fieldEditor:NO/* create */ forObject:keyResponder] selectedRange].length > 0);
		}
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performDelete:(id)	sender
{
	if ((NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]) &&
		(NO == [self viaFirstResponderTryToPerformSelector:@selector(delete:) withObject:sender]) &&
		(NO == [self viaFirstResponderTryToPerformSelector:@selector(clear:) withObject:sender]))
	{
		// assume this is potentially a Carbon window that should (for now) take a different approach;
		// longer-term this will go away and the responder chain will be used everywhere
		Commands_ExecuteByIDUsingEvent(kCommandClear, nullptr/* target */);
	}
}
- (id)
canPerformDelete:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	NSWindow*			target = [NSApp keyWindow];
	TerminalWindowRef	terminalWindow = [target terminalWindowRef];
	id					keyResponder = [target firstResponder];
	BOOL				result = NO;
	
	
	if (nullptr != terminalWindow)
	{
		result = NO;
	}
	else if ([keyResponder respondsToSelector:@selector(delete:)] || [keyResponder respondsToSelector:@selector(clear:)])
	{
		result = ([keyResponder respondsToSelector:@selector(isEditable)] && (NO == [keyResponder isEditable]))
					? NO
					: YES;
		if (result)
		{
			result = ([[target fieldEditor:NO/* create */ forObject:keyResponder] selectedRange].length > 0);
		}
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performPaste:(id)	sender
{
	if ((NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]) &&
		(NO == [self viaFirstResponderTryToPerformSelector:@selector(paste:) withObject:sender]))
	{
		// assume this is potentially a Carbon window that should (for now) take a different approach;
		// longer-term this will go away and the responder chain will be used everywhere
		Commands_ExecuteByIDUsingEvent(kCommandPaste, nullptr/* target */);
	}
}
- (id)
canPerformPaste:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	id		keyResponder = [[NSApp keyWindow] firstResponder];
	BOOL	result = NO;
	
	
	if ([keyResponder respondsToSelector:@selector(paste:)])
	{
		result = ([keyResponder respondsToSelector:@selector(isEditable)] && (NO == [keyResponder isEditable]))
					? NO
					: YES;
	}
	else
	{
		CFArrayRef		dummyCFStringArray = nullptr;
		Boolean			clipboardContainsText = false;
		
		
		if (Clipboard_CreateCFStringArrayFromPasteboard(dummyCFStringArray))
		{
			clipboardContainsText = true;
			CFRelease(dummyCFStringArray); dummyCFStringArray = nullptr; 
		}
		result = ((clipboardContainsText) &&
					(nullptr != TerminalWindow_ReturnFromKeyWindow()));
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performRedo:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandRedo, nullptr/* target */);
	}
}
- (id)
canPerformRedo:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL			result = NO;
	NSUndoManager*	undoManager = [[NSApp keyWindow] firstResponder].undoManager;
	
	
	// NOTE: Carbon windows in Cocoa appear to provide their own
	// NSUndoManager instance, which is useless; detect this and
	// ensure NSUndoManager is only used for pure Cocoa windows
	if ((nil != undoManager) && isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		result = [undoManager canRedo];
		if (result)
		{
			NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
			
			
			[asMenuItem setTitle:[undoManager redoMenuItemTitle]];
		}
	}
	else
	{
		CFStringRef		redoCommandName = nullptr;
		Boolean			isEnabled = false;
		
		
		Undoables_GetRedoCommandInfo(redoCommandName, &isEnabled);
		if (false == EventLoop_IsMainWindowFullScreen())
		{
			NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
			
			
			[asMenuItem setTitle:BRIDGE_CAST(redoCommandName, NSString*)];
			result = (isEnabled) ? YES : NO;
		}
	}
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSelectAll:(id)	sender
{
	if ((NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]) &&
		(NO == [self viaFirstResponderTryToPerformSelector:@selector(selectAll:) withObject:sender]))
	{
		// assume this is potentially a Carbon window that should (for now) take a different approach;
		// longer-term this will go away and the responder chain will be used everywhere
		Commands_ExecuteByIDUsingEvent(kCommandSelectAll, nullptr/* target */);
	}
}
- (id)
canPerformSelectAll:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if ([[[NSApp keyWindow] firstResponder] respondsToSelector:@selector(selectAll:)])
	{
		result = YES;
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSelectEntireScrollbackBuffer:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSelectAllWithScrollback, nullptr/* target */);
	}
}


- (IBAction)
performSelectNothing:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSelectNothing, nullptr/* target */);
	}
}


- (IBAction)
performUndo:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandUndo, nullptr/* target */);
	}
}
- (id)
canPerformUndo:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL			result = NO;
	NSUndoManager*	undoManager = [[NSApp keyWindow] firstResponder].undoManager;
	
	
	// NOTE: Carbon windows in Cocoa appear to provide their own
	// NSUndoManager instance, which is useless; detect this and
	// ensure NSUndoManager is only used for pure Cocoa windows
	if ((nil != undoManager) && isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		result = [undoManager canUndo];
		if (result)
		{
			NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
			
			
			[asMenuItem setTitle:[undoManager undoMenuItemTitle]];
		}
	}
	else
	{
		CFStringRef		undoCommandName = nullptr;
		Boolean			isEnabled = false;
		
		
		Undoables_GetUndoCommandInfo(undoCommandName, &isEnabled);
		if (false == EventLoop_IsMainWindowFullScreen())
		{
			NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
			
			
			[asMenuItem setTitle:BRIDGE_CAST(undoCommandName, NSString*)];
			result = (isEnabled) ? YES : NO;
		}
	}
	
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_Editing)


#pragma mark -
@implementation Commands_Executor (Commands_OpeningSessions) //{


- (IBAction)
performDuplicate:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNewDuplicateSession, nullptr/* target */);
	}
}


- (IBAction)
performNewByFavoriteName:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		BOOL	isError = YES;
		
		
		if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
		{
			// use the specified preferences
			NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
			CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
			
			
			if ((nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::SESSION, collectionName))
			{
				Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
															(Quills::Prefs::SESSION, collectionName),
															Preferences_ContextWrap::kAlreadyRetained);
				
				
				if (namedSettings.exists())
				{
					TerminalWindowRef		terminalWindow = SessionFactory_NewTerminalWindowUserFavorite();
					Preferences_ContextRef	workspaceContext = nullptr;
					SessionRef				newSession = SessionFactory_NewSessionUserFavorite
															(terminalWindow, namedSettings.returnRef(), workspaceContext,
																0/* window index */);
					
					
					isError = (nullptr == newSession);
				}
			}
		}
		
		if (isError)
		{
			// failed...
			Sound_StandardAlert();
		}
	}
}
- (id)
canPerformNewByFavoriteName:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performNewCustom:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNewSessionDialog, nullptr/* target */);
	}
}
- (id)
canPerformNewCustom:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performNewDefault:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNewSessionDefaultFavorite, nullptr/* target */);
	}
}
- (id)
canPerformNewDefault:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performNewLogInShell:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNewSessionLoginShell, nullptr/* target */);
	}
}
- (id)
canPerformNewLogInShell:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performNewShell:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNewSessionShell, nullptr/* target */);
	}
}
- (id)
canPerformNewShell:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performKill:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandKillProcessesKeepWindow, nullptr/* target */);
	}
}
- (id)
canPerformKill:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		currentSession = SessionFactory_ReturnUserRecentSession();
	BOOL			result = NO;
	
	
	if (nullptr != currentSession)
	{
		result = (false == Session_StateIsDead(currentSession));
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performRestart:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandRestartSession, nullptr/* target */);
	}
}
- (id)
canPerformRestart:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	// this is not exactly the same as the default validator;
	// in particular, this is permitted in Full Screen mode
	if (nullptr != SessionFactory_ReturnUserFocusSession())
	{
		return @(YES);
	}
	return @(NO);
}


- (IBAction)
performRestoreWorkspaceDefault:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandRestoreWorkspaceDefaultFavorite, nullptr/* target */);
	}
}
- (id)
canPerformRestoreWorkspaceDefault:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performRestoreWorkspaceByFavoriteName:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		BOOL	isError = YES;
		
		
		if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
		{
			// use the specified preferences
			NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
			CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
			
			
			if ((nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::WORKSPACE, collectionName))
			{
				Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
															(Quills::Prefs::WORKSPACE, collectionName),
															Preferences_ContextWrap::kAlreadyRetained);
				
				
				if (namedSettings.exists())
				{
					Boolean		launchedOK = SessionFactory_NewSessionsUserFavoriteWorkspace(namedSettings.returnRef());
					
					
					if (launchedOK) isError = NO;
				}
			}
		}
		
		if (isError)
		{
			// failed...
			Sound_StandardAlert();
		}
	}
}
- (id)
canPerformRestoreWorkspaceByFavoriteName:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performOpen:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandOpenSession, nullptr/* target */);
	}
}
- (id)
canPerformOpen:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
performSaveAs:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSaveSession, nullptr/* target */);
	}
}
- (id)
canPerformSaveAs:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	// this is not exactly the same as the default validator;
	// in particular, this is permitted in Full Screen mode
	if (nullptr != SessionFactory_ReturnUserFocusSession())
	{
		return @(YES);
	}
	return @(NO);
}


/*!
Invoked when a URL is opened by the user, possibly from some
other application.

(4.0)
*/
- (void)
receiveGetURLEvent:(NSAppleEventDescriptor*)	receivedEvent
replyEvent:(NSAppleEventDescriptor*)			replyEvent
{
	BOOL			openOK = NO;
	id< NSObject >	directObject = [receivedEvent paramDescriptorForKeyword:keyDirectObject];
	NSString*		errorString = nil;
	
	
	if (NO == [directObject respondsToSelector:@selector(stringValue)])
	{
		errorString = @"Failed to open URL from event.";
		Console_Warning(Console_WriteLine, "failed to open URL from event");
	}
	else
	{
		NSString*	theURLString = [[receivedEvent paramDescriptorForKeyword:keyDirectObject] stringValue];
		NSURL*		theURL = [NSURL URLWithString:theURLString];
		
		
		openOK = [[NSWorkspace sharedWorkspace] openURL:theURL];
		if (NO == openOK)
		{
			errorString = [NSString stringWithFormat:@"Failed to open URL “%@”.", theURLString];
			Console_Warning(Console_WriteValueCFString, "failed to open URL", BRIDGE_CAST(theURLString, CFStringRef));
		}
	}
	
	// give a reply if possible
	if ((nil != errorString) && (typeNull != [replyEvent descriptorType]))
	{
		[replyEvent setParamDescriptor:[NSAppleEventDescriptor descriptorWithString:errorString]
										forKeyword:keyErrorString];
	}
}


@end //} Commands_Executor (Commands_OpeningSessions)


#pragma mark -
@implementation Commands_Executor (Commands_OpeningVectorGraphics) //{


- (IBAction)
performNewTEKPage:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandTEKPageCommand, nullptr/* target */);
	}
}
- (id)
canPerformNewTEKPage:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	SessionRef		currentSession = returnTEKSession();
	BOOL			result = (nullptr != currentSession);
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performPageClearToggle:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandTEKPageClearsScreen, nullptr/* target */);
	}
}
- (id)
canPerformPageClearToggle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	SessionRef		currentSession = returnTEKSession();
	BOOL			isChecked = NO;
	BOOL			result = (nullptr != currentSession);
	
	
	if (nullptr != currentSession)
	{
		isChecked = (false == Session_TEKPageCommandOpensNewWindow(currentSession));
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_OpeningVectorGraphics)


#pragma mark -
@implementation Commands_Executor (Commands_OpeningWebPages) //{


- (IBAction)
performCheckForUpdates:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandCheckForUpdates, nullptr/* target */);
	}
}
- (id)
canPerformCheckForUpdates:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
performGoToMainWebSite:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandURLHomePage, nullptr/* target */);
	}
}
- (id)
canPerformGoToMainWebSite:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
performOpenURL:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandHandleURL, nullptr/* target */);
	}
}
- (id)
canPerformOpenURL:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == EventLoop_IsMainWindowFullScreen()) && textSelectionExists());
	
	
	if (result)
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		CFRetainRelease		selectedText(TerminalView_ReturnSelectedTextCopyAsUnicode
											(view, 0/* Copy with Tab Substitution info */,
												kTerminalView_TextFlagInline),
											CFRetainRelease::kAlreadyRetained);
		
		
		if (false == selectedText.exists())
		{
			result = NO;
		}
		else
		{
			URL_Type	urlKind = URL_ReturnTypeFromCFString(selectedText.returnCFStringRef());
			
			
			if (kURL_TypeInvalid == urlKind)
			{
				result = NO; // disable command for non-URL text selections
			}
		}
	}
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performProvideFeedback:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandURLAuthorMail, nullptr/* target */);
	}
}
- (id)
canPerformProvideFeedback:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


@end //} Commands_Executor (Commands_OpeningWebPages)


#pragma mark -
@implementation Commands_Executor (Commands_ManagingMacros) //{


- (IBAction)
performActionForMacro:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		UInt16			oneBasedMacroNumber = STATIC_CAST([asMenuItem tag], UInt16);
		
		
		MacroManager_UserInputMacro(oneBasedMacroNumber - 1/* zero-based macro number */);
	}
}
- (id)
canPerformActionForMacro:(id <NSValidatedUserInterfaceItem>)	anItem
{
	Preferences_ContextRef	currentMacros = MacroManager_ReturnCurrentMacros();
	NSMenuItem*				asMenuItem = (NSMenuItem*)anItem;
	UInt16					macroIndex = STATIC_CAST([asMenuItem tag], UInt16);
	Boolean					isTerminalWindowActive = (nullptr != TerminalWindow_ReturnFromMainWindow());
	BOOL					result = (nullptr != currentMacros);
	
	
	if (nullptr == currentMacros)
	{
		// reset the item
		if (5 == macroIndex)
		{
			// LOCALIZE THIS
			[asMenuItem setTitle:NSLocalizedString(@"Macros have been disabled.",
													@"used in Macros menu when no macro set is active")];
		}
		else if (7 == macroIndex)
		{
			// LOCALIZE THIS
			[asMenuItem setTitle:NSLocalizedString(@"To use macros, choose a set from the list below.",
													@"used in Macros menu when no macro set is active")];
		}
		else if (8 == macroIndex)
		{
			// LOCALIZE THIS
			[asMenuItem setTitle:NSLocalizedString(@"Use Preferences to modify macros, or to add or remove sets.",
													@"used in Macros menu when no macro set is active")];
		}
		else
		{
			[asMenuItem setTitle:@""];
		}
		[asMenuItem setKeyEquivalent:@""];
		[asMenuItem setKeyEquivalentModifierMask:0];
	}
	else
	{
		Boolean		macroIsUsable = MacroManager_UpdateMenuItem(asMenuItem, macroIndex/* one-based */, isTerminalWindowActive,
																currentMacros, true/* show inherited */);
		
		
		if ((result) && (false == macroIsUsable))
		{
			result = NO;
		}
	}
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMacroSwitchByFavoriteName:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		BOOL	isError = YES;
		
		
		if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
		{
			// use the specified preferences
			NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
			CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
			
			
			if ((nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::MACRO_SET, collectionName))
			{
				Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
															(Quills::Prefs::MACRO_SET, collectionName),
															Preferences_ContextWrap::kAlreadyRetained);
				
				
				if (namedSettings.exists())
				{
					MacroManager_Result		macrosResult = kMacroManager_ResultOK;
					
					
					macrosResult = MacroManager_SetCurrentMacros(namedSettings.returnRef());
					isError = (false == macrosResult.ok());
				}
			}
		}
		
		if (isError)
		{
			// failed...
			Sound_StandardAlert();
		}
	}
}
- (id)
canPerformMacroSwitchByFavoriteName:(id <NSValidatedUserInterfaceItem>)		anItem
{
	BOOL			result = YES;
	BOOL			isChecked = NO;
	NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
	
	
	if (nullptr != MacroManager_ReturnCurrentMacros())
	{
		CFStringRef		collectionName = nullptr;
		CFStringRef		menuItemName = BRIDGE_CAST([asMenuItem title], CFStringRef);
		
		
		if (nil != menuItemName)
		{
			// the context name should not be released
			Preferences_Result		prefsResult = Preferences_ContextGetName(MacroManager_ReturnCurrentMacros(), collectionName);
			
			
			if (kPreferences_ResultOK == prefsResult)
			{
				isChecked = (kCFCompareEqualTo == CFStringCompare(collectionName, menuItemName, 0/* options */));
			}
		}
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMacroSwitchDefault:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		MacroManager_Result		macrosResult = kMacroManager_ResultOK;
		
		
		macrosResult = MacroManager_SetCurrentMacros(MacroManager_ReturnDefaultMacros());
		if (false == macrosResult.ok())
		{
			Sound_StandardAlert();
		}
	}
}
- (id)
canPerformMacroSwitchDefault:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	result = YES;
	BOOL	isChecked = (MacroManager_ReturnDefaultMacros() == MacroManager_ReturnCurrentMacros());
	
	
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMacroSwitchNone:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		MacroManager_Result		macrosResult = kMacroManager_ResultOK;
		
		
		macrosResult = MacroManager_SetCurrentMacros(nullptr);
		if (false == macrosResult.ok())
		{
			Sound_StandardAlert();
		}
	}
}
- (id)
canPerformMacroSwitchNone:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	result = YES;
	BOOL	isChecked = (nullptr == MacroManager_ReturnCurrentMacros());
	
	
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMacroSwitchNext:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		std::vector< Preferences_ContextRef >	macroSets;
		Boolean									switchOK = false;
		
		
		// NOTE: this list includes “Default”
		if (Preferences_GetContextsInClass(Quills::Prefs::MACRO_SET, macroSets) &&
			(false == macroSets.empty()))
		{
			// NOTE: this should be quite similar to "performMacroSwitchPrevious:"
			MacroManager_Result		macrosResult = kMacroManager_ResultOK;
			
			
			if (gCurrentMacroSetIndex >= (macroSets.size() - 1))
			{
				gCurrentMacroSetIndex = 0;
			}
			else
			{
				++gCurrentMacroSetIndex;
			}
			
			macrosResult = MacroManager_SetCurrentMacros(macroSets[gCurrentMacroSetIndex]);
			switchOK = macrosResult.ok();
		}
		
		if (false == switchOK)
		{
			Sound_StandardAlert();
		}
	}
}
- (id)
canPerformMacroSwitchNext:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = YES;
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMacroSwitchPrevious:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		std::vector< Preferences_ContextRef >	macroSets;
		Boolean									switchOK = false;
		
		
		// NOTE: this list includes “Default”
		if (Preferences_GetContextsInClass(Quills::Prefs::MACRO_SET, macroSets) &&
			(false == macroSets.empty()))
		{
			// NOTE: this should be quite similar to "performMacroSwitchNext:"
			MacroManager_Result		macrosResult = kMacroManager_ResultOK;
			
			
			if (gCurrentMacroSetIndex < 1)
			{
				gCurrentMacroSetIndex = (macroSets.size() - 1);
			}
			else
			{
				--gCurrentMacroSetIndex;
			}
			
			macrosResult = MacroManager_SetCurrentMacros(macroSets[gCurrentMacroSetIndex]);
			switchOK = macrosResult.ok();
		}
		
		if (false == switchOK)
		{
			Sound_StandardAlert();
		}
	}
}
- (id)
canPerformMacroSwitchPrevious:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = YES;
	
	
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_ManagingMacros)


#pragma mark -
@implementation Commands_Executor (Commands_ManagingTerminalEvents) //{


- (IBAction)
performBellToggle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandBellEnabled, nullptr/* target */);
	}
}
- (id)
canPerformBellToggle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	TerminalScreenRef	currentScreen = (nullptr == terminalWindow)
										? nullptr
										: TerminalWindow_ReturnScreenWithFocus(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentScreen)
	{
		isChecked = Terminal_BellIsEnabled(currentScreen);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerNone:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandWatchNothing, nullptr/* target */);
	}
}
- (id)
canPerformSetActivityHandlerNone:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		isChecked = Session_WatchIsOff(currentSession);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerNotifyOnIdle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandWatchForInactivity, nullptr/* target */);
	}
}
- (id)
canPerformSetActivityHandlerNotifyOnIdle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		isChecked = Session_WatchIsForInactivity(currentSession);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerNotifyOnNext:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandWatchForActivity, nullptr/* target */);
	}
}
- (id)
canPerformSetActivityHandlerNotifyOnNext:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		isChecked = Session_WatchIsForPassiveData(currentSession);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSetActivityHandlerSendKeepAliveOnIdle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandTransmitOnInactivity, nullptr/* target */);
	}
}
- (id)
canPerformSetActivityHandlerSendKeepAliveOnIdle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		isChecked = Session_WatchIsForKeepAlive(currentSession);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_ManagingTerminalEvents)


#pragma mark -
@implementation Commands_Executor (Commands_ManagingTerminalKeyMappings) //{


- (IBAction)
performDeleteMapToBackspace:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandDeletePressSendsBackspace, nullptr/* target */);
	}
}
- (id)
canPerformDeleteMapToBackspace:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(currentSession);
		
		
		isChecked = (keyMappings.deleteSendsBackspace) ? YES : NO;
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performDeleteMapToDelete:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandDeletePressSendsDelete, nullptr/* target */);
	}
}
- (id)
canPerformDeleteMapToDelete:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(currentSession);
		
		
		isChecked = (false == keyMappings.deleteSendsBackspace) ? YES : NO;
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performEmacsCursorModeToggle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandEmacsArrowMapping, nullptr/* target */);
	}
}
- (id)
canPerformEmacsCursorModeToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(currentSession);
		
		
		isChecked = (keyMappings.arrowsRemappedForEmacs) ? YES : NO;
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performLocalPageKeysToggle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandLocalPageUpDown, nullptr/* target */);
	}
}
- (id)
canPerformLocalPageKeysToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		Session_EventKeys	keyMappings = Session_ReturnEventKeys(currentSession);
		
		
		isChecked = (keyMappings.pageKeysLocalControl) ? YES : NO;
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMappingCustom:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSetKeys, nullptr/* target */);
	}
}


- (IBAction)
performSetFunctionKeyLayoutRxvt:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutRxvt:sender];
	}
}
- (id)
canPerformSetFunctionKeyLayoutRxvt:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutRxvt ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


- (IBAction)
performSetFunctionKeyLayoutVT220:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutVT220:sender];
	}
}
- (id)
canPerformSetFunctionKeyLayoutVT220:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutVT220 ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


- (IBAction)
performSetFunctionKeyLayoutXTermX11:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutXTermX11:sender];
	}
}
- (id)
canPerformSetFunctionKeyLayoutXTermX11:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutXTerm ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


- (IBAction)
performSetFunctionKeyLayoutXTermXFree86:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutXTermXFree86:sender];
	}
}
- (id)
canPerformSetFunctionKeyLayoutXTermXFree86:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutXTermXFree86 ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return @(YES);
}


- (IBAction)
performTranslationSwitchByFavoriteName:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		SessionRef			session = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
		BOOL				isError = YES;
		
		
		if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
		{
			// use the specified preferences
			NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
			CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
			
			
			if ((nullptr != session) && (nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::TRANSLATION, collectionName))
			{
				Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
															(Quills::Prefs::TRANSLATION, collectionName),
															Preferences_ContextWrap::kAlreadyRetained);
				Preferences_ContextRef		sessionSettings = Session_ReturnTranslationConfiguration(session);
				
				
				if (namedSettings.exists() && (nullptr != sessionSettings))
				{
					Preferences_TagSetRef		translationTags = PrefPanelTranslations_NewTagSet();
					
					
					if (nullptr != translationTags)
					{
						// change character set of frontmost window according to the specified preferences
						Preferences_Result		prefsResult = Preferences_ContextCopy
																(namedSettings.returnRef(), sessionSettings, translationTags);
						
						
						isError = (kPreferences_ResultOK != prefsResult);
						
						Preferences_ReleaseTagSet(&translationTags);
					}
				}
			}
		}
		
		if (isError)
		{
			// failed...
			Console_Warning(Console_WriteLine, "failed to apply named translation settings to session");
			Sound_StandardAlert();
		}
	}
}


- (IBAction)
performTranslationSwitchCustom:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		// legacy Carbon
		Commands_ExecuteByIDUsingEvent(kCommandSetTranslationTable, nullptr/* target */);
	}
}


- (IBAction)
performTranslationSwitchDefault:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandTranslationTableDefault, nullptr/* target */);
	}
}


@end //} Commands_Executor (Commands_ManagingTerminalKeyMappings)


#pragma mark -
@implementation Commands_Executor (Commands_ManagingTerminalSettings) //{


- (IBAction)
performInterruptProcess:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSendInterruptProcess, nullptr/* target */);
	}
}


- (IBAction)
performJumpScrolling:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandJumpScrolling, nullptr/* target */);
	}
}


- (IBAction)
performLineWrapToggle:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandWrapMode, nullptr/* target */);
	}
}
- (id)
canPerformLineWrapToggle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	TerminalScreenRef	currentScreen = (nullptr == terminalWindow)
										? nullptr
										: TerminalWindow_ReturnScreenWithFocus(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentScreen)
	{
		isChecked = Terminal_LineWrapIsEnabled(currentScreen);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performLocalEchoToggle:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandEcho, nullptr/* target */);
	}
}
- (id)
canPerformLocalEchoToggle:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		isChecked = Session_LocalEchoIsEnabled(currentSession);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performReset:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandResetTerminal, nullptr/* target */);
	}
}


- (IBAction)
performSaveOnClearToggle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandClearScreenSavesLines, nullptr/* target */);
	}
}
- (id)
canPerformSaveOnClearToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromKeyWindow();
	TerminalScreenRef	currentScreen = (nullptr == terminalWindow)
										? nullptr
										: TerminalWindow_ReturnScreenWithFocus(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentScreen)
	{
		isChecked = Terminal_SaveLinesOnClearIsEnabled(currentScreen);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performScrollbackClear:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandClearEntireScrollback, nullptr/* target */);
	}
}


- (IBAction)
performSpeechToggle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSpeechEnabled, nullptr/* target */);
	}
}
- (id)
canPerformSpeechToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		isChecked = Session_SpeechIsEnabled(currentSession);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSuspendToggle:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSuspendNetwork, nullptr/* target */);
	}
}
- (id)
canPerformSuspendToggle:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL				isChecked = NO;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	SessionRef			currentSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (nullptr != currentSession)
	{
		isChecked = Session_NetworkIsSuspended(currentSession);
	}
	setItemCheckMark(anItem, isChecked);
	
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_ManagingTerminalSettings)


#pragma mark -
@implementation Commands_Executor (Commands_ModifyingTerminalDimensions) //{


- (IBAction)
performScreenResizeCustom:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		// legacy Carbon
		Commands_ExecuteByIDUsingEvent(kCommandSetScreenSize, nullptr/* target */);
	}
}


- (IBAction)
performScreenResizeNarrower:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNarrowerScreen, nullptr/* target */);
	}
}


- (IBAction)
performScreenResizeShorter:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShorterScreen, nullptr/* target */);
	}
}


- (IBAction)
performScreenResizeStandard:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSmallScreen, nullptr/* target */);
	}
}


- (IBAction)
performScreenResizeTall:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandTallScreen, nullptr/* target */);
	}
}


- (IBAction)
performScreenResizeTaller:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandTallerScreen, nullptr/* target */);
	}
}


- (IBAction)
performScreenResizeWide:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandLargeScreen, nullptr/* target */);
	}
}


- (IBAction)
performScreenResizeWider:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandWiderScreen, nullptr/* target */);
	}
}


@end //} Commands_Executor (Commands_ModifyingTerminalDimensions)


#pragma mark -
@implementation Commands_Executor (Commands_ModifyingTerminalText) //{


- (IBAction)
performFormatByFavoriteName:(id)	sender
{
	// IMPORTANT:	TRANSITIONAL METHOD
	//			NOW ALSO IN: TerminalView_ContentView
	//
	//			(What follows is boilerplate for all such methods.)
	//
	//			This is a transitional method.  Until the full Cocoa runtime can be
	//			adopted, Commands_Executor is king of the responder chain and is the
	//			ONLY thing that receives messages from user interfaces, such as menu
	//			commands.  This means that even if a new-style Cocoa window and view
	//			hierarchy exist, and they implement a method with the same selector
	//			that is associated with a menu item, Cocoa will not know about it;
	//			Commands_Executor must explicitly allow that action to fall through.
	//
	//			In “transitional methods” only, it does exactly that: it checks for a
	//			target, and if it finds one, it manually invokes this same selector
	//			on that target INSTEAD of using THIS implementation.  Otherwise, it
	//			uses this version (e.g. presumably to deal with an old Carbon window).
	//
	//			Initially, transitional methods are largely COPIES of their alternate
	//			implementations (see “NOW ALSO IN”, above).  But over time, the code
	//			below may become simpler, until of course Commands_Executor is
	//			removed completely and the Cocoa runtime and responder chain finally
	//			are used the way they are traditionally used.
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		BOOL				isError = YES;
		
		
		if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
		{
			// use the specified preferences
			NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
			CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
			
			
			if ((nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::FORMAT, collectionName))
			{
				Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
															(Quills::Prefs::FORMAT, collectionName),
															Preferences_ContextWrap::kAlreadyRetained);
				
				
				if (namedSettings.exists())
				{
					// change font and/or colors of frontmost window according to the specified preferences
					Boolean		changeOK = TerminalWindow_ReconfigureViewsInGroup
											(terminalWindow, kTerminalWindow_ViewGroupActive,
												namedSettings.returnRef(), Quills::Prefs::FORMAT);
					
					
					isError = (false == changeOK);
				}
			}
		}
		
		if (isError)
		{
			// failed...
			Sound_StandardAlert();
		}
	}
}


- (IBAction)
performFormatCustom:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		// legacy Carbon
		Commands_ExecuteByIDUsingEvent(kCommandFormat, nullptr/* target */);
	}
}


- (IBAction)
performFormatDefault:(id)	sender
{
	// IMPORTANT:	TRANSITIONAL METHOD
	//			NOW ALSO IN: TerminalView_ContentView
	//
	//			(What follows is boilerplate for all such methods.)
	//
	//			This is a transitional method.  Until the full Cocoa runtime can be
	//			adopted, Commands_Executor is king of the responder chain and is the
	//			ONLY thing that receives messages from user interfaces, such as menu
	//			commands.  This means that even if a new-style Cocoa window and view
	//			hierarchy exist, and they implement a method with the same selector
	//			that is associated with a menu item, Cocoa will not know about it;
	//			Commands_Executor must explicitly allow that action to fall through.
	//
	//			In “transitional methods” only, it does exactly that: it checks for a
	//			target, and if it finds one, it manually invokes this same selector
	//			on that target INSTEAD of using THIS implementation.  Otherwise, it
	//			uses this version (e.g. presumably to deal with an old Carbon window).
	//
	//			Initially, transitional methods are largely COPIES of their alternate
	//			implementations (see “NOW ALSO IN”, above).  But over time, the code
	//			below may become simpler, until of course Commands_Executor is
	//			removed completely and the Cocoa runtime and responder chain finally
	//			are used the way they are traditionally used.
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandFormatDefault, nullptr/* target */);
	}
}


- (IBAction)
performFormatTextBigger:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		// legacy Carbon
		Commands_ExecuteByIDUsingEvent(kCommandBiggerText, nullptr/* target */);
	}
}


- (IBAction)
performFormatTextMaximum:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandZoomMaximumSize, nullptr/* target */);
	}
}


- (IBAction)
performFormatTextSmaller:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		// legacy Carbon
		Commands_ExecuteByIDUsingEvent(kCommandSmallerText, nullptr/* target */);
	}
}


@end //} Commands_Executor (Commands_ModifyingTerminalText)


#pragma mark -
@implementation Commands_Executor (Commands_ModifyingWindows) //{

/*!
A helper method for the various commands that change the
location of a window.

This looks at the specified edge of a window on its screen
and determines which OTHER screen (if any) is closest if
the window were to move away from that line; it then
returns the location and size of that other screen.

The window’s current screen is only used if there are no
other displays available.

(4.1)
*/
- (NSRect)
visibleFrameOfScreenNearEdge:(NSRectEdge)	anEdge
ofWindow:(NSWindow*)						aWindow
didFindOtherScreen:(BOOL*)					outIsOtherScreen
{
	NSRect const	kWindowFrame = [aWindow frame];
	NSArray*		screenArray = [NSScreen screens];
	NSScreen*		windowScreen = [aWindow screen];
	NSRect			result = ((nil != aWindow) && (nil != windowScreen))
								? [windowScreen visibleFrame]
								: [[NSScreen mainScreen] visibleFrame];
	
	
	if ((nil != screenArray) && ([screenArray count] > 0))
	{
		// look at the frames of each display and see which one is
		// most logically “close” to the window in the given direction
		for (NSScreen* currentScreen in screenArray)
		{
			NSRect const	kScreenFrame = [currentScreen visibleFrame];
			BOOL			copyRect = NO;
			
			
			switch (anEdge)
			{
			case NSMinXEdge:
				if (NSMaxX(kScreenFrame) <= NSMinX(kWindowFrame))
				{
					copyRect = YES;
				}
				break;
			
			case NSMaxXEdge:
				if (NSMaxX(kWindowFrame) <= NSMinX(kScreenFrame))
				{
					copyRect = YES;
				}
				break;
			
			case NSMinYEdge:
				if (NSMaxY(kScreenFrame) <= NSMinY(kWindowFrame))
				{
					copyRect = YES;
				}
				break;
			
			case NSMaxYEdge:
				if (NSMaxY(kWindowFrame) <= NSMinY(kScreenFrame))
				{
					copyRect = YES;
				}
				break;
			
			default:
				// ???
				break;
			}
			
			if (copyRect)
			{
				result = NSMakeRect(NSMinX(kScreenFrame), NSMinY(kScreenFrame),
									NSWidth(kScreenFrame), NSHeight(kScreenFrame));
				*outIsOtherScreen = YES;
			}
		}
	}
	
	return result;
}// visibleFrameOfScreenNearEdge:ofWindow:didFindOtherScreen:


/*!
A helper method for the various commands that change the
location of a window.

The window is offset in the specified way unless it hits
a significant edge of the usable space (e.g. if it would
go underneath the menu bar) in which case its location is
adjusted to be right on the boundary.  If the window’s
initial position is on such a boundary however then it is
shifted to the nearest alternate display with animation.

(4.1)
*/
- (void)
moveWindow:(NSWindow*)		aWindow
distance:(UInt16)			aPixelCount
awayFromEdge:(NSRectEdge)	anEdge
withAnimation:(BOOL)		isAnimated
{
	NSRect const	kScreenVisibleFrame = [[aWindow screen] visibleFrame];
	NSRect			newFrame = [aWindow frame];
	
	
	switch (anEdge)
	{
	case NSMinXEdge:
		if (NSMinX(newFrame) == NSMinX(kScreenVisibleFrame))
		{
			// window is already exactly on the left edge; instead of shifting
			// partially offscreen, move the window to the next screen (if any)
			BOOL		isOtherScreen = NO;
			NSRect		newScreenFrame = [self visibleFrameOfScreenNearEdge:anEdge ofWindow:aWindow
																			didFindOtherScreen:&isOtherScreen];
			
			
			if (isOtherScreen)
			{
				// move to right edge of nearest screen
				newFrame.origin.x = (NSMaxX(newScreenFrame) - NSWidth(newFrame));
				
				// the screen sizes may not exactly match along the other axis
				// so determine the proportion of the offset on the previous
				// screen and apply that same proportion to the new screen
				if (NSMaxY(kScreenVisibleFrame) == NSMaxY(newFrame))
				{
					newFrame.origin.y = (NSMaxY(newScreenFrame) - NSHeight(newFrame));
				}
				else if (NSMinY(kScreenVisibleFrame) == NSMinY(newFrame))
				{
					newFrame.origin.y = NSMinY(newScreenFrame);
				}
				else
				{
					newFrame.origin.y = NSMaxY(newScreenFrame) -
										NSHeight(newScreenFrame) * ((NSMaxY(kScreenVisibleFrame) - NSMinY(newFrame)) /
																	NSHeight(kScreenVisibleFrame));
				}
				
				// prevent the title bar from being hidden in some corner cases
				if (NSMaxY(newFrame) > NSMaxY(newScreenFrame))
				{
					newFrame.origin.y = (NSMaxY(newScreenFrame) - NSHeight(newFrame));
				}
				
				// long-distance moves are always animated
				isAnimated = YES;
			}
		}
		else
		{
			// move left
			newFrame.origin.x -= (aPixelCount + (STATIC_CAST(NSMinX(newFrame), UInt16) % aPixelCount));
			
			// prefer touching the edge exactly instead of having the window cut off
			// (but if the window is already exactly on the boundary, let it move)
			if (NSMinX(newFrame) < NSMinX(kScreenVisibleFrame))
			{
				newFrame.origin.x = NSMinX(kScreenVisibleFrame);
			}
		}
		break;
	
	case NSMaxXEdge:
		{
			Float32 const	kScreenRightEdge = NSMaxX(kScreenVisibleFrame);
			
			
			if (NSMaxX(newFrame) == kScreenRightEdge)
			{
				// window is already exactly on the right edge; instead of shifting
				// partially offscreen, move the window to the next screen (if any)
				BOOL		isOtherScreen = NO;
				NSRect		newScreenFrame = [self visibleFrameOfScreenNearEdge:anEdge ofWindow:aWindow
																				didFindOtherScreen:&isOtherScreen];
				
				
				if (isOtherScreen)
				{
					// move to left edge of nearest screen
					newFrame.origin.x = NSMinX(newScreenFrame);
					
					// the screen sizes may not exactly match along the other axis
					// so determine the proportion of the offset on the previous
					// screen and apply that same proportion to the new screen
					if (NSMaxY(kScreenVisibleFrame) == NSMaxY(newFrame))
					{
						newFrame.origin.y = (NSMaxY(newScreenFrame) - NSHeight(newFrame));
					}
					else if (NSMinY(kScreenVisibleFrame) == NSMinY(newFrame))
					{
						newFrame.origin.y = NSMinY(newScreenFrame);
					}
					else
					{
						newFrame.origin.y = NSMaxY(newScreenFrame) -
											NSHeight(newScreenFrame) * ((NSMaxY(kScreenVisibleFrame) - NSMinY(newFrame)) /
																		NSHeight(kScreenVisibleFrame));
					}
					
					// prevent the title bar from being hidden in some corner cases
					if (NSMaxY(newFrame) > NSMaxY(newScreenFrame))
					{
						newFrame.origin.y = (NSMaxY(newScreenFrame) - NSHeight(newFrame));
					}
					
					// long-distance moves are always animated
					isAnimated = YES;
				}
			}
			else
			{
				// move right
				newFrame.origin.x += (aPixelCount + (STATIC_CAST(NSMinX(newFrame), UInt16) % aPixelCount));
				
				// prefer touching the edge exactly instead of having the window cut off
				// (but if the window is already exactly on the boundary, let it move)
				if (NSMaxX(newFrame) > kScreenRightEdge)
				{
					newFrame.origin.x = kScreenRightEdge - NSWidth(newFrame);
				}
			}
		}
		break;
	
	case NSMinYEdge:
		if (NSMinY(newFrame) == NSMinY(kScreenVisibleFrame))
		{
			// window is already exactly on the bottom edge; instead of shifting
			// partially offscreen, move the window to the next screen (if any)
			BOOL		isOtherScreen = NO;
			NSRect		newScreenFrame = [self visibleFrameOfScreenNearEdge:anEdge ofWindow:aWindow
																			didFindOtherScreen:&isOtherScreen];
			
			
			if (isOtherScreen)
			{
				// move to top edge of nearest screen
				newFrame.origin.y = (NSMaxY(newScreenFrame) - NSHeight(newFrame));
				
				// the screen sizes may not exactly match along the other axis
				// so determine the proportion of the offset on the previous
				// screen and apply that same proportion to the new screen
				if (NSMaxX(kScreenVisibleFrame) == NSMaxX(newFrame))
				{
					newFrame.origin.x = (NSMaxX(newScreenFrame) - NSWidth(newFrame));
				}
				else if (NSMinX(kScreenVisibleFrame) == NSMinX(newFrame))
				{
					newFrame.origin.x = NSMinX(newScreenFrame);
				}
				else
				{
					newFrame.origin.x = NSMaxX(newScreenFrame) -
										NSWidth(newScreenFrame) * ((NSMaxX(kScreenVisibleFrame) - NSMinX(newFrame)) /
																	NSWidth(kScreenVisibleFrame));
				}
				
				// prevent the title bar from being hidden in some corner cases
				if (NSMaxY(newFrame) > NSMaxY(newScreenFrame))
				{
					newFrame.origin.y = (NSMaxY(newScreenFrame) - NSHeight(newFrame));
				}
				
				// long-distance moves are always animated
				isAnimated = YES;
			}
		}
		else
		{
			// move down
			newFrame.origin.y -= (aPixelCount + (STATIC_CAST(NSMinY(newFrame), UInt16) % aPixelCount));
			
			// prefer touching the edge exactly instead of having the window cut off
			// (but if the window is already exactly on the boundary, let it move)
			if (NSMinY(newFrame) < NSMinY(kScreenVisibleFrame))
			{
				newFrame.origin.y = NSMinY(kScreenVisibleFrame);
			}
		}
		break;
	
	case NSMaxYEdge:
		{
			Float32 const	kScreenTopEdge = NSMaxY(kScreenVisibleFrame);
			
			
			if (NSMaxY(newFrame) == kScreenTopEdge)
			{
				// window is already exactly on the top edge; instead of shifting
				// partially offscreen, move the window to the next screen (if any)
				BOOL		isOtherScreen = NO;
				NSRect		newScreenFrame = [self visibleFrameOfScreenNearEdge:anEdge ofWindow:aWindow
																				didFindOtherScreen:&isOtherScreen];
				
				
				if (isOtherScreen)
				{
					// move to bottom edge of nearest screen
					newFrame.origin.y = NSMinY(newScreenFrame);
					
					// the screen sizes may not exactly match along the other axis
					// so determine the proportion of the offset on the previous
					// screen and apply that same proportion to the new screen
					if (NSMaxX(kScreenVisibleFrame) == NSMaxX(newFrame))
					{
						newFrame.origin.x = (NSMaxX(newScreenFrame) - NSWidth(newFrame));
					}
					else if (NSMinX(kScreenVisibleFrame) == NSMinX(newFrame))
					{
						newFrame.origin.x = NSMinX(newScreenFrame);
					}
					else
					{
						newFrame.origin.x = NSMaxX(newScreenFrame) -
											NSWidth(newScreenFrame) * ((NSMaxX(kScreenVisibleFrame) - NSMinX(newFrame)) /
																		NSWidth(kScreenVisibleFrame));
					}
					
					// prevent the title bar from being hidden in some corner cases
					if (NSMaxY(newFrame) > NSMaxY(newScreenFrame))
					{
						newFrame.origin.y = (NSMaxY(newScreenFrame) - NSHeight(newFrame));
					}
					
					// long-distance moves are always animated
					isAnimated = YES;
				}
			}
			else
			{
				// move up
				newFrame.origin.y += (aPixelCount + (STATIC_CAST(NSMinY(newFrame), UInt16) % aPixelCount));
				
				// prefer touching the edge exactly instead of having the window cut off
				// (but if the window is already exactly on the boundary, let it move)
				if (NSMaxY(newFrame) > kScreenTopEdge)
				{
					newFrame.origin.y = kScreenTopEdge - NSHeight(newFrame);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	
	if (isAnimated)
	{
		CocoaAnimation_TransitionWindowForMove(aWindow, CGRectMake(NSMinX(newFrame), NSMinY(newFrame),
																	NSWidth(newFrame), NSHeight(newFrame)));
		[aWindow setFrameOrigin:newFrame.origin];
	}
	else
	{
		[aWindow setFrameOrigin:newFrame.origin];
	}
}// moveWindow:distance:awayFromEdge:withAnimation:


- (IBAction)
mergeAllWindows:(id)	sender
{
	if (/*(NO == [self viaFirstResponderTryToPerformSelector:@selector(performMergeAllWindows:) withObject:sender]) &&*/
		(NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]))
	{
		Console_Warning(Console_WriteLine, "merging tabs is not implemented for legacy Carbon windows");
	}
}
- (id)
canMergeAllWindows:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	
	
	if ((nullptr != terminalWindow) &&
		(false == TerminalWindow_IsFullScreen(terminalWindow)) &&
		TerminalWindow_IsTab(terminalWindow))
	{
		return @(YES);
	}
	return @(NO);
}


- (IBAction)
moveTabToNewWindow:(id)		sender
{
	if (/*(NO == [self viaFirstResponderTryToPerformSelector:@selector(performMoveTabToNewWindow:) withObject:sender]) &&*/
		(NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]))
	{
		Commands_ExecuteByIDUsingEvent(kCommandTerminalNewWorkspace, nullptr/* target */);
	}
}
- (id)
canMoveTabToNewWindow:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	
	
	if ((nullptr != terminalWindow) &&
		(false == TerminalWindow_IsFullScreen(terminalWindow)) &&
		TerminalWindow_IsTab(terminalWindow))
	{
		return @(YES);
	}
	return @(NO);
}


- (IBAction)
performArrangeInFront:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandStackWindows, nullptr/* target */);
	}
}


- (IBAction)
performHideOtherWindows:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandHideOtherWindows, nullptr/* target */);
	}
}


- (IBAction)
performHideWindow:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandHideFrontWindow, nullptr/* target */);
	}
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
performMaximize:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandMaximizeWindow, nullptr/* target */);
	}
}
- (id)
canPerformMaximize:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	id		target = [NSApp targetForAction:@selector(performMaximize:)];
	
	
	if (false == EventLoop_IsMainWindowFullScreen())
	{
		if ([[target class] isSubclassOfClass:[NSWindow class]])
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			result = (0 != ([window styleMask] & NSResizableWindowMask));
		}
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performMoveWindowRight:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
								distance:8/* arbitrary; should match performMoveWindowLeft: */
								awayFromEdge:NSMaxXEdge withAnimation:NO];
		}
	}
}


- (IBAction)
performMoveWindowLeft:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
								distance:8/* arbitrary; should match performMoveWindowRight: */
								awayFromEdge:NSMinXEdge withAnimation:NO];
		}
	}
}


- (IBAction)
performMoveWindowDown:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
								distance:8/* arbitrary; should match performMoveWindowUp: */
								awayFromEdge:NSMinYEdge withAnimation:NO];
		}
	}
}


- (IBAction)
performMoveWindowUp:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
								distance:8/* arbitrary; should match performMoveWindowDown: */
								awayFromEdge:NSMaxYEdge withAnimation:NO];
		}
	}
}


- (IBAction)
performRename:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandChangeWindowTitle, nullptr/* target */);
	}
}
- (id)
canPerformRename:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = ((nullptr != TerminalWindow_ReturnFromKeyWindow()) ||
						(nullptr != returnTEKSession()));
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performShowHiddenWindows:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowAllHiddenWindows, nullptr/* target */);
	}
}
- (id)
canPerformShowHiddenWindows:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	__block BOOL	result = NO;
	
	
	SessionFactory_ForEachTerminalWindow
	(^(TerminalWindowRef	inTerminalWindow,
	   Boolean&				outStop)
	{
		if (TerminalWindow_IsObscured(inTerminalWindow))
		{
			result = YES;
			outStop = true;
		}
	});
	
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_ModifyingWindows)


#pragma mark -
@implementation Commands_Executor (Commands_Searching) //{


- (IBAction)
performFind:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandFind, nullptr/* target */);
	}
}
- (id)
canPerformFind:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	BOOL				result = (nullptr != terminalWindow);
	
	
	if (NO == result)
	{
		// also permitted for the Preferences window
		result = [[[NSApp mainWindow] windowController] isKindOfClass:PrefsWindow_Controller.class];
	}
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performFindCursor:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandFindCursor, nullptr/* target */);
	}
}
- (id)
canPerformFindCursor:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	BOOL				result = (nullptr != terminalWindow);
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performFindNext:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandFindAgain, nullptr/* target */);
	}
}
- (id)
canPerformFindNext:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = searchResultsExist();
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performFindPrevious:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandFindPrevious, nullptr/* target */);
	}
}
- (id)
canPerformFindPrevious:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = searchResultsExist();
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performShowCompletions:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		NSWindow*			target = [NSApp keyWindow];
		TerminalWindowRef	terminalWindow = [target terminalWindowRef];
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		
		
		TerminalView_DisplayCompletionsUI(view);
	}
}
- (id)
canPerformShowCompletions:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	NSWindow*			target = [NSApp keyWindow];
	TerminalWindowRef	terminalWindow = [target terminalWindowRef];
	BOOL				result = (nullptr != terminalWindow);
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSendMenuItemText:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		if ([[sender class] isSubclassOfClass:NSMenuItem.class])
		{
			SessionRef			focusSession = SessionFactory_ReturnUserFocusSession();
			TerminalWindowRef	terminalWindow = (nullptr != focusSession)
													? Session_ReturnActiveTerminalWindow(focusSession)
													: nullptr;
			TerminalViewRef		view = (nullptr != terminalWindow)
										? TerminalWindow_ReturnViewWithFocus(terminalWindow)
										: nullptr;
			
			
			if (nullptr == view)
			{
				Sound_StandardAlert();
				Console_Warning(Console_WriteLine, "unable to send menu item text because a user focus session or view was not found");
			}
			else
			{
				NSMenuItem*			asMenuItem = (NSMenuItem*)sender;
				CFStringRef			asCFString = BRIDGE_CAST([asMenuItem title], CFStringRef);
				CFStringRef			completionCFString = asCFString;
				CFRetainRelease		cursorCFString(TerminalView_ReturnCursorWordCopyAsUnicode(view),
													CFRetainRelease::kAlreadyRetained);
				CFRetainRelease		substringCFString;
				
				
				// since this is meant to be a “completion”, the text
				// currently at the cursor position matters; characters
				// may be pruned from the beginning of the proposed
				// completion string if the cursor text already contains
				// some part of it (case-insensitive)
				if (cursorCFString.exists() && (CFStringGetLength(cursorCFString.returnCFStringRef()) > 0))
				{
					CFIndex const	kOriginalCompletionLength = CFStringGetLength(asCFString);
					CFRange			matchRange = CFStringFind(asCFString, cursorCFString.returnCFStringRef(),
																kCFCompareCaseInsensitive | kCFCompareAnchored);
					
					
					if (matchRange.length > 0)
					{
						substringCFString = CFRetainRelease(CFStringCreateWithSubstring(kCFAllocatorDefault, asCFString,
																						CFRangeMake(matchRange.location + matchRange.length,
																									kOriginalCompletionLength - matchRange.length)),
															CFRetainRelease::kAlreadyRetained);
						completionCFString = substringCFString.returnCFStringRef();
					}
				}
				
				// send appropriate text to the session
				Session_UserInputCFString(focusSession, completionCFString);
			}
		}
		else
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "unable to send menu item text because given object is not apparently a menu item");
		}
	}
}
- (id)
canPerformSendMenuItemText:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	// NOTE: should be the same criteria as showing completions
	NSWindow*			target = [NSApp keyWindow];
	TerminalWindowRef	terminalWindow = [target terminalWindowRef];
	BOOL				result = (nullptr != terminalWindow);
	
	
	return ((result) ? @(YES) : @(NO));
}

@end //} Commands_Executor (Commands_Searching)


#pragma mark -
@implementation Commands_Executor (Commands_ShowingPanels) //{


- (IBAction)
orderFrontAbout:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandAboutThisApplication, nullptr/* target */);
	}
}
- (id)
canOrderFrontAbout:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
orderFrontClipboard:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowClipboard, nullptr/* target */);
	}
}
- (id)
canOrderFrontClipboard:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
orderFrontCommandLine:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowCommandLine, nullptr/* target */);
	}
}
- (id)
canOrderFrontCommandLine:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
orderFrontContextualHelp:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandContextSensitiveHelp, nullptr/* target */);
	}
}
- (id)
canOrderFrontContextualHelp:(id <NSValidatedUserInterfaceItem>)		anItem
{
	CFStringRef				keyPhraseCFString = nullptr;
	HelpSystem_KeyPhrase	keyPhrase = HelpSystem_ReturnCurrentContextKeyPhrase();
	HelpSystem_Result		helpResult = kHelpSystem_ResultOK;
	BOOL					result = NO;
	
	
	if (kHelpSystem_KeyPhraseDefault != keyPhrase)
	{
		helpResult = HelpSystem_CopyKeyPhraseCFString(keyPhrase, keyPhraseCFString);
		if (kHelpSystem_ResultOK == helpResult)
		{
			NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
			
			
			[asMenuItem setTitle:(NSString*)keyPhraseCFString];
			result = YES;
			
			CFRelease(keyPhraseCFString), keyPhraseCFString = nullptr;
		}
	}
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
orderFrontControlKeys:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowControlKeys, nullptr/* target */);
	}
}
- (id)
canOrderFrontControlKeys:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
orderFrontDebuggingOptions:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandDebuggingOptions, nullptr/* target */);
	}
}
- (id)
canOrderFrontDebuggingOptions:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
orderFrontIPAddresses:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowNetworkNumbers, nullptr/* target */);
	}
}
- (id)
canOrderFrontIPAddresses:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
orderFrontPreferences:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kHICommandPreferences, nullptr/* target */);
	}
}
- (id)
canOrderFrontPreferences:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL		result = YES;
	
	
	// allowed unless the Preferences window itself is active
	result = (NO == [[[NSApp keyWindow] windowController] isKindOfClass:PrefsWindow_Controller.class]);
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
orderFrontSessionInfo:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowConnectionStatus, nullptr/* target */);
	}
}
- (id)
canOrderFrontSessionInfo:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	if (EventLoop_IsMainWindowFullScreen())
	{
		return @(NO);
	}
	return @(YES);
}


- (IBAction)
orderFrontVT220FunctionKeys:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowFunction, nullptr/* target */);
	}
}
- (id)
canOrderFrontVT220FunctionKeys:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return @(YES);
}


- (IBAction)
orderFrontVT220Keypad:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandShowKeypad, nullptr/* target */);
	}
}
- (id)
canOrderFrontVT220Keypad:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


@end //} Commands_ExecutionDelegate (Commands_ShowingPanels)


#pragma mark -
@implementation Commands_Executor (Commands_SwitchingModes) //{


- (IBAction)
toggleFullScreen:(id)	sender
{
	if (/*(NO == [self viaFirstResponderTryToPerformSelector:@selector(performToggleFullScreen:) withObject:sender]) &&*/
		(NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]))
	{
		// assume this is potentially a Carbon window that should (for now) take a different approach;
		// longer-term this will go away and the responder chain will be used everywhere
		Commands_ExecuteByIDUsingEvent(kCommandFullScreenToggle, nullptr/* target */);
	}
}
- (id)
canToggleFullScreen:(id <NSObject, NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	// TEMPORARY: should not be necessary to check window controller for
	// the test Cocoa terminal but until the Terminal Window module
	// returns objects for such windows it will be necessary
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	BOOL				isCocoaTerminal = [[[NSApp mainWindow] windowController] isKindOfClass:[TerminalWindow_Controller class]];
	BOOL				isGraphicsWindow = [[[NSApp mainWindow] windowController] isKindOfClass:[VectorWindow_Controller class]];
	BOOL				result = ((nullptr != terminalWindow) || (isCocoaTerminal) || isGraphicsWindow);
	
	
	// update “Enter / Exit Full Screen” item name; note that
	// Cocoa bindings cannot be used here because the title
	// binding is read-only (the change to the string would
	// not change the menu item); instead, the item title has
	// to be updated explicitly
	{
		BOOL			isCurrentlyFullScreen = EventLoop_IsMainWindowFullScreen();
		CFStringRef		titleCFString = nullptr;
		
		
		if (UIStrings_Copy((isCurrentlyFullScreen)
							? kUIStrings_ContextualMenuFullScreenExit
							: kUIStrings_ContextualMenuFullScreenEnter, titleCFString).ok())
		{
			self.fullScreenCommandName = BRIDGE_CAST(titleCFString, NSString*);
			CFRelease(titleCFString), titleCFString = nullptr;
		}
		
		if ([anItem isKindOfClass:NSMenuItem.class])
		{
			STATIC_CAST(anItem, NSMenuItem*).title = self.fullScreenCommandName;
		}
		else if ([anItem isKindOfClass:NSToolbarItem.class])
		{
			// toolbar item titles do not change but the help that
			// appears can be updated to indicate the state change
			STATIC_CAST(anItem, NSToolbarItem*).toolTip = self.fullScreenCommandName;
		}
		else if ([anItem isKindOfClass:NSApplication.class])
		{
			// ignore
		}
		else
		{
			Console_Warning(Console_WriteValueCFString, "unable to update Full Screen title of user interface object with unknown class",
							BRIDGE_CAST(NSStringFromClass(anItem.class), CFStringRef));
		}
	}
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
toggleTabOverview:(id)	sender
{
	if (/*(NO == [self viaFirstResponderTryToPerformSelector:@selector(performToggleTabOverview:) withObject:sender]) &&*/
		(NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]))
	{
		Console_Warning(Console_WriteLine, "tab overview is not implemented for legacy Carbon windows");
	}
}
- (id)
canToggleTabOverview:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	BOOL				result = NO;
	
	
	if ((nullptr != terminalWindow) &&
		TerminalWindow_IsTab(terminalWindow))
	{
		NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(terminalWindow);
		
		
		if ([cocoaWindow respondsToSelector:@selector(tabbedWindows)])
		{
			id		tabArray = [cocoaWindow performSelector:@selector(tabbedWindows)];
			
			
			if ((nil != tabArray) && [tabArray isKindOfClass:NSArray.class])
			{
				NSArray*	asArray = STATIC_CAST(tabArray, NSArray*);
				
				
				if (asArray.count > 1)
				{
					result = YES;
				}
			}
		}
	}
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_SwitchingModes)


#pragma mark -
@implementation Commands_Executor (Commands_SwitchingWindows) //{


- (IBAction)
orderFrontNextWindow:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNextWindow, nullptr/* target */);
	}
}
- (id)
canOrderFrontNextWindow:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	// although other variants of this command are disallowed in
	// Full Screen mode, a simple window cycle is OK because the
	// rotation code is able to cycle only between Full Screen
	// windows on the active Space (see activateAnotherWindow())
	BOOL	result = (nil != [NSApp mainWindow]);
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
orderFrontNextWindowHidingPrevious:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandNextWindowHideCurrent, nullptr/* target */);
	}
}
- (id)
canOrderFrontNextWindowHidingPrevious:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = (nil != [NSApp mainWindow]);
	
	
	if ((result) && EventLoop_IsMainWindowFullScreen())
	{
		result = NO;
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
orderFrontPreviousWindow:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandPreviousWindow, nullptr/* target */);
	}
}
- (id)
canOrderFrontPreviousWindow:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = (nil != [NSApp mainWindow]);
	
	
	if ((result) && EventLoop_IsMainWindowFullScreen())
	{
		result = NO;
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
orderFrontSpecificWindow:(id)		sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		BOOL	isError = YES;
		
		
		if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
		{
			NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
			SessionRef		session = returnMenuItemSession(asMenuItem);
			
			
			if (nil != session)
			{
				TerminalWindowRef	terminalWindow = nullptr;
				NSWindow*			window = nullptr;
				
				
				// first make the window visible if it was obscured
				window = Session_ReturnActiveNSWindow(session);
				terminalWindow = Session_ReturnActiveTerminalWindow(session);
				if (nullptr != terminalWindow) TerminalWindow_SetObscured(terminalWindow, false);
				
				// now select the window
				[window makeKeyAndOrderFront:nil];
				
				isError = (nil == window);
			}
		}
		
		if (isError)
		{
			// failed...
			Sound_StandardAlert();
		}
	}
}
- (id)
canOrderFrontSpecificWindow:(id <NSValidatedUserInterfaceItem>)		anItem
{
	BOOL	result = (false == EventLoop_IsMainWindowFullScreen());
	
	
	if (result)
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
		SessionRef		itemSession = returnMenuItemSession(asMenuItem);
		
		
		if (nullptr != itemSession)
		{
			Boolean				isHighlighted = false;
			NSWindow* const		kSessionActiveWindow = Session_ReturnActiveNSWindow(itemSession);
			
			
			isHighlighted = kSessionActiveWindow.isKeyWindow;
			
			if (isHighlighted)
			{
				// check the active window in the menu
				setItemCheckMark(asMenuItem, YES);
			}
			else
			{
				// remove any mark, initially
				setItemCheckMark(asMenuItem, NO);
				
				// use the Mac OS X convention of bullet-marking windows with unsaved changes
				// UNIMPLEMENTED
				
				// use the Mac OS X convention of diamond-marking minimized windows
				// UNIMPLEMENTED
			}
		}
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
toggleTabBar:(id)	sender
{
	if (/*(NO == [self viaFirstResponderTryToPerformSelector:@selector(performToggleTabBar:) withObject:sender]) &&*/
		(NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender]))
	{
		Console_Warning(Console_WriteLine, "tab bar is not implemented for legacy Carbon windows");
	}
}
- (id)
canToggleTabBar:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	BOOL				result = NO;
	
	
	if ((nullptr != terminalWindow) &&
		TerminalWindow_IsTab(terminalWindow))
	{
		NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(terminalWindow);
		
		
		if ([cocoaWindow respondsToSelector:@selector(tabbedWindows)])
		{
			id		tabArray = [cocoaWindow performSelector:@selector(tabbedWindows)];
			
			
			if (nil == tabArray)
			{
				result = YES;
			}
			else if ([tabArray isKindOfClass:NSArray.class])
			{
				NSArray*	asArray = STATIC_CAST(tabArray, NSArray*);
				
				
				if (asArray.count <= 1)
				{
					result = YES;
				}
			}
		}
	}
	return ((result) ? @(YES) : @(NO));
}


@end //} Commands_Executor (Commands_SwitchingWindows)


#pragma mark -
@implementation Commands_Executor (Commands_TransitionFromCarbon) //{


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
//
// This method’s name cannot exactly match the standard
// method used by a Cocoa window.
- (IBAction)
performCloseSetup:(id)	sender
{
	id		target = [NSApp targetForAction:@selector(performClose:)];
	
	
	if (nil != target)
	{
		[NSApp sendAction:@selector(performClose:) to:target from:sender];
	}
}
- (id)
canPerformCloseSetup:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	id		target = [NSApp targetForAction:@selector(performClose:)];
	
	
	if ([[target class] isSubclassOfClass:[NSWindow class]])
	{
		NSWindow*	window = (NSWindow*)target;
		
		
		result = (0 != ([window styleMask] & NSClosableWindowMask));
	}
	return ((result) ? @(YES) : @(NO));
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
performMinimizeSetup:(id)	sender
{
	id		target = [NSApp targetForAction:@selector(performMiniaturize:)];
	
	
	if (nil != target)
	{
		[NSApp sendAction:@selector(performMiniaturize:) to:target from:sender];
	}
}
- (id)
canPerformMinimizeSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == EventLoop_IsMainWindowFullScreen())
	{
		id		target = [NSApp targetForAction:@selector(performMiniaturize:)];
		
		
		if ([[target class] isSubclassOfClass:[NSWindow class]])
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			result = (0 != ([window styleMask] & NSMiniaturizableWindowMask));
		}
	}
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performSpeakSelectedText:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandSpeakSelectedText, nullptr/* target */);
	}
}
- (id)
canPerformSpeakSelectedText:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return ((result) ? @(YES) : @(NO));
}


- (IBAction)
performStopSpeaking:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:_cmd withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kCommandStopSpeaking, nullptr/* target */);
	}
}
- (id)
canPerformStopSpeaking:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = CocoaBasic_SpeakingInProgress();
	
	
	return ((result) ? @(YES) : @(NO));
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
performZoomSetup:(id)	sender
{
	id		target = [NSApp targetForAction:@selector(performZoom:)];
	
	
	if (nil != target)
	{
		[NSApp sendAction:@selector(performZoom:) to:target from:sender];
	}
}
- (id)
canPerformZoomSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == EventLoop_IsMainWindowFullScreen())
	{
		id		target = [NSApp targetForAction:@selector(performZoom:)];
		
		
		if ([[target class] isSubclassOfClass:[NSWindow class]])
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			result = (0 != ([window styleMask] & NSResizableWindowMask));
		}
	}
	return ((result) ? @(YES) : @(NO));
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
runToolbarCustomizationPaletteSetup:(id)	sender
{
	if (NO == [self viaFirstResponderTryToPerformSelector:@selector(runToolbarCustomizationPalette:) withObject:sender])
	{
		Commands_ExecuteByIDUsingEvent(kHICommandCustomizeToolbar, nullptr/* target */);
	}
}
- (id)
canRunToolbarCustomizationPaletteSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == EventLoop_IsMainWindowFullScreen())
	{
		id		target = [NSApp targetForAction:@selector(runToolbarCustomizationPalette:)];
		
		
		if ((target) && ([[target class] isSubclassOfClass:[NSWindow class]]))
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			result = [[window toolbar] allowsUserCustomization];
		}
	}
	return ((result) ? @(YES) : @(NO));
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
toggleToolbarShownSetup:(id)	sender
{
#pragma unused(sender)
	if (NO == [self viaFirstResponderTryToPerformSelector:@selector(toggleToolbarShown:) withObject:sender])
	{
		// target does not support
	}
}
- (id)
canToggleToolbarShownSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == EventLoop_IsMainWindowFullScreen())
	{
		result = (nil != [NSApp mainWindow].toolbar);
	}
	return ((result) ? @(YES) : @(NO));
}


/*!
Internal version of Commands_NewMenuItemForCommand().

(4.1)
*/
- (NSMenuItem*)
newMenuItemForCommand:(UInt32)	aCommandID
itemTitle:(NSString*)			aTitle
ifEnabled:(BOOL)				onlyIfEnabled
{
	NSMenuItem*		result = nil;
	SEL				theSelector = nil;
	BOOL			isEnabled = true;
	
	
	// NOTE: Some key equivalents are arbitrarily added below.
	// They should match whatever is chosen for the same
	// commands in top-level menus.  Also, lower-case letters
	// appear as single capital letters in menus and capital
	// letters implicitly add a shift-key modifier.
	switch (aCommandID)
	{
	case kCommandFullScreenToggle:
		theSelector = @selector(toggleFullScreen:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"f"];
			[result setKeyEquivalentModifierMask:(NSControlKeyMask | NSCommandKeyMask)];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandChangeWindowTitle:
		theSelector = @selector(performRename:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandCloseConnection:
		theSelector = @selector(performCloseSetup:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"w"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandCopy:
		theSelector = @selector(performCopy:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"c"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandCopyTable:
		theSelector = @selector(performCopyWithTabSubstitution:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"C"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandFind:
		theSelector = @selector(performFind:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"f"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandShowCompletions:
		theSelector = @selector(performShowCompletions:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandFormat:
		theSelector = @selector(performFormatCustom:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"t"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandHandleURL:
		theSelector = @selector(performOpenURL:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"u"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandHideFrontWindow:
		theSelector = @selector(performHideWindow:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandPaste:
		theSelector = @selector(performPaste:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"v"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandPrintScreen:
		theSelector = @selector(performPrintScreen:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			unichar		functionKeyChar = NSF13FunctionKey;
			
			
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:[NSString stringWithCharacters:&functionKeyChar
																										length:1]];
			[result setKeyEquivalentModifierMask:0];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandSaveSelection:
		theSelector = @selector(performSaveSelection:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"S"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandSetKeys:
		theSelector = @selector(performMappingCustom:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandSetScreenSize:
		theSelector = @selector(performScreenResizeCustom:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@"k"];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandSpeakSelectedText:
		theSelector = @selector(performSpeakSelectedText:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandStopSpeaking:
		theSelector = @selector(performStopSpeaking:);
		if (onlyIfEnabled)
		{
			isEnabled = CocoaBasic_SpeakingInProgress() ? YES : NO;
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandStackWindows:
		theSelector = @selector(performArrangeInFront:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	case kCommandTerminalNewWorkspace:
		theSelector = @selector(moveTabToNewWindow:);
		if (onlyIfEnabled)
		{
			isEnabled = [self validateAction:theSelector sender:NSApp];
		}
		if (isEnabled)
		{
			result = [[NSMenuItem alloc] initWithTitle:aTitle action:theSelector
														keyEquivalent:@""];
			[result setTarget:self]; // TEMPORARY
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// newMenuItemForCommand:itemTitle:ifEnabled:


@end //} Commands_Executor (Commands_TransitionFromCarbon)


#pragma mark -
@implementation Commands_ServiceProviders //{


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
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


#pragma mark Registered Service Providers


/*!
Of standard <messageName>:userData:error form, this is a
provider for the Service that opens shells to custom working
directories.

The pasteboard data can be a string containing a POSIX pathname
or a file URL, either of which must refer to a directory.

IMPORTANT:	It MUST be advertised in the application bundle’s
			Info.plist file with an NSMessage that matches the
			<messageName> (first) portion of this method name.

(4.0)
*/
- (void)
openPathInShell:(NSPasteboard*)		aPasteboard
userData:(NSString*)				userData
error:(NSString**)					outError // NOTE: really is NSString**, not NSError** (unlike most system methods)
{
#pragma unused(userData)
	NSString*	pathString = nil;
	NSString*	errorString = nil;
	NSArray*	classOrder = @[
								// although NSURL may be a more “logical” class to consider first, resolving
								// an existing NSURL* creates the risk of a HUGE delay if, apparently, it
								// involves any sandbox transitions (conversely, it appears to be “free” if
								// the pasteboard string is used to locally construct an NSURL*); therefore,
								// strings are tested first to give the best chance of a good user experience
								NSString.class,
								NSURL.class,
							];
	NSArray*	possibleMatches = [aPasteboard readObjectsForClasses:classOrder
																		options:@{
																					NSPasteboardURLReadingFileURLsOnlyKey: @(YES),
																				}];
	
	
	// although only one value should typically exist, look
	// at each (stop as soon as a valid path string is found)
	for (id pathObject in possibleMatches)
	{
		if (nil == pathObject)
		{
			// should not happen; skip
			Console_Warning(Console_WriteLine, "open-path-in-shell service received nil object");
		}
		else if ([pathObject isKindOfClass:NSURL.class])
		{
			NSURL*		asURL = STATIC_CAST(pathObject, NSURL*);
			
			
			// strip off the file scheme component and pull out the path
			if ((nil != asURL) && [asURL isFileURL])
			{
				// in case the URL has Apple’s weird file-reference format,
				// first try to convert it into a form whose path is more
				// likely to be usable
				asURL = [asURL filePathURL];
				
				pathString = asURL.path;
			}
		}
		else if ([pathObject isKindOfClass:NSString.class])
		{
			pathString = STATIC_CAST(pathObject, NSString*);
			
			// ignore leading and trailing whitespace
			pathString = [pathString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
			
			// in the off chance that a string in URL form has somehow made it
			// through the filter, strip off the scheme and use the rest
			{
				NSURL*		testURL = nil;
				
				
				// construct a URL from the string (assume that Apple will
				// resolve any “file reference” form of string automatically
				// and do not use the "filePathURL" method in this case)
				testURL = [NSURL URLWithString:pathString];
				
				if ((nil != testURL) && [testURL isFileURL])
				{
					pathString = testURL.path;
				}
			}
			
			// past NSPasteboard methods could sometimes return multiple items
			// as a single newline-separated string; not sure if this is still
			// a possibility when asking for path names but it can’t hurt; look
			// for line endings and split them out when present
			{
				NSArray*	components = [pathString componentsSeparatedByString:@"\015"/* carriage return */];
				
				
				if ((nil != components) && (components.count > 1))
				{
					pathString = STATIC_CAST([components objectAtIndex:0], NSString*);
				}
				else
				{
					components = [pathString componentsSeparatedByString:@"\012"/* line feed */];
					if ((nil != components) && (components.count > 1))
					{
						pathString = STATIC_CAST([components objectAtIndex:0], NSString*);
					}
				}
			}
		}
		else
		{
			// should not see any other type of data; skip
			Console_Warning(Console_WriteValueCFString, "open-path-in-shell service received unexpected object, class", BRIDGE_CAST(NSStringFromClass([pathObject class]), CFStringRef));
		}
		
		if ((nil != pathString) && (NO == [pathString isEqualToString:@""]))
		{
			break;
		}
	}
	
	if (nil == pathString)
	{
		errorString = NSLocalizedStringFromTable(@"Unable to find a path in the given selection.", @"Services"/* table */,
													@"error message for non-strings given to the open-at-path Service provider");
	}
	else
	{
		// if the path points to an existing file, assume that the user
		// actually wants to open a shell to its parent directory
		BOOL				isDirectory = NO;
		NSFileManager*		fileManager = [NSFileManager defaultManager];
		
		
		if ([fileManager fileExistsAtPath:pathString isDirectory:&isDirectory])
		{
			if (NO == isDirectory)
			{
				// a file; open the shell at the parent directory instead
				pathString = [pathString stringByDeletingLastPathComponent];
			}
			else
			{
				// it is a directory; on Mac OS X however, directories can
				// appear to be files (e.g. bundles); see if the directory
				// is really a bundle and if it is, treat it like a file
				// (namely, still look for the parent); otherwise, keep the
				// original directory path as-is
				NSWorkspace*	workspace = [NSWorkspace sharedWorkspace];
				
				
				if ([workspace isFilePackageAtPath:pathString])
				{
					// a bundle; open the shell at the parent directory instead
					pathString = [pathString stringByDeletingLastPathComponent];
				}
			}
		}
		
		if ((nil == pathString) || [pathString isEqualToString:@""])
		{
			errorString = NSLocalizedStringFromTable(@"Specified path no longer exists.", @"Services"/* table */,
														@"error message for nonexistent paths given to the open-at-path Service provider");
		}
	}
	
	if (nil == pathString)
	{
		// at this point all errors should have been set above
		if (nil == errorString)
		{
			errorString = NSLocalizedStringFromTable(@"Specified path cannot be handled for an unknown reason.", @"Services"/* table */,
														@"error message for generic failures in the open-at-path Service provider");
			Console_Warning(Console_WriteLine, "open-path-in-shell service did not correctly account for a nil path string");
		}
	}
	else
	{
		// open log-in shell and change to the specified directory
		TerminalWindowRef	terminalWindow = SessionFactory_NewTerminalWindowUserFavorite();
		SessionRef			newSession = nullptr;
		
		
		// create a shell
		if (nullptr != terminalWindow)
		{
			newSession = SessionFactory_NewSessionDefaultShell(terminalWindow, nullptr/* workspace */, 0/* window index in workspace */,
																BRIDGE_CAST(pathString, CFStringRef)/* current working directory */);
		}
		if (nullptr == newSession)
		{
			errorString = NSLocalizedStringFromTable(@"Unable to start a shell for the given folder.", @"Services"/* table */,
														@"error message when a session cannot be created by the open-at-path Service provider");
		}
		else
		{
			// successfully created; initialize the window title to the starting path
			// to remind the user of the location that was set
			Session_SetWindowUserDefinedTitle(newSession, BRIDGE_CAST(pathString, CFStringRef));
		}
	}
	
	if (nil != errorString)
	{
		AlertMessages_BoxWrap	box(Alert_NewApplicationModal(),
									AlertMessages_BoxWrap::kAlreadyRetained);
		
		
		*outError = errorString;
		Alert_SetParamsFor(box.returnRef(), kAlert_StyleOK);
		Alert_SetIcon(box.returnRef(), kAlert_IconIDNote);
		Alert_SetTextCFStrings(box.returnRef(), BRIDGE_CAST(*outError, CFStringRef),
								(pathString.length > 100/* arbitrary */)
								? BRIDGE_CAST([pathString substringToIndex:99], CFStringRef)
								: BRIDGE_CAST(pathString, CFStringRef)/* help text */);
		Alert_Display(box.returnRef()); // retains alert until it is dismissed
	}
}// openPathInShell:userData:error:


/*!
Of standard <messageName>:userData:error form, this is a
provider for the Service that opens URLs.

IMPORTANT:	It MUST be advertised in the application bundle’s
			Info.plist file with an NSMessage that matches the
			<messageName> (first) portion of this method name.

(4.0)
*/
- (void)
openURL:(NSPasteboard*)		aPasteboard
userData:(NSString*)		userData
error:(NSString**)			outError // NOTE: really is NSString**, not NSError** (unlike most system methods)
{
#pragma unused(userData)
	// NOTE: later versions of Mac OS X change pasteboard APIs significantly; this will eventually change
	NSString*	theURLString = [aPasteboard stringForType:NSStringPboardType];
	NSString*	errorString = nil;
	
	
	if (nil == theURLString)
	{
		theURLString = [aPasteboard stringForType:NSURLPboardType];
	}
	
	if (nil == theURLString)
	{
		errorString = NSLocalizedStringFromTable(@"Unable to find a string in the given selection.", @"Services"/* table */,
													@"error message for non-strings given to the open-URL Service provider");
	}
	else
	{
		NSURL*	testURL = nil;
		
		
		// ignore leading and trailing whitespace
		theURLString = [theURLString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		
		testURL = [NSURL URLWithString:theURLString];
		if ((nil != testURL) && [testURL isFileURL])
		{
			// in case the URL has Apple’s weird file-reference format,
			// first try to convert it into a form whose path is more
			// likely to be usable
			testURL = [testURL filePathURL];
		}
		
		if ((nil == testURL) || (nil == testURL.scheme))
		{
			errorString = NSLocalizedStringFromTable(@"Unable to find an actual URL in the given selection.", @"Services"/* table */,
														@"error message for non-URLs given to the open-URL Service provider");
		}
		else
		{
			// handle the URL!
			std::string		urlString = [theURLString UTF8String];
			
			
			try
			{
				Quills::Session::handle_url(urlString);
			}
			catch (std::exception const&	e)
			{
				NSString*				titleText = NSLocalizedStringFromTable(@"Exception while trying to handle URL for Service", @"Services"/* table */,
																				@"title of script error");
				CFRetainRelease			messageCFString(CFStringCreateWithCString
														(kCFAllocatorDefault, e.what(), kCFStringEncodingUTF8),
														CFRetainRelease::kAlreadyRetained); // LOCALIZE THIS?
				
				
				Console_WriteScriptError(BRIDGE_CAST(titleText, CFStringRef), messageCFString.returnCFStringRef());
				errorString = NSLocalizedStringFromTable(@"Unable to do anything with the given resource.", @"Services"/* table */,
															@"error message when an exception is raised within the open-URL Service provider");
			}
		}
	}
	
	if (nil != errorString)
	{
		AlertMessages_BoxWrap	box(Alert_NewApplicationModal(),
									AlertMessages_BoxWrap::kAlreadyRetained);
		
		
		*outError = errorString;
		Alert_SetParamsFor(box.returnRef(), kAlert_StyleOK);
		Alert_SetIcon(box.returnRef(), kAlert_IconIDNote);
		Alert_SetTextCFStrings(box.returnRef(), BRIDGE_CAST(*outError, CFStringRef),
								(theURLString.length > 100/* arbitrary */)
								? BRIDGE_CAST([theURLString substringToIndex:99], CFStringRef)
								: BRIDGE_CAST(theURLString, CFStringRef)/* help text */);
		Alert_Display(box.returnRef()); // retains alert until it is dismissed
	}
}// openURL:userData:error:


@end //} Commands_ServiceProviders


#pragma mark -
@implementation Commands_SessionWrap //{


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
init
{
	return [self initWithSession:nullptr];
}// init


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initWithSession:(SessionRef)	aSession
{
	self = [super init];
	if (nil != self)
	{
		// WARNING: There is currently no retain/release mechanism
		// supported for sessions, but there ought to be.
		session = aSession;
	}
	return self;
}// initWithSession:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


@end //} Commands_SessionWrap


#pragma mark -
@implementation Commands_Executor (Commands_ExecutorInternal) //{


#pragma mark Class Methods


/*!
Given a selector name, such as performFooBar:, returns the
conventional name for a selector to do validation for that
action; in this case, canPerformFooBar:.

The idea is to avoid constantly modifying the validation
code, and to simply allow any conventionally-named method
to act as the validator for an action of a certain name.
See validateUserInterfaceItem:.

(4.0)
*/
+ (NSString*)
selectorNameForValidateActionName:(NSString*)	anActionSelectorName
{
	NSString*	result = nil;
	
	
	if (nil != anActionSelectorName)
	{
		NSMutableString*	nameOfAction = [[[NSMutableString alloc] initWithString:anActionSelectorName]
											autorelease];
		NSString*			actionFirstChar = [nameOfAction substringToIndex:1];
		
		
		[nameOfAction replaceCharactersInRange:NSMakeRange(0, 1/* length */)
												withString:[actionFirstChar uppercaseString]];
		result = [@"can" stringByAppendingString:nameOfAction];
	}
	return result;
}// selectorNameForValidateActionName:


/*!
Given a selector, such as @selector(performFooBar:), returns
the conventional selector for a method that would validate it;
in this case, @selector(canPerformFooBar:).

The signature of the validator is expected to be:
- (id) canPerformFooBar:(id <NSValidatedUserInterfaceItem>);
Due to limitations in performSelector:, the result is not a
BOOL, but rather an object of type NSNumber, whose "boolValue"
method is called.  Validators are encouraged to use the values
@(YES) or @(NO) when returning their results.

See selectorNameForValidateActionName: and
validateUserInterfaceItem:.

(4.0)
*/
+ (SEL)
selectorToValidateAction:(SEL)	anAction
{
	SEL		result = NSSelectorFromString([self.class selectorNameForValidateActionName:NSStringFromSelector(anAction)]);
	
	
	return result;
}// selectorToValidateAction:


#pragma mark New Methods


/*!
Traverses the responder chain to find a suitable target
object that supports the given selector (other than
this object), and if found invokes the selector on that
object with the given sender as a parameter.

Returns YES only if an action could be performed.

(2018.03)
*/
- (BOOL)
viaFirstResponderTryToPerformSelector:(SEL)		aSelector
withObject:(id)									aSenderOrNil
{
	BOOL	result = NO;
	id		target = [NSApp targetForAction:aSelector to:nil from:aSenderOrNil];
	
	
	if (self != target)
	{
		result = [NSApp sendAction:aSelector to:target from:aSenderOrNil];
	}
	
	return result;
}// viaFirstResponderTryToPerformSelector:withObject:



@end //} Commands_Executor (Commands_ExecutorInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
