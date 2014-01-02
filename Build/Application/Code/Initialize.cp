/*!	\file Initialize.cp
	\brief Setup and teardown for all modules that
	require it (and cannot do it just-in-time).
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#include "Initialize.h"
#include <UniversalDefines.h>

// standard-C includes
#include <cstdio>
#include <cstdlib>
#include <cstring>

// standard-C++ includes
#include <map>
#include <sstream>
#include <string>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CocoaBasic.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <Localization.h>
#include <MacHelpUtilities.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <Releases.h>
#include <Undoables.h>

// application includes
#include "AppResources.h"
#include "Clipboard.h"
#include "CommandLine.h"
#include "Commands.h"
#include "DebugInterface.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "InfoWindow.h"
#include "InternetPrefs.h"
#include "Preferences.h"
#include "PrefsWindow.h"
#include "RecordAE.h"
#include "SessionFactory.h"
#include "TerminalBackground.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "UIStrings.h"



#pragma mark Internal Method Prototypes
namespace {

void	initApplicationCore		();
void	initMacOSToolbox		();

} // anonymous namespace



#pragma mark Public Methods

/*!
This routine initializes the operating system’s
components, as well as every application module,
in a particular dependency order.

(3.0)
*/
void
Initialize_ApplicationStartup	(CFBundleRef	inApplicationBundle)
{
	// seed random number generator; calls to random() should not be
	// used for anything particularly important, arc4random() is better
	::srandom(TickCount());
	
//#define RUN_MODULE_TESTS (defined DEBUG)
#define RUN_MODULE_TESTS 0
	
	Console_Init();
#if RUN_MODULE_TESTS
	//Console_RunTests();
#endif
	
#if RUN_MODULE_TESTS
	MemoryBlockPtrLocker_RunTests();
#endif
	
	// set the application bundle so everything searches in the right place for resources
	AppResources_Init(inApplicationBundle);
	
	// initialize Cocoa
	EventLoop_Init();
	
	// initialize memory manager, start up toolbox managers, etc.
	initMacOSToolbox();
	
	// on Mac OS X the following call also stops the bouncing of the Dock icon
	//FlushEvents(everyEvent/* events to flush */, 0/* events to not flush */);
	
	initApplicationCore();
	
#ifndef NDEBUG
	// the Console must be among the first things initialized if
	// it is to be used for debugging of these early modules
	Console_WriteLine("Starting up.");
#endif
	
#if RUN_MODULE_TESTS
	// Any module that does not have an Init routine should
	// be tested up front; in theory, these modules are
	// standalone and should not require other module
	// initializations ahead of them (aside from Console,
	// which is likely used for debug)!  Other modules should
	// be tested as soon as possible after their Init routine
	// is called, i.e. call Foo_Init() and then Foo_RunTests().
	ListenerModel_RunTests();
#endif
	
	// do everything else
	{
		SessionFactory_Init();
	#if RUN_MODULE_TESTS
		//SessionFactory_RunTests();
	#endif
		
		Commands_Init();
	#if RUN_MODULE_TESTS
		//Commands_RunTests();
	#endif
		
		TerminalBackground_Init();
	#if RUN_MODULE_TESTS
		//TerminalBackground_RunTests();
	#endif
		
		TerminalView_Init();
	#if RUN_MODULE_TESTS
		//TerminalView_RunTests();
	#endif
		
		CommandLine_Init();
	#if RUN_MODULE_TESTS
		//CommandLine_RunTests();
	#endif
		
		Clipboard_Init();
	#if RUN_MODULE_TESTS
		//Clipboard_RunTests();
	#endif
		
		InfoWindow_Init(); // installs command handler to enable this window to be displayed and hidden
	#if RUN_MODULE_TESTS
		//InfooWindow_RunTests();
	#endif
		
		InternetPrefs_Init();
	#if RUN_MODULE_TESTS
		//InternetPrefs_RunTests();
	#endif
		
		{
			Size 	junk = 0L;
			
			
			MaxMem(&junk);
		}
		
	#ifndef NDEBUG
		// write an initial header to the console that describes the user’s runtime environment
		{
			std::ostringstream	messageBuffer;
			long				gestaltResult = 0L;
			
			
			// useful values
			if (Gestalt(gestaltSystemVersion, &gestaltResult) != noErr)
			{
				messageBuffer << "Could not find Mac OS version!";
			}
			else
			{
				messageBuffer
				<< "This computer is running Mac OS X "
				<< STATIC_CAST(Releases_ReturnMajorRevisionForVersion(gestaltResult), unsigned int)
				<< "."
				<< STATIC_CAST(Releases_ReturnMinorRevisionForVersion(gestaltResult), unsigned int)
				<< "."
				<< STATIC_CAST(Releases_ReturnSuperminorRevisionForVersion(gestaltResult), unsigned int)
				<< "."
				;
			}
			std::string		messageString = messageBuffer.str();
			Console_WriteLine(messageString.c_str());
		}
	#endif
		
		// open a new, untitled document (this will eventually not be necessary; see
		// the note in "applicationShouldOpenUntitledFile:" in the implementation
		// for the application delegate, in "Commands.mm")
		{
			Boolean		quellAutoNew = false;
			size_t		actualSize = 0L;
			
			
			// get the user’s “don’t auto-new” application preference, if possible
			if (kPreferences_ResultOK !=
				Preferences_GetData(kPreferences_TagDontAutoNewOnApplicationReopen, sizeof(quellAutoNew),
									&quellAutoNew, &actualSize))
			{
				// assume a value if it cannot be found
				quellAutoNew = false;
			}
			
			unless (quellAutoNew)
			{
				Commands_ExecuteByIDUsingEvent(kCommandRestoreWorkspaceDefaultFavorite);
			}
		}
	}
	
	// now set a flag indicating that initialization succeeded
	FlagManager_Set(kFlagInitializationComplete, true);
	
	// if requested, automatically show the experimental new terminal window
	{
		char const*		varValue = getenv("MACTERM_AUTO_SHOW_COCOA_TERM");
		
		
		if ((nullptr != varValue) && (0 == strcmp(varValue, "1")))
		{
			DebugInterface_DisplayTestTerminal();
		}
	}
	
	// if requested, automatically show the experimental new preferences window
	{
		char const*		varValue = getenv("MACTERM_AUTO_SHOW_COCOA_PREFS");
		
		
		if ((nullptr != varValue) && (0 == strcmp(varValue, "1")))
		{
			DebugInterface_DisplayTestPrefsWindow();
		}
	}
}// ApplicationStartup


