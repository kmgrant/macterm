/*###############################################################

	SplashScreenDialog.cp
	
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <QuickTime/ImageCompression.h>

// library includes
#include <Console.h>

// MacTelnet includes
#include "AppResources.h"
#include "Embedding.h"
#include "EventLoop.h"
#include "SplashScreenDialog.h"
#include "UIStrings.h"



#pragma mark Constants

Float32 const	kMinimumSplashScreenAlpha = 0.0;				// between 0 and 1, where 1 is opaque
Float32 const	kMaximumSplashScreenAlpha = 1.0;				// between 0 and 1, where 1 is opaque
Float32 const	kSplashScreenFadeInAlphaIncreaseRate = 0.03;	// between 0 and kMaximumSplashScreenAlpha
																//   (for best results, should be multiple of max.)
Float32 const	kSplashScreenFadeOutAlphaDecreaseRate = 0.0167;	// between kMaximumSplashScreenAlpha and 0
																//   (for best results, should be multiple of max.)
SInt16 const	kSplashScreenBetweenFadeTicks = 120;			// minimum delay between fade-in and fade-out

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowRef			gSplashScreenWindow = nullptr;
	PicHandle			gSplashScreenPicture = nullptr;
	EventLoopTimerUPP	gSplashScreenFadeInTimerUPP = nullptr; // procedure that adjusts the splash screenÕs alpha
	EventLoopTimerUPP	gSplashScreenFadeOutTimerUPP = nullptr; // procedure that adjusts the splash screenÕs alpha
	EventLoopTimerRef	gSplashScreenFadeInTimer = nullptr; // timer that fires frequently to make a fade effect
	EventLoopTimerRef	gSplashScreenFadeOutTimer = nullptr; // timer that fires frequently to make a fade effect
}

#pragma mark Internal Method Prototypes

static void				drawSplashScreen			(WindowRef);
static pascal void		fadeInSplashScreenTimer		(EventLoopTimerRef, void*);
static pascal void		fadeOutSplashScreenTimer	(EventLoopTimerRef, void*);
static void				killSplashScreen			();



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
	if ((resourceError == noErr) && (gSplashScreenPicture != nullptr))
	{
		Rect		pictureFrame;
		OSStatus	error = noErr;
		
		
		(Rect*)QDGetPictureBounds(gSplashScreenPicture, &pictureFrame);
		
		// create an overlay window exactly the same size as the picture
		error = CreateNewWindow(kOverlayWindowClass,
								kWindowNoActivatesAttribute |		// do not send activate/deactivate events
								kWindowNoConstrainAttribute/* |		// do not reposition when Dock moves, etc.
								kWindowHideOnSuspendAttribute*/,	// hide when MacTelnet is not frontmost
								&pictureFrame, &gSplashScreenWindow);
		if (error == noErr)
		{
			// on Mac OS X 10.2 and later, the Òno clicksÓ attribute is available;
			// use it, because it is annoying to have the fading-out splash screen
			// get in the way of clicking on things
			(OSStatus)ChangeWindowAttributes(gSplashScreenWindow,
												kWindowIgnoreClicksAttribute/* set these attributes */,
												kWindowNoShadowAttribute/* clear these attributes */);
			
			// center window
			error = RepositionWindow(gSplashScreenWindow, EventLoop_ReturnRealFrontWindow(),
										kWindowAlertPositionOnMainScreen);
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
	if (gSplashScreenWindow != nullptr)
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
		if (error != noErr)
		{
			// in the event of a problem, just kill the window
			killSplashScreen();
		}
	}
	if (gSplashScreenPicture != nullptr)
	{
		KillPicture(gSplashScreenPicture);
	}
}// Done


