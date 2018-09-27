/*!	\file CocoaBasic.mm
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2018 by Kevin Grant
	
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
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <CFRetainRelease.h>
#import <Console.h>
#import <Popover.objc++.h>
#import <SoundSystem.h>



#pragma mark Variables
namespace {

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
	NSString*			directoryPath = [asNSURL path];
	NSURL*				fullURL = [asNSURL URLByAppendingPathComponent:BRIDGE_CAST(inFileName, NSString*)];
	assert([fullURL isFileURL]);
	NSString*			absoluteFilePath = [fullURL path];
	NSFileManager*		fileManager = [NSFileManager defaultManager];
	NSError*			error = nil;
	Boolean				result = false;
	
	
	// create all parent directories
	result = [fileManager createDirectoryAtPath:directoryPath withIntermediateDirectories:YES
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
automatically handling them via a block.  The panel
resolves aliases, does not allow the user to choose
directories, and only allows the user to open files of
the given types.

The first two arguments can be "nullptr" to use defaults.
The defaults are: no message, and allowing all possible
file types.

The file type list is sophisticated and can accept
anything you would imagine: file name extensions, HFS
four-character codes, and reverse-domain-style UTIs.

Returns true only if the user tried to open something.

(2017.05)
*/
Boolean
CocoaBasic_FileOpenPanelDisplay		(CFStringRef	inMessage,
									 CFArrayRef		inAllowedFileTypes,
									 void			(^inOpenURLHandler)(CFURLRef))
{
@autoreleasepool {
	NSOpenPanel*		thePanel = [NSOpenPanel openPanel];
	NSModalResponse		buttonHit = NSFileHandlingPanelCancelButton;
	Boolean				result = false;
	
	
	// NOTE: newer versions of the dialog display the message but they
	// seem to ignore the "title" string entirely
	if (nullptr != inMessage)
	{
		[thePanel setMessage:BRIDGE_CAST(inMessage, NSString*)];
	}
	[thePanel setCanChooseDirectories:NO];
	[thePanel setCanChooseFiles:YES];
	[thePanel setAllowsOtherFileTypes:NO];
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
CGFloatRGBColor
CocoaBasic_GetGray	(CGFloatRGBColor const&		inColor,
					 Float32					inFraction)
{
@autoreleasepool {
	CGFloatRGBColor		result = inColor;
	NSColor*			c1 = [NSColor colorWithCalibratedRed:inColor.red green:inColor.green blue:inColor.blue alpha:1.0];
	NSColor*			c2 = [NSColor whiteColor];
	NSColor*			blended = [c1 blendedColorWithFraction:inFraction ofColor:c2];
	
	
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
CGFloatRGBColor
CocoaBasic_GetGray	(CGFloatRGBColor const&		inColor1,
					 CGFloatRGBColor const&		inColor2,
					 Float32					inFraction)
{
@autoreleasepool {
	CGFloatRGBColor		result = inColor1;
	NSColor*			c1 = [NSColor colorWithCalibratedRed:inColor1.red green:inColor1.green blue:inColor1.blue alpha:1.0];
	NSColor*			c2 = [NSColor colorWithCalibratedRed:inColor2.red green:inColor2.green blue:inColor2.blue alpha:1.0];
	NSColor*			blended = [c1 blendedColorWithFraction:inFraction ofColor:c2];
	
	
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
Returns a sorted list of names (without extensions) that are
valid sounds to pass to CocoaBasic_PlaySoundByName().  This is
useful for displaying a set of choices, such as a menu, to the
user.

Names are sorted within each section, e.g. sounds found in the
user’s Home directory or sounds found system-wide.

The list may also contain strings consisting of single dashes
("-"), which should not be treated as sound names; they can be
used to place dividing lines.

(1.0)
*/
CFArrayRef
CocoaBasic_ReturnUserSoundNames ()
{
	NSFileManager* const	kFileManager = [NSFileManager defaultManager];
	NSArray* const			libraryURLArray = [kFileManager
												URLsForDirectory:NSLibraryDirectory
																	inDomains:(NSUserDomainMask |
																				NSLocalDomainMask |
																				NSSystemDomainMask)];
	NSMutableArray*			result = [NSMutableArray array];
	
	
	for (NSURL* libraryDirectoryURL in libraryURLArray)
	{
		NSError*	error = nil;
		NSString*	soundsName = NSLocalizedStringFromTable(@"Sounds", @"FileSystem", @"the name of the directory for sound files in ~/Library, /Library or /System/Library");
		NSURL*		soundDirectoryURL = [libraryDirectoryURL URLByAppendingPathComponent:soundsName];
		NSArray*	soundFileURLs = [kFileManager
										contentsOfDirectoryAtURL:soundDirectoryURL
																	includingPropertiesForKeys:@[NSURLNameKey]
																	options:(NSDirectoryEnumerationSkipsHiddenFiles)
																	error:&error];
		
		
		if ((nil == soundFileURLs) && (nil != error))
		{
			// enable logs for debugging; basically not all directories
			// will exist (e.g. /Library/Sounds may not) so these are
			// not necessarily errors
		#if 0
			Console_Warning(Console_WriteValueCFString, "failed to iterate over sounds, directory",
							BRIDGE_CAST(soundDirectoryURL.path, CFStringRef));
			Console_Warning(Console_WriteValueCFString, "directory listing error",
							BRIDGE_CAST([error localizedDescription], CFStringRef));
		#endif
		}
		else if (soundFileURLs.count > 0)
		{
			NSSortDescriptor*	sortByPath = [NSSortDescriptor sortDescriptorWithKey:@"path" ascending:YES];
			NSArray*			descriptorArray = ((nil != sortByPath)
													? @[sortByPath]
													: @[]);
			
			
			if (result.count > 0)
			{
				[result addObject:@"-"]; // suggest a separator in between sound source directories
			}
			
			for (NSURL* soundURL in [soundFileURLs sortedArrayUsingDescriptors:descriptorArray])
			{
				NSString*	soundName = [soundURL.path lastPathComponent];
				
				
				if (nil != [NSSound soundNamed:soundName])
				{
					[result addObject:[soundName stringByDeletingPathExtension]];
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

// BELOW IS REQUIRED NEWLINE TO END FILE
