/*###############################################################

	HelpSystem.cp
	
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

// standard-C++ includes
#include <map>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include "MacHelpUtilities.h"

// resource includes
#include "StringResources.h"

// MacTelnet includes
#include "EventLoop.h"
#include "HelpSystem.h"
#include "UIStrings.h"



#pragma mark Types

typedef std::map< WindowRef, HelpSystem_KeyPhrase >		WindowToKeyPhraseMap;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowToKeyPhraseMap&	gWindowToKeyPhraseMap()		{ static WindowToKeyPhraseMap x; return x; }
}

#pragma mark Internal Method Prototypes

static HelpSystem_ResultCode	copyCFStringHelpSearch			(HelpSystem_KeyPhrase, CFStringRef&);
static HelpSystem_ResultCode	displayHelpFromKeyPhrase		(HelpSystem_KeyPhrase);
static HelpSystem_ResultCode	displayMainHelp					();
static HelpSystem_KeyPhrase		getCurrentContextKeyPhrase 		();



#pragma mark Public Methods

/*!
Opens MacTelnet Help and displays a page appropriate
for the given key phrase.  Normally, this is how you
would handle a click in a dialog box help button.

DEPRECATED.

\retval kHelpSystem_ResultCodeSuccess
if help has been displayed successfully

\retval kHelpSystem_ResultCodeNoSuchString
if the key phrase is invalid

\retval kHelpSystem_ResultCodeCannotDisplayHelp
if an OS error occurred

(3.0)
*/
HelpSystem_ResultCode
HelpSystem_DisplayHelpFromKeyPhrase		(HelpSystem_KeyPhrase	inKeyPhrase)
{
	HelpSystem_ResultCode	result = kHelpSystem_ResultCodeSuccess;
	
	
	if (inKeyPhrase == kHelpSystem_KeyPhraseDefault) result = displayMainHelp(); // same as Òwithout contextÓ
	else result = displayHelpFromKeyPhrase(inKeyPhrase); // open help in context
	
	return result;
}// DisplayHelpFromKeyPhrase


/*!
Opens MacTelnet Help and displays a page appropriate
for the current context.  Typically you set a context
(say, when a window opens) using the routine
HelpSystem_SetCurrentContextKeyPhrase(), and then you
display help for that context with this routine (say,
when the user clicks a help button).

(3.0)
*/
HelpSystem_ResultCode
HelpSystem_DisplayHelpInCurrentContext ()
{
	HelpSystem_ResultCode	result = kHelpSystem_ResultCodeSuccess;
	
	
	result = displayHelpFromKeyPhrase(getCurrentContextKeyPhrase());
	if (result != kHelpSystem_ResultCodeSuccess) result = displayMainHelp(); // fall back on table of contents view
	return result;
}// DisplayHelpInCurrentContext


/*!
Displays the main table of contents of the help system;
that is, help content without any contextual focus.

\retval kHelpSystem_ResultCodeSuccess
if the context was changed successfully

\retval kHelpSystem_ResultCodeCannotRetrieveString
if an OS error occurred

(3.0)
*/
HelpSystem_ResultCode
HelpSystem_DisplayHelpWithoutContext ()
{
	return displayMainHelp();
}// DisplayHelpWithoutContext


/*!
Returns the key phrase for the current context.  At
the moment this module only defines contexts at the
window level, so this means the return value
corresponds to the key phrase of the frontmost non-
floating window, if any.

If the return value is ÒkHelpSystem_KeyPhraseDefaultÓ,
there is no particular context set.  This would mean
a table-of-contents view should be displayed when help
is provided to the user.

(3.0)
*/
HelpSystem_KeyPhrase
HelpSystem_GetCurrentContextKeyPhrase ()
{
	return getCurrentContextKeyPhrase();
}// GetCurrentContextKeyPhrase

