/*###############################################################

	MacroSetupWindow.cp
	
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
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <Cursors.h>
#include <CommonEventHandlers.h>
#include <DialogAdjust.h>
#include <Embedding.h>
#include <HIViewWrap.h>
#include <IconManager.h>
#include <ListUtilities.h>
#include <Localization.h>
#include <NIBLoader.h>
#include <SoundSystem.h>
#include <WindowInfo.h>

// resource includes
#include "DialogResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "HelpSystem.h"
#include "MacroManager.h"
#include "MacroSetupWindow.h"
#include "Preferences.h"
#include "Terminology.h"
#include "UIStrings.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Window" NIB from the package "MacroSetupWindow.nib".
*/
enum
{
	kSignatureMyUserPaneMacroSetList		= FOUR_CHAR_CODE('MLst'),
	kSignatureMyLabelCurrentMacroSetName	= FOUR_CHAR_CODE('SetN'),
	kSignatureMyButtonImport				= FOUR_CHAR_CODE('IMcr'),
	kSignatureMyButtonExport				= FOUR_CHAR_CODE('EMcr'),
	kSignatureMyAdvancedMacros				= FOUR_CHAR_CODE('AdvM'),
	kSignatureMyButtonHelp					= FOUR_CHAR_CODE('Help'),
	kSignatureMyTextGeneralHelp				= FOUR_CHAR_CODE('HTxt'),
	kSignatureMyLabelAnyMacro				= FOUR_CHAR_CODE('McrK'),
	kSignatureMyFieldAnyMacro				= FOUR_CHAR_CODE('McrT'),
	kSignatureMySeparatorForMacros			= FOUR_CHAR_CODE('MSep'),
	kSignatureMyLabelMacroKeys				= FOUR_CHAR_CODE('MKLb'),
	kSignatureMySeparatorMacroKeys			= FOUR_CHAR_CODE('MKLS'),
	kSignatureMyRadioButtonCommandKeys		= FOUR_CHAR_CODE('MKCK'),
	kSignatureMyRadioButtonFunctionKeys		= FOUR_CHAR_CODE('MKFK')
};
static HIViewID const		idMyUserPaneMacroSetList		= { kSignatureMyUserPaneMacroSetList,		0/* ID */ };
static HIViewID const		idMyLabelCurrentMacroSetName	= { kSignatureMyLabelCurrentMacroSetName,	0/* ID */ };
static HIViewID const		idMyButtonImport				= { kSignatureMyButtonImport,				0/* ID */ };
static HIViewID const		idMyButtonExport				= { kSignatureMyButtonExport,				0/* ID */ };
static HIViewID const		idMyButtonAdvancedMacros		= { kSignatureMyAdvancedMacros,				0/* ID */ };
static HIViewID const		idMyButtonHelp					= { kSignatureMyButtonHelp,					0/* ID */ };
static HIViewID const		idMyTextGeneralHelp				= { kSignatureMyTextGeneralHelp,			0/* ID */ };
static HIViewID const		idMySeparatorForMacros			= { kSignatureMySeparatorForMacros,			0/* ID */ };
static HIViewID const		idMyLabelMacroKeys				= { kSignatureMyLabelMacroKeys,				0/* ID */ };
static HIViewID const		idMySeparatorMacroKeys			= { kSignatureMySeparatorMacroKeys,			0/* ID */ };
static HIViewID const		idMyRadioButtonCommandKeys		= { kSignatureMyRadioButtonCommandKeys,		0/* ID */ };
static HIViewID const		idMyRadioButtonFunctionKeys		= { kSignatureMyRadioButtonFunctionKeys,	0/* ID */ };

// each collection of the following ID values are expected to be numbered consecutively and zero-based;
// see the code that uses GetControlByID() to retrieve them
static HIViewID const		idMyLabelMacro0					= { kSignatureMyLabelAnyMacro,				0/* ID */ };
static HIViewID const		idMyLabelMacro1					= { kSignatureMyLabelAnyMacro,				1/* ID */ };
static HIViewID const		idMyLabelMacro2					= { kSignatureMyLabelAnyMacro,				2/* ID */ };
static HIViewID const		idMyLabelMacro3					= { kSignatureMyLabelAnyMacro,				3/* ID */ };
static HIViewID const		idMyLabelMacro4					= { kSignatureMyLabelAnyMacro,				4/* ID */ };
static HIViewID const		idMyLabelMacro5					= { kSignatureMyLabelAnyMacro,				5/* ID */ };
static HIViewID const		idMyLabelMacro6					= { kSignatureMyLabelAnyMacro,				6/* ID */ };
static HIViewID const		idMyLabelMacro7					= { kSignatureMyLabelAnyMacro,				7/* ID */ };
static HIViewID const		idMyLabelMacro8					= { kSignatureMyLabelAnyMacro,				8/* ID */ };
static HIViewID const		idMyLabelMacro9					= { kSignatureMyLabelAnyMacro,				9/* ID */ };
static HIViewID const		idMyLabelMacro10				= { kSignatureMyLabelAnyMacro,				10/* ID */ };
static HIViewID const		idMyLabelMacro11				= { kSignatureMyLabelAnyMacro,				11/* ID */ };
static HIViewID const		idMyFieldMacro0					= { kSignatureMyFieldAnyMacro,				0/* ID */ };
static HIViewID const		idMyFieldMacro1					= { kSignatureMyFieldAnyMacro,				1/* ID */ };
static HIViewID const		idMyFieldMacro2					= { kSignatureMyFieldAnyMacro,				2/* ID */ };
static HIViewID const		idMyFieldMacro3					= { kSignatureMyFieldAnyMacro,				3/* ID */ };
static HIViewID const		idMyFieldMacro4					= { kSignatureMyFieldAnyMacro,				4/* ID */ };
static HIViewID const		idMyFieldMacro5					= { kSignatureMyFieldAnyMacro,				5/* ID */ };
static HIViewID const		idMyFieldMacro6					= { kSignatureMyFieldAnyMacro,				6/* ID */ };
static HIViewID const		idMyFieldMacro7					= { kSignatureMyFieldAnyMacro,				7/* ID */ };
static HIViewID const		idMyFieldMacro8					= { kSignatureMyFieldAnyMacro,				8/* ID */ };
static HIViewID const		idMyFieldMacro9					= { kSignatureMyFieldAnyMacro,				9/* ID */ };
static HIViewID const		idMyFieldMacro10				= { kSignatureMyFieldAnyMacro,				10/* ID */ };
static HIViewID const		idMyFieldMacro11				= { kSignatureMyFieldAnyMacro,				11/* ID */ };

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WindowRef							gMacroSetupWindow = nullptr;
	WindowRef							gMacroSetupAdvancedHelpWindow = nullptr;
	CommonEventHandlers_WindowResizer	gMacroSetupWindowResizeHandler;
	IconManagerIconRef					gMacroSetIcon = nullptr;
	ControlKeyFilterUPP					gMacroFieldKeyFilterUPP = nullptr;
	ControlRef							gMacroSetupMacroSetListBox = nullptr,
										gMacroSetupMacroSetListUserPane = nullptr,
										gMacroSetupActiveSetNameLabel = nullptr,
										gMacroSetupImportButton = nullptr,
										gMacroSetupExportButton = nullptr,
										gMacroSetupAdvancedMacrosButton = nullptr,
										gMacroSetupGeneralHelpText = nullptr,
										gMacroSetupSeparator = nullptr,
										gMacroSetupMacroKeysLabel = nullptr,
										gMacroSetupMacroKeysSeparator = nullptr,
										gMacroSetupCommandKeysRadioButton = nullptr,
										gMacroSetupFunctionKeysRadioButton = nullptr;
	HIViewWrap							gMacroSetupHelpButton = nullptr;
	ControlRef							gMacroLabels[MACRO_COUNT] =
										{
											nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
										};
	ControlRef							gMacroFields[MACRO_COUNT] =
										{
											nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
										};
	EventHandlerRef						gFieldLoseFocusHandlers[MACRO_COUNT] = //!< invoked when a field’s focus changes
										{
											nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
										};
	EventHandlerRef						gButtonHICommandsHandler = nullptr,	//!< invoked when a window button is clicked
										gListBoxHitHandler = nullptr,		//!< invoked when mouse-up in list box occurs
										gListBoxKeyUpHandler = nullptr,		//!< invoked when key-up in list box occurs
										gWindowClosingHandler = nullptr,	//!< invoked when the macro setup window closes
										gHelpWindowButtonHICommandsHandler = nullptr;
	EventHandlerUPP						gButtonHICommandsUPP = nullptr,		//!< wrapper for button callback function
										gFieldLoseFocusUPP = nullptr,		//!< wrapper for lose-focus callback function
										gListBoxHitUPP = nullptr,			//!< wrapper for list box hit callback function
										gListBoxKeyUpUPP = nullptr,			//!< wrapper for list box key callback function
										gWindowClosingUPP = nullptr;		//!< wrapper for window close callback function
	ListHandle							gMacroSetupMacroSetListHandle = nullptr;
	ListenerModel_ListenerRef			gMacroChangeListener = nullptr,
										gMacroSetupWindowDisplayEventListener = nullptr;
}

