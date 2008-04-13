/*###############################################################

	PrefPanelTranslations.cp
	
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

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <CommonEventHandlers.h>
#include <Console.h>
#include <DialogAdjust.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <IconManager.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>

// resource includes
#include "DialogResources.h"
#include "GeneralResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Panel.h"
#include "Preferences.h"
#include "PrefPanelTranslations.h"
#include "TextTranslation.h"
#include "UIStrings.h"
#include "UIStrings_PrefsWindow.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the NIBs from the package "PrefPanels.nib".

In addition, they MUST be unique across all panels.
*/
static HIViewID const	idMyLabelBaseTranslationTable			= { 'LBTT', 0/* ID */ };
static HIViewID const	idMyDataBrowserBaseTranslationTable		= { 'Tran', 0/* ID */ };
static HIViewID const	idMyLabelOptions						= { 'LOpt', 0/* ID */ };
static HIViewID const	idMyCheckBoxUseBackupFont				= { 'XUBF', 0/* ID */ };
static HIViewID const	idMyButtonBackupFontName				= { 'UFNm', 0/* ID */ };
static HIViewID const	idMyLabelBackupFontTest					= { 'BFTs', 0/* ID */ };
static HIViewID const	idMyImageBackupFontTest					= { 'BFTI', 0/* ID */ };
static HIViewID const	idMyHelpTextBackupFontTest				= { 'HFTs', 0/* ID */ };

// The following cannot use any of Apple’s reserved IDs (0 to 1023).
enum
{
	kMy_DataBrowserPropertyIDBaseCharacterSet		= 'Base'
};

#pragma mark Types

/*!
Initializes a Data Browser callbacks structure to
point to appropriate functions in this file for
handling various tasks.  Creates the necessary UPP
types, which are destroyed when the instance goes
away.
*/
struct My_TranslationsDataBrowserCallbacks
{
	My_TranslationsDataBrowserCallbacks		();
	~My_TranslationsDataBrowserCallbacks	();
	
	DataBrowserCallbacks	listCallbacks;
};

/*!
Implements the user interface of the panel - only
created when the panel directs for this to happen.
*/
struct My_TranslationsPanelUI
{
public:
	My_TranslationsPanelUI	(Panel_Ref, HIWindowRef);
	
	My_TranslationsDataBrowserCallbacks		listCallbacks;
	HIViewWrap								mainView;
	HIViewWrap								labelBaseTable;
	HIViewWrap								dataBrowserBaseTable;
	CommonEventHandlers_HIViewResizer		containerResizer;	//!< invoked when the panel is resized
	CarbonEventHandlerWrap					viewClickHandler;	//!< invoked when a tab is clicked

protected:
	void
	assignAccessibilityRelationships ();
	
	HIViewWrap
	createContainerView		(Panel_Ref, HIWindowRef) const;
};
typedef My_TranslationsPanelUI*		My_TranslationsPanelUIPtr;

/*!
Contains the panel reference and its user interface
(once the UI is constructed).
*/
struct My_TranslationsPanelData
{
	My_TranslationsPanelData ();
	
	Panel_Ref					panel;			//!< the panel this data is for
	My_TranslationsPanelUI*		interfacePtr;	//!< if not nullptr, the panel user interface is active
};
typedef My_TranslationsPanelData*	My_TranslationsPanelDataPtr;

#pragma mark Internal Method Prototypes

static pascal OSStatus	accessDataBrowserItemData		(HIViewRef, DataBrowserItemID, DataBrowserPropertyID,
														 DataBrowserItemDataRef, Boolean);
static pascal Boolean	compareDataBrowserItems			(HIViewRef, DataBrowserItemID, DataBrowserItemID, DataBrowserPropertyID);
static void				deltaSizePanelContainerHIView	(HIViewRef, Float32, Float32, void*);
static void				disposePanel					(Panel_Ref, void*);
static SInt32			panelChanged					(Panel_Ref, Panel_Message, void*);
static pascal OSStatus	receiveViewHit					(EventHandlerCallRef, EventRef, void*);
static void				setDataBrowserColumnWidths		(My_TranslationsPanelUIPtr);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Float32		gIdealPanelWidth = 0.0;
	Float32		gIdealPanelHeight = 0.0;
}



