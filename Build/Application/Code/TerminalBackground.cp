/*###############################################################

	TerminalBackground.cp
	
	MacTelnet
		© 1998-2007 by Kevin Grant.
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
#include <stdexcept>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <ColorUtilities.h>
#include <Console.h>

// MacTelnet includes
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "ContextualMenuBuilder.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "NetEvents.h"
#include "TerminalBackground.h"
#include "UIStrings.h"



#pragma mark Types

struct MyTerminalBackground
{
	MyTerminalBackground	(HIViewRef	inSuperclassViewInstance);
	
	HIViewRef	view;
	RGBColor	backgroundColor;
};
typedef MyTerminalBackground*			MyTerminalBackgroundPtr;
typedef MyTerminalBackground const*		MyTerminalBackgroundConstPtr;

#pragma mark Internal Method Prototypes

static OSStatus			receiveBackgroundContextualMenuSelect	(EventHandlerCallRef, EventRef,
																 MyTerminalBackgroundPtr);
static OSStatus			receiveBackgroundDraw					(EventHandlerCallRef, EventRef,
																 MyTerminalBackgroundPtr);
static OSStatus			receiveBackgroundHICommand				(EventHandlerCallRef, EventRef,
																 MyTerminalBackgroundPtr);
static pascal OSStatus  receiveBackgroundHIObjectEvents			(EventHandlerCallRef, EventRef, void*);
static OSStatus			receiveBackgroundRegionRequest			(EventHandlerCallRef, EventRef,
																 MyTerminalBackgroundPtr);
static OSStatus			receiveBackgroundSetData				(EventHandlerCallRef, EventRef,
																 MyTerminalBackgroundPtr);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	HIObjectClassRef	gMyBackgroundViewHIObjectClassRef = nullptr;
	EventHandlerUPP		gMyBackgroundViewConstructorUPP = nullptr;
	Boolean				gTerminalBackgroundInitialized = false;
}



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
									{ kEventClassControl, kEventControlContextualMenuClick },
									{ kEventClassControl, kEventControlGetPartRegion },
									{ kEventClassControl, kEventControlSetData },
									{ kEventClassCommand, kEventCommandProcess },
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
Creates a new HIView, complete with all the callbacks
and data necessary to drive a terminal background view
based on it.

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
TerminalBackground_CreateHIView		(WindowRef		inParentWindow,
								 	 HIViewRef&		outHIViewRef)
{
	EventRef	initializationEvent = nullptr;
	OSStatus	result = noErr;
	
	
	assert(gTerminalBackgroundInitialized);
	
	outHIViewRef = nullptr;
	
	result = CreateEvent(kCFAllocatorDefault, kEventClassHIObject, kEventHIObjectInitialize,
							GetCurrentEventTime(), kEventAttributeNone, &initializationEvent);
	if (noErr == result)
	{
		// set the parent window
		result = SetEventParameter(initializationEvent, kEventParamNetEvents_ParentWindow,
									typeWindowRef, sizeof(inParentWindow), &inParentWindow);
		if (noErr == result)
		{
			Boolean		keepView = false; // used to tell when everything succeeds
			
			
			// now construct!
			result = HIObjectCreate(kConstantsRegistry_HIObjectClassIDTerminalBackgroundView, initializationEvent,
									REINTERPRET_CAST(&outHIViewRef, HIObjectRef*));
			if (noErr == result)
			{
				UInt32		actualSize = 0;
				
				
				// the event handlers for this class of HIObject will attach a custom
				// control property to the new view, containing the TerminalViewRef
				result = GetControlProperty(outHIViewRef, kConstantsRegistry_ControlPropertyCreator,
											kConstantsRegistry_ControlPropertyTypeTerminalBackgroundData,
											sizeof(result), &actualSize, &result);
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

/*!
Constructor.  See receiveTerminalBackgroundHIObjectEvents().

(3.1)
*/
MyTerminalBackground::
MyTerminalBackground	(HIViewRef		inSuperclassViewInstance)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
view(inSuperclassViewInstance)
{
	backgroundColor.red = RGBCOLOR_INTENSITY_MAX;
	backgroundColor.green = RGBCOLOR_INTENSITY_MAX;
	backgroundColor.blue = RGBCOLOR_INTENSITY_MAX;
}// MyTerminalBackground 1-argument constructor


