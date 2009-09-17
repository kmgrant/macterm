/*###############################################################

	Clipboard.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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

// standard-C includes
#include <climits>
#include <map>

// Unix includes
#include <sys/param.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CFUtilities.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <FlagManager.h>
#include <HIViewWrap.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "AppResources.h"
#include "Clipboard.h"
#include "Commands.h"
#include "CommonEventHandlers.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "DragAndDrop.h"
#include "EventLoop.h"
#include "Preferences.h"
#include "Session.h"
#include "TerminalView.h"
#include "UIStrings.h"
#include "VectorCanvas.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the “Window” NIB from the package "Clipboard.nib".
*/
HIViewID const	idMyLabelDataDescription	= { 'Info', 0/* ID */ };
HIViewID const	idMyUserPaneDragParent		= { 'Frme', 0/* ID */ };
HIViewID const	idMyImageData				= { 'Imag', 0/* ID */ };

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

pascal void			clipboardUpdatesTimer					(EventLoopTimerRef, void*);
OSStatus			createCGImageFromComponentConnection	(GraphicsImportComponent, CGImageRef&);
OSStatus			createCGImageFromData					(CFDataRef, CGImageRef&);
void				disposeImporterImageBuffer				(void*, void const*, size_t);
void				handleNewSize							(HIWindowRef, Float32, Float32, void*);
void				pictureToScrap							(Handle);
pascal OSStatus		receiveClipboardContentDraw				(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveClipboardContentDragDrop			(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveWindowClosing					(EventHandlerCallRef, EventRef, void*);
void				setDataTypeInformation					(CFStringRef, size_t);
void				setScalingInformation					(size_t, size_t);
void				textToScrap								(Handle);
void				updateClipboard							(PasteboardRef);

} // anonymous namespace

#pragma mark Variables
namespace {

My_TypeByPasteboard&					gClipboardDataGeneralTypes ()	{ static My_TypeByPasteboard x; return x; }
HIWindowRef								gClipboardWindow = nullptr;
WindowInfo_Ref							gClipboardWindowInfo = nullptr;
Boolean									gIsShowing = false;
My_FlagByPasteboard&					gClipboardLocalChanges ()	{ static My_FlagByPasteboard x; return x; }
CFRetainRelease							gCurrentRenderData;
CommonEventHandlers_WindowResizer		gClipboardResizeHandler;
CarbonEventHandlerWrap					gClipboardWindowClosingHandler;
CarbonEventHandlerWrap					gClipboardDrawHandler;
CarbonEventHandlerWrap					gClipboardDragDropHandler;
EventLoopTimerUPP						gClipboardUpdatesTimerUPP = nullptr;
EventLoopTimerRef						gClipboardUpdatesTimer = nullptr;

} // anonymous namespace



#pragma mark Public Methods

/*!
This method sets up the initial configurations for
the clipboard window.  Call this method before
calling any other clipboard methods from this file.

Support for scroll bars has not been implemented
yet.  Nonetheless, the commented-out code for adding
the controls is here.

(3.0)
*/
void
Clipboard_Init ()
{
	HIWindowRef		clipboardWindow = nullptr;
	OSStatus		error = noErr;
	
	
	// create the clipboard window
	clipboardWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
								CFSTR("Clipboard"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	
	gClipboardWindow = clipboardWindow;
	
	// install a callback that responds to drags (which copies dragged data to the clipboard)
	{
		HIViewWrap		frameView(idMyUserPaneDragParent, clipboardWindow);
		
		
		gClipboardDragDropHandler.install(HIObjectGetEventTarget(frameView.returnHIObjectRef()), receiveClipboardContentDragDrop,
											CarbonEventSetInClass(CarbonEventClass(kEventClassControl),
																	kEventControlDragEnter, kEventControlDragWithin,
																	kEventControlDragLeave, kEventControlDragReceive),
											nullptr/* user data */);
		assert(gClipboardDragDropHandler.isInstalled());
		error = SetControlDragTrackingEnabled(frameView, true/* is drag enabled */);
		assert_noerr(error);
	}
	
	// install a callback that renders a drag highlight
	{
		HIViewWrap		frameView(idMyUserPaneDragParent, clipboardWindow);
		
		
		gClipboardDrawHandler.install(HIObjectGetEventTarget(frameView.returnHIObjectRef()), receiveClipboardContentDraw,
										CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlDraw),
										nullptr/* user data */);
		assert(gClipboardDrawHandler.isInstalled());
	}
	
	// set up image view
	{
		HIViewWrap		imageView(idMyImageData, clipboardWindow);
		
		
		HIViewSetVisible(imageView, false); // initially...
	}
	
	// set up the Window Info information
	gClipboardWindowInfo = WindowInfo_New();
	WindowInfo_SetWindowDescriptor(gClipboardWindowInfo, kConstantsRegistry_WindowDescriptorClipboard);
	WindowInfo_SetWindowPotentialDropTarget(gClipboardWindowInfo, true/* can receive data via drag-and-drop */);
	WindowInfo_SetForWindow(clipboardWindow, gClipboardWindowInfo);
	
	// specify a different title for the Dock’s menu, one which doesn’t include the application name
	{
		CFStringRef			titleCFString = nullptr;
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		
		
		stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowIconName, titleCFString);
		SetWindowAlternateTitle(clipboardWindow, titleCFString);
		CFRelease(titleCFString);
	}
	
	// Place the clipboard in the standard position indicated in the Human Interface Guidelines.
	// With themes, the window frame size cannot be guessed reliably.  Therefore, first, take
	// the difference between the left edges of the window content and structure region boxes
	// and make that the left edge of the window.
	{
		Rect		screenRect;
		Rect		structureRect;
		Rect		contentRect;
		Rect		initialBounds;
		Float32		fh = 0.0;
		Float32		fw = 0.0;
		
		
		RegionUtilities_GetPositioningBounds(clipboardWindow, &screenRect);
		(OSStatus)GetWindowBounds(clipboardWindow, kWindowContentRgn, &contentRect);
		(OSStatus)GetWindowBounds(clipboardWindow, kWindowStructureRgn, &structureRect);
		
		// use main screen size to calculate the initial clipboard size
		fh = 0.35 * (screenRect.bottom - screenRect.top);
		fw = 0.80 * (screenRect.right - screenRect.left);
		
		// define the default window size, and make the window that size initially
		SetRect(&initialBounds, screenRect.left + (contentRect.left - structureRect.left) + 4/* arbitrary */,
								screenRect.bottom - ((short)fh) - 4/* arbitrary */, 0, 0);
		SetRect(&initialBounds, initialBounds.left, initialBounds.top,
								initialBounds.left + (short)fw,
								initialBounds.top + (short)fh);
		SetWindowStandardState(clipboardWindow, &initialBounds);
		(OSStatus)SetWindowBounds(clipboardWindow, kWindowContentRgn, &initialBounds);
		
		// install a callback that responds as a window is resized
		gClipboardResizeHandler.install(clipboardWindow, handleNewSize, nullptr/* user data */,
										fw / 2/* arbitrary minimum width */,
										fh / 2/* arbitrary minimum height */,
										fw + 200/* arbitrary maximum width */,
										fh + 100/* arbitrary maximum height */);
		assert(gClipboardResizeHandler.isInstalled());
	}
	
	// install a callback that hides the clipboard instead of destroying it, when it should be closed
	gClipboardWindowClosingHandler.install(GetWindowEventTarget(clipboardWindow), receiveWindowClosing,
											CarbonEventSetInClass(CarbonEventClass(kEventClassWindow), kEventWindowClose),
											nullptr/* user data */);
	assert(gClipboardWindowClosingHandler.isInstalled());
	
	// install a timer that detects changes to the clipboard
	{
		gClipboardUpdatesTimerUPP = NewEventLoopTimerUPP(clipboardUpdatesTimer);
		assert(nullptr != gClipboardUpdatesTimerUPP);
		error = InstallEventLoopTimer(GetCurrentEventLoop(),
										kEventDurationNoWait/* seconds before timer fires the first time */,
										kEventDurationSecond * 3.0/* seconds between fires */,
										gClipboardUpdatesTimerUPP, nullptr/* user data - not used */,
										&gClipboardUpdatesTimer);
	}
	
	// restore the visible state implicitly saved at last Quit
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
		Clipboard_SetWindowVisible(windowIsVisible);
	}
	
	// enable drag-and-drop on the window!
	(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(gClipboardWindow, true);
	
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
	DisposeWindow(gClipboardWindow), gClipboardWindow = nullptr;
	WindowInfo_Dispose(gClipboardWindowInfo);
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
		CFDataRef	externalRepresentation = CFStringCreateExternalRepresentation
												(kCFAllocatorDefault, inStringToCopy, kCFStringEncodingUnicode,
													'?'/* loss byte */);
		
		
		if (nullptr == externalRepresentation) result = unicodePartConvertErr;
		else
		{
			result = PasteboardPutItemFlavor(target, (PasteboardItemID)inStringToCopy,
												FUTURE_SYMBOL(CFSTR("public.utf16-external-plain-text"), kUTTypeUTF16ExternalPlainText),
												externalRepresentation, kPasteboardFlavorNoFlags);
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

You must CFRelease() the UTI string.

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
	
	
	error = PasteboardGetItemCount(inPasteboardOrNull, &totalItems);
	assert_noerr(error);
	if (totalItems > 0)
	{
		// assemble the text representations of all items into one string
		CFMutableStringRef		allItems = CFStringCreateMutable(kCFAllocatorDefault, 0/* length limit */);
		
		
		assert(nullptr != allItems);
		outCFString = allItems; // do not release
		result = (nullptr != outCFString);
		
		for (ItemCount i = 1; i <= totalItems; ++i)
		{
			PasteboardItemID	itemID = 0;
			CFStringRef			thisCFString = nullptr; // contract is to always release this at the end, if defined
			CFStringRef			thisDelimiter = nullptr; // contract is to never release this string
			
			
			if (Clipboard_Contains(FUTURE_SYMBOL(CFSTR("public.plain-text"), kUTTypePlainText), i/* which item */,
									outUTI, itemID, kPasteboard))
			{
				CFDataRef	textData = nullptr;
				
				
				// text buffers are separated by line-feeds (Unix-style new-line)
				thisDelimiter = CFSTR("\012");
				
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
					}
					if ((false == translationOK) && UTTypeConformsTo(outUTI, FUTURE_SYMBOL(CFSTR("public.utf16-plain-text"),
																							kUTTypeUTF16PlainText)))
					{
						thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																CFDataGetLength(textData), kCFStringEncodingUnicode,
																false/* is external */);
					}
					if ((false == translationOK) && UTTypeConformsTo(outUTI, FUTURE_SYMBOL(CFSTR("public.utf8-plain-text"),
																							kUTTypeUTF8PlainText)))
					{
						thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																CFDataGetLength(textData), kCFStringEncodingUTF8,
																false/* is external */);
					}
					if ((false == translationOK) && UTTypeConformsTo(outUTI, CFSTR("com.apple.traditional-mac-plain-text")))
					{
						thisCFString = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(textData),
																CFDataGetLength(textData), kCFStringEncodingUTF8,
																false/* is external */);
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
				CFStringAppend(allItems, thisCFString);
				if (nullptr != thisDelimiter)
				{
					CFStringAppend(allItems, thisDelimiter);
				}
				CFRelease(thisCFString), thisCFString = nullptr;
			}
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
Copies the indicated drawing to the clipboard.

(2.6)
*/
void
Clipboard_GraphicsToScrap	(VectorInterpreter_ID		inGraphicID)
{
	PicHandle	picture = nullptr;
	
	
	picture = VectorCanvas_ReturnNewQuickDrawPicture(inGraphicID);
	pictureToScrap((Handle)picture);
	KillPicture(picture);
	
	gClipboardLocalChanges()[Clipboard_ReturnPrimaryPasteboard()] = true;
}// GraphicsToScrap


/*!
Returns true only if the specified plain text contains new-line
characters as defined by ASCII-compatible encodings.

See also the Unicode version of this routine.

(3.1)
*/
Boolean
Clipboard_IsOneLineInBuffer		(UInt8 const*	inTextPtr,
								 CFIndex		inLength)
{
	Boolean		result = (nullptr != inTextPtr);
	
	
	for (CFIndex i = 0; ((result) && (i < inLength)); ++i)
	{
		// NOTE: It may be useful to provide a mechanism for customizing
		//       these rules for determining what makes a “line” of text.
		if (('\r' == inTextPtr[i]) || ('\n' == inTextPtr[i]))
		{
			result = false;
		}
	}
	
	return result;
}// IsOneLineInBuffer (non-Unicode)


/*!
Returns true only if the specified Unicode text contains new-line
characters.

See also the non-Unicode version of this routine.

(3.1)
*/
Boolean
Clipboard_IsOneLineInBuffer		(UInt16 const*	inTextPtr,
								 CFIndex		inLength)
{
	Boolean		result = (nullptr != inTextPtr);
	
	
	for (CFIndex i = 0; ((result) && (i < inLength)); ++i)
	{
		// NOTE: It may be useful to provide a mechanism for customizing
		//       these rules for determining what makes a “line” of text.
		if (('\r' == inTextPtr[i]) || ('\n' == inTextPtr[i]))
		{
			result = false;
		}
	}
	
	return result;
}// IsOneLineInBuffer (Unicode)


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
Returns the Mac OS window pointer for the clipboard window.

(3.0)
*/
HIWindowRef
Clipboard_ReturnWindow ()
{
	return gClipboardWindow;
}// ReturnWindow


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
	HIWindowRef		clipboard = Clipboard_ReturnWindow();
	
	
	gIsShowing = inIsVisible;
	if (inIsVisible)
	{
		ShowWindow(clipboard);
		EventLoop_SelectBehindDialogWindows(clipboard);
	}
	else
	{
		HideWindow(clipboard);
	}
}// SetWindowVisible


