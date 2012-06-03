/*!	\file SessionFactory.cp
	\brief Construction mechanism for sessions (terminal
	windows that run local or remote processes).
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

#include "SessionFactory.h"
#include <UniversalDefines.h>

// standard-C includes
#include <cctype>
#include <cstring>

// standard-C++ includes
#include <algorithm>
#include <map>
#include <sstream> // note, there is a version of this on the net that works with gcc 2.95.2, if necessary
#include <vector>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <CocoaAnimation.h>
#include <Console.h>
#include <Cursors.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// application includes
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "InfoWindow.h"
#include "Local.h"
#include "MacroManager.h"
#include "NetEvents.h"
#include "NewSessionDialog.h"
#include "Preferences.h"
#include "QuillsBase.h"
#include "SessionDescription.h"
#include "Terminal.h"
#include "TerminalFile.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "TextTranslation.h"
#include "UIStrings.h"
#include "Workspace.h"



#pragma mark Types
namespace {

typedef std::vector< SessionRef >						SessionList;
typedef std::vector< TerminalWindowRef >				TerminalWindowList;
typedef std::multimap< TerminalWindowRef, SessionRef >	TerminalWindowToSessionsMap;
typedef std::vector< Workspace_Ref >					MyWorkspaceList;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSStatus				appendDataForProcessing			(EventHandlerCallRef, EventRef, void*);
void					changeNotifyGlobal				(SessionFactory_Change, void*);
Boolean					configureSessionTerminalWindow	(TerminalWindowRef, Preferences_ContextRef);
TerminalWindowRef		createTerminalWindow			(Preferences_ContextRef = nullptr,
														 Preferences_ContextRef = nullptr,
														 Preferences_ContextRef = nullptr,
														 Boolean = false);
Workspace_Ref			createWorkspace					();
Boolean					displayTerminalWindow			(TerminalWindowRef, Preferences_ContextRef = nullptr, UInt16 = 0);
void					forEachSessionInListDo			(SessionList const&, SessionFactory_SessionFilterFlags,
														 SessionFactory_SessionOpProcPtr, void*, SInt32, void*);
void					forEveryTerminalWindowInListDo	(TerminalWindowList const&,
														 SessionFactory_TerminalWindowOpProcPtr, void*, SInt32, void*);
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
		UInt16 const			kMaxWindows = Workspace_ReturnWindowCount(inWorkspace);
		TerminalWindow_Result	terminalResult = kTerminalWindow_ResultOK;
		
		
		if ((kWorkspace_WindowIndexInfinity != kMaxWindows) && (kMaxWindows > 0))
		{
			Float32 const	kAverageTabWidth = 320.0; // arbitrary!
			Float32			currentOffset = 0.0; // while processing, each tab size is added here to define the next offset
			Float32			smallestMaxWidth = FLT_MAX;
			Float32			idealTabWidth = kAverageTabWidth;
			
			
			// determine how wide each tab should be; use the smallest window
			// for this, since Mac OS X will otherwise force a window resize
			for (UInt16 i = 0; i < kMaxWindows; ++i)
			{
				HIWindowRef			window = Workspace_ReturnWindowWithZeroBasedIndex(inWorkspace, i);
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(window);
				
				
				if (nullptr != terminalWindow)
				{
					Float32		availableSpace = FLT_MAX;
					
					
					terminalResult = TerminalWindow_GetTabWidthAvailable(terminalWindow, availableSpace);
					if (kTerminalWindow_ResultOK == terminalResult)
					{
						if (availableSpace < smallestMaxWidth)
						{
							smallestMaxWidth = availableSpace;
						}
					}
				}
			}
			idealTabWidth = smallestMaxWidth / kMaxWindows;
			if (idealTabWidth > kAverageTabWidth)
			{
				idealTabWidth = kAverageTabWidth;
			}
			
			// reposition and resize each window’s tab appropriately
			for (UInt16 i = 0; i < kMaxWindows; ++i)
			{
				HIWindowRef			window = Workspace_ReturnWindowWithZeroBasedIndex(inWorkspace, i);
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(window);
				
				
				if (nullptr != terminalWindow)
				{
					terminalResult = TerminalWindow_SetTabPosition(terminalWindow, currentOffset, idealTabWidth);
					if (kTerminalWindow_ResultOK == terminalResult)
					{
						Float32		tabWidth = 0.0;
						
						
						terminalResult = TerminalWindow_GetTabWidth(terminalWindow, tabWidth);
						//assert(kTerminalWindow_ResultOK == terminalResult);
						if (kTerminalWindow_ResultOK == terminalResult)
						{
							currentOffset += tabWidth;
							
							// reset the tab flag; this has the effect of opening the drawer
							// if it is not already open (TEMPORARY - should this be done in
							// a more direct way?)
							(OSStatus)TerminalWindow_SetTabAppearance(terminalWindow, true);
						}
					}
				}
			}
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
#pragma mark removeTerminalWindowFromWorkspace
class removeTerminalWindowFromWorkspace:
public std::unary_function< Workspace_Ref/* argument */, void/* return */ >
{
public:
	removeTerminalWindowFromWorkspace	(TerminalWindowRef	inTerminalWindow)
	: _window(TerminalWindow_ReturnWindow(inTerminalWindow))
	{
	}
	
	void
	operator()	(Workspace_Ref	inWorkspace)
	const
	{
		HIWindowRef		window = REINTERPRET_CAST(_window.returnHIObjectRef(), HIWindowRef);
		
		
		Workspace_RemoveWindow(inWorkspace, window);
	}

protected:

private:
	CFRetainRelease		_window;
};