/*!
Handles "kEventControlContextualMenuClick" of "kEventClassControl"
for terminal backgrounds.

(3.1)
*/
static OSStatus
receiveBackgroundContextualMenuSelect	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
										 EventRef					inEvent,
										 MyTerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlContextualMenuClick);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the view in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, proceed
		if (noErr == result)
		{
			if (view == inMyTerminalBackgroundPtr->view)
			{
				// make this the current focus, so that menu commands are sent to it!
				SetUserFocusWindow(HIViewGetWindow(view));
				unless (HIViewSubtreeContainsFocus(view))
				{
					HIViewAdvanceFocus(view, 0/* modifiers */);
				}
				
				// display a contextual menu
				(OSStatus)ContextualMenuBuilder_DisplayMenuForView(view, inEvent);
				result = noErr; // event is completely handled
			}
			else
			{
				// ???
				result = eventNotHandledErr;
			}
		}
	}
	
	return result;
}// receiveBackgroundContextualMenuSelect


/*!
Handles "kEventControlDraw" of "kEventClassControl".

Paints the background color of the specified view.

(3.1)
*/
static OSStatus
receiveBackgroundDraw	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef					inEvent,
						 MyTerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus					result = eventNotHandledErr;
	MyTerminalBackgroundPtr		dataPtr = inMyTerminalBackgroundPtr;
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	{
		HIViewRef	control = nullptr;
		
		
		// get the target control
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the control was found, continue
		if (result == noErr)
		{
			//HIViewPartCode		partCode = 0;
			CGrafPtr			drawingPort = nullptr;
			CGrafPtr			oldPort = nullptr;
			GDHandle			oldDevice = nullptr;
			
			
			// find out the current port
			GetGWorld(&oldPort, &oldDevice);
			
			// could determine which part (if any) to draw; if none, draw everything
			// (ignored, not needed)
			//result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			//result = noErr; // ignore part code parameter if absent
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, drawingPort);
			if (result != noErr)
			{
				// use current port
				drawingPort = oldPort;
				result = noErr;
			}
			
			// if all information can be found, proceed with drawing
			if (result == noErr)
			{
				// paint background color and draw background picture, if any
				Rect	bounds;
				
				
				SetPort(drawingPort);
				
				GetControlBounds(control, &bounds);
				OffsetRect(&bounds, -bounds.left, -bounds.top);
				
				// start with a background similar to that of the text, but darker
				// so as to create a subtle border around the interior text area
				RGBBackColor(&dataPtr->backgroundColor);
					UseSelectionColors();//TMP
					UseSelectionColors();//TMP
				EraseRect(&bounds);
				
				// frame the background in a color that contrasts with the background
				// (this may become user-configurable); for now, take advantage of the
				// algorithm in UseSelectionColors(), which tends to use a color darker
				// than the background unless the background is close to black, in which
				// case a lighter gray is used
				{
					RGBColor	frameColor = {	INTEGER_TRIPLED(INTEGER_QUARTERED(RGBCOLOR_INTENSITY_MAX)),
												INTEGER_TRIPLED(INTEGER_QUARTERED(RGBCOLOR_INTENSITY_MAX)),
												INTEGER_TRIPLED(INTEGER_QUARTERED(RGBCOLOR_INTENSITY_MAX))	};
					
					
					PenNormal();
					RGBBackColor(&dataPtr->backgroundColor);
					UseSelectionColors();
					GetPortBackColor(drawingPort, &frameColor); // selection color is normally applied to background, but framing requires a foreground
					RGBForeColor(&frameColor);
					InsetRect(&bounds, 1, 1); // make the frame visible, as the control edges are usually covered by a 1-pixel overlap
					unless (IsControlActive(control)) UseInactiveColors();
					++(bounds.right);
					++(bounds.bottom);
					FrameRect(&bounds);
				}
				
				// frame the background in a color that contrasts with the background
				// (this may become user-configurable); for now, take advantage of the
				// algorithm in UseSelectionColors(), which tends to use a color darker
				// than the background unless the background is close to black, in which
				// case a lighter gray is used
				PenNormal();
				RGBBackColor(&dataPtr->backgroundColor);
				UseSelectionColors();
				RGBForeColor(&dataPtr->backgroundColor);
				InsetRect(&bounds, 1, 1); // make the frame visible, as the control edges are usually covered by a 1-pixel overlap
				unless (IsControlActive(control)) UseInactiveColors();
				++(bounds.right);
				++(bounds.bottom);
				FrameRect(&bounds);
			}
			
			// restore port
			SetGWorld(oldPort, oldDevice);
		}
	}
	return result;
}// receiveBackgroundDraw


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the terminal background.  Responds by displaying a
color chooser and changing the background color if the
user accepts.

