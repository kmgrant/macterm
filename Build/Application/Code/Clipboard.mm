/*###############################################################

	Clipboard.mm
	
	MacTerm
		© 1998-2012 by Kevin Grant.
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
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>
#import <QuickTime/QuickTime.h>

// library includes
#import <CFUtilities.h>
#import <CocoaFuture.objc++.h>
#import <ColorUtilities.h>
#import <Console.h>
#import <Cursors.h>
#import <FlagManager.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>

// application includes
#import "AppleEventUtilities.h"
#import "Clipboard.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "DragAndDrop.h"
#import "Preferences.h"
#import "Session.h"
#import "TerminalView.h"
#import "UIStrings.h"
#import "VectorCanvas.h"
#import "VectorInterpreter.h"



#pragma mark Constants
namespace {

Float32 const	kSeparatorWidth = 1.0; // vertical line rendered at edge of Clipboard window content
Float32 const	kSeparatorPerceivedWidth = 2.0;

enum My_Type
{
	kMy_TypeUnknown		= 0,	//!< clipboard data is not in a supported form
	kMy_TypeText		= 1,	//!< clipboard contains some supported form of text
	kMy_TypeGraphics	= 2,	//!< clipboard contains some supported form of image data
};

} // anonymous namespace

#pragma mark Types
namespace {

typedef std::map< PasteboardRef, Boolean >		My_FlagByPasteboard;
typedef std::map< PasteboardRef, My_Type >		My_TypeByPasteboard;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void			clipboardUpdatesTimer					(EventLoopTimerRef, void*);
CFStringRef		copyTypeDescription						(CFStringRef);
OSStatus		createCGImageFromComponentConnection	(GraphicsImportComponent, CGImageRef&);
OSStatus		createCGImageFromData					(CFDataRef, CGImageRef&);
void			disposeImporterImageBuffer				(void*, void const*, size_t);
Boolean			isImageType								(CFStringRef);
Boolean			isTextType								(CFStringRef);
void			pictureToScrap							(Handle);
void			textToScrap								(Handle);
void			updateClipboard							(PasteboardRef);

} // anonymous namespace

#pragma mark Variables
namespace {

My_TypeByPasteboard&		gClipboardDataGeneralTypes ()	{ static My_TypeByPasteboard x; return x; }
My_FlagByPasteboard&		gClipboardLocalChanges ()	{ static My_FlagByPasteboard x; return x; }
CFRetainRelease				gCurrentRenderData;
EventLoopTimerUPP			gClipboardUpdatesTimerUPP = nullptr;
EventLoopTimerRef			gClipboardUpdatesTimer = nullptr;

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
	{
		OSStatus	error = noErr;
		
		
		gClipboardUpdatesTimerUPP = NewEventLoopTimerUPP(clipboardUpdatesTimer);
		assert(nullptr != gClipboardUpdatesTimerUPP);
		error = InstallEventLoopTimer(GetCurrentEventLoop(),
										kEventDurationNoWait + 0.01/* seconds before timer starts; must be nonzero for Mac OS X 10.3 */,
										kEventDurationSecond * 3.0/* seconds between fires */,
										gClipboardUpdatesTimerUPP, nullptr/* user data - not used */,
										&gClipboardUpdatesTimer);
	}
	
	// if the window was open at last Quit, construct it right away;
	// otherwise, wait until it is requested by the user
	{
		Boolean		windowIsVisible = false;
		size_t		actualSize = 0L;
		
		
		// get visibility preference for the Clipboard
		unless (Preferences_GetData(kPreferences_TagWasClipboardShowing,
									sizeof(windowIsVisible), &windowIsVisible,
									&actualSize) == kPreferences_ResultOK)
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		
		if (windowIsVisible)
		{
			Clipboard_SetWindowVisible(true);
		}
	}
	
	// trigger an initial rendering of whatever was on the clipboard at application launch
	gClipboardLocalChanges()[Clipboard_ReturnPrimaryPasteboard()] = true;
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
		(Preferences_Result)Preferences_SetData(kPreferences_TagWasClipboardShowing,
												sizeof(Boolean), &windowIsVisible);
	}
	
	RemoveEventLoopTimer(gClipboardUpdatesTimer), gClipboardUpdatesTimer = nullptr;
	DisposeEventLoopTimerUPP(gClipboardUpdatesTimerUPP), gClipboardUpdatesTimerUPP = nullptr;
}// Done


/*!
Publishes the specified data to the specified pasteboard
(or nullptr to use the primary pasteboard).

The string is converted into an external representation
of Unicode, "kUTTypeUTF16ExternalPlainText".

\retval noErr
if the string was added successfully

\retval unicodePartConvertErr
if there was an error creating an external representation

\retval (other)
if access to the pasteboard could not be secured

(3.1)
*/
OSStatus
Clipboard_AddCFStringToPasteboard	(CFStringRef		inStringToCopy,
									 PasteboardRef		inPasteboardOrNullForMainClipboard)
{
	OSStatus		result = noErr;
	PasteboardRef	target = (nullptr == inPasteboardOrNullForMainClipboard)
								? Clipboard_ReturnPrimaryPasteboard()
								: inPasteboardOrNullForMainClipboard;
	
	
	result = PasteboardClear(target);
	if (noErr == result)
	{
		// primary Unicode storage; this should be lossless, and should
		// succeed in order for the Copy to be considered successful
		{
			CFDataRef	externalRepresentation = CFStringCreateExternalRepresentation
													(kCFAllocatorDefault, inStringToCopy, kCFStringEncodingUnicode,
														'?'/* loss byte */);
			
			
			if (nullptr == externalRepresentation)
			{
				result = unicodePartConvertErr;
			}
			else
			{
				result = PasteboardPutItemFlavor(target, (PasteboardItemID)inStringToCopy,
													FUTURE_SYMBOL(CFSTR("public.utf16-external-plain-text"),
																	kUTTypeUTF16ExternalPlainText),
													externalRepresentation, kPasteboardFlavorNoFlags);
				CFRelease(externalRepresentation), externalRepresentation = nullptr;
			}
		}
		
		// if the copy is reasonably small, add extra versions of it, using
		// other text formats; this helps older applications to properly
		// Paste the selection; any errors here are not critical, and the
		// conversions may be lossy, so "result" is not updated from now on
		if (CFStringGetLength(inStringToCopy) < 16384/* completely arbitrary; avoid replicating “very large” selections */)
		{
			// UTF-8
			{
				CFDataRef	externalRepresentation = CFStringCreateExternalRepresentation
														(kCFAllocatorDefault, inStringToCopy, kCFStringEncodingUTF8,
														'?'/* loss byte */);
				
				
				if (nullptr == externalRepresentation)
				{
					//Console_Warning(Console_WriteLine, "unable to create UTF-8 for copied text; some apps may not be able to Paste");
				}
				else
				{
					OSStatus	error = PasteboardPutItemFlavor(target, (PasteboardItemID)inStringToCopy,
																FUTURE_SYMBOL(CFSTR("public.utf8-plain-text"),
																				kUTTypeUTF8PlainText),
																externalRepresentation, kPasteboardFlavorNoFlags);
					
					
					if (noErr != error)
					{
						//Console_Warning(Console_WriteValue, "unable to copy UTF-8 text flavor, error", error);
					}
					CFRelease(externalRepresentation), externalRepresentation = nullptr;
				}
			}
			
			// Mac Roman
			{
				CFDataRef	externalRepresentation = CFStringCreateExternalRepresentation
														(kCFAllocatorDefault, inStringToCopy, kCFStringEncodingMacRoman,
														'?'/* loss byte */);
				
				
				if (nullptr == externalRepresentation)
				{
					//Console_Warning(Console_WriteLine, "unable to create Mac Roman for copied text; some apps may not be able to Paste");
				}
				else
				{
					OSStatus	error = PasteboardPutItemFlavor(target, (PasteboardItemID)inStringToCopy,
																FUTURE_SYMBOL(CFSTR("public.plain-text"),
																				kUTTypePlainText),
																externalRepresentation, kPasteboardFlavorNoFlags);
					
					
					if (noErr != error)
					{
						//Console_Warning(Console_WriteValue, "unable to copy Mac Roman text flavor, error", error);
					}
					CFRelease(externalRepresentation), externalRepresentation = nullptr;
				}
			}
		}
	}
	return result;
}// AddCFStringToPasteboard


