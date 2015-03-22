/*!	\file TerminalBackground.cp
	\brief Renders the background of a terminal view
	(for example, a solid color or a picture).
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#include "TerminalBackground.h"
#include <UniversalDefines.h>

// standard-C++ includes
#include <stdexcept>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CFUtilities.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>

// application includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "NetEvents.h"
#include "Preferences.h"
#include "TerminalView.h"
#include "UIStrings.h"



#pragma mark Constants
namespace {

enum
{
	/*!
	Mac OS HIView part codes for Terminal Backgrounds (used ONLY when dealing
	with the HIView directly!!!).
	*/
	kTerminalBackground_ContentPartVoid		= kControlNoPart,	//!< nowhere in the content area
	kTerminalBackground_ContentPartText		= 1,				//!< draw a focus ring around entire view perimeter
};

/*!
Custom Carbon Event parameters.
*/
enum
{
	kEventParamTerminalBackground_ImageURL	= 'TBIU',	//!< CFStringRef; optional, specifies URL of image file to use for background
	kEventParamTerminalBackground_IsMatte	= 'TBMt',	//!< whether or not the view is opaque (data: typeBoolean)
};

} // anonymous namespace

#pragma mark Types
namespace {

struct My_TerminalBackground
{
	My_TerminalBackground	(HIViewRef	inSuperclassViewInstance);
	~My_TerminalBackground	();
	
	void
	setImageFromURLString	(CFStringRef);
	
	HIViewRef					view;
	HIViewPartCode				currentContentFocus;		//!< used in the content view focus handler to determine where (if anywhere) a ring goes
	CFRetainRelease				imageObject;				//!< actually represents an auto-retained/released CGImageRef for the background image
	CGImageRef					image;						//!< a convenient cast of the reference retained by "imageObject"; KEEP IN SYNC!
	Boolean						isMatte;					//!< if true, the view is considered completely opaque
	Boolean						dontDimBackgroundScreens;	//!< a cache of the preferences value, updated by the callback
	ListenerModel_ListenerWrap	preferenceMonitor;			//!< detects changes to important preference settings
};
typedef My_TerminalBackground*			My_TerminalBackgroundPtr;
typedef My_TerminalBackground const*	My_TerminalBackgroundConstPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void				preferenceChangedForBackground			(ListenerModel_Ref, ListenerModel_Event,
															 void*, void*);
OSStatus			receiveBackgroundActiveStateChange		(EventHandlerCallRef, EventRef,
															 My_TerminalBackgroundPtr);
OSStatus			receiveBackgroundDraw					(EventHandlerCallRef, EventRef,
															 My_TerminalBackgroundPtr);
OSStatus			receiveBackgroundFocus					(EventHandlerCallRef, EventRef,
															 My_TerminalBackgroundPtr);
OSStatus			receiveBackgroundHIObjectEvents			(EventHandlerCallRef, EventRef, void*);
OSStatus			receiveBackgroundHitTest				(EventHandlerCallRef, EventRef,
															 My_TerminalBackgroundPtr);
OSStatus			receiveBackgroundRegionRequest			(EventHandlerCallRef, EventRef,
															 My_TerminalBackgroundPtr);
OSStatus			receiveBackgroundTrack					(EventHandlerCallRef, EventRef,
															 My_TerminalBackgroundPtr);

} // anonymous namespace

#pragma mark Variables
namespace {

HIObjectClassRef	gMyBackgroundViewHIObjectClassRef = nullptr;
EventHandlerUPP		gMyBackgroundViewConstructorUPP = nullptr;
Boolean				gTerminalBackgroundInitialized = false;

} // anonymous namespace



#pragma mark Public Methods

