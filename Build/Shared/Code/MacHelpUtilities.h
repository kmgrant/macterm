/*!	\file MacHelpUtilities.h
	\brief Slightly simplifies interactions with Apple Help.
	
	Initialize this routine with the name of your application’s
	help system, so that you don’t have to specify it on
	subsequent calls that perform a search and launch the
	Help Viewer.
*/
/*###############################################################

	Contexts Library
	© 1998-2017 by Kevin Grant
	
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

#include <UniversalDefines.h>

#ifndef __MACHELPUTILITIES__
#define __MACHELPUTILITIES__

// Mac includes
#include <CoreFoundation/CoreFoundation.h>



#pragma mark Public Methods

void
	MacHelpUtilities_Init								(CFStringRef		inAppleTitleOfHelpFile);

OSStatus
	MacHelpUtilities_LaunchHelpSystemWithSearch			(CFStringRef		inSearchString);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