/*!
Reads an Apple Event descriptor containing data meant
to be copied to the clipboard, and attempts to copy it.
If successful, "noErr" is returned.

(3.0)
*/
OSStatus
Clipboard_AEDescToScrap		(AEDesc const*		inDescPtr)
{
	OSStatus	result = noErr;
	Size		dataSize = 0L;
	Handle		handle = nullptr;
	
	
	dataSize = AEGetDescDataSize(inDescPtr);
	handle = Memory_NewHandle(dataSize);
	if (handle == nullptr) result = memFullErr;
	else
	{
		Size		actualSize = 0L;
		
		
		// first, try coercing the data into text and copy it that way
		result = AppleEventUtilities_CopyDescriptorDataAs(typeChar, inDescPtr, *handle, GetHandleSize(handle), &actualSize);
		if (result == noErr) textToScrap(handle);
		else
		{
			// failed - try coercing the data into a picture instead, and copy it that way
			result = AppleEventUtilities_CopyDescriptorDataAs(typePict, inDescPtr, *handle, GetHandleSize(handle),
																&actualSize);
			if (result == noErr) pictureToScrap(handle);
		}
		Memory_DisposeHandle(&handle);
	}
	return result;
}// AEDescToScrap


/*!
Returns true only if the specified type of data (or something
similar enough to be compatible) is available from the given data
source.

If true is returned, the actual type name and Pasteboard item ID
of the first (or specified) conforming item is provided, so that
you can easily retrieve its data if desired.  You must
CFRelease() the type name string.

Specify 0 for "inDesiredItemOrZeroForAll", unless you know how
many items are on the pasteboard and want to test a specific
item.  Either way, the number of total items found is returned in
"outNumberOfItems".

The source is first synchronized before checking its contents.

You typically pass kUTTypePlainText to see if text is available.

For convenience, if you specify nullptr for the source, then
Clipboard_ReturnPrimaryPasteboard() is used.

(3.1)
*/
Boolean
Clipboard_Contains	(CFStringRef			inUTI,
					 UInt16					inDesiredItemOrZeroForAll,
					 CFStringRef&			outConformingItemActualType,
					 PasteboardItemID&		outConformingItemID,
					 PasteboardRef			inDataSourceOrNull)
{
	Boolean		result = false;
	OSStatus	error = noErr;
	ItemCount	totalItems = 0;
	ItemCount	firstItem = 1;
	ItemCount	lastItem = 1;
	ItemCount	itemIndex = 0;
	
	
	if (nullptr == inDataSourceOrNull)
	{
		inDataSourceOrNull = Clipboard_ReturnPrimaryPasteboard();
	}
	assert(nullptr != inDataSourceOrNull);
	
	// the data could be out of date if another application changed it...resync
	(PasteboardSyncFlags)PasteboardSynchronize(inDataSourceOrNull);
	
	error = PasteboardGetItemCount(inDataSourceOrNull, &totalItems);
	assert_noerr(error);
	
	if (0 == inDesiredItemOrZeroForAll)
	{
		firstItem = 1;
		lastItem = totalItems;
	}
	else
	{
		firstItem = inDesiredItemOrZeroForAll;
		lastItem = inDesiredItemOrZeroForAll;
	}
	
	for (itemIndex = firstItem; itemIndex <= lastItem; ++itemIndex)
	{
		PasteboardItemID	itemID = 0;
		CFArrayRef			flavorArray = nullptr;
		CFIndex				numberOfFlavors = 0;
		
		
		error = PasteboardGetItemIdentifier(inDataSourceOrNull, itemIndex, &itemID);
		assert_noerr(error);
		
		error = PasteboardCopyItemFlavors(inDataSourceOrNull, itemID, &flavorArray);
		assert_noerr(error);
		
		numberOfFlavors = CFArrayGetCount(flavorArray);
		
		for (CFIndex flavorIndex = 0; flavorIndex < numberOfFlavors; ++flavorIndex)
		{
			CFStringRef		flavorType = nullptr;
			Boolean			typeConforms = false;
			
			
			flavorType = CFUtilities_StringCast(CFArrayGetValueAtIndex(flavorArray, flavorIndex));
			typeConforms = UTTypeConformsTo(flavorType, inUTI);
			if (typeConforms)
			{
				result = true;
				outConformingItemActualType = flavorType;
				CFRetain(outConformingItemActualType);
				outConformingItemID = itemID;
				break;
			}
		}
		
		CFRelease(flavorArray), flavorArray = nullptr;
	}
	return result;
}// Contains


/*!
Returns true only if the specified pasteboard contains some
supported type of image data.

If the given pasteboard has changed, or has never been used
with this module before, you must first invoke the routine
Clipboard_SetPasteboardModified(), which will register and
cache the pasteboard’s properties.  However, this is not
necessary for the primary pasteboard, as changes to it are
automatically detected.

For convenience, if you specify nullptr for the source, then
Clipboard_ReturnPrimaryPasteboard() is used.

(3.1)
*/
Boolean
Clipboard_ContainsGraphics	(PasteboardRef		inDataSourceOrNull)
{
	Boolean		result = false;
	
	
	if (nullptr == inDataSourceOrNull)
	{
		inDataSourceOrNull = Clipboard_ReturnPrimaryPasteboard();
	}
	assert(nullptr != inDataSourceOrNull);
	
	if (gClipboardDataGeneralTypes().end() != gClipboardDataGeneralTypes().find(inDataSourceOrNull))
	{
		result = (kMy_TypeGraphics == gClipboardDataGeneralTypes()[inDataSourceOrNull]);
	}
	return result;
}// ContainsGraphics


