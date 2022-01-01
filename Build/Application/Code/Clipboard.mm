/*!	\file Clipboard.mm
	\brief The clipboard window, and Copy and Paste management.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#import <UniversalDefines.h>

// standard-C includes
#import <climits>
#import <map>

// Unix includes
#import <sys/param.h>

// Mac includes
#import <objc/objc-runtime.h>
@import ApplicationServices;
@import CoreServices;

// library includes
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <Console.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// application includes
#import "Clipboard.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DragAndDrop.h"
#import "Preferences.h"
#import "Session.h"
#import "TerminalView.h"
#import "UIStrings.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>


#pragma mark Internal Method Prototypes
namespace {

CFStringRef		copyTypeDescription			(CFStringRef);
Boolean			isImageType					(CFStringRef);
Boolean			isTextType					(CFStringRef);
void			updateClipboard				();

} // anonymous namespace

#pragma mark Types

/*!
Implements the Clipboard window.  See "ClipboardCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Clipboard_WindowController : NSWindowController < UIClipboard_ActionHandling > //{

// class methods
	+ (id)
	sharedClipboardWindowController;

// new methods
	- (void)
	setNeedsDisplay;

// accessors
	@property (strong) UIClipboard_Model*
	viewModel;
	- (void)
	setDataSize:(size_t)_;
	- (void)
	setDataWidth:(size_t)_
	andHeight:(size_t)_;
	- (void)
	setKindField:(NSString*)_;
	- (void)
	setLabel1:(NSString*)_
	andValue:(NSString*)_;
	- (void)
	setLabel2:(NSString*)_
	andValue:(NSString*)_;
	- (void)
	setSizeField:(NSString*)_;

@end //}

#pragma mark Variables
namespace {

NSTimer*	gClipboardUpdatesTimer = nil;

} // anonymous namespace



#pragma mark Public Methods

/*!
This method sets up the initial configurations for
the clipboard window.  Call this method before
calling any other clipboard methods from this file.

(3.0)
*/
void
Clipboard_Init ()
{
	// install a timer that detects changes to the clipboard
	gClipboardUpdatesTimer = [NSTimer scheduledTimerWithTimeInterval:3.0/* in seconds */
	repeats:YES
	block:^(NSTimer* UNUSED_ARGUMENT(timer))
	{
		// since there does not appear to be an event that can be handled
		// to notice clipboard changes, this timer periodically polls the
		// system to figure out when the clipboard has changed; if it
		// does change, the clipboard window is updated
		updateClipboard();
	}];
	
	// if the window was open at last Quit, construct it right away;
	// otherwise, wait until it is requested by the user
	{
		Boolean		windowIsVisible = false;
		
		
		// get visibility preference for the Clipboard
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagWasClipboardShowing,
									sizeof(windowIsVisible), &windowIsVisible))
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		
		if (windowIsVisible)
		{
			Clipboard_SetWindowVisible(true);
		}
	}
}// Init


/*!
This method cleans up the clipboard window by
destroying data structures allocated by Clipboard_Init().

(3.0)
*/
void
Clipboard_Done ()
{
	// save visibility preferences implicitly
	{
		Boolean		windowIsVisible = false;
		
		
		windowIsVisible = Clipboard_WindowIsVisible();
		UNUSED_RETURN(Preferences_Result)Preferences_SetData(kPreferences_TagWasClipboardShowing,
																sizeof(Boolean), &windowIsVisible);
	}
	
	[gClipboardUpdatesTimer invalidate];
	gClipboardUpdatesTimer = nil;
}// Done


/*!
Publishes the specified data to the specified pasteboard
(or nullptr to use the primary pasteboard).  If the
"inClearFirst" is true, the pasteboard is cleared before
new data is added; otherwise, the new string is added to
any existing data on the pasteboard.

Returns true only if the text was added successfully.

(2018.08)
*/
Boolean
Clipboard_AddCFStringToPasteboard	(CFStringRef		inStringToCopy,
									 NSPasteboard*		inPasteboardOrNullForMainClipboard,
									 Boolean			inClearFirst)
{
	Boolean			result = false;
	NSPasteboard*	target = (nullptr == inPasteboardOrNullForMainClipboard)
								? [NSPasteboard generalPasteboard]
								: inPasteboardOrNullForMainClipboard;
	
	
	if (inClearFirst)
	{
		[target clearContents];
	}
	
	if (nullptr != inStringToCopy)
	{
		result = (YES == [target writeObjects:@[BRIDGE_CAST(inStringToCopy, NSString*)]]);
	}
	
	return result;
}// AddCFStringToPasteboard


