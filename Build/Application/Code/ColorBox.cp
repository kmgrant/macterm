/*!	\file ColorBox.cp
	\brief User interface element for selecting colors.
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

#include "ColorBox.h"
#include <UniversalDefines.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <CGContextSaveRestore.h>
#include <CocoaBasic.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <FlagManager.h>
#include <RegionUtilities.h>

// application includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "UIStrings.h"



#pragma mark Types
namespace {

/*!
A pointer to this data is attached to a view using the property
"kConstantsRegistry_ControlPropertyTypeColorBoxData", so that
it is available within callback routines.
*/
struct MyColorBoxData
{
	HIViewRef						viewRef;			//!< the user pane to which this information is attached
	EventHandlerRef					buttonDrawHandler;	//!< responds to requests to paint the color box
	EventHandlerUPP					uppDraw;			//!< Mac OS wrapper for callback
	RGBColor						displayedColor;		//!< which color (the “value”) the color box displays
	ColorBox_ChangeNotifyProcPtr	notifyProcPtr;		//!< routine, if any, to call when the value is changed
	void*							contextPtr;			//!< caller-defined context to pass to the notification routine
};
typedef MyColorBoxData*		MyColorBoxDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

OSStatus	receiveColorBoxDraw		(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Assigns a bevel button view the drawing embellishment for
a color box, and attaches data to it using a property.
Also provides a simple accessibility description (you may
wish to replace this with a more precise definition of
the purpose of your color box).

Be sure to invoke ColorBox_DetachFromBevelButton() when
you are finished with the color box, to dispose of data
allocated by invoking this routine.

IMPORTANT:	To properly support modeless the color panel,
			your bevel button should have “sticky”
			behavior.  This allows the controller to
			clearly show the user which bevel button is
			the target of the panel.

(3.1)
*/
OSStatus
ColorBox_AttachToBevelButton	(HIViewRef			inoutBevelButton,
								 RGBColor const*	inInitialColorOrNull)
{
	OSStatus	result = noErr;
	
	
	try
	{
		MyColorBoxDataPtr	dataPtr = new MyColorBoxData;
		
		
		dataPtr->viewRef = inoutBevelButton;
		dataPtr->notifyProcPtr = nullptr;
		
		// install a callback that paints the button
		{
			EventTypeSpec const		whenControlNeedsDrawing[] =
									{
										{ kEventClassControl, kEventControlDraw }
									};
			
			
			dataPtr->uppDraw = NewEventHandlerUPP(receiveColorBoxDraw);
			result = HIViewInstallEventHandler(inoutBevelButton, dataPtr->uppDraw,
												GetEventTypeCount(whenControlNeedsDrawing),
												whenControlNeedsDrawing, dataPtr/* user data */,
												&dataPtr->buttonDrawHandler/* event handler reference */);
		}
		
		if (noErr == result)
		{
			// initialize color
			if (nullptr != inInitialColorOrNull)
			{
				dataPtr->displayedColor = *inInitialColorOrNull;
			}
			else
			{
				dataPtr->displayedColor.red = dataPtr->displayedColor.green = dataPtr->displayedColor.blue = 0;
			}
			
			// attach data to view
			result = SetControlProperty(inoutBevelButton, AppResources_ReturnCreatorCode(),
										kConstantsRegistry_ControlPropertyTypeColorBoxData,
										sizeof(dataPtr), &dataPtr);
			
			// set accessibility relationships, if possible
			{
				CFStringRef		accessibilityDescCFString = nullptr;
				
				
				if (UIStrings_Copy(kUIStrings_ButtonColorBoxAccessibilityDesc, accessibilityDescCFString).ok())
				{
					HIObjectRef const	kViewObjectRef = REINTERPRET_CAST(inoutBevelButton, HIObjectRef);
					OSStatus			error = noErr;
					
					
					error = HIObjectSetAuxiliaryAccessibilityAttribute
							(kViewObjectRef, 0/* sub-component identifier */,
								kAXDescriptionAttribute, accessibilityDescCFString);
					if (noErr != error)
					{
						Console_Warning(Console_WriteValue, "failed to set accessibility description of bevel button, error", error);
					}
					CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
				}
			}
		}
	}
	catch (std::bad_alloc)
	{
		result = memFullErr;
	}
	
	return result;
}// AttachToBevelButton


/*!
Disposes of a bevel button’s drawing routine, initially
installed using one of the ColorBox_AttachTo...() routines.

The accessibility description is not removed, because this
routine does not know if you have since replaced the button’s
accessibility information with something different.

(3.1)
*/
void
ColorBox_DetachFromBevelButton		(HIViewRef		inView)
{
	OSStatus			error = noErr;
	MyColorBoxDataPtr	dataPtr = nullptr;
	UInt32				actualSize = 0;
	
	
	error = GetControlProperty(inView, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeColorBoxData,
								sizeof(dataPtr), &actualSize, &dataPtr);
	assert_noerr(error);
	assert(sizeof(dataPtr) == actualSize);
	
	if (nullptr != dataPtr)
	{
		// dispose of callback data
		RemoveEventHandler(dataPtr->buttonDrawHandler), dataPtr->buttonDrawHandler = nullptr;
		DisposeEventHandlerUPP(dataPtr->uppDraw), dataPtr->uppDraw = nullptr;
		
		// dispose of color box data
		delete dataPtr, dataPtr = nullptr;
	}
}// DetachFromBevelButton


/*!
Returns the color displayed by a color box.

(3.0)
*/
void
ColorBox_GetColor	(HIViewRef		inView,
					 RGBColor*		outColorPtr)
{
	OSStatus			error = noErr;
	MyColorBoxDataPtr	dataPtr = nullptr;
	UInt32				actualSize = 0;
	
	
	error = GetControlProperty(inView, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeColorBoxData,
								sizeof(dataPtr), &actualSize, &dataPtr);
	assert_noerr(error);
	assert(sizeof(dataPtr) == actualSize);
	
	if ((nullptr != outColorPtr) && (nullptr != dataPtr))
	{
		*outColorPtr = dataPtr->displayedColor;
	}
}// GetColor


/*!
Changes the color displayed by a color box.

If "inIsUserAction" is true, the associated callback (if
any) is also invoked.  You should only invoke the callback
for color changes known to be the direct result of a user
action, such as using the system Colors window.

(3.0)
*/
void
ColorBox_SetColor	(HIViewRef			inView,
					 RGBColor const*	inColorPtr,
					 Boolean			inIsUserAction)
{
	OSStatus			error = noErr;
	MyColorBoxDataPtr	dataPtr = nullptr;
	UInt32				actualSize = 0;
	
	
	error = GetControlProperty(inView, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeColorBoxData,
								sizeof(dataPtr), &actualSize, &dataPtr);
	assert_noerr(error);
	assert(sizeof(dataPtr) == actualSize);
	
	if ((nullptr != inColorPtr) && (nullptr != dataPtr))
	{
		dataPtr->displayedColor = *inColorPtr;
		if (inIsUserAction)
		{
			if (nullptr != dataPtr->notifyProcPtr) (*(dataPtr->notifyProcPtr))(inView, &dataPtr->displayedColor, dataPtr->contextPtr);
		}
		UNUSED_RETURN(OSStatus)HIViewSetNeedsDisplay(inView, true);
	}
}// SetColor


/*!
Assigns a notification procedure to a color box.  The
specified routine will automatically be called if the
color displayed by the box changes for any reason.
For modeless windows, it may be necessary to use a
notification routine, if you want to respond right away
to the user’s changes.

(3.0)
*/
void
ColorBox_SetColorChangeNotifyProc	(HIViewRef						inView,
									 ColorBox_ChangeNotifyProcPtr	inNotifyProcPtr,
									 void*							inContextPtr)
{
	OSStatus			error = noErr;
	MyColorBoxDataPtr	dataPtr = nullptr;
	UInt32				actualSize = 0;
	
	
	error = GetControlProperty(inView, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeColorBoxData,
								sizeof(dataPtr), &actualSize, &dataPtr);
	assert_noerr(error);
	assert(sizeof(dataPtr) == actualSize);
	
	if (nullptr != dataPtr)
	{
		dataPtr->notifyProcPtr = inNotifyProcPtr;
		dataPtr->contextPtr = inContextPtr;
	}
}// SetColorChangeNotifyProc


/*!
Displays a floating color panel, and arranges for the
color box to be updated asynchronously whenever the user
changes the color panel.

You typically invoke this routine in response to a view
click in a color box button.  For 99.99% of cases, you
will not need to do anything else in response to a view
click in a color box.  Note that you can use
ColorBox_GetColor() to determine the displayed color.

(3.0)
*/
void
ColorBox_UserSetColor	(HIViewRef		inView)
{
	// ah, nice, simple and sane on recent Mac OS X versions...
	CocoaBasic_ColorPanelSetTargetView(inView);
	CocoaBasic_ColorPanelDisplay();
}// UserSetColor


#pragma mark Internal Methods
namespace {

/*!
Embellishes "kEventControlDraw" of "kEventClassControl".

Invoked by Mac OS X whenever a color box button should
be redrawn.  Since this is assumed to occur on a regular
bevel button, this routine first calls through to the
parent to get the correct button look, and only adds a
color region on top of that.

(3.1)
*/
OSStatus
receiveColorBoxDraw	(EventHandlerCallRef	inHandlerCallRef,
					 EventRef				inEvent,
					 void*					inColorBoxDataPtr)
{
	OSStatus			result = eventNotHandledErr;
	MyColorBoxDataPtr	dataPtr = REINTERPRET_CAST(inColorBoxDataPtr, MyColorBoxDataPtr);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	
	// first call through to the parent handler to get the proper button appearance
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	if (noErr == result)
	{
		HIViewRef		view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (noErr == result)
		{
			//ControlPartCode		partCode = 0;
			CGContextRef		drawingPort = nullptr;
			
			
			// could determine which part (if any) to draw; if none, draw everything
			// (ignored, not needed)
			//result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			//result = noErr; // ignore part code parameter if absent
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef,
															typeCGContextRef, drawingPort);
			if (noErr == result)
			{
				HIRect		bounds;
				
				
				// determine boundaries of the content view being drawn
				HIViewGetBounds(view, &bounds);
				
				// define the area within the button where a color will be painted
				bounds = CGRectInset(bounds, +5.0, +5.0);
				
				// paint the color; make the colored area rounded to match the
				// bevel button, and draw a slightly brighter rounded border
				{
					CGContextSaveRestore	contextLock(drawingPort);
					float const				kCurveSize = 2.0; // width in pixels of circular corners of rectangle
					float					redFraction = dataPtr->displayedColor.red;
					float					greenFraction = dataPtr->displayedColor.green;
					float					blueFraction = dataPtr->displayedColor.blue;
					
					
					redFraction /= float(RGBCOLOR_INTENSITY_MAX);
					greenFraction /= float(RGBCOLOR_INTENSITY_MAX);
					blueFraction /= float(RGBCOLOR_INTENSITY_MAX);
					
					// background
					CGContextSetRGBFillColor(drawingPort, redFraction, greenFraction, blueFraction, 1.0/* alpha */);
					CGContextBeginPath(drawingPort);
					RegionUtilities_AddRoundedRectangleToPath(drawingPort, bounds, 2.0, 2.0);
					CGContextFillPath(drawingPort);
					
					// set border color
					if (kControlCheckBoxUncheckedValue == GetControl32BitValue(view))
					{
						// brighten the border a bit (is there a function for this?)
						redFraction *= 1.3;
						if (redFraction > 1.0) redFraction = 1.0;
						greenFraction *= 1.3;
						if (greenFraction > 1.0) greenFraction = 1.0;
						blueFraction *= 1.3;
						if (blueFraction > 1.0) blueFraction = 1.0;
					}
					else
					{
						// change border to indicate selected state
						redFraction = 0.0;
						greenFraction = 0.0;
						blueFraction = 0.0;
					}
					
					// border
					CGContextSetRGBStrokeColor(drawingPort, redFraction, greenFraction, blueFraction, 1.0/* alpha */);
					CGContextSetLineWidth(drawingPort, 0.7/* arbitrary */);
					CGContextBeginPath(drawingPort);
					RegionUtilities_AddRoundedRectangleToPath(drawingPort, bounds, kCurveSize, kCurveSize);
					CGContextStrokePath(drawingPort);
				}
			}
		}
	}
	return result;
}// receiveColorBoxDraw

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
