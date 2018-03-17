/*!	\file SessionFactory.mm
	\brief Construction mechanism for sessions (terminal
	windows that run local or remote processes).
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

#import "SessionFactory.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cctype>
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <map>
#import <sstream>
#import <vector>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CocoaAnimation.h>
#import <Console.h>
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// application includes
#import "DebugInterface.h"
#import "DialogUtilities.h"
#import "EventLoop.h"
#import "GenericDialog.h"
#import "InfoWindow.h"
#import "Local.h"
#import "MacroManager.h"
#import "NetEvents.h"
#import "Preferences.h"
#import "PrefPanelSessions.h"
#import "PrefsWindow.h"
#import "QuillsBase.h"
#import "RegionUtilities.h"
#import "SessionDescription.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "TerminalWindow.h"
#import "TextTranslation.h"
#import "UIStrings.h"
#import "Workspace.h"



#pragma mark Types

namespace {

typedef std::vector< SessionRef >						SessionList;
typedef std::vector< TerminalWindowRef >				TerminalWindowList;
typedef std::multimap< TerminalWindowRef, SessionRef >	TerminalWindowToSessionsMap;
typedef std::vector< Workspace_Ref >					MyWorkspaceList;

} // anonymous namespace


/*!
Reloads the specified preferences context (assumed
to be temporary) and applies certain changes to the
target terminal window.
*/
@interface SessionFactory_NewSessionDataObject : NSObject //{
{
@private
	BOOL						_disableObservers;
	Preferences_ContextRef		_temporaryContext;
	TerminalWindowRef			_terminalWindow;
}

// initializers
	- (instancetype)
	initWithSessionContext:(Preferences_ContextRef)_
	terminalWindow:(TerminalWindowRef)_;

// accessors
	@property (assign) BOOL
	disableObservers;
	@property (readonly) Preferences_ContextRef
	temporaryContext;
	@property (assign) TerminalWindowRef
	terminalWindow;

// NSKeyValueObserving
	- (void)
	observeValueForKeyPath:(NSString*)_
	ofObject:(id)_
	change:(NSDictionary*)_
	context:(void*)_;

@end //}


/*!
This object exists only to have a way to receive
window notifications and track the active session.
*/
@interface SessionFactory_SessionWindowWatcher : NSObject //{

@end //}

