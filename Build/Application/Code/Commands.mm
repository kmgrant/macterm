/*!	\file Commands.mm
	\brief Access to various user commands that tend to
	be available in the menu bar.
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

#import "Commands.h"
#import <UniversalDefines.h>

// standard-C includes
#import <climits>
#import <cstdio>
#import <cstring>
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
@class NSCarbonWindow; // WARNING: not for general use, see actual usage below
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <AutoPool.objc++.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
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
#import "ContextualMenuBuilder.h"
#import "DebugInterface.h"
#import "DialogUtilities.h"
#import "EventLoop.h"
#import "Folder.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "MacroManager.h"
#import "MenuBar.h"
#import "PrefPanelTranslations.h"
#import "PrefsWindow.h"
#import "PrintTerminal.h"
#import "QuillsEvents.h"
#import "QuillsSession.h"
#import "Session.h"
#import "SessionDescription.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "Terminology.h"
#import "UIStrings.h"
#import "URL.h"
#import "URLAccessAE.h"
#import "VectorCanvas.h"
#import "VectorInterpreter.h"
#import "VectorWindow.h"
#import "WindowTitleDialog.h"



#pragma mark Types

/*!
Calls Commands_ExecuteByIDUsingEvent() after a delay.

This is a transitional object that will not really be needed
once the application moves to Cocoa completely.
*/
@interface Commands_DelayedCommand : NSObject
{
@private
	UInt32			commandID;
	EventTargetRef	commandTarget;
	Float32			executionDelay;
}

- (id)
initWithCommand:(UInt32)_
andEventTarget:(EventTargetRef)_
andDelay:(Float32)_;

- (void)
execute;

@end // Commands_DelayedCommand


/*!
An instance of this class handles the application’s entries in
the Services menu.  Note that the names of messages in this
class should match those published under NSServices (using the
"NSMessage" key) in the generated Info.plist file.
*/
@interface Commands_ServiceProviders : NSObject
{
}

- (void)
openPathInShell:(NSPasteboard*)_
userData:(NSString*)_
error:(NSString**)_;

- (void)
openURL:(NSPasteboard*)_
userData:(NSString*)_
error:(NSString**)_;

@end // Commands_ServiceProviders


/*!
This class exists so that it is easier to associate a SessionRef
with an NSMenuItem, via setRepresentedObject:.
*/
@interface Commands_SessionWrap : NSObject
{
@public
	SessionRef		session;
}

- (id)
initWithSession:(SessionRef)_;

@end // Commands_SessionWrap


namespace {

struct My_MenuItemInsertionInfo
{
	NSMenu*		menu;			//!< the menu in which to create the item
	int			atItemIndex;	//!< the NSMenu index at which the new item should appear
};
typedef My_MenuItemInsertionInfo const*		My_MenuItemInsertionInfoConstPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void				activateAnotherWindow							(Boolean, Boolean);
BOOL				activeCarbonWindowHasSelectedText				();
BOOL				addWindowMenuItemForSession						(SessionRef, My_MenuItemInsertionInfoConstPtr,
																	 CFStringRef);
void				addWindowMenuItemSessionOp						(SessionRef, void*, SInt32, void*);
void				changeNotifyForCommandExecution					(UInt32);
BOOL				handleQuit										(BOOL);
int					indexOfItemWithAction							(NSMenu*, SEL);
Boolean				isAnyListenerForCommandExecution				(UInt32);
BOOL				isCarbonWindow									(id);
BOOL				isCocoaWindowMoreImportantThanCarbon			(NSWindow*);
void				moveWindowAndDisplayTerminationAlertSessionOp	(SessionRef, void*, SInt32, void*);
void				preferenceChanged								(ListenerModel_Ref, ListenerModel_Event,
																	 void*, void*);
BOOL				quellAutoNew									();
void				receiveTerminationWarningAnswer					(ListenerModel_Ref, ListenerModel_Event,
																	 void*, void*);
HIViewRef			returnActiveCarbonWindowFocusedField			();
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
void				setTerminalWindowTranslucency					(TerminalWindowRef, void*, SInt32, void*);
void				setUpDynamicMenus								();
void				setUpFormatFavoritesMenu						(NSMenu*);
void				setUpMacroSetsMenu								(NSMenu*);
void				setUpSessionFavoritesMenu						(NSMenu*);
void				setUpTranslationTablesMenu						(NSMenu*);
void				setUpWindowMenu									(NSMenu*);
void				setUpWorkspaceFavoritesMenu						(NSMenu*);
void				setWindowMenuItemMarkForSession					(SessionRef, NSMenuItem* = nil);
void				showWindowTerminalWindowOp						(TerminalWindowRef, void*, SInt32, void*);
BOOL				textSelectionExists								();
id					validatorYes									(id <NSValidatedUserInterfaceItem>);

} // anonymous namespace

