/*###############################################################

	StatusIcon.cp
	
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

// standard-C++ includes
#include <algorithm>
#include <vector>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <ColorUtilities.h>
#include <Console.h>
#include <IconManager.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>

// resource includes
#include "ControlResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "DialogUtilities.h"
#include "StatusIcon.h"



#pragma mark Types

struct IconAnimationStage
{
	StatusIconAnimationStageDescriptor	descriptor;
	UInt32								ticks;
	IconManagerIconRef					icon;
};
typedef struct IconAnimationStage	IconAnimationStage;
typedef IconAnimationStage const*	IconAnimationStageConstPtr;
typedef IconAnimationStage*			IconAnimationStagePtr;

typedef std::vector< IconAnimationStagePtr >	IconAnimationStageList;

struct StatusIcon
{
	IconAnimationStageList				iconAnimationStages;
	StatusIconAnimationStageDescriptor	descriptorOfCurrentAnimationStage;
	ControlRef							userPane;
	EventLoopTimerUPP					idleUPP;
	EventLoopTimerRef					idleTimer;
	ControlUserPaneDrawUPP				drawUPP;
	ControlUserPaneHitTestUPP			hitTestUPP;
	ControlUserPaneTrackingUPP			trackingUPP;
	Boolean								animate;
	Boolean								dimWhenInactive;
	Boolean								focusRing;
};
typedef struct StatusIcon	StatusIcon;
typedef StatusIcon const*	StatusIconConstPtr;
typedef StatusIcon*			StatusIconPtr;

typedef MemoryBlockPtrLocker< StatusIconRef, StatusIcon >	StatusIconPtrLocker;

#pragma mark Internal Method Prototypes

static Boolean					addAnimationStage		(StatusIconPtr, IconAnimationStagePtr);
static void						disposeAnimationStage	(IconAnimationStagePtr*);
static pascal void				iconIdle				(EventLoopTimerRef, void*);
static pascal void				iconUserPaneDraw		(ControlRef, SInt16);
static void						iconUserPaneDeviceLoop	(short, short, GDHandle, ControlRef, ControlPartCode);
static pascal ControlPartCode	iconUserPaneHitTest		(ControlRef, Point);
static pascal ControlPartCode	iconUserPaneTracker		(ControlRef, Point, ControlActionUPP);
static IconAnimationStagePtr	newAnimationStage		(StatusIconAnimationStageDescriptor, SInt16, SInt16,
														 Boolean, UInt32);
static IconAnimationStagePtr	newAnimationStage2		(StatusIconAnimationStageDescriptor, CFStringRef, OSType,
														 OSType, UInt32);
static void						resetAnimationStage		(StatusIconPtr);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	StatusIconPtrLocker&		gStatusIconPtrLocks ()	{ static StatusIconPtrLocker x; return x; }
}



#pragma mark Functors

/*!
This is a functor that determines if the descriptor
of a Status Icon is equal to a specific value.

Model of STL Predicate.

(1.0)
*/
#pragma mark animationStageDescriptorEquals
class animationStageDescriptorEquals:
public std::unary_function< IconAnimationStageConstPtr/* argument */, bool/* return */ >
{
public:
	animationStageDescriptorEquals	(StatusIconAnimationStageDescriptor	inDescriptor)
	: _descriptor(inDescriptor)
	{
	}
	
	bool
	operator()	(IconAnimationStageConstPtr	inPtr)
	{
		return (inPtr->descriptor == _descriptor);
	}

protected:

private:
	StatusIconAnimationStageDescriptor	_descriptor;
};



#pragma mark Public Methods