/*!
Pastes text from a pasteboard into the specified session.
If the pasteboard does not have any text, this does nothing.

The modifier can be used to alter what is actually pasted.

(3.1)
*/
void
Clipboard_TextFromScrap		(SessionRef				inSession,
							 Clipboard_Modifier		inModifier,
							 PasteboardRef			inDataSourceOrNull)
{
	CFStringRef		clipboardString = nullptr;
	CFStringRef		actualTypeName = nullptr;
	
	
	if (nullptr == inDataSourceOrNull)
	{
		inDataSourceOrNull = Clipboard_ReturnPrimaryPasteboard();
	}
	
	// IMPORTANT: It may be desirable to allow customization for what
	//            identifies a line of text.  Currently, this is assumed.
	
	if (Clipboard_CreateCFStringFromPasteboard(clipboardString, actualTypeName, inDataSourceOrNull))
	{
		if (kClipboard_ModifierOneLine == inModifier)
		{
			CFMutableStringRef		mutableBuffer = CFStringCreateMutableCopy
													(kCFAllocatorDefault, 0/* capacity or 0 for no limit */, clipboardString);
			
			
			if (nullptr == mutableBuffer)
			{
				// no memory?
				Sound_StandardAlert();
			}
			else
			{
				// ignore any new-line characters in one-line mode
				(CFIndex)CFStringFindAndReplace(mutableBuffer, CFSTR("\r"), CFSTR(""),
												CFRangeMake(0, CFStringGetLength(mutableBuffer)), 0/* flags */);
				(CFIndex)CFStringFindAndReplace(mutableBuffer, CFSTR("\n"), CFSTR(""),
												CFRangeMake(0, CFStringGetLength(mutableBuffer)), 0/* flags */);
				Session_UserInputCFString(inSession, mutableBuffer, false/* send to scripts */);
				CFRelease(mutableBuffer), mutableBuffer = nullptr;
			}
		}
		else
		{
			// send the data unmodified to the session
			Session_UserInputCFString(inSession, clipboardString, false/* send to scripts */);
		}
		CFRelease(clipboardString), clipboardString = nullptr;
		CFRelease(actualTypeName), actualTypeName = nullptr;
	}
	else
	{
		// unknown or unsupported data type
		Sound_StandardAlert();
	}
}// TextFromScrap


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
Pastes whatever text is selected in the given view,
leaving a copy on the clipboard (replacing anything
there).  If nothing is selected, this routine does
nothing.