#pragma mark Variables
namespace {

ListenerModel_ListenerRef	gPreferenceChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionStateChangeEventListener = nullptr;
ListenerModel_ListenerRef	gSessionWindowStateChangeEventListener = nullptr;
UInt32						gNewCommandShortcutEffect = kCommandNewSessionDefaultFavorite;
Boolean						gCurrentQuitCancelled = false;
ListenerModel_ListenerRef	gCurrentQuitWarningAnswerListener = nullptr;
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
Determines the most appropriate string to describe
the specified command, given the name type constraint.
Most commands are described by exactly one string,
but occasionally you can get a different string that
is more appropriate for the context you specify.  If
no string specifically matching your context exists,
a more general string is returned.

Returns true as long as at least one string is found
and copied successfully.  Note in particular that you
will not see a false return value when there is no
exact match for your requested string, as long as at
least one other string for the command exists.

DEPRECATED.  Command IDs are not going to be reliable
mappings to strings in many cases, due to the Cocoa
transition.

(3.0)
*/
Boolean
Commands_CopyCommandName	(UInt32				inCommandID,
							 Commands_NameType  inNameType,
							 CFStringRef&		outName)
{
	Boolean		result = false;
	Boolean		useMenuCommandName = false;
	
	
	switch (inCommandID)
	{
	case kCommandFullScreenModal:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemFullScreen, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandNewSessionDefaultFavorite:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemNewSessionDefault, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandNewSessionLoginShell:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemNewSessionLoginShell, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandNewSessionShell:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemNewSessionShell, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandPrint:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemPrint, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandKillProcessesKeepWindow:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemKillSession, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandRestartSession:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemRestartSession, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kHICommandCustomizeToolbar:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemCustomizeToolbar, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandSuspendNetwork:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemSuspendNetwork, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandBellEnabled:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemBell, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandHideFrontWindow:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemHideFrontWindow, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandStackWindows:
		switch (inNameType)
		{
		case kCommands_NameTypeShort:
			if (UIStrings_Copy(kUIStrings_ToolbarItemArrangeAllInFront, outName).ok()) result = true;
			else useMenuCommandName = true;
			break;
		
		case kCommands_NameTypeDefault:
		default:
			useMenuCommandName = true;
			break;
		}
		break;
	
	case kCommandDisplayWindowContextualMenu:
		switch (inNameType)
		{
		case kCommands_NameTypeDefault:
		default:
			// TEMPORARY - REVIEW THIS
			result = false;
			break;
		}
		break;
	
	default:
		// attempt to derive the name from a matching menu command
		useMenuCommandName = true;
		break;
	}
	
	// TEMPORARY - the fallback to copy from a menu no longer works
	
	return result;
}// CopyCommandName


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
		Boolean				isDialog = false;
		
		
		if (EventLoop_ReturnRealFrontWindow() != nullptr)
		{
			isDialog = (kDialogWindowKind == GetWindowKind(EventLoop_ReturnRealFrontWindow()));
		}
		
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
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationApplicationUpdatesPage);
			break;
		
		case kCommandURLHomePage:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationApplicationHomePage);
			break;
		
		case kCommandURLAuthorMail:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationApplicationSupportEMail);
			break;
		
		case kCommandURLSourceLicense:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationSourceCodeLicense);
			break;
		
		case kCommandURLProjectStatus:
			(Boolean)URL_OpenInternetLocation(kURL_InternetLocationSourceForgeProject);
			break;
		
		//case kCommandNewSessionLoginShell:
		//case kCommandNewSessionShell:
		//case kCommandNewSessionDialog:
		//case kCommandNewSessionDefaultFavorite:
		//	see SessionFactory.cp
		//	break;
		
		case kCommandOpenSession:
			SessionDescription_Load();
			break;
		
		case kCommandOpenScriptMenuItemsFolder:
			{
				FSRef		folderRef;
				OSStatus	error = noErr;
				
				
				error = Folder_GetFSRef(kFolder_RefScriptsMenuItems, folderRef);
				if (noErr == error)
				{
					error = LSOpenFSRef(&folderRef, nullptr/* launched item */);
				}
				if (noErr != error)
				{
					Sound_StandardAlert();
				}
			}
			break;
		
		case kCommandCloseConnection:
			// automatically handled by the OS
			break;
		
		case kCommandSaveSession:
			if (isSession) Session_DisplaySaveDialog(frontSession);
			break;
		
		case kCommandSaveText:
			TerminalView_DisplaySaveSelectedTextUI(activeView);
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
				size_t		actualSize = 0;
				
				
				// in Full Screen mode, this command might not always be allowed
				if (kPreferences_ResultOK !=
					Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
										&allowForceQuit, &actualSize))
				{
					allowForceQuit = true; // assume a value if the preference cannot be found
				}
				
				if ((false == FlagManager_Test(kFlagKioskMode)) || (allowForceQuit))
				{
					Session_DisplayTerminationWarning(frontSession, false/* is modal */, true/* force keep window */, false/* restart */);
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
				size_t		actualSize = 0;
				
				
				// in Full Screen mode, this command might not always be allowed
				if (kPreferences_ResultOK !=
					Preferences_GetData(kPreferences_TagKioskAllowsForceQuit, sizeof(allowForceQuit),
										&allowForceQuit, &actualSize))
				{
					allowForceQuit = true; // assume a value if the preference cannot be found
				}
				
				if ((false == FlagManager_Test(kFlagKioskMode)) || (allowForceQuit))
				{
					Session_DisplayTerminationWarning(frontSession, false/* is modal */, true/* force keep window */, true/* restart */);
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
				CFStringRef			jobTitle = nullptr;
				UIStrings_Result	stringResult = (printScreen)
													? UIStrings_Copy(kUIStrings_TerminalPrintScreenJobTitle,
																		jobTitle)
													: UIStrings_Copy(kUIStrings_TerminalPrintSelectionJobTitle,
																		jobTitle);
				
				
				if (nullptr != jobTitle)
				{
					PrintTerminal_JobRef	printJob = (printScreen)
														? PrintTerminal_NewJobFromVisibleScreen
															(activeView, activeScreen, jobTitle)
														: PrintTerminal_NewJobFromSelectedText
															(activeView, jobTitle);
					
					
					if (nullptr != printJob)
					{
						(PrintTerminal_Result)PrintTerminal_JobSendToPrinter
												(printJob, TerminalWindow_ReturnWindow(frontTerminalWindow));
						PrintTerminal_ReleaseJob(&printJob);
					}
					CFRelease(jobTitle), jobTitle = nullptr;
				}
			}
			break;
		
		case kCommandUndo:
			Undoables_UndoLastAction();
			break;
		
		case kCommandRedo:
			Undoables_RedoLastUndo();
			break;
		
		case kCommandCut:
		case kCommandCopy:
		case kCommandCopyTable:
		case kCommandClear:
			if ((inCommandID == kCommandCut) ||
				(inCommandID == kCommandCopy) ||
				(inCommandID == kCommandCopyTable))
			{
				// all of these operations involving copying to the clipboard
				if (isDialog)
				{
					if (inCommandID == kCommandCut) DialogCut(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
					else if (inCommandID == kCommandCopy) DialogCopy(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
				}
				else
				{
					if (nullptr != activeView)
					{
						Clipboard_TextToScrap(activeView, (kCommandCopyTable == inCommandID)
															? kClipboard_CopyMethodTable
															: kClipboard_CopyMethodStandard);
					}
				}
			}
			if (inCommandID == kCommandClear)
			{
				// delete selection -- unimplemented, impossible to implement?
				// at least, impossible until MacTerm supports “generic” terminal
				// windows (i.e. windows that are not tied to read-only concepts
				// like connections to remote servers)
				if (isDialog)
				{
					DialogDelete(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
				}
			}
			break;
		
		case kCommandCopyAndPaste:
			if (isSession)
			{
				// type if there is a window to type into
				CFStringRef		clipboardString = nullptr;
				CFStringRef		actualTypeName = nullptr;
				
				
				Clipboard_TextToScrap(activeView, kClipboard_CopyMethodStandard | kClipboard_CopyMethodInline);
				
				if (Clipboard_CreateCFStringFromPasteboard(clipboardString, actualTypeName, Clipboard_ReturnPrimaryPasteboard()))
				{
					Session_UserInputCFString(frontSession, clipboardString);
					CFRelease(clipboardString), clipboardString = nullptr;
				}
			}
			break;
		
		case kCommandPaste:	
			if (isSession) Session_UserInputPaste(frontSession); // paste if there is a window to paste into
			else if (isDialog) DialogPaste(GetDialogFromWindow(EventLoop_ReturnRealFrontWindow()));
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
			if (isDialog)
			{
				// select the entire text in the focused field
				// unimplemented
				Sound_StandardAlert();
			}
			else if (isSession)
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
			PrefsWindow_Display();
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
		
		//case kCommandFullScreen:
		//case kCommandFullScreenModal:
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
		
		case kCommandTerminalEmulatorSetup:
			// UNIMPLEMENTED
			Sound_StandardAlert();
			break;
		
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
		
		case kCommandMinimizeWindow:
			// behave as if the user clicked the collapse box of the frontmost window
			{
				WindowRef		frontWindow = EventLoop_ReturnRealFrontWindow();
				Boolean			collapsing = false;
				
				
				if (nullptr != frontWindow)
				{
					WindowRef		sheetParentWindow = nullptr;
					
					
					// is the frontmost window a sheet?
					if (GetSheetWindowParent(frontWindow, &sheetParentWindow) == noErr)
					{
						// the front window is a sheet; minimize its parent window instead
						frontWindow = sheetParentWindow;
					}
					collapsing = !IsWindowCollapsed(frontWindow);
					CollapseWindow(frontWindow, collapsing);
				}
			}
			break;
		
		case kCommandZoomWindow:
		case kCommandMaximizeWindow:
			// do not zoom windows this way, pass events to specific windows
			// (for example, EventLoop_HandleZoomEvent())
			result = false;
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
			SessionFactory_ForEveryTerminalWindowDo(showWindowTerminalWindowOp, 0L/* data 1 - unused */,
													0L/* data 2 - unused */, nullptr/* pointer to result - unused */);
			break;
		
		//case kCommandKioskModeDisable:
		//	see TerminalWindow.mm
		//	break;
		
		case kCommandStackWindows:
			{
				// on Mac OS X, this command also requires that all application windows come to the front
				ProcessSerialNumber		psn;
				
				
				if (GetCurrentProcess(&psn) == noErr)
				{
					(OSStatus)SetFrontProcess(&psn);
				}
				
				// arrange windows in a diagonal pattern
				TerminalWindow_StackWindows();
			}
			break;
		
		case kCommandNextWindow:
			// activate the next window in the window list (terminal windows only)
			activateAnotherWindow(false/* previous */, false/* hide current */);
			break;
		
		case kCommandNextWindowHideCurrent:
			// activate the next window in the window list (terminal windows only),
			// but first obscure the frontmost terminal window
			activateAnotherWindow(false/* previous */, true/* hide current */);
			break;
		
		case kCommandPreviousWindow:
			// activate the previous window in the window list (terminal windows only)
			activateAnotherWindow(true/* previous */, false/* hide current */);
			break;
		
		case kCommandPreviousWindowHideCurrent:
			// activate the previous window in the window list (terminal windows only),
			// but first obscure the frontmost terminal window
			activateAnotherWindow(true/* previous */, true/* hide current */);
			break;
		
		//case kCommandShowConnectionStatus:
		//case kCommandHideConnectionStatus:
		//	see InfoWindow.cp
		//	break;
		
		case kCommandShowCommandLine:
			// in the Cocoa implementation this really means “show or activate”
			CommandLine_Display();
			break;
		
		//kCommandShowHidePrefCollectionsDrawer:
		//kCommandDisplayPrefPanelFormats:
		//kCommandDisplayPrefPanelGeneral:
		//kCommandDisplayPrefPanelKiosk:
		//kCommandDisplayPrefPanelMacros:
		//kCommandDisplayPrefPanelSessions:
		//kCommandDisplayPrefPanelTerminals:
		//	see PrefsWindow.cp
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
		
		case kCommandShowHelpTags:
			HMSetHelpTagsDisplayed(true);
			break;
		
		case kCommandHideHelpTags:
			HMSetHelpTagsDisplayed(false);
			break;
		
		case kCommandDisplayWindowContextualMenu: // GOING AWAY
			// display contextual menu for main focus area
			if (frontTerminalWindow != nullptr)
			{
				WindowRef		window = TerminalWindow_ReturnWindow(frontTerminalWindow);
				EventRecord		fakeEvent;
				
				
				fakeEvent.what = mouseDown;
				fakeEvent.message = 0;
				fakeEvent.when = 0;
				fakeEvent.modifiers = EventLoop_ReturnCurrentModifiers();
				GetGlobalMouse(&fakeEvent.where);
				(OSStatus)ContextualMenuBuilder_DisplayMenuForWindow(window, &fakeEvent, inContent);
			}
			break;
		
	#if 0
		case kCommandToggleMacrosMenuVisibility:
			{
				Preferences_Result		preferencesResult = kPreferences_ResultOK;
				size_t					actualSize = 0;
				Boolean					isVisible = false;
				
				
				preferencesResult = Preferences_GetData(kPreferences_TagMacrosMenuVisible,
														sizeof(isVisible), &isVisible,
														&actualSize);
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
	EventRef	executeEvent = nullptr;
	OSStatus	error = noErr;
	Boolean		result = false;
	
	
	error = CreateEvent(kCFAllocatorDefault, kEventClassCommand, kEventCommandProcess,
						0/* time of event */, kEventAttributeNone, &executeEvent);
	if (noErr == error)
	{
		HICommand		commandInfo;
		EventTargetRef	whereToStart = inTarget;
		
		
		if (nullptr == whereToStart) whereToStart = GetUserFocusEventTarget();
		if (nullptr == whereToStart) whereToStart = GetWindowEventTarget(EventLoop_ReturnRealFrontWindow());
		if (nullptr == whereToStart) whereToStart = GetApplicationEventTarget();
		assert(nullptr != whereToStart);
		
		bzero(&commandInfo, sizeof(commandInfo));
		commandInfo.commandID = inCommandID;
		error = SetEventParameter(executeEvent, kEventParamDirectObject, typeHICommand,
									sizeof(commandInfo), &commandInfo);
		if (noErr == error)
		{
			error = SendEventToEventTarget(executeEvent, whereToStart);
			if (noErr == error)
			{
				// success!
				result = true;
			}
		}
	}
	
	return result;
}// ExecuteByIDUsingEvent


/*!
Calls Commands_ExecuteByIDUsingEvent() after the specified
period of time has elapsed.

This is not recommended except in unusual situations.

(4.0)
*/
void
Commands_ExecuteByIDUsingEventAfterDelay	(UInt32				inCommandID,
											 EventTargetRef		inTarget,
											 Float32			inDelayInSeconds)
{
	Commands_DelayedCommand*	delayedExecution = [[[Commands_DelayedCommand alloc]
														initWithCommand:inCommandID
																		andEventTarget:inTarget
																		andDelay:inDelayInSeconds] autorelease];
	
	
	[delayedExecution execute];
}// ExecuteByIDUsingEventAfterDelay


/*!
Handles "kEventToolbarCreateItemWithIdentifier" from
"kEventClassToolbar" for any toolbar.  Responds by
creating the requested items.

This routine has special knowledge of certain commands
that have associated icons.  It creates those items
and automatically associates the proper command ID
with them so that the normal command handler will take
care of any clicks on them.

Currently, the context is unused and should be nullptr.

(3.1)
*/
OSStatus
Commands_HandleCreateToolbarItem	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
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
			case kEventToolbarCreateItemWithIdentifier:
				{
					CFStringRef		identifierCFString = nullptr;
					
					
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbarItemIdentifier,
																	typeCFStringRef, identifierCFString);
					if (noErr == result)
					{
						CFTypeRef	itemData = nullptr;
						
						
						// NOTE: configuration data is not always present
						result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamToolbarItemConfigData,
																		typeCFTypeRef, itemData);
						if (noErr != result)
						{
							itemData = nullptr;
							result = noErr;
						}
						
						// create the specified item, if its identifier is recognized
						{
							HIToolbarItemRef	itemRef = nullptr;
							
							
							if (kCFCompareEqualTo == CFStringCompare
														(kConstantsRegistry_HIToolbarItemIDNewShellSession,
															identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandNewSessionShell;
									IconRef			iconRef = nullptr;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = GetIconRef(kOnSystemDisk, kSystemIconsCreator,
														kInternetLocationGenericIcon, &iconRef);
									if (noErr == result)
									{
										result = HIToolbarItemSetIconRef(itemRef, iconRef);
										assert_noerr(result);
										(OSStatus)ReleaseIconRef(iconRef), iconRef = nullptr;
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare
															(kConstantsRegistry_HIToolbarItemIDNewDefaultSession,
																identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandNewSessionDefaultFavorite;
									CFStringRef		nameCFString = nullptr;
									FSRef			iconFile;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									if (AppResources_GetArbitraryResourceFileFSRef
										(AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
											CFSTR("icns")/* type */, iconFile))
									{
										IconRef		iconRef = nullptr;
										
										
										if (noErr == RegisterIconRefFromFSRef
														(AppResources_ReturnCreatorCode(),
															kConstantsRegistry_IconServicesIconPrefPanelSessions,
															&iconFile, &iconRef))
										{
											result = HIToolbarItemSetIconRef(itemRef, iconRef);
											assert_noerr(result);
											(OSStatus)ReleaseIconRef(iconRef), iconRef = nullptr;
										}
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare
															(kConstantsRegistry_HIToolbarItemIDNewLoginSession,
																identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandNewSessionLoginShell;
									IconRef			baseIconRef = nullptr;
									CFStringRef		nameCFString = nullptr;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									result = GetIconRef(kOnSystemDisk, kSystemIconsCreator,
														kInternetLocationGenericIcon, &baseIconRef);
									if (noErr == result)
									{
										IconRef		badgeIconRef = nullptr;
										
										
										if (noErr == GetIconRef(kOnSystemDisk, kSystemIconsCreator,
																kLockedBadgeIcon, &badgeIconRef))
										{
											IconRef		finalIconRef = nullptr;
											
											
											if (noErr == CompositeIconRef(baseIconRef, badgeIconRef, &finalIconRef))
											{
												result = HIToolbarItemSetIconRef(itemRef, finalIconRef);
												assert_noerr(result);
												(OSStatus)ReleaseIconRef(finalIconRef), finalIconRef = nullptr;
											}
											(OSStatus)ReleaseIconRef(badgeIconRef), badgeIconRef = nullptr;
										}
										(OSStatus)ReleaseIconRef(baseIconRef), baseIconRef = nullptr;
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							else if (kCFCompareEqualTo == CFStringCompare
															(kConstantsRegistry_HIToolbarItemIDStackWindows,
																identifierCFString, kCFCompareBackwards))
							{
								result = HIToolbarItemCreate(identifierCFString,
																kHIToolbarItemNoAttributes, &itemRef);
								if (noErr == result)
								{
									UInt32 const	kMyCommandID = kCommandStackWindows;
									CFStringRef		nameCFString = nullptr;
									FSRef			iconFile;
									
									
									if (Commands_CopyCommandName(kMyCommandID, kCommands_NameTypeShort, nameCFString))
									{
										result = HIToolbarItemSetLabel(itemRef, nameCFString);
										assert_noerr(result);
										result = HIToolbarItemSetHelpText(itemRef, nameCFString/* short text */,
																			nullptr/* long text */);
										assert_noerr(result);
										CFRelease(nameCFString), nameCFString = nullptr;
									}
									if (AppResources_GetArbitraryResourceFileFSRef
										(AppResources_ReturnStackWindowsIconFilenameNoExtension(),
											CFSTR("icns")/* type */, iconFile))
									{
										IconRef		iconRef = nullptr;
										
										
										if (noErr == RegisterIconRefFromFSRef
														(AppResources_ReturnCreatorCode(),
															kConstantsRegistry_IconServicesIconToolbarItemStackWindows,
															&iconFile, &iconRef))
										{
											result = HIToolbarItemSetIconRef(itemRef, iconRef);
											assert_noerr(result);
											(OSStatus)ReleaseIconRef(iconRef), iconRef = nullptr;
										}
									}
									result = HIToolbarItemSetCommandID(itemRef, kMyCommandID);
									assert_noerr(result);
								}
							}
							
							if (nullptr == itemRef)
							{
								result = eventNotHandledErr;
							}
							else
							{
								result = SetEventParameter(inEvent, kEventParamToolbarItem, typeHIToolbarItemRef,
															sizeof(itemRef), &itemRef);
							}
						}
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
}// HandleCreateToolbarItem


/*!
Adds to the specified menu a list of the names of valid
contexts in the specified preferences class.

If there is a problem adding anything to the menu, a
non-success code may be returned, although the menu may
still be partially changed.

NOTE:	This is like Preferences_InsertContextNamesInMenu(),
		but is Cocoa-specific.  It is only here, for now,
		because this module is Objective-C, and the
		Preferences module is not yet Objective-C.

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
Returns true only if the specified command ID is currently
available to the user, taking into account the active window
and any other relevant context.

WARNING:	This currently only works for command IDs that
		are used for contextual menu items.

DEPRECATED.  This is here for transitional purposes, as it
is useful in some cases (particularly contextual menus) to
refer to items by ID.  However, menu items from now on will
be primarily checked using Objective-C and Cocoa
selectors.

(4.0)
*/
Boolean
Commands_IsCommandEnabled	(UInt32		inCommandID)
{
	AutoPool				_;
	Commands_Executor*		commandExecutor = (Commands_Executor*)[Commands_Executor sharedExecutor];
	Boolean					result = false;
	
	
	assert(nil != commandExecutor);
	
	result = ([commandExecutor isCommandEnabled:inCommandID]) ? true : false;
	
	return result;
}// IsCommandEnabled


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
Chooses another window in the window list that is adjacent
to the frontmost window.  If "inPreviousInsteadOfNext" is
true, then the previous window is activated instead of the
next one in the list.  If "inHidePreviousWindow" is true
and the frontmost window is a terminal, it is hidden (with
the appropriate visual effects) before the next window is
activated.

(3.0)
*/
void
activateAnotherWindow	(Boolean	inPreviousInsteadOfNext,
						 Boolean	inHidePreviousWindow)
{
	HIWindowRef		frontWindow = EventLoop_ReturnRealFrontWindow();
	
	
	if (nullptr != frontWindow)
	{
		HIWindowRef		nextWindow = nullptr;
		SessionRef		frontSession = SessionFactory_ReturnUserFocusSession();
		
		
		if (nullptr == frontSession)
		{
			// not a session window; so, select the first or last session window
			SessionFactory_Result	factoryError = SessionFactory_GetWindowWithZeroBasedIndex
													((inPreviousInsteadOfNext) ? SessionFactory_ReturnCount() - 1 : 0,
														kSessionFactory_ListInCreationOrder, &nextWindow);
			
			
			if (kSessionFactory_ResultOK != factoryError)
			{
				Console_Warning(Console_WriteLine, "failed to find window with given index");
			}
		}
		else
		{
			// is a session window; so, select the next or previous session window
			UInt16					frontSessionIndex = 0;
			SessionFactory_Result	factoryError = kSessionFactory_ResultOK;
			
			
			factoryError = SessionFactory_GetZeroBasedIndexOfSession
							(frontSession, kSessionFactory_ListInTabStackOrder, &frontSessionIndex);
			if (kSessionFactory_ResultOK == factoryError)
			{
				UInt16		loopGuard = 0;
				
				
				// skip certain windows in the rotation (see below)
				while ((nullptr == nextWindow) && (loopGuard < 50/* arbitrary */))
				{
					// adjust the session index to determine the new session;
					// if modified past the bounds, wrap around
					if (inPreviousInsteadOfNext)
					{
						if (frontSessionIndex == 0) frontSessionIndex = SessionFactory_ReturnCount() - 1;
						else --frontSessionIndex;
					}
					else
					{
						if (++frontSessionIndex >= SessionFactory_ReturnCount()) frontSessionIndex = 0;
					}
					
					factoryError = SessionFactory_GetWindowWithZeroBasedIndex
									(frontSessionIndex, kSessionFactory_ListInTabStackOrder, &nextWindow);
					if (kSessionFactory_ResultOK != factoryError)
					{
						Console_Warning(Console_WriteLine, "failed to find window with given index");
					}
					
					// if possible, detect and avoid windows that are on different Spaces
					if (nullptr != nextWindow)
					{
						NSWindow*	cocoaWindow = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(nextWindow);
						
						
						// note: "(BOOL)isOnActiveSpace" is only available in Mac OS X 10.6
						if ((nil != cocoaWindow) && ([cocoaWindow respondsToSelector:@selector(isOnActiveSpace)]))
						{
							if (NO == [cocoaWindow isOnActiveSpace])
							{
								// skip this window
								nextWindow = nullptr;
							}
						}
					}
					
					++loopGuard;
				}
			}
		}
		
		if (nullptr == nextWindow)
		{
			// cannot figure out which window to activate
			Sound_StandardAlert();
		}
		else
		{
			unless (IsWindowVisible(nextWindow))
			{
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(nextWindow);
				
				
				// display the window and ensure it is not marked as hidden (in case it was)
				TerminalWindow_SetObscured(terminalWindow, false);
			}
			EventLoop_SelectBehindDialogWindows(nextWindow);
			if (inHidePreviousWindow)
			{
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(frontWindow);
				
				
				if (nullptr != terminalWindow) TerminalWindow_SetObscured(terminalWindow, true);
			}
		}
	}
}// activateAnotherWindow


/*!
Returns true only if the currently-focused view in the
currently-focused Carbon window has selected text.  This
can be used to help validators enable editing commands.

DEPRECATED.  Used only for Carbon transition.

(4.0)
*/
BOOL
activeCarbonWindowHasSelectedText ()
{
	BOOL			result = NO;
	HIViewRef		focusedView = returnActiveCarbonWindowFocusedField();
		
		
	if (nullptr != focusedView)
	{
		ControlEditTextSelectionRec		selection;
		Size							actualSize = 0;
		
		
		if (noErr == GetControlData(focusedView, kControlEntireControl, kControlEditTextSelectionTag,
									sizeof(selection), &selection, &actualSize))
		{
			result = (selection.selStart != selection.selEnd);
		}
	}
	return result;
}// activeCarbonWindowHasSelectedText


/*!
Adds a session’s name to the Window menu, arranging so
that future selections of the new menu item will cause
the window to be selected, and so that the item’s state
is synchronized with that of the session and its window.

Returns true only if an item was added.

(4.0)
*/
BOOL
addWindowMenuItemForSession		(SessionRef							inSession,
								 My_MenuItemInsertionInfoConstPtr	inMenuInfoPtr,
								 CFStringRef						inWindowName)
{
	BOOL			result = NO;
	NSMenuItem*		newItem = [inMenuInfoPtr->menu insertItemWithTitle:(NSString*)inWindowName
																		action:nil keyEquivalent:@""
																		atIndex:inMenuInfoPtr->atItemIndex];
	
	
	if (nil != newItem)
	{
		// define an attributed title so that it is possible to italicize the text for hidden windows
		NSAttributedString*			immutableText = [[[NSAttributedString alloc] initWithString:[newItem title]]
														autorelease];
		NSMutableAttributedString*	mutableText = [[immutableText mutableCopyWithZone:NULL] autorelease];
		
		
		// unfortunately, attributed strings have no properties whatsoever, so even normal text
		// requires explicitly setting the proper system font for menu items!
		[mutableText addAttribute:NSFontAttributeName value:[NSFont menuFontOfSize:[NSFont systemFontSize]]
														range:NSMakeRange(0, [mutableText length])];
		[newItem setAttributedTitle:mutableText];
		
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
This method, of standard "SessionFactory_SessionOpProcPtr"
form, adds the specified session to the Window menu.

(4.0)
*/
void
addWindowMenuItemSessionOp	(SessionRef		inSession,
							 void*			inWindowMenuInfoPtr,
							 SInt32			UNUSED_ARGUMENT(inData2),
							 void*			inoutResultPtr)
{
	CFStringRef							nameCFString = nullptr;
	My_MenuItemInsertionInfoConstPtr	menuInfoPtr = REINTERPRET_CAST
														(inWindowMenuInfoPtr, My_MenuItemInsertionInfoConstPtr);
	int*								numberOfItemsAddedPtr = REINTERPRET_CAST(inoutResultPtr, int*);
	
	
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
	
	if (addWindowMenuItemForSession(inSession, menuInfoPtr, nameCFString))
	{
		++(*numberOfItemsAddedPtr);
	}
}// addWindowMenuItemSessionOp


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
Handles a quit by reviewing any open sessions (if "inAsk" is
YES).  Returns YES only if the application should quit.

(4.0)
*/
BOOL
handleQuit	(BOOL	inAsk)
{
	BOOL		result = NO;
	SInt16		itemHit = kAlertStdAlertOKButton; // if only 1 session is open, the user “Reviews” it
	SInt32		sessionCount = SessionFactory_ReturnCount() -
								SessionFactory_ReturnStateCount(kSession_StateActiveUnstable); // ignore recently-opened windows
	
	
	// if there are any unsaved sessions, confirm quitting (nice if the user accidentally hit command-Q);
	// note that currently, because the Console is considered a session, the following statement tests
	// for MORE than 1 open session before warning the user (there has to be an elegant way to do this...)
	if ((inAsk) && (sessionCount > 1))
	{
		InterfaceLibAlertRef	box = nullptr;
		
		
		box = Alert_New();
		Alert_SetHelpButton(box, false);
		Alert_SetParamsFor(box, kAlert_StyleDontSaveCancelSave);
		Alert_SetType(box, kAlertCautionAlert);
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
					Alert_SetTextCFStrings(box, primaryTextCFString, helpTextCFString);
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
				Alert_SetTitleCFString(box, titleCFString);
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
				Alert_SetButtonText(box, kAlertStdAlertOKButton, buttonCFString);
				CFRelease(buttonCFString);
			}
		}
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			buttonCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_ButtonDiscardAll, buttonCFString);
			if (stringResult.ok())
			{
				Alert_SetButtonText(box, kAlertStdAlertOtherButton, buttonCFString);
				CFRelease(buttonCFString);
			}
		}
		Alert_Display(box);
		itemHit = Alert_ItemHit(box);
		Alert_Dispose(&box);
	}
	
	if (itemHit == kAlertStdAlertOKButton)
	{
		// “Review…”; so, display alerts for each open session, individually, and
		// quit after all alerts are closed unless the user cancels one of them.
		// A fairly simple way to handle this is to activate each window in turn,
		// and then display a modal alert asking about the frontmost window.  This
		// is not quite as elegant as using sheets, but since it is synchronous it
		// is WAY easier to write code for!  To help the user better understand
		// which window is being referred to, each window is moved to the center
		// of the main screen using a slide animation prior to popping open an
		// alert.  In addition, all background windows become translucent.
		Boolean		cancelQuit = false;
		
		
		result = YES;
		
		// a cool effect, but probably too CPU intensive for most...
		//SessionFactory_ForEveryTerminalWindowDo(setTerminalWindowTranslucency, nullptr/* data 1 */,
		//										1/* 0 = opaque, 1 = translucent */, nullptr/* result */);
		
		// prevent tabs from shifting during this process
		(SessionFactory_Result)SessionFactory_SetAutoRearrangeTabsEnabled(false);
		
		// iterate over each session in a MODAL fashion, highlighting a window
		// and either displaying an alert or discarding the window if it has only
		// been open a short time
		SessionFactory_ForEachSessionDo(kSessionFactory_SessionFilterFlagAllSessions,
										moveWindowAndDisplayTerminationAlertSessionOp,
										0L/* data 1 */, 0L/* data 2 */, &cancelQuit/* result */,
										true/* is a final iteration: use a copy of the list? */);
		
		// prevent tabs from shifting during this process
		(SessionFactory_Result)SessionFactory_SetAutoRearrangeTabsEnabled(true);
		
		// make sure all windows become opaque again
		//SessionFactory_ForEveryTerminalWindowDo(setTerminalWindowTranslucency, nullptr/* data 1 */,
		//										0/* 0 = opaque, 1 = translucent */, nullptr/* result */);
		if (cancelQuit)
		{
			result = NO;
		}
	}
	else if (itemHit == kAlertStdAlertOtherButton)
	{
		// “Discard All”; so, no alerts are displayed, but the application still quits
		result = YES;
	}
	else
	{
		// “Cancel” or Help
		result = NO;
	}
	
	return result;
}// handleQuit


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
	NSArray*		items = [inMenu itemArray];
	NSEnumerator*	toMenuItem = [items objectEnumerator];
	int				result = -1;
	
	
	while (NSMenuItem* item = [toMenuItem nextObject])
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
Returns YES only if the specified object is apparently a
Carbon window.

This is a transitional routine only.  It relies on the
existence of the undocumented internal NSCarbonWindow
class, which just so happens to be used consistently on
all versions of Mac OS X so far.

This also calls CocoaBasic_RegisterCocoaCarbonWindow() so
that the window is automatically made known to the registry;
that way, any future attempts to “allocate” a Cocoa window
for that Carbon window, will instead return the known window.

(4.0)
*/
BOOL
isCarbonWindow	(id		inObject)
{
	BOOL	result = NO;
	
	
	if (nil != inObject)
	{
		result = [[inObject class] isSubclassOfClass:[NSCarbonWindow class]];
		if (result && ([[inObject class] isSubclassOfClass:[NSWindow class]]))
		{
			(Boolean)CocoaBasic_RegisterCocoaCarbonWindow((NSWindow*)inObject);
		}
	}
	return result;
}// isCarbonWindow


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
						[[inWindow windowController] isKindOfClass:[VectorWindow_Controller class]]);
	
	
	return result;
}// isCocoaWindowMoreImportantThanCarbon


/*!
Of "SessionFactory_SessionOpProcPtr" form, this routine
slides the specified session’s window(s) into the center
of the screen and then displays a MODAL alert asking the
user if the session should close.  The "inoutResultPtr"
should be a Boolean pointer that, on output, is false
only if the user cancels the close.

(4.0)
*/
void
moveWindowAndDisplayTerminationAlertSessionOp	(SessionRef		inSession,
												 void*			UNUSED_ARGUMENT(inData1),
												 SInt32			UNUSED_ARGUMENT(inData2),
												 void*			inoutResultPtr)
{
	unless (gCurrentQuitCancelled)
	{
		HIWindowRef		window = Session_ReturnActiveWindow(inSession);
		Boolean*		outFlagPtr = REINTERPRET_CAST(inoutResultPtr, Boolean*);
		
		
		if (nullptr != window)
		{
			// if the window was obscured, show it first
			if (false == IsWindowVisible(window))
			{
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(window);
				
				
				TerminalWindow_SetObscured(terminalWindow, false);
			}
			
			// all windows became translucent; make sure the alert one is opaque
			(OSStatus)SetWindowAlpha(window, 1.0);
			
			Session_StartMonitoring(inSession, kSession_ChangeCloseWarningAnswered, gCurrentQuitWarningAnswerListener);
			Session_DisplayTerminationWarning(inSession, true/* force modal window */);
			// there is a chance that displaying the alert will destroy
			// the session, in which case the stop-monitoring call is
			// invalid (but only in that case); TEMPORARY, not sure how
			// to fix this, yet
			Session_StopMonitoring(inSession, kSession_ChangeCloseWarningAnswered, gCurrentQuitWarningAnswerListener);
			*outFlagPtr = gCurrentQuitCancelled;
		}
	}
}// moveWindowAndDisplayTerminationAlertSessionOp


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
	size_t						actualSize = 0L;
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagNewCommandShortcutEffect:
		// update internal variable that matches the value of the preference
		// (easier than calling Preferences_GetData() every time!)
		unless (Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
									sizeof(gNewCommandShortcutEffect), &gNewCommandShortcutEffect,
									&actualSize) == kPreferences_ResultOK)
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
	size_t		actualSize = 0L;
	BOOL		result = NO;
	
	
	// get the user’s “don’t auto-new” application preference, if possible
	if (kPreferences_ResultOK !=
		Preferences_GetData(kPreferences_TagDontAutoNewOnApplicationReopen, sizeof(quellAutoNew),
							&quellAutoNew, &actualSize))
	{
		// assume a value if it cannot be found
		quellAutoNew = false;
	}
	
	result = (true == quellAutoNew);
	
	return result;
}// quellAutoNew


/*!
Invoked when a session’s termination warning is
closed; 

(4.0)
*/
void
receiveTerminationWarningAnswer		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
									 ListenerModel_Event	inEventThatOccurred,
									 void*					inEventContextPtr,
									 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	assert(inEventThatOccurred == kSession_ChangeCloseWarningAnswered);
	
	{
		SessionCloseWarningButtonInfoPtr	infoPtr = REINTERPRET_CAST(inEventContextPtr,
																		SessionCloseWarningButtonInfoPtr);
		
		
		gCurrentQuitCancelled = (infoPtr->buttonHit == kAlertStdAlertCancelButton);
	}
}// receiveTerminationWarningAnswer


/*!
Returns the currently-focused view in the active Carbon window
if it supports text editing, otherwise "nullptr" is returned.

DEPRECATED.  Used only for Carbon transition.

(4.0)
*/
HIViewRef
returnActiveCarbonWindowFocusedField ()
{
	HIViewRef		result = nullptr;
	HIWindowRef		focusedWindow = GetUserFocusWindow();
	
	
	if (nullptr != focusedWindow)
	{
		HIViewRef		focusedView = nullptr;
		
		
		if (noErr == GetKeyboardFocus(focusedWindow, &focusedView))
		{
			ControlKind		viewKind;
			
			
			if (noErr == GetControlKind(focusedView, &viewKind))
			{
				if ((kControlKindSignatureApple == viewKind.signature) &&
					(kControlKindEditUnicodeText == viewKind.kind))
				{
					result = focusedView;
				}
			}
		}
	}
	return result;
}// returnActiveCarbonWindowFocusedField


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
	int		result = [inWindowMenu indexOfItemWithTag:kMenuItemIDPrecedingWindowList];
	assert(result >= 0); // make sure the tag is set correctly in the NIB
	
	
	// skip the dividing line item, then point to the next item (so +2)
	result += 2;
	
	return result;
}// returnFirstWindowItemAnchor


/*!
Returns the menu from the menu bar that corresponds to
the given "kMenuID..." constant.

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
	NSMenu*			windowMenu = returnMenu(kMenuIDWindow);
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
				setUpWindowMenu(returnMenu(kMenuIDWindow));
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
					Rect	windowMenuTitleBounds;
					
					
					if (MenuBar_GetMenuTitleRectangle(kMenuBar_MenuWindow, &windowMenuTitleBounds))
					{
						CGRect		asInvertedCGRect = CGRectMake
														(windowMenuTitleBounds.left,
															NSHeight([windowScreen frame]) - windowMenuTitleBounds.bottom,
															windowMenuTitleBounds.right - windowMenuTitleBounds.left,
															windowMenuTitleBounds.bottom - windowMenuTitleBounds.top);
						Boolean		noAnimations = false;
						size_t		actualSize = 0;
						
						
						// determine if animation should occur
						unless (kPreferences_ResultOK ==
								Preferences_GetData(kPreferences_TagNoAnimations,
													sizeof(noAnimations), &noAnimations, &actualSize))
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
							CocoaAnimation_TransitionWindowForHide(hiddenWindow, asInvertedCGRect);
						}
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
				NSMenuItem*		item = returnWindowMenuItemForSession(session);
				
				
				[item setTitle:(NSString*)text];
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
	NSMenu*			targetMenu = returnMenu(kMenuIDFile);
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
	{
		NSEnumerator*	toMenuItem = [items objectEnumerator];
		
		
		while (NSMenuItem* item = [toMenuItem nextObject])
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
A "SessionFactory_TerminalWindowOpProcPtr" that makes a
terminal window partially transparent or fully opaque
depending on the value of "inData2".  The other arguments
are unused and reserved.

(4.0)
*/
void
setTerminalWindowTranslucency	(TerminalWindowRef		inTerminalWindow,
								 void*					UNUSED_ARGUMENT(inData1),
								 SInt32					inZeroIfOpaqueOneForPartialTransparency,
								 void*					UNUSED_ARGUMENT(inoutResultPtr))
{
	if (0 == inZeroIfOpaqueOneForPartialTransparency)
	{
		(OSStatus)SetWindowAlpha(TerminalWindow_ReturnWindow(inTerminalWindow), 1.0);
	}
	else
	{
		(OSStatus)SetWindowAlpha(TerminalWindow_ReturnWindow(inTerminalWindow), 0.65/* arbitrary */);
	}
}// setTerminalWindowTranslucency


/*!
A short-cut for invoking setUpFormatFavoritesMenu() and
other setUp*Menu() routines.  In general, you should use
this routine instead of directly calling any of those.

(4.0)
*/
void
setUpDynamicMenus ()
{
	setUpSessionFavoritesMenu(returnMenu(kMenuIDFile));
	setUpWorkspaceFavoritesMenu(returnMenu(kMenuIDFile));
	setUpFormatFavoritesMenu(returnMenu(kMenuIDView));
	setUpMacroSetsMenu(returnMenu(kMenuIDMacros));
	setUpTranslationTablesMenu(returnMenu(kMenuIDKeys));
	setUpWindowMenu(returnMenu(kMenuIDWindow));
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
	// on older versions of Mac OS X, it seems that Cocoa stubbornly
	// dumps items into the Window menu automatically, which creates
	// redundancy; so, enable this feature only on newer OS versions
	if (FlagManager_Test(kFlagOS10_6API))
	{
		int const					kFirstWindowItemIndex = returnFirstWindowItemAnchor(inMenu);
		int const					kDividerIndex = kFirstWindowItemIndex - 1;
		int							deletedItemIndex = -1;
		int							numberOfWindowMenuItemsAdded = 0;
		My_MenuItemInsertionInfo	insertWhere;
		
		
		// set up callback data
		bzero(&insertWhere, sizeof(insertWhere));
		insertWhere.menu = inMenu;
		insertWhere.atItemIndex = kDividerIndex; // because, the divider is not present at insertion time
		
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
		SessionFactory_ForEachSessionDo(kSessionFactory_SessionFilterFlagAllSessions &
											~kSessionFactory_SessionFilterFlagConsoleSessions,
										addWindowMenuItemSessionOp,
										&insertWhere/* data 1: menu info */, 0L/* data 2: undefined */,
										&numberOfWindowMenuItemsAdded/* result: int*, number of items added */);
		
		// if any were added, include a dividing line (note also that this
		// item must be erased above)
		if (numberOfWindowMenuItemsAdded > 0)
		{
			[inMenu insertItem:[NSMenuItem separatorItem] atIndex:kDividerIndex];
		}
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
	IconRef				stateIconRef = nullptr;
	NSMenuItem*			item = (nil == inItemOrNil)
								? returnWindowMenuItemForSession(inSession)
								: inItemOrNil;
	
	
	sessionResult = Session_CopyStateIconRef(inSession, stateIconRef);
	if (false == sessionResult.ok())
	{
		Console_Warning(Console_WriteLine, "unable to copy session icon for menu item");
	}
	else
	{
		// set the icon for the menu; this might be easiest to do
		// if the name of the appropriate icon file is known
		// UNIMPLEMENTED
	}
	
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
This routine, of "SessionFactory_TerminalWindowOpProcPtr"
form, redisplays the specified, obscured terminal window.

(4.0)
*/
void
showWindowTerminalWindowOp		(TerminalWindowRef	inTerminalWindow,
								 void*				UNUSED_ARGUMENT(inData1),
								 SInt32				UNUSED_ARGUMENT(inData2),
								 void*				UNUSED_ARGUMENT(inoutResultPtr))
{
	TerminalWindow_SetObscured(inTerminalWindow, false);
}// showWindowTerminalWindowOp


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
Returns an object representing a true value, useful in many
simple validators.  The given item is not used.

(4.0)
*/
id
validatorYes	(id <NSValidatedUserInterfaceItem>		UNUSED_ARGUMENT(inItem))
{
	BOOL	result = YES;
	
	
	return [NSNumber numberWithBool:result];
}// validatorYes

} // anonymous namespace


@implementation Commands_DelayedCommand


/*!
Designated initializer.

(4.0)
*/
- (id)
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
	[self performSelector:@selector(executeWithoutDelay) withObject:nil afterDelay:self->executionDelay];
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


@end // Commands_DelayedCommand


@implementation Commands_Executor


Commands_Executor*		gCommands_Executor = nil;


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedExecutor
{
	if (nil == gCommands_Executor)
	{
		[[[self class] allocWithZone:NULL] init];
		assert(nil != gCommands_Executor);
	}
	return gCommands_Executor;
}


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super init];
	
	// this approach allows the singleton to be constructed from
	// anywhere, even an object in a NIB
	if (nil == gCommands_Executor)
	{
		gCommands_Executor = self;
	}
	
	return self;
}// init


// IMPORTANT: These methods are initially trivial, calling into the
// established Carbon command responder chain.  This is TEMPORARY.
// It will eventually make more sense for these methods to directly
// perform their actions.  It will also be more appropriate to use
// the Cocoa responder chain, and relocate the methods to more
// specific places (for example, NSWindowController subclasses).
// This initial design is an attempt to change as little as possible
// while performing a transition from Carbon to Cocoa.


@end // Commands_Executor


@implementation Commands_Executor (Commands_ApplicationCoreEvents)


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
	NSEnumerator*	toPath = [filenames objectEnumerator];
	BOOL			success = YES;
	
	
	while (NSString* path = [toPath nextObject])
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
												true/* is retained */); // LOCALIZE THIS?
			
			
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
		size_t		actualSize = 0;
		
		
		// assume that the user is mapping command-N to the same type of session
		// that would be appropriate for opening by default on startup or re-open
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
									sizeof(newCommandID), &newCommandID,
									&actualSize))
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
	
	// this callback changes the "gCurrentQuitCancelled" flag accordingly as session windows are visited
	gCurrentQuitWarningAnswerListener = ListenerModel_NewStandardListener(receiveTerminationWarningAnswer);
	
	// kill all open sessions (asking the user as appropriate), and if the
	// user never cancels, *flags* the main event loop to terminate cleanly
	if (NO == handleQuit(YES/* ask for user permission */))
	{
		result = NSTerminateCancel;
	}
	
	ListenerModel_ReleaseListener(&gCurrentQuitWarningAnswerListener);
	
	return result;
}// applicationShouldTerminate:


#pragma mark NSApplicationNotifications


/*!
Installs the handlers for Services.  The Info.plist of the main
bundle must also advertise the methods of each Service.

(4.0)
*/
- (void)
applicationDidFinishLaunching:(NSNotification*)	aNotification
{
	NSApplication*				application = (NSApplication*)[aNotification object];
	Commands_ServiceProviders*	providers = [[Commands_ServiceProviders alloc] init];
	
	
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
Notifies any Quills callback that the application has terminated.

(4.0)
*/
- (void)
applicationWillTerminate:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	Quills::Events::_handle_endloop();
}// applicationWillTerminate:


@end // Commands_Executor (Commands_ApplicationCoreEvents)


@implementation Commands_Executor (Commands_Capturing)


- (IBAction)
performCaptureBegin:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandCaptureToFile, nullptr/* target */);
}


- (IBAction)
performCaptureEnd:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandEndCaptureToFile, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performPrintScreen:(id)		sender
{
#pragma unused(sender)
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(performPrintScreen:) with:sender];
	}
	
	if (NO == implementedByCocoa)
	{
		Commands_ExecuteByIDUsingEvent(kCommandPrintScreen, nullptr/* target */);
	}
}
- (id)
canPerformPrintScreen:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = (PrintTerminal_IsPrintingSupported() && (nullptr != TerminalWindow_ReturnFromMainWindow()));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performPrintSelection:(id)	sender
{
#pragma unused(sender)
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(performPrintSelection:) with:sender];
	}
	
	if (NO == implementedByCocoa)
	{
		Commands_ExecuteByIDUsingEvent(kCommandPrint, nullptr/* target */);
	}
}
- (id)
canPerformPrintSelection:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = (PrintTerminal_IsPrintingSupported() && textSelectionExists());
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSaveSelection:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSaveText, nullptr/* target */);
}
- (id)
canPerformSaveSelection:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_Capturing)