//
// external methods
//

// declare the LDEF entry point (it’s only referred to here, and is implemented in IconListDef.c)
pascal void IconListDef(SInt16, Boolean, Rect*, Cell, SInt16, SInt16, ListHandle);

//
// internal methods
//

static void				handleNewSize				(WindowRef				inWindow,
													 Float32				inDeltaSizeX,
													 Float32				inDeltaSizeY,
													 void*					inContext);

static pascal ControlKeyFilterResult	macroFieldKeyFilter		(ControlRef	inControl,
													 SInt16*				inoutKeyCodePtr,
													 SInt16*				inoutCharCodePtr,
													 UInt16*				inoutModifiersPtr);

static void				macrosChanged				(ListenerModel_Ref		inUnusedModel,
													 ListenerModel_Event	inMacrosChange,
													 void*					inEventContextPtr,
													 void*					inListenerContextPtr);

static pascal OSStatus	receiveHICommand			(EventHandlerCallRef	inHandlerCallRef,
													 EventRef				inEvent,
													 void*					inContext);

static pascal OSStatus	receiveFieldFocusChange		(EventHandlerCallRef	inHandlerCallRef,
													 EventRef				inEvent,
													 void*					inContext);

static pascal OSStatus	receiveListBoxHit			(EventHandlerCallRef	inHandlerCallRef,
													 EventRef				inEvent,
													 void*					inContext);

static pascal OSStatus	receiveListBoxKeyUp			(EventHandlerCallRef	inHandlerCallRef,
													 EventRef				inEvent,
													 void*					inContext);

static pascal OSStatus	receiveWindowClosing		(EventHandlerCallRef	inHandlerCallRef,
													 EventRef				inEvent,
													 void*					inContext);

static Boolean			saveField					(ControlRef				inField);

static Boolean			saveFields					();

static Boolean			setButtonsByMode			(MacroManager_InvocationMethod		inMode);

static void				showAdvancedMacrosHelp		();

static OSStatus			showHideMacroSetupWindow	(ListenerModel_Ref					inUnusedModel,
													 ListenerModel_Event				inCommandID,
													 void*								inEventContextPtr,
													 void*								inListenerContextPtr);

static void				updateCurrentSetName		();

static void				updateTextField				(UInt8*								inoutMacroText,
													 SInt16								inZeroBasedMacroNumber);

static void				updateTextFields			();



//
// public methods
//