/*!
This is a functor that determines if the specified
terminal window is used by the given session.

Model of STL Predicate.

(3.0)
*/
#pragma mark sessionUsesTerminalWindow
class sessionUsesTerminalWindow:
public std::unary_function< SessionRef/* argument */, bool/* return */ >
{
public:
	sessionUsesTerminalWindow	(TerminalWindowRef	inTerminalWindowRef)
	: _terminalWindow(inTerminalWindowRef)
	{
	}
	
	bool
	operator()	(SessionRef		inSession)
	const
	{
		return (Session_ReturnActiveTerminalWindow(inSession) == _terminalWindow);
	}

protected:

private:
	TerminalWindowRef	_terminalWindow;
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
	workspaceContainsWindow	(HIWindowRef	inWindow)
	: _window(inWindow)
	{
	}
	
	bool
	operator()	(Workspace_Ref	inWorkspace)
	const
	{
		HIWindowRef const	kWindow = REINTERPRET_CAST(_window.returnHIObjectRef(), HIWindowRef);
		UInt16 const		kIndexOfWindow = Workspace_ReturnZeroBasedIndexOfWindow(inWorkspace, kWindow);
		
		
		return (kWorkspace_WindowIndexInfinity != kIndexOfWindow);
	}

protected:

private:
	CFRetainRelease		_window;
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
			// if an original terminal window was given, perform a transition to
			// move the window from its (unstaggered) location to its new location
			CocoaAnimation_TransitionWindowForDuplicate(TerminalWindow_ReturnNSWindow(inTerminalWindow),
														(nullptr != sourceTerminalWindow)
															? TerminalWindow_ReturnNSWindow(sourceTerminalWindow)
															: TerminalWindow_ReturnNSWindow(inTerminalWindow));
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
			Console_Warning(Console_WriteLine, "unable to reconfigure terminal window");
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
			HIWindowRef		window = TerminalWindow_ReturnWindow(terminalWindow);
			
			
			// see also SessionFactory_RespawnSession(), which must do something similar
			localResult = Local_SpawnProcess(result, TerminalWindow_ReturnScreenWithFocus(terminalWindow),
												inArgumentArray, inWorkingDirectoryOrNull);
			if (kLocal_ResultOK == localResult)
			{
				// success!
				SetWindowKind(window, WIN_SHELL);
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
						CFStringRef				contextName;
						size_t					actualSize = 0;
						
						
						prefsResult = Preferences_ContextGetData(inContextOrNull, kPreferences_TagAssociatedTranslationFavorite,
																	sizeof(contextName), &contextName,
																	false/* search defaults too */, &actualSize);
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
											true/* is retained */);
			
			
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
															true/* is retained */);
					
					
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
				TerminalWindow_SetScreenDimensions(terminalWindow, columns, rows, false/* send to recording scripts */);
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
				if (kInvalidFontFamily != FMGetFontFamilyFromName(fontName))
				{
					TerminalWindow_SetFontAndSize(terminalWindow, fontName, 0/* font size, or 0 to ignore */);
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
			TerminalWindow_SetFontAndSize(terminalWindow, nullptr/* font */, fontSize);
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
Creates a terminal window (or uses the specified
window, if non-nullptr) and attempts to run the
specified ".term" file - just like the Apple Terminal
would.

If unsuccessful, nullptr is returned and an alert
message may be displayed to the user; otherwise, the
session reference for the new local process is
returned.

(3.1)
*/
SessionRef
SessionFactory_NewSessionFromTerminalFile	(TerminalWindowRef			inTerminalWindow,
											 char const*				inAppleDotTermFilePath,
											 Preferences_ContextRef		inWorkspaceOrNull,
											 UInt16						inWindowIndexInWorkspaceOrZero)
{
	SessionRef			result = nullptr;
	TerminalWindowRef	terminalWindow = inTerminalWindow;
	
	
	assert(nullptr != terminalWindow);
	if (false == displayTerminalWindow(terminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero))
	{
		Console_WriteLine("unexpected problem displaying terminal window!!!");
	}
	else
	{
		Boolean		displayOK = false;
		
		
		result = Session_New();
		if (nullptr != result)
		{
			// create POSIX equivalent path to file
			CFStringRef		pathCFString = CFStringCreateWithCString
											(kCFAllocatorDefault, inAppleDotTermFilePath,
												kCFStringEncodingMacRoman);
			
			
			if (nullptr != pathCFString)
			{
				// create URL pointing to file
				CFURLRef	url = CFURLCreateWithFileSystemPath
									(kCFAllocatorDefault, pathCFString,
										kCFURLPOSIXPathStyle, false/* is directory */);
				
				
				if (nullptr != url)
				{
					TerminalFileRef		termFile = nullptr;
					OSStatus			error = noErr;
					
					
					// parse file
					error = TerminalFile_NewFromFile(url, &termFile);
					if (noErr == error)
					{
						// UNIMPLEMENTED - read session data (e.g. command) from file reference and open session
						if (0)
						{
							// success!
							displayOK = true;
							startTrackingSession(result, terminalWindow);
						}
					}
					
					CFRelease(url), url = nullptr;
				}
				CFRelease(pathCFString), pathCFString = nullptr;
			}
			
			unless (displayOK)
			{
				// TEMPORARY - NEED to display some kind of user alert here
				Sound_StandardAlert();
				Session_Dispose(&result);
			}
		}
	}
	
	return result;
}// NewSessionFromTerminalFile


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
		Console_Warning(Console_WriteLine, "unable to reconfigure terminal window");
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
	size_t					actualSize = 0;
	
	
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
	for (UInt16 i = 1; i <= kPreferences_MaximumWorkspaceSize; ++i)
	{
		CFStringRef		associatedSessionName = nullptr;
		
		
		prefsResult = Preferences_ContextGetData(inWorkspaceContext,
													Preferences_ReturnTagVariantForIndex
													(kPreferences_TagIndexedWindowSessionFavorite, i),
													sizeof(associatedSessionName), &associatedSessionName,
													false/* search defaults too */, &actualSize);
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
															true/* is retained */);
				
				
				if (false == namedSettings.exists())
				{
					result = false;
				}
				else
				{
					TerminalWindowRef	terminalWindow = createTerminalWindow();
					SessionRef			session = SessionFactory_NewSessionUserFavorite(terminalWindow,
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
														false/* search defaults too */, &actualSize);
			if ((kPreferences_ResultOK == prefsResult) && (0 != associatedSessionType))
			{
				TerminalWindowRef	terminalWindow = createTerminalWindow();
				Boolean				launchOK = newSessionFromCommand
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
	}
	
	if (enterFullScreen)
	{
		Commands_ExecuteByIDUsingEventAfterDelay(kCommandFullScreenModal, nullptr/* target */, 0.5/* delay */);
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
Creates a terminal window and immediately displays
a dialog allowing the user to customize what it
should be used for.  (On Mac OS X, the dialog is a
sheet attached to the window.)  The given preferences
context is used to initialize the user interface.

Returns true only if the user customization *attempt*
was made without errors.  Since the user may take
considerable time to complete the session creation,
this function returns immediately; you can only use
the return value to determine if (say) a dialog
box was displayed correctly, NOT to determine if the
user’s new session was created.

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
	TerminalWindowRef		terminalWindow = inTerminalWindow;
	Preferences_ContextRef	sessionContext = nullptr;
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Boolean					result = true;
	
	

	prefsResult = Preferences_GetDefaultContext(&sessionContext, Quills::Prefs::SESSION);
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteLine, "unable to initialize dialog with Session Default preferences!");
	}
	
	assert(nullptr != terminalWindow);
	{
		// display a terminal window and then immediately display
		// a sheet asking the user what to do with the new window
		NewSessionDialog_Ref	dialog = NewSessionDialog_New(terminalWindow, sessionContext);
		
		
		// synchronize the window’s initial appearance with its preferences
		// (e.g. window size due to font and screen dimensions, and colors);
		// note however that this will be reset anyway when the session begins
		(Boolean)configureSessionTerminalWindow(terminalWindow, sessionContext);
		
		if (displayTerminalWindow(terminalWindow, inWorkspaceOrNull, inWindowIndexInWorkspaceOrZero))
		{
			NewSessionDialog_Display(dialog); // automatically disposed when the user clicks a button
		}
		else
		{
			// some kind of problem?!?
			Sound_StandardAlert();
			Console_WriteLine("unexpected problem displaying terminal window!!!");
			NewSessionDialog_Dispose(&dialog);
			result = false;
		}
	}
	return result;
}// DisplayUserCustomizationUI


