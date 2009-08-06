/*###############################################################

	PrefPanelSessions.cp
	
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
#include "GeneralResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "DNR.h"
#include "GenericPanelTabs.h"
#include "NetEvents.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelSessions.h"
#include "ServerBrowser.h"
#include "Session.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"
#include "VectorInterpreter.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelSessions.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyFieldCommandLine			= { 'CmdL', 0/* ID */ };
HIViewID const	idMyButtonConnectToServer		= { 'CtoS', 0/* ID */ };
HIViewID const	idMyHelpTextConnectToServer		= { 'ConH', 0/* ID */ };
HIViewID const	idMyPopUpMenuTerminal			= { 'Term', 0/* ID */ };
HIViewID const	idMyPopUpMenuFormat				= { 'Frmt', 0/* ID */ };
HIViewID const	idMyPopUpMenuTranslation		= { 'Xlat', 0/* ID */ };
HIViewID const	idMyHelpTextPresets				= { 'THlp', 0/* ID */ };
HIViewID const	idMySliderScrollSpeed			= { 'SSpd', 0/* ID */ };
HIViewID const	idMyStaticTextCaptureFilePath	= { 'CapP', 0/* ID */ };
HIViewID const	idMyButtonNoCaptureFile			= { 'NoCF', 0/* ID */ };
HIViewID const	idMyButtonChooseCaptureFile		= { 'CapF', 0/* ID */ };
HIViewID const	idMyButtonChangeInterruptKey	= { 'Intr', 0/* ID */ };
HIViewID const	idMyButtonChangeSuspendKey		= { 'Susp', 0/* ID */ };
HIViewID const	idMyButtonChangeResumeKey		= { 'Resu', 0/* ID */ };
HIViewID const	idMyHelpTextControlKeys			= { 'CtlH', 0/* ID */ };
HIViewID const	idMySeparatorKeyboardOptions	= { 'KSep', 0/* ID */ };
HIViewID const	idMyRadioButtonTEKDisabled		= { 'RTNo', 0/* ID */ };
HIViewID const	idMyRadioButtonTEK4014			= { '4014', 0/* ID */ };
HIViewID const	idMyRadioButtonTEK4105			= { '4105', 0/* ID */ };
HIViewID const	idMyCheckBoxTEKPageClearsScreen	= { 'XPCS', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Implements the “Data Flow” tab.
*/
struct My_SessionsPanelDataFlowUI
{
	My_SessionsPanelDataFlowUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	CommonEventHandlers_HIViewResizer	_containerResizer;
};

/*!
Implements the “Vector Graphics” tab.
*/
struct My_SessionsPanelGraphicsUI
{
	My_SessionsPanelGraphicsUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	void
	setMode		(VectorInterpreter_Mode);
	
	void
	setPageClears	(Boolean);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	CommonEventHandlers_HIViewResizer	_containerResizer;
};

/*!
Implements the “Keyboard” tab.
*/
struct My_SessionsPanelKeyboardUI
{
	My_SessionsPanelKeyboardUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	CommonEventHandlers_HIViewResizer	_containerResizer;
};

/*!
Implements the “Resource” tab.
*/
struct My_SessionsPanelResourceUI
{
	My_SessionsPanelResourceUI	(Panel_Ref, HIWindowRef);
	My_SessionsPanelResourceUI	();
	
	Panel_Ref			panel;				//!< the panel using this UI
	Float32				idealWidth;			//!< best size in pixels
	Float32				idealHeight;		//!< best size in pixels
	HIViewWrap			mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	UInt16
	readPreferencesForRemoteServers		(Preferences_ContextRef, Boolean, Session_Protocol&,
										 CFStringRef&, UInt16&, CFStringRef&);
	
	void
	rebuildFavoritesMenu	(HIViewID const&, UInt32, Quills::Prefs::Class,
							 UInt32, MenuItemIndex&);
	
	void
	rebuildFormatMenu ();
	
	void
	rebuildTerminalMenu ();
	
	void
	rebuildTranslationMenu ();
	
	void
	saveFieldPreferences	(Preferences_ContextRef);
	
	UInt16
	savePreferencesForRemoteServers		(Preferences_ContextRef, Session_Protocol,
										 CFStringRef, UInt16, CFStringRef);
	
	void
	setAssociatedFormat		(CFStringRef);
	
	void
	setAssociatedTerminal	(CFStringRef);
	
	void
	setAssociatedTranslation	(CFStringRef);
	
	void
	setCommandLine	(CFStringRef);
	
	Boolean
	updateCommandLine	(Session_Protocol, CFStringRef, UInt16, CFStringRef);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	MenuItemIndex						_numberOfFormatItemsAdded;		//!< used to manage Format pop-up menu
	MenuItemIndex						_numberOfTerminalItemsAdded;	//!< used to manage Terminal pop-up menu
	MenuItemIndex						_numberOfTranslationItemsAdded;	//!< used to manage Translation pop-up menu
	HIViewWrap							_fieldCommandLine;				//!< text of Unix command line to run
	CommonEventHandlers_HIViewResizer	_containerResizer;
	CarbonEventHandlerWrap				_buttonCommandsHandler;			//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_whenCommandLineChangedHandler;	//!< invoked when the command line field changes
	CarbonEventHandlerWrap				_whenServerPanelChangedHandler;	//!< invoked when the server browser panel changes
	ListenerModel_ListenerRef			_whenFavoritesChangedHandler;	//!< used to manage Terminal and Format pop-up menus
};

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelDataFlowData
{
	My_SessionsPanelDataFlowData	();
	~My_SessionsPanelDataFlowData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelDataFlowUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelDataFlowData*		My_SessionsPanelDataFlowDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelGraphicsData
{
	My_SessionsPanelGraphicsData	();
	~My_SessionsPanelGraphicsData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelGraphicsUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelGraphicsData*		My_SessionsPanelGraphicsDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelKeyboardData
{
	My_SessionsPanelKeyboardData	();
	~My_SessionsPanelKeyboardData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelKeyboardUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelKeyboardData*		My_SessionsPanelKeyboardDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_SessionsPanelResourceData
{
	My_SessionsPanelResourceData	();
	~My_SessionsPanelResourceData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_SessionsPanelResourceUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_SessionsPanelResourceData*		My_SessionsPanelResourceDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void				makeAllBevelButtonsUseTheSystemFont		(HIWindowRef);
void				preferenceChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
pascal OSStatus		receiveFieldChangedInCommandLine		(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveHICommand						(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveServerBrowserEvent				(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new preference panel for the Sessions category.
Destroy it using Panel_Dispose().

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelSessions_New ()
{
	Panel_Ref				result = nullptr;
	CFStringRef				nameCFString = nullptr;
	GenericPanelTabs_List	tabList;
	
	
	tabList.push_back(PrefPanelSessions_NewResourcePane());
	tabList.push_back(PrefPanelSessions_NewDataFlowPane());
	tabList.push_back(PrefPanelSessions_NewKeyboardPane());
	tabList.push_back(PrefPanelSessions_NewGraphicsPane());
	
	if (UIStrings_Copy(kUIStrings_PreferencesWindowSessionsCategoryName, nameCFString).ok())
	{
		result = GenericPanelTabs_New(nameCFString, kConstantsRegistry_PrefPanelDescriptorSessions, tabList);
		if (nullptr != result)
		{
			Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessions);
			Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
											AppResources_ReturnCreatorCode(),
											kConstantsRegistry_IconServicesIconPrefPanelSessions);
		}
		CFRelease(nameCFString), nameCFString = nullptr;
	}
	
	return result;
}// New


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewDataFlowPane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelDataFlowUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelDataFlowDataPtr		dataPtr = new My_SessionsPanelDataFlowData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionDataFlow);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsDataFlow);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowSessionsDataFlowTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewDataFlowPane


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewGraphicsPane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelGraphicsUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelGraphicsDataPtr		dataPtr = new My_SessionsPanelGraphicsData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionGraphics);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsGraphics);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowSessionsGraphicsTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewGraphicsPane


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewKeyboardPane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelKeyboardUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelKeyboardDataPtr		dataPtr = new My_SessionsPanelKeyboardData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionKeyboard);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsKeyboard);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowSessionsKeyboardTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewKeyboardPane


/*!
Creates only the Data Flow pane, which allows the user to
set things like cache sizes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelSessions_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelSessions_NewResourcePane ()
{
	Panel_Ref	result = Panel_New(My_SessionsPanelResourceUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_SessionsPanelResourceDataPtr		dataPtr = new My_SessionsPanelResourceData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorSessionResource);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelSessionsResource);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowSessionsResourceTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelSessionsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelSessions);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewResourcePane


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_SessionsPanelDataFlowData structure.

(3.1)
*/
My_SessionsPanelDataFlowData::
My_SessionsPanelDataFlowData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelDataFlowData default constructor


/*!
Tears down a My_SessionsPanelDataFlowData structure.

(3.1)
*/
My_SessionsPanelDataFlowData::
~My_SessionsPanelDataFlowData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelDataFlowData destructor


/*!
Initializes a My_SessionsPanelDataFlowUI structure.

(3.1)
*/
My_SessionsPanelDataFlowUI::
My_SessionsPanelDataFlowUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_SessionsPanelDataFlowUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
}// My_SessionsPanelDataFlowUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelDataFlowUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("DataFlow"), inOwningWindow,
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
	
	// initialize values
	// UNIMPLEMENTED
	
	return result;
}// My_SessionsPanelDataFlowUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsPanelDataFlowUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_SessionsTabDataFlow*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabDataFlow*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyStaticTextCaptureFilePath, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyButtonNoCaptureFile, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyButtonChooseCaptureFile, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
}// My_SessionsPanelDataFlowUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelDataFlowUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionDataFlow, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelDataFlowUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_SessionsPanelDataFlowDataPtr));
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
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
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
			My_SessionsPanelDataFlowDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelDataFlowDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext) panelDataPtr->interfacePtr->readPreferences(defaultContext); // reset to known state first
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
}// My_SessionsPanelDataFlowUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelDataFlowUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// UNIMPLEMENTED
	}
}// My_SessionsPanelDataFlowUI::readPreferences


/*!
Initializes a My_SessionsPanelGraphicsData structure.

(3.1)
*/
My_SessionsPanelGraphicsData::
My_SessionsPanelGraphicsData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelGraphicsData default constructor


/*!
Tears down a My_SessionsPanelGraphicsData structure.

(3.1)
*/
My_SessionsPanelGraphicsData::
~My_SessionsPanelGraphicsData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelGraphicsData destructor


/*!
Initializes a My_SessionsPanelGraphicsUI structure.

(3.1)
*/
My_SessionsPanelGraphicsUI::
My_SessionsPanelGraphicsUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_SessionsPanelGraphicsUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
}// My_SessionsPanelGraphicsUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelGraphicsUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("TEK"), inOwningWindow,
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
	
	// initialize values
	// UNIMPLEMENTED
	
	return result;
}// My_SessionsPanelGraphicsUI::createContainerView


/*!
Resizes the views in this panel.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		UNUSED_ARGUMENT(inDeltaX),
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_SessionsPanelGraphicsUI*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsPanelGraphicsUI*);
	
	
	// UNIMPLEMENTED
}// My_SessionsPanelGraphicsUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelGraphicsUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionGraphics, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelGraphicsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelGraphicsDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelGraphicsUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_SessionsPanelGraphicsDataPtr));
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
			My_SessionsPanelGraphicsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelGraphicsDataPtr);
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
			My_SessionsPanelGraphicsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelGraphicsDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext) panelDataPtr->interfacePtr->readPreferences(defaultContext); // reset to known state first
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
}// My_SessionsPanelGraphicsUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// set TEK mode
		{
			VectorInterpreter_Mode		graphicsMode = kVectorInterpreter_ModeDisabled;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTektronixMode, sizeof(graphicsMode),
														&graphicsMode, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setMode(graphicsMode);
			}
		}
		
		// set TEK PAGE checkbox
		{
			Boolean		pageClearsScreen = false;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTektronixPAGEClearsScreen, sizeof(pageClearsScreen),
														&pageClearsScreen, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setPageClears(pageClearsScreen);
			}
		}
	}
}// My_SessionsPanelGraphicsUI::readPreferences


/*!
Updates the TEK clear checkbox based on the given setting.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
setPageClears	(Boolean	inTEKPageClearsScreen)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyCheckBoxTEKPageClearsScreen, kOwningWindow),
							BooleanToCheckBoxValue(inTEKPageClearsScreen));
}// My_SessionsPanelGraphicsUI::setPageClears


/*!
Updates the TEK graphics mode display based on the
given setting.

(3.1)
*/
void
My_SessionsPanelGraphicsUI::
setMode		(VectorInterpreter_Mode		inMode)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControl32BitValue(HIViewWrap(idMyRadioButtonTEKDisabled, kOwningWindow),
							BooleanToRadioButtonValue(kVectorInterpreter_ModeDisabled == inMode));
	SetControl32BitValue(HIViewWrap(idMyRadioButtonTEK4014, kOwningWindow),
							BooleanToRadioButtonValue(kVectorInterpreter_ModeTEK4014 == inMode));
	SetControl32BitValue(HIViewWrap(idMyRadioButtonTEK4105, kOwningWindow),
							BooleanToRadioButtonValue(kVectorInterpreter_ModeTEK4105 == inMode));
}// My_SessionsPanelGraphicsUI::setMode