#pragma mark Public Methods

/*!
This routine creates a new preference panel for the
Translations category, initializes it, and returns
a reference to it.  You must destroy the reference
using Panel_Dispose() when you are done with it.

If any problems occur, nullptr is returned.

(3.1)
*/
Panel_Ref
PrefPanelTranslations_New ()
{
	Panel_Ref	result = Panel_New(panelChanged);
	
	
	if (result != nullptr)
	{
		My_TranslationsPanelDataPtr		dataPtr = new My_TranslationsPanelData;
		CFStringRef						nameCFString = nullptr;
		
		
		Panel_SetKind(result, kConstantsRegistry_PrefPanelDescriptorTranslations);
		Panel_SetShowCommandID(result, kCommandDisplayPrefPanelTranslations);
		if (UIStrings_Copy(kUIStrings_PreferencesWindowTranslationsCategoryName, nameCFString).ok())
		{
			Panel_SetName(result, nameCFString);
			CFRelease(nameCFString), nameCFString = nullptr;
		}
		Panel_SetIconRefFromBundleFile(result, AppResources_ReturnPrefPanelTranslationsIconFilenameNoExtension(),
										AppResources_ReturnCreatorCode(),
										kConstantsRegistry_IconServicesIconPrefPanelFullScreen);
		Panel_SetImplementation(result, dataPtr);
	}
	return result;
}// New


#pragma mark Internal Methods

/*!
Initializes a My_TranslationsDataBrowserCallbacks structure.

(3.1)
*/
My_TranslationsDataBrowserCallbacks::
My_TranslationsDataBrowserCallbacks ()
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
}// My_TranslationsDataBrowserCallbacks default constructor


/*!
Tears down a My_TranslationsDataBrowserCallbacks structure.

(3.1)
*/
My_TranslationsDataBrowserCallbacks::
~My_TranslationsDataBrowserCallbacks ()
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
}// My_TranslationsDataBrowserCallbacks destructor


/*!
Initializes a My_TranslationsPanelData structure.

(3.1)
*/
My_TranslationsPanelData::
My_TranslationsPanelData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
panel(nullptr),
interfacePtr(nullptr)
{
}// My_TranslationsPanelData default constructor


/*!
Initializes a My_TranslationsPanelUI structure.

(3.1)
*/
My_TranslationsPanelUI::
My_TranslationsPanelUI	(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
listCallbacks			(),
mainView				(createContainerView(inPanel, inOwningWindow)
							<< HIViewWrap_AssertExists),
labelBaseTable			(idMyLabelBaseTranslationTable, inOwningWindow),
dataBrowserBaseTable	(idMyDataBrowserBaseTranslationTable, inOwningWindow),
containerResizer		(mainView, kCommonEventHandlers_ChangedBoundsEdgeSeparationH |
							kCommonEventHandlers_ChangedBoundsEdgeSeparationV,
							deltaSizePanelContainerHIView, this/* context */),
viewClickHandler		(CarbonEventUtilities_ReturnViewTarget(this->mainView), receiveViewHit,
							CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlHit),
							this/* user data */)
{
	assert(labelBaseTable.exists());
	assert(dataBrowserBaseTable.exists());
	assert(containerResizer.isInstalled());
	assert(viewClickHandler.isInstalled());
	
	setDataBrowserColumnWidths(this);
	assignAccessibilityRelationships();
}// My_TranslationsPanelUI 2-argument constructor


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
My_TranslationsPanelUI::
assignAccessibilityRelationships ()
{
}// assignAccessibilityRelationships


/*!
Constructs the main panel container view and its
sub-view hierarchy.  The container is returned.

(3.1)
*/
HIViewWrap
My_TranslationsPanelUI::
createContainerView		(Panel_Ref		inPanel,
						 HIWindowRef	inOwningWindow)
