/*###############################################################

	PrefPanelSessions.cp
	
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
#include <cstring>

// Unix includes
extern "C"
{
#	include <netdb.h>
}
#include <strings.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
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
#include "ControlResources.h"
#include "DialogResources.h"
#include "GeneralResources.h"
#include "StringResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "DNR.h"
#include "NetEvents.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelSessions.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

#define NUMBER_OF_SESSIONS_TABPANES		4
enum
{
	// must match tab order at creation, and be one-based
	kMy_TabIndexSessionHost				= 1,
	kMy_TabIndexSessionDataFlow			= 2,
	kMy_TabIndexSessionControlKeys		= 3,
	kMy_TabIndexSessionVectorGraphics	= 4
};

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelSessions.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMySeparatorTerminal			= { 'BotD', 0/* ID */ };
HIViewID const	idMySeparatorRemoteSessionsOnly	= { 'RmSD', 0/* ID */ };
HIViewID const	idMyFieldHostName				= { 'Host', 0/* ID */ };
HIViewID const	idMyButtonLookUpHostName		= { 'Look', 0/* ID */ };
HIViewID const	idMyChasingArrowsDNSWait		= { 'Wait', 0/* ID */ };
HIViewID const	idMyFieldCommandLine			= { 'CmdL', 0/* ID */ };
HIViewID const	idMyHelpTextCommandLine			= { 'CmdH', 0/* ID */ };
HIViewID const	idMyStaticTextCaptureFilePath	= { 'CapP', 0/* ID */ };
HIViewID const	idMySeparatorCaptureToFile		= { 'CapD', 0/* ID */ };
HIViewID const	idMyHelpTextControlKeys			= { 'CtlH', 0/* ID */ };
HIViewID const	idMyButtonChangeInterruptKey	= { 'Intr', 0/* ID */ };
HIViewID const	idMyButtonChangeSuspendKey		= { 'Susp', 0/* ID */ };
HIViewID const	idMyButtonChangeResumeKey		= { 'Resu', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Implements the “Resource” tab.
*/
struct My_SessionsTabResource:
public HIViewWrap
{
	My_SessionsTabResource	(HIWindowRef);

protected:
	HIViewWrap
	createPaneView		(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
	CarbonEventHandlerWrap				buttonCommandsHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap				whenLookupCompleteHandler;	//!< invoked when a DNS query finally returns
};

/*!
Implements the “Data Flow” tab.
*/
struct My_SessionsTabDataFlow:
public HIViewWrap
{
	My_SessionsTabDataFlow	(HIWindowRef);

protected:
	HIViewWrap
	createPaneView		(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
};

/*!
Implements the “Control Keys” tab.
*/
struct My_SessionsTabControlKeys:
public HIViewWrap
{
	My_SessionsTabControlKeys	(HIWindowRef);

protected:
	HIViewWrap
	createPaneView		(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
};

/*!
Implements the “TEK” tab.
*/
struct My_SessionsTabVectorGraphics:
public HIViewWrap
{
	My_SessionsTabVectorGraphics	(HIWindowRef);

protected:
	HIViewWrap
	createPaneView		(HIWindowRef) const;
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
	//! you should prefer setCFTypeRef(), which is clearer
	inline CFRetainRelease&
	operator =	(CFRetainRelease const&);

private:
	CommonEventHandlers_HIViewResizer	containerResizer;
};

/*!
Implements the entire panel user interface.
*/
struct My_SessionsPanelUI
{
	My_SessionsPanelUI	(Panel_Ref, HIWindowRef);
	
	My_SessionsTabResource				resourceTab;
	My_SessionsTabDataFlow				dataFlowTab;
	My_SessionsTabControlKeys			controlKeysTab;
	My_SessionsTabVectorGraphics		vectorGraphicsTab;
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
typedef My_SessionsPanelUI*			My_SessionsPanelUIPtr;
typedef My_SessionsPanelUI const*	My_SessionsPanelUIConstPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelData
{
	My_SessionsPanelData ();
	
	Panel_Ref				panel;			//!< the panel this data is for
	My_SessionsPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef My_SessionsPanelData*	My_SessionsPanelDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void				deltaSizePanelContainerHIView			(HIViewRef, Float32, Float32, void*);
void				disposePanel							(Panel_Ref, void*);
Boolean				lookupHostName							(My_SessionsTabResource&);
void				makeAllBevelButtonsUseTheSystemFont		(HIWindowRef);
SInt32				panelChanged							(Panel_Ref, Panel_Message, void*);
pascal OSStatus		receiveHICommand						(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveLookupComplete					(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveViewHit							(EventHandlerCallRef, EventRef, void*);
void				showTabPane								(My_SessionsPanelUIPtr, UInt16);

} // anonymous namespace

#pragma mark Variables
namespace {

Float32		gIdealPanelWidth = 0.0;
Float32		gIdealPanelHeight = 0.0;
Float32		gMaximumTabPaneWidth = 0.0;
Float32		gMaximumTabPaneHeight = 0.0;

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new preference panel for the Sessions
category, initializes it, and returns a reference
to it.  You must destroy the reference using
Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelSessions_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelDataPtr		dataPtr = new My_SessionsPanelData();
		CFStringRef					nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessions);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessions);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowSessionsCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// New


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_SessionsPanelData structure.

(3.1)
*/
My_SessionsPanelData::
My_SessionsPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr)
{
}// My_SessionsPanelData default constructor


/*!
Initializes a My_SessionsPanelUI structure.

(3.1)
*/
My_SessionsPanelUI::
My_SessionsPanelUI	(Panel_Ref		inPanel,
					 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
resourceTab			(inOwningWindow),
dataFlowTab			(inOwningWindow),
controlKeysTab		(inOwningWindow),
vectorGraphicsTab	(inOwningWindow),
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
}// My_SessionsPanelUI 2-argument constructor


/*!
Constructs the container for the panel.  Assumes that
the tabs view already exists.

(3.1)
*/
HIViewWrap
My_SessionsPanelUI::
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
}// My_SessionsPanelUI::createContainerView


/*!
Constructs the tab container for the panel.

(3.1)
*/
HIViewWrap
My_SessionsPanelUI::
createTabsView	(HIWindowRef	inOwningWindow)
const
{
	assert(this->resourceTab.exists());
	assert(this->dataFlowTab.exists());
	assert(this->controlKeysTab.exists());
	assert(this->vectorGraphicsTab.exists());
	
	HIViewRef			result = nullptr;
	Rect				containerBounds;
	ControlTabEntry		tabInfo[NUMBER_OF_SESSIONS_TABPANES];
	UIStrings_Result	stringResult = kUIStrings_ResultOK;
	OSStatus			error = noErr;
	
	
	// nullify or zero-fill everything, then set only what matters
	bzero(&tabInfo, sizeof(tabInfo));
	tabInfo[0].enabled =
		tabInfo[1].enabled =
		tabInfo[2].enabled =
		tabInfo[3].enabled = true;
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsHostTabName,
									tabInfo[kMy_TabIndexSessionHost - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsDataFlowTabName,
									tabInfo[kMy_TabIndexSessionDataFlow - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsControlKeysTabName,
									tabInfo[kMy_TabIndexSessionControlKeys - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsVectorGraphicsTabName,
									tabInfo[kMy_TabIndexSessionVectorGraphics - 1].name);
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
		Rect		resourceTabSize;
		Rect		dataFlowTabSize;
		Rect		controlKeysTabSize;
		Rect		vectorGraphicsTabSize;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		
		
		// determine sizes of tabs from NIBs
		GetControlBounds(this->resourceTab, &resourceTabSize);
		GetControlBounds(this->dataFlowTab, &dataFlowTabSize);
		GetControlBounds(this->controlKeysTab, &controlKeysTabSize);
		GetControlBounds(this->vectorGraphicsTab, &vectorGraphicsTabSize);
		
		// also include pane margin in panel size
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		// find the widest tab and the highest tab
		gMaximumTabPaneWidth = std::max(resourceTabSize.right - resourceTabSize.left,
										dataFlowTabSize.right - dataFlowTabSize.left);
		gMaximumTabPaneWidth = std::max(controlKeysTabSize.right - controlKeysTabSize.left,
										STATIC_CAST(gMaximumTabPaneWidth, int));
		gMaximumTabPaneWidth = std::max(vectorGraphicsTabSize.right - vectorGraphicsTabSize.left,
										STATIC_CAST(gMaximumTabPaneWidth, int));
		gMaximumTabPaneHeight = std::max(resourceTabSize.bottom - resourceTabSize.top,
											dataFlowTabSize.bottom - dataFlowTabSize.top);
		gMaximumTabPaneHeight = std::max(controlKeysTabSize.bottom - controlKeysTabSize.top,
											STATIC_CAST(gMaximumTabPaneHeight, int));
		gMaximumTabPaneHeight = std::max(vectorGraphicsTabSize.bottom - vectorGraphicsTabSize.top,
											STATIC_CAST(gMaximumTabPaneHeight, int));
		
		// make every tab pane match the ideal pane size
		{
			HIRect		containerFrame = CGRectMake(tabPaneTopLeft.h, tabPaneTopLeft.v,
													gMaximumTabPaneWidth, gMaximumTabPaneHeight);
			
			
			error = HIViewSetFrame(this->resourceTab, &containerFrame);
			assert_noerr(error);
			error = HIViewSetFrame(this->dataFlowTab, &containerFrame);
			assert_noerr(error);
			error = HIViewSetFrame(this->controlKeysTab, &containerFrame);
			assert_noerr(error);
			error = HIViewSetFrame(this->vectorGraphicsTab, &containerFrame);
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
		error = HIViewAddSubview(result, this->resourceTab);
		assert_noerr(error);
		error = HIViewAddSubview(result, this->dataFlowTab);
		assert_noerr(error);
		error = HIViewAddSubview(result, this->controlKeysTab);
		assert_noerr(error);
		error = HIViewAddSubview(result, this->vectorGraphicsTab);
		assert_noerr(error);
	}
	
	return result;
}// My_SessionsPanelUI::createTabsView


/*!
Initializes a My_SessionsTabControlKeys structure.

(3.1)
*/
My_SessionsTabControlKeys::
My_SessionsTabControlKeys	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap			(createPaneView(inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
						My_SessionsTabControlKeys::deltaSize, this/* context */)
{
	assert(exists());
	assert(containerResizer.isInstalled());
}// My_SessionsTabControlKeys 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsTabControlKeys::
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
			(CFSTR("PrefPanelSessions"), CFSTR("ControlKeys"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// this tab has extra buttons because it is also used for a sheet;
	// hide the extra buttons
	HIViewWrap		okayButton(HIViewIDWrap('Okay', 0), inOwningWindow);
	HIViewWrap		cancelButton(HIViewIDWrap('Canc', 0), inOwningWindow);
	okayButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	cancelButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	
	// make the container match the ideal size, because the tabs view
	// will need this guideline when deciding its largest size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	// make all bevel buttons use a larger font than is provided by default in a NIB
	makeAllBevelButtonsUseTheSystemFont(inOwningWindow);
	
	// make the main bevel buttons use a bold font
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyButtonChangeInterruptKey, inOwningWindow), kThemeAlertHeaderFont);
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyButtonChangeSuspendKey, inOwningWindow), kThemeAlertHeaderFont);
	Localization_SetControlThemeFontInfo(HIViewWrap(idMyButtonChangeResumeKey, inOwningWindow), kThemeAlertHeaderFont);
	
	// initialize values
	// UNIMPLEMENTED
	
	return result;
}// My_SessionsTabControlKeys::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsTabControlKeys::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_SessionsTabControlKeys*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabControlKeys*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextControlKeys, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_SessionsTabControlKeys::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_SessionsTabControlKeys::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_SessionsTabControlKeys::operator =


/*!
Initializes a My_SessionsTabDataFlow structure.

(3.1)
*/
My_SessionsTabDataFlow::
My_SessionsTabDataFlow	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap			(createPaneView(inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
						My_SessionsTabDataFlow::deltaSize, this/* context */)
{
	assert(exists());
	assert(containerResizer.isInstalled());
}// My_SessionsTabDataFlow 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsTabDataFlow::
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
			(CFSTR("PrefPanelSessions"), CFSTR("DataFlow"), inOwningWindow,
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
	// UNIMPLEMENTED
	
	return result;
}// My_SessionsTabDataFlow::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsTabDataFlow::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_SessionsTabDataFlow*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabDataFlow*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyStaticTextCaptureFilePath, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMySeparatorCaptureToFile, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	// INCOMPLETE
}// My_SessionsTabDataFlow::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_SessionsTabDataFlow::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_SessionsTabDataFlow::operator =


/*!
Initializes a My_SessionsTabResource structure.

(3.1)
*/
My_SessionsTabResource::
My_SessionsTabResource	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap					(createPaneView(inOwningWindow)
								<< HIViewWrap_AssertExists),
containerResizer			(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
								My_SessionsTabResource::deltaSize, this/* context */),
buttonCommandsHandler		(GetWindowEventTarget(inOwningWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this/* user data */),
whenLookupCompleteHandler	(GetApplicationEventTarget(), receiveLookupComplete,
								CarbonEventSetInClass(CarbonEventClass(kEventClassNetEvents_DNS),
														kEventNetEvents_HostLookupComplete),
								this/* user data */)
{
	assert(exists());
	assert(containerResizer.isInstalled());
}// My_SessionsTabResource 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsTabResource::
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
			(CFSTR("PrefPanelSessions"), CFSTR("Resource"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// this tab has extra buttons because it is also used for a sheet;
	// hide the extra buttons
	HIViewWrap		goButton(HIViewIDWrap('GoDo', 0), inOwningWindow);
	HIViewWrap		cancelButton(HIViewIDWrap('Canc', 0), inOwningWindow);
	goButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	cancelButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	
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
}// My_SessionsTabResource::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsTabResource::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_SessionsTabResource*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabResource*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMySeparatorTerminal, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMySeparatorRemoteSessionsOnly, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyChasingArrowsDNSWait, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyButtonLookUpHostName, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyFieldHostName, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyHelpTextCommandLine, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyFieldCommandLine, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	// INCOMPLETE
}// My_SessionsTabResource::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_SessionsTabResource::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_SessionsTabResource::operator =


/*!
Initializes a My_SessionsTabVectorGraphics structure.

(3.1)
*/
My_SessionsTabVectorGraphics::
My_SessionsTabVectorGraphics	(HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
HIViewWrap			(createPaneView(inOwningWindow)
						<< HIViewWrap_AssertExists),
containerResizer	(*this, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
						My_SessionsTabVectorGraphics::deltaSize, this/* context */)
{
	assert(exists());
	assert(containerResizer.isInstalled());
}// My_SessionsTabVectorGraphics 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsTabVectorGraphics::
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
			(CFSTR("PrefPanelSessions"), CFSTR("TEK"), inOwningWindow,
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
	// UNIMPLEMENTED
	
	return result;
}// My_SessionsTabVectorGraphics::createPaneView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsTabVectorGraphics::
deltaSize	(HIViewRef		inContainer,
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_SessionsTabVectorGraphics*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabVectorGraphics*);
	
	
	// UNIMPLEMENTED
}// My_SessionsTabVectorGraphics::deltaSize


/*!
Exists so the superclass assignment operator is
not hidden by an implicit assignment operator
definition.

(3.1)
*/
CFRetainRelease&
My_SessionsTabVectorGraphics::
operator =	(CFRetainRelease const&		inCopy)
{
	return HIViewWrap::operator =(inCopy);
}// My_SessionsTabVectorGraphics::operator =


/*!
Adjusts the views in the “Sessions” preference panel
to match the specified change in dimensions of its
container.

(3.1)
*/
void
deltaSizePanelContainerHIView	(HIViewRef		UNUSED_ARGUMENT(inView),
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			inContext)
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		//HIWindowRef				kPanelWindow = GetControlOwner(inView);
		My_SessionsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inContext, My_SessionsPanelUIPtr);
		assert(nullptr != interfacePtr);
		
		
		
		interfacePtr->tabView << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		
		// due to event handlers, this will automatically resize subviews too
		interfacePtr->resourceTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		interfacePtr->dataFlowTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		interfacePtr->controlKeysTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
		interfacePtr->vectorGraphicsTab << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
	}
}// deltaSizePanelContainerHIView


/*!
Cleans up a panel that is about to be destroyed.

(3.1)
*/
void
disposePanel	(Panel_Ref	UNUSED_ARGUMENT(inPanel),
				 void*		inDataPtr)
{
	My_SessionsPanelDataPtr		dataPtr = REINTERPRET_CAST(inDataPtr, My_SessionsPanelDataPtr);
	
	
	// destroy UI, if present
	if (nullptr != dataPtr->interfacePtr) delete dataPtr->interfacePtr;
	
	delete dataPtr;
}// disposePanel


/*!
Performs a DNR lookup of the host given in the dialog
box’s main text field, and when complete replaces the
contents of the text field with the resolved IP address
(as a string).

Returns true if the lookup succeeded.

(3.0)
*/
Boolean
lookupHostName		(My_SessionsTabResource&	inTab)
{
	CFStringRef		textCFString = nullptr;
	HIViewWrap		fieldHostName(idMyFieldHostName, GetControlOwner(inTab));
	Boolean			result = false;
	
	
	assert(fieldHostName.exists());
	GetControlTextAsCFString(fieldHostName, textCFString);
	if (CFStringGetLength(textCFString) <= 0)
	{
		// there has to be some text entered there; let the user
		// know that a blank is unacceptable
		Sound_StandardAlert();
	}
	else
	{
		char	hostName[256];
		
		
		//lookupWaitUI(inTab);
		if (CFStringGetCString(textCFString, hostName, sizeof(hostName), kCFStringEncodingMacRoman))
		{
			DNR_Result		lookupAttemptResult = kDNR_ResultOK;
			
			
			lookupAttemptResult = DNR_New(hostName, false/* use IP version 4 addresses (defaults to IPv6) */);
			if (false == lookupAttemptResult.ok())
			{
				// could not even initiate, so restore UI
				//lookupDoneUI(inTab);
			}
			else
			{
				// okay so far...
				result = true;
			}
		}
	}
	
	return result;
}// lookupHostName


/*!
Changes the theme font of all bevel button controls
in the specified window to use the system font
(which automatically sets the font size).  By default
a NIB makes bevel buttons use a small font, so this
routine corrects that.

(3.0)
*/
void
makeAllBevelButtonsUseTheSystemFont		(HIWindowRef	inWindow)
{
	OSStatus	error = noErr;
	HIViewRef	contentView = nullptr;
	
	
	error = HIViewFindByID(HIViewGetRoot(inWindow), kHIViewWindowContentID, &contentView);
	if (error == noErr)
	{
		ControlKind		kindInfo;
		HIViewRef		button = HIViewGetFirstSubview(contentView);
		
		
		while (nullptr != button)
		{
			error = GetControlKind(button, &kindInfo);
			if (noErr == error)
			{
				if ((kControlKindSignatureApple == kindInfo.signature) &&
					(kControlKindBevelButton == kindInfo.kind))
				{
					// only change bevel buttons
					Localization_SetControlThemeFontInfo(button, kThemeSystemFont);
				}
			}
			button = HIViewGetNextView(button);
		}
	}
}// makeAllButtonsUseTheSystemFont


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.1)
*/
SInt32
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessions, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																		My_SessionsPanelDataPtr);
			WindowRef const*			windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
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
			
			
			if ((0 != gIdealPanelWidth) && (0 != gIdealPanelHeight))
			{
				newLimits.width = gIdealPanelWidth;
				newLimits.height = gIdealPanelHeight;
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
for the buttons in the Resource tab.

(3.1)
*/
pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMySessionsTabResourcePtr)
{
	OSStatus					result = eventNotHandledErr;
	My_SessionsTabResource*		ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsTabResource*);
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
			case kCommandLookUpSelectedHostName:
				if (lookupHostName(*ptr))
				{
					result = noErr;
				}
				else
				{
					Sound_StandardAlert();
					result = resNotFound;
				}
				break;
			
			case kCommandShowCommandLine:
				// this normally means “show command line floater”, but in the context
				// of an active New Session sheet, it means “select command line field”
				{
					HIWindowRef		window = HIViewGetWindow(*ptr);
					
					
					(OSStatus)DialogUtilities_SetKeyboardFocus(HIViewWrap(idMyFieldCommandLine, window));
					result = noErr;
				}
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
Handles "kEventNetEvents_HostLookupComplete" of
"kEventClassNetEvents_DNS" by updating the text
field containing the remote host name.

