/*###############################################################

	PrefPanelGeneral.cp
	
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
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CFUtilities.h>
#include <CocoaBasic.h>
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
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Keypads.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelGeneral.h"
#include "SessionFactory.h"
#include "TerminalView.h"
#include "TerminalWindow.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants

#define NUMBER_OF_GENERAL_TABPANES		3
enum
{
	// must match tab order at creation, and be one-based
	kMy_TabIndexGeneralOptions			= 1,
	kMy_TabIndexGeneralSpecial			= 2,
	kMy_TabIndexGeneralNotification		= 3
};

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanels.nib".

In addition, they MUST be unique across all panels.
*/
static HIViewID const	idMyCheckBoxDoNotAutoClose					= { 'DACW', 0/* ID */ };
static HIViewID const	idMyCheckBoxDoNotDimInactive				= { 'DDBW', 0/* ID */ };
static HIViewID const	idMyCheckBoxMacrosMenuVisible				= { 'McMn', 0/* ID */ };
static HIViewID const	idMyCheckBoxInvertSelectedText				= { 'ISel', 0/* ID */ };
static HIViewID const	idMyCheckBoxAutoCopySelectedText			= { 'ACST', 0/* ID */ };
static HIViewID const	idMyCheckBoxMoveCursorToDropArea			= { 'MCTD', 0/* ID */ };
static HIViewID const	idMyCheckBoxMenuKeyEquivalents				= { 'MIKE', 0/* ID */ };
static HIViewID const	idMyCheckBoxMapBackquoteToEscape			= { 'MBQE', 0/* ID */ };
static HIViewID const	idMyCheckBoxDoNotAutoCreateWindows			= { 'DCNW', 0/* ID */ };
static HIViewID const	idMyCheckBoxFocusFollowsMouse				= { 'FcFM', 0/* ID */ };
static HIViewID const	idMyLabelTerminalCursor						= { 'LCrs', 0/* ID */ };
static HIViewID const	idMyCheckBoxCursorFlashing					= { 'CurF', 0/* ID */ };
static HIViewID const	idMySegmentedViewCursorShape				= { 'CShp', 0/* ID */ };
static HIViewID const	idMyLabelWindowResizeEffect					= { 'LWRE', 0/* ID */ };
static HIViewID const	idMyRadioButtonResizeAffectsScreenSize		= { 'WRSS', 0/* ID */ };
static HIViewID const	idMyRadioButtonResizeAffectsFontSize		= { 'WRFS', 0/* ID */ };
static HIViewID const	idMyHelpTextResizeEffect					= { 'HTWR', 0/* ID */ };
static HIViewID const	idMyFieldCopyUsingSpacesForTabs				= { 'CUST', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNDefault				= { 'CNDf', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNShell				= { 'CNSh', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNLogInShell			= { 'CNLI', 0/* ID */ };
static HIViewID const	idMyRadioButtonCommandNDialog				= { 'CNDg', 0/* ID */ };
static HIViewID const	idMyLabelBellType							= { 'LSnd', 0/* ID */ };
static HIViewID const	idMyPopUpMenuBellType						= { 'BSnd', 0/* ID */ };
static HIViewID const	idMyCheckBoxVisualBell						= { 'VisB', 0/* ID */ };
static HIViewID const	idMyHelpTextBellType						= { 'HTBT', 0/* ID */ };
static HIViewID const	idMyCheckBoxNotifyTerminalBeeps				= { 'BelN', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyDoNothing				= { 'NotN', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyBadgeDockIcon			= { 'NotD', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyBounceDockIcon			= { 'NotB', 0/* ID */ };
static HIViewID const	idMyRadioButtonNotifyDisplayMessage			= { 'NotM', 0/* ID */ };

#pragma mark Types

class My_GeneralPanelUI;

/*!
Implements the “Notification” tab.
*/
struct My_GeneralTabNotification:
public HIViewWrap
{
public:
	My_GeneralTabNotification	(HIWindowRef);

protected:
	void
	assignAccessibilityRelationships ();
	
	HIViewWrap
	createPaneView	(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
	HIViewWrap							labelBell;
	HIViewWrap							popupMenuBell;
};

/*!
Implements the “Options” tab.
*/
struct My_GeneralTabOptions:
public HIViewWrap
{
public:
	My_GeneralTabOptions	(HIWindowRef);

protected:
	HIViewWrap
	createPaneView	(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
};

/*!
Implements the “Special” tab.
*/
struct My_GeneralTabSpecial:
public HIViewWrap
{
public:
	My_GeneralTabSpecial	(My_GeneralPanelUI*, HIWindowRef);
	
	CarbonEventHandlerWrap		buttonCommandsHandler;		//!< invoked when a button is clicked

protected:
	void
	assignAccessibilityRelationships ();
	
	HIViewWrap
	createPaneView	(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CarbonEventHandlerWrap				fieldSpacesPerTabInputHandler;
	CommonEventHandlers_HIViewResizer	containerResizer;
	HIViewWrap							labelCursor;
	HIViewWrap							checkBoxCursorFlashing;
	HIViewWrap							labelWindowResizeEffect;
	HIViewWrap							radioButtonResizeAffectsScreenSize;
	HIViewWrap							radioButtonResizeAffectsFontSize;
};

/*!
Implements the entire panel user interface.
*/
struct My_GeneralPanelUI
{
public:
	My_GeneralPanelUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref							panel;				//!< the panel using this UI
	My_GeneralTabOptions				optionsTab;
	My_GeneralTabSpecial				specialTab;
	My_GeneralTabNotification			notificationTab;
	HIViewWrap							tabView;
	HIViewWrap							mainView;
	CommonEventHandlers_HIViewResizer	containerResizer;	//!< invoked when the panel is resized
	CarbonEventHandlerWrap				viewClickHandler;	//!< invoked when a tab is clicked

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef) const;
	
	HIViewWrap
	createTabsView	(HIWindowRef) const;
};
typedef My_GeneralPanelUI*			My_GeneralPanelUIPtr;
typedef My_GeneralPanelUI const*	My_GeneralPanelUIConstPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_GeneralPanelData
{
	My_GeneralPanelData ();
	~My_GeneralPanelData ();
	
	Panel_Ref				panel;			//!< the panel this data is for
	My_GeneralPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef My_GeneralPanelData*	My_GeneralPanelDataPtr;

#pragma mark Internal Method Prototypes

static void				changePreferenceUpdateScreenTerminalWindowOp	(TerminalWindowRef, void*, SInt32, void*);
static void				deltaSizePanelContainerHIView					(HIViewRef, Float32, Float32, void*);
static SInt32			panelChanged									(Panel_Ref, Panel_Message, void*);
static pascal OSStatus	receiveHICommand								(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveFieldChanged								(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveViewHit									(EventHandlerCallRef, EventRef, void*);
static void				saveFieldPreferences							(My_GeneralPanelUIPtr);
static void				showTabPane										(My_GeneralPanelUIPtr, UInt16);
static Boolean			updateCheckBoxPreference						(My_GeneralPanelUIPtr, HIViewRef);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Float32		gIdealPanelWidth = 0.0;
	Float32		gIdealPanelHeight = 0.0;
	Float32		gMaximumTabPaneWidth = 0.0;
	Float32		gMaximumTabPaneHeight = 0.0;
}



#pragma mark Public Methods

/*!
Creates a new preference panel for the General
category, initializes it, and returns a reference
to it.  You must destroy the reference using
Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelGeneral_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (nullptr != result)
	{
		My_GeneralPanelDataPtr	dataPtr = new My_GeneralPanelData();
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorGeneral);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelGeneral);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowGeneralCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelGeneralIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelGeneral);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Initializes a My_GeneralPanelData structure.

(3.1)
*/
My_GeneralPanelData::
My_GeneralPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr)
{
}// My_GeneralPanelData default constructor


/*!
Tears down a My_GeneralPanelData structure.

(4.0)
*/
My_GeneralPanelData::
~My_GeneralPanelData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_GeneralPanelData destructor


/*!
Initializes a My_GeneralPanelUI structure.

(3.1)
*/
My_GeneralPanelUI::
My_GeneralPanelUI	(Panel_Ref		inPanel,
					 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel				(inPanel),
optionsTab			(inOwningWindow),
specialTab			(this, inOwningWindow),
notificationTab		(inOwningWindow),
tabView				(createTabsView(inOwningWindow)
						<< HIViewWrap_AssertExists),
mainView			(createContainerView(inPanel, inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
								kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
						deltaSizePanelContainerHIView, this/* context */),
viewClickHandler	(GetControlEventTarget(this->tabView), receiveViewHit,
						CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
						this/* user data */)
{
	assert(containerResizer.isInstalled());
	assert(viewClickHandler.isInstalled());
}// My_GeneralPanelUI 2-argument constructor


/*!
Constructs the container for the panel.  Assumes that
the tabs view already exists.

(3.1)
*/
HIViewWrap
My_GeneralPanelUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
const
{
	assert(this->tabView.exists());
	assert(0 != gMaximumTabPaneWidth);
	assert(0 != gMaximumTabPaneHeight);
	
	HIViewRef	result = nullptr;
	OSStatus	error = noErr;
	
	
	// create the container
	{
		Rect	containerBounds;
		
		
		SetRect(&containerBounds, 0, 0, 0, 0);
		error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &result);
		assert_noerr(error);
		Panel_SetContainerView(inPanel, result);
		SetControlVisibility(result, false/* visible */, false/* draw */);
	}
	
	// calculate the ideal size
	{
		Point		tabFrameTopLeft;
		Point		tabFrameWidthHeight;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		
		
		// calculate initial frame and pane offsets (ignore width/height)
		Panel_CalculateTabFrame(result, &tabFrameTopLeft, &tabFrameWidthHeight);
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		gIdealPanelWidth = tabFrameTopLeft.h + tabPaneTopLeft.h + gMaximumTabPaneWidth +
							tabPaneBottomRight.h + tabFrameTopLeft.h/* right is same as left */;
		gIdealPanelHeight = tabFrameTopLeft.v + tabPaneTopLeft.v + gMaximumTabPaneHeight +
							tabPaneBottomRight.v + tabFrameTopLeft.v/* bottom is same as top */;
		
		// make the container big enough for the tabs
		{
			HIRect		containerFrame = CGRectMake(0, 0, gIdealPanelWidth, gIdealPanelHeight);
			
			
			error = HIViewSetFrame(result, &containerFrame);
			assert_noerr(error);
		}
		
		// recalculate the frame, this time the width and height will be correct
		Panel_CalculateTabFrame(result, &tabFrameTopLeft, &tabFrameWidthHeight);
		
		// make the tabs match the ideal frame, because the size
		// and position of NIB views is used to size subviews
		// and subview resizing deltas are derived directly from
		// changes to the container view size
		{
			HIRect		containerFrame = CGRectMake(tabFrameTopLeft.h, tabFrameTopLeft.v,
													tabFrameWidthHeight.h, tabFrameWidthHeight.v);
			
			
			error = HIViewSetFrame(this->tabView, &containerFrame);
			assert_noerr(error);
		}
	}
	
	// embed the tabs in the content pane; done at this stage
	// (after positioning the tabs) just in case the origin
	// of the container is not (0, 0); this prevents the tabs
	// from having to know where they will end up in the window
	error = HIViewAddSubview(result, this->tabView);
	assert_noerr(error);
	
	return result;
}// My_GeneralPanelUI::createContainerView


/*!
Constructs the container for all panel tab panes.
Assumes that all tabs already exist.

(3.1)
*/
HIViewWrap
My_GeneralPanelUI::
createTabsView	(HIWindowRef	inOwningWindow)
const
{
	assert(this->optionsTab.exists());
	assert(this->specialTab.exists());
	assert(this->notificationTab.exists());
	
	HIViewRef				result = nullptr;
	Rect					containerBounds;
	ControlTabEntry			tabInfo[NUMBER_OF_GENERAL_TABPANES];
	UIStrings_Result		stringResult = kUIStrings_ResultOK;
	OSStatus				error = noErr;
	
	
	// nullify or zero-fill everything, then set only what matters
	bzero(&tabInfo, sizeof(tabInfo));
	tabInfo[0].enabled =
		tabInfo[1].enabled =
		tabInfo[2].enabled = true;
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowGeneralOptionsTabName,
									tabInfo[kMy_TabIndexGeneralOptions - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowGeneralSpecialTabName,
									tabInfo[kMy_TabIndexGeneralSpecial - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowGeneralNotificationTabName,
									tabInfo[kMy_TabIndexGeneralNotification - 1].name);
	SetRect(&containerBounds, 0, 0, 0, 0);
	error = CreateTabsControl(inOwningWindow, &containerBounds, kControlTabSizeLarge, kControlTabDirectionNorth,
								sizeof(tabInfo) / sizeof(ControlTabEntry)/* number of tabs */, tabInfo,
								&result);
	assert_noerr(error);
	for (size_t i = 0; i < sizeof(tabInfo) / sizeof(ControlTabEntry); ++i)
	{
		if (nullptr != tabInfo[i].name) CFRelease(tabInfo[i].name), tabInfo[i].name = nullptr;
	}
	
	// calculate the largest area required for all tabs, and keep this as the ideal size
	{
		Rect		optionsTabSize;
		Rect		specialTabSize;
		Rect		notificationTabSize;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		
		
		// determine sizes of tabs from NIBs
		GetControlBounds(this->optionsTab, &optionsTabSize);
		GetControlBounds(this->specialTab, &specialTabSize);
		GetControlBounds(this->notificationTab, &notificationTabSize);
		
		// also include pane margin in panel size
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		// find the widest tab and the highest tab
		gMaximumTabPaneWidth = std::max(optionsTabSize.right - optionsTabSize.left,
										specialTabSize.right - specialTabSize.left);
		gMaximumTabPaneWidth = std::max(notificationTabSize.right - notificationTabSize.left,
										STATIC_CAST(gMaximumTabPaneWidth, int));
		gMaximumTabPaneHeight = std::max(optionsTabSize.bottom - optionsTabSize.top,
											specialTabSize.bottom - specialTabSize.top);
		gMaximumTabPaneHeight = std::max(notificationTabSize.bottom - notificationTabSize.top,
											STATIC_CAST(gMaximumTabPaneHeight, int));
		
		// make every tab pane match the ideal pane size
		{
			HIRect		containerFrame = CGRectMake(tabPaneTopLeft.h, tabPaneTopLeft.v,
													gMaximumTabPaneWidth, gMaximumTabPaneHeight);
			
			
			error = HIViewSetFrame(this->optionsTab, &containerFrame);
			assert_noerr(error);
			error = HIViewSetFrame(this->specialTab, &containerFrame);
			assert_noerr(error);
			error = HIViewSetFrame(this->notificationTab, &containerFrame);
			assert_noerr(error);
		}
		
		// make the tabs big enough for any of the panes
		{
			HIRect		containerFrame = CGRectMake(0, 0,
													tabPaneTopLeft.h + gMaximumTabPaneWidth + tabPaneBottomRight.h,
													tabPaneTopLeft.v + gMaximumTabPaneHeight + tabPaneBottomRight.v);
			
			
			error = HIViewSetFrame(result, &containerFrame);
			assert_noerr(error);
		}
		
		// embed every tab pane in the tabs view; done at this stage
		// so that the subsequent move of the tab frame (later) will
		// also offset the embedded panes; if embedding is done too
		// soon, then the panes have to know too much about where
		// they will physically reside within the window content area
		error = HIViewAddSubview(result, this->optionsTab);
		assert_noerr(error);
		error = HIViewAddSubview(result, this->specialTab);
		assert_noerr(error);
		error = HIViewAddSubview(result, this->notificationTab);
		assert_noerr(error);
	}
	
	return result;
}// My_GeneralPanelUI::createTabsView


/*!
Initializes a My_GeneralTabNotification structure.

(3.1)
*/
My_GeneralTabNotification::
My_GeneralTabNotification	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap				(createPaneView(inOwningWindow)
							<< HIViewWrap_AssertExists),
containerResizer		(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_GeneralTabNotification::deltaSize, this/* context */),
labelBell				(idMyLabelBellType, HIViewGetWindow(*this)),
popupMenuBell			(idMyPopUpMenuBellType, HIViewGetWindow(*this))
{
	assert(exists());
	assert(containerResizer.isInstalled());
	assert(labelBell.exists());
	assert(popupMenuBell.exists());
	assignAccessibilityRelationships();
}// My_GeneralTabNotification 2-argument constructor


/*!
Creates the necessary accessibility objects and assigns
relationships between things (for example, between
labels and the things they are labelling).

Since HIViewWrap automatically retains an accessibility
object, the views used for these relationships are
stored in this class permanently in order to preserve
their accessibility information.

(3.1)
*/
void
My_GeneralTabNotification::
assignAccessibilityRelationships ()
{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	// set accessibility relationships, if possible
	if (FlagManager_Test(kFlagOS10_4API))
	{
		OSStatus	error = noErr;
		
		
		// associate the terminal bell menu with its label, and vice-versa
		error = HIObjectSetAuxiliaryAccessibilityAttribute
				(this->popupMenuBell.returnHIObjectRef(), 0/* sub-component identifier */,
					kAXTitleUIElementAttribute, this->labelBell.acquireAccessibilityObject());
		{
			void const*			values[] = { this->popupMenuBell.acquireAccessibilityObject() };
			CFRetainRelease		labelForCFArray(CFArrayCreate(kCFAllocatorDefault, values, sizeof(values) / sizeof(void const*),
																&kCFTypeArrayCallBacks), true/* is retained */);
			
			
			error = HIObjectSetAuxiliaryAccessibilityAttribute
					(this->labelBell.returnHIObjectRef(), 0/* sub-component identifier */,
						kAXServesAsTitleForUIElementsAttribute, labelForCFArray.returnCFArrayRef());
		}
	}
#endif
}// My_GeneralTabNotification::assignAccessibilityRelationships


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_GeneralTabNotification::
createPaneView		(HIWindowRef	inOwningWindow)
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
			(CFSTR("PrefPanels"), CFSTR("General/Notification"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	// initialize values
	{
		size_t		actualSize = 0;
		Boolean		visualBell = false;
		Boolean		notifyOfBeeps = false;
		
		
		unless (Preferences_GetData(kPreferences_TagVisualBell, sizeof(visualBell), &visualBell, &actualSize) ==
				kPreferences_ResultOK)
		{
			visualBell = false; // assume default, if preference can’t be found
		}
		unless (Preferences_GetData(kPreferences_TagNotifyOfBeeps, sizeof(notifyOfBeeps),
									&notifyOfBeeps, &actualSize) ==
				kPreferences_ResultOK)
		{
			notifyOfBeeps = false; // assume default, if preference can’t be found
		}
		
		// add all user sounds to the list of library sounds
		// (INCOMPLETE: should reinitialize this if it changes on disk)
		{
			CFArrayRef const	kSoundsListRef = CocoaBasic_ReturnUserSoundNames();
			CFRetainRelease		kSoundsList(kSoundsListRef, true/* is retained */);
			HIViewWrap			soundsPopUpMenuButton(idMyPopUpMenuBellType, inOwningWindow);
			MenuRef				soundsMenu = GetControlPopupMenuRef(soundsPopUpMenuButton);
			MenuItemIndex		insertBelowIndex = 0;
			CFStringRef			userPreferredSoundName = nullptr;
			Boolean				releaseUserPreferredSoundName = true;
			
			
			// determine user preferred sound, to initialize the menu selection
			unless (Preferences_GetData(kPreferences_TagBellSound, sizeof(userPreferredSoundName),
										&userPreferredSoundName, &actualSize) ==
					kPreferences_ResultOK)
			{
				userPreferredSoundName = CFSTR(""); // assume default, if preference can’t be found
				releaseUserPreferredSoundName = false;
			}
			
			// add every library sound to the menu
			if (noErr == GetIndMenuItemWithCommandID(soundsMenu, kCommandPrefBellSystemAlert, 1/* which instance to return */,
														nullptr/* menu */, &insertBelowIndex))
			{
				CFIndex const	kSoundCount = CFArrayGetCount(kSoundsListRef);
				
				
				SetControl32BitMaximum(soundsPopUpMenuButton, CountMenuItems(soundsMenu) + kSoundCount);
				for (CFIndex i = 0; i < kSoundCount; ++i)
				{
					CFStringRef		soundName = CFUtilities_StringCast(CFArrayGetValueAtIndex(kSoundsListRef, i));
					
					
					if (noErr == AppendMenuItemTextWithCFString(soundsMenu, soundName, kMenuItemAttrIgnoreMeta,
																kCommandPrefBellLibrarySound, nullptr/* new item ID */))
					{
						// if this is the saved user preference, select the item initially
						if (kCFCompareEqualTo == CFStringCompare(soundName, userPreferredSoundName, kCFCompareCaseInsensitive))
						{
							SetControl32BitValue(soundsPopUpMenuButton, insertBelowIndex + i + 1/* one-based */);
						}
					}
				}
			}
			
			// if the sound is "off", select that item instead (see "Preferences.h", the
			// "off" value is specially recognized)
			if (kCFCompareEqualTo == CFStringCompare(userPreferredSoundName, CFSTR("off"), kCFCompareCaseInsensitive))
			{
				MenuItemIndex	offIndex = 0;
				
				
				if (noErr == GetIndMenuItemWithCommandID(soundsMenu, kCommandPrefBellOff, 1/* which instance to return */,
															nullptr/* menu */, &offIndex))
				{
					SetControl32BitValue(soundsPopUpMenuButton, offIndex);
				}
			}
			
			if ((releaseUserPreferredSoundName) && (nullptr != userPreferredSoundName))
			{
				CFRelease(userPreferredSoundName), userPreferredSoundName = nullptr;
			}
		}
		
		SetControl32BitValue(HIViewWrap(idMyCheckBoxVisualBell, inOwningWindow), BooleanToCheckBoxValue(visualBell));
		SetControl32BitValue(HIViewWrap(idMyCheckBoxNotifyTerminalBeeps, inOwningWindow), BooleanToCheckBoxValue(notifyOfBeeps));
		
		// ...initialize radio buttons using user preferences
		{
			HIViewRef			selectedRadioButton = nullptr;
			HIViewWrap const	radioNotifyNothing(idMyRadioButtonNotifyDoNothing, inOwningWindow);
			HIViewWrap const	radioNotifyBadgeDockIcon(idMyRadioButtonNotifyBadgeDockIcon, inOwningWindow);
			HIViewWrap const	radioNotifyBounceDockIcon(idMyRadioButtonNotifyBounceDockIcon, inOwningWindow);
			HIViewWrap const	radioNotifyDisplayMessage(idMyRadioButtonNotifyDisplayMessage, inOwningWindow);
			UInt16				notificationPreferences = kAlert_NotifyDisplayDiamondMark;
			
			
			unless (Preferences_GetData(kPreferences_TagNotification, sizeof(notificationPreferences),
										&notificationPreferences, &actualSize) ==
					kPreferences_ResultOK)
			{
				notificationPreferences = kAlert_NotifyDisplayDiamondMark; // assume default, if preference can’t be found
			}
			SetControl32BitValue(radioNotifyNothing, kControlRadioButtonUncheckedValue);
			SetControl32BitValue(radioNotifyBadgeDockIcon, kControlRadioButtonUncheckedValue);
			SetControl32BitValue(radioNotifyBounceDockIcon, kControlRadioButtonUncheckedValue);
			SetControl32BitValue(radioNotifyDisplayMessage, kControlRadioButtonUncheckedValue);
			switch (notificationPreferences)
			{
			case kAlert_NotifyDoNothing:
				selectedRadioButton = radioNotifyNothing;
				break;
			
			case kAlert_NotifyDisplayDiamondMark:
				selectedRadioButton = radioNotifyBadgeDockIcon;
				break;
			
			case kAlert_NotifyDisplayIconAndDiamondMark:
				selectedRadioButton = radioNotifyBounceDockIcon;
				break;
			
			case kAlert_NotifyAlsoDisplayAlert:
				selectedRadioButton = radioNotifyDisplayMessage;
				break;
			
			default:
				// ???
				break;
			}
			
			if (nullptr != selectedRadioButton) SetControl32BitValue(selectedRadioButton, kControlRadioButtonCheckedValue);
		}
	}
	
	return result;
}// My_GeneralTabNotification::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_GeneralTabNotification::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_GeneralTabNotification*	dataPtr = REINTERPRET_CAST(inContext, My_GeneralTabNotification*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextBellType, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_GeneralTabNotification::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_GeneralTabNotification::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_GeneralTabNotification::operator =


/*!
Initializes a My_GeneralTabOptions structure.

(3.1)
*/
My_GeneralTabOptions::
My_GeneralTabOptions	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap			(createPaneView(inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
						My_GeneralTabOptions::deltaSize, this/* context */)
{
	assert(exists());
	assert(containerResizer.isInstalled());
}// My_GeneralTabOptions 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_GeneralTabOptions::
createPaneView		(HIWindowRef	inOwningWindow)
const
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	OSStatus					error = noErr;
	size_t						actualSize = 0L;
	Boolean						flag = false;
	
	
	// create the tab pane
	SetRect(&dummy, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanels"), CFSTR("General/Options"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	//
	// initialize checkboxes using user preferences
	//
	
	{
		HIViewWrap		checkBox(idMyCheckBoxDoNotAutoClose, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagDontAutoClose, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxDoNotDimInactive, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagDontDimBackgroundScreens, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxInvertSelectedText, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagPureInverse, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxAutoCopySelectedText, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagCopySelectedText, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxMoveCursorToDropArea, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagCursorMovesPriorToDrops, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxMapBackquoteToEscape, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagMapBackquote, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxDoNotAutoCreateWindows, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagDontAutoNewOnApplicationReopen, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	{
		HIViewWrap		checkBox(idMyCheckBoxFocusFollowsMouse, inOwningWindow);
		
		
		assert(checkBox.exists());
		unless (Preferences_GetData(kPreferences_TagFocusFollowsMouse, sizeof(flag), &flag,
									&actualSize) == kPreferences_ResultOK)
		{
			flag = false; // assume a value, if preference can’t be found
		}
		SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
	}
	return result;
}// My_GeneralTabOptions::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_GeneralTabOptions::
deltaSize	(HIViewRef		UNUSED_ARGUMENT(inContainer),
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	// nothing needed
}// My_GeneralTabOptions::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_GeneralTabOptions::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_GeneralTabOptions::operator =


/*!
Initializes a My_GeneralTabSpecial structure.

(3.1)
*/
My_GeneralTabSpecial::
My_GeneralTabSpecial	(My_GeneralPanelUIPtr	inOwningUI,
						 HIWindowRef			inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap							(createPaneView(inOwningWindow)
										<< HIViewWrap_AssertExists),
buttonCommandsHandler				(GetWindowEventTarget(inOwningWindow), receiveHICommand,
										CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
										nullptr/* user data */),
fieldSpacesPerTabInputHandler		(GetControlEventTarget(HIViewWrap(idMyFieldCopyUsingSpacesForTabs, inOwningWindow)), receiveFieldChanged,
										CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
										inOwningUI/* user data */),
containerResizer					(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
										My_GeneralTabSpecial::deltaSize, this/* context */),
labelCursor							(idMyLabelTerminalCursor, HIViewGetWindow(*this)),
checkBoxCursorFlashing				(idMyCheckBoxCursorFlashing, HIViewGetWindow(*this)),
labelWindowResizeEffect				(idMyLabelWindowResizeEffect, HIViewGetWindow(*this)),
radioButtonResizeAffectsScreenSize	(idMyRadioButtonResizeAffectsScreenSize, HIViewGetWindow(*this)),
radioButtonResizeAffectsFontSize	(idMyRadioButtonResizeAffectsFontSize, HIViewGetWindow(*this))
{
	assert(exists());
	assert(buttonCommandsHandler.isInstalled());
	assert(containerResizer.isInstalled());
	assert(labelCursor.exists());
	assert(checkBoxCursorFlashing.exists());
	assert(labelWindowResizeEffect.exists());
	assert(radioButtonResizeAffectsScreenSize.exists());
	assert(radioButtonResizeAffectsFontSize.exists());
	assignAccessibilityRelationships();
}// My_GeneralTabSpecial 2-argument constructor


/*!
Creates the necessary accessibility objects and assigns
relationships between things (for example, between
labels and the things they are labelling).

Since HIViewWrap automatically retains an accessibility
object, the views used for these relationships are
stored in this class permanently in order to preserve
their accessibility information.

(3.1)
*/
void
My_GeneralTabSpecial::
assignAccessibilityRelationships ()
{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	// set accessibility relationships, if possible
	if (FlagManager_Test(kFlagOS10_4API))
	{
		OSStatus	error = noErr;
		
		
		// associate the cursor buttons with their label, and vice-versa
		error = HIObjectSetAuxiliaryAccessibilityAttribute
				(this->checkBoxCursorFlashing.returnHIObjectRef(), 0/* sub-component identifier */,
					kAXTitleUIElementAttribute, this->labelCursor.acquireAccessibilityObject());
		{
			void const*			values[] = { this->checkBoxCursorFlashing.acquireAccessibilityObject() };
			CFRetainRelease		labelForCFArray(CFArrayCreate(kCFAllocatorDefault, values, sizeof(values) / sizeof(void const*),
																&kCFTypeArrayCallBacks), true/* is retained */);
			
			
			error = HIObjectSetAuxiliaryAccessibilityAttribute
					(this->labelCursor.returnHIObjectRef(), 0/* sub-component identifier */,
						kAXServesAsTitleForUIElementsAttribute, labelForCFArray.returnCFArrayRef());
		}
		
		// associate the window resize effect buttons with their label, and vice-versa
		error = HIObjectSetAuxiliaryAccessibilityAttribute
				(this->radioButtonResizeAffectsScreenSize.returnHIObjectRef(), 0/* sub-component identifier */,
					kAXTitleUIElementAttribute, this->labelWindowResizeEffect.acquireAccessibilityObject());
		error = HIObjectSetAuxiliaryAccessibilityAttribute
				(this->radioButtonResizeAffectsFontSize.returnHIObjectRef(), 0/* sub-component identifier */,
					kAXTitleUIElementAttribute, this->labelWindowResizeEffect.acquireAccessibilityObject());
		{
			void const*			values[] =
								{
									this->radioButtonResizeAffectsScreenSize.acquireAccessibilityObject(),
									this->radioButtonResizeAffectsFontSize.acquireAccessibilityObject()
								};
			CFRetainRelease		labelForCFArray(CFArrayCreate(kCFAllocatorDefault, values, sizeof(values) / sizeof(void const*),
																&kCFTypeArrayCallBacks), true/* is retained */);
			
			
			error = HIObjectSetAuxiliaryAccessibilityAttribute
					(this->labelWindowResizeEffect.returnHIObjectRef(), 0/* sub-component identifier */,
						kAXServesAsTitleForUIElementsAttribute, labelForCFArray.returnCFArrayRef());
		}
	}
#endif
}// My_GeneralTabSpecial::assignAccessibilityRelationships


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_GeneralTabSpecial::
createPaneView		(HIWindowRef	inOwningWindow)
const
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						dummy;
	Rect						idealContainerBounds;
	size_t						actualSize = 0L;
	OSStatus					error = noErr;
	
	
	// create the tab pane
	SetRect(&dummy, 0, 0, 0, 0);
	error = CreateUserPaneControl(inOwningWindow, &dummy, kControlSupportsEmbedding, &result);
	assert_noerr(error);
	
	// create most HIViews for the tab based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
			(CFSTR("PrefPanels"), CFSTR("General/Special"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	// set up the cursor shape bevel buttons
	{
		HIViewWrap					segmentedView(idMySegmentedViewCursorShape, inOwningWindow);
		TerminalView_CursorType		terminalCursorType = kTerminalView_CursorTypeBlock;
		
		
		unless (Preferences_GetData(kPreferences_TagTerminalCursorType, sizeof(terminalCursorType),
									&terminalCursorType, &actualSize) == kPreferences_ResultOK)
		{
			terminalCursorType = kTerminalView_CursorTypeBlock; // assume a block-shaped cursor, if preference can’t be found
		}
		
		// IMPORTANT: this must agree with what the NIB does
		(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 0/* value */);
		(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 0/* value */);
		(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 0/* value */);
		(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 0/* value */);
		(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 0/* value */);
		switch (terminalCursorType)
		{
		case kTerminalView_CursorTypeVerticalLine:
			(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeUnderscore:
			(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeThickVerticalLine:
			(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeThickUnderscore:
			(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 1/* value */);
			break;
		
		case kTerminalView_CursorTypeBlock:
		default:
			(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 1/* value */);
			break;
		}
	}
	{
		Boolean		cursorBlinks = false;
		
		
		unless (Preferences_GetData(kPreferences_TagCursorBlinks, sizeof(cursorBlinks),
									&cursorBlinks, &actualSize) ==
				kPreferences_ResultOK)
		{
			cursorBlinks = false; // assume the cursor doesn’t flash, if preference can’t be found
		}
		SetControl32BitValue(HIViewWrap(idMyCheckBoxCursorFlashing, inOwningWindow), BooleanToCheckBoxValue(cursorBlinks));
	}
	
	// set window resize preferences
	{
		HIViewWrap	radioButtonScreenSize(idMyRadioButtonResizeAffectsScreenSize, inOwningWindow);
		HIViewWrap	radioButtonFontSize(idMyRadioButtonResizeAffectsFontSize, inOwningWindow);
		Boolean		affectsFontSize = false;
		
		
		unless (Preferences_GetData(kPreferences_TagTerminalResizeAffectsFontSize,
									sizeof(affectsFontSize), &affectsFontSize,
									&actualSize) ==
				kPreferences_ResultOK)
		{
			affectsFontSize = false; // assume default, if preference can’t be found
		}
		SetControl32BitValue(radioButtonScreenSize, (false == affectsFontSize)
													? kControlRadioButtonCheckedValue
													: kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonFontSize, (affectsFontSize)
													? kControlRadioButtonCheckedValue
													: kControlRadioButtonUncheckedValue);
	}
	
	// ...create “Copy Using Spaces For Tabs” controls and initialize using user preferences
	{
		HIViewWrap	fieldView(idMyFieldCopyUsingSpacesForTabs, inOwningWindow);
		UInt16		copyTableThreshold = 0;
		
		
		unless (Preferences_GetData(kPreferences_TagCopyTableThreshold, sizeof(copyTableThreshold),
									&copyTableThreshold, &actualSize) ==
				kPreferences_ResultOK)
		{
			copyTableThreshold = 4; // assume 4 spaces per tab, if preference can’t be found
		}
		SetControlNumericalText(fieldView, copyTableThreshold);
		fieldView << HIViewWrap_InstallKeyFilter(NumericalLimiterKeyFilterUPP());
	}
	
	// ...initialize command-N radio buttons using user preferences
	{
		HIViewWrap	radioButtonDefault(idMyRadioButtonCommandNDefault, inOwningWindow);
		HIViewWrap	radioButtonShell(idMyRadioButtonCommandNShell, inOwningWindow);
		HIViewWrap	radioButtonLogInShell(idMyRadioButtonCommandNLogInShell, inOwningWindow);
		HIViewWrap	radioButtonDialog(idMyRadioButtonCommandNDialog, inOwningWindow);
		UInt32		newCommandShortcutEffect = kCommandNewSessionShell;
		
		
		unless (Preferences_GetData(kPreferences_TagNewCommandShortcutEffect,
									sizeof(newCommandShortcutEffect), &newCommandShortcutEffect,
									&actualSize) ==
				kPreferences_ResultOK)
		{
			newCommandShortcutEffect = kCommandNewSessionShell; // assume default, if preference can’t be found
		}
		SetControl32BitValue(radioButtonDefault, kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonShell, kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonLogInShell, kControlRadioButtonUncheckedValue);
		SetControl32BitValue(radioButtonDialog, kControlRadioButtonUncheckedValue);
		switch (newCommandShortcutEffect)
		{
		case kCommandNewSessionDefaultFavorite:
			SetControl32BitValue(radioButtonDefault, kControlRadioButtonCheckedValue);
			break;
		
		case kCommandNewSessionShell:
			SetControl32BitValue(radioButtonShell, kControlRadioButtonCheckedValue);
			break;
		
		case kCommandNewSessionLoginShell:
			SetControl32BitValue(radioButtonLogInShell, kControlRadioButtonCheckedValue);
			break;
		
		case kCommandNewSessionDialog:
			SetControl32BitValue(radioButtonDialog, kControlRadioButtonCheckedValue);
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// My_GeneralTabSpecial::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_GeneralTabSpecial::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_GeneralTabSpecial*	dataPtr = REINTERPRET_CAST(inContext, My_GeneralTabSpecial*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextResizeEffect, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_GeneralTabSpecial::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_GeneralTabSpecial::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_GeneralTabSpecial::operator =


/*!
This method, of SessionFactory_TerminalWindowOpProcPtr
form, will update the specified window’s terminal
screens appropriately based on the value of the
preference indicated by the given General preference
tag.

Currently, this method only supports changing of
the “use background picture” or “don’t dim background
session windows” preferences.  If one of these tags
is given, then the specified screen’s port rectangle
is invalidated (which will cause the picture to
appear or disappear).  In the case of a background
picture, the per-screen flag is also updated to
reflect the preference value.

(3.0)
*/
static void
changePreferenceUpdateScreenTerminalWindowOp	(TerminalWindowRef		inTerminalWindow,
												 void*					UNUSED_ARGUMENT(inUnusedData1),
												 SInt32					inDataPreferenceTag,
												 void*					UNUSED_ARGUMENT(inoutUndefinedResultPtr))
{
	switch (inDataPreferenceTag)
	{
	case kPreferences_TagDontDimBackgroundScreens:
	case kPreferences_TagPureInverse:
		// update the entire terminal window, which will reflect the change
		RegionUtilities_RedrawWindowOnNextUpdate(TerminalWindow_ReturnWindow(inTerminalWindow));
		break;
	
	default:
		// unrecognized tag
		break;
	}
}// changePreferenceUpdateScreenTerminalWindowOp


/*!
Adjusts the controls in the “General” preference panel
to match the specified change in dimensions of its
container. 

(3.1)
*/
static void
deltaSizePanelContainerHIView	(HIViewRef		UNUSED_ARGUMENT(inView),
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			inContext)
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		//HIWindowRef				kPanelWindow = GetControlOwner(inView);
		My_GeneralPanelUIPtr	interfacePtr = REINTERPRET_CAST(inContext, My_GeneralPanelUIPtr);
		assert(nullptr != interfacePtr);
		
		
		interfacePtr->tabView << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		
		// due to event handlers, this will automatically resize subviews too
		interfacePtr->optionsTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		interfacePtr->specialTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		interfacePtr->notificationTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
	}
}// deltaSizePanelContainerHIView


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.0)
*/
static SInt32
panelChanged	(Panel_Ref			inPanel,
				 Panel_Message		inMessage,
				 void*				inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorGeneral, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			My_GeneralPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_GeneralPanelDataPtr);
			WindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_GeneralPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
			showTabPane(panelDataPtr->interfacePtr, 1/* tab index */);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_GeneralPanelDataPtr	panelDataPtr = REINTERPRET_CAST(inDataPtr, My_GeneralPanelDataPtr);
			
			
			saveFieldPreferences(panelDataPtr->interfacePtr);
			delete panelDataPtr;
		}
		break;
	
	case kPanel_MessageFocusGained: // notification that a control is now focused
		{
			//HIViewRef const*	controlPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			
			
			// do nothing
		}
		break;
	
	case kPanel_MessageFocusLost: // notification that a control is no longer focused
		{
			//HIViewRef const*	viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			My_GeneralPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_GeneralPanelDataPtr);
			
			
			saveFieldPreferences(panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after control creation)
		{
			HISize&		newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((0 != gIdealPanelWidth) && (0 != gIdealPanelHeight))
			{
				newLimits.width = gIdealPanelWidth;
				newLimits.height = gIdealPanelHeight;
				result = kPanel_ResponseSizeProvided;
			}
		}
		break;
	
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate control sizes
		{
			// this notification is currently ignored, but shouldn’t be...
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
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the fields in this panel by saving
their preferences when new text arrives.

(4.0)
*/
static pascal OSStatus
receiveFieldChanged		(EventHandlerCallRef	inHandlerCallRef,
						 EventRef				inEvent,
						 void*					inMyGeneralPanelUIPtr)
{
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	My_GeneralPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMyGeneralPanelUIPtr, My_GeneralPanelUIPtr);
	assert(nullptr != interfacePtr);
	OSStatus				result = eventNotHandledErr;
	
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with preferences
	saveFieldPreferences(interfacePtr);
	
	return result;
}// receiveFieldChanged


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in this panel.

(3.1)
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
		HICommandExtended	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			switch (received.commandID)
			{
			case kCommandPrefCursorBlock:
			case kCommandPrefCursorUnderline:
			case kCommandPrefCursorVerticalBar:
			case kCommandPrefCursorThickUnderline:
			case kCommandPrefCursorThickVerticalBar:
				{
					assert(0 != (received.attributes & kHICommandFromControl));
					HIViewRef					segmentedView = received.source.control;
					TerminalView_CursorType		cursorType = kTerminalView_CursorTypeBlock;
					
					
					// IMPORTANT: this must agree with what the NIB does
					(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 0/* value */);
					(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 0/* value */);
					(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 0/* value */);
					(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 0/* value */);
					(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 0/* value */);
					if (kCommandPrefCursorBlock == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeBlock;
						(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 1/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorVerticalBar == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeVerticalLine;
						(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 2/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorUnderline == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeUnderscore;
						(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 3/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorThickVerticalBar == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeThickVerticalLine;
						(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 4/* segment */, 1/* value */);
					}
					if (kCommandPrefCursorThickUnderline == received.commandID)
					{
						cursorType = kTerminalView_CursorTypeThickUnderscore;
						(OSStatus)HISegmentedViewSetSegmentValue(segmentedView, 5/* segment */, 1/* value */);
					}
					Preferences_SetData(kPreferences_TagTerminalCursorType, sizeof(cursorType), &cursorType);
				}
				break;
			
			case kCommandPrefSetWindowLocation:
				Keypads_SetArrangeWindowPanelBinding(kPreferences_TagWindowStackingOrigin);
				Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, true);
				result = noErr;
				break;
			
			case kCommandPrefBellOff:
				// do not “handle” the event so the pop-up menu is updated, etc.
				{
					CFStringRef		offCFString = CFSTR("off"); // see "Preferences.h"; this value is specially recognized
					
					
					Preferences_SetData(kPreferences_TagBellSound, sizeof(offCFString), &offCFString);
				}
				result = eventNotHandledErr;
				break;
			
			case kCommandPrefBellSystemAlert:
				// play the system alert sound for the user, but do not “handle”
				// the event so the pop-up menu is updated, etc.
				Sound_StandardAlert();
				{
					CFStringRef		emptyCFString = CFSTR("");
					
					
					Preferences_SetData(kPreferences_TagBellSound, sizeof(emptyCFString), &emptyCFString);
				}
				result = eventNotHandledErr;
				break;
			
			case kCommandPrefBellLibrarySound:
				// play the indicated sound for the user, but do not “handle”
				// the event so the pop-up menu is updated, etc.
				if (received.attributes & kHICommandFromMenu)
				{
					CFStringRef		itemTextCFString = nullptr;
					
					
					if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
															received.source.menu.menuItemIndex,
															&itemTextCFString))
					{
						CocoaBasic_PlaySoundByName(itemTextCFString);
						Preferences_SetData(kPreferences_TagBellSound, sizeof(itemTextCFString), &itemTextCFString);
						CFRelease(itemTextCFString), itemTextCFString = nullptr;
					}
				}
				result = eventNotHandledErr;
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
Handles "kEventControlClick" of "kEventClassControl"
for this preferences panel.  Responds by changing
the currently selected tab, etc.

Also, temporarily, this handles clicks in other
controls.  Eventually, the buttons will simply have
commands associated with them, and a command handler
will be installed instead.

(3.1)
*/
static pascal OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMyGeneralPanelUIPtr)
{
	OSStatus				result = eventNotHandledErr;
	My_GeneralPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMyGeneralPanelUIPtr, My_GeneralPanelUIPtr);
	assert(nullptr != interfacePtr);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlHit);
	{
		HIViewRef	view = nullptr;
		
		
		// determine the control in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the control was found, proceed
		if (noErr == result)
		{
			result = eventNotHandledErr; // unless set otherwise
			
			if (view == interfacePtr->tabView)
			{
				showTabPane(interfacePtr, GetControl32BitValue(view));
				result = noErr; // completely handled
			}
			else
			{
				ControlKind		controlKind;
				OSStatus		error = GetControlKind(view, &controlKind);
				
				
				if ((noErr == error) && (kControlKindSignatureApple == controlKind.signature) &&
					((kControlKindCheckBox == controlKind.kind) || (kControlKindRadioButton == controlKind.kind)))
				{
					(Boolean)updateCheckBoxPreference(interfacePtr, view);
				}
			}
		}
	}
	
	return result;
}// receiveViewHit


/*!
Updates the preferences in memory to reflect the
information in the specified text field.  This
should be done if the focus of the window changes
(by hitting the tab key or clicking in another
text field).

(3.0)
*/
static void
saveFieldPreferences	(My_GeneralPanelUIPtr	inInterfacePtr)
{
	if (nullptr != inInterfacePtr)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(inInterfacePtr->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// save spaces-per-tab
		{
			HIViewWrap	fieldSpaces(idMyFieldCopyUsingSpacesForTabs, kOwningWindow);
			assert(fieldSpaces.exists());
			SInt32		value = 0L;
			UInt16		threshold = 0;
			
			
			GetControlNumericalText(fieldSpaces, &value);
			threshold = value;
			prefsResult = Preferences_SetData(kPreferences_TagCopyTableThreshold, sizeof(threshold), &threshold);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "failed to set spaces-per-tab");
			}
		}
	}
}// savePreferenceForField


/*!
Displays the specified tab pane and hides the others.

(3.1)
*/
static void
showTabPane		(My_GeneralPanelUIPtr	inUIPtr,
				 UInt16					inTabIndex)
{
	HIViewRef	selectedTabPane = nullptr;
	
	
	if (kMy_TabIndexGeneralOptions == inTabIndex)
	{
		selectedTabPane = inUIPtr->optionsTab;
		assert_noerr(HIViewSetVisible(inUIPtr->specialTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->notificationTab, false/* visible */));
	}
	else if (kMy_TabIndexGeneralSpecial == inTabIndex)
	{
		selectedTabPane = inUIPtr->specialTab;
		assert_noerr(HIViewSetVisible(inUIPtr->optionsTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->notificationTab, false/* visible */));
	}
	else if (kMy_TabIndexGeneralNotification == inTabIndex)
	{
		selectedTabPane = inUIPtr->notificationTab;
		assert_noerr(HIViewSetVisible(inUIPtr->optionsTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->specialTab, false/* visible */));
	}
	else
	{
		assert(false && "not a recognized tab index");
	}
	
	if (nullptr != selectedTabPane)
	{
		assert_noerr(HIViewSetVisible(selectedTabPane, true/* visible */));
	}
}// showTabPane


/*!
Call this method when a click in a checkbox or
radio button has been detected AND the control
value has been toggled to its new value.

The appropriate preference will have been updated
in memory using Preferences_SetData().

If the specified view is “known”, "true" is
returned to indicate that the click was handled.
Otherwise, "false" is returned.

(3.0)
*/
static Boolean
updateCheckBoxPreference	(My_GeneralPanelUIPtr	inInterfacePtr,
							 HIViewRef				inCheckBoxClicked)
{
	Boolean		result = false;
	
	
	if (nullptr != inInterfacePtr)
	{
		WindowRef const		kWindow = GetControlOwner(inCheckBoxClicked);
		assert(IsValidWindowRef(kWindow));
		HIViewIDWrap		viewID(HIViewWrap(inCheckBoxClicked).identifier());
		Boolean				checkBoxFlagValue = (GetControl32BitValue(inCheckBoxClicked) == kControlCheckBoxCheckedValue);
		UInt16				chosenTabIndex = STATIC_CAST(GetControl32BitValue(inInterfacePtr->tabView), UInt16);
		
		
		switch (chosenTabIndex)
		{
		case kMy_TabIndexGeneralOptions:
			if (HIViewIDWrap(idMyCheckBoxDoNotAutoClose) == viewID)
			{
				Preferences_SetData(kPreferences_TagDontAutoClose,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxDoNotDimInactive) == viewID)
			{
				Preferences_SetData(kPreferences_TagDontDimBackgroundScreens,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				SessionFactory_ForEveryTerminalWindowDo(changePreferenceUpdateScreenTerminalWindowOp,
														nullptr/* undefined */, kPreferences_TagDontDimBackgroundScreens,
														nullptr/* undefined */);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxInvertSelectedText) == viewID)
			{
				Preferences_SetData(kPreferences_TagPureInverse,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				// there’s no real reason to do this, since background window text selections will not
				// have an inverted appearance anyway; re-rendering them all wouldn’t change anything
				//SessionFactory_ForEveryTerminalWindowDo(changePreferenceUpdateScreenTerminalWindowOp,
				//										nullptr/* undefined */, kPreferences_TagPureInverse,
				//										nullptr/* undefined */);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxAutoCopySelectedText) == viewID)
			{
				Preferences_SetData(kPreferences_TagCopySelectedText,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxMoveCursorToDropArea) == viewID)
			{
				Preferences_SetData(kPreferences_TagCursorMovesPriorToDrops,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxMapBackquoteToEscape) == viewID)
			{
				Preferences_SetData(kPreferences_TagMapBackquote,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxDoNotAutoCreateWindows) == viewID)
			{
				Preferences_SetData(kPreferences_TagDontAutoNewOnApplicationReopen,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else if (HIViewIDWrap(idMyCheckBoxFocusFollowsMouse) == viewID)
			{
				Preferences_SetData(kPreferences_TagFocusFollowsMouse,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			break;
		
		case kMy_TabIndexGeneralSpecial:
			if (HIViewIDWrap(idMyCheckBoxCursorFlashing) == viewID)
			{
				// save the new value
				Preferences_SetData(kPreferences_TagCursorBlinks,
									sizeof(checkBoxFlagValue), &checkBoxFlagValue);
				result = true;
			}
			else
			{
				// check if any of the radio buttons was hit; if not, check for other control hits
				HIViewWrap		radioButtonCommandNDefault(idMyRadioButtonCommandNDefault, kWindow);
				HIViewWrap		radioButtonCommandNShell(idMyRadioButtonCommandNShell, kWindow);
				HIViewWrap		radioButtonCommandNLogInShell(idMyRadioButtonCommandNLogInShell, kWindow);
				HIViewWrap		radioButtonCommandNDialog(idMyRadioButtonCommandNDialog, kWindow);
				UInt32			newCommandShortcutEffect = 0;
				
				
				if (HIViewIDWrap(radioButtonCommandNDefault.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionDefaultFavorite;
					SetControl32BitValue(radioButtonCommandNShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNLogInShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNDialog, kControlRadioButtonUncheckedValue);
				}
				else if (HIViewIDWrap(radioButtonCommandNShell.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionShell;
					SetControl32BitValue(radioButtonCommandNDefault, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNLogInShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNDialog, kControlRadioButtonUncheckedValue);
				}
				else if (HIViewIDWrap(radioButtonCommandNLogInShell.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionLoginShell;
					SetControl32BitValue(radioButtonCommandNDefault, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNDialog, kControlRadioButtonUncheckedValue);
				}
				else if (HIViewIDWrap(radioButtonCommandNDialog.identifier()) == viewID)
				{
					newCommandShortcutEffect = kCommandNewSessionDialog;
					SetControl32BitValue(radioButtonCommandNDefault, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNShell, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonCommandNLogInShell, kControlRadioButtonUncheckedValue);
				}
				else
				{
					// unrecognized radio button
					newCommandShortcutEffect = 0;
				}
				
				if (0 != newCommandShortcutEffect)
				{
					Preferences_SetData(kPreferences_TagNewCommandShortcutEffect,
										sizeof(newCommandShortcutEffect), &newCommandShortcutEffect);
					result = true;
				}
				
				HIViewWrap		radioButtonResizeAffectsScreenSize(idMyRadioButtonResizeAffectsScreenSize, kWindow);
				HIViewWrap		radioButtonResizeAffectsFontSize(idMyRadioButtonResizeAffectsFontSize, kWindow);
				Boolean			resizeAffectsFontSize = false;
				
				
				if (HIViewIDWrap(radioButtonResizeAffectsScreenSize.identifier()) == viewID)
				{
					resizeAffectsFontSize = false;
					Preferences_SetData(kPreferences_TagTerminalResizeAffectsFontSize,
										sizeof(resizeAffectsFontSize), &resizeAffectsFontSize);
					SetControl32BitValue(radioButtonResizeAffectsFontSize, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonResizeAffectsFontSize.identifier()) == viewID)
				{
					resizeAffectsFontSize = true;
					Preferences_SetData(kPreferences_TagTerminalResizeAffectsFontSize,
										sizeof(resizeAffectsFontSize), &resizeAffectsFontSize);
					SetControl32BitValue(radioButtonResizeAffectsScreenSize, kControlRadioButtonUncheckedValue);
					result = true;
				}
			}
			break;
		
		case kMy_TabIndexGeneralNotification:
			{
				HIViewWrap		radioButtonNotificationNone(idMyRadioButtonNotifyDoNothing, kWindow);
				HIViewWrap		radioButtonNotificationBadge(idMyRadioButtonNotifyBadgeDockIcon, kWindow);
				HIViewWrap		radioButtonNotificationBounce(idMyRadioButtonNotifyBounceDockIcon, kWindow);
				HIViewWrap		radioButtonNotificationDialog(idMyRadioButtonNotifyDisplayMessage, kWindow);
				
				
				if (HIViewIDWrap(radioButtonNotificationNone.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyDoNothing;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationBadge, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBounce, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationDialog, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonNotificationBadge.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyDisplayDiamondMark;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationNone, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBounce, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationDialog, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonNotificationBounce.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyDisplayIconAndDiamondMark;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationNone, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBadge, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationDialog, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewIDWrap(radioButtonNotificationDialog.identifier()) == viewID)
				{
					SInt16		newNotificationType = kAlert_NotifyAlsoDisplayAlert;
					
					
					Preferences_SetData(kPreferences_TagNotification,
										sizeof(newNotificationType), &newNotificationType);
					SetControl32BitValue(radioButtonNotificationNone, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBadge, kControlRadioButtonUncheckedValue);
					SetControl32BitValue(radioButtonNotificationBounce, kControlRadioButtonUncheckedValue);
					result = true;
				}
				else if (HIViewWrap(idMyCheckBoxVisualBell, kWindow) == HIViewWrap(inCheckBoxClicked))
				{
					// save the new value
					Preferences_SetData(kPreferences_TagVisualBell,
										sizeof(checkBoxFlagValue), &checkBoxFlagValue);
					result = true;
				}
				else if (HIViewWrap(idMyCheckBoxNotifyTerminalBeeps, kWindow) == HIViewWrap(inCheckBoxClicked))
				{
					// save the new value
					Preferences_SetData(kPreferences_TagNotifyOfBeeps,
										sizeof(checkBoxFlagValue), &checkBoxFlagValue);
					result = true;
				}
			}
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// updateCheckBoxPreference

// BELOW IS REQUIRED NEWLINE TO END FILE
