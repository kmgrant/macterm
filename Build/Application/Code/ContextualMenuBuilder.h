/*!	\file ContextualMenuBuilder.h
	\brief Configuration and display of context-sensitive
	pop-up menus.
	
	Currently, this module handles all contextual menus,
	but ideally some of this behavior would eventually be
	handled within more appropriate modules (the Terminal
	Window module might handle terminal window menus, for
	example).
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

#ifndef __CONTEXTUALMENUBUILDER__
#define __CONTEXTUALMENUBUILDER__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

OSStatus
	ContextualMenuBuilder_DisplayMenuForView	(HIViewRef			inWhichView,
												 EventRef			inContextualMenuClickEvent);

OSStatus
	ContextualMenuBuilder_DisplayMenuForWindow	(HIWindowRef		inWhichWindow,
												 EventRecord*		inoutEventPtr,
												 WindowPartCode		inWindowPart);

OSStatus
	ContextualMenuBuilder_PopulateMenuForView	(HIViewRef			inWhichView,
												 EventRef			inContextualMenuClickEvent,
												 MenuRef			inoutMenu,
												 AEDesc&			inoutViewContentsDesc);

OSStatus
	ContextualMenuBuilder_PopulateMenuForWindow	(HIWindowRef		inWhichWindow,
												 EventRecord*		inoutEventPtrOrNull,
												 WindowPartCode		inWindowPart,
												 MenuRef			inoutMenu,
												 AEDesc&			inoutWindowContentsDesc);

UInt32
	ContextualMenuBuilder_ReturnMenuHelpType	();

void
	ContextualMenuBuilder_SetMenuHelpType		(UInt32				inNewHelpType);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
