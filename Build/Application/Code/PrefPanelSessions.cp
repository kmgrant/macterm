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
#include "DialogResources.h"
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
#include "Session.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelSessions.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyPopUpMenuTerminal			= { 'Term', 0/* ID */ };
HIViewID const	idMySeparatorTerminal			= { 'BotD', 0/* ID */ };
HIViewID const	idMySeparatorRemoteSessionsOnly	= { 'RmSD', 0/* ID */ };
HIViewID const	idMyPopUpMenuProtocol			= { 'Prot', 0/* ID */ };
HIViewID const	idMyFieldHostName				= { 'Host', 0/* ID */ };
HIViewID const	idMyButtonLookUpHostName		= { 'Look', 0/* ID */ };
HIViewID const	idMyChasingArrowsDNSWait		= { 'Wait', 0/* ID */ };
HIViewID const	idMyFieldPortNumber				= { 'Port', 0/* ID */ };
HIViewID const	idMyFieldUserID					= { 'User', 0/* ID */ };
HIViewID const	idMyFieldCommandLine			= { 'CmdL', 0/* ID */ };
HIViewID const	idMyHelpTextCommandLine			= { 'CmdH', 0/* ID */ };
HIViewID const	idMyStaticTextCaptureFilePath	= { 'CapP', 0/* ID */ };
HIViewID const	idMyHelpTextControlKeys			= { 'CtlH', 0/* ID */ };
HIViewID const	idMyButtonChangeInterruptKey	= { 'Intr', 0/* ID */ };
HIViewID const	idMyButtonChangeSuspendKey		= { 'Susp', 0/* ID */ };
HIViewID const	idMyButtonChangeResumeKey		= { 'Resu', 0/* ID */ };

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
	Session_Protocol	selectedProtocol;	//!< indicates the protocol new remote sessions should use
	
	Boolean
	lookupHostName ();
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	void
	rebuildTerminalMenu ();
	
	MenuItemIndex
	returnProtocolMenuCommandIndex	(UInt32);
	
	Boolean
	updateCommandLine ();
	
	void
	updatePortNumberField ();

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);

private:
	HIViewWrap							_fieldHostName;					//!< the name, IP address or other identifier for the remote host
	HIViewWrap							_fieldPortNumber;				//!< the port number on which to attempt a connection on the host
	HIViewWrap							_fieldUserID;					//!< the (usually 8-character) login name to use for the server
	HIViewWrap							_fieldCommandLine;				//!< text of Unix command line to run
	CommonEventHandlers_HIViewResizer	_containerResizer;
	CarbonEventHandlerWrap				_buttonCommandsHandler;			//!< invoked when a button is clicked
	CarbonEventHandlerWrap				_whenHostNameChangedHandler;	//!< invoked when the host name field changes
	CarbonEventHandlerWrap				_whenPortNumberChangedHandler;	//!< invoked when the port number field changes
	CarbonEventHandlerWrap				_whenUserIDChangedHandler;		//!< invoked when the user ID field changes
	CarbonEventHandlerWrap				_whenLookupCompleteHandler;		//!< invoked when a DNS query finally returns
	ListenerModel_ListenerRef			_whenFavoritesChangedHandler;	//!< used to manage Terminal pop-up menu
	MenuItemIndex						_numberOfTerminalItemsAdded;	//!< used to manage Terminal pop-up menu
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
pascal OSStatus		receiveFieldChanged						(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveHICommand						(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveLookupComplete					(EventHandlerCallRef, EventRef, void*);

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
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_SessionsTabDataFlow*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabDataFlow*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyStaticTextCaptureFilePath, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	// INCOMPLETE
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
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
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
		// UNIMPLEMENTED
	}
}// My_SessionsPanelGraphicsUI::readPreferences


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
	HIWindowRef const		kPanelWindow = GetControlOwner(inContainer);
	//My_SessionsTabControlKeys*	dataPtr = REINTERPRET_CAST(inContext, My_SessionsTabControlKeys*);
	HIViewWrap				viewWrap;
	
	
	viewWrap = HIViewWrap(idMyHelpTextControlKeys, kPanelWindow);
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
selectedProtocol				(kSession_ProtocolSSH1), // TEMPORARY: probably should read NIB and determine default menu item to find protocol value
_fieldHostName					(HIViewWrap(idMyFieldHostName, inOwningWindow)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(HostNameLimiter)),
_fieldPortNumber				(HIViewWrap(idMyFieldPortNumber, inOwningWindow)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(NumericalLimiter)),
_fieldUserID					(HIViewWrap(idMyFieldUserID, inOwningWindow)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(NoSpaceLimiter)),
_fieldCommandLine				(HIViewWrap(idMyFieldCommandLine, inOwningWindow)
									<< HIViewWrap_AssertExists
									<< HIViewWrap_InstallKeyFilter(UnixCommandLineLimiter)),