/*!
Call this routine once, before any other method
from this module.

(3.1)
*/
void
TerminalBackground_Init ()
{
	// register a Human Interface Object class with Mac OS X
	// so that terminal view backgrounds can be easily created
	{
		EventTypeSpec const		whenHIObjectEventOccurs[] =
								{
									{ kEventClassHIObject, kEventHIObjectConstruct },
									{ kEventClassHIObject, kEventHIObjectInitialize },
									{ kEventClassHIObject, kEventHIObjectDestruct },
									{ kEventClassControl, kEventControlInitialize },
									{ kEventClassControl, kEventControlDraw },
									{ kEventClassControl, kEventControlActivate },
									{ kEventClassControl, kEventControlDeactivate },
									{ kEventClassControl, kEventControlHitTest },
									{ kEventClassControl, kEventControlTrack },
									{ kEventClassControl, kEventControlGetPartRegion },
									{ kEventClassControl, kEventControlGetFocusPart },
									{ kEventClassControl, kEventControlSetFocusPart },
									{ kEventClassAccessibility, kEventAccessibleGetAllAttributeNames },
									{ kEventClassAccessibility, kEventAccessibleGetNamedAttribute },
									{ kEventClassAccessibility, kEventAccessibleIsNamedAttributeSettable }
								};
		OSStatus				error = noErr;
		
		
		gMyBackgroundViewConstructorUPP = NewEventHandlerUPP(receiveBackgroundHIObjectEvents);
		assert(nullptr != gMyBackgroundViewConstructorUPP);
		error = HIObjectRegisterSubclass(kConstantsRegistry_HIObjectClassIDTerminalBackgroundView/* derived class */,
											kHIViewClassID/* base class */, 0/* options */,
											gMyBackgroundViewConstructorUPP,
											GetEventTypeCount(whenHIObjectEventOccurs), whenHIObjectEventOccurs,
											nullptr/* constructor data */, &gMyBackgroundViewHIObjectClassRef);
		assert_noerr(error);
	}
	
	gTerminalBackgroundInitialized = true;
}// Init


/*!
Call this routine when you are finished using
this module.

(3.1)
*/
void
TerminalBackground_Done ()
{
	gTerminalBackgroundInitialized = false;
	
	HIObjectUnregisterClass(gMyBackgroundViewHIObjectClassRef), gMyBackgroundViewHIObjectClassRef = nullptr;
	DisposeEventHandlerUPP(gMyBackgroundViewConstructorUPP), gMyBackgroundViewConstructorUPP = nullptr;
}// Done


/*!
Creates a new HIView, complete with all the callbacks and data
necessary to drive a terminal background view based on it.

If "inIsMatte" is true, the view is optimized for opaque
rendering and is not expected to have anything behind it.
Otherwise, it expects to be “in front” of something (perhaps
another Terminal Background view in a different color that
produces the matte effect).

A background image can be provided as well.  Currently, this
is experimental.

If any problems occur, nullptr is returned; otherwise, a
reference to the new view is returned.

IMPORTANT:	As with all APIs in this module, you must have
			called TerminalBackground_Init() first.  In
			this case, that will have registered the
			appropriate HIObject class with the Mac OS X
			Toolbox.

(3.1)
*/
OSStatus
TerminalBackground_CreateHIView		(HIViewRef&		outHIViewRef,
									 Boolean		inIsMatte,
									 CFStringRef	inImageURLOrNull)
{
	EventRef	initializationEvent = nullptr;
	OSStatus	result = noErr;
	
	
	assert(gTerminalBackgroundInitialized);
	
	outHIViewRef = nullptr;
	
	result = CreateEvent(kCFAllocatorDefault, kEventClassHIObject, kEventHIObjectInitialize,
							GetCurrentEventTime(), kEventAttributeNone, &initializationEvent);
	if (noErr == result)
	{
		// set the matte flag
		UNUSED_RETURN(OSStatus)SetEventParameter(initializationEvent, kEventParamTerminalBackground_IsMatte,
													typeBoolean, sizeof(inIsMatte), &inIsMatte);
		
		// set the image flag
		if (nullptr != inImageURLOrNull)
		{
			UNUSED_RETURN(OSStatus)SetEventParameter(initializationEvent, kEventParamTerminalBackground_ImageURL,
														typeCFStringRef, sizeof(inImageURLOrNull), &inImageURLOrNull);
		}
		
		// set the parent window
		//result = SetEventParameter(initializationEvent, kEventParamNetEvents_ParentWindow,
		//							typeWindowRef, sizeof(inParentWindow), &inParentWindow);
		//if (noErr == result)
		{
			Boolean		keepView = false; // used to tell when everything succeeds
			
			
			// now construct!
			result = HIObjectCreate(kConstantsRegistry_HIObjectClassIDTerminalBackgroundView, initializationEvent,
									REINTERPRET_CAST(&outHIViewRef, HIObjectRef*));
			if (noErr == result)
			{
				My_TerminalBackgroundPtr	ptr = nullptr;
				UInt32						actualSize = 0;
				
				
				// the event handlers for this class of HIObject will attach a custom
				// control property to the new view, containing the internal structure
				result = GetControlProperty(outHIViewRef, AppResources_ReturnCreatorCode(),
											kConstantsRegistry_ControlPropertyTypeTerminalBackgroundData,
											sizeof(ptr), &actualSize, &ptr);
				if (noErr == result)
				{
					// success!
					keepView = true;
				}
			}
			
			// any errors?
			unless (keepView)
			{
				outHIViewRef = nullptr;
			}
		}
		
		ReleaseEvent(initializationEvent);
	}
	return result;
}// CreateHIView


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See receiveBackgroundHIObjectEvents().

