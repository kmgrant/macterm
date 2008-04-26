/*###############################################################

	PrefPanelTerminals.cp
	
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
#include "GenericPanelTabs.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelTerminals.h"
#include "Terminal.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIBs from the package "PrefPanelTerminals.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyPopUpMenuEmulationType		= { 'MEmT', 0/* ID */ };
HIViewID const	idMyFieldAnswerBackMessage		= { 'EABM', 0/* ID */ };
HIViewID const	idMyDataBrowserHacks			= { 'HxDB', 0/* ID */ };
HIViewID const	idMyFieldColumns				= { 'Cols', 0/* ID */ };
HIViewID const	idMyFieldRows					= { 'Rows', 0/* ID */ };
HIViewID const	idMyPopUpMenuScrollbackType		= { 'SbkT', 0/* ID */ };
HIViewID const	idMyFieldScrollback				= { 'Sbak', 0/* ID */ };
HIViewID const	idMyPopUpMenuScrollbackUnits	= { 'SbkU', 0/* ID */ };
HIViewID const	idMySliderScrollSpeed			= { 'SSpd', 0/* ID */ };
HIViewID const	idMyLabelScrollSpeedFast		= { 'LScF', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Implements the “Emulation” tab.
*/
struct My_TerminalsPanelEmulationUI
{
	My_TerminalsPanelEmulationUI	(Panel_Ref, HIWindowRef);
	
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
typedef My_TerminalsPanelEmulationUI*	My_TerminalsPanelEmulationUIPtr;

/*!
Implements the “Options” tab.
*/
struct My_TerminalsPanelOptionsUI
{
	My_TerminalsPanelOptionsUI	(Panel_Ref, HIWindowRef);
	
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
typedef My_TerminalsPanelOptionsUI*		My_TerminalsPanelOptionsUIPtr;

/*!
Implements the “Screen” tab.
*/
struct My_TerminalsPanelScreenUI
{
	My_TerminalsPanelScreenUI	(Panel_Ref, HIWindowRef);
	
	Panel_Ref		panel;			//!< the panel using this UI
	Float32			idealWidth;		//!< best size in pixels
	Float32			idealHeight;	//!< best size in pixels
	HIViewWrap		mainView;
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef);
	
	static pascal OSStatus
	receiveFieldChanged		(EventHandlerCallRef, EventRef, void*);
	
	void
	saveFieldPreferences	(Preferences_ContextRef);
	
	void
	setColumns		(UInt16);
	
	void
	setRows		(UInt16);
	
	void
	setScrollbackCustomizationEnabled	(Boolean);
	
	void
	setScrollbackRows	(UInt16);
	
	void
	setScrollbackType	(Terminal_ScrollbackType);

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);
	
	static void
	deltaSize	(HIViewRef, Float32, Float32, void*);
	
private:
	CarbonEventHandlerWrap				_menuCommandsHandler;			//!< responds to menu selections
	CarbonEventHandlerWrap				_fieldColumnsInputHandler;		//!< saves field settings when they change
	CarbonEventHandlerWrap				_fieldRowsInputHandler;			//!< saves field settings when they change
	CarbonEventHandlerWrap				_fieldScrollbackInputHandler;	//!< saves field settings when they change
	CommonEventHandlers_HIViewResizer	_containerResizer;
};
typedef My_TerminalsPanelScreenUI*	My_TerminalsPanelScreenUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_TerminalsPanelEmulationData
{
	My_TerminalsPanelEmulationData	();
	~My_TerminalsPanelEmulationData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_TerminalsPanelEmulationUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_TerminalsPanelEmulationData*		My_TerminalsPanelEmulationDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_TerminalsPanelOptionsData
{
	My_TerminalsPanelOptionsData	();
	~My_TerminalsPanelOptionsData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_TerminalsPanelOptionsUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_TerminalsPanelOptionsData*		My_TerminalsPanelOptionsDataPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_TerminalsPanelScreenData
{
	My_TerminalsPanelScreenData		();
	~My_TerminalsPanelScreenData	();
	
	Panel_Ref						panel;			//!< the panel this data is for
	My_TerminalsPanelScreenUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef			dataModel;		//!< source of initializations and target of changes
};
typedef My_TerminalsPanelScreenData*		My_TerminalsPanelScreenDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

pascal OSStatus		receiveHICommand			(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new preference panel for the Terminals category.
Destroy it using Panel_Dispose().

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelTerminals_New ()
{
	Panel_Ref				result = nullptr;
	CFStringRef				nameCFString = nullptr;
	GenericPanelTabs_List	tabList;
	
	
	tabList.push_back(PrefPanelTerminals_NewOptionsPane());
	tabList.push_back(PrefPanelTerminals_NewEmulationPane());
	tabList.push_back(PrefPanelTerminals_NewScreenPane());
	
	if (UIStrings_Copy(kUIStrings_PreferencesWindowTerminalsCategoryName, nameCFString).ok())
	{
		result = GenericPanelTabs_New(nameCFString, kConstantsRegistry_PrefPanelDescriptorTerminals, tabList);
		if (nullptr != result)
		{
			Panel_SetShowCommandID(result, kCommandDisplayPrefPanelTerminals);
			Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelTerminalsIconFilenameNoExtension(),
											AppResources_ReturnCreatorCode(),
											kConstantsRegistry_IconServicesIconPrefPanelTerminals);
		}
		CFRelease(nameCFString), nameCFString = nullptr;
	}
	
	return result;
}// New


/*!
Creates only the Emulation pane, which allows the user to
set the terminal type.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelTerminals_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelTerminals_NewEmulationPane ()
{
	Panel_Ref	result = Panel_New(My_TerminalsPanelEmulationUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_TerminalsPanelEmulationDataPtr	dataPtr = new My_TerminalsPanelEmulationData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorTerminalsEmulation);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelTerminalsEmulation);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowTerminalsEmulationTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelTerminalsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelTerminals);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewEmulationPane


/*!
Creates only the Options pane, which allows the user to
set various checkboxes.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelTerminals_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelTerminals_NewOptionsPane ()
{
	Panel_Ref	result = Panel_New(My_TerminalsPanelOptionsUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_TerminalsPanelOptionsDataPtr		dataPtr = new My_TerminalsPanelOptionsData();
		CFStringRef							nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorTerminalsOptions);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelTerminalsOptions);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowTerminalsOptionsTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelTerminalsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelTerminals);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewOptionsPane


/*!
Creates only the Screen pane, which allows the user to set
the size and scrollback.  Destroy it using Panel_Dispose().

You only use this routine if you need to display the pane
alone, outside its usual tabbed interface.  To create the
complete, multi-pane interface, use PrefPanelTerminals_New().

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelTerminals_NewScreenPane ()
{
	Panel_Ref	result = Panel_New(My_TerminalsPanelScreenUI::panelChanged);
	
	
	if (nullptr != result)
	{
		My_TerminalsPanelScreenDataPtr	dataPtr = new My_TerminalsPanelScreenData();
		CFStringRef						nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorTerminalsScreen);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelTerminalsScreen);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowTerminalsScreenTabName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		// NOTE: This panel is currently not used in interfaces that rely on icons, so its icon is not unique.
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelTerminalsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelTerminals);
		Panel_SetImplementation(result, dataPtr);
		dataPtr->panel = result;
	}
	return result;
}// NewScreenPane


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_TerminalsPanelEmulationData structure.

(3.1)
*/
My_TerminalsPanelEmulationData::
My_TerminalsPanelEmulationData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_TerminalsPanelEmulationData default constructor


/*!
Tears down a My_TerminalsPanelEmulationData structure.

(3.1)
*/
My_TerminalsPanelEmulationData::
~My_TerminalsPanelEmulationData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_TerminalsPanelEmulationData destructor


/*!
Initializes a My_TerminalsPanelEmulationUI structure.

(3.1)
*/
My_TerminalsPanelEmulationUI::
My_TerminalsPanelEmulationUI	(Panel_Ref		inPanel,
								 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_TerminalsPanelEmulationUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
}// My_TerminalsPanelEmulationUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_TerminalsPanelEmulationUI::
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
			(CFSTR("PrefPanelTerminals"), CFSTR("Emulation"), inOwningWindow,
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
}// My_TerminalsPanelEmulationUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_TerminalsPanelEmulationUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const			kPanelWindow = HIViewGetWindow(inContainer);
	//My_TerminalsPanelEmulationUI*	dataPtr = REINTERPRET_CAST(inContext, My_TerminalsPanelEmulationUI*);
	
	HIViewWrap					viewWrap;
	
	
	// INCOMPLETE
	viewWrap = HIViewWrap(idMyPopUpMenuEmulationType, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyFieldAnswerBackMessage, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	viewWrap = HIViewWrap(idMyDataBrowserHacks, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_TerminalsPanelEmulationUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_TerminalsPanelEmulationUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorTerminalsEmulation, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_TerminalsPanelEmulationDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelEmulationDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_TerminalsPanelEmulationUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_TerminalsPanelEmulationDataPtr));
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
			My_TerminalsPanelEmulationDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelEmulationDataPtr);
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
			My_TerminalsPanelEmulationDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelEmulationDataPtr);
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
}// My_TerminalsPanelEmulationUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_TerminalsPanelEmulationUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// UNIMPLEMENTED
	}
}// My_TerminalsPanelEmulationUI::readPreferences