/*!
Initializes a My_SessionsPanelKeyboardData structure.

(3.1)
*/
My_SessionsPanelKeyboardData::
My_SessionsPanelKeyboardData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelKeyboardData default constructor


/*!
Tears down a My_SessionsPanelKeyboardData structure.

(3.1)
*/
My_SessionsPanelKeyboardData::
~My_SessionsPanelKeyboardData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelKeyboardData destructor


/*!
Initializes a My_SessionsPanelKeyboardUI structure.

(3.1)
*/
My_SessionsPanelKeyboardUI::
My_SessionsPanelKeyboardUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_SessionsPanelKeyboardUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
}// My_SessionsPanelKeyboardUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelKeyboardUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("Keyboard"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// calculate the ideal size
	this->idealWidth = idealContainerBounds.right - idealContainerBounds.left;
	this->idealHeight = idealContainerBounds.bottom - idealContainerBounds.top;
	
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
}// My_SessionsPanelKeyboardUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsPanelKeyboardUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const		kPanelWindow = HIViewGetWindow(inContainer);
	//My_SessionsTabControlKeys*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabControlKeys*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextControlKeys, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMySeparatorKeyboardOptions, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_SessionsPanelKeyboardUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelKeyboardUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionKeyboard, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelKeyboardDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelKeyboardDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelKeyboardUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_SessionsPanelKeyboardDataPtr));
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
			My_SessionsPanelKeyboardDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelKeyboardDataPtr);
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
			My_SessionsPanelKeyboardDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelKeyboardDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext) panelDataPtr->interfacePtr->readPreferences(defaultContext); // reset to known state first
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
}// My_SessionsPanelKeyboardUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelKeyboardUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// UNIMPLEMENTED
	}
}// My_SessionsPanelKeyboardUI::readPreferences


/*!
Initializes a My_SessionsPanelResourceData structure.

(3.1)
*/
My_SessionsPanelResourceData::
My_SessionsPanelResourceData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_SessionsPanelResourceData default constructor


/*!
Tears down a My_SessionsPanelResourceData structure.

(3.1)
*/
My_SessionsPanelResourceData::
~My_SessionsPanelResourceData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_SessionsPanelResourceData destructor


/*!
Initializes a My_SessionsPanelResourceUI structure.

(3.1)
*/
My_SessionsPanelResourceUI::
My_SessionsPanelResourceUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel							(inPanel),
idealWidth						(0.0),
idealHeight						(0.0),
mainView						(createContainerView(inPanel, inOwningWindow)
									<< HIViewWrap_AssertExists),
_numberOfFormatItemsAdded		(0),
_numberOfTerminalItemsAdded		(0),
_numberOfTranslationItemsAdded	(0),
_fieldCommandLine				(HIViewWrap(idMyFieldCommandLine, inOwningWindow)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(UnixCommandLineLimiter)),
_containerResizer				(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
									My_SessionsPanelResourceUI::deltaSize, this/* context */),
_buttonCommandsHandler			(GetWindowEventTarget(inOwningWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this/* user data */),
_whenCommandLineChangedHandler	(GetControlEventTarget(this->_fieldCommandLine), receiveFieldChangedInCommandLine,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this/* user data */),
_whenServerPanelChangedHandler	(GetWindowEventTarget(inOwningWindow), receiveServerBrowserEvent,
									CarbonEventSetInClass(CarbonEventClass(kEventClassNetEvents_ServerBrowser),
															kEventNetEvents_ServerBrowserNewData, kEventNetEvents_ServerBrowserNewEventTarget),
									this/* user data */),
_whenFavoritesChangedHandler	(ListenerModel_NewStandardListener(preferenceChanged, this/* context */))
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
	
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_StartMonitoring
					(this->_whenFavoritesChangedHandler, kPreferences_ChangeNumberOfContexts,
						true/* notify of initial value */);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_StartMonitoring
					(this->_whenFavoritesChangedHandler, kPreferences_ChangeContextName,
						true/* notify of initial value */);
	assert(kPreferences_ResultOK == prefsResult);
}// My_SessionsPanelResourceUI 2-argument constructor


