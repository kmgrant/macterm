/*###############################################################

	Clipboard.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <ColorUtilities.h>
#include <Console.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <FlagManager.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "ControlResources.h"
#include "DialogResources.h"
#include "StringResources.h"

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
#include "TextTranslation.h"
#include "TektronixMacPictureOutput.h"
#include "TektronixVirtualGraphics.h"
#include "TerminalView.h"
#include "UIStrings.h"



#pragma mark Internal Method Prototypes

static pascal void		clipboardUpdatesTimer			(EventLoopTimerRef, void*);
static OSStatus			drawPictureFromDataAndOffset	(Rect const*, Ptr, long);
static pascal void		getPICTData						(void*, short);
static void				getValidScrapType				(ResType*);
static PicHandle		graphicToPICT					(short);
static void				handleNewSize					(WindowRef, Float32, Float32, void*);
static Boolean			mainEventLoopEvent				(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void				pictureToScrap					(Handle);
static pascal OSStatus	receiveClipboardContentDraw		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveClipboardContentDragDrop	(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveWindowClosing			(EventHandlerCallRef, EventRef, void*);
static ResType			returnContentType				();
static UInt32			returnFlavorCount				();
static void				setDataTypeInformation			();
static void				setScalingInformation			();
static void				setSuspended					(Boolean);
static void				setUpControls					(WindowRef);
static void				textToScrap						(Handle);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowRef								gClipboardWindow = nullptr;
	WindowInfoRef							gClipboardWindowInfo = nullptr;
	ControlRef								//gClipboardScrollBarH = nullptr,
											//gClipboardScrollBarV = nullptr,
											gClipboardFooterPlacard = nullptr,
											gClipboardFooterPlacardText = nullptr,
											gClipboardContentUserPane = nullptr,
											gClipboardWindowHeader = nullptr,
											gClipboardWindowHeaderText = nullptr;
	Boolean									gIsSuspended = false,
											gIsShowing = false,
											gIsDataScaled = false;
	SInt16									gScaledPercentage = 100;
	Ptr										gScrapDataPtr = nullptr;
	long									gCurrentOffset = 0L;
	CQDProcsPtr								gSavedProcsPtr = nullptr;
	CQDProcs								gMyColorProcs;
	QDGetPicUPP								gGetPICTDataUPP = nullptr;
	ListenerModel_ListenerRef				gMainEventLoopEventListener = nullptr;
	CommonEventHandlers_WindowResizer		gClipboardResizeHandler;
	EventHandlerUPP							gClipboardWindowClosingUPP = nullptr;
	EventHandlerRef							gClipboardWindowClosingHandler = nullptr;
	EventHandlerUPP							gClipboardDrawUPP = nullptr;
	EventHandlerRef							gClipboardDrawHandler = nullptr;
	EventHandlerUPP							gClipboardDragDropUPP = nullptr;
	EventHandlerRef							gClipboardDragDropHandler = nullptr;
	EventLoopTimerUPP						gClipboardUpdatesTimerUPP = nullptr;
	EventLoopTimerRef						gClipboardUpdatesTimer = nullptr;
}



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
	WindowRef	clipboardWindow = nullptr;
	Rect		rect;
	OSStatus	error = noErr;
	
	
	gGetPICTDataUPP = NewQDGetPicUPP(getPICTData);
	
	// create the clipboard window
	clipboardWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
								CFSTR("Clipboard"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	
	gClipboardWindow = clipboardWindow;
	(OSStatus)SetThemeWindowBackground(clipboardWindow, kThemeBrushFinderWindowBackground, false);
	
	// content area (displays clipboard data if possible)
	SetRect(&rect, 0, 0, 0, 0);
	error = CreateUserPaneControl(clipboardWindow, &rect, kControlSupportsEmbedding | kControlSupportsDragAndDrop,
									&gClipboardContentUserPane);
	assert_noerr(error);
	SetControlVisibility(gClipboardContentUserPane, true/* visible */, true/* draw */);
	
	// install a callback that paints the clipboard data
	{
		EventTypeSpec const		whenControlNeedsDrawing[] =
								{
									{ kEventClassControl, kEventControlDraw }
								};
		
		
		gClipboardDrawUPP = NewEventHandlerUPP(receiveClipboardContentDraw);
		(OSStatus)HIViewInstallEventHandler(gClipboardContentUserPane, gClipboardDrawUPP,
											GetEventTypeCount(whenControlNeedsDrawing),
											whenControlNeedsDrawing, nullptr/* user data */,
											&gClipboardDrawHandler/* event handler reference */);
	}
	
	// install a callback that responds to drags (which copies dragged data to the clipboard)
	{
		EventTypeSpec const		whenDragActivityOccurs[] =
								{
									{ kEventClassControl, kEventControlDragEnter },
									{ kEventClassControl, kEventControlDragWithin },
									{ kEventClassControl, kEventControlDragLeave },
									{ kEventClassControl, kEventControlDragReceive }
								};
		
		
		gClipboardDragDropUPP = NewEventHandlerUPP(receiveClipboardContentDragDrop);
		(OSStatus)HIViewInstallEventHandler(gClipboardContentUserPane, gClipboardDragDropUPP,
											GetEventTypeCount(whenDragActivityOccurs),
											whenDragActivityOccurs, nullptr/* user data */,
											&gClipboardDragDropHandler/* event handler reference */);
	}
	error = SetControlDragTrackingEnabled(gClipboardContentUserPane, true/* is drag enabled */);
	assert_noerr(error);
	
	// scroll bars
	//gClipboardScrollBarH = GetNewControl(kIDClipboardHorizontalScrollBar, clipboardWindow);
	//gClipboardScrollBarV = GetNewControl(kIDClipboardVerticalScrollBar, clipboardWindow);
	
	// window footer
	SetRect(&rect, 0, 0, 0, 0);
	error = CreatePlacardControl(clipboardWindow, &rect, &gClipboardFooterPlacard);
	assert_noerr(error);
	SetControlVisibility(gClipboardFooterPlacard, true/* visible */, true/* draw */);
	
	// footer text (displays percentage of total view)
	SetRect(&rect, 0, 0, 0, 0);
	{
		ControlFontStyleRec		styleRecord;
		Style					fontStyle = normal;
		SInt16					fontSize = 9;
		SInt16					fontID = 0;
		
		
		fontID = GetScriptVariable(GetScriptManagerVariable(smKeyScript), smScriptAppFond);
		styleRecord.flags = kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask;
		styleRecord.font = fontID;
		styleRecord.size = fontSize;
		styleRecord.style = fontStyle;
		error = CreateStaticTextControl(clipboardWindow, &rect, CFSTR(""), &styleRecord, &gClipboardFooterPlacardText);
		assert_noerr(error);
		SetControlVisibility(gClipboardFooterPlacardText, true/* visible */, true/* draw */);
	}
	
	// window header
	SetRect(&rect, 0, 0, 0, 0);
	error = CreatePlacardControl(clipboardWindow, &rect, &gClipboardWindowHeader);
	assert_noerr(error);
	SetControlVisibility(gClipboardWindowHeader, true/* visible */, true/* draw */);
	
	// header text (displays content type and size)
	SetRect(&rect, 0, 0, 0, 0);
	{
		ControlFontStyleRec		styleRecord;
		
		
		styleRecord.flags = kControlUseThemeFontIDMask;
		styleRecord.font = kThemeViewsFont;
		error = CreateStaticTextControl(clipboardWindow, &rect, CFSTR(""), &styleRecord, &gClipboardWindowHeaderText);
		assert_noerr(error);
		SetControlVisibility(gClipboardWindowHeaderText, true/* visible */, true/* draw */);
	}
	
	// embedding hierarchy
	//(OSStatus)EmbedControl(gClipboardScrollBarV, gClipboardContentUserPane);
	//(OSStatus)EmbedControl(gClipboardScrollBarH, gClipboardContentUserPane);
	(OSStatus)EmbedControl(gClipboardFooterPlacardText, gClipboardFooterPlacard);
	(OSStatus)EmbedControl(gClipboardWindowHeaderText, gClipboardWindowHeader);
	
	// set up the Window Info information
	gClipboardWindowInfo = WindowInfo_New();
	WindowInfo_SetWindowDescriptor(gClipboardWindowInfo, kConstantsRegistry_WindowDescriptorClipboard);
	WindowInfo_SetWindowPotentialDropTarget(gClipboardWindowInfo, true/* can receive data via drag-and-drop */);
	WindowInfo_SetForWindow(clipboardWindow, gClipboardWindowInfo);
	
	// specify a different title for the Dock’s menu, one which doesn’t include the application name
	{
		CFStringRef				titleCFString = nullptr;
		UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
		
		
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
	
	// arrange the controls in the clipboard window
	setUpControls(clipboardWindow);
	
	// install a callback that hides the clipboard instead of destroying it, when it should be closed
	{
		EventTypeSpec const		whenWindowClosing[] =
								{
									{ kEventClassWindow, kEventWindowClose }
								};
		
		
		gClipboardWindowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
		error = InstallWindowEventHandler(clipboardWindow, gClipboardWindowClosingUPP,
											GetEventTypeCount(whenWindowClosing),
											whenWindowClosing, nullptr/* user data */,
											&gClipboardWindowClosingHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a timer that detects changes to the clipboard
	{
		gClipboardUpdatesTimerUPP = NewEventLoopTimerUPP(clipboardUpdatesTimer);
		assert(nullptr != gClipboardUpdatesTimerUPP);
		error = InstallEventLoopTimer(GetCurrentEventLoop(),
										kEventDurationNoWait/* seconds before timer fires the first time */,
										kEventDurationSecond * 10.0/* seconds between fires */,
										gClipboardUpdatesTimerUPP, nullptr/* user data - not used */,
										&gClipboardUpdatesTimer);
	}
	
	// install a notification routine in the main event loop, to find out when certain events occur
	gMainEventLoopEventListener = ListenerModel_NewBooleanListener(mainEventLoopEvent);
	EventLoop_StartMonitoring(kEventLoop_GlobalEventSuspendResume, gMainEventLoopEventListener);
	
	// restore the visible state implicitly saved at last Quit
	{
		Boolean		windowIsVisible = false;
		size_t		actualSize = 0L;
		
		
		// get visibility preference for the Clipboard
		unless (Preferences_GetData(kPreferences_TagWasClipboardShowing,
									sizeof(windowIsVisible), &windowIsVisible,
									&actualSize) == kPreferences_ResultCodeSuccess)
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		Clipboard_SetWindowVisible(windowIsVisible);
	}
	
	// enable drag-and-drop on the window!
	(OSStatus)SetAutomaticControlDragTrackingEnabledForWindow(gClipboardWindow, true);
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
		(Preferences_ResultCode)Preferences_SetData(kPreferences_TagWasClipboardShowing,
													sizeof(Boolean), &windowIsVisible);
	}
	
	RemoveEventLoopTimer(gClipboardUpdatesTimer), gClipboardUpdatesTimer = nullptr;
	DisposeEventLoopTimerUPP(gClipboardUpdatesTimerUPP), gClipboardUpdatesTimerUPP = nullptr;
	RemoveEventHandler(gClipboardDragDropHandler);
	DisposeEventHandlerUPP(gClipboardDragDropUPP);
	RemoveEventHandler(gClipboardDrawHandler);
	DisposeEventHandlerUPP(gClipboardDrawUPP);
	RemoveEventHandler(gClipboardWindowClosingHandler);
	DisposeEventHandlerUPP(gClipboardWindowClosingUPP);
	DisposeWindow(gClipboardWindow); // disposes of the clipboard window *and* all of its controls
	gClipboardWindow = nullptr;
	WindowInfo_Dispose(gClipboardWindowInfo);
	DisposeQDGetPicUPP(gGetPICTDataUPP), gGetPICTDataUPP = nullptr;
	EventLoop_StopMonitoring(kEventLoop_GlobalEventSuspendResume, gMainEventLoopEventListener);
	ListenerModel_ReleaseListener(&gMainEventLoopEventListener);
}// Done


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
Creates an appropriate Apple Event descriptor
containing the data currently on the clipboard
(if any).

(3.0)
*/
OSStatus
Clipboard_CreateContentsAEDesc		(AEDesc*		outDescPtr)
{
	OSStatus	result = noErr;
	
	
	if (outDescPtr != nullptr)
	{
		outDescPtr->descriptorType = typeNull;
		outDescPtr->dataHandle = nullptr;
		if (returnContentType() == kScrapFlavorTypeText)
		{
			Handle				handle = nullptr;
			long				length = 0L;		// the length of what is on the scrap
			Boolean				isAnyScrap = false;	// is any text on the clipboard?
			ScrapRef			currentScrap = nullptr;
			ScrapFlavorFlags	currentScrapFlags = 0L;
			
			
			(OSStatus)GetCurrentScrap(&currentScrap);
			
			isAnyScrap = (GetScrapFlavorFlags(currentScrap, kScrapFlavorTypeText, &currentScrapFlags) == noErr);
			if (isAnyScrap) // is any text on the clipboard?
			{
				(OSStatus)GetScrapFlavorSize(currentScrap, kScrapFlavorTypeText, &length);
				handle = Memory_NewHandle(length); // for characters
				(OSStatus)GetScrapFlavorData(currentScrap, kScrapFlavorTypeText, &length, *handle);
				result = AECreateDesc(typeChar, *handle, GetHandleSize(handle), outDescPtr);
			}
		}
	}
	else result = memPCErr;
	
	return result;
}// CreateContentsAEDesc


/*!
Copies the indicated drawing to the clipboard.

(2.6)
*/
void
Clipboard_GraphicsToScrap	(short	inDrawingNumber)
{
	PicHandle	picture = nullptr;
	
	
	picture = graphicToPICT(inDrawingNumber);
	pictureToScrap((Handle)picture);
	KillPicture(picture);
}// GraphicsToScrap


/*!
Returns a reference to the global pasteboard, creating
that reference if necessary.

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
WindowRef
Clipboard_ReturnWindow ()
{
	return gClipboardWindow;
}// ReturnWindow


/*!
Shows or hides the clipboard.

(3.0)
*/
void
Clipboard_SetWindowVisible	(Boolean	inIsVisible)
{
	WindowRef		clipboard = Clipboard_ReturnWindow();
	
	
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
Pastes text from the clipboard into the specified
session.  If the clipboard does not have any text
component, this routine does nothing.

(2.6)
*/
void
Clipboard_TextFromScrap		(SessionRef		inSession)
{
	if (TerminalWindow_ExistsFor(EventLoop_GetRealFrontWindow()))
	{
		SessionPasteState	pasteState;
		
		
		Session_GetPasteState(inSession, &pasteState);
		if (0 != pasteState.outLength)
		{
			// unable to paste any more, yet
			Sound_StandardAlert();
		}
		else
		{
			long				length = 0L;			// the length of what is on the scrap
			Boolean				isAnyScrap = false;		// is any text on the clipboard?
			ScrapRef			currentScrap = nullptr;
			ScrapFlavorFlags	currentScrapFlags = 0L;
			
			
			(OSStatus)GetCurrentScrap(&currentScrap);
			
			isAnyScrap = (GetScrapFlavorFlags(currentScrap, kScrapFlavorTypeText, &currentScrapFlags) == noErr);
			if (isAnyScrap)
			{
				(OSStatus)GetScrapFlavorSize(currentScrap, kScrapFlavorTypeText, &length);
				pasteState.text = Memory_NewHandle(length); // for characters
				pasteState.outLength = length;
				(OSStatus)GetScrapFlavorData(currentScrap, kScrapFlavorTypeText, &pasteState.outLength, *pasteState.text);
				
				HLock(pasteState.text);
				pasteState.nextCharPtr = *pasteState.text;
				
				pasteState.inCount = pasteState.outCount = 0;
				
				//TextTranslation_ConvertBufferToNewHandle( . . . )
				//TextTranslation_ConvertTextFromMac((UInt8*)pasteState.nextCharPtr,
				//									pasteState.outLength,
				//									connectionDataPtr->national); // translate to national characters
				
				Session_UpdatePasteState(inSession, &pasteState);
				// UNIMPLEMENTED - FIX ME, IMPLEMENT PASTE HERE
				Sound_StandardAlert(); // TMP
			}
		}
	}
	else Sound_StandardAlert();
}// TextFromScrap


/*!
Copies the current selection from the indicated
virtual screen, or does nothing if no text is
selected.  If the selection mode is rectangular,
the text is copied line-by-line according to the
rows it crosses.

If the copy method is standard, the text is
copied to the clipboard intact.  If the “table”
method is used, the text is first parsed to
replace tab characters with multiple spaces,
and then the text is copied.  If “inline” is
specified (possibly in addition to table mode),
each line is concatenated together to form a
single line.

(3.0)
*/
void
Clipboard_TextToScrap	(TerminalViewRef		inView,
						 Clipboard_CopyMethod	inHowToCopy)
{
	char**		text = nullptr;			// where to store the characters
	short		tableThreshold = 0;		// zero for normal, nonzero for copy table mode
	
	
	if (inHowToCopy & kClipboard_CopyMethodTable)
	{
		size_t	actualSize = 0L;
		
		
		// get the user’s “one tab equals X spaces” preference, if possible
		unless (Preferences_GetData(kPreferences_TagCopyTableThreshold,
									sizeof(tableThreshold), &tableThreshold,
									&actualSize) == kPreferences_ResultCodeSuccess)
		{
			tableThreshold = 4; // default to 4 spaces per tab if no preference can be found
		}
	}
	
	// get the highlighted characters; assume that text should be inlined unless selected in rectangular mode
	{
		TerminalView_TextFlags	flags = (inHowToCopy & kClipboard_CopyMethodInline) ? kTerminalView_TextFlagInline : 0;
		
		
		text = TerminalView_ReturnSelectedTextAsNewHandle(inView, tableThreshold, flags);
	}
	
	if (GetHandleSize(text) > 0)
	{
		textToScrap(text);
		Memory_DisposeHandle(&text);
	}
	else Sound_StandardAlert(); // nothing to Copy!
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

/*!
Since there does not appear to be an event that
can be handled to notice clipboard changes, this
timer periodically polls the system to figure out
when the clipboard has changed.  If it does change
the clipboard window is updated.

(3.1)
*/
static pascal void
clipboardUpdatesTimer	(EventLoopTimerRef	UNUSED_ARGUMENT(inTimer),
						 void*				UNUSED_ARGUMENT(inUnusedData))
{
	if (Clipboard_WindowIsVisible())
	{
		PasteboardSyncFlags		flags = PasteboardSynchronize(Clipboard_ReturnPrimaryPasteboard());
		
		
		if (flags & kPasteboardModified)
		{
			RegionUtilities_RedrawWindowOnNextUpdate(gClipboardWindow);
		}
	}
}// clipboardUpdatesTimer


/*!
This method will render a picture, given its data
and the desired boundary rectangle.  You provide
a data handle, which you can then offset from by
either zero or some other amount to reference the
correct picture data.

The picture is scaled, at its aspect ratio, to
fit the width and/or the height of the given
rectangle completely.  The global variable for
the scaling factor ("gScaledPercentage") is set
appropriately, and the "gIsDataScaled" flag is
set to true only if the picture is too large to
fit its desired rectangle.

(3.0)
*/
static OSStatus
drawPictureFromDataAndOffset	(Rect const*	inDrawingRect,
								 Ptr			inDataSourcePtr,
								 long			inSourceOffset)
{
	OSStatus	result = noErr;
	PicHandle	tempPict = REINTERPRET_CAST(Memory_NewHandle(sizeof(Picture)), PicHandle);
	
	
	result = MemError();
	if (result == noErr)
	{
		Rect	drawingBounds = *inDrawingRect;
		
		
		// calculate the rectangle in which to draw, and then save the
		// picture header into the temporary handle
		{
			Rect		pictureFrame;
			PicPtr		picturePtr = nullptr;
			UInt16		pictureHeight = 0;
			UInt16		pictureWidth = 0;
			UInt16		frameHeight = drawingBounds.bottom - drawingBounds.top;
			UInt16		frameWidth = drawingBounds.right - drawingBounds.left;
			
			
			picturePtr = REINTERPRET_CAST(inDataSourcePtr + inSourceOffset, PicPtr);
			BlockMoveData(picturePtr, *tempPict, sizeof(Picture));
			
			(Rect*)QDGetPictureBounds(tempPict, &pictureFrame);
			
			pictureHeight = pictureFrame.bottom - pictureFrame.top;
			pictureWidth = pictureFrame.right - pictureFrame.left;
			
			// if the picture frame will fit in the drawing area, no scaling is required
			{
				Rect	rawPictureFrame;
				Rect	rawDrawingFrame;
				Rect	junk;
				
				
				SetRect(&rawPictureFrame, 0, 0, pictureWidth, pictureHeight);
				SetRect(&rawDrawingFrame, 0, 0, frameWidth, frameHeight);
				UnionRect(&rawDrawingFrame, &rawPictureFrame, &junk);
				gIsDataScaled = (!EqualRect(&junk, &rawDrawingFrame));
			}
			
			if (gIsDataScaled)
			{
				Float32		wf = pictureWidth;
				Float32		hf = pictureHeight;
				Float32		ratio = 0.0f;
				Float32		ratioWidths = 0.0f;
				Float32		ratioHeights = 0.0f;
				
				
				ratio = wf / hf;
				wf = frameHeight;
				hf = pictureHeight;
				ratioHeights = wf / hf;
				wf = frameWidth;
				hf = pictureWidth;
				ratioWidths = wf / hf;
				
				// Make either the width or the height of the picture match the
				// maximum width of the drawing area.  This is based on which
				// dimension will fit the drawing area in such a way that the
				// limit as the other dimension approaches the maximum drawing
				// size is the maximum drawing size (that is, no clipping occurs).
				if ((pictureWidth > pictureHeight) && (ratioWidths < ratioHeights))
				{
					drawingBounds.right = drawingBounds.left + frameWidth;
					ratio = frameWidth / ratio;
					frameHeight = (UInt16)ratio;
					drawingBounds.bottom = drawingBounds.top + frameHeight;
					wf = 100 * frameHeight;
					hf = pictureHeight;
					ratio = wf / hf;
					gScaledPercentage = STATIC_CAST(ratio, UInt16);
				}
				else
				{
					drawingBounds.bottom = drawingBounds.top + frameHeight;
					ratio = ratio * frameHeight;
					frameWidth = (UInt16)ratio;
					drawingBounds.right = drawingBounds.left + frameWidth;
					wf = 100 * frameWidth;
					hf = pictureWidth;
					ratio = wf / hf;
					gScaledPercentage = STATIC_CAST(ratio, UInt16);
				}
			}
			else
			{
				// no changes are necessary
				drawingBounds.right = drawingBounds.left + pictureWidth;
				drawingBounds.bottom = drawingBounds.top + pictureHeight;
			}
		}
		
		// store into globals for the GetPicProc in preparation for the draw
		gScrapDataPtr = inDataSourcePtr;
		gCurrentOffset = inSourceOffset + sizeof(Picture);
		
		// install a GetPic proc
		{
			CGrafPtr	currentPort = nullptr;
			GDHandle	currentDevice = nullptr;
			
			
			GetGWorld(&currentPort, &currentDevice);
			SetStdCProcs(&gMyColorProcs);
			gMyColorProcs.getPicProc = gGetPICTDataUPP;
			gSavedProcsPtr = GetPortGrafProcs(currentPort);
			SetPortGrafProcs(currentPort, &gMyColorProcs);
			
			// draw the picture
			DrawPicture(tempPict, &drawingBounds);
			
			// remove the GetPic proc
			SetPortGrafProcs(currentPort, gSavedProcsPtr);
		}
		
		// update the bottom text area
		setScalingInformation();
		
		Memory_DisposeHandle(REINTERPRET_CAST(&tempPict, Handle*));
	}
	return result;
}// drawPictureFromHandleAndOffset


/*!
This is a replacement for the QuickDraw
bottleneck routine.

(3.0)
*/
static pascal void
getPICTData		(void*		inDataPtr,
				 short		inByteCount)
{ 
	long	longCount = inByteCount;
	
	
	BlockMoveData(gScrapDataPtr + gCurrentOffset, inDataPtr, longCount);
	gCurrentOffset += longCount;
}// getPICTData


/*!
This method checks the Scrap for a data type it can
render.  If none is found, this routine returns a
scrap type of '----'; otherwise, it returns the type
and GetScrap() result for the valid scrap type.

(3.0)
*/
static void
getValidScrapType	(ResType*	outScrapType)
{
	register SInt16		i = 0;
	ResType				validScrapTypeResult = '----';
	ResType				scrapTypes[] = { kScrapFlavorTypeUnicode, kScrapFlavorTypeText, kScrapFlavorTypePicture, '----' };
	ScrapRef			currentScrap = nullptr;
	ScrapFlavorFlags	currentScrapFlags = 0L;
	
	
	(OSStatus)GetCurrentScrap(&currentScrap);
	while (scrapTypes[i] != '----')
	{
		if (GetScrapFlavorFlags(currentScrap, scrapTypes[i], &currentScrapFlags) == noErr)
		{
			// if flags can be obtained with no error, then a scrap of the specified type exists
			validScrapTypeResult = scrapTypes[i];
			break;
		}
		++i;
	}
	if (outScrapType != nullptr) *outScrapType = validScrapTypeResult;
}// getValidScrapType


/*!
Converts the specified drawing into a standard
Mac OS picture format (PICT) and returns the
handle to the converted picture.

(2.6)
*/
static PicHandle
graphicToPICT	(short		inDrawingNumber)
{
	short		j = 0;
	PicHandle	tpic = nullptr;
	Rect		trect;
	
	
	SetRect(&trect, 0, 0, 384, 384);
	j = VGnewwin(TEK_DEVICE_PICTURE, VGgetVS(inDrawingNumber));
	TektronixMacPictureOutput_SetBounds(&trect);
	VGzcpy(inDrawingNumber, j);
	
	tpic = OpenPicture(&trect);
	ClipRect(&trect);
	VGredraw(inDrawingNumber, j);
	ClosePicture();
	VGclose(j);
	
	return(tpic);
}// graphicToPICT


/*!
This method moves and resizes controls in the clipboard
window in response to a resize.

(3.0)
*/
static void
handleNewSize	(WindowRef		inWindow,
				 Float32		inDeltaSizeX,
				 Float32		inDeltaSizeY,
				 void*			UNUSED_ARGUMENT(inData))
{
	SInt32		truncDeltaX = STATIC_CAST(inDeltaSizeX, SInt32);
	SInt32		truncDeltaY = STATIC_CAST(inDeltaSizeY, SInt32);
	
	
	DialogAdjust_BeginControlAdjustment(inWindow);
	DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gClipboardWindowHeader, truncDeltaX);
	//DialogAdjust_AddControl(kDialogItemAdjustmentMoveV, gClipboardScrollBarH, truncDeltaY);
	//DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gClipboardScrollBarV, truncDeltaX);
	//DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gClipboardScrollBarH, truncDeltaX);
	//DialogAdjust_AddControl(kDialogItemAdjustmentResizeV, gClipboardScrollBarV, truncDeltaY);
	DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gClipboardFooterPlacard, truncDeltaX);
	DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gClipboardFooterPlacardText, truncDeltaX);
	DialogAdjust_AddControl(kDialogItemAdjustmentMoveV, gClipboardFooterPlacard, truncDeltaY);
	DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gClipboardContentUserPane, truncDeltaX);
	DialogAdjust_AddControl(kDialogItemAdjustmentResizeV, gClipboardContentUserPane, truncDeltaY);
	DialogAdjust_EndAdjustment(truncDeltaX, truncDeltaY);
}// handleNewSize


