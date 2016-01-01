/*!	\file WindowInfo.cp
	\brief Attaches auxiliary data to Mac OS windows to make
	them easier to deal with generically.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2016 by Kevin Grant
	
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

#include <WindowInfo.h>
#include <UniversalDefines.h>

// standard-C includes
#include <climits>

// library includes
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// application includes
#include "AppResources.h"
#include "ConstantsRegistry.h"



#pragma mark Types
namespace {

struct My_WindowInfo
{
	WindowInfo_Descriptor	windowDescriptor;
	void*					auxiliaryDataPtr;
};
typedef struct My_WindowInfo	My_WindowInfo;
typedef struct My_WindowInfo*	My_WindowInfoPtr;

typedef MemoryBlockHandleLocker< WindowInfo_Ref, My_WindowInfo >	My_WindowInfoHandleLocker;
typedef LockAcquireRelease< WindowInfo_Ref, My_WindowInfo >			My_WindowInfoAutoLocker;

} // anonymous namespace

#pragma mark Variables
namespace {

My_WindowInfoHandleLocker	gWindowInfoHandleLocks;

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new Window Info object.  To assign it to a window,
call WindowInfo_SetForWindow().  To destroy it later, call
WindowInfo_Dispose().

(1.0)
*/
WindowInfo_Ref
WindowInfo_New ()
{
	WindowInfo_Ref				result = REINTERPRET_CAST(Memory_NewHandle(sizeof(My_WindowInfo)), WindowInfo_Ref);
	My_WindowInfoAutoLocker		windowInfoPtr(gWindowInfoHandleLocks, result);
	
	
	if (nullptr != windowInfoPtr)
	{
		windowInfoPtr->windowDescriptor = kWindowInfo_InvalidDescriptor;
		windowInfoPtr->auxiliaryDataPtr = nullptr;
	}
	return result;
}// New


/*!
Destroys data created by WindowInfo_New().

WARNING:	This invalidates the reference.  If you previously
			used WindowInfo_SetAuxiliaryDataPtr() to store data,
			call WindowInfo_ReturnAuxiliaryDataPtr() to retrieve
			that data before calling WindowInfo_Dispose().

(1.0)
*/
void
WindowInfo_Dispose	(WindowInfo_Ref		inWindowInfoRef)
{
	if (inWindowInfoRef != nullptr)
	{
		Memory_DisposeHandle(REINTERPRET_CAST(&inWindowInfoRef, Handle*));
	}
}// Dispose


/*!
Returns an auxiliary data pointer previously stored by a call
to WindowInfo_SetAuxiliaryDataPtr().

(1.0)
*/
void*
WindowInfo_ReturnAuxiliaryDataPtr	(WindowInfo_Ref		inWindowInfoRef)
{
	My_WindowInfoAutoLocker		windowInfoPtr(gWindowInfoHandleLocks, inWindowInfoRef);
	void*						result = nullptr;
	
	
	result = windowInfoPtr->auxiliaryDataPtr;
	return result;
}// ReturnAuxiliaryDataPtr


/*!
Returns a reference previously set by WindowInfo_SetForWindow().

The Window Info information is assumed to exist as a specific
property on the window.  If that property is not present,
nullptr is returned.

(1.0)
*/
WindowInfo_Ref
WindowInfo_ReturnFromWindow		(HIWindowRef	inWindow)
{
	WindowInfo_Ref		result = nullptr;
	
	
	if (nullptr != inWindow)
	{
		OSStatus	error = noErr;
		
		
		error = GetWindowProperty(inWindow, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_WindowPropertyTypeWindowInfoRef,
									sizeof(result), nullptr/* actual size */, &result);
		if (noErr != error)
		{
			result = nullptr;
		}
	}
	return result;
}// ReturnFromWindow


/*!
Returns a descriptor for a Window Info reference, which can
help to distinguish different types of windows for example.
See also WindowInfo_SetWindowDescriptor().

(1.0)
*/
WindowInfo_Descriptor
WindowInfo_ReturnWindowDescriptor	(WindowInfo_Ref		inWindowInfoRef)
{
	WindowInfo_Descriptor		result = kWindowInfo_InvalidDescriptor;
	My_WindowInfoAutoLocker		windowInfoPtr(gWindowInfoHandleLocks, inWindowInfoRef);
	
	
	if (nullptr != windowInfoPtr)
	{
		result = (windowInfoPtr->windowDescriptor);
	}
	return result;
}// ReturnWindowDescriptor


/*!
Associates arbitrary data with a window that can be retrieved
later by WindowInfo_ReturnAuxiliaryDataPtr().

(1.0)
*/
void
WindowInfo_SetAuxiliaryDataPtr	(WindowInfo_Ref		inWindowInfoRef,
								 void*				inAuxiliaryDataPtr)
{
	My_WindowInfoAutoLocker		windowInfoPtr(gWindowInfoHandleLocks, inWindowInfoRef);
	
	
	windowInfoPtr->auxiliaryDataPtr = inAuxiliaryDataPtr;
}// SetAuxiliaryDataPtr


/*!
Attaches a Window Info object to a window as a property.  See
also WindowInfo_ReturnFromWindow().

(1.0)
*/
void
WindowInfo_SetForWindow	(HIWindowRef		inWindow,
						 WindowInfo_Ref		inWindowInfoRef)
{
	OSStatus	error = noErr;
	
	
	error = SetWindowProperty(inWindow, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_WindowPropertyTypeWindowInfoRef,
								sizeof(inWindowInfoRef), &inWindowInfoRef);
	assert(noErr == error);
}// SetForWindow


/*!
Attaches an identifier for a Window Info reference, which can
help to distinguish different types of windows for example.
See also WindowInfo_ReturnWindowDescriptor().

(1.0)
*/
void
WindowInfo_SetWindowDescriptor	(WindowInfo_Ref			inWindowInfoRef,
								 WindowInfo_Descriptor	inNewWindowDescriptor)
{
	My_WindowInfoAutoLocker		windowInfoPtr(gWindowInfoHandleLocks, inWindowInfoRef);
	
	
	windowInfoPtr->windowDescriptor = inNewWindowDescriptor;
}// SetWindowDescriptor