(3.1)
*/
My_TerminalBackground::
My_TerminalBackground	(HIViewRef		inSuperclassViewInstance)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
view						(inSuperclassViewInstance),
currentContentFocus			(kControlNoPart),
imageObject					(),
image						(nullptr),
isMatte						(true),
dontDimBackgroundScreens	(false), // reset by callback
preferenceMonitor			(ListenerModel_NewStandardListener(preferenceChangedForBackground, this/* context */), true/* is retained */)
{
	OSStatus		error = noErr;
	CGDeviceColor	initialColor;
	
	
	initialColor.red = 1.0;
	initialColor.green = 1.0;
	initialColor.blue = 1.0;
	error = SetControlProperty(view, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeBackgroundColor,
								sizeof(initialColor), &initialColor);
	assert_noerr(error);
	
	// set up a callback to receive preference change notifications
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_StartMonitoring(this->preferenceMonitor.returnRef(), kPreferences_TagDontDimBackgroundScreens,
													true/* call immediately to get initial value */);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "terminal background failed to monitor dimming setting, error", prefsResult);
		}
	}
}// My_TerminalBackground 1-argument constructor


/*!
Destructor.

(4.0)
*/
My_TerminalBackground::
~My_TerminalBackground ()
{
	// stop receiving preference change notifications
	Preferences_StopMonitoring(this->preferenceMonitor.returnRef(), kPreferences_TagDontDimBackgroundScreens);
}// My_TerminalBackground destructor


/*!
Reads the specified image file, and stores its image data in
memory.  The image is then rendered later by the view.

(4.0)
*/
void
My_TerminalBackground::
setImageFromURLString	(CFStringRef	inURLCFString)
{
	CFRetainRelease		imageURLObject(CFURLCreateWithString
										(kCFAllocatorDefault, inURLCFString, nullptr/* base URL */), true/* is retained */);
	CFURLRef			imageURL = CFUtilities_URLCast(imageURLObject.returnCFTypeRef());
	CFRetainRelease		imageDataObject(CGImageSourceCreateWithURL(imageURL, nullptr/* options */), true/* is retained */);
	CGImageSourceRef	imageData = (CGImageSourceRef)imageDataObject.returnCFTypeRef();
	
	
	this->imageObject = CGImageSourceCreateImageAtIndex(imageData, 0/* index */, nullptr/* options */);
	
	// for convenience, pre-cast the retained reference and store it
	this->image = (CGImageRef)this->imageObject.returnCFTypeRef();
}// My_TerminalBackground::setImageFromURLString


/*!
Invoked whenever a monitored preference value is changed for
a particular view (see the My_TerminalBackground constructor
to see which preferences are monitored).  This routine responds
by updating any necessary data and display elements.

(4.0)
*/
void
preferenceChangedForBackground	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
								 ListenerModel_Event	inPreferenceTagThatChanged,
								 void*					UNUSED_ARGUMENT(inContext),
								 void*					inMyTerminalBackgroundPtr)
{
	My_TerminalBackgroundPtr	dataPtr = REINTERPRET_CAST(inMyTerminalBackgroundPtr, My_TerminalBackgroundPtr);
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagDontDimBackgroundScreens:
		{
			// update global variable with current preference value
			unless (kPreferences_ResultOK ==
					Preferences_GetData(kPreferences_TagDontDimBackgroundScreens, sizeof(dataPtr->dontDimBackgroundScreens),
										&dataPtr->dontDimBackgroundScreens))
			{
				dataPtr->dontDimBackgroundScreens = false; // assume a value, if preference can’t be found
			}
			
			// redraw the background
			UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplay(dataPtr->view, true);
		}
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChangedForBackground


/*!
Handles "kEventControlActivate" and "kEventControlDeactivate"
of "kEventClassControl".

Invoked by Mac OS X whenever a terminal background is activated
or dimmed.

(4.0)
*/
OSStatus
receiveBackgroundActiveStateChange	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef					inEvent,
									 My_TerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert((kEventKind == kEventControlActivate) || (kEventKind == kEventControlDeactivate));
	
	// since inactive and active terminal backgrounds have different appearances,
	// force redraws when they are activated or deactivated
	unless (inMyTerminalBackgroundPtr->dontDimBackgroundScreens)
	{
		HIViewRef	viewBeingActivatedOrDeactivated = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef,
														viewBeingActivatedOrDeactivated);
		if (noErr == result)
		{
			result = HIViewSetNeedsDisplay(viewBeingActivatedOrDeactivated, true);
		}
	}
	
	return result;
}// receiveBackgroundActiveStateChange