Intelligently, MacTelnet automatically uses “inline”
copy-and-paste for this type of operation, since
virtually the only reason to use “type” would be to
copy and paste a command line from the scrollback
buffer.  If a command line spans multiple lines, it
would normally have new-line sequences inserted to
preserve the formatting, which is undesirable when
pasting what “should” be seen as a single line.

(3.0)
*/
void
Clipboard_TextType	(TerminalViewRef	inSource,
					 SessionRef			inTarget)
{
	Clipboard_TextToScrap(inSource, kClipboard_CopyMethodStandard | kClipboard_CopyMethodInline);
	Clipboard_TextFromScrap(inTarget);
}// TextType


/*!
Determines if the clipboard window is showing.  This
state is not exactly the window state; specifically,
if the clipboard is suspended but set to visible,
the window will be invisible (because the application
is suspended) and yet this method will return "true".

If you need to find the definite visible state of the
clipboard window, use Clipboard_ReturnWindow() to
obtain the Mac OS window pointer and then use the
IsWindowVisible() system call.

(3.0)
*/
Boolean
Clipboard_WindowIsVisible ()
{
	return gIsShowing;
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
pascal void
clipboardUpdatesTimer	(EventLoopTimerRef	UNUSED_ARGUMENT(inTimer),
						 void*				UNUSED_ARGUMENT(inUnusedData))
{
	PasteboardRef const		kPasteboard = Clipboard_ReturnPrimaryPasteboard();
	PasteboardSyncFlags		flags = PasteboardSynchronize(kPasteboard);
	
	
	// The modification flag ONLY refers to changes made by OTHER applications.
	// Changes to the local pasteboard by MacTelnet are therefore tracked in a
	// separate map.
	if ((flags & kPasteboardModified) ||
		(gClipboardLocalChanges().end() != gClipboardLocalChanges().find(kPasteboard)))
	{
		updateClipboard(kPasteboard);
		gClipboardLocalChanges().erase(kPasteboard);
	}
}// clipboardUpdatesTimer


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
suppled by a pasteboard.

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
This does not actually do anything, but the mechanism for
installing the handler allows maximum size to be easily
enforced.  Resize is automatic through the standard handler
and settings from the NIB.

(3.0)
*/
void
handleNewSize	(WindowRef		UNUSED_ARGUMENT(inWindow),
				 Float32		UNUSED_ARGUMENT(inDeltaSizeX),
				 Float32		UNUSED_ARGUMENT(inDeltaSizeY),
				 void*			UNUSED_ARGUMENT(inData))
{
}// handleNewSize


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
			RegionUtilities_RedrawWindowOnNextUpdate(gClipboardWindow);
		}
	}
	HSetState(inPictureData, hState);
}// pictureToScrap