@implementation Commands_Executor (Commands_Editing)


- (IBAction)
performCopy:(id)	sender
{
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(performCopy:) with:sender];
		if (NO == implementedByCocoa)
		{
			implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(copy:) with:sender];
		}
	}
	
	if (NO == implementedByCocoa)
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
	else if (isCarbonWindow(target))
	{
		if (activeCarbonWindowHasSelectedText())
		{
			result = YES;
		}
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
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performCopyAndPaste:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandCopyAndPaste, nullptr/* target */);
}
- (id)
canPerformCopyAndPaste:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performCopyWithTabSubstitution:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandCopyTable, nullptr/* target */);
}
- (id)
canPerformCopyWithTabSubstitution:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = textSelectionExists();
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performCut:(id)		sender
{
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(cut:) with:sender];
	}
	
	if (NO == implementedByCocoa)
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
	else if (isCarbonWindow(target))
	{
		result = activeCarbonWindowHasSelectedText();
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
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performDelete:(id)	sender
{
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(delete:) with:sender];
		if (NO == implementedByCocoa)
		{
			implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(clear:) with:sender];
		}
	}
	
	if (NO == implementedByCocoa)
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
	else if (isCarbonWindow(target))
	{
		result = activeCarbonWindowHasSelectedText();
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
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performPaste:(id)	sender
{
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(paste:) with:sender];
	}
	
	if (NO == implementedByCocoa)
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
		result = Clipboard_ContainsText() &&
					((nullptr != TerminalWindow_ReturnFromKeyWindow()) ||
						(nullptr != returnActiveCarbonWindowFocusedField()));
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performRedo:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandRedo, nullptr/* target */);
}
- (id)
canPerformRedo:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	CFStringRef		redoCommandName = nullptr;
	Boolean			isEnabled = false;
	BOOL			result = NO;
	
	
	Undoables_GetRedoCommandInfo(redoCommandName, &isEnabled);
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
		
		
		[asMenuItem setTitle:(NSString*)redoCommandName];
		result = (isEnabled) ? YES : NO;
	}
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSelectAll:(id)	sender
{
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(selectAll:) with:sender];
	}
	
	if (NO == implementedByCocoa)
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
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSelectEntireScrollbackBuffer:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSelectAllWithScrollback, nullptr/* target */);
}