/*!
Handles "kEventControlDraw" of "kEventClassControl".

Paints the background color of the specified view.

(3.1)
*/
OSStatus
receiveBackgroundDraw	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef					inEvent,
						 My_TerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_TerminalBackgroundPtr	dataPtr = inMyTerminalBackgroundPtr;
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		if (noErr == result)
		{
			//HIViewPartCode		partCode = 0;
			CGrafPtr			drawingPort = nullptr;
			CGContextRef		drawingContext = nullptr;
			CGrafPtr			oldPort = nullptr;
			GDHandle			oldDevice = nullptr;
			CGDeviceColor		backgroundColor;
			
			
			// determine background color
			result = GetControlProperty(view, AppResources_ReturnCreatorCode(),
										kConstantsRegistry_ControlPropertyTypeBackgroundColor,
										sizeof(backgroundColor), nullptr/* actual size */, &backgroundColor);
			assert_noerr(result);
			
			// find out the current port
			GetGWorld(&oldPort, &oldDevice);
			
			// could determine which part (if any) to draw; if none, draw everything
			// (ignored, not needed)
			//result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			//result = noErr; // ignore part code parameter if absent
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, drawingPort);
			if (noErr != result)
			{
				// use current port
				drawingPort = oldPort;
			}
			
			// determine the context to draw in with Core Graphics
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef,
															drawingContext);
			assert_noerr(result);
			
			// if all information can be found, proceed with drawing
			if (noErr == result)
			{
				// paint background color and draw background picture, if any
				Rect		bounds;
				Rect		clipBounds;
				HIRect		floatBounds;
				HIRect		floatClipBounds;
				RgnHandle	optionalTargetRegion = nullptr;
				
				
				SetPort(drawingPort);
				
				// determine boundaries of the content view being drawn;
				// ensure view-local coordinates
				HIViewGetBounds(view, &floatBounds);
				GetControlBounds(view, &bounds);
				OffsetRect(&bounds, -bounds.left, -bounds.top);
				
				// maybe a focus region has been provided
				if (noErr == CarbonEventUtilities_GetEventParameter(inEvent, kEventParamRgnHandle, typeQDRgnHandle,
																	optionalTargetRegion))
				{
					GetRegionBounds(optionalTargetRegion, &clipBounds);
					floatClipBounds = CGRectMake(clipBounds.left, clipBounds.top, clipBounds.right - clipBounds.left,
													clipBounds.bottom - clipBounds.top);
				}
				else
				{
					clipBounds = bounds;
					floatClipBounds = floatBounds;
				}
				
				{
					RGBColor	tmpColor;
					
					
					tmpColor.red = STATIC_CAST(STATIC_CAST(RGBCOLOR_INTENSITY_MAX, Float32) * backgroundColor.red, unsigned short);
					tmpColor.green = STATIC_CAST(STATIC_CAST(RGBCOLOR_INTENSITY_MAX, Float32) * backgroundColor.green, unsigned short);
					tmpColor.blue = STATIC_CAST(STATIC_CAST(RGBCOLOR_INTENSITY_MAX, Float32) * backgroundColor.blue, unsigned short);
					RGBBackColor(&tmpColor);
				}
				if ((false == IsControlActive(view)) && (false == dataPtr->dontDimBackgroundScreens))
				{
					UseInactiveColors();
				}
				EraseRect(&clipBounds);
				
				if (nullptr != dataPtr->image)
				{
					UNUSED_RETURN(OSStatus)HIViewDrawCGImage(drawingContext, &floatBounds, dataPtr->image);
				}
			}
			
			// restore port
			SetGWorld(oldPort, oldDevice);
		}
	}
	return result;
}// receiveBackgroundDraw


