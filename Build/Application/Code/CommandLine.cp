/*###############################################################

	CommandLine.cp
	
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

// Enable this if you want tab keypresses to trigger immediate sends
// to the session (e.g. to allow tab completing shells to work from
// a floating command line).  This is generally disabled because it
// confuses normal tab focus in the floating window.
#define RECOGNIZE_TABS	0

// standard-C includes
#include <climits>

// standard-C++ includes
#include <vector>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <CFRetainRelease.h>
#include <Console.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <HIViewWrap.h>
#include <IconManager.h>
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
#include "AppResources.h"
#include "Commands.h"
#include "CommandLine.h"
#include "CommonEventHandlers.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Preferences.h"
#include "QuillsSession.h"
#include "Session.h"
#include "SessionFactory.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "UIStrings.h"
#include "URL.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Window" NIB from the package "CommandLineWindow.nib".
*/
static HIViewID const	idMyUserPaneCommandLine			= { 'Line', 0/* ID */ };
static HIViewID const	idMyTerminalViewCommandLine		= { 'TrmV', 0/* ID */ };
static HIViewID const	idMyTextCommandHelp				= { 'Info', 0/* ID */ };
static HIViewID const	idMyButtonHelp					= { 'Help', 0/* ID */ };
static HIViewID const	idMyMenuHistory					= { 'HMnu', 0/* ID */ };
static HIViewID const	idMyLittleArrowsHistory			= { 'Hist', 0/* ID */ };

enum
{
	kEditTextInsetH = 8, // pixels horizontally inward that the text field is from the edges of the window
	kEditTextInsetV = 8, // pixels vertically inward that the text field is from the edges of the window
	kHelpTextInsetH = 6, // pixels horizontally inward that the help text is from the edges of the window
	kHelpTextInsetV = 6, // pixels vertically inward that the help text is from the edges of the window
	kEditTextArrowsSpacingH = 8, // number of pixels of space between the text field and the little arrows
	kEditTextHelpTextSpacingV = 6, // number of pixels of space between the text field and the help text
	kCommandLineCommandHistorySize = 5
};

#pragma mark Types

typedef std::vector< CFRetainRelease >					CommandHistoryList;
typedef std::vector< CommandLine_InterpreterProcPtr >	CommandInterpreterList;
enum My_TerminatorType
{
	kMy_TerminatorTypeNewline	= 0,	// append newline sequence, whatever the user’s preferences say that is
	kMy_TerminatorTypeTab		= 1,	// append a tab character, with no newline
	kMy_TerminatorTypeBackspace	= 2		// append a delete
};

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowRef								gCommandLineWindow = nullptr;
	WindowInfo_Ref							gCommandLineWindowInfo = nullptr;
	TerminalScreenRef						gCommandLineTerminalScreen = nullptr;
	TerminalViewRef							gCommandLineTerminalView = nullptr;
	HIViewWrap								gCommandLineTerminalViewFocus = nullptr;
	HIViewWrap								gCommandLineTerminalViewContainer = nullptr;
	HIViewWrap								gCommandLineHistoryPopUpMenu = nullptr;
	HIViewWrap								gCommandLineArrows = nullptr;
	HIViewWrap								gCommandLineHelpButton = nullptr;
	HIViewWrap								gCommandLineHelpText = nullptr;
	MenuRef									gCommandLineHistoryMenuRef = nullptr;
	CommonEventHandlers_WindowResizer		gCommandLineWindowResizeHandler;
	EventHandlerUPP							gCommandLineWindowClosingUPP = nullptr;
	EventHandlerRef							gCommandLineWindowClosingHandler = nullptr;
	EventHandlerUPP							gCommandLineCursorChangeUPP = nullptr;
	EventHandlerRef							gCommandLineCursorChangeHandler = nullptr;
	EventHandlerUPP							gCommandLineTextInputUPP = nullptr;
	EventHandlerRef							gCommandLineTextInputHandler = nullptr;
	EventHandlerUPP							gCommandLineHistoryMenuCommandUPP = nullptr;
	EventHandlerRef							gCommandLineHistoryMenuCommandHandler = nullptr;
	EventHandlerUPP							gCommandLineHistoryArrowClickUPP = nullptr;
	EventHandlerRef							gCommandLineHistoryArrowClickHandler = nullptr;
	CommandInterpreterList&					gCommandLineInterpreterList ()	{ static CommandInterpreterList x; return x; }
	CommandHistoryList&						gCommandHistory ()				{ static CommandHistoryList x; return x; }
	SInt16									gCommandHistoryCurrentIndex = -1; // which history command has been displayed via up/down arrow
}

#pragma mark Internal Prototypes

static void								addToHistory					(CFStringRef);
static ControlKeyFilterResult			commandLineKeyFilter			(UInt32*);
static CFStringRef						getTextAsCFString				();
static void								handleNewSize					(WindowRef, Float32, Float32, void*);
static Boolean							issueMessageToAllInterpreters	(CommandLine_InterpreterMessage, CFArrayRef,
																		 void*, CommandLine_InterpreterProcPtr*);