/*!
Initializes a My_TerminalsPanelOptionsData structure.

(3.1)
*/
My_TerminalsPanelOptionsData::
My_TerminalsPanelOptionsData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_TerminalsPanelOptionsData default constructor


/*!
Tears down a My_TerminalsPanelOptionsData structure.

(3.1)
*/
My_TerminalsPanelOptionsData::
~My_TerminalsPanelOptionsData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_TerminalsPanelOptionsData destructor


/*!
Initializes a My_TerminalsPanelOptionsUI structure.

(3.1)
*/
My_TerminalsPanelOptionsUI::
My_TerminalsPanelOptionsUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel					(inPanel),
idealWidth				(0.0),
idealHeight				(0.0),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
_containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
							My_TerminalsPanelOptionsUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_containerResizer.isInstalled());
}// My_TerminalsPanelOptionsUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_TerminalsPanelOptionsUI::
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
			(CFSTR("PrefPanelTerminals"), CFSTR("Options"), inOwningWindow,
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
}// My_TerminalsPanelOptionsUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_TerminalsPanelOptionsUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const			kPanelWindow = HIViewGetWindow(inContainer);
	//My_TerminalsPanelOptionsUI*	dataPtr = REINTERPRET_CAST(inContext, My_TerminalsPanelOptionsUI*);
	
	HIViewWrap					viewWrap;
	
	
	// UNIMPLEMENTED
}// My_TerminalsPanelOptionsUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_TerminalsPanelOptionsUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorTerminalsOptions, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_TerminalsPanelOptionsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelOptionsDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_TerminalsPanelOptionsUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_TerminalsPanelOptionsDataPtr));
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
			My_TerminalsPanelOptionsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelOptionsDataPtr);
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
			My_TerminalsPanelOptionsDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelOptionsDataPtr);
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
}// My_TerminalsPanelOptionsUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_TerminalsPanelOptionsUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		// UNIMPLEMENTED
	}
}// My_TerminalsPanelOptionsUI::readPreferences


