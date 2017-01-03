/*!	\file CocoaBasic.mm
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2017 by Kevin Grant
	
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
#import <CFRetainRelease.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <Popover.objc++.h>
#import <SoundSystem.h>



#pragma mark Types

typedef std::map< HIWindowRef, NSWindow* >		HIWindowRefToNSWindowMap;

#pragma mark Internal Method Prototypes
namespace {

NSString*	findFolder			(short, OSType);
NSString*	returnPathForFSRef	(FSRef const&);

} // anonymous namespace

#pragma mark Variables
namespace {

#if COCOA_BASIC_SUPPORTS_CARBON
HIWindowRefToNSWindowMap&			gCocoaCarbonWindows()	{ static HIWindowRefToNSWindowMap x; return x; }
#endif
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
@autoreleasepool {
	// clear out the version field because it tends to be redundant
	NSDictionary*	aboutBoxOptions = @{
											@"Version": @"",
										};
	
	
	[NSApp orderFrontStandardAboutPanelWithOptions:aboutBoxOptions];
}// @autoreleasepool
}// AboutPanelDisplay


/*!
Creates or overwrites the file at the specified location,
creating all intermediate parent directories.  If the
initial data is not "nullptr", it is written into the
file; otherwise the file has size zero.  Upon return, the
file is closed.  Returns true only if all steps are
successful.

(1.10)
*/
Boolean
CocoaBasic_CreateFileAndDirectoriesWithData		(CFURLRef		inDirectoryURL,
												 CFStringRef	inFileName,
												 CFDataRef		inInitialDataOrNull)
{
	NSURL*				asNSURL = BRIDGE_CAST(inDirectoryURL, NSURL*);
	assert([asNSURL isFileURL]);
	NSString*			dirctoryPath = [asNSURL path];
	NSURL*				fullURL = [asNSURL URLByAppendingPathComponent:BRIDGE_CAST(inFileName, NSString*)];
	assert([fullURL isFileURL]);
	NSString*			absoluteFilePath = [fullURL path];
	NSFileManager*		fileManager = [NSFileManager defaultManager];
	NSError*			error = nil;
	Boolean				result = false;
	
	
	// create all parent directories
	result = [fileManager createDirectoryAtPath:dirctoryPath withIntermediateDirectories:YES
												attributes:nil error:&error];
	if (NO == result)
	{
		if (nil != error)
		{
			Console_Warning(Console_WriteValueCFString, "failed to create parent directories, error",
							BRIDGE_CAST([error localizedDescription], CFStringRef));
		}
		else
		{
			Console_Warning(Console_WriteLine, "failed to create parent directories");
		}
	}
	
	// create the target file
	result = [fileManager createFileAtPath:absoluteFilePath contents:BRIDGE_CAST(inInitialDataOrNull, NSData*)
											attributes:nil];
	
	return result;
}// CreateFileAndDirectoriesWithData


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
CocoaBasic_FileOpenPanelDisplay		(void		(^inOpenURLHandler)(CFURLRef),
									 CFStringRef	inMessage,
									 CFStringRef	inWindowTitle,
									 CFArrayRef		inAllowedFileTypes)
{
@autoreleasepool {
	NSOpenPanel*	thePanel = [NSOpenPanel openPanel];
	int				buttonHit = NSFileHandlingPanelCancelButton;
	Boolean			result = false;
	
	
	if (nullptr != inMessage)
	{
		[thePanel setMessage:BRIDGE_CAST(inMessage, NSString*)];
	}
	if (nullptr != inWindowTitle)
	{
		[thePanel setTitle:BRIDGE_CAST(inWindowTitle, NSString*)];
	}
	[thePanel setCanChooseDirectories:NO];
	[thePanel setCanChooseFiles:YES];
	[thePanel setAllowsOtherFileTypes:YES];
	[thePanel setAllowsMultipleSelection:YES];
	[thePanel setResolvesAliases:YES];
	thePanel.allowedFileTypes = BRIDGE_CAST(inAllowedFileTypes, NSArray*);
	buttonHit = [thePanel runModal];
	result = (NSFileHandlingPanelOKButton == buttonHit);
	if (result)
	{
		for (NSURL* currentFileURL in [thePanel URLs])
		{
			inOpenURLHandler(BRIDGE_CAST(currentFileURL, CFURLRef));
		}
	}
	
	return result;
}// @autoreleasepool
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
@autoreleasepool {
	CGDeviceColor	result = inColor;
	NSColor*		c1 = [NSColor colorWithCalibratedRed:inColor.red green:inColor.green blue:inColor.blue alpha:1.0];
	NSColor*		c2 = [NSColor whiteColor];
	NSColor*		blended = [c1 blendedColorWithFraction:inFraction ofColor:c2];
	
	
	if (blended)
	{
		result.red = [blended redComponent];
		result.green = [blended greenComponent];
		result.blue = [blended blueComponent];
	}
	return result;
}// @autoreleasepool
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
@autoreleasepool {
	CGDeviceColor	result = inColor1;
	NSColor*		c1 = [NSColor colorWithCalibratedRed:inColor1.red green:inColor1.green blue:inColor1.blue alpha:1.0];
	NSColor*		c2 = [NSColor colorWithCalibratedRed:inColor2.red green:inColor2.green blue:inColor2.blue alpha:1.0];
	NSColor*		blended = [c1 blendedColorWithFraction:inFraction ofColor:c2];
	
	
	if (blended)
	{
		result.red = [blended redComponent];
		result.green = [blended greenComponent];
		result.blue = [blended blueComponent];
	}
	return result;
}// @autoreleasepool
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
@autoreleasepool {
	if ([inResponder respondsToSelector:@selector(invalidateRestorableState)])
	{
		[inResponder invalidateRestorableState];
	}
}// @autoreleasepool
}// InvalidateRestorableState