_containerResizer				(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
									My_SessionsPanelResourceUI::deltaSize, this/* context */),
_buttonCommandsHandler			(GetWindowEventTarget(inOwningWindow), receiveHICommand,
									CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
									this/* user data */),
_whenHostNameChangedHandler		(GetControlEventTarget(this->_fieldHostName), receiveFieldChanged,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this/* user data */),
_whenPortNumberChangedHandler	(GetControlEventTarget(this->_fieldPortNumber), receiveFieldChanged,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this/* user data */),
_whenUserIDChangedHandler		(GetControlEventTarget(this->_fieldUserID), receiveFieldChanged,
									CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
									this/* user data */),
_whenLookupCompleteHandler		(GetApplicationEventTarget(), receiveLookupComplete,
									CarbonEventSetInClass(CarbonEventClass(kEventClassNetEvents_DNS),
															kEventNetEvents_HostLookupComplete),
									this/* user data */),
_whenFavoritesChangedHandler	(ListenerModel_NewStandardListener(preferenceChanged, this/* context */)),
_numberOfTerminalItemsAdded		(0)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
	
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_StartMonitoring
					(this->_whenFavoritesChangedHandler, kPreferences_ChangeNumberOfContexts,
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
	HIWindowRef const				kPanelWindow = GetControlOwner(inContainer);
	My_SessionsPanelResourceUI*		dataPtr = REINTERPRET_CAST(inContext, My_SessionsPanelResourceUI*);
	HIViewWrap						viewWrap;
	
	
	viewWrap = HIViewWrap(idMySeparatorTerminal, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMySeparatorRemoteSessionsOnly, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyChasingArrowsDNSWait, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyButtonLookUpHostName, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(inDeltaX, 0/* delta Y */);
	dataPtr->_fieldHostName << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyHelpTextCommandLine, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	dataPtr->_fieldCommandLine << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	// INCOMPLETE
}// My_SessionsPanelResourceUI::deltaSize


/*!
Performs a DNR lookup of the host given in the dialog
box’s main text field, and when complete replaces the
contents of the text field with the resolved IP address
(as a string).

Returns true if the lookup succeeded.

(3.1)
*/
Boolean
My_SessionsPanelResourceUI::
lookupHostName ()
{
	CFStringRef		textCFString = nullptr;
	Boolean			result = false;
	
	
	GetControlTextAsCFString(_fieldHostName, textCFString);
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
			delete (REINTERPRET_CAST(inDataPtr, My_SessionsPanelResourceDataPtr));
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
		// UNIMPLEMENTED
	}
}// My_SessionsPanelResourceUI::readPreferences


/*!
Deletes all the items in the Terminal pop-up menu and
rebuilds the menu based on current preferences.

(3.1)
*/
void
My_SessionsPanelResourceUI::
rebuildTerminalMenu ()
{
	HIViewWrap		terminalPopUpMenuView(idMyPopUpMenuTerminal, HIViewGetWindow(this->mainView));
	OSStatus		error = noErr;
	MenuRef			favoritesMenu = nullptr;
	MenuItemIndex	defaultIndex = 0;
	Size			actualSize = 0;
	
	
	error = GetControlData(terminalPopUpMenuView, kControlMenuPart, kControlPopupButtonMenuRefTag,
							sizeof(favoritesMenu), &favoritesMenu, &actualSize);
	assert_noerr(error);
	
	// find the key item to use as an anchor point
	error = GetIndMenuItemWithCommandID(favoritesMenu, kCommandTerminalDefaultFavorite, 1/* which match to return */,
										&favoritesMenu, &defaultIndex);
	assert_noerr(error);
	
	// erase previous items
	if (0 != this->_numberOfTerminalItemsAdded)
	{
		(OSStatus)DeleteMenuItems(favoritesMenu, defaultIndex + 1/* first item */, this->_numberOfTerminalItemsAdded);
	}
	
	// add the names of all session configurations to the menu;
	// update global count of items added at that location
	this->_numberOfTerminalItemsAdded = 0;
	(Preferences_Result)Preferences_InsertContextNamesInMenu(kPreferences_ClassTerminal, favoritesMenu,
																1/* default index */, 0/* indentation level */,
																0/* command ID */,
																this->_numberOfTerminalItemsAdded);
	
	// TEMPORARY: verify that this final step is necessary...
	error = SetControlData(terminalPopUpMenuView, kControlMenuPart, kControlPopupButtonMenuRefTag,
							sizeof(favoritesMenu), &favoritesMenu);
	assert_noerr(error);
}// My_SessionsPanelResourceUI::rebuildTerminalMenu


