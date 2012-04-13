/*!	\file CocoaBasic.mm
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.9
	© 2008-2012 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#import <CocoaBasic.h>

// standard-C++ includes
#import <map>

// Mac includes
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <AutoPool.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <HIViewWrap.h>
#import <Popover.objc++.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "ColorBox.h"
#import "FileUtilities.h"



#pragma mark Types

@interface CocoaBasic_NoticeColorPanelChange : NSResponder

- (void)
changeColor:(id)_;

@end

typedef std::map< HIWindowRef, NSWindow* >		HIWindowRefToNSWindowMap;

#pragma mark Internal Method prototypes
namespace {

NSString*	findFolder			(short, OSType);
NSString*	returnPathForFSRef	(FSRef const&);

} // anonymous namespace

#pragma mark Variables
namespace {

HIWindowRefToNSWindowMap&			gCocoaCarbonWindows()	{ static HIWindowRefToNSWindowMap x; return x; }
HIViewWrap							gCurrentColorPanelFocus;	//!< see ColorBox.h; a view with a color box that uses the current color
CocoaBasic_NoticeColorPanelChange*	gColorWatcher = nil;		//!< sees color changes
NSSpeechSynthesizer*				gDefaultSynth = nil;

} // anonymous namespace



#pragma mark Public Methods

/*!
Shows the standard About panel, or brings it to the front.

(1.0)
*/
void
CocoaBasic_AboutPanelDisplay ()
{
	AutoPool		_;
	// clear out the version field because it tends to be redundant
	NSDictionary*	aboutBoxOptions = [NSDictionary dictionaryWithObjectsAndKeys:
										@"", @"Version",
										nil];
	
	
	[NSApp orderFrontStandardAboutPanelWithOptions:aboutBoxOptions];
}// AboutPanelDisplay


/*!
Configures a popover to have a dark blue background and a
thick white border.  Popovers of this type should contain
primarily white text; they do not usually contain any other
views.

If "inHasArrow" is false, various properties are tweaked so
the popover has no arrow in its frame and so it will not
take up any extra space for an arrow.

(1.9)
*/
void
CocoaBasic_ApplyBlueStyleToPopover	(Popover_Window*	inoutPopover,
									 Boolean			inHasArrow)
{
	[inoutPopover setBackgroundColor:[NSColor colorWithDeviceRed:0 green:0.25 blue:0.5 alpha:0.93]];
	[inoutPopover setBorderOuterColor:[NSColor whiteColor]];
	[inoutPopover setBorderPrimaryColor:[NSColor blackColor]];
	[inoutPopover setViewMargin:5.0];
	[inoutPopover setBorderWidth:5.0];
	[inoutPopover setCornerRadius:5.0];
	if (inHasArrow)
	{
		[inoutPopover setHasArrow:YES];
		[inoutPopover setHasRoundCornerBesideArrow:NO];
		[inoutPopover setArrowBaseWidth:30.0];
		[inoutPopover setArrowHeight:15.0];
	}
	else
	{
		[inoutPopover setHasArrow:NO];
		[inoutPopover setHasRoundCornerBesideArrow:YES];
		[inoutPopover setArrowBaseWidth:0.0];
		[inoutPopover setArrowHeight:0.0];
	}
}// ApplyBlueStyleToPopover


