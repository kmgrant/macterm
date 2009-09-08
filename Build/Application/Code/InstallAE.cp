/*###############################################################

	InstallAE.cp
	
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
#include <cctype>
#include <cstring>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlocks.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "ConstantsRegistry.h"
#include "InstallAE.h"
#include "RequiredAE.h"
#include "Terminology.h"
#include "URLAccessAE.h"



#pragma mark Internal Method Prototypes
namespace {

Boolean		installRequiredHandlers			();
OSStatus	installURLAccessEventHandlers	();

} // anonymous namespace



#pragma mark Public Methods

/*!
This method initializes the Apple Event handlers
for the Required Suite, and creates an address
descriptor so MacTelnet can send events to itself
(for recordability).

If any problems occur, false is returned and the
application should be considered impossible to
start up!

You must call AppleEvents_Done() to clean up
the memory allocations, etc. performed by a call
to AppleEvents_Init() when you are done with
Apple Events (that is, when the program ends).

(3.0)
*/
Boolean
InstallAE_Init ()
{
	Boolean		result = true;
	OSStatus	error = noErr;
	
	
	result = installRequiredHandlers();
	if (result)
	{
		error = installURLAccessEventHandlers();
		result = (noErr == error);
	}
	
	return result;
}// Init


/*!
Cleans up this module.

(3.0)
*/
void
InstallAE_Done ()
{
	// could remove and dispose handlers; UNIMPLEMENTED
}// Done


#pragma mark Internal Methods
namespace {

/*!
This routine installs Apple Event handlers for
the Required Suite, including Appearance events
and the six standard Apple Events (open and
print documents, open, re-open and quit
application, and display preferences).

If ALL handlers install successfully, "true" is
returned; otherwise, "false" is returned.

(3.0)
*/
Boolean
installRequiredHandlers ()
{
	Boolean			result = false;
	OSStatus		error = noErr;
	AEEventClass	eventClass = '----';
	
	
	eventClass = kASRequiredSuite;
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEOpenApplication,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationOpen),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEReopenApplication,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationReopen),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEOpenDocuments,
														NewAEEventHandlerUPP(RequiredAE_HandleOpenDocuments),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEPrintDocuments,
														NewAEEventHandlerUPP(RequiredAE_HandlePrintDocuments),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEQuitApplication,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationQuit),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEShowPreferences,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationPreferences),
														0L, false);
	if (noErr == error)
	{
		// ignore any errors for these
		eventClass = kAppearanceEventClass;
		(OSStatus)AEInstallEventHandler(eventClass, kAEAppearanceChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
		(OSStatus)AEInstallEventHandler(eventClass, kAESystemFontChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
		(OSStatus)AEInstallEventHandler(eventClass, kAESmallSystemFontChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
		(OSStatus)AEInstallEventHandler(eventClass, kAEViewsFontChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
	}
	
	result = (noErr == error);
	
	return result;
}// installRequiredHandlers


/*!
This routine installs Apple Event handlers for
the URL Suite (URL Access).

If any handler cannot be installed, an error
is returned and some handlers may not be
installed properly.

(3.0)
*/
OSStatus
installURLAccessEventHandlers ()
{
	OSStatus			result = noErr;
	AEEventClass		eventClass = kStandardURLSuite;
	AEEventHandlerUPP	upp = nullptr;
	
	
	// install event handlers for every supported URL event
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(URLAccessAE_HandleUniformResourceLocator);
		result = AEInstallEventHandler(eventClass, kStandardURLEventIDGetURL, upp, 0L, false);
	}
	
	return result;
}// installURLAccessEventHandlers

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