/*!
Creates a new status icon control that will appear
in the specified window.

You can use the new reference to manipulate the
status icon using the methods in this module.  The
status icon initially contains no icon, so you
need to call StatusIcon_AddStageFrom...() methods
to make at least one animation cell.

If any problems occur, nullptr is returned; otherwise,
a reference to the new status icon is returned.

(3.0)
*/
StatusIconRef
StatusIcon_New		(WindowRef		inOwningWindow)
{
	StatusIconRef	result = REINTERPRET_CAST(new StatusIcon, StatusIconRef);
	
	
	if (result != nullptr)
	{
		StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(result);
		ControlRef		control = nullptr;
		Rect			bounds;
		OSStatus		error = noErr;
		
		
		ptr->descriptorOfCurrentAnimationStage = kInvalidStatusIconAnimationStageDescriptor;
		SetRect(&bounds, 0, 0, ICON_WD_SMALL, ICON_HT_SMALL);
		error = CreateUserPaneControl(inOwningWindow, &bounds, kControlHandlesTracking, &ptr->userPane);
		assert(error == noErr);
		control = ptr->userPane;
		SetControlVisibility(control, true/* visible */, true/* draw */);
		SetControlReference(control, REINTERPRET_CAST(result, SInt32)/* encode StatusIconRef into ref. con. */);
		ptr->idleUPP = NewEventLoopTimerUPP(iconIdle);
		error = InstallEventLoopTimer(GetMainEventLoop(), kEventDurationNoWait/* delay before first fire */,
										kEventDurationSecond/* time between fires */, ptr->idleUPP,
										result/* context */, &ptr->idleTimer);
		assert(error == noErr);
		ptr->drawUPP = NewControlUserPaneDrawUPP(iconUserPaneDraw);
		error = SetControlData(control, kControlEntireControl, kControlUserPaneDrawProcTag,
									sizeof(ptr->drawUPP), &ptr->drawUPP);
		assert(error == noErr);
		ptr->hitTestUPP = NewControlUserPaneHitTestUPP(iconUserPaneHitTest);
		error = SetControlData(control, kControlEntireControl, kControlUserPaneHitTestProcTag,
									sizeof(ptr->hitTestUPP), &ptr->hitTestUPP);
		assert(error == noErr);
		ptr->trackingUPP = NewControlUserPaneTrackingUPP(iconUserPaneTracker);
		error = SetControlData(control, kControlEntireControl, kControlUserPaneTrackingProcTag,
									sizeof(ptr->trackingUPP), &ptr->trackingUPP);
		assert(error == noErr);
		
		ptr->animate = false;
		ptr->dimWhenInactive = true;
		ptr->focusRing = false;
		
		gStatusIconPtrLocks().releaseLock(result, &ptr);
	}
	return result;
}// New


