/*!	\file CommonEventHandlers.cp
	\brief Simplifies writing handlers for very common
	kinds of events.
*/
/*###############################################################

	MacTerm
		© 1998-2013 by Kevin Grant.
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

#include "CommonEventHandlers.h"
#include <UniversalDefines.h>

// standard-C includes
#include <cctype>
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <CarbonEventHandlerWrap.template.h>
#include <Console.h>
#include <MemoryBlocks.h>
#include <MemoryBlockPtrLocker.template.h>
#include <SoundSystem.h>

// application includes
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "UIStrings.h"



#pragma mark Types
namespace {

/*!
This structure is used for CommonEventHandlers_InstallNumericalFieldArrowsHandler().
*/
struct My_NumericalFieldArrowsHandler
{
	ControlRef			field;						//!< the editable text control whose value is changed by the arrows
	EventHandlerRef		controlHitEventHandlerRef;	//!< reference retained so handler can be removed later
	EventHandlerUPP		controlHitEventHandlerUPP;	//!< reference retained so handler can be disposed later
};
typedef struct My_NumericalFieldArrowsHandler*		My_NumericalFieldArrowsHandlerPtr;

/*!
This structure is used for CommonEventHandlers_InstallPopUpMenuArrowsHandler().
*/
struct My_PopUpMenuArrowsHandler
{
	ControlRef			popUpMenu;					//!< the pop-up menu control whose value is changed by the arrows
	EventHandlerRef		controlHitEventHandlerRef;	//!< reference retained so handler can be removed later
	EventHandlerUPP		controlHitEventHandlerUPP;	//!< reference retained so handler can be disposed later
};
typedef struct My_PopUpMenuArrowsHandler*		My_PopUpMenuArrowsHandlerPtr;

typedef MemoryBlockPtrLocker< CommonEventHandlers_NumericalFieldArrowsRef, My_NumericalFieldArrowsHandler >	My_NumericalFieldArrowsHandlerPtrLocker;
typedef LockAcquireRelease< CommonEventHandlers_NumericalFieldArrowsRef, My_NumericalFieldArrowsHandler >	My_NumericalFieldArrowsHandlerAutoLocker;

typedef MemoryBlockPtrLocker< CommonEventHandlers_PopUpMenuArrowsRef, My_PopUpMenuArrowsHandler >	My_PopUpMenuArrowsHandlerPtrLocker;
typedef LockAcquireRelease< CommonEventHandlers_PopUpMenuArrowsRef, My_PopUpMenuArrowsHandler >		My_PopUpMenuArrowsHandlerAutoLocker;

} // anonymous namespace

/*!
This structure is used for CommonEventHandlers_HIViewResizer::install().
*/
class CommonEventHandlers_HIViewResizerImpl
{
public:
	CommonEventHandlers_HIViewResizerImpl	(HIViewRef									inForWhichView,
											 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
											 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
											 void*										inContext);
	
	CommonEventHandlers_ChangedBoundsEdges		edgesOfInterest;		//!< flags indicating which boundary edges trigger callbacks
	CommonEventHandlers_HIViewResizeProcPtr		procPtr;				//!< notified if the monitored control’s size is changed
	void*										context;				//!< context to pass to the resize handler
	CarbonEventHandlerWrap						resizeEventHandler;		//!< what the system invokes when resize events occur
};

/*!
This structure is used for CommonEventHandlers_WindowResizer::install().
*/
class CommonEventHandlers_WindowResizerImpl
{
public:
	CommonEventHandlers_WindowResizerImpl	(HIWindowRef								inForWhichWindow,
											 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
											 void*										inContext,
											 Float32									inMinimumWidth,
											 Float32									inMinimumHeight,
											 Float32									inMaximumWidth,
											 Float32									inMaximumHeight);
	
	CommonEventHandlers_HIWindowResizeProcPtr	procPtr;				//!< notified if the monitored window’s size is changed
	void*										context;				//!< context to pass to the resize handler
	Float32										minimumWidth;			//!< smallest pixel width allowed for the window
	Float32										maximumWidth;			//!< biggest pixel width allowed for the window
	Float32										idealWidth;				//!< zoom standard-state pixel width (or 0, if never specified)
	Float32										minimumHeight;			//!< smallest pixel height allowed for the window
	Float32										maximumHeight;			//!< biggest pixel height allowed for the window
	Float32										idealHeight;			//!< zoom standard-state pixel height (or 0, if never specified)
	CarbonEventHandlerWrap						resizeEventHandler;		//!< what the system invokes when resize events occur
};