/*!
Locates the specified key phrase string, and returns
a copy of it; thus, you must call CFRelease() when
you are finished with it.  If you pass the value
ÒkHelpSystem_KeyPhraseDefaultÓ, the string will
match the help book name.

\retval kHelpSystem_ResultCodeSuccess
if the string is found and returned successfully

\retval kHelpSystem_ResultCodeNoSuchString
if the key phrase is invalid

\retval kHelpSystem_ResultCodeCannotRetrieveString
if an OS error occurred

(3.0)
*/
HelpSystem_ResultCode
HelpSystem_CopyKeyPhraseCFString	(HelpSystem_KeyPhrase	inKeyPhrase,
									 CFStringRef&			outString)
{
	return copyCFStringHelpSearch(inKeyPhrase, outString);
}// GetKeyPhraseCFString


/*!
Sets the current help context of the specified window.
You can clear a windowÕs context by passing in the
default value, "kHelpSystem_KeyPhraseDefault"; you
should always default the value just before destroying
the window, to ensure this module can clean up its
internal cache.

\retval kHelpSystem_ResultCodeSuccess
if the context was changed successfully

(3.0)
*/
HelpSystem_ResultCode
HelpSystem_SetWindowKeyPhrase	(WindowRef				inWindow,
								 HelpSystem_KeyPhrase	inKeyPhrase)
{
	HelpSystem_ResultCode			result = kHelpSystem_ResultCodeSuccess;
	WindowToKeyPhraseMap::iterator	windowToKeyPhraseIterator;
	
	
	windowToKeyPhraseIterator = gWindowToKeyPhraseMap().find(inWindow);
	if (inKeyPhrase == kHelpSystem_KeyPhraseDefault)
	{
		if (windowToKeyPhraseIterator != gWindowToKeyPhraseMap().end())
		{
			// default key phrase; delete this windowÕs key phrase
			gWindowToKeyPhraseMap().erase(windowToKeyPhraseIterator);
			assert(gWindowToKeyPhraseMap().find(inWindow) == gWindowToKeyPhraseMap().end());
		}
	}
	else
	{
		// register a new windowÕs key phrase
		gWindowToKeyPhraseMap()[inWindow] = inKeyPhrase;
		assert(gWindowToKeyPhraseMap().find(inWindow) != gWindowToKeyPhraseMap().end());
	}
	
	return result;
}// SetWindowKeyPhrase


#pragma mark Internal Methods

/*!
Locates the specified key phrase and copies it
into the given buffer.

\retval kHelpSystem_ResultCodeSuccess
if the string is copied successfully

\retval kHelpSystem_ResultCodeNoSuchString
if no key phrase with the given ID exists

\retval kHelpSystem_ResultCodeCannotRetrieveString
if an OS error occurred

(3.0)
*/
static HelpSystem_ResultCode
copyCFStringHelpSearch	(HelpSystem_KeyPhrase	inKeyPhrase,
						 CFStringRef&			outString)
{
	HelpSystem_ResultCode	result = kHelpSystem_ResultCodeSuccess;
	UIStrings_ResultCode	stringError = kUIStrings_ResultCodeSuccess;
	
	
	switch (inKeyPhrase)
	{
	case kHelpSystem_KeyPhraseCommandLine:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpUsingTheCommandLine, outString);
		break;
	
	case kHelpSystem_KeyPhraseConnections:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpCreatingSessions, outString);
		break;
	
	case kHelpSystem_KeyPhraseDuplicate:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpWithPreferences/* TEMPORARY */,
										outString);
		break;
	
	case kHelpSystem_KeyPhraseFavorites:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpWithSessionFavorites, outString);
		break;
	
	case kHelpSystem_KeyPhraseFind:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpSearchingForText, outString);
		break;
	
	case kHelpSystem_KeyPhraseFormatting:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpWithScreenFormatting, outString);
		break;
	
	case kHelpSystem_KeyPhraseKioskSetup:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpWithKioskSetup, outString);
		break;
	
	case kHelpSystem_KeyPhraseMacros:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpUsingMacros, outString);
		break;
	
	case kHelpSystem_KeyPhrasePreferences:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpWithPreferences, outString);
		break;
	
	case kHelpSystem_KeyPhraseScreenSize:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpSettingTheScreenSize, outString);
		break;
	
	case kHelpSystem_KeyPhraseSpecialKeys:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpSettingKeyMappings,outString);
		break;
	
	case kHelpSystem_KeyPhraseTerminals:
		stringError = UIStrings_Copy(kUIStrings_HelpSystemTopicHelpWithTerminalSettings, outString);
		break;
	
	case kHelpSystem_KeyPhraseDefault:
	default:
		// ???
		if (inKeyPhrase != kHelpSystem_KeyPhraseDefault) result = kHelpSystem_ResultCodeNoSuchString;
		stringError = UIStrings_Copy(kUIStrings_HelpSystemContextualHelpCommandName, outString);
		break;
	}
	return result;
}// copyCFStringHelpSearch