/*!
Shuts down components that seem “safe” to tear down, because
they are isolated and nothing else should depend on the things
that they created.

See also Initialize_ApplicationShutDownRemainingComponents().

(4.0)
*/
void
Initialize_ApplicationShutDownIsolatedComponents ()
{
	CommandLine_Done();
	Clipboard_Done();
	InfoWindow_Done();
	PrefsWindow_Done(); // also saves other preferences
	Preferences_Done();
	EventLoop_Done();
}// ApplicationShutDownIsolatedComponents


/*!
Undoes the things that Initialize_ApplicationStartup()
does, in reverse order, that were not undone by an initial
call to ApplicationShutDownIsolatedComponents().

See the note in QuillsBase.cp, "Base::all_done()", for the
problems caused by performing the teardowns below, in the
current design.  Hopefully those issues will be addressed
by moving to Cocoa.

(4.0)
*/
void
Initialize_ApplicationShutDownRemainingComponents ()
{
	Console_Done();
	TerminalView_Done();
	TerminalBackground_Done();
	Commands_Done();
	SessionFactory_Done();
	Alert_Done();
	Undoables_Done();
}// ApplicationShutDownRemainingComponents


/*!
This function is not normally invoked directly.  It is only
needed when depending on OS version flags like "kFlagOS10_9API"
in code that is called at an unknown time or that might be
called very early in the startup process.  In those situations,
call Initialize_SetVersionFlags() as a guard to ensure that the
right API flags are actually defined before you use them.

In the vast majority of cases, initialization has already
occurred and you can simply test version flags normally without
explicitly invoking this routine first.

(4.0)
*/
void
Initialize_SetVersionFlags ()
{
	long	gestaltResult = 0L;
	UInt8   majorRev = 0;
	UInt8   minorRev = 0;
	
	
	UNUSED_RETURN(OSStatus)Gestalt(gestaltSystemVersion, &gestaltResult);
	majorRev = Releases_ReturnMajorRevisionForVersion(gestaltResult);
	minorRev = Releases_ReturnMinorRevisionForVersion(gestaltResult);
	
	// any advanced APIs available?
	FlagManager_Set(kFlagOS10_6API, true); // this source tree has a minimum SDK
	FlagManager_Set(kFlagOS10_7API, (((majorRev == 0x0A) && (minorRev >= 0x07)) || (majorRev > 0x0A)));
	FlagManager_Set(kFlagOS10_8API, (((majorRev == 0x0A) && (minorRev >= 0x08)) || (majorRev > 0x0A)));
	FlagManager_Set(kFlagOS10_9API, (((majorRev == 0x0A) && (minorRev >= 0x09)) || (majorRev > 0x0A)));
	FlagManager_Set(kFlagOS10_10API, (((majorRev == 0x0A) && (minorRev >= 0x0A)) || (majorRev > 0x0A)));
}// SetVersionFlags