(3.1)
*/
static OSStatus
receiveBackgroundHICommand	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef					inEvent,
							 MyTerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			// don’t claim to have handled any commands not shown below
			result = eventNotHandledErr;
			
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// execute a command
				switch (received.commandID)
				{
				case kCommandSetBackground:
					{
						UIStrings_Result		stringResult = kUIStrings_ResultOK;
						CFStringRef				askColorCFString = nullptr;
						PickerMenuItemInfo		editMenuInfo;
						Boolean					releaseAskColorCFString = true;
						
						
						stringResult = UIStrings_Copy(kUIStrings_SystemDialogPromptPickColor, askColorCFString);
						unless (stringResult.ok())
						{
							// cannot find prompt, but this is not a serious problem
							askColorCFString = CFSTR("");
							releaseAskColorCFString = false;
						}
						
						DeactivateFrontmostWindow();
						result = ColorUtilities_ColorChooserDialogDisplay
									(askColorCFString, &inMyTerminalBackgroundPtr->backgroundColor/* input */,
										&inMyTerminalBackgroundPtr->backgroundColor/* output */,
										true/* is modal */, NewUserEventUPP(EventLoop_HandleColorPickerUpdate),
										&editMenuInfo);
						RestoreFrontmostWindow();
						(OSStatus)HIViewSetNeedsDisplay(inMyTerminalBackgroundPtr->view, true);
						
						if (releaseAskColorCFString)
						{
							CFRelease(askColorCFString), askColorCFString = nullptr;
						}
					}
					result = noErr;
					break;
				
				default:
					// ???
					break;
				}
				break;
			
			default:
				// ???
				break;
			}
		}
	}
	
	return result;
}// receiveBackgroundHICommand


