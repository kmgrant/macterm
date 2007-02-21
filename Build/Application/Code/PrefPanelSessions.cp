/*###############################################################

	PrefPanelSessions.cp
	
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

#define NUMBER_OF_SESSIONS_TABPANES		4
enum
{
	// must match tab order at creation, and be one-based
	kTabIndexSessionHost = 1,
	kTabIndexSessionDataFlow = 2,
	kTabIndexSessionControlKeys = 3,
	kTabIndexSessionVectorGraphics = 4
};

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelSessions.nib".

In addition, they MUST be unique across all panels.
*/
static HIViewID const	idMyFieldHostName		= { FOUR_CHAR_CODE('Host'), 0/* ID */ };

#pragma mark Types

/*!
Implements the “Resource” tab.
*/
struct MySessionsTabResource
{
	MySessionsTabResource	(HIWindowRef, HIViewRef);
	
	HIViewWrap				pane;

protected:
	HIViewWrap
	createContainerView		(HIWindowRef) const;
	
private:
	CarbonEventHandlerWrap		buttonCommandsHandler;		//!< invoked when a button is clicked
	CarbonEventHandlerWrap		whenLookupCompleteHandler;	//!< invoked when a DNS query finally returns
};

/*!
Implements the “Data Flow” tab.
*/
struct MySessionsTabDataFlow
{
	MySessionsTabDataFlow	(HIWindowRef, HIViewRef);
	
	HIViewWrap				pane;

protected:
	HIViewWrap
	createContainerView		(HIWindowRef) const;
};

/*!
Implements the “Control Keys” tab.
*/
struct MySessionsTabControlKeys
{
	MySessionsTabControlKeys	(HIWindowRef, HIViewRef);
	
	HIViewWrap				pane;

protected:
	HIViewWrap
	createContainerView		(HIWindowRef) const;
};

/*!
Implements the “TEK” tab.
*/
struct MySessionsTabVectorGraphics
{
	MySessionsTabVectorGraphics	(HIWindowRef, HIViewRef);
	
	HIViewWrap				pane;

protected:
	HIViewWrap
	createContainerView		(HIWindowRef) const;
};

/*!
Implements the entire panel user interface.
*/
struct MySessionsPanelUI
{
	MySessionsPanelUI	(HIWindowRef, HIViewRef);
	
	CommonEventHandlers_HIViewResizer	containerResizer;	//!< invoked when the panel is resized
	HIViewWrap							tabView;
	CarbonEventHandlerWrap				viewClickHandler;	//!< invoked when a tab is clicked
	
	MySessionsTabResource				resourceTab;
	MySessionsTabDataFlow				dataFlowTab;
	MySessionsTabControlKeys			controlKeysTab;
	MySessionsTabVectorGraphics			vectorGraphicsTab;
	
protected:
	HIViewWrap
	createTabsView	(HIWindowRef) const;
};
typedef MySessionsPanelUI*	MySessionsPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct MySessionsPanelData
{
	MySessionsPanelData ();
	
	Panel_Ref			panel;			//!< the panel this data is for
	MySessionsPanelUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef MySessionsPanelData*		MySessionsPanelDataPtr;

#pragma mark Internal Method Prototypes

static void				createPanelViews				(Panel_Ref, WindowRef);
static void				deltaSizePanelContainerHIView	(HIViewRef, Float32, Float32, void*);
static void				disposePanel					(Panel_Ref, void*);
static Boolean			lookupHostName					(MySessionsTabResource*);
static SInt32			panelChanged					(Panel_Ref, Panel_Message, void*);
static pascal OSStatus	receiveHICommand				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveLookupComplete			(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus	receiveViewHit					(EventHandlerCallRef, EventRef, void*);
static void				setUpMainTabs					(MySessionsPanelUIPtr);
static void				showTabPane						(MySessionsPanelUIPtr, UInt16);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Float32		gIdealWidth = 0.0;
	Float32		gIdealHeight = 0.0;
}



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
		MySessionsPanelDataPtr	dataPtr = new MySessionsPanelData();
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessions);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessions);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowSessionsCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										kConstantsRegistry_ApplicationCreatorSignature,
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Initializes a MySessionsPanelData structure.