/*!
This method cleans up a status icon by destroying
all of the data associated with it.  On output,
your copy of the given reference will be set to
nullptr.

WARNING:	If you previously used accessor
			methods to acquire physical icon data
			from a status icon, all of those data
			handles will become invalid once you
			call StatusIcon_Dispose().

(3.0)
*/
void
StatusIcon_Dispose		(StatusIconRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		StatusIconPtr						ptr = gStatusIconPtrLocks().acquireLock(*inoutRefPtr);
		IconAnimationStagePtr				stagePtr = nullptr;
		IconAnimationStageList::iterator	animationStageIterator;
		
		
		(OSStatus)RemoveEventLoopTimer(ptr->idleTimer), ptr->idleTimer = nullptr;
		DisposeEventLoopTimerUPP(ptr->idleUPP), ptr->idleUPP = nullptr;
		DisposeControlUserPaneDrawUPP(ptr->drawUPP), ptr->drawUPP = nullptr;
		DisposeControlUserPaneHitTestUPP(ptr->hitTestUPP), ptr->hitTestUPP = nullptr;
		DisposeControlUserPaneTrackingUPP(ptr->trackingUPP), ptr->trackingUPP = nullptr;
		for (animationStageIterator = ptr->iconAnimationStages.begin();
					animationStageIterator != ptr->iconAnimationStages.end(); ++animationStageIterator)
		{
			stagePtr = *animationStageIterator;
			disposeAnimationStage(&stagePtr);
		}
		
		gStatusIconPtrLocks().releaseLock(*inoutRefPtr, &ptr);
		delete *(REINTERPRET_CAST(inoutRefPtr, StatusIconPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Adds an icon animation stage to a status icon.
If you want to create a single icon that does not
animate, simply call this method only once,
specifying an icon and a delay of zero.  If you
call this method multiple times (to install the
various stages of animation), you must call this
routine in the order you want those stages to be
animated.

This Mac OS X only routine allows you to find
".icns" files within localized Resources folders
in the main bundle simply by asking for them by
name (without the ".icns" or any folder path).

(3.0)
*/
void
StatusIcon_AddStageFromIconRef	(StatusIconRef						inRef,
								 StatusIconAnimationStageDescriptor	inDescriptor,
								 CFStringRef						inBundleResourceNameNoExtension,
								 OSType								inCreator,
								 OSType								inType,
								 UInt32								inAnimationDelayInTicks)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		if (!addAnimationStage(ptr, newAnimationStage2(inDescriptor, inBundleResourceNameNoExtension, inCreator,
														inType, inAnimationDelayInTicks)))
		{
			Console_WriteLine("warning, unable to add icon reference as animation frame");
		}
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
}// AddStageFromIconRef


/*!
Adds an icon animation stage to a status icon.
If you want to create a single icon that does not
animate, simply call this method only once,
specifying an icon and a delay of zero.  If you
call this method multiple times (to install the
various stages of animation), you must call this
routine in the order you want those stages to be
animated.

Under Mac OS 8.5 and beyond, the specified Òicon
suite IDÓ will be parsed, and if the icon ID
refers to an icon available via Icon Services,
the ÒbetterÓ icon will be used automatically and
transparently.  In other words, you can specify
an icon as if it were a System 7 icon, and get
Mac OS X functionality when possible without any
extra work on your part.

(3.0)
*/
void
StatusIcon_AddStageFromIconSuite	(StatusIconRef						inRef,
									 StatusIconAnimationStageDescriptor	inDescriptor,
									 SInt16								inResourceFile,
									 SInt16								inIconSuiteResourceID,
									 UInt32								inAnimationDelayInTicks)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		if (!addAnimationStage(ptr, newAnimationStage(inDescriptor, inResourceFile, inIconSuiteResourceID,
														true/* is icon suite */, inAnimationDelayInTicks)))
		{
			Console_WriteLine("warning, unable to add icon suite as animation frame");
		}
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
}// AddStageFromIconSuite


/*!
Adds an icon animation stage to a status icon.
If you want to create a single icon that does not
animate, simply call this method only once,
specifying an icon and a delay of zero.  If you
call this method multiple times (to install the
various stages of animation), you must call this
routine in the order you want those stages to be
animated.

Under Mac OS 8.5 and beyond, the specified Òcolor
icon IDÓ will be parsed, and if the icon ID
refers to an icon available via Icon Services,
the ÒbetterÓ icon will be used automatically and
transparently.  In other words, you can specify
an icon as if it were a System 7 icon, and get
Mac OS X functionality when possible without any
extra work on your part.

(3.0)
*/
void
StatusIcon_AddStageFromOldColorIcon		(StatusIconRef						inRef,
										 StatusIconAnimationStageDescriptor	inDescriptor,
										 SInt16								inResourceFile,
										 SInt16								inIconResourceID,
										 UInt32								inAnimationDelayInTicks)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		if (!addAnimationStage(ptr, newAnimationStage(inDescriptor, inResourceFile, inIconResourceID,
														false/* is icon suite */, inAnimationDelayInTicks)))
		{
			Console_WriteLine("warning, unable to add old color icon as animation frame");
		}
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
}// AddStageFromOldColorIcon


/*!
Returns a handle to the control that draws a
status icon.  You need to call this method in
order to display the icon in a window!

(3.0)
*/
ControlRef
StatusIcon_GetContainerControl		(StatusIconRef		inRef)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	ControlRef		result = nullptr;
	
	
	if (ptr != nullptr)
	{
		result = ptr->userPane;
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
	return result;
}// GetContainerControl


/*!
Determines if animation is currently turned on
for a status icon.

(3.0)
*/
Boolean
StatusIcon_IsAnimating		(StatusIconRef		inRef)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	Boolean			result = false;
	
	
	if (ptr != nullptr)
	{
		result = ptr->animate;
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
	return result;
}// IsAnimating