#pragma mark Variables
namespace {

My_NumericalFieldArrowsHandlerPtrLocker&	gMyNumericalFieldArrowsHandlerPtrLocks ()	{ static My_NumericalFieldArrowsHandlerPtrLocker x; return x; }
My_PopUpMenuArrowsHandlerPtrLocker&			gMyPopUpMenuArrowsHandlerPtrLocks ()	{ static My_PopUpMenuArrowsHandlerPtrLocker x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSStatus	receiveArrowHitForNumericalField	(EventHandlerCallRef, EventRef, void*);
OSStatus	receiveArrowHitForPopUpMenu			(EventHandlerCallRef, EventRef, void*);
OSStatus	receiveHIViewResizeOrSizeQuery		(EventHandlerCallRef, EventRef, void*);
OSStatus	receiveWindowResizeOrSizeQuery		(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace


#pragma mark Public Methods

/*!
Installs a Carbon Event handler for bounds-changed
events on the given view, and does all the “dirty
work” of detecting view resize activity.

The specified handler routine is then only invoked if
the view is being resized; and, the input to the
routine consists of useful delta-X and delta-Y values
that help when rearranging views.

Dispose of the handler later by passing the returned
reference into CommonEventHandlers_HIViewResizer::remove().

\retval noErr
if the handler was installed successfully

\retval memPCErr
if the given pointer to storage is nullptr

\retval memFullErr
if there is not enough memory to create a handler

\retval (other)
if the handler could not be installed

(3.0)
*/
OSStatus
CommonEventHandlers_HIViewResizer::
installAs	(HIViewRef									inForWhichView,
			 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
			 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
			 void*										inContext,
			 CommonEventHandlers_HIViewResizerImpl*&	outHandlerPtr)
{
	OSStatus	result = noErr;
	
	
	try
	{
		outHandlerPtr = new CommonEventHandlers_HIViewResizerImpl(inForWhichView, inEdgesOfInterest,
																	inResizeProc, inContext);
	}
	catch (std::bad_alloc)
	{
		result = memFullErr;
	}
	
	return result;
}// HIViewResizer::installAs


/*!
Removes a Carbon Event handler for bounds-changed
events that was previously added using the
CommonEventHandlers_HIViewResizer::install() routine.

(3.1)
*/
void
CommonEventHandlers_HIViewResizer::
remove	(CommonEventHandlers_HIViewResizerImpl*&	inoutImplPtr)
{
	if (nullptr != inoutImplPtr)
	{
		delete inoutImplPtr, inoutImplPtr = nullptr;
	}
}// HIViewResizer::remove


/*!
Internal implementation class for the public class
"CommonEventHandlers_HIViewResizer".

(3.1)
*/
CommonEventHandlers_HIViewResizerImpl::
CommonEventHandlers_HIViewResizerImpl	(HIViewRef									inForWhichView,
										 CommonEventHandlers_ChangedBoundsEdges		inEdgesOfInterest,
										 CommonEventHandlers_HIViewResizeProcPtr	inResizeProc,
										 void*										inContext)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
edgesOfInterest(inEdgesOfInterest),
procPtr(inResizeProc),
context(inContext),
resizeEventHandler(HIViewGetEventTarget(inForWhichView), receiveHIViewResizeOrSizeQuery,
					CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlBoundsChanged),
					this/* user data */)
{
}// 4-argument constructor


/*!
Returns the maximum size set either at construction
time or via a call to setWindowMaximumSize().

\retval noErr
if the maximum size was returned successfully

\retval memPCErr
if the given pointer is invalid

(3.1)
*/
OSStatus
CommonEventHandlers_WindowResizer::
getWindowMaximumSize	(Float32&	outMaximumWidth,
						 Float32&	outMaximumHeight)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == _impl) result = memPCErr;
	else
	{
		outMaximumWidth = _impl->maximumWidth;
		outMaximumHeight = _impl->maximumHeight;
	}
	
	return result;
}// WindowResizer::getWindowMaximumSize


