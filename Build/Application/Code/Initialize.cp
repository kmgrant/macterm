/*!	\file Initialize.cp
	\brief Setup and teardown for all modules that
	require it (and cannot do it just-in-time).
*/
/*###############################################################

	MacTelnet
		© 1998-2009 by Kevin Grant.
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
#include <Cursors.h>
#include <Localization.h>
#include <MacHelpUtilities.h>
#include <MemoryBlocks.h>
#include <Releases.h>
#include <Undoables.h>

// resource includes
#include "ApplicationVersion.h"

// MacTelnet includes
#include "AppResources.h"
#include "Clipboard.h"
#include "Commands.h"
#include "ContextualMenuBuilder.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "FileUtilities.h"
#include "Folder.h"
#include "InfoWindow.h"
#include "Initialize.h"
#include "InternetPrefs.h"
#include "MainEntryPoint.h"
#include "Preferences.h"
#include "PrefsWindow.h"
#include "RasterGraphicsKernel.h"
#include "RasterGraphicsScreen.h"
#include "RecordAE.h"
#include "SessionFactory.h"
#include "TerminalBackground.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "TextTranslation.h"
#include "UIStrings.h"



#pragma mark Internal Method Prototypes

static void		initApplicationCore		();
static void		initMacOSToolbox		();



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
	
	// set the application bundle so everything searches in the right place for resources
	AppResources_Init(inApplicationBundle);
	
	// initialize Cocoa
	EventLoop_Init();
	
	// initialize memory manager, start up toolbox managers, etc.
	initMacOSToolbox();
	
	// on Mac OS X the following call also stops the bouncing of MacTelnet’s Dock icon
	//FlushEvents(everyEvent/* events to flush */, 0/* events to not flush */);
	
	// QuickTime?
	FlagManager_Set(kFlagQuickTime, true); // always available on Mac OS X
	
	initApplicationCore();
	
#ifndef NDEBUG
	// the Console must be among the first things initialized if
	// it is to be used for debugging of these early modules
	{
		char		dateString[40];
		char		timeString[40];
		UInt32		date = 0L;
		
		
		// initialize the console and write initial information (the
		// date and time, the console name, the web site URL, etc.)
		Console_WriteLine("Starting up.");
		GetDateTime(&date);
		DateString(STATIC_CAST(date, SInt32), longDate, REINTERPRET_CAST(dateString, StringPtr), nullptr/* intlHandle */);
		StringUtilities_PToCInPlace(REINTERPRET_CAST(dateString, StringPtr));
		TimeString(STATIC_CAST(date, SInt32), true/* want seconds */, REINTERPRET_CAST(timeString, StringPtr), nullptr/* intlHandle */);
		StringUtilities_PToCInPlace(REINTERPRET_CAST(timeString, StringPtr));
		CPP_STD::strcat(dateString, " ");
		CPP_STD::strcat(dateString, timeString);
		Console_WriteLine(dateString);
	}
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
	
	// set wait cursor
	Cursors_UseWatch();
	
	// do everything else
	{
		SessionFactory_Init();
	#if RUN_MODULE_TESTS
		//SessionFactory_RunTests();
	#endif
		
		TextTranslation_Init();
	#if RUN_MODULE_TESTS
		//TextTranslation_RunTests();
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
		
		Clipboard_Init();
	#if RUN_MODULE_TESTS
		//Clipboard_RunTests();
	#endif
		
		InfoWindow_Init(); // installs command handler to enable this window to be displayed and hidden
	#if RUN_MODULE_TESTS
		//InfooWindow_RunTests();
	#endif
		
		RasterGraphicsKernel_Init(); // ICR setup
	#if RUN_MODULE_TESTS
		//RasterGraphicsKernel_RunTests();
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
				UInt32		newCommandID = kCommandNewSessionDefaultFavorite;
				
				
				// assume that the user is mapping command-N to the same type of session
				// that would be appropriate for opening by default on startup
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
			}
		}
	}
	
	// now set a flag indicating that initialization succeeded
	FlagManager_Set(kFlagInitializationComplete, true);
	
	// fix an apparent Panther bug where the application is not activated when launched
	if (FlagManager_Test(kFlagOS10_3API))
	{
		ProcessSerialNumber		psn;
		
		
		if (GetCurrentProcess(&psn) == noErr)
		{
			(OSStatus)SetFrontProcess(&psn);
		}
	}
	
	// set default cursor
	Cursors_UseArrow();
}// ApplicationStartup