(3.1)
*/
MySessionsPanelData::
MySessionsPanelData ()
:
panel(nullptr),
interfacePtr(nullptr)
{
}// MySessionsPanelData default constructor


/*!
Initializes a MySessionsPanelUI structure.

(3.1)
*/
MySessionsPanelUI::
MySessionsPanelUI	(HIWindowRef	inOwningWindow,
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
resourceTab			(inOwningWindow, this->tabView),
dataFlowTab			(inOwningWindow, this->tabView),
controlKeysTab		(inOwningWindow, this->tabView),
vectorGraphicsTab	(inOwningWindow, this->tabView)
{
	assert(tabView.exists());
	assert(viewClickHandler.isInstalled());
}// MySessionsPanelUI 2-argument constructor


/*!
Constructs the tab container for the panel.

(3.1)
*/
HIViewWrap
MySessionsPanelUI::
createTabsView	(HIWindowRef	inOwningWindow)
const
{
	HIViewRef				result = nullptr;
	Rect					containerBounds;
	ControlTabEntry			tabInfo[NUMBER_OF_SESSIONS_TABPANES];
	UIStrings_ResultCode	stringResult = kUIStrings_ResultCodeSuccess;
	OSStatus				error = noErr;
	
	
	// nullify or zero-fill everything, then set only what matters
	bzero(&tabInfo, sizeof(tabInfo));
	tabInfo[0].enabled =
		tabInfo[1].enabled =
		tabInfo[2].enabled =
		tabInfo[3].enabled = true;
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsHostTabName,
									tabInfo[kTabIndexSessionHost - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsDataFlowTabName,
									tabInfo[kTabIndexSessionDataFlow - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsControlKeysTabName,
									tabInfo[kTabIndexSessionControlKeys - 1].name);
	stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowSessionsVectorGraphicsTabName,
									tabInfo[kTabIndexSessionVectorGraphics - 1].name);
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
}// createTabsView


/*!
Initializes a MySessionsTabControlKeys structure.

(3.1)
*/
MySessionsTabControlKeys::
MySessionsTabControlKeys	(HIWindowRef	inOwningWindow,
							 HIViewRef		inTabsView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
pane	(createContainerView(inOwningWindow)
			<< HIViewWrap_AssertExists
			<< HIViewWrap_EmbedIn(inTabsView))
{
	assert(this->pane.exists());
}// MySessionsTabControlKeys 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
MySessionsTabControlKeys::
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
			(CFSTR("PrefPanelSessions"), CFSTR("ControlKeys"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// this tab has extra buttons because it is also used for a sheet;
	// hide the extra buttons
	HIViewWrap		okayButton(HIViewIDWrap('Okay', 0), inOwningWindow);
	HIViewWrap		cancelButton(HIViewIDWrap('Canc', 0), inOwningWindow);
	HIViewWrap		helpButton(HIViewIDWrap('Help', 0), inOwningWindow);
	okayButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	cancelButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	if (helpButton.exists()) helpButton << HIViewWrap_SetVisibleState(false);
	
	return result;
}// createContainerView


/*!
Initializes a MySessionsTabDataFlow structure.

(3.1)
*/
MySessionsTabDataFlow::
MySessionsTabDataFlow	(HIWindowRef	inOwningWindow,
						 HIViewRef		inTabsView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
pane	(createContainerView(inOwningWindow)
			<< HIViewWrap_AssertExists
			<< HIViewWrap_EmbedIn(inTabsView))
{
	assert(this->pane.exists());
}// MySessionsTabDataFlow 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
MySessionsTabDataFlow::
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
			(CFSTR("PrefPanelSessions"), CFSTR("DataFlow"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	return result;
}// createContainerView


/*!
Initializes a MySessionsTabResource structure.

(3.1)
*/
MySessionsTabResource::
MySessionsTabResource	(HIWindowRef	inOwningWindow,
						 HIViewRef		inTabsView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
pane						(createContainerView(inOwningWindow)
								<< HIViewWrap_AssertExists
								<< HIViewWrap_EmbedIn(inTabsView)),
buttonCommandsHandler		(GetWindowEventTarget(inOwningWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this/* user data */),
whenLookupCompleteHandler	(GetApplicationEventTarget(), receiveLookupComplete,
								CarbonEventSetInClass(CarbonEventClass(kEventClassNetEvents_DNS),
														kEventNetEvents_HostLookupComplete),
								this/* user data */)
{
	assert(this->pane.exists());
}// MySessionsTabResource 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
MySessionsTabResource::
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
			(CFSTR("PrefPanelSessions"), CFSTR("Resource"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// this tab has extra buttons because it is also used for a sheet;
	// hide the extra buttons
	HIViewWrap		goButton(HIViewIDWrap('GoDo', 0), inOwningWindow);
	HIViewWrap		cancelButton(HIViewIDWrap('Canc', 0), inOwningWindow);
	HIViewWrap		helpButton(HIViewIDWrap('Help', 0), inOwningWindow);
	goButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	cancelButton << HIViewWrap_AssertExists << HIViewWrap_SetVisibleState(false);
	if (helpButton.exists()) helpButton << HIViewWrap_SetVisibleState(false);
	
	return result;
}// createContainerView


/*!
Initializes a MySessionsTabVectorGraphics structure.

(3.1)
*/
MySessionsTabVectorGraphics::
MySessionsTabVectorGraphics	(HIWindowRef	inOwningWindow,
							 HIViewRef		inTabsView)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
pane	(createContainerView(inOwningWindow)
			<< HIViewWrap_AssertExists
			<< HIViewWrap_EmbedIn(inTabsView))
{
	assert(this->pane.exists());
}// MySessionsTabVectorGraphics 1-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
MySessionsTabVectorGraphics::
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
			(CFSTR("PrefPanelSessions"), CFSTR("TEK"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	return result;
}// createContainerView


/*!
Creates the views that belong in the “Sessions” panel, and
attaches them to the specified owning window in the proper
embedding hierarchy.  The specified panel’s descriptor must
be "kMyPrefPanelDescriptorSessions".

The views are not arranged or sized.

(3.1)
*/
static void
createPanelViews	(Panel_Ref		inPanel,
					 WindowRef		inOwningWindow)
{
	HIViewRef				container = nullptr;
	MySessionsPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel), MySessionsPanelDataPtr);
	MySessionsPanelUIPtr	interfacePtr = nullptr;
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
	dataPtr->interfacePtr = new MySessionsPanelUI(inOwningWindow, container);
	assert(nullptr != dataPtr->interfacePtr);
	interfacePtr = dataPtr->interfacePtr;
	
	// calculate the largest area required for all tabs, and keep this as the ideal size
	{
		NIBWindow	hostTab(AppResources_ReturnBundleForNIBs(), CFSTR("PrefPanelSessions"), CFSTR("Resource"));
		NIBWindow	dataFlowTab(AppResources_ReturnBundleForNIBs(), CFSTR("PrefPanelSessions"), CFSTR("DataFlow"));
		NIBWindow	controlKeysTab(AppResources_ReturnBundleForNIBs(), CFSTR("PrefPanelSessions"), CFSTR("ControlKeys"));
		NIBWindow	vectorGraphicsTab(AppResources_ReturnBundleForNIBs(), CFSTR("PrefPanelSessions"), CFSTR("TEK"));
		Rect		hostTabSize;
		Rect		dataFlowTabSize;
		Rect		controlKeysTabSize;
		Rect		vectorGraphicsTabSize;
		Point		tabFrameTopLeft;
		Point		tabFrameWidthHeight;
		Point		tabPaneTopLeft;
		Point		tabPaneBottomRight;
		
		
		// determine sizes of tabs from NIBs
		hostTab << NIBLoader_AssertWindowExists;
		dataFlowTab << NIBLoader_AssertWindowExists;
		controlKeysTab << NIBLoader_AssertWindowExists;
		vectorGraphicsTab << NIBLoader_AssertWindowExists;
		GetWindowBounds(hostTab, kWindowContentRgn, &hostTabSize);
		GetWindowBounds(dataFlowTab, kWindowContentRgn, &dataFlowTabSize);
		GetWindowBounds(controlKeysTab, kWindowContentRgn, &controlKeysTabSize);
		GetWindowBounds(vectorGraphicsTab, kWindowContentRgn, &vectorGraphicsTabSize);
		
		// also include pane margin in panel size
		Panel_CalculateTabFrame(container, &tabFrameTopLeft, &tabFrameWidthHeight);
		Panel_GetTabPaneInsets(&tabPaneTopLeft, &tabPaneBottomRight);
		
		// find the widest tab and the highest tab
		gIdealWidth = std::max(hostTabSize.right - hostTabSize.left,
								dataFlowTabSize.right - dataFlowTabSize.left);
		gIdealWidth = std::max(controlKeysTabSize.right - controlKeysTabSize.left,
								STATIC_CAST(gIdealWidth, int));
		gIdealWidth = std::max(vectorGraphicsTabSize.right - vectorGraphicsTabSize.left,
								STATIC_CAST(gIdealWidth, int));
		gIdealWidth += (tabFrameTopLeft.h + tabPaneTopLeft.h + tabPaneBottomRight.h + tabFrameTopLeft.h/* same as left */);
		gIdealHeight = std::max(hostTabSize.bottom - hostTabSize.top,
								dataFlowTabSize.bottom - dataFlowTabSize.top);
		gIdealHeight = std::max(controlKeysTabSize.bottom - controlKeysTabSize.top,
								STATIC_CAST(gIdealHeight, int));
		gIdealHeight = std::max(vectorGraphicsTabSize.bottom - vectorGraphicsTabSize.top,
								STATIC_CAST(gIdealHeight, int));
		gIdealHeight += (tabFrameTopLeft.v + tabPaneTopLeft.v + tabPaneBottomRight.v + tabFrameTopLeft.v/* same as top */);
	}
}// createPanelViews


/*!
Adjusts the views in the “Sessions” preference panel
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
		//HIWindowRef				kPanelWindow = GetControlOwner(inView);
		MySessionsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inContext, MySessionsPanelUIPtr);
		assert(nullptr != interfacePtr);
		HIViewWrap				viewWrap;
		UInt16					chosenTabIndex = STATIC_CAST(GetControl32BitValue(interfacePtr->tabView), UInt16);
		
		
		setUpMainTabs(interfacePtr);
		//(viewWrap = dataPtr->tabView)
		//	<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
		//	;
		
		if (kTabIndexSessionHost == chosenTabIndex)
		{
			// resize the Host tab pane, and its views
			viewWrap.setCFTypeRef(interfacePtr->resourceTab.pane);
			viewWrap
				<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
				;
			// INCOMPLETE
		}
		else if (kTabIndexSessionDataFlow == chosenTabIndex)
		{
			// resize the Data Flow tab pane, and its views
			viewWrap.setCFTypeRef(interfacePtr->dataFlowTab.pane);
			viewWrap
				<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
				;
			// INCOMPLETE
		}
		else if (kTabIndexSessionControlKeys == chosenTabIndex)
		{
			// resize the Control Keys tab pane, and its views
			viewWrap.setCFTypeRef(interfacePtr->controlKeysTab.pane);
			viewWrap
				<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
				;
			// INCOMPLETE
		}
		else if (kTabIndexSessionVectorGraphics == chosenTabIndex)
		{
			// resize the TEK tab pane, and its views
			viewWrap.setCFTypeRef(interfacePtr->vectorGraphicsTab.pane);
			viewWrap
				<< HIViewWrap_DeltaSize(inDeltaX, inDeltaY)
				;
			// INCOMPLETE
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
	MySessionsPanelDataPtr	dataPtr = REINTERPRET_CAST(inDataPtr, MySessionsPanelDataPtr);
	
	
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
static Boolean
lookupHostName		(MySessionsTabResource*		inPtr)
{
	CFStringRef		textCFString = nullptr;
	HIViewWrap		fieldHostName(idMyFieldHostName, GetControlOwner(inPtr->pane));
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
		
		
		//lookupWaitUI(inPtr);
		if (CFStringGetCString(textCFString, hostName, sizeof(hostName), kCFStringEncodingMacRoman))
		{
			DNR_ResultCode		lookupAttemptResult = kDNR_ResultCodeSuccess;
			
			
			lookupAttemptResult = DNR_New(hostName, false/* use IP version 4 addresses (defaults to IPv6) */);
			if (false == lookupAttemptResult.ok())
			{
				// could not even initiate, so restore UI
				//lookupDoneUI(inPtr);
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
												kConstantsRegistry_PrefPanelDescriptorSessions, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			MySessionsPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	MySessionsPanelDataPtr);
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
for the buttons in the Resource tab.

(3.1)
*/
static pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMySessionsTabResourcePtr)
{
	OSStatus				result = eventNotHandledErr;
	MySessionsTabResource*	ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, MySessionsTabResource*);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
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
				if (lookupHostName(ptr))
				{
					result = noErr;
				}
				else
				{
					Sound_StandardAlert();
					result = resNotFound;
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
static pascal OSStatus
receiveLookupComplete	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inMySessionsTabResourcePtr)
{
	OSStatus				result = eventNotHandledErr;
	MySessionsTabResource*	ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, MySessionsTabResource*);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
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
					HIViewWrap		fieldHostName(idMyFieldHostName, GetControlOwner(ptr->pane));
					
					
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
static pascal OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMySessionsPanelUIPtr)
{
	OSStatus				result = eventNotHandledErr;
	MySessionsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMySessionsPanelUIPtr, MySessionsPanelUIPtr);
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
Sizes and arranges the main tab view.

(3.1)
*/
static void
setUpMainTabs	(MySessionsPanelUIPtr	inInterfacePtr)
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
tab pane of the specified “Sessions” panel to fit
the dimensions of the panel’s container.

(3.0)
*/
static void
showTabPane		(MySessionsPanelUIPtr	inUIPtr,
				 UInt16					inTabIndex)
{
	HIViewRef	selectedTabPane = nullptr;
	HIRect		newFrame;
	
	
	if (kTabIndexSessionHost == inTabIndex)
	{
		selectedTabPane = inUIPtr->resourceTab.pane;
		assert_noerr(HIViewSetVisible(inUIPtr->dataFlowTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->controlKeysTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->vectorGraphicsTab.pane, false/* visible */));
	}
	else if (kTabIndexSessionDataFlow == inTabIndex)
	{
		selectedTabPane = inUIPtr->dataFlowTab.pane;
		assert_noerr(HIViewSetVisible(inUIPtr->resourceTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->controlKeysTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->vectorGraphicsTab.pane, false/* visible */));
	}
	else if (kTabIndexSessionControlKeys == inTabIndex)
	{
		selectedTabPane = inUIPtr->controlKeysTab.pane;
		assert_noerr(HIViewSetVisible(inUIPtr->resourceTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->dataFlowTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->vectorGraphicsTab.pane, false/* visible */));
	}
	else if (kTabIndexSessionVectorGraphics == inTabIndex)
	{
		selectedTabPane = inUIPtr->vectorGraphicsTab.pane;
		assert_noerr(HIViewSetVisible(inUIPtr->resourceTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->dataFlowTab.pane, false/* visible */));
		assert_noerr(HIViewSetVisible(inUIPtr->controlKeysTab.pane, false/* visible */));
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