#pragma mark Internal Method Prototypes
namespace {

OSStatus				appendDataForProcessing			(EventHandlerCallRef, EventRef, void*);
void					changeNotifyGlobal				(SessionFactory_Change, void*);
Boolean					configureSessionTerminalWindow	(TerminalWindowRef, Preferences_ContextRef);
Boolean					configureSessionTerminalWindowByClass	(TerminalWindowRef, Preferences_ContextRef, Quills::Prefs::Class);
void					copySessionTerminalWindowConfiguration	(Preferences_ContextRef, Quills::Prefs::Class,
														 Preferences_ContextRef&);
TerminalWindowRef		createTerminalWindow			(Preferences_ContextRef = nullptr,
														 Preferences_ContextRef = nullptr,
														 Preferences_ContextRef = nullptr,
														 Boolean = false);
Workspace_Ref			createWorkspace					();
Boolean					displayTerminalWindow			(TerminalWindowRef, Preferences_ContextRef = nullptr, UInt16 = 0);
void					forEachSessionInListDo			(SessionList const&, SessionFactory_SessionBlock);
void					forEachTerminalWindowInListDo	(TerminalWindowList const&, SessionFactory_TerminalWindowBlock);
void					handleNewSessionDialogClose		(GenericDialog_Ref, Boolean);
Boolean					newSessionFromCommand			(TerminalWindowRef, UInt32, Preferences_ContextRef, UInt16);
OSStatus				receiveHICommand				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowActivated			(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowDeactivated		(EventHandlerCallRef, EventRef, void*);
Workspace_Ref			returnActiveWorkspace			();
void					sessionChanged					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
void					sessionStateChanged				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
OSStatus				setSessionState					(EventHandlerCallRef, EventRef, void*);
void					startTrackingSession			(SessionRef, TerminalWindowRef);
void					startTrackingTerminalWindow		(TerminalWindowRef);
void					stopTrackingSession				(SessionRef);
void					stopTrackingTerminalWindow		(TerminalWindowRef);

} // anonymous namespace

#pragma mark Variables
namespace {

Boolean							gAutoRearrangeTabs = true;
ListenerModel_Ref				gSessionFactoryStateChangeListenerModel = nullptr;
ListenerModel_Ref				gSessionStateChangeListenerModel = nullptr;
ListenerModel_ListenerRef		gSessionChangeListenerRef = nullptr;
ListenerModel_ListenerRef		gSessionStateChangeListener = nullptr;
CarbonEventHandlerWrap			gNewSessionCommandHandler(GetApplicationEventTarget(),
															receiveHICommand,
															CarbonEventSetInClass
																(CarbonEventClass(kEventClassCommand),
																	kEventCommandProcess),
															nullptr/* user data */);
Console_Assertion				_1(gNewSessionCommandHandler.isInstalled(), __FILE__, __LINE__);
CarbonEventHandlerWrap			gSessionFactoryWindowActivateHandler(GetApplicationEventTarget(),
																		receiveWindowActivated,
																		CarbonEventSetInClass
																			(CarbonEventClass(kEventClassWindow),
																				kEventWindowActivated),
																		nullptr/* user data */);
Console_Assertion				_2(gSessionFactoryWindowActivateHandler.isInstalled(), __FILE__, __LINE__);
CarbonEventHandlerWrap			gSessionFactoryWindowDeactivateHandler(GetApplicationEventTarget(),
																		receiveWindowDeactivated,
																		CarbonEventSetInClass
																			(CarbonEventClass(kEventClassWindow),
																				kEventWindowDeactivated),
																		nullptr/* user data */);
Console_Assertion				_3(gSessionFactoryWindowActivateHandler.isInstalled(), __FILE__, __LINE__);
SessionFactory_SessionWindowWatcher*	gSessionWindowWatcher = nil;
SessionRef						gSessionFactoryRecentlyActiveSession = nullptr;
EventHandlerUPP					gCarbonEventSessionProcessDataUPP = nullptr;
EventHandlerUPP					gCarbonEventSessionSetStateUPP = nullptr;
EventHandlerUPP					gCarbonEventWindowFocusUPP = nullptr;
EventHandlerRef					gCarbonEventSessionProcessDataHandler = nullptr;
EventHandlerRef					gCarbonEventSessionSetStateHandler = nullptr;
EventHandlerRef					gCarbonEventWindowFocusHandler = nullptr;
SessionList&					gSessionListSortedByCreationTime ()		{ static SessionList x; return x; }
TerminalWindowList&				gTerminalWindowListSortedByCreationTime ()	{ static TerminalWindowList x; return x; }
MyWorkspaceList&				gWorkspaceListSortedByCreationTime ()	{ static MyWorkspaceList x; return x; }
TerminalWindowToSessionsMap&	gTerminalWindowToSessions()	{ static TerminalWindowToSessionsMap x; return x; }

} // anonymous namespace

#pragma mark Functors
namespace {

/*!
Examines every terminal window in the specified workspace
and updates its tab placement to match its position in the
workspace.  This has the effect of “nudging over” tabs
when windows disappear or are inserted.

This only applies to legacy Carbon tabs; Cocoa windows use
system window tabs that resize and position themselves
automatically.

IMPORTANT: Consult "gAutoRearrangeTabs" before using this.

Model of STL Unary Function.

(3.1)
*/
#pragma mark fixTerminalWindowTabPositionsInWorkspace
class fixTerminalWindowTabPositionsInWorkspace:
public std::unary_function< Workspace_Ref/* argument */, void/* return */ >
{
public:
	fixTerminalWindowTabPositionsInWorkspace ()
	{
	}
	
	void
	operator()	(Workspace_Ref	inWorkspace)
	{
		Float32 const		kAverageTabWidth = 320.0; // arbitrary!
		__block UInt16		windowCount = 0; // counts Carbon windows only
		__block Float32		currentOffset = 0.0; // while processing, each tab size is added here to define the next offset
		__block Float32		smallestMaxWidth = FLT_MAX;
		
		
		// determine how wide each tab should be; use the smallest window
		// for this, since Mac OS X will otherwise force a window resize
		Workspace_ForEachTerminalWindow(inWorkspace,
		^(TerminalWindowRef		inTerminalWindow,
		  Boolean&				UNUSED_ARGUMENT(outStopFlag))
		{
			// there is no way to implement this for Cocoa windows so only
			// the legacy Carbon window tabs are counted here (and eventually
			// this whole function will go away)
			if (TerminalWindow_IsLegacyCarbon(inTerminalWindow))
			{
				TerminalWindow_Result	terminalResult = kTerminalWindow_ResultOK;
				Float32					availableSpace = FLT_MAX;
				
				
				++windowCount;
				
				terminalResult = TerminalWindow_GetTabWidthAvailable(inTerminalWindow, availableSpace);
				if (kTerminalWindow_ResultOK == terminalResult)
				{
					if (availableSpace < smallestMaxWidth)
					{
						smallestMaxWidth = availableSpace;
					}
				}
			}
		});
		if (windowCount > 0)
		{
			Float32		idealTabWidth = smallestMaxWidth / windowCount;
			
			
			if (idealTabWidth > kAverageTabWidth)
			{
				idealTabWidth = kAverageTabWidth;
			}
			
			// reposition and resize each window’s tab appropriately
			Workspace_ForEachTerminalWindow(inWorkspace,
			^(TerminalWindowRef		inTerminalWindow,
			  Boolean&				UNUSED_ARGUMENT(outStopFlag))
			{
				TerminalWindow_Result	terminalResult = kTerminalWindow_ResultOK;
				
				
				terminalResult = TerminalWindow_SetTabPosition(inTerminalWindow, currentOffset, idealTabWidth);
				if (kTerminalWindow_ResultOK == terminalResult)
				{
					Float32		tabWidth = 0.0;
					
					
					terminalResult = TerminalWindow_GetTabWidth(inTerminalWindow, tabWidth);
					//assert(kTerminalWindow_ResultOK == terminalResult);
					if (kTerminalWindow_ResultOK == terminalResult)
					{
						currentOffset += tabWidth;
						
						// reset the tab flag; this has the effect of opening the drawer
						// if it is not already open (TEMPORARY - should this be done in
						// a more direct way?)
						UNUSED_RETURN(OSStatus)TerminalWindow_SetTabAppearance(inTerminalWindow, true);
					}
				}
			});
		}
	}

protected:

private:
};

/*!
This is a functor that removes the terminal window
given at construction time, from a specified workspace.

Model of STL Unary Function.

(3.1)
*/
#pragma mark workspaceContainsWindow
class workspaceContainsWindow:
public std::unary_function< Workspace_Ref/* argument */, bool/* return */ >
{
public:
	workspaceContainsWindow	(TerminalWindowRef		inWindow)
	: _window(inWindow)
	{
	}
	
	bool
	operator()	(Workspace_Ref	inWorkspace)
	const
	{
		__block bool	result = false;
		
		
		Workspace_ForEachTerminalWindow(inWorkspace,
		^(TerminalWindowRef		inTerminalWindow,
		  Boolean&				outStopFlag)
		{
			if (inTerminalWindow == _window)
			{
				result = true;
				outStopFlag = true;
			}
		});
		
		return result;
	}

protected:

private:
	TerminalWindowRef	_window;
};

} // anonymous namespace



#pragma mark Public Methods

/*!
Call this method at the start of the program,
before it is necessary to detect changes to
the Session Factory.

(3.0)
*/
void
SessionFactory_Init ()
{
	gSessionFactoryStateChangeListenerModel = ListenerModel_New
												(kListenerModel_StyleStandard,
													kConstantsRegistry_ListenerModelDescriptorSessionFactoryChanges);
	gSessionStateChangeListenerModel = ListenerModel_New
										(kListenerModel_StyleStandard,
											kConstantsRegistry_ListenerModelDescriptorSessionFactoryAnySessionChanges);
	gSessionChangeListenerRef = ListenerModel_NewStandardListener(sessionChanged);
	gSessionWindowWatcher = [[SessionFactory_SessionWindowWatcher alloc] init];
	
	// watch for changes to session states - in particular, when they die, update the internal lists
	gSessionStateChangeListener = ListenerModel_NewStandardListener(sessionStateChanged);
	SessionFactory_StartMonitoringSessions(kSession_ChangeState, gSessionStateChangeListener);
	
	// under Carbon, listen for special Carbon Events that effectively invoke Session_SetState();
	// this is for thread safety, to force these calls to always take place in the main thread
	{
		EventTypeSpec const		whenSessionStateMustChange[] =
								{
									{ kEventClassNetEvents_Session, kEventNetEvents_SessionSetState }
								};
		OSStatus				error = noErr;
		
		
		gCarbonEventSessionSetStateUPP = NewEventHandlerUPP(setSessionState);
		error = InstallApplicationEventHandler(gCarbonEventSessionSetStateUPP, GetEventTypeCount(whenSessionStateMustChange),
												whenSessionStateMustChange, nullptr/* user data */,
												&gCarbonEventSessionSetStateHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// under Carbon, listen for special Carbon Events that effectively invoke
	// Session_AppendDataForProcessing(); this is for thread safety, to force these calls to
	// always take place in the main thread
	{
		EventTypeSpec const		whenSessionDataIsAvailableForProcessing[] =
								{
									{ kEventClassNetEvents_Session, kEventNetEvents_SessionDataArrived }
								};
		OSStatus				error = noErr;
		
		
		gCarbonEventSessionProcessDataUPP = NewEventHandlerUPP(appendDataForProcessing);
		error = InstallApplicationEventHandler(gCarbonEventSessionProcessDataUPP,
												GetEventTypeCount(whenSessionDataIsAvailableForProcessing),
												whenSessionDataIsAvailableForProcessing, nullptr/* user data */,
												&gCarbonEventSessionProcessDataHandler/* event handler reference */);
		assert_noerr(error);
	}
}// Init


/*!
Call this method at the end of the program,
when other clean-up is being done and it is
no longer necessary to detect changes to
the Session Factory.

(3.0)
*/
void
SessionFactory_Done ()
{
	[gSessionWindowWatcher release]; gSessionWindowWatcher = nil;
	
	ListenerModel_ReleaseListener(&gSessionStateChangeListener);
	ListenerModel_ReleaseListener(&gSessionChangeListenerRef);
	ListenerModel_Dispose(&gSessionStateChangeListenerModel);
	ListenerModel_Dispose(&gSessionFactoryStateChangeListenerModel);
	
	RemoveEventHandler(gCarbonEventSessionProcessDataHandler), gCarbonEventSessionProcessDataHandler = nullptr;
	RemoveEventHandler(gCarbonEventSessionSetStateHandler), gCarbonEventSessionSetStateHandler = nullptr;
	RemoveEventHandler(gCarbonEventWindowFocusHandler), gCarbonEventWindowFocusHandler = nullptr;
	DisposeEventHandlerUPP(gCarbonEventSessionProcessDataUPP), gCarbonEventSessionProcessDataUPP = nullptr;
	DisposeEventHandlerUPP(gCarbonEventSessionSetStateUPP), gCarbonEventSessionSetStateUPP = nullptr;
	DisposeEventHandlerUPP(gCarbonEventWindowFocusUPP), gCarbonEventWindowFocusUPP = nullptr;
}// Done


/*!
Creates a copy of a session in the specified terminal window
(or a new window, if nullptr is given for the first parameter).

If a new window is constructed, it is animated into its stagger
position to reflect the fact that a duplication is occurring.
When passing in your own window, you should first perform an
animation with CocoaAnimation_TransitionWindowForDuplicate().

NOTE:	A future version of this routine might add a parameter
		to allow only some characteristics of the base session
		to be duplicated, instead of all.

(3.1)
*/
SessionRef
SessionFactory_NewCloneSession	(TerminalWindowRef		inTerminalWindow,
								 SessionRef				inBaseSession)
{
	SessionRef		result = nullptr;
	CFArrayRef		argsArray = Session_ReturnCommandLine(inBaseSession);
	
	
	if (nullptr == inTerminalWindow)
	{
		TerminalWindowRef	sourceTerminalWindow = Session_ReturnActiveTerminalWindow(inBaseSession);
		
		
		// TEMPORARY; createTerminalWindow() accepts up to 3 context arguments,
		// which should be extracted or derived from the original session somehow
		// (to completely duplicate things like window size, fonts and colors);
		// for now, a window style based on the default is chosen
		inTerminalWindow = createTerminalWindow(nullptr, nullptr, nullptr, true/* no stagger */);
		
		if (false == TerminalWindow_IsTab(sourceTerminalWindow))
		{
			NSWindow*	targetWindow = TerminalWindow_ReturnNSWindow(inTerminalWindow);
			Boolean		noAnimations = false;
			
			
			// determine if animation should occur
			unless (kPreferences_ResultOK ==
					Preferences_GetData(kPreferences_TagNoAnimations,
										sizeof(noAnimations), &noAnimations))
			{
				noAnimations = false; // assume a value, if preference can’t be found
			}
			
			// if an original terminal window was given, perform a transition to
			// move the window from its (unstaggered) location to its new location
			if (noAnimations)
			{
				[targetWindow orderFront:nil];
			}
			else
			{
				CocoaAnimation_TransitionWindowForDuplicate(targetWindow,
															(nullptr != sourceTerminalWindow)
																? TerminalWindow_ReturnNSWindow(sourceTerminalWindow)
																: TerminalWindow_ReturnNSWindow(inTerminalWindow));
			}
		}
	}
	
	if ((nullptr != argsArray) && (nullptr != inTerminalWindow))
	{
		result = SessionFactory_NewSessionArbitraryCommand(inTerminalWindow, argsArray);
	}
	return result;
}// NewCloneSession


/*!
Creates a terminal window (or uses the specified window, if not
nullptr), and attempts to run the specified process inside it.

If a main context is supplied, it is assumed to primarily
contain Session-class settings, which will be used to change
the initial settings; otherwise, global defaults will be chosen
implicitly.  Note that a Session contains a command line, which
will always be ignored in favor of "inArgumentArray".

A Session can also contain associations with other kinds of
preferences (like a Format to set fonts and colors, a Terminal
to set the screen size, etc., and a Translation to set the
character set).  Since you may already have applied these with
configureSessionTerminalWindow(), you may choose to skip them
now, if "inReconfigureTerminalFromAssociatedContexts" is false.
This is recommended when you are calling this from another
factory routine that has already displayed the terminal window,
because it should have called configureSessionTerminalWindow()
before displaying that window.

If unsuccessful, nullptr is returned, and an alert message may
be displayed to the user; otherwise, the session reference for
the new local process is returned.

(3.0)
*/
SessionRef
SessionFactory_NewSessionArbitraryCommand	(TerminalWindowRef			inTerminalWindow,
											 CFArrayRef					inArgumentArray,
											 Preferences_ContextRef		inContextOrNull,
											 Boolean					inReconfigureTerminalFromAssociatedContexts,
											 Preferences_ContextRef		inWorkspaceOrNull,
											 UInt16						inWindowIndexInWorkspaceOrZero,
											 CFStringRef				inWorkingDirectoryOrNull)
{
	SessionRef			result = nullptr;
	TerminalWindowRef	terminalWindow = inTerminalWindow;
	
	
	assert(nullptr != terminalWindow);
	
	// since this is the lowest-level routine, it is used by some
	// other session-creation routines, and those routines may have
	// already reconfigured the terminal (as it is typically done
	// before the terminal window is displayed); therefore, the
	// terminal is not automatically reconfigured here, it is only
	// done on request
	if ((inReconfigureTerminalFromAssociatedContexts) && (nullptr != inContextOrNull))
	{
		if (false == configureSessionTerminalWindow(terminalWindow, inContextOrNull))
		{
			Console_Warning(Console_WriteLine, "terminal window for arbitrary command could not apply associated-context preferences");
		}
	}
	
	if (false == displayTerminalWindow(terminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero))
	{
		Console_WriteLine("unexpected problem displaying terminal window!!!");
	}
	else
	{
		result = Session_New(inContextOrNull);
		if (nullptr != result)
		{
			Local_Result	localResult = kLocal_ResultOK;
			
			
			// see also SessionFactory_RespawnSession(), which must do something similar
			localResult = Local_SpawnProcess(result, TerminalWindow_ReturnScreenWithFocus(terminalWindow),
												inArgumentArray, inWorkingDirectoryOrNull);
			if (kLocal_ResultOK == localResult)
			{
				// success!
				startTrackingSession(result, terminalWindow);
				
				// fix initial text encoding at the Session level; it is generally set for
				// the window and view elsewhere, but UTF-8 makes it important to fix the
				// local process as well (that is why it is set here, after the process
				// exists, and not any sooner)
				{
					CFStringEncoding const		kDefaultEncoding = kCFStringEncodingUTF8;
					Preferences_ContextRef		translationSettings = nullptr;
					Preferences_Result			prefsResult = kPreferences_ResultOK;
					Boolean						releaseSettings = false;
					
					
					if (nullptr != inContextOrNull)
					{
						// a Session context could be associated with a Translation context;
						// if so, a little work must be done to find the actual context
						CFStringRef		contextName = nullptr;
						
						
						prefsResult = Preferences_ContextGetData(inContextOrNull, kPreferences_TagAssociatedTranslationFavorite,
																	sizeof(contextName), &contextName,
																	false/* search defaults too */);
						if (kPreferences_ResultOK == prefsResult)
						{
							if (false == Preferences_IsContextNameInUse(Quills::Prefs::TRANSLATION, contextName))
							{
								Console_Warning(Console_WriteValueCFString, "associated Translation not found", contextName);
							}
							else
							{
								translationSettings = Preferences_NewContextFromFavorites(Quills::Prefs::TRANSLATION, contextName);
								if (nullptr == translationSettings)
								{
									Console_Warning(Console_WriteLine, "text translation settings could not be found!");
								}
								else
								{
									releaseSettings = true;
								}
							}
							CFRelease(contextName), contextName = nullptr;
						}
					}
					
					// if necessary, fall back on default translation settings
					if (nullptr == translationSettings)
					{
						prefsResult = Preferences_GetDefaultContext(&translationSettings, Quills::Prefs::TRANSLATION);
						if (kPreferences_ResultOK != prefsResult)
						{
							translationSettings = nullptr;
						}
					}
					
					// apply translation settings
					if (nullptr == translationSettings)
					{
						Console_Warning(Console_WriteLine, "no text translation settings could be found, not even default values!");
					}
					else
					{
						if (false == TextTranslation_ContextSetEncoding(Session_ReturnTranslationConfiguration(result),
																		TextTranslation_ContextReturnEncoding
																		(translationSettings, kDefaultEncoding),
																		true/* via copy */))
						{
							Console_Warning(Console_WriteLine, "failed to set text encoding of new session");
						}
					}
					
					if (releaseSettings)
					{
						Preferences_ReleaseContext(&translationSettings);
					}
				}
			}
			else
			{
				// TEMPORARY - NEED to display some kind of user alert here
				Console_WriteValue("process spawn failed, error", localResult);
				Sound_StandardAlert();
				Session_Dispose(&result);
				
				// NOTE: normally destroying a session will also release the terminal
				// window, but in this case it isn’t associated with the session yet
				stopTrackingTerminalWindow(terminalWindow);
				TerminalWindow_Dispose(&terminalWindow);
			}
		}
	}
	
	return result;
}// NewSessionArbitraryCommand


/*!
Creates a new session whose command line is implicitly set to
the user’s preferred shell, and whose other session preferences
come from user defaults.  A workspace may be given however to
customize where the window is displayed, and a different
starting directory may also be used (user’s home directory by
default).

See SessionFactory_NewSessionArbitraryCommand() for more on
how the returned session is constructed.

(3.1)
*/
SessionRef
SessionFactory_NewSessionDefaultShell	(TerminalWindowRef			inTerminalWindow,
										 Preferences_ContextRef		inWorkspaceOrNull,
										 UInt16						inWindowIndexInWorkspaceOrZero,
										 CFStringRef				inWorkingDirectoryOrNull)
{
	SessionRef				result = nullptr;
	TerminalWindowRef		terminalWindow = inTerminalWindow;
	
	
	assert(nullptr != terminalWindow);
	
	// display the window
	if (false == displayTerminalWindow(terminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero))
	{
		// some kind of problem?!?
		Console_WriteLine("unexpected problem displaying terminal window!!!");
	}
	else
	{
		CFArrayRef		argumentCFArray = nullptr;
		Local_Result	localResult = kLocal_ResultOK;
		
		
		localResult = Local_GetDefaultShellCommandLine(argumentCFArray);
		if ((kLocal_ResultOK == localResult) && (nullptr != argumentCFArray))
		{
			result = SessionFactory_NewSessionArbitraryCommand(terminalWindow, argumentCFArray,
																nullptr/* session context */, false/* reconfigure terminal */,
																inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero,
																inWorkingDirectoryOrNull);
			CFRelease(argumentCFArray), argumentCFArray = nullptr;
		}
		// INCOMPLETE!!!
	}
	return result;
}// NewSessionDefaultShell


/*!
Creates a terminal window (or uses the specified
window, if non-nullptr) and attempts to run the
specified ".command" file - just like Terminal would.

If unsuccessful, nullptr is returned and an alert
message may be displayed to the user; otherwise, the
session reference for the new local process is
returned.

(3.0)
*/
SessionRef
SessionFactory_NewSessionFromCommandFile	(TerminalWindowRef			inTerminalWindow,
											 char const*				inCommandFilePath,
											 Preferences_ContextRef		inWorkspaceOrNull,
											 UInt16						inWindowIndexInWorkspaceOrZero)
{
	SessionRef		result = SessionFactory_NewSessionDefaultShell(inTerminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero);
	
	
	if (nullptr != result)
	{
		Boolean			displayOK = false;
		std::string		buffer(inCommandFilePath);
		
		
		// construct a command that runs the shell script and then exits;
		// escape the path from the shell by using apostrophes
		buffer = std::string("\'") + buffer;
		buffer += "\' ; exit\n";
		
		// pass the command line to the running shell
		{
			CFRetainRelease		asObject(CFStringCreateWithCString(kCFAllocatorDefault, buffer.c_str(), kCFStringEncodingUTF8),
											CFRetainRelease::kAlreadyRetained);
			
			
			if (asObject.exists())
			{
				Session_UserInputCFString(result, asObject.returnCFStringRef());
				
				// success!
				displayOK = true;
			}
		}
		
		unless (displayOK)
		{
			// TEMPORARY - NEED to display some kind of user alert here
			Sound_StandardAlert();
			stopTrackingSession(result);
			Session_Dispose(&result);
		}
	}
	
	return result;
}// NewSessionFromCommandFile


/*!
Creates a new session based on the parameters in the
given Session Description, which is RETAINED with a call
to SessionDescription_Retain() - that way, session data
can be consulted for an indefinite period of time while
the session is created asynchronously.

If you provide an existing terminal window and the given
file contains terminal-related settings, the window will
be changed to use the settings in the file (for example,
different colors or a new font size).

Returns the new session, or nullptr if errors occur.

(3.1)
*/
SessionRef
SessionFactory_NewSessionFromDescription	(TerminalWindowRef			inTerminalWindow,
											 SessionDescription_Ref		inSessionDescription,
											 Preferences_ContextRef		inWorkspaceOrNull,
											 UInt16						inWindowIndexInWorkspaceOrZero)
{
	SessionRef					result = nullptr;
	TerminalWindowRef			terminalWindow = inTerminalWindow;
	SessionDescription_Result   dataAccessError = kSessionDescription_ResultOK;
	
	
	SessionDescription_Retain(inSessionDescription); // prevent file object from being deleted until session setup completes
	
	// read terminal customization parameters
	// INCOMPLETE
	{
		// macro set
		CFStringRef		macroSetNameCFString = nullptr;
		
		
		dataAccessError = SessionDescription_GetStringData
							(inSessionDescription, kSessionDescription_StringTypeMacroSet, macroSetNameCFString);
		if ((kSessionDescription_ResultOK != dataAccessError) || (nullptr == macroSetNameCFString))
		{
			(MacroManager_Result)MacroManager_SetCurrentMacros(nullptr);
		}
		else
		{
			Preferences_ContextRef		defaultMacroSet = MacroManager_ReturnDefaultMacros();
			CFStringRef					defaultName = nullptr;
			
			
			if (nullptr != defaultMacroSet)
			{
				Preferences_Result		prefsResult = Preferences_ContextGetName(defaultMacroSet, defaultName);
				
				
				if (kPreferences_ResultOK != prefsResult)
				{
					defaultName = nullptr;
				}
			}
			
			if ((nullptr != defaultName) &&
				(kCFCompareEqualTo == CFStringCompare(defaultName, macroSetNameCFString, 0/* flags */)))
			{
				// this is the Default macro set; assign accordingly
				MacroManager_Result		macroResult = MacroManager_SetCurrentMacros(defaultMacroSet);
				
				
				if (false == macroResult.ok())
				{
					Console_Warning(Console_WriteLine, "unable to choose default macro set");
				}
			}
			else
			{
				// non-Default macro set
				if (false == Preferences_IsContextNameInUse(Quills::Prefs::MACRO_SET, macroSetNameCFString))
				{
					Console_Warning(Console_WriteValueCFString, "unable to find requested macro set", macroSetNameCFString);
					(MacroManager_Result)MacroManager_SetCurrentMacros(nullptr);
				}
				else
				{
					Preferences_ContextWrap		macroSet(Preferences_NewContextFromFavorites(Quills::Prefs::MACRO_SET,
																								macroSetNameCFString),
															Preferences_ContextWrap::kAlreadyRetained);
					
					
					if (false == macroSet.exists())
					{
						Console_Assert("macro set not found by name even though Preferences module claims it exists", false);
						(MacroManager_Result)MacroManager_SetCurrentMacros(nullptr);
					}
					else
					{
						MacroManager_Result		macroResult = MacroManager_SetCurrentMacros(macroSet.returnRef());
						
						
						if (false == macroResult.ok())
						{
							Console_Warning(Console_WriteValueCFString, "unable to choose macro set", macroSetNameCFString);
						}
					}
				}
			}
		}
	}
	{
		// screen dimensions (total)
		SInt32		rows = 0;
		SInt32		columns = 0;
		
		
		dataAccessError = SessionDescription_GetIntegerData
							(inSessionDescription, kSessionDescription_IntegerTypeTerminalVisibleColumnCount, columns);
		if (kSessionDescription_ResultOK == dataAccessError)
		{
			dataAccessError = SessionDescription_GetIntegerData
								(inSessionDescription, kSessionDescription_IntegerTypeTerminalVisibleLineCount, rows);
			if (kSessionDescription_ResultOK == dataAccessError)
			{
				TerminalWindow_SetScreenDimensions(terminalWindow, STATIC_CAST(columns, UInt16), STATIC_CAST(rows, UInt16),
													false/* send to recording scripts */);
			}
		}
	}
	{
		// font name
		CFStringRef		fontCFString = nullptr;
		
		
		dataAccessError = SessionDescription_GetStringData
							(inSessionDescription, kSessionDescription_StringTypeTerminalFont, fontCFString);
		if (kSessionDescription_ResultOK == dataAccessError)
		{
			Str255		fontName;
			
			
			CFRetain(fontCFString);
			if (CFStringGetPascalString(fontCFString, fontName, sizeof(fontName), kCFStringEncodingMacRoman))
			{
				if (nullptr != CTFontCreateWithQuickdrawInstance(fontName, 0/* font identifier */,
																	normal/* font style */, 0.0/* font size */))
				{
					TerminalWindow_SetFontAndSize(terminalWindow, fontCFString, 0/* font size, or 0 to ignore */);
				}
			}
			CFRelease(fontCFString);
		}
	}
	{
		// font size
		SInt32		fontSize = 0;
		
		
		dataAccessError = SessionDescription_GetIntegerData
							(inSessionDescription, kSessionDescription_IntegerTypeTerminalFontSize, fontSize);
		if (kSessionDescription_ResultOK == dataAccessError)
		{
			TerminalWindow_SetFontAndSize(terminalWindow, nullptr/* font */, STATIC_CAST(fontSize, UInt16));
		}
	}
	{
		// colors (currently, cannot vary across views)
		UInt16					viewCount = 0;
		TerminalViewRef*		viewArray = nullptr;
		TerminalWindow_Result   terminalWindowResult = kTerminalWindow_ResultOK;
		
		
		viewCount = TerminalWindow_ReturnViewCountInGroup(terminalWindow, kTerminalWindow_ViewGroupEverything);
		viewArray = REINTERPRET_CAST(CFAllocatorAllocate
										(kCFAllocatorDefault, viewCount * sizeof(TerminalViewRef), 0/* hints */),
										TerminalViewRef*);
		if (nullptr != viewArray)
		{
			terminalWindowResult = TerminalWindow_GetViewsInGroup(terminalWindow, kTerminalWindow_ViewGroupEverything,
																	viewCount, viewArray, nullptr/* actual size */);
			if (kTerminalWindow_ResultOK != terminalWindowResult)
			{
				CGDeviceColor	colorValue;
				UInt16			i = 0;
				
				
				for (i = 0; i < viewCount; ++i)
				{
					// normal foreground color
					dataAccessError = SessionDescription_GetRGBColorData
										(inSessionDescription, kSessionDescription_RGBColorTypeTextNormal, colorValue);
					if (kSessionDescription_ResultOK == dataAccessError)
					{
						TerminalView_SetColor(viewArray[i], kTerminalView_ColorIndexNormalText, &colorValue);
					}
					
					// normal background color
					dataAccessError = SessionDescription_GetRGBColorData
										(inSessionDescription, kSessionDescription_RGBColorTypeBackgroundNormal, colorValue);
					if (kSessionDescription_ResultOK == dataAccessError)
					{
						TerminalView_SetColor(viewArray[i], kTerminalView_ColorIndexNormalBackground, &colorValue);
					}
					
					// bold foreground color
					dataAccessError = SessionDescription_GetRGBColorData
										(inSessionDescription, kSessionDescription_RGBColorTypeTextBold, colorValue);
					if (kSessionDescription_ResultOK == dataAccessError)
					{
						TerminalView_SetColor(viewArray[i], kTerminalView_ColorIndexBoldText, &colorValue);
					}
					
					// bold background color
					dataAccessError = SessionDescription_GetRGBColorData
										(inSessionDescription, kSessionDescription_RGBColorTypeBackgroundBold, colorValue);
					if (kSessionDescription_ResultOK == dataAccessError)
					{
						TerminalView_SetColor(viewArray[i], kTerminalView_ColorIndexBoldBackground, &colorValue);
					}
					
					// blinking foreground color
					dataAccessError = SessionDescription_GetRGBColorData
										(inSessionDescription, kSessionDescription_RGBColorTypeTextBlinking, colorValue);
					if (kSessionDescription_ResultOK == dataAccessError)
					{
						TerminalView_SetColor(viewArray[i], kTerminalView_ColorIndexBlinkingText, &colorValue);
					}
					
					// blinking background color
					dataAccessError = SessionDescription_GetRGBColorData
										(inSessionDescription, kSessionDescription_RGBColorTypeBackgroundBlinking, colorValue);
					if (kSessionDescription_ResultOK == dataAccessError)
					{
						TerminalView_SetColor(viewArray[i], kTerminalView_ColorIndexBlinkingBackground, &colorValue);
					}
				}
			}
			CFAllocatorDeallocate(kCFAllocatorDefault, viewArray);
		}
	}
	
	// see if this is a command line session...
	{
		CFStringRef		commandLine = nullptr;
		
		
		dataAccessError = SessionDescription_GetStringData(inSessionDescription, kSessionDescription_StringTypeCommandLine,
															commandLine);
		if (kSessionDescription_ResultOK == dataAccessError)
		{
			// okay, that data is in the file; this means it describes a “local” session...
			CFArrayRef		argv = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, commandLine,
																			CFSTR(" ")/* separators */);
			
			
			if (nullptr != argv)
			{
				assert(nullptr != terminalWindow);
				result = SessionFactory_NewSessionArbitraryCommand(terminalWindow, argv, nullptr/* session context */,
																	false/* reconfigure terminal window */,
																	inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero);
				CFRelease(argv), argv = nullptr;
			}
		}
		else
		{
			// see if this is a remote host session...
			//CFStringRef		hostName = nullptr;
			
			
			//dataAccessError = SessionDescription_GetStringData(inSessionDescription, kSessionDescription_StringTypeHostName,
			//													hostName);
			if (kSessionDescription_ResultOK == dataAccessError)
			{
				// okay, host name data is in the file; this means it describes a “remote” session...
				// UNIMPLEMENTED
				// (NOTE: currently on Mac OS X, all “remote” sessions are effectively local commands...)
			}
			else
			{
				// ???
			}
		}
	}
	
	return result;
}// NewSessionFromDescription


/*!
Creates a new session whose command line is implicitly set to
construct a login shell, and whose other session preferences
come from user defaults.  A workspace may be given however to
customize where the window is displayed.

Unlike SessionFactory_NewSessionDefaultShell(), this function
does not allow a custom working directory to be set because a
login shell will always reset this to the user’s home directory.

See SessionFactory_NewSessionArbitraryCommand() for more on
how the returned session is constructed.

(3.1)
*/
SessionRef
SessionFactory_NewSessionLoginShell		(TerminalWindowRef			inTerminalWindow,
										 Preferences_ContextRef		inWorkspaceOrNull,
										 UInt16						inWindowIndexInWorkspaceOrZero)
{
	SessionRef				result = nullptr;
	TerminalWindowRef		terminalWindow = inTerminalWindow;
	
	
	assert(nullptr != terminalWindow);
	
	// display the window
	if (false == displayTerminalWindow(terminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero))
	{
		// some kind of problem?!?
		Console_WriteLine("unexpected problem displaying terminal window!!!");
	}
	else
	{
		CFArrayRef		argumentCFArray = nullptr;
		Local_Result	localResult = kLocal_ResultOK;
		
		
		localResult = Local_GetLoginShellCommandLine(argumentCFArray);
		if ((kLocal_ResultOK == localResult) && (nullptr != argumentCFArray))
		{
			result = SessionFactory_NewSessionArbitraryCommand(terminalWindow, argumentCFArray,
																nullptr/* session context */, false/* reconfigure terminal */,
																inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero);
			CFRelease(argumentCFArray), argumentCFArray = nullptr;
		}
		// INCOMPLETE!!!
	}
	return result;
}// NewSessionLoginShell


/*!
Creates a new session based on the given preferences.
Everything about the session is automatically determined
from its data, or filled in with defaults.

Returns true only if the Favorite location *attempt*
was made without errors.  You can only use the return
value to determine if (say) the specified Favorite
was found, NOT to determine if a new session was
actually created.

If you need to know when the session opens, install
a listener for a Session Factory event (such as
"kSessionFactory_ChangeNewSessionCount") and respond
to the listener by examining the session list.

(3.1)
*/
SessionRef
SessionFactory_NewSessionUserFavorite	(TerminalWindowRef			inTerminalWindow,
										 Preferences_ContextRef		inSessionContext,
										 Preferences_ContextRef		inWorkspaceOrNull,
										 UInt16						inWindowIndexInWorkspaceOrZero)
{
	SessionRef				result = nullptr;
	TerminalWindowRef		terminalWindow = inTerminalWindow;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	assert(nullptr != terminalWindow);
	
	// it is best to apply settings before displaying the terminal, since
	// they can have side effects (e.g. resizing); therefore, "false"
	// should be passed to SessionFactory_NewSessionArbitraryCommand(),
	// below, to make sure it does not repeat this reconfiguration
	if (false == configureSessionTerminalWindow(terminalWindow, inSessionContext))
	{
		Console_Warning(Console_WriteLine, "terminal window for user favorite command could not apply associated-context preferences");
	}
	
	// display the window
	if (false == displayTerminalWindow(terminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero))
	{
		// some kind of problem?!?
		Console_WriteLine("unexpected problem displaying terminal window!!!");
	}
	else
	{
		CFArrayRef		argumentCFArray = nullptr;
		
		
		prefsResult = Preferences_ContextGetData(inSessionContext, kPreferences_TagCommandLine,
													sizeof(argumentCFArray), &argumentCFArray);
		if (kPreferences_ResultOK == prefsResult)
		{
			result = SessionFactory_NewSessionArbitraryCommand(terminalWindow, argumentCFArray,
																inSessionContext, false/* reconfigure terminal */,
																inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero);
			CFRelease(argumentCFArray), argumentCFArray = nullptr;
		}
		// INCOMPLETE!!!
	}
	return result;
}// NewSessionUserFavorite


/*!
Creates new sessions for each listed in the given workspace,
in the order they are stored.  Any other constraints (such as
confining them to a tab stack or starting Full Screen) are
automatically respected.

(4.0)
*/
Boolean
SessionFactory_NewSessionsUserFavoriteWorkspace		(Preferences_ContextRef		inWorkspaceContext)
{
	Boolean					result = true;
	Boolean					enterFullScreen = false;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	// determine if this workspace should automatically enter Full Screen
	prefsResult = Preferences_ContextGetData(inWorkspaceContext/* context */, kPreferences_TagArrangeWindowsFullScreen,
												sizeof(enterFullScreen), &enterFullScreen,
												true/* search defaults */, nullptr/* actual size */);
	if (prefsResult != kPreferences_ResultOK)
	{
		enterFullScreen = false;
	}
	
	// not every window in the workspace may be defined; launch
	// every session that is found
	for (Preferences_Index i = 1; i <= kPreferences_MaximumWorkspaceSize; ++i)
	{
		CFStringRef			associatedSessionName = nullptr;
		TerminalWindowRef	terminalWindow = nullptr;
		
		
		prefsResult = Preferences_ContextGetData(inWorkspaceContext,
													Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedWindowSessionFavorite, i),
													sizeof(associatedSessionName), &associatedSessionName,
													false/* search defaults too */);
		if (kPreferences_ResultOK == prefsResult)
		{
			if (false == Preferences_IsContextNameInUse(Quills::Prefs::SESSION, associatedSessionName))
			{
				result = false;
			}
			else
			{
				Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
															(Quills::Prefs::SESSION, associatedSessionName),
															Preferences_ContextWrap::kAlreadyRetained);
				
				
				if (false == namedSettings.exists())
				{
					result = false;
				}
				else
				{
					terminalWindow = createTerminalWindow();
					
					SessionRef		session = SessionFactory_NewSessionUserFavorite(terminalWindow,
																					namedSettings.returnRef(),
																					inWorkspaceContext, i);
					if (nullptr == session)
					{
						result = false;
					}
				}
			}
			CFRelease(associatedSessionName), associatedSessionName = nullptr;
		}
		else
		{
			UInt32		associatedSessionType = 0;
			
			
			prefsResult = Preferences_ContextGetData(inWorkspaceContext,
														Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowCommandType, i),
														sizeof(associatedSessionType), &associatedSessionType,
														false/* search defaults too */);
			if ((kPreferences_ResultOK == prefsResult) && (0 != associatedSessionType))
			{
				terminalWindow = createTerminalWindow();
				
				Boolean		launchOK = newSessionFromCommand
										(terminalWindow, associatedSessionType, inWorkspaceContext, i);
				if (false == launchOK)
				{
					result = false;
				}
			}
			else
			{
				// this window is disabled; ignore
			}
		}
		