/*!
Returns true only if the specified pasteboard contains a
string of text.

If the given pasteboard has changed, or has never been used
with this module before, you must first invoke the routine
Clipboard_SetPasteboardModified(), which will register and
cache the pasteboard’s properties.  However, this is not
necessary for the primary pasteboard, as changes to it are
automatically detected.

For convenience, if you specify nullptr for the source, then
Clipboard_ReturnPrimaryPasteboard() is used.

(3.1)
*/
Boolean
Clipboard_ContainsText	(PasteboardRef		inDataSourceOrNull)
{
	Boolean		result = false;
	
	
	if (nullptr == inDataSourceOrNull)
	{
		inDataSourceOrNull = Clipboard_ReturnPrimaryPasteboard();
	}
	assert(nullptr != inDataSourceOrNull);
	
	if (gClipboardDataGeneralTypes().end() != gClipboardDataGeneralTypes().find(inDataSourceOrNull))
	{
		result = (kMy_TypeText == gClipboardDataGeneralTypes()[inDataSourceOrNull]);
	}
	return result;
}// ContainsText


/*!
Returns true only if the specified pasteboard contains text
that was successfully converted.  This includes any data that
might be converted into text, such as converting a file or
directory into an escaped file system path.

When there is more than one item on the pasteboard, all text
from all items is combined.  For file URL items, each is
joined by spaces; for other forms of text, items are joined
with new-lines.

When successful (returning true), the "outCFString" and
"outUTI" will both be defined and you must call CFRelease()
on them when finished.  Otherwise, neither will be defined.

This routine is aware of several Unicode variants and other
common return types, and as a last resort calls upon
Translation Services.  However, even translation will fail
if the system does not have a way to handle the required
conversion (which is to Unicode, from whatever is given).

(3.1)
*/
Boolean
Clipboard_CreateCFStringFromPasteboard	(CFStringRef&		outCFString,
										 CFStringRef&		outUTI,
										 PasteboardRef		inPasteboardOrNull)
{
	PasteboardRef const		kPasteboard = (nullptr == inPasteboardOrNull)
											? Clipboard_ReturnPrimaryPasteboard()
											: inPasteboardOrNull;
	ItemCount				totalItems = 0;
	OSStatus				error = noErr;
	Boolean					result = false;
	
	
	outCFString = nullptr; // initially...
	outUTI = nullptr; // initially...
	
	// the data could be out of date if another application changed it...resync
	(PasteboardSyncFlags)PasteboardSynchronize(kPasteboard);
	
	error = PasteboardGetItemCount(inPasteboardOrNull, &totalItems);
	if (totalItems > 0)
	{
		// assemble the text representations of all items into one string
		CFMutableStringRef		allItems = CFStringCreateMutable(kCFAllocatorDefault, 0/* length limit */);
		
		
		assert(nullptr != allItems);
		for (ItemCount i = 1; i <= totalItems; ++i)
		{
			PasteboardItemID	itemID = 0;
			CFStringRef			thisCFString = nullptr; // contract is to always release this at the end, if defined
			CFStringRef			thisDelimiter = nullptr; // contract is to never release this string
			
			
			if (Clipboard_Contains(FUTURE_SYMBOL(CFSTR("public.plain-text"), kUTTypePlainText), i/* which item */,
									outUTI, itemID, kPasteboard))
			{
				CFDataRef	textData = nullptr;
				
				
				// text buffers are separated by nothing
				thisDelimiter = CFSTR("");
				
				error = PasteboardCopyItemFlavorData(kPasteboard, itemID, outUTI, &textData);
				if (noErr == error)
				{
					Boolean		translationOK = false;
					
					
					// try anything that works, as long as previous translations fail
					if ((false == translationOK) && UTTypeConformsTo(outUTI, FUTURE_SYMBOL(CFSTR("public.utf16-external-plain-text"),
																							kUTTypeUTF16ExternalPlainText)))
					{
						thisCFString = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, textData,
																				kCFStringEncodingUnicode);
						translationOK = (nullptr != thisCFString);
					}
					if ((false == translationOK) && UTTypeConformsTo(outUTI, FUTURE_SYMBOL(CFSTR("public.utf16-plain-text"),
																							kUTTypeUTF16PlainText)))
					{
						thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																CFDataGetLength(textData), kCFStringEncodingUnicode,
																false/* is external */);
						translationOK = (nullptr != thisCFString);
					}
					if ((false == translationOK) && UTTypeConformsTo(outUTI, FUTURE_SYMBOL(CFSTR("public.utf8-plain-text"),
																							kUTTypeUTF8PlainText)))
					{
						thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																CFDataGetLength(textData), kCFStringEncodingUTF8,
																false/* is external */);
						translationOK = (nullptr != thisCFString);
					}
					if ((false == translationOK) && UTTypeConformsTo(outUTI, CFSTR("com.apple.traditional-mac-plain-text")))
					{
						thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																CFDataGetLength(textData), kCFStringEncodingUTF8,
																false/* is external */);
						translationOK = (nullptr != thisCFString);
					}
					if ((false == translationOK) && UTTypeConformsTo(outUTI, FUTURE_SYMBOL(CFSTR("public.plain-text"),
																							kUTTypePlainText)))
					{
						thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																CFDataGetLength(textData), kCFStringEncodingUTF8,
																false/* is external */);
						translationOK = (nullptr != thisCFString);
					}
					if (false == translationOK)
					{
						TranslationRef		translationInfo = nullptr;
						
						
						// don’t give up just yet...attempt to translate this data into something presentable
						thisCFString = nullptr;
						error = TranslationCreate(outUTI, FUTURE_SYMBOL(CFSTR("public.utf16-plain-text"), kUTTypeUTF16PlainText),
													kTranslationDataTranslation, &translationInfo);
						if (noErr == error)
						{
							CFDataRef	translatedData = nullptr;
							
							
							error = TranslationPerformForData(translationInfo, textData, &translatedData);
							if (noErr == error)
							{
								thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(translatedData),
																		CFDataGetLength(translatedData), kCFStringEncodingUnicode,
																		false/* is external */);
								CFRelease(translatedData), translatedData = nullptr;
							}
							CFRelease(translationInfo), translationInfo = nullptr;
						}
						
						if (nullptr == thisCFString)
						{
							// WARNING: in this case, the encoding cannot be known, so choose to show nothing
							Console_Warning(Console_WriteValueCFString, "unknown text encoding and unable to translate", outUTI);
							thisCFString = CFSTR("?");
							CFRetain(thisCFString);	
						}
					}
					CFRelease(textData), textData = nullptr;
				}
			}
			else if (Clipboard_Contains(FUTURE_SYMBOL(CFSTR("public.file-url"), kUTTypeFileURL), i/* which item */,
										outUTI, itemID, kPasteboard))
			{
				CFDataRef	textData = nullptr;
				
				
				// file paths are separated by whitespace
				thisDelimiter = CFSTR("  ");
				
				error = PasteboardCopyItemFlavorData(kPasteboard, itemID, outUTI, &textData);
				if (noErr == error)
				{
					CFRetainRelease		urlString(CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																			CFDataGetLength(textData), kCFStringEncodingUTF8,
																			false/* is external */),
													true/* is retained */);
					
					
					if (urlString.exists())
					{
						CFRetainRelease		urlObject(CFURLCreateWithString(kCFAllocatorDefault, urlString.returnCFStringRef(),
																			nullptr/* base URL */),
														true/* is retained */);
						
						
						if (urlObject.exists())
						{
							CFURLRef const		kURL = CFUtilities_URLCast(urlObject.returnCFTypeRef());
							UInt8				pathBuffer[MAXPATHLEN];
							
							
							// the API does not specify, but the implication is that the returned
							// buffer must always be null-terminated; also, the similar API
							// CFStringGetFileSystemRepresentation() *does* specify null-termination
							if (CFURLGetFileSystemRepresentation(kURL, true/* resolve against base */, pathBuffer, sizeof(pathBuffer)))
							{
								CFRetainRelease		pathCFString(CFStringCreateWithCString(kCFAllocatorDefault,
																							REINTERPRET_CAST(pathBuffer, char const*),
																							kCFStringEncodingUTF8),
																	true/* is retained */);
								
								
								if (pathCFString.exists())
								{
									CFRetainRelease		workArea(CFStringCreateMutableCopy(kCFAllocatorDefault, 0/* length limit */,
																							pathCFString.returnCFStringRef()),
																	false/* is retained; used to set return value released by caller */);
									
									
									if (workArea.exists())
									{
										CFMutableStringRef const	kMutableCFString = workArea.returnCFMutableStringRef();
										
										
										// escape any characters that will screw up a typical Unix shell;
										// TEMPORARY, INCOMPLETE - only escapes spaces, there might be
										// other characters that are important to handle here
										(CFIndex)CFStringFindAndReplace(kMutableCFString, CFSTR(" "), CFSTR("\\ "),
																		CFRangeMake(0, CFStringGetLength(kMutableCFString)),
																		0/* comparison flags */);
										thisCFString = kMutableCFString;
									}
								}
							}
						}
					}
					CFRelease(textData), textData = nullptr;
				}
			}
			
			if (nullptr != thisCFString)
			{
				result = true;
				CFStringAppend(allItems, thisCFString);
				if (nullptr != thisDelimiter)
				{
					CFStringAppend(allItems, thisDelimiter);
				}
				CFRelease(thisCFString), thisCFString = nullptr;
			}
		}
		
		if (result)
		{
			// ownership transfers to caller
			outCFString = allItems;
		}
		else
		{
			CFRelease(allItems), allItems = nullptr;
		}
	}
	return result;
}// CreateCFStringFromPasteboard