const
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
			(CFSTR("PrefPanels"), CFSTR("Translations"), inOwningWindow,
				result, viewList, idealContainerBounds);
	assert_noerr(error);
	
	// create the base table list; insert appropriate columns
	{
		HIViewWrap							listDummy(idMyDataBrowserBaseTranslationTable, inOwningWindow);
		HIViewRef							baseTableList = nullptr;
		DataBrowserTableViewColumnIndex		columnNumber = 0;
		DataBrowserListViewColumnDesc		columnInfo;
		Rect								bounds;
		UIStrings_Result					stringResult = kUIStrings_ResultOK;
		
		
		GetControlBounds(listDummy, &bounds);
		
		// NOTE: here, the original variable is being *replaced* with the data browser, as
		// the original user pane was only needed for size definition
		error = CreateDataBrowserControl(inOwningWindow, &bounds, kDataBrowserListView, &baseTableList);
		assert_noerr(error);
		error = SetControlID(baseTableList, &idMyDataBrowserBaseTranslationTable);
		assert_noerr(error);
		
		bzero(&columnInfo, sizeof(columnInfo));
		
		// set defaults for all columns, then override below
		columnInfo.propertyDesc.propertyID = '----';
		columnInfo.propertyDesc.propertyType = kDataBrowserTextType;
		columnInfo.propertyDesc.propertyFlags = kDataBrowserDefaultPropertyFlags | kDataBrowserListViewSortableColumn |
												kDataBrowserListViewTypeSelectColumn;
		columnInfo.headerBtnDesc.version = kDataBrowserListViewLatestHeaderDesc;
		columnInfo.headerBtnDesc.minimumWidth = 200; // arbitrary
		columnInfo.headerBtnDesc.maximumWidth = 600; // arbitrary
		columnInfo.headerBtnDesc.titleOffset = 0; // arbitrary
		columnInfo.headerBtnDesc.titleString = nullptr;
		columnInfo.headerBtnDesc.initialOrder = 0;
		columnInfo.headerBtnDesc.btnFontStyle.flags = kControlUseJustMask;
		columnInfo.headerBtnDesc.btnFontStyle.just = teFlushDefault;
		columnInfo.headerBtnDesc.btnContentInfo.contentType = kControlContentTextOnly;
		
		// create base table column
		stringResult = UIStrings_Copy(kUIStrings_PreferencesWindowTranslationsListHeaderBaseTable,
										columnInfo.headerBtnDesc.titleString);
		if (stringResult.ok())
		{
			columnInfo.propertyDesc.propertyID = kMy_DataBrowserPropertyIDBaseCharacterSet;
			error = AddDataBrowserListViewColumn(baseTableList, &columnInfo, columnNumber++);
			assert_noerr(error);
			CFRelease(columnInfo.headerBtnDesc.titleString), columnInfo.headerBtnDesc.titleString = nullptr;
		}
		
		// hide the heading; it is not useful
		(OSStatus)SetDataBrowserListViewHeaderBtnHeight(baseTableList, 0);
		
		// insert as many rows as there are translation tables
		{
			UInt16 const	kNumberOfTables = TextTranslation_ReturnCharacterSetCount();
			
			
			error = AddDataBrowserItems(baseTableList, kDataBrowserNoItem/* parent item */, kNumberOfTables, nullptr/* IDs */,
										kDataBrowserItemNoProperty/* pre-sort property */);
			assert_noerr(error);
		}
		
		// attach data that would not be specifiable in a NIB
		error = SetDataBrowserCallbacks(baseTableList, &this->listCallbacks.listCallbacks);
		assert_noerr(error);
		
		// initialize sort column
		error = SetDataBrowserSortProperty(baseTableList, kMy_DataBrowserPropertyIDBaseCharacterSet);
		assert_noerr(error);
		
		// set other nice things (most can be set in a NIB someday)
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		if (FlagManager_Test(kFlagOS10_4API))
		{
			(OSStatus)DataBrowserChangeAttributes(baseTableList,
													FUTURE_SYMBOL(1 << 1, kDataBrowserAttributeListViewAlternatingRowColors)/* attributes to set */,
													0/* attributes to clear */);
		}
	#endif
		(OSStatus)SetDataBrowserListViewUsePlainBackground(baseTableList, false);
		(OSStatus)SetDataBrowserTableViewHiliteStyle(baseTableList, kDataBrowserTableViewFillHilite);
		(OSStatus)SetDataBrowserHasScrollBars(baseTableList, false/* horizontal */, true/* vertical */);
		(OSStatus)SetDataBrowserSelectionFlags(baseTableList, kDataBrowserSelectOnlyOne | kDataBrowserNeverEmptySelectionSet);
		
		error = HIViewAddSubview(result, baseTableList);
		assert_noerr(error);
	}
	
	// render the backup font test
	{
		AppResources_Result		resourceError = noErr;
		CGImageRef				fontTestPicture = nullptr;
		
		
		resourceError = AppResources_GetFontTestPicture(fontTestPicture);
		assert_noerr(resourceError);
		if ((noErr == resourceError) && (nullptr != fontTestPicture))
		{
			HIViewWrap		fontTestView(idMyImageBackupFontTest, inOwningWindow);
			
			
			error = HIImageViewSetImage(fontTestView, fontTestPicture);
			assert_noerr(error);
			
			CFRelease(fontTestPicture), fontTestPicture = nullptr;
		}
	}
	
	// calculate the ideal size
	gIdealPanelWidth = idealContainerBounds.right - idealContainerBounds.left;
	gIdealPanelHeight = idealContainerBounds.bottom - idealContainerBounds.top;
	
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
	
	// initialize views - UNIMPLEMENTED
	
	return result;
}// My_TranslationsPanelUI::createContainerView