- (IBAction)
performSelectNothing:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSelectNothing, nullptr/* target */);
}


- (IBAction)
performUndo:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandUndo, nullptr/* target */);
}
- (id)
canPerformUndo:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	CFStringRef		undoCommandName = nullptr;
	Boolean			isEnabled = false;
	BOOL			result = NO;
	
	
	Undoables_GetUndoCommandInfo(undoCommandName, &isEnabled);
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
		
		
		[asMenuItem setTitle:(NSString*)undoCommandName];
		result = (isEnabled) ? YES : NO;
	}
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_Editing)


@implementation Commands_Executor (Commands_OpeningSessions)


- (IBAction)
performDuplicate:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewDuplicateSession, nullptr/* target */);
}
- (id)
canPerformDuplicate:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performNewByFavoriteName:(id)	sender
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
														true/* is retained */);
			
			
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
- (id)
canPerformNewByFavoriteName:(id <NSValidatedUserInterfaceItem>)		anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performNewCustom:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionDialog, nullptr/* target */);
}
- (id)
canPerformNewCustom:(id <NSValidatedUserInterfaceItem>)		anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performNewDefault:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionDefaultFavorite, nullptr/* target */);
}
- (id)
canPerformNewDefault:(id <NSValidatedUserInterfaceItem>)	anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performNewLogInShell:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionLoginShell, nullptr/* target */);
}
- (id)
canPerformNewLogInShell:(id <NSValidatedUserInterfaceItem>)		anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performNewShell:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNewSessionShell, nullptr/* target */);
}
- (id)
canPerformNewShell:(id <NSValidatedUserInterfaceItem>)		anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performKill:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandKillProcessesKeepWindow, nullptr/* target */);
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
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performRestart:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandRestartSession, nullptr/* target */);
}
- (id)
canPerformRestart:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	SessionRef		currentSession = SessionFactory_ReturnUserRecentSession();
	BOOL			result = NO;
	
	
	if (nullptr != currentSession)
	{
		result = YES;
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performRestoreWorkspaceDefault:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandRestoreWorkspaceDefaultFavorite, nullptr/* target */);
}
- (id)
canPerformRestoreWorkspaceDefault:(id <NSValidatedUserInterfaceItem>)	anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performRestoreWorkspaceByFavoriteName:(id)	sender
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
														true/* is retained */);
			
			
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
- (id)
canPerformRestoreWorkspaceByFavoriteName:(id <NSValidatedUserInterfaceItem>)		anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performOpen:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandOpenSession, nullptr/* target */);
}
- (id)
canPerformOpen:(id <NSValidatedUserInterfaceItem>)		anItem
{
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return validatorYes(anItem);
}