(3.1)
*/
pascal OSStatus
receiveLookupComplete	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inMySessionsTabResourcePtr)
{
	OSStatus					result = eventNotHandledErr;
	My_SessionsTabResource*		ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsTabResource*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassNetEvents_DNS);
	assert(kEventKind == kEventNetEvents_HostLookupComplete);
	{
		struct hostent*		lookupDataPtr = nullptr;
		
		
		// find the lookup results
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_DirectHostEnt,
														typeNetEvents_StructHostEntPtr, lookupDataPtr);
		if (noErr == result)
		{
			// NOTE: The lookup data could be a linked list of many matches.
			// The first is used arbitrarily.
			if ((nullptr != lookupDataPtr->h_addr_list) && (nullptr != lookupDataPtr->h_addr_list[0]))
			{
				CFStringRef		addressCFString = DNR_CopyResolvedHostAsCFString(lookupDataPtr, 0/* which address */);
				
				
				if (nullptr != addressCFString)
				{
					HIViewWrap		fieldHostName(idMyFieldHostName, GetControlOwner(*ptr));
					
					
					assert(fieldHostName.exists());
					SetControlTextWithCFString(fieldHostName, addressCFString);
					CFRelease(addressCFString);
					(OSStatus)HIViewSetNeedsDisplay(fieldHostName, true);
					result = noErr;
				}
			}
			DNR_Dispose(&lookupDataPtr);
		}
	}
	
	//lookupDoneUI(ptr);
	
	return result;
}// receiveLookupComplete