/*!
Embellishes "kEventControlDraw" of "kEventClassControl" by
rendering clipboard text content, and a drag highlight where
appropriate.

Invoked by Mac OS X whenever the parent user pane contents
should be redrawn.

(3.1)
*/
pascal OSStatus
receiveClipboardContentDraw	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	{
		HIViewRef	view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (noErr == result)
		{
			CGContextRef	drawingContext = nullptr;
			
			
			
			// determine the context to draw in with Core Graphics
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef,
															drawingContext);
			assert_noerr(result);
			
			// if all information can be found, proceed with drawing
			if (noErr == result)
			{
				OSStatus	error = noErr;
				HIRect		contentBounds;
				Boolean		dragHighlight = false;
				UInt32		actualSize = 0;
				
				
				error = HIViewGetBounds(view, &contentBounds);
				assert_noerr(error);
				
				// perform any necessary rendering for drags
				if (noErr != GetControlProperty
								(view, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
									sizeof(dragHighlight), &actualSize,
									&dragHighlight))
				{
					dragHighlight = false;
				}
				
				if (dragHighlight)
				{
					DragAndDrop_ShowHighlightBackground(drawingContext, contentBounds);
					DragAndDrop_ShowHighlightFrame(drawingContext, contentBounds);
				}
				else
				{
					DragAndDrop_HideHighlightBackground(drawingContext, contentBounds);
					DragAndDrop_HideHighlightFrame(drawingContext, contentBounds);
				}
				
				// render text, if that is what is on the clipboard
				if (gCurrentRenderData.exists() && (CFStringGetTypeID() == CFGetTypeID(gCurrentRenderData.returnCFTypeRef())))
				{
					HIThemeTextInfo		textInfo;
					
					
					bzero(&textInfo, sizeof(textInfo));
					textInfo.version = 0;
					textInfo.state = IsControlActive(view) ? kThemeStateActive : kThemeStateInactive;
					textInfo.fontID = kThemeSystemFont;
					textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushLeft;
					textInfo.verticalFlushness = kHIThemeTextVerticalFlushTop;
					textInfo.options = 0;
					textInfo.truncationPosition = kHIThemeTextTruncationNone;
					textInfo.truncationMaxLines = 0;
					
					error = HIThemeDrawTextBox(gCurrentRenderData.returnCFStringRef(), &contentBounds, &textInfo,
												drawingContext, kHIThemeOrientationNormal);
					assert_noerr(error);
				}
				else
				{
					CGContextSetRGBFillColor(drawingContext, 1.0, 1.0, 1.0, 1.0/* alpha */);
					CGContextFillRect(drawingContext, contentBounds);
				}
			}
		}
	}
	return result;
}// receiveClipboardContentDraw