/*!
When you need to perform an operation on a select subset
of all sessions, use this method.  Session references can
be used to obtain a wealth of information about a session
(including pointers to other data structures), and to
manipulate those sessions (for example, hiding a session’s
window).

The filter flags indicate which sessions you wish to
operate upon.  Often, you pass
"kSessionFactory_SessionFilterFlagAllSessions" to indicate
that you want every session.

You also provide a routine that operates on each session
from the subset you indicate.  Your routine defines what
"inoutResultPtr" is, and the meaning of "inData1" and
"inData2".

The last argument, "inFinal", is CRITICAL for any loop
that might modify the list during the iteration: when
"inFinal" is true, the iteration occurs on a copy of the
list as it existed on entry to this function, allowing
your iterations to modify the real list without creating
chaos in your iteration.

(3.0)
*/
void
SessionFactory_ForEachSessionDo		(SessionFactory_SessionFilterFlags	inFilterFlags,
									 SessionFactory_SessionOpProcPtr	inProcPtr,
									 void*								inData1,
									 SInt32								inData2,
									 void*								inoutResultPtr,
									 Boolean							inFinal)
{
	if (inFinal)
	{
		// during final iterations, make A COPY of the list, because
		// the iteration may well delete items in the list itself!
		SessionList		listCopy = gSessionListSortedByCreationTime();
		
		
		forEachSessionInListDo(listCopy, inFilterFlags, inProcPtr, inData1, inData2, inoutResultPtr);
	}
	else
	{
		// ordinary iterations can use the actual list, but they MUST
		// NOT INSERT OR DELETE any items in the list!
		forEachSessionInListDo(gSessionListSortedByCreationTime(), inFilterFlags, inProcPtr, inData1, inData2, inoutResultPtr);
	}
}// ForEachSessionDo


