/*!	\file IconManager.cp
	\brief The most powerful icon utility there is - it allows
	you to seamlessly use traditional icon suites, new Icon
	Services icons, or other formats from a single reference!
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2011 by Kevin Grant
	
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
	kIconManagerIconTypeOS7 = -1,
	kIconManagerIconTypeOS8 = 0,
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
		CIconHandle		OS7;
		IconSuiteRef	OS8;
		IconRef			OSX;
	} data;
};
typedef struct IconManagerIcon		IconManagerIcon;
typedef IconManagerIcon*			IconManagerIconPtr;

} // anonymous namespace

#pragma mark Variables
namespace {

Boolean		gHaveIconServices = false;

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
		
		
		ptr->type = kIconManagerIconTypeOS8;
		ptr->data.OS7 = nullptr;
		ptr->data.OS8 = nullptr;
		ptr->data.OSX = nullptr;
		refReleaseLock(result, &ptr);
	}
	
	// without an Init routine, this test has to be done every time an icon is created
	{
		OSStatus	error = noErr;
		SInt32		gestaltResult = 0L;
		
		
		error = Gestalt(gestaltIconUtilitiesAttr, &gestaltResult);
		if (error == noErr)
		{
			// Icon Services?
			gHaveIconServices = (gestaltResult & (1 << gestaltIconUtilitiesHasIconServices));
		}
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
To determine if an abstract icon reference defines an
icon based on modern Mac OS icon data, use this method.
If the application is presently running Mac OS 8.0 or
8.1, this routine will never return true.

(1.0)
*/
Boolean
IconManager_IsIconServices	(IconManagerIconRef		inRef)
{
	Boolean					result = false;
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		result = ((ptr->type == kIconManagerIconTypeOSX) && gHaveIconServices);
		refReleaseLock(inRef, &ptr);
	}
	return result;
}// IsIconServices


/*!
To determine if an abstract icon reference defines an
icon based on legacy Mac OS icon suite resources, use
this method.

(1.0)
*/
Boolean
IconManager_IsIconSuite		(IconManagerIconRef		inRef)
{
	Boolean					result = false;
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		result = (ptr->type == kIconManagerIconTypeOS8);
		refReleaseLock(inRef, &ptr);
	}
	return result;
}// IsIconSuite


/*!
To determine if an abstract icon reference defines an
icon based on legacy Mac OS 'cicn' data, use this method.

(1.0)
*/
Boolean
IconManager_IsOldColorIcon	(IconManagerIconRef		inRef)
{
	Boolean					result = false;
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		result = (ptr->type == kIconManagerIconTypeOS7);
		refReleaseLock(inRef, &ptr);
	}
	return result;
}// IsOldColorIcon


/*!
To force an abstract icon to render itself based on data
in an Icon Services set, use this method.  The specified
icon reference must already exist.

If Mac OS 8.0 or 8.1 is in use, Icon Services will not be
available.  To avoid problems, this routine automatically
looks for an equivalent icon suite resource and uses it
instead.  If no icon of any kind can be found, the result
is "resNotFound".

(1.0)
*/
OSStatus
IconManager_MakeIconRef		(IconManagerIconRef		inRef,
							 SInt16					inIconServicesVolumeNumber,
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
		if (gHaveIconServices)
		{
			// Icon Services is available; use it!
			ptr->type = kIconManagerIconTypeOSX;
			result = GetIconRef(inIconServicesVolumeNumber, inIconServicesCreator,
									inIconServicesDescription, &ptr->data.OSX);
		}
		else
		{
			SInt16		suiteID = 0;
			Boolean		useColorIconResourceInstead = false; // use 'cicn' instead of icon families?
			
			
			if ((inIconServicesVolumeNumber == kOnSystemDisk) &&
				(inIconServicesCreator == kSystemIconsCreator))
			{
				// NOTE:	This switch statement is infinitely expandable; I’ve only
				//			included generic icons that I need.  The list should be
				//			made more complete as necessary, to include mappings for
				//			other icon suites.  Heck, the implementation should
				//			probably be different, but I don’t have time for that!
				switch (inIconServicesDescription)
				{
				case kAlertStopIcon:
					suiteID = 0;
					useColorIconResourceInstead = true;
					break;
				
				case kAlertNoteIcon:
					suiteID = 1;
					useColorIconResourceInstead = true;
					break;
				
				case kAlertCautionIcon:
					suiteID = 2;
					useColorIconResourceInstead = true;
					break;
				
				case kGenericApplicationIcon:
					suiteID = kGenericApplicationIconResource;
					break;
				
				case kGenericDocumentIcon:
					suiteID = kGenericDocumentIconResource;
					break;
				
				case kGenericFolderIcon:
					suiteID = kGenericFolderIconResource;
					break;
				
				case kHelpIcon:
					suiteID = kHelpIconResource;
					break;
				
				default:
					break;
				}
			}
			
			if (suiteID == 0) result = resNotFound;
			else
			{
				if (useColorIconResourceInstead)
				{
					ptr->type = kIconManagerIconTypeOS7;
					ptr->data.OS7 = GetCIcon(suiteID);
					result = (ptr->data.OS7 == nullptr)
								? STATIC_CAST(resNotFound, OSStatus)
								: STATIC_CAST(noErr, OSStatus); // correct?
				}
				else
				{
					ptr->type = kIconManagerIconTypeOS8;
					result = GetIconSuite(&ptr->data.OS8, suiteID, kSelectorAllAvailableData);
				}
			}
		}
		refReleaseLock(inRef, &ptr);
	}
	
	return result;
}// MakeIconRef


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
		if (gHaveIconServices)
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
		else
		{
			result = resNotFound;
		}
		refReleaseLock(inRef, &ptr);
	}
	
	return result;
}// MakeIconRefFromBundleFile