/*!
This method is used to initialize the Macro Setup
dialog box.  It creates the dialog box invisibly,
and uses the global macros data to set up the
corresponding controls in the dialog box.

(3.0)
*/
void
MacroSetupWindow_Init ()
{
	Cursors_DeferredUseWatch(30); // if it takes more than half a second to initialize, show the watch cursor
	
	// load the NIB containing this dialog (automatically finds the right localization)
	gMacroSetupWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("MacroSetupWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	
	// find references to all controls that are needed for any operation
	// (button clicks, dealing with text or responding to window resizing)
	{
		OSStatus	error = noErr;
		
		
		error = GetControlByID(gMacroSetupWindow, &idMyUserPaneMacroSetList, &gMacroSetupMacroSetListUserPane);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyLabelCurrentMacroSetName, &gMacroSetupActiveSetNameLabel);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyButtonImport, &gMacroSetupImportButton);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyButtonExport, &gMacroSetupExportButton);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyButtonAdvancedMacros, &gMacroSetupAdvancedMacrosButton);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyTextGeneralHelp, &gMacroSetupGeneralHelpText);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMySeparatorForMacros, &gMacroSetupSeparator);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyLabelMacroKeys, &gMacroSetupMacroKeysLabel);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMySeparatorMacroKeys, &gMacroSetupMacroKeysSeparator);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyRadioButtonCommandKeys, &gMacroSetupCommandKeysRadioButton);
		assert(error == noErr);
		error = GetControlByID(gMacroSetupWindow, &idMyRadioButtonFunctionKeys, &gMacroSetupFunctionKeysRadioButton);
		assert(error == noErr);
		gMacroSetupHelpButton.setCFTypeRef(HIViewWrap(idMyButtonHelp, gMacroSetupWindow));
		assert(gMacroSetupHelpButton.exists());
		{
			HIViewID			controlID;
			register SInt16		i = 0;
			
			
			controlID.signature = kSignatureMyLabelAnyMacro;
			for (i = 0; i < MACRO_COUNT; ++i)
			{
				controlID.id = i;
				error = GetControlByID(gMacroSetupWindow, &controlID, &gMacroLabels[i]);
				assert(error == noErr);
			}
			controlID.signature = kSignatureMyFieldAnyMacro;
			for (i = 0; i < MACRO_COUNT; ++i)
			{
				controlID.id = i;
				error = GetControlByID(gMacroSetupWindow, &controlID, &gMacroFields[i]);
				assert(error == noErr);
			}
		}
	}
	
	// create an icon for use in the list
	gMacroSetIcon = IconManager_NewIcon();
	(OSStatus)IconManager_MakeIconRefFromBundleFile(gMacroSetIcon, AppResources_ReturnMacroSetIconFilenameNoExtension(),
													kConstantsRegistry_ApplicationCreatorSignature,
													kConstantsRegistry_IconServicesIconFileTypeMacroSet);
	
	// now manually construct the list control, since NIBs do not support list boxes
	{
		Rect				bounds;
		OSStatus			error = noErr;
		ListDefSpec			listSpec;
		SInt32				scrollBarWidth = 0;
		register SInt16		i = 0;
		
		
		// figure out where the box should go
		GetControlBounds(gMacroSetupMacroSetListUserPane, &bounds);
		if (GetThemeMetric(kThemeMetricScrollBarWidth, &scrollBarWidth) != noErr) scrollBarWidth = 16;
		
		// create the list box
		listSpec.defType = kListDefUserProcType;
		listSpec.u.userProc = NewListDefUPP(IconListDef); // disposed automatically by the Mac OS
		error = CreateListBoxControl(gMacroSetupWindow, &bounds, false/* auto-size */, MACRO_SET_COUNT/* number of rows */,
										1/* number of columns */, false/* horizontal scroll bar */,
										true/* vertical scroll bar */, 56/* cell height in pixels - arbitrary, but this
																			should match (5 * NIB’s list box height + 2) */,
										bounds.right - bounds.left - scrollBarWidth/* cell width in pixels */,
										false/* has space for window grow box */, &listSpec, &gMacroSetupMacroSetListBox);
		assert(error == noErr);
		
		// since a list box is just a wrapper around a list, the two
		// can disagree at times; calling this function whenever the
		// list box size changes is a good idea
		ListUtilities_SynchronizeListBoundsWithControlBounds(gMacroSetupMacroSetListBox);
		
		// set some initial list characteristics
		ListUtilities_GetControlListHandle(gMacroSetupMacroSetListBox, &gMacroSetupMacroSetListHandle);
		SetListFlags(gMacroSetupMacroSetListHandle, lDoVAutoscroll);
		SetListSelectionFlags(gMacroSetupMacroSetListHandle, lOnlyOne | lNoNilHilite);
		
		// fill in the item names and icons
		for (i = 0; i < MACRO_SET_COUNT; ++i)
		{
			StandardIconListCellDataRec			listItemData;
			UIStrings_ResultCode				stringError = kUIStrings_ResultCodeSuccess;
			UIStrings_MacroSetupWindowCFString	stringType = kUIStrings_MacroSetupWindowSetName1;
			CFStringRef							nameString = nullptr;
			Cell								listCell;
			
			
			listCell.h = 0;
			listCell.v = i;
			listItemData.iconHandle = IconManager_GetData(gMacroSetIcon);
			{
				// use the Appearance Manager to find the list/views font
				Str255		fontName;
				SInt16		fontSize = 0;
				Style		style = normal;
				
				
				(OSStatus)GetThemeFont(kThemeViewsFont, smSystemScript, fontName, &fontSize, &style);
				GetFNum(fontName, &listItemData.font);
				listItemData.face = style;
				listItemData.size = fontSize;
			}
			
			switch (i)
			{
			case 0:
				stringType = kUIStrings_MacroSetupWindowSetName1;
				break;
			
			case 1:
				stringType = kUIStrings_MacroSetupWindowSetName2;
				break;
			
			case 2:
				stringType = kUIStrings_MacroSetupWindowSetName3;
				break;
			
			case 3:
				stringType = kUIStrings_MacroSetupWindowSetName4;
				break;
			
			case 4:
				stringType = kUIStrings_MacroSetupWindowSetName5;
				break;
			
			default:
				// ???
				break;
			}
			stringError = UIStrings_Copy(stringType, nameString);
			if (stringError == kUIStrings_ResultCodeSuccess)
			{
				if (CFStringGetPascalString(nameString, listItemData.name, sizeof(listItemData.name),
											CFStringGetSystemEncoding()))
				{
					LSetCell(&listItemData, sizeof(listItemData), listCell, gMacroSetupMacroSetListHandle);
					if (listCell.v == 0) LSetSelect(true/* highlight */, listCell, gMacroSetupMacroSetListHandle);
				}
				CFRelease(nameString), nameString = nullptr;
			}
		}
	}
	
	// this is just a dummy control used to create the list box in the right place; so throw it away now
	DisposeControl(gMacroSetupMacroSetListUserPane), gMacroSetupMacroSetListUserPane = nullptr;
	
	// set up the Help System
	HelpSystem_SetWindowKeyPhrase(gMacroSetupWindow, kHelpSystem_KeyPhraseMacros);
	
	// make the help button icon appearance and state appropriate
	DialogUtilities_SetUpHelpButton(gMacroSetupHelpButton);
	
	// install a key filter to prevent user errors
	// NOTE; this could probably become a Carbon Event handler instead
	{
		ControlRef			field = nullptr;
		HIViewID			controlID;
		OSStatus			error = noErr;
		register SInt16		i = 0;
		
		
		controlID.signature = kSignatureMyFieldAnyMacro;
		gMacroFieldKeyFilterUPP = NewControlKeyFilterUPP(macroFieldKeyFilter);
		for (i = 0; i < MACRO_COUNT; ++i)
		{
			controlID.id = i;
			error = GetControlByID(gMacroSetupWindow, &controlID, &field);
			if (error == noErr)
			{
				error = SetControlData(field, kControlEditTextPart, kControlEditTextKeyFilterTag,
										sizeof(gMacroFieldKeyFilterUPP), &gMacroFieldKeyFilterUPP);
			}
		}
	}
	
	// initialize other controls
	setButtonsByMode(Macros_GetMode());
	updateCurrentSetName();
	updateTextFields();
	
	// install a callback that responds to buttons in the window
	{
		EventTypeSpec const		whenButtonClicked[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		OSStatus				error = noErr;
		
		
		gButtonHICommandsUPP = NewEventHandlerUPP(receiveHICommand);
		error = InstallWindowEventHandler(gMacroSetupWindow, gButtonHICommandsUPP, GetEventTypeCount(whenButtonClicked),
											whenButtonClicked, nullptr/* user data */,
											&gButtonHICommandsHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that responds to focus changes in the text fields
	{
		EventTypeSpec const		whenControlFocusChanges[] =
								{
									{ kEventClassControl, kEventControlSetFocusPart }
								};
		OSStatus				error = noErr;
		register SInt16			i = 0;
		
		
		gFieldLoseFocusUPP = NewEventHandlerUPP(receiveFieldFocusChange);
		for (i = 0; i < MACRO_COUNT; ++i)
		{
			error = HIViewInstallEventHandler(gMacroFields[i], gFieldLoseFocusUPP, GetEventTypeCount(whenControlFocusChanges),
												whenControlFocusChanges, nullptr/* user data */,
												&gFieldLoseFocusHandlers[i]/* event handler reference */);
			assert(error == noErr);
		}
	}
	
	// install a callback that responds to clicks in the macro set list box
	{
		EventTypeSpec const		whenMacroSetListIsHit[] =
								{
									{ kEventClassControl, kEventControlHit }
								};
		OSStatus				error = noErr;
		
		
		gListBoxHitUPP = NewEventHandlerUPP(receiveListBoxHit);
		error = HIViewInstallEventHandler(gMacroSetupMacroSetListBox, gListBoxHitUPP, GetEventTypeCount(whenMacroSetListIsHit),
											whenMacroSetListIsHit, nullptr/* user data */,
											&gListBoxHitHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that responds to key-up events in the macro set list box
	{
		EventTypeSpec const		whenMacroSetListKeyReleased[] =
								{
									{ kEventClassKeyboard, kEventRawKeyUp }
								};
		OSStatus				error = noErr;
		
		
		gListBoxKeyUpUPP = NewEventHandlerUPP(receiveListBoxKeyUp);
		error = HIViewInstallEventHandler(gMacroSetupMacroSetListBox, gListBoxKeyUpUPP,
											GetEventTypeCount(whenMacroSetListKeyReleased),
											whenMacroSetListKeyReleased, nullptr/* user data */,
											&gListBoxKeyUpHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// install a callback that hides the window instead of destroying it, when it should be closed
	{
		EventTypeSpec const		whenWindowClosing[] =
								{
									{ kEventClassWindow, kEventWindowClose }
								};
		OSStatus				error = noErr;
		
		
		gWindowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
		error = InstallWindowEventHandler(gMacroSetupWindow, gWindowClosingUPP, GetEventTypeCount(whenWindowClosing),
											whenWindowClosing, nullptr/* user data */,
											&gWindowClosingHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	{
		Rect		currentBounds;
		Point		deltaSize;
		OSStatus	error = noErr;
		
		
		// install a callback that responds as a window is resized
		error = GetWindowBounds(gMacroSetupWindow, kWindowContentRgn, &currentBounds);
		assert(noErr == error);
		gMacroSetupWindowResizeHandler.install(gMacroSetupWindow, handleNewSize, nullptr/* user data */,
												currentBounds.right - currentBounds.left/* minimum width */,
												currentBounds.bottom - currentBounds.top/* minimum height */,
												1024/* arbitrary maximum width */,
												currentBounds.bottom - currentBounds.top/* maximum height */);
		assert(gMacroSetupWindowResizeHandler.isInstalled());
		
		// use the preferred rectangle, if any; since a resize handler was
		// installed above, simply resizing the window will cause all
		// controls to be adjusted automatically by the right amount
		SetPt(&deltaSize, currentBounds.right - currentBounds.left,
				currentBounds.bottom - currentBounds.top); // initially...
		(Preferences_ResultCode)Preferences_ArrangeWindow(gMacroSetupWindow, kPreferences_WindowTagMacroSetup, &deltaSize,
															kPreferences_WindowBoundaryLocation |
															kPreferences_WindowBoundaryElementWidth);
	}
	
	// now that the macro window is ready, start listening for changes
	// to macros; every time something happens, this window should update
	gMacroChangeListener = ListenerModel_NewStandardListener(macrosChanged);
	assert(gMacroChangeListener != nullptr);
	Macros_StartMonitoring(kMacros_ChangeActiveSetPlanned, gMacroChangeListener);
	Macros_StartMonitoring(kMacros_ChangeActiveSet, gMacroChangeListener);
	Macros_StartMonitoring(kMacros_ChangeContents, gMacroChangeListener);
	Macros_StartMonitoring(kMacros_ChangeMode, gMacroChangeListener);
	
	// finally, ask to be told when a “show/hide macro setup window” command occurs
	gMacroSetupWindowDisplayEventListener = ListenerModel_NewOSStatusListener(showHideMacroSetupWindow);
	Commands_StartHandlingExecution(kCommandShowMacros, gMacroSetupWindowDisplayEventListener);
	Commands_StartHandlingExecution(kCommandHideMacros, gMacroSetupWindowDisplayEventListener);
}// Init


/*!
Call this method in the exit routine of the program
to ensure that this window’s memory allocations
are destroyed.  You can also call this method after
you are done using this window, provided you call
MacroSetupWindow_Init() before you use this window
again.

(3.0)
*/
void
MacroSetupWindow_Done ()
{
	// stop listening for changes for macros, because the window is going away
	Macros_StopMonitoring(kMacros_ChangeActiveSetPlanned, gMacroChangeListener);
	Macros_StopMonitoring(kMacros_ChangeActiveSet, gMacroChangeListener);
	Macros_StopMonitoring(kMacros_ChangeContents, gMacroChangeListener);
	Macros_StopMonitoring(kMacros_ChangeMode, gMacroChangeListener);
	ListenerModel_ReleaseListener(&gMacroChangeListener);
	
	Commands_StopHandlingExecution(kCommandShowMacros, gMacroSetupWindowDisplayEventListener);
	Commands_StopHandlingExecution(kCommandHideMacros, gMacroSetupWindowDisplayEventListener);
	ListenerModel_ReleaseListener(&gMacroSetupWindowDisplayEventListener);
	
	MacroSetupWindow_Remove(); // also saves window size and location
	
	// clean up the Help System
	HelpSystem_SetWindowKeyPhrase(gMacroSetupWindow, kHelpSystem_KeyPhraseDefault);
	
	// dispose window-related stuff
	RemoveEventHandler(gButtonHICommandsHandler);
	DisposeEventHandlerUPP(gButtonHICommandsUPP);
	{
		register SInt16		i = 0;
		
		
		for (i = 0; i < MACRO_COUNT; ++i)
		{
			RemoveEventHandler(gFieldLoseFocusHandlers[i]);
		}
		DisposeEventHandlerUPP(gFieldLoseFocusUPP);
	}
	RemoveEventHandler(gListBoxHitHandler);
	DisposeEventHandlerUPP(gListBoxHitUPP);
	RemoveEventHandler(gListBoxKeyUpHandler);
	DisposeEventHandlerUPP(gListBoxKeyUpUPP);
	RemoveEventHandler(gWindowClosingHandler);
	DisposeEventHandlerUPP(gWindowClosingUPP);
	DisposeWindow(gMacroSetupWindow), gMacroSetupWindow = nullptr;
	DisposeControlKeyFilterUPP(gMacroFieldKeyFilterUPP), gMacroFieldKeyFilterUPP = nullptr;
	
	// dispose of icon
	IconManager_DisposeIcon(&gMacroSetIcon);
}// Done


/*!
This method displays and handles events in the
macro setup window.  Since the window is floating,
this routine returns immediately.

If the window does not yet exist, it is first
initialized with MacroSetupWindow_Init().

Returns "true" if the window was displayed
successfully.

(3.0)
*/
Boolean
MacroSetupWindow_Display ()
{
	Boolean		result = false;
	
	
	if (gMacroSetupWindow == nullptr) MacroSetupWindow_Init();
	
	if (gMacroSetupWindow == nullptr) Alert_ReportOSStatus(memFullErr);
	else
	{
		ShowWindow(gMacroSetupWindow);
		EventLoop_SelectBehindDialogWindows(gMacroSetupWindow);
		updateTextFields();
		(OSStatus)SetKeyboardFocus(gMacroSetupWindow, gMacroFields[0], kControlEditTextPart);
		(OSStatus)SetUserFocusWindow(gMacroSetupWindow);
		result = true;
	}
	
	return result;
}// Display


/*!
Returns the Mac OS window reference for the
Macro Setup window, or nullptr if it has not
been created.

Use this routine only for unusual needs; if
an appropriate API exists to do what you want,
use that instead of trying to do it yourself
with a window reference.

(3.0)
*/
WindowRef
MacroSetupWindow_GetWindow ()
{
	return gMacroSetupWindow;
}// GetWindow


/*!
Handles a drag-and-drop of text by parsing the
given buffer for macros, and updating the dialog
box’s text fields appropriately if valid data is
given.

The given data is text, and is for convenience
only; the drag reference could be used to find
the text data with a bit of effort (or any other
data for that matter).  The drag reference is
given so that future versions of this function
can use other Drag Manager information such as
modifier keys, etc.

(3.0)
*/
void
MacroSetupWindow_ReceiveDrop	(DragReference		UNUSED_ARGUMENT(inDragRef),
								 UInt8*				inData,
								 Size				inDataSize)
{
	if (inData != nullptr)
	{
		MacroManager_InvocationMethod		mode = kMacroManager_InvocationMethodCommandDigit;
		
		
		Macros_ParseTextBuffer(Macros_GetActiveSet(), inData, inDataSize, &mode);
		Macros_SetMode(mode);
	}
}// ReceiveDrop


/*!
Use this method to hide the Macro Setup floating
window.

The dialog remains in memory and can be re-displayed
using MacroSetupWindow_Display().  To destroy the
window, use the method MacroSetupWindow_Done().

(3.0)
*/
void
MacroSetupWindow_Remove ()
{
	// save window size and location in preferences
	if (gMacroSetupWindow != nullptr)
	{
		saveFields();
		Preferences_SetWindowArrangementData(gMacroSetupWindow, kPreferences_WindowTagMacroSetup);
		SetUserFocusWindow(REINTERPRET_CAST(kUserFocusAuto, WindowRef));
		HideWindow(gMacroSetupWindow);
	}
}// Remove


//
// internal methods
//
#pragma mark -

/*!
This method moves and resizes controls in response to
a resize of the “Macro Setup” floating window.

(3.0)
*/
static void
handleNewSize	(WindowRef	inWindow,
				 Float32	inDeltaX,
				 Float32	inDeltaY,
				 void*		UNUSED_ARGUMENT(inContext))
{
	// only horizontal changes are significant to this dialog
	if (inDeltaX)
	{
		SInt32		truncDeltaX = STATIC_CAST(inDeltaX, SInt32);
		SInt32		truncDeltaY = STATIC_CAST(inDeltaY, SInt32);
		
		
		DialogAdjust_BeginControlAdjustment(inWindow);
		
		if (Localization_IsLeftToRight())
		{
			// controls which are moved horizontally
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupExportButton, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupImportButton, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupMacroKeysLabel, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupMacroKeysSeparator, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupCommandKeysRadioButton, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupFunctionKeysRadioButton, truncDeltaX);
			{
				SInt32				halfDeltaX = STATIC_CAST(FLOAT64_HALVED(inDeltaX), SInt32);
				register SInt16		i = 0;
				
				
				// the right half of the macro fields must move to make room for the
				// left half of the fields, which are resizing; but since the right
				// half is resizing too, only half of the total delta-X is used
				for (i = INTEGER_HALVED(MACRO_COUNT); i < MACRO_COUNT; ++i)
				{
					DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroFields[i], halfDeltaX);
					DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroLabels[i], halfDeltaX);
				}
			}
		}
		else
		{
			// controls which are moved horizontally
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupHelpButton, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupAdvancedMacrosButton, truncDeltaX);
			DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroSetupMacroSetListUserPane, truncDeltaX);
			{
				SInt32				halfDeltaX = STATIC_CAST(FLOAT64_HALVED(inDeltaX), SInt32);
				register UInt16		i = 0;
				
				
				// in right-to-left locales, the right half is actually the FIRST 6
				// macros, instead of the last 6; also, labels appear to the right of
				// the fields instead of to the left, so labels should be moved first;
				// the right half of the macro fields must move to make room for the
				// left half of the fields, which are resizing; but since the right
				// half is resizing too, only half of the total delta-X is used
				for (i = 0; i < INTEGER_HALVED(MACRO_COUNT); ++i)
				{
					DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroLabels[i], halfDeltaX);
					DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroFields[i], halfDeltaX);
				}
				
				// since labels appear to the right of fields, the labels of the other
				// column of fields must also move to make room for the resizing field
				for (i = INTEGER_HALVED(MACRO_COUNT); i < MACRO_COUNT; ++i)
				{
					DialogAdjust_AddControl(kDialogItemAdjustmentMoveH, gMacroLabels[i], halfDeltaX);
				}
			}
		}
		
		// controls which are resized horizontally independent of the locale
		DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gMacroSetupGeneralHelpText, truncDeltaX);
		DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gMacroSetupSeparator, truncDeltaX);
		{
			SInt32				halfDeltaX = STATIC_CAST(FLOAT64_HALVED(inDeltaX), SInt32);
			register SInt16		i = 0;
			
			
			// the macro fields are arranged in two columns, so each set must
			// resize by half of the total delta-X
			for (i = 0; i < MACRO_COUNT; ++i)
			{
				DialogAdjust_AddControl(kDialogItemAdjustmentResizeH, gMacroFields[i], halfDeltaX);
			}
		}
		
		DialogAdjust_EndAdjustment(truncDeltaX, truncDeltaY);
	}
}// handleNewSize