/*!
Returns true only if some item on the specified pasteboard
is an image.

You must CFRelease() the UTI string.

This routine is aware of several image types, and relies on
QuickTime to handle as many kinds of images as possible.

(3.1)
*/
Boolean
Clipboard_CreateCGImageFromPasteboard	(CGImageRef&		outImage,
										 CFStringRef&		outUTI,
										 PasteboardRef		inPasteboardOrNull)
{
	PasteboardRef const		kPasteboard = (nullptr == inPasteboardOrNull)
											? Clipboard_ReturnPrimaryPasteboard()
											: inPasteboardOrNull;
	PasteboardItemID		itemID = 0;
	OSStatus				error = noErr;
	Boolean					result = false;
	
	
	if (Clipboard_Contains(FUTURE_SYMBOL(CFSTR("public.image"), kUTTypeImage), 0/* which item, or zero to check them all */,
							outUTI, itemID, kPasteboard))
	{
		CFDataRef	imageData = nullptr;
		
		
		error = PasteboardCopyItemFlavorData(kPasteboard, itemID, outUTI, &imageData);
		if (noErr == error)
		{
			error = createCGImageFromData(imageData, outImage);
			result = (noErr == error);
			CFRelease(imageData), imageData = nullptr;
		}
	}
	return result;
}// CreateCGImageFromPasteboard


/*!
Creates an appropriate Apple Event descriptor
containing the data currently on the clipboard
(if any).

(3.0)
*/
OSStatus
Clipboard_CreateContentsAEDesc		(AEDesc*	outDescPtr)
{
	OSStatus	result = noErr;
	
	
	if (outDescPtr != nullptr)
	{
		CFStringRef		clipboardTextCFString = nullptr;
		CFStringRef		actualUTI = nullptr;
		
		
		outDescPtr->descriptorType = typeNull;
		outDescPtr->dataHandle = nullptr;
		if (Clipboard_CreateCFStringFromPasteboard(clipboardTextCFString, actualUTI))
		{
			CFIndex const	kStringLength = CFStringGetLength(clipboardTextCFString);
			UniChar*		buffer = new UniChar[kStringLength];
			
			
			CFStringGetCharacters(clipboardTextCFString, CFRangeMake(0, kStringLength), buffer);
			result = AECreateDesc(typeUnicodeText, buffer, kStringLength * sizeof(UniChar), outDescPtr);
			delete [] buffer, buffer = nullptr;
			CFRelease(clipboardTextCFString), clipboardTextCFString = nullptr;
			CFRelease(actualUTI), actualUTI = nullptr;
		}
	}
	else result = memPCErr;
	
	return result;
}// CreateContentsAEDesc


/*!
Returns true only if the data from the specified source (or the
primary pasteboard, if nullptr is given) meets the given filter
criteria.

If true is returned, the copied data is returned, along with
information about its actual type and where it came from.  You
must CFRelease() the data and the type name string.

The source is first synchronized before checking its contents.

For convenience, if you specify nullptr for the source, then
Clipboard_ReturnPrimaryPasteboard() is used.

(3.1)
*/
Boolean
Clipboard_GetData	(Clipboard_DataConstraint	inConstraint,
					 CFDataRef&					outData,
					 CFStringRef&				outConformingItemActualType,
					 PasteboardItemID&			outConformingItemID,
					 PasteboardRef				inDataSourceOrNull)
{
	Boolean			result = false;
	Boolean			isText = false;
	CFStringRef		actualTypeName = nullptr;
	
	
	if (nullptr == inDataSourceOrNull)
	{
		inDataSourceOrNull = Clipboard_ReturnPrimaryPasteboard();
	}
	assert(nullptr != inDataSourceOrNull);
	
	// NOTE: This returns only the first text format available.
	isText = Clipboard_Contains(FUTURE_SYMBOL(CFSTR("public.plain-text"), kUTTypePlainText), 0/* which item, or zero to check them all */,
								actualTypeName, outConformingItemID, inDataSourceOrNull);
	if (isText)
	{
		Boolean		copyData = false;
		
		
		// not all data will have a recognizable format; handle whatever is reasonable
		//Console_WriteValueCFString("clipboard actually contains", actualTypeName);
		if ((false == copyData) && (inConstraint & kClipboard_DataConstraintText8Bit))
		{
			if ((kCFCompareEqualTo == CFStringCompare
										(actualTypeName, FUTURE_SYMBOL(CFSTR("public.plain-text"), kUTTypePlainText),
											0/* flags */)) ||
				(kCFCompareEqualTo == CFStringCompare
										(actualTypeName, FUTURE_SYMBOL(CFSTR("public.utf8-plain-text"), kUTTypeUTF8PlainText),
											0/* flags */)) ||
				(kCFCompareEqualTo == CFStringCompare
										(actualTypeName, CFSTR("com.apple.traditional-mac-plain-text"),
											0/* flags */)))
			{
				// text that can be iterated over by byte (8-bit)
				copyData = true;
			}
		}
		if ((false == copyData) && (inConstraint & kClipboard_DataConstraintText16BitNative))
		{
			if (kCFCompareEqualTo == CFStringCompare
										(actualTypeName, FUTURE_SYMBOL(CFSTR("public.utf16-plain-text"), kUTTypeUTF16PlainText),
											0/* flags */))
			{
				// text that can be iterated over by word (16-bit) in native byte order
				copyData = true;
			}
		}
		
		// copy the data if it meets the criteria
		if (copyData)
		{
			OSStatus	error = noErr;
			
			
			error = PasteboardCopyItemFlavorData(inDataSourceOrNull, outConformingItemID, actualTypeName, &outData);
			if (noErr == error)
			{
				result = true;
				
				// only expect the caller to release the type name once everything else succeeds!
				outConformingItemActualType = actualTypeName;
				CFRetain(outConformingItemActualType);
			}
		}
		
		CFRelease(actualTypeName), actualTypeName = nullptr;
	}
	
	return result;
}// GetData