/*!
Handles standard events for the HIObject of a terminal
view’s background.

Invoked by Mac OS X.

(3.1)
*/
static pascal OSStatus
receiveBackgroundHIObjectEvents		(EventHandlerCallRef	inHandlerCallRef,
									 EventRef				inEvent,
									 void*					inMyTerminalBackgroundPtr)
{
	OSStatus					result = eventNotHandledErr;
	// IMPORTANT: This data is NOT valid during the kEventClassHIObject/kEventHIObjectConstruct
	// event: that is, in fact, when its value is defined.
	MyTerminalBackgroundPtr		dataPtr = REINTERPRET_CAST(inMyTerminalBackgroundPtr, MyTerminalBackgroundPtr);
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
			Console_WriteLine("HI OBJECT construct terminal background");
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
						MyTerminalBackgroundPtr		ptr = new MyTerminalBackground(superclassHIObjectAsHIView);
						
						
						assert(nullptr != ptr);
						
						// IMPORTANT: The setting of this parameter also ensures the context parameter
						// "inMyTerminalBackgroundPtr" is equal to this value when all other events enter
						// this function.
						result = SetEventParameter(inEvent, kEventParamHIObjectInstance,
													typeVoidPtr, sizeof(ptr), &ptr);
						if (noErr == result)
						{
							// IMPORTANT: Set a property with the MyTerminalBackgroundPtr, so that code
							// invoking HIObjectCreate() has a reasonable way to get this value.
							result = SetControlProperty(superclassHIObjectAsHIView,
														kConstantsRegistry_ControlPropertyCreator,
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
			Console_WriteLine("HI OBJECT initialize terminal background");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				WindowRef	owningWindow = nullptr;
				
				
				// get the parent window
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_ParentWindow,
																typeWindowRef, owningWindow);
				if (noErr == result)
				{
					assert(IsValidWindowRef(owningWindow));
					
					// place the created control in the right window
					HIViewRef	root = HIViewGetRoot(owningWindow);
					
					
					if (nullptr == root)
					{
						result = eventNotHandledErr;
					}
					else
					{
						HIViewRef	contentView = nullptr;
						
						
						result = HIViewFindByID(root, kHIViewWindowContentID, &contentView);
						if (noErr == result)
						{
							result = HIViewAddSubview(contentView, dataPtr->view);
							if (noErr == result)
							{
								// finally, initialize the view properly
								// none needed
								result = noErr;
							}
						}
					}
				}
			}
			break;
		
		case kEventHIObjectDestruct:
			///
			///!!! REMEMBER, THIS IS CALLED DIRECTLY BY THE TOOLBOX - NO CallNextEventHandler() ALLOWED!!!
			///
			Console_WriteLine("HI OBJECT destruct terminal background");
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
					if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXDescriptionAttribute, kCFCompareBackwards))
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
					else if (kCFCompareEqualTo == CFStringCompare(requestedAttribute, kAXRoleAttribute, kCFCompareBackwards))
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
							if (FlagManager_Test(kFlagOS10_4API))
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
							else
							{
								// no API available prior to 10.4 to find this value, so be lazy and return nothing
								result = eventNotHandledErr;
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
	else if (kEventClass == kEventClassCommand)
	{
		switch (kEventKind)
		{
		case kEventCommandProcess:
			result = receiveBackgroundHICommand(inHandlerCallRef, inEvent, dataPtr);
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
		
		case kEventControlContextualMenuClick:
			//Console_WriteLine("HI OBJECT control contextual menu click for terminal background");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				result = receiveBackgroundContextualMenuSelect(inHandlerCallRef, inEvent, dataPtr);
			}
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
		
		case kEventControlSetData:
			//Console_WriteLine("HI OBJECT control set data for terminal background");
			result = CallNextEventHandler(inHandlerCallRef, inEvent);
			if ((noErr == result) || (eventNotHandledErr == result))
			{
				result = receiveBackgroundSetData(inHandlerCallRef, inEvent, dataPtr);
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	return result;
}// receiveTerminalBackgroundHIObjectEvents


/*!
Handles "kEventControlGetPartRegion" of "kEventClassControl".

Returns the boundaries of the requested view part.

(3.1)
*/
static OSStatus
receiveBackgroundRegionRequest	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef					inEvent,
								 MyTerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus					result = eventNotHandledErr;
	MyTerminalBackgroundPtr		dataPtr = inMyTerminalBackgroundPtr;
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	//Console_WriteLine("event handler: terminal background region request");
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlGetPartRegion);
	{
		HIViewRef	control = nullptr;
		
		
		// get the target control
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the control was found, continue
		if (result == noErr)
		{
			HIViewPartCode		partNeedingRegion = kControlNoPart;
			
			
			// determine the part whose region is needed
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode,
															partNeedingRegion);
			if (result == noErr)
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
					GetControlBounds(dataPtr->view, &partBounds);
					SetRect(&partBounds, 0, 0, partBounds.right - partBounds.left, partBounds.bottom - partBounds.top);
					break;
				
				default:
					SetRect(&partBounds, 0, 0, 0, 0);
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
}// receiveTerminalBackgroundRegionRequest


/*!
Handles "kEventControlSetData" of "kEventClassControl".

Updates custom state, such as the background color or
picture.

(3.1)
*/
static OSStatus
receiveBackgroundSetData	(EventHandlerCallRef		UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef					inEvent,
							 MyTerminalBackgroundPtr	inMyTerminalBackgroundPtr)
{
	OSStatus					result = eventNotHandledErr;
	MyTerminalBackgroundPtr		dataPtr = inMyTerminalBackgroundPtr;
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	//Console_WriteLine("event handler: terminal background set data");
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlSetData);
	{
		// determine the type of data being set
		HIViewRef	control = nullptr;
		
		
		// get the target control
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the control was found, continue
		if (result == noErr)
		{
			HIViewPartCode		partNeedingDataSet = kControlNoPart;
			
			
			// determine the part for which data is being set
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode,
															partNeedingDataSet);
			if (result == noErr)
			{
				FourCharCode	dataType = '----';
				
				
				// determine the setting that is being changed
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlDataTag,
																typeEnumeration, dataType);
				if (noErr == result)
				{
					switch (dataType)
					{
					case kConstantsRegistry_ControlDataTagTerminalBackgroundColor:
						{
							SInt32		dataSize = 0;
							
							
							result = CarbonEventUtilities_GetEventParameter
										(inEvent, kEventParamControlDataBufferSize,
											typeLongInteger, dataSize);
							if (noErr == result)
							{
								RGBColor const*		colorPtr = nullptr;
								
								
								assert(dataSize == sizeof(RGBColor));
								result = CarbonEventUtilities_GetEventParameter
											(inEvent, kEventParamControlDataBuffer,
												typePtr, colorPtr);
								if (noErr == result)
								{
									dataPtr->backgroundColor = *colorPtr;
								}
							}
						}
						result = noErr;
						break;
					
					default:
						// ???
						break;
					}
				}
			}
		}
	}
	return result;
}// receiveBackgroundSetData

// BELOW IS REQUIRED NEWLINE TO END FILE
