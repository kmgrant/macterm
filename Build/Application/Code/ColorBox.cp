/*###############################################################

	ColorBox.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <CGContextSaveRestore.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <FlagManager.h>
#include <RegionUtilities.h>

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "MenuResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "ColorBox.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Preferences.h"
#include "UIStrings.h"



#pragma mark Types

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
	RGBColor						displayedColor;		//!< which color (the ÒvalueÓ) the color box displays
	ColorBox_ChangeNotifyProcPtr	notifyProcPtr;		//!< routine, if any, to call when the value is changed
	void*							contextPtr;			//!< caller-defined context to pass to the notification routine
};
typedef MyColorBoxData*		MyColorBoxDataPtr;

#pragma mark Internal Method Prototypes

static pascal OSStatus		receiveColorBoxDraw		(EventHandlerCallRef, EventRef, void*);



#pragma mark Public Methods

/*!
Assigns a bevel button view the drawing embellishment for
a color box, and attaches data to it using a property.

Be sure to invoke ColorBox_DetachFromBevelButton() when
you are finished with the color box, to dispose of data
allocated by invoking this routine.

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
		}
	}
	catch (std::bad_alloc)
	{
		result = memFullErr;
	}
	
	return result;
}// AttachToBevelButton


/*!
Disposes of a bevel buttonÕs drawing routine, initially
installed using one of the ColorBox_AttachTo...() routines.

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

(3.0)
*/
void
ColorBox_SetColor	(HIViewRef			inView,
					 RGBColor const*	inColorPtr)
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
		if (nullptr != dataPtr->notifyProcPtr) (*(dataPtr->notifyProcPtr))(inView, &dataPtr->displayedColor, dataPtr->contextPtr);
		(OSStatus)HIViewSetNeedsDisplay(inView, true);
	}
}// SetColor


/*!
Assigns a notification procedure to a color box.  The
specified routine will automatically be called if the
color displayed by the box changes for any reason.
For modeless windows, it may be necessary to use a
notification routine, if you want to respond right away
to the userÕs changes.

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
Displays a standard Color Picker dialog box, asking the
user to choose a color.  If the user accepts the color,
this routine automatically updates the displayed color
of the color box and "true" is returned; otherwise,
"false" is returned.

You typically invoke this routine in response to a view
click in a color box button.  For 99.99% of cases, you
will not need to do anything else in response to a view
click in a color box.  Note that you can use
ColorBox_GetColor() to determine the displayed color.

(3.0)
*/
Boolean
ColorBox_UserSetColor	(HIViewRef		inView)
{
	OSStatus			error = noErr;
	MyColorBoxDataPtr	dataPtr = nullptr;
	UInt32				actualSize = 0;
	Boolean				result = false;
	
	
	error = GetControlProperty(inView, AppResources_ReturnCreatorCode(),
								kConstantsRegistry_ControlPropertyTypeColorBoxData,
								sizeof(dataPtr), &actualSize, &dataPtr);
	assert_noerr(error);
	assert(sizeof(dataPtr) == actualSize);
	
	if (nullptr != dataPtr)
	{
		UIStrings_Result		stringResult = kUIStrings_ResultOK;
		CFStringRef				askColorCFString = nullptr;
		PickerMenuItemInfo		editMenuInfo;
		Boolean					releaseAskColorCFString = true;
		
		
		stringResult = UIStrings_Copy(kUIStrings_SystemDialogPromptPickColor, askColorCFString);
		if (false == stringResult.ok())
		{
			// cannot find prompt, but this is not a serious problem
			askColorCFString = CFSTR("");
			releaseAskColorCFString = false;
		}
		
		DeactivateFrontmostWindow();
		result = ColorUtilities_ColorChooserDialogDisplay
					(askColorCFString, &dataPtr->displayedColor/* input */, &dataPtr->displayedColor/* output */,
						true/* is modal */, NewUserEventUPP(EventLoop_HandleColorPickerUpdate),
						&editMenuInfo);
		RestoreFrontmostWindow();
		
		if (releaseAskColorCFString)
		{
			CFRelease(askColorCFString), askColorCFString = nullptr;
		}
		
		if ((result) && (nullptr != dataPtr->notifyProcPtr))
		{
			(*(dataPtr->notifyProcPtr))(inView, &dataPtr->displayedColor, dataPtr->contextPtr);
			(OSStatus)HIViewSetNeedsDisplay(inView, true);
		}
	}
	return result;
}// UserSetColor


#pragma mark Internal Methods

/*!
Embellishes "kEventControlDraw" of "kEventClassControl".

Invoked by Mac OS X whenever a color box button should
be redrawn.  Since this is assumed to occur on a regular
bevel button, this routine first calls through to the
parent to get the correct button look, and only adds a
color region on top of that.

(3.1)
*/
static pascal OSStatus
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
					
					// brighten the border a bit (is there a function for this?)
					redFraction *= 1.3;
					if (redFraction > 1.0) redFraction = 1.0;
					greenFraction *= 1.3;
					if (greenFraction > 1.0) greenFraction = 1.0;
					blueFraction *= 1.3;
					if (blueFraction > 1.0) blueFraction = 1.0;
					
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

// BELOW IS REQUIRED NEWLINE TO END FILE