#if COCOA_BASIC_SUPPORTS_CARBON
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
@autoreleasepool {
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
}// @autoreleasepool
}// MakeFrontWindowCarbonUserFocusWindow
#endif


#if COCOA_BASIC_SUPPORTS_CARBON
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
@autoreleasepool {
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
}// @autoreleasepool
}// MakeKeyWindowCarbonUserFocusWindow
#endif


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
@autoreleasepool {
	[(NSSound*)[NSSound soundNamed:(NSString*)inName] stop];
	[(NSSound*)[NSSound soundNamed:(NSString*)inName] play];
}// @autoreleasepool
}// PlaySoundByName


/*!
Plays the sound in the specified file (asynchronously).

(1.0)
*/
void
CocoaBasic_PlaySoundFile	(CFURLRef	inFile)
{
@autoreleasepool {
	[[[[NSSound alloc] initWithContentsOfURL:(NSURL*)inFile byReference:NO] autorelease] play];
}// @autoreleasepool
}// PlaySoundFile


#if COCOA_BASIC_SUPPORTS_CARBON
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
@autoreleasepool {
	Boolean		result = false;
	
	
	if (nullptr != [inCocoaWindow windowRef])
	{
		gCocoaCarbonWindows().insert(std::make_pair(REINTERPRET_CAST([inCocoaWindow windowRef], HIWindowRef), inCocoaWindow));
		result = true;
	}
	return result;
}// @autoreleasepool
}// RegisterCocoaCarbonWindow
#endif


#if COCOA_BASIC_SUPPORTS_CARBON
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
@autoreleasepool {
	NSWindow*	result = nil;
	
	
	if (nullptr != inCarbonWindow)
	{
		auto	toPair = gCocoaCarbonWindows().find(inCarbonWindow);
		
		
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
				UNUSED_RETURN(Boolean)CocoaBasic_RegisterCocoaCarbonWindow(result);
			}
		}
	}
	return result;
}// @autoreleasepool
}// ReturnNewOrExistingCocoaCarbonWindow
#endif


/*!
Since Cocoa has a routine for finding the *localized* name of a
string encoding, it is exposed as a general API here.

The returned string is not retained, so do not release it.

(1.0)
*/
CFStringRef
CocoaBasic_ReturnStringEncodingLocalizedName	(CFStringEncoding	inEncoding)
{
@autoreleasepool {
	NSStringEncoding	translatedEncoding = CFStringConvertEncodingToNSStringEncoding(inEncoding);
	CFStringRef			result = BRIDGE_CAST([NSString localizedNameOfStringEncoding:translatedEncoding],
												CFStringRef);
	
	
	return result;
}// @autoreleasepool
}// ReturnStringEncodingLocalizedName