/*!
Tears down a My_SessionsPanelResourceUI structure.

(3.1)
*/
My_SessionsPanelResourceUI::
My_SessionsPanelResourceUI ()
{
	Preferences_StopMonitoring(this->_whenFavoritesChangedHandler, kPreferences_ChangeNumberOfContexts);
	Preferences_StopMonitoring(this->_whenFavoritesChangedHandler, kPreferences_ChangeContextName);
	ListenerModel_ReleaseListener(&_whenFavoritesChangedHandler);
}// My_SessionsPanelResourceUI destructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_SessionsPanelResourceUI::
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
			(CFSTR("PrefPanelSessions"), CFSTR("Resource"), inOwningWindow,
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
	
	// initialize values
	// INCOMPLETE
	
	return result;
}// My_SessionsPanelResourceUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_SessionsPanelResourceUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			inContext)
{
	HIWindowRef const				kPanelWindow = HIViewGetWindow(inContainer);
	My_SessionsPanelResourceUI*		dataPtr = REINTERPRET_CAST(inContext, My_SessionsPanelResourceUI*);
	HIViewWrap						viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextConnectToServer, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyHelpTextPresets, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	dataPtr->_fieldCommandLine << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	// INCOMPLETE
}// My_SessionsPanelResourceUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_SessionsPanelResourceUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorSessionResource, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_SessionsPanelResourceUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(inDataPtr, My_SessionsPanelResourceDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
			ServerBrowser_SetVisible(false);
			delete panelDataPtr;
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
			//HIViewRef const*						viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
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
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_SessionsPanelResourceDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_Result					prefsResult = kPreferences_ResultOK;
			Preferences_ContextRef				defaultContext = nullptr;
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			// force the Servers panel to no longer be associated with this data,
			// by resetting it; this will automatically trigger appropriate UI
			// changes (namely, removing highlighting from the button that opened
			// the panel)
			ServerBrowser_RemoveEventTarget();
			
			if (nullptr != oldContext)
			{
				panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
				Preferences_ContextSave(oldContext);
			}
			prefsResult = Preferences_GetDefaultContext(&defaultContext, Quills::Prefs::SESSION);
			assert(kPreferences_ResultOK == prefsResult);
			if (newContext != defaultContext) panelDataPtr->interfacePtr->readPreferences(defaultContext); // reset to known state first
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
}// My_SessionsPanelResourceUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_SessionsPanelResourceUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// INCOMPLETE
		
		// set associated Terminal settings
		{
			CFStringRef		associatedTerminalName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagAssociatedTerminalFavorite,
														sizeof(associatedTerminalName), &associatedTerminalName,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAssociatedTerminal(associatedTerminalName);
				CFRelease(associatedTerminalName), associatedTerminalName = nullptr;
			}
			else
			{
				this->setAssociatedTerminal(nullptr);
			}
		}
		
		// set associated Format settings
		{
			CFStringRef		associatedFormatName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagAssociatedFormatFavorite,
														sizeof(associatedFormatName), &associatedFormatName,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAssociatedFormat(associatedFormatName);
				CFRelease(associatedFormatName), associatedFormatName = nullptr;
			}
			else
			{
				this->setAssociatedFormat(nullptr);
			}
		}
		
		// set associated Translation settings
		{
			CFStringRef		associatedTranslationName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagAssociatedTranslationFavorite,
														sizeof(associatedTranslationName), &associatedTranslationName,
														true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAssociatedTranslation(associatedTranslationName);
				CFRelease(associatedTranslationName), associatedTranslationName = nullptr;
			}
			else
			{
				this->setAssociatedTranslation(nullptr);
			}
		}
		
		// set command line
		{
			CFArrayRef		argumentListCFArray = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagCommandLine,
														sizeof(argumentListCFArray), &argumentListCFArray);
			if (kPreferences_ResultOK == prefsResult)
			{
				CFStringRef		concatenatedString = CFStringCreateByCombiningStrings
														(kCFAllocatorDefault, argumentListCFArray,
															CFSTR(" ")/* separator */);
				
				
				if (nullptr != concatenatedString)
				{
					this->setCommandLine(concatenatedString);
					CFRelease(concatenatedString), concatenatedString = nullptr;
				}
			}
			else
			{
				// ONLY if no actual command line was stored, generate a
				// command line based on other settings (like host name)
				Session_Protocol	givenProtocol = kSession_ProtocolSSH1;
				CFStringRef			hostCFString = nullptr;
				UInt16				portNumber = 0;
				CFStringRef			userCFString = nullptr;
				UInt16				preferenceCountOK = 0;
				Boolean				updateOK = false;
				
				
				preferenceCountOK = this->readPreferencesForRemoteServers(inSettings, true/* search defaults too */,
																			givenProtocol, hostCFString, portNumber, userCFString);
				if (4 != preferenceCountOK)
				{
					Console_Warning(Console_WriteLine, "unable to read one or more remote server preferences!");
				}
				updateOK = this->updateCommandLine(givenProtocol, hostCFString, portNumber, userCFString);
				if (false == updateOK)
				{
					Console_Warning(Console_WriteLine, "unable to update some part of command line based on given preferences!");
				}
				
				CFRelease(hostCFString), hostCFString = nullptr;
				CFRelease(userCFString), userCFString = nullptr;
			}
		}
	}
}// My_SessionsPanelResourceUI::readPreferences


/*!
Reads all preferences from the specified context that define a
remote server, optionally using global defaults as a fallback
for undefined settings.

Every result is defined, even if it is an empty string or a
zero value.  CFRelease() should be called on all strings.

Returns the number of preferences read successfully, which is
normally 4.  Any unsuccessful reads will have arbitrary values.

(4.0)
*/
UInt16
My_SessionsPanelResourceUI::
readPreferencesForRemoteServers		(Preferences_ContextRef		inSettings,
									 Boolean					inSearchDefaults,
									 Session_Protocol&			outProtocol,
									 CFStringRef&				outHostNameCopy,
									 UInt16&					outPortNumber,
									 CFStringRef&				outUserIDCopy)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	size_t					actualSize = 0;
	UInt16					result = 0;
	
	
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerProtocol, sizeof(outProtocol),
												&outProtocol, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outProtocol = kSession_ProtocolSSH1; // arbitrary
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerHost, sizeof(outHostNameCopy),
												&outHostNameCopy, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outHostNameCopy = CFSTR(""); // arbitrary
		CFRetain(outHostNameCopy);
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerPort, sizeof(outPortNumber),
												&outPortNumber, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outPortNumber = 22; // arbitrary
	}
	prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagServerUserID, sizeof(outUserIDCopy),
												&outUserIDCopy, inSearchDefaults, &actualSize);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		outUserIDCopy = CFSTR(""); // arbitrary
		CFRetain(outUserIDCopy);
	}
	
	return result;
}// My_SessionsPanelResourceUI::readPreferencesForRemoteServers


/*!
Deletes all the items in the specified pop-up menu and
rebuilds the menu based on current preferences of the
given type.  Also tracks the number of items added so
that they can be removed later.

(4.0)
*/
void
My_SessionsPanelResourceUI::
rebuildFavoritesMenu	(HIViewID const&		inMenuButtonID,
						 UInt32					inAnchorCommandID,
						 Quills::Prefs::Class	inCollectionsToUse,
						 UInt32					inEachNewItemCommandID,
						 MenuItemIndex&			inoutItemCountTracker)
{
	// it is possible for an event to arrive while the UI is not defined
	if (IsValidWindowRef(HIViewGetWindow(this->mainView)))
	{
		HIViewWrap		popUpMenuView(inMenuButtonID, HIViewGetWindow(this->mainView));
		OSStatus		error = noErr;
		MenuRef			favoritesMenu = nullptr;
		MenuItemIndex	defaultIndex = 0;
		MenuItemIndex	otherItemCount = 0;
		Size			actualSize = 0;
		
		
		error = GetControlData(popUpMenuView, kControlMenuPart, kControlPopupButtonMenuRefTag,
								sizeof(favoritesMenu), &favoritesMenu, &actualSize);
		assert_noerr(error);
		
		// find the key item to use as an anchor point
		error = GetIndMenuItemWithCommandID(favoritesMenu, inAnchorCommandID, 1/* which match to return */,
											&favoritesMenu, &defaultIndex);
		assert_noerr(error);
		
		// erase previous items
		if (0 != inoutItemCountTracker)
		{
			(OSStatus)DeleteMenuItems(favoritesMenu, defaultIndex + 1/* first item */, inoutItemCountTracker);
		}
		otherItemCount = CountMenuItems(favoritesMenu);
		
		// add the names of all terminal configurations to the menu;
		// update global count of items added at that location
		inoutItemCountTracker = 0;
		(Preferences_Result)Preferences_InsertContextNamesInMenu(inCollectionsToUse, favoritesMenu,
																	defaultIndex, 0/* indentation level */,
																	inEachNewItemCommandID, inoutItemCountTracker);
		SetControl32BitMaximum(popUpMenuView, otherItemCount + inoutItemCountTracker);
		
		// TEMPORARY: verify that this final step is necessary...
		error = SetControlData(popUpMenuView, kControlMenuPart, kControlPopupButtonMenuRefTag,
								sizeof(favoritesMenu), &favoritesMenu);
		assert_noerr(error);
	}
}// My_SessionsPanelResourceUI::rebuildFavoritesMenu