/*!
Handles "kEventControlSetFocusPart" and "kEventControlGetFocusPart"
of "kEventClassControl".

Invoked by Mac OS X whenever the currently-focused part of a
terminal view should be changed or returned.

(3.0)
*/
OSStatus
receiveBackgroundFocus	(EventHandlerCallRef		inHandlerCallRef,
						 EventRef					inEvent,
						 My_TerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert((kEventKind == kEventControlSetFocusPart) || (kEventKind == kEventControlGetFocusPart));
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		if (noErr == result)
		{
			// when setting the part, find directions in the part code parameter
			if (kEventKind == kEventControlSetFocusPart)
			{
				HIViewPartCode		focusPart = kControlNoPart;
				
				
				UNUSED_RETURN(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, focusPart);
				if (noErr == result)
				{
					HIViewPartCode		newFocusPart = kTerminalBackground_ContentPartVoid;
					
					
					switch (focusPart)
					{
					case kControlFocusNoPart:
						newFocusPart = kTerminalBackground_ContentPartVoid;
						break;
					
					case kControlFocusNextPart:
						// advance focus
						switch (inMyTerminalBackgroundPtr->currentContentFocus)
						{
						case kTerminalBackground_ContentPartVoid:
							// when the previous view is highlighted and focus advances, the
							// entire terminal view content area should be the next focus target
							newFocusPart = kTerminalBackground_ContentPartText;
							break;
						
						case kTerminalBackground_ContentPartText:
						default:
							// when the entire terminal view is highlighted and focus advances,
							// the next view should be the next focus target
							newFocusPart = kTerminalBackground_ContentPartVoid;
							break;
						}
						break;
					
					case kControlFocusPrevPart:
						// reverse focus
						switch (inMyTerminalBackgroundPtr->currentContentFocus)
						{
						case kTerminalBackground_ContentPartVoid:
							// when the next view is highlighted and focus backs up, the
							// entire terminal view content area should be the next focus target
							newFocusPart = kTerminalBackground_ContentPartText;
							break;
						
						case kTerminalBackground_ContentPartText:
						default:
							// when the entire terminal view is highlighted and focus backs up,
							// the previous view should be the next focus target
							newFocusPart = kTerminalBackground_ContentPartVoid;
							break;
						}
						break;
					
					default:
						// set focus to given part
						newFocusPart = focusPart;
						break;
					}
					
					if (inMyTerminalBackgroundPtr->currentContentFocus != newFocusPart)
					{
						// update focus flag
						inMyTerminalBackgroundPtr->currentContentFocus = newFocusPart;
					}
				}
			}
			
			// update the part code parameter with the stored focus part
			result = SetEventParameter(inEvent, kEventParamControlPart,
										typeControlPartCode, sizeof(inMyTerminalBackgroundPtr->currentContentFocus),
										&inMyTerminalBackgroundPtr->currentContentFocus);
		}
	}
	return result;
}// receiveBackgroundFocus


