/*!	\file WindowInfo.h
	\brief Attaches auxiliary data to Mac OS windows to make
	them easier to deal with generically.
	
	NOTE:   This module is deprecated on Mac OS X.  It was made
			originally to address serious deficiencies in the
			Window Manager and other toolbox APIs, but any
			Mac OS X application can use other methods to do
			the things Window Info does.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2015 by Kevin Grant
	
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

#ifndef __WINDOWINFO__
#define __WINDOWINFO__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef FourCharCode WindowInfo_Descriptor;
enum
{
	kWindowInfo_InvalidDescriptor = '----'
};

#pragma mark Types

typedef struct WindowInfo_OpaqueStruct*		WindowInfo_Ref;



#pragma mark Public Methods

//!\name Creating, Initializing and Destroying Window Info Data
//@{

WindowInfo_Ref
	WindowInfo_New								();

void
	WindowInfo_Dispose							(WindowInfo_Ref					inoutWindowFeaturesRef);

//@}

//!\name Associations With Windows
//@{

WindowInfo_Ref
	WindowInfo_ReturnFromWindow					(HIWindowRef					inWindow);

void
	WindowInfo_SetForWindow						(HIWindowRef					inWindow,
												 WindowInfo_Ref					inWindowFeaturesRef);

//@}

//!\name Accessing Data
//@{

void*
	WindowInfo_ReturnAuxiliaryDataPtr			(WindowInfo_Ref					inWindowFeaturesRef);

WindowInfo_Descriptor
	WindowInfo_ReturnWindowDescriptor			(WindowInfo_Ref					inWindowFeaturesRef);

void
	WindowInfo_SetAuxiliaryDataPtr				(WindowInfo_Ref					inWindowFeaturesRef,
												 void*							inAuxiliaryDataPtr);

void
	WindowInfo_SetWindowDescriptor				(WindowInfo_Ref					inWindowFeaturesRef,
												 WindowInfo_Descriptor			inNewWindowDescriptor);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