#pragma mark Internal Methods
namespace {

/*!
This method initializes key modules (both
internally and from MacTerm’s libraries),
installs Apple Event handlers for the
Apple Events that MacTerm supports and
requires, and reads user preferences.

If errors occur at this stage, this method
may not return, displaying an error message
to the user.

(3.0)
*/
void
initApplicationCore ()
{
	// set up the Localization module, and define where all user interface resources should come from
	{
		Localization_InitFlags		flags = 0L;
		ScriptCode					systemScriptCode = STATIC_CAST(GetScriptManagerVariable(smSysScript), ScriptCode);
		
		
		if (GetScriptVariable(systemScriptCode, smScriptRight))
		{
			// then the text reads from the right side of a page to the left (the opposite of North America)
			flags |= kLocalization_InitFlagReadTextRightToLeft;
		}
		
		Localization_Init(flags);
	}
	
	// determine help system attributes
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			helpBookAppleTitle = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_HelpSystemName, helpBookAppleTitle);
		if (stringResult.ok())
		{
			MacHelpUtilities_Init(helpBookAppleTitle);
			CFRelease(helpBookAppleTitle), helpBookAppleTitle = nullptr;
		}
	}
	
	// set up notification info
	{
		Str255		notificationMessage;
		UInt16		notificationPreferences = kAlert_NotifyDisplayDiamondMark;
		size_t		actualSize = 0L;
		
		
		unless (Preferences_GetData(kPreferences_TagNotification, sizeof(notificationPreferences),
									&notificationPreferences, &actualSize) ==
				kPreferences_ResultOK)
		{
			notificationPreferences = kAlert_NotifyDisplayDiamondMark; // assume default, if preference can’t be found
		}
		
		// the Interface Library Alert module is responsible for handling Notification Manager stuff...
		Alert_SetNotificationPreferences(notificationPreferences);
		// TEMPORARY: This needs a new localized string.  LOCALIZE THIS.
		//GetIndString(notificationMessage, rStringsNoteAlerts, siNotificationAlert);
		//Alert_SetNotificationMessage(notificationMessage);
	}
	
#if RUN_MODULE_TESTS
	Preferences_RunTests();
#endif
	
	// initialize some other flags
	FlagManager_Set(kFlagSuspended, false); // initially, the application is active
}// initApplicationCore


/*!
This method starts up all of the Mac OS tool sets
needed by version 3.0, and allocates a memory
reserve.

If errors occur at this stage, this method may
not return.  If this routine actually returns,
then the user’s computer is at least minimally
capable of running version 3.0.  However, more
tests are done in initApplicationCore()...

(3.0)
*/
void
initMacOSToolbox ()
{
	// Launch Services seems to recommend this, so do it
	LSInit(kLSInitializeDefaults);
	
	// see "kFlagOS10_9API", etc.
	Initialize_SetVersionFlags();
	
	InitCursor();
	
	// initialization of the Alert module must be done here, otherwise startup error alerts can’t be displayed
	Alert_Init();
	
	// initialize the Undoables module
	{
		CFStringRef		undoNameCFString = nullptr;
		CFStringRef		redoNameCFString = nullptr;
		
		
		assert(UIStrings_Copy(kUIStrings_UndoDefault, undoNameCFString).ok());
		assert(UIStrings_Copy(kUIStrings_RedoDefault, redoNameCFString).ok());
		Undoables_Init(kUndoables_UndoHandlingMechanismMultiple, undoNameCFString, redoNameCFString);
		if (nullptr != undoNameCFString) CFRelease(undoNameCFString), undoNameCFString = nullptr;
		if (nullptr != redoNameCFString) CFRelease(redoNameCFString), redoNameCFString = nullptr;
	}
	
	// install recording handlers
	// UNIMPLEMENTED - if recording setup fails, notify the user
	UNUSED_RETURN(RecordAE_Result)RecordAE_Init();
}// initMacOSToolbox

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