/*!
Shows the splash screen.

(3.0)
*/
void
SplashScreenDialog_Display ()
{
	WindowRef	window = nullptr;
	
	
	window = gSplashScreenWindow;
	if (window != nullptr)
	{
		ShowWindow(window);
		drawSplashScreen(window);
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

/*!
Draws the contents of the splash screen window.

(3.0)
*/
static void
drawSplashScreen	(WindowRef		UNUSED_ARGUMENT_CARBON(inSplashScreenWindow))
{
	// fade in and out the splash screen
	CGContextRef	graphicsContext = nullptr;
	Rect			windowBounds;
	
	
	GetPortBounds(GetWindowPort(gSplashScreenWindow), &windowBounds);
	if (CreateCGContextForPort(GetWindowPort(gSplashScreenWindow), &graphicsContext) == noErr)
	{
		CGRect		box;
		CGImageRef	image = nullptr;
		PicHandle	maskPicture = nullptr;
		RGBColor	blackRGB;
		
		
		// use a special routine that combines two QuickDraw pictures
		// into a Core Graphics image; since in this case no transparency
		// is desired, the 2nd picture is just a field of black; when
		// mixed, the results are a completely opaque QuickDraw picture
		box.origin.x = windowBounds.left;
		box.origin.y = windowBounds.top;
		box.size.width = windowBounds.right - windowBounds.left;
		box.size.height = windowBounds.bottom - windowBounds.top;
		{
			maskPicture = OpenPicture(&windowBounds);
			if (maskPicture != nullptr)
			{
				blackRGB.red = blackRGB.green = blackRGB.blue = 0;
				RGBBackColor(&blackRGB);
				EraseRect(&windowBounds);
				ClosePicture();
				CGContextClearRect(graphicsContext, box);
				if (Embedding_BuildCGImageFromPictureAndMask(gSplashScreenPicture, maskPicture, &image) == noErr)
				{
					CGContextDrawImage(graphicsContext, box, image);
					CGImageRelease(image);
					
					// now overlay a message string; note that the port boundaries are
					// inverted compared to normal window boundaries, so the text
					// boundaries are defined oddly to force text to the bottom
					{
						CFStringRef			randomBilineCFString = nullptr;
						
						
						if (UIStrings_CopyRandom(kUIStrings_StringClassSplashScreen, randomBilineCFString).ok())
						{
							HIThemeTextInfo		textInfo;
							HIRect				textBounds = CGRectMake(0/* x; arbitrary */, 0/* y; arbitrary */,
																		windowBounds.right - windowBounds.left/* width */,
																		40/* height; arbitrary */);
							
							
							bzero(&textInfo, sizeof(textInfo));
							textInfo.version = 0;
							textInfo.state = kThemeStateActive;
							textInfo.fontID = kThemeAlertHeaderFont;
							textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushCenter;
							textInfo.verticalFlushness = kHIThemeTextVerticalFlushCenter;
							textInfo.options = 0;
							textInfo.truncationPosition = kHIThemeTextTruncationNone;
							
						#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
							(OSStatus)HIThemeSetFill(kThemeTextColorWhite, nullptr/* info */, graphicsContext,
														kHIThemeOrientationInverted);
						#endif
							(OSStatus)HIThemeDrawTextBox(randomBilineCFString, &textBounds, &textInfo,
															graphicsContext, kHIThemeOrientationInverted);
							
							CFRelease(randomBilineCFString), randomBilineCFString = nullptr;
						}
					}
				}
				KillPicture(maskPicture), maskPicture = nullptr;
			}
		}
		
		// make window transparent initially so it is invisible;
		// it will be faded in later
		(OSStatus)SetWindowAlpha(gSplashScreenWindow, 0);
	}
	
	CGContextFlush(graphicsContext);
	CGContextRelease(graphicsContext), graphicsContext = nullptr;
}// drawSplashScreen


/*!
Adjusts the fade-in of the splash screen window.
Once complete, the timer will automatically remove
itself.

(3.0)
*/
static pascal void
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
static pascal void
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
static void
killSplashScreen ()
{
	if (gSplashScreenWindow != nullptr)
	{
		DisposeWindow(gSplashScreenWindow), gSplashScreenWindow = nullptr;
	}
}// killSplashScreen

// BELOW IS REQUIRED NEWLINE TO END FILE