/*!
Specifies whether the icon should appear dimmed
when the control is inactive.  The default is
"true".

(3.0)
*/
void
StatusIcon_SetDimWhenInactive	(StatusIconRef	inRef,
								 Boolean		inIconShouldDimWhenInactive)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		ptr->dimWhenInactive = inIconShouldDimWhenInactive;
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
}// SetDimWhenInactive


/*!
Turns icon animation on or off.  This flag only
has effect if there is more than one icon
animation stage installed for a given status icon;
if animation is on, then the icon will animate
through a Carbon Event timer.

You can stop icon animation using this routine,
and then resume it later: to do this, pass "false"
for "inResetStages".  Or, you can start the
animation over, by passing "true".

(3.0)
*/
void
StatusIcon_SetDoAnimate		(StatusIconRef	inRef,
							 Boolean		inIconShouldAnimate,
							 Boolean		inResetStages)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		ptr->animate = inIconShouldAnimate;
		if (inResetStages) resetAnimationStage(ptr);
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
}// SetDoAnimate


/*!
Specifies whether icons should be rendered with a
focus ring.  This is an aesthetic adornment and
should represent keyboard focus, but invoking this
routine only affects the icon appearance, not its
behavior.

(3.0)
*/
void
StatusIcon_SetFocus		(StatusIconRef	inRef,
						 Boolean		inFocusRing)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		ptr->focusRing = inFocusRing;
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
}// SetFocus


/*!
Sets the next animation cell.  For icons which do
not animate continuously, you generally call this
routine to set which icon in the set you wish to
use.  For best results in this latter case, you
should make the inter-frame delay small (1 tick).

(3.0)
*/
void
StatusIcon_SetNextCell	(StatusIconRef						inRef,
						 StatusIconAnimationStageDescriptor	inDescriptor)
{
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		if (std::find_if(ptr->iconAnimationStages.begin(), ptr->iconAnimationStages.end(),
							animationStageDescriptorEquals(inDescriptor))
			== ptr->iconAnimationStages.end())
		{
			ptr->descriptorOfCurrentAnimationStage = kInvalidStatusIconAnimationStageDescriptor;
		}
		else
		{
			ptr->descriptorOfCurrentAnimationStage = inDescriptor;
		}
	}
	gStatusIconPtrLocks().releaseLock(inRef, &ptr);
}// SetNextCell


#pragma mark Internal Methods

/*!
Adds a new item to an animation stage list.
If the stage was added successfully, "true"
is returned; otherwise, "false" is returned.

(3.0)
*/
static Boolean
addAnimationStage		(StatusIconPtr			inIconPtr,
						 IconAnimationStagePtr	inNewStagePtr)
{
	Boolean		result = false;
	
	
	if (inNewStagePtr != nullptr)
	{
		inIconPtr->iconAnimationStages.push_back(inNewStagePtr);
		assert(!inIconPtr->iconAnimationStages.empty());
		assert(inIconPtr->iconAnimationStages.back() == inNewStagePtr);
		if (inIconPtr->iconAnimationStages.size() == 1) resetAnimationStage(inIconPtr);
		result = true;
	}
	return result;
}// addAnimationStage


/*!
Destroys an animation stage structure created
with newAnimationStage().

(3.0)
*/
static void
disposeAnimationStage	(IconAnimationStagePtr*		inoutPtr)
{
	if (inoutPtr != nullptr)
	{
		if ((*inoutPtr) != nullptr)
		{
			IconManager_DisposeIcon(&((*inoutPtr)->icon));
			Memory_DisposePtr((Ptr*)inoutPtr);
		}
	}
}// disposeAnimationStage


