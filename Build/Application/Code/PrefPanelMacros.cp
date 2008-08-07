/*###############################################################

	PrefPanelMacros.cp
	
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

// standard-C++ includes
#include <algorithm>
#include <vector>

// Unix includes
#include <strings.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <Cursors.h>
#include <DialogAdjust.h>
#include <Embedding.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <Localization.h>
#include <ListenerModel.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
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
#include "MacroManager.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelMacros.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

UInt16 const	kMy_MaxMacroActionColumnWidthInPixels = 120; // arbitrary
UInt16 const	kMy_MaxMacroKeyColumnWidthInPixels = 100; // arbitrary

// The following cannot use any of Apple’s reserved IDs (0 to 1023).
enum
{
	kMyDataBrowserPropertyIDMacroName			= 'Name',
	kMyDataBrowserPropertyIDMacroNumber			= 'Nmbr'
};

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the NIBs from the package "PrefPanelMacros.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyUserPaneMacroSetList				= { 'MLst', 0/* ID */ };
HIViewID const	idMyDataBrowserMacroSetList				= { 'McDB', 0/* ID */ };
HIViewID const	idMySeparatorSelectedMacro				= { 'SSMc', 0/* ID */ };
HIViewID const	idMyFieldMacroName						= { 'FMNm', 0/* ID */ };
HIViewID const	idMyLabelMacroBaseKey					= { 'McKy', 0/* ID */ };
HIViewID const	idMyFieldMacroKeyCharacter				= { 'MKCh', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierCommand		= { 'McMC', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierControl		= { 'McML', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierOption		= { 'McMO', 0/* ID */ };
HIViewID const	idMyButtonInvokeWithModifierShift		= { 'McMS', 0/* ID */ };
HIViewID const	idMyPopUpMenuMacroAction				= { 'MMTy', 0/* ID */ };
HIViewID const	idMyFieldMacroText						= { 'McAc', 0/* ID */ };
HIViewID const	idMyHelpTextMacroKeys					= { 'MHlp', 0/* ID */ };

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Initializes a Data Browser callbacks structure to
point to appropriate functions in this file for
handling various tasks.  Creates the necessary UPP
types, which are destroyed when the instance goes
away.
*/
struct My_MacrosDataBrowserCallbacks
{
	My_MacrosDataBrowserCallbacks	();
	~My_MacrosDataBrowserCallbacks	();
	
	DataBrowserCallbacks	listCallbacks;
};

/*!
Implements the user interface of the panel - only
created when the panel directs for this to happen.
*/
struct My_MacrosPanelUI
{
public:
	My_MacrosPanelUI	(Panel_Ref, HIWindowRef);
	~My_MacrosPanelUI	();
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	void
	readPreferences		(Preferences_ContextRef, UInt32);
	
	static pascal OSStatus
	receiveFieldChanged		(EventHandlerCallRef, EventRef, void*);
	
	void
	refreshDisplay ();
	
	void
	saveFieldPreferences	(Preferences_ContextRef, UInt32);
	
	void
	setDataBrowserColumnWidths ();
	
	Panel_Ref							panel;						//!< the panel using this UI
	Float32								idealWidth;					//!< best size in pixels
	Float32								idealHeight;				//!< best size in pixels
	My_MacrosDataBrowserCallbacks		listCallbacks;				//!< used to provide data for the list
	HIViewWrap							mainView;

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);

private:
	CarbonEventHandlerWrap				_menuCommandsHandler;		//!< responds to menu selections
	CarbonEventHandlerWrap				_fieldNameInputHandler;		//!< saves field settings when they change
	CarbonEventHandlerWrap				_fieldContentsInputHandler;	//!< saves field settings when they change
	CommonEventHandlers_HIViewResizer	_containerResizer;			//!< invoked when the panel is resized
	ListenerModel_ListenerRef			_macroSetChangeListener;	//!< invoked when macros change externally
};
typedef My_MacrosPanelUI*	My_MacrosPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_MacrosPanelData
{
	My_MacrosPanelData ();
	
	void
	switchDataModel		(Preferences_ContextRef, UInt32);
	
	Panel_Ref				panel;			//!< the panel this data is for
	My_MacrosPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef	dataModel;		//!< source of initializations and target of changes
	UInt32					currentIndex;	//!< which index (one-based) in the data model to use for macro-specific settings
};
typedef My_MacrosPanelData*		My_MacrosPanelDataPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

pascal OSStatus		accessDataBrowserItemData			(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
														 DataBrowserItemDataRef, Boolean);
pascal Boolean		compareDataBrowserItems				(HIViewRef, DataBrowserItemID, DataBrowserItemID,
														 DataBrowserPropertyID);
void				deltaSizePanelContainerHIView		(HIViewRef, Float32, Float32, void*);
void				disposePanel						(Panel_Ref, void*);
void				macroSetChanged						(ListenerModel_Ref, ListenerModel_Event, void*, void*);
pascal void			monitorDataBrowserItems				(HIViewRef, DataBrowserItemID, DataBrowserItemNotification);
SInt32				panelChanged						(Panel_Ref, Panel_Message, void*);
pascal OSStatus		receiveHICommand					(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace



#pragma mark Public Methods

/*!
This routine creates a new preference panel for
the Macros category, initializes it, and returns
a reference to it.  You must destroy the reference
using Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.0)
*/
Panel_Ref
PrefPanelMacros_New ()
{
	Panel_Ref		result = Panel_New(My_MacrosPanelUI::panelChanged);
	
	
	if (result != nullptr)
	{
		My_MacrosPanelDataPtr	dataPtr = new My_MacrosPanelData;
		CFStringRef				nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorMacros);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelMacros);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowMacrosCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelMacrosIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelMacros);
		Panel_SetImplementation(result, dataPtr);
	}
	return result;
}// New


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_MacrosDataBrowserCallbacks structure.