/*!
Configures a popover to have the appearance that the vast
majority of popovers are expected to have.  Any view should
look good in this type of popover, and the standard system
text color (that is, black) should be used.

If "inHasArrow" is false, various properties are tweaked so
the popover has no arrow in its frame and so it will not
take up any extra space for an arrow.

(1.9)
*/
void
CocoaBasic_ApplyStandardStyleToPopover	(Popover_Window*	inoutPopover,
										 Boolean			inHasArrow)
{
	[inoutPopover setBackgroundColor:[NSColor colorWithDeviceRed:0.9 green:0.9 blue:0.9 alpha:0.95]];
	[inoutPopover setBorderOuterColor:[NSColor colorWithDeviceRed:0.25 green:0.25 blue:0.25 alpha:0.7]];
	[inoutPopover setBorderPrimaryColor:[NSColor colorWithDeviceRed:1.0 green:1.0 blue:1.0 alpha:0.8]];
	[inoutPopover setViewMargin:3.0];
	[inoutPopover setBorderWidth:2.2];
	[inoutPopover setCornerRadius:4.0];
	if (inHasArrow)
	{
		[inoutPopover setHasArrow:YES];
		[inoutPopover setHasRoundCornerBesideArrow:NO];
		[inoutPopover setArrowBaseWidth:30.0];
		[inoutPopover setArrowHeight:15.0];
	}
	else
	{
		[inoutPopover setHasArrow:NO];
		[inoutPopover setHasRoundCornerBesideArrow:YES];
		[inoutPopover setArrowBaseWidth:0.0];
		[inoutPopover setArrowHeight:0.0];
	}
}// ApplyStandardStyleToPopover


/*!
Shows the global color panel floating window, if it is
not already visible.

(1.0)
*/
void
CocoaBasic_ColorPanelDisplay ()
{
	AutoPool	_;
	
	
	[NSApp orderFrontColorPanel:NSApp];
}// ColorPanelDisplay


/*!
Specifies the view, which must be a color box, that is
the current target of the global color panel.  Also
initializes the global color panel color to whatever
ColorBox_GetColor() returns.

(1.0)
*/
void
CocoaBasic_ColorPanelSetTargetView	(HIViewRef	inColorBoxView)
{
	AutoPool		_;
	NSColorPanel*	colorPanel = [NSColorPanel sharedColorPanel];
	NSColor*		globalColor = nil;
	RGBColor		viewColor;
	CGDeviceColor	viewColorFloat;
	
	
	// create a responder if none exists, to watch for color changes
	if (nil == gColorWatcher)
	{
		gColorWatcher = [[CocoaBasic_NoticeColorPanelChange alloc] init];
		[gColorWatcher setNextResponder:[NSApp nextResponder]];
		[NSApp setNextResponder:gColorWatcher];
	}
	
	// remove highlighting from any previous focus
	if (gCurrentColorPanelFocus.exists()) SetControl32BitValue(gCurrentColorPanelFocus, kControlCheckBoxUncheckedValue);
	
	// set the global to the new target view
	gCurrentColorPanelFocus.setCFTypeRef(inColorBoxView);
	
	// highlight the new focus
	SetControl32BitValue(gCurrentColorPanelFocus, kControlCheckBoxCheckedValue);
	
	// initialize the color in the panel
	ColorBox_GetColor(inColorBoxView, &viewColor);
	viewColorFloat.red = viewColor.red;
	viewColorFloat.red /= RGBCOLOR_INTENSITY_MAX;
	viewColorFloat.green = viewColor.green;
	viewColorFloat.green /= RGBCOLOR_INTENSITY_MAX;
	viewColorFloat.blue = viewColor.blue;
	viewColorFloat.blue /= RGBCOLOR_INTENSITY_MAX;
	globalColor = [NSColor colorWithDeviceRed:viewColorFloat.red green:viewColorFloat.green blue:viewColorFloat.blue alpha:1.0];
	[colorPanel setColor:globalColor];
}// ColorPanelSetTargetView


