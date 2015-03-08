/*!	\file IconManager.cp
	\brief NOTE: This module is not really needed anymore; it
	previously allowed seamless integration of multiple icon
	representations, especially legacy formats.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2015 by Kevin Grant
	
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

#include <IconManager.h>
#include <UniversalDefines.h>

// standard-C includes
#include <cstring>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlocks.h>

// application includes
#include "AppResources.h"
#include "ConstantsRegistry.h"



#pragma mark Constants
namespace {

typedef SInt16		IconManagerIconType;
enum
{
	kIconManagerIconTypeOSX = 1
};

} // anonymous namespace

#pragma mark Types
namespace {

struct IconManagerIcon
{
	IconManagerIconType		type;
	union
	{
		IconRef		OSX;
	} data;
};
typedef struct IconManagerIcon		IconManagerIcon;
typedef IconManagerIcon*			IconManagerIconPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

IconManagerIconPtr	refAcquireLock		(IconManagerIconRef);
void				refReleaseLock		(IconManagerIconRef, IconManagerIconPtr*);
void				releaseIcons		(IconManagerIconPtr);

} // anonymous namespace



#pragma mark Public Methods

/*!
To create a new abstract icon, use this method.
The icon is uninitialized; use one of the
IconManager_MakeIcon...() routines to set up an
icon using specific data.

(1.0)
*/
IconManagerIconRef
IconManager_NewIcon ()
{
	IconManagerIconRef	result = REINTERPRET_CAST(Memory_NewPtr(sizeof(IconManagerIcon)),
													IconManagerIconRef);
	
	
	if (result != nullptr)
	{
		IconManagerIconPtr	ptr = refAcquireLock(result);
		
		
		ptr->type = kIconManagerIconTypeOSX;
		ptr->data.OSX = nullptr;
		refReleaseLock(result, &ptr);
	}
	
	return result;
}// NewIcon


/*!
To destroy an abstract icon when you are finished
using it, use this method.

(1.0)
*/
void
IconManager_DisposeIcon		(IconManagerIconRef*	inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		IconManagerIconPtr		ptr = refAcquireLock(*inoutRefPtr);
		
		
		releaseIcons(ptr);
		DisposePtr((Ptr)ptr), ptr = nullptr, *inoutRefPtr = nullptr;
	}
}// DisposeIcon


/*!
To force an abstract icon to render itself based on data
in a ".icns" file in the main application bundle, use this
method.  The specified icon reference must already exist.

This is Mac OS X only.  On Mac OS 8 the result is
"resNotFound".  If the file cannot be found the result is
"fnfErr".

(1.1)
*/
OSStatus
IconManager_MakeIconRefFromBundleFile	(IconManagerIconRef		inRef,
										 CFStringRef			inFileNameWithoutExtension,
										 OSType					inIconServicesCreator,
										 OSType					inIconServicesDescription)
{
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	OSStatus				result = noErr;
	
	
	if (ptr == nullptr) result = memPCErr;
	else
	{
		// get rid of any previous icon data
		releaseIcons(ptr);
		
		// verify that Icon Services can be used; if it can’t be, look for an equivalent icon suite
		{
			FSRef	iconFile;
			
			
			// Icon Services is available; use it!
			ptr->type = kIconManagerIconTypeOSX;
			
			// try to locate the requested file in the bundle
			if (AppResources_GetArbitraryResourceFileFSRef
				(inFileNameWithoutExtension, CFSTR("icns"), iconFile))
			{
				result = RegisterIconRefFromFSRef(inIconServicesCreator, inIconServicesDescription,
													&iconFile, &ptr->data.OSX);
			}
			else
			{
				result = fnfErr;
			}
		}
		
		refReleaseLock(inRef, &ptr);
	}
	
	return result;
}// MakeIconRefFromBundleFile