/*!
Publishes the specified data to the specified pasteboard
(or nullptr to use the primary pasteboard).  If the
"inClearFirst" is true, the pasteboard is cleared before
new data is added; otherwise, the new image is added to
any existing data on the pasteboard.

Returns true only if the image was added successfully.

(2017.12)
*/
Boolean
Clipboard_AddNSImageToPasteboard	(NSImage*			inImageToCopy,
									 NSPasteboard*		inPasteboardOrNullForMainClipboard,
									 Boolean			inClearFirst)
{
	Boolean			result = false;
	NSPasteboard*	target = (nullptr == inPasteboardOrNullForMainClipboard)
								? [NSPasteboard generalPasteboard]
								: inPasteboardOrNullForMainClipboard;
	
	
	if (inClearFirst)
	{
		[target clearContents];
	}
	
	if (nil != inImageToCopy)
	{
		result = (YES == [target writeObjects:@[inImageToCopy]]);
	}
	
	return result;
}// AddNSImageToPasteboard


/*!
Returns true only if the specified pasteboard contains text
that was successfully converted.  This includes any data that
might be converted into text, such as converting a file or
directory into an escaped file system path.

Multi-line text returns a separate item in the array for
each line (the caller can then choose how to process these
lines, e.g. by inserting new-lines or joining them).

When there is more than one item on the pasteboard, all the
items are also separated.

When successful (returning true), the "outCFStringCFArray"
will be defined and you must call CFRelease() on it when
finished. Otherwise, it will be set to nullptr.

(2018.08)
*/
Boolean
Clipboard_CreateCFStringArrayFromPasteboard		(CFArrayRef&		outCFStringCFArray,
												 NSPasteboard*		inPasteboardOrNull)
{
	NSPasteboard*		kPasteboard = (nullptr == inPasteboardOrNull)
										? [NSPasteboard generalPasteboard]
										: inPasteboardOrNull;
	NSArray*			objectArray = nil;
	NSMutableArray*		newStringArray = [[NSMutableArray alloc] init];
	NSDictionary*		fileReadingOptions = @{ NSPasteboardURLReadingFileURLsOnlyKey: @(YES) };
	Boolean				result = false;
	
	
	outCFStringCFArray = nullptr; // initially...
	
	// first look for file objects
	objectArray = [kPasteboard readObjectsForClasses:@[NSURL.class] options:fileReadingOptions];
	if ((nil != objectArray) && (objectArray.count > 0))
	{
		// read URLs; in this case, copy all of them in a row
		// (separated by new lines)
		for (id anObject in objectArray)
		{
			if ([anObject isKindOfClass:NSURL.class])
			{
				NSURL*		asURL = STATIC_CAST(anObject, NSURL*);
				NSString*	stringValue = [asURL absoluteURL].path;
				
				
				if (nil != stringValue)
				{
					[newStringArray addObject:stringValue];
				}
				else
				{
					Console_Warning(Console_WriteLine, "unable to resolve NSURL object on pasteboard");
				}
			}
			else
			{
				// ???
				Console_Warning(Console_WriteLine, "non-NSURL object in pasteboard");
			}
		}
	}
	else
	{
		// read other types of items
		NSDictionary*	readingOptions = @{};
		
		
		objectArray = [kPasteboard readObjectsForClasses:@[NSPasteboardItem.class] options:readingOptions];
		if ((nil == objectArray) || (0 == objectArray.count))
		{
			// not text content
			//Console_Warning(Console_WriteLine, "failed to read any text from pasteboard");
		}
		else
		{
			for (id anObject in objectArray)
			{
				if ([anObject isKindOfClass:NSPasteboardItem.class])
				{
					// read text
					NSPasteboardItem*	asPasteboardItem = STATIC_CAST(anObject, NSPasteboardItem*);
					NSArray*			textUTIs = @[
														// in order of preference, and most specific first (otherwise the
														// more generic types will match)
														BRIDGE_CAST(kUTTypeUTF16ExternalPlainText, NSString*),
														BRIDGE_CAST(kUTTypeUTF16PlainText, NSString*),
														BRIDGE_CAST(kUTTypeUTF8PlainText, NSString*),
														BRIDGE_CAST(kUTTypePlainText, NSString*),
														@"com.apple.traditional-mac-plain-text",
													];
					
					
					for (NSString* aUTI in textUTIs)
					{
						NSString*	stringValue = [asPasteboardItem stringForType:aUTI];
						
						
						if (nil != stringValue)
						{
							CFRetainRelease		lineArray(StringUtilities_CFNewStringsWithLines(BRIDGE_CAST(stringValue, CFStringRef)),
															CFRetainRelease::kAlreadyRetained);
							
							
							for (id stringObject in BRIDGE_CAST(lineArray.returnCFArrayRef(), NSArray*))
							{
								if (NO == [stringObject isKindOfClass:NSString.class])
								{
									Console_Warning(Console_WriteLine, "assertion failure; received non-string object in array that expected strings");
								}
								else
								{
									NSString*	asString = STATIC_CAST(stringObject, NSString*);
									
									
									[newStringArray addObject:asString];
								}
							}
							break;
						}
					}
				}
				else
				{
					// ???
					Console_Warning(Console_WriteLine, "non-NSPasteboardItem object in pasteboard");
				}
			}
		}
	}
	
	if (newStringArray.count > 0)
	{
		outCFStringCFArray = BRIDGE_CAST(newStringArray, CFArrayRef);
		CFRetain(outCFStringCFArray);
		result = true;
	}
	
	return result;
}// CreateCFStringArrayFromPasteboard