- (IBAction)
performSaveAs:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSaveSession, nullptr/* target */);
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
#pragma unused(replyEvent)
	AEDesc		reply;
	// TEMPORARY: this uses older functions and has not yet been ported to Cocoa
	OSErr		error = URLAccessAE_HandleUniformResourceLocator([receivedEvent aeDesc], &reply, 0/* data */);
	
	
	if (noErr != error)
	{
		Console_Warning(Console_WriteValue, "URL open failed, error", error);
	}
	// reply is currently ignored - TEMPORARY
}


@end // Commands_Executor (Commands_OpeningSessions)


@implementation Commands_Executor (Commands_OpeningVectorGraphics)


- (IBAction)
performNewTEKPage:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTEKPageCommand, nullptr/* target */);
}
- (id)
canPerformNewTEKPage:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	SessionRef		currentSession = returnTEKSession();
	BOOL			result = (nullptr != currentSession);
	
	
	if (FlagManager_Test(kFlagKioskMode))
	{
		return [NSNumber numberWithBool:NO];
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performPageClearToggle:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTEKPageClearsScreen, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_OpeningVectorGraphics)


@implementation Commands_Executor (Commands_OpeningWebPages)


- (IBAction)
performCheckForUpdates:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandCheckForUpdates, nullptr/* target */);
}
- (id)
canPerformCheckForUpdates:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
performGoToMainWebSite:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandURLHomePage, nullptr/* target */);
}
- (id)
canPerformGoToMainWebSite:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
performOpenURL:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandHandleURL, nullptr/* target */);
}
- (id)
canPerformOpenURL:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) && textSelectionExists());
	
	
	if (result)
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		CFRetainRelease		selectedText(TerminalView_ReturnSelectedTextAsNewUnicode
											(view, 0/* Copy with Tab Substitution info */,
												kTerminalView_TextFlagInline));
		
		
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performProvideFeedback:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandURLAuthorMail, nullptr/* target */);
}
- (id)
canPerformProvideFeedback:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


@end // Commands_Executor (Commands_OpeningWebPages)


@implementation Commands_Executor (Commands_ManagingMacros)


- (IBAction)
performActionForMacro:(id)		sender
{
	NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
	int				oneBasedMacroNumber = [asMenuItem tag];
	
	
	MacroManager_UserInputMacro(oneBasedMacroNumber - 1/* zero-based macro number */);
}
- (id)
canPerformActionForMacro:(id <NSValidatedUserInterfaceItem>)	anItem
{
	Preferences_ContextRef	currentMacros = MacroManager_ReturnCurrentMacros();
	NSMenuItem*				asMenuItem = (NSMenuItem*)anItem;
	int						macroIndex = [asMenuItem tag];
	BOOL					result = ((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
										(nullptr != currentMacros));
	
	
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
		MacroManager_UpdateMenuItem(asMenuItem, macroIndex/* one-based */);
	}
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMacroSwitchByFavoriteName:(id)	sender
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
														true/* is retained */);
			
			
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
- (id)
canPerformMacroSwitchByFavoriteName:(id <NSValidatedUserInterfaceItem>)		anItem
{
	BOOL			result = (nullptr != TerminalWindow_ReturnFromMainWindow());
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMacroSwitchDefault:(id)	sender
{
#pragma unused(sender)
	MacroManager_Result		macrosResult = kMacroManager_ResultOK;
	
	
	macrosResult = MacroManager_SetCurrentMacros(MacroManager_ReturnDefaultMacros());
	if (false == macrosResult.ok())
	{
		Sound_StandardAlert();
	}
}
- (id)
canPerformMacroSwitchDefault:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	result = (nullptr != TerminalWindow_ReturnFromMainWindow());
	BOOL	isChecked = (MacroManager_ReturnDefaultMacros() == MacroManager_ReturnCurrentMacros());
	
	
	setItemCheckMark(anItem, isChecked);
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMacroSwitchNone:(id)		sender
{
#pragma unused(sender)
	MacroManager_Result		macrosResult = kMacroManager_ResultOK;
	
	
	macrosResult = MacroManager_SetCurrentMacros(nullptr);
	if (false == macrosResult.ok())
	{
		Sound_StandardAlert();
	}
}
- (id)
canPerformMacroSwitchNone:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	result = (nullptr != TerminalWindow_ReturnFromMainWindow());
	BOOL	isChecked = (nullptr == MacroManager_ReturnCurrentMacros());
	
	
	setItemCheckMark(anItem, isChecked);
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_ManagingMacros)


@implementation Commands_Executor (Commands_ManagingTerminalEvents)


- (IBAction)
performBellToggle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandBellEnabled, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSetActivityHandlerNone:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandWatchNothing, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSetActivityHandlerNotifyOnIdle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandWatchForInactivity, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSetActivityHandlerNotifyOnNext:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandWatchForActivity, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSetActivityHandlerSendKeepAliveOnIdle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTransmitOnInactivity, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_ManagingTerminalEvents)


@implementation Commands_Executor (Commands_ManagingTerminalKeyMappings)


- (IBAction)
performDeleteMapToBackspace:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandDeletePressSendsBackspace, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performDeleteMapToDelete:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandDeletePressSendsDelete, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performEmacsCursorModeToggle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandEmacsArrowMapping, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performLocalPageKeysToggle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandLocalPageUpDown, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMappingCustom:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSetKeys, nullptr/* target */);
}


- (IBAction)
performSetFunctionKeyLayoutRxvt:(id)	sender
{
	[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutRxvt:sender];
}
- (id)
canPerformSetFunctionKeyLayoutRxvt:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutRxvt ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return validatorYes(anItem);
}


- (IBAction)
performSetFunctionKeyLayoutVT220:(id)	sender
{
	[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutVT220:sender];
}
- (id)
canPerformSetFunctionKeyLayoutVT220:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutVT220 ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return validatorYes(anItem);
}


- (IBAction)
performSetFunctionKeyLayoutXTermX11:(id)	sender
{
	[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutXTermX11:sender];
}
- (id)
canPerformSetFunctionKeyLayoutXTermX11:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutXTerm ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return validatorYes(anItem);
}


- (IBAction)
performSetFunctionKeyLayoutXTermXFree86:(id)	sender
{
	[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] performSetFunctionKeyLayoutXTermXFree86:sender];
}
- (id)
canPerformSetFunctionKeyLayoutXTermXFree86:(id <NSValidatedUserInterfaceItem>)	anItem
{
	BOOL	isChecked = (kSession_FunctionKeyLayoutXTermXFree86 ==
							[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] currentFunctionKeyLayout]);
	
	
	setItemCheckMark(anItem, isChecked);
	
	return validatorYes(anItem);
}


- (IBAction)
performTranslationSwitchByFavoriteName:(id)		sender
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
														true/* is retained */);
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


- (IBAction)
performTranslationSwitchCustom:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSetTranslationTable, nullptr/* target */);
}


- (IBAction)
performTranslationSwitchDefault:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTranslationTableDefault, nullptr/* target */);
}


@end // Commands_Executor (Commands_ManagingTerminalKeyMappings)


@implementation Commands_Executor (Commands_ManagingTerminalSettings)


- (IBAction)
performInterruptProcess:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSendInterruptProcess, nullptr/* target */);
}


- (IBAction)
performJumpScrolling:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandJumpScrolling, nullptr/* target */);
}


- (IBAction)
performLineWrapToggle:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandWrapMode, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performLocalEchoToggle:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandEcho, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performReset:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandResetTerminal, nullptr/* target */);
}


- (IBAction)
performSaveOnClearToggle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandClearScreenSavesLines, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScrollbackClear:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandClearEntireScrollback, nullptr/* target */);
}


- (IBAction)
performSpeechToggle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSpeechEnabled, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performSuspendToggle:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSuspendNetwork, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performTerminalCustomSetup:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTerminalEmulatorSetup, nullptr/* target */);
}


@end // Commands_Executor (Commands_ManagingTerminalSettings)


@implementation Commands_Executor (Commands_ModifyingTerminalDimensions)


- (IBAction)
performScreenResizeCustom:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSetScreenSize, nullptr/* target */);
}
- (id)
canPerformScreenResizeCustom:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScreenResizeNarrower:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNarrowerScreen, nullptr/* target */);
}
- (id)
canPerformScreenResizeNarrower:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScreenResizeShorter:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShorterScreen, nullptr/* target */);
}
- (id)
canPerformScreenResizeShorter:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScreenResizeStandard:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSmallScreen, nullptr/* target */);
}
- (id)
canPerformScreenResizeStandard:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScreenResizeTall:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTallScreen, nullptr/* target */);
}
- (id)
canPerformScreenResizeTall:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScreenResizeTaller:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTallerScreen, nullptr/* target */);
}
- (id)
canPerformScreenResizeTaller:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScreenResizeWide:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandLargeScreen, nullptr/* target */);
}
- (id)
canPerformScreenResizeWide:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performScreenResizeWider:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandWiderScreen, nullptr/* target */);
}
- (id)
canPerformScreenResizeWider:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_ModifyingTerminalDimensions)


@implementation Commands_Executor (Commands_ModifyingTerminalText)


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
	SEL		thisSelector = @selector(performFormatDefault:);
	id		target = [NSApp targetForAction:thisSelector to:nil from:sender];
	
	
	if (self != target)
	{
		[NSApp sendAction:thisSelector to:target from:sender];
	}
	else
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
															true/* is retained */);
				
				
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
- (id)
canPerformFormatByFavoriteName:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performFormatCustom:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFormat, nullptr/* target */);
}
- (id)
canPerformFormatCustom:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performFormatDefault:(id)	sender
{
#pragma unused(sender)
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
	SEL		thisSelector = @selector(performFormatDefault:);
	id		target = [NSApp targetForAction:thisSelector to:nil from:sender];
	
	
	if (self != target)
	{
		[NSApp sendAction:thisSelector to:target from:sender];
	}
	else
	{
		Commands_ExecuteByIDUsingEvent(kCommandFormatDefault, nullptr/* target */);
	}
}
- (id)
canPerformFormatDefault:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performFormatTextBigger:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandBiggerText, nullptr/* target */);
}
- (id)
canPerformFormatTextBigger:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performFormatTextMaximum:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFullScreen, nullptr/* target */);
}
- (id)
canPerformFormatTextMaximum:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performFormatTextSmaller:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandSmallerText, nullptr/* target */);
}
- (id)
canPerformFormatTextSmaller:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_ModifyingTerminalText)