/*!
Handles standard events for the HIObject of a terminal
view’s background.

Invoked by Mac OS X.

(3.1)
*/
OSStatus
receiveBackgroundHIObjectEvents		(EventHandlerCallRef	inHandlerCallRef,
									 EventRef				inEvent,
									 void*					inMyTerminalBackgroundPtr)
{
	OSStatus					result = eventNotHandledErr;
	// IMPORTANT: This data is NOT valid during the kEventClassHIObject/kEventHIObjectConstruct
	// event: that is, in fact, when its value is defined.
	My_TerminalBackgroundPtr	dataPtr = REINTERPRET_CAST(inMyTerminalBackgroundPtr, My_TerminalBackgroundPtr);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	if (kEventClass == kEventClassHIObject)
	{
		switch (kEventKind)
		{
		case kEventHIObjectConstruct:
			///
			///!!! REMEMBER, THIS IS CALLED DIRECTLY BY THE TOOLBOX - NO CallNextEventHandler() ALLOWED!!!
			///
			//Console_WriteLine("HI OBJECT construct terminal background");
			{
				HIObjectRef		superclassHIObject = nullptr;
				
				
				// get the superclass view that has already been constructed (but not initialized)
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamHIObjectInstance,
																typeHIObjectRef, superclassHIObject);
				if (noErr == result)
				{
					// allocate a view without initializing it
					HIViewRef		superclassHIObjectAsHIView = REINTERPRET_CAST(superclassHIObject, HIViewRef);
					
					
					try
					{
						My_TerminalBackgroundPtr	ptr = new My_TerminalBackground(superclassHIObjectAsHIView);
						
						
						assert(nullptr != ptr);
						
						// IMPORTANT: The setting of this parameter also ensures the context parameter
						// "inMyTerminalBackgroundPtr" is equal to this value when all other events enter
						// this function.
						result = SetEventParameter(inEvent, kEventParamHIObjectInstance,
													typeVoidPtr, sizeof(ptr), &ptr);
						if (noErr == result)
						{
							// IMPORTANT: Set a property with the My_TerminalBackgroundPtr, so that code
							// invoking HIObjectCreate() has a reasonable way to get this value.
							result = SetControlProperty(superclassHIObjectAsHIView,
														AppResources_ReturnCreatorCode(),
														kConstantsRegistry_ControlPropertyTypeTerminalBackgroundData,
														sizeof(ptr), &ptr);
						}
					}
					catch (std::exception)
					{
						result = memFullErr;
					}
				}
			}
			break;
		
		case kEventHIObjectInitialize:
			//Console_WriteLine("HI OBJECT initialize terminal background");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				OSStatus		error = noErr;
				CFStringRef		urlCFString = nullptr;
				Boolean			flag = false;
				
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamTerminalBackground_IsMatte, typeBoolean, flag);
				if (noErr == result)
				{
					dataPtr->isMatte = flag;
				}
				
				error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamTerminalBackground_ImageURL, typeCFStringRef, urlCFString);
				if (noErr == error)
				{
					dataPtr->setImageFromURLString(urlCFString);
				}
				
				result = noErr;
			}
			break;
		
		case kEventHIObjectDestruct:
			///
			///!!! REMEMBER, THIS IS CALLED DIRECTLY BY THE TOOLBOX - NO CallNextEventHandler() ALLOWED!!!
			///
			//Console_WriteLine("HI OBJECT destruct terminal background");
			delete dataPtr;
			result = noErr;
			break;
		
		default:
			// ???
			break;
		}
	}
	else if (kEventClass == kEventClassAccessibility)
	{
		assert(kEventClass == kEventClassAccessibility);
		switch (kEventKind)
		{
		case kEventAccessibleGetAllAttributeNames:
			{
				CFMutableArrayRef	listOfNames = nullptr;
				
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamAccessibleAttributeNames,
																typeCFMutableArrayRef, listOfNames);
				if (noErr == result)
				{
					// each attribute mentioned here should be handled below
					CFArrayAppendValue(listOfNames, kAXDescriptionAttribute);
					CFArrayAppendValue(listOfNames, kAXRoleAttribute);
					CFArrayAppendValue(listOfNames, kAXRoleDescriptionAttribute);
					CFArrayAppendValue(listOfNames, kAXTopLevelUIElementAttribute);
					CFArrayAppendValue(listOfNames, kAXWindowAttribute);
					CFArrayAppendValue(listOfNames, kAXParentAttribute);
					CFArrayAppendValue(listOfNames, kAXEnabledAttribute);
					CFArrayAppendValue(listOfNames, kAXPositionAttribute);
					CFArrayAppendValue(listOfNames, kAXSizeAttribute);
				}
			}
			break;
		
		case kEventAccessibleGetNamedAttribute:
		case kEventAccessibleIsNamedAttributeSettable:
			{
				CFStringRef		requestedAttribute = nullptr;
				
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamAccessibleAttributeName,
																typeCFStringRef, requestedAttribute);
				if (noErr == result)
				{
					// for the purposes of accessibility, identify a Terminal Background as
					// having the same role as a standard matte
					CFStringRef		roleCFString = kAXMatteRole;
					Boolean			isSettable = false;
					
					
					// IMPORTANT: The cases handled here should match the list returned
					// by "kEventAccessibleGetAllAttributeNames", above.
					if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXRoleAttribute, kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFStringRef,
														sizeof(roleCFString), &roleCFString);
						}
					}
					else if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXRoleDescriptionAttribute,
																	kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							CFStringRef		roleDescCFString = HICopyAccessibilityRoleDescription
																(roleCFString, nullptr/* sub-role */);
							
							
							if (nullptr != roleDescCFString)
							{
								result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFStringRef,
															sizeof(roleDescCFString), &roleDescCFString);
								CFRelease(roleDescCFString), roleDescCFString = nullptr;
							}
						}
					}
					else if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXDescriptionAttribute, kCFCompareBackwards))
					{
						isSettable = false;
						if (kEventAccessibleGetNamedAttribute == kEventKind)
						{
							UIStrings_Result	stringResult = kUIStrings_ResultOK;
							CFStringRef			descriptionCFString = nullptr;
							
							
							stringResult = UIStrings_Copy(kUIStrings_TerminalBackgroundAccessibilityDescription,
															descriptionCFString);
							if (false == stringResult.ok())
							{
								result = resNotFound;
							}
							else
							{
								result = SetEventParameter(inEvent, kEventParamAccessibleAttributeValue, typeCFStringRef,
															sizeof(descriptionCFString), &descriptionCFString);
							}
						}
					}
					else
					{
						// Many attributes are already supported by the default handler:
						//	kAXTopLevelUIElementAttribute
						//	kAXWindowAttribute
						//	kAXParentAttribute
						//	kAXEnabledAttribute
						//	kAXSizeAttribute
						//	kAXPositionAttribute
						result = eventNotHandledErr;
					}
					
					// return the read-only flag when requested, if the attribute was used above
					if ((noErr == result) &&
						(kEventAccessibleIsNamedAttributeSettable == kEventKind))
					{
						result = SetEventParameter(inEvent, kEventParamAccessibleAttributeSettable, typeBoolean,
													sizeof(isSettable), &isSettable);
					}
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	else
	{
		assert(kEventClass == kEventClassControl);
		switch (kEventKind)
		{
		case kEventControlInitialize:
			//Console_WriteLine("HI OBJECT control initialize for terminal background");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if (noErr == result)
			{
				UInt32		controlFeatures = kControlSupportsEmbedding |
												kControlSupportsContextualMenus;
				
				
				// return the features of this control
				result = SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32,
											sizeof(controlFeatures), &controlFeatures);
				assert_noerr(result);
			}
			break;
		
		case kEventControlActivate:
		case kEventControlDeactivate:
			//Console_WriteLine("HI OBJECT control activate or deactivate for terminal background");
			result = receiveBackgroundActiveStateChange(inHandlerCallRef, inEvent, dataPtr);
			break;
		
		case kEventControlHitTest:
			//Console_WriteLine("HI OBJECT hit test for terminal background");
			result = receiveBackgroundHitTest(inHandlerCallRef, inEvent, dataPtr);
			break;
		
		case kEventControlTrack:
			//Console_WriteLine("HI OBJECT control track for terminal background");
			result = receiveBackgroundTrack(inHandlerCallRef, inEvent, dataPtr);
			break;
		
		case kEventControlDraw:
			//Console_WriteLine("HI OBJECT control draw for terminal background");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				result = receiveBackgroundDraw(inHandlerCallRef, inEvent, dataPtr);
			}
			break;
		
		case kEventControlGetPartRegion:
			//Console_WriteLine("HI OBJECT control get part region for terminal background");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				result = receiveBackgroundRegionRequest(inHandlerCallRef, inEvent, dataPtr);
			}
			break;
		
		case kEventControlGetFocusPart:
		case kEventControlSetFocusPart:
			//Console_WriteLine("HI OBJECT control set focus part for terminal background");
			result = receiveBackgroundFocus(inHandlerCallRef, inEvent, dataPtr);
			break;
		
		default:
			// ???
			break;
		}
	}
	return result;
}// receiveBackgroundHIObjectEvents