static inline My_TerminatorType			keyCodeToTerminatorType			(SInt16);
static Boolean							parseCommandLine				(CFStringRef, My_TerminatorType);
static Boolean							parseUNIX						(CFArrayRef);
static pascal OSStatus					receiveHistoryArrowClick		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus					receiveHistoryCommandProcess	(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus					receiveTextInput				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus					receiveWindowClosing			(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus					receiveWindowCursorChange		(EventHandlerCallRef, EventRef, void*);
static Boolean							rotateHistory					(SInt16);
static void								setBilineText					(CFStringRef);



#pragma mark Public Methods

/*!
Creates the command line window and performs other
initialization tasks.

IMPORTANT:	The Command Line module can auto-initialize
			(whenever certain methods are used).  It is
			probably better not to explicitly invoke
			CommandLine_Init() unless you need absolute
			control over when the initialization occurs.

(3.0)
*/
void
CommandLine_Init ()
{
	OSStatus	error = noErr;
	
	
	// initialize command history
	gCommandHistory().reserve(kCommandLineCommandHistorySize);
	
	// load the NIB containing this window (automatically finds the right localization)
	gCommandLineWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("CommandLine"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	
	// create the terminal view
	{
		Terminal_Result		terminalError = kTerminal_ResultOK;
		Str255				fontName;
		Rect				bounds;
		HIRect				floatBounds;
		
		
		// get the proper rectangle from the NIB; but NOTE, although the NIB-based
		// control is no longer needed and is disposed, the variable is re-assigned
		// below to whatever control is created by the new Terminal View
		gCommandLineTerminalViewContainer = HIViewWrap(idMyUserPaneCommandLine, gCommandLineWindow);
		assert(gCommandLineTerminalViewContainer.exists());
		GetControlBounds(gCommandLineTerminalViewContainer, &bounds);
		DisposeControl(gCommandLineTerminalViewContainer), gCommandLineTerminalViewContainer.clear();
		floatBounds = CGRectMake(bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);
		
		// use the default terminal’s font for the command line window, because the font is fixed-width
		{
			Preferences_Result			prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef		prefsContext = nullptr;
			
			
			prefsResult = Preferences_GetDefaultContext(&prefsContext, kPreferences_ClassFormat);
			if (kPreferences_ResultOK == prefsResult)
			{
				size_t	actualSize = 0;
				
				
				// find default terminal font family
				prefsResult = Preferences_ContextGetData(prefsContext, kPreferences_TagFontName,
															sizeof(fontName), fontName, &actualSize);
				if (kPreferences_ResultOK != prefsResult)
				{
					// if unable to find a preference, choose some default
					PLstrcpy(fontName, "\pMonaco");
				}
			}
		}
		terminalError = Terminal_NewScreen(kTerminal_EmulatorVT100, CFSTR("vt100"), 0/* number of scrollback rows */, 1/* number of rows */,
											132/* number of columns */, false/* force save */, &gCommandLineTerminalScreen);
		if (kTerminal_ResultOK == terminalError)
		{
			gCommandLineTerminalView = TerminalView_NewHIViewBased
										(gCommandLineTerminalScreen, gCommandLineWindow, fontName, 14/* font size */);
			assert(nullptr != gCommandLineTerminalView);
			// NOTE: this variable is being reused, as its original user pane is no longer needed
			gCommandLineTerminalViewContainer.setCFTypeRef(TerminalView_ReturnContainerHIView(gCommandLineTerminalView));
			assert(gCommandLineTerminalViewContainer.exists());
			assert_noerr(SetControlID(gCommandLineTerminalViewContainer, &idMyTerminalViewCommandLine));
			HIViewSetFrame(gCommandLineTerminalViewContainer, &floatBounds);
			gCommandLineTerminalViewFocus.setCFTypeRef(TerminalView_ReturnUserFocusHIView(gCommandLineTerminalView));
			assert(gCommandLineTerminalViewFocus.exists());
		}
	}
	
	// find references to all controls that are needed for any operation
	// (button clicks, dealing with text or responding to window resizing)
	{
		gCommandLineTerminalViewContainer = HIViewWrap(idMyTerminalViewCommandLine, gCommandLineWindow);
		assert(gCommandLineTerminalViewContainer.exists());
		gCommandLineHelpText = HIViewWrap(idMyTextCommandHelp, gCommandLineWindow);
		assert(gCommandLineHelpText.exists());
		gCommandLineHelpButton = HIViewWrap(idMyButtonHelp, gCommandLineWindow);
		assert(gCommandLineHelpButton.exists());
		gCommandLineHistoryPopUpMenu = HIViewWrap(idMyMenuHistory, gCommandLineWindow);
		assert(gCommandLineHistoryPopUpMenu.exists());
		gCommandLineArrows = HIViewWrap(idMyLittleArrowsHistory, gCommandLineWindow);
		assert(gCommandLineArrows.exists());
	}
	
	// initialize views
	DialogUtilities_SetUpHelpButton(gCommandLineHelpButton);
	
	{
		error = GetBevelButtonMenuRef(gCommandLineHistoryPopUpMenu, &gCommandLineHistoryMenuRef);
		assert_noerr(error);
		
		// in addition, use a 32-bit icon, if available
		{
			IconManagerIconRef		icon = IconManager_NewIcon();
			
			
			if (nullptr != icon)
			{
				if (noErr == IconManager_MakeIconRef(icon, kOnSystemDisk, kSystemIconsCreator, kRecentItemsIcon))
				{
					(OSStatus)IconManager_SetButtonIcon(gCommandLineHistoryPopUpMenu, icon);
				}
				IconManager_DisposeIcon(&icon);
			}
		}
		
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		// set accessibility information, if possible
		if (FlagManager_Test(kFlagOS10_4API))
		{
			// set accessibility title
			CFStringRef		accessibilityDescCFString = nullptr;
			
			
			if (UIStrings_Copy(kUIStrings_CommandLineHistoryMenuAccessibilityDesc, accessibilityDescCFString).ok())
			{
				HIViewRef const		kViewRef = gCommandLineHistoryPopUpMenu;
				HIObjectRef const	kViewObjectRef = REINTERPRET_CAST(kViewRef, HIObjectRef);
				
				
				error = HIObjectSetAuxiliaryAccessibilityAttribute
						(kViewObjectRef, 0/* sub-component identifier */,
							kAXDescriptionAttribute, accessibilityDescCFString);
				CFRelease(accessibilityDescCFString), accessibilityDescCFString = nullptr;
			}
		}
	#endif
	}
	SetControl32BitMaximum(gCommandLineArrows, kCommandLineCommandHistorySize - 1);
	SetControl32BitMinimum(gCommandLineArrows, 0L);
	SetControl32BitValue(gCommandLineArrows, 0L);
	DeactivateControl(gCommandLineArrows);
	addToHistory(CFSTR(""));
	
	// Set up the Window Info information.
	gCommandLineWindowInfo = WindowInfo_New();
	WindowInfo_SetWindowDescriptor(gCommandLineWindowInfo, kConstantsRegistry_WindowDescriptorCommandLine);
	WindowInfo_SetWindowFloating(gCommandLineWindowInfo, true);
	WindowInfo_SetWindowPotentialDropTarget(gCommandLineWindowInfo, true/* can receive data via drag-and-drop */);
	WindowInfo_SetForWindow(gCommandLineWindow, gCommandLineWindowInfo);
	
	{
		Rect		currentBounds;
		Point		deltaSize;
		
		
		// install a callback that responds as a window is resized
		error = GetWindowBounds(gCommandLineWindow, kWindowContentRgn, &currentBounds);
		assert_noerr(error);
		gCommandLineWindowResizeHandler.install(gCommandLineWindow, handleNewSize, nullptr/* user data */,
												currentBounds.right - currentBounds.left/* minimum width */,
												currentBounds.bottom - currentBounds.top/* minimum height */,
												2048/* arbitrary maximum width */,
												currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(gCommandLineWindowResizeHandler.isInstalled());
		
		// use the preferred rectangle, if any; since a resize handler was
		// installed above, simply resizing the window will cause all
		// controls to be adjusted automatically by the right amount
		SetPt(&deltaSize, currentBounds.right - currentBounds.left,
				currentBounds.bottom - currentBounds.top); // initially...
		(Preferences_Result)Preferences_ArrangeWindow(gCommandLineWindow, kPreferences_WindowTagCommandLine,
														&deltaSize/* on input, initial size; on output, final size */,
														kPreferences_WindowBoundaryLocation |
														kPreferences_WindowBoundaryElementWidth);
	}
	
	// restore the visible state implicitly saved at last Quit
	{
		Boolean		windowIsVisible = false;
		size_t		actualSize = 0L;
		
		
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagWasCommandLineShowing,
									sizeof(windowIsVisible), &windowIsVisible,
									&actualSize))
		{
			windowIsVisible = false; // assume invisible if the preference can’t be found
		}
		
		if (windowIsVisible)
		{
			ShowWindow(gCommandLineWindow);
			TerminalView_FocusForUser(gCommandLineTerminalView);
		}
		else
		{
			HideWindow(gCommandLineWindow);
		}
	}
	
	// install a callback that hides the command line instead of destroying it, when it should be closed
	{
		EventTypeSpec const		whenWindowClosing[] =
								{
									{ kEventClassWindow, kEventWindowClose }
								};
		
		
		gCommandLineWindowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
		error = InstallWindowEventHandler(gCommandLineWindow, gCommandLineWindowClosingUPP, GetEventTypeCount(whenWindowClosing),
											whenWindowClosing, nullptr/* user data */,
											&gCommandLineWindowClosingHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that changes the mouse cursor appropriately
	{
		EventTypeSpec const		whenCursorChangeRequired[] =
								{
									{ kEventClassWindow, kEventWindowCursorChange },
									{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
								};
		
		
		gCommandLineCursorChangeUPP = NewEventHandlerUPP(receiveWindowCursorChange);
		error = InstallWindowEventHandler(gCommandLineWindow, gCommandLineCursorChangeUPP, GetEventTypeCount(whenCursorChangeRequired),
											whenCursorChangeRequired, gCommandLineWindow/* user data */,
											&gCommandLineCursorChangeHandler/* event handler reference */);
		assert_noerr(error);
	}
	
#if 1
	// install a callback that responds to key presses in the terminal view
	{
		EventTypeSpec const		whenTextInput[] =
								{
									{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
								};
		
		
		gCommandLineTextInputUPP = NewEventHandlerUPP(receiveTextInput);
		error = InstallHIObjectEventHandler(gCommandLineTerminalViewFocus.returnHIObjectRef(), gCommandLineTextInputUPP,
											GetEventTypeCount(whenTextInput), whenTextInput, nullptr/* user data */,
											&gCommandLineTextInputHandler/* event handler reference */);
		assert_noerr(error);
	}
#endif
	
	// install a callback that handles history menu item selections
	{
		EventTypeSpec const		whenHistoryMenuItemSelected[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		
		
		gCommandLineHistoryMenuCommandUPP = NewEventHandlerUPP(receiveHistoryCommandProcess);
		error = InstallMenuEventHandler(gCommandLineHistoryMenuRef, gCommandLineHistoryMenuCommandUPP,
										GetEventTypeCount(whenHistoryMenuItemSelected),
										whenHistoryMenuItemSelected, nullptr/* user data */,
										&gCommandLineHistoryMenuCommandHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	// install a callback that handles history arrow clicks
	{
		EventTypeSpec const		whenHistoryArrowClicked[] =
								{
									{ kEventClassControl, kEventControlHit }
								};
		
		
		gCommandLineHistoryArrowClickUPP = NewEventHandlerUPP(receiveHistoryArrowClick);
		error = InstallHIObjectEventHandler(gCommandLineArrows.returnHIObjectRef(), gCommandLineHistoryArrowClickUPP,
											GetEventTypeCount(whenHistoryArrowClicked),
											whenHistoryArrowClicked, nullptr/* user data */,
											&gCommandLineHistoryArrowClickHandler/* event handler reference */);
		assert_noerr(error);
	}
	
#if 0
	// fix a bug in terminal views that makes their initial rendering wrong
	// (but where subsequent resizes correct the glitch)
	{
		Rect	properViewBounds;
		
		
		// just change the size to anything, and then change it back
		// (with the window invisible, nothing will be seen)
		GetControlBounds(gCommandLineTerminalViewContainer, &properViewBounds);
		SizeControl(gCommandLineTerminalViewContainer, 0, 0);
		SizeControl(gCommandLineTerminalViewContainer, properViewBounds.right - properViewBounds.left,
					properViewBounds.bottom - properViewBounds.top);
	}
#endif
	
	// enable drag tracking, because terminal views can handle this
	SetAutomaticControlDragTrackingEnabledForWindow(gCommandLineWindow, true);
}// Init


/*!
This method cleans up the command line window
by destroying data structures allocated by
CommandLine_Init().

(3.0)
*/
void
CommandLine_Done ()
{
	if (nullptr != gCommandLineWindow)
	{
		// save window size and location in preferences
		Preferences_SetWindowArrangementData(gCommandLineWindow, kPreferences_WindowTagCommandLine);
		
		// save visibility preferences implicitly
		{
			Boolean		windowIsVisible = false;
			
			
			windowIsVisible = IsWindowVisible(gCommandLineWindow);
			(Preferences_Result)Preferences_SetData(kPreferences_TagWasCommandLineShowing,
													sizeof(Boolean), &windowIsVisible);
		}
		
		// remove event handlers
		RemoveEventHandler(gCommandLineTextInputHandler);
		DisposeEventHandlerUPP(gCommandLineTextInputUPP);
		RemoveEventHandler(gCommandLineWindowClosingHandler);
		DisposeEventHandlerUPP(gCommandLineWindowClosingUPP);
		RemoveEventHandler(gCommandLineCursorChangeHandler);
		DisposeEventHandlerUPP(gCommandLineCursorChangeUPP);
		RemoveEventHandler(gCommandLineHistoryMenuCommandHandler);
		DisposeEventHandlerUPP(gCommandLineHistoryMenuCommandUPP);
		RemoveEventHandler(gCommandLineHistoryArrowClickHandler);
		DisposeEventHandlerUPP(gCommandLineHistoryArrowClickUPP);
		
		DisposeWindow(gCommandLineWindow); // disposes of the command line window *and* all of its controls
		gCommandLineWindow = nullptr;
		WindowInfo_Dispose(gCommandLineWindowInfo);
	}
}// Done


/*!
Installs a command interpreter for the command line.
You cannot remove an interpreter once it is installed.

The command line does initial filtering of a line of
text, such as checking for URLs.  If these initial
filters come up empty, the command line traverses its
list of interpreters in the order in which they were
added.  The first interpreter whose “is command yours”
message response is "true" gets executed, and the
command line ceases to traverse its list.

A command interpreter should have a mixin HTML source
file for MacTelnet Help, with an index term that is
the same as the command’s name.  This makes the
command’s help show up when any kind of help command
(including help arguments, which are never passed to
your routine) is issued.

For information on how this function should operate,
see “CommandLine.h”.

(3.0)
*/
void
CommandLine_AddInterpreter	(CommandLine_InterpreterProcPtr		inProcPtr)
{
	gCommandLineInterpreterList().push_back(inProcPtr);
}// AddInterpreter


/*!
Returns true only if the command line is visible.  Useful
to avoid auto-initializing this module when doing a simple
query.

(3.1)
*/
Boolean
CommandLine_IsVisible ()
{
	if (nullptr == gCommandLineWindow) return false;
	return IsWindowVisible(gCommandLineWindow);
}// IsVisible


/*!
Returns the Mac OS window for the command line.

WARNING:	This will auto-initialize the module if needed
			(by creating the window).  If you intend to do a
			simple query, look at CommandLine_IsVisible() or
			other similar methods that may not trigger this
			initialization.

(3.0)
*/
WindowRef
CommandLine_ReturnWindow ()
{
	if (nullptr == gCommandLineWindow) CommandLine_Init();
	return gCommandLineWindow;
}// ReturnWindow


/*!
Sets the text displayed in the command line’s
editable text field.  You might use this to
handle a text drag-and-drop into this windoid.

(3.0)
*/
void
CommandLine_SetText		(void const*	inDataPtr,
						 Size 			inSize)
{
	char const*		eraseLineHomeCursor = "\033[2K\033[H"; // VT100 erase-line and home-cursor sequences
	
	
	Terminal_EmulatorProcessCString(gCommandLineTerminalScreen, eraseLineHomeCursor);
	Terminal_EmulatorProcessData(gCommandLineTerminalScreen, REINTERPRET_CAST(inDataPtr, UInt8 const*), inSize);
}// SetText


/*!
Sets the text displayed in the command line’s
editable text field.  You might use this to
handle a text drag-and-drop into this windoid.

(3.0)
*/
void
CommandLine_SetTextWithCFString		(CFStringRef	inString)
{
	char const*		eraseLineHomeCursor = "\033[2K\033[H"; // VT100 erase-line and home-cursor sequences
	
	
	Terminal_EmulatorProcessCString(gCommandLineTerminalScreen, eraseLineHomeCursor);
	Terminal_EmulatorProcessCFString(gCommandLineTerminalScreen, inString);
}// SetTextWithCFString


#pragma mark Internal Methods

/*!
Adds the specified command line to the history buffer,
shifting all commands back one (and therefore destroying
the oldest one) and resetting the history pointer.  The
given string reference is retained.

FUTURE: It may be nice to auto-scan the history strings
and only add a command if it is not already in the list
(or, perhaps better, remove the oldest exact match).

(3.0)
*/
static void
addToHistory	(CFStringRef	inText)
{
	register SInt16		i = 0;
	SInt16 const		kLastItem = kCommandLineCommandHistorySize - 1;
	
	
	gCommandHistory().insert(gCommandHistory().begin(), inText);
	gCommandHistory().resize(kCommandLineCommandHistorySize);
	gCommandHistoryCurrentIndex = -1;
	unless (IsControlActive(gCommandLineArrows)) ActivateControl(gCommandLineArrows);
	DeleteMenuItems(gCommandLineHistoryMenuRef, 1/* first item */, CountMenuItems(gCommandLineHistoryMenuRef)/* number of items to delete */);
	for (i = 0; i <= kLastItem; ++i)
	{
		AppendMenuItemTextWithCFString(gCommandLineHistoryMenuRef, gCommandHistory()[i].returnCFStringRef(),
										0/* attributes */, 0/* command ID */, nullptr/* new item’s index */);
	}
}// addToHistory


/*!
Filters out key presses originally sent to the command line
text field.  For example, if the user presses Return or Enter,
the key is interpreted as “parse this command line” instead of
being passed to the control; if the user presses the up or
down arrow keys, he or she can rotate through past commands.

(3.0)
*/
static ControlKeyFilterResult
commandLineKeyFilter	(UInt32*	inoutKeyCode)
{
	ControlKeyFilterResult	result = kControlKeyFilterPassKey;
	CFRetainRelease			commandLineText;
	CFIndex					commandLineTextSize = 0L;
	
	
	// determine the text currently in the command line
	commandLineText.setCFTypeRef(getTextAsCFString());
	if (commandLineText.exists())
	{
		commandLineTextSize = CFStringGetLength(commandLineText.returnCFStringRef());
	}
	
	if ((false == commandLineText.exists()) || (0 == commandLineTextSize))
	{
		// when empty, restore original biline string
		CFStringRef			defaultHelpText = nullptr;
		Boolean				releaseDefaultHelpText = true;
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		
		
		stringResult = UIStrings_Copy(kUIStrings_CommandLineHelpTextDefault, defaultHelpText);
		if (false == stringResult.ok())
		{
			defaultHelpText = CFSTR("");
			releaseDefaultHelpText = false;
		}
		setBilineText(defaultHelpText);
		if (releaseDefaultHelpText)
		{
			CFRelease(defaultHelpText), defaultHelpText = nullptr;
		}
	}
	
	// Some actions are based on the physical key, usually
	// because it is easier to do so than checking ASCII
	// codes (or there is no ASCII code).  Check for these
	// keys first.
	if (nullptr != inoutKeyCode)
	{
		switch (*inoutKeyCode)
		{
		case 0x31: // space
			// try to describe what will happen when this command is executed
			if (commandLineTextSize > 0)
			{
				CFArrayRef		argv = nullptr;
				
				
				// take apart the command line in its present state, creating a set of words
				argv = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, commandLineText.returnCFStringRef(),
																CFSTR(" ")/* separator */);
				if (nullptr != argv)
				{
					CommandLine_InterpreterProcPtr	procPtr = nullptr;
					UIStrings_Result				stringResult = kUIStrings_ResultOK;
					CFRetainRelease					finalText;
					Boolean							haveDescription = false;
					
					
					// is the command even recognized?
					if (issueMessageToAllInterpreters(kCommandLine_InterpreterMessageIsCommandYours, argv, nullptr, &procPtr)
						&& (nullptr != procPtr))
					{
						CFStringRef		descriptionText = nullptr;
						CFStringRef		templateText = nullptr;
						
						
						haveDescription = CommandLine_InvokeInterpreterProc
											(procPtr, argv, &descriptionText,
												kCommandLine_InterpreterMessageDescribeLastArgument);
						if (haveDescription)
						{
							CFRetainRelease		commandName(CFArrayGetValueAtIndex(argv, 0));
							
							
							stringResult = UIStrings_Copy(kUIStrings_CommandLineHelpTextCommandTemplate, templateText);
							if (stringResult.ok())
							{
								finalText.setCFTypeRef(CFStringCreateWithFormat
														(kCFAllocatorDefault, nullptr/* format options */,
															templateText, commandName.returnCFStringRef(), descriptionText));
							}
							else
							{
								finalText.setCFTypeRef(CFSTR(""));
							}
						}
						else if (CFArrayGetCount(argv) > 1) // that is, the missing description is for an argument, not the entire command
						{
							CFRetainRelease		commandName(CFArrayGetValueAtIndex(argv, 0));
							CFRetainRelease		argumentName(CFArrayGetValueAtIndex(argv, CFArrayGetCount(argv) - 1));
							
							
							stringResult = UIStrings_Copy(kUIStrings_CommandLineHelpTextCommandArgumentTemplate, templateText);
							if (stringResult.ok())
							{
								finalText.setCFTypeRef(CFStringCreateWithFormat
														(kCFAllocatorDefault, nullptr/* format options */,
															templateText, commandName.returnCFStringRef(),
															argumentName.returnCFStringRef(), descriptionText));
							}
							else
							{
								finalText.setCFTypeRef(CFSTR(""));
							}
						}
					}
					
					// in case the callback screws up, fix the description flag
					if (false == finalText.exists()) haveDescription = false;
					
					if ((!haveDescription) && (1 == CFArrayGetCount(argv)))
					{
						CFStringRef		localizedString = nullptr;
						
						
						// uninterpreted text - inform the user
						stringResult = UIStrings_Copy(kUIStrings_CommandLineHelpTextFreeInput, localizedString);
						if (stringResult.ok())
						{
							finalText.setCFTypeRef(localizedString);
						}
						else
						{
							finalText.setCFTypeRef(CFSTR(""));
						}
					}
					setBilineText(finalText.returnCFStringRef());
					CFRelease(argv), argv = nullptr;
				}
			}
			break;
		
		case 0x24: // Return key
		case 0x4C: // Enter key
	#if RECOGNIZE_TABS
		case 0x30: // tab
	#endif
		case 0x33: // delete (only passed if the line is empty)
			// If the user presses Return, Enter or tab, the command text should be executed.
			// In the sole case where the command line is empty, a delete key press is also
			// significant because it should be sent immediately to the server.
			// If the command “makes sense”, the text field is cleared and it executes;
			// otherwise, the text remains and the system emits a beep.
			if (((commandLineTextSize >= 0) && (0x33/* delete */ != *inoutKeyCode)) ||
				((0 == commandLineTextSize) && (0x33/* delete */ == *inoutKeyCode)))
			{
				// was command sent successfully?
				if (parseCommandLine(commandLineText.returnCFStringRef(), keyCodeToTerminatorType(*inoutKeyCode)))
				{
					// clear the text field
					CommandLine_SetText("", 0);
					
					// for entered commands, restore the original biline string; for tabbed commands, display everything so far
				#if RECOGNIZE_TABS
					if (0x30/* tab */ == *inoutKeyCode)
					{
						setBilineText(commandLineText.returnCFStringRef());
					}
					else
				#endif
					{
						CFStringRef			defaultHelpText = nullptr;
						Boolean				releaseDefaultHelpText = true;
						UIStrings_Result	stringResult = kUIStrings_ResultOK;
						
						
						stringResult = UIStrings_Copy(kUIStrings_CommandLineHelpTextDefault, defaultHelpText);
						if (false == stringResult.ok())
						{
							defaultHelpText = CFSTR("");
							releaseDefaultHelpText = false;
						}
						setBilineText(defaultHelpText);
						if (releaseDefaultHelpText)
						{
							CFRelease(defaultHelpText), defaultHelpText = nullptr;
						}
					}
				}
				else
				{
					Sound_StandardAlert();
				}
				result = kControlKeyFilterBlockKey;
			}
			break;
		
		case 0x7E: // Up arrow
		case 0x3E: // Up arrow on non-extended keyboards
			// If the user presses an up arrow key, set the
			// command line text to the previous command in
			// the history buffer rotation.
			unless (rotateHistory(+1)) Sound_StandardAlert();
			result = kControlKeyFilterBlockKey;
			break;
		
		case 0x7D: // Down arrow
		case 0x3D: // Down arrow on non-extended keyboards
			// If the user presses a down arrow key, set the
			// command line text to the next command in the
			// history buffer rotation.
			unless (rotateHistory(-1)) Sound_StandardAlert();
			result = kControlKeyFilterBlockKey;
			break;
		
		default:
			// uninteresting key was hit
			break;
		}
	}
	
	return result;
}// commandLineKeyFilter


/*!
Provides a copy of the text in the command line’s
text field.  A new CFString is returned; you must
call CFRelease() on the string yourself.

(3.0)
*/
static CFStringRef
getTextAsCFString ()
{
	CFStringRef			result = nullptr;
	Terminal_LineRef	lineIterator = nullptr;
	Terminal_Result		getResult = kTerminal_ResultOK;
	
	
	// NOTE: A future Terminal API will return a CFString directly and can be used here.
	lineIterator = Terminal_NewMainScreenLineIterator(gCommandLineTerminalScreen, 0/* row */);
	if (nullptr != lineIterator)
	{
		UniChar const*	startPtr = nullptr;
		UniChar const*	pastEndPtr = nullptr;
		
		
		getResult = Terminal_GetLine(gCommandLineTerminalScreen, lineIterator/* which row */,
										startPtr, pastEndPtr);
		if (kTerminal_ResultOK == getResult)
		{
			result = CFStringCreateWithCharacters(kCFAllocatorDefault, startPtr, pastEndPtr - startPtr);
		}
		Terminal_DisposeLineIterator(&lineIterator);
	}
	
	return result;
}// getTextAsCFString


/*!
This method moves and resizes controls in response to
a resize of the floating command line window.

(3.0)
*/
static void
handleNewSize	(WindowRef	inWindowRef,
				 Float32	inDeltaSizeX,
				 Float32	inDeltaSizeY,
				 void*		UNUSED_ARGUMENT(inData))
{
	SInt32  truncDeltaX = STATIC_CAST(inDeltaSizeX, SInt32);
	SInt32  truncDeltaY = STATIC_CAST(inDeltaSizeY, SInt32);
	
	
	DialogAdjust_BeginControlAdjustment(inWindowRef);
	DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gCommandLineArrows, truncDeltaX);
	DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gCommandLineHistoryPopUpMenu, truncDeltaX);
	DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, TerminalView_ReturnContainerHIView(gCommandLineTerminalView), truncDeltaX);
	DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gCommandLineHelpText, truncDeltaX);
	DialogAdjust_EndAdjustment(truncDeltaX, truncDeltaY);
}// handleNewSize


/*!
Iterates the linked list of command interpreters, issuing
each one the given message.  The first one that returns
"true" to that message stops the list traversal, and that
interpreter is returned.  If "true" is ever returned, this
method returns "true"; otherwise, this method returns
"false" to indicate that no interpreters responded to the
given message.

(3.0)
*/
static Boolean
issueMessageToAllInterpreters	(CommandLine_InterpreterMessage		inMessage,
								 CFArrayRef							inTokens,
								 void*								outDataPtr,
								 CommandLine_InterpreterProcPtr*	outProcPtrPtr)
{
	CommandLine_InterpreterProcPtr			procPtr = nullptr;
	CommandInterpreterList::const_iterator	interpreterIterator;
	Boolean									result = false;
	Boolean									done = false;
	
	
	Cursors_UseWatch();
	for (interpreterIterator = gCommandLineInterpreterList().begin();
			(!done) && (interpreterIterator != gCommandLineInterpreterList().end()); ++interpreterIterator)
	{
		procPtr = *interpreterIterator;
		if (nullptr != procPtr) done = CommandLine_InvokeInterpreterProc(procPtr, inTokens, outDataPtr, inMessage);
		if (done) result = true;
	}
	
	if (nullptr != outProcPtrPtr) *outProcPtrPtr = procPtr;
	Cursors_UseArrow();
	
	return result;
}// issueMessageToAllInterpreters


/*!
Returns the likely mechanism for terminating the
text sent to the session, given the key that was
pressed to trigger the send.

(3.1)
*/
static inline My_TerminatorType
keyCodeToTerminatorType		(SInt16		inVirtualKeyCode)
{
	My_TerminatorType	result = kMy_TerminatorTypeNewline;
	
	
	switch (inVirtualKeyCode)
	{
#if RECOGNIZE_TABS
	case 0x30: // tab
		result = kMy_TerminatorTypeTab;
		break;
#endif
	
	case 0x33: // delete
		result = kMy_TerminatorTypeBackspace;
		break;
	
	default:
		break;
	}
	return result;
}// keyCodeToTerminatorType


/*!
Parses the given string as a command line.  If the
command line is meaningful, it is executed and true
is returned; otherwise, false is returned to
indicate that the command is unrecognizable in some
way.

If valid, the command line is added to the command
history list and the oldest command is discarded.

(3.0)
*/
static Boolean
parseCommandLine	(CFStringRef		inText,
					 My_TerminatorType	inTerminator)
{
	Boolean		result = false;
	Boolean		isBlank = true;
	
	
	if (nullptr != inText)
	{
		isBlank = (0 == CFStringGetLength(inText));
		
		// check for URLs first
		if (kNotURL != URL_ReturnTypeFromCFString(inText))
		{
			try
			{
				// use the Quills interface to handle the URL so that
				// any installed handlers written in a scripting language
				// will be invoked first for this type of URL
				CFStringEncoding const	kEncoding = kCFStringEncodingUTF8;
				CFIndex const			kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
														(CFStringGetLength(inText), kEncoding);
				char*					buffer = new char[kBufferSize];
				
				
				if (CFStringGetCString(inText, buffer, kBufferSize, kEncoding))
				{
					Quills::Session::handle_url(buffer);
					result = true;
				}
				delete [] buffer;
			}
			catch (std::bad_alloc)
			{
				result = false;
			}
		}
		else
		{
			// if not a URL, check for UNIX commands
			CFArrayRef		argv = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, inText, CFSTR(" ")/* separator */);
			
			
			if (nullptr != argv)
			{
				result = parseUNIX(argv);
				CFRelease(argv), argv = nullptr;
			}
		}
		
		// if all else fails, send the string to the frontmost session window (if any)
		unless (result)
		{
			SessionRef		session = SessionFactory_ReturnUserFocusSession();
			
			
			if (nullptr == session)
			{
				// apparently no session to send input to; this should not
				// happen unless every terminal window is either closed or
				// hidden from view (obscured)
				Sound_StandardAlert();
			}
			else
			{
				Session_UserInputCFString(session, inText, true/* record */);
				if (kMy_TerminatorTypeNewline == inTerminator)
				{
					Session_SendNewline(session, kSession_EchoCurrentSessionValue);
					// UNIMPLEMENTED - need to record a “send-newline” in scripts, but there’s no way to do it right now
				}
				else if (kMy_TerminatorTypeTab == inTerminator)
				{
					char const*		tabTerminatorString = "\011";
					
					
					Session_UserInputString(session, tabTerminatorString, CPP_STD::strlen(tabTerminatorString) * sizeof(char),
											true/* record */);
				}
				else if (kMy_TerminatorTypeBackspace == inTerminator)
				{
					char const*		deleteTerminatorString = "\010";
					
					
					Session_UserInputString(session, deleteTerminatorString,
											CPP_STD::strlen(deleteTerminatorString) * sizeof(char), true/* record */);
				}
				else
				{
					// ???
				}
				result = true;
			}
		}
	}
	
	// only remember valid command lines
	if ((result) && (false == isBlank))
	{
		addToHistory(inText);
	}
	
	return result;
}// parseCommandLine


/*!
Parses the specified arguments, and if they ALL make sense,
performs the requested operation and returns true.  Otherwise,
false is returned (and the command line will then emit a
system beep).

Just as in a Unix main() function, the zeroeth element of the
array is the command name.

(3.0)
*/
static Boolean
parseUNIX	(CFArrayRef		argv)
{
	Boolean		result = false;
	
	
	if (CFArrayGetCount(argv) > 0)
	{
		// Iterate the linked list of command interpreters, asking each one if it can recognize
		// the command given.  The first one that returns "true" to that message gets sent the
		// “execute” message, and the list traversal stops.
		CommandLine_InterpreterProcPtr	procPtr = nullptr;
		
		
		if (issueMessageToAllInterpreters(kCommandLine_InterpreterMessageIsCommandYours, argv, nullptr, &procPtr))
		{
			if (nullptr != procPtr) result = CommandLine_InvokeInterpreterProc(procPtr, argv, nullptr,
																				kCommandLine_InterpreterMessageExecute);
		}
	}
	return result;
}// parseUNIX


/*!
Handles "kEventControlHit" of "kEventClassControl" for
the command line’s history arrows.  Responds by updating
the current position in the history list and updating
the command line display accordingly.  Effectively works
the same as up-arrow or down-arrow key presses.

(3.0)
*/
static pascal OSStatus
receiveHistoryArrowClick	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHit);
	{
		ControlRef		control = nullptr;
		
		
		// determine the control in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			// presumably this is the history arrow control!
			assert(control == gCommandLineArrows);
			
			{
				ControlPartCode		partCode = kControlNoPart;
				
				
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
				if (noErr != result) partCode = kControlUpButtonPart;
				
				switch (partCode)
				{
				case kControlDownButtonPart:
					rotateHistory(-1);
					break;
				
				case kControlUpButtonPart:
				default:
					rotateHistory(+1);
					break;
				}
			}
		}
	}
	
	return result;
}// receiveHistoryArrowClick


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the command line history menu.  Responds by updating
the command line to match the contents of the selected
item - however, the event is then passed on so that the
default handler can do other things (like update the
pop-up menu’s currently-checked item).