/*!
Handles "kEventControlClick" of "kEventClassControl"
for this preferences panel.  Responds by changing
the currently selected tab, etc.

(3.1)
*/
pascal OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMySessionsPanelUIPtr)
{
	OSStatus				result = eventNotHandledErr;
	My_SessionsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMySessionsPanelUIPtr, My_SessionsPanelUIPtr);
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
Sizes and arranges views in the currently-selected
tab pane of the specified “Sessions” panel to fit
the dimensions of the panel’s container.

(3.0)
*/
void
showTabPane		(My_SessionsPanelUIPtr	inUIPtr,
				 UInt16					inTabIndex)
{
	HIViewRef	selectedTabPane = nullptr;
	
	
	if (kMy_TabIndexSessionHost == inTabIndex)
	{
		selectedTabPane = inUIPtr->resourceTab;
		assert_noerr(HIViewSetVisible(inUIPtr->dataFlowTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->controlKeysTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->vectorGraphicsTab, false/* visible */));
	}
	else if (kMy_TabIndexSessionDataFlow == inTabIndex)
	{
		selectedTabPane = inUIPtr->dataFlowTab;
		assert_noerr(HIViewSetVisible(inUIPtr->resourceTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->controlKeysTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->vectorGraphicsTab, false/* visible */));
	}
	else if (kMy_TabIndexSessionControlKeys == inTabIndex)
	{
		selectedTabPane = inUIPtr->controlKeysTab;
		assert_noerr(HIViewSetVisible(inUIPtr->resourceTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->dataFlowTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->vectorGraphicsTab, false/* visible */));
	}
	else if (kMy_TabIndexSessionVectorGraphics == inTabIndex)
	{
		selectedTabPane = inUIPtr->vectorGraphicsTab;
		assert_noerr(HIViewSetVisible(inUIPtr->resourceTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->dataFlowTab, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->controlKeysTab, false/* visible */));
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
