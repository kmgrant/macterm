/*!	\file PrefPanelWorkspaces.mm
	\brief Implements the Workspaces panel of Preferences.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#import "PrefPanelWorkspaces.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstring>

// standard-C++ includes
#import <algorithm>
#import <vector>

// Unix includes
#import <strings.h>

// Mac includes
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>
#import <objc/objc-runtime.h>

// library includes
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaExtensions.objc++.h>
#import <CommonEventHandlers.h>
#import <Console.h>
#import <HIViewWrap.h>
#import <HIViewWrapManip.h>
#import <IconManager.h>
#import <Localization.h>
#import <ListenerModel.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <SoundSystem.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "DialogUtilities.h"
#import "HelpSystem.h"
#import "Keypads.h"
#import "Panel.h"
#import "Preferences.h"
#import "UIStrings.h"
#import "UIStrings_PrefsWindow.h"



#pragma mark Constants
namespace {

// The following cannot use any of Apple’s reserved IDs (0 to 1023).
enum
{
	kMyDataBrowserPropertyIDWindowName			= 'Name',
	kMyDataBrowserPropertyIDWindowNumber		= 'Nmbr'
};

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the NIBs from the package "PrefPanelWorkspaces.nib".

In addition, they MUST be unique across all panels.
*/
HIViewID const	idMyUserPaneWindowList					= { 'WLst', 0/* ID */ };
HIViewID const	idMyDataBrowserWindowList				= { 'WnDB', 0/* ID */ };
HIViewID const	idMySeparatorSelectedWindow				= { 'SSWn', 0/* ID */ };
HIViewID const	idMyPopUpMenuSession					= { 'WSss', 0/* ID */ };
HIViewID const	idMyButtonSetWindowBoundaries			= { 'WSBn', 0/* ID */ };
HIViewID const	idMySeparatorGlobalSettings				= { 'SSWG', 0/* ID */ };
HIViewID const	idMyLabelOptions						= { 'LWSO', 0/* ID */ };
HIViewID const	idMyCheckBoxUseTabsToArrangeWindows		= { 'UTAW', 0/* ID */ };
HIViewID const	idMyCheckBoxAutoFullScreen				= { 'AuFS', 0/* ID */ };

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
struct My_WindowsDataBrowserCallbacks
{
	My_WindowsDataBrowserCallbacks	();
	~My_WindowsDataBrowserCallbacks	();
	
	DataBrowserCallbacks	listCallbacks;
};

/*!
Implements the user interface of the panel - only
created when the panel directs for this to happen.
*/
struct My_WorkspacesPanelUI
{
public:
	My_WorkspacesPanelUI	(Panel_Ref, HIWindowRef);
	~My_WorkspacesPanelUI	();
	
	static SInt32
	panelChanged	(Panel_Ref, Panel_Message, void*);
	
	static void
	preferenceChanged	(ListenerModel_Ref, ListenerModel_Event, void*, void*);
	
	void
	readPreferences		(Preferences_ContextRef, Preferences_Index);
	
	void
	rebuildFavoritesMenu	(HIViewID const&, UInt32, Quills::Prefs::Class,
							 UInt32, MenuItemIndex&);
	
	void
	rebuildSessionMenu ();
	
	static OSStatus
	receiveHICommand	(EventHandlerCallRef, EventRef, void*);
	
	void
	refreshDisplay ();
	
	void
	setAssociatedSession	(CFStringRef);
	
	void
	setAssociatedSessionByType	(UInt32);
	
	void
	setDataBrowserColumnWidths ();
	
	Panel_Ref							panel;						//!< the panel using this UI
	Float32								idealWidth;					//!< best size in pixels
	Float32								idealHeight;				//!< best size in pixels
	My_WindowsDataBrowserCallbacks		listCallbacks;				//!< used to provide data for the list
	HIViewWrap							mainView;

protected:
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef);

private:
	MenuItemIndex						_numberOfSessionItemsAdded;	//!< used to manage Session pop-up menu
	CarbonEventHandlerWrap				_menuCommandsHandler;		//!< responds to menu selections
	CommonEventHandlers_HIViewResizer	_containerResizer;			//!< invoked when the panel is resized
	ListenerModel_ListenerWrap			_whenFavoritesChangedHandler;	//!< used to manage Session pop-up menu
};
typedef My_WorkspacesPanelUI*		My_WorkspacesPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_WorkspacesPanelData
{
	My_WorkspacesPanelData ();
	
	~My_WorkspacesPanelData ();
	
	void
	switchDataModel		(Preferences_ContextRef, Preferences_Index);
	
	Panel_Ref				panel;			//!< the panel this data is for
	My_WorkspacesPanelUI*	interfacePtr;	//!< if not nullptr, the panel user interface is active
	Preferences_ContextRef	dataModel;		//!< source of initializations and target of changes
	Preferences_Index		currentIndex;	//!< which index (one-based) in the data model to use for window-specific settings
};
typedef My_WorkspacesPanelData*		My_WorkspacesPanelDataPtr;

} // anonymous namespace


/*!
Private properties.
*/
@interface PrefPanelWorkspaces_WindowsViewManager () //{

// accessors
	@property (strong) PrefPanelWorkspaces_WindowEditorViewManager*
	windowEditorViewManager;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelWorkspaces_OptionsViewManager (PrefPanelWorkspaces_OptionsViewManagerInternal) //{

// accessors
	@property (readonly) NSArray*
	primaryDisplayBindingKeys;

@end //}


/*!
Private properties.
*/
@interface PrefPanelWorkspaces_WindowEditorViewManager () //{

// accessors
	@property (weak, readonly) PrefPanelWorkspaces_WindowsViewManager*
	windowsViewManager;

@end //}


/*!
The private class interface.
*/
@interface PrefPanelWorkspaces_WindowEditorViewManager (PrefPanelWorkspaces_WindowEditorViewManagerInternal) //{

// accessors
	@property (readonly) NSArray*
	primaryDisplayBindingKeys;

@end //}

#pragma mark Internal Method Prototypes
namespace {

OSStatus			accessDataBrowserItemData			(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
														 DataBrowserItemDataRef, Boolean);
Boolean				compareDataBrowserItems				(HIViewRef, DataBrowserItemID, DataBrowserItemID,
														 DataBrowserPropertyID);
void				deltaSizePanelContainerHIView		(HIViewRef, Float32, Float32, void*);
void				monitorDataBrowserItems				(HIViewRef, DataBrowserItemID, DataBrowserItemNotification);

} // anonymous namespace



#pragma mark Public Methods

/*!
This routine creates a new preference panel for the
Workspaces category, initializes it, and returns a
reference to it.  You must destroy the reference using
Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(4.0)
*/
Panel_Ref
PrefPanelWorkspaces_New ()
{
	Panel_Ref		result = Panel_New(My_WorkspacesPanelUI::panelChanged);
	
	
	if (result != nullptr)
	{
		My_WorkspacesPanelDataPtr	dataPtr = new My_WorkspacesPanelData;
		CFStringRef					nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorWorkspaces);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelWorkspaces);
		if (UIStrings_Copy(kUIStrings_PrefPanelWorkspacesCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelWorkspacesIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelWorkspaces);
		Panel_SetImplementation(result, dataPtr);
	}
	return result;
}// New


/*!
Creates a tag set that can be used with Preferences APIs to
filter settings (e.g. via Preferences_ContextCopy()).

The resulting set contains every tag that is possible to change
using this user interface.

Call Preferences_ReleaseTagSet() when finished with the set.

(4.1)
*/
Preferences_TagSetRef
PrefPanelWorkspaces_NewTagSet ()
{
	Preferences_TagSetRef			result = nullptr;
	std::vector< Preferences_Tag >	tagList;
	
	
	// IMPORTANT: this list should be in sync with everything in this file
	// that reads preferences from the context of a data set
	for (UInt16 i = 1; i <= kPreferences_MaximumWorkspaceSize; ++i)
	{
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowSessionFavorite, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowCommandType, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowFrameBounds, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowScreenBounds, i));
		tagList.push_back(Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowTitle, i));
	}
	tagList.push_back(kPreferences_TagArrangeWindowsUsingTabs);
	tagList.push_back(kPreferences_TagArrangeWindowsFullScreen);
	
	result = Preferences_NewTagSet(tagList);
	
	return result;
}// NewTagSet