		if (result)
		{
			if ((enterFullScreen) && (nullptr != terminalWindow))
			{
				if (TerminalWindow_IsLegacyCarbon(terminalWindow))
				{
					HIWindowRef			window = TerminalWindow_ReturnLegacyCarbonWindow(terminalWindow);
					EventTargetRef		windowTarget = GetWindowEventTarget(window);
					
					
					Commands_ExecuteByIDUsingEventAfterDelay(kCommandFullScreenToggle, windowTarget, (i + 1) * 0.5/* delay */);
				}
				else
				{
					CocoaExtensions_RunLater((i + 1) * 0.5/* delay */, ^{ [TerminalWindow_ReturnNSWindow(terminalWindow) toggleFullScreen:NSApp]; });
				}
			}
		}
	}
	
	return result;
}// NewSessionsUserFavoriteWorkspace


/*!
Creates a new terminal window that automatically
configures itself to use the default preferences
set by the user.

(3.1)
*/
TerminalWindowRef
SessionFactory_NewTerminalWindowUserFavorite	(Preferences_ContextRef		inTerminalInfoOrNull,
												 Preferences_ContextRef		inFontInfoOrNull,
												 Preferences_ContextRef		inTranslationInfoOrNull)
{
	return createTerminalWindow(inTerminalInfoOrNull, inFontInfoOrNull, inTranslationInfoOrNull);
}// NewTerminalWindowUserFavorite