/*!
Filters out key presses originally sent to a macro text field.
Certain characters are not allowed in macros without escaping,
so this routine checks for such errors.

(3.0)
*/
static pascal ControlKeyFilterResult
macroFieldKeyFilter		(ControlRef		inControl,
						 SInt16*		UNUSED_ARGUMENT(inoutKeyCodePtr),
						 SInt16*		inoutCharCodePtr,
						 UInt16*		UNUSED_ARGUMENT(inoutModifiersPtr))
{
	ControlKeyFilterResult		result = kControlKeyFilterPassKey;
	Str255						macroText;
	
	
	// determine the text currently in the command line
	GetControlText(inControl, macroText);
	
	if (inoutCharCodePtr != nullptr)
	{
		switch (*inoutCharCodePtr)
		{
		case '"':
			// the quotation mark must be escaped
			if (macroText[PLstrlen(macroText)] != '\\')
			{
				Sound_StandardAlert();
				result = kControlKeyFilterBlockKey;
			}
			break;
		
		default:
			// uninteresting key was hit
			break;
		}
	}
	
	return result;
}// macroFieldKeyFilter


/*!
Invoked whenever the active macro set changes, or
the current macro keys change, or the contents of
any given macro changes.

(3.0)
*/
static void
macrosChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
				 ListenerModel_Event	inMacrosChange,
				 void*					inEventContextPtr,
				 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	if (gMacroSetupWindow != nullptr)
	{
		switch (inMacrosChange)
		{
		case kMacros_ChangeActiveSetPlanned:
			// before the macro set changes, save all changes
			saveFields();
			break;
		
		case kMacros_ChangeActiveSet:
			// ensure the correct list item is highlighted, that all
			// macro text fields contain the active set’s macros,
			// and that the active set label is correct
			{
				SInt16 const	kActiveSetIndex = Macros_GetActiveSetNumber() - 1;
				Cell			selectedCell = { 0, 0 };
				
				
				LSetSelect((selectedCell.v == kActiveSetIndex)/* highlight */, selectedCell, gMacroSetupMacroSetListHandle);
				++(selectedCell.v);
				LSetSelect((selectedCell.v == kActiveSetIndex)/* highlight */, selectedCell, gMacroSetupMacroSetListHandle);
				++(selectedCell.v);
				LSetSelect((selectedCell.v == kActiveSetIndex)/* highlight */, selectedCell, gMacroSetupMacroSetListHandle);
				++(selectedCell.v);
				LSetSelect((selectedCell.v == kActiveSetIndex)/* highlight */, selectedCell, gMacroSetupMacroSetListHandle);
				++(selectedCell.v);
				LSetSelect((selectedCell.v == kActiveSetIndex)/* highlight */, selectedCell, gMacroSetupMacroSetListHandle);
				DrawOneControl(gMacroSetupMacroSetListBox);
				updateCurrentSetName();
				updateTextFields();
			}
			break;
		
		case kMacros_ChangeContents:
			// ensure the modified macro’s text field is updated
			{
				MacroDescriptorPtr		descriptorPtr = REINTERPRET_CAST(inEventContextPtr, MacroDescriptorPtr);
				
				
				// only the active set is displayed in the window, so nothing else matters
				if (descriptorPtr->set == Macros_GetActiveSet())
				{
					UInt8	newMacro[256];
					
					
					Macros_Get(descriptorPtr->set, descriptorPtr->index, (char*)&newMacro, sizeof(newMacro));
					updateTextField(newMacro, descriptorPtr->index);
				}
			}
			break;
		
		case kMacros_ChangeMode:
			// determine the new macro keys, and update the radio buttons
			{
				MacroManager_InvocationMethod const*	modePtr = REINTERPRET_CAST(inEventContextPtr, MacroManager_InvocationMethod const*);
				
				
				setButtonsByMode(*modePtr);
			}
			break;
		
		default:
			// ???
			break;
		}
	}
}// macrosChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in the macro setup window.