/*!
To conveniently set up any Appearance control that
can take standard content types such as icons,
pictures, etc. so it uses the data defined by an
abstract Icon Manager Icon, use this method.  You
can use this, for example, to set the icon for a
bevel button.

Automatically, if a 32-bit Icon Services icon is
available (and Icon Services is available), the
better icon will be used (and the content type
for the button will be set appropriately).  If
not, an Icon Suite will be used instead.

(1.0)
*/
OSStatus
IconManager_SetButtonIcon	(ControlRef				inControl,
							 IconManagerIconRef		inRef)
{
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	OSStatus				result = noErr;
	
	
	if (ptr == nullptr) result = memPCErr;
	else
	{
		ControlButtonContentInfo	contentInfo;
		
		
		contentInfo.contentType = kControlContentIconRef;
		contentInfo.u.iconRef = ptr->data.OSX;
		result = SetControlData(inControl, kControlNoPart, kControlBevelButtonContentTag,
								sizeof(contentInfo), &contentInfo);
		refReleaseLock(inRef, &ptr);
	}
	
	return result;
}// SetButtonIcon


/*!
To conveniently set the title of a menu so that it
uses the data defined by an abstract Icon Manager
Icon, use this method.  You can use this, for
example, to set the icon for a Scripts menu.

Automatically, if a 32-bit Icon Services icon is
available (and Icon Services is available), the
better icon will be used (and the content type
for the button will be set appropriately).  If
not, an Icon Suite will be used instead.

(1.0)
*/
OSStatus
IconManager_SetMenuTitleIcon	(MenuRef				inMenu,
								 IconManagerIconRef		inRef)
{
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	OSStatus				result = noErr;
	
	
	if (ptr == nullptr) result = memPCErr;
	else
	{
		// Make the menu’s title iconic.
		UNUSED_RETURN(OSStatus)SetMenuTitleIcon(inMenu, kMenuIconRefType, ptr->data.OSX);
		refReleaseLock(inRef, &ptr);
	}
	
	return result;
}// SetMenuTitleIcon


/*!
Conveniently sets up the icon image of an HIToolbar
item to use the icon for the specified Icon Manager
reference.

Unlike other routines, this does not work if the
underlying data is anything but a Mac OS X icon
(IconRef).

(2.0)
*/
OSStatus
IconManager_SetToolbarItemIcon	(HIToolbarItemRef		inItem,
								 IconManagerIconRef		inRef)
{
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	OSStatus				result = noErr;
	
	
	if (ptr == nullptr) result = memPCErr;
	else
	{
		HIToolbarItemSetIconRef(inItem, ptr->data.OSX);
		refReleaseLock(inRef, &ptr);
	}
	
	return result;
}// SetToolbarItemIcon


#pragma mark Internal Methods
namespace {

/*!
To acquire a pointer to the internal structure,
given a reference to it, use this method.

(1.0)
*/
IconManagerIconPtr
refAcquireLock	(IconManagerIconRef		inRef)
{
	return ((IconManagerIconPtr)inRef);
}// refAcquireLock


/*!
To release a pointer to the internal structure,
possessing a reference to it, use this method.
On return, your copy of the pointer is set to
nullptr.

(1.0)
*/
void
refReleaseLock	(IconManagerIconRef		UNUSED_ARGUMENT(inRef),
				 IconManagerIconPtr*	inoutPtr)
{
	// the memory is not relocatable, so do nothing with the reference
	if (inoutPtr != nullptr) *inoutPtr = nullptr;
}// refReleaseLock


/*!
To dispose of icon data in an abstract icon,
use this method.  You generally do this just
prior to replacing the data with something
else.

(1.0)
*/
void
releaseIcons	(IconManagerIconPtr		inPtr)
{
	if (inPtr != nullptr)
	{
		if (inPtr->type == kIconManagerIconTypeOSX)
		{
			if (inPtr->data.OSX != nullptr)
			{
				UNUSED_RETURN(OSStatus)ReleaseIconRef(inPtr->data.OSX);
				inPtr->data.OSX = nullptr;
			}
		}
	}
}// releaseIcons

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