/*!
Reruns the same command line that was used to start the
specified session, using the same terminal window.  The given
session MUST be in the “dead” state (with its window still
open).  Returns true only if successful.

IMPORTANT:	Typically you should not invoke this directly, but
			rather call Session_DisplayTerminationWarning()
			and specify that a restart should occur.  This
			will keep the user in the loop and give a warning
			when appropriate, with an option to Cancel as well.

This function should have a similar implementation to that of
SessionFactory_NewSessionArbitraryCommand(), except that no
new session object or window are created.

(4.0)
*/
Boolean
SessionFactory_RespawnSession	(SessionRef		inSession)
{
	Boolean		result = false;
	
	
	if (Session_StateIsDead(inSession))
	{
		Local_Result			localResult = kLocal_ResultOK;
		TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(inSession);
		TerminalScreenRef		screenBuffer = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
		CFStringRef				workingDirectory = Session_ReturnOriginalWorkingDirectory(inSession);
		
		
		localResult = Local_SpawnProcess(inSession, screenBuffer, Session_ReturnCommandLine(inSession),
											workingDirectory);
		result = (kLocal_ResultOK == localResult);
		
		// NOTE: the session is still being tracked from when it was first created,
		// so there is no need to start tracking it after a respawn; however, it is
		// still important to activate the session (which also notifies listeners);
		// also, make the session immediately “long lived” without the usual delay
		Session_SetState(inSession, kSession_StateActiveUnstable);
		Session_SetState(inSession, kSession_StateActiveStable);
	}
	return result;
}// RespawnSession


/*!
Returns "true" if at least one session is in use,
regardless of state (i.e. it doesn’t have to be
connected yet).

(3.0)
*/
Boolean
SessionFactory_CountIsAtLeastOne ()
{
	Boolean		result = (false == gSessionListSortedByCreationTime().empty());
	
	
	return result;
}// CountIsAtLeastOne


/*!
Creates a terminal window and immediately displays a
session customization dialog.  The given workspace
information, if any, is used to set the initial
position and title of the window.

Returns true only if the user customization *attempt*
was made without errors.  Since the user may take
considerable time to complete the session creation,
this function returns immediately; you can only use
the return value to determine if (say) the dialog was
displayed correctly, NOT to determine if the user’s
new session was created.

If you need to know when the user is finished, install
a listener for a Session Factory event (such as
"kSessionFactory_ChangeNewSessionCount") and respond
to the listener by examining the session list.

(3.0)
*/
Boolean
SessionFactory_DisplayUserCustomizationUI	(TerminalWindowRef			inTerminalWindow,
											 Preferences_ContextRef		inWorkspaceOrNull,
											 UInt16						inWindowIndexInWorkspaceOrZero)
{
	TerminalWindowRef						terminalWindow = inTerminalWindow;
	Preferences_ContextRef					sessionContext = nullptr;
	Preferences_ContextRef					temporaryContext = nullptr;
	SessionFactory_NewSessionDataObject*	dataObject = nil;
	Preferences_Result						prefsResult = kPreferences_ResultOK;
	Boolean									result = false;
	
	
	assert(nullptr != terminalWindow);
	
	prefsResult = Preferences_GetDefaultContext(&sessionContext, Quills::Prefs::SESSION);
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteLine, "unable to initialize dialog with Session Default preferences!");
	}
	
	dataObject = [[SessionFactory_NewSessionDataObject alloc]
					initWithSessionContext:sessionContext terminalWindow:terminalWindow];
	temporaryContext = dataObject.temporaryContext; // lifetime is managed by data object
	
	if (nullptr == temporaryContext)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
	}
	else
	{
		PrefPanelSessions_ResourceViewManager*		embeddedPanel = [[PrefPanelSessions_ResourceViewManager alloc] init];
		
		
		// observe changes to menus so that the terminal window
		// can show the user what their effect would be (e.g.
		// different size or colors); due to callbacks, this
		// can change the initial terminal window appearance
		[embeddedPanel addObserver:dataObject forKeyPath:@"formatFavorite.currentValueDescriptor" options:0 context:nullptr];
		[embeddedPanel addObserver:dataObject forKeyPath:@"terminalFavorite.currentValueDescriptor" options:0 context:nullptr];
		
		// display a terminal window and then immediately display
		// a sheet asking the user what to do with the new window
		if (displayTerminalWindow(terminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero))
		{
			GenericDialog_Wrap	dialog;
			CFRetainRelease		addToPrefsString(UIStrings_ReturnCopy(kUIStrings_PreferencesWindowAddToFavoritesButton),
													CFRetainRelease::kAlreadyRetained);
			CFRetainRelease		cancelString(UIStrings_ReturnCopy(kUIStrings_ButtonCancel),
												CFRetainRelease::kAlreadyRetained);
			CFRetainRelease		startSessionString(UIStrings_ReturnCopy(kUIStrings_ButtonStartSession),
													CFRetainRelease::kAlreadyRetained);
			
			
			// display the sheet
			dataObject.disableObservers = YES; // temporarily disable to prevent visible shift in window appearance
			if (TerminalWindow_IsLegacyCarbon(terminalWindow))
			{
				dialog = GenericDialog_Wrap(GenericDialog_NewParentCarbon(TerminalWindow_ReturnLegacyCarbonWindow(terminalWindow),
																			embeddedPanel, temporaryContext),
											GenericDialog_Wrap::kAlreadyRetained);
			}
			else
			{
				NSWindow*	window = TerminalWindow_ReturnNSWindow(terminalWindow);
				NSView*		parentView = STATIC_CAST(window.contentView, NSView*);
				
				
				dialog = GenericDialog_Wrap(GenericDialog_New(parentView, embeddedPanel, temporaryContext),
											GenericDialog_Wrap::kAlreadyRetained);
			}
			[embeddedPanel release], embeddedPanel = nil; // panel is retained by the call above
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton1, startSessionString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton1,
												^{ handleNewSessionDialogClose(dialog.returnRef(), true/* is OK */); });
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton2, cancelString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton2,
												^{ handleNewSessionDialogClose(dialog.returnRef(), false/* is OK */); });
			GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton3, addToPrefsString.returnCFStringRef());
			GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton3,
												^{
													Preferences_TagSetRef	tagSet = PrefPanelSessions_NewResourcePaneTagSet();
													
													
													PrefsWindow_AddCollection(temporaryContext, tagSet,
																				kCommandDisplayPrefPanelSessions);
													Preferences_ReleaseTagSet(&tagSet);
												});
			GenericDialog_SetItemDialogEffect(dialog.returnRef(), kGenericDialog_ItemIDButton2, kGenericDialog_DialogEffectCloseImmediately);
			GenericDialog_SetImplementation(dialog.returnRef(), dataObject);
			[dataObject retain], GenericDialog_Display(dialog.returnRef(), false/* animated */,
														^{ [dataObject release]; }); // retains dialog until it is dismissed
			dataObject.disableObservers = NO;
			result = true;
		}
	}
	
	return result;
}// DisplayUserCustomizationUI


/*!
Performs the specified operation on every session in
the list.  The list must NOT change during iteration;
if that condition cannot be guaranteed, you must call
SessionFactory_ForEachSessionCopyList() instead.

(2017.09)
*/
void
SessionFactory_ForEachSession	(SessionFactory_SessionBlock	inBlock)
{
	forEachSessionInListDo(gSessionListSortedByCreationTime(), inBlock);
}// ForEachSession


/*!
Performs the specified operation on every session in
a copy of the session list.  This version is required
if the primary list of sessions can be changed during
your loop iteration.

(2017.09)
*/
void
SessionFactory_ForEachSessionCopyList	(SessionFactory_SessionBlock	inBlock)
{
	SessionList		listCopy = gSessionListSortedByCreationTime();
	
	
	forEachSessionInListDo(listCopy, inBlock);
}// ForEachSessionCopyList


/*!
Performs the specified operation on every terminal window
in the list.  The list must NOT change during iteration;
if that condition cannot be guaranteed, you must call
SessionFactory_ForEachSessionCopyList() instead (and
extract the terminal window from each session).

(2017.09)
*/
void
SessionFactory_ForEachTerminalWindow	(SessionFactory_TerminalWindowBlock		inBlock)
{
	// ordinary iterations can use the actual list, but they MUST
	// NOT INSERT OR DELETE any items in the list!
	forEachTerminalWindowInListDo(gTerminalWindowListSortedByCreationTime(), inBlock);
}// ForEachTerminalWindow


/*!
Creates a brand new workspace (window group or tab group)
and moves the specified terminal window into it, removing
the window from its previous workspace.

(3.1)
*/
void
SessionFactory_MoveTerminalWindowToNewWorkspace		(TerminalWindowRef		inTerminalWindow)
{
	if (TerminalWindow_IsLegacyCarbon(inTerminalWindow))
	{
		Workspace_Ref		newWorkspace = createWorkspace();
		MyWorkspaceList&	workspaceList = gWorkspaceListSortedByCreationTime();
		
		
		// IMPORTANT: Hiding the tab prior to window group manipulation
		// seems to be key to avoiding graphical glitches.  As long as
		// the tab drawer is hidden when the window changes groups, and
		// redisplayed afterwards, drawers remain in the proper position.
		UNUSED_RETURN(OSStatus)TerminalWindow_SetTabAppearance(inTerminalWindow, false);
		
		assert(nullptr != newWorkspace);
		assert(workspaceList.end() != std::find(workspaceList.begin(), workspaceList.end(), newWorkspace));
		
		// ensure the window is not a member of any other workspace
		std::for_each(workspaceList.begin(), workspaceList.end(), [=](Workspace_Ref wsp) { Workspace_RemoveTerminalWindow(wsp, inTerminalWindow); });
		if (gAutoRearrangeTabs)
		{
			// TEMPORARY, INCOMPLETE - figure out what workspace the window used to be in,
			// and call "fixTerminalWindowTabPositionsInWorkspace()(oldWorkspace)"
			// (should this kind of thing be handled automatically through callbacks, when
			// windows are removed from a workspace for any reason?)
			(fixTerminalWindowTabPositionsInWorkspace)std::for_each(workspaceList.begin(), workspaceList.end(),
																	fixTerminalWindowTabPositionsInWorkspace());
		}
		
		// offset the window slightly to emphasize its detachment (Carbon only)
		if (TerminalWindow_IsLegacyCarbon(inTerminalWindow))
		{
			HIWindowRef		window = TerminalWindow_ReturnLegacyCarbonWindow(inTerminalWindow);
			Rect			structureBounds;
			OSStatus		error = noErr;
			
			
			error = GetWindowBounds(window, kWindowStructureRgn, &structureBounds);
			if (noErr == error)
			{
				RegionUtilities_OffsetRect(&structureBounds, 32/* arbitrary */, 32/* arbitrary */);
				SetWindowBounds(window, kWindowStructureRgn, &structureBounds);
			}
		}
		
		// now add it to the new workspace
		Workspace_AddTerminalWindow(newWorkspace, inTerminalWindow);
		UNUSED_RETURN(OSStatus)TerminalWindow_SetTabAppearance(inTerminalWindow, true);
		fixTerminalWindowTabPositionsInWorkspace()(newWorkspace);
	}
	else
	{
		// Cocoa windows support this only on certain later OS versions
		[[Commands_Executor sharedExecutor] moveTabToNewWindow:NSApp];
	}
}// MoveTerminalWindowToNewWorkspace


