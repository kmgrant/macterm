/*!	\file DialogUtilities.h
	\brief Lists methods that simplify window management,
	dialog box management, and window content control management.
	
	Actually, this module contains a ton of routines that really
	should be part of the Mac OS, but aren’t.
	
	NOTE:	Over time, this header has increasingly become a
			dumping ground for APIs that do not seem to have a
			better place.  This will eventually be fixed.
*/
/*###############################################################

	MacTelnet
		© 1998-2009 by Kevin Grant.
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

#ifndef __DIALOGUTILITIES__
#define __DIALOGUTILITIES__

// standard-C++ includes
#include <functional>
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <HIViewWrap.fwd.h>
#include <WindowInfo.h>



#pragma mark Callbacks

/*!
Device Loop Draw-Control Routine

Used to enforce type safety when using the
DeviceLoopClipAndDrawControl() routine.
Specifically, instead of having a normal
DeviceLoopDrawingUPP, an internal interface
to DeviceLoop() is used that casts the user
data to an appropriate type and then invokes
this strongly-typed callback.
*/
typedef void (*DialogUtilities_DrawViewProcPtr)	(short				inColorDepth,
												 short				inDeviceFlags,
												 GDHandle			inTargetDevice,
												 ControlRef			inControl,
												 ControlPartCode	inControlPartCode);
inline void
DialogUtilities_InvokeDrawViewProc	(DialogUtilities_DrawViewProcPtr	inProc,
									 short								inColorDepth,
									 short								inDeviceFlags,
									 GDHandle							inTargetDevice,
									 ControlRef							inControl,
									 ControlPartCode					inControlPartCode)
{
	(*inProc)(inColorDepth, inDeviceFlags, inTargetDevice, inControl, inControlPartCode);
}



#pragma mark Public Methods

#define	BooleanToCheckBoxValue(a)			((a) ? (kControlCheckBoxCheckedValue) : (kControlCheckBoxUncheckedValue))

#define	BooleanToRadioButtonValue(a)		((a) ? (kControlRadioButtonCheckedValue) : (kControlRadioButtonUncheckedValue))

OSStatus
	DialogUtilities_CreateControlsBasedOnWindowNIB	(CFStringRef			inNIBFileBasename,
											 CFStringRef					inPanelNameInNIB,
											 WindowRef						inDestinationWindow,
											 ControlRef						inDestinationParent,
											 std::vector< ControlRef >&		inoutControlArray,
											 Rect&							outIdealBounds);

void
	DialogUtilities_DisposeControlsBasedOnWindowNIB		(std::vector< ControlRef > const&	inControlArray);

void
	DeactivateFrontmostWindow				();

void
	DebugSelectControlHierarchyDumpFile		(WindowRef				inForWindow);

void
	DeviceLoopClipAndDraw					(RgnHandle				inNewClipRegion,
											 DeviceLoopDrawingUPP	inProc,
											 long					inDeviceLoopUserData,
											 DeviceLoopFlags		inDeviceLoopFlags);

void
	DeviceLoopClipAndDrawControl			(ControlRef						inControl,
											 SInt16							inControlPartCode,
											 DialogUtilities_DrawViewProcPtr inProc,
											 Boolean						inExtendClipBoundaryForFocusRing = false);

void
	DialogUtilities_DisposeDuplicateControl	(ControlRef				inDuplicatedControl);

// DISPOSE WITH DialogUtilities_DisposeDuplicateControl()
OSStatus
	DialogUtilities_DuplicateControl		(ControlRef				inTemplateControl,
											 WindowRef				inDestinationWindow,
											 ControlRef&			outNewControl);

Boolean
	FlashButtonControl						(ControlRef				inControl,
											 Boolean				inIsDefaultButton,
											 Boolean				inIsCancelButton);

void
	GetControlNumericalText					(ControlRef				inControl,
											 SInt32*				outNumberPtr);

void
	GetControlOSTypeText					(ControlRef				inControl,
											 OSType*				outTypePtr);

void
	GetControlText							(ControlRef				inControl,
											 Str255					outText);

void
	GetControlTextAsCFString				(ControlRef				inControl,
											 CFStringRef&			outCFString);