/*!
Installs a Carbon Event handler for bounds-changed,
get-maximum-size, get-minimum-size and get-ideal-size
events on the given window, and does all the “dirty
work” of detecting window resize activity, etc.

The specified handler routine is then only invoked if
the window is being resized; and, the input to the
routine consists of useful delta-X and delta-Y values
that help when rearranging content controls.

Dispose of the handler later by passing the returned
reference into CommonEventHandlers_WindowResizer::remove().

\retval noErr
if the handler was installed successfully

\retval memFullErr
if there is not enough memory to create a handler

\retval (other)
if the handler could not be installed

(3.0)
*/
OSStatus
CommonEventHandlers_WindowResizer::
installAs	(HIWindowRef								inForWhichWindow,
			 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
			 void*										inContext,
			 Float32									inMinimumWidth,
			 Float32									inMinimumHeight,
			 Float32									inMaximumWidth,
			 Float32									inMaximumHeight,
			 CommonEventHandlers_WindowResizerImpl*&	outHandlerPtr)
{
	OSStatus	result = noErr;
	
	
	try
	{
		outHandlerPtr = new CommonEventHandlers_WindowResizerImpl(inForWhichWindow, inResizeProc, inContext, inMinimumWidth, inMinimumHeight,
																	inMaximumWidth, inMaximumHeight);
	}
	catch (std::bad_alloc)
	{
		result = memFullErr;
	}
	
	return result;
}// WindowResizeHandler::installAs


/*!
Removes a Carbon Event handler for bounds-changed
events that was previously added using the
CommonEventHandlers_WindowResizer::install() routine.

(3.1)
*/
void
CommonEventHandlers_WindowResizer::
remove	(CommonEventHandlers_WindowResizerImpl*&	inoutImplPtr)
{
	if (nullptr != inoutImplPtr)
	{
		delete inoutImplPtr, inoutImplPtr = nullptr;
	}
}// WindowResizer::remove


/*!
Ensures that windows managed by the specified resizer
return the indicated dimensions when asked for the
ideal size.  See CommonEventHandlers_WindowResizer::install().

\retval noErr
if the ideal size was updated successfully

\retval memPCErr
if the given pointer is invalid

(3.1)
*/
OSStatus
CommonEventHandlers_WindowResizer::
setWindowIdealSize	(Float32	inIdealWidth,
					 Float32	inIdealHeight)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == _impl) result = memPCErr;
	else
	{
		_impl->idealWidth = inIdealWidth;
		_impl->idealHeight = inIdealHeight;
	}
	
	return result;
}// WindowResizer::setWindowIdealSize


/*!
Ensures that windows managed by the specified resizer
return the indicated dimensions when asked for the
maximum size.  See CommonEventHandlers_WindowResizer::install().

\retval noErr
if the maximum size was updated successfully

\retval memPCErr
if the given pointer is invalid

(3.1)
*/
OSStatus
CommonEventHandlers_WindowResizer::
setWindowMaximumSize	(Float32	inMaximumWidth,
						 Float32	inMaximumHeight)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == _impl) result = memPCErr;
	else
	{
		_impl->maximumWidth = inMaximumWidth;
		_impl->maximumHeight = inMaximumHeight;
	}
	
	return result;
}// WindowResizer::setWindowMaximumSize


/*!
Ensures that windows managed by the specified resizer
return the indicated dimensions when asked for the
minimum size.  See CommonEventHandlers_WindowResizer::install().

\retval noErr
if the minimum size was updated successfully

\retval memPCErr
if the given pointer is invalid

(3.1)
*/
OSStatus
CommonEventHandlers_WindowResizer::
setWindowMinimumSize	(Float32	inMinimumWidth,
						 Float32	inMinimumHeight)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == _impl) result = memPCErr;
	else
	{
		_impl->minimumWidth = inMinimumWidth;
		_impl->minimumHeight = inMinimumHeight;
	}
	
	return result;
}// WindowResizer::setWindowMinimumSize