(3.0)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (result == noErr)
		{
			switch (received.commandID)
			{
			case kHICommandCancel:
				// this command is only registered for the Advanced Macro help dialog; so just close the window
				RemoveEventHandler(gHelpWindowButtonHICommandsHandler);
				HideSheetWindow(gMacroSetupAdvancedHelpWindow);
				DisposeWindow(gMacroSetupAdvancedHelpWindow), gMacroSetupAdvancedHelpWindow = nullptr;
				break;
			
			case kCommandImportMacroSet:
			case kCommandExportCurrentMacroSet:
				// the menu bar already has these commands; just send
				// the command to the application for processing
				result = eventNotHandledErr;
				break;
			
			case kCommandContextSensitiveHelp:
				// open the Help Viewer to the right topic
				HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseMacros);
				break;
			
			case kCommandShowAdvancedMacroInformation:
				showAdvancedMacrosHelp();
				break;
			
			default:
				// must return "eventNotHandledErr" here, or (for example) the user
				// wouldn’t be able to select menu commands while the window is open
				result = eventNotHandledErr;
				break;
			}
		}
	}
	
	return result;
}// receiveHICommand


/*!
Handles "kEventControlSetFocusPart" of "kEventClassControl"
for all macro text fields the Macro Setup window.  Responds
by saving macro text to memory.

(3.0)
*/
static pascal OSStatus
receiveFieldFocusChange		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlSetFocusPart);
	{
		ControlRef		control = nullptr;
		
		
		// determine the control in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, control);
		
		// if the control was found, proceed
		if (result == noErr)
		{
			saveField(control);
			
			// DO NOT say this event is handled, so the default handler
			// can continue to set the focus ring properly, etc.
			result = eventNotHandledErr;
		}
	}
	
	return result;
}// receiveFieldFocusChange