/*!
Returns true only if some item on the specified pasteboard
is an image.

You must CFRelease() the UTI string.

(2018.08)
*/
Boolean
Clipboard_CreateCGImageFromPasteboard	(CGImageRef&		outImage,
										 CFStringRef&		outUTI,
										 NSPasteboard*		inPasteboardOrNull)
{
	Boolean			result = false;
	NSPasteboard*	kPasteboard = (nullptr == inPasteboardOrNull)
									? [NSPasteboard generalPasteboard]
									: inPasteboardOrNull;
	NSArray*		objectArray = nil;
	NSDictionary*	readingOptions = @{};
	
	
	objectArray = [kPasteboard readObjectsForClasses:@[NSImage.class] options:readingOptions];
	if ((nil == objectArray) || (0 == objectArray.count))
	{
		// not an image
		//Console_Warning(Console_WriteLine, "failed to read any image from the given pasteboard");
	}
	else
	{
		for (id anObject in objectArray)
		{
			if ([anObject isKindOfClass:NSImage.class])
			{
				NSImage*	asImage = STATIC_CAST(anObject, NSImage*);
				
				
				outImage = [asImage CGImageForProposedRect:nullptr context:nil hints:@{}];
				if (nullptr != outImage)
				{
					CGImageRetain(outImage);
					outUTI = kUTTypeImage; // INCOMPLETE; CGImageGetUTType() can be used in 10.11+
					CFRetain(outUTI);
					result = true;
					break;
				}
			}
		}
	}			
	
	return result;
}// CreateCGImageFromPasteboard


/*!
Shows or hides the clipboard.

(3.0)
*/
void
Clipboard_SetWindowVisible	(Boolean	inIsVisible)
{
	if (inIsVisible)
	{
		[[Clipboard_WindowController sharedClipboardWindowController] showWindow:NSApp];
	}
	else
	{
		[[Clipboard_WindowController sharedClipboardWindowController] close];
	}
}// SetWindowVisible