/*!
Returns the number of sessions that are in
use, regardless of state (i.e. they are counted
even if they aren’t connected yet).

(3.0)
*/
UInt16
SessionFactory_ReturnCount ()
{
	return STATIC_CAST(gSessionListSortedByCreationTime().size(), UInt16);
}// ReturnCount


/*!
Traverses all sessions and counts the number of
sessions with the specified status.

NOTE:	This operation is currently linear in the
		number of open sessions.  It could probably
		be made constant time if this is considered
		important.

(3.0)
*/
UInt16
SessionFactory_ReturnStateCount		(Session_State		inStateToCheckFor)
{
	UInt16		result = 0;
	
	
	// traverse the list
	for (auto sessionRef :gSessionListSortedByCreationTime())
	{
		if ((nullptr != sessionRef) && (Session_ReturnState(sessionRef) == inStateToCheckFor))
		{
			++result;
		}
	}
	
	return result;
}// ReturnStateCount


/*!
Returns the session of the specified terminal window.

(4.0)
*/
SessionRef
SessionFactory_ReturnTerminalWindowSession		(TerminalWindowRef		inTerminalWindow)
{
	SessionRef		result = nullptr;
	
	
	if (nullptr != inTerminalWindow)
	{
		auto	toTerminalWindowSessionPair = gTerminalWindowToSessions().find(inTerminalWindow);
		
		
		if (gTerminalWindowToSessions().end() != toTerminalWindowSessionPair)
		{
			result = toTerminalWindowSessionPair->second;
		}
	}
	
	return result;
}// ReturnTerminalWindowSession


/*!
Returns the session the user is currently interacting with, or
nullptr if none is found.  This is the appropriate target to
assume when processing keyboard input or commands, etc. and
must always be a valid, focused window if it is defined (since
commands may end up displaying sheets, etc. and need a target).

See also InfoWindow_ReturnSelectedSession(), which is useful in
certain contexts where you want to allow commands to apply to a
selected window even if no terminal window is frontmost.

And see also SessionFactory_ReturnUserRecentSession().

(3.1)
*/
SessionRef
SessionFactory_ReturnUserFocusSession ()
{
	SessionRef		result = SessionFactory_ReturnTerminalWindowSession
								(TerminalWindow_ReturnFromKeyWindow());
	
	
	return result;
}// ReturnUserFocusSession


/*!
Returns the session of the main window, or nullptr if none is
found.  The only difference between this and the user focus (see
SessionFactory_ReturnUserFocusSession()) is that this will return
the main window session even if the focus is, say, in a floating
window such as a keypad or command line.  It will never return a
session whose window is not open.

See also InfoWindow_ReturnSelectedSession() and
SessionFactory_ReturnUserFocusSession().

(4.0)
*/
SessionRef
SessionFactory_ReturnUserRecentSession ()
{
	SessionRef		result = SessionFactory_ReturnTerminalWindowSession
								(TerminalWindow_ReturnFromMainWindow());
	
	
	return result;
}// ReturnUserRecentSession


/*!
Globally enables or disables the automatic rearrangement of
tabs in all workspaces.  If you pass "true" and rearrangement
was previously disabled, all tab positions are immediately
updated; this makes up for not having done updates to tabs
individually as they changed during the disabled period.

This is useful in certain situations, where the animation
would be distracting (e.g. at quitting time, where windows
are being systematically destroyed anyway).

\retval kSessionFactory_ResultOK
always

(4.0)
*/
SessionFactory_Result
SessionFactory_SetAutoRearrangeTabsEnabled		(Boolean		inIsEnabled)
{
	SessionFactory_Result		result = kSessionFactory_ResultOK;
	
	
	if (gAutoRearrangeTabs != inIsEnabled)
	{
		gAutoRearrangeTabs = inIsEnabled;
		if (gAutoRearrangeTabs)
		{
			MyWorkspaceList&	targetList = gWorkspaceListSortedByCreationTime();
			
			
			(fixTerminalWindowTabPositionsInWorkspace)std::for_each(targetList.begin(), targetList.end(),
																	fixTerminalWindowTabPositionsInWorkspace());
		}
	}
	return result;
}// SetAutoRearrangeTabsEnabled


/*!
Arranges for a callback to be invoked whenever something
interesting in the Session Factory changes (such as the
number of sessions open).

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "SessionFactory.h" for
			comments on what the context means for each
			type of change.

\retval kSessionFactory_ResultOK
if there are no errors

\retval kSessionFactory_ResultParameterError
if there are any problems installing the specified
listener for the specified event

\retval kSessionFactory_ResultNotInitialized if
SessionFactory_Init() has not been called yet

(3.0)
*/
SessionFactory_Result
SessionFactory_StartMonitoring	(SessionFactory_Change		inForWhatChange,
								 ListenerModel_ListenerRef	inListener)
{
	SessionFactory_Result	result = kSessionFactory_ResultOK;
	
	
	if (nullptr == gSessionFactoryStateChangeListenerModel)
	{
		result = kSessionFactory_ResultNotInitialized;
	}
	else
	{
		// add a listener to the listener model for the given setting change
		OSStatus	error = noErr;
		
		
		error = ListenerModel_AddListenerForEvent(gSessionFactoryStateChangeListenerModel, inForWhatChange, inListener);
		if (noErr != error) result = kSessionFactory_ResultParameterError;
	}
	
	return result;
}// StartMonitoring


/*!
Arranges for a callback to be invoked whenever the
specified property changes in ANY Session.

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "Session.h" for comments
			on what the context means for each type of
			change.

\retval kSessionFactory_ResultOK
if there are no errors

\retval kSessionFactory_ResultParameterError
if there are any problems installing the specified
listener for the specified event

\retval kSessionFactory_ResultNotInitialized if
SessionFactory_Init() has not been called yet

(3.0)
*/
SessionFactory_Result
SessionFactory_StartMonitoringSessions	(Session_Change				inForWhatChange,
										 ListenerModel_ListenerRef	inListener)
{
	SessionFactory_Result	result = kSessionFactory_ResultOK;
	
	
	if (nullptr == gSessionFactoryStateChangeListenerModel)
	{
		result = kSessionFactory_ResultNotInitialized;
	}
	else
	{
		// add a listener to the listener model for the given setting change
		OSStatus	error = noErr;
		
		
		error = ListenerModel_AddListenerForEvent(gSessionStateChangeListenerModel, inForWhatChange, inListener);
		if (noErr != error) result = kSessionFactory_ResultParameterError;
	}
	
	return result;
}// StartMonitoringSessions


/*!
Arranges for a callback to no longer be invoked whenever
something interesting in the Session Factory changes
(such as the number of sessions open).

IMPORTANT:	Your parameters must match those of a
			previous start-call, or the stop will fail.

(3.0)
*/
void
SessionFactory_StopMonitoring	(SessionFactory_Change		inForWhatChange,
								 ListenerModel_ListenerRef	inListener)
{
	if (nullptr != gSessionFactoryStateChangeListenerModel)
	{
		// remove this listener from the listener model for the given setting change
		ListenerModel_RemoveListenerForEvent(gSessionFactoryStateChangeListenerModel, inForWhatChange, inListener);
	}
}// StopMonitoring


/*!
Arranges for a callback to no longer be invoked whenever
the specified property changes in ANY Session.

IMPORTANT:	Your parameters must match those of a
			previous start-call, or the stop will fail.

(3.0)
*/
void
SessionFactory_StopMonitoringSessions	(Session_Change				inForWhatChange,
										 ListenerModel_ListenerRef	inListener)
{
	if (nullptr != gSessionStateChangeListenerModel)
	{
		// remove this listener from the listener model for the given setting change
		ListenerModel_RemoveListenerForEvent(gSessionStateChangeListenerModel, inForWhatChange, inListener);
	}
}// StopMonitoringSessions


#pragma mark Internal Methods
namespace {

/*!
Handles the "kEventNetEvents_SessionDataArrived" event
of the "kEventClassNetEvents_Session" class.

Invoked by Mac OS X whenever a custom “data is available
for processing” event is posted (presumably by a
preemptive thread receiving data from a process, or the
main thread receiving input from the user).

This is functionally equivalent to invoking
Session_AppendDataForProcessing(), except it is accomplished
by retrieving arguments from a Carbon Event.

(3.0)
*/
OSStatus
appendDataForProcessing		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus	result = eventNotHandledErr;
	UInt32		eventClass = GetEventClass(inEvent);
	UInt32		eventKind = GetEventKind(inEvent);
	
	
	assert(eventClass == kEventClassNetEvents_Session);
	assert(eventKind == kEventNetEvents_SessionDataArrived);
	{
		EventQueueRef	queueToNotify = nullptr;
		
		
		// retrieve the queue that needs to receive an event
		// upon completion of the data processing
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_DispatcherQueue,
														typeNetEvents_EventQueueRef, queueToNotify);
		if (noErr == result)
		{
			void*	dataPtr = nullptr;
			
			
			// retrieve the data that should be processed
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_SessionData, typeVoidPtr, dataPtr);
			if (noErr == result)
			{
				UInt32		dataSize = 0;
				
				
				// retrieve the size of the data that should be processed
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_SessionDataSize,
																typeUInt32, dataSize);
				if (noErr == result)
				{
					SessionRef		session = nullptr;
					
					
					// retrieve the session for which data has arrived for processing
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_DirectSession,
																	typeNetEvents_SessionRef, session);
					if ((noErr == result) && Session_IsValid(session))
					{
						// success!
						UInt32	unprocessedDataSize = Session_AppendDataForProcessing
														(session, REINTERPRET_CAST(dataPtr, UInt8*), dataSize);
						//UInt32	unprocessedDataSize = 0;
						//TelnetProtocol_InterpretAsCommandOrWriteToSession
						//			(session, REINTERPRET_CAST(dataPtr, UInt8*), dataSize);
						
						
						// report to the dispatching thread that data processing is complete
						{
							EventRef	dataProcessedEvent = nullptr;
							
							
							// create a Carbon Event
							result = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_Session,
													kEventNetEvents_SessionDataProcessed, GetCurrentEventTime(),
													kEventAttributeNone, &dataProcessedEvent);
							
							// attach required parameters to event, then dispatch it
							if (noErr != result) dataProcessedEvent = nullptr;
							else
							{
								// specify the session whose data was processed
								result = SetEventParameter(dataProcessedEvent, kEventParamNetEvents_DirectSession,
															typeNetEvents_SessionRef, sizeof(session), &session);
								if (noErr == result)
								{
									// specify the data that was processed
									result = SetEventParameter(dataProcessedEvent, kEventParamNetEvents_SessionData,
																typeVoidPtr, sizeof(dataPtr), dataPtr);
									if (noErr == result)
									{
										// specify the size of the data that was NOT processed
										result = SetEventParameter(dataProcessedEvent, kEventParamNetEvents_SessionDataSize,
																	typeUInt32, sizeof(unprocessedDataSize),
																	&unprocessedDataSize);
										if (noErr == result)
										{
											// send the message to the given queue
											result = PostEventToQueue(queueToNotify, dataProcessedEvent,
																		kEventPriorityStandard);
										}
									}
								}
							}
							
							// dispose of event
							if (nullptr != dataProcessedEvent) ReleaseEvent(dataProcessedEvent), dataProcessedEvent = nullptr;
						}
					}
				}
			}
		}
	}
	return result;
}// appendDataForProcessing


/*!
Notifies all listeners of Session Factory state
changes that the specified change occurred.  The
given context is passed to each listener.

IMPORTANT:	The context must make sense for the
			type of change; see "SessionFactory.h"
			for the type of context associated with
			each Session Factory state change.

(3.0)
*/
void
changeNotifyGlobal		(SessionFactory_Change	inWhatChanged,
						 void*					inContextPtr)
{
	// invoke listener callback routines appropriately, from the global listener model
	ListenerModel_NotifyListenersOfEvent(gSessionFactoryStateChangeListenerModel, inWhatChanged, inContextPtr);
}// changeNotifyGlobal


/*!
Convenience routine to call configureSessionTerminalWindowByClass()
for all typical associated context types.

Returns true only if all configurations succeed.

(4.1)
*/
Boolean
configureSessionTerminalWindow	(TerminalWindowRef			inTerminalWindow,
								 Preferences_ContextRef		inSessionContext)
{
	Boolean		result = true; // initially...
	
	
	if (false == configureSessionTerminalWindowByClass(inTerminalWindow, inSessionContext, Quills::Prefs::TERMINAL))
	{
		Console_Warning(Console_WriteLine, "unable to apply terminal preferences to window");
		result = false;
	}
	
	if (false == configureSessionTerminalWindowByClass(inTerminalWindow, inSessionContext, Quills::Prefs::FORMAT))
	{
		Console_Warning(Console_WriteLine, "unable to apply format preferences to window");
		result = false;
	}
	
	if (false == configureSessionTerminalWindowByClass(inTerminalWindow, inSessionContext, Quills::Prefs::TRANSLATION))
	{
		Console_Warning(Console_WriteLine, "unable to apply translation preferences to window");
		result = false;
	}
	
	return result;
}// configureSessionTerminalWindow


