/*###############################################################

	SplashScreenDialog.cp
	
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
#include <QuickTime/ImageCompression.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <HIViewWrap.h>
#include <NIBLoader.h>

// MacTelnet includes
#include "AppResources.h"
#include "Embedding.h"
#include "EventLoop.h"
#include "SplashScreenDialog.h"
#include "UIStrings.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the ÒWindowÓ NIB from the package "SplashScreen.nib".
*/
HIViewID const	idMyImageSplashScreen		= { 'Pict', 0/* ID */ };

Float32 const	kMinimumSplashScreenAlpha = 0.0;				// between 0 and 1, where 1 is opaque
Float32 const	kMaximumSplashScreenAlpha = 1.0;				// between 0 and 1, where 1 is opaque
Float32 const	kSplashScreenFadeInAlphaIncreaseRate = 0.03;	// between 0 and kMaximumSplashScreenAlpha
																//   (for best results, should be multiple of max.)
Float32 const	kSplashScreenFadeOutAlphaDecreaseRate = 0.0167;	// between kMaximumSplashScreenAlpha and 0
																//   (for best results, should be multiple of max.)
SInt16 const	kSplashScreenBetweenFadeTicks = 120;			// minimum delay between fade-in and fade-out

} // anonymous namespace

#pragma mark Variables
namespace {

HIWindowRef				gSplashScreenWindow = nullptr;
CGImageRef				gSplashScreenPicture = nullptr;
EventLoopTimerUPP		gSplashScreenFadeInTimerUPP = nullptr; // procedure that adjusts the splash screenÕs alpha
EventLoopTimerUPP		gSplashScreenFadeOutTimerUPP = nullptr; // procedure that adjusts the splash screenÕs alpha
EventLoopTimerRef		gSplashScreenFadeInTimer = nullptr; // timer that fires frequently to make a fade effect
EventLoopTimerRef		gSplashScreenFadeOutTimer = nullptr; // timer that fires frequently to make a fade effect
CarbonEventHandlerWrap	gSplashScreenRenderer; // draws the picture and text overlay

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

pascal void			fadeInSplashScreenTimer		(EventLoopTimerRef, void*);
pascal void			fadeOutSplashScreenTimer	(EventLoopTimerRef, void*);
void				killSplashScreen			();
pascal OSStatus		receiveSplashScreenDraw		(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates the splash screen.  Use SplashScreenDialog_Display()
to show it.

(3.0)
*/
void
SplashScreenDialog_Init ()
{
	AppResources_Result		resourceError = noErr;
	
	
	resourceError = AppResources_GetSplashScreenPicture(gSplashScreenPicture);
	if ((noErr == resourceError) && (nullptr != gSplashScreenPicture))
	{
		NIBWindow	splashScreen(AppResources_ReturnBundleForNIBs(), CFSTR("SplashScreen"), CFSTR("Window"));
		
		
		assert(splashScreen.exists());
		if (splashScreen.exists())
		{
			gSplashScreenWindow = splashScreen;
			
			// set up the image view
			{
				HIViewWrap		splashScreenView(idMyImageSplashScreen, splashScreen);
				OSStatus		error = noErr;
				
				
				error = HIImageViewSetImage(splashScreenView, gSplashScreenPicture);
				assert_noerr(error);
				
				gSplashScreenRenderer.install(GetControlEventTarget(splashScreenView),
												receiveSplashScreenDraw,
												CarbonEventSetInClass(CarbonEventClass(kEventClassControl),
																		kEventControlDraw),
												nullptr/* user data */);
				assert(gSplashScreenRenderer.isInstalled());
			}
			
			// due to resize constraints, this will automatically size the image view inside the window
			SizeWindow(gSplashScreenWindow, CGImageGetWidth(gSplashScreenPicture), CGImageGetHeight(gSplashScreenPicture), true/* update */);
		}
	}
}// Init


/*!
Destroys the splash screen.  On Mac OS X, this starts
a fade-out effect that terminates asynchronously.

(3.0)
*/
void
SplashScreenDialog_Done ()
{
	if (nullptr != gSplashScreenWindow)
	{
		// on Mac OS X the splash screen is killed asynchronously,
		// after a beautiful fade-out effect makes the window invisible
		OSStatus	error = noErr;
		
		
		// install a timer to fade the splash screen out
		gSplashScreenFadeOutTimerUPP = NewEventLoopTimerUPP(fadeOutSplashScreenTimer);
		error = InstallEventLoopTimer(GetCurrentEventLoop(),
										kEventDurationNoWait/* seconds before timer fires the first time */,
										kEventDurationSecond / 60.0/* time between fires - 60 frames a second */,
										gSplashScreenFadeOutTimerUPP, nullptr/* user data - not used */,
										&gSplashScreenFadeOutTimer);
		if (noErr != error)
		{
			// in the event of a problem, just kill the window
			killSplashScreen();
		}
	}
	if (nullptr != gSplashScreenPicture)
	{
		CFRelease(gSplashScreenPicture), gSplashScreenPicture = nullptr;
	}
}// Done


/*!
Shows the splash screen.

(3.0)
*/
void
SplashScreenDialog_Display ()
{
	HIWindowRef		window = nullptr;
	
	
	window = gSplashScreenWindow;
	if (nullptr != window)
	{
		ShowWindow(window);
		// make window transparent initially so it is invisible;
		// it will be faded in later
		(OSStatus)SetWindowAlpha(gSplashScreenWindow, 0);
		EventLoop_SelectBehindDialogWindows(window);
		
	#if 1
		// display immediately
		(OSStatus)SetWindowAlpha(gSplashScreenWindow, 1.0);
	#else
		// install a timer to fade the splash screen in; NOTE that
		// timers only fire once the main event loop begins, so in
		// order for the splash screen to animate, the event loop
		// must start SOON in the application launch procedure
		gSplashScreenFadeInTimerUPP = NewEventLoopTimerUPP(fadeInSplashScreenTimer);
		if (noErr != InstallEventLoopTimer(GetCurrentEventLoop(),
											kEventDurationNoWait/* time before first fire */,
											kEventDurationSecond / 60.0/* time between fires - 60 frames a second */,
											gSplashScreenFadeInTimerUPP, nullptr/* user data - not used */,
											&gSplashScreenFadeInTimer)
		{
			gSplashScreenFadeInTimer = nullptr;
		}
	#endif
	}
}// Display


#pragma mark Internal Methods
namespace {

/*!
Adjusts the fade-in of the splash screen window.
Once complete, the timer will automatically remove
itself.

(3.0)
*/
pascal void
fadeInSplashScreenTimer		(EventLoopTimerRef	UNUSED_ARGUMENT(inTimer),
							 void*				UNUSED_ARGUMENT(inUnusedData))
{
	static Float32	alpha = 0.0; // static is OK because only one timer instance is expected
	
	
	alpha += kSplashScreenFadeInAlphaIncreaseRate;
	if (alpha > kMaximumSplashScreenAlpha)
	{
		// remove timer once the splash screen has faded out
		if (nullptr != gSplashScreenFadeInTimer)
		{
			RemoveEventLoopTimer(gSplashScreenFadeInTimer), gSplashScreenFadeInTimer = nullptr;
		}
		if (nullptr != gSplashScreenFadeInTimerUPP)
		{
			DisposeEventLoopTimerUPP(gSplashScreenFadeInTimerUPP), gSplashScreenFadeInTimerUPP = nullptr;
		}
	}
	else
	{
		// change the transparency of the window
		(OSStatus)SetWindowAlpha(gSplashScreenWindow, alpha);
	}
}// fadeInSplashScreenTimer


/*!
Adjusts the fade-out of the splash screen window.
Once complete, the timer will automatically remove
itself and the window will be disposed of.

(3.0)
*/
pascal void
fadeOutSplashScreenTimer	(EventLoopTimerRef	UNUSED_ARGUMENT(inTimer),
							 void*				UNUSED_ARGUMENT(inUnusedData))
{
	static Float32	alpha = kMaximumSplashScreenAlpha; // static is OK because only one timer instance is expected
	
	
	alpha -= kSplashScreenFadeOutAlphaDecreaseRate;
	if (alpha < kMinimumSplashScreenAlpha)
	{
		// remove timer once the splash screen has faded out
		RemoveEventLoopTimer(gSplashScreenFadeOutTimer), gSplashScreenFadeOutTimer = nullptr;
		DisposeEventLoopTimerUPP(gSplashScreenFadeOutTimerUPP), gSplashScreenFadeOutTimerUPP = nullptr;
		killSplashScreen();
	}
	else
	{
		// change the transparency of the window
		(OSStatus)SetWindowAlpha(gSplashScreenWindow, alpha);
	}
}// fadeOutSplashScreenTimer


/*!
Disposes of the splash screen window.

(3.0)
*/
void
killSplashScreen ()
{
	if (nullptr != gSplashScreenWindow)
	{
		DisposeWindow(gSplashScreenWindow), gSplashScreenWindow = nullptr;
	}
}// killSplashScreen


/*!
Embellishes "kEventControlDraw" of "kEventClassControl".

Invoked by Mac OS X whenever the picture should be drawn.
Superimposes text on top of the picture.

(3.1)
*/
pascal OSStatus
receiveSplashScreenDraw		(EventHandlerCallRef	inHandlerCallRef,
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	
	// first call through to the parent handler to get the image background
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	if (noErr == result)
	{
		HIViewRef		view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (noErr == result)
		{
			CGContextRef	drawingContext = nullptr;
			
			
			// determine the context to use for drawing
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, drawingContext);
			if (noErr == result)
			{
				CFStringRef		randomBilineCFString = nullptr;
				HIRect			bounds;
				
				
				// determine boundaries of the content view being drawn
				HIViewGetBounds(view, &bounds);
				
				// overlay a message string; note that the port boundaries are
				// inverted compared to normal window boundaries, so the text
				// boundaries are defined oddly to force text to the bottom
				if (UIStrings_CopyRandom(kUIStrings_StringClassSplashScreen, randomBilineCFString).ok())
				{
					HIThemeTextInfo		textInfo;
					HIRect				textBounds = CGRectMake(0/* x; arbitrary */, bounds.size.height - 34/* y; arbitrary */,
																bounds.size.width, 32/* height; arbitrary */);
					
					
					bzero(&textInfo, sizeof(textInfo));
					textInfo.version = 0;
					textInfo.state = kThemeStateActive;
					textInfo.fontID = kThemeMenuTitleFont; // arbitrary
					textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushCenter;
					textInfo.verticalFlushness = kHIThemeTextVerticalFlushCenter;
					textInfo.options = 0;
					textInfo.truncationPosition = kHIThemeTextTruncationNone;
					
				#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
					(OSStatus)HIThemeSetFill(kThemeTextColorWhite, nullptr/* info */, drawingContext,
												kHIThemeOrientationNormal);
				#endif
					(OSStatus)HIThemeDrawTextBox(randomBilineCFString, &textBounds, &textInfo,
													drawingContext, kHIThemeOrientationNormal);
					
					CFRelease(randomBilineCFString), randomBilineCFString = nullptr;
				}
				
				// completely handled
				result = noErr;
			}
		}
	}
	return result;
}// receiveSplashScreenDraw

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