/*!
Allows a status icon to animate icons correctly.
Should be invoked as frequently as icons might
need to animate.

(3.0)
*/
static pascal void
iconIdle	(EventLoopTimerRef	UNUSED_ARGUMENT(inTimer),
			 void*				inStatusIconRef)
{
	StatusIconRef	ref = REINTERPRET_CAST(inStatusIconRef, StatusIconRef);
	StatusIconPtr	ptr = gStatusIconPtrLocks().acquireLock(ref);
	
	
	if (ptr != nullptr)
	{
		if ((ptr->animate) && (ptr->iconAnimationStages.size() > 1))
		{
			IconAnimationStageList::const_iterator	animationStageIterator =
														std::find_if(ptr->iconAnimationStages.begin(),
																		ptr->iconAnimationStages.end(),
																		animationStageDescriptorEquals
																		(ptr->descriptorOfCurrentAnimationStage));
			
			
			if (animationStageIterator != ptr->iconAnimationStages.end())
			{
				IconAnimationStagePtr	stagePtr = *animationStageIterator;
				static UInt32			lastTime = 0L;
				UInt32					currentTime = TickCount();
				
				
				if ((currentTime - lastTime) >= stagePtr->ticks)
				{
					Rect	iconBounds;
					
					
					// set next animation cell
					++animationStageIterator;
					if (animationStageIterator == ptr->iconAnimationStages.end())
					{
						animationStageIterator = ptr->iconAnimationStages.begin();
					}
					ptr->descriptorOfCurrentAnimationStage = (*animationStageIterator)->descriptor;
					
					// redraw the icon
					GetControlBounds(ptr->userPane, &iconBounds);
					(OSStatus)InvalWindowRect(GetControlOwner(ptr->userPane), &iconBounds);
				}
				lastTime = currentTime;
			}
		}
	}
	gStatusIconPtrLocks().releaseLock(ref, &ptr);
}// iconIdle


/*!
This method, of DeviceLoopDrawControlProcPtr form,
handles drawing of Status Icons correctly no matter
what the device characteristics are.

IMPORTANT:	The control reference constant is to be
			of type StatusIconRef.

(3.0)
*/
static void
iconUserPaneDeviceLoop		(short				inColorDepth,
							 short				UNUSED_ARGUMENT(inDeviceFlags),
							 GDHandle			inTargetDevice,
							 ControlRef			inControl,
							 ControlPartCode	UNUSED_ARGUMENT(inControlPartCode))
{
	StatusIconRef		ref = REINTERPRET_CAST(GetControlReference(inControl), StatusIconRef);
	
	
	if (ref != nullptr)
	{
		StatusIconPtr		ptr = gStatusIconPtrLocks().acquireLock(ref);
		
		
		if (ptr != nullptr)
		{
			// First, get the current icon animation stage from the stage list
			// of the given status icon.  Then, plot the current stageÕs icon,
			// making it appear disabled if the given control is inactive.
			IconTransformType						iconTransform = kTransformNone;
			IconAnimationStageList::const_iterator	animationStageIterator;
			Rect									controlBounds;
			
			
			if (IsControlActiveAndHilited(inControl)) iconTransform = kTransformSelected;
			else if (!IsControlActive(inControl) && (ptr->dimWhenInactive)) iconTransform = kTransformDisabled;
			
			GetControlBounds(inControl, &controlBounds);
			(OSStatus)SetUpControlBackground(inControl, inColorDepth, ColorUtilities_IsColorDevice(inTargetDevice));
		#if TARGET_API_MAC_CARBON
			InsetRect(&controlBounds, -2, -2); // arbitrary - account for Aqua drawing outside the bounds
		#endif
			EraseRect(&controlBounds);
		#if TARGET_API_MAC_CARBON
			InsetRect(&controlBounds, 2, 2); // arbitrary - account for Aqua drawing outside the bounds
		#endif
			
			// if the boundaries are not square, make them square (but as large a square as possible within the rectangle)
			if ((controlBounds.right - controlBounds.left) != (controlBounds.bottom - controlBounds.top))
			{
				SInt16		dimension = INTEGER_MINIMUM(controlBounds.right - controlBounds.left, controlBounds.bottom - controlBounds.top);
				Rect		squareShape;
				
				
				SetRect(&squareShape, 0, 0, dimension, dimension);
				CenterRectIn(&squareShape, &controlBounds);
				controlBounds = squareShape;
			}
			
			animationStageIterator = std::find_if(ptr->iconAnimationStages.begin(), ptr->iconAnimationStages.end(),
													animationStageDescriptorEquals(ptr->descriptorOfCurrentAnimationStage));
			if (animationStageIterator != ptr->iconAnimationStages.end())
			{
				IconAnimationStagePtr	stagePtr = *animationStageIterator;
				
				
				(OSStatus)IconManager_PlotIcon(stagePtr->icon, &controlBounds, kAlignAbsoluteCenter, iconTransform);
			}
		}
		gStatusIconPtrLocks().releaseLock(ref, &ptr);
	}
}// iconUserPaneDeviceLoop