/*!
Using a *session* context, configures the specified
terminal window appropriately.

The “associated” context of the specified type is
consulted (if any), and its settings are copied.  If
no such association exists, the Default is copied.

Returns true only if successful.

(4.0)
*/
Boolean
configureSessionTerminalWindowByClass	(TerminalWindowRef			inTerminalWindow,
										 Preferences_ContextRef		inSessionContext,
										 Quills::Prefs::Class		inClass)
{
	Preferences_ContextRef		sourceContext = nullptr;
	Boolean						result = false;
	
	
	// find the appropriate settings; a session context will not
	// directly contain view settings such as fonts and colors
	// but it may contain the name of a context to use for this
	copySessionTerminalWindowConfiguration(inSessionContext, inClass, sourceContext);
	
	if (nullptr != sourceContext)
	{
		// copy recognized settings to the terminal window
		result = TerminalWindow_ReconfigureViewsInGroup
					(inTerminalWindow, kTerminalWindow_ViewGroupActive, sourceContext,
						inClass);
		Preferences_ReleaseContext(&sourceContext);
	}
	
	return result;
}// configureSessionTerminalWindowByClass


/*!
Using a *session* context, determines a related context
of the specified class that would set up a terminal
window by default.

This works by checking for the appropriate “associated”
setting in the given context, finding the context with
that name and copying its settings.  You must free your
copy of the context if it is not nullptr.  Note that
for convenience, any associated context named Default
will return a retained default context instead, which
simplifies possible issues with bindings.

See also configureSessionTerminalWindow().

(4.1)
*/
void
copySessionTerminalWindowConfiguration	(Preferences_ContextRef		inSessionContext,
										 Quills::Prefs::Class		inClass,
										 Preferences_ContextRef&	outContext)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Preferences_Tag			associationTag = 0;
	
	
	// initialize result
	outContext = nullptr;
	
	// find the appropriate tag
	switch (inClass)
	{
	case Quills::Prefs::FORMAT:
		associationTag = kPreferences_TagAssociatedFormatFavorite;
		break;
	
	case Quills::Prefs::TERMINAL:
		associationTag = kPreferences_TagAssociatedTerminalFavorite;
		break;
	
	case Quills::Prefs::TRANSLATION:
		associationTag = kPreferences_TagAssociatedTranslationFavorite;
		break;
	
	default:
		Console_Warning(Console_WriteValueFourChars, "inappropriate class for copySessionTerminalWindowConfiguration()", inClass);
		break;
	}
	
	// a session context will not directly contain view settings such as
	// fonts and colors but it may contain the name of a context to use
	{
		CFStringRef		contextName = nullptr;
		
		
		// font and color settings
		prefsResult = Preferences_ContextGetData(inSessionContext, associationTag,
													sizeof(contextName), &contextName,
													true/* search defaults too */);
		if (kPreferences_ResultOK != prefsResult)
		{
			// assume Default
			contextName = UIStrings_ReturnCopy(kUIStrings_PreferencesWindowDefaultFavoriteName);
			prefsResult = kPreferences_ResultOK;
		}
		
		if (kPreferences_ResultOK == prefsResult)
		{
			if (false == Preferences_IsContextNameInUse(inClass, contextName))
			{
				Console_Warning(Console_WriteValueCFString, "associated context not found", contextName);
				Console_Warning(Console_WriteValueFourChars, "context class", inClass);
			}
			else
			{
				// also works if context name refers to Default
				outContext = Preferences_NewContextFromFavorites(inClass, contextName);
			}
			CFRelease(contextName), contextName = nullptr;
		}
	}
}// copySessionTerminalWindowConfiguration


/*!
Internal version of SessionFactory_NewTerminalWindowUserFavorite().

TEMPORARY - The context parameters are probably overkill, since
in most cases a terminal window (new or not) needs to be
reconfigured a certain way.  This usually means that a new window
used for a new session will ultimately be “configured twice”.

(3.0)
*/
TerminalWindowRef
createTerminalWindow	(Preferences_ContextRef		inTerminalInfoOrNull,
						 Preferences_ContextRef		inFontInfoOrNull,
						 Preferences_ContextRef		inTranslationInfoOrNull,
						 Boolean					inNoStagger)
{
	TerminalWindowRef		result = nullptr;
	Boolean					inCocoaPreferred = DebugInterface_UseCocoaTerminalWindowsForNewSessions(); // TEMPORARY
	
	
	// create a new terminal window to house the session
	if (inCocoaPreferred)
	{
		Console_WriteLine("forcing new terminal window to use Cocoa test implementation");
		result = TerminalWindow_New(inTerminalInfoOrNull, inFontInfoOrNull, inTranslationInfoOrNull, inNoStagger);
	}
	else
	{
		result = TerminalWindow_NewCarbonLegacy(inTerminalInfoOrNull, inFontInfoOrNull, inTranslationInfoOrNull, inNoStagger);
	}
	
	if (nullptr != result)
	{
		startTrackingTerminalWindow(result);
	}
	return result;
}// createTerminalWindow


/*!
Use this instead of calling Workspace_New() directly, so that
any new workspaces are properly tracked.

(4.0)
*/
Workspace_Ref
createWorkspace ()
{
	Workspace_Ref	result = Workspace_New();
	
	
	gWorkspaceListSortedByCreationTime().push_back(result);
	
	return result;
}// createWorkspace


/*!
Shows a terminal window, putting it in front of all other
terminal windows and forcing its contents to be rendered.

If a workspace context is given, then any workspace settings
(such as the presence of tabs) are read from this; otherwise,
user defaults are used.  In addition, if a specific window
index is given, window-specific workspace settings (such as
the window location on screen) will also be used.

Returns "true" unless the window was not able to display for
some reason.

(4.0)
*/
Boolean
displayTerminalWindow	(TerminalWindowRef			inTerminalWindow,
						 Preferences_ContextRef		inWorkspaceOrNull,
						 UInt16						inWindowIndexInWorkspaceOrZero)
{
	Preferences_Index const		kWindowIndex = STATIC_CAST(inWindowIndexInWorkspaceOrZero, Preferences_Index);
	HIWindowRef					legacyCarbonWindow = (TerminalWindow_IsLegacyCarbon(inTerminalWindow)
														? TerminalWindow_ReturnLegacyCarbonWindow(inTerminalWindow)
														: nullptr); 
	NSWindow*					cocoaWindow = TerminalWindow_ReturnNSWindow(inTerminalWindow);
	Boolean						result = true;
	
	
	if (nil == cocoaWindow)
	{
		result = false;
	}
	else
	{
		TerminalWindow_Result	terminalWindowResult = kTerminalWindow_ResultOK;
		TerminalViewRef			view = nullptr;
		Workspace_Ref			targetWorkspace = nullptr;
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		Boolean					useTabs = false;
		
		
		// set the window location and size appropriately
		if ((0 != kWindowIndex) && (nullptr != inWorkspaceOrNull))
		{
			HIRect		frameBounds;
			
			
			prefsResult = Preferences_ContextGetData(inWorkspaceOrNull, Preferences_ReturnTagVariantForIndex
																		(kPreferences_TagIndexedWindowFrameBounds,
																			kWindowIndex),
														sizeof(frameBounds), &frameBounds, false/* search defaults */);
			if (kPreferences_ResultOK == prefsResult)
			{
				// UNIMPLEMENTED - check kPreferences_TagIndexedWindowScreenBounds too, and
				// if the current display size disagrees, adjust the window location somehow
				
				// IMPORTANT: the boundaries are not guaranteed to specify the width or height
				if (legacyCarbonWindow)
				{
					MoveWindowStructure(legacyCarbonWindow, STATIC_CAST(frameBounds.origin.x, SInt16), STATIC_CAST(frameBounds.origin.y, SInt16));
				}
				else
				{
					NSScreen*	windowScreen = cocoaWindow.screen;
					
					
					if (nil == windowScreen)
					{
						windowScreen = [NSScreen mainScreen];
					}
					
					frameBounds.size.width = NSWidth(cocoaWindow.frame);
					frameBounds.size.height = NSWidth(cocoaWindow.frame);
					
					// flip coordinates
					frameBounds.origin.y = (windowScreen.frame.origin.y + NSHeight(windowScreen.frame) - frameBounds.origin.y - frameBounds.size.height);
					
					[cocoaWindow setFrame:NSRectFromCGRect(frameBounds) display:NO];
				}
			}
		}
		
		// set the window title, if one is defined
		if ((0 != kWindowIndex) && (nullptr != inWorkspaceOrNull))
		{
			CFStringRef		titleCFString = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inWorkspaceOrNull, Preferences_ReturnTagVariantForIndex
																		(kPreferences_TagIndexedWindowTitle,
																			kWindowIndex),
														sizeof(titleCFString), &titleCFString, false/* search defaults */);
			if (kPreferences_ResultOK == prefsResult)
			{
				if (CFStringGetLength(titleCFString) > 0)
				{
					TerminalWindow_SetWindowTitle(inTerminalWindow, titleCFString);
				}
				CFRelease(titleCFString), titleCFString = nullptr;
			}
		}
		
		// figure out if this window should have a tab and be arranged
		if (nullptr == inWorkspaceOrNull)
		{
			// no specific source for workspace preferences; therefore, the appearance
			// and arrangement of the new window may depend on the frontmost window,
			// or the user may have requested a new workspace
			if (EventLoop_IsOptionKeyDown())
			{
				// if the user requests a new workspace, ignore any frontmost window
				// or active workspace, and choose the tabbed appearance from the
				// Default preferences; and, if that default is to use tabs, create
				// a new workspace for tabs (otherwise, the tab would glue itself
				// to the active workspace of tabs)
				prefsResult = Preferences_ContextGetData(nullptr/* context */, kPreferences_TagArrangeWindowsUsingTabs,
															sizeof(useTabs), &useTabs,
															true/* search defaults */, nullptr/* actual size */);
				if (prefsResult != kPreferences_ResultOK)
				{
					useTabs = false;
				}
				
				if (useTabs)
				{
					// override the default behavior of using the active set of tabs
					targetWorkspace = createWorkspace();
				}
			}
			else
			{
				// no request for new workspace, so use the existing one; copy the
				// style of the previously-frontmost terminal window, if any (no
				// need to set a workspace in this case, since it will default to
				// the active workspace, below)
				TerminalWindowRef	previousTerminalWindow = TerminalWindow_ReturnFromMainWindow();
				
				
				if (nullptr != previousTerminalWindow)
				{
					useTabs = TerminalWindow_IsTab(previousTerminalWindow);
				}
				else
				{
					prefsResult = Preferences_ContextGetData(nullptr/* context */, kPreferences_TagArrangeWindowsUsingTabs,
																sizeof(useTabs), &useTabs,
																true/* search defaults */, nullptr/* actual size */);
					if (prefsResult != kPreferences_ResultOK)
					{
						useTabs = false;
					}
				}
			}
		}
		else
		{
			// when given a target workspace, the choice of tabs or not comes from that
			// specific collection, and not the default (unless the given workspace
			// completely lacks the setting, of course)
			prefsResult = Preferences_ContextGetData(inWorkspaceOrNull, kPreferences_TagArrangeWindowsUsingTabs,
														sizeof(useTabs), &useTabs,
														true/* search defaults */, nullptr/* actual size */);
			if (prefsResult != kPreferences_ResultOK)
			{
				useTabs = false;
			}
			
			// also, when given a target workspace, assume that tabs should appear on
			// their own, even if they could otherwise fold into an existing stack
			if ((useTabs) && (1 == kWindowIndex))
			{
				// override the default behavior of using the active set of tabs
				targetWorkspace = createWorkspace();
			}
		}
		
		// display the window, and give it the appropriate ordering (depends on tab setting)
		if (useTabs)
		{
			MyWorkspaceList&	targetList = gWorkspaceListSortedByCreationTime();
			
			
			if (nullptr == targetWorkspace)
			{
				targetWorkspace = returnActiveWorkspace();
			}
			
			// IMPORTANT: Although you should add a window to a workspace before
			// it is visible, the window MUST be visible before you can give it a
			// tabbed appearance.  This is due to the tab implementation being a
			// drawer, which refuses to associate itself with an invisible window!
			Workspace_AddTerminalWindow(targetWorkspace, inTerminalWindow);
			TerminalWindow_SetVisible(inTerminalWindow, true);
			
			// tab appearance and ordering are implied when using the new Cocoa
			// window tab mechanism; only Carbon windows need additional steps
			if (TerminalWindow_IsLegacyCarbon(inTerminalWindow))
			{
				UNUSED_RETURN(OSStatus)TerminalWindow_SetTabAppearance(inTerminalWindow, true);
				if (gAutoRearrangeTabs)
				{
					(fixTerminalWindowTabPositionsInWorkspace)std::for_each(targetList.begin(), targetList.end(),
																			fixTerminalWindowTabPositionsInWorkspace());
				}
			}
		}
		else
		{
			TerminalWindow_SetVisible(inTerminalWindow, true);
		}
		TerminalWindow_Select(inTerminalWindow);
		
		// focus the first view of the first tab
		terminalWindowResult = TerminalWindow_GetViewsInGroup(inTerminalWindow, kTerminalWindow_ViewGroupEverything, 1/* array length */,
																&view, nullptr/* actual count */);
		if (terminalWindowResult == kTerminalWindow_ResultOK)
		{
			TerminalView_FocusForUser(view);
		}
	}
	return result;
}// displayTerminalWindow


/*!
Internal version of SessionFactory_ForEachSession(), except
it operates on the specific list given.

(2017.09)
*/
void
forEachSessionInListDo		(SessionList const&				inList,
							 SessionFactory_SessionBlock	inBlock)
{
	Boolean		stopFlag = false;
	
	
	// traverse the list
	for (auto sessionRef : inList)
	{
		inBlock(sessionRef, stopFlag);
		if (stopFlag)
		{
			break;
		}
	}
}// forEachSessionInListDo