/*!
Internal implementation class for the public class
"CommonEventHandlers_WindowResizer".

(3.1)
*/
CommonEventHandlers_WindowResizerImpl::	
CommonEventHandlers_WindowResizerImpl	(HIWindowRef								inForWhichWindow,
										 CommonEventHandlers_HIWindowResizeProcPtr	inResizeProc,
										 void*										inContext,
										 Float32									inMinimumWidth,
										 Float32									inMinimumHeight,
										 Float32									inMaximumWidth,
										 Float32									inMaximumHeight)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
procPtr(inResizeProc),
context(inContext),
minimumWidth(inMinimumWidth),
maximumWidth(inMaximumWidth),
idealWidth(0),
minimumHeight(inMinimumHeight),
maximumHeight(inMaximumHeight),
idealHeight(0),
resizeEventHandler(GetWindowEventTarget(inForWhichWindow), receiveWindowResizeOrSizeQuery,
					CarbonEventSetInClass(CarbonEventClass(kEventClassWindow), kEventWindowBoundsChanged,
											kEventWindowGetMinimumSize, kEventWindowGetMaximumSize,
											kEventWindowGetIdealSize), this/* user data */)
{
}// 7-argument constructor


/*!
Installs a Carbon Event handler for a “little arrows”
control that causes the current value of the given text
field to change.  The text equivalent of the numerical
value of the arrows is put into the field.

Also sets up accessibility relationships, where possible,
to indicate that the arrows control relates to the field.

Dispose of the handler later by passing the returned
reference into CommonEventHandlers_RemoveNumericalFieldArrows().

\retval noErr
if the handler was installed successfully

\retval memPCErr
if the given pointer to storage is nullptr

\retval memFullErr
if there is not enough memory to create a handler

\retval (other)
if the handler could not be installed

(3.1)
*/
OSStatus
CommonEventHandlers_InstallNumericalFieldArrows		(ControlRef										inForWhichArrows,
													 ControlRef										inForWhichField,
													 CommonEventHandlers_NumericalFieldArrowsRef*	outHandlerPtr)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == outHandlerPtr) result = memPCErr;
	else
	{
		*outHandlerPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(My_NumericalFieldArrowsHandler)), CommonEventHandlers_NumericalFieldArrowsRef);
		if (nullptr == *outHandlerPtr) result = memFullErr;
		else
		{
			My_NumericalFieldArrowsHandlerAutoLocker	ptr(gMyNumericalFieldArrowsHandlerPtrLocks(), *outHandlerPtr);
			
			
			// initialize the handler structure so appropriate information
			// is available to the otherwise-generic callback routine
			ptr->field = inForWhichField;
			ptr->controlHitEventHandlerRef = nullptr; // set below
			ptr->controlHitEventHandlerUPP = NewEventHandlerUPP(receiveArrowHitForNumericalField);
			
			// install a callback that responds when the arrows are used
			{
				EventTypeSpec const		whenControlHit[] =
										{
											{ kEventClassControl, kEventControlHit }
										};
				
				
				result = HIViewInstallEventHandler(inForWhichArrows, ptr->controlHitEventHandlerUPP,
													GetEventTypeCount(whenControlHit),
													whenControlHit, *outHandlerPtr/* user data */,
													&ptr->controlHitEventHandlerRef/* event handler reference */);
			}
			
			// set accessibility relationships, if possible
			{
				CFStringRef		accessibilityDescCFString = nullptr;
				
				
				if (UIStrings_Copy(kUIStrings_ButtonEditTextArrowsAccessibilityDesc, accessibilityDescCFString).ok())
				{
					HIObjectRef const	kViewObjectRef = REINTERPRET_CAST(inForWhichArrows, HIObjectRef);
					OSStatus			error = noErr;
					
					
					error = HIObjectSetAuxiliaryAccessibilityAttribute
							(kViewObjectRef, 0/* sub-component identifier */,
								kAXDescriptionAttribute, accessibilityDescCFString);
					CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
				}
			}
		}
	}
	return result;
}// InstallNumericalFieldArrows