/*!
This method, a standard Mac OS user pane
drawing procedure, draws the current status
icon in the control rectangle.

IMPORTANT:	The control reference constant
			is to be of type StatusIconRef.

(3.0)
*/
static pascal void
iconUserPaneDraw	(ControlRef		inControl,
					 SInt16			inControlPartCode)
{
	ColorPenState	state;
	
	
	ColorUtilities_PreserveColorAndPenState(&state);
	ColorUtilities_NormalizeColorAndPen();
	DeviceLoopClipAndDrawControl(inControl, inControlPartCode, iconUserPaneDeviceLoop);
	ColorUtilities_RestoreColorAndPenState(&state);
}// iconUserPaneDraw


/*!
This method, of proper ControlUserPaneHitTestUPP form,
determines if the mouse is inside an icon button
(kControlPartCodeStatusIconButton) or not
(kControlPartCodeStatusIconVoid).

(1.0)
*/
static pascal ControlPartCode
iconUserPaneHitTest		(ControlRef		inControl,
						 Point			inLocalMouse)
{
	ControlPartCode		result = kControlPartCodeStatusIconVoid;
	Rect				controlBounds;
	
	
	GetControlBounds(inControl, &controlBounds);
	if (PtInRect(inLocalMouse, &controlBounds)) result = kControlPartCodeStatusIconButton;
	
	return result;
}// iconUserPaneHitTest


/*!
Determines the proper control part corresponding to
the specified mouse location, for an icon button.
The only supported parts are inside the button
(kControlPartCodeStatusIconButton) or outside
(kControlPartCodeStatusIconVoid).  This routine
continues to loop until the mouse is released, and
returns the control part that the mouse last
intersects when released.  Theme sounds are handled.

The control action routine is ignored (but for future
reference, valid values are nullptr (0), -1, or a
valid function pointer!).

(1.0)
*/
static pascal ControlPartCode
iconUserPaneTracker		(ControlRef			inControl,
						 Point				inLocalMouse,
						 ControlActionUPP	UNUSED_ARGUMENT(inIgnoredActionUPP))
{
	ControlPartCode		result = kControlPartCodeStatusIconVoid;
	StatusIconPtr		ptr = nullptr;
	
	
	ptr = gStatusIconPtrLocks().acquireLock(REINTERPRET_CAST(GetControlReference(inControl), StatusIconRef));
	if (ptr != nullptr)
	{
		MouseTrackingResult		trackingResult = kMouseTrackingMouseDown;
		ControlPartCode			previousControlPartCode = kControlPartCodeStatusIconVoid;
		Boolean					wasInside = false;
		Point					localMouse = inLocalMouse;
		CGrafPtr				oldPort = nullptr;
		GDHandle				oldDevice = nullptr;
		
		
		GetGWorld(&oldPort, &oldDevice);
		SetPortWindowPort(GetControlOwner(inControl));
		do
		{
			result = TestControl(inControl, localMouse);
			if (result == kControlPartCodeStatusIconButton)
			{
				unless (wasInside) // avoid playing theme sounds repeatedly
				{
					wasInside = true;
					(OSStatus)PlayThemeSound(kThemeSoundBevelEnter);
				}
			}
			else
			{
				if (wasInside) // avoid playing theme sounds repeatedly
				{
					wasInside = false;
					(OSStatus)PlayThemeSound(kThemeSoundBevelExit);
				}
			}
			
			// redraw control appropriately
			if (previousControlPartCode != result)
			{
				HiliteControl(inControl, result);
			}
			previousControlPartCode = result;
			
			// find next mouse location
			{
				OSStatus	error = noErr;
				
				
				error = TrackMouseLocation(nullptr/* port, or nullptr for current port */, &localMouse, &trackingResult);
				if (error != noErr) break;
			}
		}
		while (trackingResult != kMouseTrackingMouseUp);
		SetGWorld(oldPort, oldDevice);
		HiliteControl(inControl, kControlPartCodeStatusIconVoid);
	}
	gStatusIconPtrLocks().releaseLock(REINTERPRET_CAST(GetControlReference(inControl), StatusIconRef), &ptr);
	
	return result;
}// iconUserPaneTracker