/*!
Shows a modal panel for opening any number of files and
automatically handling them via Apple Events.  The panel
resolves aliases, does not allow the user to choose
directories, and allows the user to try opening any type
of file (despite the list).

All of the arguments can be "nullptr" to choose defaults.
The defaults are: no message, no title, and allowing all
possible file types.

The file type list is sophisticated and can accept
anything you would imagine: file name extensions, HFS
four-character codes, and reverse-domain-style UTIs.

Returns true only if the user tried to open something.

(1.0)
*/
Boolean
CocoaBasic_FileOpenPanelDisplay		(CFStringRef	inMessage,
									 CFStringRef	inWindowTitle,
									 CFArrayRef		inAllowedFileTypes)
{
	AutoPool		_;
	NSOpenPanel*	thePanel = [NSOpenPanel openPanel];
	int				buttonHit = NSCancelButton;
	Boolean			result = false;
	
	
	if (nullptr != inMessage)
	{
		[thePanel setMessage:(NSString*)inMessage];
	}
	if (nullptr != inWindowTitle)
	{
		[thePanel setTitle:(NSString*)inWindowTitle];
	}
	[thePanel setCanChooseDirectories:NO];
	[thePanel setCanChooseFiles:YES];
	[thePanel setAllowsOtherFileTypes:YES];
	[thePanel setAllowsMultipleSelection:YES];
	[thePanel setResolvesAliases:YES];
	buttonHit = [thePanel runModalForTypes:(NSArray*)inAllowedFileTypes];
	result = (NSOKButton == buttonHit);
	if (result)
	{
		NSArray*		toOpen = [thePanel URLs];
		NSEnumerator*	forURLs = [toOpen objectEnumerator];
		id				currentFile = nil;
		
		
		while (nil != (currentFile = [forURLs nextObject]))
		{
			NSURL*		currentFileURL = (NSURL*)currentFile;
			FSRef		fileRef;
			OSStatus	error = noErr;
			
			
			if (CFURLGetFSRef((CFURLRef)currentFileURL, &fileRef))
			{
				error = FileUtilities_OpenDocument(fileRef);
			}
			else
			{
				error = kURLInvalidURLError;
			}
			
			if (noErr != error)
			{
				Sound_StandardAlert();
				Console_WriteValueCFString("unable to open file", (CFStringRef)[currentFileURL absoluteString]);
				Console_WriteValue("error", error);
			}
		}
	}
	
	return result;
}// FileOpenPanelDisplay


/*!
This one-color variation uses white as one of the colors.

See also the two-color version.

(1.2)
*/
CGDeviceColor
CocoaBasic_GetGray	(CGDeviceColor const&	inColor,
					 Float32				inFraction)
{
	AutoPool		_;
	CGDeviceColor	result = inColor;
	NSColor*		c1 = [NSColor colorWithDeviceRed:inColor.red green:inColor.green blue:inColor.blue alpha:1.0];
	NSColor*		c2 = [NSColor whiteColor];
	NSColor*		blended = [c1 blendedColorWithFraction:inFraction ofColor:c2];
	
	
	if (blended)
	{
		result.red = [blended redComponent];
		result.green = [blended greenComponent];
		result.blue = [blended blueComponent];
	}
	return result;
}// GetGray


/*!
Emulates QuickDraw’s GetGray() by using the NSColor method
"blendedColorWithFraction:ofColor:".

See also the one-color version, which implicitly uses white.

(1.2)
*/
CGDeviceColor
CocoaBasic_GetGray	(CGDeviceColor const&	inColor1,
					 CGDeviceColor const&	inColor2,
					 Float32				inFraction)
{
	AutoPool		_;
	CGDeviceColor	result = inColor1;
	NSColor*		c1 = [NSColor colorWithDeviceRed:inColor1.red green:inColor1.green blue:inColor1.blue alpha:1.0];
	NSColor*		c2 = [NSColor colorWithDeviceRed:inColor2.red green:inColor2.green blue:inColor2.blue alpha:1.0];
	NSColor*		blended = [c1 blendedColorWithFraction:inFraction ofColor:c2];
	
	
	if (blended)
	{
		result.red = [blended redComponent];
		result.green = [blended greenComponent];
		result.blue = [blended blueComponent];
	}
	return result;
}// GetGray


/*!
On Mac OS X 10.7 and beyond, calls "invalidateRestorableState"
on NSApp.  On earlier versions of the OS, has no effect.

This is used to tell the OS when something has changed that is
normally saved and restored by “Quit and Keep Windows”.

See also CocoaBasic_InvalidateRestorableState().

(1.6)
*/
void
CocoaBasic_InvalidateAppRestorableState ()
{
	CocoaBasic_InvalidateRestorableState(NSApp);
}// InvalidateAppRestorableState


