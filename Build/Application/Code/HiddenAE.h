/*!	\file HiddenAE.h
	\brief Implements MacTelnet Suite events NOT listed in
	MacTelnet’s Dictionary.
	
	These events are used by MacTelnet Help to perform tasks for
	the user (for example, displaying a dialog box).
*/
/*###############################################################

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

#ifndef __HIDDENAE__
#define __HIDDENAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

pascal OSErr
	HiddenAE_HandleConsoleWriteLine		(AppleEvent const*		inAppleEventPtr,
										 AppleEvent*			outReplyAppleEventPtr,
										 SInt32					inData);

pascal OSErr
	HiddenAE_HandleDisplayDialog		(AppleEvent const*		inAppleEventPtr,
										 AppleEvent*			outReplyAppleEventPtr,
										 SInt32					inData);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
