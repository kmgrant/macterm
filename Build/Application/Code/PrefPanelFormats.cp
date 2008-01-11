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

// resource includes
#include "GeneralResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "ColorBox.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
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

#define NUMBER_OF_FORMATS_TABPANES		2
enum
{
	// must match tab order at creation, and be one-based
	kMy_TabIndexFormatNormal		= 1,
	kMy_TabIndexFormatANSIColors	= 2
};

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelsFavorites.nib".

In addition, they MUST be unique across all panels.
*/
static HIViewID const	idMyPopupMenuFont					= { FOUR_CHAR_CODE('Font'), 0/* ID */ };
static HIViewID const	idMyFieldFontSize					= { FOUR_CHAR_CODE('Size'), 0/* ID */ };
static HIViewID const	idMyLittleArrowsFontSize			= { FOUR_CHAR_CODE('SzAr'), 0/* ID */ };
static HIViewID const	idMyBevelButtonNormalText			= { FOUR_CHAR_CODE('NTxt'), 0/* ID */ };
static HIViewID const	idMyBevelButtonNormalBackground		= { FOUR_CHAR_CODE('NBkg'), 0/* ID */ };
static HIViewID const	idMyBevelButtonBoldText				= { FOUR_CHAR_CODE('BTxt'), 0/* ID */ };
static HIViewID const	idMyBevelButtonBoldBackground		= { FOUR_CHAR_CODE('BBkg'), 0/* ID */ };
static HIViewID const	idMyBevelButtonBlinkingText			= { FOUR_CHAR_CODE('BlTx'), 0/* ID */ };
static HIViewID const	idMyBevelButtonBlinkingBackground	= { FOUR_CHAR_CODE('BlBk'), 0/* ID */ };
static HIViewID const	idMyUserPaneSampleTerminalView		= { FOUR_CHAR_CODE('Smpl'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalBlack		= { FOUR_CHAR_CODE('Cblk'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalRed		= { FOUR_CHAR_CODE('Cred'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalGreen		= { FOUR_CHAR_CODE('Cgrn'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalYellow		= { FOUR_CHAR_CODE('Cyel'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalBlue		= { FOUR_CHAR_CODE('Cblu'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalMagenta	= { FOUR_CHAR_CODE('Cmag'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalCyan		= { FOUR_CHAR_CODE('Ccyn'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSINormalWhite		= { FOUR_CHAR_CODE('Cwht'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldBlack		= { FOUR_CHAR_CODE('CBlk'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldRed			= { FOUR_CHAR_CODE('CRed'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldGreen		= { FOUR_CHAR_CODE('CGrn'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldYellow		= { FOUR_CHAR_CODE('CYel'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldBlue			= { FOUR_CHAR_CODE('CBlu'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldMagenta		= { FOUR_CHAR_CODE('CMag'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldCyan			= { FOUR_CHAR_CODE('CCyn'), 0/* ID */ };
static HIViewID const	idMyBevelButtonANSIBoldWhite		= { FOUR_CHAR_CODE('CWht'), 0/* ID */ };

#pragma mark Types

/*!
Implements the “ANSI Colors” tab.
*/
struct My_FormatsTabANSIColors
{
	My_FormatsTabANSIColors		(HIWindowRef, HIViewRef);
	
	HIViewWrap				pane;
	CarbonEventHandlerWrap	buttonCommandsHandler;		//!< invoked when a button is clicked

protected:
	HIViewWrap
	createContainerView		(HIWindowRef) const;
};

/*!
Implements the “Normal” tab.
*/
struct My_FormatsTabNormal
{
	My_FormatsTabNormal		(HIWindowRef, HIViewRef);
	~My_FormatsTabNormal	();
	
	TerminalScreenRef	sampleTerminalScreen;
	HIViewWrap			pane;

protected:
	HIViewWrap
	createContainerView		(HIWindowRef, TerminalScreenRef) const;
	
	TerminalScreenRef
	createSampleTerminalScreen () const;
	
	HIViewRef
	createSampleTerminalHIView	(HIWindowRef, TerminalScreenRef) const;

private:
	CommonEventHandlers_NumericalFieldArrowsRef		_fontSizeArrowsHandler;
};

/*!
Implements the entire panel user interface.
*/
struct My_FormatsPanelUI
{
	My_FormatsPanelUI	(HIWindowRef, HIViewRef);
	
	CommonEventHandlers_HIViewResizer	containerResizer;	//!< invoked when the panel is resized
	HIViewWrap							tabView;
	CarbonEventHandlerWrap				viewClickHandler;	//!< invoked when a tab is clicked
	
	My_FormatsTabNormal			normalTab;
	My_FormatsTabANSIColors		colorsTab;

protected:
	HIViewWrap
	createTabsView	(HIWindowRef) const;
};
typedef My_FormatsPanelUI*		My_FormatsPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_FormatsPanelData
{
	My_FormatsPanelData ();
	
	Panel_Ref			panel;			//!< the panel this data is for
	My_FormatsPanelUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef My_FormatsPanelData*	My_FormatsPanelDataPtr;

#pragma mark Internal Method Prototypes

static void				colorBoxChangeNotify			(HIViewRef, RGBColor const*, void const*);
static void				createPanelViews				(Panel_Ref, WindowRef);
static void				deltaSizePanelContainerHIView	(HIViewRef, Float32, Float32, void*);
static void				disposePanel					(Panel_Ref, void*);
static SInt32			panelChanged					(Panel_Ref, Panel_Message, void*);
static pascal OSStatus	receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveViewHit					(EventHandlerCallRef, EventRef, void*);
static void				resetANSIWarningCloseNotifyProc	(InterfaceLibAlertRef, SInt16, void*);
static void				setUpMainTabs					(My_FormatsPanelUIPtr);
static void				showTabPane						(My_FormatsPanelUIPtr, UInt16);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Float32		gIdealWidth = 0.0;
	Float32		gIdealHeight = 0.0;
}



#pragma mark Public Methods

/*!
Creates a new preference panel for the Formats
category, initializes it, and returns a reference
to it.  You must destroy the reference using
Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelFormats_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (nullptr != result)
	{
		My_FormatsPanelDataPtr	dataPtr = new My_FormatsPanelData();
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorFormats);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelFormats);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowFormatsCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelFormatsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelFormats);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Initializes a My_FormatsPanelData structure.

(3.1)
*/
My_FormatsPanelData::
My_FormatsPanelData ()
:
panel(nullptr),
interfacePtr(nullptr)
{
}// My_FormatsPanelData default constructor


/*!
Initializes a My_FormatsPanelUI structure.

(3.1)
*/
My_FormatsPanelUI::
My_FormatsPanelUI	(HIWindowRef	inOwningWindow,
					 HIViewRef		inContainer)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
containerResizer	(inContainer, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
									kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
						deltaSizePanelContainerHIView, this/* context */),
tabView				(createTabsView(inOwningWindow)
						<< HIViewWrap_AssertExists
						<< HIViewWrap_EmbedIn(inContainer)),
viewClickHandler	(GetControlEventTarget(this->tabView), receiveViewHit,
						CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
						this/* user data */),
normalTab			(inOwningWindow, this->tabView),
colorsTab			(inOwningWindow, this->tabView)
{
	assert(containerResizer.isInstalled());
	assert(tabView.exists());
	assert(viewClickHandler.isInstalled());
}// My_FormatsPanelUI 2-argument constructor


/*!
Constructs the tab container for the panel.

(3.1)
*/
HIViewWrap
My_FormatsPanelUI::
createTabsView	(HIWindowRef	inOwningWindow)
const
{
	HIViewRef				result = nullptr;
	Rect					containerBounds;
	ControlTabEntry			tabInfo[NUMBER_OF_FORMATS_TABPANES];
	UIStrings_Result		stringResult = kUIStrings_ResultOK;
	OSStatus				error = noErr;
	
	
	// nullify or zero-fill everything, then set only what matters
	bzero(&tabInfo, sizeof(tabInfo));
	tabInfo[0].enabled =
		tabInfo[1].enabled = true;
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFormatsNormalTabName,
									tabInfo[kMy_TabIndexFormatNormal - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowFormatsANSIColorsTabName,
									tabInfo[kMy_TabIndexFormatANSIColors - 1].name);
	SetRect(&containerBounds, 0, 0, 0, 0);
	error = CreateTabsControl(inOwningWindow, &containerBounds, kControlTabSizeLarge, kControlTabDirectionNorth,
								sizeof(tabInfo) / sizeof(ControlTabEntry)/* number of tabs */, tabInfo,
								&result);
	assert_noerr(error);
	for (size_t i = 0; i < sizeof(tabInfo) / sizeof(ControlTabEntry); ++i)
	{
		if (nullptr != tabInfo[i].name) CFRelease(tabInfo[i].name), tabInfo[i].name = nullptr;
	}
	
	return result;
}// My_FormatsPanelUI::createTabsView


/*!
Initializes a My_FormatsTabANSIColors structure.

(3.1)
*/
My_FormatsTabANSIColors::
My_FormatsTabANSIColors		(HIWindowRef	inOwningWindow,
							 HIViewRef		inTabsView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
pane					(createContainerView(inOwningWindow)
							<< HIViewWrap_AssertExists
							<< HIViewWrap_EmbedIn(inTabsView)),
buttonCommandsHandler	(GetWindowEventTarget(inOwningWindow), receiveHICommand,
							CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
							this/* user data */)
{
	assert(this->pane.exists());
	assert(this->buttonCommandsHandler.isInstalled());
}// My_FormatsTabANSIColors 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_FormatsTabANSIColors::
createContainerView		(HIWindowRef	inOwningWindow)
const
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
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanelFormats"), CFSTR("ANSI"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
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
		}
	}
	
	return result;
}// My_FormatsTabANSIColors::createContainerView


/*!
Initializes a My_FormatsTabNormal structure.

(3.1)
*/
My_FormatsTabNormal::
My_FormatsTabNormal		(HIWindowRef	inOwningWindow,
						 HIViewRef		inTabsView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
sampleTerminalScreen	(createSampleTerminalScreen()),
pane					(createContainerView(inOwningWindow, sampleTerminalScreen)
							<< HIViewWrap_AssertExists
							<< HIViewWrap_EmbedIn(inTabsView)),
_fontSizeArrowsHandler	(nullptr) // set later
{
	assert(this->pane.exists());
	
	// make the little arrows control change the font size
	{
		HIViewWrap		fieldFontSize(idMyFieldFontSize, inOwningWindow);
		HIViewWrap		littleArrowsFontSize(idMyLittleArrowsFontSize, inOwningWindow);
		
		
		(OSStatus)CommonEventHandlers_InstallNumericalFieldArrows
					(littleArrowsFontSize, fieldFontSize, &_fontSizeArrowsHandler);
	}
}// My_FormatsTabNormal 2-argument constructor


/*!
Tears down a My_FormatsTabNormal structure.

(3.1)
*/
My_FormatsTabNormal::
~My_FormatsTabNormal ()
{
	if (nullptr != _fontSizeArrowsHandler)
	{
		CommonEventHandlers_RemoveNumericalFieldArrows(&_fontSizeArrowsHandler);
	}
}// My_FormatsTabNormal 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_FormatsTabNormal::
createContainerView		(HIWindowRef		inOwningWindow,
						 TerminalScreenRef	inSampleTerminalScreen)
const
{
	HIViewRef					result = nullptr;
	HIViewRef					sampleContainerForSizing = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	OSStatus					error = noErr;
	
	
	// create the tab pane
	SetRect(&dummy, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanelFormats"), CFSTR("Normal"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// augment the push buttons with the color box drawing handler, property and notifier;
	// TEMPORARY: in the future, this should probably be an HIView subclass of a button
	{
		HIViewID const		kButtonIDs[] =
							{
								idMyBevelButtonNormalText, idMyBevelButtonNormalBackground,
								idMyBevelButtonBoldText, idMyBevelButtonBoldBackground,
								idMyBevelButtonBlinkingText, idMyBevelButtonBlinkingBackground
							};
		
		
		for (size_t i = 0; i < sizeof(kButtonIDs) / sizeof(HIViewID); ++i)
		{
			HIViewWrap	button(kButtonIDs[i], inOwningWindow);
			
			
			assert(button.exists());
			ColorBox_AttachToBevelButton(button);
		}
	}
	
	// find items of interest
	{
		std::vector< HIViewRef >::const_iterator	toView = viewList.end();
		
		
		toView = std::find_if(viewList.begin(), viewList.end(),
								DialogUtilities_HIViewIDEqualTo(idMyUserPaneSampleTerminalView));
		assert(viewList.end() != toView);
		sampleContainerForSizing = *toView; // used only to find boundaries
	}
	
	// use the original user pane only to define the boundaries of the new terminal view
	{
		HIViewRef	terminalView = createSampleTerminalHIView(inOwningWindow, inSampleTerminalScreen);
		Rect		viewBounds;
		HIRect		floatViewBounds;
		
		
		error = HIViewAddSubview(result, terminalView);
		assert_noerr(error);
		GetControlBounds(sampleContainerForSizing, &viewBounds);
		DisposeControl(sampleContainerForSizing), sampleContainerForSizing = nullptr;
		floatViewBounds = CGRectMake(viewBounds.left, viewBounds.top, viewBounds.right - viewBounds.left,
										viewBounds.bottom - viewBounds.top);
		error = HIViewSetFrame(terminalView, &floatViewBounds);
		assert_noerr(error);
	}
	
	// assign the real font menu to the pop-up button
	{
		HIViewWrap		popupMenuFont(idMyPopupMenuFont, inOwningWindow);
		MenuRef			fontMenu = MenuBar_ReturnFontMenu();
		
		
		SetControlPopupMenuHandle(popupMenuFont, fontMenu);
	}
	
	return result;
}// My_FormatsTabNormal::createContainerView


/*!
Constructs the data storage and emulator for the
sample terminal screen view.

(3.1)
*/
TerminalScreenRef
My_FormatsTabNormal::
createSampleTerminalScreen ()
const
{
	Terminal_Result			screenCreationError = kTerminal_ResultOK;
	TerminalScreenRef		result = nullptr;
	
	
	screenCreationError = Terminal_NewScreen(0/* number of scrollback rows */, 3/* number of rows */, 80/* number of columns */,
												false/* force save */, &result);
	assert(kTerminal_ResultOK == screenCreationError);
	assert(nullptr != result);
	
	return result;
}// createSampleTerminalScreen


/*!
Constructs the sample terminal screen HIView.

(3.1)
*/
HIViewRef
My_FormatsTabNormal::
createSampleTerminalHIView	(HIWindowRef		inOwningWindow,
							 TerminalScreenRef	inSampleTerminalScreen)
const
{
	HIViewRef			result = nullptr;
	TerminalViewRef		terminalView = nullptr;
	OSStatus			error = noErr;
	
	
	terminalView = TerminalView_NewHIViewBased(inSampleTerminalScreen, inOwningWindow,
												"\pMonaco"/* font changes later */, 10/* font size changes later */);
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
										"normal \033[1mbold\033[0m \033[3mitalic\033[3m \033[6minverse\033[3m\n"); // LOCALIZE THIS
		Terminal_EmulatorProcessCString(inSampleTerminalScreen,
										"\033[4munderline\033[0m \033[5mblinking\033[5m\n"); // LOCALIZE THIS
		Terminal_EmulatorProcessCString(inSampleTerminalScreen,
										"selected\n"); // LOCALIZE THIS
		// the range selected here should be as long as the length of the word “selected” above
		TerminalView_SelectVirtualRange(terminalView, std::make_pair(std::make_pair(0, 2), std::make_pair(7, 2)));
	}
	
	return result;
}// createSampleTerminalScreen


/*!
This routine is invoked whenever the color box value
is changed.  The panel responds by updating the color
preferences and updating all open session windows to
use the new color.

(3.0)
*/
static void
colorBoxChangeNotify	(HIViewRef			inColorBoxThatChanged,
						 RGBColor const*	inNewColor,
						 void const*		UNUSED_ARGUMENT(inContextPtr))
{
	//GeneralPanelDataConstPtr	dataPtr = REINTERPRET_CAST(inContextPtr, GeneralPanelDataConstPtr);
	UInt32						colorID = 0;
	
	
	// command ID matches preferences tag
	if (noErr == GetControlCommandID(inColorBoxThatChanged, &colorID))
	{
		DrawOneControl(inColorBoxThatChanged);
		(Preferences_Result)Preferences_SetData(colorID, sizeof(*inNewColor), inNewColor);
		SessionFactory_UpdatePalettes(colorID/* preference tag */);
	}
}// colorBoxChangeNotify


/*!
Creates the views that belong in the “Formats” panel, and
attaches them to the specified owning window in the proper
embedding hierarchy.  The specified panel’s descriptor must
be "kConstantsRegistry_PrefPanelDescriptorFormats".

The views are not arranged or sized.

(3.1)
*/
static void
createPanelViews	(Panel_Ref		inPanel,
					 WindowRef		inOwningWindow)
{
	HIViewRef				container = nullptr;
	My_FormatsPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel), My_FormatsPanelDataPtr);
	My_FormatsPanelUIPtr	interfacePtr = nullptr;
	OSStatus				error = noErr;
	
	
	// create a container view, and attach it to the panel
	{
		Rect	containerBounds;
		
		
		SetRect(&containerBounds, 0, 0, 0, 0);
		error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &container);
		assert_noerr(error);
		Panel_SetContainerView(inPanel, container);
		SetControlVisibility(container, false/* visible */, false/* draw */);
	}
	
	// create the rest of the panel user interface
	dataPtr->interfacePtr = new My_FormatsPanelUI(inOwningWindow, container);
	assert(nullptr != dataPtr->interfacePtr);
	interfacePtr = dataPtr->interfacePtr;
	
	// calculate the largest area required for all tabs, and keep this as the ideal size
	{
		NIBWindow	normalTab(AppResources_ReturnBundleForNIBs(), CFSTR("PrefPanelFormats"), CFSTR("Normal"));
		NIBWindow	colorsTab(AppResources_ReturnBundleForNIBs(), CFSTR("PrefPanelFormats"), CFSTR("ANSI"));
		Rect		normalTabSize;
		Rect		colorsTabSize;
		Point		tabFrameTopLeft;
		Point		tabFrameWidthHeight;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		
		
		// determine sizes of tabs from NIBs
		normalTab << NIBLoader_AssertWindowExists;
		colorsTab << NIBLoader_AssertWindowExists;
		GetWindowBounds(normalTab, kWindowContentRgn, &normalTabSize);
		GetWindowBounds(colorsTab, kWindowContentRgn, &colorsTabSize);
		
		// also include pane margin in panel size
		Panel_CalculateTabFrame(container, &tabFrameTopLeft, &tabFrameWidthHeight);
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		// find the widest tab and the highest tab
		gIdealWidth = std::max(normalTabSize.right - normalTabSize.left,
								colorsTabSize.right - colorsTabSize.left);
		gIdealWidth += (tabFrameTopLeft.h + tabPaneTopLeft.h + tabPaneBottomRight.h + tabFrameTopLeft.h/* same as left */);
		gIdealHeight = std::max(normalTabSize.bottom - normalTabSize.top,
								colorsTabSize.bottom - colorsTabSize.top);
		gIdealHeight += (tabFrameTopLeft.v + tabPaneTopLeft.v + tabPaneBottomRight.v + tabFrameTopLeft.v/* same as top */);
	}
}// createPanelViews


/*!
Adjusts the views in the “Formats” preference panel
to match the specified change in dimensions of its
container.

(3.1)
*/
static void
deltaSizePanelContainerHIView	(HIViewRef		inView,
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			inContext)
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		HIWindowRef				kPanelWindow = GetControlOwner(inView);
		My_FormatsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inContext, My_FormatsPanelUIPtr);
		assert(nullptr != interfacePtr);
		HIViewWrap				viewWrap;
		UInt16					chosenTabIndex = STATIC_CAST(GetControl32BitValue(interfacePtr->tabView), UInt16);
		
		
		setUpMainTabs(interfacePtr);
		//(viewWrap = interfacePtr->tabView)
		//	<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
		//	;
		
		if (kMy_TabIndexFormatNormal == chosenTabIndex)
		{
			// resize the Options tab pane, and its views
			viewWrap.setCFTypeRef(interfacePtr->normalTab.pane);
			viewWrap
				<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
				;
			
			viewWrap.setCFTypeRef(HIViewWrap(idMyUserPaneSampleTerminalView, kPanelWindow));
			viewWrap
				<< HIViewWrap_DeltaSize(inDeltaX, 0/* delta-Y */)
				;
		}
		else if (kMy_TabIndexFormatANSIColors == chosenTabIndex)
		{
			// resize the Colors tab pane, and its views
			viewWrap.setCFTypeRef(interfacePtr->colorsTab.pane);
			viewWrap
				<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
				;
		}
	}
}// deltaSizePanelContainerHIView


/*!
Cleans up a panel that is about to be destroyed.

(3.1)
*/
static void
disposePanel	(Panel_Ref	UNUSED_ARGUMENT(inPanel),
				 void*		inDataPtr)
{
	My_FormatsPanelDataPtr		dataPtr = REINTERPRET_CAST(inDataPtr, My_FormatsPanelDataPtr);
	
	
	// destroy UI, if present
	if (nullptr != dataPtr->interfacePtr) delete dataPtr->interfacePtr;
	
	delete dataPtr;
}// disposePanel


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.1)
*/
static SInt32
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorFormats, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_FormatsPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_FormatsPanelDataPtr);
			WindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			createPanelViews(inPanel, *windowPtr);
			showTabPane(panelDataPtr->interfacePtr, 1/* tab index */);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			void*	panelAuxiliaryDataPtr = inDataPtr;
			
			
			disposePanel(inPanel, panelAuxiliaryDataPtr);
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
			HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			
			
			// INCOMPLETE
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			HISize&		newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((0 != gIdealWidth) && (0 != gIdealHeight))
			{
				newLimits.width = gIdealWidth;
				newLimits.height = gIdealHeight;
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
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			
			
			// UNIMPLEMENTED
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
}// panelChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyFormatsTabANSIColorsPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_FormatsTabANSIColors*	dataPtr = REINTERPRET_CAST(inMyFormatsTabANSIColorsPtr, My_FormatsTabANSIColors*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	assert(kEventKind == kEventCommandProcess);
	{
		HICommandExtended	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			switch (received.commandID)
			{
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
					Alert_MakeWindowModal(box, GetControlOwner(dataPtr->pane), false/* is window close alert */,
											resetANSIWarningCloseNotifyProc, dataPtr/* user data */);
					Alert_Display(box); // notifier disposes the alert when the sheet eventually closes
				}
				break;
			
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
				// and then (if the user accepts a new color) update open windows;
				// note that this would be easier had Apple passed the view ID
				// into the event, which is possible on Mac OS X 10.2, but alas I
				// am using the 10.1 SDK
			#if 1
				{
					UInt32		commandID = 0;
					HIViewRef	subView = HIViewGetFirstSubview(dataPtr->pane);
					
					
					while (nullptr != subView)
					{
						if ((noErr == GetControlCommandID(subView, &commandID)) &&
							(received.commandID == commandID))
						{
							(Boolean)ColorBox_UserSetColor(subView);
							break;
						}
						subView = HIViewGetNextView(subView);
					}
				}
			#endif
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
Handles "kEventControlHit" of "kEventClassControl"
for this preferences panel.  Responds by changing
the currently selected tab, etc.

(3.1)
*/
static pascal OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMyFormatsPanelUIPtr)
{
	OSStatus				result = eventNotHandledErr;
	My_FormatsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMyFormatsPanelUIPtr, My_FormatsPanelUIPtr);
	assert(nullptr != interfacePtr);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHit);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the view in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, proceed
		if (noErr == result)
		{
			result = eventNotHandledErr; // unless set otherwise
			
			if (view == interfacePtr->tabView)
			{
				showTabPane(interfacePtr, GetControl32BitValue(view));
				result = noErr; // completely handled
			}
		}
	}
	
	return result;
}// receiveViewHit


/*!
The responder to a closed “reset ANSI colors?” alert.
This routine resets the 16 displayed ANSI colors if
the item hit is the OK button, otherwise it does not
modify the displayed colors in any way.  The given
alert is destroyed.

(3.0)
*/
static void
resetANSIWarningCloseNotifyProc		(InterfaceLibAlertRef	inAlertThatClosed,
									 SInt16					inItemHit,
									 void*					inMyFormatsTabANSIColorsPtr)
{
	My_FormatsTabANSIColors*	dataPtr = REINTERPRET_CAST(inMyFormatsTabANSIColorsPtr, My_FormatsTabANSIColors*);
	
	
	// if user consented to color reset, then change all colors to defaults
	if (kAlertStdAlertOKButton == inItemHit)
	{
		// reset colors - UNIMPLEMENTED
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
}// resetANSIWarningCloseNotifyProc


/*!
Sizes and arranges the main tab view.

(3.1)
*/
static void
setUpMainTabs	(My_FormatsPanelUIPtr	inInterfacePtr)
{
	HIPoint		tabFrameTopLeft;
	HISize		tabFrameWidthHeight;
	HIRect		newFrame;
	OSStatus	error = noErr;
	
	
	Panel_CalculateTabFrame(gIdealWidth, gIdealHeight, tabFrameTopLeft, tabFrameWidthHeight);
	newFrame.origin = tabFrameTopLeft;
	newFrame.size = tabFrameWidthHeight;
	error = HIViewSetFrame(inInterfacePtr->tabView, &newFrame);
	assert_noerr(error);
}// setUpMainTabs


/*!
Sizes and arranges views in the currently-selected
tab pane of the specified “Formats” panel to fit the
dimensions of the panel’s container.

(3.0)
*/
static void
showTabPane		(My_FormatsPanelUIPtr	inUIPtr,
				 UInt16					inTabIndex)
{
	HIViewRef	selectedTabPane = nullptr;
	HIRect		newFrame;
	
	
	if (kMy_TabIndexFormatNormal == inTabIndex)
	{
		selectedTabPane = inUIPtr->normalTab.pane;
		assert_noerr(HIViewSetVisible(inUIPtr->colorsTab.pane, false/* visible */));
	}
	else if (kMy_TabIndexFormatANSIColors == inTabIndex)
	{
		selectedTabPane = inUIPtr->colorsTab.pane;
		assert_noerr(HIViewSetVisible(inUIPtr->normalTab.pane, false/* visible */));
	}
	else
	{
		assert(false && "not a recognized tab index");
	}
	
	if (nullptr != selectedTabPane)
	{
		HIViewRef	tabPaneContainer = nullptr;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		OSStatus	error = noErr;
		
		
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		tabPaneContainer = HIViewGetSuperview(selectedTabPane);
		error = HIViewGetBounds(tabPaneContainer, &newFrame);
		assert_noerr(error);
		newFrame.origin.x = tabPaneTopLeft.h;
		newFrame.origin.y = tabPaneTopLeft.v;
		newFrame.size.width -= (tabPaneBottomRight.h + tabPaneTopLeft.h);
		newFrame.size.height -= (tabPaneBottomRight.v + tabPaneTopLeft.v);
		error = HIViewSetFrame(selectedTabPane, &newFrame);
		assert_noerr(error);
		error = HIViewSetVisible(selectedTabPane, true/* visible */);
		assert_noerr(error);
	}
}// showTabPane

// BELOW IS REQUIRED NEWLINE TO END FILE