/*!
Returns an unsorted list of names (without extensions) that are
valid sounds to pass to CocoaBasic_PlaySoundByName().  This is
useful for displaying a set of choices, such as a menu, to the
user.

The list may also contain strings consisting of single dashes
("-"), which should not be treated as sound names; they can be
used to place dividing lines.

(1.0)
*/
CFArrayRef
CocoaBasic_ReturnUserSoundNames ()
{
	NSArray* const			kSearchPaths = @[
												findFolder(kUserDomain, kSystemSoundsFolderType),
												findFolder(kLocalDomain, kSystemSoundsFolderType),
												findFolder(kSystemDomain, kSystemSoundsFolderType),
											];
	NSFileManager* const	kFileManager = [NSFileManager defaultManager];
	BOOL					isDirectory = NO;
	NSMutableArray*			result = [NSMutableArray array];
	
	
	for (NSString* soundDirectory in kSearchPaths)
	{
		if ([kFileManager fileExistsAtPath:soundDirectory isDirectory:&isDirectory])
		{
			if (isDirectory)
			{
				NSError*	error = nil;
				NSArray*	soundFiles = [kFileManager contentsOfDirectoryAtPath:soundDirectory
																					error:&error];
				
				
				if ((nil == soundFiles) && (nil != error))
				{
					Console_Warning(Console_WriteValueCFString, "failed to iterate over sounds, directory",
									BRIDGE_CAST(soundDirectory, CFStringRef));
					Console_Warning(Console_WriteValueCFString, "directory listing error",
									BRIDGE_CAST([error localizedDescription], CFStringRef));
				}
				else if (soundFiles.count > 0)
				{
					if (result.count > 0)
					{
						[result addObject:@"-"]; // suggest a separator in between sound source directories
					}
					
					for (NSString* soundFile in soundFiles)
					{
						NSString*	soundName = [soundFile stringByDeletingPathExtension];
						
						
						if (nil != [NSSound soundNamed:soundName])
						{
							[result addObject:soundName];
						}
					}
				}
			}
		}
	}
	
	return BRIDGE_CAST(result, CFArrayRef);
}// ReturnUserSoundNames


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
@autoreleasepool {
	NSDictionary*	attributeDict = @{
										NSFileHFSTypeCode: [NSNumber numberWithUnsignedLong:inNewType],
										NSFileHFSCreatorCode: [NSNumber numberWithUnsignedLong:inNewCreator],
									};
	NSFileManager*	fileManager = [NSFileManager defaultManager];
	NSError*		error = nil;
	Boolean			result = false;
	
	
	// set attributes
	result = [fileManager setAttributes:attributeDict ofItemAtPath:(NSString*)inPath
										error:&error];
	if (NO == result)
	{
		if (nil != error)
		{
			Console_Warning(Console_WriteValueCFString, "failed to set file type/creator, error",
							BRIDGE_CAST([error localizedDescription], CFStringRef));
		}
		else
		{
			Console_Warning(Console_WriteLine, "failed to set file type/creator");
		}
	}
	
	return result;
}// @autoreleasepool
}// SetFileTypeCreator


/*!
Returns true if a computer voice is currently speaking.

(1.3)
*/
Boolean
CocoaBasic_SpeakingInProgress ()
{
@autoreleasepool {
	return ([gDefaultSynth isSpeaking]) ? true : false;
}// @autoreleasepool
}// SpeakingInProgress


/*!
Passes the given string to the default speech synthesizer.

(1.3)
*/
Boolean
CocoaBasic_StartSpeakingString	(CFStringRef	inCFString)
{
@autoreleasepool {
	BOOL		speakOK;
	
	
	if (nil == gDefaultSynth)
	{
		gDefaultSynth = [[NSSpeechSynthesizer alloc] initWithVoice:nil];
	}
	speakOK = ([gDefaultSynth startSpeakingString:(NSString*)inCFString]) ? true : false;
	
	return (speakOK) ? true : false;
}// @autoreleasepool
}// StartSpeakingString


/*!
Interrupts any current speech synthesis, and not
necessarily gracefully.

(1.3)
*/
void
CocoaBasic_StopSpeaking ()
{
@autoreleasepool {
	[gDefaultSynth stopSpeaking];
}// @autoreleasepool
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
	NSString*			result = nil;
	CFRetainRelease		urlRef(CFURLCreateFromFSRef(kCFAllocatorDefault, &inFileOrFolder),
								CFRetainRelease::kAlreadyRetained);
	
	
	if (urlRef.exists())
	{
		NSURL*		urlObject = BRIDGE_CAST(urlRef.returnCFTypeRef(), NSURL*);
		
		
		result = [urlObject path];
	}
	return result;
}// returnPathForFSRef

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