/*!
Invoked whenever a monitored event from the main event
loop occurs (see Clipboard_Init() to see which events
are monitored).  This routine responds by ensuring that
the clipboard state is correct (e.g. hiding the clipboard
window on suspend events, updating the clipboard contents
on resume events, etc.).

The result is "true" only if the event is to be absorbed
(preventing anything else from seeing it).

(3.0)
*/
static Boolean
mainEventLoopEvent	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inEventThatOccurred,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Boolean		result = false;
	
	
	switch (inEventThatOccurred)
	{
	case kEventLoop_GlobalEventSuspendResume:
		// update the clipboard state to match the current suspended state of the application
		{
			Boolean		suspended = false;
			
			
			suspended = FlagManager_Test(kFlagSuspended);
			setSuspended(suspended);
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// mainEventLoopEvent


/*!
Copies the specified PICT (QuickDraw picture) data
to the clipboard.

(3.0)
*/
static void
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
Handles "kEventControlDraw" of "kEventClassControl".

Invoked by Mac OS X whenever the clipboard contents
should be redrawn.

(3.1)
*/
static pascal OSStatus
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
		ControlRef		control = nullptr;
		
		
		// get the target control
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the control was found, continue
		if (result == noErr)
		{
			//ControlPartCode		partCode = 0;
			CGrafPtr			drawingPort = nullptr;
			CGrafPtr			oldPort = nullptr;
			GDHandle			oldDevice = nullptr;
			
			
			GetGWorld(&oldPort, &oldDevice);
			
			// could determine which part (if any) to draw; if none, draw everything
			// (ignored, not needed)
			//result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			//result = noErr; // ignore part code parameter if absent
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, drawingPort);
			if (result != noErr)
			{
				// use current port
				drawingPort = oldPort;
				result = noErr;
			}
			
			// if all information can be found, proceed with drawing
			if (result == noErr)
			{
				Rect		controlRect;
				SInt16		colorDepth = ColorUtilities_GetCurrentDepth(drawingPort);
				Boolean		isColorDevice = IsPortColor(drawingPort);
				
				
				SetPort(drawingPort);
				GetControlBounds(control, &controlRect);
				
				// set up the control background and foreground colors
				(OSStatus)SetUpControlBackground(control, colorDepth, isColorDevice);
				
				// account for Aqua drawing outside the control boundaries
				//InsetRect(&controlRect, -2, -2);
				EraseRect(&controlRect);
				//InsetRect(&controlRect, 2, 2);
				
				// determine and describe what is currently on the clipboard
				setDataTypeInformation();
				
				// if the data is scaled, say so; otherwise, remove the footer text completely
				setScalingInformation();
				
				// draw the data on the clipboard
				// TEMPORARY: drawing code should not do this much work; ideally,
				// some other event handler asynchronously is told when to update
				// information such as the type of clipboard data, allowing the
				// drawing code to merely consult that information rather than
				// determine it all on the fly
				{
					OSStatus	error = noErr;
					Rect		clipboardContentRect;
					RgnHandle	preservedClippingRegion = Memory_NewRegion();
					
					
					GetControlBounds(control, &clipboardContentRect);
					if (preservedClippingRegion != nullptr) GetClip(preservedClippingRegion);
					
					// determine what is currently on the clipboard
					{
						ResType		validScrapType = '----';
						Size		scrapSize = 0L;
						
						
						// figure out the scrap type and offset
						getValidScrapType(&validScrapType);
						
						// now, draw the contents, if a supported type of data is available
						{
							Rect	clipArea = clipboardContentRect;
							Ptr		scrapDataPtr = nullptr;
							
							
							// under Carbon, the scrap handle can’t be accessed; instead, a
							// handle is created by this block and the scrap is copied into it
							{
								ScrapRef	currentScrap = nullptr;
								
								
								if (GetCurrentScrap(&currentScrap) == noErr)
								{
									Size	mutableScrapSize = 0;
									
									
									error = GetScrapFlavorSize(currentScrap, validScrapType, &mutableScrapSize);
									if (error == noErr)
									{
										Size const  kBufferSize = mutableScrapSize;
										
										
										scrapDataPtr = Memory_NewPtr(kBufferSize);
										if (scrapDataPtr != nullptr)
										{
											mutableScrapSize = kBufferSize;
											error = GetScrapFlavorData(currentScrap, validScrapType, &mutableScrapSize, scrapDataPtr);
											if (error == noErr)
											{
												// ensure string length is correct
												scrapSize = (mutableScrapSize < kBufferSize) ? mutableScrapSize : kBufferSize;
											}
										}
									}
								}
							}
							
							// only do this if scroll bars are implemented
							//{
							//	SInt32		scrollBarSize;
							//	
							//	
							//	(OSStatus)GetThemeMetric(kThemeMetricScrollBarWidth, &scrollBarSize);
							//	clipArea.right -= scrollBarSize; // only needed if scroll bars are implemented
							//	clipArea.bottom -= scrollBarSize; // only needed if scroll bars are implemented
							//}
							ClipRect(&clipArea);
							
							// initially assume the data is not scaled
							gIsDataScaled = false;
							
							// perform any necessary rendering for drags
							{
								Boolean		dragHighlight = false;
								UInt32		actualSize = 0;
								
								
								if (noErr != GetControlProperty
												(control, kConstantsRegistry_ControlPropertyCreator,
													kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
													sizeof(dragHighlight), &actualSize,
													&dragHighlight))
								{
									dragHighlight = false;
								}
								
								if (dragHighlight)
								{
									DragAndDrop_ShowHighlightBackground(drawingPort, &clipArea);
									// frame is drawn at the end, after any content
								}
								else
								{
									DragAndDrop_HideHighlightBackground(drawingPort, &clipArea);
									DragAndDrop_HideHighlightFrame(drawingPort, &clipArea);
								}
							}
							
							switch (validScrapType)
							{
							case kScrapFlavorTypePicture:
								drawPictureFromDataAndOffset(&clipArea, scrapDataPtr, 0/* offset */);
								break;
							
							case kScrapFlavorTypeText:
							case kScrapFlavorTypeUnicode:
								// use the default terminal’s monospaced font for drawing text
								{
									Preferences_ResultCode		prefsResult = kPreferences_ResultCodeSuccess;
									Preferences_ContextRef		prefsContext = nullptr;
									
									
									prefsResult = Preferences_GetDefaultContext(&prefsContext, kPreferences_ClassFormat);
									if (prefsResult == kPreferences_ResultCodeSuccess)
									{
										Str255		fontName;
										SInt16		fontSize = 0;
										size_t		actualSize = 0;
										
										
										// find default terminal font family
										prefsResult = Preferences_ContextGetData
														(prefsContext, kPreferences_TagFontName, sizeof(fontName),
															fontName, &actualSize);
										if (prefsResult == kPreferences_ResultCodeSuccess)
										{
											TextFontByName(fontName);
										}
										
										// find default terminal font size
										prefsResult = Preferences_ContextGetData
														(prefsContext, kPreferences_TagFontSize, sizeof(fontSize),
															&fontSize, &actualSize);
										if (prefsResult == kPreferences_ResultCodeSuccess)
										{
											TextSize(fontSize);
										}
									}
								}
								
								// use an appropriate theme color - since icon labels show up on
								// Finder window backgrounds, that color should work here
								{
									ColorPenState		state;
									
									
									ColorUtilities_PreserveColorAndPenState(&state);
									ColorUtilities_NormalizeColorAndPen();
									error = SetThemeTextColor(kThemeTextColorIconLabel, colorDepth, isColorDevice);
									ColorUtilities_RestoreColorAndPenState(&state);
								}
								
								{
									// draw anti-aliased text on Mac OS X
									CFStringRef		stringRef = nullptr;
									
									
									if (validScrapType == kScrapFlavorTypeUnicode)
									{
										stringRef = CFStringCreateWithCharacters
													(kCFAllocatorDefault, REINTERPRET_CAST(scrapDataPtr, UniChar const*),
														scrapSize / sizeof(UniChar)/* number of Unicode characters */);
									}
									else
									{
										stringRef = CFStringCreateWithBytes
													(kCFAllocatorDefault, REINTERPRET_CAST(scrapDataPtr, UInt8 const*),
														scrapSize, CFStringGetSystemEncoding(),
														false/* is external representation */);
									}
									if (stringRef != nullptr)
									{
										error = DrawThemeTextBox(stringRef, kThemeCurrentPortFont, kThemeStateActive,
																	false/* wrap to width */, &clipArea, teJustLeft,
																	nullptr/* context */);
										CFRelease(stringRef);
									}
								}
								break;
							
							default:
								break;
							}
							Memory_DisposePtr(&scrapDataPtr);
							
							// perform any necessary rendering for drags
							{
								Boolean		dragHighlight = false;
								UInt32		actualSize = 0;
								
								
								if (noErr != GetControlProperty
												(control, kConstantsRegistry_ControlPropertyCreator,
													kConstantsRegistry_ControlPropertyTypeShowDragHighlight,
													sizeof(dragHighlight), &actualSize,
													&dragHighlight))
								{
									dragHighlight = false;
								}
								
								if (dragHighlight)
								{
									DragAndDrop_ShowHighlightFrame(drawingPort, &clipArea);
								}
							}
						}
						
						if (preservedClippingRegion != nullptr) SetClip(preservedClippingRegion);
					}
					
					Memory_DisposeRegion(&preservedClippingRegion);
				}
			}
			
			// restore port
			SetGWorld(oldPort, oldDevice);
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
static pascal OSStatus
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
				switch (kEventKind)
				{
				case kEventControlDragEnter:
					// indicate whether or not this drag is interesting
					{
						Boolean		acceptDrag = DragAndDrop_DragContainsOnlyTextItems(dragRef);
						
						
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
						
						
						result = SetControlProperty(view, kConstantsRegistry_ControlPropertyCreator,
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
					#if 1 // NON-COMPOSITING
						DrawOneControl(view);
					#else // COMPOSITING
						(OSStatus)HIViewSetNeedsDisplay(view, true);
					#endif
					}
					break;
				
				case kEventControlDragReceive:
					// handle the drop (by copying the data to the clipboard)
					{
						Handle		textToCopy = nullptr;
						
						
						// TEMPORARY - this ought to work for any text in any character set!
						result = DragAndDrop_GetDraggedTextAsNewHandle(dragRef, &textToCopy);
						if (noErr == result)
						{
							HLock(textToCopy);
							
							// put the text on the clipboard
							(OSStatus)ClearCurrentScrap();
							{
								ScrapRef	currentScrap = nullptr;
								
								
								if (noErr == GetCurrentScrap(&currentScrap))
								{
									// copy to clipboard
									(OSStatus)PutScrapFlavor(currentScrap, kScrapFlavorTypeText,
																kScrapFlavorMaskNone, GetHandleSize(textToCopy),
																*textToCopy);
									
									// force a view update, as obviously it is now out of date
								#if 1 // NON-COMPOSITING
									DrawOneControl(view);
								#else // COMPOSITING
									(OSStatus)HIViewSetNeedsDisplay(view, true);
								#endif
								}
							}
							
							// clean up
							HUnlock(textToCopy);
							Memory_DisposeHandle(&textToCopy);
							
							// success!
							result = noErr;
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
static pascal OSStatus
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
To determine what kind of data is on the
clipboard, use this method.  If the data
is something that the clipboard window
cannot render, '----' is returned.

(3.0)
*/
static ResType
returnContentType ()
{
	ResType		result = '----';
	
	
	getValidScrapType(&result);
	return result;
}// returnContentType


/*!
Returns the number of data choices from the
current clipboard.  If this number changes
during the lifetime of the active program,
then the clipboard window may need updating.

(3.0)
*/
static UInt32
returnFlavorCount ()
{
	UInt32		result = 0L;
	ScrapRef	scrap = nullptr;
	
	
	if (GetCurrentScrap(&scrap) == noErr)
	{
		(OSStatus)GetScrapFlavorCount(scrap, &result);
	}
	
	return result;
}// returnFlavorCount


/*!
To set up the contents of the top text area of
the clipboard window, use this method.  The
text varies based on the presence of data on
the clipboard, as well as the kind of data and
its size.

(3.0)
*/
static void
setDataTypeInformation ()
{
	UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
	ResType					validScrapType = '----';
	CFStringRef				finalCFString = CFSTR("");
	Boolean					releaseFinalCFString = false;
	
	
	// figure out the scrap type and offset
	getValidScrapType(&validScrapType);
	
	// the contents of the description depend on whether or not
	// the clipboard has any data, and if so, the type and size
	// of that data
	if (returnFlavorCount() == 0)
	{
		// clipboard is empty; display a simple description
		stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowDescriptionEmpty, finalCFString);
		if (kUIStrings_ResultCodeSuccess == stringResult)
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
		if (kUIStrings_ResultCodeSuccess == stringResult)
		{
			CFStringRef							contentTypeCFString = CFSTR("");
			Boolean								releaseContentTypeCFString = false;
			UIStrings_ClipboardWindowCFString	contentTypeStringCode = kUIStrings_ClipboardWindowContentTypeUnknown;
			
			
			// find a good way to describe what is on the clipboard
			switch (validScrapType)
			{
			case kScrapFlavorTypePicture:
				contentTypeStringCode = kUIStrings_ClipboardWindowContentTypePicture;
				break;
			
			case kScrapFlavorTypeUnicode:
				contentTypeStringCode = kUIStrings_ClipboardWindowContentTypeUnicodeText;
				break;
			
			case kScrapFlavorTypeText:
				contentTypeStringCode = kUIStrings_ClipboardWindowContentTypeText;
				break;
			
			default:
				break;
			}
			stringResult = UIStrings_Copy(contentTypeStringCode, contentTypeCFString);
			if (kUIStrings_ResultCodeSuccess == stringResult)
			{
				// a new string was created, so it must be released later
				releaseContentTypeCFString = true;
			}
			
			// now create the description string
			{
				// find scrap size in bytes, but then adjust the value so
				// the number displayed to the user is not too big and ugly
				ScrapRef	scrap = nullptr;
				Size		contentSize = 0L;
				OSStatus	error = noErr;
				
				
				error = GetCurrentScrap(&scrap);
				if (noErr == error)
				{
					error = GetScrapFlavorSize(scrap, returnContentType(), &contentSize);
				}
				
				if (noErr == error)
				{
					// construct a size string, with appropriate units and the word “about”, if necessary
					CFStringRef							aboutCFString = CFSTR("");
					CFStringRef							unitsCFString = CFSTR("");
					UIStrings_ClipboardWindowCFString	unitsStringCode = kUIStrings_ClipboardWindowUnitsBytes;
					Boolean								releaseAboutCFString = false;
					Boolean								releaseUnitsCFString = false;
					Boolean								approximation = false;
					
					
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
						if (kUIStrings_ResultCodeSuccess == stringResult)
						{
							// a new string was created, so it must be released later
							releaseAboutCFString = true;
						}
					}
					
					// based on the determined units, find the proper unit string
					stringResult = UIStrings_Copy(unitsStringCode, unitsCFString);
					if (kUIStrings_ResultCodeSuccess == stringResult)
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
			}
			
			// if a content type string was created before, free it now
			if (releaseContentTypeCFString)
			{
				CFRelease(contentTypeCFString), contentTypeCFString = nullptr;
			}
			
			// free the template string that was allocated
			CFRelease(descriptionTemplateCFString), descriptionTemplateCFString = nullptr;
		}
	}
	
	// figure out what font to use for the window header text, and set its size to fit
	// (this routine also sets the text contained in the control)
#if 0
	(UInt16)Localization_SetUpSingleLineTextControl(gClipboardWindowHeaderText, finalCFString,
													false/* is a checkbox or radio button */);
#else
	SetControlTextWithCFString(gClipboardWindowHeaderText, finalCFString);
#endif
	
	if (releaseFinalCFString)
	{
		CFRelease(finalCFString), finalCFString = nullptr;
	}
}// setDataTypeInformation


/*!
To set up the contents of the bottom text area
of the clipboard window, use this method.  The
text is blank unless the clipboard contains a
picture that has been scaled smaller than its
actual size.

(3.0)
*/
static void
setScalingInformation ()
{
	// if the data is scaled, say so; otherwise, remove the footer text completely
	if (gIsDataScaled)
	{
		UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
		CFStringRef				footerCFString = nullptr;
		CFStringRef				footerTemplateCFString = nullptr;
		Boolean					releaseFooterCFString = true;
		
		
		stringResult = UIStrings_Copy(kUIStrings_ClipboardWindowDisplaySizePercentage, footerTemplateCFString);
		if (kUIStrings_ResultCodeSuccess == stringResult)
		{
			footerCFString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* format options */,
														footerTemplateCFString,
														gScaledPercentage/* substituted number */);
		}
		else
		{
			footerCFString = CFSTR("");
			releaseFooterCFString = false;
		}
		SetControlTextWithCFString(gClipboardFooterPlacardText, footerCFString);
		
		if (releaseFooterCFString)
		{
			CFRelease(footerCFString), footerCFString = nullptr;
		}
		CFRelease(footerTemplateCFString), footerTemplateCFString = nullptr;
	}
	else
	{
		SetControlTextWithCFString(gClipboardFooterPlacardText, CFSTR(""));
	}
}// setScalingInformation


/*!
To set the suspended state of the clipboard,
call this method.  If the clipboard is suspended,
it is collapsed, in order to remain on the screen
without showing any contents (other applications
could change the contents of the Clipboard
without MacTelnet’s knowledge, so it should not
try to render the Clipboard when in the background).

(3.0)
*/
static void
setSuspended	(Boolean	inIsSuspended)
{
	WindowRef		clipboard = Clipboard_ReturnWindow();
	
	
	gIsSuspended = inIsSuspended;
	if (inIsSuspended) HideWindow(clipboard);
	else if (gIsShowing) ShowWindow(clipboard);
}// setSuspended


/*!
To set the initial arrangement of the controls
for the clipboard window, use this method.
Subsequent resizings will simply add or subtract
deltas from the initial control rectangles, so
tbe “initial arrangement” calculation only needs
to be done once.

(3.0)
*/
static void
setUpControls	(WindowRef		inWindow)
{
	Rect		clipboardContentRect;
	SInt32		scrollBarSize = 0L;
	
	
	// get scroll bar height
	(OSStatus)GetThemeMetric(kThemeMetricScrollBarWidth, &scrollBarSize);
	
	// calculate the area at the top that says what is on the clipboard,
	// and the area that displays the clipboard data
	{
		enum
		{
			kHeaderTextInsetH = 4, // pixels horizontally inward that the window header text is from the edges of the header
			kHeaderTextInsetV = 4 // pixels vertically inward that the window header text is from the edges of the header
		};
		Rect	topRect;
		Rect	controlRect;
		
		
		// use this routine to automatically set the proper height for the text
		Localization_SetUpSingleLineTextControl(gClipboardWindowHeaderText, EMPTY_PSTRING);
		
		GetPortBounds(GetWindowPort(inWindow), &topRect);
		GetControlBounds(gClipboardWindowHeaderText, &controlRect);
		topRect.bottom = (controlRect.bottom - controlRect.top) + (2 * kHeaderTextInsetV);
		
		// resize the window header and its text control
		InsetRect(&topRect, kHeaderTextInsetH, kHeaderTextInsetV);
		MoveControl(gClipboardWindowHeaderText, topRect.left, topRect.top);
		SizeControl(gClipboardWindowHeaderText, topRect.right - topRect.left, topRect.bottom - topRect.top);
		InsetRect(&topRect, -kHeaderTextInsetH, -kHeaderTextInsetV);
		MoveControl(gClipboardWindowHeader, topRect.left - 1, topRect.top - 1);
		SizeControl(gClipboardWindowHeader, topRect.right - topRect.left + 2, topRect.bottom - topRect.top + 2);
		
		// calculate the part *not* in the top area
		clipboardContentRect = topRect;
		clipboardContentRect.top = topRect.bottom + 1;
		GetPortBounds(GetWindowPort(inWindow), &topRect); // "topRect" has a new meaning here...
		clipboardContentRect.bottom = topRect.bottom - scrollBarSize + 1;
	}
	
	// arrange scroll bars
	//(OSStatus)GetThemeMetric(kThemeMetricScrollBarWidth, &scrollBarSize);
	//MoveControl(gClipboardScrollBarH, clipboardContentRect.left - 1,
	//									clipboardContentRect.bottom - scrollBarSize + 1);
	//SizeControl(gClipboardScrollBarH,
	//				clipboardContentRect.right - clipboardContentRect.left - scrollBarSize + 1 + 2/* 2 * frame width */,
	//				scrollBarSize);
	//MoveControl(gClipboardScrollBarV, clipboardContentRect.right - scrollBarSize + 1,
	//										clipboardContentRect.top - 1);
	//SizeControl(gClipboardScrollBarV, scrollBarSize,
	//				clipboardContentRect.bottom - clipboardContentRect.top - scrollBarSize + 1 + 2/* 2 * frame width */);
	
	// arrange other controls
	{
		enum
		{
			kPlacardTextInsetH = 2, // pixels horizontally inward that the window footer text is from the placard edges
			kPlacardTextInsetV = 2 // pixels vertically inward that the window footer text is from the placard edges
		};
		
		
		MoveControl(gClipboardContentUserPane, clipboardContentRect.left, clipboardContentRect.top);
		SizeControl(gClipboardContentUserPane, clipboardContentRect.right - clipboardContentRect.left,
						clipboardContentRect.bottom - clipboardContentRect.top);
		MoveControl(gClipboardFooterPlacard, clipboardContentRect.left - 1, clipboardContentRect.bottom);
		SizeControl(gClipboardFooterPlacard,
					clipboardContentRect.right - clipboardContentRect.left - scrollBarSize + 1 + 2, scrollBarSize);
		InsetRect(&clipboardContentRect, kPlacardTextInsetH, kPlacardTextInsetV);
		MoveControl(gClipboardFooterPlacardText, clipboardContentRect.left,
						clipboardContentRect.bottom + 2 * kPlacardTextInsetV);
		SizeControl(gClipboardFooterPlacardText,
						clipboardContentRect.right - clipboardContentRect.left - scrollBarSize + 1 + 2,
						scrollBarSize - 2 * kPlacardTextInsetH);
		InsetRect(&clipboardContentRect, -kPlacardTextInsetH, -kPlacardTextInsetV);
	}
}// setUpControls


/*!
Copies the specified plain text data to the
clipboard.

(3.0)
*/
static void
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

// BELOW IS REQUIRED NEWLINE TO END FILE