/*!
On Mac OS X 10.7 and beyond, calls "invalidateRestorableState"
on the specified responder (e.g. NSApp, or any NSWindow).  On
earlier versions of the OS, has no effect.

This is used to tell the OS when something has changed that is
normally saved and restored by “Quit and Keep Windows”.

See also CocoaBasic_InvalidateAppRestorableState(), which is a
short-cut for passing "NSApp" as the responder (useful mostly
in code that is not yet Cocoa-based).

(1.6)
*/
void
CocoaBasic_InvalidateRestorableState	(NSResponder*	inResponder)
{
	AutoPool	_;
	
	
	if ([inResponder respondsToSelector:@selector(invalidateRestorableState)])
	{
		[inResponder invalidateRestorableState];
	}
}// InvalidateRestorableState


/*!
Forces the Cocoa runtime to consider the Carbon user focus
window (as returned by GetUserFocusWindow()) to be the
frontmost, key window.

This can be important any time a window is selected using
Carbon APIs instead of user input; the Cocoa runtime may
correctly highlight the new window but not order it in
front.  So call SelectWindow(), and then this routine.

See also CocoaBasic_MakeKeyWindowCarbonUserFocusWindow().

(1.1)
*/
void
CocoaBasic_MakeFrontWindowCarbonUserFocusWindow ()
{
	AutoPool		_;
	HIWindowRef		carbonWindow = GetUserFocusWindow();
	
	
	if (nullptr != carbonWindow)
	{
		NSWindow*		window = [[NSWindow alloc] initWithWindowRef:carbonWindow];
		
		
		// as recommended in the documentation, retain the given window
		// manually, because initWithWindowRef: does not retain it (but
		// does release it)
		RetainWindow(carbonWindow);
		
		[window makeKeyAndOrderFront:nil];
	}
}// MakeFrontWindowCarbonUserFocusWindow


/*!
Forces the Cocoa runtime to consider the Carbon user focus
window (as returned by GetUserFocusWindow()) to be key.

This is mostly important for Carbon sheets that spawn from
Carbon windows that are running in a Cocoa runtime; the
sheets do not acquire focus automatically.  So call
ShowSheetWindow(), and then this routine.

See also CocoaBasic_MakeFrontWindowCarbonUserFocusWindow().

(1.1)
*/
void
CocoaBasic_MakeKeyWindowCarbonUserFocusWindow ()
{
	AutoPool		_;
	HIWindowRef		carbonWindow = GetUserFocusWindow();
	
	
	if (nullptr != carbonWindow)
	{
		NSWindow*		window = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(carbonWindow);
		
		
		// as recommended in the documentation, retain the given window
		// manually, because initWithWindowRef: does not retain it (but
		// does release it)
		RetainWindow(carbonWindow);
		
		[window makeKeyWindow];
	}
}// MakeKeyWindowCarbonUserFocusWindow


/*!
Plays the sound with the given name (no extension), if it
is found anywhere in the standard search paths like
/System/Library/Sounds, etc.

See also CocoaBasic_ReturnUserSoundNames().

(1.0)
*/
void
CocoaBasic_PlaySoundByName	(CFStringRef	inName)
{
	AutoPool	_;
	
	
	[(NSSound*)[NSSound soundNamed:(NSString*)inName] stop];
	[(NSSound*)[NSSound soundNamed:(NSString*)inName] play];
}// PlaySoundByName


/*!
Plays the sound in the specified file (asynchronously).

(1.0)
*/
void
CocoaBasic_PlaySoundFile	(CFURLRef	inFile)
{
	AutoPool	_;
	
	
	[[[[NSSound alloc] initWithContentsOfURL:(NSURL*)inFile byReference:NO] autorelease] play];
}// PlaySoundFile


/*!
For an existing window suspected to be a Carbon window (e.g. as
obtained from [NSApp keyWindow]), registers its Carbon window
reference for later use, so that another will not be allocated.
Returns true only if successfully registered.

(1.5)
*/
Boolean
CocoaBasic_RegisterCocoaCarbonWindow	(NSWindow*		inCocoaWindow)
{
	AutoPool	_;
	Boolean		result = false;
	
	
	if (nullptr != [inCocoaWindow windowRef])
	{
		gCocoaCarbonWindows().insert(std::make_pair(REINTERPRET_CAST([inCocoaWindow windowRef], HIWindowRef), inCocoaWindow));
		result = true;
	}
	return result;
}// RegisterCocoaCarbonWindow