/*!
Handles "kEventControlHitTest" of "kEventClassControl"
for terminal backgrounds.

(4.0)
*/
OSStatus
receiveBackgroundHitTest	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef					inEvent,
							 My_TerminalBackgroundPtr	UNUSED_ARGUMENT(inMyTerminalBackgroundPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHitTest);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (noErr == result)
		{
			HIPoint   localMouse;
			
			
			// determine where the mouse is
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, localMouse);
			if (noErr == result)
			{
				HIViewPartCode		hitPart = kControlNoPart;
				HIRect				viewBounds;
				
				
				// find the part the mouse is in
				HIViewGetBounds(view, &viewBounds);
				if (CGRectContainsPoint(viewBounds, localMouse))
				{
					hitPart = 1; // arbitrary; a hack, really...parts are never used otherwise, but has to be nonzero
				}
				
				// update the part code parameter with the part under the mouse
				result = SetEventParameter(inEvent, kEventParamControlPart,
											typeControlPartCode, sizeof(hitPart), &hitPart);
			}
		}
	}
	
	return result;
}// receiveBackgroundHitTest


/*!
Handles "kEventControlGetPartRegion" of "kEventClassControl".

Returns the boundaries of the requested view part.

(3.1)
*/
OSStatus
receiveBackgroundRegionRequest	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef					inEvent,
								 My_TerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_TerminalBackgroundPtr	dataPtr = inMyTerminalBackgroundPtr;
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	//Console_WriteLine("event handler: terminal background region request");
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlGetPartRegion);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		if (noErr == result)
		{
			HIViewPartCode		partNeedingRegion = kControlNoPart;
			
			
			// determine the part whose region is needed
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode,
															partNeedingRegion);
			if (noErr == result)
			{
				Rect	partBounds;
				
				
				// IMPORTANT: All regions are currently rectangles, so this code is simplified.
				// If any irregular regions are added in the future, this has to be restructured.
				switch (partNeedingRegion)
				{
				case kControlStructureMetaPart:
				case kControlContentMetaPart:
				case kControlOpaqueMetaPart:
				case kControlClickableMetaPart:
				case kTerminalBackground_ContentPartText:
					GetControlBounds(dataPtr->view, &partBounds);
					SetRect(&partBounds, 0, 0, partBounds.right - partBounds.left, partBounds.bottom - partBounds.top);
					break;
				
				case kTerminalBackground_ContentPartVoid:
				default:
					bzero(&partBounds, sizeof(partBounds));
					break;
				}
				
				//Console_WriteValue("request was for region code", partNeedingRegion);
				//Console_WriteValueFloat4("returned terminal background region bounds",
				//							STATIC_CAST(partBounds.left, Float32),
				//							STATIC_CAST(partBounds.top, Float32),
				//							STATIC_CAST(partBounds.right, Float32),
				//							STATIC_CAST(partBounds.bottom, Float32));
				
				// attach the rectangle to the event, because sometimes this is most efficient
				result = SetEventParameter(inEvent, kEventParamControlPartBounds,
													typeQDRectangle, sizeof(partBounds), &partBounds);
				
				// perhaps a region was given that should be filled in
				{
					RgnHandle	regionToSet = nullptr;
					OSStatus	error = noErr;
					
					
					error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlRegion, typeQDRgnHandle,
																	regionToSet);
					if (noErr == error)
					{
						// modify the given region, which effectively returns the boundaries to the caller
						RectRgn(regionToSet, &partBounds);
						result = noErr;
					}
				}
			}
		}
	}
	return result;
}// receiveBackgroundRegionRequest