#pragma mark Internal Methods
namespace {

/*!
Initializes a My_WindowsDataBrowserCallbacks structure.

(4.0)
*/
My_WindowsDataBrowserCallbacks::
My_WindowsDataBrowserCallbacks ()
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
}// My_WindowsDataBrowserCallbacks default constructor


/*!
Tears down a My_WindowsDataBrowserCallbacks structure.

(4.0)
*/
My_WindowsDataBrowserCallbacks::
~My_WindowsDataBrowserCallbacks ()
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
}// My_WindowsDataBrowserCallbacks destructor


/*!
Initializes a My_WorkspacesPanelData structure.

(4.0)
*/
My_WorkspacesPanelData::
My_WorkspacesPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr),
dataModel(nullptr),
currentIndex(1)
{
}// My_WorkspacesPanelData default constructor


/*!
Tears down a My_WorkspacesPanelData structure.

(4.0)
*/
My_WorkspacesPanelData::
~My_WorkspacesPanelData ()
{
	if (nullptr != this->interfacePtr) delete this->interfacePtr;
}// My_WorkspacesPanelData destructor


/*!
Updates the current data model and/or window index to
the specified values.  Pass nullptr to leave the context
unchanged, and 0 to leave the index unchanged.

If the user interface is initialized, it is automatically
asked to refresh itself.

(4.0)
*/
void
My_WorkspacesPanelData::
switchDataModel		(Preferences_ContextRef		inNewContext,
					 Preferences_Index			inNewIndex)
{
	if (nullptr != inNewContext) this->dataModel = inNewContext;
	if (0 != inNewIndex) this->currentIndex = inNewIndex;
	if (nullptr != this->interfacePtr)
	{
		this->interfacePtr->refreshDisplay();
		this->interfacePtr->readPreferences(this->dataModel, this->currentIndex);
	}
}// My_WorkspacesPanelData::switchDataModel