/*!
Returns a reference to the global pasteboard, creating that
reference if necessary.

IMPORTANT:	If you use this reference to change the clipboard
			contents manually, you have to notify this module of
			your change with Clipboard_SetPasteboardModified().
			(Changes made by other applications are detected
			automatically.)

(3.1)
*/
PasteboardRef
Clipboard_ReturnPrimaryPasteboard ()
{
	static PasteboardRef	result = nullptr;
	
	
	if (nullptr == result)
	{
		OSStatus	error = noErr;
		
		
		error = PasteboardCreate(kPasteboardClipboard, &result);
		assert_noerr(error);
		assert(nullptr != result);
	}
	return result;
}// ReturnPrimaryPasteboard


/*!
Reads and caches information about the specified pasteboard,
and if the pasteboard is actually the Clipboard, triggers an
update of the Clipboard window.

IMPORTANT:	For efficiency, this routine is not automatically
			called for queries such as Clipboard_ContainsText().
			You must call this routine yourself at least once
			per pasteboard you use, and also whenever those
			pasteboards change, for query results to be correct.

NOTE:	Changes made to the primary pasteboard by other
		applications are (eventually) found automatically.
		But changes made by this application are only found
		when this routine is called.

(3.1)
*/
void
Clipboard_SetPasteboardModified		(PasteboardRef		inWhatChangedOrNullForPrimaryPasteboard)
{
	PasteboardRef const		kPasteboard = (nullptr == inWhatChangedOrNullForPrimaryPasteboard)
											? Clipboard_ReturnPrimaryPasteboard()
											: inWhatChangedOrNullForPrimaryPasteboard;
	
	
	updateClipboard(kPasteboard);
}// SetPasteboardModified


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
						 PasteboardRef			inDataTargetOrNull)
{
	CFStringRef		textToCopy = nullptr;			// where to store the characters
	short			tableThreshold = 0;		// zero for normal, nonzero for copy table mode
	
	
	if (nullptr == inDataTargetOrNull)
	{
		inDataTargetOrNull = Clipboard_ReturnPrimaryPasteboard();
	}
	
	if (inHowToCopy & kClipboard_CopyMethodTable)
	{
		size_t	actualSize = 0L;
		
		
		// get the user’s “one tab equals X spaces” preference, if possible
		unless (Preferences_GetData(kPreferences_TagCopyTableThreshold,
									sizeof(tableThreshold), &tableThreshold,
									&actualSize) == kPreferences_ResultOK)
		{
			tableThreshold = 4; // default to 4 spaces per tab if no preference can be found
		}
	}
	
	// get the highlighted characters; assume that text should be inlined unless selected in rectangular mode
	{
		TerminalView_TextFlags	flags = (inHowToCopy & kClipboard_CopyMethodInline) ? kTerminalView_TextFlagInline : 0;
		
		
		textToCopy = TerminalView_ReturnSelectedTextAsNewUnicode(inView, tableThreshold, flags);
	}
	
	if (nullptr != textToCopy)
	{
		if (CFStringGetLength(textToCopy) > 0)
		{
			(OSStatus)Clipboard_AddCFStringToPasteboard(textToCopy, inDataTargetOrNull);
		}
		CFRelease(textToCopy), textToCopy = nullptr;
	}
	
	gClipboardLocalChanges()[inDataTargetOrNull] = true;
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
Since there does not appear to be an event that can be handled
to notice clipboard changes, this timer periodically polls the
system to figure out when the clipboard has changed.  If it
does change, the clipboard window is updated.

(3.1)
*/
void
clipboardUpdatesTimer	(EventLoopTimerRef	UNUSED_ARGUMENT(inTimer),
						 void*				UNUSED_ARGUMENT(inUnusedData))
{
	PasteboardRef const		kPasteboard = Clipboard_ReturnPrimaryPasteboard();
	PasteboardSyncFlags		flags = PasteboardSynchronize(kPasteboard);
	
	
	// The modification flag ONLY refers to changes made by OTHER applications.
	// Changes to the local pasteboard by MacTerm are therefore tracked in a
	// separate map.
	if ((flags & kPasteboardModified) ||
		(gClipboardLocalChanges().end() != gClipboardLocalChanges().find(kPasteboard)))
	{
		updateClipboard(kPasteboard);
		gClipboardLocalChanges().erase(kPasteboard);
	}
}// clipboardUpdatesTimer


/*!
Returns a human-readable description of the specified data type.
If not nullptr, call CFRelease() on the result when finished.

(4.0)
*/
CFStringRef
copyTypeDescription		(CFStringRef	inUTI)
{
	CFStringRef		result = nullptr;
	
	
	// for reasons which aren’t yet understood, it is very easy for
	// Panther to hang at some point after UTTypeCopyDescription()
	// is called (but before returning to application code); for
	// now, this is avoided by simply do nothing at all on 10.3.x
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	result = UTTypeCopyDescription(inUTI);
#endif
	if (nullptr == result)
	{
		if (nullptr != inUTI)
		{
			if (isTextType(inUTI))
			{
				(UIStrings_Result)UIStrings_Copy(kUIStrings_ClipboardWindowGenericKindText, result);
			}
			else if (isImageType(inUTI))
			{
				(UIStrings_Result)UIStrings_Copy(kUIStrings_ClipboardWindowGenericKindImage, result);
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
For an open connection to a graphics importer, creates
a CGImage out of the data.

(3.1)
*/
OSStatus
createCGImageFromComponentConnection	(GraphicsImportComponent	inImporter,
										 CGImageRef&				outImage)
{
	OSStatus	result = noErr;
	Rect		imageBounds;
	
	
	result = GraphicsImportGetNaturalBounds(inImporter, &imageBounds);
	if (noErr == result)
	{
		size_t const	kWidth = imageBounds.right - imageBounds.left;
		size_t const	kHeight = imageBounds.bottom - imageBounds.top;
		size_t const	kBitsPerComponent = 8;
		size_t const	kBytesPerPixel = 4;
		size_t const	kBitsPerPixel = kBitsPerComponent * kBytesPerPixel;
		size_t const	kRowBytes = kWidth * kBytesPerPixel;
		size_t const	kImageSize = kHeight * kRowBytes;
		Ptr				dataPtr = Memory_NewPtr(kImageSize);
		
		
		if (nullptr == dataPtr) result = memFullErr;
		else
		{
			GWorldPtr	gWorld = nullptr;
			QDErr		quickDrawError = 0;
			
			
			quickDrawError = NewGWorldFromPtr(&gWorld, kBitsPerPixel, &imageBounds, nullptr/* color table */,
												nullptr/* device */, 0/* flags */, dataPtr, kRowBytes);
			if (nullptr == gWorld) result = memFullErr;
			else
			{
				result = GraphicsImportSetGWorld(inImporter, gWorld, GetGWorldDevice(gWorld));
				
				
				if (noErr == result)
				{
					result = GraphicsImportDraw(inImporter);
					if (noErr == result)
					{
						CGDataProviderRef	provider = CGDataProviderCreateWithData(nullptr/* info */, dataPtr, kImageSize,
																					disposeImporterImageBuffer);
						
						
						if (nullptr == provider) result = memFullErr;
						else
						{
							CGColorSpaceRef		colorspace = CGColorSpaceCreateDeviceRGB();
							
							
							if (nullptr == colorspace) result = memFullErr;
							else
							{
								outImage = CGImageCreate(kWidth, kHeight, kBitsPerComponent, kBitsPerPixel, kRowBytes,
															colorspace, kCGImageAlphaFirst, provider, nullptr/* decode */,
															false/* interpolate */, kCGRenderingIntentDefault);
								if (nullptr == outImage) result = memFullErr;
								else
								{
									// success!
									result = noErr;
								}
								CFRelease(colorspace), colorspace = nullptr;
							}
							CFRelease(provider), provider = nullptr;
						}
					}
				}
				DisposeGWorld(gWorld), gWorld = nullptr;
			}
		}
	}
	return result;
}// createCGImageFromComponentConnection


/*!
Uses QuickTime to import image data that was presumably
supplied by a pasteboard.

(3.1)
*/
OSStatus
createCGImageFromData	(CFDataRef		inData,
						 CGImageRef&	outImage)
{
	OSStatus	result = noErr;
	Handle		pointerDataHandle = Memory_NewHandle(sizeof(PointerDataRefRecord));
	
	
    if (nullptr == pointerDataHandle) result = memFullErr;
	else
	{
		PointerDataRef				pointerDataRef = REINTERPRET_CAST(pointerDataHandle, PointerDataRef);
		GraphicsImportComponent		importer = nullptr;
		
		
		(**pointerDataRef).data = CONST_CAST(CFDataGetBytePtr(inData), UInt8*);
		(**pointerDataRef).dataLength = CFDataGetLength(inData);
		
		result = GetGraphicsImporterForDataRef(pointerDataHandle, PointerDataHandlerSubType, &importer);
		if (noErr == result)
		{
			result = createCGImageFromComponentConnection(importer, outImage);
			CloseComponent(importer);
		}
		Memory_DisposeHandle(&pointerDataHandle), pointerDataRef = nullptr;
	}
	return result;
}// createCGImageFromData


/*!
Reverses the allocation done by
createCGImageFromComponentConnection().

(3.1)
*/
void
disposeImporterImageBuffer	(void*			UNUSED_ARGUMENT(inInfo),
							 void const*	inData,
							 size_t			UNUSED_ARGUMENT(inSize))
{
	DisposePtr(REINTERPRET_CAST(CONST_CAST(inData, void*), Ptr));
}// disposeImporterImageBuffer


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
slower and more accurate method may be to construct a
string, using Clipboard_CreateCFStringFromPasteboard().

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
Copies the specified PICT (QuickDraw picture) data
to the clipboard.

(3.0)
*/
void
pictureToScrap	(Handle		inPictureData)
{
	SInt8	hState = HGetState(inPictureData);
	
	
	HLock((Handle)inPictureData);
	(OSStatus)ClearCurrentScrap();
	{
		ScrapRef	currentScrap = nullptr;
		
		
		if (GetCurrentScrap(&currentScrap) == noErr)
		{
			(OSStatus)PutScrapFlavor(currentScrap, kScrapFlavorTypePicture,
										kScrapFlavorMaskNone, GetHandleSize(inPictureData),
										*inPictureData);
		}
	}
	HSetState(inPictureData, hState);
}// pictureToScrap


/*!
Copies the specified plain text data to the clipboard.

DEPRECATED, use Clipboard_AddCFStringToPasteboard().

(3.0)
*/
void
textToScrap		(Handle		inTextHandle)
{
	(OSStatus)ClearCurrentScrap();
	{
		ScrapRef	currentScrap = nullptr;
		
		
		if (GetCurrentScrap(&currentScrap) == noErr)
		{
			(OSStatus)PutScrapFlavor(currentScrap, kScrapFlavorTypeText,
										kScrapFlavorMaskNone, GetHandleSize(inTextHandle),
										*inTextHandle);
		}
	}
}// textToScrap


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
updateClipboard		(PasteboardRef		inPasteboard)
{
	Clipboard_WindowController*		controller = (Clipboard_WindowController*)
													[Clipboard_WindowController sharedClipboardWindowController];
	CGImageRef						imageToRender = nullptr;
	CFStringRef						textToRender = nullptr;
	CFStringRef						typeIdentifier = nullptr;
	CFStringRef						typeCFString = nullptr;
	
	
	if (Clipboard_CreateCFStringFromPasteboard(textToRender, typeIdentifier, inPasteboard))
	{
		// text
		typeCFString = copyTypeDescription(typeIdentifier);
		gClipboardDataGeneralTypes()[inPasteboard] = kMy_TypeText;
		if (Clipboard_ReturnPrimaryPasteboard() == inPasteboard)
		{
			gCurrentRenderData = textToRender;
			
			[controller->clipboardTextContent setEditable:YES];
			[controller->clipboardTextContent setString:@""];
			{
				// since the previous content may have been styled text, try
				// to reset as much as possible (font, color, etc.)
				[controller->clipboardTextContent setFont:[controller returnNSFontForMonospacedTextOfSize:14/* arbitrary */]];
				[controller->clipboardTextContent setTextColor:[NSColor textColor]];
			}
		#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
			[controller->clipboardTextContent paste:NSApp];
		#else
			// on Panther, pasting does not seem to work; so, just do a
			// raw text dump (not as pretty, but better than nothing)
			[controller->clipboardTextContent setString:(NSString*)textToRender];
		#endif
			[controller->clipboardTextContent setEditable:NO];
			[controller setShowImage:NO];
			[controller setShowText:YES];
			[controller setKindField:(NSString*)typeCFString];
			[controller setDataSize:(CFStringGetLength(textToRender) * sizeof(UniChar))]; // TEMPORARY; this is probably not always right
			[controller setLabel1:nil andValue:nil];
			[controller setLabel2:nil andValue:nil];
			[controller setNeedsDisplay];
		}
		CFRelease(textToRender), textToRender = nullptr;
		CFRelease(typeIdentifier), typeIdentifier = nullptr;
	}
	else if (Clipboard_CreateCGImageFromPasteboard(imageToRender, typeIdentifier, inPasteboard))
	{
		// image
		typeCFString = copyTypeDescription(typeIdentifier);
		gClipboardDataGeneralTypes()[inPasteboard] = kMy_TypeGraphics;
		if (Clipboard_ReturnPrimaryPasteboard() == inPasteboard)
		{
			NSPasteboard*	generalPasteboard = [NSPasteboard generalPasteboard];
			
			
			gCurrentRenderData.clear();
			
			[controller->clipboardImageContent setImage:[[[NSImage alloc] initWithPasteboard:generalPasteboard] autorelease]];
			[controller setShowImage:YES];
			[controller setShowText:NO];
			[controller setKindField:(NSString*)typeCFString];
			[controller setDataSize:(CGImageGetHeight(imageToRender) * CGImageGetBytesPerRow(imageToRender))];
			[controller setDataWidth:CGImageGetWidth(imageToRender) andHeight:CGImageGetHeight(imageToRender)];
			[controller setNeedsDisplay];
		}
		CFRelease(imageToRender), imageToRender = nullptr;
		CFRelease(typeIdentifier), typeIdentifier = nullptr;
	}
	else
	{
		// unknown, or empty
		typeCFString = copyTypeDescription(typeIdentifier);
		gClipboardDataGeneralTypes()[inPasteboard] = kMy_TypeUnknown;
		if (Clipboard_ReturnPrimaryPasteboard() == inPasteboard)
		{
			CFStringRef		unknownCFString = nullptr;
			
			
			gCurrentRenderData.clear();
			
			[controller setShowImage:NO];
			[controller setShowText:NO];
			[controller setKindField:(NSString*)typeCFString];
			if (UIStrings_Copy(kUIStrings_ClipboardWindowValueUnknown, unknownCFString).ok())
			{
				[controller setSizeField:(NSString*)unknownCFString];
				
				if ((nullptr == typeCFString) || (0 == CFStringGetLength(typeCFString)))
				{
					[controller setKindField:(NSString*)unknownCFString];
				}
				
				CFRelease(unknownCFString), unknownCFString = nullptr;
			}
			else
			{
				[controller setDataSize:0];
			}
			[controller setLabel1:nil andValue:nil];
			[controller setLabel2:nil andValue:nil];
			[controller setNeedsDisplay];
		}
	}
	if (nullptr != typeCFString)
	{
		CFRelease(typeCFString), typeCFString = nullptr;
	}
}// updateClipboard

} // anonymous namespace


@implementation Clipboard_ContentView


/*!
Constructor.

(4.0)
*/
- (id)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		self->showDragHighlight = NO;
		
		// the list of accepted drag text types should correspond with what
		// Clipboard_CreateCFStringFromPasteboard() will actually support
		[self registerForDraggedTypes:[NSArray arrayWithObjects:
													(NSString*)FUTURE_SYMBOL(CFSTR("public.utf16-external-plain-text"),
																				kUTTypeUTF16ExternalPlainText),
													(NSString*)FUTURE_SYMBOL(CFSTR("public.utf16-plain-text"),
																				kUTTypeUTF16PlainText),
													(NSString*)FUTURE_SYMBOL(CFSTR("public.utf8-plain-text"),
																				kUTTypeUTF8PlainText),
													(NSString*)FUTURE_SYMBOL(CFSTR("public.plain-text"),
																				kUTTypePlainText),
													(NSString*)CFSTR("com.apple.traditional-mac-plain-text"),
													nil]];
	}
	return self;
}// initWithFrame:


- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (BOOL)
showDragHighlight
{
	return showDragHighlight;
}
- (void)
setShowDragHighlight:(BOOL)		flag
{
	showDragHighlight = flag;
}// setShowDragHighlight:


#pragma mark NSControl


/*!
Draws the separator on the edge of clipboard content,
as well as any applicable drag highlights.

(4.0)
*/
- (void)
drawRect:(NSRect)	rect
{
	Boolean const		kHasOverlayScrollBars = (NULL != class_getClassMethod([NSScroller class], @selector(preferredScrollerStyle)))
												? (NSScrollerStyleOverlay == [NSScroller preferredScrollerStyle])
												: false;
	Float32 const		kHorizontalScrollBarWidth = (kHasOverlayScrollBars)
														? 0
														: 15.0; // arbitrary; TEMPORARY, should retrieve dynamically from somewhere
	Float32 const		kVerticalScrollBarWidth = (kHasOverlayScrollBars)
													? 0
													: 15.0; // arbitrary; TEMPORARY, should retrieve dynamically from somewhere
	NSGraphicsContext*	contextMgr = [NSGraphicsContext currentContext];
	CGContextRef		drawingContext = (CGContextRef)[contextMgr graphicsPort];
	NSRect				entireRect = [self bounds];
	CGRect				contentBounds;
	
	
	[super drawRect:rect];
	
	// paint entire background first
	contentBounds = CGRectMake(entireRect.origin.x, entireRect.origin.y, entireRect.size.width, entireRect.size.height);
	CGContextSetRGBFillColor(drawingContext, 1.0, 1.0, 1.0, 1.0/* alpha */);
	CGContextFillRect(drawingContext, contentBounds);
	
	// now define the region that excludes the vertical separator and scroll bar
	// (so that an appropriate drag highlight appears around the logical border)
	contentBounds.origin.x += kSeparatorWidth;
	contentBounds.origin.y += kHorizontalScrollBarWidth;
	contentBounds.size.width -= (kSeparatorWidth + kVerticalScrollBarWidth);
	contentBounds.size.height -= kHorizontalScrollBarWidth;
	
	// perform any necessary rendering for drags
	if ([self showDragHighlight])
	{
		DragAndDrop_ShowHighlightBackground(drawingContext, contentBounds);
		DragAndDrop_ShowHighlightFrame(drawingContext, contentBounds);
	}
	else
	{
		DragAndDrop_HideHighlightBackground(drawingContext, contentBounds);
		DragAndDrop_HideHighlightFrame(drawingContext, contentBounds);
	}
	
	// draw a separator on the edge, which will be next to the splitter;
	// the scroll view should be offset slightly in the NIB, so that
	// this separator is visible while the scroll view is visible
	contentBounds = CGRectMake(entireRect.origin.x, entireRect.origin.y, entireRect.size.width, entireRect.size.height);
	contentBounds.size.width = kSeparatorWidth;
	CGContextSetRGBStrokeColor(drawingContext, 0, 0, 0, 1.0/* alpha */);
	CGContextStrokeRect(drawingContext, contentBounds);
}// drawRect


#pragma mark NSDraggingDestination


/*!
Invoked when the user has dragged data into the main content
area of the Clipboard window.

(4.0)
*/
- (NSDragOperation)
draggingEntered:(id <NSDraggingInfo>)	sender
{
	NSDragOperation		result = NSDragOperationNone;
	
	
	if ([sender draggingSourceOperationMask] & NSDragOperationCopy)
	{
		result = NSDragOperationCopy;
		
		// show a highlight area by setting the appropriate view property
		// and requesting a redraw
		[self setShowDragHighlight:YES];
		[self setNeedsDisplay];
	}
	return result;
}// draggingEntered:


/*!
Invoked when the user has dragged data out of the main content
area of the Clipboard window.

(4.0)
*/
- (void)
draggingExited:(id <NSDraggingInfo>)	sender
{
#pragma unused(sender)
	// show a highlight area by setting the appropriate view property
	// and requesting a redraw
	[self setShowDragHighlight:NO];
	[self setNeedsDisplay];
}// draggingExited:


/*!
Invoked when the user has dropped data into the main content
area of the Clipboard window.

(4.0)
*/
- (BOOL)
performDragOperation:(id <NSDraggingInfo>)		sender
{
	NSPasteboard*	dragPasteboard = [sender draggingPasteboard];
	PasteboardRef	asPasteboardRef = nullptr;
	OSStatus		error = PasteboardCreate((CFStringRef)[dragPasteboard name], &asPasteboardRef);
	BOOL			result = NO;
	
	
	// handle the drop (by copying the data to the clipboard)
	if (noErr == error)
	{
		CFStringRef		copiedTextCFString;
		CFStringRef		copiedTextUTI;
		Boolean			copyOK = Clipboard_CreateCFStringFromPasteboard
									(copiedTextCFString, copiedTextUTI, asPasteboardRef);
		
		
		if (false == copyOK)
		{
			Console_Warning(Console_WriteLine, "failed to copy the dragged text!");
			result = NO;
		}
		else
		{
			// put the text on the clipboard
			error = Clipboard_AddCFStringToPasteboard(copiedTextCFString, Clipboard_ReturnPrimaryPasteboard());
			if (noErr == error)
			{
				// force a view update, as obviously it is now out of date
				updateClipboard(Clipboard_ReturnPrimaryPasteboard());
				
				// success!
				result = YES;
			}
			CFRelease(copiedTextCFString), copiedTextCFString = nullptr;
			CFRelease(copiedTextUTI), copiedTextUTI = nullptr;
		}
		CFRelease(asPasteboardRef), asPasteboardRef = nullptr;
	}
	
	return result;
}// performDragOperation:


/*!
Invoked when the user is about to drop data into the content
area of the Clipboard window; returns YES only if the data is
acceptable.

(4.0)
*/
- (BOOL)
prepareForDragOperation:(id <NSDraggingInfo>)	sender
{
#pragma unused(sender)
	BOOL	result = NO;
	
	
	[self setShowDragHighlight:NO];
	[self setNeedsDisplay];
	
	// always accept the drag
	result = YES;
	
	return result;
}// prepareForDragOperation:


#pragma mark NSView


/*!
Returns YES only if the view has no transparency at all.

(4.0)
*/
- (BOOL)
isOpaque
{
	return YES;
}// isOpaque


@end // Clipboard_ContentView


@implementation Clipboard_WindowController


static Clipboard_WindowController*	gClipboard_WindowController = nil;


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedClipboardWindowController
{
	if (nil == gClipboard_WindowController)
	{
		gClipboard_WindowController = [[[self class] allocWithZone:NULL] init];
	}
	return gClipboard_WindowController;
}// sharedClipboardWindowController


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"ClipboardCocoa"];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Returns a system-managed NSFont object that represents
the user’s default terminal font at the specified size.
This should be reasonable for displaying plain text in
the Clipboard window.

(4.0)
*/
- (NSFont*)
returnNSFontForMonospacedTextOfSize:(unsigned int)	fontSize
{
	NSFont*				result = nil;
	Str255				fontName;
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_GetData(kPreferences_TagFontName, sizeof(fontName), fontName);
	if (kPreferences_ResultOK == prefsResult)
	{
		CFRetainRelease		fontNameCFString(CFStringCreateWithPascalString(kCFAllocatorDefault,
																			fontName, kCFStringEncodingMacRoman),
												true/* is retained */);
		
		
		result = [NSFont fontWithName:(NSString*)fontNameCFString.returnCFStringRef() size:fontSize];
	}
	return result;
}// returnNSFontForMonospacedTextOfSize:


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
	[self setSizeField:(NSString*)finalCFString];
	
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
				[self setLabel1:(NSString*)widthCFString andValue:(NSString*)finalCFString];
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
				[self setLabel2:(NSString*)heightCFString andValue:(NSString*)finalCFString];
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
	[valueKind setStringValue:aString];
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
		[label1 setStringValue:labelString];
		[value1 setStringValue:valueString];
		[label1 setHidden:NO];
		[value1 setHidden:NO];
	}
	else
	{
		[label1 setStringValue:@""];
		[value1 setStringValue:@""];
		[label1 setHidden:YES];
		[value1 setHidden:YES];
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
		[label2 setStringValue:labelString];
		[value2 setStringValue:valueString];
		[label2 setHidden:NO];
		[value2 setHidden:NO];
	}
	else
	{
		[label2 setStringValue:@""];
		[value2 setStringValue:@""];
		[label2 setHidden:YES];
		[value2 setHidden:YES];
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
	[clipboardContent setNeedsDisplay];
}// setNeedsDisplay