/*!
Deletes all the items in the Format pop-up menu and
rebuilds the menu based on current preferences.

(4.0)
*/
void
My_SessionsPanelResourceUI::
rebuildFormatMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuFormat, kCommandFormatDefault/* anchor */, Quills::Prefs::FORMAT,
							kCommandFormatByFavoriteName/* command ID of new items */, this->_numberOfFormatItemsAdded);
}// My_SessionsPanelResourceUI::rebuildFormatMenu


/*!
Deletes all the items in the Terminal pop-up menu and
rebuilds the menu based on current preferences.

(3.1)
*/
void
My_SessionsPanelResourceUI::
rebuildTerminalMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuTerminal, kCommandTerminalDefault/* anchor */, Quills::Prefs::TERMINAL,
							kCommandTerminalByFavoriteName/* command ID of new items */, this->_numberOfTerminalItemsAdded);
}// My_SessionsPanelResourceUI::rebuildTerminalMenu


/*!
Deletes all the items in the Translation pop-up menu and
rebuilds the menu based on current preferences.

(4.0)
*/
void
My_SessionsPanelResourceUI::
rebuildTranslationMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuTranslation, kCommandTranslationTableDefault/* anchor */, Quills::Prefs::TRANSLATION,
							kCommandTranslationTableByFavoriteName/* command ID of new items */, this->_numberOfTranslationItemsAdded);
}// My_SessionsPanelResourceUI::rebuildTranslationMenu


/*!
Saves every text field in the panel to the data model.
It is necessary to treat fields specially because they
do not have obvious state changes (as, say, buttons do);
they might need saving when focus is lost or the window
is closed, etc.

(3.1)
*/
void
My_SessionsPanelResourceUI::
saveFieldPreferences	(Preferences_ContextRef		inoutSettings)
{
	if (nullptr != inoutSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// set command line
		{
			CFStringRef		commandLineCFString = nullptr;
			Boolean			saveOK = false;
			
			
			GetControlTextAsCFString(_fieldCommandLine, commandLineCFString);
			if (nullptr != commandLineCFString)
			{
				CFArrayRef		argumentListCFArray = nullptr;
				
				
				argumentListCFArray = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, commandLineCFString,
																				CFSTR(" ")/* separator */);
				if (nullptr != argumentListCFArray)
				{
					prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagCommandLine,
																sizeof(argumentListCFArray), &argumentListCFArray);
					if (kPreferences_ResultOK == prefsResult) saveOK = true;
					CFRelease(argumentListCFArray), argumentListCFArray = nullptr;
				}
				CFRelease(commandLineCFString), commandLineCFString = nullptr;
			}
			if (false == saveOK)
			{
				Console_Warning(Console_WriteLine, "failed to set command line");
			}
		}
	}
}// My_SessionsPanelResourceUI::saveFieldPreferences


/*!
Saves the given preferences to the specified context.

Returns the number of preferences saved successfully, which is
normally 4.

(4.0)
*/
UInt16
My_SessionsPanelResourceUI::
savePreferencesForRemoteServers		(Preferences_ContextRef		inoutSettings,
									 Session_Protocol			inProtocol,
									 CFStringRef				inHostName,
									 UInt16						inPortNumber,
									 CFStringRef				inUserID)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	SInt16					portAsSInt16 = STATIC_CAST(inPortNumber, SInt16);
	UInt16					result = 0;
	
	
	// host name
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerHost,
												sizeof(inHostName), &inHostName);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save host name setting");
	}
	// protocol
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerProtocol,
												sizeof(inProtocol), &inProtocol);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save protocol setting");
	}
	// port number
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerPort,
												sizeof(portAsSInt16), &portAsSInt16);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save port number setting");
	}
	// user ID
	prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagServerUserID,
												sizeof(inUserID), &inUserID);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	else
	{
		Console_Warning(Console_WriteLine, "unable to save user name setting");
	}
	
	return result;
}// My_SessionsPanelResourceUI::savePreferencesForRemoteServers


/*!
Changes the Format menu selection to the specified
match, or chooses the Default if none is found or
the name is not defined.

(4.0)
*/
void
My_SessionsPanelResourceUI::
setAssociatedFormat		(CFStringRef	inContextNameOrNull)
{
	SInt32 const	kFallbackValue = 1; // index of Default item
	HIViewWrap		popUpMenuButton(idMyPopUpMenuFormat, HIViewGetWindow(this->mainView));
	
	
	if (nullptr == inContextNameOrNull)
	{
		SetControl32BitValue(popUpMenuButton, kFallbackValue);
	}
	else
	{
		OSStatus	error = noErr;
		
		
		error = DialogUtilities_SetPopUpItemByText(popUpMenuButton, inContextNameOrNull, kFallbackValue);
		if (controlPropertyNotFoundErr == error)
		{
			Console_Warning(Console_WriteValueCFString, "unable to find menu item for requested Format context",
							inContextNameOrNull);
		}
		else
		{
			assert_noerr(error);
		}
	}
}// My_SessionsPanelResourceUI::setAssociatedFormat


/*!
Changes the Terminal menu selection to the specified
match, or chooses the Default if none is found or
the name is not defined.

(4.0)
*/
void
My_SessionsPanelResourceUI::
setAssociatedTerminal	(CFStringRef	inContextNameOrNull)
{
	SInt32 const	kFallbackValue = 1; // index of Default item
	HIViewWrap		popUpMenuButton(idMyPopUpMenuTerminal, HIViewGetWindow(this->mainView));
	
	
	if (nullptr == inContextNameOrNull)
	{
		SetControl32BitValue(popUpMenuButton, kFallbackValue);
	}
	else
	{
		OSStatus	error = noErr;
		
		
		error = DialogUtilities_SetPopUpItemByText(popUpMenuButton, inContextNameOrNull, kFallbackValue);
		if (controlPropertyNotFoundErr == error)
		{
			Console_Warning(Console_WriteValueCFString, "unable to find menu item for requested Terminal context",
							inContextNameOrNull);
		}
		else
		{
			assert_noerr(error);
		}
	}
}// My_SessionsPanelResourceUI::setAssociatedTerminal