(3.0)
*/
static pascal OSStatus
receiveHistoryCommandProcess	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	commandInfo;
		
		
		// determine the menu and menu item to process
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, commandInfo);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			// presumably this is the history menu!
			assert(commandInfo.menu.menuRef == gCommandLineHistoryMenuRef);
			assert(commandInfo.menu.menuItemIndex >= 1);
			
			gCommandHistoryCurrentIndex = commandInfo.menu.menuItemIndex - 1; // internal index is zero-based
			{
				CFStringRef		command = gCommandHistory()[gCommandHistoryCurrentIndex].returnCFStringRef();
				
				
				CommandLine_SetTextWithCFString((nullptr == command) ? CFSTR("") : command);
				TerminalView_FocusForUser(gCommandLineTerminalView);
			}
		}
	}
	
	return result;
}// receiveHistoryCommandProcess


/*!
Handles "kEventTextInputUnicodeForKeyEvent" of "kEventClassTextInput"
for the command line.  Arrow keys are used to rotate through history,
whereas Tab, Return and Enter will cause data to be sent.  A backspace
will delete a character in the field or, when the field is empty, send
a delete character.

(3.0)
*/
static pascal OSStatus
receiveTextInput	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	{
		EventRef	originalKeyPressEvent = nullptr;
		
		
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent,
														typeEventRef, originalKeyPressEvent);
		assert_noerr(result);
		{
			UInt32		keyCode = 0;
			UInt32		numberOfCharacters = 0;
			UInt8		characterCodes[2] = { '\0', '\0' };
			
			
			// determine the key codes
			result = CarbonEventUtilities_GetEventParameter(originalKeyPressEvent, kEventParamKeyCode, typeUInt32, keyCode);
			if (noErr == result)
			{
				result = CarbonEventUtilities_GetEventParameterVariableSize
							(originalKeyPressEvent, kEventParamKeyMacCharCodes, typeChar, characterCodes,
								numberOfCharacters);
			}
			
			// if the key codes were found, proceed
			if (noErr == result)
			{
			#if ! RECOGNIZE_TABS
				// when tabs are not sent to the terminal, they should not be
				// handled AT ALL so that the parent handler will perform
				// view focus changes, etc.
				if (0x30/* tab */ == keyCode)
				{
					result = eventNotHandledErr;
				}
				else
			#endif
				if (kControlKeyFilterPassKey == commandLineKeyFilter(&keyCode))
				{
					// not special - just type the character(s)
					Terminal_EmulatorProcessData(gCommandLineTerminalScreen, characterCodes, numberOfCharacters);
					
					// when backspacing, delete the last character
					if (0x33/* backspace virtual key code */ == keyCode)
					{
						char const*		terminalEraseEndOfLineSequence = "\033[K";
						
						
						Terminal_EmulatorProcessCString(gCommandLineTerminalScreen, terminalEraseEndOfLineSequence);
					}
					result = noErr; // event is completely handled
				}
				else
				{
					// key was blocked; it has special meaning
					// (but this is handled by the key filter)
					result = noErr; // event is completely handled
				}
			}
		}
	}
	
	return result;
}// receiveTextInput


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for the command line.  The default handler destroys
windows, but the command line should only be hidden
or shown (as it always remains in memory).

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
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (noErr == result)
		{
			if (window == gCommandLineWindow)
			{
				Commands_ExecuteByID(kCommandHideCommandLine);
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
Handles "kEventWindowCursorChange" of "kEventClassWindow",
or "kEventRawKeyModifiersChanged" of "kEventClassKeyboard",
for a terminal window.

IMPORTANT:	This is completely generic.  It should move
			into some other module, so it can be used as
			a utility for other windows.

(3.1)
*/
static pascal OSStatus
receiveWindowCursorChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inWindowRef)
{
	OSStatus		result = eventNotHandledErr;
	WindowRef		targetWindow = REINTERPRET_CAST(inWindowRef, WindowRef);
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(((kEventClass == kEventClassWindow) && (kEventKind == kEventWindowCursorChange)) ||
			((kEventClass == kEventClassKeyboard) && (kEventKind == kEventRawKeyModifiersChanged)));
	
	// since the window floats, change the cursor even if it is not focused
	//if (GetUserFocusWindow() == targetWindow)
	{
		UInt32		modifiers = 0;
		Boolean		wasSet = false;
		
		
		if (kEventClass == kEventClassWindow)
		{
			WindowRef	window = nullptr;
			
			
			// determine the window in question
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
			if (noErr == result)
			{
				Point	globalMouse;
				
				
				Cursors_UseArrow();
				assert(window == targetWindow);
				result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, globalMouse);
				if (noErr == result)
				{
					// try to vary the cursor according to key modifiers, but it’s no
					// catastrophe if this information isn’t available
					(OSStatus)CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
					
					// finally, set the cursor; make the simplifying assumption that the
					// only view that cares about the cursor is the terminal view itself
					{
						Point	localMouse = globalMouse;
						
						
						QDGlobalToLocalPoint(GetWindowPort(targetWindow), &localMouse);
						result = HandleControlSetCursor(gCommandLineTerminalViewFocus, localMouse, modifiers, &wasSet);
					}
				}
			}
		}
		else
		{
			// when the key modifiers change, it is still nice to have the
			// cursor automatically change when necessary; however, there
			// is no mouse information available, so it must be determined
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
			if (noErr == result)
			{
				Point	globalMouse;
				
				
				GetMouse(&globalMouse);
				{
					Point	localMouse = globalMouse;
					
					
					QDGlobalToLocalPoint(GetWindowPort(targetWindow), &localMouse);
					result = HandleControlSetCursor(gCommandLineTerminalViewFocus, localMouse, modifiers, &wasSet);
				}
			}
		}
	}
	
	return result;
}// receiveWindowCursorChange