/*!
Returns the item index in the protocol pop-up menu
that matches the specified command.

(3.1)
*/
MenuItemIndex
My_SessionsPanelResourceUI::
returnProtocolMenuCommandIndex	(UInt32		inCommandID)
{
	MenuItemIndex	result = 0;
	OSStatus		error = GetIndMenuItemWithCommandID(GetControlPopupMenuHandle
														(HIViewWrap(idMyPopUpMenuProtocol, HIViewGetWindow(this->mainView))),
														inCommandID, 1/* 1 = return first match */,
														nullptr/* menu */, &result);
	assert_noerr(error);
	
	
	return result;
}// My_SessionsPanelResourceUI::returnProtocolMenuCommandIndex


/*!
Since everything is “really” a local Unix command,
this routine scans the Remote Session options and
updates the command line field with appropriate
values.

Returns "true" only if the update was successful.

NOTE:	Currently, this routine basically obliterates
		whatever the user may have entered.  A more
		desirable solution is to implement this with
		a find/replace strategy that changes only
		those command line arguments that need to be
		updated, preserving any additional parts of
		the command line.

(3.1)
*/
Boolean
My_SessionsPanelResourceUI::
updateCommandLine ()
{
	Boolean					result = true;
	CFStringRef				hostNameCFString = nullptr;
	CFMutableStringRef		newCommandLineCFString = CFStringCreateMutable(kCFAllocatorDefault,
																			0/* length, or 0 for unlimited */);
	
	
	// the host field is required when updating the command line
	GetControlTextAsCFString(this->_fieldHostName, hostNameCFString);
	if ((nullptr == hostNameCFString) || (0 == CFStringGetLength(hostNameCFString)))
	{
		hostNameCFString = nullptr;
	}
	
	if ((nullptr == newCommandLineCFString) || (nullptr == hostNameCFString))
	{
		// failed to create string!
		result = false;
	}
	else
	{
		CFStringRef		portNumberCFString = nullptr;
		CFStringRef		userIDCFString = nullptr;
		Boolean			standardLoginOptionAppend = false;
		Boolean			standardHostNameAppend = false;
		Boolean			standardPortAppend = false;
		Boolean			standardIPv6Append = false;
		
		
		// see what is available
		// NOTE: In the following, whitespace should probably be stripped
		//       from all field values first, since otherwise the user
		//       could enter a non-empty but blank value that would become
		//       a broken command line.
		GetControlTextAsCFString(this->_fieldPortNumber, portNumberCFString);
		if ((nullptr == portNumberCFString) || (0 == CFStringGetLength(portNumberCFString)))
		{
			portNumberCFString = nullptr;
		}
		GetControlTextAsCFString(this->_fieldUserID, userIDCFString);
		if ((nullptr == userIDCFString) || (0 == CFStringGetLength(userIDCFString)))
		{
			userIDCFString = nullptr;
		}
		
		// set command based on the protocol, and add arguments
		// based on the specific command and the available data
		switch (this->selectedProtocol)
		{
		case kSession_ProtocolFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ftp"));
			if (nullptr != hostNameCFString)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != userIDCFString)
				{
					CFStringAppend(newCommandLineCFString, userIDCFString);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, hostNameCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			standardPortAppend = true; // ftp supports a standalone server port number argument
			break;
		
		case kSession_ProtocolSFTP:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/sftp"));
			if (nullptr != hostNameCFString)
			{
				// ftp uses "user@host" format
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				if (nullptr != userIDCFString)
				{
					CFStringAppend(newCommandLineCFString, userIDCFString);
					CFStringAppend(newCommandLineCFString, CFSTR("@"));
				}
				CFStringAppend(newCommandLineCFString, hostNameCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if (nullptr != portNumberCFString)
			{
				// sftp uses "-oPort=port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -oPort="));
				CFStringAppend(newCommandLineCFString, portNumberCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			break;
		
		case kSession_ProtocolSSH1:
		case kSession_ProtocolSSH2:
			CFStringAppend(newCommandLineCFString, CFSTR("/usr/bin/ssh"));
			if (kSession_ProtocolSSH2 == this->selectedProtocol)
			{
				// -2: protocol version 2
				CFStringAppend(newCommandLineCFString, CFSTR(" -2 "));
			}
			else
			{
				// -1: protocol version 1
				CFStringAppend(newCommandLineCFString, CFSTR(" -1 "));
			}
			if (nullptr != portNumberCFString)
			{
				// ssh uses "-p port" to specify the port number
				CFStringAppend(newCommandLineCFString, CFSTR(" -p "));
				CFStringAppend(newCommandLineCFString, portNumberCFString);
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
			if ((standardIPv6Append) && (CFStringFind(hostNameCFString, CFSTR(":"), 0/* options */).length > 0))
			{
				// -6: address is in IPv6 format
				CFStringAppend(newCommandLineCFString, CFSTR(" -6 "));
			}
			if ((standardLoginOptionAppend) && (nullptr != userIDCFString))
			{
				// standard form is a Unix command accepting a "-l" option
				// followed by the user ID for login
				CFStringAppend(newCommandLineCFString, CFSTR(" -l "));
				CFStringAppend(newCommandLineCFString, userIDCFString);
			}
			if ((standardHostNameAppend) && (nullptr != hostNameCFString))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the host name of the server to connect to
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, hostNameCFString);
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
			}
			if ((standardPortAppend) && (nullptr != portNumberCFString))
			{
				// standard form is a Unix command accepting a standalone argument
				// that is the port number on the server to connect to, AFTER the
				// standalone host name option
				CFStringAppend(newCommandLineCFString, CFSTR(" "));
				CFStringAppend(newCommandLineCFString, portNumberCFString);
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
Sets the value of the port number field to match the
currently-selected protocol.  Use this to “clear” its
value appropriately.

(3.1)
*/
void
My_SessionsPanelResourceUI::
updatePortNumberField ()
{
	SInt16		newPort = 0;
	
	
	switch (this->selectedProtocol)
	{
	case kSession_ProtocolFTP:
		newPort = 21;
		break;
	
	case kSession_ProtocolSFTP:
	case kSession_ProtocolSSH1:
	case kSession_ProtocolSSH2:
		newPort = 22;
		break;
	
	case kSession_ProtocolTelnet:
		newPort = 23;
		break;
	
	default:
		// ???
		break;
	}
	
	if (0 != newPort)
	{
		SetControlNumericalText(this->_fieldPortNumber, newPort);
		(OSStatus)HIViewSetNeedsDisplay(this->_fieldPortNumber, true);
	}
}// My_SessionsPanelResourceUI::updatePortNumberField


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
		ptr->rebuildTerminalMenu();
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the remote session fields
in the Resource tab.  Since a command line is
constructed based on user entries, every change
requires an update to the command line field.

(3.1)
*/
pascal OSStatus
receiveFieldChanged	(EventHandlerCallRef	inHandlerCallRef,
					 EventRef				inEvent,
					 void*					inMySessionsTabResourcePtr)
{
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsPanelResourceUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with the command line field
	ptr->updateCommandLine();
	
	return result;
}// receiveFieldChanged


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
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsPanelResourceUI*);
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
			case kCommandNewProtocolSelectedFTP:
			case kCommandNewProtocolSelectedSFTP:
			case kCommandNewProtocolSelectedSSH1:
			case kCommandNewProtocolSelectedSSH2:
			case kCommandNewProtocolSelectedTELNET:
				// a new protocol menu item was chosen
				{
					MenuItemIndex const		kItemIndex = ptr->returnProtocolMenuCommandIndex(received.commandID);
					
					
					switch (received.commandID)
					{
					case kCommandNewProtocolSelectedFTP:
						ptr->selectedProtocol = kSession_ProtocolFTP;
						break;
					
					case kCommandNewProtocolSelectedSFTP:
						ptr->selectedProtocol = kSession_ProtocolSFTP;
						break;
					
					case kCommandNewProtocolSelectedSSH1:
						ptr->selectedProtocol = kSession_ProtocolSSH1;
						break;
					
					case kCommandNewProtocolSelectedSSH2:
						ptr->selectedProtocol = kSession_ProtocolSSH2;
						break;
					
					case kCommandNewProtocolSelectedTELNET:
						ptr->selectedProtocol = kSession_ProtocolTelnet;
						break;
					
					default:
						// ???
						break;
					}
					SetControl32BitValue(HIViewWrap(idMyPopUpMenuProtocol, HIViewGetWindow(ptr->mainView)), kItemIndex);
					ptr->updatePortNumberField();
					(Boolean)ptr->updateCommandLine();
					result = noErr;
				}
				break;
			
			case kCommandLookUpSelectedHostName:
				if (ptr->lookupHostName())
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
					HIWindowRef		window = HIViewGetWindow(ptr->mainView);
					
					
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
	OSStatus						result = eventNotHandledErr;
	My_SessionsPanelResourceUI*		ptr = REINTERPRET_CAST(inMySessionsTabResourcePtr, My_SessionsPanelResourceUI*);
	UInt32 const					kEventClass = GetEventClass(inEvent);
	UInt32 const					kEventKind = GetEventKind(inEvent);
	
	
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
					HIViewWrap		fieldHostName(idMyFieldHostName, GetControlOwner(ptr->mainView));
					
					
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