/*!
Initializes a My_WorkspacesPanelUI structure.

(4.0)
*/
My_WorkspacesPanelUI::
My_WorkspacesPanelUI	(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel						(inPanel),
idealWidth					(0.0),
idealHeight					(0.0),
listCallbacks				(),
mainView					(createContainerView(inPanel, inOwningWindow)
								<< HIViewWrap_AssertExists),
_numberOfSessionItemsAdded	(0),
_menuCommandsHandler		(GetWindowEventTarget(inOwningWindow), receiveHICommand,
								CarbonEventSetInClass(CarbonEventClass(kEventClassCommand), kEventCommandProcess),
								this/* user data */),
_containerResizer			(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
										kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
								deltaSizePanelContainerHIView, this/* context */),
_whenFavoritesChangedHandler(ListenerModel_NewStandardListener(preferenceChanged, this/* context */), true/* is retained */)
{
	assert(_menuCommandsHandler.isInstalled());
	assert(_containerResizer.isInstalled());
	
	this->setDataBrowserColumnWidths();
	
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	prefsResult = Preferences_StartMonitoring
					(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeNumberOfContexts,
						true/* notify of initial value */);
	assert(kPreferences_ResultOK == prefsResult);
	prefsResult = Preferences_StartMonitoring
					(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeContextName,
						true/* notify of initial value */);
	assert(kPreferences_ResultOK == prefsResult);
}// My_WorkspacesPanelUI 2-argument constructor


/*!
Tears down a My_WorkspacesPanelUI structure.

(4.0)
*/
My_WorkspacesPanelUI::
~My_WorkspacesPanelUI ()
{
	Preferences_StopMonitoring(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeNumberOfContexts);
	Preferences_StopMonitoring(this->_whenFavoritesChangedHandler.returnRef(), kPreferences_ChangeContextName);
}// My_WorkspacesPanelUI destructor


/*!
Constructs the sub-view hierarchy for the panel,
and makes the specified container the parent.
Returns this parent.

(4.0)
*/
HIViewWrap
My_WorkspacesPanelUI::
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
		
		
		bzero(&containerBounds, sizeof(containerBounds));
		error = CreateUserPaneControl(inOwningWindow, &containerBounds, kControlSupportsEmbedding, &result);
		assert_noerr(error);
		Panel_SetContainerView(inPanel, result);
		SetControlVisibility(result, false/* visible */, false/* draw */);
	}
	
	// create most HIViews for the panel based on the NIB
	error = DialogUtilities_CreateControlsBasedOnWindowNIB
				(CFSTR("PrefPanelWorkspaces"), CFSTR("Panel"), inOwningWindow,
					result/* parent */, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// create the window list; insert appropriate columns
	{
		HIViewWrap							listDummy(idMyUserPaneWindowList, inOwningWindow);
		HIViewRef							windowsList = nullptr;
		DataBrowserTableViewColumnIndex		columnNumber = 0;
		DataBrowserListViewColumnDesc		columnInfo;
		Rect								bounds;
		UIStrings_Result					stringResult = kUIStrings_ResultOK;
		
		
		GetControlBounds(listDummy, &bounds);
		
		// NOTE: here, the original variable is being *replaced* with the data browser, as
		// the original user pane was only needed for size definition
		error = CreateDataBrowserControl(inOwningWindow, &bounds, kDataBrowserListView, &windowsList);
		assert_noerr(error);
		error = SetControlID(windowsList, &idMyDataBrowserWindowList);
		assert_noerr(error);
		
		bzero(&columnInfo, sizeof(columnInfo));
		
		// set defaults for all columns, then override below
		columnInfo.propertyDesc.propertyID = '----';
		columnInfo.propertyDesc.propertyType = kDataBrowserTextType;
		columnInfo.propertyDesc.propertyFlags = kDataBrowserDefaultPropertyFlags | kDataBrowserListViewSortableColumn |
												kDataBrowserListViewMovableColumn;
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
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowListHeaderNumber,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDWindowNumber;
			error = AddDataBrowserListViewColumn(windowsList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// create name column
		stringResult = UIStrings_Copy(kUIStrings_PrefPanelWorkspacesWindowsListHeaderName,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMyDataBrowserPropertyIDWindowName;
			columnInfo.propertyDesc.propertyFlags |= kDataBrowserListViewTypeSelectColumn;
			error = AddDataBrowserListViewColumn(windowsList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// insert as many rows as there can be windows in a workspace
		error = AddDataBrowserItems(windowsList, kDataBrowserNoItem/* parent item */, kPreferences_MaximumWorkspaceSize,
									nullptr/* IDs */, kDataBrowserItemNoProperty/* pre-sort property */);
		assert_noerr(error);
		
		// attach data that would not be specifiable in a NIB
		error = SetDataBrowserCallbacks(windowsList, &this->listCallbacks.listCallbacks);
		assert_noerr(error);
		
		// initialize sort column
		error = SetDataBrowserSortProperty(windowsList, kMyDataBrowserPropertyIDWindowNumber);
		assert_noerr(error);
		
		// set other nice things (most can be set in a NIB someday)
		UNUSED_RETURN(OSStatus)DataBrowserChangeAttributes(windowsList,
															kDataBrowserAttributeListViewAlternatingRowColors/* attributes to set */,
															0/* attributes to clear */);
		UNUSED_RETURN(OSStatus)SetDataBrowserListViewUsePlainBackground(windowsList, false);
		UNUSED_RETURN(OSStatus)SetDataBrowserTableViewHiliteStyle(windowsList, kDataBrowserTableViewFillHilite);
		UNUSED_RETURN(OSStatus)SetDataBrowserHasScrollBars(windowsList, false/* horizontal */, true/* vertical */);
		error = SetDataBrowserSelectionFlags(windowsList, kDataBrowserSelectOnlyOne | kDataBrowserNeverEmptySelectionSet);
		assert_noerr(error);
		{
			DataBrowserPropertyFlags	flags = 0;
			
			
			error = GetDataBrowserPropertyFlags(windowsList, kMyDataBrowserPropertyIDWindowName, &flags);
			if (noErr == error)
			{
				flags |= kDataBrowserPropertyIsMutable;
				error = SetDataBrowserPropertyFlags(windowsList, kMyDataBrowserPropertyIDWindowName, flags);
				if (noErr != error)
				{
					Console_Warning(Console_WriteValue, "failed to add mutability flag to the Workspaces panel’s data browser (names may not be editable), error", error);
				}
			}
		}
		
		// attach panel to data browser
		error = SetControlProperty(windowsList, AppResources_ReturnCreatorCode(),
									kConstantsRegistry_ControlPropertyTypeOwningPanel,
									sizeof(inPanel), &inPanel);
		assert_noerr(error);
		
		error = HIViewAddSubview(result, windowsList);
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
}// My_WorkspacesPanelUI::createContainerView


/*!
This routine, of standard PanelChangedProcPtr form,
is invoked by the Panel module whenever a property
of one of the preferences dialog’s panels changes.

(4.0)
*/
SInt32
My_WorkspacesPanelUI::
panelChanged	(Panel_Ref		inPanel,
				 Panel_Message	inMessage,
				 void*			inDataPtr)
{
	SInt32		result = 0L;
	assert(kCFCompareEqualTo == CFStringCompare(Panel_ReturnKind(inPanel),
												kConstantsRegistry_PrefPanelDescriptorWorkspaces, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			My_WorkspacesPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																		My_WorkspacesPanelDataPtr);
			HIWindowRef const*			windowPtr = REINTERPRET_CAST(inDataPtr, HIWindowRef*);
			
			
			panelDataPtr->interfacePtr = new My_WorkspacesPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
			panelDataPtr->interfacePtr->setDataBrowserColumnWidths();
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			delete (REINTERPRET_CAST(inDataPtr, My_WorkspacesPanelDataPtr));
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
			
			
			// do nothing
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
			My_WorkspacesPanelDataPtr	panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																		My_WorkspacesPanelDataPtr);
			HISize&						newLimits = *(REINTERPRET_CAST(inDataPtr, HISize*));
			
			
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
			My_WorkspacesPanelDataPtr			panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																				My_WorkspacesPanelDataPtr);
			Panel_DataSetTransition const*		dataSetsPtr = REINTERPRET_CAST(inDataPtr, Panel_DataSetTransition*);
			Preferences_ContextRef				oldContext = REINTERPRET_CAST(dataSetsPtr->oldDataSet, Preferences_ContextRef);
			Preferences_ContextRef				newContext = REINTERPRET_CAST(dataSetsPtr->newDataSet, Preferences_ContextRef);
			
			
			if (nullptr != oldContext) Preferences_ContextSave(oldContext);
			
			// select first item
			{
				HIWindowRef					kOwningWindow = Panel_ReturnOwningWindow(inPanel);
				HIViewWrap					dataBrowser(idMyDataBrowserWindowList, kOwningWindow);
				DataBrowserItemID const		kFirstItem = 1;
				
				
				assert(dataBrowser.exists());
				UNUSED_RETURN(OSStatus)SetDataBrowserSelectedItems(dataBrowser, 1/* number of items */, &kFirstItem, kDataBrowserItemsAssign);
			}
			
			// update the current data model accordingly
			panelDataPtr->switchDataModel(newContext, 1/* one-based new item */);
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
}// My_WorkspacesPanelUI::panelChanged


/*!
Invoked whenever a monitored preference value is changed.
Responds by updating the user interface.

(3.1)
*/
void
My_WorkspacesPanelUI::
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					inMyWorkspacesPanelUIPtr)
{
	//Preferences_ChangeContext*	contextPtr = REINTERPRET_CAST(inEventContextPtr, Preferences_ChangeContext*);
	My_WorkspacesPanelUI*		ptr = REINTERPRET_CAST(inMyWorkspacesPanelUIPtr, My_WorkspacesPanelUI*);
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_ChangeNumberOfContexts:
	case kPreferences_ChangeContextName:
		ptr->rebuildSessionMenu();
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Updates the display based on the given settings.

(4.0)
*/
void
My_WorkspacesPanelUI::
readPreferences		(Preferences_ContextRef		inSettings,
					 Preferences_Index			inOneBasedIndex)
{
	assert(0 != inOneBasedIndex);
	if (nullptr != inSettings)
	{
		HIWindowRef const		kOwningWindow = Panel_ReturnOwningWindow(this->panel);
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		Boolean					wasDefault = false;
		
		
		// INCOMPLETE
		
		// set associated Session settings; if this is None, then the entire
		// window entry is effectively disabled and no other window-specific
		// preferences will matter
		{
			CFStringRef		associatedSessionName = nullptr;
			
			
			prefsResult = Preferences_ContextGetData(inSettings, Preferences_ReturnTagVariantForIndex
																	(kPreferences_TagIndexedWindowSessionFavorite,
																		inOneBasedIndex),
														sizeof(associatedSessionName), &associatedSessionName,
														false/* search defaults too */);
			if (kPreferences_ResultOK == prefsResult)
			{
				this->setAssociatedSession(associatedSessionName);
				CFRelease(associatedSessionName), associatedSessionName = nullptr;
			}
			else
			{
				UInt32		associatedSessionType = 0;
				
				
				prefsResult = Preferences_ContextGetData(inSettings, Preferences_ReturnTagVariantForIndex
																		(kPreferences_TagIndexedWindowCommandType,
																			inOneBasedIndex),
															sizeof(associatedSessionType), &associatedSessionType,
															false/* search defaults too */);
				if (kPreferences_ResultOK == prefsResult)
				{
					this->setAssociatedSessionByType(associatedSessionType);
				}
				else
				{
					this->setAssociatedSessionByType(0);
				}
			}
		}
		
		{
			HIViewWrap		checkBox(idMyCheckBoxUseTabsToArrangeWindows, kOwningWindow);
			Boolean			flag = false;
			
			
			assert(checkBox.exists());
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagArrangeWindowsUsingTabs, sizeof(flag), &flag,
														true/* search defaults */, &wasDefault);
			unless (prefsResult == kPreferences_ResultOK)
			{
				flag = false; // assume a value, if preference can’t be found
			}
			SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
		}
		
		{
			HIViewWrap		checkBox(idMyCheckBoxAutoFullScreen, kOwningWindow);
			Boolean			flag = false;
			
			
			assert(checkBox.exists());
			prefsResult = Preferences_ContextGetData(inSettings, kPreferences_TagArrangeWindowsFullScreen, sizeof(flag), &flag,
														true/* search defaults */, &wasDefault);
			unless (prefsResult == kPreferences_ResultOK)
			{
				flag = false; // assume a value, if preference can’t be found
			}
			SetControl32BitValue(checkBox, BooleanToCheckBoxValue(flag));
		}
	}
}// My_WorkspacesPanelUI::readPreferences


/*!
Deletes "inoutItemCountTracker" items below the anchor item for
the specified pop-up menu, and rebuilds the menu based on current
preferences of the given type.  Also tracks the number of items
added so that they can be removed later.

(4.0)
*/
void
My_WorkspacesPanelUI::
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
			UNUSED_RETURN(OSStatus)DeleteMenuItems(favoritesMenu, defaultIndex + 1/* first item */, inoutItemCountTracker);
		}
		otherItemCount = CountMenuItems(favoritesMenu);
		
		// add the names of all terminal configurations to the menu;
		// update global count of items added at that location
		inoutItemCountTracker = 0;
		UNUSED_RETURN(Preferences_Result)Preferences_InsertContextNamesInMenu(inCollectionsToUse, favoritesMenu,
																				defaultIndex, 0/* indentation level */,
																				inEachNewItemCommandID, inoutItemCountTracker);
		SetControl32BitMaximum(popUpMenuView, otherItemCount + inoutItemCountTracker);
		
		// TEMPORARY: verify that this final step is necessary...
		error = SetControlData(popUpMenuView, kControlMenuPart, kControlPopupButtonMenuRefTag,
								sizeof(favoritesMenu), &favoritesMenu);
		assert_noerr(error);
	}
}// My_WorkspacesPanelUI::rebuildFavoritesMenu