/*!
A standard DataBrowserItemDataProcPtr, this routine
responds to requests sent by Mac OS X for data that
belongs in the specified list.

(3.1)
*/
static pascal OSStatus
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
			// TEMPORARY - UNIMPLEMENTED
			result = SetDataBrowserItemDataBooleanValue(inItemData, false/* is editable */);
			break;
		
		case kMy_DataBrowserPropertyIDBaseCharacterSet:
			// return the text string for the character set name
			{
				CFStringEncoding	thisEncoding = TextTranslation_ReturnIndexedCharacterSet(inItemID);
				CFStringRef			tableNameCFString = nullptr;
				
				
				// LOCALIZE THIS
				tableNameCFString = CFStringGetNameOfEncoding(thisEncoding);
				if (nullptr == tableNameCFString) result = resNotFound;
				else
				{
					result = SetDataBrowserItemDataText(inItemData, tableNameCFString);
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
		case kMy_DataBrowserPropertyIDBaseCharacterSet:
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
static pascal Boolean
compareDataBrowserItems		(HIViewRef					UNUSED_ARGUMENT(inDataBrowser),
							 DataBrowserItemID			inItemOne,
							 DataBrowserItemID			inItemTwo,
							 DataBrowserPropertyID		inSortProperty)
{
	Boolean		result = false;
	
	
	switch (inSortProperty)
	{
	case kMy_DataBrowserPropertyIDBaseCharacterSet:
		result = (inItemOne < inItemTwo);
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// compareDataBrowserItems


/*!
Adjusts the views in preference panels to match the
specified change in dimensions of a panel’s container.

(3.1)
*/
static void
deltaSizePanelContainerHIView	(HIViewRef		inView,
								 Float32		inDeltaX,
								 Float32		inDeltaY,
								 void*			inContext)
{
	HIWindowRef					kPanelWindow = HIViewGetWindow(inView);
	My_TranslationsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inContext, My_TranslationsPanelUIPtr);
	HIViewWrap					viewWrap;
	
	
	viewWrap = HIViewWrap(idMyDataBrowserBaseTranslationTable, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, inDeltaY);
	
	viewWrap = HIViewWrap(idMyLabelOptions, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	viewWrap = HIViewWrap(idMyCheckBoxUseBackupFont, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	viewWrap = HIViewWrap(idMyButtonBackupFontName, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	viewWrap = HIViewWrap(idMyLabelBackupFontTest, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	viewWrap = HIViewWrap(idMyImageBackupFontTest, kPanelWindow);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	viewWrap = HIViewWrap(idMyHelpTextBackupFontTest, kPanelWindow);
	viewWrap << HIViewWrap_DeltaSize(inDeltaX, 0);
	viewWrap << HIViewWrap_MoveBy(0/* delta X */, inDeltaY);
	
	setDataBrowserColumnWidths(interfacePtr);
}// deltaSizePanelContainerHIView


/*!
This routine responds to a change in the existence
of a panel.  The panel is about to be destroyed, so
this routine disposes of private data structures
associated with the specified panel.

(3.1)
*/
static void
disposePanel	(Panel_Ref	UNUSED_ARGUMENT(inPanel),
				 void*		inDataPtr)
{
	My_TranslationsPanelDataPtr		dataPtr = REINTERPRET_CAST(inDataPtr, My_TranslationsPanelDataPtr);
	
	
	// destroy UI, if present
	if (nullptr != dataPtr->interfacePtr) delete dataPtr->interfacePtr;
	
	delete dataPtr, dataPtr = nullptr;
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
												kConstantsRegistry_PrefPanelDescriptorTranslations, 0/* options */));
	
	
	switch (inMessage)
	{
	case kPanel_MessageCreateViews: // specification of the window containing the panel - create controls using this window
		{
			My_TranslationsPanelDataPtr		panelDataPtr = REINTERPRET_CAST(Panel_ReturnImplementation(inPanel),
																			My_TranslationsPanelDataPtr);
			HIWindowRef const*				windowPtr = REINTERPRET_CAST(inDataPtr, HIWindowRef*);
			
			
			panelDataPtr->interfacePtr = new My_TranslationsPanelUI(inPanel, *windowPtr);
			assert(nullptr != panelDataPtr->interfacePtr);
			setDataBrowserColumnWidths(panelDataPtr->interfacePtr);
		}
		break;
	
	case kPanel_MessageDestroyed: // request to dispose of private data structures
		{
			void*		panelAuxiliaryDataPtr = inDataPtr;
			
			
			disposePanel(inPanel, panelAuxiliaryDataPtr);
		}
		break;
	
	case kPanel_MessageFocusGained: // notification that a control is now focused
		{
			//ControlRef const*	controlPtr = REINTERPRET_CAST(inDataPtr, ControlRef*);
			
			
			// do nothing
		}
		break;
	
	case kPanel_MessageFocusLost: // notification that a control is no longer focused
		{
			//ControlRef const*	controlPtr = REINTERPRET_CAST(inDataPtr, ControlRef*);
			
			
			// do nothing
		}
		break;
	
	case kPanel_MessageGetEditType: // request for panel to return whether or not it behaves like an inspector
		result = kPanel_ResponseEditTypeInspector;
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
			// do nothing
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
			//Boolean		isNowVisible = *(REINTERPRET_CAST(inDataPtr, Boolean*));
			
			
			// do nothing
		}
		break;
	
	default:
		break;
	}
	
	return result;
}// panelChanged


/*!
Handles "kEventControlClick" of "kEventClassControl"
for this preferences panel.

(3.1)
*/
static pascal OSStatus
receiveViewHit	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
				 EventRef				inEvent,
				 void*					inMyTranslationsPanelUIPtr)
{
	OSStatus					result = eventNotHandledErr;
	My_TranslationsPanelUIPtr	interfacePtr = REINTERPRET_CAST(inMyTranslationsPanelUIPtr, My_TranslationsPanelUIPtr);
	assert(nullptr != interfacePtr);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
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
			
			// UNIMPLEMENTED
		}
	}
	
	return result;
}// receiveViewHit


/*!
Sets the widths of the data browser columns
proportionately based on the total width.

(3.1)
*/
static void
setDataBrowserColumnWidths	(My_TranslationsPanelUIPtr		inInterfacePtr)
{
	Rect			containerRect;
	HIViewWrap		baseTableListContainer(idMyDataBrowserBaseTranslationTable, HIViewGetWindow(inInterfacePtr->mainView));
	assert(baseTableListContainer.exists());
	
	
	(OSStatus)GetControlBounds(baseTableListContainer, &containerRect);
	
	// set column widths proportionately
	{
		UInt16		kAvailableWidth = (containerRect.right - containerRect.left) - 16/* scroll bar width */;
		Float32		calculatedWidth = 0;
		UInt16		totalWidthSoFar = 0;
		
		
		// give all remaining space to the base table column
		calculatedWidth = kAvailableWidth - totalWidthSoFar;
		(OSStatus)SetDataBrowserTableViewNamedColumnWidth
					(baseTableListContainer, kMy_DataBrowserPropertyIDBaseCharacterSet,
						STATIC_CAST(calculatedWidth, UInt16));
	}
}// setDataBrowserColumnWidths

// BELOW IS REQUIRED NEWLINE TO END FILE