/*!
Installs a Carbon Event handler for a “little arrows”
control that causes the current value of the given pop-up
menu control to change.

Also sets up accessibility relationships, where possible,
to indicate that the arrows control relates to the menu.

Dispose of the handler later by passing the returned
reference into CommonEventHandlers_RemovePopUpMenuArrows().

\retval noErr
if the handler was installed successfully

\retval memPCErr
if the given pointer to storage is nullptr

\retval memFullErr
if there is not enough memory to create a handler

\retval (other)
if the handler could not be installed

(3.0)
*/
OSStatus
CommonEventHandlers_InstallPopUpMenuArrows	(ControlRef									inForWhichArrows,
											 ControlRef									inForWhichPopUpMenu,
											 CommonEventHandlers_PopUpMenuArrowsRef*	outHandlerPtr)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == outHandlerPtr) result = memPCErr;
	else
	{
		*outHandlerPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(My_PopUpMenuArrowsHandler)), CommonEventHandlers_PopUpMenuArrowsRef);
		if (nullptr == *outHandlerPtr) result = memFullErr;
		else
		{
			My_PopUpMenuArrowsHandlerAutoLocker		ptr(gMyPopUpMenuArrowsHandlerPtrLocks(), *outHandlerPtr);
			
			
			// initialize the handler structure so appropriate information
			// is available to the otherwise-generic callback routine
			ptr->popUpMenu = inForWhichPopUpMenu;
			ptr->controlHitEventHandlerRef = nullptr; // set below
			ptr->controlHitEventHandlerUPP = NewEventHandlerUPP(receiveArrowHitForPopUpMenu);
			
			// install a callback that responds when the arrows are used
			{
				EventTypeSpec const		whenControlHit[] =
										{
											{ kEventClassControl, kEventControlHit }
										};
				
				
				result = HIViewInstallEventHandler(inForWhichArrows, ptr->controlHitEventHandlerUPP,
													GetEventTypeCount(whenControlHit),
													whenControlHit, *outHandlerPtr/* user data */,
													&ptr->controlHitEventHandlerRef/* event handler reference */);
			}
			
			// set accessibility relationships, if possible
			{
				CFStringRef		accessibilityDescCFString = nullptr;
				
				
				if (UIStrings_Copy(kUIStrings_ButtonPopUpMenuArrowsAccessibilityDesc, accessibilityDescCFString).ok())
				{
					HIObjectRef const	kViewObjectRef = REINTERPRET_CAST(inForWhichArrows, HIObjectRef);
					OSStatus			error = noErr;
					
					
					error = HIObjectSetAuxiliaryAccessibilityAttribute
							(kViewObjectRef, 0/* sub-component identifier */,
								kAXDescriptionAttribute, accessibilityDescCFString);
					CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
				}
			}
		}
	}
	return result;
}// InstallPopUpMenuArrows


/*!
Removes a Carbon Event handler for control-hit events
that was previously added using the
CommonEventHandlers_InstallNumericalFieldArrows() routine.

\retval noErr
if the handler was removed successfully

\retval memPCErr
if the given pointer is invalid

\retval (other)
if the handler could not be removed successfully

(3.1)
*/
OSStatus
CommonEventHandlers_RemoveNumericalFieldArrows	(CommonEventHandlers_NumericalFieldArrowsRef*	inoutHandlerToDisposePtr)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == inoutHandlerToDisposePtr) result = memPCErr;
	else
	{
		{
			My_NumericalFieldArrowsHandlerAutoLocker	ptr(gMyNumericalFieldArrowsHandlerPtrLocks(), *inoutHandlerToDisposePtr);
			
			
			result = RemoveEventHandler(ptr->controlHitEventHandlerRef);
			DisposeEventHandlerUPP(ptr->controlHitEventHandlerUPP);
		}
		Memory_DisposePtr(REINTERPRET_CAST(inoutHandlerToDisposePtr, Ptr*));
	}
	
	return result;
}// RemoveNumericalFieldArrows


/*!
Removes a Carbon Event handler for control-hit events
that was previously added using the
CommonEventHandlers_InstallPopUpMenuArrows() routine.

\retval noErr
if the handler was removed successfully

\retval memPCErr
if the given pointer is invalid

\retval (other)
if the handler could not be removed successfully

(3.0)
*/
OSStatus
CommonEventHandlers_RemovePopUpMenuArrows	(CommonEventHandlers_PopUpMenuArrowsRef*	inoutHandlerToDisposePtr)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == inoutHandlerToDisposePtr) result = memPCErr;
	else
	{
		{
			My_PopUpMenuArrowsHandlerAutoLocker		ptr(gMyPopUpMenuArrowsHandlerPtrLocks(), *inoutHandlerToDisposePtr);
			
			
			result = RemoveEventHandler(ptr->controlHitEventHandlerRef);
			DisposeEventHandlerUPP(ptr->controlHitEventHandlerUPP);
		}
		Memory_DisposePtr(REINTERPRET_CAST(inoutHandlerToDisposePtr, Ptr*));
	}
	
	return result;
}// RemovePopUpMenuArrows