/*!
Handles "kEventControlDragEnter", "kEventControlDragWithin",
"kEventControlDragLeave" and "kEventControlDragReceive" of
"kEventClassControl".

Invoked by Mac OS X whenever the clipboard is involved in a
drag-and-drop operation.

(3.1)
*/
pascal OSStatus
receiveClipboardContentDragDrop		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
									 EventRef				inEvent,
									 void*					UNUSED_ARGUMENT(inContextPtr))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert((kEventKind == kEventControlDragEnter) ||
			(kEventKind == kEventControlDragWithin) ||
			(kEventKind == kEventControlDragLeave) ||
			(kEventKind == kEventControlDragReceive));
	{
		HIViewRef	view = nullptr;
		
		
		// get the target control
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the control was found, continue
		if (noErr == result)
		{
			DragRef		dragRef = nullptr;
			
			
			// determine the drag taking place
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, dragRef);
			if (noErr == result)
			{
				PasteboardRef		dragPasteboard = nullptr;
				
				
				result = GetDragPasteboard(dragRef, &dragPasteboard);
				assert_noerr(result);
				
				// read and cache information about this pasteboard
				Clipboard_SetPasteboardModified(dragPasteboard);
				
				switch (kEventKind)
				{
				case kEventControlDragEnter:
					// indicate whether or not this drag is interesting
					{
						Boolean		acceptDrag = Clipboard_ContainsText(dragPasteboard);
						
						
						result = SetEventParameter(inEvent, kEventParamControlWouldAcceptDrop,
													typeBoolean, sizeof(acceptDrag), &acceptDrag);
					}
					break;
				
				case kEventControlDragWithin:
				case kEventControlDragLeave:
					// show or hide a highlight area; this is a control property so that all the
					// highlight rendering can simply be done by the drawing routine
					{
						Boolean		showHighlight = (kEventControlDragWithin == kEventKind);
						
						
						result = SetControlProperty(view, AppResources_ReturnCreatorCode(),
													kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
													sizeof(showHighlight), &showHighlight);
						// change the cursor, for additional visual feedback
						if (showHighlight)
						{
							Cursors_UseArrowCopy();
						}
						else
						{
							Cursors_UseArrow();
						}
						(OSStatus)HIViewSetNeedsDisplay(view, true);
					}
					break;
				
				case kEventControlDragReceive:
					// handle the drop (by copying the data to the clipboard)
					{
						CFStringRef		copiedTextCFString;
						CFStringRef		copiedTextUTI;
						Boolean			copyOK = Clipboard_CreateCFStringFromPasteboard
													(copiedTextCFString, copiedTextUTI, dragPasteboard);
						
						
						if (false == copyOK)
						{
							Console_Warning(Console_WriteLine, "failed to copy the dragged text!");
							result = resNotFound;
						}
						else
						{
							// put the text on the clipboard
							result = Clipboard_AddCFStringToPasteboard(copiedTextCFString, Clipboard_ReturnPrimaryPasteboard());
							if (noErr == result)
							{
								// force a view update, as obviously it is now out of date
								updateClipboard(Clipboard_ReturnPrimaryPasteboard());
								
								// success!
								result = noErr;
							}
							CFRelease(copiedTextCFString), copiedTextCFString = nullptr;
							CFRelease(copiedTextUTI), copiedTextUTI = nullptr;
						}
					}
					break;
				
				default:
					// ???
					break;
				}
			}
		}
	}
	return result;
}// receiveClipboardContentDragDrop


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for the clipboard window.  The default handler destroys
windows, but the clipboard should only be hidden or
shown (as it always remains in memory).

