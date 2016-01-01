/*!	\file IconManager.h
	\brief NOTE: This module is not really needed anymore; it
	previously allowed seamless integration of multiple icon
	representations, especially legacy formats.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2016 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
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

#ifndef __ICONMANAGER__
#define __ICONMANAGER__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef struct OpaqueIconManagerIcon*	IconManagerIconRef;



#pragma mark Public Methods

//!\name Creating, Initializing and Destroying Icon References
//@{

IconManagerIconRef
	IconManager_NewIcon						();

void
	IconManager_DisposeIcon					(IconManagerIconRef*		inoutRefPtr);

OSStatus
	IconManager_MakeIconRefFromBundleFile	(IconManagerIconRef			inRef,
											 CFStringRef				inFileNameWithoutExtension,
											 OSType						inIconServicesCreator,
											 OSType						inIconServicesDescription);

//@}

//!\name Icons in User Interface Elements
//@{

OSStatus
	IconManager_SetButtonIcon				(ControlRef					inControl,
											 IconManagerIconRef			inRef);

OSStatus
	IconManager_SetMenuTitleIcon			(MenuRef					inMenu,
											 IconManagerIconRef			inRef);

OSStatus
	IconManager_SetToolbarItemIcon			(HIToolbarItemRef			inItem,
											 IconManagerIconRef			inRef);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