/*!
Undoes the main things that Initialize_ApplicationStartup()
does, in reverse order.

(3.0)
*/
void
Initialize_ApplicationShutdown ()
{
	// saving user preferences is a top priority, since other quitting procedures could cause a crash
	PrefsWindow_Done();
	
	Clipboard_Done();
	InfoWindow_Done();
	
	InternetPrefs_Done();
	
#ifndef NDEBUG
	Console_Done();
#endif
	
	TerminalView_Done();
	TerminalBackground_Done();
	Commands_Done();
	SessionFactory_Done();
	TextTranslation_Done();
	Alert_Done();
	
	Preferences_Done();
	Cursors_Done();
	Undoables_Done();
	EventLoop_Done();
	FlagManager_Done();
}// ApplicationShutdown


#pragma mark Internal Methods

/*!
This method initializes key modules (both
internally and from MacTelnet’s libraries),
installs Apple Event handlers for the
Apple Events that MacTelnet supports and
requires, and reads user preferences.

If errors occur at this stage, this method
may not return, displaying an error message
to the user.

(3.0)
*/
static void
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
		if (false/* I can’t think of an appropriate Script Manager variable for determining this */)
		{
			// then the text reads from the bottom of a page to the top (the opposite of North America)
			flags |= kLocalization_InitFlagReadTextBottomToTop;
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
	
	ContextualMenuBuilder_SetMenuHelpType(kCMHelpItemOtherHelp);
	
	// initialize cursors
	Cursors_Init();
	
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
needed by MacTelnet 3.0, and allocates a memory
reserve.

If errors occur at this stage, this method may
not return.  If this routine actually returns,
then the user’s computer is at least minimally
capable of running MacTelnet 3.0.  However, more
tests are done in initApplicationCore()...

(3.0)
*/
static void
initMacOSToolbox ()
{
	// initialize MacTelnet’s memory module, providing a grow zone for emergency situations;
	// NOTE: this will probably already have been called indirectly at static initialization
	// time (for operator new[], etc.) so the following call may have no effect
	Memory_Init();
	
	FlagManager_Init();
	
	// register as an Appearance client
	RegisterAppearanceClient();
	
	// Launch Services seems to recommend this, so do it
	LSInit(kLSInitializeDefaults);
	
	// At least Mac OS 8 is now required.  Is Mac OS 8.5 or
	// later being used?  If so, use the cool new stuff
	// where possible.
	{
		long	gestaltResult = 0L;
		UInt8   majorRev = 0;
		UInt8   minorRev = 0;
		
		
		(OSStatus)Gestalt(gestaltSystemVersion, &gestaltResult);
		majorRev = Releases_ReturnMajorRevisionForVersion(gestaltResult);
		minorRev = Releases_ReturnMinorRevisionForVersion(gestaltResult);
		
		// any advanced APIs available?
		FlagManager_Set(kFlagOS10_0API, true); // this source tree is Mac OS X only
		FlagManager_Set(kFlagOS10_1API, (((majorRev == 0x0A) && (minorRev >= 0x01)) || (majorRev > 0x0A)));
		FlagManager_Set(kFlagOS10_2API, (((majorRev == 0x0A) && (minorRev >= 0x02)) || (majorRev > 0x0A)));
		FlagManager_Set(kFlagOS10_3API, (((majorRev == 0x0A) && (minorRev >= 0x03)) || (majorRev > 0x0A)));
		FlagManager_Set(kFlagOS10_4API, (((majorRev == 0x0A) && (minorRev >= 0x04)) || (majorRev > 0x0A)));
		FlagManager_Set(kFlagOS10_5API, (((majorRev == 0x0A) && (minorRev >= 0x05)) || (majorRev > 0x0A)));
		FlagManager_Set(kFlagOS10_6API, (((majorRev == 0x0A) && (minorRev >= 0x06)) || (majorRev > 0x0A)));
	}
	
	InitCursor();
	
	// initialization of the Alert module must be done here, otherwise startup error alerts can’t be displayed
	AppResources_SetResFile(kAppResources_FileIDPreferences, -1);
	Alert_Init(CurResFile());
	
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
	(RecordAE_Result)RecordAE_Init();
}// initMacOSToolbox

// BELOW IS REQUIRED NEWLINE TO END FILE
