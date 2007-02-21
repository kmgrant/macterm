/*!	\file IconManager.h
	\brief The most powerful icon utility there is - it allows
	you to seamlessly use traditional icon suites, new Icon
	Services icons, or other formats from a single reference!
	
	But more than that, it doesn’t matter *how* you describe
	your icon - for example, if it’s convenient to specify it as
	an old-style color icon, go ahead - on Mac OS 8.0 and 8.1, a
	suitable icon resource will automatically be used, and yet
	on Mac OS 9 and X Icon Services will be used to retrieve an
	icon of higher quality.  In short, everything about the
	“real” icon is handled at runtime.  Your code can use
	new-style specifications such as Icon Services, or old-style
	resource-based requests, and BOTH will work on ANY version
	of the Mac OS.  Is that powerful or what?  (This feature is
	limited to the code’s internal knowledge of the available
	icons; feel free to extend the code to ensure other icons
	you want to use will work in this way.)
*/
/*###############################################################

	Interface Library 2.0
	© 1998-2006 by Kevin Grant
	
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

#include "UniversalDefines.h"

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
	IconManager_MakeIconRef					(IconManagerIconRef			inRef,
											 SInt16						inIconServicesVolumeNumber,
											 OSType						inIconServicesCreator,
											 OSType						inIconServicesDescription);

OSStatus
	IconManager_MakeIconRefFromBundleFile	(IconManagerIconRef			inRef,
											 CFStringRef				inFileNameWithoutExtension,
											 OSType						inIconServicesCreator,
											 OSType						inIconServicesDescription);

OSStatus
	IconManager_MakeIconSuite				(IconManagerIconRef			inRef,
											 SInt16						inSuiteResourceID,
											 IconSelectorValue			inWhichIcons);

OSStatus
	IconManager_MakeOldColorIcon			(IconManagerIconRef			inRef,
											 SInt16						inIconResourceID_cicn);

//@}

//!\name Icon Inquiries
//@{

Boolean
	IconManager_IsIconServices				(IconManagerIconRef			inRef);

Boolean
	IconManager_IsIconSuite					(IconManagerIconRef			inRef);

Boolean
	IconManager_IsOldColorIcon				(IconManagerIconRef			inRef);

//@}

//!\name Acquiring Icon Data
//@{

// RETURNS DATA COERCED TO A HANDLE; USE IconManager_Is...() METHODS TO FIND THE ACTUAL DATA TYPE!!!
Handle
	IconManager_GetData						(IconManagerIconRef			inRef);

//@}

//!\name Drawing Icons
//@{

OSStatus
	IconManager_PlotIcon					(IconManagerIconRef			inRef,
											 Rect const*				inBounds,
											 IconAlignmentType			inAlignment,
											 IconTransformType			inTransform);

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