void
	GetNewDialogWithWindowInfo				(SInt16					inDialogResourceID,
											 DialogRef*				outDialog,
											 WindowInfo_Ref*		outWindowInfoRefPtr);

class DialogUtilities_HIViewIDEqualTo; // STL Unary Function - returns: bool, argument: HIViewRef

pascal ControlKeyFilterResult
	HostNameLimiter							(ControlRef				inControl,
											 SInt16*				inKeyCode,
											 SInt16*				inCharCode,
											 EventModifiers*		inModifiers);

ControlKeyFilterUPP
	HostNameLimiterKeyFilterUPP				();

//! since the Carbon implementation of IsControlHilited() isn’t smart enough to consider the inactive case, this API exists...
#define IsControlActiveAndHilited(inControlHandle)			((GetControlHilite(inControlHandle) > 0) && (GetControlHilite(inControlHandle) < 255))

pascal ControlKeyFilterResult
	NoSpaceLimiter							(ControlRef				inControl,
											 SInt16*				inKeyCode,
											 SInt16*				inCharCode,
											 EventModifiers*		inModifiers);

ControlKeyFilterUPP
	NoSpaceLimiterKeyFilterUPP				();

pascal ControlKeyFilterResult
	NumericalLimiter						(ControlRef				inControl,
											 SInt16*				inKeyCode,
											 SInt16*				inCharCode,
											 EventModifiers*		inModifiers);

ControlKeyFilterUPP
	NumericalLimiterKeyFilterUPP			();

pascal ControlKeyFilterResult
	OSTypeLengthLimiter						(ControlRef				inControl,
											 SInt16*				inKeyCode,
											 SInt16*				inCharCode,
											 EventModifiers*		inModifiers);

ControlKeyFilterUPP
	OSTypeLengthLimiterKeyFilterUPP			();

void
	OutlineRegion							(RgnHandle				inoutRegion);

void
	RestoreFrontmostWindow					();

void
	SetControlNumericalText					(ControlRef				inControl,
											 SInt32					inNumber);

void
	SetControlOSTypeText					(ControlRef				inControl,
											 OSType					inType);

void
	SetControlText							(ControlRef				inControl,
											 ConstStringPtr			inText);

void
	SetControlTextWithCFString				(ControlRef				inControl,
											 CFStringRef			inText);

OSStatus
	DialogUtilities_SetKeyboardFocus		(HIViewRef				inView);

OSStatus
	DialogUtilities_SetPopUpItemByCommand	(HIViewRef				inPopUpMenuView,
											 UInt32					inMenuCommandID);

OSStatus
	DialogUtilities_SetPopUpItemByText		(HIViewRef				inPopUpMenuView,
											 CFStringRef			inText,
											 MenuItemIndex			inFallbackSelection = 1);

HIViewWrap&
	DialogUtilities_SetUpHelpButton			(HIViewWrap&			inoutView);

OSStatus
	SetWindowAlternateTitleWithPString		(WindowRef				inWindow,
											 ConstStringPtr			inText);

void
	TextFontByName							(ConstStringPtr			inFontName);

pascal ControlKeyFilterResult
	UnixCommandLineLimiter					(ControlRef				inControl,
											 SInt16*				inKeyCode,
											 SInt16*				inCharCode,
											 EventModifiers*		inModifiers);

ControlKeyFilterUPP
	UnixCommandLineLimiterKeyFilterUPP		();



#pragma mark Functor Implementations

/*!
This is a functor that returns true if the specified
HIViewRef’s ID is equal to the one given at construction
time.

Model of STL Predicate.

(3.1)
*/
class DialogUtilities_HIViewIDEqualTo:
public std::unary_function< HIViewRef/* argument */, bool/* return */ >
{
public:
	DialogUtilities_HIViewIDEqualTo		(HIViewID const&	inID)
	: _viewID(inID)
	{
	}
	
	bool
	operator ()	(HIViewRef	inView)
	{
		bool		result = false;
		HIViewID	comparedID;
		OSStatus	error = GetControlID(inView, &comparedID);
		
		
		if (noErr == error)
		{
			result = ((comparedID.signature == _viewID.signature) &&
						(comparedID.id == _viewID.id));
		}
		
		return result;
	}

protected:

private:
	HIViewID	_viewID;
};

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