/*!
Initializes a My_TerminalsPanelScreenData structure.

(3.1)
*/
My_TerminalsPanelScreenData::
My_TerminalsPanelScreenData ()
:
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr)
{
}// My_TerminalsPanelScreenData default constructor


/*!
Tears down a My_TerminalsPanelScreenData structure.

(3.1)
*/
My_TerminalsPanelScreenData::
~My_TerminalsPanelScreenData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_TerminalsPanelScreenData destructor


/*!
Initializes a My_TerminalsPanelScreenUI structure.

(3.1)
*/
My_TerminalsPanelScreenUI::
My_TerminalsPanelScreenUI	(Panel_Ref		inPanel,
							 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel						(inPanel),
idealWidth					(0.0),
idealHeight					(0.0),
mainView					(createContainerView(inPanel, inOwningWindow)
								<< HIViewWrap_AssertExists),
_menuCommandsHandler		(GetWindowEventTarget(inOwningWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this/* user data */),
_fieldColumnsInputHandler	(GetControlEventTarget(HIViewWrap(idMyFieldColumns, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_fieldRowsInputHandler		(GetControlEventTarget(HIViewWrap(idMyFieldRows, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_fieldScrollbackInputHandler(GetControlEventTarget(HIViewWrap(idMyFieldScrollback, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_containerResizer			(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH,
								My_TerminalsPanelScreenUI::deltaSize, this/* context */)
{
	assert(this->mainView.exists());
	assert(_menuCommandsHandler.isInstalled());
	assert(_fieldColumnsInputHandler.isInstalled());
	assert(_fieldRowsInputHandler.isInstalled());
	assert(_fieldScrollbackInputHandler.isInstalled());
	assert(_containerResizer.isInstalled());
}// My_TerminalsPanelScreenUI 2-argument constructor


/*!
Constructs the HIView that resides within the tab, and
the sub-views that belong in its hierarchy.

(3.1)
*/
HIViewWrap
My_TerminalsPanelScreenUI::
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
			(CFSTR("PrefPanelTerminals"), CFSTR("Screen"), inOwningWindow,
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
}// My_TerminalsPanelScreenUI::createContainerView


/*!
Resizes the views in this tab.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
deltaSize	(HIViewRef		inContainer,
			 Float32		inDeltaX,
			 Float32		UNUSED_ARGUMENT(inDeltaY),
			 void*			UNUSED_ARGUMENT(inContext))
{
	HIWindowRef const			kPanelWindow = HIViewGetWindow(inContainer);
	//My_TerminalsPanelScreenUI*	dataPtr = REINTERPRET_CAST(inContext, My_TerminalsPanelScreenUI*);
	
	HIViewWrap					viewWrap;
	
	
	// INCOMPLETE
	//viewWrap = HIViewWrap(idMySliderScrollSpeed, kPanelWindow);
	//viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	//viewWrap = HIViewWrap(idMyLabelScrollSpeedFast, kPanelWindow);
	//viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
}// My_TerminalsPanelScreenUI::deltaSize


/*!
Invoked when the state of a panel changes, or information
about the panel is required.  (This routine is of type
PanelChangedProcPtr.)

(3.1)
*/
SInt32
My_TerminalsPanelScreenUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorTerminalsScreen, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create views using this window
		{
			My_TerminalsPanelScreenDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelScreenDataPtr);
			WindowRef const*					windowPtr = REINTERPRET_CAST(inDataPtr, WindowRef*);
			
			
			// create the rest of the panel user interface
			panelDataPtr->interfacePtr = new My_TerminalsPanelScreenUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_TerminalsPanelScreenDataPtr		panelDataPtr = REINTERPRET_CAST(inDataPtr, My_TerminalsPanelScreenDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
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
			//HIViewRef const*					viewPtr = REINTERPRET_CAST(inDataPtr, HIViewRef*);
			My_TerminalsPanelScreenDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_TerminalsPanelScreenDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after view creation)
		{
			My_TerminalsPanelScreenDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelScreenDataPtr);
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
			My_TerminalsPanelScreenDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_TerminalsPanelScreenDataPtr);
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
}// My_TerminalsPanelScreenUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
readPreferences		(Preferences_ContextRef		inSettings)
{
	if (nullptr != inSettings)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// INCOMPLETE
		
		// set columns
		{
			UInt16		dimension = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalScreenColumns, sizeof(dimension),
														&dimension, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setColumns(dimension);
			}
		}
		
		// set rows
		{
			UInt16		dimension = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalScreenRows, sizeof(dimension),
														&dimension, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setRows(dimension);
			}
		}
		
		// set scrollback rows
		{
			UInt16		dimension = 0;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalScreenScrollbackRows, sizeof(dimension),
														&dimension, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setScrollbackRows(dimension);
			}
		}
		
		// set scrollback type
		{
			Terminal_ScrollbackType		allocationRule = kTerminal_ScrollbackTypeFixed;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagTerminalScreenScrollbackType, sizeof(allocationRule),
														&allocationRule, true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setScrollbackType(allocationRule);
			}
		}
	}
}// My_TerminalsPanelScreenUI::readPreferences


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the fields in this panel by saving
their preferences when new text arrives.

(3.1)
*/
pascal OSStatus
My_TerminalsPanelScreenUI::
receiveFieldChanged		(EventHandlerCallRef	inHandlerCallRef,
						 EventRef				inEvent,
						 void*					inMyTerminalsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_TerminalsPanelScreenUI*	screenInterfacePtr = REINTERPRET_CAST(inMyTerminalsPanelUIPtr, My_TerminalsPanelScreenUI*);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with preferences
	{
		My_TerminalsPanelScreenDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(screenInterfacePtr->panel),
																			My_TerminalsPanelScreenDataPtr);
		
		
		screenInterfacePtr->saveFieldPreferences(panelDataPtr->dataModel);
	}
	
	return result;
}// My_TerminalsPanelScreenUI::receiveFieldChanged


/*!
Saves every text field in the panel to the data model.
It is necessary to treat fields specially because they
do not have obvious state changes (as, say, buttons do);
they might need saving when focus is lost or the window
is closed, etc.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
saveFieldPreferences	(Preferences_ContextRef		inoutSettings)
{
	if (nullptr != inoutSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		SInt32					dummyInteger = 0;
		
		
		// set columns
		{
			UInt16		dimension = 0;
			
			
			GetControlNumericalText(HIViewWrap(idMyFieldColumns, kOwningWindow), &dummyInteger);
			dimension = STATIC_CAST(dummyInteger, UInt16);
			
			prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagTerminalScreenColumns,
														sizeof(dimension), &dimension);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_WriteLine("warning, failed to set screen columns");
			}
		}
		
		// set rows
		{
			UInt16		dimension = 0;
			
			
			GetControlNumericalText(HIViewWrap(idMyFieldRows, kOwningWindow), &dummyInteger);
			dimension = STATIC_CAST(dummyInteger, UInt16);
			
			prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagTerminalScreenRows,
														sizeof(dimension), &dimension);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_WriteLine("warning, failed to set screen rows");
			}
		}
		
		// set scrollback rows
		{
			UInt16		dimension = 0;
			
			
			// INCOMPLETE - take the current units into account!!!
			GetControlNumericalText(HIViewWrap(idMyFieldScrollback, kOwningWindow), &dummyInteger);
			dimension = STATIC_CAST(dummyInteger, UInt16);
			
			prefsResult = Preferences_ContextSetData(inoutSettings, kPreferences_TagTerminalScreenScrollbackRows,
														sizeof(dimension), &dimension);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_WriteLine("warning, failed to set screen scrollback");
			}
		}
	}
}// My_TerminalsPanelScreenUI::saveFieldPreferences


/*!
Updates the columns display based on the given setting.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
setColumns		(UInt16		inDimension)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControlNumericalText(HIViewWrap(idMyFieldColumns, kOwningWindow), inDimension);
}// My_TerminalsPanelScreenUI::setColumns


/*!
Updates the rows display based on the given setting.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
setRows		(UInt16		inDimension)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	SetControlNumericalText(HIViewWrap(idMyFieldRows, kOwningWindow), inDimension);
}// My_TerminalsPanelScreenUI::setRows


/*!
Enables or disables the views that handle customizing
the scrollback value.  These views are not used for
anything but fixed-size scrollbacks.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
setScrollbackCustomizationEnabled	(Boolean	inIsEnabled)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	HIViewWrap			viewWrap;
	
	
	viewWrap = HIViewWrap(idMyFieldScrollback, kOwningWindow);
	viewWrap << HIViewWrap_SetActiveState(inIsEnabled);
	viewWrap = HIViewWrap(idMyPopUpMenuScrollbackUnits, kOwningWindow);
	viewWrap << HIViewWrap_SetActiveState(inIsEnabled);
}// My_TerminalsPanelScreenUI::setScrollbackCustomizationEnabled


/*!
Updates the scrollback display based on the given setting
in units of rows.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
setScrollbackRows	(UInt16		inDimension)
{
	HIWindowRef const	kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	
	
	this->setScrollbackType(kTerminal_ScrollbackTypeFixed);
	
	SetControlNumericalText(HIViewWrap(idMyFieldScrollback, kOwningWindow), inDimension);
	(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackUnits, kOwningWindow),
													kCommandSetScrollbackUnitsRows);
}// My_TerminalsPanelScreenUI::setScrollbackRows


/*!
Updates the scrollback display based on the given type.

(3.1)
*/
void
My_TerminalsPanelScreenUI::
setScrollbackType	(Terminal_ScrollbackType	inAllocationRule)
{
	HIWindowRef const				kOwningWindow = Panel_ReturnOwningWindow(this->panel);
	My_TerminalsPanelScreenDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(this->panel),
																	My_TerminalsPanelScreenDataPtr);
	Preferences_Result				prefsResult = kPreferences_ResultOK;
	
	
	switch (inAllocationRule)
	{
	case kTerminal_ScrollbackTypeDisabled:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
														kCommandSetScrollbackTypeDisabled);
		this->setScrollbackCustomizationEnabled(false);
		break;
	
	case kTerminal_ScrollbackTypeUnlimited:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
														kCommandSetScrollbackTypeUnlimited);
		this->setScrollbackCustomizationEnabled(false);
		break;
	
	case kTerminal_ScrollbackTypeDistributed:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
														kCommandSetScrollbackTypeDistributed);
		this->setScrollbackCustomizationEnabled(false);
		break;
	
	case kTerminal_ScrollbackTypeFixed:
	default:
		(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackType, kOwningWindow),
														kCommandSetScrollbackTypeFixed);
		this->setScrollbackCustomizationEnabled(true);
		break;
	}
	
	prefsResult = Preferences_ContextSetData(panelDataPtr->dataModel, kPreferences_TagTerminalScreenScrollbackType,
												sizeof(inAllocationRule), &inAllocationRule);
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_WriteLine("warning, failed to set screen scrollback type");
	}
}// My_TerminalsPanelScreenUI::setScrollbackType


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons and menus in this panel.