/*!
Specifies that the current Clipboard data is graphical, and
causes a more appropriate user interface to be displayed.

(4.0)
*/
- (void)
setShowImage:(BOOL)		flag
{
	if (flag)
	{
		NSSize	imageSize = [[clipboardImageContent image] size];
		NSRect	newFrame = [clipboardContent bounds];
		
		
		newFrame.origin.x += kSeparatorPerceivedWidth;
		newFrame.size.width -= kSeparatorPerceivedWidth;
		if (NO == [clipboardImageContainer isDescendantOf:clipboardContent])
		{
			[clipboardContent addSubview:clipboardImageContainer];
		}
		[clipboardImageContainer setFrame:newFrame];
		[clipboardImageContent setFrameSize:imageSize];
		[clipboardImageContainer setHidden:NO];
	}
	else
	{
		[clipboardImageContainer removeFromSuperview];
		[clipboardImageContainer setHidden:YES];
	}
}// setShowImage:


/*!
Specifies that the current Clipboard data is textual, and
causes a more appropriate user interface to be displayed.

(4.0)
*/
- (void)
setShowText:(BOOL)		flag
{
	if (flag)
	{
		NSRect	newFrame = [clipboardContent bounds];
		
		
		newFrame.origin.x += kSeparatorPerceivedWidth;
		newFrame.size.width -= kSeparatorPerceivedWidth;
		if (NO == [clipboardTextContainer isDescendantOf:clipboardContent])
		{
			[clipboardContent addSubview:clipboardTextContainer];
		}
		[clipboardTextContainer setFrame:newFrame];
		[clipboardTextContainer setHidden:NO];
	}
	else
	{
		[clipboardTextContainer removeFromSuperview];
		[clipboardTextContainer setHidden:YES];
	}
}// setShowText:


/*!
Updates the user interface element that shows the Size of the
data on the Clipboard.

(4.0)
*/
- (void)
setSizeField:(NSString*)	aString
{
	[valueSize setStringValue:aString];
}// setSizeField:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init...".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	assert(nil != clipboardContent);
	assert(nil != clipboardImageContainer);
	assert(nil != clipboardImageContent);
	assert(nil != clipboardTextContainer);
	assert(nil != clipboardTextContent);
	assert(nil != valueKind);
	assert(nil != valueSize);
	assert(nil != label1);
	assert(nil != value1);
	assert(nil != label2);
	assert(nil != value2);
	
	[[self window] setExcludedFromWindowsMenu:YES];
	
	// the horizontal scroller is enabled in the NIB, but that has no effect
	// unless the text itself is set up to never wrap...
	//[[clipboardTextContent textContainer] setWidthTracksTextView:NO];
	
	[self setShowImage:NO];
	[self setShowText:NO];
	[self setLabel1:nil andValue:nil];
	[self setLabel2:nil andValue:nil];
}// windowDidLoad


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