/*!
Copies the current selection from the indicated
virtual screen, or does nothing if no text is
selected.  If the selection mode is rectangular,
the text is copied line-by-line according to the
rows it crosses.

If the copy method is standard, the text is copied
to the pasteboard intact.  If the “table” method
is used, the text is first parsed to replace tab
characters with multiple spaces, and then the text
is copied.  If “inline” is specified (possibly in
addition to table mode), each line is concatenated
together to form a single line.

(3.0)
*/
void
Clipboard_TextToScrap	(TerminalViewRef		inView,
						 Clipboard_CopyMethod	inHowToCopy,
						 NSPasteboard*			inDataTargetOrNull)
{
	CFStringRef		textToCopy = nullptr;	// where to store the characters
	short			tableThreshold = 0;		// zero for normal, nonzero for copy table mode
	
	
	if (nullptr == inDataTargetOrNull)
	{
		inDataTargetOrNull = [NSPasteboard generalPasteboard];
	}
	
	if (inHowToCopy & kClipboard_CopyMethodTable)
	{
		// get the user’s “one tab equals X spaces” preference, if possible
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCopyTableThreshold,
									sizeof(tableThreshold), &tableThreshold))
		{
			tableThreshold = 4; // default to 4 spaces per tab if no preference can be found
		}
	}
	
	// get the highlighted characters; assume that text should be inlined unless selected in rectangular mode
	{
		TerminalView_TextFlags	flags = (inHowToCopy & kClipboard_CopyMethodInline) ? kTerminalView_TextFlagInline : 0;
		
		
		textToCopy = TerminalView_ReturnSelectedTextCopyAsUnicode(inView, tableThreshold, flags);
	}
	
	if (nullptr != textToCopy)
	{
		if (CFStringGetLength(textToCopy) > 0)
		{
			UNUSED_RETURN(Boolean)Clipboard_AddCFStringToPasteboard(textToCopy, inDataTargetOrNull);
		}
		CFRelease(textToCopy), textToCopy = nullptr;
	}
}// TextToScrap


/*!
Determines if the clipboard window is showing.  This
state is not exactly the window state; specifically,
if the clipboard is suspended but set to visible,
the window will be invisible (because the application
is suspended) and yet this method will return "true".

(3.0)
*/
Boolean
Clipboard_WindowIsVisible ()
{
	return ([[[Clipboard_WindowController sharedClipboardWindowController] window] isVisible]) ? true : false;
}// WindowIsVisible