/*!
To draw an icon properly no matter how it is internally
defined, use this convenient method.  If an error occurs,
the result will depend on the nature of the icon being
drawn.

(1.0)
*/
OSStatus
IconManager_PlotIcon	(IconManagerIconRef		inRef,
						 Rect const*			inBounds,
						 IconAlignmentType		inAlignment,
						 IconTransformType		inTransform)
{
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	OSStatus				result = noErr;
	
	
	if (ptr != nullptr)
	{
		switch (ptr->type)
		{
		case kIconManagerIconTypeOS7:	// treat it as an old-style color icon
			if (ptr->data.OS7 == nullptr) result = memPCErr;
			else
			{
				result = PlotCIconHandle(inBounds, inAlignment, inTransform, ptr->data.OS7);
			}
			break;
		
		case kIconManagerIconTypeOS8:	// treat it as an icon suite
			if (ptr->data.OS8 == nullptr) result = memPCErr;
			else
			{
				result = PlotIconSuite(inBounds, inAlignment, inTransform, ptr->data.OS8);
			}
			break;
		
		case kIconManagerIconTypeOSX:	// use Icon Services
			if (gHaveIconServices)
			{
				if (ptr->data.OS8 == nullptr) result = memPCErr;
				else
				{
					result = PlotIconRef(inBounds, inAlignment, inTransform,
											kIconServicesNormalUsageFlag, ptr->data.OSX);
				}
			}
			else
			{
				result = cfragNoSymbolErr;
			}
			break;
		
		default:
			break;
		}
		refReleaseLock(inRef, &ptr);
	}
	
	return result;
}// PlotIcon


/*!
Returns a reference to the internal icon, coerced to
a Handle.  Use IconManager_Is...() methods to determine
what type of data this is (IconSuite or IconRef).

(1.0)
*/
Handle
IconManager_ReturnData	(IconManagerIconRef		inRef)
{
	Handle					result = false;
	IconManagerIconPtr		ptr = refAcquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		if ((ptr->type == kIconManagerIconTypeOSX) && gHaveIconServices)
		{
			result = (Handle)ptr->data.OSX;
		}
		else if (ptr->type == kIconManagerIconTypeOS8)
		{
			result = (Handle)ptr->data.OS8;
		}
		else
		{
			result = (Handle)ptr->data.OS7;
		}
		refReleaseLock(inRef, &ptr);
	}
	return result;
}// ReturnData


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
		
		
		if (IconManager_IsIconServices(inRef))
		{
			contentInfo.contentType = kControlContentIconRef;
			contentInfo.u.iconRef = ptr->data.OSX;
		}
		else if (IconManager_IsOldColorIcon(inRef))
		{
			contentInfo.contentType = kControlContentCIconHandle;
			contentInfo.u.cIconHandle = ptr->data.OS7;
		}
		else
		{
			contentInfo.contentType = kControlContentIconSuiteHandle;
			contentInfo.u.iconSuite = ptr->data.OS8;
		}
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
		if (IconManager_IsIconServices(inRef))
		{
			(OSStatus)SetMenuTitleIcon(inMenu, kMenuIconRefType, ptr->data.OSX);
		}
		else if (IconManager_IsOldColorIcon(inRef))
		{
			// UNSUPPORTED
			result = unimpErr;
		}
		else
		{
			(OSStatus)SetMenuTitleIcon(inMenu, kMenuIconSuiteType, ptr->data.OS8);
		}
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
		if (IconManager_IsIconServices(inRef))
		{
			result = HIToolbarItemSetIconRef(inItem, ptr->data.OSX);
		}
		else
		{
			result = paramErr;
		}
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
		if (inPtr->type == kIconManagerIconTypeOS7)
		{
			if (inPtr->data.OS7 != nullptr)
			{
				DisposeCIcon(inPtr->data.OS7), inPtr->data.OS7 = nullptr;
			}
		}
		else if (inPtr->type == kIconManagerIconTypeOS8)
		{
			if (inPtr->data.OS8 != nullptr)
			{
				(OSStatus)DisposeIconSuite(inPtr->data.OS8, true/* dispose data too */);
				inPtr->data.OS8 = nullptr;
			}
		}
		else if (inPtr->type == kIconManagerIconTypeOSX)
		{
			if (inPtr->data.OSX != nullptr)
			{
				(OSStatus)ReleaseIconRef(inPtr->data.OSX);
				inPtr->data.OSX = nullptr;
			}
		}
	}
}// releaseIcons

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