(3.0)
*/
pascal OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClassWindow == kEventClass);
	assert(kEventWindowClose == kEventKind);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			if (window == gClipboardWindow)
			{
				// toggle window display
				Commands_ExecuteByID(kCommandHideClipboard);
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
}// receiveWindowClosing


/*!
Updates a description area in the Clipboard window to
describe the specified text.

The size is given in bytes, but is represented in a more
human-readable form, e.g. "352 K" or "1.2 MB".

(3.1)
*/
void
setDataTypeInformation	(CFStringRef	inUTI,
						 size_t			inDataSize)
{
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	CFStringRef			finalCFString = CFSTR("");
	Boolean				releaseFinalCFString = false;
	
	
	// the contents of the description depend on whether or not
	// the clipboard has any data, and if so, the type and size
	// of that data
	if (0 == inDataSize)
	{
		// clipboard is empty; display a simple description
		stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowDescriptionEmpty, finalCFString);
		if (stringResult.ok())
		{
			releaseFinalCFString = true;
		}
	}
	else
	{
		// create a string describing the contents, by first making
		// the substrings that will be used to form that description
		CFStringRef		descriptionTemplateCFString = CFSTR("");
		
		
		// get a template for a description string
		stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowDescriptionTemplate, descriptionTemplateCFString);
		if (stringResult.ok())
		{
			CFStringRef		contentTypeCFString = UTTypeCopyDescription(inUTI);
			
			
			if (nullptr == contentTypeCFString)
			{
				stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowContentTypeUnknown, contentTypeCFString);
				if (false == stringResult.ok())
				{
					contentTypeCFString = CFSTR("?");
					CFRetain(contentTypeCFString);
				}
			}
			assert(nullptr != contentTypeCFString);
			
			// now create the description string
			{
				// adjust the size value so the number displayed to the user is not too big and ugly;
				// construct a size string, with appropriate units and the word “about”, if necessary
				CFStringRef							aboutCFString = CFSTR("");
				CFStringRef							unitsCFString = CFSTR("");
				UIStrings_ClipboardWindowCFString	unitsStringCode = kUIStrings_ClipboardWindowUnitsBytes;
				Boolean								releaseAboutCFString = false;
				Boolean								releaseUnitsCFString = false;
				Boolean								approximation = false;
				size_t								contentSize = inDataSize;
				
				
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
					stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowDescriptionApproximately,
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
				
			#if 0
				// debug
				Console_WriteValueCFString("description template is", descriptionTemplateCFString);
				Console_WriteValueCFString("determined content type to be", contentTypeCFString);
				Console_WriteValueCFString("determined approximation to be", aboutCFString);
				Console_WriteValue("determined size number to be", contentSize);
				Console_WriteValueCFString("determined size units to be", unitsCFString);
			#endif
				
				// construct a string that contains all the proper substitutions
				// (see the UIStrings code to ensure the substitution order is right!!!)
				finalCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
															descriptionTemplateCFString,
															contentTypeCFString, aboutCFString,
															contentSize, unitsCFString);
				if (nullptr == finalCFString)
				{
					Console_WriteLine("unable to create clipboard description string from format");
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
			}
			
			CFRelease(contentTypeCFString), contentTypeCFString = nullptr;
			CFRelease(descriptionTemplateCFString), descriptionTemplateCFString = nullptr;
		}
	}
	
	// update the window header text
	{
		HIViewWrap		descriptionLabel(idMyLabelDataDescription, gClipboardWindow);
		
		
	#if 0
		(UInt16)Localization_SetUpSingleLineTextControl(descriptionLabel, finalCFString,
														false/* is a checkbox or radio button */);
	#else
		SetControlTextWithCFString(descriptionLabel, finalCFString);
	#endif
	}
	
	if (releaseFinalCFString)
	{
		CFRelease(finalCFString), finalCFString = nullptr;
	}
}// setDataTypeInformation