/*!
When you need to perform an operation on every
terminal window, use this method.  Terminal window
references can be used to obtain a wealth of
information about a terminal window (including
pointers to other data structures), and to
manipulate those windows (for example, changing
its colors).

You also provide a routine that operates on each
screen from the subset you indicate.  Your routine
defines what "inoutResultPtr" is, and the meaning of
"inData1" and "inData2".

WARNING:	It is not safe to use this routine with
			an iterator that destroys terminal
			windows, sessions, or other “first class”
			objects.

(3.0)
*/
void
SessionFactory_ForEveryTerminalWindowDo		(SessionFactory_TerminalWindowOpProcPtr		inProcPtr,
											 void*										inData1,
											 SInt32										inData2,
											 void*										inoutResultPtr)
{
	// ordinary iterations can use the actual list, but they MUST
	// NOT INSERT OR DELETE any items in the list!
	forEveryTerminalWindowInListDo(gTerminalWindowListSortedByCreationTime(), inProcPtr, inData1, inData2, inoutResultPtr);
}// ForEveryTerminalWindowDo


/*!
Returns the window whose order in the specified list
(each list is sorted by a different criteria) is the
index given.

\retval kSessionFactory_ResultOK
if there are no errors

\retval kSessionFactory_ResultParameterError
if the specified index, list type or window pointer
is invalid

(3.0)
*/
SessionFactory_Result
SessionFactory_GetWindowWithZeroBasedIndex		(UInt16					inZeroBasedSessionIndex,
												 SessionFactory_List	inFromWhichList,
												 HIWindowRef*			outWindowPtr)
{
	SessionFactory_Result	result = kSessionFactory_ResultOK;
	
	
	if (nullptr == outWindowPtr) result = kSessionFactory_ResultParameterError;
	else
	{
		switch (inFromWhichList)
		{
		case kSessionFactory_ListInCreationOrder:
			// return the order in which the specified session was created,
			// relative to all other sessions currently in existence
			{
				// look for the given session in the list
				if (inZeroBasedSessionIndex >= gSessionListSortedByCreationTime().size())
				{
					// index is invalid
					result = kSessionFactory_ResultParameterError;
				}
				else
				{
					// index is valid; return the appropriate session reference
					SessionRef		session = gSessionListSortedByCreationTime()[inZeroBasedSessionIndex];
					
					
					*outWindowPtr = Session_ReturnActiveWindow(session);
				}
			}
			break;
		
		case kSessionFactory_ListInTabStackOrder:
			{
				MyWorkspaceList&					workspaceList = gWorkspaceListSortedByCreationTime();
				MyWorkspaceList::const_iterator		toWorkspace;
				UInt16								currentIndex = inZeroBasedSessionIndex;
				
				
				// the index basically acts as if all workspaces were laid
				// end-to-end, and the indices in each workspace were renumbered
				// according to their placement in that super-list; if there is
				// no match among tabbed windows, there could still be windows
				// that are not tabbed that match
				*outWindowPtr = nullptr;
				for (toWorkspace = workspaceList.begin();
						toWorkspace != workspaceList.end(); ++toWorkspace)
				{
					UInt16 const	kWindowCount = Workspace_ReturnWindowCount(*toWorkspace);
					
					
					if (currentIndex >= kWindowCount)
					{
						currentIndex -= kWindowCount;
					}
					else
					{
						*outWindowPtr = Workspace_ReturnWindowWithZeroBasedIndex(*toWorkspace, currentIndex);
						break;
					}
				}
				if (nullptr == *outWindowPtr)
				{
					if (currentIndex < gSessionListSortedByCreationTime().size())
					{
						SessionRef		session = gSessionListSortedByCreationTime()[currentIndex];
						
						
						*outWindowPtr = Session_ReturnActiveWindow(session);
					}
				}
				
				if (nullptr == *outWindowPtr)
				{
					result = kSessionFactory_ResultParameterError;
				}
			}
			break;
		
		default:
			// ???
			result = kSessionFactory_ResultParameterError;
			break;
		}
	}
	
	return result;
}// GetSessionWithZeroBasedIndex