#pragma mark Internal Methods
namespace {

/*!
Handles "kEventControlHit" of "kEventClassControl" for
the terminal menu’s arrows.  Responds by updating the
current selection in the menu button.

(3.1)
*/
OSStatus
receiveArrowHitForNumericalField	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef				inEvent,
									 void*					inNumericalFieldArrowsHandlerRef)
{
	OSStatus										result = eventNotHandledErr;
	CommonEventHandlers_NumericalFieldArrowsRef		ref = REINTERPRET_CAST(inNumericalFieldArrowsHandlerRef, CommonEventHandlers_NumericalFieldArrowsRef);
	My_NumericalFieldArrowsHandlerAutoLocker		ptr(gMyNumericalFieldArrowsHandlerPtrLocks(), ref);
	UInt32 const									kEventClass = GetEventClass(inEvent);
	UInt32 const									kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHit);
	{
		ControlRef		control = nullptr;
		
		
		// determine the control in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			ControlPartCode		partCode = kControlNoPart;
			SInt32				currentFieldValue = 0;
			SInt32				kIncrementDecrement = 1;
			
			
			GetControlNumericalText(ptr->field, &currentFieldValue);
			
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			if (noErr != result) partCode = kControlUpButtonPart;
			
			// try to find the increment value associated with the arrows view;
			// ignore any errors, this is not that important
			UNUSED_RETURN(OSStatus)GetControlData(control, kControlEntireControl, kControlLittleArrowsIncrementValueTag,
													sizeof(kIncrementDecrement), &kIncrementDecrement, nullptr/* actual size */);
			
			switch (partCode)
			{
			case kControlDownButtonPart:
				if (GetControl32BitMinimum(control) >= currentFieldValue)
				{
					SetControlNumericalText(ptr->field, GetControl32BitMaximum(control));
				}
				else
				{
					SetControlNumericalText(ptr->field, currentFieldValue - kIncrementDecrement);
				}
				break;
			
			case kControlUpButtonPart:
			default:
				if (GetControl32BitMaximum(control) <= currentFieldValue)
				{
					SetControlNumericalText(ptr->field, GetControl32BitMinimum(control));
				}
				else
				{
					SetControlNumericalText(ptr->field, currentFieldValue + kIncrementDecrement);
				}
				break;
			}
		}
	}
	return result;
}// receiveArrowHitForNumericalField


/*!
Handles "kEventControlHit" of "kEventClassControl" for
the terminal menu’s arrows.  Responds by updating the
current selection in the menu button.

(3.1)
*/
OSStatus
receiveArrowHitForPopUpMenu		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inPopUpMenuArrowsHandlerRef)
{
	OSStatus									result = eventNotHandledErr;
	CommonEventHandlers_PopUpMenuArrowsRef		ref = REINTERPRET_CAST(inPopUpMenuArrowsHandlerRef, CommonEventHandlers_PopUpMenuArrowsRef);
	My_PopUpMenuArrowsHandlerAutoLocker			ptr(gMyPopUpMenuArrowsHandlerPtrLocks(), ref);
	UInt32 const								kEventClass = GetEventClass(inEvent);
	UInt32 const								kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHit);
	{
		ControlRef		control = nullptr;
		
		
		// determine the control in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			ControlPartCode		partCode = kControlNoPart;
			SInt32 const		kCurrentMenuValue = GetControl32BitValue(ptr->popUpMenu);
			SInt32 const		kIncrement = 1;
			SInt32 const		kDecrement = 1;
			
			
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			if (noErr != result) partCode = kControlUpButtonPart;
			
			switch (partCode)
			{
			case kControlDownButtonPart:
				if (GetControl32BitMaximum(ptr->popUpMenu) == kCurrentMenuValue)
				{
					SetControl32BitValue(ptr->popUpMenu, GetControl32BitMinimum(ptr->popUpMenu));
				}
				else
				{
					SetControl32BitValue(ptr->popUpMenu, kCurrentMenuValue + kIncrement);
				}
				break;
			
			case kControlUpButtonPart:
			default:
				if (GetControl32BitMinimum(ptr->popUpMenu) == kCurrentMenuValue)
				{
					SetControl32BitValue(ptr->popUpMenu, GetControl32BitMaximum(ptr->popUpMenu));
				}
				else
				{
					SetControl32BitValue(ptr->popUpMenu, kCurrentMenuValue - kDecrement);
				}
				break;
			}
		}
	}
	return result;
}// receiveArrowHitForPopUpMenu


