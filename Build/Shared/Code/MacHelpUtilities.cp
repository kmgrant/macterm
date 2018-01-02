/*!	\file MacHelpUtilities.cp
	\brief Slightly simplifies interactions with Apple Help.
*/
/*###############################################################

	Contexts Library
	© 1998-2018 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
    This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.

    You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "MacHelpUtilities.h"
#include <UniversalDefines.h>

// Mac includes
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// library includes
#include "CFRetainRelease.h"



#pragma mark Variables
namespace {

CFRetainRelease		gHelpBookAppleTitleRetainer;	//!< string for help book name; used to focus searches better

} // anonymous namespace



#pragma mark Public Methods

/*!
Initializes this module with the name of the
help book, used for search queries on Mac OS X.
The given string should match the "AppleTitle"
meta-tag in your HTML help.

The given string is retained so you do not
have to worry about deleting it.

(1.0)
*/
void
MacHelpUtilities_Init	(CFStringRef	inAppleTitleOfHelpFile)
{
	gHelpBookAppleTitleRetainer.setWithRetain(inAppleTitleOfHelpFile);
}// Init


/*!
Launches the Apple Help Viewer and searches the application
help system (as defined by a called to MacHelpUtilities_Init())
using the given query string.

(1.0)
*/
OSStatus
MacHelpUtilities_LaunchHelpSystemWithSearch		(CFStringRef	inSearchString)
{
	OSStatus	result = 0L;
	
	
	result = AHSearch(gHelpBookAppleTitleRetainer.returnCFStringRef()/* help book to search, or nullptr for all books */, inSearchString);
	return result;
}// LaunchHelpSystemWithSearch

// BELOW IS REQUIRED NEWLINE TO END FILE