/*!
Handles "kEventControlTrack" of "kEventClassControl"
for terminal backgrounds.

There is an optical illusion when a terminal background
directly contains a terminal view, where the user might
try to click on the view and really hit the background;
this therefore intercepts clicks and forwards them to
the view’s nearest point.

(4.0)
*/
OSStatus
receiveBackgroundTrack	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef					inEvent,
						 My_TerminalBackgroundPtr	UNUSED_ARGUMENT(inMyTerminalBackgroundPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlTrack);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the view in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		if (noErr == result)
		{
			Point		viewLocalMouse;
			
			
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, viewLocalMouse);
			if (noErr == result)
			{
				HIViewRef	terminalContentView = nullptr;
				
				
				result = HIViewFindByID(view, TerminalView_ReturnContainerHIViewID(), &terminalContentView);
				if (noErr == result)
				{
					HIPoint		translatedPoint;
					
					
					translatedPoint.x = viewLocalMouse.h;
					translatedPoint.y = viewLocalMouse.v;
					
					// NOTE: This is basically a large hack, and it is temporary, until all of this
					// moves to Cocoa anyway.  Basically, if the user clicks in a background view,
					// the click should appear to be in the content view instead.
					result = HIViewConvertPoint(&translatedPoint, view, terminalContentView);
					if (noErr == result)
					{
						Point	truncatedPoint;
						
						
						truncatedPoint.h = STATIC_CAST(translatedPoint.x, unsigned short);
						truncatedPoint.v = STATIC_CAST(translatedPoint.y, unsigned short);
						result = TrackControl(terminalContentView, truncatedPoint, nullptr/* action routine */);
					}
				}
			}
		}
		
		if (noErr != result)
		{
			result = eventNotHandledErr;
		}
	}
	
	return result;
}// receiveBackgroundTrack

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