/*!
Allows the image view to intelligently choose whether or
not to scale its content.  Pass zeroes if no image is being
rendered.

(3.1)
*/
void
setScalingInformation	(size_t		inImageWidth,
						 size_t		inImageHeight)
{
	HIViewWrap		imageView(idMyImageData, gClipboardWindow);
	Boolean			doScale = false;
	
	
	if ((0 != inImageWidth) && (0 != inImageHeight))
	{
		// scale only if the image is bigger than its view
		HIRect		imageBounds;
		
		
		if (noErr == HIViewGetBounds(imageView, &imageBounds))
		{
			doScale = ((inImageWidth > imageBounds.size.width) ||
						(inImageHeight > imageBounds.size.height));
		}
	}
	(OSStatus)HIImageViewSetScaleToFit(imageView, doScale);
}// setScalingInformation


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
			RegionUtilities_RedrawWindowOnNextUpdate(gClipboardWindow);
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
	HIViewWrap		imageView(idMyImageData, gClipboardWindow);
	HIViewWrap		textView(idMyUserPaneDragParent, gClipboardWindow);
	CGImageRef		imageToRender = nullptr;
	CFStringRef		textToRender = nullptr;
	CFStringRef		typeIdentifier = nullptr;
	
	
	if (Clipboard_CreateCFStringFromPasteboard(textToRender, typeIdentifier, inPasteboard))
	{
		// text
		gClipboardDataGeneralTypes()[inPasteboard] = kMy_TypeText;
		if (Clipboard_ReturnPrimaryPasteboard() == inPasteboard)
		{
			gCurrentRenderData = textToRender;
			(OSStatus)HIViewSetVisible(imageView, false);
			(OSStatus)HIViewSetNeedsDisplay(textView, true);
			
			setDataTypeInformation(typeIdentifier, CFStringGetLength(textToRender) * sizeof(UniChar));
			setScalingInformation(0, 0);
		}
		CFRelease(textToRender), textToRender = nullptr;
		CFRelease(typeIdentifier), typeIdentifier = nullptr;
	}
	else if (Clipboard_CreateCGImageFromPasteboard(imageToRender, typeIdentifier, inPasteboard))
	{
		// image
		gClipboardDataGeneralTypes()[inPasteboard] = kMy_TypeGraphics;
		if (Clipboard_ReturnPrimaryPasteboard() == inPasteboard)
		{
			gCurrentRenderData.clear();
			(OSStatus)HIImageViewSetImage(imageView, imageToRender);
			(OSStatus)HIViewSetVisible(imageView, true);
			(OSStatus)HIViewSetNeedsDisplay(imageView, true);
			
			setDataTypeInformation(typeIdentifier,
									CGImageGetHeight(imageToRender) * CGImageGetBytesPerRow(imageToRender));
			setScalingInformation(CGImageGetWidth(imageToRender), CGImageGetHeight(imageToRender));
		}
		CFRelease(imageToRender), imageToRender = nullptr;
		CFRelease(typeIdentifier), typeIdentifier = nullptr;
	}
	else
	{
		// unknown, or empty
		gClipboardDataGeneralTypes()[inPasteboard] = kMy_TypeUnknown;
		if (Clipboard_ReturnPrimaryPasteboard() == inPasteboard)
		{
			gCurrentRenderData.clear();
			(OSStatus)HIViewSetVisible(imageView, false);
			(OSStatus)HIViewSetNeedsDisplay(textView, true);
			
			// TEMPORARY: could determine actual UTI here
			setDataTypeInformation(nullptr, 0);
			setScalingInformation(0, 0);
		}
	}
}// updateClipboard

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