/*!
Returns the order in which the given session
appears in the specified list (each list is
sorted by a different criteria).

\retval kSessionFactory_ResultOK
if there are no errors

\retval kSessionFactory_ResultParameterError
if the specified session, list type or index
pointer is invalid

(3.0)
*/
SessionFactory_Result
SessionFactory_GetZeroBasedIndexOfSession	(SessionRef				inOfWhichSession,
											 SessionFactory_List	inFromWhichList,
											 UInt16*				outIndexPtr)
{
	SessionFactory_Result	result = kSessionFactory_ResultOK;
	
	
	if (nullptr == inOfWhichSession) result = kSessionFactory_ResultParameterError;
	else
	{
		switch (inFromWhichList)
		{
		case kSessionFactory_ListInCreationOrder:
			// return the order in which the specified session was created,
			// relative to all other sessions currently in existence
			{
				SessionList::const_iterator		sessionIterator;
				SessionRef						session = nullptr;
				
				
				// look for the given session in the list
				*outIndexPtr = 0;
				for (sessionIterator = gSessionListSortedByCreationTime().begin();
						sessionIterator != gSessionListSortedByCreationTime().end(); ++sessionIterator)
				{
					session = *sessionIterator;
					if (session == inOfWhichSession) break;
					++(*outIndexPtr);
				}
				
				if (*outIndexPtr >= gSessionListSortedByCreationTime().size())
				{
					// session was not in the list; return an error
					result = kSessionFactory_ResultParameterError;
				}
			}
			break;
		
		case kSessionFactory_ListInTabStackOrder:
			{
				TerminalWindowRef					terminalWindow = Session_ReturnActiveTerminalWindow
																		(inOfWhichSession);
				HIWindowRef							window = TerminalWindow_ReturnWindow(terminalWindow);
				MyWorkspaceList&					workspaceList = gWorkspaceListSortedByCreationTime();
				MyWorkspaceList::const_iterator		toWorkspace;
				Boolean								foundWindow = false;
				
				
				// look for the given session in available workspaces;
				// it is also possible that the window will not be tabbed
				*outIndexPtr = 0;
				for (toWorkspace = workspaceList.begin();
						toWorkspace != workspaceList.end(); ++toWorkspace)
				{
					UInt16		windowIndex = Workspace_ReturnZeroBasedIndexOfWindow(*toWorkspace, window);
					
					
					if (kWorkspace_WindowIndexInfinity != windowIndex)
					{
						foundWindow = true;
						(*outIndexPtr) += windowIndex;
						break;
					}
					else
					{
						(*outIndexPtr) += Workspace_ReturnWindowCount(*toWorkspace);
					}
				}
				if (false == foundWindow)
				{
					SessionList::const_iterator		sessionIterator;
					SessionRef						session = nullptr;
					
					
					// look for the given session in the list
					for (sessionIterator = gSessionListSortedByCreationTime().begin();
							sessionIterator != gSessionListSortedByCreationTime().end(); ++sessionIterator)
					{
						session = *sessionIterator;
						if (session == inOfWhichSession)
						{
							foundWindow = true;
							break;
						}
						++(*outIndexPtr);
					}
				}
				
				if (false == foundWindow)
				{
					result = kSessionFactory_ResultParameterError;
				}
			}
			break;
		
		default:
			// ???
			result = kSessionFactory_ResultParameterError;
			break;
		}
	}
	
	return result;
}// GetZeroBasedIndexOfSession