@implementation Commands_Executor (Commands_ModifyingWindows)

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
		NSEnumerator*	forScreens = [screenArray objectEnumerator];
		
		
		// look at the frames of each display and see which one is
		// most logically “close” to the window in the given direction
		while (NSScreen* currentScreen = [forScreens nextObject])
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
performArrangeInFront:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandStackWindows, nullptr/* target */);
}
- (id)
canPerformArrangeInFront:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = (false == FlagManager_Test(kFlagKioskMode));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performHideOtherWindows:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandHideOtherWindows, nullptr/* target */);
}
- (id)
canPerformHideOtherWindows:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	// INCOMPLETE, should really be disabled if all other terminal windows are hidden
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 1)));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performHideWindow:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandHideFrontWindow, nullptr/* target */);
}
- (id)
canPerformHideWindow:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = ((false == FlagManager_Test(kFlagKioskMode)) &&
						((nullptr != TerminalWindow_ReturnFromMainWindow()) &&
											(SessionFactory_ReturnCount() > 0)));
	
	
	return [NSNumber numberWithBool:result];
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
performMaximize:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandMaximizeWindow, nullptr/* target */);
}
- (id)
canPerformMaximize:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	id		target = [NSApp targetForAction:@selector(performMaximize:)];
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		if (isCarbonWindow(target))
		{
			HIWindowRef		userFocusWindow = GetUserFocusWindow();
			
			
			if (nullptr != userFocusWindow)
			{
				WindowAttributes	attributes = kWindowNoAttributes;
				
				
				result = YES;
				if (noErr == GetWindowAttributes(userFocusWindow, &attributes))
				{
					result = (0 != (attributes & kWindowFullZoomAttribute));
				}
			}
		}
		else if ([[target class] isSubclassOfClass:[NSWindow class]])
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			result = (0 != ([window styleMask] & NSResizableWindowMask));
		}
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMoveToNewWorkspace:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandTerminalNewWorkspace, nullptr/* target */);
}
- (id)
canPerformMoveToNewWorkspace:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			result = TerminalWindow_IsTab(terminalWindow) ? YES : NO;
		}
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMoveWindowRight:(id)		sender
{
#pragma unused(sender)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	
	
	if (nullptr != terminalWindow)
	{
		[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
							distance:8/* arbitrary; should match performMoveWindowLeft: */
							awayFromEdge:NSMaxXEdge withAnimation:NO];
	}
}
- (id)
canPerformMoveWindowRight:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			result = YES;
		}
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMoveWindowLeft:(id)		sender
{
#pragma unused(sender)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	
	
	if (nullptr != terminalWindow)
	{
		[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
							distance:8/* arbitrary; should match performMoveWindowRight: */
							awayFromEdge:NSMinXEdge withAnimation:NO];
	}
}
- (id)
canPerformMoveWindowLeft:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			result = YES;
		}
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMoveWindowDown:(id)		sender
{
#pragma unused(sender)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	
	
	if (nullptr != terminalWindow)
	{
		[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
							distance:8/* arbitrary; should match performMoveWindowUp: */
							awayFromEdge:NSMinYEdge withAnimation:NO];
	}
}
- (id)
canPerformMoveWindowDown:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			result = YES;
		}
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performMoveWindowUp:(id)		sender
{
#pragma unused(sender)
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
	
	
	if (nullptr != terminalWindow)
	{
		[self moveWindow:TerminalWindow_ReturnNSWindow(terminalWindow)
							distance:8/* arbitrary; should match performMoveWindowDown: */
							awayFromEdge:NSMaxYEdge withAnimation:NO];
	}
}
- (id)
canPerformMoveWindowUp:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromMainWindow();
		
		
		if (nullptr != terminalWindow)
		{
			result = YES;
		}
	}
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performRename:(id)	sender
{
#pragma unused(sender)
	BOOL	implementedByCocoa = NO;
	
	
	if (isCocoaWindowMoreImportantThanCarbon([NSApp keyWindow]))
	{
		// assume that abnormal Cocoa windows should handle this directly
		implementedByCocoa = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(performRename:) with:sender];
	}
	
	if (NO == implementedByCocoa)
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
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performShowHiddenWindows:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowAllHiddenWindows, nullptr/* target */);
}
- (id)
canPerformShowHiddenWindows:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	// INCOMPLETE, should really be disabled if no windows are actually hidden
	BOOL	result = (false == FlagManager_Test(kFlagKioskMode));
	
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_ModifyingWindows)


@implementation Commands_Executor (Commands_Searching)


- (IBAction)
performFind:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFind, nullptr/* target */);
}


- (IBAction)
performFindCursor:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFindCursor, nullptr/* target */);
}


- (IBAction)
performFindNext:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFindAgain, nullptr/* target */);
}
- (id)
canPerformFindNext:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = searchResultsExist();
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performFindPrevious:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFindPrevious, nullptr/* target */);
}
- (id)
canPerformFindPrevious:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = searchResultsExist();
	
	
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_Searching)


@implementation Commands_Executor (Commands_ShowingPanels)


- (IBAction)
orderFrontAbout:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandAboutThisApplication, nullptr/* target */);
}
- (id)
canOrderFrontAbout:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
orderFrontClipboard:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowClipboard, nullptr/* target */);
}
- (id)
canOrderFrontClipboard:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
orderFrontCommandLine:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowCommandLine, nullptr/* target */);
}
- (id)
canOrderFrontCommandLine:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
orderFrontContextualHelp:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandContextSensitiveHelp, nullptr/* target */);
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
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
orderFrontControlKeys:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowControlKeys, nullptr/* target */);
}
- (id)
canOrderFrontControlKeys:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
orderFrontDebuggingOptions:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandDebuggingOptions, nullptr/* target */);
}
- (id)
canOrderFrontDebuggingOptions:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
orderFrontIPAddresses:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowNetworkNumbers, nullptr/* target */);
}
- (id)
canOrderFrontIPAddresses:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


- (IBAction)
orderFrontPreferences:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kHICommandPreferences, nullptr/* target */);
}
- (id)
canOrderFrontPreferences:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL					result = YES;
	WindowInfo_Ref			windowInfoRef = nullptr;
	WindowInfo_Descriptor	windowDescriptor = kWindowInfo_InvalidDescriptor;
	
	
	// set the enabled state
	windowInfoRef = WindowInfo_ReturnFromWindow(EventLoop_ReturnRealFrontWindow());
	if (nullptr != windowInfoRef)
	{
		windowDescriptor = WindowInfo_ReturnWindowDescriptor(windowInfoRef);
	}
	result = (windowDescriptor != kConstantsRegistry_WindowDescriptorPreferences);
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
orderFrontSessionInfo:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowConnectionStatus, nullptr/* target */);
}
- (id)
canOrderFrontSessionInfo:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = (false == FlagManager_Test(kFlagKioskMode));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
orderFrontVT220FunctionKeys:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowFunction, nullptr/* target */);
}
- (id)
canOrderFrontVT220FunctionKeys:(id <NSValidatedUserInterfaceItem>)		anItem
{
	return validatorYes(anItem);
}


- (IBAction)
orderFrontVT220Keypad:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandShowKeypad, nullptr/* target */);
}
- (id)
canOrderFrontVT220Keypad:(id <NSValidatedUserInterfaceItem>)	anItem
{
	return validatorYes(anItem);
}


@end // Commands_ExecutionDelegate (Commands_ShowingPanels)


@implementation Commands_Executor (Commands_SwitchingModes)


- (IBAction)
performFullScreenOff:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandKioskModeDisable, nullptr/* target */);
}
- (id)
canPerformFullScreenOff:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = (FlagManager_Test(kFlagKioskMode) ? YES : NO);
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
performFullScreenOn:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandFullScreenModal, nullptr/* target */);
}


@end // Commands_Executor (Commands_SwitchingModes)


@implementation Commands_Executor (Commands_SwitchingWindows)


- (IBAction)
orderFrontNextWindow:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNextWindow, nullptr/* target */);
}
- (id)
canOrderFrontNextWindow:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = (false == FlagManager_Test(kFlagKioskMode));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
orderFrontNextWindowHidingPrevious:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandNextWindowHideCurrent, nullptr/* target */);
}
- (id)
canOrderFrontNextWindowHidingPrevious:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	BOOL	result = (false == FlagManager_Test(kFlagKioskMode));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
orderFrontPreviousWindow:(id)		sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandPreviousWindow, nullptr/* target */);
}
- (id)
canOrderFrontPreviousWindow:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = (false == FlagManager_Test(kFlagKioskMode));
	
	
	return [NSNumber numberWithBool:result];
}


- (IBAction)
orderFrontSpecificWindow:(id)		sender
{
	BOOL	isError = YES;
	
	
	if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		SessionRef		session = returnMenuItemSession(asMenuItem);
		
		
		if (nil != session)
		{
			TerminalWindowRef	terminalWindow = nullptr;
			HIWindowRef			window = nullptr;
			
			
			// first make the window visible if it was obscured
			window = Session_ReturnActiveWindow(session);
			terminalWindow = Session_ReturnActiveTerminalWindow(session);
			if (nullptr != terminalWindow) TerminalWindow_SetObscured(terminalWindow, false);
			
			// now select the window
			EventLoop_SelectOverRealFrontWindow(window);
			
			isError = (nullptr == window);
		}
	}
	
	if (isError)
	{
		// failed...
		Sound_StandardAlert();
	}
}
- (id)
canOrderFrontSpecificWindow:(id <NSValidatedUserInterfaceItem>)		anItem
{
	BOOL	result = (false == FlagManager_Test(kFlagKioskMode));
	
	
	if (result)
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)anItem;
		SessionRef		itemSession = returnMenuItemSession(asMenuItem);
		
		
		if (nullptr != itemSession)
		{
			HIWindowRef const	kSessionActiveWindow = Session_ReturnActiveWindow(itemSession);
			
			
			if (IsWindowHilited(kSessionActiveWindow))
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
	return [NSNumber numberWithBool:result];
}


@end // Commands_Executor (Commands_SwitchingWindows)


@implementation Commands_Executor (Commands_TransitionFromCarbon)


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
//
// This method’s name cannot exactly match the standard
// method used by a Cocoa window.
- (IBAction)
performCloseSetup:(id)	sender
{
	id		target = [NSApp targetForAction:@selector(performClose:)];
	
	
	// NSCarbonWindow does not implement performClose: properly,
	// so the mere existence of the method does not mean it can
	// be called.
	if ((nil == target) || isCarbonWindow(target))
	{
		Commands_ExecuteByIDUsingEvent(kCommandCloseConnection, nullptr/* target */);
	}
	else
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
	
	
	if (isCarbonWindow(target))
	{
		HIWindowRef		userFocusWindow = GetUserFocusWindow();
		
		
		if (nullptr != userFocusWindow)
		{
			WindowAttributes	attributes = kWindowNoAttributes;
			
			
			result = YES;
			if (noErr == GetWindowAttributes(userFocusWindow, &attributes))
			{
				result = (0 != (attributes & kWindowCloseBoxAttribute));
			}
		}
	}
	else if ([[target class] isSubclassOfClass:[NSWindow class]])
	{
		NSWindow*	window = (NSWindow*)target;
		
		
		result = (0 != ([window styleMask] & NSClosableWindowMask));
	}
	return [NSNumber numberWithBool:result];
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
performMinimizeSetup:(id)	sender
{
#pragma unused(sender)
	id		target = [NSApp targetForAction:@selector(performMiniaturize:)];
	
	
	// NSCarbonWindow does not implement performMiniaturize: properly, so
	// the mere existence of the method does not mean it can be called.
	if ((nil == target) || isCarbonWindow(target))
	{
		Commands_ExecuteByIDUsingEvent(kCommandMinimizeWindow, nullptr/* target */);
	}
	else
	{
		[NSApp sendAction:@selector(performMiniaturize:) to:target from:sender];
	}
}
- (id)
canPerformMinimizeSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		id		target = [NSApp targetForAction:@selector(performMiniaturize:)];
		
		
		if (isCarbonWindow(target))
		{
			HIWindowRef		userFocusWindow = GetUserFocusWindow();
			
			
			if (nullptr != userFocusWindow)
			{
				WindowAttributes	attributes = kWindowNoAttributes;
				
				
				result = YES;
				if (noErr == GetWindowAttributes(userFocusWindow, &attributes))
				{
					result = (0 != (attributes & kWindowCollapseBoxAttribute));
				}
			}
		}
		else if ([[target class] isSubclassOfClass:[NSWindow class]])
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			result = (0 != ([window styleMask] & NSMiniaturizableWindowMask));
		}
	}
	return [NSNumber numberWithBool:result];
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
performZoomSetup:(id)	sender
{
#pragma unused(sender)
	id		target = [NSApp targetForAction:@selector(performZoom:)];
	
	
	// NSCarbonWindow does not implement performZoom: properly, so the
	// mere existence of the method does not mean it can be called.
	if ((nil == target) || isCarbonWindow(target))
	{
		Commands_ExecuteByIDUsingEvent(kCommandZoomWindow, nullptr/* target */);
	}
	else
	{
		[NSApp sendAction:@selector(performZoom:) to:target from:sender];
	}
}
- (id)
canPerformZoomSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		id		target = [NSApp targetForAction:@selector(performZoom:)];
		
		
		if (isCarbonWindow(target))
		{
			HIWindowRef		userFocusWindow = GetUserFocusWindow();
			
			
			if (nullptr != userFocusWindow)
			{
				WindowAttributes	attributes = kWindowNoAttributes;
				
				
				result = YES;
				if (noErr == GetWindowAttributes(userFocusWindow, &attributes))
				{
					result = (0 != (attributes & kWindowFullZoomAttribute));
				}
			}
		}
		else if ([[target class] isSubclassOfClass:[NSWindow class]])
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			result = (0 != ([window styleMask] & NSResizableWindowMask));
		}
	}
	return [NSNumber numberWithBool:result];
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
runToolbarCustomizationPaletteSetup:(id)	sender
{
#pragma unused(sender)
	if (isCarbonWindow([NSApp mainWindow]))
	{
		HIWindowRef		userFocusWindow = GetUserFocusWindow();
		
		
		if (nullptr != userFocusWindow)
		{
			Commands_ExecuteByIDUsingEvent(kHICommandCustomizeToolbar, nullptr/* target */);
		}
	}
	else
	{
		id		target = [NSApp targetForAction:@selector(runToolbarCustomizationPalette:)];
		
		
		if ((target) && ([[target class] isSubclassOfClass:[NSWindow class]]))
		{
			NSWindow*	window = (NSWindow*)target;
			
			
			[window runToolbarCustomizationPalette:sender];
		}
	}
}
- (id)
canRunToolbarCustomizationPaletteSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		if (isCarbonWindow([NSApp mainWindow]))
		{
			HIWindowRef		userFocusWindow = GetUserFocusWindow();
			
			
			if (nullptr != userFocusWindow)
			{
				HIToolbarRef	toolbar = nullptr;
				
				
				if ((noErr == GetWindowToolbar(userFocusWindow, &toolbar)) && (nullptr != toolbar))
				{
					OptionBits		optionBits = 0;
					
					
					result = YES;
					if (noErr == HIToolbarGetAttributes(toolbar, &optionBits))
					{
						result = (0 != (optionBits & kHIToolbarIsConfigurable));
					}
				}
			}
		}
		else
		{
			id		target = [NSApp targetForAction:@selector(runToolbarCustomizationPalette:)];
			
			
			if ((target) && ([[target class] isSubclassOfClass:[NSWindow class]]))
			{
				NSWindow*	window = (NSWindow*)target;
				
				
				result = [[window toolbar] allowsUserCustomization];
			}
		}
	}
	return [NSNumber numberWithBool:result];
}