/*!
Internal version of SessionFactory_ForEachTerminalWindow(),
except it operates on the specific list given.

(2017.09)
*/
void
forEachTerminalWindowInListDo	(TerminalWindowList const&				inList,
								 SessionFactory_TerminalWindowBlock		inBlock)
{
	Boolean		stopFlag = false;
	
	
	// traverse the list
	for (auto terminalWindowRef : inList)
	{
		inBlock(terminalWindowRef, stopFlag);
		if (stopFlag)
		{
			break;
		}
	}
}// forEachTerminalWindowInListDo


/*!
Responds to a close of a New Session Dialog by creating a
new session.

(4.1)
*/
void
handleNewSessionDialogClose		(GenericDialog_Ref		inDialogThatClosed,
								 Boolean				inOKButtonPressed)
{
	SessionFactory_NewSessionDataObject*	dataObject = REINTERPRET_CAST(GenericDialog_ReturnImplementation(inDialogThatClosed), SessionFactory_NewSessionDataObject*);
	PrefPanelSessions_ResourceViewManager*	viewMgr = STATIC_CAST(GenericDialog_ReturnViewManager(inDialogThatClosed),
																	PrefPanelSessions_ResourceViewManager*);
	
	
	assert([viewMgr isKindOfClass:[PrefPanelSessions_ResourceViewManager class]]);
	
	// remove observers added by SessionFactory_DisplayUserCustomizationUI()
	// (the parameters should be consistent)
	[viewMgr removeObserver:dataObject forKeyPath:@"formatFavorite.currentValueDescriptor"];
	[viewMgr removeObserver:dataObject forKeyPath:@"terminalFavorite.currentValueDescriptor"];
	
	if (inOKButtonPressed)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		CFArrayRef				argumentListCFArray = nullptr;
		
		
		// create a session; this information is expected to be defined by
		// the panel as the user makes edits in the user interface
		prefsResult = Preferences_ContextGetData(dataObject.temporaryContext, kPreferences_TagCommandLine,
													sizeof(argumentListCFArray), &argumentListCFArray);
		if (kPreferences_ResultOK != prefsResult)
		{
			Sound_StandardAlert();
			Console_Warning(Console_WriteLine, "unexpected problem retrieving the command line from the panel!");
		}
		else
		{
			// dump the process into the terminal window associated with the dialog
			SessionRef		session = nullptr;
			
			
			// since the terminal window is already displayed when the UI is presented,
			// its workspace-window-specific settings have already been applied;
			// therefore, no specific workspace or window index should ever be given here
			// TEMPORARY: the reconfigure-terminal argument is "true" for now, because
			// update-on-menu-selection is not implemented for all pop-up menus of
			// associated favorites (it only works for Formats; to work for others, new
			// command IDs must be implemented in TerminalWindow.mm or Commands.mm);
			// the goal is to eventually auto-update windows as soon as items are chosen
			// from menus, so that no reconfiguration is necessary when the session spawns
			session = SessionFactory_NewSessionArbitraryCommand
						(dataObject.terminalWindow, argumentListCFArray, dataObject.temporaryContext,
							true/* reconfigure terminal */, nullptr/* workspace context */, 0/* window index */);
			if (nullptr == session)
			{
				Console_Warning(Console_WriteLine, "failed to create new session");
				Sound_StandardAlert();
			}
			CFRelease(argumentListCFArray), argumentListCFArray = nullptr;
			
			if (TerminalWindow_IsLegacyCarbon(dataObject.terminalWindow))
			{
				// in this case the new window must be explicitly activated to prevent the
				// window from staying in the background; and, notably, the mechanism for
				// selecting the window MUST be the Carbon API call SelectWindow()
				// (otherwise system-handled commands such as Close and Minimize have no
				// effect on the new window; it is not clear why this is an issue)
				SelectWindow(TerminalWindow_ReturnLegacyCarbonWindow(dataObject.terminalWindow));
			}
		}
	}
	else
	{
		// immediately destroy the parent terminal window
		if (nullptr != dataObject.terminalWindow)
		{
			TerminalWindowRef	localCopy = dataObject.terminalWindow;
			
			
			stopTrackingTerminalWindow(localCopy);
			TerminalWindow_Dispose(&localCopy), dataObject.terminalWindow = nullptr;
		}
	}
	
	[dataObject release], GenericDialog_SetImplementation(inDialogThatClosed, nullptr);
}// handleNewSessionDialogClose


/*!
Creates a new session based on a command ID (such as from a
menu); or, arranges for the appropriate user interface to
appear, in the case of a custom new session.  Returns true only
if successful.

If the workspace is defined, then workspace-wide settings such
as the use of tabs can be applied to the specified terminal
window.  In addition, if the window index is nonzero, any
window-specific settings (such as location on screen) can also
be applied.

(4.0)
*/
Boolean
newSessionFromCommand	(TerminalWindowRef			inTerminalWindow,
						 UInt32						inCommandID,
						 Preferences_ContextRef		inWorkspaceOrNull,
						 UInt16						inWindowIndexInWorkspaceOrZero)
{
	Boolean		result = false;
	
	
	switch (inCommandID)
	{
	case kCommandNewSessionDefaultFavorite:
		{
			Preferences_ContextRef		sessionContext = nullptr;
			Preferences_Result			prefsResult = Preferences_GetDefaultContext
														(&sessionContext, Quills::Prefs::SESSION);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				sessionContext = nullptr;
			}
			
			// finally, create the session from the specified context
			if (nullptr != sessionContext)
			{
				SessionRef		newSession = SessionFactory_NewSessionUserFavorite
												(inTerminalWindow, sessionContext, inWorkspaceOrNull,
													inWindowIndexInWorkspaceOrZero);
				
				
				if (nullptr != newSession)
				{
					// success!
					result = true;
				}
			}
		}
		break;
	
	case kCommandNewSessionDialog:
		result = SessionFactory_DisplayUserCustomizationUI(inTerminalWindow, inWorkspaceOrNull,
															inWindowIndexInWorkspaceOrZero);
		if (false == result)
		{
			// some kind of problem
			Sound_StandardAlert();
			Console_WriteLine("unexpected problem displaying custom session dialog!");
		}
		break;
	
	case kCommandNewSessionLoginShell:
	case kCommandNewSessionShell:
		{
			SessionRef		newSession = nullptr;
			
			
			// create a shell
			if (kCommandNewSessionLoginShell == inCommandID)
			{
				newSession = SessionFactory_NewSessionLoginShell(inTerminalWindow, inWorkspaceOrNull,
																	inWindowIndexInWorkspaceOrZero);
			}
			else
			{
				newSession = SessionFactory_NewSessionDefaultShell(inTerminalWindow, inWorkspaceOrNull,
																	inWindowIndexInWorkspaceOrZero);
			}
			
			if (nullptr != newSession)
			{
				// success!
				result = true;
			}
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// newSessionFromCommand


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for commands that create new sessions.

(3.1)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// execute a command selected from a menu
				switch (received.commandID)
				{
				case kCommandNewSessionDefaultFavorite:
				case kCommandNewSessionDialog:
				case kCommandNewSessionLoginShell:
				case kCommandNewSessionShell:
					{
						Preferences_Result			prefsResult = kPreferences_ResultOK;
						Preferences_ContextRef		defaultSession = nullptr;
						Preferences_ContextRef		workspaceContext = nullptr;
						TerminalWindowRef			terminalWindow = createTerminalWindow();
						
						
						prefsResult = Preferences_GetDefaultContext(&defaultSession, Quills::Prefs::SESSION);
						if (kPreferences_ResultOK != prefsResult)
						{
							Console_Warning(Console_WriteValue, "failed to find default session settings for new terminal window; error", prefsResult);
						}
						else
						{
							if (false == configureSessionTerminalWindow(terminalWindow, defaultSession))
							{
								Console_Warning(Console_WriteValueFourChars, "terminal window could not apply associated-context preferences for command ID", received.commandID);
							}
						}
						
						Boolean		launchOK = newSessionFromCommand
												(terminalWindow, received.commandID, workspaceContext,
													0/* window index */);
						
						
						if (launchOK)
						{
							result = noErr;
						}
						else
						{
							// UNIMPLEMENTED - report any problems to the user!!!
							result = eventNotHandledErr;
						}
					}
					break;
				
				case kCommandRestoreWorkspaceDefaultFavorite:
					{
						std::string					preferredName = Quills::Base::_initial_workspace_name();
						CFRetainRelease				asCFString(CFStringCreateWithCString(kCFAllocatorDefault,
																							preferredName.c_str(),
																							kCFStringEncodingUTF8),
																CFRetainRelease::kAlreadyRetained);
						Preferences_ContextRef		workspaceContext = Preferences_IsContextNameInUse
																		(Quills::Prefs::WORKSPACE, asCFString.returnCFStringRef())
																		? Preferences_NewContextFromFavorites
																			(Quills::Prefs::WORKSPACE, asCFString.returnCFStringRef())
																		: nullptr;
						Boolean						releaseContext = (nullptr != workspaceContext);
						
						
						if (nullptr == workspaceContext)
						{
							// preferred one does not exist; find the Default, instead
							Preferences_Result		prefsResult = Preferences_GetDefaultContext
																	(&workspaceContext, Quills::Prefs::WORKSPACE);
							
							
							if (kPreferences_ResultOK == prefsResult)
							{
								// default contexts are borrowed, not allocated or retained
								releaseContext = false;
							}
							else
							{
								// error
								workspaceContext = nullptr;
							}
						}
						
						// finally, create the session from the specified context
						if (nullptr == workspaceContext) result = eventNotHandledErr;
						else
						{
							Boolean		launchedOK = SessionFactory_NewSessionsUserFavoriteWorkspace(workspaceContext);
							
							
							if (launchedOK) result = noErr;
							else result = eventNotHandledErr;
						}
						
						// report any errors to the user
						if (noErr != result)
						{
							// UNIMPLEMENTED!!!
							Console_Warning(Console_WriteLine, "failed to launch startup workspace");
							Sound_StandardAlert();
						}
						
						if ((releaseContext) && (nullptr != workspaceContext))
						{
							Preferences_ReleaseContext(&workspaceContext);
						}
					}
					break;
				
				default:
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
Embellishes "kEventWindowActivated" of "kEventClassWindow" by
keeping track of the session for the most-recently-activated
window.

Note that the recent-session global can also change in other
ways, e.g. at the time a session is constructed.

(4.0)
*/
OSStatus
receiveWindowActivated	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowActivated);
	
	// determine the window that was activated
	{
		HIWindowRef		activatedWindow = nullptr;
		OSStatus		error = CarbonEventUtilities_GetEventParameter
								(inEvent, kEventParamDirectObject, typeWindowRef, activatedWindow);
		
		
		if (noErr == error)
		{
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(activatedWindow);
			
			
			if (nullptr != terminalWindow)
			{
				SessionRef		newSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
				
				
				if (newSession != gSessionFactoryRecentlyActiveSession)
				{
					// overwrite the value; assume that if there was a session
					// active, it would have fired a deactivation event prior
					// to being cleared
					gSessionFactoryRecentlyActiveSession = newSession;
					changeNotifyGlobal(kSessionFactory_ChangeActivatingSession, newSession);
				}
			}
		}
	}
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveWindowActivated


/*!
Embellishes "kEventWindowDeactivated" of "kEventClassWindow" by
sending an appropriate event notification for the corresponding
session.

(4.0)
*/
OSStatus
receiveWindowDeactivated	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowDeactivated);
	
	// determine if a session window was deactivated
	{
		HIWindowRef		deactivatedWindow = nullptr;
		OSStatus		error = CarbonEventUtilities_GetEventParameter
								(inEvent, kEventParamDirectObject, typeWindowRef, deactivatedWindow);
		
		
		if (noErr == error)
		{
			TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(deactivatedWindow);
			
			
			if (nullptr != terminalWindow)
			{
				SessionRef		oldSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
				
				
				if (nullptr != oldSession)
				{
					// clear the current session completely; assume that it may be
					// quickly restored by an activation event in another window
					changeNotifyGlobal(kSessionFactory_ChangeDeactivatingSession, oldSession);
					gSessionFactoryRecentlyActiveSession = nullptr;
				}
			}
		}
	}
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveWindowDeactivated


/*!
Returns the most appropriate workspace for a new terminal
window.  If no workspaces exist, one is created; otherwise,
the workspace of the most recently active terminal window
is used.

(3.1)
*/
Workspace_Ref
returnActiveWorkspace ()
{
	Workspace_Ref	result = nullptr;
	
	
	if (gWorkspaceListSortedByCreationTime().empty())
	{
		result = createWorkspace();
	}
	else
	{
		SessionRef		activeSession = SessionFactory_ReturnUserRecentSession();
		
		
		if (nullptr == activeSession)
		{
			activeSession = InfoWindow_ReturnSelectedSession();
		}
		
		if (nullptr != activeSession)
		{
			TerminalWindowRef const		kActiveWindow = Session_ReturnActiveTerminalWindow(activeSession);
			MyWorkspaceList&			targetList = gWorkspaceListSortedByCreationTime();
			auto						toWorkspace = std::find_if(targetList.begin(), targetList.end(),
																	workspaceContainsWindow(kActiveWindow));
			
			
			if (targetList.end() != toWorkspace)
			{
				result = *toWorkspace;
			}
		}
		
		if (nullptr == result)
		{
			// no recent workspace; perhaps the user toggled the preference
			// to turn on tabs, and an untabbed window is still open and
			// active; no problem, choose the last workspace
			result = gWorkspaceListSortedByCreationTime().back();
		}
	}
	
	return result;
}// returnActiveWorkspace


/*!
Invoked whenever a monitored property of any session
is changed.  This routine responds to changes by
notifying interested parties.

(3.0)
*/
void
sessionChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inWhatChanged,
					 void*					inEventContextPtr,
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	// invoke listener callback routines appropriately, from the global listener model
	ListenerModel_NotifyListenersOfEvent(gSessionStateChangeListenerModel, inWhatChanged, inEventContextPtr);
}// sessionChanged