/*!
Creates a brand new workspace (window group or tab group)
and moves the specified terminal window into it, removing
the window from its previous workspace.

(3.1)
*/
void
SessionFactory_MoveTerminalWindowToNewWorkspace		(TerminalWindowRef		inTerminalWindow)
{
	HIWindowRef			window = TerminalWindow_ReturnWindow(inTerminalWindow);
	Workspace_Ref		newWorkspace = createWorkspace();
	MyWorkspaceList&	workspaceList = gWorkspaceListSortedByCreationTime();
	
	
	// IMPORTANT: Hiding the tab prior to window group manipulation
	// seems to be key to avoiding graphical glitches.  As long as
	// the tab drawer is hidden when the window changes groups, and
	// redisplayed afterwards, drawers remain in the proper position.
	(OSStatus)TerminalWindow_SetTabAppearance(inTerminalWindow, false);
	
	assert(nullptr != newWorkspace);
	assert(workspaceList.end() != std::find(workspaceList.begin(), workspaceList.end(), newWorkspace));
	
	// ensure the window is not a member of any other workspace
	(removeTerminalWindowFromWorkspace)std::for_each(workspaceList.begin(), workspaceList.end(),
														removeTerminalWindowFromWorkspace(inTerminalWindow));
	if (gAutoRearrangeTabs)
	{
		// TEMPORARY, INCOMPLETE - figure out what workspace the window used to be in,
		// and call "fixTerminalWindowTabPositionsInWorkspace()(oldWorkspace)"
		// (should this kind of thing be handled automatically through callbacks, when
		// windows are removed from a workspace for any reason?)
		(fixTerminalWindowTabPositionsInWorkspace)std::for_each(workspaceList.begin(), workspaceList.end(),
																fixTerminalWindowTabPositionsInWorkspace());
	}
	
	// offset the window slightly to emphasize its detachment
	{
		Rect		structureBounds;
		OSStatus	error = noErr;
		
		
		error = GetWindowBounds(window, kWindowStructureRgn, &structureBounds);
		if (noErr == error)
		{
			OffsetRect(&structureBounds, 32/* arbitrary */, 32/* arbitrary */);
			SetWindowBounds(window, kWindowStructureRgn, &structureBounds);
		}
	}
	
	// now add it to the new workspace
	Workspace_AddWindow(newWorkspace, window);
	(OSStatus)TerminalWindow_SetTabAppearance(inTerminalWindow, true);
	fixTerminalWindowTabPositionsInWorkspace()(newWorkspace);
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
	return gSessionListSortedByCreationTime().size();
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
	SessionList::const_iterator		sessionIterator;
	UInt16							result = 0;
	
	
	// traverse the list
	for (sessionIterator = gSessionListSortedByCreationTime().begin();
			sessionIterator != gSessionListSortedByCreationTime().end(); ++sessionIterator)
	{
		if ((nullptr != *sessionIterator) && (Session_ReturnState(*sessionIterator) == inStateToCheckFor)) ++result;
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
		TerminalWindowToSessionsMap::const_iterator		terminalWindowToSessionIterator =
															gTerminalWindowToSessions().find(inTerminalWindow);
		
		
		if (gTerminalWindowToSessions().end() != terminalWindowToSessionIterator)
		{
			result = terminalWindowToSessionIterator->second;
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
Using a *session* context, configures the specified
terminal window appropriately.

This works by checking for any “associated format” in
the given context, finds the context with that name,
and copies its settings.

Returns true only if successful.

(4.0)
*/
Boolean
configureSessionTerminalWindow	(TerminalWindowRef			inTerminalWindow,
								 Preferences_ContextRef		inSessionContext)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	size_t					actualSize = 0;
	Boolean					result = false;
	
	
	// copy settings to the terminal window; note that a session context
	// will not directly contain view settings such as fonts and colors,
	// but it may contain the name of a context to use for this
	{
		CFStringRef		contextName = nullptr;
		
		
		// font and color settings
		prefsResult = Preferences_ContextGetData(inSessionContext, kPreferences_TagAssociatedFormatFavorite,
													sizeof(contextName), &contextName,
													false/* search defaults too */, &actualSize);
		if (kPreferences_ResultOK == prefsResult)
		{
			if (false == Preferences_IsContextNameInUse(Quills::Prefs::FORMAT, contextName))
			{
				Console_Warning(Console_WriteValueCFString, "associated Format not found", contextName);
			}
			else
			{
				Preferences_ContextWrap		associatedContext(Preferences_NewContextFromFavorites
																(Quills::Prefs::FORMAT, contextName), true/* is retained */);
				
				
				if (false == associatedContext.exists())
				{
					Console_Warning(Console_WriteValueCFString, "associated Format could not be used", contextName);
				}
				else
				{
					result = TerminalWindow_ReconfigureViewsInGroup
								(inTerminalWindow, kTerminalWindow_ViewGroupActive, associatedContext.returnRef(),
									Quills::Prefs::FORMAT);
				}
			}
			CFRelease(contextName), contextName = nullptr;
		}
		
		// terminal emulator settings
		prefsResult = Preferences_ContextGetData(inSessionContext, kPreferences_TagAssociatedTerminalFavorite,
													sizeof(contextName), &contextName,
													false/* search defaults too */, &actualSize);
		if (kPreferences_ResultOK == prefsResult)
		{
			if (false == Preferences_IsContextNameInUse(Quills::Prefs::TERMINAL, contextName))
			{
				Console_Warning(Console_WriteValueCFString, "associated Terminal not found", contextName);
			}
			else
			{
				Preferences_ContextWrap		associatedContext(Preferences_NewContextFromFavorites
																(Quills::Prefs::TERMINAL, contextName), true/* is retained */);
				
				
				if (false == associatedContext.exists())
				{
					Console_Warning(Console_WriteValueCFString, "associated Terminal could not be used", contextName);
				}
				else
				{
					result = TerminalWindow_ReconfigureViewsInGroup
								(inTerminalWindow, kTerminalWindow_ViewGroupActive, associatedContext.returnRef(),
									Quills::Prefs::TERMINAL);
				}
			}
			CFRelease(contextName), contextName = nullptr;
		}
		
		// translation settings
		prefsResult = Preferences_ContextGetData(inSessionContext, kPreferences_TagAssociatedTranslationFavorite,
													sizeof(contextName), &contextName,
													false/* search defaults too */, &actualSize);
		if (kPreferences_ResultOK == prefsResult)
		{
			if (false == Preferences_IsContextNameInUse(Quills::Prefs::TRANSLATION, contextName))
			{
				Console_Warning(Console_WriteValueCFString, "associated Translation not found", contextName);
			}
			else
			{
				Preferences_ContextWrap		associatedContext(Preferences_NewContextFromFavorites
																(Quills::Prefs::TRANSLATION, contextName), true/* is retained */);
				
				
				if (false == associatedContext.exists())
				{
					Console_Warning(Console_WriteValueCFString, "associated Translation could not be used", contextName);
				}
				else
				{
					result = TerminalWindow_ReconfigureViewsInGroup
								(inTerminalWindow, kTerminalWindow_ViewGroupActive, associatedContext.returnRef(),
									Quills::Prefs::TRANSLATION);
				}
			}
			CFRelease(contextName), contextName = nullptr;
		}
	}
	
	return result;
}// configureSessionTerminalWindow


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
	
	
	// create a new terminal window to house the session
	result = TerminalWindow_New(inTerminalInfoOrNull, inFontInfoOrNull, inTranslationInfoOrNull, inNoStagger);
	
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
	WindowRef	window = TerminalWindow_ReturnWindow(inTerminalWindow);
	Boolean		result = true;
	
	
	if (nullptr == window) result = false;
	else
	{
		TerminalWindow_Result	terminalWindowResult = kTerminalWindow_ResultOK;
		TerminalViewRef			view = nullptr;
		Workspace_Ref			targetWorkspace = nullptr;
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		Boolean					useTabs = false;
		
		
		// set the window location and size appropriately
		if ((0 != inWindowIndexInWorkspaceOrZero) && (nullptr != inWorkspaceOrNull))
		{
			HIRect		frameBounds;
			size_t		actualSize = 0;
			
			
			prefsResult = Preferences_ContextGetData(inWorkspaceOrNull, Preferences_ReturnTagVariantForIndex
																		(kPreferences_TagIndexedWindowFrameBounds,
																			inWindowIndexInWorkspaceOrZero),
														sizeof(frameBounds), &frameBounds, false/* search defaults */,
														&actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				// UNIMPLEMENTED - check kPreferences_TagIndexedWindowScreenBounds too, and
				// if the current display size disagrees, adjust the window location somehow
				
				// IMPORTANT: the boundaries are not guaranteed to specify the width or height
				MoveWindowStructure(window, frameBounds.origin.x, frameBounds.origin.y);
			}
		}
		
		// set the window title, if one is defined
		if ((0 != inWindowIndexInWorkspaceOrZero) && (nullptr != inWorkspaceOrNull))
		{
			CFStringRef		titleCFString = nullptr;
			size_t			actualSize = 0;
			
			
			prefsResult = Preferences_ContextGetData(inWorkspaceOrNull, Preferences_ReturnTagVariantForIndex
																		(kPreferences_TagIndexedWindowTitle,
																			inWindowIndexInWorkspaceOrZero),
														sizeof(titleCFString), &titleCFString, false/* search defaults */,
														&actualSize);
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
			if ((useTabs) && (1 == inWindowIndexInWorkspaceOrZero))
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
			Workspace_AddWindow(targetWorkspace, window);
			TerminalWindow_SetVisible(inTerminalWindow, true);
			(OSStatus)TerminalWindow_SetTabAppearance(inTerminalWindow, true);
			if (gAutoRearrangeTabs)
			{
				(fixTerminalWindowTabPositionsInWorkspace)std::for_each(targetList.begin(), targetList.end(),
																		fixTerminalWindowTabPositionsInWorkspace());
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
Internal version of SessionFactory_ForEachSessionDo(),
except it operates on the specific list given.

(3.1)
*/
void
forEachSessionInListDo		(SessionList const&					inList,
							 SessionFactory_SessionFilterFlags	inFilterFlags,
							 SessionFactory_SessionOpProcPtr	inProcPtr,
							 void*								inData1,
							 SInt32								inData2,
							 void*								inoutResultPtr)
{
	SessionList::const_iterator		sessionIterator;
	SessionRef						session = nullptr;
	Boolean							doInvoke = false;
	
	
	// traverse the list
	for (sessionIterator = inList.begin(); sessionIterator != inList.end(); ++sessionIterator)
	{
		session = *sessionIterator;
		doInvoke = true;
		if (0 == (inFilterFlags & kSessionFactory_SessionFilterFlagConsoleSessions))
		{
			HIWindowRef		sessionWindow = Session_ReturnActiveWindow(session);
			
			
			if ((nullptr != sessionWindow) && (WIN_CONSOLE == GetWindowKind(sessionWindow)))
			{
				doInvoke = false;
			}
		}
		if (doInvoke) SessionFactory_InvokeSessionOpProc(inProcPtr, session, inData1, inData2, inoutResultPtr);
	}
}// forEachSessionInListDo


/*!
Internal version of SessionFactory_ForEveryTerminalWindowDo(),
except it operates on the specific list given.

(3.1)
*/
void
forEveryTerminalWindowInListDo	(TerminalWindowList const&					inList,
								 SessionFactory_TerminalWindowOpProcPtr		inProcPtr,
								 void*										inData1,
								 SInt32										inData2,
								 void*										inoutResultPtr)
{
	TerminalWindowList::const_iterator		terminalWindowIterator;
	TerminalWindowRef						terminalWindow = nullptr;
	
	
	// traverse the list
	for (terminalWindowIterator = inList.begin(); terminalWindowIterator != inList.end(); ++terminalWindowIterator)
	{
		terminalWindow = *terminalWindowIterator;
		SessionFactory_InvokeTerminalWindowOpProc(inProcPtr, terminalWindow, inData1, inData2, inoutResultPtr);
	}
}// forEveryTerminalWindowInListDo


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
						TerminalWindowRef			terminalWindow = createTerminalWindow();
						Preferences_ContextRef		workspaceContext = nullptr;
						Boolean						launchOK = newSessionFromCommand
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
																true/* is retained */);
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
			HIWindowRef const					kActiveWindow = Session_ReturnActiveWindow(activeSession);
			MyWorkspaceList&					targetList = gWorkspaceListSortedByCreationTime();
			MyWorkspaceList::const_iterator		toWorkspace = std::find_if(targetList.begin(), targetList.end(),
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
				// final state; delete the session from all internal lists and maps that have it
				stopTrackingSession(session);
				// end kiosk mode no matter what terminal is disconnecting
				// TEMPORARY: fix this...why does it seem sometimes this event does not happen?!?
				Commands_ExecuteByIDUsingEvent(kCommandKioskModeDisable);
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
			SessionList&			targetList = gSessionListSortedByCreationTime();
			SessionList::iterator	sessionIterator = std::find_if(targetList.begin(), targetList.end(),
																	sessionUsesTerminalWindow(terminalWindow));
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
	MyWorkspaceList&		workspaceList = gWorkspaceListSortedByCreationTime();
	(removeTerminalWindowFromWorkspace)std::for_each(workspaceList.begin(), workspaceList.end(),
														removeTerminalWindowFromWorkspace(inTerminalWindow));
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

// BELOW IS REQUIRED NEWLINE TO END FILE
