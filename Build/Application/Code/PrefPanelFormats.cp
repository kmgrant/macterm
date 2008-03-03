/*###############################################################

	PrefPanelFormats.cp
	
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

// standard-C includes
#include <algorithm>
#include <cstring>
#include <utility>

// Unix includes
#include <strings.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventUtilities.template.h>
#include <CarbonEventHandlerWrap.template.h>
#include <ColorUtilities.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <DialogAdjust.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// resource includes
#include "GeneralResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "ColorBox.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "GenericPanelTabs.h"
#include "MenuBar.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelFormats.h"
#include "SessionFactory.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelsFavorites.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyButtonFontName					= { 'Font', 0/* ID */ };
HIViewID const	idMyButtonFontSize					= { 'Size', 0/* ID */ };
HIViewID const	idMyStaticTextNonMonospacedWarning	= { 'WMno', 0/* ID */ };
HIViewID const	idMyBevelButtonNormalText			= { 'NTxt', 0/* ID */ };
HIViewID const	idMyBevelButtonNormalBackground		= { 'NBkg', 0/* ID */ };
HIViewID const	idMyBevelButtonBoldText				= { 'BTxt', 0/* ID */ };
HIViewID const	idMyBevelButtonBoldBackground		= { 'BBkg', 0/* ID */ };
HIViewID const	idMyBevelButtonBlinkingText			= { 'BlTx', 0/* ID */ };
HIViewID const	idMyBevelButtonBlinkingBackground	= { 'BlBk', 0/* ID */ };
HIViewID const	idMyBevelButtonMatteForeground		= { 'MtTx', 0/* ID */ };
HIViewID const	idMyBevelButtonMatteBackground		= { 'MtBk', 0/* ID */ };
HIViewID const	idMyUserPaneSampleTerminalView		= { 'Smpl', 0/* ID */ };
HIViewID const	idMyHelpTextSampleTerminalView		= { 'HSmp', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalBlack		= { 'Cblk', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalRed		= { 'Cred', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalGreen		= { 'Cgrn', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalYellow		= { 'Cyel', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalBlue		= { 'Cblu', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalMagenta	= { 'Cmag', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalCyan		= { 'Ccyn', 0/* ID */ };
HIViewID const	idMyBevelButtonANSINormalWhite		= { 'Cwht', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldBlack		= { 'CBlk', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldRed			= { 'CRed', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldGreen		= { 'CGrn', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldYellow		= { 'CYel', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldBlue			= { 'CBlu', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldMagenta		= { 'CMag', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldCyan			= { 'CCyn', 0/* ID */ };
HIViewID const	idMyBevelButtonANSIBoldWhite		= { 'CWht', 0/* ID */ };
HIViewID const	idMyHelpTextANSIColors				= { 'HANS', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Implements the “ANSI Colors” tab.
*/
struct My_FormatsPanelANSIColorsUI
{
	My_FormatsPanelANSIColorsUI		(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	void
	readPreferences		(Preferences_ContextRef);
	
	void
	resetColors ();

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
private:
	CarbonEventHandlerWrap				_buttonCommandsHandler;		//!< invoked when a button is clicked
	CommonEventHandlers_HIViewResizer	_containerResizer;
};
typedef My_FormatsPanelANSIColorsUI*	My_FormatsPanelANSIColorsUIPtr;

/*!
Implements the “Normal” tab.
*/
struct My_FormatsPanelNormalUI
{
	My_FormatsPanelNormalUI		(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	void
	loseFocus	();
	
	void
	readPreferences		(Preferences_ContextRef);
	
	void
	setFontName		(StringPtr);
	
	void
	setFontSize		(SInt16);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef, TerminalScreenRef);
	
	HIViewRef
	createSampleTerminalHIView	(HIWindowRef, TerminalScreenRef) const;
	
	TerminalScreenRef
	createSampleTerminalScreen () const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	CarbonEventHandlerWrap				_buttonCommandsHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_fontPanelHandler;			//!< invoked when font panel events occur
	CarbonEventHandlerWrap				_windowFocusHandler;		//!< invoked when the window loses keyboard focus
	CommonEventHandlers_HIViewResizer	_containerResizer;
};
typedef My_FormatsPanelNormalUI*	My_FormatsPanelNormalUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_FormatsPanelANSIColorsData
{
	My_FormatsPanelANSIColorsData	();
	~My_FormatsPanelANSIColorsData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_FormatsPanelANSIColorsUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_FormatsPanelANSIColorsData*	My_FormatsPanelANSIColorsDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_FormatsPanelNormalData
{
	My_FormatsPanelNormalData	();
	~My_FormatsPanelNormalData	();
	
	Panel_Ref					panel;			//!< the panel this data is for
	My_FormatsPanelNormalUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef		dataModel;		//!< source of initializations and target of changes
};
typedef My_FormatsPanelNormalData*	My_FormatsPanelNormalDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void				colorBoxANSIChangeNotify		(HIViewRef, RGBColor const*, void*);
void				colorBoxNormalChangeNotify		(HIViewRef, RGBColor const*, void*);
Boolean				isMonospacedFont				(Str255);
SInt32				panelChangedANSIColors			(Panel_Ref, Panel_Message, void*);
SInt32				panelChangedNormal				(Panel_Ref, Panel_Message, void*);
pascal OSStatus		receiveFontChange				(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveHICommand				(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveWindowFocusChange		(EventHandlerCallRef, EventRef, void*);
void				resetANSIWarningCloseNotifyProc	(InterfaceLibAlertRef, SInt16, void*);
void				setColorBox						(Preferences_ContextRef, Preferences_Tag, HIViewRef);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new preference panel for the Formats category,
Destroy it using Panel_Dispose().

This is technically a tab container containing both the
panels from PrefPanelFormats_NewNormalPane() and
PrefPanelFormats_NewANSIColorsPane().  Since you did not
create those panels individually, you do not have to
dispose of them; destroying the tab container will delete
the tabs too.

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelFormats_New ()
{
	Panel_Ref				result = nullptr;
	CFStringRef				nameCFString = nullptr;
	GenericPanelTabs_List	tabList;
	
	
	tabList.push_back(PrefPanelFormats_NewNormalPane());
	tabList.push_back(PrefPanelFormats_NewANSIColorsPane());
	
	if (UIStrings_Copy(kUIStrings_PreferencesWindowFormatsCategoryName, nameCFString).ok())
	{
		result = GenericPanelTabs_New(nameCFString, kConstantsRegistry_PrefPanelDescriptorFormats, tabList);
		if (nullptr != result)
		{
			Panel_SetShowCommandID(result, kCommandDisplayPrefPanelFormats);
			Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension(),
											AppResources_ReturnCreatorCode(),
											kConstantsRegistry_IconServicesIconPrefPanelFormats);
		}
		CFRelease(nameCFString), nameCFString = nullptr;
	}
	
	return result;
}// New


/*!
Creates only the Normal colors pane, which allows the user
to edit the font, font sizs, and basic terminal colors with
a preview.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelFormats_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelFormats_NewANSIColorsPane ()
{
	Panel_Ref	result = Panel_New(panelChangedANSIColors);
	
	
	if (nullptr != result)
	{
		My_FormatsPanelANSIColorsDataPtr	dataPtr = new My_FormatsPanelANSIColorsData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorFormatsANSI);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelFormatsANSI);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowFormatsANSIColorsTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelFormats);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewANSIColorsPane


/*!
Creates only the Normal colors pane, which allows the user
to edit the font, font sizs, and basic terminal colors with
a preview.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelFormats_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelFormats_NewNormalPane ()
{
	Panel_Ref	result = Panel_New(panelChangedNormal);
	
	
	if (nullptr != result)
	{
		My_FormatsPanelNormalDataPtr	dataPtr = new My_FormatsPanelNormalData();
		CFStringRef						nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorFormatsNormal);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelFormatsNormal);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowFormatsNormalTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelFormats);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewNormalPane


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_FormatsPanelANSIColorsData structure.

(3.1)
*/
My_FormatsPanelANSIColorsData::
My_FormatsPanelANSIColorsData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_FormatsPanelANSIColorsData default constructor


/*!
Tears down a My_FormatsPanelANSIColorsData structure.

(3.1)
*/
My_FormatsPanelANSIColorsData::
~My_FormatsPanelANSIColorsData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_FormatsPanelANSIColorsData destructor


/*!
Initializes a My_FormatsPanelANSIColorsUI structure.

(3.1)
*/
My_FormatsPanelANSIColorsUI::
My_FormatsPanelANSIColorsUI		(Panel_Ref		inPanel,
								 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_buttonCommandsHandler	(GetWindowEventTarget(inOwningWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_FormatsPanelANSIColorsUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_buttonCommandsHandler.isInstalled());
	assert(_containerResizer.isInstalled());
}// My_FormatsPanelANSIColorsUI 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_FormatsPanelANSIColorsUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	OSStatus					error = noErr;
	
	
	// create the tab pane
	SetRect(&dummy, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	Panel_SetContainerView(inPanel, result);
	SetControlVisibility(result, false/* visible */, false/* draw */);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanelFormats"), CFSTR("ANSI"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// calculate the ideal size
	this->idealWidth = idealContainerBounds.right - idealContainerBounds.left;
	this->idealHeight = idealContainerBounds.bottom - idealContainerBounds.top;
	
	// augment the push buttons with the color box drawing handler, property and notifier;
	// TEMPORARY: in the future, this should probably be an HIView subclass of a button
	{
		HIViewID const		kButtonIDs[] =
							{
								idMyBevelButtonANSINormalBlack, idMyBevelButtonANSIBoldBlack,
								idMyBevelButtonANSINormalRed, idMyBevelButtonANSIBoldRed,
								idMyBevelButtonANSINormalGreen, idMyBevelButtonANSIBoldGreen,
								idMyBevelButtonANSINormalYellow, idMyBevelButtonANSIBoldYellow,
								idMyBevelButtonANSINormalBlue, idMyBevelButtonANSIBoldBlue,
								idMyBevelButtonANSINormalMagenta, idMyBevelButtonANSIBoldMagenta,
								idMyBevelButtonANSINormalCyan, idMyBevelButtonANSIBoldCyan,
								idMyBevelButtonANSINormalWhite, idMyBevelButtonANSIBoldWhite
							};
		
		
		for (size_t i = 0; i < sizeof(kButtonIDs) / sizeof(HIViewID); ++i)
		{
			HIViewWrap	button(kButtonIDs[i], inOwningWindow);
			
			
			assert(button.exists());
			ColorBox_AttachToBevelButton(button);
			ColorBox_SetColorChangeNotifyProc(button, colorBoxANSIChangeNotify, this/* context */);
		}
	}
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	// initialize values
	// UNIMPLEMENTED
	
	return result;
}// My_FormatsPanelANSIColorsUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_FormatsPanelANSIColorsUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const			kPanelWindow = HIViewGetWindow(inContainer);
	//My_FormatsPanelANSIColorsUI*	dataPtr = REINTERPRET_CAST(inContext, My_FormatsPanelANSIColorsUI*);
	
	HIViewWrap					viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextANSIColors, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_FormatsPanelANSIColorsUI::deltaSize


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_FormatsPanelANSIColorsUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		
		
		// read each color, skipping ones that are not defined
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlack, HIViewWrap(idMyBevelButtonANSINormalBlack, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIRed, HIViewWrap(idMyBevelButtonANSINormalRed, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIGreen, HIViewWrap(idMyBevelButtonANSINormalGreen, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIYellow, HIViewWrap(idMyBevelButtonANSINormalYellow, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlue, HIViewWrap(idMyBevelButtonANSINormalBlue, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIMagenta, HIViewWrap(idMyBevelButtonANSINormalMagenta, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSICyan, HIViewWrap(idMyBevelButtonANSINormalCyan, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIWhite, HIViewWrap(idMyBevelButtonANSINormalWhite, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlackBold, HIViewWrap(idMyBevelButtonANSIBoldBlack, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIRedBold, HIViewWrap(idMyBevelButtonANSIBoldRed, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIGreenBold, HIViewWrap(idMyBevelButtonANSIBoldGreen, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIYellowBold, HIViewWrap(idMyBevelButtonANSIBoldYellow, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIBlueBold, HIViewWrap(idMyBevelButtonANSIBoldBlue, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIMagentaBold, HIViewWrap(idMyBevelButtonANSIBoldMagenta, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSICyanBold, HIViewWrap(idMyBevelButtonANSIBoldCyan, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorANSIWhiteBold, HIViewWrap(idMyBevelButtonANSIBoldWhite, kOwningWindow));
	}
}// My_FormatsPanelNormalUI::readPreferences


/*!
Restores all 16 colors to the values from the
application’s default preferences.

(3.1)
*/
void
My_FormatsPanelANSIColorsUI::
resetColors ()
{
	Preferences_ContextRef		defaultFormat = nullptr;
	Preferences_Result			prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_GetDefaultContext(&defaultFormat, kPreferences_ClassFormat);
	if (kPreferences_ResultOK == prefsResult)
	{
		readPreferences(defaultFormat);
	}
}// My_FormatsPanelANSIColorsUI::resetColors


/*!
Initializes a My_FormatsPanelNormalData structure.

(3.1)
*/
My_FormatsPanelNormalData::
My_FormatsPanelNormalData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_FormatsPanelNormalData default constructor


/*!
Tears down a My_FormatsPanelNormalData structure.

(3.1)
*/
My_FormatsPanelNormalData::
~My_FormatsPanelNormalData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_FormatsPanelNormalData destructor


/*!
Initializes a My_FormatsPanelNormalUI structure.

(3.1)
*/
My_FormatsPanelNormalUI::
My_FormatsPanelNormalUI		(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow, createSampleTerminalScreen())
							<< HIViewWrap_AssertExists),
_buttonCommandsHandler	(GetWindowEventTarget(inOwningWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */),
_fontPanelHandler		(GetWindowEventTarget(inOwningWindow), receiveFontChange,
							CarbonEventSetInClass(CarbonEventClass(kEventClassFont), kEventFontPanelClosed, kEventFontSelection),
							this/* user data */),
_windowFocusHandler		(GetWindowEventTarget(inOwningWindow), receiveWindowFocusChange,
							CarbonEventSetInClass(CarbonEventClass(kEventClassWindow), kEventWindowFocusRelinquish),
							this/* user data */),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
									kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
							My_FormatsPanelNormalUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_buttonCommandsHandler.isInstalled());
	assert(_fontPanelHandler.isInstalled());
	assert(_windowFocusHandler.isInstalled());
	assert(_containerResizer.isInstalled());
	
	// this button is not used
	DeactivateControl(HIViewWrap(idMyBevelButtonMatteForeground, inOwningWindow));
}// My_FormatsPanelNormalUI 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_FormatsPanelNormalUI::
createContainerView		(Panel_Ref			inPanel,
						 HIWindowRef		inOwningWindow,
						 TerminalScreenRef	inSampleTerminalScreen)
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	HIRect						sampleViewFrame;
	OSStatus					error = noErr;
	
	
	// create the tab pane
	SetRect(&dummy, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	Panel_SetContainerView(inPanel, result);
	SetControlVisibility(result, false/* visible */, false/* draw */);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanelFormats"), CFSTR("Normal"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// calculate the ideal size
	this->idealWidth = idealContainerBounds.right - idealContainerBounds.left;
	this->idealHeight = idealContainerBounds.bottom - idealContainerBounds.top;
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	// augment the push buttons with the color box drawing handler, property and notifier;
	// TEMPORARY: in the future, this should probably be an HIView subclass of a button
	{
		HIViewID const		kButtonIDs[] =
							{
								idMyBevelButtonNormalText, idMyBevelButtonNormalBackground,
								idMyBevelButtonBoldText, idMyBevelButtonBoldBackground,
								idMyBevelButtonBlinkingText, idMyBevelButtonBlinkingBackground,
								idMyBevelButtonMatteBackground
							};
		
		
		for (size_t i = 0; i < sizeof(kButtonIDs) / sizeof(HIViewID); ++i)
		{
			HIViewWrap	button(kButtonIDs[i], inOwningWindow);
			
			
			assert(button.exists());
			ColorBox_AttachToBevelButton(button);
			ColorBox_SetColorChangeNotifyProc(button, colorBoxNormalChangeNotify, this/* context */);
		}
	}
	
	// find items of interest
	{
		std::vector< HIViewRef >::const_iterator	toView = viewList.end();
		
		
		// find the boundaries of the sample area; then throw it away
		toView = std::find_if(viewList.begin(), viewList.end(),
								DialogUtilities_HIViewIDEqualTo(idMyUserPaneSampleTerminalView));
		assert(viewList.end() != toView);
		error = HIViewGetFrame(*toView, &sampleViewFrame);
		assert_noerr(error);
		DisposeControl(*toView);
	}
	
	// use the original user pane only to define the boundaries of the new terminal view
	{
		HIViewRef	terminalView = createSampleTerminalHIView(inOwningWindow, inSampleTerminalScreen);
		
		
		error = HIViewAddSubview(result, terminalView);
		assert_noerr(error);
		error = HIViewSetFrame(terminalView, &sampleViewFrame);
		assert_noerr(error);
	}
	
	return result;
}// My_FormatsPanelNormalUI::createContainerView


/*!
Constructs the sample terminal screen HIView.

(3.1)
*/
HIViewRef
My_FormatsPanelNormalUI::
createSampleTerminalHIView	(HIWindowRef		inOwningWindow,
							 TerminalScreenRef	inSampleTerminalScreen)
const
{
	HIViewRef			result = nullptr;
	TerminalViewRef		terminalView = nullptr;
	OSStatus			error = noErr;
	
	
	terminalView = TerminalView_NewHIViewBased(inSampleTerminalScreen, inOwningWindow);
	assert(nullptr != terminalView);
	result = TerminalView_ReturnContainerHIView(terminalView);
	assert(nullptr != result);
	
	// tag this view with an ID so it can be found easily later
	error = SetControlID(result, &idMyUserPaneSampleTerminalView);
	assert_noerr(error);
	
	// TEMPORARY - test
	{
		RGBColor	newColor;
		
		
		newColor.red = 0;
		newColor.green = 0;
		newColor.blue = 0;
		TerminalView_SetColor(terminalView, kTerminalView_ColorIndexNormalText, &newColor);
		newColor.red = RGBCOLOR_INTENSITY_MAX;
		newColor.green = RGBCOLOR_INTENSITY_MAX;
		newColor.blue = RGBCOLOR_INTENSITY_MAX;
		TerminalView_SetColor(terminalView, kTerminalView_ColorIndexNormalBackground, &newColor);
	}
	
	// write some sample text to the view
	{
		// assumes VT100
		Terminal_EmulatorProcessCString(inSampleTerminalScreen,
										"\033[2J\033[H"); // clear screen, home cursor
		Terminal_EmulatorProcessCString(inSampleTerminalScreen,
										"normal \033[1mbold\033[0m \033[3mitalic\033[0m \033[6minverse\033[0m\015\012"); // LOCALIZE THIS
		Terminal_EmulatorProcessCString(inSampleTerminalScreen,
										"\033[4munderline\033[0m \033[5mblinking\033[0m\015\012"); // LOCALIZE THIS
		Terminal_EmulatorProcessCString(inSampleTerminalScreen,
										"selected\015\012"); // LOCALIZE THIS
		// the range selected here should be as long as the length of the word “selected” above
		TerminalView_SelectVirtualRange(terminalView, std::make_pair(std::make_pair(0, 2), std::make_pair(7, 2)));
	}
	
	return result;
}// My_FormatsPanelNormalUI::createSampleTerminalHIView


/*!
Constructs the data storage and emulator for the
sample terminal screen view.

(3.1)
*/
TerminalScreenRef
My_FormatsPanelNormalUI::
createSampleTerminalScreen ()
const
{
	Terminal_Result			screenCreationError = kTerminal_ResultOK;
	TerminalScreenRef		result = nullptr;
	
	
	screenCreationError = Terminal_NewScreen(kTerminal_EmulatorVT100, CFSTR("vt100"), 0/* number of scrollback rows */, 3/* number of rows */,
												80/* number of columns */, false/* force save */, &result);
	assert(kTerminal_ResultOK == screenCreationError);
	assert(nullptr != result);
	
	return result;
}// My_FormatsPanelNormalUI::createSampleTerminalScreen


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_FormatsPanelNormalUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		inDeltaY,
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_FormatsPanelNormalUI*	dataPtr = REINTERPRET_CAST(inContext, My_FormatsPanelNormalUI*);
	
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyButtonFontName, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyStaticTextNonMonospacedWarning, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyUserPaneSampleTerminalView, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
	viewWrap = HIViewWrap(idMyHelpTextSampleTerminalView, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
}// My_FormatsPanelNormalUI::deltaSize


/*!
Deselects any font buttons that are active, in response to
a focus-lost event.

(3.1)
*/
void
My_FormatsPanelNormalUI::
loseFocus ()
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	HIViewWrap			fontNameButton(idMyButtonFontName, kOwningWindow);
	HIViewWrap			fontSizeButton(idMyButtonFontSize, kOwningWindow);
	
	
	SetControl32BitValue(fontNameButton, kControlCheckBoxUncheckedValue);
	SetControl32BitValue(fontSizeButton, kControlCheckBoxUncheckedValue);
}// My_FormatsPanelNormalUI::loseFocus


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_FormatsPanelNormalUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// set font
		{
			Str255		fontName;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagFontName, sizeof(fontName),
														&fontName, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setFontName(fontName);
			}
		}
		
		// set font size
		{
			SInt16		fontSize = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagFontSize, sizeof(fontSize),
														&fontSize, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setFontSize(fontSize);
			}
		}
		
		// read each color, skipping ones that are not defined
		setColorBox(inSettings, kPreferences_TagTerminalColorNormalForeground, HIViewWrap(idMyBevelButtonNormalText, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorNormalBackground, HIViewWrap(idMyBevelButtonNormalBackground, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorBoldForeground, HIViewWrap(idMyBevelButtonBoldText, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorBoldBackground, HIViewWrap(idMyBevelButtonBoldBackground, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorBlinkingForeground, HIViewWrap(idMyBevelButtonBlinkingText, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorBlinkingBackground, HIViewWrap(idMyBevelButtonBlinkingBackground, kOwningWindow));
		setColorBox(inSettings, kPreferences_TagTerminalColorMatteBackground, HIViewWrap(idMyBevelButtonMatteBackground, kOwningWindow));
	}
}// My_FormatsPanelNormalUI::readPreferences


/*!
Updates the font size display based on the given setting.

(3.1)
*/
void
My_FormatsPanelNormalUI::
setFontName		(StringPtr		inFontName)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControlTitle(HIViewWrap(idMyButtonFontName, kOwningWindow), inFontName);
	HIViewSetVisible(HIViewWrap(idMyStaticTextNonMonospacedWarning, kOwningWindow), false == isMonospacedFont(inFontName));
}// My_FormatsPanelNormalUI::setFontName


/*!
Updates the font size display based on the given setting.

(3.1)
*/
void
My_FormatsPanelNormalUI::
setFontSize		(SInt16		inFontSize)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	Str255				sizeString;
	
	
	NumToString(inFontSize, sizeString);
	SetControlTitle(HIViewWrap(idMyButtonFontSize, kOwningWindow), sizeString);
}// My_FormatsPanelNormalUI::setFontSize


/*!
This routine is invoked whenever the color box value
is changed.  The panel responds by updating the color
preferences and updating all open session windows to
use the new color.

(3.1)
*/
void
colorBoxANSIChangeNotify	(HIViewRef			inColorBoxThatChanged,
							 RGBColor const*	inNewColor,
							 void*				inMyFormatsPanelANSIColorsUIPtr)
{
	My_FormatsPanelANSIColorsUIPtr		interfacePtr = REINTERPRET_CAST(inMyFormatsPanelANSIColorsUIPtr, My_FormatsPanelANSIColorsUIPtr);
	My_FormatsPanelANSIColorsDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																	My_FormatsPanelANSIColorsDataPtr);
	UInt32								colorID = 0;
	
	
	// command ID matches preferences tag
	if (noErr == GetControlCommandID(inColorBoxThatChanged, &colorID))
	{
		Boolean		isOK = false;
		
		
		if (nullptr == dataPtr->dataModel) isOK = false;
		else
		{
			Preferences_Result		prefsResult = Preferences_ContextSetData(dataPtr->dataModel, colorID,
																				sizeof(*inNewColor), inNewColor);
			
			
			isOK = (kPreferences_ResultOK == prefsResult);
		}
		
		if (false == isOK) Sound_StandardAlert();
	}
}// colorBoxANSIChangeNotify


/*!
This routine is invoked whenever the color box value
is changed.  The panel responds by updating the color
preferences and updating all open session windows to
use the new color.

(3.1)
*/
void
colorBoxNormalChangeNotify	(HIViewRef			inColorBoxThatChanged,
							 RGBColor const*	inNewColor,
							 void*				inMyFormatsPanelNormalUIPtr)
{
	My_FormatsPanelNormalUIPtr			interfacePtr = REINTERPRET_CAST(inMyFormatsPanelNormalUIPtr, My_FormatsPanelNormalUIPtr);
	My_FormatsPanelANSIColorsDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																	My_FormatsPanelANSIColorsDataPtr);
	UInt32								colorID = 0;
	
	
	// command ID matches preferences tag
	if (noErr == GetControlCommandID(inColorBoxThatChanged, &colorID))
	{
		Boolean		isOK = false;
		
		
		if (nullptr == dataPtr->dataModel) isOK = false;
		else
		{
			Preferences_Result		prefsResult = Preferences_ContextSetData(dataPtr->dataModel, colorID,
																				sizeof(*inNewColor), inNewColor);
			
			
			isOK = (kPreferences_ResultOK == prefsResult);
		}
		
		if (false == isOK) Sound_StandardAlert();
	}
}// colorBoxNormalChangeNotify


/*!
Determines if a font is monospaced.

(2.6)
*/
Boolean
isMonospacedFont	(Str255		inFontName)
{
	Boolean		result = false;
	Boolean		doRomanTest = false;
	SInt32		numberOfScriptsEnabled = GetScriptManagerVariable(smEnabled);
	
	
	if (numberOfScriptsEnabled > 1)
	{
		ScriptCode		scriptNumber = smRoman;
		FMFontFamily	fontID = FMGetFontFamilyFromName(inFontName);
		
		
		scriptNumber = FontToScript(fontID);
		if (scriptNumber != smRoman)
		{
			SInt32		thisScriptEnabled = GetScriptVariable(scriptNumber, smScriptEnabled);
			
			
			if (thisScriptEnabled)
			{
				// check if this font is the preferred monospaced font for its script
				SInt32		theSizeAndFontFamily = 0L;
				SInt16		thePreferredFontFamily = 0;
				
				
				theSizeAndFontFamily = GetScriptVariable(scriptNumber, smScriptMonoFondSize);
				thePreferredFontFamily = theSizeAndFontFamily >> 16; // high word is font family 
				result = (thePreferredFontFamily == fontID);
			}
			else result = false; // this font’s script isn’t enabled
		}
		else doRomanTest = true;
	}
	else doRomanTest = true;
		
	if (doRomanTest)
	{
		TextFontByName(inFontName);
		result = (CharWidth('W') == CharWidth('.'));
	}
	
	return result;
}// isMonospacedFont


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
panelChangedANSIColors	(Panel_Ref		inPanel,
						 Panel_Message	inMessage,
						 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorFormatsANSI, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_FormatsPanelANSIColorsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelANSIColorsDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_FormatsPanelANSIColorsUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_FormatsPanelANSIColorsDataPtr));
		}
		break;
	
	case kPanel_MessageFocusGained: // notification that a view is now focused
		{
			//HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			
			
			// do nothing
		}
		break;
	
	case kPanel_MessageFocusLost: // notification that a view is no longer focused
		{
			//HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			
			
			// do nothing
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			My_FormatsPanelANSIColorsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelANSIColorsDataPtr);
			HISize&								newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((0 != panelDataPtr->interfacePtr->idealWidth) && (0 != panelDataPtr->interfacePtr->idealHeight))
			{
				newLimits.width = panelDataPtr->interfacePtr->idealWidth;
				newLimits.height = panelDataPtr->interfacePtr->idealHeight;
				result = kPanel_ResponseSizeProvided;
			}
		}
		break;
	
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate view sizes
		{
			// this notification is currently ignored, but shouldn’t be...
		}
		break;
	
	case kPanel_MessageNewDataSet:
		{
			My_FormatsPanelANSIColorsDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelANSIColorsDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			panelDataPtr->dataModel = newContext;
			panelDataPtr->interfacePtr->readPreferences(newContext);
		}
		break;
	
	case kPanel_MessageNewVisibility: // visible state of the panel’s container has changed to visible (true) or invisible (false)
		{
			//Boolean		isNowVisible = *((Boolean*)inDataPtr);
			
			
			// do nothing
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// panelChangedANSIColors


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
panelChangedNormal	(Panel_Ref		inPanel,
					 Panel_Message	inMessage,
					 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorFormatsNormal, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_FormatsPanelNormalDataPtr);
			WindowRef const*				windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_FormatsPanelNormalUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_FormatsPanelNormalDataPtr));
		}
		break;
	
	case kPanel_MessageFocusGained: // notification that a view is now focused
		{
			//HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			
			
			// do nothing
		}
		break;
	
	case kPanel_MessageFocusLost: // notification that a view is no longer focused
		{
			//HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			
			
			// do nothing
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_FormatsPanelNormalDataPtr);
			HISize&							newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((0 != panelDataPtr->interfacePtr->idealWidth) && (0 != panelDataPtr->interfacePtr->idealHeight))
			{
				newLimits.width = panelDataPtr->interfacePtr->idealWidth;
				newLimits.height = panelDataPtr->interfacePtr->idealHeight;
				result = kPanel_ResponseSizeProvided;
			}
		}
		break;
	
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate view sizes
		{
			// this notification is currently ignored, but shouldn’t be...
		}
		break;
	
	case kPanel_MessageNewDataSet:
		{
			My_FormatsPanelNormalDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_FormatsPanelNormalDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			panelDataPtr->dataModel = newContext;
			panelDataPtr->interfacePtr->readPreferences(newContext);
		}
		break;
	
	case kPanel_MessageNewVisibility: // visible state of the panel’s container has changed to visible (true) or invisible (false)
		{
			//Boolean		isNowVisible = *((Boolean*)inDataPtr);
			
			
			// do nothing
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// panelChangedNormal


/*!
Handles "kEventFontPanelClosed" and "kEventFontSelection" of
"kEventClassFont" for the font and size of the panel.

(3.1)
*/
pascal OSStatus
receiveFontChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyFormatsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_FormatsPanelNormalUI*		normalDataPtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelNormalUI*);
	My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(normalDataPtr->panel),
																	My_FormatsPanelNormalDataPtr);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassFont);
	assert((kEventKind == kEventFontPanelClosed) || (kEventKind == kEventFontSelection));
	switch (kEventKind)
	{
	case kEventFontPanelClosed:
		// user has closed the panel; clear focus
		normalDataPtr->loseFocus();
		break;
	
	case kEventFontSelection:
		// user has accepted font changes in some way...update views
		// and internal preferences
		{
			Preferences_Result	prefsResult = kPreferences_ResultOK;
			FMFontFamily		fontFamily = kInvalidFontFamily;
			FMFontSize			fontSize = 0;
			OSStatus			error = noErr;
			
			
			result = noErr; // initially...
			
			// determine the font, if possible
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamFMFontFamily, typeFMFontFamily, fontFamily);
			if (noErr == error)
			{
				Str255		fontName;
				
				
				error = FMGetFontFamilyName(fontFamily, fontName);
				if (noErr != error) result = eventNotHandledErr;
				else
				{
					prefsResult = Preferences_ContextSetData(panelDataPtr->dataModel, kPreferences_TagFontName,
																sizeof(fontName), fontName);
					if (kPreferences_ResultOK != prefsResult)
					{
						result = eventNotHandledErr;
					}
					else
					{
						// success!
						normalDataPtr->setFontName(fontName);
					}
				}
			}
			
			// determine the font size, if possible
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamFMFontSize, typeFMFontSize, fontSize);
			if (noErr == error)
			{
				prefsResult = Preferences_ContextSetData(panelDataPtr->dataModel, kPreferences_TagFontSize,
															sizeof(fontSize), &fontSize);
				if (kPreferences_ResultOK != prefsResult)
				{
					result = eventNotHandledErr;
				}
				else
				{
					// success!
					normalDataPtr->setFontSize(fontSize);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// receiveFontChange


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
*/
pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyFormatsPanelUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	// WARNING: More than one UI uses this handler.  The context will
	// depend on the command ID.
	My_FormatsPanelANSIColorsUI*	ansiDataPtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelANSIColorsUI*);
	My_FormatsPanelNormalUI*		normalDataPtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelNormalUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommandExtended	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if ((noErr == result) && (received.attributes & kHICommandFromControl))
		{
			HIViewRef	buttonHit = received.source.control;
			
			
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandEditFontAndSize:
				// select the button that was hit, and transmit font information
				{
					My_FormatsPanelNormalDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(normalDataPtr->panel),
																					My_FormatsPanelNormalDataPtr);
					FontSelectionQDStyle			fontInfo;
					Str255							fontName;
					SInt16							fontSize = 0;
					size_t							actualSize = 0;
					Preferences_Result				prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextGetData(panelDataPtr->dataModel, kPreferences_TagFontName, sizeof(fontName),
																fontName, &actualSize);
					if (kPreferences_ResultOK != prefsResult)
					{
						// error...pick an arbitrary value
						PLstrcpy(fontName, "\pMonaco");
					}
					prefsResult = Preferences_ContextGetData(panelDataPtr->dataModel, kPreferences_TagFontSize, sizeof(fontSize),
																&fontSize, &actualSize);
					if (kPreferences_ResultOK != prefsResult)
					{
						// error...pick an arbitrary value
						fontSize = 12;
					}
					
					bzero(&fontInfo, sizeof(fontInfo));
					fontInfo.version = kFontSelectionQDStyleVersionZero;
					fontInfo.instance.fontFamily = FMGetFontFamilyFromName(fontName);
					fontInfo.instance.fontStyle = normal;
					fontInfo.size = fontSize;
					fontInfo.hasColor = false;
					// apparently this API can return paramErr even though it
					// successfully sets the desired font information...
					(OSStatus)SetFontInfoForSelection(kFontSelectionQDType, 1/* number of styles */, &fontInfo,
													// NOTE: This API is misdeclared in older headers, the last argument is supposed to
													// be an event target.  It is bastardized into HIObjectRef form for older compiles.
													#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
														GetControlEventTarget(buttonHit)
													#else
														REINTERPRET_CAST(buttonHit, HIObjectRef)
													#endif
													);
					if (1)
					{
						SetControl32BitValue(HIViewWrap(idMyButtonFontName, HIViewGetWindow(buttonHit)), kControlCheckBoxCheckedValue);
						SetControl32BitValue(HIViewWrap(idMyButtonFontSize, HIViewGetWindow(buttonHit)), kControlCheckBoxCheckedValue);
						if (false == FPIsFontPanelVisible())
						{
							result = FPShowHideFontPanel();
						}
					}
				}
				break;
			
			case kCommandResetANSIColors:
				// check with the user first!
				{
					AlertMessages_BoxRef	box = nullptr;
					UIStrings_Result		stringResult = kUIStrings_ResultOK;
					CFStringRef				dialogTextCFString = nullptr;
					CFStringRef				helpTextCFString = nullptr;
					
					
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowANSIColorsResetPrimaryText, dialogTextCFString);
					assert(stringResult.ok());
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowGenericCannotUndoHelpText, helpTextCFString);
					assert(stringResult.ok());
					
					box = Alert_New();
					Alert_SetHelpButton(box, false);
					Alert_SetParamsFor(box, kAlert_StyleOKCancel);
					Alert_SetTextCFStrings(box, dialogTextCFString, helpTextCFString);
					Alert_SetType(box, kAlertCautionAlert);
					Alert_MakeWindowModal(box, HIViewGetWindow(ansiDataPtr->mainView), false/* is window close alert */,
											resetANSIWarningCloseNotifyProc, ansiDataPtr/* user data */);
					Alert_Display(box); // notifier disposes the alert when the sheet eventually closes
				}
				result = noErr; // event is handled
				break;
			
			case kCommandColorMatteBackground:
			case kCommandColorBlinkingForeground:
			case kCommandColorBlinkingBackground:
			case kCommandColorBoldForeground:
			case kCommandColorBoldBackground:
			case kCommandColorNormalForeground:
			case kCommandColorNormalBackground:
			case kCommandColorBlack:
			case kCommandColorBlackEmphasized:
			case kCommandColorRed:
			case kCommandColorRedEmphasized:
			case kCommandColorGreen:
			case kCommandColorGreenEmphasized:
			case kCommandColorYellow:
			case kCommandColorYellowEmphasized:
			case kCommandColorBlue:
			case kCommandColorBlueEmphasized:
			case kCommandColorMagenta:
			case kCommandColorMagentaEmphasized:
			case kCommandColorCyan:
			case kCommandColorCyanEmphasized:
			case kCommandColorWhite:
			case kCommandColorWhiteEmphasized:
				// see which of the color boxes was hit, display a color chooser
				// and then (if the user accepts a new color) update open windows
				(Boolean)ColorBox_UserSetColor(buttonHit);
				result = noErr; // event is handled
				break;
			
			default:
				break;
			}
		}
		else
		{
			result = eventNotHandledErr;
		}
	}
	return result;
}// receiveHICommand


/*!
Embellishes "kEventWindowFocusRelinquish" of "kEventClassWindow"
for font buttons in this panel.

(3.1)
*/
pascal OSStatus
receiveWindowFocusChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inMyFormatsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_FormatsPanelNormalUI*	normalDataPtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelNormalUI*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowFocusRelinquish);
	{
		HIWindowRef		windowLosingFocus = nullptr;
		
		
		// determine the window that is losing focus
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, windowLosingFocus);
		if (noErr == result)
		{
			normalDataPtr->loseFocus();
			
			// never completely handle this event
			result = eventNotHandledErr;
		}
		else
		{
			result = eventNotHandledErr;
		}
	}
	return result;
}// receiveWindowFocusChange


/*!
The responder to a closed “reset ANSI colors?” alert.
This routine resets the 16 displayed ANSI colors if
the item hit is the OK button, otherwise it does not
modify the displayed colors in any way.  The given
alert is destroyed.

(3.1)
*/
void
resetANSIWarningCloseNotifyProc		(InterfaceLibAlertRef	inAlertThatClosed,
									 SInt16					inItemHit,
									 void*					inMyFormatsPanelANSIColorsUIPtr)
{
	My_FormatsPanelANSIColorsUI*	dataPtr = REINTERPRET_CAST(inMyFormatsPanelANSIColorsUIPtr, My_FormatsPanelANSIColorsUI*);
	
	
	// if user consented to color reset, then change all colors to defaults
	if (kAlertStdAlertOKButton == inItemHit)
	{
		dataPtr->resetColors();
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
}// resetANSIWarningCloseNotifyProc


/*!
Updates the specified color box with the value (if any)
of the specified preference of type RGBColor.

(3.1)
*/
void
setColorBox		(Preferences_ContextRef		inSettings,
				 Preferences_Tag			inSourceTag,
				 HIViewRef					inDestinationBox)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	RGBColor				colorValue;
	size_t					actualSize = 0;
	
	
	// read each color, skipping ones that are not defined
	prefsResult = Preferences_ContextGetData(inSettings, inSourceTag, sizeof(colorValue),
												&colorValue, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		ColorBox_SetColor(inDestinationBox, &colorValue);
	}
}// setColorBox

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