/*!
Deletes all the items in the Format pop-up menu and
rebuilds the menu based on current preferences.

(4.0)
*/
void
My_WorkspacesPanelUI::
rebuildSessionMenu ()
{
	rebuildFavoritesMenu(idMyPopUpMenuSession, kCommandSetWorkspaceSessionDefault/* anchor */, Quills::Prefs::SESSION,
							kCommandSetWorkspaceSessionByFavoriteName/* command ID of new items */, this->_numberOfSessionItemsAdded);
}// My_WorkspacesPanelUI::rebuildSessionMenu


/*!
Handles "kEventCommandProcess" of "kEventClassCommand"
for the buttons and menus in this panel.

(4.0)
*/
OSStatus
My_WorkspacesPanelUI::
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inMyWorkspacesPanelUIPtr)
{
	OSStatus				result = eventNotHandledErr;
	My_WorkspacesPanelUI*	interfacePtr = REINTERPRET_CAST(inMyWorkspacesPanelUIPtr, My_WorkspacesPanelUI*);
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
			My_WorkspacesPanelDataPtr	dataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(interfacePtr->panel),
																	My_WorkspacesPanelDataPtr);
			Preferences_Result			prefsResult = kPreferences_ResultOK;
			
			
			result = eventNotHandledErr; // initially...
			
			switch (received.commandID)
			{
			case kCommandSetWorkspaceSessionNone:
			case kCommandSetWorkspaceSessionDefault:
			case kCommandSetWorkspaceSessionShell:
			case kCommandSetWorkspaceSessionLogInShell:
			case kCommandSetWorkspaceSessionCustom:
				// for all of these cases, start by deleting any specific Session association,
				// then put a difference preference in its place (if appropriate)
				{
					Boolean		isError = true;
					
					
					// delete the “associated Session” preference first, because
					// it will take precedence when it is present
					prefsResult = Preferences_ContextDeleteData(dataPtr->dataModel,
																Preferences_ReturnTagVariantForIndex
																(kPreferences_TagIndexedWindowSessionFavorite,
																	dataPtr->currentIndex));
					if (kPreferences_ResultOK == prefsResult)
					{
						// add a new preference for the built-in, if applicable
						// IMPORTANT: this setting CANNOT be deleted to represent “none”,
						// because the factory default preference would then be copied
						// in its place at startup time
						UInt32		savedType = kCommandSetWorkspaceSessionNone;
						
						
						switch (received.commandID)
						{
						case kCommandSetWorkspaceSessionNone:
							savedType = 0;
							break;
						
						case kCommandSetWorkspaceSessionDefault:
							savedType = kCommandNewSessionDefaultFavorite;
							break;
						
						case kCommandSetWorkspaceSessionShell:
							savedType = kCommandNewSessionShell;
							break;
						
						case kCommandSetWorkspaceSessionLogInShell:
							savedType = kCommandNewSessionLoginShell;
							break;
						
						case kCommandSetWorkspaceSessionCustom:
							savedType = kCommandNewSessionDialog;
							break;
						
						default:
							// ???
							break;
						}
						
						prefsResult = Preferences_ContextSetData(dataPtr->dataModel,
																	Preferences_ReturnTagVariantForIndex
																	(kPreferences_TagIndexedWindowCommandType,
																		dataPtr->currentIndex),
																	sizeof(savedType), &savedType);
						if (kPreferences_ResultOK == prefsResult)
						{
							isError = false;
							interfacePtr->setAssociatedSessionByType(savedType);
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandSetWorkspaceSessionByFavoriteName:
				{
					Boolean		isError = true;
					
					
					// determine the name of the selected item
					if (received.attributes & kHICommandFromMenu)
					{
						CFStringRef		collectionName = nullptr;
						
						
						if (noErr == CopyMenuItemTextAsCFString(received.source.menu.menuRef,
																received.source.menu.menuItemIndex, &collectionName))
						{
							// set this name as the new preference value
							prefsResult = Preferences_ContextSetData(dataPtr->dataModel,
																		Preferences_ReturnTagVariantForIndex
																		(kPreferences_TagIndexedWindowSessionFavorite,
																			dataPtr->currentIndex),
																		sizeof(collectionName), &collectionName);
							if (kPreferences_ResultOK == prefsResult)
							{
								isError = false;
								interfacePtr->setAssociatedSession(collectionName);
							}
							CFRelease(collectionName), collectionName = nullptr;
						}
					}
					
					if (isError)
					{
						// failed...
						Sound_StandardAlert();
					}
					
					// pass this handler through to the window
					result = eventNotHandledErr;
				}
				break;
			
			case kCommandSetWorkspaceWindowPosition:
				Keypads_SetArrangeWindowPanelBinding(Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowFrameBounds, dataPtr->currentIndex),
														typeHIRect,
														Preferences_ReturnTagVariantForIndex
														(kPreferences_TagIndexedWindowScreenBounds, dataPtr->currentIndex),
														typeHIRect,
														dataPtr->dataModel);
				Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, true);
				result = noErr;
				break;
			
			case kCommandSetWorkspaceDisplayRegions1x1:
				// UNIMPLEMENTED
				break;
			
			case kCommandSetWorkspaceDisplayRegions2x2:
				// UNIMPLEMENTED
				break;
			
			case kCommandSetWorkspaceDisplayRegions3x3:
				// UNIMPLEMENTED
				break;
			
			default:
				if (received.attributes & kHICommandFromControl)
				{
					HIViewWrap		commandingView(received.source.control);
					Boolean const	kCheckBoxFlagValue = (kControlCheckBoxCheckedValue == GetControl32BitValue(commandingView));
					
					
					if (HIViewIDWrap(idMyCheckBoxUseTabsToArrangeWindows) == commandingView.identifier())
					{
						prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagArrangeWindowsUsingTabs,
																	sizeof(kCheckBoxFlagValue), &kCheckBoxFlagValue);
						if (kPreferences_ResultOK != prefsResult)
						{
							Console_Warning(Console_WriteLine, "unable to save window tabs setting");
						}
						result = noErr;
					}
					else if (HIViewIDWrap(idMyCheckBoxAutoFullScreen) == commandingView.identifier())
					{
						prefsResult = Preferences_ContextSetData(dataPtr->dataModel, kPreferences_TagArrangeWindowsFullScreen,
																	sizeof(kCheckBoxFlagValue), &kCheckBoxFlagValue);
						if (kPreferences_ResultOK != prefsResult)
						{
							Console_Warning(Console_WriteLine, "unable to save window full-screen setting");
						}
						result = noErr;
					}
				}
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
Redraws the windows display.  Use this when you have caused
it to change in some way (by spawning an editor dialog box
or by switching tabs, etc.).

(4.0)
*/
void
My_WorkspacesPanelUI::
refreshDisplay ()
{
	HIViewWrap		windowsListContainer(idMyDataBrowserWindowList, HIViewGetWindow(this->mainView));
	assert(windowsListContainer.exists());
	
	
	UNUSED_RETURN(OSStatus)UpdateDataBrowserItems(windowsListContainer, kDataBrowserNoItem/* parent item */,
													0/* number of IDs */, nullptr/* IDs */,
													kDataBrowserItemNoProperty/* pre-sort property */,
													kMyDataBrowserPropertyIDWindowName);
}// My_WorkspacesPanelUI::refreshDisplay


/*!
Changes the Session menu selection to the specified match,
or chooses the Default if none is found or the name is not
defined.

Note that this only applies to associations with user
favorites.  It is also possible to select from built-in
session types; see setAssociatedSessionByType().

(4.0)
*/
void
My_WorkspacesPanelUI::
setAssociatedSession	(CFStringRef	inContextNameOrNull)
{
	SInt32 const	kFallbackValue = 3; // index of Default item
	HIViewWrap		popUpMenuButton(idMyPopUpMenuSession, HIViewGetWindow(this->mainView));
	
	
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
			Console_Warning(Console_WriteValueCFString, "unable to find menu item for requested Session context",
							inContextNameOrNull);
		}
		else
		{
			assert_noerr(error);
		}
	}
	
	// window-specific views might have been disabled by a None selection
	{
		Boolean const	kActivated = true;
		HIViewWrap		viewWrap;
		
		
		viewWrap = HIViewWrap(idMyButtonSetWindowBoundaries, HIViewGetWindow(this->mainView));
		viewWrap << HIViewWrap_SetActiveState(kActivated);
	}
}// My_WorkspacesPanelUI::setAssociatedSession


