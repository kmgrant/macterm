/*###############################################################

	Keypads.cp
	
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

// standard-C++ includes
#include <map>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <CommonEventHandlers.h>
#include <HIViewWrap.h>
#include <IconManager.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <NIBLoader.h>

// resource includes
#include "MenuResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConnectionData.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Keypads.h"
#include "Preferences.h"
#include "SessionFactory.h"
#include "UIStrings.h"
#include "VTKeys.h"



#pragma mark Types

typedef std::map< UInt32, UInt8 >	CommandIDToCharacterOrKeyMap;	//!< if more than 127, code is special key; otherwise, ASCII

#pragma mark Internal Method Prototypes

static SessionRef			getCurrentSession					();
static void					makeAllButtonsUseTheSystemFont		(WindowRef);
static pascal OSStatus		receiveHICommand					(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowClosing				(EventHandlerCallRef, EventRef, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowRef						gControlKeysWindow = nullptr;
	WindowRef						gFunctionKeysWindow = nullptr;
	WindowRef						gVT220KeypadWindow = nullptr;
	EventHandlerUPP					gButtonHICommandsUPP = nullptr; //!< wrapper for button callback function
	EventHandlerUPP					gWindoidClosingUPP = nullptr; //!< wrapper for window-close callback function
	EventHandlerRef					gControlKeysButtonHICommandsHandler = nullptr; //!< invoked when a keypad button is clicked
	EventHandlerRef					gFunctionKeysButtonHICommandsHandler = nullptr; //!< invoked when a keypad button is clicked
	EventHandlerRef					gVT220KeypadButtonHICommandsHandler = nullptr; //!< invoked when a keypad button is clicked
	EventHandlerRef					gControlKeysWindoidClosingHandler = nullptr; //!< invoked when Control Keys is about to close
	EventHandlerRef					gFunctionKeysWindoidClosingHandler = nullptr; //!< invoked when Function Keys is about to close
	EventHandlerRef					gVT220KeypadWindoidClosingHandler = nullptr; //!< invoked when VT220 Keypad is about to close
	Boolean							gInited = false;
	CommandIDToCharacterOrKeyMap&	gCommandIDToCharacterOrKeyMap()	{ static CommandIDToCharacterOrKeyMap x; return x; }
}



#pragma mark Public Methods

/*!
Creates a number of windoids that contain a set of
keys that can be “clicked”.   The windows are
automatically positioned according to any existing
user preferences (implicitly saved by Keypads_Done()),
and displayed (if the user left them visible when
last quitting).

(3.0)
*/
void
Keypads_Init ()
{
	Point		deltaSize;
	Boolean		windowIsVisible;
	size_t		actualSize = 0L;
	
	
	// assign all key values
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlAtSign] = 0x00;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlA] = 0x01;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlB] = 0x02;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlC] = 0x03;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlD] = 0x04;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlE] = 0x05;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlF] = 0x06;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlG] = 0x07;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlH] = 0x08;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlI] = 0x09;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlJ] = 0x0A;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlK] = 0x0B;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlL] = 0x0C;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlM] = 0x0D;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlN] = 0x0E;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlO] = 0x0F;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlP] = 0x10;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlQ] = 0x11;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlR] = 0x12;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlS] = 0x13;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlT] = 0x14;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlU] = 0x15;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlV] = 0x16;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlW] = 0x17;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlX] = 0x18;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlY] = 0x19;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlZ] = 0x1A;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlLeftSquareBracket] = 0x1B;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlBackslash] = 0x1C;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlRightSquareBracket] = 0x1D;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlTilde] = 0x1E;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadControlQuestionMark] = 0x1F;
	//gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction1] = ?;
	//gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction2] = ?;
	//gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction3] = ?;
	//gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction4] = ?;
	//gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction5] = ?;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction6] = VSF6;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction7] = VSF7;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction8] = VSF8;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction9] = VSF9;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction10] = VSF10;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction11] = VSF11;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction12] = VSF12;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction13] = VSF13;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction14] = VSF14;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction15] = VSF15;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction16] = VSF16;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction17] = VSF17;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction18] = VSF18;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction19] = VSF19;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFunction20] = VSF20;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadFind] = VSHELP;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadInsert] = VSHOME;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadDelete] = VSPGUP;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadSelect] = VSDEL;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadPageUp] = VSEND;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadPageDown] = VSPGDN;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadLeftArrow] = VSLT;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadUpArrow] = VSUP;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadDownArrow] = VSDN;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadRightArrow] = VSRT;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadProgrammableFunction1] = VSF1;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadProgrammableFunction2] = VSF2;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadProgrammableFunction3] = VSF3;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadProgrammableFunction4] = VSF4;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad0] = VSK0;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad1] = VSK1;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad2] = VSK2;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad3] = VSK3;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad4] = VSK4;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad5] = VSK5;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad6] = VSK6;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad7] = VSK7;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad8] = VSK8;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypad9] = VSK9;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadPeriod] = VSKP;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadComma] = VSKC;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadDash] = VSKM;
	gCommandIDToCharacterOrKeyMap()[kCommandKeypadEnter] = VSKE;
	assert(gCommandIDToCharacterOrKeyMap().size() == 75); // set to the number of insertions done above
	
	//
	// keypad window creation
	//
	
	// load the NIB containing the windoids (automatically finds the right localization)
	gControlKeysWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("Keypads"), CFSTR("ControlKeys")) << NIBLoader_AssertWindowExists;
	gFunctionKeysWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("Keypads"), CFSTR("FunctionKeys")) << NIBLoader_AssertWindowExists;
	gVT220KeypadWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("Keypads"), CFSTR("VT220Keypad")) << NIBLoader_AssertWindowExists;
	
	//
	// Control Keys windoid
	//
	
	// for each button in the window, change its font size to be more readable
	makeAllButtonsUseTheSystemFont(gControlKeysWindow);
	
	SetPt(&deltaSize, 0, 0); // initially...
	if (kPreferences_ResultOK != Preferences_ArrangeWindow(gControlKeysWindow, kPreferences_WindowTagControlKeypad,
															&deltaSize, kPreferences_WindowBoundaryLocation))
	{
		// no preference found, so pick a default location
		// (use NIB-provided location)
		//MoveWindow(gControlKeysWindow, 200, 300, false/* activate */);
	}
	unless (Preferences_GetData(kPreferences_TagWasControlKeypadShowing, sizeof(windowIsVisible),
								&windowIsVisible, &actualSize) == kPreferences_ResultOK)
	{
		windowIsVisible = false; // assume invisible if the preference can’t be found
	}
	if (windowIsVisible) ShowWindow(gControlKeysWindow);
	
	//
	// Function Keys windoid
	//
	
	// for each button in the window, change its font size to be more readable
	makeAllButtonsUseTheSystemFont(gFunctionKeysWindow);
	
	SetPt(&deltaSize, 0, 0); // initially...
	if (kPreferences_ResultOK != Preferences_ArrangeWindow(gFunctionKeysWindow, kPreferences_WindowTagFunctionKeypad,
															&deltaSize, kPreferences_WindowBoundaryLocation))
	{
		// no preference found, so pick a default location
		// (use NIB-provided location)
		//MoveWindow(gFunctionKeysWindow, 450, 70, false/* activate */);
	}
	unless (Preferences_GetData(kPreferences_TagWasFunctionKeypadShowing, sizeof(windowIsVisible),
								&windowIsVisible, &actualSize) == kPreferences_ResultOK)
	{
		windowIsVisible = false; // assume invisible if the preference can’t be found
	}
	if (windowIsVisible) ShowWindow(gFunctionKeysWindow);
	
	//
	// VT220 Keys windoid
	//
	
	// for each button in the window, change its font size to be more readable
	makeAllButtonsUseTheSystemFont(gVT220KeypadWindow);
	
	// now install icons for certain command buttons
	{
		HIViewWrap				enterButton(HIViewIDWrap('KPEn'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (enterButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadEnterIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadEnter);
			IconManager_SetButtonIcon(enterButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				arrowButton(HIViewIDWrap('KPAD'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (arrowButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadArrowDownIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadArrowDown);
			IconManager_SetButtonIcon(arrowButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				arrowButton(HIViewIDWrap('KPAL'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (arrowButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadArrowLeftIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadArrowLeft);
			IconManager_SetButtonIcon(arrowButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				arrowButton(HIViewIDWrap('KPAR'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (arrowButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadArrowRightIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadArrowRight);
			IconManager_SetButtonIcon(arrowButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				arrowButton(HIViewIDWrap('KPAU'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (arrowButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadArrowUpIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadArrowUp);
			IconManager_SetButtonIcon(arrowButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				keypadButton(HIViewIDWrap('KPDl'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (keypadButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadDeleteIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadDelete);
			IconManager_SetButtonIcon(keypadButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				keypadButton(HIViewIDWrap('KPFn'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (keypadButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadFindIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadFind);
			IconManager_SetButtonIcon(keypadButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				keypadButton(HIViewIDWrap('KPIn'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (keypadButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadInsertIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadInsert);
			IconManager_SetButtonIcon(keypadButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				pageButton(HIViewIDWrap('KPPD'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (pageButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadPageDownIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadPageDown);
			IconManager_SetButtonIcon(pageButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				pageButton(HIViewIDWrap('KPPU'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (pageButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadPageUpIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadPageUp);
			IconManager_SetButtonIcon(pageButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	{
		HIViewWrap				keypadButton(HIViewIDWrap('KPSl'), gVT220KeypadWindow);
		IconManagerIconRef		iconRef = IconManager_NewIcon();
		
		
		if ((nullptr != iconRef) && (keypadButton.exists()))
		{
			(OSStatus)IconManager_MakeIconRefFromBundleFile
						(iconRef, AppResources_ReturnKeypadSelectIconFilenameNoExtension(),
							AppResources_ReturnCreatorCode(),
							kConstantsRegistry_IconServicesIconKeypadSelect);
			IconManager_SetButtonIcon(keypadButton, iconRef);
			
			IconManager_DisposeIcon(&iconRef);
		}
	}
	
	SetPt(&deltaSize, 0, 0); // initially...
	if (kPreferences_ResultOK != Preferences_ArrangeWindow(gVT220KeypadWindow, kPreferences_WindowTagVT220Keypad,
															&deltaSize, kPreferences_WindowBoundaryLocation))
	{
		// no preference found, so pick a default location
		// (use NIB-provided location)
		//MoveWindow(gVT220KeypadWindow, 200, 70, false/* activate */);
	}
	unless (Preferences_GetData(kPreferences_TagWasVT220KeypadShowing, sizeof(windowIsVisible),
								&windowIsVisible, &actualSize) == kPreferences_ResultOK)
	{
		windowIsVisible = false; // assume invisible if the preference can’t be found
	}
	if (windowIsVisible) ShowWindow(gVT220KeypadWindow);
	
	// install a callback that responds to buttons in each keypad window
	{
		EventTypeSpec const		whenButtonClicked[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		OSStatus				error = noErr;
		
		
		gButtonHICommandsUPP = NewEventHandlerUPP(receiveHICommand);
		error = InstallWindowEventHandler(gControlKeysWindow, gButtonHICommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, gControlKeysWindow/* user data */,
											&gControlKeysButtonHICommandsHandler/* event handler reference */);
		error = InstallWindowEventHandler(gFunctionKeysWindow, gButtonHICommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, gFunctionKeysWindow/* user data */,
											&gFunctionKeysButtonHICommandsHandler/* event handler reference */);
		error = InstallWindowEventHandler(gVT220KeypadWindow, gButtonHICommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, gVT220KeypadWindow/* user data */,
											&gVT220KeypadButtonHICommandsHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that hides a keypad instead of destroying it, when it should be closed
	{
		EventTypeSpec const		whenWindowClosing[] =
								{
									{ kEventClassWindow, kEventWindowClose }
								};
		OSStatus				error = noErr;
		
		
		gWindoidClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
		error = InstallWindowEventHandler(gControlKeysWindow, gWindoidClosingUPP, GetEventTypeCount(whenWindowClosing),
											whenWindowClosing, nullptr/* user data */,
											&gControlKeysWindoidClosingHandler/* event handler reference */);
		error = InstallWindowEventHandler(gFunctionKeysWindow, gWindoidClosingUPP, GetEventTypeCount(whenWindowClosing),
											whenWindowClosing, nullptr/* user data */,
											&gFunctionKeysWindoidClosingHandler/* event handler reference */);
		error = InstallWindowEventHandler(gVT220KeypadWindow, gWindoidClosingUPP, GetEventTypeCount(whenWindowClosing),
											whenWindowClosing, nullptr/* user data */,
											&gVT220KeypadWindoidClosingHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	gInited = true;
}// Init


/*!
Invoke this routine when you are finished with
keypads.  The locations of all keypad windoids
are saved, and then the windoids are destroyed.

(3.0)
*/
void
Keypads_Done ()
{
	Boolean		windowIsVisible = false;
	
	
	gInited = false;
	Preferences_SetWindowArrangementData(gControlKeysWindow, kPreferences_WindowTagControlKeypad);
	Preferences_SetWindowArrangementData(gFunctionKeysWindow, kPreferences_WindowTagFunctionKeypad);
	Preferences_SetWindowArrangementData(gVT220KeypadWindow, kPreferences_WindowTagVT220Keypad);
	windowIsVisible = IsWindowVisible(gControlKeysWindow);
	(Preferences_Result)Preferences_SetData(kPreferences_TagWasControlKeypadShowing,
											sizeof(Boolean), &windowIsVisible);
	windowIsVisible = IsWindowVisible(gFunctionKeysWindow);
	(Preferences_Result)Preferences_SetData(kPreferences_TagWasFunctionKeypadShowing,
											sizeof(Boolean), &windowIsVisible);
	windowIsVisible = IsWindowVisible(gVT220KeypadWindow);
	(Preferences_Result)Preferences_SetData(kPreferences_TagWasVT220KeypadShowing,
											sizeof(Boolean), &windowIsVisible);
	(Preferences_Result)Preferences_Save(); // write the implicit data to disk
	
	// unregister event listeners
	RemoveEventHandler(gControlKeysButtonHICommandsHandler);
	RemoveEventHandler(gFunctionKeysButtonHICommandsHandler);
	RemoveEventHandler(gVT220KeypadButtonHICommandsHandler);
	DisposeEventHandlerUPP(gButtonHICommandsUPP);
	RemoveEventHandler(gControlKeysWindoidClosingHandler);
	RemoveEventHandler(gFunctionKeysWindoidClosingHandler);
	RemoveEventHandler(gVT220KeypadWindoidClosingHandler);
	DisposeEventHandlerUPP(gWindoidClosingUPP);
	
	// destroy the windoids
	DisposeWindow(gControlKeysWindow);
	DisposeWindow(gFunctionKeysWindow);
	DisposeWindow(gVT220KeypadWindow);
}// Done


/*!
Returns a Mac OS window reference for the specified
keypad’s window.  If any problems occur, nullptr is
returned.

(3.0)
*/
WindowRef
Keypads_ReturnWindow	(Keypads_WindowType		inFromKeypad)
{
	WindowRef	result = nullptr;
	
	
	if (gInited)
	{
		switch (inFromKeypad)
		{
		case kKeypads_WindowTypeControlKeys:
			result = gControlKeysWindow;
			break;
		
		case kKeypads_WindowTypeFunctionKeys:
			result = gFunctionKeysWindow;
			break;
		
		case kKeypads_WindowTypeVT220Keys:
			result = gVT220KeypadWindow;
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// ReturnWindow


#pragma mark Internal Methods

/*!
This routine consistently determines the current
keyboard focus session for all keypad menus.

(3.0)
*/
static SessionRef
getCurrentSession ()
{
	return SessionFactory_ReturnUserFocusSession();
}// getCurrentSession


/*!
Changes the theme font of all top-level controls
in the specified window to use the system font
(which automatically sets the font size).  For
keypads, this has the effect of making all buttons
use a more readable font size.  By default, a NIB
makes bevel buttons use a small font.

(3.0)
*/
static void
makeAllButtonsUseTheSystemFont	(WindowRef	inWindow)
{
	OSStatus	error = noErr;
	ControlRef	root = nullptr;
	
	
	error = GetRootControl(inWindow, &root);
	if (error == noErr)
	{
		UInt16	buttonCount = 0;
		
		
		error = CountSubControls(root, &buttonCount);
		if (error == noErr)
		{
			ControlRef			button = nullptr;
			register SInt16		i = 0;
			
			
			for (i = 1; i <= buttonCount; ++i)
			{
				GetIndexedSubControl(root, i, &button);
				Localization_SetControlThemeFontInfo(button, kThemeAlertHeaderFont/*kThemeSystemFont*/);
			}
		}
	}
}// makeAllButtonsUseTheSystemFont


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in keypads.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inWindowRef)
{
	OSStatus		result = eventNotHandledErr;
	WindowRef		window = REINTERPRET_CAST(inWindowRef, WindowRef);
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(IsValidWindowRef(window));
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			CommandIDToCharacterOrKeyMap::const_iterator	commandToCharacterOrKeyIterator;
			
			
			commandToCharacterOrKeyIterator = gCommandIDToCharacterOrKeyMap().find(received.commandID);
			if (commandToCharacterOrKeyIterator != gCommandIDToCharacterOrKeyMap().end())
			{
				// command ID maps to a key or character code; send to the active session,
				// but first determine whether the command ID corresponds to a control key
				switch (commandToCharacterOrKeyIterator->first)
				{
				case kCommandKeypadControlAtSign:
				case kCommandKeypadControlA:
				case kCommandKeypadControlB:
				case kCommandKeypadControlC:
				case kCommandKeypadControlD:
				case kCommandKeypadControlE:
				case kCommandKeypadControlF:
				case kCommandKeypadControlG:
				case kCommandKeypadControlH:
				case kCommandKeypadControlI:
				case kCommandKeypadControlJ:
				case kCommandKeypadControlK:
				case kCommandKeypadControlL:
				case kCommandKeypadControlM:
				case kCommandKeypadControlN:
				case kCommandKeypadControlO:
				case kCommandKeypadControlP:
				case kCommandKeypadControlQ:
				case kCommandKeypadControlR:
				case kCommandKeypadControlS:
				case kCommandKeypadControlT:
				case kCommandKeypadControlU:
				case kCommandKeypadControlV:
				case kCommandKeypadControlW:
				case kCommandKeypadControlX:
				case kCommandKeypadControlY:
				case kCommandKeypadControlZ:
				case kCommandKeypadControlLeftSquareBracket:
				case kCommandKeypadControlBackslash:
				case kCommandKeypadControlRightSquareBracket:
				case kCommandKeypadControlTilde:
				case kCommandKeypadControlQuestionMark:
					// control keys are actually represented by their ASCII character codes, so just “type” the character
					{
						SessionRef		currentSession = getCurrentSession();
						
						
						if (currentSession != nullptr)
						{
							char	ck[1] = { commandToCharacterOrKeyIterator->second };
							
							
							Session_UserInputString(currentSession, ck, sizeof(ck), false/* record */);
						}
					}
					break;
				
				default:
					// for key codes, send the character appropriate for the current terminal mode
					{
						SessionRef		currentSession = getCurrentSession();
						
						
						if (currentSession != nullptr)
						{
							Session_UserInputKey(currentSession, commandToCharacterOrKeyIterator->second);
						}
					}
					break;
				}
			}
			else
			{
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the keypad is showing
				result = eventNotHandledErr;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for keypads.  The default handler destroys windows,
but keypads should only be hidden or shown (as
they always remain in memory).

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
		if (result == noErr)
		{
			if (window == gControlKeysWindow)
			{
				Commands_ExecuteByID(kCommandHideControlKeys);
				result = noErr; // event is completely handled
			}
			else if (window == gFunctionKeysWindow)
			{
				Commands_ExecuteByID(kCommandHideFunction);
				result = noErr; // event is completely handled
			}
			else if (window == gVT220KeypadWindow)
			{
				Commands_ExecuteByID(kCommandHideKeypad);
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

// BELOW IS REQUIRED NEWLINE TO END FILE