/*!
Handles "kEventControlClick" of "kEventClassControl"
for the list box of the Macro Setup window.  Responds
by changing the active macro set, which in turn would
trigger updates of the window contents, etc.

(3.0)
*/
static pascal OSStatus
receiveListBoxHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
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
		
		// if the control was found, proceed
		if (result == noErr)
		{
			// no other controls are expected...
			assert(control == gMacroSetupMacroSetListBox);
			
			// determine the selected item and react appropriately
			{
				Cell	selectedCell = { 0, 0 };
				
				
				unless (LGetSelect(true/* next */, &selectedCell, gMacroSetupMacroSetListHandle))
				{
					// if there is somehow no selection (not normally possible), choose the first cell
					selectedCell.h = 0;
					selectedCell.v = 0;
					LSetSelect(true/* highlight */, selectedCell, gMacroSetupMacroSetListHandle);
				}
				
				// set the new active macro set using its one-based index
				Macros_SetActiveSetNumber(1 + selectedCell.v);
				
				// return event-not-handled so that the list box works normally;
				// all this handler tried to do was ensure other controls were
				// updated properly
				result = eventNotHandledErr;
			}
		}
	}
	
	return result;
}// receiveListBoxHit


/*!
Handles "kEventRawKeyUp" of "kEventClassKeyboard" for
the list box of the Macro Setup window.  Responds by
changing the active macro set, which in turn would
trigger updates of the window contents, etc.

(3.0)
*/
static pascal OSStatus
receiveListBoxKeyUp		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassKeyboard);
	assert(kEventKind == kEventRawKeyUp);
	{
		// determine the selected item and react appropriately; currently,
		// this handler just assumes that users will probably only type
		// arrow keys while the list box is highlighted; if other keys are
		// used, technically this handler will be run pointlessly (but for
		// simplicity’s sake, this is the way it is)
		Cell	selectedCell = { 0, 0 };
		
		
		unless (LGetSelect(true/* next */, &selectedCell, gMacroSetupMacroSetListHandle))
		{
			// if there is somehow no selection (not normally possible), choose the first cell
			selectedCell.h = 0;
			selectedCell.v = 0;
			LSetSelect(true/* highlight */, selectedCell, gMacroSetupMacroSetListHandle);
		}
		
		// set the new active macro set using its one-based index
		Macros_SetActiveSetNumber(1 + selectedCell.v);
		
		// return event-not-handled so that the list box works normally;
		// all this handler tried to do was ensure other controls were
		// updated properly
		result = eventNotHandledErr;
	}
	
	return result;
}// receiveListBoxKeyUp


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for the Macro Setup window.  The default handler
destroys windows, but the Macro Setup window should
only be hidden or shown (as it always remains in
memory).

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
			if (window == gMacroSetupWindow)
			{
				Commands_ExecuteByID(kCommandHideMacros);
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
Saves the contents of the specified text field
into the proper macro of the active macro set.

Returns "true" only if the field was saved
successfully.

(3.0)
*/
static Boolean
saveField	(ControlRef		inField)
{
	Boolean		result = false;
	
	
	if (gMacroSetupWindow != nullptr)
	{
		OSStatus	error = noErr;
		HIViewID	controlID;
		
		
		error = GetControlID(inField, &controlID);
		if (error == noErr)
		{
			Str255		text;
			
			
			assert(controlID.signature == kSignatureMyFieldAnyMacro);
			assert(controlID.id >= 0);
			assert(controlID.id < MACRO_COUNT);
			
			GetControlText(inField, text);
			StringUtilities_PToCInPlace(text);
			Macros_Set(Macros_GetActiveSet(), controlID.id/* macro index */, REINTERPRET_CAST(text, char const*));
			result = true;
		}
	}
	return result;
}// saveField


/*!
Saves the contents of every text field into the
macros of the active macro set.

Returns "true" only if all fields were saved
successfully.

(3.0)
*/
static Boolean
saveFields ()
{
	Boolean		result = false;
	
	
	if (gMacroSetupWindow != nullptr)
	{
		MacroIndex	i = 0;
		Boolean		success = false;
		
		
		result = true;
		for (i = 0; i < MACRO_COUNT; ++i)
		{
			success = saveField(gMacroFields[i]);
			unless (success) result = false;
		}
	}
	return result;
}// saveFields


/*!
To label the ten macro buttons based on the current
macro mode (function key or command-digit, for
example), call this method.  This method is in
standard OffscreenOperationProcPtr form.

(3.0)
*/
static Boolean
setButtonsByMode	(MacroManager_InvocationMethod	inMode)
{
	Str255				titleText;
	ControlRef			control = nullptr;
	HIViewID			controlID;
	OSStatus			error = noErr;
	register UInt16		i = 0;
	Boolean				result = pascal_true;
	
	
	// unmark all radio buttons first...
	SetControl32BitValue(gMacroSetupCommandKeysRadioButton, kControlRadioButtonUncheckedValue);
	SetControl32BitValue(gMacroSetupFunctionKeysRadioButton, kControlRadioButtonUncheckedValue);
	
	// now mark the radio button for the desired mode
	if (inMode == kMacroManager_InvocationMethodCommandDigit)
	{
		SetControl32BitValue(gMacroSetupCommandKeysRadioButton, kControlRadioButtonCheckedValue);
	}
	if (inMode == kMacroManager_InvocationMethodFunctionKeys)
	{
		SetControl32BitValue(gMacroSetupFunctionKeysRadioButton, kControlRadioButtonCheckedValue);
	}
	
	// change the field labels to match the mode
	controlID.signature = kSignatureMyLabelAnyMacro;
	for (i = 0; i <= MACRO_COUNT; ++i)
	{
		controlID.id = i;
		PLstrcpy(titleText, EMPTY_PSTRING);
		if (inMode == kMacroManager_InvocationMethodCommandDigit)
		{
			PLstrcpy(titleText, "\p");
			if (i < 10) StringUtilities_PAppendNumber(titleText, (long)i);
			else if (i == 10) PLstrcat(titleText, "\p="); // special macro
			else if (i == 11) PLstrcat(titleText, "\p/"); // special macro
		}
		else if (inMode == kMacroManager_InvocationMethodFunctionKeys)
		{
			PLstrcpy(titleText, "\pF");
			StringUtilities_PAppendNumber(titleText, (long)(i + 1));
		}
		
		error = GetControlByID(gMacroSetupWindow, &controlID, &control);
		if (error == noErr)
		{
			(UInt16)Localization_SetUpSingleLineTextControl(control, titleText, false/* is a checkbox or radio button */);
			DrawOneControl(control);
		}
	}
	return result;
}// setButtonsByMode


/*!
Displays a help window giving the user detailed information
on how to embed special characters or substitute properties
(like the current IP address) in a macro.

Returns immediately, but the user may close the window
whenever he or she wishes.

(3.0)
*/
static void
showAdvancedMacrosHelp ()
{
	// load the NIB containing this dialog (automatically finds the right localization)
	gMacroSetupAdvancedHelpWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
												CFSTR("MacroSetupWindow"), CFSTR("SpecialSequencesDialog"))
									<< NIBLoader_AssertWindowExists;
	
	// install a callback that responds to buttons in the window
	{
		EventTypeSpec const		whenButtonClicked[] =
								{
									{ kEventClassCommand, kEventCommandProcess }
								};
		OSStatus				error = noErr;
		
		
		error = InstallWindowEventHandler(gMacroSetupAdvancedHelpWindow, gButtonHICommandsUPP,
											GetEventTypeCount(whenButtonClicked), whenButtonClicked, nullptr/* user data */,
											&gHelpWindowButtonHICommandsHandler/* event handler reference */);
		assert(error == noErr);
	}
	
	// display the help; Carbon Events will handle the rest, as the default handler
	// will dispose of the window when the action button is clicked
	{
		OSStatus	error = noErr;
		
		
		error = ShowSheetWindow(gMacroSetupAdvancedHelpWindow, gMacroSetupWindow);
		if (error != noErr) Sound_StandardAlert();
	}
}// showAdvancedMacrosHelp