/*!
Invoked when a session changes state, this routine
updates the internal list as sessions are destroyed.

(3.1)
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
			case kSession_StateImminentDisposal:
				{
					TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(session);
					
					
					// final state; delete the session from all internal lists and maps that have it
					stopTrackingSession(session);
					
					// end kiosk mode no matter what terminal is disconnecting
					// TEMPORARY: fix this...why does it seem sometimes this event does not happen?!?
					if ((nullptr != terminalWindow) && TerminalWindow_IsFullScreen(terminalWindow))
					{
						Commands_ExecuteByIDUsingEvent(kCommandFullScreenToggle);
					}
				}
				break;
			
			case kSession_StateBrandNew:
			case kSession_StateInitialized:
				// ignore
				break;
			
			case kSession_StateActiveUnstable:
				// initialize the session user-defined title to match whatever
				// title the window is given when it opens
				{
					TerminalWindowRef	terminalWindow = Session_ReturnActiveTerminalWindow(session);
					Boolean				foundTitle = false;
					
					
					if (nullptr != terminalWindow)
					{
						CFStringRef		existingTitleCFString = nullptr;
						
						
						TerminalWindow_CopyWindowTitle(terminalWindow, existingTitleCFString);
						if (nullptr != existingTitleCFString)
						{
							if (CFStringGetLength(existingTitleCFString) > 0)
							{
								Session_SetWindowUserDefinedTitle(session, existingTitleCFString);
								foundTitle = true;
							}
							CFRelease(existingTitleCFString), existingTitleCFString = nullptr;
						}
					}
					
					if (false == foundTitle)
					{
						// by default, use the resource location string
						Session_SetWindowUserDefinedTitle(session, Session_ReturnResourceLocationCFString(session));
					}
				}
				break;
			
			case kSession_StateActiveStable:
			case kSession_StateDead:
			default:
				// ignore
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
Handles "kEventNetEvents_SessionSetState" of
"kEventClassNetEvents_Session".

Invoked by Mac OS X whenever a custom “set session
state” event is posted (presumably by a preemptive
thread running that session).

This is functionally equivalent to invoking
Session_SetState(), except it is accomplished by
retrieving arguments from a Carbon Event.

(3.0)
*/
OSStatus
setSessionState		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inUserData))
{
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	OSStatus		result = eventNotHandledErr;
	
	
	assert((kEventClass == kEventClassNetEvents_Session) &&
			(kEventKind == kEventNetEvents_SessionSetState));
	{
		SessionRef		session = nullptr;
		
		
		// retrieve the session whose state should change; since this
		// is captured asynchronously, it is possible that the session
		// has become invalid since the time the event was first fired,
		// so ensure the session is still in use
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_DirectSession,
														typeNetEvents_SessionRef, session);
		if ((noErr == result) && Session_IsValid(session))
		{
			Session_State		newState = kSession_StateBrandNew;
			
			
			// retrieve the state the session should end up in
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_NewSessionState,
															typeNetEvents_SessionState, newState);
			if (noErr == result)
			{
				// success!
				Session_SetState(session, newState);
			}
		}
	}
	return result;
}// setSessionState


/*!
Invoke this routine from every factory method, to
start tracking the new SessionRef in this module.
See also stopTrackingSession().

(3.1)
*/
void
startTrackingSession	(SessionRef				inSession,
						 TerminalWindowRef		inTerminalWindow)
{
	//Console_WriteLine("NEW SESSION CONSTRUCTED");
	
	// ensure that changes to the specified session invoke the Session Factory’s notifier
	Session_StartMonitoring(inSession, kSession_AllChanges, gSessionChangeListenerRef);
	
	// start listening for data; tell the session where to dump
	// locally echoed or returned data, and tell the terminal
	// where to return data (rarely needed, but used for things
	// like status reports)
	{
		TerminalScreenRef	screen = TerminalWindow_ReturnScreenWithFocus(inTerminalWindow);
		
		
	#if 1
		Session_AddDataTarget(inSession, kSession_DataTargetStandardTerminal, screen);
	#else
		Session_AddDataTarget(inSession, kSession_DataTargetDumbTerminal, screen); // debug - see it all!
	#endif
		Terminal_SetListeningSession(screen, inSession);
	}
	
	// append the specified session to the creation-order list
	gSessionListSortedByCreationTime().push_back(inSession);
	assert(false == gSessionListSortedByCreationTime().empty());
	assert(inSession == gSessionListSortedByCreationTime().back());
	
	// attach the window (this triggers other things as well)
	Session_SetTerminalWindow(inSession, inTerminalWindow);
	
	// remember that this terminal window is used for this session
	gTerminalWindowToSessions().insert(std::make_pair(inTerminalWindow, inSession));
	
	// activate the session (which also notifies listeners)
	Session_SetState(inSession, kSession_StateActiveUnstable);
	
	// notify global listeners that the session is now in the list
	changeNotifyGlobal(kSessionFactory_ChangeNewSessionCount, nullptr/* context */);
	
	// reset this value; it can change if another window is activated later
	gSessionFactoryRecentlyActiveSession = inSession;
	changeNotifyGlobal(kSessionFactory_ChangeActivatingSession, inSession/* context */);
}// startTrackingSession


/*!
Invoke this routine from every factory method, to
start tracking the new TerminalWindowRef in this
module.  See also stopTrackingTerminalWindow().

(3.1)
*/
void
startTrackingTerminalWindow		(TerminalWindowRef		inTerminalWindow)
{
	// append the specified session to the creation-order list
	gTerminalWindowListSortedByCreationTime().push_back(inTerminalWindow);
	assert(false == gTerminalWindowListSortedByCreationTime().empty());
	assert(inTerminalWindow == gTerminalWindowListSortedByCreationTime().back());
}// startTrackingTerminalWindow


/*!
Invoke this routine when a session is being destroyed,
to undo the effects of startTrackingSession().

(3.1)
*/
void
stopTrackingSession		(SessionRef		inSession)
{
	//Console_WriteLine("SESSION DESTRUCTED");
	
	// ensure that changes to the specified session no longer invoke the Session Factory’s notifier
	Session_StopMonitoring(inSession, kSession_AllChanges, gSessionChangeListenerRef);
	
	// remove the specified session from the creation-order list;
	// the idea here is to shuffle the list so that the given
	// session is at the end, and then all matching items are just
	// erased off the end of the list
	{
		SessionList&	targetList = gSessionListSortedByCreationTime();
		
		
		targetList.erase(std::remove(targetList.begin(), targetList.end(), inSession),
							targetList.end());
		assert(targetList.end() == std::find(targetList.begin(), targetList.end(), inSession));
	}
	
	// break various terminal window associations
	{
		TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(inSession);
		
		
		if (nullptr != terminalWindow)
		{
			// determine if the session of this terminal window is in use
			// by any other sessions; if not, stop tracking the terminal
			// window as well!
			SessionList&	targetList = gSessionListSortedByCreationTime();
			auto			sessionIterator = std::find_if(targetList.begin(), targetList.end(),
															[=](SessionRef s)
															{
																return (Session_ReturnActiveTerminalWindow(s) == terminalWindow);
															});
			
			
			if (targetList.end() == sessionIterator)
			{
				stopTrackingTerminalWindow(terminalWindow);
			}
			
			// forget the specific association of this terminal window and session
			gTerminalWindowToSessions().erase(terminalWindow);
		}
	}
	
	// reset this value; it can change if another window is activated later
	if (inSession == gSessionFactoryRecentlyActiveSession)
	{
		changeNotifyGlobal(kSessionFactory_ChangeDeactivatingSession, inSession/* context */);
		gSessionFactoryRecentlyActiveSession = nullptr;
	}
	
	// notify listeners that this has occurred
	changeNotifyGlobal(kSessionFactory_ChangeNewSessionCount, nullptr/* context */);
}// stopTrackingSession


/*!
Invoke this routine when a terminal window is being destroyed,
to undo the effects of startTrackingTerminalWindow().

(3.1)
*/
void
stopTrackingTerminalWindow		(TerminalWindowRef		inTerminalWindow)
{
	// remove this window from any workspaces that contain it,
	// and shuffle tab order across workspaces accordingly
	MyWorkspaceList&	workspaceList = gWorkspaceListSortedByCreationTime();
	
	
	std::for_each(workspaceList.begin(), workspaceList.end(),
					[=](Workspace_Ref wsp) { Workspace_RemoveTerminalWindow(wsp, inTerminalWindow); });
	
	if (gAutoRearrangeTabs)
	{
		(fixTerminalWindowTabPositionsInWorkspace)std::for_each(workspaceList.begin(), workspaceList.end(),
																fixTerminalWindowTabPositionsInWorkspace());
	}
	
	// remove the specified window from the creation-order list;
	// the idea here is to shuffle the list so that the given
	// window is at the end, and then all matching items are just
	// erased off the end of the list
	TerminalWindowList&		targetList = gTerminalWindowListSortedByCreationTime();
	targetList.erase(std::remove(targetList.begin(), targetList.end(), inTerminalWindow),
						targetList.end());
	assert(targetList.end() == std::find(targetList.begin(), targetList.end(), inTerminalWindow));
}// stopTrackingTerminalWindow

} // anonymous namespace


#pragma mark -
@implementation SessionFactory_NewSessionDataObject //{


@synthesize disableObservers = _disableObservers;
@synthesize temporaryContext = _temporaryContext;
@synthesize terminalWindow = _terminalWindow;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithSessionContext:(Preferences_ContextRef)		aSessionContext
terminalWindow:(TerminalWindowRef)					aTerminalWindow
{
	self = [super init];
	if (nil != self)
	{
		_disableObservers = NO;
		_terminalWindow = aTerminalWindow;
		_temporaryContext = Preferences_NewContext(Quills::Prefs::SESSION);
		
		// make a locally-modifiable copy of the relevant
		// original data, to store user changes temporarily
		Preferences_TagSetRef	tagSet = PrefPanelSessions_NewResourcePaneTagSet();
		Preferences_Result		prefsResult = Preferences_ContextCopy
												(aSessionContext, _temporaryContext, tagSet);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "failed to copy default session preferences for New Session sheet!");
		}
		Preferences_ReleaseTagSet(&tagSet);
	}
	return self;
}// initWithSessionContext:terminalWindow:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	Preferences_ReleaseContext(&_temporaryContext);
	[super dealloc];
}// dealloc


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to bound values by updating the parent
terminal window.  For example, if the user chooses a new
Format, the colors of the window may change accordingly;
and if the user chooses a new Terminal, the size may
change.

(4.1)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
	BOOL	handled = NO;
	
	
	// WARNING: this is relying on the “probably unique” nature of the
	// key paths being observed, and ignoring the possibility that the
	// superclass may be observing the same thing in a different context
	// (a strictly-correct approach is for the original registration call
	// to set a unique context that would then be checked here)
	//if ([self observerArray:self.registeredObservers containsContext:aContext])
	{
		handled = YES;
		
		if (NO == self.disableObservers)
		{
			if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
			{
				if ([aKeyPath isEqualToString:@"formatFavorite.currentValueDescriptor"])
				{
					UNUSED_RETURN(Boolean)configureSessionTerminalWindowByClass(self.terminalWindow, self.temporaryContext,
																				Quills::Prefs::FORMAT);
				}
				else if ([aKeyPath isEqualToString:@"terminalFavorite.currentValueDescriptor"])
				{
					UNUSED_RETURN(Boolean)configureSessionTerminalWindowByClass(self.terminalWindow, self.temporaryContext,
																				Quills::Prefs::TERMINAL);
				}
				else
				{
					Console_Warning(Console_WriteValueCFString, "valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
				}
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


@end //} SessionFactory_NewSessionDataObject


@implementation SessionFactory_SessionWindowWatcher //{


#pragma mark Initializers


/*!
Designated initializer.

(2018.03)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		[self whenObject:nil postsNote:NSWindowDidBecomeMainNotification
							performSelector:@selector(windowDidBecomeMain:)];
		[self whenObject:nil postsNote:NSWindowDidResignMainNotification
							performSelector:@selector(windowDidResignMain:)];
	}
	return self;
}// init


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


#pragma mark Notifications


/*!
When a window becomes “main”, this responds by updating the
local active-session information.

(2018.03)
*/
- (void)
windowDidBecomeMain:(NSNotification*)	aNotification
{
	if (NO == [aNotification.object isKindOfClass:NSWindow.class])
	{
		Console_Warning(Console_WriteLine, "expected notification object to be an NSWindow!");
	}
	else
	{
		NSWindow*			asWindow = STATIC_CAST(aNotification.object, NSWindow*);
		TerminalWindowRef	terminalWindow = [asWindow terminalWindowRef];
		
		
		if (nullptr != terminalWindow)
		{
			SessionRef		newSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
			
			
			if (newSession != gSessionFactoryRecentlyActiveSession)
			{
				// overwrite the value; assume that if there was a session
				// active, it would have fired a deactivation event prior
				// to being cleared
				gSessionFactoryRecentlyActiveSession = newSession;
				changeNotifyGlobal(kSessionFactory_ChangeActivatingSession, newSession);
			}
		}
	} 
}// windowDidBecomeMain:


/*!
When a window resigns “main”, this responds by updating the
local active-session information.

(2018.03)
*/
- (void)
windowDidResignMain:(NSNotification*)		aNotification
{
	if (NO == [aNotification.object isKindOfClass:NSWindow.class])
	{
		Console_Warning(Console_WriteLine, "expected notification object to be an NSWindow!");
	}
	else
	{
		NSWindow*			asWindow = STATIC_CAST(aNotification.object, NSWindow*);
		TerminalWindowRef	terminalWindow = [asWindow terminalWindowRef];
		
		
		if (nullptr != terminalWindow)
		{
			SessionRef		oldSession = SessionFactory_ReturnTerminalWindowSession(terminalWindow);
			
			
			if (nullptr != oldSession)
			{
				// clear the current session completely; assume that it may be
				// quickly restored by an activation event in another window
				changeNotifyGlobal(kSessionFactory_ChangeDeactivatingSession, oldSession);
				gSessionFactoryRecentlyActiveSession = nullptr;
			}
		}
	} 
}// windowDidResignMain:


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