/*!
Changes the Translation menu selection to the specified
match, or chooses the Default if none is found or the
name is not defined.

(4.0)
*/
void
My_SessionsPanelResourceUI::
setAssociatedTranslation		(CFStringRef	inContextNameOrNull)
{
	SInt32 const	kFallbackValue = 1; // index of Default item
	HIViewWrap		popUpMenuButton(idMyPopUpMenuTranslation, HIViewGetWindow(this->mainView));
	
	
	if (nullptr == inContextNameOrNull)
	{
		SetControl32BitValue(popUpMenuButton, kFallbackValue);
	}
	else
	{
		OSStatus	error = noErr;
		
		
		error = DialogUtilities_SetPopUpItemByText(popUpMenuButton, inContextNameOrNull, kFallbackValue);
		if (controlPropertyNotFoundErr == error)
		{
			Console_Warning(Console_WriteValueCFString, "unable to find menu item for requested Translation context",
							inContextNameOrNull);
		}
		else
		{
			assert_noerr(error);
		}
	}
}// My_SessionsPanelResourceUI::setAssociatedTranslation


/*!
Changes the command line field.

IMPORTANT:	Normally, this is for initialization; it overwrites
			the entire field with a set value.  Most of the time
			it is more appropriate to use updateCommandLine() to
			respond to other changes (such as, the host).

(3.1)
*/
void
My_SessionsPanelResourceUI::
setCommandLine		(CFStringRef	inCommandLine)
{
	SetControlTextWithCFString(_fieldCommandLine, inCommandLine);
}// My_SessionsPanelResourceUI::setCommandLine


/*!
Since everything is “really” a local Unix command, this
routine uses the given remote options and updates the
command line field with appropriate values.

Note that the port number should be redundantly specified
as both a number and a string.

Every value should be defined, but if a string is empty
or the port number is 0, it is skipped in the resulting
command line.

Returns "true" only if the update was successful.

NOTE:	Currently, this routine basically obliterates
		whatever the user may have entered.  A more
		desirable solution is to implement this with
		a find/replace strategy that changes only
		those command line arguments that need to be
		updated, preserving any additional parts of
		the command line.

See also setCommandLine().

(3.1)
*/
Boolean
My_SessionsPanelResourceUI::
updateCommandLine	(Session_Protocol	inProtocol,
					 CFStringRef		inHostName,
					 UInt16				inPortNumber,
					 CFStringRef		inUserID)
{
	CFMutableStringRef		newCommandLineCFString = CFStringCreateMutable(kCFAllocatorDefault,
																			0/* length, or 0 for unlimited */);
	Str31					portString;
	Boolean					result = false;
	
	
	// the host field is required when updating the command line
	if ((nullptr != inHostName) && (0 == CFStringGetLength(inHostName)))
	{
		inHostName = nullptr;
	}
	
	PLstrcpy(portString, "\p");
	if (0 != inPortNumber) NumToString(STATIC_CAST(inPortNumber, SInt32), portString);
	
	if ((nullptr != newCommandLineCFString) && (nullptr != inHostName))
	{
		CFRetainRelease		portNumberCFString(CFStringCreateWithPascalStringNoCopy
												(kCFAllocatorDefault, portString, kCFStringEncodingASCII, kCFAllocatorNull),
												true/* is retained */);
		Boolean				standardLoginOptionAppend = false;
		Boolean				standardHostNameAppend = false;
		Boolean				standardPortAppend = false;
		Boolean				standardIPv6Append = false;
		
		
		// see what is available
		// NOTE: In the following, whitespace should probably be stripped
		//       from all field values first, since otherwise the user
		//       could enter a non-empty but blank value that would become
		//       a broken command line.
		if ((false == portNumberCFString.exists()) || (0 == CFStringGetLength(portNumberCFString.returnCFStringRef())))
		{
			portNumberCFString.clear();
		}
		if ((nullptr != inUserID) && (0 == CFStringGetLength(inUserID)))
		{
			inUserID = nullptr;
		}
		
		// TEMPORARY - convert all of this logic to Python!
		
		// set command based on the protocol, and add arguments
		// based on the specific command and the available data
		result = true; // initially...
		switch (inProtocol)
		{
		case kSession_ProtocolFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ftp"));
			if (nullptr != inHostName)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != inUserID)
				{
					CFStringAppend(newCommandLineCFString, inUserID);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardPortAppend = true; // ftp supports a standalone server port number argument
			break;
		
		case kSession_ProtocolSFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/sftp"));
			if (nullptr != inHostName)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != inUserID)
				{
					CFStringAppend(newCommandLineCFString, inUserID);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if (portNumberCFString.exists())
			{
				// sftp uses "-oPort=port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -oPort="));
				CFStringAppend(newCommandLineCFString, portNumberCFString.returnCFStringRef());
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			break;
		
		case kSession_ProtocolSSH1:
		case kSession_ProtocolSSH2:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ssh"));
			if (kSession_ProtocolSSH2 == inProtocol)
			{
				// -2: protocol version 2
				CFStringAppend(newCommandLineCFString, CFSTR(" -2 "));
			}
			else
			{
				// -1: protocol version 1
				CFStringAppend(newCommandLineCFString, CFSTR(" -1 "));
			}
			if (portNumberCFString.exists())
			{
				// ssh uses "-p port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -p "));
				CFStringAppend(newCommandLineCFString, portNumberCFString.returnCFStringRef());
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardIPv6Append = true; // ssh supports a "-6" argument if the host is exactly in IP version 6 format
			standardLoginOptionAppend = true; // ssh supports a "-l userid" option
			standardHostNameAppend = true; // ssh supports a standalone server host argument
			break;
		
		case kSession_ProtocolTelnet:
			// -K: disable automatic login
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/telnet -K "));
			standardIPv6Append = true; // telnet supports a "-6" argument if the host is exactly in IP version 6 format
			standardLoginOptionAppend = true; // telnet supports a "-l userid" option
			standardHostNameAppend = true; // telnet supports a standalone server host argument
			standardPortAppend = true; // telnet supports a standalone server port number argument
			break;
		
		default:
			// ???
			result = false;
			break;
		}
		
		if (result)
		{
			// since commands tend to follow similar conventions, this section
			// appends options in “standard” form if supported by the protocol
			// (if MOST commands do not have an option, append it in the above
			// switch, instead)
			if ((standardIPv6Append) && (nullptr != inHostName) &&
				(CFStringFind(inHostName, CFSTR(":"), 0/* options */).length > 0))
			{
				// -6: address is in IPv6 format
				CFStringAppend(newCommandLineCFString, CFSTR(" -6 "));
			}
			if ((standardLoginOptionAppend) && (nullptr != inUserID))
			{
				// standard form is a Unix command accepting a "-l" option
				// followed by the user ID for login
				CFStringAppend(newCommandLineCFString, CFSTR(" -l "));
				CFStringAppend(newCommandLineCFString, inUserID);
			}
			if ((standardHostNameAppend) && (nullptr != inHostName))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the host name of the server to connect to
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, inHostName);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if ((standardPortAppend) && (portNumberCFString.exists()))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the port number on the server to connect to, AFTER the
				// standalone host name option
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, portNumberCFString.returnCFStringRef());
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			
			// update command line field
			if (0 == CFStringGetLength(newCommandLineCFString))
			{
				result = false;
			}
			else
			{
				SetControlTextWithCFString(this->_fieldCommandLine, newCommandLineCFString);
				(OSStatus)HIViewSetNeedsDisplay(this->_fieldCommandLine, true);
				result = true;
			}
		}
		
		CFRelease(newCommandLineCFString), newCommandLineCFString = nullptr;
	}
	
	// if unsuccessful, clear the command line
	if (false == result)
	{
		SetControlTextWithCFString(this->_fieldCommandLine, CFSTR(""));
		(OSStatus)HIViewSetNeedsDisplay(this->_fieldCommandLine, true);
	}
	
	return result;
}// My_SessionsPanelResourceUI::updateCommandLine


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
Invoked whenever a monitored preference value is changed.
Responds by updating the user interface.