(3.1)
*/
My_MacrosDataBrowserCallbacks::
My_MacrosDataBrowserCallbacks ()
{
	// set up all the callbacks needed for the data browser
	this->listCallbacks.version = kDataBrowserLatestCallbacks;
	if (noErr != InitDataBrowserCallbacks(&this->listCallbacks))
	{
		// fallback
		bzero(&this->listCallbacks, sizeof(this->listCallbacks));
		this->listCallbacks.version = kDataBrowserLatestCallbacks;
	}
	this->listCallbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(accessDataBrowserItemData);
	assert(nullptr != this->listCallbacks.u.v1.itemDataCallback);
	this->listCallbacks.u.v1.itemCompareCallback = NewDataBrowserItemCompareUPP(compareDataBrowserItems);
	assert(nullptr != this->listCallbacks.u.v1.itemCompareCallback);
	this->listCallbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(monitorDataBrowserItems);
	assert(nullptr != this->listCallbacks.u.v1.itemNotificationCallback);
}// My_MacrosDataBrowserCallbacks default constructor


/*!
Tears down a My_MacrosDataBrowserCallbacks structure.

(3.1)
*/
My_MacrosDataBrowserCallbacks::
~My_MacrosDataBrowserCallbacks ()
{
	// dispose callbacks
	if (nullptr != this->listCallbacks.u.v1.itemDataCallback)
	{
		DisposeDataBrowserItemDataUPP(this->listCallbacks.u.v1.itemDataCallback),
			this->listCallbacks.u.v1.itemDataCallback = nullptr;
	}
	if (nullptr != this->listCallbacks.u.v1.itemCompareCallback)
	{
		DisposeDataBrowserItemCompareUPP(this->listCallbacks.u.v1.itemCompareCallback),
			this->listCallbacks.u.v1.itemCompareCallback = nullptr;
	}
	if (nullptr != this->listCallbacks.u.v1.itemNotificationCallback)
	{
		DisposeDataBrowserItemNotificationUPP(this->listCallbacks.u.v1.itemNotificationCallback),
			this->listCallbacks.u.v1.itemNotificationCallback = nullptr;
	}
}// My_MacrosDataBrowserCallbacks destructor


/*!
Initializes a My_MacrosPanelData structure.

(3.1)
*/
My_MacrosPanelData::
My_MacrosPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr),
currentIndex(1)
{
}// My_MacrosPanelData default constructor


/*!
Updates the current data model and/or macro index to
the specified values.  Pass nullptr to leave the context
unchanged, and 0 to leave the index unchanged.

If the user interface is initialized, it is automatically
asked to refresh itself.

(3.1)
*/
void
My_MacrosPanelData::
switchDataModel		(Preferences_ContextRef		inNewContext,
					 UInt32						inNewIndex)
{
	if (nullptr != inNewContext) this->dataModel = inNewContext;
	if (0 != inNewIndex) this->currentIndex = inNewIndex;
	if (nullptr != this->interfacePtr)
	{
		this->interfacePtr->readPreferences(this->dataModel, this->currentIndex);
	}
}// My_MacrosPanelData::switchDataModel