/*!
Changes the Session menu selection to the specified match
among built-in types (such as Log-In Shell).  You may also
pass 0 to remove the association, setting it to None in the
menu.

Note that it is also possible to select from user favorites;
see setAssociatedSession().

(4.0)
*/
void
My_WorkspacesPanelUI::
setAssociatedSessionByType	(UInt32		inNewSessionCommandIDOrZero)
{
	HIViewWrap		popUpMenuButton(idMyPopUpMenuSession, HIViewGetWindow(this->mainView));
	UInt32			menuCommandID = kCommandSetWorkspaceSessionNone;
	OSStatus		error = noErr;
	
	
	switch (inNewSessionCommandIDOrZero)
	{
	case kCommandNewSessionDefaultFavorite:
		menuCommandID = kCommandSetWorkspaceSessionDefault;
		break;
	
	case kCommandNewSessionShell:
		menuCommandID = kCommandSetWorkspaceSessionShell;
		break;
	
	case kCommandNewSessionLoginShell:
		menuCommandID = kCommandSetWorkspaceSessionLogInShell;
		break;
	
	case kCommandNewSessionDialog:
		menuCommandID = kCommandSetWorkspaceSessionCustom;
		break;
	
	case 0:
	default:
		menuCommandID = kCommandSetWorkspaceSessionNone;
		break;
	}
	
	error = DialogUtilities_SetPopUpItemByCommand(popUpMenuButton, menuCommandID);
	if (noErr != error)
	{
		Console_Warning(Console_WriteLine, "specified built-in session type not found in menu; reverting to None");
		menuCommandID = kCommandSetWorkspaceSessionNone;
		UNUSED_RETURN(OSStatus)DialogUtilities_SetPopUpItemByCommand(popUpMenuButton, menuCommandID);
	}
	
	// the None case effectively disables all other window-specific
	// views, because the window is not in use
	{
		// enable or disable window-specific views
		Boolean const	kActivated = (kCommandSetWorkspaceSessionNone != menuCommandID);
		HIViewWrap		viewWrap;
		
		
		viewWrap = HIViewWrap(idMyButtonSetWindowBoundaries, HIViewGetWindow(this->mainView));
		viewWrap << HIViewWrap_SetActiveState(kActivated);
	}
}// My_WorkspacesPanelUI::setAssociatedSessionByType


/*!
Sets the widths of the data browser columns
proportionately based on the total width.

(4.0)
*/
void
My_WorkspacesPanelUI::
setDataBrowserColumnWidths ()
{
	Rect			containerRect;
	HIViewWrap		windowsListContainer(idMyDataBrowserWindowList, HIViewGetWindow(this->mainView));
	assert(windowsListContainer.exists());
	
	
	UNUSED_RETURN(Rect*)GetControlBounds(windowsListContainer, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		availableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */ - 6/* double the inset focus ring width */;
		UInt16		integerWidth = 0;
		
		
		// leave number column fixed-size
		{
			integerWidth = 42; // arbitrary
			UNUSED_RETURN(OSStatus)SetDataBrowserTableViewNamedColumnWidth
									(windowsListContainer, kMyDataBrowserPropertyIDWindowNumber, integerWidth);
			availableWidth -= integerWidth;
		}
		
		// give all remaining space to the title
		UNUSED_RETURN(OSStatus)SetDataBrowserTableViewNamedColumnWidth
								(windowsListContainer, kMyDataBrowserPropertyIDWindowName, availableWidth);
	}
}// My_WorkspacesPanelUI::setDataBrowserColumnWidths


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(4.0)
*/
OSStatus
accessDataBrowserItemData	(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemID,
							 DataBrowserPropertyID		inPropertyID,
							 DataBrowserItemDataRef		inItemData,
							 Boolean					inSetValue)
{
	OSStatus					result = noErr;
	My_WorkspacesPanelDataPtr	panelDataPtr = nullptr;
	Panel_Ref					owningPanel = nullptr;
	
	
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
											My_WorkspacesPanelDataPtr);
		}
	}
	
	if (nullptr != panelDataPtr)
	{
		if (false == inSetValue)
		{
			switch (inPropertyID)
			{
			case kDataBrowserItemIsEditableProperty:
				result = SetDataBrowserItemDataBooleanValue(inItemData, true/* is editable */);
				break;
			
			case kMyDataBrowserPropertyIDWindowName:
				// return the text string for the window name
				{
					Preferences_Index	windowIndex = STATIC_CAST(inItemID, Preferences_Index);
					CFStringRef			nameCFString = nullptr;
					Preferences_Result	prefsResult = kPreferences_ResultOK;
					
					
					prefsResult = Preferences_ContextGetData
									(panelDataPtr->dataModel,
										Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowTitle, windowIndex),
										sizeof(nameCFString), &nameCFString, false/* search defaults too */);
					if (kPreferences_ResultOK == prefsResult)
					{
						result = SetDataBrowserItemDataText(inItemData, nameCFString);
						CFRelease(nameCFString), nameCFString = nullptr;
					}
					else
					{
						result = SetDataBrowserItemDataText(inItemData, CFSTR(""));
					}
				}
				break;
			
			case kMyDataBrowserPropertyIDWindowNumber:
				// return the window number as a string
				{
					SInt32			numericalValue = STATIC_CAST(inItemID, SInt32);
					CFStringRef		numberCFString = CFStringCreateWithFormat(kCFAllocatorDefault,
																				nullptr/* options dictionary */,
																				CFSTR("%d")/* LOCALIZE THIS? */,
																				(int)numericalValue);
					
					
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
				result = errDataBrowserPropertyNotSupported;
				break;
			}
		}
		else
		{
			switch (inPropertyID)
			{
			case kMyDataBrowserPropertyIDWindowName:
				// user has changed the window name; update the window in memory
				{
					CFStringRef		newName = nullptr;
					
					
					result = GetDataBrowserItemDataText(inItemData, &newName);
					if (noErr == result)
					{
						// fix window name
						Preferences_Index	windowIndex = STATIC_CAST(inItemID, Preferences_Index);
						Preferences_Result	prefsResult = kPreferences_ResultOK;
						
						
						prefsResult = Preferences_ContextSetData
										(panelDataPtr->dataModel,
											Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowTitle, windowIndex),
											sizeof(newName), &newName);
						if (kPreferences_ResultOK == prefsResult)
						{
							result = noErr;
						}
						else
						{
							result = errDataBrowserItemNotFound;
						}
					}
				}
				break;
			
			case kMyDataBrowserPropertyIDWindowNumber:
				// read-only
				result = paramErr;
				break;
			
			default:
				// ???
				result = errDataBrowserPropertyNotSupported;
				break;
			}
		}
	}
	
	return result;
}// accessDataBrowserItemData


/*!
A standard DataBrowserItemCompareProcPtr, this
method compares items in the list.

(4.0)
*/
Boolean
compareDataBrowserItems		(HIViewRef					inDataBrowser,
							 DataBrowserItemID			inItemOne,
							 DataBrowserItemID			inItemTwo,
							 DataBrowserPropertyID		inSortProperty)
{
	Boolean						result = false;
	My_WorkspacesPanelDataPtr	panelDataPtr = nullptr;
	Panel_Ref					owningPanel = nullptr;
	
	
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
											My_WorkspacesPanelDataPtr);
		}
	}
	
	switch (inSortProperty)
	{
	case kMyDataBrowserPropertyIDWindowName:
		{
			CFStringRef		string1 = nullptr;
			CFStringRef		string2 = nullptr;
			
			
			if (nullptr != panelDataPtr)
			{
				SInt32		windowIndex1 = STATIC_CAST(inItemOne, SInt32);
				SInt32		windowIndex2 = STATIC_CAST(inItemTwo, SInt32);
				
				
				// ignore results, the strings are checked below
				UNUSED_RETURN(Preferences_Result)Preferences_ContextGetData
													(panelDataPtr->dataModel,
														Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowTitle, windowIndex1),
														sizeof(string1), &string1, false/* search defaults too */);
				
				UNUSED_RETURN(Preferences_Result)Preferences_ContextGetData
													(panelDataPtr->dataModel,
														Preferences_ReturnTagVariantForIndex(kPreferences_TagIndexedWindowTitle, windowIndex2),
														sizeof(string2), &string2, false/* search defaults too */);
			}
			
			// check for nullptr, because CFStringCompare() will not deal with it
			if ((nullptr == string1) && (nullptr != string2)) result = true;
			else if ((nullptr == string1) || (nullptr == string2)) result = false;
			else
			{
				result = (kCFCompareLessThan == CFStringCompare(string1, string2,
																kCFCompareCaseInsensitive | kCFCompareLocalized/* options */));
			}
			
			if (nullptr != string1)
			{
				CFRelease(string1), string1 = nullptr;
			}
			if (nullptr != string2)
			{
				CFRelease(string2), string2 = nullptr;
			}
		}
		break;
	
	case kMyDataBrowserPropertyIDWindowNumber:
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