/*!
Using the registry maintained by this module, returns the only
known NSWindow* for the given Carbon window reference.  If none
existed, "initWithWindowRef:" is used to make one.  This should
always be used instead of calling "initWithWindowRef:" yourself!

This calls CocoaBasic_RegisterCocoaCarbonWindow() automatically
when a new window is created.

(1.5)
*/
NSWindow*
CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow		(HIWindowRef	inCarbonWindow)
{
	AutoPool	_;
	NSWindow*	result = nil;
	
	
	if (nullptr != inCarbonWindow)
	{
		HIWindowRefToNSWindowMap::const_iterator	toPair = gCocoaCarbonWindows().find(inCarbonWindow);
		
		
		if (toPair != gCocoaCarbonWindows().end())
		{
			result = toPair->second;
		}
		
		if (nil == result)
		{
			result = [[NSWindow alloc] initWithWindowRef:inCarbonWindow];
			
			// as recommended in the documentation, retain the given window
			// manually, because initWithWindowRef: does not retain it (but
			// does release it)
			RetainWindow(inCarbonWindow);
			
			if (nil != [result windowRef])
			{
				(Boolean)CocoaBasic_RegisterCocoaCarbonWindow(result);
			}
		}
	}
	return result;
}// ReturnNewOrExistingCocoaCarbonWindow


/*!
Since Cocoa has a routine for finding the *localized* name of a
string encoding, it is exposed as a general API here.

The returned string is not retained, so do not release it.

(1.0)
*/
CFStringRef
CocoaBasic_ReturnStringEncodingLocalizedName	(CFStringEncoding	inEncoding)
{
	AutoPool			_;
	NSStringEncoding	translatedEncoding = CFStringConvertEncodingToNSStringEncoding(inEncoding);
	CFStringRef			result = nullptr;
	
	
	result = (CFStringRef)[NSString localizedNameOfStringEncoding:translatedEncoding];
	return result;
}// ReturnStringEncodingLocalizedName


/*!
Returns an unsorted list of names (without extensions) that are
valid sounds to pass to CocoaBasic_PlaySoundByName().  This is
useful for displaying a set of choices, such as a menu, to the
user.

(1.0)
*/
CFArrayRef
CocoaBasic_ReturnUserSoundNames ()
{
	AutoPool				_;
	NSArray* const			kSearchPaths = [NSArray arrayWithObjects:
												findFolder(kUserDomain, kSystemSoundsFolderType),
												findFolder(kLocalDomain, kSystemSoundsFolderType),
												findFolder(kSystemDomain, kSystemSoundsFolderType),
												nil];
	NSArray* const			kSupportedFileTypes = [NSSound soundUnfilteredFileTypes];
	NSFileManager* const	kFileManager = [NSFileManager defaultManager];
	NSEnumerator*			toSearchPath = [kSearchPaths objectEnumerator];
	BOOL					isDirectory = NO;
	NSMutableArray*			result = [NSMutableArray array];
	
	
	while (NSString* soundDirectory = [toSearchPath nextObject])
	{
		if ([kFileManager fileExistsAtPath:soundDirectory isDirectory:&isDirectory])
		{
			if (isDirectory)
			{
				NSEnumerator*	toFile = [[kFileManager directoryContentsAtPath:soundDirectory] objectEnumerator];
				
				
				while (NSString* soundFile = [toFile nextObject])
				{
					if ([kSupportedFileTypes containsObject:[soundFile pathExtension]])
					{
						[result addObject:[soundFile stringByDeletingPathExtension]];
					}
				}
			}
		}
	}
	
	[result retain];
	return REINTERPRET_CAST(result, CFArrayRef);
}// ReturnUserSoundNames