(3.1)
*/
pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyTerminalsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	// WARNING: More than one UI uses this handler.  The context will
	// depend on the command ID.
	My_TerminalsPanelScreenUI*	screenInterfacePtr = REINTERPRET_CAST(inMyTerminalsPanelUIPtr, My_TerminalsPanelScreenUI*);
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
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandSetScrollbackTypeDisabled:
				screenInterfacePtr->setScrollbackType(kTerminal_ScrollbackTypeDisabled);
				result = noErr; // event is handled
				break;
			
			case kCommandSetScrollbackTypeFixed:
				screenInterfacePtr->setScrollbackType(kTerminal_ScrollbackTypeFixed);
				result = noErr; // event is handled
				break;
			
			case kCommandSetScrollbackTypeUnlimited:
				screenInterfacePtr->setScrollbackType(kTerminal_ScrollbackTypeUnlimited);
				result = noErr; // event is handled
				break;
			
			case kCommandSetScrollbackTypeDistributed:
				screenInterfacePtr->setScrollbackType(kTerminal_ScrollbackTypeDistributed);
				result = noErr; // event is handled
				break;
			
			case kCommandSetScrollbackUnitsRows:
				// INCOMPLETE
				(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackUnits,
																			Panel_ReturnOwningWindow(screenInterfacePtr->panel)),
																received.commandID);
				result = noErr; // event is handled
				break;
			
			case kCommandSetScrollbackUnitsKilobytes:
				// UNIMPLEMENTED
				Sound_StandardAlert();
			#if 0
				(OSStatus)DialogUtilities_SetPopUpItemByCommand(HIViewWrap(idMyPopUpMenuScrollbackUnits,
																			Panel_ReturnOwningWindow(screenInterfacePtr->panel)),
																received.commandID);
			#endif
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