(4.0)
*/
void
deltaSizePanelContainerHIView	(HIViewRef		inView,
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			inContext)
{
	if ((0 != inDeltaX) || (0 != inDeltaY))
	{
		HIWindowRef const			kPanelWindow = HIViewGetWindow(inView);
		My_WorkspacesPanelUIPtr		interfacePtr = REINTERPRET_CAST(inContext, My_WorkspacesPanelUIPtr);
		HIViewWrap					viewWrap;
		
		
		viewWrap = HIViewWrap(idMyDataBrowserWindowList, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(0/* delta X */, inDeltaY);
		
		interfacePtr->setDataBrowserColumnWidths();
		
		viewWrap = HIViewWrap(idMySeparatorSelectedWindow, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
		
		viewWrap = HIViewWrap(idMySeparatorGlobalSettings, kPanelWindow);
		viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0/* delta Y */);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY/* delta Y */);
		viewWrap = HIViewWrap(idMyLabelOptions, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY/* delta Y */);
		viewWrap = HIViewWrap(idMyCheckBoxUseTabsToArrangeWindows, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY/* delta Y */);
		viewWrap = HIViewWrap(idMyCheckBoxAutoFullScreen, kPanelWindow);
		viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY/* delta Y */);
	}
}// deltaSizePanelContainerHIView


/*!
Responds to changes in the data browser.

(4.0)
*/
void
monitorDataBrowserItems		(HIViewRef						inDataBrowser,
							 DataBrowserItemID				inItemID,
							 DataBrowserItemNotification	inMessage)
{
	My_WorkspacesPanelDataPtr	panelDataPtr = nullptr;
	Panel_Ref					owningPanel = nullptr;
	
	
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
											My_WorkspacesPanelDataPtr);
		}
	}
	
	switch (inMessage)
	{
	case kDataBrowserItemSelected:
		if (nullptr != panelDataPtr)
		{
			DataBrowserTableViewRowIndex	rowIndex = 0;
			OSStatus						error = noErr;
			
			
			// update the “selected window” fields to match the newly-selected item
			error = GetDataBrowserTableViewItemRow(inDataBrowser, inItemID, &rowIndex);
			if (noErr == error)
			{
				panelDataPtr->switchDataModel(nullptr/* context */, 1 + rowIndex/* convert from zero-based to one-based */);
			}
			else
			{
				Console_Warning(Console_WriteLine, "unexpected problem determining selected window!");
				Sound_StandardAlert();
			}
		}
		break;
	
	case kDataBrowserEditStopped:
		// it seems to be possible for the I-beam to persist at times
		// unless the cursor is explicitly reset here
		//[[NSCursor arrowCursor] set];
		break;
	
	default:
		// not all messages are supported
		break;
	}
}// monitorDataBrowserItems

} // anonymous namespace


#pragma mark -
@implementation PrefPanelWorkspaces_ViewManager //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	NSArray*	subViewManagers =
				@[
					[[[PrefPanelWorkspaces_OptionsViewManager alloc] init] autorelease],
					[[[PrefPanelWorkspaces_WindowsViewManager alloc] init] autorelease],
				];
	
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Workspaces"
										localizedName:NSLocalizedStringFromTable(@"Workspaces", @"PrefPanelWorkspaces",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelWorkspaces"]
										viewManagerArray:subViewManagers];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


@end //} PrefPanelWorkspaces_ViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_OptionsViewManager //{


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelWorkspaceOptionsCocoa" delegate:self context:nullptr];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
automaticallyEnterFullScreen
{
	return [self->byKey objectForKey:@"automaticallyEnterFullScreen"];
}// automaticallyEnterFullScreen


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_Flag*)
useTabbedWindows
{
	return [self->byKey objectForKey:@"useTabbedWindows"];
}// useTabbedWindows


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(void*)			aContext
{
#pragma unused(aViewManager, aContext)
	self->prefsMgr = [[PrefsContextManager_Object alloc] initWithDefaultContextInClass:[self preferencesClass]];
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:5/* arbitrary; number of settings */];
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	*outEditType = kPanel_EditTypeInspector;
}// panelViewManager:requestingEditType:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
#pragma unused(aViewManager, aContainerView)
	assert(nil != byKey);
	assert(nil != prefsMgr);
	
	// remember frame from XIB (it might be changed later)
	self->idealSize = [aContainerView frame].size;
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagArrangeWindowsFullScreen
														contextManager:self->prefsMgr] autorelease]
					forKey:@"automaticallyEnterFullScreen"];
	[self->byKey setObject:[[[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagArrangeWindowsUsingTabs
														contextManager:self->prefsMgr] autorelease]
					forKey:@"useTabbedWindows"];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
	}
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	*outIdealSize = self->idealSize;
}


/*!
Responds to a request for contextual help in this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager, sender)
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the new data set.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// now apply the specified settings
	[self->prefsMgr setCurrentContext:REINTERPRET_CAST(newDataSet, Preferences_ContextRef)];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
	}
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).  Responds by saving preferences.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager, aContainerView)
	if (isAccepted)
	{
		Preferences_Result	prefsResult = Preferences_Save();
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "failed to save preferences!");
		}
	}
	else
	{
		// revert - UNIMPLEMENTED (not supported)
	}
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_ViewManager


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [NSImage imageNamed:@"IconForPrefPanelWorkspaces"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Workspaces.Options";
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

(4.1)
*/
- (NSString*)
panelName
{
	return NSLocalizedStringFromTable(@"Options", @"PrefPanelWorkspaces", @"the name of this panel");
}// panelName


/*!
Returns information on which directions are most useful for
resizing the panel.  For instance a window container may
disallow vertical resizing if no panel in the window has
any reason to resize vertically.

IMPORTANT:	This is only a hint.  Panels must be prepared
			to resize in both directions.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintHorizontal;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::WORKSPACE;
}// preferencesClass


@end //} PrefPanelWorkspaces_OptionsViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_OptionsViewManager (PrefPanelWorkspaces_OptionsViewManagerInternal) //{


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"automaticallyEnterFullScreen", @"useTabbedWindows"];
}// primaryDisplayBindingKeys


@end //} PrefPanelWorkspaces_OptionsViewManager (PrefPanelWorkspaces_OptionsViewManagerInternal)


#pragma mark -
@implementation PrefPanelWorkspaces_WindowInfo //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
	}
	return self;
}// init


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
windowIndexLabel
{
	return [[NSNumber numberWithUnsignedInteger:self.currentIndex] stringValue];
}// windowIndexLabel


/*!
Accessor.

(4.1)
*/
- (NSString*)
windowName
{
	NSString*				result = @"";
	assert(0 != self.currentIndex);
	Preferences_Tag const	windowTitleIndexedTag = [self actualTagForBase:kPreferences_TagIndexedWindowTitle];
	CFStringRef				nameCFString = nullptr;
	Preferences_Result		prefsResult = Preferences_ContextGetData
											(self.currentContext, windowTitleIndexedTag,
												sizeof(nameCFString), &nameCFString,
												false/* search defaults */);
	
	
	if (kPreferences_ResultBadVersionDataNotAvailable == prefsResult)
	{
		// this is possible for indexed tags, as not all indexes
		// in the range may have data defined; ignore
		//Console_Warning(Console_WriteValue, "failed to retrieve window title preference, error", prefsResult);
	}
	else if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "failed to retrieve window title preference, error", prefsResult);
	}
	else
	{
		result = BRIDGE_CAST(nameCFString, NSString*);
	}
	
	return result;
}
- (void)
setWindowName:(NSString*)	aWindowName
{
	assert(0 != self.currentIndex);
	Preferences_Tag const	windowTitleIndexedTag = [self actualTagForBase:kPreferences_TagIndexedWindowTitle];
	CFStringRef				asCFStringRef = BRIDGE_CAST(aWindowName, CFStringRef);
	Preferences_Result		prefsResult = Preferences_ContextSetData
											(self.currentContext, windowTitleIndexedTag,
												sizeof(asCFStringRef), &asCFStringRef);
	
	
	if (kPreferences_ResultOK != prefsResult)
	{
		Console_Warning(Console_WriteValue, "failed to set window title preference, error", prefsResult);
	}
}// setWindowName:


#pragma mark GenericPanelNumberedList_ListItemHeader


/*!
Return strong reference to user interface string representing
numbered index in list.

Returns the "windowIndexLabel".

(4.1)
*/
- (NSString*)
numberedListIndexString
{
	return self.windowIndexLabel;
}// numberedListIndexString


/*!
Return or update user interface string for name of item in list.

Accesses the "windowName".

(4.1)
*/
- (NSString*)
numberedListItemName
{
	return self.windowName;
}
- (void)
setNumberedListItemName:(NSString*)		aName
{
	self.windowName = aName;
}// setNumberedListItemName:


@end //}


#pragma mark -
@implementation PrefPanelWorkspaces_WindowsViewManager //{


@synthesize windowEditorViewManager = _windowEditorViewManager;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self.windowEditorViewManager = [[[PrefPanelWorkspaces_WindowEditorViewManager alloc] init] autorelease];
	
	self = [super initWithIdentifier:@"net.macterm.prefpanels.Workspaces.Windows"
										localizedName:NSLocalizedStringFromTable(@"Windows", @"PrefPanelWorkspaces",
																					@"the name of this panel")
										localizedIcon:[NSImage imageNamed:@"IconForPrefPanelWorkspaces"]
										master:self detailViewManager:self.windowEditorViewManager];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowInfo*)
selectedWindowInfo
{
	PrefPanelWorkspaces_WindowInfo*		result = nil;
	NSUInteger							currentIndex = [self.listItemHeaderIndexes firstIndex];
	
	
	if (NSNotFound != currentIndex)
	{
		result = [self.listItemHeaders objectAtIndex:currentIndex];
	}
	
	return result;
}// selectedWindowInfo


#pragma mark GeneralPanelNumberedList_Master


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
initializeNumberedListViewManager:(GenericPanelNumberedList_ViewManager*)	aViewManager
{
	NSArray*			listData =
						@[
							// create as many managers as there are supported indexes
							// (see the Preferences module)
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
							[[[PrefPanelWorkspaces_WindowInfo alloc] init] autorelease],
						];
	Preferences_Index	prefsIndexValue = 1;
	
	
	// configure each context manager to modify a different index
	// of the current workspace data (the context itself is set
	// later, through callbacks)
	for (PrefPanelWorkspaces_WindowInfo* eachInfo in listData)
	{
		[eachInfo setCurrentIndex:prefsIndexValue];
		++prefsIndexValue;
	}
	
	aViewManager.listItemHeaders = listData;
}// initializeNumberedListViewManager:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
containerViewDidLoadForNumberedListViewManager:(GenericPanelNumberedList_ViewManager*)	aViewManager
{
	// customize numbered-list interface
	aViewManager.headingTitleForNameColumn = NSLocalizedStringFromTable(@"Window Name", @"PrefPanelWorkspaces", @"the title for the item name column");
}// containerViewDidLoadForNumberedListViewManager:


/*!
Responds to a change in data set by updating the context that
is associated with each item in the list.

Note that this is also called when the selected index changes
and the context has not changed (context may be nullptr), and
this variant is ignored.

(4.1)
*/
- (void)
numberedListViewManager:(GenericPanelNumberedList_ViewManager*)		aViewManager
didChangeFromDataSet:(GenericPanelNumberedList_DataSet*)			oldStructPtr
toDataSet:(GenericPanelNumberedList_DataSet*)						newStructPtr
{
#pragma unused(aViewManager, oldStructPtr)
	
	if (nullptr != newStructPtr)
	{
		Preferences_ContextRef		asContext = REINTERPRET_CAST(newStructPtr->parentPanelDataSetOrNull, Preferences_ContextRef);
		
		
		// if the context is nullptr, only the index has changed;
		// since the index is not important here, it is ignored
		if (nullptr != asContext)
		{
			// update each object in the list to use the new context
			// (so that, for instance, the right names are displayed)
			for (PrefPanelWorkspaces_WindowInfo* eachInfo in aViewManager.listItemHeaders)
			{
				[eachInfo setCurrentContext:asContext];
			}
		}
	}
}// numberedListViewManager:didChangeFromDataSet:toDataSet:


@end //} PrefPanelWorkspaces_WindowsViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_WindowBoundariesValue


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		// WARNING: this is not actually of “flag” type and any attempt
		// to read or set the value will fail; this object is only used
		// for simplicity because the binding currently only requires
		// the inherited/enabled settings
		self->frameObject = [[PreferenceValue_Flag alloc]
								initWithPreferencesTag:kPreferences_TagIndexedWindowFrameBounds
														contextManager:aContextMgr];
		self->screenBoundsObject = [[PreferenceValue_Flag alloc]
									initWithPreferencesTag:kPreferences_TagIndexedWindowScreenBounds
															contextManager:aContextMgr];
		
		// monitor the preferences context manager so that observers
		// of preferences in sub-objects can be told to expect changes
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(prefsContextWillChange:)
															name:kPrefsContextManager_ContextWillChangeNotification
															object:aContextMgr];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(prefsContextDidChange:)
															name:kPrefsContextManager_ContextDidChangeNotification
															object:aContextMgr];
	}
	return self;
}// initWithContextManager:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextDidChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextWillChange:"
	[self didSetPreferenceValue];
}// prefsContextDidChange:


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextWillChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextDidChange:"
	[self willSetPreferenceValue];
}// prefsContextWillChange:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = ([self->frameObject isInherited] && [self->screenBoundsObject isInherited]);
	
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self willSetPreferenceValue];
	[self->frameObject setNilPreferenceValue];
	[self->screenBoundsObject setNilPreferenceValue];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end //} PrefPanelWorkspaces_WindowBoundariesValue


#pragma mark -
@implementation PrefPanelWorkspaces_WindowEditorViewManager //{


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
init
{
	self = [super initWithNibNamed:@"PrefPanelWorkspaceWindowsCocoa" delegate:self context:nil];
	if (nil != self)
	{
		// do not initialize here; most likely should use "panelViewManager:initializeWithContext:"
	}
	return self;
}// init


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Actions


/*!
Displays a panel to edit the location of the window.

(4.1)
*/
- (IBAction)
performSetBoundary:(id)		sender
{
#pragma unused(sender)
	Preferences_ContextRef const	currentContext = [self.windowsViewManager.selectedWindowInfo currentContext];
	
	
	if (nullptr == currentContext)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "attempt to perform set-boundary with invalid context");
	}
	else
	{
		Preferences_Index const		currentIndex = [self.windowsViewManager.selectedWindowInfo currentIndex];
		
		
		Keypads_SetArrangeWindowPanelBinding(Preferences_ReturnTagVariantForIndex
												(kPreferences_TagIndexedWindowFrameBounds, currentIndex),
												typeHIRect,
												Preferences_ReturnTagVariantForIndex
												(kPreferences_TagIndexedWindowScreenBounds, currentIndex),
												typeHIRect,
												currentContext);
		Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, true);
	}
}// performSetBoundary:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowBoundariesValue*)
windowBoundaries
{
	return [self->byKey objectForKey:@"windowBoundaries"];
}// windowBoundaries


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowSessionValue*)
windowSession
{
	return [self->byKey objectForKey:@"windowSession"];
}// windowSession