/*!
Invoked when the visible state of the Macro Setup
Window should be toggled or shown explicitly.

The result is "eventNotHandledErr" if the command was
not actually executed - this frees other possible
handlers to take a look.  Any other return value
including "noErr" terminates the command sequence.

(3.0)
*/
static OSStatus
showHideMacroSetupWindow	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inCommandID,
							 void*					inEventContextPtr,
							 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	Commands_ExecutionEventContextPtr	commandInfoPtr = REINTERPRET_CAST(inEventContextPtr, Commands_ExecutionEventContextPtr);
	OSStatus							result = eventNotHandledErr;
	
	
	assert((inCommandID == kCommandHideMacros) || (inCommandID == kCommandShowMacros));
	assert((commandInfoPtr->commandID == kCommandHideMacros) || (commandInfoPtr->commandID == kCommandShowMacros));
	if (inCommandID == kCommandShowMacros)
	{
		MacroSetupWindow_Display();
	}
	else
	{
		MacroSetupWindow_Remove();
	}
	return result;
}// showHideMacroSetupWindow


/*!
Updates the text field showing the name of
the active macro set.

If the dialog box is not in memory (so it’s
not on the screen either), this method does
nothing.

(3.0)
*/
static void
updateCurrentSetName ()
{
	if (gMacroSetupWindow != nullptr)
	{
		UIStrings_ResultCode				stringError = kUIStrings_ResultCodeSuccess;
		UIStrings_MacroSetupWindowCFString	stringType = kUIStrings_MacroSetupWindowSetName1;
		CFStringRef							nameString = nullptr;
		
		
		switch (Macros_GetActiveSetNumber())
		{
		case 1:
			stringType = kUIStrings_MacroSetupWindowSetName1;
			break;
		
		case 2:
			stringType = kUIStrings_MacroSetupWindowSetName2;
			break;
		
		case 3:
			stringType = kUIStrings_MacroSetupWindowSetName3;
			break;
		
		case 4:
			stringType = kUIStrings_MacroSetupWindowSetName4;
			break;
		
		case 5:
			stringType = kUIStrings_MacroSetupWindowSetName5;
			break;
		
		default:
			// ???
			break;
		}
		stringError = UIStrings_Copy(stringType, nameString);
		if (stringError == kUIStrings_ResultCodeSuccess)
		{
			SetControlTextWithCFString(gMacroSetupActiveSetNameLabel, nameString);
			DrawOneControl(gMacroSetupActiveSetNameLabel);
			CFRelease(nameString), nameString = nullptr;
		}
	}
}// updateCurrentSetName