// These are obviously Carbon-specific, and will disappear
// once target windows are based exclusively on NSWindow.
- (IBAction)
toggleToolbarShownSetup:(id)	sender
{
#pragma unused(sender)
	if (IsWindowToolbarVisible(GetUserFocusWindow()))
	{
		Commands_ExecuteByIDUsingEvent(kHICommandHideToolbar, nullptr/* target */);
	}
	else
	{
		Commands_ExecuteByIDUsingEvent(kHICommandShowToolbar, nullptr/* target */);
	}
}
- (id)
canToggleToolbarShownSetup:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	BOOL	result = NO;
	
	
	if (false == FlagManager_Test(kFlagKioskMode))
	{
		HIWindowRef		userFocusWindow = GetUserFocusWindow();
		
		
		if (nullptr != userFocusWindow)
		{
			HIToolbarRef	toolbar = nullptr;
			BOOL			useShowText = YES;
			
			
			if ((noErr == GetWindowToolbar(userFocusWindow, &toolbar)) && (nullptr != toolbar))
			{
				result = YES;
				if (IsWindowToolbarVisible(userFocusWindow))
				{
					useShowText = NO;
				}
			}
			
			// update item to use the appropriate show/hide command text
			// UNIMPLEMENTED
		}
	}
	return [NSNumber numberWithBool:result];
}


/*!
Internal version of Commands_IsCommandEnabled().

(4.0)
*/
- (BOOL)
isCommandEnabled:(UInt32)	aCommandID
{
	NSMenuItem*		targetItem = nil; // currently, never defined for these checks
	NSNumber*		numericalBool = nil;
	BOOL			result = false;
	
	
	switch (aCommandID)
	{
	case kCommandKioskModeDisable:
		numericalBool = [self canPerformFullScreenOff:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandChangeWindowTitle:
		numericalBool = [self canPerformRename:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandCloseConnection:
		numericalBool = [self canPerformCloseSetup:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandCopy:
		numericalBool = [self canPerformCopy:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandCopyTable:
		numericalBool = [self canPerformCopyWithTabSubstitution:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandFind:
		result = YES; // TEMPORARY
		break;
	
	case kCommandFormat:
		numericalBool = [self canPerformFormatCustom:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandHandleURL:
		numericalBool = [self canPerformOpenURL:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandHideFrontWindow:
		numericalBool = [self canPerformHideWindow:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandPaste:
		numericalBool = [self canPerformPaste:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandPrintScreen:
		numericalBool = [self canPerformPrintScreen:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandSaveText:
		numericalBool = [self canPerformSaveSelection:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandSetKeys:
		result = YES;
		break;
	
	case kCommandSetScreenSize:
		numericalBool = [self canPerformScreenResizeCustom:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandSpeakSelectedText:
		// for now, reuse the handler for saving selections; they should be equivalent
		numericalBool = [self canPerformSaveSelection:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandStopSpeaking:
		result = CocoaBasic_SpeakingInProgress() ? YES : NO;
		break;
	
	case kCommandStackWindows:
		numericalBool = [self canPerformArrangeInFront:targetItem];
		result = [numericalBool boolValue];
		break;
	
	case kCommandTerminalNewWorkspace:
		numericalBool = [self canPerformMoveToNewWorkspace:targetItem];
		result = [numericalBool boolValue];
		break;
	
	default:
		break;
	}
	
	return result;
}// isCommandEnabled:


@end // Commands_Executor (Commands_TransitionFromCarbon)


@implementation Commands_Executor (Commands_Validation)


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
	NSString*			result = nil;
	NSMutableString*	nameOfAction = [[[NSMutableString alloc] initWithString:anActionSelectorName]
										autorelease];
	NSString*			actionFirstChar = [nameOfAction substringToIndex:1];
	
	
	[nameOfAction replaceCharactersInRange:NSMakeRange(0, 1/* length */)
											withString:[actionFirstChar uppercaseString]];
	result = [@"can" stringByAppendingString:nameOfAction];
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
method is called.  Validators are encouraged to use the method
[NSNumber numberWithBool:] when returning their results.

See selectorNameForValidateActionName: and
validateUserInterfaceItem:.

(4.0)
*/
+ (SEL)
selectorToValidateAction:(SEL)	anAction
{
	SEL		result = NSSelectorFromString([[self class] selectorNameForValidateActionName:NSStringFromSelector(anAction)]);
	
	
	return result;
}// selectorToValidateAction:


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
	SEL		itemAction = [anItem action];
	SEL		validator = [[self class] selectorToValidateAction:itemAction];
	BOOL	result = YES;
	
	
	if (nil != validator)
	{
		id		target = [NSApp targetForAction:validator to:nil from:anItem];
		
		
		if (nil != target)
		{
			// See selectorToValidateAction: for more information on the form of the selector.
			result = [[target performSelector:validator withObject:anItem] boolValue];
		}
		else if ([self respondsToSelector:validator])
		{
			// See selectorToValidateAction: for more information on the form of the selector.
			result = [[self performSelector:validator withObject:anItem] boolValue];
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
			result = (nullptr != SessionFactory_ReturnUserFocusSession());
		}
	}
	return result;
}// validateUserInterfaceItem:


@end // Commands_Executor (Commands_Validation)


@implementation Commands_ServiceProviders


/*!
Designated initializer.

(4.0)
*/
- (id)
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
error:(NSString**)					error
{
#pragma unused(userData)
	// NOTE: later versions of Mac OS X change pasteboard APIs significantly; this will eventually change
	NSString*	pathString = [aPasteboard stringForType:NSStringPboardType];
	NSString*	errorString = nil;
	
	
	if (nil == pathString)
	{
		pathString = [aPasteboard stringForType:NSURLPboardType];
	}
	if (nil == pathString)
	{
		pathString = [aPasteboard stringForType:NSFilenamesPboardType];
	}
	
	// for a completely incomprehensible reason, it seems that the system converts
	// simple text selections containing pathnames into multiple-line text strings
	// where one line is the path and another line is a file URL; so instead of
	// just opening the damned path, it is necessary to first look for this
	// bastardized version of the string and strip out whatever was appended...
	{
		NSArray*	components = [pathString componentsSeparatedByString:@"\015"/* carriage return */];
		
		
		if ((nil != components) && ([components count] > 1))
		{
			pathString = (NSString*)[components objectAtIndex:0];
		}
		else
		{
			components = [pathString componentsSeparatedByString:@"\012"/* line feed */];
			if ((nil != components) && ([components count] > 1))
			{
				pathString = (NSString*)[components objectAtIndex:0];
			}
		}
	}
	
	if (nil != pathString)
	{
		NSURL*		testURL = nil;
		
		
		// ignore leading and trailing whitespace
		pathString = [pathString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		
		// in the off chance that a file URL was given, just strip off the
		// file scheme component and implicitly pull out the path
		testURL = [NSURL URLWithString:pathString];
		if ((nil != testURL) && (nil != [testURL path]) && ([[testURL scheme] isEqualToString:@"file"]))
		{
			pathString = [testURL path];
		}
	}
	
	if (nil == pathString)
	{
		errorString = NSLocalizedStringFromTable(@"Unable to find a path in the given selection.", @"Services"/* table */,
													@"error message for non-strings given to the open-at-path Service provider");
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
																(CFStringRef)pathString/* current working directory */);
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
			Session_SetWindowUserDefinedTitle(newSession, (CFStringRef)pathString);
		}
	}
	
	if (nil != errorString)
	{
		AlertMessages_BoxRef	box = Alert_New();
		
		
		*error = errorString;
		Alert_SetParamsFor(box, kAlert_StyleOK);
		Alert_SetType(box, kAlertNoteAlert);
		Alert_SetTextCFStrings(box, (CFStringRef)*error, ([pathString length] > 100/* arbitrary */)
															? (CFStringRef)[pathString substringToIndex:99]
															: (CFStringRef)pathString/* help text */);
		Alert_Display(box);
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
error:(NSString**)			error
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
		if ((nil == testURL) || (nil == [testURL scheme]))
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
														true/* is retained */); // LOCALIZE THIS?
				
				
				Console_WriteScriptError((CFStringRef)titleText, messageCFString.returnCFStringRef());
				errorString = NSLocalizedStringFromTable(@"Unable to do anything with the given resource.", @"Services"/* table */,
															@"error message when an exception is raised within the open-URL Service provider");
			}
		}
	}
	
	if (nil != errorString)
	{
		AlertMessages_BoxRef	box = Alert_New();
		
		
		*error = errorString;
		Alert_SetParamsFor(box, kAlert_StyleOK);
		Alert_SetType(box, kAlertNoteAlert);
		Alert_SetTextCFStrings(box, (CFStringRef)*error, ([theURLString length] > 100/* arbitrary */)
															? (CFStringRef)[theURLString substringToIndex:99]
															: (CFStringRef)theURLString/* help text */);
		Alert_Display(box);
	}
}// openURL:userData:error:


@end // Commands_ServiceProviders


@implementation Commands_SessionWrap


/*!
Designated initializer.

(4.0)
*/
- (id)
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


@end // Commands_SessionWrap

// BELOW IS REQUIRED NEWLINE TO END FILE