(3.1)
*/
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					inMySessionsTabResourcePtr)
{
	//Preferences_ChangeContext*		contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	My_SessionsPanelResourceUI*		ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsPanelResourceUI*);
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_ChangeNumberOfContexts:
	case kPreferences_ChangeContextName:
		ptr->rebuildFormatMenu();
		ptr->rebuildTerminalMenu();
		ptr->rebuildTranslationMenu();
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the command line field
in the Resource tab.  This saves the preferences
for text field data.

(3.1)
*/
pascal OSStatus
receiveFieldChangedInCommandLine	(EventHandlerCallRef	inHandlerCallRef,
									 EventRef				inEvent,
									 void*					inMySessionsTabResourcePtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		resourceInterfacePtr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsPanelResourceUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// synchronize the post-input change with preferences
	{
		My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																			My_SessionsPanelResourceDataPtr);
		
		
		resourceInterfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
	}
	
	return result;
}// receiveFieldChangedInCommandLine


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons in various tabs.

(3.1)
*/
pascal OSStatus
receiveHICommand	(EventHandlerCallRef	inHandlerCallRef,
					 EventRef				inEvent,
					 void*					inMySessionsUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	// WARNING: More than one UI uses this handler.  The context will
	// depend on the command ID.
	My_SessionsPanelGraphicsUI*		graphicsInterfacePtr = REINTERPRET_CAST(inMySessionsUIPtr, My_SessionsPanelGraphicsUI*);
	My_SessionsPanelResourceUI*		resourceInterfacePtr = REINTERPRET_CAST(inMySessionsUIPtr, My_SessionsPanelResourceUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
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
			case kCommandFormatDefault:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// delete the “associated Format” preference, which will cause
					// a fallback to the default when it is later queried
					prefsResult = Preferences_ContextDeleteData(dataPtr->dataModel, kPreferences_TagAssociatedFormatFavorite);
					
					// pass this handler through to the window, which will update the font and colors!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandFormatByFavoriteName:
				{
					Boolean		isError = true;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation
																							(resourceInterfacePtr->panel),
																							My_SessionsPanelResourceDataPtr);
							Preferences_Result					prefsResult = kPreferences_ResultOK;
							
							
							// set this name as the new preference value
							prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagAssociatedFormatFavorite,
																		sizeof(collectionName), &collectionName);
							if (kPreferences_ResultOK == prefsResult) isError = false;
							CFRelease(collectionName), collectionName = nullptr;
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window, which will update the font and colors!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTerminalDefault:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// delete the “associated Terminal” preference, which will cause
					// a fallback to the default when it is later queried
					prefsResult = Preferences_ContextDeleteData(dataPtr->dataModel, kPreferences_TagAssociatedTerminalFavorite);
					
					// pass this handler through to the window
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTerminalByFavoriteName:
				{
					Boolean		isError = true;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation
																							(resourceInterfacePtr->panel),
																							My_SessionsPanelResourceDataPtr);
							Preferences_Result					prefsResult = kPreferences_ResultOK;
							
							
							// set this name as the new preference value
							prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagAssociatedTerminalFavorite,
																		sizeof(collectionName), &collectionName);
							if (kPreferences_ResultOK == prefsResult) isError = false;
							CFRelease(collectionName), collectionName = nullptr;
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window, which will update the terminal settings!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTranslationTableDefault:
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// delete the “associated Translation” preference, which will cause
					// a fallback to the default when it is later queried
					prefsResult = Preferences_ContextDeleteData(dataPtr->dataModel, kPreferences_TagAssociatedTranslationFavorite);
					
					// pass this handler through to the window
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandTranslationTableByFavoriteName:
				{
					Boolean		isError = true;
					
					
					// update the pop-up button
					(OSStatus)CallNextEventHandler(inHandlerCallRef, inEvent);
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation
																							(resourceInterfacePtr->panel),
																							My_SessionsPanelResourceDataPtr);
							Preferences_Result					prefsResult = kPreferences_ResultOK;
							
							
							// set this name as the new preference value
							prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagAssociatedTranslationFavorite,
																		sizeof(collectionName), &collectionName);
							if (kPreferences_ResultOK == prefsResult) isError = false;
							CFRelease(collectionName), collectionName = nullptr;
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window, which will update the translation settings!
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandShowCommandLine:
				// this normally means “show command line floater”, but in the context
				// of an active New Session sheet, it means “select command line field”
				{
					HIWindowRef		window = HIViewGetWindow(resourceInterfacePtr->mainView);
					
					
					(OSStatus)DialogUtilities_SetKeyboardFocus(HIViewWrap(idMyFieldCommandLine, window));
					result = noErr;
				}
				break;
			
			case kCommandEditCommandLine:
				// show the server browser panel and target the command line field
				{
					My_SessionsPanelResourceDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																					My_SessionsPanelResourceDataPtr);
					HIWindowRef							window = HIViewGetWindow(resourceInterfacePtr->mainView);
					HIViewWrap							panelDisplayButton(idMyButtonConnectToServer, window);
					Session_Protocol					dataModelProtocol = kSession_ProtocolSSH1;
					CFStringRef							dataModelHostName = nullptr;
					UInt16								dataModelPort = 0;
					CFStringRef							dataModelUserID = nullptr;
					UInt16								preferenceCountOK = 0;
					
					
					preferenceCountOK = resourceInterfacePtr->readPreferencesForRemoteServers
										(dataPtr->dataModel, true/* search defaults too */,
											dataModelProtocol, dataModelHostName, dataModelPort, dataModelUserID);
					if (4 != preferenceCountOK)
					{
						Console_Warning(Console_WriteLine, "unable to read one or more remote server preferences!");
					}
					SetControl32BitValue(panelDisplayButton, kControlCheckBoxCheckedValue);
					ServerBrowser_SetVisible(true);
					ServerBrowser_SetEventTarget(GetWindowEventTarget(window), dataModelProtocol, dataModelHostName, dataModelPort, dataModelUserID);
					
					CFRelease(dataModelHostName), dataModelHostName = nullptr;
					CFRelease(dataModelUserID), dataModelUserID = nullptr;
					
					result = noErr;
				}
				break;
			
			case kCommandSetTEKModeDisabled:
			case kCommandSetTEKModeTEK4014:
			case kCommandSetTEKModeTEK4105:
				{
					VectorInterpreter_Mode				newMode = kVectorInterpreter_ModeDisabled;
					My_SessionsPanelGraphicsDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(graphicsInterfacePtr->panel),
																					My_SessionsPanelGraphicsDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					if (kCommandSetTEKModeTEK4014 == received.commandID) newMode = kVectorInterpreter_ModeTEK4014;
					if (kCommandSetTEKModeTEK4105 == received.commandID) newMode = kVectorInterpreter_ModeTEK4105;
					graphicsInterfacePtr->setMode(newMode);
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagTektronixMode,
																sizeof(newMode), &newMode);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "unable to save TEK mode setting");
					}
					result = noErr;
				}
				break;
			
			case kCommandSetTEKPageClearsScreen:
				assert(received.attributes & kHICommandFromControl);
				{
					HIViewRef const						kCheckBox = received.source.control;
					Boolean const						kIsSet = (kControlCheckBoxCheckedValue == GetControl32BitValue(kCheckBox));
					My_SessionsPanelGraphicsDataPtr		dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(graphicsInterfacePtr->panel),
																					My_SessionsPanelGraphicsDataPtr);
					Preferences_Result					prefsResult = kPreferences_ResultOK;
					
					
					graphicsInterfacePtr->setPageClears(kIsSet);
					prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagTektronixPAGEClearsScreen,
																sizeof(kIsSet), &kIsSet);
					if (kPreferences_ResultOK != prefsResult)
					{
						Console_Warning(Console_WriteLine, "unable to save TEK PAGE setting");
					}
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
Implements "kEventNetEvents_ServerBrowserNewData" and
"kEventNetEvents_ServerBrowserNewEventTarget" of
"kEventClassNetEvents_ServerBrowser" for the Resource tab.