/*!
To update a specific text field (representing
a macro’s text) to display a particular C
string, call this method.  In order to display
the string, it is translated to a Pascal string.

If the dialog box is not in memory (so it’s not
on the screen either), this method does nothing.

(3.0)
*/
static void
updateTextField		(UInt8*		inoutMacroText,
					 SInt16		inZeroBasedMacroNumber)
{
	if (gMacroSetupWindow != nullptr)
	{
		ControlRef		field = nullptr;
		OSStatus		error = noErr;
		HIViewID		controlID;
		
		
		StringUtilities_CToPInPlace((char*)inoutMacroText);
		controlID.signature = kSignatureMyFieldAnyMacro;
		controlID.id = inZeroBasedMacroNumber;
		error = GetControlByID(gMacroSetupWindow, &controlID, &field);
		if (error == noErr)
		{
			SetControlText(field, inoutMacroText);
			DrawOneControl(field);
		}
	}
}// updateTextField


/*!
To make sure that the text fields in the
Macro Setup dialog box reflect exactly the
values of the macros in memory, call this
method.

If the dialog box is not in memory (so
it’s not on the screen either), this method
does nothing.

(3.0)
*/
static void
updateTextFields ()
{
	if (gMacroSetupWindow != nullptr)
	{
		UInt8		newMacro[256];
		MacroIndex	i = 0;
		
		
		for (i = 0; i < MACRO_COUNT; ++i)
		{
			Macros_Get(Macros_GetActiveSet(), i, (char*)&newMacro, 256);
			updateTextField(newMacro, i);
		}
	}
}// updateTextFields

// BELOW IS REQUIRED NEWLINE TO END FILE