/*!
Handles "kEventControlBoundsChanged" of "kEventClassControl".
Invokes the control resize callback provided at install time.

(3.0)
*/
OSStatus
receiveHIViewResizeOrSizeQuery	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inResizeHandlerRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlBoundsChanged);
	{
		HIViewRef	control = nullptr;
		
		
		// determine the view in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the view was found, proceed
		if (noErr == result)
		{
			switch (kEventKind)
			{
			case kEventControlBoundsChanged:
				{
					UInt32	changeAttributes = 0L;
					
					
					// determine what is changing; only size matters :)
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamAttributes, typeUInt32, changeAttributes);
					
					if ((noErr == result) && (changeAttributes & kControlBoundsChangeSizeChanged))
					{
						Rect	previousBounds;
						
						
						// determine the bounds of the view just before the size change
						result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamOriginalBounds/*kEventParamPreviousBounds*/, typeQDRectangle,
																		previousBounds);
						if (noErr == result)
						{
							Rect	currentBounds;
							
							
							// determine the bounds of the view just after the size change
							result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCurrentBounds, typeQDRectangle,
																			currentBounds);
							if (noErr == result)
							{
								// finally, invoke the custom handler!
								CommonEventHandlers_HIViewResizerImpl*	ptr = REINTERPRET_CAST(inResizeHandlerRef,
																								CommonEventHandlers_HIViewResizerImpl*);
								
								
								// if any boundary edge of interest to the handler has changed,
								// invoke the callback; otherwise, do not invoke the callback!
								if (((currentBounds.left != previousBounds.left) &&
									(ptr->edgesOfInterest & kCommonEventHandlers_ChangedBoundsEdgeLeft)) ||
									((currentBounds.top != previousBounds.top) &&
									(ptr->edgesOfInterest & kCommonEventHandlers_ChangedBoundsEdgeTop)) ||
									((currentBounds.right != previousBounds.right) &&
									(ptr->edgesOfInterest & kCommonEventHandlers_ChangedBoundsEdgeRight)) ||
									((currentBounds.bottom != previousBounds.bottom) &&
									(ptr->edgesOfInterest & kCommonEventHandlers_ChangedBoundsEdgeBottom)) ||
									(((currentBounds.right - currentBounds.left) !=
									(previousBounds.right - previousBounds.left)) &&
									(ptr->edgesOfInterest & kCommonEventHandlers_ChangedBoundsEdgeSeparationH)) ||
									(((currentBounds.bottom - currentBounds.top) !=
									(previousBounds.bottom - previousBounds.top)) &&
									(ptr->edgesOfInterest & kCommonEventHandlers_ChangedBoundsEdgeSeparationV)))
								{
									CommonEventHandlers_InvokeHIViewResizeProc(ptr->procPtr, control,
																				(currentBounds.right - currentBounds.left) -
																				(previousBounds.right - previousBounds.left)/* delta X */,
																				(currentBounds.bottom - currentBounds.top) -
																				(previousBounds.bottom - previousBounds.top)/* delta Y */,
																				ptr->context);
								}
								
								// now return EVENT NOT HANDLED so controls are resized normally
								result = eventNotHandledErr;
							}
						}
					}
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	return result;
}// receiveHIViewResizeOrSizeQuery