#pragma mark Internal Methods
namespace {

/*!
Returns a human-readable description of the specified data type.
If not nullptr, call CFRelease() on the result when finished.

(4.0)
*/
CFStringRef
copyTypeDescription		(CFStringRef	inUTI)
{
	CFStringRef		result = nullptr;
	
	
	result = UTTypeCopyDescription(inUTI);
	if (nullptr == result)
	{
		if (nullptr != inUTI)
		{
			if (isTextType(inUTI))
			{
				result = UIStrings_ReturnCopy(kUIStrings_ClipboardWindowGenericKindText);
			}
			else if (isImageType(inUTI))
			{
				result = UIStrings_ReturnCopy(kUIStrings_ClipboardWindowGenericKindImage);
			}
		}
		if (nullptr == result)
		{
			result = CFSTR("-");
			CFRetain(result);
		}
	}
	return result;
}// copyTypeDescription


/*!
Returns true only if the specified UTI appears to be
referring to some kind of image.

This uses a heuristic, and is meant for efficiency; a
slower and more accurate method may be to construct an
image, using Clipboard_CreateCGImageFromPasteboard().

(4.0)
*/
Boolean
isImageType		(CFStringRef	inUTI)
{
	Boolean		result = false;
	
	
	if (nullptr != inUTI)
	{
		result = ((CFStringFind(inUTI, CFSTR("image"), kCFCompareBackwards).length > 0) ||
					(CFStringFind(inUTI, CFSTR("picture"), kCFCompareBackwards).length > 0) ||
					(CFStringFind(inUTI, CFSTR("graphics"), kCFCompareBackwards).length > 0));
	}
	return result;
}// isImageType


/*!
Returns true only if the specified UTI appears to be
referring to some kind of text.

This uses a heuristic, and is meant for efficiency; a
slower and more accurate method may be
Clipboard_CreateCFStringArrayFromPasteboard().

(4.0)
*/
Boolean
isTextType	(CFStringRef	inUTI)
{
	Boolean		result = false;
	
	
	if (nullptr != inUTI)
	{
		result = (CFStringFind(inUTI, CFSTR("text"), kCFCompareBackwards).length > 0);
	}
	return result;
}// isTextType


/*!
Updates internal state so that other API calls from this
module actually work with the given pasteboard!

If the given pasteboard is the primary pasteboard, this
also copies recognizable data (if any) from the specified
pasteboard and arranges to update the Clipboard window
accordingly.  The Clipboard window is only capable of
rendering one pasteboard at a time.

IMPORTANT:	This API should be called whenever the
			pasteboard is changed.

(3.1)
*/
void
updateClipboard ()
{
	Clipboard_WindowController*		controller = (Clipboard_WindowController*)
													[Clipboard_WindowController sharedClipboardWindowController];
	NSPasteboard*					generalPasteboard = [NSPasteboard generalPasteboard];
	CGImageRef						imageToRender = nullptr;
	CFArrayRef						stringsToRender = nullptr;
	CFStringRef						typeIdentifier = nullptr;
	CFStringRef						typeCFString = nullptr;
	
	
	if (Clipboard_CreateCGImageFromPasteboard(imageToRender, typeIdentifier, generalPasteboard))
	{
		// image
		typeCFString = copyTypeDescription(typeIdentifier);
		controller.viewModel.clipboardText = @"";
		controller.viewModel.clipboardImage = [[NSImage alloc] initWithPasteboard:generalPasteboard];
		controller.viewModel.clipboardUnknown = NO;
		controller.viewModel.kindValue = BRIDGE_CAST(typeCFString, NSString*);
		[controller setDataSize:(CGImageGetHeight(imageToRender) * CGImageGetBytesPerRow(imageToRender))];
		[controller setDataWidth:CGImageGetWidth(imageToRender) andHeight:CGImageGetHeight(imageToRender)];
		CFRelease(imageToRender), imageToRender = nullptr;
		CFRelease(typeIdentifier), typeIdentifier = nullptr;
	}
	else if (Clipboard_CreateCFStringArrayFromPasteboard(stringsToRender, generalPasteboard))
	{
		// text
		NSString*	textToRender = [BRIDGE_CAST(stringsToRender, NSArray*) componentsJoinedByString:@"\n"];
		
		
		typeCFString = copyTypeDescription(kUTTypeText);
		controller.viewModel.clipboardText = textToRender;
		controller.viewModel.clipboardImage = nil;
		controller.viewModel.clipboardUnknown = NO;
		controller.viewModel.kindValue = BRIDGE_CAST(typeCFString, NSString*);
		[controller setDataSize:(textToRender.length * sizeof(UniChar))]; // TEMPORARY; this is probably not always right
		controller.viewModel.extraInfoLabel1 = nil;
		controller.viewModel.extraInfoValue1 = nil;
		controller.viewModel.extraInfoLabel2 = nil;
		controller.viewModel.extraInfoValue2 = nil;
		CFRelease(stringsToRender), stringsToRender = nullptr;
	}
	else
	{
		// unknown, or empty
		CFRetainRelease		unknownCFString(UIStrings_ReturnCopy(kUIStrings_ClipboardWindowValueUnknown),
											CFRetainRelease::kAlreadyRetained);
		
		
		controller.viewModel.clipboardText = @"";
		controller.viewModel.clipboardImage = nil;
		controller.viewModel.clipboardUnknown = (generalPasteboard.pasteboardItems.count > 0);
		typeCFString = copyTypeDescription(typeIdentifier);
		[controller setKindField:BRIDGE_CAST(typeCFString, NSString*)];
		[controller setSizeField:BRIDGE_CAST(unknownCFString.returnCFStringRef(), NSString*)];
		if ((nullptr == typeCFString) || (0 == CFStringGetLength(typeCFString)))
		{
			[controller setKindField:BRIDGE_CAST(unknownCFString.returnCFStringRef(), NSString*)];
		}
		controller.viewModel.extraInfoLabel1 = nil;
		controller.viewModel.extraInfoValue1 = nil;
		controller.viewModel.extraInfoLabel2 = nil;
		controller.viewModel.extraInfoValue2 = nil;
	}
	if (nullptr != typeCFString)
	{
		CFRelease(typeCFString), typeCFString = nullptr;
	}
}// updateClipboard

} // anonymous namespace


#pragma mark -
@implementation Clipboard_WindowController