When data is changed, the command line is rewritten to use the
new protocol, remote server, port number, and user ID from the
panel.

When the event target is changed, the highlighting is removed
from the button that originally spawned the server browser panel.

(4.0)
*/
pascal OSStatus
receiveServerBrowserEvent	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inMySessionsPanelResourceUIPtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		resourceInterfacePtr = REINTERPRET_CAST(inMySessionsPanelResourceUIPtr, My_SessionsPanelResourceUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassNetEvents_ServerBrowser);
	assert((kEventKind == kEventNetEvents_ServerBrowserNewData) ||
			(kEventKind == kEventNetEvents_ServerBrowserNewEventTarget));
	
	switch (kEventKind)
	{
	case kEventNetEvents_ServerBrowserNewData:
		// update command line field
		{
			My_SessionsPanelResourceDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(resourceInterfacePtr->panel),
																				My_SessionsPanelResourceDataPtr);
			Session_Protocol					newProtocol = kSession_ProtocolSSH1; // arbitrary initial value
			Session_Protocol					existingProtocol = kSession_ProtocolSSH1; // arbitrary initial value
			CFStringRef							newHostName = nullptr;
			CFStringRef							existingHostName = nullptr;
			UInt32								newPortNumber = 0;
			UInt16								existingPortNumber = 0;
			CFStringRef							newUserID = nullptr;
			CFStringRef							existingUserID = nullptr;
			UInt16								preferenceCountOK = 0;
			OSStatus							error = noErr;
			Boolean								updateOK = false;
			
			
			// first set defaults
			preferenceCountOK = resourceInterfacePtr->readPreferencesForRemoteServers(panelDataPtr->dataModel, true/* search defaults too */,
																						existingProtocol, existingHostName,
																						existingPortNumber, existingUserID);
			if (4 != preferenceCountOK) // TEMPORARY - should this be an assertion?
			{
				Console_Warning(Console_WriteLine, "unable to set defaults for one or more remote server preferences!");
			}
			
			// now read any updated values; note that CFStrings in events are automatically released
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_Protocol, typeNetEvents_SessionProtocol,
															newProtocol);
			if (noErr != error)
			{
				//Console_WriteLine("no protocol found in event"); // debug - this is not an error
				newProtocol = existingProtocol;
			}
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_HostName, typeCFStringRef,
															newHostName);
			if (noErr != error)
			{
				//Console_WriteLine("no host name found in event"); // debug - this is not an error
				newHostName = existingHostName;
			}
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_PortNumber, typeUInt32,
															newPortNumber);
			if (noErr != error)
			{
				//Console_WriteLine("no port number found in event"); // debug - this is not an error
				newPortNumber = existingPortNumber;
			}
			error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamNetEvents_UserID, typeCFStringRef,
															newUserID);
			if (noErr != error)
			{
				//Console_WriteLine("no user ID found in event"); // debug - this is not an error
				newUserID = existingUserID;
			}
			
			// finally, update the user interface to incorporate the changes
			updateOK = resourceInterfacePtr->updateCommandLine(newProtocol, newHostName, newPortNumber, newUserID);
			if (false == updateOK) // TEMPORARY - should this be an assertion?
			{
				Console_Warning(Console_WriteLine, "unable to update some part of command line based on panel settings!");
			}
			preferenceCountOK = resourceInterfacePtr->savePreferencesForRemoteServers
														(panelDataPtr->dataModel, newProtocol, newHostName,
															newPortNumber, newUserID);
			if (4 != preferenceCountOK)
			{
				Console_Warning(Console_WriteValue, "some remote server settings could not be saved; 4 exist, saved",
								preferenceCountOK);
			}
			resourceInterfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
			
			CFRelease(existingHostName), existingHostName = nullptr;
			CFRelease(existingUserID), existingUserID = nullptr;
			
			result = noErr;
		}
		break;
	
	case kEventNetEvents_ServerBrowserNewEventTarget:
		// remove highlighting from the panel-spawning button
		{
			HIWindowRef		window = HIViewGetWindow(resourceInterfacePtr->mainView);
			HIViewWrap		panelDisplayButton(idMyButtonConnectToServer, window);
			
			
			SetControl32BitValue(panelDisplayButton, kControlCheckBoxUncheckedValue);
			result = noErr;
		}
		break;
	
	default:
		// ???
		result = eventNotHandledErr;
		break;
	}
	return result;
}// receiveServerBrowserEvent

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