/*!
Initializes a My_MacrosPanelUI structure.

(3.1)
*/
My_MacrosPanelUI::
My_MacrosPanelUI	(Panel_Ref		inPanel,
					 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel						(inPanel),
idealWidth					(0.0),
idealHeight					(0.0),
listCallbacks				(),
mainView					(createContainerView(inPanel, inOwningWindow)
								<< HIViewWrap_AssertExists),
_menuCommandsHandler		(GetWindowEventTarget(inOwningWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this/* user data */),
_fieldNameInputHandler		(GetControlEventTarget(HIViewWrap(idMyFieldMacroName, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_fieldContentsInputHandler	(GetControlEventTarget(HIViewWrap(idMyFieldMacroText, inOwningWindow)), receiveFieldChanged,
								CarbonEventSetInClass(CarbonEventClass(kEventClassTextInput), kEventTextInputUnicodeForKeyEvent),
								this/* user data */),
_containerResizer			(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
										kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
								deltaSizePanelContainerHIView, this/* context */),
_macroSetChangeListener		(nullptr)
{
	assert(_menuCommandsHandler.isInstalled());
	assert(_fieldNameInputHandler.isInstalled());
	assert(_fieldContentsInputHandler.isInstalled());
	assert(_containerResizer.isInstalled());
	
	this->setDataBrowserColumnWidths();
	
	// now that the views exist, it is safe to monitor macro activity
	this->_macroSetChangeListener = ListenerModel_NewStandardListener(macroSetChanged, this/* context */);
	assert(nullptr != this->_macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeActiveSetPlanned, this->_macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeActiveSet, this->_macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeContents, this->_macroSetChangeListener);
	Macros_StartMonitoring(kMacros_ChangeMode, this->_macroSetChangeListener);
}// My_MacrosPanelUI 2-argument constructor


/*!
Tears down a My_MacrosPanelUI structure.

(3.1)
*/
My_MacrosPanelUI::
~My_MacrosPanelUI ()
{
	// remove event handlers
	Macros_StopMonitoring(kMacros_ChangeActiveSetPlanned, this->_macroSetChangeListener);
	Macros_StopMonitoring(kMacros_ChangeActiveSet, this->_macroSetChangeListener);
	Macros_StopMonitoring(kMacros_ChangeContents, this->_macroSetChangeListener);
	Macros_StopMonitoring(kMacros_ChangeMode, this->_macroSetChangeListener);
	ListenerModel_ReleaseListener(&this->_macroSetChangeListener);
}// My_MacrosPanelUI destructor


/*!
Constructs the sub-view hierarchy for the panel,
and makes the specified container the parent.
Returns this parent.

(3.1)
*/
HIViewWrap
My_MacrosPanelUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
{
	HIViewRef					result = nullptr;
	std::vector< HIViewRef >	viewList;
	Rect						idealContainerBounds;
	OSStatus					error = noErr;
	
	
	// create the container
	{
		Rect	containerBounds;
		
		
		SetRect(&containerBounds, 0, 0, 0, 0);
		error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &result);
		assert_noerr(error);
		Panel_SetContainerView(inPanel, result);
		SetControlVisibility(result, false/* visible */, false/* draw */);
	}
	
	// create most HIViews for the panel based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
				(CFSTR("PrefPanelMacros"), CFSTR("Panel"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// create the macro list; insert appropriate columns
	{
		HIViewWrap							listDummy(idMyUserPaneMacroSetList, inOwningWindow);
		HIViewRef							macrosList = nullptr;
		DataBrowserTableViewColumnIndex		columnNumber = 0;
		DataBrowserListViewColumnDesc		columnInfo;
		Rect								bounds;
		UIStrings_Result					stringResult = kUIStrings_ResultOK;
		
		
		GetControlBounds(listDummy, &bounds);
		
		// NOTE: here, the original variable is being *replaced* with the data browser, as
		// the original user pane was only needed for size definition
		error = CreateDataBrowserControl(inOwningWindow, &bounds, kDataBrowserListView, &macrosList);
		assert_noerr(error);
		error = SetControlID(macrosList, &idMyDataBrowserMacroSetList);
		assert_noerr(error);
		
		bzero(&columnInfo, sizeof(columnInfo));
		
		// set defaults for all columns, then override below
		columnInfo.propertyDesc.propertyID = '----';
		columnInfo.propertyDesc.propertyType = kDataBrowserTextType;
		columnInfo.propertyDesc.propertyFlags = kDataBrowserDefaultPropertyFlags | kDataBrowserListViewSortableColumn |
												kDataBrowserListViewMovableColumn | kDataBrowserListViewTypeSelectColumn;
		columnInfo.headerBtnDesc.version = kDataBrowserListViewLatestHeaderDesc;
		columnInfo.headerBtnDesc.minimumWidth = 100; // arbitrary
		columnInfo.headerBtnDesc.maximumWidth = 400; // arbitrary
		columnInfo.headerBtnDesc.titleOffset = 0; // arbitrary
		columnInfo.headerBtnDesc.titleString = nullptr;
		columnInfo.headerBtnDesc.initialOrder = 0;
		columnInfo.headerBtnDesc.btnFontStyle.flags = kControlUseJustMask;
		columnInfo.headerBtnDesc.btnFontStyle.just = teFlushDefault;
		columnInfo.headerBtnDesc.btnContentInfo.contentType = kControlContentTextOnly;
		
		// create number column
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowMacrosListHeaderNumber,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDMacroNumber;
			error = AddDataBrowserListViewColumn(macrosList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// create name column
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowMacrosListHeaderName,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDMacroName;
			error = AddDataBrowserListViewColumn(macrosList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// insert as many rows as there are macros in a set
		error = AddDataBrowserItems(macrosList, kDataBrowserNoItem/* parent item */, MACRO_COUNT, nullptr/* IDs */,
									kDataBrowserItemNoProperty/* pre-sort property */);
		assert_noerr(error);
		
		// attach data that would not be specifiable in a NIB
		error = SetDataBrowserCallbacks(macrosList, &this->listCallbacks.listCallbacks);
		assert_noerr(error);
		
		// initialize sort column
		error = SetDataBrowserSortProperty(macrosList, kMyDataBrowserPropertyIDMacroNumber);
		assert_noerr(error);
		
		// set other nice things (most can be set in a NIB someday)
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		if (FlagManager_Test(kFlagOS10_4API))
		{
			(OSStatus)DataBrowserChangeAttributes(macrosList,
													FUTURE_SYMBOL(1 << 1, kDataBrowserAttributeListViewAlternatingRowColors)/* attributes to set */,
													0/* attributes to clear */);
		}
	#endif
		(OSStatus)SetDataBrowserListViewUsePlainBackground(macrosList, false);
		(OSStatus)SetDataBrowserTableViewHiliteStyle(macrosList, kDataBrowserTableViewFillHilite);
		(OSStatus)SetDataBrowserHasScrollBars(macrosList, false/* horizontal */, true/* vertical */);
		error = SetDataBrowserSelectionFlags(macrosList, kDataBrowserSelectOnlyOne | kDataBrowserNeverEmptySelectionSet);
		assert_noerr(error);
		{
			DataBrowserPropertyFlags	flags = 0;
			
			
			error = GetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDMacroName, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserPropertyIsMutable;
				error = SetDataBrowserPropertyFlags(macrosList, kMyDataBrowserPropertyIDMacroName, flags);
			}
		}
		
		// attach panel to data browser
		error = SetControlProperty(macrosList, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeOwningPanel,
									sizeof(inPanel), &inPanel);
		assert_noerr(error);
		
		error = HIViewAddSubview(result, macrosList);
		assert_noerr(error);
	}
	
	// calculate the ideal size
	this->idealWidth = idealContainerBounds.right - idealContainerBounds.left;
	this->idealHeight = idealContainerBounds.bottom - idealContainerBounds.top;
	
	// make the container match the ideal size, because the
	// size and position of NIB views is used to size subviews
	// and subview resizing deltas are derived directly from
	// changes to the container view size
	{
		HIRect		containerFrame = CGRectMake(0, 0, idealContainerBounds.right - idealContainerBounds.left,
												idealContainerBounds.bottom - idealContainerBounds.top);
		
		
		error = HIViewSetFrame(result, &containerFrame);
		assert_noerr(error);
	}
	
	return result;
}// My_MacrosPanelUI::createContainerView


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(3.0)
*/
SInt32
My_MacrosPanelUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorMacros, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			My_MacrosPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_MacrosPanelDataPtr);
			HIWindowRef const*		windowPtr = REINTERPRET_CAST(inDataPtr, HIWindowRef*);
			
			
			panelDataPtr->interfacePtr = new My_MacrosPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
			panelDataPtr->interfacePtr->setDataBrowserColumnWidths();
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			My_MacrosPanelDataPtr	panelDataPtr = REINTERPRET_CAST(inDataPtr, My_MacrosPanelDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel, panelDataPtr->currentIndex);
			disposePanel(inPanel, panelDataPtr);
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
			My_MacrosPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_MacrosPanelDataPtr);
			
			
			panelDataPtr->interfacePtr->saveFieldPreferences(panelDataPtr->dataModel, panelDataPtr->currentIndex);
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
		break;
	
	case kPanel_MessageGetGrowBoxLook: // request for panel to return its preferred appearance for the window grow box
		result = kPanel_ResponseGrowBoxOpaque;
		break;
	
	case kPanel_MessageGetIdealSize: // request for panel to return its required dimensions in pixels (after control creation)
		{
			My_MacrosPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																	My_MacrosPanelDataPtr);
			HISize&					newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
			if ((0 != panelDataPtr->interfacePtr->idealWidth) && (0 != panelDataPtr->interfacePtr->idealHeight))
			{
				newLimits.width = panelDataPtr->interfacePtr->idealWidth;
				newLimits.height = panelDataPtr->interfacePtr->idealHeight;
				result = kPanel_ResponseSizeProvided;
			}
		}
		break;
	
	case kPanel_MessageNewAppearanceTheme: // notification of theme switch, a request to recalculate control sizes
		// do nothing
		break;
	
	case kPanel_MessageNewDataSet:
		{
			My_MacrosPanelDataPtr				panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_MacrosPanelDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			
			// select first item
			{
				HIWindowRef					kOwningWindow = Panel_ReturnOwningWindow(inPanel);
				HIViewWrap					dataBrowser(idMyDataBrowserMacroSetList, kOwningWindow);
				DataBrowserItemID const		kFirstItem = 1;
				
				
				assert(dataBrowser.exists());
				(OSStatus)SetDataBrowserSelectedItems(dataBrowser, 1/* number of items */, &kFirstItem, kDataBrowserItemsAssign);
			}
			
			// update the current data model accordingly
			panelDataPtr->switchDataModel(newContext, 1/* one-based new item */);
		}
		break;
	
	case kPanel_MessageNewVisibility: // visible state of the panel’s container has changed to visible (true) or invisible (false)
		{
			//My_MacrosPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnAuxiliaryDataPtr(inPanel), My_MacrosPanelDataPtr);
			//Boolean					isNowVisible = *((Boolean*)inDataPtr);
			
			
			// hack - on pre-Mac OS 9 systems, the pesky “Edit...” buttons sticks around for some reason; explicitly show/hide it
			//SetControlVisibility(panelDataPtr->controls.editButton, isNowVisible/* visibility */, isNowVisible/* draw */);
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// My_MacrosPanelUI::panelChanged


/*!
Updates the display based on the given settings.

(3.1)
*/
void
My_MacrosPanelUI::
readPreferences		(Preferences_ContextRef		inSettings,
					 UInt32						inOneBasedIndex)
{
	assert(0 != inOneBasedIndex);
	if (nullptr != inSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		size_t					actualSize = 0;
		
		
		// INCOMPLETE
		
		// set name
		{
			CFStringRef		nameCFString = nullptr;
			
			
			prefsResult = Preferences_ContextGetDataAtIndex(inSettings, kPreferences_TagIndexedMacroName,
															inOneBasedIndex, sizeof(nameCFString), &nameCFString,
															true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				SetControlTextWithCFString(HIViewWrap(idMyFieldMacroName, kOwningWindow), nameCFString);
			}
		}
		
		// set action text
		{
			CFStringRef		actionCFString = nullptr;
			
			
			prefsResult = Preferences_ContextGetDataAtIndex(inSettings, kPreferences_TagIndexedMacroContents,
															inOneBasedIndex, sizeof(actionCFString), &actionCFString,
															true/* search defaults too */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				SetControlTextWithCFString(HIViewWrap(idMyFieldMacroText, kOwningWindow), actionCFString);
			}
		}
	}
}// My_MacrosPanelUI::readPreferences


/*!
Embellishes "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for the fields in this panel by saving
their preferences when new text arrives.

(3.1)
*/
pascal OSStatus
My_MacrosPanelUI::
receiveFieldChanged		(EventHandlerCallRef	inHandlerCallRef,
						 EventRef				inEvent,
						 void*					inMyMacrosPanelUIPtr)
{
	OSStatus			result = eventNotHandledErr;
	My_MacrosPanelUI*	interfacePtr = REINTERPRET_CAST(inMyMacrosPanelUIPtr, My_MacrosPanelUI*);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	
	// first ensure the keypress takes effect (that is, it updates
	// whatever text field it is for)
	result = CallNextEventHandler(inHandlerCallRef, inEvent);
	
	// now synchronize the post-input change with preferences
	{
		My_MacrosPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																My_MacrosPanelDataPtr);
		
		
		interfacePtr->saveFieldPreferences(panelDataPtr->dataModel, panelDataPtr->currentIndex);
	}
	
	return result;
}// My_MacrosPanelUI::receiveFieldChanged


/*!
Redraws the macro display.  Use this when you have caused
it to change in some way (by spawning an editor dialog box
or by switching tabs, etc.).

(3.0)
*/
void
My_MacrosPanelUI::
refreshDisplay ()
{
	HIViewWrap		macrosListContainer(idMyDataBrowserMacroSetList, HIViewGetWindow(this->mainView));
	assert(macrosListContainer.exists());
	
	
	(OSStatus)UpdateDataBrowserItems(macrosListContainer, kDataBrowserNoItem/* parent item */,
										0/* number of IDs */, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */,
										kMyDataBrowserPropertyIDMacroName);
}// My_MacrosPanelUI::refreshDisplay


/*!
Saves every text field in the panel to the data model,
and under the specified index for macro-specific values.

It is necessary to treat fields specially because they
do not have obvious state changes (as, say, buttons do);
they might need saving when focus is lost or the window
is closed, etc.

(3.1)
*/
void
My_MacrosPanelUI::
saveFieldPreferences	(Preferences_ContextRef		inoutSettings,
						 UInt32						inOneBasedIndex)
{
	assert(0 != inOneBasedIndex);
	if (nullptr != inoutSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		
		
		// set name
		{
			CFStringRef		nameCFString = nullptr;
			
			
			GetControlTextAsCFString(HIViewWrap(idMyFieldMacroName, kOwningWindow), nameCFString);
			
			prefsResult = Preferences_ContextSetDataAtIndex(inoutSettings, kPreferences_TagIndexedMacroName,
															inOneBasedIndex, sizeof(nameCFString), &nameCFString);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_WriteValue("warning, failed to set name of macro with index", inOneBasedIndex);
			}
		}
		
		// set key - UNIMPLEMENTED
		
		// set action text
		{
			CFStringRef		actionCFString = nullptr;
			
			
			GetControlTextAsCFString(HIViewWrap(idMyFieldMacroText, kOwningWindow), actionCFString);
			
			prefsResult = Preferences_ContextSetDataAtIndex(inoutSettings, kPreferences_TagIndexedMacroContents,
															inOneBasedIndex, sizeof(actionCFString), &actionCFString);
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_WriteValue("warning, failed to set action text of macro with index", inOneBasedIndex);
			}
		}
	}
}// My_MacrosPanelUI::saveFieldPreferences


/*!
Sets the widths of the data browser columns
proportionately based on the total width.

(3.1)
*/
void
My_MacrosPanelUI::
setDataBrowserColumnWidths ()
{
	Rect			containerRect;
	HIViewWrap		macrosListContainer(idMyDataBrowserMacroSetList, HIViewGetWindow(this->mainView));
	assert(macrosListContainer.exists());
	
	
	(Rect*)GetControlBounds(macrosListContainer, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		availableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */ - 6/* double the inset focus ring width */;
		UInt16		integerWidth = 0;
		
		
		// leave number column fixed-size
		{
			integerWidth = 42; // arbitrary
			(OSStatus)SetDataBrowserTableViewNamedColumnWidth
						(macrosListContainer, kMyDataBrowserPropertyIDMacroNumber, integerWidth);
			availableWidth -= integerWidth;
		}
		
		// give all remaining space to the title
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(macrosListContainer, kMyDataBrowserPropertyIDMacroName, availableWidth);
	}
}// My_MacrosPanelUI::setDataBrowserColumnWidths


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
pascal OSStatus
accessDataBrowserItemData	(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	OSStatus	result = noErr;
	
	
	if (false == inSetValue)
	{
		switch (inPropertyID)
		{
		case kDataBrowserItemIsEditableProperty:
			result = SetDataBrowserItemDataBooleanValue(inItemData, true/* is editable */);
			break;
		
		case kMyDataBrowserPropertyIDMacroName:
			// return the text string for the macro name
			// UNIMPLEMENTED
			break;
		
		case kMyDataBrowserPropertyIDMacroNumber:
			// return the macro number as a string
			{
				SInt32			numericalValue = STATIC_CAST(inItemID, SInt32);
				CFStringRef		numberCFString = CFStringCreateWithFormat(kCFAllocatorDefault,
																			nullptr/* options dictionary */,
																			CFSTR("%d")/* LOCALIZE THIS? */,
																			numericalValue);
				
				
				if (nullptr == numberCFString) result = memFullErr;
				else
				{
					result = SetDataBrowserItemDataText(inItemData, numberCFString);
					CFRelease(numberCFString), numberCFString = nullptr;
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	else
	{
		switch (inPropertyID)
		{
		case kMyDataBrowserPropertyIDMacroName:
			// user has changed the macro name; update the macro in memory
			{
				CFStringRef		newName = nullptr;
				
				
				result = GetDataBrowserItemDataText(inItemData, &newName);
				if (noErr == result)
				{
					// fix macro name - UNIMPLEMENTED
					result = noErr;
				}
			}
			break;
		
		case kMyDataBrowserPropertyIDMacroNumber:
			// read-only
			result = paramErr;
			break;
		
		default:
			// ???
			break;
		}
	}
	
	return result;
}// accessDataBrowserItemData


/*!
A standard DataBrowserItemCompareProcPtr, this
method compares items in the list.

(3.1)
*/
pascal Boolean
compareDataBrowserItems		(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemOne,
							 DataBrowserItemID			inItemTwo,
							 DataBrowserPropertyID		inSortProperty)
{
	Boolean		result = false;
	
	
	switch (inSortProperty)
	{
	case kMyDataBrowserPropertyIDMacroName:
		{
			CFStringRef		string1 = nullptr;
			CFStringRef		string2 = nullptr;
			
			
			// INCOMPLETE
			
			// check for nullptr, because CFStringCompare() will not deal with it
			if ((nullptr == string1) && (nullptr != string2)) result = true;
			else if ((nullptr == string1) || (nullptr == string2)) result = false;
			else
			{
				result = (kCFCompareLessThan == CFStringCompare(string1, string2,
																kCFCompareCaseInsensitive | kCFCompareLocalized/* options */));
			}
		}
		break;
	
	case kMyDataBrowserPropertyIDMacroNumber:
		result = (inItemOne < inItemTwo);
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// compareDataBrowserItems


/*!
Adjusts the views in the panel to match the specified change
in dimensions of their container.

(3.1)
*/
void
deltaSizePanelContainerHIView	(HIViewRef		inView,
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			inContext)
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		HIWindowRef const		kPanelWindow = HIViewGetWindow(inView);
		My_MacrosPanelUIPtr		interfacePtr = REINTERPRET_CAST(inContext, My_MacrosPanelUIPtr);
		HIViewWrap				viewWrap;
		
		
		viewWrap = HIViewWrap(idMyDataBrowserMacroSetList, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(0/* delta X */, inDeltaY);
		
		interfacePtr->setDataBrowserColumnWidths();
		
		viewWrap = HIViewWrap(idMySeparatorSelectedMacro, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
		viewWrap = HIViewWrap(idMyFieldMacroText, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
		viewWrap = HIViewWrap(idMyHelpTextMacroKeys, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
	}
}// deltaSizePanelContainerHIView


/*!
This routine responds to a change in the existence
of a panel.  The panel is about to be destroyed, so
this routine disposes of private data structures
associated with the specified panel.

(3.0)
*/
void
disposePanel	(Panel_Ref		UNUSED_ARGUMENT(inPanel),
				 void*			inDataPtr)
{
	My_MacrosPanelDataPtr	dataPtr = REINTERPRET_CAST(inDataPtr, My_MacrosPanelDataPtr);
	
	
	// destroy UI, if present
	if (nullptr != dataPtr->interfacePtr) delete dataPtr->interfacePtr;
	
	delete dataPtr, dataPtr = nullptr;
}// disposePanel


/*!
Invoked whenever interesting macro activity occurs.
This routine responds by updating the macro list
to be sure it displays accurate information.

(3.0)
*/
void
macroSetChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inMacrosChange,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					inMyMacrosPanelUIPtr)
{
	My_MacrosPanelUIPtr		interfacePtr = REINTERPRET_CAST(inMyMacrosPanelUIPtr, My_MacrosPanelUIPtr);
	
	
	switch (inMacrosChange)
	{
	case kMacros_ChangeActiveSetPlanned:
		// Just before the active macro set changes, save any
		// pending changes to the current one (open edits, etc.).
		// UNIMPLEMENTED
		break;
	
	case kMacros_ChangeActiveSet:
		// If the active set changes, the displayed content will
		// change also; re-sort the list (which will automatically
		// read the updated macro values), then redraw the list.
		// Also ensure the selected tab is that of the active set.
		interfacePtr->refreshDisplay();
		break;
	
	case kMacros_ChangeContents:
		// If any macro changes, the displayed content will change
		// also; re-sort the list (which will automatically read the
		// updated macro value from its set), then redraw the list.
		// NOTE: Technically, "inEventContextPtr" provides information
		//       on the particular macro that changed.  Future
		//       efficiency improvements would include not re-sorting
		//       the list if the user isn’t currently sorting
		//       according to macro content, and only refreshing the
		//       part of the display that contains the changed macro.
		interfacePtr->refreshDisplay();
		break;
	
	case kMacros_ChangeMode:
		// Obsolete; the Macro Manager will no longer have a mode, as
		// each macro will be able to have its own unique key equivalent.
		break;
	
	default:
		break;
	}
}// macroSetChanged


/*!
Responds to changes in the data browser.

(3.1)
*/
pascal void
monitorDataBrowserItems		(HIViewRef						inDataBrowser,
							 DataBrowserItemID				inItemID,
							 DataBrowserItemNotification	inMessage)
{
	My_MacrosPanelDataPtr	panelDataPtr = nullptr;
	Panel_Ref				owningPanel = nullptr;
	
	
	{
		UInt32			actualSize = 0;
		OSStatus		getPropertyError = GetControlProperty(inDataBrowser, AppResources_ReturnCreatorCode(),
																kConstantsRegistry_ControlPropertyTypeOwningPanel,
																sizeof(owningPanel), &actualSize, &owningPanel);
		
		
		if (noErr == getPropertyError)
		{
			// IMPORTANT: If this callback ever supports more than one panel, the
			// panel descriptor can be read at this point to determine the type.
			panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(owningPanel),
											My_MacrosPanelDataPtr);
		}
	}
	
	switch (inMessage)
	{
	case kDataBrowserItemSelected:
		if (nullptr != panelDataPtr)
		{
			DataBrowserTableViewRowIndex	rowIndex = 0;
			OSStatus						error = noErr;
			
			
			// update the “selected macro” fields to match the newly-selected item
			error = GetDataBrowserTableViewItemRow(inDataBrowser, inItemID, &rowIndex);
			if (noErr == error)
			{
				panelDataPtr->switchDataModel(nullptr/* context */, 1 + rowIndex/* convert from zero-based to one-based */);
			}
			else
			{
				Console_WriteLine("warning, unexpected problem determining selected macro!");
				Sound_StandardAlert();
			}
		}
		break;
	
	case kDataBrowserEditStopped:
		// it seems to be possible for the I-beam to persist at times
		// unless the cursor is explicitly reset here
		Cursors_UseArrow();
		break;
	
	default:
		// not all messages are supported
		break;
	}
}// monitorDataBrowserItems


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons and menus in this panel.

(3.1)
*/
pascal OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyMacrosPanelUIPtr)
{
	OSStatus			result = eventNotHandledErr;
	My_MacrosPanelUI*	interfacePtr = REINTERPRET_CAST(inMyMacrosPanelUIPtr, My_MacrosPanelUI*);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
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