/*!
Adds a caution icon overlay to the main application icon
in the Dock.

See also CocoaBasic_SetDockTileToDefaultAppIcon().

(1.4)
*/
void
CocoaBasic_SetDockTileToCautionOverlay ()
{
	static BOOL		gHaveDisplayed = NO;
	
	
	// TEMPORARY; on certain versions of Mac OS X, it seems that this can
	// crash in the middle of Cocoa the *second* time it is called; for
	// now just avoid the problem, as this is low-priority code
	if (NO == gHaveDisplayed)
	{
		AutoPool		_;
		NSImage*		appIconImage = [[NSImage imageNamed:(NSString*)AppResources_ReturnBundleIconFilenameNoExtension()] copy];
		NSImage*		overlayImage = [NSImage imageNamed:(NSString*)AppResources_ReturnCautionIconFilenameNoExtension()];
		
		
		// the image location is somewhat arbitrary, and should probably be made configurable; TEMPORARY
		[appIconImage lockFocus];
		[overlayImage drawAtPoint:NSMakePoint(56, 16) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
		[appIconImage unlockFocus];
		[NSApp setApplicationIconImage:appIconImage];
		[appIconImage release];
		
		gHaveDisplayed = YES;
	}
}// SetDockTileToCautionOverlay


/*!
Returns the Dock icon back to a plain application icon.

(1.4)
*/
void
CocoaBasic_SetDockTileToDefaultAppIcon ()
{
	AutoPool	_;
	
	
	[NSApp setApplicationIconImage:nil];
}// SetDockTileToDefaultAppIcon


/*!
Uses Cocoa NSFileManager APIs to set the HFS file type and
creator codes for the file at the given path.  Returns true
if both were set successfully.

(1.4)
*/
Boolean
CocoaBasic_SetFileTypeCreator	(CFStringRef	inPath,
								 OSType			inNewType,
								 OSType			inNewCreator)
{
	AutoPool		_;
	Boolean			result = false;
	NSDictionary*	attributeDict = [NSDictionary dictionaryWithObjectsAndKeys:
																				[NSNumber numberWithUnsignedLong:inNewType],
																				NSFileHFSTypeCode,
																				[NSNumber numberWithUnsignedLong:inNewCreator],
																				NSFileHFSCreatorCode,
																				nil];
	
	
	if ([[NSFileManager defaultManager] changeFileAttributes:attributeDict atPath:(NSString*)inPath])
	{
		result = true;
	}
	
	return result;
}// SetFileTypeCreator


/*!
Returns true if a computer voice is currently speaking.

(1.3)
*/
Boolean
CocoaBasic_SpeakingInProgress ()
{
	AutoPool	_;
	
	
	return ([gDefaultSynth isSpeaking]) ? true : false;
}// SpeakingInProgress


/*!
Passes the given string to the default speech synthesizer.

(1.3)
*/
Boolean
CocoaBasic_StartSpeakingString	(CFStringRef	inCFString)
{
	AutoPool	_;
	BOOL		speakOK;
	
	
	if (nil == gDefaultSynth)
	{
		gDefaultSynth = [[NSSpeechSynthesizer alloc] initWithVoice:nil];
	}
	speakOK = ([gDefaultSynth startSpeakingString:(NSString*)inCFString]) ? true : false;
	
	return (speakOK) ? true : false;
}// StartSpeakingString


/*!
Interrupts any current speech synthesis, and not
necessarily gracefully.

(1.3)
*/
void
CocoaBasic_StopSpeaking ()
{
	AutoPool	_;
	
	
	[gDefaultSynth stopSpeaking];
}// StopSpeaking


#pragma mark Internal Methods
namespace {

/*!
Returns the path of the specified folder.  The domain is
one that is valid for FSFindFolder, e.g. kUserDomain or
kSystemDomain or a volume.

The result is always defined, which is convenient for
such things as initializing arrays.  Though it may be an
empty string (if there was a problem) or a path to a
folder that does not exist yet.

(1.0)
*/
NSString*
findFolder	(short		inDomain,
			 OSType		inType)
{
	OSStatus	error = noErr;
	FSRef		fileOrFolder;
	NSString*	result = nil;
	
	
	error = FSFindFolder(inDomain, inType, kDontCreateFolder, &fileOrFolder);
	if (noErr == error)
	{
		result = returnPathForFSRef(fileOrFolder);
	}
	if (nil == result) result = [NSString string];
	return result;
}// findFolder


/*!
Converts a Carbon-style FSRef for an existing file or folder
into a path as required by Cocoa routines.

(1.0)
*/
NSString*
returnPathForFSRef	(FSRef const&	inFileOrFolder)
{
	NSString*	result = nil;
	CFURLRef	urlRef = CFURLCreateFromFSRef(kCFAllocatorDefault, &inFileOrFolder);
	
	
	if (nil != urlRef)
	{
		NSURL*		urlObject = (NSURL*)urlRef;
		
		
		result = [urlObject path];
	}
	return result;
}// returnPathForFSRef

} // anonymous namespace


@implementation CocoaBasic_NoticeColorPanelChange


/*!
Notified when the user selects a new color in the system-wide
Colors panel.

(4.0)
*/
- (void)
changeColor:(id)	sender
{
#pragma unused(sender)
	NSColorPanel*	colorPanel = [NSColorPanel sharedColorPanel];
	NSColor*		newColor = [[colorPanel color] colorUsingColorSpaceName:NSDeviceRGBColorSpace];
	CGDeviceColor	newColorFloat;
	float			ignoredAlpha = 0;
	RGBColor		newColorRGB;
	
	
	[newColor getRed:&newColorFloat.red green:&newColorFloat.green blue:&newColorFloat.blue alpha:&ignoredAlpha];
	newColorFloat.red *= RGBCOLOR_INTENSITY_MAX;
	newColorFloat.green *= RGBCOLOR_INTENSITY_MAX;
	newColorFloat.blue *= RGBCOLOR_INTENSITY_MAX;
	newColorRGB.red = STATIC_CAST(newColorFloat.red, unsigned short);
	newColorRGB.green = STATIC_CAST(newColorFloat.green, unsigned short);
	newColorRGB.blue = STATIC_CAST(newColorFloat.blue, unsigned short);
	
	ColorBox_SetColor(gCurrentColorPanelFocus, &newColorRGB, true/* is user action */);
	
	// TEMPORARY UGLY HACK: since the Cocoa color panel contains sliders with their
	// own event loops, it is possible for update routines to be invoked continuously
	// in such a way that the window is ERASED without an update; all attempts to
	// render the window in other ways have failed, but slightly RESIZING the window
	// performs a full redraw and is still fairly subtle (the user might notice a
	// slight shift on the right edge of the window, but this is still far better
	// than having the entire window turn white); it is not clear why this is
	// happening, other than the usual theory that Carbon and Cocoa just weren’t
	// meant to play nicely together and crap like this will never be truly fixed
	// until the user interface can migrate to Cocoa completely...
	if (nullptr != gCurrentColorPanelFocus)
	{
		HIWindowRef		window = HIViewGetWindow(gCurrentColorPanelFocus);
		WindowClass		windowClass = kDocumentWindowClass;
		Rect			bounds;
		OSStatus		error = noErr;
		
		
		// for sheets, the PARENT window can sometimes be erased too...sigh
		error = GetWindowClass(window, &windowClass);
		if (kSheetWindowClass == windowClass)
		{
			HIWindowRef		parentWindow;
			
			
			error = GetSheetWindowParent(window, &parentWindow);
			if (noErr == error)
			{
				(OSStatus)HIViewRender(HIViewGetRoot(parentWindow));
			}
		}
		
		// view rendering does not seem to work for the main window, so
		// use a resize hack to force a complete update
		error = GetWindowBounds(window, kWindowContentRgn, &bounds);
		if (noErr == error)
		{
			++bounds.right;
			(OSStatus)SetWindowBounds(window, kWindowContentRgn, &bounds);
			--bounds.right;
			(OSStatus)SetWindowBounds(window, kWindowContentRgn, &bounds);
		}
	}
}/// changeColor:


@end // CocoaBasic_NoticeColorPanelChange

// BELOW IS REQUIRED NEWLINE TO END FILE
