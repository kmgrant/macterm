/*!	\file DataAccessAE.h
	\brief Routines for accessing data via Apple Events.
	
	For example, “set data”, “get data” and “get data size”
	events are handled here.
	
	Internally, this module uses "ObjectClassesAE.h" structures.
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

#ifndef __DATAACCESSAE__
#define __DATAACCESSAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

pascal OSErr
	DataAccessAE_HandleGetData			(AppleEvent const*	inAppleEventPtr,
										 AppleEvent*		outReplyAppleEventPtr,
										 SInt32				inData);

pascal OSErr
	DataAccessAE_HandleGetDataSize		(AppleEvent const*	inAppleEventPtr,
										 AppleEvent*		outReplyAppleEventPtr,
										 SInt32				inData);

pascal OSErr
	DataAccessAE_HandleSetData			(AppleEvent const*	inAppleEventPtr,
										 AppleEvent*		outReplyAppleEventPtr,
										 SInt32				inData);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