/*!
Opens MacTelnet Help and displays a page appropriate
for the given key phrase.

\retval kHelpSystem_ResultCodeSuccess
if help has been displayed successfully

\retval kHelpSystem_ResultCodeNoSuchString
if the key phrase is invalid

\retval kHelpSystem_ResultCodeCannotDisplayHelp
if an OS error occurred

(3.0)
*/
static HelpSystem_ResultCode
displayHelpFromKeyPhrase	(HelpSystem_KeyPhrase	inKeyPhrase)
{
	HelpSystem_ResultCode	result = kHelpSystem_ResultCodeSuccess;
	CFStringRef				searchString = nullptr;
	
	
	result = copyCFStringHelpSearch(inKeyPhrase, searchString);
	if (result == kHelpSystem_ResultCodeSuccess)
	{
		OSStatus	error = MacHelpUtilities_LaunchHelpSystemWithSearch(searchString);
		
		
		if (error != noErr) result = kHelpSystem_ResultCodeCannotDisplayHelp;
	}
	
	return result;
}// displayHelpFromKeyPhrase


/*!
Displays the main table of contents for the help
system - that is, no particular context is
considered for help.

\retval kHelpSystem_ResultCodeSuccess
if help has been displayed successfully

\retval kHelpSystem_ResultCodeCannotDisplayHelp
if an OS error occurred

(3.0)
*/
static HelpSystem_ResultCode
displayMainHelp ()
{
	HelpSystem_ResultCode	result = kHelpSystem_ResultCodeSuccess;
	UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
	CFStringRef				helpBookNameString = nullptr;
	OSStatus				error = noErr;
	
	
	stringResult = UIStrings_Copy(kUIStrings_HelpSystemName, helpBookNameString);
	if (false == stringResult.ok()) error = memFullErr;
	else
	{
		error = AHGotoPage(helpBookNameString, nullptr/* specific path */, nullptr/* page anchor */);
		CFRelease(helpBookNameString);
	}
	
	if (error != noErr) result = kHelpSystem_ResultCodeCannotDisplayHelp;
	return result;
}// displayMainHelp


/*!
Returns the key phrase for the current context,
or the default phrase if there is no particular
context.

(3.0)
*/
static HelpSystem_KeyPhrase
getCurrentContextKeyPhrase ()
{
	HelpSystem_KeyPhrase	result = kHelpSystem_KeyPhraseDefault;
	WindowRef				frontWindow = EventLoop_GetRealFrontWindow();
	
	
	if (frontWindow != nullptr)
	{
		// see if the frontmost window has a context associated with it
		WindowToKeyPhraseMap::const_iterator	windowToKeyPhraseIterator =
												gWindowToKeyPhraseMap().find(frontWindow);
		
		
		if (windowToKeyPhraseIterator != gWindowToKeyPhraseMap().end())
		{
			result = windowToKeyPhraseIterator->second;
		}
	}
	return result;
}// getCurrentContextKeyPhrase

// BELOW IS REQUIRED NEWLINE TO END FILE
