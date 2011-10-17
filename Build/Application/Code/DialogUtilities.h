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

	MacTerm
		© 1998-2011 by Kevin Grant.
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



#pragma mark Public Methods

#define	BooleanToCheckBoxValue(a)			((a) ? (kControlCheckBoxCheckedValue) : (kControlCheckBoxUncheckedValue))

#define	BooleansToCheckBoxValue(a,b)		(((a) && (b)) \
												? (kControlCheckBoxCheckedValue) \
												: (((a) || (b)) \
													? (kControlCheckBoxMixedValue) \
													: (kControlCheckBoxUncheckedValue)))

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
	DialogUtilities_DisposeDuplicateControl	(ControlRef				inDuplicatedControl);

// DISPOSE WITH DialogUtilities_DisposeDuplicateControl()
OSStatus
	DialogUtilities_DuplicateControl		(ControlRef				inTemplateControl,
											 WindowRef				inDestinationWindow,
											 ControlRef&			outNewControl);

void
	GetControlNumericalText					(ControlRef				inControl,
											 SInt32*				outNumberPtr,
											 size_t*				outStringLengthPtrOrNull = nullptr);

void
	GetControlOSTypeText					(ControlRef				inControl,
											 OSType*				outTypePtr);

void
	GetControlText							(ControlRef				inControl,
											 Str255					outText);

void
	GetControlTextAsCFString				(ControlRef				inControl,
											 CFStringRef&			outCFString);

class DialogUtilities_HIViewIDEqualTo; // STL Unary Function - returns: bool, argument: HIViewRef

//! since the Carbon implementation of IsControlHilited() isn’t smart enough to consider the inactive case, this API exists...
#define IsControlActiveAndHilited(inControlHandle)			((GetControlHilite(inControlHandle) > 0) && (GetControlHilite(inControlHandle) < 255))

ControlKeyFilterResult
	NumericalLimiter						(ControlRef				inControl,
											 SInt16*				inKeyCode,
											 SInt16*				inCharCode,
											 EventModifiers*		inModifiers);

ControlKeyFilterUPP
	NumericalLimiterKeyFilterUPP			();

void
	OutlineRegion							(RgnHandle				inoutRegion);

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

OSStatus
	DialogUtilities_SetSegmentByCommand		(HIViewRef				inSegmentedView,
											 UInt32					inCommandID);

HIViewWrap&
	DialogUtilities_SetUpHelpButton			(HIViewWrap&			inoutView);

void
	TextFontByName							(ConstStringPtr			inFontName);

ControlKeyFilterResult
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
