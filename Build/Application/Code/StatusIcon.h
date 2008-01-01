/*!	\file StatusIcon.h
	\brief Implements a UI element that displays one icon out of
	a set of possible values.
	
	Icons only animate if they are set to animate; but, you can
	still create a series of “animation cells” for icons that
	do not animate continuously.  StatusIcon_SetNextCell() can
	then be used to update the icon display in a controlled
	manner.  Typically you create a series of icons representing
	all possible states of a Status Icon, make them all “stages
	of animation”, and then call StatusIcon_SetNextCell() to
	specify the current state.  The idle handler takes care of
	updating the display whether or not animation is occurring.
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

#ifndef __STATUSICON__
#define __STATUSICON__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

/*!
Every animation stage has a unique descriptor, which is a
4-character code.  There is one reserved descriptor, however,
for internal use.
*/
typedef FourCharCode StatusIconAnimationStageDescriptor;
enum
{
	kInvalidStatusIconAnimationStageDescriptor = FOUR_CHAR_CODE('----')		//!< never use this value
};

/*!
Icon part codes (used ONLY when dealing with icons directly,
as button controls).
*/
enum
{
	kControlPartCodeStatusIconVoid		= kControlNoPart,		//!< nowhere in a particular icon button
	kControlPartCodeStatusIconButton	= kControlButtonPart	//!< the entire structure region of an icon
};

#pragma mark Types

typedef struct OpaqueStatusIcon*		StatusIconRef;



#pragma mark Public Methods

StatusIconRef
	StatusIcon_New						(WindowRef							inOwningWindow);

void
	StatusIcon_Dispose					(StatusIconRef*						inoutRefPtr);

void
	StatusIcon_AddStageFromIconRef		(StatusIconRef						inRef,
										 StatusIconAnimationStageDescriptor	inDescriptor,
										 CFStringRef						inBundleResourceNameNoExtension,
										 OSType								inCreator,
										 OSType								inType,
										 UInt32								inAnimationDelayInTicks);

void
	StatusIcon_AddStageFromIconSuite	(StatusIconRef						inRef,
										 StatusIconAnimationStageDescriptor	inDescriptor,
										 SInt16								inResourceFile,
										 SInt16								inIconSuiteResourceID,
										 UInt32								inAnimationDelayInTicks);

void
	StatusIcon_AddStageFromOldColorIcon	(StatusIconRef						inRef,
										 StatusIconAnimationStageDescriptor	inDescriptor,
										 SInt16								inResourceFile,
										 SInt16								inIconResourceID,
										 UInt32								inAnimationDelayInTicks);

HIViewRef
	StatusIcon_ReturnContainerView		(StatusIconRef						inRef);

Boolean
	StatusIcon_IsAnimating				(StatusIconRef						inRef);

void
	StatusIcon_RemoveStage				(StatusIconRef						inRef,
										 StatusIconAnimationStageDescriptor	inDescriptor);

void
	StatusIcon_SetDimWhenInactive		(StatusIconRef						inRef,
										 Boolean							inIconShouldDimWhenInactive);

void
	StatusIcon_SetDoAnimate				(StatusIconRef						inRef,
										 Boolean							inIconShouldAnimate,
										 Boolean							inResetStages);

void
	StatusIcon_SetFocus					(StatusIconRef						inRef,
										 Boolean							inFocusRing);

void
	StatusIcon_SetNextCell				(StatusIconRef						inRef,
										 StatusIconAnimationStageDescriptor	inDescriptorOfNextCell);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