/*!
Adjusts the command line history pointer by so
many positions in one direction or the other.
If the request can be fulfilled (that is, there
are enough commands on one side of the buffer
pointer to be able to move it), true is returned
and the command line text field is changed to
reflect the current history command.  Otherwise,
false is returned and the text field is left
untouched.

A negative number will move the history pointer
towards the present.  A positive number will
move the history pointer back in time.  The
control value of the “little arrows” control is
the history pointer.

(3.0)
*/
static Boolean
rotateHistory	(SInt16		inHowFarWhichWay)
{
	Boolean		result = false;
	
	
	// the requested adjustment must land the history index in the valid range to be acceptable
	result = (((gCommandHistoryCurrentIndex + inHowFarWhichWay) >= -1) &&
				((gCommandHistoryCurrentIndex + inHowFarWhichWay) < kCommandLineCommandHistorySize));
	
	if (result)
	{
		CFStringRef		command = nullptr;
		
		
		gCommandHistoryCurrentIndex += inHowFarWhichWay;
		SetControl32BitValue(gCommandLineArrows, gCommandHistoryCurrentIndex);
		
		if (gCommandHistoryCurrentIndex >= 0) command = gCommandHistory()[gCommandHistoryCurrentIndex].returnCFStringRef();
		
		CommandLine_SetTextWithCFString((nullptr == command) ? CFSTR("") : command);
		//TerminalView_FocusForUser(gCommandLineTerminalView);
	}
	return result;
}// rotateHistory


/*!
Use this method to specify the text underneath the
editable text field in the command line windoid,
using a Core Foundation string reference.

(3.0)
*/
static void
setBilineText	(CFStringRef	inContents)
{
	// set the control text
	SetControlTextWithCFString(gCommandLineHelpText, inContents);
	DrawOneControl(gCommandLineHelpText);
}// setBilineText

// BELOW IS REQUIRED NEWLINE TO END FILE