/*!
Accessor.

(4.1)
*/
- (PrefPanelWorkspaces_WindowsViewManager*)
windowsViewManager
{
	Panel_ViewManager*		parentViewManager = self.panelParent;
	assert(nil != parentViewManager);
	assert([parentViewManager isKindOfClass:PrefPanelWorkspaces_WindowsViewManager.class]);
	PrefPanelWorkspaces_WindowsViewManager*		result = STATIC_CAST(parentViewManager, PrefPanelWorkspaces_WindowsViewManager*);
	
	
	return result;
}// windowsViewManager


#pragma mark Panel_Delegate


/*!
The first message ever sent, before any NIB loads; initialize the
subclass, at least enough so that NIB object construction and
bindings succeed.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
initializeWithContext:(void*)			aContext
{
#pragma unused(aViewManager, aContext)
	self->byKey = [[NSMutableDictionary alloc] initWithCapacity:5/* arbitrary; number of settings */];
}// panelViewManager:initializeWithContext:


/*!
Specifies the editing style of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingEditType:(Panel_EditType*)	outEditType
{
#pragma unused(aViewManager)
	*outEditType = kPanel_EditTypeInspector;
}// panelViewManager:requestingEditType:


/*!
First entry point after view is loaded; responds by performing
any other view-dependent initializations.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didLoadContainerView:(NSView*)			aContainerView
{
#pragma unused(aViewManager, aContainerView)
	assert(nil != byKey);
	
	// remember frame from XIB (it might be changed later)
	self->idealSize = [aContainerView frame].size;
	
	// note that all current values will change
	for (NSString* keyName in [self primaryDisplayBindingKeys])
	{
		[self willChangeValueForKey:keyName];
	}
	
	// WARNING: Key names are depended upon by bindings in the XIB file.
	// NOTE: These are initialized in an invalid state (no context manager)
	// because they will always be changed via callback to a specific
	// combination of context and index.
	[self->byKey setObject:[[[PrefPanelWorkspaces_WindowBoundariesValue alloc]
								initWithContextManager:nullptr] autorelease]
					forKey:@"windowBoundaries"];
	//[self->byKey setObject:[[[PrefPanelWorkspaces_WindowSessionValue alloc]
	//							initWithContextManager:nullptr] autorelease]
	//				forKey:@"windowSession"];
	
	// note that all values have changed (causes the display to be refreshed)
	for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
	{
		[self didChangeValueForKey:keyName];
	}
}// panelViewManager:didLoadContainerView:


/*!
Specifies a sensible width and height for this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
requestingIdealSize:(NSSize*)			outIdealSize
{
#pragma unused(aViewManager)
	*outIdealSize = self->idealSize;
}


/*!
Responds to a request for contextual help in this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didPerformContextSensitiveHelp:(id)		sender
{
#pragma unused(aViewManager, sender)
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhrasePreferences);
}// panelViewManager:didPerformContextSensitiveHelp:


/*!
Responds just before a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
willChangePanelVisibility:(Panel_Visibility)	aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:willChangePanelVisibility:


/*!
Responds just after a change to the visible state of this panel.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)			aViewManager
didChangePanelVisibility:(Panel_Visibility)		aVisibility
{
#pragma unused(aViewManager, aVisibility)
}// panelViewManager:didChangePanelVisibility:


/*!
Responds to a change of data sets by resetting the panel to
display the data set currently selected by the controlling
parent (numbered list); the assumption is that this will
always be in sync with the data set actually given.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didChangeFromDataSet:(void*)			oldDataSet
toDataSet:(void*)						newDataSet
{
#pragma unused(aViewManager, oldDataSet)
	//GenericPanelNumberedList_DataSet*	oldStructPtr = REINTERPRET_CAST(oldDataSet, GenericPanelNumberedList_DataSet*);
	GenericPanelNumberedList_DataSet*	newStructPtr = REINTERPRET_CAST(newDataSet, GenericPanelNumberedList_DataSet*);
	Preferences_ContextRef				asPrefsContext = REINTERPRET_CAST(newStructPtr->parentPanelDataSetOrNull, Preferences_ContextRef);
	Preferences_Index					asIndex = STATIC_CAST(newStructPtr->selectedListItem, Preferences_Index);
	
	
	if (self.isPanelUserInterfaceLoaded)
	{
		// note that all current values will change
		for (NSString* keyName in [self primaryDisplayBindingKeys])
		{
			[self willChangeValueForKey:keyName];
		}
		
		// now apply the specified settings
		if (nullptr != asPrefsContext)
		{
			self.windowBoundaries.prefsMgr.currentContext = asPrefsContext;
			self.windowSession.prefsMgr.currentContext = asPrefsContext;
		}
		self.windowBoundaries.prefsMgr.currentIndex = asIndex;
		self.windowSession.prefsMgr.currentIndex = asIndex;
		
		// note that all values have changed (causes the display to be refreshed)
		for (NSString* keyName in [[self primaryDisplayBindingKeys] reverseObjectEnumerator])
		{
			[self didChangeValueForKey:keyName];
		}
	}
}// panelViewManager:didChangeFromDataSet:toDataSet:


/*!
Last entry point before the user finishes making changes
(or discarding them).  Responds by saving preferences.

(4.1)
*/
- (void)
panelViewManager:(Panel_ViewManager*)	aViewManager
didFinishUsingContainerView:(NSView*)	aContainerView
userAccepted:(BOOL)						isAccepted
{
#pragma unused(aViewManager, aContainerView)
	if (isAccepted)
	{
		Preferences_Result	prefsResult = Preferences_Save();
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "failed to save preferences!");
		}
	}
	else
	{
		// revert - UNIMPLEMENTED (not supported)
	}
}// panelViewManager:didFinishUsingContainerView:userAccepted:


#pragma mark Panel_ViewManager


/*!
Returns the localized icon image that should represent
this panel in user interface elements (e.g. it might be
used in a toolbar item).

(4.1)
*/
- (NSImage*)
panelIcon
{
	return [NSImage imageNamed:@"IconForPrefPanelWorkspaces"];
}// panelIcon


/*!
Returns a unique identifier for the panel (e.g. it may be
used in toolbar items that represent panels).

(4.1)
*/
- (NSString*)
panelIdentifier
{
	return @"net.macterm.prefpanels.Workspaces.Windows.EditWindow";
}// panelIdentifier


/*!
Returns the localized name that should be displayed as
a label for this panel in user interface elements (e.g.
it might be the name of a tab or toolbar icon).

(4.1)
*/
- (NSString*)
panelName
{
	return NSLocalizedStringFromTable(@"Edit Window", @"PrefPanelWorkspaces", @"the name of this panel");
}// panelName


/*!
Returns information on which directions are most useful for
resizing the panel.  For instance a window container may
disallow vertical resizing if no panel in the window has
any reason to resize vertically.

IMPORTANT:	This is only a hint.  Panels must be prepared
			to resize in both directions.

(4.1)
*/
- (Panel_ResizeConstraint)
panelResizeAxes
{
	return kPanel_ResizeConstraintHorizontal;
}// panelResizeAxes


#pragma mark PrefsWindow_PanelInterface


/*!
Returns the class of preferences edited by this panel.

(4.1)
*/
- (Quills::Prefs::Class)
preferencesClass
{
	return Quills::Prefs::WORKSPACE;
}// preferencesClass


@end //} PrefPanelWorkspaces_WindowEditorViewManager


#pragma mark -
@implementation PrefPanelWorkspaces_WindowEditorViewManager (PrefPanelWorkspaces_WindowEditorViewManagerInternal) //{


/*!
Returns the names of key-value coding keys that represent the
primary bindings of this panel (those that directly correspond
to saved preferences).

(4.1)
*/
- (NSArray*)
primaryDisplayBindingKeys
{
	return @[@"windowBoundaries", @"windowSession"];
}// primaryDisplayBindingKeys


@end //} PrefPanelWorkspaces_WindowEditorViewManager (PrefPanelWorkspaces_WindowEditorViewManagerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