/*!
Handles "kEventWindowBoundsChanged", "kEventWindowGetMinimumSize",
"kEventWindowGetMaximumSize" and "kEventWindowGetIdealSize" of
"kEventClassWindow".  Either moves and resizes the views in the
window, or returns the requested dimensions.

(3.0)
*/
OSStatus
receiveWindowResizeOrSizeQuery	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inResizeHandlerRef)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert((kEventKind == kEventWindowBoundsChanged) || (kEventKind == kEventWindowGetMinimumSize) ||
			(kEventKind == kEventWindowGetMaximumSize) || (kEventKind == kEventWindowGetIdealSize));
	{
		HIWindowRef		window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (noErr == result)
		{
			switch (kEventKind)
			{
			case kEventWindowBoundsChanged:
				{
					UInt32	changeAttributes = 0L;
					
					
					// determine what is changing; only size matters :)
					result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamAttributes, typeUInt32, changeAttributes);
					
					if ((noErr == result) && (changeAttributes & kWindowBoundsChangeSizeChanged))
					{
						Rect	previousBounds;
						
						
						// determine the bounds of the window just before the size change
						result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamPreviousBounds, typeQDRectangle,
																		previousBounds);
						if (noErr == result)
						{
							Rect	currentBounds;
							
							
							// determine the bounds of the window just after the size change
							result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCurrentBounds, typeQDRectangle,
																			currentBounds);
							if (noErr == result)
							{
								// finally, invoke the custom handler!
								CommonEventHandlers_WindowResizerImpl*	ptr = REINTERPRET_CAST(inResizeHandlerRef,
																								CommonEventHandlers_WindowResizerImpl*);
								
								
								CommonEventHandlers_InvokeHIWindowResizeProc(ptr->procPtr, window,
																				(currentBounds.right - currentBounds.left) -
																				(previousBounds.right - previousBounds.left)/* delta X */,
																				(currentBounds.bottom - currentBounds.top) -
																				(previousBounds.bottom - previousBounds.top)/* delta Y */,
																				ptr->context);
							}
						}
					}
				}
				break;
			
			case kEventWindowGetMinimumSize:
			case kEventWindowGetMaximumSize:
			case kEventWindowGetIdealSize:
				{
					CommonEventHandlers_WindowResizerImpl*	ptr = REINTERPRET_CAST(inResizeHandlerRef,
																					CommonEventHandlers_WindowResizerImpl*);
					Point									dimensions =
															{
																STATIC_CAST(ptr->maximumHeight, SInt16),
																STATIC_CAST(ptr->maximumWidth, SInt16)
															};
					
					
					if (kEventKind == kEventWindowGetMinimumSize)
					{
						SetPt(&dimensions, STATIC_CAST(ptr->minimumWidth, SInt16), STATIC_CAST(ptr->minimumHeight, SInt16));
					}
					if (kEventKind == kEventWindowGetIdealSize)
					{
						// if the ideal size is specified, use that exclusively
						if ((0 != ptr->idealWidth) && (0 != ptr->idealHeight))
						{
							SetPt(&dimensions, STATIC_CAST(ptr->idealWidth, SInt16), STATIC_CAST(ptr->idealHeight, SInt16));
						}
						else
						{
							OSStatus	error = noErr;
							GDHandle	windowDevice = nullptr;
							
							
							result = eventNotHandledErr; // initially...
							error = GetWindowGreatestAreaDevice(window, kWindowStructureRgn, &windowDevice, nullptr/* device rectangle */);
							if (noErr == error)
							{
								Rect	positioningBounds;
								
								
								// let the ideal size be the largest width and height allowed by the window
								// that will also fit within the window positioning area of its display
								error = GetAvailableWindowPositioningBounds(windowDevice, &positioningBounds);
								if (noErr == error)
								{
									SetPt(&dimensions,
											STATIC_CAST(FLOAT64_MINIMUM(ptr->maximumWidth, positioningBounds.right - positioningBounds.left), SInt16),
											STATIC_CAST(FLOAT64_MINIMUM(ptr->maximumHeight, positioningBounds.bottom - positioningBounds.top), SInt16));
									result = noErr;
								}
							}
						}
					}
					
					// return the value
					{
						EventParamName const	kParameterName = kEventParamDimensions;
						void*					parameterValueAddress = &dimensions;
						UInt32 const			kActualSize = sizeof(dimensions);
						EventParamType const	kActualType = typeQDPoint;
						
						
						result = SetEventParameter(inEvent, kParameterName, kActualType, kActualSize,
													parameterValueAddress);
					}
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	return result;
}// receiveWindowResizeOrSizeQuery

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