/*!
Creates a new animation stage structure.  See
also newAnimationStage2().

(3.0)
*/
static IconAnimationStagePtr
newAnimationStage		(StatusIconAnimationStageDescriptor		inDescriptor,
						 SInt16									inResourceFile,
						 SInt16									inIconResourceID,
						 Boolean								inIconResourceIDIsForIconSuite, // if not, use a 'cicn'
						 UInt32									inTicks)
{
	IconAnimationStagePtr	result = nullptr;
	SInt16					oldResourceFile = CurResFile();
	
	
	UseResFile(inResourceFile);
	if (ResError() == noErr)
	{
		result = REINTERPRET_CAST(Memory_NewPtr(sizeof(IconAnimationStage)), IconAnimationStagePtr);
		if (result != nullptr)
		{
			OSStatus	error = noErr;
			
			
			result->descriptor = inDescriptor;
			result->ticks = inTicks;
			result->icon = IconManager_NewIcon();
			if (inIconResourceIDIsForIconSuite)
			{
				error = IconManager_MakeIconSuite(result->icon, inIconResourceID, kSelectorAllAvailableData);
			}
			else
			{
				error = IconManager_MakeOldColorIcon(result->icon, inIconResourceID);
			}
			
			if (error != noErr)
			{
				// destroy the new stage if any problems occur
				IconManager_DisposeIcon(&result->icon);
				Memory_DisposePtr(REINTERPRET_CAST(&result, Ptr*));
			}
		}
	}
	UseResFile(oldResourceFile);
	return result;
}// newAnimationStage


/*!
Creates a new animation stage structure.  See
also newAnimationStage().

(3.0)
*/
static IconAnimationStagePtr
newAnimationStage2	(StatusIconAnimationStageDescriptor		inDescriptor,
					 CFStringRef							inBundleResourceNameNoExtension,
					 OSType									inCreator,
					 OSType									inType,
					 UInt32									inTicks)
{
	IconAnimationStagePtr	result = nullptr;
	
	
	result = REINTERPRET_CAST(Memory_NewPtr(sizeof(IconAnimationStage)), IconAnimationStagePtr);
	if (result != nullptr)
	{
		OSStatus	error = noErr;
		
		
		result->descriptor = inDescriptor;
		result->ticks = inTicks;
		result->icon = IconManager_NewIcon();
		error = IconManager_MakeIconRefFromBundleFile(result->icon, inBundleResourceNameNoExtension, inCreator, inType);
		
		if (error != noErr)
		{
			// destroy the new stage if any problems occur
			IconManager_DisposeIcon(&result->icon);
			Memory_DisposePtr(REINTERPRET_CAST(&result, Ptr*));
		}
	}
	return result;
}// newAnimationStage2


/*!
Sets the current animation stage to the beginning,
or to nothing if there are no icons in the given
Status IconÕs cell list.

(3.0)
*/
static void
resetAnimationStage		(StatusIconPtr		inIconPtr)
{
	if (inIconPtr->iconAnimationStages.empty())
	{
		inIconPtr->descriptorOfCurrentAnimationStage = kInvalidStatusIconAnimationStageDescriptor;
	}
	else
	{
		assert(!inIconPtr->iconAnimationStages.empty());
		inIconPtr->descriptorOfCurrentAnimationStage = inIconPtr->iconAnimationStages.front()->descriptor;
	}
}// resetAnimationStage

// BELOW IS REQUIRED NEWLINE TO END FILE