static Clipboard_WindowController*	gClipboard_WindowController = nil;


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedClipboardWindowController
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gClipboard_WindowController = [[self.class allocWithZone:NULL] init];
	});
	return gClipboard_WindowController;
}// sharedClipboardWindowController


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	UIClipboard_Model*		viewModel = [[UIClipboard_Model alloc] initWithRunner:self];
	NSViewController*		viewController = [UIClipboard_ObjC makeViewController:viewModel];
	NSWindow*				windowObject = [NSWindow windowWithContentViewController:viewController];
	
	
	windowObject.styleMask = (NSWindowStyleMaskTitled |
								NSWindowStyleMaskClosable |
								NSWindowStyleMaskMiniaturizable |
								NSWindowStyleMaskResizable);
	windowObject.title = NSLocalizedString(@"Clipboard: MacTerm", @"title of Clipboard window");
	
	self = [super initWithWindow:windowObject];
	if (nil != self)
	{
		self.viewModel = viewModel;
	}
	return self;
}// init


/*!
Updates the Size field in the Clipboard window to show the
specified pasteboard data size.

The size is given in bytes, but is represented in a more
human-readable form, e.g. "352 K" or "1.2 MB".

(4.0)
*/
- (void)
setDataSize:(size_t)	dataSize
{
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	CFStringRef			templateCFString = CFSTR("");
	CFStringRef			finalCFString = CFSTR("");
	Boolean				releaseFinalCFString = false;
	
	
	// create a string describing the approximate size, by first making
	// the substrings that will be used to form that description
	stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowDataSizeTemplate, templateCFString);
	if (stringResult.ok())
	{
		// adjust the size value so the number displayed to the user is not too big and ugly;
		// construct a size string, with appropriate units and the word “about”, if necessary
		CFStringRef							aboutCFString = CFSTR("");
		CFStringRef							unitsCFString = CFSTR("");
		UIStrings_ClipboardWindowCFString	unitsStringCode = kUIStrings_ClipboardWindowUnitsBytes;
		Boolean								releaseAboutCFString = false;
		Boolean								releaseUnitsCFString = false;
		Boolean								approximation = false;
		size_t								contentSize = dataSize;
		
		
		if (STATIC_CAST(contentSize, size_t) > INTEGER_MEGABYTES(1))
		{
			approximation = true;
			contentSize /= INTEGER_MEGABYTES(1);
			unitsStringCode = kUIStrings_ClipboardWindowUnitsMB;
		}
		else if (STATIC_CAST(contentSize, size_t) > INTEGER_KILOBYTES(1))
		{
			approximation = true;
			contentSize /= INTEGER_KILOBYTES(1);
			unitsStringCode = kUIStrings_ClipboardWindowUnitsK;
		}
		else if (contentSize == 1)
		{
			unitsStringCode = kUIStrings_ClipboardWindowUnitsByte;
		}
		
		// if the size value is not exact, insert a word into the
		// string; e.g. “about 3 MB”
		if (approximation)
		{
			stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowDataSizeApproximately,
											aboutCFString);
			if (stringResult.ok())
			{
				// a new string was created, so it must be released later
				releaseAboutCFString = true;
			}
		}
		
		// based on the determined units, find the proper unit string
		stringResult = UIStrings_Copy(unitsStringCode, unitsCFString);
		if (stringResult.ok())
		{
			// a new string was created, so it must be released later
			releaseUnitsCFString = true;
		}
		
		// construct a string that contains all the proper substitutions
		// (see the UIStrings code to ensure the substitution order is right!!!)
		finalCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
													templateCFString,
													aboutCFString, contentSize, unitsCFString);
		if (nullptr == finalCFString)
		{
			Console_WriteLine("unable to create clipboard data size string from format");
			finalCFString = CFSTR("");
		}
		else
		{
			releaseFinalCFString = true;
		}
		
		if (releaseAboutCFString)
		{
			CFRelease(aboutCFString), aboutCFString = nullptr;
		}
		if (releaseUnitsCFString)
		{
			CFRelease(unitsCFString), unitsCFString = nullptr;
		}
		CFRelease(templateCFString), templateCFString = nullptr;
	}
	
	// update the window header text
	[self setSizeField:BRIDGE_CAST(finalCFString, NSString*)];
	
	if (releaseFinalCFString)
	{
		CFRelease(finalCFString), finalCFString = nullptr;
	}
}// setDataSize:


/*!
Updates the Width and Height fields in the Clipboard window to
show the specified pasteboard image dimensions, which are
currently assumed to be only in pixels.

(4.0)
*/
- (void)
setDataWidth:(size_t)	widthInPixels
andHeight:(size_t)		heightInPixels
{
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	CFStringRef			templateCFString = nullptr;
	
	
	// create a string describing the approximate size, by first making
	// the substrings that will be used to form that description
	stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowPixelDimensionTemplate, templateCFString);
	if (stringResult.ok())
	{
		CFStringRef		widthCFString = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowLabelWidth, widthCFString);
		if (stringResult.ok())
		{
			CFStringRef		heightCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowLabelHeight, heightCFString);
			if (stringResult.ok())
			{
				// set the width and height strings, using the same string template
				// (see the UIStrings code to ensure the substitution order is right!!!)
				CFStringRef		finalCFString = nullptr;
				
				
				if (1 == widthInPixels)
				{
					stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowOnePixel, finalCFString);
				}
				else
				{
					finalCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
																templateCFString, widthInPixels);
				}
				if (nullptr == finalCFString)
				{
					Console_WriteLine("unable to create clipboard image dimension string from format");
					finalCFString = CFSTR("");
					CFRetain(finalCFString);
				}
				[self setLabel1:BRIDGE_CAST(widthCFString, NSString*) andValue:BRIDGE_CAST(finalCFString, NSString*)];
				CFRelease(finalCFString), finalCFString = nullptr;
				
				if (1 == heightInPixels)
				{
					stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowOnePixel, finalCFString);
				}
				else
				{
					finalCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
																templateCFString, heightInPixels);
				}
				if (nullptr == finalCFString)
				{
					Console_WriteLine("unable to create clipboard image dimension string from format");
					finalCFString = CFSTR("");
					CFRetain(finalCFString);
				}
				[self setLabel2:BRIDGE_CAST(heightCFString, NSString*) andValue:BRIDGE_CAST(finalCFString, NSString*)];
				CFRelease(finalCFString), finalCFString = nullptr;
				
				CFRelease(heightCFString), heightCFString = nullptr;
			}
			CFRelease(widthCFString), widthCFString = nullptr;
		}
		CFRelease(templateCFString), templateCFString = nullptr;
	}
}// setDataWidth:andHeight:


/*!
Updates the user interface element that shows the Kind of the
data on the Clipboard.

(4.0)
*/
- (void)
setKindField:(NSString*)	aString
{
	self.viewModel.kindValue = aString;
}// setKindField:


/*!
Updates the first generic key-value pair in the user interface
that shows information about the data on the Clipboard.

(4.0)
*/
- (void)
setLabel1:(NSString*)	labelString
andValue:(NSString*)	valueString
{
	if (nil != labelString)
	{
		self.viewModel.extraInfoLabel1 = labelString;
		self.viewModel.extraInfoValue1 = valueString;
	}
	else
	{
		self.viewModel.extraInfoLabel1 = nil;
		self.viewModel.extraInfoValue1 = nil;
	}
}// setLabel1:andValue:


/*!
Updates the second generic key-value pair in the user interface
that shows information about the data on the Clipboard.

(4.0)
*/
- (void)
setLabel2:(NSString*)	labelString
andValue:(NSString*)	valueString
{
	if (nil != labelString)
	{
		self.viewModel.extraInfoLabel2 = labelString;
		self.viewModel.extraInfoValue2 = valueString;
	}
	else
	{
		self.viewModel.extraInfoLabel2 = nil;
		self.viewModel.extraInfoValue2 = nil;
	}
}// setLabel2:andValue:


/*!
Marks the main content area as dirty, so that it will be
redrawn soon.  This is appropriate after the Clipboard’s data
has been changed.

(4.0)
*/
- (void)
setNeedsDisplay
{
	self.window.contentView.needsDisplay = YES;
}// setNeedsDisplay


/*!
Updates the user interface element that shows the Size of the
data on the Clipboard.

(4.0)
*/
- (void)
setSizeField:(NSString*)	aString
{
	self.viewModel.sizeValue = aString;
}// setSizeField:


#pragma mark NSWindowController


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"Clipboard";
}// windowFrameAutosaveName


@end // Clipboard_WindowController

// BELOW IS REQUIRED NEWLINE TO END FILE
